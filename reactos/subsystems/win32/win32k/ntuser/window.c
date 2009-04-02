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
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>


/* dialog resources appear to pass this in 16 bits, handle them properly */
#define CW_USEDEFAULT16 (0x8000)

#define POINT_IN_RECT(p, r) (((r.bottom >= p.y) && (r.top <= p.y))&&((r.left <= p.x )&&( r.right >= p.x )))

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * InitWindowImpl
 *
 * Initialize windowing implementation.
 */

NTSTATUS FASTCALL
InitWindowImpl(VOID)
{
   return STATUS_SUCCESS;
}

/*
 * CleanupWindowImpl
 *
 * Cleanup windowing implementation.
 */

NTSTATUS FASTCALL
CleanupWindowImpl(VOID)
{
   return STATUS_SUCCESS;
}

/* HELPER FUNCTIONS ***********************************************************/

BOOL FASTCALL UserUpdateUiState(PWINDOW Wnd, WPARAM wParam)
{
    WORD Action = LOWORD(wParam);
    WORD Flags = HIWORD(wParam);

    if (Flags & ~(UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE))
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (Action)
    {
        case UIS_INITIALIZE:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
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

PWINDOW_OBJECT FASTCALL IntGetWindowObject(HWND hWnd)
{
   PWINDOW_OBJECT Window;

   if (!hWnd) return NULL;

   Window = UserGetWindowObject(hWnd);
   if (Window)
   {
      ASSERT(USER_BODY_TO_HEADER(Window)->RefCount >= 0);

      USER_BODY_TO_HEADER(Window)->RefCount++;
   }
   return Window;
}

/* temp hack */
PWINDOW_OBJECT FASTCALL UserGetWindowObject(HWND hWnd)
{
   PW32THREADINFO ti;
   PWINDOW_OBJECT Window;

   if (PsGetCurrentProcess() != PsInitialSystemProcess)
   {
       ti = GetW32ThreadInfo();
       if (ti == NULL)
       {
          SetLastWin32Error(ERROR_ACCESS_DENIED);
          return NULL;
       }
   }

   if (!hWnd)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   Window = (PWINDOW_OBJECT)UserGetObject(gHandleTable, hWnd, otWindow);
   if (!Window || 0 != (Window->Status & WINDOWSTATUS_DESTROYED))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   ASSERT(USER_BODY_TO_HEADER(Window)->RefCount >= 0);
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
   PWINDOW_OBJECT Window;

   if (!(Window = UserGetWindowObject(hWnd)))
      return FALSE;

   return TRUE;
}



/*
  Caller must NOT dereference retval!
  But if caller want the returned value to persist spanning a co_ call,
  it must reference the value (because the owner is not garanteed to
  exist just because the owned window exist)!
*/
PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd)
{
   if (Wnd->Wnd->Style & WS_POPUP)
   {
      return UserGetWindowObject(Wnd->hOwner);
   }
   else if (Wnd->Wnd->Style & WS_CHILD)
   {
      return Wnd->Parent;
   }

   return NULL;
}


/*
  Caller must NOT dereference retval!
  But if caller want the returned value to persist spanning a co_ call,
  it must reference the value (because the owner is not garanteed to
  exist just because the owned window exist)!
*/
PWINDOW_OBJECT FASTCALL
IntGetOwner(PWINDOW_OBJECT Wnd)
{
   return UserGetWindowObject(Wnd->hOwner);
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
IntWinListChildren(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT Child;
   HWND *List;
   UINT Index, NumChildren = 0;

   for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
      ++NumChildren;

   List = ExAllocatePoolWithTag(PagedPool, (NumChildren + 1) * sizeof(HWND), TAG_WINLIST);
   if(!List)
   {
      DPRINT1("Failed to allocate memory for children array\n");
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   for (Child = Window->FirstChild, Index = 0;
         Child != NULL;
         Child = Child->NextSibling, ++Index)
      List[Index] = Child->hSelf;
   List[Index] = NULL;

   return List;
}

/***********************************************************************
 *           IntSendDestroyMsg
 */
static void IntSendDestroyMsg(HWND hWnd)
{

   PWINDOW_OBJECT Window;
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

      if (!IntGetOwner(Window) && !IntGetParent(Window))
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
      DPRINT("destroyed itself while in WM_DESTROY!\n");
   }
#endif
}

static VOID
UserFreeWindowInfo(PW32THREADINFO ti, PWINDOW_OBJECT WindowObject)
{
    PCLIENTINFO ClientInfo = GetWin32ClientInfo();
    PWINDOW Wnd = WindowObject->Wnd;

    if (ClientInfo->CallbackWnd.pvWnd == DesktopHeapAddressToUser(WindowObject->Wnd))
    {
        ClientInfo->CallbackWnd.hWnd = NULL;
        ClientInfo->CallbackWnd.pvWnd = NULL;
    }

   if (Wnd->WindowName.Buffer != NULL)
   {
       Wnd->WindowName.Length = 0;
       Wnd->WindowName.MaximumLength = 0;
       DesktopHeapFree(Wnd->pdesktop,
                       Wnd->WindowName.Buffer);
       Wnd->WindowName.Buffer = NULL;
   }

    DesktopHeapFree(Wnd->pdesktop, Wnd);
    WindowObject->Wnd = NULL;
}

/***********************************************************************
 *           IntDestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 *
 * This is the "functional" DestroyWindows function ei. all stuff
 * done in CreateWindow is undone here and not in DestroyWindow:-P

 */
static LRESULT co_UserFreeWindow(PWINDOW_OBJECT Window,
                                   PW32PROCESS ProcessData,
                                   PTHREADINFO ThreadData,
                                   BOOLEAN SendMessages)
{
   HWND *Children;
   HWND *ChildHandle;
   PWINDOW_OBJECT Child;
   PMENU_OBJECT Menu;
   BOOLEAN BelongsToThreadData;
   PWINDOW Wnd;

   ASSERT(Window);

   Wnd = Window->Wnd;

   if(Window->Status & WINDOWSTATUS_DESTROYING)
   {
      DPRINT("Tried to call IntDestroyWindow() twice\n");
      return 0;
   }
   Window->Status |= WINDOWSTATUS_DESTROYING;
   Wnd->Style &= ~WS_VISIBLE;
   /* remove the window already at this point from the thread window list so we
      don't get into trouble when destroying the thread windows while we're still
      in IntDestroyWindow() */
   RemoveEntryList(&Window->ThreadListEntry);

   BelongsToThreadData = IntWndBelongsToThread(Window, ThreadData);

   IntDeRegisterShellHookWindow(Window->hSelf);

   if(SendMessages)
   {
      /* Send destroy messages */
      IntSendDestroyMsg(Window->hSelf);
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
               IntSendDestroyMsg(Child->hSelf);
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
         co_IntSendMessage(Window->hSelf, WM_NCDESTROY, 0, 0);
   }
   MsqRemoveTimersWindow(ThreadData->MessageQueue, Window->hSelf);

   /* flush the message queue */
   MsqRemoveWindowMessagesFromQueue(Window);

   /* from now on no messages can be sent to this window anymore */
   Window->Status |= WINDOWSTATUS_DESTROYED;
   /* don't remove the WINDOWSTATUS_DESTROYING bit */

   /* reset shell window handles */
   if(ThreadData->Desktop)
   {
      if (Window->hSelf == ThreadData->Desktop->WindowStation->ShellWindow)
         ThreadData->Desktop->WindowStation->ShellWindow = NULL;

      if (Window->hSelf == ThreadData->Desktop->WindowStation->ShellListView)
         ThreadData->Desktop->WindowStation->ShellListView = NULL;
   }

   /* Unregister hot keys */
   UnregisterWindowHotKeys (Window);

   /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

#if 0 /* FIXME */

   WinPosCheckInternalPos(Window->hSelf);
   if (Window->hSelf == GetCapture())
   {
      ReleaseCapture();
   }

   /* free resources associated with the window */
   TIMER_RemoveWindowTimers(Window->hSelf);
#endif

   if (!(Wnd->Style & WS_CHILD) && Wnd->IDMenu
       && (Menu = UserGetMenuObject((HMENU)Wnd->IDMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE, TRUE);
      Wnd->IDMenu = 0;
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
   UserDeleteObject(Window->hSelf, otWindow);

   IntDestroyScrollBars(Window);

   /* dereference the class */
   IntDereferenceClass(Wnd->Class,
                       Window->ti->pDeskInfo,
                       Window->ti->ppi);
   Wnd->Class = NULL;

   if(Window->WindowRegion)
   {
      GreDeleteObject(Window->WindowRegion);
   }

   ASSERT(Window->Wnd != NULL);
   UserFreeWindowInfo(Window->ti, Window);

   UserDereferenceObject(Window);

   IntClipboardFreeWindow(Window);

   return 0;
}

VOID FASTCALL
IntGetWindowBorderMeasures(PWINDOW_OBJECT Window, UINT *cx, UINT *cy)
{
   PWINDOW Wnd = Window->Wnd;
   if(HAS_DLGFRAME(Wnd->Style, Wnd->ExStyle) && !(Wnd->Style & WS_MINIMIZE))
   {
      *cx = UserGetSystemMetrics(SM_CXDLGFRAME);
      *cy = UserGetSystemMetrics(SM_CYDLGFRAME);
   }
   else
   {
      if(HAS_THICKFRAME(Wnd->Style, Wnd->ExStyle)&& !(Wnd->Style & WS_MINIMIZE))
      {
         *cx = UserGetSystemMetrics(SM_CXFRAME);
         *cy = UserGetSystemMetrics(SM_CYFRAME);
      }
      else if(HAS_THINFRAME(Wnd->Style, Wnd->ExStyle))
      {
         *cx = UserGetSystemMetrics(SM_CXBORDER);
         *cy = UserGetSystemMetrics(SM_CYBORDER);
      }
      else
      {
         *cx = *cy = 0;
      }
   }
}

static WNDPROC
IntGetWindowProc(IN PWINDOW_OBJECT Window,
                 IN BOOL Ansi)
{
    PWINDOW Wnd = Window->Wnd;

    ASSERT(UserIsEnteredExclusive() == TRUE);

    if (Wnd->IsSystem)
    {
        return (Ansi ? Wnd->WndProcExtra : Wnd->WndProc);
    }
    else
    {
        if (!Ansi == Wnd->Unicode)
        {
            return Wnd->WndProc;
        }
        else
        {
            if (Wnd->CallProc != NULL)
            {
                return GetCallProcHandle(Wnd->CallProc);
            }
            /* BUGBOY Comments: Maybe theres something Im not undestanding here, but why would a CallProc be created
               on a function that I thought is only suppose to return the current Windows Proc? */
            else
            {
                PCALLPROC NewCallProc, CallProc;

                NewCallProc = UserFindCallProc(Wnd->Class,
                                               Wnd->WndProc,
                                               Wnd->Unicode);
                if (NewCallProc == NULL)
                {
                    NewCallProc = CreateCallProc(Wnd->ti->pDeskInfo,
                                                 Wnd->WndProc,
                                                 Wnd->Unicode,
                                                 Wnd->ti->ppi);
                    if (NewCallProc == NULL)
                    {
                        SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                        return NULL;
                    }

                    UserAddCallProcToClass(Wnd->Class,
                                           NewCallProc);
                }

                CallProc = Wnd->CallProc;
                Wnd->CallProc = NewCallProc;

                return GetCallProcHandle((CallProc == NULL ? NewCallProc : CallProc));
            }
        }
    }
}

BOOL FASTCALL
IntGetWindowInfo(PWINDOW_OBJECT Window, PWINDOWINFO pwi)
{
   PWINDOW Wnd = Window->Wnd;

   pwi->cbSize = sizeof(WINDOWINFO);
   pwi->rcWindow = Window->Wnd->WindowRect;
   pwi->rcClient = Window->Wnd->ClientRect;
   pwi->dwStyle = Wnd->Style;
   pwi->dwExStyle = Wnd->ExStyle;
   pwi->dwWindowStatus = (UserGetForegroundWindow() == Window->hSelf); /* WS_ACTIVECAPTION */
   IntGetWindowBorderMeasures(Window, &pwi->cxWindowBorders, &pwi->cyWindowBorders);
   pwi->atomWindowType = (Wnd->Class ? Wnd->Class->Atom : 0);
   pwi->wCreatorVersion = 0x400; /* FIXME - return a real version number */
   return TRUE;
}

static BOOL FASTCALL
IntSetMenu(
   PWINDOW_OBJECT Window,
   HMENU Menu,
   BOOL *Changed)
{
   PMENU_OBJECT OldMenu, NewMenu = NULL;
   PWINDOW Wnd = Window->Wnd;

   if ((Wnd->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
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
      ASSERT(NULL == OldMenu || OldMenu->MenuInfo.Wnd == Window->hSelf);
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
         SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }
      if (NULL != NewMenu->MenuInfo.Wnd)
      {
         /* Can't use the same menu for two windows */
         if (NULL != OldMenu)
         {
            IntReleaseMenuObject(OldMenu);
         }
         SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }

   }

   Wnd->IDMenu = (UINT) Menu;
   if (NULL != NewMenu)
   {
      NewMenu->MenuInfo.Wnd = Window->hSelf;
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
   PWINDOW_OBJECT Wnd;
   USER_REFERENCE_ENTRY Ref;
   WThread = (PTHREADINFO)Thread->Tcb.Win32Thread;

   while (!IsListEmpty(&WThread->WindowListHead))
   {
      Current = WThread->WindowListHead.Flink;
      Wnd = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);

      DPRINT("thread cleanup: while destroy wnds, wnd=0x%x\n",Wnd);

      /* window removes itself from the list */

      /*
      fixme: it is critical that the window removes itself! if now, we will loop
      here forever...
      */

      //ASSERT(co_UserDestroyWindow(Wnd));

      UserRefObjectCo(Wnd, &Ref);//faxme: temp hack??
      if (!co_UserDestroyWindow(Wnd))
      {
         DPRINT1("Unable to destroy window 0x%x at thread cleanup... This is _VERY_ bad!\n", Wnd);
      }
      UserDerefObjectCo(Wnd);//faxme: temp hack??
   }
}



/*!
 * Internal function.
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \note Does not check the validity of the parameters
*/
VOID FASTCALL
IntGetClientRect(PWINDOW_OBJECT Window, RECTL *Rect)
{
   ASSERT( Window );
   ASSERT( Rect );

   Rect->left = Rect->top = 0;
   Rect->right = Window->Wnd->ClientRect.right - Window->Wnd->ClientRect.left;
   Rect->bottom = Window->Wnd->ClientRect.bottom - Window->Wnd->ClientRect.top;
}


#if 0
HWND FASTCALL
IntGetFocusWindow(VOID)
{
   PUSER_MESSAGE_QUEUE Queue;
   PDESKTOP pdo = IntGetActiveDesktop();

   if( !pdo )
      return NULL;

   Queue = (PUSER_MESSAGE_QUEUE)pdo->ActiveMessageQueue;

   if (Queue == NULL)
      return(NULL);
   else
      return(Queue->FocusWindow);
}
#endif

PMENU_OBJECT FASTCALL
IntGetSystemMenu(PWINDOW_OBJECT Window, BOOL bRevert, BOOL RetMenu)
{
   PMENU_OBJECT Menu, NewMenu = NULL, SysMenu = NULL, ret = NULL;
   PTHREADINFO W32Thread;
   HMENU hNewMenu, hSysMenu;
   ROSMENUITEMINFO ItemInfo;

   if(bRevert)
   {
      W32Thread = PsGetCurrentThreadWin32Thread();

      if(!W32Thread->Desktop)
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

      if(W32Thread->Desktop->WindowStation->SystemMenuTemplate)
      {
         /* clone system menu */
         Menu = UserGetMenuObject(W32Thread->Desktop->WindowStation->SystemMenuTemplate);
         if(!Menu)
            return NULL;

         NewMenu = IntCloneMenu(Menu);
         if(NewMenu)
         {
            Window->SystemMenu = NewMenu->MenuInfo.Self;
            NewMenu->MenuInfo.Flags |= MF_SYSMENU;
            NewMenu->MenuInfo.Wnd = Window->hSelf;
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
         SysMenu->MenuInfo.Wnd = Window->hSelf;
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
IntIsChildWindow(PWINDOW_OBJECT Parent, PWINDOW_OBJECT BaseWindow)
{
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;

   Window = BaseWindow;
   while (Window)
   {
      Wnd = Window->Wnd;
      if (Window == Parent)
      {
         return(TRUE);
      }
      if(!(Wnd->Style & WS_CHILD))
      {
         break;
      }

      Window = Window->Parent;
   }

   return(FALSE);
}

BOOL FASTCALL
IntIsWindowVisible(PWINDOW_OBJECT BaseWindow)
{
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;

   Window = BaseWindow;
   while(Window)
   {
      Wnd = Window->Wnd;
      if(!(Wnd->Style & WS_CHILD))
      {
         break;
      }
      if(!(Wnd->Style & WS_VISIBLE))
      {
         return FALSE;
      }

      Window = Window->Parent;
   }

   if(Window && Wnd->Style & WS_VISIBLE)
   {
      return TRUE;
   }

   return FALSE;
}


/* link the window into siblings and parent. children are kept in place. */
VOID FASTCALL
IntLinkWindow(
   PWINDOW_OBJECT Wnd,
   PWINDOW_OBJECT WndParent,
   PWINDOW_OBJECT WndPrevSibling /* set to NULL if top sibling */
)
{
   PWINDOW_OBJECT Parent;

   Wnd->Parent = WndParent;
   Wnd->Wnd->Parent = WndParent ? WndParent->Wnd : NULL;
   if ((Wnd->PrevSibling = WndPrevSibling))
   {
      /* link after WndPrevSibling */
      if ((Wnd->NextSibling = WndPrevSibling->NextSibling))
         Wnd->NextSibling->PrevSibling = Wnd;
      else if ((Parent = Wnd->Parent))
      {
         if(Parent->LastChild == WndPrevSibling)
            Parent->LastChild = Wnd;
      }
      Wnd->PrevSibling->NextSibling = Wnd;
   }
   else
   {
      /* link at top */
      Parent = Wnd->Parent;
      if ((Wnd->NextSibling = WndParent->FirstChild))
         Wnd->NextSibling->PrevSibling = Wnd;
      else if (Parent)
      {
         Parent->LastChild = Wnd;
         Parent->FirstChild = Wnd;
         return;
      }
      if(Parent)
      {
         Parent->FirstChild = Wnd;
      }
   }

}

HWND FASTCALL
IntSetOwner(HWND hWnd, HWND hWndNewOwner)
{
   PWINDOW_OBJECT Wnd, WndOldOwner, WndNewOwner;
   HWND ret;

   Wnd = IntGetWindowObject(hWnd);
   if(!Wnd)
      return NULL;

   WndOldOwner = IntGetWindowObject(Wnd->hOwner);
   if (WndOldOwner)
   {
      ret = WndOldOwner->hSelf;
      UserDereferenceObject(WndOldOwner);
   }
   else
   {
      ret = 0;
   }

   if((WndNewOwner = UserGetWindowObject(hWndNewOwner)))
   {
      Wnd->hOwner = hWndNewOwner;
      Wnd->Wnd->Owner = WndNewOwner->Wnd;
   }
   else
   {
      Wnd->hOwner = NULL;
      Wnd->Wnd->Owner = NULL;
   }

   UserDereferenceObject(Wnd);
   return ret;
}

PWINDOW_OBJECT FASTCALL
co_IntSetParent(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndNewParent)
{
   PWINDOW_OBJECT WndOldParent, Sibling, InsertAfter;
//   HWND hWnd, hWndNewParent;
   BOOL WasVisible;

   ASSERT(Wnd);
   ASSERT(WndNewParent);
   ASSERT_REFS_CO(Wnd);
   ASSERT_REFS_CO(WndNewParent);

//   hWnd = Wnd->hSelf;
//   hWndNewParent = WndNewParent->hSelf;

   /*
    * Windows hides the window first, then shows it again
    * including the WM_SHOWWINDOW messages and all
    */
   WasVisible = co_WinPosShowWindow(Wnd, SW_HIDE);

//   /* Validate that window and parent still exist */
//   if (!IntIsWindow(hWnd) || !IntIsWindow(hWndNewParent))
//      return NULL;

   /* Window must belong to current process */
   if (Wnd->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
      return NULL;

   WndOldParent = Wnd->Parent;

   if (WndOldParent) UserReferenceObject(WndOldParent); /* caller must deref */

   if (WndNewParent != WndOldParent)
   {
      IntUnlinkWindow(Wnd);
      InsertAfter = NULL;
      if (0 == (Wnd->Wnd->ExStyle & WS_EX_TOPMOST))
      {
         /* Not a TOPMOST window, put after TOPMOSTs of new parent */
         Sibling = WndNewParent->FirstChild;
         while (NULL != Sibling && 0 != (Sibling->Wnd->ExStyle & WS_EX_TOPMOST))
         {
            InsertAfter = Sibling;
            Sibling = Sibling->NextSibling;
         }
      }
      if (NULL == InsertAfter)
      {
         IntLinkWindow(Wnd, WndNewParent, InsertAfter /*prev sibling*/);
      }
      else
      {
//         UserReferenceObject(InsertAfter);
         IntLinkWindow(Wnd, WndNewParent, InsertAfter /*prev sibling*/);
//         UserDereferenceObject(InsertAfter);
      }
   }

   /*
    * SetParent additionally needs to make hwnd the top window
    * in the z-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   co_WinPosSetWindowPos(Wnd, (0 == (Wnd->Wnd->ExStyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
                         0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
                         | (WasVisible ? SWP_SHOWWINDOW : 0));

   /*
    * FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
    * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE
    */

   /*
    * Validate that the old parent still exist, since it migth have been
    * destroyed during the last callbacks to user-mode
    */
//   if(WndOldParent)
//   {
//      if(!IntIsWindow(WndOldParent->hSelf))
//      {
//         UserDereferenceObject(WndOldParent);
//         return NULL;
//      }

      /* don't dereference the window object here, it must be done by the caller
         of IntSetParent() */
//      return WndOldParent;
//   }

   return WndOldParent;//NULL;
}

BOOL FASTCALL
IntSetSystemMenu(PWINDOW_OBJECT Window, PMENU_OBJECT Menu)
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
      /* FIXME check window style, propably return FALSE ? */
      Window->SystemMenu = Menu->MenuInfo.Self;
      Menu->MenuInfo.Flags |= MF_SYSMENU;
   }
   else
      Window->SystemMenu = (HMENU)0;

   return TRUE;
}


/* unlink the window from siblings and parent. children are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd)
{
   PWINDOW_OBJECT WndParent = Wnd->Parent;

   if (Wnd->NextSibling)
      Wnd->NextSibling->PrevSibling = Wnd->PrevSibling;
   else if (WndParent && WndParent->LastChild == Wnd)
      WndParent->LastChild = Wnd->PrevSibling;

   if (Wnd->PrevSibling)
      Wnd->PrevSibling->NextSibling = Wnd->NextSibling;
   else if (WndParent && WndParent->FirstChild == Wnd)
      WndParent->FirstChild = Wnd->NextSibling;

   Wnd->PrevSibling = Wnd->NextSibling = Wnd->Parent = NULL;
   if (Wnd->Wnd)
       Wnd->Wnd->Parent = NULL;
}

BOOL FASTCALL
IntAnyPopup(VOID)
{
   PWINDOW_OBJECT Window, Child;

   if(!(Window = UserGetWindowObject(IntGetDesktopWindow())))
   {
      return FALSE;
   }

   for(Child = Window->FirstChild; Child; Child = Child->NextSibling)
   {
      if(Child->hOwner && Child->Wnd->Style & WS_VISIBLE)
      {
         /*
          * The desktop has a popup window if one of them has
          * an owner window and is visible
          */
         return TRUE;
      }
   }

   return FALSE;
}

BOOL FASTCALL
IntIsWindowInDestroy(PWINDOW_OBJECT Window)
{
   return ((Window->Status & WINDOWSTATUS_DESTROYING) == WINDOWSTATUS_DESTROYING);
}


BOOL
FASTCALL
IntGetWindowPlacement(PWINDOW_OBJECT Window, WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW Wnd;
   POINT Size;

   Wnd = Window->Wnd;
   if (!Wnd) return FALSE;

   if(lpwndpl->length != sizeof(WINDOWPLACEMENT))
   {
      return FALSE;
   }

   lpwndpl->flags = 0;
   if (0 == (Wnd->Style & WS_VISIBLE))
   {
      lpwndpl->showCmd = SW_HIDE;
   }
   else if (0 != (Window->Flags & WINDOWOBJECT_RESTOREMAX) ||
            0 != (Wnd->Style & WS_MAXIMIZE))
   {
      lpwndpl->showCmd = SW_MAXIMIZE;
   }
   else if (0 != (Wnd->Style & WS_MINIMIZE))
   {
      lpwndpl->showCmd = SW_MINIMIZE;
   }
   else if (0 != (Wnd->Style & WS_VISIBLE))
   {
      lpwndpl->showCmd = SW_SHOWNORMAL;
   }

   Size.x = Wnd->WindowRect.left;
   Size.y = Wnd->WindowRect.top;
   WinPosInitInternalPos(Window, &Size,
                         &Wnd->WindowRect);

   lpwndpl->rcNormalPosition = Wnd->InternalPos.NormalRect;
   lpwndpl->ptMinPosition = Wnd->InternalPos.IconPos;
   lpwndpl->ptMaxPosition = Wnd->InternalPos.MaxPos;

   return TRUE;
}


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserAlterWindowStyle(DWORD Unknown0,
                       DWORD Unknown1,
                       DWORD Unknown2)
{
   UNIMPLEMENTED

   return(0);
}

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
      PWINDOW_OBJECT Parent, Window;

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
         (Window = Parent->FirstChild))
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
                     *pWnd = Window->hSelf;
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
               if (Window->FirstChild && bChildren)
               {
                  Window = Window->FirstChild;
                  continue;
               }
               bGoDown = FALSE;
            }
            if (Window->NextSibling)
            {
               Window = Window->NextSibling;
               bGoDown = TRUE;
               continue;
            }
            Window = Window->Parent;
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
   else
   {
      PETHREAD Thread;
      PTHREADINFO W32Thread;
      PLIST_ENTRY Current;
      PWINDOW_OBJECT Window;

      Status = PsLookupThreadByThreadId((HANDLE)dwThreadId, &Thread);
      if(!NT_SUCCESS(Status))
      {
         return ERROR_INVALID_PARAMETER;
      }
      if(!(W32Thread = (PTHREADINFO)Thread->Tcb.Win32Thread))
      {
         ObDereferenceObject(Thread);
         DPRINT("Thread is not a GUI Thread!\n");
         return ERROR_INVALID_PARAMETER;
      }

      Current = W32Thread->WindowListHead.Flink;
      while(Current != &(W32Thread->WindowListHead))
      {
         Window = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);
         ASSERT(Window);

         if(bChildren || Window->hOwner != NULL)
         {
             if(dwCount < *pBufSize && pWnd)
             {
                Status = MmCopyToCaller(pWnd++, &Window->hSelf, sizeof(HWND));
                if(!NT_SUCCESS(Status))
                {
                   SetLastNtError(Status);
                   break;
                }
             }
             dwCount++;
         }
         Current = Current->Flink;
      }

      ObDereferenceObject(Thread);
   }

   *pBufSize = dwCount;
   return STATUS_SUCCESS;
}


/*
 * @implemented
 */
HWND APIENTRY
NtUserChildWindowFromPointEx(HWND hwndParent,
                             LONG x,
                             LONG y,
                             UINT uiFlags)
{
   PWINDOW_OBJECT Parent;
   POINTL Pt;
   HWND Ret;
   HWND *List, *phWnd;

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      return NULL;
   }

   Pt.x = x;
   Pt.y = y;

   if(Parent->hSelf != IntGetDesktopWindow())
   {
      Pt.x += Parent->Wnd->ClientRect.left;
      Pt.y += Parent->Wnd->ClientRect.top;
   }

   if(!IntPtInWindow(Parent, Pt.x, Pt.y))
   {
      return NULL;
   }

   Ret = Parent->hSelf;
   if((List = IntWinListChildren(Parent)))
   {
      for(phWnd = List; *phWnd; phWnd++)
      {
         PWINDOW_OBJECT Child;
         PWINDOW ChildWnd;
         if((Child = UserGetWindowObject(*phWnd)))
         {
            ChildWnd = Child->Wnd;
            if(!(ChildWnd->Style & WS_VISIBLE) && (uiFlags & CWP_SKIPINVISIBLE))
            {
               continue;
            }
            if((ChildWnd->Style & WS_DISABLED) && (uiFlags & CWP_SKIPDISABLED))
            {
               continue;
            }
            if((ChildWnd->ExStyle & WS_EX_TRANSPARENT) && (uiFlags & CWP_SKIPTRANSPARENT))
            {
               continue;
            }
            if(IntPtInWindow(Child, Pt.x, Pt.y))
            {
               Ret = Child->hSelf;
               break;
            }
         }
      }
      ExFreePool(List);
   }

   return Ret;
}


/*
 * calculates the default position of a window
 */
BOOL FASTCALL
IntCalcDefPosSize(PWINDOW_OBJECT Parent, PWINDOW_OBJECT Window, RECTL *rc, BOOL IncPos)
{
   SIZE Sz;
   POINT Pos = {0, 0};

   if(Parent != NULL)
   {
      RECTL_bIntersectRect(rc, rc, &Parent->Wnd->ClientRect);

      if(IncPos)
      {
         Pos.x = Parent->TiledCounter * (UserGetSystemMetrics(SM_CXSIZE) + UserGetSystemMetrics(SM_CXFRAME));
         Pos.y = Parent->TiledCounter * (UserGetSystemMetrics(SM_CYSIZE) + UserGetSystemMetrics(SM_CYFRAME));
         if(Pos.x > ((rc->right - rc->left) / 4) ||
               Pos.y > ((rc->bottom - rc->top) / 4))
         {
            /* reset counter and position */
            Pos.x = 0;
            Pos.y = 0;
            Parent->TiledCounter = 0;
         }
         Parent->TiledCounter++;
      }
      Pos.x += rc->left;
      Pos.y += rc->top;
   }
   else
   {
      Pos.x = rc->left;
      Pos.y = rc->top;
   }

   Sz.cx = EngMulDiv(rc->right - rc->left, 3, 4);
   Sz.cy = EngMulDiv(rc->bottom - rc->top, 3, 4);

   rc->left = Pos.x;
   rc->top = Pos.y;
   rc->right = rc->left + Sz.cx;
   rc->bottom = rc->top + Sz.cy;
   return TRUE;
}


/*
 * @implemented
 */
HWND APIENTRY
co_IntCreateWindowEx(DWORD dwExStyle,
                     PUNICODE_STRING ClassName,
                     PUNICODE_STRING WindowName,
                     DWORD dwStyle,
                     LONG x,
                     LONG y,
                     LONG nWidth,
                     LONG nHeight,
                     HWND hWndParent,
                     HMENU hMenu,
                     HINSTANCE hInstance,
                     LPVOID lpParam,
                     DWORD dwShowMode,
                     BOOL bUnicodeWindow)
{
   PWINSTATION_OBJECT WinSta;
   PWINDOW Wnd = NULL;
   PWINDOWCLASS *ClassLink, Class = NULL;
   RTL_ATOM ClassAtom;
   PWINDOW_OBJECT Window = NULL;
   PWINDOW_OBJECT ParentWindow = NULL, OwnerWindow;
   HWND ParentWindowHandle;
   HWND OwnerWindowHandle;
   PMENU_OBJECT SystemMenu;
   HWND hWnd;
   POINT Pos;
   SIZE Size;
   PW32THREADINFO ti = NULL;
#if 0

   POINT MaxSize, MaxPos, MinTrack, MaxTrack;
#else

   POINT MaxPos;
#endif
   CREATESTRUCTW Cs;
   CBT_CREATEWNDW CbtCreate;
   LRESULT Result;
   BOOL MenuChanged;
   DECLARE_RETURN(HWND);
   BOOL HasOwner;
   USER_REFERENCE_ENTRY ParentRef, Ref;
   PTHREADINFO pti;

   pti = PsGetCurrentThreadWin32Thread();
   ParentWindowHandle = pti->Desktop->DesktopWindow;
   OwnerWindowHandle = NULL;

   if (hWndParent == HWND_MESSAGE)
   {
      /*
       * native ole32.OleInitialize uses HWND_MESSAGE to create the
       * message window (style: WS_POPUP|WS_DISABLED)
       */
      DPRINT1("FIXME - Parent is HWND_MESSAGE\n");
      // ParentWindowHandle set already.      
   }
   else if (hWndParent)
   {
      if ((dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
      {  //temp hack
         PWINDOW_OBJECT Par = UserGetWindowObject(hWndParent), Root;
         if (Par && (Root = UserGetAncestor(Par, GA_ROOT)))
            OwnerWindowHandle = Root->hSelf;
      }
      else
         ParentWindowHandle = hWndParent;
   }
   else if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      SetLastWin32Error(ERROR_TLW_WITH_WSCHILD);
      RETURN( (HWND)0);  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
   }

//   if (NULL != ParentWindowHandle)
//   {
   ParentWindow = UserGetWindowObject(ParentWindowHandle);

   if (ParentWindow) UserRefObjectCo(ParentWindow, &ParentRef);
//   }
//   else
//   {
//      ParentWindow = NULL;
//   }

   /* FIXME: parent must belong to the current process */

   /* Check the window station. */
   ti = GetW32ThreadInfo();
   if (ti == NULL || pti->Desktop == NULL)
   {
      DPRINT1("Thread is not attached to a desktop! Cannot create window!\n");
      RETURN( (HWND)0);
   }

   /* Check the class. */

   ClassAtom = IntGetClassAtom(ClassName,
                               hInstance,
                               ti->ppi,
                               &Class,
                               &ClassLink);

   if (ClassAtom == (RTL_ATOM)0)
   {
      if (IS_ATOM(ClassName->Buffer))
      {
         DPRINT1("Class 0x%p not found\n", (DWORD_PTR) ClassName->Buffer);
      }
      else
      {
         DPRINT1("Class \"%wZ\" not found\n", ClassName);
      }

      SetLastWin32Error(ERROR_CANNOT_FIND_WND_CLASS);
      RETURN((HWND)0);
   }

   Class = IntReferenceClass(Class,
                             ClassLink,
                             pti->Desktop);
   if (Class == NULL)
   {
       DPRINT1("Failed to reference window class!\n");
       RETURN(NULL);
   }

   WinSta = pti->Desktop->WindowStation;

   //FIXME: Reference thread/desktop instead
   ObReferenceObjectByPointer(WinSta, KernelMode, ExWindowStationObjectType, 0);

   /* Create the window object. */
   Window = (PWINDOW_OBJECT)
            UserCreateObject(gHandleTable, (PHANDLE)&hWnd,
                            otWindow, sizeof(WINDOW_OBJECT));
   if (Window)
   {
       Window->Wnd = DesktopHeapAlloc(pti->Desktop,
                                      sizeof(WINDOW) + Class->WndExtra);
       if (!Window->Wnd)
           goto AllocErr;
       RtlZeroMemory(Window->Wnd,
                     sizeof(WINDOW) + Class->WndExtra);
       Window->Wnd->hdr.Handle = hWnd; /* FIXME: Remove hack */
       Wnd = Window->Wnd;

       Wnd->ti = ti;
       Wnd->pi = ti->ppi;
       Wnd->pdesktop = pti->Desktop;
       Wnd->hWndLastActive = hWnd;
   }

   DPRINT("Created object with handle %X\n", hWnd);
   if (!Window)
   {
AllocErr:
      ObDereferenceObject(WinSta);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      RETURN( (HWND)0);
   }

   UserRefObjectCo(Window, &Ref);

   ObDereferenceObject(WinSta);

   if (NULL == pti->Desktop->DesktopWindow)
   {
      /* If there is no desktop window yet, we must be creating it */
      pti->Desktop->DesktopWindow = hWnd;
      pti->Desktop->DesktopInfo->Wnd = Wnd;
   }

   /*
    * Fill out the structure describing it.
    */
   Window->ti = ti;
   Wnd->Class = Class;
   Class = NULL;

   Window->SystemMenu = (HMENU)0;
   Wnd->ContextHelpId = 0;
   Wnd->IDMenu = 0;
   Wnd->Instance = hInstance;
   Window->hSelf = hWnd;

   Window->MessageQueue = pti->MessageQueue;
   IntReferenceMessageQueue(Window->MessageQueue);
   Window->Parent = ParentWindow;
   Wnd->Parent = ParentWindow ? ParentWindow->Wnd : NULL;
   if (Wnd->Parent != NULL && hWndParent != 0)
   {
       Wnd->HideFocus = Wnd->Parent->HideFocus;
       Wnd->HideAccel = Wnd->Parent->HideAccel;
   }

   if((OwnerWindow = UserGetWindowObject(OwnerWindowHandle)))
   {
      Window->hOwner = OwnerWindowHandle;
      Wnd->Owner = OwnerWindow->Wnd;
      HasOwner = TRUE;
   }
   else
   {
      Window->hOwner = NULL;
      Wnd->Owner = NULL;
      HasOwner = FALSE;
   }

   Wnd->UserData = 0;

   Wnd->IsSystem = Wnd->Class->System;

   /* BugBoy Comments: Comment below say that System classes are always created as UNICODE.
      In windows, creating a window with the ANSI version of CreateWindow sets the window
      to ansi as verified by testing with IsUnicodeWindow API.

      No where can I see in code or through testing does the window change back to ANSI
      after being created as UNICODE in ROS. I didnt do more testing to see what problems this would cause.*/
    // See NtUserDefSetText! We convert to Unicode all the time and never use Mix. (jt)
   if (Wnd->Class->System)
   {
       /* NOTE: Always create a unicode window for system classes! */
       Wnd->Unicode = TRUE;
       Wnd->WndProc = Wnd->Class->WndProc;
       Wnd->WndProcExtra = Wnd->Class->WndProcExtra;
   }
   else
   {
       Wnd->Unicode = Wnd->Class->Unicode;
       Wnd->WndProc = Wnd->Class->WndProc;
       Wnd->CallProc = NULL;
   }

   Window->OwnerThread = PsGetCurrentThread();
   Window->FirstChild = NULL;
   Window->LastChild = NULL;
   Window->PrevSibling = NULL;
   Window->NextSibling = NULL;
   Wnd->ExtraDataSize = Wnd->Class->WndExtra;

   InitializeListHead(&Wnd->PropListHead);
   InitializeListHead(&Window->WndObjListHead);

   if (NULL != WindowName->Buffer && WindowName->Length > 0)
   {
      Wnd->WindowName.Buffer = DesktopHeapAlloc(Wnd->pdesktop,
                                                WindowName->Length + sizeof(UNICODE_NULL));
      if (Wnd->WindowName.Buffer == NULL)
      {
          SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
          RETURN( (HWND)0);
      }

      Wnd->WindowName.Buffer[WindowName->Length / sizeof(WCHAR)] = L'\0';
      _SEH2_TRY
      {
          RtlCopyMemory(Wnd->WindowName.Buffer,
                        WindowName->Buffer,
                        WindowName->Length);
          Wnd->WindowName.Length = WindowName->Length;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
          WindowName->Length = 0;
          Wnd->WindowName.Buffer[0] = L'\0';
      }
      _SEH2_END;
   }

   /*
    * This has been tested for WS_CHILD | WS_VISIBLE.  It has not been
    * tested for WS_POPUP
    */
   if ((dwExStyle & WS_EX_DLGMODALFRAME) ||
         ((!(dwExStyle & WS_EX_STATICEDGE)) &&
          (dwStyle & (WS_DLGFRAME | WS_THICKFRAME))))
      dwExStyle |= WS_EX_WINDOWEDGE;
   else
      dwExStyle &= ~WS_EX_WINDOWEDGE;

   /* Correct the window style. */
   if (!(dwStyle & WS_CHILD))
   {
      dwStyle |= WS_CLIPSIBLINGS;
      DPRINT("3: Style is now %lx\n", dwStyle);
      if (!(dwStyle & WS_POPUP))
      {
         dwStyle |= WS_CAPTION;
         Window->Flags |= WINDOWOBJECT_NEED_SIZE;
         DPRINT("4: Style is now %lx\n", dwStyle);
      }
   }

   /* create system menu */
   if((dwStyle & WS_SYSMENU) )//&& (dwStyle & WS_CAPTION) == WS_CAPTION)
   {
      SystemMenu = IntGetSystemMenu(Window, TRUE, TRUE);
      if(SystemMenu)
      {
         Window->SystemMenu = SystemMenu->MenuInfo.Self;
         IntReleaseMenuObject(SystemMenu);
      }
   }

   /* Set the window menu */
   if ((dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
   {
      if (hMenu)
         IntSetMenu(Window, hMenu, &MenuChanged);
      else
      {
          hMenu = Wnd->Class->hMenu;
          if (hMenu) IntSetMenu(Window, hMenu, &MenuChanged);
      }
   }
   else
       Wnd->IDMenu = (UINT) hMenu;

   /* Insert the window into the thread's window list. */
   InsertTailList (&pti->WindowListHead, &Window->ThreadListEntry);

   /*  Handle "CS_CLASSDC", it is tested first. */
   if ((Wnd->Class->Style & CS_CLASSDC) && !(Wnd->Class->Dce)) // One DCE per class to have CLASS.
      Wnd->Class->Dce = DceAllocDCE(Window, DCE_CLASS_DC);
   /* Allocate a DCE for this window. */
   else if ( Wnd->Class->Style & CS_OWNDC)
      Window->Dce = DceAllocDCE(Window, DCE_WINDOW_DC);

   Pos.x = x;
   Pos.y = y;
   Size.cx = nWidth;
   Size.cy = nHeight;

   Wnd->ExStyle = dwExStyle;
   Wnd->Style = dwStyle & ~WS_VISIBLE;

   /* call hook */
   Cs.lpCreateParams = lpParam;
   Cs.hInstance = hInstance;
   Cs.hMenu = hMenu;
   Cs.hwndParent = hWndParent; //Pass the original Parent handle!
   Cs.cx = Size.cx;
   Cs.cy = Size.cy;
   Cs.x = Pos.x;
   Cs.y = Pos.y;
   Cs.style = Wnd->Style;
   Cs.lpszName = (LPCWSTR) WindowName;
   Cs.lpszClass = (LPCWSTR) ClassName;
   Cs.dwExStyle = dwExStyle;
   CbtCreate.lpcs = &Cs;
   CbtCreate.hwndInsertAfter = HWND_TOP;
   if (co_HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (WPARAM) hWnd, (LPARAM) &CbtCreate))
   {
      /* FIXME - Delete window object and remove it from the thread windows list */
      /* FIXME - delete allocated DCE */
      DPRINT1("CBT-hook returned !0\n");
      RETURN( (HWND) NULL);
   }

   x = Cs.x;
   y = Cs.y;
   nWidth = Cs.cx;
   nHeight = Cs.cy;

   /* default positioning for overlapped windows */
   if(!(Wnd->Style & (WS_POPUP | WS_CHILD)))
   {
      RECTL rc, WorkArea;
      PRTL_USER_PROCESS_PARAMETERS ProcessParams;
      BOOL CalculatedDefPosSize = FALSE;

      IntGetDesktopWorkArea(((PTHREADINFO)Window->OwnerThread->Tcb.Win32Thread)->Desktop, &WorkArea);

      rc = WorkArea;
      ProcessParams = PsGetCurrentProcess()->Peb->ProcessParameters;

      if(x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
      {
         CalculatedDefPosSize = IntCalcDefPosSize(ParentWindow, Window, &rc, TRUE);

         if(ProcessParams->WindowFlags & STARTF_USEPOSITION)
         {
            ProcessParams->WindowFlags &= ~STARTF_USEPOSITION;
            Pos.x = WorkArea.left + ProcessParams->StartingX;
            Pos.y = WorkArea.top + ProcessParams->StartingY;
         }
         else
         {
            Pos.x = rc.left;
            Pos.y = rc.top;
         }

/*
   According to wine, the ShowMode is set to y if x == CW_USEDEFAULT(16) and
   y is something else. and Quote!
 */

/* Never believe Microsoft's documentation... CreateWindowEx doc says
 * that if an overlapped window is created with WS_VISIBLE style bit
 * set and the x parameter is set to CW_USEDEFAULT, the system ignores
 * the y parameter. However, disassembling NT implementation (WIN32K.SYS)
 * reveals that
 *
 * 1) not only it checks for CW_USEDEFAULT but also for CW_USEDEFAULT16
 * 2) it does not ignore the y parameter as the docs claim; instead, it
 *    uses it as second parameter to ShowWindow() unless y is either
 *    CW_USEDEFAULT or CW_USEDEFAULT16.
 *
 * The fact that we didn't do 2) caused bogus windows pop up when wine
 * was running apps that were using this obscure feature. Example -
 * calc.exe that comes with Win98 (only Win98, it's different from
 * the one that comes with Win95 and NT)
 */
         if(y != CW_USEDEFAULT && y != CW_USEDEFAULT16)
         {
            dwShowMode = y;
         }
      }
      if(nWidth == CW_USEDEFAULT || nWidth == CW_USEDEFAULT16)
      {
         if(!CalculatedDefPosSize)
         {
            IntCalcDefPosSize(ParentWindow, Window, &rc, FALSE);
         }
         if(ProcessParams->WindowFlags & STARTF_USESIZE)
         {
            ProcessParams->WindowFlags &= ~STARTF_USESIZE;
            Size.cx = ProcessParams->CountX;
            Size.cy = ProcessParams->CountY;
         }
         else
         {
            Size.cx = rc.right - rc.left;
            Size.cy = rc.bottom - rc.top;
         }

         /* move the window if necessary */
         if(Pos.x > rc.left)
            Pos.x = max(rc.left, 0);
         if(Pos.y > rc.top)
            Pos.y = max(rc.top, 0);
      }
   }
   else
   {
      /* if CW_USEDEFAULT(16) is set for non-overlapped windows, both values are set to zero) */
      if(x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
      {
         Pos.x = 0;
         Pos.y = 0;
      }
      if(nWidth == CW_USEDEFAULT || nWidth == CW_USEDEFAULT16)
      {
         Size.cx = 0;
         Size.cy = 0;
      }
   }

   /* Initialize the window dimensions. */
   Wnd->WindowRect.left = Pos.x;
   Wnd->WindowRect.top = Pos.y;
   Wnd->WindowRect.right = Pos.x + Size.cx;
   Wnd->WindowRect.bottom = Pos.y + Size.cy;
   if (0 != (Wnd->Style & WS_CHILD) && ParentWindow)
   {
      RECTL_vOffsetRect(&(Wnd->WindowRect), ParentWindow->Wnd->ClientRect.left,
                       ParentWindow->Wnd->ClientRect.top);
   }
   Wnd->ClientRect = Wnd->WindowRect;

   /*
    * Get the size and position of the window.
    */
   if ((dwStyle & WS_THICKFRAME) || !(dwStyle & (WS_POPUP | WS_CHILD)))
   {
      POINT MaxSize, MaxPos, MinTrack, MaxTrack;

      /* WinPosGetMinMaxInfo sends the WM_GETMINMAXINFO message */
      co_WinPosGetMinMaxInfo(Window, &MaxSize, &MaxPos, &MinTrack,
                             &MaxTrack);
      if (MaxSize.x < Size.cx)
         Size.cx = MaxSize.x;
      if (MaxSize.y < Size.cy)
         Size.cy = MaxSize.y;
      if (Size.cx < MinTrack.x )
         Size.cx = MinTrack.x;
      if (Size.cy < MinTrack.y )
         Size.cy = MinTrack.y;
      if (Size.cx < 0)
         Size.cx = 0;
      if (Size.cy < 0)
         Size.cy = 0;
   }

   Wnd->WindowRect.left = Pos.x;
   Wnd->WindowRect.top = Pos.y;
   Wnd->WindowRect.right = Pos.x + Size.cx;
   Wnd->WindowRect.bottom = Pos.y + Size.cy;
   if (0 != (Wnd->Style & WS_CHILD) && ParentWindow)
   {
      RECTL_vOffsetRect(&(Wnd->WindowRect), ParentWindow->Wnd->ClientRect.left,
                       ParentWindow->Wnd->ClientRect.top);
   }
   Wnd->ClientRect = Wnd->WindowRect;

   /* FIXME: Initialize the window menu. */

   /* Send a NCCREATE message. */
   Cs.cx = Size.cx;
   Cs.cy = Size.cy;
   Cs.x = Pos.x;
   Cs.y = Pos.y;

   DPRINT("[win32k.window] IntCreateWindowEx style %d, exstyle %d, parent %d\n", Cs.style, Cs.dwExStyle, Cs.hwndParent);
   DPRINT("IntCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, Size.cx, Size.cy);
   DPRINT("IntCreateWindowEx(): About to send NCCREATE message.\n");
   Result = co_IntSendMessage(Window->hSelf, WM_NCCREATE, 0, (LPARAM) &Cs);
   if (!Result)
   {
      /* FIXME: Cleanup. */
      DPRINT1("IntCreateWindowEx(): NCCREATE message failed. No cleanup performed!\n");
      RETURN((HWND)0);
   }

   /* Calculate the non-client size. */
   MaxPos.x = Window->Wnd->WindowRect.left;
   MaxPos.y = Window->Wnd->WindowRect.top;


   DPRINT("IntCreateWindowEx(): About to get non-client size.\n");
   /* WinPosGetNonClientSize SENDS THE WM_NCCALCSIZE message */
   Result = co_WinPosGetNonClientSize(Window,
                                      &Window->Wnd->WindowRect,
                                      &Window->Wnd->ClientRect);

   RECTL_vOffsetRect(&Window->Wnd->WindowRect,
                    MaxPos.x - Window->Wnd->WindowRect.left,
                    MaxPos.y - Window->Wnd->WindowRect.top);


   if (NULL != ParentWindow)
   {
      /* link the window into the parent's child list */
      if ((dwStyle & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
      {
         PWINDOW_OBJECT PrevSibling;

         PrevSibling = ParentWindow->LastChild;

         /* link window as bottom sibling */
         IntLinkWindow(Window, ParentWindow, PrevSibling /*prev sibling*/);
      }
      else
      {
         /* link window as top sibling (but after topmost siblings) */
         PWINDOW_OBJECT InsertAfter, Sibling;
         if (!(dwExStyle & WS_EX_TOPMOST))
         {
            InsertAfter = NULL;
            Sibling = ParentWindow->FirstChild;
            while (Sibling && (Sibling->Wnd->ExStyle & WS_EX_TOPMOST))
            {
               InsertAfter = Sibling;
               Sibling = Sibling->NextSibling;
            }
         }
         else
         {
            InsertAfter = NULL;
         }

         IntLinkWindow(Window, ParentWindow, InsertAfter /* prev sibling */);

      }
   }

   /* Send the WM_CREATE message. */
   DPRINT("IntCreateWindowEx(): about to send CREATE message.\n");
   Result = co_IntSendMessage(Window->hSelf, WM_CREATE, 0, (LPARAM) &Cs);

   if (Result == (LRESULT)-1)
   {
      /* FIXME: Cleanup. */
      DPRINT1("IntCreateWindowEx(): send CREATE message failed. No cleanup performed!\n");
      IntUnlinkWindow(Window);
      RETURN((HWND)0);
   }
#if 0
   Result = IntNotifyWinEvent(EVENT_OBJECT_CREATE, Window, OBJID_WINDOW, 0);

   if (Result == (LRESULT)-1)
   {
      /* FIXME: Cleanup. */
      DPRINT1("IntCreateWindowEx(): event CREATE hook failed. No cleanup performed!\n");
      IntUnlinkWindow(Window);
      RETURN((HWND)0);
   }
#endif
   /* Send move and size messages. */
   if (!(Window->Flags & WINDOWOBJECT_NEED_SIZE))
   {
      LONG lParam;

      DPRINT("IntCreateWindow(): About to send WM_SIZE\n");

      if ((Window->Wnd->ClientRect.right - Window->Wnd->ClientRect.left) < 0 ||
            (Window->Wnd->ClientRect.bottom - Window->Wnd->ClientRect.top) < 0)
      {
         DPRINT("Sending bogus WM_SIZE\n");
      }

      lParam = MAKE_LONG(Window->Wnd->ClientRect.right -
                         Window->Wnd->ClientRect.left,
                         Window->Wnd->ClientRect.bottom -
                         Window->Wnd->ClientRect.top);
      co_IntSendMessage(Window->hSelf, WM_SIZE, SIZE_RESTORED,
                        lParam);

      DPRINT("IntCreateWindow(): About to send WM_MOVE\n");

      if (0 != (Wnd->Style & WS_CHILD) && ParentWindow)
      {
         lParam = MAKE_LONG(Wnd->ClientRect.left - ParentWindow->Wnd->ClientRect.left,
                            Wnd->ClientRect.top - ParentWindow->Wnd->ClientRect.top);
      }
      else
      {
         lParam = MAKE_LONG(Wnd->ClientRect.left,
                            Wnd->ClientRect.top);
      }

      co_IntSendMessage(Window->hSelf, WM_MOVE, 0, lParam);

      /* Call WNDOBJ change procs */
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);
   }

   /* Show or maybe minimize or maximize the window. */
   if (Wnd->Style & (WS_MINIMIZE | WS_MAXIMIZE))
   {
      RECTL NewPos;
      UINT16 SwFlag;

      SwFlag = (Wnd->Style & WS_MINIMIZE) ? SW_MINIMIZE :
               SW_MAXIMIZE;

      co_WinPosMinMaximize(Window, SwFlag, &NewPos);

      SwFlag = ((Wnd->Style & WS_CHILD) || UserGetActiveWindow()) ?
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
                SWP_NOZORDER | SWP_FRAMECHANGED;

      DPRINT("IntCreateWindow(): About to minimize/maximize\n");
      DPRINT("%d,%d %dx%d\n", NewPos.left, NewPos.top, NewPos.right, NewPos.bottom);
      co_WinPosSetWindowPos(Window, 0, NewPos.left, NewPos.top,
                            NewPos.right, NewPos.bottom, SwFlag);
   }

   /* Notify the parent window of a new child. */
   if ((Wnd->Style & WS_CHILD) &&
       (!(Wnd->ExStyle & WS_EX_NOPARENTNOTIFY)) && ParentWindow)
   {
      DPRINT("IntCreateWindow(): About to notify parent\n");
      co_IntSendMessage(ParentWindow->hSelf,
                        WM_PARENTNOTIFY,
                        MAKEWPARAM(WM_CREATE, Wnd->IDMenu),
                        (LPARAM)Window->hSelf);
   }

   if ((!hWndParent) && (!HasOwner))
   {
      DPRINT("Sending CREATED notify\n");
      co_IntShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)hWnd);
   }
   else
   {
      DPRINT("Not sending CREATED notify, %x %d\n", ParentWindow, HasOwner);
   }

   /* Initialize and show the window's scrollbars */
   if (Wnd->Style & WS_VSCROLL)
   {
      co_UserShowScrollBar(Window, SB_VERT, TRUE);
   }
   if (Wnd->Style & WS_HSCROLL)
   {
      co_UserShowScrollBar(Window, SB_HORZ, TRUE);
   }

   if (dwStyle & WS_VISIBLE)
   {
      if (Wnd->Style & WS_MAXIMIZE)
         dwShowMode = SW_SHOW;
      else if (Wnd->Style & WS_MINIMIZE)
         dwShowMode = SW_SHOWMINIMIZED;

      DPRINT("IntCreateWindow(): About to show window\n");
      co_WinPosShowWindow(Window, dwShowMode);

      if (Wnd->ExStyle & WS_EX_MDICHILD)
      {
        co_IntSendMessage(ParentWindow->hSelf, WM_MDIREFRESHMENU, 0, 0);
        /* ShowWindow won't activate child windows */
        co_WinPosSetWindowPos(Window, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
      }
   }

   /* BugBoy Comments: if the window being created is a edit control, ATOM 0xC007,
      then my testing shows that windows (2k and XP) creates a CallProc for it immediately 
      Dont understand why it does this. */
   if (ClassAtom == 0XC007)
   {
      PCALLPROC CallProc;
      //CallProc = CreateCallProc(NULL, Wnd->WndProc, bUnicodeWindow, Wnd->ti->ppi);
      CallProc = CreateCallProc(NULL, Wnd->WndProc, Wnd->Unicode , Wnd->ti->ppi);

      if (!CallProc)
      {
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         DPRINT1("Warning: Unable to create CallProc for edit control. Control may not operate correctly! hwnd %x\n",hWnd);
      }
      else
      {
         UserAddCallProcToClass(Wnd->Class, CallProc);
         Wnd->CallProc = CallProc;
         Wnd->IsSystem = FALSE;
      }
   }

   DPRINT("IntCreateWindow(): = %X\n", hWnd);
   DPRINT("WindowObject->SystemMenu = 0x%x\n", Window->SystemMenu);
   RETURN(hWnd);

CLEANUP:
   if (!_ret_ && Window && Window->Wnd && ti)
       UserFreeWindowInfo(ti, Window);
   if (Window)
   {
      UserDerefObjectCo(Window);
      UserDereferenceObject(Window);
   }
   if (ParentWindow) UserDerefObjectCo(ParentWindow);
   if (!_ret_ && ti != NULL)
   {
       if (Class != NULL)
       {
           IntDereferenceClass(Class,
                               ti->pDeskInfo,
                               ti->ppi);
       }
   }
   END_CLEANUP;
}

HWND APIENTRY
NtUserCreateWindowEx(DWORD dwExStyle,
                     PUNICODE_STRING UnsafeClassName,
                     PUNICODE_STRING UnsafeWindowName,
                     DWORD dwStyle,
                     LONG x,
                     LONG y,
                     LONG nWidth,
                     LONG nHeight,
                     HWND hWndParent,
                     HMENU hMenu,
                     HINSTANCE hInstance,
                     LPVOID lpParam,
                     DWORD dwShowMode,
                     BOOL bUnicodeWindow,
                     DWORD dwUnknown)
{
   NTSTATUS Status;
   UNICODE_STRING WindowName;
   UNICODE_STRING ClassName;
   HWND NewWindow;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);
   UserEnterExclusive();

   /* Get the class name (string or atom) */
   Status = MmCopyFromCaller(&ClassName, UnsafeClassName, sizeof(UNICODE_STRING));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( NULL);
   }
   if (ClassName.Length != 0)
   {
      Status = IntSafeCopyUnicodeStringTerminateNULL(&ClassName, UnsafeClassName);
      if (! NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( NULL);
      }
   }
   else if (! IS_ATOM(ClassName.Buffer))
   {
       SetLastWin32Error(ERROR_INVALID_PARAMETER);
       RETURN(NULL);
   }

   /* safely copy the window name */
   if (NULL != UnsafeWindowName)
   {
      Status = IntSafeCopyUnicodeString(&WindowName, UnsafeWindowName);
      if (! NT_SUCCESS(Status))
      {
         if (! IS_ATOM(ClassName.Buffer))
         {
            ExFreePoolWithTag(ClassName.Buffer, TAG_STRING);
         }
         SetLastNtError(Status);
         RETURN( NULL);
      }
   }
   else
   {
      RtlInitUnicodeString(&WindowName, NULL);
   }

   NewWindow = co_IntCreateWindowEx(dwExStyle, &ClassName, &WindowName, dwStyle, x, y, nWidth, nHeight,
                                    hWndParent, hMenu, hInstance, lpParam, dwShowMode, bUnicodeWindow);

   if (WindowName.Buffer)
   {
      ExFreePoolWithTag(WindowName.Buffer, TAG_STRING);
   }
   if (! IS_ATOM(ClassName.Buffer))
   {
      ExFreePoolWithTag(ClassName.Buffer, TAG_STRING);
   }

   RETURN( NewWindow);

CLEANUP:
   DPRINT("Leave NtUserCreateWindowEx, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @unimplemented
 */
HDWP APIENTRY
NtUserDeferWindowPos(HDWP WinPosInfo,
                     HWND Wnd,
                     HWND WndInsertAfter,
                     int x,
                     int y,
                     int cx,
                     int cy,
                     UINT Flags)
{
   UNIMPLEMENTED

   return 0;
}


BOOLEAN FASTCALL co_UserDestroyWindow(PWINDOW_OBJECT Window)
{
   BOOLEAN isChild;
   PWINDOW Wnd;

   ASSERT_REFS_CO(Window); //fixme: temp hack?

   Wnd = Window->Wnd;

   /* Check for owner thread */
   if ((Window->OwnerThread != PsGetCurrentThread()))
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   /* Look whether the focus is within the tree of windows we will
    * be destroying.
    */
   if (!co_WinPosShowWindow(Window, SW_HIDE))
   {
      if (UserGetActiveWindow() == Window->hSelf)
      {
         co_WinPosActivateOtherWindow(Window);
      }
   }

   if (Window->MessageQueue->ActiveWindow == Window->hSelf)
      Window->MessageQueue->ActiveWindow = NULL;
   if (Window->MessageQueue->FocusWindow == Window->hSelf)
      Window->MessageQueue->FocusWindow = NULL;
   if (Window->MessageQueue->CaptureWindow == Window->hSelf)
      Window->MessageQueue->CaptureWindow = NULL;

   IntDereferenceMessageQueue(Window->MessageQueue);
   /* Call hooks */
#if 0 /* FIXME */

   if (co_HOOK_CallHooks(WH_CBT, HCBT_DESTROYWND, (WPARAM) hwnd, 0, TRUE))
   {
      return FALSE;
   }
#endif

   IntEngWindowChanged(Window, WOC_DELETE);
   isChild = (0 != (Wnd->Style & WS_CHILD));

#if 0 /* FIXME */

   if (isChild)
   {
      if (! USER_IsExitingThread(GetCurrentThreadId()))
      {
         send_parent_notify(hwnd, WM_DESTROY);
      }
   }
   else if (NULL != GetWindow(Wnd, GW_OWNER))
   {
      co_HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0L, TRUE );
      /* FIXME: clean up palette - see "Internals" p.352 */
   }
#endif

   if (!IntIsWindow(Window->hSelf))
   {
      return TRUE;
   }

   /* Recursively destroy owned windows */
   if (! isChild)
   {
      for (;;)
      {
         BOOL GotOne = FALSE;
         HWND *Children;
         HWND *ChildHandle;
         PWINDOW_OBJECT Child, Desktop;

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
               if (Child->hOwner != Window->hSelf)
               {
                  continue;
               }

               if (IntWndBelongsToThread(Child, PsGetCurrentThreadWin32Thread()))
               {
                  USER_REFERENCE_ENTRY ChildRef;
                  UserRefObjectCo(Child, &ChildRef);//temp hack?
                  co_UserDestroyWindow(Child);
                  UserDerefObjectCo(Child);//temp hack?

                  GotOne = TRUE;
                  continue;
               }

               if (Child->hOwner != NULL)
               {
                  Child->hOwner = NULL;
                  Child->Wnd->Owner = NULL;
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

   if (!IntIsWindow(Window->hSelf))
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
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(BOOLEAN);
   BOOLEAN ret;
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserDestroyWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(Wnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);//faxme: dunno if win should be reffed during destroy..
   ret = co_UserDestroyWindow(Window);
   UserDerefObjectCo(Window);//faxme: dunno if win should be reffed during destroy..

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserDestroyWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



/*
 * @unimplemented
 */
DWORD
APIENTRY
NtUserDrawMenuBarTemp(
   HWND hWnd,
   HDC hDC,
   PRECT hRect,
   HMENU hMenu,
   HFONT hFont)
{
   /* we'll use this function just for caching the menu bar */
   UNIMPLEMENTED
   return 0;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserEndDeferWindowPosEx(DWORD Unknown0,
                          DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * FillWindow: Called from User; Dialog, Edit and ListBox procs during a WM_ERASEBKGND.
 */
/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserFillWindow(HWND hWndPaint,
                 HWND hWndPaint1,
                 HDC  hDC,
                 HBRUSH hBrush)
{
   UNIMPLEMENTED

   return 0;
}


static HWND FASTCALL
IntFindWindow(PWINDOW_OBJECT Parent,
              PWINDOW_OBJECT ChildAfter,
              RTL_ATOM ClassAtom,
              PUNICODE_STRING WindowName)
{
   BOOL CheckWindowName;
   HWND *List, *phWnd;
   HWND Ret = NULL;

   ASSERT(Parent);

   CheckWindowName = WindowName->Length != 0;

   if((List = IntWinListChildren(Parent)))
   {
      phWnd = List;
      if(ChildAfter)
      {
         /* skip handles before and including ChildAfter */
         while(*phWnd && (*(phWnd++) != ChildAfter->hSelf))
            ;
      }

      /* search children */
      while(*phWnd)
      {
         PWINDOW_OBJECT Child;
         if(!(Child = UserGetWindowObject(*(phWnd++))))
         {
            continue;
         }

         /* Do not send WM_GETTEXT messages in the kernel mode version!
            The user mode version however calls GetWindowText() which will
            send WM_GETTEXT messages to windows belonging to its processes */
         if((!CheckWindowName || !RtlCompareUnicodeString(WindowName, &(Child->Wnd->WindowName), TRUE)) &&
               (!ClassAtom || Child->Wnd->Class->Atom == ClassAtom))
         {
            Ret = Child->hSelf;
            break;
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
   PWINDOW_OBJECT Parent, ChildAfter;
   UNICODE_STRING ClassName = {0}, WindowName = {0};
   HWND Desktop, Ret = NULL;
   RTL_ATOM ClassAtom = (RTL_ATOM)0;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserFindWindowEx\n");
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
                   SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
               SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
   /* FIXME
   else if(hwndParent == HWND_MESSAGE)
   {
     hwndParent = IntGetMessageWindow();
   }
   */

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
       if(Parent->hSelf == Desktop)
       {
          HWND *List, *phWnd;
          PWINDOW_OBJECT TopLevelWindow;
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
                while(*phWnd && (*(phWnd++) != ChildAfter->hSelf))
                   ;
             }

             CheckWindowName = WindowName.Length != 0;

             /* search children */
             while(*phWnd)
             {
                if(!(TopLevelWindow = UserGetWindowObject(*(phWnd++))))
                {
                   continue;
                }

                /* Do not send WM_GETTEXT messages in the kernel mode version!
                   The user mode version however calls GetWindowText() which will
                   send WM_GETTEXT messages to windows belonging to its processes */
                WindowMatches = !CheckWindowName || !RtlCompareUnicodeString(
                                   &WindowName, &TopLevelWindow->Wnd->WindowName, TRUE);
                ClassMatches = (ClassAtom == (RTL_ATOM)0) ||
                               ClassAtom == TopLevelWindow->Wnd->Class->Atom;

                if (WindowMatches && ClassMatches)
                {
                   Ret = TopLevelWindow->hSelf;
                   break;
                }

                if (IntFindWindow(TopLevelWindow, NULL, ClassAtom, &WindowName))
                {
                   /* window returns the handle of the top-level window, in case it found
                      the child window */
                   Ret = TopLevelWindow->hSelf;
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
          /* FIXME - if both hwndParent and hwndChildAfter are NULL, we also should
                     search the message-only windows. Should this also be done if
                     Parent is the desktop window??? */
          PWINDOW_OBJECT MsgWindows;

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
   DPRINT("Leave NtUserFindWindowEx, ret %i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserFlashWindowEx(IN PFLASHWINFO pfwi)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
PWINDOW_OBJECT FASTCALL UserGetAncestor(PWINDOW_OBJECT Wnd, UINT Type)
{
   PWINDOW_OBJECT WndAncestor, Parent;

   if (Wnd->hSelf == IntGetDesktopWindow())
   {
      return NULL;
   }

   switch (Type)
   {
      case GA_PARENT:
         {
            WndAncestor = Wnd->Parent;
            break;
         }

      case GA_ROOT:
         {
            WndAncestor = Wnd;
            Parent = NULL;

            for(;;)
            {
               if(!(Parent = WndAncestor->Parent))
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
               PWINDOW_OBJECT Parent, Old;

               Old = WndAncestor;
               Parent = IntGetParent(WndAncestor);

               if (!Parent)
               {
                  break;
               }

               //temp hack
//               UserDereferenceObject(Parent);

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
   PWINDOW_OBJECT Window, Ancestor;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetAncestor\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(NULL);
   }

   Ancestor = UserGetAncestor(Window, Type);
   /* faxme: can UserGetAncestor ever return NULL for a valid window? */

   RETURN(Ancestor ? Ancestor->hSelf : NULL);

CLEANUP:
   DPRINT("Leave NtUserGetAncestor, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
APIENTRY
NtUserGetComboBoxInfo(
   HWND hWnd,
   PCOMBOBOXINFO pcbi)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetComboBoxInfo\n");
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
   RETURN( (BOOL) co_IntSendMessage( Wnd->hSelf, CB_GETCOMBOBOXINFO, 0, (LPARAM)pcbi));

CLEANUP:
   DPRINT("Leave NtUserGetComboBoxInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD APIENTRY
NtUserGetInternalWindowPos( HWND hWnd,
                            LPRECT rectWnd,
                            LPPOINT ptIcon)
{
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;
   DWORD Ret = 0;
   BOOL Hit = FALSE;
   WINDOWPLACEMENT wndpl;

   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {
      Hit = FALSE;
      goto Exit;
   }
   Wnd = Window->Wnd;

   _SEH2_TRY
   {
       if(rectWnd)
       {
          ProbeForWrite(rectWnd,
                        sizeof(RECT),
                        1);
       }
       if(ptIcon)
       {
          ProbeForWrite(ptIcon,
                        sizeof(POINT),
                        1);
       }
       
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       SetLastNtError(_SEH2_GetExceptionCode());
       Hit = TRUE;
   }
   _SEH2_END;

   wndpl.length = sizeof(WINDOWPLACEMENT);   

   if (IntGetWindowPlacement(Window, &wndpl) && !Hit)
   {
      _SEH2_TRY
      {
          if (rectWnd)
          {
             RtlCopyMemory(rectWnd, &wndpl.rcNormalPosition , sizeof(RECT));
          }
          if (ptIcon)
          {
             RtlCopyMemory(ptIcon, &wndpl.ptMinPosition, sizeof(POINT));
          }
       
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
          SetLastNtError(_SEH2_GetExceptionCode());
          Hit = TRUE;
      }
      _SEH2_END;

      if (!Hit) Ret = wndpl.showCmd;
   }
Exit:
   UserLeave();
   return Ret;
}

DWORD
APIENTRY
NtUserGetListBoxInfo(
   HWND hWnd)
{
   PWINDOW_OBJECT Wnd;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetListBoxInfo\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( 0 );
   }

   RETURN( (DWORD) co_IntSendMessage( Wnd->hSelf, LB_GETLISTBOXINFO, 0, 0 ));

CLEANUP:
   DPRINT("Leave NtUserGetListBoxInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


HWND FASTCALL
co_UserSetParent(HWND hWndChild, HWND hWndNewParent)
{
   PWINDOW_OBJECT Wnd = NULL, WndParent = NULL, WndOldParent;
   HWND hWndOldParent = NULL;
   USER_REFERENCE_ENTRY Ref, ParentRef;

   if (IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( NULL);
   }

   if (hWndChild == IntGetDesktopWindow())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
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
      hWndOldParent = WndOldParent->hSelf;
      UserDereferenceObject(WndOldParent);
   }

   return( hWndOldParent);
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

   DPRINT("Enter NtUserSetParent\n");
   UserEnterExclusive();

   RETURN( co_UserSetParent(hWndChild, hWndNewParent));

CLEANUP:
   DPRINT("Leave NtUserSetParent, ret=%i\n",_ret_);
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
HWND FASTCALL UserGetShellWindow()
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
   PWINDOW_OBJECT WndShell;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;
   NTSTATUS Status;
   PW32THREADINFO ti;

   DPRINT("Enter NtUserSetShellWindowEx\n");
   UserEnterExclusive();

   if (!(WndShell = UserGetWindowObject(hwndShell)))
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

      if (UserGetWindowLong(hwndListView, GWL_EXSTYLE, FALSE) & WS_EX_TOPMOST)
      {
         ObDereferenceObject(WinStaObject);
         RETURN( FALSE);
      }
   }

   if (UserGetWindowLong(hwndShell, GWL_EXSTYLE, FALSE) & WS_EX_TOPMOST)
   {
      ObDereferenceObject(WinStaObject);
      RETURN( FALSE);
   }

   UserRefObjectCo(WndShell, &Ref);
   co_WinPosSetWindowPos(WndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

   WinStaObject->ShellWindow = hwndShell;
   WinStaObject->ShellListView = hwndListView;

   ti = GetW32ThreadInfo();
   if (ti->pDeskInfo) ti->pDeskInfo->hShellWindow = hwndShell;

   UserDerefObjectCo(WndShell);

   ObDereferenceObject(WinStaObject);
   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetShellWindowEx, ret=%i\n",_ret_);
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
   PWINDOW_OBJECT Window;
   PMENU_OBJECT Menu;
   DECLARE_RETURN(HMENU);

   DPRINT("Enter NtUserGetSystemMenu\n");
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
   DPRINT("Leave NtUserGetSystemMenu, ret=%i\n",_ret_);
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
   PWINDOW_OBJECT Window;
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetSystemMenu\n");
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
   DPRINT("Leave NtUserSetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}




HWND FASTCALL
UserGetWindow(HWND hWnd, UINT Relationship)
{
   PWINDOW_OBJECT Parent, Window;
   HWND hWndResult = NULL;

   if (!(Window = UserGetWindowObject(hWnd)))
      return NULL;

   switch (Relationship)
   {
      case GW_HWNDFIRST:
         if((Parent = Window->Parent))
         {
            if (Parent->FirstChild)
               hWndResult = Parent->FirstChild->hSelf;
         }
         break;

      case GW_HWNDLAST:
         if((Parent = Window->Parent))
         {
            if (Parent->LastChild)
               hWndResult = Parent->LastChild->hSelf;
         }
         break;

      case GW_HWNDNEXT:
         if (Window->NextSibling)
            hWndResult = Window->NextSibling->hSelf;
         break;

      case GW_HWNDPREV:
         if (Window->PrevSibling)
            hWndResult = Window->PrevSibling->hSelf;
         break;

      case GW_OWNER:
         if((Parent = UserGetWindowObject(Window->hOwner)))
         {
            hWndResult = Parent->hSelf;
         }
         break;
      case GW_CHILD:
         if (Window->FirstChild)
            hWndResult = Window->FirstChild->hSelf;
         break;
   }

   return hWndResult;
}



/*
 * NtUserGetWindow
 *
 * The NtUserGetWindow function retrieves a handle to a window that has the
 * specified relationship (Z order or owner) to the specified window.
 *
 * Status
 *    @implemented
 */

HWND APIENTRY
NtUserGetWindow(HWND hWnd, UINT Relationship)
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetWindow\n");
   UserEnterShared();

   RETURN(UserGetWindow(hWnd, Relationship));

CLEANUP:
   DPRINT("Leave NtUserGetWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}




/*
 * NtUserGetWindowLong
 *
 * The NtUserGetWindowLong function retrieves information about the specified
 * window. The function also retrieves the 32-bit (long) value at the
 * specified offset into the extra window memory.
 *
 * Status
 *    @implemented
 */

LONG FASTCALL
UserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi)
{
   PWINDOW_OBJECT Window, Parent;
   PWINDOW Wnd;
   LONG Result = 0;

   DPRINT("NtUserGetWindowLong(%x,%d,%d)\n", hWnd, (INT)Index, Ansi);

   if (!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {
      return 0;
   }

   Wnd = Window->Wnd;

   /*
    * WndProc is only available to the owner process
    */
   if (GWL_WNDPROC == Index
         && Window->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return 0;
   }

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Window->Wnd->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }
      Result = *((LONG *)((PCHAR)(Window->Wnd + 1) + Index));
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            Result = Wnd->ExStyle;
            break;

         case GWL_STYLE:
            Result = Wnd->Style;
            break;

         case GWL_WNDPROC:
            Result = (LONG)IntGetWindowProc(Window,
                                            Ansi);
            break;

         case GWL_HINSTANCE:
            Result = (LONG) Wnd->Instance;
            break;

         case GWL_HWNDPARENT:
            Parent = Window->Parent;
            if(Parent)
            {
               if (Parent && Parent->hSelf == IntGetDesktopWindow())
                  Result = (LONG) UserGetWindow(Window->hSelf, GW_OWNER);
               else
                  Result = (LONG) Parent->hSelf;
            }
            break;

         case GWL_ID:
            Result = (LONG) Wnd->IDMenu;
            break;

         case GWL_USERDATA:
            Result = Wnd->UserData;
            break;

         default:
            DPRINT1("NtUserGetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            Result = 0;
            break;
      }
   }

   return Result;
}




/*
 * NtUserGetWindowLong
 *
 * The NtUserGetWindowLong function retrieves information about the specified
 * window. The function also retrieves the 32-bit (long) value at the
 * specified offset into the extra window memory.
 *
 * Status
 *    @implemented
 */

LONG APIENTRY
NtUserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi)
{
   DECLARE_RETURN(LONG);

   DPRINT("Enter NtUserGetWindowLong(%x,%d,%d)\n", hWnd, (INT)Index, Ansi);
   UserEnterExclusive();

   RETURN(UserGetWindowLong(hWnd, Index, Ansi));

CLEANUP:
   DPRINT("Leave NtUserGetWindowLong, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

static WNDPROC
IntSetWindowProc(PWINDOW_OBJECT Window,
                 WNDPROC NewWndProc,
                 BOOL Ansi)
{
    WNDPROC Ret;
    PCALLPROC CallProc;
    PWINDOW Wnd = Window->Wnd;

    /* resolve any callproc handle if possible */
    if (IsCallProcHandle(NewWndProc))
    {
        WNDPROC_INFO wpInfo;

        if (UserGetCallProcInfo((HANDLE)NewWndProc,
                                &wpInfo))
        {
            NewWndProc = wpInfo.WindowProc;
            /* FIXME - what if wpInfo.IsUnicode doesn't match Ansi? */
        }
    }

    /* attempt to get the previous window proc */
    if (Wnd->IsSystem)
    {
        Ret = (Ansi ? Wnd->WndProcExtra : Wnd->WndProc);
    }
    else
    {
        if (!Ansi == Wnd->Unicode)
        {
            Ret = Wnd->WndProc;
        }
        else
        {
            CallProc = UserFindCallProc(Wnd->Class,
                                        Wnd->WndProc,
                                        Wnd->Unicode);
            if (CallProc == NULL)
            {
                CallProc = CreateCallProc(NULL,
                                          Wnd->WndProc,
                                          Wnd->Unicode,
                                          Wnd->ti->ppi);
                if (CallProc == NULL)
                {
                    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                    return NULL;
                }

                UserAddCallProcToClass(Wnd->Class,
                                       CallProc);
            }
            /* BugBoy Comments: Added this if else, see below comments */
            if (!Wnd->CallProc)
            {
               Ret = Wnd->WndProc;
            }
            else
            {
                Ret = GetCallProcHandle(Wnd->CallProc);
            }

            Wnd->CallProc = CallProc;

            /* BugBoy Comments: Above sets the current CallProc for the
               window and below we set the Ret value to it.
               SetWindowLong for WNDPROC should return the previous proc
            Ret = GetCallProcHandle(Wnd->CallProc); */
        }
    }

    if (Wnd->Class->System)
    {
        /* check if the new procedure matches with the one in the
           window class. If so, we need to restore both procedures! */
        Wnd->IsSystem = (NewWndProc == Wnd->Class->WndProc ||
                         NewWndProc == Wnd->Class->WndProcExtra);

        if (Wnd->IsSystem)
        {
            Wnd->WndProc = Wnd->Class->WndProc;
            Wnd->WndProcExtra = Wnd->Class->WndProcExtra;
            Wnd->Unicode = !Ansi;
            return Ret;
        }
    }

    ASSERT(!Wnd->IsSystem);

    /* update the window procedure */
    Wnd->WndProc = NewWndProc;
    Wnd->Unicode = !Ansi;

    return Ret;
}


LONG FASTCALL
co_UserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   PWINDOW_OBJECT Window, Parent;
   PWINDOW Wnd;
   PWINSTATION_OBJECT WindowStation;
   LONG OldValue;
   STYLESTRUCT Style;

   if (hWnd == IntGetDesktopWindow())
   {
      SetLastWin32Error(STATUS_ACCESS_DENIED);
      return( 0);
   }

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      return( 0);
   }

   Wnd = Window->Wnd;

   if (!Wnd) return 0; // No go on zero.

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Wnd->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return( 0);
      }
      OldValue = *((LONG *)((PCHAR)(Wnd + 1) + Index));
      *((LONG *)((PCHAR)(Wnd + 1) + Index)) = NewValue;
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            OldValue = (LONG) Wnd->ExStyle;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;

            /*
             * Remove extended window style bit WS_EX_TOPMOST for shell windows.
             */
            WindowStation = ((PTHREADINFO)Window->OwnerThread->Tcb.Win32Thread)->Desktop->WindowStation;
            if(WindowStation)
            {
               if (hWnd == WindowStation->ShellWindow || hWnd == WindowStation->ShellListView)
                  Style.styleNew &= ~WS_EX_TOPMOST;
            }

            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM) &Style);
            Wnd->ExStyle = (DWORD)Style.styleNew;
            co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_EXSTYLE, (LPARAM) &Style);
            break;

         case GWL_STYLE:
            OldValue = (LONG) Wnd->Style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);
            Wnd->Style = (DWORD)Style.styleNew;
            co_IntSendMessage(hWnd, WM_STYLECHANGED, GWL_STYLE, (LPARAM) &Style);
            break;

         case GWL_WNDPROC:
         {
            /* FIXME: should check if window belongs to current process */
            OldValue = (LONG)IntSetWindowProc(Window,
                                              (WNDPROC)NewValue,
                                              Ansi);
            break;
         }

         case GWL_HINSTANCE:
            OldValue = (LONG) Wnd->Instance;
            Wnd->Instance = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            Parent = Window->Parent;
            if (Parent && (Parent->hSelf == IntGetDesktopWindow()))
               OldValue = (LONG) IntSetOwner(Window->hSelf, (HWND) NewValue);
            else
               OldValue = (LONG) co_UserSetParent(Window->hSelf, (HWND) NewValue);
            break;

         case GWL_ID:
            OldValue = (LONG) Wnd->IDMenu;
            Wnd->IDMenu = (UINT) NewValue;
            break;

         case GWL_USERDATA:
            OldValue = Wnd->UserData;
            Wnd->UserData = NewValue;
            break;

         default:
            DPRINT1("NtUserSetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
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

   DPRINT("Enter NtUserSetWindowLong\n");
   UserEnterExclusive();

   RETURN( co_UserSetWindowLong(hWnd, Index, NewValue, Ansi));

CLEANUP:
   DPRINT("Leave NtUserSetWindowLong, ret=%i\n",_ret_);
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
   PWINDOW_OBJECT Window;
   WORD OldValue;
   DECLARE_RETURN(WORD);

   DPRINT("Enter NtUserSetWindowWord\n");
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
         RETURN( co_UserSetWindowLong(Window->hSelf, Index, (UINT)NewValue, TRUE));
      default:
         if (Index < 0)
         {
            SetLastWin32Error(ERROR_INVALID_INDEX);
            RETURN( 0);
         }
   }

   if (Index > Window->Wnd->ExtraDataSize - sizeof(WORD))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   OldValue = *((WORD *)((PCHAR)(Window->Wnd + 1) + Index));
   *((WORD *)((PCHAR)(Window->Wnd + 1) + Index)) = NewValue;

   RETURN( OldValue);

CLEANUP:
   DPRINT("Leave NtUserSetWindowWord, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserGetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;
   POINT Size;
   WINDOWPLACEMENT Safepl;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetWindowPlacement\n");
   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   Wnd = Window->Wnd;

   Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }
   if(Safepl.length != sizeof(WINDOWPLACEMENT))
   {
      RETURN( FALSE);
   }

   Safepl.flags = 0;
   if (0 == (Wnd->Style & WS_VISIBLE))
   {
      Safepl.showCmd = SW_HIDE;
   }
   else if (0 != (Window->Flags & WINDOWOBJECT_RESTOREMAX) ||
            0 != (Wnd->Style & WS_MAXIMIZE))
   {
      Safepl.showCmd = SW_MAXIMIZE;
   }
   else if (0 != (Wnd->Style & WS_MINIMIZE))
   {
      Safepl.showCmd = SW_MINIMIZE;
   }
   else if (0 != (Wnd->Style & WS_VISIBLE))
   {
      Safepl.showCmd = SW_SHOWNORMAL;
   }

   Size.x = Wnd->WindowRect.left;
   Size.y = Wnd->WindowRect.top;
   WinPosInitInternalPos(Window, &Size,
                         &Wnd->WindowRect);

   Safepl.rcNormalPosition = Wnd->InternalPos.NormalRect;
   Safepl.ptMinPosition = Wnd->InternalPos.IconPos;
   Safepl.ptMaxPosition = Wnd->InternalPos.MaxPos;

   Status = MmCopyToCaller(lpwndpl, &Safepl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetWindowPlacement, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserLockWindowUpdate(HWND hWnd)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserMoveWindow(
   HWND hWnd,
   int X,
   int Y,
   int nWidth,
   int nHeight,
   BOOL bRepaint)
{
   return NtUserSetWindowPos(hWnd, 0, X, Y, nWidth, nHeight,
                             (bRepaint ? SWP_NOZORDER | SWP_NOACTIVATE :
                              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW));
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
   PWINDOW_OBJECT Window;
   DWORD Result;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserQueryWindow\n");
   UserEnterShared();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   switch(Index)
   {
      case QUERY_WINDOW_UNIQUE_PROCESS_ID:
         Result = (DWORD)IntGetWndProcessId(Window);
         break;

      case QUERY_WINDOW_UNIQUE_THREAD_ID:
         Result = (DWORD)IntGetWndThreadId(Window);
         break;

      case QUERY_WINDOW_ACTIVE:
         Result = (DWORD)UserGetActiveWindow();
         break;

      case QUERY_WINDOW_FOCUS:
         Result = (DWORD)IntGetFocusWindow();
         break;

      case QUERY_WINDOW_ISHUNG:
         Result = (DWORD)MsqIsHung(Window->MessageQueue);
         break;

      default:
         Result = (DWORD)NULL;
         break;
   }

   RETURN( Result);

CLEANUP:
   DPRINT("Leave NtUserQueryWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserRealChildWindowFromPoint(DWORD Unknown0,
                               DWORD Unknown1,
                               DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
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

   DPRINT("Enter NtUserRegisterWindowMessage\n");
   UserEnterExclusive();

   if(MessageNameUnsafe == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   Status = IntSafeCopyUnicodeStringTerminateNULL(&SafeMessageName, MessageNameUnsafe);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( 0);
   }

   Ret = (UINT)IntAddAtom(SafeMessageName.Buffer);

   ExFreePoolWithTag(SafeMessageName.Buffer, TAG_STRING);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserRegisterWindowMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserSetImeOwnerWindow(DWORD Unknown0,
                        DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD APIENTRY
NtUserSetInternalWindowPos(
   HWND    hwnd,
   UINT    showCmd,
   LPRECT  rect,
   LPPOINT pt)
{
   UNIMPLEMENTED

   return 0;

}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserSetLayeredWindowAttributes(HWND hwnd,
			   COLORREF crKey,
			   BYTE bAlpha,
			   DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserSetLogonNotifyWindow(HWND hWnd)
{
   UNIMPLEMENTED

   return 0;
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
   PWINDOW_OBJECT Window;
   BOOL Changed;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetMenu\n");
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
   DPRINT("Leave NtUserSetMenu, ret=%i\n",_ret_);
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
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetWindowFNID\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   Wnd = Window->Wnd;

   if (Wnd->Class)
   {  // From user land we only set these.
      if ((fnID != FNID_DESTROY) || ((fnID < FNID_BUTTON) && (fnID > FNID_IME)) )
      {
         RETURN( FALSE);
      }
      else
         Wnd->Class->fnID |= fnID;
   }
   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetWindowFNID\n");
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;
   WINDOWPLACEMENT Safepl;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserSetWindowPlacement\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   Wnd = Window->Wnd;

   Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }
   if(Safepl.length != sizeof(WINDOWPLACEMENT))
   {
      RETURN( FALSE);
   }

   UserRefObjectCo(Window, &Ref);

   if ((Wnd->Style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
   {
      co_WinPosSetWindowPos(Window, NULL,
                            Safepl.rcNormalPosition.left, Safepl.rcNormalPosition.top,
                            Safepl.rcNormalPosition.right - Safepl.rcNormalPosition.left,
                            Safepl.rcNormalPosition.bottom - Safepl.rcNormalPosition.top,
                            SWP_NOZORDER | SWP_NOACTIVATE);
   }

   /* FIXME - change window status */
   co_WinPosShowWindow(Window, Safepl.showCmd);

   Wnd->InternalPosInitialized = TRUE;
   Wnd->InternalPos.NormalRect = Safepl.rcNormalPosition;
   Wnd->InternalPos.IconPos = Safepl.ptMinPosition;
   Wnd->InternalPos.MaxPos = Safepl.ptMaxPosition;

   UserDerefObjectCo(Window);
   RETURN(TRUE);

CLEANUP:
   DPRINT("Leave NtUserSetWindowPlacement, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowPos(
   HWND hWnd,
   HWND hWndInsertAfter,
   int X,
   int Y,
   int cx,
   int cy,
   UINT uFlags)
{
   DECLARE_RETURN(BOOL);
   PWINDOW_OBJECT Window;
   BOOL ret;
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserSetWindowPos\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   /* First make sure that coordinates are valid for WM_WINDOWPOSCHANGING */
   if (!(uFlags & SWP_NOMOVE))
   {
      if (X < -32768) X = -32768;
      else if (X > 32767) X = 32767;
      if (Y < -32768) Y = -32768;
      else if (Y > 32767) Y = 32767;
   }
   if (!(uFlags & SWP_NOSIZE))
   {
      if (cx < 0) cx = 0;
      else if (cx > 32767) cx = 32767;
      if (cy < 0) cy = 0;
      else if (cy > 32767) cy = 32767;
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_WinPosSetWindowPos(Window, hWndInsertAfter, X, Y, cx, cy, uFlags);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserSetWindowPos, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


INT FASTCALL
IntGetWindowRgn(PWINDOW_OBJECT Window, HRGN hRgn)
{
   INT Ret;
   HRGN VisRgn;
   ROSRGNDATA *pRgn;
   PWINDOW Wnd;

   if(!Window)
   {
      return ERROR;
   }
   if(!hRgn)
   {
      return ERROR;
   }

   Wnd = Window->Wnd;

   /* Create a new window region using the window rectangle */
   VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->WindowRect);
   NtGdiOffsetRgn(VisRgn, -Window->Wnd->WindowRect.left, -Window->Wnd->WindowRect.top);
   /* if there's a region assigned to the window, combine them both */
   if(Window->WindowRegion && !(Wnd->Style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, Window->WindowRegion, RGN_AND);
   /* Copy the region into hRgn */
   NtGdiCombineRgn(hRgn, VisRgn, NULL, RGN_COPY);

   if((pRgn = REGION_LockRgn(hRgn)))
   {
      Ret = pRgn->rdh.iType;
      REGION_UnlockRgn(pRgn);
   }
   else
      Ret = ERROR;

   GreDeleteObject(VisRgn);

   return Ret;
}

INT FASTCALL
IntGetWindowRgnBox(PWINDOW_OBJECT Window, RECTL *Rect)
{
   INT Ret;
   HRGN VisRgn;
   ROSRGNDATA *pRgn;
   PWINDOW Wnd;

   if(!Window)
   {
      return ERROR;
   }
   if(!Rect)
   {
      return ERROR;
   }

   Wnd = Window->Wnd;

   /* Create a new window region using the window rectangle */
   VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->Wnd->WindowRect);
   NtGdiOffsetRgn(VisRgn, -Window->Wnd->WindowRect.left, -Window->Wnd->WindowRect.top);
   /* if there's a region assigned to the window, combine them both */
   if(Window->WindowRegion && !(Wnd->Style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, Window->WindowRegion, RGN_AND);

   if((pRgn = REGION_LockRgn(VisRgn)))
   {
      Ret = pRgn->rdh.iType;
      *Rect = pRgn->rdh.rcBound;
      REGION_UnlockRgn(pRgn);
   }
   else
      Ret = ERROR;

   GreDeleteObject(VisRgn);

   return Ret;
}


/*
 * @implemented
 */
INT APIENTRY
NtUserSetWindowRgn(
   HWND hWnd,
   HRGN hRgn,
   BOOL bRedraw)
{
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(INT);

   DPRINT("Enter NtUserSetWindowRgn\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   /* FIXME - Verify if hRgn is a valid handle!!!!
              Propably make this operation thread-safe, but maybe it's not necessary */

   if(Window->WindowRegion)
   {
      /* Delete no longer needed region handle */
      GreDeleteObject(Window->WindowRegion);
   }
   Window->WindowRegion = hRgn;

   /* FIXME - send WM_WINDOWPOSCHANGING and WM_WINDOWPOSCHANGED messages to the window */

   if(bRedraw)
   {
      USER_REFERENCE_ENTRY Ref;
      UserRefObjectCo(Window, &Ref);
      co_UserRedrawWindow(Window, NULL, NULL, RDW_INVALIDATE);
      UserDerefObjectCo(Window);
   }

   RETURN( (INT)hRgn);

CLEANUP:
   DPRINT("Leave NtUserSetWindowRgn, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL APIENTRY
NtUserShowWindow(HWND hWnd, LONG nCmdShow)
{
   PWINDOW_OBJECT Window;
   BOOL ret;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserShowWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_WinPosShowWindow(Window, nCmdShow);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   DPRINT("Leave NtUserShowWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserShowWindowAsync(HWND hWnd, LONG nCmdShow)
{
#if 0
   UNIMPLEMENTED
   return 0;
#else
   return NtUserShowWindow(hWnd, nCmdShow);
#endif
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserUpdateLayeredWindow(
   HWND hwnd,
   HDC hdcDst,
   POINT *pptDst,
   SIZE *psize,
   HDC hdcSrc,
   POINT *pptSrc,
   COLORREF crKey,
   BLENDFUNCTION *pblend,
   DWORD dwFlags)
{
   UNIMPLEMENTED

   return 0;
}


/*
 *    @implemented
 */
HWND APIENTRY
NtUserWindowFromPoint(LONG X, LONG Y)
{
   POINT pt;
   HWND Ret;
   PWINDOW_OBJECT DesktopWindow = NULL, Window = NULL;
   DECLARE_RETURN(HWND);
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserWindowFromPoint\n");
   UserEnterExclusive();

   if ((DesktopWindow = UserGetWindowObject(IntGetDesktopWindow())))
   {
      PTHREADINFO pti;
      USHORT Hit;

      pt.x = X;
      pt.y = Y;

      //hmm... threads live on desktops thus we have a reference on the desktop and indirectly the desktop window
      //its possible this referencing is useless, thou it shouldnt hurt...
      UserRefObjectCo(DesktopWindow, &Ref);

      pti = PsGetCurrentThreadWin32Thread();
      Hit = co_WinPosWindowFromPoint(DesktopWindow, pti->MessageQueue, &pt, &Window);

      if(Window)
      {
         Ret = Window->hSelf;

         RETURN( Ret);
      }
   }

   RETURN( NULL);

CLEANUP:
   if (Window) UserDereferenceObject(Window);
   if (DesktopWindow) UserDerefObjectCo(DesktopWindow);

   DPRINT("Leave NtUserWindowFromPoint, ret=%i\n",_ret_);
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
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;
   LARGE_STRING SafeText;
   UNICODE_STRING UnicodeString;
   BOOL Ret = TRUE;

   DPRINT("Enter NtUserDefSetText\n");

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

   if(!(Window = UserGetWindowObject(hWnd)) || !Window->Wnd)
   {
      UserLeave();
      return FALSE;
   }
   Wnd = Window->Wnd;

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
      if (Wnd->WindowName.MaximumLength > 0 &&
          UnicodeString.Length <= Wnd->WindowName.MaximumLength - sizeof(UNICODE_NULL))
      {
         ASSERT(Wnd->WindowName.Buffer != NULL);

         Wnd->WindowName.Length = UnicodeString.Length;
         Wnd->WindowName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
         RtlCopyMemory(Wnd->WindowName.Buffer,
                              UnicodeString.Buffer,
                              UnicodeString.Length);
      }
      else
      {
         PWCHAR buf;
         Wnd->WindowName.MaximumLength = Wnd->WindowName.Length = 0;
         buf = Wnd->WindowName.Buffer;
         Wnd->WindowName.Buffer = NULL;
         if (buf != NULL)
         {
            DesktopHeapFree(Wnd->pdesktop, buf);
         }

         Wnd->WindowName.Buffer = DesktopHeapAlloc(Wnd->pdesktop,
                                                   UnicodeString.Length + sizeof(UNICODE_NULL));
         if (Wnd->WindowName.Buffer != NULL)
         {
            Wnd->WindowName.Buffer[UnicodeString.Length / sizeof(WCHAR)] = L'\0';
            RtlCopyMemory(Wnd->WindowName.Buffer,
                                 UnicodeString.Buffer,
                                 UnicodeString.Length);
            Wnd->WindowName.MaximumLength = UnicodeString.Length + sizeof(UNICODE_NULL);
            Wnd->WindowName.Length = UnicodeString.Length;
         }
         else
         {
            SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
            Ret = FALSE;
            goto Exit;
         }
      }
   }
   else
   {
      Wnd->WindowName.Length = 0;
      if (Wnd->WindowName.Buffer != NULL)
          Wnd->WindowName.Buffer[0] = L'\0';
   }

   // HAX! FIXME! Windows does not do this in here!
   // In User32, these are called after: NotifyWinEvent EVENT_OBJECT_NAMECHANGE than
   // RepaintButton, StaticRepaint, NtUserCallHwndLock HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK, etc.
   /* Send shell notifications */
   if (!IntGetOwner(Window) && !IntGetParent(Window))
   {
      co_IntShellHookNotify(HSHELL_REDRAW, (LPARAM) hWnd);
   }

   Ret = TRUE;
Exit:
   if (UnicodeString.Buffer) RtlFreeUnicodeString(&UnicodeString);
   DPRINT("Leave NtUserDefSetText, ret=%i\n", Ret);
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
   PWINDOW_OBJECT Window;
   PWINDOW Wnd;
   NTSTATUS Status;
   INT Result;
   DECLARE_RETURN(INT);

   DPRINT("Enter NtUserInternalGetWindowText\n");
   UserEnterShared();

   if(lpString && (nMaxCount <= 1))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }
   Wnd = Window->Wnd;

   Result = Wnd->WindowName.Length / sizeof(WCHAR);
   if(lpString)
   {
      const WCHAR Terminator = L'\0';
      INT Copy;
      WCHAR *Buffer = (WCHAR*)lpString;

      Copy = min(nMaxCount - 1, Result);
      if(Copy > 0)
      {
         Status = MmCopyToCaller(Buffer, Wnd->WindowName.Buffer, Copy * sizeof(WCHAR));
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
   DPRINT("Leave NtUserInternalGetWindowText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL
FASTCALL
IntShowOwnedPopups(PWINDOW_OBJECT OwnerWnd, BOOL fShow )
{
   int count = 0;
   PWINDOW_OBJECT pWnd;
   HWND *win_array;

//   ASSERT(OwnerWnd);

   win_array = IntWinListChildren(UserGetWindowObject(IntGetDesktopWindow()));

   if (!win_array)
      return TRUE;

   while (win_array[count])
      count++;
   while (--count >= 0)
   {
      if (UserGetWindow( win_array[count], GW_OWNER ) != OwnerWnd->hSelf)
         continue;
      if (!(pWnd = UserGetWindowObject( win_array[count] )))
         continue;
      //        if (pWnd == WND_OTHER_PROCESS) continue;

      if (fShow)
      {
         if (pWnd->Flags & WIN_NEEDS_SHOW_OWNEDPOPUP)
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
         if (pWnd->Wnd->Style & WS_VISIBLE)
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

/*
 * NtUserValidateHandleSecure
 *
 * Status
 *    @implemented
 */

BOOL
APIENTRY
NtUserValidateHandleSecure(
   HANDLE handle,
   BOOL Restricted)
{
   if(!Restricted)
   {
     UINT uType;
     {
       PUSER_HANDLE_ENTRY entry;
       if (!(entry = handle_to_entry(gHandleTable, handle )))
       {
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return FALSE;
       }
       uType = entry->type;
     }
     switch (uType)
     {
       case otWindow:
       {
         PWINDOW_OBJECT Window;
         if ((Window = UserGetWindowObject((HWND) handle))) return TRUE;
         return FALSE;
       }
       case otMenu:
       {
         PMENU_OBJECT Menu;
         if ((Menu = UserGetMenuObject((HMENU) handle))) return TRUE;
         return FALSE;
       }
       case otAccel:
       {
         PACCELERATOR_TABLE Accel;
         if ((Accel = UserGetAccelObject((HACCEL) handle))) return TRUE;
         return FALSE;
       }
       case otCursorIcon:
       {
         PCURICON_OBJECT Cursor;
         if ((Cursor = UserGetCurIconObject((HCURSOR) handle))) return TRUE;
         return FALSE;
       }
       case otHook:
       {
         PHOOK Hook;
         if ((Hook = IntGetHookObject((HHOOK) handle))) return TRUE;
         return FALSE;
       }
       case otMonitor:
       {
         PMONITOR_OBJECT Monitor;
         if ((Monitor = UserGetMonitorObject((HMONITOR) handle))) return TRUE;
         return FALSE;
       }
       case otCallProc:
       {
         WNDPROC_INFO Proc;
         return UserGetCallProcInfo( handle, &Proc );
       }
       default:
         SetLastWin32Error(ERROR_INVALID_HANDLE);
     }
   }
   else
   { /* Is handle entry restricted? */
     UNIMPLEMENTED
   }
   return FALSE;
}


/* EOF */
