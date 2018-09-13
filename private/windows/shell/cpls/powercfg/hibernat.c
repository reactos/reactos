/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       HIBERNAT.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Support for hibernate page of PowerCfg.Cpl.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>
#include <help.h>
#include <powercfp.h>

#include "powercfg.h"
#include "pwrresid.h"
#include "PwrMn_cs.h"

// Private functions implemented in HIBERNAT.C

VOID SetNumberMB(LPTSTR, DWORD);
VOID InsertSeparators(LPTSTR);
UINT UpdateFreeSpace(HWND, UINT);
UINT UpdatePhysMem(void);

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

// This structure is filled in by the Power Policy Manager at CPL_INIT time.
extern SYSTEM_POWER_CAPABILITIES g_SysPwrCapabilities;
extern BOOL g_bRunningUnderNT;

// A systary change requires PowerSchemeDlgProc re-init.
extern BOOL g_bSystrayChange;

// Persistant storage of this data is managed by POWRPROF.DLL API's.
extern GLOBAL_POWER_POLICY  g_gpp;

// Power button power action string ID's. With and without hibernate.
UINT g_uiPwrActIDs[] =
{
    IDS_STANDBY,    PowerActionSleep,
    IDS_HIBERNATE,  PowerActionHibernate,
    IDS_SHUTDOWN,   PowerActionShutdownOff,
    0,              0
};

// Lid action string ID's. With and without hibernate.
UINT g_uiLidActIDs[] =
{
    IDS_NONE,       PowerActionNone,
    IDS_STANDBY,    PowerActionSleep,
    IDS_HIBERNATE,  PowerActionHibernate,
    IDS_SHUTDOWN,   PowerActionShutdownOff,
    0,              0
};

// UI state variables
TCHAR   g_szRequiredSpace[128];
DWORD   g_dwShowHibernate;
DWORD   g_dwShowNoDiskSpace;
DWORD   g_dwShowDiskSpace;
DWORD   g_dwTrueFlag = (DWORD) TRUE;
BOOLEAN g_bHibernate;

// Globals for DoHibernateApply:
BOOL    g_bHibernateDirty;
HWND    g_hwndHibernateDlg;
UINT    g_uiRequiredMB;

// Hibernate policies dialog controls descriptions:

#define NUM_HIBERNATE_POL_CONTROLS 7

// Handy indicies into our g_pcHibernatePol array:
#define ID_REQUIREDSPACE    0
#define ID_NOTENOUGHSPACE   1
#define ID_HIBERNATE        2

POWER_CONTROLS g_pcHibernatePol[NUM_HIBERNATE_POL_CONTROLS] =
{// Control ID              Control Type    Data Address        Data Size               Parameter Pointer    Enable/Visible State Pointer
    IDC_REQUIREDSPACE,      EDIT_TEXT_RO,   &g_szRequiredSpace, 0,                      NULL,                &g_dwShowDiskSpace,
    IDC_NOTENOUGHSPACE,     STATIC_TEXT,    NULL,               0,                      NULL,                &g_dwShowNoDiskSpace,
    IDC_HIBERNATE,          CHECK_BOX,      &g_bHibernate,      sizeof(g_bHibernate),   &g_dwTrueFlag,       &g_dwShowHibernate,
    IDC_DISKSPACEGROUPBOX,  STATIC_TEXT,    NULL,               0,                      NULL,                &g_dwShowDiskSpace,
    IDC_FREESPACETEXT,      STATIC_TEXT,    NULL,               0,                      NULL,                &g_dwShowDiskSpace,
    IDC_REQUIREDSPACETEXT,  STATIC_TEXT,    NULL,               0,                      NULL,                &g_dwShowDiskSpace,
    IDC_FREESPACE,          STATIC_TEXT,    NULL,               0,                      NULL,                &g_dwShowDiskSpace,
};

// "Hibernate" Dialog Box (IDD_HIBERNATE == 105) help array:

const DWORD g_HibernateHelpIDs[]=
{
    IDC_HIBERNATE,          IDH_105_1400,   // Hibernate: "After going on standby, &hibernate." (Button)
    IDC_FREESPACE,          IDH_105_1401,   // Hibernate: "Free space" (Static)
    IDC_REQUIREDSPACE,      IDH_105_1402,   // Hibernate: "Required space to hibernate" (Static)
    IDC_NOTENOUGHSPACE,     IDH_105_1403,   // Hibernate: "You must free up some disk space before your computer can hibernate." (Static)
    IDC_DISKSPACEGROUPBOX,  IDH_105_1402,
    IDC_FREESPACETEXT,      IDH_105_1401,
    IDC_REQUIREDSPACETEXT,  IDH_105_1402,
    IDC_HIBERNATEGROUPBOX,  IDH_105_1400,
    IDI_HIBERNATE,          NO_HELP,
    IDC_NO_HELP_6,          NO_HELP,
    IDI_PWRMNG,             NO_HELP,
    0, 0
};

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  MapPwrAct
*
*  DESCRIPTION:
*   Map power action to one of a lesser number of UI supported actions.
*   Depends on state of hibernate so implemented here.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL MapPwrAct(
    PPOWER_ACTION   ppa,
    BOOL            bNone
)
{
    switch (*ppa) {
        case PowerActionNone:
            if (bNone) {
                *ppa = PowerActionNone;
                break;
            }
        case PowerActionReserved:
        case PowerActionSleep:
            *ppa = PowerActionSleep;
            break;

        case PowerActionHibernate:
            if (g_SysPwrCapabilities.HiberFilePresent) {
                *ppa = PowerActionHibernate;
            }
            else {
                *ppa = PowerActionSleep;
            }
            break;

        case PowerActionShutdown:
        case PowerActionShutdownReset:
        case PowerActionShutdownOff:
            *ppa = PowerActionShutdownOff;
            break;

        default:
            DebugPrint( "MapPwrAct, unknown power action: %X", *ppa);
            *ppa = PowerActionShutdownOff;
            return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
*
*  DoHibernateApply
*
*  DESCRIPTION:
*   Handle the WM_NOTIFY, PSN_APPLY message for HibernateDlgProc. Updates
*   global hibernate state.
*
*  PARAMETERS:
*
*******************************************************************************/

void DoHibernateApply(void)
{
    NTSTATUS    status;

    // Only handle if hibernate page is dirty.
    if (g_bHibernateDirty) {
        // Fetch data from dialog controls.
        GetControls(g_hwndHibernateDlg,
                    NUM_HIBERNATE_POL_CONTROLS,
                    g_pcHibernatePol);

        status = CallNtPowerInformation(SystemReserveHiberFile,
                                        &g_bHibernate,
                                        sizeof(g_bHibernate),
                                        NULL,
                                        0);
        if (status != STATUS_SUCCESS) {
            ErrorMsgBox(g_hwndHibernateDlg,
#ifdef WINNT
                        RtlNtStatusToDosError(status),
#else
                        NO_ERROR,
#endif
                        IDS_UNABLETOSETHIBER);
        }

        // Get the current hibernate state from the PPM.
        if (GetPwrCapabilities(&g_SysPwrCapabilities)) {
            g_bHibernate = g_SysPwrCapabilities.HiberFilePresent;

            // Map power actions to allowed UI values.
            MapPwrAct(&g_gpp.user.LidCloseDc.Action, TRUE);
            MapPwrAct(&g_gpp.user.PowerButtonDc.Action, FALSE);
            MapPwrAct(&g_gpp.user.SleepButtonDc.Action, FALSE);
        }
        SetControls(g_hwndHibernateDlg, 1, &g_pcHibernatePol[ID_HIBERNATE]);
        UpdateFreeSpace(g_hwndHibernateDlg, g_uiRequiredMB);
        g_bHibernateDirty = FALSE;
    }
}

/*******************************************************************************
*
*  HibernateDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR CALLBACK HibernateDlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
   NMHDR FAR   *lpnm;
   LPTSTR      pszUPS;

   switch (uMsg) {
      case WM_INITDIALOG:

         // Save the hibernate dialog hwnd for use by DoHibernateApply.
         g_hwndHibernateDlg = hWnd;

         // Get the current hibernate state from the PPM.
         if (GetPwrCapabilities(&g_SysPwrCapabilities)) {
            g_bHibernate = g_SysPwrCapabilities.HiberFilePresent;
         }

         // Get the disk free and required space only under NT.
         if (g_bRunningUnderNT) {
            g_dwShowDiskSpace = CONTROL_ENABLE;

            // Get the required space from the power capabilities.
            g_uiRequiredMB = UpdatePhysMem();

            // Update the disk free space and enable/disable
            // disk space warning and hibernate time out.
            UpdateFreeSpace(hWnd, g_uiRequiredMB);

         } else {
            g_dwShowHibernate = CONTROL_ENABLE;
            g_dwShowDiskSpace = CONTROL_HIDE;
            g_dwShowNoDiskSpace = CONTROL_HIDE;
         }

         SetControls(hWnd, NUM_HIBERNATE_POL_CONTROLS, g_pcHibernatePol);
         return TRUE;

      case WM_ACTIVATE:
         // If user switches away, check the disk space when they come back.
         if (g_bRunningUnderNT) {
            GetControls(hWnd, NUM_HIBERNATE_POL_CONTROLS, g_pcHibernatePol);
            UpdateFreeSpace(hWnd, g_uiRequiredMB);
            SetControls(hWnd, NUM_HIBERNATE_POL_CONTROLS-1, g_pcHibernatePol);
         }
         break;

      case WM_NOTIFY:
         lpnm = (NMHDR FAR *)lParam;
         switch (lpnm->code) {
            case PSN_APPLY:
               DoHibernateApply();
               break;
         }
         break;

      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case IDC_HIBERNATE:
               MarkSheetDirty(hWnd, &g_bHibernateDirty);
               break;
         }
         break;

      case PCWM_NOTIFYPOWER:
         // Notification from systray, user has changed a PM UI setting.
         g_bSystrayChange = TRUE;
         break;

      case WM_HELP:             // F1
         WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_HibernateHelpIDs);
         return TRUE;

      case WM_CONTEXTMENU:      // right mouse click
         WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_HibernateHelpIDs);
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
*  SetNumberMB
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID SetNumberMB(LPTSTR psz, DWORD dwValue)
{
    LPTSTR      pszNumber;
    TCHAR       szBuf[128];
    TCHAR       szBufLow[64];

    wsprintf(szBuf, TEXT("%u"), dwValue);
    InsertSeparators(szBuf);
    pszNumber = LoadDynamicString(IDS_MBYTES, szBuf);
    if (pszNumber) {
        lstrcpy(psz, pszNumber);
        LocalFree(pszNumber);
    }
}


/*******************************************************************************
*
*  InsertSeparators
*
*  DESCRIPTION:
*   Passed string must be large enough to hold seperators.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID InsertSeparators(LPTSTR pszNumber)
{
    TCHAR szSeparator[10];
    TCHAR Separator;
    TCHAR szBuf[128];
    ULONG cchNumber;
    UINT  Triples;
    LPTSTR  pch;

    if (GetLocaleInfo(GetUserDefaultLCID(),
                      LOCALE_STHOUSAND,
                      szSeparator,
                      sizeof(szSeparator)/sizeof(TCHAR))) {
        Separator = szSeparator[0];
        cchNumber = lstrlen(pszNumber);
        Triples = 0;
        szBuf[127] = '\0';
        pch = &szBuf[126];
        while (cchNumber > 0) {
            *pch-- = pszNumber[--cchNumber];
            ++Triples;
            if ((0 == (Triples % 3)) && (cchNumber > 0)) {
                *pch-- = Separator;
            }
        }
        lstrcpy(pszNumber, pch + 1);
    }
}

/*******************************************************************************
*
*  UpdateFreeSpace
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

UINT UpdateFreeSpace(HWND hWnd, UINT uiRequiredMB)
{
   DWORD      dwSectorsPerCluster, dwBytesPerSector;
   DWORD      dwFreeClusters, dwTotalClusters;
   ULONGLONG  ullFreeBytes = 0;
   UINT       uiFreeMB;
   TCHAR      szTmp[MAX_PATH];

   // Get the free space on the system drive.
   if (GetSystemDirectory(szTmp, sizeof(szTmp)/sizeof(TCHAR))) {
      szTmp[3] = '\0';
      if (GetDiskFreeSpace(szTmp,
                           &dwSectorsPerCluster,
                           &dwBytesPerSector,
                           &dwFreeClusters,
                           &dwTotalClusters)) {
         ullFreeBytes =  dwBytesPerSector * dwSectorsPerCluster;
         ullFreeBytes *= dwFreeClusters;
         uiFreeMB = (UINT) (ullFreeBytes /= 0x100000);
         SetNumberMB(szTmp, uiFreeMB);
         SetDlgItemText(hWnd, IDC_FREESPACE, szTmp);

         // Logic to enable/disable disk space warning and hibernate time out.
         if ((uiFreeMB >= uiRequiredMB) || g_bHibernate) {
            g_dwShowHibernate   = CONTROL_ENABLE;
            g_dwShowNoDiskSpace = CONTROL_HIDE;
         } else {
            if (g_bHibernate) {
               g_dwShowHibernate   = CONTROL_ENABLE;
            }
            else {
               g_dwShowHibernate   = CONTROL_DISABLE;
            }
            g_dwShowNoDiskSpace = CONTROL_ENABLE;
         }

      }
   }
   return uiFreeMB;
}

/*******************************************************************************
*
*  UpdatePhysMem
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

UINT UpdatePhysMem(void)
{
   UINT           uiPhysMemMB;

#ifdef WINNT
   MEMORYSTATUSEX msex;

   msex.dwLength = sizeof(msex);

   GlobalMemoryStatusEx(&msex);
   uiPhysMemMB = (UINT) (msex.ullTotalPhys / 0x100000);

   if (msex.ullTotalPhys % 0x100000) {
      uiPhysMemMB++;
   }
#else
   MEMORYSTATUS ms;

   ms.dwLength = sizeof(ms);

   GlobalMemoryStatus(&ms);
   uiPhysMemMB = (UINT) (ms.dwTotalPhys / 0x100000);

   if (ms.dwTotalPhys % 0x100000) {
      uiPhysMemMB++;
   }
#endif

   SetNumberMB(g_szRequiredSpace, uiPhysMemMB);
   return uiPhysMemMB;
}

