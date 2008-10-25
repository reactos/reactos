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
//  add missing icons

#include <windows.h>
#include <winuser.h>
#include <devguid.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>
#include <math.h>
#include <limits.h>
#include <shlobj.h>
#include <cplext.h>
#include <regstr.h>

#include <stdio.h>

#include "main.h"
#include "resource.h"

#define DEFAULT_DOUBLE_CLICK_SPEED	500
#define DEFAULT_CLICK_LOCK_TIME		2200
#define DEFAULT_MOUSE_SPEED		10
#define DEFAULT_MOUSE_ACCELERATION	1
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

    INT cxCursor;
    INT cyCursor;
} POINTER_DATA, *PPOINTER_DATA;


typedef struct _MOUSE_ACCEL
{
    INT nThreshold1;
    INT nThreshold2;
    INT nAcceleration;
} MOUSE_ACCEL;

typedef struct _OPTION_DATA
{
    ULONG ulMouseSpeed;
    ULONG ulOrigMouseSpeed;

    MOUSE_ACCEL MouseAccel;
    MOUSE_ACCEL OrigMouseAccel;

    BOOL bSnapToDefaultButton;
    BOOL bOrigSnapToDefaultButton;

    UINT uMouseTrails;
    UINT uOrigMouseTrails;

    BOOL bMouseVanish;
    BOOL bOrigMouseVanish;

    BOOL bMouseSonar;
    BOOL bOrigMouseSonar;
} OPTION_DATA, *POPTION_DATA;


typedef struct _WHEEL_DATA
{
    UINT uWheelScrollLines;
} WHEEL_DATA, *PWHEEL_DATA;


typedef struct _CURSOR_DATA
{
    UINT uStringId;
    UINT uDefaultCursorId;
    LPTSTR lpValueName;
    HCURSOR hCursor;
    TCHAR szCursorName[MAX_PATH];
    TCHAR szCursorPath[MAX_PATH];
} CURSOR_DATA, *PCURSOR_DATA;


CURSOR_DATA g_CursorData[] =
{{IDS_ARROW,       100/*OCR_NORMAL*/,      _T("Arrow"),       0, _T(""), _T("")},
 {IDS_HELP,        112/*OCR_HELP*/,        _T("Help"),        0, _T(""), _T("")},
 {IDS_APPSTARTING, 111/*OCR_APPSTARTING*/, _T("AppStarting"), 0, _T(""), _T("")},
 {IDS_WAIT,        102/*OCR_WAIT*/,        _T("Wait"),        0, _T(""), _T("")},
 {IDS_CROSSHAIR,   103/*OCR_CROSS*/,       _T("Crosshair"),   0, _T(""), _T("")},
 {IDS_IBEAM,       101/*OCR_IBEAM*/,       _T("IBeam"),       0, _T(""), _T("")},
 {IDS_NWPEN,       113/*OCR_NWPEN*/,       _T("NWPen"),       0, _T(""), _T("")},
 {IDS_NO,          110/*OCR_NO*/,          _T("No"),          0, _T(""), _T("")},
 {IDS_SIZENS,      108/*OCR_SIZENS*/,      _T("SizeNS"),      0, _T(""), _T("")},
 {IDS_SIZEWE,      107/*OCR_SIZEWE*/,      _T("SizeWE"),      0, _T(""), _T("")},
 {IDS_SIZENWSE,    105/*OCR_SIZENWSE*/,    _T("SizeNWSE"),    0, _T(""), _T("")},
 {IDS_SIZENESW,    106/*OCR_SIZENESW*/,    _T("SizeNESW"),    0, _T(""), _T("")},
 {IDS_SIZEALL,     109/*OCR_SIZEALL*/,     _T("SizeAll"),     0, _T(""), _T("")},
 {IDS_UPARROW,     104/*OCR_UP*/,          _T("UpArrow"),     0, _T(""), _T("")},
 {IDS_HAND,        114/*OCR_HAND*/,        _T("Hand"),        0, _T(""), _T("")}};


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
    static HICON hIcon;

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

            hIcon = LoadImage(hApplet, MAKEINTRESOURCE(IDI_LOOK_KEY),
                              IMAGE_ICON, 16, 16, 0);
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_CLICK_LOCK);
                pButtonData->g_ClickLockTime = (DWORD) (SendMessage(hDlgCtrl, TBM_GETPOS, 0, 0) * 200) + 200;
                EndDialog(hwndDlg, TRUE);
                if (hIcon) DestroyIcon(hIcon);
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, FALSE);
                if (hIcon) DestroyIcon(hIcon);
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
                    SystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, pButtonData->g_SwapMouseButtons, NULL, 0);
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
                        pButtonData->g_ClickLockEnabled = TRUE;
                        EnableWindow(hDlgCtrl, TRUE);
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
                if (pButtonData->g_OrigSwapMouseButtons != pButtonData->g_SwapMouseButtons)
                {
                    pButtonData->g_OrigSwapMouseButtons = pButtonData->g_SwapMouseButtons;
                    SystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, pButtonData->g_OrigSwapMouseButtons, NULL, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    
                }

#if (WINVER >= 0x0500)
                SystemParametersInfo(SPI_SETMOUSECLICKLOCK, pButtonData->g_ClickLockEnabled, NULL, SPIF_SENDCHANGE);
                if (pButtonData->g_ClickLockEnabled)
                   SystemParametersInfo(SPI_SETMOUSECLICKLOCKTIME, pButtonData->g_ClickLockTime, NULL, SPIF_SENDCHANGE);
#endif
            }
            else if (lppsn->hdr.code == PSN_RESET)
            {
                /* Reset swap mouse button setting */
                SystemParametersInfo(SPI_SETMOUSEBUTTONSWAP, pButtonData->g_OrigSwapMouseButtons, NULL, 0);

                /* Reset double click speed setting */
//                SystemParametersInfo(SPI_SETDOUBLECLICKTIME, pButtonData->g_OrigDoubleClickSpeed, NULL, 0);
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
//                        SystemParametersInfo(SPI_SETDOUBLECLICKTIME, pButtonData->g_DoubleClickSpeed, NULL, 0);
                        SetDoubleClickTime(pButtonData->g_DoubleClickSpeed);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;

                    case TB_THUMBTRACK:
                        pButtonData->g_DoubleClickSpeed = (14 - (INT)HIWORD(wParam)) * 50 + 200;
//                        SystemParametersInfo(SPI_SETDOUBLECLICKTIME, pButtonData->g_DoubleClickSpeed, NULL, 0);
                        SetDoubleClickTime(pButtonData->g_DoubleClickSpeed);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;
                }
            }
            break;
    }

    return FALSE;
}


static VOID
CompressPath(LPTSTR lpShortPath, LPTSTR lpPath)
{
    TCHAR szUserProfile[MAX_PATH];
    TCHAR szSystemRoot[MAX_PATH];
    TCHAR szProgramFiles[MAX_PATH];
    DWORD dwUserProfile;
    DWORD dwSystemRoot;
    DWORD dwProgramFiles;

    dwUserProfile = GetEnvironmentVariable(_T("USERPROFILE"), szUserProfile, MAX_PATH);
    dwSystemRoot = GetEnvironmentVariable(_T("SystemRoot"), szSystemRoot, MAX_PATH);
    dwProgramFiles = GetEnvironmentVariable(_T("ProgramFiles"), szProgramFiles, MAX_PATH);

    if (dwUserProfile > 0 && _tcsncmp(lpPath, szUserProfile, dwUserProfile) == 0)
    {
        _tcscpy(lpShortPath, _T("%USERPROFILE%"));
        _tcscat(lpShortPath, &lpPath[dwUserProfile]);
    }
    else if (dwSystemRoot > 0 && _tcsncmp(lpPath, szSystemRoot, dwSystemRoot) == 0)
    {
        _tcscpy(lpShortPath, _T("%SystemRoot%"));
        _tcscat(lpShortPath, &lpPath[dwSystemRoot]);
    }
    else if (dwProgramFiles > 0 && _tcsncmp(lpPath, szProgramFiles, dwProgramFiles) == 0)
    {
        _tcscpy(lpShortPath, _T("%ProgramFiles%"));
        _tcscat(lpShortPath, &lpPath[dwProgramFiles]);
    }
    else
    {
        _tcscpy(lpShortPath, lpPath);
    }
}


static BOOL
EnumerateCursorSchemes(HWND hwndDlg)
{
    HKEY hCursorKey;
    DWORD dwIndex;
    TCHAR szValueName[MAX_PATH];
    DWORD dwValueName;
    TCHAR szSystemScheme[MAX_PATH];
    TCHAR szValueData[2000];
    TCHAR szTempData[2000];
    DWORD dwValueData;
    LONG lError;
    HWND hDlgCtrl;
    LRESULT lResult;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMBO_CURSOR_SCHEME);
    SendMessage(hDlgCtrl, CB_RESETCONTENT, 0, 0);

    /* Read the users cursor schemes */
    lError = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Cursors\\Schemes"),
                          0, KEY_READ | KEY_QUERY_VALUE , &hCursorKey);
    if (lError == ERROR_SUCCESS)
    {
        for (dwIndex = 0;; dwIndex++)
        {
            dwValueName = sizeof(szValueName) / sizeof(TCHAR);
            dwValueData = sizeof(szValueData) / sizeof(TCHAR);
            lError = RegEnumValue(hCursorKey, dwIndex, szValueName, &dwValueName,
                                  NULL, NULL, (LPBYTE)szValueData, &dwValueData);
            if (lError == ERROR_NO_MORE_ITEMS)
                break;

            ExpandEnvironmentStrings(szValueData, szTempData, 2000);

            if (_tcslen(szTempData) > 0)
            {
                LPTSTR lpCopy, lpStart;

                /* Remove quotation marks */
                if (szTempData[0] == _T('"'))
                {
                    lpStart = szValueData + 1;
                    szTempData[_tcslen(szTempData) - 1] = 0;
                }
                else
                {
                    lpStart = szTempData;
                }

                lpCopy = _tcsdup(lpStart);

                lResult = SendMessage(hDlgCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValueName);
                SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)lpCopy);
            }
        }

        RegCloseKey(hCursorKey);
    }

    /* Read the system cursor schemes */
    lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cursors\\Schemes"),
                          0, KEY_READ | KEY_QUERY_VALUE , &hCursorKey);
    if (lError == ERROR_SUCCESS)
    {
        LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);

        for (dwIndex = 0;; dwIndex++)
        {
            dwValueName = sizeof(szValueName) / sizeof(TCHAR);
            dwValueData = sizeof(szValueData) / sizeof(TCHAR);
            lError = RegEnumValue(hCursorKey, dwIndex, szValueName, &dwValueName,
                                  NULL, NULL, (LPBYTE)szValueData, &dwValueData);
            if (lError == ERROR_NO_MORE_ITEMS)
                break;

            if (_tcslen(szValueData) > 0)
            {
                LPTSTR lpCopy, lpStart;

                /* Remove quotation marks */
                if (szValueData[0] == _T('"'))
                {
                    lpStart = szValueData + 1;
                    szValueData[_tcslen(szValueData) - 1] = 0;
                }
                else
                {
                    lpStart = szValueData;
                }

                lpCopy = _tcsdup(lpStart);

                _tcscat(szValueName, TEXT(" "));
                _tcscat(szValueName, szSystemScheme);

                lResult = SendMessage(hDlgCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValueName);
                SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)lpCopy);
            }
        }

        RegCloseKey(hCursorKey);
    }

    /* Add the "(none)" entry */
    LoadString(hApplet, IDS_NONE, szSystemScheme, MAX_PATH);
    lResult = SendMessage(hDlgCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)szSystemScheme);
    SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)NULL);

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

    EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_USE_DEFAULT_CURSOR), (g_CursorData[nSel].szCursorPath[0] != 0));
}


static BOOL
DeleteUserCursorScheme(HWND hwndDlg)
{
    TCHAR szSchemeName[MAX_PATH];
    TCHAR szTitle[128];
    TCHAR szRawText[256];
    TCHAR szText[256];
    HWND hDlgCtrl;
    HKEY hCuKey;
    HKEY hCuCursorKey;
    LONG lResult;
    INT nSel;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_COMBO_CURSOR_SCHEME);
    nSel = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (nSel == CB_ERR)
        return FALSE;

    SendMessage(hDlgCtrl, CB_GETLBTEXT, nSel, (LPARAM)szSchemeName);

    LoadString(hApplet, IDS_REMOVE_TITLE, szTitle, 128);
    LoadString(hApplet, IDS_REMOVE_TEXT, szRawText, 256);

    _stprintf(szText, szRawText, szSchemeName);

    /* Confirm scheme removal */
    if (MessageBox(hwndDlg, szText, szTitle, MB_YESNO | MB_ICONQUESTION) == IDNO)
        return TRUE;

    if (RegOpenCurrentUser(KEY_READ | KEY_SET_VALUE, &hCuKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegOpenKeyEx(hCuKey, _T("Control Panel\\Cursors\\Schemes"), 0, KEY_READ | KEY_SET_VALUE, &hCuCursorKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hCuKey);
        return FALSE;
    }

    lResult = RegDeleteValue(hCuCursorKey, szSchemeName);

    RegCloseKey(hCuCursorKey);
    RegCloseKey(hCuKey);

    if (lResult == ERROR_SUCCESS)
    {
        SendMessage(hDlgCtrl, CB_DELETESTRING, nSel, 0);
        SendMessage(hDlgCtrl, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
    }

    return (lResult == ERROR_SUCCESS);
}


static INT_PTR CALLBACK
SaveSchemeProc(IN HWND hwndDlg,
               IN UINT uMsg,
               IN WPARAM wParam,
               IN LPARAM lParam)
{
    LPTSTR pSchemeName;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pSchemeName = (LPTSTR)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pSchemeName);
            SendDlgItemMessage(hwndDlg, IDC_EDIT_SCHEME_NAME, WM_SETTEXT,
                               0, (LPARAM)pSchemeName);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                pSchemeName = (LPTSTR)GetWindowLongPtr(hwndDlg, DWLP_USER);
                SendDlgItemMessage(hwndDlg, IDC_EDIT_SCHEME_NAME, WM_GETTEXT,
                                   (WPARAM)MAX_PATH, (LPARAM)pSchemeName);
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
SaveCursorScheme(HWND hwndDlg)
{
    TCHAR szSystemScheme[MAX_PATH];
    TCHAR szSchemeName[MAX_PATH];
    TCHAR szNewSchemeName[MAX_PATH];
    TCHAR szTempPath[MAX_PATH];
    TCHAR szTitle[128];
    TCHAR szText[256];
    INT nSel;
    INT index, i, nLength;
    LPTSTR lpSchemeData;
    HKEY hCuKey;
    HKEY hCuCursorKey;
    LONG lError = ERROR_SUCCESS;
    BOOL bSchemeExists;

    LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);

    nSel = SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETCURSEL, 0, 0);
    if (nSel == CB_ERR)
       return FALSE;

    if (nSel == 0)
    {
        szSchemeName[0] = 0;
    }
    else
    {
        SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETLBTEXT, nSel, (LPARAM)szNewSchemeName);

        if (_tcsstr(szNewSchemeName, szSystemScheme))
        {
            szNewSchemeName[_tcslen(szNewSchemeName) - _tcslen(szSystemScheme) - 1] = 0;
        }
    }

    /* Ask for a name for the new cursor scheme */
    if (!DialogBoxParam(hApplet, MAKEINTRESOURCE(IDD_CURSOR_SCHEME_SAVEAS),
                        hwndDlg, SaveSchemeProc, (LPARAM)szNewSchemeName))
        return TRUE;

    /* Check all non-system schemes for the new name */
    nSel = SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETCOUNT, 0, 0);
    if (nSel == CB_ERR)
        return FALSE;

    bSchemeExists = FALSE;
    for (i = 0; i < nSel; i++)
    {
        SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETLBTEXT, i, (LPARAM)szSchemeName);
        if (_tcsstr(szSchemeName, szSystemScheme) == NULL)
        {
            if (_tcscmp(szSchemeName, szNewSchemeName) == 0)
            {
                bSchemeExists = TRUE;
                break;
            }
        }
    }

    if (bSchemeExists)
    {
        LoadString(hApplet, IDS_OVERWRITE_TITLE, szTitle, 128);
        LoadString(hApplet, IDS_OVERWRITE_TEXT, szText, 256);

         /* Confirm scheme overwrite */
        if (MessageBox(hwndDlg, szText, szTitle, MB_YESNO | MB_ICONQUESTION) == IDNO)
            return TRUE;
    }

    /* Save the cursor scheme */
    nLength = 0;
    for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
    {
        if (i > 0)
            nLength++;
        nLength += _tcslen(g_CursorData[i].szCursorPath);
    }
    nLength++;

    lpSchemeData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nLength * sizeof(TCHAR));

    for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
    {
        CompressPath(szTempPath, g_CursorData[i].szCursorPath);
        if (i > 0)
            _tcscat(lpSchemeData, _T(","));
        _tcscat(lpSchemeData, szTempPath);
    }

    if (RegOpenCurrentUser(KEY_READ | KEY_SET_VALUE, &hCuKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegOpenKeyEx(hCuKey, _T("Control Panel\\Cursors\\Schemes"), 0, KEY_READ | KEY_SET_VALUE, &hCuCursorKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hCuKey);
        return FALSE;
    }

    lError = RegSetValueEx(hCuCursorKey, szNewSchemeName, 0,
                           REG_EXPAND_SZ, (LPBYTE)lpSchemeData,
                           (_tcslen(lpSchemeData) + 1) * sizeof(TCHAR));

    RegCloseKey(hCuCursorKey);
    RegCloseKey(hCuKey);

    /* Add the new scheme to the scheme list and select it */
    if (lError == ERROR_SUCCESS)
    {
        LPTSTR copy = _tcsdup(lpSchemeData);

        nSel = SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_ADDSTRING, (WPARAM)0, (LPARAM)szNewSchemeName);
        SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_SETITEMDATA, (WPARAM)nSel, (LPARAM)copy);
        SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_SETCURSEL, (WPARAM)nSel, (LPARAM)0);
    }

    HeapFree(GetProcessHeap(), 0, lpSchemeData);

    return (lError == ERROR_SUCCESS);
}


static BOOL
BrowseCursor(HWND hwndDlg)
{
    TCHAR szFileName[MAX_PATH];
    TCHAR szFilter[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    OPENFILENAME ofn;
    INT nSel;

    LoadString(hApplet, IDS_BROWSE_FILTER, szFilter, MAX_PATH);
    LoadString(hApplet, IDS_BROWSE_TITLE, szTitle, MAX_PATH);

    memset(szFileName, 0x0, sizeof(szFileName));
    nSel = SendDlgItemMessage(hwndDlg, IDC_LISTBOX_CURSOR, LB_GETCURSEL, 0, 0);
    if (nSel == LB_ERR)
    {
        MessageBox(hwndDlg, _T("LB_ERR"), _T(""), MB_ICONERROR);
        return FALSE;
    }

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = _T("%WINDIR%\\Cursors");
    ofn.lpstrTitle = szTitle;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

    if (!GetOpenFileName(&ofn))
        return FALSE;

    /* Store the new cursor file path */
    _tcsncpy(g_CursorData[nSel].szCursorPath, szFileName, MAX_PATH);

    return TRUE;
}


static VOID
LoadCursorScheme(LPTSTR lpName, BOOL bSystem)
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
            g_CursorData[i].hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(g_CursorData[i].uDefaultCursorId),
                                                         IMAGE_CURSOR, 0, 0,
                                                         LR_DEFAULTSIZE | LR_SHARED);
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
            g_CursorData[i].hCursor = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(g_CursorData[i].uDefaultCursorId),
                                                         IMAGE_CURSOR, 0, 0,
                                                         LR_DEFAULTSIZE | LR_SHARED);
        else
            g_CursorData[i].hCursor = (HCURSOR)LoadImage(NULL, g_CursorData[i].szCursorPath,
                                                         IMAGE_CURSOR, 0, 0,
                                                         LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
}


static VOID
OnDrawItem(UINT idCtl,
           LPDRAWITEMSTRUCT lpdis,
           PPOINTER_DATA pPointerData)
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
                     lpdis->rcItem.right - pPointerData->cxCursor - 4,
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


static VOID
LoadNewCursorScheme(HWND hwndDlg)
{
    TCHAR buffer[MAX_PATH];
    TCHAR szSystemScheme[MAX_PATH];
    HWND hDlgCtrl;
    BOOL bEnable;
    LPTSTR lpName;
    INT nSel;

    nSel = SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETCURSEL, 0, 0);
    if (nSel == CB_ERR)
       return;

    SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETLBTEXT, nSel, (LPARAM)buffer);

    LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);
    if (_tcsstr(buffer, szSystemScheme) || nSel == 0) //avoid the default scheme can be deleted
        bEnable = FALSE;
    else
        bEnable = TRUE;

    /* delete button */
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_BUTTON_DELETE_SCHEME);
    EnableWindow(hDlgCtrl, bEnable);

    lpName = (LPTSTR)SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETITEMDATA, nSel, 0);
    LoadCursorScheme(lpName, !bEnable);
    RefreshCursorList(hwndDlg, FALSE);
}


static VOID
LoadInitialCursorScheme(HWND hwndDlg)
{
    TCHAR szSchemeName[256];
    TCHAR szSystemScheme[256];
    TCHAR szCursorPath[256];
    HKEY hCursorKey;
    LONG lError;
    DWORD dwDataSize;
    DWORD dwSchemeSource = 0;
    UINT index, i;
    DWORD dwType;
    INT nSel;

    for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
    {
        g_CursorData[i].hCursor = 0;
        g_CursorData[i].szCursorPath[0] = 0;
    }

    lError = RegOpenKeyEx(HKEY_CURRENT_USER,
                          _T("Control Panel\\Cursors"),
                          0,
                          KEY_READ | KEY_QUERY_VALUE,
                          &hCursorKey);
    if (lError != ERROR_SUCCESS)
        return;

    dwDataSize = sizeof(DWORD);
    lError = RegQueryValueEx(hCursorKey,
                             _T("Scheme Source"),
                             NULL,
                             NULL,
                             (LPBYTE)&dwSchemeSource,
                             &dwDataSize);

    if (dwSchemeSource != 0)
    {
        dwDataSize = 256 * sizeof(TCHAR);
        lError = RegQueryValueEx(hCursorKey,
                                 NULL,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szSchemeName,
                                 &dwDataSize);

        for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
        {
            dwDataSize = MAX_PATH * sizeof(TCHAR);
            lError = RegQueryValueEx(hCursorKey,
                                     g_CursorData[i].lpValueName,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)szCursorPath,
                                     &dwDataSize);
            if (lError == ERROR_SUCCESS)
            {
                if (dwType == REG_EXPAND_SZ)
                {
                    ExpandEnvironmentStrings(szCursorPath, g_CursorData[i].szCursorPath, MAX_PATH);
                }
                else
                {
                    _tcscpy(g_CursorData[i].szCursorPath, szCursorPath);
                }
            }
        }
    }

    RegCloseKey(hCursorKey);

    ReloadCurrentCursorScheme();
    RefreshCursorList(hwndDlg, TRUE);

    /* Build the full scheme name */
    if (dwSchemeSource == 0)
    {
        LoadString(hApplet, IDS_NONE, szSchemeName, MAX_PATH);
    }
    else if (dwSchemeSource == 2)
    {
        LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);
        _tcscat(szSchemeName, _T(" "));
        _tcscat(szSchemeName, szSystemScheme);
    }

    /* Search and select the curent scheme name from the scheme list */
    nSel = SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_FINDSTRINGEXACT, -1, (LPARAM)szSchemeName);
    if (nSel != CB_ERR)
        SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_SETCURSEL, (WPARAM)nSel, (LPARAM)0);

    /* Enable /disable delete button */
    EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON_DELETE_SCHEME), (dwSchemeSource == 1));
}


static BOOL
ApplyCursorScheme(HWND hwndDlg)
{
    TCHAR szSchemeName[MAX_PATH];
    TCHAR szSystemScheme[MAX_PATH];
    TCHAR szTempPath[MAX_PATH];
    LPTSTR lpSchemeData;
    DWORD dwNameLength;
    DWORD dwSchemeSource;
    UINT index, i;
    HKEY hCursorKey;
    INT nSel;

    nSel = SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETCURSEL, 0, 0);
    if (nSel == CB_ERR)
       return FALSE;

    lpSchemeData = (LPTSTR)SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETITEMDATA, nSel, 0);
    if (lpSchemeData == NULL)
    {
        /* "None" cursor scheme */
        dwSchemeSource = 0;
        szSchemeName[0] = 0;
        dwNameLength = 0;
    }
    else
    {
        SendDlgItemMessage(hwndDlg, IDC_COMBO_CURSOR_SCHEME, CB_GETLBTEXT, nSel, (LPARAM)szSchemeName);
        LoadString(hApplet, IDS_SYSTEM_SCHEME, szSystemScheme, MAX_PATH);

        if (_tcsstr(szSchemeName, szSystemScheme))
        {
            /* System scheme */
            dwSchemeSource = 2;
            szSchemeName[_tcslen(szSchemeName) - _tcslen(szSystemScheme) - 1] = 0;
        }
        else
        {
            /* User scheme */
            dwSchemeSource = 1;
        }

        dwNameLength = (_tcslen(szSchemeName) + 1) * sizeof(TCHAR);
    }

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Cursors"), 0,
                     KEY_READ | KEY_SET_VALUE, &hCursorKey) != ERROR_SUCCESS)
        return FALSE;

    RegSetValueEx(hCursorKey, NULL, 0, REG_SZ,
                  (LPBYTE)szSchemeName, dwNameLength);

    RegSetValueEx(hCursorKey, _T("Scheme Source"), 0, REG_DWORD,
                  (LPBYTE)&dwSchemeSource, sizeof(DWORD));

    for (index = IDS_ARROW, i = 0; index <= IDS_HAND; index++, i++)
    {
        CompressPath(szTempPath, g_CursorData[i].szCursorPath);
        RegSetValueEx(hCursorKey, g_CursorData[i].lpValueName, 0,
                      REG_EXPAND_SZ, (LPBYTE)szTempPath,
                      (_tcslen(szTempPath) + 1) * sizeof(TCHAR));
    }

    RegCloseKey(hCursorKey);

    /* Force the system to reload its cursors */
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);

    return TRUE;
}


static INT_PTR CALLBACK
PointerProc(IN HWND hwndDlg,
            IN UINT uMsg,
            IN WPARAM wParam,
            IN LPARAM lParam)
{
    PPOINTER_DATA pPointerData;
    LPPSHNOTIFY lppsn;
    INT nSel;

    pPointerData = (PPOINTER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pPointerData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POINTER_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pPointerData);

            pPointerData->cxCursor = GetSystemMetrics(SM_CXCURSOR);
            pPointerData->cyCursor = GetSystemMetrics(SM_CYCURSOR);

            EnumerateCursorSchemes(hwndDlg);
            LoadInitialCursorScheme(hwndDlg);

            /* Get drop shadow setting */
            if (!SystemParametersInfo(SPI_GETDROPSHADOW, 0, &pPointerData->bDropShadow, 0))
                pPointerData->bDropShadow = FALSE;

            pPointerData->bOrigDropShadow = pPointerData->bDropShadow;

            if (pPointerData->bDropShadow)
            {
                SendDlgItemMessage(hwndDlg, IDC_CHECK_DROP_SHADOW, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            if ((INT)wParam == IDC_LISTBOX_CURSOR)
                return TRUE;
            else
                return FALSE;

        case WM_MEASUREITEM:
            ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = GetSystemMetrics(SM_CYCURSOR) + 4;
            break;

        case WM_DRAWITEM:
            if (wParam == IDC_LISTBOX_CURSOR)
                OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam, pPointerData);
            return TRUE;

        case WM_DESTROY:
            HeapFree(GetProcessHeap(), 0, pPointerData);
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                ApplyCursorScheme(hwndDlg);
//#if (WINVER >= 0x0500)
                if (pPointerData->bOrigDropShadow != pPointerData->bDropShadow)
                {
                    SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bDropShadow, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    pPointerData->bOrigDropShadow = pPointerData->bDropShadow;
                }
//#endif
                return TRUE;
            }
            else if (lppsn->hdr.code == PSN_RESET)
            {
//#if (WINVER >= 0x0500)
                SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bOrigDropShadow, 0);
//#endif
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_COMBO_CURSOR_SCHEME:
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        LoadNewCursorScheme(hwndDlg);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_LISTBOX_CURSOR:
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                            nSel = SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                            SendDlgItemMessage(hwndDlg, IDC_IMAGE_CURRENT_CURSOR, STM_SETIMAGE, IMAGE_CURSOR,
                                               (LPARAM)g_CursorData[nSel].hCursor);
                            EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_USE_DEFAULT_CURSOR),
                                         (g_CursorData[nSel].szCursorPath[0] != 0));
                            break;

                        case LBN_DBLCLK:
                            if (BrowseCursor(hwndDlg))
                            {
                                /* Update cursor list and preview */
                                ReloadCurrentCursorScheme();
                                RefreshCursorList(hwndDlg, FALSE);

                                /* Enable the "Set Default" button */
                                EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_USE_DEFAULT_CURSOR), TRUE);

                                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                            }
                            break;
                    }
                    break;

                case IDC_BUTTON_SAVEAS_SCHEME:
                    SaveCursorScheme(hwndDlg);
                    break;

                case IDC_BUTTON_USE_DEFAULT_CURSOR:
                    nSel = SendDlgItemMessage(hwndDlg, IDC_LISTBOX_CURSOR, LB_GETCURSEL, 0, 0);
                    if (nSel != LB_ERR)
                    {
                        /* Clean the path of the currently selected cursor */
                        memset(g_CursorData[nSel].szCursorPath, 0x0, MAX_PATH * sizeof(TCHAR));

                        /* Update cursor list and preview */
                        ReloadCurrentCursorScheme();
                        RefreshCursorList(hwndDlg, FALSE);

                        /* Disable the "Set Default" button */
                        EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_USE_DEFAULT_CURSOR), FALSE);

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_BUTTON_BROWSE_CURSOR:
                    if (BrowseCursor(hwndDlg))
                    {
                        /* Update cursor list and preview */
                        ReloadCurrentCursorScheme();
                        RefreshCursorList(hwndDlg, FALSE);

                        /* Enable the "Set Default" button */
                        EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON_USE_DEFAULT_CURSOR), TRUE);

                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;

                case IDC_BUTTON_DELETE_SCHEME:
                    DeleteUserCursorScheme(hwndDlg);
                    break;

                case IDC_CHECK_DROP_SHADOW:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_DROP_SHADOW))
                    {
                        pPointerData->bDropShadow = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
//#if (WINVER >= 0x0500)
//                        SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bDropShadow, 0);
//#endif
//                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    else
                    {
                        pPointerData->bDropShadow = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
//#if (WINVER >= 0x0500)
                    SystemParametersInfo(SPI_SETDROPSHADOW, 0, (PVOID)pPointerData->bDropShadow, 0);
//#endif
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    break;
            }
            break;
    }

    return FALSE;
}


static INT_PTR CALLBACK
OptionProc(IN HWND hwndDlg,
           IN UINT uMsg,
           IN WPARAM wParam,
           IN LPARAM lParam)
{
    POPTION_DATA pOptionData;
    HWND hDlgCtrl;
    LPPSHNOTIFY lppsn;

    pOptionData = (POPTION_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pOptionData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OPTION_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pOptionData);

            /* Get mouse sensitivity */
            if (!SystemParametersInfo(SPI_GETMOUSESPEED, 0, &pOptionData->ulMouseSpeed, 0))
                pOptionData->ulMouseSpeed = DEFAULT_MOUSE_SPEED;
            pOptionData->ulOrigMouseSpeed = pOptionData->ulMouseSpeed;


            if (!SystemParametersInfo(SPI_GETMOUSE, 0, &pOptionData->MouseAccel, 0))
            {
                pOptionData->MouseAccel.nAcceleration = DEFAULT_MOUSE_ACCELERATION;
                pOptionData->MouseAccel.nThreshold1 = DEFAULT_MOUSE_THRESHOLD1;
                pOptionData->MouseAccel.nThreshold2 = DEFAULT_MOUSE_THRESHOLD2;
            }
            pOptionData->OrigMouseAccel.nAcceleration = pOptionData->MouseAccel.nAcceleration;
            pOptionData->OrigMouseAccel.nThreshold1 = pOptionData->MouseAccel.nThreshold1;
            pOptionData->OrigMouseAccel.nThreshold2 = pOptionData->MouseAccel.nThreshold2;

            /* snap to default button */
            if (SystemParametersInfo(SPI_GETSNAPTODEFBUTTON, 0, &pOptionData->bSnapToDefaultButton, 0))
                pOptionData->bSnapToDefaultButton = FALSE;
            pOptionData->bOrigSnapToDefaultButton = pOptionData->bSnapToDefaultButton;

            /* mouse trails */
            if (!SystemParametersInfo(SPI_GETMOUSETRAILS, 0, &pOptionData->uMouseTrails, 0))
                pOptionData->uMouseTrails = 0;
            pOptionData->uOrigMouseTrails = pOptionData->uMouseTrails;

            /* hide pointer while typing */
            if (!SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &pOptionData->bMouseVanish, 0))
                pOptionData->bMouseVanish = FALSE;
            pOptionData->bOrigMouseVanish = pOptionData->bMouseVanish;

            /* show pointer with Ctrl-Key */
            if (!SystemParametersInfo(SPI_GETMOUSESONAR, 0, &pOptionData->bMouseSonar, 0))
                pOptionData->bMouseSonar = FALSE;
            pOptionData->bOrigMouseSonar = pOptionData->bMouseSonar;

            /* Set mouse speed */
            hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_MOUSE_SPEED);
            SendMessage(hDlgCtrl, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 19));
            SendMessage(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pOptionData->ulMouseSpeed - 1);

            if (pOptionData->MouseAccel.nAcceleration)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_POINTER_PRECISION);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            if (pOptionData->bSnapToDefaultButton)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_SNAP_TO);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            /* set mouse trail */
            hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_POINTER_TRAIL);
            SendMessage(hDlgCtrl, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 5));
            if (pOptionData->uMouseTrails < 2)
            {
                SendMessage(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)5);
                EnableWindow(hDlgCtrl, FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_TRAIL_SHORT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_TRAIL_LONG), FALSE);
            }
            else
            {
                SendDlgItemMessage(hwndDlg, IDC_CHECK_POINTER_TRAIL, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                SendMessage(hDlgCtrl, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pOptionData->uMouseTrails - 2);
            }

            if (pOptionData->bMouseVanish)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_HIDE_POINTER);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            if (pOptionData->bMouseSonar)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_SHOW_POINTER);
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
                        pOptionData->MouseAccel.nAcceleration = 0;
                        pOptionData->MouseAccel.nThreshold1 = 0;
                        pOptionData->MouseAccel.nThreshold2 = 0;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->MouseAccel.nAcceleration = 1;
                        pOptionData->MouseAccel.nThreshold1 = 6;
                        pOptionData->MouseAccel.nThreshold2 = 10;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    SystemParametersInfo(SPI_SETMOUSE, 0, &pOptionData->MouseAccel, 0);
                    break;

                case IDC_CHECK_SNAP_TO:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_SNAP_TO))
                    {
                        pOptionData->bSnapToDefaultButton = 0;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->bSnapToDefaultButton = 1;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    SystemParametersInfo(SPI_SETSNAPTODEFBUTTON, (UINT)pOptionData->bSnapToDefaultButton, 0, 0);
                    break;

                case IDC_CHECK_POINTER_TRAIL:
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_SLIDER_POINTER_TRAIL);
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_POINTER_TRAIL))
                    {
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                        EnableWindow(hDlgCtrl, FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_TRAIL_SHORT), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_TRAIL_LONG), FALSE);
                        pOptionData->uMouseTrails = 0;
                    }
                    else
                    {
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                        EnableWindow(hDlgCtrl, TRUE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_TRAIL_SHORT), TRUE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_TRAIL_LONG), TRUE);
                        pOptionData->uMouseTrails = (UINT)SendMessage(hDlgCtrl, TBM_GETPOS, 0, 0) + 2;
                    }
                    SystemParametersInfo(SPI_SETMOUSETRAILS, pOptionData->uMouseTrails, 0, 0);
                    break;

                case IDC_CHECK_HIDE_POINTER:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_HIDE_POINTER))
                    {
                        pOptionData->bMouseVanish = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->bMouseVanish = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    SystemParametersInfo(SPI_SETMOUSEVANISH, 0, (PVOID)pOptionData->bMouseVanish, 0);
                    break;

                case IDC_CHECK_SHOW_POINTER:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CHECK_SHOW_POINTER))
                    {
                        pOptionData->bMouseSonar = FALSE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
                    }
                    else
                    {
                        pOptionData->bMouseSonar = TRUE;
                        SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
                    }
                    SystemParametersInfo(SPI_SETMOUSESONAR, 0, (PVOID)pOptionData->bMouseSonar, 0);
                    break;

            }
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                /* Set mouse speed */
                if (pOptionData->ulOrigMouseSpeed != pOptionData->ulMouseSpeed)
                {
                    SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulMouseSpeed, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    pOptionData->ulOrigMouseSpeed = pOptionData->ulMouseSpeed;
                }

                if (pOptionData->OrigMouseAccel.nAcceleration != pOptionData->MouseAccel.nAcceleration)
                {
                    SystemParametersInfo(SPI_SETMOUSE, 0, &pOptionData->MouseAccel, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    pOptionData->OrigMouseAccel.nAcceleration = pOptionData->MouseAccel.nAcceleration;
                    pOptionData->OrigMouseAccel.nThreshold1 = pOptionData->MouseAccel.nThreshold1;
                    pOptionData->OrigMouseAccel.nThreshold2 = pOptionData->MouseAccel.nThreshold2;
                }


                /* set snap to default button */
                if (pOptionData->bOrigSnapToDefaultButton != pOptionData->bSnapToDefaultButton)
                {
                    SystemParametersInfo(SPI_SETSNAPTODEFBUTTON, (UINT)pOptionData->bSnapToDefaultButton, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    pOptionData->bOrigSnapToDefaultButton = pOptionData->bSnapToDefaultButton;
                }

                /* Set mouse trails setting */
                if (pOptionData->uOrigMouseTrails != pOptionData->uMouseTrails)
                {
                    SystemParametersInfo(SPI_SETMOUSETRAILS, pOptionData->uMouseTrails, 0, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    pOptionData->uOrigMouseTrails = pOptionData->uMouseTrails;
                }

                /* hide pointer while typing */
                if (pOptionData->bOrigMouseVanish != pOptionData->bMouseVanish)
                {
                    SystemParametersInfo(SPI_SETMOUSEVANISH, 0, (PVOID)pOptionData->bMouseVanish, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    pOptionData->bOrigMouseVanish = pOptionData->bMouseVanish;
                }

                /* show pointer with Ctrl-Key */
                if (pOptionData->bOrigMouseSonar != pOptionData->bMouseSonar)
                {
                    SystemParametersInfo(SPI_SETMOUSESONAR, 0, (PVOID)pOptionData->bMouseSonar, SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
                    pOptionData->bOrigMouseSonar = pOptionData->bMouseSonar;
                }
                return TRUE;
            }
            else if (lppsn->hdr.code == PSN_RESET)
            {
                /* Set the original mouse speed */
                SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulOrigMouseSpeed, 0);
                SystemParametersInfo(SPI_SETMOUSE, 0, &pOptionData->OrigMouseAccel, 0);
                SystemParametersInfo(SPI_SETSNAPTODEFBUTTON, (UINT)pOptionData->bOrigSnapToDefaultButton, 0, 0);
                SystemParametersInfo(SPI_SETMOUSETRAILS, pOptionData->uOrigMouseTrails, 0, 0);
                SystemParametersInfo(SPI_SETMOUSEVANISH, 0, (PVOID)pOptionData->bOrigMouseVanish, 0);
                SystemParametersInfo(SPI_SETMOUSESONAR, 0, (PVOID)pOptionData->bOrigMouseSonar, 0);
            }
            break;

        case WM_HSCROLL:
            if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SLIDER_MOUSE_SPEED))
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
                        pOptionData->ulMouseSpeed = (ULONG)SendDlgItemMessage(hwndDlg, IDC_SLIDER_MOUSE_SPEED, TBM_GETPOS, 0, 0) + 1;
                        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulMouseSpeed, SPIF_SENDCHANGE);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;
#if 0
                    case TB_THUMBTRACK:
                        pOptionData->ulMouseSpeed = (ULONG)HIWORD(wParam) + 1;
                        SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)pOptionData->ulMouseSpeed, SPIF_SENDCHANGE);
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
                        pOptionData->uMouseTrails = (ULONG)SendDlgItemMessage(hwndDlg, IDC_SLIDER_POINTER_TRAIL, TBM_GETPOS, 0, 0) + 2;
                        SystemParametersInfo(SPI_SETMOUSETRAILS, pOptionData->uMouseTrails, 0, SPIF_UPDATEINIFILE);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        break;

                    case TB_THUMBTRACK:
                        pOptionData->uMouseTrails = (ULONG)HIWORD(wParam) + 2;
                        SystemParametersInfo(SPI_SETMOUSETRAILS, pOptionData->uMouseTrails, 0, SPIF_UPDATEINIFILE);
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

static const struct
{
    WORD idDlg;
    DLGPROC DlgProc;
    UINT uiReplaceWith;
} PropPages[] =
{
    { IDD_PAGE_BUTTON, ButtonProc, CPLPAGE_MOUSE_BUTTONS },
    { IDD_PAGE_POINTER, PointerProc, 0 },
    { IDD_PAGE_OPTION, OptionProc, CPLPAGE_MOUSE_PTRMOTION },
    { IDD_PAGE_WHEEL, WheelProc, CPLPAGE_MOUSE_WHEEL },
    { IDD_HARDWARE, MouseHardwareProc, 0 },
};

LONG APIENTRY
MouseApplet(HWND hwnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    HPROPSHEETPAGE hpsp[MAX_CPL_PAGES];
    PROPSHEETHEADER psh;
    HPSXA hpsxa;
    TCHAR Caption[256];
    UINT i;
    LONG ret;

    UNREFERENCED_PARAMETER(lParam1);
    UNREFERENCED_PARAMETER(lParam2);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet, IDS_CPLNAME_1, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON_1));
    psh.pszCaption = Caption;
    psh.nStartPage = 0;
    psh.phpage = hpsp;

    /* Load additional pages provided by shell extensions */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Mouse"), MAX_CPL_PAGES - psh.nPages);

    for (i = 0; i != sizeof(PropPages) / sizeof(PropPages[0]); i++)
    {
        /* Override the background page if requested by a shell extension */
        if (PropPages[i].uiReplaceWith != 0 && hpsxa != NULL &&
            SHReplaceFromPropSheetExtArray(hpsxa, PropPages[i].uiReplaceWith, PropSheetAddPage, (LPARAM)&psh) != 0)
        {
            /* The shell extension added one or more pages to replace a page.
               Don't create the built-in page anymore! */
            continue;
        }

        InitPropSheetPage(&psh, PropPages[i].idDlg, PropPages[i].DlgProc);
    }

    if (hpsxa != NULL)
        SHAddFromPropSheetExtArray(hpsxa, PropSheetAddPage, (LPARAM)&psh);

    ret = (LONG)(PropertySheet(&psh) != -1);

    if (hpsxa != NULL)
        SHDestroyPropSheetExtArray(hpsxa);

    return ret;
}

/* EOF */
