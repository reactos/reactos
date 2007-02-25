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
 * $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Visibility computations
 * FILE:             subsys/win32k/ntuser/vis.c
 * PROGRAMMER:       Ge van Geldorp (ge@gse.nl)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

HRGN FASTCALL
VIS_ComputeVisibleRegion(
   PWINDOW_OBJECT Window,
   BOOLEAN ClientArea,
   BOOLEAN ClipChildren,
   BOOLEAN ClipSiblings)
{
   HRGN VisRgn, ClipRgn;
   PWINDOW_OBJECT PreviousWindow, CurrentWindow, CurrentSibling;

   if (!(Window->Style & WS_VISIBLE))
   {
      return NULL;
   }

   if (ClientArea)
   {
      if (!(ClipRgn = VIS_ComputeVisibleRegion(Window, FALSE, ClipChildren, ClipSiblings)))
      {
         return NULL;
      }
      if (!(VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect)))
      {
         NtGdiDeleteObject(VisRgn);
         return NULL;
      }
      NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_AND);
      NtGdiDeleteObject(ClipRgn);
      return VisRgn;
   }

   VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);

   /*
    * Walk through all parent windows and for each clip the visble region
    * to the parent's client area and exclude all siblings that are over
    * our window.
    */

   PreviousWindow = Window;
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

      if ((PreviousWindow->Style & WS_CLIPSIBLINGS) ||
          (PreviousWindow == Window && ClipSiblings))
      {
         CurrentSibling = CurrentWindow->FirstChild;
         while (CurrentSibling != NULL && CurrentSibling != PreviousWindow)
         {
            if ((CurrentSibling->Style & WS_VISIBLE) &&
                !(CurrentSibling->ExStyle & WS_EX_TRANSPARENT))
            {
               ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentSibling->WindowRect);
               /* Combine it with the window region if available */
               if (CurrentSibling->WindowRegion && !(CurrentSibling->Style & WS_MINIMIZE))
               {
                  NtGdiOffsetRgn(ClipRgn, -CurrentSibling->WindowRect.left, -CurrentSibling->WindowRect.top);
                  NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentSibling->WindowRegion, RGN_AND);
                  NtGdiOffsetRgn(ClipRgn, CurrentSibling->WindowRect.left, CurrentSibling->WindowRect.top);
               }
               NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
               NtGdiDeleteObject(ClipRgn);
            }
            CurrentSibling = CurrentSibling->NextSibling;
         }
      }

      PreviousWindow = CurrentWindow;
      CurrentWindow = CurrentWindow->Parent;
   }

   if (ClipChildren)
   {
      CurrentWindow = Window->FirstChild;
      while (CurrentWindow)
      {
         if ((CurrentWindow->Style & WS_VISIBLE) &&
             !(CurrentWindow->ExStyle & WS_EX_TRANSPARENT))
         {
            ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentWindow->WindowRect);
            /* Combine it with the window region if available */
            if (CurrentWindow->WindowRegion && !(CurrentWindow->Style & WS_MINIMIZE))
            {
               NtGdiOffsetRgn(ClipRgn, -CurrentWindow->WindowRect.left, -CurrentWindow->WindowRect.top);
               NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentWindow->WindowRegion, RGN_AND);
               NtGdiOffsetRgn(ClipRgn, CurrentWindow->WindowRect.left, CurrentWindow->WindowRect.top);
            }
            NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
            NtGdiDeleteObject(ClipRgn);
         }
         CurrentWindow = CurrentWindow->NextSibling;
      }
   }

   if (Window->WindowRegion && !(Window->Style & WS_MINIMIZE))
   {
      NtGdiOffsetRgn(VisRgn, -Window->WindowRect.left, -Window->WindowRect.top);
      NtGdiCombineRgn(VisRgn, VisRgn, Window->WindowRegion, RGN_AND);
      NtGdiOffsetRgn(VisRgn, Window->WindowRect.left, Window->WindowRect.top);
   }

   return VisRgn;
}

VOID FASTCALL
co_VIS_WindowLayoutChanged(
   PWINDOW_OBJECT Window,
   HRGN NewlyExposed)
{
   HRGN Temp;
   PWINDOW_OBJECT Parent;
   USER_REFERENCE_ENTRY Ref;

   ASSERT_REFS_CO(Window);

   Temp = NtGdiCreateRectRgn(0, 0, 0, 0);
   NtGdiCombineRgn(Temp, NewlyExposed, NULL, RGN_COPY);

   Parent = Window->Parent;
   if(Parent)
   {
      NtGdiOffsetRgn(Temp,
                     Window->WindowRect.left - Parent->ClientRect.left,
                     Window->WindowRect.top - Parent->ClientRect.top);

      UserRefObjectCo(Parent, &Ref);
      co_UserRedrawWindow(Parent, NULL, Temp,
                          RDW_FRAME | RDW_ERASE | RDW_INVALIDATE |
                          RDW_ALLCHILDREN);
      UserDerefObjectCo(Parent);
   }
   NtGdiDeleteObject(Temp);
}

/* EOF */
