/*
 * rehlds.h - xash3d-fwgs port stub
 *
 * The upstream SevenKewp mod uses ReHLDS-only APIs (low-level netchan
 * routing, voice-data multiplexing, full-client-update hook) to deliver
 * larger-than-512-byte SVC_TEMPENTITY messages and per-listener voice
 * muting to vanilla GoldSrc HLDS.  xash3d-fwgs already supports the
 * needed protocol extensions natively (Xash supports BF_WriteByte
 * messages exceeding 512 bytes via fragmentation, and routes SVC_VOICEDATA
 * itself), so we simply stub out the ReHLDS API: every ReHLDS-dependent
 * code path in the mod is gated behind a NULL check on these globals
 * and gracefully no-ops when the API is unavailable, exactly as it does
 * on a vanilla (non-ReHLDS) HLDS.  No game-logic features are lost; only
 * a few server-only optimizations are deferred.
 */
#pragma once

#include "extdll.h"

// Forward-only declarations are sufficient for the upstream code, which
// only ever uses these types behind pointers.
struct IRehldsApi;
struct IRehldsServerData;
struct IRehldsHookchains;
struct IRehldsServerStatic;
struct RehldsFuncs_t;

bool RehldsApi_Init();
void RegisterRehldsHooks();
void UnregisterRehldsHooks();

EXPORT extern IRehldsApi*              g_RehldsApi;
EXPORT extern const RehldsFuncs_t*     g_RehldsFuncs;
EXPORT extern IRehldsServerData*       g_RehldsData;
EXPORT extern IRehldsHookchains*       g_RehldsHookchains;
EXPORT extern IRehldsServerStatic*     g_RehldsSvs;

// Send a big network message (used for SVC_VOICEDATA and oversized
// SVC_TEMPENTITY).  On xash3d-fwgs we just route through the standard
// MESSAGE_BEGIN / WRITE_* / MESSAGE_END path - the engine handles
// splitting transparently.
EXPORT void rehlds_SendBigMessage(int msgMode, int msgType, void* data, int sz, int playerindex);
