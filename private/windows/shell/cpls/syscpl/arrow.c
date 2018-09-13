/** FILE: arrow.c ********** Module Header ********************************
 *
 * Control panel utility library routines for managing "cpArrow" window
 * class/spinner controls used in applet dialogs.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  12:00 on Fri   07 Aug 1992  -by-  Steve Cathcart   [stevecat]
 *        Implemented new drawing scheme for spinner/arrow control
 *  17:00 on Mon   18 Sep 1995  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update - SUR release NT v4.0
 *
 *
 *  Copyright (C) 1990-1995 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                        Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "system.h"

//==========================================================================
//                        Local Definitions
//==========================================================================

//  Offsets to use with GetWindowLong
#define GWL_SPINNERSTATE    0

//  Control state flags.
#define SPINNERSTATE_GRAYED      0x0001
#define SPINNERSTATE_HIDDEN      0x0002
#define SPINNERSTATE_MOUSEOUT    0x0004
#define SPINNERSTATE_UPCLICK     0x0008
#define SPINNERSTATE_DOWNCLICK   0x0010

//  Combination of click states.
#define SPINNERSTATE_CLICKED   (SPINNERSTATE_UPCLICK | SPINNERSTATE_DOWNCLICK)

//  Combination of state flags.
#define SPINNERSTATE_ALL         0x001F

//  Sinner Control color indices
#define SPINNERCOLOR_FACE        0
#define SPINNERCOLOR_ARROW       1
#define SPINNERCOLOR_SHADOW      2
#define SPINNERCOLOR_HIGHLIGHT   3
#define SPINNERCOLOR_FRAME       4

#define CCOLORS                  5

//==========================================================================
//                        External Declarations
//==========================================================================


//==========================================================================
//                        Local Data Declarations
//==========================================================================

/*
 * Macros to change the control state given the state flag(s)
 */
#define StateSet( dwState, wFlags )    (dwState |=  (wFlags))
#define StateClear( dwState, wFlags )  (dwState &= ~(wFlags))
#define StateTest( dwState, wFlags )   (dwState &   (wFlags))


//Array of default colors, matching the order of SPINNERCOLOR_* values.
DWORD rgColorDef[CCOLORS]={
                         COLOR_BTNFACE,             //  SPINNERCOLOR_FACE
                         COLOR_BTNTEXT,             //  SPINNERCOLOR_ARROW
                         COLOR_BTNSHADOW,           //  SPINNERCOLOR_SHADOW
                         COLOR_BTNHIGHLIGHT,        //  SPINNERCOLOR_HIGHLIGHT
                         COLOR_WINDOWFRAME          //  SPINNERCOLOR_FRAME
                         };

BOOL   bArrowTimed = FALSE;
BOOL   bRight;
HANDLE hParent;


//==========================================================================
//                        Local Function Prototypes
//==========================================================================
void Draw3DButtonRect( HDC hDC, HPEN hPenHigh, HPEN hPenShadow, int x1,
                       int y1, int x2, int y2, BOOL fClicked );
LONG SpinnerPaint( HWND hWnd, DWORD dwSpinnerState );


//==========================================================================
//                            Functions
//==========================================================================

BOOL OddArrowWindow( HWND hArrowWnd )
{
    return( TRUE );
}


VOID ArrowTimerProc( HWND hWnd, UINT wMsg, UINT nID, DWORD dwTime )
{
    WORD  wScroll;
    DWORD dwSpinnerState;

    dwSpinnerState = (DWORD) GetWindowLong (hWnd, GWL_SPINNERSTATE);

    if (StateTest(dwSpinnerState, SPINNERSTATE_CLICKED))
    {
        wScroll = (StateTest(dwSpinnerState, SPINNERSTATE_DOWNCLICK)) ?
                                                    SB_LINEDOWN : SB_LINEUP;
        if (bRight == WM_RBUTTONDOWN)
            wScroll += SB_PAGEUP - SB_LINEUP;

            SendMessage(hParent, WM_VSCROLL,
                        MAKELONG(wScroll, GetWindowLong(hWnd, GWL_ID)),
                        (LONG) hWnd);
    }

    //  Don't need to call KillTimer(), because SetTimer will
    //  reset the right one

    SetTimer(hWnd, nID, 50, (TIMERPROC) ArrowTimerProc);

    return ;

    wMsg = wMsg;
    dwTime = dwTime;
}


/*
 * ClickedRectCalc
 *
 * Description:
 *  Calculates the rectangle of the clicked region based on the
 *  state flags SPINNERSTATE_UPCLICK and SPINNERSTATE_DOWNCLICK.
 *
 * Parameter:
 *  hWnd            HWND handle to the control window.
 *  lpRect          LPRECT rectangle structure to fill.
 *
 * Return Value:
 *  void
 *
 */

void ClickedRectCalc(HWND hWnd, DWORD dwState, LPRECT lpRect)
{
    int  cx, cy;

    GetClientRect (hWnd, lpRect);

    cx = lpRect->right  >> 1;
    cy = lpRect->bottom >> 1;

    if (StateTest(dwState, SPINNERSTATE_DOWNCLICK))
        lpRect->top = cy;
    else
        lpRect->bottom = 1+cy;

    return;
}

/*
 * ArrowControlProc
 *
 * Description:
 *
 *  Window Procedure for the Spinner/Arrow custom control.  Handles all
 *  messages like WM_PAINT just as a normal application window would.
 *  State information about the control is maintained ALL drawing is
 *  handled during WM_PAINT message processing.
 *
 */
LRESULT APIENTRY ArrowControlProc(HWND hArrow, UINT message, WPARAM wParam, LONG lParam)
{
    WORD    wScroll;
    POINT   pt;
    RECT    rect;
    int     x, y;
    int     cy;
    DWORD   dwSpinnerState, dwState;


    dwSpinnerState = (DWORD) GetWindowLong (hArrow, GWL_SPINNERSTATE);

    switch (message)
    {
    case WM_CREATE:
        dwSpinnerState = 0;
        SetWindowLong (hArrow, GWL_SPINNERSTATE, (LONG) dwSpinnerState);
        break;


    case WM_ENABLE:
        //  Handles disabling/enabling case.  Example of a
        //  change-state-and-repaint strategy since we let the
        //  painting code take care of the visuals.

        if (wParam)
            StateClear(dwSpinnerState, SPINNERSTATE_GRAYED);
        else
            StateSet(dwSpinnerState, SPINNERSTATE_GRAYED);

        SetWindowLong (hArrow, GWL_SPINNERSTATE, (LONG) dwSpinnerState);

        //  Force a repaint since the control will look different.

        InvalidateRect (hArrow, NULL, TRUE);
        UpdateWindow (hArrow);
        break;


    case WM_SHOWWINDOW:
        //  Set or clear the hidden flag. Windows will
        //  automatically force a repaint if we become visible.

        if (wParam)
            StateClear(dwSpinnerState, SPINNERSTATE_HIDDEN);
        else
            StateSet(dwSpinnerState, SPINNERSTATE_HIDDEN);

        SetWindowLong (hArrow, GWL_SPINNERSTATE, (LONG) dwSpinnerState);
        break;


    case WM_CANCELMODE:
        //  IMPORTANT MESSAGE!  WM_CANCELMODE means that a
        //  dialog or some other modal process has started.
        //  we must make sure that we cancel any clicked state
        //  we are in, kill the timers, and release the capture.

        StateClear(dwSpinnerState, SPINNERSTATE_CLICKED);
        if (bArrowTimed)
        {
            SendMessage (hParent, WM_VSCROLL, MAKELONG(SB_ENDSCROLL,
                           GetWindowLong (hArrow, GWL_ID)), (LONG) hArrow);
            KillTimer (hArrow, GetWindowLong (hArrow, GWL_ID));
            bArrowTimed = FALSE;
        }
        ReleaseCapture();
        break;

    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
        //  When we get a mouse down message, we know that the mouse
        //  is over the control.  We then do the following steps
        //  to set up the new state:
        //   1.  Hit-test the coordinates of the click to
        //       determine in which half the click occurred.
        //   2.  Set the appropriate SPINNERSTATE_*CLICK state
        //       and repaint that clicked half.  This is another
        //       example of a change-state-and-repaint strategy.
        //   3.  Send an initial scroll message.
        //   4.  Set the mouse capture.
        //   5.  Set the initial delay timer before repeating
        //       the scroll message.

        if (bRight)
            break;

        bRight = message;

        hParent = GetParent (hArrow);

        //  Get the mouse coordinates.
        x = (int) LOWORD(lParam);
        y = (int) HIWORD(lParam);

        //  Only need to hit-test the upper half
        //  Then change-state-and-repaint

        GetClientRect (hArrow, &rect);
        cy = rect.bottom >> 1;

        if (y > cy)
        {
            StateSet(dwSpinnerState, SPINNERSTATE_DOWNCLICK);
            rect.top = cy;
            wScroll = SB_LINEDOWN;
        }
        else
        {
            StateSet(dwSpinnerState, SPINNERSTATE_UPCLICK);
            rect.bottom = cy + 1;
            wScroll = SB_LINEUP;
        }

        SetWindowLong (hArrow, GWL_SPINNERSTATE, (LONG) dwSpinnerState);

        InvalidateRect (hArrow, &rect, TRUE);
        UpdateWindow (hArrow);

        SetCapture (hArrow);

        //  Process SHIFT key state along with button message

        if (wParam & MK_SHIFT)
        {
            if (message != WM_RBUTTONDOWN)
                wScroll += (WORD) (SB_TOP - SB_LINEUP);
            else
                wScroll += (WORD) (SB_THUMBPOSITION - SB_LINEUP);
        }
        else
        {
            if (message == WM_RBUTTONDOWN)
                wScroll += SB_PAGEUP - SB_LINEUP;

            bArrowTimed = SetTimer (hArrow, GetWindowLong (hArrow, GWL_ID),
                                             200, (TIMERPROC) ArrowTimerProc);
        }
        SendMessage (hParent, WM_VSCROLL, MAKELONG(wScroll,
                              GetWindowLong (hArrow, GWL_ID)), (LONG) hArrow);
        break;

    case WM_MOUSEMOVE:
        //  On WM_MOUSEMOVE messages we want to know if the mouse
        //  has moved out of the control when the control is in
        //  a clicked state.  If the control has not been clicked,
        //  then we have nothing to do.  Otherwise we want to set
        //  the SPINNERSTATE_MOUSEOUT flag and repaint so the button
        //  visually comes up.

        if (!StateTest(dwSpinnerState, SPINNERSTATE_CLICKED))
            break;

        //  Save copy of original state
        dwState = dwSpinnerState;

        //  Get the mouse coordinates.
        pt.x = (int) LOWORD(lParam);
        pt.y = (int) HIWORD(lParam);

        //  Get the area we originally clicked and the new POINT
        ClickedRectCalc (hArrow, dwSpinnerState, &rect);

        //  Hit-Test the rectange and change the state if necessary.
        if (PtInRect(&rect, pt))
            StateClear(dwSpinnerState, SPINNERSTATE_MOUSEOUT);
        else
            StateSet(dwSpinnerState, SPINNERSTATE_MOUSEOUT);

        SetWindowLong (hArrow, GWL_SPINNERSTATE, (LONG) dwSpinnerState);

        //  If the state changed, repaint the appropriate part of
        //  the control.
        if (dwState != dwSpinnerState)
        {
            InvalidateRect (hArrow, &rect, TRUE);
            UpdateWindow (hArrow);
        }

        break;


    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        //  A mouse button up event is much like WM_CANCELMODE since
        //  we have to clean out whatever state the control is in:
        //   1.  Kill any repeat timers we might have created.
        //   2.  Release the mouse capture.
        //   3.  Clear the clicked states and repaint, another example
        //       of a change-state-and-repaint strategy.

        if ((UINT) (bRight - WM_LBUTTONDOWN + WM_LBUTTONUP) == message)
        {
            bRight = 0;
            ReleaseCapture();

            if (bArrowTimed)
            {
                SendMessage (hParent, WM_VSCROLL, MAKELONG(SB_ENDSCROLL,
                               GetWindowLong (hArrow, GWL_ID)), (LONG) hArrow);
                KillTimer (hArrow, GetWindowLong (hArrow, GWL_ID));
                bArrowTimed = FALSE;
            }

            //  Repaint if necessary, only if we are clicked AND the mouse
            //  is still in the boundaries of the control.

            if (StateTest(dwSpinnerState, SPINNERSTATE_CLICKED) &&
                StateTest(dwSpinnerState, ~SPINNERSTATE_MOUSEOUT))
            {
                //  Calculate the rectangle before clearing states.
                ClickedRectCalc (hArrow, dwSpinnerState, &rect);

                //  Clear the states so we repaint properly.
                StateClear(dwSpinnerState, SPINNERSTATE_CLICKED | SPINNERSTATE_MOUSEOUT);

                SetWindowLong (hArrow, GWL_SPINNERSTATE, (LONG) dwSpinnerState);
                InvalidateRect (hArrow, &rect, TRUE);
                UpdateWindow (hArrow);
            }
        }
        break;


    case WM_PAINT:
        return SpinnerPaint (hArrow, dwSpinnerState);


    default:
        return (DefWindowProc (hArrow, message, wParam, lParam));
        break;
    }
    return(0L);
}


/*
 * SpinnerPaint
 *
 * Description:
 *
 *  Handles all WM_PAINT messages for the control and paints
 *  the control for the current state, whether it be clicked
 *  or disabled.
 *
 * Parameters:
 *  hWnd            HWND Handle to the control.
 *  dwSpinnerState  DWORD Spinner control status flags
 *
 * Return Value:
 *  LONG            0L.
 */

LONG SpinnerPaint (HWND hWnd, DWORD dwSpinnerState)
{
    PAINTSTRUCT ps;
    LPRECT      lpRect;
    RECT        rect;
    HDC         hDC;
    COLORREF    rgCr[CCOLORS];
    HPEN        rgHPen[CCOLORS];
    int         iColor;

    HBRUSH      hBrushArrow;
    HBRUSH      hBrushFace;
    HBRUSH      hBrushBlack;

    POINT       rgpt1[3];
    POINT       rgpt2[3];

    int         xAdd1=0, yAdd1=0;
    int         xAdd2=0, yAdd2=0;

    int         cx,  cy;        //  Whole dimensions
    int         cx2, cy2;       //  Half dimensions
    int         cx4, cy4;       //  Quarter dimensions

    lpRect = &rect;

    hDC = BeginPaint (hWnd, &ps);
    GetClientRect (hWnd, lpRect);

    //  Get colors that we'll need.  We do not want to cache these
    //  items since we may our top-level parent window may have
    //  received a WM_WININICHANGE message at which time the control
    //  is repainted.  Since this control never sees that message,
    //  we cannot assume that colors will remain the same throughout
    //  the life of the control.

    for (iColor = 0; iColor < CCOLORS; iColor++)
    {
        rgCr[iColor] = GetSysColor (rgColorDef[iColor]);

        rgHPen[iColor] = CreatePen (PS_SOLID, 1, rgCr[iColor]);
    }

    hBrushFace  = CreateSolidBrush (rgCr[SPINNERCOLOR_FACE]);
    hBrushArrow = CreateSolidBrush (rgCr[SPINNERCOLOR_ARROW]);
    hBrushBlack = GetStockObject (BLACK_BRUSH);

    //  These values are extremely cheap to calculate for the amount
    //  we are going to use them.

    cx  = lpRect->right  - lpRect->left;
    cy  = lpRect->bottom - lpRect->top;
    cx2 = cx  >> 1;
    cy2 = cy  >> 1;
    cx4 = cx2 >> 1;
    cy4 = cy2 >> 1;

    //  If one half is depressed, set the x/yAdd varaibles that we use
    //  to shift the small arrow image down and right.

    if (!StateTest(dwSpinnerState, SPINNERSTATE_MOUSEOUT))
    {
        if (StateTest(dwSpinnerState, SPINNERSTATE_UPCLICK))
        {
            xAdd1 = 1;
            yAdd1 = 1;
        }
        else if (StateTest(dwSpinnerState, SPINNERSTATE_DOWNCLICK))
        {
            xAdd2 = 1;
            yAdd2 = 1;
        }
    }

    //  Draw the face color and the outer frame
    SelectObject (hDC, hBrushFace);
    SelectObject (hDC, rgHPen[SPINNERCOLOR_FRAME]);

    Rectangle (hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

    //  Draw the horizontal center line.
    MoveToEx (hDC, 0, cy2, NULL);
    LineTo (hDC, cx, cy2);

    //  We do one of three modifications for drawing the borders:
    //   1) Both halves un-clicked.
    //   2) Top clicked,   bottom unclicked.
    //   3) Top unclicked, bottom clicked.
    //
    //  Case 1 is xAdd1==xAdd2==0
    //  Case 2 is xAdd1==1, xAdd2=0
    //  Case 3 is xAdd1==0, xAdd2==1

    //  Draw top and bottom buttons borders.
    Draw3DButtonRect (hDC, rgHPen[SPINNERCOLOR_HIGHLIGHT],
                      rgHPen[SPINNERCOLOR_SHADOW],
                      0,  0,  cx-1, cy2,  (BOOL) xAdd1);

    Draw3DButtonRect (hDC, rgHPen[SPINNERCOLOR_HIGHLIGHT],
                      rgHPen[SPINNERCOLOR_SHADOW],
                      0, cy2, cx-1, cy-1, (BOOL) xAdd2);


    //  Select default line color.
    SelectObject (hDC, rgHPen[SPINNERCOLOR_ARROW]);

    //  Draw the arrows depending on the enable state.
    if (StateTest (dwSpinnerState, SPINNERSTATE_GRAYED))
    {
        //  Draw arrow color lines in the upper left of the
        //  top arrow and on the top of the bottom arrow.
        //  Pen was already selected as a default.

        MoveToEx (hDC, cx2,   cy4-2, NULL);      //Top arrow
        LineTo   (hDC, cx2-3, cy4+1);
        MoveToEx (hDC, cx2-3, cy2+cy4-2, NULL);  //Bottom arrow
        LineTo   (hDC, cx2+3, cy2+cy4-2);

        //  Draw highlight color lines in the bottom of the
        //  top arrow and on the lower right of the bottom arrow.

        SelectObject (hDC, rgHPen[SPINNERCOLOR_HIGHLIGHT]);
        MoveToEx (hDC, cx2-3, cy4+1, NULL);      //Top arrow
        LineTo   (hDC, cx2+3, cy4+1);
        MoveToEx (hDC, cx2+3, cy2+cy4-2, NULL);  //Bottom arrow
        LineTo   (hDC, cx2,   cy2+cy4+1);
        SetPixel (hDC, cx2,   cy2+cy4+1, rgCr[SPINNERCOLOR_HIGHLIGHT]);
    }
    else
    {
        //  Top arrow polygon
        rgpt1[0].x = xAdd1 + cx2;
        rgpt1[0].y = yAdd1 + cy4 - 2;
        rgpt1[1].x = xAdd1 + cx2 - 3;
        rgpt1[1].y = yAdd1 + cy4 + 1;
        rgpt1[2].x = xAdd1 + cx2 + 3;
        rgpt1[2].y = yAdd1 + cy4 + 1;

        //  Bottom arrow polygon
        rgpt2[0].x = xAdd2 + cx2;
        rgpt2[0].y = yAdd2 + cy2 + cy4 + 1;
        rgpt2[1].x = xAdd2 + cx2 - 3;
        rgpt2[1].y = yAdd2 + cy2 + cy4 - 2;
        rgpt2[2].x = xAdd2 + cx2 + 3;
        rgpt2[2].y = yAdd2 + cy2 + cy4 - 2;

        //  Draw the arrows
        SelectObject (hDC, hBrushArrow);
        Polygon (hDC, (LPPOINT)rgpt1, 3);
        Polygon (hDC, (LPPOINT)rgpt2, 3);
    }

    //  Clean up
    EndPaint(hWnd, &ps);

    DeleteObject (hBrushFace);
    DeleteObject (hBrushArrow);

    for (iColor = 0; iColor < CCOLORS; iColor++)
    {
        if (rgHPen[iColor])
            DeleteObject (rgHPen[iColor]);
    }

    return 0L;
}


/*
 * Draw3DButtonRect
 *
 * Description:
 *  Draws the 3D button look within a given rectangle.  This rectangle
 *  is assumed to be bounded by a one pixel black border, so everything
 *  is bumped in by one.
 *
 * Parameters:
 *  hDC         DC to draw to.
 *  hPenHigh    HPEN highlight color pen.
 *  hPenShadow  HPEN shadow color pen.
 *  x1          int Upper left corner x.
 *  y1          int Upper left corner y.
 *  x2          int Lower right corner x.
 *  y2          int Lower right corner y.
 *  fClicked    BOOL specifies if the button is down or not (TRUE==DOWN)
 *
 * Return Value:
 *  void
 *
 */

void Draw3DButtonRect (HDC hDC, HPEN hPenHigh, HPEN hPenShadow, int x1,
                       int y1, int x2, int y2, BOOL fClicked)
{
    HPEN  hPenOrg;

    //  Shrink the rectangle to account for borders.
    x1+=1;
    x2-=1;
    y1+=1;
    y2-=1;

    hPenOrg = SelectObject (hDC, hPenShadow);

    if (fClicked)
    {
        //  Shadow on left and top edge when clicked.
        MoveToEx (hDC, x1, y2, NULL);
        LineTo (hDC, x1, y1);
        LineTo (hDC, x2+1, y1);
    }
    else
    {
        //  Lowest shadow line.
        MoveToEx (hDC, x1, y2, NULL);
        LineTo (hDC, x2, y2);
        LineTo (hDC, x2, y1-1);

        //  Upper shadow line.
        MoveToEx (hDC, x1+1, y2-1, NULL);
        LineTo (hDC, x2-1, y2-1);
        LineTo (hDC, x2-1, y1);

        SelectObject (hDC, hPenHigh);

        //  Upper highlight line.
        MoveToEx (hDC, x1, y2-1, NULL);
        LineTo (hDC, x1, y1);
        LineTo (hDC, x2, y1);
    }

    if (hPenOrg)
        SelectObject (hDC, hPenOrg);

    return;
}


BOOL RegisterArrowClass (HANDLE hModule)
{
    WNDCLASS wcArrow;

    wcArrow.lpszClassName = TEXT("cpArrow");
    wcArrow.hInstance     = hModule;
    wcArrow.lpfnWndProc   = ArrowControlProc;
    wcArrow.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcArrow.hIcon         = NULL;
    wcArrow.lpszMenuName  = NULL;
    wcArrow.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wcArrow.style         = CS_HREDRAW | CS_VREDRAW;
    wcArrow.cbClsExtra    = 0;
    wcArrow.cbWndExtra    = sizeof(DWORD);

    return(RegisterClass((LPWNDCLASS) &wcArrow));
}


VOID UnRegisterArrowClass (HANDLE hModule)
{
    UnregisterClass(TEXT("cpArrow"), hModule);
}

/*
short ArrowVScrollProc(wScroll, nCurrent, lpAVS)

wScroll is an SB_* message
nCurrent is the base value to change
lpAVS is a far pointer to the structure containing change amounts
      and limits to be used, along with a flags location for errors

returns a short value of the final amount
        the flags element in the lpAVS struct is
                0 if no problems found
         OVERFLOW set if the change exceeded upper limit (limit is returned)
        UNDERFLOW set if the change exceeded lower limit (limit is returned)
   UNKNOWNCOMMAND set if wScroll is not a known SB_* message

NOTE: Only one of OVERFLOW or UNDERFLOW may be set.  If you send in values
      that would allow both to be set, that's your problem.  Either can
      be set in combination with UNKNOWNCOMMAND (when the command is not
      known and the input value is out of bounds).
*/

short ArrowVScrollProc(short wScroll, short nCurrent, LPARROWVSCROLL lpAVS)
{
    short    nDelta;

/* Find the message and put the relative change in nDelta.  If the
   message is an absolute change, put 0 in nDelta and set nCurrent
   to the value specified.  If the command is unknown, set error
   flag, set nDelta to 0, and proceed through checks.
*/

    switch (wScroll)
    {
    case SB_LINEUP:
        nDelta = lpAVS->lineup;
        break;
    case SB_LINEDOWN:
        nDelta = lpAVS->linedown;
        break;
    case SB_PAGEUP:
        nDelta = lpAVS->pageup;
        break;
    case SB_PAGEDOWN:
        nDelta = lpAVS->pagedown;
        break;
    case SB_TOP:
        nCurrent = lpAVS->top;
        nDelta = 0;
        break;
    case SB_BOTTOM:
        nCurrent = lpAVS->bottom;
        nDelta = 0;
        break;
    case SB_THUMBTRACK:
        nCurrent = lpAVS->thumbtrack;
        nDelta = 0;
        break;
    case SB_THUMBPOSITION:
        nCurrent = lpAVS->thumbpos;
        nDelta = 0;
        break;
    case SB_ENDSCROLL:
        nDelta = 0;
        break;
    default:
        lpAVS->flags = UNKNOWNCOMMAND;
        nDelta = 0;
        break;
    }
    if (nCurrent + nDelta > lpAVS->top)
    {
        nCurrent = lpAVS->top;
        nDelta = 0;
        lpAVS->flags = OVERFLOW;
    }
    else if (nCurrent + nDelta < lpAVS->bottom)
    {
        nCurrent = lpAVS->bottom;
        nDelta = 0;
        lpAVS->flags = UNDERFLOW;
    }
    else
        lpAVS->flags = 0;
    return(nCurrent + nDelta);
}

