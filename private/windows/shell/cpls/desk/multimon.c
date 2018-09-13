/*  MULTIMON.C
**
**  Copyright (C) Microsoft, 1993, All Rights Reserved.
**
**  History:
**      Francish / wrote this lame hack until we have 32bit PnP APIs
**      30-Sep-1996 JonPa   Ported lame hack to 32bits for NT.
**                  In the process made it a little less lame.
*/
#include <windows.h>
#include <windowsx.h>
#include "desk.h"
#include "deskid.h"
//#include <print.h>
#include <regstr.h>
#include <memory.h>
#include <help.h>
#include <cfgmgr32.h>
//#include <commctrl.h>
#include "dragsize.h"

#define _WARNING_ "foobar"  // Some stupid part of the build program changes any string with "warning"
                            // in it to "error" and causes the build to stop on some machines

#ifdef TEST_ON_ONE_MON
#   pragma message( __FILE__"(21):" _WARNING_ " !!!: compiling with one monitor test hacks")
#endif

//-----------------------------------------------------------------------------
//TCHAR g_szDisplayX[] = TEXT("\\\\.\\Display%d");

//
// BUGBUG take this out.
//
#ifndef CDS_SETRECT
#define CDS_SETRECT	    0x20000000
#define CDS_NORESET         0x10000000
#endif

LONG CALLBACK MonitorWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam);

//
// CleanUpDesktopRectangles flags
//
#define CUDR_NORMAL             0x0000
#define CUDR_NOSNAPTOGRID       0x0001
#define CUDR_NORESOLVEPOSITIONS 0x0002
#define CUDR_NOCLOSEGAPS        0x0004

#define MONITORS_MAX    10

void NEAR PASCAL CleanUpDesktopRectangles(LPRECT arc, UINT count, UINT flags);
void CleanupRects(HWND hwndP);

//DWORD GetMonitorDevNode(int dev);
//DWORD GetDisplayDevNode(int dev);
//BOOL GetDisplayInfo(DWORD dev, DISPLAYINFO FAR *pdi);

#define DisplayExists(dev)  GetDisplayInfo(NULL, dev)
BOOL GetDisplayInfo( LPTSTR psz, DWORD iDev );

//
// union of all monitor RECTs
//
RECT rcDesk;

//
// how to translate to preview size
//
int   DeskScale;
POINT DeskOff;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DeskToPreview(LPRECT in, LPRECT out)
{
    out->left   = DeskOff.x + MulDiv(in->left   - rcDesk.left,DeskScale,1000);
    out->top    = DeskOff.y + MulDiv(in->top    - rcDesk.top, DeskScale,1000);
    out->right  = DeskOff.x + MulDiv(in->right  - rcDesk.left,DeskScale,1000);
    out->bottom = DeskOff.y + MulDiv(in->bottom - rcDesk.top, DeskScale,1000);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void PreviewToDesk(LPRECT in, LPRECT out)
{
    out->left   = rcDesk.left + MulDiv(in->left   - DeskOff.x,1000,DeskScale);
    out->top    = rcDesk.top  + MulDiv(in->top    - DeskOff.y,1000,DeskScale);
    out->right  = rcDesk.left + MulDiv(in->right  - DeskOff.x,1000,DeskScale);
    out->bottom = rcDesk.top  + MulDiv(in->bottom - DeskOff.y,1000,DeskScale);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
HWND GetMonitorW(HWND hDlg, int iDevice)
{
    return GetDlgItem(GetDlgItem(hDlg, IDC_DISPLAYDESK), iDevice);
}

//-----------------------------------------------------------------------------
// GetDisplaySettings
//
// call USER to get the settings for a device.
//
//-----------------------------------------------------------------------------

BOOL GetDisplaySettings(int i, LPDEVMODE pdm, LPRECT prc)
{
    TCHAR ach[40];
#ifdef TEST_ON_ONE_MON
    if (i < 3)
        i = 1;
#endif

    GetDisplayInfo(ach, i);


    FillMemory(pdm,  sizeof(DEVMODE), 0);
    pdm->dmSize = sizeof(DEVMODE);

#if 1
    // check for a bogus USER
    if (EnumDisplaySettings(ach, (DWORD)-10, pdm))
        return FALSE;
#endif

    if (!EnumDisplaySettings(ach, ENUM_REGISTRY_SETTINGS, pdm))
        return FALSE;

    DPRINTF((DM_TRACE, TEXT("%s registry settings are %dx%dx%d %dHz [%d,%d,%d,%d]"),
        (LPTSTR)ach,
        (int)pdm->dmPelsWidth,
        (int)pdm->dmPelsHeight,
        (int)pdm->dmBitsPerPel,
        (int)pdm->dmDisplayFrequency,
        (int)((LPRECTL)&pdm->dmOrientation)->left,
        (int)((LPRECTL)&pdm->dmOrientation)->top,
        (int)((LPRECTL)&pdm->dmOrientation)->right,
        (int)((LPRECTL)&pdm->dmOrientation)->bottom));

    if (EnumDisplaySettings(ach, ENUM_CURRENT_SETTINGS, pdm))
    {
        DPRINTF((DM_TRACE, TEXT("%s current settings are %dx%dx%d %dHz [%d,%d,%d,%d]"),
            (LPTSTR)ach,
            (int)pdm->dmPelsWidth,
            (int)pdm->dmPelsHeight,
            (int)pdm->dmBitsPerPel,
            (int)pdm->dmDisplayFrequency,
            (int)((LPRECTL)&pdm->dmOrientation)->left,
            (int)((LPRECTL)&pdm->dmOrientation)->top,
            (int)((LPRECTL)&pdm->dmOrientation)->right,
            (int)((LPRECTL)&pdm->dmOrientation)->bottom));

        prc->left   = (int)((LPRECTL)&pdm->dmOrientation)->left;
        prc->top    = (int)((LPRECTL)&pdm->dmOrientation)->top;
        prc->right  = (int)((LPRECTL)&pdm->dmOrientation)->right;
        prc->bottom = (int)((LPRECTL)&pdm->dmOrientation)->bottom;
    }
    else
    {
        prc->left   = 0;
        prc->top    = 0;
        prc->right  = 0;
        prc->bottom = 0;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// SetDisplaySettings
//
// call USER to set the settings for a device.
//
//-----------------------------------------------------------------------------

LONG SetDisplaySettings(int i, LPDEVMODE pdm, LPRECT prc, DWORD flags)
{
    RECTL rcl = {0,0,0,0};
    LPVOID pv=NULL;
    TCHAR ach[80];

    if (pdm == NULL)
    {
        DPRINTF((DM_TRACE, TEXT("SetDisplaySettings CANT TAKE a NULL pdm!!!!!!")));
        return -1;
    }

    GetDisplayInfo(ach, i);

    if (prc)
    {
        flags     |= CDS_SETRECT;
        rcl.left   = prc->left;
        rcl.top    = prc->top;
        rcl.right  = prc->right;
        rcl.bottom = prc->bottom;
        pv = (LPVOID)&rcl;
    }

    DPRINTF((DM_TRACE, TEXT("%s SetDisplaySettings %dx%dx%d %dHz [%d,%d,%d,%d]"),
        (LPTSTR)ach,
        (int)pdm->dmPelsWidth,
        (int)pdm->dmPelsHeight,
        (int)pdm->dmBitsPerPel,
        (int)pdm->dmDisplayFrequency,
        (int)rcl.left,
        (int)rcl.top,
        (int)rcl.right,
        (int)rcl.bottom));

#if 0
    return ChangeDisplaySettingsEx(ach, pdm, NULL, flags, pv);
#else
    if (pv)
    {
        *(LPRECTL)&pdm->dmOrientation = rcl;
        pdm->dmFields |= DM_ORIENTATION;
        flags &= ~CDS_SETRECT;
    }

    lstrcpy(pdm->dmDeviceName, ach);
    return ChangeDisplaySettings(pdm, flags);
#endif
}

//-----------------------------------------------------------------------------
BOOL CALLBACK MultiMonitorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
const static DWORD FAR aMultiMonitorHelpIds[] =
{
    IDC_DISPLAYLIST,         NO_HELP,
    IDC_DISPLAYDESK,         NO_HELP,
    0, 0
};

//-----------------------------------------------------------------------------
#ifdef WIN95_GDI_HACK
    // This function is only ever used to validate that GDI can talk to a
    // specific display devices.  According to AndreVa, if it came back
    // from EnumDisplayDevices, then NT GDI will always know how to talk
    // to it, so this check is now bogus.

HDC FAR _GetDisplayIC(int iDisplay)
{
    HDC dc;



    if (iDisplay == 1)
    {
//
// BUGBUG this assumes \\.\Display1 is primary
//
        dc = GetDC(NULL);
    }
    else if (iDisplay > 1)
    {
        TCHAR szDisplay[64];
        DEVMODE dm;
        RECT rc;

        GetDisplayInfo(szDisplay, iDisplay);
        GetDisplaySettings(iDisplay, &dm, &rc);

        DPRINTF((DM_TRACE, TEXT("GetDisplayIC: calling CreateDC(\"%s\",%d,%d,%d)"),
            (LPTSTR)szDisplay,
            (int)dm.dmPelsWidth,
            (int)dm.dmPelsHeight,
            (int)dm.dmBitsPerPel));

        dc = CreateDC(szDisplay, NULL, NULL, &dm);

        if (dc == NULL)
        {
            dm.dmBitsPerPel = 8;
            dm.dmPelsWidth  = 640;
            dm.dmPelsHeight = 480;

            DPRINTF((DM_TRACE, TEXT("GetDisplayIC: CreateDC failed, trying 640x480x8")));

            dc = CreateDC(szDisplay, NULL, NULL, &dm);

            if (dc != NULL)
            {
                DPRINTF((DM_TRACE, TEXT("GetDisplayIC: writing 640x480x8 to registry.")));
                SetDisplaySettings(iDisplay, &dm, &rc, CDS_NORESET | CDS_UPDATEREGISTRY);
            }
        }
    }
    else
    {
        dc = NULL;
    }

    return dc;
}

//-----------------------------------------------------------------------------
void FAR _ReleaseDisplayIC(int iDisplay, HDC dc)
{
    if (iDisplay == 1)
    {
        ReleaseDC(NULL, dc);
    }
    else if (iDisplay > 1)
    {
        DeleteDC(dc);
    }
}
#endif


//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
#if 0
BOOL MM_Read_Registry_Value(DWORD dn, LPTSTR pszDesc, DWORD dwRegType, LPVOID pvData, LPDWORD lpcbData, INT cmReg) {
    HKEY hk;
    DWORD RegDataType;
    BOOL fRet = FALSE;

    if(CM_Open_DevNode_Key(dn,
                           KEY_READ,
                           0,
                           RegDisposition_OpenExisting,
                           &hk,
                           cmReg) == CR_SUCCESS) {

        if((RegQueryValueEx(hk,
                            pszDesc,
                            NULL,
                            &RegDataType,
                            pvData,
                            lpcbData) == ERROR_SUCCESS) && (RegDataType == dwRegType)) {
            fRet = TRUE;
        }

        RegCloseKey(hk);
    }

    return fRet;
}
#endif
///////////////////////////////////////////////////////////////////////////////


//BUGBUG - remove this if it becomes obsolete
extern VOID GetDisplayAdaptorDescriptionTheOldWay(LPTSTR szDesc, LPTSTR szDevName);

void GetDisplayDescription(int iDevice, LPTSTR szDescription, DWORD cb)
{
    TCHAR disp[80];
    TCHAR mon[80];
    DWORD dn;
    BOOL fOK = FALSE;
#if 0
//
// BUGBUG this uses PnP specific stuff
//
    if (dn = GetMonitorDevNode(iDevice))
    {
        cb = sizeof(mon);
        fOK = MM_Read_Registry_Value(dn, REGSTR_VAL_DRVDESC,
            REG_SZ, mon, &cb, CM_REGISTRY_SOFTWARE);
    }
    if (!fOK)
        LoadString(hInstance, IDS_UNKNOWN, mon, ARRAYSIZE(mon));

    fOK = FALSE;
    if (dn = GetDisplayDevNode(iDevice))
    {
        cb = sizeof(disp);
        fOK = MM_Read_Registry_Value(dn, REGSTR_VAL_DRVDESC,
            REG_SZ, disp, &cb, CM_REGISTRY_SOFTWARE);
    }
    if (!fOK)
        LoadString(hInstance, IDS_UNKNOWN, disp, ARRAYSIZE(disp));

    wsprintf(szDescription, TEXT("%s on %s"), (LPTSTR)mon, (LPTSTR)disp);
#else
    DISPLAY_DEVICE dspdev;

    ZeroMemory( &dspdev, sizeof(dspdev) );
    dspdev.cb = sizeof(dspdev);

#ifdef TEST_ON_ONE_MON
    if (iDevice < 3)
        iDevice = 1;

#endif

    if (EnumDisplayDevices( NULL, iDevice, &dspdev, 0 )) {

        lstrcpyn(szDescription, dspdev.DeviceString, cb );

        // HACK to fix bug in EnumDisplayDevices
        if (szDescription[0] == TEXT('\0')) {
            GetDisplayAdaptorDescriptionTheOldWay(szDescription, dspdev.DeviceName);
        }
    } else {
        LoadString(hInstance, IDS_UNKNOWN, szDescription, cb);
    }

    //BUGBUG - add monitor name when we can get it
#endif
}

//-----------------------------------------------------------------------------
BOOL FAR GetMonitorSettingsPage(LPPROPSHEETPAGE psp, int iDevice)
{
    BOOL fDefault = FALSE;

    DEVMODE dm;
    RECT rc;

    if ((psp->dwSize != sizeof(PROPSHEETPAGE)) || !InitDragSizeClass()) {
        DPRINTF((DM_TRACE, TEXT("GetMonitorSettingsPage: init failed")));
        return FALSE;
    }

    //
    // what page does the caller want?
    //
    // devices 1..n are real device indices
    // device 0 is SETTINGSPAGE_FALLBACK
    // device -1 is SETTINGSPAGE_DEFAULT
    //
    // device 1 is the primary display (the VGA)
    // SETTINGSPAGE_FALLBACK is also the VGA but gets passed down to the page
    //
    if ((iDevice != 1) &&
        (iDevice != SETTINGSPAGE_FALLBACK) &&
        !DisplayExists(1))
    {
        //
        // the primary display didn't start
        // don't allow anything fancy
        //
        if (iDevice > 1) {
            DPRINTF((DM_TRACE, TEXT("GetMonitorSettingsPage: GetDevNode Failed")));
            return FALSE;
        }

        // show the primary display settings page (like fallback)
        iDevice = 1;
    }
    else if (iDevice < 0)
    {
        if (iDevice != SETTINGSPAGE_DEFAULT) {
            DPRINTF((DM_TRACE, TEXT("GetMonitorSettingsPage: Negative device requested")));
            return FALSE;
        }

        //
        // caller wants us to pick a page based on how many displays are around
        // check for a display 2 to see if this is a multi-monitor system
        //
        fDefault = TRUE;

        //
        // if there is only one card in the machine *or* USER does not
        // understand multi-displays bring up the settings page for device #1
        //
#ifdef NO_MULTIPAGE
        if (!DisplayExists(2) || !GetDisplaySettings(2, &dm, &rc))
#endif
        {
            // this is a uni-monitor box; show settings for the primary display
            iDevice = 1;
        }
    }
    else if (iDevice > 1)
    {
        //
        // caller asked for a page on a secondary device
        // make sure it actually exists
        //
        if (!DisplayExists(iDevice))
        {
            // sorry, bogus display
            DPRINTF((DM_TRACE, TEXT("GetMonitorSettingsPage: GetDispDevNode failed(2nd call)")));
            return FALSE;
        }
    }

    //
    // fill in the PROSHEETPAGE struct with the appropriate information
    //
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hInstance;

    if (iDevice == SETTINGSPAGE_DEFAULT)
    {
        DPRINTF((DM_TRACE, TEXT("GetMonitorSettingsPage: Creating up MultiMonitor Page")));
        psp->pfnDlgProc = MultiMonitorDlgProc;
        psp->pszTemplate = MAKEINTRESOURCE(DLG_MULTIMONITOR);
        psp->lParam = 0;
    }
    else
    {
        DPRINTF((DM_TRACE, TEXT("GetMonitorSettingsPage: Creating up Single Monitor Page")));
        //psp->pfnDlgProc = MonitorDlgProc;
        psp->pfnDlgProc = DisplayPageProc;
        psp->pszTemplate = MAKEINTRESOURCE(DLG_MONITOR);
        psp->lParam = (LPARAM)MAKELPARAM(iDevice, fDefault);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
void DoMonitorSettingsSheet(HWND hDlg, int iDevice)
{
    TCHAR szDescription[128];
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[1];
    PROPSHEETPAGE psp;
    int iResult = 0;
    HDC hdc;

    if (!DisplayExists(iDevice))
        return;

#ifdef WIN95_GDI_HACK
    hdc = _GetDisplayIC(iDevice);

    if (hdc == NULL)
        return;
#endif

    GetDisplayDescription(iDevice, szDescription, ARRAYSIZE(szDescription));

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hDlg;
    psh.hInstance = hInstance;
    psh.pszCaption = szDescription;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    psp.dwSize = sizeof(psp);
    if (GetMonitorSettingsPage(&psp, iDevice) &&
        ((rPages[psh.nPages] = CreatePropertySheetPage(&psp)) != NULL))
    {
        psh.nPages++;
    }

    if (psh.nPages)
        iResult = PropertySheet(&psh);

    if ((iResult == ID_PSRESTARTWINDOWS) || (iResult == ID_PSREBOOTSYSTEM))
    {
        PropSheet_CancelToClose(GetParent(hDlg));

        if (iResult == ID_PSREBOOTSYSTEM)
            PropSheet_RebootSystem(GetParent(hDlg));
        else
            PropSheet_RestartWindows(GetParent(hDlg));
    }

#ifdef WIN95_GDI_HACK
    _ReleaseDisplayIC(iDevice, hdc);
#endif
}

//-----------------------------------------------------------------------------
static void GetModeName(LPDEVMODE pdm, LPTSTR szMode)
{
    TCHAR fmt[80], cres[80];

    LoadString(hInstance, IDS_CATRESCOLOR, fmt, sizeof(fmt));
    LoadString(hInstance, IDS_COLOR + (int)pdm->dmBitsPerPel / 4, cres, sizeof(cres));
    wsprintf(szMode, fmt, (int)pdm->dmPelsWidth, (int)pdm->dmPelsHeight, (LPCTSTR)cres);
}

static BOOL g_InSetInfo;
static int g_iCurDevice;

//-----------------------------------------------------------------------------
void SetInfo(HWND hDlg, int iDevice)
{
    TCHAR ach[128];
    DEVMODE dm;
    RECT rc;
    BOOL fUseDevice;
    HWND lb = GetDlgItem(hDlg, IDC_DISPLAYLIST);
    HWND hwndC;

    g_InSetInfo = TRUE;

    if (iDevice == -1)
    {
        iDevice = (int)ListBox_GetItemData(lb, ListBox_GetCurSel(lb));
    }
    else
    {
        // BUGBUG assumes non-sorted
        ListBox_SetCurSel(lb, iDevice-1);
    }

    if (GetDisplaySettings(iDevice, &dm, &rc))
    {
        hwndC = GetMonitorW(hDlg, g_iCurDevice);
        g_iCurDevice = iDevice;

        if (hwndC)
            RedrawWindow(hwndC, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

        hwndC = GetMonitorW(hDlg, g_iCurDevice);
        RedrawWindow(hwndC, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

        GetDisplayDescription(iDevice, ach, ARRAYSIZE(ach));
        SetDlgItemText(hDlg, IDC_DISPLAYNAME, ach);

        fUseDevice = iDevice==1 || (GetWindowLong(hwndC, GWL_STYLE) & WS_VISIBLE);
        CheckDlgButton(hDlg, IDC_DISPLAYUSEME, fUseDevice);
        EnableWindow(GetDlgItem(hDlg, IDC_DISPLAYUSEME), iDevice != 1);

        GetModeName(&dm, ach);

#ifdef DEBUG
        wsprintf(ach + lstrlen(ach), TEXT(" [%d,%d,%d,%d]"),
            rc.left,
            rc.top,
            rc.right,
            rc.bottom);
#endif

        SetDlgItemText(hDlg, IDC_DISPLAYMODE, ach);
    }

    g_InSetInfo = FALSE;
}

//-----------------------------------------------------------------------------
void UpdateRects(HWND hDlg)
{
    int iDevice;
    DEVMODE dm;
    RECT rcNew;
    RECT rcOld;
    RECT rcWin;
    HWND hwndDesk;
    HWND hwndC;
    int  x,y;

    hwndDesk = GetDlgItem(hDlg, IDC_DISPLAYDESK);

    for (iDevice = 1; TRUE; iDevice++)
    {
        if (!GetDisplaySettings(iDevice, &dm, &rcOld))
            break;

        if ((hwndC = GetDlgItem(hwndDesk, iDevice)) &&
            (GetWindowLong(hwndC, GWL_STYLE) & WS_VISIBLE))
        {
            GetWindowRect(hwndC, &rcWin);
            MapWindowPoints(NULL, hwndDesk, (POINT FAR*)&rcWin, 2);
            PreviewToDesk(&rcWin, &rcNew);

            //
            // make sure we can create a DC for this device before we add it
            //
            if (IsRectEmpty(&rcOld))
            {
#ifdef WIN95_GDI_HACK
                HDC hdc = _GetDisplayIC(iDevice);

                if (hdc)
                {
                    _ReleaseDisplayIC(iDevice, hdc);
                    GetDisplaySettings(iDevice, &dm, &rcOld);
                }
                else
                {
                    SetRectEmpty(&rcNew);
                }
#else
                GetDisplaySettings(iDevice, &dm, &rcOld);
#endif
            }
        }
        else
        {
            SetRectEmpty(&rcNew);
        }
//
// BUGBUG normalize so \\.\Display1 is at (0,0)
// BUGBUG we should not need to assume this
//
        if (iDevice == 1)
        {
            x = rcNew.left - rcOld.left;
            y = rcNew.top  - rcOld.top;
        }

        OffsetRect(&rcNew, -x, -y);
        SetDisplaySettings(iDevice, &dm, &rcNew, CDS_NORESET | CDS_UPDATEREGISTRY);
    }

    hwndC = CreateCoverWindow(COVER_NOPAINT);
    ChangeDisplaySettings(NULL, 0);
    DestroyWindow(hwndC);
}

//-----------------------------------------------------------------------------
void InitMultiMonitorDlg(HWND hDlg)
{
    HWND    hwndList;
    HWND    hwndDesk;
    HWND    hwndC;
    int     iItem;
    int     iDevice;
    DEVMODE dm;
    RECT    rc;
    TCHAR    ach[128];
    DWORD   vis;

    WNDCLASS cls;

    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = TEXT("Monitor");
    cls.hbrBackground  = (HBRUSH)(COLOR_DESKTOP + 1);
    cls.hInstance      = hInstance;
    cls.style          = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    cls.lpfnWndProc    = (WNDPROC)MonitorWindowProc;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    RegisterClass(&cls);

#ifdef DEBUG
    if (!GetDisplaySettings(1, &dm, &rc))
    {
        MessageBox(hDlg, TEXT("this cpl needs a new USER"), TEXT("Error!"), MB_OK);
        return;
    }
#endif

    g_InSetInfo = TRUE;

    hwndDesk = GetDlgItem(hDlg, IDC_DISPLAYDESK);
    hwndList = GetDlgItem(hDlg, IDC_DISPLAYLIST);

    ListBox_ResetContent(hwndList);
    SetRectEmpty(&rcDesk);

    while (hwndC = GetWindow(hwndDesk, GW_CHILD))
        DestroyWindow(hwndC);

    ShowWindow(hwndDesk, SW_HIDE);

    for (iDevice = 1; TRUE; iDevice++)
    {
        if (!GetDisplaySettings(iDevice, &dm, &rc))
            break;

        GetDisplayDescription(iDevice, ach, ARRAYSIZE(ach));
        iItem = ListBox_AddString(hwndList, ach);
        ListBox_SetItemData(hwndList, iItem, (DWORD)iDevice);

        if (IsRectEmpty(&rc))
        {
            SetRect(&rc, 0, 0, (int)dm.dmPelsWidth, (int)dm.dmPelsHeight);
            vis = 0;
        }
        else
        {
            vis = WS_VISIBLE;
        }

        wsprintf(ach, TEXT("%d"), iDevice);
        hwndC = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("Monitor"),ach,
                WS_CLIPSIBLINGS | WS_DLGFRAME | vis | WS_CHILD,
                rc.left, rc.top, rc.right - rc.left, rc.bottom-rc.top,
                hwndDesk, (HMENU)iDevice, hInstance, NULL);
    }

    DeskScale = 1000;
    DeskOff.x = 0;
    DeskOff.y = 0;
    rcDesk.left = 0;
    rcDesk.top = 0;
    CleanupRects(hwndDesk);
    ShowWindow(hwndDesk, SW_SHOW);

    g_InSetInfo = FALSE;
    SetInfo(hDlg, 1);
}

//-----------------------------------------------------------------------------
BOOL CALLBACK
MultiMonitorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;

    switch (message)
    {
    case WM_INITDIALOG:
        InitMultiMonitorDlg(hDlg);
        break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch (lpnm->code)
        {
        case PSN_APPLY:
//          UpdateInfo(hDlg,-1);
            UpdateRects(hDlg);
            InitMultiMonitorDlg(hDlg);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_CTLCOLORSTATIC:
        if (GetDlgCtrlID((HWND)lParam) == IDC_DISPLAYDESK)
        {
            return (BOOL)(UINT)GetSysColorBrush(COLOR_APPWORKSPACE);
        }
        return FALSE;


    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DISPLAYUSEME:
            if (!g_InSetInfo)
            {
                HWND hwndC;

                hwndC = GetMonitorW(hDlg, g_iCurDevice);

                if (GetWindowLong(hwndC, GWL_STYLE) & WS_VISIBLE)
                    ShowWindow(hwndC, SW_HIDE);
                else
                    ShowWindow(hwndC, SW_SHOW);

                BringWindowToTop(hwndC);
                PropSheet_Changed(GetParent(hDlg), hDlg);
                CleanupRects(GetParent(hwndC));
            }
            break;

        case IDC_DISPLAYLIST:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case LBN_DBLCLK:
                goto DoDeviceSettings;

            case LBN_SELCHANGE:
                {
                    BOOL fAttached = FALSE;
                    HWND hwndList = GetDlgItem(hDlg, IDC_DISPLAYLIST);
                    int iSel = ListBox_GetCurSel(hwndList);

                    if (iSel >= 0)
                    {
                        int iDevice = (int)ListBox_GetItemData(hwndList, iSel);
                        SetInfo(hDlg, iDevice);
                    }
                }
                break;

            default:
                return FALSE;
            }
            break;

        case IDC_DISPLAYPROPERTIES:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            DoDeviceSettings:
            case BN_CLICKED:
                if (IsWindowEnabled(GetDlgItem(hDlg, IDC_DISPLAYPROPERTIES)))
                {
                    HWND hwndList = GetDlgItem(hDlg, IDC_DISPLAYLIST);
                    int iSel = ListBox_GetCurSel(hwndList);

                    if (iSel >= 0)
                    {
                        int iDevice = (int)ListBox_GetItemData(hwndList, iSel);
                        DoMonitorSettingsSheet(hDlg, iDevice);
                        InitMultiMonitorDlg(hDlg);
                    }
                }
                break;

            default:
                return FALSE;
            }
            break;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP,
            (DWORD)(LPTSTR)aMultiMonitorHelpIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU,
            (DWORD)(LPTSTR)aMultiMonitorHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

#ifdef WINNT
DWORD VDDCall(DWORD dev, DWORD function, DWORD flags, LPVOID buffer, DWORD buffer_size) {
    return 0xFFFFFFFF;
}
#else
#pragma optimize("gle", off)
DWORD VDDCall(DWORD dev, DWORD function, DWORD flags, LPVOID buffer, DWORD buffer_size)
{
    DWORD   VDDEntryPoint;
    DWORD   result=0xFFFFFFFF;

    _asm {
        _emit 66h _asm pusha    ; pushad
        xor     di,di           ;set these to zero before calling
        mov     es,di           ;
        mov     ax,1684h        ;INT 2FH: Get VxD API Entry Point
        mov     bx,0ah          ;this is device code for VDD
        int     2fh             ;call the multiplex interrupt
        mov     word ptr VDDEntryPoint[0],di    ;
        mov     word ptr VDDEntryPoint[2],es    ;save the returned data
        mov     ax,es
        or      ax,di           ;did we get back a NULL?
        jz      fail            ;yes, don't attempt PnP stuff!
        _emit 66h _asm mov ax,word ptr function        ;eax = function
        _emit 66h _asm mov bx,word ptr dev             ;ebx = device
        _emit 66h _asm mov cx,word ptr buffer_size     ;ecx = buffer_size
        _emit 66h _asm mov dx,word ptr flags           ;edx = flags
        _emit 66h _asm xor di,di                       ; HIWORD(edi)=0
        les     di,buffer
        call    dword ptr VDDEntryPoint     ;call the VDD's PM API
        cmp     ax,word ptr function
        je      fail
        _emit 66h _asm mov word ptr result,ax
fail:   _emit 66h _asm popa
    }
    return result;
}
#pragma optimize("", on)
#endif

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
BOOL GetDisplayInfo( LPTSTR psz, DWORD iDev ) {
    DISPLAY_DEVICEW dspdev;
    BOOL fRet;

#ifdef TEST_ON_ONE_MON
    if (iDev < 3)
        iDev = 1;
#endif

    ZeroMemory( &dspdev, sizeof(dspdev));
    dspdev.cb = sizeof(dspdev);

    fRet = EnumDisplayDevices( NULL, iDev, &dspdev, 0 );

    if (fRet && psz != NULL)
        lstrcpy(psz, dspdev.DeviceName);

    return fRet;
}

#if 0
DWORD GetDisplayDevNode(int dev)
{
    DWORD dn;
    TCHAR ach[40];
    wsprintf(ach, g_szDisplayX, dev);
    dn = VDDCall(0, VDD_OPEN, 0, (LPVOID)ach, ARRAYSIZE(ach));

    if (dn == 0xFFFFFFFF)
        dn = 0;

    if (dn != 0)
    {
        VDDCall(dn, VDD_CLOSE, 0, NULL, 0);
    }

    return dn;
}

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

DWORD GetMonitorDevNode(int dev)
{
    DWORD dn;
    DISPLAYINFO di;

    di.diMonitorDevNodeHandle = 0;

    if (dn = GetDisplayDevNode(dev))
    {
        GetDisplayInfo(dev, &di);
    }

    return di.diMonitorDevNodeHandle;
}

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

BOOL GetDisplayInfo(DWORD dev, DISPLAYINFO FAR *pdi)
{
    FillMemory(pdi,  sizeof(*pdi), 0);
    pdi->diHdrSize = sizeof(DISPLAYINFO);

    VDDCall(dev, VDD_GET_DISPLAY_CONFIG2, 0, (LPVOID)pdi, sizeof(DISPLAYINFO));

    if (pdi->diDevNodeHandle != 0)
        return TRUE;

    VDDCall(dev, VDD_GET_DISPLAY_CONFIG, 0, (LPVOID)pdi, sizeof(DISPLAYINFO));
    return pdi->diDevNodeHandle != 0;
}

#else
// remove the above function when we can get monitor info in a real way
#endif

HFONT GetFont(LPRECT prc)
{
    LOGFONT lf;

    FillMemory(&lf,  sizeof(lf), 0);
    lf.lfWeight = FW_EXTRABOLD;
    lf.lfHeight = prc->bottom - prc->top;
    lf.lfWidth  = 0;
    lf.lfPitchAndFamily = FF_SWISS;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;

    return CreateFontIndirect(&lf);
}

#if 0
void xxFlashText(HDC hdc, LPRECT prc, LPTSTR sz)
{
    HFONT hfont,hfontT;
    HBITMAP hbm;
    SIZE size;
    HDC hdcM;
    DWORD time0;
    POINT pt,pt0;

    GetCursorPos(&pt0);

    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        return;

    hfont = GetFont(prc);

    hdcM = CreateCompatibleDC(hdc);
    hfontT = SelectObject(hdcM, hfont);
    GetTextExtentPoint(hdcM, sz, lstrlen(sz), &size);
    hbm = CreateBitmap(size.cx,size.cy,1,1,NULL);
    SelectObject(hdcM, hbm);
    TextOut(hdcM, 0, 0, sz, lstrlen(sz));
    SelectObject(hdcM, hfontT);
    SetTextColor(hdc, 0xFFFFFF);
    SetBkColor(hdc, 0x000000);

    //
    // wait for a timeout or the mouse button to go up
    //
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        BitBlt(hdc, (prc->right  + prc->left - size.cx)/2,
                (prc->bottom + prc->top  - size.cy)/2,
                size.cx, size.cy, hdcM, 0, 0, SRCINVERT);

        time0 = GetTickCount();

        for (;;)
        {
            if (GetTickCount()-time0 > 2000)
                break;

            if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
                break;

            GetCursorPos(&pt);

            if ((pt.y - pt0.y) > 2 || (pt.y - pt0.y) < -2)
                break;

            if ((pt.x - pt0.x) > 2 || (pt.x - pt0.x) < -2)
                break;
        }

        BitBlt(hdc, (prc->right  + prc->left - size.cx)/2,
            (prc->bottom + prc->top  - size.cy)/2,
            size.cx, size.cy, hdcM, 0, 0, SRCINVERT);
    }

    DeleteDC(hdcM);
    DeleteObject(hbm);
    DeleteObject(hfont);
}
#endif

BOOL Bail(DWORD time0, POINT pt0)
{
    POINT pt;

    if (GetTickCount()-time0 > 2000)
        return TRUE;

    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
        return TRUE;

    GetCursorPos(&pt);

    if ((pt.y - pt0.y) > 2 || (pt.y - pt0.y) < -2)
        return TRUE;

    if ((pt.x - pt0.x) > 2 || (pt.x - pt0.x) < -2)
        return TRUE;

    return FALSE;
}

void FlashText(HDC hdc, LPRECT prc, LPTSTR sz)
{
    HFONT hfont,hfontT;
    HBITMAP hbm;
    SIZE size;
    HDC hdcM;
    DWORD time0;
    POINT pt0;

    time0 = GetTickCount();
    GetCursorPos(&pt0);

    if (Bail(time0, pt0))
        return;

    hfont = GetFont(prc);

    hfontT = SelectObject(hdc, hfont);
    GetTextExtentPoint(hdc, sz, lstrlen(sz), &size);

    hdcM = CreateCompatibleDC(hdc);
    hbm = CreateCompatibleBitmap(hdc,size.cx,size.cy);
    SelectObject(hdcM, hbm);

    BitBlt(hdcM, 0, 0, size.cx, size.cy, hdc,
            (prc->right  + prc->left - size.cx)/2,
            (prc->bottom + prc->top  - size.cy)/2,
            SRCCOPY);

    if (Bail(time0, pt0))
        goto exit;

    //
    // wait for a timeout or the mouse button to go up
    //
    if (!Bail(time0, pt0))
    {
        SetTextColor(hdc, 0x000000);
        SetBkMode(hdc, TRANSPARENT);
        TextOut(hdc,
                (prc->right  + prc->left - size.cx)/2 + 2,
                (prc->bottom + prc->top  - size.cy)/2 + 2,
                sz, lstrlen(sz));

        SetTextColor(hdc, 0xFFFFFF);
        TextOut(hdc,
                (prc->right  + prc->left - size.cx)/2,
                (prc->bottom + prc->top  - size.cy)/2,
                sz, lstrlen(sz));

        while (!Bail(time0, pt0))
            ;

        BitBlt(hdc,
                (prc->right  + prc->left - size.cx)/2,
                (prc->bottom + prc->top  - size.cy)/2,
                size.cx, size.cy, hdcM, 0, 0,
                SRCCOPY);
    }

exit:
    SelectObject(hdc, hfontT);
    DeleteDC(hdcM);
    DeleteObject(hbm);
    DeleteObject(hfont);
}

void CleanupRects(HWND hwndP)
{
    HWND hwndC;
    RECT rc;
    RECT rcU;
    RECT arc[20];
    int i,n;
    RECT rcPrev;
    int sx,sy;
    int x,y;

    //
    // get the positions of all the windows
    //
    for (n=0,i=1; hwndC=GetDlgItem(hwndP, i); i++)
    {
        if (GetWindowLong(hwndC, GWL_STYLE) & WS_VISIBLE)
        {
            GetWindowRect(hwndC, &arc[n]);
            MapWindowPoints(NULL, hwndP, (POINT FAR*)&arc[n], 2);
            PreviewToDesk(&arc[n], &arc[n]);
            n++;
        }
    }

    //
    // cleanup the rects
    //
    CleanUpDesktopRectangles(arc, n, CUDR_NORMAL);

    //
    // get union
    //
    SetRectEmpty(&rcU);
    for (i=0; i<n; i++)
    {
        UnionRect(&rcU, &rcU, &arc[i]);
    }

    GetClientRect(hwndP, &rcPrev);

    //
    // only rescale if the new desk hangs outside the preview area.
    // or is too small
    //
    DeskToPreview(&rcU, &rc);
    x = ((rcPrev.right  - rcPrev.left)-(rc.right  - rc.left))/2;
    y = ((rcPrev.bottom - rcPrev.top) -(rc.bottom - rc.top))/2;

    if (rcU.left < 0 || rcU.top < 0 || x < 0 || y < 0 ||
        rcU.right > rcPrev.right || rcU.bottom > rcPrev.bottom ||
        (x > (rcPrev.right-rcPrev.left)/8 &&
         y > (rcPrev.bottom-rcPrev.top)/8))
    {
        rcDesk = rcU;
        sx = MulDiv(rcPrev.right  - rcPrev.left - 16,1000,rcDesk.right  - rcDesk.left);
        sy = MulDiv(rcPrev.bottom - rcPrev.top  - 16,1000,rcDesk.bottom - rcDesk.top);

        DeskScale = min(sx,sy);
        DeskToPreview(&rcDesk, &rc);
        DeskOff.x = ((rcPrev.right  - rcPrev.left)-(rc.right  - rc.left))/2;
        DeskOff.y = ((rcPrev.bottom - rcPrev.top) -(rc.bottom - rc.top))/2;
    }

    for (n=0,i=1; hwndC=GetDlgItem(hwndP, i); i++)
    {
        DEVMODE dm;
        UINT swp;

        GetDisplaySettings(i, &dm, &rc);

        if (GetWindowLong(hwndC, GWL_STYLE) & WS_VISIBLE)
        {
            swp = 0;
            DeskToPreview(&arc[n], &rc);
            n++;
        }
        else
        {
            swp = SWP_NOMOVE;
        }

        SetWindowPos(hwndC, NULL, rc.left, rc.top,
            MulDiv((int)dm.dmPelsWidth, DeskScale, 1000),
            MulDiv((int)dm.dmPelsHeight, DeskScale, 1000),
            SWP_NOZORDER | swp);
    }
}

//#define WNDCLASS_TRAYNOTIFY     "Shell_TrayWnd"

LONG CALLBACK MonitorWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    TCHAR ach[80];
    int iDevice;
    DEVMODE dm;
    HWND hDlg;
    HWND hwndTray;
    HFONT hfont,hfontT;
    COLORREF rgb;
    COLORREF rgbDesk;

    switch (msg)
    {
        case WM_NCHITTEST:
            return HTCAPTION;

        case WM_NCLBUTTONDBLCLK:
            hDlg = GetParent(GetParent(hwnd));
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DISPLAYPROPERTIES, BN_CLICKED), (LPARAM)hwnd );
            break;

        case WM_NCLBUTTONDOWN:
            BringWindowToTop(hwnd);

            hDlg = GetParent(GetParent(hwnd));
            iDevice = GetDlgCtrlID(hwnd);
            SetInfo(hDlg, iDevice);

            GetDisplaySettings(iDevice, &dm, &rc);

            if (!IsRectEmpty(&rc))
            {
                hdc = GetDC(NULL);
                GetWindowText(hwnd, ach, ARRAYSIZE(ach));
                FlashText(hdc, &rc, ach);
                ReleaseDC(NULL, hdc);
            }
            break;

        case WM_ENTERSIZEMOVE:
            break;

        case WM_EXITSIZEMOVE:
            if (!g_InSetInfo)
            {
                CleanupRects(GetParent(hwnd));

                hDlg = GetParent(GetParent(hwnd));
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }
	    break;

	case WM_DESTROY:
	    break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd,&ps);
            GetClientRect(hwnd, &rc);
            GetWindowText(hwnd, ach, ARRAYSIZE(ach));

            iDevice = GetDlgCtrlID(hwnd);

            if (iDevice != g_iCurDevice)
            {
                rgb = GetSysColor(COLOR_CAPTIONTEXT);
                rgbDesk = GetSysColor(COLOR_DESKTOP);
                FillRect(hdc, &rc, GetSysColorBrush(COLOR_DESKTOP));
            }
            else
            {
                rgb = GetSysColor(COLOR_CAPTIONTEXT);
                rgbDesk = GetSysColor(COLOR_DESKTOP);
                FillRect(hdc, &rc, GetSysColorBrush(COLOR_ACTIVECAPTION));
            }

            if (rgbDesk == rgb)
                rgb = GetSysColor(COLOR_WINDOWTEXT);

            if (rgbDesk == rgb)
                rgb = rgbDesk ^ 0x00FFFFFF;

            SetTextColor(hdc, rgb);

            hfont = GetFont(&rc);
            hfontT = SelectObject(hdc, hfont);
            SetTextAlign(hdc, TA_CENTER | TA_TOP);
            SetBkMode(hdc, TRANSPARENT);
            ExtTextOut(hdc, (rc.left+rc.right)/2, rc.top, 0, NULL, ach, lstrlen(ach), NULL);
            SelectObject(hdc, hfontT);
            DeleteObject(hfont);

            hwndTray = FindWindow(TEXT(WNDCLASS_TRAYNOTIFY), NULL);

            if (hwndTray)
            {
#if 0
                RECT rcTray;
                GetWindowRect(hwndTray, &rcTray);
                DeskToPreview(&rcTray, &rcTray);
                MapWindowPoints(GetParent(hwnd), hwnd, (POINT FAR*)&rcTray, 2);
                FillRect(hdc, &rcTray, GetSysColorBrush(COLOR_3DFACE));
#endif
            }

            EndPaint(hwnd,&ps);
            return 0L;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

//BUGBUG should we export this from USER???????

#define RectCenterX(prc)    ((prc)->left+((prc)->right-(prc)->left)/2)
#define RectCenterY(prc)    ((prc)->top+((prc)->bottom-(prc)->top)/2)
#define RectWidth(prc)      ((prc)->right-(prc)->left)
#define RectHeight(prc)     ((prc)->bottom-(prc)->top)

void CopyOffsetRect(LPRECT out, LPRECT in, int x, int y)
{
    *out = *in;
    OffsetRect(out, x, y);
}

// ----------------------------------------------------------------------------
//
//  INTERSECTION_AXIS()
//      This macro tells us how a particular set of overlapping rectangles
//  should be adjusted to remove the overlap.  It is basically a condensed
//  version of a lookup table that does the same job.  The parameters for the
//  macro are two rectangles, where one is the intersection of the other with
//  a third (unspecified) rectangle.  The macro compares the edges of the
//  rectangles to determine which sides of the intersection were "caused" by
//  the source rectangle.  In the pre-condensed version of this macro, the
//  results of these comparisons (4 bits) would be used to index into a 16
//  entry table which specifies the way to resolve the overlap.  However, this
//  is highly redundant, as the table would actually represents several rotated
//  and/or inverted instances of a few basic relationships:
//
//  Horizontal Vertical  Diagonal  Contained       Crossing
//      *--*    *-----*   *---*     *-----*         *----*
//   *--+* |    | *-* |   | *-+-*   | *-* |       *-+----+-*
//   |  || |    *-+-+-*   | | | |   | | | |  and  | |    | |
//   *--+* |      | |     *-+-* |   | *-* |       *-+----+-*
//      *--*      *-*       *---*   *-----*         *----*
//
//  What we are really interested in determining is whether we "should" move
//  the rectangles horizontally or vertically to resolve the overlap, hence we
//  are testing for three states: Horizontal, Vertical and Don't Know.
//
//  The macro gives us these three states by XORing the high and low bits of
//  of the comparison to reduce the table to 4 cases where 1 and 2 are
//  vertical and horizontal respectively, and then subtracting 1 so that the
//  2 bit signifies "unknown-ness."
//
//  Note that there are some one-off cases in the comparisons because we are
//  not actually looking at the third rectangle.  However this greatly reduces
//  the complexity so these small errors are acceptible given the scale of the
//  rectangles we are comparing.
//
// ----------------------------------------------------------------------------
#define INTERSECTION_AXIS(a, b) \
    (((((a->left == b->left) << 1) | (a->top == b->top)) ^ \
    (((a->right == b->right) << 1) | (a->bottom == b->bottom))) - 1)

#define INTERSECTION_AXIS_VERTICAL      (0)
#define INTERSECTION_AXIS_HORIZONTAL    (1)
#define INTERSECTION_AXIS_UNKNOWN(code) (code & 2)

// ----------------------------------------------------------------------------
//
//  OffsetRectangles()
//
// ----------------------------------------------------------------------------
void NEAR PASCAL OffsetRectangles(LPRECT arc, UINT count, int x, int y)
{
    if (x || y)
    {
        LPPOINT ppt = (LPPOINT)arc;

        count *= 2;
        while (count--)
        {
            ppt->x += x;
            ppt->y += y;
            ppt++;
        }
    }
}

// ----------------------------------------------------------------------------
//
//  CenterRectangles()
//      Move all the rectangles so their origin is the center of their union.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL CenterRectangles(LPRECT arc, UINT count)
{
    LPRECT lprc, lprcL;
    RECT rcUnion;

    CopyRect(&rcUnion, arc);

    lprcL = arc + count;
    for (lprc = arc + 1; lprc < lprcL; lprc++)
        UnionRect(&rcUnion, &rcUnion, lprc);

    OffsetRectangles(arc, count,
        -RectCenterX(&rcUnion), -RectCenterY(&rcUnion));
}

// ----------------------------------------------------------------------------
//
//  RemoveOverlap()
//    This is called from RemoveOverlaps to resolve conflicts when two
//  rectangles overlap.  It returns the PMONITOR for the monitor it decided to
//  move.  This routine always moves rectangles away from the origin so it can
//  be used to converge on a zero-overlap configuration.
//
//  This function will bias slightly toward moving lprc2 (all other things
//  being equal).
//
// ----------------------------------------------------------------------------
LPRECT NEAR PASCAL RemoveOverlap(LPRECT lprc1, LPRECT lprc2, LPRECT lprcI)
{
    LPRECT lprcMove, lprcStay;
    POINT ptC1, ptC2;
    BOOL fNegative;
    BOOL fC1Neg;
    BOOL fC2Neg;
    int dC1, dC2;
    int xOffset;
    int yOffset;
    int nAxis;

    //
    // Compute the centers of both rectangles.  We will need them later.
    //
    ptC1.x = RectCenterX(lprc1);
    ptC1.y = RectCenterY(lprc1);
    ptC2.x = RectCenterX(lprc2);
    ptC2.y = RectCenterY(lprc2);

    //
    // Decide whether we should move things horizontally or vertically.  All
    // this goop is here so it will "feel" right when the system needs to
    // move a monitor on you.
    //
    nAxis = INTERSECTION_AXIS(lprcI, lprc1);

    if (INTERSECTION_AXIS_UNKNOWN(nAxis))
    {
        //
        // Is this a "big" intersection between the two rectangles?
        //
        if (PtInRect(lprcI, ptC1) || PtInRect(lprcI, ptC2))
        {
            //
            // This is a "big" overlap.  Decide if the rectangles
            // are aligned more "horizontal-ish" or "vertical-ish."
            //
            xOffset = ptC1.x - ptC2.x;
            if (xOffset < 0)
                xOffset *= -1;
            yOffset = ptC1.y - ptC2.y;
            if (yOffset < 0)
                yOffset *= -1;

            if (xOffset >= yOffset)
                nAxis = INTERSECTION_AXIS_HORIZONTAL;
            else
                nAxis = INTERSECTION_AXIS_VERTICAL;
        }
        else
        {
            //
            // This is a "small" overlap.  Move the rectangles the
            // smallest distance that will fix the overlap.
            //
            if ((lprcI->right - lprcI->left) <= (lprcI->bottom - lprcI->top))
                nAxis = INTERSECTION_AXIS_HORIZONTAL;
            else
                nAxis = INTERSECTION_AXIS_VERTICAL;
        }
    }

    //
    // We now need to pick the rectangle to move.  Move the one
    // that is further from the origin along the axis of motion.
    //
    if (nAxis == INTERSECTION_AXIS_HORIZONTAL)
    {
        dC1 = ptC1.x;
        dC2 = ptC2.x;
    }
    else
    {
        dC1 = ptC1.y;
        dC2 = ptC2.y;
    }

    if ((fC1Neg = (dC1 < 0)) != 0)
        dC1 *= -1;

    if ((fC2Neg = (dC2 < 0)) != 0)
        dC2 *= -1;

    if (dC2 < dC1)
    {
        lprcMove     = lprc1;
        lprcStay     = lprc2;
        fNegative    = fC1Neg;
    }
    else
    {
        lprcMove     = lprc2;
        lprcStay     = lprc1;
        fNegative    = fC2Neg;
    }

    //
    // Compute a new home for the rectangle and put it there.
    //
    if (nAxis == INTERSECTION_AXIS_HORIZONTAL)
    {
        int xPos;

        if (fNegative)
            xPos = lprcStay->left - (lprcMove->right - lprcMove->left);
        else
            xPos = lprcStay->right;

        xOffset = xPos - lprcMove->left;
        yOffset = 0;
    }
    else
    {
        int yPos;

        if (fNegative)
            yPos = lprcStay->top - (lprcMove->bottom - lprcMove->top);
        else
            yPos = lprcStay->bottom;

        yOffset = yPos - lprcMove->top;
        xOffset = 0;
    }

    OffsetRect(lprcMove, xOffset, yOffset);
    return lprcMove;
}

// ----------------------------------------------------------------------------
//
//  RemoveOverlaps()
//    This is called from CleanupDesktopRectangles make sure the monitor array
//  is non-overlapping.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RemoveOverlaps(LPRECT arc, UINT count)
{
    LPRECT lprc1, lprc2, lprcL;

    //
    // Center the rectangles around a common origin.  We will move them outward
    // when there are conflicts so centering (a) reduces running time and
    // hence (b) reduces the chances of totally mangling the positions.
    //
    CenterRectangles(arc, count);

    //
    // Now loop through the array fixing any overlaps.
    //
    lprcL = arc + count;
    lprc2 = arc + 1;

ReScan:
    while (lprc2 < lprcL)
    {
        //
        // Scan all rectangles before this one looking for intersections.
        //
        for (lprc1 = arc; lprc1 < lprc2; lprc1++)
        {
            RECT rcI;

            //
            // Move one of the rectanges if there is an intersection.
            //
            if (IntersectRect(&rcI, lprc1, lprc2))
            {
                //
                // Move one of the rectangles out of the way and then restart
                // the scan for overlaps with that rectangle (since moving it
                // may have created new overlaps).
                //
                lprc2 = RemoveOverlap(lprc1, lprc2, &rcI);
                goto ReScan;
            }
        }

        lprc2++;
    }
}

// ----------------------------------------------------------------------------
//
//  AddNextContiguousRectangle()
//    This is called from RemoveGaps to find the next contiguous rectangle
//  in the array.  If there are no more contiguous rectangles it picks the
//  closest rectangle and moves it so it is contiguous.
//
// ----------------------------------------------------------------------------
LPRECT FAR * NEAR PASCAL AddNextContiguousRectangle(LPRECT FAR *aprc,
    LPRECT FAR *pprcSplit, UINT count)
{
    LPRECT FAR *pprcL;
    LPRECT FAR *pprcTest;
    LPRECT FAR *pprcAxis;
    LPRECT FAR *pprcDiag;
    UINT dAxis = (UINT)-1;
    UINT dDiag = (UINT)-1;
    POINT dpAxis;
    POINT dpDiag;
    POINT dpMove;

    pprcL = aprc + count;

    for (pprcTest = aprc; pprcTest < pprcSplit; pprcTest++)
    {
        LPRECT lprcTest = *pprcTest;
        LPRECT FAR *pprcScan;

        for (pprcScan = pprcSplit; pprcScan < pprcL; pprcScan++)
        {
            RECT rcCheckOverlap;
            LPRECT lprcScan = *pprcScan;
            LPRECT FAR *pprcCheckOverlap;
            LPRECT FAR *FAR *pppBest;
            LPPOINT pdpBest;
            UINT FAR *pdBest;
            UINT dX, dY;
            UINT dTotal;

            //
            // Figure out how far the rectangle may be along both axes.
            // Note some of these numbers could be garbage at this point but
            // the code below will take care of it.
            //
            if (lprcScan->right <= lprcTest->left)
                dpMove.x = dX = lprcTest->left - lprcScan->right;
            else
                dpMove.x = -(int)(dX = (lprcScan->left - lprcTest->right));

            if (lprcScan->bottom <= lprcTest->top)
                dpMove.y = dY = lprcTest->top - lprcScan->bottom;
            else
                dpMove.y = -(int)(dY = (lprcScan->top - lprcTest->bottom));

            //
            // Figure out whether the rectangles are vertical, horizontal or
            // diagonal to each other and pick the measurements we will test.
            //
            if ((lprcScan->top < lprcTest->bottom) &&
                (lprcScan->bottom > lprcTest->top))
            {
                // The rectangles are somewhat horizontally aligned.
                dpMove.y = dY = 0;
                pppBest = &pprcAxis;
                pdpBest = &dpAxis;
                pdBest = &dAxis;
            }
            else if ((lprcScan->left < lprcTest->right) &&
                (lprcScan->right > lprcTest->left))
            {
                // The rectangles are somewhat vertically aligned.
                dpMove.x = dX = 0;
                pppBest = &pprcAxis;
                pdpBest = &dpAxis;
                pdBest = &dAxis;
            }
            else
            {
                // The rectangles are somewhat diagonally aligned.
                pppBest = &pprcDiag;
                pdpBest = &dpDiag;
                pdBest = &dDiag;
            }

            //
            // Make sure there aren't other rectangles in the way.  We only
            // need to check the upper array since that is the pool of
            // semi-placed rectangles.  Any rectangles in the lower array that
            // are "in the way" will be found in a different iteration of the
            // enclosing loop.
            //
            CopyOffsetRect(&rcCheckOverlap, lprcScan, dpMove.x, dpMove.y);
            for (pprcCheckOverlap = pprcScan + 1; pprcCheckOverlap < pprcL;
                pprcCheckOverlap++)
            {
                RECT rc;
                if (IntersectRect(&rc, *pprcCheckOverlap, &rcCheckOverlap))
                    break;
            }
            if (pprcCheckOverlap < pprcL)
            {
                // There was another rectangle in the way; don't use this one.
                continue;
            }

            //
            // If it is closer than the one we already had, use it instead.
            //
            dTotal = dX + dY;
            if (dTotal < *pdBest)
            {
                *pdBest = dTotal;
                *pdpBest = dpMove;
                *pppBest = pprcScan;
            }
        }
    }

    //
    // If we found anything along an axis use that otherwise use a diagonal.
    //
    if (dAxis != (UINT)-1)
    {
        pprcSplit = pprcAxis;
        dpMove = dpAxis;
    }
    else if (dDiag != (UINT)-1)
    {
        // BUGBUG: consider moving the rectangle to a side in this case.
        // (that, of course would add a lot of code to avoid collisions)
        pprcSplit = pprcDiag;
        dpMove = dpDiag;
    }
    else
        dpMove.x = dpMove.y = 0;

    //
    // Move the monitor into place and return it as the one we chose.
    //
    if (dpMove.x || dpMove.y)
        OffsetRect(*pprcSplit, dpMove.x, dpMove.y);

    return pprcSplit;
}

// ----------------------------------------------------------------------------
//
//  RemoveGaps()
//    This is called from CleanupDesktopRectangles to make sure the monitor
//  array is contiguous.  It assumes that the array is already non-overlapping.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RemoveGaps(LPRECT arc, UINT count)
{
    LPRECT aprc[MONITORS_MAX];
    LPRECT lprc, lprcL, lprcSwap, FAR *pprc, FAR *pprcNearest;
    UINT uNearest;

    //
    // We will need to find the rectangle closest to the center of the group.
    // We don't really need to center the array here but it doesn't hurt and
    // saves us some code below.
    //
    CenterRectangles(arc, count);

    //
    // Build an array of LPRECTs we can shuffle around with relative ease while
    // not disturbing the order of the passed array.  Also take note of which
    // one is closest to the center so we start with it and pull the rest of
    // the rectangles inward.  This can make a big difference in placement when
    // there are more than 2 rectangles.
    //
    uNearest = (UINT)-1;
    pprc = aprc;
    lprcL = (lprc = arc) + count;

    while (lprc < lprcL)
    {
        int x, y;
        UINT u;

        //
        // Fill in the array.
        //
        *pprc = lprc;

        //
        // Check if this one is closer to the center of the group.
        //
        x = RectCenterX(lprc);
        y = RectCenterY(lprc);
        if (x < 0) x *= -1;
        if (y < 0) y *= -1;

        u = (UINT)x + (UINT)y;
        if (u < uNearest)
        {
            uNearest    = u;
            pprcNearest = pprc;
        }

        pprc++;
        lprc++;
    }

    //
    // Now make sure we move everything toward the centermost rectangle.
    //
    if (pprcNearest != aprc)
    {
        lprcSwap     = *pprcNearest;
        *pprcNearest = *aprc;
        *aprc        = lprcSwap;
    }

    //
    // Finally, loop through the array closing any gaps.
    //
    pprc = aprc + 1;
    for (lprc = arc + 1; lprc < lprcL; pprc++, lprc++)
    {
        //
        // Find the next suitable rectangle to combine into the group and move
        // it into position.
        //
        pprcNearest = AddNextContiguousRectangle(aprc, pprc, count);

        //
        // If the rectangle that was added is not the next in our array, swap.
        //
        if (pprcNearest != pprc)
        {
            lprcSwap     = *pprcNearest;
            *pprcNearest = *pprc;
            *pprc        = lprcSwap;
        }
    }
}

// ----------------------------------------------------------------------------
//
//  CleanUpDesktopRectangles()
//    This is called by CleanUpMonitorRectangles (etc) to force a set of
//  rectangles into a contiguous, non-overlapping arrangement.
//
// ----------------------------------------------------------------------------
void NEAR PASCAL CleanUpDesktopRectangles(LPRECT arc, UINT count, UINT flags)
{
    //
    // We don't need to get all worked up if there is only one rectangle.
    //
    if (count > 1)
    {
        LPRECT lprc, lprcL;
    
        //
        // Limit for loops.
        //
        lprcL = arc + count;

        if (!(flags & CUDR_NOSNAPTOGRID))
        {
            //
            // Align monitors on 8 pixel boundaries so GDI can use the same
            // brush realization on compatible devices (BIG performance win).
            // Note that we assume the size of a monitor will be in multiples
            // of 8 pixels on X and Y.  We cannot do this for the work areas so
            // we convert them to be relative to the origins of their monitors
            // for the time being.
            //
            // The way we do this alignment is to just do the overlap/gap
            // resoluton in 8 pixel space (ie divide everything by 8 beforehand
            // and multiply it by 8 afterward).
            //
            // Note: WE CAN'T USE MULTDIV HERE because it introduces one-off
            // errors when monitors span the origin.  These become eight-off
            // errors when we scale things back up and we end up trying to
            // create DCs with sizes like 632x472 etc (not too good).  It also
            // handles rounding the wierdly in both positive and negative space
            // and we just want to snap things to a grid so we compensate for
            // truncation differently here.
            //
            for (lprc = arc; lprc < lprcL; lprc++)
            {
                RECT rc;
                int d;

                CopyRect(&rc, lprc);

                d = rc.right - rc.left;

                if (rc.left < 0)
                    rc.left -= 4;
                else
                    rc.left += 3;

                rc.left /= 8;
                rc.right = rc.left + (d / 8);

                d = rc.bottom - rc.top;

                if (rc.top < 0)
                    rc.top -= 4;
                else
                    rc.top += 3;

                rc.top /= 8;
                rc.bottom = rc.top + (d / 8);

                CopyRect(lprc, &rc);
            }
        }

        //
        // RemoveGaps is designed assuming that none of the rectangles that it
        // is passed will overlap.  Thus we cannot safely call it if we have
        // skipped the call to RemoveOverlaps or it might loop forever.
        //
        if (!(flags & CUDR_NORESOLVEPOSITIONS))
        {
            RemoveOverlaps(arc, count);

            if (!(flags & CUDR_NOCLOSEGAPS))
                RemoveGaps(arc, count);
        }

        if (!(flags & CUDR_NOSNAPTOGRID))
        {
            //
            // Now return the monitor rectangles to pixel units this is a
            // simple multiply and MultDiv doesn't offer us any code size
            // advantage so (I guess that assumes a bit about the compiler,
            // but...) just do it right here.
            //
            for (lprc = arc; lprc < lprcL; lprc++)
            {
                lprc->left  *= 8;   lprc->top    *= 8;
                lprc->right *= 8;   lprc->bottom *= 8;
            }
        }
    }

    //
    // Make sure the first rectangle is at (0,0).  There's no point in offering
    // an option to skip this step because the rectangles get bounced around
    // so much by the RemoveX functions that they have already lost their
    // initial origin.  The caller will just have to deal with this if they
    // really care.
    //
    OffsetRectangles(arc, count, -(arc->left), -(arc->top));
}
