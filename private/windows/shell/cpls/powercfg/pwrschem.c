/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       PWRSCHEM.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Support for power scheme page (front page) of PowerCfg.Cpl.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>
#include <regstr.h>
#include <help.h>
#include <powercfp.h>

#include "powercfg.h"
#include "pwrresid.h"
#include "PwrMn_cs.h"

// Structure to manage the scheme list information.
typedef struct _SCHEME_LIST
{
    LIST_ENTRY              leSchemeList;
    UINT                    uiID;
    LPTSTR                  lpszName;
    LPTSTR                  lpszDesc;
    PPOWER_POLICY           ppp;
} SCHEME_LIST, *PSCHEME_LIST;

// Structure to manage the power scheme dialog proc info.
typedef struct _POWER_SCHEME_DLG_INFO
{
    HWND  hwndSchemeList;
} POWER_SCHEME_DLG_INFO, *PPOWER_SCHEME_DLG_INFO;

// Private functions implemented in PWRSCHEM.C:
UINT StripBlanks(LPTSTR);
UINT RangeLimitHiberTimeOuts(UINT uiIdleTimeout, UINT *uiHiberToIDs);
VOID RefreshSchemes(HWND, PSCHEME_LIST);
VOID HandleIdleTimeOutChanged(HWND hWnd, UINT uMsg, WPARAM wParam, BOOL *pbDirty);
LONG MsgBoxId(HWND, UINT, UINT, LPTSTR, UINT);

BOOLEAN DoDeleteScheme(HWND, LPTSTR);
BOOLEAN DoSaveScheme(HWND);
BOOLEAN ClearSchemeList(VOID);
BOOLEAN RemoveScheme(PSCHEME_LIST, LPTSTR);
BOOLEAN PowerSchemeDlgInit(HWND, PPOWER_SCHEME_DLG_INFO);
BOOLEAN CALLBACK PowerSchemeEnumProc(UINT, DWORD, LPTSTR, DWORD, LPTSTR, PPOWER_POLICY, LPARAM);
BOOLEAN HandleCurSchemeChanged(HWND hWnd);
BOOLEAN MapHiberTimer(PPOWER_POLICY ppp, BOOLEAN Get);

PSCHEME_LIST GetCurSchemeFromCombo(HWND hWnd);
PSCHEME_LIST FindScheme(LPTSTR, BOOLEAN);
PSCHEME_LIST AddScheme(UINT, LPTSTR, UINT, LPTSTR, UINT, PPOWER_POLICY);
PSCHEME_LIST FindNextScheme(LPTSTR);

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

extern HINSTANCE g_hInstance;           // Global instance handle of this DLL.

// This structure is filled in by the Power Policy Manager at CPL_INIT time.
extern SYSTEM_POWER_CAPABILITIES g_SysPwrCapabilities;
extern BOOLEAN  g_bVideoLowPowerSupported;
extern DWORD    g_dwNumSleepStates;
extern UINT     g_uiSpindownMaxMin;
extern BOOL     g_bRunningUnderNT;

UINT g_uiTimeoutIDs[] =                 // Timeout string ID's.
{
    IDS_01_MIN,     60 * 1,         // 1 Min.
    IDS_02_MIN,     60 * 2,
    IDS_03_MIN,     60 * 3,
    IDS_05_MIN,     60 * 5,
    IDS_10_MIN,     60 * 10,
    IDS_15_MIN,     60 * 15,
    IDS_20_MIN,     60 * 20,
    IDS_25_MIN,     60 * 25,
    IDS_30_MIN,     60 * 30,
    IDS_45_MIN,     60 * 45,
    IDS_01_HOUR,    60 * 60 * 1,    // 1 Hour
    IDS_02_HOUR,    60 * 60 * 2,
    IDS_03_HOUR,    60 * 60 * 3,
    IDS_04_HOUR,    60 * 60 * 4,
    IDS_05_HOUR,    60 * 60 * 5,
    IDS_NEVER,      0,
    0,              0
};

UINT g_uiHiberToIDs[] =                 // Hiber timeout string ID's.
{
    IDS_01_MIN,     60 * 1,         // 1 Min.
    IDS_02_MIN,     60 * 2,
    IDS_03_MIN,     60 * 3,
    IDS_05_MIN,     60 * 5,
    IDS_10_MIN,     60 * 10,
    IDS_15_MIN,     60 * 15,
    IDS_20_MIN,     60 * 20,
    IDS_25_MIN,     60 * 25,
    IDS_30_MIN,     60 * 30,
    IDS_45_MIN,     60 * 45,
    IDS_01_HOUR,    60 * 60 * 1,    // 1 Hour
    IDS_02_HOUR,    60 * 60 * 2,
    IDS_03_HOUR,    60 * 60 * 3,
    IDS_04_HOUR,    60 * 60 * 4,
    IDS_05_HOUR,    60 * 60 * 5,
    IDS_06_HOUR,    60 * 60 * 6,
    IDS_NEVER,      0,
    0,              0
};

UINT g_uiHiberToAcIDs[sizeof(g_uiHiberToIDs)]; // Hibernate AC timeout string ID's.
UINT g_uiHiberToDcIDs[sizeof(g_uiHiberToIDs)]; // Hibernate DC timeout string ID's.

UINT g_uiSpinDownIDs[] =            // Disk spin down timeout string ID's.
{
    IDS_01_MIN,     60 * 1,         // 1 Min.
    IDS_02_MIN,     60 * 2,
    IDS_03_MIN,     60 * 3,
    IDS_05_MIN,     60 * 5,
    IDS_10_MIN,     60 * 10,
    IDS_15_MIN,     60 * 15,
    IDS_20_MIN,     60 * 20,
    IDS_25_MIN,     60 * 25,
    IDS_30_MIN,     60 * 30,
    IDS_45_MIN,     60 * 45,
    IDS_01_HOUR,    60 * 60 * 1,    // 1 Hour
    IDS_02_HOUR,    60 * 60 * 2,
    IDS_03_HOUR,    60 * 60 * 3,
    IDS_04_HOUR,    60 * 60 * 4,
    IDS_05_HOUR,    60 * 60 * 5,
    IDS_NEVER,      0,
    0,              0
};

// Show/hide UI state variables for power schemes dialog.
UINT g_uiWhenComputerIsState;
UINT g_uiStandbyState;
UINT g_uiMonitorState;
UINT g_uiDiskState;
UINT g_uiHiberState;
UINT g_uiHiberTimeoutAc;
UINT g_uiHiberTimeoutDc;
UINT g_uiIdleTimeoutAc;
UINT g_uiIdleTimeoutDc;

// Power schemes dialog controls descriptions:
UINT g_uiNumPwrSchemeCntrls;
#define NUM_POWER_SCHEME_CONTROLS       17
#define NUM_POWER_SCHEME_CONTROLS_NOBAT 8

// Handy indicies into our g_pcPowerScheme control array
#define ID_GOONSTANDBY         0
#define ID_STANDBYACCOMBO      1
#define ID_TURNOFFMONITOR      2
#define ID_MONITORACCOMBO      3
#define ID_TURNOFFHARDDISKS    4
#define ID_DISKACCOMBO         5
#define ID_SYSTEMHIBERNATES    6
#define ID_HIBERACCOMBO        7
#define ID_STANDBYDCCOMBO      8
#define ID_MONITORDCCOMBO      9
#define ID_DISKDCCOMBO         10
#define ID_HIBERDCCOMBO        11
#define ID_WHENCOMPUTERIS      12
#define ID_PLUGGEDIN           13
#define ID_RUNNINGONBAT        14
#define ID_PLUG                15
#define ID_BATTERY             16

POWER_CONTROLS g_pcPowerScheme[NUM_POWER_SCHEME_CONTROLS] =
{// Control ID              Control Type        Data Address        Data Size                       Parameter Pointer               EnableVisible State Pointer
    IDC_GOONSTANDBY,        STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiStandbyState,
    IDC_STANDBYACCOMBO,     COMBO_BOX,          &g_uiTimeoutIDs,    sizeof(DWORD),                  &g_uiIdleTimeoutAc,             &g_uiStandbyState,
    IDC_TURNOFFMONITOR,     STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiMonitorState,
    IDC_MONITORACCOMBO,     COMBO_BOX,          &g_uiTimeoutIDs,    sizeof(DWORD),                  NULL,                           &g_uiMonitorState,
    IDC_TURNOFFHARDDISKS,   STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiDiskState,
    IDC_DISKACCOMBO,        COMBO_BOX,          &g_uiSpinDownIDs,   sizeof(DWORD),                  NULL,                           &g_uiDiskState,
    IDC_SYSTEMHIBERNATES,   STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiHiberState,
    IDC_HIBERACCOMBO,       COMBO_BOX,          &g_uiHiberToAcIDs,  sizeof(DWORD),                  &g_uiHiberTimeoutAc,            &g_uiHiberState,
    IDC_STANDBYDCCOMBO,     COMBO_BOX,          &g_uiTimeoutIDs,    sizeof(DWORD),                  &g_uiIdleTimeoutDc,             &g_uiStandbyState,
    IDC_MONITORDCCOMBO,     COMBO_BOX,          &g_uiTimeoutIDs,    sizeof(DWORD),                  NULL,                           &g_uiMonitorState,
    IDC_DISKDCCOMBO,        COMBO_BOX,          &g_uiSpinDownIDs,   sizeof(DWORD),                  NULL,                           &g_uiDiskState,
    IDC_HIBERDCCOMBO,       COMBO_BOX,          &g_uiHiberToDcIDs,  sizeof(DWORD),                  &g_uiHiberTimeoutDc,            &g_uiHiberState,
    IDC_WHENCOMPUTERIS,     STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiWhenComputerIsState,
    IDC_PLUGGEDIN,          STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiWhenComputerIsState,
    IDC_RUNNINGONBAT,       STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiWhenComputerIsState,
    IDI_PLUG,               STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiWhenComputerIsState,
    IDI_BATTERY,            STATIC_TEXT,        NULL,               0,                              NULL,                           &g_uiWhenComputerIsState,
};

// Show/hide UI state variables for advanced power schemes dialog.
UINT g_uiAdvWhenComputerIsState;
UINT g_uiOptimizeState;


// Globals to manage the power schemes list:
SCHEME_LIST     g_sl;               // Head of the power schemes list.
PSCHEME_LIST    g_pslCurActive;     // Currently active power scheme.
PSCHEME_LIST    g_pslCurSel;        // Currently selected power scheme.
PSCHEME_LIST    g_pslValid;         // A valid scheme for error recovery.
UINT            g_uiSchemeCount;    // Number of power schemes.
LIST_ENTRY      g_leSchemeList;     // Head of the power schemes list.
BOOL            g_bSystrayChange;   // A systary change requires PowerSchemeDlgProc re-init.

// "Power Schemes" Dialog Box (IDD_POWERSCHEME == 100) help arrays:

const DWORD g_PowerSchemeHelpIDs[]=
{
    IDC_SCHEMECOMBO,        IDH_100_1000,   // Power Schemes: "Power schemes" (ComboBox)
    IDC_POWERSCHEMESTEXT,   IDH_COMM_GROUPBOX,
    IDC_SAVEAS,             IDH_100_1001,   // Power Schemes: "&Save As..." (Button)
    IDC_DELETE,             IDH_100_1002,   // Power Schemes: "&Delete" (Button)
    IDC_SETTINGSFOR,        IDH_COMM_GROUPBOX,   // Power Schemes: "Settings for groupbox" (Button)
    IDC_GOONSTANDBY,        IDH_100_1009,   // Power Schemes: "Go on s&tandby:" (Static)
    IDC_STANDBYACCOMBO,     IDH_100_1005,   // Power Schemes: "Standby AC time" (ComboBox)
    IDC_STANDBYDCCOMBO,     IDH_100_1006,   // Power Schemes: "Standby DC time" (ComboBox)
    IDC_SYSTEMHIBERNATES,   IDH_SYSTEMHIBERNATES,
    IDC_HIBERACCOMBO,       IDH_HIBERACCOMBO,
    IDC_HIBERDCCOMBO,       IDH_HIBERDCCOMBO,
    IDC_TURNOFFMONITOR,     IDH_100_1010,   // Power Schemes: "Turn off &monitor:" (Static)
    IDC_MONITORACCOMBO,     IDH_100_1007,   // Power Schemes: "Monitor AC time" (ComboBox)
    IDC_MONITORDCCOMBO,     IDH_100_1008,   // Power Schemes: "Monitor DC time" (ComboBox)
    IDC_TURNOFFHARDDISKS,   IDH_107_1509,   // Advanced Power Scheme Settings: "Turn off hard &disks:" (Static)
    IDC_DISKACCOMBO,        IDH_107_1505,   // Advanced Power Scheme Settings: "Disk off time AC" (ComboBox)
    IDC_DISKDCCOMBO,        IDH_107_1506,   // Advanced Power Scheme Settings: "Disk off time DC" (ComboBox)
    IDC_PLUGGEDIN,          NO_HELP,
    IDC_NO_HELP_0,          NO_HELP,
    IDC_NO_HELP_7,          NO_HELP,
    IDI_PWRMNG,             NO_HELP,
    IDI_PLUG,               NO_HELP,
    IDI_BATTERY,            NO_HELP,
    IDC_WHENCOMPUTERIS,     NO_HELP,
    IDC_PLUGGEDIN,          NO_HELP,
    IDC_RUNNINGONBAT,       NO_HELP,
    0, 0
};

// "Save Scheme" Dialog Box (IDD_SAVE == 109) help ID's:

#define IDH_109_1700    111411309   // Save Scheme: "Save name power scheme" (Edit)

// "Save Scheme" Dialog Box (IDD_SAVE == 109) help array:

const DWORD g_SaveAsHelpIDs[]=
{
    IDC_SAVENAMEEDIT,   IDH_109_1700,   // Save Scheme: "Save name power scheme" (Edit)
    IDC_SAVETEXT,       IDH_109_1700,
    0, 0
};

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  InitSchemesList
*
*  DESCRIPTION:
*   Called once at DLL initialization time.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID InitSchemesList(VOID)
{
    InitializeListHead(&g_leSchemeList);
}

/*******************************************************************************
*
*  SaveAsDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*   Dialog procedure for the advanced power scheme dialog.
*
*******************************************************************************/

INT_PTR CALLBACK SaveAsDlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    TCHAR           szBuf[2 * MAX_FRIENDLY_NAME_LEN]; // Leave room for DBCS
    PSCHEME_LIST    pslNew;
    static PBOOLEAN pbSavedCurrent;

    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemText(hWnd, IDC_SAVENAMEEDIT,  g_pslCurSel->lpszName);
            SendDlgItemMessage(hWnd, IDC_SAVENAMEEDIT, EM_SETSEL, 0, -1);
            SendDlgItemMessage(hWnd, IDC_SAVENAMEEDIT, EM_LIMITTEXT, MAX_FRIENDLY_NAME_LEN-2, 0L);
            EnableWindow(GetDlgItem(hWnd, IDOK), (g_pslCurSel->lpszName[0] != TEXT('\0')));
            pbSavedCurrent = (PBOOLEAN) lParam;
            *pbSavedCurrent = FALSE;
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SAVENAMEEDIT:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        GetDlgItemText(hWnd, IDC_SAVENAMEEDIT, szBuf, 2);
                        if (*szBuf) {
                            EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
                        }
                    }
                    break;

                case IDOK:
                    GetDlgItemText(hWnd, IDC_SAVENAMEEDIT, szBuf, MAX_FRIENDLY_NAME_LEN-1);

                    // Strip trailing blanks, don't allow blank scheme name.
                    if (!StripBlanks(szBuf)) {
                        MsgBoxId(hWnd, IDS_SAVESCHEME, IDS_BLANKNAME,
                                 NULL, MB_OK | MB_ICONEXCLAMATION);
                        return TRUE;
                    }

                    // Insert a new policies element in the policies list.
                    pslNew = AddScheme(NEWSCHEME, szBuf, STRSIZE(szBuf),
                             TEXT(""), sizeof(TCHAR), g_pslCurSel->ppp);

                    // Write out the Scheme.
                    if (pslNew) {
                        if (g_pslCurSel == pslNew) {
                            *pbSavedCurrent = TRUE;
                        }
                        else {
                            g_pslCurSel = pslNew;
                        }
                        WritePwrSchemeReport(hWnd,
                                             &(g_pslCurSel->uiID),
                                             g_pslCurSel->lpszName,
                                             g_pslCurSel->lpszDesc,
                                             g_pslCurSel->ppp);
                    }

                case IDCANCEL:
                    EndDialog(hWnd, wParam);
                    break;
            }
            break;

        case WM_HELP:             // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_SaveAsHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_SaveAsHelpIDs);
            return TRUE;
    }

    return FALSE;
}

/*******************************************************************************
*
*  PowerSchemeDlgProc
*
*  DESCRIPTION:
*   Dialog procedure for power scheme page.
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR CALLBACK PowerSchemeDlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    NMHDR  FAR  *lpnm;
    UINT   uiNewSel, uiNewState;
    LPTSTR pszUPS;
    static POWER_SCHEME_DLG_INFO  psdi;
    static BOOL bDirty = FALSE;
    static BOOL bInitFailed = FALSE;

    if (bInitFailed) {
        return FALSE;
    }

    switch (uMsg) {

        case WM_INITDIALOG:
            // Set the control count to match the dialog template we're using.
            if (g_SysPwrCapabilities.SystemBatteriesPresent) {
                g_uiNumPwrSchemeCntrls = NUM_POWER_SCHEME_CONTROLS;
                if (g_SysPwrCapabilities.BatteriesAreShortTerm) {
                   pszUPS = LoadDynamicString(IDS_POWEREDBYUPS);
                   DisplayFreeStr(hWnd, IDC_RUNNINGONBAT, pszUPS, FREE_STR);
                }
            }
            else {
                g_uiNumPwrSchemeCntrls = NUM_POWER_SCHEME_CONTROLS_NOBAT;
            }
            if (!PowerSchemeDlgInit(hWnd, &psdi)) {
                bInitFailed = TRUE;
            }
            return TRUE;

        case WM_CHILDACTIVATE:
            // If Systray changed something while another property page (dialog)
            // had the focus reinitialize the dialog.
            if (g_bSystrayChange) {
                PowerSchemeDlgInit(hWnd, &psdi);
                g_bSystrayChange = FALSE;
            }

            // Reinitialize hibernate timer since the hibernate tab
            // may have changed it's state.


            if (GetPwrCapabilities(&g_SysPwrCapabilities)) {
                if (g_bRunningUnderNT &&
                        g_SysPwrCapabilities.SystemS4 &&
                        g_SysPwrCapabilities.SystemS5 &&
                        g_SysPwrCapabilities.HiberFilePresent) {
                    uiNewState = CONTROL_ENABLE;
                } else {
                    uiNewState = CONTROL_HIDE;
                }

                if (g_bRunningUnderNT && (g_uiStandbyState == CONTROL_HIDE) &&
                        (g_SysPwrCapabilities.SystemS1 || 
                         g_SysPwrCapabilities.SystemS2 ||
                         g_SysPwrCapabilities.SystemS3)) {
                    
                    g_uiStandbyState = CONTROL_ENABLE;
                }
            }


            if (g_uiHiberState != uiNewState) {
                g_uiHiberState = uiNewState;
                MapHiberTimer(g_pslCurSel->ppp, FALSE);
                SetControls(hWnd, g_uiNumPwrSchemeCntrls, g_pcPowerScheme);
            }
            break;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code) {
                case PSN_APPLY:
                    if (bDirty) {

                        // Do the hibernate PSN_APPLY since the
                        // PowerSchemeDlgProc PSN_APPLY logic depends
                        // on hibernate state.
                        DoHibernateApply();

                        GetControls(hWnd, g_uiNumPwrSchemeCntrls,
                                    g_pcPowerScheme);
                        MapHiberTimer(g_pslCurSel->ppp, TRUE);

                        // Set active scheme.
                        if (SetActivePwrSchemeReport(hWnd,
                                                     g_pslCurSel->uiID,
                                                     NULL,
                                                     g_pslCurSel->ppp)) {

                            if (g_pslCurSel != g_pslCurActive) {
                                g_pslCurActive = g_pslCurSel;
                                RefreshSchemes(hWnd, g_pslCurSel);
                            }
                        }
                        bDirty = FALSE;

                        // The Power Policy Manager may have changed
                        // the scheme during validation.
                        MapHiberTimer(g_pslCurSel->ppp, FALSE);
                        SetControls(hWnd, g_uiNumPwrSchemeCntrls,
                                    g_pcPowerScheme);

                    }
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SCHEMECOMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        if (g_pslCurSel = GetCurSchemeFromCombo(hWnd)) {
                            HandleCurSchemeChanged(hWnd);
                            MarkSheetDirty(hWnd, &bDirty);
                        }
                    }
                    break;

                case IDC_STANDBYACCOMBO:
                case IDC_STANDBYDCCOMBO:
                    HandleIdleTimeOutChanged(hWnd, uMsg, wParam, &bDirty);
                    break;

                case IDC_MONITORACCOMBO:
                case IDC_MONITORDCCOMBO:
                case IDC_DISKACCOMBO:
                case IDC_DISKDCCOMBO:
                case IDC_HIBERACCOMBO:
                case IDC_HIBERDCCOMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        MarkSheetDirty(hWnd, &bDirty);
                    }
                    break;

                case IDC_SAVEAS:
                    if (DoSaveScheme(hWnd)) {
                        HandleCurSchemeChanged(hWnd);
                        MarkSheetDirty(hWnd, &bDirty);
                    }
                    break;

                case IDC_DELETE:
                    if (DoDeleteScheme(hWnd, g_pslCurSel->lpszName)) {
                        HandleCurSchemeChanged(hWnd);
                    }
                    break;

                default:
                    return FALSE;

            }
            break;

        case PCWM_NOTIFYPOWER:
            // Notification from systray, user has changed a PM UI setting.
            PowerSchemeDlgInit(hWnd, &psdi);
            break;

        case WM_HELP:             // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_PowerSchemeHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_PowerSchemeHelpIDs);
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
*  HandleIdleTimeOutChanged
*
*  DESCRIPTION:
*   Range limit the hibernate timeout combo boxes based on the value of the
*   idle timeouts.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID HandleIdleTimeOutChanged(HWND hWnd, UINT uMsg, WPARAM wParam, BOOL *pbDirty)
{
    UINT uiIdleTo, uiLimitedTo;

    if (HIWORD(wParam) == CBN_SELCHANGE) {

        MarkSheetDirty(hWnd, pbDirty);

        if (LOWORD(wParam) == IDC_STANDBYACCOMBO) {
            GetControls(hWnd, 1, &g_pcPowerScheme[ID_STANDBYACCOMBO]);
            GetControls(hWnd, 1, &g_pcPowerScheme[ID_HIBERACCOMBO]);
            uiIdleTo = g_uiIdleTimeoutAc;
            uiLimitedTo = RangeLimitHiberTimeOuts(uiIdleTo, g_uiHiberToAcIDs);
            if (g_uiHiberTimeoutAc && (uiIdleTo >= g_uiHiberTimeoutAc)) {
                g_uiHiberTimeoutAc = uiLimitedTo;
            }
            SetControls(hWnd, 1, &g_pcPowerScheme[ID_HIBERACCOMBO]);
        }
        else {
            GetControls(hWnd, 1, &g_pcPowerScheme[ID_STANDBYDCCOMBO]);
            GetControls(hWnd, 1, &g_pcPowerScheme[ID_HIBERDCCOMBO]);
            uiIdleTo = g_uiIdleTimeoutDc;
            uiLimitedTo = RangeLimitHiberTimeOuts(uiIdleTo, g_uiHiberToDcIDs);
            if (g_uiHiberTimeoutDc && (uiIdleTo >= g_uiHiberTimeoutDc)) {
                g_uiHiberTimeoutDc = uiLimitedTo;
            }
            SetControls(hWnd, 1, &g_pcPowerScheme[ID_HIBERDCCOMBO]);
        }
    }
}

/*******************************************************************************
*
*  RangeLimitHiberTimeOuts
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

UINT RangeLimitHiberTimeOuts(UINT uiIdleTimeout, UINT *uiHiberToIDs)
{
    UINT i, uiNewMin;

    // Initialize the hiber timout ID's to full range.
    memcpy(uiHiberToIDs, g_uiHiberToIDs, sizeof(g_uiHiberToIDs));

    if (uiIdleTimeout) {
        i = 0;
        while (uiHiberToIDs[i++]) {
            if (uiHiberToIDs[i] >= uiIdleTimeout) {
                i += 2;
                uiNewMin = uiHiberToIDs[i];
                RangeLimitIDarray(uiHiberToIDs, uiNewMin, (UINT) -1);
                return uiNewMin;
            }
            i++;
        }
        DebugPrint( "RangeLimitHiberTimeOuts: couldn't find value larger than: %d", uiIdleTimeout);
    }
    return (UINT) -1;
}

/*******************************************************************************
*
*  MapHiberTimer
*
*  DESCRIPTION:
*   Displayed hibernate timeout may never be less than the idle timeout. This
*   function handles the mapping. The following table (per KenR) specifies the
*   Idle action to be set by  the UI for different combinations of Idle and
*   Hibernate timeouts. It is understood that the Hibernate timeout UI only
*   appears when HiberFilePresent is TRUE. For case E, the HiberTimeout will be
*   set in the IdleTimeout member. For case F, the UI will adjust the
*   DozeS4Timeout member to be the displayed HiberTimeout plus the IdleTimeout.
*
* Case HiberFilePresent UiHiberTimeout UiIdleTimeout IdleAction           DozeS4Timeout   IdleTimeout
* ---------------------------------------------------------------------------------------------------------------
* A.   FALSE            N/A            0 (Never)     PowerActionNone      0               0
* B.   FALSE            N/A            !0            PowerActionSleep     0               UiIdleTimeout
* C.   TRUE             0 (Never)      0 (Never)     PowerActionNone      0               0
* D.   TRUE             0 (Never)      !0            PowerActionSleep     0               UiIdleTimeout
* E.   TRUE             !0             0 (Never)     PowerActionHibernate 0               UiHiberTimeout
* F.   TRUE             !0             !0            PowerActionSleep     UiHiber-UiIdle  UiIdleTimeout
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN MapHiberTimer(PPOWER_POLICY ppp, BOOLEAN Get)
{
   if (Get) {

      // Get values from the UI. AC.
      ppp->mach.DozeS4TimeoutAc =  0;
      ppp->user.IdleTimeoutAc   =  g_uiIdleTimeoutAc;
      if (g_uiHiberTimeoutAc) {
         if (g_uiIdleTimeoutAc) {
            ppp->mach.DozeS4TimeoutAc = g_uiHiberTimeoutAc - g_uiIdleTimeoutAc;
         }
         else {
            ppp->user.IdleTimeoutAc   = g_uiHiberTimeoutAc;
         }
      }

      // DC.
      ppp->mach.DozeS4TimeoutDc =  0;
      ppp->user.IdleTimeoutDc   =  g_uiIdleTimeoutDc;
      if (g_uiHiberTimeoutDc) {
         if (g_uiIdleTimeoutDc) {
            ppp->mach.DozeS4TimeoutDc = g_uiHiberTimeoutDc - g_uiIdleTimeoutDc;
         }
         else {
            ppp->user.IdleTimeoutDc   = g_uiHiberTimeoutDc;
         }
      }

      // Set the correct idle action. AC.
      ppp->user.IdleAc.Action = PowerActionNone;
      if (g_uiIdleTimeoutAc) {
         ppp->user.IdleAc.Action = PowerActionSleep;
      }
      else {
         if (g_SysPwrCapabilities.HiberFilePresent) {
            if (g_uiHiberTimeoutAc) {
               ppp->user.IdleAc.Action = PowerActionHibernate;
            }
         }
      }

      // DC.
      ppp->user.IdleDc.Action = PowerActionNone;
      if (g_uiIdleTimeoutDc) {
         ppp->user.IdleDc.Action = PowerActionSleep;
      }
      else {
         if (g_SysPwrCapabilities.HiberFilePresent) {
            if (g_uiHiberTimeoutDc) {
               ppp->user.IdleDc.Action = PowerActionHibernate;
            }
         }
      }
   }
   else {

      // Set values to the UI. AC.
      if (ppp->user.IdleAc.Action == PowerActionHibernate) {
         g_uiHiberTimeoutAc = ppp->user.IdleTimeoutAc;
         g_uiIdleTimeoutAc  = 0;
      }
      else {
         g_uiIdleTimeoutAc  = ppp->user.IdleTimeoutAc;
         if (ppp->mach.DozeS4TimeoutAc && g_SysPwrCapabilities.HiberFilePresent) {
            g_uiHiberTimeoutAc = ppp->user.IdleTimeoutAc +
                                 ppp->mach.DozeS4TimeoutAc;
         }
         else {
            g_uiHiberTimeoutAc = 0;
         }
      }

      // DC.
      if (ppp->user.IdleDc.Action == PowerActionHibernate) {
         g_uiHiberTimeoutDc = ppp->user.IdleTimeoutDc;
         g_uiIdleTimeoutDc  = 0;
      }
      else {
         g_uiIdleTimeoutDc  = ppp->user.IdleTimeoutDc;
         if (ppp->mach.DozeS4TimeoutDc && g_SysPwrCapabilities.HiberFilePresent) {
            g_uiHiberTimeoutDc = ppp->user.IdleTimeoutDc +
                                 ppp->mach.DozeS4TimeoutDc;
         }
         else {
            g_uiHiberTimeoutDc = 0;
         }
      }

      // Range limit the hibernate timeout combo boxes based
      // on the value of the idle timeouts.
      RangeLimitHiberTimeOuts(g_uiIdleTimeoutAc, g_uiHiberToAcIDs);
      RangeLimitHiberTimeOuts(g_uiIdleTimeoutDc, g_uiHiberToDcIDs);
   }
   return TRUE;
}

/*******************************************************************************
*
*  HandleCurSchemeChanged
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN HandleCurSchemeChanged(HWND hWnd)
{
    LPTSTR  pString;
    BOOL    bEnable;

    // Update the group box text if enabled.
    if ((g_uiStandbyState != CONTROL_HIDE) ||
        (g_uiMonitorState != CONTROL_HIDE) ||
        (g_uiDiskState    != CONTROL_HIDE) ||
        (g_uiHiberState   != CONTROL_HIDE)) {
        pString = LoadDynamicString(IDS_SETTINGSFORMAT, g_pslCurSel->lpszName);
        DisplayFreeStr(hWnd, IDC_SETTINGSFOR, pString, FREE_STR);
    }
    else {
        ShowWindow(GetDlgItem(hWnd, IDC_SETTINGSFOR), SW_HIDE);
    }

    // Update the power schemes combobox list.
    RefreshSchemes(hWnd, g_pslCurSel);

    // Setup the data pointers in the g_pcPowerScheme array.
    g_pcPowerScheme[ID_MONITORACCOMBO].lpdwParam =
        &(g_pslCurSel->ppp->user.VideoTimeoutAc);
    g_pcPowerScheme[ID_MONITORDCCOMBO].lpdwParam =
        &(g_pslCurSel->ppp->user.VideoTimeoutDc);
    g_pcPowerScheme[ID_DISKACCOMBO].lpdwParam =
        &(g_pslCurSel->ppp->user.SpindownTimeoutAc);
    g_pcPowerScheme[ID_DISKDCCOMBO].lpdwParam =
        &(g_pslCurSel->ppp->user.SpindownTimeoutDc);

    // Update the rest of the controls.
    MapHiberTimer(g_pslCurSel->ppp, FALSE);
    SetControls(hWnd, g_uiNumPwrSchemeCntrls, g_pcPowerScheme);

    // Set the delete push button state.
    if (g_uiSchemeCount < 2) {
        bEnable = FALSE;
    }
    else {
        bEnable = TRUE;
    }
    EnableWindow(GetDlgItem(hWnd, IDC_DELETE), bEnable);
    return TRUE;
}

/*******************************************************************************
*
*  GetCurSchemeFromCombo
*
*  DESCRIPTION:
*   Get the current selection from the power schemes combobox list.
*
*  PARAMETERS:
*
*******************************************************************************/

PSCHEME_LIST GetCurSchemeFromCombo(HWND hWnd)
{
    UINT            uiCBRet;
    PSCHEME_LIST    psl;

    uiCBRet = (UINT) SendDlgItemMessage(hWnd, IDC_SCHEMECOMBO, CB_GETCURSEL, 0, 0);
    if (uiCBRet != CB_ERR) {
        psl = (PSCHEME_LIST) SendDlgItemMessage(hWnd, IDC_SCHEMECOMBO,
                                                CB_GETITEMDATA, uiCBRet, 0);
        if (psl != (PSCHEME_LIST) CB_ERR) {
            return FindScheme(psl->lpszName, TRUE);
        }
    }
    DebugPrint( "GetCurSchemeFromCombo, CB_GETITEMDATA or CB_GETCURSEL failed");
    return FALSE;
}

/*******************************************************************************
*
*  ClearSchemeList
*
*  DESCRIPTION:
*   Clear the scheme list if it's not already empty. Return TRUE if there's
*   a change to the contents of power scheme list.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ClearSchemeList(VOID)
{
    PSCHEME_LIST psl, pslNext;

    if (IsListEmpty(&g_leSchemeList)) {
        return FALSE;
    }

    for (psl = (PSCHEME_LIST)g_leSchemeList.Flink;
         psl != (PSCHEME_LIST)&g_leSchemeList; psl = pslNext) {

        pslNext = (PSCHEME_LIST) psl->leSchemeList.Flink;
        RemoveScheme(psl, NULL);
    }
    g_pslCurActive  = NULL;
    g_pslCurSel     = NULL;
    g_uiSchemeCount = 0;
    return TRUE;
}

/*******************************************************************************
*
*  RemoveScheme
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN RemoveScheme(PSCHEME_LIST psl, LPTSTR lpszName)
{
    if (lpszName) {
        psl = FindScheme(lpszName, TRUE);
    }

    if (psl == &g_sl) {
        DebugPrint( "RemoveScheme, Attempted to delete head!");
        return FALSE;
    }

    if (psl) {
        LocalFree(psl->lpszName);
        LocalFree(psl->lpszDesc);
        RemoveEntryList(&psl->leSchemeList);
        LocalFree(psl);
        g_uiSchemeCount--;
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
*
*  FindScheme
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

PSCHEME_LIST FindScheme(LPTSTR lpszName, BOOLEAN bShouldExist)
{
    PSCHEME_LIST  psl, pslNext;

    if (!lpszName) {
        DebugPrint( "FindScheme, invalid parameters");
        return NULL;
    }

    // Search by name.
    for (psl = (PSCHEME_LIST)g_leSchemeList.Flink;
        psl != (PSCHEME_LIST)&g_leSchemeList; psl = pslNext) {

        pslNext = (PSCHEME_LIST) psl->leSchemeList.Flink;

        if (!lstrcmpi(lpszName, psl->lpszName)) {
            return psl;
        }
    }
    if (bShouldExist) {
        DebugPrint( "FindScheme, couldn't find: %s", lpszName);
    }
    return NULL;
}

/*******************************************************************************
*
*  AddScheme
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

PSCHEME_LIST AddScheme(
    UINT                    uiID,
    LPTSTR                  lpszName,
    UINT                    uiNameSize,
    LPTSTR                  lpszDesc,
    UINT                    uiDescSize,
    PPOWER_POLICY           ppp
)
{
    PSCHEME_LIST psl;

    if (!lpszName || !lpszDesc) {
        DebugPrint( "AddScheme, invalid parameters");
        return NULL;
    }

    // If a scheme of this name already exists just return a pointer to it.
    if (psl = FindScheme(lpszName, FALSE)) {
        return psl;
    }

    // Allocate and initalize a Scheme element for the scheme list.
    if ((psl = LocalAlloc(0, sizeof(SCHEME_LIST))) != NULL) {
        psl->uiID     = uiID;
        psl->lpszName = LocalAlloc(0, uiNameSize);
        psl->lpszDesc = LocalAlloc(0, uiDescSize);
        psl->ppp      = LocalAlloc(0, sizeof(POWER_POLICY));

        if (psl->lpszName && psl->lpszDesc && psl->ppp) {
            lstrcpy(psl->lpszName, lpszName);
            lstrcpy(psl->lpszDesc, lpszDesc);
            memcpy(psl->ppp, ppp, sizeof(POWER_POLICY));
            InsertTailList(&g_leSchemeList, &psl->leSchemeList);
            g_uiSchemeCount++;
            return psl;
        }

        LocalFree(psl->lpszName);
        LocalFree(psl->lpszDesc);
        LocalFree(psl->ppp);
        LocalFree(psl);
        psl = NULL;
    }
    return psl;
}

/*******************************************************************************
*
*  PowerSchemeEnumProc
*   Builds the policies list.
*
*  DESCRIPTION:
*
*  PARAMETERS:
*   lParam  - Is the ID of the currently active power scheme.
*
*******************************************************************************/

BOOLEAN CALLBACK PowerSchemeEnumProc(
    UINT                    uiID,
    DWORD                   dwNameSize,
    LPTSTR                  lpszName,
    DWORD                   dwDescSize,
    LPTSTR                  lpszDesc,
    PPOWER_POLICY           ppp,
    LPARAM                  lParam
)
{
    PSCHEME_LIST psl;

    // Validate the new scheme.
    if (ValidateUISchemeFields(ppp)) {

        // Allocate and initalize a policies element.
        if ((psl = AddScheme(uiID, lpszName, dwNameSize, lpszDesc,
                             dwDescSize, ppp)) != NULL) {

            // Save a valid entry for error recovery.
            g_pslValid = psl;

            // Setup currently active policies pointer.
            if ((UINT)lParam == uiID) {
                g_pslCurActive = psl;
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
*
*  PowerSchemeDlgInit
*
*  DESCRIPTION:
*   Initialize the power scheme dialog.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN PowerSchemeDlgInit(
    HWND                    hWnd,
    PPOWER_SCHEME_DLG_INFO  ppsdi
)
{
    UINT uiCurrentSchemeID;
    UINT i;

#ifdef WINNT
    // On WINNT, only power users may add new power schemes.
    if (CanUserWritePwrScheme()) {
        ShowWindow(GetDlgItem(hWnd, IDC_SAVEAS), SW_SHOW);
        ShowWindow(GetDlgItem(hWnd, IDC_DELETE), SW_SHOW);
    }
    else {
        ShowWindow(GetDlgItem(hWnd, IDC_SAVEAS), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, IDC_DELETE), SW_HIDE);
    }
#endif

    ppsdi->hwndSchemeList = GetDlgItem(hWnd, IDC_SCHEMECOMBO);
    ClearSchemeList();

    // Get the currently active power scheme.
    if (GetActivePwrScheme(&uiCurrentSchemeID)) {

        // Get the Policies list from PowrProf.
        for (i = 0; i < 2; i++) {
            if (EnumPwrSchemes(PowerSchemeEnumProc, (LPARAM)uiCurrentSchemeID) &&
                g_pslCurActive) {

                g_pslCurSel = g_pslCurActive;

                // Setup UI show/hide state variables.
                g_uiWhenComputerIsState = CONTROL_HIDE;
                if (g_SysPwrCapabilities.SystemS1 ||
                    g_SysPwrCapabilities.SystemS2 ||
                    g_SysPwrCapabilities.SystemS3) {
                    g_uiStandbyState = CONTROL_ENABLE;
                    g_uiWhenComputerIsState = CONTROL_ENABLE;
                }
                else {
                    g_uiStandbyState = CONTROL_HIDE;
                }

                if (g_bVideoLowPowerSupported) {
                    g_uiMonitorState = CONTROL_ENABLE;
                    g_uiWhenComputerIsState = CONTROL_ENABLE;
                }
                else {
                    g_uiMonitorState = CONTROL_HIDE;
                }

                if (g_SysPwrCapabilities.DiskSpinDown) {
                    g_uiDiskState = CONTROL_ENABLE;
                    RangeLimitIDarray(g_uiSpinDownIDs,
                                      HIWORD(g_uiSpindownMaxMin)*60,
                                      LOWORD(g_uiSpindownMaxMin)*60);
                }
                else {
                    g_uiDiskState = CONTROL_HIDE;
                }

                if (g_bRunningUnderNT &&
                    g_SysPwrCapabilities.SystemS4 &&
                    g_SysPwrCapabilities.SystemS5 &&
                    g_SysPwrCapabilities.HiberFilePresent) {
                    g_uiHiberState = CONTROL_ENABLE;
                }
                else {
                    g_uiHiberState = CONTROL_HIDE;
                }

                // Update the UI.
                HandleCurSchemeChanged(hWnd);
                return TRUE;
            }
            else {
                DebugPrint( "PowerSchemeDlgInit, failure enumerating schemes. g_pslCurActive: %X", g_pslCurActive);
                if (g_pslValid) {
                    if (SetActivePwrScheme(g_pslValid->uiID, NULL, g_pslValid->ppp)) {
                        uiCurrentSchemeID = g_pslValid->uiID;
                        ClearSchemeList();
                    }
                    else {
                        DebugPrint( "PowerSchemeDlgInit, unable to set valid scheme");
                    }
                }
                else {
                    DebugPrint( "PowerSchemeDlgInit, no valid schemes");
                    break;
                }
            }
        }
    }

    DisableControls(hWnd, g_uiNumPwrSchemeCntrls, g_pcPowerScheme);
    return FALSE;
}

/*******************************************************************************
*
*  RefreshSchemes
*
*  DESCRIPTION:
*   Update the power schemes combobox list.
*
*  PARAMETERS:
*   hWnd    - Power schemes dialog hWnd.
*   pslSel  - Power scheme to leave selected on exit.
*
*******************************************************************************/

VOID RefreshSchemes(
    HWND            hWnd,
    PSCHEME_LIST    pslSel
)
{
    PSCHEME_LIST    psl, pslNext;
    UINT            uiIndex;

    SendDlgItemMessage(hWnd, IDC_SCHEMECOMBO, CB_RESETCONTENT, FALSE, 0L);

    for (psl = (PSCHEME_LIST)g_leSchemeList.Flink;
         psl != (PSCHEME_LIST)&g_leSchemeList; psl = pslNext) {

        pslNext = (PSCHEME_LIST) psl->leSchemeList.Flink;

        // Add the schemes to the combo list box.
        uiIndex = (UINT) SendDlgItemMessage(hWnd, IDC_SCHEMECOMBO, CB_ADDSTRING,
                                            0, (LPARAM) psl->lpszName);
        if (uiIndex != CB_ERR) {
            SendDlgItemMessage(hWnd, IDC_SCHEMECOMBO, CB_SETITEMDATA,
                               uiIndex, (LPARAM) psl);
        }
        else {
            DebugPrint( "RefreshSchemes, CB_ADDSTRING failed: %s", psl->lpszName);
        }
    }

    // Select the passed entry.
    if (pslSel) {
        uiIndex = (UINT) SendDlgItemMessage(hWnd, IDC_SCHEMECOMBO, CB_FINDSTRINGEXACT,
                                            (WPARAM)-1, (LPARAM)pslSel->lpszName);
        if (uiIndex != CB_ERR) {
            uiIndex = (UINT) SendDlgItemMessage(hWnd, IDC_SCHEMECOMBO, CB_SETCURSEL,
                                                (WPARAM)uiIndex, 0);
            if (uiIndex == CB_ERR) {
                DebugPrint( "RefreshSchemes, CB_SETCURSEL failed: %s, index: %d", psl->lpszName, uiIndex);
            }
        }
        else {
            DebugPrint( "RefreshSchemes, CB_FINDSTRINGEXACT failed: %s", psl->lpszName);
        }
    }
}

/*******************************************************************************
*
*  StripBlanks
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

UINT StripBlanks(LPTSTR lpszString)
{
    LPTSTR lpszPosn, lpszSrc;

    /* strip leading blanks */
    lpszPosn = lpszString;
    while(*lpszPosn == TEXT(' ')) {
            lpszPosn++;
    }
    if (lpszPosn != lpszString)
        lstrcpy(lpszString, lpszPosn);

    /* strip trailing blanks */
    if ((lpszPosn=lpszString+lstrlen(lpszString)) != lpszString) {
        lpszPosn = CharPrev(lpszString, lpszPosn);
        while(*lpszPosn == TEXT(' '))
           lpszPosn = CharPrev(lpszString, lpszPosn);
        lpszPosn = CharNext(lpszPosn);
        *lpszPosn = TEXT('\0');
    }
    return lstrlen(lpszString);
}

/*******************************************************************************
*
*  MsgBoxId
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

LONG MsgBoxId(
    HWND    hWnd,
    UINT    uiCaption,
    UINT    uiFormat,
    LPTSTR  lpszParam,
    UINT    uiFlags
)
{
    LPTSTR  lpszCaption;
    LPTSTR  lpszText;
    LONG    lRet = 0;

    lpszCaption = LoadDynamicString(uiCaption);
    if (lpszCaption) {
        lpszText = LoadDynamicString(uiFormat, lpszParam);
        if (lpszText) {
            lRet = MessageBox(hWnd, lpszText, lpszCaption, uiFlags);
            LocalFree(lpszText);
        }
        LocalFree(lpszCaption);
    }
    return lRet;
}

/*******************************************************************************
*
*  DoDeleteScheme
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DoDeleteScheme(HWND hWnd, LPTSTR lpszName)
{
    LPTSTR          lpszCaption;
    LPTSTR          lpszText;
    PSCHEME_LIST    psl, pslDelete;

    // Dont't allow delete unless we have at least two schemes.
    if ((g_uiSchemeCount < 2) || !(pslDelete = FindScheme(lpszName, TRUE))) {
        return FALSE;
    }

    // Get confirmation from the user.
    if (IDYES == MsgBoxId(hWnd, IDS_CONFIRMDELETECAPTION, IDS_CONFIRMDELETE,
                          lpszName, MB_YESNO | MB_ICONQUESTION)) {

        // If we deleted the currently active scheme set the next scheme active.
        if (pslDelete == g_pslCurActive) {
            if ((psl = FindNextScheme(lpszName)) &&
                (SetActivePwrSchemeReport(hWnd, psl->uiID, NULL, psl->ppp))) {
                g_pslCurActive = psl;
            }
            else {
                return FALSE;
            }
        }

        // Remove requested scheme.
        if (DeletePwrScheme(pslDelete->uiID)) {
            RemoveScheme(NULL, lpszName);
            g_pslCurSel = g_pslCurActive;
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
*
*  FindNextScheme
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

PSCHEME_LIST FindNextScheme(LPTSTR lpszName)
{
    PSCHEME_LIST psl, pslFirst, pslNext;

    for (pslFirst = psl = (PSCHEME_LIST)g_leSchemeList.Flink;
        psl != (PSCHEME_LIST)&g_leSchemeList; psl = pslNext) {

        pslNext = (PSCHEME_LIST) psl->leSchemeList.Flink;

        if (!lstrcmpi(lpszName, psl->lpszName)) {
            if (pslNext != (PSCHEME_LIST)&g_leSchemeList) {
                return pslNext;
            }
            else {
                return pslFirst;
            }
        }
    }
    DebugPrint( "FindNextScheme, unable to find: %s", lpszName);
    return NULL;
}

/*******************************************************************************
*
*  DoSaveScheme
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DoSaveScheme(HWND hWnd)
{
    POWER_POLICY    ppSave;
    BOOLEAN         bSavedCurrent;
    PSCHEME_LIST    pslTemplateScheme = g_pslCurSel;

    // Make a copy of the template scheme to restore after the save.
    memcpy(&ppSave, pslTemplateScheme->ppp, sizeof(ppSave));

    // Get any changes the user might have made for the new scheme.
    GetControls(hWnd, g_uiNumPwrSchemeCntrls, g_pcPowerScheme);
    MapHiberTimer(g_pslCurSel->ppp, TRUE);


    if (IDOK != DialogBoxParam(g_hInstance,
                               MAKEINTRESOURCE(IDD_SAVE),
                               hWnd,
                               SaveAsDlgProc,
                               (LPARAM)&bSavedCurrent)) {
        return FALSE;
    }

    // Restore the template scheme if we didn't save the current scheme.
    if (!bSavedCurrent) {
        memcpy(pslTemplateScheme->ppp, &ppSave, sizeof(ppSave));
        return TRUE;
    }
    return FALSE;
}
