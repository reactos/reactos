/* $Id: scrollbar.c,v 1.18 2003/09/25 21:16:56 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>
#include <draw.h>
#include <stdlib.h>
#include <string.h>

/* GLOBAL VARIABLES **********************************************************/

#define SBU_TOPRIGHT   0x10
#define SBU_BOTTOMLEFT 0x20
#define SBU_SCROLLBOX  0x40

#define TOP_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_UP) ? hUpArrowI : ((pressed) ? hUpArrowD:hUpArrow))
#define BOTTOM_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_DOWN) ? hDnArrowI : ((pressed) ? hDnArrowD:hDnArrow))
#define LEFT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_LEFT) ? hLfArrowI : ((pressed) ? hLfArrowD:hLfArrow))
#define RIGHT_ARROW(flags,pressed) \
   (((flags)&ESB_DISABLE_RIGHT) ? hRgArrowI : ((pressed) ? hRgArrowD:hRgArrow))

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
#define SCROLL_NOWHERE  0x00    /* Outside the scroll bar */
#define SCROLL_TOP_ARROW    0x01    /* Top or left arrow */
#define SCROLL_TOP_RECT 0x02    /* Rectangle between the top arrow and the thumb */
#define SCROLL_THUMB    0x03    /* Thumb rectangle */
#define SCROLL_BOTTOM_RECT  0x04    /* Rectangle between the thumb and the bottom arrow */
#define SCROLL_BOTTOM_ARROW 0x05    /* Bottom or right arrow */

static BOOL SCROLL_MovingThumb = FALSE; /* Is the moving thumb being displayed? */

/* Thumb-tracking info */
static HWND SCROLL_TrackingWin = 0;
static INT SCROLL_TrackingBar = 0;
static INT SCROLL_TrackingPos = 0;
/* static INT  SCROLL_TrackingVal = 0; */
static DWORD SCROLL_trackHitTest; /* Hit test code of the last button-down event */
static BOOL SCROLL_trackVertical;

/* INTERNAL FUNCTIONS *********************************************************/

HBRUSH DefWndControlColor (HDC hDC, UINT ctlType);

/* Ported from WINE20020904 */
/* Draw the scroll bar interior (everything except the arrows). */
static void
SCROLL_DrawInterior (HWND hwnd, HDC hdc, INT nBar, BOOL vertical, INT
arrowSize, PSCROLLBARINFO psbi)
{
  INT thumbSize = psbi->xyThumbBottom - psbi->xyThumbTop;
  RECT rc;
  HPEN hSavePen;
  HBRUSH hSaveBrush, hBrush;
  BOOLEAN top_selected = FALSE, bottom_selected = FALSE;
DbgPrint("[SCROLL_DrawInterior:%d]\n", nBar);
  if(psbi->rgstate[SCROLL_TOP_RECT] & STATE_SYSTEM_PRESSED)
  {
    top_selected = TRUE;
  }
  if(psbi->rgstate[SCROLL_BOTTOM_RECT] & STATE_SYSTEM_PRESSED)
  {
    bottom_selected = TRUE;
  }

  /* Only scrollbar controls send WM_CTLCOLORSCROLLBAR.
   * The window-owned scrollbars need to call DefWndControlColor
   * to correctly setup default scrollbar colors
   */
  if ( nBar == SB_CTL )
  {
    hBrush = (HBRUSH) NtUserSendMessage (GetParent (hwnd), WM_CTLCOLORSCROLLBAR, (WPARAM) hdc, (LPARAM) hwnd);
    if(!hBrush)
      hBrush = GetSysColorBrush(COLOR_SCROLLBAR);
  }
  else
  {
/*    hBrush = NtUserGetControlColor (hdc, CTLCOLOR_SCROLLBAR); FIXME
*/ /* DefWndControlColor */
    hBrush = GetSysColorBrush(COLOR_SCROLLBAR);
  }

  hSavePen = SelectObject (hdc, GetSysColorPen (COLOR_WINDOWFRAME));
  hSaveBrush = SelectObject (hdc, hBrush);

  /* Calculate the scroll rectangle */
  if (vertical)
  {
    rc.top = psbi->rcScrollBar.top + arrowSize;
    rc.bottom = psbi->rcScrollBar.bottom - arrowSize;
    rc.left = psbi->rcScrollBar.left;
    rc.right = psbi->rcScrollBar.right;
  }
  else
  {
    rc.top = psbi->rcScrollBar.top;
    rc.bottom = psbi->rcScrollBar.bottom;
    rc.left = psbi->rcScrollBar.left + arrowSize;
    rc.right = psbi->rcScrollBar.right - arrowSize;
  }

  /* Draw the scroll rectangles and thumb */
  if (!psbi->xyThumbBottom)		/* No thumb to draw */
  {
    PatBlt (hdc,
            rc.left,
            rc.top,
            rc.right - rc.left,
            rc.bottom - rc.top,
            PATCOPY);

    /* cleanup and return */
    SelectObject (hdc, hSavePen);
    SelectObject (hdc, hSaveBrush);
    return;
  }
  
  psbi->xyThumbTop -= arrowSize;

  if(psbi->dxyLineButton)
  {
      if (vertical)
      {
        if(thumbSize)
        {
          PatBlt (hdc,
                  rc.left,
                  rc.top,
                  rc.right - rc.left,
                  psbi->xyThumbTop,
                  top_selected ? 0x0f0000 : PATCOPY);
          rc.top += psbi->xyThumbTop;
          PatBlt (hdc,
                  rc.left,
                  rc.top + thumbSize,
                  rc.right - rc.left,
                  rc.bottom - rc.top - thumbSize,
                  bottom_selected ? 0x0f0000 : PATCOPY);
          rc.bottom = rc.top + thumbSize;
        }
        else
        {
          if(psbi->xyThumbTop)
              PatBlt (hdc,
                      rc.left,
                      psbi->dxyLineButton,
                      rc.right - rc.left,
                      rc.bottom - rc.bottom,
                      PATCOPY);
        }
      }
      else
      {
        if(thumbSize)
        {
          PatBlt (hdc,
                  rc.left,
                  rc.top,
                  psbi->xyThumbTop,
                  rc.bottom - rc.top,
                  top_selected ? 0x0f0000 : PATCOPY);
          rc.left += psbi->xyThumbTop;
          PatBlt (hdc,
                  rc.left + thumbSize,
                  rc.top,
                  rc.right - rc.left - thumbSize,
                  rc.bottom - rc.top,
                  bottom_selected ? 0x0f0000 : PATCOPY);
          rc.right = rc.left + thumbSize;
        }
        else
        {
          if(psbi->xyThumbTop)
              PatBlt (hdc,
                      psbi->dxyLineButton,
                      rc.top,
                      rc.right - rc.left,
                      rc.bottom - rc.top,
                      PATCOPY);
        }
      }
  }

  /* Draw the thumb */
  if(thumbSize)
    DrawEdge (hdc, &rc, EDGE_RAISED, BF_RECT | BF_MIDDLE);

  /* cleanup */
  SelectObject (hdc, hSavePen);
  SelectObject (hdc, hSaveBrush);
}

/* Ported from WINE20020904 */
static void
SCROLL_DrawMovingThumb (HDC hdc, RECT * rect, BOOL vertical, int arrowSize, int thumbSize, PSCROLLBARINFO psbi)
{
  INT pos = SCROLL_TrackingPos;
  INT max_size;

  if ( vertical )
    max_size = psbi->rcScrollBar.bottom - psbi->rcScrollBar.top;
  else
    max_size = psbi->rcScrollBar.right - psbi->rcScrollBar.left;

  max_size -= arrowSize + thumbSize;

  if (pos < arrowSize)
    pos = arrowSize;
  else if (pos > max_size)
    pos = max_size;

  SCROLL_DrawInterior (SCROLL_TrackingWin, hdc, SCROLL_TrackingBar, vertical, arrowSize, psbi);

  SCROLL_MovingThumb = !SCROLL_MovingThumb;
}

/* Ported from WINE20020904 */
/* Draw the scroll bar arrows. */
static void
SCROLL_DrawArrows (HDC hdc, PSCROLLBARINFO info,
           RECT * rect, INT arrowSize, BOOL vertical,
           BOOL top_pressed, BOOL bottom_pressed)
{
  RECT r1, r2;
  int scrollDirFlag1, scrollDirFlag2;

  r1 = r2 = *rect;
  if (vertical)
  {
    scrollDirFlag1 = DFCS_SCROLLUP;
    scrollDirFlag2 = DFCS_SCROLLDOWN;
    r1.bottom = r1.top + arrowSize;
    r2.top = r2.bottom - arrowSize;
  }
  else
  {
    scrollDirFlag1 = DFCS_SCROLLLEFT;
    scrollDirFlag2 = DFCS_SCROLLRIGHT;
    r1.right = r1.left + arrowSize;
    r2.left = r2.right - arrowSize;
  }

  DrawFrameControl (hdc, &r1, DFC_SCROLL,
            scrollDirFlag1 | (top_pressed ? (DFCS_PUSHED | DFCS_FLAT) : 0)
            /* | (info.flags&ESB_DISABLE_LTUP ? DFCS_INACTIVE : 0) */
    );
  DrawFrameControl (hdc, &r2, DFC_SCROLL,
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
  INT arrowSize = 0;
  INT thumbSize;
  SCROLLBARINFO info;
  BOOL Save_SCROLL_MovingThumb = SCROLL_MovingThumb;
  BOOL vertical;

  info.cbSize = sizeof(SCROLLBARINFO);

  switch ( nBar )
  {
  case SB_HORZ:
    vertical = FALSE;
    NtUserGetScrollBarInfo (hwnd, OBJID_HSCROLL, &info);
    break;
  case SB_VERT:
    vertical = TRUE;
    NtUserGetScrollBarInfo (hwnd, OBJID_VSCROLL, &info);
    break;
  case SB_CTL:
    vertical = (GetWindowLongW(hwnd,GWL_STYLE)&SBS_VERT) != 0;
    NtUserGetScrollBarInfo (hwnd, OBJID_CLIENT, &info);
    break;
#ifdef DBG
  default:
    DASSERT(!"SCROLL_DrawScrollBar() called with invalid nBar");
    break;
#endif /* DBG */
  }
  
  thumbSize = info.xyThumbBottom - info.xyThumbTop;

  arrowSize = info.dxyLineButton;

  if (IsRectEmpty (&(info.rcScrollBar))) goto END;

  if (Save_SCROLL_MovingThumb && (SCROLL_TrackingWin == hwnd) && (SCROLL_TrackingBar == nBar))
  {
    SCROLL_DrawMovingThumb (hdc, &(info.rcScrollBar), vertical, arrowSize, thumbSize, &info);
  }

  /* Draw the arrows */
  if (arrows && arrowSize)
  {
    if (SCROLL_trackVertical == TRUE /* && GetCapture () == hwnd */)
    {
      SCROLL_DrawArrows (hdc, &info, &(info.rcScrollBar), arrowSize, vertical,
                         (SCROLL_trackHitTest == SCROLL_TOP_ARROW),
                         (SCROLL_trackHitTest == SCROLL_BOTTOM_ARROW));
    }
    else
    {
      SCROLL_DrawArrows (hdc, &info, &(info.rcScrollBar), arrowSize, vertical, FALSE, FALSE);
    }
  }

  if (interior)
  {
    SCROLL_DrawInterior (hwnd, hdc, nBar, vertical, arrowSize, &info);
  }

  if (Save_SCROLL_MovingThumb &&
      (SCROLL_TrackingWin == hwnd) && (SCROLL_TrackingBar == nBar))
    SCROLL_DrawMovingThumb (hdc, &info.rcScrollBar, vertical, arrowSize, thumbSize, &info);
  /* if scroll bar has focus, reposition the caret */

/*  if (hwnd == GetFocus () && (nBar == SB_CTL))
    {
      if (nBar == SB_HORZ)
      {
        SetCaretPos (info.dxyLineButton + 1, info.rcScrollBar.top + 1);
      }
      else if (nBAR == SB_VERT)
      {
        SetCaretPos (info.rcScrollBar.top + 1, info.dxyLineButton + 1);
      }
    } */
END:;
/*    WIN_ReleaseWndPtr(wndPtr); */
}

static BOOL 
SCROLL_PtInRectEx( LPRECT lpRect, POINT pt, BOOL vertical )
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

DWORD
SCROLL_HitTest( HWND hwnd, INT nBar, POINT pt, BOOL bDragging )
{
  SCROLLBARINFO sbi;
  RECT wndrect;
  INT arrowSize, thumbSize, thumbPos;
  BOOL vertical = (nBar == SB_VERT); /* FIXME - ((Window->Style & SBS_VERT) != 0) */
  
  NtUserGetWindowRect(hwnd, &wndrect);
  
  sbi.cbSize = sizeof(SCROLLBARINFO);
  NtUserGetScrollBarInfo(hwnd, vertical ? OBJID_VSCROLL : OBJID_HSCROLL, &sbi);
  
  OffsetRect(&sbi.rcScrollBar, wndrect.left, wndrect.top);

  if ( (bDragging && !SCROLL_PtInRectEx( &sbi.rcScrollBar, pt, vertical )) ||
       (!PtInRect( &sbi.rcScrollBar, pt )) ) return SCROLL_NOWHERE;

  thumbPos = sbi.xyThumbTop;
  thumbSize = sbi.xyThumbBottom - thumbPos;
  arrowSize = sbi.dxyLineButton;

  if (vertical) 
  {
    if (pt.y < sbi.rcScrollBar.top + arrowSize) return SCROLL_TOP_ARROW;
    if (pt.y >= sbi.rcScrollBar.bottom - arrowSize) return SCROLL_BOTTOM_ARROW;
    if (!thumbPos) return SCROLL_TOP_RECT;
      pt.y -= sbi.rcScrollBar.top;
    if (pt.y < thumbPos) return SCROLL_TOP_RECT;
    if (pt.y >= thumbPos + thumbSize) return SCROLL_BOTTOM_RECT;
  }
  else  /* horizontal */
  {
    if (pt.x < sbi.rcScrollBar.left + arrowSize) return SCROLL_TOP_ARROW;
    if (pt.x >= sbi.rcScrollBar.right - arrowSize) return SCROLL_BOTTOM_ARROW;
    if (!thumbPos) return SCROLL_TOP_RECT;
    pt.x -= sbi.rcScrollBar.left;
    if (pt.x < thumbPos) return SCROLL_TOP_RECT;
    if (pt.x >= thumbPos + thumbSize) return SCROLL_BOTTOM_RECT;
  }
  return SCROLL_THUMB;
}


/* FUNCTIONS ******************************************************************/


/*
 * @implemented
 */
WINBOOL STDCALL
EnableScrollBar(HWND hWnd, UINT wSBflags, UINT wArrows)
{
  return NtUserEnableScrollBar(hWnd, wSBflags, wArrows);
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetScrollBarInfo(HWND hwnd, LONG idObject, PSCROLLBARINFO psbi)
{
  return NtUserGetScrollBarInfo (hwnd, idObject, psbi);
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetScrollInfo (HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
  return (WINBOOL)NtUserGetScrollInfo(hwnd, fnBar, lpsi);
}


/*
 * @implemented
 */
int STDCALL
GetScrollPos (HWND hWnd, int nBar)
{
  SCROLLINFO si;
  BOOL ret;
  int res = 0;
  
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS;
  ret = NtUserGetScrollInfo(hWnd, nBar, &si);
  if(ret)
    res = si.nPos;
  return res;  
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetScrollRange (HWND hWnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos)
{
  WINBOOL ret;
  SCROLLINFO si;
  
  if(!lpMinPos || !lpMaxPos)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE;
  ret = NtUserGetScrollInfo(hWnd, nBar, &si);
  if(ret)
  {
    *lpMinPos = si.nMin;
    *lpMaxPos = si.nMax;
  }
  
  return ret;
}


/*
 * @implemented
 */
int STDCALL
SetScrollInfo (HWND hwnd, int fnBar, LPCSCROLLINFO lpsi, WINBOOL fRedraw)
{
  DWORD Changed;
  int Ret = (int)NtUserSetScrollInfo(hwnd, fnBar, (LPSCROLLINFO)lpsi, &Changed);
  if(fRedraw && Changed)
  {
  
  }
  
  return Ret;
}


/*
 * @implemented
 */
int STDCALL
SetScrollPos (HWND hWnd, int nBar, int nPos, WINBOOL bRedraw)
{
  int Res = 0;
  BOOL ret;
  DWORD Action;
  SCROLLINFO si;
  
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS;
  
  /* call NtUserGetScrollInfo() because we need to return the previous position */
  ret = NtUserGetScrollInfo(hWnd, nBar, &si);
  
  if(ret)
  {
    Res = si.nPos;
    
    if(Res != nPos)
    {
      si.nPos = nPos;
      /* finally set the new position */
      NtUserSetScrollInfo(hWnd, nBar, &si, &Action);
    }
  }
  
  return Res;
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetScrollRange (HWND hWnd,
		int nBar, int nMinPos, int nMaxPos, WINBOOL bRedraw)
{
  SCROLLINFO si;
  DWORD Action;
  
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_RANGE;
  si.nMin = nMinPos;
  si.nMax = nMaxPos;
  
  NtUserSetScrollInfo(hWnd, nBar, &si, &Action);
  /* FIXME - check if called successfully */
  
  return TRUE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
ShowScrollBar (HWND hWnd, int wBar, WINBOOL bShow)
{
  return (WINBOOL)NtUserShowScrollBar (hWnd, wBar, bShow);
}
