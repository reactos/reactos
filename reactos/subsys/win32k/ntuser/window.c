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
/* $Id: window.c,v 1.128 2003/11/02 14:08:34 navaraf Exp $
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

#include <ddk/ntddk.h>
#include <internal/safe.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/winpos.h>
#include <include/callback.h>
#include <include/msgqueue.h>
#include <include/rect.h>
#include <include/dce.h>
#include <include/paint.h>
#include <include/painting.h>
#include <include/scroll.h>
#include <include/vis.h>
#include <include/menu.h>

#define NDEBUG
#include <win32k/debug1.h>
#include <debug.h>

#define TAG_WNAM  TAG('W', 'N', 'A', 'M')

typedef struct _REGISTERED_MESSAGE
{
   LIST_ENTRY ListEntry;
   WCHAR MessageName[1];
} REGISTERED_MESSAGE, *PREGISTERED_MESSAGE;

static LIST_ENTRY RegisteredMessageListHead;

#define REGISTERED_MESSAGE_MIN 0xc000
#define REGISTERED_MESSAGE_MAX 0xffff

/* globally stored handles to the shell windows */
HWND hwndShellWindow = 0;
HWND hwndShellListView = 0;

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * InitWindowImpl
 *
 * Initialize windowing implementation.
 */

NTSTATUS FASTCALL
InitWindowImpl(VOID)
{
   InitializeListHead(&RegisteredMessageListHead);
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

/*
 * IntIsBroadcastHwnd
 *
 * Check if window is a broadcast.
 *
 * Return Value
 *    TRUE if window is broadcast, FALSE otherwise.
 */

inline BOOL
IntIsBroadcastHwnd(HWND hWnd)
{
   return (hWnd == HWND_BROADCAST || hWnd == HWND_TOPMOST);
}

/*
 * IntIsDesktopWindow
 *
 * Check if window is a desktop. Desktop windows has no parent.
 *
 * Return Value
 *    TRUE if window is desktop, FALSE otherwise.
 */

inline BOOL
IntIsDesktopWindow(PWINDOW_OBJECT Wnd)
{
   return Wnd->Parent == NULL;
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
   else
      IntReleaseWindowObject(Window);

   return TRUE;
}

/*
 * IntIsMenu
 *
 * The function determines whether the specified menu handle identifies
 * an existing menu.
 *
 * Parameters
 *    hMenu
 *       Handle to the menu to test.
 *
 * Return Value
 *    If the menu handle identifies an existing menu, the return value
 *    is TRUE. If the menu handle does not identify an existing menu,
 *    the return value is FALSE. 
 *
 * FIXME: Move this routine to menu.c.
 */

BOOL FASTCALL
IntIsMenu(HMENU hMenu)
{

   PMENU_OBJECT Menu;

   if (!(Menu = IntGetMenuObject(hMenu)))
      return FALSE;
   else
      IntReleaseMenuObject(Menu);

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

/*
 * IntGetWindowObject
 *
 * Get window object from handle of current process.
 */

PWINDOW_OBJECT FASTCALL
IntGetWindowObject(HWND hWnd)
{
   return IntGetProcessWindowObject(PsGetWin32Process(), hWnd);
}

/*
 * IntReleaseWindowObject
 *
 * Release window object returned by IntGetWindowObject or
 * IntGetProcessWindowObject.
 */

VOID FASTCALL
IntReleaseWindowObject(PWINDOW_OBJECT Window)
{
   ObmDereferenceObject(Window);
}

/*
 * IntBuildChildWindowArray
 *
 * Compile a list of all child window handles from given window.
 *
 * Remarks
 *    This function is similar to Wine WIN_ListChildren, but has different
 *    syntax.
 */

BOOL FASTCALL
IntBuildChildWindowArray(PWINDOW_OBJECT Window, HWND **Children, unsigned *NumChildren)
{
   PWINDOW_OBJECT Child;
   UINT Index;

   *Children = NULL;
   *NumChildren = 0;

   ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);

   Child = Window->FirstChild;
   while (Child)
   {
      (*NumChildren)++;
      Child = Child->NextSibling;
   }
  
   if (*NumChildren != 0)
   {
      *Children = ExAllocatePoolWithTag(PagedPool, *NumChildren * sizeof(HWND), TAG_WNAM);
      if (*Children != NULL)
      {
         for (Child = Window->FirstChild, Index = 0;
              Child != NULL;
              Child = Child->NextSibling, ++Index)
         {
            (*Children)[Index] = Child->Self;
         }
         ASSERT(Index == *NumChildren);
      }
      else
      {
         DPRINT1("Failed to allocate memory for children array\n");
      }
   }

   ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);

   return ((*NumChildren != 0) && (*Children != NULL));
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
  unsigned NumChildren;
  unsigned Index;
  PWINDOW_OBJECT Child;

  if (! IntWndBelongsToThread(Window, ThreadData))
    {
      DPRINT1("Window doesn't belong to current thread\n");
      return 0;
    }

  /* free child windows */
  if (IntBuildChildWindowArray(Window, &Children, &NumChildren))
    {
      DbgPrint("NumChildren: %d\n", NumChildren);
      for (Index = NumChildren; 0 < Index; Index--)
        {
          Child = IntGetProcessWindowObject(ProcessData, Children[Index - 1]);
          DbgPrint("Child %d: %x\n", Index - 1, Child);
          if (NULL != Child)
	{
	  if (IntWndBelongsToThread(Child, ThreadData))
	    {
	      DbgPrint("Destroying\n");
	      IntDestroyWindow(Child, ProcessData, ThreadData, SendMessages);
	      DbgPrint("End Destroying\n");
	    }
#if 0 /* FIXME */
	  else
	    {
	      SendMessageW( list[i], WM_WINE_DESTROYWINDOW, 0, 0 );
	    }
#endif
	}
        }
      if (0 != NumChildren)
        {
          ExFreePool(Children);
        }
    }

  if (SendMessages)
    {
      /*
       * Clear the update region to make sure no WM_PAINT messages will be
       * generated for this window while processing the WM_NCDESTROY.
       */
      PaintRedrawWindow(Window, NULL, 0,
                        RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_NOCHILDREN,
                        0);

      /*
       * Send the WM_NCDESTROY to the window being destroyed.
       */
      NtUserSendMessage(Window->Self, WM_NCDESTROY, 0, 0);
    }

  /* reset shell window handles */
  if (Window->Self == hwndShellWindow)
    hwndShellWindow = 0;

  if (Window->Self == hwndShellListView)
    hwndShellListView = 0;

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

#if 0 /* FIXME */
  if (0 == (Window->Style & WS_CHILD))
    {
      HMENU Menu = (HMENU) NtUserSetWindowLongW(Window->Self, GWL_ID, 0);
      if (NULL != Menu)
	{
	  DestroyMenu(Menu);
	}
    }
  if (Window->hSysMenu)
    {
      DestroyMenu(Window->hSysMenu);
      Window->hSysMenu = 0;
    }
#endif
  DceFreeWindowDCE(Window);    /* Always do this to catch orphaned DCs */
#if 0 /* FIXME */
  WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
  CLASS_RemoveWindow(Window->Class);
#endif

  ExAcquireFastMutexUnsafe(&Window->Parent->ChildrenListLock);
  IntUnlinkWindow(Window);
  ExReleaseFastMutexUnsafe(&Window->Parent->ChildrenListLock);

  ExAcquireFastMutexUnsafe (&ThreadData->WindowListLock);
  RemoveEntryList(&Window->ThreadListEntry);
  ExReleaseFastMutexUnsafe (&ThreadData->WindowListLock);
  
  IntDestroyScrollBar(Window, SB_VERT);
  IntDestroyScrollBar(Window, SB_HORZ);

  Window->Class = NULL;
  ObmCloseHandle(ProcessData->WindowStation->HandleTable, Window->Self);

  IntGraphicsCheck(FALSE);

  return 0;
}


/* INTERNAL ******************************************************************/


VOID FASTCALL
DestroyThreadWindows(struct _ETHREAD *Thread)
{
  PLIST_ENTRY LastHead;
  PW32PROCESS Win32Process;
  PW32THREAD Win32Thread;
  PWINDOW_OBJECT Window;

  Win32Thread = Thread->Win32Thread;
  Win32Process = Thread->ThreadsProcess->Win32Process;
  ExAcquireFastMutexUnsafe(&Win32Thread->WindowListLock);
  LastHead = NULL;
  while (Win32Thread->WindowListHead.Flink != &(Win32Thread->WindowListHead) &&
         Win32Thread->WindowListHead.Flink != LastHead)
    {
      LastHead = Win32Thread->WindowListHead.Flink;
      Window = CONTAINING_RECORD(Win32Thread->WindowListHead.Flink, WINDOW_OBJECT, ThreadListEntry);
      ExReleaseFastMutexUnsafe(&Win32Thread->WindowListLock);
      IntDestroyWindow(Window, Win32Process, Win32Thread, FALSE);
      ExAcquireFastMutexUnsafe(&Win32Thread->WindowListLock);
    }
  if (Win32Thread->WindowListHead.Flink == LastHead)
    {
      /* Window at head of list was not removed, should never happen, infinite loop */
      KEBUGCHECK(0);
    }
  ExReleaseFastMutexUnsafe(&Win32Thread->WindowListLock);
}


HWND STDCALL
IntCreateDesktopWindow(PWINSTATION_OBJECT WindowStation,
			PWNDCLASS_OBJECT DesktopClass,
			ULONG Width, ULONG Height)
{
  PWSTR WindowName;
  HWND Handle;
  PWINDOW_OBJECT WindowObject;

  /* Create the window object. */
  WindowObject = (PWINDOW_OBJECT)ObmCreateObject(WindowStation->HandleTable, 
						 &Handle, 
						 otWindow,
						 sizeof(WINDOW_OBJECT));
  if (!WindowObject) 
    {
      return((HWND)0);
    }

  /*
   * Fill out the structure describing it.
   */
  WindowObject->Class = DesktopClass;
  WindowObject->ExStyle = 0;
  WindowObject->Style = WS_VISIBLE;
  WindowObject->Flags |= WINDOWOBJECT_NEED_ERASEBACKGRD;
  WindowObject->x = 0;
  WindowObject->y = 0;
  WindowObject->Width = Width;
  WindowObject->Height = Height;
  WindowObject->Parent = NULL;
  WindowObject->Owner = NULL;
  WindowObject->IDMenu = 0;
  WindowObject->Instance = NULL;
  WindowObject->Parameters = NULL;
  WindowObject->Self = Handle;
  WindowObject->MessageQueue = NULL;
  WindowObject->ExtraData = NULL;
  WindowObject->ExtraDataSize = 0;
  WindowObject->WindowRect.left = 0;
  WindowObject->WindowRect.top = 0;
  WindowObject->WindowRect.right = Width;
  WindowObject->WindowRect.bottom = Height;
  WindowObject->ClientRect = WindowObject->WindowRect;
  WindowObject->UserData = 0;
  /*FIXME: figure out what the correct strange value is and what to do with it (and how to set the wndproc values correctly) */
  WindowObject->WndProcA = DesktopClass->lpfnWndProcA;
  WindowObject->WndProcW = DesktopClass->lpfnWndProcW;
  WindowObject->OwnerThread = PsGetCurrentThread();
  WindowObject->FirstChild = NULL;
  WindowObject->LastChild = NULL;
  WindowObject->PrevSibling = NULL;
  WindowObject->NextSibling = NULL;

  ExInitializeFastMutex(&WindowObject->ChildrenListLock);

  WindowName = ExAllocatePool(NonPagedPool, sizeof(L"DESKTOP"));
  wcscpy(WindowName, L"DESKTOP");
  RtlInitUnicodeString(&WindowObject->WindowName, WindowName);

  return(Handle);
}


HWND FASTCALL
IntGetActiveWindow(VOID)
{
  PUSER_MESSAGE_QUEUE Queue;
  Queue = (PUSER_MESSAGE_QUEUE)IntGetActiveDesktop()->ActiveMessageQueue;
  if (Queue == NULL)
    {
      return(NULL);
    }
  else
    {
      return(Queue->ActiveWindow);
    }
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
  Rect->bottom = 
    WindowObject->ClientRect.bottom - WindowObject->ClientRect.top;
}


HWND FASTCALL IntGetDesktopWindow(VOID)
{
  return IntGetActiveDesktop()->DesktopWindow;
}


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

PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd)
{
  if (Wnd->Style & WS_POPUP)
  {
    return Wnd->Owner;
  }
  else if (Wnd->Style & WS_CHILD) 
  {
    return Wnd->Parent;
  }

  return NULL;
}



PMENU_OBJECT FASTCALL
IntGetSystemMenu(PWINDOW_OBJECT WindowObject, BOOL bRevert, BOOL RetMenu)
{
  PMENU_OBJECT MenuObject, NewMenuObject, ret = NULL;
  PW32PROCESS W32Process;
  HMENU NewMenu;

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
        WindowObject->SystemMenu = NewMenuObject->Self;
        NewMenuObject->IsSystemMenu = TRUE;
        ret = NewMenuObject;
        //IntReleaseMenuObject(NewMenuObject);
      }
      IntReleaseMenuObject(MenuObject);
    }
    else
    {
      NewMenu = IntLoadSysMenuTemplate();
      if(!NewMenu)
        return NULL;
      MenuObject = IntGetMenuObject(NewMenu);
      if(!MenuObject)
        return NULL;
      
      NewMenuObject = IntCloneMenu(MenuObject);
      if(NewMenuObject)
      {
        WindowObject->SystemMenu = NewMenuObject->Self;
        NewMenuObject->IsSystemMenu = TRUE;
        ret = NewMenuObject;
        //IntReleaseMenuObject(NewMenuObject);
      }
      IntDestroyMenuObject(MenuObject, FALSE, TRUE);
    }
    if(RetMenu)
      return ret;
    else
      return NULL;
  }
  else
  {
    return IntGetMenuObject((HMENU)WindowObject->SystemMenu);
  }
}



DWORD FASTCALL
IntGetWindowThreadProcessId(PWINDOW_OBJECT Wnd, PDWORD pid)
{
   if (pid) *pid = (DWORD) Wnd->OwnerThread->ThreadsProcess->UniqueProcessId;
   return (DWORD) Wnd->OwnerThread->Cid.UniqueThread;
}


VOID FASTCALL
IntInitDesktopWindow(ULONG Width, ULONG Height)
{
  PWINDOW_OBJECT DesktopWindow;
  HRGN DesktopRgn;
  
  DesktopWindow = IntGetWindowObject(PsGetWin32Thread()->Desktop->DesktopWindow);
  if (NULL == DesktopWindow)
    {
      return;
    }
  DesktopWindow->WindowRect.right = Width;
  DesktopWindow->WindowRect.bottom = Height;
  DesktopWindow->ClientRect = DesktopWindow->WindowRect;

  DesktopRgn = UnsafeIntCreateRectRgnIndirect(&(DesktopWindow->WindowRect));
  VIS_WindowLayoutChanged(PsGetWin32Thread()->Desktop, DesktopWindow, DesktopRgn);
  NtGdiDeleteObject(DesktopRgn);
  IntReleaseWindowObject(DesktopWindow);
}


BOOL FASTCALL
IntIsChildWindow(HWND Parent, HWND Child)
{
  PWINDOW_OBJECT BaseWindow = IntGetWindowObject(Child);
  PWINDOW_OBJECT Window = BaseWindow;
  while (Window != NULL && Window->Style & WS_CHILD)
    {
      if (Window->Self == Parent)
	{
	  IntReleaseWindowObject(BaseWindow);
	  return(TRUE);
	}
      Window = Window->Parent;
    }
  IntReleaseWindowObject(BaseWindow);
  return(FALSE);  
}


BOOL FASTCALL
IntIsWindowVisible(HWND Wnd)
{
  PWINDOW_OBJECT BaseWindow = IntGetWindowObject(Wnd);
  PWINDOW_OBJECT Window = BaseWindow;
  BOOLEAN Result = FALSE;
  while (Window != NULL && Window->Style & WS_CHILD)
    {
      if (!(Window->Style & WS_VISIBLE))
	{
	  IntReleaseWindowObject(BaseWindow);
	  return(FALSE);
	}
      Window = Window->Parent;
    }
  if (Window != NULL && Window->Style & WS_VISIBLE)
    {
      Result = TRUE;
    }
  IntReleaseWindowObject(BaseWindow);
  return(Result);
}


/* link the window into siblings and parent. children are kept in place. */
VOID FASTCALL
IntLinkWindow(
  PWINDOW_OBJECT Wnd, 
  PWINDOW_OBJECT WndParent,
  PWINDOW_OBJECT WndPrevSibling /* set to NULL if top sibling */
  )
{
  Wnd->Parent = WndParent; 

  if ((Wnd->PrevSibling = WndPrevSibling))
  {
    /* link after WndPrevSibling */
    if ((Wnd->NextSibling = WndPrevSibling->NextSibling)) Wnd->NextSibling->PrevSibling = Wnd;
    else if (Wnd->Parent->LastChild == WndPrevSibling) Wnd->Parent->LastChild = Wnd;
    Wnd->PrevSibling->NextSibling = Wnd;
  }
  else
  {
    /* link at top */
    if ((Wnd->NextSibling = WndParent->FirstChild)) Wnd->NextSibling->PrevSibling = Wnd;
    else Wnd->Parent->LastChild = Wnd;
    WndParent->FirstChild = Wnd;
  }

}

/*****************************************************************
 *              set_focus_window
 *
 * Change the focus window, sending the WM_SETFOCUS and WM_KILLFOCUS messages
 */
static HWND FASTCALL
set_focus_window(HWND New, PWINDOW_OBJECT Window, HWND Previous)
{
  PDESKTOP_OBJECT Desktop;

  ASSERT(NULL == Window || New == Window->Self);

  Desktop = IntGetActiveDesktop();
  ASSERT(NULL != Desktop);

  if (Window != NULL)
    {
      Window->MessageQueue->FocusWindow = New;
      (PUSER_MESSAGE_QUEUE)Desktop->ActiveMessageQueue =
        Window->MessageQueue;
    }
  else
    {
      (PUSER_MESSAGE_QUEUE) Desktop->ActiveMessageQueue = NULL;
    }

  if (Previous == New)
    {
      return Previous;
    }

  if (NULL != Previous)
    {
      NtUserSendMessage(Previous, WM_KILLFOCUS, (WPARAM) New, 0);

      if ((NULL == Desktop->ActiveMessageQueue && NULL != New)
          || (NULL != Desktop->ActiveMessageQueue
              && ((PUSER_MESSAGE_QUEUE)(Desktop->ActiveMessageQueue))->FocusWindow != New))
        {
          /* changed by the message */
          return Previous;
        }
    }

  if (IntIsWindow(New))
    {
      NtUserSendMessage(New, WM_SETFOCUS, (WPARAM) Previous, 0);
    }

  return Previous;
}

HWND FASTCALL
IntSetFocusWindow(HWND hWnd)
{
  PUSER_MESSAGE_QUEUE OldMessageQueue;
  PDESKTOP_OBJECT DesktopObject;
  PWINDOW_OBJECT WindowObject;
  HWND hWndOldFocus;
  HWND hWndTop;

  DPRINT("IntSetFocusWindow(hWnd 0x%x)\n", hWnd);

  if (hWnd != (HWND)0)
    {
      WindowObject = IntGetWindowObject(hWnd);
      if (!WindowObject)
        {
          DPRINT("Bad window handle 0x%x\n", hWnd);
          SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      	  return (HWND)0;
        }
    }
  else
    {
      WindowObject = NULL;
    }

  DesktopObject = IntGetActiveDesktop();
  if (! DesktopObject)
    {
      DPRINT("No active desktop\n");
      if (WindowObject != NULL)
        {
          IntReleaseWindowObject(WindowObject);
        }
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return (HWND)0;
    }

  hWndOldFocus = (HWND)0;
  OldMessageQueue = (PUSER_MESSAGE_QUEUE)DesktopObject->ActiveMessageQueue;
  if (OldMessageQueue != NULL)
    {
      hWndOldFocus = OldMessageQueue->FocusWindow;
    }

  if (hWndOldFocus == hWnd)
    {
      /* Nothing to do */
      IntReleaseWindowObject(WindowObject);
      return hWndOldFocus;
    }

  if (NULL != WindowObject)
    {
      hWndTop = hWnd;
      for (;;)
        {
          LONG style = NtUserGetWindowLong(hWndTop, GWL_STYLE, FALSE);
          if (style & (WS_MINIMIZE | WS_DISABLED))
            {
              IntReleaseWindowObject(WindowObject);
              return NULL;
            }
          if (! (style & WS_CHILD))
            {
              break;
            }
          hWndTop = NtUserGetAncestor(hWndTop, GA_PARENT);
        }

#if 0 /* FIXME */
      /* call hooks */
      if (HOOK_CallHooks(WH_CBT, HCBT_SETFOCUS, (WPARAM)hwnd, (LPARAM)previous, TRUE))
        {
          IntReleaseWindowObject(WindowObject);
          return NULL;
        }
#endif

      /* activate hWndTop if needed. */
      if (hWndTop != NtUserGetActiveWindow())
        {
#ifdef TODO
          if (! set_active_window(hWndTop, NULL, FALSE, FALSE))
            {
              return NULL;
            }
#endif
#if 0 /* FIXME */
          if (! NtUserIsWindow(hWnd))
            {
              /* Abort if window destroyed */
              return NULL;
            }
#endif
        }
    }
#if 0 /* FIXME */
  else
    {
      /* call hooks */
      if (HOOK_CallHooks(WH_CBT, HCBT_SETFOCUS, (WPARAM)hwnd, (LPARAM)previous, TRUE))
        {
          IntReleaseWindowObject(WindowObject);
          return NULL;
        }
    }
#endif

  hWndOldFocus = set_focus_window(hWnd, WindowObject, hWndOldFocus);
  if (WindowObject != NULL)
    {
      IntReleaseWindowObject(WindowObject);
    }

  DPRINT("hWndOldFocus = 0x%x\n", hWndOldFocus);

  return hWndOldFocus;
}


HWND FASTCALL
IntSetOwner(HWND hWnd, HWND hWndNewOwner)
{
  PWINDOW_OBJECT Wnd, WndOldOwner;
  HWND ret;
  
  Wnd = IntGetWindowObject(hWnd);
  WndOldOwner = Wnd->Owner;
  if (WndOldOwner)
  {
     ret = WndOldOwner->Self;
     IntReleaseWindowObject(WndOldOwner);
  }
  else
  {
     ret = 0;
  }
  Wnd->Owner = IntGetWindowObject(hWndNewOwner);
  IntReleaseWindowObject(Wnd);
  return ret;
}

PWINDOW_OBJECT FASTCALL
IntSetParent(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndNewParent)
{
   PWINDOW_OBJECT WndOldParent;
   HWND hWnd, hWndNewParent, hWndOldParent;
   BOOL WasVisible;

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

   WndOldParent = Wnd->Parent;
   hWndOldParent = WndOldParent->Self;

   if (WndNewParent != WndOldParent)
   {
      IntUnlinkWindow(Wnd);
      IntLinkWindow(Wnd, WndNewParent, NULL /*prev sibling*/);

      if (WndNewParent->Self != IntGetDesktopWindow()) /* a child window */
      {
         if (!(Wnd->Style & WS_CHILD))
         {
            //if ( Wnd->Menu ) DestroyMenu ( Wnd->menu );
            Wnd->IDMenu = 0;
         }
      }
   }

   /*
    * SetParent additionally needs to make hwnd the topmost window
    * in the x-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   WinPosSetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | (WasVisible ? SWP_SHOWWINDOW : 0));

   /*
    * FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
    * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE
    */
   
   /*
    * Validate that the old parent still exist, since it migth have been
    * destroyed during the last callbacks to user-mode 
    */
   return !IntIsWindow(hWndOldParent) ? NULL : WndOldParent;
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
  NtUserSendMessage(Wnd, WM_DESTROY, 0, 0);

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


BOOL FASTCALL
IntSetSystemMenu(PWINDOW_OBJECT WindowObject, PMENU_OBJECT MenuObject)
{
  PMENU_OBJECT OldMenuObject;
  if(WindowObject->SystemMenu)
  {
    OldMenuObject = IntGetMenuObject(WindowObject->SystemMenu);
    if(OldMenuObject)
    {
      OldMenuObject->IsSystemMenu = FALSE;
      IntReleaseMenuObject(OldMenuObject);
    }
  }
  
  WindowObject->SystemMenu = MenuObject->Self;
  if(MenuObject) /* FIXME check window style, propably return FALSE ? */
    MenuObject->IsSystemMenu = TRUE;
  
  return TRUE;
}


/* unlink the window from siblings and parent. children are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd)
{
  PWINDOW_OBJECT WndParent = Wnd->Parent; 
 
  if (Wnd->NextSibling) Wnd->NextSibling->PrevSibling = Wnd->PrevSibling;
  else if (WndParent->LastChild == Wnd) WndParent->LastChild = Wnd->PrevSibling;
 
  if (Wnd->PrevSibling) Wnd->PrevSibling->NextSibling = Wnd->NextSibling;
  else if (WndParent->FirstChild == Wnd) WndParent->FirstChild = Wnd->NextSibling;
  //else if (parent->first_unlinked == win) parent->first_unlinked = Wnd->NextSibling;
}


BOOLEAN FASTCALL
IntWndBelongsToThread(PWINDOW_OBJECT Window, PW32THREAD ThreadData)
{
  if (Window->OwnerThread && Window->OwnerThread->Win32Thread)
  {
    return (Window->OwnerThread->Win32Thread == ThreadData);
  }

  return FALSE;
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
  ULONG dwCount = 0;

  /* FIXME handle bChildren */
  if ( hwndParent )
    {
      PWINDOW_OBJECT WindowObject = NULL;
      PWINDOW_OBJECT Child;

      WindowObject = IntGetWindowObject ( hwndParent );
      if ( !WindowObject )
	{
	  DPRINT("Bad window handle 0x%x\n", hwndParent);
	  SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
	  return 0;
	}

      ExAcquireFastMutex ( &WindowObject->ChildrenListLock );
      Child = WindowObject->FirstChild;
      while (Child)
	{
	  if ( pWnd && dwCount < nBufSize )
	    pWnd[dwCount] = Child->Self;
	  dwCount++;
    Child = Child->NextSibling;
	}
      ExReleaseFastMutex ( &WindowObject->ChildrenListLock );
      IntReleaseWindowObject ( WindowObject );
    }
  else if ( dwThreadId )
    {
      NTSTATUS Status;
      struct _ETHREAD* Thread;
      struct _EPROCESS* ThreadsProcess;
      struct _W32PROCESS*   Win32Process;
      struct _WINSTATION_OBJECT* WindowStation;
      PUSER_HANDLE_TABLE HandleTable;
      PLIST_ENTRY Current;
      PUSER_HANDLE_BLOCK Block = NULL;
      ULONG i;

      Status = PsLookupThreadByThreadId ( (PVOID)dwThreadId, &Thread );
      if ( !NT_SUCCESS(Status) || !Thread )
	{
	  DPRINT("Bad ThreadId 0x%x\n", dwThreadId );
	  SetLastWin32Error(ERROR_INVALID_HANDLE);
	  return 0;
	}
      ThreadsProcess = Thread->ThreadsProcess;
      ASSERT(ThreadsProcess);
      Win32Process = ThreadsProcess->Win32Process;
      ASSERT(Win32Process);
      WindowStation = Win32Process->WindowStation;
      ASSERT(WindowStation);
      HandleTable = (PUSER_HANDLE_TABLE)(WindowStation->HandleTable);
      ASSERT(HandleTable);

      ExAcquireFastMutex(&HandleTable->ListLock);

      Current = HandleTable->ListHead.Flink;
      while ( Current != &HandleTable->ListHead )
	{
	  Block = CONTAINING_RECORD(Current, USER_HANDLE_BLOCK, ListEntry);
	  for ( i = 0; i < HANDLE_BLOCK_ENTRIES; i++ )
	    {
	      PVOID ObjectBody = Block->Handles[i].ObjectBody;
	      if ( ObjectBody )
	      {
		if ( pWnd && dwCount < nBufSize )
		  {
		    pWnd[dwCount] =
		      (HWND)ObmReferenceObjectByPointer ( ObjectBody, otWindow );
		  }
		dwCount++;
	      }
	    }
	  Current = Current->Flink;
	}

      ExReleaseFastMutex(&HandleTable->ListLock);
    }
  else
    {
      PDESKTOP_OBJECT DesktopObject = NULL;
      KIRQL OldIrql;
      PWINDOW_OBJECT Child, WndDesktop;

      if ( hDesktop )
	DesktopObject = IntGetDesktopObject ( hDesktop );
      else
	DesktopObject = IntGetActiveDesktop();
      if (!DesktopObject)
	{
	  DPRINT("Bad desktop handle 0x%x\n", hDesktop );
	  SetLastWin32Error(ERROR_INVALID_HANDLE);
	  return 0;
	}

      KeAcquireSpinLock ( &DesktopObject->Lock, &OldIrql );

      WndDesktop = IntGetWindowObject(DesktopObject->DesktopWindow);
      Child = (WndDesktop ? WndDesktop->FirstChild : NULL);
      while (Child)
	{
	  if ( pWnd && dwCount < nBufSize )
	    pWnd[dwCount] = Child->Self;
	  dwCount++;
    Child = Child->NextSibling;
	}
      KeReleaseSpinLock ( &DesktopObject->Lock, OldIrql );
    }

  return dwCount;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserCallHwndLock(
  HWND hWnd,
  DWORD Unknown1)
{
  UNIMPLEMENTED
  /* DrawMenuBar() calls it with Unknown1==0x55 */
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserChildWindowFromPointEx(HWND Parent,
			     LONG x,
			     LONG y,
			     UINT Flags)
{
  UNIMPLEMENTED

  return(0);
}


/*
 * @implemented
 */
HWND STDCALL
NtUserCreateWindowEx(DWORD dwExStyle,
		     PUNICODE_STRING lpClassName,
		     PUNICODE_STRING lpWindowName,
		     DWORD dwStyle,
		     LONG x,
		     LONG y,
		     LONG nWidth,
		     LONG nHeight,
		     HWND hWndParent,
		     HMENU hMenu,
		     HINSTANCE hInstance,
		     LPVOID lpParam,
		     DWORD dwShowMode)
{
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  PWINDOW_OBJECT WindowObject;
  PWINDOW_OBJECT ParentWindow;
  HWND ParentWindowHandle;
  HWND OwnerWindowHandle;
  PMENU_OBJECT SystemMenu;
  UNICODE_STRING WindowName;
  NTSTATUS Status;
  HANDLE Handle;
#if 0
  POINT MaxSize, MaxPos, MinTrack, MaxTrack;
#else
  POINT MaxPos;
#endif
  CREATESTRUCTW Cs;
  LRESULT Result;
  DPRINT("NtUserCreateWindowEx\n");

  DPRINT("NtUserCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);

  /* Initialize gui state if necessary. */
  IntGraphicsCheck(TRUE);

  if (!RtlCreateUnicodeString(&WindowName,
                              NULL == lpWindowName->Buffer ?
                              L"" : lpWindowName->Buffer))
    {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return((HWND)0);
    }

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

  ParentWindow = IntGetWindowObject(ParentWindowHandle);
    
  /* FIXME: parent must belong to the current process */

  /* Check the class. */
  Status = ClassReferenceClassByNameOrAtom(&ClassObject, lpClassName->Buffer);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&WindowName);
      IntReleaseWindowObject(ParentWindow);
      return((HWND)0);
    }

  /* Check the window station. */
  DPRINT("IoGetCurrentProcess() %X\n", IoGetCurrentProcess());
  DPRINT("PROCESS_WINDOW_STATION %X\n", PROCESS_WINDOW_STATION());
  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&WindowName);
      ObmDereferenceObject(ClassObject);
      IntReleaseWindowObject(ParentWindow);
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
      ObmDereferenceObject(ClassObject);
      RtlFreeUnicodeString(&WindowName);
      IntReleaseWindowObject(ParentWindow);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return (HWND)0;
    }
  ObDereferenceObject(WinStaObject);

  /*
   * Fill out the structure describing it.
   */
  WindowObject->Class = ClassObject;
  WindowObject->ExStyle = dwExStyle;
  WindowObject->Style = dwStyle & ~WS_VISIBLE;
  DPRINT("1: Style is now %d\n", WindowObject->Style);
  
  SystemMenu = IntGetSystemMenu(WindowObject, TRUE, TRUE);
  if(SystemMenu)
  {
    WindowObject->SystemMenu = SystemMenu->Self;
    IntReleaseMenuObject(SystemMenu);
  }
  else
    WindowObject->SystemMenu = (HANDLE)0;
  
  WindowObject->x = x;
  WindowObject->y = y;
  WindowObject->Width = nWidth;
  WindowObject->Height = nHeight;
  WindowObject->ContextHelpId = 0;
  WindowObject->IDMenu = (UINT)hMenu;
  WindowObject->Instance = hInstance;
  WindowObject->Parameters = lpParam;
  WindowObject->Self = Handle;
  WindowObject->MessageQueue = PsGetWin32Thread()->MessageQueue;
  WindowObject->Parent = ParentWindow;
  WindowObject->Owner = IntGetWindowObject(OwnerWindowHandle);
  WindowObject->UserData = 0;
  WindowObject->Unicode = ClassObject->Unicode;
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
  ExInitializeFastMutex(&WindowObject->ChildrenListLock);

  RtlInitUnicodeString(&WindowObject->WindowName, WindowName.Buffer);
  RtlFreeUnicodeString(&WindowName);


  /* Correct the window style. */
  if (!(dwStyle & WS_CHILD))
    {
      WindowObject->Style |= WS_CLIPSIBLINGS;
      DPRINT("3: Style is now %d\n", WindowObject->Style);
      if (!(dwStyle & WS_POPUP))
	{
	  WindowObject->Style |= WS_CAPTION;
      WindowObject->Flags |= WINDOWOBJECT_NEED_SIZE;
      DPRINT("4: Style is now %d\n", WindowObject->Style);
	  /* FIXME: Note the window needs a size. */ 
	}
    }

  /* Insert the window into the thread's window list. */
  ExAcquireFastMutexUnsafe (&PsGetWin32Thread()->WindowListLock);
  InsertTailList (&PsGetWin32Thread()->WindowListHead, 
		  &WindowObject->ThreadListEntry);
  ExReleaseFastMutexUnsafe (&PsGetWin32Thread()->WindowListLock);

  /* Allocate a DCE for this window. */
  if (dwStyle & CS_OWNDC) WindowObject->Dce = DceAllocDCE(WindowObject->Self,DCE_WINDOW_DC);
  /* FIXME:  Handle "CS_CLASSDC" */

  /* Initialize the window dimensions. */
  WindowObject->WindowRect.left = x;
  WindowObject->WindowRect.top = y;
  WindowObject->WindowRect.right = x + nWidth;
  WindowObject->WindowRect.bottom = y + nHeight;
  if (0 != (WindowObject->Style & WS_CHILD))
    {
      NtGdiOffsetRect(&(WindowObject->WindowRect), WindowObject->Parent->ClientRect.left,
                      WindowObject->Parent->ClientRect.top);
    }
  WindowObject->ClientRect = WindowObject->WindowRect;

  /*
   * Get the size and position of the window.
   */
#if 0
  if ((dwStyle & WS_THICKFRAME) || !(dwStyle & (WS_POPUP | WS_CHILD)))
    {
      /* WinPosGetMinMaxInfo sends the WM_GETMINMAXINFO message */
      WinPosGetMinMaxInfo(WindowObject, &MaxSize, &MaxPos, &MinTrack,
			  &MaxTrack);
      x = min(MaxSize.x, y);
      y = min(MaxSize.y, y);
      x = max(MinTrack.x, x);
      y = max(MinTrack.y, y);
    }
#endif

  WindowObject->WindowRect.left = x;
  WindowObject->WindowRect.top = y;
  WindowObject->WindowRect.right = x + nWidth;
  WindowObject->WindowRect.bottom = y + nHeight;
  if (0 != (WindowObject->Style & WS_CHILD))
    {
      NtGdiOffsetRect(&(WindowObject->WindowRect), WindowObject->Parent->ClientRect.left,
                      WindowObject->Parent->ClientRect.top);
    }
  WindowObject->ClientRect = WindowObject->WindowRect;

  /* FIXME: Initialize the window menu. */
  
  /* Initialize the window's scrollbars */
  if (dwStyle & WS_VSCROLL)
      IntCreateScrollBar(WindowObject, SB_VERT);
  if (dwStyle & WS_HSCROLL)
      IntCreateScrollBar(WindowObject, SB_HORZ);

  /* Send a NCCREATE message. */
  Cs.lpCreateParams = lpParam;
  Cs.hInstance = hInstance;
  Cs.hMenu = hMenu;
  Cs.hwndParent = ParentWindowHandle;
  Cs.cx = nWidth;
  Cs.cy = nHeight;
  Cs.x = x;
  Cs.y = y;
  Cs.style = dwStyle;
  Cs.lpszName = lpWindowName->Buffer;
  Cs.lpszClass = lpClassName->Buffer;
  Cs.dwExStyle = dwExStyle;

    // AG: For some reason these don't get set already. This might need moving
    // elsewhere... What is actually done with WindowObject anyway, to retain
    // its data?
  DPRINT("[win32k.window] NtUserCreateWindowEx style %d, exstyle %d, parent %d\n", Cs.style, Cs.dwExStyle, Cs.hwndParent);
//  NtUserSetWindowLong(Handle, GWL_STYLE, WindowObject->Style, TRUE);
//  NtUserSetWindowLong(Handle, GWL_EXSTYLE, WindowObject->ExStyle, TRUE);
  DPRINT("NtUserCreateWindowEx(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);
  // Any more?

  DPRINT("NtUserCreateWindowEx(): About to send NCCREATE message.\n");
  Result = IntSendNCCREATEMessage(WindowObject->Self, &Cs);
  if (!Result)
    {
      /* FIXME: Cleanup. */
      IntReleaseWindowObject(ParentWindow);
      DPRINT("NtUserCreateWindowEx(): NCCREATE message failed.\n");
      return((HWND)0);
    }
 
  /* Calculate the non-client size. */
  MaxPos.x = WindowObject->WindowRect.left;
  MaxPos.y = WindowObject->WindowRect.top;
  DPRINT("NtUserCreateWindowEx(): About to get non-client size.\n");
  /* WinPosGetNonClientSize SENDS THE WM_NCCALCSIZE message */
  Result = WinPosGetNonClientSize(WindowObject->Self, 
				  &WindowObject->WindowRect,
				  &WindowObject->ClientRect);
  NtGdiOffsetRect(&WindowObject->WindowRect, 
		 MaxPos.x - WindowObject->WindowRect.left,
		 MaxPos.y - WindowObject->WindowRect.top);


  /* link the window into the parent's child list */
  ExAcquireFastMutexUnsafe(&ParentWindow->ChildrenListLock);
  if ((dwStyle & (WS_CHILD|WS_MAXIMIZE)) == WS_CHILD)
  {
    /* link window as bottom sibling */
    IntLinkWindow(WindowObject, ParentWindow, ParentWindow->LastChild /*prev sibling*/);
  }
  else
  {
    /* link window as top sibling */
    IntLinkWindow(WindowObject, ParentWindow, NULL /*prev sibling*/);
  }
  ExReleaseFastMutexUnsafe(&ParentWindow->ChildrenListLock);

  /* Send the WM_CREATE message. */
  DPRINT("NtUserCreateWindowEx(): about to send CREATE message.\n");
  Result = IntSendCREATEMessage(WindowObject->Self, &Cs);
  if (Result == (LRESULT)-1)
    {
      /* FIXME: Cleanup. */
      IntReleaseWindowObject(ParentWindow);
      DPRINT("NtUserCreateWindowEx(): send CREATE message failed.\n");
      return((HWND)0);
    } 
  
  /* Send move and size messages. */
  if (!(WindowObject->Flags & WINDOWOBJECT_NEED_SIZE))
    {
      LONG lParam;

      DPRINT("NtUserCreateWindow(): About to send WM_SIZE\n");

      if ((WindowObject->ClientRect.right - WindowObject->ClientRect.left) < 0 ||
          (WindowObject->ClientRect.bottom - WindowObject->ClientRect.top) < 0)
         DPRINT("Sending bogus WM_SIZE\n");
      
      lParam = MAKE_LONG(WindowObject->ClientRect.right - 
		  WindowObject->ClientRect.left,
		  WindowObject->ClientRect.bottom - 
		  WindowObject->ClientRect.top);
      IntCallWindowProc(NULL, WindowObject->Self, WM_SIZE, SIZE_RESTORED, 
          lParam);

      DPRINT("NtUserCreateWindow(): About to send WM_MOVE\n");

      if (0 != (WindowObject->Style & WS_CHILD))
	{
	  lParam = MAKE_LONG(WindowObject->ClientRect.left - WindowObject->Parent->ClientRect.left,
	      WindowObject->ClientRect.top - WindowObject->Parent->ClientRect.top);
	}
      else
	{
	  lParam = MAKE_LONG(WindowObject->ClientRect.left,
	      WindowObject->ClientRect.top);
	}
      IntCallWindowProc(NULL, WindowObject->Self, WM_MOVE, 0, lParam);
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
	((WindowObject->Style & WS_CHILD) || IntGetActiveWindow()) ?
	SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
	SWP_NOZORDER | SWP_FRAMECHANGED;
      DPRINT("NtUserCreateWindow(): About to minimize/maximize\n");
      WinPosSetWindowPos(WindowObject->Self, 0, NewPos.left, NewPos.top,
			 NewPos.right, NewPos.bottom, SwFlag);
    }

  /* Notify the parent window of a new child. */
  if ((WindowObject->Style & WS_CHILD) ||
      (!(WindowObject->ExStyle & WS_EX_NOPARENTNOTIFY)))
    {
      DPRINT("NtUserCreateWindow(): About to notify parent\n");
      IntCallWindowProc(NULL, WindowObject->Parent->Self,
			 WM_PARENTNOTIFY, 
			 MAKEWPARAM(WM_CREATE, WindowObject->IDMenu),
			 (LPARAM)WindowObject->Self);
    }

  if (dwStyle & WS_VISIBLE)
    {
      DPRINT("NtUserCreateWindow(): About to show window\n");
      WinPosShowWindow(WindowObject->Self, dwShowMode);
    }
  /* FIXME:  Should code be reworked to accomodate the following line? */
	DPRINT("Setting Active Window to %d\n\n\n",WindowObject->Self);
  NtUserSetActiveWindow(WindowObject->Self);
  DPRINT("NtUserCreateWindow(): = %X\n", Handle);
  DPRINT("WindowObject->SystemMenu = 0x%x\n", WindowObject->SystemMenu);
  return((HWND)Handle);
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
  HWND hWndFocus;

  Window = IntGetWindowObject(Wnd);
  if (Window == NULL)
    {
      return FALSE;
    }

  /* Check for desktop window */
  if (IntIsDesktopWindow(Window))
    {
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
    }

  /* Look whether the focus is within the tree of windows we will
   * be destroying.
   */
  hWndFocus = IntGetFocusWindow();
  if (hWndFocus == Wnd || IntIsChildWindow(Wnd, hWndFocus))
    {
      HWND Parent = NtUserGetAncestor(Wnd, GA_PARENT);
      if (Parent == IntGetDesktopWindow())
      	{
      	  Parent = NULL;
      	}
      IntSetFocusWindow(Parent);
    }

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

  if (! IsWindow(Wnd))
    {
    return TRUE;
    }
#endif

  /* Hide the window */
  if (! WinPosShowWindow(Wnd, SW_HIDE ))
    {
      if (Wnd == NtUserGetActiveWindow())
	{
          PWINDOW_OBJECT ActiveWindow = IntGetWindowObject(Wnd);
          if (NULL != ActiveWindow)
            {
              WinPosActivateOtherWindow(ActiveWindow);
            }
	}
    }

  if (!IntIsWindow(Wnd))
    {
    return TRUE;
    }

  /* Recursively destroy owned windows */
#if 0 /* FIXME */
  if (! isChild)
    {
      for (;;)
	{
	  int i;
	  BOOL GotOne = FALSE;
	  HWND *list;
	  UINT NumChildren;
	  PWINDOW_OBJECT Child;

	  if (IntBuildChildWindowArray(IntGetWindowObject(IntGetDesktopWindow()),
	          &list, &NumChildren))
	    {
	      for (i = 0; i < NumChildren; i++)
		{
		  Child = IntGetWindowObject(list[i]);
		  if (Child->Owner != Window)
		    {
		      continue;
		    }
		  if (IntWndBelongsToThread(Child, PsGetWin32Thread()))
		    {
		      IntReleaseWindowObject(Child);
		      NtUserDestroyWindow(list[i]);
		      GotOne = TRUE;		      
		      continue;
		    }
		  if (Child->Owner != NULL)
		    {
		      IntReleaseWindowObject(Child->Owner);
		      Child->Owner = NULL;
		    }
		  IntReleaseWindowObject(Child);
		}
	      ExFreePool(list);
	    }
	  if (! GotOne)
	    {
	      break;
	    }
	}
    }
#endif

  /* Send destroy messages */
  IntSendDestroyMsg(Wnd);

  if (!IntIsWindow(Wnd))
    {
      return TRUE;
    }

  /* Unlink now so we won't bother with the children later on */
  IntUnlinkWindow(Window);

  /* Destroy the window storage */
  IntDestroyWindow(Window, PsGetWin32Process(), PsGetWin32Thread(), TRUE);

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


/*
 * FUNCTION:
 *   Searches a window's children for a window with the specified
 *   class and name
 * ARGUMENTS:
 *   hwndParent	    = The window whose childs are to be searched. 
 *					  NULL = desktop
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
 *
 * FIXME:
 *   Should use MmCopyFromCaller, we don't want an access violation in here
 *	 
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
  NTSTATUS status;
  HWND windowHandle;
  PWINDOW_OBJECT ParentWindow, WndChildAfter, WndChild;
  PWNDCLASS_OBJECT classObject;
  
  // Get a pointer to the class
  status = ClassReferenceClassByNameOrAtom(&classObject, ucClassName->Buffer);
  if (!NT_SUCCESS(status))
    {
      return NULL;
    }
  
  // If hwndParent==NULL use the desktop window instead
  if(!hwndParent)
      hwndParent = PsGetWin32Thread()->Desktop->DesktopWindow;

  // Get the object
  ParentWindow = IntGetWindowObject(hwndParent);

  if(!ParentWindow)
    {
      ObmDereferenceObject(classObject);
      return NULL;      
    }

  ExAcquireFastMutexUnsafe (&ParentWindow->ChildrenListLock);

  if(hwndChildAfter)
  {
    if (!(WndChildAfter = IntGetWindowObject(hwndChildAfter)))
    {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
    }

    /* must be a direct child (not a decendant child)*/
    if (WndChildAfter->Parent != ParentWindow)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
    }
    
    WndChild = WndChildAfter->NextSibling;
  }
  else
  {
    WndChild = ParentWindow->FirstChild;
  }

  while (WndChild)
  {
    if (classObject == WndChild->Class && (ucWindowName->Buffer==NULL || 
        RtlCompareUnicodeString (ucWindowName, &WndChild->WindowName, TRUE) == 0))
    {
      windowHandle = WndChild->Self;

      ExReleaseFastMutexUnsafe (&ParentWindow->ChildrenListLock);
      IntReleaseWindowObject(ParentWindow);
      ObmDereferenceObject (classObject);
      
      return windowHandle;
    }
    
    WndChild = WndChild->NextSibling;
  }

  ExReleaseFastMutexUnsafe (&ParentWindow->ChildrenListLock);

  IntReleaseWindowObject(ParentWindow);
  ObmDereferenceObject (classObject);

  return  NULL;
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
   PWINDOW_OBJECT Wnd, WndAncestor;
   HWND hWndAncestor = NULL;

   if (hWnd == IntGetDesktopWindow())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return NULL;
   }

   IntAcquireWinLockShared();

   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      IntReleaseWinLock();
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   switch (Type)
   {
      case GA_PARENT:
         WndAncestor = Wnd->Parent;
         break;

      case GA_ROOT:
         WndAncestor = Wnd;
         while (!IntIsDesktopWindow(WndAncestor->Parent))
            WndAncestor = WndAncestor->Parent;
         break;
    
      case GA_ROOTOWNER:
         WndAncestor = Wnd;
         for (;;)
         {
            PWINDOW_OBJECT Parent = IntGetParent(WndAncestor);
            if (!Parent) break;
            WndAncestor = Parent;
         }
         break;

      default:
         WndAncestor = NULL;
   }

   if (WndAncestor)
   {
      hWndAncestor = WndAncestor->Self;
   }

   IntReleaseWindowObject(Wnd);
   IntReleaseWinLock();

   return hWndAncestor;
}


/*
 * @implemented
 */
HWND
STDCALL
NtUserGetCapture(VOID)
{
  PWINDOW_OBJECT Window;
  Window = IntGetCaptureWindow();
  if (Window != NULL)
    {
      return(Window->Self);
    }
  else
    {
      return(NULL);
    }
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

  IntAcquireWinLockShared();
  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    IntReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);      
    return FALSE;
  }

  IntGetClientRect(WindowObject, &SafeRect);
  IntReleaseWinLock();

  if (! NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
  {
    return(FALSE);
  }

  return(TRUE);
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
NtUserGetForegroundWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
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
 * @unimplemented
 */
DWORD STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
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

   IntAcquireWinLockShared();

   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      IntReleaseWinLock();
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   WndParent = IntGetParent(Wnd);
   if (WndParent)
   {
      hWndParent = WndParent->Self;
   }

   IntReleaseWindowObject(Wnd);
   IntReleaseWinLock();

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

   IntAcquireWinLockExclusive();
   if (hWndNewParent)
   {
      if (!(WndParent = IntGetWindowObject(hWndNewParent)))
      {
         IntReleaseWinLock();
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return NULL;
      }
   }
   else
   {
      if (!(WndParent = IntGetWindowObject(IntGetDesktopWindow())))
      {
         IntReleaseWinLock();
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return NULL;
      }
   }

   if (!(Wnd = IntGetWindowObject(hWndChild)))
   {
      IntReleaseWinLock();
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }

   WndOldParent = IntSetParent(Wnd, WndParent);

   if (WndOldParent)
   {
      hWndOldParent = WndOldParent->Self;
   }

   IntReleaseWindowObject(Wnd);
   IntReleaseWindowObject(WndParent);
   IntReleaseWinLock();

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
   return hwndShellWindow;
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

DWORD STDCALL
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
   /*
    * Test if we are permitted to change the shell window.
    */
   if (hwndShellWindow)
      return FALSE;

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
         return FALSE;
   }

   if (NtUserGetWindowLong(hwndShell, GWL_EXSTYLE, FALSE) & WS_EX_TOPMOST)
      return FALSE;

   WinPosSetWindowPos(hwndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

   hwndShellWindow = hwndShell;
   hwndShellListView = hwndListView;

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
      /*
       * Return the handle of the first submenu.
       */
      ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
      if (MenuObject->MenuItemList)
      {
         Result = MenuObject->MenuItemList->hSubMenu;
         if (!IntIsMenu(Result))
            Result = 0;
      }    
      ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
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
   PWINDOW_OBJECT WindowObject;
   HWND hWndResult = NULL;

   IntAcquireWinLockShared();

   if (!(WindowObject = IntGetWindowObject(hWnd)))
   {
      IntReleaseWinLock();
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return NULL;
   }
  
   switch (Relationship)
   {
      case GW_HWNDFIRST:
         if (WindowObject->Parent && WindowObject->Parent->FirstChild)
            hWndResult = WindowObject->Parent->FirstChild->Self;
         break;

      case GW_HWNDLAST:
         if (WindowObject->Parent && WindowObject->Parent->LastChild)
            hWndResult = WindowObject->Parent->LastChild->Self;
         break;

      case GW_HWNDNEXT:
         if (WindowObject->NextSibling)
            hWndResult = WindowObject->NextSibling->Self;
         break;

      case GW_HWNDPREV:
         if (WindowObject->PrevSibling)
            hWndResult = WindowObject->PrevSibling->Self;
         break;

      case GW_OWNER:
         if (WindowObject->Owner)
            hWndResult = WindowObject->Owner->Self;
         break;
      case GW_CHILD:
         if (WindowObject->FirstChild)
            hWndResult = WindowObject->FirstChild->Self;
         break;
   }

   IntReleaseWindowObject(WindowObject);
   IntReleaseWinLock();

   return hWndResult;
}

/*
 * NtUserGetWindowDC
 *
 * The NtUserGetWindowDC function retrieves the device context (DC) for the
 * entire window, including title bar, menus, and scroll bars. A window device
 * context permits painting anywhere in a window, because the origin of the
 * device context is the upper-left corner of the window instead of the client
 * area. 
 *
 * Status
 *    @implemented
 */

DWORD STDCALL
NtUserGetWindowDC(HWND hWnd)
{
   return (DWORD)NtUserGetDCEx(hWnd, 0, DCX_USESTYLE | DCX_WINDOW);
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
   PWINDOW_OBJECT WindowObject;
   LONG Result;

   DPRINT("NtUserGetWindowLong(%x,%d,%d)\n", hWnd, (INT)Index, Ansi);

   /*
    * Don't allow GetWindowLong with desktop window handle.
    */
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
      if (Index > WindowObject->ExtraDataSize - sizeof(LONG))
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
            if (WindowObject->Parent->Self == IntGetDesktopWindow())
               Result = (LONG) NtUserGetWindow(WindowObject->Self, GW_OWNER);
            else
               Result = (LONG) NtUserGetAncestor(WindowObject->Self, GA_PARENT);
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
   PWINDOW_OBJECT WindowObject;
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
      if (Index > WindowObject->ExtraDataSize - sizeof(LONG))
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
            if (hWnd == hwndShellWindow || hWnd == hwndShellListView)
               Style.styleNew &= ~WS_EX_TOPMOST;

            IntSendSTYLECHANGINGMessage(hWnd, GWL_EXSTYLE, &Style);
            WindowObject->ExStyle = (DWORD)Style.styleNew;
            IntSendSTYLECHANGEDMessage(hWnd, GWL_EXSTYLE, &Style);
            break;

         case GWL_STYLE:
            OldValue = (LONG) WindowObject->Style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            IntSendSTYLECHANGINGMessage(hWnd, GWL_STYLE, &Style);
            WindowObject->Style = (DWORD)Style.styleNew;
            IntSendSTYLECHANGEDMessage(hWnd, GWL_STYLE, &Style);
            break;

         case GWL_WNDPROC:
            /* FIXME: should check if window belongs to current process */
            if (Ansi)
            {
               OldValue = (LONG) WindowObject->WndProcA;
               WindowObject->WndProcA = (WNDPROC) NewValue;
               WindowObject->WndProcW = (WNDPROC) NewValue + 0x80000000;
               WindowObject->Unicode = FALSE;
            }
            else
            {
               OldValue = (LONG) WindowObject->WndProcW;
               WindowObject->WndProcW = (WNDPROC) NewValue;
               WindowObject->WndProcA = (WNDPROC) NewValue + 0x80000000;
               WindowObject->Unicode = TRUE;
            }
            break;

         case GWL_HINSTANCE:
            OldValue = (LONG) WindowObject->Instance;
            WindowObject->Instance = (HINSTANCE) NewValue;
            break;

         case GWL_HWNDPARENT:
            if (WindowObject->Parent->Self == IntGetDesktopWindow())
               OldValue = (LONG) IntSetOwner(WindowObject->Self, (HWND) NewValue);
            else
               OldValue = (LONG) NtUserSetParent(WindowObject->Self, (HWND) NewValue);
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
 * @unimplemented
 */
DWORD STDCALL
NtUserGetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
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
  RECT SafeRect;

  IntAcquireWinLockShared();
  if (!(Wnd = IntGetWindowObject(hWnd)))
  {
    IntReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);      
    return FALSE;
  }
  
  SafeRect = Wnd->WindowRect;
  IntReleaseWinLock();

  if (! NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
  {
    return FALSE;
  }

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

   IntAcquireWinLockShared();

   if (!(Wnd = IntGetWindowObject(hWnd)))
   {
      IntReleaseWinLock();
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }

   tid = IntGetWindowThreadProcessId(Wnd, &pid);
   IntReleaseWinLock();
   
   if (UnsafePid) MmCopyToCaller(UnsafePid, &pid, sizeof(DWORD));
   
   return tid;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserInternalGetWindowText(HWND hWnd,
			    LPWSTR lpString,
			    int nMaxCount)
{
  DWORD res = 0;
  PWINDOW_OBJECT WindowObject;
  
  IntAcquireWinLockShared(); /* ??? */
  WindowObject = IntGetWindowObject(hWnd);
  if(!WindowObject)
  {
    IntReleaseWinLock(); /* ??? */
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  
  if(lpString)
  {
    /* FIXME - Window text is currently stored
               in the Atom 'USER32!WindowTextAtomA' */
    
  }
  else
  {
    /* FIXME - return length of window text */
  }
  
  IntReleaseWindowObject(WindowObject);
  
  IntReleaseWinLock(); /* ??? */
  return res;
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
	UINT	flags = SWP_NOZORDER | SWP_NOACTIVATE;

	if(!bRepaint)
		flags |= SWP_NOREDRAW;
	return NtUserSetWindowPos(hWnd, 0, X, Y, nWidth, nHeight, flags);
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
      case 0x00:
         Result = (DWORD)Window->OwnerThread->ThreadsProcess->UniqueProcessId;
         break;

      case 0x01:
         Result = (DWORD)Window->OwnerThread->Cid.UniqueThread;
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
BOOL
STDCALL
NtUserRedrawWindow
(
 HWND hWnd,
 CONST RECT *lprcUpdate,
 HRGN hrgnUpdate,
 UINT flags
)
{
 RECT SafeUpdateRect;
 NTSTATUS Status;
 PWINDOW_OBJECT Wnd;

 if (!(Wnd = IntGetWindowObject(hWnd)))
 {
   SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
   return FALSE;
 }

 if(NULL != lprcUpdate)
 {
  Status = MmCopyFromCaller(&SafeUpdateRect, (PRECT)lprcUpdate, sizeof(RECT));

  if(!NT_SUCCESS(Status))
  {
   /* FIXME: set last error */
   return FALSE;
  }
 }


 Status = PaintRedrawWindow
 (
  Wnd,
  NULL == lprcUpdate ? NULL : &SafeUpdateRect,
  hrgnUpdate,
  flags,
  0
 );


 if(!NT_SUCCESS(Status))
 {
  /* FIXME: set last error */
  return FALSE;
 }
 
 return TRUE;
}


/*
 * @implemented
 */
UINT STDCALL
NtUserRegisterWindowMessage(PUNICODE_STRING MessageNameUnsafe)
{
  PLIST_ENTRY Current;
  PREGISTERED_MESSAGE NewMsg, RegMsg;
  UINT Msg = REGISTERED_MESSAGE_MIN;
  UNICODE_STRING MessageName;
  NTSTATUS Status;

  Status = MmCopyFromCaller(&MessageName, MessageNameUnsafe, sizeof(UNICODE_STRING));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return 0;
    }

  NewMsg = ExAllocatePoolWithTag(PagedPool,
                                 sizeof(REGISTERED_MESSAGE) +
                                 MessageName.Length,
                                 TAG_WNAM);
  if (NULL == NewMsg)
    {
      SetLastNtError(STATUS_NO_MEMORY);
      return 0;
    }

  Status = MmCopyFromCaller(NewMsg->MessageName, MessageName.Buffer, MessageName.Length);
  if (! NT_SUCCESS(Status))
    {
      ExFreePool(NewMsg);
      SetLastNtError(Status);
      return 0;
    }
  NewMsg->MessageName[MessageName.Length / sizeof(WCHAR)] = L'\0';
  if (wcslen(NewMsg->MessageName) != MessageName.Length / sizeof(WCHAR))
    {
      ExFreePool(NewMsg);
      SetLastNtError(STATUS_INVALID_PARAMETER);
      return 0;
    }

  Current = RegisteredMessageListHead.Flink;
  while (Current != &RegisteredMessageListHead)
    {
      RegMsg = CONTAINING_RECORD(Current, REGISTERED_MESSAGE, ListEntry);
      if (0 == wcscmp(NewMsg->MessageName, RegMsg->MessageName))
	{
	  ExFreePool(NewMsg);
	  return Msg;
	}
      Msg++;
      Current = Current->Flink;
    }

  if (REGISTERED_MESSAGE_MAX < Msg)
    {
      ExFreePool(NewMsg);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return 0;
    }

  InsertTailList(&RegisteredMessageListHead, &(NewMsg->ListEntry));

  return Msg;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserScrollWindowEx(DWORD Unknown0,
		     DWORD Unknown1,
		     DWORD Unknown2,
		     DWORD Unknown3,
		     DWORD Unknown4,
		     DWORD Unknown5,
		     DWORD Unknown6,
		     DWORD Unknown7)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
HWND STDCALL
NtUserSetCapture(HWND Wnd)
{
  PWINDOW_OBJECT Window;
  PWINDOW_OBJECT Prev;

  Prev = IntGetCaptureWindow();

  if (Prev != NULL)
    {
      IntSendMessage(Prev->Self, WM_CAPTURECHANGED, 0L, (LPARAM)Wnd, FALSE);
    }

  if (Wnd == NULL)
    {
      IntSetCaptureWindow(NULL);
    }
  else  
    {
      Window = IntGetWindowObject(Wnd);
      IntSetCaptureWindow(Window);
      IntReleaseWindowObject(Window);
    }
  if (Prev != NULL)
    {
      return(Prev->Self);
    }
  else
    {
      return(NULL);
    }
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
  HMENU hMenu,
  BOOL bRepaint)
{
  PWINDOW_OBJECT WindowObject;
  PMENU_OBJECT MenuObject;
  BOOL Changed = FALSE;
  WindowObject = IntGetWindowObject((HWND)hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  if(hMenu)
  {
    /* assign new menu handle */
    MenuObject = IntGetMenuObject((HWND)hMenu);
    if(!MenuObject)
    {
      IntReleaseWindowObject(WindowObject);
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      return FALSE;
    }
    
    Changed = (WindowObject->IDMenu != (UINT)hMenu);
    WindowObject->IDMenu = (UINT)hMenu;
    
    IntReleaseMenuObject(MenuObject);
  }
  else
  {
    /* remove the menu handle */
    Changed = (WindowObject->IDMenu != 0);
    WindowObject->IDMenu = 0;
  }
  
  IntReleaseWindowObject(WindowObject);
  
  if(Changed && bRepaint && IntIsWindowVisible(hWnd))
  {
    WinPosSetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
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
 * @unimplemented
 */
DWORD STDCALL
NtUserSetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
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
   PWINDOW_OBJECT WindowObject;

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if (WindowObject->Style & WS_CHILD)
   {
      X += WindowObject->Parent->ClientRect.left;
      Y += WindowObject->Parent->ClientRect.top;
   }

   IntReleaseWindowObject(WindowObject);

   return WinPosSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserSetWindowRgn(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
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
 * @implemented
 */
BOOL STDCALL
NtUserUpdateWindow(HWND hWnd)
{
    PWINDOW_OBJECT pWindow = IntGetWindowObject( hWnd);

    if (!pWindow)
        return FALSE;
    if (pWindow->UpdateRegion)
        NtUserSendMessage( hWnd, WM_PAINT,0,0);
    IntReleaseWindowObject(pWindow);
    return TRUE;
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


#define POINT_IN_RECT(p, r) (((r.bottom >= p.y) && (r.top <= p.y))&&((r.left <= p.x )&&( r.right >= p.x )))
static BOOL IsStaticClass(PWINDOW_OBJECT Window)
{
    BOOL rc = FALSE;

    ASSERT(0 != Window->Class);
    ASSERT(0 != Window->Class->lpszClassName);
    
    DbgPrint("FIXME: Update IsStatic to really check if a window is a static\n");
    /*
    if (0 == wcscmp((LPWSTR)Window->Class->lpszClassName, L"Static"))
        rc = TRUE;
    */    
    return rc;
}

static BOOL IsHidden(PWINDOW_OBJECT Window)
{
    BOOL rc = FALSE;
    ASSERT(0 != Window);
    PWINDOW_OBJECT wnd = Window;

    while (0 != wnd && wnd->Style & WS_CHILD)
    {
      if (!(wnd->Style & WS_VISIBLE))
	  {
	    rc = TRUE;
        break;
	  }
      wnd = wnd->Parent;
    }
    if (0 != wnd)
    {
      rc = !(wnd->Style & WS_VISIBLE);
    }

    return rc;
}

static BOOL IsDisabled(PWINDOW_OBJECT Window)
{
    BOOL rc = FALSE;
    ASSERT(0 != Window);

    rc = (Window->Style & WS_DISABLED);
    
    return rc;
}

static PWINDOW_OBJECT RestrictiveSearchChildWindows(PWINDOW_OBJECT Window, POINT p)
{
    PWINDOW_OBJECT ChildWindow = 0;
    PWINDOW_OBJECT rc          = Window;
    ASSERT(0 != Window);

    /*
    **Aquire a mutex to the Child list.
    */
    ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);

    /*
    **Now Find the first non-static window.
    */
    ChildWindow = Window->FirstChild;

    while (0 != ChildWindow)
    {
        /*
        **Test Restrictions on the window.
        */
        if (!IsStaticClass(ChildWindow) &&
            !IsDisabled(ChildWindow)    &&
            !IsHidden(ChildWindow)       )
        {
            /*
            **Now find the deepest child window
            */
            if (POINT_IN_RECT(p, ChildWindow->WindowRect))
            {
                rc = RestrictiveSearchChildWindows(ChildWindow, p);
                break;
            }
        }
        ChildWindow = ChildWindow->NextSibling;
    }
   
    /*
    **Release mutex.
    */     
    ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);

    return rc;
}

/*
 * NtUserWindowFromPoint
 *
 * Return Value
 *    The return value is a handle to the window that contains the point. 
 *    If no window exists at the given point, the return value is NULL. 
 *    If the point is over a static text control, the return value is a handle 
 *    to the window under the static text control. 
 *
 * Remarks
 *    The WindowFromPoint function does not retrieve a handle to a hidden or 
 *    disabled window, even if the point is within the window. An application
 *    should use the ChildWindowFromPoint function for a nonrestrictive search.
 *
 * Author
 *    Mark Tempel
 *
 * Status
 *    @implemented
 */

HWND STDCALL
NtUserWindowFromPoint(LONG X, LONG Y)
{
   HWND hWnd = 0;
   PWINDOW_OBJECT DesktopWindow = 0;
   PWINDOW_OBJECT TopWindow = 0;
   PWINDOW_OBJECT ChildWindow = 0;
   POINT pt;

   DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());

   pt.x = X;
   pt.y = Y;

   if (DesktopWindow != NULL)
   {
      /*
       * Aquire the mutex to the child window list. (All the topmost windows)
       */
      ExAcquireFastMutexUnsafe(&DesktopWindow->ChildrenListLock);

      /*
       * Spin through the windows looking for the window that contains the 
       * point. The child windows are in Z-order.
       */
      TopWindow = DesktopWindow->FirstChild;
      while (TopWindow != NULL)
      {
         if (POINT_IN_RECT(pt, TopWindow->WindowRect))
         {
            break;
         }
         TopWindow = TopWindow->NextSibling;
      }
      
      /*
       * Search for the child window that contains the point.
       */
      if (TopWindow != NULL)
      {
         ChildWindow = RestrictiveSearchChildWindows(TopWindow, pt);
         if (ChildWindow != NULL)
         {
            hWnd = ChildWindow->Self;
         }
      }
      
      /*
       * Release mutex.
       */
      ExReleaseFastMutexUnsafe(&DesktopWindow->ChildrenListLock);
   }

   IntReleaseWindowObject(DesktopWindow);
  
   return hWnd;
}

/* EOF */
