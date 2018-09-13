/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    watch.c

Abstract:

    This module contains the routines to deal with the watch
    dialog box.

Author:

    William J. Heaton (v-willhe) 20-Jul-1992
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993
    
Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

//
//  Preprocessor Macros
//

#define STYLE(Id) ((WORD)GetWindowLong(GetDlgItem(hDlg, Id), GWL_STYLE))
#define ITEM(val) (GetDlgItem(hDlg, val))
#define ISITEMDEFAULT(i) ( (LOWORD(GetWindowLong(ITEM(i),GWL_STYLE)) == LOWORD(BS_DEFPUSHBUTTON)) )
#define WatchSelection wGeneric1

//
//  Function Prototypes (LOCAL)

VOID AddWatchExpression(HWND);
VOID DeleteWatchExpression(HWND);
VOID WatchExpressionHandler( HWND hDlg, WPARAM wParam, LPARAM lParam);
VOID WatchListHandler( HWND hDlg, WPARAM wParam, LPARAM lParam);
VOID OkButtonHandler( HWND hDlg, WPARAM wParam, LPARAM lParam);

/***    WatchDefPushButton
**
**  Synopsis:
**      void = WatchDefPushButton(hDlg, ButtonId)
**
**  Entry:
**      HWND hDlg    Dialog Window Handle
**      int Id       Button Id
**
**  Returns:
**      Nothing
**
**  Description:
**
*/

void NEAR PASCAL WatchDefPushButton(HWND hDlg, int Id)
{
    // Undo the current default push button

    if ( Id != IDOK && STYLE(IDOK) == BS_DEFPUSHBUTTON) {
        PostMessage(ITEM(IDOK), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }

    if ( Id != IDCANCEL && STYLE(IDCANCEL) == BS_DEFPUSHBUTTON) {
        PostMessage(ITEM(IDCANCEL), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }

    if ( Id != IDWINDBGHELP && STYLE(IDWINDBGHELP) == (WORD)BS_DEFPUSHBUTTON) {
        PostMessage(ITEM(IDWINDBGHELP), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }

    if ( Id != ID_WATCH_ADD && STYLE(ID_WATCH_ADD) == BS_DEFPUSHBUTTON) {
        PostMessage(ITEM(ID_WATCH_ADD), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }

    if ( Id != ID_WATCH_DELETE && STYLE(ID_WATCH_DELETE) == BS_DEFPUSHBUTTON) {
        PostMessage(ITEM(ID_WATCH_DELETE), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }

    if ( Id != ID_WATCH_CLEARALL && STYLE(ID_WATCH_CLEARALL) == BS_DEFPUSHBUTTON) {
        PostMessage(ITEM(ID_WATCH_CLEARALL), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
    }

    if (STYLE(Id) != BS_DEFPUSHBUTTON) {
        PostMessage(ITEM(Id), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
    }

    return;
}                                       /* WatchDefPushButton() */

/***    FillWatchListbox
**
**  Synopsis:
**      void = FillWatchListbox(hDlg)
**
**  Entry:
**      hDlg    - handle to dialog box containning the list box
**
**  Returns:
**      Nothing
**
**  Description:
**      Fill the watch list box with the current watches
**
*/

void PASCAL NEAR FillWatchListbox(HWND hDlg)
{
    PTRVIT pVit = InitWatchVit();
    PTRVIB pVib;

    DAssert(pVit);

    // If no VIB'S, No Clear All Button

    pVib = pVit->pvibChild;
    if (pVib == NULL) {
        EnableWindow(ITEM(ID_WATCH_CLEARALL), FALSE);
    }

    // While we have a vib, add the expression to the listbox
    // and mark it as a original.

    while (pVib) {
        SendMessage( ITEM(ID_WATCH_CUREXPRESSION), LB_ADDSTRING,
                     0,(LPARAM)(LPSTR)pVib->pwoj->szExpStr);

        pVib->flags.DlgOrig = TRUE;
        pVib->flags.DlgAdd  = FALSE;
        pVib->flags.DlgDel  = FALSE;

        pVib = pVib->pvibSib;
    }
    return;
}                                       /* FillWatchListbox() */


/***    AddWatchListbox
**
**  Synopsis:
**      void = AddWatchListbox(hDlg, szExp)
**
**  Entry:
**      HWND hDlg    - handle of dialog containning the list box
**      PSTR szExp   - Pointer to the Expression to be added
**
**  Returns:
**      nothing
**
**  Description:
**      Creates the Vib, Makrs it as an add, and updates the Watch
**      listbox. Also a sucessful add turns on the clear all button
**
*/

void PASCAL NEAR AddWatchListbox(HWND hDlg, PSTR szExpress)
{
    PTRVIB pVib;
    PTRVIT pVit = GetWatchVit();

    pVib = AddCVWatch( pVit, szExpress);

    if ( pVib) {
        pVib->flags.DlgAdd = TRUE;

        SendMessage(ITEM(ID_WATCH_CUREXPRESSION),LB_ADDSTRING, 0, (LPARAM)szExpress);
        EnableWindow(ITEM(ID_WATCH_CLEARALL), TRUE);
    }

    return;
}

/***    DeleteWatchListbox
**
**  Synopsis:
**      void = DeleteWatchListbox(hDlg, WatchNum)
**
**  Entry:
**      hDlg    - Dialog containning the listbox
**      WatchNum -
**
**  Returns:
**      Nothing
**
**  Description:
**      Mark the passed watch as deleted and update the watch list box.
*/

void PASCAL NEAR DeleteWatchListbox(HWND hDlg, int WatchNum)
{
    PTRVIT pVit = GetWatchVit();
    PTRVIB pVib = pVit->pvibChild;
    LRESULT NumItems;
    int    i = 0;

    //
    // Find the Nth Vib
    //

    while (pVib) {

        //  If it isn't a pending delete, check if its the
        //  target otherwise bump the count

        if ( !pVib->flags.DlgDel ) {
            if ( i == WatchNum ) break;
            i++;
        }
        pVib = pVib->pvibSib;
    }
    if ( !pVib ) return;

    //
    // Mark it as deleted and remove it from listbox
    //

    pVib->flags.DlgDel = TRUE;
    SendMessage(ITEM(ID_WATCH_CUREXPRESSION), LB_DELETESTRING, WatchNum, 0L);

    //
    // Reset the highlights and buttons if needed
    //

    NumItems = SendMessage(ITEM(ID_WATCH_CUREXPRESSION),LB_GETCOUNT,0,0L);

    if (NumItems > 0) {
        if (WatchNum == NumItems) WatchNum--;
        SendMessage( ITEM(ID_WATCH_CUREXPRESSION),LB_SETCURSEL,WatchNum,0L);
    }

    else {
        SetFocus(ITEM(ID_WATCH_EXPRESSION));
        EnableWindow(ITEM(ID_WATCH_CLEARALL), FALSE);
        EnableWindow(ITEM(ID_WATCH_DELETE), FALSE);
    }
    return;
}                                       /* DeleteWatchListbox() */


/***    ClearAllWatch
**
**  Synopsis:
**      void = ClearAllWatch(hDlg)
**
**  Entry:
**      hDlg    - Handle to dialog containning list box
**
**  Returns:
**      Nothing
**
**  Description:
**      Mark all the watches as deleted and clear the listbox
**
*/

void PASCAL NEAR ClearAllWatch(HWND hDlg)
{
    PTRVIT pVit = GetWatchVit();
    PTRVIB pVib = pVit->pvibChild;

    // Set the Pending Delete on all vibs
    while (pVib) {
        pVib->flags.DlgDel = TRUE;
        pVib = pVib->pvibSib;
    }

    // Clear the watch list box
    SendMessage(ITEM(ID_WATCH_CUREXPRESSION),LB_RESETCONTENT, 0, 0L);
    SetFocus(ITEM(ID_WATCH_EXPRESSION));

    // Grey Delete and Clear All
    EnableWindow(ITEM(ID_WATCH_CLEARALL), FALSE);
    EnableWindow(ITEM(ID_WATCH_DELETE), FALSE);

    return;
}                                       /* ClearAllWatch() */

/***    OKWatch
**
**  Synopsis:
**      void = OKWatch()
**
**  Entry:
**      None
**
**  Returns:
**      Nothing
**
**  Description:
**      Delete any watches marked for deletion, and reset the
**      watch   editing flags.  Update the watch window to reflect
**      any changes.
**
*/

void PASCAL NEAR OKWatch(void)
{
    PTRVIT pVit = GetWatchVit();
    PTRVIB pVib = pVit->pvibChild;
    PTRVIB pVibTemp;
    BOOL   fChanged = FALSE;


    while (pVib) {

        if ( pVib->flags.DlgDel ) {
            pVibTemp = pVib->pvibSib;
            DeleteCVWatch(pVit, pVib);
            pVib = pVibTemp;
            fChanged = TRUE;
        }

        else {
            if ( pVib->flags.DlgAdd ) fChanged = TRUE;
            pVib->flags.DlgAdd  = FALSE;
            pVib->flags.DlgOrig = FALSE;
            pVib = pVib->pvibSib;
        }
    }

    // If we have watchs, but no watchwindow, create one

    if ( pVit->pvibChild && GetWatchHWND() == NULL) {
        PostMessage(hwndFrame, WM_COMMAND, IDM_VIEW_WATCH, 0L);
        return;
    }

    // Refresh watch window

    if ( fChanged ) {
        UpdateCVWatchs();
    }

    return;
}                                       /* OKWatch() */

/***    CancelWatch
**
**  Synopsis:
**      void = CancelWatch()
**
**  Entry:
**      none
**
**  Returns:
**      Nothing
**
**  Description:
**      Delete any watches marked as added and reset the watch
**      editing flags.
**
*/

void PASCAL NEAR CancelWatch(void)
{
    PTRVIT pVit     = GetWatchVit();
    PTRVIB pVib     = pVit->pvibChild;
    PTRVIB pVibTemp;
    BOOL   fChanged = FALSE;

    while (pVib) {

        if ( !pVib->flags.DlgOrig ) {
            pVibTemp = pVib->pvibSib;
            DeleteCVWatch(pVit, pVib);
            pVib = pVibTemp;
            fChanged = TRUE;
        }

        else {
            pVib->flags.DlgAdd  = FALSE;
            pVib->flags.DlgDel  = FALSE;
            pVib->flags.DlgOrig = FALSE;
            pVib = pVib->pvibSib;
        }

    }
    return;
}                                       /* CancelWatch() */


/***    AddWatchExpression
**
**  Synopsis:
**      void AddWatchExpression(hDlg);
**
**  Entry:
**      hDlg    - Handle to dialog for which to process the message
**
**  Returns:
**
**  Description:
**      Addes a watch to the watch list
**
**
*/


VOID AddWatchExpression( HWND hDlg )
{
    HWND hExp = ITEM(ID_WATCH_EXPRESSION);
    char szExpr[MAX_EXPRESS_SIZE];

    // get the input expression
    *szExpr = '\0';
    GetDlgItemText(hDlg, ID_WATCH_EXPRESSION, (LPSTR)szExpr, sizeof(szExpr)-1);

    if (CheckExpression(szExpr, radix, TRUE)) {
        AddWatchListbox(hDlg, szExpr );
    }

    else
        MessageBeep(0);

    // Either way put focus back in the edit field
    SendMessage(hExp, WM_SETTEXT, 0, (LPARAM)(LPSTR)szNull);
    SetFocus(hExp);
}

/***    DeleteWatchExpression
**
**  Synopsis:
**      void DeleteWatchExpression(hDlg);
**
**  Entry:
**      hDlg    - Handle to dialog for which to process the message
**
**  Returns:
**
**  Description:
**      Delete the current line in the watch list box.
**
**
*/

VOID DeleteWatchExpression( HWND hDlg )
{
    WORD WatchNum;
    HWND hList = ITEM(ID_WATCH_CUREXPRESSION);

    WatchNum = (WORD)SendMessage(hList, LB_GETCURSEL, 0, 0L);
    if (WatchNum != LB_ERR) {
        DeleteWatchListbox(hDlg, WatchNum);
    }

    else{
        MessageBeep(0);
    }
}

/***    WatchExpressionHandler
**
**  Synopsis:
**      WatchExpressionHandler(hDlg, wParam, lParam)
**
**  Entry:
**      hDlg    - Handle to dialog for which to process the message
**      wParam  - info about the message
**      lParam  - info about the message
**
**  Returns:
**
**  Description:
**      Handles the messages that orginated from the Watch Expression
**      edit control.
**
*/


VOID WatchExpressionHandler( HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    char szExpr[MAX_EXPRESS_SIZE];
    BOOL fHasText = GetDlgItemText(hDlg, ID_WATCH_EXPRESSION, (LPSTR)szExpr, sizeof(szExpr)-1);

    switch (HIWORD(wParam)) {

        case EN_SETFOCUS:
            SendMessage((HWND) lParam, EM_SETSEL, 0, 0xffffffff);
            SendMessage(ITEM(ID_WATCH_CUREXPRESSION),LB_SETCURSEL, 0xffffffff, 0L);

            EnableWindow(ITEM(ID_WATCH_DELETE), FALSE);

            if ( fHasText ) {
                EnableWindow(ITEM(ID_WATCH_ADD), TRUE);
                WatchDefPushButton(hDlg, ID_WATCH_ADD);

                PostMessage(ITEM(IDOK),BM_SETSTYLE, (WORD)BS_PUSHBUTTON, TRUE);
                PostMessage(ITEM(ID_WATCH_ADD),BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
            }

            else {
                EnableWindow(ITEM(ID_WATCH_ADD), FALSE);
                WatchDefPushButton(hDlg, IDOK);
            }
            break;

        case EN_CHANGE:

            if ( fHasText ) {
                EnableWindow(ITEM(ID_WATCH_ADD), TRUE);
                WatchDefPushButton(hDlg, ID_WATCH_ADD);
            }

            else {
                EnableWindow(ITEM(ID_WATCH_ADD), FALSE);
                WatchDefPushButton(hDlg, IDOK);
            }
            break;
    }
    return;
}

/***    WatchExpressionHandler
**
**  Synopsis:
**      WatchListHandler(hDlg, wParam, lParam)
**
**  Entry:
**      hDlg    - Handle to dialog for which to process the message
**      wParam  - info about the message
**      lParam  - info about the message
**
**  Returns:
**
**  Description:
**      Handles the messages that orginated from the Watch Expression
**      List box.
**
*/

VOID WatchListHandler( HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    HWND hList = ITEM(ID_WATCH_CUREXPRESSION);
    BOOL Enabled;

    switch (HIWORD(wParam)) {

        case LBN_SETFOCUS:
            EnableWindow(ITEM(ID_WATCH_ADD), FALSE);

            SendMessage((HWND) lParam, LB_SETCURSEL, WatchSelection, 0L);
            Enabled = (SendMessage((HWND) lParam, LB_GETCURSEL, 0, 0L) != LB_ERR);
            EnableWindow(ITEM(ID_WATCH_DELETE), Enabled);
            if (Enabled){
                WatchDefPushButton(hDlg, ID_WATCH_DELETE);

                // KLUGE to stop Windows making the OK button
                // default when coming from buttons
                PostMessage(ITEM(IDOK),BM_SETSTYLE, (WORD)BS_PUSHBUTTON, TRUE);
                PostMessage(ITEM(ID_WATCH_DELETE),BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
            }
            break;

        case LBN_DBLCLK:
            DeleteWatchExpression(hDlg);
            break;

        case LBN_SELCHANGE:
            WatchSelection = (WORD)SendMessage(hList, LB_GETCURSEL, 0, 0L);
            EnableWindow(ITEM(ID_WATCH_DELETE), (WatchSelection != -1));
            if ((WatchSelection != -1)){
                WatchDefPushButton(hDlg, ID_WATCH_DELETE);
            }
            break;
    }
    return;
}

/***    OkButtonHandler
**
**  Synopsis:
**      OkButtonHandler(hDlg, wParam, lParam)
**
**  Entry:
**      hDlg    - Handle to dialog for which to process the message
**      wParam  - info about the message
**      lParam  - info about the message
**
**  Returns:
**
**  Description:
**      Handles the messages that orginated from the Watch Expression
**      buttons.
**
*/

VOID OkButtonHandler( HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    // Important. The tests are done in this order to get around a
    // bug in Windows whereby more than button can be set as the
    // default.  In our case this always the OK button that can be
    // set by mistake so we test this case last.

    if ((HWND)LOWORD(lParam) == ITEM(IDOK)) {
        OKWatch();
        EndDialog(hDlg, TRUE);
    }

    else if ( ISITEMDEFAULT(ID_WATCH_ADD) ) {
        AddWatchExpression(hDlg);
    }

    else if ( ISITEMDEFAULT(ID_WATCH_DELETE) ) {
        DeleteWatchExpression(hDlg);
    }

    else if ( ISITEMDEFAULT(IDOK) ) {
        OKWatch();
        EndDialog(hDlg, TRUE);
    }

    return;
}


