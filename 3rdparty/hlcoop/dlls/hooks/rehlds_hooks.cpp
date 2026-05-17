/*
 * rehlds_hooks.cpp - xash3d-fwgs port
 *
 * The upstream SevenKewp version uses ReHLDS-only hookchains to capture
 * raw clc_voicedata packets and to splice giant SVC_TEMPENTITY messages
 * directly into the per-client datagram.  xash3d-fwgs natively routes
 * voice data through the engine and supports oversized messages via the
 * standard MESSAGE_BEGIN/MSG_BROADCAST/MSG_ONE_UNRELIABLE paths, so the
 * single piece of game-visible behavior we still need - the "muted"
 * speaker icon - is reimplemented here using only stock HLSDK calls.
 *
 * RegisterRehldsHooks / UnregisterRehldsHooks are provided here as no-ops;
 * the canonical empty bodies live in rehlds.cpp so they're emitted exactly
 * once.  rehlds_SendBigMessage falls back to engine MESSAGE_BEGIN.
 */
#include "rehlds.h"
#include "util.h"
#include "PluginManager.h"
#include "CBasePlayer.h"

// show a mute icon and return true if the listener has muted the sender
bool handleMutedPlayer(int senderIdx, int listenerIdx, bool forceMute)
{
    static float lastIconTime[33][33];

    if (Voice_GetClientListening(listenerIdx, senderIdx) && !forceMute)
        return false;

    // indicate that the player is muted
    if (gpGlobals->time - lastIconTime[senderIdx][listenerIdx] > 0.15f
        || lastIconTime[senderIdx][listenerIdx] > gpGlobals->time)
    {
        MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, NULL, INDEXENT(listenerIdx));
        WRITE_BYTE(TE_PLAYERATTACHMENT);
        WRITE_BYTE(senderIdx);
        WRITE_COORD(45);
        WRITE_SHORT(MODEL_INDEX(GET_MODEL("sprites/voiceicon_m.spr")));
        WRITE_SHORT(2);
        MESSAGE_END();

        lastIconTime[senderIdx][listenerIdx] = gpGlobals->time;
    }

    return true;
}

// Without the ReHLDS direct-datagram path, fall back to the standard
// MESSAGE_BEGIN/WRITE_BYTE/MESSAGE_END route.  xash3d-fwgs natively
// fragments oversized messages; on a vanilla HLDS this would silently
// truncate, but xash supports this transparently.
static void rehlds_SendBigMessage_internal_fallback(int msgType, void* data, int sz, int playerindex)
{
    edict_t* dst = (playerindex >= 1 && playerindex <= gpGlobals->maxClients)
                       ? INDEXENT(playerindex) : NULL;
    if (dst == NULL || dst->free)
        return;

    MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, msgType, NULL, dst);
    for (int i = 0; i < sz; ++i)
        WRITE_BYTE(((uint8_t*)data)[i]);
    MESSAGE_END();
}

void rehlds_SendBigMessage(int msgMode, int msgType, void* data, int sz, int playerindex)
{
    CALL_HOOKS_VOID(pfnSendBigMessage, msgMode, msgType, data, sz, playerindex);

    bool pluginWantsMute = false;

    if (msgMode != MSG_BROADCAST)
    {
        if (msgType == SVC_VOICEDATA)
        {
            int senderIdx = ((uint8_t*)data)[0];
            CALL_HOOKS_VOID(pfnSendVoiceData, senderIdx + 1, playerindex,
                            ((uint8_t*)data) + 3, sz - 3, pluginWantsMute);
            if (handleMutedPlayer(senderIdx + 1, playerindex, pluginWantsMute))
                return;
        }

        if (playerindex > 0 && playerindex <= gpGlobals->maxClients)
            rehlds_SendBigMessage_internal_fallback(msgType, data, sz, playerindex);
        return;
    }

    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        CBasePlayer* plr = UTIL_PlayerByIndex(i);
        if (!plr)
            continue;

        if (msgType == SVC_VOICEDATA)
        {
            int senderIdx = ((uint8_t*)data)[0];
            CALL_HOOKS_VOID(pfnSendVoiceData, senderIdx + 1, playerindex,
                            ((uint8_t*)data) + 3, sz - 3, pluginWantsMute);
            if (pluginWantsMute)
                return;

            if (handleMutedPlayer(senderIdx + 1, playerindex, pluginWantsMute))
                continue;
        }

        rehlds_SendBigMessage_internal_fallback(msgType, data, sz, i);
    }
}
