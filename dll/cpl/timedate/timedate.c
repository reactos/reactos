/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/timedate.c
 * PURPOSE:     ReactOS Timedate Control Panel
 * COPYRIGHT:   Copyright 2004-2005 Eric Kohl
 *              Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "timedate.h"

#define NUM_APPLETS 1

static LONG APIENTRY Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

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
    PWSTR lpMsgBuf;
#if DBG
    WCHAR szMsg[255];
#endif

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   dwErrorCode,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPWSTR) &lpMsgBuf,
                   0,
                   NULL );

#if DBG
    if (swprintf(szMsg, L"%hs:%d: %s", file, line, (PWSTR)lpMsgBuf))
    {
        MessageBoxW(NULL, szMsg, NULL, MB_OK | MB_ICONERROR);
    }
#else
    MessageBox(NULL, lpMsgBuf, NULL, MB_OK | MB_ICONERROR);
#endif

    LocalFree(lpMsgBuf);
}


static VOID
InitPropSheetPage(PROPSHEETPAGEW *psp, WORD idDlg, DLGPROC DlgProc)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGEW));
    psp->dwSize = sizeof(PROPSHEETPAGEW);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hApplet;
    psp->pszTemplate = MAKEINTRESOURCEW(idDlg);
    psp->pfnDlgProc = DlgProc;
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
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDC_CPLICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

static LONG APIENTRY
Applet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGEW psp[3];
    WCHAR Caption[256];
    LONG Ret = 0;

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if (RegisterMonthCalControl(hApplet) &&
        RegisterClockControl())
    {
        LoadStringW(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(WCHAR));

        ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
        psh.dwSize = sizeof(PROPSHEETHEADERW);
        psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
        psh.hwndParent = hwnd;
        psh.hInstance = hApplet;
        psh.pszIcon = MAKEINTRESOURCEW(IDC_CPLICON);
        psh.pszCaption = Caption;
        psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGEW);
        psh.nStartPage = 0;
        psh.ppsp = psp;
        psh.pfnCallback = PropSheetProc;

        InitPropSheetPage(&psp[0], IDD_DATETIMEPAGE, DateTimePageProc);
        InitPropSheetPage(&psp[1], IDD_TIMEZONEPAGE, TimeZonePageProc);
        InitPropSheetPage(&psp[2], IDD_INETTIMEPAGE, InetTimePageProc);

        Ret = (LONG)(PropertySheetW(&psh) != -1);

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
    INT i = (INT)lParam1;

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
