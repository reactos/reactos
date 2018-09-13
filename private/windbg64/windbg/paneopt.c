/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    PANEOPT.C

Abstract:

    This file contains the code for dealing with the Pane Manager Options

Author:

    Bill Heaton (v-willhe)
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

    Win32 - User

--*/


#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"

#define MAXFMT 512

INT_PTR DlgPaneOptInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR DlgPaneOptCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern void CheckHorizontalScroll (PPANE p);


WORD     DialogType;
HWND     hWnd;
PPANE    pPane;
PANEINFO Info;
char     format[MAXFMT];
char     tformat[MAXFMT];

/***    DlgQuickW
**
**  Synopsis:
**      bool = DlgQuickW(hwnd, message, wParam, lParam)
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Processes messages for "xxx Options" dialog box
**      (Edit Find Option)
**
**      MESSAGES:
**
**              WM_INITDIALOG - Initialize dialog box
**              WM_COMMAND- Input received
**
*/

INT_PTR
CALLBACK
DlgPaneOptions(
               HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam
               )
{
    static DWORD HelpArray[]=
    {
        ID_PANEMGR_FORMAT, IDH_FMT,
        ID_PANEMGR_EXPAND_1ST, IDH_EXPAND,
        ID_PANEMGR_EXPAND_NONE, IDH_NOTEXPAND,
        0, 0
    };


    switch (message) {
    case WM_INITDIALOG:
        return ( DlgPaneOptInit( hDlg, message, wParam, lParam) );

    case WM_COMMAND:
        return ( DlgPaneOptCommand( hDlg, message, wParam, lParam) );

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
            (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;

    }
    return (FALSE);

}   /* DlgPaneOptions() */



INT_PTR
DlgPaneOptInit(
               HWND hDlg, 
               UINT message, 
               WPARAM wParam, 
               LPARAM lParam
               )
{


    switch ( DialogType ) {
    case WATCH_WIN:
        SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)"Watch Window Options");
        hWnd = GetWatchHWND();
        break;

    case LOCALS_WIN:
        SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)"Locals Window Options");
        hWnd = GetLocalHWND();
        break;

    case CPU_WIN:
        SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)"CPU Window Options");
        hWnd = GetCpuHWND();
        break;

    case FLOAT_WIN:
        SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)"Float Window Options");
        hWnd = GetFloatHWND();
        break;

    default:
        DAssert(FALSE);

    }

    // Get the Options for the Window
    pPane = (PPANE)GetWindowLongPtr(hWnd, GWW_EDIT );
    DAssert(pPane);
    Info.CtrlId = pPane->nCtrlId;
    Info.ItemId = pPane->CurIdx;
    (PSTR)(*pPane->fnEditProc)(hWnd,WU_INFO,(WPARAM)&Info,(LPARAM)pPane);

    // Set the Format string if it exists
    if ( Info.pFormat) {
        SendDlgItemMessage(hDlg, ID_PANEMGR_FORMAT, WM_SETTEXT, 0, (LPARAM)Info.pFormat);
    }

    // Set the Expand button if true
    if ( pPane->bFlags.Expand1st ) {
        SendDlgItemMessage(hDlg, ID_PANEMGR_EXPAND_1ST, BM_SETCHECK, 1, 0);
    } else {
        SendDlgItemMessage(hDlg, ID_PANEMGR_EXPAND_NONE, BM_SETCHECK, 1, 0);
    }

    return(TRUE);
}


INT_PTR
DlgPaneOptCommand(
                  HWND hDlg,
                  UINT message,
                  WPARAM wParam,
                  LPARAM lParam
                  )

{
    PTRVIT pVit = NULL;
    PCHAR  pFmt = &format[0];

    switch (wParam) {

    case ID_PANEMGR_FORMAT:
        return 0;

    case ID_PANEMGR_EXPAND_1ST:
        pPane->bFlags.Expand1st = TRUE;
        return 0;

    case ID_PANEMGR_EXPAND_NONE:
        pPane->bFlags.Expand1st = FALSE;
        return 0;

    case IDOK:
        SendDlgItemMessage(hDlg, ID_PANEMGR_FORMAT, WM_GETTEXT, MAXFMT, (LPARAM)&format[0]);
        while ( isspace(*pFmt) ) {
            pFmt++;
        }

        if ( strlen (pFmt) == 0) {
            Info.pFormat = NULL;
        } else {
            // Ensure a leading comma
            if ( *pFmt != ',') {

                strcpy (tformat,",");
                strcat (tformat,pFmt);
                pFmt = &tformat[0];
            }

            Info.pFormat = pFmt;
        }

        (PSTR)(*pPane->fnEditProc)(hWnd,WU_OPTIONS,(WPARAM)&Info,(LPARAM)pPane);
        CheckHorizontalScroll (pPane);

        // No break intended
    case IDCANCEL:
        pPane    = NULL;
        hWnd = 0;
        memset( &Info, 0, sizeof(PANEINFO));
        EndDialog(hDlg, FALSE);
        return 0;
    }

    return 1;
}
