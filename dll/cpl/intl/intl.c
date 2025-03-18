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
#include <setupapi_undoc.h> // For IsUserAdmin()

#include <debug.h>

#define NUM_APPLETS    (1)

#define BUFFERSIZE 512

static LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);


HINSTANCE hApplet = 0;
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
    WCHAR szErrorText[BUFFERSIZE];
    WCHAR szErrorCaption[BUFFERSIZE];

    LoadStringW(hApplet, msg, szErrorText, sizeof(szErrorText) / sizeof(WCHAR));
    LoadStringW(hApplet, IDS_ERROR, szErrorCaption, sizeof(szErrorCaption) / sizeof(WCHAR));

    MessageBoxW(NULL, szErrorText, szErrorCaption, MB_OK | MB_ICONERROR);
}

INT
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

    return MessageBoxW(hwnd, szErrorText, szErrorCaption, uType);
}

static VOID
InitIntlPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc, LPARAM lParam)
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

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDC_CPLICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

static LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    PROPSHEETPAGE psp[3];
    PROPSHEETHEADER psh;
    PGLOBALDATA pGlobalData;
    INT nPage = 0;
    LONG ret;

    if (OpenSetupInf())
    {
        ParseSetupInf();
    }

    if (uMsg == CPL_STARTWPARMSW && lParam != 0)
        nPage = _wtoi((PWSTR)lParam);

    pGlobalData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBALDATA));
    if (pGlobalData == NULL)
        return FALSE;

    pGlobalData->SystemLCID = GetSystemDefaultLCID();
    pGlobalData->bIsUserAdmin = IsUserAdmin();

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDC_CPLICON);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pfnCallback = PropSheetProc;

    InitIntlPropSheetPage(&psp[0], IDD_GENERALPAGE, GeneralPageProc, (LPARAM)pGlobalData);
    psh.nPages++;
    InitIntlPropSheetPage(&psp[1], IDD_LANGUAGESPAGE, LanguagesPageProc, (LPARAM)pGlobalData);
    psh.nPages++;

    if (pGlobalData->bIsUserAdmin)
    {
        InitIntlPropSheetPage(&psp[2], IDD_ADVANCEDPAGE, AdvancedPageProc, (LPARAM)pGlobalData);
        psh.nPages++;
    }

    if (nPage != 0 && nPage <= psh.nPages)
        psh.nStartPage = nPage;

    ret = (LONG)(PropertySheet(&psh) != -1);
    if (ret > 0)
        SendMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"intl");

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
    UINT i = (UINT)lParam1;

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            if (i < NUM_APPLETS)
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
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
                Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            else
                return TRUE;
            break;

        case CPL_STARTWPARMSW:
            if (i < NUM_APPLETS)
                return Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            break;
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
