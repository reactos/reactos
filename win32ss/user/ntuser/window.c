/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Windows
 * FILE:             win32ss/user/ntuser/window.c
 * PROGRAMERS:       Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                   Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <win32k.h>
#include <immdev.h>
#include <unaligned.h>

DBG_DEFAULT_CHANNEL(UserWnd);

INT gNestedWindowLimit = 50;

PWINDOWLIST gpwlList = NULL;
PWINDOWLIST gpwlCache = NULL;

/* HELPER FUNCTIONS ***********************************************************/

PVOID FASTCALL
IntReAllocatePoolWithTag(
    POOL_TYPE PoolType,
    PVOID pOld,
    SIZE_T cbOld,
    SIZE_T cbNew,
    ULONG Tag)
{
    PVOID pNew = ExAllocatePoolWithTag(PoolType, cbNew, Tag);
    if (!pNew)
        return NULL;

    RtlCopyMemory(pNew, pOld, min(cbOld, cbNew));
    ExFreePoolWithTag(pOld, Tag);
    return pNew;
}

BOOL FASTCALL UserUpdateUiState(PWND Wnd, WPARAM wParam)
{
    WORD Action = LOWORD(wParam);
    WORD Flags = HIWORD(wParam);

    if (Flags & ~(UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (Action)
    {
        case UIS_INITIALIZE:
            EngSetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;

        case UIS_SET:
            if (Flags & UISF_HIDEFOCUS)
                Wnd->HideFocus = TRUE;
            if (Flags & UISF_HIDEACCEL)
                Wnd->HideAccel = TRUE;
            break;

        case UIS_CLEAR:
            if (Flags & UISF_HIDEFOCUS)
                Wnd->HideFocus = FALSE;
            if (Flags & UISF_HIDEACCEL)
                Wnd->HideAccel = FALSE;
            break;
    }

    return TRUE;
}

PWND FASTCALL IntGetWindowObject(HWND hWnd)
{
   PWND Window;

   if (!hWnd) return NULL;

   Window = UserGetWindowObject(hWnd);
   if (Window)
      Window->head.cLockObj++;

   return Window;
}

PWND FASTCALL VerifyWnd(PWND pWnd)
{
    ULONG Error;

    if (!pWnd ||
        (pWnd->state & WNDS_DESTROYED) ||
        (pWnd->state2 & WNDS2_INDESTROY))
    {
        return NULL;
    }

    Error = EngGetLastError();

    if (UserObjectInDestroy(UserHMGetHandle(pWnd)))
        pWnd = NULL;

    EngSetLastError(Error);
    return pWnd;
}

PWND FASTCALL ValidateHwndNoErr(HWND hWnd)
{
    PWND Window;

    if (!hWnd)
        return NULL;

    Window = (PWND)UserGetObjectNoErr(gHandleTable, hWnd, TYPE_WINDOW);
    if (!Window || (Window->state & WNDS_DESTROYED))
        return NULL;

    return Window;
}

/* Temp HACK */
// Win: ValidateHwnd
PWND FASTCALL UserGetWindowObject(HWND hWnd)
{
    PWND Window;

   if (!hWnd)
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   Window = (PWND)UserGetObject(gHandleTable, hWnd, TYPE_WINDOW);
   if (!Window || 0 != (Window->state & WNDS_DESTROYED))
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   return Window;
}

ULONG FASTCALL
IntSetStyle( PWND pwnd, ULONG set_bits, ULONG clear_bits )
{
    ULONG styleOld, styleNew;
    styleOld = pwnd->style;
    styleNew = (pwnd->style | set_bits) & ~clear_bits;
    if (styleNew == styleOld) return styleNew;
    pwnd->style = styleNew;
    if ((styleOld ^ styleNew) & WS_VISIBLE) // State Change.
    {
       if (styleOld & WS_VISIBLE) pwnd->head.pti->cVisWindows--;
       if (styleNew & WS_VISIBLE) pwnd->head.pti->cVisWindows++;
       DceResetActiveDCEs( pwnd );
    }
    return styleOld;
}

/*
 * IntIsWindow
 *
 * The function determines whether the specified window handle identifies
 * an existing window.
 *
 * Parameters
 *    hWnd
 *       Handle to the window to test.
 *
 * Return Value
 *    If the window handle identifies an existing window, the return value
 *    is TRUE. If the window handle does not identify an existing window,
 *    the return value is FALSE.
 */

BOOL FASTCALL
IntIsWindow(HWND hWnd)
{
   PWND Window;

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      return FALSE;
   }

   return TRUE;
}

BOOL FASTCALL
IntIsWindowVisible(PWND Wnd)
{
   PWND Temp = Wnd;
   for (;;)
   {
      if (!Temp) return TRUE;
      if (!(Temp->style & WS_VISIBLE)) break;
      if (Temp->style & WS_MINIMIZE && Temp != Wnd) break;
      if (Temp->fnid == FNID_DESKTOP) return TRUE;
      Temp = Temp->spwndParent;
   }
   return FALSE;
}

PWND FASTCALL
IntGetParent(PWND Wnd)
{
   if (Wnd->style & WS_POPUP)
   {
      return Wnd->spwndOwner;
   }
   else if (Wnd->style & WS_CHILD)
   {
      return Wnd->spwndParent;
   }

   return NULL;
}

BOOL
FASTCALL
IntEnableWindow( HWND hWnd, BOOL bEnable )
{
   BOOL Update;
   PWND pWnd;
   UINT bIsDisabled;

   if(!(pWnd = UserGetWindowObject(hWnd)))
   {
      return FALSE;
   }

   /* check if updating is needed */
   bIsDisabled = !!(pWnd->style & WS_DISABLED);
   Update = bIsDisabled;

    if (bEnable)
    {
       IntSetStyle( pWnd, 0, WS_DISABLED );
    }
    else
    {
       Update = !bIsDisabled;

       co_IntSendMessage( hWnd, WM_CANCELMODE, 0, 0);

       /* Remove keyboard focus from that window if it had focus */
       if (hWnd == IntGetThreadFocusWindow())
       {
          TRACE("IntEnableWindow SF NULL\n");
          co_UserSetFocus(NULL);
       }
       IntSetStyle( pWnd, WS_DISABLED, 0 );
    }

    if (Update)
    {
        IntNotifyWinEvent(EVENT_OBJECT_STATECHANGE, pWnd, OBJID_WINDOW, CHILDID_SELF, 0);
        co_IntSendMessage(hWnd, WM_ENABLE, (LPARAM)bEnable, 0);
    }
    // Return nonzero if it was disabled, or zero if it wasn't:
    return bIsDisabled;
}

/*
 * IntWinListChildren
 *
 * Compile a list of all child window handles from given window.
 *
 * Remarks
 *    This function is similar to Wine WIN_ListChildren. The caller
 *    must free the returned list with ExFreePool.
 */

HWND* FASTCALL
IntWinListChildren(PWND Window)
{
    PWND Child;
    HWND *List;
    UINT Index, NumChildren = 0;

    if (!Window) return NULL;

    for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
    {
        ++NumChildren;
    }

    List = ExAllocatePoolWithTag(PagedPool, (NumChildren + 1) * sizeof(HWND), USERTAG_WINDOWLIST);
    if(!List)
    {
        ERR("Failed to allocate memory for children array\n");
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    Index = 0;
    for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
    {
        List[Index++] = UserHMGetHandle(Child);
    }
    List[Index] = NULL;

    return List;
}

static BOOL
IntWndIsDefaultIme(_In_ PWND Window)
{
    PTHREADINFO pti = Window->head.pti;

    return (IS_IMM_MODE() && !(pti->TIF_flags & TIF_INCLEANUP) &&
            Window == pti->spwndDefaultIme);
}

HWND* FASTCALL
IntWinListOwnedPopups(PWND Window)
{
    PWND Child, Desktop;
    HWND *List;
    UINT Index, NumOwned = 0;

    Desktop = co_GetDesktopWindow(Window);
    if (!Desktop)
        return NULL;

    for (Child = Desktop->spwndChild; Child; Child = Child->spwndNext)
    {
        if (Child->spwndOwner == Window && !IntWndIsDefaultIme(Child))
            ++NumOwned;
    }

    List = ExAllocatePoolWithTag(PagedPool, (NumOwned + 1) * sizeof(HWND), USERTAG_WINDOWLIST);
    if (!List)
    {
        ERR("Failed to allocate memory for children array\n");
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    Index = 0;
    for (Child = Desktop->spwndChild; Child; Child = Child->spwndNext)
    {
        if (Child->spwndOwner == Window && !IntWndIsDefaultIme(Child))
            List[Index++] = UserHMGetHandle(Child);
    }
    List[Index] = NULL;

    return List;
}

PWND FASTCALL
IntGetNonChildAncestor(PWND pWnd)
{
   while(pWnd && (pWnd->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
      pWnd = pWnd->spwndParent;
   return pWnd;
}

BOOL FASTCALL
IntIsTopLevelWindow(PWND pWnd)
{
   if ( pWnd->spwndParent &&
        pWnd->spwndParent == co_GetDesktopWindow(pWnd) ) return TRUE;
   return FALSE;
}

BOOL FASTCALL
IntValidateOwnerDepth(PWND Wnd, PWND Owner)
{
   INT Depth = 1;
   for (;;)
   {
      if ( !Owner ) return gNestedWindowLimit >= Depth;
      if (Owner == Wnd) break;
      Owner = Owner->spwndOwner;
      Depth++;
   }
   return FALSE;
}

HWND FASTCALL
IntGetWindow(HWND hWnd,
          UINT uCmd)
{
    PWND Wnd, FoundWnd;
    HWND Ret = NULL;

    Wnd = ValidateHwndNoErr(hWnd);
    if (!Wnd)
        return NULL;

    FoundWnd = NULL;
    switch (uCmd)
    {
            case GW_OWNER:
                if (Wnd->spwndOwner != NULL)
                    FoundWnd = Wnd->spwndOwner;
                break;

            case GW_HWNDFIRST:
                if(Wnd->spwndParent != NULL)
                {
                    FoundWnd = Wnd->spwndParent;
                    if (FoundWnd->spwndChild != NULL)
                        FoundWnd = FoundWnd->spwndChild;
                }
                break;
            case GW_HWNDNEXT:
                if (Wnd->spwndNext != NULL)
                    FoundWnd = Wnd->spwndNext;
                break;

            case GW_HWNDPREV:
                if (Wnd->spwndPrev != NULL)
                    FoundWnd = Wnd->spwndPrev;
                break;

            case GW_CHILD:
                if (Wnd->spwndChild != NULL)
                    FoundWnd = Wnd->spwndChild;
                break;

            case GW_HWNDLAST:
                FoundWnd = Wnd;
                while ( FoundWnd->spwndNext != NULL)
                    FoundWnd = FoundWnd->spwndNext;
                break;

            default:
                Wnd = NULL;
                break;
    }

    if (FoundWnd != NULL)
        Ret = UserHMGetHandle(FoundWnd);
    return Ret;
}

DWORD FASTCALL IntGetWindowContextHelpId( PWND pWnd )
{
   DWORD HelpId;

   do
   {
      HelpId = HandleToUlong(UserGetProp(pWnd, gpsi->atomContextHelpIdProp, TRUE));
      if (!HelpId) break;
      pWnd = IntGetParent(pWnd);
   }
   while (pWnd && pWnd->fnid != FNID_DESKTOP);
   return HelpId;
}


VOID
FASTCALL
IntRemoveTrackMouseEvent(
    PDESKTOP pDesk);

/***********************************************************************
 *           IntSendDestroyMsg
 */
static void IntSendDestroyMsg(HWND hWnd)
{
   PTHREADINFO ti;
   PWND Window;

   ti = PsGetCurrentThreadWin32Thread();
   Window = UserGetWindowObject(hWnd);

   if (Window)
   {
      /*
       * Look whether the focus is within the tree of windows
       * we will be destroying.
       */
      // Rule #1
      if ( ti->MessageQueue->spwndActive == Window || // Fixes CORE-106 RegSvr32 exit and return focus to CMD.
          (ti->MessageQueue->spwndActive == NULL && ti->MessageQueue == IntGetFocusMessageQueue()) )
      {
         co_WinPosActivateOtherWindow(Window);
      }

      /* Fixes CMD properties closing and returning focus to CMD */
      if (ti->MessageQueue->spwndFocus == Window)
      {
         if ((Window->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
         {
            co_UserSetFocus(Window->spwndParent);
         }
         else
         {
            co_UserSetFocus(NULL);
         }
      }

      if (ti->MessageQueue->CaretInfo.hWnd == UserHMGetHandle(Window))
      {
         co_IntDestroyCaret(ti);
      }

      /* If the window being destroyed is currently tracked... */
      if (ti->rpdesk && ti->rpdesk->spwndTrack == Window)
      {
          IntRemoveTrackMouseEvent(ti->rpdesk);
      }
   }

   /* If the window being destroyed is the current clipboard owner... */
   if (ti->ppi->prpwinsta != NULL && Window == ti->ppi->prpwinsta->spwndClipOwner)
   {
       /* ... make it release the clipboard */
       UserClipboardRelease(Window);
   }

   /* Send the WM_DESTROY to the window */
   co_IntSendMessage(hWnd, WM_DESTROY, 0, 0);

   /*
    * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
    * make sure that the window still exists when we come back.
    */
   if (IntIsWindow(hWnd))
   {
      HWND* pWndArray;
      int i;

      if (!(pWndArray = IntWinListChildren( Window ))) return;

      for (i = 0; pWndArray[i]; i++)
      {
         if (IntIsWindow( pWndArray[i] )) IntSendDestroyMsg( pWndArray[i] );
      }
      ExFreePoolWithTag(pWndArray, USERTAG_WINDOWLIST);
   }
   else
   {
      TRACE("destroyed itself while in WM_DESTROY!\n");
   }
}

static VOID
UserFreeWindowInfo(PTHREADINFO ti, PWND Wnd)
{
    PCLIENTINFO ClientInfo = GetWin32ClientInfo();

    if (!Wnd) return;

    if (ClientInfo->CallbackWnd.pWnd == DesktopHeapAddressToUser(Wnd))
    {
        ClientInfo->CallbackWnd.hWnd = NULL;
        ClientInfo->CallbackWnd.pWnd = NULL;
    }

   if (Wnd->strName.Buffer != NULL)
   {
       Wnd->strName.Length = 0;
       Wnd->strName.MaximumLength = 0;
       DesktopHeapFree(Wnd->head.rpdesk,
                       Wnd->strName.Buffer);
       Wnd->strName.Buffer = NULL;
   }

//    DesktopHeapFree(Wnd->head.rpdesk, Wnd);
//    WindowObject->hWnd = NULL;
}

/***********************************************************************
 *           co_UserFreeWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 *
 * This is the "functional" DestroyWindows function i.e. all stuff
 * done in CreateWindow is undone here and not in DestroyWindow :-P
 */
LRESULT co_UserFreeWindow(PWND Window,
                          PPROCESSINFO ProcessData,
                          PTHREADINFO ThreadData,
                          BOOLEAN SendMessages)
{
   HWND *Children;
   HWND *ChildHandle;
   PWND Child;
   PMENU Menu;
   BOOLEAN BelongsToThreadData;
   USER_REFERENCE_ENTRY Ref;

   ASSERT(Window);

   if(Window->state2 & WNDS2_INDESTROY)
   {
      TRACE("Tried to call co_UserFreeWindow() twice\n");
      return 0;
   }
   Window->state2 |= WNDS2_INDESTROY;
   Window->style &= ~WS_VISIBLE;
   Window->head.pti->cVisWindows--;

   /* remove the window already at this point from the thread window list so we
      don't get into trouble when destroying the thread windows while we're still
      in co_UserFreeWindow() */
   if (!IsListEmpty(&Window->ThreadListEntry))
       RemoveEntryList(&Window->ThreadListEntry);

   BelongsToThreadData = IntWndBelongsToThread(Window, ThreadData);

   IntDeRegisterShellHookWindow(UserHMGetHandle(Window));

   /* free child windows */
   Children = IntWinListChildren(Window);
   if (Children)
   {
      for (ChildHandle = Children; *ChildHandle; ++ChildHandle)
      {
         if ((Child = IntGetWindowObject(*ChildHandle)))
         {
            if (!IntWndBelongsToThread(Child, ThreadData))
            {
               /* send WM_DESTROY messages to windows not belonging to the same thread */
               co_IntSendMessage( UserHMGetHandle(Child), WM_ASYNC_DESTROYWINDOW, 0, 0 );
            }
            else
               co_UserFreeWindow(Child, ProcessData, ThreadData, SendMessages);

            UserDereferenceObject(Child);
         }
      }
      ExFreePoolWithTag(Children, USERTAG_WINDOWLIST);
   }

   if (SendMessages)
   {
      /*
       * Clear the update region to make sure no WM_PAINT messages will be
       * generated for this window while processing the WM_NCDESTROY.
       */
      co_UserRedrawWindow(Window, NULL, 0,
                          RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE |
                          RDW_NOINTERNALPAINT | RDW_NOCHILDREN);
      if (BelongsToThreadData)
         co_IntSendMessage(UserHMGetHandle(Window), WM_NCDESTROY, 0, 0);
   }

   UserClipboardFreeWindow(Window);

   DestroyTimersForWindow(ThreadData, Window);

   /* Unregister hot keys */
   UnregisterWindowHotKeys(Window);

   /* flush the message queue */
   MsqRemoveWindowMessagesFromQueue(Window);

   /* from now on no messages can be sent to this window anymore */
   Window->state |= WNDS_DESTROYED;
   Window->fnid |= FNID_FREED;

   /* don't remove the WINDOWSTATUS_DESTROYING bit */

   /* reset shell window handles */
   if (ThreadData->rpdesk)
   {
      if (UserHMGetHandle(Window) == ThreadData->rpdesk->rpwinstaParent->ShellWindow)
         ThreadData->rpdesk->rpwinstaParent->ShellWindow = NULL;

      if (UserHMGetHandle(Window) == ThreadData->rpdesk->rpwinstaParent->ShellListView)
         ThreadData->rpdesk->rpwinstaParent->ShellListView = NULL;
   }

   if (ThreadData->spwndDefaultIme &&
       ThreadData->spwndDefaultIme->spwndOwner == Window)
   {
      WndSetOwner(ThreadData->spwndDefaultIme, NULL);
   }

   if (IS_IMM_MODE() && Window == ThreadData->spwndDefaultIme)
   {
      UserAssignmentUnlock((PVOID*)&(ThreadData->spwndDefaultIme));
   }

   /* Fixes dialog test_focus breakage due to r66237. */
   if (ThreadData->MessageQueue->spwndFocus == Window)
      ThreadData->MessageQueue->spwndFocus = NULL;

   if (ThreadData->MessageQueue->spwndActive == Window)
      ThreadData->MessageQueue->spwndActive = NULL;

   if (ThreadData->MessageQueue->spwndCapture == Window)
   {
      IntReleaseCapture();
   }

   //// Now kill those remaining "PAINTING BUG: Thread marked as containing dirty windows" spam!!!
   if ( Window->hrgnUpdate != NULL || Window->state & WNDS_INTERNALPAINT )
   {
      MsqDecPaintCountQueue(Window->head.pti);
      if (Window->hrgnUpdate > HRGN_WINDOW && GreIsHandleValid(Window->hrgnUpdate))
      {
         IntGdiSetRegionOwner(Window->hrgnUpdate, GDI_OBJ_HMGR_POWNED);
         GreDeleteObject(Window->hrgnUpdate);
      }
      Window->hrgnUpdate = NULL;
      Window->state &= ~WNDS_INTERNALPAINT;
   }

   if (Window->state & (WNDS_SENDERASEBACKGROUND|WNDS_SENDNCPAINT))
   {
      Window->state &= ~(WNDS_SENDERASEBACKGROUND|WNDS_SENDNCPAINT);
   }

   if ( ((Window->style & (WS_CHILD|WS_POPUP)) != WS_CHILD) &&
        Window->IDMenu &&
        (Menu = UserGetMenuObject((HMENU)Window->IDMenu)))
   {
      TRACE("UFW: IDMenu %p\n",Window->IDMenu);
      IntDestroyMenuObject(Menu, TRUE);
      Window->IDMenu = 0;
   }

   if (Window->SystemMenu
        && (Menu = UserGetMenuObject(Window->SystemMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE);
      Window->SystemMenu = (HMENU)0;
   }

   DceFreeWindowDCE(Window);    /* Always do this to catch orphaned DCs */

   IntUnlinkWindow(Window);

   if (Window->PropListItems)
   {
      UserRemoveWindowProps(Window);
      TRACE("Window->PropListItems %lu\n",Window->PropListItems);
      ASSERT(Window->PropListItems==0);
   }

   /* Kill any reference to linked windows. Prev & Next are taken care of in IntUnlinkWindow */
   WndSetOwner(Window, NULL);
   WndSetParent(Window, NULL);
   WndSetChild(Window, NULL);
   WndSetLastActive(Window, NULL);

   UserRefObjectCo(Window, &Ref);
   UserMarkObjectDestroy(Window);

   IntDestroyScrollBars(Window);

   if (Window->pcls->atomClassName == gaGuiConsoleWndClass)
   {
       /* Count only console windows manually */
       co_IntUserManualGuiCheck(FALSE);
   }

   /* dereference the class */
   NT_ASSERT(Window->head.pti != NULL);
   IntDereferenceClass(Window->pcls,
                       Window->head.pti->pDeskInfo,
                       Window->head.pti->ppi);
   Window->pcls = NULL;

   if (Window->hrgnClip)
   {
      IntGdiSetRegionOwner(Window->hrgnClip, GDI_OBJ_HMGR_POWNED);
      GreDeleteObject(Window->hrgnClip);
      Window->hrgnClip = NULL;
   }
   Window->head.pti->cWindows--;

//   ASSERT(Window != NULL);
   UserFreeWindowInfo(Window->head.pti, Window);

   UserDerefObjectCo(Window);
   UserDeleteObject(UserHMGetHandle(Window), TYPE_WINDOW);

   return 0;
}

//
// Same as User32:IntGetWndProc.
//
WNDPROC FASTCALL
IntGetWindowProc(PWND pWnd,
                 BOOL Ansi)
{
   INT i;
   PCLS Class;
   WNDPROC gcpd, Ret = 0;

   ASSERT(UserIsEnteredExclusive());

   Class = pWnd->pcls;

   if (pWnd->state & WNDS_SERVERSIDEWINDOWPROC)
   {
      for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
      {
         if (GETPFNSERVER(i) == pWnd->lpfnWndProc)
         {
            if (Ansi)
               Ret = GETPFNCLIENTA(i);
            else
               Ret = GETPFNCLIENTW(i);
         }
      }
      return Ret;
   }

   if (Class->fnid == FNID_EDIT)
      Ret = pWnd->lpfnWndProc;
   else
   {
      Ret = pWnd->lpfnWndProc;

      if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
      {
         if (Ansi)
         {
            if (GETPFNCLIENTW(Class->fnid) == pWnd->lpfnWndProc)
               Ret = GETPFNCLIENTA(Class->fnid);
         }
         else
         {
            if (GETPFNCLIENTA(Class->fnid) == pWnd->lpfnWndProc)
               Ret = GETPFNCLIENTW(Class->fnid);
         }
      }
      if ( Ret != pWnd->lpfnWndProc)
         return Ret;
   }
   if ( Ansi == !!(pWnd->state & WNDS_ANSIWINDOWPROC) )
      return Ret;

   gcpd = (WNDPROC)UserGetCPD(
                       pWnd,
                      (Ansi ? UserGetCPDA2U : UserGetCPDU2A )|UserGetCPDWindow,
                      (ULONG_PTR)Ret);

   return (gcpd ? gcpd : Ret);
}

static WNDPROC
IntSetWindowProc(PWND pWnd,
                 WNDPROC NewWndProc,
                 BOOL Ansi)
{
   INT i;
   PCALLPROCDATA CallProc;
   PCLS Class;
   WNDPROC Ret, chWndProc = NULL;

   // Retrieve previous window proc.
   Ret = IntGetWindowProc(pWnd, Ansi);

   Class = pWnd->pcls;

   if (IsCallProcHandle(NewWndProc))
   {
      CallProc = UserGetObject(gHandleTable, NewWndProc, TYPE_CALLPROC);
      if (CallProc)
      {  // Reset new WndProc.
         NewWndProc = CallProc->pfnClientPrevious;
         // Reset Ansi from CallProc handle. This is expected with wine "deftest".
         Ansi = !!(CallProc->wType & UserGetCPDU2A);
      }
   }
   // Switch from Client Side call to Server Side call if match. Ref: "deftest".
   for ( i = FNID_FIRST; i <= FNID_SWITCH; i++)
   {
       if (GETPFNCLIENTW(i) == NewWndProc)
       {
          chWndProc = GETPFNSERVER(i);
          break;
       }
       if (GETPFNCLIENTA(i) == NewWndProc)
       {
          chWndProc = GETPFNSERVER(i);
          break;
       }
   }
   // If match, set/reset to Server Side and clear ansi.
   if (chWndProc)
   {
      pWnd->lpfnWndProc = chWndProc;
      pWnd->Unicode = TRUE;
      pWnd->state &= ~WNDS_ANSIWINDOWPROC;
      pWnd->state |= WNDS_SERVERSIDEWINDOWPROC;
   }
   else
   {
      pWnd->Unicode = !Ansi;
      // Handle the state change in here.
      if (Ansi)
         pWnd->state |= WNDS_ANSIWINDOWPROC;
      else
         pWnd->state &= ~WNDS_ANSIWINDOWPROC;

      if (pWnd->state & WNDS_SERVERSIDEWINDOWPROC)
         pWnd->state &= ~WNDS_SERVERSIDEWINDOWPROC;

      if (!NewWndProc) NewWndProc = pWnd->lpfnWndProc;

      if (Class->fnid <= FNID_GHOST && Class->fnid >= FNID_BUTTON)
      {
         if (Ansi)
         {
            if (GETPFNCLIENTW(Class->fnid) == NewWndProc)
               chWndProc = GETPFNCLIENTA(Class->fnid);
         }
         else
         {
            if (GETPFNCLIENTA(Class->fnid) == NewWndProc)
               chWndProc = GETPFNCLIENTW(Class->fnid);
         }
      }
      // Now set the new window proc.
      pWnd->lpfnWndProc = (chWndProc ? chWndProc : NewWndProc);
   }
   return Ret;
}


/* INTERNAL ******************************************************************/

////
//   This fixes a check for children messages that need paint while searching the parents messages!
//   Fixes wine msg:test_paint_messages:WmParentErasePaint ..
////
BOOL FASTCALL
IntIsChildWindow(PWND Parent, PWND BaseWindow)
{
   PWND Window = BaseWindow;
   do
   {
     if ( Window == NULL || (Window->style & (WS_POPUP|WS_CHILD)) != WS_CHILD )
        return FALSE;

     Window = Window->spwndParent;
   }
   while(Parent != Window);
   return TRUE;
}
////

/* Link the window into siblings list. Children and parent are kept in place. */
VOID FASTCALL
IntLinkWindow(
    PWND Wnd,
    PWND WndInsertAfter /* Set to NULL if top sibling */
)
{
    if (Wnd == WndInsertAfter)
    {
        ERR("Trying to link window 0x%p to itself\n", Wnd);
        ASSERT(WndInsertAfter != Wnd);
        return;
    }

    WndSetPrev(Wnd, WndInsertAfter);
    if (Wnd->spwndPrev)
    {
        /* Link after WndInsertAfter */
        ASSERT(Wnd != WndInsertAfter->spwndNext);
        WndSetNext(Wnd, WndInsertAfter->spwndNext);
        if (Wnd->spwndNext)
            WndSetPrev(Wnd->spwndNext, Wnd);

        ASSERT(Wnd != Wnd->spwndPrev);
        WndSetNext(Wnd->spwndPrev, Wnd);
    }
    else
    {
        /* Link at the top */
        ASSERT(Wnd != Wnd->spwndParent->spwndChild);
        WndSetNext(Wnd, Wnd->spwndParent->spwndChild);
        if (Wnd->spwndNext)
            WndSetPrev(Wnd->spwndNext, Wnd);

        WndSetChild(Wnd->spwndParent, Wnd);
    }
}

/*
 Note: Wnd->spwndParent can be null if it is the desktop.
*/
VOID FASTCALL IntLinkHwnd(PWND Wnd, HWND hWndPrev)
{
    if (hWndPrev == HWND_NOTOPMOST)
    {
        if (!(Wnd->ExStyle & WS_EX_TOPMOST) && (Wnd->ExStyle2 & WS_EX2_LINKED))
            return;  /* nothing to do */
        Wnd->ExStyle &= ~WS_EX_TOPMOST;
        hWndPrev = HWND_TOP;  /* fallback to the HWND_TOP case */
    }

    IntUnlinkWindow(Wnd);  /* unlink it from the previous location */

    if (hWndPrev == HWND_BOTTOM)
    {
        /* Link in the bottom of the list */
        PWND WndInsertAfter;

        WndInsertAfter = Wnd->spwndParent->spwndChild;
        while (WndInsertAfter && WndInsertAfter->spwndNext)
        {
            WndInsertAfter = WndInsertAfter->spwndNext;
        }

        IntLinkWindow(Wnd, WndInsertAfter);
        Wnd->ExStyle &= ~WS_EX_TOPMOST;
    }
    else if (hWndPrev == HWND_TOPMOST)
    {
        /* Link in the top of the list */
        IntLinkWindow(Wnd, NULL);
        Wnd->ExStyle |= WS_EX_TOPMOST;
    }
    else if (hWndPrev == HWND_TOP)
    {
        /* Link it after the last topmost window */
        PWND WndInsertBefore;

        WndInsertBefore = Wnd->spwndParent->spwndChild;

        if (!(Wnd->ExStyle & WS_EX_TOPMOST))  /* put it above the first non-topmost window */
        {
            while (WndInsertBefore != NULL && WndInsertBefore->spwndNext != NULL)
            {
                if (!(WndInsertBefore->ExStyle & WS_EX_TOPMOST))
                    break;

                if (WndInsertBefore == Wnd->spwndOwner)  /* keep it above owner */
                {
                    Wnd->ExStyle |= WS_EX_TOPMOST;
                    break;
                }
                WndInsertBefore = WndInsertBefore->spwndNext;
            }
        }

        IntLinkWindow(Wnd, WndInsertBefore ? WndInsertBefore->spwndPrev : NULL);
    }
    else
    {
        /* Link it after hWndPrev */
        PWND WndInsertAfter;

        WndInsertAfter = UserGetWindowObject(hWndPrev);
        /* Are we called with an erroneous handle */
        if (WndInsertAfter == NULL)
        {
            /* Link in a default position */
            IntLinkHwnd(Wnd, HWND_TOP);
            return;
        }

        if (Wnd == WndInsertAfter)
        {
            ERR("Trying to link window 0x%p to itself\n", Wnd);
            ASSERT(WndInsertAfter != Wnd);
            // FIXME: IntUnlinkWindow(Wnd) was already called. Continuing as is seems wrong!
        }
        else
        {
            IntLinkWindow(Wnd, WndInsertAfter);
        }

        /* Fix the WS_EX_TOPMOST flag */
        if (!(WndInsertAfter->ExStyle & WS_EX_TOPMOST))
        {
            Wnd->ExStyle &= ~WS_EX_TOPMOST;
        }
        else
        {
            if (WndInsertAfter->spwndNext &&
                (WndInsertAfter->spwndNext->ExStyle & WS_EX_TOPMOST))
            {
                Wnd->ExStyle |= WS_EX_TOPMOST;
            }
        }
    }
    Wnd->ExStyle2 |= WS_EX2_LINKED;
}

VOID FASTCALL
IntProcessOwnerSwap(PWND Wnd, PWND WndNewOwner, PWND WndOldOwner)
{
   if (WndOldOwner)
   {
      if (Wnd->head.pti != WndOldOwner->head.pti)
      {
         if (!WndNewOwner ||
              Wnd->head.pti == WndNewOwner->head.pti ||
              WndOldOwner->head.pti != WndNewOwner->head.pti )
         {
            //ERR("ProcessOwnerSwap Old out.\n");
            UserAttachThreadInput(Wnd->head.pti, WndOldOwner->head.pti, FALSE);
         }
      }
   }
   if (WndNewOwner)
   {
      if (Wnd->head.pti != WndNewOwner->head.pti)
      {
         if (!WndOldOwner ||
              WndOldOwner->head.pti != WndNewOwner->head.pti )
         {
            //ERR("ProcessOwnerSwap New in.\n");
            UserAttachThreadInput(Wnd->head.pti, WndNewOwner->head.pti, TRUE);
         }
      }
   }
   // FIXME: System Tray checks.
}

static
HWND FASTCALL
IntSetOwner(HWND hWnd, HWND hWndNewOwner)
{
   PWND Wnd, WndOldOwner, WndNewOwner;
   HWND ret;

   Wnd = IntGetWindowObject(hWnd);
   if(!Wnd)
      return NULL;

   WndOldOwner = Wnd->spwndOwner;

   ret = WndOldOwner ? UserHMGetHandle(WndOldOwner) : 0;
   WndNewOwner = UserGetWindowObject(hWndNewOwner);

   if (!WndNewOwner && hWndNewOwner)
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ret = NULL;
      goto Error;
   }

   /* if parent belongs to a different thread and the window isn't */
   /* top-level, attach the two threads */
   IntProcessOwnerSwap(Wnd, WndNewOwner, WndOldOwner);

   if (IntValidateOwnerDepth(Wnd, WndNewOwner))
   {
      WndSetOwner(Wnd, WndNewOwner);
   }
   else
   {
      IntProcessOwnerSwap(Wnd, WndOldOwner, WndNewOwner);
      EngSetLastError(ERROR_INVALID_PARAMETER);
      ret = NULL;
   }
Error:
   UserDereferenceObject(Wnd);
   return ret;
}

PWND FASTCALL
co_IntSetParent(PWND Wnd, PWND WndNewParent)
{
   PWND WndOldParent, pWndExam;
   BOOL WasVisible;
   POINT pt;
   int swFlags = SWP_NOSIZE|SWP_NOZORDER;

   ASSERT(Wnd);
   ASSERT(WndNewParent);
   ASSERT_REFS_CO(Wnd);
   ASSERT_REFS_CO(WndNewParent);

   if (Wnd == Wnd->head.rpdesk->spwndMessage)
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return NULL;
   }

   /* Some applications try to set a child as a parent */
   if (IntIsChildWindow(Wnd, WndNewParent))
   {
      TRACE("IntSetParent try to set a child as a parent.\n");
      EngSetLastError( ERROR_INVALID_PARAMETER );
      return NULL;
   }

   pWndExam = WndNewParent; // Load parent Window to examine.
   // Now test for set parent to parent hit.
   while (pWndExam)
   {
      if (Wnd == pWndExam)
      {
         TRACE("IntSetParent Failed Test for set parent to parent!\n");
         EngSetLastError(ERROR_INVALID_PARAMETER);
         return NULL;
      }
      pWndExam = pWndExam->spwndParent;
   }

   /*
    * Windows hides the window first, then shows it again
    * including the WM_SHOWWINDOW messages and all
    */
   WasVisible = co_WinPosShowWindow(Wnd, SW_HIDE);

   /* Window must belong to current process */
   if (Wnd->head.pti->ppi != PsGetCurrentProcessWin32Process())
   {
      ERR("IntSetParent Window must belong to current process!\n");
      return NULL;
   }

   WndOldParent = Wnd->spwndParent;

   if ( WndOldParent &&
        WndOldParent->ExStyle & WS_EX_LAYOUTRTL)
      pt.x = Wnd->rcWindow.right;
   else
      pt.x = Wnd->rcWindow.left;
   pt.y = Wnd->rcWindow.top;

   IntScreenToClient(WndOldParent, &pt);

   if (WndOldParent) UserReferenceObject(WndOldParent); /* Caller must deref */

   /* Even if WndNewParent == WndOldParent continue because the
    * child window (Wnd) should be moved to the top of the z-order */

   /* Unlink the window from the siblings list */
   IntUnlinkWindow(Wnd);
   Wnd->ExStyle2 &= ~WS_EX2_LINKED;

   /* Set the new parent */
   WndSetParent(Wnd, WndNewParent);

   if (Wnd->style & WS_CHILD &&
       Wnd->spwndOwner &&
       Wnd->spwndOwner->ExStyle & WS_EX_TOPMOST)
   {
      ERR("SetParent Top Most from Pop up\n");
      Wnd->ExStyle |= WS_EX_TOPMOST;
   }

   /* Link the window with its new siblings */
   IntLinkHwnd(Wnd,
               ((0 == (Wnd->ExStyle & WS_EX_TOPMOST) &&
               UserIsDesktopWindow(WndNewParent)) ? HWND_TOP : HWND_TOPMOST));

   if ( WndNewParent == co_GetDesktopWindow(Wnd) &&
       !(Wnd->style & WS_CLIPSIBLINGS) )
   {
      Wnd->style |= WS_CLIPSIBLINGS;
      DceResetActiveDCEs(Wnd);
   }

   /* if parent belongs to a different thread and the window isn't */
   /* top-level, attach the two threads */
   if ((Wnd->style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
   {
      if ( Wnd->spwndParent != co_GetDesktopWindow(Wnd))
      {
         if (WndOldParent && (Wnd->head.pti != WndOldParent->head.pti))
         {
            //ERR("SetParent Old out.\n");
            UserAttachThreadInput(Wnd->head.pti, WndOldParent->head.pti, FALSE);
         }
      }
      if ( WndNewParent != co_GetDesktopWindow(Wnd))
      {
         if (Wnd->head.pti != WndNewParent->head.pti)
         {
            //ERR("SetParent New in.\n");
            UserAttachThreadInput(Wnd->head.pti, WndNewParent->head.pti, TRUE);
         }
      }
   }

   if (UserIsMessageWindow(WndOldParent) || UserIsMessageWindow(WndNewParent))
      swFlags |= SWP_NOACTIVATE;

   IntNotifyWinEvent(EVENT_OBJECT_PARENTCHANGE, Wnd ,OBJID_WINDOW, CHILDID_SELF, WEF_SETBYWNDPTI);
   /*
    * SetParent additionally needs to make hwnd the top window
    * in the z-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   //ERR("IntSetParent SetWindowPos 1\n");
   co_WinPosSetWindowPos( Wnd,
                         (0 == (Wnd->ExStyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
                          pt.x, pt.y, 0, 0, swFlags);
   //ERR("IntSetParent SetWindowPos 2 X %d Y %d\n",pt.x, pt.y);
   if (WasVisible) co_WinPosShowWindow(Wnd, SW_SHOWNORMAL);

   return WndOldParent;
}

// Win: xxxSetParent
HWND FASTCALL
co_UserSetParent(HWND hWndChild, HWND hWndNewParent)
{
   PWND Wnd = NULL, WndParent = NULL, WndOldParent;
   HWND hWndOldParent = NULL;
   USER_REFERENCE_ENTRY Ref, ParentRef;

   if (IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   if (hWndChild == IntGetDesktopWindow())
   {
      ERR("UserSetParent Access Denied!\n");
      EngSetLastError(ERROR_ACCESS_DENIED);
      return NULL;
   }

   if (hWndNewParent)
   {
      if (!(WndParent = UserGetWindowObject(hWndNewParent)))
      {
         ERR("UserSetParent Bad New Parent!\n");
         return NULL;
      }
   }
   else
   {
      if (!(WndParent = UserGetWindowObject(IntGetDesktopWindow())))
      {
         return NULL;
      }
   }

   if (!(Wnd = UserGetWindowObject(hWndChild)))
   {
      ERR("UserSetParent Bad Child!\n");
      return NULL;
   }

   UserRefObjectCo(Wnd, &Ref);
   UserRefObjectCo(WndParent, &ParentRef);
   //ERR("Enter co_IntSetParent\n");
   WndOldParent = co_IntSetParent(Wnd, WndParent);
   //ERR("Leave co_IntSetParent\n");
   UserDerefObjectCo(WndParent);
   UserDerefObjectCo(Wnd);

   if (WndOldParent)
   {
      hWndOldParent = UserHMGetHandle(WndOldParent);
      UserDereferenceObject(WndOldParent);
   }

   return hWndOldParent;
}

/* Unlink the window from siblings. Children and parent are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWND Wnd)
{
    ASSERT(Wnd != Wnd->spwndNext);
    ASSERT(Wnd != Wnd->spwndPrev);

    if (Wnd->spwndNext)
        WndSetPrev(Wnd->spwndNext, Wnd->spwndPrev);

    if (Wnd->spwndPrev)
        WndSetNext(Wnd->spwndPrev, Wnd->spwndNext);

    if (Wnd->spwndParent && Wnd->spwndParent->spwndChild == Wnd)
        WndSetChild(Wnd->spwndParent, Wnd->spwndNext);

    WndSetPrev(Wnd, NULL);
    WndSetNext(Wnd, NULL);
}

// Win: ExpandWindowList
BOOL FASTCALL IntGrowHwndList(PWINDOWLIST *ppwl)
{
    PWINDOWLIST pwlOld, pwlNew;
    SIZE_T ibOld, ibNew;

#define GROW_COUNT 8
    pwlOld = *ppwl;
    ibOld = (LPBYTE)pwlOld->phwndLast - (LPBYTE)pwlOld;
    ibNew = ibOld + GROW_COUNT * sizeof(HWND);
#undef GROW_COUNT
    pwlNew = IntReAllocatePoolWithTag(PagedPool, pwlOld, ibOld, ibNew, USERTAG_WINDOWLIST);
    if (!pwlNew)
        return FALSE;

    pwlNew->phwndLast = (HWND *)((LPBYTE)pwlNew + ibOld);
    pwlNew->phwndEnd = (HWND *)((LPBYTE)pwlNew + ibNew);
    *ppwl = pwlNew;
    return TRUE;
}

// Win: InternalBuildHwndList
PWINDOWLIST FASTCALL IntPopulateHwndList(PWINDOWLIST pwl, PWND pwnd, DWORD dwFlags)
{
    ASSERT(!WL_IS_BAD(pwl));

    for (; pwnd; pwnd = pwnd->spwndNext)
    {
        if (!pwl->pti || pwl->pti == pwnd->head.pti)
        {
            *(pwl->phwndLast) = UserHMGetHandle(pwnd);
            ++(pwl->phwndLast);

            if (pwl->phwndLast == pwl->phwndEnd && !IntGrowHwndList(&pwl))
                break;
        }

        if ((dwFlags & IACE_CHILDREN) && pwnd->spwndChild)
        {
            pwl = IntPopulateHwndList(pwl, pwnd->spwndChild, IACE_CHILDREN | IACE_LIST);
            if (WL_IS_BAD(pwl))
                break;
        }

        if (!(dwFlags & IACE_LIST))
            break;
    }

    return pwl;
}

// Win: BuildHwndList
PWINDOWLIST FASTCALL IntBuildHwndList(PWND pwnd, DWORD dwFlags, PTHREADINFO pti)
{
    PWINDOWLIST pwl;
    DWORD cbWL;

    if (gpwlCache)
    {
        pwl = gpwlCache;
        gpwlCache = NULL;
    }
    else
    {
#define INITIAL_COUNT 32
        cbWL = sizeof(WINDOWLIST) + (INITIAL_COUNT - 1) * sizeof(HWND);
        pwl = ExAllocatePoolWithTag(PagedPool, cbWL, USERTAG_WINDOWLIST);
        if (!pwl)
            return NULL;

        pwl->phwndEnd = &pwl->ahwnd[INITIAL_COUNT];
#undef INITIAL_COUNT
    }

    pwl->pti = pti;
    pwl->phwndLast = pwl->ahwnd;
    pwl = IntPopulateHwndList(pwl, pwnd, dwFlags);
    if (WL_IS_BAD(pwl))
    {
        ExFreePoolWithTag(pwl, USERTAG_WINDOWLIST);
        return NULL;
    }

    *(pwl->phwndLast) = HWND_TERMINATOR;

    if (dwFlags & 0x8)
    {
        // TODO:
    }

    pwl->pti = GetW32ThreadInfo();
    pwl->pNextList = gpwlList;
    gpwlList = pwl;

    return pwl;
}

// Win: FreeHwndList
VOID FASTCALL IntFreeHwndList(PWINDOWLIST pwlTarget)
{
    PWINDOWLIST pwl, *ppwl;

    for (ppwl = &gpwlList; *ppwl; ppwl = &(*ppwl)->pNextList)
    {
        if (*ppwl != pwlTarget)
            continue;

        *ppwl = pwlTarget->pNextList;

        if (gpwlCache)
        {
            if (WL_CAPACITY(pwlTarget) > WL_CAPACITY(gpwlCache))
            {
                pwl = gpwlCache;
                gpwlCache = pwlTarget;
                ExFreePoolWithTag(pwl, USERTAG_WINDOWLIST);
            }
            else
            {
                ExFreePoolWithTag(pwlTarget, USERTAG_WINDOWLIST);
            }
        }
        else
        {
            gpwlCache = pwlTarget;
        }

        break;
    }
}

/* FUNCTIONS *****************************************************************/

/*
 * As best as I can figure, this function is used by EnumWindows,
 * EnumChildWindows, EnumDesktopWindows, & EnumThreadWindows.
 *
 * It's supposed to build a list of HWNDs to return to the caller.
 * We can figure out what kind of list by what parameters are
 * passed to us.
 */
/*
 * @implemented
 */
NTSTATUS
NTAPI
NtUserBuildHwndList(
   HDESK hDesktop,
   HWND hwndParent,
   BOOLEAN bChildren,
   ULONG dwThreadId,
   ULONG cHwnd,
   HWND* phwndList,
   ULONG* pcHwndNeeded)
{
   NTSTATUS Status;
   ULONG dwCount = 0;

   if (pcHwndNeeded == NULL)
       return STATUS_INVALID_PARAMETER;

   UserEnterExclusive();

   if (hwndParent || !dwThreadId)
   {
      PDESKTOP Desktop;
      PWND Parent, Window;

      if(!hwndParent)
      {
         if(hDesktop == NULL && !(Desktop = IntGetActiveDesktop()))
         {
            Status = STATUS_INVALID_HANDLE;
            goto Quit;
         }

         if(hDesktop)
         {
            Status = IntValidateDesktopHandle(hDesktop,
                                              UserMode,
                                              0,
                                              &Desktop);
            if(!NT_SUCCESS(Status))
            {
                Status = STATUS_INVALID_HANDLE;
                goto Quit;
            }
         }
         hwndParent = Desktop->DesktopWindow;
      }
      else
      {
         hDesktop = 0;
      }

      if((Parent = UserGetWindowObject(hwndParent)) &&
         (Window = Parent->spwndChild))
      {
         BOOL bGoDown = TRUE;

         Status = STATUS_SUCCESS;
         while(TRUE)
         {
            if (bGoDown)
            {
               if (dwCount++ < cHwnd && phwndList)
               {
                  _SEH2_TRY
                  {
                     ProbeForWrite(phwndList, sizeof(HWND), 1);
                     *phwndList = UserHMGetHandle(Window);
                     phwndList++;
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                     Status = _SEH2_GetExceptionCode();
                  }
                  _SEH2_END
                  if(!NT_SUCCESS(Status))
                  {
                     break;
                  }
               }
               if (Window->spwndChild && bChildren)
               {
                  Window = Window->spwndChild;
                  continue;
               }
               bGoDown = FALSE;
            }
            if (Window->spwndNext)
            {
               Window = Window->spwndNext;
               bGoDown = TRUE;
               continue;
            }
            Window = Window->spwndParent;
            if (Window == Parent)
            {
               break;
            }
         }
      }

      if(hDesktop)
      {
         ObDereferenceObject(Desktop);
      }
   }
   else // Build EnumThreadWindows list!
   {
      PETHREAD Thread;
      PTHREADINFO W32Thread;
      PWND Window;
      HWND *List = NULL;

      Status = PsLookupThreadByThreadId(UlongToHandle(dwThreadId), &Thread);
      if (!NT_SUCCESS(Status))
      {
         ERR("Thread Id is not valid!\n");
         Status = STATUS_INVALID_PARAMETER;
         goto Quit;
      }
      if (!(W32Thread = (PTHREADINFO)Thread->Tcb.Win32Thread))
      {
         ObDereferenceObject(Thread);
         TRACE("Tried to enumerate windows of a non gui thread\n");
         Status = STATUS_INVALID_PARAMETER;
         goto Quit;
      }

     // Do not use Thread link list due to co_UserFreeWindow!!!
     // Current = W32Thread->WindowListHead.Flink;
     // Fixes Api:CreateWindowEx tests!!!
      List = IntWinListChildren(UserGetDesktopWindow());
      if (List)
      {
         int i;
         for (i = 0; List[i]; i++)
         {
            Window = ValidateHwndNoErr(List[i]);
            if (Window && Window->head.pti == W32Thread)
            {
               if (dwCount < cHwnd && phwndList)
               {
                  _SEH2_TRY
                  {
                     ProbeForWrite(phwndList, sizeof(HWND), 1);
                     *phwndList = UserHMGetHandle(Window);
                     phwndList++;
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                     Status = _SEH2_GetExceptionCode();
                  }
                  _SEH2_END
                  if (!NT_SUCCESS(Status))
                  {
                     ERR("Failure to build window list!\n");
                     break;
                  }
               }
               dwCount++;
            }
         }
         ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
      }

      ObDereferenceObject(Thread);
   }

   *pcHwndNeeded = dwCount;
   Status = STATUS_SUCCESS;

Quit:
   SetLastNtError(Status);
   UserLeave();
   return Status;
}

static void IntSendParentNotify( PWND pWindow, UINT msg )
{
    if ( (pWindow->style & (WS_CHILD | WS_POPUP)) == WS_CHILD &&
         !(pWindow->ExStyle & WS_EX_NOPARENTNOTIFY))
    {
        if (VerifyWnd(pWindow->spwndParent) && !UserIsDesktopWindow(pWindow->spwndParent))
        {
            USER_REFERENCE_ENTRY Ref;
            UserRefObjectCo(pWindow->spwndParent, &Ref);
            co_IntSendMessage( UserHMGetHandle(pWindow->spwndParent),
                               WM_PARENTNOTIFY,
                               MAKEWPARAM( msg, pWindow->IDMenu),
                               (LPARAM)UserHMGetHandle(pWindow) );
            UserDerefObjectCo(pWindow->spwndParent);
        }
    }
}

void FASTCALL
IntFixWindowCoordinates(CREATESTRUCTW* Cs, PWND ParentWindow, DWORD* dwShowMode)
{
#define IS_DEFAULT(x)  ((x) == CW_USEDEFAULT || (x) == (SHORT)0x8000)

   /* default positioning for overlapped windows */
    if(!(Cs->style & (WS_POPUP | WS_CHILD)))
   {
      PMONITOR pMonitor;
      PRTL_USER_PROCESS_PARAMETERS ProcessParams;

      pMonitor = UserGetPrimaryMonitor();

      /* Check if we don't have a monitor attached yet */
      if(pMonitor == NULL)
      {
          Cs->x = Cs->y = 0;
          Cs->cx = 800;
          Cs->cy = 600;
          return;
      }

      ProcessParams = PsGetCurrentProcess()->Peb->ProcessParameters;

      if (IS_DEFAULT(Cs->x))
      {
          if (!IS_DEFAULT(Cs->y)) *dwShowMode = Cs->y;

          if(ProcessParams->WindowFlags & STARTF_USEPOSITION)
          {
              Cs->x = ProcessParams->StartingX;
              Cs->y = ProcessParams->StartingY;
          }
          else
          {
               Cs->x = pMonitor->cWndStack * (UserGetSystemMetrics(SM_CXSIZE) + UserGetSystemMetrics(SM_CXFRAME));
               Cs->y = pMonitor->cWndStack * (UserGetSystemMetrics(SM_CYSIZE) + UserGetSystemMetrics(SM_CYFRAME));
               if (Cs->x > ((pMonitor->rcWork.right - pMonitor->rcWork.left) / 4) ||
                   Cs->y > ((pMonitor->rcWork.bottom - pMonitor->rcWork.top) / 4))
               {
                  /* reset counter and position */
                  Cs->x = 0;
                  Cs->y = 0;
                  pMonitor->cWndStack = 0;
               }
               pMonitor->cWndStack++;
          }
      }

      if (IS_DEFAULT(Cs->cx))
      {
          if (ProcessParams->WindowFlags & STARTF_USEPOSITION)
          {
              Cs->cx = ProcessParams->CountX;
              Cs->cy = ProcessParams->CountY;
          }
          else
          {
              Cs->cx = (pMonitor->rcWork.right - pMonitor->rcWork.left) * 3 / 4;
              Cs->cy = (pMonitor->rcWork.bottom - pMonitor->rcWork.top) * 3 / 4;
          }
      }
      /* neither x nor cx are default. Check the y values .
       * In the trace we see Outlook and Outlook Express using
       * cy set to CW_USEDEFAULT when opening the address book.
       */
      else if (IS_DEFAULT(Cs->cy))
      {
          TRACE("Strange use of CW_USEDEFAULT in nHeight\n");
          Cs->cy = (pMonitor->rcWork.bottom - pMonitor->rcWork.top) * 3 / 4;
      }
   }
   else
   {
      /* if CW_USEDEFAULT is set for non-overlapped windows, both values are set to zero */
      if(IS_DEFAULT(Cs->x))
      {
         Cs->x = 0;
         Cs->y = 0;
      }
      if(IS_DEFAULT(Cs->cx))
      {
         Cs->cx = 0;
         Cs->cy = 0;
      }
   }

#undef IS_DEFAULT
}

/* Allocates and initializes a window */
PWND FASTCALL IntCreateWindow(CREATESTRUCTW* Cs,
                              PLARGE_STRING WindowName,
                              PCLS Class,
                              PWND ParentWindow,
                              PWND OwnerWindow,
                              PVOID acbiBuffer,
                              PDESKTOP pdeskCreated,
                              DWORD dwVer )
{
   PWND pWnd = NULL;
   HWND hWnd;
   PTHREADINFO pti;
   BOOL MenuChanged;
   BOOL bUnicodeWindow;
   PCALLPROCDATA pcpd;

   pti = pdeskCreated ? gptiDesktopThread : GetW32ThreadInfo();

   if (!(Cs->dwExStyle & WS_EX_LAYOUTRTL))
   {      // Need both here for wine win.c test_CreateWindow.
      //if (Cs->hwndParent && ParentWindow)
      if (ParentWindow) // It breaks more tests..... WIP.
      {
         if ( (Cs->style & (WS_CHILD|WS_POPUP)) == WS_CHILD &&
              ParentWindow->ExStyle & WS_EX_LAYOUTRTL &&
             !(ParentWindow->ExStyle & WS_EX_NOINHERITLAYOUT) )
            Cs->dwExStyle |= WS_EX_LAYOUTRTL;
      }
      else
      { /*
         * Note from MSDN <http://msdn.microsoft.com/en-us/library/aa913269.aspx>:
         *
         * Dialog boxes and message boxes do not inherit layout, so you must
         * set the layout explicitly.
         */
         if ( Class->fnid != FNID_DIALOG )
         {
            if (pti->ppi->dwLayout & LAYOUT_RTL)
            {
               Cs->dwExStyle |= WS_EX_LAYOUTRTL;
            }
         }
      }
   }

   /* Automatically add WS_EX_WINDOWEDGE */
   if ((Cs->dwExStyle & WS_EX_DLGMODALFRAME) ||
         ((!(Cs->dwExStyle & WS_EX_STATICEDGE)) &&
         (Cs->style & (WS_DLGFRAME | WS_THICKFRAME))))
      Cs->dwExStyle |= WS_EX_WINDOWEDGE;
   else
      Cs->dwExStyle &= ~WS_EX_WINDOWEDGE;

   /* Is it a unicode window? */
   bUnicodeWindow =!(Cs->dwExStyle & WS_EX_SETANSICREATOR);
   Cs->dwExStyle &= ~WS_EX_SETANSICREATOR;

   /* Allocate the new window */
   pWnd = (PWND) UserCreateObject( gHandleTable,
                                   pdeskCreated ? pdeskCreated : pti->rpdesk,
                                   pti,
                                  (PHANDLE)&hWnd,
                                   TYPE_WINDOW,
                                   sizeof(WND) + Class->cbwndExtra);

   if (!pWnd)
   {
      goto AllocError;
   }

   TRACE("Created window object with handle %p\n", hWnd);

   if (pdeskCreated && pdeskCreated->DesktopWindow == NULL )
   {  /* HACK: Helper for win32csr/desktopbg.c */
      /* If there is no desktop window yet, we must be creating it */
      TRACE("CreateWindow setting desktop.\n");
      pdeskCreated->DesktopWindow = hWnd;
      pdeskCreated->pDeskInfo->spwnd = pWnd;
   }

   /*
    * Fill out the structure describing it.
    */
   /* Remember, pWnd->head is setup in object.c ... */
   WndSetParent(pWnd, ParentWindow);
   WndSetOwner(pWnd, OwnerWindow);
   pWnd->fnid = 0;
   WndSetLastActive(pWnd, pWnd);
   // Ramp up compatible version sets.
   if ( dwVer >= WINVER_WIN31 )
   {
       pWnd->state2 |= WNDS2_WIN31COMPAT;
       if ( dwVer >= WINVER_WINNT4 )
       {
           pWnd->state2 |= WNDS2_WIN40COMPAT;
           if ( dwVer >= WINVER_WIN2K )
           {
               pWnd->state2 |= WNDS2_WIN50COMPAT;
           }
       }
   }
   pWnd->pcls = Class;
   pWnd->hModule = Cs->hInstance;
   pWnd->style = Cs->style & ~WS_VISIBLE;
   pWnd->ExStyle = Cs->dwExStyle;
   pWnd->cbwndExtra = pWnd->pcls->cbwndExtra;
   pWnd->pActCtx = acbiBuffer;

   if (pti->spDefaultImc && Class->atomClassName != gpsi->atomSysClass[ICLS_BUTTON])
      pWnd->hImc = UserHMGetHandle(pti->spDefaultImc);

   pWnd->InternalPos.MaxPos.x  = pWnd->InternalPos.MaxPos.y  = -1;
   pWnd->InternalPos.IconPos.x = pWnd->InternalPos.IconPos.y = -1;

   if (pWnd->spwndParent != NULL && Cs->hwndParent != 0)
   {
       pWnd->HideFocus = pWnd->spwndParent->HideFocus;
       pWnd->HideAccel = pWnd->spwndParent->HideAccel;
   }

   InitializeListHead(&pWnd->ThreadListEntry);
   pWnd->head.pti->cWindows++;

   if (Class->spicn && !Class->spicnSm)
   {
       HICON IconSmHandle = NULL;
       if((Class->spicn->CURSORF_flags & (CURSORF_LRSHARED | CURSORF_FROMRESOURCE))
               == (CURSORF_LRSHARED | CURSORF_FROMRESOURCE))
       {
           IconSmHandle = co_IntCopyImage(
               UserHMGetHandle(Class->spicn),
               IMAGE_ICON,
               UserGetSystemMetrics( SM_CXSMICON ),
               UserGetSystemMetrics( SM_CYSMICON ),
               LR_COPYFROMRESOURCE);
       }
       if (!IconSmHandle)
       {
           /* Retry without copying from resource */
           IconSmHandle = co_IntCopyImage(
               UserHMGetHandle(Class->spicn),
               IMAGE_ICON,
               UserGetSystemMetrics( SM_CXSMICON ),
               UserGetSystemMetrics( SM_CYSMICON ),
               0);
       }

       if (IconSmHandle)
       {
           Class->spicnSm = UserGetCurIconObject(IconSmHandle);
           Class->CSF_flags |= CSF_CACHEDSMICON;
       }
   }

   if (pWnd->pcls->CSF_flags & CSF_SERVERSIDEPROC)
      pWnd->state |= WNDS_SERVERSIDEWINDOWPROC;

 /* BugBoy Comments: Comment below say that System classes are always created
    as UNICODE. In windows, creating a window with the ANSI version of CreateWindow
    sets the window to ansi as verified by testing with IsUnicodeWindow API.

    No where can I see in code or through testing does the window change back
    to ANSI after being created as UNICODE in ROS. I didnt do more testing to
    see what problems this would cause. */

   // Set WndProc from Class.
   if (IsCallProcHandle(pWnd->pcls->lpfnWndProc))
   {
      pcpd = UserGetObject(gHandleTable, pWnd->pcls->lpfnWndProc, TYPE_CALLPROC);
      if (pcpd)
         pWnd->lpfnWndProc = pcpd->pfnClientPrevious;
   }
   else
   {
      pWnd->lpfnWndProc = pWnd->pcls->lpfnWndProc;
   }

   // GetWindowProc, test for non server side default classes and set WndProc.
    if ( pWnd->pcls->fnid <= FNID_GHOST && pWnd->pcls->fnid >= FNID_BUTTON )
    {
      if (bUnicodeWindow)
      {
         if (GETPFNCLIENTA(pWnd->pcls->fnid) == pWnd->lpfnWndProc)
            pWnd->lpfnWndProc = GETPFNCLIENTW(pWnd->pcls->fnid);
      }
      else
      {
         if (GETPFNCLIENTW(pWnd->pcls->fnid) == pWnd->lpfnWndProc)
            pWnd->lpfnWndProc = GETPFNCLIENTA(pWnd->pcls->fnid);
      }
    }

   // If not an Unicode caller, set Ansi creator bit.
   if (!bUnicodeWindow) pWnd->state |= WNDS_ANSICREATOR;

   // Clone Class Ansi/Unicode proc type.
   if (pWnd->pcls->CSF_flags & CSF_ANSIPROC)
   {
      pWnd->state |= WNDS_ANSIWINDOWPROC;
      pWnd->Unicode = FALSE;
   }
   else
   { /*
      * It seems there can be both an Ansi creator and Unicode Class Window
      * WndProc, unless the following overriding conditions occur:
      */
      if ( !bUnicodeWindow &&
          ( Class->atomClassName == gpsi->atomSysClass[ICLS_BUTTON]    ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_COMBOBOX]  ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_COMBOLBOX] ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_DIALOG]    ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_EDIT]      ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_IME]       ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_LISTBOX]   ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_MDICLIENT] ||
            Class->atomClassName == gpsi->atomSysClass[ICLS_STATIC] ) )
      { // Override Class and set the window Ansi WndProc.
         pWnd->state |= WNDS_ANSIWINDOWPROC;
         pWnd->Unicode = FALSE;
      }
      else
      { // Set the window Unicode WndProc.
         pWnd->state &= ~WNDS_ANSIWINDOWPROC;
         pWnd->Unicode = TRUE;
      }
   }

   /* BugBoy Comments: if the window being created is a edit control, ATOM 0xCxxx,
      then my testing shows that windows (2k and XP) creates a CallProc for it immediately
      Dont understand why it does this. */
   if (Class->atomClassName == gpsi->atomSysClass[ICLS_EDIT])
   {
      PCALLPROCDATA CallProc;
      CallProc = CreateCallProc(pWnd->head.rpdesk, pWnd->lpfnWndProc, pWnd->Unicode , pWnd->head.pti->ppi);

      if (!CallProc)
      {
         EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
         ERR("Warning: Unable to create CallProc for edit control. Control may not operate correctly! hwnd %p\n", hWnd);
      }
      else
      {
         UserAddCallProcToClass(pWnd->pcls, CallProc);
      }
   }

   InitializeListHead(&pWnd->PropListHead);
   pWnd->PropListItems = 0;

   if ( WindowName->Buffer != NULL && WindowName->Length > 0 )
   {
      pWnd->strName.Buffer = DesktopHeapAlloc(pWnd->head.rpdesk,
                                             WindowName->Length + sizeof(UNICODE_NULL));
      if (pWnd->strName.Buffer == NULL)
      {
          goto AllocError;
      }

      RtlCopyMemory(pWnd->strName.Buffer, WindowName->Buffer, WindowName->Length);
      pWnd->strName.Buffer[WindowName->Length / sizeof(WCHAR)] = L'\0';
      pWnd->strName.Length = WindowName->Length;
      pWnd->strName.MaximumLength = WindowName->Length + sizeof(UNICODE_NULL);
   }

   /* Correct the window style. */
   if ((pWnd->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
   {
      pWnd->style |= WS_CLIPSIBLINGS;
      if (!(pWnd->style & WS_POPUP))
      {
         pWnd->style |= WS_CAPTION;
      }
   }

   /* WS_EX_WINDOWEDGE depends on some other styles */
   if (pWnd->ExStyle & WS_EX_DLGMODALFRAME)
       pWnd->ExStyle |= WS_EX_WINDOWEDGE;
   else if (pWnd->style & (WS_DLGFRAME | WS_THICKFRAME))
   {
       if (!((pWnd->ExStyle & WS_EX_STATICEDGE) &&
            (pWnd->style & (WS_CHILD | WS_POPUP))))
           pWnd->ExStyle |= WS_EX_WINDOWEDGE;
   }
    else
        pWnd->ExStyle &= ~WS_EX_WINDOWEDGE;

   if (!(pWnd->style & (WS_CHILD | WS_POPUP)))
      pWnd->state |= WNDS_SENDSIZEMOVEMSGS;

   /* Set the window menu */
   if ((Cs->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
   {
      if (Cs->hMenu)
      {
         IntSetMenu(pWnd, Cs->hMenu, &MenuChanged);
      }
      else if (pWnd->pcls->lpszMenuName) // Take it from the parent.
      {
          UNICODE_STRING MenuName;
          HMENU hMenu;

          if (IS_INTRESOURCE(pWnd->pcls->lpszMenuName))
          {
             MenuName.Length = 0;
             MenuName.MaximumLength = 0;
             MenuName.Buffer = pWnd->pcls->lpszMenuName;
          }
          else
          {
             RtlInitUnicodeString( &MenuName, pWnd->pcls->lpszMenuName);
          }
          hMenu = co_IntCallLoadMenu( pWnd->pcls->hModule, &MenuName);
          if (hMenu) IntSetMenu(pWnd, hMenu, &MenuChanged);
      }
   }
   else // Not a child
      pWnd->IDMenu = (UINT_PTR)Cs->hMenu;


   if ( ParentWindow &&
        ParentWindow != ParentWindow->head.rpdesk->spwndMessage &&
        ParentWindow != ParentWindow->head.rpdesk->pDeskInfo->spwnd )
   {
       PWND Owner = IntGetNonChildAncestor(ParentWindow);

       if (!IntValidateOwnerDepth(pWnd, Owner))
       {
          EngSetLastError(ERROR_INVALID_PARAMETER);
          goto Error;
       }
       if ( pWnd->spwndOwner &&
            pWnd->spwndOwner->ExStyle & WS_EX_TOPMOST )
       {
          pWnd->ExStyle |= WS_EX_TOPMOST;
       }
       if ( pWnd->spwndOwner &&
            Class->atomClassName != gpsi->atomSysClass[ICLS_IME] &&
            pti != pWnd->spwndOwner->head.pti)
       {
          //ERR("CreateWindow Owner in.\n");
          UserAttachThreadInput(pti, pWnd->spwndOwner->head.pti, TRUE);
       }
   }

   /* Insert the window into the thread's window list. */
   InsertTailList (&pti->WindowListHead, &pWnd->ThreadListEntry);

   /* Handle "CS_CLASSDC", it is tested first. */
   if ( (pWnd->pcls->style & CS_CLASSDC) && !(pWnd->pcls->pdce) )
   {  /* One DCE per class to have CLASS. */
      pWnd->pcls->pdce = DceAllocDCE( pWnd, DCE_CLASS_DC );
   }
   else if ( pWnd->pcls->style & CS_OWNDC)
   {  /* Allocate a DCE for this window. */
      DceAllocDCE(pWnd, DCE_WINDOW_DC);
   }

   return pWnd;

AllocError:
   ERR("IntCreateWindow Allocation Error.\n");
   SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
Error:
   if(pWnd)
      UserDereferenceObject(pWnd);
   return NULL;
}

/*
 * @implemented
 */
PWND FASTCALL
co_UserCreateWindowEx(CREATESTRUCTW* Cs,
                     PUNICODE_STRING ClassName,
                     PLARGE_STRING WindowName,
                     PVOID acbiBuffer,
                     DWORD dwVer )
{
   ULONG style;
   PWND Window = NULL, ParentWindow = NULL, OwnerWindow;
   HWND hWnd, hWndParent, hWndOwner, hwndInsertAfter;
   PWINSTATION_OBJECT WinSta;
   PCLS Class = NULL;
   SIZE Size;
   POINT MaxSize, MaxPos, MinTrack, MaxTrack;
   CBT_CREATEWNDW * pCbtCreate;
   LRESULT Result;
   USER_REFERENCE_ENTRY ParentRef, Ref;
   PTHREADINFO pti;
   DWORD dwShowMode = SW_SHOW;
   CREATESTRUCTW *pCsw = NULL;
   PVOID pszClass = NULL, pszName = NULL;
   PWND ret = NULL;

   /* Get the current window station and reference it */
   pti = GetW32ThreadInfo();
   if (pti == NULL || pti->rpdesk == NULL)
   {
      ERR("Thread is not attached to a desktop! Cannot create window (%wZ)\n", ClassName);
      return NULL; // There is nothing to cleanup.
   }
   WinSta = pti->rpdesk->rpwinstaParent;
   ObReferenceObjectByPointer(WinSta, KernelMode, ExWindowStationObjectType, 0);

   pCsw = NULL;
   pCbtCreate = NULL;

   /* Get the class and reference it */
   Class = IntGetAndReferenceClass(ClassName, Cs->hInstance, FALSE);
   if(!Class)
   {
       EngSetLastError(ERROR_CANNOT_FIND_WND_CLASS);
       ERR("Failed to find class %wZ\n", ClassName);
       goto cleanup;
   }

   /* Now find the parent and the owner window */
   hWndParent = UserHMGetHandle(pti->rpdesk->pDeskInfo->spwnd);
   hWndOwner = NULL;

    if (Cs->hwndParent == HWND_MESSAGE)
    {
        Cs->hwndParent = hWndParent = UserHMGetHandle(pti->rpdesk->spwndMessage);
    }
    else if (Cs->hwndParent)
    {
        if ((Cs->style & (WS_CHILD|WS_POPUP)) != WS_CHILD)
            hWndOwner = Cs->hwndParent;
        else
            hWndParent = Cs->hwndParent;
    }
    else if ((Cs->style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
    {
         ERR("Cannot create a child window (%wZ) without a parent\n", ClassName);
         EngSetLastError(ERROR_TLW_WITH_WSCHILD);
         goto cleanup;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
    }
    else if (Cs->lpszClass != (LPCWSTR)MAKEINTATOM(gpsi->atomSysClass[ICLS_DESKTOP]) &&
             (IS_INTRESOURCE(Cs->lpszClass) ||
              Cs->lpszClass != (LPCWSTR)MAKEINTATOM(gpsi->atomSysClass[ICLS_HWNDMESSAGE]) ||
              _wcsicmp(Cs->lpszClass, L"Message") != 0))
    {
        if (pti->ppi->dwLayout & LAYOUT_RTL)
        {
            Cs->dwExStyle |= WS_EX_LAYOUTRTL;
        }
    }

    ParentWindow = hWndParent ? UserGetWindowObject(hWndParent): NULL;
    OwnerWindow = hWndOwner ? UserGetWindowObject(hWndOwner): NULL;

    if (hWndParent && !ParentWindow)
    {
        ERR("Got invalid parent window handle for %wZ\n", ClassName);
        goto cleanup;
    }
    else if (hWndOwner && !OwnerWindow)
    {
        ERR("Got invalid owner window handle for %wZ\n", ClassName);
        ParentWindow = NULL;
        goto cleanup;
    }

    if(OwnerWindow)
    {
       if (IntIsDesktopWindow(OwnerWindow)) OwnerWindow = NULL;
       else if (ParentWindow && !IntIsDesktopWindow(ParentWindow))
       {
          ERR("an owned window must be created as top-level\n");
          EngSetLastError( STATUS_ACCESS_DENIED );
          goto cleanup;
       }
       else /* owner must be a top-level window */
       {
          while ((OwnerWindow->style & (WS_POPUP|WS_CHILD)) == WS_CHILD && !IntIsDesktopWindow(OwnerWindow->spwndParent))
                 OwnerWindow = OwnerWindow->spwndParent;
       }
    }

   /* Fix the position and the size of the window */
   if (ParentWindow)
   {
       UserRefObjectCo(ParentWindow, &ParentRef);
       IntFixWindowCoordinates(Cs, ParentWindow, &dwShowMode);
   }

   /* Allocate and initialize the new window */
   Window = IntCreateWindow(Cs,
                            WindowName,
                            Class,
                            ParentWindow,
                            OwnerWindow,
                            acbiBuffer,
                            NULL,
                            dwVer );
   if(!Window)
   {
       ERR("IntCreateWindow(%wZ) failed\n", ClassName);
       goto cleanup;
   }

   hWnd = UserHMGetHandle(Window);
   hwndInsertAfter = HWND_TOP;

   UserRefObjectCo(Window, &Ref);
   UserDereferenceObject(Window);
   ObDereferenceObject(WinSta);

   /* NCCREATE, WM_NCCALCSIZE and Hooks need the original values */
   Cs->lpszName = (LPCWSTR) WindowName;
   Cs->lpszClass = (LPCWSTR) ClassName;

   //// Check for a hook to eliminate overhead. ////
   if ( ISITHOOKED(WH_CBT) ||  (pti->rpdesk->pDeskInfo->fsHooks & HOOKID_TO_FLAG(WH_CBT)) )
   {
      // Allocate the calling structures Justin Case this goes Global.
      pCsw = ExAllocatePoolWithTag(NonPagedPool, sizeof(CREATESTRUCTW), TAG_HOOK);
      pCbtCreate = ExAllocatePoolWithTag(NonPagedPool, sizeof(CBT_CREATEWNDW), TAG_HOOK);
      if (!pCsw || !pCbtCreate)
      {
      	 ERR("UserHeapAlloc() failed!\n");
      	 goto cleanup;
      }

      if (!IntMsgCreateStructW( Window, pCsw, Cs, &pszClass, &pszName ) )
      {
          ERR("IntMsgCreateStructW() failed!\n");
          goto cleanup;
      }

      pCbtCreate->lpcs = pCsw;
      pCbtCreate->hwndInsertAfter = hwndInsertAfter;

      //// Call the WH_CBT hook ////
      Result = co_HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (WPARAM) hWnd, (LPARAM) pCbtCreate);
      if (Result != 0)
      {
         ERR("WH_CBT HCBT_CREATEWND hook failed! 0x%x\n", Result);
         goto cleanup;
      }
      // Write back changes.
      Cs->cx = pCsw->cx;
      Cs->cy = pCsw->cy;
      Cs->x = pCsw->x;
      Cs->y = pCsw->y;
      hwndInsertAfter = pCbtCreate->hwndInsertAfter;
   }

   if ((Cs->style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
   {
      if (ParentWindow != co_GetDesktopWindow(Window))
      {
         Cs->x += ParentWindow->rcClient.left;
         Cs->y += ParentWindow->rcClient.top;
      }
   }

   /* Send the WM_GETMINMAXINFO message */
   Size.cx = Cs->cx;
   Size.cy = Cs->cy;

   if ((Cs->style & WS_THICKFRAME) || !(Cs->style & (WS_POPUP | WS_CHILD)))
   {
      co_WinPosGetMinMaxInfo(Window, &MaxSize, &MaxPos, &MinTrack, &MaxTrack);
      if (Size.cx > MaxTrack.x) Size.cx = MaxTrack.x;
      if (Size.cy > MaxTrack.y) Size.cy = MaxTrack.y;
      if (Size.cx < MinTrack.x) Size.cx = MinTrack.x;
      if (Size.cy < MinTrack.y) Size.cy = MinTrack.y;
   }

   Window->rcWindow.left = Cs->x;
   Window->rcWindow.top = Cs->y;
   Window->rcWindow.right = Cs->x + Size.cx;
   Window->rcWindow.bottom = Cs->y + Size.cy;
 /*
   if (0 != (Window->style & WS_CHILD) && ParentWindow)
   {
      ERR("co_UserCreateWindowEx(): Offset rcWindow\n");
      RECTL_vOffsetRect(&Window->rcWindow,
                        ParentWindow->rcClient.left,
                        ParentWindow->rcClient.top);
   }
 */
   /* correct child window coordinates if mirroring on parent is enabled */
   if (ParentWindow != NULL)
   {
      if ( ((Cs->style & WS_CHILD) == WS_CHILD) &&
          ((ParentWindow->ExStyle & WS_EX_LAYOUTRTL) ==  WS_EX_LAYOUTRTL))
      {
          Window->rcWindow.right = ParentWindow->rcClient.right - (Window->rcWindow.left - ParentWindow->rcClient.left);
          Window->rcWindow.left = Window->rcWindow.right - Size.cx;
      }
   }

   Window->rcClient = Window->rcWindow;

   if (Window->spwndNext || Window->spwndPrev)
   {
      ERR("Window 0x%p has been linked too early!\n", Window);
   }

   if (!(Window->state2 & WNDS2_WIN31COMPAT))
   {
      if (Class->style & CS_PARENTDC && !(ParentWindow->style & WS_CLIPCHILDREN))
         Window->style &= ~(WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
   }

   if ((Window->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      if ( !IntIsTopLevelWindow(Window) )
      {
         if (pti != ParentWindow->head.pti)
         {
            //ERR("CreateWindow Parent in.\n");
            UserAttachThreadInput(pti, ParentWindow->head.pti, TRUE);
         }
      }
   }

   /* Send the NCCREATE message */
   Result = co_IntSendMessage(UserHMGetHandle(Window), WM_NCCREATE, 0, (LPARAM) Cs);
   if (!Result)
   {
      ERR("co_UserCreateWindowEx(%wZ): NCCREATE message failed\n", ClassName);
      goto cleanup;
   }

   /* Link the window */
   if (ParentWindow != NULL)
   {
      /* Link the window into the siblings list */
      if ((Cs->style & (WS_CHILD | WS_MAXIMIZE)) == WS_CHILD)
          IntLinkHwnd(Window, HWND_BOTTOM);
      else
          IntLinkHwnd(Window, hwndInsertAfter);
   }

   /* Create the IME window for pWnd */
   if (IS_IMM_MODE() && !pti->spwndDefaultIme && IntWantImeWindow(Window))
   {
      PWND pwndDefaultIme = co_IntCreateDefaultImeWindow(Window, Window->hModule);
      UserAssignmentLock((PVOID*)&pti->spwndDefaultIme, pwndDefaultIme);

      if (pwndDefaultIme)
      {
         HWND hImeWnd;
         USER_REFERENCE_ENTRY Ref;
         UserRefObjectCo(pwndDefaultIme, &Ref);

         hImeWnd = UserHMGetHandle(pwndDefaultIme);

         co_IntSendMessage(hImeWnd, WM_IME_SYSTEM, IMS_LOADTHREADLAYOUT, 0);

         if (pti->pClientInfo->CI_flags & CI_IMMACTIVATE)
         {
            HKL hKL = pti->KeyboardLayout->hkl;
            co_IntSendMessage(hImeWnd, WM_IME_SYSTEM, IMS_ACTIVATELAYOUT, (LPARAM)hKL);
            pti->pClientInfo->CI_flags &= ~CI_IMMACTIVATE;
         }

         UserDerefObjectCo(pwndDefaultIme);
      }
   }

   /* Send the WM_NCCALCSIZE message */
   {
  // RECT rc;
   MaxPos.x = Window->rcWindow.left;
   MaxPos.y = Window->rcWindow.top;

   Result = co_WinPosGetNonClientSize(Window, &Window->rcWindow, &Window->rcClient);
   //rc = Window->rcWindow;
   //Result = co_IntSendMessageNoWait(UserHMGetHandle(Window), WM_NCCALCSIZE, FALSE, (LPARAM)&rc);
   //Window->rcClient = rc;

   RECTL_vOffsetRect(&Window->rcWindow, MaxPos.x - Window->rcWindow.left,
                                     MaxPos.y - Window->rcWindow.top);
   }

   /* Send the WM_CREATE message. */
   Result = co_IntSendMessage(UserHMGetHandle(Window), WM_CREATE, 0, (LPARAM) Cs);
   if (Result == (LRESULT)-1)
   {
      ERR("co_UserCreateWindowEx(%wZ): WM_CREATE message failed\n", ClassName);
      goto cleanup;
   }

   /* Send the EVENT_OBJECT_CREATE event */
   IntNotifyWinEvent(EVENT_OBJECT_CREATE, Window, OBJID_WINDOW, CHILDID_SELF, 0);

   /* By setting the flag below it can be examined to determine if the window
      was created successfully and a valid pwnd was passed back to caller since
      from here the function has to succeed. */
   Window->state2 |= WNDS2_WMCREATEMSGPROCESSED;

   /* Send the WM_SIZE and WM_MOVE messages. */
   if (!(Window->state & WNDS_SENDSIZEMOVEMSGS))
   {
        co_WinPosSendSizeMove(Window);
   }

   /* Show or maybe minimize or maximize the window. */

   style = IntSetStyle( Window, 0, WS_MAXIMIZE | WS_MINIMIZE );
   if (style & (WS_MINIMIZE | WS_MAXIMIZE))
   {
      RECTL NewPos;
      UINT SwFlag = (style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;

      SwFlag = co_WinPosMinMaximize(Window, SwFlag, &NewPos);
      SwFlag |= SWP_NOZORDER|SWP_FRAMECHANGED; /* Frame always gets changed */
      if (!(style & WS_VISIBLE) || (style & WS_CHILD) || UserGetActiveWindow() ||
          (Window->ExStyle & WS_EX_NOACTIVATE))
      {
         SwFlag |= SWP_NOACTIVATE;
      }
      co_WinPosSetWindowPos(Window, 0, NewPos.left, NewPos.top,
                            NewPos.right, NewPos.bottom, SwFlag);
   }

   /* Send the WM_PARENTNOTIFY message */
   IntSendParentNotify(Window, WM_CREATE);

   /* Notify the shell that a new window was created */
   if (Window->spwndOwner == NULL ||
       !(Window->spwndOwner->style & WS_VISIBLE) ||
       (Window->spwndOwner->ExStyle & WS_EX_TOOLWINDOW))
   {
      if (UserIsDesktopWindow(Window->spwndParent) &&
          (Window->style & WS_VISIBLE) &&
          (!(Window->ExStyle & WS_EX_TOOLWINDOW) ||
           (Window->ExStyle & WS_EX_APPWINDOW)))
      {
         co_IntShellHookNotify(HSHELL_WINDOWCREATED, (WPARAM)hWnd, 0);
      }
   }

   /* Initialize and show the window's scrollbars */
   if (Window->style & WS_VSCROLL)
   {
      co_UserShowScrollBar(Window, SB_VERT, FALSE, TRUE);
   }
   if (Window->style & WS_HSCROLL)
   {
      co_UserShowScrollBar(Window, SB_HORZ, TRUE, FALSE);
   }

   /* Show the new window */
   if (Cs->style & WS_VISIBLE)
   {
      if (Window->style & WS_MAXIMIZE)
         dwShowMode = SW_SHOW;
      else if (Window->style & WS_MINIMIZE)
         dwShowMode = SW_SHOWMINIMIZED;

      co_WinPosShowWindow(Window, dwShowMode);

      if (Window->ExStyle & WS_EX_MDICHILD)
      {
          ASSERT(ParentWindow);
          if(!ParentWindow)
              goto cleanup;
        co_IntSendMessage(UserHMGetHandle(ParentWindow), WM_MDIREFRESHMENU, 0, 0);
        /* ShowWindow won't activate child windows */
        co_WinPosSetWindowPos(Window, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
      }
   }

   if (Class->atomClassName == gaGuiConsoleWndClass)
   {
       /* Count only console windows manually */
       co_IntUserManualGuiCheck(TRUE);
   }

   TRACE("co_UserCreateWindowEx(%wZ): Created window %p\n", ClassName, hWnd);
   ret = Window;

cleanup:
   if (!ret)
   {
       TRACE("co_UserCreateWindowEx(): Error Created window!\n");
       /* If the window was created, the class will be dereferenced by co_UserDestroyWindow */
       if (Window)
            co_UserDestroyWindow(Window);
       else if (Class)
           IntDereferenceClass(Class, pti->pDeskInfo, pti->ppi);
   }

   if (pCsw) ExFreePoolWithTag(pCsw, TAG_HOOK);
   if (pCbtCreate) ExFreePoolWithTag(pCbtCreate, TAG_HOOK);
   if (pszName) UserHeapFree(pszName);
   if (pszClass) UserHeapFree(pszClass);

   if (Window)
   {
      UserDerefObjectCo(Window);
   }
   if (ParentWindow) UserDerefObjectCo(ParentWindow);

   // See CORE-13717, not setting error on success.
   if (ret)
      EngSetLastError(ERROR_SUCCESS);

   return ret;
}

NTSTATUS
NTAPI
ProbeAndCaptureLargeString(
    OUT PLARGE_STRING plstrSafe,
    IN PLARGE_STRING plstrUnsafe)
{
    LARGE_STRING lstrTemp;
    PVOID pvBuffer = NULL;

    _SEH2_TRY
    {
        /* Probe and copy the string */
        ProbeForRead(plstrUnsafe, sizeof(LARGE_STRING), sizeof(ULONG));
        lstrTemp = *plstrUnsafe;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail */
        _SEH2_YIELD(return _SEH2_GetExceptionCode();)
    }
    _SEH2_END

    if (lstrTemp.Length != 0)
    {
        /* Allocate a buffer from paged pool */
        pvBuffer = ExAllocatePoolWithTag(PagedPool, lstrTemp.Length, TAG_STRING);
        if (!pvBuffer)
        {
            return STATUS_NO_MEMORY;
        }

        _SEH2_TRY
        {
            /* Probe and copy the buffer */
            ProbeForRead(lstrTemp.Buffer, lstrTemp.Length, sizeof(WCHAR));
            RtlCopyMemory(pvBuffer, lstrTemp.Buffer, lstrTemp.Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Cleanup and fail */
            ExFreePoolWithTag(pvBuffer, TAG_STRING);
            _SEH2_YIELD(return _SEH2_GetExceptionCode();)
        }
        _SEH2_END
    }

    /* Set the output string */
    plstrSafe->Buffer = pvBuffer;
    plstrSafe->Length = lstrTemp.Length;
    plstrSafe->MaximumLength = lstrTemp.Length;

    return STATUS_SUCCESS;
}

/**
 * \todo Allow passing plstrClassName as ANSI.
 */
HWND
NTAPI
NtUserCreateWindowEx(
    DWORD dwExStyle,
    PLARGE_STRING plstrClassName,
    PLARGE_STRING plstrClsVersion,
    PLARGE_STRING plstrWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam,
    DWORD dwFlags,
    PVOID acbiBuffer)
{
    NTSTATUS Status;
    LARGE_STRING lstrWindowName;
    LARGE_STRING lstrClassName;
    LARGE_STRING lstrClsVersion;
    UNICODE_STRING ustrClassName;
    UNICODE_STRING ustrClsVersion;
    CREATESTRUCTW Cs;
    HWND hwnd = NULL;
    PWND pwnd;

    lstrWindowName.Buffer = NULL;
    lstrClassName.Buffer = NULL;
    lstrClsVersion.Buffer = NULL;

    if ( (dwStyle & (WS_POPUP|WS_CHILD)) != WS_CHILD)
    {
        /* check hMenu is valid handle */
        if (hMenu && !UserGetMenuObject(hMenu))
        {
            ERR("NtUserCreateWindowEx: Got an invalid menu handle!\n");
            EngSetLastError(ERROR_INVALID_MENU_HANDLE);
            return NULL;
        }
    }

    /* Copy the window name to kernel mode */
    Status = ProbeAndCaptureLargeString(&lstrWindowName, plstrWindowName);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtUserCreateWindowEx: failed to capture plstrWindowName\n");
        SetLastNtError(Status);
        return NULL;
    }

    plstrWindowName = &lstrWindowName;

    /* Check if the class is an atom */
    if (IS_ATOM(plstrClassName))
    {
        /* It is, pass the atom in the UNICODE_STRING */
        ustrClassName.Buffer = (PVOID)plstrClassName;
        ustrClassName.Length = 0;
        ustrClassName.MaximumLength = 0;
    }
    else
    {
        /* It's not, capture the class name */
        Status = ProbeAndCaptureLargeString(&lstrClassName, plstrClassName);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtUserCreateWindowEx: failed to capture plstrClassName\n");
            /* Set last error, cleanup and return */
            SetLastNtError(Status);
            goto cleanup;
        }

        /* We pass it on as a UNICODE_STRING */
        ustrClassName.Buffer = lstrClassName.Buffer;
        ustrClassName.Length = (USHORT)min(lstrClassName.Length, MAXUSHORT); // FIXME: LARGE_STRING truncated
        ustrClassName.MaximumLength = (USHORT)min(lstrClassName.MaximumLength, MAXUSHORT);
    }

    /* Check if the class version is an atom */
    if (IS_ATOM(plstrClsVersion))
    {
        /* It is, pass the atom in the UNICODE_STRING */
        ustrClsVersion.Buffer = (PVOID)plstrClsVersion;
        ustrClsVersion.Length = 0;
        ustrClsVersion.MaximumLength = 0;
    }
    else
    {
        /* It's not, capture the class name */
        Status = ProbeAndCaptureLargeString(&lstrClsVersion, plstrClsVersion);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtUserCreateWindowEx: failed to capture plstrClsVersion\n");
            /* Set last error, cleanup and return */
            SetLastNtError(Status);
            goto cleanup;
        }

        /* We pass it on as a UNICODE_STRING */
        ustrClsVersion.Buffer = lstrClsVersion.Buffer;
        ustrClsVersion.Length = (USHORT)min(lstrClsVersion.Length, MAXUSHORT); // FIXME: LARGE_STRING truncated
        ustrClsVersion.MaximumLength = (USHORT)min(lstrClsVersion.MaximumLength, MAXUSHORT);
    }

    /* Fill the CREATESTRUCTW */
    /* we will keep here the original parameters */
    Cs.style = dwStyle;
    Cs.lpCreateParams = lpParam;
    Cs.hInstance = hInstance;
    Cs.hMenu = hMenu;
    Cs.hwndParent = hWndParent;
    Cs.cx = nWidth;
    Cs.cy = nHeight;
    Cs.x = x;
    Cs.y = y;
    Cs.lpszName = (LPCWSTR) plstrWindowName->Buffer;
    Cs.lpszClass = ustrClassName.Buffer;
    Cs.dwExStyle = dwExStyle;

    UserEnterExclusive();

    /* Call the internal function */
    pwnd = co_UserCreateWindowEx(&Cs, &ustrClsVersion, plstrWindowName, acbiBuffer, dwFlags);

    if(!pwnd)
    {
        ERR("co_UserCreateWindowEx failed!\n");
    }
    hwnd = pwnd ? UserHMGetHandle(pwnd) : NULL;

    UserLeave();

cleanup:
    if (lstrWindowName.Buffer)
    {
        ExFreePoolWithTag(lstrWindowName.Buffer, TAG_STRING);
    }
    if (lstrClassName.Buffer)
    {
        ExFreePoolWithTag(lstrClassName.Buffer, TAG_STRING);
    }
    if (lstrClsVersion.Buffer)
    {
        ExFreePoolWithTag(lstrClsVersion.Buffer, TAG_STRING);
    }

   return hwnd;
}

// Win: xxxDW_DestroyOwnedWindows
VOID FASTCALL IntDestroyOwnedWindows(PWND Window)
{
    HWND* List;
    HWND* phWnd;
    PWND pWnd;
    USER_REFERENCE_ENTRY Ref;

    List = IntWinListOwnedPopups(Window);
    if (!List)
        return;

    for (phWnd = List; *phWnd; ++phWnd)
    {
        pWnd = ValidateHwndNoErr(*phWnd);
        if (pWnd == NULL)
            continue;
        ASSERT(pWnd->spwndOwner == Window);
        ASSERT(pWnd != Window);

        WndSetOwner(pWnd, NULL);
        if (IntWndBelongsToThread(pWnd, PsGetCurrentThreadWin32Thread()))
        {
            UserRefObjectCo(pWnd, &Ref); // Temp HACK?
            co_UserDestroyWindow(pWnd);
            UserDerefObjectCo(pWnd); // Temp HACK?
        }
        else
        {
            ERR("IntWndBelongsToThread(0x%p) is FALSE, ignoring.\n", pWnd);
        }
    }

    ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
}

// Win: xxxDestroyWindow
BOOLEAN co_UserDestroyWindow(PVOID Object)
{
   HWND hWnd;
   PWND pwndTemp;
   PTHREADINFO ti;
   MSG msg;
   PWND Window = Object;

   ASSERT_REFS_CO(Window); // FIXME: Temp HACK?

   /* NtUserDestroyWindow does check if the window has already been destroyed
      but co_UserDestroyWindow can be called from more paths which means
      that it can also be called for a window that has already been destroyed. */
   if (!IntIsWindow(UserHMGetHandle(Window)))
   {
      TRACE("Tried to destroy a window twice\n");
      return TRUE;
   }

   hWnd = UserHMGetHandle(Window);
   ti = PsGetCurrentThreadWin32Thread();

   TRACE("co_UserDestroyWindow(Window = 0x%p, hWnd = 0x%p)\n", Window, hWnd);

   /* Check for owner thread */
   if (Window->head.pti != ti)
   {
       /* Check if we are destroying the desktop window */
       if (! ((Window->head.rpdesk->dwDTFlags & DF_DESTROYED) && Window == Window->head.rpdesk->pDeskInfo->spwnd))
       {
           EngSetLastError(ERROR_ACCESS_DENIED);
           return FALSE;
       }
   }

   /* If window was created successfully and it is hooked */
   if ((Window->state2 & WNDS2_WMCREATEMSGPROCESSED))
   {
      if (co_HOOK_CallHooks(WH_CBT, HCBT_DESTROYWND, (WPARAM) hWnd, 0))
      {
         ERR("Destroy Window WH_CBT Call Hook return!\n");
         return FALSE;
      }
   }

   if (Window->pcls->atomClassName != gpsi->atomSysClass[ICLS_IME])
   {
      if ((Window->style & (WS_POPUP|WS_CHILD)) != WS_CHILD)
      {
         if (Window->spwndOwner)
         {
            //ERR("DestroyWindow Owner out.\n");
            UserAttachThreadInput(Window->head.pti, Window->spwndOwner->head.pti, FALSE);
         }
      }
   }

   /* Inform the parent */
   if (Window->style & WS_CHILD)
   {
      IntSendParentNotify(Window, WM_DESTROY);
   }

   if (!Window->spwndOwner && !IntGetParent(Window))
   {
      co_IntShellHookNotify(HSHELL_WINDOWDESTROYED, (WPARAM) hWnd, 0);
   }

   /* Hide the window */
   if (Window->style & WS_VISIBLE)
   {
      if (Window->style & WS_CHILD)
      {
         /* Only child windows receive WM_SHOWWINDOW in DestroyWindow() */
         co_WinPosShowWindow(Window, SW_HIDE);
      }
      else
      {
         co_WinPosSetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_HIDEWINDOW );
      }
   }

   /* Adjust last active */
   if ((pwndTemp = Window->spwndOwner))
   {
      while (pwndTemp->spwndOwner)
         pwndTemp = pwndTemp->spwndOwner;

      if (pwndTemp->spwndLastActive == Window)
         WndSetLastActive(pwndTemp, Window->spwndOwner);
   }

   if (Window->spwndParent && IntIsWindow(UserHMGetHandle(Window)))
   {
      if ((Window->style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
      {
         if (!IntIsTopLevelWindow(Window))
         {
            //ERR("DestroyWindow Parent out.\n");
            UserAttachThreadInput(Window->head.pti, Window->spwndParent->head.pti, FALSE);
         }
      }
   }

   if (Window->head.pti->MessageQueue->spwndActive == Window)
      Window->head.pti->MessageQueue->spwndActive = NULL;
   if (Window->head.pti->MessageQueue->spwndFocus == Window)
      Window->head.pti->MessageQueue->spwndFocus = NULL;
   if (Window->head.pti->MessageQueue->spwndActivePrev == Window)
      Window->head.pti->MessageQueue->spwndActivePrev = NULL;
   if (Window->head.pti->MessageQueue->spwndCapture == Window)
      Window->head.pti->MessageQueue->spwndCapture = NULL;

   /*
    * Check if this window is the Shell's Desktop Window. If so set hShellWindow to NULL
    */

   if (ti->pDeskInfo != NULL)
   {
      if (ti->pDeskInfo->hShellWindow == hWnd)
      {
         ERR("Destroying the ShellWindow!\n");
         ti->pDeskInfo->hShellWindow = NULL;
      }
   }

   IntEngWindowChanged(Window, WOC_DELETE);

   if (!IntIsWindow(UserHMGetHandle(Window)))
   {
      return TRUE;
   }

    /* Recursively destroy owned windows */
    if (!(Window->style & WS_CHILD))
    {
        IntDestroyOwnedWindows(Window);
    }

    /* Generate mouse move message for the next window */
    msg.message = WM_MOUSEMOVE;
    msg.wParam = UserGetMouseButtonsState();
    msg.lParam = MAKELPARAM(gpsi->ptCursor.x, gpsi->ptCursor.y);
    msg.pt = gpsi->ptCursor;
    co_MsqInsertMouseMessage(&msg, 0, 0, TRUE);

   IntNotifyWinEvent(EVENT_OBJECT_DESTROY, Window, OBJID_WINDOW, CHILDID_SELF, 0);

   /* Send destroy messages */
   IntSendDestroyMsg(UserHMGetHandle(Window));

   /* Destroy the default IME window if necessary */
   if (IS_IMM_MODE() && !(ti->TIF_flags & TIF_INCLEANUP) &&
       ti->spwndDefaultIme && (ti->spwndDefaultIme != Window) &&
       !(Window->state & WNDS_DESTROYED) && !IS_WND_IMELIKE(Window))
   {
       if (IS_WND_CHILD(Window))
       {
           if (IntImeCanDestroyDefIMEforChild(ti->spwndDefaultIme, Window))
               co_UserDestroyWindow(ti->spwndDefaultIme);
       }
       else
       {
           if (IntImeCanDestroyDefIME(ti->spwndDefaultIme, Window))
               co_UserDestroyWindow(ti->spwndDefaultIme);
       }
   }

   if (!IntIsWindow(UserHMGetHandle(Window)))
   {
      return TRUE;
   }

   /* Destroy the window storage */
   co_UserFreeWindow(Window, PsGetCurrentProcessWin32Process(), PsGetCurrentThreadWin32Thread(), TRUE);

   return TRUE;
}


/*
 * @implemented
 */
BOOLEAN APIENTRY
NtUserDestroyWindow(HWND Wnd)
{
   PWND Window;
   BOOLEAN ret = FALSE;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserDestroyWindow\n");
   UserEnterExclusive();

   Window = UserGetWindowObject(Wnd);
   if (Window)
   {
      UserRefObjectCo(Window, &Ref); // FIXME: Dunno if win should be reffed during destroy...
      ret = co_UserDestroyWindow(Window);
      UserDerefObjectCo(Window); // FIXME: Dunno if win should be reffed during destroy...
   }

   TRACE("Leave NtUserDestroyWindow, ret=%u\n", ret);
   UserLeave();
   return ret;
}


HWND FASTCALL
IntFindWindow(PWND Parent,
              PWND ChildAfter,
              RTL_ATOM ClassAtom,
              PUNICODE_STRING WindowName)
{
   BOOL CheckWindowName;
   HWND *List, *phWnd;
   HWND Ret = NULL;
   UNICODE_STRING CurrentWindowName;

   ASSERT(Parent);

   CheckWindowName = WindowName->Buffer != 0;

   if((List = IntWinListChildren(Parent)))
   {
      phWnd = List;
      if(ChildAfter)
      {
         /* skip handles before and including ChildAfter */
         while(*phWnd && (*(phWnd++) != UserHMGetHandle(ChildAfter)))
            ;
      }

      /* search children */
      while(*phWnd)
      {
         PWND Child;
         if(!(Child = UserGetWindowObject(*(phWnd++))))
         {
            continue;
         }

         /* Do not send WM_GETTEXT messages in the kernel mode version!
            The user mode version however calls GetWindowText() which will
            send WM_GETTEXT messages to windows belonging to its processes */
         if (!ClassAtom || Child->pcls->atomNVClassName == ClassAtom)
         {
             // FIXME: LARGE_STRING truncated
             CurrentWindowName.Buffer = Child->strName.Buffer;
             CurrentWindowName.Length = (USHORT)min(Child->strName.Length, MAXUSHORT);
             CurrentWindowName.MaximumLength = (USHORT)min(Child->strName.MaximumLength, MAXUSHORT);
             if(!CheckWindowName ||
                (Child->strName.Length < 0xFFFF &&
                 !RtlCompareUnicodeString(WindowName, &CurrentWindowName, TRUE)))
             {
                Ret = UserHMGetHandle(Child);
                break;
             }
         }
      }
      ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
   }

   return Ret;
}

/*
 * FUNCTION:
 *   Searches a window's children for a window with the specified
 *   class and name
 * ARGUMENTS:
 *   hwndParent     = The window whose childs are to be searched.
 *       NULL = desktop
 *       HWND_MESSAGE = message-only windows
 *
 *   hwndChildAfter = Search starts after this child window.
 *       NULL = start from beginning
 *
 *   ucClassName    = Class name to search for
 *       Reguired parameter.
 *
 *   ucWindowName   = Window name
 *       ->Buffer == NULL = don't care
 *
 * RETURNS:
 *   The HWND of the window if it was found, otherwise NULL
 */
/*
 * @implemented
 */
HWND APIENTRY
NtUserFindWindowEx(HWND hwndParent,
                   HWND hwndChildAfter,
                   PUNICODE_STRING ucClassName,
                   PUNICODE_STRING ucWindowName,
                   DWORD dwUnknown)
{
   PWND Parent, ChildAfter;
   UNICODE_STRING ClassName = {0}, WindowName = {0};
   HWND Desktop, Ret = NULL;
   BOOL DoMessageWnd = FALSE;
   RTL_ATOM ClassAtom = (RTL_ATOM)0;

   TRACE("Enter NtUserFindWindowEx\n");
   UserEnterShared();

   if (ucClassName != NULL || ucWindowName != NULL)
   {
       _SEH2_TRY
       {
           if (ucClassName != NULL)
           {
               ClassName = ProbeForReadUnicodeString(ucClassName);
               if (ClassName.Length != 0)
               {
                   ProbeForRead(ClassName.Buffer,
                                ClassName.Length,
                                sizeof(WCHAR));
               }
               else if (!IS_ATOM(ClassName.Buffer))
               {
                   EngSetLastError(ERROR_INVALID_PARAMETER);
                   _SEH2_LEAVE;
               }

               if (!IntGetAtomFromStringOrAtom(&ClassName,
                                               &ClassAtom))
               {
                   EngSetLastError(ERROR_CANNOT_FIND_WND_CLASS);
                   _SEH2_LEAVE;
               }
           }

           if (ucWindowName != NULL)
           {
               WindowName = ProbeForReadUnicodeString(ucWindowName);
               if (WindowName.Length != 0)
               {
                   ProbeForRead(WindowName.Buffer,
                                WindowName.Length,
                                sizeof(WCHAR));
               }
           }
       }
       _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
       {
           SetLastNtError(_SEH2_GetExceptionCode());
           _SEH2_YIELD(goto Exit); // Return NULL
       }
       _SEH2_END;

       if (ucClassName != NULL)
       {
           if (ClassName.Length == 0 && ClassName.Buffer != NULL &&
               !IS_ATOM(ClassName.Buffer))
           {
               EngSetLastError(ERROR_INVALID_PARAMETER);
               goto Exit; // Return NULL
           }
           else if (ClassAtom == (RTL_ATOM)0)
           {
               /* LastError code was set by IntGetAtomFromStringOrAtom */
               goto Exit; // Return NULL
           }
       }
   }

   Desktop = IntGetCurrentThreadDesktopWindow();

   if(hwndParent == NULL)
   {
      hwndParent = Desktop;
      DoMessageWnd = TRUE;
   }
   else if(hwndParent == HWND_MESSAGE)
   {
     hwndParent = IntGetMessageWindow();
   }

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      goto Exit; // Return NULL
   }

   ChildAfter = NULL;
   if(hwndChildAfter && !(ChildAfter = UserGetWindowObject(hwndChildAfter)))
   {
      goto Exit; // Return NULL
   }

   _SEH2_TRY
   {
       if(UserHMGetHandle(Parent) == Desktop)
       {
          HWND *List, *phWnd;
          PWND TopLevelWindow;
          BOOLEAN CheckWindowName;
          BOOLEAN WindowMatches;
          BOOLEAN ClassMatches;

          /* windows searches through all top-level windows if the parent is the desktop
             window */

          if((List = IntWinListChildren(Parent)))
          {
             phWnd = List;

             if(ChildAfter)
             {
                /* skip handles before and including ChildAfter */
                while(*phWnd && (*(phWnd++) != UserHMGetHandle(ChildAfter)))
                   ;
             }

             CheckWindowName = WindowName.Buffer != 0;

             /* search children */
             while(*phWnd)
             {
                 UNICODE_STRING ustr;

                if(!(TopLevelWindow = UserGetWindowObject(*(phWnd++))))
                {
                   continue;
                }

                /* Do not send WM_GETTEXT messages in the kernel mode version!
                   The user mode version however calls GetWindowText() which will
                   send WM_GETTEXT messages to windows belonging to its processes */
                ustr.Buffer = TopLevelWindow->strName.Buffer;
                ustr.Length = (USHORT)min(TopLevelWindow->strName.Length, MAXUSHORT); // FIXME:LARGE_STRING truncated
                ustr.MaximumLength = (USHORT)min(TopLevelWindow->strName.MaximumLength, MAXUSHORT);
                WindowMatches = !CheckWindowName ||
                                (TopLevelWindow->strName.Length < 0xFFFF &&
                                 !RtlCompareUnicodeString(&WindowName, &ustr, TRUE));
                ClassMatches = (ClassAtom == (RTL_ATOM)0) ||
                               ClassAtom == TopLevelWindow->pcls->atomNVClassName;

                if (WindowMatches && ClassMatches)
                {
                   Ret = UserHMGetHandle(TopLevelWindow);
                   break;
                }

                if (IntFindWindow(TopLevelWindow, NULL, ClassAtom, &WindowName))
                {
                   /* window returns the handle of the top-level window, in case it found
                      the child window */
                   Ret = UserHMGetHandle(TopLevelWindow);
                   break;
                }

             }
             ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
          }
       }
       else
       {
          TRACE("FindWindowEx: Not Desktop Parent!\n");
          Ret = IntFindWindow(Parent, ChildAfter, ClassAtom, &WindowName);
       }

       if (Ret == NULL && DoMessageWnd)
       {
          PWND MsgWindows;

          if((MsgWindows = UserGetWindowObject(IntGetMessageWindow())))
          {
             Ret = IntFindWindow(MsgWindows, ChildAfter, ClassAtom, &WindowName);
          }
       }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       Ret = NULL;
   }
   _SEH2_END;

Exit:
   TRACE("Leave NtUserFindWindowEx, ret %p\n", Ret);
   UserLeave();
   return Ret;
}


/*
 * @implemented
 */
PWND FASTCALL UserGetAncestor(PWND Wnd, UINT Type)
{
   PWND WndAncestor, Parent;

   if (UserHMGetHandle(Wnd) == IntGetDesktopWindow())
   {
      return NULL;
   }

   switch (Type)
   {
      case GA_PARENT:
         {
            WndAncestor = Wnd->spwndParent;
            break;
         }

      case GA_ROOT:
         {
            WndAncestor = Wnd;
            Parent = NULL;

            for(;;)
            {
               if(!(Parent = WndAncestor->spwndParent))
               {
                  break;
               }
               if(IntIsDesktopWindow(Parent))
               {
                  break;
               }

               WndAncestor = Parent;
            }
            break;
         }

      case GA_ROOTOWNER:
         {
            WndAncestor = Wnd;

            for (;;)
            {
               Parent = IntGetParent(WndAncestor);

               if (!Parent)
               {
                  break;
               }

               WndAncestor = Parent;
            }
            break;
         }

      default:
         {
            return NULL;
         }
   }

   return WndAncestor;
}

/*
 * @implemented
 */
HWND APIENTRY
NtUserGetAncestor(HWND hWnd, UINT Type)
{
   PWND Window, Ancestor;
   HWND Ret = NULL;

   TRACE("Enter NtUserGetAncestor\n");
   UserEnterExclusive();

   Window = UserGetWindowObject(hWnd);
   if (Window)
   {
      Ancestor = UserGetAncestor(Window, Type);
      /* fixme: can UserGetAncestor ever return NULL for a valid window? */

      Ret = (Ancestor ? UserHMGetHandle(Ancestor) : NULL);
   }

   TRACE("Leave NtUserGetAncestor, ret=%p\n", Ret);
   UserLeave();
   return Ret;
}

////
//// ReactOS work around! Keep it the sames as in Combo.c and Controls.h
////
/* combo state struct */
typedef struct
{
   HWND           self;
   HWND           owner;
   UINT           dwStyle;
   HWND           hWndEdit;
   HWND           hWndLBox;
   UINT           wState;
   HFONT          hFont;
   RECT           textRect;
   RECT           buttonRect;
   RECT           droppedRect;
   INT            droppedIndex;
   INT            fixedOwnerDrawHeight;
   INT            droppedWidth;   /* last two are not used unless set */
   INT            editHeight;     /* explicitly */
   LONG           UIState;
} HEADCOMBO,*LPHEADCOMBO;

// Window Extra data container.
typedef struct _WND2CBOX
{
  WND;
  LPHEADCOMBO pCBox;
} WND2CBOX, *PWND2CBOX;

#define CBF_BUTTONDOWN          0x0002
////
////
////
BOOL
APIENTRY
NtUserGetComboBoxInfo(
   HWND hWnd,
   PCOMBOBOXINFO pcbi)
{
   PWND Wnd;
   PPROCESSINFO ppi;
   BOOL NotSameppi = FALSE;
   BOOL Ret = TRUE;

   TRACE("Enter NtUserGetComboBoxInfo\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      Ret = FALSE;
      goto Exit;
   }
   _SEH2_TRY
   {
        ProbeForWrite(pcbi, sizeof(COMBOBOXINFO), 1);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       Ret = FALSE;
       _SEH2_YIELD(goto Exit);
   }
   _SEH2_END;

   if (pcbi->cbSize < sizeof(COMBOBOXINFO))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      goto Exit;
   }

   // Pass the user pointer, it was already probed.
   if ((Wnd->pcls->atomClassName != gpsi->atomSysClass[ICLS_COMBOBOX]) && Wnd->fnid != FNID_COMBOBOX)
   {
      Ret = (BOOL)co_IntSendMessage(UserHMGetHandle(Wnd), CB_GETCOMBOBOXINFO, 0, (LPARAM)pcbi);
      goto Exit;
   }

   ppi = PsGetCurrentProcessWin32Process();
   NotSameppi = ppi != Wnd->head.pti->ppi;
   if (NotSameppi)
   {
      KeAttachProcess(&Wnd->head.pti->ppi->peProcess->Pcb);
   }

   _SEH2_TRY
   {
      LPHEADCOMBO lphc = ((PWND2CBOX)Wnd)->pCBox;
      pcbi->rcItem = lphc->textRect;
      pcbi->rcButton = lphc->buttonRect;
      pcbi->stateButton = 0;
      if (lphc->wState & CBF_BUTTONDOWN)
         pcbi->stateButton |= STATE_SYSTEM_PRESSED;
      if (RECTL_bIsEmptyRect(&lphc->buttonRect))
         pcbi->stateButton |= STATE_SYSTEM_INVISIBLE;
      pcbi->hwndCombo = lphc->self;
      pcbi->hwndItem = lphc->hWndEdit;
      pcbi->hwndList = lphc->hWndLBox;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Ret = FALSE;
      SetLastNtError(_SEH2_GetExceptionCode());
   }
   _SEH2_END;

Exit:
   if (NotSameppi) KeDetachProcess();
   TRACE("Leave NtUserGetComboBoxInfo, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

////
//// ReactOS work around! Keep it the sames as in Listbox.c
////
/* Listbox structure */
typedef struct
{
    HWND        self;           /* Our own window handle */
    HWND        owner;          /* Owner window to send notifications to */
    UINT        style;          /* Window style */
    INT         width;          /* Window width */
    INT         height;         /* Window height */
    VOID       *items;          /* Array of items */
    INT         nb_items;       /* Number of items */
    INT         top_item;       /* Top visible item */
    INT         selected_item;  /* Selected item */
    INT         focus_item;     /* Item that has the focus */
    INT         anchor_item;    /* Anchor item for extended selection */
    INT         item_height;    /* Default item height */
    INT         page_size;      /* Items per listbox page */
    INT         column_width;   /* Column width for multi-column listboxes */
} LB_DESCR;

// Window Extra data container.
typedef struct _WND2LB
{
  WND;
  LB_DESCR * pLBiv;
} WND2LB, *PWND2LB;
////
////
////
DWORD
APIENTRY
NtUserGetListBoxInfo(
   HWND hWnd)
{
   PWND Wnd;
   PPROCESSINFO ppi;
   BOOL NotSameppi = FALSE;
   DWORD Ret = 0;

   TRACE("Enter NtUserGetListBoxInfo\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return 0
   }

   if ((Wnd->pcls->atomClassName != gpsi->atomSysClass[ICLS_LISTBOX]) && Wnd->fnid != FNID_LISTBOX)
   {
      Ret = (DWORD)co_IntSendMessage(UserHMGetHandle(Wnd), LB_GETLISTBOXINFO, 0, 0);
      goto Exit;
   }

   // wine lisbox:test_GetListBoxInfo lb_getlistboxinfo = 0, should not send a message!
   ppi = PsGetCurrentProcessWin32Process();
   NotSameppi = ppi != Wnd->head.pti->ppi;
   if (NotSameppi)
   {
      KeAttachProcess(&Wnd->head.pti->ppi->peProcess->Pcb);
   }

   _SEH2_TRY
   {
      LB_DESCR *descr = ((PWND2LB)Wnd)->pLBiv;
      // See Controls ListBox.c:LB_GETLISTBOXINFO must match...
      Ret = descr->page_size;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Ret = 0;
      SetLastNtError(_SEH2_GetExceptionCode());
   }
   _SEH2_END;

Exit:
   if (NotSameppi) KeDetachProcess();
   TRACE("Leave NtUserGetListBoxInfo, ret=%lu\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * NtUserSetParent
 *
 * The NtUserSetParent function changes the parent window of the specified
 * child window.
 *
 * Remarks
 *    The new parent window and the child window must belong to the same
 *    application. If the window identified by the hWndChild parameter is
 *    visible, the system performs the appropriate redrawing and repainting.
 *    For compatibility reasons, NtUserSetParent does not modify the WS_CHILD
 *    or WS_POPUP window styles of the window whose parent is being changed.
 *
 * Status
 *    @implemented
 */

HWND APIENTRY
NtUserSetParent(HWND hWndChild, HWND hWndNewParent)
{
   HWND Ret;

   TRACE("Enter NtUserSetParent\n");
   UserEnterExclusive();

   /*
      Check Parent first from user space, set it here.
    */
   if (!hWndNewParent)
   {
      hWndNewParent = IntGetDesktopWindow();
   }
   else if (hWndNewParent == HWND_MESSAGE)
   {
      hWndNewParent = IntGetMessageWindow();
   }

   Ret = co_UserSetParent(hWndChild, hWndNewParent);

   TRACE("Leave NtUserSetParent, ret=%p\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * UserGetShellWindow
 *
 * Returns a handle to shell window that was set by NtUserSetShellWindowEx.
 *
 * Status
 *    @implemented
 */
HWND FASTCALL UserGetShellWindow(VOID)
{
   PWINSTATION_OBJECT WinStaObject;
   HWND Ret;

   NTSTATUS Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     UserMode,
                     0,
                     &WinStaObject,
                     0);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return NULL;
   }

   Ret = (HWND)WinStaObject->ShellWindow;

   ObDereferenceObject(WinStaObject);
   return Ret;
}

/*
 * NtUserSetShellWindowEx
 *
 * This is undocumented function to set global shell window. The global
 * shell window has special handling of window position.
 *
 * Status
 *    @implemented
 */
BOOL APIENTRY
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
   PWINSTATION_OBJECT WinStaObject;
   PWND WndShell, WndListView;
   BOOL Ret = FALSE;
   USER_REFERENCE_ENTRY Ref;
   NTSTATUS Status;
   PTHREADINFO ti;

   TRACE("Enter NtUserSetShellWindowEx\n");
   UserEnterExclusive();

   if (!(WndShell = UserGetWindowObject(hwndShell)))
   {
      goto Exit; // Return FALSE
   }

   if (!(WndListView = UserGetWindowObject(hwndListView)))
   {
      goto Exit; // Return FALSE
   }

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     UserMode,
                     0,
                     &WinStaObject,
                     0);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Exit; // Return FALSE
   }

   /*
    * Test if we are permitted to change the shell window.
    */
   if (WinStaObject->ShellWindow)
   {
      ObDereferenceObject(WinStaObject);
      goto Exit; // Return FALSE
   }

   /*
    * Move shell window into background.
    */
   if (hwndListView && hwndListView != hwndShell)
   {
      /*
       * Disabled for now to get Explorer working.
       * -- Filip, 01/nov/2003
       */
#if 0
      co_WinPosSetWindowPos(WndListView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif

      if (WndListView->ExStyle & WS_EX_TOPMOST)
      {
         ObDereferenceObject(WinStaObject);
         goto Exit; // Return FALSE
      }
   }

   if (WndShell->ExStyle & WS_EX_TOPMOST)
   {
      ObDereferenceObject(WinStaObject);
      goto Exit; // Return FALSE
   }

   UserRefObjectCo(WndShell, &Ref);
   WndShell->state2 |= WNDS2_BOTTOMMOST;
   co_WinPosSetWindowPos(WndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

   WinStaObject->ShellWindow = hwndShell;
   WinStaObject->ShellListView = hwndListView;

   ti = GetW32ThreadInfo();
   if (ti->pDeskInfo)
   {
       ti->pDeskInfo->hShellWindow = hwndShell;
       ti->pDeskInfo->spwndShell = WndShell;
       ti->pDeskInfo->spwndBkGnd = WndListView;
       ti->pDeskInfo->ppiShellProcess = ti->ppi;
   }

   UserRegisterHotKey(WndShell, SC_TASKLIST, MOD_CONTROL, VK_ESCAPE);

   UserDerefObjectCo(WndShell);

   ObDereferenceObject(WinStaObject);
   Ret = TRUE;

Exit:
   TRACE("Leave NtUserSetShellWindowEx, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

// Fixes wine Win test_window_styles and todo tests...
static BOOL FASTCALL
IntCheckFrameEdge(ULONG Style, ULONG ExStyle)
{
   if (ExStyle & WS_EX_DLGMODALFRAME)
      return TRUE;
   else if (!(ExStyle & WS_EX_STATICEDGE) && (Style & (WS_DLGFRAME | WS_THICKFRAME)))
      return TRUE;
   else
      return FALSE;
}

static LONG_PTR
co_IntSetWindowLongPtr(HWND hWnd, DWORD Index, LONG_PTR NewValue, BOOL Ansi, ULONG Size, BOOL bAlter)
{
   PWND Window, Parent;
   PWINSTATION_OBJECT WindowStation;
   LONG_PTR OldValue;
   STYLESTRUCT Style;

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      return 0;
   }

   if ((INT)Index >= 0)
   {
      if ((Index + Size) > Window->cbwndExtra)
      {
         EngSetLastError(ERROR_INVALID_INDEX);
         return 0;
      }

      PVOID Address = (PUCHAR)(&Window[1]) + Index;

#ifdef _WIN64
      if (Size == sizeof(LONG))
      {
         OldValue = ReadUnalignedU32(Address);
         WriteUnalignedU32(Address, NewValue);
      }
      else
#endif
      {
         OldValue = ReadUnalignedUlongPtr(Address);
         /*
         if ( Index == DWLP_DLGPROC && Wnd->state & WNDS_DIALOGWINDOW)
         {
            OldValue = (LONG_PTR)IntSetWindowProc( Wnd, (WNDPROC)NewValue, Ansi);
            if (!OldValue) return 0;
         }
         */
         WriteUnalignedUlongPtr(Address, NewValue);
      }

   }
   else
   {
#ifdef _WIN64
      if (Size == sizeof(LONG))
      {
         if ((Index != GWL_STYLE) &&
             (Index != GWL_EXSTYLE) &&
             (Index != GWL_ID) &&
             (Index != GWL_USERDATA))
         {
            ERR("NtUserSetWindowLong(): Index requires pointer size: %lu\n", Index);
            EngSetLastError(ERROR_INVALID_INDEX);
            return 0;
         }
      }
#endif

      switch (Index)
      {
         case GWL_EXSTYLE: // LONG
            OldValue = (LONG) Window->ExStyle;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;

            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM) &Style);

            /*
             * Remove extended window style bit WS_EX_TOPMOST for shell windows.
             */
            WindowStation = Window->head.pti->rpdesk->rpwinstaParent;
            if(WindowStation)
            {
               if (hWnd == WindowStation->ShellWindow || hWnd == WindowStation->ShellListView)
                  Style.styleNew &= ~WS_EX_TOPMOST;
            }
            /* WS_EX_WINDOWEDGE depends on some other styles */
            if (IntCheckFrameEdge(Window->style, NewValue))
               Style.styleNew |= WS_EX_WINDOWEDGE;
            else
               Style.styleNew &= ~WS_EX_WINDOWEDGE;

            if (!(Window->ExStyle & WS_EX_LAYERED))
            {
               SetLayeredStatus(Window, 0);
            }

            Window->ExStyle = (DWORD)Style.styleNew;

            co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_EXSTYLE, (LPARAM) &Style);
            break;

         case GWL_STYLE: // LONG
            OldValue = (LONG) Window->style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;

            if (!bAlter)
                co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);

            /* WS_CLIPSIBLINGS can't be reset on top-level windows */
            if (UserIsDesktopWindow(Window->spwndParent)) Style.styleNew |= WS_CLIPSIBLINGS;
            /* WS_MINIMIZE can't be reset */
            if (OldValue & WS_MINIMIZE) Style.styleNew |= WS_MINIMIZE;
            /* Fixes wine FIXME: changing WS_DLGFRAME | WS_THICKFRAME is supposed to change WS_EX_WINDOWEDGE too */
            if (IntCheckFrameEdge(NewValue, Window->ExStyle))
               Window->ExStyle |= WS_EX_WINDOWEDGE;
            else
               Window->ExStyle &= ~WS_EX_WINDOWEDGE;

            if ((OldValue & (WS_CHILD | WS_POPUP)) == WS_CHILD)
            {
               if ((NewValue & (WS_CHILD | WS_POPUP)) != WS_CHILD)
               {
                  //// From child to non-child it should be null already.
                  ERR("IDMenu going null! %d\n",Window->IDMenu);
                  Window->IDMenu = 0; // Window->spmenu = 0;
               }
            }
            else
            {
               if ((NewValue & (WS_CHILD | WS_POPUP)) == WS_CHILD)
               {
                  PMENU pMenu = UserGetMenuObject(UlongToHandle(Window->IDMenu));
                  Window->state &= ~WNDS_HASMENU;
                  if (pMenu)
                  {
                     ERR("IDMenu released 0x%p\n",pMenu);
                     // ROS may not hold a lock after setting menu to window. But it should!
                     //IntReleaseMenuObject(pMenu);
                  }
               }
            }

            if ((Style.styleOld ^ Style.styleNew) & WS_VISIBLE)
            {
               if (Style.styleOld & WS_VISIBLE) Window->head.pti->cVisWindows--;
               if (Style.styleNew & WS_VISIBLE) Window->head.pti->cVisWindows++;
               DceResetActiveDCEs( Window );
            }
            Window->style = (DWORD)Style.styleNew;

            if (!bAlter)
                co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_STYLE, (LPARAM) &Style);
            break;

         case GWLP_WNDPROC: // LONG_PTR
         {
            if ( Window->head.pti->ppi != PsGetCurrentProcessWin32Process() ||
                 Window->fnid & FNID_FREED)
            {
               EngSetLastError(ERROR_ACCESS_DENIED);
               return 0;
            }
            OldValue = (LONG_PTR)IntSetWindowProc(Window,
                                                  (WNDPROC)NewValue,
                                                  Ansi);
            break;
         }

         case GWLP_HINSTANCE: // LONG_PTR
            OldValue = (LONG_PTR) Window->hModule;
            Window->hModule = (HINSTANCE) NewValue;
            break;

         case GWLP_HWNDPARENT: // LONG_PTR
            Parent = Window->spwndParent;
            if (Parent && (UserHMGetHandle(Parent) == IntGetDesktopWindow()))
               OldValue = (LONG_PTR)IntSetOwner(UserHMGetHandle(Window), (HWND)NewValue);
            else
               OldValue = (LONG_PTR)co_UserSetParent(UserHMGetHandle(Window), (HWND)NewValue);
            break;

         case GWLP_ID: // LONG
            OldValue = (LONG) Window->IDMenu;
            Window->IDMenu = (UINT) NewValue;
            break;

         case GWLP_USERDATA: // LONG or LONG_PTR
            OldValue = Window->dwUserData;
            Window->dwUserData = NewValue;
            break;

         default:
            ERR("NtUserSetWindowLong(): Unsupported index %lu\n", Index);
            EngSetLastError(ERROR_INVALID_INDEX);
            OldValue = 0;
            break;
      }
   }

   return OldValue;
}

LONG FASTCALL
co_UserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
    return (LONG)co_IntSetWindowLongPtr(hWnd, Index, NewValue, Ansi, sizeof(LONG), FALSE);
}

LONG_PTR FASTCALL
co_UserSetWindowLongPtr(HWND hWnd, DWORD Index, LONG_PTR NewValue, BOOL Ansi)
{
    return co_IntSetWindowLongPtr(hWnd, Index, NewValue, Ansi, sizeof(LONG_PTR), FALSE);
}

/*
 * NtUserSetWindowLong
 *
 * The NtUserSetWindowLong function changes an attribute of the specified
 * window. The function also sets the 32-bit (long) value at the specified
 * offset into the extra window memory.
 *
 * Status
 *    @implemented
 */

LONG APIENTRY
NtUserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   LONG ret;

   UserEnterExclusive();

   if (hWnd == IntGetDesktopWindow())
   {
      EngSetLastError(STATUS_ACCESS_DENIED);
      UserLeave();
      return 0;
   }

   ret = (LONG)co_IntSetWindowLongPtr(hWnd, Index, NewValue, Ansi, sizeof(LONG), FALSE);

   UserLeave();

   return ret;
}

#ifdef _WIN64
LONG_PTR APIENTRY
NtUserSetWindowLongPtr(HWND hWnd, DWORD Index, LONG_PTR NewValue, BOOL Ansi)
{
    LONG_PTR ret;

    UserEnterExclusive();

    if (hWnd == IntGetDesktopWindow())
    {
        EngSetLastError(STATUS_ACCESS_DENIED);
        UserLeave();
        return 0;
    }

    ret = co_IntSetWindowLongPtr(hWnd, Index, NewValue, Ansi, sizeof(LONG_PTR), FALSE);

    UserLeave();

    return ret;
}
#endif // _WIN64

DWORD APIENTRY
NtUserAlterWindowStyle(HWND hWnd, DWORD Index, LONG NewValue)
{
   LONG ret;

   UserEnterExclusive();

   if (hWnd == IntGetDesktopWindow())
   {
      EngSetLastError(STATUS_ACCESS_DENIED);
      UserLeave();
      return 0;
   }

   ret = co_IntSetWindowLongPtr(hWnd, Index, NewValue, FALSE, sizeof(LONG), TRUE);

   UserLeave();

   return ret;
}


/*
 * NtUserSetWindowWord
 *
 * Legacy function similar to NtUserSetWindowLong.
 *
 * Status
 *    @implemented
 */

WORD APIENTRY
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewValue)
{
   PWND Window;
   WORD OldValue;
   WORD Ret = 0;

   TRACE("Enter NtUserSetWindowWord\n");
   UserEnterExclusive();

   if (hWnd == IntGetDesktopWindow())
   {
      EngSetLastError(STATUS_ACCESS_DENIED);
      goto Exit; // Return 0
   }

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return 0
   }

   switch (Index)
   {
      case GWL_ID:
      case GWL_HINSTANCE:
      case GWL_HWNDPARENT:
         Ret = (WORD)co_UserSetWindowLong(UserHMGetHandle(Window), Index, (UINT)NewValue, TRUE);
         goto Exit;

      default:
         if (Index < 0)
         {
            EngSetLastError(ERROR_INVALID_INDEX);
            goto Exit; // Return 0
         }
   }

   if ((ULONG)Index > (Window->cbwndExtra - sizeof(WORD)))
   {
      EngSetLastError(ERROR_INVALID_INDEX);
      goto Exit; // Return 0
   }

   OldValue = *((WORD *)((PCHAR)(Window + 1) + Index));
   *((WORD *)((PCHAR)(Window + 1) + Index)) = NewValue;

   Ret = OldValue;

Exit:
   TRACE("Leave NtUserSetWindowWord, ret=%u\n", Ret);
   UserLeave();
   return Ret;
}

/*
 QueryWindow based on KJK::Hyperion and James Tabor.

 0 = QWUniqueProcessId
 1 = QWUniqueThreadId
 2 = QWActiveWindow
 3 = QWFocusWindow
 4 = QWIsHung            Implements IsHungAppWindow found
                                by KJK::Hyperion.

 9 = QWKillWindow        When I called this with hWnd ==
                           DesktopWindow, it shutdown the system
                           and rebooted.
*/
/*
 * @implemented
 */
DWORD_PTR APIENTRY
NtUserQueryWindow(HWND hWnd, DWORD Index)
{
/* Console Leader Process CID Window offsets */
#define GWLP_CONSOLE_LEADER_PID 0
#define GWLP_CONSOLE_LEADER_TID 4

   DWORD_PTR Result = 0;
   PWND pWnd, pwndActive;
   PTHREADINFO pti, ptiActive;

   TRACE("Enter NtUserQueryWindow\n");
   UserEnterShared();

   if (!(pWnd = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return 0
   }

   switch(Index)
   {
      case QUERY_WINDOW_UNIQUE_PROCESS_ID:
      {
         if ( (pWnd->head.pti->TIF_flags & TIF_CSRSSTHREAD) &&
              (pWnd->pcls->atomClassName == gaGuiConsoleWndClass) )
         {
            // IntGetWindowLong(offset == GWLP_CONSOLE_LEADER_PID)
            Result = (DWORD_PTR)(*((LONG_PTR*)((PCHAR)(pWnd + 1) + GWLP_CONSOLE_LEADER_PID)));
         }
         else
         {
            Result = (DWORD_PTR)IntGetWndProcessId(pWnd);
         }
         break;
      }

      case QUERY_WINDOW_UNIQUE_THREAD_ID:
      {
         if ( (pWnd->head.pti->TIF_flags & TIF_CSRSSTHREAD) &&
              (pWnd->pcls->atomClassName == gaGuiConsoleWndClass) )
         {
            // IntGetWindowLong(offset == GWLP_CONSOLE_LEADER_TID)
            Result = (DWORD_PTR)(*((LONG_PTR*)((PCHAR)(pWnd + 1) + GWLP_CONSOLE_LEADER_TID)));
         }
         else
         {
            Result = (DWORD_PTR)IntGetWndThreadId(pWnd);
         }
         break;
      }

      case QUERY_WINDOW_ACTIVE:
         Result = (DWORD_PTR)(pWnd->head.pti->MessageQueue->spwndActive ? UserHMGetHandle(pWnd->head.pti->MessageQueue->spwndActive) : 0);
         break;

      case QUERY_WINDOW_FOCUS:
         Result = (DWORD_PTR)(pWnd->head.pti->MessageQueue->spwndFocus ? UserHMGetHandle(pWnd->head.pti->MessageQueue->spwndFocus) : 0);
         break;

      case QUERY_WINDOW_ISHUNG:
         Result = (pWnd->fnid == FNID_GHOST) || MsqIsHung(pWnd->head.pti, MSQ_HUNG);
         break;

      case QUERY_WINDOW_REAL_ID:
         Result = (DWORD_PTR)pWnd->head.pti->pEThread->Cid.UniqueProcess;
         break;

      case QUERY_WINDOW_FOREGROUND:
         Result = (pWnd->head.pti->MessageQueue == gpqForeground);
         break;

      case QUERY_WINDOW_DEFAULT_IME: /* default IME window */
         if (pWnd->head.pti->spwndDefaultIme)
            Result = (DWORD_PTR)UserHMGetHandle(pWnd->head.pti->spwndDefaultIme);
         break;

      case QUERY_WINDOW_DEFAULT_ICONTEXT: /* default input context handle */
         if (pWnd->head.pti->spDefaultImc)
            Result = (DWORD_PTR)UserHMGetHandle(pWnd->head.pti->spDefaultImc);
         break;

      case QUERY_WINDOW_ACTIVE_IME:
         if (gpqForeground && gpqForeground->spwndActive)
         {
             pwndActive = gpqForeground->spwndActive;
             pti = PsGetCurrentThreadWin32Thread();
             if (pti->rpdesk == pwndActive->head.rpdesk)
             {
                ptiActive = pwndActive->head.pti;
                if (ptiActive->spwndDefaultIme)
                   Result = (DWORD_PTR)UserHMGetHandle(ptiActive->spwndDefaultIme);
             }
         }
         break;
   }

Exit:
   TRACE("Leave NtUserQueryWindow, ret=%u\n", Result);
   UserLeave();
   return Result;
}

/*
 * @implemented
 */
UINT APIENTRY
NtUserRegisterWindowMessage(PUNICODE_STRING MessageNameUnsafe)
{
   UNICODE_STRING SafeMessageName;
   NTSTATUS Status;
   UINT Ret = 0;

   TRACE("Enter NtUserRegisterWindowMessage\n");
   UserEnterExclusive();

   if(MessageNameUnsafe == NULL)
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      goto Exit; // Return 0
   }

   Status = IntSafeCopyUnicodeStringTerminateNULL(&SafeMessageName, MessageNameUnsafe);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Exit; // Return 0
   }

   Ret = (UINT)IntAddAtom(SafeMessageName.Buffer);
   if (SafeMessageName.Buffer)
      ExFreePoolWithTag(SafeMessageName.Buffer, TAG_STRING);

Exit:
   TRACE("Leave NtUserRegisterWindowMessage, ret=%u\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowFNID(HWND hWnd,
                    WORD fnID)
{
   PWND Wnd;
   BOOL Ret = FALSE;

   TRACE("Enter NtUserSetWindowFNID\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return FALSE
   }

   if (Wnd->head.pti->ppi != PsGetCurrentProcessWin32Process())
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      goto Exit; // Return FALSE
   }

   // From user land we only set these.
   if (fnID != FNID_DESTROY)
   {
      /* HACK: The minimum should be FNID_BUTTON, but menu code relies on this */
      if (fnID < FNID_FIRST || fnID > FNID_GHOST ||
          Wnd->fnid != 0)
      {
         EngSetLastError(ERROR_INVALID_PARAMETER);
         goto Exit; // Return FALSE
      }
   }

   Wnd->fnid |= fnID;
   Ret = TRUE;

Exit:
   TRACE("Leave NtUserSetWindowFNID\n");
   UserLeave();
   return Ret;
}

BOOL APIENTRY
DefSetText(PWND Wnd, PCWSTR WindowText)
{
   UNICODE_STRING UnicodeString;
   BOOL Ret = FALSE;

   RtlInitUnicodeString(&UnicodeString, WindowText);

   if (UnicodeString.Length != 0)
   {
      if (Wnd->strName.MaximumLength > 0 &&
          UnicodeString.Length <= Wnd->strName.MaximumLength - sizeof(UNICODE_NULL))
      {
         ASSERT(Wnd->strName.Buffer != NULL);

         Wnd->strName.Length = UnicodeString.Length;
         Wnd->strName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
         RtlCopyMemory(Wnd->strName.Buffer,
                              UnicodeString.Buffer,
                              UnicodeString.Length);
      }
      else
      {
         PWCHAR buf;
         Wnd->strName.MaximumLength = Wnd->strName.Length = 0;
         buf = Wnd->strName.Buffer;
         Wnd->strName.Buffer = NULL;
         if (buf != NULL)
         {
            DesktopHeapFree(Wnd->head.rpdesk, buf);
         }

         Wnd->strName.Buffer = DesktopHeapAlloc(Wnd->head.rpdesk,
                                                   UnicodeString.Length + sizeof(UNICODE_NULL));
         if (Wnd->strName.Buffer != NULL)
         {
            Wnd->strName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
            RtlCopyMemory(Wnd->strName.Buffer,
                                 UnicodeString.Buffer,
                                 UnicodeString.Length);
            Wnd->strName.MaximumLength = UnicodeString.Length + sizeof(UNICODE_NULL);
            Wnd->strName.Length = UnicodeString.Length;
         }
         else
         {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
         }
      }
   }
   else
   {
      Wnd->strName.Length = 0;
      if (Wnd->strName.Buffer != NULL)
          Wnd->strName.Buffer[0] = L'\0';
   }

   // FIXME: HAX! Windows does not do this in here!
   // In User32, these are called after: NotifyWinEvent EVENT_OBJECT_NAMECHANGE than
   // RepaintButton, StaticRepaint, NtUserCallHwndLock HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK, etc.
   /* Send shell notifications */
   if (!Wnd->spwndOwner && !IntGetParent(Wnd))
   {
      co_IntShellHookNotify(HSHELL_REDRAW, (WPARAM) UserHMGetHandle(Wnd), FALSE); // FIXME Flashing?
   }

   Ret = TRUE;
Exit:
   if (UnicodeString.Buffer) RtlFreeUnicodeString(&UnicodeString);
   return Ret;
}

/*
 * NtUserDefSetText
 *
 * Undocumented function that is called from DefWindowProc to set
 * window text.
 *
 * Status
 *    @implemented
 */
BOOL APIENTRY
NtUserDefSetText(HWND hWnd, PLARGE_STRING WindowText)
{
   PWND Wnd;
   LARGE_STRING SafeText;
   UNICODE_STRING UnicodeString;
   BOOL Ret = TRUE;

   TRACE("Enter NtUserDefSetText\n");

   if (WindowText != NULL)
   {
      _SEH2_TRY
      {
         SafeText = ProbeForReadLargeString(WindowText);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Ret = FALSE;
         SetLastNtError(_SEH2_GetExceptionCode());
      }
      _SEH2_END;

      if (!Ret)
         return FALSE;
   }
   else
      return TRUE;

   UserEnterExclusive();

   if(!(Wnd = UserGetWindowObject(hWnd)))
   {
      UserLeave();
      return FALSE;
   }

   // ReactOS uses Unicode and not mixed. Up/Down converting will take time.
   // Brought to you by: The Wine Project! Dysfunctional Thought Processes!
   // Now we know what the bAnsi is for.
   RtlInitUnicodeString(&UnicodeString, NULL);
   if (SafeText.Buffer)
   {
      _SEH2_TRY
      {
         if (SafeText.bAnsi)
            ProbeForRead(SafeText.Buffer, SafeText.Length, sizeof(CHAR));
         else
            ProbeForRead(SafeText.Buffer, SafeText.Length, sizeof(WCHAR));
         Ret = RtlLargeStringToUnicodeString(&UnicodeString, &SafeText);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Ret = FALSE;
         SetLastNtError(_SEH2_GetExceptionCode());
      }
      _SEH2_END;
      if (!Ret) goto Exit;
   }

   if (UnicodeString.Length != 0)
   {
      if (Wnd->strName.MaximumLength > 0 &&
          UnicodeString.Length <= Wnd->strName.MaximumLength - sizeof(UNICODE_NULL))
      {
         ASSERT(Wnd->strName.Buffer != NULL);

         Wnd->strName.Length = UnicodeString.Length;
         Wnd->strName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
         RtlCopyMemory(Wnd->strName.Buffer,
                              UnicodeString.Buffer,
                              UnicodeString.Length);
      }
      else
      {
         PWCHAR buf;
         Wnd->strName.MaximumLength = Wnd->strName.Length = 0;
         buf = Wnd->strName.Buffer;
         Wnd->strName.Buffer = NULL;
         if (buf != NULL)
         {
            DesktopHeapFree(Wnd->head.rpdesk, buf);
         }

         Wnd->strName.Buffer = DesktopHeapAlloc(Wnd->head.rpdesk,
                                                   UnicodeString.Length + sizeof(UNICODE_NULL));
         if (Wnd->strName.Buffer != NULL)
         {
            Wnd->strName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
            RtlCopyMemory(Wnd->strName.Buffer,
                                 UnicodeString.Buffer,
                                 UnicodeString.Length);
            Wnd->strName.MaximumLength = UnicodeString.Length + sizeof(UNICODE_NULL);
            Wnd->strName.Length = UnicodeString.Length;
         }
         else
         {
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Ret = FALSE;
            goto Exit;
         }
      }
   }
   else
   {
      Wnd->strName.Length = 0;
      if (Wnd->strName.Buffer != NULL)
          Wnd->strName.Buffer[0] = L'\0';
   }

   // FIXME: HAX! Windows does not do this in here!
   // In User32, these are called after: NotifyWinEvent EVENT_OBJECT_NAMECHANGE than
   // RepaintButton, StaticRepaint, NtUserCallHwndLock HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK, etc.
   /* Send shell notifications */
   if (!Wnd->spwndOwner && !IntGetParent(Wnd))
   {
      co_IntShellHookNotify(HSHELL_REDRAW, (WPARAM) hWnd, FALSE); // FIXME Flashing?
   }

   Ret = TRUE;
Exit:
   if (UnicodeString.Buffer) RtlFreeUnicodeString(&UnicodeString);
   TRACE("Leave NtUserDefSetText, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * NtUserInternalGetWindowText
 *
 * Status
 *    @implemented
 */

INT APIENTRY
NtUserInternalGetWindowText(HWND hWnd, LPWSTR lpString, INT nMaxCount)
{
   PWND Wnd;
   NTSTATUS Status;
   INT Result = 0;

   TRACE("Enter NtUserInternalGetWindowText\n");
   UserEnterShared();

   if(lpString && (nMaxCount <= 1))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      goto Exit; // Return 0
   }

   if(!(Wnd = UserGetWindowObject(hWnd)))
   {
      goto Exit; // Return 0
   }

   Result = Wnd->strName.Length / sizeof(WCHAR);
   if(lpString)
   {
      const WCHAR Terminator = L'\0';
      INT Copy;
      WCHAR *Buffer = (WCHAR*)lpString;

      Copy = min(nMaxCount - 1, Result);
      if(Copy > 0)
      {
         Status = MmCopyToCaller(Buffer, Wnd->strName.Buffer, Copy * sizeof(WCHAR));
         if(!NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            Result = 0;
            goto Exit;
         }
         Buffer += Copy;
      }

      Status = MmCopyToCaller(Buffer, &Terminator, sizeof(WCHAR));
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         Result = 0;
         goto Exit;
      }

      Result = Copy;
   }

Exit:
   TRACE("Leave NtUserInternalGetWindowText, ret=%i\n", Result);
   UserLeave();
   return Result;
}

/*
  API Call
*/
BOOL
FASTCALL
IntShowOwnedPopups(PWND OwnerWnd, BOOL fShow )
{
   int count = 0;
   PWND pWnd;
   HWND *win_array;

//   ASSERT(OwnerWnd);

   TRACE("Enter ShowOwnedPopups Show: %s\n", (fShow ? "TRUE" : "FALSE"));

   /* NOTE: Popups are not children */
   win_array = IntWinListOwnedPopups(OwnerWnd);

   if (!win_array)
      return TRUE;

   while (win_array[count])
      count++;
   while (--count >= 0)
   {
      if (!(pWnd = ValidateHwndNoErr( win_array[count] )))
         continue;
      ASSERT(pWnd->spwndOwner == OwnerWnd);

      if (fShow)
      {
         if (pWnd->state & WNDS_HIDDENPOPUP)
         {
            /* In Windows, ShowOwnedPopups(TRUE) generates
             * WM_SHOWWINDOW messages with SW_PARENTOPENING,
             * regardless of the state of the owner
             */
            co_IntSendMessage(win_array[count], WM_SHOWWINDOW, SW_SHOWNORMAL, SW_PARENTOPENING);
            pWnd->state &= ~WNDS_HIDDENPOPUP;
            continue;
         }
      }
      else
      {
         if (pWnd->style & WS_VISIBLE)
         {
            /* In Windows, ShowOwnedPopups(FALSE) generates
             * WM_SHOWWINDOW messages with SW_PARENTCLOSING,
             * regardless of the state of the owner
             */
            co_IntSendMessage(win_array[count], WM_SHOWWINDOW, SW_HIDE, SW_PARENTCLOSING);
            pWnd->state |= WNDS_HIDDENPOPUP;
            continue;
         }
      }
   }
   ExFreePoolWithTag(win_array, USERTAG_WINDOWLIST);
   TRACE("Leave ShowOwnedPopups\n");
   return TRUE;
}

/* EOF */
