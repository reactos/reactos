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
/* $Id: scrollbar.c,v 1.34.4.2 2004/09/14 01:00:44 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Scrollbars
 * FILE:             subsys/win32k/ntuser/scrollbar.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                   Jason Filby (jasonfilby@yahoo.com)
 * REVISION HISTORY:
 *       16-11-2002  Jason Filby  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#ifdef __USE_W32API
#include <oleacc.h>
#endif

#define NDEBUG
#include <debug.h>

#define MINTRACKTHUMB    8               /* Minimum size of the rectangle between the arrows */

#define SBRG_SCROLLBAR     0 /* the scrollbar itself */
#define SBRG_TOPRIGHTBTN   1 /* the top or right button */
#define SBRG_PAGEUPRIGHT   2 /* the page up or page right region */
#define SBRG_SCROLLBOX     3 /* the scroll box */
#define SBRG_PAGEDOWNLEFT  4 /* the page down or page left region */
#define SBRG_BOTTOMLEFTBTN 5 /* the bottom or left button */

#define CHANGERGSTATE(item, status) \
  if(Info->rgstate[(item)] != (status)) \
    Chg = TRUE; \
  Info->rgstate[(item)] = (status); 

/* FUNCTIONS *****************************************************************/

/* Ported from WINE20020904 */
/* Compute the scroll bar rectangle, in drawing coordinates (i.e. client coords for SB_CTL, window coords for SB_VERT and
 * SB_HORZ). 'arrowSize' returns the width or height of an arrow (depending on * the orientation of the scrollbar),
 * 'thumbSize' returns the size of the thumb, and 'thumbPos' returns the position of the thumb relative to the left or to
 * the top. Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
BOOL INTERNAL_CALL
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
      lprect->bottom = lprect->top + IntGetSystemMetrics (SM_CYHSCROLL);
      vertical = FALSE;
      break;

    case SB_VERT:
      if(Window->ExStyle & WS_EX_LEFTSCROLLBAR)
      {
        lprect->right = ClientRect.left - WindowRect.left;
        lprect->left = lprect->right - IntGetSystemMetrics(SM_CXVSCROLL);
      }
      else
      {
        lprect->left = ClientRect.right - WindowRect.left;
        lprect->right = lprect->left + IntGetSystemMetrics(SM_CXVSCROLL);
      }
      lprect->top = ClientRect.top - WindowRect.top;
      lprect->bottom = ClientRect.bottom - WindowRect.top;
      vertical = TRUE;
      break;

    case SB_CTL:
      IntGetClientRect (Window, lprect);
      vertical = ((Window->Style & SBS_VERT) != 0);
      break;

    default:
      return FALSE;
    }

  return vertical;
}

static BOOL INTERNAL_CALL
IntCalculateThumb(PWINDOW_OBJECT Window, LONG idObject, PSCROLLBARINFO psbi, LPSCROLLINFO psi)
{
  INT Thumb, ThumbBox, ThumbPos, cxy, mx;
  RECT ClientRect;
  
  switch(idObject)
  {
    case SB_HORZ:
      Thumb = IntGetSystemMetrics(SM_CXHSCROLL);
      cxy = psbi->rcScrollBar.right - psbi->rcScrollBar.left;
      break;
    case SB_VERT:
      Thumb = IntGetSystemMetrics(SM_CYVSCROLL);
      cxy = psbi->rcScrollBar.bottom - psbi->rcScrollBar.top;
      break;
    case SB_CTL:
      IntGetClientRect (Window, &ClientRect);
      if(Window->Style & SBS_VERT)
      {
        Thumb = IntGetSystemMetrics(SM_CYVSCROLL);
        cxy = ClientRect.bottom - ClientRect.top;
      }
      else
      {
        Thumb = IntGetSystemMetrics(SM_CXHSCROLL);
        cxy = ClientRect.right - ClientRect.left;
      }
      break;
    default:
      return FALSE;
  }

  ThumbPos = Thumb;
  /* calculate Thumb */
  if(cxy <= (2 * Thumb))
  {
    Thumb = cxy / 2;
    psbi->xyThumbTop = 0;
    psbi->xyThumbBottom = 0;
    ThumbPos = Thumb;
  }
  else
  {
    ThumbBox = psi->nPage ? MINTRACKTHUMB : IntGetSystemMetrics(SM_CXHTHUMB);
    cxy -= (2 * Thumb);
    if(cxy >= ThumbBox)
    {
      if(psi->nPage)
      {
        ThumbBox = max(EngMulDiv(cxy, psi->nPage, psi->nMax - psi->nMin + 1), ThumbBox);
      }
      
      if(cxy > ThumbBox)
      {
        mx = psi->nMax - max(psi->nPage - 1, 0);
        if(psi->nMin < mx)
          ThumbPos = Thumb + EngMulDiv(cxy - ThumbBox, psi->nPos - psi->nMin, mx - psi->nMin);
        else
          ThumbPos = Thumb + ThumbBox;
      }
      
      psbi->xyThumbTop = ThumbPos;
      psbi->xyThumbBottom = ThumbPos + ThumbBox;
    }
    else
    {
      psbi->xyThumbTop = 0;
      psbi->xyThumbBottom = 0;
    }
  }
  psbi->dxyLineButton = Thumb;

  return TRUE;
}

static VOID INTERNAL_CALL
IntUpdateSBInfo(PWINDOW_OBJECT Window, int wBar)
{
  PSCROLLBARINFO sbi;
  LPSCROLLINFO psi;
  
  ASSERT(Window);
  ASSERT(Window->Scroll);
  
  sbi = IntGetScrollbarInfoFromWindow(Window, wBar);
  psi = IntGetScrollInfoFromWindow(Window, wBar);
  IntGetScrollBarRect(Window, wBar, &(sbi->rcScrollBar));
  IntCalculateThumb(Window, wBar, sbi, psi);
}

BOOL INTERNAL_CALL
IntGetScrollInfo(PWINDOW_OBJECT Window, INT nBar, LPSCROLLINFO lpsi)
{
  UINT Mask;
  LPSCROLLINFO psi;
  
  if(!SBID_IS_VALID(nBar))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    DPRINT1("Trying to get scrollinfo for unknown scrollbar type %d\n", nBar);
    return FALSE;
  }

  if(!IntCreateScrollBars(Window))
    {
      return FALSE;
    }
  
  psi = IntGetScrollInfoFromWindow(Window, nBar);
  
  if (lpsi->fMask == SIF_ALL)
    {
      Mask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
    }
  else
    {
      Mask = lpsi->fMask;
    }

  if (0 != (Mask & SIF_PAGE))
    {
      lpsi->nPage = psi->nPage;
    }
  
  if (0 != (Mask & SIF_POS))
    {
      lpsi->nPos = psi->nPos;
    }
  
  if (0 != (Mask & SIF_RANGE))
    {
      lpsi->nMin = psi->nMin;
      lpsi->nMax = psi->nMax;
    }

  if (0 != (Mask & SIF_TRACKPOS))
    {
      lpsi->nTrackPos = psi->nTrackPos;
    }
  
  return TRUE;
}

DWORD INTERNAL_CALL
IntSetScrollInfo(PWINDOW_OBJECT Window, INT nBar, LPCSCROLLINFO lpsi, BOOL bRedraw)
{
  /*
   * Update the scrollbar state and set action flags according to
   * what has to be done graphics wise.
   */

  LPSCROLLINFO Info;
  PSCROLLBARINFO psbi;
/*   UINT new_flags;*/
  BOOL bChangeParams = FALSE; /* don't show/hide scrollbar if params don't change */

  if(!SBID_IS_VALID(nBar))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    DPRINT1("Trying to set scrollinfo for unknown scrollbar type %d", nBar);
    return FALSE;
  }

  if(!IntCreateScrollBars(Window))
    {
      return FALSE;
    }
  
  if (lpsi->cbSize != sizeof(SCROLLINFO) && 
      lpsi->cbSize != (sizeof(SCROLLINFO) - sizeof(lpsi->nTrackPos)))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
    }
  if (lpsi->fMask & ~(SIF_ALL | SIF_DISABLENOSCROLL))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
    }
  
  psbi = IntGetScrollbarInfoFromWindow(Window, nBar);
  Info = IntGetScrollInfoFromWindow(Window, nBar);

  /* Set the page size */
  if (0 != (lpsi->fMask & SIF_PAGE))
    {
      if (Info->nPage != lpsi->nPage)
        {
          Info->nPage = lpsi->nPage;
          bChangeParams = TRUE;
        }
    }

  /* Set the scroll pos */
  if (0 != (lpsi->fMask & SIF_POS))
    {
      if (Info->nPos != lpsi->nPos)
        {
          Info->nPos = lpsi->nPos;
        }
    }

  /* Set the scroll range */
  if (0 != (lpsi->fMask & SIF_RANGE))
    {
      /* Invalid range -> range is set to (0,0) */
      if (lpsi->nMin > lpsi->nMax ||
          0x80000000 <= (UINT)(lpsi->nMax - lpsi->nMin))
        {
          Info->nMin = 0;
          Info->nMax = 0;
          bChangeParams = TRUE;
        }
      else if (Info->nMin != lpsi->nMin || Info->nMax != lpsi->nMax)
        {
          Info->nMin = lpsi->nMin;
          Info->nMax = lpsi->nMax;
          bChangeParams = TRUE;
        }
    }

  /* Make sure the page size is valid */
  if (Info->nPage < 0)
    {
      Info->nPage = 0;
    }
  else if (Info->nMax - Info->nMin + 1 < Info->nPage)
    {
      Info->nPage = Info->nMax - Info->nMin + 1;
    }

  /* Make sure the pos is inside the range */
  if (Info->nPos < Info->nMin)
    {
      Info->nPos = Info->nMin;
    }
  else if (Info->nPos > Info->nMax - max(Info->nPage - 1, 0))
    {
      Info->nPos = Info->nMax - max(Info->nPage - 1, 0);
    }

  /*
   * Don't change the scrollbar state if SetScrollInfo is just called
   * with SIF_DISABLENOSCROLL
   */
  if (0 == (lpsi->fMask & SIF_ALL))
    {
      return Info->nPos;
    }

  /* Check if the scrollbar should be hidden or disabled */
  if (0 != (lpsi->fMask & (SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL)))
    {
      if (Info->nMin >= Info->nMax - max(Info->nPage - 1, 0))
        {
          /* Hide or disable scroll-bar */
          if (0 != (lpsi->fMask & SIF_DISABLENOSCROLL))
            {
/*            new_flags = ESB_DISABLE_BOTH;*/
            }
          else if ((nBar != SB_CTL) && bChangeParams)
            {
	      IntShowScrollBar(Window, nBar, FALSE);
              return Info->nPos;
            }
        }
      else  /* Show and enable scroll-bar */
        {
/*         new_flags = 0;*/
          if ((nBar != SB_CTL) && bChangeParams)
            {
	      IntShowScrollBar(Window, nBar, TRUE);
            }
        }

#if 0
      if (infoPtr->flags != new_flags) /* check arrow flags */
        {
          infoPtr->flags = new_flags;
          *Action |= SA_SSI_REPAINT_ARROWS;
        }
#endif
    }

  if (bRedraw)
    {
      RECT UpdateRect = psbi->rcScrollBar;
      UpdateRect.left -= Window->ClientRect.left - Window->WindowRect.left;
      UpdateRect.right -= Window->ClientRect.left - Window->WindowRect.left;
      UpdateRect.top -= Window->ClientRect.top - Window->WindowRect.top;
      UpdateRect.bottom -= Window->ClientRect.top - Window->WindowRect.top;
      IntRedrawWindow(Window, &UpdateRect, 0, RDW_INVALIDATE | RDW_FRAME);
    }

   /* Return current position */
   return Info->nPos;
}

BOOL INTERNAL_CALL
IntGetScrollBarInfo(PWINDOW_OBJECT Window, LONG idObject, PSCROLLBARINFO psbi)
{
  INT Bar;
  PSCROLLBARINFO sbi;
  LPSCROLLINFO psi;
  
  Bar = SBOBJ_TO_SBID(idObject);
  
  if(!SBID_IS_VALID(Bar))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    DPRINT1("Trying to get scrollinfo for unknown scrollbar type %d\n", Bar);
    return FALSE;
  }
  
  if(!IntCreateScrollBars(Window))
    {
      return FALSE;
    }
  
  sbi = IntGetScrollbarInfoFromWindow(Window, Bar);
  psi = IntGetScrollInfoFromWindow(Window, Bar);

  IntGetScrollBarRect(Window, Bar, &(sbi->rcScrollBar));
  IntCalculateThumb(Window, Bar, sbi, psi);
  
  RtlCopyMemory(psbi, sbi, sizeof(SCROLLBARINFO));
  
  return TRUE;
}

BOOL INTERNAL_CALL
IntCreateScrollBars(PWINDOW_OBJECT Window)
{
  PSCROLLBARINFO psbi;
  LPSCROLLINFO psi;
  LRESULT Result;
  ULONG Size, s;
  INT i;
  
  if(Window->Scroll)
  {
    /* no need to create it anymore */
    return TRUE;
  }
  
  /* allocate memory for all scrollbars (HORZ, VERT, CONTROL) */
  Size = 3 * (sizeof(WINDOW_SCROLLINFO));
  if(!(Window->Scroll = ExAllocatePoolWithTag(PagedPool, Size, TAG_SBARINFO)))
  {
    DPRINT1("Unable to allocate memory for scrollbar information for window 0x%x\n", Window->Handle);
    return FALSE;
  }
  
  RtlZeroMemory(Window->Scroll, Size);

  Result = WinPosGetNonClientSize(Window,
				  &Window->WindowRect,
				  &Window->ClientRect);
  
  for(s = SB_HORZ; s <= SB_VERT; s++)
  {
    psbi = IntGetScrollbarInfoFromWindow(Window, s);
    psbi->cbSize = sizeof(SCROLLBARINFO);
    for (i = 0; i < CCHILDREN_SCROLLBAR + 1; i++)
      psbi->rgstate[i] = 0;
    
    psi = IntGetScrollInfoFromWindow(Window, s);
    psi->cbSize = sizeof(LPSCROLLINFO);
    psi->nMax = 100;
    
    IntGetScrollBarRect(Window, s, &(psbi->rcScrollBar));
    IntCalculateThumb(Window, s, psbi, psi);
  }

  return TRUE;
}

BOOL INTERNAL_CALL
IntDestroyScrollBars(PWINDOW_OBJECT Window)
{
  if(Window->Scroll)
  {
    ExFreePool(Window->Scroll);
    Window->Scroll = NULL;
    return TRUE;
  }
  return FALSE;
}

BOOL INTERNAL_CALL
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

BOOL INTERNAL_CALL
IntEnableWindowScrollBar(PWINDOW_OBJECT Window, 
                         UINT wSBflags, 
                         UINT wArrows)
{
  PSCROLLBARINFO InfoV = NULL, InfoH = NULL;
  BOOL Chg = FALSE;
  
  ASSERT(Window);
  
  if(wSBflags == SB_CTL)
  {
    /* FIXME */
    return FALSE;
  }
  
  if(wSBflags != SB_BOTH && !SBID_IS_VALID(wSBflags))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    DPRINT1("Trying to set scrollinfo for unknown scrollbar type %d", wSBflags);
    return FALSE;
  }
  
  if(!IntCreateScrollBars(Window))
    {
      return FALSE;
    }
  
  switch(wSBflags)
  {
    case SB_BOTH:
      InfoV = IntGetScrollbarInfoFromWindow(Window, SB_VERT);
      /* fall through */
    case SB_HORZ:
      InfoH = IntGetScrollbarInfoFromWindow(Window, SB_HORZ);
      break;
    case SB_VERT:
      InfoV = IntGetScrollbarInfoFromWindow(Window, SB_VERT);
      break;
    default:
      return FALSE;
  }
  
  if(InfoV)
    Chg = IntEnableScrollBar(FALSE, InfoV, wArrows);
    
  if(InfoH)
    Chg = (IntEnableScrollBar(TRUE, InfoH, wArrows) || Chg);

  //if(Chg && (Window->Style & WS_VISIBLE))
    /* FIXME - repaint scrollbars */
  
  return TRUE;
}

BOOL INTERNAL_CALL
IntSetScrollBarInfo(PWINDOW_OBJECT Window,
                    LONG idObject,
                    SETSCROLLBARINFO *info)
{
  PSCROLLBARINFO sbi;
  LPSCROLLINFO psi;
  LONG Obj;
  
  ASSERT(Window);
  
  Obj = SBOBJ_TO_SBID(idObject);
  if(!SBID_IS_VALID(Obj))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    DPRINT1("Trying to set scrollinfo for unknown scrollbar type %d", Obj);
    return FALSE;
  }

  if(!IntCreateScrollBars(Window))
    {
      return FALSE;
    }
  
  sbi = IntGetScrollbarInfoFromWindow(Window, Obj);
  psi = IntGetScrollInfoFromWindow(Window, Obj);
  
  psi->nTrackPos = info->nTrackPos;
  sbi->reserved = info->reserved;
  RtlCopyMemory(&sbi->rgstate, &info->rgstate, sizeof(info->rgstate));
  
  return TRUE;
}

/* Ported from WINE20020904 (SCROLL_ShowScrollBar) */
DWORD INTERNAL_CALL
IntShowScrollBar(PWINDOW_OBJECT Window, int wBar, DWORD bShow)
{
   DWORD Style, OldStyle;
   
   ASSERT(Window);
   
   switch(wBar)
   {
     case SB_HORZ:
       Style = WS_HSCROLL;
       break;
     case SB_VERT:
       Style = WS_VSCROLL;
       break;
     case SB_BOTH:
       Style = WS_HSCROLL | WS_VSCROLL;
       break;
     case SB_CTL:
       Style = 0;
       break;
     default:
       SetLastWin32Error(ERROR_INVALID_PARAMETER);
       return FALSE;
   }
   
  if(!IntCreateScrollBars(Window))
    {
      return FALSE;
    }

   if (wBar == SB_CTL)
   {
      IntUpdateSBInfo(Window, SB_CTL);
      
      WinPosShowWindow(Window, bShow ? SW_SHOW : SW_HIDE);
      return TRUE;
   }
   
   OldStyle = Window->Style;
   if(bShow)
     Window->Style |= Style;
   else
     Window->Style &= ~Style;
   
   if(Window->Style != OldStyle)
   {
     if(Window->Style & WS_HSCROLL)
       IntUpdateSBInfo(Window, SB_HORZ);
     if(Window->Style & WS_VSCROLL)
       IntUpdateSBInfo(Window, SB_VERT);
     
     if(Window->Style & WS_VISIBLE)
     {
       /* Frame has been changed, let the window redraw itself */
       WinPosSetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
          SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
     }
   }
   
   return TRUE;
}

