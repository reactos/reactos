/*
 *  ReactOS
 *  Copyright (C) 2007 ReactOS Team
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
/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/advanced.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "resource.h"
#include "input.h"

static VOID
InitControls(HWND hWnd)
{
    HKEY hKey;
    DWORD dwValue, dwType, dwSize;

    if (SendDlgItemMessage(hWnd, IDC_TURNOFF_ADV_TXTSERV_CHECKBOX, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        EnableWindow(GetDlgItem(hWnd, IDC_SUPPORT_ADV_SERV_CHECKBOX),FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_SUPPORT_ADV_SERV_TEXT),FALSE);
    }
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\CTF", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey,
                            L"Disable Thread Input Manager",
                            NULL,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (dwValue == 0x0)
                SendDlgItemMessage(hWnd, IDC_TURNOFF_ADV_TXTSERV_CHECKBOX, BM_SETCHECK, 0, 0);
            if (dwValue == 0x1)
                SendDlgItemMessage(hWnd, IDC_TURNOFF_ADV_TXTSERV_CHECKBOX, BM_SETCHECK, 1, 1);
        }
    }
    RegCloseKey(hKey);
}

static VOID
SaveSettings(HWND hWnd)
{
    HKEY hKey;
    DWORD dwValue, dwType, dwSize;

    if (RegOpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\CTF",&hKey) == ERROR_SUCCESS)
    {
        dwType = REG_DWORD;
        dwSize = sizeof(DWORD);
        if (SendDlgItemMessage(hWnd, IDC_TURNOFF_ADV_TXTSERV_CHECKBOX, BM_GETCHECK, 0, 0) == BST_CHECKED)
            dwValue = 0x1;
        else
            dwValue = 0x0;
		RegSetValueEx(hKey,
                      L"Disable Thread Input Manager",
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwValue,
                      sizeof(DWORD));
    }
    RegCloseKey(hKey);
}

/* Property page dialog callback */
INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            InitControls(hwndDlg);
        }
            break;
        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            if (lpnm->code == (UINT)PSN_APPLY)
            {
                SaveSettings(hwndDlg);
            }
        }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SUPPORT_ADV_SERV_CHECKBOX:
                case IDC_TURNOFF_ADV_TXTSERV_CHECKBOX:
                {
                    if (SendDlgItemMessage(hwndDlg, IDC_TURNOFF_ADV_TXTSERV_CHECKBOX, BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SUPPORT_ADV_SERV_CHECKBOX),FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SUPPORT_ADV_SERV_TEXT),FALSE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SUPPORT_ADV_SERV_CHECKBOX),TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SUPPORT_ADV_SERV_TEXT),TRUE);
                    }
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
