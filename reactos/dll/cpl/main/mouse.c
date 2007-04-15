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


TCHAR g_CurrentScheme[MAX_PATH];
TCHAR g_szArrow[MAX_PATH];
TCHAR g_szHelp[MAX_PATH];
TCHAR g_szAppStarting[MAX_PATH];
TCHAR g_szWait[MAX_PATH];
TCHAR g_szCrosshair[MAX_PATH];
TCHAR g_szIBeam[MAX_PATH];
TCHAR g_szNWPen[MAX_PATH];
TCHAR g_szNo[MAX_PATH];
TCHAR g_szSizeNS[MAX_PATH];
TCHAR g_szSizeWE[MAX_PATH];
TCHAR g_szSizeNWSE[MAX_PATH];
TCHAR g_szSizeNESW[MAX_PATH];
TCHAR g_szSizeAll[MAX_PATH];
TCHAR g_szUpArrow[MAX_PATH];
TCHAR g_szHand[MAX_PATH];

TCHAR g_szNewScheme[MAX_PATH];


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


#if 0
static BOOL
ReadActiveCursorScheme(VOID)
{
    HKEY hCuKey;
    HKEY hCuCursorKey;
    DWORD dwIndex;
    TCHAR szValueName[MAX_PATH];
    DWORD dwValueName;
    TCHAR szValueData[2000];
    DWORD dwValueData;
    LONG dwResult;

    if (RegOpenCurrentUser(KEY_READ, &hCuKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegOpenKeyEx(hCuKey, _T("Control Panel\\Cursors"), 0, KEY_READ, &hCuCursorKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hCuKey);
        return FALSE;
    }

    memset(g_szArrow, 0x0, sizeof(g_szArrow));
    memset(g_szHelp, 0x0, sizeof(g_szHelp));
    memset(g_szAppStarting, 0x0, sizeof(g_szAppStarting));
    memset(g_szWait, 0x0, sizeof(g_szWait));
    memset(g_szCrosshair, 0x0, sizeof(g_szCrosshair));
    memset(g_szIBeam, 0x0, sizeof(g_szIBeam));
    memset(g_szNWPen, 0x0, sizeof(g_szNWPen));
    memset(g_szNo, 0x0, sizeof(g_szNo));
    memset(g_szSizeNS, 0x0, sizeof(g_szSizeNS));
    memset(g_szSizeWE, 0x0, sizeof(g_szSizeWE));
    memset(g_szSizeNWSE, 0x0, sizeof(g_szSizeNWSE));
    memset(g_szSizeNESW, 0x0, sizeof(g_szSizeNESW));
    memset(g_szSizeAll, 0x0, sizeof(g_szSizeAll));
    memset(g_szUpArrow, 0x0, sizeof(g_szUpArrow));
    memset(g_szHand, 0x0, sizeof(g_szHand));

    dwIndex = 0;
    for (;;)
    {
        dwValueName = sizeof(szValueName) / sizeof(TCHAR);
        dwValueData = sizeof(szValueData) / sizeof(TCHAR);
        dwResult = RegEnumValue(hCuCursorKey, dwIndex, szValueName, &dwValueName, NULL, NULL, (LPBYTE)szValueData, &dwValueData);
        if (dwResult == ERROR_NO_MORE_ITEMS)
            break;

        if (_tcsicmp(_T("Arrow"), szValueName))
            _tcsncpy(g_szArrow, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("Help"), szValueName))
            _tcsncpy(g_szHelp, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("AppStarting"), szValueName))
            _tcsncpy(g_szAppStarting, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("Wait"), szValueName))
            _tcsncpy(g_szWait, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("Crosshair"), szValueName))
            _tcsncpy(g_szCrosshair, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("IBeam"), szValueName))
            _tcsncpy(g_szIBeam, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("NWPen"), szValueName))
            _tcsncpy(g_szNWPen, szValueData, MAX_PATH);
        else if (_tcscmp(_T("No"), szValueName))
            _tcsncpy(g_szNo, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("SizeNS"), szValueName))
            _tcsncpy(g_szSizeNS, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("SizeWE"), szValueName))
            _tcsncpy(g_szSizeWE, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("SizeNWSE"), szValueName))
            _tcsncpy(g_szSizeNWSE, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("SizeNESW"), szValueName))
            _tcsncpy(g_szSizeNESW, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("SizeAll"), szValueName))
            _tcsncpy(g_szSizeAll, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("UpArrow"), szValueName))
            _tcsncpy(g_szUpArrow, szValueData, MAX_PATH);
        else if (_tcsicmp(_T("Hand"), szValueName))
            _tcsncpy(g_szHand, szValueData, MAX_PATH);

        dwIndex++;
    }

    RegCloseKey(hCuCursorKey);
    RegCloseKey(hCuKey);

    return TRUE;
}
#endif


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

    LoadString(hApplet, IDS_NONE, szSystemScheme, MAX_PATH);
    lResult = SendMessage(hDlgCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)szSystemScheme);
    SendMessage(hDlgCtrl, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)NULL);
    SendMessage(hDlgCtrl, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    return TRUE;
}


static VOID
RefreshCursorList(HWND hwndDlg)
{
    TCHAR szCursorName[MAX_PATH];
    HWND hDlgCtrl;
    LV_ITEM listItem;
    LV_COLUMN column;
    INT index = 0;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_LISTVIEW_CURSOR);
    (void)ListView_DeleteAllItems(hDlgCtrl);

    ZeroMemory(&column, sizeof(LV_COLUMN));
    column.mask      = LVCF_SUBITEM | LVCF_WIDTH;
    column.iSubItem  = 0;
    column.cx        = 200;

    (void)ListView_InsertColumn(hDlgCtrl, 0, &column);

    LoadString(hApplet, IDS_ARROW, szCursorName, MAX_PATH);

    ZeroMemory(&listItem, sizeof(LV_ITEM));
    listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    listItem.pszText    = szCursorName;
    listItem.state      = 0;
    listItem.iImage     = -1;
    listItem.iItem      = index++;
    listItem.lParam     = 0;

    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_HELP, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_APPSTARTING, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_WAIT, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_CROSSHAIR, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_IBEAM, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_NWPEN, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_NO, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_SIZENS, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);


    LoadString(hApplet, IDS_SIZENWSE, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_SIZENESW, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_SIZEALL, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_UPARROW, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);

    LoadString(hApplet, IDS_HAND, szCursorName, MAX_PATH);
    listItem.iItem      = index++;
    (void)ListView_InsertItem(hDlgCtrl, &listItem);
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
    HKEY hCursorKey;
    TCHAR szValue[2048];
    TCHAR szRaw[256];
    DWORD dwValueLen;
    LONG lError;
    LPTSTR ptrStart, ptrEnd;
    INT_PTR len;
    int i;

    if (lpName == NULL)
    {
        memset(g_szArrow, 0x0, sizeof(g_szArrow));
        memset(g_szHelp, 0x0, sizeof(g_szHelp));
        memset(g_szAppStarting, 0x0, sizeof(g_szAppStarting));
        memset(g_szWait, 0x0, sizeof(g_szWait));
        memset(g_szCrosshair, 0x0, sizeof(g_szCrosshair));
        memset(g_szIBeam, 0x0, sizeof(g_szIBeam));
        memset(g_szNWPen, 0x0, sizeof(g_szNWPen));
        memset(g_szNo, 0x0, sizeof(g_szNo));
        memset(g_szSizeNS, 0x0, sizeof(g_szSizeNS));
        memset(g_szSizeWE, 0x0, sizeof(g_szSizeWE));
        memset(g_szSizeNWSE, 0x0, sizeof(g_szSizeNWSE));
        memset(g_szSizeNESW, 0x0, sizeof(g_szSizeNESW));
        memset(g_szSizeAll, 0x0, sizeof(g_szSizeAll));
        memset(g_szUpArrow, 0x0, sizeof(g_szUpArrow));
        memset(g_szHand, 0x0, sizeof(g_szHand));
    }
    else
    {
        if (bSystem)
        {
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cursors\\Schemes"),
                             0, KEY_READ, &hCursorKey))
                return;
        }
        else
        {
            if (RegOpenKeyEx(HKEY_CURRENT_USER,
                             _T("Control Panel\\Cursors\\Schemes"),
                             0, KEY_READ, &hCursorKey) != ERROR_SUCCESS)
                return;
        }

        dwValueLen = 2048 * sizeof(TCHAR);
        lError = RegQueryValueEx(hCursorKey, lpName, NULL, NULL, (LPBYTE)szValue, &dwValueLen);

        RegCloseKey(hCursorKey);

        if (lError == ERROR_SUCCESS)
        {
            ptrStart = szValue;
            for (i = 0; ; i++)
            {
                ptrEnd = _tcschr(ptrStart, TEXT(','));
                if (ptrEnd != NULL)
                {
                    len = (ptrEnd - ptrStart) / sizeof(TCHAR);
                    _tcsncpy(szRaw, ptrStart, len);
                    szRaw[len] = 0;
                }
                else
                {
                    _tcscpy(szRaw, ptrStart);
                }

                switch (i)
                {
                    case 0:
                        ExpandEnvironmentStrings(szRaw, g_szArrow, MAX_PATH);
                        break;

                }



                if (ptrEnd == NULL)
                    break;

                ptrStart = ptrEnd + 1;
            }
        }
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
    HDC memDC;
    HCURSOR hCursor;
    LRESULT lResult;

    PPOINTER_DATA pPointerData;

    pPointerData = (PPOINTER_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pPointerData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POINTER_DATA));
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pPointerData);

            EnumerateCursorSchemes(hwndDlg);
            RefreshCursorList(hwndDlg);

            /* Get drop shadow setting */
            if (!SystemParametersInfo(SPI_GETDROPSHADOW, 0, &pPointerData->bDropShadow, 0))
                pPointerData->bDropShadow = FALSE;

            pPointerData->bOrigDropShadow = pPointerData->bDropShadow;

            if (pPointerData->bDropShadow)
            {
                hDlgCtrl = GetDlgItem(hwndDlg, IDC_CHECK_DROP_SHADOW);
                SendMessage(hDlgCtrl, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            }

            if ((INT)wParam == IDC_LISTVIEW_CURSOR)
                return TRUE;
            else
                return FALSE;

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
            switch(HIWORD(wParam))
            {
                case CBN_SELENDOK:
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
                    break;
                }
            }

            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_SAVEAS_SCHEME:
                    if (DialogBox(hApplet, MAKEINTRESOURCE(IDD_CURSOR_SCHEME_SAVEAS), hwndDlg, SaveSchemeProc))
                    {
                        //FIXME
                        //save cursor scheme
                    }
                    break;

                case IDC_BUTTON_USE_DEFAULT_CURSOR:
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_LISTVIEW_CURSOR);
                    lResult = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
                    if (lResult != CB_ERR)
                    {
                        if ((INT)lResult == 0)
                            memset(g_szArrow, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 1)
                            memset(g_szHelp, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 2)
                            memset(g_szAppStarting, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 3)
                            memset(g_szWait, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 4)
                            memset(g_szCrosshair, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 5)
                            memset(g_szIBeam, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 6)
                            memset(g_szNWPen, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 7)
                            memset(g_szNo, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 8)
                            memset(g_szSizeNS, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 9)
                            memset(g_szSizeWE,0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 10)
                            memset(g_szSizeNWSE, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 11)
                            memset(g_szSizeNESW, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 12)
                            memset(g_szSizeAll, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 13)
                            memset(g_szUpArrow, 0x0, MAX_PATH * sizeof(TCHAR));
                        else if ((INT)lResult == 14)
                            memset(g_szHand, 0x0, MAX_PATH * sizeof(TCHAR));
                    }
                    break;

                case IDC_BUTTON_BROWSE_CURSOR:
                    memset(buffer, 0x0, sizeof(buffer));
                    hDlgCtrl = GetDlgItem(hwndDlg, IDC_LISTVIEW_CURSOR);
                    lResult = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
                    if (lResult == CB_ERR)
                    MessageBox(hwndDlg, _T("CB_ERR"), _T(""),MB_ICONERROR);
                    if (BrowseCursor(buffer, hwndDlg))
                    {
                        if ((INT)lResult == 0)
                            _tcsncpy(g_szArrow, buffer, MAX_PATH);
                        else if ((INT)lResult == 1)
                            _tcsncpy(g_szHelp, buffer, MAX_PATH);
                        else if ((INT)lResult == 2)
                            _tcsncpy(g_szAppStarting, buffer, MAX_PATH);
                        else if ((INT)lResult == 3)
                            _tcsncpy(g_szWait, buffer, MAX_PATH);
                        else if ((INT)lResult == 4)
                            _tcsncpy(g_szCrosshair, buffer, MAX_PATH);
                        else if ((INT)lResult == 5)
                            _tcsncpy(g_szIBeam, buffer, MAX_PATH);
                        else if ((INT)lResult == 6)
                            _tcsncpy(g_szNWPen, buffer, MAX_PATH);
                        else if ((INT)lResult == 7)
                            _tcsncpy(g_szNo, buffer, MAX_PATH);
                        else if ((INT)lResult == 8)
                            _tcsncpy(g_szSizeNS, buffer, MAX_PATH);
                        else if ((INT)lResult == 9)
                            _tcsncpy(g_szSizeWE, buffer, MAX_PATH);
                        else if ((INT)lResult == 10)
                            _tcsncpy(g_szSizeNWSE, buffer, MAX_PATH);
                        else if ((INT)lResult == 11)
                            _tcsncpy(g_szSizeNESW, buffer, MAX_PATH);
                        else if ((INT)lResult == 12)
                            _tcsncpy(g_szSizeAll, buffer, MAX_PATH);
                        else if ((INT)lResult == 13)
                            _tcsncpy(g_szUpArrow, buffer, MAX_PATH);
                        else if ((INT)lResult == 14)
                            _tcsncpy(g_szHand, buffer, MAX_PATH);

                        //FIXME
                        //clear screen

                        hDlgCtrl = GetDlgItem(hwndDlg, IDC_IMAGE_CURRENT_CURSOR);
                        memDC = GetDC(hDlgCtrl);
                        hCursor = (HCURSOR) LoadImage(NULL, buffer, IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_SHARED);
                        if (hCursor)
                        {
                            DrawIcon(memDC, 10, 10, (HICON)hCursor);
                            DestroyCursor(hCursor);
                        }
                        ReleaseDC(hDlgCtrl, memDC);
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
