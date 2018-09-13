/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

     paneuser.c

Abstract:

     This module contains the code for handling the keyboard and mouse
     for the panel manager windows.

Author:

     William J. Heaton (v-willhe) 25-Nov-1992
     Griffith Wm. Kadnier (v-griffk) 10-Mar-1993

Environment:

     Win32, User Mode

--*/
#include "precomp.h"
#pragma hdrstop

void PaneClearEdit( PPANE p);
void PaneDeleteChar( PPANE p, SHORT Idx);
void PaneEditMode( PPANE p);
void PaneInsertChar( PPANE p, CHAR wParam );
void PaneCopyClipBoard( PPANE p );
void PanePasteClipBoard( PPANE p );
void PaneSetPos( PPANE p, SHORT NewPos);
void PaneSetPosXY( HWND hWnd, int X, int Y, BOOL Select);
void PaneSelectWord( PPANE p );
void PaneCutSelection( PPANE p );


extern LRESULT SendMessageNZ (HWND,UINT,WPARAM,LPARAM);
extern void CheckHorizontalScroll (PPANE p);

BOOL inMouseMove=FALSE;

/***  PaneKeyboardHandler
**
**  Synopsis:
**      VOID PaneKeyboardHandler( hWnd, msg, wParam, lParam)
**
**  Entry:
**      Standard WNDPROC
**
**  Returns:
**      Standard WNDPROC
**
**  Description:
**      The Standard Keyboard Handler for all of the Panemanager Panes.
**
*/

#define PAGE (p->PaneLines-1)

void
PaneKeyboardHandler(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PPANE p = (PPANE)GetWindowLongPtr(GetParent(hWnd), GWW_EDIT );
    BOOL  isShiftDown;
    BOOL  isCtrlDown;


    if (IsIconic(GetParent(hWnd))) {
        return;
    }

    if ((message == WM_KEYDOWN) && ((wParam == VK_CONTROL) || (wParam == VK_SHIFT))) {
        return;   // don't care if it is just the ctrl/shift key
    }

    isShiftDown = (GetKeyState(VK_SHIFT) < 0);
    isCtrlDown = (GetKeyState(VK_CONTROL) < 0);

    switch (message) {

    case WM_COPY:
        PaneCopyClipBoard( p );
        break;

    case WM_PASTE:
        PanePasteClipBoard( p );
        break;

    case WM_MOUSEMOVE:
        if ( wParam & MK_LBUTTON ) {
            inMouseMove = TRUE;
            PaneSetPosXY( hWnd, LOWORD(lParam), HIWORD(lParam), TRUE );
            inMouseMove = FALSE;
        }
        break;

    case WM_LBUTTONDOWN:
        PaneSetPosXY( hWnd, LOWORD(lParam), HIWORD(lParam), FALSE );
        break;

    case WM_LBUTTONDBLCLK:
        if ( p->hWndFocus != p->hWndButton ) {
            PaneSelectWord( p );
        }
        break;

    case WM_RBUTTONDOWN:
        if (p->SelLen) {
            PaneCopyClipBoard( p );
        } else {
            PanePasteClipBoard( p );
        }
        return;

    case WM_CHAR:
        switch(wParam) {

        // Handled by WM_KEYDOWN
        case TAB:
        case CTRL_H:
        case CTRL_M:
        case ESCAPE:
            break;

        case CTRL_C:
            PostMessage(hwndFrame, WM_COMMAND, MAKEWPARAM(IDM_EDIT_COPY, 1), 0);
            break;

        case CTRL_V:
            PostMessage(hwndFrame, WM_COMMAND, MAKEWPARAM(IDM_EDIT_PASTE, 1), 0);
            break;

        case CTRL_X:
            PostMessage(hwndFrame, WM_COMMAND, MAKEWPARAM(IDM_EDIT_CUT, 1), 0);
            break;

        default:
            if ( p->hWndFocus != p->hWndButton && wParam >= ' ') {
                PaneInsertChar(p, (CHAR)wParam );
            } else {
                MessageBeep(0);
            }
        }
        break;

        case WM_KEYDOWN:

            if (hWnd != p->hWndButton) {
                MSG msg;

                // if text has been highlighted, a cut may be necessary
                // in preparing for an insert

                if (p->SelLen != 0 &&
                    PeekMessage(&msg, hWnd, WM_KEYDOWN, WM_CHAR, PM_NOREMOVE)) {
                    if (msg.message == WM_CHAR) {
                        switch (msg.wParam) {
                            // there will not be any insert for these four cases
                        case TAB:
                        case CTRL_C:
                        case CTRL_H:
                        case CTRL_M:
                        case ESCAPE:
                            break;

                        default:
                            if (msg.wParam >= ' ') {
                                PaneCutSelection(p);
                                p->SelLen = 0;
                                return;
                            }
                        }
                    }
                }

                // removes any highlighting if necessary
                // except the following key combinations:
                // DELETE, BACKSPACE
                // Ctrl+Insert
                // Shift+(Ctrl)+Left/Right Arrow
                // Shift+Home/End Key

                if (wParam != VK_DELETE &&
                    wParam != VK_BACK &&
                    !(isCtrlDown && ('C' == wParam || VK_INSERT == wParam)) &&
                    !(isShiftDown &&
                    (wParam == VK_LEFT || wParam == VK_RIGHT ||
                    (!isCtrlDown && (wParam == VK_HOME || wParam == VK_END)))
                    )
                    ) {
                    POINT cPos;
                    GetCaretPos (&cPos);
                    PaneSetPosXY( hWnd, (WORD)cPos.x, (WORD)cPos.y, FALSE);
                }
            }

            switch ( wParam ) {

            case VK_DELETE:
            case VK_BACK:
                if (p->SelLen != 0) {
                    PaneCutSelection(p);
                    p->SelLen = 0;
                } else if (wParam == VK_BACK) {
                    PaneDeleteChar(p, (SHORT)(p->CurPos-1));  // backspace
                } else {
                    PaneDeleteChar(p, (SHORT)p->CurPos);
                }
                break;

            case VK_LEFT:
            case VK_RIGHT:
                if (isShiftDown) {
                    POINT cPos;
                    if (p->SelLen == 0) {
                        p->SelPos = p->CurPos;
                    }
                    PaneSetPos(p,(SHORT)(p->CurPos+((wParam == VK_LEFT)?-1:1)));
                    GetCaretPos (&cPos);
                    PaneSetPosXY( hWnd, (WORD)cPos.x, (WORD)cPos.y, TRUE );
                } else {
                    PaneSetPos(p,(SHORT)(p->CurPos+((wParam == VK_LEFT)?-1:1)));
                }
                break;

            case VK_UP:
            case VK_DOWN:
                PaneInvalidateCurrent(p->hWndFocus, p, -1);
                PaneSetIdx(p,(SHORT)(p->CurIdx + ((wParam == VK_UP) ?-1:1)));
                if (isShiftDown) {
                    POINT cPos;
                    GetCaretPos (&cPos);
                    PaneSetPosXY( hWnd, (WORD)cPos.x, (WORD)cPos.y, FALSE);
                }
                p->SelLen = 0;
                p->SelPos = 0;
                PaneCaretNum(p);
                break;

            case VK_PRIOR:
            case VK_NEXT:
                PaneSetIdx(p,(SHORT)(p->CurIdx+((wParam == VK_PRIOR) ?-PAGE:PAGE)));
                if (isShiftDown) {
                    POINT cPos;
                    GetCaretPos (&cPos);
                    PaneSetPosXY( hWnd, (WORD)cPos.x, (WORD)cPos.y, FALSE );
                }
                p->SelLen = 0;
                p->SelPos = 0;
                PaneCaretNum(p);
                break;

            case VK_HOME:
            case VK_END:
                {
                    SHORT   tmp;
                    POINT   cPos;

                    tmp = (wParam == VK_HOME) ? 0 : 0x7fff; // first or last
                    if (isCtrlDown) {
                        PaneSetIdx(p, (SHORT)tmp);
                    } else if (isShiftDown) {
                        if (p->SelLen == 0) {
                            p->SelPos = p->CurPos;
                        }
                        PaneSetPos(p, (SHORT)tmp);
                        GetCaretPos (&cPos);
                        PaneSetPosXY( hWnd, (WORD)cPos.x, (WORD)cPos.y, TRUE);
                    } else {
                        PaneSetPos(p, (SHORT)tmp);
                        p->SelLen = p->SelPos = 0; // why?
                    }
                    PaneCaretNum(p);
                }
                break;

            case VK_TAB:
                PaneSwitchFocus(p, NULL, (BOOL)(((GetKeyState (VK_SHIFT) >= 0)) ? FALSE : TRUE));
                break;

            case VK_INSERT:
                if (isShiftDown) {
                    PanePasteClipBoard( p );
                } else if (isCtrlDown) {
                    PaneCopyClipBoard( p );
                } else {
                    PaneEditMode(p);
                }
                break;

            case VK_ESCAPE:
                PaneClearEdit(p);
                break;

            case VK_RETURN:
                if (p->nCtrlId == ID_PANE_BUTTON) {
                    (*p->fnEditProc)(p->hWndFocus, WU_EXPANDWATCH, (WPARAM)p->CurIdx, (LPARAM)p);
                    CheckHorizontalScroll (p);
                } else {
                    PaneCloseEdit(p);
                }
                break;

            default:
                break;
        }
        break;

    default:
        DAssert(FALSE);
    }
}                               /* PaneKeyboardHandler() */


 /***  PaneCopyClipBoard
 **
 **  Synopsis:
 **      void PaneCopyClipBoard( PPANE p);
 **
 **  Entry:
 **      p      - pointer to Pane Information Strucure
 **
 **  Returns:
 **      None
 **
 **  Description:
 **      Copy the current selection (If any to the clipboard)
 **
 */

void
PaneCopyClipBoard(
                  PPANE p
                  )
{

    PSTR        pBuf = NULL;
    int         nLen = 0;
    PANEINFO    Info = {0,0,0,0,NULL};
    HANDLE      hData;
    LPSTR       pData;
    LPSTR       pSrc;

    if ( p->SelLen != 0 ) {

        if ( p->Edit ) {

            pBuf = p->EditBuf;

        } else {

            Info.CtrlId = p->nCtrlId;
            Info.ItemId = p->CurIdx;
            (PSTR)(*p->fnEditProc)(p->hWndFocus,WU_INFO,(WPARAM)&Info,(LPARAM)p);
            pBuf = Info.pBuffer;
        }

        if ( p->SelLen > 0 ) {
            pSrc = pBuf + p->SelPos;
            nLen = p->SelLen;
        } else {
            pSrc = pBuf + p->SelPos + p->SelLen;
            nLen = -(p->SelLen);
        }

        Dbg(hData = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE, nLen + 1));
        if ( hData ) {

            Dbg((pData = (PSTR) GlobalLock( hData )) != NULL);
            if ( pData ) {

                while ( nLen-- ) {
                    *pData++ = *pSrc++;
                }
                *pData = '\0';

                DbgX(GlobalUnlock(hData) == 0);

                if (OpenClipboard (hwndFrame)) {
                    EmptyClipboard();
                    SetClipboardData(CF_TEXT, hData);
                    CloseClipboard();
                    p->SelLen = 0;
                }
            }

        }
    }
}   /* PaneCopyClipBoard() */


 /***  PanePasteClipBoard
 **
 **  Synopsis:
 **      void PanePasteClipBoard( PPANE p);
 **
 **  Entry:
 **      p      - pointer to Pane Information Strucure
 **
 **  Returns:
 **      None
 **
 **  Description:
 **      Copy the clipboard to the current pane item
 **
 */

void
PanePasteClipBoard(
                   PPANE p
                   )
{

    HANDLE    hData;
    size_t    size;
    LPSTR     p1;
    LPSTR     pData;

    if ( !p->ReadOnly ) {

        if (OpenClipboard(hwndFrame)) {

            hData = GetClipboardData(CF_TEXT);

            if (hData && (size = GlobalSize (hData))) {

                if (size >= MAX_CLIPBOARD_SIZE) {

                    ErrorBox(ERR_Clipboard_Overflow);

                } else if ( pData = (PSTR) GlobalLock(hData) ) {

                    p1 = pData;
                    while (size && *p1) {
                        size--;
                        if (*p1 == '\r' || *p1 == '\n') {
                            break;
                        }

                        PaneInsertChar(p, (CHAR)*p1 );

                        p1++;
                    }
                    DbgX(GlobalUnlock (hData) == FALSE);
                }
                CloseClipboard();
            }
        }
    }
}   /* PanePasteClipBoard() */



/***  PaneCutSelection
**
**  Synopsis:
**      void PaneCutSelection( PPANE p);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**
**  Returns:
**      None
**
**  Description:
**      Cut the current selection (delete)
**
*/

void
PaneCutSelection(
                 PPANE p
                 )
{

    PSTR        pBuf = NULL;
    int         nLen = 0;
    PANEINFO    Info = {0,0,0,0,NULL};
    SHORT       Indx;

    if ( p->SelLen != 0 ) {

        if ( p->Edit ) {
            pBuf = p->EditBuf;
        } else {
            Info.CtrlId = p->nCtrlId;
            Info.ItemId = p->CurIdx;
            (PSTR)(*p->fnEditProc)(p->hWndFocus,WU_INFO,(WPARAM)&Info,(LPARAM)p);
            pBuf = Info.pBuffer;
        }

        if ( p->SelLen > 0 ) {
            Indx = p->SelPos;
            nLen = p->SelLen;
        } else {
            Indx = p->CurPos;
            nLen = -(p->SelLen);
        }

        while ( nLen-- ) {
            PaneDeleteChar(p, Indx);
        }

    }

}   /* PaneCutSelection() */


/***  PaneSetPos
**
**  Synopsis:
**      void PaneSetPos( PPANE p, SHORT NewPos);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**      NewPos - The index of the position to set on the current pane item
**
**  Returns:
**      None
**
**  Description:
**      This routine is used to set the x coordinate on a pane item
**
*/

void
PaneSetPos(
           PPANE p,
           SHORT NewPosArg
           )
{
    SIZE     Size = { 0, 0 };
    PANEINFO Info = {0,0,0,0,NULL};
    RECT Rect = {0,0,0,0};
    PSTR     pBuf = NULL;
    int      nLen = 0;
    HDC      hDC  = 0;
    int      NewPos = NewPosArg;

    if ( p->nCtrlId == ID_PANE_BUTTON ) {
        InvertButton( p );
    } else {

        //  Figure out which buffer we're using and it's max
        if ( p->Edit ) {
            pBuf = &p->EditBuf[0];
            nLen = p->CurLen;
        } else {
            Info.CtrlId = p->nCtrlId;
            Info.ItemId = p->CurIdx;
            (PSTR)(*p->fnEditProc)(p->hWndFocus,WU_INFO,(WPARAM)&Info,(LPARAM)p);
            pBuf = Info.pBuffer;
            if ( pBuf ) {
                nLen = strlen(pBuf);
            }
        }

        if(NewPos > p->CurPos) { /* move right */
            if(IsDBCSLeadByte((BYTE)*(pBuf + p->CurPos))) {
                NewPos++;
            }
        }
        else if(NewPos < p->CurPos) { /* move left */
            if(IsDBCSLeadByte((BYTE)(*CharPrev(pBuf, pBuf + p->CurPos)))) {
                NewPos--;
            }
        }

        //  If the New position is out of range, put it back in range
        if ( NewPos < 0 ) {
            NewPos = 0;
        }
        if ( NewPos > nLen ) {
            NewPos = nLen;
        }
        p->CurPos = (WORD) NewPos;

        //  Figure out the offset to the new caret
        if ( NewPos <= 0) {
            Size.cx = 0;
        } else {
            hDC = GetDC(p->hWndFocus);
            SelectObject(hDC, p->hFont);
            GetTextExtentPoint(hDC, pBuf, NewPos, &Size);
            ReleaseDC(p->hWndFocus,hDC);
        }

        PaneSetCaret(p, Size.cx, TRUE);
    }

    //  Set the Status Bar
    SetLineColumn_StatusBar(p->CurIdx+1, p->CurPos+1);

}  /* PaneSetPos() */


/***  PaneSetPosXY
**
**  Synopsis:
**      void PaneSetPosXY( HWND hWnd, int X, int Y, BOOL Select)
**
**  Entry:
**      hWnd   - The Window
**      x      - The X coordinate
**      y      - The Y coordinate
**      Select -
**
**  Returns:
**      None
**
**  Description:
**
**
*/

void
PaneSetPosXY(
             HWND hWnd,
             int X,
             int Y,
             BOOL Select
             )
{
    PPANE       p = (PPANE)GetWindowLongPtr(GetParent(hWnd), GWW_EDIT );
    SHORT       NewIdx;
    SHORT       NewPos;
    PSTR        pBuf = NULL;
    int         nLen = 0;
    int         Offset = 0;
    PANEINFO    Info = {0,0,0,0,NULL};
    HDC         hDC  = 0;
    BOOL        isShiftDown = (GetKeyState(VK_SHIFT) < 0);
    SIZE        Size;
    POINT       point;

    //
    //  Calculate what index the mouse hit was on, If we're selecting
    //  we ignore anything off our pane and/or index
    //

    if ( hWnd != p->hWndButton) {
        HideCaret(hWnd);
    }

    NewIdx = p->TopIdx + Y/p->LineHeight;
    if ( Select ) {
        if ( hWnd != p->hWndFocus || NewIdx != p->CurIdx ) {
            if ( hWnd != p->hWndButton) {
                ShowCaret(hWnd);
            }
            return;
        }
    }

    //
    //  Dialog boxes (quickwatch) can't select
    //

    if (( p->Type == QUICKW_WIN) || (!isShiftDown && !inMouseMove)) {
        Select = FALSE;
    }



    //
    //  Switch pane if necessary
    //

    if ( hWnd != p->hWndFocus ) {
        PaneSwitchFocus(p, hWnd, FALSE);
        Select      = FALSE;

        // Buttons are special, Expand/Contract the item and get out
        if ( hWnd == p->hWndButton) {
            p->CurIdx = NewIdx;
            (*p->fnEditProc)(p->hWndFocus, WU_EXPANDWATCH, (WPARAM)NewIdx, (LPARAM)p);
            CheckHorizontalScroll (p);
            return;
        }
    }



    if ( NewIdx < p->MaxIdx ) {

        //
        //  Calculate new position
        //
        if ( NewIdx == p->CurIdx && p->Edit ) {

            pBuf = &p->EditBuf[0];
            nLen = p->CurLen;

        } else {

            Info.CtrlId = p->nCtrlId;
            Info.ItemId = NewIdx;

            (PSTR)(*p->fnEditProc)( p->hWndFocus,
                WU_INFO,
                (WPARAM)&Info,
                (LPARAM)p);

            if ( pBuf = Info.pBuffer ) {
                nLen = strlen(pBuf);
            }
        }

        if ( p->nCtrlId == ID_PANE_BUTTON) {
            (*p->fnEditProc)(p->hWndFocus,
                WU_EXPANDWATCH,
                (WPARAM)NewIdx,
                (LPARAM)p);


            if ( p->ScrollBarUp ) {
                SetScrollRange(p->hWndScroll, SB_CTL, 0, p->MaxIdx - 1, FALSE);
                SetScrollPos( p->hWndScroll, SB_CTL, (INT)p->CurIdx, FALSE);
            }
            ShowScrollBar( p->hWndScroll, SB_CTL, p->ScrollBarUp);

            if (!p->ScrollBarUp) {
                p->TopIdx = 0;   //reset top if no scrolling
            }

            SyncPanes(p,(WORD)-1);

            CheckHorizontalScroll (p);
        }

        if ( pBuf ) {

            NewPos  = -1;

            if (p->nCtrlId == ID_PANE_LEFT) {
                Offset = p->nXoffLeft;
            } else if (p->nCtrlId == ID_PANE_RIGHT) {
                Offset = p->nXoffRight;
            }

            X += Offset;

            hDC = GetDC(p->hWndFocus);
            SelectObject(hDC, p->hFont);
            GetTextExtentPoint(hDC, pBuf, 1, &Size);
            ReleaseDC(p->hWndFocus,hDC);


            if (X < Size.cx) {
                NewPos = 0;
            } else {
                hDC = GetDC(p->hWndFocus);
                SelectObject(hDC, p->hFont);
                do {
                    if(NewPos >=0 && IsDBCSLeadByte((BYTE)*(pBuf+NewPos)))
                        NewPos++;
                    NewPos++;
                    GetTextExtentPoint(hDC, pBuf, NewPos, &Size);
                } while (Size.cx < X);
                ReleaseDC(p->hWndFocus,hDC);
            }

            if ( NewPos > nLen ) {
                NewPos = (SHORT) nLen;
            }

            if ( p->CurIdx != NewIdx ) {
                Select    = FALSE;
                p->SelLen = 0;
                PaneInvalidateCurrent( p->hWndFocus, p, -1);
                PaneSetIdx( p, NewIdx );
            }

            if ( Select ) {
                p->SelLen = NewPos - p->SelPos;
            } else {
                p->SelPos = NewPos;
                p->SelLen = 0;
            }
            p->CurPos = NewPos;

            PaneSetPos( p, NewPos );
            PaneInvalidateCurrent( p->hWndFocus, p, -1);
        } else {
            NewPos  = 0;

            PaneInvalidateCurrent( p->hWndFocus, p, -1);

            PaneSetPos( p, NewPos );
            PaneSetIdx( p, NewIdx );
        }
    }

    if ( hWnd != p->hWndButton) {
        GetCaretPos(&point);
        p->X = point.x;
        p->Y = point.y;
        ShowCaret(hWnd);
    }

}   /* PaneSetPosXY() */


/***  PaneSelectWord
**
**  Synopsis:
**      void PaneSelectWord( PPANE p );
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**
**  Returns:
**      None
**
**  Description:
**      Selects the word around the current selection point
**
*/

void
PaneSelectWord(
               PPANE p
               )
{
    WORD            Start = p->CurPos;
    WORD            End   = p->CurPos;
    PSTR            pBuf = NULL;
    int             nLen = 0;
    PANEINFO        Info = {0,0,0,0,NULL};

    if ( p->Edit ) {

        pBuf = p->EditBuf;
        nLen = p->CurLen;

    } else {

        Info.CtrlId = p->nCtrlId;
        Info.ItemId = p->CurIdx;
        (PSTR)(*p->fnEditProc)(p->hWndFocus,WU_INFO,(WPARAM)&Info,(LPARAM)p);
        pBuf = Info.pBuffer;
        if ( pBuf ) {
            nLen = strlen(pBuf);
        }
    }

    if ( pBuf && pBuf[Start] != ' ' && pBuf[Start] != '\t' ) {

        if(!IsDBCSLeadByte((BYTE)pBuf[Start])) {
            while ( Start > 0 && pBuf[Start] != ' ' && pBuf[Start] != '\t' ) {
                Start--;
            }

            if ( pBuf[Start] == ' ' || pBuf[Start] == '\t' ) {
                Start++;
            }
        }

        while ( End < nLen
            && pBuf[End]
            && pBuf[End] != ' '
            && pBuf[End] != '\t' ) {
            if(IsDBCSLeadByte((BYTE)pBuf[End])) {
                End++;
                break;
            }
            End++;
        }

        if ( pBuf[End] == ' ' || pBuf[End] == '\t' || !pBuf[End] ) {
            End--;
        }

        p->SelPos = Start;
        p->SelLen = End - Start + 1;

        PaneInvalidateCurrent( p->hWndFocus, p, -1);
    }
}                                          /* PaneSelectWord */

/***  PaneSetIdx
**
**  Synopsis:
**      void PaneSetIdx( PPANE p, SHORT NewIdx);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**      NewIdx - The index (Line) to set the current pane item to.
**
**  Returns:
**      None
**
**  Description:
**      This routine is used to set the y coordinate on a pane item
**
*/

void
PaneSetIdx(
           PPANE p,
           SHORT NewIdx
           )
{
    //  If we're in edit mode and can't close edit, bail out.
    if ( !PaneCloseEdit(p) ) {
        PaneClearEdit(p);
    }

    PaneResetIdx(p, NewIdx);
}   /* PaneSetIdx */


/***  PaneResetIdx
**
**  Synopsis:
**      void PaneResetIdx( PPANE p, SHORT NewIdx);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**      NewIdx - The index (Line) to set the current pane item to.
**
**  Returns:
**      None
**
**  Description:
**      This routine is used to set the y coordinate on a pane item
**
*/

void
PaneResetIdx(
             PPANE p,
             SHORT NewIdx
             )
{

    PANEINFO Info = {0,0,0,0,NULL};

    if ( p->nCtrlId == ID_PANE_BUTTON ) {
        PaneInvalidateCurrent( p->hWndFocus, p, -1);
    }

    //  If the New index is out of range, put it back in range
    if ( NewIdx < 0 ) {
        NewIdx = 0;
    }
    if ( (WORD)NewIdx > p->MaxIdx-1) {
        NewIdx = (SHORT)p->MaxIdx-1;
    }

    p->CurIdx = (WORD)NewIdx;

    SyncPanes(p, (WORD)-1);

    if ( p->nCtrlId != ID_PANE_BUTTON ) {
        PaneSetPos(p, 0);
    } else {
        InvertButton(p);
    }

    //  Set the Status Bar
    Info.CtrlId = p->nCtrlId;
    Info.ItemId = p->CurIdx;
    (PSTR)(*p->fnEditProc)(p->hWndFocus,WU_INFO,(WPARAM)&Info,(LPARAM)p);

    p->ReadOnly = Info.ReadOnly;
    SetLineColumn_StatusBar(p->CurIdx+1, p->CurPos+1);

}   /* PaneResetIdx */


/***  PaneSetCaret
**
**  Synopsis:
**      void PaneSetCaret( PPANE p, LONG cx, BOOL Scroll);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**      cx     - The pixel address of where the caret should be
**      Scroll - Scroll the pane to show the insertion point
**
**  Returns:
**      None
**
**  Description:
**      Set the caret (if in view) on the current pane item
**
*/

void
PaneSetCaret(
             PPANE p,
             LONG cx,
             BOOL Scroll
             )
{
    RECT Rect = {0,0,0,0};
    RECT cRect = {0,0,0,0};
    LONG Offset;
    LONG PerCent;
    LONG Max;
    HWND hFoc;
    POINT point;
    double dMax, dCx;


    if (p->CurIdx > p->MaxIdx) {
        return;                      //window is empty or bad indx
    }

    if (p->hWndFocus) {
        SendMessage( p->hWndFocus, LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
        GetClientRect( p->hWndFocus, &cRect);
        Offset = Rect.right - cRect.right;
    } else
        Offset = 0;


    if (p->nCtrlId == ID_PANE_LEFT) {
        p->nXoffLeft = Offset;
    } else if (p->nCtrlId == ID_PANE_RIGHT) {
        p->nXoffRight = Offset;
    }

    p->nCaretPos = cx;

    if ((hFoc = GetFocus ()) == p->hWndFocus) {
        HideCaret (p->hWndFocus);
    }

    if (p->CurPos == 0) {
        PaneInvalidateItem( p->hWndFocus, p, p->CurIdx);
    }


    // Is the caret in range?
    if ((p->nCaretPos < Offset || p->nCaretPos > (Rect.right -1))
        && ((hFoc = GetFocus ()) == p->hWndFocus)) {
        if (!Scroll) {
            ShowCaret(p->hWndFocus);
            GetCaretPos(&point);
            p->X = point.x;
            return;
        } else {

            // Note: Listbox scrollbars are a percentage 1-100
            Max = (long) (SendMessage( p->hWndFocus, LB_GETHORIZONTALEXTENT, 0,0))
                - p->CharWidth;

            DestroyCaret();
            CreateCaret( p->hWndFocus, 0, 3, p->LineHeight);

            SetCaretPos(cx - Offset, Rect.top);
            GetCaretPos(&point);
            p->X = point.x;
            ShowCaret(p->hWndFocus);

            if (Max != 0) {
                dMax = (double) Max;
                dCx = (double) cx;
                PerCent = ((long)((dCx /dMax) * 100.0));
                SendMessage( p->hWndFocus,
                    WM_HSCROLL,
                    MAKELONG(SB_THUMBPOSITION,PerCent),
                    0);
            }

            SendMessage( p->hWndFocus, LB_GETITEMRECT, (WPARAM)p->CurIdx, (LPARAM)&Rect);
            GetClientRect( p->hWndFocus, &cRect);
            Offset = Rect.right - cRect.right;

            if (p->nCtrlId == ID_PANE_LEFT) {
                p->nXoffLeft = Offset;
            } else if (p->nCtrlId == ID_PANE_RIGHT) {
                p->nXoffRight = Offset;
            }

            PaneInvalidateItem( p->hWndFocus, p, p->CurIdx);

            return;
        }
    }

    if ((hFoc = GetFocus ()) == p->hWndFocus) {
        SetCaretPos(cx - Offset, Rect.top);
        GetCaretPos(&point);
        p->X = point.x;
        ShowCaret(p->hWndFocus);
    }
}
/* PaneSetCaret() */



/***  PaneCaretNum
**
**  Synopsis:
**      int PaneCaretNum( PPANE p);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**
**  Returns:
**      Number of Items visible
**
**
*/

int
PaneCaretNum(
             PPANE p
             )
{
    RECT Rect = {0,0,0,0};
    RECT tRect = {0,0,0,0};
    RECT cRect = {0,0,0,0};
    const RECT zeroRect = {0,0,0,0};
    int   nNum = 0, nMin, nMax, nScrollPos;
    HWND  hPane;
    BOOL  fRedo = TRUE;
    LRESULT rst;

    GetScrollRange (p->hWndScroll,SB_CTL,&nMin, &nMax);

    do {
        hPane = (p->hWndFocus != p->hWndButton) ?
            p->hWndFocus
            :p->hWndLeft;
        rst = SendMessageNZ(hPane,
            LB_GETITEMRECT,
            (WPARAM)p->CurIdx,
            (LPARAM)&Rect);
        if (rst == LB_ERR) {
            return 0;
        }
        rst = SendMessageNZ(hPane,
            LB_GETITEMRECT,
            (WPARAM)p->TopIdx,
            (LPARAM)&tRect);
        if (rst == LB_ERR) {
            return 0;
        }

        nScrollPos = GetScrollPos (p->hWndScroll,SB_CTL);

        // Is the caret in range?

        if (hPane != NULL) {

            int nRct;

            GetClientRect(hPane, &cRect);
            nRct = (cRect.bottom - cRect.top);
            if ((Rect.bottom - Rect.top) > nRct) {
                nNum = 0;
            } else if ((Rect.bottom - Rect.top) != 0) {
                nNum = (nRct / (Rect.bottom - Rect.top));
            } else {
                nNum = 0;
            }
        }

        if ((Rect.bottom > cRect.bottom)
            && (p->CurIdx < p->MaxIdx)
            && (nNum > 0)) {

            ScrollPanes (p,
                MAKEWPARAM (SB_THUMBTRACK,(p->CurIdx - PAGE)),
                0);

        } else if ((Rect.top < tRect.top)
            && (p->CurIdx != 0xFFFF)
            && (nNum > 0)) {

            ScrollPanes (p,
                MAKEWPARAM (SB_THUMBTRACK,p->CurIdx),
                0);

        } else {

            fRedo = FALSE;

        }

    } while (fRedo == TRUE);

    return(nNum);

}
/* PaneCaretNum() */




/***  PaneDeleteChar
**
**  Synopsis:
**      void PaneDeleteChar( PPANE p, SHORT Idx);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**      Idx    - The index of the character to delete
**
**  Returns:
**      None
**
**  Description:
**      Deletes the character at the specified index from the current
**      pane item.
**
*/

void
PaneDeleteChar(
               PPANE p,
               SHORT Idx
               )
{
    RECT Rect = {0,0,0,0};
    PANEINFO Info = {0,0,0,0,NULL};
    PSTR     pBuf = NULL;
    int      nLen = 0;

    // Can't delete on a readonly panel (Catchs Button Pane too!)
    if ( p->ReadOnly ) {
        MessageBeep(0);
        return;
    }

    // Get the Buffer Information
    if ( p->Edit ) {
        pBuf = &p->EditBuf[0];
        nLen = p->CurLen;
    }

    else {
        Info.CtrlId = p->nCtrlId;
        Info.ItemId = p->CurIdx;
        (PSTR)(*p->fnEditProc)(p->hWndFocus,WU_INFO,(WPARAM)&Info,(LPARAM)p);
        pBuf = Info.pBuffer;
        if ( pBuf) {
            nLen = strlen(pBuf);
        }
    }


    //  If the New position is out of range, we can't delete
    if ( nLen == 0 || Idx < 0 || Idx > nLen-1) {
        MessageBeep(0);
        return;
    }

    // If no Edit open, Start it up
    if ( !p->Edit ) {
        strcpy(p->EditBuf,pBuf);
        p->CurLen = (WORD) nLen;
        p->Edit = TRUE;
    }

    // Let do the Delete

    if ( Idx < p->CurLen ) {        // If not at end, shift the string over
        memmove( &p->EditBuf[Idx],
            &p->EditBuf[Idx+1], p->CurLen - Idx);
    }
    p->CurLen--;                    // One Less char to worry about
    p->EditBuf[p->CurLen] = 0;      // But make sure it goes away


    PaneInvalidateCurrent( p->hWndFocus, p, Idx);
}   /* PaneDeleteChar() */


/***  PaneInsertChar
**
**  Synopsis:
**      void PaneDeleteChar( PPANE p, CHAR c);
**
**  Entry:
**      p      - pointer to Pane Information Strucure
**      c      - The character to be inserted.
**
**  Returns:
**      None
**
**  Description:
**      Insert the character passed into the buffer at the current
**      insert point.
**
*/

void
PaneInsertChar(
               PPANE p,
               CHAR c
               )
{
    PANEINFO Info = {0,0,0,0,NULL};

    //  Can't insert on Readonly Pane (Catches Buttons too!)
    //  Don't Insert into null at end of buffer
    if ( p->ReadOnly || p->CurPos == EDITMAX-1) {
        MessageBeep(0);
        return;
    }

    // If no Edit open, Start it up
    if ( !p->Edit ) {
        Info.CtrlId = p->nCtrlId;
        Info.ItemId = p->CurIdx;
        (PSTR)(*p->fnEditProc)(p->hWndFocus,WU_INFO,(WPARAM)&Info,(LPARAM)p);
        if ( Info.pBuffer) {
            strcpy(p->EditBuf,Info.pBuffer);
            p->CurLen = strlen(p->EditBuf);
        }
        p->Edit = TRUE;
    }

    // OverStrike Mode
    if ( p->OverType ) {
        if ( p->EditBuf[p->CurPos] == 0) {
            p->CurLen++;
        }
        p->EditBuf[p->CurPos] = c;
    }

    // Insert Mode
    else {

        // Insert at end of line is a no brainer
        if ( p->EditBuf[p->CurPos] == 0) {
            p->CurLen++;
            p->EditBuf[p->CurPos] = c;
        }

        // Insert into the Middle
        else {
            memmove( &p->EditBuf[p->CurPos+1], &p->EditBuf[p->CurPos], p->CurLen - p->CurPos);
            p->EditBuf[p->CurPos] = c;
            p->CurLen++;
        }
    }

    // Repaint the Line
    PaneInvalidateCurrent( p->hWndFocus, p,(SHORT)(p->CurPos+1));
}   /* PaneInsertChar() */


/***  PaneEditMode
**
**  Synopsis:
**      void PaneEditMode( PPANE p)
**
**  Entry:
**      p    - The Pane info for the window
**
**  Returns:
**      None
**
**  Description:
**      Set insert/overstrike mode.
*/

void
PaneEditMode(
             PPANE p
             )
{
    p->OverType = !p->OverType;
    SetOverType_StatusBar(p->OverType);

} /* PaneEditMode */


/***  PaneClearEdit
**
**  Synopsis:
**      void PaneClearEdit( PPANE p)
**
**  Entry:
**      p    - The Pane info for the window
**
**  Returns:
**      None
**
**  Description:
**      Aborts the current edit and repaints the item
*/

void
PaneClearEdit(
    PPANE p
    )
{
    p->Edit   = FALSE;
    p->CurLen = 0;
    memset(p->EditBuf,0,EDITMAX);
    PaneInvalidateCurrent( p->hWndFocus, p, 0);

}     /* PaneClearEdit */


/***  PaneCloseEdit
**
**  Synopsis:
**      BOOL PaneCloseEdit( PPANE p)
**
**  Entry:
**      p    - The Pane info for the window
**
**  Returns:
**      None
**
**  Description:
**      Attempts to close the edit and update the item.
*/

BOOL
PaneCloseEdit(
              PPANE p
              )
{
    //  If no Edit open, we're done
    if ( !p->Edit ) {
        PaneInvalidateCurrent( p->hWndRight, p, -1);
        return(TRUE);
    }

    if ( (*p->fnEditProc)(p->hWndFocus, WU_SETWATCH, 0, (LPARAM)p) ) {
        PaneClearEdit(p);
        return(TRUE);
    }

    PaneClearEdit(p);
    MessageBeep(0);
    return(FALSE);
}                              /* PaneCloseEdit */


/***  PaneInvalidateRow
**
**  Synopsis:
**      void PaneInvalidateRow(PPANE p)
**
**  Entry:
**      p    - The Pane info for the window
**
**  Returns:
**      None
**
**  Description:
**      Invalidate all panes on the current index.
*/

void
PaneInvalidateRow(
                  PPANE p
                  )
{
    PaneInvalidateCurrent( p->hWndButton, p, -1);
    PaneInvalidateCurrent( p->hWndLeft, p, -1);
    PaneInvalidateCurrent( p->hWndRight, p, -1);
}


/***  PaneInvalidateCurrent
**
**  Synopsis:
**      void PaneInvalidateCurrent( HWND hWnd, PPANE p, SHORT pos)
**
**  Entry:
**      hWnd - The handle to the current window
**      p    - The Pane info for the window
**      pos  - A optional position to set the caret to
**
**  Returns:
**      None
**
**  Description:
**      Invalidate the current item that has the focus causing it to be
**      repainted
*/


void
PaneInvalidateCurrent(
                      HWND hWnd,
                      PPANE p,
                      SHORT pos
                      )
{

    PaneInvalidateItem( hWnd, p, p->CurIdx);

    if ( pos >= 0) {
        PaneSetPos(p,pos);
    }

}   /*  PaneInvalidateCurrent */

/***  PaneInvalidateItem
**
**  Synopsis:
**      void PaneInvalidateItem( HWND hWnd, PPANE p, WORD Item)
**
**  Entry:
**      hWnd - The handle to the current window
**      p    - The Pane info for the window
**      pos  - Item number to invalidate
**
**  Returns:
**      None
**
**  Description:
**      Invalidate the a given item and cause it to be repainted
*/

void
PaneInvalidateItem(
                   HWND hWnd,
                   PPANE p,
                   SHORT item
                   )
{
    RECT Rect;

    if ( hWnd && p->PaneLines > 0 ) {
        SendMessage( hWnd, LB_GETITEMRECT, (WPARAM)item, (LPARAM)&Rect);
        InvalidateRect( hWnd, &Rect, TRUE);
    }

}   /*  PaneInvalidateItem */

/***  PaneSwitchFocus
**
**  Synopsis:
**      PaneSwitchFocus(PPANE p, HWND hwnd, BOOL fPrev);
**
**  Entry:
**      p  - Pointer to the Pane structure.
**    hWnd - Handle to the New pane (NULL if we get to pick)
**    Prev - flag to force a switch to reverse order
**
**  Returns:
**      None
**
**  Description:
**      Switchs to the next logical pane in the panemanager.
*/

void
PaneSwitchFocus(
                PPANE p,
                HWND hWnd,
                BOOL fPrev
                )
{

    if ( !PaneCloseEdit(p) ) {
        MessageBeep(0);
        return;
    }

    // We're not selecting, need to lose the caret
    // and are going to repaint the current item (Caret may
    // have left junk on screen.

    p->SelPos = 0;
    p->SelLen = 0;

    PaneInvalidateCurrent( p->hWndFocus, p, -1);

    //  If we got a hWnd to set use that one
    if (hWnd != NULL) {
        p->hWndFocus = hWnd;
    }
    else if (p->hWndFocus == NULL) {   //  If we don't have one, default to left pane
        p->hWndFocus = p->hWndLeft;
    }

    //
    //  Other wise move to the right one
    //
    else if ( p->hWndFocus == p->hWndLeft ) {
        if (fPrev == FALSE) {
            p->hWndFocus = p->hWndRight;
        } else {
            p->hWndFocus = p->hWndButton;
        }
    }
    else if ( p->hWndFocus == p->hWndRight) {
        if (fPrev == FALSE) {
            p->hWndFocus = p->hWndButton;
        } else {
            p->hWndFocus = p->hWndLeft;
        }
    }
    else if ( p->hWndFocus == p->hWndButton) {
        if (fPrev == FALSE) {
            p->hWndFocus = p->hWndLeft;
        } else {
            p->hWndFocus = p->hWndRight;
        }
    }
    else {
        p->hWndFocus = p->hWndLeft;
    }

    //
    // Make sure we have the right control number
    //

    if ( p->hWndFocus == p->hWndLeft ) {
        p->nCtrlId = ID_PANE_LEFT;
    } else if ( p->hWndFocus == p->hWndRight ) {
        p->nCtrlId = ID_PANE_RIGHT;
    } else {
        p->nCtrlId = ID_PANE_BUTTON;
    }

    SetFocus(p->hWndFocus);
    PaneResetIdx( p, p->CurIdx);

}   /* PaneSwitch */

