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
   if (Wnd->Style & WS_POPUP)
   {
      return UserGetWindowObject(Wnd->hOwner);
   }
   else if (Wnd->Style & WS_CHILD)
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
                                   PW32THREAD ThreadData,
                                   BOOLEAN SendMessages)
{
   HWND *Children;
   HWND *ChildHandle;
   PWINDOW_OBJECT Child;
   PMENU_OBJECT Menu;
   BOOLEAN BelongsToThreadData;

   ASSERT(Window);

   if(Window->Status & WINDOWSTATUS_DESTROYING)
   {
      DPRINT("Tried to call IntDestroyWindow() twice\n");
      return 0;
   }
   Window->Status |= WINDOWSTATUS_DESTROYING;
   Window->Style &= ~WS_VISIBLE;
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

            UserDerefObject(Child);
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

   if (!(Window->Style & WS_CHILD) && Window->IDMenu
         && (Menu = UserGetMenuObject((HMENU)Window->IDMenu)))
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

   UserRefObject(Window);
   ObmDeleteObject(Window->hSelf, otWindow);

   IntDestroyScrollBars(Window);

   if (!Window->Class->System && Window->CallProc != NULL)
   {
       DestroyCallProc(Window->ti->Desktop,
                       Window->CallProc);
   }

   if (Window->CallProc2 != NULL)
   {
       DestroyCallProc(Window->ti->Desktop,
                       Window->CallProc2);
   }

   /* dereference the class */
   IntDereferenceClass(Window->Class,
                       Window->ti->Desktop,
                       Window->ti->kpi);
   Window->Class = NULL;

   if(Window->WindowRegion)
   {
      NtGdiDeleteObject(Window->WindowRegion);
   }

   RtlFreeUnicodeString(&Window->WindowName);

   UserDerefObject(Window);

   IntClipboardFreeWindow(Window);

   return 0;
}

VOID FASTCALL
IntGetWindowBorderMeasures(PWINDOW_OBJECT Window, UINT *cx, UINT *cy)
{
   if(HAS_DLGFRAME(Window->Style, Window->ExStyle) && !(Window->Style & WS_MINIMIZE))
   {
      *cx = UserGetSystemMetrics(SM_CXDLGFRAME);
      *cy = UserGetSystemMetrics(SM_CYDLGFRAME);
   }
   else
   {
      if(HAS_THICKFRAME(Window->Style, Window->ExStyle)&& !(Window->Style & WS_MINIMIZE))
      {
         *cx = UserGetSystemMetrics(SM_CXFRAME);
         *cy = UserGetSystemMetrics(SM_CYFRAME);
      }
      else if(HAS_THINFRAME(Window->Style, Window->ExStyle))
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
    if (Window->IsSystem)
    {
        return (Ansi ? Window->WndProcExtra : Window->WndProc);
    }
    else
    {
        if (!Ansi == Window->Unicode)
        {
            return Window->WndProc;
        }
        else
        {
            if (Window->CallProc != NULL)
            {
                return GetCallProcHandle(Window->CallProc);
            }
            else
            {
                PCALLPROC NewCallProc, CallProc;

                /* NOTE: use the interlocked functions, as this operation may be done even
                         when only the shared lock is held! */
                NewCallProc = CreateCallProc(Window->ti->Desktop,
                                             Window->WndProc,
                                             Window->Unicode,
                                             Window->ti->kpi);
                if (NewCallProc == NULL)
                {
                    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                    return NULL;
                }

                CallProc = InterlockedCompareExchangePointer(&Window->CallProc,
                                                             NewCallProc,
                                                             NULL);
                if (CallProc != NULL)
                {
                    DestroyCallProc(Window->ti->Desktop,
                                    NewCallProc);
                }

                return GetCallProcHandle((CallProc == NULL ? NewCallProc : CallProc));
            }
        }
    }
}

BOOL FASTCALL
IntGetWindowInfo(PWINDOW_OBJECT Window, PWINDOWINFO pwi)
{
   pwi->cbSize = sizeof(WINDOWINFO);
   pwi->rcWindow = Window->WindowRect;
   pwi->rcClient = Window->ClientRect;
   pwi->dwStyle = Window->Style;
   pwi->dwExStyle = Window->ExStyle;
   pwi->dwWindowStatus = (UserGetForegroundWindow() == Window->hSelf); /* WS_ACTIVECAPTION */
   IntGetWindowBorderMeasures(Window, &pwi->cxWindowBorders, &pwi->cyWindowBorders);
   pwi->atomWindowType = (Window->Class ? Window->Class->Atom : 0);
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

   if ((Window->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   *Changed = (Window->IDMenu != (UINT) Menu);
   if (! *Changed)
   {
      return TRUE;
   }

   if (Window->IDMenu)
   {
      OldMenu = IntGetMenuObject((HMENU) Window->IDMenu);
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

   Window->IDMenu = (UINT) Menu;
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
   PW32THREAD WThread;
   PLIST_ENTRY Current;
   PWINDOW_OBJECT Wnd;
   USER_REFERENCE_ENTRY Ref;
   WThread = (PW32THREAD)Thread->Tcb.Win32Thread;

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
IntGetClientRect(PWINDOW_OBJECT Window, PRECT Rect)
{
   ASSERT( Window );
   ASSERT( Rect );

   Rect->left = Rect->top = 0;
   Rect->right = Window->ClientRect.right - Window->ClientRect.left;
   Rect->bottom = Window->ClientRect.bottom - Window->ClientRect.top;
}


#if 0
HWND FASTCALL
IntGetFocusWindow(VOID)
{
   PUSER_MESSAGE_QUEUE Queue;
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();

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
   PW32THREAD W32Thread;
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
            IntDestroyMenuObject(Menu, FALSE, TRUE);
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

   Window = BaseWindow;
   while (Window)
   {
      if (Window == Parent)
      {
         return(TRUE);
      }
      if(!(Window->Style & WS_CHILD))
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

   Window = BaseWindow;
   while(Window)
   {
      if(!(Window->Style & WS_CHILD))
      {
         break;
      }
      if(!(Window->Style & WS_VISIBLE))
      {
         return FALSE;
      }

      Window = Window->Parent;
   }

   if(Window && Window->Style & WS_VISIBLE)
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
      UserDerefObject(WndOldOwner);
   }
   else
   {
      ret = 0;
   }

   if((WndNewOwner = UserGetWindowObject(hWndNewOwner)))
   {
      Wnd->hOwner = hWndNewOwner;
   }
   else
      Wnd->hOwner = NULL;

   UserDerefObject(Wnd);
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

   if (WndOldParent) UserRefObject(WndOldParent); /* caller must deref */

   if (WndNewParent != WndOldParent)
   {
      IntUnlinkWindow(Wnd);
      InsertAfter = NULL;
      if (0 == (Wnd->ExStyle & WS_EX_TOPMOST))
      {
         /* Not a TOPMOST window, put after TOPMOSTs of new parent */
         Sibling = WndNewParent->FirstChild;
         while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
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
//         UserRefObject(InsertAfter);
         IntLinkWindow(Wnd, WndNewParent, InsertAfter /*prev sibling*/);
//         UserDerefObject(InsertAfter);
      }
   }

   /*
    * SetParent additionally needs to make hwnd the top window
    * in the z-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   co_WinPosSetWindowPos(Wnd, (0 == (Wnd->ExStyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
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
//         UserDerefObject(WndOldParent);
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
      if(Child->hOwner && Child->Style & WS_VISIBLE)
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

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
DWORD STDCALL
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
ULONG
STDCALL
NtUserBuildHwndList(
   HDESK hDesktop,
   HWND hwndParent,
   BOOLEAN bChildren,
   ULONG dwThreadId,
   ULONG lParam,
   HWND* pWnd,
   ULONG nBufSize)
{
   NTSTATUS Status;
   ULONG dwCount = 0;

   if (hwndParent || !dwThreadId)
   {
      PDESKTOP_OBJECT Desktop;
      PWINDOW_OBJECT Parent, Window;

      if(!hwndParent)
      {
         if(hDesktop == NULL && !(Desktop = IntGetActiveDesktop()))
         {
            SetLastWin32Error(ERROR_INVALID_HANDLE);
            return 0;
         }

         if(hDesktop)
         {
            Status = IntValidateDesktopHandle(hDesktop,
                                              UserMode,
                                              0,
                                              &Desktop);
            if(!NT_SUCCESS(Status))
            {
               SetLastWin32Error(ERROR_INVALID_HANDLE);
               return 0;
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
               if(dwCount++ < nBufSize && pWnd)
               {
                  _SEH_TRY
                  {
                     ProbeForWrite(pWnd, sizeof(HWND), 1);
                     *pWnd = Window->hSelf;
                     pWnd++;
                  }
                  _SEH_HANDLE
                  {
                     Status = _SEH_GetExceptionCode();
                  }
                  _SEH_END
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
      PW32THREAD W32Thread;
      PLIST_ENTRY Current;
      PWINDOW_OBJECT Window;

      Status = PsLookupThreadByThreadId((HANDLE)dwThreadId, &Thread);
      if(!NT_SUCCESS(Status))
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }
      if(!(W32Thread = (PW32THREAD)Thread->Tcb.Win32Thread))
      {
         ObDereferenceObject(Thread);
         DPRINT("Thread is not a GUI Thread!\n");
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }

      Current = W32Thread->WindowListHead.Flink;
      while(Current != &(W32Thread->WindowListHead))
      {
         Window = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);
         ASSERT(Window);

         if(dwCount < nBufSize && pWnd)
         {
            Status = MmCopyToCaller(pWnd++, &Window->hSelf, sizeof(HWND));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               break;
            }
         }
         dwCount++;
         Current = Current->Flink;
      }

      ObDereferenceObject(Thread);
   }

   return dwCount;
}


/*
 * @implemented
 */
HWND STDCALL
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
      Pt.x += Parent->ClientRect.left;
      Pt.y += Parent->ClientRect.top;
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
         if((Child = UserGetWindowObject(*phWnd)))
         {
            if(!(Child->Style & WS_VISIBLE) && (uiFlags & CWP_SKIPINVISIBLE))
            {
               continue;
            }
            if((Child->Style & WS_DISABLED) && (uiFlags & CWP_SKIPDISABLED))
            {
               continue;
            }
            if((Child->ExStyle & WS_EX_TRANSPARENT) && (uiFlags & CWP_SKIPTRANSPARENT))
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
IntCalcDefPosSize(PWINDOW_OBJECT Parent, PWINDOW_OBJECT Window, RECT *rc, BOOL IncPos)
{
   SIZE Sz;
   POINT Pos = {0, 0};

   if(Parent != NULL)
   {
      IntGdiIntersectRect(rc, rc, &Parent->ClientRect);

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
HWND STDCALL
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

   ParentWindowHandle = PsGetCurrentThreadWin32Thread()->Desktop->DesktopWindow;
   OwnerWindowHandle = NULL;

   if (hWndParent == HWND_MESSAGE)
   {
      /*
       * native ole32.OleInitialize uses HWND_MESSAGE to create the
       * message window (style: WS_POPUP|WS_DISABLED)
       */
      DPRINT1("FIXME - Parent is HWND_MESSAGE\n");
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
   if (ti == NULL || PsGetCurrentThreadWin32Thread()->Desktop == NULL)
   {
      DPRINT1("Thread is not attached to a desktop! Cannot create window!\n");
      RETURN( (HWND)0);
   }

   /* Check the class. */

   ClassAtom = IntGetClassAtom(ClassName,
                               hInstance,
                               ti->kpi,
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
                             ti->Desktop);
   if (Class == NULL)
   {
       DPRINT1("Failed to reference window class!\n");
       RETURN(NULL);
   }

   WinSta = PsGetCurrentThreadWin32Thread()->Desktop->WindowStation;

   //FIXME: Reference thread/desktop instead
   ObReferenceObjectByPointer(WinSta, KernelMode, ExWindowStationObjectType, 0);

   /* Create the window object. */
   Window = (PWINDOW_OBJECT)
            ObmCreateObject(gHandleTable, (PHANDLE)&hWnd,
                            otWindow, sizeof(WINDOW_OBJECT) + Class->WndExtra
                           );

   DPRINT("Created object with handle %X\n", hWnd);
   if (!Window)
   {
      ObDereferenceObject(WinSta);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      RETURN( (HWND)0);
   }

   UserRefObjectCo(Window, &Ref);

   ObDereferenceObject(WinSta);

   if (NULL == PsGetCurrentThreadWin32Thread()->Desktop->DesktopWindow)
   {
      /* If there is no desktop window yet, we must be creating it */
      PsGetCurrentThreadWin32Thread()->Desktop->DesktopWindow = hWnd;
   }

   /*
    * Fill out the structure describing it.
    */
   Window->ti = ti;
   Window->Class = Class;
   Class = NULL;

   Window->SystemMenu = (HMENU)0;
   Window->ContextHelpId = 0;
   Window->IDMenu = 0;
   Window->Instance = hInstance;
   Window->hSelf = hWnd;

   if (!hMenu)
       hMenu = Window->Class->hMenu;

   if (0 != (dwStyle & WS_CHILD))
   {
      Window->IDMenu = (UINT) hMenu;
   }
   else
   {
      IntSetMenu(Window, hMenu, &MenuChanged);
   }

   Window->MessageQueue = PsGetCurrentThreadWin32Thread()->MessageQueue;
   IntReferenceMessageQueue(Window->MessageQueue);
   Window->Parent = ParentWindow;

   if((OwnerWindow = UserGetWindowObject(OwnerWindowHandle)))
   {
      Window->hOwner = OwnerWindowHandle;
      HasOwner = TRUE;
   }
   else
   {
      Window->hOwner = NULL;
      HasOwner = FALSE;
   }

   Window->UserData = 0;

   Window->IsSystem = Window->Class->System;
   if (Window->Class->System)
   {
       /* NOTE: Always create a unicode window for system classes! */
       Window->Unicode = TRUE;
       Window->WndProc = Window->Class->WndProc;
       Window->WndProcExtra = Window->Class->WndProcExtra;
   }
   else
   {
       Window->Unicode = Window->Class->Unicode;
       Window->WndProc = Window->Class->WndProc;
       Window->CallProc = NULL;
   }

   Window->OwnerThread = PsGetCurrentThread();
   Window->FirstChild = NULL;
   Window->LastChild = NULL;
   Window->PrevSibling = NULL;
   Window->NextSibling = NULL;
   Window->ExtraDataSize = Window->Class->WndExtra;

   /* extra window data */
   if (Window->Class->WndExtra)
      Window->ExtraData = (PCHAR)(Window + 1);

   InitializeListHead(&Window->PropListHead);
   InitializeListHead(&Window->WndObjListHead);

   if (NULL != WindowName->Buffer)
   {
      Window->WindowName.MaximumLength = WindowName->MaximumLength;
      Window->WindowName.Length = WindowName->Length;
      Window->WindowName.Buffer = ExAllocatePoolWithTag(PagedPool, WindowName->MaximumLength,
                                  TAG_STRING);
      if (NULL == Window->WindowName.Buffer)
      {
         DPRINT1("Failed to allocate mem for window name\n");
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         RETURN( NULL);
      }
      RtlCopyMemory(Window->WindowName.Buffer, WindowName->Buffer, WindowName->MaximumLength);
   }
   else
   {
      RtlInitUnicodeString(&Window->WindowName, NULL);
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
   if((dwStyle & WS_SYSMENU) &&
         (dwStyle & WS_CAPTION) == WS_CAPTION)
   {
      SystemMenu = IntGetSystemMenu(Window, TRUE, TRUE);
      if(SystemMenu)
      {
         Window->SystemMenu = SystemMenu->MenuInfo.Self;
         IntReleaseMenuObject(SystemMenu);
      }
   }

   /* Insert the window into the thread's window list. */
   InsertTailList (&PsGetCurrentThreadWin32Thread()->WindowListHead, &Window->ThreadListEntry);

   /* Allocate a DCE for this window. */
   if (dwStyle & CS_OWNDC)
   {
      Window->Dce = DceAllocDCE(Window, DCE_WINDOW_DC);
   }
   /* FIXME:  Handle "CS_CLASSDC" */

   Pos.x = x;
   Pos.y = y;
   Size.cx = nWidth;
   Size.cy = nHeight;

   Window->ExStyle = dwExStyle;
   Window->Style = dwStyle & ~WS_VISIBLE;

   /* call hook */
   Cs.lpCreateParams = lpParam;
   Cs.hInstance = hInstance;
   Cs.hMenu = hMenu;
   Cs.hwndParent = hWndParent; //Pass the original Parent handle!
   Cs.cx = Size.cx;
   Cs.cy = Size.cy;
   Cs.x = Pos.x;
   Cs.y = Pos.y;
   Cs.style = Window->Style;
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
   if(!(Window->Style & (WS_POPUP | WS_CHILD)))
   {
      RECT rc, WorkArea;
      PRTL_USER_PROCESS_PARAMETERS ProcessParams;
      BOOL CalculatedDefPosSize = FALSE;

      IntGetDesktopWorkArea(((PW32THREAD)Window->OwnerThread->Tcb.Win32Thread)->Desktop, &WorkArea);

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
   Window->WindowRect.left = Pos.x;
   Window->WindowRect.top = Pos.y;
   Window->WindowRect.right = Pos.x + Size.cx;
   Window->WindowRect.bottom = Pos.y + Size.cy;
   if (0 != (Window->Style & WS_CHILD) && ParentWindow)
   {
      IntGdiOffsetRect(&(Window->WindowRect), ParentWindow->ClientRect.left,
                       ParentWindow->ClientRect.top);
   }
   Window->ClientRect = Window->WindowRect;

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

   Window->WindowRect.left = Pos.x;
   Window->WindowRect.top = Pos.y;
   Window->WindowRect.right = Pos.x + Size.cx;
   Window->WindowRect.bottom = Pos.y + Size.cy;
   if (0 != (Window->Style & WS_CHILD) && ParentWindow)
   {
      IntGdiOffsetRect(&(Window->WindowRect), ParentWindow->ClientRect.left,
                       ParentWindow->ClientRect.top);
   }
   Window->ClientRect = Window->WindowRect;

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
      DPRINT("IntCreateWindowEx(): NCCREATE message failed.\n");
      RETURN((HWND)0);
   }

   /* Calculate the non-client size. */
   MaxPos.x = Window->WindowRect.left;
   MaxPos.y = Window->WindowRect.top;


   DPRINT("IntCreateWindowEx(): About to get non-client size.\n");
   /* WinPosGetNonClientSize SENDS THE WM_NCCALCSIZE message */
   Result = co_WinPosGetNonClientSize(Window,
                                      &Window->WindowRect,
                                      &Window->ClientRect);

   IntGdiOffsetRect(&Window->WindowRect,
                    MaxPos.x - Window->WindowRect.left,
                    MaxPos.y - Window->WindowRect.top);


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
            while (Sibling && (Sibling->ExStyle & WS_EX_TOPMOST))
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
      DPRINT("IntCreateWindowEx(): send CREATE message failed.\n");
      RETURN((HWND)0);
   }


   /* Send move and size messages. */
   if (!(Window->Flags & WINDOWOBJECT_NEED_SIZE))
   {
      LONG lParam;

      DPRINT("IntCreateWindow(): About to send WM_SIZE\n");

      if ((Window->ClientRect.right - Window->ClientRect.left) < 0 ||
            (Window->ClientRect.bottom - Window->ClientRect.top) < 0)
      {
         DPRINT("Sending bogus WM_SIZE\n");
      }


      lParam = MAKE_LONG(Window->ClientRect.right -
                         Window->ClientRect.left,
                         Window->ClientRect.bottom -
                         Window->ClientRect.top);
      co_IntSendMessage(Window->hSelf, WM_SIZE, SIZE_RESTORED,
                        lParam);


      DPRINT("IntCreateWindow(): About to send WM_MOVE\n");

      if (0 != (Window->Style & WS_CHILD) && ParentWindow)
      {
         lParam = MAKE_LONG(Window->ClientRect.left - ParentWindow->ClientRect.left,
                            Window->ClientRect.top - ParentWindow->ClientRect.top);
      }
      else
      {
         lParam = MAKE_LONG(Window->ClientRect.left,
                            Window->ClientRect.top);
      }


      co_IntSendMessage(Window->hSelf, WM_MOVE, 0, lParam);


      /* Call WNDOBJ change procs */
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);


   }

   /* Show or maybe minimize or maximize the window. */
   if (Window->Style & (WS_MINIMIZE | WS_MAXIMIZE))
   {
      RECT NewPos;
      UINT16 SwFlag;

      SwFlag = (Window->Style & WS_MINIMIZE) ? SW_MINIMIZE :
               SW_MAXIMIZE;
      co_WinPosMinMaximize(Window, SwFlag, &NewPos);
      SwFlag =
         ((Window->Style & WS_CHILD) || UserGetActiveWindow()) ?
         SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
         SWP_NOZORDER | SWP_FRAMECHANGED;
      DPRINT("IntCreateWindow(): About to minimize/maximize\n");
      DPRINT("%d,%d %dx%d\n", NewPos.left, NewPos.top, NewPos.right, NewPos.bottom);
      co_WinPosSetWindowPos(Window, 0, NewPos.left, NewPos.top,
                            NewPos.right, NewPos.bottom, SwFlag);
   }

   /* Notify the parent window of a new child. */
   if ((Window->Style & WS_CHILD) &&
         (!(Window->ExStyle & WS_EX_NOPARENTNOTIFY)) && ParentWindow)
   {
      DPRINT("IntCreateWindow(): About to notify parent\n");
      co_IntSendMessage(ParentWindow->hSelf,
                        WM_PARENTNOTIFY,
                        MAKEWPARAM(WM_CREATE, Window->IDMenu),
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
   if (Window->Style & WS_VSCROLL)
   {
      co_UserShowScrollBar(Window, SB_VERT, TRUE);
   }
   if (Window->Style & WS_HSCROLL)
   {
      co_UserShowScrollBar(Window, SB_HORZ, TRUE);
   }

   if (dwStyle & WS_VISIBLE)
   {
      DPRINT("IntCreateWindow(): About to show window\n");
      co_WinPosShowWindow(Window, dwShowMode);
   }

   DPRINT("IntCreateWindow(): = %X\n", hWnd);
   DPRINT("WindowObject->SystemMenu = 0x%x\n", Window->SystemMenu);
   RETURN(hWnd);

CLEANUP:
   if (Window) UserDerefObjectCo(Window);
   if (ParentWindow) UserDerefObjectCo(ParentWindow);
   if (!_ret_ && ti != NULL)
   {
       if (Class != NULL)
       {
           IntDereferenceClass(Class,
                               ti->Desktop,
                               ti->kpi);
       }
   }
   END_CLEANUP;
}

HWND STDCALL
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
                     BOOL bUnicodeWindow)
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
       return NULL;
   }

   /* safely copy the window name */
   if (NULL != UnsafeWindowName)
   {
      Status = IntSafeCopyUnicodeString(&WindowName, UnsafeWindowName);
      if (! NT_SUCCESS(Status))
      {
         if (! IS_ATOM(ClassName.Buffer))
         {
            RtlFreeUnicodeString(&ClassName);
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

   RtlFreeUnicodeString(&WindowName);
   if (! IS_ATOM(ClassName.Buffer))
   {
      RtlFreeUnicodeString(&ClassName);
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
HDWP STDCALL
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

   ASSERT_REFS_CO(Window); //fixme: temp hack?

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
   isChild = (0 != (Window->Style & WS_CHILD));

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
BOOLEAN STDCALL
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
STDCALL
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
DWORD STDCALL
NtUserEndDeferWindowPosEx(DWORD Unknown0,
                          DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserFillWindow(DWORD Unknown0,
                 DWORD Unknown1,
                 DWORD Unknown2,
                 DWORD Unknown3)
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
         if((!CheckWindowName || !RtlCompareUnicodeString(WindowName, &(Child->WindowName), TRUE)) &&
               (!ClassAtom || Child->Class->Atom == ClassAtom))
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
HWND STDCALL
NtUserFindWindowEx(HWND hwndParent,
                   HWND hwndChildAfter,
                   PUNICODE_STRING ucClassName,
                   PUNICODE_STRING ucWindowName)
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
       _SEH_TRY
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
                   _SEH_LEAVE;
               }

               if (!IntGetAtomFromStringOrAtom(&ClassName,
                                               &ClassAtom))
               {
                   _SEH_LEAVE;
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
       _SEH_HANDLE
       {
           SetLastNtError(_SEH_GetExceptionCode());
           _SEH_YIELD(RETURN(NULL));
       }
       _SEH_END;

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

   _SEH_TRY
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
                                   &WindowName, &TopLevelWindow->WindowName, TRUE);
                ClassMatches = (ClassAtom == (RTL_ATOM)0) ||
                               ClassAtom == TopLevelWindow->Class->Atom;

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
   _SEH_HANDLE
   {
       SetLastNtError(_SEH_GetExceptionCode());
       Ret = NULL;
   }
   _SEH_END;

   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserFindWindowEx, ret %i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserFlashWindowEx(DWORD Unknown0)
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
//               UserDerefObject(Parent);

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
HWND STDCALL
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

/*!
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \param hWnd window handle.
 * \param Rect pointer to the buffer where the coordinates are returned.
 *
*/
/*
 * @implemented
 */
BOOL STDCALL
NtUserGetClientRect(HWND hWnd, LPRECT Rect)
{
   PWINDOW_OBJECT Window;
   RECT SafeRect;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetClientRect\n");
   UserEnterShared();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   IntGetClientRect(Window, &SafeRect);

   if(!NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
   {
      RETURN( FALSE);
   }
   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetClientRect, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
HWND STDCALL
NtUserGetDesktopWindow()
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetDesktopWindow\n");
   UserEnterShared();

   RETURN( IntGetDesktopWindow());

CLEANUP:
   DPRINT("Leave NtUserGetDesktopWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserGetInternalWindowPos(DWORD Unknown0,
                           DWORD Unknown1,
                           DWORD Unknown2)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
HWND STDCALL
NtUserGetLastActivePopup(HWND hWnd)
{
   /*
    * This code can't work, because hWndLastPopup member of WINDOW_OBJECT is
    * not changed anywhere.
    * -- Filip, 01/nov/2003
    */
#if 0
   PWINDOW_OBJECT Wnd;
   HWND hWndLastPopup;

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      return NULL;
   }

   hWndLastPopup = Wnd->hWndLastPopup;

   return hWndLastPopup;
#else

   return NULL;
#endif
}

/*
 * NtUserGetParent
 *
 * The NtUserGetParent function retrieves a handle to the specified window's
 * parent or owner.
 *
 * Remarks
 *    Note that, despite its name, this function can return an owner window
 *    instead of a parent window.
 *
 * Status
 *    @implemented
 */

HWND STDCALL
NtUserGetParent(HWND hWnd)
{
   PWINDOW_OBJECT Wnd, WndParent;
   HWND hWndParent = NULL;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetParent\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( NULL);
   }

   WndParent = IntGetParent(Wnd);
   if (WndParent)
   {
      hWndParent = WndParent->hSelf;
   }

   RETURN( hWndParent);

CLEANUP:
   DPRINT("Leave NtUserGetParent, ret=%i\n",_ret_);
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
      UserDerefObject(WndOldParent);
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

HWND STDCALL
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
 * NtUserGetShellWindow
 *
 * Returns a handle to shell window that was set by NtUserSetShellWindowEx.
 *
 * Status
 *    @implemented
 */

HWND STDCALL
NtUserGetShellWindow()
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetShellWindow\n");
   UserEnterShared();

   RETURN( UserGetShellWindow() );

CLEANUP:
   DPRINT("Leave NtUserGetShellWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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

BOOL STDCALL
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
   PWINSTATION_OBJECT WinStaObject;
   PWINDOW_OBJECT WndShell;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;
   NTSTATUS Status;

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

HMENU STDCALL
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

BOOL STDCALL
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

HWND STDCALL
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
   LONG Result = 0;

   DPRINT("NtUserGetWindowLong(%x,%d,%d)\n", hWnd, (INT)Index, Ansi);

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      return 0;
   }

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
      if ((Index + sizeof(LONG)) > Window->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }
      Result = *((LONG *)(Window->ExtraData + Index));
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            Result = Window->ExStyle;
            break;

         case GWL_STYLE:
            Result = Window->Style;
            break;

         case GWL_WNDPROC:
            Result = (LONG)IntGetWindowProc(Window,
                                            Ansi);
            break;

         case GWL_HINSTANCE:
            Result = (LONG) Window->Instance;
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
            Result = (LONG) Window->IDMenu;
            break;

         case GWL_USERDATA:
            Result = Window->UserData;
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

LONG STDCALL
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
    if (Window->IsSystem)
    {
        Ret = (Ansi ? Window->WndProcExtra : Window->WndProc);
    }
    else
    {
        if (!Ansi == Window->Unicode)
        {
            Ret = Window->WndProc;
        }
        else
        {
            /* allocate or update an existing call procedure handle to return
               the old window proc */
            if (Window->CallProc2 != NULL)
            {
                Window->CallProc2->WndProc = Window->WndProc;
                Window->CallProc2->Unicode = Window->Unicode;
            }
            else
            {
                Window->CallProc2 = CreateCallProc(Window->ti->Desktop,
                                                   Window->WndProc,
                                                   Window->Unicode,
                                                   Window->ti->kpi);
                if (Window->CallProc2 == NULL)
                {
                    SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
                    return NULL;
                }
            }

            Ret = GetCallProcHandle(Window->CallProc2);
        }
    }

    if (Window->Class->System)
    {
        BOOL SysWnd = Window->IsSystem;

        /* check if the new procedure matches with the one in the
           window class. If so, we need to restore both procedures! */
        Window->IsSystem = (NewWndProc == Window->Class->WndProc ||
                            NewWndProc == Window->Class->WndProcExtra);

        if (Window->IsSystem != SysWnd)
        {
            if (!Window->IsSystem && Window->CallProc != NULL)
            {
                /* destroy the callproc, we don't need it anymore */
                DestroyCallProc(Window->ti->Desktop,
                                Window->CallProc);
                Window->CallProc = NULL;
            }
        }

        if (Window->IsSystem)
        {
            Window->WndProc = Window->Class->WndProc;
            Window->WndProcExtra = Window->Class->WndProcExtra;
            Window->Unicode = !Ansi;
            return Ret;
        }
    }

    ASSERT(!Window->IsSystem);

    /* update the window procedure */
    Window->WndProc = NewWndProc;
    if (Window->CallProc != NULL)
    {
        Window->CallProc->WndProc = NewWndProc;
        Window->CallProc->Unicode = !Ansi;
    }
    Window->Unicode = !Ansi;

    return Ret;
}


LONG FASTCALL
co_UserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
   PWINDOW_OBJECT Window, Parent;
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

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Window->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return( 0);
      }
      OldValue = *((LONG *)(Window->ExtraData + Index));
      *((LONG *)(Window->ExtraData + Index)) = NewValue;
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
            WindowStation = ((PW32THREAD)Window->OwnerThread->Tcb.Win32Thread)->Desktop->WindowStation;
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
            OldValue = (LONG) Window->Style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            co_IntSendMessage(hWnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);
            Window->Style = (DWORD)Style.styleNew;
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
            OldValue = (LONG) Window->Instance;
            Window->Instance = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            Parent = Window->Parent;
            if (Parent && (Parent->hSelf == IntGetDesktopWindow()))
               OldValue = (LONG) IntSetOwner(Window->hSelf, (HWND) NewValue);
            else
               OldValue = (LONG) co_UserSetParent(Window->hSelf, (HWND) NewValue);
            break;

         case GWL_ID:
            OldValue = (LONG) Window->IDMenu;
            Window->IDMenu = (UINT) NewValue;
            break;

         case GWL_USERDATA:
            OldValue = Window->UserData;
            Window->UserData = NewValue;
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

LONG STDCALL
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

WORD STDCALL
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

   if (Index > Window->ExtraDataSize - sizeof(WORD))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN( 0);
   }

   OldValue = *((WORD *)(Window->ExtraData + Index));
   *((WORD *)(Window->ExtraData + Index)) = NewValue;

   RETURN( OldValue);

CLEANUP:
   DPRINT("Leave NtUserSetWindowWord, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL STDCALL
NtUserGetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
   PINTERNALPOS InternalPos;
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
   if (0 == (Window->Style & WS_VISIBLE))
   {
      Safepl.showCmd = SW_HIDE;
   }
   else if (0 != (Window->Flags & WINDOWOBJECT_RESTOREMAX) ||
            0 != (Window->Style & WS_MAXIMIZE))
   {
      Safepl.showCmd = SW_MAXIMIZE;
   }
   else if (0 != (Window->Style & WS_MINIMIZE))
   {
      Safepl.showCmd = SW_MINIMIZE;
   }
   else if (0 != (Window->Style & WS_VISIBLE))
   {
      Safepl.showCmd = SW_SHOWNORMAL;
   }

   Size.x = Window->WindowRect.left;
   Size.y = Window->WindowRect.top;
   InternalPos = WinPosInitInternalPos(Window, &Size,
                                       &Window->WindowRect);
   if (InternalPos)
   {
      Safepl.rcNormalPosition = InternalPos->NormalRect;
      Safepl.ptMinPosition = InternalPos->IconPos;
      Safepl.ptMaxPosition = InternalPos->MaxPos;
   }
   else
   {
      RETURN( FALSE);
   }

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


/*!
 * Return the dimension of the window in the screen coordinates.
 * \param hWnd window handle.
 * \param Rect pointer to the buffer where the coordinates are returned.
*/
/*
 * @implemented
 */
BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect)
{
   PWINDOW_OBJECT Wnd;
   NTSTATUS Status;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetWindowRect\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }
   Status = MmCopyToCaller(Rect, &Wnd->WindowRect, sizeof(RECT));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserGetWindowRect, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid)
{
   PWINDOW_OBJECT Wnd;
   DWORD tid, pid;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserGetWindowThreadProcessId\n");
   UserEnterShared();

   if (!(Wnd = UserGetWindowObject(hWnd)))
   {
      RETURN( 0);
   }

   tid = (DWORD)IntGetWndThreadId(Wnd);
   pid = (DWORD)IntGetWndProcessId(Wnd);

   if (UnsafePid)
      MmCopyToCaller(UnsafePid, &pid, sizeof(DWORD));

   RETURN( tid);

CLEANUP:
   DPRINT("Leave NtUserGetWindowThreadProcessId, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserLockWindowUpdate(DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
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
 4 = QWIsHung            Implements IsHungAppWindow found
                                by KJK::Hyperion.

        9 = QWKillWindow        When I called this with hWnd ==
                                DesktopWindow, it shutdown the system
                                and rebooted.
*/
/*
 * @implemented
 */
DWORD STDCALL
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
DWORD STDCALL
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
UINT STDCALL
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

   RtlFreeUnicodeString(&SafeMessageName);
   RETURN( Ret);

CLEANUP:
   DPRINT("Leave NtUserRegisterWindowMessage, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetImeOwnerWindow(DWORD Unknown0,
                        DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetInternalWindowPos(DWORD Unknown0,
                           DWORD Unknown1,
                           DWORD Unknown2,
                           DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;

}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetLayeredWindowAttributes(DWORD Unknown0,
                                 DWORD Unknown1,
                                 DWORD Unknown2,
                                 DWORD Unknown3)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetLogonNotifyWindow(DWORD Unknown0)
{
   UNIMPLEMENTED

   return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
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
 * @unimplemented
 */
DWORD STDCALL
NtUserSetWindowFNID(DWORD Unknown0,
                    DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
}



/*
 * @implemented
 */
BOOL STDCALL
NtUserSetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWINDOW_OBJECT Window;
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

   if ((Window->Style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
   {
      co_WinPosSetWindowPos(Window, NULL,
                            Safepl.rcNormalPosition.left, Safepl.rcNormalPosition.top,
                            Safepl.rcNormalPosition.right - Safepl.rcNormalPosition.left,
                            Safepl.rcNormalPosition.bottom - Safepl.rcNormalPosition.top,
                            SWP_NOZORDER | SWP_NOACTIVATE);
   }

   /* FIXME - change window status */
   co_WinPosShowWindow(Window, Safepl.showCmd);

   if (Window->InternalPos == NULL)
      Window->InternalPos = ExAllocatePoolWithTag(PagedPool, sizeof(INTERNALPOS), TAG_WININTLIST);
   Window->InternalPos->NormalRect = Safepl.rcNormalPosition;
   Window->InternalPos->IconPos = Safepl.ptMinPosition;
   Window->InternalPos->MaxPos = Safepl.ptMaxPosition;

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
BOOL STDCALL
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

   if(!Window)
   {
      return ERROR;
   }
   if(!hRgn)
   {
      return ERROR;
   }

   /* Create a new window region using the window rectangle */
   VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
   NtGdiOffsetRgn(VisRgn, -Window->WindowRect.left, -Window->WindowRect.top);
   /* if there's a region assigned to the window, combine them both */
   if(Window->WindowRegion && !(Window->Style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, Window->WindowRegion, RGN_AND);
   /* Copy the region into hRgn */
   NtGdiCombineRgn(hRgn, VisRgn, NULL, RGN_COPY);

   if((pRgn = RGNDATA_LockRgn(hRgn)))
   {
      Ret = pRgn->rdh.iType;
      RGNDATA_UnlockRgn(pRgn);
   }
   else
      Ret = ERROR;

   NtGdiDeleteObject(VisRgn);

   return Ret;
}

INT FASTCALL
IntGetWindowRgnBox(PWINDOW_OBJECT Window, RECT *Rect)
{
   INT Ret;
   HRGN VisRgn;
   ROSRGNDATA *pRgn;

   if(!Window)
   {
      return ERROR;
   }
   if(!Rect)
   {
      return ERROR;
   }

   /* Create a new window region using the window rectangle */
   VisRgn = UnsafeIntCreateRectRgnIndirect(&Window->WindowRect);
   NtGdiOffsetRgn(VisRgn, -Window->WindowRect.left, -Window->WindowRect.top);
   /* if there's a region assigned to the window, combine them both */
   if(Window->WindowRegion && !(Window->Style & WS_MINIMIZE))
      NtGdiCombineRgn(VisRgn, VisRgn, Window->WindowRegion, RGN_AND);

   if((pRgn = RGNDATA_LockRgn(VisRgn)))
   {
      Ret = pRgn->rdh.iType;
      *Rect = pRgn->rdh.rcBound;
      RGNDATA_UnlockRgn(pRgn);
   }
   else
      Ret = ERROR;

   NtGdiDeleteObject(VisRgn);

   return Ret;
}


/*
 * @implemented
 */
INT STDCALL
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
      NtGdiDeleteObject(Window->WindowRegion);
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
BOOL STDCALL
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
BOOL STDCALL
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
DWORD STDCALL
NtUserUpdateLayeredWindow(DWORD Unknown0,
                          DWORD Unknown1,
                          DWORD Unknown2,
                          DWORD Unknown3,
                          DWORD Unknown4,
                          DWORD Unknown5,
                          DWORD Unknown6,
                          DWORD Unknown7,
                          DWORD Unknown8)
{
   UNIMPLEMENTED

   return 0;
}




/*
 *    @implemented
 */
HWND STDCALL
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
      USHORT Hit;

      pt.x = X;
      pt.y = Y;

      //hmm... threads live on desktops thus we have a reference on the desktop and indirectly the desktop window
      //its possible this referencing is useless, thou it shouldnt hurt...
      UserRefObjectCo(DesktopWindow, &Ref);

      Hit = co_WinPosWindowFromPoint(DesktopWindow, PsGetCurrentThreadWin32Thread()->MessageQueue, &pt, &Window);

      if(Window)
      {
         Ret = Window->hSelf;

         RETURN( Ret);
      }
   }

   RETURN( NULL);

CLEANUP:
   if (Window) UserDerefObject(Window);
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

BOOL STDCALL
NtUserDefSetText(HWND hWnd, PUNICODE_STRING WindowText)
{
   PWINDOW_OBJECT Window;
   UNICODE_STRING SafeText;
   NTSTATUS Status;
   DECLARE_RETURN(INT);

   DPRINT("Enter NtUserDefSetText\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if(WindowText)
   {
      Status = IntSafeCopyUnicodeString(&SafeText, WindowText);
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( FALSE);
      }
   }
   else
   {
      RtlInitUnicodeString(&SafeText, NULL);
   }

   /* FIXME - do this thread-safe! otherwise one could crash here! */
   RtlFreeUnicodeString(&Window->WindowName);

   Window->WindowName = SafeText;

   /* Send shell notifications */
   if (!IntGetOwner(Window) && !IntGetParent(Window))
   {
      co_IntShellHookNotify(HSHELL_REDRAW, (LPARAM) hWnd);
   }

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserDefSetText, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserInternalGetWindowText
 *
 * Status
 *    @implemented
 */

INT STDCALL
NtUserInternalGetWindowText(HWND hWnd, LPWSTR lpString, INT nMaxCount)
{
   PWINDOW_OBJECT Window;
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

   /* FIXME - do this thread-safe! otherwise one could crash here! */
   Result = Window->WindowName.Length / sizeof(WCHAR);
   if(lpString)
   {
      const WCHAR Terminator = L'\0';
      INT Copy;
      WCHAR *Buffer = (WCHAR*)lpString;

      Copy = min(nMaxCount - 1, Result);
      if(Copy > 0)
      {
         Status = MmCopyToCaller(Buffer, Window->WindowName.Buffer, Copy * sizeof(WCHAR));
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

#define WIN_NEEDS_SHOW_OWNEDPOPUP (0x00000040)

BOOL
FASTCALL
IntShowOwnedPopups(PWINDOW_OBJECT OwnerWnd, BOOL fShow )
{
   int count = 0;
   PWINDOW_OBJECT pWnd;
   HWND *win_array;

   ASSERT(OwnerWnd);

   win_array = IntWinListChildren(OwnerWnd);//faxme: use desktop?

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
         if (pWnd->Style & WS_VISIBLE)
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
STDCALL
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
