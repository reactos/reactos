/*
 * Pager control
 *
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
 * NOTES
 *    Tested primarily with the controlspy Pager application.
 *       Susan Farley (susan@codeweavers.com)
 *
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 18, 2004, by Robert Shearman.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features or bugs please note them below.
 *
 * TODO:
 *    Implement repetitive button press.
 *    Adjust arrow size relative to size of button.
 *    Allow border size changes.
 *    Styles:
 *      PGS_DRAGNDROP
 *    Notifications:
 *      PGN_HOTITEMCHANGE
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

WINE_DEFAULT_DEBUG_CHANNEL(pager);

typedef struct
{
    HWND   hwndChild;  /* handle of the contained wnd */
    HWND   hwndNotify; /* handle of the parent wnd */
    BOOL   bNoResize;  /* set when created with CCS_NORESIZE */
    COLORREF clrBk;    /* background color */
    INT    nBorder;    /* border size for the control */
    INT    nButtonSize;/* size of the pager btns */
    INT    nPos;       /* scroll position */
    INT    nWidth;     /* from child wnd's response to PGN_CALCSIZE */
    INT    nHeight;    /* from child wnd's response to PGN_CALCSIZE */
    BOOL   bForward;   /* forward WM_MOUSEMOVE msgs to the contained wnd */
    BOOL   bCapture;   /* we have captured the mouse  */
    INT    TLbtnState; /* state of top or left btn */
    INT    BRbtnState; /* state of bottom or right btn */
    INT    direction;  /* direction of the scroll, (e.g. PGF_SCROLLUP) */
} PAGER_INFO;

#define PAGER_GetInfoPtr(hwnd) ((PAGER_INFO *)GetWindowLongPtrW(hwnd, 0))
#define PAGER_IsHorizontal(hwnd) ((GetWindowLongA (hwnd, GWL_STYLE) & PGS_HORZ))

#define MIN_ARROW_WIDTH  8
#define MIN_ARROW_HEIGHT 5

#define TIMERID1         1
#define TIMERID2         2
#define INITIAL_DELAY    500
#define REPEAT_DELAY     50

static void
PAGER_GetButtonRects(HWND hwnd, PAGER_INFO* infoPtr, RECT* prcTopLeft, RECT* prcBottomRight, BOOL bClientCoords)
{
    RECT rcWindow;
    GetWindowRect (hwnd, &rcWindow);

    if (bClientCoords)
    {
        POINT pt = {rcWindow.left, rcWindow.top};
        ScreenToClient(hwnd, &pt);
        OffsetRect(&rcWindow, -(rcWindow.left-pt.x), -(rcWindow.top-pt.y));
    }
    else
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

    *prcTopLeft = *prcBottomRight = rcWindow;
    if (PAGER_IsHorizontal(hwnd))
    {
        prcTopLeft->right = prcTopLeft->left + infoPtr->nButtonSize;
        prcBottomRight->left = prcBottomRight->right - infoPtr->nButtonSize;
    }
    else
    {
        prcTopLeft->bottom = prcTopLeft->top + infoPtr->nButtonSize;
        prcBottomRight->top = prcBottomRight->bottom - infoPtr->nButtonSize;
    }
}

/* the horizontal arrows are:
 *
 * 01234    01234
 * 1  *      *
 * 2 **      **
 * 3***      ***
 * 4***      ***
 * 5 **      **
 * 6  *      *
 * 7
 *
 */
static void
PAGER_DrawHorzArrow (HDC hdc, RECT r, INT colorRef, BOOL left)
{
    INT x, y, w, h;
    HPEN hPen, hOldPen;

    w = r.right - r.left + 1;
    h = r.bottom - r.top + 1;
    if ((h < MIN_ARROW_WIDTH) || (w < MIN_ARROW_HEIGHT))
        return;  /* refuse to draw partial arrow */

    if (!(hPen = CreatePen( PS_SOLID, 1, GetSysColor( colorRef )))) return;
    hOldPen = SelectObject ( hdc, hPen );
    if (left)
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 3;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 1;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x--, y+5); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x--, y+3); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x, y+1);
    }
    else
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 1;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 1;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x++, y+5); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x++, y+3); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x, y+1);
    }

    SelectObject( hdc, hOldPen );
    DeleteObject( hPen );
}

/* the vertical arrows are:
 *
 * 01234567    01234567
 * 1******        **
 * 2 ****        ****
 * 3  **        ******
 * 4
 *
 */
static void
PAGER_DrawVertArrow (HDC hdc, RECT r, INT colorRef, BOOL up)
{
    INT x, y, w, h;
    HPEN hPen, hOldPen;

    w = r.right - r.left + 1;
    h = r.bottom - r.top + 1;
    if ((h < MIN_ARROW_WIDTH) || (w < MIN_ARROW_HEIGHT))
        return;  /* refuse to draw partial arrow */

    if (!(hPen = CreatePen( PS_SOLID, 1, GetSysColor( colorRef )))) return;
    hOldPen = SelectObject ( hdc, hPen );
    if (up)
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 1;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 3;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+5, y--); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+3, y--); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+1, y);
    }
    else
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 1;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 1;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+5, y++); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+3, y++); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+1, y);
    }

    SelectObject( hdc, hOldPen );
    DeleteObject( hPen );
}

static void
PAGER_DrawButton(HDC hdc, COLORREF clrBk, RECT arrowRect,
                 BOOL horz, BOOL topLeft, INT btnState)
{
    HBRUSH   hBrush, hOldBrush;
    RECT     rc = arrowRect;

    TRACE("arrowRect = %s, btnState = %d\n", wine_dbgstr_rect(&arrowRect), btnState);

    if (btnState == PGF_INVISIBLE)
        return;

    if ((rc.right - rc.left <= 0) || (rc.bottom - rc.top <= 0))
        return;

    hBrush = CreateSolidBrush(clrBk);
    hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    FillRect(hdc, &rc, hBrush);

    if (btnState == PGF_HOT)
    {
       DrawEdge( hdc, &rc, BDR_RAISEDINNER, BF_RECT);
       if (horz)
           PAGER_DrawHorzArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       else
           PAGER_DrawVertArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
    }
    else if (btnState == PGF_NORMAL)
    {
       DrawEdge (hdc, &rc, BDR_OUTER, BF_FLAT);
       if (horz)
           PAGER_DrawHorzArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       else
           PAGER_DrawVertArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
    }
    else if (btnState == PGF_DEPRESSED)
    {
       DrawEdge( hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
       if (horz)
           PAGER_DrawHorzArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       else
           PAGER_DrawVertArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
    }
    else if (btnState == PGF_GRAYED)
    {
       DrawEdge (hdc, &rc, BDR_OUTER, BF_FLAT);
       if (horz)
       {
           PAGER_DrawHorzArrow(hdc, rc, COLOR_3DHIGHLIGHT, topLeft);
           rc.left++, rc.top++; rc.right++, rc.bottom++;
           PAGER_DrawHorzArrow(hdc, rc, COLOR_3DSHADOW, topLeft);
       }
       else
       {
           PAGER_DrawVertArrow(hdc, rc, COLOR_3DHIGHLIGHT, topLeft);
           rc.left++, rc.top++; rc.right++, rc.bottom++;
           PAGER_DrawVertArrow(hdc, rc, COLOR_3DSHADOW, topLeft);
       }
    }

    SelectObject( hdc, hOldBrush );
    DeleteObject(hBrush);
}

/* << PAGER_GetDropTarget >> */

static inline LRESULT
PAGER_ForwardMouse (HWND hwnd, WPARAM wParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%p]\n", hwnd);

    infoPtr->bForward = (BOOL)wParam;

    return 0;
}

static inline LRESULT
PAGER_GetButtonState (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    LRESULT btnState = PGF_INVISIBLE;
    INT btn = (INT)lParam;
    TRACE("[%p]\n", hwnd);

    if (btn == PGB_TOPORLEFT)
        btnState = infoPtr->TLbtnState;
    else if (btn == PGB_BOTTOMORRIGHT)
        btnState = infoPtr->BRbtnState;

    return btnState;
}


static inline LRESULT
PAGER_GetPos(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%p] returns %d\n", hwnd, infoPtr->nPos);
    return (LRESULT)infoPtr->nPos;
}

static inline LRESULT
PAGER_GetButtonSize(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%p] returns %d\n", hwnd, infoPtr->nButtonSize);
    return (LRESULT)infoPtr->nButtonSize;
}

static inline LRESULT
PAGER_GetBorder(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%p] returns %d\n", hwnd, infoPtr->nBorder);
    return (LRESULT)infoPtr->nBorder;
}

static inline LRESULT
PAGER_GetBkColor(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%p] returns %06lx\n", hwnd, infoPtr->clrBk);
    return (LRESULT)infoPtr->clrBk;
}

static void
PAGER_CalcSize (HWND hwnd, INT* size, BOOL getWidth)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    NMPGCALCSIZE nmpgcs;
    ZeroMemory (&nmpgcs, sizeof (NMPGCALCSIZE));
    nmpgcs.hdr.hwndFrom = hwnd;
    nmpgcs.hdr.idFrom   = GetWindowLongPtrW (hwnd, GWLP_ID);
    nmpgcs.hdr.code = PGN_CALCSIZE;
    nmpgcs.dwFlag = getWidth ? PGF_CALCWIDTH : PGF_CALCHEIGHT;
    nmpgcs.iWidth = getWidth ? *size : 0;
    nmpgcs.iHeight = getWidth ? 0 : *size;
    SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
                  (WPARAM)nmpgcs.hdr.idFrom, (LPARAM)&nmpgcs);

    *size = getWidth ? nmpgcs.iWidth : nmpgcs.iHeight;

    TRACE("[%p] PGN_CALCSIZE returns %s=%d\n", hwnd,
                  getWidth ? "width" : "height", *size);
}

static void
PAGER_PositionChildWnd(HWND hwnd, PAGER_INFO* infoPtr)
{
    if (infoPtr->hwndChild)
    {
        RECT rcClient;
        int nPos = infoPtr->nPos;

        /* compensate for a grayed btn, which will soon become invisible */
        if (infoPtr->TLbtnState == PGF_GRAYED)
            nPos += infoPtr->nButtonSize;

        GetClientRect(hwnd, &rcClient);

        if (PAGER_IsHorizontal(hwnd))
        {
            int wndSize = max(0, rcClient.right - rcClient.left);
            if (infoPtr->nWidth < wndSize)
                infoPtr->nWidth = wndSize;

            TRACE("[%p] SWP %dx%d at (%d,%d)\n", hwnd,
                         infoPtr->nWidth, infoPtr->nHeight,
                         -nPos, 0);
            SetWindowPos(infoPtr->hwndChild, 0,
                         -nPos, 0,
                         infoPtr->nWidth, infoPtr->nHeight,
                         SWP_NOZORDER);
        }
        else
        {
            int wndSize = max(0, rcClient.bottom - rcClient.top);
            if (infoPtr->nHeight < wndSize)
                infoPtr->nHeight = wndSize;

            TRACE("[%p] SWP %dx%d at (%d,%d)\n", hwnd,
                         infoPtr->nWidth, infoPtr->nHeight,
                         0, -nPos);
            SetWindowPos(infoPtr->hwndChild, 0,
                         0, -nPos,
                         infoPtr->nWidth, infoPtr->nHeight,
                         SWP_NOZORDER);
        }

        InvalidateRect(infoPtr->hwndChild, NULL, TRUE);
    }
}

static INT
PAGER_GetScrollRange(HWND hwnd, PAGER_INFO* infoPtr)
{
    INT scrollRange = 0;

    if (infoPtr->hwndChild)
    {
        INT wndSize, childSize;
        RECT wndRect;
        GetWindowRect(hwnd, &wndRect);

        if (PAGER_IsHorizontal(hwnd))
        {
            wndSize = wndRect.right - wndRect.left;
            PAGER_CalcSize(hwnd, &infoPtr->nWidth, TRUE);
            childSize = infoPtr->nWidth;
        }
        else
        {
            wndSize = wndRect.bottom - wndRect.top;
            PAGER_CalcSize(hwnd, &infoPtr->nHeight, FALSE);
            childSize = infoPtr->nHeight;
        }

        TRACE("childSize = %d,  wndSize = %d\n", childSize, wndSize);
        if (childSize > wndSize)
            scrollRange = childSize - wndSize + infoPtr->nButtonSize;
    }

    TRACE("[%p] returns %d\n", hwnd, scrollRange);
    return scrollRange;
}

static void
PAGER_UpdateBtns(HWND hwnd, PAGER_INFO *infoPtr,
                 INT scrollRange, BOOL hideGrayBtns)
{
    BOOL resizeClient;
    BOOL repaintBtns;
    INT oldTLbtnState = infoPtr->TLbtnState;
    INT oldBRbtnState = infoPtr->BRbtnState;
    POINT pt;
    RECT rcTopLeft, rcBottomRight;

    /* get button rects */
    PAGER_GetButtonRects(hwnd, infoPtr, &rcTopLeft, &rcBottomRight, FALSE);

    GetCursorPos(&pt);

    /* update states based on scroll position */
    if (infoPtr->nPos > 0)
    {
        if (infoPtr->TLbtnState == PGF_INVISIBLE || infoPtr->TLbtnState == PGF_GRAYED)
            infoPtr->TLbtnState = PGF_NORMAL;
    }
    else if (PtInRect(&rcTopLeft, pt))
        infoPtr->TLbtnState = PGF_GRAYED;
    else
        infoPtr->TLbtnState = PGF_INVISIBLE;

    if (scrollRange <= 0)
    {
        infoPtr->TLbtnState = PGF_INVISIBLE;
        infoPtr->BRbtnState = PGF_INVISIBLE;
    }
    else if (infoPtr->nPos < scrollRange)
    {
        if (infoPtr->BRbtnState == PGF_INVISIBLE || infoPtr->BRbtnState == PGF_GRAYED)
            infoPtr->BRbtnState = PGF_NORMAL;
    }
    else if (PtInRect(&rcBottomRight, pt))
        infoPtr->BRbtnState = PGF_GRAYED;
    else
        infoPtr->BRbtnState = PGF_INVISIBLE;

    /* only need to resize when entering or leaving PGF_INVISIBLE state */
    resizeClient =
        ((oldTLbtnState == PGF_INVISIBLE) != (infoPtr->TLbtnState == PGF_INVISIBLE)) ||
        ((oldBRbtnState == PGF_INVISIBLE) != (infoPtr->BRbtnState == PGF_INVISIBLE));
    /* initiate NCCalcSize to resize client wnd if necessary */
    if (resizeClient)
        SetWindowPos(hwnd, 0,0,0,0,0,
                     SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                     SWP_NOZORDER | SWP_NOACTIVATE);

    /* repaint when changing any state */
    repaintBtns = (oldTLbtnState != infoPtr->TLbtnState) || 
                  (oldBRbtnState != infoPtr->BRbtnState);
    if (repaintBtns)
        SendMessageW(hwnd, WM_NCPAINT, 0, 0);
}

static LRESULT
PAGER_SetPos(HWND hwnd, INT newPos, BOOL fromBtnPress)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT scrollRange = PAGER_GetScrollRange(hwnd, infoPtr);
    INT oldPos = infoPtr->nPos;

    if ((scrollRange <= 0) || (newPos < 0))
        infoPtr->nPos = 0;
    else if (newPos > scrollRange)
        infoPtr->nPos = scrollRange;
    else
        infoPtr->nPos = newPos;

    TRACE("[%p] pos=%d, oldpos=%d\n", hwnd, infoPtr->nPos, oldPos);

    if (infoPtr->nPos != oldPos)
    {
        /* gray and restore btns, and if from WM_SETPOS, hide the gray btns */
        PAGER_UpdateBtns(hwnd, infoPtr, scrollRange, !fromBtnPress);
        PAGER_PositionChildWnd(hwnd, infoPtr);
    }

    return 0;
}

static LRESULT
PAGER_HandleWindowPosChanging(HWND hwnd, WPARAM wParam, WINDOWPOS *winpos)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    if (infoPtr->bNoResize && !(winpos->flags & SWP_NOSIZE))
    {
        /* don't let the app resize the nonscrollable dimension of a control
         * that was created with CCS_NORESIZE style
         * (i.e. height for a horizontal pager, or width for a vertical one) */

	/* except if the current dimension is 0 and app is setting for
	 * first time, then save amount as dimension. - GA 8/01 */

        if (PAGER_IsHorizontal(hwnd))
	    if (!infoPtr->nHeight && winpos->cy)
		infoPtr->nHeight = winpos->cy;
	    else
		winpos->cy = infoPtr->nHeight;
        else
	    if (!infoPtr->nWidth && winpos->cx)
		infoPtr->nWidth = winpos->cx;
	    else
		winpos->cx = infoPtr->nWidth;
	return 0;
    }

    DefWindowProcW (hwnd, WM_WINDOWPOSCHANGING, wParam, (LPARAM)winpos);

    return 1;
}

static INT
PAGER_SetFixedWidth(HWND hwnd, PAGER_INFO* infoPtr)
{
  /* Must set the non-scrollable dimension to be less than the full height/width
   * so that NCCalcSize is called.  The Msoft docs mention 3/4 factor for button
   * size, and experimentation shows that affect is almost right. */

    RECT wndRect;
    INT delta, h;
    GetWindowRect(hwnd, &wndRect);

    /* see what the app says for btn width */
    PAGER_CalcSize(hwnd, &infoPtr->nWidth, TRUE);

    if (infoPtr->bNoResize)
    {
        delta = wndRect.right - wndRect.left - infoPtr->nWidth;
        if (delta > infoPtr->nButtonSize)
            infoPtr->nWidth += 4 * infoPtr->nButtonSize / 3;
        else if (delta > 0)
            infoPtr->nWidth +=  infoPtr->nButtonSize / 3;
    }

    h = wndRect.bottom - wndRect.top + infoPtr->nButtonSize;

    TRACE("[%p] infoPtr->nWidth set to %d\n",
	       hwnd, infoPtr->nWidth);

    return h;
}

static INT
PAGER_SetFixedHeight(HWND hwnd, PAGER_INFO* infoPtr)
{
  /* Must set the non-scrollable dimension to be less than the full height/width
   * so that NCCalcSize is called.  The Msoft docs mention 3/4 factor for button
   * size, and experimentation shows that affect is almost right. */

    RECT wndRect;
    INT delta, w;
    GetWindowRect(hwnd, &wndRect);

    /* see what the app says for btn height */
    PAGER_CalcSize(hwnd, &infoPtr->nHeight, FALSE);

    if (infoPtr->bNoResize)
    {
        delta = wndRect.bottom - wndRect.top - infoPtr->nHeight;
        if (delta > infoPtr->nButtonSize)
            infoPtr->nHeight += infoPtr->nButtonSize;
        else if (delta > 0)
            infoPtr->nHeight +=  infoPtr->nButtonSize / 3;
    }

    w = wndRect.right - wndRect.left + infoPtr->nButtonSize;

    TRACE("[%p] infoPtr->nHeight set to %d\n",
	       hwnd, infoPtr->nHeight);

    return w;
}

/******************************************************************
 * For the PGM_RECALCSIZE message (but not the other uses in      *
 * this module), the native control does only the following:      *
 *                                                                *
 *    if (some condition)                                         *
 *          PostMessageW(hwnd, EM_FMTLINES, 0, 0);                *
 *    return DefWindowProcW(hwnd, PGM_RECALCSIZE, 0, 0);          *
 *                                                                *
 * When we figure out what the "some condition" is we will        *
 * implement that for the message processing.                     *
 ******************************************************************/

static LRESULT
PAGER_RecalcSize(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    TRACE("[%p]\n", hwnd);

    if (infoPtr->hwndChild)
    {
        INT scrollRange = PAGER_GetScrollRange(hwnd, infoPtr);

        if (scrollRange <= 0)
        {
            infoPtr->nPos = -1;
            PAGER_SetPos(hwnd, 0, FALSE);
        }
        else
            PAGER_PositionChildWnd(hwnd, infoPtr);
    }

    return 1;
}


static LRESULT
PAGER_SetBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    COLORREF clrTemp = infoPtr->clrBk;

    infoPtr->clrBk = (COLORREF)lParam;
    TRACE("[%p] %06lx\n", hwnd, infoPtr->clrBk);

    /* the native control seems to do things this way */
    SetWindowPos(hwnd, 0,0,0,0,0,
		 SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
		 SWP_NOZORDER | SWP_NOACTIVATE);

    RedrawWindow(hwnd, 0, 0, RDW_ERASE | RDW_INVALIDATE);

    return (LRESULT)clrTemp;
}


static LRESULT
PAGER_SetBorder (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nBorder;

    infoPtr->nBorder = (INT)lParam;
    TRACE("[%p] %d\n", hwnd, infoPtr->nBorder);

    PAGER_RecalcSize(hwnd);

    return (LRESULT)nTemp;
}


static LRESULT
PAGER_SetButtonSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nButtonSize;

    infoPtr->nButtonSize = (INT)lParam;
    TRACE("[%p] %d\n", hwnd, infoPtr->nButtonSize);

    PAGER_RecalcSize(hwnd);

    return (LRESULT)nTemp;
}


static LRESULT
PAGER_SetChild (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT hw;

    infoPtr->hwndChild = IsWindow ((HWND)lParam) ? (HWND)lParam : 0;

    if (infoPtr->hwndChild)
    {
        TRACE("[%p] hwndChild=%p\n", hwnd, infoPtr->hwndChild);

        if (PAGER_IsHorizontal(hwnd)) {
            hw = PAGER_SetFixedHeight(hwnd, infoPtr);
	    /* adjust non-scrollable dimension to fit the child */
	    SetWindowPos(hwnd, 0, 0,0, hw, infoPtr->nHeight,
			 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER |
			 SWP_NOSIZE | SWP_NOACTIVATE);
	}
        else {
            hw = PAGER_SetFixedWidth(hwnd, infoPtr);
	    /* adjust non-scrollable dimension to fit the child */
	    SetWindowPos(hwnd, 0, 0,0, infoPtr->nWidth, hw,
			 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER |
			 SWP_NOSIZE | SWP_NOACTIVATE);
	}

        /* position child within the page scroller */
        SetWindowPos(infoPtr->hwndChild, HWND_TOP,
                     0,0,0,0,
                     SWP_SHOWWINDOW | SWP_NOSIZE);  /* native is 0 */

        infoPtr->nPos = -1;
        PAGER_SetPos(hwnd, 0, FALSE);
    }

    return 0;
}

static void
PAGER_Scroll(HWND hwnd, INT dir)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    NMPGSCROLL nmpgScroll;
    RECT rcWnd;

    if (infoPtr->hwndChild)
    {
        ZeroMemory (&nmpgScroll, sizeof (NMPGSCROLL));
        nmpgScroll.hdr.hwndFrom = hwnd;
        nmpgScroll.hdr.idFrom   = GetWindowLongPtrW (hwnd, GWLP_ID);
        nmpgScroll.hdr.code = PGN_SCROLL;

        GetWindowRect(hwnd, &rcWnd);
        GetClientRect(hwnd, &nmpgScroll.rcParent);
        nmpgScroll.iXpos = nmpgScroll.iYpos = 0;
        nmpgScroll.iDir = dir;

        if (PAGER_IsHorizontal(hwnd))
        {
            nmpgScroll.iScroll = rcWnd.right - rcWnd.left;
            nmpgScroll.iXpos = infoPtr->nPos;
        }
        else
        {
            nmpgScroll.iScroll = rcWnd.bottom - rcWnd.top;
            nmpgScroll.iYpos = infoPtr->nPos;
        }
        nmpgScroll.iScroll -= 2*infoPtr->nButtonSize;

        SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
                    (WPARAM)nmpgScroll.hdr.idFrom, (LPARAM)&nmpgScroll);

        TRACE("[%p] PGN_SCROLL returns iScroll=%d\n", hwnd, nmpgScroll.iScroll);

        if (nmpgScroll.iScroll > 0)
        {
            infoPtr->direction = dir;

            if (dir == PGF_SCROLLLEFT || dir == PGF_SCROLLUP)
                PAGER_SetPos(hwnd, infoPtr->nPos - nmpgScroll.iScroll, TRUE);
            else
                PAGER_SetPos(hwnd, infoPtr->nPos + nmpgScroll.iScroll, TRUE);
        }
        else
            infoPtr->direction = -1;
    }
}

static LRESULT
PAGER_FmtLines(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    /* initiate NCCalcSize to resize client wnd and get size */
    SetWindowPos(hwnd, 0, 0,0,0,0,
		 SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
		 SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(infoPtr->hwndChild, 0,
		 0,0,infoPtr->nWidth,infoPtr->nHeight,
		 0);

    return DefWindowProcW (hwnd, EM_FMTLINES, 0, 0);
}

static LRESULT
PAGER_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    /* allocate memory for info structure */
    infoPtr = (PAGER_INFO *)Alloc (sizeof(PAGER_INFO));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    /* set default settings */
    infoPtr->hwndChild = NULL;
    infoPtr->hwndNotify = ((LPCREATESTRUCTW)lParam)->hwndParent;
    infoPtr->bNoResize = dwStyle & CCS_NORESIZE;
    infoPtr->clrBk = GetSysColor(COLOR_BTNFACE);
    infoPtr->nBorder = 0;
    infoPtr->nButtonSize = 12;
    infoPtr->nPos = 0;
    infoPtr->nWidth = 0;
    infoPtr->nHeight = 0;
    infoPtr->bForward = FALSE;
    infoPtr->bCapture = FALSE;
    infoPtr->TLbtnState = PGF_INVISIBLE;
    infoPtr->BRbtnState = PGF_INVISIBLE;
    infoPtr->direction = -1;

    if (dwStyle & PGS_DRAGNDROP)
        FIXME("[%p] Drag and Drop style is not implemented yet.\n", hwnd);
    /*
	 * If neither horizontal nor vertical style specified, default to vertical.
	 * This is probably not necessary, since the style may be set later on as
	 * the control is initialized, but just in case it isn't, set it here.
	 */
    if (!(dwStyle & PGS_HORZ) && !(dwStyle & PGS_VERT))
    {
        dwStyle |= PGS_VERT;
        SetWindowLongA(hwnd, GWL_STYLE, dwStyle);
    }

    return 0;
}


static LRESULT
PAGER_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    /* free pager info data */
    Free (infoPtr);
    SetWindowLongPtrW (hwnd, 0, 0);
    return 0;
}

static LRESULT
PAGER_NCCalcSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    LPRECT lpRect = (LPRECT)lParam;
    RECT rcChild, rcWindow;
    INT scrollRange;

    /*
     * lParam points to a RECT struct.  On entry, the struct
     * contains the proposed wnd rectangle for the window.
     * On exit, the struct should contain the screen
     * coordinates of the corresponding window's client area.
     */

    DefWindowProcW (hwnd, WM_NCCALCSIZE, wParam, lParam);

    TRACE("orig rect=%s\n", wine_dbgstr_rect(lpRect));

    GetWindowRect (infoPtr->hwndChild, &rcChild);
    MapWindowPoints (0, hwnd, (LPPOINT)&rcChild, 2); /* FIXME: RECT != 2 POINTS */
    GetWindowRect (hwnd, &rcWindow);

    if (PAGER_IsHorizontal(hwnd))
    {
	infoPtr->nWidth = lpRect->right - lpRect->left;
	PAGER_CalcSize (hwnd, &infoPtr->nWidth, TRUE);

	scrollRange = infoPtr->nWidth - (rcWindow.right - rcWindow.left);

	if (infoPtr->TLbtnState && (lpRect->left + infoPtr->nButtonSize < lpRect->right))
	    lpRect->left += infoPtr->nButtonSize;
	if (infoPtr->BRbtnState && (lpRect->right - infoPtr->nButtonSize > lpRect->left))
	    lpRect->right -= infoPtr->nButtonSize;
    }
    else
    {
	infoPtr->nHeight = lpRect->bottom - lpRect->top;
	PAGER_CalcSize (hwnd, &infoPtr->nHeight, FALSE);

	scrollRange = infoPtr->nHeight - (rcWindow.bottom - rcWindow.top);

	if (infoPtr->TLbtnState && (lpRect->top + infoPtr->nButtonSize < lpRect->bottom))
	    lpRect->top += infoPtr->nButtonSize;
	if (infoPtr->BRbtnState && (lpRect->bottom - infoPtr->nButtonSize > lpRect->top))
	    lpRect->bottom -= infoPtr->nButtonSize;
    }

    TRACE("nPos=%d, nHeigth=%d, window=%s\n",
          infoPtr->nPos, infoPtr->nHeight,
          wine_dbgstr_rect(&rcWindow));

    TRACE("[%p] client rect set to %ldx%ld at (%ld,%ld) BtnState[%d,%d]\n",
	  hwnd, lpRect->right-lpRect->left, lpRect->bottom-lpRect->top,
	  lpRect->left, lpRect->top,
	  infoPtr->TLbtnState, infoPtr->BRbtnState);

    return 0;
}

static LRESULT
PAGER_NCPaint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO* infoPtr = PAGER_GetInfoPtr(hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT rcBottomRight, rcTopLeft;
    HDC hdc;
    BOOL bHorizontal = PAGER_IsHorizontal(hwnd);

    if (dwStyle & WS_MINIMIZE)
        return 0;

    DefWindowProcW (hwnd, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx (hwnd, 0, DCX_USESTYLE | DCX_WINDOW)))
        return 0;

    PAGER_GetButtonRects(hwnd, infoPtr, &rcTopLeft, &rcBottomRight, FALSE);

    PAGER_DrawButton(hdc, infoPtr->clrBk, rcTopLeft,
                     bHorizontal, TRUE, infoPtr->TLbtnState);
    PAGER_DrawButton(hdc, infoPtr->clrBk, rcBottomRight,
                     bHorizontal, FALSE, infoPtr->BRbtnState);

    ReleaseDC( hwnd, hdc );
    return 0;
}

static INT
PAGER_HitTest (HWND hwnd, const POINT * pt)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    RECT clientRect, rcTopLeft, rcBottomRight;
    POINT ptWindow;

    GetClientRect (hwnd, &clientRect);

    if (PtInRect(&clientRect, *pt))
    {
        TRACE("child\n");
        return -1;
    }

    ptWindow = *pt;
    PAGER_GetButtonRects(hwnd, infoPtr, &rcTopLeft, &rcBottomRight, TRUE);

    if ((infoPtr->TLbtnState != PGF_INVISIBLE) && PtInRect(&rcTopLeft, ptWindow))
    {
        TRACE("PGB_TOPORLEFT\n");
        return PGB_TOPORLEFT;
    }
    else if ((infoPtr->BRbtnState != PGF_INVISIBLE) && PtInRect(&rcBottomRight, ptWindow))
    {
        TRACE("PGB_BOTTOMORRIGHT\n");
        return PGB_BOTTOMORRIGHT;
    }

    TRACE("nowhere\n");
    return -1;
}

static LRESULT
PAGER_NCHitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    INT nHit;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    ScreenToClient (hwnd, &pt);
    nHit = PAGER_HitTest(hwnd, &pt);
    if (nHit < 0)
        return HTTRANSPARENT;
    return HTCLIENT;
}

static LRESULT
PAGER_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    POINT clpt, pt;
    RECT wnrect, *btnrect = NULL;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    BOOL topLeft = FALSE;
    INT btnstate = 0;
    INT hit;
    HDC hdc;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    TRACE("[%p] to (%ld,%ld)\n", hwnd, pt.x, pt.y);
    ClientToScreen(hwnd, &pt);
    GetWindowRect(hwnd, &wnrect);
    if (PtInRect(&wnrect, pt)) {
        RECT TLbtnrect, BRbtnrect;
        PAGER_GetButtonRects(hwnd, infoPtr, &TLbtnrect, &BRbtnrect, FALSE);

	clpt = pt;
	MapWindowPoints(0, hwnd, &clpt, 1);
	hit = PAGER_HitTest(hwnd, &clpt);
	if ((hit == PGB_TOPORLEFT) && (infoPtr->TLbtnState == PGF_NORMAL))
	{
	    topLeft = TRUE;
	    btnrect = &TLbtnrect;
	    infoPtr->TLbtnState = PGF_HOT;
	    btnstate = infoPtr->TLbtnState;
	}
	else if ((hit == PGB_BOTTOMORRIGHT) && (infoPtr->BRbtnState == PGF_NORMAL))
	{
	    topLeft = FALSE;
	    btnrect = &BRbtnrect;
	    infoPtr->BRbtnState = PGF_HOT;
	    btnstate = infoPtr->BRbtnState;
	}

	/* If in one of the buttons the capture and draw buttons */
	if (btnrect)
	{
	    TRACE("[%p] draw btn (%ld,%ld)-(%ld,%ld), Capture %s, style %08lx\n",
		  hwnd, btnrect->left, btnrect->top,
		  btnrect->right, btnrect->bottom,
		  (infoPtr->bCapture) ? "TRUE" : "FALSE",
		  dwStyle);
	    if (!infoPtr->bCapture)
	    {
	        TRACE("[%p] SetCapture\n", hwnd);
	        SetCapture(hwnd);
	        infoPtr->bCapture = TRUE;
	    }
	    if (dwStyle & PGS_AUTOSCROLL)
		SetTimer(hwnd, TIMERID1, 0x3e, 0);
	    hdc = GetWindowDC(hwnd);
	    /* OffsetRect(wnrect, 0 | 1, 0 | 1) */
	    PAGER_DrawButton(hdc, infoPtr->clrBk, *btnrect,
			     PAGER_IsHorizontal(hwnd), topLeft, btnstate);
	    ReleaseDC(hwnd, hdc);
	    return 0;
	}
    }

    /* If we think we are captured, then do release */
    if (infoPtr->bCapture && (WindowFromPoint(pt) != hwnd))
    {
    	NMHDR nmhdr;

        infoPtr->bCapture = FALSE;

        if (GetCapture() == hwnd)
        {
            ReleaseCapture();

            if (infoPtr->TLbtnState == PGF_GRAYED)
            {
                infoPtr->TLbtnState = PGF_INVISIBLE;
                SetWindowPos(hwnd, 0,0,0,0,0,
                             SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            else if (infoPtr->TLbtnState == PGF_HOT)
            {
        	infoPtr->TLbtnState = PGF_NORMAL;
        	/* FIXME: just invalidate button rect */
                RedrawWindow(hwnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }

            if (infoPtr->BRbtnState == PGF_GRAYED)
            {
                infoPtr->BRbtnState = PGF_INVISIBLE;
                SetWindowPos(hwnd, 0,0,0,0,0,
                             SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            else if (infoPtr->BRbtnState == PGF_HOT)
            {
        	infoPtr->BRbtnState = PGF_NORMAL;
        	/* FIXME: just invalidate button rect */
                RedrawWindow(hwnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }

            /* Notify parent of released mouse capture */
        	memset(&nmhdr, 0, sizeof(NMHDR));
        	nmhdr.hwndFrom = hwnd;
        	nmhdr.idFrom   = GetWindowLongPtrW(hwnd, GWLP_ID);
        	nmhdr.code = NM_RELEASEDCAPTURE;
        	SendMessageW(infoPtr->hwndNotify, WM_NOTIFY,
                         (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
        }
        if (IsWindow(hwnd))
            KillTimer(hwnd, TIMERID1);
    }
    return 0;
}

static LRESULT
PAGER_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    BOOL repaintBtns = FALSE;
    POINT pt;
    INT hit;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    TRACE("[%p] at (%d,%d)\n", hwnd, (short)LOWORD(lParam), (short)HIWORD(lParam));

    hit = PAGER_HitTest(hwnd, &pt);

    /* put btn in DEPRESSED state */
    if (hit == PGB_TOPORLEFT)
    {
        repaintBtns = infoPtr->TLbtnState != PGF_DEPRESSED;
        infoPtr->TLbtnState = PGF_DEPRESSED;
        SetTimer(hwnd, TIMERID1, INITIAL_DELAY, 0);
    }
    else if (hit == PGB_BOTTOMORRIGHT)
    {
        repaintBtns = infoPtr->BRbtnState != PGF_DEPRESSED;
        infoPtr->BRbtnState = PGF_DEPRESSED;
        SetTimer(hwnd, TIMERID1, INITIAL_DELAY, 0);
    }

    if (repaintBtns)
        SendMessageW(hwnd, WM_NCPAINT, 0, 0);

    switch(hit)
    {
    case PGB_TOPORLEFT:
        if (PAGER_IsHorizontal(hwnd))
        {
            TRACE("[%p] PGF_SCROLLLEFT\n", hwnd);
            PAGER_Scroll(hwnd, PGF_SCROLLLEFT);
        }
        else
        {
            TRACE("[%p] PGF_SCROLLUP\n", hwnd);
            PAGER_Scroll(hwnd, PGF_SCROLLUP);
        }
        break;
    case PGB_BOTTOMORRIGHT:
        if (PAGER_IsHorizontal(hwnd))
        {
            TRACE("[%p] PGF_SCROLLRIGHT\n", hwnd);
            PAGER_Scroll(hwnd, PGF_SCROLLRIGHT);
        }
        else
        {
            TRACE("[%p] PGF_SCROLLDOWN\n", hwnd);
            PAGER_Scroll(hwnd, PGF_SCROLLDOWN);
        }
        break;
    default:
        break;
    }

    return TRUE;
}

static LRESULT
PAGER_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%p]\n", hwnd);

    KillTimer (hwnd, TIMERID1);
    KillTimer (hwnd, TIMERID2);

    /* make PRESSED btns NORMAL but don't hide gray btns */
    if (infoPtr->TLbtnState & (PGF_HOT | PGF_DEPRESSED))
        infoPtr->TLbtnState = PGF_NORMAL;
    if (infoPtr->BRbtnState & (PGF_HOT | PGF_DEPRESSED))
        infoPtr->BRbtnState = PGF_NORMAL;

    return 0;
}

static LRESULT
PAGER_Timer (HWND hwnd, WPARAM wParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    INT dir;

    /* if initial timer, kill it and start the repeat timer */
    if (wParam == TIMERID1) {
	if (infoPtr->TLbtnState == PGF_HOT)
	    dir = PAGER_IsHorizontal(hwnd) ?
		PGF_SCROLLLEFT : PGF_SCROLLUP;
	else
	    dir = PAGER_IsHorizontal(hwnd) ?
		PGF_SCROLLRIGHT : PGF_SCROLLDOWN;
	TRACE("[%p] TIMERID1: style=%08lx, dir=%d\n", hwnd, dwStyle, dir);
	KillTimer(hwnd, TIMERID1);
	SetTimer(hwnd, TIMERID1, REPEAT_DELAY, 0);
	if (dwStyle & PGS_AUTOSCROLL) {
	    PAGER_Scroll(hwnd, dir);
	    SetWindowPos(hwnd, 0,0,0,0,0,
			 SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
			 SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return 0;

    }

    TRACE("[%p] TIMERID2: dir=%d\n", hwnd, infoPtr->direction);
    KillTimer(hwnd, TIMERID2);
    if (infoPtr->direction > 0) {
	PAGER_Scroll(hwnd, infoPtr->direction);
	SetTimer(hwnd, TIMERID2, REPEAT_DELAY, 0);
    }
    return 0;
}

static LRESULT
PAGER_EraseBackground (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    POINT pt, ptorig;
    HDC hdc = (HDC)wParam;
    HWND parent;

    pt.x = 0;
    pt.y = 0;
    parent = GetParent(hwnd);
    MapWindowPoints(hwnd, parent, &pt, 1);
    OffsetWindowOrgEx (hdc, pt.x, pt.y, &ptorig);
    SendMessageW (parent, WM_ERASEBKGND, wParam, lParam);
    SetWindowOrgEx (hdc, ptorig.x, ptorig.y, 0);

    return TRUE;
}


static LRESULT
PAGER_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* note that WM_SIZE is sent whenever NCCalcSize resizes the client wnd */

    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%p] %dx%d\n", hwnd, (short)LOWORD(lParam), (short)HIWORD(lParam));

    if (PAGER_IsHorizontal(hwnd))
        infoPtr->nHeight = (short)HIWORD(lParam);
    else
        infoPtr->nWidth = (short)LOWORD(lParam);

    return PAGER_RecalcSize(hwnd);
}


static LRESULT WINAPI
PAGER_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    if (!infoPtr && (uMsg != WM_CREATE))
	return DefWindowProcW (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case EM_FMTLINES:
	    return PAGER_FmtLines(hwnd);

        case PGM_FORWARDMOUSE:
            return PAGER_ForwardMouse (hwnd, wParam);

        case PGM_GETBKCOLOR:
            return PAGER_GetBkColor(hwnd);

        case PGM_GETBORDER:
            return PAGER_GetBorder(hwnd);

        case PGM_GETBUTTONSIZE:
            return PAGER_GetButtonSize(hwnd);

        case PGM_GETPOS:
            return PAGER_GetPos(hwnd);

        case PGM_GETBUTTONSTATE:
            return PAGER_GetButtonState (hwnd, wParam, lParam);

/*      case PGM_GETDROPTARGET: */

        case PGM_RECALCSIZE:
            return PAGER_RecalcSize(hwnd);

        case PGM_SETBKCOLOR:
            return PAGER_SetBkColor (hwnd, wParam, lParam);

        case PGM_SETBORDER:
            return PAGER_SetBorder (hwnd, wParam, lParam);

        case PGM_SETBUTTONSIZE:
            return PAGER_SetButtonSize (hwnd, wParam, lParam);

        case PGM_SETCHILD:
            return PAGER_SetChild (hwnd, wParam, lParam);

        case PGM_SETPOS:
            return PAGER_SetPos(hwnd, (INT)lParam, FALSE);

        case WM_CREATE:
            return PAGER_Create (hwnd, wParam, lParam);

        case WM_DESTROY:
            return PAGER_Destroy (hwnd, wParam, lParam);

        case WM_SIZE:
            return PAGER_Size (hwnd, wParam, lParam);

        case WM_NCPAINT:
            return PAGER_NCPaint (hwnd, wParam, lParam);

        case WM_WINDOWPOSCHANGING:
            return PAGER_HandleWindowPosChanging (hwnd, wParam, (WINDOWPOS*)lParam);

        case WM_NCCALCSIZE:
            return PAGER_NCCalcSize (hwnd, wParam, lParam);

        case WM_NCHITTEST:
            return PAGER_NCHitTest (hwnd, wParam, lParam);

        case WM_MOUSEMOVE:
            if (infoPtr->bForward && infoPtr->hwndChild)
                PostMessageW(infoPtr->hwndChild, WM_MOUSEMOVE, wParam, lParam);
            return PAGER_MouseMove (hwnd, wParam, lParam);

        case WM_LBUTTONDOWN:
            return PAGER_LButtonDown (hwnd, wParam, lParam);

        case WM_LBUTTONUP:
            return PAGER_LButtonUp (hwnd, wParam, lParam);

        case WM_ERASEBKGND:
            return PAGER_EraseBackground (hwnd, wParam, lParam);

        case WM_TIMER:
            return PAGER_Timer (hwnd, wParam);

        case WM_NOTIFY:
        case WM_COMMAND:
            return SendMessageW (infoPtr->hwndNotify, uMsg, wParam, lParam);

        default:
            return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


VOID
PAGER_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = PAGER_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(PAGER_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wndClass.lpszClassName = WC_PAGESCROLLERW;

    RegisterClassW (&wndClass);
}


VOID
PAGER_Unregister (void)
{
    UnregisterClassW (WC_PAGESCROLLERW, NULL);
}
