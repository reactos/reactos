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
 *
 * $Id: vis.c,v 1.2 2003/07/18 20:55:21 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Visibility computations
 * FILE:             subsys/win32k/ntuser/vis.c
 * PROGRAMMER:       Ge van Geldorp (ge@gse.nl)
 */

#include <win32k/win32k.h>
#include <include/painting.h>
#include <include/rect.h>
#include <include/vis.h>


#define NDEBUG
#include <win32k/debug1.h>
#include <debug.h>

BOOL STATIC FASTCALL
VIS_GetVisRect(PDESKTOP_OBJECT Desktop, PWINDOW_OBJECT Window,
               BOOLEAN ClientArea, RECT* Rect)
{
  PWINDOW_OBJECT DesktopWindow;

  if (ClientArea)
    {
      *Rect = Window->ClientRect;
    }
  else
    {
      *Rect = Window->WindowRect;
    }

  if (0 == (Window->Style & WS_VISIBLE))
    {
      W32kSetEmptyRect(Rect);

      return FALSE;
    }

  if (Window->Self == Desktop->DesktopWindow)
    {
      return TRUE;
    }

  if (0 != (Window->Style & WS_CHILD))
    {
      do
	{
	  Window = Window->Parent;
	  if (WS_VISIBLE != (Window->Style & (WS_ICONIC | WS_VISIBLE)))
	    {
	      W32kSetEmptyRect(Rect);
	      return FALSE;
	    }
	  if (! W32kIntersectRect(Rect, Rect, &(Window->ClientRect)))
	    {
	      return FALSE;
	    }
	}
      while (0 != (Window->Style & WS_CHILD));
    }

  DesktopWindow = W32kGetWindowObject(Desktop->DesktopWindow);
  if (NULL == DesktopWindow)
    {
      ASSERT(FALSE);
      return FALSE;
    }

  if (! W32kIntersectRect(Rect, Rect, &(DesktopWindow->ClientRect)))
    {
      W32kReleaseWindowObject(DesktopWindow);
      return FALSE;
    }
  W32kReleaseWindowObject(DesktopWindow);

  return TRUE;
}

STATIC BOOL FASTCALL
VIS_AddClipRects(PWINDOW_OBJECT Parent, PWINDOW_OBJECT End, 
		 HRGN ClipRgn, PRECT Rect)
{
  PLIST_ENTRY ChildListEntry;
  PWINDOW_OBJECT Child;
  RECT Intersect;

  ExAcquireFastMutexUnsafe(&Parent->ChildrenListLock);
  ChildListEntry = Parent->ChildrenListHead.Flink;
  while (ChildListEntry != &Parent->ChildrenListHead)
    {
      Child = CONTAINING_RECORD(ChildListEntry, WINDOW_OBJECT, 
				SiblingListEntry);
      if (Child == End)
	{
	  ExReleaseFastMutexUnsafe(&Parent->ChildrenListLock);
	  return TRUE;
	}
      if (Child->Style & WS_VISIBLE)
	{
	  if (W32kIntersectRect(&Intersect, &Child->WindowRect, Rect))
	    {
	      UnsafeW32kUnionRectWithRgn(ClipRgn, &Child->WindowRect);
	    }
	}
      ChildListEntry = ChildListEntry->Flink;
    }

  ExReleaseFastMutexUnsafe(&Parent->ChildrenListLock);
  return FALSE;
}

HRGN FASTCALL
VIS_ComputeVisibleRegion(PDESKTOP_OBJECT Desktop, PWINDOW_OBJECT Window,
                         BOOLEAN ClientArea, BOOLEAN ClipChildren,
                         BOOLEAN ClipSiblings)
{
  HRGN VisRgn;
  RECT Rect;
  HRGN ClipRgn;
  PWINDOW_OBJECT DesktopWindow;

  DesktopWindow = W32kGetWindowObject(Desktop->DesktopWindow);
  if (NULL == DesktopWindow)
    {
      ASSERT(FALSE);
      return NULL;
    }

  if (VIS_GetVisRect(Desktop, Window, ClientArea, &Rect))
    {
      VisRgn = UnsafeW32kCreateRectRgnIndirect(&Rect);
      if (NULL != VisRgn)
	{
	  ClipRgn = W32kCreateRectRgn(0, 0, 0, 0);

	  if (ClipRgn != NULL)
	    {
	      if (ClipChildren && 
		  ! IsListEmpty(&Window->ChildrenListHead))
		{
		  VIS_AddClipRects(Window, NULL, ClipRgn, &Rect);
		}

	      if (ClipSiblings && 0 != (Window->Style & WS_CHILD))
		{
		  VIS_AddClipRects(Window->Parent, Window, ClipRgn, &Rect);
		}
	      
	      while (0 != (Window->Style & WS_CHILD))
		{
		  if (0 != (Window->Style & WS_CLIPSIBLINGS))
		    {
		      VIS_AddClipRects(Window->Parent, Window, ClipRgn, &Rect);
		    }
		  Window = Window->Parent;
		}

	      VIS_AddClipRects(DesktopWindow, Window, ClipRgn, &Rect);

	      W32kCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
	      W32kDeleteObject(ClipRgn);
	    }
	  else
	    {
	      W32kDeleteObject(VisRgn);
	      VisRgn = NULL;
	    }
	}
    }
  else
    {
      VisRgn = W32kCreateRectRgn(0, 0, 0, 0);
    }

  W32kReleaseWindowObject(DesktopWindow);

  return VisRgn;
}

/* FIXME: to be replaced by a normal window proc in CSRSS */
VOID STATIC FASTCALL
VIS_RepaintDesktop(HWND Desktop, HRGN RepaintRgn)
{
  HDC dc = NtUserGetDC(Desktop);
  HBRUSH DesktopBrush = W32kCreateSolidBrush(RGB(58, 110, 165));
  W32kFillRgn(dc, RepaintRgn, DesktopBrush);
  W32kDeleteObject(DesktopBrush);
  NtUserReleaseDC(Desktop, dc);
}


VOID FASTCALL
VIS_WindowLayoutChanged(PDESKTOP_OBJECT Desktop, PWINDOW_OBJECT Window,
                        HRGN NewlyExposed)
{
  PWINDOW_OBJECT DesktopWindow;
  PWINDOW_OBJECT Child;
  PLIST_ENTRY CurrentEntry;
  HRGN Uncovered;
  HRGN Covered;
  HRGN Repaint;

  DesktopWindow = W32kGetWindowObject(Desktop->DesktopWindow);
  Uncovered = UnsafeW32kCreateRectRgnIndirect(&DesktopWindow->WindowRect);
  ExAcquireFastMutexUnsafe(&DesktopWindow->ChildrenListLock);
  CurrentEntry = DesktopWindow->ChildrenListHead.Flink;
  while (CurrentEntry != &DesktopWindow->ChildrenListHead)
    {
      Child = CONTAINING_RECORD(CurrentEntry, WINDOW_OBJECT, SiblingListEntry);
      if (0 != (Child->Style & WS_VISIBLE))
	{
	  Covered = UnsafeW32kCreateRectRgnIndirect(&Child->WindowRect);
	  W32kCombineRgn(Uncovered, Uncovered, Covered, RGN_DIFF);
	  PaintRedrawWindow(Child->Self, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE, 0);
	  W32kDeleteObject(Covered);
	}
      CurrentEntry = CurrentEntry->Flink;
    }
  ExReleaseFastMutexUnsafe(&DesktopWindow->ChildrenListLock);

  Repaint = W32kCreateRectRgn(0, 0, 0, 0);
  W32kCombineRgn(Repaint, NewlyExposed, Uncovered, RGN_AND);
  VIS_RepaintDesktop(DesktopWindow->Self, Repaint);
  W32kDeleteObject(Repaint);

  W32kDeleteObject(Uncovered);
}

/* EOF */
