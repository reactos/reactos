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
 *  $Id: painting.c,v 1.36 2003/11/20 21:21:29 navaraf Exp $
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
 *    the fake paint message from message queue.
 */

HRGN FASTCALL
IntGetNCUpdateRegion(PWINDOW_OBJECT Window)
{
   HRGN WindowRgn;
   HRGN NonclientRgn;

   Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
   MsqDecPaintCountQueue(Window->MessageQueue);
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
   if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
       WindowRgn, RGN_AND) == NULLREGION)
   {
      NtGdiDeleteObject(Window->UpdateRegion);
      Window->UpdateRegion = NULL;
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
               DeleteObject(WindowObject->UpdateRegion);
               WindowObject->UpdateRegion = NULL;
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
            NtUserSendMessage(hWnd, WM_NCPAINT, (WPARAM)IntGetNCUpdateRegion(Window), 0);
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
IntInvalidateWindows(PWINDOW_OBJECT Window, HRGN hRgn, ULONG Flags)
{
   INT RgnType;

   if (!(Window->Style & WS_VISIBLE))
   {
      return;
   }
   
   /*
    * Clip the given region with window rectangle (or region)
    */

#ifdef TODO
   if (!Window->WindowRegion)
#endif
   {
      HRGN hRgnWindow;
      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
      NtGdiOffsetRgn(hRgnWindow,
         -Window->WindowRect.left,
         -Window->WindowRect.top);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnWindow, RGN_AND);        
   }
#ifdef TODO
   else
   {
      RgnType = NtGdiCombineRgn(hRgn, hRgn, Window->WindowRegion, RGN_AND);        
   }
#endif

   if (RgnType == NULLREGION)
   {
      return;
   }

   /*
    * Remove fake posted paint messages from window message queue
    */

#ifndef DESKTOP_IN_CSRSS
   if (!IntIsDesktopWindow(Window))
   {
#endif
      if (Window->UpdateRegion != NULL ||
          Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
      {
         MsqDecPaintCountQueue(Window->MessageQueue);
      }

      if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
      {
         MsqDecPaintCountQueue(Window->MessageQueue);
      }
#ifndef DESKTOP_IN_CSRSS
   }
#endif

   /*
    * Update the region and flags
    */

   if (Flags & RDW_INVALIDATE)
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

   if (Flags & RDW_VALIDATE)
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
    * Fake post paint messages to window message queue
    */

#ifndef DESKTOP_IN_CSRSS
   if (!IntIsDesktopWindow(Window))
   {
#endif
      if (Window->UpdateRegion != NULL ||
          Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
      {
         MsqIncPaintCountQueue(Window->MessageQueue);
      }

      if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
      {
         MsqIncPaintCountQueue(Window->MessageQueue);
      }
#ifndef DESKTOP_IN_CSRSS
   }
#endif

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
            HRGN hRgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
            Child = IntGetWindowObject(*phWnd);
            NtGdiCombineRgn(hRgnTemp, hRgn, 0, RGN_COPY);
            NtGdiOffsetRgn(hRgnTemp,
               Window->WindowRect.left - Child->WindowRect.left,
               Window->WindowRect.top - Child->WindowRect.top);
            IntInvalidateWindows(Child, hRgnTemp, Flags);
            NtGdiDeleteObject(hRgnTemp);
            IntReleaseWindowObject(Child);
         }
         ExFreePool(List);
      }
   }
}

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
            NtGdiOffsetRgn(Child->UpdateRegion, -OffsetX, -OffsetY);
         }
      }
      IntReleaseWindowObject(ParentWindow);
      Parent = NtUserGetAncestor(Parent, GA_PARENT);
   }
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

   if ((Flags & (RDW_VALIDATE | RDW_INVALIDATE)) == (RDW_VALIDATE | RDW_INVALIDATE))
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
      IntInvalidateWindows(Window, hRgn, Flags);
   }

   /*
    * Validate parent covered by region.
    */

   if (Window->UpdateRegion != NULL && Flags & RDW_UPDATENOW)
   {
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

   NtGdiDeleteObject(hRgn);

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

#if 0
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
#endif
         for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
         {
            if (Child->Style & WS_VISIBLE)
            {
               hFoundWnd = IntFindWindowToRepaint(Child->Self, Thread);
               if (hFoundWnd != NULL)
                  break;
            }
         }
#if 0
      }
#endif

      ExReleaseFastMutex(&Window->ChildrenListLock);
      IntReleaseWindowObject(Window);

      return hFoundWnd;
   }
}

BOOL FASTCALL
IntGetPaintMessage(PWINDOW_OBJECT Wnd, PW32THREAD Thread, MSG *Message)
{
   PWINDOW_OBJECT Window;

   if (Wnd)
      Message->hwnd = IntFindWindowToRepaint(Wnd->Self, PsGetWin32Thread());
   else
      Message->hwnd = IntFindWindowToRepaint(NULL, PsGetWin32Thread());

   if (Message->hwnd == NULL)
   {
      PUSER_MESSAGE_QUEUE MessageQueue;

      DPRINT1("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found!\n");
      MessageQueue = (PUSER_MESSAGE_QUEUE)Thread->MessageQueue;
      ExAcquireFastMutex(&MessageQueue->Lock);
      DPRINT1("Current paint count: %d\n", MessageQueue->PaintCount);
      MessageQueue->PaintCount = 0;
      MessageQueue->PaintPosted = FALSE;
      ExReleaseFastMutex(&MessageQueue->Lock);
      return FALSE;
   }

   Window = IntGetWindowObject(Message->hwnd);
   if (Window != NULL)
   {
      if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
      {
         Message->message = WM_NCPAINT;
         Message->wParam = (WPARAM)IntGetNCUpdateRegion(Window);
         Message->lParam = 0;
      } else
      {
         Message->message = WM_PAINT;
         Message->wParam = Message->lParam = 0;
         if (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
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

   if (Window->UpdateRegion != NULL)
   {
      MsqDecPaintCountQueue(Window->MessageQueue);
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

   if (!(Wnd = IntGetWindowObject(hWnd)))
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
