#include <windows.h>
#include <stdlib.h>
#include <user32/nc.h>
#include <user32/syscolor.h>
#include <user32/debug.h>

// GetBitmapDimensionEx in NC_DrawMaxButton95 returns TRUE on invalid bitmap

static HBITMAP hbitmapClose = 0;
static HBITMAP hbitmapCloseD = 0;
static HBITMAP hbitmapMinimize = 0;
static HBITMAP hbitmapMinimizeD = 0;
static HBITMAP hbitmapMaximize = 0;
static HBITMAP hbitmapMaximizeD = 0;
static HBITMAP hbitmapRestore = 0;
static HBITMAP hbitmapRestoreD = 0;

#define SC_ABOUTWINE    	(SC_SCREENSAVE+1)
#define SC_PUTMARK     		(SC_SCREENSAVE+2)

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_BORDER)))

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_FIXEDFRAME(style,exStyle) \
   (((((exStyle) & WS_EX_DLGMODALFRAME) || \
      ((style) & WS_DLGFRAME)) && ((style) & WS_BORDER)) && \
     !((style) & WS_THICKFRAME))

#define HAS_SIZEFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_MENU(w)  (!((w)->dwStyle & WS_CHILD) && ((w)->wIDmenu != 0))

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))


int TWEAK_WineLook = WIN95_LOOK;

/***********************************************************************
 *           NC_AdjustRect
 *
 * Compute the size of the window rectangle from the size of the
 * client rectangle.
 */
void NC_AdjustRect( LPRECT rect, DWORD style, WINBOOL menu,
                           DWORD exStyle )
{

    if(style & WS_ICONIC) return;
    /* Decide if the window will be managed (see CreateWindowEx) */
    // removed Options.managed &&
    if (!( !(style & WS_CHILD) &&
          ((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
           (exStyle & WS_EX_DLGMODALFRAME))))
    {
        if (HAS_DLGFRAME( style, exStyle ))
            InflateRect(rect, SYSMETRICS_CXDLGFRAME, SYSMETRICS_CYDLGFRAME );
        else
        {
            if (HAS_THICKFRAME(style))
                InflateRect( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );
            if (style & WS_BORDER)
                InflateRect( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER);
        }

        if ((style & WS_CAPTION) == WS_CAPTION)
            rect->top -= SYSMETRICS_CYCAPTION - SYSMETRICS_CYBORDER;
    }
    if (menu) rect->top -= SYSMETRICS_CYMENU + SYSMETRICS_CYBORDER;

    if (style & WS_VSCROLL) {
      rect->right  += SYSMETRICS_CXVSCROLL - 1;
      if(!(style & WS_BORDER))
	 rect->right++;
    }

    if (style & WS_HSCROLL) {
      rect->bottom += SYSMETRICS_CYHSCROLL - 1;
      if(!(style & WS_BORDER))
	 rect->bottom++;
    }
}

/******************************************************************************
 * NC_AdjustRectOuter95
 *
 * Computes the size of the "outside" parts of the window based on the
 * parameters of the client area.
 *
 + PARAMS
 *     LPRECT  rect
 *     DWORD  style
 *     WINBOOL  menu
 *     DWORD  exStyle
 *
 * NOTES
 *     "Outer" parts of a window means the whole window frame, caption and
 *     menu bar. It does not include "inner" parts of the frame like client
 *     edge, static edge or scroll bars.
 *
 * Revision history
 *     05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *        Original (NC_AdjustRect95) cut & paste from NC_AdjustRect
 *
 *     20-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *        Split NC_AdjustRect95 into NC_AdjustRectOuter95 and
 *        NC_AdjustRectInner95 and added handling of Win95 styles.
 *
 *****************************************************************************/

void
NC_AdjustRectOuter95 (LPRECT rect, DWORD style, WINBOOL menu, DWORD exStyle)
{
    if(style & WS_ICONIC) return;

    /* Decide if the window will be managed (see CreateWindowEx) */
    // Options.managed &&

    if (!( !(style & WS_CHILD) &&
          ((style & (WS_DLGFRAME | WS_THICKFRAME)) ||
           (exStyle & WS_EX_DLGMODALFRAME))))
    {
        if (HAS_FIXEDFRAME( style, exStyle ))
            InflateRect(rect, SYSMETRICS_CXDLGFRAME, SYSMETRICS_CYDLGFRAME );
        else
        {
            if (HAS_SIZEFRAME(style))
                InflateRect( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );

            if (style & WS_BORDER)
                InflateRect( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER);

        }

        if ((style & WS_CAPTION) == WS_CAPTION)
        {
	    if (exStyle & WS_EX_TOOLWINDOW)
		rect->top -= SYSMETRICS_CYSMCAPTION;
	    else
		rect->top -= SYSMETRICS_CYCAPTION;
        }
    } else {

	if (HAS_SIZEFRAME(style))
                InflateRect( rect, SYSMETRICS_CXFRAME, SYSMETRICS_CYFRAME );
#if 0
        if (style & WS_BORDER)
            InflateRect( rect, SYSMETRICS_CXBORDER, SYSMETRICS_CYBORDER);
#endif

        if ((style & WS_CAPTION) == WS_CAPTION)
        {
	    if (exStyle & WS_EX_TOOLWINDOW)
		rect->top -= SYSMETRICS_CYSMCAPTION;
	    else
		rect->top -= SYSMETRICS_CYCAPTION;
        }
    }

    if (menu)
	rect->top -= sysMetrics[SM_CYMENU];
}


/******************************************************************************
 * NC_AdjustRectInner95
 *
 * Computes the size of the "inside" part of the window based on the
 * parameters of the client area.
 *
 + PARAMS
 *     LPRECT rect
 *     DWORD    style
 *     DWORD    exStyle
 *
 * NOTES
 *     "Inner" part of a window means the window frame inside of the flat
 *     window frame. It includes the client edge, the static edge and the
 *     scroll bars.
 *
 * Revision history
 *     05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *        Original (NC_AdjustRect95) cut & paste from NC_AdjustRect
 *
 *     20-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *        Split NC_AdjustRect95 into NC_AdjustRectOuter95 and
 *        NC_AdjustRectInner95 and added handling of Win95 styles.
 *
 *****************************************************************************/

void
NC_AdjustRectInner95 (LPRECT rect, DWORD style, DWORD exStyle)
{
    if(style & WS_ICONIC) return;

    if (exStyle & WS_EX_CLIENTEDGE)
	InflateRect (rect, sysMetrics[SM_CXEDGE], sysMetrics[SM_CYEDGE]);

    if (exStyle & WS_EX_STATICEDGE)
	InflateRect (rect, sysMetrics[SM_CXBORDER], sysMetrics[SM_CYBORDER]);

    if (style & WS_VSCROLL) rect->right  += SYSMETRICS_CXVSCROLL;
    if (style & WS_HSCROLL) rect->bottom += SYSMETRICS_CYHSCROLL;
}

/***********************************************************************
 *           NC_HandleNCCalcSize
 *
 * Handle a WM_NCCALCSIZE message. Called from DefWindowProc().
 */
LONG NC_HandleNCCalcSize( WND *pWnd, RECT *winRect )
{
    RECT tmpRect = { 0, 0, 0, 0 };
    LONG result = 0;

    if (pWnd->class->style & CS_VREDRAW) result |= WVR_VREDRAW;
    if (pWnd->class->style & CS_HREDRAW) result |= WVR_HREDRAW;

    if( !( pWnd->dwStyle & WS_MINIMIZE ) ) {
	if (TWEAK_WineLook == WIN31_LOOK)
	    NC_AdjustRect( &tmpRect, pWnd->dwStyle, FALSE, pWnd->dwExStyle );
	else
	    NC_AdjustRectOuter95( &tmpRect, pWnd->dwStyle, FALSE, pWnd->dwExStyle );

	winRect->left   -= tmpRect.left;
	winRect->top    -= tmpRect.top;
	winRect->right  -= tmpRect.right;
	winRect->bottom -= tmpRect.bottom;

	if (HAS_MENU(pWnd)) {
	    DPRINT( "Calling GetMenuBarHeight with HWND 0x%x, width %d, at (%d, %d).\n", pWnd->hwndSelf,
			       winRect->right - winRect->left, -tmpRect.left, -tmpRect.top );

	    winRect->top +=
		MENU_GetMenuBarHeight( pWnd->hwndSelf,
				       winRect->right - winRect->left,
				       -tmpRect.left, -tmpRect.top ) + 1;
	}

	if (TWEAK_WineLook > WIN31_LOOK) {
	    SetRect (&tmpRect, 0, 0, 0, 0);
	    NC_AdjustRectInner95 (&tmpRect, pWnd->dwStyle, pWnd->dwExStyle);
	    winRect->left   -= tmpRect.left;
	    winRect->top    -= tmpRect.top;
	    winRect->right  -= tmpRect.right;
	    winRect->bottom -= tmpRect.bottom;
	}
    }
    return result;
}


/***********************************************************************
 *           NC_GetInsideRect
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 * The rectangle is in window coordinates (for drawing with GetWindowDC()).
 */
static void NC_GetInsideRect( HWND hwnd, RECT *rect )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->top    = rect->left = 0;
    rect->right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect->bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;
// || (wndPtr->flags & WIN_MANAGED)
    if ((wndPtr->dwStyle & WS_ICONIC) ) return;

    /* Remove frame from rectangle */
    if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
	InflateRect( rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
	if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
            InflateRect( rect, -1, 0 );
    }
    else
    {
	if (HAS_THICKFRAME( wndPtr->dwStyle ))
	    InflateRect( rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
	if (wndPtr->dwStyle & WS_BORDER)
	    InflateRect( rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER );
    }

    return;
}


/***********************************************************************
 *           NC_GetInsideRect95
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 * The rectangle is in window coordinates (for drawing with GetWindowDC()).
 */

static void
NC_GetInsideRect95 (HWND hwnd, RECT *rect)
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->top    = rect->left = 0;
    rect->right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect->bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

// || (wndPtr->flags & WIN_MANAGED)
    if ((wndPtr->dwStyle & WS_ICONIC) ) return;

    /* Remove frame from rectangle */
    if (HAS_FIXEDFRAME (wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
	InflateRect( rect, -SYSMETRICS_CXFIXEDFRAME, -SYSMETRICS_CYFIXEDFRAME);
    }
    else if (HAS_SIZEFRAME (wndPtr->dwStyle))
    {
	InflateRect( rect, -SYSMETRICS_CXSIZEFRAME, -SYSMETRICS_CYSIZEFRAME );

/*	if (wndPtr->dwStyle & WS_BORDER)
          InflateRect( rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER );*/
    }

//    if (wndPtr->dwStyle & WS_CHILD) {
	if (wndPtr->dwExStyle & WS_EX_CLIENTEDGE)
	    InflateRect(rect, -SYSMETRICS_CXEDGE, -SYSMETRICS_CYEDGE);

	if (wndPtr->dwExStyle & WS_EX_STATICEDGE)
	    InflateRect(rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER);
  //  }

    return;
}


/***********************************************************************
 * NC_DoNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from NC_HandleNcHitTest().
 */

LONG NC_DoNCHitTest (WND *wndPtr, POINT pt )
{
    RECT rect;

    DPRINT( "hwnd=%04x pt=%d,%d\n",
		      wndPtr->hwndSelf, pt.x, pt.y );

    GetWindowRect (wndPtr->hwndSelf, &rect );
    if (!PtInRect( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

//    if (!(wndPtr->flags & WIN_MANAGED))
    {
        /* Check borders */
        if (HAS_THICKFRAME( wndPtr->dwStyle ))
        {
            InflateRect( &rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
            if (wndPtr->dwStyle & WS_BORDER)
                InflateRect(&rect,-SYSMETRICS_CXBORDER,-SYSMETRICS_CYBORDER);
            if (!PtInRect( &rect, pt ))
            {
                /* Check top sizing border */
                if (pt.y < rect.top)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTTOPLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTTOPRIGHT;
                    return HTTOP;
                }
                /* Check bottom sizing border */
                if (pt.y >= rect.bottom)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTBOTTOMLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTBOTTOMRIGHT;
                    return HTBOTTOM;
                }
                /* Check left sizing border */
                if (pt.x < rect.left)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPLEFT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMLEFT;
                    return HTLEFT;
                }
                /* Check right sizing border */
                if (pt.x >= rect.right)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPRIGHT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMRIGHT;
                    return HTRIGHT;
                }
            }
        }
        else  /* No thick frame */
        {
            if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
                InflateRect(&rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
            else if (wndPtr->dwStyle & WS_BORDER)
                InflateRect(&rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER);
            if (!PtInRect( &rect, pt )) return HTBORDER;
        }

        /* Check caption */

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            rect.top += sysMetrics[SM_CYCAPTION] - 1;
            if (!PtInRect( &rect, pt ))
            {
                /* Check system menu */
                if (wndPtr->dwStyle & WS_SYSMENU)
                    rect.left += SYSMETRICS_CXSIZE;
                if (pt.x <= rect.left) return HTSYSMENU;
                /* Check maximize box */
                if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x >= rect.right) return HTMAXBUTTON;
                /* Check minimize box */
                if (wndPtr->dwStyle & WS_MINIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x >= rect.right) return HTMINBUTTON;
                return HTCAPTION;
            }
        }
    }

      /* Check client area */

    ScreenToClient( wndPtr->hwndSelf, &pt );
    GetClientRect( wndPtr->hwndSelf, &rect );
    if (PtInRect( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */

    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += SYSMETRICS_CXVSCROLL;
	if (PtInRect( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += SYSMETRICS_CYHSCROLL;
	if (PtInRect( &rect, pt ))
	{
	      /* Check size box */
	    if ((wndPtr->dwStyle & WS_VSCROLL) &&
		(pt.x >= rect.right - SYSMETRICS_CXVSCROLL))
		return HTSIZE;
	    return HTHSCROLL;
	}
    }

      /* Check menu bar */

    if (HAS_MENU(wndPtr))
    {
	if ((pt.y < 0) && (pt.x >= 0) && (pt.x < rect.right))
	    return HTMENU;
    }

      /* Should never get here */
    return HTERROR;
}


/***********************************************************************
 * NC_DoNCHitTest95
 *
 * Handle a WM_NCHITTEST message. Called from NC_HandleNCHitTest().
 *
 * FIXME:  Just a modified copy of the Win 3.1 version.
 */

LONG
NC_DoNCHitTest95 (WND *wndPtr, POINT pt )
{
    RECT rect;

    DPRINT( "hwnd=%04x pt=%d,%d\n",
		      wndPtr->hwndSelf, pt.x, pt.y );

    GetWindowRect (wndPtr->hwndSelf, &rect );
    if (!PtInRect( &rect, pt )) return HTNOWHERE;

    if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;

//    if (!(wndPtr->flags & WIN_MANAGED))
    {
        /* Check borders */
        if (HAS_SIZEFRAME( wndPtr->dwStyle ))
        {
            InflateRect( &rect, -SYSMETRICS_CXFRAME, -SYSMETRICS_CYFRAME );
/*            if (wndPtr->dwStyle & WS_BORDER) */
/*                InflateRect(&rect,-SYSMETRICS_CXBORDER,-SYSMETRICS_CYBORDER); */
            if (!PtInRect( &rect, pt ))
            {
                /* Check top sizing border */
                if (pt.y < rect.top)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTTOPLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTTOPRIGHT;
                    return HTTOP;
                }
                /* Check bottom sizing border */
                if (pt.y >= rect.bottom)
                {
                    if (pt.x < rect.left+SYSMETRICS_CXSIZE) return HTBOTTOMLEFT;
                    if (pt.x >= rect.right-SYSMETRICS_CXSIZE) return HTBOTTOMRIGHT;
                    return HTBOTTOM;
                }
                /* Check left sizing border */
                if (pt.x < rect.left)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPLEFT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMLEFT;
                    return HTLEFT;
                }
                /* Check right sizing border */
                if (pt.x >= rect.right)
                {
                    if (pt.y < rect.top+SYSMETRICS_CYSIZE) return HTTOPRIGHT;
                    if (pt.y >= rect.bottom-SYSMETRICS_CYSIZE) return HTBOTTOMRIGHT;
                    return HTRIGHT;
                }
            }
        }
        else  /* No thick frame */
        {
            if (HAS_FIXEDFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
                InflateRect(&rect, -SYSMETRICS_CXDLGFRAME, -SYSMETRICS_CYDLGFRAME);
/*            else if (wndPtr->dwStyle & WS_BORDER) */
/*                InflateRect(&rect, -SYSMETRICS_CXBORDER, -SYSMETRICS_CYBORDER); */
            if (!PtInRect( &rect, pt )) return HTBORDER;
        }

        /* Check caption */

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
	    if (wndPtr->dwExStyle & WS_EX_TOOLWINDOW)
	        rect.top += sysMetrics[SM_CYSMCAPTION] - 1;
	    else
	        rect.top += sysMetrics[SM_CYCAPTION] - 1;
            if (!PtInRect( &rect, pt ))
            {
                /* Check system menu */
                if ((wndPtr->dwStyle & WS_SYSMENU) &&
		    ((wndPtr->class->hIconSm) || (wndPtr->class->hIcon)))
                    rect.left += sysMetrics[SM_CYCAPTION] - 1;
                if (pt.x < rect.left) return HTSYSMENU;

                /* Check close button */
                if (wndPtr->dwStyle & WS_SYSMENU)
                    rect.right -= sysMetrics[SM_CYCAPTION] - 1;
                if (pt.x > rect.right) return HTCLOSE;

                /* Check maximize box */
                if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x > rect.right) return HTMAXBUTTON;

                /* Check minimize box */
                if (wndPtr->dwStyle & WS_MINIMIZEBOX)
                    rect.right -= SYSMETRICS_CXSIZE + 1;
                if (pt.x > rect.right) return HTMINBUTTON;
                return HTCAPTION;
            }
        }
    }

      /* Check client area */

    ScreenToClient( wndPtr->hwndSelf, &pt );
    GetClientRect( wndPtr->hwndSelf, &rect );
    if (PtInRect( &rect, pt )) return HTCLIENT;

      /* Check vertical scroll bar */

    if (wndPtr->dwStyle & WS_VSCROLL)
    {
	rect.right += SYSMETRICS_CXVSCROLL;
	if (PtInRect( &rect, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (wndPtr->dwStyle & WS_HSCROLL)
    {
	rect.bottom += SYSMETRICS_CYHSCROLL;
	if (PtInRect( &rect, pt ))
	{
	      /* Check size box */
	    if ((wndPtr->dwStyle & WS_VSCROLL) &&
		(pt.x >= rect.right - SYSMETRICS_CXVSCROLL))
		return HTSIZE;
	    return HTHSCROLL;
	}
    }

      /* Check menu bar */

    if (HAS_MENU(wndPtr))
    {
	if ((pt.y < 0) && (pt.x >= 0) && (pt.x < rect.right))
	    return HTMENU;
    }

      /* Should never get here */
    return HTERROR;
}


/***********************************************************************
 * NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LONG
NC_HandleNCHitTest (HWND hwnd , POINT pt)
{
    WND *wndPtr = WIN_FindWndPtr (hwnd);

    if (!wndPtr)
	return HTERROR;

    if (TWEAK_WineLook == WIN31_LOOK)
        return NC_DoNCHitTest (wndPtr, pt);
    else
	return NC_DoNCHitTest95 (wndPtr, pt);
}


/***********************************************************************
 *           NC_DrawSysButton
 */
void NC_DrawSysButton( HWND hwnd, HDC hdc, WINBOOL down )
{
    RECT rect;
    HDC hdcMem;
    HBITMAP hbitmap;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

//    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      hdcMem = CreateCompatibleDC( hdc );
      hbitmap = SelectObject( hdcMem, hbitmapClose );
      BitBlt(hdc, rect.left, rect.top, SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE,
               hdcMem, (wndPtr->dwStyle & WS_CHILD) ? SYSMETRICS_CXSIZE : 0, 0,
               down ? NOTSRCCOPY : SRCCOPY );
      SelectObject( hdcMem, hbitmap );
      DeleteDC( hdcMem );
    }
}


/***********************************************************************
 *           NC_DrawMaxButton
 */
static void NC_DrawMaxButton( HWND hwnd, HDC hdc, WINBOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdcMem;

//    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      hdcMem = CreateCompatibleDC( hdc );
      SelectObject( hdcMem,  (IsZoomed(hwnd) 
			     ? (down ? hbitmapRestoreD : hbitmapRestore)
			     : (down ? hbitmapMaximizeD : hbitmapMaximize)) );
      BitBlt( hdc, rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		SYSMETRICS_CXSIZE + 1, SYSMETRICS_CYSIZE, hdcMem, 0, 0,
		SRCCOPY );
      DeleteDC( hdcMem );
    }
}


/***********************************************************************
 *           NC_DrawMinButton
 */
static void NC_DrawMinButton( HWND hwnd, HDC hdc, WINBOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HDC hdcMem;

//    if( !(wndPtr->flags & WIN_MANAGED) )
    {
      NC_GetInsideRect( hwnd, &rect );
      hdcMem = CreateCompatibleDC( hdc );
      SelectObject( hdcMem, (down ? hbitmapMinimizeD : hbitmapMinimize) );
      if (wndPtr->dwStyle & WS_MAXIMIZEBOX) rect.right -= SYSMETRICS_CXSIZE+1;
      BitBlt( hdc, rect.right - SYSMETRICS_CXSIZE - 1, rect.top,
		SYSMETRICS_CXSIZE + 1, SYSMETRICS_CYSIZE, hdcMem, 0, 0,
		SRCCOPY );
      DeleteDC( hdcMem );
    }
}


/******************************************************************************
 *
 *   void  NC_DrawSysButton95(
 *      HWND  hwnd,
 *      HDC  hdc,
 *      WINBOOL  down )
 *
 *   Draws the Win95 system icon.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation from NC_DrawSysButton source.
 *        11-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Fixed most bugs.
 *
 *****************************************************************************/

WINBOOL
NC_DrawSysButton95 (HWND hwnd, HDC hdc, WINBOOL down)
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

//    if( !(wndPtr->flags & WIN_MANAGED) )
    {
	HICON  hIcon = 0;
	RECT rect;

	NC_GetInsideRect95( hwnd, &rect );

	if (wndPtr->class->hIconSm)
	    hIcon = wndPtr->class->hIconSm;
	else if (wndPtr->class->hIcon)
	    hIcon = wndPtr->class->hIcon;

	if (hIcon)
	    DrawIconEx(hdc, rect.left + 2, rect.top + 2, hIcon,
			  sysMetrics[SM_CXSMICON],
			  sysMetrics[SM_CYSMICON],
			  0, 0, DI_NORMAL);

	return (hIcon != 0);
    }
    return FALSE;
}


/******************************************************************************
 *
 *   void  NC_DrawCloseButton95(
 *      HWND  hwnd,
 *      HDC  hdc,
 *      WINBOOL  down )
 *
 *   Draws the Win95 close button.
 *
 *   Revision history
 *        11-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Original implementation from NC_DrawSysButton95 source.
 *
 *****************************************************************************/

void
NC_DrawCloseButton95 (HWND hwnd, HDC hdc, WINBOOL down)
{
    RECT rect;
    HDC hdcMem;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

//    if( !(wndPtr->flags & WIN_MANAGED) )
    {
	BITMAP bmp;
	HBITMAP hBmp, hOldBmp;

	NC_GetInsideRect95( hwnd, &rect );

	hdcMem = CreateCompatibleDC( hdc );
	hBmp = /*down ? hbitmapCloseD :*/ hbitmapClose;
	hOldBmp = SelectObject (hdcMem, hBmp);
	GetObject(hBmp, sizeof(BITMAP), &bmp);
	BitBlt(hdc, rect.right - (sysMetrics[SM_CYCAPTION] + 1 + bmp.bmWidth) / 2,
		  rect.top + (sysMetrics[SM_CYCAPTION] - 1 - bmp.bmHeight) / 2,
		  bmp.bmWidth, bmp.bmHeight, hdcMem, 0, 0, down ? NOTSRCCOPY : SRCCOPY);

	SelectObject(hdcMem, hOldBmp);
	DeleteDC (hdcMem);
    }
}


/******************************************************************************
 *
 *   NC_DrawMaxButton95(
 *      HWND  hwnd,
 *      HDC  hdc,
 *      WINBOOL  down )
 *
 *   Draws the maximize button for Win95 style windows.
 *
 *   Bugs
 *        Many.  Spacing might still be incorrect.  Need to fit a close
 *        button between the max button and the edge.
 *        Should scale the image with the title bar.  And more...
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static void  NC_DrawMaxButton95(
    HWND hwnd,
    HDC hdc,
    WINBOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SIZE  bmsz;
    HBITMAP  bm;
    BITMAP bmp;
    HDC hdcMem;


//    if( !(wndPtr->flags & WIN_MANAGED) ) 
      {
	GetBitmapDimensionEx((bm = IsZoomed(hwnd) ?
			(down ? hbitmapRestoreD : hbitmapRestore ) :
			(down ? hbitmapMaximizeD : hbitmapMaximize)),
			 &bmsz);

	if ( bmsz.cx == 0 || bmsz.cy == 0 ) {
		GetObject(bm, sizeof(BITMAP), &bmp);
		bmsz.cx = bmp.bmWidth;
		bmsz.cy = bmp.bmHeight;
		DPRINT("WARN GetBitmapDimensionEx returned 0 size ");
	}

	NC_GetInsideRect95( hwnd, &rect );

	if (wndPtr->dwStyle & WS_SYSMENU)
	    rect.right -= sysMetrics[SM_CYCAPTION] + 1;
	
	hdcMem = CreateCompatibleDC( hdc );
	SelectObject( hdc, bm );
	BitBlt( hdc, rect.right - (sysMetrics[SM_CXSIZE] + bmsz.cx) / 2,
		  rect.top + (sysMetrics[SM_CYCAPTION] - 1 - bmsz.cy) / 2,
		  bmsz.cx, bmsz.cy, hdcMem, 0, 0, SRCCOPY );
	DeleteDC( hdcMem );
    }

    return;
}


/******************************************************************************
 *
 *   NC_DrawMinButton95(
 *      HWND  hwnd,
 *      HDC  hdc,
 *      WINBOOL  down )
 *
 *   Draws the minimize button for Win95 style windows.
 *
 *   Bugs
 *        Many.  Spacing is still incorrect.  Should scale the image with the
 *        title bar.  And more...
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

static void  NC_DrawMinButton95(
    HWND hwnd,
    HDC hdc,
    WINBOOL down )
{
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SIZE  bmsz;
    HBITMAP  bm;
    HDC hdcMem;
// !(wndPtr->flags & WIN_MANAGED) &&
    if( GetBitmapDimensionEx((bm = down ? hbitmapMinimizeD :
				hbitmapMinimize), &bmsz)) {
	
	NC_GetInsideRect95( hwnd, &rect );

	if (wndPtr->dwStyle & WS_SYSMENU)
	    rect.right -= sysMetrics[SM_CYCAPTION] + 1;

	if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
	    rect.right += -1 -
		(sysMetrics[SM_CXSIZE] + bmsz.cx) / 2;

	hdcMem = CreateCompatibleDC( hdc );
	SelectObject( hdc, bm );
	BitBlt( hdc, rect.right - (sysMetrics[SM_CXSIZE] + bmsz.cx) / 2,
		  rect.top + (sysMetrics[SM_CYCAPTION] - 1 - bmsz.cy) / 2,
		  bmsz.cx, bmsz.cy, hdcMem, 0, 0, SRCCOPY );
	DeleteDC( hdcMem );
    }

    return;
}


/***********************************************************************
 *           NC_DrawFrame
 *
 * Draw a window frame inside the given rectangle, and update the rectangle.
 * The correct pen for the frame must be selected in the DC.
 */
static void NC_DrawFrame( HDC hdc, RECT *rect, WINBOOL dlgFrame,
                          WINBOOL active )
{
    INT width, height;

 //   if (TWEAK_WineLook != WIN31_LOOK)
//	DPRINT( "Called in Win95 mode. Aiee! Please report this.\n" );

    if (dlgFrame)
    {
	width = SYSMETRICS_CXDLGFRAME - 1;
	height = SYSMETRICS_CYDLGFRAME - 1;
        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
						COLOR_INACTIVECAPTION) );
    }
    else
    {
	width = SYSMETRICS_CXFRAME - 1;
	height = SYSMETRICS_CYFRAME - 1;
        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
						COLOR_INACTIVEBORDER) );
    }

      /* Draw frame */
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, height, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
              width, rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom,
              rect->right - rect->left, -height, PATCOPY );
    PatBlt( hdc, rect->right, rect->top,
              -width, rect->bottom - rect->top, PATCOPY );

    if (dlgFrame)
    {
	InflateRect( rect, -width, -height );
    } 
    else
    {
        INT decYOff = SYSMETRICS_CXFRAME + SYSMETRICS_CXSIZE;
	INT decXOff = SYSMETRICS_CYFRAME + SYSMETRICS_CYSIZE;

      /* Draw inner rectangle */

	SelectObject( hdc, GetStockObject(NULL_BRUSH) );
	Rectangle( hdc, rect->left + width, rect->top + height,
		     rect->right - width , rect->bottom - height );

      /* Draw the decorations */

	MoveToEx( hdc, rect->left, rect->top + decYOff, NULL );
	LineTo( hdc, rect->left + width, rect->top + decYOff );
	MoveToEx( hdc, rect->right - 1, rect->top + decYOff, NULL );
	LineTo( hdc, rect->right - width - 1, rect->top + decYOff );
	MoveToEx( hdc, rect->left, rect->bottom - decYOff, NULL );
	LineTo( hdc, rect->left + width, rect->bottom - decYOff );
	MoveToEx( hdc, rect->right - 1, rect->bottom - decYOff, NULL );
	LineTo( hdc, rect->right - width - 1, rect->bottom - decYOff );

	MoveToEx( hdc, rect->left + decXOff, rect->top, NULL );
	LineTo( hdc, rect->left + decXOff, rect->top + height);
	MoveToEx( hdc, rect->left + decXOff, rect->bottom - 1, NULL );
	LineTo( hdc, rect->left + decXOff, rect->bottom - height - 1 );
	MoveToEx( hdc, rect->right - decXOff, rect->top, NULL );
	LineTo( hdc, rect->right - decXOff, rect->top + height );
	MoveToEx( hdc, rect->right - decXOff, rect->bottom - 1, NULL );
	LineTo( hdc, rect->right - decXOff, rect->bottom - height - 1 );

	InflateRect( rect, -width - 1, -height - 1 );
    }
}


/******************************************************************************
 *
 *   void  NC_DrawFrame95(
 *      HDC  hdc,
 *      RECT  *rect,
 *      WINBOOL  dlgFrame,
 *      WINBOOL  active )
 *
 *   Draw a window frame inside the given rectangle, and update the rectangle.
 *   The correct pen for the frame must be selected in the DC.
 *
 *   Bugs
 *        Many.  First, just what IS a frame in Win95?  Note that the 3D look
 *        on the outer edge is handled by NC_DoNCPaint95.  As is the inner
 *        edge.  The inner rectangle just inside the frame is handled by the
 *        Caption code.
 *
 *        In short, for most people, this function should be a nop (unless
 *        you LIKE thick borders in Win95/NT4.0 -- I've been working with
 *        them lately, but just to get this code right).  Even so, it doesn't
 *        appear to be so.  It's being worked on...
 * 
 *   Revision history
 *        06-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation (based on NC_DrawFrame)
 *        02-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Some minor fixes.
 *
 *****************************************************************************/

static void  NC_DrawFrame95(
    HDC  hdc,
    RECT  *rect,
    WINBOOL  dlgFrame,
    WINBOOL  active )
{
    INT width, height;

    if (dlgFrame)
    {
	width = sysMetrics[SM_CXDLGFRAME] - sysMetrics[SM_CXEDGE];
	height = sysMetrics[SM_CYDLGFRAME] - sysMetrics[SM_CYEDGE];
    }
    else
    {
	width = sysMetrics[SM_CXFRAME] - sysMetrics[SM_CXEDGE];
	height = sysMetrics[SM_CYFRAME] - sysMetrics[SM_CYEDGE];
    }

    SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
		COLOR_INACTIVEBORDER) );

    /* Draw frame */
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, height, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
              width, rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom,
              rect->right - rect->left, -height, PATCOPY );
    PatBlt( hdc, rect->right, rect->top,
              -width, rect->bottom - rect->top, PATCOPY );

    InflateRect( rect, -width, -height );
}



/***********************************************************************
 *           NC_DrawMovingFrame
 *
 * Draw the frame used when moving or resizing window.
 *
 * FIXME:  This causes problems in Win95 mode.  (why?)
 */
static void NC_DrawMovingFrame( HDC hdc, RECT *rect, WINBOOL thickframe )
{
    if (thickframe)
    {
        FastWindowFrame( hdc, rect, SYSMETRICS_CXFRAME,
                         SYSMETRICS_CYFRAME, PATINVERT );
    }
    else DrawFocusRect( hdc, rect );
}


/***********************************************************************
 *           NC_DrawCaption
 *
 * Draw the window caption.
 * The correct pen for the window frame must be selected in the DC.
 */
static void NC_DrawCaption( HDC hdc, RECT *rect, HWND hwnd,
			    DWORD style, WINBOOL active )
{
    RECT r = *rect;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    char buffer[256];

//    if (wndPtr->flags & WIN_MANAGED) return;

    if (!hbitmapClose)
    {
	if (!(hbitmapClose = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_CLOSE) )))
	    return;
	hbitmapCloseD    = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_CLOSE) );
	hbitmapMinimize  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_RESTORED) );
    }
    
    if (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME)
    {
        HBRUSH hbrushOld = SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW) );
	PatBlt( hdc, r.left, r.top, 1, r.bottom-r.top+1,PATCOPY );
	PatBlt( hdc, r.right-1, r.top, 1, r.bottom-r.top+1, PATCOPY );
	PatBlt( hdc, r.left, r.top-1, r.right-r.left, 1, PATCOPY );
	r.left++;
	r.right--;
	SelectObject( hdc, hbrushOld );
    }

    MoveToEx( hdc, r.left, r.bottom,NULL );
    LineTo( hdc, r.right, r.bottom );

    if (style & WS_SYSMENU)
    {
	NC_DrawSysButton( hwnd, hdc, FALSE );
	r.left += SYSMETRICS_CXSIZE + 1;
	MoveToEx( hdc, r.left - 1, r.top,NULL );
	LineTo( hdc, r.left - 1, r.bottom );
    }
    if (style & WS_MAXIMIZEBOX)
    {
	NC_DrawMaxButton( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MINIMIZEBOX)
    {
	NC_DrawMinButton( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }

    FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) );

    if (GetWindowTextA( hwnd, buffer, sizeof(buffer) ))
    {
	if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
	SetBkMode( hdc, TRANSPARENT );
	DrawTextA( hdc, buffer, -1, &r,
                     DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX );

	
    }
}


/******************************************************************************
 *
 *   NC_DrawCaption95(
 *      HDC  hdc,
 *      RECT *rect,
 *      HWND hwnd,
 *      DWORD  style,
 *      WINBOOL active )
 *
 *   Draw the window caption for Win95 style windows.
 *   The correct pen for the window frame must be selected in the DC.
 *
 *   Bugs
 *        Hey, a function that finally works!  Well, almost.
 *        It's being worked on.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *        02-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Some minor fixes.
 *
 *****************************************************************************/

void  NC_DrawCaption95(
    HDC  hdc,
    RECT *rect,
    HWND hwnd,
    DWORD  style,
    DWORD  exStyle,
    WINBOOL active )
{
    RECT  r = *rect;
    WND     *wndPtr = WIN_FindWndPtr( hwnd );
    char    buffer[256];
    HPEN  hPrevPen;
    int txtlen;

//    if (wndPtr->flags & WIN_MANAGED) return;

    hPrevPen = SelectObject( hdc, GetSysColorPen(COLOR_3DFACE) );
    MoveToEx( hdc, r.left, r.bottom - 1, NULL );
    LineTo( hdc, r.right, r.bottom - 1 );
    SelectObject( hdc, hPrevPen );
    r.bottom - 2;
   

    FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) );

    if (!hbitmapClose) {
	if (!(hbitmapClose = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_CLOSE) )))
	    return;
	hbitmapMinimize  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_REDUCE) );
	hbitmapMinimizeD = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_REDUCED) );
	hbitmapMaximize  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_ZOOM) );
	hbitmapMaximizeD = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_ZOOMD) );
	hbitmapRestore   = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_RESTORE) );
	hbitmapRestoreD  = LoadBitmapA( 0, MAKEINTRESOURCEA(OBM_RESTORED) );
    }

    if ((style & WS_SYSMENU) && !(exStyle & WS_EX_TOOLWINDOW)) {
	if (NC_DrawSysButton95 (hwnd, hdc, FALSE))
	    r.left += sysMetrics[SM_CYCAPTION] - 1;
    }
    if (style & WS_SYSMENU) {
	NC_DrawCloseButton95 (hwnd, hdc, FALSE);
	r.right -= sysMetrics[SM_CYCAPTION] - 1;
    }
    if (style & WS_MAXIMIZEBOX) {
	NC_DrawMaxButton95( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }
    if (style & WS_MINIMIZEBOX) {
	NC_DrawMinButton95( hwnd, hdc, FALSE );
	r.right -= SYSMETRICS_CXSIZE + 1;
    }
   
    if ( wndPtr->class->bUnicode )
    	txtlen = GetWindowTextW( hwnd, buffer, sizeof(buffer) );
    else
    	txtlen = GetWindowTextA( hwnd, buffer, sizeof(buffer) );


    if (txtlen > 0 && txtlen <  sizeof(buffer)  ) {
	NONCLIENTMETRICS nclm;
	HFONT hFont, hOldFont;
	nclm.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	if (exStyle & WS_EX_TOOLWINDOW)
	    hFont = CreateFontIndirectA (&nclm.lfSmCaptionFont);
	else
	    hFont = CreateFontIndirectA (&nclm.lfCaptionFont);
	hOldFont = SelectObject (hdc, hFont);
	if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
	SetBkMode( hdc, TRANSPARENT );
	r.left += 2;
	if ( wndPtr->class->bUnicode )
		DrawTextW( hdc, buffer, -1, &r,
		     DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT );
	else {
		DrawTextA( hdc, buffer, -1, &r,
		     DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT );
	}
	DeleteObject(SelectObject (hdc, hOldFont));
    }
}



/***********************************************************************
 *           NC_DoNCPaint
 *
 * Paint the non-client area. clip is currently unused.
 */
void NC_DoNCPaint( WND* wndPtr, HRGN clip, WINBOOL suppress_menupaint )
{
    HDC hdc;
    RECT rect;
    WINBOOL active;
    HWND hwnd = wndPtr->hwndSelf;

    if ( wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return; /* Nothing to do */

    active  = wndPtr->flags & WIN_NCACTIVATED;

    DPRINT( "%04x %d\n", hwnd, active );

    if (!(hdc = GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW ))) return;

    if (ExcludeClipRect( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC( hwnd, hdc );
	return;
    }

    rect.top = rect.left = 0;
    rect.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;


    SelectObject( hdc, GetSysColorPen(COLOR_WINDOWFRAME) );

//    if (!(wndPtr->flags & WIN_MANAGED))
    {
        if ((wndPtr->dwStyle & WS_BORDER) || (wndPtr->dwStyle & WS_DLGFRAME) ||
            (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME))
        {
	    SelectObject( hdc, GetStockObject(NULL_BRUSH) );
            Rectangle( hdc, rect.top, rect.left, rect.right, rect.bottom );
            InflateRect( &rect, -1, -1 );
        }

        if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle )) 
            NC_DrawFrame( hdc, &rect, TRUE, active );
        else if (wndPtr->dwStyle & WS_THICKFRAME)
            NC_DrawFrame(hdc, &rect, FALSE, active );

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            RECT r = rect;
            r.bottom = rect.top + SYSMETRICS_CYSIZE;
            rect.top += SYSMETRICS_CYSIZE + SYSMETRICS_CYBORDER;
            NC_DrawCaption( hdc, &r, hwnd, wndPtr->dwStyle, active );
        }
    }

    if (HAS_MENU(wndPtr))
    {
	RECT r = rect;
	r.bottom = rect.top + SYSMETRICS_CYMENU;  /* default height */
	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint );
    }

      /* Draw the scroll-bars */

    if (wndPtr->dwStyle & WS_VSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_VERT, TRUE, TRUE );
    if (wndPtr->dwStyle & WS_HSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_HORZ, TRUE, TRUE );

      /* Draw the "size-box" */

    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL))
    {
        RECT r = rect;
        r.left = r.right - SYSMETRICS_CXVSCROLL + 1;
        r.top  = r.bottom - SYSMETRICS_CYHSCROLL + 1;
	if(wndPtr->dwStyle & WS_BORDER) {
	  r.left++;
	  r.top++;
	}
        FillRect( hdc, &r, GetSysColorBrush(COLOR_SCROLLBAR) );
    }    

    ReleaseDC( hwnd, hdc );
}


/******************************************************************************
 *
 *   void  NC_DoNCPaint95(
 *      WND  *wndPtr,
 *      HRGN  clip,
 *      WINBOOL  suppress_menupaint )
 *
 *   Paint the non-client area for Win95 windows.  The clip region is
 *   currently ignored.
 *
 *   Bugs
 *        grep -E -A10 -B5 \(95\)\|\(Bugs\)\|\(FIXME\) windows/nonclient.c \
 *           misc/tweak.c controls/menu.c  # :-)
 *
 *   Revision history
 *        03-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation
 *        10-Jun-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Fixed some bugs.
 *
 *****************************************************************************/

void  NC_DoNCPaint95(
    WND  *wndPtr,
    HRGN  clip,
    WINBOOL  suppress_menupaint )
{
    HDC hdc;
    RECT rect;
    WINBOOL active;
    HWND hwnd = wndPtr->hwndSelf;

    if ( wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return; /* Nothing to do */

    active  = wndPtr->flags & WIN_NCACTIVATED;

    DPRINT( "%04x %d\n", hwnd, active );

    if (!(hdc = GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW ))) return;

    if (ExcludeClipRect( hdc, wndPtr->rectClient.left -wndPtr->rectWindow.left,
		        wndPtr->rectClient.top  -wndPtr->rectWindow.top,
		        wndPtr->rectClient.right  -wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom  -wndPtr->rectWindow.top )
	== NULLREGION)
    {
	ReleaseDC( hwnd, hdc );
	return;
    }



    rect.top = rect.left = 0;
    rect.right  = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
    rect.bottom = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;

    //rect = wndPtr->rectWindow;

    printf("::%d %d\n", rect.right, rect.bottom);


    SelectObject( hdc, GetSysColorPen(COLOR_WINDOWFRAME) );

    //if((wndPtr->flags & WIN_MANAGED)) {
        if ((wndPtr->dwStyle & WS_BORDER) && ((wndPtr->dwStyle & WS_DLGFRAME) ||
	    (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME) || (wndPtr->dwStyle & WS_THICKFRAME))) {
            DrawEdge(hdc, &rect, EDGE_RAISED, BF_RECT | BF_ADJUST);
        }

        if (HAS_FIXEDFRAME( wndPtr->dwStyle, wndPtr->dwExStyle )) 
            NC_DrawFrame95( hdc, &rect, TRUE, active );
        else if (wndPtr->dwStyle & WS_THICKFRAME)
            NC_DrawFrame95(hdc, &rect, FALSE, active );

        if ((wndPtr->dwStyle & WS_CAPTION) == WS_CAPTION)
        {
            RECT  r = rect;
	    if (wndPtr->dwExStyle & WS_EX_TOOLWINDOW) {
		r.bottom = rect.top + sysMetrics[SM_CYSMCAPTION];
		rect.top += sysMetrics[SM_CYSMCAPTION];
	    }
	    else {
		r.bottom = rect.top + sysMetrics[SM_CYCAPTION];
		rect.top += sysMetrics[SM_CYCAPTION];
	    }
            NC_DrawCaption95 (hdc, &r, hwnd, wndPtr->dwStyle,
                              wndPtr->dwExStyle, active);
        }
    //}

    if (HAS_MENU(wndPtr))
    {
	RECT r = rect;
	r.bottom = rect.top + sysMetrics[SM_CYMENU];
	
	DPRINT( "Calling DrawMenuBar with "
			  "rect (%d, %d)-(%d, %d)\n", r.left, r.top,
			  r.right, r.bottom);

	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint ) + 1;
    }

    DPRINT( "After MenuBar, rect is (%d, %d)-(%d, %d).\n",
		       rect.left, rect.top, rect.right, rect.bottom );

    if (wndPtr->dwExStyle & WS_EX_CLIENTEDGE)
	DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    if (wndPtr->dwExStyle & WS_EX_STATICEDGE)
	DrawEdge(hdc, &rect, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);

    /* Draw the scroll-bars */

    if (wndPtr->dwStyle & WS_VSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_VERT, TRUE, TRUE );
    if (wndPtr->dwStyle & WS_HSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_HORZ, TRUE, TRUE );

    /* Draw the "size-box" */
    if ((wndPtr->dwStyle & WS_VSCROLL) && (wndPtr->dwStyle & WS_HSCROLL))
    {
        RECT r = rect;
        r.left = r.right - SYSMETRICS_CXVSCROLL + 1;
        r.top  = r.bottom - SYSMETRICS_CYHSCROLL + 1;
        FillRect( hdc, &r,  GetSysColorBrush(COLOR_SCROLLBAR) );
    }    

    ReleaseDC( hwnd, hdc );
}




/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LONG NC_HandleNCPaint( HWND hwnd , HRGN clip)
{
    WND* wndPtr = WIN_FindWndPtr( hwnd );

    //if( wndPtr && wndPtr->dwStyle & WS_VISIBLE )
    {
	if( wndPtr->dwStyle & WS_MINIMIZE )
	    WINPOS_RedrawIconTitle( hwnd );
	else if (TWEAK_WineLook == WIN31_LOOK)
	    NC_DoNCPaint( wndPtr, clip, FALSE );
	else
	    NC_DoNCPaint95( wndPtr, clip, FALSE );
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleNCActivate
 *
 * Handle a WM_NCACTIVATE message. Called from DefWindowProc().
 */
LONG NC_HandleNCActivate( WND *wndPtr, WPARAM wParam )
{
    WORD wStateChange;

//    if( wParam ) wStateChange = !(wndPtr->flags & WIN_NCACTIVATED);
//    else wStateChange = wndPtr->flags & WIN_NCACTIVATED;

//    if( wStateChange )
    {
	if (wParam) wndPtr->flags |= WIN_NCACTIVATED;
	else wndPtr->flags &= ~WIN_NCACTIVATED;

	if( wndPtr->dwStyle & WS_MINIMIZE ) 
	    WINPOS_RedrawIconTitle( wndPtr->hwndSelf );
	else if (TWEAK_WineLook == WIN31_LOOK)
	    NC_DoNCPaint( wndPtr, (HRGN)1, FALSE );
	else
	    NC_DoNCPaint95( wndPtr, (HRGN)1, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           NC_HandleSetCursor
 *
 * Handle a WM_SETCURSOR message. Called from DefWindowProc().
 */
LONG NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if (hwnd != (HWND)wParam) return 0;  /* Don't set the cursor for child windows */

    switch(LOWORD(lParam))
    {
    case HTERROR:
	{
	    WORD msg = HIWORD( lParam );
	    if ((msg == WM_LBUTTONDOWN) || (msg == WM_MBUTTONDOWN) ||
		(msg == WM_RBUTTONDOWN))
		MessageBeep(0);
	}
	break;

    case HTCLIENT:
	{
	    WND *wndPtr;
	    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) break;
	    if (wndPtr->class->hCursor)
	    {
		SetCursor( wndPtr->class->hCursor );
		return TRUE;
	    }
	    else return FALSE;
	}

    case HTLEFT:
    case HTRIGHT:
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZEWE ) );

    case HTTOP:
    case HTBOTTOM:
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZENS ) );

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:	
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZENWSE ) );

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
	return (LONG)SetCursor( LoadCursorA( 0, IDC_SIZENESW ) );
    }

    /* Default cursor: arrow */
    return (LONG)SetCursor( LoadCursorA( 0, IDC_ARROW ) );
}

/***********************************************************************
 *           NC_GetSysPopupPos
 */
WINBOOL NC_GetSysPopupPos( WND* wndPtr, RECT* rect )
{
  if( wndPtr->hSysMenu )
  {
      if( wndPtr->dwStyle & WS_MINIMIZE )
	  GetWindowRect( wndPtr->hwndSelf, rect );
      else
      {
          if (TWEAK_WineLook == WIN31_LOOK)
              NC_GetInsideRect( wndPtr->hwndSelf, rect );
          else
              NC_GetInsideRect95( wndPtr->hwndSelf, rect );
  	  OffsetRect( rect, wndPtr->rectWindow.left, wndPtr->rectWindow.top);
  	  if (wndPtr->dwStyle & WS_CHILD)
     	      ClientToScreen( wndPtr->parent->hwndSelf, (POINT *)rect );
          if (TWEAK_WineLook == WIN31_LOOK) {
            rect->right = rect->left + SYSMETRICS_CXSIZE;
            rect->bottom = rect->top + SYSMETRICS_CYSIZE;
	  }
	  else {
            rect->right = rect->left + sysMetrics[SM_CYCAPTION] - 1;
            rect->bottom = rect->top + sysMetrics[SM_CYCAPTION] - 1;
	  }
      }
      return TRUE;
  }
  return FALSE;
}

/***********************************************************************
 *           NC_StartSizeMove
 *
 * Initialisation of a move or resize, when initiatied from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG NC_StartSizeMove( WND* wndPtr, WPARAM wParam,
                              POINT *capturePoint )
{
    LONG hittest = 0;
    POINT pt;
    MSG msg;

    if ((wParam & 0xfff0) == SC_MOVE)
    {
	  /* Move pointer at the center of the caption */
	RECT rect;
        if (TWEAK_WineLook == WIN31_LOOK)
            NC_GetInsideRect( wndPtr->hwndSelf, &rect );
        else
            NC_GetInsideRect95( wndPtr->hwndSelf, &rect );
	if (wndPtr->dwStyle & WS_SYSMENU)
	    rect.left += SYSMETRICS_CXSIZE + 1;
	if (wndPtr->dwStyle & WS_MINIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;
	if (wndPtr->dwStyle & WS_MAXIMIZEBOX)
	    rect.right -= SYSMETRICS_CXSIZE + 1;
	pt.x = wndPtr->rectWindow.left + (rect.right - rect.left) / 2;
	pt.y = wndPtr->rectWindow.top + rect.top + SYSMETRICS_CYSIZE/2;
	hittest = HTCAPTION;
	*capturePoint = pt;
    }
    else  /* SC_SIZE */
    {
	while(!hittest)
	{
            MSG_InternalGetMessage( &msg, 0, 0, MSGF_SIZE, PM_REMOVE, FALSE );
	    switch(msg.message)
	    {
	    case WM_MOUSEMOVE:
		hittest = NC_HandleNCHitTest( wndPtr->hwndSelf, msg.pt );
		pt = msg.pt;
		if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT))
		    hittest = 0;
		break;

	    case WM_LBUTTONUP:
		return 0;

	    case WM_KEYDOWN:
		switch(msg.wParam)
		{
		case VK_UP:
		    hittest = HTTOP;
		    pt.x =(wndPtr->rectWindow.left+wndPtr->rectWindow.right)/2;
		    pt.y = wndPtr->rectWindow.top + SYSMETRICS_CYFRAME / 2;
		    break;
		case VK_DOWN:
		    hittest = HTBOTTOM;
		    pt.x =(wndPtr->rectWindow.left+wndPtr->rectWindow.right)/2;
		    pt.y = wndPtr->rectWindow.bottom - SYSMETRICS_CYFRAME / 2;
		    break;
		case VK_LEFT:
		    hittest = HTLEFT;
		    pt.x = wndPtr->rectWindow.left + SYSMETRICS_CXFRAME / 2;
		    pt.y =(wndPtr->rectWindow.top+wndPtr->rectWindow.bottom)/2;
		    break;
		case VK_RIGHT:
		    hittest = HTRIGHT;
		    pt.x = wndPtr->rectWindow.right - SYSMETRICS_CXFRAME / 2;
		    pt.y =(wndPtr->rectWindow.top+wndPtr->rectWindow.bottom)/2;
		    break;
		case VK_RETURN:
		case VK_ESCAPE: return 0;
		}
	    }
	}
	*capturePoint = pt;
    }
    SetCursorPos( pt.x, pt.y );
    NC_HandleSetCursor( wndPtr->hwndSelf, 
			wndPtr->hwndSelf, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           NC_DoSizeMove
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
static void NC_DoSizeMove( HWND hwnd, WORD wParam )
{
    MSG msg;
    RECT sizingRect, mouseRect;
    HDC hdc;
    LONG hittest = (LONG)(wParam & 0x0f);
    HCURSOR hDragCursor = 0, hOldCursor = 0;
    POINT minTrack, maxTrack;
    POINT capturePoint, pt;
    WND *     wndPtr = WIN_FindWndPtr( hwnd );
    WINBOOL    thickframe = HAS_THICKFRAME( wndPtr->dwStyle );
    WINBOOL    iconic = wndPtr->dwStyle & WS_MINIMIZE;
    WINBOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();

    capturePoint = pt = *(POINT*)&dwPoint;

// || (wndPtr->flags & WIN_MANAGED)
    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd) ) 
	return;

    if ((wParam & 0xfff0) == SC_MOVE)
    {
	if (!(wndPtr->dwStyle & WS_CAPTION)) return;
	if (!hittest) 
	     hittest = NC_StartSizeMove( wndPtr, wParam, &capturePoint );
	if (!hittest) return;
    }
    else  /* SC_SIZE */
    {
	if (!thickframe) return;
	if ( hittest && hittest != HTSYSMENU ) hittest += 2;
	else
	{
	    SetCapture(hwnd);
	    hittest = NC_StartSizeMove( wndPtr, wParam, &capturePoint );
	    if (!hittest)
	    {
		ReleaseCapture();
		return;
	    }
	}
    }

      /* Get min/max info */

    WINPOS_GetMinMaxInfo( wndPtr, NULL, NULL, &minTrack, &maxTrack );
    sizingRect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	GetClientRect( wndPtr->parent->hwndSelf, &mouseRect );
    else 
        SetRect(&mouseRect, 0, 0, SYSMETRICS_CXSCREEN, SYSMETRICS_CYSCREEN);
    if (ON_LEFT_BORDER(hittest))
    {
	mouseRect.left  = max( mouseRect.left, sizingRect.right-maxTrack.x );
	mouseRect.right = min( mouseRect.right, sizingRect.right-minTrack.x );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
	mouseRect.left  = max( mouseRect.left, sizingRect.left+minTrack.x );
	mouseRect.right = min( mouseRect.right, sizingRect.left+maxTrack.x );
    }
    if (ON_TOP_BORDER(hittest))
    {
	mouseRect.top    = max( mouseRect.top, sizingRect.bottom-maxTrack.y );
	mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minTrack.y);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
	mouseRect.top    = max( mouseRect.top, sizingRect.top+minTrack.y );
	mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+maxTrack.y );
    }
    if (wndPtr->dwStyle & WS_CHILD)
    {
	MapWindowPoints( wndPtr->parent->hwndSelf, 0, 
		(LPPOINT)&mouseRect, 2 );
    }
    SendMessageA( hwnd, WM_ENTERSIZEMOVE, 0, 0 );

    if (GetCapture() != hwnd) SetCapture( hwnd );    

    if (wndPtr->dwStyle & WS_CHILD)
    {
          /* Retrieve a default cache DC (without using the window style) */
        hdc = GetDCEx( wndPtr->parent->hwndSelf, 0, DCX_CACHE );
    }
    else
    {  /* Grab the server only when moving top-level windows without desktop */
	hdc = GetDC( 0 );
    }

//    wndPtr->pDriver->pPreSizeMove(wndPtr);

    if( iconic ) /* create a cursor for dragging */
    {
	HICON hIcon = (wndPtr->class->hIcon) ? wndPtr->class->hIcon
                      : (HICON)SendMessageA( hwnd, WM_QUERYDRAGICON, 0, 0L);
//	if( hIcon ) hDragCursor =  CURSORICON_IconToCursor( hIcon, TRUE );
	if( !hDragCursor ) iconic = FALSE;
    }

    if( !iconic ) NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    while(1)
    {
        int dx = 0, dy = 0;

        MSG_InternalGetMessage( &msg, 0, 0, MSGF_SIZE, PM_REMOVE, FALSE );

	  /* Exit on button-up, Return, or Esc */
	if ((msg.message == WM_LBUTTONUP) ||
	    ((msg.message == WM_KEYDOWN) && 
	     ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

	if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
	    continue;  /* We are not interested in other messages */

	dwPoint = GetMessagePos ();
	pt = *(POINT*)&dwPoint;
	
	if (msg.message == WM_KEYDOWN) switch(msg.wParam)
	{
	    case VK_UP:    pt.y -= 8; break;
	    case VK_DOWN:  pt.y += 8; break;
	    case VK_LEFT:  pt.x -= 8; break;
	    case VK_RIGHT: pt.x += 8; break;		
	}

	pt.x = max( pt.x, mouseRect.left );
	pt.x = min( pt.x, mouseRect.right );
	pt.y = max( pt.y, mouseRect.top );
	pt.y = min( pt.y, mouseRect.bottom );

	dx = pt.x - capturePoint.x;
	dy = pt.y - capturePoint.y;

	if (dx || dy)
	{
	    if( !moved )
	    {
		moved = TRUE;
        	if( iconic ) /* ok, no system popup tracking */
		{
		    hOldCursor = SetCursor(hDragCursor);
		    ShowCursor( TRUE );
		    WINPOS_ShowIconTitle( wndPtr, FALSE );
		}
	    }

	    if (msg.message == WM_KEYDOWN) SetCursorPos( pt.x, pt.y );
	    else
	    {
		RECT newRect = sizingRect;

		if (hittest == HTCAPTION) OffsetRect( &newRect, dx, dy );
		if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
		else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
		if (ON_TOP_BORDER(hittest)) newRect.top += dy;
		else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
		if( !iconic )
		{
		    NC_DrawMovingFrame( hdc, &sizingRect, thickframe );
		    NC_DrawMovingFrame( hdc, &newRect, thickframe );
		}
		capturePoint = pt;
		sizingRect = newRect;
	    }
	}
    }

    ReleaseCapture();
    if( iconic )
    {
	if( moved ) /* restore cursors, show icon title later on */
	{
	    ShowCursor( FALSE );
	    SetCursor( hOldCursor );
	}
        DestroyCursor( hDragCursor );
    }
    else
	NC_DrawMovingFrame( hdc, &sizingRect, thickframe );

    if (wndPtr->dwStyle & WS_CHILD)
        ReleaseDC( wndPtr->parent->hwndSelf, hdc );
    else
    {
	ReleaseDC( 0, hdc );
    }

//    wndPtr->pDriver->pPostSizeMove(wndPtr);

    if (HOOK_IsHooked( WH_CBT ))
    {
	if ( wndPtr->class->bUnicode == FALSE ) {
		if( HOOK_CallHooksA( WH_CBT, HCBT_MOVESIZE,(WPARAM) hwnd,(LPARAM)&sizingRect) )
			sizingRect = wndPtr->rectWindow;   
	}
	else {
		if( HOOK_CallHooksW( WH_CBT, HCBT_MOVESIZE, (WPARAM) hwnd,(LPARAM)&sizingRect) )
			sizingRect = wndPtr->rectWindow;   
	}
    }
    SendMessageA( hwnd, WM_EXITSIZEMOVE, 0, (LPARAM)0 );
    SendMessageA( hwnd, WM_SETVISIBLE, !IsIconic(hwnd), (LPARAM)0);

    if( moved && !((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) )
    {
	/* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
	SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
			sizingRect.right - sizingRect.left,
			sizingRect.bottom - sizingRect.top,
		      ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
    }

    if( IsWindow(hwnd) )
	if( wndPtr->dwStyle & WS_MINIMIZE )
	{
	    /* Single click brings up the system menu when iconized */

	    if( !moved ) 
	    {
		 if( wndPtr->dwStyle & WS_SYSMENU ) 
		     SendMessageA( hwnd, WM_SYSCOMMAND,
				    SC_MOUSEMENU + HTSYSMENU, *((LPARAM*)&pt));
	    }
	    else WINPOS_ShowIconTitle( wndPtr, TRUE );
	}
}


/***********************************************************************
 *           NC_TrackMinMaxBox
 *
 * Track a mouse button press on the minimize or maximize box.
 */
static void NC_TrackMinMaxBox( HWND hwnd, WORD wParam )
{
    MSG msg;
    HDC hdc = GetWindowDC( hwnd );
    WINBOOL pressed = TRUE;
    void  (*paintButton)(HWND, HDC, WINBOOL);

    SetCapture( hwnd );
    if (wParam == HTMINBUTTON)
	paintButton =
	    (TWEAK_WineLook == WIN31_LOOK) ? &NC_DrawMinButton : &NC_DrawMinButton95;
    else
	paintButton =
	    (TWEAK_WineLook == WIN31_LOOK) ? &NC_DrawMaxButton : &NC_DrawMaxButton95;

    (*paintButton)( hwnd, hdc, TRUE );

    do
    {
	WINBOOL oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, 0, PM_REMOVE, FALSE );

	pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
	if (pressed != oldstate)
	   (*paintButton)( hwnd, hdc, pressed );
    } while (msg.message != WM_LBUTTONUP);

    (*paintButton)( hwnd, hdc, FALSE );

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );
    if (!pressed) return;

    if (wParam == HTMINBUTTON) 
	SendMessageA( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, *(LONG*)&msg.pt );
    else
	SendMessageA( hwnd, WM_SYSCOMMAND, 
		  IsZoomed(hwnd) ? SC_RESTORE:SC_MAXIMIZE, *(LONG*)&msg.pt );
}


/***********************************************************************
 * NC_TrackCloseButton95
 *
 * Track a mouse button press on the Win95 close button.
 */
static void
NC_TrackCloseButton95 (HWND hwnd, WORD wParam)
{
    MSG msg;
    HDC hdc = GetWindowDC( hwnd );
    WINBOOL pressed = TRUE;

    SetCapture( hwnd );

    NC_DrawCloseButton95 (hwnd, hdc, TRUE);

    do
    {
	WINBOOL oldstate = pressed;
        MSG_InternalGetMessage( &msg, 0, 0, 0, PM_REMOVE, FALSE );

	pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
	if (pressed != oldstate)
	   NC_DrawCloseButton95 (hwnd, hdc, pressed);
    } while (msg.message != WM_LBUTTONUP);

    NC_DrawCloseButton95 (hwnd, hdc, FALSE);

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );
    if (!pressed) return;

    SendMessageA( hwnd, WM_SYSCOMMAND, SC_CLOSE, *(LONG*)&msg.pt );
}


/***********************************************************************
 *           NC_TrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void NC_TrackScrollBar( HWND hwnd, WPARAM wParam, POINT pt )
{
    MSG *msg;
    INT scrollbar;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if ((wParam & 0xfff0) == SC_HSCROLL)
    {
	if ((wParam & 0x0f) != HTHSCROLL) return;
	scrollbar = SB_HORZ;
    }
    else  /* SC_VSCROLL */
    {
	if ((wParam & 0x0f) != HTVSCROLL) return;
	scrollbar = SB_VERT;
    }

    if (!(msg = malloc(sizeof(MSG)))) return;
    pt.x -= wndPtr->rectWindow.left;
    pt.y -= wndPtr->rectWindow.top;
    SetCapture( hwnd );
    SCROLL_HandleScrollEvent( hwnd, scrollbar, WM_LBUTTONDOWN, pt );

    do
    {
        GetMessageA( (msg), 0, 0, 0 );
	switch(msg->message)
	{
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
        case WM_SYSTIMER:
            pt.x = LOWORD(msg->lParam) + wndPtr->rectClient.left - 
	      wndPtr->rectWindow.left;
            pt.y = HIWORD(msg->lParam) + wndPtr->rectClient.top - 
	      wndPtr->rectWindow.top;
            SCROLL_HandleScrollEvent( hwnd, scrollbar, msg->message, pt );
	    break;
        default:
            TranslateMessage( msg );
            DispatchMessage( msg );
            break;
	}
        if (!IsWindow( hwnd ))
        {
            ReleaseCapture();
            break;
        }
    } while (msg->message != WM_LBUTTONUP);
    free(msg);
}

/***********************************************************************
 *           NC_HandleNCLButtonDown
 *
 * Handle a WM_NCLBUTTONDOWN message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDown( WND* pWnd, WPARAM wParam, LPARAM lParam )
{
    HWND hwnd = pWnd->hwndSelf;

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
	 hwnd = WIN_GetTopParent(hwnd);

	 if( WINPOS_SetActiveWindow(hwnd, TRUE, TRUE) || (GetActiveWindow() == hwnd) )
		SendMessageA( pWnd->hwndSelf, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam );
	 break;

    case HTSYSMENU:
	 if( pWnd->dwStyle & WS_SYSMENU )
	 {
	     if( !(pWnd->dwStyle & WS_MINIMIZE) )
	     {
		HDC hDC = GetWindowDC(hwnd);
		if (TWEAK_WineLook == WIN31_LOOK)
		    NC_DrawSysButton( hwnd, hDC, TRUE );
		else
		    NC_DrawSysButton95( hwnd, hDC, TRUE );
		ReleaseDC( hwnd, hDC );
	     }
	     SendMessageA( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, lParam );
	 }
	 break;

    case HTMENU:
	SendMessageA( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU, lParam );
	break;

    case HTHSCROLL:
	SendMessageA( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam );
	break;

    case HTVSCROLL:
	SendMessageA( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam );
	break;

    case HTMINBUTTON:
    case HTMAXBUTTON:
	NC_TrackMinMaxBox( hwnd, wParam );
	break;

    case HTCLOSE:
	if (TWEAK_WineLook >= WIN95_LOOK)
	    NC_TrackCloseButton95 (hwnd, wParam);
	break;

    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
	/* make sure hittest fits into 0xf and doesn't overlap with HTSYSMENU */
	SendMessageA( hwnd, WM_SYSCOMMAND, SC_SIZE + wParam - 2, lParam);
	break;

    case HTBORDER:
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleNCLButtonDblClk
 *
 * Handle a WM_NCLBUTTONDBLCLK message. Called from DefWindowProc().
 */
LONG NC_HandleNCLButtonDblClk( WND *pWnd, WPARAM wParam, LPARAM lParam )
{
    /*
     * if this is an icon, send a restore since we are handling
     * a double click
     */
    if (pWnd->dwStyle & WS_MINIMIZE)
    {
        SendMessageA( pWnd->hwndSelf, WM_SYSCOMMAND, SC_RESTORE, lParam );
        return 0;
    } 

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
        /* stop processing if WS_MAXIMIZEBOX is missing */
        if (pWnd->dwStyle & WS_MAXIMIZEBOX)
            SendMessageA( pWnd->hwndSelf, WM_SYSCOMMAND,
                      (pWnd->dwStyle & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE,
                      lParam );
	break;

    case HTSYSMENU:
        if (!(pWnd->class->style & CS_NOCLOSE))
            SendMessageA( pWnd->hwndSelf, WM_SYSCOMMAND, SC_CLOSE, lParam );
	break;

    case HTHSCROLL:
	SendMessageA( pWnd->hwndSelf, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL,
		       lParam );
	break;

    case HTVSCROLL:
	SendMessageA( pWnd->hwndSelf, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL,
		       lParam );
	break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LONG NC_HandleSysCommand( HWND hwnd, WPARAM wParam, POINT pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    UINT uCommand = wParam & 0xFFF0;

    DPRINT( "Handling WM_SYSCOMMAND %x %d,%d\n", 
		      wParam, pt.x, pt.y );

    if (wndPtr->dwStyle & WS_CHILD && uCommand != SC_KEYMENU )
        ScreenToClient( wndPtr->parent->hwndSelf, &pt );

    switch (uCommand)
    {
    case SC_SIZE:
    case SC_MOVE:
	NC_DoSizeMove( hwnd, wParam );
	break;

    case SC_MINIMIZE:
	ShowWindow( hwnd, SW_MINIMIZE ); 
	break;

    case SC_MAXIMIZE:
	ShowWindow( hwnd, SW_MAXIMIZE );
	break;

    case SC_RESTORE:
	ShowWindow( hwnd, SW_RESTORE );
	break;

    case SC_CLOSE:
	return SendMessageA( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
	NC_TrackScrollBar( hwnd, wParam, pt );
	break;

    case SC_MOUSEMENU:
        MENU_TrackMouseMenuBar( wndPtr, wParam & 0x000F, pt );
	break;

    case SC_KEYMENU:
	MENU_TrackKbdMenuBar( wndPtr , wParam , pt.x );
	break;
	
    case SC_TASKLIST:
	WinExec( "taskman.exe", SW_SHOWNORMAL ); 
	break;

    case SC_SCREENSAVE:
	if (wParam == SC_ABOUTWINE) {
            //ShellAboutA(hwnd,"Wine", WINE_RELEASE_INFO, 0);
	}
	else 
	  if (wParam == SC_PUTMARK)
            	DPRINT("Mark requested by user\n");
	break;
  
    case SC_HOTKEY:
    case SC_ARRANGE:
    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
 	DPRINT( "unimplemented!\n");
        break;
    }
    return 0;
}


/***********************************************************************
 *           FastWindowFrame    (GDI.400)
 */
WINBOOL STDCALL FastWindowFrame( HDC hdc, const RECT *rect,
                               INT width, INT height, DWORD rop )
{
    HBRUSH hbrush = SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left - width, height, rop );
    PatBlt( hdc, rect->left, rect->top + height, width,
              rect->bottom - rect->top - height, rop );
    PatBlt( hdc, rect->left + width, rect->bottom,
              rect->right - rect->left - width, -height, rop );
    PatBlt( hdc, rect->right, rect->top, -width, 
              rect->bottom - rect->top - height, rop );
    SelectObject( hdc, hbrush );
    return TRUE;
}