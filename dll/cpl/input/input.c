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

static LONG CALLBACK SystemApplet(VOID);

HINSTANCE hApplet = NULL;
static HWND hCPLWindow;

/* Applets */
static APPLET Applets[NUM_APPLETS] =
{
    {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};


static VOID
InitPropSheetPage(PROPSHEETPAGEW *page, WORD idDlg, DLGPROC DlgProc)
{
    ZeroMemory(page, sizeof(*page));

    page->dwSize      = sizeof(*page);
    page->dwFlags     = PSP_DEFAULT;
    page->hInstance   = hApplet;
    page->pszTemplate = MAKEINTRESOURCEW(idDlg);
    page->pfnDlgProc  = DlgProc;
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
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_KEY_SHORT_ICO));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

/* First Applet */
static LONG CALLBACK
SystemApplet(VOID)
{
    PROPSHEETPAGEW page[2];
    PROPSHEETHEADERW header;
    WCHAR szCaption[MAX_STR_LEN];

    LoadStringW(hApplet, IDS_CPLSYSTEMNAME, szCaption, ARRAYSIZE(szCaption));

    ZeroMemory(&header, sizeof(header));

    header.dwSize      = sizeof(header);
    header.dwFlags     = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    header.hwndParent  = hCPLWindow;
    header.hInstance   = hApplet;
    header.pszIcon     = MAKEINTRESOURCEW(IDI_KEY_SHORT_ICO);
    header.pszCaption  = szCaption;
    header.nPages      = ARRAYSIZE(page);
    header.nStartPage  = 0;
    header.ppsp        = page;
    header.pfnCallback = PropSheetProc;

    /* Settings */
    InitPropSheetPage(&page[0], IDD_PROPPAGESETTINGS, SettingsPageProc);

    /* Advanced Settings */
    InitPropSheetPage(&page[1], IDD_PROPPAGEADVANCEDSETTINGS, AdvancedSettingsPageProc);

    return (LONG)(PropertySheetW(&header) != -1);
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
