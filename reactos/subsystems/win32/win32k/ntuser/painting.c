/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Window painting function
 *  FILE:             subsys/win32k/ntuser/painting.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 *  REVISION HISTORY:
 *       06/06/2001   Created (?)
 *       18/11/2003   Complete rewrite
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

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
IntIntersectWithParents(PWINDOW_OBJECT Child, RECTL *WindowRect)
{
   PWINDOW_OBJECT ParentWindow;
   PWND ParentWnd;

   ParentWindow = Child->spwndParent;
   while (ParentWindow != NULL)
   {
      ParentWnd = ParentWindow->Wnd;
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

      ParentWindow = ParentWindow->spwndParent;
   }

   return TRUE;
}

BOOL FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN hValidateRgn, BOOL Recurse)
{
   PWINDOW_OBJECT ParentWindow = Child->spwndParent;
   PWND ParentWnd;

   while (ParentWindow)
   {
      ParentWnd = ParentWindow->Wnd;
      if (ParentWnd->style & WS_CLIPCHILDREN)
         break;

      if (ParentWindow->hrgnUpdate != 0)
      {
         if (Recurse)
            return FALSE;

         IntInvalidateWindows( ParentWindow,
                               hValidateRgn,
                               RDW_VALIDATE | RDW_NOCHILDREN);
      }

      ParentWindow = ParentWindow->spwndParent;
   }

   return TRUE;
}

/**
 * @name IntCalcWindowRgn
 *
 * Get a window or client region.
 */

HRGN FASTCALL
IntCalcWindowRgn(PWINDOW_OBJECT Window, BOOL Client)
{
   PWND Wnd;
   HRGN hRgnWindow;
   UINT RgnType;

   Wnd = Window->Wnd;
   if (Client)
      hRgnWindow = IntSysCreateRectRgnIndirect(&Wnd->rcClient);
   else
      hRgnWindow = IntSysCreateRectRgnIndirect(&Wnd->rcWindow);

   if (Window->hrgnClip != NULL && !(Wnd->style & WS_MINIMIZE))
   {
      NtGdiOffsetRgn(hRgnWindow,
         -Wnd->rcWindow.left,
         -Wnd->rcWindow.top);
      RgnType = NtGdiCombineRgn(hRgnWindow, hRgnWindow, Window->hrgnClip, RGN_AND);
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
IntGetNCUpdateRgn(PWINDOW_OBJECT Window, BOOL Validate)
{
   HRGN hRgnNonClient;
   HRGN hRgnWindow;
   UINT RgnType;

   if (Window->hrgnUpdate != NULL &&
       Window->hrgnUpdate != (HRGN)1)
   {
      hRgnNonClient = IntCalcWindowRgn(Window, FALSE);

      /*
       * If region creation fails it's safe to fallback to whole
       * window region.
       */
      if (hRgnNonClient == NULL)
      {
         return (HRGN)1;
      }

      hRgnWindow = IntCalcWindowRgn(Window, TRUE);
      if (hRgnWindow == NULL)
      {
         REGION_FreeRgnByHandle(hRgnNonClient);
         return (HRGN)1;
      }

      RgnType = NtGdiCombineRgn(hRgnNonClient, hRgnNonClient,
                                hRgnWindow, RGN_DIFF);
      if (RgnType == ERROR)
      {
         REGION_FreeRgnByHandle(hRgnWindow);
         REGION_FreeRgnByHandle(hRgnNonClient);
         return (HRGN)1;
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
            if (!(Window->state & WINDOWOBJECT_NEED_INTERNALPAINT))
               MsqDecPaintCountQueue(Window->pti->MessageQueue);
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
co_IntPaintWindows(PWINDOW_OBJECT Window, ULONG Flags, BOOL Recurse)
{
   HDC hDC;
   HWND hWnd = Window->hSelf;
   HRGN TempRegion;
   PWND Wnd;

   Wnd = Window->Wnd;

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      if (Window->hrgnUpdate)
      {
         if (!IntValidateParent(Window, Window->hrgnUpdate, Recurse))
            return;
      }

      if (Flags & RDW_UPDATENOW)
      {
         if (Window->hrgnUpdate != NULL ||
             Window->state & WINDOWOBJECT_NEED_INTERNALPAINT)
         {
            co_IntSendMessage(hWnd, WM_PAINT, 0, 0);
         }
      }
      else
      {
         if (Window->state & WINDOWOBJECT_NEED_NCPAINT)
         {
            TempRegion = IntGetNCUpdateRgn(Window, TRUE);
            Window->state &= ~WINDOWOBJECT_NEED_NCPAINT;
            MsqDecPaintCountQueue(Window->pti->MessageQueue);
            co_IntSendMessage(hWnd, WM_NCPAINT, (WPARAM)TempRegion, 0);

            if ( (HANDLE) 1 != TempRegion &&
                 NULL != TempRegion)
            {
               /* NOTE: The region can already be deleted! */
               GDIOBJ_FreeObjByHandle(TempRegion, GDI_OBJECT_TYPE_REGION | GDI_OBJECT_TYPE_SILENT);
            }
         }

         if (Window->state & WINDOWOBJECT_NEED_ERASEBKGND)
         {
            if (Window->hrgnUpdate)
            {
               hDC = UserGetDCEx( Window,
                                  Window->hrgnUpdate,
                                  DCX_CACHE|DCX_USESTYLE|DCX_INTERSECTRGN|DCX_KEEPCLIPRGN);

               if (co_IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0))
               {
                  Window->state &= ~WINDOWOBJECT_NEED_ERASEBKGND;
               }
               UserReleaseDC(Window, hDC, FALSE);
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

      if ((List = IntWinListChildren(Window)))
      {
         /* FIXME: Handle WS_EX_TRANSPARENT */
         for (phWnd = List; *phWnd; ++phWnd)
         {
            Window = UserGetWindowObject(*phWnd);
            Wnd = Window->Wnd;
            if (Window && (Wnd->style & WS_VISIBLE))
            {
               USER_REFERENCE_ENTRY Ref;
               UserRefObjectCo(Window, &Ref);
               co_IntPaintWindows(Window, Flags, TRUE);
               UserDerefObjectCo(Window);
            }
         }
         ExFreePool(List);
      }
   }
}

/*
 * IntInvalidateWindows
 *
 * Internal function used by IntRedrawWindow.
 */

VOID FASTCALL
IntInvalidateWindows(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags)
{
   INT RgnType;
   PWND Wnd;
   BOOL HadPaintMessage, HadNCPaintMessage;
   BOOL HasPaintMessage, HasNCPaintMessage;

   Wnd = Window->Wnd;
   DPRINT("IntInvalidateWindows start\n");
   /*
    * If the nonclient is not to be redrawn, clip the region to the client
    * rect
    */
   if (0 != (Flags & RDW_INVALIDATE) && 0 == (Flags & RDW_FRAME))
   {
      HRGN hRgnClient;

      hRgnClient = IntSysCreateRectRgnIndirect(&Window->Wnd->rcClient);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnClient, RGN_AND);
      REGION_FreeRgnByHandle(hRgnClient);
   }

   /*
    * Clip the given region with window rectangle (or region)
    */

   if (!Window->hrgnClip || (Wnd->style & WS_MINIMIZE))
   {
      HRGN hRgnWindow;

      hRgnWindow = IntSysCreateRectRgnIndirect(&Window->Wnd->rcWindow);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnWindow, RGN_AND);
      REGION_FreeRgnByHandle(hRgnWindow);
   }
   else
   {
      NtGdiOffsetRgn( hRgn,
                     -Wnd->rcWindow.left,
                     -Wnd->rcWindow.top);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, Window->hrgnClip, RGN_AND);
      NtGdiOffsetRgn( hRgn,
                      Wnd->rcWindow.left,
                      Wnd->rcWindow.top);
   }

   /*
    * Save current state of pending updates
    */

   HadPaintMessage = Window->hrgnUpdate != NULL ||
                     Window->state & WINDOWOBJECT_NEED_INTERNALPAINT;
   HadNCPaintMessage = Window->state & WINDOWOBJECT_NEED_NCPAINT;

   /*
    * Update the region and flags
    */

   if (Flags & RDW_INVALIDATE && RgnType != NULLREGION)
   {
      if (Window->hrgnUpdate == NULL)
      {
         Window->hrgnUpdate = IntSysCreateRectRgn(0, 0, 0, 0);
         IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_PUBLIC);
      }

      if (NtGdiCombineRgn(Window->hrgnUpdate, Window->hrgnUpdate,
                          hRgn, RGN_OR) == NULLREGION)
      {
         IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
         REGION_FreeRgnByHandle(Window->hrgnUpdate);
         Window->hrgnUpdate = NULL;
      }

      if (Flags & RDW_FRAME)
         Window->state |= WINDOWOBJECT_NEED_NCPAINT;
      if (Flags & RDW_ERASE)
         Window->state |= WINDOWOBJECT_NEED_ERASEBKGND;

      Flags |= RDW_FRAME;
   }

   if (Flags & RDW_VALIDATE && RgnType != NULLREGION)
   {
      if (Window->hrgnUpdate != NULL)
      {
         if (NtGdiCombineRgn(Window->hrgnUpdate, Window->hrgnUpdate,
                             hRgn, RGN_DIFF) == NULLREGION)
         {
            IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
            REGION_FreeRgnByHandle(Window->hrgnUpdate);
            Window->hrgnUpdate = NULL;
         }
      }

      if (Window->hrgnUpdate == NULL)
         Window->state &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      if (Flags & RDW_NOFRAME)
         Window->state &= ~WINDOWOBJECT_NEED_NCPAINT;
      if (Flags & RDW_NOERASE)
         Window->state &= ~WINDOWOBJECT_NEED_ERASEBKGND;
   }

   if (Flags & RDW_INTERNALPAINT)
   {
      Window->state |= WINDOWOBJECT_NEED_INTERNALPAINT;
   }

   if (Flags & RDW_NOINTERNALPAINT)
   {
      Window->state &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
   }

   /*
    * Process children if needed
    */

   if (!(Flags & RDW_NOCHILDREN) && !(Wnd->style & WS_MINIMIZE) &&
         ((Flags & RDW_ALLCHILDREN) || !(Wnd->style & WS_CLIPCHILDREN)))
   {
      PWINDOW_OBJECT Child;

      for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
      {
         if (Child->Wnd->style & WS_VISIBLE)
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

   HasPaintMessage = Window->hrgnUpdate != NULL ||
                     Window->state & WINDOWOBJECT_NEED_INTERNALPAINT;
   HasNCPaintMessage = Window->state & WINDOWOBJECT_NEED_NCPAINT;

   if (HasPaintMessage != HadPaintMessage)
   {
      if (HadPaintMessage)
         MsqDecPaintCountQueue(Window->pti->MessageQueue);
      else
         MsqIncPaintCountQueue(Window->pti->MessageQueue);
   }

   if (HasNCPaintMessage != HadNCPaintMessage)
   {
      if (HadNCPaintMessage)
         MsqDecPaintCountQueue(Window->pti->MessageQueue);
      else
         MsqIncPaintCountQueue(Window->pti->MessageQueue);
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
IntIsWindowDrawable(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT WndObject;
   PWND Wnd;

   for (WndObject = Window; WndObject != NULL; WndObject = WndObject->spwndParent)
   {
      Wnd = WndObject->Wnd;
      if (!(Wnd->style & WS_VISIBLE) ||
            ((Wnd->style & WS_MINIMIZE) && (WndObject != Window)))
      {
         return FALSE;
      }
   }

   return TRUE;
}

/*
 * IntRedrawWindow
 *
 * Internal version of NtUserRedrawWindow that takes WINDOW_OBJECT as
 * first parameter.
 */

BOOL FASTCALL
co_UserRedrawWindow(
   PWINDOW_OBJECT Window,
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
            NtGdiOffsetRgn(hRgn, Window->Wnd->rcClient.left, Window->Wnd->rcClient.top);
      }
      else if (UpdateRect != NULL)
      {
         if (!RECTL_bIsEmptyRect(UpdateRect))
         {
            hRgn = IntSysCreateRectRgnIndirect((RECTL *)UpdateRect);
            NtGdiOffsetRgn(hRgn, Window->Wnd->rcClient.left, Window->Wnd->rcClient.top);
         }
      }
      else if ((Flags & (RDW_INVALIDATE | RDW_FRAME)) == (RDW_INVALIDATE | RDW_FRAME) ||
               (Flags & (RDW_VALIDATE | RDW_NOFRAME)) == (RDW_VALIDATE | RDW_NOFRAME))
      {
         if (!RECTL_bIsEmptyRect(&Window->Wnd->rcWindow))
            hRgn = IntSysCreateRectRgnIndirect(&Window->Wnd->rcWindow);
      }
      else
      {
         if (!RECTL_bIsEmptyRect(&Window->Wnd->rcClient))
            hRgn = IntSysCreateRectRgnIndirect(&Window->Wnd->rcClient);
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
IntIsWindowDirty(PWINDOW_OBJECT Window)
{
   PWND Wnd = Window->Wnd;
   return (Wnd->style & WS_VISIBLE) &&
          ((Window->hrgnUpdate != NULL) ||
           (Window->state & WINDOWOBJECT_NEED_INTERNALPAINT) ||
           (Window->state & WINDOWOBJECT_NEED_NCPAINT));
}

HWND FASTCALL
IntFindWindowToRepaint(PWINDOW_OBJECT Window, PTHREADINFO Thread)
{
   HWND hChild;
   PWINDOW_OBJECT TempWindow;
   PWND Wnd, TempWnd;

   for (; Window != NULL; Window = Window->spwndNext)
   {
      Wnd = Window->Wnd;
      if (IntWndBelongsToThread(Window, Thread) &&
          IntIsWindowDirty(Window))
      {
         /* Make sure all non-transparent siblings are already drawn. */
         if (Wnd->ExStyle & WS_EX_TRANSPARENT)
         {
            for (TempWindow = Window->spwndNext; TempWindow != NULL;
                 TempWindow = TempWindow->spwndNext)
            {
               TempWnd = TempWindow->Wnd;
               if (!(TempWnd->ExStyle & WS_EX_TRANSPARENT) &&
                   IntWndBelongsToThread(TempWindow, Thread) &&
                   IntIsWindowDirty(TempWindow))
               {
                  return TempWindow->hSelf;
               }
            }
         }

         return Window->hSelf;
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
   PWINDOW_OBJECT Window,
   UINT MsgFilterMin,
   UINT MsgFilterMax,
   PTHREADINFO Thread,
   MSG *Message,
   BOOL Remove)
{
   PUSER_MESSAGE_QUEUE MessageQueue = (PUSER_MESSAGE_QUEUE)Thread->MessageQueue;

   if (!MessageQueue->PaintCount)
      return FALSE;

   if ((MsgFilterMin != 0 || MsgFilterMax != 0) &&
         (MsgFilterMin > WM_PAINT || MsgFilterMax < WM_PAINT))
      return FALSE;

   Message->hwnd = IntFindWindowToRepaint(UserGetDesktopWindow(), PsGetCurrentThreadWin32Thread());

   if (Message->hwnd == NULL)
   {
      DPRINT1("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found!\n");
      /* Hack to stop spamming the debuglog ! */
      MessageQueue->PaintCount = 0;
      return FALSE;
   }

   if (Window != NULL && Message->hwnd != Window->hSelf)
      return FALSE;

   Message->message = WM_PAINT;
   Message->wParam = Message->lParam = 0;

   return TRUE;
}

static
HWND FASTCALL
co_IntFixCaret(PWINDOW_OBJECT Window, RECTL *lprc, UINT flags)
{
   PDESKTOP Desktop;
   PTHRDCARETINFO CaretInfo;
   HWND hWndCaret;
   PWINDOW_OBJECT WndCaret;

   ASSERT_REFS_CO(Window);

   Desktop = ((PTHREADINFO)PsGetCurrentThread()->Tcb.Win32Thread)->rpdesk;
   CaretInfo = ((PUSER_MESSAGE_QUEUE)Desktop->ActiveMessageQueue)->CaretInfo;
   hWndCaret = CaretInfo->hWnd;

   WndCaret = UserGetWindowObject(hWndCaret);

   //fix: check for WndCaret can be null
   if (WndCaret == Window ||
         ((flags & SW_SCROLLCHILDREN) && IntIsChildWindow(Window, WndCaret)))
   {
      POINT pt, FromOffset, ToOffset, Offset;
      RECTL rcCaret;

      pt.x = CaretInfo->Pos.x;
      pt.y = CaretInfo->Pos.y;
      IntGetClientOrigin(WndCaret, &FromOffset);
      IntGetClientOrigin(Window, &ToOffset);
      Offset.x = FromOffset.x - ToOffset.x;
      Offset.y = FromOffset.y - ToOffset.y;
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
   PWINDOW_OBJECT Window = NULL;
   PAINTSTRUCT Ps;
   NTSTATUS Status;
   DECLARE_RETURN(HDC);
   USER_REFERENCE_ENTRY Ref;
   PWND Wnd;

   DPRINT("Enter NtUserBeginPaint\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( NULL);
   }

   UserRefObjectCo(Window, &Ref);

   Wnd = Window->Wnd;

   co_UserHideCaret(Window);

   if (Window->state & WINDOWOBJECT_NEED_NCPAINT)
   {
      HRGN hRgn;

      hRgn = IntGetNCUpdateRgn(Window, FALSE);
      Window->state &= ~WINDOWOBJECT_NEED_NCPAINT;
      MsqDecPaintCountQueue(Window->pti->MessageQueue);
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
      MsqDecPaintCountQueue(Window->pti->MessageQueue);
      GdiGetClipBox(Ps.hdc, &Ps.rcPaint);
      IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
      /* The region is part of the dc now and belongs to the process! */
      Window->hrgnUpdate = NULL;
   }
   else
   {
      if (Window->state & WINDOWOBJECT_NEED_INTERNALPAINT)
         MsqDecPaintCountQueue(Window->pti->MessageQueue);

      IntGetClientRect(Window, &Ps.rcPaint);
   }

   Window->state &= ~WINDOWOBJECT_NEED_INTERNALPAINT;

   if (Window->state & WINDOWOBJECT_NEED_ERASEBKGND)
   {
      Window->state &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      Ps.fErase = !co_IntSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)Ps.hdc, 0);
   }
   else
   {
      Ps.fErase = FALSE;
   }
   if (Window->hrgnUpdate)
   {
      if (!(Wnd->style & WS_CLIPCHILDREN))
      {
         PWINDOW_OBJECT Child;
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
   PWINDOW_OBJECT Window;
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
co_UserGetUpdateRgn(PWINDOW_OBJECT Window, HRGN hRgn, BOOL bErase)
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
      Rect = Window->Wnd->rcClient;
      IntIntersectWithParents(Window, &Rect);
      NtGdiSetRectRgn(hRgn, Rect.left, Rect.top, Rect.right, Rect.bottom);
      RegionType = NtGdiCombineRgn(hRgn, hRgn, Window->hrgnUpdate, RGN_AND);
      NtGdiOffsetRgn(hRgn, -Window->Wnd->rcClient.left, -Window->Wnd->rcClient.top);
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
   PWINDOW_OBJECT Window;
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
   PWINDOW_OBJECT Window;
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
      if (Window->hrgnUpdate == (HRGN)1)
      {
         Rect = Window->Wnd->rcClient;
      }
      else
      {
         RgnData = RGNOBJAPI_Lock(Window->hrgnUpdate, NULL);
         ASSERT(RgnData != NULL);
         RegionType = REGION_GetRgnBox(RgnData, &Rect);
         RGNOBJAPI_Unlock(RgnData);

         if (RegionType != ERROR && RegionType != NULLREGION)
            RECTL_bIntersectRect(&Rect, &Rect, &Window->Wnd->rcClient);
      }

      if (IntIntersectWithParents(Window, &Rect))
      {
         RECTL_vOffsetRect(&Rect,
                          -Window->Wnd->rcClient.left,
                          -Window->Wnd->rcClient.top);
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
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
   PWINDOW_OBJECT Wnd;
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
         SetLastWin32Error(RtlNtStatusToDosError(Status));
         RETURN( FALSE);
      }
   }

   if ( flags & ~(RDW_ERASE|RDW_FRAME|RDW_INTERNALPAINT|RDW_INVALIDATE|
                  RDW_NOERASE|RDW_NOFRAME|RDW_NOINTERNALPAINT|RDW_VALIDATE|
                  RDW_ERASENOW|RDW_UPDATENOW|RDW_ALLCHILDREN|RDW_NOCHILDREN) )
   {
      /* RedrawWindow fails only in case that flags are invalid */
      SetLastWin32Error(ERROR_INVALID_FLAGS);
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
      hrgnVisible = pDC->rosdc.hVisRgn;  // pDC->prgnRao?
      DC_UnlockDc(pDC);

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
   PWINDOW_OBJECT Window = NULL, CaretWnd;
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
      PWINDOW_OBJECT Child;
      RECTL rcChild;
      POINT ClientOrigin;
      USER_REFERENCE_ENTRY WndRef;
      RECTL rcDummy;

      IntGetClientOrigin(Window, &ClientOrigin);
      for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
      {
         rcChild = Child->Wnd->rcWindow;
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
   PWINDOW_OBJECT pWnd,
   HDC hDc,
   RECTL *lpRc,
   BOOL Down)
{
   HICON hIcon;
   PCURICON_OBJECT pIcon;

   ASSERT(pWnd && lpRc);

   /* Get the icon to draw. We don't care about WM_GETICON here. */

   hIcon = pWnd->Wnd->pcls->hIconSm;

   if(!hIcon)
   {
      DPRINT("Wnd class has no small icon.\n");
      hIcon = pWnd->Wnd->pcls->hIcon;
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
   PWINDOW_OBJECT pWnd,
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
   PWND Wnd = NULL;

   //ASSERT(pWnd != NULL);

   if (pWnd)
       Wnd = pWnd->Wnd;

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

   if ((!hIcon) && (Wnd != NULL))
   {
     HasIcon = (uFlags & DC_ICON) && (Wnd->style & WS_SYSMENU)
        && !(uFlags & DC_SMALLCAP) && !(Wnd->ExStyle & WS_EX_DLGMODALFRAME)
        && !(Wnd->ExStyle & WS_EX_TOOLWINDOW);
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
         PDC pMemDc;

		 if (Wnd != NULL)
		 {
			 if(Wnd->style & WS_SYSMENU)
			 {
				r.right -= 3 + ButtonWidth;
				if(!(uFlags & DC_SMALLCAP))
				{
				   if(Wnd->style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
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

         pMemDc = DC_LockDc(hMemDc);
         if(!pMemDc)
         {
            DPRINT1("%s: Can't lock dc!\n", __FUNCTION__);
            goto cleanup;
         }

         if(!IntGdiGradientFill(pMemDc, vert, 2, &gcap,
            1, GRADIENT_FILL_RECT_H))
         {
            DPRINT1("%s: IntGdiGradientFill() failed!\n", __FUNCTION__);
         }

         DC_UnlockDc(pMemDc);
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

         if ((Wnd != NULL) && (Wnd->style & WS_SYSMENU))
         {
            r.right -= 3 + ButtonWidth;
            if(! (uFlags & DC_SMALLCAP))
            {
               if(Wnd->style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
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
         UserDrawCaptionText(hMemDc, &pWnd->Wnd->strName, &r, uFlags);
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
         co_IntSendMessage((HWND)HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hWnd, 0);
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
   PWINDOW_OBJECT pWnd = NULL;
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
