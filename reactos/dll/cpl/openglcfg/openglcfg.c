#include "openglcfg.h"

#include <cpl.h>

HINSTANCE hApplet = 0;

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
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = hWnd;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLICON));
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = &psp;

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
