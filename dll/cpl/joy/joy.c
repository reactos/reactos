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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 *
 * PROJECT:         ReactOS Software Control Panel
 * FILE:            dll/cpl/joy/joy.c
 * PURPOSE:         ReactOS Software Control Panel
 * PROGRAMMER:      Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *	10-18-2007  Created
 */

#include "joy.h"

#define NUM_APPLETS	(1)

LONG CALLBACK SystemApplet(VOID);
HINSTANCE hApplet = 0;
HWND MainDlg;

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
            break;

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
                    // not implemented
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
            break;

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
                    // not implemented
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
                    // not implemented
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
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            AddColumns(GetDlgItem(hwndDlg,IDC_CONTROLLER_LIST));
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
                    EndDialog(hwndDlg,LOWORD(wParam));
                    break;

                case SC_CONTEXTHELP:
                    // not implemented
                    break;
            }
            break;
    }
    return 0;
}

/* First Applet */
LONG CALLBACK
SystemApplet(VOID)
{
    DialogBox(hApplet,
              MAKEINTRESOURCE(IDD_PROPPAGEMAIN),
              MainDlg,
              MainPageProc);

    return (LONG)TRUE;
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    CPLINFO *CPlInfo;
    DWORD i;

    UNREFERENCED_PARAMETER(hwndCPl);

    i = (DWORD)lParam1;
    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            CPlInfo = (CPLINFO*)lParam2;
            CPlInfo->lData = 0;
            CPlInfo->idIcon = Applets[i].idIcon;
            CPlInfo->idName = Applets[i].idName;
            CPlInfo->idInfo = Applets[i].idDescription;
            break;

        case CPL_DBLCLK:
            {
                MainDlg = hwndCPl;
                Applets[i].AppletProc();
            }
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
