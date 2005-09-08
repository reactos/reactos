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

static WndProcHandle *WndProcHandlesArray = 0;
static WORD WndProcHandlesArraySize = 0;
#define WPH_SIZE 0x40 /* the size to add to the WndProcHandle array each time */

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
   WndProcHandlesArray = ExAllocatePoolWithTag(PagedPool,WPH_SIZE * sizeof(WndProcHandle), TAG_WINPROCLST);
   WndProcHandlesArraySize = WPH_SIZE;
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
   ExFreePool(WndProcHandlesArray);
   WndProcHandlesArray = 0;
   WndProcHandlesArraySize = 0;
   return STATUS_SUCCESS;
}

/* HELPER FUNCTIONS ***********************************************************/

VOID FASTCALL IntReleaseWindowObject(PWINDOW_OBJECT Window)
{
 /*
 ASSERT(Window);

   ASSERT(USER_BODY_TO_HEADER(Window)->RefCount >= 1);

   USER_BODY_TO_HEADER(Window)->RefCount--;

   if (USER_BODY_TO_HEADER(Window)->RefCount == 0 && USER_BODY_TO_HEADER(Window)->destroyed)
   {
   }
   */
   
   ObmDereferenceObject(Window);
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
   PWINDOW_OBJECT Window;
   
   if (!hWnd) return NULL;
   
   Window = (PWINDOW_OBJECT)UserGetObject(&gHandleTable, hWnd, otWindow);
   if (!Window)
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

   if (!(Window = IntGetWindowObject(hWnd)))
      return FALSE;

   IntReleaseWindowObject(Window);
   return TRUE;
}




PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd)
{
   HWND hWnd;

   if (Wnd->Style & WS_POPUP)
   {
      hWnd = Wnd->hOwner;
      return IntGetWindowObject(hWnd);
   }
   else if (Wnd->Style & WS_CHILD)
   {
      PWINDOW_OBJECT par;

      par = Wnd->Parent;
      if (par)
         IntReferenceWindowObject(par);
      return par;
      //return IntGetWindowObject(hWnd);
   }

   return NULL;
}

PWINDOW_OBJECT FASTCALL
IntGetOwner(PWINDOW_OBJECT Wnd)
{
   HWND hWnd;

   hWnd = Wnd->hOwner;

   return IntGetWindowObject(hWnd);
}

PWINDOW_OBJECT FASTCALL
IntGetParentObject(PWINDOW_OBJECT Wnd)
{
   PWINDOW_OBJECT par;

   par = Wnd->Parent;
   if (par)
      IntReferenceWindowObject(par);
   return par;
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
static void IntSendDestroyMsg(HWND Wnd)
{

   PWINDOW_OBJECT Window, Owner, Parent;
#if 0 /* FIXME */

   GUITHREADINFO info;

   if (GetGUIThreadInfo(GetCurrentThreadId(), &info))
   {
      if (Wnd == info.hwndCaret)
      {
         DestroyCaret();
      }
   }
#endif

   Window = IntGetWindowObject(Wnd);
   if (Window)
   {
      Owner = IntGetOwner(Window);
      if (!Owner)
      {
         Parent = IntGetParent(Window);
         if (!Parent)
            co_IntShellHookNotify(HSHELL_WINDOWDESTROYED, (LPARAM) Wnd);
         else
            IntReleaseWindowObject(Parent);
      }
      else
      {
         IntReleaseWindowObject(Owner);
      }

      IntReleaseWindowObject(Window);
   }

   /* The window could already be destroyed here */

   /*
    * Send the WM_DESTROY to the window.
    */

   co_IntSendMessage(Wnd, WM_DESTROY, 0, 0);

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
 */
static LRESULT co_IntDestroyWindow(PWINDOW_OBJECT Window,
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
   Window->Flags &= ~WS_VISIBLE;
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
               co_IntDestroyWindow(Child, ProcessData, ThreadData, SendMessages);
            IntReleaseWindowObject(Child);
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
         && (Menu = IntGetMenuObject((HMENU)Window->IDMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE, TRUE);
      Window->IDMenu = 0;
      IntReleaseMenuObject(Menu);
   }

   if(Window->SystemMenu
         && (Menu = IntGetMenuObject(Window->SystemMenu)))
   {
      IntDestroyMenuObject(Menu, TRUE, TRUE);
      Window->SystemMenu = (HMENU)0;
      IntReleaseMenuObject(Menu);
   }

   DceFreeWindowDCE(Window);    /* Always do this to catch orphaned DCs */
#if 0 /* FIXME */

   WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
   CLASS_RemoveWindow(Window->Class);
#endif

   IntUnlinkWindow(Window);

   IntReferenceWindowObject(Window);
   ObmDeleteObject(Window->hSelf, otWindow);

   IntDestroyScrollBars(Window);

   /* remove the window from the class object */
   RemoveEntryList(&Window->ClassListEntry);

   /* dereference the class */
   ClassDereferenceObject(Window->Class);
   Window->Class = NULL;

   if(Window->WindowRegion)
   {
      NtGdiDeleteObject(Window->WindowRegion);
   }

   RtlFreeUnicodeString(&Window->WindowName);

   IntReleaseWindowObject(Window);

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
   PLIST_ENTRY Current;
   PW32PROCESS Win32Process;
   PW32THREAD Win32Thread;
   PWINDOW_OBJECT *List, *pWnd;
   ULONG Cnt = 0;

   Win32Thread = Thread->Tcb.Win32Thread;
   Win32Process = (PW32PROCESS)Thread->ThreadsProcess->Win32Process;

   Current = Win32Thread->WindowListHead.Flink;
   while (Current != &(Win32Thread->WindowListHead))
   {
      Cnt++;
      Current = Current->Flink;
   }

   if(Cnt > 0)
   {
      List = ExAllocatePool(PagedPool, (Cnt + 1) * sizeof(PWINDOW_OBJECT));
      if(!List)
      {
         DPRINT("Not enough memory to allocate window handle list\n");
         return;
      }
      pWnd = List;
      Current = Win32Thread->WindowListHead.Flink;
      while (Current != &(Win32Thread->WindowListHead))
      {
         *pWnd = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);
         IntReferenceWindowObject(*pWnd);
         pWnd++;
         Current = Current->Flink;
      }
      *pWnd = NULL;

      for(pWnd = List; *pWnd; pWnd++)
      {
         co_UserDestroyWindow(*pWnd);
         IntReleaseWindowObject(*pWnd);
      }
      ExFreePool(List);
      return;
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
      W32Thread = PsGetWin32Thread();

      if(!W32Thread->Desktop)
         return NULL;

      if(Window->SystemMenu)
      {
         Menu = IntGetMenuObject(Window->SystemMenu);
         if(Menu)
         {
            IntDestroyMenuObject(Menu, FALSE, TRUE);
            Window->SystemMenu = (HMENU)0;
            IntReleaseMenuObject(Menu);
         }
      }

      if(W32Thread->Desktop->WindowStation->SystemMenuTemplate)
      {
         /* clone system menu */
         Menu = IntGetMenuObject(W32Thread->Desktop->WindowStation->SystemMenuTemplate);
         if(!Menu)
            return NULL;

         NewMenu = IntCloneMenu(Menu);
         if(NewMenu)
         {
            Window->SystemMenu = NewMenu->MenuInfo.Self;
            NewMenu->MenuInfo.Flags |= MF_SYSMENU;
            NewMenu->MenuInfo.Wnd = Window->hSelf;
            ret = NewMenu;
            //IntReleaseMenuObject(NewMenuObject);
         }
         IntReleaseMenuObject(Menu);
      }
      else
      {
         hSysMenu = UserCreateMenu(FALSE);
         if (NULL == SysMenu)
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
         if(!NewMenu)
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
         IntReleaseMenuObject(Menu);
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
IntIsChildWindow(HWND Parent, HWND Child)
{
   PWINDOW_OBJECT BaseWindow, Window;

   if(!(BaseWindow = UserGetWindowObject(Child)))
   {
      return FALSE;
   }

   Window = BaseWindow;
   while (Window)
   {
      if (Window->hSelf == Parent)
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
      IntReleaseWindowObject(WndOldOwner);
   }
   else
   {
      ret = 0;
   }

   if((WndNewOwner = IntGetWindowObject(hWndNewOwner)))
   {
      Wnd->hOwner = hWndNewOwner;
      IntReleaseWindowObject(WndNewOwner);
   }
   else
      Wnd->hOwner = NULL;

   IntReleaseWindowObject(Wnd);
   return ret;
}

PWINDOW_OBJECT FASTCALL
co_IntSetParent(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndNewParent)
{
   PWINDOW_OBJECT WndOldParent, Sibling, InsertAfter;
   HWND hWnd, hWndNewParent, hWndOldParent;
   BOOL WasVisible;
   BOOL MenuChanged;

   ASSERT(Wnd);
   ASSERT(WndNewParent);
   ASSERT_REFS_CO(Wnd);
   ASSERT_REFS_CO(WndNewParent);

   hWnd = Wnd->hSelf;
   hWndNewParent = WndNewParent->hSelf;

   /*
    * Windows hides the window first, then shows it again
    * including the WM_SHOWWINDOW messages and all
    */
   WasVisible = co_WinPosShowWindow(Wnd, SW_HIDE);

   /* Validate that window and parent still exist */
   if (!IntIsWindow(hWnd) || !IntIsWindow(hWndNewParent))
      return NULL;

   /* Window must belong to current process */
   if (Wnd->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
      return NULL;

   WndOldParent = IntGetParentObject(Wnd);
   hWndOldParent = (WndOldParent ? WndOldParent->hSelf : NULL);

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
         IntReferenceWindowObject(InsertAfter);
         IntLinkWindow(Wnd, WndNewParent, InsertAfter /*prev sibling*/);
         IntReleaseWindowObject(InsertAfter);
      }

      if (WndNewParent->hSelf != IntGetDesktopWindow()) /* a child window */
      {
         if (!(Wnd->Style & WS_CHILD))
         {
            //if ( Wnd->Menu ) DestroyMenu ( Wnd->menu );
            IntSetMenu(Wnd, NULL, &MenuChanged);
         }
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
   if(WndOldParent)
   {
      if(!IntIsWindow(WndOldParent->hSelf))
      {
         IntReleaseWindowObject(WndOldParent);
         return NULL;
      }

      /* don't dereference the window object here, it must be done by the caller
         of IntSetParent() */
      return WndOldParent;
   }
   return NULL;
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

   if(!(Window = IntGetWindowObject(IntGetDesktopWindow())))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
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
         IntReleaseWindowObject(Window);
         return TRUE;
      }
   }

   IntReleaseWindowObject(Window);
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

   /* FIXME handle bChildren */

   if(hwndParent)
   {
      PWINDOW_OBJECT Window, Child;
      if(!(Window = IntGetWindowObject(hwndParent)))
      {
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         return 0;
      }

      for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
      {
         if(dwCount++ < nBufSize && pWnd)
         {
            Status = MmCopyToCaller(pWnd++, &Child->hSelf, sizeof(HWND));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               break;
            }
         }
      }

      IntReleaseWindowObject(Window);
   }
   else if(dwThreadId)
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
      if(!(W32Thread = Thread->Tcb.Win32Thread))
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
   else
   {
      PDESKTOP_OBJECT Desktop;
      PWINDOW_OBJECT Window, Child;

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
      if(!(Window = IntGetWindowObject(Desktop->DesktopWindow)))
      {
         if(hDesktop)
            ObDereferenceObject(Desktop);
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return 0;
      }

      for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
      {
         if(dwCount++ < nBufSize && pWnd)
         {
            Status = MmCopyToCaller(pWnd++, &Child->hSelf, sizeof(HWND));
            if(!NT_SUCCESS(Status))
            {
               SetLastNtError(Status);
               break;
            }
         }
      }

      IntReleaseWindowObject(Window);
      if(hDesktop)
         ObDereferenceObject(Desktop);
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

   if(!(Parent = IntGetWindowObject(hwndParent)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
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
      IntReleaseWindowObject(Parent);
      return NULL;
   }

   Ret = Parent->hSelf;
   if((List = IntWinListChildren(Parent)))
   {
      for(phWnd = List; *phWnd; phWnd++)
      {
         PWINDOW_OBJECT Child;
         if((Child = IntGetWindowObject(*phWnd)))
         {
            if(!(Child->Style & WS_VISIBLE) && (uiFlags & CWP_SKIPINVISIBLE))
            {
               IntReleaseWindowObject(Child);
               continue;
            }
            if((Child->Style & WS_DISABLED) && (uiFlags & CWP_SKIPDISABLED))
            {
               IntReleaseWindowObject(Child);
               continue;
            }
            if((Child->ExStyle & WS_EX_TRANSPARENT) && (uiFlags & CWP_SKIPTRANSPARENT))
            {
               IntReleaseWindowObject(Child);
               continue;
            }
            if(IntPtInWindow(Child, Pt.x, Pt.y))
            {
               Ret = Child->hSelf;
               IntReleaseWindowObject(Child);
               break;
            }
            IntReleaseWindowObject(Child);
         }
      }
      ExFreePool(List);
   }

   IntReleaseWindowObject(Parent);
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
   PWNDCLASS_OBJECT Class;
   PWINDOW_OBJECT Window = NULL;
   PWINDOW_OBJECT ParentWindow = NULL, OwnerWindow;
   HWND ParentWindowHandle;
   HWND OwnerWindowHandle;
   PMENU_OBJECT SystemMenu;
   HANDLE Handle;
   POINT Pos;
   SIZE Size;
#if 0

   POINT MaxSize, MaxPos, MinTrack, MaxTrack;
#else

   POINT MaxPos;
#endif

   CREATESTRUCTW Cs;
   CBT_CREATEWNDW CbtCreate;
   LRESULT Result;
   BOOL MenuChanged;
   BOOL ClassFound;
   DECLARE_RETURN(HWND);
   BOOL HasOwner;

   ParentWindowHandle = PsGetWin32Thread()->Desktop->DesktopWindow;
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
      if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
         ParentWindowHandle = hWndParent;
      else
         OwnerWindowHandle = UserGetAncestor(hWndParent, GA_ROOT);
   }
   else if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      RETURN( (HWND)0);  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
   }

   if (NULL != ParentWindowHandle)
   {
      ParentWindow = UserGetWindowObject(ParentWindowHandle);

      if (ParentWindow)
         UserRefObjectCo(ParentWindow);
   }
   else
   {
      ParentWindow = NULL;
   }

   /* FIXME: parent must belong to the current process */

   /* Check the class. */
   ClassFound = ClassReferenceClassByNameOrAtom(&Class, ClassName->Buffer, hInstance);
   if (!ClassFound)
   {
      if (IS_ATOM(ClassName->Buffer))
      {
         DPRINT1("Class 0x%x not found\n", (DWORD_PTR) ClassName->Buffer);
      }
      else
      {
         DPRINT1("Class %wZ not found\n", ClassName);
      }

      SetLastWin32Error(ERROR_CANNOT_FIND_WND_CLASS);
      RETURN((HWND)0);
   }

   /* Check the window station. */
   if (PsGetWin32Thread()->Desktop == NULL)
   {
      ClassDereferenceObject(Class);

      DPRINT("Thread is not attached to a desktop! Cannot create window!\n");
      RETURN( (HWND)0);
   }
   WinSta = PsGetWin32Thread()->Desktop->WindowStation;
   ObReferenceObjectByPointer(WinSta, KernelMode, ExWindowStationObjectType, 0);

   /* Create the window object. */
   Window = (PWINDOW_OBJECT)
            ObmCreateObject(&gHandleTable, &Handle,
                            otWindow, sizeof(WINDOW_OBJECT) + Class->cbWndExtra
                           );

   DPRINT("Created object with handle %X\n", Handle);
   if (!Window)
   {
      ObDereferenceObject(WinSta);
      ClassDereferenceObject(Class);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      RETURN( (HWND)0);
   }

   UserRefObjectCo(Window);


   ObDereferenceObject(WinSta);

   if (NULL == PsGetWin32Thread()->Desktop->DesktopWindow)
   {
      /* If there is no desktop window yet, we must be creating it */
      PsGetWin32Thread()->Desktop->DesktopWindow = Handle;
   }

   /*
    * Fill out the structure describing it.
    */
   Window->Class = Class;

   InsertTailList(&Class->ClassWindowsListHead, &Window->ClassListEntry);

   Window->ExStyle = dwExStyle;
   Window->Style = dwStyle & ~WS_VISIBLE;
   DPRINT("1: Style is now %lx\n", Window->Style);

   Window->SystemMenu = (HMENU)0;
   Window->ContextHelpId = 0;
   Window->IDMenu = 0;
   Window->Instance = hInstance;
   Window->hSelf = Handle;
   if (0 != (dwStyle & WS_CHILD))
   {
      Window->IDMenu = (UINT) hMenu;
   }
   else
   {
      IntSetMenu(Window, hMenu, &MenuChanged);
   }
   Window->MessageQueue = PsGetWin32Thread()->MessageQueue;
   IntReferenceMessageQueue(Window->MessageQueue);
   Window->Parent = ParentWindow;
   if((OwnerWindow = IntGetWindowObject(OwnerWindowHandle)))
   {
      Window->hOwner = OwnerWindowHandle;
      IntReleaseWindowObject(OwnerWindow);
      HasOwner = TRUE;
   }
   else
   {
      Window->hOwner = NULL;
      HasOwner = FALSE;
   }
   Window->UserData = 0;
   if ((((DWORD)Class->lpfnWndProcA & 0xFFFF0000) != 0xFFFF0000)
         && (((DWORD)Class->lpfnWndProcW & 0xFFFF0000) != 0xFFFF0000))
   {
      Window->Unicode = bUnicodeWindow;
   }
   else
   {
      Window->Unicode = Class->Unicode;
   }
   Window->WndProcA = Class->lpfnWndProcA;
   Window->WndProcW = Class->lpfnWndProcW;
   Window->OwnerThread = PsGetCurrentThread();
   Window->FirstChild = NULL;
   Window->LastChild = NULL;
   Window->PrevSibling = NULL;
   Window->NextSibling = NULL;

   /* extra window data */
   if (Class->cbWndExtra != 0)
   {
      Window->ExtraData = (PCHAR)(Window + 1);
      Window->ExtraDataSize = Class->cbWndExtra;
      RtlZeroMemory(Window->ExtraData, Window->ExtraDataSize);
   }
   else
   {
      Window->ExtraData = NULL;
      Window->ExtraDataSize = 0;
   }

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
         ClassDereferenceObject(Class);
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
      Window->Style |= WS_CLIPSIBLINGS;
      DPRINT("3: Style is now %lx\n", Window->Style);
      if (!(dwStyle & WS_POPUP))
      {
         Window->Style |= WS_CAPTION;
         Window->Flags |= WINDOWOBJECT_NEED_SIZE;
         DPRINT("4: Style is now %lx\n", Window->Style);
      }
   }

   /* create system menu */
   if((Window->Style & WS_SYSMENU) &&
         (Window->Style & WS_CAPTION) == WS_CAPTION)
   {
      SystemMenu = IntGetSystemMenu(Window, TRUE, TRUE);
      if(SystemMenu)
      {
         Window->SystemMenu = SystemMenu->MenuInfo.Self;
         IntReleaseMenuObject(SystemMenu);
      }
   }

   /* Insert the window into the thread's window list. */
   InsertTailList (&PsGetWin32Thread()->WindowListHead, &Window->ThreadListEntry);

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

   /* call hook */
   Cs.lpCreateParams = lpParam;
   Cs.hInstance = hInstance;
   Cs.hMenu = hMenu;
   Cs.hwndParent = ParentWindowHandle;
   Cs.cx = Size.cx;
   Cs.cy = Size.cy;
   Cs.x = Pos.x;
   Cs.y = Pos.y;
   Cs.style = dwStyle;
   Cs.lpszName = (LPCWSTR) WindowName;
   Cs.lpszClass = (LPCWSTR) ClassName;
   Cs.dwExStyle = dwExStyle;
   CbtCreate.lpcs = &Cs;
   CbtCreate.hwndInsertAfter = HWND_TOP;
   if (co_HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (WPARAM) Handle, (LPARAM) &CbtCreate))
   {

      /* FIXME - Delete window object and remove it from the thread windows list */
      /* FIXME - delete allocated DCE */

      ClassDereferenceObject(Class);
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

      IntGetDesktopWorkArea(Window->OwnerThread->Tcb.Win32Thread->Desktop, &WorkArea);

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

         /* According to wine, the ShowMode is set to y if x == CW_USEDEFAULT(16) and
            y is something else */
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
      if (MaxSize.x < nWidth)
         nWidth = MaxSize.x;
      if (MaxSize.y < nHeight)
         nHeight = MaxSize.y;
      if (nWidth < MinTrack.x )
         nWidth = MinTrack.x;
      if (nHeight < MinTrack.y )
         nHeight = MinTrack.y;
      if (nWidth < 0)
         nWidth = 0;
      if (nHeight < 0)
         nHeight = 0;
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
   DPRINT("IntCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);
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
         if((PrevSibling = ParentWindow->LastChild))
            IntReferenceWindowObject(PrevSibling);
         /* link window as bottom sibling */
         IntLinkWindow(Window, ParentWindow, PrevSibling /*prev sibling*/);
         if(PrevSibling)
            IntReleaseWindowObject(PrevSibling);
      }
      else
      {
         /* link window as top sibling (but after topmost siblings) */
         PWINDOW_OBJECT InsertAfter, Sibling;
         if (0 == (dwExStyle & WS_EX_TOPMOST))
         {
            InsertAfter = NULL;
            Sibling = ParentWindow->FirstChild;
            while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
            {
               InsertAfter = Sibling;
               Sibling = Sibling->NextSibling;
            }
         }
         else
         {
            InsertAfter = NULL;
         }
         if (NULL != InsertAfter)
         {
            IntReferenceWindowObject(InsertAfter);
         }
         IntLinkWindow(Window, ParentWindow, InsertAfter /* prev sibling */);
         if (NULL != InsertAfter)
         {
            IntReleaseWindowObject(InsertAfter);
         }
      }
   }

   /* Send the WM_CREATE message. */
   DPRINT("IntCreateWindowEx(): about to send CREATE message.\n");
   Result = co_IntSendMessage(Window->hSelf, WM_CREATE, 0, (LPARAM) &Cs);
   if (Result == (LRESULT)-1)
   {
      /* FIXME: Cleanup. */
      ClassDereferenceObject(Class);
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
      co_IntShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)Handle);
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

   DPRINT("IntCreateWindow(): = %X\n", Handle);
   DPRINT("WindowObject->SystemMenu = 0x%x\n", Window->SystemMenu);
   RETURN((HWND)Handle);

CLEANUP:
   if (Window)
      UserDerefObjectCo(Window);
   if (ParentWindow)
      UserDerefObjectCo(ParentWindow);

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
   if (! IS_ATOM(ClassName.Buffer))
   {
      Status = IntSafeCopyUnicodeStringTerminateNULL(&ClassName, UnsafeClassName);
      if (! NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         RETURN( NULL);
      }
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

   ASSERT_REFS_CO(Window);

   if (Window == NULL)
   {
      return FALSE;
   }

   /* Check for owner thread and desktop window */
   if ((Window->OwnerThread != PsGetCurrentThread()) || IntIsDesktopWindow(Window))
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

         Desktop = IntGetWindowObject(IntGetDesktopWindow());
         Children = IntWinListChildren(Desktop);
         IntReleaseWindowObject(Desktop);
         if (Children)
         {
            for (ChildHandle = Children; *ChildHandle; ++ChildHandle)
            {
               Child = IntGetWindowObject(*ChildHandle);
               if (Child == NULL)
                  continue;
               if (Child->hOwner != Window->hSelf)
               {
                  IntReleaseWindowObject(Child);
                  continue;
               }

               if (IntWndBelongsToThread(Child, PsGetWin32Thread()))
               {
                  co_UserDestroyWindow(Child);
                  IntReleaseWindowObject(Child);
                  GotOne = TRUE;
                  continue;
               }

               if (Child->hOwner != NULL)
               {
                  Child->hOwner = NULL;
               }

               IntReleaseWindowObject(Child);
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
   co_IntDestroyWindow(Window, PsGetWin32Process(), PsGetWin32Thread(), TRUE);

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

   DPRINT("Enter NtUserDestroyWindow\n");
   UserEnterExclusive();

   Window = IntGetWindowObject(Wnd);
   if (Window == NULL)
   {
      RETURN(FALSE);
   }

   RETURN(co_UserDestroyWindow(Window));

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


HWND FASTCALL
IntFindWindow(PWINDOW_OBJECT Parent,
              PWINDOW_OBJECT ChildAfter,
              RTL_ATOM ClassAtom,
              PUNICODE_STRING WindowName)
{
   BOOL CheckWindowName;
   HWND *List, *phWnd;
   HWND Ret = NULL;

   ASSERT(Parent);

   CheckWindowName = (WindowName && (WindowName->Length > 0));

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
         if(!(Child = IntGetWindowObject(*(phWnd++))))
         {
            continue;
         }

         /* Do not send WM_GETTEXT messages in the kernel mode version!
            The user mode version however calls GetWindowText() which will
            send WM_GETTEXT messages to windows belonging to its processes */
         if((!CheckWindowName || !RtlCompareUnicodeString(WindowName, &(Child->WindowName), FALSE)) &&
               (!ClassAtom || Child->Class->Atom == ClassAtom))
         {
            Ret = Child->hSelf;
            IntReleaseWindowObject(Child);
            break;
         }

         IntReleaseWindowObject(Child);
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
   UNICODE_STRING ClassName, WindowName;
   NTSTATUS Status;
   HWND Desktop, Ret = NULL;
   RTL_ATOM ClassAtom;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserFindWindowEx\n");
   UserEnterShared();

   Desktop = IntGetCurrentThreadDesktopWindow();

   if(hwndParent == NULL)
      hwndParent = Desktop;
   /* FIXME
   else if(hwndParent == HWND_MESSAGE)
   {
     hwndParent = IntGetMessageWindow();
   }
   */

   if(!(Parent = IntGetWindowObject(hwndParent)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( NULL);
   }

   ChildAfter = NULL;
   if(hwndChildAfter && !(ChildAfter = IntGetWindowObject(hwndChildAfter)))
   {
      IntReleaseWindowObject(Parent);
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( NULL);
   }

   /* copy the window name */
   Status = IntSafeCopyUnicodeString(&WindowName, ucWindowName);
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Cleanup3;
   }

   /* safely copy the class name */
   Status = MmCopyFromCaller(&ClassName, ucClassName, sizeof(UNICODE_STRING));
   if(!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Cleanup2;
   }
   if(ClassName.Length > 0 && ClassName.Buffer)
   {
      WCHAR *buf;
      /* safely copy the class name string (NULL terminated because class-lookup
         depends on it... */
      buf = ExAllocatePoolWithTag(PagedPool, ClassName.Length + sizeof(WCHAR), TAG_STRING);
      if(!buf)
      {
         SetLastWin32Error(STATUS_INSUFFICIENT_RESOURCES);
         goto Cleanup2;
      }
      Status = MmCopyFromCaller(buf, ClassName.Buffer, ClassName.Length);
      if(!NT_SUCCESS(Status))
      {
         ExFreePool(buf);
         SetLastNtError(Status);
         goto Cleanup2;
      }
      ClassName.Buffer = buf;
      /* make sure the string is null-terminated */
      buf += ClassName.Length / sizeof(WCHAR);
      *buf = L'\0';
   }

   /* find the class object */
   if(ClassName.Buffer)
   {
      PWINSTATION_OBJECT WinStaObject;

      if (PsGetWin32Thread()->Desktop == NULL)
      {
         SetLastWin32Error(ERROR_INVALID_HANDLE);
         goto Cleanup;
      }

      WinStaObject = PsGetWin32Thread()->Desktop->WindowStation;

      Status = RtlLookupAtomInAtomTable(
                  WinStaObject->AtomTable,
                  ClassName.Buffer,
                  &ClassAtom);

      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Failed to lookup class atom!\n");
         SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
         goto Cleanup;
      }
   }

   if(Parent->hSelf == Desktop)
   {
      HWND *List, *phWnd;
      PWINDOW_OBJECT TopLevelWindow;
      BOOLEAN CheckWindowName;
      BOOLEAN CheckClassName;
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

         CheckWindowName = WindowName.Length > 0;
         CheckClassName = ClassName.Buffer != NULL;

         /* search children */
         while(*phWnd)
         {
            if(!(TopLevelWindow = IntGetWindowObject(*(phWnd++))))
            {
               continue;
            }

            /* Do not send WM_GETTEXT messages in the kernel mode version!
               The user mode version however calls GetWindowText() which will
               send WM_GETTEXT messages to windows belonging to its processes */
            WindowMatches = !CheckWindowName || !RtlCompareUnicodeString(
                               &WindowName, &TopLevelWindow->WindowName, FALSE);
            ClassMatches = !CheckClassName ||
                           ClassAtom == TopLevelWindow->Class->Atom;

            if (WindowMatches && ClassMatches)
            {
               Ret = TopLevelWindow->hSelf;
               IntReleaseWindowObject(TopLevelWindow);
               break;
            }

            if (IntFindWindow(TopLevelWindow, NULL, ClassAtom, &WindowName))
            {
               /* window returns the handle of the top-level window, in case it found
                  the child window */
               Ret = TopLevelWindow->hSelf;
               IntReleaseWindowObject(TopLevelWindow);
               break;
            }

            IntReleaseWindowObject(TopLevelWindow);
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

      if((MsgWindows = IntGetWindowObject(IntGetMessageWindow())))
      {
         Ret = IntFindWindow(MsgWindows, ChildAfter, ClassAtom, &WindowName);
         IntReleaseWindowObject(MsgWindows);
      }
   }
#endif

Cleanup:
   if(ClassName.Length > 0 && ClassName.Buffer)
      ExFreePool(ClassName.Buffer);

Cleanup2:
   RtlFreeUnicodeString(&WindowName);

Cleanup3:
   if(ChildAfter)
      IntReleaseWindowObject(ChildAfter);
   IntReleaseWindowObject(Parent);

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
HWND FASTCALL UserGetAncestor(HWND hWnd, UINT Type)
{
   PWINDOW_OBJECT Wnd, WndAncestor, Parent;
   HWND hWndAncestor;

   if (hWnd == IntGetDesktopWindow())
   {
      return NULL;
   }

   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   switch (Type)
   {
      case GA_PARENT:
         {
            WndAncestor = IntGetParentObject(Wnd);
            break;
         }

      case GA_ROOT:
         {
            PWINDOW_OBJECT tmp;
            WndAncestor = Wnd;
            Parent = NULL;

            for(;;)
            {
               tmp = Parent;
               if(!(Parent = IntGetParentObject(WndAncestor)))
               {
                  break;
               }
               if(IntIsDesktopWindow(Parent))
               {
                  IntReleaseWindowObject(Parent);
                  break;
               }
               if(tmp)
                  IntReleaseWindowObject(tmp);
               WndAncestor = Parent;
            }
            break;
         }

      case GA_ROOTOWNER:
         {
            WndAncestor = Wnd;
            IntReferenceWindowObject(WndAncestor);
            for (;;)
            {
               PWINDOW_OBJECT Old;
               Old = WndAncestor;
               Parent = IntGetParent(WndAncestor);
               IntReleaseWindowObject(Old);
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
            IntReleaseWindowObject(Wnd);
            return NULL;
         }
   }

   hWndAncestor = (WndAncestor ? WndAncestor->hSelf : NULL);
   IntReleaseWindowObject(Wnd);

   if(WndAncestor && (WndAncestor != Wnd))
      IntReleaseWindowObject(WndAncestor);

   return hWndAncestor;
}



/*
 * @implemented
 */
HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Type)
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetAncestor\n");
   UserEnterExclusive();

   RETURN(UserGetAncestor(hWnd, Type));

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

   if(!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( FALSE);
   }

   IntGetClientRect(Window, &SafeRect);
   IntReleaseWindowObject(Window);

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

   IntAcquireWinLockShared();

   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      IntReleaseWinLock();
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   hWndLastPopup = Wnd->hWndLastPopup;

   IntReleaseWinLock();

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

   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( NULL);
   }

   WndParent = IntGetParent(Wnd);
   if (WndParent)
   {
      hWndParent = WndParent->hSelf;
      IntReleaseWindowObject(WndParent);
   }

   IntReleaseWindowObject(Wnd);

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
      if (!(WndParent = IntGetWindowObject(hWndNewParent)))
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return( NULL);
      }
   }
   else
   {
      if (!(WndParent = IntGetWindowObject(IntGetDesktopWindow())))
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return( NULL);
      }
   }

   if (!(Wnd = IntGetWindowObject(hWndChild)))
   {
      IntReleaseWindowObject(WndParent);
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return( NULL);
   }

   WndOldParent = co_IntSetParent(Wnd, WndParent);

   if (WndOldParent)
   {
      hWndOldParent = WndOldParent->hSelf;
      IntReleaseWindowObject(WndOldParent);
   }

   IntReleaseWindowObject(Wnd);
   IntReleaseWindowObject(WndParent);

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

   DPRINT("Enter NtUserSetShellWindowEx\n");
   UserEnterExclusive();

   if (!(WndShell = UserGetWindowObject(hwndShell)))
   {
      RETURN(FALSE);
   }

   NTSTATUS Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
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

   UserRefObjectCo(WndShell);
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

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( FALSE);
   }

   if (hMenu)
   {
      /*
       * Assign new menu handle.
       */
      Menu = IntGetMenuObject(hMenu);
      if (!Menu)
      {
         IntReleaseWindowObject(Window);
         SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
         RETURN( FALSE);
      }

      Result = IntSetSystemMenu(Window, Menu);

      IntReleaseMenuObject(Menu);
   }

   IntReleaseWindowObject(Window);

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
         if((Parent = IntGetWindowObject(Window->hOwner)))
         {
            hWndResult = Parent->hSelf;
            IntReleaseWindowObject(Parent);
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

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
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
            if (Ansi)
               Result = (LONG) Window->WndProcA;
            else
               Result = (LONG) Window->WndProcW;
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

   IntReleaseWindowObject(Window);

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

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return( 0);
   }

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > Window->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         IntReleaseWindowObject(Window);
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
            WindowStation = Window->OwnerThread->Tcb.Win32Thread->Desktop->WindowStation;
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
            /* FIXME: should check if window belongs to current process */
            if (Ansi)
            {
               OldValue = (LONG) Window->WndProcA;
               Window->WndProcA = (WNDPROC) NewValue;
               Window->WndProcW = (WNDPROC) IntAddWndProcHandle((WNDPROC)NewValue,FALSE);
               Window->Unicode = FALSE;
            }
            else
            {
               OldValue = (LONG) Window->WndProcW;
               Window->WndProcW = (WNDPROC) NewValue;
               Window->WndProcA = (WNDPROC) IntAddWndProcHandle((WNDPROC)NewValue,TRUE);
               Window->Unicode = TRUE;
            }
            break;

         case GWL_HINSTANCE:
            OldValue = (LONG) Window->Instance;
            Window->Instance = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            Parent = IntGetParentObject(Window);
            if (Parent && (Parent->hSelf == IntGetDesktopWindow()))
               OldValue = (LONG) IntSetOwner(Window->hSelf, (HWND) NewValue);
            else
               OldValue = (LONG) co_UserSetParent(Window->hSelf, (HWND) NewValue);
            if(Parent)
               IntReleaseWindowObject(Parent);
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

   IntReleaseWindowObject(Window);

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

   switch (Index)
   {
      case GWL_ID:
      case GWL_HINSTANCE:
      case GWL_HWNDPARENT:
         RETURN( co_UserSetWindowLong(hWnd, Index, (UINT)NewValue, TRUE));
      default:
         if (Index < 0)
         {
            SetLastWin32Error(ERROR_INVALID_INDEX);
            RETURN( 0);
         }
   }

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( 0);
   }

   if (Index > Window->ExtraDataSize - sizeof(WORD))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      IntReleaseWindowObject(Window);
      RETURN( 0);
   }

   OldValue = *((WORD *)(Window->ExtraData + Index));
   *((WORD *)(Window->ExtraData + Index)) = NewValue;

   IntReleaseWindowObject(Window);

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
   Safepl.showCmd = ((Window->Flags & WINDOWOBJECT_RESTOREMAX) ? SW_MAXIMIZE : SW_SHOWNORMAL);

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
      UserRefObjectCo(Window);
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

   UserRefObjectCo(Window);

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

   DPRINT("Enter NtUserSetWindowPos\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window);
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
      UserRefObjectCo(Window);
      co_UserRedrawWindow(Window, NULL, NULL, RDW_INVALIDATE);
      UserDerefObjectCo(Window);
   }

   RETURN( (INT)hRgn);

CLEANUP:
   DPRINT("Leave NtUserSystemParametersInfo, ret=%i\n",_ret_);
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

   DPRINT("Enter NtUserShowWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   UserRefObjectCo(Window);
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
DWORD STDCALL
NtUserShowWindowAsync(DWORD Unknown0,
                      DWORD Unknown1)
{
   UNIMPLEMENTED

   return 0;
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
 * @implemented
 */
VOID STDCALL
NtUserValidateRect(HWND hWnd, const RECT* Rect)
{
   return (VOID)NtUserRedrawWindow(hWnd, Rect, 0, RDW_VALIDATE | RDW_NOCHILDREN);
}


/*
 *    @implemented
 */
HWND STDCALL
NtUserWindowFromPoint(LONG X, LONG Y)
{
   POINT pt;
   HWND Ret;
   PWINDOW_OBJECT DesktopWindow, Window = NULL;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserWindowFromPoint\n");
   UserEnterExclusive();

   if ((DesktopWindow = IntGetWindowObject(IntGetDesktopWindow())))
   {
      USHORT Hit;

      pt.x = X;
      pt.y = Y;

      Hit = co_WinPosWindowFromPoint(DesktopWindow, PsGetWin32Thread()->MessageQueue, &pt, &Window);

      if(Window)
      {
         Ret = Window->hSelf;
         IntReleaseWindowObject(Window);
         IntReleaseWindowObject(DesktopWindow);
         RETURN( Ret);
      }

      IntReleaseWindowObject(DesktopWindow);
   }

   RETURN( NULL);

CLEANUP:
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
   PWINDOW_OBJECT Window, Parent, Owner;
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

   Owner = IntGetOwner(Window);
   Parent = IntGetParent(Window);

   if ((!Owner) && (!Parent))
   {
      co_IntShellHookNotify(HSHELL_REDRAW, (LPARAM) hWnd);
   }

   if (Owner)
   {
      IntReleaseWindowObject(Owner);
   }

   if (Parent)
   {
      IntReleaseWindowObject(Parent);
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

DWORD STDCALL
NtUserDereferenceWndProcHandle(WNDPROC wpHandle, WndProcHandle *Data)
{
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserDereferenceWndProcHandle\n");
   UserEnterShared();

   WndProcHandle Entry;
   if (((DWORD)wpHandle & 0xFFFF0000) == 0xFFFF0000)
   {
      Entry = WndProcHandlesArray[(DWORD)wpHandle & 0x0000FFFF];
      Data->WindowProc = Entry.WindowProc;
      Data->IsUnicode = Entry.IsUnicode;
      Data->ProcessID = Entry.ProcessID;
      RETURN(  TRUE);
   }
   else
   {
      RETURN(  FALSE);
   }
   RETURN( FALSE);

CLEANUP:
   DPRINT("Leave NtUserDereferenceWndProcHandle, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

DWORD
IntAddWndProcHandle(WNDPROC WindowProc, BOOL IsUnicode)
{
   WORD i;
   WORD FreeSpot = 0;
   BOOL found;
   WndProcHandle *OldArray;
   WORD OldArraySize;
   found = FALSE;
   for (i = 0;i < WndProcHandlesArraySize;i++)
   {
      if (WndProcHandlesArray[i].WindowProc == NULL)
      {
         FreeSpot = i;
         found = TRUE;
      }
   }
   if (!found)
   {
      OldArray = WndProcHandlesArray;
      OldArraySize = WndProcHandlesArraySize;
      WndProcHandlesArray = ExAllocatePoolWithTag(PagedPool,(OldArraySize + WPH_SIZE) * sizeof(WndProcHandle), TAG_WINPROCLST);
      WndProcHandlesArraySize = OldArraySize + WPH_SIZE;
      RtlCopyMemory(WndProcHandlesArray,OldArray,OldArraySize * sizeof(WndProcHandle));
      ExFreePool(OldArray);
      FreeSpot = OldArraySize + 1;
   }
   WndProcHandlesArray[FreeSpot].WindowProc = WindowProc;
   WndProcHandlesArray[FreeSpot].IsUnicode = IsUnicode;
   WndProcHandlesArray[FreeSpot].ProcessID = PsGetCurrentProcessId();
   return FreeSpot + 0xFFFF0000;
}

DWORD
IntRemoveWndProcHandle(WNDPROC Handle)
{
   WORD position;
   position = (DWORD)Handle & 0x0000FFFF;
   if (position > WndProcHandlesArraySize)
   {
      return FALSE;
   }
   WndProcHandlesArray[position].WindowProc = NULL;
   WndProcHandlesArray[position].IsUnicode = FALSE;
   WndProcHandlesArray[position].ProcessID = NULL;
   return TRUE;
}

DWORD
IntRemoveProcessWndProcHandles(HANDLE ProcessID)
{
   WORD i;
   for (i = 0;i < WndProcHandlesArraySize;i++)
   {
      if (WndProcHandlesArray[i].ProcessID == ProcessID)
      {
         WndProcHandlesArray[i].WindowProc = NULL;
         WndProcHandlesArray[i].IsUnicode = FALSE;
         WndProcHandlesArray[i].ProcessID = NULL;
      }
   }
   return TRUE;
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


/* EOF */
