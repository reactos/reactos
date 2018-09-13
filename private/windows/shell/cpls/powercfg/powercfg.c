/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       POWERCFG.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Power management UI. Control panel applet.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlwapi.h>
#include <cpl.h>
#include <help.h>
#include <regstr.h>

#include "powercfg.h"
#include "pwrresid.h"

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

HINSTANCE   g_hInstance;        // Global instance handle of this DLL.
UINT        wHelpMessage;       // Registered help message.

// Registry path to optional top level powercfg pages.
TCHAR g_szRegOptionalPages[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Power");

// Array which defines the property tabs/pages in the applet.
// The contents of this array are built dynamically, since they
// depend on the machine power management capabilities.

POWER_PAGES g_TopLevelPages[MAX_PAGES] =
{
    MAKEINTRESOURCE(IDS_APPNAME),   NULL,   0,   // Caption
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0,
    0,                              NULL,   0
};

// Specifies which OS, filled in at init CPL time.
BOOL g_bRunningUnderNT;

// This structure is filled in by the Power Policy Manager at CPL_INIT time.
SYSTEM_POWER_CAPABILITIES g_SysPwrCapabilities;

// The following globals are derived from g_SysPwrCapabilities:
DWORD g_dwNumSleepStates = 1;   // Every one suports PowerSystemWorking.
DWORD g_dwSleepStatesMaxMin;    // Specs sleep states slider range.
DWORD g_dwBattryLevelMaxMin;    // Specs battery level slider range.
DWORD g_dwProcThrottleMaxMin;   // Specs processor throttle slider range.
DWORD g_dwFanThrottleMaxMin;    // Specs fan throttle slider range.
BOOL  g_bVideoLowPowerSupported;// This will be moved to g_SysPwrCapabilities

UINT  g_uiVideoTimeoutMaxMin;   // May be set from registry.
UINT  g_uiSpindownMaxMin;       // May be set from registry.
PUINT g_puiBatCount;            // Number of displayable batteries.
BOOL  g_bIsUserAdministrator;   // The current user has admin. privileges.

// Static flags:
UINT g_uiOverrideAppsFlag = POWER_ACTION_OVERRIDE_APPS;
UINT g_uiDisableWakesFlag = POWER_ACTION_DISABLE_WAKES;

int  BuildPages(PSYSTEM_POWER_CAPABILITIES, PPOWER_PAGES);
VOID SyncRegPPM(VOID);


/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  DllInitialize
*
*  DESCRIPTION:
*   Library entry point
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL DllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL)
{

    UNREFERENCED_PARAMETER(pctx);

    switch (ulReason) {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hmod;
            DisableThreadLibraryCalls(g_hInstance);
            wHelpMessage = RegisterWindowMessage(TEXT("ShellHelp"));
            InitSchemesList();
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}


/*******************************************************************************
*
*  CplApplet
*
*  DESCRIPTION:
*   Called by control panel.
*
*  PARAMETERS:
*
*******************************************************************************/

LRESULT APIENTRY
CPlApplet(
    HWND hCPLWnd,
    UINT Message,
    LPARAM wParam,
    LPARAM lParam
    )
{
    LPNEWCPLINFO  lpNewCPLInfo;
    LPCPLINFO     lpCPlInfo;
    WNDCLASS      cls;
    DWORD         dwSize, dwSessionId, dwTry = 0;
    OSVERSIONINFO osvi;

    switch (Message) {

        case CPL_INIT:              // Is there an applet ?
            // Set OS global.
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            GetVersionEx(&osvi);
            g_bRunningUnderNT = (osvi.dwMajorVersion >= 5) &&
                                (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);

            // If we're running under NT don't allow power management UI
            // unless we have power management capabilities.
            if (g_bRunningUnderNT) {
                if (!PowerCapabilities()) {
                    return FALSE;
                }
            }

#ifdef WINNT
            // On NT don't allow power management UI
            // if we're running as a remote session. As per SalimC.
            if (ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) {
                if (dwSessionId) {
                    return FALSE;
                }
            }
#endif
            // Set global variables based on the machine capabilities.
            return InitCapabilities(&g_SysPwrCapabilities);

        case CPL_GETCOUNT:          // PowerCfg.Cpl supports one applet.
            return 1;

#ifdef WINNT
        case CPL_INQUIRE:           // Fill CplInfo structure
            lpCPlInfo = (LPCPLINFO)lParam;
            lpCPlInfo->idIcon = CPL_DYNAMIC_RES;
            lpCPlInfo->idName = CPL_DYNAMIC_RES;
            lpCPlInfo->idInfo = CPL_DYNAMIC_RES;
            lpCPlInfo->lData  = 0;
            return 1;

        case CPL_NEWINQUIRE:
            lpNewCPLInfo = (LPNEWCPLINFO)lParam;
            memset(lpNewCPLInfo, 0, sizeof(NEWCPLINFO));
            lpNewCPLInfo->dwSize = sizeof(NEWCPLINFO);

            // On NT don't allow power management UI icon
            // if we're running as a remote session. As per SalimC.

            if (ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) {
                if (dwSessionId) {
                    return 1;
                }
            }

            lpNewCPLInfo->hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_PWRMNG));
            LoadString(g_hInstance, IDS_APPNAME, lpNewCPLInfo->szName, sizeof(lpNewCPLInfo->szName));
            LoadString(g_hInstance, IDS_INFO, lpNewCPLInfo->szInfo, sizeof(lpNewCPLInfo->szInfo));
            lstrcpy(lpNewCPLInfo->szHelpFile, TEXT(""));
            return 1;
#else
        case CPL_INQUIRE:           // Fill CplInfo structure
            lpCPlInfo = (LPCPLINFO)lParam;
            lpCPlInfo->idIcon = IDI_PWRMNG;
            lpCPlInfo->idName = IDS_APPNAME;
            lpCPlInfo->idInfo = IDS_INFO;
            lpCPlInfo->lData  = 0;
            break;

        case CPL_NEWINQUIRE:
            lpNewCPLInfo = (LPNEWCPLINFO)lParam;
            memset(lpNewCPLInfo, 0, sizeof(lpNewCPLInfo));
            lpNewCPLInfo->dwSize = sizeof( lpNewCPLInfo );
            lpNewCPLInfo->hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_PWRMNG));
            LoadString(g_hInstance, IDS_APPNAME, lpNewCPLInfo->szName, sizeof(lpNewCPLInfo->szName));
            LoadString(g_hInstance, IDS_INFO, lpNewCPLInfo->szInfo, sizeof(lpNewCPLInfo->szInfo));
            lstrcpy(lpNewCPLInfo->szHelpFile, TEXT(""));
            return TRUE;

#endif

        case CPL_DBLCLK:          // This applet has been chosen to run
        case CPL_STARTWPARMS:     // Started from RUNDLL

            // Initialize the common controls.
            InitCommonControls();

            // Sync the current scheme registry and PPM.
            SyncRegPPM();

            // Build the displayed pages base on system capabilities.
            BuildPages(&g_SysPwrCapabilities, g_TopLevelPages);

            // Will return FALSE if we didn't display any pages.
            return DoPropSheetPages(hCPLWnd, &(g_TopLevelPages[0]),
                                    g_szRegOptionalPages);

        case CPL_EXIT:            // This applet must die
        case CPL_STOP:
            break;

        case CPL_SELECT:          // This applet has been selected
            break;
    }

    return FALSE;
}


/*******************************************************************************
*
*  LoadDynamicString
*
*  DESCRIPTION:
*     Wrapper for the FormatMessage function that loads a string from our
*     resource table into a dynamically allocated buffer, optionally filling
*     it with the variable arguments passed.
*
*  PARAMETERS:
*     StringID, resource identifier of the string to use.
*     (optional), parameters to use to format the string message.
*
*******************************************************************************/

LPTSTR CDECL LoadDynamicString( UINT StringID, ... )
{
    va_list Marker;
    TCHAR Buffer[256];
    LPTSTR pStr;
    int   iLen;

    // va_start is a macro...it breaks when you use it as an assign...on ALPHA.
    va_start(Marker, StringID);

    iLen = LoadString(g_hInstance, StringID, Buffer, sizeof(Buffer));

    if (iLen == 0) {
        DebugPrint( "LoadDynamicString: LoadString on: %X failed", StringID);
        return NULL;
    }

    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        (LPVOID) (LPTSTR) Buffer, 0, 0, (LPTSTR) &pStr, 0, &Marker);

    return pStr;
}

/*******************************************************************************
*
*  DisplayFreeStr
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

LPTSTR DisplayFreeStr(HWND hWnd, UINT uID, LPTSTR  pStr, BOOL bFree)
{
    if (pStr) {
        SetDlgItemText(hWnd, uID, pStr);
        ShowWindow(GetDlgItem(hWnd, uID), SW_SHOWNOACTIVATE);
        if (bFree) {
            LocalFree(pStr);
            return NULL;
        }
    }
    else {
        ShowWindow(GetDlgItem(hWnd, uID), SW_HIDE);
    }
    return pStr;
}

/*******************************************************************************
*
*  ValidateUISchemeFields
*
*  DESCRIPTION:
*   Validate only the data values which are set by our UI.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ValidateUISchemeFields(PPOWER_POLICY ppp)
{
    POWER_POLICY pp;
    static PGLOBAL_POWER_POLICY pgpp;

    memcpy(&pp, ppp, sizeof(pp));

    if (ValidatePowerPolicies(NULL, &pp)) {

        if (g_SysPwrCapabilities.HiberFilePresent) {
            ppp->mach.DozeS4TimeoutAc = pp.mach.DozeS4TimeoutAc;
            ppp->mach.DozeS4TimeoutDc = pp.mach.DozeS4TimeoutDc;
        }

        if (g_SysPwrCapabilities.SystemS1 ||
            g_SysPwrCapabilities.SystemS2 ||
            g_SysPwrCapabilities.SystemS3) {
            ppp->user.IdleTimeoutAc = pp.user.IdleTimeoutAc;
            ppp->user.IdleTimeoutDc = pp.user.IdleTimeoutDc;
        }

        if (g_bVideoLowPowerSupported) {
            ppp->user.VideoTimeoutAc = pp.user.VideoTimeoutAc;
            ppp->user.VideoTimeoutDc = pp.user.VideoTimeoutDc;
        }

        if (g_SysPwrCapabilities.DiskSpinDown) {
             ppp->user.SpindownTimeoutAc = pp.user.SpindownTimeoutAc;
             ppp->user.SpindownTimeoutDc = pp.user.SpindownTimeoutDc;
        }
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
*
*  GetGlobalPwrPolicy
*
*  DESCRIPTION:
*   Read the global power policy and validate only the data values which are
*   set by our UI.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN GetGlobalPwrPolicy(PGLOBAL_POWER_POLICY pgpp)
{
    int i;
    GLOBAL_POWER_POLICY gpp;

    if (ReadGlobalPwrPolicy(pgpp)) {

        memcpy(&gpp, pgpp, sizeof(gpp));

        if (ValidatePowerPolicies(&gpp, NULL)) {

            if (g_SysPwrCapabilities.PowerButtonPresent &&
                !g_SysPwrCapabilities.SleepButtonPresent) {
                pgpp->user.PowerButtonAc = gpp.user.PowerButtonAc;
                pgpp->user.PowerButtonDc = gpp.user.PowerButtonDc;
            }
            if (g_SysPwrCapabilities.LidPresent) {
                pgpp->user.LidCloseAc = gpp.user.LidCloseAc;
                pgpp->user.LidCloseDc = gpp.user.LidCloseDc;
            }
            if (g_SysPwrCapabilities.SystemBatteriesPresent) {
                for (i = 0; i < NUM_DISCHARGE_POLICIES; i++) {
                    pgpp->user.DischargePolicy[i] = gpp.user.DischargePolicy[i];
                }
            }
            pgpp->user.GlobalFlags = gpp.user.GlobalFlags;
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
*
*  ErrorMsgBox
*
*  DESCRIPTION:
*   Display a messag box for system message strings specified by dwErr and
*   title string specified by uiTitleID.
*
*  PARAMETERS:
*
*******************************************************************************/

int ErrorMsgBox(
    HWND    hwnd,
    DWORD   dwErr,
    UINT    uiTitleID)
{
   LPTSTR pszErr   = NULL;
   LPTSTR pszTitle = NULL;
   TCHAR  szUnknownErr[64];
   UINT   idRet;

   if (pszTitle = LoadDynamicString(uiTitleID)) {
      if (LoadString(g_hInstance, IDS_UNKNOWN_ERROR,
                     szUnknownErr, sizeof(szUnknownErr))) {
         pszErr = szUnknownErr;
      }
   }

   if (dwErr == NO_ERROR) {
      dwErr = GetLastError();
   }

   if (dwErr != NO_ERROR) {
      if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                         NULL,
                         dwErr,
                         0,
                         (LPTSTR)&pszErr,
                         1,
                         NULL)) {
         if (LoadString(g_hInstance, IDS_UNKNOWN_ERROR,
                        szUnknownErr, sizeof(szUnknownErr))) {
            pszErr = szUnknownErr;
         }
      }
   }

   idRet = MessageBox(hwnd, pszErr, pszTitle, MB_ICONEXCLAMATION);

   if (pszTitle) {
      LocalFree(pszTitle);
   }

   if ((pszErr) && (pszErr != szUnknownErr)) {
      LocalFree(pszErr);
   }
   return idRet;
}

/*******************************************************************************
*
*  WritePwrSchemeReport
*
*  DESCRIPTION:
*   Cover for WritePwrScheme with error reporting.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WritePwrSchemeReport(
    HWND            hwnd,
    PUINT           puiID,
    LPTSTR          lpszSchemeName,
    LPTSTR          lpszDescription,
    PPOWER_POLICY   lpScheme
)
{
   if (WritePwrScheme(puiID, lpszSchemeName, lpszDescription, lpScheme)) {
      return TRUE;
   }
   else {
      ErrorMsgBox(hwnd, NO_ERROR, IDS_UNABLETOSETPOLICY);
      return FALSE;
   }
}

/*******************************************************************************
*
*  WriteGlobalPwrPolicyReport
*
*  DESCRIPTION:
*   Cover for WriteGlobalPwrPolicy with error reporting.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN WriteGlobalPwrPolicyReport(
    HWND                   hwnd,
    PGLOBAL_POWER_POLICY   pgpp
)
{
   if (WriteGlobalPwrPolicy(pgpp)) {
      return TRUE;
   }
   else {
      ErrorMsgBox(hwnd, NO_ERROR, IDS_UNABLETOSETGLOBALPOLICY);
      return FALSE;
   }
}

/*******************************************************************************
*
*  SetActivePwrSchemeReport
*
*  DESCRIPTION:
*   Cover for WriteGlobalPwrPolicy with error reporting.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN SetActivePwrSchemeReport(
    HWND                    hwnd,
    UINT                    uiID,
    PGLOBAL_POWER_POLICY    pgpp,
    PPOWER_POLICY           ppp)
{
   if (SetActivePwrScheme(uiID, pgpp, ppp)) {
      return TRUE;
   }
   else {
      ErrorMsgBox(hwnd, NO_ERROR, IDS_UNABLETOSETACTIVEPOLICY);
      return FALSE;
   }
}

/*******************************************************************************
*
*                 P R I V A T E   F U N C T I O N S
*
*******************************************************************************/

/*******************************************************************************
*
*  BuildPages
*
*  DESCRIPTION:
*   Build the g_TopLevelPages array based on the machine capabilities. The
*   order of the tabs\pages is set here.
*
*  PARAMETERS:
*
*******************************************************************************/

int BuildPages(PSYSTEM_POWER_CAPABILITIES pspc, PPOWER_PAGES ppp)
{
    int     iPageCount = 1;     // We always have at least the power scheme page.


    // Do we have system batteries? Different dialog templates will be used
    // depending on the answer to this question.
    if (pspc->SystemBatteriesPresent) {
        AppendPropSheetPage(ppp, IDD_POWERSCHEME, PowerSchemeDlgProc);
        AppendPropSheetPage(ppp, IDD_ALARMPOLICY, AlarmDlgProc);
        iPageCount++;

        // Is there a battery driver that the battery meter can query?
        if (BatMeterCapabilities(&g_puiBatCount)) {
            AppendPropSheetPage(ppp, IDD_BATMETERCFG, BatMeterCfgDlgProc);
            iPageCount++;
        }
    }
    else {
        // No battery pages.
        AppendPropSheetPage(ppp, IDD_POWERSCHEME_NOBAT, PowerSchemeDlgProc);
    }

    // Always show the Advanced page.
    AppendPropSheetPage(ppp, IDD_ADVANCEDPOLICY, AdvancedDlgProc);
    iPageCount++;

    // Can we put up the hibernate page?
    if (pspc->SystemS4) {
        AppendPropSheetPage(ppp, IDD_HIBERNATE, HibernateDlgProc);
        iPageCount++;
    }

#ifdef WINNT
    if (pspc->ApmPresent) {
        //
        // Is APM present on the machine?  This page is
        // not shown if ACPI is present
        //
        AppendPropSheetPage(ppp, IDD_APM, APMDlgProc);
        iPageCount++;
    }

    if (pspc->UpsPresent) {
        AppendPropSheetPage(ppp, IDD_UPS, UPSMainPageProc);
        iPageCount++;
    }

#endif

    return iPageCount;
}

/*******************************************************************************
*
*  InitCapabilities
*
*  DESCRIPTION:
*   Call down to the PPM to get power management capabilities and set
*   global variables based on the results.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL InitCapabilities(PSYSTEM_POWER_CAPABILITIES pspc)
{
    UINT i, uiGran = 0, uiMax, uiMin;
    ADMINISTRATOR_POWER_POLICY app;
    int   dummy;

    // Set hard limits. These may be overridden by optional registry values.
    g_uiVideoTimeoutMaxMin = MAKELONG((short) MAX_VIDEO_TIMEOUT, (short) 1);
    g_uiSpindownMaxMin     = MAKELONG((short) MAX_SPINDOWN_TIMEOUT,(short) 1);

    g_dwNumSleepStates = 0;
    if (!GetPwrCapabilities(pspc)) {
            return FALSE;
    }


    if (pspc->SystemS1) {
            g_dwNumSleepStates++;
    }

    if (pspc->SystemS2) {
            g_dwNumSleepStates++;
    }

    if (pspc->SystemS3) {
            g_dwNumSleepStates++;
    }

    if (pspc->SystemS4) {
            g_dwNumSleepStates++;
    }

    // Get administrator overrides if present.
    if (IsAdminOverrideActive(&app)) {
        if (app.MaxVideoTimeout > -1) {
            uiMin = LOWORD(g_uiVideoTimeoutMaxMin);
            uiMax = app.MaxVideoTimeout;
            g_uiVideoTimeoutMaxMin = MAKELONG((short) uiMax,(short) uiMin);
        }

        if (app.MaxSleep < PowerSystemHibernate) {
            g_dwNumSleepStates = (DWORD)app.MaxSleep;
        }
    }

    // Get the optional disk spindown timeout range.
    if (GetPwrDiskSpindownRange(&uiMax, &uiMin)) {
        g_uiSpindownMaxMin = MAKELONG((short) uiMax,(short) uiMin);
    }

    if (g_dwNumSleepStates > 1) {
        g_dwSleepStatesMaxMin =
            MAKELONG((short) 0, (short) g_dwNumSleepStates - 1);
    }

    g_dwBattryLevelMaxMin = MAKELONG((short)0, (short)100);

    g_dwProcThrottleMaxMin =
        MAKELONG((short)0, (short)pspc->ProcessorThrottleScale);
    g_dwFanThrottleMaxMin =  MAKELONG((short)0, (short)100);


    // Call will fail if monitor or adapter don't support DPMS.
    g_bVideoLowPowerSupported = SystemParametersInfo(SPI_GETLOWPOWERACTIVE,
                                                     0, &dummy, 0);
    if (!g_bVideoLowPowerSupported) {
        g_bVideoLowPowerSupported = SystemParametersInfo(SPI_GETPOWEROFFACTIVE,
                                                         0, &dummy, 0);
    }

#ifdef WINNT
    //
    // Check to see if APM is present
    //
    pspc->ApmPresent = IsNtApmPresent(pspc);
    pspc->UpsPresent = IsUpsPresent(pspc);
#endif

    return TRUE;
}

/*******************************************************************************
*
*  SyncRegPPM
*
*  DESCRIPTION:
*   Call down to the PPM to get the current power policies and write them
*   to the registry. This is done in case the PPM is out of sync with the
*   PowerCfg registry settings. Requested by JVert.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID SyncRegPPM(VOID)
{
   GLOBAL_POWER_POLICY  gpp;
   POWER_POLICY         pp;
   UINT                 uiID, uiFlags = 0;

   if (ReadGlobalPwrPolicy(&gpp)) {
       uiFlags = gpp.user.GlobalFlags;
   }

   if (GetActivePwrScheme(&uiID)) {
      // Get the current PPM settings.
      if (GetCurrentPowerPolicies(&gpp, &pp)) {
         SetActivePwrScheme(uiID, &gpp, &pp);
      }
   }

   gpp.user.GlobalFlags |= uiFlags;
}

