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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES
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
 *    Messages:
 *      WM_PRINT and/or WM_PRINTCLIENT
 *
 * TESTING:
 *    Tested primarily with the controlspy Pager application.
 *       Susan Farley (susan@codeweavers.com)
 *
 * IMPLEMENTATION NOTES:
 *    This control uses WM_NCPAINT instead of WM_PAINT to paint itself
 *    as we need to scroll a child window. In order to do this we move 
 *    the child window in the control's client area, using the clipping
 *    region that is automatically set around the client area. As the 
 *    entire client area now consists of the child window, we must 
 *    allocate space (WM_NCCALCSIZE) for the buttons and draw them as 
 *    a non-client area (WM_NCPAINT).
 *       Robert Shearman <rob@codeweavers.com>
 */

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(pager);

typedef struct
{
    HWND   hwndSelf;   /* handle of the control wnd */
    HWND   hwndChild;  /* handle of the contained wnd */
    HWND   hwndNotify; /* handle of the parent wnd */
    DWORD  dwStyle;    /* styles for this control */
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

#define TIMERID1         1
#define TIMERID2         2
#define INITIAL_DELAY    500
#define REPEAT_DELAY     50

static void
PAGER_GetButtonRects(const PAGER_INFO* infoPtr, RECT* prcTopLeft, RECT* prcBottomRight, BOOL bClientCoords)
{
    RECT rcWindow;
    GetWindowRect (infoPtr->hwndSelf, &rcWindow);

    if (bClientCoords)
        MapWindowPoints( 0, infoPtr->hwndSelf, (POINT *)&rcWindow, 2 );
    else
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

    *prcTopLeft = *prcBottomRight = rcWindow;
    if (infoPtr->dwStyle & PGS_HORZ)
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

static void
PAGER_DrawButton(HDC hdc, COLORREF clrBk, RECT rc,
                 BOOL horz, BOOL topLeft, INT btnState)
{
    UINT flags;

    TRACE("rc = %s, btnState = %d\n", wine_dbgstr_rect(&rc), btnState);

    if (btnState == PGF_INVISIBLE)
        return;

    if ((rc.right - rc.left <= 0) || (rc.bottom - rc.top <= 0))
        return;

    if (horz)
        flags = topLeft ? DFCS_SCROLLLEFT : DFCS_SCROLLRIGHT;
    else
        flags = topLeft ? DFCS_SCROLLUP : DFCS_SCROLLDOWN;

    switch (btnState)
    {
    case PGF_HOT:
        break;
    case PGF_NORMAL:
        flags |= DFCS_FLAT;
        break;
    case PGF_DEPRESSED:
        flags |= DFCS_PUSHED;
        break;
    case PGF_GRAYED:
        flags |= DFCS_INACTIVE | DFCS_FLAT;
        break;
    }
    DrawFrameControl( hdc, &rc, DFC_SCROLL, flags );
}

/* << PAGER_GetDropTarget >> */

static inline LRESULT
PAGER_ForwardMouse (PAGER_INFO* infoPtr, BOOL bFwd)
{
    TRACE("[%p]\n", infoPtr->hwndSelf);

    infoPtr->bForward = bFwd;

    return 0;
}

static inline LRESULT
PAGER_GetButtonState (const PAGER_INFO* infoPtr, INT btn)
{
    LRESULT btnState = PGF_INVISIBLE;
    TRACE("[%p]\n", infoPtr->hwndSelf);

    if (btn == PGB_TOPORLEFT)
        btnState = infoPtr->TLbtnState;
    else if (btn == PGB_BOTTOMORRIGHT)
        btnState = infoPtr->BRbtnState;

    return btnState;
}


static inline INT
PAGER_GetPos(const PAGER_INFO *infoPtr)
{
    TRACE("[%p] returns %d\n", infoPtr->hwndSelf, infoPtr->nPos);
    return infoPtr->nPos;
}

static inline INT
PAGER_GetButtonSize(const PAGER_INFO *infoPtr)
{
    TRACE("[%p] returns %d\n", infoPtr->hwndSelf, infoPtr->nButtonSize);
    return infoPtr->nButtonSize;
}

static inline INT
PAGER_GetBorder(const PAGER_INFO *infoPtr)
{
    TRACE("[%p] returns %d\n", infoPtr->hwndSelf, infoPtr->nBorder);
    return infoPtr->nBorder;
}

static inline COLORREF
PAGER_GetBkColor(const PAGER_INFO *infoPtr)
{
    TRACE("[%p] returns %06x\n", infoPtr->hwndSelf, infoPtr->clrBk);
    return infoPtr->clrBk;
}

static void
PAGER_CalcSize( PAGER_INFO *infoPtr )
{
    NMPGCALCSIZE nmpgcs;
    ZeroMemory (&nmpgcs, sizeof (NMPGCALCSIZE));
    nmpgcs.hdr.hwndFrom = infoPtr->hwndSelf;
    nmpgcs.hdr.idFrom   = GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_ID);
    nmpgcs.hdr.code = PGN_CALCSIZE;
    nmpgcs.dwFlag = (infoPtr->dwStyle & PGS_HORZ) ? PGF_CALCWIDTH : PGF_CALCHEIGHT;
    nmpgcs.iWidth = infoPtr->nWidth;
    nmpgcs.iHeight = infoPtr->nHeight;
    SendMessageW (infoPtr->hwndNotify, WM_NOTIFY, nmpgcs.hdr.idFrom, (LPARAM)&nmpgcs);

    if (infoPtr->dwStyle & PGS_HORZ)
        infoPtr->nWidth = nmpgcs.iWidth;
    else
        infoPtr->nHeight = nmpgcs.iHeight;

    TRACE("[%p] PGN_CALCSIZE returns %dx%d\n", infoPtr->hwndSelf, nmpgcs.iWidth, nmpgcs.iHeight );
}

static void
PAGER_PositionChildWnd(PAGER_INFO* infoPtr)
{
    if (infoPtr->hwndChild)
    {
        RECT rcClient;
        int nPos = infoPtr->nPos;

        /* compensate for a grayed btn, which will soon become invisible */
        if (infoPtr->TLbtnState == PGF_GRAYED)
            nPos += infoPtr->nButtonSize;

        GetClientRect(infoPtr->hwndSelf, &rcClient);

        if (infoPtr->dwStyle & PGS_HORZ)
        {
            int wndSize = max(0, rcClient.right - rcClient.left);
            if (infoPtr->nWidth < wndSize)
                infoPtr->nWidth = wndSize;

            TRACE("[%p] SWP %dx%d at (%d,%d)\n", infoPtr->hwndSelf,
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

            TRACE("[%p] SWP %dx%d at (%d,%d)\n", infoPtr->hwndSelf,
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
PAGER_GetScrollRange(PAGER_INFO* infoPtr)
{
    INT scrollRange = 0;

    if (infoPtr->hwndChild)
    {
        INT wndSize, childSize;
        RECT wndRect;
        GetWindowRect(infoPtr->hwndSelf, &wndRect);

        PAGER_CalcSize(infoPtr);
        if (infoPtr->dwStyle & PGS_HORZ)
        {
            wndSize = wndRect.right - wndRect.left;
            childSize = infoPtr->nWidth;
        }
        else
        {
            wndSize = wndRect.bottom - wndRect.top;
            childSize = infoPtr->nHeight;
        }

        TRACE("childSize = %d,  wndSize = %d\n", childSize, wndSize);
        if (childSize > wndSize)
            scrollRange = childSize - wndSize + infoPtr->nButtonSize;
    }

    TRACE("[%p] returns %d\n", infoPtr->hwndSelf, scrollRange);
    return scrollRange;
}

static void
PAGER_UpdateBtns(PAGER_INFO *infoPtr, INT scrollRange, BOOL hideGrayBtns)
{
    BOOL resizeClient;
    BOOL repaintBtns;
    INT oldTLbtnState = infoPtr->TLbtnState;
    INT oldBRbtnState = infoPtr->BRbtnState;
    POINT pt;
    RECT rcTopLeft, rcBottomRight;

    /* get button rects */
    PAGER_GetButtonRects(infoPtr, &rcTopLeft, &rcBottomRight, TRUE);

    GetCursorPos(&pt);
    ScreenToClient( infoPtr->hwndSelf, &pt );

    /* update states based on scroll position */
    if (infoPtr->nPos > 0)
    {
        if (infoPtr->TLbtnState == PGF_INVISIBLE || infoPtr->TLbtnState == PGF_GRAYED)
            infoPtr->TLbtnState = PGF_NORMAL;
    }
    else if (!hideGrayBtns && PtInRect(&rcTopLeft, pt))
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
    else if (!hideGrayBtns && PtInRect(&rcBottomRight, pt))
        infoPtr->BRbtnState = PGF_GRAYED;
    else
        infoPtr->BRbtnState = PGF_INVISIBLE;

    /* only need to resize when entering or leaving PGF_INVISIBLE state */
    resizeClient =
        ((oldTLbtnState == PGF_INVISIBLE) != (infoPtr->TLbtnState == PGF_INVISIBLE)) ||
        ((oldBRbtnState == PGF_INVISIBLE) != (infoPtr->BRbtnState == PGF_INVISIBLE));
    /* initiate NCCalcSize to resize client wnd if necessary */
    if (resizeClient)
        SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                     SWP_NOZORDER | SWP_NOACTIVATE);

    /* repaint when changing any state */
    repaintBtns = (oldTLbtnState != infoPtr->TLbtnState) || 
                  (oldBRbtnState != infoPtr->BRbtnState);
    if (repaintBtns)
        SendMessageW(infoPtr->hwndSelf, WM_NCPAINT, 0, 0);
}

static LRESULT
PAGER_SetPos(PAGER_INFO* infoPtr, INT newPos, BOOL fromBtnPress)
{
    INT scrollRange = PAGER_GetScrollRange(infoPtr);
    INT oldPos = infoPtr->nPos;

    if ((scrollRange <= 0) || (newPos < 0))
        infoPtr->nPos = 0;
    else if (newPos > scrollRange)
        infoPtr->nPos = scrollRange;
    else
        infoPtr->nPos = newPos;

    TRACE("[%p] pos=%d, oldpos=%d\n", infoPtr->hwndSelf, infoPtr->nPos, oldPos);

    if (infoPtr->nPos != oldPos)
    {
        /* gray and restore btns, and if from WM_SETPOS, hide the gray btns */
        PAGER_UpdateBtns(infoPtr, scrollRange, !fromBtnPress);
        PAGER_PositionChildWnd(infoPtr);
    }

    return 0;
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
PAGER_RecalcSize(PAGER_INFO *infoPtr)
{
    TRACE("[%p]\n", infoPtr->hwndSelf);

    if (infoPtr->hwndChild)
    {
        INT scrollRange = PAGER_GetScrollRange(infoPtr);

        if (scrollRange <= 0)
        {
            infoPtr->nPos = -1;
            PAGER_SetPos(infoPtr, 0, FALSE);
        }
        else
            PAGER_PositionChildWnd(infoPtr);
    }

    return 1;
}


static COLORREF
PAGER_SetBkColor (PAGER_INFO* infoPtr, COLORREF clrBk)
{
    COLORREF clrTemp = infoPtr->clrBk;

    infoPtr->clrBk = clrBk;
    TRACE("[%p] %06x\n", infoPtr->hwndSelf, infoPtr->clrBk);

    /* the native control seems to do things this way */
    SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0,
		 SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
		 SWP_NOZORDER | SWP_NOACTIVATE);

    RedrawWindow(infoPtr->hwndSelf, 0, 0, RDW_ERASE | RDW_INVALIDATE);

    return clrTemp;
}


static INT
PAGER_SetBorder (PAGER_INFO* infoPtr, INT iBorder)
{
    INT nTemp = infoPtr->nBorder;

    infoPtr->nBorder = iBorder;
    TRACE("[%p] %d\n", infoPtr->hwndSelf, infoPtr->nBorder);

    PAGER_RecalcSize(infoPtr);

    return nTemp;
}


static INT
PAGER_SetButtonSize (PAGER_INFO* infoPtr, INT iButtonSize)
{
    INT nTemp = infoPtr->nButtonSize;

    infoPtr->nButtonSize = iButtonSize;
    TRACE("[%p] %d\n", infoPtr->hwndSelf, infoPtr->nButtonSize);

    PAGER_RecalcSize(infoPtr);

    return nTemp;
}


static LRESULT
PAGER_SetChild (PAGER_INFO* infoPtr, HWND hwndChild)
{
    infoPtr->hwndChild = IsWindow (hwndChild) ? hwndChild : 0;

    if (infoPtr->hwndChild)
    {
        TRACE("[%p] hwndChild=%p\n", infoPtr->hwndSelf, infoPtr->hwndChild);

        SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

        /* position child within the page scroller */
        SetWindowPos(infoPtr->hwndChild, HWND_TOP,
                     0,0,0,0,
                     SWP_SHOWWINDOW | SWP_NOSIZE);  /* native is 0 */

        infoPtr->nPos = -1;
        PAGER_SetPos(infoPtr, 0, FALSE);
    }

    return 0;
}

static void
PAGER_Scroll(PAGER_INFO* infoPtr, INT dir)
{
    NMPGSCROLL nmpgScroll;
    RECT rcWnd;

    if (infoPtr->hwndChild)
    {
        ZeroMemory (&nmpgScroll, sizeof (NMPGSCROLL));
        nmpgScroll.hdr.hwndFrom = infoPtr->hwndSelf;
        nmpgScroll.hdr.idFrom   = GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_ID);
        nmpgScroll.hdr.code = PGN_SCROLL;

        GetWindowRect(infoPtr->hwndSelf, &rcWnd);
        GetClientRect(infoPtr->hwndSelf, &nmpgScroll.rcParent);
        nmpgScroll.iXpos = nmpgScroll.iYpos = 0;
        nmpgScroll.iDir = dir;

        if (infoPtr->dwStyle & PGS_HORZ)
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

        SendMessageW (infoPtr->hwndNotify, WM_NOTIFY, nmpgScroll.hdr.idFrom, (LPARAM)&nmpgScroll);

        TRACE("[%p] PGN_SCROLL returns iScroll=%d\n", infoPtr->hwndSelf, nmpgScroll.iScroll);

        if (nmpgScroll.iScroll > 0)
        {
            infoPtr->direction = dir;

            if (dir == PGF_SCROLLLEFT || dir == PGF_SCROLLUP)
                PAGER_SetPos(infoPtr, infoPtr->nPos - nmpgScroll.iScroll, TRUE);
            else
                PAGER_SetPos(infoPtr, infoPtr->nPos + nmpgScroll.iScroll, TRUE);
        }
        else
            infoPtr->direction = -1;
    }
}

static LRESULT
PAGER_FmtLines(const PAGER_INFO *infoPtr)
{
    /* initiate NCCalcSize to resize client wnd and get size */
    SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0,
		 SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
		 SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(infoPtr->hwndChild, 0,
		 0,0,infoPtr->nWidth,infoPtr->nHeight,
		 0);

    return DefWindowProcW (infoPtr->hwndSelf, EM_FMTLINES, 0, 0);
}

static LRESULT
PAGER_Create (HWND hwnd, const CREATESTRUCTW *lpcs)
{
    PAGER_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = Alloc (sizeof(PAGER_INFO));
    if (!infoPtr) return -1;
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    /* set default settings */
    infoPtr->hwndSelf = hwnd;
    infoPtr->hwndChild = NULL;
    infoPtr->hwndNotify = lpcs->hwndParent;
    infoPtr->dwStyle = lpcs->style;
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

    if (infoPtr->dwStyle & PGS_DRAGNDROP)
        FIXME("[%p] Drag and Drop style is not implemented yet.\n", infoPtr->hwndSelf);

    return 0;
}


static LRESULT
PAGER_Destroy (PAGER_INFO *infoPtr)
{
    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);
    Free (infoPtr);  /* free pager info data */
    return 0;
}

static LRESULT
PAGER_NCCalcSize(PAGER_INFO* infoPtr, WPARAM wParam, LPRECT lpRect)
{
    RECT rcChild, rcWindow;

    /*
     * lpRect points to a RECT struct.  On entry, the struct
     * contains the proposed wnd rectangle for the window.
     * On exit, the struct should contain the screen
     * coordinates of the corresponding window's client area.
     */

    DefWindowProcW (infoPtr->hwndSelf, WM_NCCALCSIZE, wParam, (LPARAM)lpRect);

    TRACE("orig rect=%s\n", wine_dbgstr_rect(lpRect));

    GetWindowRect (infoPtr->hwndChild, &rcChild);
    MapWindowPoints (0, infoPtr->hwndSelf, (LPPOINT)&rcChild, 2); /* FIXME: RECT != 2 POINTS */
    GetWindowRect (infoPtr->hwndSelf, &rcWindow);

    infoPtr->nWidth = lpRect->right - lpRect->left;
    infoPtr->nHeight = lpRect->bottom - lpRect->top;
    PAGER_CalcSize( infoPtr );

    if (infoPtr->dwStyle & PGS_HORZ)
    {
	if (infoPtr->TLbtnState && (lpRect->left + infoPtr->nButtonSize < lpRect->right))
	    lpRect->left += infoPtr->nButtonSize;
	if (infoPtr->BRbtnState && (lpRect->right - infoPtr->nButtonSize > lpRect->left))
	    lpRect->right -= infoPtr->nButtonSize;
    }
    else
    {
	if (infoPtr->TLbtnState && (lpRect->top + infoPtr->nButtonSize < lpRect->bottom))
	    lpRect->top += infoPtr->nButtonSize;
	if (infoPtr->BRbtnState && (lpRect->bottom - infoPtr->nButtonSize > lpRect->top))
	    lpRect->bottom -= infoPtr->nButtonSize;
    }

    TRACE("nPos=%d, nHeight=%d, window=%s\n", infoPtr->nPos, infoPtr->nHeight, wine_dbgstr_rect(&rcWindow));
    TRACE("[%p] client rect set to %s BtnState[%d,%d]\n", infoPtr->hwndSelf, wine_dbgstr_rect(lpRect),
	  infoPtr->TLbtnState, infoPtr->BRbtnState);

    return 0;
}

static LRESULT
PAGER_NCPaint (const PAGER_INFO* infoPtr, HRGN hRgn)
{
    RECT rcBottomRight, rcTopLeft;
    HDC hdc;

    if (infoPtr->dwStyle & WS_MINIMIZE)
        return 0;

    DefWindowProcW (infoPtr->hwndSelf, WM_NCPAINT, (WPARAM)hRgn, 0);

    if (!(hdc = GetDCEx (infoPtr->hwndSelf, 0, DCX_USESTYLE | DCX_WINDOW)))
        return 0;

    PAGER_GetButtonRects(infoPtr, &rcTopLeft, &rcBottomRight, FALSE);

    PAGER_DrawButton(hdc, infoPtr->clrBk, rcTopLeft,
                     infoPtr->dwStyle & PGS_HORZ, TRUE, infoPtr->TLbtnState);
    PAGER_DrawButton(hdc, infoPtr->clrBk, rcBottomRight,
                     infoPtr->dwStyle & PGS_HORZ, FALSE, infoPtr->BRbtnState);

    ReleaseDC( infoPtr->hwndSelf, hdc );
    return 0;
}

static INT
PAGER_HitTest (const PAGER_INFO* infoPtr, const POINT * pt)
{
    RECT clientRect, rcTopLeft, rcBottomRight;
    POINT ptWindow;

    GetClientRect (infoPtr->hwndSelf, &clientRect);

    if (PtInRect(&clientRect, *pt))
    {
        TRACE("child\n");
        return -1;
    }

    ptWindow = *pt;
    PAGER_GetButtonRects(infoPtr, &rcTopLeft, &rcBottomRight, TRUE);

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
PAGER_NCHitTest (const PAGER_INFO* infoPtr, INT x, INT y)
{
    POINT pt;
    INT nHit;

    pt.x = x;
    pt.y = y;

    ScreenToClient (infoPtr->hwndSelf, &pt);
    nHit = PAGER_HitTest(infoPtr, &pt);

    return (nHit < 0) ? HTTRANSPARENT : HTCLIENT;
}

static LRESULT
PAGER_MouseMove (PAGER_INFO* infoPtr, INT keys, INT x, INT y)
{
    POINT clpt, pt;
    RECT wnrect;
    BOOL topLeft = FALSE;
    INT btnstate = 0;
    INT hit;
    HDC hdc;

    pt.x = x;
    pt.y = y;

    TRACE("[%p] to (%d,%d)\n", infoPtr->hwndSelf, x, y);
    ClientToScreen(infoPtr->hwndSelf, &pt);
    GetWindowRect(infoPtr->hwndSelf, &wnrect);
    if (PtInRect(&wnrect, pt)) {
	RECT topleft, bottomright, *rect = NULL;

	PAGER_GetButtonRects(infoPtr, &topleft, &bottomright, FALSE);

	clpt = pt;
	MapWindowPoints(0, infoPtr->hwndSelf, &clpt, 1);
	hit = PAGER_HitTest(infoPtr, &clpt);
	if ((hit == PGB_TOPORLEFT) && (infoPtr->TLbtnState == PGF_NORMAL))
	{
	    topLeft = TRUE;
	    rect = &topleft;
	    infoPtr->TLbtnState = PGF_HOT;
	    btnstate = infoPtr->TLbtnState;
	}
	else if ((hit == PGB_BOTTOMORRIGHT) && (infoPtr->BRbtnState == PGF_NORMAL))
	{
	    topLeft = FALSE;
	    rect = &bottomright;
	    infoPtr->BRbtnState = PGF_HOT;
	    btnstate = infoPtr->BRbtnState;
	}

	/* If in one of the buttons the capture and draw buttons */
	if (rect)
	{
            TRACE("[%p] draw btn (%s), Capture %s, style %08x\n",
                  infoPtr->hwndSelf, wine_dbgstr_rect(rect),
		  (infoPtr->bCapture) ? "TRUE" : "FALSE",
		  infoPtr->dwStyle);
	    if (!infoPtr->bCapture)
	    {
	        TRACE("[%p] SetCapture\n", infoPtr->hwndSelf);
	        SetCapture(infoPtr->hwndSelf);
	        infoPtr->bCapture = TRUE;
	    }
	    if (infoPtr->dwStyle & PGS_AUTOSCROLL)
		SetTimer(infoPtr->hwndSelf, TIMERID1, 0x3e, 0);
	    hdc = GetWindowDC(infoPtr->hwndSelf);
	    /* OffsetRect(wnrect, 0 | 1, 0 | 1) */
	    PAGER_DrawButton(hdc, infoPtr->clrBk, *rect,
			     infoPtr->dwStyle & PGS_HORZ, topLeft, btnstate);
	    ReleaseDC(infoPtr->hwndSelf, hdc);
	    return 0;
	}
    }

    /* If we think we are captured, then do release */
    if (infoPtr->bCapture && (WindowFromPoint(pt) != infoPtr->hwndSelf))
    {
    	NMHDR nmhdr;

        infoPtr->bCapture = FALSE;

        if (GetCapture() == infoPtr->hwndSelf)
        {
            ReleaseCapture();

            if (infoPtr->TLbtnState == PGF_GRAYED)
            {
                infoPtr->TLbtnState = PGF_INVISIBLE;
                SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0,
                             SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            else if (infoPtr->TLbtnState == PGF_HOT)
            {
        	infoPtr->TLbtnState = PGF_NORMAL;
        	/* FIXME: just invalidate button rect */
                RedrawWindow(infoPtr->hwndSelf, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }

            if (infoPtr->BRbtnState == PGF_GRAYED)
            {
                infoPtr->BRbtnState = PGF_INVISIBLE;
                SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0,
                             SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            else if (infoPtr->BRbtnState == PGF_HOT)
            {
        	infoPtr->BRbtnState = PGF_NORMAL;
        	/* FIXME: just invalidate button rect */
                RedrawWindow(infoPtr->hwndSelf, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }

            /* Notify parent of released mouse capture */
        	memset(&nmhdr, 0, sizeof(NMHDR));
        	nmhdr.hwndFrom = infoPtr->hwndSelf;
        	nmhdr.idFrom   = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
        	nmhdr.code = NM_RELEASEDCAPTURE;
		SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
        }
        if (IsWindow(infoPtr->hwndSelf))
            KillTimer(infoPtr->hwndSelf, TIMERID1);
    }
    return 0;
}

static LRESULT
PAGER_LButtonDown (PAGER_INFO* infoPtr, INT keys, INT x, INT y)
{
    BOOL repaintBtns = FALSE;
    POINT pt;
    INT hit;

    pt.x = x;
    pt.y = y;

    TRACE("[%p] at (%d,%d)\n", infoPtr->hwndSelf, x, y);

    hit = PAGER_HitTest(infoPtr, &pt);

    /* put btn in DEPRESSED state */
    if (hit == PGB_TOPORLEFT)
    {
        repaintBtns = infoPtr->TLbtnState != PGF_DEPRESSED;
        infoPtr->TLbtnState = PGF_DEPRESSED;
        SetTimer(infoPtr->hwndSelf, TIMERID1, INITIAL_DELAY, 0);
    }
    else if (hit == PGB_BOTTOMORRIGHT)
    {
        repaintBtns = infoPtr->BRbtnState != PGF_DEPRESSED;
        infoPtr->BRbtnState = PGF_DEPRESSED;
        SetTimer(infoPtr->hwndSelf, TIMERID1, INITIAL_DELAY, 0);
    }

    if (repaintBtns)
        SendMessageW(infoPtr->hwndSelf, WM_NCPAINT, 0, 0);

    switch(hit)
    {
    case PGB_TOPORLEFT:
        if (infoPtr->dwStyle & PGS_HORZ)
        {
            TRACE("[%p] PGF_SCROLLLEFT\n", infoPtr->hwndSelf);
            PAGER_Scroll(infoPtr, PGF_SCROLLLEFT);
        }
        else
        {
            TRACE("[%p] PGF_SCROLLUP\n", infoPtr->hwndSelf);
            PAGER_Scroll(infoPtr, PGF_SCROLLUP);
        }
        break;
    case PGB_BOTTOMORRIGHT:
        if (infoPtr->dwStyle & PGS_HORZ)
        {
            TRACE("[%p] PGF_SCROLLRIGHT\n", infoPtr->hwndSelf);
            PAGER_Scroll(infoPtr, PGF_SCROLLRIGHT);
        }
        else
        {
            TRACE("[%p] PGF_SCROLLDOWN\n", infoPtr->hwndSelf);
            PAGER_Scroll(infoPtr, PGF_SCROLLDOWN);
        }
        break;
    default:
        break;
    }

    return 0;
}

static LRESULT
PAGER_LButtonUp (PAGER_INFO* infoPtr, INT keys, INT x, INT y)
{
    TRACE("[%p]\n", infoPtr->hwndSelf);

    KillTimer (infoPtr->hwndSelf, TIMERID1);
    KillTimer (infoPtr->hwndSelf, TIMERID2);

    /* make PRESSED btns NORMAL but don't hide gray btns */
    if (infoPtr->TLbtnState & (PGF_HOT | PGF_DEPRESSED))
        infoPtr->TLbtnState = PGF_NORMAL;
    if (infoPtr->BRbtnState & (PGF_HOT | PGF_DEPRESSED))
        infoPtr->BRbtnState = PGF_NORMAL;

    return 0;
}

static LRESULT
PAGER_Timer (PAGER_INFO* infoPtr, INT nTimerId)
{
    INT dir;

    /* if initial timer, kill it and start the repeat timer */
    if (nTimerId == TIMERID1) {
	if (infoPtr->TLbtnState == PGF_HOT)
	    dir = (infoPtr->dwStyle & PGS_HORZ) ?
		PGF_SCROLLLEFT : PGF_SCROLLUP;
	else
	    dir = (infoPtr->dwStyle & PGS_HORZ) ?
		PGF_SCROLLRIGHT : PGF_SCROLLDOWN;
	TRACE("[%p] TIMERID1: style=%08x, dir=%d\n",
              infoPtr->hwndSelf, infoPtr->dwStyle, dir);
	KillTimer(infoPtr->hwndSelf, TIMERID1);
	SetTimer(infoPtr->hwndSelf, TIMERID1, REPEAT_DELAY, 0);
	if (infoPtr->dwStyle & PGS_AUTOSCROLL) {
	    PAGER_Scroll(infoPtr, dir);
	    SetWindowPos(infoPtr->hwndSelf, 0, 0, 0, 0, 0,
			 SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
			 SWP_NOZORDER | SWP_NOACTIVATE);
	}
	return 0;

    }

    TRACE("[%p] TIMERID2: dir=%d\n", infoPtr->hwndSelf, infoPtr->direction);
    KillTimer(infoPtr->hwndSelf, TIMERID2);
    if (infoPtr->direction > 0) {
	PAGER_Scroll(infoPtr, infoPtr->direction);
	SetTimer(infoPtr->hwndSelf, TIMERID2, REPEAT_DELAY, 0);
    }
    return 0;
}

static LRESULT
PAGER_EraseBackground (const PAGER_INFO* infoPtr, HDC hdc)
{
    POINT pt, ptorig;
    HWND parent;
    LRESULT ret;

    pt.x = 0;
    pt.y = 0;
    parent = GetParent(infoPtr->hwndSelf);
    MapWindowPoints(infoPtr->hwndSelf, parent, &pt, 1);
    OffsetWindowOrgEx (hdc, pt.x, pt.y, &ptorig);
    ret = SendMessageW (parent, WM_ERASEBKGND, (WPARAM)hdc, 0);
    SetWindowOrgEx (hdc, ptorig.x, ptorig.y, 0);

    return ret;
}


static LRESULT
PAGER_Size (PAGER_INFO* infoPtr, INT type, INT x, INT y)
{
    /* note that WM_SIZE is sent whenever NCCalcSize resizes the client wnd */

    TRACE("[%p] %d,%d\n", infoPtr->hwndSelf, x, y);

    if (infoPtr->dwStyle & PGS_HORZ)
        infoPtr->nHeight = y;
    else
        infoPtr->nWidth = x;

    return PAGER_RecalcSize(infoPtr);
}


static LRESULT 
PAGER_StyleChanged(PAGER_INFO *infoPtr, WPARAM wStyleType, const STYLESTRUCT *lpss)
{
    DWORD oldStyle = infoPtr->dwStyle;

    TRACE("(styletype=%lx, styleOld=0x%08x, styleNew=0x%08x)\n",
          wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE) return 0;
  
    infoPtr->dwStyle = lpss->styleNew;

    if ((oldStyle ^ lpss->styleNew) & (PGS_HORZ | PGS_VERT))
    {
        PAGER_RecalcSize(infoPtr);
    }

    return 0;
}

static LRESULT WINAPI
PAGER_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = (PAGER_INFO *)GetWindowLongPtrW(hwnd, 0);

    TRACE("(%p, %#x, %#lx, %#lx)\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_CREATE))
	return DefWindowProcW (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case EM_FMTLINES:
	    return PAGER_FmtLines(infoPtr);

        case PGM_FORWARDMOUSE:
            return PAGER_ForwardMouse (infoPtr, (BOOL)wParam);

        case PGM_GETBKCOLOR:
            return PAGER_GetBkColor(infoPtr);

        case PGM_GETBORDER:
            return PAGER_GetBorder(infoPtr);

        case PGM_GETBUTTONSIZE:
            return PAGER_GetButtonSize(infoPtr);

        case PGM_GETPOS:
            return PAGER_GetPos(infoPtr);

        case PGM_GETBUTTONSTATE:
            return PAGER_GetButtonState (infoPtr, (INT)lParam);

/*      case PGM_GETDROPTARGET: */

        case PGM_RECALCSIZE:
            return PAGER_RecalcSize(infoPtr);

        case PGM_SETBKCOLOR:
            return PAGER_SetBkColor (infoPtr, (COLORREF)lParam);

        case PGM_SETBORDER:
            return PAGER_SetBorder (infoPtr, (INT)lParam);

        case PGM_SETBUTTONSIZE:
            return PAGER_SetButtonSize (infoPtr, (INT)lParam);

        case PGM_SETCHILD:
            return PAGER_SetChild (infoPtr, (HWND)lParam);

        case PGM_SETPOS:
            return PAGER_SetPos(infoPtr, (INT)lParam, FALSE);

        case WM_CREATE:
            return PAGER_Create (hwnd, (LPCREATESTRUCTW)lParam);

        case WM_DESTROY:
            return PAGER_Destroy (infoPtr);

        case WM_SIZE:
            return PAGER_Size (infoPtr, (INT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_NCPAINT:
            return PAGER_NCPaint (infoPtr, (HRGN)wParam);

        case WM_STYLECHANGED:
            return PAGER_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

        case WM_NCCALCSIZE:
            return PAGER_NCCalcSize (infoPtr, wParam, (LPRECT)lParam);

        case WM_NCHITTEST:
            return PAGER_NCHitTest (infoPtr, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_MOUSEMOVE:
            if (infoPtr->bForward && infoPtr->hwndChild)
                PostMessageW(infoPtr->hwndChild, WM_MOUSEMOVE, wParam, lParam);
            return PAGER_MouseMove (infoPtr, (INT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_LBUTTONDOWN:
            return PAGER_LButtonDown (infoPtr, (INT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_LBUTTONUP:
            return PAGER_LButtonUp (infoPtr, (INT)wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));

        case WM_ERASEBKGND:
            return PAGER_EraseBackground (infoPtr, (HDC)wParam);

        case WM_TIMER:
            return PAGER_Timer (infoPtr, (INT)wParam);

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
