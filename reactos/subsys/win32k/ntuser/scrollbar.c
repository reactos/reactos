/* $Id: scrollbar.c,v 1.4 2003/01/24 22:42:15 jfilby Exp $
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

//#define NDEBUG
#include <debug.h>

#define SCROLL_MIN_RECT  4               /* Minimum size of the rectangle between the arrows */
#define SCROLL_ARROW_THUMB_OVERLAP 0     /* Overlap between arrows and thumb */

/* FUNCTIONS *****************************************************************/

/* Ported from WINE20020904 */
/* Compute the scroll bar rectangle, in drawing coordinates (i.e. client coords for SB_CTL, window coords for SB_VERT and
 * SB_HORZ). 'arrowSize' returns the width or height of an arrow (depending on * the orientation of the scrollbar),
 * 'thumbSize' returns the size of the thumb, and 'thumbPos' returns the position of the thumb relative to the left or to
 * the top. Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
static BOOL
SCROLL_GetScrollBarRect (PWINDOW_OBJECT Window, INT nBar, PRECT lprect)
{
  SCROLLBARINFO info;
  INT pixels, thumbSize, arrowSize;
  BOOL vertical;
  RECT ClientRect = Window->ClientRect;
  RECT WindowRect = Window->WindowRect;
  ULONG Style;

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
      lprect->left = ClientRect.right - WindowRect.left;
      lprect->top = ClientRect.top - WindowRect.top;
      lprect->right = lprect->left + NtUserGetSystemMetrics (SM_CXVSCROLL);
      lprect->bottom = ClientRect.bottom - WindowRect.top;
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
      W32kGetClientRect (Window, lprect);
      vertical = ((Window->Style & SBS_VERT) != 0);
      break;

    default:
      W32kReleaseWindowObject(Window);
      return FALSE;
    }

  if (vertical)
    pixels = lprect->bottom - lprect->top;
  else
    pixels = lprect->right - lprect->left;

  info.cbSize = sizeof(SCROLLBARINFO);
  SCROLL_GetScrollBarInfo (Window, nBar, &info);

  if (pixels <= 2 * NtUserGetSystemMetrics (SM_CXVSCROLL) + SCROLL_MIN_RECT)
    {
      info.dxyLineButton = info.xyThumbTop = info.xyThumbBottom = 0;
    }
  else
    {
      arrowSize = NtUserGetSystemMetrics (SM_CXVSCROLL);
      pixels -= (2 * (NtUserGetSystemMetrics (SM_CXVSCROLL) - SCROLL_ARROW_THUMB_OVERLAP));

      /* Temporary initialization - to be removed once proper code is in */
      info.dxyLineButton = info.xyThumbTop = info.xyThumbBottom = 0;

/*        if (info->Page)
        {
	    thumbSize = MulDiv(pixels,info->Page,(info->MaxVal-info->MinVal+1));
            if (*thumbSize < SCROLL_MIN_THUMB) *thumbSize = SCROLL_MIN_THUMB;
        }
        else *thumbSize = NtUserGetSystemMetrics(SM_CXVSCROLL); */
/*
        if (((pixels -= *thumbSize ) < 0) ||
            ((info->flags & ESB_DISABLE_BOTH) == ESB_DISABLE_BOTH))
        { */
      /* Rectangle too small or scrollbar disabled -> no thumb */
/*            *thumbPos = *thumbSize = 0;
        }
        else
        { */
/*            INT max = info->MaxVal - max( info->Page-1, 0 );
            if (info->MinVal >= max)
                *thumbPos = *arrowSize - SCROLL_ARROW_THUMB_OVERLAP;
            else
                *thumbPos = *arrowSize - SCROLL_ARROW_THUMB_OVERLAP
		  + MulDiv(pixels, (info->CurVal-info->MinVal),(max - info->MinVal));
        } */
  }

  return vertical;
}

DWORD SCROLL_CreateScrollBar(PWINDOW_OBJECT Window, LONG idObject)
{
  PSCROLLBARINFO psbi;
  LRESULT Result;
  INT i;

  Result = WinPosGetNonClientSize(Window->Self,
				  &Window->WindowRect,
				  &Window->ClientRect);

  psbi = ExAllocatePool(NonPagedPool, sizeof(SCROLLBARINFO));

  for (i=0; i<CCHILDREN_SCROLLBAR+1; i++)
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
      return FALSE;
  }

  SCROLL_GetScrollBarRect (Window, idObject, &(psbi->rcScrollBar));

  return 0;
}

DWORD SCROLL_GetScrollBarInfo(PWINDOW_OBJECT Window, LONG idObject, PSCROLLBARINFO psbi)
{
  switch(idObject)
  {
    case SB_HORZ:
      memcpy(psbi, Window->pHScroll, psbi->cbSize);
      break;
    case SB_VERT:
      memcpy(psbi, Window->pVScroll, psbi->cbSize);
      break;
    case SB_CTL:
      memcpy(psbi, Window->wExtra, psbi->cbSize);
      break;
    default:
      W32kReleaseWindowObject(Window);
      return FALSE;
  }

  return TRUE;
}

DWORD
STDCALL
NtUserGetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
  PWINDOW_OBJECT Window = W32kGetWindowObject(hWnd);

  if (!Window) return FALSE;

  SCROLL_GetScrollBarInfo(Window, idObject, psbi);

  W32kReleaseWindowObject(Window);

  return TRUE;
}

DWORD
STDCALL
NtUserEnableScrollBar(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  return 0;
}

DWORD
STDCALL
NtUserScrollDC(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)

{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetScrollInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

/* Ported from WINE20020904 (SCROLL_ShowScrollBar) */
DWORD
STDCALL
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow)
{
  BOOL fShowV = (wBar == SB_VERT) ? 0 : bShow;
  BOOL fShowH = (wBar == SB_HORZ) ? 0 : bShow;
  PWINDOW_OBJECT Window = W32kGetWindowObject(hWnd);

  switch (wBar)
    {
    case SB_CTL:
      NtUserShowWindow (hWnd, fShowH ? SW_SHOW : SW_HIDE);
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
    NtUserSetWindowPos (hWnd, 0, 0, 0, 0, 0,
                        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    return TRUE;
  }
  return FALSE;			/* no frame changes */
}

/* EOF */
