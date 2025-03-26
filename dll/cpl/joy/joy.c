/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 *
 * PROJECT:         ReactOS Software Control Panel
 * FILE:            dll/cpl/joy/joy.c
 * PURPOSE:         ReactOS Software Control Panel
 * PROGRAMMER:      Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *    10-18-2007  Created
 *    05-18-2020  Updated (init of dialog and combobox)
 */

#include "joy.h"

#define NUM_APPLETS    (1)

LONG CALLBACK SystemApplet(HWND hwnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
HINSTANCE hApplet = 0;

/* Applets */

APPLET Applets[NUM_APPLETS] =
{
    {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};

VOID
AddColumns(HWND hList)
{
    LV_COLUMN dummy;
    RECT rect;
    int Size;
    TCHAR szBuf[256];

    GetClientRect(hList, &rect);
    Size = rect.right - rect.left - 250;

    LoadString(hApplet, IDS_STATUS, szBuf, sizeof(szBuf) / sizeof(TCHAR));

    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask     = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    dummy.iSubItem = 0;
    dummy.pszText  = (LPTSTR)szBuf;
    dummy.cx       = Size;
    (void)ListView_InsertColumn(hList, 0, &dummy);

    GetClientRect(hList, &rect);

    LoadString(hApplet, IDS_CONTROLLER, szBuf, sizeof(szBuf) / sizeof(TCHAR));

    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask     = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    dummy.iSubItem = 0;
    dummy.pszText  = (LPTSTR)szBuf;
    dummy.cx       = rect.right - rect.left - Size;
    (void)ListView_InsertColumn(hList, 0, &dummy);
}

INT_PTR CALLBACK
AdvancedPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            WCHAR szBuf[256];
            HWND hComboHwnd = GetDlgItem(hwndDlg,IDC_PREFERRED_DEV_COMBO);

            LoadStringW(hApplet, IDS_NONE, szBuf, _countof(szBuf));
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            SendMessageW(hComboHwnd, CB_SETCURSEL, 0, (LPARAM)NULL);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;
            }
            break;

        case WM_NOTIFY:
            break;

        case WM_SYSCOMMAND:
            switch (LOWORD(wParam))
            {
                case SC_CONTEXTHELP:
                    // Not implemented yet
                    break;
            }
            break;
    }
    return 0;
}

INT_PTR CALLBACK
CustomPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            WCHAR szBuf[2];
            HWND hComboHwnd;
            szBuf[1] = UNICODE_NULL;

            CheckDlgButton(hwndDlg, IDC_JOYSTICK_RADIO, BST_CHECKED);

            hComboHwnd = GetDlgItem(hwndDlg, IDC_AXES_COMBO);
            szBuf[0] = L'2';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            szBuf[0] = L'3';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            szBuf[0] = L'4';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            SendMessageW(hComboHwnd, CB_SETCURSEL, 0, 0);

            hComboHwnd = GetDlgItem(hwndDlg, IDC_BUTTONS_COMBO);
            szBuf[0] = L'0';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            szBuf[0] = L'1';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            szBuf[0] = L'2';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            szBuf[0] = L'3';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            szBuf[0] = L'4';
            SendMessageW(hComboHwnd, CB_ADDSTRING, 0, (LPARAM)szBuf);
            SendMessageW(hComboHwnd, CB_SETCURSEL, 4, 0);

            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;
            }
            break;

        case WM_NOTIFY:
            break;

        case WM_SYSCOMMAND:
            switch (LOWORD(wParam))
            {
                case SC_CONTEXTHELP:
                    // Not implemented yet
                    break;
            }
            break;
    }
    return 0;
}

INT_PTR CALLBACK
AddPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CUSTOM_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_CUSTOM),
                              hwndDlg,
                              CustomPageProc);
                    break;

                case IDOK:
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;
            }
            break;

        case WM_NOTIFY:
            break;

        case WM_SYSCOMMAND:
            switch (LOWORD(wParam))
            {
                case SC_CONTEXTHELP:
                    // Not implemented yet
                    break;
            }
            break;
    }
    return 0;
}

/* Property page dialog callback */
INT_PTR CALLBACK
MainPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HICON s_hIcon = NULL, s_hIconSm = NULL;
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            AddColumns(GetDlgItem(hwndDlg, IDC_CONTROLLER_LIST));
            s_hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_CPLSYSTEM));
            s_hIconSm = (HICON)LoadImageW(hApplet, MAKEINTRESOURCEW(IDI_CPLSYSTEM),
                                          IMAGE_ICON,
                                          GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON), 0);
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)s_hIcon);
            SendMessageW(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)s_hIconSm);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_ADD_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_ADD),
                              hwndDlg,
                              AddPageProc);
                    break;

                case IDC_ADVANCED_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_ADVANCED),
                              hwndDlg,
                              AdvancedPageProc);
                    break;

                case IDOK:
                case IDCANCEL:
                    DestroyIcon(s_hIcon);
                    DestroyIcon(s_hIconSm);
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;
            }
            break;

        case WM_NOTIFY:
            break;

        case WM_SYSCOMMAND:
            switch (LOWORD(wParam))
            {
                case SC_CLOSE:
                    DestroyIcon(s_hIcon);
                    DestroyIcon(s_hIconSm);
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;

                case SC_CONTEXTHELP:
                    // Not implemented yet
                    break;
            }
            break;
    }
    return 0;
}

/* First Applet */
LONG CALLBACK
SystemApplet(HWND hwnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(lParam1);
    UNREFERENCED_PARAMETER(lParam2);

    DialogBox(hApplet,
              MAKEINTRESOURCE(IDD_PROPPAGEMAIN),
              hwnd,
              MainPageProc);

    return (LONG)TRUE;
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    CPLINFO *CPlInfo;
    UINT i = (UINT)lParam1;
    UNREFERENCED_PARAMETER(hwndCPl);

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            if (i < NUM_APPLETS)
            {
                CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            else
            {
                return TRUE;
            }
            break;

        case CPL_DBLCLK:
            if (i < NUM_APPLETS)
                Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            else
                return TRUE;
            break;
    }

    return FALSE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            CoInitialize(NULL);
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}
