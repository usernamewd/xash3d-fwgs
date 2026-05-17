/*
 * rehlds.cpp - xash3d-fwgs port stub.  See rehlds.h for the rationale.
 */
#include "rehlds.h"
#include "util.h"

IRehldsApi*              g_RehldsApi          = NULL;
const RehldsFuncs_t*     g_RehldsFuncs        = NULL;
IRehldsServerData*       g_RehldsData         = NULL;
IRehldsHookchains*       g_RehldsHookchains   = NULL;
IRehldsServerStatic*     g_RehldsSvs          = NULL;

bool RehldsApi_Init()
{
    // ReHLDS is a HLDS-only extension - xash3d-fwgs is a clean engine
    // re-implementation and does not export the ReHLDS factory.  All
    // upstream call-sites null-check g_RehldsApi before use.
    return false;
}

void RegisterRehldsHooks()   {}
void UnregisterRehldsHooks() {}
