/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       PRSHTHLP.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        6 May, 1997
*
*  DESCRIPTION:
*   Property sheet helper functions.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlwapi.h>
#include <help.h>

#include "powercfg.h"

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

extern  HINSTANCE   g_hInstance;    // Global instance handle of this DLL.

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  AppendPropSheetPage
*
*  DESCRIPTION:
*   Append a power page entry to an array of power pages.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL AppendPropSheetPage(
    PPOWER_PAGES    pppArray,
    UINT            uiDlgId,
    DLGPROC         pfnDlgProc
)
{
        UINT    i = 0;

    // Find the end.
    while (pppArray[++i].pfnDlgProc);

    pppArray[i].pfnDlgProc   = pfnDlgProc;
    pppArray[i].pDlgTemplate = MAKEINTRESOURCE(uiDlgId);
    return TRUE;
}

/*******************************************************************************
*
*  GetNumPropSheetPages
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

UINT GetNumPropSheetPages(
    PPOWER_PAGES    pppArray
)
{
    UINT    i = START_OF_PAGES;

    // Find the end.
    while (pppArray[i++].pfnDlgProc);

    return i - 1;
}

/*******************************************************************************
*
*  _AddPowerPropSheetPage
*
*  DESCRIPTION:
*   Adds optional pages for outside callers.

*  PARAMETERS:
*
*******************************************************************************/

BOOL CALLBACK _AddPowerPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER FAR * ppsh = (PROPSHEETHEADER FAR *)lParam;

    if (hpage && (ppsh->nPages < MAX_PAGES )) {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
*
*  DoPropSheetPages
*
*  DESCRIPTION:
*   Bring up the specified property sheet pages. Return FALSE if no pages
*   were displayed.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL PASCAL DoPropSheetPages(
    HWND         hwnd,
    POWER_PAGES  PowerPages[],
    LPTSTR       lpszOptionalPages
)
{
    HPROPSHEETPAGE  rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp;

    HPSXA   hpsxa = NULL;
    ULONG   uPage;
    BOOLEAN bRet = TRUE;

    // Fill in the sheet header
    psh.dwSize     = sizeof(psh);
    psh.dwFlags    = PSH_PROPTITLE;
    psh.hwndParent = hwnd;
    psh.hInstance  = g_hInstance;

    psh.pszCaption = PowerPages[CAPTION_INDEX].pDlgTemplate;
    psh.nStartPage = 0;
    psh.nPages     = 0;
    psh.phpage     = rPages;

    // Fill in the page constants
    psp.dwSize     = sizeof(PROPSHEETPAGE);
    psp.dwFlags    = PSP_DEFAULT;
    psp.hInstance  = g_hInstance;

    for (uPage = START_OF_PAGES; uPage < MAX_PAGES; uPage++) {

        if (PowerPages[uPage].pDlgTemplate == NULL) {
            break;
        }

        (PPOWER_PAGES)psp.lParam    = &(PowerPages[uPage]);
        psp.pszTemplate             = PowerPages[uPage].pDlgTemplate;
        psp.pfnDlgProc              = PowerPages[uPage].pfnDlgProc;

        rPages[psh.nPages] = CreatePropertySheetPage(&psp);
        PowerPages[uPage].hPropSheetPage = rPages[psh.nPages];

        if (rPages[psh.nPages] != NULL) {
            psh.nPages++;
        }
    }

    // Add any optional pages specified in the registry.
    if (lpszOptionalPages) {
        hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE,
                                          lpszOptionalPages, MAX_PAGES);
        if (hpsxa) {
            SHAddFromPropSheetExtArray(hpsxa, _AddPowerPropSheetPage, (LPARAM)&psh);
        }
    }

    // Did we come up with any pages to show ?
    if (psh.nPages == 0) {
        return FALSE;
    }

    // Bring up the pages.
    if (PropertySheet(&psh) < 0) {
        DebugPrint( "DoPropSheetPages, PropertySheet failed, LastError: 0x%08X", GetLastError());
        bRet = FALSE;
    }

    // Free any optional pages if we loaded them.
    if (hpsxa) {
        SHDestroyPropSheetExtArray(hpsxa);
    }
    return bRet;
}

/*******************************************************************************
*
*  MarkSheetDirty
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID MarkSheetDirty(HWND hWnd, PBOOL pb)
{
    SendMessage(GetParent(hWnd), PSM_CHANGED, (WPARAM)hWnd, 0L);
    *pb = TRUE;
}

/*******************************************************************************
*
*                 P R I V A T E   F U N C T I O N S
*
*******************************************************************************/


