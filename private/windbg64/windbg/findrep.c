/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    findrep.c

Abstract:

    This module contains find and replace code for the editor

Author:

    Griffith Wm. Kadnier (v-griffk) 01-Aug-1992

Environment:

    Win32, User Mode

--*/



#include "precomp.h"
#pragma hdrstop




//Prototypes

void PASCAL SetDialogPos (BOOL forced, HWND hDlg);
void PASCAL RefreshView (int startingCol,int endingCol,BOOL selectFoundText);
char *REmalloc (size_t size);
BOOL PASCAL RESearch (LPSTR line, int *xStart, int *xEnd);
BOOL PASCAL SearchLineBack (int lineNb,int *xStart,int *xEnd,PSTR findWhat);
BOOL PASCAL FindPrev (long startingLine,int startingCol,BOOL startFromSelection,BOOL selectFoundText,BOOL errorIfNotFound,HCURSOR hSaveCursor,PSTR findWhat);
BOOL PASCAL SearchLineForw (int lineNb,int *xStart,int *xEnd,PSTR findWhat);





void PASCAL
SetDialogPos(
    BOOL forced,
    HWND hDlg)
{
    RECT r, rInt;
    int y;
    LPVIEWREC v = &Views[curView];
    BOOL inter;

    Assert(v->Doc >= 0);

    y = (v->Y - GetScrollPos(v->hwndClient, SB_VERT)) * v->charHeight;
    GetWindowRect(v->hwndClient, &r);

    //Calculate caret pos and a virtual rectangle
    y += r.top;
    rInt.left = 0;
    rInt.right = GetSystemMetrics(SM_CXSCREEN);
    rInt.top = y;
    rInt.bottom = y + v->charHeight;

    //See if our dialog box hide the caret
    GetWindowRect(hDlg, &r);
    inter = IntersectRect(&rInt, &r, &rInt);

    if (forced || inter) {
        y += (v->charHeight / 2);

        //Put the dialogbox where we have the biggest free space
        if ((forced && !inter) || y > (GetSystemMetrics(SM_CYSCREEN) / 2)) {
            y = 0;
        } else {
            y = GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top);
        }
        MoveWindow(hDlg,
        (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2,
            y, r.right - r.left, r.bottom - r.top, TRUE);
        ShowWindow(hDlg, SW_NORMAL);
    }
}

void FAR PASCAL
SetStopLimit(
    void
    )
{
    frMem.stopLine = Views[curView].Y;
    frMem.stopCol = Views[curView].X;
    frMem.allFileDone = FALSE;
    frMem.oneLineDone = FALSE;
}

void PASCAL
RefreshView(
    int startingCol,
    int endingCol,
    BOOL selectFoundText
    )
{
    LPVIEWREC v = &Views[curView];

    Assert(v->Doc >= 0);

    //First put cursor at left of selection to possibly scroll
    //hidden word
    PosXY(curView, startingCol, frMem.line, TRUE);
    PosXY(curView, endingCol, frMem.line, TRUE);

    //Now select text found
    if (selectFoundText) {
        ClearSelection(curView);
        v->BlockStatus = TRUE;
        v->BlockYL = frMem.line;
        v->BlockYR = frMem.line;
        v->BlockXL = startingCol;
        v->BlockXR = endingCol;
    }

    //Refresh line
    InvalidateLines(curView, frMem.line, frMem.line, FALSE);

    //Change position of modeless dialog boxes if needed
    if (frMem.hDlgConfirmWnd) {
        SetDialogPos(FALSE, frMem.hDlgConfirmWnd);
    }
    if (frMem.hDlgFindNextWnd) {
        SetDialogPos(FALSE, frMem.hDlgFindNextWnd);
    }
}

char *
REmalloc(
    size_t size
    )
{
    return (char *)LocalAlloc(LPTR, size);
}

BOOL PASCAL
RESearch(
    LPSTR line,
    int *xStart,
    int *xEnd
    )
{
    int maxREStack = 256;
    BOOL done;
    RE_OPCODE **stack; //Allocation for RE stack

    //We may redo it if we need more stack
    do {

        done = TRUE;

        //Do not search if start is out of string
        if (*xStart >= (int)strlen(line)) {
            break;
        }

        //Allocates the stack for the regular expression
        stack = (RE_OPCODE **)REmalloc(maxREStack * sizeof(*stack));

        //Try the match
        switch (REMatch(pat, line, line + *xStart, stack, maxREStack, TRUE)) {

          case REM_MATCH: {

            int len = RELength(pat, 0);

            if (len > 0) {
                *xStart = (int) (REStart(pat) - line);
                *xEnd =  *xStart + len;
                Dbg(LocalFree((HANDLE)stack) == NULL);
                return TRUE;
            } else if (len == 0) {
                *xEnd = *xStart + 1;
                Dbg(LocalFree((HANDLE)stack) == NULL);
                return TRUE;
            }
            break;
          }

          case REM_NOMATCH:
            break;

          case REM_STKOVR:
            //The RE engine had a stack overflow. Add 256 bytes
            //and try again
            maxREStack += 256;
            done = FALSE;
            break;

          case REM_INVALID:
            ErrorBox(ERR_RegExpr_Invalid, (LPSTR)findReplace.findWhat);
            frMem.hadError = TRUE;
            break;

          case REM_UNDEF:
            ErrorBox(ERR_RegExpr_Undef, (LPSTR)findReplace.findWhat);
            frMem.hadError = TRUE;
            break;

          default:
            break;

        }

        Dbg(LocalFree((HANDLE)stack) == NULL);

    } while (!done);

    return FALSE;
}

BOOL PASCAL
SearchLineBack(
    int lineNb,
    int *xStart,
    int *xEnd,
    PSTR findWhat
    )
{
    char line[MAX_USER_LINE + 1];
    BOOL found = FALSE;
    int xLimit = min(*xStart, elLen);

    //Make a local copy of the line
    memmove(line, (LPSTR)el, xLimit);
    line[xLimit] = '\0';

    if (findReplace.regExpr) {

        while (!found && *xStart >= 0)
            if (!(found = RESearch(line, xStart, xEnd)))
                (*xStart)--;

    } else {
        register int xCur;
        register int k;
        int limit = lstrlen(findWhat);

        if (!findReplace.matchCase)
            AnsiUpper((LPSTR)line);

        while (!found && *xStart >= 0) {

            xCur = *xStart;

            for (; xCur < xLimit;
                IsDBCSLeadByte(line[xCur]) ? xCur+=2 : xCur++) {

                if (xCur + limit > elLen) {
                    break;
                }

                //Select type of comparison according to user choice
                k = 0;
                while (k < limit && line[xCur + k] == findWhat[k]) {
                    if (IsDBCSLeadByte(line[xCur + k])) {
                        if (xCur + k + 1 >= xLimit) {
                            break;
                        }
                        if (line[xCur + k + 1] != findWhat[k + 1]) {
                            break;
                        }
                        k += 2;
                    } else {
                        k++;
                    }
                }

                if (k == limit) {
                    *xEnd = xCur + k;

                    //If we search for a whole word
                    if (findReplace.wholeWord) {
                        BOOL separator;

                        BOOL bDBCS = FALSE;
                        int  i;

                        for (i = 0; i < xCur; i++) {
                            if (IsDBCSLeadByte(line[i])) {
                                bDBCS = TRUE;
                                i++;
                            } else {
                                bDBCS = FALSE;
                            }
                        }
                        if (xCur > 0) {
                            if (bDBCS) {
                                separator = TRUE;
                            } else {
                                separator = !CHARINALPHASET(line[xCur - 1]);
                            }
                        } else {
                            separator = TRUE;
                        }
                        if (separator) {
                            if (xCur + limit < elLen) {
                                separator = !CHARINALPHASET(line[xCur + limit]);
                            } else {
                                separator = TRUE;
                            }
                        }
                        if (separator) {
                            *xStart = xCur;
                            found = TRUE;
                        }
                        break;
                    } else {
                        *xStart = xCur;
                        found =  TRUE;
                        break;
                    }
                }
            }

            if (!found) {
                (*xStart)--;
            }
        }
    }

    if (frMem.oneLineDone
           && lineNb == frMem.stopLine
           && (*xStart <= frMem.stopCol || !found)) {
        frMem.allFileDone = TRUE;
    }

    if (!found) {
        *xEnd = *xStart = MAX_LINE_SIZE;
    }

    return found;
}

BOOL PASCAL
FindPrev(
    long startingLine,
    int startingCol,
    BOOL startFromSelection,
    BOOL selectFoundText,
    BOOL errorIfNotFound,
    HCURSOR hSaveCursor,
    PSTR findWhat
    )
{
    LPLINEREC pl;
    LPBLOCKDEF pb;
    LPVIEWREC v = &Views[curView];
    BOOL found = FALSE;
    int endingCol;

    Assert(v->Doc >= 0);

    //The caller wants us to start from begin of selected text
    if (startFromSelection) {

        long XL,XR;
        long YL,YR;

        Assert(v->BlockStatus);
        GetBlockCoord(curView, &XL, &YL, &XR, &YR);
        startingCol = XL;
    }

    //Scan text from current line till beginning of file
    if (!FirstLine(v->Doc, &pl, &startingLine, &pb)) {
        goto err;
    }

    startingLine--;

    do {
        found = SearchLineBack(startingLine,
                               &startingCol,
                               &endingCol,
                               findWhat);
        if (frMem.hadError) {
            goto out;
        }
        if (found) {
            break;
        }
        if (!PreviousLine(v->Doc, &pl, startingLine, &pb)) {
            goto err;
        }
        startingLine--;
        frMem.oneLineDone = TRUE;
    } while (startingLine >= 0);

    startingLine = max(startingLine, 0);
    CloseLine(v->Doc, &pl, startingLine + 1, &pb);

    //Scan text from end till current line
    if (!found) {

        if (!frMem.replaceAll && (!frMem.allFileDone || !frMem.replacing)) {
            if ( IsWindowVisible(GetHwnd_StatusBar()) ) {
                ErrorBox(STA_Find_Hit_EOF);
            } else {
                SetMessageText_StatusBar(STA_Find_Hit_EOF, STATUS_INFOTEXT);
            }
        }

        startingLine = Docs[v->Doc].NbLines - 1;
        if (!FirstLine(v->Doc, &pl, &startingLine, &pb)) {
            goto err;
        }
        startingLine--;

        do {

            found = SearchLineBack(startingLine,
                                   &startingCol,
                                   &endingCol,
                                   findWhat);
            if (frMem.hadError) {
                goto out;
            }
            if (found) {
                break;
            }
            if (!PreviousLine(v->Doc, &pl, startingLine, &pb)) {
                goto err;
            }
            startingLine--;
            frMem.oneLineDone = TRUE;
        } while (startingLine >= frMem.line);

        startingLine = max(startingLine, 0);
        CloseLine(v->Doc, &pl, startingLine + 1, &pb);
    }

    if (found) {

        //Next find will start from here
        frMem.line = startingLine;
        frMem.leftCol = frMem.rightCol = startingCol;

        if (!frMem.replaceAll) {
            RefreshView(startingCol, endingCol, selectFoundText);
        }

    } else if (errorIfNotFound) {
        if (findReplace.regExpr) {
            ErrorBox(ERR_No_RegExp_Match, (LPSTR)findReplace.findWhat);
        } else {
            ErrorBox(ERR_String_Not_Found, (LPSTR)findReplace.findWhat);
        }
    }

    //Restore cursor
    SetCursor(hSaveCursor);

    return found;

    err : {
        //Restore cursor
        SetCursor(hSaveCursor);

        Assert(FALSE);
        return FALSE;
    }

    out : {
        //Restore cursor
        SetCursor(hSaveCursor);
        CloseLine(v->Doc, &pl, startingLine, &pb);
        return FALSE;
    }
}

BOOL PASCAL
SearchLineForw(
    int lineNb,
    int *xStart,
    int *xEnd,
    PSTR findWhat
    )
{

    char line[MAX_USER_LINE + 1];
    BOOL found = FALSE;
    int xLimit = elLen;

    //Make a local copy of the line
    memmove(line, (LPSTR)el, xLimit);
    line[xLimit] = '\0';

    if (findReplace.regExpr) {
        found = RESearch(line, xStart, xEnd);
    } else {

        register int xCur = *xStart;
        register int k;
        int limit = lstrlen(findWhat);

        if (!findReplace.matchCase) {
            AnsiUpper((LPSTR)line);
        }

        for (; xCur < xLimit;
            IsDBCSLeadByte(line[xCur]) ? xCur+=2 : xCur++) {

            if (xCur + limit > elLen) {
                break;
            }

            //Select type of comparison according to user choice
            k = 0;
            while (k < limit && line[xCur + k] == findWhat[k]) {
                if (IsDBCSLeadByte(line[xCur + k])) {
                    if (xCur + k + 1 >= xLimit)
                        break;
                    if (line[xCur + k + 1] != findWhat[k + 1])
                        break;
                    k += 2;
                } else {
                    k++;
                }
            }

            if (k == limit) {
                *xEnd = xCur + k;

                //If we search for a whole word
                if (!findReplace.wholeWord) {

                    *xStart = xCur;
                    found =  TRUE;
                    break;

                } else {
                    BOOL separator;
                    BOOL bDBCS = FALSE;
                    int  i;

                    for (i = 0; i < xCur; i++) {
                        if (IsDBCSLeadByte(line[i])) {
                            bDBCS = TRUE;
                            i++;
                        } else {
                            bDBCS = FALSE;
                        }
                    }
                    if (xCur > 0) {
                        if (bDBCS) {
                            separator = TRUE;
                        } else {
                            separator = !CHARINALPHASET(line[xCur - 1]);
                        }
                    } else {
                        separator = TRUE;
                    }
                    if (separator) {
                        if (xCur + limit < xLimit) {
                            separator = !CHARINALPHASET(line[xCur + limit]);
                        } else {
                            separator = TRUE;
                        }
                    }
                    if (separator) {
                        *xStart = xCur;
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if (frMem.oneLineDone
           && lineNb - 1 == frMem.stopLine
           && (*xStart >= frMem.stopCol || !found))
    {
        frMem.allFileDone = TRUE;
    }

    if (!found) {
        *xEnd = *xStart = 0;
    }

    return found;
}

BOOL FAR PASCAL
FindNext(
        HWND hwndParent,
        long startingLine,
        int startingCol,
        BOOL startFromSelection,
        BOOL selectFoundText,
        BOOL errorIfNotFound
        )
{
    LPLINEREC pl;
    LPBLOCKDEF pb;
    LPVIEWREC v = &Views[curView];
    BOOL found = FALSE;
    HCURSOR hSaveCursor;
    char findWhat[MAX_USER_LINE + 1];
    int endingCol;

    Assert(v->Doc >= 0);

    frMem.hadError = FALSE;

    //Transfer target to a near variable, and upcase it if we are making
    //a no case
    strcpy(findWhat, findReplace.findWhat);
    if (!findReplace.matchCase) {
        AnsiUpper((LPSTR)findWhat);
    }

    //Set the Hour glass cursor
    hSaveCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    frMem.line = startingLine;
    SetMessageText_StatusBar(SYS_StatusClear, STATUS_INFOTEXT);

    //Set elements if regular expressions mode is selected
    if (findReplace.regExpr) {

        //For unknown reasons RE Engine take care of the dynamic Allocs
        //but you have to do the freeing ...
        if (pat != NULL) {
            Dbg(LocalFree((HANDLE)pat) == NULL);
            pat = NULL;
        }

        //Compile the regular expression, selecting Unix mode
        //(WARNING : RECompile can return NULL)
        pat = RECompile(findWhat,
                        (flagType)findReplace.matchCase,
                        (flagType)FALSE);
    }

    //Another func takes care of the !@?!ing search going Up
    if (findReplace.goUp) {
        return FindPrev(startingLine,
                        startingCol,
                        startFromSelection,
                        selectFoundText,
                        errorIfNotFound,
                        hSaveCursor,
                        findWhat);
    }

    //The caller wants us to start from end of selected text
    if (startFromSelection && v->BlockStatus) {

        long XL,XR;
        long YL,YR;

        GetBlockCoord(curView, &XL, &YL, &XR, &YR);
        startingCol = XL;

        //When replacing, we want to first replace the word selected
        if (!frMem.replacing) {
            startingCol++;
        }

        //Adjust stop column
        if (YL == frMem.stopLine) {
            frMem.stopCol = XL;
        }
    }

    //Scan text from current line till end of file
    if (!FirstLine(v->Doc, &pl, &startingLine, &pb)) {
        goto err;
    }

    do {

        found = SearchLineForw(startingLine, &startingCol, &endingCol, findWhat);
        if (frMem.hadError) {
            goto out;
        }
        if (found) {
            break;
        }

        if (!NextLine(v->Doc, &pl, &startingLine, &pb)){
            goto err;
        }
        frMem.oneLineDone = TRUE;

    } while (startingLine != LAST_LINE);
    CloseLine(v->Doc, &pl, startingLine, &pb);

    //Scan text from beginning till current line
    if (!found) {

        if (!frMem.replaceAll && (!frMem.allFileDone || !frMem.replacing)) {
            SetMessageText_StatusBar(STA_Find_Hit_EOF, STATUS_INFOTEXT);
            ErrorBox2(hwndParent, MB_TASKMODAL, STA_Find_Hit_EOF);
        }

        startingLine = 0;
        if (!FirstLine(v->Doc, &pl, &startingLine, &pb)) {
            goto err;
        }

        do {
            found = SearchLineForw(startingLine,
                                   &startingCol,
                                   &endingCol,
                                   findWhat);
            if (frMem.hadError) {
                goto out;
            }
            if (found) {
                break;
            }
            if (!NextLine(v->Doc, &pl, &startingLine, &pb)) {
                goto err;
            }
            frMem.oneLineDone = TRUE;
        } while (startingLine != LAST_LINE
                      || startingLine - 1 <= frMem.line);
        CloseLine(v->Doc, &pl, startingLine, &pb);
    }

    if (found) {

        //Next find will start from here
        frMem.line = startingLine - 1;
        frMem.leftCol = startingCol;

        if (frMem.replacing) {
            frMem.rightCol = endingCol;
        } else {
            frMem.rightCol = startingCol + 1;
        }

        if (!frMem.replaceAll) {
            RefreshView(startingCol, endingCol, selectFoundText);
        }

    } else if (errorIfNotFound) {

        if (findReplace.regExpr) {
            ErrorBox(ERR_No_RegExp_Match, (LPSTR)findReplace.findWhat);
        } else {
            ErrorBox(ERR_String_Not_Found, (LPSTR)findReplace.findWhat);
        }

    }

    //Restore cursor
    SetCursor(hSaveCursor);

    return found;

    err : {
        //Restore cursor
        SetCursor(hSaveCursor);

        Assert(FALSE);
        return FALSE;
    }

    out : {
        //Restore cursor
        SetCursor(hSaveCursor);

        CloseLine(v->Doc, &pl, startingLine, &pb);

        return FALSE;
    }
}

void FAR PASCAL
ReplaceOne(
    void
    )
{
    BOOL prevStopMark = stopMarkStatus;

    int repLen = lstrlen(findReplace.replaceWith);

    DestroyRecBuf(Views[curView].Doc, REC_REDO);

    //Do the replacement
    DeleteBlock(Views[curView].Doc,
                frMem.leftCol,
                frMem.line,
                frMem.rightCol,
                frMem.line);

    if (repLen > 0) {
        //More than 1 edit action for this, operation. Tell the
        //undo/redo engine that this record have no stop marks
        stopMarkStatus = HAS_NO_STOP;

        InsertBlock(Views[curView].Doc,
                    frMem.leftCol,
                    frMem.line,
                    repLen,
                    (LPSTR)findReplace.replaceWith);

        //Put undo/redo engine back in normal state
        if (prevStopMark == HAS_STOP) {
            stopMarkStatus = HAS_STOP;
        }
    }

    //Reajust the stop column if line is the stopLine and if we
    //replaced before the stop column
    if (frMem.line == frMem.stopLine && frMem.leftCol < frMem.stopCol) {
        frMem.stopCol += repLen - (frMem.rightCol - frMem.leftCol);
    }

    //Reajust the next start
    frMem.rightCol = frMem.leftCol + repLen;

    //Count the occurences
    frMem.nbReplaced++;
}

void
FAR PASCAL
ReplaceAll(
          HWND hwndParent
          )
{
    BOOL prevStopMark = stopMarkStatus;

    //More than 1 edit action for this, operation. Tell the
    //undo/redo engine that records have no stop marks after
    //this record
    stopMarkStatus = NEXT_HAS_NO_STOP;

    ReplaceOne();
    while (FindNext(hwndParent, frMem.line, frMem.rightCol, FALSE, TRUE, FALSE)
           && !frMem.allFileDone && !frMem.hadError) {

        ReplaceOne();
    }

    InvalidateLines(curView, 0, LAST_LINE, FALSE);

    //Put undo/redo engine back in normal state
    if (prevStopMark == HAS_STOP) {
        stopMarkStatus = HAS_STOP;
    }
}

void
FAR PASCAL
Replace(
       HWND hwndParent
       )
{
    MSG msg;
    LPVIEWREC v = &Views[curView];

    Assert(v->Doc >= 0);

    //If we replace all, do it. Else control replace with a modeless box
    if (frMem.replaceAll) {
        ReplaceAll(hwndParent);
    } else {
        //Create the Confirm dialog box (modeless)
        //Dbg((LONG) (frMem.lpConfirmProc = MakeProcInstance(DlgConfirm, g_hInst)));

        frMem.lpConfirmProc = DlgConfirm;

        Dbg(frMem.hDlgConfirmWnd = CreateDialog(g_hInst,
                                                MAKEINTRESOURCE(DLG_CONFIRM),
                                                hwndFrame,
                                                frMem.lpConfirmProc));
        //Now show Confirm dialog
        SetDialogPos(TRUE, frMem.hDlgConfirmWnd);

        frMem.exitModelessReplace = FALSE;

        //Process our message loop until user Confirm dialog box is over
        while (!frMem.exitModelessReplace
               && hwndActiveEdit
               && !TerminatedApp
               && GetMessage(&msg, NULL, 0, 0)) {

            if (msg.message == WM_SYSKEYDOWN
                && (msg.wParam >= '0' && msg.wParam <= '9')) {

                SetFocus(hwndFrame);
                ProcessQCQPMessage(&msg);

            } else if (msg.message == WM_KEYDOWN && msg.wParam == VK_F3) {

                if (GetFocus() != frMem.hDlgConfirmWnd) {

                    ProcessQCQPMessage(&msg);

                } else {
                    frMem.exitModelessReplace = (!FindNext(frMem.hDlgConfirmWnd,
                                                           frMem.line,
                                                           frMem.rightCol,
                                                           FALSE,
                                                           TRUE,
                                                           FALSE)
                                                 || frMem.allFileDone
                                                 || frMem.hadError);
                }

            } else if (IsDialogMessage(frMem.hDlgConfirmWnd, &msg)) {

                if (msg.message == WM_KEYDOWN && msg.wParam == VK_F1) {
                    Dbg(PostMessage(frMem.hDlgConfirmWnd,
                                    WM_COMMAND,
                                    IDWINDBGHELP,
                                    0L));
                }

            } else {

                ProcessQCQPMessage(&msg);

            }
        }

       findReplace.goUp = frMem.goUpCopy;

       if (!TerminatedApp && hwndActiveEdit) {
           SetFocus(hwndActiveEdit);
       }

       //Destroy the Confirm dialog box
       if (IsWindow(frMem.hDlgConfirmWnd)) {
           Dbg(DestroyWindow(frMem.hDlgConfirmWnd));
           frMem.hDlgConfirmWnd = 0;
       }

       //Destroy proc instance
       //FreeProcInstance(frMem.lpConfirmProc);

    }

    if (!TerminatedApp && hwndActiveEdit) {

        //Clear selection
        ClearSelection(curView);
        if (frMem.replaceAll) {
            PosXY(curView, v->X, v->Y, TRUE);
        } else {
            PosXY(curView, v->BlockXL, v->BlockYL, TRUE);
        }

        //Prompt the number of replace done (if any)
        if (frMem.nbReplaced) {
            if (IsWindowVisible(GetHwnd_StatusBar()) ) {
                ErrorBox(SYS_Nb_Of_Occurrences_Replaced, frMem.nbReplaced);
            } else {
                SetMessageText_StatusBar(SYS_Nb_Of_Occurrences_Replaced,
                    STATUS_INFOTEXT, frMem.nbReplaced);
            }
        }

    }

    frMem.replaceAll = FALSE;
    frMem.replacing = FALSE;

}

/***    Find
**
**  Synopsis:
**      void = Find()
**
**  Entry:
**      None
**
**  Returns:
**      Nothing
**
**  Description:
**      This function deals with the "during" find dialog box.  It looks
**      for the "next" item if needed
**
*/

void FAR PASCAL Find(void)
{
    MSG msg;

    //Control find progression with a modeless box
    //Create the FindNext dialog box (modeless)

    //Dbg((LONG) (frMem.lpFindNextProc =  MakeProcInstance(DlgFindNext, g_hInst)));

    frMem.lpFindNextProc = DlgFindNext;

    Dbg(frMem.hDlgFindNextWnd = CreateDialog(g_hInst,
                                             MAKEINTRESOURCE(DLG_FINDNEXT),
                                             hwndFrame,
                                             frMem.lpFindNextProc));

    //Now show FindNext dialog

    SetDialogPos(TRUE, frMem.hDlgFindNextWnd);

    frMem.exitModelessFind = FALSE;

    //Process our message loop until user FindNext dialog box is over

    while (!frMem.exitModelessFind
           && hwndActiveEdit
           && !TerminatedApp
           && GetMessage(&msg, NULL, 0, 0)) {

        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F3) {
            if (GetFocus() == frMem.hDlgFindNextWnd) {
                frMem.exitModelessFind = (!FindNext(frMem.hDlgFindNextWnd,
                                                    frMem.line,
                                                    frMem.rightCol,
                                                    FALSE,
                                                    TRUE,
                                                    FALSE)
                                          || frMem.allFileDone
                                          || frMem.hadError);
            } else {
                ProcessQCQPMessage(&msg);
            }
        } else if (msg.message == WM_SYSKEYDOWN
                   && (msg.wParam >= '0' && msg.wParam <= '9')) {
            SetFocus(hwndFrame);
            ProcessQCQPMessage(&msg);
        } else if (IsDialogMessage(frMem.hDlgFindNextWnd, &msg)) {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_F1) {
                PostMessage(frMem.hDlgFindNextWnd, WM_COMMAND, IDWINDBGHELP, 0L);
            }
        } else {
            ProcessQCQPMessage(&msg);
        }
    }

    //Give focus to the edit window

    if (!TerminatedApp && hwndActiveEdit) {

        SetFocus(hwndActiveEdit);

        //Clear the selection
        if (Views[curView].Doc >= 0) {
            ClearSelection(curView);

            if (frMem.allTagged) {
                PosXY(curView, 0, Views[curView].BlockYL, TRUE);
            } else if (Views[curView].BlockXL != Views[curView].X) {
                PosXY(curView, Views[curView].X, Views[curView].Y, TRUE);
            } else {
                PosXY(curView, Views[curView].BlockXL,
                      Views[curView].BlockYL, TRUE);
            }
        }
    }

    //Destroy the FindNext dialog box

    if (IsWindow(frMem.hDlgFindNextWnd)) {
        Dbg(DestroyWindow(frMem.hDlgFindNextWnd));
        frMem.hDlgFindNextWnd = 0;
    }

    //Destroy proc instance
    //FreeProcInstance(frMem.lpFindNextProc);

    return;
}                                       /* Find() */

void
FAR PASCAL
TagAll(
      HWND hwndParent,
      int y
      )
{
    Assert(Views[curView].Doc >= 0);

    frMem.replaceAll = TRUE;
    Views[curView].X = Views[curView].Y = frMem.line = frMem.rightCol = 0;
    SetStopLimit();
    while (FindNext(hwndParent, frMem.line, frMem.rightCol, FALSE, TRUE, FALSE)
           && !frMem.allFileDone
           && !frMem.hadError) {

        LineStatus(Views[curView].Doc, frMem.line + 1, TAGGED_LINE,
                   LINESTATUS_ON, FALSE, FALSE);
    }

    frMem.replaceAll = FALSE;
    ClearSelection(curView);
    InvalidateLines(curView, 0, LAST_LINE, FALSE);
    PosXY(curView, 0 , y, FALSE);
}

BOOL
FAR PASCAL
InsertInPickList(
                WORD type
                )
{
    int i, j;
    LPSTR s[MAX_PICK_LIST] = {0};
    BOOL found = FALSE;
    int nb = findReplace.nbInPick[type];
    LPSTR newString;

    if (type == FIND_PICK) {
        newString = findReplace.findWhat;
    } else {
        newString = findReplace.replaceWith;
    }

    //First lock memory for existing strings
    for (i = 0; i < nb; i++) {
        Dbg(s[i] = (LPSTR)GlobalLock(findReplace.hPickList[type][i]));
    }

    //First check if string is not already in list
    i = 0;
    while (i < nb && !found) {
        found = (_fstrcmp(s[i], newString) == 0);
        if (!found) {
            i++;
        }
    }

    if (found) {

        //String already exist, move it to first place
        if (i > 0) {
            lstrcpy((LPSTR)szTmp, s[i]);
            for (j = i; j > 0; j--) {
                lstrcpy(s[j], s[j - 1]);
            }
            lstrcpy(s[0], (LPSTR)szTmp);
        }
    } else {

        //String not found, do we have a new space to create ?
        if (nb < MAX_PICK_LIST) {
            Dbg(findReplace.hPickList[type][nb] =
                    GlobalAlloc(GMEM_MOVEABLE, MAX_USER_LINE + 1));
            Dbg(s[nb] = (LPSTR)GlobalLock(findReplace.hPickList[type][nb]));
            nb++;
        }

        //Shift list
        for (i = nb - 1; i >= 1; i--) {
            lstrcpy(s[i], s[i - 1]);
        }
        lstrcpy(s[0], newString);
    }

    //Now list is Most Recently Used sorted, unlock memory
    for (i = 0 ; i < nb; i++) {
        Dbg(GlobalUnlock (findReplace.hPickList[type][i]) == FALSE);
    }

    findReplace.nbInPick[type] = nb;
    return !found;
}


void
FAR PASCAL
RemoveFromPick(
              WORD type
              )
{
    int i;

    if (findReplace.nbInPick[type] <= 0) {
        return;
    }

    //Free the first element
    Dbg(GlobalFree(findReplace.hPickList[type][0]) == NULL);

    //Shift the pick list
    for (i = 1; i < findReplace.nbInPick[type]; i++) {
        findReplace.hPickList[type][i - 1] = findReplace.hPickList[type][i];
    }

    findReplace.nbInPick[type]--;
}
