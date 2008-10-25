/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/sysdm.c
 * PURPOSE:     dll entry file
 * COPYRIGHT:   Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *
 */

#include "precomp.h"

LONG CALLBACK SystemApplet(VOID);
HINSTANCE hApplet = 0;
HWND hCPLWindow;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
};

#define MAX_SYSTEM_PAGES    32

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

/* First Applet */
LONG CALLBACK
SystemApplet(VOID)
{
    HPROPSHEETPAGE hpsp[MAX_SYSTEM_PAGES];
    PROPSHEETHEADER psh;
    HMODULE hNetIdDll;
    HPSXA hpsxa = NULL;
    LONG Ret;
    static INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_LINK_CLASS};

    if (!InitCommonControlsEx(&icc))
        return 0;

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPTITLE;
    psh.hwndParent = hCPLWindow;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
    psh.pszCaption = MAKEINTRESOURCE(IDS_CPLSYSTEMNAME);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = hpsp;
    psh.pfnCallback = NULL;

    InitPropSheetPage(&psh, IDD_PROPPAGEGENERAL, (DLGPROC) GeneralPageProc);
    hNetIdDll = AddNetIdPage(&psh);
    InitPropSheetPage(&psh, IDD_PROPPAGEHARDWARE, (DLGPROC) HardwarePageProc);
    InitPropSheetPage(&psh, IDD_PROPPAGEADVANCED, (DLGPROC) AdvancedPageProc);

    /* Load additional pages provided by shell extensions */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\System"), MAX_SYSTEM_PAGES - psh.nPages);
    if (hpsxa != NULL)
    {
        SHAddFromPropSheetExtArray(hpsxa, PropSheetAddPage, (LPARAM)&psh);
    }

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
            hCPLWindow = hwndCPl;
            Applets[i].AppletProc();
            break;
    }

    return FALSE;
}


BOOL STDCALL
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
