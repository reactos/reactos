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
/* $Id: window.c,v 1.98 2003/08/21 15:26:19 weiden Exp $
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


/* FUNCTIONS *****************************************************************/

/* check if hwnd is a broadcast magic handle */
inline BOOL IntIsBroadcastHwnd( HWND hwnd )
{
    return (hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST);
}


inline BOOL IntIsDesktopWindow(PWINDOW_OBJECT Wnd)
{
  return Wnd->Parent == NULL;
}


PWINDOW_OBJECT FASTCALL
IntGetAncestor(PWINDOW_OBJECT Wnd, UINT Type)
{
  if (IntIsDesktopWindow(Wnd)) return NULL;

  switch (Type)
  {
    case GA_PARENT:
      return Wnd->Parent;

    case GA_ROOT:
      while(!IntIsDesktopWindow(Wnd->Parent))
      {
        Wnd = Wnd->Parent;
      }
      return Wnd;
    
    case GA_ROOTOWNER:
      while ((Wnd = IntGetParent(Wnd)));
      return Wnd;
  }

  return NULL;
}


HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Type)
{
  PWINDOW_OBJECT Wnd, WndAncestor;
  HWND hWndAncestor = NULL;

  IntAcquireWinLockShared();

  if (!(Wnd = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }

  WndAncestor = IntGetAncestor(Wnd, Type);
  if (WndAncestor) hWndAncestor = WndAncestor->Self;

  IntReleaseWinLock();

  return hWndAncestor;
}

PWINDOW_OBJECT FASTCALL
IntGetParent(PWINDOW_OBJECT Wnd)
{
  if (Wnd->Style & WS_POPUP)
  {
    return IntGetWindowObject(Wnd->ParentHandle); /* wine use HWND for owner window (unknown reason) */
  }
  else if (Wnd->Style & WS_CHILD) 
  {
    return Wnd->Parent;
  }

  return NULL;
}


HWND STDCALL
NtUserGetParent(HWND hWnd)
{
  PWINDOW_OBJECT Wnd, WndParent;
  HWND hWndParent = NULL;

  IntAcquireWinLockShared();

  if (!(Wnd = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }

  WndParent = IntGetParent(Wnd);
  if (WndParent) hWndParent = WndParent->Self;

  IntReleaseWinLock();

  return hWndParent;
}



HWND FASTCALL
IntSetFocusWindow(HWND hWnd)
{
  PUSER_MESSAGE_QUEUE OldMessageQueue;
  PDESKTOP_OBJECT DesktopObject;
  PWINDOW_OBJECT WindowObject;
  HWND hWndOldFocus;

  DPRINT("IntSetFocusWindow(hWnd 0x%x)\n", hWnd);

  if (hWnd != (HWND)0)
    {
      WindowObject = IntGetWindowObject(hWnd);
      if (!WindowObject)
        {
          DPRINT("Bad window handle 0x%x\n", hWnd);
          SetLastWin32Error(ERROR_INVALID_HANDLE);
      	  return (HWND)0;
        }
    }
  else
  {
    WindowObject = NULL;
  }

  DesktopObject = IntGetActiveDesktop();
  if (!DesktopObject)
    {
      DPRINT("No active desktop\n");
      if (WindowObject != NULL)
        {
    	    IntReleaseWindowObject(WindowObject);
        }
      SetLastWin32Error(ERROR_INVALID_HANDLE);
  	  return (HWND)0;
    }

  hWndOldFocus = (HWND)0;
  OldMessageQueue = (PUSER_MESSAGE_QUEUE)DesktopObject->ActiveMessageQueue;
  if (OldMessageQueue != NULL)
    {
      hWndOldFocus = OldMessageQueue->FocusWindow;
    }

  if (WindowObject != NULL)
    {
      WindowObject->MessageQueue->FocusWindow = hWnd;
      (PUSER_MESSAGE_QUEUE)DesktopObject->ActiveMessageQueue =
        WindowObject->MessageQueue;
      IntReleaseWindowObject(WindowObject);
    }
  else
    {
      (PUSER_MESSAGE_QUEUE)DesktopObject->ActiveMessageQueue = NULL;
    }

  DPRINT("hWndOldFocus = 0x%x\n", hWndOldFocus);

  return hWndOldFocus;
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


HWND FASTCALL IntGetDesktopWindow(VOID)
{
  return IntGetActiveDesktop()->DesktopWindow;
}

PWINDOW_OBJECT FASTCALL
IntGetWindowObject(HWND hWnd)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->
			       HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  return(WindowObject);
}

VOID FASTCALL
IntReleaseWindowObject(PWINDOW_OBJECT Window)
{
  ObmDereferenceObject(Window);
}

HMENU FASTCALL
IntGetSystemMenu(PWINDOW_OBJECT WindowObject, BOOL bRevert)
{
  PMENU_OBJECT MenuObject;
  PW32PROCESS W32Process;

  if(bRevert)
  {
    if(WindowObject->SystemMenu)
    {
      MenuObject = IntGetMenuObject(WindowObject->SystemMenu);
      if(MenuObject)
      {
        IntDestroyMenuObject(MenuObject, FALSE, TRUE);
      }
    }
    W32Process = PsGetWin32Process();
    if(W32Process->WindowStation->SystemMenuTemplate)
    {
      /* clone system menu */
      MenuObject = IntGetMenuObject(W32Process->WindowStation->SystemMenuTemplate);
      if(!MenuObject)
        return (HMENU)0;

      MenuObject = IntCloneMenu(MenuObject);
      if(MenuObject)
      {
        WindowObject->SystemMenu = MenuObject->Self;
        MenuObject->IsSystemMenu = TRUE;
      }
    }
    /* FIXME Load system menu here? 
    else
    {
    
    }*/
    return (HMENU)0;
  }
  else
  {
    return WindowObject->SystemMenu;
  }
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


/*!
 * Return the dimension of the window in the screen coordinates.
 * \param	hWnd	window handle.
 * \param	Rect	pointer to the buffer where the coordinates are returned.
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

/*!
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \param	hWnd	window handle.
 * \param	Rect	pointer to the buffer where the coordinates are returned.
 *
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


NTSTATUS FASTCALL
InitWindowImpl(VOID)
{
  InitializeListHead(&RegisteredMessageListHead);

  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupWindowImpl(VOID)
{
  return(STATUS_SUCCESS);
}


DWORD STDCALL
NtUserAlterWindowStyle(DWORD Unknown0,
		       DWORD Unknown1,
		       DWORD Unknown2)
{
  UNIMPLEMENTED

  return(0);
}

DWORD STDCALL
NtUserChildWindowFromPointEx(HWND Parent,
			     LONG x,
			     LONG y,
			     UINT Flags)
{
  UNIMPLEMENTED

  return(0);
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
  WindowObject->x = 0;
  WindowObject->y = 0;
  WindowObject->Width = Width;
  WindowObject->Height = Height;
  WindowObject->ParentHandle = NULL;
  WindowObject->Parent = NULL;
  WindowObject->Menu = NULL;
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
  PMENU_OBJECT SystemMenu;
  UNICODE_STRING WindowName;
  NTSTATUS Status;
  HANDLE Handle;
  POINT MaxSize, MaxPos, MinTrack, MaxTrack;
  CREATESTRUCTW Cs;
  LRESULT Result;
  DPRINT("NtUserCreateWindowEx\n");

  /* Initialize gui state if necessary. */
  IntGraphicsCheck(TRUE);

  if (!RtlCreateUnicodeString(&WindowName,
                              NULL == lpWindowName->Buffer ?
                              L"" : lpWindowName->Buffer))
    {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return((HWND)0);
    }

  /* FIXME: parent must belong to the current process */

  if (hWndParent != NULL)
    {
      ParentWindow = IntGetWindowObject(hWndParent);
    }
  else
    {
      hWndParent = PsGetWin32Thread()->Desktop->DesktopWindow;
      ParentWindow = IntGetWindowObject(hWndParent);
    }

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
  DbgPrint("1: Style is now %d\n", WindowObject->Style);
  
  SystemMenu = IntGetSystemMenu(WindowObject, TRUE);
  
  WindowObject->x = x;
  WindowObject->y = y;
  WindowObject->Width = nWidth;
  WindowObject->Height = nHeight;
  WindowObject->ContextHelpId = 0;
  WindowObject->ParentHandle = hWndParent;
  WindowObject->Menu = hMenu;
  if(SystemMenu)
    WindowObject->SystemMenu = SystemMenu->Self;
  else
    WindowObject->SystemMenu = (HMENU)0;
  WindowObject->Instance = hInstance;
  WindowObject->Parameters = lpParam;
  WindowObject->Self = Handle;
  WindowObject->MessageQueue = PsGetWin32Thread()->MessageQueue;
  WindowObject->Parent = ParentWindow;
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
      WindowObject->ExtraData = (PULONG)(WindowObject + 1);
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
      DbgPrint("3: Style is now %d\n", WindowObject->Style);
      if (!(dwStyle & WS_POPUP))
	{
	  WindowObject->Style |= WS_CAPTION;
      DbgPrint("4: Style is now %d\n", WindowObject->Style);
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
  WindowObject->ClientRect = WindowObject->WindowRect;

  /*
   * Get the size and position of the window.
   */
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

  WindowObject->WindowRect.left = x;
  WindowObject->WindowRect.top = y;
  WindowObject->WindowRect.right = x + nWidth;
  WindowObject->WindowRect.bottom = y + nHeight;
  WindowObject->ClientRect = WindowObject->WindowRect;

  /* FIXME: Initialize the window menu. */
  
  /* Initialize the window's scrollbars */
  if (dwStyle & WS_VSCROLL)
      SCROLL_CreateScrollBar(WindowObject, SB_VERT);
  if (dwStyle & WS_HSCROLL)
      SCROLL_CreateScrollBar(WindowObject, SB_HORZ);

  /* Send a NCCREATE message. */
  Cs.lpCreateParams = lpParam;
  Cs.hInstance = hInstance;
  Cs.hMenu = hMenu;
  Cs.hwndParent = hWndParent;
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
  DbgPrint("[win32k.window] NtUserCreateWindowEx style %d, exstyle %d, parent %d\n", Cs.style, Cs.dwExStyle, Cs.hwndParent);
//  NtUserSetWindowLong(Handle, GWL_STYLE, WindowObject->Style, TRUE);
//  NtUserSetWindowLong(Handle, GWL_EXSTYLE, WindowObject->ExStyle, TRUE);
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
      
      lParam = 
	MAKE_LONG(WindowObject->ClientRect.right - 
		  WindowObject->ClientRect.left,
		  WindowObject->ClientRect.bottom - 
		  WindowObject->ClientRect.top);
 
      DPRINT("NtUserCreateWindow(): About to send WM_SIZE\n");
      IntCallWindowProc(NULL, WindowObject->Self, WM_SIZE, SIZE_RESTORED, 
			 lParam);
      lParam = 
	MAKE_LONG(WindowObject->ClientRect.left,
		  WindowObject->ClientRect.top);
      DPRINT("NtUserCreateWindow(): About to send WM_MOVE\n");
      IntCallWindowProc(NULL, WindowObject->Self, WM_MOVE, 0, lParam);
    }

  /* Move from parent-client to screen coordinates */
  if (0 != (WindowObject->Style & WS_CHILD))
    {
    NtGdiOffsetRect(&WindowObject->WindowRect,
		   ParentWindow->ClientRect.left,
		   ParentWindow->ClientRect.top);
    NtGdiOffsetRect(&WindowObject->ClientRect,
		   ParentWindow->ClientRect.left,
		   ParentWindow->ClientRect.top);
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
	DbgPrint("Setting Active Window to %d\n\n\n",WindowObject->Self);
  NtUserSetActiveWindow(WindowObject->Self);
  DPRINT("NtUserCreateWindow(): = %X\n", Handle);
  return((HWND)Handle);
}

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

static BOOLEAN IntWndBelongsToThread(PWINDOW_OBJECT Window, PW32THREAD ThreadData)
{
  if (Window->OwnerThread && Window->OwnerThread->Win32Thread)
  {
    return (Window->OwnerThread->Win32Thread == ThreadData);
  }

  return FALSE;
}

static BOOL BuildChildWindowArray(PWINDOW_OBJECT Window, HWND **Children, unsigned *NumChildren)
{
  unsigned Index;
  PWINDOW_OBJECT Child;

  *Children = NULL;
  *NumChildren = 0;

  ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);
  Child = Window->FirstChild;
  while (Child)
  {
    (*NumChildren)++;
    Child = Child->NextSibling;
  }
  
  if (0 != *NumChildren)
  {
    *Children = ExAllocatePoolWithTag(PagedPool, *NumChildren * sizeof(HWND), TAG_WNAM);
    if (NULL != *Children)
    {
      Child = Window->FirstChild;
      Index = 0;
      while (Child)
      {
        (*Children)[Index] = Child->Self;
        Child = Child->NextSibling;
        Index++;
      }
      assert(Index == *NumChildren);
    }
    else
    {
      DPRINT1("Failed to allocate memory for children array\n");
    }
  }
  ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);


  return 0 == *NumChildren || NULL != *Children;
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
  if (! BuildChildWindowArray(Window, &Children, &NumChildren))
    {
      return 0;
    }
  for (Index = NumChildren; 0 < Index; Index--)
    {
      Child = IntGetWindowObject(Children[Index - 1]);
      if (NULL != Child)
	{
	  if (IntWndBelongsToThread(Child, ThreadData))
	    {
	      IntDestroyWindow(Child, ProcessData, ThreadData, SendMessages);
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
  DCE_FreeWindowDCE(Window->Self);    /* Always do this to catch orphaned DCs */
  WINPROC_FreeProc(Window->winproc, WIN_PROC_WINDOW);
  CLASS_RemoveWindow(Window->Class);
#endif

  ExAcquireFastMutexUnsafe(&Window->Parent->ChildrenListLock);
  IntUnlinkWindow(Window);
  ExReleaseFastMutexUnsafe(&Window->Parent->ChildrenListLock);

  ExAcquireFastMutexUnsafe (&ThreadData->WindowListLock);
  RemoveEntryList(&Window->ThreadListEntry);
  ExReleaseFastMutexUnsafe (&ThreadData->WindowListLock);

  Window->Class = NULL;
  ObmCloseHandle(ProcessData->WindowStation->HandleTable, Window->Self);

  IntGraphicsCheck(FALSE);

  return 0;
}

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

  /* Check for desktop window (has NULL parent) */
  if (NULL == Window->Parent)
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
#if 0 /* FIXME */
      if (hwnd == GetActiveWindow())
	{
	  WINPOS_ActivateOtherWindow( hwnd );
	}
#endif
    }

#if 0 /* FIXME */
  if (! IsWindow(Wnd))
    {
    return TRUE;
    }
#endif

  /* Recursively destroy owned windows */
#if 0 /* FIXME */
  if (! isChild)
    {
      for (;;)
	{
	  int i;
	  BOOL GotOne = FALSE;
	  HWND *list = WIN_ListChildren(GetDesktopWindow());
	  if (list)
	    {
	      for (i = 0; list[i]; i++)
		{
		  if (GetWindow(list[i], GW_OWNER) != Wnd)
		    {
		      continue;
		    }
		  if (WIN_IsCurrentThread(list[i]))
		    {
		      DestroyWindow(list[i]);
		      GotOne = TRUE;
		      continue;
		    }
		  WIN_SetOwner(list[i], NULL);
		}
	      HeapFree(GetProcessHeap(), 0, list);
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

#if 0 /* FIXME */
  if (!IsWindow(Wnd))
    {
      return TRUE;
    }
#endif

  /* Unlink now so we won't bother with the children later on */
#if 0 /* FIXME */
  WIN_UnlinkWindow( hwnd );
#endif

  /* Destroy the window storage */
  IntDestroyWindow(Window, PsGetWin32Process(), PsGetWin32Thread(), TRUE);

  return TRUE;
}

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

DWORD STDCALL
NtUserEndDeferWindowPosEx(DWORD Unknown0,
			  DWORD Unknown1)
{
  UNIMPLEMENTED
    
  return 0;
}

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

DWORD STDCALL
NtUserFlashWindowEx(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetForegroundWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetWindowDC(HWND hWnd)
{
  return (DWORD) NtUserGetDCEx( hWnd, 0, DCX_USESTYLE | DCX_WINDOW );
}

DWORD STDCALL
NtUserGetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

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

DWORD STDCALL
NtUserLockWindowUpdate(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

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
DWORD STDCALL
NtUserQueryWindow(HWND hWnd, DWORD Index)
{

PWINDOW_OBJECT Window = IntGetWindowObject(hWnd);

        if(Window == NULL) return((DWORD)NULL);

        IntReleaseWindowObject(Window);

        switch(Index)
        {
        case 0x00:
                return((DWORD)Window->OwnerThread->ThreadsProcess->UniqueProcessId);

        case 0x01:
                return((DWORD)Window->OwnerThread->Cid.UniqueThread);

        default:
                return((DWORD)NULL);
        }

}

DWORD STDCALL
NtUserRealChildWindowFromPoint(DWORD Unknown0,
			       DWORD Unknown1,
			       DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

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

DWORD STDCALL
NtUserSetImeOwnerWindow(DWORD Unknown0,
			DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2,
			   DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;

}

DWORD STDCALL
NtUserSetLayeredWindowAttributes(DWORD Unknown0,
				 DWORD Unknown1,
				 DWORD Unknown2,
				 DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetLogonNotifyWindow(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}


 /* globally stored handles to the shell windows */
HWND hwndShellWindow = 0;
HWND hwndShellListView = 0;
DWORD pidShellWindow = 0;

DWORD STDCALL
NtUserSetShellWindowEx(HWND hwndShell, HWND hwndShellListView)
{
	PEPROCESS my_current = IoGetCurrentProcess();

	 /* test if we are permitted to change the shell window */
	if (pidShellWindow && my_current->UniqueProcessId!=pidShellWindow)
		return FALSE;

	hwndShellWindow = hwndShell;
	hwndShellListView = hwndShellListView;

	if (hwndShell)
		pidShellWindow = my_current->UniqueProcessId;	/* request shell window for the calling process */
	else
		pidShellWindow = 0;	/* shell window is now free for other processes. */

	return TRUE;
}

HWND STDCALL
NtUserGetShellWindow()
{
	return hwndShellWindow;
}


DWORD STDCALL
NtUserSetWindowFNID(DWORD Unknown0,
		    DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

LONG STDCALL
NtUserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  LONG Result;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  if (0 <= (int) Index)
    {
      if (WindowObject->ExtraDataSize - sizeof(LONG) < Index ||
          0 != Index % sizeof(LONG))
	{
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  return 0;
	}
      Result = WindowObject->ExtraData[Index / sizeof(LONG)];
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
	  {
		Result = (LONG) WindowObject->WndProcA;
	  }
	  else
	  {
		Result = (LONG) WindowObject->WndProcW;
	  }
	  break;

	case GWL_HINSTANCE:
	  Result = (LONG) WindowObject->Instance;
	  break;

	case GWL_HWNDPARENT:
	  Result = (LONG) WindowObject->ParentHandle;
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

  ObmDereferenceObject(WindowObject);

  return Result;
}

LONG STDCALL
NtUserSetWindowLong(HWND hWnd, DWORD Index, LONG NewValue, BOOL Ansi)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  LONG OldValue;
  STYLESTRUCT Style;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       hWnd,
			       otWindow,
			       (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return(0);
    }

  if (0 <= (int) Index)
    {
      if (WindowObject->ExtraDataSize - sizeof(LONG) < Index ||
          0 != Index % sizeof(LONG))
	{
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  return 0;
	}
      OldValue = WindowObject->ExtraData[Index / sizeof(LONG)];
      WindowObject->ExtraData[Index / sizeof(LONG)] = NewValue;
    }
  else
    {
      switch (Index)
	{
	case GWL_EXSTYLE:
	  OldValue = (LONG) WindowObject->ExStyle;
	  Style.styleOld = OldValue;
	  Style.styleNew = NewValue;
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
		WindowObject->WndProcW = (WNDPROC) NewValue+0x80000000;
		WindowObject->Unicode = FALSE;
	  }
	  else
	  {
	    OldValue = (LONG) WindowObject->WndProcW;
	    WindowObject->WndProcW = (WNDPROC) NewValue;
		WindowObject->WndProcA = (WNDPROC) NewValue+0x80000000;
		WindowObject->Unicode = TRUE;
	  }
	  break;

	case GWL_HINSTANCE:
	  OldValue = (LONG) WindowObject->Instance;
	  WindowObject->Instance = (HINSTANCE) NewValue;
	  break;

	case GWL_HWNDPARENT:
	  OldValue = (LONG) WindowObject->ParentHandle;
	  WindowObject->ParentHandle = (HWND) NewValue;
	  /* FIXME: Need to update window lists of old and new parent */
	  UNIMPLEMENTED;
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

  ObmDereferenceObject(WindowObject);
  return(OldValue);
}

DWORD STDCALL
NtUserSetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

BOOL 
STDCALL NtUserSetWindowPos(      
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

DWORD STDCALL
NtUserSetWindowRgn(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}


WORD STDCALL
NtUserSetWindowWord(HWND hWnd, INT Index, WORD NewVal)
{
  UNIMPLEMENTED
  return 0;
}


BOOL STDCALL
NtUserShowWindow(HWND hWnd,
		 LONG nCmdShow)
{
  return(WinPosShowWindow(hWnd, nCmdShow));
}

DWORD STDCALL
NtUserShowWindowAsync(DWORD Unknown0,
		      DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL NtUserUpdateWindow( HWND hWnd )
{
    PWINDOW_OBJECT pWindow = IntGetWindowObject( hWnd);

    if (!pWindow)
        return FALSE;
    if (pWindow->UpdateRegion)
        NtUserSendMessage( hWnd, WM_PAINT,0,0);
    IntReleaseWindowObject(pWindow);
    return TRUE;
}

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

DWORD STDCALL
NtUserWindowFromPoint(DWORD Unknown0,
		      DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

HWND STDCALL
NtUserGetDesktopWindow()
{
  return IntGetDesktopWindow();
}

HWND STDCALL
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


DWORD FASTCALL
IntGetWindowThreadProcessId(PWINDOW_OBJECT Wnd, PDWORD pid)
{
   if (pid) *pid = (DWORD) Wnd->OwnerThread->ThreadsProcess->UniqueProcessId;
   return (DWORD) Wnd->OwnerThread->Cid.UniqueThread;
   
}


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
 * As best as I can figure, this function is used by EnumWindows,
 * EnumChildWindows, EnumDesktopWindows, & EnumThreadWindows.
 *
 * It's supposed to build a list of HWNDs to return to the caller.
 * We can figure out what kind of list by what parameters are
 * passed to us.
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
	  DPRINT("Bad window handle 0x%x\n", hWnd);
	  SetLastWin32Error(ERROR_INVALID_HANDLE);
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

VOID STDCALL
NtUserValidateRect(HWND hWnd, const RECT* Rect)
{
  (VOID)NtUserRedrawWindow(hWnd, Rect, 0, RDW_VALIDATE | RDW_NOCHILDREN);
}

/*
 * @unimplemented
 */
DWORD STDCALL
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


HWND STDCALL
NtUserGetWindow(HWND hWnd, UINT Relationship)
{
  PWINDOW_OBJECT Wnd;
  HWND hWndResult = NULL;

  IntAcquireWinLockShared();

  if (!(Wnd = IntGetWindowObject(hWnd)))
  {
    IntReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }
  
  switch (Relationship)
  {
    case GW_HWNDFIRST:
      if (Wnd->Parent && Wnd->Parent->FirstChild)
      {
        hWndResult = Wnd->Parent->FirstChild->Self;
      }
      break;
    case GW_HWNDLAST:
      if (Wnd->Parent && Wnd->Parent->LastChild)
      {
        hWndResult = Wnd->Parent->LastChild->Self;
      }
      break;
    case GW_HWNDNEXT:
      if (Wnd->Parent && Wnd->NextSibling)
      {
        hWndResult = Wnd->NextSibling->Self;
      }
      break;
    case GW_HWNDPREV:
      if (Wnd->Parent && Wnd->PrevSibling)
      {
        hWndResult = Wnd->PrevSibling->Self;
      }
      break;
    case GW_OWNER:
      if (Wnd->Parent)
      {
        hWndResult = Wnd->hWndOwner;
      }
      break;
    case GW_CHILD:
      if (Wnd->FirstChild)
      {
        hWndResult = Wnd->FirstChild->Self;
      }
      break;
  }

  IntReleaseWinLock();

  return hWndResult;
}


HWND STDCALL
NtUserGetLastActivePopup(HWND hWnd)
{
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
}


PWINDOW_OBJECT FASTCALL
IntSetParent(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndNewParent)
{
  PWINDOW_OBJECT WndOldParent;
  BOOL was_visible;
  HWND hWnd, hWndNewParent, hWndOldParent;

  if (!WndNewParent) WndNewParent = IntGetWindowObject(IntGetDesktopWindow());

  hWnd = Wnd;
  hWndNewParent = WndNewParent;

#if 0
  if (!(full_handle = WIN_IsCurrentThread( hwnd )))
    return (HWND)SendMessageW( hwnd, WM_WINE_SETPARENT, (WPARAM)parent, 0 );

  if (USER_Driver.pSetParent)
    return USER_Driver.pSetParent( hwnd, parent );
#endif

  /* Windows hides the window first, then shows it again
   * including the WM_SHOWWINDOW messages and all */
  was_visible = WinPosShowWindow( hWnd, SW_HIDE );

  /* validate that window and parent still exist */
  if (!IntGetWindowObject(hWnd) || !IntGetWindowObject(hWndNewParent)) return NULL;

  /* window must belong to current process */
  if (Wnd->OwnerThread->ThreadsProcess != PsGetCurrentProcess()) return NULL;

  WndOldParent = Wnd->Parent;
  hWndOldParent =  WndOldParent->Self;

  if (WndNewParent != WndOldParent)
  {
    IntUnlinkWindow(Wnd);
    IntLinkWindow(Wnd, WndNewParent, NULL /*prev sibling*/);

    if (WndNewParent->Self != IntGetDesktopWindow()) /* a child window */
    {
      if (!(Wnd->Style & WS_CHILD))
      {
        //if ( Wnd->Menu ) DestroyMenu ( Wnd->menu );
        Wnd->Menu = NULL;
      }
    }
  }

  /* SetParent additionally needs to make hwnd the topmost window
       in the x-order and send the expected WM_WINDOWPOSCHANGING and
       WM_WINDOWPOSCHANGED notification messages.
   */
  WinPosSetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | (was_visible ? SWP_SHOWWINDOW : 0) );
  /* FIXME: a WM_MOVE is also generated (in the DefWindowProc handler
   * for WM_WINDOWPOSCHANGED) in Windows, should probably remove SWP_NOMOVE */
  
  /* validate that the old parent still exist, since it migth have been destroyed
     during the last callbacks to user-mode 
   */
  return (IntGetWindowObject(hWndOldParent) ? WndOldParent : NULL);
}


HWND
STDCALL
NtUserSetParent(HWND hWndChild, HWND hWndNewParent)
{
  PWINDOW_OBJECT Wnd = NULL, WndParent = NULL, WndOldParent;
  HWND hWndOldParent;

  if (IntIsBroadcastHwnd(hWndChild) || IntIsBroadcastHwnd(hWndNewParent))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
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

  if (!(Wnd = IntGetWindowObject(hWndNewParent)))
  {
    IntReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }

  WndOldParent = IntSetParent(Wnd, WndParent);
  if (WndOldParent) hWndOldParent = WndOldParent->Self;

  IntReleaseWinLock();

  return hWndOldParent;    
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetMenu(
  HWND hWnd,
  HMENU hMenu,
  BOOL bRepaint)
{
  PWINDOW_OBJECT WindowObject;
  PMENU_OBJECT MenuObject;
  WindowObject = IntGetWindowObject((HWND)hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
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
    
    WindowObject->Menu = hMenu;
    
    IntReleaseMenuObject(MenuObject);
  }
  else
  {
    /* remove the menu handle */
    WindowObject->Menu = 0;
  }
  
  IntReleaseWindowObject(WindowObject);
  
  /* FIXME (from wine)
  if(bRepaint)
  {
    if (IsWindowVisible(hWnd))
        SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                      SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
  }
  */
  
  return TRUE;
}


/*
 * @implemented
 */
HMENU
STDCALL
NtUserGetSystemMenu(
  HWND hWnd,
  BOOL bRevert)
{
  HMENU res = (HMENU)0;
  PWINDOW_OBJECT WindowObject;
  WindowObject = IntGetWindowObject((HWND)hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return (HMENU)0;
  }
  
  res = IntGetSystemMenu(WindowObject, bRevert);
  
  IntReleaseWindowObject(WindowObject);
  return res;
}

/*
 * @implemented
 */
BOOL
STDCALL
NtUserSetSystemMenu(
  HWND hWnd,
  HMENU hMenu)
{
  BOOL res = FALSE;
  PWINDOW_OBJECT WindowObject;
  PMENU_OBJECT MenuObject;
  WindowObject = IntGetWindowObject((HWND)hWnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  if(hMenu)
  {
    /* assign new menu handle */
    MenuObject = IntGetMenuObject(hMenu);
    if(!MenuObject)
    {
      IntReleaseWindowObject(WindowObject);
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      return FALSE;
    }
    
    res = IntSetSystemMenu(WindowObject, MenuObject);
    
    IntReleaseMenuObject(MenuObject);
  }
  
  IntReleaseWindowObject(WindowObject);
  return res;
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


/* EOF */
