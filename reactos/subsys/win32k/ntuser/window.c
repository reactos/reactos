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
/* $Id: window.c,v 1.244.2.5 2004/09/01 22:14:50 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
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

/***********************************************************************
 *           IntSendDestroyMsg
 */
static void IntSendDestroyMsg(PWINDOW_OBJECT Window)
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
  IntSendMessage(Window, WM_DESTROY, 0, 0);

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
BOOL FASTCALL
IntDestroyWindow(PWINDOW_OBJECT Window,
                 PW32PROCESS ProcessData,
                 PW32THREAD ThreadData,
                 BOOL SendMessages)
{
  PWINDOW_OBJECT Child;
  #if 0
  PMENU_OBJECT Menu;
  #endif
  BOOL BelongsToThreadData;
  
  ASSERT(Window);
  
  IntReferenceWindowObject(Window);
  
  RemoveTimersWindow(Window);
  
  if(Window->Status & WINDOWSTATUS_DESTROYING)
  {
    IntReleaseWindowObject(Window);
    DPRINT("Tried to call IntDestroyWindow() twice\n");
    return FALSE;
  }
  Window->Status |= WINDOWSTATUS_DESTROYING;
  /* remove the window already at this point from the thread window list so we
     don't get into trouble when destroying the thread windows while we're still
     in IntDestroyWindow() */
  RemoveEntryList(&Window->ThreadListEntry);
  
  BelongsToThreadData = IntWndBelongsToThread(Window, ThreadData);
  
  if(SendMessages)
  {
    /* Send destroy messages */
    IntSendDestroyMsg(Window);
  }

  /* free child windows */
  for(Child = Window->FirstChild; Child != NULL; Child = Child->NextSibling)
  {
    /* FIXME - Do this a safer way, make sure the windows don't get unlinked
               while doing this loop and also make sure that no new windows
               get linked into the list! */
    if(!IntWndBelongsToThread(Child, ThreadData))
    {
      /* send WM_DESTROY messages to windows not belonging to the same thread */
      IntSendDestroyMsg(Child);
    }
    else
    {
      IntDestroyWindow(Child, ProcessData, ThreadData, SendMessages);
    }
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
      {
        IntSendMessage(Window, WM_NCDESTROY, 0, 0);
      }
    }

  /* remove references in the message queue */
  InterlockedCompareExchangePointer(&Window->MessageQueue->FocusWindow, NULL, Window);
  InterlockedCompareExchangePointer(&Window->MessageQueue->ActiveWindow, NULL, Window);
  InterlockedCompareExchangePointer(&Window->MessageQueue->CaptureWindow, NULL, Window);
  InterlockedCompareExchangePointer(&Window->MessageQueue->MoveSize, NULL, Window);
  InterlockedCompareExchangePointer(&Window->MessageQueue->MenuOwner, NULL, Window);

  /* reset shell window handles */
  if(ProcessData->WindowStation)
  {
    InterlockedCompareExchangePointer(&ProcessData->WindowStation->ShellWindow, NULL, Window);
    InterlockedCompareExchangePointer(&ProcessData->WindowStation->ShellListView, NULL, Window);
  }
  
  #if 0
  /* Unregister hot keys */
  UnregisterWindowHotKeys (Window);
  #endif

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
  
  /* FIXME - delete menus and system menu */
  
  DceFreeWindowDCE(Window);    /* Always do this to catch orphaned DCs */
#if 0 /* FIXME */
  WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
  CLASS_RemoveWindow(Window->Class);
#endif
  
  IntUnlinkWindow(Window);
  
  ObmDeleteObject(ProcessData->WindowStation->HandleTable, Window);
  
  IntDestroyScrollBars(Window);
  
  Window->Status |= WINDOWSTATUS_DESTROYED;
  /* don't remove the WINDOWSTATUS_DESTROYING bit */
  
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
  
  /* remove the reference to the window station that had been increased in CreateWindow */
  ObDereferenceObject(ProcessData->WindowStation);
  
  IntReleaseWindowObject(Window);

  return 0;
}

VOID FASTCALL
IntGetWindowBorderMeasures(PWINDOW_OBJECT WindowObject, INT *cx, INT *cy)
{
  if(HAS_DLGFRAME(WindowObject->Style, WindowObject->ExStyle) && !(WindowObject->Style & WS_MINIMIZE))
  {
    *cx = IntGetSystemMetrics(SM_CXDLGFRAME);
    *cy = IntGetSystemMetrics(SM_CYDLGFRAME);
  }
  else
  {
    if(HAS_THICKFRAME(WindowObject->Style, WindowObject->ExStyle)&& !(WindowObject->Style & WS_MINIMIZE))
    {
      *cx = IntGetSystemMetrics(SM_CXFRAME);
      *cy = IntGetSystemMetrics(SM_CYFRAME);
    }
    else if(HAS_THINFRAME(WindowObject->Style, WindowObject->ExStyle))
    {
      *cx = IntGetSystemMetrics(SM_CXBORDER);
      *cy = IntGetSystemMetrics(SM_CYBORDER);
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
  pwi->dwWindowStatus = (IntGetForegroundWindow() == WindowObject); /* WS_ACTIVECAPTION */
  IntGetWindowBorderMeasures(WindowObject, &pwi->cxWindowBorders, &pwi->cyWindowBorders);
  pwi->atomWindowType = (WindowObject->Class ? WindowObject->Class->Atom : 0);
  pwi->wCreatorVersion = 0x400; /* FIXME - return a real version number */
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

  ASSERT(Thread);

  Win32Thread = Thread->Win32Thread;
  Win32Process = Thread->ThreadsProcess->Win32Process;
  
  /* FIXME - acquire lock */
  
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
      
      /* FIXME - unlock */
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
    
    /* FIXME - unlock */
    
    for(pWnd = List; *pWnd; pWnd++)
    {
      IntDestroyWindow(*pWnd, Win32Process, Win32Thread, FALSE /* FIXME - send messages? */);
      /* FIXME - what happens here if the application calls ExitThread()/ExitProcess() ??? */
      IntReleaseWindowObject(*pWnd);
    }
    ExFreePool(List);
    return;
  }
  else
  {
    /* FIXME - unlock */
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
  Rect->bottom = WindowObject->ClientRect.bottom - WindowObject->ClientRect.top;
}


BOOL FASTCALL
IntIsChildWindow(PWINDOW_OBJECT Parent, PWINDOW_OBJECT Child)
{
  PWINDOW_OBJECT Window;
  
  Window = Child;
  while (Window)
  {
    if (Window == Parent)
    {
      /* FIXME - really return TRUE?! */
      return TRUE;
    }
    if(!(Window->Style & WS_CHILD))
    {
      break;
    }
    
    Window = IntGetParentObject(Window);
  }
  
  return(FALSE);  
}

BOOL FASTCALL
IntIsWindowVisible(PWINDOW_OBJECT Wnd)
{
  PWINDOW_OBJECT Window;
  
  ASSERT(Wnd);
  
  Window = Wnd;
  while(Window)
  {
    if(!(Window->Style & WS_VISIBLE))
    {
      return FALSE;
    }
    if(!(Window->Style & WS_CHILD))
    {
      break;
    }
    
    Window = IntGetParentObject(Window);
  }
  
  return (Window && (Window->Style & WS_VISIBLE));
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
    {
      Wnd->NextSibling->PrevSibling = Wnd;
    }
    else if ((Parent = IntGetParentObject(Wnd)))
    {
      if(Parent->LastChild == WndPrevSibling)
      { 
        Parent->LastChild = Wnd;
      }
    }
    Wnd->PrevSibling->NextSibling = Wnd;
  }
  else
  {
    /* link at top */
    Parent = IntGetParentObject(Wnd);
    if ((Wnd->NextSibling = WndParent->FirstChild))
    { 
      Wnd->NextSibling->PrevSibling = Wnd;
    }
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

PWINDOW_OBJECT FASTCALL
IntSetOwner(PWINDOW_OBJECT Window, PWINDOW_OBJECT NewOwner)
{
  ASSERT(Window);
  return (PWINDOW_OBJECT)InterlockedExchange((LONG*)&Window->Owner, (LONG)NewOwner);
}

PWINDOW_OBJECT FASTCALL
IntSetParent(PWINDOW_OBJECT Window, PWINDOW_OBJECT WndNewParent)
{
   PWINDOW_OBJECT WndOldParent, Sibling, InsertAfter;
   BOOL WasVisible;
   #if 0
   BOOL MenuChanged;
   #endif
   
   ASSERT(Window);
   ASSERT(WndNewParent);
   
   /*
    * Windows hides the window first, then shows it again
    * including the WM_SHOWWINDOW messages and all
    */
   WasVisible = WinPosShowWindow(Window, SW_HIDE);

   /* Validate that window and parent still exist */
   if (!IntIsWindow(Window) || !IntIsWindow(WndNewParent))
   {
      return NULL;
   }

   /* Window must belong to current process */
   if (!IntWndBelongsToThread(Window, PsGetWin32Thread()))
   {
      return NULL;
   }

   WndOldParent = IntGetParentObject(Window);

   if (WndNewParent != WndOldParent)
   {
      IntUnlinkWindow(Window);
      InsertAfter = NULL;
      if (0 == (Window->ExStyle & WS_EX_TOPMOST))
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
        IntLinkWindow(Window, WndNewParent, InsertAfter /*prev sibling*/);
      }
      else
      {
        IntLinkWindow(Window, WndNewParent, InsertAfter /*prev sibling*/);
      }

      if (WndNewParent != IntGetDesktopWindow()) /* a child window */
      {
         if (!(Window->Style & WS_CHILD))
         {
            //if ( Wnd->Menu ) DestroyMenu ( Wnd->menu );
            #if 0
	    IntSetMenu(Window, NULL, &MenuChanged);
	    #endif
         }
      }
   }
   
   /*
    * SetParent additionally needs to make hwnd the top window
    * in the z-order and send the expected WM_WINDOWPOSCHANGING and
    * WM_WINDOWPOSCHANGED notification messages.
    */
   WinPosSetWindowPos(Window, (0 == (Window->ExStyle & WS_EX_TOPMOST) ? WINDOW_TOP : WINDOW_TOPMOST),
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
   if(WndOldParent && IntIsWindow(WndOldParent->Handle))
   {
     return WndOldParent;
   }
   
   return NULL;
}


/* unlink the window from siblings and parent. children are kept in place. */
VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd)
{
  PWINDOW_OBJECT WndParent;
  
  WndParent = IntGetParentObject(Wnd);
  
  if (Wnd->NextSibling) Wnd->NextSibling->PrevSibling = Wnd->PrevSibling;
  else if (WndParent && WndParent->LastChild == Wnd) WndParent->LastChild = Wnd->PrevSibling;
 
  if (Wnd->PrevSibling) Wnd->PrevSibling->NextSibling = Wnd->NextSibling;
  else if (WndParent && WndParent->FirstChild == Wnd) WndParent->FirstChild = Wnd->NextSibling;
  
  Wnd->PrevSibling = Wnd->NextSibling = Wnd->Parent = NULL;
}

BOOL FASTCALL
IntAnyPopup(VOID)
{
  PWINDOW_OBJECT Window, Child;
  
  if(!(Window = IntGetDesktopWindow()))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  for(Child = Window->FirstChild; Child; Child = Child->NextSibling)
  {
    if(Child->Owner && Child->Style & WS_VISIBLE)
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
      Pos.x = Parent->TiledCounter * (IntGetSystemMetrics(SM_CXSIZE) + IntGetSystemMetrics(SM_CXFRAME));
      Pos.y = Parent->TiledCounter * (IntGetSystemMetrics(SM_CYSIZE) + IntGetSystemMetrics(SM_CYFRAME));
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


PWINDOW_OBJECT FASTCALL
IntCreateWindow(DWORD dwExStyle,
		PUNICODE_STRING ClassName,
		PUNICODE_STRING WindowName,
		ULONG dwStyle,
		LONG x,
		LONG y,
		LONG nWidth,
		LONG nHeight,
		PWINDOW_OBJECT Parent,
		PMENU_OBJECT Menu,
		ULONG WindowID,
		HINSTANCE hInstance,
		LPVOID lpParam,
		DWORD dwShowMode,
		BOOL bUnicodeWindow)
{
  PWINSTATION_OBJECT WinStaObject;
  PCLASS_OBJECT ClassObject;
  PWINDOW_OBJECT WindowObject, ParentWindow, OwnerWindow;
  NTSTATUS Status;
  HANDLE Handle;
  POINT Pos;
  SIZE Size;
  POINT MaxPos;
  CREATESTRUCTW Cs;
  CBT_CREATEWNDW CbtCreate;
  LRESULT Result;
  #if 0
  BOOL MenuChanged;
  #endif
  
  ParentWindow = IntGetDesktopWindow();
  OwnerWindow = NULL;
  
  /* Check the window station. */
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
					  KernelMode,
					  0,
					  &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
	   PROCESS_WINDOW_STATION());
    return NULL;
  }
  
  if (Parent)
  {
    if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
    {
      ParentWindow = Parent;
    }
    else
    {
      OwnerWindow = IntGetAncestor(Parent, GA_ROOT);
    }
  }
  else if ((dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD)
  {
    return NULL;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
  }
  
  /* Lookup and reference the class object */
  if(!IntReferenceClassByNameOrAtom(&ClassObject, ClassName, hInstance))
  {
    DbgPrint("Failed to reference class: 0x%x, %S\n", (ClassName ? ClassName->Buffer : NULL), (ClassName ? ClassName->Buffer : L"?!"));
    ObDereferenceObject(WinStaObject);
    return NULL;
  }
  
  /* Create the window object. */
  WindowObject = (PWINDOW_OBJECT)ObmCreateObject(WinStaObject->HandleTable,
                                                 &Handle,
                                                 otWINDOW,
                                                 sizeof(WINDOW_OBJECT) + ClassObject->cbWndExtra);
  DbgPrint("ObmCreateObject returned 0x%x\n", WindowObject);
  if(WindowObject == NULL)
  {
    DbgPrint("fail:1\n");
    ClassDereferenceObject(ClassObject);
    DbgPrint("fail:2\n");
    ObDereferenceObject(WinStaObject);
    DbgPrint("fail:3\n");
    SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
    return NULL;
  }
  DPRINT1("Created object with handle %X on Desktop 0x%x\n", Handle, PsGetWin32Thread()->Desktop);
  
  /* If there is no desktop window yet, we must be creating it */
  InterlockedCompareExchange((LONG*)&PsGetWin32Thread()->Desktop->DesktopWindow, (LONG)WindowObject, 0);

  /*
   * Fill out the structure describing it.
   */
  WindowObject->Class = ClassObject;
  InsertTailList(&ClassObject->ClassWindowsListHead, &WindowObject->ClassListEntry);
  
  WindowObject->ExStyle = dwExStyle;
  WindowObject->Style = dwStyle & ~WS_VISIBLE;
  DPRINT("1: Style is now %lx\n", WindowObject->Style);
  
  WindowObject->ContextHelpId = 0;
  WindowObject->WindowID = WindowID;
  WindowObject->Instance = hInstance;
  WindowObject->Handle = Handle;
  
  InitializeListHead(&WindowObject->PropListHead);
  
  WindowObject->MessageQueue = PsGetWin32Thread()->MessageQueue;
  IntReferenceMessageQueue(WindowObject->MessageQueue);
  
  if (!(dwStyle & WS_CHILD))
  {
    #if 0
    IntSetMenu(WindowObject, Menu, &MenuChanged);
    #endif
  }
  
  WindowObject->Parent = ParentWindow;
  WindowObject->Owner = OwnerWindow;
  WindowObject->FirstChild = NULL;
  WindowObject->LastChild = NULL;
  WindowObject->PrevSibling = NULL;
  WindowObject->NextSibling = NULL;
  
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

  if (NULL != WindowName->Buffer)
  {  
    WindowObject->WindowName.MaximumLength = WindowName->MaximumLength;
    WindowObject->WindowName.Length = WindowName->Length;
    WindowObject->WindowName.Buffer = ExAllocatePoolWithTag(PagedPool, WindowName->MaximumLength,
                                                            TAG_STRING);
    if (NULL == WindowObject->WindowName.Buffer)
    { 
      RtlInitUnicodeString(&WindowObject->WindowName, NULL);
      DPRINT1("Failed to allocate mem for window name\n");
    }
    else
    {
      RtlCopyMemory(WindowObject->WindowName.Buffer, WindowName->Buffer, WindowName->MaximumLength);
    }
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
  #if 0
  if((WindowObject->Style & WS_SYSMENU) && (WindowObject->Style & WS_CAPTION))
  {
    SystemMenu = IntGetSystemMenu(WindowObject, TRUE, TRUE);
    if(SystemMenu)
    {
      WindowObject->SystemMenu = SystemMenu->MenuInfo.Self;
      IntReleaseMenuObject(SystemMenu);
    }
  }
  #endif
  
  /* Insert the window into the thread's window list. */
  InsertTailList (&PsGetWin32Thread()->WindowListHead, 
		  &WindowObject->ThreadListEntry);

  /* Allocate a DCE for this window. */
  if (dwStyle & CS_OWNDC)
  {
    WindowObject->Dce = DceAllocDCE(WindowObject, DCE_WINDOW_DC);
  }
  /* FIXME:  Handle "CS_CLASSDC" */

  Pos.x = x;
  Pos.y = y;
  Size.cx = nWidth;
  Size.cy = nHeight;

  /* call hook */
  Cs.lpCreateParams = lpParam;
  Cs.hInstance = hInstance;
  Cs.hMenu = 0; /* FIXME */
  Cs.hwndParent = (Parent ? Parent->Handle : NULL);
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
  #if 0
  if (HOOK_CallHooks(WH_CBT, HCBT_CREATEWND, (WPARAM) Handle, (LPARAM) &CbtCreate))
  #else
  if (FALSE)
  #endif
    {
      
      
      /* FIXME - Delete window object and remove it from the thread windows list */
      /* FIXME - delete allocated DCE */
      
      RemoveEntryList(&WindowObject->ClassListEntry);
      ClassDereferenceObject(ClassObject);
      /* FIXME - remove the reference from the window station */
      DPRINT1("CBT-hook returned !0\n");
      return NULL;
    }
  
  /* FIXME - make sure the window still exists! */
  
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
    
    IntGetDesktopWorkArea(WindowObject->MessageQueue->Thread->Win32Thread->Desktop, &WorkArea);
    
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
  if ((WindowObject->Style & WS_CHILD) && ParentWindow)
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
      
      /* FIXME - make sure the window still exists! */
      
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
  if ((WindowObject->Style & WS_CHILD) && ParentWindow)
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

  DPRINT1("[win32k.window] IntCreateWindow style %d, exstyle %d, parent %d\n", Cs.style, Cs.dwExStyle, Cs.hwndParent);
  DPRINT1("IntCreateWindow(): (%d,%d-%d,%d)\n", x, y, nWidth, nHeight);
  DPRINT1("IntCreateWindow(): About to send NCCREATE message.\n");
  Result = IntSendMessage(WindowObject, WM_NCCREATE, 0, (LPARAM) &Cs);
  
  /* FIXME - make sure the window still exists! */
  
  if (!Result)
    {
      /* FIXME - Cleanup. */
      RemoveEntryList(&WindowObject->ClassListEntry);
      ClassDereferenceObject(ClassObject);
      /* FIXME - remove the reference to the window station */
      DPRINT("IntCreateWindow(): NCCREATE message failed.\n");
      return NULL;
    }
 
  /* Calculate the non-client size. */
  MaxPos.x = WindowObject->WindowRect.left;
  MaxPos.y = WindowObject->WindowRect.top;
  DPRINT("IntCreateWindow(): About to get non-client size.\n");
  /* WinPosGetNonClientSize SENDS THE WM_NCCALCSIZE message */
  Result = WinPosGetNonClientSize(WindowObject, 
				  &WindowObject->WindowRect,
				  &WindowObject->ClientRect);
  /* FIXME - make sure the window still exists! */
  
  NtGdiOffsetRect(&WindowObject->WindowRect, 
		 MaxPos.x - WindowObject->WindowRect.left,
		 MaxPos.y - WindowObject->WindowRect.top);

  if (NULL != ParentWindow)
  {
    /* link the window into the parent's child list */
    if ((dwStyle & (WS_CHILD | WS_MAXIMIZE)) == WS_CHILD)
    {
      /* link window as bottom sibling */
      IntLinkWindow(WindowObject, ParentWindow, ParentWindow->LastChild);
    }
    else
    {
      /* link window as top sibling (but after topmost siblings) */
      PWINDOW_OBJECT Sibling, InsertAfter = NULL;
      
      if (!(dwExStyle & WS_EX_TOPMOST))
      {
        Sibling = ParentWindow->FirstChild;
        while (NULL != Sibling && (Sibling->ExStyle & WS_EX_TOPMOST))
        {
          InsertAfter = Sibling;
          Sibling = Sibling->NextSibling;
        }
      }
      
      IntLinkWindow(WindowObject, ParentWindow, InsertAfter /* prev sibling */);
    }
  }

  /* Send the WM_CREATE message. */
  DPRINT("IntCreateWindow(): about to send CREATE message.\n");
  Result = IntSendMessage(WindowObject, WM_CREATE, 0, (LPARAM) &Cs);
  /* FIXME - make sure the window still exists! */
  if (Result == (LRESULT)-1)
  {
    /* FIXME: Cleanup. */
    RemoveEntryList(&WindowObject->ClassListEntry);
    ClassDereferenceObject(ClassObject);
    /* FIXME - remove the reference to the window station */
    DPRINT("IntCreateWindow(): send CREATE message failed.\n");
    return NULL;
  } 
  
  /* Send move and size messages. */
  if (!(WindowObject->Flags & WINDOWOBJECT_NEED_SIZE))
  {
    LONG lParam;
    
    DPRINT("IntCreateWindow(): About to send WM_SIZE\n");

    #ifdef DBG
    if ((WindowObject->ClientRect.right - WindowObject->ClientRect.left) < 0 ||
        (WindowObject->ClientRect.bottom - WindowObject->ClientRect.top) < 0)
    {
      DPRINT("Sending bogus WM_SIZE\n");
    }
    #endif
     
    lParam = MAKE_LONG(WindowObject->ClientRect.right - 
		WindowObject->ClientRect.left,
		WindowObject->ClientRect.bottom - 
		WindowObject->ClientRect.top);
    IntSendMessage(WindowObject, WM_SIZE, SIZE_RESTORED, lParam);
    /* FIXME - make sure the window still exists! */
    
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
    
    IntSendMessage(WindowObject, WM_MOVE, 0, lParam);
    /* FIXME - make sure the window still exists! */
  }

  /* Show or maybe minimize or maximize the window. */
  if (WindowObject->Style & (WS_MINIMIZE | WS_MAXIMIZE))
  {
    RECT NewPos;
    UINT16 SwFlag;

    SwFlag = (WindowObject->Style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;
    
    WinPosMinMaximize(WindowObject, SwFlag, &NewPos);
    /* FIXME - make sure the window still exists! */
    
    SwFlag = ((WindowObject->Style & WS_CHILD) || IntGetActiveWindow()) ?
              SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED :
              SWP_NOZORDER | SWP_FRAMECHANGED;
    DPRINT("IntCreateWindow(): About to minimize/maximize\n");
    DPRINT("%d,%d %dx%d\n", NewPos.left, NewPos.top, NewPos.right, NewPos.bottom);
    
    WinPosSetWindowPos(WindowObject, 0, NewPos.left, NewPos.top,
                       NewPos.right, NewPos.bottom, SwFlag);
    /* FIXME - make sure the window still exists! */
  }

  /* Notify the parent window of a new child. */
  if ((WindowObject->Style & WS_CHILD) &&
      (!(WindowObject->ExStyle & WS_EX_NOPARENTNOTIFY)) && ParentWindow)
  {
    /* FIXME - shouldn't we check if the parent belongs to our process, too? */
    
    DPRINT("IntCreateWindow(): About to notify parent\n");
    IntSendMessage(ParentWindow,
                   WM_PARENTNOTIFY, 
                   MAKEWPARAM(WM_CREATE, WindowObject->WindowID),
                   (LPARAM)WindowObject->Handle);
    
    /* FIXME - make sure the window still exists! */
  }
  
  /* Initialize and show the window's scrollbars */
  if (WindowObject->Style & WS_VSCROLL)
  {
    IntShowScrollBar(WindowObject, SB_VERT, (dwStyle & WS_VISIBLE) != 0);
  }
  if (WindowObject->Style & WS_HSCROLL)
  {
    IntShowScrollBar(WindowObject, SB_HORZ, (dwStyle & WS_VISIBLE) != 0);
  }
  
  if (dwStyle & WS_VISIBLE)
  {
    DPRINT("IntCreateWindow(): About to show window\n");
    WinPosShowWindow(WindowObject, dwShowMode);
    /* FIXME - make sure the window still exists! */
  }

  DPRINT("IntCreateWindow(): = %X\n", Handle);
  #if 0
  DPRINT("WindowObject->SystemMenu = 0x%x\n", WindowObject->SystemMenu);
  #endif
  return WindowObject;
}

PWINDOW_OBJECT FASTCALL
IntFindWindow(PWINDOW_OBJECT Parent,
              PWINDOW_OBJECT ChildAfter,
              PCLASS_OBJECT ClassObject,
              PUNICODE_STRING WindowName)
{
  BOOL CheckWindowName;
  PWINDOW_OBJECT Current, Ret;
  
  ASSERT(Parent);
  
  CheckWindowName = (WindowName && (WindowName->Length > 0));
  
  Current = Parent->FirstChild;
  
  if(ChildAfter)
  {
    while(Current && ((Current = Current->NextSibling) != ChildAfter));
  }
  
  while(Current)
  {
    /* Do not send WM_GETTEXT messages in the kernel mode version!
       The user mode version however calls GetWindowText() which will
       send WM_GETTEXT messages to windows belonging to its processes */
    if(((!CheckWindowName || (CheckWindowName && !RtlCompareUnicodeString(WindowName, &(Current->WindowName), FALSE))) &&
        (!ClassObject || (ClassObject && (Current->Class == ClassObject))))
       ||
       ((!CheckWindowName || (CheckWindowName && !RtlCompareUnicodeString(WindowName, &(Current->WindowName), FALSE))) &&
        (!ClassObject || (ClassObject && (Current->Class == ClassObject)))))
    {
      Ret = Current;
      break;
    }
    
    Current = Current->NextSibling;
  }
  
  return Ret;
}


PWINDOW_OBJECT FASTCALL
IntGetAncestor(PWINDOW_OBJECT Window, UINT Type)
{
   PWINDOW_OBJECT WndAncestor, Parent;

   if (Window == IntGetDesktopWindow())
   {
      return NULL;
   }

   switch (Type)
   {
      case GA_PARENT:
      {
         return IntGetParentObject(Window);
      }

      case GA_ROOT:
      {
         WndAncestor = Window;

         Parent = IntGetParentObject(WndAncestor);
         while(Parent != NULL && !IntIsDesktopWindow(Parent))
         {
           WndAncestor = Parent;
	   Parent = IntGetParentObject(WndAncestor);
         }
         return WndAncestor;
      }
    
      case GA_ROOTOWNER:
      {
         WndAncestor = Window;
         while((Parent = IntGetParent(WndAncestor)))
         {
            WndAncestor = Parent;
         }
         return WndAncestor;
      }
   }

   return NULL;
}


PWINDOW_OBJECT FASTCALL
IntGetShellWindow(VOID)
{
  PWINSTATION_OBJECT WinStaObject;
  PWINDOW_OBJECT Ret;
  
  if (!(WinStaObject = PsGetWin32Process()->WindowStation))
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return NULL;
  }
  
  Ret = WinStaObject->ShellWindow;
  
  return Ret;
}


BOOL FASTCALL
IntSetShellWindowEx(PWINDOW_OBJECT Shell, PWINDOW_OBJECT ListView)
{
  PWINSTATION_OBJECT WinStaObject;
  
  ASSERT(Shell);
  
  if (!(WinStaObject = PsGetWin32Process()->WindowStation))
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
   /*
    * Test if we are permitted to change the shell window.
    */
   if (WinStaObject->ShellWindow)
   {
      return FALSE;
   }

   /*
    * Move shell window into background.
    */
   if (ListView && ListView != Shell)
   {
/*
 * Disabled for now to get Explorer working.
 * -- Filip, 01/nov/2003
 */
#if 0
       WinPosSetWindowPos(ListView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
       /* FIXME - make sure the window still exists! */
#endif

      if (ListView->ExStyle & WS_EX_TOPMOST)
      {
         return FALSE;
      }
   }

   if (Shell->ExStyle & WS_EX_TOPMOST)
   {
      return FALSE;
   }

   WinPosSetWindowPos(Shell, WINDOW_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
   /* FIXME - make sure the window still exists! */

   InterlockedExchange((LONG*)&WinStaObject->ShellWindow, (LONG)Shell);
   InterlockedExchange((LONG*)&WinStaObject->ShellListView, (LONG)ListView);
   
   return TRUE;
}


PWINDOW_OBJECT FASTCALL
IntGetRelatedWindow(PWINDOW_OBJECT WindowObject, UINT Relationship)
{
   PWINDOW_OBJECT Parent;

   ASSERT(WindowObject);
  
   switch (Relationship)
   {
      case GW_HWNDFIRST:
         if((Parent = IntGetParentObject(WindowObject)))
         {
           return Parent->FirstChild;
         }
         break;

      case GW_HWNDLAST:
         if((Parent = IntGetParentObject(WindowObject)))
         {
           return Parent->LastChild;
         }
         break;

      case GW_HWNDNEXT:
         return WindowObject->NextSibling;

      case GW_HWNDPREV:
         return WindowObject->PrevSibling;

      case GW_OWNER:
         return WindowObject->Owner;

      case GW_CHILD:
         return WindowObject->FirstChild;
   }

   return NULL;
}


LONG FASTCALL
IntGetWindowLong(PWINDOW_OBJECT WindowObject, INT Index, BOOL Ansi)
{
   LONG Result = 0;

   ASSERT(WindowObject);

   DPRINT("IntGetWindowLong(0x%x, %d, %d)\n", WindowObject->Handle, (INT)Index, Ansi);

   /*
    * Only allow CSRSS to mess with the desktop window
    */
   if (WindowObject == IntGetDesktopWindow()
       && !IntWndBelongsToThread(WindowObject, PsGetWin32Thread()))
   {
      SetLastWin32Error(STATUS_ACCESS_DENIED);
      return 0;
   }

   if (Index >= 0)
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
            if(WindowObject->Parent != NULL && WindowObject->Parent == IntGetDesktopWindow())
            {
              Result = (LONG) IntGetRelatedWindow(WindowObject, GW_OWNER);
            }
            else
            {
              Result = (LONG) WindowObject->Parent;
            }
            break;

         case GWL_ID:
            /* FIXME - what about the menu handle? */
            Result = (LONG) WindowObject->WindowID;
            break;

         case GWL_USERDATA:
            Result = WindowObject->UserData;
            break;
    
         default:
            DPRINT1("IntGetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            Result = 0;
            break;
      }
   }

   return Result;
}


LONG FASTCALL
IntSetWindowLong(PWINDOW_OBJECT WindowObject, INT Index, LONG NewValue, BOOL Ansi)
{
   PWINDOW_OBJECT Parent;
   PW32PROCESS Process;
   PWINSTATION_OBJECT WindowStation;
   LONG OldValue;
   STYLESTRUCT Style;

   ASSERT(WindowObject);

   if (WindowObject == IntGetDesktopWindow())
   {
      SetLastWin32Error(STATUS_ACCESS_DENIED);
      return 0;
   }

   if (Index >= 0)
   {
      if ((Index + sizeof(LONG)) > WindowObject->ExtraDataSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
            Process = WindowObject->MessageQueue->Thread->ThreadsProcess->Win32Process;
            WindowStation = Process->WindowStation;
            if(WindowStation)
            {
              if (WindowObject == WindowStation->ShellWindow || WindowObject == WindowStation->ShellListView)
                 Style.styleNew &= ~WS_EX_TOPMOST;
            }

            IntSendMessage(WindowObject, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM) &Style);
            /* FIXME - make sure the window still exists! */
            InterlockedExchange(&WindowObject->ExStyle, (LONG)Style.styleNew);
            IntSendMessage(WindowObject, WM_STYLECHANGED, GWL_EXSTYLE, (LPARAM) &Style);
            /* FIXME - make sure the window still exists! */
            break;

         case GWL_STYLE:
            OldValue = (LONG) WindowObject->Style;
            Style.styleOld = OldValue;
            Style.styleNew = NewValue;
            IntSendMessage(WindowObject, WM_STYLECHANGING, GWL_STYLE, (LPARAM) &Style);
            /* FIXME - make sure the window still exists! */
            InterlockedExchange(&WindowObject->Style, (LONG)Style.styleNew);
            IntSendMessage(WindowObject, WM_STYLECHANGED, GWL_STYLE, (LPARAM) &Style);
            /* FIXME - make sure the window still exists! */
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
            OldValue = (LONG) InterlockedExchange((LONG*)&WindowObject->Instance, (LONG)NewValue);
            break;

         case GWL_HWNDPARENT:
            Parent = IntGetParentObject(WindowObject);
            if (Parent && (Parent == IntGetDesktopWindow()))
               OldValue = (LONG) IntSetOwner(WindowObject, (PWINDOW_OBJECT)NewValue);
            else
               OldValue = (LONG) IntSetParent(WindowObject, (PWINDOW_OBJECT)NewValue);
            /* FIXME - make sure the window still exists! */
            break;

         case GWL_ID:
            /* FIXME - what about the menu handle? */
            OldValue = (LONG) InterlockedExchange((LONG*)&WindowObject->WindowID, (LONG)NewValue);
            break;

         case GWL_USERDATA:
            OldValue = (LONG) InterlockedExchange((LONG*)&WindowObject->UserData, (LONG)NewValue);
            break;
    
         default:
            DPRINT1("IntSetWindowLong(): Unsupported index %d\n", Index);
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            OldValue = 0;
            break;
      }
   }
 
   return OldValue;
}

WORD FASTCALL
IntSetWindowWord(PWINDOW_OBJECT WindowObject, INT Index, WORD NewValue)
{
   WORD OldValue;

   ASSERT(WindowObject);

   switch (Index)
   {
      case GWL_ID:
      case GWL_HINSTANCE:
      case GWL_HWNDPARENT:
         return IntSetWindowLong(WindowObject, Index, (UINT)NewValue, TRUE);
      default:
         if (Index < 0)
         {
            SetLastWin32Error(ERROR_INVALID_INDEX);
            return 0;
         }
         break;
   }

   if (Index > WindowObject->ExtraDataSize - sizeof(WORD))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   OldValue = *((WORD *)(WindowObject->ExtraData + Index));
   *((WORD *)(WindowObject->ExtraData + Index)) = NewValue;

   return OldValue;
}


BOOL FASTCALL
IntGetWindowPlacement(PWINDOW_OBJECT WindowObject,
		      WINDOWPLACEMENT *lpwndpl)
{
  PINTERNALPOS InternalPos;
  POINT Size;
  
  ASSERT(WindowObject);
  
  lpwndpl->length = sizeof(WINDOWPLACEMENT);
  lpwndpl->flags = 0;
  lpwndpl->showCmd = ((WindowObject->Flags & WINDOWOBJECT_RESTOREMAX) ? SW_MAXIMIZE : SW_SHOWNORMAL);
  
  Size.x = WindowObject->WindowRect.left;
  Size.y = WindowObject->WindowRect.top;
  InternalPos = WinPosInitInternalPos(WindowObject, &Size, 
				      &WindowObject->WindowRect);
  if (!InternalPos)
  {
    return FALSE;
  }
  
  lpwndpl->rcNormalPosition = InternalPos->NormalRect;
  lpwndpl->ptMinPosition = InternalPos->IconPos;
  lpwndpl->ptMaxPosition = InternalPos->MaxPos;
  
  return TRUE;
}


ULONG FASTCALL
IntGetWindowThreadProcessId(PWINDOW_OBJECT Window, ULONG *Pid)
{
   ASSERT(Window);
   
   if(Pid)
   {
     *Pid = (ULONG)IntGetWndProcessId(Window);
   }
   
   return (ULONG)IntGetWndThreadId(Window);
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
DWORD FASTCALL
IntQueryWindow(PWINDOW_OBJECT Window, DWORD Index)
{
   DWORD Result;

   ASSERT(Window);

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

   return Result;
}


UINT FASTCALL
IntRegisterWindowMessage(PUNICODE_STRING MessageName)
{
  /* WARNING !!! The Unicode string is supposed to be NULL-Terminated! */
  
  ASSERT(MessageName);
  return (UINT)IntAddAtom(MessageName->Buffer);
}


BOOL FASTCALL
IntSetWindowPlacement(PWINDOW_OBJECT WindowObject,
		      WINDOWPLACEMENT *lpwndpl)
{
  ASSERT(WindowObject);
  ASSERT(lpwndpl);
  
  if ((WindowObject->Style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
  {
     WinPosSetWindowPos(WindowObject, NULL,
        lpwndpl->rcNormalPosition.left, lpwndpl->rcNormalPosition.top,
        lpwndpl->rcNormalPosition.right - lpwndpl->rcNormalPosition.left,
        lpwndpl->rcNormalPosition.bottom - lpwndpl->rcNormalPosition.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
  }
  
  /* FIXME - change window status */
  WinPosShowWindow(WindowObject, lpwndpl->showCmd);
  /* FIXME - make sure the window still exists! */

  if (WindowObject->InternalPos == NULL)
  {
     if(!(WindowObject->InternalPos = ExAllocatePoolWithTag(PagedPool, sizeof(INTERNALPOS), TAG_WININTLIST)))
     {
       DPRINT1("Unable to allocate a INTERNALPOS structure for window 0x%x\n", WindowObject->Handle);
       return FALSE;
     }
  }
  
  WindowObject->InternalPos->NormalRect = lpwndpl->rcNormalPosition;
  WindowObject->InternalPos->IconPos = lpwndpl->ptMinPosition;
  WindowObject->InternalPos->MaxPos = lpwndpl->ptMaxPosition;
  
  return TRUE;
}


INT FASTCALL
IntGetWindowRgn(PWINDOW_OBJECT WindowObject, HRGN hRgn)
{
  INT Ret;
  HRGN VisRgn;
  ROSRGNDATA *pRgn;
  
  ASSERT(WindowObject);
  
  if(!hRgn)
  {
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
  
  return Ret;
}

INT FASTCALL
IntGetWindowRgnBox(PWINDOW_OBJECT WindowObject, RECT *Rect)
{
  INT Ret;
  HRGN VisRgn;
  ROSRGNDATA *pRgn;
  
  ASSERT(WindowObject);
  
  if(!Rect)
  {
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
  
  return Ret;
}


INT FASTCALL
IntSetWindowRgn(
  PWINDOW_OBJECT WindowObject,
  HRGN hRgn,
  BOOL bRedraw)
{
  HRGN OldRgn;
  
  ASSERT(WindowObject);
  
  /* FIXME - Verify if hRgn is a valid handle!!!! */
  
  if((OldRgn = (HRGN)InterlockedExchange((LONG*)&WindowObject->WindowRegion, (LONG)hRgn)))
  {
    /* Delete no longer needed region handle */
    NtGdiDeleteObject(OldRgn);
  }
  
  /* FIXME - send WM_WINDOWPOSCHANGING and WM_WINDOWPOSCHANGED messages to the window */
  
  if(bRedraw)
  {
    IntRedrawWindow(WindowObject, NULL, NULL, RDW_INVALIDATE);
  }
  
  return (INT)hRgn;
}


PWINDOW_OBJECT FASTCALL
IntWindowFromPoint(LONG X, LONG Y)
{
   POINT pt;
   PWINDOW_OBJECT DesktopWindow;

   if ((DesktopWindow = IntGetDesktopWindow()))
   {
      PWINDOW_OBJECT Window = NULL;
      
      pt.x = X;
      pt.y = Y;
      
      WinPosWindowFromPoint(DesktopWindow, PsGetWin32Thread()->MessageQueue, &pt, &Window);
      
      return Window;
   }
  
   return NULL;
}

PWINDOW_OBJECT FASTCALL
IntGetWindow(PWINDOW_OBJECT Window, UINT uCmd)
{
  ASSERT(Window);
  
  switch(uCmd)
  {
    case GW_HWNDFIRST:
      return (Window->Parent != NULL ? Window->Parent->FirstChild : NULL);
    
    case GW_HWNDLAST:
      return (Window->Parent != NULL ? Window->Parent->LastChild : NULL);
    
    case GW_HWNDNEXT:
      return Window->NextSibling;
    
    case GW_HWNDPREV:
      return Window->PrevSibling;
    
    case GW_OWNER:
      return Window->Owner;
    
    case GW_CHILD:
      return Window->FirstChild;
  }
  
  SetLastWin32Error(ERROR_INVALID_GW_COMMAND);
  DPRINT1("GetWindow(): Invalid command 0x%x\n", uCmd);
  return NULL;
}


BOOL FASTCALL
IntDefSetText(PWINDOW_OBJECT WindowObject, PUNICODE_STRING WindowText)
{
  /* WARNING - do not use or free the WindowText after this call ! */
  ASSERT(WindowObject);
  
  RtlFreeUnicodeString(&WindowObject->WindowName);
  WindowObject->WindowName = *WindowText;
  WindowObject->WindowName.Buffer = ExAllocatePoolWithTag(PagedPool, WindowObject->WindowName.Length, TAG_STRING);
  if(WindowObject->WindowName.Buffer != NULL)
  {
    RtlCopyUnicodeString(&WindowObject->WindowName, WindowText);
  }
  else
  {
    DPRINT1("Failed to allocate enough memory for the window text!\n");
    RtlZeroMemory(&WindowObject->WindowName, sizeof(UNICODE_STRING));
  }
  
  return FALSE;
}

INT FASTCALL
IntInternalGetWindowText(PWINDOW_OBJECT WindowObject, WCHAR *Buffer, INT nMaxCount)
{
  INT Result;
  
  ASSERT(WindowObject);
  ASSERT(Buffer && (nMaxCount <= 1));
  
  Result = WindowObject->WindowName.Length / sizeof(WCHAR);
  if(Buffer != NULL)
  {
    INT Copy;
    
    Copy = min(nMaxCount - 1, Result);
    if(Copy > 0)
    {
      RtlCopyMemory(Buffer, WindowObject->WindowName.Buffer, Copy * sizeof(WCHAR));
      Buffer += Copy;
    }
    *Buffer = L'\0';
    
    return Copy;
  }
  
  return Result;
}

/* WINPROC ********************************************************************/

BOOL FASTCALL
IntDereferenceWndProcHandle(WNDPROC wpHandle, WndProcHandle *Data)
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

DWORD FASTCALL
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

DWORD FASTCALL
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

DWORD FASTCALL
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
