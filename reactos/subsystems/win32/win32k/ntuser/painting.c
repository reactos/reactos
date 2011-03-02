/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Window painting function
 *  FILE:             subsystems/win32/win32k/ntuser/painting.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 *  REVISION HISTORY:
 *       06/06/2001   Created (?)
 *       18/11/2003   Complete rewrite
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

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
IntValidateParent(PWND Child, HRGN hValidateRgn, BOOL Recurse)
{
   PWND ParentWnd = Child->spwndParent;

   while (ParentWnd)
   {
      if (ParentWnd->style & WS_CLIPCHILDREN)
         break;

      if (ParentWnd->hrgnUpdate != 0)
      {
         if (Recurse)
            return FALSE;

         IntInvalidateWindows( ParentWnd,
                               hValidateRgn,
                               RDW_VALIDATE | RDW_NOCHILDREN);
      }

      ParentWnd = ParentWnd->spwndParent;
   }

   return TRUE;
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
      hRgnWindow = IntSysCreateRectRgnIndirect(&Wnd->rcClient);
   else
      hRgnWindow = IntSysCreateRectRgnIndirect(&Wnd->rcWindow);

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
         REGION_FreeRgnByHandle(hRgnNonClient);
         return HRGN_WINDOW;
      }

      RgnType = NtGdiCombineRgn(hRgnNonClient, hRgnNonClient,
                                hRgnWindow, RGN_DIFF);
      if (RgnType == ERROR)
      {
         REGION_FreeRgnByHandle(hRgnWindow);
         REGION_FreeRgnByHandle(hRgnNonClient);
         return HRGN_WINDOW;
      }
      else if (RgnType == NULLREGION)
      {
         REGION_FreeRgnByHandle(hRgnWindow);
         REGION_FreeRgnByHandle(hRgnNonClient);
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
            REGION_FreeRgnByHandle(Window->hrgnUpdate);
            Window->hrgnUpdate = NULL;
            if (!(Window->state & WNDS_INTERNALPAINT))
               MsqDecPaintCountQueue(Window->head.pti->MessageQueue);
         }
      }

      REGION_FreeRgnByHandle(hRgnWindow);

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

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      if (Wnd->hrgnUpdate)
      {
         if (!IntValidateParent(Wnd, Wnd->hrgnUpdate, Recurse))
            return;
      }

      if (Flags & RDW_UPDATENOW)
      {
         if (Wnd->hrgnUpdate != NULL ||
             Wnd->state & WNDS_INTERNALPAINT)
         {
            co_IntSendMessage(hWnd, WM_PAINT, 0, 0);
         }
      }
      else
      {
         if (Wnd->state & WNDS_SENDNCPAINT)
         {
            TempRegion = IntGetNCUpdateRgn(Wnd, TRUE);
            Wnd->state &= ~WNDS_SENDNCPAINT;
            MsqDecPaintCountQueue(Wnd->head.pti->MessageQueue);
            co_IntSendMessage(hWnd, WM_NCPAINT, (WPARAM)TempRegion, 0);

            if ( (HANDLE) 1 != TempRegion &&
                 NULL != TempRegion)
            {
               /* NOTE: The region can already be deleted! */
               GDIOBJ_FreeObjByHandle(TempRegion, GDI_OBJECT_TYPE_REGION | GDI_OBJECT_TYPE_SILENT);
            }
         }

         if (Wnd->state & WNDS_ERASEBACKGROUND)
         {
            if (Wnd->hrgnUpdate)
            {
               hDC = UserGetDCEx( Wnd,
                                  Wnd->hrgnUpdate,
                                  DCX_CACHE|DCX_USESTYLE|DCX_INTERSECTRGN|DCX_KEEPCLIPRGN);

               if (co_IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0))
               {
                  Wnd->state &= ~WNDS_ERASEBACKGROUND;
               }
               UserReleaseDC(Wnd, hDC, FALSE);
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
         ExFreePool(List);
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
IntInvalidateWindows(PWND Wnd, HRGN hRgn, ULONG Flags)
{
   INT RgnType;
   BOOL HadPaintMessage, HadNCPaintMessage;
   BOOL HasPaintMessage, HasNCPaintMessage;

   DPRINT("IntInvalidateWindows start\n");
   /*
    * If the nonclient is not to be redrawn, clip the region to the client
    * rect
    */
   if (0 != (Flags & RDW_INVALIDATE) && 0 == (Flags & RDW_FRAME))
   {
      HRGN hRgnClient;

      hRgnClient = IntSysCreateRectRgnIndirect(&Wnd->rcClient);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnClient, RGN_AND);
      REGION_FreeRgnByHandle(hRgnClient);
   }

   /*
    * Clip the given region with window rectangle (or region)
    */

   if (!Wnd->hrgnClip || (Wnd->style & WS_MINIMIZE))
   {
      HRGN hRgnWindow;

      hRgnWindow = IntSysCreateRectRgnIndirect(&Wnd->rcWindow);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnWindow, RGN_AND);
      REGION_FreeRgnByHandle(hRgnWindow);
   }
   else
   {
      NtGdiOffsetRgn( hRgn,
                     -Wnd->rcWindow.left,
                     -Wnd->rcWindow.top);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, Wnd->hrgnClip, RGN_AND);
      NtGdiOffsetRgn( hRgn,
                      Wnd->rcWindow.left,
                      Wnd->rcWindow.top);
   }

   /*
    * Save current state of pending updates
    */

   HadPaintMessage = Wnd->hrgnUpdate != NULL ||
                     Wnd->state & WNDS_INTERNALPAINT;
   HadNCPaintMessage = Wnd->state & WNDS_SENDNCPAINT;

   /*
    * Update the region and flags
    */

   if (Flags & RDW_INVALIDATE && RgnType != NULLREGION)
   {
      if (Wnd->hrgnUpdate == NULL)
      {
         Wnd->hrgnUpdate = IntSysCreateRectRgn(0, 0, 0, 0);
         IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_PUBLIC);
      }

      if (NtGdiCombineRgn(Wnd->hrgnUpdate, Wnd->hrgnUpdate,
                          hRgn, RGN_OR) == NULLREGION)
      {
         IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
         REGION_FreeRgnByHandle(Wnd->hrgnUpdate);
         Wnd->hrgnUpdate = NULL;
      }

      if (Flags & RDW_FRAME)
         Wnd->state |= WNDS_SENDNCPAINT;
      if (Flags & RDW_ERASE)
         Wnd->state |= WNDS_ERASEBACKGROUND;

      Flags |= RDW_FRAME;
   }

   if (Flags & RDW_VALIDATE && RgnType != NULLREGION)
   {
      if (Wnd->hrgnUpdate != NULL)
      {
         if (NtGdiCombineRgn(Wnd->hrgnUpdate, Wnd->hrgnUpdate,
                             hRgn, RGN_DIFF) == NULLREGION)
         {
            IntGdiSetRegionOwner(Wnd->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
            REGION_FreeRgnByHandle(Wnd->hrgnUpdate);
            Wnd->hrgnUpdate = NULL;
         }
      }

      if (Wnd->hrgnUpdate == NULL)
         Wnd->state &= ~WNDS_ERASEBACKGROUND;
      if (Flags & RDW_NOFRAME)
         Wnd->state &= ~WNDS_SENDNCPAINT;
      if (Flags & RDW_NOERASE)
         Wnd->state &= ~WNDS_ERASEBACKGROUND;
   }

   if (Flags & RDW_INTERNALPAINT)
   {
      Wnd->state |= WNDS_INTERNALPAINT;
   }

   if (Flags & RDW_NOINTERNALPAINT)
   {
      Wnd->state &= ~WNDS_INTERNALPAINT;
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
            HRGN hRgnTemp = IntSysCreateRectRgn(0, 0, 0, 0);
            NtGdiCombineRgn(hRgnTemp, hRgn, 0, RGN_COPY);
            IntInvalidateWindows(Child, hRgnTemp, Flags);
            REGION_FreeRgnByHandle(hRgnTemp);
         }

      }
   }

   /*
    * Fake post paint messages to window message queue if needed
    */

   HasPaintMessage = Wnd->hrgnUpdate != NULL ||
                     Wnd->state & WNDS_INTERNALPAINT;
   HasNCPaintMessage = Wnd->state & WNDS_SENDNCPAINT;

   if (HasPaintMessage != HadPaintMessage)
   {
      if (HadPaintMessage)
         MsqDecPaintCountQueue(Wnd->head.pti->MessageQueue);
      else
         MsqIncPaintCountQueue(Wnd->head.pti->MessageQueue);
   }

   if (HasNCPaintMessage != HadNCPaintMessage)
   {
      if (HadNCPaintMessage)
         MsqDecPaintCountQueue(Wnd->head.pti->MessageQueue);
      else
         MsqIncPaintCountQueue(Wnd->head.pti->MessageQueue);
   }
   DPRINT("IntInvalidateWindows exit\n");
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
   HRGN UpdateRgn,
   ULONG Flags)
{
   HRGN hRgn = NULL;
   DPRINT("co_UserRedrawWindow start\n");

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
      if (UpdateRgn != NULL)
      {
         hRgn = IntSysCreateRectRgn(0, 0, 0, 0);
         if (NtGdiCombineRgn(hRgn, UpdateRgn, NULL, RGN_COPY) == NULLREGION)
         {
            REGION_FreeRgnByHandle(hRgn);
            hRgn = NULL;
         }
         else
            NtGdiOffsetRgn(hRgn, Window->rcClient.left, Window->rcClient.top);
      }
      else if (UpdateRect != NULL)
      {
         if (!RECTL_bIsEmptyRect(UpdateRect))
         {
            hRgn = IntSysCreateRectRgnIndirect((RECTL *)UpdateRect);
            NtGdiOffsetRgn(hRgn, Window->rcClient.left, Window->rcClient.top);
         }
      }
      else if ((Flags & (RDW_INVALIDATE | RDW_FRAME)) == (RDW_INVALIDATE | RDW_FRAME) ||
               (Flags & (RDW_VALIDATE | RDW_NOFRAME)) == (RDW_VALIDATE | RDW_NOFRAME))
      {
         if (!RECTL_bIsEmptyRect(&Window->rcWindow))
            hRgn = IntSysCreateRectRgnIndirect(&Window->rcWindow);
      }
      else
      {
         if (!RECTL_bIsEmptyRect(&Window->rcClient))
            hRgn = IntSysCreateRectRgnIndirect(&Window->rcClient);
      }
   }

   /*
    * Step 3.
    * Adjust the window update region depending on hRgn and flags.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE | RDW_INTERNALPAINT | RDW_NOINTERNALPAINT) &&
       hRgn != NULL)
   {
      IntInvalidateWindows(Window, hRgn, Flags);
   }

   /*
    * Step 4.
    * Repaint and erase windows if needed.
    */

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      co_IntPaintWindows(Window, Flags, FALSE);
   }

   /*
    * Step 5.
    * Cleanup ;-)
    */

   if (hRgn != NULL)
   {
      REGION_FreeRgnByHandle(hRgn);
   }
   DPRINT("co_UserRedrawWindow exit\n");

   return TRUE;
}

BOOL FASTCALL
IntIsWindowDirty(PWND Wnd)
{
   return (Wnd->style & WS_VISIBLE) &&
          ((Wnd->hrgnUpdate != NULL) ||
           (Wnd->state & WNDS_INTERNALPAINT) ||
           (Wnd->state & WNDS_SENDNCPAINT));
}

HWND FASTCALL
IntFindWindowToRepaint(PWND Window, PTHREADINFO Thread)
{
   HWND hChild;
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
                  return TempWindow->head.h;
               }
            }
         }

         return Window->head.h;
      }

      if (Window->spwndChild)
      {
         hChild = IntFindWindowToRepaint(Window->spwndChild, Thread);
         if (hChild != NULL)
            return hChild;
      }
   }

   return NULL;
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
   if (!Thread->cPaintsReady)
      return FALSE;

   if ((MsgFilterMin != 0 || MsgFilterMax != 0) &&
         (MsgFilterMin > WM_PAINT || MsgFilterMax < WM_PAINT))
      return FALSE;

   Message->hwnd = IntFindWindowToRepaint(UserGetDesktopWindow(), PsGetCurrentThreadWin32Thread());

   if (Message->hwnd == NULL)
   {
      DPRINT1("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found! Counts %d\n",Thread->cPaintsReady);
      /* Hack to stop spamming the debuglog ! */
      Thread->cPaintsReady = 0;
      return FALSE;
   }

   if (Window != NULL && Message->hwnd != Window->head.h)
      return FALSE;

   Message->message = WM_PAINT;
   Message->wParam = Message->lParam = 0;

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

   WndCaret = UserGetWindowObject(hWndCaret);

   //fix: check for WndCaret can be null
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
   DECLARE_RETURN(HDC);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserBeginPaint\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( NULL);
   }

   UserRefObjectCo(Window, &Ref);

   co_UserHideCaret(Window);

   if (Window->state & WNDS_SENDNCPAINT)
   {
      HRGN hRgn;

      hRgn = IntGetNCUpdateRgn(Window, FALSE);
      Window->state &= ~WNDS_SENDNCPAINT;
      MsqDecPaintCountQueue(Window->head.pti->MessageQueue);
      co_IntSendMessage(hWnd, WM_NCPAINT, (WPARAM)hRgn, 0);
      if (hRgn != (HANDLE)1 && hRgn != NULL)
      {
         /* NOTE: The region can already by deleted! */
         GDIOBJ_FreeObjByHandle(hRgn, GDI_OBJECT_TYPE_REGION | GDI_OBJECT_TYPE_SILENT);
      }
   }

   RtlZeroMemory(&Ps, sizeof(PAINTSTRUCT));

   Ps.hdc = UserGetDCEx( Window,
                         Window->hrgnUpdate,
                         DCX_INTERSECTRGN | DCX_USESTYLE);
   if (!Ps.hdc)
   {
      RETURN(NULL);
   }

   if (Window->hrgnUpdate != NULL)
   {
      MsqDecPaintCountQueue(Window->head.pti->MessageQueue);
      GdiGetClipBox(Ps.hdc, &Ps.rcPaint);
      IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
      /* The region is part of the dc now and belongs to the process! */
      Window->hrgnUpdate = NULL;
   }
   else
   {
      if (Window->state & WNDS_INTERNALPAINT)
         MsqDecPaintCountQueue(Window->head.pti->MessageQueue);

      IntGetClientRect(Window, &Ps.rcPaint);
   }

   Window->state &= ~WNDS_INTERNALPAINT;

   if (Window->state & WNDS_ERASEBACKGROUND)
   {
      Window->state &= ~WNDS_ERASEBACKGROUND;
      Ps.fErase = !co_IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)Ps.hdc, 0);
   }
   else
   {
      Ps.fErase = FALSE;
   }
   if (Window->hrgnUpdate)
   {
      if (!(Window->style & WS_CLIPCHILDREN))
      {
         PWND Child;
         for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
         {
            IntInvalidateWindows(Child, Window->hrgnUpdate, RDW_FRAME | RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
         }
      }
   }

   Status = MmCopyToCaller(UnsafePs, &Ps, sizeof(PAINTSTRUCT));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(NULL);
   }

   RETURN(Ps.hdc);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);

   DPRINT("Leave NtUserBeginPaint, ret=%i\n",_ret_);
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
   PWND Window;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;
   HDC hdc = NULL;

   DPRINT("Enter NtUserEndPaint\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   _SEH2_TRY
   {
      ProbeForRead(pUnsafePs, sizeof(*pUnsafePs), 1);
      hdc = pUnsafePs->hdc;
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

   UserReleaseDC(Window, hdc, TRUE);

   UserRefObjectCo(Window, &Ref);
   co_UserShowCaret(Window);
   UserDerefObjectCo(Window);

   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserEndPaint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


INT FASTCALL
co_UserGetUpdateRgn(PWND Window, HRGN hRgn, BOOL bErase)
{
   int RegionType;
   RECTL Rect;

   ASSERT_REFS_CO(Window);

   if (Window->hrgnUpdate == NULL)
   {
      RegionType = (NtGdiSetRectRgn(hRgn, 0, 0, 0, 0) ? NULLREGION : ERROR);
   }
   else
   {
      Rect = Window->rcClient;
      IntIntersectWithParents(Window, &Rect);
      NtGdiSetRectRgn(hRgn, Rect.left, Rect.top, Rect.right, Rect.bottom);
      RegionType = NtGdiCombineRgn(hRgn, hRgn, Window->hrgnUpdate, RGN_AND);
      NtGdiOffsetRgn(hRgn, -Window->rcClient.left, -Window->rcClient.top);
   }

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

   DPRINT("Enter NtUserGetUpdateRgn\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(ERROR);
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_UserGetUpdateRgn(Window, hRgn, bErase);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserGetUpdateRgn, ret=%i\n",_ret_);
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
   PROSRGNDATA RgnData;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetUpdateRect\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

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
         RgnData = RGNOBJAPI_Lock(Window->hrgnUpdate, NULL);
         ASSERT(RgnData != NULL);
         RegionType = REGION_GetRgnBox(RgnData, &Rect);
         RGNOBJAPI_Unlock(RgnData);

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
   DPRINT("Leave NtUserGetUpdateRect, ret=%i\n",_ret_);
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
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserRedrawWindow\n");
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

   UserRefObjectCo(Wnd, &Ref);

   Ret = co_UserRedrawWindow( Wnd,
                              lprcUpdate ? &SafeUpdateRect : NULL,
                              hrgnUpdate,
                              flags);

   UserDerefObjectCo(Wnd);

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserRedrawWindow, ret=%i\n",_ret_);
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
   if (hrgnUpdate || prcUpdate)
   {
      HRGN hrgnOwn, hrgnVisible, hrgnTmp;

      pDC = DC_LockDc(hDC);
      if (!pDC)
      {
         return FALSE;
      }
      hrgnVisible = ((PROSRGNDATA)pDC->prgnVis)->BaseObject.hHmgr;  // pDC->prgnRao?

      /* Begin with the shifted and then clipped scroll rect */
      rcDst = rcScroll;
      RECTL_vOffsetRect(&rcDst, dx, dy);
      RECTL_bIntersectRect(&rcDst, &rcDst, &rcClip);
      if (hrgnUpdate)
      {
         hrgnOwn = hrgnUpdate;
         if (!NtGdiSetRectRgn(hrgnOwn, rcDst.left, rcDst.top, rcDst.right, rcDst.bottom))
         {
            return ERROR;
         }
      }
      else
      {
         hrgnOwn = IntSysCreateRectRgnIndirect(&rcDst);
      }

      /* Add the source rect */
      hrgnTmp = IntSysCreateRectRgnIndirect(&rcSrc);
      NtGdiCombineRgn(hrgnOwn, hrgnOwn, hrgnTmp, RGN_OR);

      /* Substract the part of the dest that was visible in source */
      NtGdiCombineRgn(hrgnTmp, hrgnTmp, hrgnVisible, RGN_AND);
      NtGdiOffsetRgn(hrgnTmp, dx, dy);
      Result = NtGdiCombineRgn(hrgnOwn, hrgnOwn, hrgnTmp, RGN_DIFF);

	  /* DO NOT Unlock DC while messing with prgnVis! */
	  DC_UnlockDc(pDC);

      REGION_FreeRgnByHandle(hrgnTmp);

      if (prcUpdate)
      {
         IntGdiGetRgnBox(hrgnOwn, prcUpdate);
      }

      if (!hrgnUpdate)
      {
         REGION_FreeRgnByHandle(hrgnOwn);
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

   DPRINT("Enter NtUserScrollDC\n");
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
   DPRINT("Leave NtUserScrollDC, ret=%i\n",_ret_);
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
   HRGN hrgnOwn = NULL, hrgnTemp;
   HWND hwndCaret;
   NTSTATUS Status = STATUS_SUCCESS;
   DECLARE_RETURN(DWORD);
   USER_REFERENCE_ENTRY Ref, CaretRef;

   DPRINT("Enter NtUserScrollWindowEx\n");
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

   if (hrgnUpdate)
      hrgnOwn = hrgnUpdate;
   else
      hrgnOwn = IntSysCreateRectRgn(0, 0, 0, 0);

   hDC = UserGetDCEx(Window, 0, DCX_CACHE | DCX_USESTYLE);
   if (!hDC)
   {
      /* FIXME: SetLastError? */
      RETURN(ERROR);
   }

   rcCaret = rcScroll;
   hwndCaret = co_IntFixCaret(Window, &rcCaret, flags);

   Result = UserScrollDC( hDC,
                          dx,
                          dy,
                         &rcScroll,
                         &rcClip,
                          hrgnOwn,
                          prcUnsafeUpdate? &rcUpdate : NULL);

   UserReleaseDC(Window, hDC, FALSE);

   /*
    * Take into account the fact that some damage may have occurred during
    * the scroll.
    */

   hrgnTemp = IntSysCreateRectRgn(0, 0, 0, 0);
   if (co_UserGetUpdateRgn(Window, hrgnTemp, FALSE) != NULLREGION)
   {
      HRGN hrgnClip = IntSysCreateRectRgnIndirect(&rcClip);
      NtGdiOffsetRgn(hrgnTemp, dx, dy);
      NtGdiCombineRgn(hrgnTemp, hrgnTemp, hrgnClip, RGN_AND);
      co_UserRedrawWindow(Window, NULL, hrgnTemp, RDW_INVALIDATE | RDW_ERASE);
      REGION_FreeRgnByHandle(hrgnClip);
   }
   REGION_FreeRgnByHandle(hrgnTemp);

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
                                  SWP_NOREDRAW);
            UserDerefObjectCo(Child);
         }
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
   {
      co_UserRedrawWindow(Window, NULL, hrgnOwn, RDW_INVALIDATE | RDW_ERASE |
                          ((flags & SW_ERASE) ? RDW_ERASENOW : 0) |
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0));
   }

   if ((CaretWnd = UserGetWindowObject(hwndCaret)))
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
   if (hrgnOwn && !hrgnUpdate)
   {
      REGION_FreeRgnByHandle(hrgnOwn);
   }

   if (Window)
      UserDerefObjectCo(Window);

   DPRINT("Leave NtUserScrollWindowEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
UserDrawSysMenuButton(
   PWND pWnd,
   HDC hDc,
   RECTL *lpRc,
   BOOL Down)
{
   HICON hIcon;
   PCURICON_OBJECT pIcon;

   ASSERT(pWnd && lpRc);

   /* Get the icon to draw. We don't care about WM_GETICON here. */

   hIcon = pWnd->pcls->hIconSm;

   if(!hIcon)
   {
      DPRINT("Wnd class has no small icon.\n");
      hIcon = pWnd->pcls->hIcon;
   }

   if(!hIcon)
   {
      DPRINT("Wnd class hasn't any icon.\n");
      //FIXME: Draw "winlogo" icon.
      return FALSE;
   }

   if(!(pIcon = UserGetCurIconObject(hIcon)))
   {
      DPRINT1("UserGetCurIconObject() failed!\n");
      return FALSE;
   }

   return UserDrawIconEx(hDc, lpRc->left, lpRc->top, pIcon,
                        UserGetSystemMetrics(SM_CXSMICON),
                        UserGetSystemMetrics(SM_CYSMICON),
                        0, NULL, DI_NORMAL);
}

BOOL
UserDrawCaptionText(
   HDC hDc,
   const PUNICODE_STRING Text,
   const RECTL *lpRc,
   UINT uFlags)
{
   HFONT hOldFont = NULL, hFont = NULL;
   COLORREF OldTextColor;
   NONCLIENTMETRICSW nclm;
   NTSTATUS Status;
   #ifndef NDEBUG
   INT i;
   DPRINT("%s:", __FUNCTION__);
   for(i = 0; i < Text->Length/sizeof(WCHAR); i++)
      DbgPrint("%C", Text->Buffer[i]);
   DbgPrint(", %d\n", Text->Length/sizeof(WCHAR));
   #endif

   nclm.cbSize = sizeof(nclm);
   if(!UserSystemParametersInfo(SPI_GETNONCLIENTMETRICS,
      sizeof(NONCLIENTMETRICS), &nclm, 0))
   {
      DPRINT1("%s: UserSystemParametersInfo() failed!\n", __FUNCTION__);
      return FALSE;
   }

   IntGdiSetBkMode(hDc, TRANSPARENT);

   if(uFlags & DC_SMALLCAP)
      Status = TextIntCreateFontIndirect(&nclm.lfSmCaptionFont, &hFont);
   else Status = TextIntCreateFontIndirect(&nclm.lfCaptionFont, &hFont);

   if(!NT_SUCCESS(Status))
   {
      DPRINT1("%s: TextIntCreateFontIndirect() failed! Status: 0x%x\n",
         __FUNCTION__, Status);
      return FALSE;
   }

   hOldFont = NtGdiSelectFont(hDc, hFont);
   if(!hOldFont)
   {
      DPRINT1("%s: SelectFont() failed!\n", __FUNCTION__);
      GreDeleteObject(hFont);
      return FALSE;
   }

   if(uFlags & DC_INBUTTON)
      OldTextColor = IntGdiSetTextColor(hDc, IntGetSysColor(COLOR_BTNTEXT));
   else OldTextColor = IntGdiSetTextColor(hDc, IntGetSysColor(uFlags & DC_ACTIVE
         ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));

   //FIXME: If string doesn't fit to rc, truncate it and add ellipsis.

   GreExtTextOutW(hDc, lpRc->left,
      lpRc->top, 0, NULL, Text->Buffer,
      Text->Length/sizeof(WCHAR), NULL, 0);

   IntGdiSetTextColor(hDc, OldTextColor);
   NtGdiSelectFont(hDc, hOldFont);
   GreDeleteObject(hFont);

   return TRUE;
}

BOOL UserDrawCaption(
   PWND pWnd,
   HDC hDc,
   RECTL *lpRc,
   HFONT hFont,
   HICON hIcon,
   const PUNICODE_STRING str,
   UINT uFlags)
{
   BOOL Ret = FALSE;
   HBITMAP hMemBmp = NULL, hOldBmp = NULL;
   HBRUSH hOldBrush = NULL;
   HDC hMemDc = NULL;
   ULONG Height;
   UINT VCenter = 0, Padding = 0;
   RECTL r = *lpRc;
   LONG ButtonWidth, IconWidth;
   BOOL HasIcon;

   //ASSERT(pWnd != NULL);

   RECTL_vMakeWellOrdered(lpRc);
   hMemBmp = NtGdiCreateCompatibleBitmap(hDc,
      lpRc->right - lpRc->left,
      lpRc->bottom - lpRc->top);

   if(!hMemBmp)
   {
      DPRINT1("%s: NtGdiCreateCompatibleBitmap() failed!\n", __FUNCTION__);
      return FALSE;
   }

   hMemDc = NtGdiCreateCompatibleDC(hDc);
   if(!hMemDc)
   {
      DPRINT1("%s: NtGdiCreateCompatibleDC() failed!\n", __FUNCTION__);
      goto cleanup;
   }

   hOldBmp = NtGdiSelectBitmap(hMemDc, hMemBmp);
   if(!hOldBmp)
   {
      DPRINT1("%s: NtGdiSelectBitmap() failed!\n", __FUNCTION__);
      goto cleanup;
   }

   Height = UserGetSystemMetrics(SM_CYCAPTION) - 1;
   VCenter = (lpRc->bottom - lpRc->top) / 2;
   Padding = VCenter - (Height / 2);

   if ((!hIcon) && (pWnd != NULL))
   {
     HasIcon = (uFlags & DC_ICON) && (pWnd->style & WS_SYSMENU)
        && !(uFlags & DC_SMALLCAP) && !(pWnd->ExStyle & WS_EX_DLGMODALFRAME)
        && !(pWnd->ExStyle & WS_EX_TOOLWINDOW);
   }
   else
     HasIcon = (hIcon != 0);

   IconWidth = UserGetSystemMetrics(SM_CXSIZE) + Padding;

   r.left = Padding;
   r.right = r.left + (lpRc->right - lpRc->left);
   r.top = Padding;
   r.bottom = r.top + (Height / 2);

   // Draw the caption background
   if(uFlags & DC_INBUTTON)
   {
      hOldBrush = NtGdiSelectBrush(hMemDc,
         IntGetSysColorBrush(COLOR_3DFACE));

      if(!hOldBrush)
      {
         DPRINT1("%s: NtGdiSelectBrush() failed!\n", __FUNCTION__);
         goto cleanup;
      }

      if(!NtGdiPatBlt(hMemDc, 0, 0,
         lpRc->right - lpRc->left,
         lpRc->bottom - lpRc->top,
         PATCOPY))
      {
         DPRINT1("%s: NtGdiPatBlt() failed!\n", __FUNCTION__);
         goto cleanup;
      }

      if(HasIcon) r.left+=IconWidth;
   }
   else
   {
      r.right = (lpRc->right - lpRc->left);
      if(uFlags & DC_SMALLCAP)
         ButtonWidth = UserGetSystemMetrics(SM_CXSMSIZE) - 2;
      else ButtonWidth = UserGetSystemMetrics(SM_CXSIZE) - 2;

      hOldBrush = NtGdiSelectBrush(hMemDc,
         IntGetSysColorBrush(uFlags & DC_ACTIVE ?
            COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));

      if(!hOldBrush)
      {
         DPRINT1("%s: NtGdiSelectBrush() failed!\n", __FUNCTION__);
         goto cleanup;
      }

      if(HasIcon && (uFlags & DC_GRADIENT))
      {
         NtGdiPatBlt(hMemDc, 0, 0,
            IconWidth+1,
            lpRc->bottom - lpRc->top,
            PATCOPY);
         r.left+=IconWidth;
      }
      else
      {
         NtGdiPatBlt(hMemDc, 0, 0,
            lpRc->right - lpRc->left,
            lpRc->bottom - lpRc->top,
            PATCOPY);
      }

      if(uFlags & DC_GRADIENT)
      {
         static GRADIENT_RECT gcap = {0, 1};
         TRIVERTEX vert[2];
         COLORREF Colors[2];

		 if (pWnd != NULL)
		 {
			 if(pWnd->style & WS_SYSMENU)
			 {
				r.right -= 3 + ButtonWidth;
				if(!(uFlags & DC_SMALLCAP))
				{
				   if(pWnd->style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
					  r.right -= 2 + 2 * ButtonWidth;
				   else r.right -= 2;
				   r.right -= 2;
				}

				//Draw buttons background
				if(!NtGdiSelectBrush(hMemDc,
				   IntGetSysColorBrush(uFlags & DC_ACTIVE ?
					  COLOR_GRADIENTACTIVECAPTION:COLOR_GRADIENTINACTIVECAPTION)))
				{
				   DPRINT1("%s: NtGdiSelectBrush() failed!\n", __FUNCTION__);
				   goto cleanup;
				}

				NtGdiPatBlt(hMemDc,
				   r.right,
				   0,
				   lpRc->right - lpRc->left - r.right,
				   lpRc->bottom - lpRc->top,
				   PATCOPY);
			 }
		 }

         Colors[0] = IntGetSysColor((uFlags & DC_ACTIVE) ?
            COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);

         Colors[1] = IntGetSysColor((uFlags & DC_ACTIVE) ?
            COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);

         vert[0].x = r.left;
         vert[0].y = 0;
         vert[0].Red = (WORD)Colors[0]<<8;
         vert[0].Green = (WORD)Colors[0] & 0xFF00;
         vert[0].Blue = (WORD)(Colors[0]>>8) & 0xFF00;
         vert[0].Alpha = 0;

         vert[1].x = r.right;
         vert[1].y = lpRc->bottom - lpRc->top;
         vert[1].Red = (WORD)Colors[1]<<8;
         vert[1].Green = (WORD)Colors[1] & 0xFF00;
         vert[1].Blue = (WORD)(Colors[1]>>8) & 0xFF00;
         vert[1].Alpha = 0;

         if(!GreGradientFill(hMemDc, vert, 2, &gcap,
            1, GRADIENT_FILL_RECT_H))
         {
            DPRINT1("%s: IntGdiGradientFill() failed!\n", __FUNCTION__);
         }

      } //if(uFlags & DC_GRADIENT)
   }

   if(HasIcon)
   {
      r.top ++;
      r.left -= --IconWidth;
	  /* FIXME: Draw the Icon when pWnd == NULL but  hIcon is valid */
	  if (pWnd != NULL)
		UserDrawSysMenuButton(pWnd, hMemDc, &r, FALSE);

      r.left += IconWidth;
      r.top --;
   }

   r.top ++;
   r.left += 2;

   r.bottom = r.top + Height;

   if((uFlags & DC_TEXT))
   {
      if(!(uFlags & DC_GRADIENT))
      {
         r.right = (lpRc->right - lpRc->left);

         if(uFlags & DC_SMALLCAP)
            ButtonWidth = UserGetSystemMetrics(SM_CXSMSIZE) - 2;
         else ButtonWidth = UserGetSystemMetrics(SM_CXSIZE) - 2;

         if ((pWnd != NULL) && (pWnd->style & WS_SYSMENU))
         {
            r.right -= 3 + ButtonWidth;
            if(! (uFlags & DC_SMALLCAP))
            {
               if(pWnd->style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
                  r.right -= 2 + 2 * ButtonWidth;
               else r.right -= 2;
               r.right -= 2;
            }
         }
      }

	  /* FIXME: hFont isn't handled */
      if (str)
         UserDrawCaptionText(hMemDc, str, &r, uFlags);
      else if (pWnd != NULL)
      {
         UNICODE_STRING ustr;
         ustr.Buffer = pWnd->strName.Buffer;
         ustr.Length = pWnd->strName.Length;
         ustr.MaximumLength = pWnd->strName.MaximumLength;
         UserDrawCaptionText(hMemDc, &ustr, &r, uFlags);
      }
   }

   if(!NtGdiBitBlt(hDc, lpRc->left, lpRc->top,
      lpRc->right - lpRc->left, lpRc->bottom - lpRc->top,
      hMemDc, 0, 0, SRCCOPY, 0, 0))
   {
      DPRINT1("%s: NtGdiBitBlt() failed!\n", __FUNCTION__);
      goto cleanup;
   }

   Ret = TRUE;

cleanup:
   if (hOldBrush) NtGdiSelectBrush(hMemDc, hOldBrush);
   if (hOldBmp) NtGdiSelectBitmap(hMemDc, hOldBmp);
   if (hMemBmp) GreDeleteObject(hMemBmp);
   if (hMemDc) NtGdiDeleteObjectApp(hMemDc);

   return Ret;
}

INT
FASTCALL
UserRealizePalette(HDC hdc)
{
  HWND hWnd;
  DWORD Ret;

  Ret = IntGdiRealizePalette(hdc);
  if (Ret) // There was a change.
  {
      hWnd = IntWindowFromDC(hdc);
      if (hWnd) // Send broadcast if dc is associated with a window.
      {  // FYI: Thread locked in CallOneParam.
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
    return NtUserRedrawWindow(hWnd, lpUnsafeRect, NULL, RDW_INVALIDATE | (bErase? RDW_ERASE : 0));
}

BOOL
APIENTRY
NtUserInvalidateRgn(
    HWND hWnd,
    HRGN hRgn,
    BOOL bErase)
{
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
       Window = UserGetWindowObject(hwnd);
       // TODO: Add Desktop and MessageBox check via FNID's.
       if ( Window )
       {
          /* Validate flags and check it as a mask for 0 or 1. */
          if ( (nFlags & PW_CLIENTONLY) == nFlags)
             Ret = IntPrintWindow( Window, hdcBlt, nFlags);
          else
             EngSetLastError(ERROR_INVALID_PARAMETER);
       }
    }

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
    if (hWnd)
    {
       return NtUserRedrawWindow(hWnd, lpRect, NULL, RDW_VALIDATE );
    }
    return NtUserRedrawWindow(hWnd, lpRect, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_ERASENOW|RDW_ALLCHILDREN);
}

/* EOF */
