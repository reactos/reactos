/*
 * Progress control
 *
 * Copyright 1997, 2002 Dimitrie O. Paun
 * Copyright 1998, 1999 Eric Kohl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTE
 * 
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 9, 2002, by Dimitrie O. Paun.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 * 
 * TODO
 *   --support PBS_MARQUE
 *
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(progress);

typedef struct
{
    HWND      Self;         /* The window handle for this control */
    INT       CurVal;       /* Current progress value */
    INT       MinVal;       /* Minimum progress value */
    INT       MaxVal;       /* Maximum progress value */
    INT       Step;         /* Step to use on PMB_STEPIT */
    COLORREF  ColorBar;     /* Bar color */
    COLORREF  ColorBk;      /* Background color */
    HFONT     Font;         /* Handle to font (not unused) */
} PROGRESS_INFO;

/* Control configuration constants */

#define LED_GAP    2

/***********************************************************************
 * PROGRESS_Invalidate
 *
 * Invalide the range between old and new pos.
 */
static void PROGRESS_Invalidate( PROGRESS_INFO *infoPtr, INT old, INT new )
{
    LONG style = GetWindowLongW (infoPtr->Self, GWL_STYLE);
    RECT rect;
    int oldPos, newPos, ledWidth;

    GetClientRect (infoPtr->Self, &rect);
    InflateRect(&rect, -1, -1);

    if (style & PBS_VERTICAL)
    {
        oldPos = rect.bottom - MulDiv (old - infoPtr->MinVal, rect.bottom - rect.top,
                                       infoPtr->MaxVal - infoPtr->MinVal);
        newPos = rect.bottom - MulDiv (new - infoPtr->MinVal, rect.bottom - rect.top,
                                       infoPtr->MaxVal - infoPtr->MinVal);
        ledWidth = MulDiv (rect.right - rect.left, 2, 3);
        rect.top = min( oldPos, newPos );
        rect.bottom = max( oldPos, newPos );
        if (!(style & PBS_SMOOTH)) rect.top -= ledWidth;
        InvalidateRect( infoPtr->Self, &rect, oldPos < newPos );
    }
    else
    {
        oldPos = rect.left + MulDiv (old - infoPtr->MinVal, rect.right - rect.left,
                                     infoPtr->MaxVal - infoPtr->MinVal);
        newPos = rect.left + MulDiv (new - infoPtr->MinVal, rect.right - rect.left,
                                     infoPtr->MaxVal - infoPtr->MinVal);
        ledWidth = MulDiv (rect.bottom - rect.top, 2, 3);
        rect.left = min( oldPos, newPos );
        rect.right = max( oldPos, newPos );
        if (!(style & PBS_SMOOTH)) rect.right += ledWidth;
        InvalidateRect( infoPtr->Self, &rect, oldPos > newPos );
    }
}


/***********************************************************************
 * PROGRESS_Draw
 * Draws the progress bar.
 */
static LRESULT PROGRESS_Draw (PROGRESS_INFO *infoPtr, HDC hdc)
{
    HBRUSH hbrBar, hbrBk;
    int rightBar, rightMost, ledWidth;
    RECT rect;
    DWORD dwStyle;

    TRACE("(infoPtr=%p, hdc=%p)\n", infoPtr, hdc);

    /* get the required bar brush */
    if (infoPtr->ColorBar == CLR_DEFAULT)
        hbrBar = GetSysColorBrush(COLOR_HIGHLIGHT);
    else
        hbrBar = CreateSolidBrush (infoPtr->ColorBar);

    if (infoPtr->ColorBk == CLR_DEFAULT)
        hbrBk = GetSysColorBrush(COLOR_3DFACE);
    else
        hbrBk = CreateSolidBrush(infoPtr->ColorBk);

    /* get client rectangle */
    GetClientRect (infoPtr->Self, &rect);
    FrameRect( hdc, &rect, hbrBk );
    InflateRect(&rect, -1, -1);

    /* get the window style */
    dwStyle = GetWindowLongW (infoPtr->Self, GWL_STYLE);

    /* compute extent of progress bar */
    if (dwStyle & PBS_VERTICAL) {
        rightBar  = rect.bottom -
                    MulDiv (infoPtr->CurVal - infoPtr->MinVal,
	                    rect.bottom - rect.top,
	                    infoPtr->MaxVal - infoPtr->MinVal);
        ledWidth  = MulDiv (rect.right - rect.left, 2, 3);
        rightMost = rect.top;
    } else {
        rightBar = rect.left +
                   MulDiv (infoPtr->CurVal - infoPtr->MinVal,
	                   rect.right - rect.left,
	                   infoPtr->MaxVal - infoPtr->MinVal);
        ledWidth = MulDiv (rect.bottom - rect.top, 2, 3);
        rightMost = rect.right;
    }

    /* now draw the bar */
    if (dwStyle & PBS_SMOOTH)
    {
        if (dwStyle & PBS_VERTICAL)
        {
            INT old_top = rect.top;
            rect.top = rightBar;
            FillRect(hdc, &rect, hbrBar);
            rect.bottom = rect.top;
            rect.top = old_top;
            FillRect(hdc, &rect, hbrBk);
        }
        else
        {
            INT old_right = rect.right;
            rect.right = rightBar;
            FillRect(hdc, &rect, hbrBar);
            rect.left = rect.right;
            rect.right = old_right;
            FillRect(hdc, &rect, hbrBk);
        }
    } else {
        if (dwStyle & PBS_VERTICAL) {
            while(rect.bottom > rightBar) {
                rect.top = rect.bottom - ledWidth;
                if (rect.top < rightMost)
                    rect.top = rightMost;
                FillRect(hdc, &rect, hbrBar);
                rect.bottom = rect.top;
                rect.top -= LED_GAP;
                if (rect.top <= rightBar) break;
                FillRect(hdc, &rect, hbrBk);
                rect.bottom = rect.top;
            }
            rect.top = rightMost;
            FillRect(hdc, &rect, hbrBk);
        } else {
            while(rect.left < rightBar) {
                rect.right = rect.left + ledWidth;
                if (rect.right > rightMost)
                    rect.right = rightMost;
                FillRect(hdc, &rect, hbrBar);
                rect.left = rect.right;
                rect.right += LED_GAP;
                if (rect.right >= rightBar) break;
                FillRect(hdc, &rect, hbrBk);
                rect.left = rect.right;
            }
            rect.right = rightMost;
            FillRect(hdc, &rect, hbrBk);
        }
    }

    /* delete bar brush */
    if (infoPtr->ColorBar != CLR_DEFAULT) DeleteObject (hbrBar);
    if (infoPtr->ColorBk != CLR_DEFAULT) DeleteObject (hbrBk);

    return 0;
}


/***********************************************************************
 * PROGRESS_Paint
 * Draw the progress bar. The background need not be erased.
 * If dc!=0, it draws on it
 */
static LRESULT PROGRESS_Paint (PROGRESS_INFO *infoPtr, HDC hdc)
{
    PAINTSTRUCT ps;
    if (hdc) return PROGRESS_Draw (infoPtr, hdc);
    hdc = BeginPaint (infoPtr->Self, &ps);
    PROGRESS_Draw (infoPtr, hdc);
    EndPaint (infoPtr->Self, &ps);
    return 0;
}


/***********************************************************************
 *           PROGRESS_CoercePos
 * Makes sure the current position (CurVal) is within bounds.
 */
static void PROGRESS_CoercePos(PROGRESS_INFO *infoPtr)
{
    if(infoPtr->CurVal < infoPtr->MinVal)
        infoPtr->CurVal = infoPtr->MinVal;
    if(infoPtr->CurVal > infoPtr->MaxVal)
        infoPtr->CurVal = infoPtr->MaxVal;
}


/***********************************************************************
 *           PROGRESS_SetFont
 * Set new Font for progress bar
 */
static HFONT PROGRESS_SetFont (PROGRESS_INFO *infoPtr, HFONT hFont, BOOL bRedraw)
{
    HFONT hOldFont = infoPtr->Font;
    infoPtr->Font = hFont;
    /* Since infoPtr->Font is not used, there is no need for repaint */
    return hOldFont;
}

static DWORD PROGRESS_SetRange (PROGRESS_INFO *infoPtr, int low, int high)
{
    DWORD res = MAKELONG(LOWORD(infoPtr->MinVal), LOWORD(infoPtr->MaxVal));

    /* if nothing changes, simply return */
    if(infoPtr->MinVal == low && infoPtr->MaxVal == high) return res;

    infoPtr->MinVal = low;
    infoPtr->MaxVal = high;
    PROGRESS_CoercePos(infoPtr);
    InvalidateRect(infoPtr->Self, NULL, TRUE);
    return res;
}

/***********************************************************************
 *           ProgressWindowProc
 */
static LRESULT WINAPI ProgressWindowProc(HWND hwnd, UINT message,
                                  WPARAM wParam, LPARAM lParam)
{
    PROGRESS_INFO *infoPtr;

    TRACE("hwnd=%p msg=%04x wparam=%x lParam=%lx\n", hwnd, message, wParam, lParam);

    infoPtr = (PROGRESS_INFO *)GetWindowLongW(hwnd, 0);

    if (!infoPtr && message != WM_CREATE)
        return DefWindowProcW( hwnd, message, wParam, lParam );

    switch(message) {
    case WM_CREATE:
    {
	DWORD dwExStyle = GetWindowLongW (hwnd, GWL_EXSTYLE);
	dwExStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
	dwExStyle |= WS_EX_STATICEDGE;
        SetWindowLongW (hwnd, GWL_EXSTYLE, dwExStyle);
	/* Force recalculation of a non-client area */
	SetWindowPos(hwnd, 0, 0, 0, 0, 0,
	    SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        /* allocate memory for info struct */
        infoPtr = (PROGRESS_INFO *)Alloc (sizeof(PROGRESS_INFO));
        if (!infoPtr) return -1;
        SetWindowLongW (hwnd, 0, (DWORD)infoPtr);

        /* initialize the info struct */
        infoPtr->Self = hwnd;
        infoPtr->MinVal = 0;
        infoPtr->MaxVal = 100;
        infoPtr->CurVal = 0;
        infoPtr->Step = 10;
        infoPtr->ColorBar = CLR_DEFAULT;
        infoPtr->ColorBk = CLR_DEFAULT;
        infoPtr->Font = 0;
        TRACE("Progress Ctrl creation, hwnd=%p\n", hwnd);
        return 0;
    }

    case WM_DESTROY:
        TRACE("Progress Ctrl destruction, hwnd=%p\n", hwnd);
        Free (infoPtr);
        SetWindowLongW(hwnd, 0, 0);
        return 0;

    case WM_GETFONT:
        return (LRESULT)infoPtr->Font;

    case WM_SETFONT:
        return (LRESULT)PROGRESS_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_PAINT:
        return PROGRESS_Paint (infoPtr, (HDC)wParam);

    case PBM_DELTAPOS:
    {
	INT oldVal;
        oldVal = infoPtr->CurVal;
        if(wParam != 0) {
	    infoPtr->CurVal += (INT)wParam;
	    PROGRESS_CoercePos (infoPtr);
	    TRACE("PBM_DELTAPOS: current pos changed from %d to %d\n", oldVal, infoPtr->CurVal);
            PROGRESS_Invalidate( infoPtr, oldVal, infoPtr->CurVal );
        }
        return oldVal;
    }

    case PBM_SETPOS:
    {
        UINT oldVal;
        oldVal = infoPtr->CurVal;
        if(oldVal != wParam) {
	    infoPtr->CurVal = (INT)wParam;
	    PROGRESS_CoercePos(infoPtr);
	    TRACE("PBM_SETPOS: current pos changed from %d to %d\n", oldVal, infoPtr->CurVal);
            PROGRESS_Invalidate( infoPtr, oldVal, infoPtr->CurVal );
        }
        return oldVal;
    }

    case PBM_SETRANGE:
        return PROGRESS_SetRange (infoPtr, (int)LOWORD(lParam), (int)HIWORD(lParam));

    case PBM_SETSTEP:
    {
	INT oldStep;
        oldStep = infoPtr->Step;
        infoPtr->Step = (INT)wParam;
        return oldStep;
    }

    case PBM_STEPIT:
    {
	INT oldVal;
        oldVal = infoPtr->CurVal;
        infoPtr->CurVal += infoPtr->Step;
        if(infoPtr->CurVal > infoPtr->MaxVal)
	    infoPtr->CurVal = infoPtr->MinVal;
        if(oldVal != infoPtr->CurVal)
	{
	    TRACE("PBM_STEPIT: current pos changed from %d to %d\n", oldVal, infoPtr->CurVal);
            PROGRESS_Invalidate( infoPtr, oldVal, infoPtr->CurVal );
	}
        return oldVal;
    }

    case PBM_SETRANGE32:
        return PROGRESS_SetRange (infoPtr, (int)wParam, (int)lParam);

    case PBM_GETRANGE:
        if (lParam) {
            ((PPBRANGE)lParam)->iLow = infoPtr->MinVal;
            ((PPBRANGE)lParam)->iHigh = infoPtr->MaxVal;
        }
        return wParam ? infoPtr->MinVal : infoPtr->MaxVal;

    case PBM_GETPOS:
        return infoPtr->CurVal;

    case PBM_SETBARCOLOR:
        infoPtr->ColorBar = (COLORREF)lParam;
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;

    case PBM_SETBKCOLOR:
        infoPtr->ColorBk = (COLORREF)lParam;
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;

    default:
        if ((message >= WM_USER) && (message < WM_APP))
	    ERR("unknown msg %04x wp=%04x lp=%08lx\n", message, wParam, lParam );
        return DefWindowProcW( hwnd, message, wParam, lParam );
    }
}


/***********************************************************************
 * PROGRESS_Register [Internal]
 *
 * Registers the progress bar window class.
 */
VOID PROGRESS_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(wndClass));
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC)ProgressWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof (PROGRESS_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.lpszClassName = PROGRESS_CLASSW;

    RegisterClassW (&wndClass);
}


/***********************************************************************
 * PROGRESS_Unregister [Internal]
 *
 * Unregisters the progress bar window class.
 */
VOID PROGRESS_Unregister (void)
{
    UnregisterClassW (PROGRESS_CLASSW, NULL);
}
