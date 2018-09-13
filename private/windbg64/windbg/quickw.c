/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    QUICKW.C

Abstract:

    This file contains the code for dealing with the Quickwatch Dialog box

Author:

    Bill Heaton (v-willhe)
    Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"


/*
 *  Global Memory (PROGRAM)
 */

extern CXF      CxfIp;

/*
 *  Global Memory (FILE)
 */

PTRVIT pvitQuick;
HWND   hWndQuick;
char   WatchVar[MAX_USER_LINE + 1];
PSTR   pWatchVar;

PTRVIT pVit = NULL;
PTRVIB pVib = NULL;



/*
 *  Function Prototypes
 */

INT_PTR CALLBACK DlgQuickInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgQuickCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID UpdateQuickWatch(PPANE p, WPARAM wParam);
PTRVIT InitQuickVit(void);

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
**      Processes messages for "QUICKWATCH" dialog box
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
DlgQuickW(
          HWND hDlg,
          UINT message,
          WPARAM wParam,
          LPARAM lParam
          )
{

    static DWORD HelpArray[]=
    {
       ID_QUICKW_MODIFY, IDH_QWEXPR,
       ID_QUICKW_ZOOM, IDH_EVALUATE,
       ID_QUICKW_ADD, IDH_ADDWATCH,
       ID_QUICKW_REM_LAST, IDH_REMOVELAST,
       ID_QUICKW_LIST, IDH_PANE,
       0, 0
    };

    Unused(lParam);

    switch (message) {

    case WM_INITDIALOG:
        pVit = NULL;
        return ( DlgQuickInit( hDlg, message, wParam, lParam) );

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
           (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
           (DWORD_PTR)(LPVOID) HelpArray );
        return TRUE;

    case WM_COMMAND:
        return ( DlgQuickCommand( hDlg, message, wParam, lParam) );

    case WM_SYSCOMMAND:

        if (wParam == SC_CLOSE){
            EndDialog(hDlg, FALSE);
            return TRUE;
        }
    }
    return (FALSE);

}   /* DlgQuickW() */



INT_PTR
FAR PASCAL EXPORT
DlgQuickInit(
             HWND hDlg,
             UINT message,
             WPARAM wParam,
             LPARAM lParam
             )
{
    BOOL lookAround = TRUE;

    // First, is there something to try and watch

    *WatchVar = '\0';

    if (Views[curView].Doc > -1) {
        if (hwndActiveEdit && GetCurrentText(curView,
            &lookAround, (LPSTR)WatchVar,
            MAX_USER_LINE, NULL, NULL) ) {

            // Is what we got interesting?
            pWatchVar = WatchVar;

            while (whitespace(*pWatchVar)) {
                pWatchVar++;
            }

            if (strlen(pWatchVar) > 0) {
                SetDlgItemText( hDlg, ID_QUICKW_MODIFY, pWatchVar);
                AddCVWatch( pvitQuick, pWatchVar);
                SendDlgItemMessage( hDlg, ID_QUICKW_LIST, WU_UPDATE, 0, 0);
            }

        }
    } else {
        SetDlgItemText( hDlg, ID_QUICKW_MODIFY, pWatchVar);
        SetFocus (GetDlgItem (hDlg,ID_QUICKW_MODIFY));
    }

    EnableWindow (GetDlgItem (hDlg, ID_QUICKW_REM_LAST), FALSE);

    return (TRUE);
}


INT_PTR
FAR PASCAL EXPORT
DlgQuickCommand(
                HWND hDlg,
                UINT message,
                WPARAM wParam,
                LPARAM lParam
                )
{
    switch (wParam) {

    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SETSEL,0,-1);
            SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SCROLLCARET,0,-1);
            SetFocus (GetDlgItem (hDlg,ID_QUICKW_MODIFY));
            return(FALSE);
        }
        break;

    case ID_QUICKW_MODIFY:
        SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SETSEL,0,-1);
        SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SCROLLCARET,0,-1);
        SetFocus (GetDlgItem (hDlg,ID_QUICKW_MODIFY));
        break;


    case ID_QUICKW_ZOOM:
        if (pvitQuick == NULL) {
            return(FALSE);
        }

        pWatchVar = WatchVar;
        GetDlgItemText( hDlg, ID_QUICKW_MODIFY, pWatchVar, MAX_USER_LINE);

        while (whitespace(*pWatchVar)) {
            pWatchVar++;
        }

        if (strlen(pWatchVar) == 0) {
            return(TRUE);
        }

        if ( pvitQuick->cln == 0) {
            pVib = AddCVWatch( pvitQuick, pWatchVar);
        } else {
            ReplaceCVWatch( pvitQuick, pvitQuick->pvibChild, pWatchVar);
        }

        SendDlgItemMessage( hDlg, ID_QUICKW_LIST, WU_UPDATE, 0, 0);
        SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SETSEL,0,-1);
        SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SCROLLCARET,0,-1);
        SetFocus (GetDlgItem (hDlg,ID_QUICKW_MODIFY));
        break;

    case ID_QUICKW_ADD:

        pWatchVar = WatchVar;
        GetDlgItemText( hDlg, ID_QUICKW_MODIFY, pWatchVar, MAX_USER_LINE);

        if (!FTAddWatchVariable(&pVit, &pVib, pWatchVar)) {
            return TRUE;
        }

        if (GetWatchHWND() == NULL) {
            SendMessage(hwndFrame, WM_COMMAND, IDM_VIEW_WATCH, 0L);
        } else {
            PostMessage( GetWatchHWND(), WU_UPDATE, 0, 0L);
        }

        SendDlgItemMessage( hDlg, ID_QUICKW_LIST, WU_UPDATE, 0, 0);
        SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SETSEL,0,-1);
        SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SCROLLCARET,0,-1);

        EnableWindow (GetDlgItem (hDlg, ID_QUICKW_REM_LAST), TRUE);

        SetFocus (GetDlgItem (hDlg,ID_QUICKW_MODIFY));

        break;

    case ID_QUICKW_REM_LAST:
        if ( pVit != NULL) {
            DeleteCVWatch( pVit, pVib);
            pVit = (PTRVIT) NULL;
            PostMessage( GetWatchHWND(), WU_UPDATE, (WPARAM) TRUE, 0L);
            SendDlgItemMessage( hDlg, ID_QUICKW_LIST, WU_DBG_UNLOADEE, 0, 0);
            SendDlgItemMessage( hDlg, ID_QUICKW_LIST, WU_UPDATE, 0, 0);
            Dbg(InitQuickVit());

            pWatchVar = WatchVar;
            GetDlgItemText( hDlg, ID_QUICKW_MODIFY, pWatchVar, MAX_USER_LINE);
            while (whitespace(*pWatchVar)) {
                pWatchVar++;
            }

            if (strlen(pWatchVar) == 0) {
                return(TRUE);
            }

            if ( pvitQuick->cln == 0) {
                pVib = AddCVWatch( pvitQuick, pWatchVar);
            } else {
                ReplaceCVWatch( pvitQuick, pvitQuick->pvibChild, pWatchVar);
            }

            SendDlgItemMessage( hDlg, ID_QUICKW_LIST, WU_UPDATE, 0, 0);

            SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SETSEL,0,-1);
            SendDlgItemMessage (hDlg,ID_QUICKW_MODIFY,EM_SCROLLCARET,0,-1);

            EnableWindow (GetDlgItem (hDlg, ID_QUICKW_REM_LAST), FALSE);
            SetFocus (GetDlgItem (hDlg,ID_QUICKW_MODIFY));
        }
        break;


    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        return TRUE;

    case IDHELP:
       WinHelp( hDlg, "windbg.hlp", HELP_CONTEXT, IDH_QUICKWATCH );
       break;

    }
    return(FALSE);
}


/***    InitQuickVit
**
**  Synopsis:
**      pVit = InitQuickVit()
**
**  Entry:
**      None
**
**  Returns:
**     Returns a pointer to the current vit (Allocating one if needed).
**     return NULL if can't
**
**  Description:
**
**     Creates the Vit (Variable Information Top) block for the
**     watch window
**
*/

PTRVIT
PASCAL
InitQuickVit()
{
    if (pvitQuick == NULL) {
        pvitQuick = (PTRVIT) calloc(1, sizeof(VIT));
    }
    return (pvitQuick);
}                                       /* InitWatchVit() */


/***    QuickEditProc
**
**  Synopsis:
**      long = QuickEditProc(hwnd, msg, wParam, lParam)
**
**  Entry:
**      hwnd    - handle to window to process message for
**      msg     - message to be processed
**      wParam  - information about message to be processed
**      lParam  - information about message to be processed
**
**  Returns:
**
**  Description:
**      MDI function to handle Watch window messages
**
*/

LRESULT
CALLBACK
QuickEditProc(
              HWND hwnd,
              UINT msg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    HCURSOR hOldCursor = 0;

    PPANE     p     = (PPANE)lParam;
    PPANEINFO pInfo = (PPANEINFO)wParam;
    PTRVIB    pVib  = NULL;

    switch (msg) {

    case WU_INITDEBUGWIN:

        Dbg(InitQuickVit());
        hWndQuick = hwnd;
        UpdateQuickWatch(p, wParam);
        break;

    case WM_DESTROY:

        hWndQuick = NULL;       //  Lose the Watch Window Handle
        // No Break Intended

    case WU_DBG_UNLOADEE:

        if (pvitQuick != NULL) {
            // Lose the Watch Tree
            if ( pvitQuick->pvibChild ) {
                FTFreeAllSib(pvitQuick->pvibChild);
                pvitQuick->pvibChild = NULL;
                free(pvitQuick);
                pvitQuick=NULL;
            }

        }
        break;

    case WU_INFO:
        pInfo->NewText  = FALSE;
        pInfo->ReadOnly = TRUE;
        pInfo->pFormat  = NULL;

        pVib = FTvibGetAtLine( pvitQuick, pInfo->ItemId);
        if ( pVib == NULL) return(FALSE);

        pInfo->pBuffer  = FTGetPanelString( pvitQuick, pVib, pInfo->CtrlId);
        return(TRUE);

    case WU_EXPANDWATCH:

        if ( FTExpand(pvitQuick, (ULONG)(wParam)) == OK) {
            UpdateQuickWatch(p, wParam);   // Watch Count changed
        }
        break;

    case WU_UPDATE:
        UpdateQuickWatch(p, wParam);
        break;
    }

    return 0L;
}                                       /* WatchEditProc() */


VOID
UpdateQuickWatch(
                 PPANE p,
                 WPARAM wParam
                 )
{
    LONG    Len = 0;
    LONG    lLen = 0;
    RECT    Rect, tRect;
    HWND      hFoc;
    HCURSOR     hOldCursor, hWaitCursor;

    // Set hourglass cursor
    hWaitCursor = LoadCursor (NULL, IDC_WAIT);
    hOldCursor = SetCursor (hWaitCursor);

    hFoc = GetFocus();

    if ( pvitQuick == NULL ) {
        InvalidateRect(p->hWndLeft, NULL, TRUE);
        InvalidateRect(p->hWndButton, NULL, TRUE);
        InvalidateRect(p->hWndRight, NULL, TRUE);
        // Set original cursor
        hOldCursor = SetCursor (hOldCursor);
        return;
    }
    FTVerify(&CxfIp, pvitQuick->pvibChild);
    pvitQuick->cxf = CxfIp;

    Len = (LONG)pvitQuick->cln;

    lLen = (LONG)SendMessage(p->hWndLeft, LB_GETCOUNT, 0, 0L);
    if ((lLen < Len) || (lLen == 0)) {
        SendMessage(p->hWndLeft, LB_SETCOUNT, Len, 0);
        SendMessage(p->hWndButton, LB_SETCOUNT, Len, 0);
        SendMessage(p->hWndRight, LB_SETCOUNT, Len, 0);
    } else {
        SendMessage(p->hWndLeft, LB_SETCOUNT, (WPARAM) ((int)lLen), 0L);
        SendMessage(p->hWndButton, LB_SETCOUNT, (WPARAM) ((int)lLen), 0L);
        SendMessage(p->hWndRight, LB_SETCOUNT, (WPARAM) ((int)lLen), 0L);
    }


    p->MaxIdx = (WORD)Len;

    //  Reseting the count, lost where we were so put us back
    PaneResetIdx(p, p->CurIdx);

    PaneCaretNum(p);


    if ((hFoc == p->hWndButton) || (hFoc == p->hWndLeft) || (hFoc == p->hWndRight)) {
        SendMessage(p->hWndButton , LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
        GetClientRect (p->hWndButton, &tRect);
        tRect.top = Rect.top;
        InvalidateRect(p->hWndButton, &tRect, TRUE);

        SendMessage(p->hWndLeft , LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
        GetClientRect (p->hWndLeft, &tRect);
        tRect.top = Rect.top;
        InvalidateRect(p->hWndLeft, &tRect, TRUE);

        SendMessage(p->hWndRight , LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
        GetClientRect (p->hWndRight, &tRect);
        tRect.top = Rect.top;
        InvalidateRect(p->hWndRight, &tRect, TRUE);
    }

    // Set original cursor
    hOldCursor = SetCursor (hOldCursor);

    CheckPaneScrollBar( p, (WORD)Len);
}

