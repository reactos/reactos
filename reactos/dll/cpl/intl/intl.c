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
 * PROJECT:         ReactOS International Control Panel
 * FILE:            dll/cpl/intl/intl.c
 * PURPOSE:         Property sheet code
 * PROGRAMMER:      Eric Kohl
 */

#include "intl.h"

#include <debug.h>

#define NUM_APPLETS    (1)

#define BUFFERSIZE 512

static LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);


HINSTANCE hApplet = 0;
HWND hCPLWindow;
HINF hSetupInf = INVALID_HANDLE_VALUE;
DWORD IsUnattendedSetupEnabled = 0;
DWORD UnattendLCID = 0;


/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};

VOID
PrintErrorMsgBox(UINT msg)
{
    TCHAR szErrorText[BUFFERSIZE];
    TCHAR szErrorCaption[BUFFERSIZE];

    LoadString(hApplet, msg, szErrorText, sizeof(szErrorText)/sizeof(TCHAR));
    LoadString(hApplet, IDS_ERROR, szErrorCaption, sizeof(szErrorCaption)/sizeof(TCHAR));

    MessageBox(NULL, szErrorText, szErrorCaption, MB_OK | MB_ICONERROR);
}

VOID
ResourceMessageBox(
    HWND hwnd,
    UINT uType,
    UINT uCaptionId,
    UINT uMessageId)
{
    WCHAR szErrorText[BUFFERSIZE];
    WCHAR szErrorCaption[BUFFERSIZE];

    LoadStringW(hApplet, uMessageId, szErrorText, sizeof(szErrorText) / sizeof(WCHAR));
    LoadStringW(hApplet, uCaptionId, szErrorCaption, sizeof(szErrorCaption) / sizeof(WCHAR));

    MessageBoxW(hwnd, szErrorText, szErrorCaption, uType);
}

static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc, LPARAM lParam)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGE));
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = DlgProc;
    psp->lParam = lParam;
}

BOOL
OpenSetupInf(VOID)
{
    PWSTR lpCmdLine;
    PWSTR lpSwitch;
    size_t len;

    lpCmdLine = GetCommandLineW();

    lpSwitch = wcsstr(lpCmdLine, L"/f:\"");
    if (!lpSwitch)
        return FALSE;

    len = wcslen(lpSwitch);
    if (len < 5 || lpSwitch[len-1] != L'\"')
    {
        DPRINT1("Invalid switch: %ls\n", lpSwitch);
        return FALSE;
    }

    lpSwitch[len-1] = L'\0';

    hSetupInf = SetupOpenInfFileW(&lpSwitch[4], NULL, INF_STYLE_OLDNT, NULL);
    if (hSetupInf == INVALID_HANDLE_VALUE)
    {
        DPRINT1("Failed to open INF file: %ls\n", &lpSwitch[4]);
        return FALSE;
    }

    return TRUE;
}

VOID
ParseSetupInf(VOID)
{
    INFCONTEXT InfContext;
    WCHAR szBuffer[30];

    if (!SetupFindFirstLineW(hSetupInf,
                             L"Unattend",
                             L"LocaleID",
                             &InfContext))
    {
        SetupCloseInfFile(hSetupInf);
        DPRINT1("SetupFindFirstLine failed\n");
        return;
    }

    if (!SetupGetStringFieldW(&InfContext, 1, szBuffer,
                              sizeof(szBuffer) / sizeof(WCHAR), NULL))
    {
        SetupCloseInfFile(hSetupInf);
        DPRINT1("SetupGetStringField failed\n");
        return;
    }

    UnattendLCID = wcstoul(szBuffer, NULL, 16);
    IsUnattendedSetupEnabled = 1;
    SetupCloseInfFile(hSetupInf);
}

static LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    TCHAR Caption[BUFFERSIZE];
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;
    PGLOBALDATA pGlobalData;
    LONG ret;

    if (OpenSetupInf())
    {
        ParseSetupInf();
    }

    pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBALDATA));
    if (pGlobalData == NULL)
        return FALSE;

    pGlobalData->SystemLCID = GetSystemDefaultLCID();

    LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = hCPLWindow;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_GENERALPAGE, GeneralPageProc, (LPARAM)pGlobalData);
    InitPropSheetPage(&psp[1], IDD_LANGUAGESPAGE, LanguagesPageProc, (LPARAM)pGlobalData);
    InitPropSheetPage(&psp[2], IDD_ADVANCEDPAGE, AdvancedPageProc, (LPARAM)pGlobalData);

    ret = (LONG)(PropertySheet(&psh) != -1);

    HeapFree(GetProcessHeap(), 0, pGlobalData);

    return ret;
}


/* Control Panel Callback */
LONG APIENTRY
CPlApplet(HWND hwndCpl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    switch(uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
        {
            CPLINFO *CPlInfo = (CPLINFO*)lParam2;
            UINT uAppIndex = (UINT)lParam1;

            CPlInfo->lData = 0;
            CPlInfo->idIcon = Applets[uAppIndex].idIcon;
            CPlInfo->idName = Applets[uAppIndex].idName;
            CPlInfo->idInfo = Applets[uAppIndex].idDescription;
            break;
        }

        case CPL_DBLCLK:
        {
            UINT uAppIndex = (UINT)lParam1;
            hCPLWindow = hwndCpl;
            Applets[uAppIndex].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            break;
        }
    }

    return FALSE;
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpReserved)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            hApplet = hinstDLL;
            break;
    }

  return TRUE;
}
