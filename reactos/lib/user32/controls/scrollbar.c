/* $Id: scrollbar.c,v 1.5 2002/12/21 19:23:50 jfilby Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* GLOBAL VARIABLES **********************************************************/
/*
static HBITMAP hUpArrow;
static HBITMAP hDnArrow;
static HBITMAP hLfArrow;
static HBITMAP hRgArrow;
static HBITMAP hUpArrowD;
static HBITMAP hDnArrowD;
static HBITMAP hLfArrowD;
static HBITMAP hRgArrowD;
static HBITMAP hUpArrowI;
static HBITMAP hDnArrowI;
static HBITMAP hLfArrowI;
static HBITMAP hRgArrowI;
*/
#define TOP_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_UP) ? hUpArrowI : ((pressed) ? hUpArrowD:hUpArrow))
#define BOTTOM_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_DOWN) ? hDnArrowI : ((pressed) ? hDnArrowD:hDnArrow))
#define LEFT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_LEFT) ? hLfArrowI : ((pressed) ? hLfArrowD:hLfArrow))
#define RIGHT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_RIGHT) ? hRgArrowI : ((pressed) ? hRgArrowD:hRgArrow))

#define SCROLL_ARROW_THUMB_OVERLAP 0     /* Overlap between arrows and thumb */
#define SCROLL_MIN_THUMB 6               /* Minimum size of the thumb in pixels */
#define SCROLL_FIRST_DELAY   200         /* Delay (in ms) before first repetition when holding the button down */
#define SCROLL_REPEAT_DELAY  50          /* Delay (in ms) between scroll repetitions */
#define SCROLL_TIMER   0                 /* Scroll timer id */

 /* What to do after SCROLL_SetScrollInfo() */
#define SA_SSI_HIDE		0x0001
#define SA_SSI_SHOW		0x0002
#define SA_SSI_REFRESH		0x0004
#define SA_SSI_REPAINT_ARROWS	0x0008

  /* Scroll-bar hit testing */
enum SCROLL_HITTEST
{
  SCROLL_NOWHERE,		/* Outside the scroll bar */
  SCROLL_TOP_ARROW,		/* Top or left arrow */
  SCROLL_TOP_RECT,		/* Rectangle between the top arrow and the thumb */
  SCROLL_THUMB,			/* Thumb rectangle */
  SCROLL_BOTTOM_RECT,		/* Rectangle between the thumb and the bottom arrow */
  SCROLL_BOTTOM_ARROW		/* Bottom or right arrow */
};

static BOOL SCROLL_MovingThumb = FALSE; /* Is the moving thumb being displayed? */

/* Thumb-tracking info */
static HWND SCROLL_TrackingWin = 0;
static INT SCROLL_TrackingBar = 0;
static INT SCROLL_TrackingPos = 0;
/* static INT  SCROLL_TrackingVal = 0; */
static enum SCROLL_HITTEST SCROLL_trackHitTest; /* Hit test code of the last button-down event */
static BOOL SCROLL_trackVertical;

/* FUNCTIONS *****************************************************************/

HBRUSH DefWndControlColor (HDC hDC, UINT ctlType);
HPEN STDCALL GetSysColorPen (int nIndex);



WINBOOL STDCALL
GetScrollBarInfo (HWND hwnd, LONG idObject, PSCROLLBARINFO psbi)
{
  int ret = NtUserGetScrollBarInfo (hwnd, idObject, psbi);

  return ret;
}

/* Ported from WINE20020904 */
/* Draw the scroll bar interior (everything except the arrows). */
static void
SCROLL_DrawInterior (HWND hwnd, HDC hdc, INT nBar,
		     RECT * rect, INT arrowSize,
		     INT thumbSize, INT thumbPos,
		     UINT flags, BOOL top_selected, BOOL bottom_selected)
{
  RECT r;
  HPEN hSavePen;
  HBRUSH hSaveBrush, hBrush;

  /* Only scrollbar controls send WM_CTLCOLORSCROLLBAR.
   * The window-owned scrollbars need to call DefWndControlColor
   * to correctly setup default scrollbar colors
   */
  if (nBar == SB_CTL)
    {
      hBrush = (HBRUSH) NtUserSendMessage (GetParent (hwnd), WM_CTLCOLORSCROLLBAR,
                                           (WPARAM) hdc, (LPARAM) hwnd);
    }
  else
    {
/*      hBrush = NtUserGetControlColor (hdc, CTLCOLOR_SCROLLBAR); FIXME */ /* DefWndControlColor */
      hBrush = GetSysColorBrush(COLOR_SCROLLBAR);
    }

  hSavePen = SelectObject (hdc, GetSysColorPen (COLOR_WINDOWFRAME));
  hSaveBrush = SelectObject (hdc, hBrush);

  /* Calculate the scroll rectangle */
  r = *rect;

  if (nBar == SB_VERT)
    {
      r.top += arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
      r.bottom -= (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
    }
  else if (nBar == SB_HORZ)
    {
      r.left += arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
      r.right -= (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
    }

  /* Draw the scroll rectangles and thumb */
  if (!thumbPos)		/* No thumb to draw */
    {
      PatBlt (hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, PATCOPY);

      /* cleanup and return */
      SelectObject (hdc, hSavePen);
      SelectObject (hdc, hSaveBrush);
      return;
    }
  if (nBar == SB_VERT)
    {
      PatBlt (hdc, r.left, r.top, r.right - r.left, thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP),
              top_selected ? 0x0f0000 : PATCOPY);
      r.top += thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
      PatBlt (hdc, r.left, r.top + thumbSize, r.right - r.left,
              r.bottom - r.top - thumbSize,
              bottom_selected ? 0x0f0000 : PATCOPY);
      r.bottom = r.top + thumbSize;
    }
  else if (nBar == SB_HORZ)
    {
      PatBlt (hdc, r.left, r.top, thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP),
              r.bottom - r.top, top_selected ? 0x0f0000 : PATCOPY);
      r.left += thumbPos - (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
      PatBlt (hdc, r.left + thumbSize, r.top,
              r.right - r.left - thumbSize,
              r.bottom - r.top, bottom_selected ? 0x0f0000 : PATCOPY);
      r.right = r.left + thumbSize;
    }

  /* Draw the thumb */
  DrawEdge (hdc, &r, EDGE_RAISED, BF_RECT | BF_MIDDLE);

  /* cleanup */
  SelectObject (hdc, hSavePen);
  SelectObject (hdc, hSaveBrush);
}

/* Ported from WINE20020904 */
static void
SCROLL_DrawMovingThumb (HDC hdc, RECT * rect, int nBar,
			INT arrowSize, INT thumbSize)
{
  INT pos = SCROLL_TrackingPos;
  INT max_size;

  if (nBar == SB_VERT)
    max_size = rect->bottom - rect->top;
  else if (nBar == SB_HORZ)
    max_size = rect->right - rect->left;

  max_size -= (arrowSize - SCROLL_ARROW_THUMB_OVERLAP) + thumbSize;

  if (pos < (arrowSize - SCROLL_ARROW_THUMB_OVERLAP))
    pos = (arrowSize - SCROLL_ARROW_THUMB_OVERLAP);
  else if (pos > max_size)
    pos = max_size;

  SCROLL_DrawInterior (SCROLL_TrackingWin, hdc, SCROLL_TrackingBar,
		       rect, arrowSize, thumbSize, pos,
		       0, FALSE, FALSE);

  SCROLL_MovingThumb = !SCROLL_MovingThumb;
}

/* Ported from WINE20020904 */
/* Draw the scroll bar arrows. */
static void
SCROLL_DrawArrows (HDC hdc, PSCROLLBARINFO info,
		   RECT * rect, INT arrowSize, int nBar,
		   BOOL top_pressed, BOOL bottom_pressed)
{
  RECT r;
  int scrollDirFlag1, scrollDirFlag2;

  if (nBar == SB_VERT)
  {
    scrollDirFlag1 = DFCS_SCROLLUP;
    scrollDirFlag2 = DFCS_SCROLLDOWN;
  }
  else if (nBar == SB_HORZ)
  {
    scrollDirFlag1 = DFCS_SCROLLLEFT;
    scrollDirFlag2 = DFCS_SCROLLRIGHT;
  }

  r = *rect;
  if (nBar == SB_VERT)
    r.bottom = r.top + arrowSize;
  else if (nBar == SB_HORZ)
    r.right = r.left + arrowSize;

  DrawFrameControl (hdc, &r, DFC_SCROLL,
		    scrollDirFlag1 | (top_pressed ? (DFCS_PUSHED | DFCS_FLAT) : 0)
		    /* | (info.flags&ESB_DISABLE_LTUP ? DFCS_INACTIVE : 0) */
    );
  r = *rect;
  if (nBar == SB_VERT)
    r.top = r.bottom - arrowSize;
  else if (nBar == SB_HORZ)
    r.left = r.right - arrowSize;
  DrawFrameControl (hdc, &r, DFC_SCROLL,
		    scrollDirFlag2 | (bottom_pressed ? (DFCS_PUSHED | DFCS_FLAT) : 0)
		    /* | (info.flags&ESB_DISABLE_RTDN ? DFCS_INACTIVE : 0) */
    );
}

/* Ported from WINE20020904 */
/* Redraw the whole scrollbar. */
void
SCROLL_DrawScrollBar (HWND hwnd, HDC hdc, INT nBar,
		      BOOL arrows, BOOL interior)
{
  INT arrowSize = 20, thumbSize = 20, thumbPos = 1; /* FIXME: Should not be assign values, but rather obtaining */
/*    WND *wndPtr = WIN_FindWndPtr( hwnd ); */
  SCROLLBARINFO info;
  BOOL Save_SCROLL_MovingThumb = SCROLL_MovingThumb;

  info.cbSize = sizeof(SCROLLBARINFO);
  GetScrollBarInfo (hwnd, nBar, &info);

/*    if (!wndPtr || !info ||
        ((nBar == SB_VERT) && !(wndPtr->dwStyle & WS_VSCROLL)) ||
        ((nBar == SB_HORZ) && !(wndPtr->dwStyle & WS_HSCROLL))) goto END;
    if (!WIN_IsWindowDrawable( hwnd, FALSE )) goto END;
    hwnd = wndPtr->hwndSelf; */ /* make it a full handle */

  if (IsRectEmpty (&(info.rcScrollBar))) goto END;

  if (Save_SCROLL_MovingThumb && (SCROLL_TrackingWin == hwnd) && (SCROLL_TrackingBar == nBar))
  {
    SCROLL_DrawMovingThumb (hdc, &(info.rcScrollBar), nBar, arrowSize, thumbSize);
  }

  /* Draw the arrows */
  if (arrows && arrowSize)
  {
    if (SCROLL_trackVertical == TRUE /* && GetCapture () == hwnd */)
    {
      SCROLL_DrawArrows (hdc, &info, &(info.rcScrollBar), arrowSize, nBar,
                         (SCROLL_trackHitTest == SCROLL_TOP_ARROW),
                         (SCROLL_trackHitTest == SCROLL_BOTTOM_ARROW));
    }
    else
    {
      SCROLL_DrawArrows (hdc, &info, &(info.rcScrollBar), arrowSize, nBar, FALSE, FALSE);
    }
  }

  if (interior)
  {
    SCROLL_DrawInterior (hwnd, hdc, nBar, &(info.rcScrollBar), arrowSize, thumbSize,
                         thumbPos, /* info->flags FIXME */ 0, FALSE, FALSE);
  }

  if (Save_SCROLL_MovingThumb &&
      (SCROLL_TrackingWin == hwnd) && (SCROLL_TrackingBar == nBar))
    SCROLL_DrawMovingThumb (hdc, &info.rcScrollBar, nBar, arrowSize, thumbSize);
  /* if scroll bar has focus, reposition the caret */

/*  if (hwnd == GetFocus () && (nBar == SB_CTL))
    {
      if (nBar == SB_HORZ)
	{
	  SetCaretPos (thumbPos + 1, info.rcScrollBar.top + 1);
	}
      else if (nBAR == SB_VERT)
	{
	  SetCaretPos (info.rcScrollBar.top + 1, thumbPos + 1);
	}
    } */
END:
/*    WIN_ReleaseWndPtr(wndPtr); */
}

WINBOOL STDCALL
GetScrollInfo (HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
  UNIMPLEMENTED;
  return FALSE;
}

int STDCALL
GetScrollPos (HWND hWnd, int nBar)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
GetScrollRange (HWND hWnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos)
{
  UNIMPLEMENTED;
  return FALSE;
}

int STDCALL
SetScrollInfo (HWND hwnd, int fnBar, LPCSCROLLINFO lpsi, WINBOOL fRedraw)
{
  UNIMPLEMENTED;
  return 0;
}

int STDCALL
SetScrollPos (HWND hWnd, int nBar, int nPos, WINBOOL bRedraw)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
SetScrollRange (HWND hWnd,
		int nBar, int nMinPos, int nMaxPos, WINBOOL bRedraw)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* Ported from WINE20020904 */
WINBOOL STDCALL
ShowScrollBar (HWND hWnd, int wBar, WINBOOL bShow)
{
  NtUserShowScrollBar (hWnd, wBar, bShow);
  return TRUE;
}
