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
/* $Id$
 *
 * PROJECT:         ReactOS Software Control Panel
 * FILE:            dll/cpl/appwiz/appwiz.c
 * PURPOSE:         ReactOS Software Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      06-17-2004  Created
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <prsht.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>

#include "resource.h"
#include "appwiz.h"

#define NUM_APPLETS	(1)

LONG CALLBACK SystemApplet(VOID);
INT_PTR CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ComputerPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
    {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};


static VOID
CallUninstall(HWND hwndDlg)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    INT nIndex;
    HKEY hKey;
    DWORD dwType;
    TCHAR pszUninstallString[MAX_PATH];
    DWORD dwSize;

    nIndex = (INT)SendDlgItemMessage(hwndDlg, IDC_SOFTWARELIST, LB_GETCURSEL, 0, 0);
    if (nIndex == -1)
    {
        MessageBox(hwndDlg,
                   _TEXT("No item selected"),
                   _TEXT("Error"),
                   MB_ICONSTOP);
    }
    else
    {
        hKey = (HKEY)SendDlgItemMessage(hwndDlg, IDC_SOFTWARELIST, LB_GETITEMDATA, (WPARAM)nIndex, 0);

        dwType = REG_SZ;
        dwSize = MAX_PATH;
        if (RegQueryValueEx(hKey,
                            _TEXT("UninstallString"),
                            NULL,
                            &dwType,
                            (LPBYTE)pszUninstallString,
                            &dwSize) == ERROR_SUCCESS)
        {
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.wShowWindow = SW_SHOW;
            if (CreateProcess(NULL,pszUninstallString,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
        else
        {
            MessageBox(hwndDlg,
                       _TEXT("Unable to read UninstallString. This entry is invalid or has been created by an MSI installer."),
                       _TEXT("Error"),
                       MB_ICONSTOP);
        }
    }
}


static VOID
FillSoftwareList(HWND hwndDlg)
{
    TCHAR pszName[MAX_PATH];
    TCHAR pszDisplayName[MAX_PATH];
    TCHAR pszParentKeyName[MAX_PATH];
    FILETIME FileTime;
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwValue = 0;
    BOOL bIsUpdate = FALSE;
    BOOL bIsSystemComponent = FALSE;
    BOOL bShowUpdates = FALSE;
    INT i;
    ULONG ulIndex;

    bShowUpdates = (SendMessage(GetDlgItem(hwndDlg, IDC_SHOWUPDATES), BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   _TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                   &hKey) != ERROR_SUCCESS)
    {
        MessageBox(hwndDlg,
                   _TEXT("Unable to open Uninstall Key"),
                   _TEXT("Error"),
                   MB_ICONSTOP);
        return;
    }

    i = 0;
    dwSize = MAX_PATH;
    while (RegEnumKeyEx (hKey, i, pszName, &dwSize, NULL, NULL, NULL, &FileTime) == ERROR_SUCCESS)
    {
        if (RegOpenKey(hKey,pszName,&hSubKey) == ERROR_SUCCESS)
        {
            dwType = REG_DWORD;
            dwSize = sizeof(DWORD);
            if (RegQueryValueEx(hSubKey,
                                _TEXT("SystemComponent"),
                                NULL,
                                &dwType,
                                (LPBYTE)&dwValue,
                                &dwSize) == ERROR_SUCCESS)
            {
                bIsSystemComponent = (dwValue == 0x1);
            }
            else
            {
                bIsSystemComponent = FALSE;
            }

            dwType = REG_SZ;
            dwSize = MAX_PATH;
            bIsUpdate = (RegQueryValueEx(hSubKey,
                                         _TEXT("ParentKeyName"),
                                         NULL,
                                         &dwType,
                                         (LPBYTE)pszParentKeyName,
                                         &dwSize) == ERROR_SUCCESS);
            dwSize = MAX_PATH;
            if (RegQueryValueEx(hSubKey,
                                _TEXT("DisplayName"),
                                NULL,
                                &dwType,
                                (LPBYTE)pszDisplayName,
                                &dwSize) == ERROR_SUCCESS)
            {
                if ((!bIsUpdate) && (!bIsSystemComponent))
                {
                    ulIndex = (ULONG)SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_ADDSTRING,0,(LPARAM)pszDisplayName);
                    SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_SETITEMDATA,ulIndex,(LPARAM)hSubKey);
                }
                else if (bIsUpdate && bShowUpdates)
                {
                    ulIndex = (ULONG)SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_ADDSTRING,0,(LPARAM)pszDisplayName);
                    SendDlgItemMessage(hwndDlg,IDC_SOFTWARELIST,LB_SETITEMDATA,ulIndex,(LPARAM)hSubKey);
                }
            }
        }

        dwSize = MAX_PATH;
        i++;
    }

    RegCloseKey(hKey);
}


/* Property page dialog callback */
static INT_PTR CALLBACK
InstallPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hwndDlg, IDC_INSTALL), FALSE);
            FillSoftwareList(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SHOWUPDATES:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        SendDlgItemMessage(hwndDlg, IDC_SOFTWARELIST, LB_RESETCONTENT, 0, 0);
                        FillSoftwareList(hwndDlg);
                    }
                    break;

                case IDC_SOFTWARELIST:
                    if (HIWORD(wParam) == LBN_DBLCLK)
                    {
                        CallUninstall(hwndDlg);
                    }
                    break;

                case IDC_ADDREMOVE:
                    CallUninstall(hwndDlg);
                    break;
            }
            break;
    }

    return FALSE;
}


/* Property page dialog callback */
static INT_PTR CALLBACK
RosPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;
    }

    return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGE));
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = DlgProc;
}


/* First Applet */

LONG CALLBACK
SystemApplet(VOID)
{
    PROPSHEETPAGE psp[2];
    PROPSHEETHEADER psh;
    TCHAR Caption[1024];

    LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pfnCallback = NULL;

    InitPropSheetPage(&psp[0], IDD_PROPPAGEINSTALL, (DLGPROC) InstallPageProc);
    InitPropSheetPage(&psp[1], IDD_PROPPAGEROSSETUP, (DLGPROC) RosPageProc);

    return (LONG)(PropertySheet(&psh) != -1);
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
            Applets[i].AppletProc();
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
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}

INT_PTR
CALLBACK
WelcomeDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
            break;
        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case EN_CHANGE:
                    if (SendDlgItemMessage(hwndDlg, IDC_SHORTCUT_LOCATION, WM_GETTEXTLENGTH, 0, 0))
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
                    }
            }

    }
    return FALSE;
}

INT_PTR
CALLBACK
FinishDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
            break;
    }
    return FALSE;
}


LONG CALLBACK
NewLinkHere(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    PROPSHEETHEADERW psh;
    HPROPSHEETPAGE ahpsp[2];
    PROPSHEETPAGE psp;
    UINT nPages = 0;

    /* Create the Welcome page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.pfnDlgProc = WelcomeDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SHORTCUT_LOCATION);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Finish page */
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.pfnDlgProc = FinishDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SHORTCUT_FINISH);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);


    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK;
    psh.hInstance = hApplet;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);

    /* Display the wizard */
    PropertySheet(&psh);

    return TRUE;







   return FALSE;
}
