/* $Id: scrollbar.c,v 1.1 2002/11/24 20:15:37 jfilby Exp $
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
SCROLL_GetScrollBarRect (PWINDOW_OBJECT Window, INT nBar, PRECT lprect,
			 PINT arrowSize, PINT thumbSize, PINT thumbPos)
{
  INT pixels;
  BOOL vertical;
  RECT ClientRect;
  ULONG Style;
DbgPrint("[SCROLL_GetScrollBarRect]");
  W32kGetClientRect (Window, &ClientRect);
DbgPrint("[WindowRect:%d,%d,%d,%d]\n", Window->WindowRect.left, Window->WindowRect.top, Window->WindowRect.right, Window->WindowRect.bottom);
DbgPrint("[ClientRect:%d,%d,%d,%d]\n", ClientRect.left, ClientRect.top, ClientRect.right, ClientRect.bottom);

  switch (nBar)
    {
    case SB_HORZ:
      DbgPrint ("[SCROLL_GetScrollBarRect:SB_HORZ]");
      lprect->left = ClientRect.left - Window->WindowRect.left;
      lprect->top = ClientRect.bottom - Window->WindowRect.top;
      lprect->right = ClientRect.right - Window->WindowRect.left;
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
      DbgPrint ("[SCROLL_GetScrollBarRect:SB_VERT]\n");
/*    lprect->left = ClientRect.right - Window->WindowRect.left;
      lprect->top = ClientRect.top - Window->WindowRect.top;
      lprect->right = lprect->left + NtUserGetSystemMetrics (SM_CXVSCROLL);
      lprect->bottom = ClientRect.bottom - WindowRect.top; */
      lprect->left = Window->WindowRect.left + ClientRect.right;
      lprect->top = Window->WindowRect.bottom - ClientRect.bottom;
      lprect->right = lprect->left + NtUserGetSystemMetrics (SM_CXVSCROLL);
      lprect->bottom = Window->WindowRect.bottom;
      if (Window->Style & WS_BORDER)
	{
	  lprect->top--;
	  lprect->bottom++;
	}
      else if (Window->Style & WS_HSCROLL)
	lprect->bottom++;
      DbgPrint ("[VERTDIMEN:%d,%d,%d,%d]\n", lprect->left, lprect->top,
		lprect->right, lprect->bottom);
      DbgPrint ("[Window:%d,%d,%d,%d]\n", Window->WindowRect.left, Window->WindowRect.top,
		Window->WindowRect.right, Window->WindowRect.bottom);
      DbgPrint ("[Client:%d,%d,%d,%d]\n", ClientRect.left, ClientRect.top,
		ClientRect.right, ClientRect.bottom);
      DbgPrint ("[NtUserGetSystemMetrics(SM_CXVSCROLL):%d]\n",
		NtUserGetSystemMetrics (SM_CXVSCROLL));
      vertical = TRUE;
      break;

    case SB_CTL:
      DbgPrint ("[SCROLL_GetScrollBarRect:SB_CTL]");
      W32kGetClientRect (Window, lprect);
      vertical = ((Window->Style & SBS_VERT) != 0);
      break;

    default:
      DbgPrint ("[SCROLL_GetScrollBarRect:FAIL]");
      W32kReleaseWindowObject(Window);
      return FALSE;
    }

  if (vertical)
    pixels = lprect->bottom - lprect->top;
  else
    pixels = lprect->right - lprect->left;

  if (pixels <= 2 * NtUserGetSystemMetrics (SM_CXVSCROLL) + SCROLL_MIN_RECT)
    {
      if (pixels > SCROLL_MIN_RECT)
	*arrowSize = (pixels - SCROLL_MIN_RECT) / 2;
      else
	*arrowSize = 0;
      *thumbPos = *thumbSize = 0;
    }
  else
    {
/*      PSCROLLBARINFO info;

      NtUserGetScrollBarInfo (hWnd, nBar, info); recursive loop.. since called function calls this function */

      *arrowSize = NtUserGetSystemMetrics (SM_CXVSCROLL);
      pixels -=
	(2 * (NtUserGetSystemMetrics (SM_CXVSCROLL) - SCROLL_ARROW_THUMB_OVERLAP));
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
  W32kReleaseWindowObject(Window);

  return vertical;
}

DWORD
STDCALL
NtUserGetScrollBarInfo(HWND hWnd, LONG idObject, PSCROLLBARINFO psbi)
{
  PWINDOW_OBJECT Window = W32kGetWindowObject(hWnd);
  int thumbSize = 20, arrowSize = 20, thumbPos = 0;

  if (!Window) return FALSE;

  switch(idObject)
  {
    case SB_HORZ: psbi = Window->pHScroll; break;
    case SB_VERT: psbi = Window->pVScroll; break;
    case SB_CTL:  psbi = Window->wExtra; break;
    default:
      W32kReleaseWindowObject(Window);
      return FALSE;
  }

  if (!psbi)  /* Create the info structure if needed */
  {
    if ((psbi = ExAllocatePool(NonPagedPool, sizeof(SCROLLBARINFO))))
    {
      DbgPrint("Creating PSCROLLBARINFO for %d - psbi: %08x\n", idObject, psbi);
      SCROLL_GetScrollBarRect (Window, idObject, &(psbi->rcScrollBar), &arrowSize, &thumbSize, &thumbPos);
      DbgPrint("NtUserGetScrollBarInfo: Creating with rect (%d,%d,%d,%d)\n",
               psbi->rcScrollBar.left, psbi->rcScrollBar.top, psbi->rcScrollBar.right, psbi->rcScrollBar.bottom);

      if (idObject == SB_HORZ) Window->pHScroll = psbi;
        else Window->pVScroll = psbi;
    }
/*    if (!hUpArrow) SCROLL_LoadBitmaps(); FIXME: This must be moved somewhere in user32 code */
  }
DbgPrint("z1: psbi: %08x\n", psbi);
  W32kReleaseWindowObject(Window);
DbgPrint("z2: psbi: %08x\n", psbi);
  return TRUE;
}

/* Ported from WINE20020904 */
BOOL
SCROLL_ShowScrollBar (HWND hwnd, INT nBar, BOOL fShowH, BOOL fShowV)
{
  PWINDOW_OBJECT Window = W32kGetWindowObject(hwnd);

  switch (nBar)
    {
    case SB_CTL:
      NtUserShowWindow (hwnd, fShowH ? SW_SHOW : SW_HIDE);
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
      if (nBar == SB_HORZ)
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
      if (nBar == SB_VERT)
	fShowH = FALSE;
      break;

    default:
      return FALSE;		/* Nothing to do! */
    }

  if (fShowH || fShowV)		/* frame has been changed, let the window redraw itself */
  {
    NtUserSetWindowPos (hwnd, 0, 0, 0, 0, 0,
                        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    return TRUE;
  }
  return FALSE;			/* no frame changes */
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

DWORD
STDCALL
NtUserShowScrollBar(HWND hWnd, int wBar, DWORD bShow)
{
DbgPrint("[NtUserShowScrollBar:%d]", bShow);
  SCROLL_ShowScrollBar (hWnd, wBar, (wBar == SB_VERT) ? 0 : bShow, (wBar == SB_HORZ) ? 0 : bShow);

  return 0;
}

/* EOF */
