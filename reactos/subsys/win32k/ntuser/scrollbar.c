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
/* $Id: scrollbar.c,v 1.7 2003/05/26 10:52:15 rcampbell Exp $
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

/* FUNCTIONS *****************************************************************/

/* Ported from WINE20020904 */
/* Compute the scroll bar rectangle, in drawing coordinates (i.e. client coords for SB_CTL, window coords for SB_VERT and
 * SB_HORZ). 'arrowSize' returns the width or height of an arrow (depending on * the orientation of the scrollbar),
 * 'thumbSize' returns the size of the thumb, and 'thumbPos' returns the position of the thumb relative to the left or to
 * the top. Return TRUE if the scrollbar is vertical, FALSE if horizontal.
 */
static BOOL STDCALL
SCROLL_GetScrollBarRect (PWINDOW_OBJECT Window, INT nBar, PRECT lprect)
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
      W32kGetClientRect (Window, lprect);
      vertical = ((Window->Style & SBS_VERT) != 0);
      break;

    default:
      W32kReleaseWindowObject(Window);
      return FALSE;
    }

  return vertical;
}

DWORD FASTCALL SCROLL_CreateScrollBar(PWINDOW_OBJECT Window, LONG idObject)
{
  PSCROLLBARINFO psbi;
  LRESULT Result;
  INT i;

  Result = WinPosGetNonClientSize(Window->Self,
				  &Window->WindowRect,
				  &Window->ClientRect);

  psbi = ExAllocatePool(PagedPool, sizeof(SCROLLBARINFO));
  psbi->cbSize = sizeof(SCROLLBARINFO);

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

DWORD STDCALL SCROLL_GetScrollBarInfo(PWINDOW_OBJECT Window, LONG idObject, PSCROLLBARINFO psbi)
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

  SCROLL_GetScrollBarRect (Window, idObject, &(psbi->rcScrollBar));

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
