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
 * $Id: vis.c,v 1.16 2004/01/17 15:18:25 navaraf Exp $
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

#if 1
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
VIS_ComputeVisibleRegion(PWINDOW_OBJECT Window,
                         BOOLEAN ClientArea, BOOLEAN ClipChildren,
                         BOOLEAN ClipSiblings)
{
  HRGN VisRgn;
  RECT Rect;
  HRGN ClipRgn;
  PWINDOW_OBJECT DesktopWindow;
  INT LeftOffset, TopOffset;
  PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop;

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
#else
HRGN FASTCALL
VIS_ComputeVisibleRegion(
   PWINDOW_OBJECT Window,
   BOOLEAN ClientArea,
   BOOLEAN ClipChildren,
   BOOLEAN ClipSiblings)
{
   HRGN VisRgn, ClipRgn;
   INT LeftOffset, TopOffset;
   PWINDOW_OBJECT CurrentWindow;

   if (!(Window->Style & WS_VISIBLE))
   {
      return NULL;
   }

   if (ClientArea)
   {
      VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
      LeftOffset = Window->ClientRect.left;
      TopOffset = Window->ClientRect.top;
   }
   else
   {
      VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
      LeftOffset = Window->WindowRect.left;
      TopOffset = Window->WindowRect.top;
   }

   CurrentWindow = Window->Parent;
   while (CurrentWindow)
   {
      if (!(CurrentWindow->Style & WS_VISIBLE))
      {
         NtGdiDeleteObject(VisRgn);
         return NULL;
      }
      ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentWindow->ClientRect);
      NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_AND);
      NtGdiDeleteObject(ClipRgn);
      CurrentWindow = CurrentWindow->Parent;
   }

   if (ClipChildren)
   {
      ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);
      CurrentWindow = Window->FirstChild;
      while (CurrentWindow)
      {
         if (CurrentWindow->Style & WS_VISIBLE)
         {
            ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentWindow->WindowRect);
            NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
            NtGdiDeleteObject(ClipRgn);
         }
         CurrentWindow = CurrentWindow->NextSibling;
      }
      ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);
   }

   if (ClipSiblings && Window->Parent != NULL)
   {
      ExAcquireFastMutexUnsafe(&Window->Parent->ChildrenListLock);
      CurrentWindow = Window->Parent->FirstChild;
      while (CurrentWindow != Window)
      {
         if (CurrentWindow->Style & WS_VISIBLE)
         {
            ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentWindow->WindowRect);
            NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
            NtGdiDeleteObject(ClipRgn);
         }
         CurrentWindow = CurrentWindow->NextSibling;
      }
      ExReleaseFastMutexUnsafe(&Window->Parent->ChildrenListLock);
   }

   NtGdiOffsetRgn(VisRgn, -LeftOffset, -TopOffset);

   return VisRgn;
}
#endif

VOID FASTCALL
VIS_WindowLayoutChanged(
   PWINDOW_OBJECT Window,
   HRGN NewlyExposed)
{
   HRGN Temp;
   
   Temp = NtGdiCreateRectRgn(0, 0, 0, 0);
   NtGdiCombineRgn(Temp, NewlyExposed, NULL, RGN_COPY);
   if (Window->Parent != NULL)
   {
      NtGdiOffsetRgn(Temp,
                     Window->WindowRect.left - Window->Parent->ClientRect.left,
                     Window->WindowRect.top - Window->Parent->ClientRect.top);
   }
   IntRedrawWindow(Window->Parent, NULL, Temp,
                   RDW_FRAME | RDW_ERASE | RDW_INVALIDATE | 
                   RDW_ALLCHILDREN);
   NtGdiDeleteObject(Temp);
}

/* EOF */
