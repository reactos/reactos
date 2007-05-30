/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cpl/intl/intl.c
 * PURPOSE:         ReactOS International Control Panel
 * PROGRAMMERS:     Eric Kohl
 *                  Alexey Zavyalov (gen_x@mail.ru)
*/

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "resource.h"
#include "intl.h"

/* GLOBALS ******************************************************************/

#define NUM_APPLETS     (1)
#define NUM_SHEETS       3

LONG APIENTRY Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam);

HINSTANCE hApplet;

APPLET Applets[NUM_APPLETS] = 
{
  {IDC_CPLICON, IDS_CPLNAME, IDS_CPLDESCRIPTION, Applet}
};

/* FUNCTIONS ****************************************************************/

static
VOID
InitPropSheetPage(PROPSHEETPAGE *PsPage, WORD IdDlg, DLGPROC DlgProc)
{
    ZeroMemory(PsPage, sizeof(PROPSHEETPAGE));
    PsPage->dwSize = sizeof(PROPSHEETPAGE);
    PsPage->dwFlags = PSP_DEFAULT;
    PsPage->hInstance = hApplet;
    PsPage->pszTemplate = MAKEINTRESOURCE(IdDlg);
    PsPage->pfnDlgProc = DlgProc;
}

/* Create applets */
LONG
APIENTRY
Applet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
    PROPSHEETPAGE PsPage[NUM_SHEETS];
    PROPSHEETHEADER psh;
    TCHAR Caption[MAX_STR_SIZE];

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(PsPage) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = PsPage;

    InitPropSheetPage(&PsPage[0], IDD_REGOPTSPAGE, RegOptsProc);
    InitPropSheetPage(&PsPage[1], IDD_LANGSOPTSPAGE, LangsOptsProc);
    InitPropSheetPage(&PsPage[2], IDD_EXTRAOPTSPAGE, ExtraOptsProc);

    return (LONG)(PropertySheet(&psh) != -1);
}

/* Control Panel Callback */
LONG
CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    switch(uMsg)
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
        {
            CPLINFO *CplInfo = (CPLINFO*)lParam2;
            UINT uAppIndex = (UINT)lParam1;

            CplInfo->lData = 0;
            CplInfo->idIcon = Applets[uAppIndex].idIcon;
            CplInfo->idName = Applets[uAppIndex].idName;
            CplInfo->idInfo = Applets[uAppIndex].idDescription;
            break;
        }
        case CPL_DBLCLK:
        {
            UINT uAppIndex = (UINT)lParam1;
            Applets[uAppIndex].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            break;
        }
    }

    return FALSE;
}

/* Standart DLL entry */

BOOL
STDCALL
DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    INITCOMMONCONTROLSEX InitControls;
    UNREFERENCED_PARAMETER(lpvReserved);
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        {
            InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
            InitControls.dwICC = ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;
            InitCommonControlsEx(&InitControls);

            hApplet = hinstDLL;
            break;
        }
    }


    return TRUE;
}

/* EOF */
