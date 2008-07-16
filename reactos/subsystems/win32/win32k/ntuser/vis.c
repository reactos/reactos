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
   PWINDOW Wnd, CurrentWnd, PreviousWnd, CurrentSiblingWnd;

   Wnd = Window->Wnd;

   if (!(Wnd->Style & WS_VISIBLE))
   {
      return NULL;
   }

   if (ClientArea)
   {
      VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->ClientRect);
   }
   else
   {
      VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->WindowRect);
   }

   /*
    * Walk through all parent windows and for each clip the visble region
    * to the parent's client area and exclude all siblings that are over
    * our window.
    */

   PreviousWindow = Window;
   PreviousWnd = PreviousWindow->Wnd;
   CurrentWindow = Window->Parent;
   while (CurrentWindow)
   {
      CurrentWnd = CurrentWindow->Wnd;
      if (!(CurrentWnd) || !(CurrentWnd->Style & WS_VISIBLE))
      {
         NtGdiDeleteObject(VisRgn);
         return NULL;
      }

      ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentWnd->ClientRect);
      NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_AND);
      NtGdiDeleteObject(ClipRgn);

      if ((PreviousWnd->Style & WS_CLIPSIBLINGS) ||
          (PreviousWnd == Wnd && ClipSiblings))
      {
         CurrentSibling = CurrentWindow->FirstChild;
         while (CurrentSibling != NULL && CurrentSibling != PreviousWindow)
         {
            CurrentSiblingWnd = CurrentSibling->Wnd;
            if ((CurrentSiblingWnd->Style & WS_VISIBLE) &&
                !(CurrentSiblingWnd->ExStyle & WS_EX_TRANSPARENT))
            {
               ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentSiblingWnd->WindowRect);
               /* Combine it with the window region if available */
               if (CurrentSibling->WindowRegion && !(CurrentSiblingWnd->Style & WS_MINIMIZE))
               {
                  NtGdiOffsetRgn(ClipRgn, -CurrentSiblingWnd->WindowRect.left, -CurrentSiblingWnd->WindowRect.top);
                  NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentSibling->WindowRegion, RGN_AND);
                  NtGdiOffsetRgn(ClipRgn, CurrentSiblingWnd->WindowRect.left, CurrentSiblingWnd->WindowRect.top);
               }
               NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
               NtGdiDeleteObject(ClipRgn);
            }
            CurrentSibling = CurrentSibling->NextSibling;
         }
      }

      PreviousWindow = CurrentWindow;
      PreviousWnd = PreviousWindow->Wnd;
      CurrentWindow = CurrentWindow->Parent;
   }

   if (ClipChildren)
   {
      CurrentWindow = Window->FirstChild;
      while (CurrentWindow)
      {
         CurrentWnd = CurrentWindow->Wnd;
         if ((CurrentWnd->Style & WS_VISIBLE) &&
             !(CurrentWnd->ExStyle & WS_EX_TRANSPARENT))
         {
            ClipRgn = UnsafeIntCreateRectRgnIndirect(&CurrentWnd->WindowRect);
            /* Combine it with the window region if available */
            if (CurrentWindow->WindowRegion && !(CurrentWnd->Style & WS_MINIMIZE))
            {
               NtGdiOffsetRgn(ClipRgn, -CurrentWnd->WindowRect.left, -CurrentWnd->WindowRect.top);
               NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentWindow->WindowRegion, RGN_AND);
               NtGdiOffsetRgn(ClipRgn, CurrentWnd->WindowRect.left, CurrentWnd->WindowRect.top);
            }
            NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
            NtGdiDeleteObject(ClipRgn);
         }
         CurrentWindow = CurrentWindow->NextSibling;
      }
   }

   if (Window->WindowRegion && !(Wnd->Style & WS_MINIMIZE))
   {
      NtGdiOffsetRgn(VisRgn, -Wnd->WindowRect.left, -Wnd->WindowRect.top);
      NtGdiCombineRgn(VisRgn, VisRgn, Window->WindowRegion, RGN_AND);
      NtGdiOffsetRgn(VisRgn, Wnd->WindowRect.left, Wnd->WindowRect.top);
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
   PWINDOW Wnd, ParentWnd;

   ASSERT_REFS_CO(Window);

   Wnd = Window->Wnd;

   Temp = NtGdiCreateRectRgn(0, 0, 0, 0);
   NtGdiCombineRgn(Temp, NewlyExposed, NULL, RGN_COPY);

   Parent = Window->Parent;
   if(Parent)
   {
      ParentWnd = Parent->Wnd;
      NtGdiOffsetRgn(Temp,
                     Wnd->WindowRect.left - ParentWnd->ClientRect.left,
                     Wnd->WindowRect.top - ParentWnd->ClientRect.top);

      UserRefObjectCo(Parent, &Ref);
      co_UserRedrawWindow(Parent, NULL, Temp,
                          RDW_FRAME | RDW_ERASE | RDW_INVALIDATE |
                          RDW_ALLCHILDREN);
      UserDerefObjectCo(Parent);
   }
   NtGdiDeleteObject(Temp);
}

/* EOF */
