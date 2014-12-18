/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Visibility computations
 * FILE:             subsys/win32k/ntuser/vis.c
 * PROGRAMMER:       Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserWinpos);

PREGION FASTCALL
VIS_ComputeVisibleRegion(
   PWND Wnd,
   BOOLEAN ClientArea,
   BOOLEAN ClipChildren,
   BOOLEAN ClipSiblings)
{
   PREGION VisRgn, ClipRgn;
   PWND PreviousWindow, CurrentWindow, CurrentSibling;

   if (!Wnd || !(Wnd->style & WS_VISIBLE))
   {
      return NULL;
   }

   VisRgn = NULL;

   if (ClientArea)
   {
      VisRgn = IntSysCreateRectpRgnIndirect(&Wnd->rcClient);
   }
   else
   {
      VisRgn = IntSysCreateRectpRgnIndirect(&Wnd->rcWindow);
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
         if (VisRgn)
             REGION_Delete(VisRgn);
         return NULL;
      }

      if (!(CurrentWindow->style & WS_VISIBLE))
      {
         if (VisRgn)
             REGION_Delete(VisRgn);
         return NULL;
      }

      ClipRgn = IntSysCreateRectpRgnIndirect(&CurrentWindow->rcClient);
      IntGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_AND);
      REGION_Delete(ClipRgn);

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
               ClipRgn = IntSysCreateRectpRgnIndirect(&CurrentSibling->rcWindow);
               /* Combine it with the window region if available */
               if (CurrentSibling->hrgnClip && !(CurrentSibling->style & WS_MINIMIZE))
               {
                  PREGION SiblingClipRgn = RGNOBJAPI_Lock(CurrentSibling->hrgnClip, NULL);
                  if (SiblingClipRgn)
                  {
                      REGION_bOffsetRgn(ClipRgn, -CurrentSibling->rcWindow.left, -CurrentSibling->rcWindow.top);
                      IntGdiCombineRgn(ClipRgn, ClipRgn, SiblingClipRgn, RGN_AND);
                      REGION_bOffsetRgn(ClipRgn, CurrentSibling->rcWindow.left, CurrentSibling->rcWindow.top);
                      RGNOBJAPI_Unlock(SiblingClipRgn);
                  }
               }
               IntGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
               REGION_Delete(ClipRgn);
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
            ClipRgn = IntSysCreateRectpRgnIndirect(&CurrentWindow->rcWindow);
            /* Combine it with the window region if available */
            if (CurrentWindow->hrgnClip && !(CurrentWindow->style & WS_MINIMIZE))
            {
               PREGION CurrentRgnClip = RGNOBJAPI_Lock(CurrentWindow->hrgnClip, NULL);
               if (CurrentRgnClip)
               {
                   REGION_bOffsetRgn(ClipRgn, -CurrentWindow->rcWindow.left, -CurrentWindow->rcWindow.top);
                   IntGdiCombineRgn(ClipRgn, ClipRgn, CurrentRgnClip, RGN_AND);
                   REGION_bOffsetRgn(ClipRgn, CurrentWindow->rcWindow.left, CurrentWindow->rcWindow.top);
                   RGNOBJAPI_Unlock(CurrentRgnClip);
               }
            }
            IntGdiCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
            REGION_Delete(ClipRgn);
         }
         CurrentWindow = CurrentWindow->spwndNext;
      }
   }

   if (Wnd->hrgnClip && !(Wnd->style & WS_MINIMIZE))
   {
      PREGION WndRgnClip = RGNOBJAPI_Lock(Wnd->hrgnClip, NULL);
      if (WndRgnClip)
      {
          REGION_bOffsetRgn(VisRgn, -Wnd->rcWindow.left, -Wnd->rcWindow.top);
          IntGdiCombineRgn(VisRgn, VisRgn, WndRgnClip, RGN_AND);
          REGION_bOffsetRgn(VisRgn, Wnd->rcWindow.left, Wnd->rcWindow.top);
          RGNOBJAPI_Unlock(WndRgnClip);
      }
   }

   return VisRgn;
}

VOID FASTCALL
co_VIS_WindowLayoutChanged(
   PWND Wnd,
   PREGION NewlyExposed)
{
   PWND Parent;
   USER_REFERENCE_ENTRY Ref;

   ASSERT_REFS_CO(Wnd);

   Parent = Wnd->spwndParent;
   if(Parent)
   {
       PREGION TempRgn = IntSysCreateRectpRgn(0, 0, 0, 0);

       if (!TempRgn)
           return;

       IntGdiCombineRgn(TempRgn, NewlyExposed, NULL, RGN_COPY);
       REGION_bOffsetRgn(TempRgn,
                         Wnd->rcWindow.left - Parent->rcClient.left,
                         Wnd->rcWindow.top - Parent->rcClient.top);

       UserRefObjectCo(Parent, &Ref);
       co_UserRedrawWindow(Parent, NULL, TempRgn,
                           RDW_FRAME | RDW_ERASE | RDW_INVALIDATE |
                           RDW_ALLCHILDREN);
       UserDerefObjectCo(Parent);

       REGION_Delete(TempRgn);
   }
}

/* EOF */
