#include "precomp.h"
#pragma hdrstop

#define extraPick                       (*((BOOL *)&wGeneric1))

/***    DlgReplace
**
**
**  Description:
**      Processes messages for "REPLACE" dialog box
**      (Edit Replace menu Option)
**
**      MESSAGES:
**
**      WM_INITDIALOG - Initialize dialog box
**      WM_COMMAND - Input received
*/

INT_PTR
CALLBACK
DlgReplace(
           HWND hDlg,
           UINT message,
           WPARAM wParam,
           LPARAM lParam
           )
{
    Unused( lParam );

    switch (message) {

    case WM_INITDIALOG: {

            BOOL lookAround = TRUE;
            int i;
            LPSTR s;

            frMem.replaceAll = FALSE;
            frMem.oneLineDone = FALSE;
            frMem.replacing = TRUE;
            SetStopLimit();

            //Retrieve text from document, blank string if nothing found
            //and send string to dialog box
            if (GetCurrentText(curView, &lookAround,
                               (LPSTR)findReplace.findWhat,
                               MAX_USER_LINE, &frMem.leftCol, NULL)) {

                //Temporarly put the string in the picklist
                extraPick = InsertInPickList(FIND_PICK);

            } else
                extraPick = FALSE;

            frMem.rightCol = frMem.leftCol;
            SendDlgItemMessage(hDlg, ID_REPLACE_WHAT,
                               CB_LIMITTEXT, MAX_USER_LINE, (LPARAM) NULL);
            SendDlgItemMessage(hDlg, ID_REPLACE_WHAT, WM_SETTEXT, 0,
                               (LPARAM)((LPSTR)findReplace.findWhat));

            //Put back last replaced value
            SendDlgItemMessage(hDlg, ID_REPLACE_REPLACEWITH,
                               CB_LIMITTEXT, MAX_USER_LINE, (LPARAM) NULL);
            SendDlgItemMessage(hDlg, ID_REPLACE_REPLACEWITH, WM_SETTEXT, 0,
                               (LPARAM)((LPSTR)findReplace.replaceWith));

            //Transfer boolean values to Dialog Box
            SendDlgItemMessage(hDlg, ID_REPLACE_MATCHUPLO, BM_SETCHECK,
                               findReplace.matchCase, 0L);
            SendDlgItemMessage(hDlg, ID_REPLACE_WHOLEWORD, BM_SETCHECK,
                               !findReplace.regExpr & findReplace.wholeWord, 0L);
            SendDlgItemMessage(hDlg, ID_REPLACE_REGEXP, BM_SETCHECK,
                               findReplace.regExpr, 0L);
            EnableWindow(GetDlgItem(hDlg, ID_REPLACE_WHOLEWORD),
                         !findReplace.regExpr);

            frMem.goUpCopy = findReplace.goUp;
            findReplace.goUp = FALSE;

            //Set the line to start find
            frMem.line = Views[curView].Y;

            //Set number of replaced occurences to 0
            frMem.nbReplaced = 0;

            //Fill find pick list
            for (i = 0 ; i < findReplace.nbInPick[FIND_PICK]; i++) {
                Dbg(s = (LPSTR)GlobalLock(findReplace.hPickList[FIND_PICK][i]));
                SendDlgItemMessage(hDlg, ID_REPLACE_WHAT, CB_INSERTSTRING, (WPARAM) -1,
                                   (LPARAM)(LPSTR)s);
                Dbg(GlobalUnlock (findReplace.hPickList[FIND_PICK][i]) == FALSE);
            }

            //Fill replace pick list
            for (i = 0 ; i < findReplace.nbInPick[REPLACE_PICK]; i++) {
                Dbg(s = (LPSTR)GlobalLock(findReplace.hPickList[REPLACE_PICK][i]));
                SendDlgItemMessage(hDlg, ID_REPLACE_REPLACEWITH, CB_INSERTSTRING, (WPARAM) -1,
                                   (LPARAM)(LPSTR)s);
                Dbg(GlobalUnlock (findReplace.hPickList[REPLACE_PICK][i]) == FALSE);
            }

            return TRUE;
        }

    case WM_COMMAND: {

            switch (wParam) {

            case ID_REPLACE_FINDNEXT:
            case ID_REPLACE_REPLACEALL:

                if (SendDlgItemMessage(hDlg, ID_REPLACE_WHAT, WM_GETTEXT,
                                       MAX_USER_LINE,
                                       (LPARAM)((LPSTR)findReplace.findWhat))) {


                    SendDlgItemMessage(hDlg, ID_REPLACE_REPLACEWITH,
                                       WM_GETTEXT, MAX_USER_LINE,
                                       (LPARAM)((LPSTR)findReplace.replaceWith));

                    if (extraPick)
                        RemoveFromPick(FIND_PICK);

                    frMem.replaceAll = (wParam == ID_REPLACE_REPLACEALL);
                    InsertInPickList(FIND_PICK);
                    InsertInPickList(REPLACE_PICK);
                    if (FindNext(hDlg, Views[curView].Y, Views[curView].X,
                                 TRUE, TRUE, TRUE)) {
                        if (frMem.hadError)
                            findReplace.goUp = frMem.goUpCopy;
                        frMem.replacing = !frMem.hadError;
                        frMem.firstConfirmInvoc = TRUE;
                        EndDialog(hDlg, !frMem.hadError);
                    } else {
                        SetStopLimit();
                        SetFocus(GetDlgItem(hDlg, ID_REPLACE_WHAT));
                    }

                }
                return TRUE;

            case ID_REPLACE_WHOLEWORD:
                findReplace.wholeWord = !findReplace.wholeWord;
                return TRUE;

            case ID_REPLACE_MATCHUPLO:
                findReplace.matchCase = !findReplace.matchCase;
                return TRUE;

            case ID_REPLACE_REGEXP:
                findReplace.regExpr = !findReplace.regExpr;
                SendDlgItemMessage(hDlg, ID_REPLACE_WHOLEWORD, BM_SETCHECK,
                                   !findReplace.regExpr & findReplace.wholeWord, 0L);
                EnableWindow(GetDlgItem(hDlg, ID_REPLACE_WHOLEWORD),
                             !findReplace.regExpr);
                return TRUE;

            case IDCANCEL :

                //Remove the extra pick we inserted
                if (extraPick)
                    RemoveFromPick(FIND_PICK);

                findReplace.goUp = frMem.goUpCopy;
                frMem.replacing = FALSE;
                EndDialog(hDlg, FALSE);
                return TRUE;

            case IDWINDBGHELP :
                Dbg(WinHelp(hDlg,szHelpFileName,HELP_CONTEXT,ID_REPLACE_HELP));
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}
