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
 * $Id: vis.c,v 1.14 2003/12/12 18:59:24 weiden Exp $
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
      NtGdiSetEmptyRect(Rect);

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
	      NtGdiSetEmptyRect(Rect);
	      return FALSE;
	    }
	  if (! NtGdiIntersectRect(Rect, Rect, &(Window->ClientRect)))
	    {
	      return FALSE;
	    }
	}
      while (0 != (Window->Style & WS_CHILD));
    }

  DesktopWindow = IntGetWindowObject(Desktop->DesktopWindow);
  if (NULL == DesktopWindow)
    {
      ASSERT(FALSE);
      return FALSE;
    }

  if (! NtGdiIntersectRect(Rect, Rect, &(DesktopWindow->ClientRect)))
    {
      IntReleaseWindowObject(DesktopWindow);
      return FALSE;
    }
  IntReleaseWindowObject(DesktopWindow);

  return TRUE;
}

STATIC BOOL FASTCALL
VIS_AddClipRects(PWINDOW_OBJECT Parent, PWINDOW_OBJECT End, 
		 HRGN ClipRgn, PRECT Rect)
{
  PWINDOW_OBJECT Child;
  RECT Intersect;

  ExAcquireFastMutexUnsafe(&Parent->ChildrenListLock);
  Child = Parent->FirstChild;
  while (Child)
  {
    if (Child == End)
    {
      ExReleaseFastMutexUnsafe(&Parent->ChildrenListLock);
      return TRUE;
    }
    
    if (Child->Style & WS_VISIBLE)
    {
      if (NtGdiIntersectRect(&Intersect, &Child->WindowRect, Rect))
      {
        UnsafeIntUnionRectWithRgn(ClipRgn, &Child->WindowRect);
      }
    }
    
    Child = Child->NextSibling;
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
  INT LeftOffset, TopOffset;

  DesktopWindow = IntGetWindowObject(Desktop->DesktopWindow);
  if (NULL == DesktopWindow)
    {
      ASSERT(FALSE);
      return NULL;
    }

  if (VIS_GetVisRect(Desktop, Window, ClientArea, &Rect))
    {
      VisRgn = UnsafeIntCreateRectRgnIndirect(&Rect);
      if (NULL != VisRgn)
	{
	  if (ClientArea)
	    {
	      LeftOffset = Window->ClientRect.left;
	      TopOffset = Window->ClientRect.top;
	    }
	  else
	    {
	      LeftOffset = Window->WindowRect.left;
	      TopOffset = Window->WindowRect.top;
	    }

	  ClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

	  if (ClipRgn != NULL)
	    {
	      if (ClipChildren && Window->FirstChild)
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

	      NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
	      NtGdiDeleteObject(ClipRgn);
	      NtGdiOffsetRgn(VisRgn, -LeftOffset, -TopOffset);
	    }
	  else
	    {
	      NtGdiDeleteObject(VisRgn);
	      VisRgn = NULL;
	    }
	}
    }
  else
    {
      VisRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
    }

  IntReleaseWindowObject(DesktopWindow);

  return VisRgn;
}

static VOID FASTCALL
GetUncoveredArea(HRGN Uncovered, PWINDOW_OBJECT Parent, PWINDOW_OBJECT TargetChild,
                 BOOL IncludeTarget)
{
  PWINDOW_OBJECT Child;
  BOOL Passed;
  HRGN Covered;

  Passed = FALSE;
  ExAcquireFastMutexUnsafe(&Parent->ChildrenListLock);
  Child = Parent->FirstChild;
  while (! Passed && Child)
    {
      if (0 != (Child->Style & WS_VISIBLE) && (Child != TargetChild || IncludeTarget))
	{
	  Covered = UnsafeIntCreateRectRgnIndirect(&Child->WindowRect);
	  NtGdiCombineRgn(Uncovered, Uncovered, Covered, RGN_DIFF);
	  NtGdiDeleteObject(Covered);
	}
      if (Child == TargetChild)
	{
	  Passed = TRUE;
	}
      Child = Child->NextSibling;
    }
  ExReleaseFastMutexUnsafe(&Parent->ChildrenListLock);
}

VOID FASTCALL
VIS_WindowLayoutChanged(PDESKTOP_OBJECT Desktop, PWINDOW_OBJECT Window,
                        HRGN NewlyExposed)
{
  PWINDOW_OBJECT DesktopWindow;
  PWINDOW_OBJECT Parent;
  PWINDOW_OBJECT Sibling;
  PWINDOW_OBJECT TopLevel;
  HRGN Uncovered;
  HRGN Repaint;
  HRGN DirtyRgn;
  HRGN ExposedWindow;
  HRGN Covered;
  INT RgnType;
  POINT Offset;

  DesktopWindow = IntGetWindowObject(Desktop->DesktopWindow);
  Uncovered = UnsafeIntCreateRectRgnIndirect(&DesktopWindow->WindowRect);

  if (Window->Style & WS_CHILD)
    {
      /* Determine our toplevel window */
      TopLevel = Window;
      while (TopLevel->Style & WS_CHILD)
	{
	  TopLevel = TopLevel->Parent;
	}

      GetUncoveredArea(Uncovered, DesktopWindow, TopLevel, FALSE);
      Parent = Window->Parent;
    }
  else
    {
      Parent = DesktopWindow;
    }
  GetUncoveredArea(Uncovered, Parent, Window, TRUE);

  ExAcquireFastMutexUnsafe(&Parent->ChildrenListLock);
  Sibling = Window->NextSibling;
  while (Sibling)
    {
      if (0 != (Sibling->Style & WS_VISIBLE))
	{
	  Offset.x = - Sibling->WindowRect.left;
	  Offset.y = - Sibling->WindowRect.top;
	  DirtyRgn = REGION_CropRgn(NULL, Uncovered, &Sibling->WindowRect, &Offset);
	  Offset.x = Window->WindowRect.left - Sibling->WindowRect.left;
	  Offset.y = Window->WindowRect.top - Sibling->WindowRect.top;
	  ExposedWindow = REGION_CropRgn(NULL, NewlyExposed, NULL, &Offset);
	  RgnType = NtGdiCombineRgn(DirtyRgn, DirtyRgn, ExposedWindow, RGN_AND);
	  if (NULLREGION != RgnType && ERROR != RgnType)
	    {
              NtGdiOffsetRgn(DirtyRgn,
                Sibling->WindowRect.left - Sibling->ClientRect.left,
                Sibling->WindowRect.top - Sibling->ClientRect.top);
	      IntRedrawWindow(Sibling, NULL, DirtyRgn,
	                      RDW_INVALIDATE | RDW_FRAME | RDW_ERASE |
	                      RDW_ALLCHILDREN);
	    }
	  Covered = UnsafeIntCreateRectRgnIndirect(&Sibling->WindowRect);
	  NtGdiCombineRgn(Uncovered, Uncovered, Covered, RGN_DIFF);
	  NtGdiDeleteObject(Covered);
	  NtGdiDeleteObject(ExposedWindow);
	  NtGdiDeleteObject(DirtyRgn);
	}

      Sibling = Sibling->NextSibling;
    }
  ExReleaseFastMutexUnsafe(&Parent->ChildrenListLock);

  if (Window->Style & WS_CHILD)
    {
      Offset.x = - Parent->WindowRect.left;
      Offset.y = - Parent->WindowRect.top;
      DirtyRgn = REGION_CropRgn(NULL, Uncovered, &Parent->WindowRect, &Offset);
      Offset.x = Window->WindowRect.left - Parent->WindowRect.left;
      Offset.y = Window->WindowRect.top - Parent->WindowRect.top;
      ExposedWindow = REGION_CropRgn(NULL, NewlyExposed, NULL, &Offset);
      RgnType = NtGdiCombineRgn(DirtyRgn, DirtyRgn, ExposedWindow, RGN_AND);
      if (NULLREGION != RgnType && ERROR != RgnType)
	{
          NtGdiOffsetRgn(DirtyRgn,
            Parent->WindowRect.left - Parent->ClientRect.left,
            Parent->WindowRect.top - Parent->ClientRect.top);
	  IntRedrawWindow(Parent, NULL, DirtyRgn,
	                  RDW_INVALIDATE | RDW_FRAME | RDW_ERASE |
	                  RDW_NOCHILDREN);
	}
      NtGdiDeleteObject(ExposedWindow);
      NtGdiDeleteObject(DirtyRgn);
    }
  else
    {
      Repaint = NtGdiCreateRectRgn(0, 0, 0, 0);
      NtGdiCombineRgn(Repaint, NewlyExposed, NULL, RGN_COPY);
      NtGdiOffsetRgn(Repaint, Window->WindowRect.left, Window->WindowRect.top);
      NtGdiCombineRgn(Repaint, Repaint, Uncovered, RGN_AND);
      NtUserRedrawWindow(DesktopWindow->Self, NULL, Repaint,
                         RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOCHILDREN);
      NtGdiDeleteObject(Repaint);
    }

  NtGdiDeleteObject(Uncovered);
}

/* EOF */
