/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Window scrolling function
 *  FILE:             win32ss/user/ntuser/scrollex.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 */

#include <win32k.h>

DBG_DEFAULT_CHANNEL(UserPainting);

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

/*
    Old GetUpdateRgn, for scrolls, see above note.
 */
INT FASTCALL
co_IntGetUpdateRgn(PWND Window, PREGION Rgn, BOOL bErase)
{
    int RegionType;
    RECTL Rect;
    PREGION UpdateRgn;

    ASSERT_REFS_CO(Window);

    if (bErase)
    {
       co_IntPaintWindows(Window, RDW_NOCHILDREN, FALSE);
    }

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

    return RegionType;
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

   if (GdiGetClipBox(hDC, &rcClip) == ERROR)
   {
       ERR("GdiGetClipBox failed for HDC %p\n", hDC);
       return ERROR;
   }

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

DWORD
FASTCALL
IntScrollWindowEx(
   PWND Window,
   INT dx,
   INT dy,
   const RECT *prcScroll,
   const RECT *prcClip,
   HRGN hrgnUpdate,
   LPRECT prcUpdate,
   UINT flags)
{
   RECTL rcScroll, rcClip, rcCaret;
   INT Result;
   PWND CaretWnd;
   HDC hDC;
   PREGION RgnUpdate = NULL, RgnTemp, RgnWinupd = NULL;
   HWND hwndCaret;
   DWORD dcxflags = 0;
   int rdw_flags;
   USER_REFERENCE_ENTRY CaretRef;

   IntGetClientRect(Window, &rcClip);

   if (prcScroll)
   {
      RECTL_bIntersectRect(&rcScroll, &rcClip, prcScroll);
   }
   else
      rcScroll = rcClip;

   if (prcClip)
   {
      RECTL_bIntersectRect(&rcClip, &rcClip, prcClip);
   }

   if (rcClip.right <= rcClip.left || rcClip.bottom <= rcClip.top ||
         (dx == 0 && dy == 0))
   {
      return NULLREGION;
   }

   /* We must use a copy of the region, as we can't hold an exclusive lock
    * on it while doing callouts to user-mode */
   RgnUpdate = IntSysCreateRectpRgn(0, 0, 0, 0);
   if(!RgnUpdate)
   {
       EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return ERROR;
   }

   if (hrgnUpdate)
   {
       RgnTemp = REGION_LockRgn(hrgnUpdate);
       if (!RgnTemp)
       {
           EngSetLastError(ERROR_INVALID_HANDLE);
           return ERROR;
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
      return ERROR;
   }

   rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ? RDW_INVALIDATE | RDW_ERASE : RDW_INVALIDATE ;

   rcCaret = rcScroll;
   hwndCaret = co_IntFixCaret(Window, &rcCaret, flags);

   Result = UserScrollDC( hDC,
                          dx,
                          dy,
                          &rcScroll,
                          &rcClip,
                          NULL,
                          RgnUpdate,
                          prcUpdate);

   UserReleaseDC(Window, hDC, FALSE);

   /*
    * Take into account the fact that some damage may have occurred during
    * the scroll. Keep a copy in hrgnWinupd to be added to hrngUpdate at the end.
    */

   RgnTemp = IntSysCreateRectpRgn(0, 0, 0, 0);
   if (!RgnTemp)
   {
       EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return ERROR;
   }

   if (co_IntGetUpdateRgn(Window, RgnTemp, FALSE) != NULLREGION)
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
      LPARAM lParam;

      IntGetClientOrigin(Window, &ClientOrigin);

      for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
      {
         rcChild = Child->rcWindow;
         rcChild.left -= ClientOrigin.x;
         rcChild.top -= ClientOrigin.y;
         rcChild.right -= ClientOrigin.x;
         rcChild.bottom -= ClientOrigin.y;

         if (!prcScroll || RECTL_bIntersectRect(&rcDummy, &rcChild, &rcScroll))
         {
            UserRefObjectCo(Child, &WndRef);

            if (Window->spwndParent == UserGetDesktopWindow()) // Window->spwndParent->fnid == FNID_DESKTOP )
               lParam = MAKELONG(Child->rcClient.left, Child->rcClient.top);
            else
               lParam = MAKELONG(rcChild.left + dx, rcChild.top + dy);

            /* wine sends WM_POSCHANGING, WM_POSCHANGED messages */
            /* windows sometimes a WM_MOVE */                  
            co_IntSendMessage(UserHMGetHandle(Child), WM_MOVE, 0, lParam);
            
            UserDerefObjectCo(Child);
         }
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
   {
      co_UserRedrawWindow( Window,
                           NULL,
                           RgnUpdate,
                           rdw_flags |                                    /*    HACK    */
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : RDW_NOCHILDREN) );
   }

   if (hwndCaret && (CaretWnd = UserGetWindowObject(hwndCaret)))
   {
      UserRefObjectCo(CaretWnd, &CaretRef);

      co_IntSetCaretPos(rcCaret.left + dx, rcCaret.top + dy);
      co_UserShowCaret(CaretWnd);

      UserDerefObjectCo(CaretWnd);
   }

   if (hrgnUpdate && (Result != ERROR))
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

   return Result;
}


BOOL FASTCALL
IntScrollWindow(PWND pWnd,
                int dx,
                int dy,
                CONST RECT *lpRect,
                CONST RECT *prcClip)
{
   return IntScrollWindowEx( pWnd, dx, dy, lpRect, prcClip, 0, NULL,
                            (lpRect ? 0 : SW_SCROLLCHILDREN) | (SW_ERASE|SW_INVALIDATE|SW_SCROLLWNDDCE)) != ERROR;
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

   rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ? RDW_INVALIDATE | RDW_ERASE : RDW_INVALIDATE ;

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

   if (co_IntGetUpdateRgn(Window, RgnTemp, FALSE) != NULLREGION)
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
      LPARAM lParam;

      IntGetClientOrigin(Window, &ClientOrigin);

      for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
      {
         rcChild = Child->rcWindow;
         rcChild.left -= ClientOrigin.x;
         rcChild.top -= ClientOrigin.y;
         rcChild.right -= ClientOrigin.x;
         rcChild.bottom -= ClientOrigin.y;

         if (!prcUnsafeScroll || RECTL_bIntersectRect(&rcDummy, &rcChild, &rcScroll))
         {
            UserRefObjectCo(Child, &WndRef);

            if (Window->spwndParent == UserGetDesktopWindow()) // Window->spwndParent->fnid == FNID_DESKTOP )
               lParam = MAKELONG(Child->rcClient.left, Child->rcClient.top);
            else
               lParam = MAKELONG(rcChild.left + dx, rcChild.top + dy);

            /* wine sends WM_POSCHANGING, WM_POSCHANGED messages */
            /* windows sometimes a WM_MOVE */                  
            co_IntSendMessage(UserHMGetHandle(Child), WM_MOVE, 0, lParam);
            
            UserDerefObjectCo(Child);
         }
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
   {
      co_UserRedrawWindow( Window,
                           NULL,
                           RgnUpdate,
                           rdw_flags |                                    /*    HACK    */
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : RDW_NOCHILDREN) );
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


/* EOF */
