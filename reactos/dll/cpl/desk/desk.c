/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/desk.c
 * PURPOSE:         ReactOS Display Control Panel
 * 
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include "desk.h"
#include "preview.h"

#define NUM_APPLETS	(1)

static LONG APIENTRY DisplayApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

extern INT_PTR CALLBACK BackgroundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK ScreenSaverPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
    {
        IDC_DESK_ICON,
        IDS_CPLNAME,
        IDS_CPLDESCRIPTION,
        DisplayApplet
    }
};

static BOOL CALLBACK
PropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_DESK_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}

static BOOL
InitPropSheetPage(PROPSHEETHEADER *ppsh, WORD idDlg, DLGPROC DlgProc)
{
    HPROPSHEETPAGE hPage;
    PROPSHEETPAGE psp;

    if (ppsh->nPages < MAX_DESK_PAGES)
    {
        ZeroMemory(&psp, sizeof(psp));
        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hApplet;
        psp.pszTemplate = MAKEINTRESOURCE(idDlg);
        psp.pfnDlgProc = DlgProc;

        hPage = CreatePropertySheetPage(&psp);
        if (hPage != NULL)
        {
            return PropSheetAddPage(hPage, (LPARAM)ppsh);
        }
    }

    return FALSE;
}

static const struct
{
    WORD idDlg;
    DLGPROC DlgProc;
} PropPages[] =
{
    { IDD_BACKGROUND, BackgroundPageProc },
    { IDD_SCREENSAVER, ScreenSaverPageProc },
    { IDD_APPEARANCE, AppearancePageProc },
    { IDD_SETTINGS, SettingsPageProc },
};

/* Display Applet */
static LONG APIENTRY
DisplayApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    HPROPSHEETPAGE hpsp[MAX_DESK_PAGES];
    PROPSHEETHEADER psh;
    HPSXA hpsxa;
    TCHAR Caption[1024];
    LONG ret;
    UINT i;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USECALLBACK | PSH_PROPTITLE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_DESK_ICON));
    psh.pszCaption = Caption;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = hpsp;

    /* Allow shell extensions to replace the background page */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Desk"), MAX_DESK_PAGES - psh.nPages);

    for (i = 0; i != sizeof(PropPages) / sizeof(PropPages[0]); i++)
    {
        /* Override the background page if requested by a shell extension */
        if (PropPages[i].idDlg == IDD_BACKGROUND && hpsxa != NULL &&
            SHReplaceFromPropSheetExtArray(hpsxa, CPLPAGE_DISPLAY_BACKGROUND, PropSheetAddPage, (LPARAM)&psh) != 0)
        {
            /* The shell extension added one or more pages to replace the background page.
               Don't create the built-in page anymore! */
            continue;
        }

        InitPropSheetPage(&psh, PropPages[i].idDlg, PropPages[i].DlgProc);
    }

    /* NOTE: Don;t call SHAddFromPropSheetExtArray here because this applet only allows
             replacing the background page but not extending the applet by more pages */

    ret = (LONG)(PropertySheet(&psh) != -1);

    if (hpsxa != NULL)
        SHDestroyPropSheetExtArray(hpsxa);

    return ret;
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    int i = (int)lParam1;

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            break;

        case CPL_DBLCLK:
            Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            break;
    }

    return FALSE;
}


BOOL WINAPI
DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            RegisterPreviewControl(hInstDLL);
//        case DLL_THREAD_ATTACH:
            hApplet = hInstDLL;
            break;

        case DLL_PROCESS_DETACH:
            UnregisterPreviewControl(hInstDLL);
            break;
    }

    return TRUE;
}
