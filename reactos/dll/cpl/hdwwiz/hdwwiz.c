/*
 * ReactOS New devices installation
 * Copyright (C) 2005 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * PROJECT:         ReactOS Add hardware control panel
 * FILE:            dll/cpl/hdwwiz/hdwwiz.c
 * PURPOSE:         ReactOS Add hardware control panel
 * PROGRAMMER:      Hervé Poussineau (hpoussin@reactos.org)
 *                  Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <cpl.h>
#include <tchar.h>
#include <stdio.h>

#include "resource.h"
#include "hdwwiz.h"

HINSTANCE hApplet = NULL;

typedef BOOL (WINAPI *PINSTALL_NEW_DEVICE)(HWND, LPGUID, PDWORD);


BOOL CALLBACK
InstallNewDevice(HWND hwndParent, LPGUID ClassGuid, PDWORD pReboot)
{
    return FALSE;
}

static INT_PTR CALLBACK
StartPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

static INT_PTR CALLBACK
SearchPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

static INT_PTR CALLBACK
IsConnctedPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

static INT_PTR CALLBACK
FinishPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

static VOID
HardwareWizardInit(HWND hwnd)
{
    HPROPSHEETPAGE ahpsp[3];
    PROPSHEETPAGE psp = {0};
    PROPSHEETHEADER psh;
    UINT nPages = 0;

    /* Create the Start page, until setup is working */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = StartPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create search page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SEARCHTITLE);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = SearchPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SEARCHPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create is connected page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ISCONNECTED);
    psp.pszHeaderSubTitle = NULL;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = IsConnctedPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ISCONNECTEDPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create finish page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.lParam = 0;
    psp.pfnDlgProc = FinishPageDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hApplet;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Display the wizard */
    PropertySheet(&psh);
}

VOID CALLBACK
AddHardwareWizard(HWND hwnd, LPTSTR lpName)
{
    HardwareWizardInit(hwnd);
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = IDI_CPLICON;
                CPlInfo->idName = IDS_CPLNAME;
                CPlInfo->idInfo = IDS_CPLDESCRIPTION;
            }
            break;

        case CPL_DBLCLK:
            AddHardwareWizard(hwndCpl, NULL);
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
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}
