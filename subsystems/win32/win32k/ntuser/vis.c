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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
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
   PWND Wnd, CurrentWnd, PreviousWnd, CurrentSiblingWnd;

   Wnd = Window->Wnd;

   if (!Wnd || !(Wnd->style & WS_VISIBLE))
   {
      return NULL;
   }

   VisRgn = NULL;

   if (ClientArea)
   {
      VisRgn = IntSysCreateRectRgnIndirect(&Window->Wnd->rcClient);
   }
   else
   {
      VisRgn = IntSysCreateRectRgnIndirect(&Window->Wnd->rcWindow);
   }

   /*
    * Walk through all parent windows and for each clip the visble region
    * to the parent's client area and exclude all siblings that are over
    * our window.
    */

   PreviousWindow = Window;
   PreviousWnd = PreviousWindow->Wnd;
   CurrentWindow = Window->spwndParent;
   while (CurrentWindow)
   {
      if ( CurrentWindow->state & WINDOWSTATUS_DESTROYING || // state2
           CurrentWindow->state & WINDOWSTATUS_DESTROYED )
      {
         DPRINT1("ATM the Current Window or Parent is dead!\n");
         if (VisRgn) REGION_FreeRgnByHandle(VisRgn);
         return NULL;
      }

      CurrentWnd = CurrentWindow->Wnd;
      if (!CurrentWnd || !(CurrentWnd->style & WS_VISIBLE))
      {
         if (VisRgn) REGION_FreeRgnByHandle(VisRgn);
         return NULL;
      }

      ClipRgn = IntSysCreateRectRgnIndirect(&CurrentWnd->rcClient);
      NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_AND);
      REGION_FreeRgnByHandle(ClipRgn);

      if ((PreviousWnd->style & WS_CLIPSIBLINGS) ||
          (PreviousWnd == Wnd && ClipSiblings))
      {
         CurrentSibling = CurrentWindow->spwndChild;
         while ( CurrentSibling != NULL && 
                 CurrentSibling != PreviousWindow &&
                 CurrentSibling->Wnd )
         {
            CurrentSiblingWnd = CurrentSibling->Wnd;
            if ((CurrentSiblingWnd->style & WS_VISIBLE) &&
                !(CurrentSiblingWnd->ExStyle & WS_EX_TRANSPARENT))
            {
               ClipRgn = IntSysCreateRectRgnIndirect(&CurrentSiblingWnd->rcWindow);
               /* Combine it with the window region if available */
               if (CurrentSibling->hrgnClip && !(CurrentSiblingWnd->style & WS_MINIMIZE))
               {
                  NtGdiOffsetRgn(ClipRgn, -CurrentSiblingWnd->rcWindow.left, -CurrentSiblingWnd->rcWindow.top);
                  NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentSibling->hrgnClip, RGN_AND);
                  NtGdiOffsetRgn(ClipRgn, CurrentSiblingWnd->rcWindow.left, CurrentSiblingWnd->rcWindow.top);
               }
               NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
               REGION_FreeRgnByHandle(ClipRgn);
            }
            CurrentSibling = CurrentSibling->spwndNext;
         }
      }

      PreviousWindow = CurrentWindow;
      PreviousWnd = PreviousWindow->Wnd;
      CurrentWindow = CurrentWindow->spwndParent;
   }

   if (ClipChildren)
   {
      CurrentWindow = Window->spwndChild;
      while (CurrentWindow && CurrentWindow->Wnd)
      {
         CurrentWnd = CurrentWindow->Wnd;
         if ((CurrentWnd->style & WS_VISIBLE) &&
             !(CurrentWnd->ExStyle & WS_EX_TRANSPARENT))
         {
            ClipRgn = IntSysCreateRectRgnIndirect(&CurrentWnd->rcWindow);
            /* Combine it with the window region if available */
            if (CurrentWindow->hrgnClip && !(CurrentWnd->style & WS_MINIMIZE))
            {
               NtGdiOffsetRgn(ClipRgn, -CurrentWnd->rcWindow.left, -CurrentWnd->rcWindow.top);
               NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentWindow->hrgnClip, RGN_AND);
               NtGdiOffsetRgn(ClipRgn, CurrentWnd->rcWindow.left, CurrentWnd->rcWindow.top);
            }
            NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
            REGION_FreeRgnByHandle(ClipRgn);
         }
         CurrentWindow = CurrentWindow->spwndNext;
      }
   }

   if (Window->hrgnClip && !(Wnd->style & WS_MINIMIZE))
   {
      NtGdiOffsetRgn(VisRgn, -Wnd->rcWindow.left, -Wnd->rcWindow.top);
      NtGdiCombineRgn(VisRgn, VisRgn, Window->hrgnClip, RGN_AND);
      NtGdiOffsetRgn(VisRgn, Wnd->rcWindow.left, Wnd->rcWindow.top);
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
   PWND Wnd, ParentWnd;

   ASSERT_REFS_CO(Window);

   Wnd = Window->Wnd;

   Temp = IntSysCreateRectRgn(0, 0, 0, 0);
   NtGdiCombineRgn(Temp, NewlyExposed, NULL, RGN_COPY);

   Parent = Window->spwndParent;
   if(Parent)
   {
      ParentWnd = Parent->Wnd;
      NtGdiOffsetRgn(Temp,
                     Wnd->rcWindow.left - ParentWnd->rcClient.left,
                     Wnd->rcWindow.top - ParentWnd->rcClient.top);

      UserRefObjectCo(Parent, &Ref);
      co_UserRedrawWindow(Parent, NULL, Temp,
                          RDW_FRAME | RDW_ERASE | RDW_INVALIDATE |
                          RDW_ALLCHILDREN);
      UserDerefObjectCo(Parent);
   }
   REGION_FreeRgnByHandle(Temp);
}

/* EOF */
