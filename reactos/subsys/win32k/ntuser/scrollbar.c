/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: scrollbar.c,v 1.13 2003/09/08 18:50:00 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Scrollbars
 * FILE:             subsys/win32k/ntuser/scrollbar.c
 * PROGRAMER:        Jason Filby (jasonfilby@yahoo.com)
 * REVISION HISTORY:
 *       16-11-2002  Jason Filby  Created
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/winpos.h>
#include <include/rect.h>
#include <include/scroll.h>

//#define NDEBUG
#include <debug.h>

#define SCROLL_MIN_RECT  4               /* Minimum size of the rectangle between the arrows */
#define SCROLL_ARROW_THUMB_OVERLAP 0     /* Overlap between arrows and thumb */

#define SBRG_SCROLLBAR     0 /* the scrollbar itself */
#define SBRG_TOPRIGHTBTN   1 /* the top or right button */
#define SBRG_PAGEUPRIGHT   2 /* the page up or page right region */
#define SBRG_SCROLLBOX     3 /* the scroll box */
#define SBRG_PAGEDOWNLEFT  4 /* the page down or page left region */
#define SBRG_BOTTOMLEFTBTN 5 /* the bottom or left button */

/***********************************************************************
 *           MulDiv  (copied from kernel32)
 */
static INT IntMulDiv(
  INT nMultiplicand,
  INT nMultiplier,
  INT nDivisor)
{
#if SIZEOF_LONG_LONG >= 8
    long long ret;

    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
         ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      ret = (((long long)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
      ret = (((long long)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
#else
    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
         ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      return ((nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;

    return ((nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

#endif
}


/* FUNCTIONS *****************************************************************/

/* Ported from WINE20020904 */
/* Compute the scroll bar rectangle, in drawing coordinates (i.e. client coords for SB_CTL, window coords for SB_VERT and
 * SB_HORZ). 'arrowSize' returns the width or height of an arrow (depending on * the orientation of the scrollbar),
 * 'thumbSize' returns the size of the thumb, and 'thumbPos' returns the position of the thumb relative to the left or to
 * the top. Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
BOOL STDCALL
IntGetScrollBarRect (PWINDOW_OBJECT Window, INT nBar, PRECT lprect)
{
  BOOL vertical;
  RECT ClientRect = Window->ClientRect;
  RECT WindowRect = Window->WindowRect;

  switch (nBar)
    {
    case SB_HORZ:
      lprect->left = ClientRect.left - WindowRect.left;
      lprect->top = ClientRect.bottom - WindowRect.top;
      lprect->right = ClientRect.right - WindowRect.left;
      lprect->bottom = lprect->top + NtUserGetSystemMetrics (SM_CYHSCROLL);
      if (Window->Style & WS_BORDER)
	{
	  lprect->left--;
	  lprect->right++;
	}
      else if (Window->Style & WS_VSCROLL)
	lprect->right++;
      vertical = FALSE;
      break;

    case SB_VERT:
      lprect->left = (ClientRect.right - WindowRect.left);
      lprect->top = (ClientRect.top - WindowRect.top) + 1;
      lprect->right = (lprect->left + NtUserGetSystemMetrics (SM_CXVSCROLL));
      lprect->bottom = (ClientRect.bottom - WindowRect.top) - 1;
      if (Window->Style & WS_BORDER)
	{
	  lprect->top--;
	  lprect->bottom++;
	}
      else if (Window->Style & WS_HSCROLL)
	lprect->bottom++;
      vertical = TRUE;
      break;

    case SB_CTL:
      IntGetClientRect (Window, lprect);
      vertical = ((Window->Style & SBS_VERT) != 0);
      break;

    default:
      IntReleaseWindowObject(Window);
      return FALSE;
    }

  return vertical;
}

#define MINTRACKTHUMB (8)
BOOL FASTCALL
IntCalculateThumb(PWINDOW_OBJECT Window, LONG idObject, PSCROLLBARINFO psbi, LPSCROLLINFO psi)
{
  INT xThumb, yThumb, ThumbBox, cxy;
  switch(idObject)
  {
    case SB_HORZ:
      xThumb = NtUserGetSystemMetrics(SM_CXHSCROLL);
      cxy = psbi->rcScrollBar.right - psbi->rcScrollBar.left;
      if(cxy < (2 * xThumb))
      {
        xThumb = cxy / 2;
        psbi->xyThumbTop = 0;
        psbi->xyThumbBottom = 0;
      }
      else
      {
        ThumbBox = psi->nPage ? MINTRACKTHUMB : NtUserGetSystemMetrics(SM_CXHTHUMB);
        cxy -= (2 * xThumb);
        if(cxy >= ThumbBox)
        {
          if(psi->nPage)
          {
            ThumbBox = max(IntMulDiv(cxy, psi->nPage, psi->nMax - psi->nMin + 1), ThumbBox);
          }
          psbi->xyThumbTop = xThumb;
          psbi->xyThumbBottom = xThumb + ThumbBox;
        }
        else
        {
          psbi->xyThumbTop = 0;
          psbi->xyThumbBottom = 0;
        }
      }
      psbi->dxyLineButton = xThumb;
      return TRUE;
    case SB_VERT:
      yThumb = NtUserGetSystemMetrics(SM_CYVSCROLL);
      cxy = psbi->rcScrollBar.bottom - psbi->rcScrollBar.top;
      if(cxy < (2 * yThumb))
      {
        yThumb = cxy / 2;
        psbi->xyThumbTop = 0;
        psbi->xyThumbBottom = 0;
      }
      else
      {
        ThumbBox = psi->nPage ? MINTRACKTHUMB : NtUserGetSystemMetrics(SM_CYVTHUMB);
        cxy -= (2 * yThumb);
        if(cxy >= ThumbBox)
        {
          if(psi->nPage)
          {
            ThumbBox = max(IntMulDiv(cxy, psi->nPage, psi->nMax - psi->nMin + 1), ThumbBox);
          }
          psbi->xyThumbTop = yThumb;
          psbi->xyThumbBottom = yThumb + ThumbBox;
        }
        else
        {
          psbi->xyThumbTop = 0;
          psbi->xyThumbBottom = 0;
        }
      }
      psbi->dxyLineButton = yThumb;
      return TRUE;
    case SB_CTL:
      /* FIXME */
      return FALSE;
    default:
      return FALSE;
  }
  return FALSE;
}

DWORD FASTCALL 
IntCreateScrollBar(PWINDOW_OBJECT Window, LONG idObject)
{
  PSCROLLBARINFO psbi;
  LPSCROLLINFO psi;
  LRESULT Result;
  INT i;

  psbi = ExAllocatePool(PagedPool, sizeof(SCROLLBARINFO) + sizeof(SCROLLINFO));
  if(!psbi)
    return FALSE;
    
  psi = (LPSCROLLINFO)((PSCROLLBARINFO)(psbi + 1));
  
  psi->cbSize = sizeof(LPSCROLLINFO);
  psi->nMin = 0;
  psi->nMax = 100;
  psi->nPage = 0;
  psi->nPos = 0;
  psi->nTrackPos = 0;

  Result = WinPosGetNonClientSize(Window->Self,
				  &Window->WindowRect,
				  &Window->ClientRect);

  psbi->cbSize = sizeof(SCROLLBARINFO);

  for (i = 0; i < CCHILDREN_SCROLLBAR + 1; i++)
    psbi->rgstate[i] = 0;

  switch(idObject)
  {
    case SB_HORZ:
      //psbi->dxyLineButton = NtUserGetSystemMetrics(SM_CXHSCROLL);
      Window->pHScroll = psbi;
      break;
    case SB_VERT:
      //psbi->dxyLineButton = NtUserGetSystemMetrics(SM_CYVSCROLL);
      Window->pVScroll = psbi;
      break;
    case SB_CTL:
      /* FIXME - set psbi->dxyLineButton */
      Window->wExtra = psbi;
      break;
    default:
      ExFreePool(psbi);
      return FALSE;
  }

  IntGetScrollBarRect(Window, idObject, &(psbi->rcScrollBar));
  IntCalculateThumb(Window, idObject, psbi, psi);

  return 0;
}

BOOL FASTCALL 
IntDestroyScrollBar(PWINDOW_OBJECT Window, LONG idObject)
{
  switch(idObject)
  {
    case SB_HORZ:
      if(Window->pHScroll)
      {
        ExFreePool(Window->pHScroll);
        Window->pHScroll = NULL;
        return TRUE;
      }
      return FALSE;
    case SB_VERT:
      if(Window->pVScroll)
      {
        ExFreePool(Window->pVScroll);
        Window->pVScroll = NULL;
        return TRUE;
      }
      return FALSE;
    case SB_CTL:
      if(Window->wExtra)
      {
        ExFreePool(Window->wExtra);
        Window->wExtra = NULL;
        return TRUE;
      }
      return FALSE;
  }
  return FALSE;
}

#define CHANGERGSTATE(item, status) \
  if(Info->rgstate[(item)] != (status)) \
    Chg = TRUE; \
  Info->rgstate[(item)] = (status); 


BOOL STDCALL
IntEnableScrollBar(BOOL Horz, PSCROLLBARINFO Info, UINT wArrows)
{
  BOOL Chg = FALSE;
  switch(wArrows)
  {
    case ESB_DISABLE_BOTH:
      CHANGERGSTATE(SBRG_TOPRIGHTBTN, STATE_SYSTEM_UNAVAILABLE);
      CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, STATE_SYSTEM_UNAVAILABLE);
      break;
    case ESB_DISABLE_RTDN:
      if(Horz)
      {
        CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, STATE_SYSTEM_UNAVAILABLE);
      }
      else
      {
        CHANGERGSTATE(SBRG_TOPRIGHTBTN, STATE_SYSTEM_UNAVAILABLE);
      }
      break;
    case ESB_DISABLE_LTUP:
      if(Horz)
      {
        CHANGERGSTATE(SBRG_TOPRIGHTBTN, STATE_SYSTEM_UNAVAILABLE);
      }
      else
      {
        CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, STATE_SYSTEM_UNAVAILABLE);
      }
      break;
    case ESB_ENABLE_BOTH:
      CHANGERGSTATE(SBRG_TOPRIGHTBTN, 0);
      CHANGERGSTATE(SBRG_BOTTOMLEFTBTN, 0);
      break;
  }
  return Chg;
}


BOOL
STDCALL
NtUserGetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
  PWINDOW_OBJECT Window;
  PSCROLLBARINFO sbi;
  LPSCROLLINFO psi;
  INT Bar;
  
  if(!psbi || (psbi->cbSize != sizeof(SCROLLBARINFO)))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Window = IntGetWindowObject(hWnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }

  switch(idObject)
  {
    case OBJID_HSCROLL:
      if(Window->pHScroll)
      {
        Bar = SB_HORZ;
        sbi = Window->pHScroll;
        break;
      }
      /* fall through */
    case OBJID_VSCROLL:
      if(Window->pVScroll)
      {
        Bar = SB_VERT;
        sbi = Window->pVScroll;
        break;
      }
      /* fall through */
    case OBJID_CLIENT:
      if(Window->wExtra)
      {
        Bar = SB_CTL;
        sbi = Window->wExtra;
        break;
      }
      /* fall through */
    default:
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
  }
  
  psi = (LPSCROLLINFO)((PSCROLLBARINFO)(sbi + 1));

  IntGetScrollBarRect(Window, Bar, &(sbi->rcScrollBar));
  IntCalculateThumb(Window, Bar, sbi, psi);
  
  memcpy(psbi, sbi, sizeof(SCROLLBARINFO));

  IntReleaseWindowObject(Window);
  return TRUE;
}


BOOL
STDCALL
NtUserGetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
  PWINDOW_OBJECT Window;
  PSCROLLBARINFO Info = NULL;
  
  Window = IntGetWindowObject(hwnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  switch(fnBar)
  {
    case SB_HORZ:
      Info = Window->pHScroll;
      break;
    case SB_VERT:
      Info = Window->pVScroll;
      break;
    case SB_CTL:
      Info = Window->wExtra;
      break;
    default:
      IntReleaseWindowObject(Window);
      return FALSE;
  }
  
  IntReleaseWindowObject(Window);
  return FALSE;
}


BOOL
STDCALL
NtUserEnableScrollBar(
  HWND hWnd, 
  UINT wSBflags, 
  UINT wArrows)
{
  PWINDOW_OBJECT Window;
  PSCROLLBARINFO InfoV = NULL, InfoH = NULL;
  BOOL Chg = FALSE;
  
  Window = IntGetWindowObject(hWnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  switch(wSBflags)
  {
    case SB_BOTH:
      InfoV = Window->pVScroll;
      /* fall through */
    case SB_HORZ:
      InfoH = Window->pHScroll;
      break;
    case SB_VERT:
      InfoV = Window->pVScroll;
      break;
    case SB_CTL:
      InfoV = Window->wExtra;
      break;
    default:
      IntReleaseWindowObject(Window);
      return FALSE;
  }
  
  if(InfoV)
    Chg = IntEnableScrollBar(FALSE, InfoV, wArrows);
    
  if(InfoH)
    Chg = (IntEnableScrollBar(TRUE, InfoH, wArrows) || Chg);

  //if(Chg && (Window->Style & WS_VISIBLE))
    /* FIXME - repaint scrollbars */

  IntReleaseWindowObject(Window);
  return TRUE;
}

DWORD
STDCALL
NtUserScrollDC(
  HDC hDC,
  int dx,
  int dy,
  CONST RECT *lprcScroll,
  CONST RECT *lprcClip ,
  HRGN hrgnUpdate,
  LPRECT lprcUpdate)

{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetScrollInfo(
  HWND hwnd, 
  int fnBar, 
  LPCSCROLLINFO lpsi, 
  WINBOOL fRedraw)
{
  PWINDOW_OBJECT Window;
  PSCROLLBARINFO Info = NULL;
  LPSCROLLINFO psi;
  BOOL Chg = FALSE;
  UINT Mask;
  DWORD Ret;
  
  if(!lpsi || ((lpsi->cbSize != sizeof(SCROLLINFO)) && 
               (lpsi->cbSize != sizeof(SCROLLINFO) - sizeof(lpsi->nTrackPos))))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  Window = IntGetWindowObject(hwnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  
  switch(fnBar)
  {
    case SB_HORZ:
      Info = Window->pHScroll;
      if(Info)
        break;
      /* fall through */
    case SB_VERT:
      Info = Window->pVScroll;
      if(Info)
        break;
      /* fall through */
    case SB_CTL:
      Info = Window->wExtra;
      if(Info)
        break;
      /* fall through */
    default:
      IntReleaseWindowObject(Window);
      return 0;
  }
  
  psi = (LPSCROLLINFO)((PSCROLLBARINFO)(Info + 1));
  
  if(lpsi->fMask == SIF_ALL)
    Mask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
  else
    Mask = lpsi->fMask;
  
  if(Mask & SIF_DISABLENOSCROLL)
  {
    /* FIXME */
  }

  if((Mask & SIF_PAGE) && (psi->nPage != lpsi->nPage))
  {
    psi->nPage = lpsi->nPage;
    Chg = TRUE;
  }
  
  if((Mask & SIF_POS) && (psi->nPos != lpsi->nPos))
  {
    psi->nPos = lpsi->nPos;
    Chg = TRUE;
  }
  
  if((Mask & SIF_RANGE) && ((psi->nMin != lpsi->nMin) || (psi->nMax != lpsi->nMax)))
  {
    psi->nMin = lpsi->nMin;
    psi->nMax = lpsi->nMax;
    Chg = TRUE;
  }
  
  /* FIXME check assigned values */
  
  if(fRedraw && Chg && (Window->Style & WS_VISIBLE))
  {
    /* FIXME - Redraw */
  }
  
  Ret = psi->nPos;
  
  IntReleaseWindowObject(Window);
  return Ret;
}

/* Ported from WINE20020904 (SCROLL_ShowScrollBar) */
DWORD
STDCALL
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow)
{
  BOOL fShowV = (wBar == SB_VERT) ? 0 : bShow;
  BOOL fShowH = (wBar == SB_HORZ) ? 0 : bShow;
  PWINDOW_OBJECT Window = IntGetWindowObject(hWnd);
  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }

  switch (wBar)
    {
    case SB_CTL:
      WinPosShowWindow (hWnd, fShowH ? SW_SHOW : SW_HIDE);
      return TRUE;

    case SB_BOTH:
    case SB_HORZ:
      if (fShowH)
	{
	  fShowH = !(Window->Style & WS_HSCROLL);
	  Window->Style |= WS_HSCROLL;
	}
      else			/* hide it */
	{
	  fShowH = (Window->Style & WS_HSCROLL);
	  Window->Style &= ~WS_HSCROLL;
	}
      if (wBar == SB_HORZ)
	{
	  fShowV = FALSE;
	  break;
	}
      /* fall through */

    case SB_VERT:
      if (fShowV)
	{
	  fShowV = !(Window->Style & WS_VSCROLL);
	  Window->Style |= WS_VSCROLL;
	}
      else			/* hide it */
	{
	  fShowV = (Window->Style & WS_VSCROLL);
	  Window->Style &= ~WS_VSCROLL;
	}
      if (wBar == SB_VERT)
	fShowH = FALSE;
      break;

    default:
      return FALSE;		/* Nothing to do! */
    }

  if (fShowH || fShowV)		/* frame has been changed, let the window redraw itself */
  {
    WinPosSetWindowPos (hWnd, 0, 0, 0, 0, 0,
                        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    return TRUE;
  }
  return FALSE;			/* no frame changes */
}

/* EOF */
