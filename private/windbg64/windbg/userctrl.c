/*
 *  Copyright   Microsoft 1991
 *
 *  Date        Jan 09, 1991
 *
 *  Project     Asterix/Obelix
 *
 *  History
 *  Date        Initial     Description
 *  ----        -------     -----------
 *  01-09-91    ChauV       Created for use with Tools and Status bars.
 *
 */

#include "precomp.h"
#pragma hdrstop


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/
/* begin static function prototypes *****************************************/

/* user defined Rectangular box */

void DrawBitmapButton (HWND, LPRECT) ;

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/
/* begin variable definitions ***********************************************/

static  BOOL        bTrack = FALSE ;            // mouse down flag
static  WORD       wOldState ;                  // preserve the control state just before
                                                                                                                                // a button is being pushed.

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/
/***    EnableQCQPCtrl
**
**  Synopsis:
**      void = EnableQCQPCtrl(hWnd, id, fEnable)
**
**  Entry:
**      hWnd    - parent's handle
**      id      - control's ID
**      fEnable - FALSE for disable
**
**  Returns:
**      nothing
**
**  Description:
**      This function enable or disable a control identified by the the
**      parent's handle and the control's ID. If enable is FALSE, the control
**      is grayed, otherwise it is activated. This function is only valid to
**      pushbutton style (QCQP_CS_PUSHBUTTON).
**
*/

void
EnableQCQPCtrl(
    HWND hWnd,
    int id,
    BOOL enable
    )
{
    HWND hwndControl;

    hwndControl = GetDlgItem(hWnd, id);

    InvalidateRect(hwndControl, (LPRECT)NULL, FALSE);

    return;
}                                       /* EnableQCQPCtrl() */

/***    GetBitmapIndex
**
**  Synopsis:
**      word = GetBitmapIndex(state)
**
**  Entry:
**      state -
**
**  Returns:
**
**  Description:
**
*/

WORD NEAR PASCAL GetBitmapIndex(WORD State)
{
    switch (State) {
      case STATE_NORMAL:
        return CBWNDEXTRA_BMAP_NORMAL;

      case STATE_PUSHED:
        return CBWNDEXTRA_BMAP_PUSHED;

      case STATE_GRAYED:
        return CBWNDEXTRA_BMAP_GREYED;

      default:
        Assert(FALSE);
        // Have to return something.
        return CBWNDEXTRA_BMAP_NORMAL;
    }
}                                       /* GetBitmapIndex() */


/***    CreateQCQPWindow
**
**  Synopsis:
**      hwnd = CreateQCQPWindow(lpszWindowName, dwStyle, x, y, dx, dy
**                      hParent, hMenu, hInstance, wMessage)
**
**  Entry:
**      lpszWindowName  -
**      dwStyle         -
**      x               -
**      y               -
**      dx              -
**      dy              -
**      hParent         -
**      hMenu           -
**      hInstance       -
**      wMessage        -
**
**  Returns:
**
**  Description:
**
*/

HWND CreateQCQPWindow(LPSTR lpWindowName,
         DWORD   dwStyle,
         int     x,
         int     y,
         int     dx,
         int     dy,
         HWND    hParent,
         HMENU   hMenu,
         HINSTANCE hInstance,
         WPARAM  wMessage)
{
    HWND hTemp ;
    char szClass[MAX_MSG_TXT] ;
    WORD BaseId = 0;
    WORD State;
    HBITMAP hBitmap;

    Dbg(LoadString(hInstance, SYS_QCQPCtrl_wClass, szClass, MAX_MSG_TXT)) ;
    hTemp = CreateWindow(
          (LPSTR)szClass,            // Window szClass name
          lpWindowName,            // Window's title
          WS_CHILD | WS_VISIBLE,   // window created visible
          x, y,                    // X, Y
          dx, dy,                  // Width, Height of window
          hParent,                 // Parent window's handle
          hMenu,                   // child's id
          hInstance,               // Instance of window
          NULL);                   // Create struct for WM_CREATE

    if (hTemp != NULL) {
        SetWindowWord (hTemp, CBWNDEXTRA_STYLE, LOWORD(dwStyle)) ;
        SetWindowWord (hTemp, CBWNDEXTRA_BITMAP, HIWORD(dwStyle)) ;
        SetWindowWord (hTemp, CBWNDEXTRA_STATE, STATE_NORMAL) ;
        SetWindowHandle (hTemp, CBWNDEXTRA_MESSAGE, wMessage) ;

        if (LOWORD(dwStyle) == QCQP_CS_PUSHBUTTON) {
            // Load the bitmaps and store the handles
            switch (HIWORD(dwStyle)) {
              case IDS_CTRL_TRACENORMAL:
              case IDS_CTRL_TRACEPUSHED:
              case IDS_CTRL_TRACEGRAYED:
                BaseId = VGA_TRACE_NORMAL;
                break;

              case IDS_CTRL_STEPNORMAL:
              case IDS_CTRL_STEPPUSHED:
              case IDS_CTRL_STEPGRAYED:
                BaseId = VGA_STEP_NORMAL;
                break;

              case IDS_CTRL_BREAKNORMAL:
              case IDS_CTRL_BREAKPUSHED:
              case IDS_CTRL_BREAKGRAYED:
                BaseId = VGA_BREAK_NORMAL;
                break;

              case IDS_CTRL_GONORMAL:
              case IDS_CTRL_GOPUSHED:
              case IDS_CTRL_GOGRAYED:
                BaseId = VGA_GO_NORMAL;
                break;

              case IDS_CTRL_HALTNORMAL:
              case IDS_CTRL_HALTPUSHED:
              case IDS_CTRL_HALTGRAYED:
                BaseId = VGA_HALT_NORMAL;
                break;

              case IDS_CTRL_QWATCHNORMAL:
              case IDS_CTRL_QWATCHPUSHED:
              case IDS_CTRL_QWATCHGRAYED:
                BaseId = VGA_QWATCH_NORMAL;
                break;

              case IDS_CTRL_SMODENORMAL:
              case IDS_CTRL_SMODEPUSHED:
              case IDS_CTRL_SMODEGRAYED:
                BaseId = VGA_SMODE_NORMAL;
                break;

              case IDS_CTRL_AMODENORMAL:
              case IDS_CTRL_AMODEPUSHED:
              case IDS_CTRL_AMODEGRAYED:
                BaseId = VGA_AMODE_NORMAL;
                break;


              case IDS_CTRL_FORMATNORMAL:
              case IDS_CTRL_FORMATPUSHED:
              case IDS_CTRL_FORMATGRAYED:
                BaseId = VGA_FORMAT_NORMAL;
                break;


              default:


                Assert(FALSE);
            }

            // Load the bitmaps for each state for the button
            for (State = STATE_NORMAL; State <= STATE_GRAYED; State++) {

                Dbg(hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE( BaseId + State )));

                SetWindowHandle(hTemp, GetBitmapIndex(State), (WPARAM)hBitmap);
            }
        }
    }

    return hTemp ;
}                                       /* CreateQCQPWindow() */


/***    QCQPCtrlWndProc
**
**  Synopsis:
**      lresult = QCQPCtrlWndProc(hWnd, iMessage, wParam, lParam)
**
**  Entry:
**      hWnd
**      iMessage
**      wParam
**      lParam
**
**  Returns:
**
**  Description:
**
*/

LRESULT
CALLBACK
QCQPCtrlWndProc(
                HWND hWnd,
                UINT iMessage,
                WPARAM wParam,
                LPARAM lParam
                )
{
    PAINTSTRUCT     ps ;
    char            szText [128] ;
    WPARAM          wStyle ;
    RECT            r ;

    wStyle = GetWindowWord (hWnd, CBWNDEXTRA_STYLE) ;
    switch ( iMessage ) {
      case WM_CREATE:
        bTrack = FALSE ;
        break ;

      case WM_PAINT:
        GetClientRect (hWnd, (LPRECT)&r) ;

        BeginPaint (hWnd, &ps) ;

        switch ( wStyle ) {
          case QCQP_CS_PUSHBUTTON:
          case QCQP_CS_LATCHBUTTON:
            DrawBitmapButton (hWnd, (LPRECT)&r) ;
            break ;

          default:
            break ;
        }

        EndPaint (hWnd, &ps) ;
        break ;

      case WM_LBUTTONUP:
        if ( GetWindowWord (hWnd, CBWNDEXTRA_STATE) != STATE_GRAYED ) {
            bTrack = FALSE ;
            ReleaseCapture () ;
            switch (wStyle) {
              case QCQP_CS_PUSHBUTTON:
                // Only change the state and send message back to parent
                // if state is not normal. This prevent user from clicking
                // the mouse on the button then dragging it outside of
                // the button.

                if (GetWindowWord (hWnd, CBWNDEXTRA_STATE) != STATE_NORMAL) {
                    SetWindowWord (hWnd, CBWNDEXTRA_STATE, STATE_NORMAL) ;
                    InvalidateRect (hWnd, (LPRECT)NULL, FALSE) ;

                    // Send information back to where the function key is being
                    // used for the same purpose.

                    SendMessage (GetParent (hWnd),
                          WM_COMMAND,
                          (WPARAM) GetWindowHandle (hWnd, CBWNDEXTRA_MESSAGE),
                          MAKELONG(0, GetDlgCtrlID (hWnd))) ;
                }
                break ;

              case QCQP_CS_LATCHBUTTON:
                if (GetWindowWord (hWnd, CBWNDEXTRA_STATE) != wOldState) {
                    if (wOldState == STATE_NORMAL)
                        SetWindowWord (hWnd, CBWNDEXTRA_STATE, STATE_ON) ;
                    else
                          SetWindowWord (hWnd, CBWNDEXTRA_STATE, STATE_NORMAL) ;

                    InvalidateRect (hWnd, (LPRECT)NULL, FALSE) ;

                    // Send information back to where the function key is being
                    // used for the same purpose.

                    SendMessage (GetParent (hWnd),
                          WM_COMMAND,
                          (WPARAM) GetWindowHandle (hWnd, CBWNDEXTRA_MESSAGE),
                          MAKELONG(0, GetDlgCtrlID (hWnd))) ;
                }
                break ;
            }
        }
        break ;

      case WM_LBUTTONDOWN:
        if ( GetWindowWord (hWnd, CBWNDEXTRA_STATE) != STATE_GRAYED ) {
            bTrack = TRUE ;
            wOldState = GetWindowWord (hWnd, CBWNDEXTRA_STATE) ;
            switch (wStyle) {
              case QCQP_CS_PUSHBUTTON:
              case QCQP_CS_LATCHBUTTON:
                SetWindowWord (hWnd, CBWNDEXTRA_STATE, STATE_PUSHED) ;
                InvalidateRect (hWnd, (LPRECT)NULL, FALSE) ;
                break ;
            }
            SetCapture (hWnd) ;
        }
        break ;

      case WM_MOUSEMOVE:
        if ( GetWindowWord (hWnd, CBWNDEXTRA_STATE) != STATE_GRAYED ) {
            if ( bTrack ) {
                int             x, y ;

                x = LOWORD (lParam) ;   // get x position
                y = HIWORD (lParam) ;   // get y position
                GetClientRect (hWnd, &r) ;

                // if mouse position is outside of button area, bring it
                // back to its old state stored in wOldState.

                if ( ((x < r.left) || (x > r.right)) ||
                    ((y < r.top) || (y > r.bottom)) ) {
                    // redraw the button only if it's not in normal position.
                    if ( GetWindowWord (hWnd, CBWNDEXTRA_STATE) != wOldState ) {
                        SetWindowWord (hWnd, CBWNDEXTRA_STATE, wOldState) ;
                        InvalidateRect (hWnd, (LPRECT)NULL, FALSE) ;
                    }
                } else {
                    // redraw the button only if it's not in pushed position.

                    if ( GetWindowWord (hWnd, CBWNDEXTRA_STATE) != STATE_PUSHED ) {
                        SetWindowWord (hWnd, CBWNDEXTRA_STATE, STATE_PUSHED) ;
                        InvalidateRect (hWnd, (LPRECT)NULL, FALSE) ;
                    }
                }
            }
        }
        break ;

      default:
        return DefWindowProc (hWnd, iMessage, wParam, lParam) ;
        break ;
    }
    return 0L ;
}                                       /* QCQPCtrlWndProc() */


/***    DrawBitmapButton
**
**  Synopsis:
**      void = DrawBitmapButton(hWnd, r)
**
**  Entry:
**      hWnd
**      r
**
**  Returns:
**      Nothing
**
**  Description:
**
**
*/

void DrawBitmapButton (HWND hWnd, LPRECT r)
{
    HDC         hDC, hMemoryDC ;
    HBITMAP     hBitmap, hTempBitmap ;
    int         OldStretchMode ;
    BITMAP      Bitmap ;
    WORD        State;

    State = (WORD) GetWindowWord(hWnd, CBWNDEXTRA_STATE);
    hBitmap = (HBITMAP) GetWindowHandle(hWnd, GetBitmapIndex(State));

    hDC = GetDC (hWnd) ;
    Dbg(hMemoryDC = CreateCompatibleDC (hDC));
    Dbg(GetObject (hBitmap, sizeof(BITMAP), (LPSTR) &Bitmap));

    // save the current bitmap handle.
    Dbg(hTempBitmap = (HBITMAP) SelectObject (hMemoryDC, hBitmap));

    OldStretchMode = SetStretchBltMode (hDC, COLORONCOLOR);
    StretchBlt (hDC, r->left, r->top,
          r->right, r->bottom,
          hMemoryDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, SRCCOPY);

    SetStretchBltMode(hDC, OldStretchMode);

    // restore the old bitmap back into DC

    SelectObject(hMemoryDC, hTempBitmap);
    Dbg(DeleteDC(hMemoryDC));
    Dbg(ReleaseDC(hWnd, hDC));

    return;
}                                       /* DrawBitmapButton() */

/***    FreeBitmaps
**
**  Synopsis:
**      void = FreeBitmaps(hwnd, ctrlId)
**
**  Entry:
**      hwnd   -
**      ctrlId -
**
**  Returns:
**      nothing
**
**  Description;
**
*/

void NEAR PASCAL FreeBitmaps(HWND hwndToolbar, int CtrlId)
{
    HWND hwndCtrl;
    WORD State;
    HBITMAP hBitmap;

    hwndCtrl = GetDlgItem(hwndToolbar, CtrlId);

    for (State = STATE_NORMAL; State <= STATE_GRAYED; State++) {
        hBitmap = (HBITMAP)GetWindowHandle(hwndCtrl, GetBitmapIndex(State));
        Dbg(DeleteObject(hBitmap));
    }
}                                       /* FreeBitmaps() */

