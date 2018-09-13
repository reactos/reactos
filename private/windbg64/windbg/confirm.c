/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    confirm.c

Abstract:

    This file contains the code for dealing with the dialog box used to
    confirm that a replacement is to be preformed.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop


/***    DlgConfirm
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

/****************************************************************************

        FUNCTION:   DlgConfirm(HWND, unsigned, WORD, LONG)

        PURPOSE:    Processes messages for "CONFIRM" dialog box
                                        (When changing text occurences)

        MESSAGES:

                WM_INITDIALOG - Initialize dialog box
                WM_COMMAND- Input received

****************************************************************************/

INT_PTR
WINAPI
DlgConfirm(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {

      case WM_INITDIALOG:
        SetFocus(GetDlgItem(hDlg, ID_CONFIRM_REPLACE));
        break;

      case WM_ACTIVATE :

        if ((wParam != 0) &&
#ifdef WIN32
             ((HWND) lParam == hwndFrame) &&
#else
             (LOWORD(lParam) == hwndFrame) &&
#endif
             (!frMem.firstConfirmInvoc && hwndActiveEdit)) {

            //Disable Replace and Replace all buttons

            EnableWindow(GetDlgItem(hDlg, ID_CONFIRM_REPLACE), FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_CONFIRM_REPLACEALL), FALSE);
            SetFocus(GetDlgItem(hDlg, ID_CONFIRM_FINDNEXT));

            //Set search start to current position

            frMem.leftCol = frMem.rightCol = Views[curView].X;
            frMem.line = Views[curView].Y;

            //Set search stop limit to the current position

            SetStopLimit();
        }
        frMem.firstConfirmInvoc = FALSE;
        return TRUE;

      case WM_COMMAND:
        switch (wParam) {

          case ID_CONFIRM_REPLACEALL :

            frMem.replaceAll = TRUE;
            ReplaceAll(hDlg);
            frMem.replaceAll = FALSE;
            frMem.exitModelessReplace = TRUE;
            return TRUE;

          case IDCANCEL:
            frMem.exitModelessReplace = TRUE;
            return TRUE;

          case ID_CONFIRM_FINDNEXT:
            //Disable Replace and Replace all buttons
            EnableWindow(GetDlgItem(hDlg, ID_CONFIRM_REPLACE), TRUE);
            EnableWindow(GetDlgItem(hDlg, ID_CONFIRM_REPLACEALL), TRUE);

            frMem.exitModelessReplace = (!FindNext(hDlg, frMem.line, frMem.rightCol, FALSE, TRUE, FALSE)
                  || frMem.allFileDone
                  || frMem.hadError);
            return TRUE;

          case ID_CONFIRM_REPLACE:

            ReplaceOne();
            InvalidateLines(curView, frMem.line, frMem.line, FALSE);
            frMem.exitModelessReplace = (!FindNext(hDlg, frMem.line, frMem.rightCol, FALSE, TRUE, FALSE)
                  || frMem.allFileDone
                  || frMem.hadError);

            //Replace "Cancel" pushbutton with "Done"

            Dbg(LoadString(g_hInst, SYS_Done, szTmp, sizeof(szTmp)));
            SetWindowText(GetDlgItem(hDlg, IDCANCEL),   (LPSTR)szTmp);
            return TRUE;

          case IDWINDBGHELP :
            Dbg(WinHelp(hDlg, szHelpFileName, HELP_CONTEXT, ID_CONFIRM_HELP));
            return TRUE;
        }
        break;

      case WM_DESTROY:
        frMem.hDlgConfirmWnd = NULL; //Must be here to void reentrancy prob.
        FlushKeyboard();
        break;
    }

    return (FALSE);
}
