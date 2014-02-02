/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Visibility computations
 * FILE:             subsys/win32k/ntuser/vis.c
 * PROGRAMMER:       Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserWinpos);

HRGN FASTCALL
VIS_ComputeVisibleRegion(
   PWND Wnd,
   BOOLEAN ClientArea,
   BOOLEAN ClipChildren,
   BOOLEAN ClipSiblings)
{
   HRGN VisRgn, ClipRgn;
   PWND PreviousWindow, CurrentWindow, CurrentSibling;

   if (!Wnd || !(Wnd->style & WS_VISIBLE))
   {
      return NULL;
   }

   VisRgn = NULL;

   if (ClientArea)
   {
      VisRgn = IntSysCreateRectRgnIndirect(&Wnd->rcClient);
   }
   else
   {
      VisRgn = IntSysCreateRectRgnIndirect(&Wnd->rcWindow);
   }

   /*
    * Walk through all parent windows and for each clip the visble region
    * to the parent's client area and exclude all siblings that are over
    * our window.
    */

   PreviousWindow = Wnd;
   CurrentWindow = Wnd->spwndParent;
   while (CurrentWindow)
   {
      if (!VerifyWnd(CurrentWindow))
      {
         ERR("ATM the Current Window or Parent is dead! %p\n",CurrentWindow);
         if (VisRgn) GreDeleteObject(VisRgn);
         return NULL;
      }

      if (!(CurrentWindow->style & WS_VISIBLE))
      {
         if (VisRgn) GreDeleteObject(VisRgn);
         return NULL;
      }

      ClipRgn = IntSysCreateRectRgnIndirect(&CurrentWindow->rcClient);
      NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_AND);
      GreDeleteObject(ClipRgn);

      if ((PreviousWindow->style & WS_CLIPSIBLINGS) ||
          (PreviousWindow == Wnd && ClipSiblings))
      {
         CurrentSibling = CurrentWindow->spwndChild;
         while ( CurrentSibling != NULL &&
                 CurrentSibling != PreviousWindow )
         {
            if ((CurrentSibling->style & WS_VISIBLE) &&
                !(CurrentSibling->ExStyle & WS_EX_TRANSPARENT))
            {
               ClipRgn = IntSysCreateRectRgnIndirect(&CurrentSibling->rcWindow);
               /* Combine it with the window region if available */
               if (CurrentSibling->hrgnClip && !(CurrentSibling->style & WS_MINIMIZE))
               {
                  NtGdiOffsetRgn(ClipRgn, -CurrentSibling->rcWindow.left, -CurrentSibling->rcWindow.top);
                  NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentSibling->hrgnClip, RGN_AND);
                  NtGdiOffsetRgn(ClipRgn, CurrentSibling->rcWindow.left, CurrentSibling->rcWindow.top);
               }
               NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
               GreDeleteObject(ClipRgn);
            }
            CurrentSibling = CurrentSibling->spwndNext;
         }
      }

      PreviousWindow = CurrentWindow;
      CurrentWindow = CurrentWindow->spwndParent;
   }

   if (ClipChildren)
   {
      CurrentWindow = Wnd->spwndChild;
      while (CurrentWindow)
      {
         if ((CurrentWindow->style & WS_VISIBLE) &&
             !(CurrentWindow->ExStyle & WS_EX_TRANSPARENT))
         {
            ClipRgn = IntSysCreateRectRgnIndirect(&CurrentWindow->rcWindow);
            /* Combine it with the window region if available */
            if (CurrentWindow->hrgnClip && !(CurrentWindow->style & WS_MINIMIZE))
            {
               NtGdiOffsetRgn(ClipRgn, -CurrentWindow->rcWindow.left, -CurrentWindow->rcWindow.top);
               NtGdiCombineRgn(ClipRgn, ClipRgn, CurrentWindow->hrgnClip, RGN_AND);
               NtGdiOffsetRgn(ClipRgn, CurrentWindow->rcWindow.left, CurrentWindow->rcWindow.top);
            }
            NtGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
            GreDeleteObject(ClipRgn);
         }
         CurrentWindow = CurrentWindow->spwndNext;
      }
   }

   if (Wnd->hrgnClip && !(Wnd->style & WS_MINIMIZE))
   {
      NtGdiOffsetRgn(VisRgn, -Wnd->rcWindow.left, -Wnd->rcWindow.top);
      NtGdiCombineRgn(VisRgn, VisRgn, Wnd->hrgnClip, RGN_AND);
      NtGdiOffsetRgn(VisRgn, Wnd->rcWindow.left, Wnd->rcWindow.top);
   }

   return VisRgn;
}

VOID FASTCALL
co_VIS_WindowLayoutChanged(
   PWND Wnd,
   HRGN NewlyExposed)
{
   HRGN Temp;
   PWND Parent;
   USER_REFERENCE_ENTRY Ref;

   ASSERT_REFS_CO(Wnd);

   Parent = Wnd->spwndParent;
   if(Parent)
   {
      Temp = IntSysCreateRectRgn(0, 0, 0, 0);

      NtGdiCombineRgn(Temp, NewlyExposed, NULL, RGN_COPY);
      NtGdiOffsetRgn(Temp,
                     Wnd->rcWindow.left - Parent->rcClient.left,
                     Wnd->rcWindow.top - Parent->rcClient.top);

      UserRefObjectCo(Parent, &Ref);
      co_UserRedrawWindow(Parent, NULL, Temp,
                          RDW_FRAME | RDW_ERASE | RDW_INVALIDATE |
                          RDW_ALLCHILDREN);
      UserDerefObjectCo(Parent);

      GreDeleteObject(Temp);
   }
}

/* EOF */
