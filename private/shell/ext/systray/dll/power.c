/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*
*  TITLE:       POWER.C
*
*  VERSION:     2.0
*
*  AUTHOR:      TCS/RAL
*
*  DATE:        08 Feb 1994
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  08 Feb 1994 TCS Original implementation.
*  11 Nov 1994 RAL Converted from batmeter to systray
*  11 Aug 1995 JEM Split batmeter functions into power.c & minor enahncements
*  23 Oct 1995 Shawnb UNICODE Enabled
*  24 Jan 1997 Reedb ACPI power management, common battery meter code.
*
*******************************************************************************/

#include "stdafx.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <compobj.h>
#include <initguid.h>
#include <ntpoapi.h>
#include <poclass.h>

#include "systray.h"

#include "batmeter.h"
#include "powrprof.h"
#include "powercfp.h"

#define UPDATE_REGISTRY TRUE
#define NO_REGISTRY_UPDATE FALSE

// Structure to manage the power profile enum proc parameters.
typedef struct _POWER_PROFILE_ENUM_PROC_PARAMS
{
    UINT    uiCurActiveIndex;
    HMENU   hMenu;
    UINT    uiCurActiveID;
} POWER_PROFILE_ENUM_PROC_PARAMS, *PPOWER_PROFILE_ENUM_PROC_PARAMS;


// G L O B A L  D A T A -------------------------------------------------------
BOOL    g_bPowerEnabled;      // Tracks the power service state.
UINT    g_uiPowerSchemeCount; // Number of power schemes, left context menu.
HMENU   g_hMenu[2];           // Context menus.

// BatMeter creation parameters.
HWND    g_hwndBatMeter;
BOOL    g_bShowMulti;
HWND    g_hwndBatMeterFrame;

GLOBAL_POWER_POLICY g_gpp;

// Context sensitive help must be added to the windows.hlp file,
// for now we will use this dummy array define. Remove when windows.hlp updated.

#define IDH_POWERCFG_ENABLEMULTI IDH_POWERCFG_POWERSTATUSBAR

const DWORD g_ContextMenuHelpIDs[] = {
    IDC_POWERSTATUSGROUPBOX,    IDH_COMM_GROUPBOX,
    IDC_ENABLEMETER,            IDH_POWERCFG_ENABLEMETER,
    IDC_ENABLEMULTI,            IDH_POWERCFG_ENABLEMULTI,
    0, 0
};

// Used to track registration for WM_DEVICECHANGED message.
HDEVNOTIFY g_hDevNotify;

/*******************************************************************************
*
*  RunningOffLine
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN RunningOffLine(void)
{
   SYSTEM_POWER_STATUS  sps;
   BOOLEAN              bRet = FALSE;

   if (GetSystemPowerStatus(&sps)) {
      if (sps.ACLineStatus == 0) {
         bRet = TRUE;
      }
   }
   return bRet;
}

/*----------------------------------------------------------------------------
 * Power_OnCommand
 *
 * Process WM_COMMAND msgs for the battery meter dialog.
 *
 *----------------------------------------------------------------------------*/

void
Power_OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    BOOL  Checked;
    DWORD dwMask;
    UINT  uiCommandID = GET_WM_COMMAND_ID(wParam, lParam);

    switch (uiCommandID) {

        case IDC_ENABLEMETER:
            dwMask = EnableSysTrayBatteryMeter;
            goto DoUpdateFlags;

        case IDC_ENABLEMULTI:
            dwMask = EnableMultiBatteryDisplay;
            goto DoUpdateFlags;

DoUpdateFlags:
            Checked = (IsDlgButtonChecked(hWnd, uiCommandID) == BST_CHECKED);
            Update_PowerFlags(dwMask, Checked);
            if (uiCommandID == IDC_ENABLEMETER) {
                PowerCfg_Notify();
                SysTray_EnableService(STSERVICE_POWER, g_gpp.user.GlobalFlags & EnableSysTrayBatteryMeter);
            }
            else {
                g_bShowMulti = Checked;
                Power_UpdateStatus(hWnd, NIM_MODIFY, TRUE);
            }
            break;

        case IDCANCEL:
            EndDialog(hWnd, wParam);
            break;

        default:
            // Notify battery meter of enter key events.
            if (HIWORD(wParam) == BN_CLICKED) {
                SendMessage(g_hwndBatMeter, WM_COMMAND, wParam, lParam);
            }
    }
}

/*******************************************************************************
*
*  Power_OnPowerBroadcast
*
*  DESCRIPTION:
*   Process WM_POWERBROADCAS message for the battery meter dialog.
*
*  PARAMETERS:
*
*******************************************************************************/

void Power_OnPowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
   if (wParam == PBT_APMPOWERSTATUSCHANGE) {

      // If the power icon is not showing (power service disabled) and
      // we are running on batteries, enable the systray power service.
      if (!g_bPowerEnabled && RunningOffLine()) {
         PostMessage(hWnd, STWM_ENABLESERVICE, STSERVICE_POWER, TRUE);
         return;
      }

      // If the power icon is showing (power service enabled) and
      // we are not running on batteries, disable the systray power service.
      if (g_bPowerEnabled && !RunningOffLine()) {
         PostMessage(hWnd, STWM_ENABLESERVICE, STSERVICE_POWER, FALSE);
         return;
      }

      // Don't change the state of the power service, just update the icon.
      Power_UpdateStatus(hWnd, NIM_MODIFY, FALSE);
   }
}

/*******************************************************************************
*
*  Power_OnDeviceChange
*
*  DESCRIPTION:
*   Process WM_DEVICECHANGE message for the battery meter dialog.
*
*  PARAMETERS:
*
*******************************************************************************/

void Power_OnDeviceChange(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
   //
   // Only listen to the WM_DEVICECHANGE if it is for GUID_DEVICE_BATTERY and
   // it is a DBT_DEVICEARRIVAL, DBT_DEVICEREMOVECOMPLETE, or DBT_DEVICEQUERYREMOVEFAILED.
   //
   if (((wParam == DBT_DEVICEARRIVAL) ||
#ifndef WINNT
       (wParam == DBT_DEVICEREMOVECOMPLETE) ||
#endif
       (wParam == DBT_DEVICEQUERYREMOVEFAILED)) &&
       (lParam) &&
       (((PDEV_BROADCAST_DEVICEINTERFACE)lParam)->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE) &&
       (IsEqualGUID(&((PDEV_BROADCAST_DEVICEINTERFACE)lParam)->dbcc_classguid, &GUID_DEVICE_BATTERY))) {

      // Make sure BatMeter has been initialized.
      if (g_hwndBatMeterFrame) {
         if (g_hwndBatMeter) {
            g_hwndBatMeter = DestroyBatMeter(g_hwndBatMeter);
         }
         g_hwndBatMeter = CreateBatMeter(hWnd,
                                         g_hwndBatMeterFrame,
                                         g_bShowMulti,
                                         NULL);
         InvalidateRect(hWnd, NULL, TRUE);
      }
   }
}

/*******************************************************************************
*
*  Power_OnActivate
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN Power_OnActivate(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
   if (g_hwndBatMeter) {
      SendMessage(g_hwndBatMeter, WM_ACTIVATE, wParam, lParam);
      return TRUE;
   }
   return FALSE;
}

/*******************************************************************************
*
*  PowerProfileEnumProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

#define POWERMENU_SCHEME 300

BOOLEAN CALLBACK PowerProfileEnumProc(
    UINT                    uiID,
    DWORD                   dwNameSize,
    LPTSTR                  lpszName,
    DWORD                   dwDescSize,
    LPTSTR                  lpszDesc,
    PPOWER_POLICY           ppp,
    LPARAM                  lParam
)
{
    PPOWER_PROFILE_ENUM_PROC_PARAMS pppepp;
    MENUITEMINFO mii;

    if ((pppepp = (PPOWER_PROFILE_ENUM_PROC_PARAMS) lParam) == NULL) {
        return FALSE;
    }

    AppendMenu(pppepp->hMenu, MF_STRING,
               POWERMENU_SCHEME + g_uiPowerSchemeCount, lpszName);

    // Store the power scheme ID in the menu info.
    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_DATA;
    mii.dwItemData = uiID;
    SetMenuItemInfo(pppepp->hMenu,
                    POWERMENU_SCHEME + g_uiPowerSchemeCount,
                    FALSE, &mii);

    if (uiID == pppepp->uiCurActiveID) {
        pppepp->uiCurActiveIndex = POWERMENU_SCHEME + g_uiPowerSchemeCount;
    }

    g_uiPowerSchemeCount++;
    return TRUE;
}

/*----------------------------------------------------------------------------
 * GetPowerMenu()
 *
 * Build a menu containing battery meter/power selections.
 *
 *----------------------------------------------------------------------------*/

#define POWERMENU_OPEN          100
#define POWERMENU_PROPERTIES    101

#define POWERMENU_ENABLEWARN    200
#define POWERMENU_SHOWTIME      201
#define POWERMENU_SHOWPERCENT   202


HMENU
GetPowerMenu(LONG l)
{
    LPTSTR  lpszMenu;
    UINT    uiCurActiveID;

    POWER_PROFILE_ENUM_PROC_PARAMS  ppepp;

    if (l > 0)
    {
        // Right button menu -- can change, rebuild each time.
       if (g_hMenu[0])
       {
           DestroyMenu(g_hMenu[0]);
       }

       g_hMenu[1] = CreatePopupMenu();

       // Properties for Power, PowerCfg.
       if ((lpszMenu = LoadDynamicString(IDS_PROPFORPOWER)) != NULL)
       {
           AppendMenu(g_hMenu[1], MF_STRING, POWERMENU_PROPERTIES, lpszMenu);
           DeleteDynamicString(lpszMenu);
       }

       // If we have a battery meter, add it's menu item and set as default.
       if (g_hwndBatMeter) {
           if ((lpszMenu = LoadDynamicString(IDS_OPEN)) != NULL)
           {
               AppendMenu(g_hMenu[1], MF_STRING, POWERMENU_OPEN, lpszMenu);
               DeleteDynamicString(lpszMenu);
           }
           // Open BatMeter is default (double click action)
           SetMenuDefaultItem(g_hMenu[1], POWERMENU_OPEN, FALSE);
       }
       else {
           // Use open PowerCfg as default (double click action)
           SetMenuDefaultItem(g_hMenu[1], POWERMENU_PROPERTIES, FALSE);
       }
    }

    // Left button menu -- can change, rebuild each time.
    if (g_hMenu[0])
    {
        DestroyMenu(g_hMenu[0]);
    }

    g_hMenu[0] = CreatePopupMenu();

    // Get the currently active power policies.
    if (GetActivePwrScheme(&uiCurActiveID)) {
        g_uiPowerSchemeCount = 0;
        ppepp.hMenu = g_hMenu[0];
        ppepp.uiCurActiveID = uiCurActiveID;
        EnumPwrSchemes(PowerProfileEnumProc, (LPARAM)&ppepp);

        // Check the currently active menu item.
        CheckMenuRadioItem(g_hMenu[0],
                           POWERMENU_SCHEME,
                           POWERMENU_SCHEME + g_uiPowerSchemeCount - 1,
                           ppepp.uiCurActiveIndex,
                           MF_BYCOMMAND);
    }
    return g_hMenu[l];
}

/*----------------------------------------------------------------------------
 * Power_Open
 *
 * Update and display the battery meter dialog
 *
 *----------------------------------------------------------------------------*/

void
Power_Open(HWND hWnd)
{
    if (g_hwndBatMeter) {
        SetFocus(GetDlgItem(hWnd, IDC_ENABLEMETER));
        CheckDlgButton(hWnd, IDC_ENABLEMULTI,
                       (g_gpp.user.GlobalFlags & EnableMultiBatteryDisplay) ?
                       BST_CHECKED : BST_UNCHECKED);

        CheckDlgButton(hWnd, IDC_ENABLEMETER,
                       (g_gpp.user.GlobalFlags & EnableSysTrayBatteryMeter) ?
                       BST_CHECKED : BST_UNCHECKED);

        Power_UpdateStatus(hWnd, NIM_MODIFY, FALSE); // show current info
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
    }
    else {
        SysTray_RunProperties(IDS_RUNPOWERPROPERTIES);
    }
}


/*----------------------------------------------------------------------------
 * DoPowerMenu
 *
 * Create and process a right or left button menu.
 *
 *----------------------------------------------------------------------------*/

void
DoPowerMenu(HWND hwnd, UINT uMenuNum, UINT uButton)
{
    POINT pt;
    UINT iCmd;
    MENUITEMINFO mii;

    SetForegroundWindow(hwnd);
    GetCursorPos(&pt);

    iCmd = (UINT)TrackPopupMenu(GetPowerMenu(uMenuNum),
                          uButton | TPM_RETURNCMD | TPM_NONOTIFY,
                          pt.x, pt.y, 0, hwnd, NULL);

    if (iCmd >= POWERMENU_SCHEME) {
        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_DATA;
        if (GetMenuItemInfo(g_hMenu[uMenuNum], iCmd, FALSE, &mii)) {
            SetActivePwrScheme((UINT)mii.dwItemData, NULL, NULL);
            PowerCfg_Notify();
        }
    }
    else {
        switch (iCmd) {

            case POWERMENU_OPEN:
                Power_Open(hwnd);
                break;

            case POWERMENU_PROPERTIES:
                SysTray_RunProperties(IDS_RUNPOWERPROPERTIES);
                break;

            case 0:
                // The user cancelled the menu without choosing.
                SetIconFocus(hwnd, STWM_NOTIFYPOWER);
                break;
        }
    }
}


/*----------------------------------------------------------------------------
 * Power_Notify
 *
 * Handle a notification from the power tray icon.
 *
 *----------------------------------------------------------------------------*/

#define PN_TIMER_CLEAR  0
#define PN_TIMER_SET    1
#define PN_DBLCLK       2

UINT g_uiTimerSet = PN_TIMER_CLEAR;

void Power_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
        case WM_RBUTTONUP:
        DoPowerMenu(hWnd, 1, TPM_RIGHTBUTTON);  // right button menu
            break;

    case WM_LBUTTONUP:
        // start timing for left button menu
        if (g_uiTimerSet == PN_TIMER_CLEAR) {
            SetTimer(hWnd, POWER_TIMER_ID, GetDoubleClickTime()+100, NULL);
            g_uiTimerSet = PN_TIMER_SET;
        }
        break;

        case WM_LBUTTONDBLCLK:
        g_uiTimerSet = PN_DBLCLK;
        Power_Open(hWnd);                       // show battery meter dialog
            break;
    }
}

/*-----------------------------------------------------------------------------
 * Power_Timer
 *
 * Execute the left button menu on WM_LBUTTONDOWN time-out.
 *
 *----------------------------------------------------------------------------*/

void Power_Timer(HWND hwnd)
{
    KillTimer(hwnd, POWER_TIMER_ID);
    if (g_uiTimerSet != PN_DBLCLK) {
        DoPowerMenu(hwnd, 0, TPM_LEFTBUTTON);
    }
    g_uiTimerSet = PN_TIMER_CLEAR;
}

/*----------------------------------------------------------------------------
 * Update_PowerFlags
 *
 * Set power flags using powrprof.dll API's.
 *
 *----------------------------------------------------------------------------*/

void Update_PowerFlags(DWORD dwMask, BOOL bEnable)
{
    if (bEnable) {
        g_gpp.user.GlobalFlags |= dwMask;
    }
    else {
        g_gpp.user.GlobalFlags &= ~dwMask;
    }
    WriteGlobalPwrPolicy(&g_gpp);
}

/*----------------------------------------------------------------------------
 * Get_PowerFlags
 *
 * Get power flags using powrprof.dll API's.
 *
 *----------------------------------------------------------------------------*/

DWORD Get_PowerFlags(void)
{
    ReadGlobalPwrPolicy(&g_gpp);
    return g_gpp.user.GlobalFlags;
}


/*******************************************************************************
*
*  BatteryMeterInit
*
*  DESCRIPTION:
*       NOTE: Can be called multiple times.  Simply re-init.
*
*  PARAMETERS:
*     (returns), TRUE if the Battery Meter could be enabled
*
*******************************************************************************/

BOOL PASCAL BatteryMeterInit(HWND hWnd)
{
   PUINT puiBatCount = NULL;

   if (!BatMeterCapabilities(&puiBatCount)) {
      return FALSE;
   }

   if (!g_hwndBatMeter) {
      g_hwndBatMeterFrame = GetDlgItem(hWnd, IDC_STATIC_FRAME_BATMETER);
      g_bShowMulti = g_gpp.user.GlobalFlags & EnableMultiBatteryDisplay;
      g_hwndBatMeter = CreateBatMeter(hWnd,
                                      g_hwndBatMeterFrame,
                                      g_bShowMulti,
                                      NULL);
   }
   return TRUE;
}

/*******************************************************************************
*
*  Power_UpdateStatus
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID PASCAL Power_UpdateStatus(
    HWND hWnd,
    DWORD NotifyIconMessage,
    BOOL bForceUpdate
)
{
   static  TCHAR szTipCache[64];
   static  HICON hIconCache;

   TCHAR   szTip[64];
   LPTSTR  lpsz;
   BATTERY_STATE bs;
   UINT    uiHour, uiMin;

   bs.ulSize = sizeof(BATTERY_STATE);
   UpdateBatMeter(g_hwndBatMeter,
                  g_bShowMulti,
                  bForceUpdate,
                  &bs);

   // Build up a new tool tip.
   if (g_hwndBatMeter &&
       !(((bs.ulPowerState & BATTERY_POWER_ON_LINE) &&
          !(bs.ulPowerState & BATTERY_CHARGING)))) {

      if (bs.ulBatLifePercent <= 100) {
         if (bs.ulBatLifeTime != (UINT) -1) {
            uiHour = bs.ulBatLifeTime / 3600;
            uiMin  = (bs.ulBatLifeTime % 3600) / 60;
            if (uiHour) {
               lpsz = LoadDynamicString(IDS_TIMEREMFORMATHOUR,
                                        uiHour, uiMin,
                                        bs.ulBatLifePercent);
            }
            else {
               lpsz = LoadDynamicString(IDS_TIMEREMFORMATMIN, uiMin,
                                        bs.ulBatLifePercent);
            }
            if (lpsz) {
               lstrcpy(szTip, lpsz);
               LocalFree(lpsz);
               if (bs.ulPowerState & BATTERY_CHARGING) {
                  if ((lpsz = LoadDynamicString(IDS_CHARGING)) != NULL) {
                     lstrcat(szTip, lpsz);
                     LocalFree(lpsz);
                  }
               }
            }
         }
         else {
            if ((lpsz = LoadDynamicString(IDS_REMAINING,
                                          bs.ulBatLifePercent)) != NULL) {
               lstrcpy(szTip, lpsz);
               LocalFree(lpsz);
               if (bs.ulPowerState & BATTERY_CHARGING) {
                  if ((lpsz = LoadDynamicString(IDS_CHARGING)) != NULL) {
                     lstrcat(szTip, lpsz);
                     LocalFree(lpsz);
                  }
               }
            }
         }
      }
      else {
         lpsz = LoadDynamicString(IDS_UNKNOWN);
         lstrcpy(szTip, lpsz);
         LocalFree(lpsz);
      }
   }
   else {
      lpsz = LoadDynamicString(IDS_ACPOWER);
      lstrcpy(szTip, lpsz);
      LocalFree(lpsz);
   }

   if ((NotifyIconMessage == NIM_ADD)  ||
       (hIconCache != bs.hIconCache16) ||
       (lstrcmp(szTip, szTipCache))) {

      hIconCache = bs.hIconCache16;
      lstrcpy(szTipCache, szTip);

      SysTray_NotifyIcon(hWnd, STWM_NOTIFYPOWER, NotifyIconMessage,
                         hIconCache, szTipCache);
   }
}

/*******************************************************************************
*
*  RegisterForDeviceNotification
*
*  DESCRIPTION:
*    Do onetime registration for WM_DEVICECHANGED.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL RegisterForDeviceNotification(HWND hWnd)
{
   DEV_BROADCAST_DEVICEINTERFACE dbc;

   memset(&dbc, 0, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
   dbc.dbcc_size         = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
   dbc.dbcc_devicetype   = DBT_DEVTYP_DEVICEINTERFACE;
   dbc.dbcc_classguid    = GUID_DEVICE_BATTERY;
   g_hDevNotify = RegisterDeviceNotification(hWnd,
                                             &dbc,
                                             DEVICE_NOTIFY_WINDOW_HANDLE);
   if (!g_hDevNotify) {
      return FALSE;
   }
   return TRUE;
}

/*******************************************************************************
*
*  Power_WmDestroy
*
*  DESCRIPTION:
*
*
*  PARAMETERS:
*
*******************************************************************************/

void Power_WmDestroy(HWND hWnd)
{
   if (g_hDevNotify) {
      UnregisterDeviceNotification(g_hDevNotify);
      g_hDevNotify = NULL;
   }
}

/*******************************************************************************
*
*  Power_CheckEnable
*
*  DESCRIPTION:
*   Return TRUE if the power service icon was enabled.
*        Can be called multiple times.  Simply re-init.
*
*  PARAMETERS:
*     bSvcEnabled - Request to enable/disable power service on tray.
*
*******************************************************************************/

BOOL Power_CheckEnable(HWND hWnd, BOOL bSvcEnable)
{
   static BOOL bRegisteredForDC = FALSE;
#ifdef WINNT
   DWORD         dwSessionId;

   // On NT don't allow power management UI
   // if we're running as a remote session. As per SalimC.
   if (ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId)) {
      if (dwSessionId) {
         return FALSE;
      }
   }
#endif

   // Is there any reason to display the systray power icon?
   if (!PowerCapabilities()) {
      return FALSE;
   }

   // Do onetime registration for WM_DEVICECHANGED.
   if (!bRegisteredForDC) {
      bRegisteredForDC = RegisterForDeviceNotification(hWnd);
   }

   // Get current battery meter flags from the registry
   Get_PowerFlags();

   // Are we running on battery power or has the user set
   // the systray power icon to always on? If so, force enable.
   if ((g_gpp.user.GlobalFlags & EnableSysTrayBatteryMeter) ||
       (RunningOffLine())) {
      bSvcEnable = TRUE;
   }
   else {
      bSvcEnable = FALSE;
   }

   // Set the power service state.
   if (bSvcEnable) {
      if (g_bPowerEnabled) {
         Power_UpdateStatus(hWnd, NIM_MODIFY, FALSE);
      }
      else {
         BatteryMeterInit(hWnd);
         Power_UpdateStatus(hWnd, NIM_ADD, FALSE);
      }
      g_bPowerEnabled = TRUE;
   }
   else {
      SysTray_NotifyIcon(hWnd, STWM_NOTIFYPOWER, NIM_DELETE, NULL, NULL);
      g_bPowerEnabled = FALSE;
   }
   return g_bPowerEnabled;
}


