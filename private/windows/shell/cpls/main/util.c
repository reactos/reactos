/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    util.c

Abstract:

    This module contains the utility routines for this project.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "util.h"
#include "rc.h"

//
// From shell\inc\shsemip.h
//
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


//
//  Global Variables.
//

extern HINSTANCE g_hInst;





////////////////////////////////////////////////////////////////////////////
//
//  HourGlass
//
////////////////////////////////////////////////////////////////////////////

void HourGlass(
    BOOL fOn)
{
   if (!GetSystemMetrics(SM_MOUSEPRESENT))
   {
      ShowCursor(fOn);
   }
   SetCursor(LoadCursor(NULL, (fOn ? IDC_WAIT : IDC_ARROW)));
}


////////////////////////////////////////////////////////////////////////////
//
//  MyMessageBox
//
////////////////////////////////////////////////////////////////////////////

#ifdef WINNT

int MyMessageBox(
    HWND hWnd,
    UINT uText,
    UINT uCaption,
    UINT uType,
    ...)
{
    TCHAR szText[4 * PATHMAX], szCaption[2 * PATHMAX];
    int result;
    va_list parg;

    va_start(parg, uType);

    LoadString(g_hInst, uText, szCaption, ARRAYSIZE(szCaption));

    wvsprintf(szText, szCaption, parg);

    LoadString(g_hInst, uCaption, szCaption, ARRAYSIZE(szCaption));

    result = MessageBox(hWnd, szText, szCaption, uType);

    va_end(parg);

    return (result);
}

#else

int MyMessageBox(
    HWND hWnd,
    UINT uText,
    UINT uCaption,
    UINT uType,
    ...)
{
    TCHAR szText[256 + PATHMAX], szCaption[256];
    int result;


    LoadString(g_hInst, uText, szCaption, ARRAYSIZE(szCaption));

    wvsprintf(szText, szCaption, (LPTSTR)(&uType + 1));

    LoadString(g_hInst, uCaption, szCaption, ARRAYSIZE(szCaption));

    result = MessageBox(hWnd, szText, szCaption, uType);

    return (result);
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  TrackInit
//
////////////////////////////////////////////////////////////////////////////

void TrackInit(
    HWND hwndScroll,
    int nCurrent,
    PARROWVSCROLL pAVS)
{
    SendMessage(hwndScroll, TBM_SETRANGE, 0, MAKELONG(pAVS->bottom, pAVS->top));
    SendMessage(hwndScroll, TBM_SETPOS, TRUE, (LONG)nCurrent);
}


////////////////////////////////////////////////////////////////////////////
//
//  TrackMessage
//
////////////////////////////////////////////////////////////////////////////

int TrackMessage(
    WPARAM wParam,
    LPARAM lParam,
    PARROWVSCROLL pAVS)
{
    return ((int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0L));
}

////////////////////////////////////////////////////////////////////////////
//
//  HardwareDlg_OnInitDialog
//
//  Load the real hardware tab out of devmgr.dll.
//
//  DWLP_USER     - HWND of inner page
//
////////////////////////////////////////////////////////////////////////////

// There is no devmgr.h (go figure) so we must declare it ourselves.

EXTERN_C DECLSPEC_IMPORT HWND STDAPICALLTYPE
DeviceCreateHardwarePage(HWND hwndParent, const GUID *pguid);

void
HardwareDlg_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PCHWPAGEINFO phpi = (PCHWPAGEINFO)((LPPROPSHEETPAGE)lp)->lParam;
    HWND hwndHW = DeviceCreateHardwarePage(hdlg, &phpi->guidClass);

    if (hwndHW) {
        TCHAR tszTshoot[MAX_PATH];
        SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)hwndHW);
        if (LoadString(g_hInst, phpi->idsTshoot, tszTshoot, ARRAYSIZE(tszTshoot))) {
            SetWindowText(hwndHW, tszTshoot);
        }
    } else {
        DestroyWindow(hdlg); // catastrophic failure
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  HardwareDlg
//
//  The dialog procedure for generic hardware tabs.
//
//  GWLP_USERDATA - HINSTANCE of devmgr.dll
//  DWLP_USER     - HWND of inner page
//
//
////////////////////////////////////////////////////////////////////////////

typedef HWND (WINAPI *DEVICECREATEHARDWAREPAGE)
        (HWND hwndParent, const GUID *pguid);

INT_PTR CALLBACK HardwareDlg(HWND hdlg, UINT uMsg, WPARAM wp, LPARAM lp)
{
    switch (uMsg) {

    case WM_INITDIALOG:
        HardwareDlg_OnInitDialog(hdlg, lp);
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//  CreateHardwarePage
//
////////////////////////////////////////////////////////////////////////////

HPROPSHEETPAGE
CreateHardwarePage(PCHWPAGEINFO phpi)
{
    PROPSHEETPAGE psp;

    // No hardware tab if we are a remote TS session, since we would end
    // up showing the properties of the server, not the client, and that
    // would be too confusing.
    if (GetSystemMetrics(SM_REMOTESESSION))
        return NULL;

    // No hardware tab if it's been disabled via policy.
    if (SHRestricted(REST_NOHARDWARETAB))
        return NULL;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = g_hInst;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_HARDWARE);
    psp.pfnDlgProc  = HardwareDlg;
    psp.lParam      = (LPARAM)phpi;

    return CreatePropertySheetPage(&psp);
}
