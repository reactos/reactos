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
 *  $Id: painting.c,v 1.39 2003/11/25 23:46:23 gvg Exp $
 *
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

#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <windows.h>
#include <include/painting.h>
#include <user32/wininternal.h>
#include <include/rect.h>
#include <win32k/coord.h>
#include <win32k/region.h>
#include <include/vis.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/*
 * Define this after the desktop will be moved to CSRSS and will
 * get proper message queue.
 */
/* #define DESKTOP_IN_CSRSS */

/* PRIVATE FUNCTIONS **********************************************************/

VOID FASTCALL
IntValidateParent(PWINDOW_OBJECT Child)
{
   HWND Parent;
   PWINDOW_OBJECT ParentWindow;

   Parent = NtUserGetAncestor(Child->Self, GA_PARENT);
   while (Parent && Parent != IntGetDesktopWindow())
   {
      ParentWindow = IntGetWindowObject(Parent);
      if (ParentWindow && !(ParentWindow->Style & WS_CLIPCHILDREN))
      {
         if (ParentWindow->UpdateRegion != 0)
         {
            INT OffsetX, OffsetY;

            /*
             * We must offset the child region by the offset of the
             * child rect in the parent.
             */
            OffsetX = Child->WindowRect.left - ParentWindow->WindowRect.left;
            OffsetY = Child->WindowRect.top - ParentWindow->WindowRect.top;
            NtGdiOffsetRgn(Child->UpdateRegion, OffsetX, OffsetY );
            NtGdiCombineRgn(ParentWindow->UpdateRegion, ParentWindow->UpdateRegion,
               Child->UpdateRegion, RGN_DIFF);
            /* FIXME: If the resulting region is empty, remove fake posted paint message */
            NtGdiOffsetRgn(Child->UpdateRegion, -OffsetX, -OffsetY);
         }
      }
      IntReleaseWindowObject(ParentWindow);
      Parent = NtUserGetAncestor(Parent, GA_PARENT);
   }
}

/*
 * IntGetNCUpdateRegion
 *
 * Get nonclient part of window update region. 
 * 
 * Return Value
 *    Handle to region that represents invalid nonclient window area. The
 *    caller is responsible for deleting it.
 *
 * Remarks
 *    This function also marks the nonclient update region of window
 *    as valid, clears the WINDOWOBJECT_NEED_NCPAINT flag and removes
 *    the fake paint message from message queue if the Remove is set
 *    to TRUE.
 */

HRGN FASTCALL
IntGetNCUpdateRegion(PWINDOW_OBJECT Window, BOOL Remove)
{
   HRGN WindowRgn;
   HRGN NonclientRgn;

   WindowRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
   NtGdiOffsetRgn(WindowRgn, 
      -Window->WindowRect.left,
      -Window->WindowRect.top);
   NonclientRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
   if (NtGdiCombineRgn(NonclientRgn, Window->UpdateRegion,
       WindowRgn, RGN_DIFF) == NULLREGION)
   {
      NtGdiDeleteObject(NonclientRgn);
      NonclientRgn = NULL;
   }
   if (Remove)
   {
      if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
          WindowRgn, RGN_AND) == NULLREGION)
      {
         NtGdiDeleteObject(Window->UpdateRegion);
         Window->UpdateRegion = NULL;
      }
      Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      MsqDecPaintCountQueue(Window->MessageQueue);
   }

   return NonclientRgn;
}

/*
 * IntPaintWindows
 *
 * Internal function used by IntRedrawWindow.
 */

VOID FASTCALL
IntPaintWindows(PWINDOW_OBJECT Window, ULONG Flags)
{
   HDC hDC;
   HWND hWnd = Window->Self;

   if (!(Window->Style & WS_VISIBLE))
   {
      return;
   }

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      if (IntIsDesktopWindow(Window))
      {
         /*
          * Repainting of desktop window
          */

#ifndef DESKTOP_IN_CSRSS
         VIS_RepaintDesktop(hWnd, Window->UpdateRegion);
         Window->Flags &= ~(WINDOWOBJECT_NEED_NCPAINT |
            WINDOWOBJECT_NEED_INTERNALPAINT | WINDOWOBJECT_NEED_ERASEBKGND);
         NtGdiDeleteObject(Window->UpdateRegion);
         Window->UpdateRegion = NULL;
#else
         if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
         {
            MsqDecPaintCountQueue(Window->MessageQueue);
            Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
         }
         if (Window->UpdateRegion ||
             Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
         {
            MsqDecPaintCountQueue(Window->MessageQueue);
            Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
         }
         if (Window->UpdateRegion)
         {
            hDC = NtUserGetDCEx(hWnd, 0, DCX_CACHE | DCX_USESTYLE |
               DCX_INTERSECTUPDATE);
            if (hDC != NULL)
            {
               NtUserSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0);
               NtUserReleaseDC(hWnd, hDC);
               NtGdiDeleteObject(Window->UpdateRegion);
               Window->UpdateRegion = NULL;
            }
         }
#endif
      }
      else
      {
         /*
          * Repainting of non-desktop window
          */

         if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
         {
            NtUserSendMessage(hWnd, WM_NCPAINT, (WPARAM)IntGetNCUpdateRegion(Window, TRUE), 0);
         }

         if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
         {
            if (Window->UpdateRegion)
            {
               hDC = NtUserGetDCEx(hWnd, 0, DCX_CACHE | DCX_USESTYLE |
                  DCX_INTERSECTUPDATE);
               if (hDC != NULL)
               {
                  if (NtUserSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)hDC, 0))
                     Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
                  NtUserReleaseDC(hWnd, hDC);
               }
            }
         }

         if (Flags & RDW_UPDATENOW)
         {
            if (Window->UpdateRegion != NULL ||
                Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
            {
               NtUserSendMessage(hWnd, WM_PAINT, 0, 0);
               if (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
               {
                  Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
                  if (Window->UpdateRegion == NULL)
                  {
                     MsqDecPaintCountQueue(Window->MessageQueue);
                  }
               }
            }
         }
      }
   }

   /*
    * Check that the window is still valid at this point
    */

   if (!IntIsWindow(hWnd))
      return;

   /*
    * Paint child windows.
    */

   if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
       ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)))
   {
      HWND *List, *phWnd;

      if ((List = IntWinListChildren(Window)))
      {
         for (phWnd = List; *phWnd; ++phWnd)
         {
            Window = IntGetWindowObject(*phWnd);
            if (Window)
            {
               IntPaintWindows(Window, Flags);
               IntReleaseWindowObject(Window);
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
IntInvalidateWindows(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags,
   BOOL ValidateParent)
{
   INT RgnType;
   BOOL HadPaintMessage, HadNCPaintMessage;
   BOOL HasPaintMessage, HasNCPaintMessage;
   HRGN hRgnWindow;

   /*
    * Clip the given region with window rectangle (or region)
    */

#ifdef TODO
   if (!Window->WindowRegion)
#endif
   {
      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
      NtGdiOffsetRgn(hRgnWindow,
         -Window->WindowRect.left,
         -Window->WindowRect.top);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnWindow, RGN_AND);        
      NtGdiDeleteObject(hRgnWindow);
   }
#ifdef TODO
   else
   {
      RgnType = NtGdiCombineRgn(hRgn, hRgn, Window->WindowRegion, RGN_AND);        
   }
#endif

   /*
    * Save current state of pending updates
    */

   HadPaintMessage = Window->UpdateRegion != NULL ||
      Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT;
   HadNCPaintMessage = Window->Flags & WINDOWOBJECT_NEED_NCPAINT;

   /*
    * Update the region and flags
    */

   if (Flags & RDW_INVALIDATE && RgnType != NULLREGION)
   {
      if (Window->UpdateRegion == NULL)
      {
         Window->UpdateRegion = NtGdiCreateRectRgn(0, 0, 0, 0);
      }

      if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
          hRgn, RGN_OR) == NULLREGION)
      {
         NtGdiDeleteObject(Window->UpdateRegion);
         Window->UpdateRegion = NULL;
      }

      if (Flags & RDW_FRAME)
         Window->Flags |= WINDOWOBJECT_NEED_NCPAINT;
      if (Flags & RDW_ERASE)
         Window->Flags |= WINDOWOBJECT_NEED_ERASEBKGND;

      Flags |= RDW_FRAME;
   }

   if (Flags & RDW_VALIDATE && RgnType != NULLREGION)
   {
      if (Window->UpdateRegion != NULL)
      {
         if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
             hRgn, RGN_DIFF) == NULLREGION)
         {
            NtGdiDeleteObject(Window->UpdateRegion);
            Window->UpdateRegion = NULL;
         }
      }

      if (Window->UpdateRegion == NULL)
         Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      if (Flags & RDW_NOFRAME)
         Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
      if (Flags & RDW_NOERASE)
         Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
   }

   if (Flags & RDW_INTERNALPAINT)
   {
      Window->Flags |= WINDOWOBJECT_NEED_INTERNALPAINT;
   }

   if (Flags & RDW_NOINTERNALPAINT)
   {
      Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
   }

   /*
    * Validate parent covered by region
    */

   if (ValidateParent)
   {
      IntValidateParent(Window);
   }

   /*
    * Process children if needed
    */

   if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
       ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)))
   {
      HWND *List, *phWnd;
      PWINDOW_OBJECT Child;

      if ((List = IntWinListChildren(Window)))
      {
         for (phWnd = List; *phWnd; ++phWnd)
         {
            Child = IntGetWindowObject(*phWnd);
            if ((Child->Style & (WS_VISIBLE | WS_MINIMIZE)) == WS_VISIBLE)
            {
               /*
                * Recursive call to update children UpdateRegion
                */
               HRGN hRgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
               NtGdiCombineRgn(hRgnTemp, hRgn, 0, RGN_COPY);
               NtGdiOffsetRgn(hRgnTemp,
                  Window->WindowRect.left - Child->WindowRect.left,
                  Window->WindowRect.top - Child->WindowRect.top);
               IntInvalidateWindows(Child, hRgnTemp, Flags, FALSE);

               /*
                * Update our UpdateRegion depending on children
                */
               NtGdiCombineRgn(hRgnTemp, Child->UpdateRegion, 0, RGN_COPY);
               NtGdiOffsetRgn(hRgnTemp,
                  Child->WindowRect.left - Window->WindowRect.left,
                  Child->WindowRect.top - Window->WindowRect.top);
               hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
               NtGdiOffsetRgn(hRgnWindow,
                  -Window->WindowRect.left,
                  -Window->WindowRect.top);
               NtGdiCombineRgn(hRgnTemp, hRgnTemp, hRgnWindow, RGN_AND);
               if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
                   hRgnTemp, RGN_DIFF) == NULLREGION)
               {
                  NtGdiDeleteObject(Window->UpdateRegion);
                  Window->UpdateRegion = NULL;
               }
               NtGdiDeleteObject(hRgnTemp);
            }
            IntReleaseWindowObject(Child);
         }
         ExFreePool(List);
      }
   }

   /*
    * Fake post paint messages to window message queue if needed
    */

#ifndef DESKTOP_IN_CSRSS
   if (Window->MessageQueue)
#endif
   {
      HasPaintMessage = Window->UpdateRegion != NULL ||
         Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT;
      HasNCPaintMessage = Window->Flags & WINDOWOBJECT_NEED_NCPAINT;

      if (HasPaintMessage != HadPaintMessage)
      {
         if (HadPaintMessage)
            MsqDecPaintCountQueue(Window->MessageQueue);
         else
            MsqIncPaintCountQueue(Window->MessageQueue);
      }

      if (HasNCPaintMessage != HadNCPaintMessage)
      {
         if (HadNCPaintMessage)
            MsqDecPaintCountQueue(Window->MessageQueue);
         else
            MsqIncPaintCountQueue(Window->MessageQueue);
      }
#ifndef DESKTOP_IN_CSRSS
   }
#endif
}

/*
 * IntIsWindowDrawable
 *
 * Remarks
 *    Window is drawable when it is visible, all parents are not
 *    minimized, and it is itself not minimized.
 */

BOOL FASTCALL
IntIsWindowDrawable(PWINDOW_OBJECT Window)
{
   for (; Window; Window = Window->Parent)
   {
      if ((Window->Style & (WS_VISIBLE | WS_MINIMIZE)) != WS_VISIBLE)
         return FALSE;
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
IntRedrawWindow(PWINDOW_OBJECT Window, const RECT* UpdateRect, HRGN UpdateRgn,
   ULONG Flags)
{
   HRGN hRgn = NULL;

   /*
    * Step 1.
    * Validation of passed parameters.
    */

   if (!IntIsWindowDrawable(Window) ||
       (Flags & (RDW_VALIDATE | RDW_INVALIDATE)) ==
       (RDW_VALIDATE | RDW_INVALIDATE))
   {
      return FALSE;
   }

   /*
    * Step 2.
    * Transform the parameters UpdateRgn and UpdateRect into
    * a region hRgn specified in window coordinates.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE))
   {
      if (UpdateRgn != NULL)
      {
         hRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         NtGdiCombineRgn(hRgn, UpdateRgn, NULL, RGN_COPY);
         NtGdiOffsetRgn(hRgn, 
            Window->ClientRect.left - Window->WindowRect.left,
            Window->ClientRect.top - Window->WindowRect.top);
      } else
      if (UpdateRect != NULL)
      {
         hRgn = UnsafeIntCreateRectRgnIndirect((RECT *)UpdateRect);
         NtGdiOffsetRgn(hRgn, 
            Window->ClientRect.left - Window->WindowRect.left,
            Window->ClientRect.top - Window->WindowRect.top);
      } else
      if ((Flags & (RDW_INVALIDATE | RDW_FRAME)) == (RDW_INVALIDATE | RDW_FRAME) ||
          (Flags & (RDW_VALIDATE | RDW_NOFRAME)) == (RDW_VALIDATE | RDW_NOFRAME))
      {
         hRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
         NtGdiOffsetRgn(hRgn, 
            -Window->WindowRect.left,
            -Window->WindowRect.top);
      }
      else
      {
         hRgn = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
         NtGdiOffsetRgn(hRgn, 
            -Window->WindowRect.left,
            -Window->WindowRect.top);
      }
   }

   /*
    * Step 3.
    * Adjust the window update region depending on hRgn and flags.
    */

   if (Flags & (RDW_INVALIDATE | RDW_VALIDATE | RDW_INTERNALPAINT | RDW_NOINTERNALPAINT))
   {
      IntInvalidateWindows(Window, hRgn, Flags, TRUE);
   } else
   if (Window->UpdateRegion != NULL && Flags & RDW_ERASENOW)
   {
      /* Validate parent covered by region. */
      IntValidateParent(Window);
   }

   /*
    * Step 4.
    * Repaint and erase windows if needed.
    */

   if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
   {
      IntPaintWindows(Window, Flags);
   }

   /*
    * Step 5.
    * Cleanup ;-)
    */

   if (NULL != hRgn)
   {
      NtGdiDeleteObject(hRgn);
   }

   return TRUE;
}

HWND STDCALL
IntFindWindowToRepaint(HWND hWnd, PW32THREAD Thread)
{
   PWINDOW_OBJECT Window;
   PWINDOW_OBJECT Child;
   HWND hFoundWnd = NULL;

   if (hWnd == NULL)
   {
      PLIST_ENTRY CurrentEntry;

      ExAcquireFastMutex(&Thread->WindowListLock);

      for (CurrentEntry = Thread->WindowListHead.Flink;
           CurrentEntry != &Thread->WindowListHead;
           CurrentEntry = CurrentEntry->Flink)
      {
         Window = CONTAINING_RECORD(CurrentEntry, WINDOW_OBJECT, ThreadListEntry);
         if (Window->Parent != NULL && !IntIsDesktopWindow(Window->Parent))
         {
            continue;
         }
         if (Window->Style & WS_VISIBLE)
         {
            hFoundWnd = IntFindWindowToRepaint(Window->Self, Thread);
            if (hFoundWnd != NULL)
            {
                ExReleaseFastMutex(&Thread->WindowListLock);
                return hFoundWnd;
            }
         }
      }

      ExReleaseFastMutex(&Thread->WindowListLock);
      return NULL;
   }
   else
   {
      Window = IntGetWindowObject(hWnd);
      if (Window == NULL)
         return NULL;

      if (Window->UpdateRegion != NULL ||
          Window->Flags & (WINDOWOBJECT_NEED_INTERNALPAINT | WINDOWOBJECT_NEED_NCPAINT))
      {
         IntReleaseWindowObject(Window);
         return hWnd;
      }

      ExAcquireFastMutex(&Window->ChildrenListLock);

      for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
      {
         if (Child->Style & WS_VISIBLE &&
             (Child->UpdateRegion != NULL ||
              Child->Flags & WINDOWOBJECT_NEED_INTERNALPAINT ||
              Child->Flags & WINDOWOBJECT_NEED_NCPAINT))
         {
            hFoundWnd = Child->Self;
            break;
         }
      }

      if (hFoundWnd == NULL)
      {
         for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
         {
            if (Child->Style & WS_VISIBLE)
            {
               hFoundWnd = IntFindWindowToRepaint(Child->Self, Thread);
               if (hFoundWnd != NULL)
                  break;
            }
         }
      }

      ExReleaseFastMutex(&Window->ChildrenListLock);
      IntReleaseWindowObject(Window);

      return hFoundWnd;
   }
}

BOOL FASTCALL
IntGetPaintMessage(PWINDOW_OBJECT Wnd, PW32THREAD Thread, MSG *Message,
   BOOL Remove)
{
   PWINDOW_OBJECT Window;
   PUSER_MESSAGE_QUEUE MessageQueue = (PUSER_MESSAGE_QUEUE)Thread->MessageQueue;

   if (!MessageQueue->PaintPosted)
      return FALSE;

   if (Wnd)
      Message->hwnd = IntFindWindowToRepaint(Wnd->Self, PsGetWin32Thread());
   else
      Message->hwnd = IntFindWindowToRepaint(NULL, PsGetWin32Thread());

   if (Message->hwnd == NULL)
   {
      DPRINT1("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found!\n");
      return FALSE;
   }

   Window = IntGetWindowObject(Message->hwnd);
   if (Window != NULL)
   {
      if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
      {
         Message->message = WM_NCPAINT;
         Message->wParam = (WPARAM)IntGetNCUpdateRegion(Window, Remove);
         Message->lParam = 0;
      } else
      {
         Message->message = WM_PAINT;
         Message->wParam = Message->lParam = 0;
         if (Remove && Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
         {
            Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
            if (Window->UpdateRegion == NULL)
            {
               MsqDecPaintCountQueue(Window->MessageQueue);
            }
         }
      }

      IntReleaseWindowObject(Window);
      return TRUE;
   }

   return FALSE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * NtUserBeginPaint
 *
 * Status
 *    @implemented
 */

HDC STDCALL
NtUserBeginPaint(HWND hWnd, PAINTSTRUCT* lPs)
{
   PWINDOW_OBJECT Window;
   RECT ClientRect;
   RECT ClipRect;
   INT DcxFlags;

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   NtUserHideCaret(hWnd);

   DcxFlags = DCX_INTERSECTUPDATE | DCX_WINDOWPAINT | DCX_USESTYLE;
   if (IntGetClassLong(Window, GCL_STYLE, FALSE) & CS_PARENTDC)
   {
      /* FIXME: Is this correct? */
      /* Don't clip the output to the update region for CS_PARENTDC window */
      DcxFlags &= ~DCX_INTERSECTUPDATE;
   }
   
   lPs->hdc = NtUserGetDCEx(hWnd, 0, DcxFlags);

   if (!lPs->hdc)
   {
      return NULL;
   }

/*   IntRedrawWindow(Window, NULL, 0, RDW_NOINTERNALPAINT | RDW_VALIDATE | RDW_NOCHILDREN);*/
   if (Window->UpdateRegion != NULL)
   {
      MsqDecPaintCountQueue(Window->MessageQueue);
      IntValidateParent(Window);
      NtGdiDeleteObject(Window->UpdateRegion);
      Window->UpdateRegion = NULL;
   }

   IntGetClientRect(Window, &ClientRect);
   NtGdiGetClipBox(lPs->hdc, &ClipRect);
   NtGdiLPtoDP(lPs->hdc, (LPPOINT)&ClipRect, 2);
   NtGdiIntersectRect(&lPs->rcPaint, &ClientRect, &ClipRect);
   NtGdiDPtoLP(lPs->hdc, (LPPOINT)&lPs->rcPaint, 2);

   if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
   {
      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      lPs->fErase = !NtUserSendMessage(hWnd, WM_ERASEBKGND, (WPARAM)lPs->hdc, 0);
   }
   else
   {
      lPs->fErase = FALSE;
   }

   IntReleaseWindowObject(Window);

   return lPs->hdc;
}

/*
 * NtUserEndPaint
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserEndPaint(HWND hWnd, CONST PAINTSTRUCT* lPs)
{
   NtUserReleaseDC(hWnd, lPs->hdc);
   NtUserShowCaret(hWnd);

   return TRUE;
}

/*
 * NtUserInvalidateRect
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserInvalidateRect(HWND hWnd, CONST RECT *Rect, BOOL Erase)
{
   return NtUserRedrawWindow(hWnd, Rect, 0, RDW_INVALIDATE | (Erase ? RDW_ERASE : 0));
}

/*
 * NtUserInvalidateRgn
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserInvalidateRgn(HWND hWnd, HRGN Rgn, BOOL Erase)
{
   return NtUserRedrawWindow(hWnd, NULL, Rgn, RDW_INVALIDATE | (Erase ? RDW_ERASE : 0));
}

/*
 * NtUserValidateRgn
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserValidateRgn(HWND hWnd, HRGN hRgn)
{
   return NtUserRedrawWindow(hWnd, NULL, hRgn, RDW_VALIDATE | RDW_NOCHILDREN);
}

/*
 * NtUserUpdateWindow
 *
 * Status
 *    @implemented
 */
 
BOOL STDCALL
NtUserUpdateWindow(HWND hWnd)
{
   return NtUserRedrawWindow(hWnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN);
}

/*
 * NtUserGetUpdateRgn
 *
 * Status
 *    @implemented
 */

INT STDCALL
NtUserGetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
   PWINDOW_OBJECT Window;
   int RegionType;

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return ERROR;
   }
    
   if (Window->UpdateRegion == NULL)
   {
      RegionType = (NtGdiSetRectRgn(hRgn, 0, 0, 0, 0) ? NULLREGION : ERROR);
   }
   else
   {
      RegionType = NtGdiCombineRgn(hRgn, Window->UpdateRegion, hRgn, RGN_COPY);
      NtGdiOffsetRgn(
         hRgn,
         Window->WindowRect.left - Window->ClientRect.left,
         Window->WindowRect.top - Window->ClientRect.top);
   }

   IntReleaseWindowObject(Window);

   if (bErase && RegionType != NULLREGION && RegionType != ERROR)
   {
      NtUserRedrawWindow(hWnd, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN);
   }
  
   return RegionType;
}

/*
 * NtUserGetUpdateRect
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserGetUpdateRect(HWND hWnd, LPRECT lpRect, BOOL fErase)
{
   HRGN hRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

   if (!lpRect)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   NtUserGetUpdateRgn(hWnd, hRgn, fErase);
   NtGdiGetRgnBox(hRgn, lpRect);

   return lpRect->left < lpRect->right && lpRect->top < lpRect->bottom;
}

/*
 * NtUserRedrawWindow
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserRedrawWindow(HWND hWnd, CONST RECT *lprcUpdate, HRGN hrgnUpdate,
   UINT flags)
{
   RECT SafeUpdateRect;
   NTSTATUS Status;
   PWINDOW_OBJECT Wnd;

   if (!(Wnd = IntGetWindowObject(hWnd ? hWnd : IntGetDesktopWindow())))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if (lprcUpdate != NULL)
   {
      Status = MmCopyFromCaller(&SafeUpdateRect, (PRECT)lprcUpdate,
         sizeof(RECT));

      if (!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return FALSE;
      }
   }

   Status = IntRedrawWindow(Wnd, NULL == lprcUpdate ? NULL : &SafeUpdateRect,
      hrgnUpdate, flags);

   if (!NT_SUCCESS(Status))
   {
      /* IntRedrawWindow fails only in case that flags are invalid */
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
 
   return TRUE;
}

/* EOF */
