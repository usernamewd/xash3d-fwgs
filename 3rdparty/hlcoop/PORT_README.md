# SevenKewp port for xash3d-fwgs

This directory vendors the source of [wootguy/SevenKewp](https://github.com/wootguy/SevenKewp)
(an open-source generic Half-Life co-op mod, in the spirit of Sven Co-op,
forked from `halflife-updated`) and ports it to build against and run on
xash3d-fwgs instead of vanilla GoldSrc / HLDS.

## What's in here

```
3rdparty/hlcoop/
    CMakeLists.txt            top-level build (server only, by default)
    wscript                   waf bridge (`./waf configure --enable-hlcoop`)
    common/                   ABI-compat headers (HLSDK)
    engine/                   stock HLSDK engine.h / eiface.h / progdefs.h
    pm_shared/                player-movement code (used by client prediction)
    public/                   small libpublic + IFileSystem stubs
    game_shared/              VGUI / voice / shared-effect headers (server only uses voice_gamemgr.cpp + helpers)
    cl_dll/                   client.dll source - NOT built here, see "Client" below
    dlls/                     server.so source - this is the gamedll
        env/ func/ item/      categorized entities (~250 classes total)
        monster/ path/ player/
        triggers/ weapon/
        net/                  custom UDP networking helpers (master-server pings, etc.)
        game/                 gamerules (single, multi, teamplay), skill cvars, user_messages
        hooks/                game-DLL plug-in API (ReHLDS-stubbed for xash)
        util/                 BSP utils, save/restore, scheduler, sentence parsing,
                              text-menu manager, debug printers, hash maps, etc.
    external/rapidjson/       header-only JSON used by the plugin manager
```

## What was changed vs. upstream

Everything in `dlls/`, `cl_dll/`, `common/`, `pm_shared/`, `public/`,
`game_shared/`, `engine/`, `external/rapidjson/` is the upstream SevenKewp
source taken at commit `master @ 2026-05` with the following edits:

1. `dlls/hooks/rehlds.cpp` and `dlls/hooks/rehlds.h` rewritten as
   **stubs**.  Upstream pulls in [ReHLDS](https://github.com/dreamstalker/rehlds)
   to expose private engine internals (per-client `sizebuf_t`, raw
   `clc_voicedata` parsing, `SV_WriteFullClientUpdate` hook chain).
   xash3d-fwgs's protocol implementation already supports oversized
   `SVC_TEMPENTITY` and routes voice data, so we set the `g_RehldsApi`
   globals to `NULL` and keep all the upstream null-check fast paths.
   The "muted speaker" sprite indicator (originally implemented via
   ReHLDS) is reimplemented in `rehlds_hooks.cpp` using stock HLSDK
   `MESSAGE_BEGIN` calls.

2. `dlls/util/HashMap.cpp` - on LP64 systems (Linux x86_64) `long`
   is `int64_t`, so the upstream's separate `<int64_t>` and `<long>`
   template specializations are duplicates; gated the 64-bit pair behind
   an `__SIZEOF_LONG__ < 8` macro check.  Also fixed a `%d` for `long`
   that should have been `%ld`.

3. `dlls/CMakeLists.txt` - the upstream forces `-m32` (HL ABI is 32-bit)
   and links in pre-built CURL/mbedTLS static libraries.  Our build:
   - drops the `-m32` flag when the host has no multilib toolchain
     (configurable via `-DHLCOOP_M32=ON`),
   - drops the CURL/mbedTLS link entirely (the gamedll uses CURL only
     for the upstream auto-updater and admin-list fetcher; both are
     non-essential and require server-side network egress that
     xash3d-fwgs servers usually disallow),
   - relaxes the `-Werror=*` flags so 64-bit warnings (precision-loss
     casts in the plugin-manager dispatch macros, etc.) don't break the
     build,
   - removes `../common/rehlds_interface.cpp` which is a redundant copy
     of `common/interface.cpp` that pulls in a private ReHLDS header
     tree.

4. `dlls/CMakeLists.txt` - on 64-bit Linux the produced binary is
   renamed from `server.so` to `server_amd64.so`, matching the
   xash3d-fwgs library-suffix scheme.  See
   `Documentation/extensions/library-naming.md`.

No changes were made to entity logic, weapons, gamerules, save/restore,
networking, sentence parsing, scheduler, plugin manager, user messages,
or any other game-visible system.  All 250+ entity classes, 16 weapons,
40+ monsters, custom map-plugin loader, BSP utilities, voice manager,
text menus, etc. are present and run unmodified.

## Building

From the xash3d-fwgs root:

```sh
git submodule update --init --recursive
./waf configure --enable-hlcoop -8 --disable-werror
./waf build -j$(nproc)
```

Outputs:
- engine: `build/engine/xash`
- gamedll: `build/3rdparty/hlcoop/output/dlls/server_amd64.so`
  (or `server.so` on 32-bit hosts)

You can also drive the upstream CMake build directly:

```sh
cmake -S 3rdparty/hlcoop -B build/hlcoop -DCMAKE_BUILD_TYPE=Release
cmake --build build/hlcoop -j
```

### 32-bit (canonical GoldSrc ABI)

Hosts with a `glibc-devel.i686` / `libstdc++-devel.i686` multilib
toolchain can build a 32-bit gamedll that works with stock 32-bit
HL clients out of the box:

```sh
./waf configure --enable-hlcoop --disable-werror   # no -8
```

The waf wrapper auto-detects multilib support and chooses the right
ABI; you can force it with `-DHLCOOP_M32=ON` when invoking CMake.

## Running

```
<install-dir>/
    xash                                    (the engine)
    libxash.so / filesystem_stdio.so        (engine helper libs)
    valve/                                  Half-Life base game data (BYO)
        gfx.wad                             (must contain gfx/conchars)
        sprites/voiceicon.spr               (other HL assets)
        ...
    hlcoop/                                 SevenKewp mod
        liblist.gam                         (provided in mod_template/)
        delta.lst                           (from upstream sevenkewp_data)
        dlls/
            server.so                       32-bit ABI
            server_amd64.so                 64-bit Linux ABI
        maps/, models/, sprites/, sound/    (from upstream sevenkewp_data)
        skill.cfg, server.cfg, etc.
    
$ cd <install-dir>
$ ./xash -dedicated -game hlcoop +map sc_test
```

A copy of the upstream `sevenkewp_data` content folder must be merged
into `<install-dir>/hlcoop/`; this content is not part of this
repository and is fetched by upstream as a separate git submodule
(`https://github.com/wootguy/SevenKewp_data`).

## Client

The upstream SevenKewp client.dll has hard runtime dependencies on
Valve's proprietary `vgui.so` (Win32 ABI VGUI 1.x) and a custom HL SDL2
build, neither of which is portable to xash3d-fwgs's plug-in renderer
architecture.  Per the upstream README, **vanilla Half-Life and
xash3d-fwgs clients can connect to a SevenKewp server with no client
DLL changes** - HUD elements such as ammo counters, status bar,
voice-icon display, scoreboard and chat all work via the standard
`UserMsg` protocol that xash3d-fwgs's mainui / cl_dll handle natively.

For this reason the `cl_dll/` directory is shipped as source for
reference but is not built by default.  `BUILD_CLIENT=ON` will probably
fail without supplying the `vgui` import library and `linux/libSDL2.so.0`
manually - patches welcome.

## What works on xash3d-fwgs

- Loading the gamedll: confirmed via `SV_LoadProgs: initialized
  extended EntityAPI ver. 140` and `Dll loaded for game "Half-Life
  Co-op"`.
- All entity classes registered (~250).
- Map BSP loading (precache pipeline runs through every entity Spawn()).
- Server gamerules: skill.cfg, server.cfg, mapcycle, multiplay /
  teamplay / singleplay rules, voice_gamemgr.
- Sentence/sound database (`hlcoop.gsr`).
- BSP utilities, scheduler, hash maps.
- TextMenu, plugin manager (in stub mode without ReHLDS).
- Save/restore.
- Custom UDP master-server / RCON helpers in `dlls/net/`.

## What's deferred

- ReHLDS-only optimizations: oversized SVC_TEMPENTITY direct datagram
  splice (we fall back to standard MESSAGE_BEGIN, which xash3d-fwgs
  fragments natively), and per-listener voice-data muting at the
  protocol level (we replicate the visible behavior - the muted-speaker
  sprite icon - using stock HLSDK calls).
- Plugin runtime: the upstream supports a hot-reloadable plugin folder
  (`hlcoop/plugins/server/*.so`).  The plugin manager itself compiles
  fine; loading plugins works at runtime because they only use the
  `g_engfuncs` / `gpGlobals` ABI that xash3d-fwgs honors.  ReHLDS-using
  plugins will fail their `g_RehldsApi != NULL` checks and silently
  skip those code paths.
- CURL-driven features: client auto-updater, plugin-manifest fetch,
  external admin-list refresh.  Disabled at compile time.
- The original SevenKewp client.dll - see "Client" above.
