/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       BATMETER.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*
*   Implements the battery meter of the PowerCfg or SysTray battery
*   meter windows. The battery meter has two display modes, single and
*   multi-battery. In single mode, a representation of the total of all battery
*   capacity in a system is displayed. In multi-battery mode, battery
*   information is displayed for each individual battery as well as the total.
*
*   The battery meter parent window receives notification from USER when
*   any battery status has changed through the WM_POWERBROADCAST,
*   PBT_APMPOWERSTATUSCHANGE message.
*
*   ??? We need to add perfmon support: Create and maintain keys/values
*   under HKEY_PERFORMANCE_DATA.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>

#include <dbt.h>
#include <objbase.h>
#include <initguid.h>
#include <ntpoapi.h>
#include <poclass.h>

#include <setupapi.h>
#include <syssetup.h>
#include <setupbat.h>

#include <help.h>

#include "powrprof.h"
#include "batmeter.h"
#include "bmresid.h"
#include "..\powercfg\PwrMn_cs.h"

// Simulated battery only for debug build.
#ifndef DEBUG
#undef SIM_BATTERY
#endif

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

HINSTANCE   g_hInstance;        // Global instance handle of this DLL.
HWND        g_hwndParent;       // Parent of the battery meter.
HWND        g_hwndBatMeter;     // Battery meter.

// The following constant global array is used to walk through the
// control ID's in the battery metter dialog box. It makes getting
// a control ID from a battery number easy.

#define BAT_ICON      0
#define BAT_STATUS    1
#define BAT_REMAINING 2
#define BAT_NUM       3
#define BAT_LAST      BAT_NUM+1

UINT g_iMapBatNumToID [NUM_BAT+1][4]={
    {IDC_POWERSTATUSICON,  IDC_POWERSTATUSBAR, IDC_REMAINING, IDC_BATNUM0},
    {IDC_POWERSTATUSICON1, IDC_STATUS1, IDC_REMAINING1, IDC_BATNUM1},
    {IDC_POWERSTATUSICON2, IDC_STATUS2, IDC_REMAINING2, IDC_BATNUM2},
    {IDC_POWERSTATUSICON3, IDC_STATUS3, IDC_REMAINING3, IDC_BATNUM3},
    {IDC_POWERSTATUSICON4, IDC_STATUS4, IDC_REMAINING4, IDC_BATNUM4},
    {IDC_POWERSTATUSICON5, IDC_STATUS5, IDC_REMAINING5, IDC_BATNUM5},
    {IDC_POWERSTATUSICON6, IDC_STATUS6, IDC_REMAINING6, IDC_BATNUM6},
    {IDC_POWERSTATUSICON7, IDC_STATUS7, IDC_REMAINING7, IDC_BATNUM7},
    {IDC_POWERSTATUSICON8, IDC_STATUS8, IDC_REMAINING8, IDC_BATNUM8}
};

// Global battery state list. This list has the composite system battery state
// as it's always present head. individual battery devices are linked to this
// head. Use WalkBatteryState(ALL, ... to walk the entire list, including the
// head. Use WalkBatteryState(DEVICES, ... to walk just the device list. If a
// battery is in this list, it's displayable. g_uiBatCount is the count of
// battery devices in this list. The composite battery is not counted. The
// g_pbs array provides a handy UI battery number to pbs conversion. The
// following three variables are only changed during DeviceChanged.

BATTERY_STATE   g_bs;
UINT            g_uiBatCount;
PBATTERY_STATE  g_pbs[NUM_BAT+1];
LPTSTR          g_lpszDriverNames[NUM_BAT];
UINT            g_uiDriverCount;
BOOL            g_bShowingMulti;

// The following array provides context sensitive help associations between
// resource control identifiers and help resource string identifiers.

const DWORD g_ContextMenuHelpIDs[] =
{
    IDC_BATMETERGROUPBOX,       IDH_COMM_GROUPBOX,
    IDC_BATMETERGROUPBOX1,      IDH_COMM_GROUPBOX,
    IDC_POWERSTATUSICON,        NO_HELP,
    IDC_POWERSTATUSICON1,       IDH_BATMETER_CHARGING_ICON,
    IDC_POWERSTATUSICON2,       IDH_BATMETER_CHARGING_ICON,
    IDC_POWERSTATUSICON3,       IDH_BATMETER_CHARGING_ICON,
    IDC_POWERSTATUSICON4,       IDH_BATMETER_CHARGING_ICON,
    IDC_POWERSTATUSICON5,       IDH_BATMETER_CHARGING_ICON,
    IDC_POWERSTATUSICON6,       IDH_BATMETER_CHARGING_ICON,
    IDC_POWERSTATUSICON7,       IDH_BATMETER_CHARGING_ICON,
    IDC_POWERSTATUSICON8,       IDH_BATMETER_CHARGING_ICON,
    IDC_BATNUM1,                NO_HELP,
    IDC_BATNUM2,                NO_HELP,
    IDC_BATNUM3,                NO_HELP,
    IDC_BATNUM4,                NO_HELP,
    IDC_BATNUM5,                NO_HELP,
    IDC_BATNUM6,                NO_HELP,
    IDC_BATNUM7,                NO_HELP,
    IDC_BATNUM8,                NO_HELP,
    IDC_STATUS1,                NO_HELP,
    IDC_STATUS2,                NO_HELP,
    IDC_STATUS3,                NO_HELP,
    IDC_STATUS4,                NO_HELP,
    IDC_STATUS5,                NO_HELP,
    IDC_STATUS6,                NO_HELP,
    IDC_STATUS7,                NO_HELP,
    IDC_STATUS8,                NO_HELP,
    IDC_MOREINFO,               NO_HELP,
    IDC_CURRENTPOWERSOURCE,     IDH_BATMETER_CURPOWERSOURCE,
    IDC_BATTERYLEVEL,           IDH_BATMETER_CURPOWERSOURCE,
    IDC_TOTALBATPWRREMAINING,   IDH_BATMETER_TOTALBATPOWER,
    IDC_REMAINING,              IDH_BATMETER_TOTALBATPOWER,
    IDC_POWERSTATUSBAR,         IDH_BATMETER_TOTALBATPOWER,
    IDC_BARPERCENT,             IDH_BATMETER_TOTALBATPOWER,
    IDC_TOTALTIME,              IDH_BATMETER_TOTALTIME,
    IDC_TIMEREMAINING,          IDH_BATMETER_TOTALTIME,
    IDC_BATTERYNAME,            IDH_DETAILED_BATINFO_LABELS,
    IDC_DEVNAME,                IDH_DETAILED_BATINFO_LABELS,
    IDC_UNIQUEID,               IDH_DETAILED_BATINFO_LABELS,
    IDC_BATID,                  IDH_DETAILED_BATINFO_LABELS,
    IDC_MANUFACTURE,            IDH_DETAILED_BATINFO_LABELS,
    IDC_BATMANNAME,             IDH_DETAILED_BATINFO_LABELS,
    IDC_DATEMANUFACTURED,       IDH_DETAILED_BATINFO_LABELS,
    IDC_BATMANDATE,             IDH_DETAILED_BATINFO_LABELS,
    IDC_CHEMISTRY,              IDH_DETAILED_BATINFO_LABELS,
    IDC_CHEM,                   IDH_DETAILED_BATINFO_LABELS,
    IDC_POWERSTATE,             IDH_DETAILED_BATINFO_LABELS,
    IDC_STATE,                  IDH_DETAILED_BATINFO_LABELS,
    IDC_REFRESH,                IDH_DETAILED_BATINFO_REFRESH,
    0, 0
};

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
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL DllInitialize(IN PVOID hmod, IN ULONG ulReason, IN PCONTEXT pctx OPTIONAL)
{
    UNREFERENCED_PARAMETER(pctx);

    switch (ulReason) {

        case DLL_PROCESS_ATTACH:
            g_hInstance = hmod;
            DisableThreadLibraryCalls(g_hInstance);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

/*******************************************************************************
*
*  PowerCapabilities
*
*  DESCRIPTION:
*   This public function is used to determine if the system has any power
*   management capabilities which require UI support. Return TRUE if power
*   management UI should be displayed.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL PowerCapabilities()
{
    SYSTEM_POWER_CAPABILITIES   spc;
    int   dummy;

#ifndef SIM_BATTERY
    if (GetPwrCapabilities(&spc)) {
        if ((spc.PowerButtonPresent) ||
            (spc.SleepButtonPresent) ||
            (spc.LidPresent) ||
            (spc.SystemS1) ||
            (spc.SystemS2) ||
            (spc.SystemS3) ||
            (spc.SystemS4) ||
            (spc.SystemS5) ||
            (spc.DiskSpinDown) ||
            (spc.SystemBatteriesPresent)) {
            return TRUE;
        }
        else {
            if (SystemParametersInfo(SPI_GETLOWPOWERACTIVE, 0, &dummy, 0)) {
                return TRUE;
            }
        }
    }
    return FALSE;
#else
    return TRUE;
#endif
}

/*******************************************************************************
*
*  BatMeterCapabilities
*
*  DESCRIPTION:
*   This public function is used to determine if the battery meter library
*   can run on the host machine. Return TRUE on success (battery meter can run).
*
*  PARAMETERS:
*   ppuiBatCount - Points to a pointer which will be filled in with a pointer
*                  to the global battery count.
*
*******************************************************************************/

BOOL BatMeterCapabilities(
    PUINT   *ppuiBatCount
)
{
    UINT                        i;
    PBATTERY_STATE              pbs;
    SYSTEM_POWER_CAPABILITIES   spc;

    if (ppuiBatCount) {
        *ppuiBatCount = &g_uiBatCount;
    }
    g_uiBatCount = 0;

#ifndef SIM_BATTERY
    // Make sure we have batteries to query.
    if (GetPwrCapabilities(&spc)) {
        if (spc.SystemBatteriesPresent) {
            g_uiDriverCount = GetBatteryDriverNames(g_lpszDriverNames);
            if (g_uiDriverCount != 0) {
                g_uiBatCount = g_uiDriverCount;

                return TRUE;
            }
            else {
                DebugPrint( "BatMeterCapabilities, no battery drivers found.");
            }
        }
    }
    return FALSE;

#else
    g_uiBatCount = g_uiDriverCount = GetBatteryDriverNames(g_lpszDriverNames);
    return UpdateDriverList(g_lpszDriverNames, g_uiDriverCount);
#endif

}

/*******************************************************************************
*
*  CreateBatMeter
*
*  DESCRIPTION:
*   Create, fetch data for and draw the battery meter window. Returns a handle
*   to the newly created battery meter window on success, NULL on failure.
*
*  PARAMETERS:
*   hwndParent      - Parent of the battery meter dialog.
*   wndFrame        - Frame to locate the battery meter dialog.
*   bShowMulti      - Specifies the display mode (TRUE -> multiple battery).
*   pbsComposite    - Optional pointer to composite battery state.
*
*******************************************************************************/

HWND CreateBatMeter(
    HWND            hwndParent,
    HWND            hwndFrame,
    BOOL            bShowMulti,
    PBATTERY_STATE  pbsComposite
)
{
   RECT                    rFrame;
   INT                     iWidth, iHeight;
   SYSTEM_POWER_STATUS     sps;

   // Build the battery devices name list if hasn't already been built.
   if (!g_uiBatCount) {
      BatMeterCapabilities(NULL);
   }

   // Remember if we are showing details for each battery
   g_bShowingMulti = bShowMulti;

   // Make sure we have at least one battery.
   if (g_uiBatCount) {

      // Create the battery meter control.
      g_hwndParent = hwndParent;
      g_hwndBatMeter = CreateDialog(g_hInstance,
                                    MAKEINTRESOURCE(IDD_BATMETER),
                                    hwndParent,
                                    BatMeterDlgProc);

      // Place the battery meter in the passed frame window.
      if ((g_hwndBatMeter) && (hwndFrame)) {

         // Position the BatMeter dialog in the frame.
         memset(&rFrame, 0, sizeof(rFrame));
         if (!GetWindowRect(hwndFrame, &rFrame)) {
            DebugPrint( "CreateBatMeter, GetWindowRect failed, hwndFrame: %08X", hwndFrame);
         }
         iWidth  = rFrame.right  - rFrame.left;
         iHeight = rFrame.bottom - rFrame.top;
         if (!ScreenToClient(hwndParent, (LPPOINT)&rFrame)) {
            DebugPrint( "CreateBatMeter, ScreenToClient failed");
         }
         if (!MoveWindow(g_hwndBatMeter,
                         rFrame.left,
                         rFrame.top,
                         iWidth,
                         iHeight,
                         FALSE)) {
            DebugPrint( "CreateBatMeter, MoveWindow failed, %d, %d", rFrame.left, rFrame.top);
         }

         // Build the battery driver data list.
         if (!UpdateDriverList(g_lpszDriverNames, g_uiDriverCount)) {
            return DestroyBatMeter(g_hwndBatMeter);
         }

         // Do the first update.
         UpdateBatMeter(g_hwndBatMeter, bShowMulti, TRUE, pbsComposite);
         ShowWindow(g_hwndBatMeter, SW_SHOWNOACTIVATE);
      }
   }
   return g_hwndBatMeter;
}

/*******************************************************************************
*
*  DestroyBatMeter
*
*  DESCRIPTION:
*
*******************************************************************************/

HWND DestroyBatMeter(HWND hWnd)
{
   SendMessage(hWnd, WM_DESTROYBATMETER, 0, 0);
   g_hwndBatMeter = NULL;
   return g_hwndBatMeter;
}

/*******************************************************************************
*
*  UpdateBatMeter
*
*  DESCRIPTION:
*   This function should be called when the battery meter parent window
*   receives a WM_POWERBROADCAST, PBT_APMPOWERSTATUSCHANGE message, it will
*   update the data in the global battery state list. If needed the display
*   will also be updated.
*
*  PARAMETERS:
*   HWND hwndBatMeter,          hWnd of the battery meter dialog
*   BOOL bShowMulti,            Specifies the display mode
*   BOOL bForceUpdate,          Forces a UI update
*   PBATTERY_STATE pbsComposite Optional pointer to composite battery state.
*
*******************************************************************************/

BOOL UpdateBatMeter(
    HWND            hWnd,
    BOOL            bShowMulti,
    BOOL            bForceUpdate,
    PBATTERY_STATE  pbsComposite
)
{
    BOOL bRet = FALSE;
    SYSTEM_POWER_STATUS sps;
    UINT uIconID;

    // Update the composite battery state.
    if (GetSystemPowerStatus(&sps) && hWnd) {
        if (sps.BatteryLifePercent > 100) {
            DebugPrint( "GetSystemPowerStatuse, set BatteryLifePercent: %d", sps.BatteryLifePercent);
        }

        // Fill in the composite battery state.
        SystemPowerStatusToBatteryState(&sps, &g_bs);

        // Update the information in the battery state list if we have a battery.
        if (g_hwndBatMeter) {

#ifndef SIM_BATTERY
           WalkBatteryState(DEVICES,
                            (WALKENUMPROC)UpdateBatInfoProc,
                            NULL,
                            (LPARAM)NULL,
                            (LPARAM)NULL);
#else
           WalkBatteryState(DEVICES,
                            (WALKENUMPROC)SimUpdateBatInfoProc,
                            NULL,
                            (LPARAM)NULL,
                            (LPARAM)NULL);
#endif

           // See if the current display mode matches the requested mode.
           if ((g_bShowingMulti != bShowMulti) || (bForceUpdate)) {
               g_bShowingMulti = SwitchDisplayMode(hWnd, bShowMulti);
               bForceUpdate  = TRUE;
           }

           if (g_bShowingMulti) {
               // Walk the bs list, and update all battery displays.
               WalkBatteryState(ALL,
                                (WALKENUMPROC)UpdateBatMeterProc,
                                hWnd,
                                (LPARAM)g_bShowingMulti,
                                (LPARAM)bForceUpdate);
           }
           else {
               // Display only the comosite battery information.
               UpdateBatMeterProc(&g_bs,
                                  hWnd,
                                  (LPARAM)g_bShowingMulti,
                                  (LPARAM)bForceUpdate);
           }
           bRet = TRUE;
        }
    }
    else {
        // Fill in default composite info.
        g_bs.ulPowerState     = BATTERY_POWER_ON_LINE;
        g_bs.ulBatLifePercent = (UINT) -1;
        g_bs.ulBatLifeTime    = (UINT) -1;

        uIconID = MapBatInfoToIconID(&g_bs);
        g_bs.hIconCache = GetBattIcon(hWnd, uIconID, g_bs.hIconCache, FALSE, 32);
        g_bs.hIconCache16 = GetBattIcon(hWnd, uIconID, g_bs.hIconCache16, FALSE, 16);
    }

    // If a pointer is provided, copy the composite battery state data.
    if (pbsComposite) {
        if (pbsComposite->ulSize == sizeof(BATTERY_STATE)) {
            memcpy(pbsComposite, &g_bs, sizeof(BATTERY_STATE));
        }
        else {
            DebugPrint( "UpdateBatMeter, passed BATTERY_STATE size is invalid");
        }
    }
    return bRet;
}

/*******************************************************************************
*
*                 P R I V A T E   F U N C T I O N S
*
*******************************************************************************/

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
*     uiStringID    - resource identifier of the string to use.
*     ...           - Optional parameters to use to format the string message.
*
*******************************************************************************/

LPTSTR CDECL LoadDynamicString(UINT uiStringID, ... )
{
    va_list Marker;
    TCHAR szBuf[256];
    LPTSTR lpsz;
    int   iLen;

    // va_start is a macro...it breaks when you use it as an assign...on ALPHA.
    va_start(Marker, uiStringID);

    iLen = LoadString(g_hInstance, uiStringID, szBuf, sizeof(szBuf));

    if (iLen == 0) {
        DebugPrint( "LoadDynamicString: LoadString on: 0x%X failed", uiStringID);
        return NULL;
    }

    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                  (LPVOID) szBuf, 0, 0, (LPTSTR)&lpsz, 0, &Marker);

    return lpsz;
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

LPTSTR DisplayFreeStr(HWND hWnd, UINT uID, LPTSTR  lpsz, BOOL bFree)
{
    if (lpsz) {
        SetDlgItemText(hWnd, uID, lpsz);
        ShowWindow(GetDlgItem(hWnd, uID), SW_SHOWNOACTIVATE);
        if (bFree) {
            LocalFree(lpsz);
            return NULL;
        }
    }
    else {
        ShowWindow(GetDlgItem(hWnd, uID), SW_HIDE);
    }
    return lpsz;
}

/*******************************************************************************
*
*  ShowHideItem
*  ShowItem
*  HideItem
*
*  DESCRIPTION:
*     Handy helpers to show or hide dialog items in the battery meter dialog.
*
*  PARAMETERS:
*     hWnd - Battery meter dialog handle.
*     uID  - Control ID of control to be shown or hidden.
*
*******************************************************************************/

BOOL ShowHideItem(HWND hWnd, UINT uID, BOOL bShow)
{
    ShowWindow(GetDlgItem(hWnd, uID), (bShow)  ? SW_SHOWNOACTIVATE : SW_HIDE);
    return bShow;
}

void ShowItem(HWND hWnd, UINT uID)
{
    ShowWindow(GetDlgItem(hWnd, uID), SW_SHOWNOACTIVATE);
}

void HideItem(HWND hWnd, UINT uID)
{
    ShowWindow(GetDlgItem(hWnd, uID), SW_HIDE);
}

/*******************************************************************************
*
*  SwitchDisplayMode
*
*  DESCRIPTION:
*   Return TRUE if display is switched to multi battery mode.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL SwitchDisplayMode(HWND hWnd, BOOL bShowMulti)
{
    ULONG i, j;

    // Override request if multi-battery display is not possible.
    if ((bShowMulti) && (!g_uiBatCount)) {
        bShowMulti = FALSE;
    }

    if (!g_uiBatCount) {

        //
        // Hide all info if no batteries are installed
        //
        HideItem(hWnd, IDC_POWERSTATUSBAR);
        HideItem(hWnd, IDC_BARPERCENT);
        HideItem(hWnd, IDC_MOREINFO);

    } else if (bShowMulti) {
        HideItem(hWnd, IDC_POWERSTATUSBAR);
        HideItem(hWnd, IDC_BARPERCENT);
        ShowItem(hWnd, IDC_MOREINFO);

        for (i = 1; i <= g_uiBatCount; i++) {
            for (j = 0; j < BAT_LAST; j++) {
                ShowItem(hWnd, g_iMapBatNumToID[i][0]);
            }
        }
    }
    else {
        for (i = 1; i <= g_uiBatCount; i++) {
            for (j = 0; j < BAT_LAST; j++) {
                HideItem(hWnd, g_iMapBatNumToID[i][j]);
            }
        }

        ShowItem(hWnd, IDC_POWERSTATUSBAR);
        ShowItem(hWnd, IDC_BARPERCENT);
        HideItem(hWnd, IDC_MOREINFO);
    }
    return bShowMulti;
}

/*******************************************************************************
*
*  CleanupBatteryData
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void CleanupBatteryData(void)
{
   g_hwndBatMeter = NULL;

   // Mark all batteries as missing.
   memset(&g_pbs, 0, sizeof(g_pbs));

   // Walk the bs list, remove all devices and cleanup.
   WalkBatteryState(DEVICES,
                    (WALKENUMPROC)RemoveMissingProc,
                    NULL,
                    (LPARAM)NULL,
                    (LPARAM)REMOVE_ALL);

   // Free any old driver names.
   FreeBatteryDriverNames(g_lpszDriverNames);
   g_uiBatCount = 0;
}

/*******************************************************************************
*
*  BatMeterDlgProc
*
*  DESCRIPTION:
*   DialogProc for the Battery Meter control. Provide support for more battery
*   info.
*
*  PARAMETERS:
*
*******************************************************************************/

LRESULT CALLBACK BatMeterDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   UINT    uiBatNum, i, j;
   PBATTERY_STATE pbsTemp;

   switch (uMsg) {
      case WM_COMMAND:
         if ((HIWORD(wParam) == STN_CLICKED) ||
             (HIWORD(wParam) == BN_CLICKED)) {
            switch (LOWORD(wParam)) {
               case IDC_POWERSTATUSICON1:
               case IDC_POWERSTATUSICON2:
               case IDC_POWERSTATUSICON3:
               case IDC_POWERSTATUSICON4:
               case IDC_POWERSTATUSICON5:
               case IDC_POWERSTATUSICON6:
               case IDC_POWERSTATUSICON7:
               case IDC_POWERSTATUSICON8:
                  uiBatNum = LOWORD(wParam) - IDC_POWERSTATUSICON1 + 1;
                  // Allow battery details only for present batteries.
                  if ((g_pbs[uiBatNum]) &&
                      (g_pbs[uiBatNum]->ulTag != BATTERY_TAG_INVALID)) {
                     DialogBoxParam(g_hInstance,
                                    MAKEINTRESOURCE(IDD_BATDETAIL),
                                    hWnd,
                                    BatDetailDlgProc,
                                    (LPARAM)g_pbs[uiBatNum]);
                  }
                  break;
            }
         }
         break;

      case WM_DESTROYBATMETER:
         CleanupBatteryData();
         EndDialog(hWnd, wParam);
         break;

      case WM_DESTROY:
         CleanupBatteryData();
         break;

      case WM_DEVICECHANGE:
#ifdef WINNT
         if ((wParam == DBT_DEVICEQUERYREMOVE) || (wParam == DBT_DEVICEREMOVECOMPLETE)) {
            if ( ((PDEV_BROADCAST_HANDLE)lParam)->dbch_devicetype == DBT_DEVTYP_HANDLE) {

               //
               // Find Device that got removed
               //
               pbsTemp = DEVICES;
               while (pbsTemp) {
                  if (pbsTemp->hDevNotify == ((PDEV_BROADCAST_HANDLE)lParam)->dbch_hdevnotify) {
                     break;
                  }
                  pbsTemp = pbsTemp->bsNext;
               }
               if (!pbsTemp) {
                  break;
               }

               //
               // Close the handle to this device and release cached data.
               //
               RemoveBatteryStateDevice (pbsTemp);
               g_uiDriverCount--;
               g_uiBatCount = g_uiDriverCount;

               // Clear and rebuild g_pbs, the handy batttery number to pbs array.
               memset(&g_pbs, 0, sizeof(g_pbs));
               pbsTemp = &g_bs;
               for (i = 0; i <= g_uiBatCount; i++) {
                  if (pbsTemp) {
                     g_pbs[i] = pbsTemp;
                     pbsTemp->ulBatNum = i;
                     pbsTemp = pbsTemp->bsNext;
                  }
               }

               // Refresh display
               for (i = 1; i <= NUM_BAT; i++) {
                  for (j = 0; j < BAT_LAST; j++) {
                     HideItem(g_hwndBatMeter, g_iMapBatNumToID[i][j]);
                  }
               }

               g_bShowingMulti = SwitchDisplayMode (g_hwndBatMeter, g_bShowingMulti);
               if (g_bShowingMulti) {
                  // Walk the bs list, and update all battery displays.
                  WalkBatteryState(DEVICES,
                                   (WALKENUMPROC)UpdateBatMeterProc,
                                   g_hwndBatMeter,
                                   (LPARAM)g_bShowingMulti,
                                   (LPARAM)TRUE);
               }
            }
         }
#else
         if (wParam == DBT_DEVICEQUERYREMOVE) {
            if (g_hwndBatMeter) {
               // Close all of the batteries.
               CleanupBatteryData();
            }
         }
#endif
         return TRUE;

      case WM_HELP:             // F1
         WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_ContextMenuHelpIDs);
         return TRUE;

      case WM_CONTEXTMENU:      // right mouse click
         WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_ContextMenuHelpIDs);
         return TRUE;
   }
   return FALSE;
}

/*******************************************************************************
*
*  GetBattIcon
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

HICON PASCAL GetBattIcon(
    HWND    hWnd,
    UINT    uIconID,
    HICON   hIconCache,
    BOOL    bWantBolt,
    UINT    uiRes)
{
    static HIMAGELIST hImgLst32, hImgLst16;
    HIMAGELIST hImgLst;
    int ImageIndex;

    // Destroy the old cached icon.
    if (hIconCache) {
        DestroyIcon(hIconCache);
    }

    // Don't put the charging bolt over the top of IDI_BATGONE.
    if (uIconID == IDI_BATGONE) {
        bWantBolt = FALSE;
    }

    // Use the transparency color must match that in the bit maps.
    if (!hImgLst32 || !hImgLst16) {
        hImgLst32 = ImageList_LoadImage(g_hInstance,
                                        MAKEINTRESOURCE(IDB_BATTS),
                                        32, 0, RGB(255, 0, 255), IMAGE_BITMAP, 0);
        hImgLst16 = ImageList_LoadImage(g_hInstance,
                                        MAKEINTRESOURCE(IDB_BATTS16),
                                        16, 0, RGB(255, 0, 255), IMAGE_BITMAP, 0);
        ImageList_SetOverlayImage(hImgLst32, IDI_CHARGE-FIRST_ICON_IMAGE, 1);
        ImageList_SetOverlayImage(hImgLst16, IDI_CHARGE-FIRST_ICON_IMAGE, 1);
    }

    if (uiRes == 32) {
        hImgLst = hImgLst32;
    }
    else {
        hImgLst = hImgLst16;
    }

    ImageIndex = uIconID - FIRST_ICON_IMAGE;

    if (bWantBolt) {
        return ImageList_GetIcon(hImgLst, ImageIndex, INDEXTOOVERLAYMASK(1));
    }
    else {
        return ImageList_GetIcon(hImgLst, ImageIndex, ILD_NORMAL);
    }
}

/*******************************************************************************
*
*  CheckUpdateBatteryState
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

#define UPDATESTATUS_NOUPDATE        0
#define UPDATESTATUS_UPDATE          1
#define UPDATESTATUS_UPDATE_CHARGE   2

UINT CheckUpdateBatteryState(
    PBATTERY_STATE   pbs,
    BOOL             bForceUpdate
)
{
    UINT uiRetVal = UPDATESTATUS_NOUPDATE;

    // Check to see if anything in the battery status has changed
    // since last time.  If not then we have no work to do!

    if ((bForceUpdate) ||
        !((pbs->ulTag            == pbs->ulLastTag) &&
          (pbs->ulBatLifePercent == pbs->ulLastBatLifePercent) &&
          (pbs->ulBatLifeTime    == pbs->ulLastBatLifeTime) &&
          (pbs->ulPowerState     == pbs->ulLastPowerState))) {

        uiRetVal = UPDATESTATUS_UPDATE;

        //  Check for the special case where the charging state has changed.
        if ((pbs->ulPowerState     & BATTERY_CHARGING) !=
            (pbs->ulLastPowerState & BATTERY_CHARGING)) {
                uiRetVal |= UPDATESTATUS_UPDATE_CHARGE;
        }

        // Copy current battery state to last.
        pbs->ulLastTag            = pbs->ulTag;
        pbs->ulLastBatLifePercent = pbs->ulBatLifePercent;
        pbs->ulLastBatLifeTime    = pbs->ulBatLifeTime;
        pbs->ulLastPowerState     = pbs->ulPowerState;
    }
    return uiRetVal;
}

/*******************************************************************************
*
*  MapBatInfoToIconID
*
*  DESCRIPTION:
*    Map battery info to an Icon ID.
*
*  PARAMETERS:
*    ulBatNum - Zero implies composite system state
*
*******************************************************************************/

UINT MapBatInfoToIconID(PBATTERY_STATE pbs)
{
    UINT uIconID = IDI_BATDEAD;

    if (!pbs->ulBatNum) {
        if (pbs->ulPowerState & BATTERY_POWER_ON_LINE) {
            return IDI_PLUG;
        }
    }
    else {
        if (pbs->ulTag == BATTERY_TAG_INVALID) {
            return IDI_BATGONE;
        }
    }

    if  (pbs->ulPowerState & BATTERY_CRITICAL) {
        return IDI_BATDEAD;
    }

    if (pbs->ulBatLifePercent > 66) {
        uIconID = IDI_BATFULL;
    }
    else {
        if (pbs->ulBatLifePercent > 33) {
            uIconID = IDI_BATHALF;
        }
        else {
            if (pbs->ulBatLifePercent > 9) {
                uIconID = IDI_BATLOW;
            }
        }
    }

    return uIconID;
}

/*******************************************************************************
*
*  DisplayIcon
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

void DisplayIcon(
    HWND            hWnd,
    UINT            uIconID,
    PBATTERY_STATE  pbs,
    ULONG           ulUpdateStatus
)
{
    BOOL    bBolt;
    UINT    uiMsg;

    // Only redraw the icon if it has changed OR
    // if it has gone from charging to not charging.
    if ((uIconID != pbs->uiIconIDcache) ||
        (ulUpdateStatus != UPDATESTATUS_NOUPDATE)) {

        pbs->uiIconIDcache = uIconID;
        bBolt = (pbs->ulPowerState & BATTERY_CHARGING);

        pbs->hIconCache   = GetBattIcon(hWnd, uIconID, pbs->hIconCache, bBolt, 32);
        pbs->hIconCache16 = GetBattIcon(hWnd, uIconID, pbs->hIconCache16, bBolt, 16);

        if (pbs->ulBatNum) {
            uiMsg = BM_SETIMAGE;
        }
        else {
            uiMsg = STM_SETIMAGE;
        }
        SendDlgItemMessage(hWnd, g_iMapBatNumToID[pbs->ulBatNum][BAT_ICON],
                           uiMsg, IMAGE_ICON, (LPARAM) pbs->hIconCache);
        ShowItem(hWnd, g_iMapBatNumToID[pbs->ulBatNum][BAT_ICON]);
    }
}

/*******************************************************************************
*
*  UpdateBatMeterProc
*
*  DESCRIPTION:
*    Updates the System and per battery UI elements if needed.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL UpdateBatMeterProc(
    PBATTERY_STATE pbs,
    HWND hWnd,
    LPARAM bShowMulti,
    LPARAM bForceUpdate
)
{
    UINT   uIconID, uiHour, uiMin;
    LPTSTR lpsz, lpszRemaining;
    ULONG  ulUpdateStatus;

    ulUpdateStatus = CheckUpdateBatteryState(pbs, (BOOL) bForceUpdate);

    // Make sure there is work to do.
    if (ulUpdateStatus == UPDATESTATUS_NOUPDATE) {
       return TRUE;
    }

    // Determine which icon to display.
    uIconID = MapBatInfoToIconID(pbs);
    DisplayIcon(hWnd, uIconID, pbs, ulUpdateStatus);

    // Are we looking for system power status ?
    if (!pbs->ulBatNum) {

        // Display the Current Power Source text
        lpsz = LoadDynamicString(((pbs->ulPowerState & BATTERY_POWER_ON_LINE) ?
                                   IDS_ACLINEONLINE : IDS_BATTERIES));
        DisplayFreeStr(hWnd, IDC_BATTERYLEVEL, lpsz, FREE_STR);

        if (pbs->ulBatLifePercent <= 100) {
            lpsz = LoadDynamicString(IDS_PERCENTREMAININGFORMAT,
                                        pbs->ulBatLifePercent);
        }
        else {
            lpsz = LoadDynamicString(IDS_UNKNOWN);
        }
        DisplayFreeStr(hWnd, IDC_REMAINING, lpsz, NO_FREE_STR);

        ShowHideItem(hWnd, IDC_CHARGING, pbs->ulPowerState & BATTERY_CHARGING);

        // Show and Update the PowerStatusBar only if in single battery mode and
        // there is al least one battery installed.
        if (!bShowMulti && g_uiBatCount) {
            SendDlgItemMessage(hWnd, IDC_POWERSTATUSBAR, PBM_SETPOS,
                               (WPARAM) pbs->ulBatLifePercent, 0);
            lpsz = DisplayFreeStr(hWnd, IDC_BARPERCENT, lpsz, FREE_STR);
        }

        if (lpsz) {
            LocalFree(lpsz);
        }

        if (pbs->ulBatLifeTime != (UINT) -1) {
            uiHour = pbs->ulBatLifeTime / 3600;
            uiMin  = (pbs->ulBatLifeTime % 3600) / 60;
            if (uiHour) {
                lpsz = LoadDynamicString(IDS_TIMEREMFORMATHOUR, uiHour, uiMin);
            }
            else {
                lpsz = LoadDynamicString(IDS_TIMEREMFORMATMIN, uiMin);
            }
            DisplayFreeStr(hWnd, IDC_TIMEREMAINING, lpsz, FREE_STR);
            ShowHideItem(hWnd, IDC_TOTALTIME, TRUE);
        }
        else {
            ShowHideItem(hWnd, IDC_TOTALTIME, FALSE);
            ShowHideItem(hWnd, IDC_TIMEREMAINING, FALSE);
        }
    }
    else {

        // Here when getting the power status of each individual battery
        // when in multi-battery display mode.
        lpsz = LoadDynamicString(IDS_BATNUM, pbs->ulBatNum);
        DisplayFreeStr(hWnd, g_iMapBatNumToID[pbs->ulBatNum][BAT_NUM],
                       lpsz, FREE_STR);

        if (pbs->ulTag != BATTERY_TAG_INVALID) {
            if (pbs->ulPowerState & BATTERY_CHARGING) {
                lpsz = LoadDynamicString(IDS_BATTCHARGING);
            }
            else {
                lpsz = NULL;
            }
            lpszRemaining  = LoadDynamicString(IDS_PERCENTREMAININGFORMAT,
                                               pbs->ulBatLifePercent);
        }
        else {
            lpsz = LoadDynamicString(IDS_NOT_PRESENT);
            lpszRemaining  = NULL;
        }
        DisplayFreeStr(hWnd, g_iMapBatNumToID[pbs->ulBatNum][BAT_STATUS],
                       lpsz, FREE_STR);

        DisplayFreeStr(hWnd, g_iMapBatNumToID[pbs->ulBatNum][BAT_REMAINING],
                       lpszRemaining, FREE_STR);
    }
    return TRUE;
}

/*******************************************************************************
*
*  FreeBatteryDriverNames
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID FreeBatteryDriverNames(LPTSTR *lpszDriverNames)
{
    UINT i;

    // Free any old driver names.
    for (i = 0; i < NUM_BAT; i++) {
        if (lpszDriverNames[i]) {
            LocalFree(lpszDriverNames[i]);
            lpszDriverNames[i] = NULL;
        }
    }
}

/*******************************************************************************
*
*  GetBatteryDriverNames
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetBatteryDriverNames(LPTSTR *lpszDriverNames)
{
    UINT                                uiDriverCount, uiIndex;
    DWORD                               dwReqSize;
    HDEVINFO                            hDevInfo;
    SP_INTERFACE_DEVICE_DATA            InterfaceDevData;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    pFuncClassDevData;

    // Free any old driver names.
    FreeBatteryDriverNames(lpszDriverNames);
    uiDriverCount = 0;

#ifndef SIM_BATTERY
    // Use the SETUPAPI.DLL interface to get the
    // possible battery driver names.
    hDevInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVICE_BATTERY, NULL, NULL,
                                   DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

    if (hDevInfo != INVALID_HANDLE_VALUE) {
        InterfaceDevData.cbSize = sizeof(SP_DEVINFO_DATA);

        uiIndex = 0;
        while (uiDriverCount < NUM_BAT) {
            if (SetupDiEnumInterfaceDevice(hDevInfo,
                                           0,
                                           (LPGUID)&GUID_DEVICE_BATTERY,
                                           uiIndex,
                                           &InterfaceDevData)) {

                // Get the required size of the function class device data.
                SetupDiGetInterfaceDeviceDetail(hDevInfo,
                                                &InterfaceDevData,
                                                NULL,
                                                0,
                                                &dwReqSize,
                                                NULL);

                pFuncClassDevData = LocalAlloc(0, dwReqSize);
                if (pFuncClassDevData != NULL) {
                    pFuncClassDevData->cbSize =
                        sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

                    if (SetupDiGetInterfaceDeviceDetail(hDevInfo,
                                                        &InterfaceDevData,
                                                        pFuncClassDevData,
                                                        dwReqSize,
                                                        &dwReqSize,
                                                        NULL)) {

                        dwReqSize = (lstrlen(pFuncClassDevData->DevicePath) + 1) * sizeof(TCHAR);
                        lpszDriverNames[uiDriverCount] = LocalAlloc(0, dwReqSize);

                        if (lpszDriverNames[uiDriverCount]) {
                            lstrcpy(lpszDriverNames[uiDriverCount],
                                    pFuncClassDevData->DevicePath);
                            uiDriverCount++;
                        }
                    }
                    else {
                        DebugPrint("SetupDiGetInterfaceDeviceDetail, failed: %d", GetLastError());
                    }

                    LocalFree(pFuncClassDevData);
                }
            } else {
                if (ERROR_NO_MORE_ITEMS == GetLastError()) {
                    break;
                }
                else {
                    DebugPrint("SetupDiEnumInterfaceDevice, failed: %d", GetLastError());
                }
            }
            uiIndex++;
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    else {
        DebugPrint("SetupDiGetClassDevs on GUID_DEVICE_BATTERY, failed: %d", GetLastError());
    }
#else
   // Simulate batteries.
   {
      UINT i;
      static UINT uiState = 1;

      for (i = 0; i <= uiState; i++) {
         lpszDriverNames[i] = LocalAlloc(0, STRSIZE(TEXT("SIMULATED_BATTERY_0")));
         wsprintf(lpszDriverNames[i], TEXT("SIMULATED_BATTERY_%d"), i);
      }
      uiState++;
      uiDriverCount = uiState;
      if (uiState >= NUM_BAT) {
         uiState = 0;
      }
   }
#endif
    return uiDriverCount;
}

/*******************************************************************************
*
*  UpdateDriverList
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL UpdateDriverList(
    LPTSTR *lpszDriverNames,
    UINT uiDriverCount
)
{
    UINT            i;
    PBATTERY_STATE  pbs;

    // Walk the bs list, and remove any devices which aren't in pszDeviceNames.
    WalkBatteryState(DEVICES,
                     (WALKENUMPROC)RemoveMissingProc,
                     NULL,
                     (LPARAM)g_lpszDriverNames,
                     (LPARAM)REMOVE_MISSING);

    // Scan the pszDeviceNames list and add any devices which aren't in bs.
    for (i = 0; i < uiDriverCount; i++) {

        if (WalkBatteryState(DEVICES,
                             (WALKENUMPROC)FindNameProc,
                             NULL,
                             (LPARAM)g_lpszDriverNames[i],
                             (LPARAM)NULL)) {

#ifndef SIM_BATTERY
            if (!AddBatteryStateDevice(g_lpszDriverNames[i], i + 1)) {
                // We weren't able get minimal info from driver, dec the
                // battery counts. g_uiBatCount should always be > 0.
                if (--g_uiDriverCount) {;
                    g_uiBatCount--;
                }
            }
#else
            SimAddBatteryStateDevice(g_lpszDriverNames[i], i + 1);
#endif
        }
    }

    // Clear and rebuild g_pbs, the handy batttery number to pbs array.
    memset(&g_pbs, 0, sizeof(g_pbs));
    pbs = &g_bs;
    for (i = 0; i <= g_uiBatCount; i++) {
        if (pbs) {
            g_pbs[i] = pbs;
            pbs = pbs->bsNext;
        }
    }
    return TRUE;
}

