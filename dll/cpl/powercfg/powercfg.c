/*
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/powercfg.c
 * PURPOSE:         initialization of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 *                  Martin Rottensteiner
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 */

#include "powercfg.h"

#include <winreg.h>
#include <regstr.h>

#define NUM_APPLETS (1)

static LONG APIENTRY Applet1(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);


HINSTANCE hApplet = 0;
GLOBAL_POWER_POLICY gGPP;
TCHAR langSel[255];

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDC_CPLICON_1, IDS_CPLNAME_1, IDS_CPLDESCRIPTION_1, Applet1}
};

static BOOL CALLBACK
PropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_POWER_PAGES)
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

    if (ppsh->nPages < MAX_POWER_PAGES)
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

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;

    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDC_CPLICON_1));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

/* First Applet */
static LONG APIENTRY
Applet1(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    HPROPSHEETPAGE hpsp[MAX_POWER_PAGES];
    PROPSHEETHEADER psh;
    HPSXA hpsxa = NULL;
    SYSTEM_POWER_CAPABILITIES spc;
    LONG ret;

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDC_CPLICON_1);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME_1);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = hpsp;
    psh.pfnCallback = PropSheetProc;

    if (!GetPwrCapabilities(&spc))
    {
        return GetLastError();
    }

    if (spc.SystemBatteriesPresent)
    {
        InitPropSheetPage(&psh, IDD_POWERSCHEMESPAGE_ACDC, PowerSchemesDlgProc);
        InitPropSheetPage(&psh, IDD_PROPPAGEALARMS, AlarmsDlgProc);
        InitPropSheetPage(&psh, IDD_PROPPAGEPOWERMETER, PowerMeterDlgProc);

        /* FIXME: Add battery page */
    }
    else
    {
        InitPropSheetPage(&psh, IDD_POWERSCHEMESPAGE_AC, PowerSchemesDlgProc);
    }
    InitPropSheetPage(&psh, IDD_PROPPAGEADVANCED, AdvancedDlgProc);
    if (spc.SystemS4)
    {
        InitPropSheetPage(&psh, IDD_PROPPAGEHIBERNATE, HibernateDlgProc);
    }

    /* FIXME: Add UPS page */

    /* Load additional pages provided by shell extensions */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Power"), MAX_POWER_PAGES - psh.nPages);
    if (hpsxa != NULL)
        SHAddFromPropSheetExtArray(hpsxa, PropSheetAddPage, (LPARAM)&psh);

    ret = (LONG)(PropertySheet(&psh) != -1);

    if (hpsxa != NULL)
        SHDestroyPropSheetExtArray(hpsxa);

    return ret;
}

/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    UINT i = (UINT)lParam1;

    switch (uMsg)
    {
        case CPL_INIT:
        {
            return TRUE;
        }

        case CPL_GETCOUNT:
        {
            return NUM_APPLETS;
        }

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
                Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            else
                return TRUE;
            break;
    }

    return FALSE;
}


BOOLEAN WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}
