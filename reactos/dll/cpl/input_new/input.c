/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/input.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (lentind@yandex.ru)
 *                  Colin Finck
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "input.h"

#include <cpl.h>

#define NUM_APPLETS    (1)

LONG CALLBACK SystemApplet(VOID);
HINSTANCE hApplet = NULL;
HWND hCPLWindow;

/* Applets */
static APPLET Applets[NUM_APPLETS] =
{
    {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};


VOID
InitPropSheetPage(PROPSHEETPAGE *page, WORD idDlg, DLGPROC DlgProc)
{
    memset(page, 0, sizeof(PROPSHEETPAGE));

    page->dwSize      = sizeof(PROPSHEETPAGE);
    page->dwFlags     = PSP_DEFAULT;
    page->hInstance   = hApplet;
    page->pszTemplate = MAKEINTRESOURCE(idDlg);
    page->pfnDlgProc  = DlgProc;
}


INT_PTR CALLBACK
AdvancedSettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}


/* First Applet */
LONG CALLBACK
SystemApplet(VOID)
{
    PROPSHEETPAGE page[2];
    PROPSHEETHEADER header;
    WCHAR szCaption[MAX_STR_LEN];

    LoadString(hApplet, IDS_CPLSYSTEMNAME, szCaption, ARRAYSIZE(szCaption));

    memset(&header, 0, sizeof(PROPSHEETHEADER));

    header.dwSize      = sizeof(PROPSHEETHEADER);
    header.dwFlags     = PSH_PROPSHEETPAGE;
    header.hwndParent  = hCPLWindow;
    header.hInstance   = hApplet;
    header.hIcon       = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
    header.pszCaption  = szCaption;
    header.nPages      = ARRAYSIZE(page);
    header.nStartPage  = 0;
    header.ppsp        = page;
    header.pfnCallback = NULL;

    /* Settings */
    InitPropSheetPage(&page[0], IDD_PROPPAGESETTINGS, SettingsPageProc);

    /* Advanced Settings */
    InitPropSheetPage(&page[1], IDD_PROPPAGEADVANCEDSETTINGS, AdvancedSettingsPageProc);

    return (LONG)(PropertySheet(&header) != -1);
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    CPLINFO *CPlInfo;
    DWORD i;

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
            hCPLWindow = hwndCPl;
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

/* EOF */
