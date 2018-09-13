/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    mouse.c

Abstract:

    This module contains the main routines for the Mouse applet.

Revision History:

--*/



//
//  Include Files.
//

#include "main.h"
#include "rc.h"
#include "applet.h"
#include <regstr.h>
#include <cplext.h>
#include "util.h"



//
//  Constant Declarations.
//

#define MAX_PAGES 32


const HWPAGEINFO c_hpiMouse = {
    // Mouse device class
    { 0x4d36e96fL, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } },

    // Mouse troubleshooter command line
    IDS_MOUSE_TSHOOT,
};


//
//  Global Variables.
//

//
//  Location of prop sheet hooks in the registry.
//
static const TCHAR sc_szRegMouse[] = REGSTR_PATH_CONTROLSFOLDER TEXT("\\Mouse");




//
//  Function Prototypes.
//

INT_PTR CALLBACK
MouseButDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
MousePtrDlg(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR CALLBACK
MouseMovDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);





////////////////////////////////////////////////////////////////////////////
//
//  _AddMousePropSheetPage
//
//  Adds a property sheet page.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK _AddMousePropSheetPage(
    HPROPSHEETPAGE hpage,
    LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;

    if (hpage && (ppsh->nPages < MAX_PAGES))
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return (TRUE);
    }
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  MouseApplet
//
////////////////////////////////////////////////////////////////////////////

int MouseApplet(
    HINSTANCE instance,
    HWND parent,
    LPCTSTR cmdline)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETPAGE psp;
    PROPSHEETHEADER psh;
    HPSXA hpsxa;
    int Result;

    //
    //  Make the initial page.
    //
    psh.dwSize     = sizeof(psh);
    psh.dwFlags    = PSH_PROPTITLE;
    psh.hwndParent = parent;
    psh.hInstance  = instance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_MOUSE_TITLE);
    psh.nPages     = 0;
    psh.nStartPage = 0;
    psh.phpage     = rPages;

    //
    //  Load any installed extensions.
    //
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, sc_szRegMouse, 8);

    //
    //  Add the Buttons page, giving the extensions a chance to replace it.
    //
    if (!hpsxa ||
        !SHReplaceFromPropSheetExtArray( hpsxa,
                                         CPLPAGE_MOUSE_BUTTONS,
                                         _AddMousePropSheetPage,
                                         (LPARAM)&psh ))
    {
        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_DEFAULT;
        psp.hInstance   = instance;
        psp.pszTemplate = MAKEINTRESOURCE(DLG_MOUSE_BUTTONS);
        psp.pfnDlgProc  = MouseButDlg;
        psp.lParam      = 0;

        _AddMousePropSheetPage(CreatePropertySheetPage(&psp), (LPARAM)&psh);
    }

    //
    //  Add the Pointers page (not replaceable).
    //
    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = instance;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_MOUSE_POINTER);
    psp.pfnDlgProc  = MousePtrDlg;
    psp.lParam      = 0;

    _AddMousePropSheetPage(CreatePropertySheetPage(&psp), (LPARAM)&psh);

    //
    //  Add the Motion page, giving the extensions a chance to replace it.
    //
    if (!hpsxa ||
        !SHReplaceFromPropSheetExtArray( hpsxa,
                                         CPLPAGE_MOUSE_PTRMOTION,
                                         _AddMousePropSheetPage,
                                         (LPARAM)&psh ))
    {
        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_DEFAULT;
        psp.hInstance   = instance;
        psp.pszTemplate = MAKEINTRESOURCE(DLG_MOUSE_MOTION);
        psp.pfnDlgProc  = MouseMovDlg;
        psp.lParam      = 0;

        _AddMousePropSheetPage(CreatePropertySheetPage(&psp), (LPARAM)&psh);
    }

    _AddMousePropSheetPage(CreateHardwarePage(&c_hpiMouse), (LPARAM)&psh);

    //
    //  Add any extra pages that the extensions want in there.
    //
    if (hpsxa)
    {
        UINT cutoff = psh.nPages;
        UINT added = SHAddFromPropSheetExtArray( hpsxa,
                                                 _AddMousePropSheetPage,
                                                 (LPARAM)&psh );

        if (psh.nStartPage >= cutoff)
        {
            psh.nStartPage += added;
        }
    }

    //
    //  Invoke the Property Sheets.
    //
    switch (PropertySheet(&psh))
    {
        case ( ID_PSRESTARTWINDOWS ) :
        {
            Result = APPLET_RESTART;
            break;
        }
        case ( ID_PSREBOOTSYSTEM ) :
        {
            Result = APPLET_REBOOT;
            break;
        }
        default :
        {
            Result = 0;
            break;
        }
    }

    //
    //  Free any loaded extensions.
    //
    if (hpsxa)
    {
        SHDestroyPropSheetExtArray(hpsxa);
    }

    return (Result);
}
