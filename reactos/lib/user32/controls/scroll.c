/*		
 * Scrollbar control
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994, 1996 Alexandre Julliard
 */

#include <ntos/minmax.h>
#define MAX max
#define MIN min

#include <windows.h>
#include <user32/sysmetr.h>
#include <user32/scroll.h>
#include <user32/heapdup.h>
#include <user32/win.h>
#include <user32/syscolor.h>
#include <user32/debug.h>

static HBITMAP hUpArrow = 0;
static HBITMAP hDnArrow = 0;
static HBITMAP hLfArrow = 0;
static HBITMAP hRgArrow = 0;
static HBITMAP hUpArrowD = 0;
static HBITMAP hDnArrowD = 0;
static HBITMAP hLfArrowD = 0;
static HBITMAP hRgArrowD = 0;
static HBITMAP hUpArrowI = 0;
static HBITMAP hDnArrowI = 0;
static HBITMAP hLfArrowI = 0;
static HBITMAP hRgArrowI = 0;

#define TOP_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_UP) ? hUpArrowI : ((pressed) ? hUpArrowD:hUpArrow))
#define BOTTOM_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_DOWN) ? hDnArrowI : ((pressed) ? hDnArrowD:hDnArrow))
#define LEFT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_LEFT) ? hLfArrowI : ((pressed) ? hLfArrowD:hLfArrow))
#define RIGHT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_RIGHT) ? hRgArrowI : ((pressed) ? hRgArrowD:hRgArrow))


  /* Minimum size of the rectangle between the arrows */
#define SCROLL_MIN_RECT  4  

  /* Minimum size of the thumb in pixels */
#define SCROLL_MIN_THUMB 6

  /* Overlap between arrows and thumb */
#define SCROLL_ARROW_THUMB_OVERLAP 1

  /* Delay (in ms) before first repetition when holding the button down */
#define SCROLL_FIRST_DELAY   200

  /* Delay (in ms) between scroll repetitions */
#define SCROLL_REPEAT_DELAY  50

  /* Scroll timer id */
#define SCROLL_TIMER   0

  /* Scroll-bar hit testing */
enum SCROLL_HITTEST
{
    SCROLL_NOWHERE,      /* Outside the scroll bar */
    SCROLL_TOP_ARROW,    /* Top or left arrow */
    SCROLL_TOP_RECT,     /* Rectangle between the top arrow and the thumb */
    SCROLL_THUMB,        /* Thumb rectangle */
    SCROLL_BOTTOM_RECT,  /* Rectangle between the thumb and the bottom arrow */
    SCROLL_BOTTOM_ARROW  /* Bottom or right arrow */
};

 /* What to do after SCROLL_SetScrollInfo() */
#define SA_SSI_HIDE		0x0001
#define SA_SSI_SHOW		0x0002
#define SA_SSI_REFRESH		0x0004
#define SA_SSI_REPAINT_ARROWS	0x0008

 /* Thumb-tracking info */
static HWND SCROLL_TrackingWin = 0;
static INT  SCROLL_TrackingBar = 0;
static INT  SCROLL_TrackingPos = 0;
static INT  SCROLL_TrackingVal = 0;
 /* Hit test code of the last button-down event */
static enum SCROLL_HITTEST SCROLL_trackHitTest;
static BOOL SCROLL_trackVertical;

 /* Is the moving thumb being displayed? */
static BOOL SCROLL_MovingThumb = FALSE;

 /* Local functions */
static BOOL SCROLL_ShowScrollBar( HWND hwnd, INT nBar, 
				    BOOL fShowH, BOOL fShowV );
static INT SCROLL_SetScrollInfo( HWND hwnd, INT nBar, 
				   const SCROLLINFO *info, INT *action );

/***********************************************************************
 *           SCROLL_LoadBitmaps
 */
static void SCROLL_LoadBitmaps(void)
{
    hUpArrow  = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_UPARROW) );
    hDnArrow  = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_DNARROW) );
    hLfArrow  = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_LFARROW) );
    hRgArrow  = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_RGARROW) );
    hUpArrowD = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_UPARROWD) );
    hDnArrowD = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_DNARROWD) );
    hLfArrowD = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_LFARROWD) );
    hRgArrowD = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_RGARROWD) );
    hUpArrowI = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_UPARROWI) );
    hDnArrowI = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_DNARROWI) );
    hLfArrowI = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_LFARROWI) );
    hRgArrowI = LoadBitmapA( 0, MAKEINTRESOURCE(OBM_RGARROWI) );
}


/***********************************************************************
 *           SCROLL_GetPtrScrollInfo
 */
static SCROLLBAR_INFO *SCROLL_GetPtrScrollInfo( WND* wndPtr, INT nBar )
{
    SCROLLBAR_INFO *infoPtr;

    if (!wndPtr) return NULL;
    switch(nBar)
    {
        case SB_HORZ: infoPtr = (SCROLLBAR_INFO *)wndPtr->pHScroll; break;
        case SB_VERT: infoPtr = (SCROLLBAR_INFO *)wndPtr->pVScroll; break;
        case SB_CTL:  infoPtr = (SCROLLBAR_INFO *)wndPtr->wExtra; break;
        default:      return NULL;
    }

    if (!infoPtr)  /* Create the info structure if needed */
    {
        if ((infoPtr = HeapAlloc( GetProcessHeap(), 0, sizeof(SCROLLBAR_INFO) )))
        {
            infoPtr->MinVal = infoPtr->CurVal = infoPtr->Page = 0;
            infoPtr->MaxVal = 100;
            infoPtr->flags  = ESB_ENABLE_BOTH;
            if (nBar == SB_HORZ) wndPtr->pHScroll = infoPtr;
            else wndPtr->pVScroll = infoPtr;
        }
        if (!hUpArrow) SCROLL_LoadBitmaps();
    }
    return infoPtr;
}


/***********************************************************************
 *           SCROLL_GetScrollInfo
 */
static SCROLLBAR_INFO *SCROLL_GetScrollInfo( HWND hwnd, INT nBar )
{
   WND *wndPtr = WIN_FindWndPtr( hwnd );
   return SCROLL_GetPtrScrollInfo( wndPtr, nBar );
}


/***********************************************************************
 *           SCROLL_GetScrollBarRect
 *
 * Compute the scroll bar rectangle, in drawing coordinates (i.e. client
 * coords for SB_CTL, window coords for SB_VERT and SB_HORZ).
 * 'arrowSize' returns the width or height of an arrow (depending on
 * the orientation of the scrollbar), 'thumbSize' returns the size of
 * the thumb, and 'thumbPos' returns the position of the thumb
 * relative to the left or to the top.
 * Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
static BOOL SCROLL_GetScrollBarRect( HWND hwnd, INT nBar, RECT *lprect,
                                       INT *arrowSize, INT *thumbSize,
                                       INT *thumbPos )
{
    INT pixels;
    BOOL vertical;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    switch(nBar)
    {
      case SB_HORZ:
        lprect->left   = wndPtr->rectClient.left - wndPtr->rectWindow.left;
        lprect->top    = wndPtr->rectClient.bottom - wndPtr->rectWindow.top;
        lprect->right  = wndPtr->rectClient.right - wndPtr->rectWindow.left;
        lprect->bottom = lprect->top + SYSMETRICS_CYHSCROLL;
	if(wndPtr->dwStyle & WS_BORDER) {
	  lprect->left--;
	  lprect->right++;
	} else if(wndPtr->dwStyle & WS_VSCROLL)
	  lprect->right++;
        vertical = FALSE;
	break;

      case SB_VERT:
        lprect->left   = wndPtr->rectClient.right - wndPtr->rectWindow.left;
        lprect->top    = wndPtr->rectClient.top - wndPtr->rectWindow.top;
        lprect->right  = lprect->left + SYSMETRICS_CXVSCROLL;
        lprect->bottom = wndPtr->rectClient.bottom - wndPtr->rectWindow.top;
	if(wndPtr->dwStyle & WS_BORDER) {
	  lprect->top--;
	  lprect->bottom++;
	} else if(wndPtr->dwStyle & WS_HSCROLL)
	  lprect->bottom++;
        vertical = TRUE;
	break;

      case SB_CTL:
	GetClientRect( hwnd, lprect );
        vertical = ((wndPtr->dwStyle & SBS_VERT) != 0);
	break;

    default:
        return FALSE;
    }

    if (vertical) pixels = lprect->bottom - lprect->top;
    else pixels = lprect->right - lprect->left;

    if (pixels <= 2*SYSMETRICS_CXVSCROLL + SCROLL_MIN_RECT)
    {
        if (pixels > SCROLL_MIN_RECT)
            *arrowSize = (pixels - SCROLL_MIN_RECT) / 2;
        else
            *arrowSize = 0;
        *thumbPos = *thumbSize = 0;
    }
    else
    {
        SCROLLBAR_INFO *info = SCROLL_GetPtrScrollInfo( wndPtr, nBar );

        *arrowSize = SYSMETRICS_CXVSCROLL;
        pixels -= (2 * (SYSMETRICS_CXVSCROLL - SCROLL_ARROW_THUMB_OVERLAP));

        if (info->Page)
        {
            *thumbSize = pixels * info->Page / (info->MaxVal-info->MinVal+1);
            if (*thumbSize < SCROLL_MIN_THUMB) *thumbSize = SCROLL_MIN_THUMB;
        }
        else *thumbSize = SYSMETRICS_CXVSCROLL;

        if (((pixels -= *thumbSize ) < 0) ||
            ((info->flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH))
        {
            /* Rectangle too small or scrollbar disabled -> no thumb */
            *thumbPos = *thumbSize = 0;
        }
        else
        {
            INT max = info->MaxVal - MAX( info->Page-1, 0 );
            if (info->MinVal >= max)
                *thumbPos = *arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
            else
                *thumbPos = *arrowSize - SCROLL_ARROW_THUMB_OVERLAP
		 + pixels * (info->CurVal-info->MinVal) / (max - info->MinVal);
        }
    }
    return vertical;
}


/***********************************************************************
 *           SCROLL_GetThumbVal
 *
 * Compute the current scroll position based on the thumb position in pixels
 * from the top of the scroll-bar.
 */
static UINT SCROLL_GetThumbVal( SCROLLBAR_INFO *infoPtr, RECT *rect,
                                  BOOL vertical, INT pos )
{
    INT thumbSize;
    INT pixels = vertical ? rect->bottom-rect->top : rect->right-rect->left;

    if ((pixels -= 2*(SYSMETRICS_CXVSCROLL - SCROLL_ARROW_THUMB_OVERLAP)) <= 0)
        return infoPtr->MinVal;

    if (infoPtr->Page)
    {
        thumbSize = pixels * infoPtr->Page/(infoPtr->MaxVal-infoPtr->MinVal+1);
        if (thumbSize < SCROLL_MIN_THUMB) thumbSize = SCROLL_MIN_THUMB;
    }
    else thumbSize = SYSMETRICS_CXVSCROLL;

    if ((pixels -= thumbSize) <= 0) return infoPtr->MinVal;

    pos = MAX( 0, pos - (SYSMETRICS_CXVSCROLL - SCROLL_ARROW_THUMB_OVERLAP) );
    if (pos > pixels) pos = pixels;

    if (!infoPtr->Page) pos *= infoPtr->MaxVal - infoPtr->MinVal;
    else pos *= infoPtr->MaxVal - infoPtr->MinVal - infoPtr->Page + 1;
    return infoPtr->MinVal + ((pos + pixels / 2) / pixels);
}

/***********************************************************************
 *           SCROLL_PtInRectEx
 */
static BOOL SCROLL_PtInRectEx( LPRECT lpRect, POINT pt, BOOL vertical )
{
    RECT rect = *lpRect;

    if (vertical)
    {
	rect.left -= lpRect->right - lpRect->left;
	rect.right += lpRect->right - lpRect->left;
    }
    else
    {
	rect.top -= lpRect->bottom - lpRect->top;
	rect.bottom += lpRect->bottom - lpRect->top;
    }
    return PtInRect( &rect, pt );
}

/***********************************************************************
 *           SCROLL_ClipPos
 */
static POINT SCROLL_ClipPos( LPRECT lpRect, POINT pt )
{
    if( pt.x < lpRect->left )
	pt.x = lpRect->left;
    else
    if( pt.x > lpRect->right )
	pt.x = lpRect->right;

    if( pt.y < lpRect->top )
	pt.y = lpRect->top;
    else
    if( pt.y > lpRect->bottom )
	pt.y = lpRect->bottom;

    return pt;
}


/***********************************************************************
 *           SCROLL_HitTest
 *
 * Scroll-bar hit testing (don't confuse this with WM_NCHITTEST!).
 */
static enum SCROLL_HITTEST SCROLL_HitTest( HWND hwnd, INT nBar,
                                           POINT pt, BOOL bDragging )
{
    INT arrowSize, thumbSize, thumbPos;
    RECT rect;

    BOOL vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                           &arrowSize, &thumbSize, &thumbPos );

    if ( (bDragging && !SCROLL_PtInRectEx( &rect, pt, vertical )) ||
	 (!PtInRect( &rect, pt )) ) return SCROLL_NOWHERE;

    if (vertical)
    {
        if (pt.y < rect.top + arrowSize) return SCROLL_TOP_ARROW;
        if (pt.y >= rect.bottom - arrowSize) return SCROLL_BOTTOM_ARROW;
        if (!thumbPos) return SCROLL_TOP_RECT;
        pt.y -= rect.top;
        if (pt.y < thumbPos) return SCROLL_TOP_RECT;
        if (pt.y >= thumbPos + thumbSize) return SCROLL_BOTTOM_RECT;
    }
    else  /* horizontal */
    {
        if (pt.x < rect.left + arrowSize) return SCROLL_TOP_ARROW;
        if (pt.x >= rect.right - arrowSize) return SCROLL_BOTTOM_ARROW;
        if (!thumbPos) return SCROLL_TOP_RECT;
        pt.x -= rect.left;
        if (pt.x < thumbPos) return SCROLL_TOP_RECT;
        if (pt.x >= thumbPos + thumbSize) return SCROLL_BOTTOM_RECT;
    }
    return SCROLL_THUMB;
}


/***********************************************************************
 *           SCROLL_DrawArrows
 *
 * Draw the scroll bar arrows.
 */
static void SCROLL_DrawArrows( HDC hdc, SCROLLBAR_INFO *infoPtr,
                               RECT *rect, INT arrowSize, BOOL vertical,
                               BOOL top_pressed, BOOL bottom_pressed )
{
    HDC hdcMem = CreateCompatibleDC( hdc );
    HBITMAP hbmpPrev = SelectObject( hdcMem, vertical ?
                                    TOP_ARROW(infoPtr->flags, top_pressed)
                                    : LEFT_ARROW(infoPtr->flags, top_pressed));

    SetStretchBltMode( hdc, STRETCH_DELETESCANS );
    StretchBlt( hdc, rect->left, rect->top,
                  vertical ? rect->right-rect->left : arrowSize,
                  vertical ? arrowSize : rect->bottom-rect->top,
                  hdcMem, 0, 0,
                  SYSMETRICS_CXVSCROLL, SYSMETRICS_CYHSCROLL,
                  SRCCOPY );

    SelectObject( hdcMem, vertical ?
                    BOTTOM_ARROW( infoPtr->flags, bottom_pressed )
                    : RIGHT_ARROW( infoPtr->flags, bottom_pressed ) );
    if (vertical)
        StretchBlt( hdc, rect->left, rect->bottom - arrowSize,
                      rect->right - rect->left, arrowSize,
                      hdcMem, 0, 0,
                      SYSMETRICS_CXVSCROLL, SYSMETRICS_CYHSCROLL,
                      SRCCOPY );
    else
        StretchBlt( hdc, rect->right - arrowSize, rect->top,
                      arrowSize, rect->bottom - rect->top,
                      hdcMem, 0, 0,
                      SYSMETRICS_CXVSCROLL, SYSMETRICS_CYHSCROLL,
                      SRCCOPY );
    SelectObject( hdcMem, hbmpPrev );
    DeleteDC( hdcMem );
}


/***********************************************************************
 *           SCROLL_DrawMovingThumb
 *
 * Draw the moving thumb rectangle.
 */
static void SCROLL_DrawMovingThumb( HDC hdc, RECT *rect, BOOL vertical,
                                    INT arrowSize, INT thumbSize )
{
    RECT r = *rect;
    if (vertical)
    {
        r.top += SCROLL_TrackingPos;
        if (r.top < rect->top + arrowSize - SCROLL_ARROW_THUMB_OVERLAP)
	    r.top = rect->top + arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
        if (r.top + thumbSize >
	               rect->bottom - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP))
            r.top = rect->bottom - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP)
	                                                          - thumbSize;
        r.bottom = r.top + thumbSize;
    }
    else
    {
        r.left += SCROLL_TrackingPos;
        if (r.left < rect->left + arrowSize - SCROLL_ARROW_THUMB_OVERLAP)
	    r.left = rect->left + arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
        if (r.left + thumbSize >
	               rect->right - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP))
            r.left = rect->right - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP) 
	                                                          - thumbSize;
        r.right = r.left + thumbSize;
    }
    DrawFocusRect( hdc, &r );
    SCROLL_MovingThumb = !SCROLL_MovingThumb;
}


/***********************************************************************
 *           SCROLL_DrawInterior
 *
 * Draw the scroll bar interior (everything except the arrows).
 */
static void SCROLL_DrawInterior( HWND hwnd, HDC hdc, INT nBar, 
                                 RECT *rect, INT arrowSize,
                                 INT thumbSize, INT thumbPos,
                                 UINT flags, BOOL vertical,
                                 BOOL top_selected, BOOL bottom_selected )
{
    RECT r;

      /* Select the correct brush and pen */

    SelectObject( hdc, GetSysColorPen(COLOR_WINDOWFRAME) );
    if ((flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH)
    {
          /* This ought to be the color of the parent window */
        SelectObject( hdc, GetSysColorBrush(COLOR_WINDOW) );
    }
    else
    {
        if (nBar == SB_CTL)  /* Only scrollbar controls send WM_CTLCOLOR */
        {
            HBRUSH hbrush = SendMessageA(GetParent(hwnd),
                                             WM_CTLCOLORSCROLLBAR, hdc, hwnd );
            SelectObject( hdc, hbrush );
        }
        else SelectObject( hdc, GetSysColorBrush(COLOR_SCROLLBAR) );
    }

      /* Calculate the scroll rectangle */

    r = *rect;
    if (vertical)
    {
        r.top    += arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
        r.bottom -= (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
    }
    else
    {
        r.left  += arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
        r.right -= (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
    }

      /* Draw the scroll bar frame */

    Rectangle( hdc, r.left, r.top, r.right, r.bottom );

      /* Draw the scroll rectangles and thumb */

    if (!thumbPos)  /* No thumb to draw */
    {
        PatBlt( hdc, r.left+1, r.top+1, r.right - r.left - 2,
                  r.bottom - r.top - 2, PATCOPY );
        return;
    }

    if (vertical)
    {
        PatBlt( hdc, r.left + 1, r.top + 1,
                  r.right - r.left - 2,
                  thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP) - 1,
                  top_selected ? 0x0f0000 : PATCOPY );
        r.top += thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
        PatBlt( hdc, r.left + 1, r.top + thumbSize,
                  r.right - r.left - 2,
                  r.bottom - r.top - thumbSize - 1,
                  bottom_selected ? 0x0f0000 : PATCOPY );
        r.bottom = r.top + thumbSize;
    }
    else  /* horizontal */
    {
        PatBlt( hdc, r.left + 1, r.top + 1,
                  thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP) - 1,
                  r.bottom - r.top - 2,
                  top_selected ? 0x0f0000 : PATCOPY );
        r.left += thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
        PatBlt( hdc, r.left + thumbSize, r.top + 1,
                  r.right - r.left - thumbSize - 1,
                  r.bottom - r.top - 2,
                  bottom_selected ? 0x0f0000 : PATCOPY );
        r.right = r.left + thumbSize;
    }

      /* Draw the thumb */

    SelectObject( hdc, GetSysColorBrush(COLOR_BTNFACE) );
    Rectangle( hdc, r.left, r.top, r.right, r.bottom );
    r.top++, r.left++;
    DrawEdge( hdc, &r, EDGE_RAISED, BF_RECT );
    if (SCROLL_MovingThumb &&
        (SCROLL_TrackingWin == hwnd) &&
        (SCROLL_TrackingBar == nBar))
    {
        SCROLL_DrawMovingThumb( hdc, rect, vertical, arrowSize, thumbSize );
        SCROLL_MovingThumb = TRUE;
    }
}


/***********************************************************************
 *           SCROLL_DrawScrollBar
 *
 * Redraw the whole scrollbar.
 */
void SCROLL_DrawScrollBar( HWND hwnd, HDC hdc, INT nBar, 
			   BOOL arrows, BOOL interior )
{
    INT arrowSize, thumbSize, thumbPos;
    RECT rect;
    BOOL vertical;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SCROLLBAR_INFO *infoPtr = SCROLL_GetPtrScrollInfo( wndPtr, nBar );

    if (!wndPtr || !infoPtr ||
        ((nBar == SB_VERT) && !(wndPtr->dwStyle & WS_VSCROLL)) ||
        ((nBar == SB_HORZ) && !(wndPtr->dwStyle & WS_HSCROLL))) return;
    if (!WIN_IsWindowDrawable( wndPtr, FALSE )) return;

    vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                        &arrowSize, &thumbSize, &thumbPos );

      /* Draw the arrows */

    if (arrows && arrowSize)
    {
	if( vertical == SCROLL_trackVertical && GetCapture() == hwnd )
	    SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize, vertical,
			       (SCROLL_trackHitTest == SCROLL_TOP_ARROW),
			       (SCROLL_trackHitTest == SCROLL_BOTTOM_ARROW) );
	else
	    SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize, vertical, 
							       FALSE, FALSE );
    }
    if( interior )
	SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                         thumbPos, infoPtr->flags, vertical, FALSE, FALSE );
}


/***********************************************************************
 *           SCROLL_RefreshScrollBar
 *
 * Repaint the scroll bar interior after a SetScrollRange() or
 * SetScrollPos() call.
 */
static void SCROLL_RefreshScrollBar( HWND hwnd, INT nBar, 
				     BOOL arrows, BOOL interior )
{
    HDC hdc = GetDCEx( hwnd, 0,
                           DCX_CACHE | ((nBar == SB_CTL) ? 0 : DCX_WINDOW) );
    if (!hdc) return;

    SCROLL_DrawScrollBar( hwnd, hdc, nBar, arrows, interior );
    ReleaseDC( hwnd, hdc );
}


/***********************************************************************
 *           SCROLL_HandleKbdEvent
 *
 * Handle a keyboard event (only for SB_CTL scrollbars).
 */
static void SCROLL_HandleKbdEvent( HWND hwnd, WPARAM wParam )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WPARAM msg;
    
    switch(wParam)
    {
    case VK_PRIOR: msg = SB_PAGEUP; break;
    case VK_NEXT:  msg = SB_PAGEDOWN; break;
    case VK_HOME:  msg = SB_TOP; break;
    case VK_END:   msg = SB_BOTTOM; break;
    case VK_UP:    msg = SB_LINEUP; break;
    case VK_DOWN:  msg = SB_LINEDOWN; break;
    default:
        return;
    }
    SendMessageA( GetParent(hwnd),
                    (wndPtr->dwStyle & SBS_VERT) ? WM_VSCROLL : WM_HSCROLL,
                    msg, hwnd );
}


/***********************************************************************
 *           SCROLL_HandleScrollEvent
 *
 * Handle a mouse or timer event for the scrollbar.
 * 'pt' is the location of the mouse event in client (for SB_CTL) or
 * windows coordinates.
 */
void SCROLL_HandleScrollEvent( HWND hwnd, INT nBar, UINT msg, POINT pt)
{
      /* Previous mouse position for timer events */
    static POINT prevPt;
      /* Thumb position when tracking started. */
    static UINT trackThumbPos;
      /* Position in the scroll-bar of the last button-down event. */
    static INT lastClickPos;
      /* Position in the scroll-bar of the last mouse event. */
    static INT lastMousePos;

    enum SCROLL_HITTEST hittest;
    HWND hwndOwner, hwndCtl;
    BOOL vertical;
    INT arrowSize, thumbSize, thumbPos;
    RECT rect;
    HDC hdc;

    SCROLLBAR_INFO *infoPtr = SCROLL_GetScrollInfo( hwnd, nBar );
    if (!infoPtr) return;
    if ((SCROLL_trackHitTest == SCROLL_NOWHERE) && (msg != WM_LBUTTONDOWN)) 
		  return;

    hdc = GetDCEx( hwnd, 0, DCX_CACHE | ((nBar == SB_CTL) ? 0 : DCX_WINDOW));
    vertical = SCROLL_GetScrollBarRect( hwnd, nBar, &rect,
                                        &arrowSize, &thumbSize, &thumbPos );
    hwndOwner = (nBar == SB_CTL) ? GetParent(hwnd) : hwnd;
    hwndCtl   = (nBar == SB_CTL) ? hwnd : 0;

    switch(msg)
    {
      case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
	  SCROLL_trackVertical = vertical;
          SCROLL_trackHitTest  = hittest = SCROLL_HitTest( hwnd, nBar, pt, FALSE );
          lastClickPos  = vertical ? (pt.y - rect.top) : (pt.x - rect.left);
          lastMousePos  = lastClickPos;
          trackThumbPos = thumbPos;
          prevPt = pt;
          SetCapture( hwnd );
          if (nBar == SB_CTL) SetFocus( hwnd );
          break;

      case WM_MOUSEMOVE:
          hittest = SCROLL_HitTest( hwnd, nBar, pt, TRUE );
          prevPt = pt;
          break;

      case WM_LBUTTONUP:
          hittest = SCROLL_NOWHERE;
          ReleaseCapture();
          break;

      case WM_TIMER:
          pt = prevPt;
          hittest = SCROLL_HitTest( hwnd, nBar, pt, FALSE );
          break;

      default:
          return;  /* Should never happen */
    }

    DPRINT( "Event: hwnd=%04x bar=%d msg=%x pt=%d,%d hit=%d\n",
		 hwnd, nBar, msg, pt.x, pt.y, hittest );

    switch(SCROLL_trackHitTest)
    {
    case SCROLL_NOWHERE:  /* No tracking in progress */
        break;

    case SCROLL_TOP_ARROW:
        SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize, vertical,
                           (hittest == SCROLL_trackHitTest), FALSE );
        if (hittest == SCROLL_trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_TIMER))
            {
                SendMessageA( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_LINEUP, hwndCtl );
                SetTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC)0 );
            }
        }
        else KillTimer( hwnd, SCROLL_TIMER );
        break;

    case SCROLL_TOP_RECT:
        SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                             thumbPos, infoPtr->flags, vertical,
                             (hittest == SCROLL_trackHitTest), FALSE );
        if (hittest == SCROLL_trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_TIMER))
            {
                SendMessageA( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_PAGEUP, hwndCtl );
                SetTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC)0 );
            }
        }
        else KillTimer( hwnd, SCROLL_TIMER );
        break;

    case SCROLL_THUMB:
        if (msg == WM_LBUTTONDOWN)
        {
            SCROLL_TrackingWin = hwnd;
            SCROLL_TrackingBar = nBar;
            SCROLL_TrackingPos = trackThumbPos + lastMousePos - lastClickPos;
            SCROLL_DrawMovingThumb(hdc, &rect, vertical, arrowSize, thumbSize);
        }
        else if (msg == WM_LBUTTONUP)
        {
            SCROLL_TrackingWin = 0;
            SCROLL_MovingThumb = FALSE;
            SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                                 thumbPos, infoPtr->flags, vertical,
                                 FALSE, FALSE );
        }
        else  /* WM_MOUSEMOVE */
        {
            UINT pos;

            if (!SCROLL_PtInRectEx( &rect, pt, vertical )) pos = lastClickPos;
            else
	    {
		pt = SCROLL_ClipPos( &rect, pt );
		pos = vertical ? (pt.y - rect.top) : (pt.x - rect.left);
	    }
            if (pos != lastMousePos)
            {
                SCROLL_DrawMovingThumb( hdc, &rect, vertical,
                                        arrowSize, thumbSize );
                lastMousePos = pos;
                SCROLL_TrackingPos = trackThumbPos + pos - lastClickPos;
                SCROLL_TrackingVal = SCROLL_GetThumbVal( infoPtr, &rect,
                                                         vertical,
                                                         SCROLL_TrackingPos );
                SendMessageA( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                MAKEWPARAM( SB_THUMBTRACK, SCROLL_TrackingVal),
                                hwndCtl );
                SCROLL_DrawMovingThumb( hdc, &rect, vertical,
                                        arrowSize, thumbSize );
            }
        }
        break;
        
    case SCROLL_BOTTOM_RECT:
        SCROLL_DrawInterior( hwnd, hdc, nBar, &rect, arrowSize, thumbSize,
                             thumbPos, infoPtr->flags, vertical,
                             FALSE, (hittest == SCROLL_trackHitTest) );
        if (hittest == SCROLL_trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_TIMER))
            {
                SendMessageA( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_PAGEDOWN, hwndCtl );
                SetTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC)0 );
            }
        }
        else KillTimer( hwnd, SCROLL_TIMER );
        break;
        
    case SCROLL_BOTTOM_ARROW:
        SCROLL_DrawArrows( hdc, infoPtr, &rect, arrowSize, vertical,
                           FALSE, (hittest == SCROLL_trackHitTest) );
        if (hittest == SCROLL_trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_TIMER))
            {
                SendMessageA( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_LINEDOWN, (LPARAM)hwndCtl );
                SetTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                                  SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY,
                                  (TIMERPROC)0 );
            }
        }
        else KillTimer( hwnd, SCROLL_TIMER );
        break;
    }

    if (msg == WM_LBUTTONUP)
    {
	hittest = SCROLL_trackHitTest;
	SCROLL_trackHitTest = SCROLL_NOWHERE;  /* Terminate tracking */

        if (hittest == SCROLL_THUMB)
        {
            UINT val = SCROLL_GetThumbVal( infoPtr, &rect, vertical,
                                 trackThumbPos + lastMousePos - lastClickPos );
            SendMessageA( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                            MAKEWPARAM( SB_THUMBPOSITION, val ), (LPARAM)hwndCtl );
        }
        else
            SendMessageA( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                            SB_ENDSCROLL, (LPARAM)hwndCtl );
    }

    ReleaseDC( hwnd, hdc );
}


/***********************************************************************
 *           ScrollBarWndProc
 */
LRESULT STDCALL ScrollBarWndProc( HWND hwnd, UINT message, WPARAM wParam,
                                 LPARAM lParam )
{
    POINT *pt;
    switch(message)
    {
    case WM_CREATE:
        {
	    CREATESTRUCT *lpCreat = (CREATESTRUCT *)lParam;
            if (lpCreat->style & SBS_SIZEBOX)
            {
                DPRINT( "FIXME Unimplemented style SBS_SIZEBOX.\n" );
                return 0;
            }
            
	    if (lpCreat->style & SBS_VERT)
            {
                if (lpCreat->style & SBS_LEFTALIGN)
                    MoveWindow( hwnd, lpCreat->x, lpCreat->y,
                                  SYSMETRICS_CXVSCROLL+1, lpCreat->cy, FALSE );
                else if (lpCreat->style & SBS_RIGHTALIGN)
                    MoveWindow( hwnd, 
                                  lpCreat->x+lpCreat->cx-SYSMETRICS_CXVSCROLL-1,
                                  lpCreat->y,
                                  SYSMETRICS_CXVSCROLL+1, lpCreat->cy, FALSE );
            }
            else  /* SBS_HORZ */
            {
                if (lpCreat->style & SBS_TOPALIGN)
                    MoveWindow( hwnd, lpCreat->x, lpCreat->y,
                                  lpCreat->cx, SYSMETRICS_CYHSCROLL+1, FALSE );
                else if (lpCreat->style & SBS_BOTTOMALIGN)
                    MoveWindow( hwnd, 
                                  lpCreat->x,
                                  lpCreat->y+lpCreat->cy-SYSMETRICS_CYHSCROLL-1,
                                  lpCreat->cx, SYSMETRICS_CYHSCROLL+1, FALSE );
            }
        }
        if (!hUpArrow) SCROLL_LoadBitmaps();
        DPRINT( "ScrollBar creation, hwnd=%04x\n", hwnd );
        return 0;
	
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_TIMER:
	pt = (POINT *)&lParam;
        SCROLL_HandleScrollEvent( hwnd, SB_CTL, message,*pt );
        break;

    case WM_KEYDOWN:
        SCROLL_HandleKbdEvent( hwnd, wParam );
        break;

    case WM_ERASEBKGND:
         return 1;

    case WM_GETDLGCODE:
         return DLGC_WANTARROWS; /* Windows returns this value */

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hwnd, &ps );
            SCROLL_DrawScrollBar( hwnd, hdc, SB_CTL, TRUE, TRUE );
            EndPaint( hwnd, &ps );
        }
        break;

  
    case SBM_SETPOS:
        return SetScrollPos( hwnd, SB_CTL, wParam, (BOOL)lParam );


    case SBM_GETPOS:
        return GetScrollPos( hwnd, SB_CTL );

    
    case SBM_SETRANGE:
        SetScrollRange( hwnd, SB_CTL, wParam, lParam, FALSE );
        return 0;  /* FIXME: return previous position */

   

    case SBM_GETRANGE:
        GetScrollRange( hwnd, SB_CTL, (LPINT)wParam, (LPINT)lParam );
        return 0;

   
    case SBM_ENABLE_ARROWS:
        return EnableScrollBar( hwnd, SB_CTL, wParam );

    case SBM_SETRANGEREDRAW:
        SetScrollRange( hwnd, SB_CTL, wParam, lParam, TRUE );
        return 0;  /* FIXME: return previous position */
        
    case SBM_SETSCROLLINFO:
        return SetScrollInfo( hwnd, SB_CTL, (SCROLLINFO *)lParam, wParam );

    case SBM_GETSCROLLINFO:
        return GetScrollInfo( hwnd, SB_CTL, (SCROLLINFO *)lParam );

    case 0x00e5:
    case 0x00e7:
    case 0x00e8:
    case 0x00eb:
    case 0x00ec:
    case 0x00ed:
    case 0x00ee:
    case 0x00ef:
        DPRINT( "unknown Win msg %04x wp=%08x lp=%08lx\n",
		    message, wParam, lParam );
        break;

    default:
        if (message >= WM_USER)
            DPRINT( "unknown msg %04x wp=%04x lp=%08lx\n",
			 message, wParam, lParam );
        return DefWindowProcA( hwnd, message, wParam, lParam );
    }
    return 0;
}





/*************************************************************************
 *           SetScrollInfo   (USER.501)
 * SetScrollInfo can be used to set the position, upper bound, 
 * lower bound, and page size of a scrollbar control.
 *
 * RETURNS
 *    Scrollbar position
 *
 * NOTE
 *    For 100 lines of text to be displayed in a window of 25 lines,
 *  one would for instance use info->nMin=0, info->nMax=75
 *  (corresponding to the 76 different positions of the window on
 *  the text), and info->nPage=25.
 */
INT STDCALL SetScrollInfo( HWND hwnd, INT nBar , 
const SCROLLINFO *info , BOOL bRedraw )
{
    INT action;
    INT retVal = SCROLL_SetScrollInfo( hwnd, nBar, info, &action );

    if( action & SA_SSI_HIDE )
	SCROLL_ShowScrollBar( hwnd, nBar, FALSE, FALSE );
    else
    {
	if( action & SA_SSI_SHOW )
	    if( SCROLL_ShowScrollBar( hwnd, nBar, TRUE, TRUE ) )
		return retVal; /* SetWindowPos() already did the painting */

	if( bRedraw && (action & SA_SSI_REFRESH))
	    SCROLL_RefreshScrollBar( hwnd, nBar, TRUE, TRUE );
	else if( action & SA_SSI_REPAINT_ARROWS )
	    SCROLL_RefreshScrollBar( hwnd, nBar, TRUE, FALSE );
    }
    return retVal;
}

INT SCROLL_SetScrollInfo( HWND hwnd, INT nBar, 
			    const SCROLLINFO *info, INT *action  )
{
    /* Update the scrollbar state and set action flags according to 
     * what has to be done graphics wise. */

    SCROLLBAR_INFO *infoPtr;
    UINT new_flags;

//    dbg_decl_str(scroll, 256);

   *action = 0;

    if (!(infoPtr = SCROLL_GetScrollInfo(hwnd, nBar))) return 0;
    if (info->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL)) return 0;
    if ((info->cbSize != sizeof(*info)) &&
        (info->cbSize != sizeof(*info)-sizeof(info->nTrackPos))) return 0;

    /* Set the page size */

    if (info->fMask & SIF_PAGE)
    {
  //      dsprintf(scroll, " page=%d", info->nPage );
	if( infoPtr->Page != info->nPage )
	{
            infoPtr->Page = info->nPage;
	   *action |= SA_SSI_REFRESH;
	}
    }

    /* Set the scroll pos */

    if (info->fMask & SIF_POS)
    {
   //     dsprintf(scroll, " pos=%d", info->nPos );
	if( infoPtr->CurVal != info->nPos )
	{
	    infoPtr->CurVal = info->nPos;
	   *action |= SA_SSI_REFRESH;
	}
    }

    /* Set the scroll range */

    if (info->fMask & SIF_RANGE)
    {
     //   dsprintf(scroll, " min=%d max=%d", info->nMin, info->nMax );

        /* Invalid range -> range is set to (0,0) */
        if ((info->nMin > info->nMax) ||
            ((UINT)(info->nMax - info->nMin) >= 0x80000000))
        {
            infoPtr->MinVal = 0;
            infoPtr->MaxVal = 0;
        }
        else
        {
	    if( infoPtr->MinVal != info->nMin ||
		infoPtr->MaxVal != info->nMax )
	    {
	       *action |= SA_SSI_REFRESH;
                infoPtr->MinVal = info->nMin;
                infoPtr->MaxVal = info->nMax;
	    }
        }
    }

    DPRINT( "hwnd=%04x bar=%d %s\n", 
		    hwnd, nBar, dbg_str(scroll));

    /* Make sure the page size is valid */

    if (infoPtr->Page < 0) infoPtr->Page = 0;
    else if (infoPtr->Page > infoPtr->MaxVal - infoPtr->MinVal + 1 )
        infoPtr->Page = infoPtr->MaxVal - infoPtr->MinVal + 1;

    /* Make sure the pos is inside the range */

    if (infoPtr->CurVal < infoPtr->MinVal)
        infoPtr->CurVal = infoPtr->MinVal;
    else if (infoPtr->CurVal > infoPtr->MaxVal - MAX( infoPtr->Page-1, 0 ))
        infoPtr->CurVal = infoPtr->MaxVal - MAX( infoPtr->Page-1, 0 );

    DPRINT( "    new values: page=%d pos=%d min=%d max=%d\n",
		 infoPtr->Page, infoPtr->CurVal,
		 infoPtr->MinVal, infoPtr->MaxVal );

    /* Check if the scrollbar should be hidden or disabled */

    if (info->fMask & (SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL))
    {
        new_flags = infoPtr->flags;
        if (infoPtr->MinVal >= infoPtr->MaxVal - MAX( infoPtr->Page-1, 0 ))
        {
            /* Hide or disable scroll-bar */
            if (info->fMask & SIF_DISABLENOSCROLL)
	    {
                new_flags = ESB_DISABLE_BOTH;
	       *action |= SA_SSI_REFRESH;
	    }
            else if (nBar != SB_CTL)
	    {
		*action = SA_SSI_HIDE;
		goto done;
            }
        }
        else  /* Show and enable scroll-bar */
        {
	    new_flags = 0;
            if (nBar != SB_CTL)
		*action |= SA_SSI_SHOW;
        }

        if (infoPtr->flags != new_flags) /* check arrow flags */
        {
            infoPtr->flags = new_flags;
           *action |= SA_SSI_REPAINT_ARROWS;
        }
    }

done:
    /* Return current position */

    return infoPtr->CurVal;
}




/*************************************************************************
 *           GetScrollInfo   (USER.284)
 * GetScrollInfo can be used to retrieve the position, upper bound, 
 * lower bound, and page size of a scrollbar control.
 *
 * RETURNS STD
 */
BOOL STDCALL GetScrollInfo( HWND hwnd , 
  INT nBar ,   LPSCROLLINFO info )
{
    SCROLLBAR_INFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return FALSE;
    if (info->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL)) return FALSE;
    if ((info->cbSize != sizeof(*info)) &&
        (info->cbSize != sizeof(*info)-sizeof(info->nTrackPos))) return FALSE;

    if (info->fMask & SIF_PAGE) info->nPage = infoPtr->Page;
    if (info->fMask & SIF_POS) info->nPos = infoPtr->CurVal;
    if ((info->fMask & SIF_TRACKPOS) && (info->cbSize == sizeof(*info)))
        info->nTrackPos = (SCROLL_TrackingWin==hwnd) ? SCROLL_TrackingVal : 0;
    if (info->fMask & SIF_RANGE)
    {
	info->nMin = infoPtr->MinVal;
	info->nMax = infoPtr->MaxVal;
    }
    return (info->fMask & SIF_ALL) != 0;
}




/*************************************************************************
 *           SetScrollPos   (USER.502)
 *
 * RETURNS
 *    Success: Scrollbar position
 *    Failure: 0
 *
 * REMARKS
 *    Note the ambiguity when 0 is returned.  Use GetLastError
 *    to make sure there was an error (and to know which one).
 */
INT STDCALL SetScrollPos( HWND hwnd , INT nBar , INT nPos ,BOOL bRedraw )
{
    SCROLLINFO info;
    SCROLLBAR_INFO *infoPtr;
    INT oldPos;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return 0;
    oldPos      = infoPtr->CurVal;
    info.cbSize = sizeof(info);
    info.nPos   = nPos;
    info.fMask  = SIF_POS;
    SetScrollInfo( hwnd, nBar, &info, bRedraw );
    return oldPos;
}




/*************************************************************************
 *           GetScrollPos   (USER.285)
 *
 * RETURNS
 *    Success: Current position
 *    Failure: 0   
 *
 * REMARKS
 *    Note the ambiguity when 0 is returned.  Use GetLastError
 *    to make sure there was an error (and to know which one).
 */
INT STDCALL GetScrollPos( HWND hwnd,INT nBar )
{
    SCROLLBAR_INFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar ))) return 0;
    return infoPtr->CurVal;
}




/*************************************************************************
 *           SetScrollRange   (USER.503)
 *
 * RETURNS STD
 */
BOOL STDCALL SetScrollRange( HWND hwnd, INT nBar, 
INT MinVal, INT MaxVal, BOOL bRedraw )
{
    SCROLLINFO info;

    info.cbSize = sizeof(info);
    info.nMin   = MinVal;
    info.nMax   = MaxVal;
    info.fMask  = SIF_RANGE;
    SetScrollInfo( hwnd, nBar, &info, bRedraw );
    return TRUE;
}


/*************************************************************************
 *	     SCROLL_SetNCSbState
 *
 * Updates both scrollbars at the same time. Used by MDI CalcChildScroll().
 */
INT SCROLL_SetNCSbState(WND* wndPtr, int vMin, int vMax, int vPos,
				       int hMin, int hMax, int hPos)
{
    INT vA, hA;
    SCROLLINFO vInfo, hInfo;
 
    vInfo.cbSize = hInfo.cbSize = sizeof(SCROLLINFO);
    vInfo.nMin   = vMin;	 hInfo.nMin   = hMin;
    vInfo.nMax   = vMax;	 hInfo.nMax   = hMax;
    vInfo.nPos   = vPos;	 hInfo.nPos   = hPos;
    vInfo.fMask  = hInfo.fMask = SIF_RANGE | SIF_POS;

    SCROLL_SetScrollInfo( wndPtr->hwndSelf, SB_VERT, &vInfo, &vA );
    SCROLL_SetScrollInfo( wndPtr->hwndSelf, SB_HORZ, &hInfo, &hA );

    if( !SCROLL_ShowScrollBar( wndPtr->hwndSelf, SB_BOTH,
			      (hA & SA_SSI_SHOW),(vA & SA_SSI_SHOW) ) )
    {
	/* SetWindowPos() wasn't called, just redraw the scrollbars if needed */
	if( vA & SA_SSI_REFRESH )
	    SCROLL_RefreshScrollBar( wndPtr->hwndSelf, SB_VERT, FALSE, TRUE );

	if( hA & SA_SSI_REFRESH )
	    SCROLL_RefreshScrollBar( wndPtr->hwndSelf, SB_HORZ, FALSE, TRUE );
    }
    return 0;
}




/*************************************************************************
 *           GetScrollRange   (USER.286)
 *
 * RETURNS STD
 */
BOOL STDCALL GetScrollRange( HWND hwnd, INT nBar, 
	LPINT lpMin,LPINT lpMax )
{
    SCROLLBAR_INFO *infoPtr;

    if (!(infoPtr = SCROLL_GetScrollInfo( hwnd, nBar )))
    {
        if (lpMin) lpMin = 0;
        if (lpMax) lpMax = 0;
        return FALSE;
    }
    if (lpMin) *lpMin = infoPtr->MinVal;
    if (lpMax) *lpMax = infoPtr->MaxVal;
    return TRUE;
}


/*************************************************************************
 *           SCROLL_ShowScrollBar()
 *
 * Back-end for ShowScrollBar(). Returns FALSE if no action was taken.
 * NOTE: fShowV/fShowH must be zero when nBar is SB_HORZ/SB_VERT.
 */
BOOL SCROLL_ShowScrollBar( HWND hwnd, INT nBar, 
			     BOOL fShowH, BOOL fShowV )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return FALSE;
    DPRINT( "hwnd=%04x bar=%d horz=%d, vert=%d\n",
                    hwnd, nBar, fShowH, fShowV );

    switch(nBar)
    {
    case SB_CTL:
        ShowWindow( hwnd, fShowH ? SW_SHOW : SW_HIDE );
        return TRUE;

    case SB_BOTH:
    case SB_HORZ:
        if (fShowH)
        {
            fShowH = !(wndPtr->dwStyle & WS_HSCROLL);
            wndPtr->dwStyle |= WS_HSCROLL;
        }
        else  /* hide it */
        {
            fShowH = (wndPtr->dwStyle & WS_HSCROLL);
            wndPtr->dwStyle &= ~WS_HSCROLL;
        }
        if( nBar == SB_HORZ ) break; 
	/* fall through */

    case SB_VERT:
        if (fShowV)
        {
            fShowV = !(wndPtr->dwStyle & WS_VSCROLL);
            wndPtr->dwStyle |= WS_VSCROLL;
        }
	else  /* hide it */
        {
            fShowV = (wndPtr->dwStyle & WS_VSCROLL);
            wndPtr->dwStyle &= ~WS_VSCROLL;
        }
        break;

    default:
        return FALSE;  /* Nothing to do! */
    }

    if( fShowH || fShowV ) /* frame has been changed, let the window redraw itself */
    {
	SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE
                    | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
        return TRUE;
    }

    return FALSE; /* no frame changes */
}



/*************************************************************************
 *             SCROLL_FixCaret
 */
WINBOOL SCROLL_FixCaret(HWND hWnd, LPRECT lprc, UINT flags)
{
   HWND hCaret = CARET_GetHwnd();

   if( hCaret )
   {
       RECT	rc;
       CARET_GetRect( &rc );
       if( hCaret == hWnd ||
          (flags & SW_SCROLLCHILDREN && IsChild(hWnd, hCaret)) )
       {
           POINT     pt;

           pt.x = rc.left; pt.y = rc.top;
           MapWindowPoints( hCaret, hWnd, (LPPOINT)&rc, 2 );
           if( IntersectRect(lprc, lprc, &rc) )
           {
               HideCaret(0);
  	       lprc->left = pt.x; lprc->top = pt.y;
	       return TRUE;
           }
       }
   }
   return FALSE;
}

/*************************************************************************
 *           ShowScrollBar   (USER.5)
 *
 * RETURNS STD
 */
BOOL STDCALL ShowScrollBar(
	HWND hwnd, INT nBar, BOOL fShow )
{
    SCROLL_ShowScrollBar( hwnd, nBar, (nBar == SB_VERT) ? 0 : fShow,
                                      (nBar == SB_HORZ) ? 0 : fShow );
    return TRUE;
}




/*************************************************************************
 *           EnableScrollBar   (USER.171)
 */
WINBOOL
STDCALL
EnableScrollBar(HWND hWnd,UINT wSBflags, UINT wArrows)
{
    BOOL bFineWithMe;
    SCROLLBAR_INFO *infoPtr;

    DPRINT( "%04x %d %d\n", hwnd, nBar, flags );

     wArrows &= ESB_DISABLE_BOTH;

    if (wSBflags == SB_BOTH)
    {
	if (!(infoPtr = SCROLL_GetScrollInfo( hWnd, SB_VERT ))) return FALSE;
	if (!(bFineWithMe = (infoPtr->flags ==  wArrows)) )
	{
	    infoPtr->flags =  wArrows;
	    SCROLL_RefreshScrollBar( hWnd, SB_VERT, TRUE, TRUE );
	}
	wSBflags = SB_HORZ;
    }
    else
	bFineWithMe = TRUE;
   
    if (!(infoPtr = SCROLL_GetScrollInfo( hWnd, wSBflags ))) return FALSE;
    if (bFineWithMe && infoPtr->flags ==  wArrows) return FALSE;
    infoPtr->flags =  wArrows;

    SCROLL_RefreshScrollBar( hWnd, wSBflags, TRUE, TRUE );
    return TRUE;
}
