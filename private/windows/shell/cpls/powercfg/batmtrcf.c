/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       BATMTRCF.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Support for the battery meter configuration page of PowerCfg.Cpl.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>
#include <powercfp.h>
#include <dbt.h>

#include <objbase.h>
#include <initguid.h>
#include <ntpoapi.h>
#include <poclass.h>

#include "powercfg.h"
#include "pwrresid.h"
#include "PwrMn_cs.h"

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

extern UINT g_uiEnableSysTrayFlag;

// A systary change requires PowerSchemeDlgProc re-init.
extern BOOL g_bSystrayChange;

// Persistant storage of this data is managed by POWRPROF.DLL API's.
extern GLOBAL_POWER_POLICY  g_gpp;

// Subclass variables:
WNDPROC g_fnOldPropShtProc;

// BatMeter creation parameters.
HWND    g_hwndBatMeter;
BOOL    g_bShowMulti;
HWND    g_hwndBatMeterFrame;

// Show/hide multi-bat display check box.
DWORD g_dwShowMultiBatDispOpt = CONTROL_ENABLE;

// Static flags:
UINT g_uiEnableMultiFlag = EnableMultiBatteryDisplay;

#ifdef WINNT
// Used to track registration for WM_DEVICECHANGED message.
HDEVNOTIFY g_hDevNotify;
#endif

// Battery meter policies dialog controls descriptions:
#define NUM_BATMETERCFG_CONTROLS 1

POWER_CONTROLS g_pcBatMeterCfg[NUM_BATMETERCFG_CONTROLS] =
{// Control ID          Control Type    Data Address                  Data Size                         Parameter Pointer       Enable/Visible State Pointer
    IDC_ENABLEMULTI,    CHECK_BOX,      &(g_gpp.user.GlobalFlags),    sizeof(g_gpp.user.GlobalFlags),   &g_uiEnableMultiFlag,   &g_dwShowMultiBatDispOpt,
};

// "Battery Meter" Dialog Box (IDD_BATMETERCFG == 102) help arrays:

const DWORD g_BatMeterCfgHelpIDs[]=
{
    IDC_ENABLEMULTI,    IDH_102_1204,   // Battery Meter: "Show the status of all &batteries." (Button)
    IDC_STATIC_FRAME_BATMETER,  IDH_102_1205,   // Battery Meter: "Batmeter frame" (Static)
    IDC_POWERSTATUSGROUPBOX,    IDH_102_1201,   // Battery Meter: "Power status" (Button)
    0, 0
};

#ifdef WINNT
// Private function prototypes
BOOL RegisterForDeviceNotification(HWND hWnd);
void UnregisterForDeviceNotification(void);
#endif

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  PropShtSubclassProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

LRESULT PropShtSubclassProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT lRet;

   lRet = CallWindowProc(g_fnOldPropShtProc, hWnd, uiMsg, wParam, lParam);

   if ((uiMsg == WM_POWERBROADCAST) && (wParam == PBT_APMPOWERSTATUSCHANGE)) {
      UpdateBatMeter(g_hwndBatMeter, g_bShowMulti, TRUE, NULL);
   }
   return lRet;
}

/*******************************************************************************
*
*  BatMeterCfgDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR CALLBACK BatMeterCfgDlgProc
(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    static BOOL bDirty = FALSE;
#ifdef WINNT
    static BOOL bRegisteredForDC = FALSE;
#endif

    NMHDR *lpnm;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            // If we can't read the global power policies
            // disable the controls on this page.
            if (!GetGlobalPwrPolicy(&g_gpp)) {
                DisableControls(hWnd, NUM_BATMETERCFG_CONTROLS, g_pcBatMeterCfg);
            }
            else {
                if (g_gpp.user.GlobalFlags & EnableMultiBatteryDisplay) {
                    g_bShowMulti = TRUE;
                }
                else {
                    g_bShowMulti = FALSE;
                }

                // If we can't write the global power policies disable
                // the controls this page.
                if (!WriteGlobalPwrPolicyReport(hWnd, &g_gpp)) {
                    HideControls(hWnd, NUM_BATMETERCFG_CONTROLS, g_pcBatMeterCfg);
                }

                SetControls(hWnd, NUM_BATMETERCFG_CONTROLS, g_pcBatMeterCfg);
            }
            g_hwndBatMeterFrame = GetDlgItem(hWnd, IDC_STATIC_FRAME_BATMETER);
            g_hwndBatMeter = CreateBatMeter(hWnd,
                                            g_hwndBatMeterFrame,
                                            g_bShowMulti,
                                            NULL);

            // The top level window must be subclassed to receive
            // the WM_POWERBROADCAST message.
            if (g_hwndBatMeter) {
                g_fnOldPropShtProc =
                    (WNDPROC) SetWindowLongPtr(GetParent(hWnd), DWLP_DLGPROC,
                                            (LONG_PTR)PropShtSubclassProc);

#ifdef WINNT
                // Do onetime registration for WM_DEVICECHANGED.
                if (!bRegisteredForDC) {
                   bRegisteredForDC = RegisterForDeviceNotification(hWnd);
                }
#endif
            }
            return TRUE;

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code) {
                case PSN_APPLY:
                    if (bDirty) {
                        GetControls(hWnd, NUM_BATMETERCFG_CONTROLS,
                                    g_pcBatMeterCfg);
                        WriteGlobalPwrPolicyReport(hWnd, &g_gpp);
                        bDirty = FALSE;
                    }
                    break;
            }
            break;

        case WM_COMMAND:
            switch (wParam) {
                case IDC_ENABLEMULTI:
                    GetControls(hWnd, NUM_BATMETERCFG_CONTROLS, g_pcBatMeterCfg);
                    if (g_gpp.user.GlobalFlags & EnableMultiBatteryDisplay) {
                        g_bShowMulti = TRUE;
                    }
                    else {
                        g_bShowMulti = FALSE;
                    }

                    UpdateBatMeter(g_hwndBatMeter, g_bShowMulti, TRUE, NULL);

                    // Enable the parent dialog Apply button on change.
                    MarkSheetDirty(hWnd, &bDirty);
                    break;

                default:
                    // Notify battery meter of enter key events.
                    if (HIWORD(wParam) == BN_CLICKED) {
                        SendMessage(g_hwndBatMeter, uMsg, wParam, lParam);
                    }
            }
            break;

        case PCWM_NOTIFYPOWER:
            // Systray changed something. Get the flags and update controls.
            if (GetGlobalPwrPolicy(&g_gpp)) {
                SetControls(hWnd, NUM_BATMETERCFG_CONTROLS, g_pcBatMeterCfg);
            }
            g_bSystrayChange = TRUE;
            break;

        case WM_DEVICECHANGE:
            if ((wParam == DBT_DEVICEARRIVAL) ||
#ifndef WINNT
                (wParam == DBT_DEVICEREMOVECOMPLETE) ||
#endif
                (wParam == DBT_DEVICEQUERYREMOVEFAILED)) {

               if (g_hwndBatMeter) {
                  g_hwndBatMeter = DestroyBatMeter(g_hwndBatMeter);
               }
               g_hwndBatMeter = CreateBatMeter(hWnd,
                                               g_hwndBatMeterFrame,
                                               g_bShowMulti,
                                               NULL);
               InvalidateRect(hWnd, NULL, TRUE);
            }
            return TRUE;

#ifdef WINNT
        case WM_DESTROY:
            UnregisterForDeviceNotification();
            break;
#endif

        case WM_HELP:             // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_BatMeterCfgHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_BatMeterCfgHelpIDs);
            return TRUE;
    }

    return FALSE;
}

/*******************************************************************************
*
*                 P R I V A T E   F U N C T I O N S
*
*******************************************************************************/

#ifdef WINNT
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
      DebugPrint( "RegisterForDeviceNotification failed");
      return FALSE;
   }
   return TRUE;
}

/*******************************************************************************
*
*  UnregisterForDeviceNotification
*
*  DESCRIPTION:
*
*
*  PARAMETERS:
*
*******************************************************************************/

void UnregisterForDeviceNotification(void)
{
   if (g_hDevNotify) {
      UnregisterDeviceNotification(g_hDevNotify);
      g_hDevNotify = NULL;
   }
}
#endif
