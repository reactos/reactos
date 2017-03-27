/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS uxtheme.dll
 * FILE:            dll/win32/uxtheme/ncscrollbar.c
 * PURPOSE:         uxtheme scrollbar support
 * PROGRAMMER:      Giannis Adamopoulos
 *                  This file is heavily based on code from the wine project:
 *                  Copyright 1993 Martin Ayotte
 *                  Copyright 1994, 1996 Alexandre Julliard
 */
 
#include "uxthemep.h"

#include <assert.h>

static void ScreenToWindow( HWND hWnd, POINT* pt)
{
    RECT rcWnd;
    GetWindowRect(hWnd, &rcWnd);
    pt->x -= rcWnd.left;
    pt->y -= rcWnd.top;
}

static BOOL SCROLL_IsVertical(HWND hwnd, INT nBar)
{
    switch(nBar)
    {
    case SB_HORZ:
        return FALSE;
    case SB_VERT:
        return TRUE;
    default:
        assert(FALSE);
        return FALSE;
    }
}

static LONG SCROLL_getObjectId(INT nBar)
{
    switch(nBar)
    {
    case SB_HORZ:
        return OBJID_HSCROLL;
    case SB_VERT:
        return OBJID_VSCROLL;
    default:
        assert(FALSE);
        return 0;
    }
}

/***********************************************************************
 *           SCROLL_PtInRectEx
 */
static BOOL SCROLL_PtInRectEx( LPRECT lpRect, POINT pt, BOOL vertical )
{
    RECT rect = *lpRect;
    int scrollbarWidth;

    /* Pad hit rect to allow mouse to be dragged outside of scrollbar and
     * still be considered in the scrollbar. */
    if (vertical)
    {
        scrollbarWidth = lpRect->right - lpRect->left;
        rect.left -= scrollbarWidth*8;
        rect.right += scrollbarWidth*8;
        rect.top -= scrollbarWidth*2;
        rect.bottom += scrollbarWidth*2;
    }
    else
    {
        scrollbarWidth = lpRect->bottom - lpRect->top;
        rect.left -= scrollbarWidth*2;
        rect.right += scrollbarWidth*2;
        rect.top -= scrollbarWidth*8;
        rect.bottom += scrollbarWidth*8;
    }
    return PtInRect( &rect, pt );
}


/***********************************************************************
 *           SCROLL_HitTest
 *
 * Scroll-bar hit testing (don't confuse this with WM_NCHITTEST!).
 */
static enum SCROLL_HITTEST SCROLL_HitTest( HWND hwnd, SCROLLBARINFO* psbi, BOOL vertical,
                                           POINT pt, BOOL bDragging )
{
    if ( (bDragging && !SCROLL_PtInRectEx( &psbi->rcScrollBar, pt, vertical )) ||
	     (!PtInRect( &psbi->rcScrollBar, pt )) ) 
    {
         return SCROLL_NOWHERE;
    }

    if (vertical)
    {
        if (pt.y < psbi->rcScrollBar.top + psbi->dxyLineButton) 
            return SCROLL_TOP_ARROW;
        if (pt.y >= psbi->rcScrollBar.bottom - psbi->dxyLineButton) 
            return SCROLL_BOTTOM_ARROW;
        if (!psbi->xyThumbTop) 
            return SCROLL_TOP_RECT;
        pt.y -= psbi->rcScrollBar.top;
        if (pt.y < psbi->xyThumbTop) 
            return SCROLL_TOP_RECT;
        if (pt.y >= psbi->xyThumbBottom) 
            return SCROLL_BOTTOM_RECT;
    }
    else  /* horizontal */
    {
        if (pt.x < psbi->rcScrollBar.left + psbi->dxyLineButton)
            return SCROLL_TOP_ARROW;
        if (pt.x >= psbi->rcScrollBar.right - psbi->dxyLineButton) 
            return SCROLL_BOTTOM_ARROW;
        if (!psbi->xyThumbTop) 
            return SCROLL_TOP_RECT;
        pt.x -= psbi->rcScrollBar.left;
        if (pt.x < psbi->xyThumbTop) 
            return SCROLL_TOP_RECT;
        if (pt.x >= psbi->xyThumbBottom) 
            return SCROLL_BOTTOM_RECT;
    }
    return SCROLL_THUMB;
}

static void SCROLL_ThemeDrawPart(PDRAW_CONTEXT pcontext, int iPartId,int iStateId,  SCROLLBARINFO* psbi, int htCurrent, int htDown, int htHot, RECT* r)
{
    if(psbi->rgstate[htCurrent] & STATE_SYSTEM_UNAVAILABLE)
        iStateId += BUTTON_DISABLED - BUTTON_NORMAL;
    else if (htHot == htCurrent)
        iStateId += BUTTON_HOT - BUTTON_NORMAL;
    else if (htDown == htCurrent)
        iStateId += BUTTON_PRESSED - BUTTON_NORMAL;

    DrawThemeBackground(pcontext->scrolltheme, pcontext->hDC, iPartId, iStateId, r, NULL);
}

/***********************************************************************
 *           SCROLL_DrawArrows
 *
 * Draw the scroll bar arrows.
 */
static void SCROLL_DrawArrows( PDRAW_CONTEXT pcontext, SCROLLBARINFO* psbi, 
                               BOOL vertical, int htDown, int htHot )
{
    RECT r;
    int iStateId;

    r = psbi->rcScrollBar;
    if( vertical )
    {
        r.bottom = r.top + psbi->dxyLineButton;
        iStateId = ABS_UPNORMAL;
    }
    else
    {
        r.right = r.left + psbi->dxyLineButton;
        iStateId = ABS_LEFTNORMAL;
    }
    
    SCROLL_ThemeDrawPart(pcontext, SBP_ARROWBTN, iStateId, psbi, SCROLL_TOP_ARROW, htDown, htHot, &r);
    
    r = psbi->rcScrollBar;
    if( vertical )
    {
        r.top = r.bottom - psbi->dxyLineButton;
        iStateId = ABS_DOWNNORMAL;
    }
    else
    {
        iStateId = ABS_RIGHTNORMAL;
        r.left = r.right - psbi->dxyLineButton;
    }

    SCROLL_ThemeDrawPart(pcontext, SBP_ARROWBTN, iStateId, psbi, SCROLL_BOTTOM_ARROW, htDown, htHot, &r);
}

static void SCROLL_DrawInterior( PDRAW_CONTEXT pcontext, SCROLLBARINFO* psbi,
                                  INT thumbPos, BOOL vertical,
                                  int htDown, int htHot )
{
    RECT r, rcPart;

    /* thumbPos is relative to the edge of the scrollbar */

    r = psbi->rcScrollBar;
    if (vertical)
    {
        thumbPos += pcontext->wi.rcClient.top - pcontext->wi.rcWindow.top;
        r.top    += psbi->dxyLineButton;
        r.bottom -= (psbi->dxyLineButton);
    }
    else
    {
        thumbPos += pcontext->wi.rcClient.left - pcontext->wi.rcWindow.left;
        r.left  += psbi->dxyLineButton;
        r.right -= psbi->dxyLineButton;
    }

    /* Draw the scroll rectangles and thumb */

    if (!thumbPos)  /* No thumb to draw */
    {
        rcPart = r;
        SCROLL_ThemeDrawPart(pcontext, vertical ? SBP_UPPERTRACKVERT: SBP_UPPERTRACKHORZ , BUTTON_NORMAL, psbi, SCROLL_THUMB, 0, 0, &rcPart);
        return;
    }

    if (vertical)
    { 
        rcPart = r;
        rcPart.bottom = thumbPos;
        SCROLL_ThemeDrawPart(pcontext, SBP_UPPERTRACKVERT, BUTTON_NORMAL, psbi, SCROLL_TOP_RECT, htDown, htHot, &rcPart);
        r.top = rcPart.bottom;

        rcPart = r;
        rcPart.top += psbi->xyThumbBottom - psbi->xyThumbTop;
        SCROLL_ThemeDrawPart(pcontext, SBP_LOWERTRACKVERT, BUTTON_NORMAL, psbi, SCROLL_BOTTOM_RECT, htDown, htHot, &rcPart); 
        r.bottom = rcPart.top;

        SCROLL_ThemeDrawPart(pcontext, SBP_THUMBBTNVERT, BUTTON_NORMAL, psbi, SCROLL_THUMB, htDown, htHot, &r); 
        SCROLL_ThemeDrawPart(pcontext, SBP_GRIPPERVERT, BUTTON_NORMAL, psbi, SCROLL_THUMB, htDown, htHot, &r); 
    }
    else  /* horizontal */
    {
        rcPart = r;
        rcPart.right = thumbPos;
        SCROLL_ThemeDrawPart(pcontext, SBP_UPPERTRACKHORZ, BUTTON_NORMAL, psbi, SCROLL_TOP_RECT, htDown, htHot, &rcPart);
        r.left = rcPart.right;

        rcPart = r;
        rcPart.left += psbi->xyThumbBottom - psbi->xyThumbTop;
        SCROLL_ThemeDrawPart(pcontext, SBP_LOWERTRACKHORZ, BUTTON_NORMAL, psbi, SCROLL_BOTTOM_RECT, htDown, htHot, &rcPart);
        r.right = rcPart.left;

        SCROLL_ThemeDrawPart(pcontext, SBP_THUMBBTNHORZ, BUTTON_NORMAL, psbi, SCROLL_THUMB, htDown, htHot, &r);
        SCROLL_ThemeDrawPart(pcontext, SBP_GRIPPERHORZ, BUTTON_NORMAL, psbi, SCROLL_THUMB, htDown, htHot, &r);
    }
}

static void SCROLL_DrawMovingThumb(PWND_CONTEXT pwndContext, PDRAW_CONTEXT pcontext, SCROLLBARINFO* psbi,  BOOL vertical)
{
  INT pos = pwndContext->SCROLL_TrackingPos;
  INT max_size;

  if( vertical )
      max_size = psbi->rcScrollBar.bottom - psbi->rcScrollBar.top;
  else
      max_size = psbi->rcScrollBar.right - psbi->rcScrollBar.left;

  max_size -= psbi->xyThumbBottom - psbi->xyThumbTop + psbi->dxyLineButton;

  if( pos < (psbi->dxyLineButton) )
    pos = (psbi->dxyLineButton);
  else if( pos > max_size )
    pos = max_size;

  SCROLL_DrawInterior(pcontext, psbi, pos, vertical, SCROLL_THUMB, 0);  

  pwndContext->SCROLL_MovingThumb = !pwndContext->SCROLL_MovingThumb;
}


void 
ThemeDrawScrollBar(PDRAW_CONTEXT pcontext, INT nBar, POINT* pt)
{
    SCROLLINFO si;
    SCROLLBARINFO sbi;
    BOOL vertical;
    enum SCROLL_HITTEST htHot = SCROLL_NOWHERE;
    PWND_CONTEXT pwndContext;

    if (((nBar == SB_VERT) && !(pcontext->wi.dwStyle & WS_VSCROLL)) ||
        ((nBar == SB_HORZ) && !(pcontext->wi.dwStyle & WS_HSCROLL))) return;

    if (!(pwndContext = ThemeGetWndContext(pcontext->hWnd)))
        return;

    if (pwndContext->SCROLL_TrackingWin)
        return;

    /* Retrieve scrollbar info */
    sbi.cbSize = sizeof(sbi);
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL ;
    GetScrollInfo(pcontext->hWnd, nBar, &si);
    GetScrollBarInfo(pcontext->hWnd, SCROLL_getObjectId(nBar), &sbi);
    vertical = SCROLL_IsVertical(pcontext->hWnd, nBar);
    if(sbi.rgstate[SCROLL_TOP_ARROW] & STATE_SYSTEM_UNAVAILABLE  && 
       sbi.rgstate[SCROLL_BOTTOM_ARROW] & STATE_SYSTEM_UNAVAILABLE  )
    {
        sbi.xyThumbTop = 0;
    }

    /* The scrollbar rect is in screen coordinates */
    OffsetRect(&sbi.rcScrollBar, -pcontext->wi.rcWindow.left, -pcontext->wi.rcWindow.top);

    if(pt)
    {
        ScreenToWindow(pcontext->hWnd, pt);
        htHot = SCROLL_HitTest(pcontext->hWnd, &sbi, vertical, *pt, FALSE);
    }

    /* do not draw if the scrollbar rectangle is empty */
    if(IsRectEmpty(&sbi.rcScrollBar)) return;

    /* Draw the scrollbar */
    SCROLL_DrawArrows( pcontext, &sbi, vertical, 0, htHot );
	SCROLL_DrawInterior( pcontext, &sbi, sbi.xyThumbTop, vertical, 0, htHot );
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
 *           SCROLL_GetThumbVal
 *
 * Compute the current scroll position based on the thumb position in pixels
 * from the top of the scroll-bar.
 */
static UINT SCROLL_GetThumbVal( SCROLLINFO *psi, RECT *rect,
                                  BOOL vertical, INT pos )
{
    INT thumbSize;
    INT pixels = vertical ? rect->bottom-rect->top : rect->right-rect->left;
    INT range;

    if ((pixels -= 2*(GetSystemMetrics(SM_CXVSCROLL) - SCROLL_ARROW_THUMB_OVERLAP)) <= 0)
        return psi->nMin;

    if (psi->nPage)
    {
        thumbSize = MulDiv(pixels,psi->nPage,(psi->nMax-psi->nMin+1));
        if (thumbSize < SCROLL_MIN_THUMB) thumbSize = SCROLL_MIN_THUMB;
    }
    else thumbSize = GetSystemMetrics(SM_CXVSCROLL);

    if ((pixels -= thumbSize) <= 0) return psi->nMin;

    pos = max( 0, pos - (GetSystemMetrics(SM_CXVSCROLL) - SCROLL_ARROW_THUMB_OVERLAP) );
    if (pos > pixels) pos = pixels;

    if (!psi->nPage)
        range = psi->nMax - psi->nMin;
    else
        range = psi->nMax - psi->nMin - psi->nPage + 1;

    return psi->nMin + MulDiv(pos, range, pixels);
}

static void 
SCROLL_HandleScrollEvent(PWND_CONTEXT pwndContext, HWND hwnd, INT nBar, UINT msg, POINT pt)
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
    SCROLLINFO si;
    SCROLLBARINFO sbi;
    DRAW_CONTEXT context;

    si.cbSize = sizeof(si);
    sbi.cbSize = sizeof(sbi);
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd, nBar, &si);
    GetScrollBarInfo(hwnd, SCROLL_getObjectId(nBar), &sbi);
    vertical = SCROLL_IsVertical(hwnd, nBar);
    if(sbi.rgstate[SCROLL_TOP_ARROW] & STATE_SYSTEM_UNAVAILABLE  && 
       sbi.rgstate[SCROLL_BOTTOM_ARROW] & STATE_SYSTEM_UNAVAILABLE  )
    {
        return;
    }

    if ((pwndContext->SCROLL_trackHitTest == SCROLL_NOWHERE) && (msg != WM_LBUTTONDOWN))
		  return;
    
    ThemeInitDrawContext(&context, hwnd, 0);

    /* The scrollbar rect is in screen coordinates */
    OffsetRect(&sbi.rcScrollBar, -context.wi.rcWindow.left, -context.wi.rcWindow.top);

    hwndOwner = (nBar == SB_CTL) ? GetParent(hwnd) : hwnd;
    hwndCtl   = (nBar == SB_CTL) ? hwnd : 0;

    switch(msg)
    {
      case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
          HideCaret(hwnd);  /* hide caret while holding down LBUTTON */
          pwndContext->SCROLL_trackVertical = vertical;
          pwndContext->SCROLL_trackHitTest  = hittest = SCROLL_HitTest( hwnd, &sbi, vertical, pt, FALSE );
          lastClickPos  = vertical ? (pt.y - sbi.rcScrollBar.top) : (pt.x - sbi.rcScrollBar.left);
          lastMousePos  = lastClickPos;
          trackThumbPos = sbi.xyThumbTop;
          prevPt = pt;
          SetCapture( hwnd );
          break;

      case WM_MOUSEMOVE:
          hittest = SCROLL_HitTest( hwnd, &sbi, vertical, pt, TRUE );
          prevPt = pt;
          break;

      case WM_LBUTTONUP:
          hittest = SCROLL_NOWHERE;
          ReleaseCapture();
          /* if scrollbar has focus, show back caret */
          if (hwnd==GetFocus()) 
              ShowCaret(hwnd);
          break;

      case WM_SYSTIMER:
          pt = prevPt;
          hittest = SCROLL_HitTest( hwnd, &sbi, vertical, pt, FALSE );
          break;

      default:
          return;  /* Should never happen */
    }

    //TRACE("Event: hwnd=%p bar=%d msg=%s pt=%d,%d hit=%d\n",
    //      hwnd, nBar, SPY_GetMsgName(msg,hwnd), pt.x, pt.y, hittest );

    switch(pwndContext->SCROLL_trackHitTest)
    {
    case SCROLL_NOWHERE:  /* No tracking in progress */
        break;

    case SCROLL_TOP_ARROW:
        if (hittest == pwndContext->SCROLL_trackHitTest)
        {
            SCROLL_DrawArrows( &context, &sbi, vertical, pwndContext->SCROLL_trackHitTest, 0 );
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_LINEUP, (LPARAM)hwndCtl );
	        }

        SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                            SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY, NULL );
        }
        else
        {
            SCROLL_DrawArrows( &context, &sbi, vertical, 0, 0 );
            KillSystemTimer( hwnd, SCROLL_TIMER );
        }

        break;

    case SCROLL_TOP_RECT:
        SCROLL_DrawInterior( &context, &sbi, sbi.xyThumbTop, vertical, pwndContext->SCROLL_trackHitTest, 0);
        if (hittest == pwndContext->SCROLL_trackHitTest)
        {
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_PAGEUP, (LPARAM)hwndCtl );
            }
            SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                              SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY, NULL );
        }
        else KillSystemTimer( hwnd, SCROLL_TIMER );
        break;

    case SCROLL_THUMB:
        if (msg == WM_LBUTTONDOWN)
        {
            pwndContext->SCROLL_TrackingWin = hwnd;
            pwndContext->SCROLL_TrackingBar = nBar;
            pwndContext->SCROLL_TrackingPos = trackThumbPos + lastMousePos - lastClickPos;
            pwndContext->SCROLL_TrackingVal = SCROLL_GetThumbVal( &si, &sbi.rcScrollBar, 
                                                     vertical, pwndContext->SCROLL_TrackingPos );
	        if (!pwndContext->SCROLL_MovingThumb)
		        SCROLL_DrawMovingThumb(pwndContext, &context, &sbi, vertical);
        }
        else if (msg == WM_LBUTTONUP)
        {
	        if (pwndContext->SCROLL_MovingThumb)
		        SCROLL_DrawMovingThumb(pwndContext, &context, &sbi, vertical);

            SCROLL_DrawInterior(  &context, &sbi, sbi.xyThumbTop, vertical, 0, pwndContext->SCROLL_trackHitTest );
        }
        else  /* WM_MOUSEMOVE */
        {
            INT pos;

            if (!SCROLL_PtInRectEx( &sbi.rcScrollBar, pt, vertical )) 
                pos = lastClickPos;
            else
            {
                pt = SCROLL_ClipPos( &sbi.rcScrollBar, pt );
                pos = vertical ? (pt.y - sbi.rcScrollBar.top) : (pt.x - sbi.rcScrollBar.left);
            }
            if ( (pos != lastMousePos) || (!pwndContext->SCROLL_MovingThumb) )
            {
                if (pwndContext->SCROLL_MovingThumb)
                    SCROLL_DrawMovingThumb(pwndContext, &context, &sbi, vertical);
                lastMousePos = pos;
                pwndContext->SCROLL_TrackingPos = trackThumbPos + pos - lastClickPos;
                pwndContext->SCROLL_TrackingVal = SCROLL_GetThumbVal( &si, &sbi.rcScrollBar,
                                                         vertical,
                                                         pwndContext->SCROLL_TrackingPos );
                SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                MAKEWPARAM( SB_THUMBTRACK, pwndContext->SCROLL_TrackingVal),
                                (LPARAM)hwndCtl );
                if (!pwndContext->SCROLL_MovingThumb)
                    SCROLL_DrawMovingThumb(pwndContext, &context, &sbi, vertical);
            }
        }
        break;

    case SCROLL_BOTTOM_RECT:
        if (hittest == pwndContext->SCROLL_trackHitTest)
        {
            SCROLL_DrawInterior(  &context, &sbi, sbi.xyThumbTop, vertical, pwndContext->SCROLL_trackHitTest, 0 );
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_PAGEDOWN, (LPARAM)hwndCtl );
            }
            SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                              SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY, NULL );
        }
        else
        {
            SCROLL_DrawInterior(  &context, &sbi, sbi.xyThumbTop, vertical, 0, 0 );
            KillSystemTimer( hwnd, SCROLL_TIMER );
        }
        break;

    case SCROLL_BOTTOM_ARROW:
        if (hittest == pwndContext->SCROLL_trackHitTest)
        {
            SCROLL_DrawArrows(  &context, &sbi, vertical, pwndContext->SCROLL_trackHitTest, 0 );
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_SYSTIMER))
            {
                SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                                SB_LINEDOWN, (LPARAM)hwndCtl );
	        }

        SetSystemTimer( hwnd, SCROLL_TIMER, (msg == WM_LBUTTONDOWN) ?
                            SCROLL_FIRST_DELAY : SCROLL_REPEAT_DELAY, NULL );
        }
        else
        {
            SCROLL_DrawArrows(  &context, &sbi, vertical, 0, 0 );
            KillSystemTimer( hwnd, SCROLL_TIMER );
        }
        break;
    }

    if (msg == WM_LBUTTONDOWN)
    {

        if (hittest == SCROLL_THUMB)
        {
            UINT val = SCROLL_GetThumbVal( &si, &sbi.rcScrollBar, vertical,
                                 trackThumbPos + lastMousePos - lastClickPos );
            SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                            MAKEWPARAM( SB_THUMBTRACK, val ), (LPARAM)hwndCtl );
        }
    }

    if (msg == WM_LBUTTONUP)
    {
        hittest = pwndContext->SCROLL_trackHitTest;
        pwndContext->SCROLL_trackHitTest = SCROLL_NOWHERE;  /* Terminate tracking */

        if (hittest == SCROLL_THUMB)
        {
            UINT val = SCROLL_GetThumbVal( &si, &sbi.rcScrollBar, vertical,
                                 trackThumbPos + lastMousePos - lastClickPos );
            SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                            MAKEWPARAM( SB_THUMBPOSITION, val ), (LPARAM)hwndCtl );
        }
        /* SB_ENDSCROLL doesn't report thumb position */
        SendMessageW( hwndOwner, vertical ? WM_VSCROLL : WM_HSCROLL,
                          SB_ENDSCROLL, (LPARAM)hwndCtl );

        /* Terminate tracking */
        pwndContext->SCROLL_TrackingWin = 0;
    }

    ThemeCleanupDrawContext(&context);
}

static void 
SCROLL_TrackScrollBar( HWND hwnd, INT scrollbar, POINT pt )
{
    MSG msg;
    PWND_CONTEXT pwndContext = ThemeGetWndContext(hwnd);
    if(!pwndContext)
        return;

    ScreenToWindow(hwnd, &pt);

    SCROLL_HandleScrollEvent(pwndContext, hwnd, scrollbar, WM_LBUTTONDOWN, pt );

    do
    {
        if (!GetMessageW( &msg, 0, 0, 0 )) break;
        if (CallMsgFilterW( &msg, MSGF_SCROLLBAR )) continue;
        if (msg.message == WM_LBUTTONUP ||
            msg.message == WM_MOUSEMOVE ||
            (msg.message == WM_SYSTIMER && msg.wParam == SCROLL_TIMER))
        {
            pt.x = GET_X_LPARAM(msg.lParam);
            pt.y = GET_Y_LPARAM(msg.lParam);
            ClientToScreen(hwnd, &pt);
            ScreenToWindow(hwnd, &pt);
            SCROLL_HandleScrollEvent(pwndContext, hwnd, scrollbar, msg.message, pt );
        }
        else
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
        if (!IsWindow( hwnd ))
        {
            ReleaseCapture();
            break;
        }
    } while (msg.message != WM_LBUTTONUP && GetCapture() == hwnd);
}

void NC_TrackScrollBar( HWND hwnd, WPARAM wParam, POINT pt )
{
    INT scrollbar;
    
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
    SCROLL_TrackScrollBar( hwnd, scrollbar, pt );
}
