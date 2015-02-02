/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Window painting function
 *  FILE:             subsystems/win32/win32k/ntuser/painting.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserPainting);

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

   ParentWnd = Child->spwndParent;
   while (ParentWnd != NULL)
   {
      if (!(ParentWnd->style & WS_VISIBLE) ||
          (ParentWnd->style & WS_MINIMIZE))
      {
         return FALSE;
      }

      if (!RECTL_bIntersectRect(WindowRect, WindowRect, &ParentWnd->rcClient))
      {
         return FALSE;
      }

      /* FIXME: Layered windows. */

      ParentWnd = ParentWnd->spwndParent;
   }

   return TRUE;
}

BOOL FASTCALL
IntValidateParent(PWND Child, PREGION ValidateRgn, BOOL Recurse)
{
   PWND ParentWnd = Child;

   if (ParentWnd->style & WS_CHILD)
   {
      do
         ParentWnd = ParentWnd->spwndParent;
      while (ParentWnd->style & WS_CHILD);
   }
// Hax out for drawing issues.
//   if (!(ParentWnd->state & WNDS_SYNCPAINTPENDING)) Recurse = FALSE;

   ParentWnd = Child->spwndParent;
   while (ParentWnd)
   {
      if (ParentWnd->style & WS_CLIPCHILDREN)
         break;

      if (ParentWnd->hrgnUpdate != 0)
      {
         if (Recurse)
            return FALSE;

         IntInvalidateWindows( ParentWnd,
                               ValidateRgn,
                               RDW_VALIDATE | RDW_NOCHILDREN);
      }

      ParentWnd = ParentWnd->spwndParent;
   }

   return TRUE;
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
      ERR("SendSyncPaint Wnd in State!\n");
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
         ERR("Sending WM_SYNCPAINT\n");
         // This message has no parameters. But it does! Pass Flags along.
         co_IntSendMessageNoWait(UserHMGetHandle(Wnd), WM_SYNCPAINT, Flags, 0);
         Wnd->state |= WNDS_SYNCPAINTPENDING;
      }
   }

   // Send to all the children if this is the desktop window.
   if ( Wnd == UserGetDesktopWindow() )
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

/**
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

/**
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
   UINT RgnType;

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

      RgnType = NtGdiCombineRgn(hRgnNonClient, hRgnNonClient,
                                hRgnWindow, RGN_DIFF);
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
         if (NtGdiCombineRgn(Window->hrgnUpdate, Window->hrgnUpdate,
                             hRgnWindow, RGN_AND) == NULLREGION)
         {
            IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
            GreDeleteObject(Window->hrgnUpdate);
            Window->state &= ~WNDS_UPDATEDIRTY;
            Window->hrgnUpdate = NULL;
            if (!(Window->state & WNDS_INTERNALPAINT))
               MsqDecPaintCountQueue(Window->head.pti);
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
   HRGN TempRegion;

   Wnd->state &= ~WNDS_PAINTNOTPROCESSED;

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      if (Wnd->hrgnUpdate)
      {
          PREGION RgnUpdate = REGION_LockRgn(Wnd->hrgnUpdate);
          if (RgnUpdate)
          {
              if (!IntValidateParent(Wnd, RgnUpdate, Recurse))
              {
                  REGION_UnlockRgn(RgnUpdate);
                  return;
              }
              REGION_UnlockRgn(RgnUpdate);
          }
      }

      if (Flags & RDW_UPDATENOW)
      {
         if ((Wnd->hrgnUpdate != NULL ||
              Wnd->state & WNDS_INTERNALPAINT))
         {
            Wnd->state2 |= WNDS2_WMPAINTSENT;
            co_IntSendMessage(hWnd, WM_PAINT, 0, 0);
         }
      }
      else if (Wnd->head.pti == PsGetCurrentThreadWin32Thread())
      {
         if (Wnd->state & WNDS_SENDNCPAINT)
         {
            TempRegion = IntGetNCUpdateRgn(Wnd, TRUE);
            Wnd->state &= ~WNDS_SENDNCPAINT;
            if ( Wnd == GetW32ThreadInfo()->MessageQueue->spwndActive &&
                !(Wnd->state & WNDS_ACTIVEFRAME))
            {
               Wnd->state |= WNDS_ACTIVEFRAME;
               Wnd->state &= ~WNDS_NONCPAINT;
            }
            if (TempRegion) co_IntSendMessage(hWnd, WM_NCPAINT, (WPARAM)TempRegion, 0);
         }

         if (Wnd->state & WNDS_SENDERASEBACKGROUND)
         {
            if (Wnd->hrgnUpdate)
            {
               hDC = UserGetDCEx( Wnd,
                                  Wnd->hrgnUpdate,
                                  DCX_CACHE|DCX_USESTYLE|DCX_INTERSECTRGN|DCX_KEEPCLIPRGN);

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
   else
   {
      Wnd->state &= ~(WNDS_SENDNCPAINT|WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
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
        ((Flags & RDW_ALLCHILDREN) || !(Wnd->style & WS_CLIPCHILDREN)) )
   {
      HWND *List, *phWnd;

      if ((List = IntWinListChildren(Wnd)))
      {
         /* FIXME: Handle WS_EX_TRANSPARENT */
         for (phWnd = List; *phWnd; ++phWnd)
         {
            Wnd = UserGetWindowObject(*phWnd);
            if (Wnd && (Wnd->style & WS_VISIBLE))
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
 * IntInvalidateWindows
 *
 * Internal function used by IntRedrawWindow, UserRedrawDesktop,
 * co_WinPosSetWindowPos, IntValidateParent, co_UserRedrawWindow.
 */
VOID FASTCALL
IntInvalidateWindows(PWND Wnd, PREGION Rgn, ULONG Flags)
{
   INT RgnType;
   BOOL HadPaintMessage;

   TRACE("IntInvalidateWindows start\n");

   Wnd->state |= WNDS_PAINTNOTPROCESSED;

   /*
    * If the nonclient is not to be redrawn, clip the region to the client
    * rect
    */
   if (0 != (Flags & RDW_INVALIDATE) && 0 == (Flags & RDW_FRAME))
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

      if (Flags & RDW_INVALIDATE && RgnType != NULLREGION)
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

         if (Wnd->hrgnUpdate == NULL)
         {
            Wnd->hrgnUpdate = NtGdiCreateRectRgn(0, 0, 0, 0);
            IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_PUBLIC);
         }

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
         Flags |= RDW_FRAME; // For children.
      }
   }    // The following flags are used to validate the window.
   else if (Flags & (RDW_VALIDATE|RDW_NOINTERNALPAINT|RDW_NOERASE|RDW_NOFRAME))
   {
      /* FIXME: Handle WNDS_UPDATEDIRTY */

      if (Flags & RDW_NOINTERNALPAINT)
      {
         Wnd->state &= ~WNDS_INTERNALPAINT;
      }

      if (Flags & RDW_VALIDATE && RgnType != NULLREGION)
      {
         if (Flags & RDW_NOFRAME)
            Wnd->state &= ~WNDS_SENDNCPAINT;
         if (Flags & RDW_NOERASE)
            Wnd->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);

         if (Wnd->hrgnUpdate != NULL)
         {
             PREGION RgnUpdate = REGION_LockRgn(Wnd->hrgnUpdate);

             if (RgnUpdate)
             {
                 RgnType = IntGdiCombineRgn(RgnUpdate, RgnUpdate, Rgn, RGN_DIFF);
                 REGION_UnlockRgn(RgnUpdate);

                 if(RgnType == NULLREGION)
                 {
                     IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
                     GreDeleteObject(Wnd->hrgnUpdate);
                     Wnd->hrgnUpdate = NULL;
                 }
             }
         }

         if (Wnd->hrgnUpdate == NULL)
            Wnd->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
      }
   }

   /*
    * Process children if needed
    */

   if (!(Flags & RDW_NOCHILDREN) && !(Wnd->style & WS_MINIMIZE) &&
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
                IntGdiCombineRgn(RgnTemp, Rgn, 0, RGN_COPY);
                IntInvalidateWindows(Child, RgnTemp, Flags);
                REGION_Delete(RgnTemp);
            }
         }
      }
   }

   /*
    * Fake post paint messages to window message queue if needed
    */

   if (HadPaintMessage != IntIsWindowDirty(Wnd))
   {
      if (HadPaintMessage)
         MsqDecPaintCountQueue(Wnd->head.pti);
      else
         MsqIncPaintCountQueue(Wnd->head.pti);
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
   TRACE("co_UserRedrawWindow start\n");

   /*
    * Step 1.
    * Validation of passed parameters.
    */

   if (!IntIsWindowDrawable(Window))
   {
      return TRUE; // Just do nothing!!!
   }

   /*
    * Step 2.
    * Transform the parameters UpdateRgn and UpdateRect into
    * a region hRgn specified in screen coordinates.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE)) // Both are OKAY!
   {
      if (UpdateRgn)
      {
          TmpRgn = IntSysCreateRectpRgn(0, 0, 0, 0);
          if (IntGdiCombineRgn(TmpRgn, UpdateRgn, NULL, RGN_COPY) == NULLREGION)
          {
              REGION_Delete(TmpRgn);
              TmpRgn = NULL;
          }
          else
          {
              REGION_bOffsetRgn(TmpRgn, Window->rcClient.left, Window->rcClient.top);
          }
      }
      else if (UpdateRect != NULL)
      {
         if (!RECTL_bIsEmptyRect(UpdateRect))
         {
            TmpRgn = IntSysCreateRectpRgnIndirect(UpdateRect);
            REGION_bOffsetRgn(TmpRgn, Window->rcClient.left, Window->rcClient.top);
         }
      }
      else if ((Flags & (RDW_INVALIDATE | RDW_FRAME)) == (RDW_INVALIDATE | RDW_FRAME) ||
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

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      if (Flags & RDW_ERASENOW) IntSendSyncPaint(Window, Flags);
      co_IntPaintWindows(Window, Flags, FALSE);
   }

   /*
    * Step 5.
    * Cleanup ;-)
    */

   if (TmpRgn != NULL)
   {
      REGION_Delete(TmpRgn);
   }
   TRACE("co_UserRedrawWindow exit\n");

   return TRUE;
}

BOOL FASTCALL
IntIsWindowDirty(PWND Wnd)
{
   return ( Wnd->style & WS_VISIBLE &&
           ( Wnd->hrgnUpdate != NULL ||
             Wnd->state & WNDS_INTERNALPAINT ) );
}

PWND FASTCALL
IntFindWindowToRepaint(PWND Window, PTHREADINFO Thread)
{
   PWND hChild;
   PWND TempWindow;

   for (; Window != NULL; Window = Window->spwndNext)
   {
      if (IntWndBelongsToThread(Window, Thread) &&
          IntIsWindowDirty(Window))
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

      if (Window->spwndChild)
      {
         hChild = IntFindWindowToRepaint(Window->spwndChild, Thread);
         if (hChild != NULL)
            return hChild;
      }
   }
   return Window;
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
   PWND PaintWnd;

   if ((MsgFilterMin != 0 || MsgFilterMax != 0) &&
         (MsgFilterMin > WM_PAINT || MsgFilterMax < WM_PAINT))
      return FALSE;

   if (Thread->TIF_flags & TIF_SYSTEMTHREAD )
   {
      ERR("WM_PAINT is in a System Thread!\n");
   }

   PaintWnd = IntFindWindowToRepaint(UserGetDesktopWindow(), Thread);

   Message->hwnd = PaintWnd ? UserHMGetHandle(PaintWnd) : NULL;

   if (Message->hwnd == NULL)
   {
      ERR("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found! Counts %u\n",Thread->cPaintsReady);
      /* Hack to stop spamming the debuglog ! */
      Thread->cPaintsReady = 0;
      return FALSE;
   }

   if (Window != NULL && PaintWnd != Window)
      return FALSE;

   if (PaintWnd->state & WNDS_INTERNALPAINT)
   {
      PaintWnd->state &= ~WNDS_INTERNALPAINT;
      if (!PaintWnd->hrgnUpdate)
         MsqDecPaintCountQueue(Thread);
   }
   PaintWnd->state2 &= ~WNDS2_WMPAINTSENT;
   PaintWnd->state &= ~WNDS_UPDATEDIRTY;
   Message->wParam = Message->lParam = 0;
   Message->message = WM_PAINT;
   return TRUE;
}

static
HWND FASTCALL
co_IntFixCaret(PWND Window, RECTL *lprc, UINT flags)
{
   PDESKTOP Desktop;
   PTHRDCARETINFO CaretInfo;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ActiveMessageQueue;
   HWND hWndCaret;
   PWND WndCaret;

   ASSERT_REFS_CO(Window);

   pti = PsGetCurrentThreadWin32Thread();
   Desktop = pti->rpdesk;
   ActiveMessageQueue = Desktop->ActiveMessageQueue;
   if (!ActiveMessageQueue) return 0;
   CaretInfo = ActiveMessageQueue->CaretInfo;
   hWndCaret = CaretInfo->hWnd;

   WndCaret = ValidateHwndNoErr(hWndCaret);

   // FIXME: Check for WndCaret can be NULL
   if (WndCaret == Window ||
         ((flags & SW_SCROLLCHILDREN) && IntIsChildWindow(Window, WndCaret)))
   {
      POINT pt, FromOffset, ToOffset;
      RECTL rcCaret;

      pt.x = CaretInfo->Pos.x;
      pt.y = CaretInfo->Pos.y;
      IntGetClientOrigin(WndCaret, &FromOffset);
      IntGetClientOrigin(Window, &ToOffset);
      rcCaret.left = pt.x;
      rcCaret.top = pt.y;
      rcCaret.right = pt.x + CaretInfo->Size.cx;
      rcCaret.bottom = pt.y + CaretInfo->Size.cy;
      if (RECTL_bIntersectRect(lprc, lprc, &rcCaret))
      {
         co_UserHideCaret(0);
         lprc->left = pt.x;
         lprc->top = pt.y;
         return hWndCaret;
      }
   }

   return 0;
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
    co_IntPaintWindows( pwnd, RDW_ERASENOW|RDW_UPDATENOW, FALSE);

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
   DWORD FlashState;
   UINT uCount = pfwi->uCount;
   BOOL Activate = FALSE, Ret = FALSE;

   ASSERT(pfwi);

   FlashState = (DWORD)UserGetProp(pWnd, AtomFlashWndState);

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

      if ( pfwi->dwFlags & FLASHW_TIMERNOFG && 
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

      IntRemoveProp(pWnd, AtomFlashWndState);
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
      IntSetProp(pWnd, AtomFlashWndState, (HANDLE) FlashState);
   }
   return Ret;
}

HDC FASTCALL
IntBeginPaint(PWND Window, PPAINTSTRUCT Ps)
{
   co_UserHideCaret(Window);

   Window->state2 |= WNDS2_STARTPAINT;
   Window->state &= ~WNDS_PAINTNOTPROCESSED;

   if (Window->state & WNDS_SENDNCPAINT)
   {
      HRGN hRgn;

      Window->state &= ~WNDS_UPDATEDIRTY;
      hRgn = IntGetNCUpdateRgn(Window, FALSE);
      Window->state &= ~WNDS_SENDNCPAINT;
      co_IntSendMessage(UserHMGetHandle(Window), WM_NCPAINT, (WPARAM)hRgn, 0);
      if (hRgn != HRGN_WINDOW && hRgn != NULL && GreIsHandleValid(hRgn))
      {
         /* NOTE: The region can already be deleted! */
         GreDeleteObject(hRgn);
      }
   }
   else
   {
      Window->state &= ~WNDS_UPDATEDIRTY;
   }

   RtlZeroMemory(Ps, sizeof(PAINTSTRUCT));

   Ps->hdc = UserGetDCEx( Window,
                         Window->hrgnUpdate,
                         DCX_INTERSECTRGN | DCX_USESTYLE);
   if (!Ps->hdc)
   {
      return NULL;
   }

   if (Window->hrgnUpdate != NULL)
   {
      MsqDecPaintCountQueue(Window->head.pti);
      GdiGetClipBox(Ps->hdc, &Ps->rcPaint);
      IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
      /* The region is part of the dc now and belongs to the process! */
      Window->hrgnUpdate = NULL;
   }
   else
   {
      if (Window->state & WNDS_INTERNALPAINT)
         MsqDecPaintCountQueue(Window->head.pti);

      IntGetClientRect(Window, &Ps->rcPaint);
   }

   Window->state &= ~WNDS_INTERNALPAINT;

   if (Window->state & WNDS_SENDERASEBACKGROUND)
   {
      Window->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_ERASEBACKGROUND);
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
   if (Window->hrgnUpdate)
   {
      if (!(Window->style & WS_CLIPCHILDREN))
      {
         PWND Child;
         for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
         {
            if (Child->hrgnUpdate == NULL && Child->state & WNDS_SENDNCPAINT) // Helped fixing test_redrawnow.
            IntInvalidateWindows(Child, NULL, RDW_FRAME | RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
         }
      }
   }
   return Ps->hdc;
}

BOOL FASTCALL
IntEndPaint(PWND Wnd, PPAINTSTRUCT Ps)
{
   HDC hdc = NULL;

   hdc = Ps->hdc;

   UserReleaseDC(Wnd, hdc, TRUE);

   Wnd->state2 &= ~(WNDS2_WMPAINTSENT|WNDS2_STARTPAINT);

   co_UserShowCaret(Wnd);

   return TRUE;
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
 * @implemented
 */
BOOL APIENTRY
NtUserFlashWindowEx(IN PFLASHWINFO pfwi)
{
   PWND pWnd;
   FLASHWINFO finfo = {0};
   BOOL Ret = TRUE;

   UserEnterExclusive();

   _SEH2_TRY
   {
      ProbeForRead(pfwi, sizeof(FLASHWINFO), sizeof(ULONG));
      RtlCopyMemory(&finfo, pfwi, sizeof(FLASHWINFO));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      Ret = FALSE;
   }
   _SEH2_END

   if (!Ret) goto Exit;

   if (!( pWnd = (PWND)UserGetObject(gHandleTable, finfo.hwnd, TYPE_WINDOW)) ||
        finfo.cbSize != sizeof(FLASHWINFO) ||
        finfo.dwFlags & ~(FLASHW_ALL|FLASHW_TIMER|FLASHW_TIMERNOFG) )
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      goto Exit;
   }

   Ret = IntFlashWindowEx(pWnd, &finfo);

Exit:
   UserLeave();
   return Ret;
}

INT FASTCALL
co_UserGetUpdateRgn(PWND Window, PREGION Rgn, BOOL bErase)
{
    int RegionType;
    RECTL Rect;
    PREGION UpdateRgn;

    ASSERT_REFS_CO(Window);

    Window->state &= ~WNDS_UPDATEDIRTY;

    if (Window->hrgnUpdate == NULL)
    {
        REGION_SetRectRgn(Rgn, 0, 0, 0, 0);
        return NULLREGION;
    }

    UpdateRgn = REGION_LockRgn(Window->hrgnUpdate);
    if (!UpdateRgn)
       return ERROR;

    Rect = Window->rcClient;
    IntIntersectWithParents(Window, &Rect);
    REGION_SetRectRgn(Rgn, Rect.left, Rect.top, Rect.right, Rect.bottom);
    RegionType = IntGdiCombineRgn(Rgn, Rgn, UpdateRgn, RGN_AND);
    REGION_bOffsetRgn(Rgn, -Window->rcClient.left, -Window->rcClient.top);
    REGION_UnlockRgn(UpdateRgn);

   if (bErase && RegionType != NULLREGION && RegionType != ERROR)
   {
      co_UserRedrawWindow(Window, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN);
   }

   return RegionType;
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
   USER_REFERENCE_ENTRY Ref;
   PREGION Rgn = NULL;

   TRACE("Enter NtUserGetUpdateRgn\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(ERROR);
   }

   /* Use a system region, we can't hold GDI locks when doing roundtrips to user mode */
   Rgn = IntSysCreateRectpRgn(0, 0, 0, 0);
   if (!Rgn)
       RETURN(ERROR);

   UserRefObjectCo(Window, &Ref);
   ret = co_UserGetUpdateRgn(Window, Rgn, bErase);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   if (Rgn && (_ret_ != ERROR))
   {
       PREGION TheRgn = REGION_LockRgn(hRgn);
       if (!TheRgn)
       {
           EngSetLastError(ERROR_INVALID_HANDLE);
           _ret_ = ERROR;
       }
       IntGdiCombineRgn(TheRgn, Rgn, NULL, RGN_COPY);
       REGION_UnlockRgn(TheRgn);
   }

   if (Rgn)
       REGION_Delete(Rgn);

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
   INT RegionType;
   PREGION RgnData;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserGetUpdateRect\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   Window->state &= ~WNDS_UPDATEDIRTY;

   if (Window->hrgnUpdate == NULL)
   {
      Rect.left = Rect.top = Rect.right = Rect.bottom = 0;
   }
   else
   {
      /* Get the update region bounding box. */
      if (Window->hrgnUpdate == HRGN_WINDOW)
      {
         Rect = Window->rcClient;
      }
      else
      {
         RgnData = REGION_LockRgn(Window->hrgnUpdate);
         ASSERT(RgnData != NULL);
         RegionType = REGION_GetRgnBox(RgnData, &Rect);
         REGION_UnlockRgn(RgnData);

         if (RegionType != ERROR && RegionType != NULLREGION)
            RECTL_bIntersectRect(&Rect, &Rect, &Window->rcClient);
      }

      if (IntIntersectWithParents(Window, &Rect))
      {
         RECTL_vOffsetRect(&Rect,
                          -Window->rcClient.left,
                          -Window->rcClient.top);
      } else
      {
         Rect.left = Rect.top = Rect.right = Rect.bottom = 0;
      }
   }

   if (bErase && !RECTL_bIsEmptyRect(&Rect))
   {
      USER_REFERENCE_ENTRY Ref;
      UserRefObjectCo(Window, &Ref);
      co_UserRedrawWindow(Window, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN);
      UserDerefObjectCo(Window);
   }

   if (UnsafeRect != NULL)
   {
      Status = MmCopyToCaller(UnsafeRect, &Rect, sizeof(RECTL));
      if (!NT_SUCCESS(Status))
      {
         EngSetLastError(ERROR_INVALID_PARAMETER);
         RETURN(FALSE);
      }
   }

   RETURN(!RECTL_bIsEmptyRect(&Rect));

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

   /* We can't hold lock on GDI obects while doing roundtrips to user mode,
    * so use a copy instead */
   if (hrgnUpdate)
   {
       PREGION RgnTemp;

       RgnUpdate = IntSysCreateRectpRgn(0, 0, 0, 0);
       if (!RgnUpdate)
       {
           EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
           RETURN(FALSE);
       }

       RgnTemp = REGION_LockRgn(hrgnUpdate);
       if (!RgnTemp)
       {
           EngSetLastError(ERROR_INVALID_HANDLE);
           RETURN(FALSE);
       }
       IntGdiCombineRgn(RgnUpdate, RgnTemp, NULL, RGN_COPY);
       REGION_UnlockRgn(RgnTemp);
   }

   UserRefObjectCo(Wnd, &Ref);

   Ret = co_UserRedrawWindow( Wnd,
                              lprcUpdate ? &SafeUpdateRect : NULL,
                              RgnUpdate,
                              flags);

   UserDerefObjectCo(Wnd);

   RETURN( Ret);

CLEANUP:
    if (RgnUpdate)
        REGION_Delete(RgnUpdate);
   TRACE("Leave NtUserRedrawWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static
INT FASTCALL
UserScrollDC(
   HDC hDC,
   INT dx,
   INT dy,
   const RECTL *prcScroll,
   const RECTL *prcClip,
   HRGN hrgnUpdate,
   PREGION RgnUpdate,
   RECTL *prcUpdate)
{
   PDC pDC;
   RECTL rcScroll, rcClip, rcSrc, rcDst;
   INT Result;

   GdiGetClipBox(hDC, &rcClip);
   rcScroll = rcClip;
   if (prcClip)
   {
      RECTL_bIntersectRect(&rcClip, &rcClip, prcClip);
   }

   if (prcScroll)
   {
      rcScroll = *prcScroll;
      RECTL_bIntersectRect(&rcSrc, &rcClip, prcScroll);
   }
   else
   {
      rcSrc = rcClip;
   }

   rcDst = rcSrc;
   RECTL_vOffsetRect(&rcDst, dx, dy);
   RECTL_bIntersectRect(&rcDst, &rcDst, &rcClip);

   if (!NtGdiBitBlt( hDC,
                     rcDst.left,
                     rcDst.top,
                     rcDst.right - rcDst.left,
                     rcDst.bottom - rcDst.top,
                     hDC,
                     rcDst.left - dx,
                     rcDst.top - dy,
                     SRCCOPY,
                     0,
                     0))
   {
      return ERROR;
   }

   /* Calculate the region that was invalidated by moving or
      could not be copied, because it was not visible */
   if (RgnUpdate || hrgnUpdate || prcUpdate)
   {
      PREGION RgnOwn, RgnTmp;

      pDC = DC_LockDc(hDC);
      if (!pDC)
      {
         return ERROR;
      }

       if (hrgnUpdate)
       {
           NT_ASSERT(RgnUpdate == NULL);
           RgnUpdate = REGION_LockRgn(hrgnUpdate);
           if (!RgnUpdate)
           {
               DC_UnlockDc(pDC);
               return ERROR;
           }
       }

      /* Begin with the shifted and then clipped scroll rect */
      rcDst = rcScroll;
      RECTL_vOffsetRect(&rcDst, dx, dy);
      RECTL_bIntersectRect(&rcDst, &rcDst, &rcClip);
      if (RgnUpdate)
      {
         RgnOwn = RgnUpdate;
         REGION_SetRectRgn(RgnOwn, rcDst.left, rcDst.top, rcDst.right, rcDst.bottom);
      }
      else
      {
         RgnOwn = IntSysCreateRectpRgnIndirect(&rcDst);
      }

      /* Add the source rect */
      RgnTmp = IntSysCreateRectpRgnIndirect(&rcSrc);
      IntGdiCombineRgn(RgnOwn, RgnOwn, RgnTmp, RGN_OR);

      /* Substract the part of the dest that was visible in source */
      IntGdiCombineRgn(RgnTmp, RgnTmp, pDC->prgnVis, RGN_AND);
      REGION_bOffsetRgn(RgnTmp, dx, dy);
      Result = IntGdiCombineRgn(RgnOwn, RgnOwn, RgnTmp, RGN_DIFF);

      /* DO NOT Unlock DC while messing with prgnVis! */
      DC_UnlockDc(pDC);

      REGION_Delete(RgnTmp);

      if (prcUpdate)
      {
         REGION_GetRgnBox(RgnOwn, prcUpdate);
      }

      if (hrgnUpdate)
      {
         REGION_UnlockRgn(RgnUpdate);
      }
      else if (!RgnUpdate)
      {
         REGION_Delete(RgnOwn);
      }
   }
   else
      Result = NULLREGION;

   return Result;
}

/*
 * NtUserScrollDC
 *
 * Status
 *    @implemented
 */
BOOL APIENTRY
NtUserScrollDC(
   HDC hDC,
   INT dx,
   INT dy,
   const RECT *prcUnsafeScroll,
   const RECT *prcUnsafeClip,
   HRGN hrgnUpdate,
   LPRECT prcUnsafeUpdate)
{
   DECLARE_RETURN(DWORD);
   RECTL rcScroll, rcClip, rcUpdate;
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD Result;

   TRACE("Enter NtUserScrollDC\n");
   UserEnterExclusive();

   _SEH2_TRY
   {
      if (prcUnsafeScroll)
      {
         ProbeForRead(prcUnsafeScroll, sizeof(*prcUnsafeScroll), 1);
         rcScroll = *prcUnsafeScroll;
      }
      if (prcUnsafeClip)
      {
         ProbeForRead(prcUnsafeClip, sizeof(*prcUnsafeClip), 1);
         rcClip = *prcUnsafeClip;
      }
      if (prcUnsafeUpdate)
      {
         ProbeForWrite(prcUnsafeUpdate, sizeof(*prcUnsafeUpdate), 1);
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   Result = UserScrollDC( hDC,
                          dx,
                          dy,
                          prcUnsafeScroll? &rcScroll : 0,
                          prcUnsafeClip? &rcClip : 0,
                          hrgnUpdate,
                          NULL,
                          prcUnsafeUpdate? &rcUpdate : NULL);
   if(Result == ERROR)
   {
   	  /* FIXME: Only if hRgnUpdate is invalid we should SetLastError(ERROR_INVALID_HANDLE) */
      RETURN(FALSE);
   }

   if (prcUnsafeUpdate)
   {
      _SEH2_TRY
      {
         *prcUnsafeUpdate = rcUpdate;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END
      if (!NT_SUCCESS(Status))
      {
         /* FIXME: SetLastError? */
         /* FIXME: correct? We have already scrolled! */
         RETURN(FALSE);
      }
   }

   RETURN(TRUE);

CLEANUP:
   TRACE("Leave NtUserScrollDC, ret=%lu\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserScrollWindowEx
 *
 * Status
 *    @implemented
 */

DWORD APIENTRY
NtUserScrollWindowEx(
   HWND hWnd,
   INT dx,
   INT dy,
   const RECT *prcUnsafeScroll,
   const RECT *prcUnsafeClip,
   HRGN hrgnUpdate,
   LPRECT prcUnsafeUpdate,
   UINT flags)
{
   RECTL rcScroll, rcClip, rcCaret, rcUpdate;
   INT Result;
   PWND Window = NULL, CaretWnd;
   HDC hDC;
   PREGION RgnUpdate = NULL, RgnTemp, RgnWinupd = NULL;
   HWND hwndCaret;
   DWORD dcxflags = 0;
   int rdw_flags;
   NTSTATUS Status = STATUS_SUCCESS;
   DECLARE_RETURN(DWORD);
   USER_REFERENCE_ENTRY Ref, CaretRef;

   TRACE("Enter NtUserScrollWindowEx\n");
   UserEnterExclusive();

   Window = UserGetWindowObject(hWnd);
   if (!Window || !IntIsWindowDrawable(Window))
   {
      Window = NULL; /* prevent deref at cleanup */
      RETURN( ERROR);
   }
   UserRefObjectCo(Window, &Ref);

   IntGetClientRect(Window, &rcClip);

   _SEH2_TRY
   {
      if (prcUnsafeScroll)
      {
         ProbeForRead(prcUnsafeScroll, sizeof(*prcUnsafeScroll), 1);
         RECTL_bIntersectRect(&rcScroll, &rcClip, prcUnsafeScroll);
      }
      else
         rcScroll = rcClip;

      if (prcUnsafeClip)
      {
         ProbeForRead(prcUnsafeClip, sizeof(*prcUnsafeClip), 1);
         RECTL_bIntersectRect(&rcClip, &rcClip, prcUnsafeClip);
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(ERROR);
   }

   if (rcClip.right <= rcClip.left || rcClip.bottom <= rcClip.top ||
         (dx == 0 && dy == 0))
   {
      RETURN(NULLREGION);
   }

   /* We must use a copy of the region, as we can't hold an exclusive lock
    * on it while doing callouts to user-mode */
   RgnUpdate = IntSysCreateRectpRgn(0, 0, 0, 0);
   if(!RgnUpdate)
   {
       EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
       RETURN(ERROR);
   }

   if (hrgnUpdate)
   {
       RgnTemp = REGION_LockRgn(hrgnUpdate);
       if (!RgnTemp)
       {
           EngSetLastError(ERROR_INVALID_HANDLE);
           RETURN(ERROR);
       }
       IntGdiCombineRgn(RgnUpdate, RgnTemp, NULL, RGN_COPY);
       REGION_UnlockRgn(RgnTemp);
   }

   /* ScrollWindow uses the window DC, ScrollWindowEx doesn't */
   if (flags & SW_SCROLLWNDDCE)
   {
      dcxflags = DCX_USESTYLE;

      if (!(Window->pcls->style & (CS_OWNDC|CS_CLASSDC)))
         dcxflags |= DCX_CACHE; // AH??? wine~ If not Powned or with Class go Cheap!

      if (flags & SW_SCROLLCHILDREN && Window->style & WS_CLIPCHILDREN)
         dcxflags |= DCX_CACHE|DCX_NOCLIPCHILDREN;
   }
   else
   {
       /* So in this case ScrollWindowEx uses Cache DC. */
       dcxflags = DCX_CACHE|DCX_USESTYLE;
       if (flags & SW_SCROLLCHILDREN) dcxflags |= DCX_NOCLIPCHILDREN;
   }

   hDC = UserGetDCEx(Window, 0, dcxflags);
   if (!hDC)
   {
      /* FIXME: SetLastError? */
      RETURN(ERROR);
   }

   rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ? RDW_INVALIDATE | RDW_ERASE  : RDW_INVALIDATE ;

   rcCaret = rcScroll;
   hwndCaret = co_IntFixCaret(Window, &rcCaret, flags);

   Result = UserScrollDC( hDC,
                          dx,
                          dy,
                          &rcScroll,
                          &rcClip,
                          NULL,
                          RgnUpdate,
                          prcUnsafeUpdate? &rcUpdate : NULL);

   UserReleaseDC(Window, hDC, FALSE);

   /*
    * Take into account the fact that some damage may have occurred during
    * the scroll. Keep a copy in hrgnWinupd to be added to hrngUpdate at the end.
    */

   RgnTemp = IntSysCreateRectpRgn(0, 0, 0, 0);
   if (!RgnTemp)
   {
       EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
       RETURN(ERROR);
   }

   if (co_UserGetUpdateRgn(Window, RgnTemp, FALSE) != NULLREGION)
   {
      PREGION RgnClip = IntSysCreateRectpRgnIndirect(&rcClip);
      if (RgnClip)
      {
          if (hrgnUpdate)
          {
             RgnWinupd = IntSysCreateRectpRgn( 0, 0, 0, 0);
             IntGdiCombineRgn( RgnWinupd, RgnTemp, 0, RGN_COPY);
          }
          REGION_bOffsetRgn(RgnTemp, dx, dy);
          IntGdiCombineRgn(RgnTemp, RgnTemp, RgnClip, RGN_AND);
          if (hrgnUpdate)
              IntGdiCombineRgn( RgnWinupd, RgnWinupd, RgnTemp, RGN_OR );
          co_UserRedrawWindow(Window, NULL, RgnTemp, rdw_flags );
          REGION_Delete(RgnClip);
      }
   }
   REGION_Delete(RgnTemp);

   if (flags & SW_SCROLLCHILDREN)
   {
      PWND Child;
      RECTL rcChild;
      POINT ClientOrigin;
      USER_REFERENCE_ENTRY WndRef;
      RECTL rcDummy;

      IntGetClientOrigin(Window, &ClientOrigin);
      for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
      {
         rcChild = Child->rcWindow;
         rcChild.left -= ClientOrigin.x;
         rcChild.top -= ClientOrigin.y;
         rcChild.right -= ClientOrigin.x;
         rcChild.bottom -= ClientOrigin.y;

         if (! prcUnsafeScroll || RECTL_bIntersectRect(&rcDummy, &rcChild, &rcScroll))
         {
            UserRefObjectCo(Child, &WndRef);
            co_WinPosSetWindowPos(Child, 0, rcChild.left + dx, rcChild.top + dy, 0, 0,
                                  SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                  SWP_NOREDRAW | SWP_DEFERERASE);
            UserDerefObjectCo(Child);
         }
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
   {
      co_UserRedrawWindow(Window, NULL, RgnUpdate, rdw_flags |
                          ((flags & SW_ERASE) ? RDW_ERASENOW : 0) |
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0));
   }

   if (hwndCaret && (CaretWnd = UserGetWindowObject(hwndCaret)))
   {
      UserRefObjectCo(CaretWnd, &CaretRef);

      co_IntSetCaretPos(rcCaret.left + dx, rcCaret.top + dy);
      co_UserShowCaret(CaretWnd);

      UserDerefObjectCo(CaretWnd);
   }

   if (prcUnsafeUpdate)
   {
      _SEH2_TRY
      {
         /* Probe here, to not fail on invalid pointer before scrolling */
         ProbeForWrite(prcUnsafeUpdate, sizeof(*prcUnsafeUpdate), 1);
         *prcUnsafeUpdate = rcUpdate;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END

      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN(ERROR);
      }
   }

   RETURN(Result);

CLEANUP:
   if (hrgnUpdate && (_ret_ != ERROR))
   {
       /* Give everything back to the caller */
       RgnTemp = REGION_LockRgn(hrgnUpdate);
       /* The handle should still be valid */
       ASSERT(RgnTemp);
       if (RgnWinupd)
           IntGdiCombineRgn(RgnTemp, RgnUpdate, RgnWinupd, RGN_OR);
       else
           IntGdiCombineRgn(RgnTemp, RgnUpdate, NULL, RGN_COPY);
       REGION_UnlockRgn(RgnTemp);
   }

   if (RgnWinupd)
   {
       REGION_Delete(RgnWinupd);
   }

   if (RgnUpdate)
   {
      REGION_Delete(RgnUpdate);
   }

   if (Window)
      UserDerefObjectCo(Window);

   TRACE("Leave NtUserScrollWindowEx, ret=%lu\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

BOOL
UserDrawCaptionText(
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

   TRACE("UserDrawCaptionText: %wZ\n", Text);

   nclm.cbSize = sizeof(nclm);
   if(!UserSystemParametersInfo(SPI_GETNONCLIENTMETRICS,
      sizeof(NONCLIENTMETRICS), &nclm, 0))
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
   if(!hOldFont)
   {
      ERR("SelectFont() failed!\n");
      /* Don't fail */
   }

   if(uFlags & DC_INBUTTON)
      OldTextColor = IntGdiSetTextColor(hDc, IntGetSysColor(COLOR_BTNTEXT));
   else
      OldTextColor = IntGdiSetTextColor(hDc, IntGetSysColor(uFlags & DC_ACTIVE
         ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));

   // FIXME: If string doesn't fit to rc, truncate it and add ellipsis.
   GreGetTextExtentW(hDc, Text->Buffer, Text->Length/sizeof(WCHAR), &Size, 0);
   GreExtTextOutW(hDc,
                  lpRc->left, (lpRc->top + lpRc->bottom)/2 - Size.cy/2,
                  0, NULL, Text->Buffer, Text->Length/sizeof(WCHAR), NULL, 0);

   IntGdiSetTextColor(hDc, OldTextColor);
   if (hOldFont)
      NtGdiSelectFont(hDc, hOldFont);
   if (bDeleteFont)
      GreDeleteObject(hFont);

   return TRUE;
}

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

   if (!hIcon && pWnd != NULL)
   {
     HasIcon = (uFlags & DC_ICON) && (pWnd->style & WS_SYSMENU)
        && !(uFlags & DC_SMALLCAP) && !(pWnd->ExStyle & WS_EX_DLGMODALFRAME)
        && !(pWnd->ExStyle & WS_EX_TOOLWINDOW);
   }
   else
     HasIcon = (hIcon != 0);

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
         LONG y = (Rect.top + Rect.bottom)/2 - cy/2; // center
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
      Rect.left += 2;

      if (Str)
         UserDrawCaptionText(hDc, Str, &Rect, uFlags, hFont);
      else if (pWnd != NULL) // FIXME: Windows does not do that
      {
         UNICODE_STRING ustr;
         ustr.Buffer = pWnd->strName.Buffer; // FIXME: LARGE_STRING truncated!
         ustr.Length = (USHORT)min(pWnd->strName.Length, MAXUSHORT);
         ustr.MaximumLength = (USHORT)min(pWnd->strName.MaximumLength, MAXUSHORT);
         UserDrawCaptionText(hDc, &ustr, &Rect, uFlags, hFont);
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
            ERR("RealizePalette Desktop.");
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
      Ret = UserDrawCaption(pWnd, hDC, &SafeRect, hFont, hIcon, NULL, uFlags);

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
       if (!(Window = UserGetWindowObject(hwnd)) || // FIXME:
             Window == UserGetDesktopWindow() ||    // pWnd->fnid == FNID_DESKTOP
             Window == UserGetMessageWindow() )     // pWnd->fnid == FNID_MESSAGEWND
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
