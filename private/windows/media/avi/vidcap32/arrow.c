/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   arrow.c: Arrow control window
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/


#include <windows.h>
#include <windowsx.h>

#include <stdlib.h>

#include "arrow.h"


// a few porting macros
#ifdef _WIN32
#define SENDSCROLL(hwnd, msg, a, b, h)           \
        SendMessage(hwnd, msg, (UINT)MAKELONG(a,b), (LONG_PTR)(h))

#define EXPORT

#else
#define SENDSCROLL(hwnd, msg, a, b, h)
        SendMessage(hwnd, msg, a, MAKELONG(b,h))   // handle is in HIWORD

#endif


#ifndef LONG2POINT
    #define LONG2POINT(l, pt)               ((pt).x = (SHORT)LOWORD(l), (pt).y = (SHORT)HIWORD(l))
#endif
#define GWID(hwnd)      (GetDlgCtrlID(hwnd))


#define SHIFT_TO_DOUBLE 1
#define DOUBLECLICK     0
#define POINTSPERARROW  3
#define ARROWXAXIS      15
#define ARROWYAXIS      15

POINT ArrowUp[POINTSPERARROW] = {7,1, 3,5, 11,5};
POINT ArrowDown[POINTSPERARROW] = {7,13, 3,9, 11,9};

static    BOOL      bRight;
static    RECT      rUp, rDown;
static    LPRECT    lpUpDown;
static    FARPROC   lpArrowProc;
static    HANDLE    hParent;
BOOL      fInTimer;


#define TEMP_BUFF_SIZE    32

#define SCROLLMSG(hwndTo, msg, code, hwndId)                                     \
                          SENDSCROLL(hwndTo, msg, code, GWID(hwndId), hwndId)

/*
 * @doc EXTERNAL WINCOM
 *
 * @api LONG | ArrowEditChange | This function is helps process the WM_VSCROLL
 * message when using the Arrow controlled edit box.
 * It will increment/decrement the value in the given edit box and return
 * the new value.  Increment/decrement bounds are checked and Beep 0 is produced if
 * the user attempts to go beyond the bounds.
 *
 * @parm        HWND | hwndEdit | Specifies a handle to the edit box window.
 *
 * @parm        UINT | wParam | Specifies the <p wParam> passed to the WM_VSCROLL message.
 *
 * @parm        LONG | lMin | Specifies the minimum value bound for decrements.
 *
 * @parm        LONG | lMax | Specifies the maximum value bound for increments.
 *
 * @rdesc       Returns the updated value of the edit box.
 *
 */
LONG FAR PASCAL ArrowEditChange( HWND hwndEdit, UINT wParam,
                                 LONG lMin, LONG lMax )
{
    TCHAR achTemp[TEMP_BUFF_SIZE];
    LONG l;

    GetWindowText( hwndEdit, achTemp, TEMP_BUFF_SIZE );
    l = atol(achTemp);
    if( wParam == SB_LINEUP ) {
        /* size kluge for now */
        if( l < lMax ) {
            l++;
            wsprintf( achTemp, "%ld", l );
            SetWindowText( hwndEdit, achTemp );
        } else {
        MessageBeep( 0 );
        }
    } else if( wParam == SB_LINEDOWN ) {
        if( l > lMin ) {
            l--;
            wsprintf( achTemp, "%ld", l );
            SetWindowText( hwndEdit, achTemp );
        } else {
            MessageBeep( 0 );
        }
    }
    return( l );

}



UINT NEAR PASCAL UpOrDown()
{
    LONG pos;
    UINT retval;
    POINT pt;

    pos = GetMessagePos();
    LONG2POINT(pos,pt);
    if (PtInRect((LPRECT)&rUp, pt))
        retval = SB_LINEUP;
    else if (PtInRect((LPRECT)&rDown, pt))
        retval = SB_LINEDOWN;
    else
        retval = (UINT)(-1);      /* -1, because SB_LINEUP == 0 */

    return(retval);
}



UINT FAR PASCAL ArrowTimerProc(hWnd, wMsg, nID, dwTime)
HANDLE hWnd;
UINT wMsg;
short nID;
DWORD dwTime;
{
    UINT wScroll;

    if ((wScroll = UpOrDown()) != -1)
    {
        if (bRight == WM_RBUTTONDOWN)
            wScroll += SB_PAGEUP - SB_LINEUP;
        SCROLLMSG( hParent, WM_VSCROLL, wScroll, hWnd);
    }
/* Don't need to call KillTimer(), because SetTimer will reset the right one */
    SetTimer(hWnd, nID, 50, (TIMERPROC)lpArrowProc);
    return(0);
}


void InvertArrow(HANDLE hArrow, UINT wScroll)
{
    HDC hDC;

    lpUpDown = (wScroll == SB_LINEUP) ? &rUp : &rDown;
    hDC = GetDC(hArrow);
    ScreenToClient(hArrow, (LPPOINT)&(lpUpDown->left));
    ScreenToClient(hArrow, (LPPOINT)&(lpUpDown->right));
    InvertRect(hDC, lpUpDown);
    ClientToScreen(hArrow, (LPPOINT)&(lpUpDown->left));
    ClientToScreen(hArrow, (LPPOINT)&(lpUpDown->right));
    ReleaseDC(hArrow, hDC);
    ValidateRect(hArrow, lpUpDown);
    return;
}


LRESULT FAR PASCAL EXPORT ArrowControlProc(HWND hArrow, unsigned message,
                                         WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT        rArrow;
    HBRUSH      hbr;
    short       fUpDownOut;
    UINT        wScroll;

    switch (message) {
/*
        case WM_CREATE:
            break;

        case WM_DESTROY:
            break;
*/

        case WM_MOUSEMOVE:
            if (!bRight)  /* If not captured, don't worry about it */
                break;

            if (lpUpDown == &rUp)
                fUpDownOut = SB_LINEUP;
            else if (lpUpDown == &rDown)
                fUpDownOut = SB_LINEDOWN;
            else
                fUpDownOut = -1;

            switch (wScroll = UpOrDown()) {
                case SB_LINEUP:
                    if (fUpDownOut == SB_LINEDOWN)
                        InvertArrow(hArrow, SB_LINEDOWN);

                    if (fUpDownOut != SB_LINEUP)
                        InvertArrow(hArrow, wScroll);

                    break;

                case SB_LINEDOWN:
                    if (fUpDownOut == SB_LINEUP)
                        InvertArrow(hArrow, SB_LINEUP);

                    if (fUpDownOut != SB_LINEDOWN)
                        InvertArrow(hArrow, wScroll);

                    break;

                default:
                    if (lpUpDown) {
                        InvertArrow(hArrow, fUpDownOut);
                        lpUpDown = 0;
                    }
                }

                break;

        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN:
            if (bRight)
                break;

            bRight = message;
            SetCapture(hArrow);
            hParent = GetParent(hArrow);
            GetWindowRect(hArrow, (LPRECT) &rUp);
            CopyRect((LPRECT)&rDown, (LPRECT) &rUp);
            rUp.bottom = (rUp.top + rUp.bottom) / 2;
            rDown.top = rUp.bottom + 1;
            wScroll = UpOrDown();
            InvertArrow(hArrow, wScroll);
#if SHIFT_TO_DOUBLE
            if (wParam & MK_SHIFT) {
                if (message != WM_RBUTTONDOWN)
                    goto ShiftLClick;
                else
                    goto ShiftRClick;
            }
#endif
            if (message == WM_RBUTTONDOWN)
                wScroll += SB_PAGEUP - SB_LINEUP;

            SCROLLMSG(hParent, WM_VSCROLL, wScroll, hArrow);

            lpArrowProc = MakeProcInstance((FARPROC) ArrowTimerProc,ghInst);
            SetTimer(hArrow, GWID(hArrow), 200, (TIMERPROC)lpArrowProc);

            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            if ((bRight - WM_LBUTTONDOWN + WM_LBUTTONUP) == (int)message) {
                bRight = 0;
                ReleaseCapture();
                if (lpUpDown)
                    InvertArrow(hArrow,(UINT)(lpUpDown==&rUp)?
                                                        SB_LINEUP:SB_LINEDOWN);
                if (lpArrowProc) {
                    SCROLLMSG(hParent, WM_VSCROLL, SB_ENDSCROLL, hArrow);
                    KillTimer(hArrow, GWID(hArrow));

                    FreeProcInstance(lpArrowProc);
                    ReleaseCapture();
                    lpArrowProc = 0;
                }
            }
            break;

        case WM_LBUTTONDBLCLK:
ShiftLClick:
            wScroll = UpOrDown() + SB_TOP - SB_LINEUP;
            SCROLLMSG(hParent, WM_VSCROLL, wScroll, hArrow);
            SCROLLMSG(hParent, WM_VSCROLL, SB_ENDSCROLL, hArrow);

            break;

        case WM_RBUTTONDBLCLK:
ShiftRClick:
            wScroll = UpOrDown() + SB_THUMBPOSITION - SB_LINEUP;
            SCROLLMSG(hParent, WM_VSCROLL, wScroll, hArrow);
            SCROLLMSG(hParent, WM_VSCROLL, SB_ENDSCROLL, hArrow);
/*
            hDC = GetDC(hArrow);
            InvertRect(hDC, (LPRECT) &rArrow);
            ReleaseDC(hArrow, hDC);
            ValidateRect(hArrow, (LPRECT) &rArrow);
*/
            break;

        case WM_PAINT:
            BeginPaint(hArrow, &ps);
            GetClientRect(hArrow, (LPRECT) &rArrow);
            hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(ps.hdc, (LPRECT)&rArrow, hbr);
            DeleteObject(hbr);
            hbr = SelectObject(ps.hdc, GetStockObject(BLACK_BRUSH));
            SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWFRAME));
            SetMapMode(ps.hdc, MM_ANISOTROPIC);

            SetViewportOrgEx(ps.hdc, rArrow.left, rArrow.top, NULL);

            SetViewportExtEx(ps.hdc, rArrow.right - rArrow.left,
                                                    rArrow.bottom - rArrow.top, NULL);
            SetWindowOrgEx(ps.hdc, 0, 0, NULL);
            SetWindowExtEx(ps.hdc, ARROWXAXIS, ARROWYAXIS, NULL);
            MoveToEx(ps.hdc, 0, (ARROWYAXIS / 2), NULL);
            LineTo(ps.hdc, ARROWXAXIS, (ARROWYAXIS / 2));
/*
            Polygon(ps.hdc, (LPPOINT) Arrow, 10);
*/
            Polygon(ps.hdc, (LPPOINT) ArrowUp, POINTSPERARROW);
            Polygon(ps.hdc, (LPPOINT) ArrowDown, POINTSPERARROW);
            SelectObject(ps.hdc, hbr);

            EndPaint(hArrow, &ps);

            break;

        default:
            return(DefWindowProc(hArrow, message, wParam, lParam));

            break;
        }

    return(0L);
}

#ifndef _WIN32
#pragma alloc_text(_INIT, ArrowInit)
#endif


BOOL FAR PASCAL ArrowInit(HANDLE hInst)
{
    WNDCLASS wcArrow;

    wcArrow.lpszClassName = SPINARROW_CLASSNAME;
    wcArrow.hInstance     = hInst;
    wcArrow.lpfnWndProc   = ArrowControlProc;
    wcArrow.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcArrow.hIcon         = NULL;
    wcArrow.lpszMenuName  = NULL;
    wcArrow.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    wcArrow.style         = CS_HREDRAW | CS_VREDRAW;
#if DOUBLECLICK
    wcArrow.style         |= CS_DBLCLKS;
#endif
    wcArrow.cbClsExtra    = 0;
    wcArrow.cbWndExtra    = 0;

    if (!RegisterClass(&wcArrow))
        return FALSE;

    return TRUE;
}

