/* $Id$
 *
 * PROJECT:         ReactOS Accessibility Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/access/access.c
 * PURPOSE:         Main control panel code
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 *                  Copyright 2007 Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdlib.h>
#include "resource.h"
#include "access.h"

#define NUM_APPLETS	(1)

LONG CALLBACK SystemApplet(VOID);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDI_CPLACCESS, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, SystemApplet}
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


/* First Applet */

LONG CALLBACK
SystemApplet(VOID)
{
    PROPSHEETPAGE psp[5];
    PROPSHEETHEADER psh;
    TCHAR Caption[1024];

    LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLACCESS));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_PROPPAGEKEYBOARD, (DLGPROC)KeyboardPageProc);
    InitPropSheetPage(&psp[1], IDD_PROPPAGESOUND, (DLGPROC)SoundPageProc);
    InitPropSheetPage(&psp[2], IDD_PROPPAGEDISPLAY, (DLGPROC)DisplayPageProc);
    InitPropSheetPage(&psp[3], IDD_PROPPAGEMOUSE, (DLGPROC)MousePageProc);
    InitPropSheetPage(&psp[4], IDD_PROPPAGEGENERAL, (DLGPROC)GeneralPageProc);

    return (LONG)(PropertySheet(&psh) != -1);
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

