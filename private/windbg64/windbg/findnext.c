/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    findnext.c

Abstract:

    This file contains the code for dealing with the Findnext Dialog box

Author:

    Kent Forschmiedt (kentf)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"

/***    DlgFindNext
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

INT_PTR
WINAPI
DlgFindNext(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD HelpArray[]=
    {
       ID_FIND_WHAT, IDH_WHAT,
       ID_FIND_WHOLEWORD, IDH_WHOLE,
       ID_FIND_MATCHUPLO, IDH_CASE,
       ID_FIND_REGEXP, IDH_REGEXPR,
       ID_FIND_UP, IDH_UP,
       ID_FIND_DOWN, IDH_DOWN,
       ID_FIND_NEXT, IDH_FINDNEXT,
       ID_FIND_TAGALL, IDH_TAGALL,
       ID_FINDNEXT_TAGALL, IDH_TAGALL,
       0, 0
    };

    switch (message) {

      case WM_INITDIALOG :

        if (Views[curView].Doc > -1) {
            if (Docs[Views[curView].Doc].docType == DOC_WIN) {
                EnableWindow(GetDlgItem(hDlg, ID_FINDNEXT_TAGALL),TRUE);
            }
        }
        return TRUE;

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                  "windbg.hlp",
                  HELP_WM_HELP, 
                  (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam,
                   "windbg.hlp",
                   HELP_CONTEXTMENU,
                   (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_ACTIVATE :

        if (wParam != 0 &&
              (HWND) lParam == hwndFrame
              && !frMem.firstFindNextInvoc && hwndActiveEdit) {

            //Set search start to current position

            frMem.leftCol = frMem.rightCol = Views[curView].X;
            frMem.line = Views[curView].Y;

            //Set search stop limit to the current position
            SetStopLimit();
        }
        frMem.firstFindNextInvoc = FALSE;
        return TRUE;

      case WM_COMMAND:
        switch (wParam) {

          case IDCANCEL:
              frMem.exitModelessFind = TRUE;
              return TRUE;

          case IDOK:
              frMem.exitModelessFind =
                   (!FindNext(hDlg, frMem.line,
                              frMem.rightCol,
                              FALSE, TRUE, FALSE)
                   || frMem.allFileDone
                   || frMem.hadError);
              return TRUE;

          case ID_FINDNEXT_TAGALL:
              TagAll(hDlg, Views[curView].Y);
              frMem.allTagged = TRUE;
              frMem.exitModelessFind = TRUE;
              return TRUE;
        }
        break;

      case WM_DESTROY:
          frMem.hDlgFindNextWnd = NULL; //Must be here to void reentrancy prob.
          FlushKeyboard();
          break;
    }

    return (FALSE);
}                                       /* DlgFindNext() */
