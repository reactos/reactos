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
/* $Id: scrollbar.c,v 1.29 2004/04/09 20:03:19 navaraf Exp $
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

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <internal/safe.h>
#include <include/object.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/winpos.h>
#include <include/rect.h>
#include <include/scroll.h>
#include <include/painting.h>
#include <include/tags.h>
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
      vertical = FALSE;
      break;

    case SB_VERT:
      lprect->left = ClientRect.right - WindowRect.left;
      lprect->top = ClientRect.top - WindowRect.top;
      lprect->right = lprect->left + NtUserGetSystemMetrics(SM_CXVSCROLL);
      lprect->bottom = ClientRect.bottom - WindowRect.top;
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

BOOL FASTCALL
IntCalculateThumb(PWINDOW_OBJECT Window, LONG idObject, PSCROLLBARINFO psbi, LPSCROLLINFO psi)
{
  INT Thumb, ThumbBox, ThumbPos, cxy, mx;
  RECT ClientRect;
  
  switch(idObject)
  {
    case SB_HORZ:
      Thumb = NtUserGetSystemMetrics(SM_CXHSCROLL);
      cxy = psbi->rcScrollBar.right - psbi->rcScrollBar.left;
      break;
    case SB_VERT:
      Thumb = NtUserGetSystemMetrics(SM_CYVSCROLL);
      cxy = psbi->rcScrollBar.bottom - psbi->rcScrollBar.top;
      break;
    case SB_CTL:
      IntGetClientRect (Window, &ClientRect);
      if(Window->Style & SBS_VERT)
      {
        Thumb = NtUserGetSystemMetrics(SM_CYVSCROLL);
        cxy = ClientRect.bottom - ClientRect.top;
      }
      else
      {
        Thumb = NtUserGetSystemMetrics(SM_CXHSCROLL);
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
    ThumbBox = psi->nPage ? MINTRACKTHUMB : NtUserGetSystemMetrics(SM_CXHTHUMB);
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

static BOOL FASTCALL
IntGetScrollInfo(PWINDOW_OBJECT Window, INT nBar, LPSCROLLINFO lpsi)
{
  UINT Mask;
  LPSCROLLINFO psi;
  PSCROLLBARINFO Info;
  
  switch (nBar)
    {
      case SB_HORZ:
        Info = Window->pHScroll;
        break;
      case SB_VERT:
        Info = Window->pVScroll;
        break;
      case SB_CTL:
        if (Window->ExtraDataSize < sizeof(SCROLLBARINFO) + sizeof(SCROLLINFO))
          {
            Info = NULL;
          }
        else
          {
            Info = (PSCROLLBARINFO) Window->ExtraData;
          }
        break;
      default:
        DPRINT1("Trying to get scrollinfo for unknown scrollbar type %d", nBar);
        return FALSE;
    }

  if (NULL == Info)
    {
      DPRINT1("No scrollinfo available\n");
      return FALSE;
    }
  
  psi = (LPSCROLLINFO)(Info + 1);
  
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

static DWORD FASTCALL
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
  switch (nBar)
    {
      case SB_HORZ:
        if (NULL == Window->pHScroll)
          {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
          }
        psbi = Window->pHScroll;
        break;
      case SB_VERT:
        if (NULL == Window->pVScroll)
          {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
          }
        psbi = Window->pVScroll;
        break;
      case SB_CTL:
        if (Window->ExtraDataSize < sizeof(SCROLLBARINFO) + sizeof(SCROLLINFO))
          {
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            return 0;
          }
        psbi = (PSCROLLBARINFO) Window->ExtraData;
        break;
      default:
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

  Info = (LPSCROLLINFO)(psbi + 1);

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
              NtUserShowScrollBar(Window->Self, nBar, FALSE);
              return Info->nPos;
            }
        }
      else  /* Show and enable scroll-bar */
        {
/*         new_flags = 0;*/
          if ((nBar != SB_CTL) && bChangeParams)
            {
              NtUserShowScrollBar(Window->Self, nBar, TRUE);
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

BOOL FASTCALL
IntGetScrollBarInfo(PWINDOW_OBJECT Window, LONG idObject, PSCROLLBARINFO psbi)
{
  INT Bar;
  PSCROLLBARINFO sbi;
  LPSCROLLINFO psi;
  
  switch(idObject)
    {
      case OBJID_HSCROLL:
        Bar = SB_HORZ;
        sbi = Window->pHScroll;
        break;
      case OBJID_VSCROLL:
        Bar = SB_VERT;
        sbi = Window->pVScroll;
        break;
      case OBJID_CLIENT:
        Bar = SB_CTL;
        if (Window->ExtraDataSize < sizeof(SCROLLBARINFO) + sizeof(SCROLLINFO))
          {
            sbi = NULL;
          }
        else
          {
            sbi = (PSCROLLBARINFO) Window->ExtraData;
          }
        break;
      default:
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

  if (NULL == sbi)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  
  psi = (LPSCROLLINFO)(sbi + 1);

  IntGetScrollBarRect(Window, Bar, &(sbi->rcScrollBar));
  IntCalculateThumb(Window, Bar, sbi, psi);
  
  RtlCopyMemory(psbi, sbi, sizeof(SCROLLBARINFO));
  
  return TRUE;
}

DWORD FASTCALL 
IntCreateScrollBar(PWINDOW_OBJECT Window, LONG idObject)
{
  PSCROLLBARINFO psbi;
  LPSCROLLINFO psi;
  LRESULT Result;
  INT i;

  psbi = ExAllocatePoolWithTag(PagedPool, sizeof(SCROLLBARINFO) + sizeof(SCROLLINFO), TAG_SBARINFO);
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
      Window->pHScroll = psbi;
      break;
    case SB_VERT:
      Window->pVScroll = psbi;
      break;
    case SB_CTL:
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
  NTSTATUS Status;
  SCROLLBARINFO sbi;
  PWINDOW_OBJECT Window;
  BOOL Ret;

  Status = MmCopyFromCaller(&sbi, psbi, sizeof(SCROLLBARINFO));
  if(!NT_SUCCESS(Status) || (sbi.cbSize != sizeof(SCROLLBARINFO)))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  Window = IntGetWindowObject(hWnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }

  Ret = IntGetScrollBarInfo(Window, idObject, &sbi);

  Status = MmCopyToCaller(psbi, &sbi, sizeof(SCROLLBARINFO));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    Ret = FALSE;
  }
  IntReleaseWindowObject(Window);
  return Ret;
}


BOOL
STDCALL
NtUserGetScrollInfo(HWND hwnd, int fnBar, LPSCROLLINFO lpsi)
{
  NTSTATUS Status;
  PWINDOW_OBJECT Window;
  SCROLLINFO psi;
  DWORD sz;
  BOOL Ret;
  
  Status = MmCopyFromCaller(&psi.cbSize, &(lpsi->cbSize), sizeof(UINT));
  if(!NT_SUCCESS(Status) || 
     !((psi.cbSize == sizeof(SCROLLINFO)) || (psi.cbSize == sizeof(SCROLLINFO) - sizeof(psi.nTrackPos))))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  sz = psi.cbSize;
  Status = MmCopyFromCaller(&psi, lpsi, sz);
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  Window = IntGetWindowObject(hwnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  Ret = IntGetScrollInfo(Window, fnBar, &psi);
  
  IntReleaseWindowObject(Window);
  
  Status = MmCopyToCaller(lpsi, &psi, sz);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  return Ret;
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

BOOL
STDCALL
NtUserSetScrollBarInfo(
  HWND hwnd,
  LONG idObject,
  SETSCROLLBARINFO *info)
{
  PWINDOW_OBJECT Window;
  SETSCROLLBARINFO Safeinfo;
  PSCROLLBARINFO sbi = NULL;
  LPSCROLLINFO psi;
  NTSTATUS Status;
  
  Window = IntGetWindowObject(hwnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  Status = MmCopyFromCaller(&Safeinfo, info, sizeof(SETSCROLLBARINFO));
  if(!NT_SUCCESS(Status))
  {
    IntReleaseWindowObject(Window);
    SetLastNtError(Status);
    return FALSE;
  }
  
  switch(idObject)
  {
    case OBJID_HSCROLL:
      sbi = Window->pHScroll;
      break;
    case OBJID_VSCROLL:
      sbi = Window->pVScroll;
      break;
    case OBJID_CLIENT:
      /* FIXME */
      IntReleaseWindowObject(Window);
      return FALSE;

    default:
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
  }
  
  if(!sbi)
  {
    DPRINT1("SCROLLBARINFO structure not allocated!\n");
    IntReleaseWindowObject(Window);
    return FALSE;
  }
  
  psi = (LPSCROLLINFO)((PSCROLLBARINFO)(sbi + 1));
  
  psi->nTrackPos = Safeinfo.nTrackPos;
  sbi->reserved = Safeinfo.reserved;
  RtlCopyMemory(&sbi->rgstate, &Safeinfo.rgstate, sizeof(Safeinfo.rgstate));
  
  IntReleaseWindowObject(Window);
  return TRUE;
}

DWORD
STDCALL
NtUserSetScrollInfo(
  HWND hwnd, 
  int fnBar, 
  LPCSCROLLINFO lpsi, 
  BOOL bRedraw)
{
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  SCROLLINFO ScrollInfo;
  DWORD Ret;
  
  Window = IntGetWindowObject(hwnd);

  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  
  Status = MmCopyFromCaller(&ScrollInfo, lpsi, sizeof(SCROLLINFO) - sizeof(ScrollInfo.nTrackPos));
  if(!NT_SUCCESS(Status))
  {
    IntReleaseWindowObject(Window);
    SetLastNtError(Status);
    return 0;
  }
  
  Ret = IntSetScrollInfo(Window, fnBar, &ScrollInfo, bRedraw);
  IntReleaseWindowObject(Window);
  
  return Ret;
}

/* Ported from WINE20020904 (SCROLL_ShowScrollBar) */
DWORD STDCALL
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow)
{
   PWINDOW_OBJECT Window = IntGetWindowObject(hWnd);

   if (!Window)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if (wBar == SB_CTL)
   {
      WinPosShowWindow(hWnd, bShow ? SW_SHOW : SW_HIDE);
      IntReleaseWindowObject(Window);
      return TRUE;
   }

   if (wBar == SB_BOTH || wBar == SB_HORZ)
   {
      if (bShow)
      {
         Window->Style |= WS_HSCROLL;
         if (!Window->pHScroll)
            IntCreateScrollBar(Window, SB_HORZ);
      }
      else
      {
         Window->Style &= ~WS_HSCROLL;
      }
   }

   if (wBar == SB_BOTH || wBar == SB_VERT)
   {
      if (bShow)
      {
         Window->Style |= WS_VSCROLL;
         if (!Window->pVScroll)
            IntCreateScrollBar(Window, SB_VERT);
      }
      else
      {
         Window->Style &= ~WS_VSCROLL;
      }
   }

   /* Frame has been changed, let the window redraw itself */
   WinPosSetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
      SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);

   IntReleaseWindowObject(Window);
   return TRUE;
}

/* EOF */
