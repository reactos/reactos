/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/sysdm.c
 * PURPOSE:     dll entry file
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *
 */

#include "precomp.h"

#include <regstr.h>

static LONG APIENTRY SystemApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};

#define MAX_SYSTEM_PAGES    32


INT
ResourceMessageBox(
    IN HINSTANCE hInstance,
    IN HWND hwnd,
    IN UINT uType,
    IN UINT uCaption,
    IN UINT uText)
{
    WCHAR szCaption[256];
    WCHAR szText[256];

    LoadStringW(hInstance, uCaption, szCaption, 256);
    LoadStringW(hInstance, uText, szText, 256);

    return MessageBoxW(hwnd,
                       szText,
                       szCaption,
                       uType);
}


static BOOL CALLBACK
PropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_SYSTEM_PAGES)
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

    if (ppsh->nPages < MAX_SYSTEM_PAGES)
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

typedef HPROPSHEETPAGE (WINAPI *PCreateNetIDPropertyPage)(VOID);

static HMODULE
AddNetIdPage(PROPSHEETHEADER *ppsh)
{
    HPROPSHEETPAGE hPage;
    HMODULE hMod;
    PCreateNetIDPropertyPage pCreateNetIdPage;

    hMod = LoadLibrary(TEXT("netid.dll"));
    if (hMod != NULL)
    {
        pCreateNetIdPage = (PCreateNetIDPropertyPage)GetProcAddress(hMod,
                                                                    "CreateNetIDPropertyPage");
        if (pCreateNetIdPage != NULL)
        {
            hPage = pCreateNetIdPage();
            if (hPage == NULL)
                goto Fail;

            if (!PropSheetAddPage(hPage, (LPARAM)ppsh))
            {
                DestroyPropertySheetPage(hPage);
                goto Fail;
            }
        }
        else
        {
Fail:
            FreeLibrary(hMod);
            hMod = NULL;
        }
    }

    return hMod;
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
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_USERPROF));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

/* First Applet */
LONG CALLBACK
SystemApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    HPROPSHEETPAGE hpsp[MAX_SYSTEM_PAGES];
    PROPSHEETHEADER psh;
    HMODULE hNetIdDll;
    HPSXA hpsxa = NULL;
    INT nPage = 0;
    LONG Ret;
    static INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_LINK_CLASS};

    if (!InitCommonControlsEx(&icc))
        return 0;

    if (uMsg == CPL_STARTWPARMSW && lParam != 0)
    {
        nPage = _wtoi((PWSTR)lParam);
    }

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_USERPROF);
    psh.pszCaption = MAKEINTRESOURCE(IDS_CPLSYSTEMNAME);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = hpsp;
    psh.pfnCallback = PropSheetProc;

    InitPropSheetPage(&psh, IDD_PROPPAGEGENERAL, GeneralPageProc);
    hNetIdDll = AddNetIdPage(&psh);
    InitPropSheetPage(&psh, IDD_PROPPAGEHARDWARE, HardwarePageProc);
    InitPropSheetPage(&psh, IDD_PROPPAGEADVANCED, AdvancedPageProc);

    /* Load additional pages provided by shell extensions */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\System"), MAX_SYSTEM_PAGES - psh.nPages);
    if (hpsxa != NULL)
    {
        SHAddFromPropSheetExtArray(hpsxa, PropSheetAddPage, (LPARAM)&psh);
    }

    if (nPage != 0 && nPage <= psh.nPages)
        psh.nStartPage = nPage;

    Ret = (LONG)(PropertySheet(&psh) != -1);

    if (hpsxa != NULL)
    {
        SHDestroyPropSheetExtArray(hpsxa);
    }

    if (hNetIdDll != NULL)
        FreeLibrary(hNetIdDll);

    return Ret;
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    INT i = (INT)lParam1;

    UNREFERENCED_PARAMETER(hwndCPl);

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

        case CPL_STARTWPARMSW:
            return Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);

    }

    return FALSE;
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
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
