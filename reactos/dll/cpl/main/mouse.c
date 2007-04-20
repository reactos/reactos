/*
 *  ReactOS
 *  Copyright (C) 2004, 2007 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * PROJECT:         ReactOS Main Control Panel
 * FILE:            lib/cpl/main/mouse.c
 * PURPOSE:         Mouse Control Panel
 * PROGRAMMER:      Eric Kohl
 *                  Johannes Anderwald
 */

//TODO:
//detect slider changes - for apply stuff
//          cursor icon shows - may need overriden items
//			implement Pointer-APPLY
//			implement Pointer-Browser
//			add missing icons
//			Options- pointer precision

#define WINVER 0x0501

#include <windows.h>
#include <winuser.h>
#include <devguid.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <math.h>
#include <limits.h>

#include <stdio.h>

#include "main.h"
#include "resource.h"

#define DEFAULT_DOUBLE_CLICK_SPEED	500
#define DEFAULT_CLICK_LOCK_TIME		2200
#define DEFAULT_MOUSE_SENSITIVITY	16
#define DEFAULT_MOUSE_THRESHOLD1	6
#define DEFAULT_MOUSE_THRESHOLD2	10
#define MIN_DOUBLE_CLICK_SPEED		200
#define MAX_DOUBLE_CLICK_SPEED		900
#define DEFAULT_WHEEL_SCROLL_LINES	3


typedef struct _BUTTON_DATA
{
    ULONG g_SwapMouseButtons;
    ULONG g_OrigSwapMouseButtons;
    ULONG g_DoubleClickSpeed; // = DEFAULT_DOUBLE_CLICK_SPEED;
    ULONG g_OrigDoubleClickSpeed;
    BOOL g_ClickLockEnabled; // = 0;
    DWORD g_ClickLockTime; // = DEFAULT_CLICK_LOCK_TIME;

    HICON hButtonLeft;
    HICON hButtonRight;

    HICON hIcon1;
    HICON hIcon2;
    BOOL bClicked;
} BUTTON_DATA, *PBUTTON_DATA;


typedef struct _POINTER_DATA
{
    BOOL bDropShadow;
    BOOL bOrigDropShadow;

} POINTER_DATA, *PPOINTER_DATA;


typedef struct _OPTION_DATA
{
    ULONG ulMouseSensitivity;
    ULONG ulOrigMouseSensitivity;

    ULONG ulMouseSpeed; // = 1;
    ULONG ulMouseThreshold1; // = DEFAULT_MOUSE_THRESHOLD1;
    ULONG ulMouseThreshold2; // = DEFAULT_MOUSE_THRESHOLD2;

    ULONG ulSnapToDefaultButton;
    ULONG ulMouseTrails;
    ULONG ulShowPointer;
    ULONG ulHidePointer;
} OPTION_DATA, *POPTION_DATA;


typedef struct _WHEEL_DATA
{
    UINT uWheelScrollLines;
} WHEEL_DATA, *PWHEEL_DATA;


typedef struct _CURSOR_DATA
{
    UINT uStringId;
    LPWSTR uDefaultCursorId;
    HCURSOR hCursor;
    TCHAR szCursorName[MAX_PATH];
    TCHAR szCursorPath[MAX_PATH];
} CURSOR_DATA, *PCURSOR_DATA;


CURSOR_DATA g_CursorData[] =
{{IDS_ARROW,       IDC_ARROW,       0, _T(""), _T("")},
 {IDS_HELP,        IDC_HELP,        0, _T(""), _T("")},
 {IDS_APPSTARTING, IDC_APPSTARTING, 0, _T(""), _T("")},
 {IDS_WAIT,        IDC_WAIT,        0, _T(""), _T("")},
 {IDS_CROSSHAIR,   IDC_CROSS,       0, _T(""), _T("")},
 {IDS_IBEAM,       IDC_IBEAM,       0, _T(""), _T("")},
 {IDS_NWPEN,       0/*IDC_NWPEN*/,       0, _T(""), _T("")}, /* FIXME */
 {IDS_NO,          IDC_NO,          0, _T(""), _T("")},
 {IDS_SIZENS,      IDC_SIZENS,      0, _T(""), _T("")},
 {IDS_SIZEWE,      IDC_SIZEWE,      0, _T(""), _T("")},
 {IDS_SIZENWSE,    IDC_SIZENWSE,    0, _T(""), _T("")},
 {IDS_SIZENESW,    IDC_SIZENESW,    0, _T(""), _T("")},
 {IDS_SIZEALL,     IDC_SIZEALL,     0, _T(""), _T("")},
 {IDS_UPARROW,     IDC_UPARROW,     0, _T(""), _T("")},
 {IDS_HAND,        IDC_HAND,        0, _T(""), _T("")}};


TCHAR g_szNewScheme[MAX_PATH];

#if 0
static VOID
DebugMsg(LPTSTR fmt, ...)
{
    TCHAR szBuffer[2048];
    va_list marker;

    va_start(marker, fmt);
    _vstprintf(szBuffer, fmt, marker);
    va_end(marker);

    MessageBox(NULL, szBuffer, _T("Debug message"), MB_OK);
}
#endif


/* Property page dialog callback */
static INT_PTR CALLBACK
MouseHardwareProc(IN HWND hwndDlg,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    GUID Guids[1];
    Guids[0] = GUID_DEVCLASS_MOUSE;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            /* create the hardware page */
            DeviceCreateHardwarePageEx(hwndDlg,
                                       Guids,
                                       sizeof(Guids) / sizeof(Guids[0]),
                                       HWPD_STANDARDLIST);
            break;
    }

    return FALSE;
}


static INT_PTR CALLBACK
ClickLockProc(IN HWND hwndDlg,
              IN UINT uMsg,
              IN WPARAM wParam,
              IN LPARAM lParam)
{
    HWND hDlgCtrl;
    int pos;

    PBUTTON_DATA pButtonData;

    pButtonData = (PBUTTON_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pButtonData = (PBUTTON_DATA)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pButtonData);

            hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_CLICK_LOCK);
            SendMessage(hDlgCtrl, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 10));
            pos = (pButtonData->g_ClickLockTime - 200) / 200;
            SendMessage(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pos);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_CLICK_LOCK);
                pButtonData->g_ClickLockTime = (DWORD) (SendMessage(hDlgCtrl, TBM_GETPOS, 0, 0) * 200) + 200;
                EndDialog(hwndDlg, TRUE);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, FALSE);
            }
            break;
    }

    return FALSE;
}


static INT_PTR CALLBACK
ButtonProc(IN HWND hwndDlg,
           IN UINT uMsg,
           IN WPARAM wParam,
           IN LPARAM lParam)
{
    HWND hDlgCtrl;
    LRESULT lResult;
    LPPSHNOTIFY lppsn;

    PBUTTON_DATA pButtonData;

    pButtonData = (PBUTTON_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pButtonData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BUTTON_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pButtonData);

            pButtonData->g_SwapMouseButtons = GetSystemMetrics(SM_SWAPBUTTON);
            pButtonData->g_OrigSwapMouseButtons = pButtonData->g_SwapMouseButtons;
            pButtonData->g_DoubleClickSpeed = GetDoubleClickTime();
            pButtonData->g_OrigDoubleClickSpeed = pButtonData->g_DoubleClickSpeed;

            /* Click lock time */
            SystemParametersInfo(SPI_GETMOUSECLICKLOCK, 0, &pButtonData->g_ClickLockEnabled, 0);
            SystemParametersInfo(SPI_GETMOUSECLICKLOCKTIME, 0, &pButtonData->g_ClickLockTime, 0);

            /* Load mouse button icons */
            pButtonData->hButtonLeft = LoadImage(hApplet, MAKEINTRESOURCE(IDI_MOUSE_LEFT), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
            pButtonData->hButtonRight = LoadImage(hApplet, MAKEINTRESOURCE(IDI_MOUSE_RIGHT), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);

            /* Load folder icons */
            pButtonData->hIcon1 = LoadImage(hApplet, MAKEINTRESOURCE(IDI_FOLDER_CLOSED), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
            pButtonData->hIcon2 = LoadImage(hApplet, MAKEINTRESOURCE(IDI_FOLDER_OPEN), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);

            if (pButtonData->g_SwapMouseButtons)
            {
                SendDlgItemMessage(hwndDlg, IDC_SWAP_MOUSE_BUTTONS, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                SendDlgItemMessage(hwndDlg, IDC_IMAGE_SWAP_MOUSE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pButtonData->hButtonRight);
            }
            else
            {
                SendDlgItemMessage(hwndDlg, IDC_IMAGE_SWAP_MOUSE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pButtonData->hButtonLeft);
            }

            if (pButtonData->g_ClickLockEnabled)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_CLICK_LOCK);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }
            else
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_BUTTON_CLICK_LOCK);
                EnableWindow(hDlgCtrl, FALSE);
            }

            hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_DOUBLE_CLICK_SPEED);
            SendMessage(hDlgCtrl, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 14));
            SendMessage(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, 14 - ((pButtonData->g_DoubleClickSpeed - 200) / 50));


            SendDlgItemMessage(hwndDlg, IDC_IMAGE_DOUBLE_CLICK_SPEED, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pButtonData->hIcon1);
            pButtonData->bClicked = TRUE;
            return TRUE;

        case WM_DESTROY:
            DestroyIcon(pButtonData->hButtonLeft);
            DestroyIcon(pButtonData->hButtonRight);
            DestroyIcon(pButtonData->hIcon1);
            DestroyIcon(pButtonData->hIcon2);
            HeapFree(GetProcessHeap(), 0, pButtonData);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_SWAP_MOUSE_BUTTONS:
                    lResult = SendMessage((HWND)lParam, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
                    if (lResult == BST_CHECKED)
                    {
                        pButtonData->g_SwapMouseButtons = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                        SendDlgItemMessage(hwndDlg, IDC_IMAGE_SWAP_MOUSE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pButtonData->hButtonLeft);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        pButtonData->g_SwapMouseButtons = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                        SendDlgItemMessage(hwndDlg, IDC_IMAGE_SWAP_MOUSE, STM_SETIMAGE, IMAGE_ICON, (LPARAM)pButtonData->hButtonRight);
                    }
                    SystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, pButtonData->g_SwapMouseButtons, NULL, SPIF_SENDCHANGE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_CHECK_CLICK_LOCK:
                    lResult = SendMessage((HWND)lParam, BM_GETCHECK, (WPARAM)0, (LPARAM)0);
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_BUTTON_CLICK_LOCK);
                    if (lResult == BST_CHECKED)
                    {
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                        pButtonData->g_ClickLockEnabled = FALSE;
                        EnableWindow(hDlgCtrl, FALSE);
                    }
                    else if (lResult == BST_UNCHECKED)
                    {
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                        EnableWindow(hDlgCtrl, TRUE);
                        pButtonData->g_ClickLockEnabled = TRUE;
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_BUTTON_CLICK_LOCK:
                    DialogBoxParam(hApplet, MAKEINTRESOURCE(IDD_CLICK_LOCK), hwndDlg, ClickLockProc, (LPARAM)pButtonData);
                    break;

                case IDC_IMAGE_DOUBLE_CLICK_SPEED:
                    if (HIWORD(wParam) == STN_DBLCLK)
                    {
                        pButtonData->bClicked = !pButtonData->bClicked;
                        SendDlgItemMessage(hwndDlg, IDC_IMAGE_DOUBLE_CLICK_SPEED, STM_SETIMAGE, IMAGE_ICON,
                                           (LPARAM)(pButtonData->bClicked ? pButtonData->hIcon1 : pButtonData->hIcon2));
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
#if (WINVER >= 0x0500)
                SystemParametersInfo(SPI_SETMOUSECLICKLOCK, pButtonData->g_ClickLockEnabled, NULL, SPIF_SENDCHANGE);
                if (pButtonData->g_ClickLockEnabled)
                   SystemParametersInfo(SPI_SETMOUSECLICKLOCKTIME, pButtonData->g_ClickLockTime, NULL, SPIF_SENDCHANGE);
#endif
                SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
            }
            else if (lppsn->hdr.code == PSN_RESET)
            {
                /* Reset swap mouse button setting */
                SystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, pButtonData->g_OrigSwapMouseButtons, NULL, SPIF_SENDCHANGE);

                /* Reset double click speed setting */
//                SystemParametersInfo(SPI_SETDOUBLECLICKTIME, pButtonData->g_OrigDoubleClickSpeed, NULL, SPIF_SENDCHANGE);
                SetDoubleClickTime(pButtonData->g_OrigDoubleClickSpeed);
            }
            return TRUE;

        case WM_HSCROLL:
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_DOUBLE_CLICK_SPEED))
            {
                switch (LOWORD(wParam))
                {
                    case TB_LINEUP:
                    case TB_LINEDOWN:
                    case TB_PAGEUP:
                    case TB_PAGEDOWN:
                    case TB_TOP:
                    case TB_BOTTOM:
                    case TB_ENDTRACK:
                        lResult = SendDlgItemMessage(hwndDlg, IDC_SLIDER_DOUBLE_CLICK_SPEED, TBM_GETPOS, 0, 0);
                        pButtonData->g_DoubleClickSpeed = (14 - (INT)lResult) * 50 + 200;
//                        SystemParametersInfo(SPI_SETDOUBLECLICKTIME, pButtonData->g_DoubleClickSpeed, NULL, SPIF_SENDCHANGE);
                        SetDoubleClickTime(pButtonData->g_DoubleClickSpeed);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;

                    case TB_THUMBTRACK:
                        pButtonData->g_DoubleClickSpeed = (14 - (INT)HIWORD(wParam)) * 50 + 200;
//                        SystemParametersInfo(SPI_SETDOUBLECLICKTIME, pButtonData->g_DoubleClickSpeed, NULL, SPIF_SENDCHANGE);
                        SetDoubleClickTime(pButtonData->g_DoubleClickSpeed);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;
                }
            }
            break;
    }

    return FALSE;
}


static BOOL
EnumerateCursorSchemes(HWND hwndDlg)
{
    HKEY hCuKey;
    HKEY hCuCursorKey;
    DWORD dwIndex;
    TCHAR szValueName[MAX_PATH];
    DWORD dwValueName;
    TCHAR szSystemScheme[MAX_PATH];
    TCHAR szValueData[2000];
    DWORD dwValueData;
    LONG dwResult;
    HWND hDlgCtrl;
    LRESULT lResult;
    BOOL ProcessedHKLM = FALSE;
    TCHAR szCurrentScheme[MAX_PATH];
    DWORD dwCurrentScheme;
    INT nSchemeIndex;
    INT i, nCount;
    LPTSTR p;

    if (RegOpenCurrentUser(KEY_READ, &hCuKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegOpenKeyEx(hCuKey, _T("Control Panel\\Cursors\\Schemes"), 0, KEY_READ, &hCuCursorKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hCuKey);
        return FALSE;
    }

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMBO_CURSOR_SCHEME);
    SendMessage(hDlgCtrl, CB_RESETCONTENT, 0, 0);
    dwIndex = 0;

    for (;;)
    {
        dwValueName = sizeof(szValueName) / sizeof(TCHAR);
        dwValueData = sizeof(szValueData) / sizeof(TCHAR);
        dwResult = RegEnumValue(hCuCursorKey, dwIndex, szValueName, &dwValueName, NULL, NULL, (LPBYTE)szValueData, &dwValueData);

        if (dwResult == ERROR_NO_MORE_ITEMS)
        {
            if (!ProcessedHKLM)
            {
                RegCloseKey(hCuCursorKey);
                dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                        _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cursors\\Schemes"),
                                        0, KEY_READ, &hCuCursorKey);
                if (dwResult == ERROR_SUCCESS)
                {
                    dwIndex = 0;
                    ProcessedHKLM = TRUE;
                    LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);
                    continue;
                }
            }
            break;
        }

        if (_tcslen(szValueData) > 0)
        {
            TCHAR * copy = _tcsdup(szValueData);
            if (ProcessedHKLM)
            {
               _tcscat(szValueName, TEXT(" "));
               _tcscat(szValueName, szSystemScheme);
            }
            lResult = SendMessage(hDlgCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValueName);
            SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)copy);
        }

        dwIndex++;
    }

    RegCloseKey(hCuCursorKey);
    RegCloseKey(hCuKey);

    /* Add the "(none)" entry */
    LoadString(hApplet, IDS_NONE, szSystemScheme, MAX_PATH);
    lResult = SendMessage(hDlgCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)szSystemScheme);
    SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)NULL);


    /* Get the name of the current cursor scheme */
    szCurrentScheme[0] = 0;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Cursors"), 0, KEY_READ, &hCuCursorKey) == ERROR_SUCCESS)
    {
        dwCurrentScheme = sizeof(szCurrentScheme) / sizeof(TCHAR);
        if (RegQueryValueEx(hCuCursorKey, NULL, NULL, NULL, (LPBYTE)szCurrentScheme, &dwCurrentScheme))
            szCurrentScheme[0] = 0;
        RegCloseKey(hCuCursorKey);
    }

    /* Search for the matching entry in the cursor scheme list */
    LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);
    nSchemeIndex = -1;
    nCount = (INT)SendMessage(hDlgCtrl, CB_GETCOUNT, 0, 0);
    for (i = 0; i < nCount; i++)
    {
        SendMessage(hDlgCtrl, CB_GETLBTEXT, i, (LPARAM)szValueName);

        p = _tcsstr(szValueName, szSystemScheme);
        if (p)
        {
            p -= 1;
            *p = 0;
        }

//        DebugMsg(_T("szCurrentScheme: \"%s\"\nszValueName: \"%s\""), szCurrentScheme, szValueName);

        if (_tcscmp(szValueName, szCurrentScheme) == 0)
        {
            nSchemeIndex = (INT)i;
            break;
        }
    }

    /* Select the matching entry */
    if (nSchemeIndex != -1)
        SendMessage(hDlgCtrl, CB_SETCURSEL, (WPARAM)nSchemeIndex, (LPARAM)0);
    else
    {
        SendMessage(hDlgCtrl, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_BUTTON_DELETE_SCHEME);
        EnableWindow(hDlgCtrl, FALSE);
    }

    return TRUE;
}


static VOID
RefreshCursorList(HWND hwndDlg, BOOL bInit)
{
    INT index;
    INT i;
    INT nSel;

    nSel = bInit ? 0 : SendDlgItemMessage(hwndDlg, IDC_LISTBOX_CURSOR, LB_GETCURSEL, 0, 0);

    if (bInit)
    {
        SendDlgItemMessage(hwndDlg, IDC_LISTBOX_CURSOR, LB_RESETCONTENT, 0, 0);
        for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
        {
            LoadString(hApplet, index, g_CursorData[i].szCursorName, MAX_PATH);
            SendDlgItemMessage(hwndDlg, IDC_LISTBOX_CURSOR, LB_ADDSTRING, 0, (LPARAM)i);
        }

        SendDlgItemMessage(hwndDlg, IDC_LISTBOX_CURSOR, LB_SETCURSEL, nSel, 0);
    }
    else
    {
        InvalidateRect(GetDlgItem(hwndDlg, IDC_LISTBOX_CURSOR), NULL, FALSE);
    }

    SendDlgItemMessage(hwndDlg, IDC_IMAGE_CURRENT_CURSOR, STM_SETIMAGE, IMAGE_CURSOR,
                       (LPARAM)g_CursorData[nSel].hCursor);
}


static BOOL
DeleteUserCursorScheme(TCHAR * szScheme)
{
    HKEY hCuKey;
    HKEY hCuCursorKey;
    LONG Result;

    if (RegOpenCurrentUser(KEY_READ | KEY_SET_VALUE, &hCuKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegOpenKeyEx(hCuKey, _T("Control Panel\\Cursors\\Schemes"), 0, KEY_READ | KEY_SET_VALUE, &hCuCursorKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hCuKey);
        return FALSE;
    }

    Result = RegDeleteValue(hCuCursorKey, szScheme);

    RegCloseKey(hCuCursorKey);
    RegCloseKey(hCuKey);

    return (Result == ERROR_SUCCESS);
}


static INT_PTR CALLBACK
SaveSchemeProc(IN HWND hwndDlg,
               IN UINT uMsg,
               IN WPARAM wParam,
               IN LPARAM lParam)
{
    HWND hDlgCtrl;
    UNREFERENCED_PARAMETER(lParam);

    switch(uMsg)
    {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_SCHEME_NAME);
                SendMessage(hDlgCtrl, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)g_szNewScheme);
                EndDialog(hwndDlg, TRUE);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, FALSE);
            }
            break;
    }

    return FALSE;
}


static BOOL
BrowseCursor(TCHAR * szFileName, HWND hwndDlg)
{
    //FIXME load text resources from string
    OPENFILENAME ofn;
    static TCHAR szFilter[] = _T("Cursors\0*.ani;*.cur\0Animated Cursors\0*.ani\0Static Cursors\0*.cur\0All Files\0*.*\0\0");

    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = _T("%WINDIR%\\Cursors");
    ofn.lpstrTitle = _T("Browse");
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
        return TRUE;
    else
        return FALSE;
}


static VOID
LoadCurrentCursorScheme(LPTSTR lpName, BOOL bSystem)
{
    UINT index, i;

    for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
    {
        if (g_CursorData[i].hCursor != NULL)
        {
            DestroyCursor(g_CursorData[i].hCursor);
            g_CursorData[i].hCursor = 0;
        }
        g_CursorData[i].szCursorPath[0] = 0;
    }

#if 0
    if (lpName == NULL)
    {
        for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
        {
            g_CursorData[i].hCursor = LoadCursor(NULL, g_CursorData[i].uDefaultCursorId);
        }
    }
    else
#endif

    if (lpName != NULL)
    {
        LPTSTR pStart = lpName;
        LPTSTR pEnd = pStart;
        INT nLength;

        i = 0;
        while (pEnd)
        {
            pEnd = _tcschr(pStart, _T(','));
            if (pEnd)
                nLength = ((INT_PTR)pEnd - (INT_PTR)pStart) / sizeof(TCHAR);
            else
                nLength = _tcslen(pStart);

            _tcsncpy(g_CursorData[i].szCursorPath, pStart, nLength);
            g_CursorData[i].szCursorPath[nLength] = 0;

            pStart = pStart + (nLength + 1);
            i++;
        }

    }

    for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
    {
        if (g_CursorData[i].szCursorPath[0] == 0)
            g_CursorData[i].hCursor = LoadCursor(NULL, g_CursorData[i].uDefaultCursorId);
        else
            g_CursorData[i].hCursor = (HCURSOR)LoadImage(NULL, g_CursorData[i].szCursorPath,
                                                         IMAGE_CURSOR, 0, 0,
                                                         LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
}


static VOID
ReloadCurrentCursorScheme(VOID)
{
    UINT index, i;

    for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
    {
        if (g_CursorData[i].hCursor != NULL)
            DestroyCursor(g_CursorData[i].hCursor);

        if (g_CursorData[i].szCursorPath[0] == 0)
            g_CursorData[i].hCursor = LoadCursor(NULL, g_CursorData[i].uDefaultCursorId);
        else
            g_CursorData[i].hCursor = (HCURSOR)LoadImage(NULL, g_CursorData[i].szCursorPath,
                                                         IMAGE_CURSOR, 0, 0,
                                                         LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
}


static VOID
OnDrawItem(UINT idCtl,
           LPDRAWITEMSTRUCT lpdis)
{
    RECT rc;

    if (lpdis->itemState & ODS_SELECTED)
    {
        FillRect(lpdis->hDC,
                 &lpdis->rcItem,
                 (HBRUSH)(COLOR_HIGHLIGHT + 1));
        SetBkColor(lpdis->hDC,
                   GetSysColor(COLOR_HIGHLIGHT));
        SetTextColor(lpdis->hDC,
                   GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        FillRect(lpdis->hDC,
                 &lpdis->rcItem,
                 (HBRUSH)(COLOR_WINDOW + 1));
        SetBkColor(lpdis->hDC,
                   GetSysColor(COLOR_WINDOW));
        SetTextColor(lpdis->hDC,
                   GetSysColor(COLOR_WINDOWTEXT));
    }

    if (lpdis->itemID != -1)
    {
        CopyRect(&rc, &lpdis->rcItem);
        rc.left += 5;
        DrawText(lpdis->hDC,
                 g_CursorData[lpdis->itemData].szCursorName,
                 -1,
                 &rc,
                 DT_SINGLELINE | DT_VCENTER | DT_LEFT);

        if (g_CursorData[lpdis->itemData].hCursor != NULL)
        {
            DrawIcon(lpdis->hDC,
                     lpdis->rcItem.right - 32 - 4,
                     lpdis->rcItem.top + 2,
                     g_CursorData[lpdis->itemData].hCursor);
        }
    }

    if (lpdis->itemState & ODS_FOCUS)
    {
        CopyRect(&rc, &lpdis->rcItem);
        InflateRect(&rc, -1, -1);
        DrawFocusRect(lpdis->hDC, &rc);
    }
}


static INT_PTR CALLBACK
PointerProc(IN HWND hwndDlg,
            IN UINT uMsg,
            IN WPARAM wParam,
            IN LPARAM lParam)
{
    LPPSHNOTIFY lppsn;
    TCHAR buffer[MAX_PATH];
    TCHAR szSystemScheme[MAX_PATH];
    HWND hDlgCtrl;
    LRESULT lResult;

    PPOINTER_DATA pPointerData;

    pPointerData = (PPOINTER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pPointerData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POINTER_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pPointerData);

            EnumerateCursorSchemes(hwndDlg);
            LoadCurrentCursorScheme(NULL, FALSE);
            RefreshCursorList(hwndDlg, TRUE);

            /* Get drop shadow setting */
            if (!SystemParametersInfo(SPI_GETDROPSHADOW, 0, &pPointerData->bDropShadow, 0))
                pPointerData->bDropShadow = FALSE;

            pPointerData->bOrigDropShadow = pPointerData->bDropShadow;

            if (pPointerData->bDropShadow)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_DROP_SHADOW);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            if ((INT)wParam == IDC_LISTBOX_CURSOR)
                return TRUE;
            else
                return FALSE;

        case WM_MEASUREITEM:
            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = 32 + 4;
            break;

        case WM_DRAWITEM:
            if (wParam == IDC_LISTBOX_CURSOR)
                OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
            return TRUE;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pPointerData);
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
#if (WINVER >= 0x0500)
                SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bDropShadow, SPIF_SENDCHANGE);
#endif
//                SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                return TRUE;
            }
            else if (lppsn->hdr.code == PSN_RESET)
            {
#if (WINVER >= 0x0500)
                SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bOrigDropShadow, SPIF_SENDCHANGE);
#endif
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_COMBO_CURSOR_SCHEME:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        BOOL bEnable;
                        LPTSTR lpName;

                        wParam = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        if(wParam == CB_ERR)
                           break;

                        SendMessage((HWND)lParam, CB_GETLBTEXT, wParam, (LPARAM)buffer);
                        LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);
                        if(_tcsstr(buffer, szSystemScheme) || wParam == 0) //avoid the default scheme can be deleted
                            bEnable = FALSE;
                        else
                            bEnable = TRUE;

                        /* delete button */
                        hDlgCtrl = GetDlgItem(hwndDlg, IDC_BUTTON_DELETE_SCHEME);
                        EnableWindow(hDlgCtrl, bEnable);

                        lpName = (LPTSTR)SendMessage((HWND)lParam, CB_GETITEMDATA, wParam, 0);
                        LoadCurrentCursorScheme(lpName, !bEnable);
                        RefreshCursorList(hwndDlg, FALSE);
                    }
                    break;

                case IDC_LISTBOX_CURSOR:
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        UINT uSel, uIndex;
                        uSel = SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                        uIndex = (UINT)SendMessage((HWND)lParam, LB_GETITEMDATA, (WPARAM)uSel, 0);
                        SendDlgItemMessage(hwndDlg, IDC_IMAGE_CURRENT_CURSOR, STM_SETIMAGE, IMAGE_CURSOR,
                                           (LPARAM)g_CursorData[uIndex].hCursor);
                    }
                    break;

                case IDC_BUTTON_SAVEAS_SCHEME:
                    if (DialogBox(hApplet, MAKEINTRESOURCE(IDD_CURSOR_SCHEME_SAVEAS), hwndDlg, SaveSchemeProc))
                    {
                        /* FIXME: save the cursor scheme */
                    }
                    break;

                case IDC_BUTTON_USE_DEFAULT_CURSOR:
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_LISTBOX_CURSOR);
                    lResult = SendMessage(hDlgCtrl, LB_GETCURSEL, 0, 0);
                    if (lResult != LB_ERR)
                    {
                        /* Clean the path of the currently selected cursor */
                        memset(g_CursorData[lResult].szCursorPath, 0x0, MAX_PATH * sizeof(TCHAR));

                        /* Update cursor list and preview */
                        ReloadCurrentCursorScheme();
                        RefreshCursorList(hwndDlg, FALSE);
                    }
                    break;

                case IDC_BUTTON_BROWSE_CURSOR:
                    memset(buffer, 0x0, sizeof(buffer));
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_LISTBOX_CURSOR);
                    lResult = SendMessage(hDlgCtrl, LB_GETCURSEL, 0, 0);
                    if (lResult == LB_ERR)
                        MessageBox(hwndDlg, _T("LB_ERR"), _T(""),MB_ICONERROR);
                    if (BrowseCursor(buffer, hwndDlg))
                    {
                        /* Store the new cursor file path */
                        _tcsncpy(g_CursorData[lResult].szCursorPath, buffer, MAX_PATH);

                        /* Update cursor list and preview */
                        ReloadCurrentCursorScheme();
                        RefreshCursorList(hwndDlg, FALSE);
                    }
                    break;

                case IDC_BUTTON_DELETE_SCHEME:
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMBO_CURSOR_SCHEME);
                    wParam = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
                    if(wParam == CB_ERR)
                        break;
                    SendMessage(hDlgCtrl, CB_GETLBTEXT, wParam, (LPARAM)buffer);
                    if (DeleteUserCursorScheme(buffer))
                    {
                        SendMessage(hDlgCtrl, CB_DELETESTRING, wParam, 0);
                        SendMessage(hDlgCtrl, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
                    }
                    break;

                case IDC_CHECK_DROP_SHADOW:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_DROP_SHADOW))
                    {
                        pPointerData->bDropShadow = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
#if (WINVER >= 0x0500)
                        SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bDropShadow, SPIF_SENDCHANGE);
#endif
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    else
                    {
                        pPointerData->bDropShadow = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
#if (WINVER >= 0x0500)
                        SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bDropShadow, SPIF_SENDCHANGE);
#endif
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;
    }

    return FALSE;
}


static BOOL
InitializeMouse(POPTION_DATA pOptionData)
{
    //FIXME
    //pointer precision
    // SPI_GETMOUSE?

    /* Get mouse sensitivity */
    if (!SystemParametersInfo(SPI_GETMOUSESPEED, 0, &pOptionData->ulMouseSensitivity, 0))
        pOptionData->ulMouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;
    pOptionData->ulOrigMouseSensitivity = pOptionData->ulMouseSensitivity;

    pOptionData->ulMouseSpeed = 1;
    pOptionData->ulMouseThreshold1 = DEFAULT_MOUSE_THRESHOLD1;
    pOptionData->ulMouseThreshold2 = DEFAULT_MOUSE_THRESHOLD2;

    /* snap to default button */
    if (SystemParametersInfo(SPI_GETSNAPTODEFBUTTON, 0, &pOptionData->ulSnapToDefaultButton, 0))
        pOptionData->ulSnapToDefaultButton = 0;

    /* mouse trails */
    if (!SystemParametersInfo(SPI_GETMOUSETRAILS, 0, &pOptionData->ulMouseTrails, 0))
        pOptionData->ulMouseTrails = 0;

    /* hide pointer while typing */
    if (!SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &pOptionData->ulHidePointer, 0))
        pOptionData->ulHidePointer = 0;

    /* show pointer with Ctrl-Key */
    if (!SystemParametersInfo(SPI_GETMOUSESONAR, 0, &pOptionData->ulShowPointer, 0))
        pOptionData->ulShowPointer = 0;

    return TRUE;
}


static INT_PTR CALLBACK
OptionProc(IN HWND hwndDlg,
           IN UINT uMsg,
           IN WPARAM wParam,
           IN LPARAM lParam)
{
    HWND hDlgCtrl;
    LPPSHNOTIFY lppsn;
    LRESULT lResult;
    POPTION_DATA pOptionData;

    pOptionData = (POPTION_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pOptionData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OPTION_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pOptionData);

            InitializeMouse(pOptionData);

            /* set mouse sensitivity */
            hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_MOUSE_SENSITIVITY);
            SendMessage(hDlgCtrl, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 19));
            SendMessage(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pOptionData->ulMouseSensitivity - 1);

            if (pOptionData->ulMouseSpeed)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_POINTER_PRECISION);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            if (pOptionData->ulSnapToDefaultButton)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_SNAP_TO);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            /* set mouse trail */
            hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_POINTER_TRAIL);
            SendMessage(hDlgCtrl, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 5));
            if (pOptionData->ulMouseTrails < 2)
                EnableWindow(hDlgCtrl, FALSE);
            else
                SendMessage(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pOptionData->ulMouseTrails - 2);

            if (pOptionData->ulShowPointer)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_SHOW_POINTER);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            if (pOptionData->ulHidePointer)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_HIDE_POINTER);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }
            break;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pOptionData);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_CHECK_POINTER_PRECISION:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_POINTER_PRECISION))
                    {
                        pOptionData->ulMouseSpeed = 0;
                        pOptionData->ulMouseThreshold1 = 0;
                        pOptionData->ulMouseThreshold2 = 0;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->ulMouseSpeed = 1;
                        pOptionData->ulMouseThreshold1 = 6;
                        pOptionData->ulMouseThreshold2 = 10;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    break;

                case IDC_CHECK_SNAP_TO:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_SNAP_TO))
                    {
                        pOptionData->ulSnapToDefaultButton = 0;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->ulSnapToDefaultButton = 1;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    break;

                case IDC_CHECK_POINTER_TRAIL:
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_POINTER_TRAIL);
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_POINTER_TRAIL))
                    {
                        pOptionData->ulMouseTrails = 0;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                        EnableWindow(hDlgCtrl, FALSE);
                    }
                    else
                    {
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                        EnableWindow(hDlgCtrl, TRUE);
                        pOptionData->ulMouseTrails = (ULONG)SendMessage(hDlgCtrl, TBM_GETPOS, 0, 0) + 2;
                    }
                    break;

                case IDC_CHECK_SHOW_POINTER:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOW_POINTER))
                    {
                        pOptionData->ulShowPointer = 0;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->ulShowPointer = 1;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    break;

                case IDC_CHECK_HIDE_POINTER:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_HIDE_POINTER))
                    {
                        pOptionData->ulHidePointer = 0;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->ulHidePointer = 1;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    break;
            }
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                /* set snap to default button */
                SystemParametersInfo(SPI_SETSNAPTODEFBUTTON, pOptionData->ulSnapToDefaultButton, 0, SPIF_SENDCHANGE);

#if 0
                /* set mouse trails */
                if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_POINTER_TRAIL))
                {
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_POINTER_TRAIL);
                    lResult = SendMessage(hDlgCtrl, TBM_GETPOS, 0, 0) + 2;
                }
                else
                {
                    lResult = 0;
                }

                SystemParametersInfo(SPI_SETMOUSETRAILS, (UINT) lResult, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
#endif

                //FIXME
                //pointer precision
                //SPI_SETMOUSE?

                /* calc pos and set mouse sensitivity */
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_MOUSE_SENSITIVITY);
                lResult = SendMessage(hDlgCtrl, TBM_GETPOS, 0, 0) + 1;
                SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulMouseSensitivity, SPIF_SENDCHANGE);

                /* hide pointer while typing */
                SystemParametersInfo(SPI_SETMOUSEVANISH, 0, (PVOID)pOptionData->ulHidePointer, SPIF_SENDCHANGE);

                /* show pointer with Ctrl-Key */
                SystemParametersInfo(SPI_SETMOUSESONAR, 0, (PVOID)pOptionData->ulShowPointer, SPIF_SENDCHANGE);

                SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
                return TRUE;
            }
            else if (lppsn->hdr.code == PSN_RESET)
            {
                /* Set the original mouse speed */
                SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulOrigMouseSensitivity, SPIF_SENDCHANGE);
            }
            break;

        case WM_HSCROLL:
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_MOUSE_SENSITIVITY))
            {
                switch (LOWORD(wParam))
                {
                    case TB_LINEUP:
                    case TB_LINEDOWN:
                    case TB_PAGEUP:
                    case TB_PAGEDOWN:
                    case TB_TOP:
                    case TB_BOTTOM:
                    case TB_ENDTRACK:
                        pOptionData->ulMouseSensitivity = (ULONG)SendDlgItemMessage(hwndDlg, IDC_SLIDER_MOUSE_SENSITIVITY, TBM_GETPOS, 0, 0) + 1;
                        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulMouseSensitivity, SPIF_SENDCHANGE);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;
#if 0
                    case TB_THUMBTRACK:
                        pOptionData->ulMouseSensitivity = (ULONG)HIWORD(wParam) + 1;
                        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulMouseSensitivity, SPIF_SENDCHANGE);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;
#endif
                }
            }
            else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_POINTER_TRAIL))
            {
                switch (LOWORD(wParam))
                {
                    case TB_LINEUP:
                    case TB_LINEDOWN:
                    case TB_PAGEUP:
                    case TB_PAGEDOWN:
                    case TB_TOP:
                    case TB_BOTTOM:
                    case TB_ENDTRACK:
                        pOptionData->ulMouseTrails = (ULONG)SendDlgItemMessage(hwndDlg, IDC_SLIDER_POINTER_TRAIL, TBM_GETPOS, 0, 0) + 2;
                        SystemParametersInfo(SPI_SETMOUSETRAILS, pOptionData->ulMouseTrails, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;

                    case TB_THUMBTRACK:
                        pOptionData->ulMouseTrails = (ULONG)HIWORD(wParam) + 2;
                        SystemParametersInfo(SPI_SETMOUSETRAILS, pOptionData->ulMouseTrails, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;
                }
            }
            break;
    }

    return FALSE;
}


static VOID
ShowDialogWheelControls(HWND hwndDlg, UINT uWheelScrollLines, BOOL bInit)
{
    HWND hDlgCtrl;

    if (uWheelScrollLines != WHEEL_PAGESCROLL)
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_WHEEL_SCROLL_LINES);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_WHEEL_SCROLL_LINES);
        EnableWindow(hDlgCtrl, TRUE);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_WHEEL_SCROLL_LINES);
        EnableWindow(hDlgCtrl, TRUE);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_WHEEL_SCROLL_PAGE);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
    }
    else
    {
        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_WHEEL_SCROLL_LINES);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_EDIT_WHEEL_SCROLL_LINES);
        EnableWindow(hDlgCtrl, FALSE);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_UPDOWN_WHEEL_SCROLL_LINES);
        EnableWindow(hDlgCtrl, FALSE);

        hDlgCtrl = GetDlgItem(hwndDlg, IDC_RADIO_WHEEL_SCROLL_PAGE);
        SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);

        /* Set the default scroll lines value */
        if (bInit == TRUE)
            SetDlgItemInt(hwndDlg, IDC_EDIT_WHEEL_SCROLL_LINES, DEFAULT_WHEEL_SCROLL_LINES, FALSE);
    }
}


static INT_PTR CALLBACK
WheelProc(IN HWND hwndDlg,
          IN UINT uMsg,
          IN WPARAM wParam,
          IN LPARAM lParam)
{
    LPPSHNOTIFY lppsn;
    PWHEEL_DATA pWheelData;

    pWheelData = (PWHEEL_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pWheelData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WHEEL_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pWheelData);

            /* Get wheel scroll lines */
            if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &pWheelData->uWheelScrollLines, 0))
            {
                pWheelData->uWheelScrollLines = DEFAULT_WHEEL_SCROLL_LINES;
            }

            ShowDialogWheelControls(hwndDlg, pWheelData->uWheelScrollLines, TRUE);
            SendDlgItemMessage(hwndDlg, IDC_UPDOWN_WHEEL_SCROLL_LINES, UDM_SETRANGE, 0, MAKELONG((short)100, (short)0));
            if (pWheelData->uWheelScrollLines != WHEEL_PAGESCROLL)
            {
                SetDlgItemInt(hwndDlg, IDC_EDIT_WHEEL_SCROLL_LINES, pWheelData->uWheelScrollLines, FALSE);
            }
            return TRUE;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pWheelData);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO_WHEEL_SCROLL_LINES:
                    pWheelData->uWheelScrollLines = GetDlgItemInt(hwndDlg, IDC_EDIT_WHEEL_SCROLL_LINES, NULL, FALSE);
                    ShowDialogWheelControls(hwndDlg, pWheelData->uWheelScrollLines, FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_RADIO_WHEEL_SCROLL_PAGE:
                    pWheelData->uWheelScrollLines = WHEEL_PAGESCROLL;
                    ShowDialogWheelControls(hwndDlg, pWheelData->uWheelScrollLines, FALSE);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;

                case IDC_EDIT_WHEEL_SCROLL_LINES:
                    if (pWheelData && HIWORD(wParam) == EN_CHANGE)
                    {
                        pWheelData->uWheelScrollLines = GetDlgItemInt(hwndDlg, IDC_EDIT_WHEEL_SCROLL_LINES, NULL, FALSE);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                SystemParametersInfo(SPI_SETWHEELSCROLLLINES, pWheelData->uWheelScrollLines,
                                     0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                return TRUE;
            }
            break;
    }

    return FALSE;
}


LONG APIENTRY
MouseApplet(HWND hwnd, UINT uMsg, LONG lParam1, LONG lParam2)
{
    PROPSHEETPAGE psp[5];
    PROPSHEETHEADER psh;
    TCHAR Caption[256];

    UNREFERENCED_PARAMETER(lParam1);
    UNREFERENCED_PARAMETER(lParam2);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet, IDS_CPLNAME_1, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON_1));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_PAGE_BUTTON, ButtonProc);
    InitPropSheetPage(&psp[1], IDD_PAGE_POINTER, PointerProc);
    InitPropSheetPage(&psp[2], IDD_PAGE_OPTION, OptionProc);
    InitPropSheetPage(&psp[3], IDD_PAGE_WHEEL, WheelProc);
    InitPropSheetPage(&psp[4], IDD_HARDWARE, MouseHardwareProc);
    return (LONG)(PropertySheet(&psh) != -1);
}

/* EOF */
