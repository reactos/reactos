/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Window painting function
 *  FILE:             win32ss/user/ntuser/painting.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserPainting);

BOOL UserExtTextOutW(HDC hdc, INT x, INT y, UINT flags, PRECTL lprc,
                     LPCWSTR lpString, UINT count);

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @name IntIntersectWithParents
 *
 * Intersect window rectangle with all parent client rectangles.
 *
 * @param Child
 *        Pointer to child window to start intersecting from.
 * @param WindowRect
 *        Pointer to rectangle that we want to intersect in screen
 *        coordinates on input and intersected rectangle on output (if TRUE
 *        is returned).
 *
 * @return
 *    If any parent is minimized or invisible or the resulting rectangle
 *    is empty then FALSE is returned. Otherwise TRUE is returned.
 */

BOOL FASTCALL
IntIntersectWithParents(PWND Child, RECTL *WindowRect)
{
   PWND ParentWnd;

   if (Child->ExStyle & WS_EX_REDIRECTED)
      return TRUE;

   ParentWnd = Child->spwndParent;
   while (ParentWnd != NULL)
   {
      if (!(ParentWnd->style & WS_VISIBLE)  ||
           (ParentWnd->style & WS_MINIMIZE) ||
          !RECTL_bIntersectRect(WindowRect, WindowRect, &ParentWnd->rcClient) )
      {
         return FALSE;
      }

      if (ParentWnd->ExStyle & WS_EX_REDIRECTED)
         return TRUE;

      ParentWnd = ParentWnd->spwndParent;
   }

   return TRUE;
}

BOOL FASTCALL
IntValidateParents(PWND Child, BOOL Recurse)
{
   RECTL ParentRect, Rect;
   BOOL Start, Ret = TRUE;
   PWND ParentWnd = Child;
   PREGION Rgn = NULL;

   if (ParentWnd->style & WS_CHILD)
   {
      do
         ParentWnd = ParentWnd->spwndParent;
      while (ParentWnd->style & WS_CHILD);
   }

   // No pending nonclient paints.
   if (!(ParentWnd->state & WNDS_SYNCPAINTPENDING)) Recurse = FALSE;

   Start = TRUE;
   ParentWnd = Child->spwndParent;
   while (ParentWnd)
   {
      if (ParentWnd->style & WS_CLIPCHILDREN)
         break;

      if (ParentWnd->hrgnUpdate != 0)
      {
         if (Recurse)
         {
            Ret = FALSE;
            break;
         }
         // Start with child clipping.
         if (Start)
         {
            Start = FALSE;

            Rect = Child->rcWindow;

            if (!IntIntersectWithParents(Child, &Rect)) break;

            Rgn = IntSysCreateRectpRgnIndirect(&Rect);

            if (Child->hrgnClip)
            {
               PREGION RgnClip = REGION_LockRgn(Child->hrgnClip);
               IntGdiCombineRgn(Rgn, Rgn, RgnClip, RGN_AND);
               REGION_UnlockRgn(RgnClip);
            }
         }

         ParentRect = ParentWnd->rcWindow;

         if (!IntIntersectWithParents(ParentWnd, &ParentRect)) break;

         IntInvalidateWindows( ParentWnd,
                               Rgn,
                               RDW_VALIDATE | RDW_NOCHILDREN | RDW_NOUPDATEDIRTY);
      }
      ParentWnd = ParentWnd->spwndParent;
   }

   if (Rgn) REGION_Delete(Rgn);

   return Ret;
}

/*
  Synchronize painting to the top-level windows of other threads.
*/
VOID FASTCALL
IntSendSyncPaint(PWND Wnd, ULONG Flags)
{
   PTHREADINFO ptiCur, ptiWnd;
   PUSER_SENT_MESSAGE Message;
   PLIST_ENTRY Entry;
   BOOL bSend = TRUE;

   ptiWnd = Wnd->head.pti;
   ptiCur = PsGetCurrentThreadWin32Thread();
   /*
      Not the current thread, Wnd is in send Nonclient paint also in send erase background and it is visiable.
   */
   if ( Wnd->head.pti != ptiCur &&
        Wnd->state & WNDS_SENDNCPAINT &&
        Wnd->state & WNDS_SENDERASEBACKGROUND &&
        Wnd->style & WS_VISIBLE)
   {
      // For testing, if you see this, break out the Champagne and have a party!
      TRACE("SendSyncPaint Wnd in State!\n");
      if (!IsListEmpty(&ptiWnd->SentMessagesListHead))
      {
         // Scan sent queue messages to see if we received sync paint messages.
         Entry = ptiWnd->SentMessagesListHead.Flink;
         Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);
         do
         {
            ERR("LOOP it\n");
            if (Message->Msg.message == WM_SYNCPAINT &&
                Message->Msg.hwnd == UserHMGetHandle(Wnd))
            {  // Already received so exit out.
                ERR("SendSyncPaint Found one in the Sent Msg Queue!\n");
                bSend = FALSE;
                break;
            }
            Entry = Message->ListEntry.Flink;
            Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);
         }
         while (Entry != &ptiWnd->SentMessagesListHead);
      }
      if (bSend)
      {
         TRACE("Sending WM_SYNCPAINT\n");
         // This message has no parameters. But it does! Pass Flags along.
         co_IntSendMessageNoWait(UserHMGetHandle(Wnd), WM_SYNCPAINT, Flags, 0);
         Wnd->state |= WNDS_SYNCPAINTPENDING;
      }
   }

   // Send to all the children if this is the desktop window.
   if (UserIsDesktopWindow(Wnd))
   {
      if ( Flags & RDW_ALLCHILDREN ||
          ( !(Flags & RDW_NOCHILDREN) && Wnd->style & WS_CLIPCHILDREN))
      {
         PWND spwndChild = Wnd->spwndChild;
         while(spwndChild)
         {
            if ( spwndChild->style & WS_CHILD &&
                 spwndChild->head.pti != ptiCur)
            {
               spwndChild = spwndChild->spwndNext;
               continue;
            }
            IntSendSyncPaint( spwndChild, Flags );
            spwndChild = spwndChild->spwndNext;
         }
      }
   }
}

/*
 * @name IntCalcWindowRgn
 *
 * Get a window or client region.
 */

HRGN FASTCALL
IntCalcWindowRgn(PWND Wnd, BOOL Client)
{
   HRGN hRgnWindow;

   if (Client)
   {
      hRgnWindow = NtGdiCreateRectRgn(
          Wnd->rcClient.left,
          Wnd->rcClient.top,
          Wnd->rcClient.right,
          Wnd->rcClient.bottom);
   }
   else
   {
      hRgnWindow = NtGdiCreateRectRgn(
          Wnd->rcWindow.left,
          Wnd->rcWindow.top,
          Wnd->rcWindow.right,
          Wnd->rcWindow.bottom);
   }

   if (Wnd->hrgnClip != NULL && !(Wnd->style & WS_MINIMIZE))
   {
      NtGdiOffsetRgn(hRgnWindow,
         -Wnd->rcWindow.left,
         -Wnd->rcWindow.top);
      NtGdiCombineRgn(hRgnWindow, hRgnWindow, Wnd->hrgnClip, RGN_AND);
      NtGdiOffsetRgn(hRgnWindow,
         Wnd->rcWindow.left,
         Wnd->rcWindow.top);
   }

   return hRgnWindow;
}

/*
 * @name IntGetNCUpdateRgn
 *
 * Get non-client update region of a window and optionally validate it.
 *
 * @param Window
 *        Pointer to window to get the NC update region from.
 * @param Validate
 *        Set to TRUE to force validating the NC update region.
 *
 * @return
 *    Handle to NC update region. The caller is responsible for deleting
 *    it.
 */

HRGN FASTCALL
IntGetNCUpdateRgn(PWND Window, BOOL Validate)
{
   HRGN hRgnNonClient;
   HRGN hRgnWindow;
   UINT RgnType, NcType;
   RECT update;

   if (Window->hrgnUpdate != NULL &&
       Window->hrgnUpdate != HRGN_WINDOW)
   {
      hRgnNonClient = IntCalcWindowRgn(Window, FALSE);

      /*
       * If region creation fails it's safe to fallback to whole
       * window region.
       */
      if (hRgnNonClient == NULL)
      {
         return HRGN_WINDOW;
      }

      hRgnWindow = IntCalcWindowRgn(Window, TRUE);
      if (hRgnWindow == NULL)
      {
         GreDeleteObject(hRgnNonClient);
         return HRGN_WINDOW;
      }

      NcType = IntGdiGetRgnBox(hRgnNonClient, &update);

      RgnType = NtGdiCombineRgn(hRgnNonClient, hRgnNonClient, hRgnWindow, RGN_DIFF);

      if (RgnType == ERROR)
      {
         GreDeleteObject(hRgnWindow);
         GreDeleteObject(hRgnNonClient);
         return HRGN_WINDOW;
      }
      else if (RgnType == NULLREGION)
      {
         GreDeleteObject(hRgnWindow);
         GreDeleteObject(hRgnNonClient);
         Window->state &= ~WNDS_UPDATEDIRTY;
         return NULL;
      }

      /*
       * Remove the nonclient region from the standard update region if
       * we were asked for it.
       */

      if (Validate)
      {
         if (NtGdiCombineRgn(Window->hrgnUpdate, Window->hrgnUpdate, hRgnWindow, RGN_AND) == NULLREGION)
         {
            IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
            GreDeleteObject(Window->hrgnUpdate);
            Window->state &= ~WNDS_UPDATEDIRTY;
            Window->hrgnUpdate = NULL;
            if (!(Window->state & WNDS_INTERNALPAINT))
               MsqDecPaintCountQueue(Window->head.pti);
         }
      }

      /* check if update rgn contains complete nonclient area */
      if (NcType == SIMPLEREGION)
      {
         RECT window;
         IntGetWindowRect( Window, &window );

         if (IntEqualRect( &window, &update ))
         {
            GreDeleteObject(hRgnNonClient);
            hRgnNonClient = HRGN_WINDOW;
         }
      }

      GreDeleteObject(hRgnWindow);

      return hRgnNonClient;
   }
   else
   {
      return Window->hrgnUpdate;
   }
}

VOID FASTCALL
IntSendNCPaint(PWND pWnd, HRGN hRgn)
{
   pWnd->state &= ~WNDS_SENDNCPAINT;

   if ( pWnd == GetW32ThreadInfo()->MessageQueue->spwndActive &&
       !(pWnd->state & WNDS_ACTIVEFRAME))
   {
      pWnd->state |= WNDS_ACTIVEFRAME;
      pWnd->state &= ~WNDS_NONCPAINT;
      hRgn = HRGN_WINDOW;
   }

   if (pWnd->state2 & WNDS2_FORCEFULLNCPAINTCLIPRGN)
   {
      pWnd->state2 &= ~WNDS2_FORCEFULLNCPAINTCLIPRGN;
      hRgn = HRGN_WINDOW;
   }

   if (hRgn) co_IntSendMessage(UserHMGetHandle(pWnd), WM_NCPAINT, (WPARAM)hRgn, 0);
}

VOID FASTCALL
IntSendChildNCPaint(PWND pWnd)
{
    pWnd = pWnd->spwndChild;
    while (pWnd)
    {
        if ((pWnd->hrgnUpdate == NULL) && (pWnd->state & WNDS_SENDNCPAINT))
        {
            PWND Next;
            USER_REFERENCE_ENTRY Ref;

            /* Reference, IntSendNCPaint leaves win32k */
            UserRefObjectCo(pWnd, &Ref);
            IntSendNCPaint(pWnd, HRGN_WINDOW);

            /* Make sure to grab next one before dereferencing/freeing */
            Next = pWnd->spwndNext;
            UserDerefObjectCo(pWnd);
            pWnd = Next;
        }
        else
        {
            pWnd = pWnd->spwndNext;
        }
    }
}

/*
 * IntPaintWindows
 *
 * Internal function used by IntRedrawWindow.
 */

VOID FASTCALL
co_IntPaintWindows(PWND Wnd, ULONG Flags, BOOL Recurse)
{
   HDC hDC;
   HWND hWnd = Wnd->head.h;
   HRGN TempRegion = NULL;

   Wnd->state &= ~WNDS_PAINTNOTPROCESSED;

   if (Wnd->state & WNDS_SENDNCPAINT ||
       Wnd->state & WNDS_SENDERASEBACKGROUND)
   {
      if (!(Wnd->style & WS_VISIBLE))
      {
         Wnd->state &= ~(WNDS_SENDNCPAINT|WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
         return;
      }
      else
      {
         if (Wnd->hrgnUpdate == NULL)
         {
            Wnd->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
         }

         if (Wnd->head.pti == PsGetCurrentThreadWin32Thread())
         {
            if (Wnd->state & WNDS_SENDNCPAINT)
            {
               TempRegion = IntGetNCUpdateRgn(Wnd, TRUE);

               IntSendNCPaint(Wnd, TempRegion);

               if (TempRegion > HRGN_WINDOW && GreIsHandleValid(TempRegion))
               {
                  /* NOTE: The region can already be deleted! */
                  GreDeleteObject(TempRegion);
               }
            }

            if (Wnd->state & WNDS_SENDERASEBACKGROUND)
            {
               PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
               if (Wnd->hrgnUpdate)
               {
                  hDC = UserGetDCEx( Wnd,
                                     Wnd->hrgnUpdate,
                                     DCX_CACHE|DCX_USESTYLE|DCX_INTERSECTRGN|DCX_KEEPCLIPRGN);

                  if (Wnd->head.pti->ppi != pti->ppi)
                  {
                     ERR("Sending DC to another Process!!!\n");
                  }

                  Wnd->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
                  // Kill the loop, so Clear before we send.
                  if (!co_IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0))
                  {
                     Wnd->state |= (WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
                  }
                  UserReleaseDC(Wnd, hDC, FALSE);
               }
            }
         }

      }
   }

   /*
    * Check that the window is still valid at this point
    */
   if (!IntIsWindow(hWnd))
   {
      return;
   }

   /*
    * Paint child windows.
    */

   if (!(Flags & RDW_NOCHILDREN) &&
       !(Wnd->style & WS_MINIMIZE) &&
        ( Flags & RDW_ALLCHILDREN ||
         (Flags & RDW_CLIPCHILDREN && Wnd->style & WS_CLIPCHILDREN) ) )
   {
      HWND *List, *phWnd;
      PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

      if ((List = IntWinListChildren(Wnd)))
      {
         for (phWnd = List; *phWnd; ++phWnd)
         {
            if ((Wnd = UserGetWindowObject(*phWnd)) == NULL)
               continue;

            if (Wnd->head.pti != pti && Wnd->style & WS_CHILD)
               continue;

            if (Wnd->style & WS_VISIBLE)
            {
               USER_REFERENCE_ENTRY Ref;
               UserRefObjectCo(Wnd, &Ref);
               co_IntPaintWindows(Wnd, Flags, TRUE);
               UserDerefObjectCo(Wnd);
            }
         }
         ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
      }
   }
}

/*
 * IntUpdateWindows
 *
 * Internal function used by IntRedrawWindow, simplecall.
 */

VOID FASTCALL
co_IntUpdateWindows(PWND Wnd, ULONG Flags, BOOL Recurse)
{
   HWND hWnd = Wnd->head.h;

   if ( Wnd->hrgnUpdate != NULL || Wnd->state & WNDS_INTERNALPAINT )
   {
      if (Wnd->hrgnUpdate)
      {
         if (!IntValidateParents(Wnd, Recurse))
         {
            return;
         }
      }

      if (Wnd->state & WNDS_INTERNALPAINT)
      {
          Wnd->state &= ~WNDS_INTERNALPAINT;

          if (Wnd->hrgnUpdate == NULL)
             MsqDecPaintCountQueue(Wnd->head.pti);
      }

      Wnd->state |= WNDS_PAINTNOTPROCESSED;
      Wnd->state &= ~WNDS_UPDATEDIRTY;

      Wnd->state2 |= WNDS2_WMPAINTSENT;
      co_IntSendMessage(hWnd, WM_PAINT, 0, 0);

      if (Wnd->state & WNDS_PAINTNOTPROCESSED)
      {
         USER_REFERENCE_ENTRY Ref;
         UserRefObjectCo(Wnd, &Ref);
         co_IntPaintWindows(Wnd, RDW_NOCHILDREN, FALSE);
         UserDerefObjectCo(Wnd);
      }
   }

   // Force flags as a toggle. Fixes msg:test_paint_messages:WmChildPaintNc.
   Flags = (Flags & RDW_NOCHILDREN) ? RDW_NOCHILDREN : RDW_ALLCHILDREN; // All children is the default.

  /*
   * Update child windows.
   */

   if (!(Flags & RDW_NOCHILDREN)  &&
        (Flags & RDW_ALLCHILDREN) &&
        !UserIsDesktopWindow(Wnd))
   {
      PWND Child;

      for (Child = Wnd->spwndChild; Child; Child = Child->spwndNext)
      {
         /* transparent window, check for non-transparent sibling to paint first, then skip it */
         if ( Child->ExStyle & WS_EX_TRANSPARENT &&
             ( Child->hrgnUpdate != NULL || Child->state & WNDS_INTERNALPAINT ) )
         {
            PWND Next = Child->spwndNext;
            while (Next)
            {
               if ( Next->hrgnUpdate != NULL || Next->state & WNDS_INTERNALPAINT ) break;

               Next = Next->spwndNext;
            }

            if (Next) continue;
         }

         if (Child->style & WS_VISIBLE)
         {
             USER_REFERENCE_ENTRY Ref;
             UserRefObjectCo(Child, &Ref);
             co_IntUpdateWindows(Child, Flags, TRUE);
             UserDerefObjectCo(Child);
         }
      }
   }
}

VOID FASTCALL
UserUpdateWindows(PWND pWnd, ULONG Flags)
{
   // If transparent and any sibling windows below needs to be painted, leave.
   if (pWnd->ExStyle & WS_EX_TRANSPARENT)
   {
      PWND Next = pWnd->spwndNext;

      while(Next)
      {
         if ( Next->head.pti == pWnd->head.pti &&
            ( Next->hrgnUpdate != NULL || Next->state & WNDS_INTERNALPAINT) )
         {
            return;
         }

         Next = Next->spwndNext;
      }
   }
   co_IntUpdateWindows(pWnd, Flags, FALSE);
}

VOID FASTCALL
UserSyncAndPaintWindows(PWND pWnd, ULONG Flags)
{
   PWND Parent = pWnd;
   // Find parent, if it needs to be painted, leave.
   while(TRUE)
   {
      if ((Parent = Parent->spwndParent) == NULL) break;
      if ( Parent->style & WS_CLIPCHILDREN ) break;
      if ( Parent->hrgnUpdate != NULL || Parent->state & WNDS_INTERNALPAINT ) return;
   }

   IntSendSyncPaint(pWnd, Flags);
   co_IntPaintWindows(pWnd, Flags, FALSE);
}

/*
 * IntInvalidateWindows
 *
 * Internal function used by IntRedrawWindow, UserRedrawDesktop,
 * co_WinPosSetWindowPos, co_UserRedrawWindow.
 */
VOID FASTCALL
IntInvalidateWindows(PWND Wnd, PREGION Rgn, ULONG Flags)
{
   INT RgnType = NULLREGION;
   BOOL HadPaintMessage;

   TRACE("IntInvalidateWindows start Rgn %p\n",Rgn);

   if ( Rgn > PRGN_WINDOW )
   {
      /*
       * If the nonclient is not to be redrawn, clip the region to the client
       * rect
       */
      if ((Flags & RDW_INVALIDATE) != 0 && (Flags & RDW_FRAME) == 0)
      {
         PREGION RgnClient;

         RgnClient = IntSysCreateRectpRgnIndirect(&Wnd->rcClient);
         if (RgnClient)
         {
             RgnType = IntGdiCombineRgn(Rgn, Rgn, RgnClient, RGN_AND);
             REGION_Delete(RgnClient);
         }
      }

      /*
       * Clip the given region with window rectangle (or region)
       */

      if (!Wnd->hrgnClip || (Wnd->style & WS_MINIMIZE))
      {
         PREGION RgnWindow = IntSysCreateRectpRgnIndirect(&Wnd->rcWindow);
         if (RgnWindow)
         {
             RgnType = IntGdiCombineRgn(Rgn, Rgn, RgnWindow, RGN_AND);
             REGION_Delete(RgnWindow);
         }
      }
      else
      {
          PREGION RgnClip = REGION_LockRgn(Wnd->hrgnClip);
          if (RgnClip)
          {
              REGION_bOffsetRgn(Rgn,
                                -Wnd->rcWindow.left,
                                -Wnd->rcWindow.top);
              RgnType = IntGdiCombineRgn(Rgn, Rgn, RgnClip, RGN_AND);
              REGION_bOffsetRgn(Rgn,
                                Wnd->rcWindow.left,
                                Wnd->rcWindow.top);
              REGION_UnlockRgn(RgnClip);
          }
      }
   }
   else
   {
      RgnType = NULLREGION;
   }

   /*
    * Save current state of pending updates
    */

   HadPaintMessage = IntIsWindowDirty(Wnd);

   /*
    * Update the region and flags
    */

   // The following flags are used to invalidate the window.
   if (Flags & (RDW_INVALIDATE|RDW_INTERNALPAINT|RDW_ERASE|RDW_FRAME))
   {
      if (Flags & RDW_INTERNALPAINT)
      {
         Wnd->state |= WNDS_INTERNALPAINT;
      }

      if (Flags & RDW_INVALIDATE )
      {
         PREGION RgnUpdate;

         Wnd->state &= ~WNDS_NONCPAINT;

         /* If not the same thread set it dirty. */
         if (Wnd->head.pti != PsGetCurrentThreadWin32Thread())
         {
            Wnd->state |= WNDS_UPDATEDIRTY;
            if (Wnd->state2 & WNDS2_WMPAINTSENT)
               Wnd->state2 |= WNDS2_ENDPAINTINVALIDATE;
         }

         if (Flags & RDW_FRAME)
            Wnd->state |= WNDS_SENDNCPAINT;

         if (Flags & RDW_ERASE)
            Wnd->state |= WNDS_SENDERASEBACKGROUND;

         if (RgnType != NULLREGION && Rgn > PRGN_WINDOW)
         {
            if (Wnd->hrgnUpdate == NULL)
            {
               Wnd->hrgnUpdate = NtGdiCreateRectRgn(0, 0, 0, 0);
               IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_PUBLIC);
            }

            if (Wnd->hrgnUpdate != HRGN_WINDOW)
            {
               RgnUpdate = REGION_LockRgn(Wnd->hrgnUpdate);
               if (RgnUpdate)
               {
                  RgnType = IntGdiCombineRgn(RgnUpdate, RgnUpdate, Rgn, RGN_OR);
                  REGION_UnlockRgn(RgnUpdate);
                  if (RgnType == NULLREGION)
                  {
                     IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
                     GreDeleteObject(Wnd->hrgnUpdate);
                     Wnd->hrgnUpdate = NULL;
                  }
               }
            }
         }

         Flags |= RDW_ERASE|RDW_FRAME; // For children.

      }

      if (!HadPaintMessage && IntIsWindowDirty(Wnd))
      {
         MsqIncPaintCountQueue(Wnd->head.pti);
      }

   }    // The following flags are used to validate the window.
   else if (Flags & (RDW_VALIDATE|RDW_NOINTERNALPAINT|RDW_NOERASE|RDW_NOFRAME))
   {
      if (Wnd->state & WNDS_UPDATEDIRTY && !(Flags & RDW_NOUPDATEDIRTY))
         return;

      if (Flags & RDW_NOINTERNALPAINT)
      {
         Wnd->state &= ~WNDS_INTERNALPAINT;
      }

      if (Flags & RDW_VALIDATE)
      {
         if (Flags & RDW_NOFRAME)
            Wnd->state &= ~WNDS_SENDNCPAINT;

         if (Flags & RDW_NOERASE)
            Wnd->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);

         if (Wnd->hrgnUpdate > HRGN_WINDOW && RgnType != NULLREGION && Rgn > PRGN_WINDOW)
         {
             PREGION RgnUpdate = REGION_LockRgn(Wnd->hrgnUpdate);

             if (RgnUpdate)
             {
                 RgnType = IntGdiCombineRgn(RgnUpdate, RgnUpdate, Rgn, RGN_DIFF);
                 REGION_UnlockRgn(RgnUpdate);

                 if (RgnType == NULLREGION)
                 {
                     IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
                     GreDeleteObject(Wnd->hrgnUpdate);
                     Wnd->hrgnUpdate = NULL;
                 }
             }
         }
         // If update is null, do not erase.
         if (Wnd->hrgnUpdate == NULL)
         {
            Wnd->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
         }
      }

      if (HadPaintMessage && !IntIsWindowDirty(Wnd))
      {
         MsqDecPaintCountQueue(Wnd->head.pti);
      }
   }

   /*
    * Process children if needed
    */

   if (!(Flags & RDW_NOCHILDREN) &&
       !(Wnd->style & WS_MINIMIZE) &&
         ((Flags & RDW_ALLCHILDREN) || !(Wnd->style & WS_CLIPCHILDREN)))
   {
      PWND Child;

      for (Child = Wnd->spwndChild; Child; Child = Child->spwndNext)
      {
         if (Child->style & WS_VISIBLE)
         {
            /*
             * Recursive call to update children hrgnUpdate
             */
            PREGION RgnTemp = IntSysCreateRectpRgn(0, 0, 0, 0);
            if (RgnTemp)
            {
                if (Rgn > PRGN_WINDOW) IntGdiCombineRgn(RgnTemp, Rgn, 0, RGN_COPY);
                IntInvalidateWindows(Child, ((Rgn > PRGN_WINDOW)?RgnTemp:Rgn), Flags);
                REGION_Delete(RgnTemp);
            }
         }
      }
   }
   TRACE("IntInvalidateWindows exit\n");
}

/*
 * IntIsWindowDrawable
 *
 * Remarks
 *    Window is drawable when it is visible and all parents are not
 *    minimized.
 */

BOOL FASTCALL
IntIsWindowDrawable(PWND Wnd)
{
   PWND WndObject;

   for (WndObject = Wnd; WndObject != NULL; WndObject = WndObject->spwndParent)
   {
      if ( WndObject->state2 & WNDS2_INDESTROY ||
           WndObject->state & WNDS_DESTROYED ||
           !WndObject ||
           !(WndObject->style & WS_VISIBLE) ||
            ((WndObject->style & WS_MINIMIZE) && (WndObject != Wnd)))
      {
         return FALSE;
      }
   }

   return TRUE;
}

/*
 * IntRedrawWindow
 *
 * Internal version of NtUserRedrawWindow that takes WND as
 * first parameter.
 */

BOOL FASTCALL
co_UserRedrawWindow(
   PWND Window,
   const RECTL* UpdateRect,
   PREGION UpdateRgn,
   ULONG Flags)
{
   PREGION TmpRgn = NULL;
   TRACE("co_UserRedrawWindow start Rgn %p\n",UpdateRgn);

   /*
    * Step 1.
    * Validation of passed parameters.
    */

   if (!IntIsWindowDrawable(Window))
   {
      return TRUE; // Just do nothing!!!
   }

   if (Window == NULL)
   {
      Window = UserGetDesktopWindow();
   }

   /*
    * Step 2.
    * Transform the parameters UpdateRgn and UpdateRect into
    * a region hRgn specified in screen coordinates.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE)) // Both are OKAY!
   {
      /* We can't hold lock on GDI objects while doing roundtrips to user mode,
       * so use a copy instead */
      if (UpdateRgn)
      {
          TmpRgn = IntSysCreateRectpRgn(0, 0, 0, 0);

          if (UpdateRgn > PRGN_WINDOW)
          {
             IntGdiCombineRgn(TmpRgn, UpdateRgn, NULL, RGN_COPY);
          }

          if (Window != UserGetDesktopWindow())
          {
             REGION_bOffsetRgn(TmpRgn, Window->rcClient.left, Window->rcClient.top);
          }
      }
      else
      {
         if (UpdateRect != NULL)
         {
            if (Window == UserGetDesktopWindow())
            {
               TmpRgn = IntSysCreateRectpRgnIndirect(UpdateRect);
            }
            else
            {
               TmpRgn = IntSysCreateRectpRgn(Window->rcClient.left + UpdateRect->left,
                                             Window->rcClient.top  + UpdateRect->top,
                                             Window->rcClient.left + UpdateRect->right,
                                             Window->rcClient.top  + UpdateRect->bottom);
            }
         }
         else
         {
            if ((Flags & (RDW_INVALIDATE | RDW_FRAME)) == (RDW_INVALIDATE | RDW_FRAME) ||
                (Flags & (RDW_VALIDATE | RDW_NOFRAME)) == (RDW_VALIDATE | RDW_NOFRAME))
            {
               if (!RECTL_bIsEmptyRect(&Window->rcWindow))
                   TmpRgn = IntSysCreateRectpRgnIndirect(&Window->rcWindow);
            }
            else
            {
               if (!RECTL_bIsEmptyRect(&Window->rcClient))
                   TmpRgn = IntSysCreateRectpRgnIndirect(&Window->rcClient);
            }
         }
      }
   }

   /* Fixes test RDW_INTERNALPAINT behavior */
   if (TmpRgn == NULL)
   {
      TmpRgn = PRGN_WINDOW; // Need a region so the bits can be set!!!
   }

   /*
    * Step 3.
    * Adjust the window update region depending on hRgn and flags.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE | RDW_INTERNALPAINT | RDW_NOINTERNALPAINT) &&
       TmpRgn != NULL)
   {
      IntInvalidateWindows(Window, TmpRgn, Flags);
   }

   /*
    * Step 4.
    * Repaint and erase windows if needed.
    */

   if (Flags & RDW_UPDATENOW)
   {
      UserUpdateWindows(Window, Flags);
   }
   else if (Flags & RDW_ERASENOW)
   {
      if ((Flags & (RDW_NOCHILDREN|RDW_ALLCHILDREN)) == 0)
         Flags |= RDW_CLIPCHILDREN;

      UserSyncAndPaintWindows(Window, Flags);
   }

   /*
    * Step 5.
    * Cleanup ;-)
    */

   if (TmpRgn > PRGN_WINDOW)
   {
      REGION_Delete(TmpRgn);
   }
   TRACE("co_UserRedrawWindow exit\n");

   return TRUE;
}

VOID FASTCALL
PaintSuspendedWindow(PWND pwnd, HRGN hrgnOrig)
{
   if (pwnd->hrgnUpdate)
   {
      HDC hDC;
      INT Flags = DC_NC|DC_NOSENDMSG;
      HRGN hrgnTemp;
      RECT Rect;
      INT type;
      PREGION prgn;

      if (pwnd->hrgnUpdate > HRGN_WINDOW)
      {
         hrgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
         type = NtGdiCombineRgn( hrgnTemp, pwnd->hrgnUpdate, 0, RGN_COPY);
         if (type == ERROR)
         {
            GreDeleteObject(hrgnTemp);
            hrgnTemp = HRGN_WINDOW;
         }
      }
      else
      {
         hrgnTemp = GreCreateRectRgnIndirect(&pwnd->rcWindow);
      }

      if ( hrgnOrig &&
           hrgnTemp > HRGN_WINDOW &&
           NtGdiCombineRgn(hrgnTemp, hrgnTemp, hrgnOrig, RGN_AND) == NULLREGION)
      {
         GreDeleteObject(hrgnTemp);
         return;
      }

      hDC = UserGetDCEx(pwnd, hrgnTemp, DCX_WINDOW|DCX_INTERSECTRGN|DCX_USESTYLE|DCX_KEEPCLIPRGN);

      Rect = pwnd->rcWindow;
      RECTL_vOffsetRect(&Rect, -pwnd->rcWindow.left, -pwnd->rcWindow.top);

      // Clear out client area!
      FillRect(hDC, &Rect, IntGetSysColorBrush(COLOR_WINDOW));

      NC_DoNCPaint(pwnd, hDC, Flags); // Redraw without MENUs.

      UserReleaseDC(pwnd, hDC, FALSE);

      prgn = REGION_LockRgn(hrgnTemp);
      IntInvalidateWindows(pwnd, prgn, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
      REGION_UnlockRgn(prgn);

      // Set updates for this window.
      pwnd->state |= WNDS_SENDNCPAINT|WNDS_SENDERASEBACKGROUND|WNDS_UPDATEDIRTY;

      // DCX_KEEPCLIPRGN is set. Check it anyway.
      if (hrgnTemp > HRGN_WINDOW && GreIsHandleValid(hrgnTemp)) GreDeleteObject(hrgnTemp);
   }
}

VOID FASTCALL
UpdateTheadChildren(PWND pWnd, HRGN hRgn)
{
   PaintSuspendedWindow( pWnd, hRgn );

   if (!(pWnd->style & WS_CLIPCHILDREN))
      return;

   pWnd = pWnd->spwndChild; // invalidate children if any.
   while (pWnd)
   {
      UpdateTheadChildren( pWnd, hRgn );
      pWnd = pWnd->spwndNext;
   }
}

VOID FASTCALL
UpdateThreadWindows(PWND pWnd, PTHREADINFO pti, HRGN hRgn)
{
   PWND pwndTemp;

   for ( pwndTemp = pWnd;
         pwndTemp;
         pwndTemp = pwndTemp->spwndNext )
   {
      if (pwndTemp->head.pti == pti)
      {
          UserUpdateWindows(pwndTemp, RDW_ALLCHILDREN);
      }
      else
      {
          if (IsThreadSuspended(pwndTemp->head.pti) || MsqIsHung(pwndTemp->head.pti, MSQ_HUNG))
          {
             UpdateTheadChildren(pwndTemp, hRgn);
          }
          else
             UserUpdateWindows(pwndTemp, RDW_ALLCHILDREN);
      }
   }
}

BOOL FASTCALL
IntIsWindowDirty(PWND Wnd)
{
   return ( Wnd->style & WS_VISIBLE &&
           ( Wnd->hrgnUpdate != NULL ||
             Wnd->state & WNDS_INTERNALPAINT ) );
}

/*
   Conditions to paint any window:

   1. Update region is not null.
   2. Internal paint flag is set.
   3. Paint count is not zero.

 */
PWND FASTCALL
IntFindWindowToRepaint(PWND Window, PTHREADINFO Thread)
{
   PWND hChild;
   PWND TempWindow;

   for (; Window != NULL; Window = Window->spwndNext)
   {
      if (IntWndBelongsToThread(Window, Thread))
      {
         if (IntIsWindowDirty(Window))
         {
            /* Make sure all non-transparent siblings are already drawn. */
            if (Window->ExStyle & WS_EX_TRANSPARENT)
            {
               for (TempWindow = Window->spwndNext; TempWindow != NULL;
                    TempWindow = TempWindow->spwndNext)
               {
                  if (!(TempWindow->ExStyle & WS_EX_TRANSPARENT) &&
                       IntWndBelongsToThread(TempWindow, Thread) &&
                       IntIsWindowDirty(TempWindow))
                  {
                     return TempWindow;
                  }
               }
            }
            return Window;
         }
      }
      /* find a child of the specified window that needs repainting */
      if (Window->spwndChild)
      {
         hChild = IntFindWindowToRepaint(Window->spwndChild, Thread);
         if (hChild != NULL)
            return hChild;
      }
   }
   return Window;
}

//
// Internal painting of windows.
//
VOID FASTCALL
IntPaintWindow( PWND Window )
{
   // Handle normal painting.
   co_IntPaintWindows( Window, RDW_NOCHILDREN, FALSE );
}

BOOL FASTCALL
IntGetPaintMessage(
   PWND Window,
   UINT MsgFilterMin,
   UINT MsgFilterMax,
   PTHREADINFO Thread,
   MSG *Message,
   BOOL Remove)
{
   PWND PaintWnd, StartWnd;

   if ((MsgFilterMin != 0 || MsgFilterMax != 0) &&
         (MsgFilterMin > WM_PAINT || MsgFilterMax < WM_PAINT))
      return FALSE;

   if (Thread->TIF_flags & TIF_SYSTEMTHREAD )
   {
      ERR("WM_PAINT is in a System Thread!\n");
   }

   StartWnd = UserGetDesktopWindow();
   PaintWnd = IntFindWindowToRepaint(StartWnd, Thread);

   Message->hwnd = PaintWnd ? UserHMGetHandle(PaintWnd) : NULL;

   if (Message->hwnd == NULL && Thread->cPaintsReady)
   {
      // Find note in window.c:"PAINTING BUG".
      ERR("WARNING SOMETHING HAS GONE WRONG: Thread marked as containing dirty windows, but no dirty windows found! Counts %u\n",Thread->cPaintsReady);
      /* Hack to stop spamming the debug log ! */
      Thread->cPaintsReady = 0;
      return FALSE;
   }

   if (Message->hwnd == NULL)
      return FALSE;

   if (!(Window == NULL ||
         PaintWnd == Window ||
         IntIsChildWindow(Window, PaintWnd))) /* check that it is a child of the specified parent */
      return FALSE;

   if (PaintWnd->state & WNDS_INTERNALPAINT)
   {
      PaintWnd->state &= ~WNDS_INTERNALPAINT;
      if (!PaintWnd->hrgnUpdate)
         MsqDecPaintCountQueue(Thread);
   }
   PaintWnd->state2 &= ~WNDS2_STARTPAINT;
   PaintWnd->state &= ~WNDS_UPDATEDIRTY;

   Window = PaintWnd;
   while (Window && !UserIsDesktopWindow(Window))
   {
      // Role back and check for clip children, do not set if any.
      if (Window->spwndParent && !(Window->spwndParent->style & WS_CLIPCHILDREN))
      {
         PaintWnd->state2 |= WNDS2_WMPAINTSENT;
      }
      Window = Window->spwndParent;
   }

   Message->wParam = Message->lParam = 0;
   Message->message = WM_PAINT;
   return TRUE;
}

BOOL
FASTCALL
IntPrintWindow(
    PWND pwnd,
    HDC hdcBlt,
    UINT nFlags)
{
    HDC hdcSrc;
    INT cx, cy, xSrc, ySrc;

    if ( nFlags & PW_CLIENTONLY)
    {
       cx = pwnd->rcClient.right - pwnd->rcClient.left;
       cy = pwnd->rcClient.bottom - pwnd->rcClient.top;
       xSrc = pwnd->rcClient.left - pwnd->rcWindow.left;
       ySrc = pwnd->rcClient.top - pwnd->rcWindow.top;
    }
    else
    {
       cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
       cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top;
       xSrc = 0;
       ySrc = 0;
    }

    // TODO: Setup Redirection for Print.
    return FALSE;

    /* Update the window just incase. */
    co_IntUpdateWindows( pwnd, RDW_ALLCHILDREN, FALSE);

    hdcSrc = UserGetDCEx( pwnd, NULL, DCX_CACHE|DCX_WINDOW);
    /* Print window to printer context. */
    NtGdiBitBlt( hdcBlt,
                 0,
                 0,
                 cx,
                 cy,
                 hdcSrc,
                 xSrc,
                 ySrc,
                 SRCCOPY,
                 0,
                 0);

    UserReleaseDC( pwnd, hdcSrc, FALSE);

    // TODO: Release Redirection from Print.

    return TRUE;
}

BOOL
FASTCALL
IntFlashWindowEx(PWND pWnd, PFLASHWINFO pfwi)
{
   DWORD_PTR FlashState;
   UINT uCount = pfwi->uCount;
   BOOL Activate = FALSE, Ret = FALSE;

   ASSERT(pfwi);

   FlashState = (DWORD_PTR)UserGetProp(pWnd, AtomFlashWndState, TRUE);

   if (FlashState == FLASHW_FINISHED)
   {
      // Cycle has finished, kill timer and set this to Stop.
      FlashState |= FLASHW_KILLSYSTIMER;
      pfwi->dwFlags = FLASHW_STOP;
   }
   else
   {
      if (FlashState)
      {
         if (pfwi->dwFlags == FLASHW_SYSTIMER)
         {
             // Called from system timer, restore flags, counts and state.
             pfwi->dwFlags = LOWORD(FlashState);
             uCount = HIWORD(FlashState);
             FlashState = MAKELONG(LOWORD(FlashState),0);
         }
         else
         {
             // Clean out the trash! Fix SeaMonkey crash after restart.
             FlashState = 0;
         }
      }

      if (FlashState == 0)
      {  // First time in cycle, setup flash state.
         if ( pWnd->state & WNDS_ACTIVEFRAME ||
             (pfwi->dwFlags & FLASHW_CAPTION && pWnd->style & (WS_BORDER|WS_DLGFRAME)))
         {
             FlashState = FLASHW_STARTED|FLASHW_ACTIVE;
         }
      }

      // Set previous window state.
      Ret = !!(FlashState & FLASHW_ACTIVE);

      if ( (pfwi->dwFlags & FLASHW_TIMERNOFG) == FLASHW_TIMERNOFG &&
           gpqForeground == pWnd->head.pti->MessageQueue )
      {
          // Flashing until foreground, set this to Stop.
          pfwi->dwFlags = FLASHW_STOP;
      }
   }

   // Toggle activate flag.
   if ( pfwi->dwFlags == FLASHW_STOP )
   {
      if (gpqForeground && gpqForeground->spwndActive == pWnd)
         Activate = TRUE;
      else
         Activate = FALSE;
   }
   else
   {
      Activate = (FlashState & FLASHW_ACTIVE) == 0;
   }

   if ( pfwi->dwFlags == FLASHW_STOP || pfwi->dwFlags & FLASHW_CAPTION )
   {
      co_IntSendMessage(UserHMGetHandle(pWnd), WM_NCACTIVATE, Activate, 0);
   }

   // FIXME: Check for a Stop Sign here.
   if ( pfwi->dwFlags & FLASHW_TRAY )
   {
      // Need some shell work here too.
      TRACE("FIXME: Flash window no Tray support!\n");
   }

   if ( pfwi->dwFlags == FLASHW_STOP )
   {
      if (FlashState & FLASHW_KILLSYSTIMER)
      {
         IntKillTimer(pWnd, ID_EVENT_SYSTIMER_FLASHWIN, TRUE);
      }

      UserRemoveProp(pWnd, AtomFlashWndState, TRUE);
   }
   else
   {  // Have a count and started, set timer.
      if ( uCount )
      {
         FlashState |= FLASHW_COUNT;

         if (!(Activate ^ !!(FlashState & FLASHW_STARTED)))
             uCount--;

         if (!(FlashState & FLASHW_KILLSYSTIMER))
             pfwi->dwFlags |= FLASHW_TIMER;
      }

      if (pfwi->dwFlags & FLASHW_TIMER)
      {
         FlashState |= FLASHW_KILLSYSTIMER;

         IntSetTimer( pWnd,
                      ID_EVENT_SYSTIMER_FLASHWIN,
                      pfwi->dwTimeout ? pfwi->dwTimeout : gpsi->dtCaretBlink,
                      SystemTimerProc,
                      TMRF_SYSTEM );
      }

      if (FlashState & FLASHW_COUNT && uCount == 0)
      {
         // Keep spinning? Nothing else to do.
         FlashState = FLASHW_FINISHED;
      }
      else
      {
         // Save state and flags so this can be restored next time through.
         FlashState ^= (FlashState ^ -!!(Activate)) & FLASHW_ACTIVE;
         FlashState ^= (FlashState ^ pfwi->dwFlags) & (FLASHW_MASK & ~FLASHW_TIMER);
      }
      FlashState = MAKELONG(LOWORD(FlashState),uCount);
      UserSetProp(pWnd, AtomFlashWndState, (HANDLE)FlashState, TRUE);
   }
   return Ret;
}

// Win: xxxBeginPaint
HDC FASTCALL
IntBeginPaint(PWND Window, PPAINTSTRUCT Ps)
{
   RECT Rect;
   INT type;
   BOOL Erase = FALSE;

   co_UserHideCaret(Window);

   Window->state2 |= WNDS2_STARTPAINT;
   Window->state &= ~WNDS_PAINTNOTPROCESSED;

   if (Window->state & WNDS_SENDNCPAINT)
   {
      HRGN hRgn;
      // Application can keep update dirty.
      do
      {
         Window->state &= ~WNDS_UPDATEDIRTY;
         hRgn = IntGetNCUpdateRgn(Window, FALSE);
         IntSendNCPaint(Window, hRgn);
         if (hRgn > HRGN_WINDOW && GreIsHandleValid(hRgn))
         {
            /* NOTE: The region can already be deleted! */
            GreDeleteObject(hRgn);
         }
      }
      while(Window->state & WNDS_UPDATEDIRTY);
   }
   else
   {
      Window->state &= ~WNDS_UPDATEDIRTY;
   }

   RtlZeroMemory(Ps, sizeof(PAINTSTRUCT));

   if (Window->state2 & WNDS2_ENDPAINTINVALIDATE)
   {
      ERR("BP: Another thread invalidated this window\n");
   }

   Ps->hdc = UserGetDCEx( Window,
                          Window->hrgnUpdate,
                          DCX_INTERSECTRGN | DCX_USESTYLE);
   if (!Ps->hdc)
   {
      return NULL;
   }

   // If set, always clear flags out due to the conditions later on for sending the message.
   if (Window->state & WNDS_SENDERASEBACKGROUND)
   {
      Window->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
      Erase = TRUE;
   }

   if (Window->hrgnUpdate != NULL)
   {
      MsqDecPaintCountQueue(Window->head.pti);
      IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
      /* The region is part of the dc now and belongs to the process! */
      Window->hrgnUpdate = NULL;
   }
   else
   {
      if (Window->state & WNDS_INTERNALPAINT)
         MsqDecPaintCountQueue(Window->head.pti);
   }

   type = GdiGetClipBox(Ps->hdc, &Ps->rcPaint);

   IntGetClientRect(Window, &Rect);

   Window->state &= ~WNDS_INTERNALPAINT;

   if ( Erase &&               // Set to erase,
        type != NULLREGION &&  // don't erase if the clip box is empty,
        (!(Window->pcls->style & CS_PARENTDC) || // not parent dc or
         RECTL_bIntersectRect( &Rect, &Rect, &Ps->rcPaint) ) ) // intersecting.
   {
      Ps->fErase = !co_IntSendMessage(UserHMGetHandle(Window), WM_ERASEBKGND, (WPARAM)Ps->hdc, 0);
      if ( Ps->fErase )
      {
         Window->state |= (WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
      }
   }
   else
   {
      Ps->fErase = FALSE;
   }

   IntSendChildNCPaint(Window);

   return Ps->hdc;
}

// Win: xxxEndPaint
BOOL FASTCALL
IntEndPaint(PWND Wnd, PPAINTSTRUCT Ps)
{
   HDC hdc = NULL;

   hdc = Ps->hdc;

   UserReleaseDC(Wnd, hdc, TRUE);

   if (Wnd->state2 & WNDS2_ENDPAINTINVALIDATE)
   {
      ERR("EP: Another thread invalidated this window\n");
      Wnd->state2 &= ~WNDS2_ENDPAINTINVALIDATE;
   }

   Wnd->state2 &= ~(WNDS2_WMPAINTSENT|WNDS2_STARTPAINT);

   co_UserShowCaret(Wnd);

   return TRUE;
}

// Win: xxxFillWindow
BOOL FASTCALL
IntFillWindow(PWND pWndParent,
              PWND pWnd,
              HDC  hDC,
              HBRUSH hBrush)
{
   RECT Rect, Rect1;
   INT type;

   if (!pWndParent)
      pWndParent = pWnd;

   type = GdiGetClipBox(hDC, &Rect);

   IntGetClientRect(pWnd, &Rect1);

   if ( type != NULLREGION && // Clip box is not empty,
       (!(pWnd->pcls->style & CS_PARENTDC) || // not parent dc or
         RECTL_bIntersectRect( &Rect, &Rect, &Rect1) ) ) // intersecting.
   {
      POINT ppt;
      INT x = 0, y = 0;

      if (!UserIsDesktopWindow(pWndParent))
      {
          x = pWndParent->rcClient.left - pWnd->rcClient.left;
          y = pWndParent->rcClient.top  - pWnd->rcClient.top;
      }

      GreSetBrushOrg(hDC, x, y, &ppt);

      if ( hBrush < (HBRUSH)CTLCOLOR_MAX )
          hBrush = GetControlColor( pWndParent, pWnd, hDC, HandleToUlong(hBrush) + WM_CTLCOLORMSGBOX);

      FillRect(hDC, &Rect, hBrush);

      GreSetBrushOrg(hDC, ppt.x, ppt.y, NULL);

      return TRUE;
   }
   else
      return FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * NtUserBeginPaint
 *
 * Status
 *    @implemented
 */

HDC APIENTRY
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* UnsafePs)
{
   PWND Window = NULL;
   PAINTSTRUCT Ps;
   NTSTATUS Status;
   HDC hDC;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(HDC);

   TRACE("Enter NtUserBeginPaint\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( NULL);
   }

   UserRefObjectCo(Window, &Ref);

   hDC = IntBeginPaint(Window, &Ps);

   Status = MmCopyToCaller(UnsafePs, &Ps, sizeof(PAINTSTRUCT));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(NULL);
   }

   RETURN(hDC);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);

   TRACE("Leave NtUserBeginPaint, ret=%p\n",_ret_);
   UserLeave();
   END_CLEANUP;

}

/*
 * NtUserEndPaint
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* pUnsafePs)
{
   NTSTATUS Status = STATUS_SUCCESS;
   PWND Window = NULL;
   PAINTSTRUCT Ps;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserEndPaint\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref); // Here for the exception.

   _SEH2_TRY
   {
      ProbeForRead(pUnsafePs, sizeof(*pUnsafePs), 1);
      RtlCopyMemory(&Ps, pUnsafePs, sizeof(PAINTSTRUCT));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END
   if (!NT_SUCCESS(Status))
   {
      RETURN(FALSE);
   }

   RETURN(IntEndPaint(Window, &Ps));

CLEANUP:
   if (Window) UserDerefObjectCo(Window);

   TRACE("Leave NtUserEndPaint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * FillWindow: Called from User; Dialog, Edit and ListBox procs during a WM_ERASEBKGND.
 */
/*
 * @implemented
 */
BOOL APIENTRY
NtUserFillWindow(HWND hWndParent,
                 HWND hWnd,
                 HDC  hDC,
                 HBRUSH hBrush)
{
   BOOL ret = FALSE;
   PWND pWnd, pWndParent = NULL;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserFillWindow\n");
   UserEnterExclusive();

   if (!hDC)
   {
      goto Exit;
   }

   if (!(pWnd = UserGetWindowObject(hWnd)))
   {
      goto Exit;
   }

   if (hWndParent && !(pWndParent = UserGetWindowObject(hWndParent)))
   {
      goto Exit;
   }

   UserRefObjectCo(pWnd, &Ref);
   ret = IntFillWindow( pWndParent, pWnd, hDC, hBrush );
   UserDerefObjectCo(pWnd);

Exit:
   TRACE("Leave NtUserFillWindow, ret=%i\n",ret);
   UserLeave();
   return ret;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserFlashWindowEx(IN PFLASHWINFO pfwi)
{
   PWND pWnd;
   FLASHWINFO finfo = {0};
   BOOL Ret = FALSE;

   UserEnterExclusive();

   _SEH2_TRY
   {
      ProbeForRead(pfwi, sizeof(FLASHWINFO), 1);
      RtlCopyMemory(&finfo, pfwi, sizeof(FLASHWINFO));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      _SEH2_YIELD(goto Exit);
   }
   _SEH2_END

   if (!( pWnd = ValidateHwndNoErr(finfo.hwnd)) ||
        finfo.cbSize != sizeof(FLASHWINFO) ||
        finfo.dwFlags & ~(FLASHW_ALL|FLASHW_TIMER|FLASHW_TIMERNOFG) )
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      goto Exit;
   }

   Ret = IntFlashWindowEx(pWnd, &finfo);

Exit:
   UserLeave();
   return Ret;
}

/*
    GetUpdateRgn, this fails the same as the old one.
 */
INT FASTCALL
co_UserGetUpdateRgn(PWND Window, HRGN hRgn, BOOL bErase)
{
   int RegionType;
   BOOL Type;
   RECTL Rect;

   ASSERT_REFS_CO(Window);

   if (bErase)
   {
      USER_REFERENCE_ENTRY Ref;
      UserRefObjectCo(Window, &Ref);
      co_IntPaintWindows(Window, RDW_NOCHILDREN, FALSE);
      UserDerefObjectCo(Window);
   }

   Window->state &= ~WNDS_UPDATEDIRTY;

   if (Window->hrgnUpdate == NULL)
   {
       NtGdiSetRectRgn(hRgn, 0, 0, 0, 0);
       return NULLREGION;
   }

   Rect = Window->rcClient;
   Type = IntIntersectWithParents(Window, &Rect);

   if (Window->hrgnUpdate == HRGN_WINDOW)
   {
      // Trap it out.
      ERR("GURn: Caller is passing Window Region 1\n");
      if (!Type)
      {
         NtGdiSetRectRgn(hRgn, 0, 0, 0, 0);
         return NULLREGION;
      }

      RegionType = SIMPLEREGION;

      if (!UserIsDesktopWindow(Window))
      {
         RECTL_vOffsetRect(&Rect,
                          -Window->rcClient.left,
                          -Window->rcClient.top);
      }
      GreSetRectRgnIndirect(hRgn, &Rect);
   }
   else
   {
      HRGN hrgnTemp = GreCreateRectRgnIndirect(&Rect);

      RegionType = NtGdiCombineRgn(hRgn, hrgnTemp, Window->hrgnUpdate, RGN_AND);

      if (RegionType == ERROR || RegionType == NULLREGION)
      {
         if (hrgnTemp) GreDeleteObject(hrgnTemp);
         NtGdiSetRectRgn(hRgn, 0, 0, 0, 0);
         return RegionType;
      }

      if (!UserIsDesktopWindow(Window))
      {
         NtGdiOffsetRgn(hRgn,
                       -Window->rcClient.left,
                       -Window->rcClient.top);
      }
      if (hrgnTemp) GreDeleteObject(hrgnTemp);
   }
   return RegionType;
}

BOOL FASTCALL
co_UserGetUpdateRect(PWND Window, PRECT pRect, BOOL bErase)
{
   INT RegionType;
   BOOL Ret = TRUE;

   if (bErase)
   {
      USER_REFERENCE_ENTRY Ref;
      UserRefObjectCo(Window, &Ref);
      co_IntPaintWindows(Window, RDW_NOCHILDREN, FALSE);
      UserDerefObjectCo(Window);
   }

   Window->state &= ~WNDS_UPDATEDIRTY;

   if (Window->hrgnUpdate == NULL)
   {
      pRect->left = pRect->top = pRect->right = pRect->bottom = 0;
      Ret = FALSE;
   }
   else
   {
      /* Get the update region bounding box. */
      if (Window->hrgnUpdate == HRGN_WINDOW)
      {
         *pRect = Window->rcClient;
         ERR("GURt: Caller is retrieving Window Region 1\n");
      }
      else
      {
         RegionType = IntGdiGetRgnBox(Window->hrgnUpdate, pRect);

         if (RegionType != ERROR && RegionType != NULLREGION)
            RECTL_bIntersectRect(pRect, pRect, &Window->rcClient);
      }

      if (IntIntersectWithParents(Window, pRect))
      {
         if (!UserIsDesktopWindow(Window))
         {
            RECTL_vOffsetRect(pRect,
                              -Window->rcClient.left,
                              -Window->rcClient.top);
         }
         if (Window->pcls->style & CS_OWNDC)
         {
            HDC hdc;
            //DWORD layout;
            hdc = UserGetDCEx(Window, NULL, DCX_USESTYLE);
            //layout = NtGdiSetLayout(hdc, -1, 0);
            //IntMapWindowPoints( 0, Window, (LPPOINT)pRect, 2 );
            GreDPtoLP( hdc, (LPPOINT)pRect, 2 );
            //NtGdiSetLayout(hdc, -1, layout);
            UserReleaseDC(Window, hdc, FALSE);
         }
      }
      else
      {
         pRect->left = pRect->top = pRect->right = pRect->bottom = 0;
      }
   }
   return Ret;
}

/*
 * NtUserGetUpdateRgn
 *
 * Status
 *    @implemented
 */

INT APIENTRY
NtUserGetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
   DECLARE_RETURN(INT);
   PWND Window;
   INT ret;

   TRACE("Enter NtUserGetUpdateRgn\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(ERROR);
   }

   ret = co_UserGetUpdateRgn(Window, hRgn, bErase);

   RETURN(ret);

CLEANUP:
   TRACE("Leave NtUserGetUpdateRgn, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserGetUpdateRect
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserGetUpdateRect(HWND hWnd, LPRECT UnsafeRect, BOOL bErase)
{
   PWND Window;
   RECTL Rect;
   NTSTATUS Status;
   BOOL Ret;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserGetUpdateRect\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   Ret = co_UserGetUpdateRect(Window, &Rect, bErase);

   if (UnsafeRect != NULL)
   {
      Status = MmCopyToCaller(UnsafeRect, &Rect, sizeof(RECTL));
      if (!NT_SUCCESS(Status))
      {
         EngSetLastError(ERROR_INVALID_PARAMETER);
         RETURN(FALSE);
      }
   }

   RETURN(Ret);

CLEANUP:
   TRACE("Leave NtUserGetUpdateRect, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserRedrawWindow
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserRedrawWindow(
   HWND hWnd,
   CONST RECT *lprcUpdate,
   HRGN hrgnUpdate,
   UINT flags)
{
   RECTL SafeUpdateRect;
   PWND Wnd;
   BOOL Ret;
   USER_REFERENCE_ENTRY Ref;
   NTSTATUS Status = STATUS_SUCCESS;
   PREGION RgnUpdate = NULL;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserRedrawWindow\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd ? hWnd : IntGetDesktopWindow())))
   {
      RETURN( FALSE);
   }

   if (lprcUpdate)
   {
      _SEH2_TRY
      {
          ProbeForRead(lprcUpdate, sizeof(RECTL), 1);
          RtlCopyMemory(&SafeUpdateRect, lprcUpdate, sizeof(RECTL));
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END
      if (!NT_SUCCESS(Status))
      {
         EngSetLastError(RtlNtStatusToDosError(Status));
         RETURN( FALSE);
      }
   }

   if ( flags & ~(RDW_ERASE|RDW_FRAME|RDW_INTERNALPAINT|RDW_INVALIDATE|
                  RDW_NOERASE|RDW_NOFRAME|RDW_NOINTERNALPAINT|RDW_VALIDATE|
                  RDW_ERASENOW|RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_NOCHILDREN) )
   {
      /* RedrawWindow fails only in case that flags are invalid */
      EngSetLastError(ERROR_INVALID_FLAGS);
      RETURN( FALSE);
   }

   /* We can't hold lock on GDI objects while doing roundtrips to user mode,
    * so it will be copied.
    */
   if (hrgnUpdate > HRGN_WINDOW)
   {
       RgnUpdate = REGION_LockRgn(hrgnUpdate);
       if (!RgnUpdate)
       {
           EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
           RETURN(FALSE);
       }
       REGION_UnlockRgn(RgnUpdate);
   }
   else if (hrgnUpdate == HRGN_WINDOW) // Trap it out.
   {
       ERR("NTRW: Caller is passing Window Region 1\n");
   }

   UserRefObjectCo(Wnd, &Ref);

   Ret = co_UserRedrawWindow( Wnd,
                              lprcUpdate ? &SafeUpdateRect : NULL,
                              RgnUpdate,
                              flags);

   UserDerefObjectCo(Wnd);

   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserRedrawWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
UserDrawCaptionText(
   PWND pWnd,
   HDC hDc,
   const PUNICODE_STRING Text,
   const RECTL *lpRc,
   UINT uFlags,
   HFONT hFont)
{
   HFONT hOldFont = NULL;
   COLORREF OldTextColor;
   NONCLIENTMETRICSW nclm;
   NTSTATUS Status;
   BOOLEAN bDeleteFont = FALSE;
   SIZE Size;
   BOOL Ret = TRUE;
   ULONG fit = 0, Length;
   RECTL r = *lpRc;

   TRACE("UserDrawCaptionText: %wZ\n", Text);

   nclm.cbSize = sizeof(nclm);
   if (!UserSystemParametersInfo(SPI_GETNONCLIENTMETRICS, nclm.cbSize, &nclm, 0))
   {
      ERR("UserSystemParametersInfo() failed!\n");
      return FALSE;
   }

   if (!hFont)
   {
      if(uFlags & DC_SMALLCAP)
         Status = TextIntCreateFontIndirect(&nclm.lfSmCaptionFont, &hFont);
      else
         Status = TextIntCreateFontIndirect(&nclm.lfCaptionFont, &hFont);

      if(!NT_SUCCESS(Status))
      {
         ERR("TextIntCreateFontIndirect() failed! Status: 0x%x\n", Status);
         return FALSE;
      }

      bDeleteFont = TRUE;
   }

   IntGdiSetBkMode(hDc, TRANSPARENT);

   hOldFont = NtGdiSelectFont(hDc, hFont);

   if(uFlags & DC_INBUTTON)
      OldTextColor = IntGdiSetTextColor(hDc, IntGetSysColor(COLOR_BTNTEXT));
   else
      OldTextColor = IntGdiSetTextColor(hDc,
                                        IntGetSysColor(uFlags & DC_ACTIVE ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));

   // Adjust for system menu.
   if (pWnd && pWnd->style & WS_SYSMENU)
   {
      r.right -= UserGetSystemMetrics(SM_CYCAPTION) - 1;
      if ((pWnd->style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX)) && !(pWnd->ExStyle & WS_EX_TOOLWINDOW))
      {
         r.right -= UserGetSystemMetrics(SM_CXSIZE) + 1;
         r.right -= UserGetSystemMetrics(SM_CXSIZE) + 1;
      }
   }

   GreGetTextExtentExW(hDc, Text->Buffer, Text->Length/sizeof(WCHAR), r.right - r.left, &fit, 0, &Size, 0);

   Length = (Text->Length/sizeof(WCHAR) == fit ? fit : fit+1);

   if (Text->Length/sizeof(WCHAR) > Length)
   {
      Ret = FALSE;
   }

   if (Ret)
   {  // Faster while in setup.
      UserExtTextOutW( hDc,
                      lpRc->left,
                      lpRc->top + (lpRc->bottom - lpRc->top - Size.cy) / 2, // DT_SINGLELINE && DT_VCENTER
                      ETO_CLIPPED,
                     (RECTL *)lpRc,
                      Text->Buffer,
                      Length);
   }
   else
   {
      DrawTextW( hDc,
                 Text->Buffer,
                 Text->Length/sizeof(WCHAR),
                (RECTL *)&r,
                 DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_LEFT);
   }

   IntGdiSetTextColor(hDc, OldTextColor);

   if (hOldFont)
      NtGdiSelectFont(hDc, hOldFont);

   if (bDeleteFont)
      GreDeleteObject(hFont);

   return Ret;
}

//
// This draws Buttons, Icons and Text...
//
BOOL UserDrawCaption(
   PWND pWnd,
   HDC hDc,
   RECTL *lpRc,
   HFONT hFont,
   HICON hIcon,
   const PUNICODE_STRING Str,
   UINT uFlags)
{
   BOOL Ret = FALSE;
   HBRUSH hBgBrush, hOldBrush = NULL;
   RECTL Rect = *lpRc;
   BOOL HasIcon;

   RECTL_vMakeWellOrdered(lpRc);

   /* Determine whether the icon needs to be displayed */
   if (!hIcon && pWnd != NULL)
   {
     HasIcon = (uFlags & DC_ICON) && !(uFlags & DC_SMALLCAP) &&
               (pWnd->style & WS_SYSMENU) && !(pWnd->ExStyle & WS_EX_TOOLWINDOW);
   }
   else
     HasIcon = (hIcon != NULL);

   // Draw the caption background
   if((uFlags & DC_GRADIENT) && !(uFlags & DC_INBUTTON))
   {
      static GRADIENT_RECT gcap = {0, 1};
      TRIVERTEX Vertices[2];
      COLORREF Colors[2];

      Colors[0] = IntGetSysColor((uFlags & DC_ACTIVE) ?
            COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);

      Colors[1] = IntGetSysColor((uFlags & DC_ACTIVE) ?
            COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);

      Vertices[0].x = Rect.left;
      Vertices[0].y = Rect.top;
      Vertices[0].Red = (WORD)Colors[0]<<8;
      Vertices[0].Green = (WORD)Colors[0] & 0xFF00;
      Vertices[0].Blue = (WORD)(Colors[0]>>8) & 0xFF00;
      Vertices[0].Alpha = 0;

      Vertices[1].x = Rect.right;
      Vertices[1].y = Rect.bottom;
      Vertices[1].Red = (WORD)Colors[1]<<8;
      Vertices[1].Green = (WORD)Colors[1] & 0xFF00;
      Vertices[1].Blue = (WORD)(Colors[1]>>8) & 0xFF00;
      Vertices[1].Alpha = 0;

      if(!GreGradientFill(hDc, Vertices, 2, &gcap, 1, GRADIENT_FILL_RECT_H))
      {
         ERR("GreGradientFill() failed!\n");
         goto cleanup;
      }
   }
   else
   {
      if(uFlags & DC_INBUTTON)
         hBgBrush = IntGetSysColorBrush(COLOR_3DFACE);
      else if(uFlags & DC_ACTIVE)
         hBgBrush = IntGetSysColorBrush(COLOR_ACTIVECAPTION);
      else
         hBgBrush = IntGetSysColorBrush(COLOR_INACTIVECAPTION);

      hOldBrush = NtGdiSelectBrush(hDc, hBgBrush);

      if(!hOldBrush)
      {
         ERR("NtGdiSelectBrush() failed!\n");
         goto cleanup;
      }

      if(!NtGdiPatBlt(hDc, Rect.left, Rect.top,
         Rect.right - Rect.left,
         Rect.bottom - Rect.top,
         PATCOPY))
      {
         ERR("NtGdiPatBlt() failed!\n");
         goto cleanup;
      }
   }

   /* Draw icon */
   if (HasIcon)
   {
      PCURICON_OBJECT pIcon = NULL;

      if (hIcon)
      {
          pIcon = UserGetCurIconObject(hIcon);
      }
      else if (pWnd)
      {
          pIcon = NC_IconForWindow(pWnd);
          // FIXME: NC_IconForWindow should reference it for us */
          if (pIcon)
              UserReferenceObject(pIcon);
      }

      if (pIcon)
      {
         LONG cx = UserGetSystemMetrics(SM_CXSMICON);
         LONG cy = UserGetSystemMetrics(SM_CYSMICON);
         LONG x = Rect.left - cx/2 + 1 + (Rect.bottom - Rect.top)/2; // this is really what Window does
         LONG y = (Rect.top + Rect.bottom - cy)/2; // center
         UserDrawIconEx(hDc, x, y, pIcon, cx, cy, 0, NULL, DI_NORMAL);
         UserDereferenceObject(pIcon);
      }
      else
      {
          HasIcon = FALSE;
      }
   }

   if (HasIcon)
      Rect.left += Rect.bottom - Rect.top;

   if((uFlags & DC_TEXT))
   {
      BOOL Set = FALSE;
      Rect.left += 2;

      if (Str)
         Set = UserDrawCaptionText(pWnd, hDc, Str, &Rect, uFlags, hFont);
      else if (pWnd != NULL) // FIXME: Windows does not do that
      {
         UNICODE_STRING ustr;
         ustr.Buffer = pWnd->strName.Buffer; // FIXME: LARGE_STRING truncated!
         ustr.Length = (USHORT)min(pWnd->strName.Length, MAXUSHORT);
         ustr.MaximumLength = (USHORT)min(pWnd->strName.MaximumLength, MAXUSHORT);
         Set = UserDrawCaptionText(pWnd, hDc, &ustr, &Rect, uFlags, hFont);
      }
      if (pWnd)
      {
         if (Set)
            pWnd->state2 &= ~WNDS2_CAPTIONTEXTTRUNCATED;
         else
            pWnd->state2 |= WNDS2_CAPTIONTEXTTRUNCATED;
      }
   }

   Ret = TRUE;

cleanup:
   if (hOldBrush) NtGdiSelectBrush(hDc, hOldBrush);

   return Ret;
}

INT
FASTCALL
UserRealizePalette(HDC hdc)
{
  HWND hWnd, hWndDesktop;
  DWORD Ret;

  Ret = IntGdiRealizePalette(hdc);
  if (Ret) // There was a change.
  {
      hWnd = IntWindowFromDC(hdc);
      if (hWnd) // Send broadcast if dc is associated with a window.
      {  // FYI: Thread locked in CallOneParam.
         hWndDesktop = IntGetDesktopWindow();
         if ( hWndDesktop != hWnd )
         {
            PWND pWnd = UserGetWindowObject(hWndDesktop);
            ERR("RealizePalette Desktop.\n");
            hdc = UserGetWindowDC(pWnd);
            IntPaintDesktop(hdc);
            UserReleaseDC(pWnd,hdc,FALSE);
         }
         UserSendNotifyMessage((HWND)HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hWnd, 0);
      }
  }
  return Ret;
}

BOOL
APIENTRY
NtUserDrawCaptionTemp(
   HWND hWnd,
   HDC hDC,
   LPCRECT lpRc,
   HFONT hFont,
   HICON hIcon,
   const PUNICODE_STRING str,
   UINT uFlags)
{
   PWND pWnd = NULL;
   UNICODE_STRING SafeStr = {0};
   NTSTATUS Status = STATUS_SUCCESS;
   RECTL SafeRect;
   BOOL Ret;

   UserEnterExclusive();

   if (hWnd != NULL)
   {
     if(!(pWnd = UserGetWindowObject(hWnd)))
     {
        UserLeave();
        return FALSE;
     }
   }

   _SEH2_TRY
   {
      ProbeForRead(lpRc, sizeof(RECTL), sizeof(ULONG));
      RtlCopyMemory(&SafeRect, lpRc, sizeof(RECTL));
      if (str != NULL)
      {
         SafeStr = ProbeForReadUnicodeString(str);
         if (SafeStr.Length != 0)
         {
             ProbeForRead( SafeStr.Buffer,
                           SafeStr.Length,
                            sizeof(WCHAR));
         }
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (Status != STATUS_SUCCESS)
   {
      SetLastNtError(Status);
      UserLeave();
      return FALSE;
   }

   if (str != NULL)
      Ret = UserDrawCaption(pWnd, hDC, &SafeRect, hFont, hIcon, &SafeStr, uFlags);
   else
   {
      if ( RECTL_bIsEmptyRect(&SafeRect) && hFont == 0 && hIcon == 0 )
      {
         Ret = TRUE;
         if (uFlags & DC_DRAWCAPTIONMD)
         {
            ERR("NC Caption Mode\n");
            UserDrawCaptionBar(pWnd, hDC, uFlags);
            goto Exit;
         }
         else if (uFlags & DC_DRAWFRAMEMD)
         {
            ERR("NC Paint Mode\n");
            NC_DoNCPaint(pWnd, hDC, uFlags); // Update Menus too!
            goto Exit;
         }
      }
      Ret = UserDrawCaption(pWnd, hDC, &SafeRect, hFont, hIcon, NULL, uFlags);
   }
Exit:
   UserLeave();
   return Ret;
}

BOOL
APIENTRY
NtUserDrawCaption(HWND hWnd,
   HDC hDC,
   LPCRECT lpRc,
   UINT uFlags)
{
   return NtUserDrawCaptionTemp(hWnd, hDC, lpRc, 0, 0, NULL, uFlags);
}

INT FASTCALL
co_UserExcludeUpdateRgn(HDC hDC, PWND Window)
{
    POINT pt;
    RECT rc;

    if (Window->hrgnUpdate)
    {
        if (Window->hrgnUpdate == HRGN_WINDOW)
        {
            return NtGdiIntersectClipRect(hDC, 0, 0, 0, 0);
        }
        else
        {
            INT ret = ERROR;
            HRGN hrgn = NtGdiCreateRectRgn(0,0,0,0);

            if ( hrgn && GreGetDCPoint( hDC, GdiGetDCOrg, &pt) )
            {
                if ( NtGdiGetRandomRgn( hDC, hrgn, CLIPRGN) == NULLREGION )
                {
                    NtGdiOffsetRgn(hrgn, pt.x, pt.y);
                }
                else
                {
                    HRGN hrgnScreen;
                    PMONITOR pm = UserGetPrimaryMonitor();
                    hrgnScreen = NtGdiCreateRectRgn(0,0,0,0);
                    NtGdiCombineRgn(hrgnScreen, hrgnScreen, pm->hrgnMonitor, RGN_OR);

                    NtGdiCombineRgn(hrgn, hrgnScreen, NULL, RGN_COPY);

                    GreDeleteObject(hrgnScreen);
                }

                NtGdiCombineRgn(hrgn, hrgn, Window->hrgnUpdate, RGN_DIFF);

                NtGdiOffsetRgn(hrgn, -pt.x, -pt.y);

                ret = NtGdiExtSelectClipRgn(hDC, hrgn, RGN_COPY);

                GreDeleteObject(hrgn);
            }
            return ret;
        }
    }
    else
    {
        return GdiGetClipBox( hDC, &rc);
    }
}

INT
APIENTRY
NtUserExcludeUpdateRgn(
    HDC hDC,
    HWND hWnd)
{
    INT ret = ERROR;
    PWND pWnd;

    TRACE("Enter NtUserExcludeUpdateRgn\n");
    UserEnterExclusive();

    pWnd = UserGetWindowObject(hWnd);

    if (hDC && pWnd)
        ret = co_UserExcludeUpdateRgn(hDC, pWnd);

    TRACE("Leave NtUserExcludeUpdateRgn, ret=%i\n", ret);

    UserLeave();
    return ret;
}

BOOL
APIENTRY
NtUserInvalidateRect(
    HWND hWnd,
    CONST RECT *lpUnsafeRect,
    BOOL bErase)
{
    UINT flags = RDW_INVALIDATE | (bErase ? RDW_ERASE : 0);
    if (!hWnd)
    {
       flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
       lpUnsafeRect = NULL;
    }
    return NtUserRedrawWindow(hWnd, lpUnsafeRect, NULL, flags);
}

BOOL
APIENTRY
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase)
{
    if (!hWnd)
    {
       EngSetLastError( ERROR_INVALID_WINDOW_HANDLE );
       return FALSE;
    }
    return NtUserRedrawWindow(hWnd, NULL, hRgn, RDW_INVALIDATE | (bErase? RDW_ERASE : 0));
}

BOOL
APIENTRY
NtUserPrintWindow(
    HWND hwnd,
    HDC  hdcBlt,
    UINT nFlags)
{
    PWND Window;
    BOOL Ret = FALSE;

    UserEnterExclusive();

    if (hwnd)
    {
       if (!(Window = UserGetWindowObject(hwnd)) ||
            UserIsDesktopWindow(Window) || UserIsMessageWindow(Window))
       {
          goto Exit;
       }

       if ( Window )
       {
          /* Validate flags and check it as a mask for 0 or 1. */
          if ( (nFlags & PW_CLIENTONLY) == nFlags)
             Ret = IntPrintWindow( Window, hdcBlt, nFlags);
          else
             EngSetLastError(ERROR_INVALID_PARAMETER);
       }
    }
Exit:
    UserLeave();
    return Ret;
}

/* ValidateRect gets redirected to NtUserValidateRect:
   http://blog.csdn.net/ntdll/archive/2005/10/19/509299.aspx */
BOOL
APIENTRY
NtUserValidateRect(
    HWND hWnd,
    const RECT *lpRect)
{
    UINT flags = RDW_VALIDATE;
    if (!hWnd)
    {
       flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
       lpRect = NULL;
    }
    return NtUserRedrawWindow(hWnd, lpRect, NULL, flags);
}

/* EOF */
