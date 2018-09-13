/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       MAKEINF.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        1 Jan, 1997
*
*  DESCRIPTION:
*   Main code for the default power schemes INF generator, MAKEINF.EXE.
*   Generates INF file which can be read by Memphis setup.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <windows.h>
#include <regstr.h>
#include <ntpoapi.h>

#include "powrprof.h"
#include "..\ini\parse.h"

#define SRC_NAME            "..\\ini\\DEFAULTS.CSV"
#define INF_NAME            "winnt\\POWERCFG.INF"
#define TYPICAL_INF         "win95\\TYPICAL.INF"
#define COMPACT_INF         "win95\\COMPACT.INF"
#define CUSTOM_INF          "win95\\CUSTOM.INF"
#define PORTABLE_INF        "win95\\PORTABLE.INF"
#define SERVER_INF          "win95\\SERVER.INF"

#define MAX_PROFILES        128
#define MAX_LINE_SIZE       1024
#define DATA_REV            1

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

char *g_pszSrc;
char *g_pszLines[MAX_LINES];

UINT g_uiLineCount;
UINT g_uiPoliciesCount;

UINT g_uiIDS = 4000;

PMACHINE_POWER_POLICY       g_pmpp[MAX_PROFILES];
PUSER_POWER_POLICY          g_pupp[MAX_PROFILES];
UINT                        g_uiPolicyInfTypes[MAX_PROFILES];
UINT                        g_uiPolicyOSTypes[MAX_PROFILES];

GLOBAL_USER_POWER_POLICY    g_gupp;
GLOBAL_MACHINE_POWER_POLICY g_gmpp;

/*******************************************************************************
*
*  DefFatalExit
*
*  DESCRIPTION:
*   Print error and exit.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID CDECL DefFatalExit(BOOLEAN bGetLastError, char *pszFormat, ... )
{
    va_list Marker;

    va_start(Marker, pszFormat);
    printf("\n\n");
    vprintf(pszFormat, Marker);
    if (bGetLastError) {
         printf("Last error: %d\n", GetLastError());
    }
    printf("\n\nDefault Processing Failure. Output files are invalid.\n");
    exit(1);
}

/*******************************************************************************
*
*  ReadSource
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ReadSource(void)
{
    HANDLE  fh;
    DWORD   dwSize, dwRead;
    BOOLEAN bRet = FALSE;

    fh = CreateFile(SRC_NAME, GENERIC_READ,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);

    if (fh != INVALID_HANDLE_VALUE) {

        // Allocate the source buffer.
        if ((dwSize = GetFileSize(fh, NULL)) != INVALID_FILE_SIZE) {
            if ((g_pszSrc = (char *) malloc(dwSize)) != NULL) {
                // Read in the file buffer.
                SetFilePointer(fh, 0, NULL, FILE_BEGIN);
                if (ReadFile(fh, g_pszSrc, dwSize, &dwRead, NULL)) {
                    printf("ReadSource successful.\n");
                    bRet = TRUE;
                }
            }
        }
        CloseHandle(fh);
    }
    if (!bRet) {
        DefFatalExit(TRUE, "ReadSource failed reading: %s\n", SRC_NAME);
    }
    return bRet;
}

/*******************************************************************************
*
*  BuildLineArray
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

UINT BuildLineArray(void)
{
    char    *psz;

    printf("\nBuilding the line array.");
    g_uiLineCount = GetTokens(g_pszSrc, MAX_LINE_SIZE,
                              g_pszLines, MAX_LINES, LINE_DELIMITERS);
    printf("\nFound %d lines.\n\n", g_uiLineCount);
    return g_uiLineCount;
}

/*******************************************************************************
*
*  GetSleepActionFlags
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID GetSleepActionFlags(
    UINT    uiStartLine,
    UINT    uiFlagsAC[],
    UINT    uiFlagsDC[],
    PUINT   puiCount
)
{
    UINT    i;

    for (i = 0; i < *puiCount; i++) {
        uiFlagsAC[i] = 0;
        uiFlagsDC[i] = 0;
    }
    GetCheckLabelToken(uiStartLine, "Sleep action flags");
    GetCheckLabelToken(uiStartLine+QUERY_APPS, "Query apps");
    for (i = 0; i < *puiCount; i++) {
        uiFlagsAC[i] += GetFlagToken(POWER_ACTION_QUERY_ALLOWED);
        uiFlagsDC[i] += GetFlagToken(POWER_ACTION_QUERY_ALLOWED);
    }
    GetCheckLabelToken(uiStartLine+ALLOW_UI, "Allow UI");
    for (i = 0; i < *puiCount; i++) {
        uiFlagsAC[i] += GetFlagToken(POWER_ACTION_UI_ALLOWED);
        uiFlagsDC[i] += GetFlagToken(POWER_ACTION_UI_ALLOWED);
    }
    GetCheckLabelToken(uiStartLine+IGNORE_NON_RESP, "Ignore non-responsive apps");
    for (i = 0; i < *puiCount; i++) {
        uiFlagsAC[i] += GetFlagToken(POWER_ACTION_OVERRIDE_APPS);
        uiFlagsDC[i] += GetFlagToken(POWER_ACTION_OVERRIDE_APPS);
    }
    GetCheckLabelToken(uiStartLine+IGNORE_WAKE, "Ignore wakeup events");
    for (i = 0; i < *puiCount; i++) {
        uiFlagsAC[i] += GetFlagToken(POWER_ACTION_DISABLE_WAKES);
        uiFlagsDC[i] += GetFlagToken(POWER_ACTION_DISABLE_WAKES);
    }
    GetCheckLabelToken(uiStartLine+IGNORE_CRITICAL, "Critical (go to sleep immediately)");
    for (i = 0; i < *puiCount; i++) {
        uiFlagsAC[i] += GetFlagToken(POWER_ACTION_CRITICAL);
        uiFlagsDC[i] += GetFlagToken(POWER_ACTION_CRITICAL);
    }
}

/*******************************************************************************
*
*  GetSleepActionFlagsGlobal
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID GetSleepActionFlagsGlobal(
    UINT                    uiStartLine,
    PPOWER_ACTION_POLICY    ppapAC,
    PPOWER_ACTION_POLICY    ppapDC
)
{
    UINT    uiOne = 1;
    UINT    uiFlagsAC[MAX_PROFILES];
    UINT    uiFlagsDC[MAX_PROFILES];

    GetSleepActionFlags(uiStartLine, uiFlagsAC, uiFlagsDC, &uiOne);
    if (ppapAC) {
        ppapAC->Flags = uiFlagsAC[0];
    }
    ppapDC->Flags = uiFlagsDC[0];
}

/*******************************************************************************
*
*  GetSleepActionFlagsPolicy
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID GetSleepActionFlagsUserPolicy(
    UINT    uiStartLine,
    ULONG   ulAcPowerPolicyOffset,
    ULONG   ulDcPowerPolicyOffset,
    PUINT   puiCount
)
{
    UINT    i;
    UINT    uiFlagsAC[MAX_PROFILES];
    UINT    uiFlagsDC[MAX_PROFILES];
    PPOWER_ACTION_POLICY ppap;

    GetSleepActionFlags(uiStartLine, uiFlagsAC, uiFlagsDC, puiCount);
    for (i = 0; i < *puiCount; i++) {

        ppap = (PPOWER_ACTION_POLICY)((BYTE*)(g_pupp[i]) + ulAcPowerPolicyOffset);
        ppap->Flags = uiFlagsAC[i];

        ppap = (PPOWER_ACTION_POLICY)((BYTE*)(g_pupp[i]) + ulDcPowerPolicyOffset);
        ppap->Flags = uiFlagsDC[i];
    }
}

/*******************************************************************************
*
*  GetSleepActionFlagsMachinePolicy
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID GetSleepActionFlagsMachinePolicy(
    UINT    uiStartLine,
    ULONG   ulAcPowerPolicyOffset,
    ULONG   ulDcPowerPolicyOffset,
    PUINT   puiCount
)
{
    UINT    i;
    UINT    uiFlagsAC[MAX_PROFILES];
    UINT    uiFlagsDC[MAX_PROFILES];
    PPOWER_ACTION_POLICY ppap;

    GetSleepActionFlags(uiStartLine, uiFlagsAC, uiFlagsDC, puiCount);
    for (i = 0; i < *puiCount; i++) {
        ppap = (PPOWER_ACTION_POLICY)((BYTE*)(g_pmpp[i]) + ulAcPowerPolicyOffset);
        ppap->Flags = uiFlagsAC[i];

        ppap = (PPOWER_ACTION_POLICY)((BYTE*)(g_pmpp[i]) + ulDcPowerPolicyOffset);
        ppap->Flags = uiFlagsDC[i];
    }
}

/*******************************************************************************
*
*  GetPolicies
*
*  DESCRIPTION:
*   Parse the power policies into the USER: and HKEY_LOCAL_MACHINE global
*   power policies structures arrays.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN GetPolicies(void)
{
    UINT    i;
    BOOLEAN bRet = FALSE;

    printf("Parsing power policies\n");

    // First get a place to put the data.
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i] = (PUSER_POWER_POLICY)malloc(sizeof(USER_POWER_POLICY));
        g_pmpp[i] = (PMACHINE_POWER_POLICY)malloc(sizeof(MACHINE_POWER_POLICY));
        if (!g_pupp[i] || !g_pmpp[i]) {
            goto gp_leave;
        }
    }

    // Initialize revision data.
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->Revision = DATA_REV;
        g_pmpp[i]->Revision = DATA_REV;
    }

    printf("  Allocated policies\n");

    //**********************************************************************
    // INF type and OS Type
    //**********************************************************************
    GetCheckLabelToken(PLATFORM_LINE, "Platform");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_uiPolicyOSTypes[i]  = GetOSTypeToken();
    }
    GetCheckLabelToken(INSTALL_ON_LINE, "Install On");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_uiPolicyInfTypes[i] = GetINFTypeToken();
    }
    printf("  Parsed INF and OS Types\n");

    //**********************************************************************
    // System Idle
    //**********************************************************************
    GetCheckLabelToken(SYSTEM_IDLE_LINE, "Idle action");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->IdleAc.Action = GetPowerActionToken();
        g_pupp[i]->IdleDc.Action = GetPowerActionToken();
    }
    GetCheckLabelToken(SYSTEM_IDLE_TIMEOUT_LINE, "Idle timeout");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->IdleTimeoutAc = GetIntToken("MIN");
        g_pupp[i]->IdleTimeoutDc = GetIntToken("MIN");
    }
    GetSleepActionFlagsUserPolicy(SYSTEM_IDLE_SLEEP_ACTION_FLAGS_LINE,
                                  FIELD_OFFSET(USER_POWER_POLICY, IdleAc),
                                  FIELD_OFFSET(USER_POWER_POLICY, IdleDc),
                                  &g_uiPoliciesCount);
    GetCheckLabelToken(SYSTEM_IDLE_SENSITIVITY_LINE, "Idle sensitivity");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->IdleSensitivityAc = GetIntToken("%");
        g_pupp[i]->IdleSensitivityDc = GetIntToken("%");
    }
    printf("  Parsed System Idle Policies\n");

    //**********************************************************************
    // Sleep Policies
    //**********************************************************************
    GetCheckLabelToken(MIN_SLEEP_LINE, "Minimum sleep");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pmpp[i]->MinSleepAc = GetPowerStateToken();
        g_pmpp[i]->MinSleepDc = GetPowerStateToken();
    }
    GetCheckLabelToken(MAX_SLEEP_LINE, "Max sleep");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->MaxSleepAc = GetPowerStateToken();
        g_pupp[i]->MaxSleepDc = GetPowerStateToken();
    }
    GetCheckLabelToken(REDUCED_LATENCY_SLEEP_LINE, "Reduced latency sleep");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pmpp[i]->ReducedLatencySleepAc = GetPowerStateToken();
        g_pmpp[i]->ReducedLatencySleepDc = GetPowerStateToken();
    }
    GetCheckLabelToken(DOZE_TIMEOUT_LINE, "Doze timeout");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pmpp[i]->DozeTimeoutAc = GetIntToken("MIN");
        g_pmpp[i]->DozeTimeoutDc = GetIntToken("MIN");
    }
    GetCheckLabelToken(DOZE_S4_TIMEOUT_LINE, "DozeS4Timeout");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pmpp[i]->DozeS4TimeoutAc = GetIntToken("MIN");
        g_pmpp[i]->DozeS4TimeoutDc = GetIntToken("MIN");
    }

    printf("  Parsed Sleep Policies\n");

    //**********************************************************************
    // Device Power Management
    //**********************************************************************
    GetCheckLabelToken(VIDEO_TIMEOUT_LINE, "Video timeout");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->VideoTimeoutAc = GetIntToken("MIN");
        g_pupp[i]->VideoTimeoutDc = GetIntToken("MIN");
    }
    GetCheckLabelToken(SPINDOWN_TIMEOUT_LINE, "Spindown timeout");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->SpindownTimeoutAc = GetIntToken("MIN");
        g_pupp[i]->SpindownTimeoutDc = GetIntToken("MIN");
    }
    printf("  Parsed Device Power Management Policies\n");


    //**********************************************************************
    // CPU Policies
    //**********************************************************************
    GetCheckLabelToken(OPTIMIZE_FOR_POWER_LINE, "Optimize for power");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->OptimizeForPowerAc = GetFlagToken(TRUE);
        g_pupp[i]->OptimizeForPowerDc = GetFlagToken(TRUE);
    }
    GetCheckLabelToken(FAN_THROTTLE_TOL_LINE, "Fan throttle Tolerance");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->FanThrottleToleranceAc = GetIntToken("%");
        g_pupp[i]->FanThrottleToleranceDc = GetIntToken("%");
    }
    GetCheckLabelToken(FORCED_THROTTLE_LINE, "Forced throttle");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pupp[i]->ForcedThrottleAc = GetIntToken("%");
        g_pupp[i]->ForcedThrottleDc = GetIntToken("%");
    }
    GetCheckLabelToken(MIN_THROTTLE_LINE, "Min throttle");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pmpp[i]->MinThrottleAc = GetIntToken("%");
        g_pmpp[i]->MinThrottleDc = GetIntToken("%");
    }
    GetCheckLabelToken(OVERTHROTTLED_LINE, "Overthrottled");
    for (i = 0; i < g_uiPoliciesCount; i++) {
        g_pmpp[i]->OverThrottledAc.Action = GetPowerActionToken();
        g_pmpp[i]->OverThrottledDc.Action = GetPowerActionToken();
    }
    GetSleepActionFlagsMachinePolicy(OVERTHROTTLED_SLEEP_ACTION_FLAGS_LINE,
                                     FIELD_OFFSET(MACHINE_POWER_POLICY, OverThrottledAc),
                                     FIELD_OFFSET(MACHINE_POWER_POLICY, OverThrottledDc),
                                     &g_uiPoliciesCount);

    printf("  Parsed CPU Policies\n");

    bRet = TRUE;
    printf("Parsing power policies success!\n\n");

gp_leave:
    if (!bRet) {
        printf("GetPolicies failed, Last Error: %d\n", GetLastError());
    }
    return bRet;
}

/*******************************************************************************
*
*  GetDischargePolicies
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID GetDischargePolicies(
    UINT uiLine,
    UINT uiNum,
    UINT uiIndex
)
{
    UINT    i, uiOne = 1;
    char szLabel[] = "Discharge Policy 1";

    sprintf(szLabel, "Discharge Policy %d", uiNum);
    GetCheckLabelToken(uiLine, szLabel);

    GetCheckLabelToken(uiLine+DP_ENABLE, "Enable");
    GetNAToken();
    g_gupp.DischargePolicy[uiIndex].Enable = GetFlagToken(TRUE);

    GetCheckLabelToken(uiLine+DP_BAT_LEVEL, "Battery Level");
    GetNAToken();
    g_gupp.DischargePolicy[uiIndex].BatteryLevel = GetIntToken("%");

    GetCheckLabelToken(uiLine + DP_POWER_POLICY, "Power Policy");
    GetNAToken();
    g_gupp.DischargePolicy[uiIndex].PowerPolicy.Action = GetPowerActionToken();

    GetCheckLabelToken(uiLine + DP_MIN_SLEEP_STATE, "Min system sleep state");
    GetNAToken();
    g_gupp.DischargePolicy[uiIndex].MinSystemState = GetPowerStateToken();

    GetCheckLabelToken(uiLine + DP_TEXT_NOTIFY, "Text Notify");
    GetNAToken();
    g_gupp.DischargePolicy[uiIndex].PowerPolicy.EventCode =
        GetFlagToken(POWER_LEVEL_USER_NOTIFY_TEXT);
    GetCheckLabelToken(uiLine+DP_SOUND_NOTIFY, "Sound Notify");
    GetNAToken();
    g_gupp.DischargePolicy[uiIndex].PowerPolicy.EventCode |=
        GetFlagToken(POWER_LEVEL_USER_NOTIFY_SOUND);

    GetSleepActionFlagsGlobal(uiLine + DP_SLEEP_ACT_FLAGS, NULL,
                              &(g_gupp.DischargePolicy[uiIndex].PowerPolicy));

    printf("  Parsed %s\n", szLabel);
}

/*******************************************************************************
*
*  GetGlobalPolicies
*
*  DESCRIPTION:
*   Parse the global policies into the USER: and HKEY_LOCAL_MACHINE global
*   power policies structures.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN GetGlobalPolicies(void)
{
    UINT    i, uiOne = 1;

    printf("Parsing global power policies\n");

    // Set both User and Local Machine revision levels
    g_gupp.Revision = DATA_REV;
    g_gmpp.Revision = DATA_REV;

    //**********************************************************************
    // Advanced
    //**********************************************************************
    GetCheckLabelToken(ADVANCED_LINE, "Advanced");
    GetCheckLabelToken(LOCK_ON_SLEEP_LINE, "Lock Workstation");
    g_gupp.GlobalFlags = GetFlagToken(EnablePasswordLogon);
    if (g_gupp.GlobalFlags != GetFlagToken(EnablePasswordLogon)) {
            DefFatalExit(FALSE,"AC and DC Lock Workstation entries don't match");
    }

    GetCheckLabelToken(WAKE_ON_RING_LINE, "Wake on Ring");
    g_gupp.GlobalFlags |= GetFlagToken(EnableWakeOnRing);
    if ((g_gupp.GlobalFlags & EnableWakeOnRing) !=
        GetFlagToken(EnableWakeOnRing)) {
            DefFatalExit(FALSE,"AC and DC Wake on Ring entries don't match");
    }

    GetCheckLabelToken(VIDEO_DIM_DISPLAY_LINE, "Video Dim Display on DC");
    GetFlagToken(0);
    g_gupp.GlobalFlags |= GetFlagToken(EnableVideoDimDisplay);

    //**********************************************************************
    // Power button
    //**********************************************************************
    GetCheckLabelToken(POWER_BUTTON_LINE, "Power button");

    g_gupp.PowerButtonAc.Action = GetPowerActionToken();
    g_gupp.PowerButtonDc.Action = GetPowerActionToken();
    GetSleepActionFlagsGlobal(POWER_BUTTON_SLEEP_ACTION_FLAGS_LINE,
                              &(g_gupp.PowerButtonAc),
                              &(g_gupp.PowerButtonDc));
    printf("  Parsed Power Button Policies\n");

    //**********************************************************************
    // Sleep button
    //**********************************************************************
    GetCheckLabelToken(SLEEP_BUTTON_LINE, "Sleep button");

    g_gupp.SleepButtonAc.Action = GetPowerActionToken();
    g_gupp.SleepButtonDc.Action = GetPowerActionToken();
    GetSleepActionFlagsGlobal(SLEEP_BUTTON_SLEEP_ACTION_FLAGS_LINE,
                              &(g_gupp.SleepButtonAc),
                              &(g_gupp.SleepButtonDc));
    printf("  Parsed Sleep Button Policies\n");

    //**********************************************************************
    // Lid Closed
    //**********************************************************************
    GetCheckLabelToken(LID_CLOSE_LINE, "Lid close");
    g_gupp.LidCloseAc.Action = GetPowerActionToken();
    g_gupp.LidCloseDc.Action = GetPowerActionToken();
    GetSleepActionFlagsGlobal(LID_CLOSE_SLEEP_ACTION_FLAGS_LINE,
                              &(g_gupp.LidCloseAc),
                              &(g_gupp.LidCloseDc));
    printf("  Parsed Lid Close Policies\n");

    //**********************************************************************
    // Lid Open Wake
    //**********************************************************************
    GetCheckLabelToken(LID_OPEN_WAKE_LINE, "Lid Open Wake");
    g_gmpp.LidOpenWakeAc = GetPowerStateToken();
    g_gmpp.LidOpenWakeDc = GetPowerStateToken();
    printf("  Parsed Lid Open Wake Policies\n");

    //**********************************************************************
    // Battery Policies
    //**********************************************************************
    GetCheckLabelToken(BROADCAST_CAP_RES_LINE, "Broadcast capacity resolution");
    GetIntToken(NULL);
    g_gmpp.BroadcastCapacityResolution = GetIntToken("%");
    GetCheckLabelToken(BATMETER_ENABLE_SYSTRAY_FLAG_LINE, "Force Systray Battery Meter");
    g_gupp.GlobalFlags |= GetFlagToken(EnableSysTrayBatteryMeter);
    GetFlagToken(0);
    GetCheckLabelToken(BATMETER_ENABLE_MULTI_FLAG_LINE, "Enable Multi-Battery Display");
    GetFlagToken(0);
    g_gupp.GlobalFlags |= GetFlagToken(EnableMultiBatteryDisplay);
    printf("  Parsed Battery Policies\n");

    //**********************************************************************
    // Discharge Policies 1, Low Battery
    //**********************************************************************
    GetDischargePolicies(DISCHARGE_POLICY_1_LINE, 1, DISCHARGE_POLICY_LOW);

    //**********************************************************************
    // Discharge Policies 2, Critical Battery
    //**********************************************************************
    GetDischargePolicies(DISCHARGE_POLICY_2_LINE, 2, DISCHARGE_POLICY_CRITICAL);

    return TRUE;
}

/*******************************************************************************
*
*  WriteRegBinary
*
*  DESCRIPTION:
*   Write binary data out to the registry specification file.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID WriteRegBinary(FILE *fInf, PVOID pv, UINT uiSize, char *pszIndent)
{
    PBYTE   pb = pv;
    UINT    uiRow = 0;

    while (uiSize) {
        if (uiSize > 1) {
            fprintf(fInf, "%02X,", *pb++);
        }
        else {
            fprintf(fInf, "%02X", *pb++);
        }
        uiSize--;
        if (uiRow++ == 15) {
            uiRow = 0;
            if (uiSize > 1) {
                fprintf(fInf, "\\\n%s", pszIndent);
            }
            else {
                fprintf(fInf, "\n");
            }
        }
    }

}

/*******************************************************************************
*
*  WriteInfHeader
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WriteInfHeader(FILE *fInf)
{
    fprintf(fInf, "; POWERCFG.INF\n");
    fprintf(fInf, "; Copyright (c) 1993-1997, Microsoft Corporation\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "[Version]\n");
    fprintf(fInf, "Signature  = \"$CHICAGO$\"\n");
    fprintf(fInf, "SetupClass = BASE\n");
    fprintf(fInf, "LayoutFile = layout.inf, layout1.inf\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "[DestinationDirs]\n");
    fprintf(fInf, "PowerCfg.copy.inf = 17   ; LDID_INF\n");
    fprintf(fInf, "PowerCfg.copy.sys = 11   ; LDID_SYS\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "[BaseWinOptions]\n");
    fprintf(fInf, "PowerCfg.base\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "[PowerCfg.base]\n");
    fprintf(fInf, "CopyFiles = PowerCfg.copy.inf, PowerCfg.copy.sys\n");
    fprintf(fInf, "AddReg    = PowerCfg.addreg\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "[PowerCfg.copy.inf]\n");
    fprintf(fInf, "; files to copy to \\windows\\inf directory\n");
    fprintf(fInf, "PowerCfg.inf\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "[PowerCfg.copy.sys]\n");
    fprintf(fInf, "; files to copy to \\windows\\system directory\n");
    fprintf(fInf, "powercfg.cpl\n");
    fprintf(fInf, "powrprof.dll\n");
    fprintf(fInf, "batmeter.dll\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "[PowerCfg.addreg]\n");
    fprintf(fInf, "HKCU,\"AppEvents\\EventLabels\\LowBatteryAlarm\",,2,\"Low Battery Alarm\"\n");
    fprintf(fInf, "HKCU,\"AppEvents\\EventLabels\\CriticalBatteryAlarm\",,2,\"Critical Battery Alarm\"\n");
    fprintf(fInf, "\n");
    fprintf(fInf, "HKCU,\"AppEvents\\Schemes\\Apps\\PowerCfg\",,2,\"Power Management\"\n");
    fprintf(fInf, "HKCU,\"AppEvents\\Schemes\\Apps\\PowerCfg\\LowBatteryAlarm\\.Current\",,2,\"ding.wav\"\n");
    fprintf(fInf, "HKCU,\"AppEvents\\Schemes\\Apps\\PowerCfg\\LowBatteryAlarm\\.Default\",,2,\"ding.wav\"\n");
    fprintf(fInf, "HKCU,\"AppEvents\\Schemes\\Apps\\PowerCfg\\CriticalBatteryAlarm\\.Current\",,2,\"ding.wav\"\n");
    fprintf(fInf, "HKCU,\"AppEvents\\Schemes\\Apps\\PowerCfg\\CriticalBatteryAlarm\\.Default\",,2,\"ding.wav\"\n");

    return TRUE;
}

/*******************************************************************************
*
*  TabTo
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID TabTo(FILE *fInf, UINT uiCharSoFar, UINT uiCol)
{
    UINT i;

    for (i = 0; i < (uiCol - uiCharSoFar); i++) {
        fprintf(fInf, " ");
    }
}

/*******************************************************************************
*
*  WriteNTInf
*
*  DESCRIPTION:
*   Write out the NT setup file in INF format.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WriteNTInf(
    char **pszName,
    char **pszDesc,
    UINT uiCount
)
{
    UINT i;
    FILE *fInf;

    if ((fInf = fopen(INF_NAME, "w+")) != NULL) {
        printf("Writing INF specification file: %s", INF_NAME);
    }
    else {
        DefFatalExit(TRUE, "Error opening INF specification file: %s for output\n", INF_NAME);
    }

    // Write fixed header information.
    WriteInfHeader(fInf);
    printf(".");

    // User misc.
    fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\",CurrentPowerPolicy,0x00000002,\"0\"\n");

    // User global policies.
    fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\GlobalPowerPolicy\",Policies,0x00030003,\\\n  ");
    WriteRegBinary(fInf, &g_gupp, sizeof(GLOBAL_USER_POWER_POLICY), "  ");
    fprintf(fInf, "\n\n");
    printf(".");

    // User power schemes.
    for (i = 0; i < uiCount; i++) {
        fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\PowerPolicies\\%d\",Name,0x00000002,\"%s\"\n", i, pszName[i]);
        fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\PowerPolicies\\%d\",Description,0x00000002,\"%s\"\n", i, pszDesc[i]);
        fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\PowerPolicies\\%d\",Policies,0x00030003,\\\n  ", i);
        WriteRegBinary(fInf, g_pupp[i], sizeof(USER_POWER_POLICY),"  ");
        fprintf(fInf, "\n\n");
        printf(".");
    }

    // Machine misc.
    fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\",\"LastID\",0x00000002,\"%d\"\n", uiCount - 1);
    fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\",\"DiskSpinDownMax\",0x00000002,\"3600\"\n");
    fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\",\"DiskSpinDownMin\",0x00000002,\"3\"\n");
    fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\GlobalPowerPolicy\",\"Policies\",0x00030003,\\\n  ");
    WriteRegBinary(fInf, &g_gmpp, sizeof(GLOBAL_MACHINE_POWER_POLICY),
                   "                   ");
    fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\PowerPolicies\",,0x00000012\n  ");
    fprintf(fInf, "\n\n");
    printf(".");

    for (i = 0; i < uiCount; i++) {
        fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\PowerPolicies\\%d\",\"Policies\",0x00030003,\\\n  ", i);
        WriteRegBinary(fInf, g_pmpp[i], sizeof(MACHINE_POWER_POLICY),"  ");
        fprintf(fInf, "\n");
        printf(".");
    }
    fclose(fInf);
    printf("OK\n");
    return TRUE;
}

/*******************************************************************************
*
*  WriteMemphisInfs
*
*  DESCRIPTION:
*   Write out the Memphis setup files in INF format.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WriteMemphisInfs(
    char **pszName,
    char **pszDesc,
    UINT uiCount
)
{
    UINT i, j;
    FILE *fInf;
    static char *pszINFName[NUM_INF_TYPES] = {
        TYPICAL_INF,
        COMPACT_INF,
        CUSTOM_INF,
        PORTABLE_INF,
        SERVER_INF
    };

    static UINT uiINFType[NUM_INF_TYPES] = {
        TYPICAL,
        COMPACT,
        CUSTOM,
        PORTABLE,
        SERVER
    };

    // Iterate throught the Memphis INF files.
    for (i = 0; i < NUM_INF_TYPES; i++) {
        if ((fInf = fopen(pszINFName[i], "w+")) != NULL) {
            printf("Writing INF specification file: %s", pszINFName[i]);
        }
        else {
            DefFatalExit(TRUE, "Error opening INF specification file: %s for output\n", pszINFName[i]);
        }

        // Write fixed header information.
        WriteInfHeader(fInf);
        printf(".");

        // Default power policy.
        for (j = 0; j < uiCount; j++) {
            if ((g_uiPolicyInfTypes[j] & uiINFType[i]) &&
                (g_uiPolicyOSTypes[j] & WIN_95)) {
                fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\",CurrentPowerPolicy,0x00000002,\"%d\"\n", j);
                break;
            }
        }

        // User global policies.
        fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\GlobalPowerPolicy\",Policies,0x00030003,\\\n  ");
        WriteRegBinary(fInf, &g_gupp, sizeof(GLOBAL_USER_POWER_POLICY), "  ");
        fprintf(fInf, "\n\n");
        printf(".");
        // User power schemes.
        for (j = 0; j < uiCount; j++) {
            if ((g_uiPolicyInfTypes[j] & uiINFType[i]) &&
                (g_uiPolicyOSTypes[j] & WIN_95)) {
                fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\PowerPolicies\\%d\",Name,0x00000002,\"%s\"\n", j, pszName[j]);
                fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\PowerPolicies\\%d\",Description,0x00000002,\"%s\"\n", j, pszDesc[j]);
                fprintf(fInf, "HKCU,\"Control Panel\\PowerCfg\\PowerPolicies\\%d\",Policies,0x00030003,\\\n  ", j);
                WriteRegBinary(fInf, g_pupp[j], sizeof(USER_POWER_POLICY),"  ");
                fprintf(fInf, "\n\n");
                printf(".");
            }
        }

        // Machine misc.
        fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\",\"LastID\",0x00000002,\"%d\"\n", uiCount - 1);
        fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\",\"DiskSpinDownMax\",0x00000002,\"3600\"\n");
        fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\",\"DiskSpinDownMin\",0x00000002,\"3\"\n");
        fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\GlobalPowerPolicy\",\"Policies\",0x00030003,\\\n  ");
        WriteRegBinary(fInf, &g_gmpp, sizeof(GLOBAL_MACHINE_POWER_POLICY),
                   "                   ");
        fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\PowerPolicies\",,0x00000012\n  ");
        fprintf(fInf, "\n\n");
        printf(".");

        // Machine power schemes.
        for (j = 0; j < uiCount; j++) {
            if ((g_uiPolicyInfTypes[j] & uiINFType[i]) &&
                (g_uiPolicyOSTypes[j] & WIN_95)) {
                fprintf(fInf, "HKLM,\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\PowerPolicies\\%d\",\"Policies\",0x00030003,\\\n  ", j);
                WriteRegBinary(fInf, g_pmpp[j], sizeof(MACHINE_POWER_POLICY),"  ");
                fprintf(fInf, "\n");
                printf(".");
            }
        }

        fclose(fInf);
        printf("OK\n");
    }
    return TRUE;
}

/*******************************************************************************
*
*  main
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void __cdecl main (int argc, char **argv)
{
    DWORD   dwSize;
    char    *psz;
    FILE    *fInf;
    UINT    uiNameCount, uiDescCount, i;
    char    *p;
    char    *pszName[MAX_PROFILES];
    char    *pszDesc[MAX_PROFILES];

    printf("Processing: %s\n", SRC_NAME);

    printf("Building name and description arrays...\n");
    ReadSource();
    BuildLineArray();


    printf("  Parsing names.");
    GetCheckLabelToken(NAME_LINE, "Name");
    uiNameCount = GetTokens(NULL, REGSTR_MAX_VALUE_LENGTH, pszName,
                            MAX_PROFILES, DELIMITERS);
    if (uiNameCount) {
        printf("  Parsed %d names successfully.\n", uiNameCount);
        printf("  Parsing descriptions.");
        GetCheckLabelToken(DESCRIPTION_LINE, "Description");
        uiDescCount = GetTokens(NULL, MAX_DESC_LEN, pszDesc,
                                MAX_PROFILES, DELIMITERS);
        if (uiDescCount == uiNameCount) {
            printf("  Parsed %d descriptions successfully.\n", uiDescCount);
            g_uiPoliciesCount = uiNameCount;

            // Get the power policies, schemes
            GetPolicies();

            // Get the global power policies
            GetGlobalPolicies();

            // Write the INF specification files.
            WriteNTInf(pszName, pszDesc, g_uiPoliciesCount);
            WriteMemphisInfs(pszName, pszDesc, g_uiPoliciesCount);

            printf("\n\nDefault Processing Success. Output files are valid.\n");
            exit(0);
        }
        else {
            printf("  Name count: %d != description count: %d.\n", uiNameCount, uiDescCount);
        }
    }
    else {
        printf("  Name parsing failure.\n");
    }

    printf("ProcessAndWrite failed, Last Error: %d\n", GetLastError());
    exit(1);
}


