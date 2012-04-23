/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Windows
 * FILE:             subsystems/win32/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserWnd);

/* HELPER FUNCTIONS ***********************************************************/

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

/* Temp HACK */
PWND FASTCALL UserGetWindowObject(HWND hWnd)
{
    PWND Window;

   if (!hWnd)
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   Window = (PWND)UserGetObject(gHandleTable, hWnd, otWindow);
   if (!Window || 0 != (Window->state & WNDS_DESTROYED))
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   return Window;
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
      return FALSE;

   return TRUE;
}

BOOL FASTCALL
IntIsWindowVisible(PWND Wnd)
{
   BOOL Ret = TRUE;
   do
   {
      if (!(Wnd->style & WS_VISIBLE))
      {
         Ret = FALSE;
         break;
      }
      if (Wnd->spwndParent != NULL)
         Wnd = Wnd->spwndParent;
      else
         break;
   }
   while (Wnd != NULL);
   return Ret;
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
       pWnd->style &= ~WS_DISABLED;
    }
    else
    {
       Update = !bIsDisabled;

       co_IntSendMessage( hWnd, WM_CANCELMODE, 0, 0);

       /* Remove keyboard focus from that window if it had focus */
       if (hWnd == IntGetThreadFocusWindow())
       {
          co_UserSetFocus(NULL);
       }
       pWnd->style |= WS_DISABLED;
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
      ++NumChildren;

   List = ExAllocatePoolWithTag(PagedPool, (NumChildren + 1) * sizeof(HWND), USERTAG_WINDOWLIST);
   if(!List)
   {
      ERR("Failed to allocate memory for children array\n");
      EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   for (Child = Window->spwndChild, Index = 0;
         Child != NULL;
         Child = Child->spwndNext, ++Index)
      List[Index] = Child->head.h;
   List[Index] = NULL;

   return List;
}

/***********************************************************************
 *           IntSendDestroyMsg
 */
static void IntSendDestroyMsg(HWND hWnd)
{

   PWND Window;
#if 0 /* FIXME */

   GUITHREADINFO info;

   if (GetGUIThreadInfo(GetCurrentThreadId(), &info))
   {
      if (hWnd == info.hwndCaret)
      {
         DestroyCaret();
      }
   }
#endif

   Window = UserGetWindowObject(hWnd);
   if (Window)
   {
//      USER_REFERENCE_ENTRY Ref;
//      UserRefObjectCo(Window, &Ref);

      if (!Window->spwndOwner && !IntGetParent(Window))
      {
         co_IntShellHookNotify(HSHELL_WINDOWDESTROYED, (LPARAM) hWnd);
      }

//      UserDerefObjectCo(Window);
   }

   /* The window could already be destroyed here */

   /*
    * Send the WM_DESTROY to the window.
    */

   co_IntSendMessage(hWnd, WM_DESTROY, 0, 0);

   /*
    * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
    * make sure that the window still exists when we come back.
    */
#if 0 /* FIXME */

   if (IsWindow(Wnd))
   {
      HWND* pWndArray;
      int i;

      if (!(pWndArray = WIN_ListChildren( hwnd )))
         return;

      /* start from the end (FIXME: is this needed?) */
      for (i = 0; pWndArray[i]; i++)
         ;

      while (--i >= 0)
      {
         if (IsWindow( pWndArray[i] ))
            WIN_SendDestroyMsg( pWndArray[i] );
      }
      HeapFree(GetProcessHeap(), 0, pWndArray);
   }
   else
   {
      TRACE("destroyed itself while in WM_DESTROY!\n");
   }
#endif
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
//    WindowObject->Wnd = NULL;
}

/***********************************************************************
 *           IntDestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 *
 * This is the "functional" DestroyWindows function ei. all stuff
 * done in CreateWindow is undone here and not in DestroyWindow:-P

 */
static LRESULT co_UserFreeWindow(PWND Window,
                                   PPROCESSINFO ProcessData,
                                   PTHREADINFO ThreadData,
                                   BOOLEAN SendMessages)
{
   HWND *Children;
   HWND *ChildHandle;
   PWND Child;
   PMENU_OBJECT Menu;
   BOOLEAN BelongsToThreadData;

   ASSERT(Window);

   if(Window->state2 & WNDS2_INDESTROY)
   {
      TRACE("Tried to call IntDestroyWindow() twice\n");
      return 0;
   }
   Window->state2 |= WNDS2_INDESTROY;
   Window->style &= ~WS_VISIBLE;

   IntNotifyWinEvent(EVENT_OBJECT_DESTROY, Window, OBJID_WINDOW, CHILDID_SELF, 0);

   /* remove the window already at this point from the thread window list so we
      don't get into trouble when destroying the thread windows while we're still
      in IntDestroyWindow() */
   RemoveEntryList(&Window->ThreadListEntry);

   BelongsToThreadData = IntWndBelongsToThread(Window, ThreadData);

   IntDeRegisterShellHookWindow(Window->head.h);

   if(SendMessages)
   {
      /* Send destroy messages */
      IntSendDestroyMsg(Window->head.h);
   }

   /* free child windows */
   Children = IntWinListChildren(Window);
   if (Children)
   {
      for (ChildHandle = Children; *ChildHandle; ++ChildHandle)
      {
         if ((Child = IntGetWindowObject(*ChildHandle)))
         {
            if(!IntWndBelongsToThread(Child, ThreadData))
            {
               /* send WM_DESTROY messages to windows not belonging to the same thread */
               IntSendDestroyMsg(Child->head.h);
            }
            else
               co_UserFreeWindow(Child, ProcessData, ThreadData, SendMessages);

            UserDereferenceObject(Child);
         }
      }
      ExFreePool(Children);
   }

   if(SendMessages)
   {
      /*
       * Clear the update region to make sure no WM_PAINT messages will be
       * generated for this window while processing the WM_NCDESTROY.
       */
      co_UserRedrawWindow(Window, NULL, 0,
                          RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE |
                          RDW_NOINTERNALPAINT | RDW_NOCHILDREN);
      if(BelongsToThreadData)
         co_IntSendMessage(Window->head.h, WM_NCDESTROY, 0, 0);
   }

   DestroyTimersForWindow(ThreadData, Window);

   /* Unregister hot keys */
   UnregisterWindowHotKeys (Window);

   /* flush the message queue */
   MsqRemoveWindowMessagesFromQueue(Window);

   IntDereferenceMessageQueue(Window->head.pti->MessageQueue);

   /* from now on no messages can be sent to this window anymore */
   Window->state |= WNDS_DESTROYED;
   Window->fnid |= FNID_FREED;

   /* don't remove the WINDOWSTATUS_DESTROYING bit */

   /* reset shell window handles */
   if(ThreadData->rpdesk)
   {
      if (Window->head.h == ThreadData->rpdesk->rpwinstaParent->ShellWindow)
         ThreadData->rpdesk->rpwinstaParent->ShellWindow = NULL;

      if (Window->head.h == ThreadData->rpdesk->rpwinstaParent->ShellListView)
         ThreadData->rpdesk->rpwinstaParent->ShellListView = NULL;
   }

   /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

#if 0 /* FIXME */

   WinPosCheckInternalPos(Window->head.h);
   if (Window->head.h == GetCapture())
   {
      ReleaseCapture();
   }

   /* free resources associated with the window */
   TIMER_RemoveWindowTimers(Window->head.h);
#endif

   if ( ((Window->style & (WS_CHILD|WS_POPUP)) != WS_CHILD) &&
        Window->IDMenu &&
        (Menu = UserGetMenuObject((HMENU)Window->IDMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE, TRUE);
      Window->IDMenu = 0;
   }

   if(Window->SystemMenu
         && (Menu = UserGetMenuObject(Window->SystemMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE, TRUE);
      Window->SystemMenu = (HMENU)0;
   }

   DceFreeWindowDCE(Window);    /* Always do this to catch orphaned DCs */
#if 0 /* FIXME */

   WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
   CLASS_RemoveWindow(Window->Class);
#endif

   IntUnlinkWindow(Window);

   UserReferenceObject(Window);
   UserDeleteObject(Window->head.h, otWindow);

   IntDestroyScrollBars(Window);

   /* dereference the class */
   IntDereferenceClass(Window->pcls,
                       Window->head.pti->pDeskInfo,
                       Window->head.pti->ppi);
   Window->pcls = NULL;

   if(Window->hrgnClip)
   {
      IntGdiSetRegionOwner(Window->hrgnClip, GDI_OBJ_HMGR_POWNED);
      GreDeleteObject(Window->hrgnClip);
      Window->hrgnClip = NULL;
   }

//   ASSERT(Window != NULL);
   UserFreeWindowInfo(Window->head.pti, Window);

   UserDereferenceObject(Window);

   UserClipboardFreeWindow(Window);

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

   ASSERT(UserIsEnteredExclusive() == TRUE);

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
      CallProc = UserGetObject(gHandleTable, NewWndProc, otCallProc);
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

static BOOL FASTCALL
IntSetMenu(
   PWND Wnd,
   HMENU Menu,
   BOOL *Changed)
{
   PMENU_OBJECT OldMenu, NewMenu = NULL;

   if ((Wnd->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   *Changed = (Wnd->IDMenu != (UINT) Menu);
   if (! *Changed)
   {
      return TRUE;
   }

   if (Wnd->IDMenu)
   {
      OldMenu = IntGetMenuObject((HMENU) Wnd->IDMenu);
      ASSERT(NULL == OldMenu || OldMenu->MenuInfo.Wnd == Wnd->head.h);
   }
   else
   {
      OldMenu = NULL;
   }

   if (NULL != Menu)
   {
      NewMenu = IntGetMenuObject(Menu);
      if (NULL == NewMenu)
      {
         if (NULL != OldMenu)
         {
            IntReleaseMenuObject(OldMenu);
         }
         EngSetLastError(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }
      if (NULL != NewMenu->MenuInfo.Wnd)
      {
         /* Can't use the same menu for two windows */
         if (NULL != OldMenu)
         {
            IntReleaseMenuObject(OldMenu);
         }
         EngSetLastError(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }

   }

   Wnd->IDMenu = (UINT) Menu;
   if (NULL != NewMenu)
   {
      NewMenu->MenuInfo.Wnd = Wnd->head.h;
      IntReleaseMenuObject(NewMenu);
   }
   if (NULL != OldMenu)
   {
      OldMenu->MenuInfo.Wnd = NULL;
      IntReleaseMenuObject(OldMenu);
   }

   return TRUE;
}


/* INTERNAL ******************************************************************/


VOID FASTCALL
co_DestroyThreadWindows(struct _ETHREAD *Thread)
{
   PTHREADINFO WThread;
   PLIST_ENTRY Current;
   PWND Wnd;
   USER_REFERENCE_ENTRY Ref;
   WThread = (PTHREADINFO)Thread->Tcb.Win32Thread;

   while (!IsListEmpty(&WThread->WindowListHead))
   {
      Current = WThread->WindowListHead.Flink;
      Wnd = CONTAINING_RECORD(Current, WND, ThreadListEntry);

      TRACE("thread cleanup: while destroy wnds, wnd=0x%x\n",Wnd);

      /* Window removes itself from the list */

      /*
       * FIXME: It is critical that the window removes itself! If now, we will loop
       * here forever...
       */

      //ASSERT(co_UserDestroyWindow(Wnd));

      UserRefObjectCo(Wnd, &Ref); // FIXME: Temp HACK??
      if (!co_UserDestroyWindow(Wnd))
      {
         ERR("Unable to destroy window 0x%x at thread cleanup... This is _VERY_ bad!\n", Wnd);
      }
      UserDerefObjectCo(Wnd); // FIXME: Temp HACK??
   }
}

PMENU_OBJECT FASTCALL
IntGetSystemMenu(PWND Window, BOOL bRevert, BOOL RetMenu)
{
   PMENU_OBJECT Menu, NewMenu = NULL, SysMenu = NULL, ret = NULL;
   PTHREADINFO W32Thread;
   HMENU hNewMenu, hSysMenu;
   ROSMENUITEMINFO ItemInfo;

   if(bRevert)
   {
      W32Thread = PsGetCurrentThreadWin32Thread();

      if(!W32Thread->rpdesk)
         return NULL;

      if(Window->SystemMenu)
      {
         Menu = UserGetMenuObject(Window->SystemMenu);
         if(Menu)
         {
            IntDestroyMenuObject(Menu, TRUE, TRUE);
            Window->SystemMenu = (HMENU)0;
         }
      }

      if(W32Thread->rpdesk->rpwinstaParent->SystemMenuTemplate)
      {
         /* Clone system menu */
         Menu = UserGetMenuObject(W32Thread->rpdesk->rpwinstaParent->SystemMenuTemplate);
         if(!Menu)
            return NULL;

         NewMenu = IntCloneMenu(Menu);
         if(NewMenu)
         {
            Window->SystemMenu = NewMenu->MenuInfo.Self;
            NewMenu->MenuInfo.Flags |= MF_SYSMENU;
            NewMenu->MenuInfo.Wnd = Window->head.h;
            ret = NewMenu;
            //IntReleaseMenuObject(NewMenu);
         }
      }
      else
      {
         hSysMenu = UserCreateMenu(FALSE);
         if (NULL == hSysMenu)
         {
            return NULL;
         }
         SysMenu = IntGetMenuObject(hSysMenu);
         if (NULL == SysMenu)
         {
            UserDestroyMenu(hSysMenu);
            return NULL;
         }
         SysMenu->MenuInfo.Flags |= MF_SYSMENU;
         SysMenu->MenuInfo.Wnd = Window->head.h;
         hNewMenu = co_IntLoadSysMenuTemplate();
         if(!hNewMenu)
         {
            IntReleaseMenuObject(SysMenu);
            UserDestroyMenu(hSysMenu);
            return NULL;
         }
         Menu = IntGetMenuObject(hNewMenu);
         if(!Menu)
         {
            IntReleaseMenuObject(SysMenu);
            UserDestroyMenu(hSysMenu);
            return NULL;
         }

         NewMenu = IntCloneMenu(Menu);
         if(NewMenu)
         {
            NewMenu->MenuInfo.Flags |= MF_SYSMENU | MF_POPUP;
            IntReleaseMenuObject(NewMenu);
            UserSetMenuDefaultItem(NewMenu, SC_CLOSE, FALSE);

            ItemInfo.cbSize = sizeof(MENUITEMINFOW);
            ItemInfo.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_SUBMENU;
            ItemInfo.fType = MF_POPUP;
            ItemInfo.fState = MFS_ENABLED;
            ItemInfo.dwTypeData = NULL;
            ItemInfo.cch = 0;
            ItemInfo.hSubMenu = NewMenu->MenuInfo.Self;
            IntInsertMenuItem(SysMenu, (UINT) -1, TRUE, &ItemInfo);

            Window->SystemMenu = SysMenu->MenuInfo.Self;

            ret = SysMenu;
         }
         IntDestroyMenuObject(Menu, FALSE, TRUE);
      }
      if(RetMenu)
         return ret;
      else
         return NULL;
   }
   else
   {
      if(Window->SystemMenu)
         return IntGetMenuObject((HMENU)Window->SystemMenu);
      else
         return NULL;
   }
}


BOOL FASTCALL
IntIsChildWindow(PWND Parent, PWND BaseWindow)
{
   PWND Window;

   Window = BaseWindow;
   while (Window && ((Window->style & (WS_POPUP|WS_CHILD)) == WS_CHILD))
   {
      if (Window == Parent)
      {
         return(TRUE);
      }

      Window = Window->spwndParent;
   }

   return(FALSE);
}

/*
   Link the window into siblings list
   children and parent are kept in place.
*/
VOID FASTCALL
IntLinkWindow(
   PWND Wnd,
   PWND WndInsertAfter /* set to NULL if top sibling */
)
{
  if ((Wnd->spwndPrev = WndInsertAfter))
   {
      /* link after WndInsertAfter */
      if ((Wnd->spwndNext = WndInsertAfter->spwndNext))
         Wnd->spwndNext->spwndPrev = Wnd;

      Wnd->spwndPrev->spwndNext = Wnd;
   }
   else
   {
      /* link at top */
     if ((Wnd->spwndNext = Wnd->spwndParent->spwndChild))
         Wnd->spwndNext->spwndPrev = Wnd;

     Wnd->spwndParent->spwndChild = Wnd;
   }
}

/*
 Note: Wnd->spwndParent can be null if it is the desktop.
*/
VOID FASTCALL IntLinkHwnd(PWND Wnd, HWND hWndPrev)
{
    if (hWndPrev == HWND_NOTOPMOST)
    {
        if (!(Wnd->ExStyle & WS_EX_TOPMOST) &&
            (Wnd->ExStyle2 & WS_EX2_LINKED)) return;  /* nothing to do */
        Wnd->ExStyle &= ~WS_EX_TOPMOST;
        hWndPrev = HWND_TOP;  /* fallback to the HWND_TOP case */
    }

    IntUnlinkWindow(Wnd);  /* unlink it from the previous location */

    if (hWndPrev == HWND_BOTTOM)
    {
        /* Link in the bottom of the list */
        PWND WndInsertAfter;

        WndInsertAfter = Wnd->spwndParent->spwndChild;
        while( WndInsertAfter && WndInsertAfter->spwndNext)
            WndInsertAfter = WndInsertAfter->spwndNext;

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
                if (!(WndInsertBefore->ExStyle & WS_EX_TOPMOST)) break;
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
        if(WndInsertAfter == NULL)
        {
            /* Link in a default position */
            IntLinkHwnd(Wnd, HWND_TOP);
            return;
        }

        IntLinkWindow(Wnd, WndInsertAfter);

        /* Fix the WS_EX_TOPMOST flag */
        if (!(WndInsertAfter->ExStyle & WS_EX_TOPMOST))
        {
            Wnd->ExStyle &= ~WS_EX_TOPMOST;
        }
        else
        {
            if(WndInsertAfter->spwndNext &&
               WndInsertAfter->spwndNext->ExStyle & WS_EX_TOPMOST)
            {
                Wnd->ExStyle |= WS_EX_TOPMOST;
            }
        }
    }
}

HWND FASTCALL
IntSetOwner(HWND hWnd, HWND hWndNewOwner)
{
   PWND Wnd, WndOldOwner, WndNewOwner;
   HWND ret;

   Wnd = IntGetWindowObject(hWnd);
   if(!Wnd)
      return NULL;

   WndOldOwner = Wnd->spwndOwner;

   ret = WndOldOwner ? WndOldOwner->head.h : 0;

   if((WndNewOwner = UserGetWindowObject(hWndNewOwner)))
   {
       Wnd->spwndOwner= WndNewOwner;
   }
   else
   {
       Wnd->spwndOwner = NULL;
   }

   UserDereferenceObject(Wnd);
   return ret;
}

PWND FASTCALL
co_IntSetParent(PWND Wnd, PWND WndNewParent)
{
   PWND WndOldParent, pWndExam;
   BOOL WasVisible;

   ASSERT(Wnd);
   ASSERT(WndNewParent);
   ASSERT_REFS_CO(Wnd);
   ASSERT_REFS_CO(WndNewParent);

   if (Wnd == Wnd->head.rpdesk->spwndMessage)
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return( NULL);
   }

   /* Some applications try to set a child as a parent */
   if (IntIsChildWindow(Wnd, WndNewParent))
   {
      EngSetLastError( ERROR_INVALID_PARAMETER );
      return NULL;
   }

   pWndExam = WndNewParent; // Load parent Window to examine.
   // Now test for set parent to parent hit.
   while (pWndExam)
   {
      if (Wnd == pWndExam)
      {
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
   if (Wnd->head.pti->pEThread->ThreadsProcess != PsGetCurrentProcess())
      return NULL;

   WndOldParent = Wnd->spwndParent;

   if (WndOldParent) UserReferenceObject(WndOldParent); /* Caller must deref */

   if (WndNewParent != WndOldParent)
   {
      /* Unlink the window from the siblings list */
      IntUnlinkWindow(Wnd);

      /* Set the new parent */
      Wnd->spwndParent = WndNewParent;

      /* Link the window with its new siblings */
      IntLinkHwnd(Wnd, HWND_TOP);

   }

   IntNotifyWinEvent(EVENT_OBJECT_PARENTCHANGE, Wnd ,OBJID_WINDOW, CHILDID_SELF, WEF_SETBYWNDPTI);
   /*
    * SetParent additionally needs to make hwnd the top window
    * in the z-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   co_WinPosSetWindowPos(Wnd, (0 == (Wnd->ExStyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
                         0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
                         | (WasVisible ? SWP_SHOWWINDOW : 0));

   /*
    * FIXME: A WM_MOVE is also generated (in the DefWindowProc handler
    * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE.
    */
   if (WasVisible) co_WinPosShowWindow(Wnd, SW_SHOWNORMAL);

   return WndOldParent;
}

HWND FASTCALL
co_UserSetParent(HWND hWndChild, HWND hWndNewParent)
{
   PWND Wnd = NULL, WndParent = NULL, WndOldParent;
   HWND hWndOldParent = NULL;
   USER_REFERENCE_ENTRY Ref, ParentRef;

   if (IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return( NULL);
   }

   if (hWndChild == IntGetDesktopWindow())
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return( NULL);
   }

   if (hWndNewParent)
   {
      if (!(WndParent = UserGetWindowObject(hWndNewParent)))
      {
         return( NULL);
      }
   }
   else
   {
      if (!(WndParent = UserGetWindowObject(IntGetDesktopWindow())))
      {
         return( NULL);
      }
   }

   if (!(Wnd = UserGetWindowObject(hWndChild)))
   {
      return( NULL);
   }

   UserRefObjectCo(Wnd, &Ref);
   UserRefObjectCo(WndParent, &ParentRef);

   WndOldParent = co_IntSetParent(Wnd, WndParent);

   UserDerefObjectCo(WndParent);
   UserDerefObjectCo(Wnd);

   if (WndOldParent)
   {
      hWndOldParent = WndOldParent->head.h;
      UserDereferenceObject(WndOldParent);
   }

   return( hWndOldParent);
}

BOOL FASTCALL
IntSetSystemMenu(PWND Window, PMENU_OBJECT Menu)
{
   PMENU_OBJECT OldMenu;
   if(Window->SystemMenu)
   {
      OldMenu = IntGetMenuObject(Window->SystemMenu);
      if(OldMenu)
      {
         OldMenu->MenuInfo.Flags &= ~ MF_SYSMENU;
         IntReleaseMenuObject(OldMenu);
      }
   }

   if(Menu)
   {
      /* FIXME: Check window style, propably return FALSE? */
      Window->SystemMenu = Menu->MenuInfo.Self;
      Menu->MenuInfo.Flags |= MF_SYSMENU;
   }
   else
      Window->SystemMenu = (HMENU)0;

   return TRUE;
}

/* Unlink the window from siblings. children and parent are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWND Wnd)
{
   if (Wnd->spwndNext)
       Wnd->spwndNext->spwndPrev = Wnd->spwndPrev;

   if (Wnd->spwndPrev)
       Wnd->spwndPrev->spwndNext = Wnd->spwndNext;

   if (Wnd->spwndParent && Wnd->spwndParent->spwndChild == Wnd)
       Wnd->spwndParent->spwndChild = Wnd->spwndNext;

   Wnd->spwndPrev = Wnd->spwndNext = NULL;
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
APIENTRY
NtUserBuildHwndList(
   HDESK hDesktop,
   HWND hwndParent,
   BOOLEAN bChildren,
   ULONG dwThreadId,
   ULONG lParam,
   HWND* pWnd,
   ULONG* pBufSize)
{
   NTSTATUS Status;
   ULONG dwCount = 0;

   if (pBufSize == 0)
       return ERROR_INVALID_PARAMETER;

   if (hwndParent || !dwThreadId)
   {
      PDESKTOP Desktop;
      PWND Parent, Window;

      if(!hwndParent)
      {
         if(hDesktop == NULL && !(Desktop = IntGetActiveDesktop()))
         {
            return ERROR_INVALID_HANDLE;
         }

         if(hDesktop)
         {
            Status = IntValidateDesktopHandle(hDesktop,
                                              UserMode,
                                              0,
                                              &Desktop);
            if(!NT_SUCCESS(Status))
            {
               return ERROR_INVALID_HANDLE;
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
               if(dwCount++ < *pBufSize && pWnd)
               {
                  _SEH2_TRY
                  {
                     ProbeForWrite(pWnd, sizeof(HWND), 1);
                     *pWnd = Window->head.h;
                     pWnd++;
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                     Status = _SEH2_GetExceptionCode();
                  }
                  _SEH2_END
                  if(!NT_SUCCESS(Status))
                  {
                     SetLastNtError(Status);
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
      PLIST_ENTRY Current;
      PWND Window;

      Status = PsLookupThreadByThreadId((HANDLE)dwThreadId, &Thread);
      if (!NT_SUCCESS(Status))
      {
         ERR("Thread Id is not valid!\n");
         return ERROR_INVALID_PARAMETER;
      }
      if (!(W32Thread = (PTHREADINFO)Thread->Tcb.Win32Thread))
      {
         ObDereferenceObject(Thread);
         ERR("Thread is not initialized!\n");
         return ERROR_INVALID_PARAMETER;
      }

      Current = W32Thread->WindowListHead.Flink;
      while (Current != &(W32Thread->WindowListHead))
      {
         Window = CONTAINING_RECORD(Current, WND, ThreadListEntry);
         ASSERT(Window);

         if (dwCount < *pBufSize && pWnd)
         {
            _SEH2_TRY
            {
               ProbeForWrite(pWnd, sizeof(HWND), 1);
               *pWnd = Window->head.h;
               pWnd++;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
               Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END
            if (!NT_SUCCESS(Status))
            {
               ERR("Failure to build window list!\n");
               SetLastNtError(Status);
               break;
            }
         }
         dwCount++;
         Current = Window->ThreadListEntry.Flink;
      }

      ObDereferenceObject(Thread);
   }

   *pBufSize = dwCount;
   return STATUS_SUCCESS;
}

static void IntSendParentNotify( PWND pWindow, UINT msg )
{
    if ( (pWindow->style & (WS_CHILD | WS_POPUP)) == WS_CHILD &&
         !(pWindow->style & WS_EX_NOPARENTNOTIFY))
    {
        if (pWindow->spwndParent && pWindow->spwndParent != UserGetDesktopWindow())
        {
            USER_REFERENCE_ENTRY Ref;
            UserRefObjectCo(pWindow->spwndParent, &Ref);
            co_IntSendMessage( pWindow->spwndParent->head.h,
                               WM_PARENTNOTIFY,
                               MAKEWPARAM( msg, pWindow->IDMenu),
                               (LPARAM)pWindow->head.h );
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
                                        PVOID acbiBuffer)
{
   PWND pWnd = NULL;
   HWND hWnd;
   PTHREADINFO pti = NULL;
   PMENU_OBJECT SystemMenu;
   BOOL MenuChanged;
   BOOL bUnicodeWindow;

   pti = PsGetCurrentThreadWin32Thread();

   if (!(Cs->dwExStyle & WS_EX_LAYOUTRTL))
   {
      if (ParentWindow)
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
         if ( Class->fnid != FNID_DIALOG)
         {
            PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
            if (ppi->dwLayout & LAYOUT_RTL)
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
                                   pti->rpdesk,
                                  (PHANDLE)&hWnd,
                                   otWindow,
                                   sizeof(WND) + Class->cbwndExtra);

   if (!pWnd)
   {
      goto AllocError;
   }

   TRACE("Created object with handle %X\n", hWnd);

   if (NULL == pti->rpdesk->DesktopWindow)
   {  /* HACK: Helper for win32csr/desktopbg.c */
      /* If there is no desktop window yet, we must be creating it */
      pti->rpdesk->DesktopWindow = hWnd;
      pti->rpdesk->pDeskInfo->spwnd = pWnd;
   }

   /*
    * Fill out the structure describing it.
    */
   /* Remember, pWnd->head is setup in object.c ... */
   pWnd->spwndParent = ParentWindow;
   pWnd->spwndOwner = OwnerWindow;
   pWnd->fnid = 0;
   pWnd->spwndLastActive = pWnd;
   pWnd->state2 |= WNDS2_WIN40COMPAT; // FIXME!!!
   pWnd->pcls = Class;
   pWnd->hModule = Cs->hInstance;
   pWnd->style = Cs->style & ~WS_VISIBLE;
   pWnd->ExStyle = Cs->dwExStyle;
   pWnd->cbwndExtra = pWnd->pcls->cbwndExtra;
   pWnd->pActCtx = acbiBuffer;
   pWnd->InternalPos.MaxPos.x  = pWnd->InternalPos.MaxPos.y  = -1;
   pWnd->InternalPos.IconPos.x = pWnd->InternalPos.IconPos.y = -1;

   IntReferenceMessageQueue(pWnd->head.pti->MessageQueue);
   if (pWnd->spwndParent != NULL && Cs->hwndParent != 0)
   {
       pWnd->HideFocus = pWnd->spwndParent->HideFocus;
       pWnd->HideAccel = pWnd->spwndParent->HideAccel;
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
   pWnd->lpfnWndProc  = pWnd->pcls->lpfnWndProc;

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
      CallProc = CreateCallProc(NULL, pWnd->lpfnWndProc, pWnd->Unicode , pWnd->head.pti->ppi);

      if (!CallProc)
      {
         EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
         ERR("Warning: Unable to create CallProc for edit control. Control may not operate correctly! hwnd %x\n",hWnd);
      }
      else
      {
         UserAddCallProcToClass(pWnd->pcls, CallProc);
      }
   }

   InitializeListHead(&pWnd->PropListHead);

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

   /* Create system menu */
   if ((Cs->style & WS_SYSMENU)) // && (dwStyle & WS_CAPTION) == WS_CAPTION)
   {
      SystemMenu = IntGetSystemMenu(pWnd, TRUE, TRUE);
      if(SystemMenu)
      {
         pWnd->SystemMenu = SystemMenu->MenuInfo.Self;
         IntReleaseMenuObject(SystemMenu);
      }
   }

   /* Set the window menu */
   if ((Cs->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
   {
       if (Cs->hMenu)
         IntSetMenu(pWnd, Cs->hMenu, &MenuChanged);
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
      pWnd->IDMenu = (UINT) Cs->hMenu;

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
   if(pWnd)
      UserDereferenceObject(pWnd);

   SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
   return NULL;
}

/*
 * @implemented
 */
PWND FASTCALL
co_UserCreateWindowEx(CREATESTRUCTW* Cs,
                     PUNICODE_STRING ClassName,
                     PLARGE_STRING WindowName,
                     PVOID acbiBuffer)
{
   PWND Window = NULL, ParentWindow = NULL, OwnerWindow;
   HWND hWnd, hWndParent, hWndOwner, hwndInsertAfter;
   PWINSTATION_OBJECT WinSta;
   PCLS Class = NULL;
   SIZE Size;
   POINT MaxPos;
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
      ERR("Thread is not attached to a desktop! Cannot create window!\n");
      return NULL; // There is nothing to cleanup.
   }
   WinSta = pti->rpdesk->rpwinstaParent;
   ObReferenceObjectByPointer(WinSta, KernelMode, ExWindowStationObjectType, 0);

   pCsw = NULL;
   pCbtCreate = NULL;

   /* Get the class and reference it */
   Class = IntGetAndReferenceClass(ClassName, Cs->hInstance);
   if(!Class)
   {
       ERR("Failed to find class %wZ\n", ClassName);
       goto cleanup;
   }

   /* Now find the parent and the owner window */
   hWndParent = IntGetDesktopWindow();
   hWndOwner = NULL;

    if (Cs->hwndParent == HWND_MESSAGE)
    {
        Cs->hwndParent = hWndParent = IntGetMessageWindow();
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
         ERR("Cannot create a child window without a parrent!\n");
         EngSetLastError(ERROR_TLW_WITH_WSCHILD);
         goto cleanup;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
    }

    ParentWindow = hWndParent ? UserGetWindowObject(hWndParent): NULL;
    OwnerWindow = hWndOwner ? UserGetWindowObject(hWndOwner): NULL;

    /* FIXME: Is this correct? */
    if(OwnerWindow)
        OwnerWindow = UserGetAncestor(OwnerWindow, GA_ROOT);

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
                            acbiBuffer);
   if(!Window)
   {
       ERR("IntCreateWindow failed!\n");
       goto cleanup;
   }

   hWnd = UserHMGetHandle(Window);
   hwndInsertAfter = HWND_TOP;

   UserRefObjectCo(Window, &Ref);
   UserDereferenceObject(Window);
   ObDereferenceObject(WinSta);

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

      /* Fill the new CREATESTRUCTW */
      RtlCopyMemory(pCsw, Cs, sizeof(CREATESTRUCTW));
      pCsw->style = Window->style; /* HCBT_CREATEWND needs the real window style */

      // Based on the assumption this is from "unicode source" user32, ReactOS, answer is yes.
      if (!IS_ATOM(ClassName->Buffer))
      {
         if (Window->state & WNDS_ANSICREATOR)
         {
            ANSI_STRING AnsiString;
            AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(ClassName)+sizeof(CHAR);
            pszClass = UserHeapAlloc(AnsiString.MaximumLength);
            if (!pszClass)
            {
               ERR("UserHeapAlloc() failed!\n");
               goto cleanup;
            }
            RtlZeroMemory(pszClass, AnsiString.MaximumLength);
            AnsiString.Buffer = (PCHAR)pszClass;
            RtlUnicodeStringToAnsiString(&AnsiString, ClassName, FALSE);
         }
         else
         {
            UNICODE_STRING UnicodeString;
            UnicodeString.MaximumLength = ClassName->Length + sizeof(UNICODE_NULL);
            pszClass = UserHeapAlloc(UnicodeString.MaximumLength);
            if (!pszClass)
            {
               ERR("UserHeapAlloc() failed!\n");
               goto cleanup;
            }
            RtlZeroMemory(pszClass, UnicodeString.MaximumLength);
            UnicodeString.Buffer = (PWSTR)pszClass;
            RtlCopyUnicodeString(&UnicodeString, ClassName);
         }
         pCsw->lpszClass = UserHeapAddressToUser(pszClass);
      }
      if (WindowName->Length)
      {
         UNICODE_STRING Name;
         Name.Buffer = WindowName->Buffer;
         Name.Length = (USHORT)min(WindowName->Length, MAXUSHORT); // FIXME: LARGE_STRING truncated
         Name.MaximumLength = (USHORT)min(WindowName->MaximumLength, MAXUSHORT);

         if (Window->state & WNDS_ANSICREATOR)
         {
            ANSI_STRING AnsiString;
            AnsiString.MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize(&Name) + sizeof(CHAR);
            pszName = UserHeapAlloc(AnsiString.MaximumLength);
            if (!pszName)
            {
               ERR("UserHeapAlloc() failed!\n");
               goto cleanup;
            }
            RtlZeroMemory(pszName, AnsiString.MaximumLength);
            AnsiString.Buffer = (PCHAR)pszName;
            RtlUnicodeStringToAnsiString(&AnsiString, &Name, FALSE);
         }
         else
         {
            UNICODE_STRING UnicodeString;
            UnicodeString.MaximumLength = Name.Length + sizeof(UNICODE_NULL);
            pszName = UserHeapAlloc(UnicodeString.MaximumLength);
            if (!pszName)
            {
               ERR("UserHeapAlloc() failed!\n");
               goto cleanup;
            }
            RtlZeroMemory(pszName, UnicodeString.MaximumLength);
            UnicodeString.Buffer = (PWSTR)pszName;
            RtlCopyUnicodeString(&UnicodeString, &Name);
         }
         pCsw->lpszName = UserHeapAddressToUser(pszName);
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

   /* NCCREATE and WM_NCCALCSIZE need the original values */
   Cs->lpszName = (LPCWSTR) WindowName;
   Cs->lpszClass = (LPCWSTR) ClassName;

   /* Send the WM_GETMINMAXINFO message */
   Size.cx = Cs->cx;
   Size.cy = Cs->cy;

   if ((Cs->style & WS_THICKFRAME) || !(Cs->style & (WS_POPUP | WS_CHILD)))
   {
      POINT MaxSize, MaxPos, MinTrack, MaxTrack;

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
   if (0 != (Window->style & WS_CHILD) && ParentWindow)
   {
//      ERR("co_UserCreateWindowEx(): Offset rcWindow\n");
      RECTL_vOffsetRect(&Window->rcWindow,
                        ParentWindow->rcClient.left,
                        ParentWindow->rcClient.top);
   }
   Window->rcClient = Window->rcWindow;

   /* Link the window */
   if (NULL != ParentWindow)
   {
      /* Link the window into the siblings list */
      if ((Cs->style & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
          IntLinkHwnd(Window, HWND_BOTTOM);
      else
          IntLinkHwnd(Window, hwndInsertAfter);
   }

   /* Send the NCCREATE message */
   Result = co_IntSendMessage(UserHMGetHandle(Window), WM_NCCREATE, 0, (LPARAM) Cs);
   if (!Result)
   {
      ERR("co_UserCreateWindowEx(): NCCREATE message failed\n");
      goto cleanup;
   }

   /* Send the WM_NCCALCSIZE message */
   MaxPos.x = Window->rcWindow.left;
   MaxPos.y = Window->rcWindow.top;

   Result = co_WinPosGetNonClientSize(Window, &Window->rcWindow, &Window->rcClient);

   RECTL_vOffsetRect(&Window->rcWindow, MaxPos.x - Window->rcWindow.left,
                                     MaxPos.y - Window->rcWindow.top);


   /* Send the WM_CREATE message. */
   Result = co_IntSendMessage(UserHMGetHandle(Window), WM_CREATE, 0, (LPARAM) Cs);
   if (Result == (LRESULT)-1)
   {
      ERR("co_UserCreateWindowEx(): WM_CREATE message failed\n");
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
   if (Window->style & (WS_MINIMIZE | WS_MAXIMIZE))
   {
      RECTL NewPos;
      UINT16 SwFlag;

      SwFlag = (Window->style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;

      co_WinPosMinMaximize(Window, SwFlag, &NewPos);

      SwFlag = ((Window->style & WS_CHILD) || UserGetActiveWindow()) ?
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
                SWP_NOZORDER | SWP_FRAMECHANGED;

      co_WinPosSetWindowPos(Window, 0, NewPos.left, NewPos.top,
                            NewPos.right, NewPos.bottom, SwFlag);
   }

   /* Send the WM_PARENTNOTIFY message */
   IntSendParentNotify(Window, WM_CREATE);

   /* Notify the shell that a new window was created */
   if ((!hWndParent) && (!hWndOwner))
   {
      co_IntShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)hWnd);
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

   TRACE("co_UserCreateWindowEx(): Created window %X\n", hWnd);
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
            ExFreePool(pvBuffer);
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
    UNICODE_STRING ustrClassName;
    CREATESTRUCTW Cs;
    HWND hwnd = NULL;
    PWND pwnd;

    lstrWindowName.Buffer = NULL;
    lstrClassName.Buffer = NULL;

    ASSERT(plstrWindowName);

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
    if (IS_ATOM(plstrClassName))
       Cs.lpszClass = (LPCWSTR) plstrClassName;
    else
       Cs.lpszClass = (LPCWSTR) plstrClassName->Buffer;
    Cs.dwExStyle = dwExStyle;

    UserEnterExclusive();

    /* Call the internal function */
    pwnd = co_UserCreateWindowEx(&Cs, &ustrClassName, plstrWindowName, acbiBuffer);

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

   return hwnd;
}


BOOLEAN FASTCALL co_UserDestroyWindow(PWND Window)
{
   HWND hWnd;
   PTHREADINFO ti;
   MSG msg;

   ASSERT_REFS_CO(Window); // FIXME: Temp HACK?

   hWnd = Window->head.h;

   TRACE("co_UserDestroyWindow \n");

   /* Check for owner thread */
   if ( (Window->head.pti->pEThread != PsGetCurrentThread()) ||
        Window->head.pti != PsGetCurrentThreadWin32Thread() )
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return FALSE;
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

   /* Inform the parent */
   if (Window->style & WS_CHILD)
   {
      IntSendParentNotify(Window, WM_DESTROY);
   }

   /* Look whether the focus is within the tree of windows we will
    * be destroying.
    */
   if (!co_WinPosShowWindow(Window, SW_HIDE))
   {
      if (UserGetActiveWindow() == Window->head.h)
      {
         co_WinPosActivateOtherWindow(Window);
      }
   }

   if (Window->head.pti->MessageQueue->spwndActive == Window)
      Window->head.pti->MessageQueue->spwndActive = NULL;
   if (Window->head.pti->MessageQueue->spwndFocus == Window)
      Window->head.pti->MessageQueue->spwndFocus = NULL;
   if (Window->head.pti->MessageQueue->CaptureWindow == Window->head.h)
      Window->head.pti->MessageQueue->CaptureWindow = NULL;

   /*
    * Check if this window is the Shell's Desktop Window. If so set hShellWindow to NULL
    */

   ti = PsGetCurrentThreadWin32Thread();

   if ((ti != NULL) & (ti->pDeskInfo != NULL))
   {
      if (ti->pDeskInfo->hShellWindow == hWnd)
      {
         ERR("Destroying the ShellWindow!\n");
         ti->pDeskInfo->hShellWindow = NULL;
      }
   }

   IntEngWindowChanged(Window, WOC_DELETE);

   if (!IntIsWindow(Window->head.h))
   {
      return TRUE;
   }

   /* Recursively destroy owned windows */

   if (! (Window->style & WS_CHILD))
   {
      for (;;)
      {
         BOOL GotOne = FALSE;
         HWND *Children;
         HWND *ChildHandle;
         PWND Child, Desktop;

         Desktop = IntIsDesktopWindow(Window) ? Window :
                   UserGetWindowObject(IntGetDesktopWindow());
         Children = IntWinListChildren(Desktop);

         if (Children)
         {
            for (ChildHandle = Children; *ChildHandle; ++ChildHandle)
            {
               Child = UserGetWindowObject(*ChildHandle);
               if (Child == NULL)
                  continue;
               if (Child->spwndOwner != Window)
               {
                  continue;
               }

               if (IntWndBelongsToThread(Child, PsGetCurrentThreadWin32Thread()))
               {
                  USER_REFERENCE_ENTRY ChildRef;
                  UserRefObjectCo(Child, &ChildRef); // Temp HACK?
                  co_UserDestroyWindow(Child);
                  UserDerefObjectCo(Child); // Temp HACK?

                  GotOne = TRUE;
                  continue;
               }

               if (Child->spwndOwner != NULL)
               {
                  Child->spwndOwner = NULL;
               }

            }
            ExFreePool(Children);
         }
         if (! GotOne)
         {
            break;
         }
      }
   }

    /* Generate mouse move message for the next window */
    msg.message = WM_MOUSEMOVE;
    msg.wParam = UserGetMouseButtonsState();
    msg.lParam = MAKELPARAM(gpsi->ptCursor.x, gpsi->ptCursor.y);
    msg.pt = gpsi->ptCursor;
    co_MsqInsertMouseMessage(&msg, 0, 0, TRUE);

   if (!IntIsWindow(Window->head.h))
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
   DECLARE_RETURN(BOOLEAN);
   BOOLEAN ret;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserDestroyWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(Wnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref); // FIXME: Dunno if win should be reffed during destroy...
   ret = co_UserDestroyWindow(Window);
   UserDerefObjectCo(Window); // FIXME: Dunno if win should be reffed during destroy...

   RETURN(ret);

CLEANUP:
   TRACE("Leave NtUserDestroyWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


static HWND FASTCALL
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

   CheckWindowName = WindowName->Length != 0;

   if((List = IntWinListChildren(Parent)))
   {
      phWnd = List;
      if(ChildAfter)
      {
         /* skip handles before and including ChildAfter */
         while(*phWnd && (*(phWnd++) != ChildAfter->head.h))
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
         if (!ClassAtom || Child->pcls->atomClassName == ClassAtom)
         {
             // FIXME: LARGE_STRING truncated
             CurrentWindowName.Buffer = Child->strName.Buffer;
             CurrentWindowName.Length = (USHORT)min(Child->strName.Length, MAXUSHORT);
             CurrentWindowName.MaximumLength = (USHORT)min(Child->strName.MaximumLength, MAXUSHORT);
             if(!CheckWindowName ||
                (Child->strName.Length < 0xFFFF &&
                 !RtlCompareUnicodeString(WindowName, &CurrentWindowName, TRUE)))
             {
            Ret = Child->head.h;
            break;
         }
      }
      }
      ExFreePool(List);
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
   RTL_ATOM ClassAtom = (RTL_ATOM)0;
   DECLARE_RETURN(HWND);

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
           _SEH2_YIELD(RETURN(NULL));
       }
       _SEH2_END;

       if (ucClassName != NULL)
       {
           if (ClassName.Length == 0 && ClassName.Buffer != NULL &&
               !IS_ATOM(ClassName.Buffer))
           {
               EngSetLastError(ERROR_INVALID_PARAMETER);
               RETURN(NULL);
           }
           else if (ClassAtom == (RTL_ATOM)0)
           {
               /* LastError code was set by IntGetAtomFromStringOrAtom */
               RETURN(NULL);
           }
       }
   }

   Desktop = IntGetCurrentThreadDesktopWindow();

   if(hwndParent == NULL)
      hwndParent = Desktop;
   else if(hwndParent == HWND_MESSAGE)
   {
     hwndParent = IntGetMessageWindow();
   }

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      RETURN( NULL);
   }

   ChildAfter = NULL;
   if(hwndChildAfter && !(ChildAfter = UserGetWindowObject(hwndChildAfter)))
   {
      RETURN( NULL);
   }

   _SEH2_TRY
   {
       if(Parent->head.h == Desktop)
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
                while(*phWnd && (*(phWnd++) != ChildAfter->head.h))
                   ;
             }

             CheckWindowName = WindowName.Length != 0;

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
                               ClassAtom == TopLevelWindow->pcls->atomClassName;

                if (WindowMatches && ClassMatches)
                {
                   Ret = TopLevelWindow->head.h;
                   break;
                }

                if (IntFindWindow(TopLevelWindow, NULL, ClassAtom, &WindowName))
                {
                   /* window returns the handle of the top-level window, in case it found
                      the child window */
                   Ret = TopLevelWindow->head.h;
                   break;
                }

             }
             ExFreePool(List);
          }
       }
       else
          Ret = IntFindWindow(Parent, ChildAfter, ClassAtom, &WindowName);

#if 0

       if(Ret == NULL && hwndParent == NULL && hwndChildAfter == NULL)
       {
          /* FIXME:  If both hwndParent and hwndChildAfter are NULL, we also should
                     search the message-only windows. Should this also be done if
                     Parent is the desktop window??? */
          PWND MsgWindows;

          if((MsgWindows = UserGetWindowObject(IntGetMessageWindow())))
          {
             Ret = IntFindWindow(MsgWindows, ChildAfter, ClassAtom, &WindowName);
          }
       }
#endif
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       Ret = NULL;
   }
   _SEH2_END;

   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserFindWindowEx, ret %i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
PWND FASTCALL UserGetAncestor(PWND Wnd, UINT Type)
{
   PWND WndAncestor, Parent;

   if (Wnd->head.h == IntGetDesktopWindow())
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
               PWND Parent;

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
   DECLARE_RETURN(HWND);

   TRACE("Enter NtUserGetAncestor\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(NULL);
   }

   Ancestor = UserGetAncestor(Window, Type);
   /* faxme: can UserGetAncestor ever return NULL for a valid window? */

   RETURN(Ancestor ? Ancestor->head.h : NULL);

CLEANUP:
   TRACE("Leave NtUserGetAncestor, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
APIENTRY
NtUserGetComboBoxInfo(
   HWND hWnd,
   PCOMBOBOXINFO pcbi)
{
   PWND Wnd;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserGetComboBoxInfo\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE );
   }
   _SEH2_TRY
   {
       if(pcbi)
       {
          ProbeForWrite(pcbi,
                        sizeof(COMBOBOXINFO),
                        1);
       }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       _SEH2_YIELD(RETURN(FALSE));
   }
   _SEH2_END;

   // Pass the user pointer, it was already probed.
   RETURN( (BOOL) co_IntSendMessage( Wnd->head.h, CB_GETCOMBOBOXINFO, 0, (LPARAM)pcbi));

CLEANUP:
   TRACE("Leave NtUserGetComboBoxInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD
APIENTRY
NtUserGetListBoxInfo(
   HWND hWnd)
{
   PWND Wnd;
   DECLARE_RETURN(DWORD);

   TRACE("Enter NtUserGetListBoxInfo\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( 0 );
   }

   RETURN( (DWORD) co_IntSendMessage( Wnd->head.h, LB_GETLISTBOXINFO, 0, 0 ));

CLEANUP:
   TRACE("Leave NtUserGetListBoxInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   DECLARE_RETURN(HWND);

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

   RETURN( co_UserSetParent(hWndChild, hWndNewParent));

CLEANUP:
   TRACE("Leave NtUserSetParent, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
                     KernelMode,
                     0,
                     &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return( (HWND)0);
   }

   Ret = (HWND)WinStaObject->ShellWindow;

   ObDereferenceObject(WinStaObject);
   return( Ret);
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
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;
   NTSTATUS Status;
   PTHREADINFO ti;

   TRACE("Enter NtUserSetShellWindowEx\n");
   UserEnterExclusive();

   if (!(WndShell = UserGetWindowObject(hwndShell)))
   {
      RETURN(FALSE);
   }

   if(!(WndListView = UserGetWindowObject(hwndListView)))
   {
      RETURN(FALSE);
   }

   Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     KernelMode,
                     0,
                     &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   /*
    * Test if we are permitted to change the shell window.
    */
   if (WinStaObject->ShellWindow)
   {
      ObDereferenceObject(WinStaObject);
      RETURN( FALSE);
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
      co_WinPosSetWindowPos(hwndListView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif

      if (WndListView->ExStyle & WS_EX_TOPMOST)
      {
         ObDereferenceObject(WinStaObject);
         RETURN( FALSE);
      }
   }

   if (WndShell->ExStyle & WS_EX_TOPMOST)
   {
      ObDereferenceObject(WinStaObject);
      RETURN( FALSE);
   }

   UserRefObjectCo(WndShell, &Ref);
   co_WinPosSetWindowPos(WndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

   WinStaObject->ShellWindow = hwndShell;
   WinStaObject->ShellListView = hwndListView;

   ti = GetW32ThreadInfo();
   if (ti->pDeskInfo)
   {
       ti->pDeskInfo->hShellWindow = hwndShell;
       ti->pDeskInfo->ppiShellProcess = ti->ppi;
   }

   UserDerefObjectCo(WndShell);

   ObDereferenceObject(WinStaObject);
   RETURN( TRUE);

CLEANUP:
   TRACE("Leave NtUserSetShellWindowEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserGetSystemMenu
 *
 * The NtUserGetSystemMenu function allows the application to access the
 * window menu (also known as the system menu or the control menu) for
 * copying and modifying.
 *
 * Parameters
 *    hWnd
 *       Handle to the window that will own a copy of the window menu.
 *    bRevert
 *       Specifies the action to be taken. If this parameter is FALSE,
 *       NtUserGetSystemMenu returns a handle to the copy of the window menu
 *       currently in use. The copy is initially identical to the window menu
 *       but it can be modified.
 *       If this parameter is TRUE, GetSystemMenu resets the window menu back
 *       to the default state. The previous window menu, if any, is destroyed.
 *
 * Return Value
 *    If the bRevert parameter is FALSE, the return value is a handle to a
 *    copy of the window menu. If the bRevert parameter is TRUE, the return
 *    value is NULL.
 *
 * Status
 *    @implemented
 */

HMENU APIENTRY
NtUserGetSystemMenu(HWND hWnd, BOOL bRevert)
{
   PWND Window;
   PMENU_OBJECT Menu;
   DECLARE_RETURN(HMENU);

   TRACE("Enter NtUserGetSystemMenu\n");
   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(NULL);
   }

   if (!(Menu = IntGetSystemMenu(Window, bRevert, FALSE)))
   {
      RETURN(NULL);
   }

   RETURN(Menu->MenuInfo.Self);

CLEANUP:
   TRACE("Leave NtUserGetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserSetSystemMenu
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserSetSystemMenu(HWND hWnd, HMENU hMenu)
{
   BOOL Result = FALSE;
   PWND Window;
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetSystemMenu\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if (hMenu)
   {
      /*
       * Assign new menu handle.
       */
      if (!(Menu = UserGetMenuObject(hMenu)))
      {
         RETURN( FALSE);
      }

      Result = IntSetSystemMenu(Window, Menu);
   }

   RETURN( Result);

CLEANUP:
   TRACE("Leave NtUserSetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

LONG FASTCALL
co_UserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   PWND Window, Parent;
   PWINSTATION_OBJECT WindowStation;
   LONG OldValue;
   STYLESTRUCT Style;

   if (hWnd == IntGetDesktopWindow())
   {
      EngSetLastError(STATUS_ACCESS_DENIED);
      return( 0);
   }

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      return( 0);
   }

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Window->cbwndExtra)
      {
         EngSetLastError(ERROR_INVALID_INDEX);
         return( 0);
      }

      OldValue = *((LONG *)((PCHAR)(Window + 1) + Index));
/*
      if ( Index == DWLP_DLGPROC && Wnd->state & WNDS_DIALOGWINDOW)
      {
         OldValue = (LONG)IntSetWindowProc( Wnd,
                                           (WNDPROC)NewValue,
                                            Ansi);
         if (!OldValue) return 0;
      }
*/
      *((LONG *)((PCHAR)(Window + 1) + Index)) = NewValue;
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            OldValue = (LONG) Window->ExStyle;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;

            /*
             * Remove extended window style bit WS_EX_TOPMOST for shell windows.
             */
            WindowStation = Window->head.pti->rpdesk->rpwinstaParent;
            if(WindowStation)
            {
               if (hWnd == WindowStation->ShellWindow || hWnd == WindowStation->ShellListView)
                  Style.styleNew &= ~WS_EX_TOPMOST;
            }

            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM) &Style);
            Window->ExStyle = (DWORD)Style.styleNew;
            co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_EXSTYLE, (LPARAM) &Style);
            break;

         case GWL_STYLE:
            OldValue = (LONG) Window->style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);
            Window->style = (DWORD)Style.styleNew;
            co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_STYLE, (LPARAM) &Style);
            break;

         case GWL_WNDPROC:
         {
            if ( Window->head.pti->ppi != PsGetCurrentProcessWin32Process() ||
                 Window->fnid & FNID_FREED)
            {
               EngSetLastError(ERROR_ACCESS_DENIED);
               return( 0);
            }
            OldValue = (LONG)IntSetWindowProc(Window,
                                              (WNDPROC)NewValue,
                                              Ansi);
            break;
         }

         case GWL_HINSTANCE:
            OldValue = (LONG) Window->hModule;
            Window->hModule = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            Parent = Window->spwndParent;
            if (Parent && (Parent->head.h == IntGetDesktopWindow()))
               OldValue = (LONG) IntSetOwner(Window->head.h, (HWND) NewValue);
            else
               OldValue = (LONG) co_UserSetParent(Window->head.h, (HWND) NewValue);
            break;

         case GWL_ID:
            OldValue = (LONG) Window->IDMenu;
            Window->IDMenu = (UINT) NewValue;
            break;

         case GWL_USERDATA:
            OldValue = Window->dwUserData;
            Window->dwUserData = NewValue;
            break;

         default:
            ERR("NtUserSetWindowLong(): Unsupported index %d\n", Index);
            EngSetLastError(ERROR_INVALID_INDEX);
            OldValue = 0;
            break;
      }
   }

   return( OldValue);
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
   DECLARE_RETURN(LONG);

   TRACE("Enter NtUserSetWindowLong\n");
   UserEnterExclusive();

   RETURN( co_UserSetWindowLong(hWnd, Index, NewValue, Ansi));

CLEANUP:
   TRACE("Leave NtUserSetWindowLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   DECLARE_RETURN(WORD);

   TRACE("Enter NtUserSetWindowWord\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   switch (Index)
   {
      case GWL_ID:
      case GWL_HINSTANCE:
      case GWL_HWNDPARENT:
         RETURN( (WORD)co_UserSetWindowLong(Window->head.h, Index, (UINT)NewValue, TRUE));
      default:
         if (Index < 0)
         {
            EngSetLastError(ERROR_INVALID_INDEX);
            RETURN( 0);
         }
   }

   if ((ULONG)Index > (Window->cbwndExtra - sizeof(WORD)))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   OldValue = *((WORD *)((PCHAR)(Window + 1) + Index));
   *((WORD *)((PCHAR)(Window + 1) + Index)) = NewValue;

   RETURN( OldValue);

CLEANUP:
   TRACE("Leave NtUserSetWindowWord, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
DWORD APIENTRY
NtUserQueryWindow(HWND hWnd, DWORD Index)
{
   PWND pWnd;
   DWORD Result;
   DECLARE_RETURN(UINT);

   TRACE("Enter NtUserQueryWindow\n");
   UserEnterShared();

   if (!(pWnd = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   switch(Index)
   {
      case QUERY_WINDOW_UNIQUE_PROCESS_ID:
         Result = (DWORD)IntGetWndProcessId(pWnd);
         break;

      case QUERY_WINDOW_UNIQUE_THREAD_ID:
         Result = (DWORD)IntGetWndThreadId(pWnd);
         break;

      case QUERY_WINDOW_ACTIVE:
         Result = (DWORD)(pWnd->head.pti->MessageQueue->spwndActive ? UserHMGetHandle(pWnd->head.pti->MessageQueue->spwndActive) : 0);
         break;

      case QUERY_WINDOW_FOCUS:
         Result = (DWORD)(pWnd->head.pti->MessageQueue->spwndFocus ? UserHMGetHandle(pWnd->head.pti->MessageQueue->spwndFocus) : 0);
         break;

      case QUERY_WINDOW_ISHUNG:
         Result = (DWORD)MsqIsHung(pWnd->head.pti->MessageQueue);
         break;

      case QUERY_WINDOW_REAL_ID:
         Result = (DWORD)pWnd->head.pti->pEThread->Cid.UniqueProcess;
         break;

      default:
         Result = (DWORD)NULL;
         break;
   }

   RETURN( Result);

CLEANUP:
   TRACE("Leave NtUserQueryWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
UINT APIENTRY
NtUserRegisterWindowMessage(PUNICODE_STRING MessageNameUnsafe)
{
   UNICODE_STRING SafeMessageName;
   NTSTATUS Status;
   UINT Ret;
   DECLARE_RETURN(UINT);

   TRACE("Enter NtUserRegisterWindowMessage\n");
   UserEnterExclusive();

   if(MessageNameUnsafe == NULL)
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   Status = IntSafeCopyUnicodeStringTerminateNULL(&SafeMessageName, MessageNameUnsafe);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   Ret = (UINT)IntAddAtom(SafeMessageName.Buffer);
   if (SafeMessageName.Buffer)
      ExFreePoolWithTag(SafeMessageName.Buffer, TAG_STRING);
   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserRegisterWindowMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetMenu(
   HWND hWnd,
   HMENU Menu,
   BOOL Repaint)
{
   PWND Window;
   BOOL Changed;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetMenu\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if (! IntSetMenu(Window, Menu, &Changed))
   {
      RETURN( FALSE);
   }

   if (Changed && Repaint)
   {
      USER_REFERENCE_ENTRY Ref;

      UserRefObjectCo(Window, &Ref);
      co_WinPosSetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                            SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);

      UserDerefObjectCo(Window);
   }

   RETURN( TRUE);

CLEANUP:
   TRACE("Leave NtUserSetMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowFNID(HWND hWnd,
                    WORD fnID)
{
   PWND Wnd;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetWindowFNID\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if (Wnd->head.pti->ppi != PsGetCurrentProcessWin32Process())
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      RETURN( FALSE);
   }

   // From user land we only set these.
   if (fnID != FNID_DESTROY)
   { //       Hacked so we can mark desktop~!
      if ( (/*(fnID < FNID_BUTTON)*/ (fnID < FNID_FIRST) && (fnID > FNID_GHOST)) ||
           Wnd->fnid != 0 )
      {
         EngSetLastError(ERROR_INVALID_PARAMETER);
         RETURN( FALSE);
      }
   }

   Wnd->fnid |= fnID;
   RETURN( TRUE);

CLEANUP:
   TRACE("Leave NtUserSetWindowFNID\n");
   UserLeave();
   END_CLEANUP;
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
      co_IntShellHookNotify(HSHELL_REDRAW, (LPARAM) hWnd);
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
   INT Result;
   DECLARE_RETURN(INT);

   TRACE("Enter NtUserInternalGetWindowText\n");
   UserEnterShared();

   if(lpString && (nMaxCount <= 1))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   if(!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
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
            RETURN( 0);
         }
         Buffer += Copy;
      }

      Status = MmCopyToCaller(Buffer, &Terminator, sizeof(WCHAR));
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( 0);
      }

      Result = Copy;
   }

   RETURN( Result);

CLEANUP:
   TRACE("Leave NtUserInternalGetWindowText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
FASTCALL
IntShowOwnedPopups(PWND OwnerWnd, BOOL fShow )
{
   int count = 0;
   PWND pWnd;
   HWND *win_array;

//   ASSERT(OwnerWnd);

   win_array = IntWinListChildren(UserGetWindowObject(IntGetDesktopWindow()));

   if (!win_array)
      return TRUE;

   while (win_array[count])
      count++;
   while (--count >= 0)
   {
      if (!(pWnd = UserGetWindowObject( win_array[count] )))
         continue;
      if (pWnd->spwndOwner != OwnerWnd)
         continue;

      if (fShow)
      {
         if (pWnd->state & WNDS_HIDDENPOPUP)
         {
            /* In Windows, ShowOwnedPopups(TRUE) generates
             * WM_SHOWWINDOW messages with SW_PARENTOPENING,
             * regardless of the state of the owner
             */
            co_IntSendMessage(win_array[count], WM_SHOWWINDOW, SW_SHOWNORMAL, SW_PARENTOPENING);
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
            continue;
         }
      }

   }
   ExFreePool( win_array );
   return TRUE;
}

/* EOF */
