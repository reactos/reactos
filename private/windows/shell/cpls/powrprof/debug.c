/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       DEBUG.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        21 Jan, 1997
*
*  DESCRIPTION:
*
*   Private debug routines implemented here.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntpoapi.h>
#include <stdio.h>
#include <powrprof.h>

#ifdef DEBUG

LPTSTR lpszDebugPwrState[] = {
    TEXT("PwrSysUnspecified"),
    TEXT("S0, PwrSysWorking"),
    TEXT("S1, PwrSysSleeping1"),
    TEXT("S2, PwrSysSleeping2"),
    TEXT("S3, PwrSysSleeping3"),
    TEXT("S4, PwrSysHibernate"),
    TEXT("S5, PwrSysShutdown"),
    TEXT("PwrSysMaximum")
};

LPTSTR lpszDebugPwrAction[] = {
    TEXT("PwrActionNone"),
    TEXT("PwrActionDoze"),
    TEXT("PwrActionSleep"),
    TEXT("PwrActionHibernate"),
    TEXT("PwrActionShutdown"),
    TEXT("PwrActionShutdownReset"),
    TEXT("PwrActionShutdownOff")
};

extern int g_iShowValidationChanges;

#endif

/*******************************************************************************
*
*  DebugPrintA
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/
void CDECL DebugPrintA(LPCSTR pszFmt, ...)
{
   char szDebug[MAX_PATH];

   va_list  arglist;

   va_start(arglist, pszFmt);
   wvsprintfA(szDebug, pszFmt, arglist);
   OutputDebugStringA(szDebug);
   OutputDebugStringA("\n");
}

#ifdef DEBUG
/*******************************************************************************
*
*  GetIndent
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/
#define MAX_INDENT 32

void GetIndent(LPSTR lpsz, LPSTR lpszIndent)
{
    UINT i;

    for (i = 0; i < MAX_INDENT; i++) {
        if (*lpsz != ' ') {
            break;
        }
        lpsz++;
        *lpszIndent++ = ' ';
    }
    *lpszIndent = '\0';
}

/*******************************************************************************
*
*  DumpPowerActionPolicy
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void DumpPowerActionPolicy(
    LPSTR                  lpszLabel,
    PPOWER_ACTION_POLICY   ppap
)
{
    char szLabel[MAX_INDENT], szIndent[MAX_INDENT];

    GetIndent(lpszLabel, szIndent);
    DebugPrint("%s, POWER_ACTION_POLICY: %08X", lpszLabel, ppap);
    if (ppap->Action <= PowerActionShutdownOff) {
        DebugPrint("%s  Action:    %08X, %s", szIndent, ppap->Action, lpszDebugPwrAction[ppap->Action]);
    }
    else {
        DebugPrint("%s  Action:    %08X, Invalid!", szIndent, ppap->Action);
    }
    DebugPrint("%s  Flags:     %08X", szIndent, ppap->Flags);
    DebugPrint("%s  EventCode: %08X", szIndent, ppap->EventCode);
}

/*******************************************************************************
*
*  DumpSystemPowerLevel
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void DumpSystemPowerLevel(
    LPSTR                  lpszLabel,
    PSYSTEM_POWER_LEVEL     pspl
)
{
    char   szLabel[MAX_INDENT], szIndent[MAX_INDENT];

    GetIndent(lpszLabel, szIndent);
    DebugPrint("%s, SYSTEM_POWER_LEVEL: %08X", lpszLabel, pspl);
    DebugPrint("%s  Enable:         %08X", szIndent, pspl->Enable);
    DebugPrint("%s  BatteryLevel:   %08X", szIndent, pspl->BatteryLevel);
    wsprintfA(szLabel, "%s  PowerPolicy", szIndent);
    DumpPowerActionPolicy(szLabel, &pspl->PowerPolicy);
    if (pspl->MinSystemState <= PowerSystemMaximum) {
        DebugPrint("%s  MinSystemState: %08X, %s", szIndent, pspl->MinSystemState, lpszDebugPwrState[pspl->MinSystemState]);
    }
    else {
        DebugPrint("%s  MinSystemState: %08X, Invalid!!", szIndent, pspl->MinSystemState);
    }
}

/*******************************************************************************
*
*  DumpSystemPowerPolicy
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void DumpSystemPowerPolicy(
    LPSTR                  lpszLabel,
    PSYSTEM_POWER_POLICY    pspp
)
{
    UINT    i;
    char   szLabel[MAX_INDENT], szIndent[MAX_INDENT];

    GetIndent(lpszLabel, szIndent);
    DebugPrint("%s, SYSTEM_POWER_POLICY: %08X", lpszLabel, pspp);
    DebugPrint("%s  Revision: %08X", szIndent, pspp->Revision);
    wsprintfA(szLabel, "%s  PowerButton", szIndent);
    DumpPowerActionPolicy( szLabel, &pspp->PowerButton);
    wsprintfA(szLabel, "%s  SleepButton", szIndent);
    DumpPowerActionPolicy( szLabel, &pspp->SleepButton);
    wsprintfA(szLabel, "%s  LidClose", szIndent);
    DumpPowerActionPolicy( szLabel,    &pspp->LidClose);
    DebugPrint("%s  LidOpenWake:                   %08X, %s", szIndent, pspp->LidOpenWake, lpszDebugPwrState[pspp->LidOpenWake]);
    wsprintfA(szLabel, "%s  Idle", szIndent);
    DumpPowerActionPolicy( szLabel, &pspp->Idle);
    DebugPrint("%s  IdleTimeout:                   %08X", szIndent, pspp->IdleTimeout);
    DebugPrint("%s  IdleSensitivity:               %08X", szIndent, pspp->IdleSensitivity);
    DebugPrint("%s  MinSleep:                      %08X, %s", szIndent, pspp->MinSleep, lpszDebugPwrState[pspp->MinSleep]);
    DebugPrint("%s  MaxSleep:                      %08X, %s", szIndent, pspp->MaxSleep, lpszDebugPwrState[pspp->MaxSleep]);
    DebugPrint("%s  ReducedLatencySleep:           %08X, %s", szIndent, pspp->ReducedLatencySleep, lpszDebugPwrState[pspp->ReducedLatencySleep]);
    DebugPrint("%s  WinLogonFlags:                 %08X", szIndent, pspp->WinLogonFlags);
    DebugPrint("%s  DozeS4Timeout:                 %08X", szIndent, pspp->DozeS4Timeout);
    DebugPrint("%s  BroadcastCapacityResolution:   %08X", szIndent, pspp->BroadcastCapacityResolution);
    for (i = 0; i < NUM_DISCHARGE_POLICIES; i++) {
        wsprintfA(szLabel, "%s  DischargePolicy[%d]", szIndent, i);
        DumpSystemPowerLevel( szLabel, &pspp->DischargePolicy[i]);
    }
    DebugPrint("%s  VideoTimeout:                  %08X", szIndent, pspp->VideoTimeout);
    DebugPrint("%s  VideoDimDisplay:               %08X", szIndent, pspp->VideoDimDisplay);
    DebugPrint("%s  SpindownTimeout:               %08X", szIndent, pspp->SpindownTimeout);
    DebugPrint("%s  OptimizeForPower:              %08X", szIndent, pspp->OptimizeForPower);
    DebugPrint("%s  FanThrottleTolerance:          %08X", szIndent, pspp->FanThrottleTolerance);
    DebugPrint("%s  ForcedThrottle:                %08X", szIndent, pspp->ForcedThrottle);
    DebugPrint("%s  MinThrottle:                   %08X", szIndent, pspp->MinThrottle);
    wsprintfA(szLabel, "%s  OverThrottled", szIndent);
    DumpPowerActionPolicy( szLabel, &pspp->OverThrottled);
    DebugPrint("\n\n");
}

/*******************************************************************************
*
*  DumpSystemPowerCapabilities
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void DumpSystemPowerCapabilities(
    LPSTR                      lpszLabel,
    PSYSTEM_POWER_CAPABILITIES  pspc
)
{
    UINT    i;
    char   szLabel[MAX_INDENT], szIndent[MAX_INDENT];

    GetIndent(lpszLabel, szIndent);
    DebugPrint("%s, SYSTEM_POWER_CAPABILITIES: %08X", lpszLabel, pspc);
    DebugPrint("%s  PowerButtonPresent:           %08X", szIndent, pspc->PowerButtonPresent);
    DebugPrint("%s  SleepButtonPresent:           %08X", szIndent, pspc->SleepButtonPresent);
    DebugPrint("%s  LidPresent:                   %08X", szIndent, pspc->LidPresent);
    DebugPrint("%s  SystemS1:                     %08X", szIndent, pspc->SystemS1);
    DebugPrint("%s  SystemS2:                     %08X", szIndent, pspc->SystemS2);
    DebugPrint("%s  SystemS3:                     %08X", szIndent, pspc->SystemS3);
    DebugPrint("%s  SystemS4:                     %08X", szIndent, pspc->SystemS4);
    DebugPrint("%s  SystemS5:                     %08X", szIndent, pspc->SystemS5);
    DebugPrint("%s  HiberFilePresent:             %08X", szIndent, pspc->HiberFilePresent);
    DebugPrint("%s  ThermalControl:               %08X", szIndent, pspc->ThermalControl);
    DebugPrint("%s  ProcessorThrottle:            %08X", szIndent, pspc->ProcessorThrottle);
    DebugPrint("%s  ProcessorMinThrottle:             %02X", szIndent, pspc->ProcessorMinThrottle);
    DebugPrint("%s  ProcessorThrottleScale:           %02X", szIndent, pspc->ProcessorThrottleScale);
    DebugPrint("%s  VideoDimPresent:              %08X", szIndent, pspc->VideoDimPresent);
    DebugPrint("%s  DiskSpinDown:                 %08X", szIndent, pspc->DiskSpinDown);
    DebugPrint("%s  SystemBatteriesPresent:       %08X", szIndent, pspc->SystemBatteriesPresent);
    DebugPrint("%s  BatteriesAreShortTerm:        %08X", szIndent, pspc->BatteriesAreShortTerm);

    // Not implemented:   BATTERY_REPORTING_SCALE BatteryScale[3];

    DebugPrint("%s  AcOnLineWake:                 %08X, %s", szIndent, pspc->AcOnLineWake, lpszDebugPwrState[pspc->AcOnLineWake]);
    DebugPrint("%s  SoftLidWake:                  %08X, %s", szIndent, pspc->SoftLidWake, lpszDebugPwrState[pspc->SoftLidWake]);
    DebugPrint("%s  RtcWake:                      %08X, %s", szIndent, pspc->RtcWake, lpszDebugPwrState[pspc->RtcWake]);
    DebugPrint("%s  MinDeviceWakeState:           %08X, %s", szIndent, pspc->MinDeviceWakeState, lpszDebugPwrState[pspc->MinDeviceWakeState]);
    DebugPrint("%s  DefaultLowLatencyWake:        %08X, %s", szIndent, pspc->DefaultLowLatencyWake, lpszDebugPwrState[pspc->DefaultLowLatencyWake]);
    DebugPrint("\n\n");
}

/*******************************************************************************
*
*  DifSystemPowerPolicies
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void DifSystemPowerPolicies(
    LPSTR               lpszLabel,
    PSYSTEM_POWER_POLICY pspp1,
    PSYSTEM_POWER_POLICY pspp2
)
{
    UINT    i;
    char   szLabel[MAX_INDENT], szIndent[MAX_INDENT];

    if (!g_iShowValidationChanges) {
        return;
    }

    if (memcmp(pspp1, pspp2, sizeof(SYSTEM_POWER_POLICY))) {

        GetIndent(lpszLabel, szIndent);
        DebugPrint("%s, spp1: %08X, spp2: %08X", lpszLabel, pspp1, pspp2);
        if (pspp1->Revision != pspp2->Revision) {
            DebugPrint("%s  Revision, spp1: %08X, spp2: %08X", szIndent, pspp1->Revision, pspp2->Revision);
        }
        if (memcmp(&pspp1->PowerButton, &pspp2->PowerButton, sizeof(POWER_ACTION_POLICY))) {
            wsprintfA(szLabel, "%s  spp1, PowerButton", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp1->PowerButton);
            wsprintfA(szLabel, "%s  spp2, PowerButton", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp2->PowerButton);
        }
        if (memcmp(&pspp1->SleepButton, &pspp2->SleepButton, sizeof(POWER_ACTION_POLICY))) {
            wsprintfA(szLabel, "%s  spp1, SleepButton", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp1->SleepButton);
            wsprintfA(szLabel, "%s  spp2, SleepButton", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp2->SleepButton);
        }
        if (memcmp(&pspp1->LidClose, &pspp2->LidClose, sizeof(POWER_ACTION_POLICY))) {
            wsprintfA(szLabel, "%s  spp1, LidClose", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp1->LidClose);
            wsprintfA(szLabel, "%s  spp2, LidClose:", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp2->LidClose);
        }
        if (pspp1->LidOpenWake != pspp2->LidOpenWake) {
            DebugPrint("%s  spp1, LidOpenWake: %08X, %s", szIndent, pspp1->LidOpenWake, lpszDebugPwrState[pspp1->LidOpenWake]);
            DebugPrint("%s  spp2, LidOpenWake: %08X, %s", szIndent, pspp2->LidOpenWake, lpszDebugPwrState[pspp2->LidOpenWake]);
        }
        if (memcmp(&pspp1->Idle, &pspp2->Idle, sizeof(POWER_ACTION_POLICY))) {
            wsprintfA(szLabel, "%s  spp1, Idle", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp1->Idle);
            wsprintfA(szLabel, "%s  spp2, Idle", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp2->Idle);
        }
        if (pspp1->IdleTimeout != pspp2->IdleTimeout) {
            DebugPrint("%s  IdleTimeout, spp1: %08X, spp2: %08X", szIndent, pspp1->IdleTimeout, pspp2->IdleTimeout);
        }
        if (pspp1->IdleSensitivity != pspp2->IdleSensitivity) {
            DebugPrint("%s  IdleSensitivity, spp1: %08X, spp2: %08X", szIndent, pspp1->IdleSensitivity, pspp2->IdleSensitivity);
        }
        if (pspp1->MinSleep != pspp2->MinSleep) {
            DebugPrint("%s  MinSleep, spp1: %08X, %s", szIndent, pspp1->MinSleep, lpszDebugPwrState[pspp1->MinSleep]);
            DebugPrint("%s  MinSleep, spp2: %08X, %s", szIndent, pspp2->MinSleep, lpszDebugPwrState[pspp2->MinSleep]);
        }
        if (pspp1->MaxSleep != pspp2->MaxSleep) {
            DebugPrint("%s  MaxSleep, spp1: %08X, %s", szIndent, pspp1->MaxSleep, lpszDebugPwrState[pspp1->MaxSleep]);
            DebugPrint("%s  MaxSleep, spp2: %08X, %s", szIndent, pspp2->MaxSleep, lpszDebugPwrState[pspp2->MaxSleep]);
        }
        if (pspp1->ReducedLatencySleep != pspp2->ReducedLatencySleep) {
            DebugPrint("%s  ReducedLatencySleep, spp1: %08X, %s", szIndent, pspp1->ReducedLatencySleep, lpszDebugPwrState[pspp1->ReducedLatencySleep]);
            DebugPrint("%s  ReducedLatencySleep, spp2: %08X, %s", szIndent, pspp2->ReducedLatencySleep, lpszDebugPwrState[pspp2->ReducedLatencySleep]);
        }
        if (pspp1->WinLogonFlags != pspp2->WinLogonFlags) {
            DebugPrint("%s  WinLogonFlags, spp1: %08X, spp2: %08X", szIndent, pspp1->WinLogonFlags, pspp2->WinLogonFlags);
        }
        if (pspp1->DozeS4Timeout != pspp2->DozeS4Timeout) {
            DebugPrint("%s  DozeS4Timeout, spp1: %08X, spp2: %08X", szIndent, pspp1->DozeS4Timeout, pspp2->DozeS4Timeout);
        }
        if (pspp1->BroadcastCapacityResolution != pspp2->BroadcastCapacityResolution) {
            DebugPrint("%s  BroadcastCapacityResolution, spp1: %08X, spp2: %08X", szIndent, pspp1->BroadcastCapacityResolution, pspp2->BroadcastCapacityResolution);
        }
        for (i = 0; i < NUM_DISCHARGE_POLICIES; i++) {
            if (memcmp(&(pspp1->DischargePolicy[i]), &(pspp2->DischargePolicy[i]), sizeof(SYSTEM_POWER_LEVEL))) {
                wsprintfA(szLabel, "%s  DischargePolicy[%d], spp1:", szIndent, i);
                DumpSystemPowerLevel( szLabel, &pspp1->DischargePolicy[i]);
                wsprintfA(szLabel, "%s  DischargePolicy[%d], spp2:", szIndent, i);
                DumpSystemPowerLevel( szLabel, &pspp2->DischargePolicy[i]);
            }
        }
        if (pspp1->VideoTimeout != pspp2->VideoTimeout) {
            DebugPrint("%s  VideoTimeout, spp1: %08X, spp2: %08X", szIndent, pspp1->VideoTimeout, pspp2->VideoTimeout);
        }
        if (pspp1->VideoDimDisplay != pspp2->VideoDimDisplay) {
            DebugPrint("%s  VideoDimDisplay, spp1: %08X, spp2: %08X", szIndent, pspp1->VideoDimDisplay, pspp2->VideoDimDisplay);
        }
        if (pspp1->SpindownTimeout != pspp2->SpindownTimeout) {
            DebugPrint("%s  SpindownTimeout, spp1: %08X, spp2: %08X", szIndent, pspp1->SpindownTimeout, pspp2->SpindownTimeout);
        }
        if (pspp1->OptimizeForPower != pspp2->OptimizeForPower) {
            DebugPrint("%s  OptimizeForPower, spp1: %08X, spp2: %08X", szIndent, pspp1->OptimizeForPower, pspp2->OptimizeForPower);
        }
        if (pspp1->FanThrottleTolerance != pspp2->FanThrottleTolerance) {
            DebugPrint("%s  FanThrottleTolerance, spp1: %08X, spp2: %08X", szIndent, pspp1->FanThrottleTolerance, pspp2->FanThrottleTolerance);
        }
        if (pspp1->ForcedThrottle != pspp2->ForcedThrottle) {
            DebugPrint("%s  ForcedThrottle, spp1: %08X, spp2: %08X", szIndent, pspp1->ForcedThrottle, pspp2->ForcedThrottle);
        }
        if (pspp1->MinThrottle != pspp2->MinThrottle) {
            DebugPrint("%s  MinThrottle, spp1: %08X, spp2: %08X", szIndent, pspp1->MinThrottle, pspp2->MinThrottle);
        }
        if (memcmp(&pspp1->OverThrottled, &pspp2->OverThrottled, sizeof(POWER_ACTION_POLICY))) {
            wsprintfA(szLabel, "%s  spp1, OverThrottled", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp1->OverThrottled);
            wsprintfA(szLabel, "%s  spp2, OverThrottled", szIndent);
            DumpPowerActionPolicy( szLabel, &pspp2->OverThrottled);
        }
        DebugPrint("\n\n");
    }
}
#endif
