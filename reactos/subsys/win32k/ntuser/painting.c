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
 *  $Id: painting.c,v 1.84.2.3 2004/09/12 19:21:07 weiden Exp $
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
#include <w32k.h>

#define NDEBUG
#include <win32k/debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

VOID FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN ValidRegion)
{
   PWINDOW_OBJECT OldWindow;
   PWINDOW_OBJECT ParentWindow = Child->Parent;

   while (ParentWindow)
   {
      if (!(ParentWindow->Style & WS_CLIPCHILDREN))
      {
         IntLockWindowUpdate(ParentWindow);
         if (ParentWindow->UpdateRegion != 0)
         {
            INT OffsetX, OffsetY;

            /*
             * We must offset the child region by the offset of the
             * child rect in the parent.
             */
            OffsetX = Child->WindowRect.left - ParentWindow->WindowRect.left;
            OffsetY = Child->WindowRect.top - ParentWindow->WindowRect.top;
            NtGdiOffsetRgn(ValidRegion, OffsetX, OffsetY);
            NtGdiCombineRgn(ParentWindow->UpdateRegion, ParentWindow->UpdateRegion,
               ValidRegion, RGN_DIFF);
            /* FIXME: If the resulting region is empty, remove fake posted paint message */
            NtGdiOffsetRgn(ValidRegion, -OffsetX, -OffsetY);
         }
         IntUnLockWindowUpdate(ParentWindow);
      }
      OldWindow = ParentWindow;
      ParentWindow = ParentWindow->Parent;
   }
}

/*
 * IntPaintWindows
 *
 * Internal function used by IntRedrawWindow.
 */

STATIC VOID FASTCALL
IntPaintWindows(PWINDOW_OBJECT Window, ULONG Flags)
{
  HDC hDC;
  HRGN TempRegion;
  
  ASSERT(Window);

  if (Flags & (RDW_ERASENOW | RDW_UPDATENOW))
    {
      if (Window->Flags & WINDOWOBJECT_NEED_NCPAINT)
        {
          IntLockWindowUpdate(Window);
          if (Window->NCUpdateRegion)
            {
              IntValidateParent(Window, Window->NCUpdateRegion);
            }
          TempRegion = Window->NCUpdateRegion;
          if ((HANDLE) 1 != TempRegion && NULL != TempRegion)
            {
              GDIOBJ_SetOwnership(TempRegion, PsGetCurrentProcess());
            }
          Window->NCUpdateRegion = NULL;
          Window->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
          MsqDecPaintCountQueue(Window->MessageQueue);
          IntUnLockWindowUpdate(Window);
          IntSendMessage(Window, WM_NCPAINT, (WPARAM)TempRegion, 0);
          if(!IntIsWindow(Window))
          {
            return;
          }
        }

      if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
        {
          if (Window->UpdateRegion)
            {
              IntValidateParent(Window, Window->UpdateRegion);
              hDC = IntGetDCEx(Window, 0, DCX_CACHE | DCX_USESTYLE |
                                           DCX_INTERSECTUPDATE);
              if (hDC != NULL)
                {
                  if (IntSendMessage(Window, WM_ERASEBKGND, (WPARAM)hDC, 0))
                    {
                      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
                    }
		  IntReleaseDC(Window, hDC);
		  if(!IntIsWindow(Window))
		  {
		    return;
		  }
                }
            }
        }

      if (Flags & RDW_UPDATENOW)
        {
          if (Window->UpdateRegion != NULL ||
              Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
            {
              IntSendMessage(Window, WM_PAINT, 0, 0);
              if(!IntIsWindow(Window))
              {
                return;
              }
              IntLockWindowUpdate(Window);
              if (Window->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
                {
                  Window->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
                  if (Window->UpdateRegion == NULL)
                    {
                      MsqDecPaintCountQueue(Window->MessageQueue);
                    }
                }
              IntUnLockWindowUpdate(Window);
            }
        }
    }

  /*
   * Check that the window is still valid at this point
   */

  if (! IntIsWindow(Window))
    {
      return;
    }

  /*
   * Paint child windows.
   */
  if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
      ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)))
    {
      PWINDOW_OBJECT Current;
      
      for(Current = Window->FirstChild; Current != NULL; Current = Current->NextSibling)
      {
        if(Current->Style & WS_VISIBLE)
        {
          IntPaintWindows(Window, Flags);
        }
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
   BOOL HadPaintMessage, HadNCPaintMessage;
   BOOL HasPaintMessage, HasNCPaintMessage;

   /*
    * Clip the given region with window rectangle (or region)
    */

   IntLockWindowUpdate(Window);
   if (!Window->WindowRegion || (Window->Style & WS_MINIMIZE))
   {
      HRGN hRgnWindow;

      IntUnLockWindowUpdate(Window);
      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
      NtGdiOffsetRgn(hRgnWindow,
         -Window->WindowRect.left,
         -Window->WindowRect.top);
      RgnType = NtGdiCombineRgn(hRgn, hRgn, hRgnWindow, RGN_AND);
      NtGdiDeleteObject(hRgnWindow);
   }
   else
   {
      RgnType = NtGdiCombineRgn(hRgn, hRgn, Window->WindowRegion, RGN_AND);
      IntUnLockWindowUpdate(Window);
   }

   /*
    * Save current state of pending updates
    */

   IntLockWindowUpdate(Window);
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
         GDIOBJ_SetOwnership(Window->UpdateRegion, NULL);
      }

      if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
          hRgn, RGN_OR) == NULLREGION)
      {
         GDIOBJ_SetOwnership(Window->UpdateRegion, PsGetCurrentProcess());
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
            GDIOBJ_SetOwnership(Window->UpdateRegion, PsGetCurrentProcess());
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
    * Split the nonclient update region.
    */

   if (NULL != Window->UpdateRegion)
   {
      HRGN hRgnWindow, hRgnNonClient;

      hRgnWindow = UnsafeIntCreateRectRgnIndirect(&Window->ClientRect);
      NtGdiOffsetRgn(hRgnWindow,
         -Window->WindowRect.left,
         -Window->WindowRect.top);

      hRgnNonClient = NtGdiCreateRectRgn(0, 0, 0, 0);
      GDIOBJ_SetOwnership(hRgnNonClient, NULL);
      if (NtGdiCombineRgn(hRgnNonClient, Window->UpdateRegion,
          hRgnWindow, RGN_DIFF) == NULLREGION)
      {
         GDIOBJ_SetOwnership(hRgnNonClient, PsGetCurrentProcess());
         NtGdiDeleteObject(hRgnNonClient);
         hRgnNonClient = NULL;
      }

      /*
       * Remove the nonclient region from the standard update region.
       */

      if (NtGdiCombineRgn(Window->UpdateRegion, Window->UpdateRegion,
          hRgnWindow, RGN_AND) == NULLREGION)
      {
         GDIOBJ_SetOwnership(Window->UpdateRegion, PsGetCurrentProcess());
         NtGdiDeleteObject(Window->UpdateRegion);
         Window->UpdateRegion = NULL;
      }

      if (Window->NCUpdateRegion == NULL)
      {
         Window->NCUpdateRegion = hRgnNonClient;
      }
      else
      {
         NtGdiCombineRgn(Window->NCUpdateRegion, Window->NCUpdateRegion,
            hRgnNonClient, RGN_OR);
         if (NULL != hRgnNonClient)
         {
            GDIOBJ_SetOwnership(hRgnNonClient, PsGetCurrentProcess());
            NtGdiDeleteObject(hRgnNonClient);
         }
      }

      NtGdiDeleteObject(hRgnWindow);
   }

   /*
    * Process children if needed
    */

   if (!(Flags & RDW_NOCHILDREN) && !(Window->Style & WS_MINIMIZE) &&
       ((Flags & RDW_ALLCHILDREN) || !(Window->Style & WS_CLIPCHILDREN)))
   {
      PWINDOW_OBJECT Child;
      
      for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
      {
        if(Child->Style & WS_VISIBLE)
        {
          /*
           * Recursive call to update children UpdateRegion
           */
          HRGN hRgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
          NtGdiCombineRgn(hRgnTemp, hRgn, 0, RGN_COPY);
          NtGdiOffsetRgn(hRgnTemp,
                         Window->WindowRect.left - Child->WindowRect.left,
                         Window->WindowRect.top - Child->WindowRect.top);
          IntInvalidateWindows(Child, hRgnTemp, Flags);
          NtGdiDeleteObject(hRgnTemp);
        }
      }
   }

   /*
    * Fake post paint messages to window message queue if needed
    */

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

   IntUnLockWindowUpdate(Window);
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
   PWINDOW_OBJECT Wnd;
   
   for(Wnd = Window; Wnd != NULL; Wnd = IntGetParentObject(Wnd))
   {
      if (!(Wnd->Style & WS_VISIBLE) ||
          ((Wnd->Style & WS_MINIMIZE) && (Wnd != Window)))
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
IntRedrawWindow(PWINDOW_OBJECT Window, LPRECT UpdateRect, HRGN UpdateRgn, UINT Flags)
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
      IntInvalidateWindows(Window, hRgn, Flags);
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

   if (hRgn != NULL)
   {
      NtGdiDeleteObject(hRgn);
   }

   return TRUE;
}

PWINDOW_OBJECT STDCALL
IntFindWindowToRepaint(PWINDOW_OBJECT Parent, PW32THREAD Thread)
{
   PWINDOW_OBJECT Child;
   PWINDOW_OBJECT FoundWnd = NULL;

   ASSERT(Parent);

   if (IntIsWindowDirty(Parent) && 
       IntWndBelongsToThread(Parent, Thread))
   {
      return Parent;
   }

   for (Child = Parent->FirstChild; Child; Child = Child->NextSibling)
   {
      if (IntIsWindowDirty(Child) &&
          IntWndBelongsToThread(Child, Thread))
      {
         FoundWnd = Child;
         break;
      }
   }

   if (FoundWnd != NULL)
   {
     return FoundWnd;
   }

   for (Child = Parent->FirstChild; Child; Child = Child->NextSibling)
   {
      if ((FoundWnd = IntFindWindowToRepaint(Child, Thread)))
      {
         break;
      }
   }

   return FoundWnd;
}

BOOL FASTCALL
IntGetPaintMessage(PWINDOW_OBJECT Window, UINT MsgFilterMin, UINT MsgFilterMax,
                   PW32THREAD Thread, PKMSG Message, BOOL Remove)
{
   PWINDOW_OBJECT RepaintWnd;
   
   PUSER_MESSAGE_QUEUE MessageQueue = (PUSER_MESSAGE_QUEUE)Thread->MessageQueue;

   if (!MessageQueue->PaintPosted)
   {
      return FALSE;
   }

   if (Window)
   {
      RepaintWnd = IntFindWindowToRepaint(Window, PsGetWin32Thread());
   }
   else
   {
      RepaintWnd = IntFindWindowToRepaint(IntGetDesktopWindow(), PsGetWin32Thread());
   }
   Message->Window = RepaintWnd;

   if (RepaintWnd == NULL)
   {
      if (NULL == Window)
      {
         DPRINT1("PAINTING BUG: Thread marked as containing dirty windows, but no dirty windows found!\n");
         IntLockMessageQueue(MessageQueue);
         MessageQueue->PaintPosted = 0;
         MessageQueue->PaintCount = 0;
         IntUnLockMessageQueue(MessageQueue);
      }
      return FALSE;
   }

   if (RepaintWnd != NULL)
   {
      IntLockWindowUpdate(RepaintWnd);
      if (0 != (RepaintWnd->Flags & WINDOWOBJECT_NEED_NCPAINT)
          && ((0 == MsgFilterMin && 0 == MsgFilterMax) ||
              (MsgFilterMin <= WM_NCPAINT &&
               WM_NCPAINT <= MsgFilterMax)))
      {
         Message->message = WM_NCPAINT;
         Message->wParam = (WPARAM)RepaintWnd->NCUpdateRegion;
         Message->lParam = 0;
         if (Remove)
         {
            IntValidateParent(RepaintWnd, RepaintWnd->NCUpdateRegion);
            RepaintWnd->NCUpdateRegion = NULL;
            RepaintWnd->Flags &= ~WINDOWOBJECT_NEED_NCPAINT;
            MsqDecPaintCountQueue(RepaintWnd->MessageQueue);
         }
      } else if ((0 == MsgFilterMin && 0 == MsgFilterMax) ||
                 (MsgFilterMin <= WM_PAINT &&
                  WM_PAINT <= MsgFilterMax))
      {
         Message->message = WM_PAINT;
         Message->wParam = Message->lParam = 0;
         if (Remove && RepaintWnd->Flags & WINDOWOBJECT_NEED_INTERNALPAINT)
         {
            RepaintWnd->Flags &= ~WINDOWOBJECT_NEED_INTERNALPAINT;
            if (RepaintWnd->UpdateRegion == NULL)
            {
               MsqDecPaintCountQueue(RepaintWnd->MessageQueue);
            }
         }
      }
      IntUnLockWindowUpdate(RepaintWnd);

      return TRUE;
   }

   return FALSE;
}

PWINDOW_OBJECT FASTCALL
IntFixCaret(PWINDOW_OBJECT Window, LPRECT lprc, UINT flags)
{
   PUSER_MESSAGE_QUEUE ActiveQueue;
   PTHRDCARETINFO CaretInfo;

   if(!(ActiveQueue = IntGetActiveMessageQueue()))
   {
     return NULL;
   }
   
   CaretInfo = ActiveQueue->CaretInfo;
   if (CaretInfo != NULL && (CaretInfo->Window == Window ||
                             ((flags & SW_SCROLLCHILDREN) && IntIsChildWindow(Window, CaretInfo->Window))))
   {
      POINT pt, FromOffset, ToOffset, Offset;
      RECT rcCaret;
      
      pt.x = CaretInfo->Pos.x;
      pt.y = CaretInfo->Pos.y;
      IntGetClientOrigin(CaretInfo->Window, &FromOffset);
      IntGetClientOrigin(Window, &ToOffset);
      Offset.x = FromOffset.x - ToOffset.x;
      Offset.y = FromOffset.y - ToOffset.y;
      rcCaret.left = pt.x;
      rcCaret.top = pt.y;
      rcCaret.right = pt.x + CaretInfo->Size.cx;
      rcCaret.bottom = pt.y + CaretInfo->Size.cy;
      if (NtGdiIntersectRect(lprc, lprc, &rcCaret))
      {
         #if 0
	 IntHideCaret(0);
	 #endif
         lprc->left = pt.x;
         lprc->top = pt.y;
         return CaretInfo->Window;
      }
   }

   return NULL;
}

HDC FASTCALL
IntBeginPaint(PWINDOW_OBJECT Window, PAINTSTRUCT* lPs)
{
   ASSERT(Window);

   #if 0
   IntHideCaret(hWnd);
   #endif

   if (!(lPs->hdc = IntGetDCEx(Window, 0, DCX_INTERSECTUPDATE | DCX_WINDOWPAINT | DCX_USESTYLE)))
   {
      return NULL;
   }

   IntLockWindowUpdate(Window);
   if (Window->UpdateRegion != NULL)
   {
      MsqDecPaintCountQueue(Window->MessageQueue);
      IntValidateParent(Window, Window->UpdateRegion);
      NtGdiGetRgnBox(Window->UpdateRegion, &lPs->rcPaint);
      NtGdiOffsetRect(&lPs->rcPaint,
         Window->WindowRect.left - Window->ClientRect.left,
         Window->WindowRect.top - Window->ClientRect.top);
      GDIOBJ_SetOwnership(Window->UpdateRegion, PsGetCurrentProcess());
      NtGdiDeleteObject(Window->UpdateRegion);
      Window->UpdateRegion = NULL;
   }
   else
   {
      IntGetClientRect(Window, &lPs->rcPaint);
   }
   IntUnLockWindowUpdate(Window);

   if (Window->Flags & WINDOWOBJECT_NEED_ERASEBKGND)
   {
      Window->Flags &= ~WINDOWOBJECT_NEED_ERASEBKGND;
      lPs->fErase = !IntSendMessage(Window, WM_ERASEBKGND, (WPARAM)lPs->hdc, 0);
   }
   else
   {
      lPs->fErase = FALSE;
   }

   return lPs->hdc;
}

BOOL FASTCALL
IntEndPaint(PWINDOW_OBJECT Window, CONST PAINTSTRUCT* lPs)
{
   IntReleaseDC(Window, lPs->hdc);
   #if 0
   IntShowCaret(Window);
   #endif

   return TRUE;
}


INT FASTCALL
IntGetUpdateRgn(PWINDOW_OBJECT Window, HRGN hRgn, BOOL bErase)
{
   int RegionType;
   
   ASSERT(Window);
    
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

   if (bErase && RegionType != NULLREGION && RegionType != ERROR)
   {
      IntRedrawWindow(Window, NULL, NULL, RDW_ERASENOW | RDW_NOCHILDREN);
   }
  
   return RegionType;
}


BOOL FASTCALL
IntGetUpdateRect(PWINDOW_OBJECT Window, LPRECT Rect, BOOL Erase)
{
  HRGN Rgn;
  PROSRGNDATA RgnData;

  ASSERT(Window);
  ASSERT(Rect);

  Rgn = NtGdiCreateRectRgn(0, 0, 0, 0);
  if (NULL == Rgn)
    {
      NtGdiDeleteObject(Rgn);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return FALSE;
    }
  
  IntGetUpdateRgn(Window, Rgn, Erase);
  RgnData = RGNDATA_LockRgn(Rgn);
  if (NULL == RgnData)
    {
      NtGdiDeleteObject(Rgn);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return FALSE;
    }
  if (ERROR == UnsafeIntGetRgnBox(RgnData, Rect))
    {
      RGNDATA_UnlockRgn(RgnData);
      NtGdiDeleteObject(Rgn);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return FALSE;
    }
  RGNDATA_UnlockRgn(RgnData);
  NtGdiDeleteObject(Rgn);

  return ((Rect->left < Rect->right) && (Rect->top < Rect->bottom));
}


DWORD FASTCALL
IntScrollDC(HDC hDC, INT dx, INT dy, LPRECT lprcScroll,
            LPRECT lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate)
{
   RECT rSrc, rClipped_src, rClip, rDst, offset;
   PDC DC;

   /*
    * Compute device clipping region (in device coordinates).
    */

   DC = DC_LockDc(hDC);
   if (NULL == DC)
     {
       return FALSE;
     }
   if (lprcScroll)
      rSrc = *lprcScroll;
   else
      IntGdiGetClipBox(hDC, &rSrc);
   IntLPtoDP(DC, (LPPOINT)&rSrc, 2);

   if (lprcClip)
      rClip = *lprcClip;
   else
      IntGdiGetClipBox(hDC, &rClip);
   IntLPtoDP(DC, (LPPOINT)&rClip, 2);

   NtGdiIntersectRect(&rClipped_src, &rSrc, &rClip);

   rDst = rClipped_src;
   NtGdiSetRect(&offset, 0, 0, dx, dy);
   IntLPtoDP(DC, (LPPOINT)&offset, 2);
   NtGdiOffsetRect(&rDst, offset.right - offset.left,  offset.bottom - offset.top);
   NtGdiIntersectRect(&rDst, &rDst, &rClip);

   /*
    * Copy bits, if possible.
    */

   if (rDst.bottom > rDst.top && rDst.right > rDst.left)
   {
      RECT rDst_lp = rDst, rSrc_lp = rDst;

      NtGdiOffsetRect(&rSrc_lp, offset.left - offset.right, offset.top - offset.bottom);
      IntDPtoLP(DC, (LPPOINT)&rDst_lp, 2);
      IntDPtoLP(DC, (LPPOINT)&rSrc_lp, 2);
      DC_UnlockDc(DC);

      if (!NtGdiBitBlt(hDC, rDst_lp.left, rDst_lp.top, rDst_lp.right - rDst_lp.left,
                       rDst_lp.bottom - rDst_lp.top, hDC, rSrc_lp.left, rSrc_lp.top,
                       SRCCOPY))
         return FALSE;
   }
   else
   {
      DC_UnlockDc(DC);
   }

   /*
    * Compute update areas.  This is the clipped source or'ed with the
    * unclipped source translated minus the clipped src translated (rDst)
    * all clipped to rClip.
    */

   if (hrgnUpdate || lprcUpdate)
   {
      HRGN hRgn = hrgnUpdate, hRgn2;

      if (hRgn)
         NtGdiSetRectRgn(hRgn, rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom);
      else
         hRgn = NtGdiCreateRectRgn(rClipped_src.left, rClipped_src.top, rClipped_src.right, rClipped_src.bottom);

      hRgn2 = UnsafeIntCreateRectRgnIndirect(&rSrc);
      NtGdiOffsetRgn(hRgn2, offset.right - offset.left,  offset.bottom - offset.top);
      NtGdiCombineRgn(hRgn, hRgn, hRgn2, RGN_OR);

      NtGdiSetRectRgn(hRgn2, rDst.left, rDst.top, rDst.right, rDst.bottom);
      NtGdiCombineRgn(hRgn, hRgn, hRgn2, RGN_DIFF);
        
      NtGdiSetRectRgn(hRgn2, rClip.left, rClip.top, rClip.right, rClip.bottom);
      NtGdiCombineRgn(hRgn, hRgn, hRgn2, RGN_AND);

      if (lprcUpdate)
      {
         NtGdiGetRgnBox(hRgn, lprcUpdate);

         /* Put the lprcUpdate in logical coordinate */
         NtGdiDPtoLP(hDC, (LPPOINT)lprcUpdate, 2);
      }
      if (!hrgnUpdate)
         NtGdiDeleteObject(hRgn);
      NtGdiDeleteObject(hRgn2);
   }
   return TRUE;
}


DWORD FASTCALL
IntScrollWindowEx(PWINDOW_OBJECT Window, INT dx, INT dy, LPRECT rect,
                  LPRECT clipRect, HRGN hrgnUpdate, LPRECT rcUpdate, UINT flags)
{
   RECT rc, cliprc, caretrc;
   INT Result;
   PWINDOW_OBJECT CaretWindow;
   HDC hDC;
   HRGN hrgnTemp;
   BOOL bUpdate;
   BOOL bOwnRgn = TRUE;

   ASSERT(Window);
   
   if (!IntIsWindowDrawable(Window))
   {
      return ERROR;
   }
   
   bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));

   IntGetClientRect(Window, &rc);
   if (rect)
      NtGdiIntersectRect(&rc, &rc, rect);

   if (clipRect)
      NtGdiIntersectRect(&cliprc, &rc, clipRect);
   else
      cliprc = rc;

   if (cliprc.right <= cliprc.left || cliprc.bottom <= cliprc.top ||
       (dx == 0 && dy == 0))
   { 
      return NULLREGION;
   }

   caretrc = rc;
   CaretWindow = IntFixCaret(Window, &caretrc, flags);

   if (hrgnUpdate)
      bOwnRgn = FALSE;
   else if (bUpdate)
      hrgnUpdate = NtGdiCreateRectRgn(0, 0, 0, 0);

   hDC = IntGetDCEx(Window, 0, DCX_CACHE | DCX_USESTYLE);
   if (hDC)
   {
      IntScrollDC(hDC, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate);
      IntReleaseDC(Window, hDC);
   }

   /* 
    * Take into account the fact that some damage may have occurred during
    * the scroll.
    */

   hrgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
   Result = IntGetUpdateRgn(Window, hrgnTemp, FALSE);
   if (Result != NULLREGION)
   {
      HRGN hrgnClip = UnsafeIntCreateRectRgnIndirect(&cliprc);
      NtGdiOffsetRgn(hrgnTemp, dx, dy);
      NtGdiCombineRgn(hrgnTemp, hrgnTemp, hrgnClip, RGN_AND);
      IntRedrawWindow(Window, NULL, hrgnTemp, RDW_INVALIDATE | RDW_ERASE);
      NtGdiDeleteObject(hrgnClip);
   }
   NtGdiDeleteObject(hrgnTemp);

   if (flags & SW_SCROLLCHILDREN)
   {
      PWINDOW_OBJECT Current;
      RECT r, dummy;
      POINT ClientOrigin;
      
      for(Current = Window->FirstChild; Current != NULL; Current = Current->NextSibling)
      {
        r = Current->WindowRect;
        IntGetClientOrigin(Window, &ClientOrigin);
        r.left -= ClientOrigin.x;
        r.top -= ClientOrigin.y;
        r.right -= ClientOrigin.x;
        r.bottom -= ClientOrigin.y;
        if(!rect || NtGdiIntersectRect(&dummy, &r, &rc))
        {
          WinPosSetWindowPos(Current, 0, r.left + dx, r.top + dy, 0, 0,
                             SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                             SWP_NOREDRAW);
        }
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
      IntRedrawWindow(Window, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
                      ((flags & SW_ERASE) ? RDW_ERASENOW : 0) |
                      ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0));

   if (bOwnRgn && hrgnUpdate)
      NtGdiDeleteObject(hrgnUpdate);
   
   if (CaretWindow != NULL)
   {
      #if 0
      IntSetCaretPos(CaretWindow.left + dx, caretrc.top + dy);
      IntShowCaret(hwndCaret);
      #endif
   }
    
   return Result;
}

