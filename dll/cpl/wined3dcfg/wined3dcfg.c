#include "wined3dcfg.h"

#include <cpl.h>

HINSTANCE hApplet = 0;

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_CPLICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

LONG CALLBACK AppletInit(HWND hWnd)
{
    PROPSHEETPAGEW psp;
    PROPSHEETHEADERW psh;
    WCHAR szCaption[1024];

    LoadStringW(hApplet, IDS_CPLNAME, szCaption, sizeof(szCaption) / sizeof(WCHAR));

    ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hApplet;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGEGENERAL);
    psp.pfnDlgProc = GeneralPageProc;

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hWnd;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_CPLICON);
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = &psp;
    psh.pfnCallback = PropSheetProc;

    return (LONG)(PropertySheet(&psh) != -1);
}

LONG CALLBACK CPlApplet(HWND hWnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
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
                CPlInfo->idInfo = IDS_CPLDESCRIPTION;
                CPlInfo->idName = IDS_CPLNAME;
            }
            break;

        case CPL_DBLCLK:
            AppletInit(hWnd);
            break;
    }

    return FALSE;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
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
