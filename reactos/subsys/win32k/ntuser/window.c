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
/* $Id: window.c,v 1.239 2004/06/20 00:45:37 navaraf Exp $
 *
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

static WndProcHandle *WndProcHandlesArray = 0;
static WORD WndProcHandlesArraySize = 0;
#define WPH_SIZE 0x40 /* the size to add to the WndProcHandle array each time */

/* dialog resources appear to pass this in 16 bits, handle them properly */
#define CW_USEDEFAULT16	(0x8000)

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

/*
 * IntGetProcessWindowObject
 *
 * Get window object from handle of specified process.
 */

PWINDOW_OBJECT FASTCALL
IntGetProcessWindowObject(PW32PROCESS ProcessData, HWND hWnd)
{
   PWINDOW_OBJECT WindowObject;
   NTSTATUS Status;

   Status = ObmReferenceObjectByHandle(ProcessData->WindowStation->HandleTable,
      hWnd, otWindow, (PVOID*)&WindowObject);
   if (!NT_SUCCESS(Status))
   {
      return NULL;
   }
   return WindowObject;
}


PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd)
{
  HWND hWnd;
  
  if (Wnd->Style & WS_POPUP)
  {
    IntLockRelatives(Wnd);
    hWnd = Wnd->Owner;
    IntUnLockRelatives(Wnd);
    return IntGetWindowObject(hWnd);
  }
  else if (Wnd->Style & WS_CHILD) 
  {
    IntLockRelatives(Wnd);
    hWnd = Wnd->Parent;
    IntUnLockRelatives(Wnd);
    return IntGetWindowObject(hWnd);
  }

  return NULL;
}


PWINDOW_OBJECT FASTCALL
IntGetParentObject(PWINDOW_OBJECT Wnd)
{
  HWND hParent;
  
  IntLockRelatives(Wnd);
  hParent = Wnd->Parent;
  IntUnLockRelatives(Wnd);
  return IntGetWindowObject(hParent);
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

   IntLockRelatives(Window);

   for (Child = Window->FirstChild; Child; Child = Child->NextSibling)
      ++NumChildren;
  
   List = ExAllocatePoolWithTag(PagedPool, (NumChildren + 1) * sizeof(HWND), TAG_WINLIST);
   if(!List)
   {
     DPRINT1("Failed to allocate memory for children array\n");
     IntUnLockRelatives(Window);
     SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
     return NULL;
   }
   for (Child = Window->FirstChild, Index = 0;
        Child != NULL;
        Child = Child->NextSibling, ++Index)
      List[Index] = Child->Self;
   List[Index] = NULL;

   IntUnLockRelatives(Window);

   return List;
}

/***********************************************************************
 *           IntSendDestroyMsg
 */
static void IntSendDestroyMsg(HWND Wnd)
{
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

  /*
   * Send the WM_DESTROY to the window.
   */
  IntSendMessage(Wnd, WM_DESTROY, 0, 0);

  /*
   * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
   * make sure that the window still exists when we come back.
   */
#if 0 /* FIXME */
  if (IsWindow(Wnd))
    {
      HWND* pWndArray;
      int i;

      if (!(pWndArray = WIN_ListChildren( hwnd ))) return;

      /* start from the end (FIXME: is this needed?) */
      for (i = 0; pWndArray[i]; i++) ;

      while (--i >= 0)
	{
	  if (IsWindow( pWndArray[i] )) WIN_SendDestroyMsg( pWndArray[i] );
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
static LRESULT IntDestroyWindow(PWINDOW_OBJECT Window,
                                PW32PROCESS ProcessData,
                                PW32THREAD ThreadData,
                                BOOLEAN SendMessages)
{
  HWND *Children;
  HWND *ChildHandle;
  PWINDOW_OBJECT Child;
  PMENU_OBJECT Menu;
  BOOL BelongsToThreadData;
  
  ASSERT(Window);

  RemoveTimersWindow(Window->Self);
  
  IntLockThreadWindows(Window->OwnerThread->Win32Thread);
  if(Window->Status & WINDOWSTATUS_DESTROYING)
  {
    IntUnLockThreadWindows(Window->OwnerThread->Win32Thread);
    DPRINT("Tried to call IntDestroyWindow() twice\n");
    return 0;
  }
  Window->Status |= WINDOWSTATUS_DESTROYING;
  /* remove the window already at this point from the thread window list so we
     don't get into trouble when destroying the thread windows while we're still
     in IntDestroyWindow() */
  RemoveEntryList(&Window->ThreadListEntry);
  IntUnLockThreadWindows(Window->OwnerThread->Win32Thread);
  
  BelongsToThreadData = IntWndBelongsToThread(Window, ThreadData);
  
  if(SendMessages)
  {
    /* Send destroy messages */
    IntSendDestroyMsg(Window->Self);
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
                IntSendDestroyMsg(Child->Self);
              }
              else
                IntDestroyWindow(Child, ProcessData, ThreadData, SendMessages);
              IntReleaseWindowObject(Child);
            }
        }
      ExFreePool(Children);
    }

  if (SendMessages)
    {      
      /*
       * Clear the update region to make sure no WM_PAINT messages will be
       * generated for this window while processing the WM_NCDESTROY.
       */ 
      IntRedrawWindow(Window, NULL, 0,
                      RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE |
                      RDW_NOINTERNALPAINT | RDW_NOCHILDREN);

      /*
       * Send the WM_NCDESTROY to the window being destroyed.
       */
      if(BelongsToThreadData)
        IntSendMessage(Window->Self, WM_NCDESTROY, 0, 0);
    }

  /* reset shell window handles */
  if(ProcessData->WindowStation)
  {
    if (Window->Self == ProcessData->WindowStation->ShellWindow)
      ProcessData->WindowStation->ShellWindow = NULL;

    if (Window->Self == ProcessData->WindowStation->ShellListView)
      ProcessData->WindowStation->ShellListView = NULL;
  }
  
  /* Unregister hot keys */
  UnregisterWindowHotKeys (Window);

  /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

#if 0 /* FIXME */
  WinPosCheckInternalPos(Window->Self);
  if (Window->Self == GetCapture())
    {
      ReleaseCapture();
    }

  /* free resources associated with the window */
  TIMER_RemoveWindowTimers(Window->Self);
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
  ObmCloseHandle(ProcessData->WindowStation->HandleTable, Window->Self);
  
  IntDestroyScrollBars(Window);
  
  IntLockThreadWindows(Window->OwnerThread->Win32Thread);
  Window->Status |= WINDOWSTATUS_DESTROYED;
  /* don't remove the WINDOWSTATUS_DESTROYING bit */
  IntUnLockThreadWindows(Window->OwnerThread->Win32Thread);
  
  /* remove the window from the class object */
  IntLockClassWindows(Window->Class);
  RemoveEntryList(&Window->ClassListEntry);
  IntUnLockClassWindows(Window->Class);
  
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
IntGetWindowBorderMeasures(PWINDOW_OBJECT WindowObject, INT *cx, INT *cy)
{
  if(HAS_DLGFRAME(WindowObject->Style, WindowObject->ExStyle) && !(WindowObject->Style & WS_MINIMIZE))
  {
    *cx = NtUserGetSystemMetrics(SM_CXDLGFRAME);
    *cy = NtUserGetSystemMetrics(SM_CYDLGFRAME);
  }
  else
  {
    if(HAS_THICKFRAME(WindowObject->Style, WindowObject->ExStyle)&& !(WindowObject->Style & WS_MINIMIZE))
    {
      *cx = NtUserGetSystemMetrics(SM_CXFRAME);
      *cy = NtUserGetSystemMetrics(SM_CYFRAME);
    }
    else if(HAS_THINFRAME(WindowObject->Style, WindowObject->ExStyle))
    {
      *cx = NtUserGetSystemMetrics(SM_CXBORDER);
      *cy = NtUserGetSystemMetrics(SM_CYBORDER);
    }
    else
    {
      *cx = *cy = 0;
    }
  }
}

BOOL FASTCALL
IntGetWindowInfo(PWINDOW_OBJECT WindowObject, PWINDOWINFO pwi)
{
  pwi->cbSize = sizeof(WINDOWINFO);
  pwi->rcWindow = WindowObject->WindowRect;
  pwi->rcClient = WindowObject->ClientRect;
  pwi->dwStyle = WindowObject->Style;
  pwi->dwExStyle = WindowObject->ExStyle;
  pwi->dwWindowStatus = (NtUserGetForegroundWindow() == WindowObject->Self); /* WS_ACTIVECAPTION */
  IntGetWindowBorderMeasures(WindowObject, &pwi->cxWindowBorders, &pwi->cyWindowBorders);
  pwi->atomWindowType = (WindowObject->Class ? WindowObject->Class->Atom : 0);
  pwi->wCreatorVersion = 0x400; /* FIXME - return a real version number */
  return TRUE;
}

static BOOL FASTCALL
IntSetMenu(
   PWINDOW_OBJECT WindowObject,
   HMENU Menu,
   BOOL *Changed)
{
  PMENU_OBJECT OldMenuObject, NewMenuObject;

  *Changed = (WindowObject->IDMenu != (UINT) Menu);
  if (! *Changed)
    {
      return TRUE;
    }

  if (0 != WindowObject->IDMenu)
    {
      OldMenuObject = IntGetMenuObject((HMENU) WindowObject->IDMenu);
      ASSERT(NULL == OldMenuObject || OldMenuObject->MenuInfo.Wnd == WindowObject->Self);
    }
  else
    {
      OldMenuObject = NULL;
    }

  if (NULL != Menu)
    {
      NewMenuObject = IntGetMenuObject(Menu);
      if (NULL == NewMenuObject)
        {
          if (NULL != OldMenuObject)
            {
              IntReleaseMenuObject(OldMenuObject);
            }
          SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
          return FALSE;
        }
      if (NULL != NewMenuObject->MenuInfo.Wnd)
        {
          /* Can't use the same menu for two windows */
          if (NULL != OldMenuObject)
            {
              IntReleaseMenuObject(OldMenuObject);
            }
          SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
          return FALSE;
        }
 
    }

  WindowObject->IDMenu = (UINT) Menu;
  if (NULL != NewMenuObject)
    {
      NewMenuObject->MenuInfo.Wnd = WindowObject->Self;
      IntReleaseMenuObject(NewMenuObject);
    }
  if (NULL != OldMenuObject)
    {
      OldMenuObject->MenuInfo.Wnd = NULL;
      IntReleaseMenuObject(OldMenuObject);
    }

  return TRUE;
}


/* INTERNAL ******************************************************************/


VOID FASTCALL
DestroyThreadWindows(struct _ETHREAD *Thread)
{
  PLIST_ENTRY Current;
  PW32PROCESS Win32Process;
  PW32THREAD Win32Thread;
  PWINDOW_OBJECT *List, *pWnd;
  ULONG Cnt = 0;

  Win32Thread = Thread->Win32Thread;
  Win32Process = Thread->ThreadsProcess->Win32Process;
  
  IntLockThreadWindows(Win32Thread);
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
      IntUnLockThreadWindows(Win32Thread);
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
    IntUnLockThreadWindows(Win32Thread);
    *pWnd = NULL;
    
    for(pWnd = List; *pWnd; pWnd++)
    {
      NtUserDestroyWindow((*pWnd)->Self);
      IntReleaseWindowObject(*pWnd);
    }
    ExFreePool(List);
    return;
  }
  
  IntUnLockThreadWindows(Win32Thread);
}


/*!
 * Internal function.
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \note Does not check the validity of the parameters
*/
VOID FASTCALL
IntGetClientRect(PWINDOW_OBJECT WindowObject, PRECT Rect)
{
  ASSERT( WindowObject );
  ASSERT( Rect );

  Rect->left = Rect->top = 0;
  Rect->right = WindowObject->ClientRect.right - WindowObject->ClientRect.left;
  Rect->bottom = WindowObject->ClientRect.bottom - WindowObject->ClientRect.top;
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
IntGetSystemMenu(PWINDOW_OBJECT WindowObject, BOOL bRevert, BOOL RetMenu)
{
  PMENU_OBJECT MenuObject, NewMenuObject, SysMenuObject, ret = NULL;
  PW32PROCESS W32Process;
  HMENU NewMenu, SysMenu;
  ROSMENUITEMINFO ItemInfo;

  if(bRevert)
  {
    W32Process = PsGetWin32Process();
    
    if(!W32Process->WindowStation)
      return NULL;
      
    if(WindowObject->SystemMenu)
    {
      MenuObject = IntGetMenuObject(WindowObject->SystemMenu);
      if(MenuObject)
      {
        IntDestroyMenuObject(MenuObject, FALSE, TRUE);
        WindowObject->SystemMenu = (HMENU)0;
        IntReleaseMenuObject(MenuObject);
      }
    }
      
    if(W32Process->WindowStation->SystemMenuTemplate)
    {
      /* clone system menu */
      MenuObject = IntGetMenuObject(W32Process->WindowStation->SystemMenuTemplate);
      if(!MenuObject)
        return NULL;

      NewMenuObject = IntCloneMenu(MenuObject);
      if(NewMenuObject)
      {
        WindowObject->SystemMenu = NewMenuObject->MenuInfo.Self;
        NewMenuObject->MenuInfo.Flags |= MF_SYSMENU;
        NewMenuObject->MenuInfo.Wnd = WindowObject->Self;
        ret = NewMenuObject;
        //IntReleaseMenuObject(NewMenuObject);
      }
      IntReleaseMenuObject(MenuObject);
    }
    else
    {
      SysMenu = NtUserCreateMenu(FALSE);
      if (NULL == SysMenu)
      {
        return NULL;
      }
      SysMenuObject = IntGetMenuObject(SysMenu);
      if (NULL == SysMenuObject)
      {
        NtUserDestroyMenu(SysMenu);
        return NULL;
      }
      SysMenuObject->MenuInfo.Flags |= MF_SYSMENU;
      SysMenuObject->MenuInfo.Wnd = WindowObject->Self;
      NewMenu = IntLoadSysMenuTemplate();
      if(!NewMenu)
      {
        IntReleaseMenuObject(SysMenuObject);
        NtUserDestroyMenu(SysMenu);
        return NULL;
      }
      MenuObject = IntGetMenuObject(NewMenu);
      if(!MenuObject)
      {
        IntReleaseMenuObject(SysMenuObject);
        NtUserDestroyMenu(SysMenu);
        return NULL;
      }
      
      NewMenuObject = IntCloneMenu(MenuObject);
      if(NewMenuObject)
      {
        NewMenuObject->MenuInfo.Flags |= MF_SYSMENU | MF_POPUP;
        IntReleaseMenuObject(NewMenuObject);
	NtUserSetMenuDefaultItem(NewMenuObject->MenuInfo.Self, SC_CLOSE, FALSE);

        ItemInfo.cbSize = sizeof(MENUITEMINFOW);
        ItemInfo.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_SUBMENU;
        ItemInfo.fType = MF_POPUP;
        ItemInfo.fState = MFS_ENABLED;
        ItemInfo.dwTypeData = NULL;
        ItemInfo.cch = 0;
        ItemInfo.hSubMenu = NewMenuObject->MenuInfo.Self;
        IntInsertMenuItem(SysMenuObject, (UINT) -1, TRUE, &ItemInfo);

        WindowObject->SystemMenu = SysMenuObject->MenuInfo.Self;

        ret = SysMenuObject;
      }
      IntDestroyMenuObject(MenuObject, FALSE, TRUE);
      IntReleaseMenuObject(MenuObject);
    }
    if(RetMenu)
      return ret;
    else
      return NULL;
  }
  else
  {
    if(WindowObject->SystemMenu)
      return IntGetMenuObject((HMENU)WindowObject->SystemMenu);
    else
      return NULL;
  }
}


BOOL FASTCALL
IntIsChildWindow(HWND Parent, HWND Child)
{
  PWINDOW_OBJECT BaseWindow, Window, Old;
  
  if(!(BaseWindow = IntGetWindowObject(Child)))
  {
    return FALSE;
  }
  
  Window = BaseWindow;
  while (Window)
  {
    if (Window->Self == Parent)
    {
      if(Window != BaseWindow)
        IntReleaseWindowObject(Window);
      IntReleaseWindowObject(BaseWindow);
      return(TRUE);
    }
    if(!(Window->Style & WS_CHILD))
    {
      if(Window != BaseWindow)
        IntReleaseWindowObject(Window);
      break;
    }
    Old = Window;
    Window = IntGetParentObject(Window);
    if(Old != BaseWindow)
      IntReleaseWindowObject(Old);
  }
  
  IntReleaseWindowObject(BaseWindow);
  return(FALSE);  
}

BOOL FASTCALL
IntIsWindowVisible(HWND hWnd)
{
  PWINDOW_OBJECT BaseWindow, Window, Old;
  
  if(!(BaseWindow = IntGetWindowObject(hWnd)))
  {
    return FALSE;
  }
  
  Window = BaseWindow;
  while(Window)
  {
    if(!(Window->Style & WS_CHILD))
    {
      break;
    }
    if(!(Window->Style & WS_VISIBLE))
    {
      if(Window != BaseWindow)
        IntReleaseWindowObject(Window);
      IntReleaseWindowObject(BaseWindow);
      return FALSE;
    }
    Old = Window;
    Window = IntGetParentObject(Window);
    if(Old != BaseWindow)
      IntReleaseWindowObject(Old);
  }
  
  if(Window)
  {
    if(Window->Style & WS_VISIBLE)
    {
      if(Window != BaseWindow)
        IntReleaseWindowObject(Window);
      IntReleaseWindowObject(BaseWindow);
      return TRUE;
    }
    if(Window != BaseWindow)
      IntReleaseWindowObject(Window);
  }
  IntReleaseWindowObject(BaseWindow);
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
  
  IntLockRelatives(Wnd);
  Wnd->Parent = WndParent->Self;
  if ((Wnd->PrevSibling = WndPrevSibling))
  {
    /* link after WndPrevSibling */
    if ((Wnd->NextSibling = WndPrevSibling->NextSibling))
      Wnd->NextSibling->PrevSibling = Wnd;
    else if ((Parent = IntGetWindowObject(Wnd->Parent)))
    {
      IntLockRelatives(Parent);
      if(Parent->LastChild == WndPrevSibling) 
        Parent->LastChild = Wnd;
      IntUnLockRelatives(Parent);
      IntReleaseWindowObject(Parent);
    }
    Wnd->PrevSibling->NextSibling = Wnd;
  }
  else
  {
    /* link at top */
    Parent = IntGetWindowObject(Wnd->Parent);
    if ((Wnd->NextSibling = WndParent->FirstChild)) 
      Wnd->NextSibling->PrevSibling = Wnd;
    else if (Parent)
    {
      IntLockRelatives(Parent);
      Parent->LastChild = Wnd;
      Parent->FirstChild = Wnd;
      IntUnLockRelatives(Parent);
      IntReleaseWindowObject(Parent);
      IntUnLockRelatives(Wnd);
      return;
    }
    if(Parent)
    {
      IntLockRelatives(Parent);
      Parent->FirstChild = Wnd;
      IntUnLockRelatives(Parent);
      IntReleaseWindowObject(Parent);
    }
  }
  IntUnLockRelatives(Wnd);
}

HWND FASTCALL
IntSetOwner(HWND hWnd, HWND hWndNewOwner)
{
  PWINDOW_OBJECT Wnd, WndOldOwner, WndNewOwner;
  HWND ret;
  
  Wnd = IntGetWindowObject(hWnd);
  if(!Wnd)
    return NULL;
  
  IntLockRelatives(Wnd);
  WndOldOwner = IntGetWindowObject(Wnd->Owner);
  if (WndOldOwner)
  {
     ret = WndOldOwner->Self;
     IntReleaseWindowObject(WndOldOwner);
  }
  else
  {
     ret = 0;
  }
  
  if((WndNewOwner = IntGetWindowObject(hWndNewOwner)))
  {
    Wnd->Owner = hWndNewOwner;
    IntReleaseWindowObject(WndNewOwner);
  }
  else
    Wnd->Owner = NULL;
  
  IntUnLockRelatives(Wnd);
  IntReleaseWindowObject(Wnd);
  return ret;
}

PWINDOW_OBJECT FASTCALL
IntSetParent(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndNewParent)
{
   PWINDOW_OBJECT WndOldParent, Sibling, InsertAfter;
   HWND hWnd, hWndNewParent, hWndOldParent;
   BOOL WasVisible;
   BOOL MenuChanged;
   
   ASSERT(Wnd);
   ASSERT(WndNewParent);
   
   hWnd = Wnd->Self;
   hWndNewParent = WndNewParent->Self;

   /*
    * Windows hides the window first, then shows it again
    * including the WM_SHOWWINDOW messages and all
    */
   WasVisible = WinPosShowWindow(hWnd, SW_HIDE);

   /* Validate that window and parent still exist */
   if (!IntIsWindow(hWnd) || !IntIsWindow(hWndNewParent))
      return NULL;

   /* Window must belong to current process */
   if (Wnd->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
      return NULL;

   WndOldParent = IntGetParentObject(Wnd);
   hWndOldParent = (WndOldParent ? WndOldParent->Self : NULL);

   if (WndNewParent != WndOldParent)
   {
      IntUnlinkWindow(Wnd);
      InsertAfter = NULL;
      if (0 == (Wnd->ExStyle & WS_EX_TOPMOST))
      {
        /* Not a TOPMOST window, put after TOPMOSTs of new parent */
        IntLockRelatives(WndNewParent);
        Sibling = WndNewParent->FirstChild;
        while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
        {
          InsertAfter = Sibling;
          Sibling = Sibling->NextSibling;
        }
        IntUnLockRelatives(WndNewParent);
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

      if (WndNewParent->Self != IntGetDesktopWindow()) /* a child window */
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
   WinPosSetWindowPos(hWnd, (0 == (Wnd->ExStyle & WS_EX_TOPMOST) ? HWND_TOP : HWND_TOPMOST),
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
     if(!IntIsWindow(WndOldParent->Self))
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
IntSetSystemMenu(PWINDOW_OBJECT WindowObject, PMENU_OBJECT MenuObject)
{
  PMENU_OBJECT OldMenuObject;
  if(WindowObject->SystemMenu)
  {
    OldMenuObject = IntGetMenuObject(WindowObject->SystemMenu);
    if(OldMenuObject)
    {
      OldMenuObject->MenuInfo.Flags &= ~ MF_SYSMENU;
      IntReleaseMenuObject(OldMenuObject);
    }
  }
  
  if(MenuObject)
  {
    /* FIXME check window style, propably return FALSE ? */
    WindowObject->SystemMenu = MenuObject->MenuInfo.Self;
    MenuObject->MenuInfo.Flags |= MF_SYSMENU;
  }
  else
    WindowObject->SystemMenu = (HMENU)0;
  
  return TRUE;
}


/* unlink the window from siblings and parent. children are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd)
{
  PWINDOW_OBJECT WndParent;
  
  IntLockRelatives(Wnd);
  if((WndParent = IntGetWindowObject(Wnd->Parent)))
  {
    IntLockRelatives(WndParent);
  }
  
  if (Wnd->NextSibling) Wnd->NextSibling->PrevSibling = Wnd->PrevSibling;
  else if (WndParent && WndParent->LastChild == Wnd) WndParent->LastChild = Wnd->PrevSibling;
 
  if (Wnd->PrevSibling) Wnd->PrevSibling->NextSibling = Wnd->NextSibling;
  else if (WndParent && WndParent->FirstChild == Wnd) WndParent->FirstChild = Wnd->NextSibling;
  
  if(WndParent)
  {
    IntUnLockRelatives(WndParent);
    IntReleaseWindowObject(WndParent);
  }
  Wnd->PrevSibling = Wnd->NextSibling = Wnd->Parent = NULL;
  IntUnLockRelatives(Wnd);
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
  
  IntLockRelatives(Window);
  for(Child = Window->FirstChild; Child; Child = Child->NextSibling)
  {
    if(Child->Owner && Child->Style & WS_VISIBLE)
    {
      /*
       * The desktop has a popup window if one of them has 
       * an owner window and is visible
       */
      IntUnLockRelatives(Window);
      IntReleaseWindowObject(Window);
      return TRUE;
    }
  }
  IntUnLockRelatives(Window);
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
    
    IntLockRelatives(Window);
    for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
    {
      if(dwCount++ < nBufSize && pWnd)
      {
        Status = MmCopyToCaller(pWnd++, &Child->Self, sizeof(HWND));
        if(!NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          break;
        }
      }
    }
    IntUnLockRelatives(Window);
    
    IntReleaseWindowObject(Window);
  }
  else if(dwThreadId)
  {
    PETHREAD Thread;
    PW32THREAD W32Thread;
    PLIST_ENTRY Current;
    PWINDOW_OBJECT *Window;
    
    Status = PsLookupThreadByThreadId((PVOID)dwThreadId, &Thread);
    if(!NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
    }
    if(!(W32Thread = Thread->Win32Thread))
    {
      ObDereferenceObject(Thread);
      DPRINT1("Thread is not a GUI Thread!\n");
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
    }
    
    IntLockThreadWindows(W32Thread);
    Current = W32Thread->WindowListHead.Flink;
    while(Current != &(W32Thread->WindowListHead))
    {
      *Window = CONTAINING_RECORD(Current, WINDOW_OBJECT, ThreadListEntry);
      ASSERT(*Window);
      
      if(dwCount < nBufSize && pWnd && ((*Window)->Style & WS_CHILD))
      {
        Status = MmCopyToCaller(pWnd++, &(*Window)->Self, sizeof(HWND));
        if(!NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          break;
        }
      }
      
      if(!((*Window)->Style & WS_CHILD))
      {
        dwCount++;
      }
      
      Current = Current->Flink;
    }
    IntUnLockThreadWindows(W32Thread);
    
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
    
    IntLockRelatives(Window);
    for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
    {
      if(dwCount++ < nBufSize && pWnd)
      {
        Status = MmCopyToCaller(pWnd++, &Child->Self, sizeof(HWND));
        if(!NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          break;
        }
      }
    }
    IntUnLockRelatives(Window);
    
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
  
  if(Parent->Self != IntGetDesktopWindow())
  {
    Pt.x += Parent->ClientRect.left;
    Pt.y += Parent->ClientRect.top;
  }
  
  if(!IntPtInWindow(Parent, Pt.x, Pt.y))
  {
    IntReleaseWindowObject(Parent);
    return NULL;
  }
  
  Ret = Parent->Self;
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
          Ret = Child->Self;
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
IntCalcDefPosSize(PWINDOW_OBJECT Parent, PWINDOW_OBJECT WindowObject, RECT *rc, BOOL IncPos)
{
  SIZE Sz;
  POINT Pos;
  
  if(Parent != NULL)
  {
    NtGdiIntersectRect(rc, rc, &Parent->ClientRect);
    
    if(IncPos)
    {
      Pos.x = Parent->TiledCounter * (NtUserGetSystemMetrics(SM_CXSIZE) + NtUserGetSystemMetrics(SM_CXFRAME));
      Pos.y = Parent->TiledCounter * (NtUserGetSystemMetrics(SM_CYSIZE) + NtUserGetSystemMetrics(SM_CYFRAME));
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
IntCreateWindowEx(DWORD dwExStyle,
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
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  PWINDOW_OBJECT WindowObject;
  PWINDOW_OBJECT ParentWindow, OwnerWindow;
  HWND ParentWindowHandle;
  HWND OwnerWindowHandle;
  PMENU_OBJECT SystemMenu;
  NTSTATUS Status;
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
  PWSTR ClassNameString;
  
  ParentWindowHandle = PsGetWin32Thread()->Desktop->DesktopWindow;
  OwnerWindowHandle = NULL;

  if (hWndParent == HWND_MESSAGE)
    {
      /*
       * native ole32.OleInitialize uses HWND_MESSAGE to create the
       * message window (style: WS_POPUP|WS_DISABLED)
       */
      UNIMPLEMENTED;
    }
  else if (hWndParent)
    {
      if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
        ParentWindowHandle = hWndParent;
      else
        OwnerWindowHandle = NtUserGetAncestor(hWndParent, GA_ROOT);
    }
  else if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
    {
      return (HWND)0;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
    }

  if (NULL != ParentWindowHandle)
    {
      ParentWindow = IntGetWindowObject(ParentWindowHandle);
    }
  else
    {
      ParentWindow = NULL;
    }
    
  /* FIXME: parent must belong to the current process */

  /* Check the class. */
  if (IS_ATOM(ClassName->Buffer))
    {
      ClassFound = ClassReferenceClassByNameOrAtom(&ClassObject, ClassName->Buffer, hInstance);
    }
  else
    {
      Status = IntUnicodeStringToNULLTerminated(&ClassNameString, ClassName);
      if (! NT_SUCCESS(Status))
        {
          if (NULL != ParentWindow)
            {
              IntReleaseWindowObject(ParentWindow);
            }
          return NULL;
        }
      ClassFound = ClassReferenceClassByNameOrAtom(&ClassObject, ClassNameString, hInstance);
      IntFreeNULLTerminatedFromUnicodeString(ClassNameString, ClassName);
    }
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
     if (NULL != ParentWindow)
     {
        IntReleaseWindowObject(ParentWindow);
     }
     return((HWND)0);
  }

  /* Check the window station. */
  DPRINT("IoGetCurrentProcess() %X\n", IoGetCurrentProcess());
  DPRINT("PROCESS_WINDOW_STATION %X\n", PROCESS_WINDOW_STATION());
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
					  KernelMode,
					  0,
					  &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      ClassDereferenceObject(ClassObject);
      if (NULL != ParentWindow)
        {
          IntReleaseWindowObject(ParentWindow);
        }
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return (HWND)0;
    }

  /* Create the window object. */
  WindowObject = (PWINDOW_OBJECT)
    ObmCreateObject(PsGetWin32Process()->WindowStation->HandleTable, &Handle,
        otWindow, sizeof(WINDOW_OBJECT) + ClassObject->cbWndExtra
        );

  DPRINT("Created object with handle %X\n", Handle);
  if (!WindowObject)
    {
      ObDereferenceObject(WinStaObject);
      ClassDereferenceObject(ClassObject);
      if (NULL != ParentWindow)
        {
          IntReleaseWindowObject(ParentWindow);
        }
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return (HWND)0;
    }
  ObDereferenceObject(WinStaObject);

  if (NULL == PsGetWin32Thread()->Desktop->DesktopWindow)
    {
      /* If there is no desktop window yet, we must be creating it */
      PsGetWin32Thread()->Desktop->DesktopWindow = Handle;
    }

  /*
   * Fill out the structure describing it.
   */
  WindowObject->Class = ClassObject;
  IntLockClassWindows(ClassObject);
  InsertTailList(&ClassObject->ClassWindowsListHead, &WindowObject->ClassListEntry);
  IntUnLockClassWindows(ClassObject);
  
  WindowObject->ExStyle = dwExStyle;
  WindowObject->Style = dwStyle & ~WS_VISIBLE;
  DPRINT("1: Style is now %lx\n", WindowObject->Style);
  
  WindowObject->SystemMenu = (HMENU)0;
  WindowObject->ContextHelpId = 0;
  WindowObject->IDMenu = 0;
  WindowObject->Instance = hInstance;
  WindowObject->Self = Handle;
  if (0 != (dwStyle & WS_CHILD))
    {
      WindowObject->IDMenu = (UINT) hMenu;
    }
  else
    {
      IntSetMenu(WindowObject, hMenu, &MenuChanged);
    }
  WindowObject->MessageQueue = PsGetWin32Thread()->MessageQueue;
  WindowObject->Parent = (ParentWindow ? ParentWindow->Self : NULL);
  if((OwnerWindow = IntGetWindowObject(OwnerWindowHandle)))
  {
    WindowObject->Owner = OwnerWindowHandle;
    IntReleaseWindowObject(OwnerWindow);
  }
  else
    WindowObject->Owner = NULL;
  WindowObject->UserData = 0;
  if ((((DWORD)ClassObject->lpfnWndProcA & 0xFFFF0000) != 0xFFFF0000)
      && (((DWORD)ClassObject->lpfnWndProcW & 0xFFFF0000) != 0xFFFF0000)) 
    {
      WindowObject->Unicode = bUnicodeWindow;
    }
  else
    {
      WindowObject->Unicode = ClassObject->Unicode;
    }
  WindowObject->WndProcA = ClassObject->lpfnWndProcA;
  WindowObject->WndProcW = ClassObject->lpfnWndProcW;
  WindowObject->OwnerThread = PsGetCurrentThread();
  WindowObject->FirstChild = NULL;
  WindowObject->LastChild = NULL;
  WindowObject->PrevSibling = NULL;
  WindowObject->NextSibling = NULL;

  /* extra window data */
  if (ClassObject->cbWndExtra != 0)
    {
      WindowObject->ExtraData = (PCHAR)(WindowObject + 1);
      WindowObject->ExtraDataSize = ClassObject->cbWndExtra;
      RtlZeroMemory(WindowObject->ExtraData, WindowObject->ExtraDataSize);
    }
  else
    {
      WindowObject->ExtraData = NULL;
      WindowObject->ExtraDataSize = 0;
    }

  InitializeListHead(&WindowObject->PropListHead);
  ExInitializeFastMutex(&WindowObject->PropListLock);
  ExInitializeFastMutex(&WindowObject->RelativesLock);
  ExInitializeFastMutex(&WindowObject->UpdateLock);

  if (NULL != WindowName->Buffer)
    {  
      WindowObject->WindowName.MaximumLength = WindowName->MaximumLength;
      WindowObject->WindowName.Length = WindowName->Length;
      WindowObject->WindowName.Buffer = ExAllocatePoolWithTag(PagedPool, WindowName->MaximumLength,
                                                              TAG_STRING);
      if (NULL == WindowObject->WindowName.Buffer)
        {
          ClassDereferenceObject(ClassObject);
          DPRINT1("Failed to allocate mem for window name\n");
          SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
          return NULL;
        }
      RtlCopyMemory(WindowObject->WindowName.Buffer, WindowName->Buffer, WindowName->MaximumLength);
    }
  else
    {
      RtlInitUnicodeString(&WindowObject->WindowName, NULL);
    }


  /* Correct the window style. */
  if (!(dwStyle & WS_CHILD))
    {
      WindowObject->Style |= WS_CLIPSIBLINGS;
      DPRINT("3: Style is now %lx\n", WindowObject->Style);
      if (!(dwStyle & WS_POPUP))
	{
	  WindowObject->Style |= WS_CAPTION;
          WindowObject->Flags |= WINDOWOBJECT_NEED_SIZE;
          DPRINT("4: Style is now %lx\n", WindowObject->Style);
	  /* FIXME: Note the window needs a size. */ 
	}
    }
  
  if(!(WindowObject->Style & (WS_POPUP | WS_CHILD)))
  {
    /* Automatically assign the caption and border style. Also always
       clip siblings for overlapped windows. */
    WindowObject->Style |= (WS_CAPTION | WS_BORDER | WS_CLIPSIBLINGS);
  }
  
  /* create system menu */
  if((WindowObject->Style & WS_SYSMENU) && (WindowObject->Style & WS_CAPTION))
  {
    SystemMenu = IntGetSystemMenu(WindowObject, TRUE, TRUE);
    if(SystemMenu)
    {
      WindowObject->SystemMenu = SystemMenu->MenuInfo.Self;
      IntReleaseMenuObject(SystemMenu);
    }
  }
  
  /* Insert the window into the thread's window list. */
  IntLockThreadWindows(PsGetWin32Thread());
  InsertTailList (&PsGetWin32Thread()->WindowListHead, 
		  &WindowObject->ThreadListEntry);
  IntUnLockThreadWindows(PsGetWin32Thread());

  /* Allocate a DCE for this window. */
  if (dwStyle & CS_OWNDC)
    {
      WindowObject->Dce = DceAllocDCE(WindowObject->Self, DCE_WINDOW_DC);
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
  if (HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (WPARAM) Handle, (LPARAM) &CbtCreate))
    {
      if (NULL != ParentWindow)
        {
          IntReleaseWindowObject(ParentWindow);
        }
      
      /* FIXME - Delete window object and remove it from the thread windows list */
      /* FIXME - delete allocated DCE */
      
      ClassDereferenceObject(ClassObject);
      DPRINT1("CBT-hook returned !0\n");
      return (HWND) NULL;
    }

  x = Cs.x;
  y = Cs.y;
  nWidth = Cs.cx;
  nHeight = Cs.cy;

  /* default positioning for overlapped windows */
  if(!(WindowObject->Style & (WS_POPUP | WS_CHILD)))
  {
    RECT rc, WorkArea;
    PRTL_USER_PROCESS_PARAMETERS ProcessParams;
    BOOL CalculatedDefPosSize = FALSE;
    
    IntGetDesktopWorkArea(WindowObject->OwnerThread->Win32Thread->Desktop, &WorkArea);
    
    rc = WorkArea;
    ProcessParams = PsGetCurrentProcess()->Peb->ProcessParameters;
    
    if(x == CW_USEDEFAULT || x == CW_USEDEFAULT16)
    {
      CalculatedDefPosSize = IntCalcDefPosSize(ParentWindow, WindowObject, &rc, TRUE);
      
      if(ProcessParams->dwFlags & STARTF_USEPOSITION)
      {
        ProcessParams->dwFlags &= ~STARTF_USEPOSITION;
        Pos.x = WorkArea.left + ProcessParams->dwX;
        Pos.y = WorkArea.top + ProcessParams->dwY;
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
        IntCalcDefPosSize(ParentWindow, WindowObject, &rc, FALSE);
      }
      if(ProcessParams->dwFlags & STARTF_USESIZE)
      {
        ProcessParams->dwFlags &= ~STARTF_USESIZE;
        Size.cx = ProcessParams->dwXSize;
        Size.cy = ProcessParams->dwYSize;
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
  WindowObject->WindowRect.left = Pos.x;
  WindowObject->WindowRect.top = Pos.y;
  WindowObject->WindowRect.right = Pos.x + Size.cx;
  WindowObject->WindowRect.bottom = Pos.y + Size.cy;
  if (0 != (WindowObject->Style & WS_CHILD) && ParentWindow)
    {
      NtGdiOffsetRect(&(WindowObject->WindowRect), ParentWindow->ClientRect.left,
                      ParentWindow->ClientRect.top);
    }
  WindowObject->ClientRect = WindowObject->WindowRect;

  /*
   * Get the size and position of the window.
   */
  if ((dwStyle & WS_THICKFRAME) || !(dwStyle & (WS_POPUP | WS_CHILD)))
    {
      POINT MaxSize, MaxPos, MinTrack, MaxTrack;
     
      /* WinPosGetMinMaxInfo sends the WM_GETMINMAXINFO message */
      WinPosGetMinMaxInfo(WindowObject, &MaxSize, &MaxPos, &MinTrack,
			  &MaxTrack);
      if (MaxSize.x < nWidth) nWidth = MaxSize.x;
      if (MaxSize.y < nHeight) nHeight = MaxSize.y;
      if (nWidth < MinTrack.x ) nWidth = MinTrack.x;
      if (nHeight < MinTrack.y ) nHeight = MinTrack.y;
      if (nWidth < 0) nWidth = 0;
      if (nHeight < 0) nHeight = 0;
    }

  WindowObject->WindowRect.left = Pos.x;
  WindowObject->WindowRect.top = Pos.y;
  WindowObject->WindowRect.right = Pos.x + Size.cx;
  WindowObject->WindowRect.bottom = Pos.y + Size.cy;
  if (0 != (WindowObject->Style & WS_CHILD) && ParentWindow)
    {
      NtGdiOffsetRect(&(WindowObject->WindowRect), ParentWindow->ClientRect.left,
                      ParentWindow->ClientRect.top);
    }
  WindowObject->ClientRect = WindowObject->WindowRect;

  /* FIXME: Initialize the window menu. */

  /* Send a NCCREATE message. */
  Cs.cx = Size.cx;
  Cs.cy = Size.cy;
  Cs.x = Pos.x;
  Cs.y = Pos.y;

  DPRINT("[win32k.window] IntCreateWindowEx style %d, exstyle %d, parent %d\n", Cs.style, Cs.dwExStyle, Cs.hwndParent);
  DPRINT("IntCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);
  DPRINT("IntCreateWindowEx(): About to send NCCREATE message.\n");
  Result = IntSendMessage(WindowObject->Self, WM_NCCREATE, 0, (LPARAM) &Cs);
  if (!Result)
    {
      /* FIXME: Cleanup. */
      if (NULL != ParentWindow)
        {
          IntReleaseWindowObject(ParentWindow);
        }
      DPRINT("IntCreateWindowEx(): NCCREATE message failed.\n");
      return((HWND)0);
    }
 
  /* Calculate the non-client size. */
  MaxPos.x = WindowObject->WindowRect.left;
  MaxPos.y = WindowObject->WindowRect.top;
  DPRINT("IntCreateWindowEx(): About to get non-client size.\n");
  /* WinPosGetNonClientSize SENDS THE WM_NCCALCSIZE message */
  Result = WinPosGetNonClientSize(WindowObject->Self, 
				  &WindowObject->WindowRect,
				  &WindowObject->ClientRect);
  NtGdiOffsetRect(&WindowObject->WindowRect, 
		 MaxPos.x - WindowObject->WindowRect.left,
		 MaxPos.y - WindowObject->WindowRect.top);

  if (NULL != ParentWindow)
    {
      /* link the window into the parent's child list */
      if ((dwStyle & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
        {
          PWINDOW_OBJECT PrevSibling;
          IntLockRelatives(ParentWindow);
          if((PrevSibling = ParentWindow->LastChild))
            IntReferenceWindowObject(PrevSibling);
          IntUnLockRelatives(ParentWindow);
          /* link window as bottom sibling */
          IntLinkWindow(WindowObject, ParentWindow, PrevSibling /*prev sibling*/);
          if(PrevSibling)
            IntReleaseWindowObject(PrevSibling);
        }
      else
        {
          /* link window as top sibling (but after topmost siblings) */
          PWINDOW_OBJECT InsertAfter, Sibling;
          if (0 == (dwExStyle & WS_EX_TOPMOST))
            {
              IntLockRelatives(ParentWindow);
              InsertAfter = NULL;
              Sibling = ParentWindow->FirstChild;
              while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
                {
                  InsertAfter = Sibling;
                  Sibling = Sibling->NextSibling;
                }
              IntUnLockRelatives(ParentWindow);
            }
          else
            {
              InsertAfter = NULL;
            }
          if (NULL != InsertAfter)
            {
              IntReferenceWindowObject(InsertAfter);
            }
          IntLinkWindow(WindowObject, ParentWindow, InsertAfter /* prev sibling */);
          if (NULL != InsertAfter)
            {
              IntReleaseWindowObject(InsertAfter);
            }
        }
    }

  /* Send the WM_CREATE message. */
  DPRINT("IntCreateWindowEx(): about to send CREATE message.\n");
  Result = IntSendMessage(WindowObject->Self, WM_CREATE, 0, (LPARAM) &Cs);
  if (Result == (LRESULT)-1)
    {
      /* FIXME: Cleanup. */
      if (NULL != ParentWindow)
        {
          IntReleaseWindowObject(ParentWindow);
        }
      ClassDereferenceObject(ClassObject);
      DPRINT("IntCreateWindowEx(): send CREATE message failed.\n");
      return((HWND)0);
    } 
  
  /* Send move and size messages. */
  if (!(WindowObject->Flags & WINDOWOBJECT_NEED_SIZE))
    {
      LONG lParam;

      DPRINT("IntCreateWindow(): About to send WM_SIZE\n");

      if ((WindowObject->ClientRect.right - WindowObject->ClientRect.left) < 0 ||
          (WindowObject->ClientRect.bottom - WindowObject->ClientRect.top) < 0)
         DPRINT("Sending bogus WM_SIZE\n");
      
      lParam = MAKE_LONG(WindowObject->ClientRect.right - 
		  WindowObject->ClientRect.left,
		  WindowObject->ClientRect.bottom - 
		  WindowObject->ClientRect.top);
      IntSendMessage(WindowObject->Self, WM_SIZE, SIZE_RESTORED, 
          lParam);

      DPRINT("IntCreateWindow(): About to send WM_MOVE\n");

      if (0 != (WindowObject->Style & WS_CHILD) && ParentWindow)
	{
	  lParam = MAKE_LONG(WindowObject->ClientRect.left - ParentWindow->ClientRect.left,
	      WindowObject->ClientRect.top - ParentWindow->ClientRect.top);
	}
      else
	{
	  lParam = MAKE_LONG(WindowObject->ClientRect.left,
	      WindowObject->ClientRect.top);
	}
      IntSendMessage(WindowObject->Self, WM_MOVE, 0, lParam);
    }

  /* Show or maybe minimize or maximize the window. */
  if (WindowObject->Style & (WS_MINIMIZE | WS_MAXIMIZE))
    {
      RECT NewPos;
      UINT16 SwFlag;

      SwFlag = (WindowObject->Style & WS_MINIMIZE) ? SW_MINIMIZE : 
	SW_MAXIMIZE;
      WinPosMinMaximize(WindowObject, SwFlag, &NewPos);
      SwFlag = 
	((WindowObject->Style & WS_CHILD) || NtUserGetActiveWindow()) ?
	SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
	SWP_NOZORDER | SWP_FRAMECHANGED;
      DPRINT("IntCreateWindow(): About to minimize/maximize\n");
      DPRINT("%d,%d %dx%d\n", NewPos.left, NewPos.top, NewPos.right, NewPos.bottom);
      WinPosSetWindowPos(WindowObject->Self, 0, NewPos.left, NewPos.top,
			 NewPos.right, NewPos.bottom, SwFlag);
    }

  /* Notify the parent window of a new child. */
  if ((WindowObject->Style & WS_CHILD) &&
      (!(WindowObject->ExStyle & WS_EX_NOPARENTNOTIFY)) && ParentWindow)
    {
      DPRINT("IntCreateWindow(): About to notify parent\n");
      IntSendMessage(ParentWindow->Self,
                     WM_PARENTNOTIFY, 
                     MAKEWPARAM(WM_CREATE, WindowObject->IDMenu),
                     (LPARAM)WindowObject->Self);
    }

  if (NULL != ParentWindow)
    {
      IntReleaseWindowObject(ParentWindow);
    }
  
  /* Initialize and show the window's scrollbars */
  if (WindowObject->Style & WS_VSCROLL)
  {
     NtUserShowScrollBar(WindowObject->Self, SB_VERT, TRUE);
  }
  if (WindowObject->Style & WS_HSCROLL)
  {
     NtUserShowScrollBar(WindowObject->Self, SB_HORZ, TRUE);
  }
  
  if (dwStyle & WS_VISIBLE)
    {
      DPRINT("IntCreateWindow(): About to show window\n");
      WinPosShowWindow(WindowObject->Self, dwShowMode);
    }

  DPRINT("IntCreateWindow(): = %X\n", Handle);
  DPRINT("WindowObject->SystemMenu = 0x%x\n", WindowObject->SystemMenu);
  return((HWND)Handle);
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

  DPRINT("NtUserCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);

  /* Get the class name (string or atom) */
  Status = MmCopyFromCaller(&ClassName, UnsafeClassName, sizeof(UNICODE_STRING));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return NULL;
    }
  if (! IS_ATOM(ClassName.Buffer))
    {
      Status = IntSafeCopyUnicodeString(&ClassName, UnsafeClassName);
      if (! NT_SUCCESS(Status))
        {
          SetLastNtError(Status);
          return NULL;
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
          return NULL;
        }
    }
  else
    {
      RtlInitUnicodeString(&WindowName, NULL);
    }

  NewWindow = IntCreateWindowEx(dwExStyle, &ClassName, &WindowName, dwStyle, x, y, nWidth, nHeight,
		                hWndParent, hMenu, hInstance, lpParam, dwShowMode, bUnicodeWindow);

  RtlFreeUnicodeString(&WindowName);
  if (! IS_ATOM(ClassName.Buffer))
    {
      RtlFreeUnicodeString(&ClassName);
    }

  return NewWindow;
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


/*
 * @implemented
 */
BOOLEAN STDCALL
NtUserDestroyWindow(HWND Wnd)
{
  PWINDOW_OBJECT Window;
  BOOLEAN isChild;

  Window = IntGetWindowObject(Wnd);
  if (Window == NULL)
    {
      return FALSE;
    }

  /* Check for owner thread and desktop window */
  if ((Window->OwnerThread != PsGetCurrentThread()) || IntIsDesktopWindow(Window))
    {
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
    }

  /* Look whether the focus is within the tree of windows we will
   * be destroying.
   */
  if (!WinPosShowWindow(Wnd, SW_HIDE))
    {
      if (NtUserGetActiveWindow() == Wnd)
        {
          WinPosActivateOtherWindow(Window);
        }
    }
  IntLockMessageQueue(Window->MessageQueue);
  if (Window->MessageQueue->ActiveWindow == Window->Self)
    Window->MessageQueue->ActiveWindow = NULL;
  if (Window->MessageQueue->FocusWindow == Window->Self)
    Window->MessageQueue->FocusWindow = NULL;
  if (Window->MessageQueue->CaptureWindow == Window->Self)
    Window->MessageQueue->CaptureWindow = NULL;
  IntUnLockMessageQueue(Window->MessageQueue);

  /* Call hooks */
#if 0 /* FIXME */
  if (HOOK_CallHooks(WH_CBT, HCBT_DESTROYWND, (WPARAM) hwnd, 0, TRUE))
    {
    return FALSE;
    }
#endif

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
      HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0L, TRUE );
      /* FIXME: clean up palette - see "Internals" p.352 */
    }
#endif

  if (!IntIsWindow(Wnd))
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
		  IntLockRelatives(Child);
		  if (Child->Owner != Window->Self)
		    {
		      IntUnLockRelatives(Child);
		      IntReleaseWindowObject(Child);
		      continue;
		    }
		  IntUnLockRelatives(Child);
		  if (IntWndBelongsToThread(Child, PsGetWin32Thread()))
		    {
		      IntReleaseWindowObject(Child);
		      NtUserDestroyWindow(*ChildHandle);
		      GotOne = TRUE;		      
		      continue;
		    }
		  IntLockRelatives(Child);
		  if (Child->Owner != NULL)
		    {
		      Child->Owner = NULL;
		    }
		  IntUnLockRelatives(Child);
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
  
  if (!IntIsWindow(Wnd))
    {
      IntReleaseWindowObject(Window);
      return TRUE;
    }

  /* Destroy the window storage */
  IntDestroyWindow(Window, PsGetWin32Process(), PsGetWin32Thread(), TRUE);
  
  IntReleaseWindowObject(Window);
  return TRUE;
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
              PWNDCLASS_OBJECT ClassObject,
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
      while(*phWnd && (*(phWnd++) != ChildAfter->Self));
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
      if(((!CheckWindowName || (CheckWindowName && !RtlCompareUnicodeString(WindowName, &(Child->WindowName), FALSE))) &&
          (!ClassObject || (ClassObject && (Child->Class == ClassObject))))
         ||
         ((!CheckWindowName || (CheckWindowName && !RtlCompareUnicodeString(WindowName, &(Child->WindowName), FALSE))) &&
          (!ClassObject || (ClassObject && (Child->Class == ClassObject)))))
      {
        Ret = Child->Self;
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
 *   hwndParent	    = The window whose childs are to be searched. 
 *					  NULL = desktop
 *					  HWND_MESSAGE = message-only windows
 *
 *   hwndChildAfter = Search starts after this child window. 
 *					  NULL = start from beginning
 *
 *   ucClassName    = Class name to search for
 *					  Reguired parameter.
 *
 *   ucWindowName   = Window name
 *					  ->Buffer == NULL = don't care
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
  PWNDCLASS_OBJECT ClassObject = NULL;
  BOOL ClassFound;
  
  Desktop = IntGetDesktopWindow();
  
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
    return NULL;
  }
  
  ChildAfter = NULL;
  if(hwndChildAfter && !(ChildAfter = IntGetWindowObject(hwndChildAfter)))
  {
    IntReleaseWindowObject(hwndParent);
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
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
    /* this expects the string in ClassName to be NULL-terminated! */
    ClassFound = ClassReferenceClassByNameOrAtom(&ClassObject, ClassName.Buffer, NULL);
    if(!ClassFound)
    {
      if (IS_ATOM(ClassName.Buffer))
        DPRINT1("Window class not found (%lx)\n", (ULONG_PTR)ClassName.Buffer);
      else
        DPRINT1("Window class not found (%S)\n", ClassName.Buffer);
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      goto Cleanup;
    }
  }
  
  if(Parent->Self == Desktop)
  {
    HWND *List, *phWnd;
    PWINDOW_OBJECT TopLevelWindow;
    BOOL CheckWindowName;
    
    /* windows searches through all top-level windows if the parent is the desktop
       window */
    
    if((List = IntWinListChildren(Parent)))
    {
      phWnd = List;
      
      if(ChildAfter)
      {
        /* skip handles before and including ChildAfter */
        while(*phWnd && (*(phWnd++) != ChildAfter->Self));
      }
      
      CheckWindowName = WindowName.Length > 0;
      
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
        if(((!CheckWindowName || (CheckWindowName && !RtlCompareUnicodeString(&WindowName, &(TopLevelWindow->WindowName), FALSE))) &&
            (!ClassObject || (ClassObject && (TopLevelWindow->Class == ClassObject))))
           ||
           ((!CheckWindowName || (CheckWindowName && !RtlCompareUnicodeString(&WindowName, &(TopLevelWindow->WindowName), FALSE))) &&
            (!ClassObject || (ClassObject && (TopLevelWindow->Class == ClassObject)))))
        {
          Ret = TopLevelWindow->Self;
          IntReleaseWindowObject(TopLevelWindow);
          break;
        }
        
        if(IntFindWindow(TopLevelWindow, NULL, ClassObject, &WindowName))
        {
          /* window returns the handle of the top-level window, in case it found
             the child window */
          Ret = TopLevelWindow->Self;
          IntReleaseWindowObject(TopLevelWindow);
          break;
        }
        
        IntReleaseWindowObject(TopLevelWindow);
      }
      ExFreePool(List);
    }
  }
  else
    Ret = IntFindWindow(Parent, ChildAfter, ClassObject, &WindowName);
  
#if 0
  if(Ret == NULL && hwndParent == NULL && hwndChildAfter == NULL)
  {
    /* FIXME - if both hwndParent and hwndChildAfter are NULL, we also should
               search the message-only windows. Should this also be done if
               Parent is the desktop window??? */
    PWINDOW_OBJECT MsgWindows;
    
    if((MsgWindows = IntGetWindowObject(IntGetMessageWindow())))
    {
      Ret = IntFindWindow(MsgWindows, ChildAfter, ClassObject, &WindowName);
      IntReleaseWindowObject(MsgWindows);
    }
  }
#endif
  
  ClassDereferenceObject(ClassObject);
  
  Cleanup:
  if(ClassName.Length > 0 && ClassName.Buffer)
    ExFreePool(ClassName.Buffer);
  
  Cleanup2:
  RtlFreeUnicodeString(&WindowName);
  
  Cleanup3:
  if(ChildAfter)
    IntReleaseWindowObject(ChildAfter);
  IntReleaseWindowObject(Parent);
  
  return Ret;
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
HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Type)
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
   
   hWndAncestor = (WndAncestor ? WndAncestor->Self : NULL);
   IntReleaseWindowObject(Wnd);
   
   if(WndAncestor && (WndAncestor != Wnd))
     IntReleaseWindowObject(WndAncestor);

   return hWndAncestor;
}


/*!
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \param	hWnd	window handle.
 * \param	Rect	pointer to the buffer where the coordinates are returned.
 *
*/
/*
 * @implemented
 */
BOOL STDCALL
NtUserGetClientRect(HWND hWnd, LPRECT Rect)
{
  PWINDOW_OBJECT WindowObject;
  RECT SafeRect;

  if(!(WindowObject = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);      
    return FALSE;
  }

  IntGetClientRect(WindowObject, &SafeRect);
  IntReleaseWindowObject(WindowObject);
  
  if(!NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
  {
    return FALSE;
  }
  return TRUE;
}


/*
 * @implemented
 */
HWND STDCALL
NtUserGetDesktopWindow()
{
   return IntGetDesktopWindow();
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

   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   WndParent = IntGetParent(Wnd);
   if (WndParent)
   {
      hWndParent = WndParent->Self;
      IntReleaseWindowObject(WndParent);
   }

   IntReleaseWindowObject(Wnd);

   return hWndParent;
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
   PWINDOW_OBJECT Wnd = NULL, WndParent = NULL, WndOldParent;
   HWND hWndOldParent = NULL;

   if (IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }
  
   if (hWndChild == IntGetDesktopWindow())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return NULL;
   }

   if (hWndNewParent)
   {
      if (!(WndParent = IntGetWindowObject(hWndNewParent)))
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return NULL;
      }
   }
   else
   {
      if (!(WndParent = IntGetWindowObject(IntGetDesktopWindow())))
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return NULL;
      }
   }

   if (!(Wnd = IntGetWindowObject(hWndChild)))
   {
      IntReleaseWindowObject(WndParent);
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   WndOldParent = IntSetParent(Wnd, WndParent);

   if (WndOldParent)
   {
      hWndOldParent = WndOldParent->Self;
      IntReleaseWindowObject(WndOldParent);
   }

   IntReleaseWindowObject(Wnd);
   IntReleaseWindowObject(WndParent);

   return hWndOldParent;
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
  PWINSTATION_OBJECT WinStaObject;
  HWND Ret;

  NTSTATUS Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return (HWND)0;
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

BOOL STDCALL
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
  PWINSTATION_OBJECT WinStaObject;

  NTSTATUS Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
   /*
    * Test if we are permitted to change the shell window.
    */
   if (WinStaObject->ShellWindow)
   {
      ObDereferenceObject(WinStaObject);
      return FALSE;
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
       WinPosSetWindowPos(hwndListView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif

      if (NtUserGetWindowLong(hwndListView, GWL_EXSTYLE, FALSE) & WS_EX_TOPMOST)
      {
         ObDereferenceObject(WinStaObject);
         return FALSE;
      }
   }

   if (NtUserGetWindowLong(hwndShell, GWL_EXSTYLE, FALSE) & WS_EX_TOPMOST)
   {
      ObDereferenceObject(WinStaObject);
      return FALSE;
   }

   WinPosSetWindowPos(hwndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

   WinStaObject->ShellWindow = hwndShell;
   WinStaObject->ShellListView = hwndListView;
   
   ObDereferenceObject(WinStaObject);
   return TRUE;
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
   HMENU Result = 0;
   PWINDOW_OBJECT WindowObject;
   PMENU_OBJECT MenuObject;

   WindowObject = IntGetWindowObject((HWND)hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }
  
   MenuObject = IntGetSystemMenu(WindowObject, bRevert, FALSE);
   if (MenuObject)
   {
      Result = MenuObject->MenuInfo.Self;
      IntReleaseMenuObject(MenuObject);
   }
  
   IntReleaseWindowObject(WindowObject);
   return Result;
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
   PWINDOW_OBJECT WindowObject;
   PMENU_OBJECT MenuObject;

   WindowObject = IntGetWindowObject(hWnd);
   if (!WindowObject)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }
  
   if (hMenu)
   {
      /*
       * Assign new menu handle.
       */
      MenuObject = IntGetMenuObject(hMenu);
      if (!MenuObject)
      {
         IntReleaseWindowObject(WindowObject);
         SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }
    
      Result = IntSetSystemMenu(WindowObject, MenuObject);
    
      IntReleaseMenuObject(MenuObject);
   }
  
   IntReleaseWindowObject(WindowObject);
 
   return Result;
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
   PWINDOW_OBJECT WindowObject, Parent;
   HWND hWndResult = NULL;

   if (!(WindowObject = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }
  
   switch (Relationship)
   {
      case GW_HWNDFIRST:
         if((Parent = IntGetParentObject(WindowObject)))
         {
           IntLockRelatives(Parent);
           if (Parent->FirstChild)
              hWndResult = Parent->FirstChild->Self;
           IntUnLockRelatives(Parent);
           IntReleaseWindowObject(Parent);
         }
         break;

      case GW_HWNDLAST:
         if((Parent = IntGetParentObject(WindowObject)))
         {
           IntLockRelatives(Parent);
           if (Parent->LastChild)
              hWndResult = Parent->LastChild->Self;
           IntUnLockRelatives(Parent);
           IntReleaseWindowObject(Parent);
         }
         break;

      case GW_HWNDNEXT:
         IntLockRelatives(WindowObject);
         if (WindowObject->NextSibling)
            hWndResult = WindowObject->NextSibling->Self;
         IntUnLockRelatives(WindowObject);
         break;

      case GW_HWNDPREV:
         IntLockRelatives(WindowObject);
         if (WindowObject->PrevSibling)
            hWndResult = WindowObject->PrevSibling->Self;
         IntUnLockRelatives(WindowObject);
         break;

      case GW_OWNER:
         IntLockRelatives(WindowObject);
         if((Parent = IntGetWindowObject(WindowObject->Owner)))
         {
           hWndResult = Parent->Self;
           IntReleaseWindowObject(Parent);
         }
         IntUnLockRelatives(WindowObject);
         break;
      case GW_CHILD:
         IntLockRelatives(WindowObject);
         if (WindowObject->FirstChild)
            hWndResult = WindowObject->FirstChild->Self;
         IntUnLockRelatives(WindowObject);
         break;
   }

   IntReleaseWindowObject(WindowObject);

   return hWndResult;
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
   PWINDOW_OBJECT WindowObject, Parent;
   LONG Result;

   DPRINT("NtUserGetWindowLong(%x,%d,%d)\n", hWnd, (INT)Index, Ansi);

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }

   /*
    * Only allow CSRSS to mess with the desktop window
    */
   if (hWnd == IntGetDesktopWindow()
       && WindowObject->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
   {
      SetLastWin32Error(STATUS_ACCESS_DENIED);
      return 0;
   }

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > WindowObject->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         return 0;
      }
      Result = *((LONG *)(WindowObject->ExtraData + Index));
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            Result = WindowObject->ExStyle;
            break;

         case GWL_STYLE:
            Result = WindowObject->Style;
            break;

         case GWL_WNDPROC:
            if (Ansi)
               Result = (LONG) WindowObject->WndProcA;
            else
               Result = (LONG) WindowObject->WndProcW;
            break;

         case GWL_HINSTANCE:
            Result = (LONG) WindowObject->Instance;
            break;

         case GWL_HWNDPARENT:
            IntLockRelatives(WindowObject);
            Parent = IntGetWindowObject(WindowObject->Parent);
            IntUnLockRelatives(WindowObject);
            if(Parent)
            {
              if (Parent && Parent->Self == IntGetDesktopWindow())
                 Result = (LONG) NtUserGetWindow(WindowObject->Self, GW_OWNER);
              else
                 Result = (LONG) Parent->Self;
              IntReleaseWindowObject(Parent);
            }
            break;

         case GWL_ID:
            Result = (LONG) WindowObject->IDMenu;
            break;

         case GWL_USERDATA:
            Result = WindowObject->UserData;
            break;
    
         default:
            DPRINT1("NtUserGetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            Result = 0;
            break;
      }
   }

   IntReleaseWindowObject(WindowObject);

   return Result;
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
   PWINDOW_OBJECT WindowObject, Parent;
   PW32PROCESS Process;
   PWINSTATION_OBJECT WindowStation;
   LONG OldValue;
   STYLESTRUCT Style;

   if (hWnd == IntGetDesktopWindow())
   {
      SetLastWin32Error(STATUS_ACCESS_DENIED);
      return 0;
   }

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }

   if ((INT)Index >= 0)
   {
      if ((Index + sizeof(LONG)) > WindowObject->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         IntReleaseWindowObject(WindowObject);
         return 0;
      }
      OldValue = *((LONG *)(WindowObject->ExtraData + Index));
      *((LONG *)(WindowObject->ExtraData + Index)) = NewValue;
   }
   else
   {
      switch (Index)
      {
         case GWL_EXSTYLE:
            OldValue = (LONG) WindowObject->ExStyle;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;

            /*
             * Remove extended window style bit WS_EX_TOPMOST for shell windows.
             */
            Process = WindowObject->OwnerThread->ThreadsProcess->Win32Process;
            WindowStation = Process->WindowStation;
            if(WindowStation)
            {
              if (hWnd == WindowStation->ShellWindow || hWnd == WindowStation->ShellListView)
                 Style.styleNew &= ~WS_EX_TOPMOST;
            }

            IntSendMessage(hWnd, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM) &Style);
            WindowObject->ExStyle = (DWORD)Style.styleNew;
            IntSendMessage(hWnd, WM_STYLECHANGED, GWL_EXSTYLE, (LPARAM) &Style);
            break;

         case GWL_STYLE:
            OldValue = (LONG) WindowObject->Style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            IntSendMessage(hWnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);
            WindowObject->Style = (DWORD)Style.styleNew;
            IntSendMessage(hWnd, WM_STYLECHANGED, GWL_STYLE, (LPARAM) &Style);
            break;

         case GWL_WNDPROC:
            /* FIXME: should check if window belongs to current process */
            if (Ansi)
            {
               OldValue = (LONG) WindowObject->WndProcA;
               WindowObject->WndProcA = (WNDPROC) NewValue;
               WindowObject->WndProcW = (WNDPROC) IntAddWndProcHandle((WNDPROC)NewValue,FALSE);
               WindowObject->Unicode = FALSE;
            }
            else
            {
               OldValue = (LONG) WindowObject->WndProcW;
               WindowObject->WndProcW = (WNDPROC) NewValue;
               WindowObject->WndProcA = (WNDPROC) IntAddWndProcHandle((WNDPROC)NewValue,TRUE);
               WindowObject->Unicode = TRUE;
            }
            break;

         case GWL_HINSTANCE:
            OldValue = (LONG) WindowObject->Instance;
            WindowObject->Instance = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            Parent = IntGetParentObject(WindowObject);
            if (Parent && (Parent->Self == IntGetDesktopWindow()))
               OldValue = (LONG) IntSetOwner(WindowObject->Self, (HWND) NewValue);
            else
               OldValue = (LONG) NtUserSetParent(WindowObject->Self, (HWND) NewValue);
            if(Parent)
              IntReleaseWindowObject(Parent);
            break;

         case GWL_ID:
            OldValue = (LONG) WindowObject->IDMenu;
            WindowObject->IDMenu = (UINT) NewValue;
            break;

         case GWL_USERDATA:
            OldValue = WindowObject->UserData;
            WindowObject->UserData = NewValue;
            break;
    
         default:
            DPRINT1("NtUserSetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            OldValue = 0;
            break;
      }
   }

   IntReleaseWindowObject(WindowObject);
 
   return OldValue;
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
   PWINDOW_OBJECT WindowObject;
   WORD OldValue;

   switch (Index)
   {
      case GWL_ID:
      case GWL_HINSTANCE:
      case GWL_HWNDPARENT:
         return NtUserSetWindowLong(hWnd, Index, (UINT)NewValue, TRUE);
      default:
         if (Index < 0)
         {
            SetLastWin32Error(ERROR_INVALID_INDEX);
            return 0;
         }
   }

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }

   if (Index > WindowObject->ExtraDataSize - sizeof(WORD))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      IntReleaseWindowObject(WindowObject);
      return 0;
   }

   OldValue = *((WORD *)(WindowObject->ExtraData + Index));
   *((WORD *)(WindowObject->ExtraData + Index)) = NewValue;

   IntReleaseWindowObject(WindowObject);

   return OldValue;
}

/*
 * @implemented
 */
BOOL STDCALL
NtUserGetWindowPlacement(HWND hWnd,
			 WINDOWPLACEMENT *lpwndpl)
{
  PWINDOW_OBJECT WindowObject;
  PINTERNALPOS InternalPos;
  POINT Size;
  WINDOWPLACEMENT Safepl;
  NTSTATUS Status;
  
  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    IntReleaseWindowObject(WindowObject);
    return FALSE;
  }
  if(Safepl.length != sizeof(WINDOWPLACEMENT))
  {
    IntReleaseWindowObject(WindowObject);
    return FALSE;
  }
  
  Safepl.flags = 0;
  Safepl.showCmd = ((WindowObject->Flags & WINDOWOBJECT_RESTOREMAX) ? SW_MAXIMIZE : SW_SHOWNORMAL);
  
  Size.x = WindowObject->WindowRect.left;
  Size.y = WindowObject->WindowRect.top;
  InternalPos = WinPosInitInternalPos(WindowObject, &Size, 
				      &WindowObject->WindowRect);
  if (InternalPos)
  {
    Safepl.rcNormalPosition = InternalPos->NormalRect;
    Safepl.ptMinPosition = InternalPos->IconPos;
    Safepl.ptMaxPosition = InternalPos->MaxPos;
  }
  else
  {
    IntReleaseWindowObject(WindowObject);
    return FALSE;
  }
  
  Status = MmCopyToCaller(lpwndpl, &Safepl, sizeof(WINDOWPLACEMENT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    IntReleaseWindowObject(WindowObject);
    return FALSE;
  }
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}


/*!
 * Return the dimension of the window in the screen coordinates.
 * \param	hWnd	window handle.
 * \param	Rect	pointer to the buffer where the coordinates are returned.
*/
/*
 * @implemented
 */
BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect)
{
  PWINDOW_OBJECT Wnd;
  NTSTATUS Status;

  if (!(Wnd = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);      
    return FALSE;
  }
  Status = MmCopyToCaller(Rect, &Wnd->WindowRect, sizeof(RECT));
  if (!NT_SUCCESS(Status))
  {
    IntReleaseWindowObject(Wnd);
    SetLastNtError(Status);
    return FALSE;
  }
  
  IntReleaseWindowObject(Wnd);
  return TRUE;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserGetWindowThreadProcessId(HWND hWnd, LPDWORD UnsafePid)
{
   PWINDOW_OBJECT Wnd;
   DWORD tid, pid;
   
   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }

   tid = (DWORD)IntGetWndThreadId(Wnd);
   pid = (DWORD)IntGetWndProcessId(Wnd);
   
   if (UnsafePid) MmCopyToCaller(UnsafePid, &pid, sizeof(DWORD));
   
   return tid;
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
   PWINDOW_OBJECT Window = IntGetWindowObject(hWnd);
   DWORD Result;

   if (Window == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
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

   IntReleaseWindowObject(Window);

   return Result;
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
  
  if(MessageNameUnsafe == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  Status = IntSafeCopyUnicodeStringTerminateNULL(&SafeMessageName, MessageNameUnsafe);
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }
  
  Ret = (UINT)IntAddAtom(SafeMessageName.Buffer);
  
  RtlFreeUnicodeString(&SafeMessageName);
  return Ret;
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
   HWND Wnd,
   HMENU Menu,
   BOOL Repaint)
{
  PWINDOW_OBJECT WindowObject;
  BOOL Changed;

  WindowObject = IntGetWindowObject((HWND) Wnd);
  if (NULL == WindowObject)
    {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
    }
  
  if (! IntSetMenu(WindowObject, Menu, &Changed))
    {
      IntReleaseWindowObject(WindowObject);
      return FALSE;
    }
  
  IntReleaseWindowObject(WindowObject);
  
  if (Changed && Repaint)
    {
      WinPosSetWindowPos(Wnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                         SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
  
  return TRUE;
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
  PWINDOW_OBJECT WindowObject;
  WINDOWPLACEMENT Safepl;
  NTSTATUS Status;

  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  Status = MmCopyFromCaller(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    IntReleaseWindowObject(WindowObject);
    return FALSE;
  }
  if(Safepl.length != sizeof(WINDOWPLACEMENT))
  {
    IntReleaseWindowObject(WindowObject);
    return FALSE;
  }
  
  if ((WindowObject->Style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
  {
     WinPosSetWindowPos(WindowObject->Self, NULL,
        Safepl.rcNormalPosition.left, Safepl.rcNormalPosition.top,
        Safepl.rcNormalPosition.right - Safepl.rcNormalPosition.left,
        Safepl.rcNormalPosition.bottom - Safepl.rcNormalPosition.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
  }
  
  /* FIXME - change window status */
  WinPosShowWindow(WindowObject->Self, Safepl.showCmd);

  if (WindowObject->InternalPos == NULL)
     WindowObject->InternalPos = ExAllocatePoolWithTag(PagedPool, sizeof(INTERNALPOS), TAG_WININTLIST);
  WindowObject->InternalPos->NormalRect = Safepl.rcNormalPosition;
  WindowObject->InternalPos->IconPos = Safepl.ptMinPosition;
  WindowObject->InternalPos->MaxPos = Safepl.ptMaxPosition;

  IntReleaseWindowObject(WindowObject);
  return TRUE;
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
   return WinPosSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}


INT FASTCALL
IntGetWindowRgn(HWND hWnd, HRGN hRgn)
{
  INT Ret;
  PWINDOW_OBJECT WindowObject;
  HRGN VisRgn;
  ROSRGNDATA *pRgn;
  
  if(!(WindowObject = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return ERROR;
  }
  if(!hRgn)
  {
    IntReleaseWindowObject(WindowObject);
    return ERROR;
  }
  
  /* Create a new window region using the window rectangle */
  VisRgn = UnsafeIntCreateRectRgnIndirect(&WindowObject->WindowRect);
  NtGdiOffsetRgn(VisRgn, -WindowObject->WindowRect.left, -WindowObject->WindowRect.top);
  /* if there's a region assigned to the window, combine them both */
  if(WindowObject->WindowRegion && !(WindowObject->Style & WS_MINIMIZE))
    NtGdiCombineRgn(VisRgn, VisRgn, WindowObject->WindowRegion, RGN_AND);
  /* Copy the region into hRgn */
  NtGdiCombineRgn(hRgn, VisRgn, NULL, RGN_COPY);
  
  if((pRgn = RGNDATA_LockRgn(hRgn)))
  {
    Ret = pRgn->rdh.iType;
    RGNDATA_UnlockRgn(hRgn);
  }
  else
    Ret = ERROR;
  
  NtGdiDeleteObject(VisRgn);
  
  IntReleaseWindowObject(WindowObject);
  return Ret;
}

INT FASTCALL
IntGetWindowRgnBox(HWND hWnd, RECT *Rect)
{
  INT Ret;
  PWINDOW_OBJECT WindowObject;
  HRGN VisRgn;
  ROSRGNDATA *pRgn;
  
  if(!(WindowObject = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return ERROR;
  }
  if(!Rect)
  {
    IntReleaseWindowObject(WindowObject);
    return ERROR;
  }
  
  /* Create a new window region using the window rectangle */
  VisRgn = UnsafeIntCreateRectRgnIndirect(&WindowObject->WindowRect);
  NtGdiOffsetRgn(VisRgn, -WindowObject->WindowRect.left, -WindowObject->WindowRect.top);
  /* if there's a region assigned to the window, combine them both */
  if(WindowObject->WindowRegion && !(WindowObject->Style & WS_MINIMIZE))
    NtGdiCombineRgn(VisRgn, VisRgn, WindowObject->WindowRegion, RGN_AND);
  
  if((pRgn = RGNDATA_LockRgn(VisRgn)))
  {
    Ret = pRgn->rdh.iType;
    *Rect = pRgn->rdh.rcBound;
    RGNDATA_UnlockRgn(VisRgn);
  }
  else
    Ret = ERROR;
  
  NtGdiDeleteObject(VisRgn);
  
  IntReleaseWindowObject(WindowObject);
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
  PWINDOW_OBJECT WindowObject;
  
  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  
  /* FIXME - Verify if hRgn is a valid handle!!!!
             Propably make this operation thread-safe, but maybe it's not necessary */
  
  if(WindowObject->WindowRegion)
  {
    /* Delete no longer needed region handle */
    NtGdiDeleteObject(WindowObject->WindowRegion);
  }
  WindowObject->WindowRegion = hRgn;
  
  /* FIXME - send WM_WINDOWPOSCHANGING and WM_WINDOWPOSCHANGED messages to the window */
  
  if(bRedraw)
  {
    IntRedrawWindow(WindowObject, NULL, NULL, RDW_INVALIDATE);
  }
  
  IntReleaseWindowObject(WindowObject);
  return (INT)hRgn;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserShowWindow(HWND hWnd,
		 LONG nCmdShow)
{
   return WinPosShowWindow(hWnd, nCmdShow);
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

   if ((DesktopWindow = IntGetWindowObject(IntGetDesktopWindow())))
   {
      USHORT Hit;
      
      pt.x = X;
      pt.y = Y;
      
      Hit = WinPosWindowFromPoint(DesktopWindow, PsGetWin32Thread()->MessageQueue, &pt, &Window);
      
      if(Window)
      {
        Ret = Window->Self;
        IntReleaseWindowObject(Window);
        IntReleaseWindowObject(DesktopWindow);
        return Ret;
      }
      
      IntReleaseWindowObject(DesktopWindow);
   }
  
   return NULL;
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
NtUserDefSetText(HWND WindowHandle, PUNICODE_STRING WindowText)
{
  PWINDOW_OBJECT WindowObject;
  UNICODE_STRING SafeText;
  NTSTATUS Status;
  
  WindowObject = IntGetWindowObject(WindowHandle);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  if(WindowText)
  {
    Status = IntSafeCopyUnicodeString(&SafeText, WindowText);
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      IntReleaseWindowObject(WindowObject);
      return FALSE;
    }
  }
  else
  {
    RtlInitUnicodeString(&SafeText, NULL);
  }
  
  /* FIXME - do this thread-safe! otherwise one could crash here! */
  RtlFreeUnicodeString(&WindowObject->WindowName);
  
  WindowObject->WindowName = SafeText;
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
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
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  INT Result;
  
  if(lpString && (nMaxCount <= 1))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  
  /* FIXME - do this thread-safe! otherwise one could crash here! */
  Result = WindowObject->WindowName.Length / sizeof(WCHAR);
  if(lpString)
  {
    const WCHAR Terminator = L'\0';
    INT Copy;
    WCHAR *Buffer = (WCHAR*)lpString;
    
    Copy = min(nMaxCount - 1, Result);
    if(Copy > 0)
    {
      Status = MmCopyToCaller(Buffer, WindowObject->WindowName.Buffer, Copy * sizeof(WCHAR));
      if(!NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        IntReleaseWindowObject(WindowObject);
        return 0;
      }
      Buffer += Copy;
    }
    
    Status = MmCopyToCaller(Buffer, &Terminator, sizeof(WCHAR));
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      IntReleaseWindowObject(WindowObject);
      return 0;
    }
    
    Result = Copy;
  }
  
  IntReleaseWindowObject(WindowObject);
  return Result;
}

DWORD STDCALL
NtUserDereferenceWndProcHandle(WNDPROC wpHandle, WndProcHandle *Data)
{
	WndProcHandle Entry;
	if (((DWORD)wpHandle & 0xFFFF0000) == 0xFFFF0000)
	{
		Entry = WndProcHandlesArray[(DWORD)wpHandle & 0x0000FFFF];
		Data->WindowProc = Entry.WindowProc;
		Data->IsUnicode = Entry.IsUnicode;
		Data->ProcessID = Entry.ProcessID;
		return TRUE;
	} else {
		return FALSE;
	}
	return FALSE;
}

DWORD
IntAddWndProcHandle(WNDPROC WindowProc, BOOL IsUnicode)
{
	WORD i;
	WORD FreeSpot;
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

/* EOF */
