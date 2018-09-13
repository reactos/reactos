/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       MAKEINI.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        1 Jan, 1997
*
*  DESCRIPTION:
*   Main code for the default power schemes ini file generator, MAKEINI.EXE.
*   Generates a registry specification file which can be read by REGINI.EXE.
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
#include "parse.h"

#define SRC_NAME            "DEFAULTS.CSV"
#define INI_NAME            "POWERCFG.INI"
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

VOID WriteRegBinary(FILE *fIni, PVOID pv, UINT uiSize, char *pszIndent)
{
    PDWORD  pdw = pv;
    DWORD   dw;
    PBYTE   pb;
    UINT    uiRow = 2;
    UINT    uiNumDWords, uiNumBytes;

    fprintf(fIni, "REG_BINARY 0x%08X ", uiSize);

    uiNumDWords = uiSize / sizeof(DWORD);
    uiNumBytes  = uiSize % sizeof(DWORD);
    while (uiNumDWords) {
        fprintf(fIni, "0x%08X ", *pdw++);
        uiNumDWords--;
        if (uiRow++ == 3) {
            uiRow = 0;
            fprintf(fIni, "\\\n%s", pszIndent);
        }
    }

    if (uiNumBytes) {
        pb = (PBYTE)pdw;
        dw = (DWORD)*pb++;

        if (uiNumBytes >= 2) {
            dw |=  ((DWORD)*pb++ << 8);
        }

        if (uiNumBytes == 3) {
            dw |=  ((DWORD)*pb << 16);
        }
    }
}

/*******************************************************************************
*
*  WriteRegSpec
*
*  DESCRIPTION:
*   Write out the registry specification file in REGINI format.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WriteRegSpec(FILE *fIni, char **pszName, char **pszDesc, UINT uiCount)
{
    UINT i;

    // PowerCfg sound events
    fprintf(fIni, "USER:\\AppEvents\n");
    fprintf(fIni, "    EventLabels\n");
    fprintf(fIni, "       LowBatteryAlarm\n");
    fprintf(fIni, "           = Low Battery Alarm\n");
    fprintf(fIni, "       CriticalBatteryAlarm\n");
    fprintf(fIni, "            = Critical Battery Alarm\n\n");
    fprintf(fIni, "USER:\\AppEvents\n");
    fprintf(fIni, "    Schemes\n");
    fprintf(fIni, "        Apps\n");
    fprintf(fIni, "            PowerCfg\n");
    fprintf(fIni, "                = Power Management\n");
    fprintf(fIni, "                LowBatteryAlarm\n");
    fprintf(fIni, "                    .Current\n");
    fprintf(fIni, "                        = ding.wav\n");
    fprintf(fIni, "                    .Default\n");
    fprintf(fIni, "                        =\n");
    fprintf(fIni, "                CriticalBatteryAlarm\n");
    fprintf(fIni, "                    .Current\n");
    fprintf(fIni, "                        = ding.wav\n");
    fprintf(fIni, "                    .Default\n");
    fprintf(fIni, "                        =\n\n");

    // User misc.
    fprintf(fIni, "USER:\\Control Panel\\PowerCfg\n");
    fprintf(fIni, "    CurrentPowerPolicy = 0\n");

    // User global policies.
    fprintf(fIni, "    GlobalPowerPolicy\n");
    fprintf(fIni, "        Policies = ");
    WriteRegBinary(fIni, &g_gupp, sizeof(GLOBAL_USER_POWER_POLICY),
                   "                   ");
    fprintf(fIni, "\n\n");

    // User power schemes.
    fprintf(fIni, "    PowerPolicies\n");
    for (i = 0; i < uiCount; i++) {
        fprintf(fIni, "        %d\n", i);
        fprintf(fIni, "            Name = %s\n", pszName[i]);
        fprintf(fIni, "            Description = %s\n", pszDesc[i]);
        fprintf(fIni, "            Policies = ");
        WriteRegBinary(fIni, g_pupp[i], sizeof(USER_POWER_POLICY),
                       "                       ");
        fprintf(fIni, "\n\n");
    }

    // Machine misc.
    fprintf(fIni, "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\n");
    fprintf(fIni, "    DiskSpinDownMax = 3600\n");
    fprintf(fIni, "    DiskSpinDownMin = 3\n");

    fprintf(fIni, "    LastID = %d\n", uiCount - 1);

    fprintf(fIni, "    GlobalPowerPolicy\n");
    fprintf(fIni, "        Policies = ");
    WriteRegBinary(fIni, &g_gmpp, sizeof(GLOBAL_MACHINE_POWER_POLICY),
                   "                   ");
    fprintf(fIni, "\n\n");
    fprintf(fIni, "    PowerPolicies\n");
    for (i = 0; i < uiCount; i++) {
        fprintf(fIni, "        %d\n", i);
        fprintf(fIni, "            Policies = ");
        WriteRegBinary(fIni, g_pmpp[i], sizeof(MACHINE_POWER_POLICY),
                       "                       ");
        fprintf(fIni, "\n\n");
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
    FILE    *fIni;
    UINT    uiNameCount, uiDescCount;
    char    *pszName[MAX_PROFILES];
    char    *pszDesc[MAX_PROFILES];

    printf("Processing: %s\n", SRC_NAME);

    printf("Building name and description arrays...\n");
    ReadSource();
    BuildLineArray();

    if ((fIni = fopen(INI_NAME, "w+")) != NULL) {
        printf("Writing registry specification file: %s\n", INI_NAME);
    }
    else {
        DefFatalExit(TRUE, "Error opening registry specification file: %s for output\n", INI_NAME);
    }

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

                    // Write the registry specification file.
                    WriteRegSpec(fIni, pszName, pszDesc, g_uiPoliciesCount);

                    printf("Registry specification file: %s, written sucessfully \n", INI_NAME);
                    fclose(fIni);
                    printf("\n\nDefault Processing Success. Output file is valid.\n");
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


