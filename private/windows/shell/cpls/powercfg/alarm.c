/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       ALARM.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Alarm dialog support.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#include <help.h>
#include <powercfp.h>
#include <mstask.h>

#include "powercfg.h"
#include "pwrresid.h"
#include "PwrMn_cs.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

// Private functions implemented in ALARM.C
void     EditWorkItem(HWND, LPTSTR);
BOOLEAN  SetSliderStatusText(HWND, UINT, UINT);
BOOLEAN  SetAlarmStatusText(HWND);
#ifdef WINNT
void     HideShowRunProgram(HWND hWnd);
#endif

// Alarm dialog property sheet init data structure:
typedef struct _ALARM_POL_DLG_DATA
{
    LPTSTR  lpszTitleExt;
    WPARAM  wParam;
} ALARM_POL_DLG_DATA, *PALARM_POL_DLG_DATA;

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

extern HINSTANCE g_hInstance;           // Global instance handle of this DLL.

// This structure is filled in by the Power Policy Manager at CPL_INIT time.
extern SYSTEM_POWER_CAPABILITIES g_SysPwrCapabilities;
extern DWORD g_dwNumSleepStates;
extern DWORD g_dwSleepStatesMaxMin;
extern DWORD g_dwBattryLevelMaxMin;

SYSTEM_POWER_STATE g_spsMaxSleepState = PowerSystemHibernate;

extern UINT g_uiDisableWakesFlag;           // Flag mask value.
extern UINT g_uiOverrideAppsFlag;           // Flag mask value.

// A systary change requires PowerSchemeDlgProc re-init.
extern BOOL g_bSystrayChange;

// Machine is currently capable of hibernate, managed by code in hibernat.c.
extern UINT g_uiPwrActIDs[];

// Persistant storage of this data is managed by POWRPROF.DLL API's.
extern GLOBAL_POWER_POLICY  g_gpp;

// Indices into g_uiPwrActIDs
#define ID_STANDBY  0
#define ID_SHUTDOWN 1

// Local visable/enabled control state variables.
UINT g_uiSoundState;
UINT g_uiTextState;
UINT g_uiProgState;
UINT g_uiLoChangeEnable;
UINT g_uiLoChangeState;
UINT g_uiAlwaysHide = CONTROL_HIDE;

UINT g_uiNotifySoundFlag   = POWER_LEVEL_USER_NOTIFY_SOUND;
UINT g_uiNotifyTextFlag    = POWER_LEVEL_USER_NOTIFY_TEXT;

#ifdef WINNT
UINT g_uiNotifyProgFlag    = POWER_LEVEL_USER_NOTIFY_EXEC;

CONST LPTSTR g_szAlarmTaskName [NUM_DISCHARGE_POLICIES] = {
    TEXT("Critical Battery Alarm Program"),
    TEXT("Low Battery Alarm Program"),
    NULL,
    NULL
};
#endif

// Advanced alarm policies dialog controls descriptions:
#ifdef WINNT
#define NUM_ALARM_ACTIONS_CONTROLS 7
#else
#define NUM_ALARM_ACTIONS_CONTROLS 5
#endif

// Handy indicies into our AlarmActions control arrays
#define ID_NOTIFYWITHSOUND      0
#define ID_NOTIFYWITHTEXT       1
#define ID_ENABLELOWSTATE       2
#define ID_ALARMACTIONPOLICY    3
#define ID_ALARMIGNORENONRESP   4
#ifdef WINNT
#define ID_RUNPROGCHECKBOX      5
#define ID_RUNPROGWORKITEM      6
#endif

POWER_CONTROLS g_pcAlarmActions[NUM_ALARM_ACTIONS_CONTROLS] =
{// Control ID              Control Type        Data Address        Data Size                       Parameter Pointer               EnableVisible State Pointer
    IDC_NOTIFYWITHSOUND,    CHECK_BOX_ENABLE,   NULL,               sizeof(DWORD),                  &g_uiNotifySoundFlag,           &g_uiSoundState,
    IDC_NOTIFYWITHTEXT,     CHECK_BOX,          NULL,               sizeof(DWORD),                  &g_uiNotifyTextFlag,            &g_uiTextState,
    IDC_ENABLELOWSTATE,     CHECK_BOX_ENABLE,   &g_uiLoChangeEnable,sizeof(DWORD),                  NULL,                           &g_uiLoChangeState,
    IDC_ALARMACTIONPOLICY,  COMBO_BOX,          NULL,               sizeof(DWORD),                  NULL,                           &g_uiLoChangeState,
    IDC_ALARMIGNORENONRESP, CHECK_BOX,          NULL,               sizeof(DWORD),                  &g_uiOverrideAppsFlag,          &g_uiLoChangeState,
#ifdef WINNT
    IDC_RUNPROGCHECKBOX,    CHECK_BOX_ENABLE,   NULL,               sizeof(DWORD),                  &g_uiNotifyProgFlag,            &g_uiProgState,
    IDC_RUNPROGWORKITEM,    PUSHBUTTON,         NULL,               0,                              NULL,                           &g_uiProgState,
#endif
};

// Alarm policies dialog controls descriptions:
#define NUM_ALARM_CONTROLS 6

// Local visable/enabled control state variables.
UINT g_uiLoState;
UINT g_uiCritState;
UINT g_uiBatteryLevelScale;

POWER_CONTROLS g_pcAlarm[NUM_ALARM_CONTROLS] =
{// Control ID                Control Type        Data Address                                                            Data Size                       Parameter Pointer       Enable/Visible State Pointer
    IDC_LOBATALARMENABLE,     CHECK_BOX_ENABLE,   &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_LOW].Enable),             sizeof(ULONG),                  NULL,                   &g_uiLoState,
    IDC_LOWACTION,            PUSHBUTTON,         NULL,                                                                   0,                              NULL,                   &g_uiLoState,
    IDC_LOALARMSLIDER,        SLIDER,             &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel),       sizeof(ULONG),                  &g_dwBattryLevelMaxMin, &g_uiLoState,
    IDC_CRITBATALARMENABLE,   CHECK_BOX_ENABLE,   &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].Enable),        sizeof(ULONG),                  NULL,                   &g_uiCritState,
    IDC_CRITACTION,           PUSHBUTTON,         NULL,                                                                   0,                              NULL,                   &g_uiCritState,
    IDC_CRITALARMSLIDER,      SLIDER,             &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel),  sizeof(ULONG),                  &g_dwBattryLevelMaxMin, &g_uiCritState,
};

// "Alarms" Dialog Box (IDD_ALARMPOLICY == 103) help array:

const DWORD g_AlarmHelpIDs[]=
{
    IDC_POWERCFGGROUPBOX3,    IDH_103_1110,   // Alarms: "Low battery alarm groupbox" (Button)
    IDC_LOBATALARMENABLE,     IDH_103_1106,   // Alarms: "Set off &low battery alarm when power level reaches:" (Button)
    IDC_LOWALARMLEVEL,        IDH_103_1104,   // Alarms: "Low alarm level" (Static)
    IDC_LOALARMSLIDER,        IDH_103_1102,   // Alarms: "Low alarm slider" (msctls_trackbar32)
    IDC_LOWACTION,            IDH_103_1101,   // Alarms: "Alar&m Action..." (Button)
    IDC_LOALARMNOTIFICATION,  IDH_103_1108,   // Alarms: "Low alarm status text" (Static)
    IDC_LOALARMPOWERMODE,     IDH_103_1108,   // Alarms: "Low alarm status text" (Static)
#ifdef WINNT
    IDC_LOALARMPROGRAM,       IDH_103_1108,   // Alarms: "Low alarm status text" (Static)
#endif
    IDC_POWERCFGGROUPBOX4,    IDH_103_1111,   // Alarms: "Critical battery alarm groupbox" (Button)
    IDC_CRITBATALARMENABLE,   IDH_103_1107,   // Alarms: "Set off &critical battery alarm when power level reaches:" (Button)
    IDC_CRITALARMLEVEL,       IDH_103_1105,   // Alarms: "Critical alarm level" (Static)
    IDC_CRITALARMSLIDER,      IDH_103_1103,   // Alarms: "Critical alarm slider" (msctls_trackbar32)
    IDC_CRITACTION,           IDH_103_1100,   // Alarms: "Ala&rm Action..." (Button)
    IDC_CRITALARMNOTIFICATION,IDH_103_1109,   // Alarms: "Critical alarm status text" (Static)
    IDC_CRITALARMPOWERMODE,   IDH_103_1109,   // Alarms: "Critical alarm status text" (Static)
#ifdef WINNT
    IDC_CRITALARMPROGRAM,     IDH_103_1109,   // Alarms: "Critical alarm status text" (Static)
#endif
    IDC_NO_HELP_1,            NO_HELP,
    IDC_NO_HELP_2,            NO_HELP,
    IDC_NO_HELP_3,            NO_HELP,
    IDC_NO_HELP_4,            NO_HELP,
    0, 0
};

// "Alarm Actions" Dialog Box (IDD_ALARMACTIONS == 106) help array:

const DWORD g_AlarmActHelpIDs[]=
{
    IDC_POWERCFGGROUPBOX5,  IDH_106_1608,   // Alarm Actions: "Notification groupbox" (Button)
    IDC_NOTIFYWITHSOUND,    IDH_106_1603,   // Alarm Actions: "&Sound alarm" (Button)
    IDC_NOTIFYWITHTEXT,     IDH_106_1605,   // Alarm Actions: "&Display message" (Button)
    IDC_POWERCFGGROUPBOX6,  IDH_106_1609,   // Alarm Actions: "Power level groupbox" (Button)
    IDC_POWERCFGGROUPBOX7,  IDH_106_1609,   // Alarm Actions: "Run program groupbox"
    IDC_ENABLELOWSTATE,     IDH_106_1600,   // Alarm Actions: "When the &alarm goes off, the computer will:" (Button)
    IDC_ALARMACTIONPOLICY,  IDH_106_1601,   // Alarm Actions: "Alarm action dropdown" (ComboBox)
    IDC_ALARMIGNORENONRESP, IDH_106_1602,   // Alarm Actions: "&Force standby or shutdown even if a program stops responding." (Button)
#ifdef WINNT
    IDC_RUNPROGCHECKBOX,    IDH_106_1620,   // Alarm Actions: "Specifies that you want a program to run..."
    IDC_RUNPROGWORKITEM,    IDH_106_1621,   // Alarm Actions: "Displays a dialog box wher the work item is configured..."
#endif
    0, 0
};

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*   AlarmActionsDlgProc
*
*   DESCRIPTION:
*
*   PARAMETERS:
*
*******************************************************************************/

INT_PTR CALLBACK AlarmActionsDlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static  GLOBAL_POWER_POLICY   gpp;
    static  PALARM_POL_DLG_DATA   papdd;

    static  UINT    uiIndex;
    static  UINT    uiEventId;
#ifdef WINNT
    static  LPTSTR  lpszTaskName;
    HWND    hTaskWnd;
#endif
    LPTSTR  lpszCaption;
    UINT    ii;

    switch (uMsg) {

        case WM_INITDIALOG:

            // Save a copy of the global policies to restore on cancel.
            memcpy(&gpp, &g_gpp, sizeof(gpp));

            // Set the pointers to the data of interest.
            papdd  = (PALARM_POL_DLG_DATA) lParam;
            if (papdd->wParam == IDC_LOWACTION) {
                uiIndex = DISCHARGE_POLICY_LOW;
                uiEventId = IDS_LOWSOUNDEVENT;
            }
            else {
                uiIndex = DISCHARGE_POLICY_CRITICAL;
                uiEventId = IDS_CRITSOUNDEVENT;
            }
#ifdef WINNT
            lpszTaskName = g_szAlarmTaskName [uiIndex];
#endif
            // Set up the data pointers in g_pcAlarmActions.
            g_pcAlarmActions[ID_NOTIFYWITHSOUND].lpvData =
                &(g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.EventCode);
            g_pcAlarmActions[ID_NOTIFYWITHTEXT].lpvData =
                &(g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.EventCode);
#ifdef WINNT
            g_pcAlarmActions[ID_RUNPROGCHECKBOX].lpvData =
                &(g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.EventCode);
#endif
            g_pcAlarmActions[ID_ALARMACTIONPOLICY].lpdwParam =
                (LPDWORD)&(g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.Action);
            g_pcAlarmActions[ID_ALARMIGNORENONRESP].lpvData =
                &(g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.Flags);

            //
            // Set the appropriate choices for the Alarms
            //
            ii=0;

            if (g_SysPwrCapabilities.SystemS1 ||
                    g_SysPwrCapabilities.SystemS2 || g_SysPwrCapabilities.SystemS3) {
                g_uiPwrActIDs[ii++] = IDS_STANDBY;
                g_uiPwrActIDs[ii++] = PowerActionSleep;
            }

            if (g_SysPwrCapabilities.HiberFilePresent) {
                g_uiPwrActIDs[ii++] = IDS_HIBERNATE;
                g_uiPwrActIDs[ii++] = PowerActionHibernate;
            }

            g_uiPwrActIDs[ii++] = IDS_POWEROFF;
            g_uiPwrActIDs[ii++] = PowerActionShutdownOff;
            g_uiPwrActIDs[ii++] = 0;
            g_uiPwrActIDs[ii++] = 0;

            g_pcAlarmActions[ID_ALARMACTIONPOLICY].lpvData = g_uiPwrActIDs;

            if (g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.Action == PowerActionNone) {
                g_uiLoChangeEnable = FALSE;
            }
            else {
                g_uiLoChangeEnable = TRUE;
            }
            MapPwrAct(&(g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.Action), FALSE);

            // Set the dialog caption.
            lpszCaption = LoadDynamicString(IDS_ALARMACTIONS,
                                            papdd->lpszTitleExt);
            if (lpszCaption) {
                SetWindowText(hWnd, lpszCaption);
                LocalFree(lpszCaption);
            }

            // Initialize the controls.
            SetControls(hWnd, NUM_ALARM_ACTIONS_CONTROLS, g_pcAlarmActions);

#ifdef WINNT
            HideShowRunProgram(hWnd);
#endif
            return (INT_PTR) TRUE;

        case WM_COMMAND:
            switch (wParam) {
#ifdef WINNT
                case IDC_RUNPROGWORKITEM:
                    hTaskWnd =  FindWindow( NULL, lpszTaskName);
                    if (hTaskWnd) {
                        BringWindowToTop(hTaskWnd);
                    } else {
                        EditWorkItem(hWnd, lpszTaskName);
                    }
                    break;

                case IDC_RUNPROGCHECKBOX:
                    hTaskWnd =  FindWindow( NULL, lpszTaskName);
                    if (hTaskWnd)
                    {
                        DestroyWindow(hTaskWnd);
                    }
                    // No break: Fall through to update grayed status of controls.
#endif
                case IDC_ENABLELOWSTATE:
                    GetControls(hWnd, NUM_ALARM_ACTIONS_CONTROLS, g_pcAlarmActions);
                    SetControls(hWnd, NUM_ALARM_ACTIONS_CONTROLS, g_pcAlarmActions);
#ifdef WINNT
                    HideShowRunProgram(hWnd);
#endif
                    break;

                case IDOK:
#ifdef WINNT
                    hTaskWnd =  FindWindow( NULL, lpszTaskName);
                    if (hTaskWnd) {
                        BringWindowToTop(hTaskWnd);
                    } else {
#endif
                        GetControls(hWnd, NUM_ALARM_ACTIONS_CONTROLS, g_pcAlarmActions);
                        if (!g_uiLoChangeEnable) {
                            g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.Action =
                                PowerActionNone;
                        }
                        g_gpp.user.DischargePolicy[uiIndex].MinSystemState = g_spsMaxSleepState;
                        EndDialog(hWnd, wParam);
#ifdef WINNT
                    }
#endif
                    break;

                case IDCANCEL:
#ifdef WINNT
                    hTaskWnd =  FindWindow( NULL, lpszTaskName);
                    if (hTaskWnd)
                    {
                        DestroyWindow(hTaskWnd);
                    }
#endif
                    // Restore the original global policies.
                    memcpy(&g_gpp, &gpp, sizeof(gpp));
                    EndDialog(hWnd, wParam);
                    break;
            }
            break;

        case PCWM_NOTIFYPOWER:
           // Notification from systray, user has changed a PM UI setting.
           g_bSystrayChange = TRUE;
           break;

        case WM_HELP:             // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_AlarmActHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_AlarmActHelpIDs);
            return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
*
*   AlarmDlgProc
*
*   DESCRIPTION:
*
*   PARAMETERS:
*
*******************************************************************************/

INT_PTR CALLBACK AlarmDlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NMHDR FAR *lpnm;
    ALARM_POL_DLG_DATA apdd;
    PUINT puiPos, puiPosVar, puiOtherPosVar, puiOtherPos, puiEnableState;
    UINT  uiEnable, uiSliderStatusId, uiID;
    static HWND hWndLoSlider, hWndCritSlider;
    static UINT uiLoPos, uiCritPos, uiLoPosSave, uiCritPosSave;
    static BOOL bDirty = FALSE;

    switch (uMsg) {

        case WM_INITDIALOG:
            // If we can't read the global power policies hide
            // the controls on this page.
            if (!GetGlobalPwrPolicy(&g_gpp)) {
                HideControls(hWnd, NUM_ALARM_CONTROLS, g_pcAlarm);
                return TRUE;
            }

            g_uiTextState = g_uiSoundState = CONTROL_ENABLE;

            // Set the scale value.
            if (!HIWORD(g_dwBattryLevelMaxMin)) {
                g_uiBatteryLevelScale = 1;
            }
            else {
                g_uiBatteryLevelScale = 100 / HIWORD(g_dwBattryLevelMaxMin);
            }

            g_gpp.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel /=
                g_uiBatteryLevelScale;
            g_gpp.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel /=
                g_uiBatteryLevelScale;

            // Cache the low alarm slider window handle.
            hWndLoSlider   = GetDlgItem(hWnd, IDC_LOALARMSLIDER);
            hWndCritSlider = GetDlgItem(hWnd, IDC_CRITALARMSLIDER);

            // Initialize the local enable and position variables.
            uiLoPosSave   = uiLoPos   =
                g_gpp.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel;
            uiCritPosSave = uiCritPos =
                g_gpp.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel;

            // Initialize the dialog controls
            SendDlgItemMessage(hWnd, IDC_LOALARMSLIDER, TBM_SETTICFREQ, 25, 0);
            SendDlgItemMessage(hWnd, IDC_CRITALARMSLIDER, TBM_SETTICFREQ, 25, 0);
            SetControls(hWnd, NUM_ALARM_CONTROLS, g_pcAlarm);
            SetSliderStatusText(hWnd, IDC_LOWALARMLEVEL,  uiLoPos);
            SetSliderStatusText(hWnd, IDC_CRITALARMLEVEL, uiCritPos);
            SetAlarmStatusText(hWnd);

            // If we can't write the global policies disable the controls.
            if (!WriteGlobalPwrPolicyReport(hWnd, &g_gpp)) {
                DisableControls(hWnd, NUM_ALARM_CONTROLS, g_pcAlarm);
            }
            return TRUE;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code) {
                case PSN_APPLY:
                    if (bDirty) {
                        GetControls(hWnd, NUM_ALARM_CONTROLS, g_pcAlarm);
                        g_gpp.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel *=
                             g_uiBatteryLevelScale;
                        g_gpp.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel *=
                             g_uiBatteryLevelScale;
                        WriteGlobalPwrPolicyReport(hWnd, &g_gpp);
                        GetActivePwrScheme(&uiID);
                        SetActivePwrSchemeReport(hWnd, uiID, &g_gpp, NULL);
                        bDirty = FALSE;
                    }
                    break;
            }
            break;

        case WM_COMMAND:
            switch (wParam) {
                case IDC_CRITACTION:
                    apdd.lpszTitleExt = LoadDynamicString(IDS_CRITBAT);
                    goto do_config_alarm_act;

                case IDC_LOWACTION:
                    apdd.lpszTitleExt = LoadDynamicString(IDS_LOWBAT);

do_config_alarm_act:
                    apdd.wParam = wParam;
                    if (IDOK == DialogBoxParam(g_hInstance,
                                               MAKEINTRESOURCE(IDD_ALARMACTIONS),
                                               hWnd,
                                               AlarmActionsDlgProc,
                                               (LPARAM)&apdd)) {
                        // Enable the parent dialog Apply button on change.
                        MarkSheetDirty(hWnd, &bDirty);
                    }

                    if (apdd.lpszTitleExt) {
                        LocalFree(apdd.lpszTitleExt);
                    }
                    SetAlarmStatusText(hWnd);
                    break;

                case IDC_LOBATALARMENABLE:
                    puiPosVar = &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel);
                    puiOtherPosVar = &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel);
                    uiSliderStatusId = IDC_LOWALARMLEVEL;
                    goto do_sheet_dirty;

                case IDC_CRITBATALARMENABLE:
                    puiPosVar = &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel);
                    puiOtherPosVar = &(g_gpp.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel);
                    uiSliderStatusId = IDC_CRITALARMLEVEL;

do_sheet_dirty:
                    GetControls(hWnd, NUM_ALARM_CONTROLS, g_pcAlarm);
                    if ((uiEnable = IsDlgButtonChecked(hWnd, (int) wParam)) ==
                        BST_CHECKED) {
                        if (uiLoPos < uiCritPos) {
                            uiLoPos = uiCritPos = *puiPosVar = *puiOtherPosVar;
                            SetSliderStatusText(hWnd, uiSliderStatusId, uiCritPos);
                        }
                    }
                    SetControls(hWnd, NUM_ALARM_CONTROLS, g_pcAlarm);
                    SetAlarmStatusText(hWnd);
                    MarkSheetDirty(hWnd, &bDirty);
                    break;
            }
            break;

        case WM_HSCROLL:
            // Only handle slider controls.
            if (((HWND)lParam != hWndLoSlider) &&
                ((HWND)lParam != hWndCritSlider)) {
                break;
            }

            // Don't allow the low slider to be set lower than the critical
            // slider. Reset position on TB_ENDTRACK for this case.
            if (hWndLoSlider == (HWND)lParam) {
                puiPos           = &uiLoPos;
                puiOtherPos      = &uiCritPos;
                puiEnableState   = &g_uiCritState;
                uiSliderStatusId = IDC_LOWALARMLEVEL;
            }
            else {
                puiPos           = &uiCritPos;
                puiOtherPos      = &uiLoPos;
                puiEnableState   = &g_uiLoState;
                uiSliderStatusId = IDC_CRITALARMLEVEL;
            }

            switch (LOWORD(wParam)) {
               case TB_ENDTRACK:
                    if (*puiEnableState & CONTROL_ENABLE) {
                        if (uiLoPos < uiCritPos) {
                            SendMessage((HWND)lParam, TBM_SETPOS, TRUE,
                                        (LPARAM)*puiOtherPos);
                            *puiPos = *puiOtherPos;
                        }
                    }
                    break;

                case TB_THUMBPOSITION:
                case TB_THUMBTRACK:
                    // New position comes with these messages.
                    *puiPos = HIWORD(wParam);
                    break;

                default:
                    // New position must be fetched for the rest.
                    *puiPos = (UINT) SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
            }

            // Update the current slider position text.
            SetSliderStatusText(hWnd, uiSliderStatusId, *puiPos);

            // Enable the parent dialog Apply button on any  change.
            MarkSheetDirty(hWnd, &bDirty);
            break;

        case WM_HELP:             // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_AlarmHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_AlarmHelpIDs);
            return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
*
*                 P R I V A T E   F U N C T I O N S
*
*******************************************************************************/

/*******************************************************************************
*
*  PathOnly
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL PathOnly(LPTSTR sz)
{
   LPTSTR p = sz;
   LPTSTR s = NULL;

   while ( *p ) {
      if ( *p == TEXT('\\') ) {
         s = p;
      } else if ( *p == TEXT(':') ) {
         s = p + 1;
      }
#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
      p = AnsiNext(p);
#else
      p++;
#endif
   }

   if ( s ) {
      if ( s == sz )
         s++;

      *s = TEXT('\0');
      return TRUE;
   }

   return FALSE;
}

#ifdef WINNT
/*******************************************************************************
*
*  FileNameOnly
*
*  DESCRIPTION: Returns a pointer to the first character after the last
*               backslash in a string
*
*  PARAMETERS:
*
*******************************************************************************/

LPTSTR FileNameOnly(LPTSTR sz)
{
    LPTSTR next = sz;
    LPTSTR prev;
    LPTSTR begin = next;

    if (next == NULL) {
        return NULL;
    }

    while ( *next ) {
        prev = next;

#if defined(DBCS) || (defined(FE_SB) && !defined(UNICODE))
      next = AnsiNext(next);
#else
      next++;
#endif
        if ( (*prev == TEXT('\\')) || (*prev == TEXT(':')) ) {
            begin = next;
        }
    }

    return begin;
}

/*******************************************************************************
*
*  EditWorkItem
*
*  DESCRIPTION: Opens the specified task.
*
*  PARAMETERS:
*
*******************************************************************************/
void EditWorkItem(HWND hWnd, LPTSTR pszTaskName)
{
    ITaskScheduler  *pISchedAgent = NULL;
    ITask           *pITask;
    IPersistFile    *pIPersistFile;
    HRESULT     hr;

    hr = CoInitialize(NULL);

    if (FAILED(hr)) {
        DebugPrint( "EditWorkItem: CoInitialize returned hr = %08x\n", hr);
        return;
    }

    hr = CoCreateInstance( &CLSID_CSchedulingAgent,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           &IID_ISchedulingAgent,
                           (LPVOID*)&pISchedAgent);

    if (SUCCEEDED(hr)) {

        hr = pISchedAgent->lpVtbl->Activate(pISchedAgent,
                                       pszTaskName,
                                       &IID_ITask,
                                       &(IUnknown *)pITask);

        if (SUCCEEDED(hr)) {
            pITask->lpVtbl->EditWorkItem(pITask, hWnd, 0);
            pITask->lpVtbl->Release(pITask);
        }
        else if (HRESULT_CODE (hr) == ERROR_FILE_NOT_FOUND){
            hr = pISchedAgent->lpVtbl->NewWorkItem(
                    pISchedAgent,
                    pszTaskName,
                    &CLSID_CTask,
                    &IID_ITask,
                    &(IUnknown *)pITask);

            if (SUCCEEDED(hr)) {
                hr = pITask->lpVtbl->QueryInterface(pITask, &IID_IPersistFile,
                                (void **)&pIPersistFile);

                if (SUCCEEDED(hr)) {
                    hr = pIPersistFile->lpVtbl->Save(pIPersistFile, NULL, TRUE);

                    if (SUCCEEDED(hr)) {
                        pITask->lpVtbl->EditWorkItem(pITask, hWnd, 0);
                    }
                    else {
                        DebugPrint( "EditWorkItem: Save filed hr = %08x\n", hr);
                    }
                    pIPersistFile->lpVtbl->Release(pIPersistFile);
                }
                else {
                    DebugPrint( "EditWorkItem: QueryInterface for IPersistFile hr = %08x\n", hr);
                }
                pITask->lpVtbl->Release(pITask);

            }
            else {
                DebugPrint( "EditWorkItem: Activate returned hr = %08x\n", hr);
            }
        }
        else {
            DebugPrint( "EditWorkItem: NewWorkItem returned hr = %08x\n", hr);
        }

        pISchedAgent->lpVtbl->Release(pISchedAgent);
    }
    else {
        DebugPrint( "EditWorkItem: CoCreateInstance returned hr = %08x\n", hr);
    }

    CoUninitialize();

}
#endif
/*******************************************************************************
*
*  SetSliderStatusText
*
*  DESCRIPTION:
*   Update the current slider position text.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN SetSliderStatusText(HWND hWnd, UINT uiStatusId, UINT uiLevel)
{
    LPTSTR  pString;

    pString = LoadDynamicString(IDS_ALARMLEVELFORMAT,
                                uiLevel * g_uiBatteryLevelScale);
    DisplayFreeStr(hWnd, uiStatusId, pString, FREE_STR);
    return TRUE;
}

/*******************************************************************************
*
*  SetAlarmStatusText
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN SetAlarmStatusText(HWND hWnd)
{
   TCHAR   szStatus[MAX_UI_STR_LEN];
   LPTSTR  lpsz;
   UINT    uiActionId, uiStatusId, uiIndex, uiAction;
   PUINT   puiState;
#ifdef WINNT

   LPTSTR  lpszRunProg;

#endif

   puiState    = &g_uiCritState;
   uiStatusId  = IDC_CRITALARMNOTIFICATION;
   for (uiIndex = DISCHARGE_POLICY_CRITICAL; uiIndex <= DISCHARGE_POLICY_LOW; uiIndex++) {

      // Format the alarm action notification status string.
      szStatus[0] = '\0';
      if (g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.EventCode &
          POWER_LEVEL_USER_NOTIFY_SOUND) {
         if ((lpsz = LoadDynamicString(IDS_ALARMSTATUSSOUND)) != NULL) {
            lstrcat(szStatus, lpsz);
            LocalFree(lpsz);
         }
      }

      if (g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.EventCode &
          POWER_LEVEL_USER_NOTIFY_TEXT) {
         if (szStatus[0] != '\0') {
            lstrcat(szStatus, TEXT(", "));
         }
         if ((lpsz = LoadDynamicString(IDS_ALARMSTATUSTEXT)) != NULL) {
            lstrcat(szStatus, lpsz);
            LocalFree(lpsz);
         }
      }

      if (szStatus[0] == '\0') {
         if ((lpsz = LoadDynamicString(IDS_NOACTION)) != NULL) {
            lstrcat(szStatus, lpsz);
            LocalFree(lpsz);
         }
      }
      DisplayFreeStr(hWnd, uiStatusId, szStatus, NO_FREE_STR);
      ShowWindow(GetDlgItem(hWnd, uiStatusId),
                 (*puiState & CONTROL_ENABLE) ?  SW_SHOW:SW_HIDE);
      uiStatusId++;

      // Format the alarm action power mode status string.
      uiAction = g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.Action;
      switch (uiAction) {
         case PowerActionNone:
            uiActionId = IDS_NOACTION;
            break;

         case PowerActionSleep:
            uiActionId = IDS_STANDBY;
            break;

         case PowerActionHibernate:
            uiActionId = IDS_HIBERNATE;
            break;

         case PowerActionShutdown:
         case PowerActionShutdownReset:
         case PowerActionShutdownOff:
            uiActionId = IDS_POWEROFF;
            break;

         case PowerActionReserved:
         default:
            DebugPrint( "SetAlarmStatusText, unable to map power action: %X", uiAction);
            uiActionId = IDS_NOACTION;
      }
      lpsz = LoadDynamicString(uiActionId);
      DisplayFreeStr(hWnd, uiStatusId, lpsz, FREE_STR);
      ShowWindow(GetDlgItem(hWnd, uiStatusId),
                 (*puiState & CONTROL_ENABLE) ?  SW_SHOW:SW_HIDE);
      uiStatusId++;

      // Format the alarm action run program status string.
#ifdef WINNT
      lpszRunProg = NULL;

      if (g_gpp.user.DischargePolicy[uiIndex].PowerPolicy.EventCode &
         POWER_LEVEL_USER_NOTIFY_EXEC) {
         {
            //
            // Open up the alarm action task and read the program name.
            //

            ITaskScheduler   *pISchedAgent = NULL;
            ITask            *pITask;

            HRESULT     hr;

            hr = CoInitialize(NULL);

            if (SUCCEEDED(hr)) {

               hr = CoCreateInstance( &CLSID_CSchedulingAgent,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      &IID_ISchedulingAgent,
                                      (LPVOID*)&pISchedAgent);

               if (SUCCEEDED(hr)) {

                  hr = pISchedAgent->lpVtbl->Activate(pISchedAgent,
                                                      g_szAlarmTaskName [uiIndex],
                                                      &IID_ITask,
                                                      &(IUnknown *)pITask);

                  if (SUCCEEDED(hr)) {
                     pITask->lpVtbl->GetApplicationName(pITask, &lpszRunProg);
                     pITask->lpVtbl->Release(pITask);
                  }

                  pISchedAgent->lpVtbl->Release(pISchedAgent);
               }
               else {
                  DebugPrint( "SetAlarmStatusText: CoCreateInstance returned hr = %08x\n", hr);
               }

               CoUninitialize();
            }
         }

      }
      if (lpszRunProg != NULL) {

          DisplayFreeStr(hWnd, uiStatusId, FileNameOnly(lpszRunProg), NO_FREE_STR);
          CoTaskMemFree (lpszRunProg);
          lpszRunProg = NULL;
      }
      else {
         lpsz = LoadDynamicString(IDS_NONE);
         DisplayFreeStr(hWnd, uiStatusId, lpsz, FREE_STR);
      }
      ShowWindow(GetDlgItem(hWnd, uiStatusId),
                 (*puiState & CONTROL_ENABLE) ?  SW_SHOW:SW_HIDE);
#endif
      uiStatusId++;

      puiState    = &g_uiLoState;
      uiStatusId  = IDC_LOALARMNOTIFICATION;
   }
   return TRUE;
}

#ifdef WINNT
/*******************************************************************************
*
*  HideShowRunProgram
*
*  DESCRIPTION:
*
*  PARAMETERS:
*   On WINNT, only power users may set the run program.
*   The run program is stored under HKLM.
*
*******************************************************************************/

void HideShowRunProgram(HWND hWnd)
{
    if (CanUserWritePwrScheme()) {
        ShowWindow(GetDlgItem(hWnd, IDC_POWERCFGGROUPBOX7), SW_SHOW);
        ShowWindow(GetDlgItem(hWnd, IDC_RUNPROGCHECKBOX), SW_SHOW);
        ShowWindow(GetDlgItem(hWnd, IDC_RUNPROGWORKITEM), SW_SHOW);
    }
    else {
        ShowWindow(GetDlgItem(hWnd, IDC_POWERCFGGROUPBOX7), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, IDC_RUNPROGCHECKBOX), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, IDC_RUNPROGWORKITEM), SW_HIDE);
    }
}
#endif

