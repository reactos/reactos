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

#define NUM_APPLETS    (1)

static LONG CALLBACK SystemApplet(HWND hwnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

HINSTANCE hApplet = NULL;
BOOL g_bRebootNeeded = FALSE;

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

static BOOL AskForReboot(HWND hwndDlg)
{
    WCHAR szText[128], szCaption[64];
    LoadStringW(hApplet, IDS_REBOOT_NOW, szText, _countof(szText));
    LoadStringW(hApplet, IDS_LANGUAGE, szCaption, _countof(szCaption));
    return (MessageBoxW(hwndDlg, szText, szCaption, MB_ICONINFORMATION | MB_YESNO) == IDYES);
}

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            /* Set large icon correctly */
            HICON hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_CPLSYSTEM));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }

        case PSCB_BUTTONPRESSED:
        {
            switch (lParam)
            {
                case PSBTN_OK:
                case PSBTN_APPLYNOW:
                {
                    if (g_bRebootNeeded && AskForReboot(hwndDlg))
                    {
                        EnableProcessPrivileges(SE_SHUTDOWN_NAME, TRUE);
                        ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
                    }
                    break;
                }
            }
            break;
        }
    }
    return 0;
}

/* First Applet */
static LONG CALLBACK
SystemApplet(HWND hwnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    PROPSHEETPAGEW page[2];
    PROPSHEETHEADERW header;

    ZeroMemory(&header, sizeof(header));

    header.dwSize      = sizeof(header);
    header.dwFlags     = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    header.hwndParent  = hwnd;
    header.hInstance   = hApplet;
    header.pszIcon     = MAKEINTRESOURCEW(IDI_CPLSYSTEM);
    header.pszCaption  = MAKEINTRESOURCEW(IDS_CPLSYSTEMNAME);
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
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}

/* EOF */
