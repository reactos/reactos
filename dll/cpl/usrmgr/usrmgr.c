/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/usrmgr.c
 * PURPOSE:         Main functions
 *
 * PROGRAMMERS:     Eric Kohl
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "usrmgr.h"

#define NUM_APPLETS 1

static LONG APIENTRY UsrmgrApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

HINSTANCE hApplet = 0;

LPTSTR GetDlgItemTextAlloc(HWND hwndDlg, INT nDlgItem)
{
    INT nLength = GetWindowTextLength(GetDlgItem(hwndDlg, nDlgItem));
    LPTSTR psz = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
    if (psz)
        GetDlgItemText(hwndDlg, nDlgItem, psz, nLength + 1);
    return psz;
}

LPTSTR GetComboBoxLBTextAlloc(HWND hwndDlg, INT nDlgItem, INT nIndex)
{
    INT nLength = (INT)SendDlgItemMessage(hwndDlg, nDlgItem, CB_GETLBTEXTLEN, nIndex, 0);
    LPTSTR psz = HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(TCHAR));
    if (psz)
        SendDlgItemMessage(hwndDlg, nDlgItem, CB_GETLBTEXT, nIndex, (LPARAM)psz);
    return psz;
}

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {
        IDI_USRMGR_ICON,
        IDS_CPLNAME,
        IDS_CPLDESCRIPTION,
        UsrmgrApplet
    }
};

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

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_USRMGR_ICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

/* Display Applet */
static LONG APIENTRY
UsrmgrApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    PROPSHEETPAGE psp[2];
    PROPSHEETHEADER psh;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwnd;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_USRMGR_ICON);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME);
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;
    psh.pfnCallback = PropSheetProc;

    InitPropSheetPage(&psp[0], IDD_USERS, UsersPageProc);
    InitPropSheetPage(&psp[1], IDD_GROUPS, GroupsPageProc);
    /* InitPropSheetPage(&psp[2], IDD_EXTRA, ExtraPageProc); */

    return (LONG)(PropertySheet(&psh) != -1);
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
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


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}
