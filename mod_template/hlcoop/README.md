# hlcoop mod template

This skeleton folder is what you copy into your xash3d-fwgs install
directory under `hlcoop/`.

After building, the gamedll appears as
`<repo>/build/3rdparty/hlcoop/output/dlls/server_amd64.so` (64-bit) or
`server.so` (32-bit).  Copy that file to `<install>/hlcoop/dlls/`.

The full mod content (maps, models, sprites, sounds, skill.cfg,
server.cfg, hlcoop.gsr, delta.lst, etc.) ships in the upstream
[SevenKewp_data](https://github.com/wootguy/SevenKewp_data) repo and
must be merged into this folder.  See
`3rdparty/hlcoop/PORT_README.md` for the full setup instructions.
