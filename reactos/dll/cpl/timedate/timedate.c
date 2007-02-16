/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/cpl/timedate/timedate.c
 * PURPOSE:     ReactOS Timedate Control Panel
 * COPYRIGHT:   Copyright 2004-2005 Eric Kohl
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include <timedate.h>

#define NUM_APPLETS 1

LONG APIENTRY Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);

HINSTANCE hApplet;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};

#if DBG
VOID DisplayWin32ErrorDbg(DWORD dwErrorCode, const char *file, int line)
#else
VOID DisplayWin32Error(DWORD dwErrorCode)
#endif
{
    LPVOID lpMsgBuf;
#if DBG
    TCHAR szMsg[255];
#endif

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  dwErrorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL );

#if DBG
    if (_stprintf(szMsg, _T("%hs:%d: %s"), file, line, lpMsgBuf))
    {
        MessageBox(NULL, szMsg, NULL, MB_OK | MB_ICONERROR);
    }
#else
    MessageBox(NULL, lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
#endif

    LocalFree(lpMsgBuf);
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGE));
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pfnDlgProc = DlgProc;
}


LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[3];
    TCHAR Caption[256];
    LONG Ret = 0;

    UNREFERENCED_PARAMETER(hwnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if (RegisterMonthCalControl(hApplet) && 
        RegisterClockControl())
    {
        LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

        ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
        psh.hwndParent = NULL;
        psh.hInstance = hApplet;
        psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
        psh.pszCaption = Caption;
        psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
        psh.nStartPage = 0;
        psh.ppsp = psp;

        InitPropSheetPage(&psp[0], IDD_DATETIMEPAGE, (DLGPROC) DateTimePageProc);
        InitPropSheetPage(&psp[1], IDD_TIMEZONEPAGE, (DLGPROC) TimeZonePageProc);
        InitPropSheetPage(&psp[2], IDD_INETTIMEPAGE, (DLGPROC) InetTimePageProc);

        Ret = (LONG)(PropertySheet(&psh) != -1);

        UnregisterMonthCalControl(hApplet);
        UnregisterClockControl();
    }

    return Ret;
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
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
        {
            Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
        }
        break;
    }
    return FALSE;
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            INITCOMMONCONTROLSEX InitControls;

            InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
            InitControls.dwICC = ICC_DATE_CLASSES | ICC_PROGRESS_CLASS | ICC_UPDOWN_CLASS;
            InitCommonControlsEx(&InitControls);

            hApplet = hinstDLL;
        }
        break;
    }

    return TRUE;
}
