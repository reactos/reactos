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
/* $Id: window.c,v 1.51 2003/05/31 08:51:58 gvg Exp $
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

HWND STDCALL
NtUserGetAncestor(HWND hWnd, UINT Flags)
{
  if (W32kIsDesktopWindow(hWnd))
    {
      return(NULL);
    }
  if (Flags & GA_PARENT)
    {
      PWINDOW_OBJECT Window;
      HWND hParent;

      Window = W32kGetWindowObject(hWnd);
      if (Window == NULL)
	{
	  return(NULL);
	}     

      if (Window->Parent == NULL)
	{
	  W32kReleaseWindowObject(Window);
	}

      hParent = Window->Parent->Self;

      W32kReleaseWindowObject(Window);

      return(hParent);
    }
  else
    {
      UNIMPLEMENTED;
      return(NULL);
    }
}

VOID FASTCALL
W32kSetFocusWindow(HWND hWnd)
{
}

BOOL FASTCALL
W32kIsChildWindow(HWND Parent, HWND Child)
{
  PWINDOW_OBJECT BaseWindow = W32kGetWindowObject(Child);
  PWINDOW_OBJECT Window = BaseWindow;
  while (Window != NULL && Window->Style & WS_CHILD)
    {
      if (Window->Self == Parent)
	{
	  W32kReleaseWindowObject(BaseWindow);
	  return(TRUE);
	}
      Window = Window->Parent;
    }
  W32kReleaseWindowObject(BaseWindow);
  return(FALSE);  
}

BOOL FASTCALL
W32kIsWindowVisible(HWND Wnd)
{
  PWINDOW_OBJECT BaseWindow = W32kGetWindowObject(Wnd);
  PWINDOW_OBJECT Window = BaseWindow;
  BOOLEAN Result = FALSE;
  while (Window != NULL && Window->Style & WS_CHILD)
    {
      if (!(Window->Style & WS_VISIBLE))
	{
	  W32kReleaseWindowObject(BaseWindow);
	  return(FALSE);
	}
      Window = Window->Parent;
    }
  if (Window != NULL && Window->Style & WS_VISIBLE)
    {
      Result = TRUE;
    }
  W32kReleaseWindowObject(BaseWindow);
  return(Result);
}

BOOL FASTCALL
W32kIsDesktopWindow(HWND hWnd)
{
  PWINDOW_OBJECT WindowObject;
  BOOL IsDesktop;
  WindowObject = W32kGetWindowObject(hWnd);
  IsDesktop = WindowObject->Parent == NULL;
  W32kReleaseWindowObject(WindowObject);
  return(IsDesktop);
}

HWND FASTCALL W32kGetDesktopWindow(VOID)
{
  return W32kGetActiveDesktop()->DesktopWindow;
}

HWND FASTCALL W32kGetParentWindow(HWND hWnd)
{
  return W32kGetWindowObject(hWnd)->ParentHandle;
}

PWINDOW_OBJECT FASTCALL
W32kGetWindowObject(HWND hWnd)
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
W32kReleaseWindowObject(PWINDOW_OBJECT Window)
{
  ObmDereferenceObject(Window);
}

/*!
 * Internal function.
 * Returns client window rectangle relative to the upper-left corner of client area.
 *
 * \note Does not check the validity of the parameters
*/
VOID FASTCALL
W32kGetClientRect(PWINDOW_OBJECT WindowObject, PRECT Rect)
{
  ASSERT( WindowObject );
  ASSERT( Rect );

  Rect->left = Rect->top = 0;
  Rect->right = WindowObject->ClientRect.right - WindowObject->ClientRect.left;
  Rect->bottom = 
    WindowObject->ClientRect.bottom - WindowObject->ClientRect.top;
}

/*!
 * Internal Function.
 * Return the dimension of the window in the screen coordinates.
*/
BOOL STDCALL
W32kGetWindowRect(HWND hWnd, LPRECT Rect)
{
  PWINDOW_OBJECT WindowObject;

  ASSERT( Rect );

  WindowObject = W32kGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return(FALSE);
    }
  *Rect = WindowObject->WindowRect;
  if (WindowObject->Style & WS_CHILD)
    {
      DbgBreakPoint();
    }
  W32kReleaseWindowObject(WindowObject);
  return(TRUE);
}

/*!
 * Return the dimension of the window in the screen coordinates.
 * \param	hWnd	window handle.
 * \param	Rect	pointer to the buffer where the coordinates are returned.
*/
BOOL STDCALL
NtUserGetWindowRect(HWND hWnd, LPRECT Rect)
{
  RECT SafeRect;
  BOOL bRet;

  bRet = W32kGetWindowRect(hWnd, &SafeRect);
  if (! NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT)))){
    return(FALSE);
  }
  return( bRet );
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

  WindowObject = W32kGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return(FALSE);
    }
  W32kGetClientRect(WindowObject, &SafeRect);
  if (! NT_SUCCESS(MmCopyToCaller(Rect, &SafeRect, sizeof(RECT))))
    {
      return(FALSE);
    }

  W32kReleaseWindowObject(WindowObject);
  return(TRUE);
}

HWND FASTCALL
W32kGetActiveWindow(VOID)
{
  PUSER_MESSAGE_QUEUE Queue;
  Queue = (PUSER_MESSAGE_QUEUE)W32kGetActiveDesktop()->ActiveMessageQueue;
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
W32kGetFocusWindow(VOID)
{
  PUSER_MESSAGE_QUEUE Queue;
  PDESKTOP_OBJECT pdo = W32kGetActiveDesktop();

  if( !pdo )
	return NULL;

  Queue = (PUSER_MESSAGE_QUEUE)pdo->ActiveMessageQueue;

  if (Queue == NULL)
      return(NULL);
  else
      return(Queue->FocusWindow);
}


WNDPROC FASTCALL
W32kGetWindowProc(HWND Wnd)
{
  PWINDOW_OBJECT WindowObject;
  WNDPROC WndProc;

  WindowObject = W32kGetWindowObject(Wnd);
  if( !WindowObject )
	return NULL;

  WndProc = WindowObject->WndProc;
  W32kReleaseWindowObject(Wnd);
  return(WndProc);
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
W32kCreateDesktopWindow(PWINSTATION_OBJECT WindowStation,
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
  WindowObject->WndProc = DesktopClass->Class.lpfnWndProc;
  InitializeListHead(&WindowObject->ChildrenListHead);

  WindowName = ExAllocatePool(NonPagedPool, sizeof(L"DESKTOP"));
  wcscpy(WindowName, L"DESKTOP");
  RtlInitUnicodeString(&WindowObject->WindowName, WindowName);

  return(Handle);
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
  UNICODE_STRING WindowName;
  NTSTATUS Status;
  HANDLE Handle;
  POINT MaxSize, MaxPos, MinTrack, MaxTrack;
  CREATESTRUCTW Cs;
  LRESULT Result;
  DPRINT("NtUserCreateWindowEx\n");

  /* Initialize gui state if necessary. */
  W32kGuiCheck();
  W32kGraphicsCheck(TRUE);

  if (!RtlCreateUnicodeString(&WindowName, lpWindowName->Buffer))
    {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return((HWND)0);
    }

  if (hWndParent != NULL)
    {
      ParentWindow = W32kGetWindowObject(hWndParent);
    }
  else
    {
      hWndParent = PsGetWin32Thread()->Desktop->DesktopWindow;
      ParentWindow = W32kGetWindowObject(hWndParent);
    }

  /* Check the class. */
  Status = ClassReferenceClassByNameOrAtom(&ClassObject, lpClassName->Buffer);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&WindowName);
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
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return (HWND)0;
    }

  /* Create the window object. */
  WindowObject = (PWINDOW_OBJECT)
    ObmCreateObject(PsGetWin32Process()->WindowStation->HandleTable, &Handle,
		    otWindow, sizeof(WINDOW_OBJECT));
  DPRINT("Created object with handle %X\n", Handle);
  if (!WindowObject)
    {
      ObDereferenceObject(WinStaObject);
      ObmDereferenceObject(ClassObject);
      RtlFreeUnicodeString(&WindowName);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return (HWND)0;
    }
  ObDereferenceObject(WinStaObject);

  /*
   * Fill out the structure describing it.
   */
  WindowObject->Class = ClassObject;
  WindowObject->ExStyle = dwExStyle;
  WindowObject->Style = dwStyle | WIN_NCACTIVATED;
  WindowObject->x = x;
  WindowObject->y = y;
  WindowObject->Width = nWidth;
  WindowObject->Height = nHeight;
  WindowObject->ParentHandle = hWndParent;
  WindowObject->Menu = hMenu;
  WindowObject->Instance = hInstance;
  WindowObject->Parameters = lpParam;
  WindowObject->Self = Handle;
  WindowObject->MessageQueue = PsGetWin32Thread()->MessageQueue;
  WindowObject->Parent = ParentWindow;
  WindowObject->UserData = 0;
  WindowObject->WndProc = ClassObject->Class.lpfnWndProc;
  InsertHeadList(&ParentWindow->ChildrenListHead,
		 &WindowObject->SiblingListEntry);
  InitializeListHead(&WindowObject->ChildrenListHead);
  InitializeListHead(&WindowObject->PropListHead);
  ExInitializeFastMutex(&WindowObject->ChildrenListLock);

  RtlInitUnicodeString(&WindowObject->WindowName, WindowName.Buffer);
  RtlFreeUnicodeString(&WindowName);

  if (ClassObject->Class.cbWndExtra != 0)
    {
      WindowObject->ExtraData =
	ExAllocatePool(PagedPool,
		       ClassObject->Class.cbWndExtra);
      WindowObject->ExtraDataSize = ClassObject->Class.cbWndExtra;
    }
  else
    {
      WindowObject->ExtraData = NULL;
      WindowObject->ExtraDataSize = 0;
    }

  /* Correct the window style. */
  if (!(dwStyle & WS_CHILD))
    {
      WindowObject->Style |= WS_CLIPSIBLINGS;
      if (!(dwStyle & WS_POPUP))
	{
	  WindowObject->Style |= WS_CAPTION;
	  /* FIXME: Note the window needs a size. */ 
	}
    }

  /* Insert the window into the process's window list. */
  ExAcquireFastMutexUnsafe (&PsGetWin32Thread()->WindowListLock);
  InsertTailList (&PsGetWin32Thread()->WindowListHead, 
		  &WindowObject->ThreadListEntry);
  ExReleaseFastMutexUnsafe (&PsGetWin32Thread()->WindowListLock);

  /*
   * Insert the window into the list of windows associated with the thread's
   * desktop. 
   */
  InsertTailList(&PsGetWin32Thread()->Desktop->WindowListHead,
		 &WindowObject->DesktopListEntry);
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
  DPRINT("NtUserCreateWindowEx(): About to send NCCREATE message.\n");
  Result = W32kSendNCCREATEMessage(WindowObject->Self, &Cs);
  if (!Result)
    {
      /* FIXME: Cleanup. */
      DPRINT("NtUserCreateWindowEx(): NCCREATE message failed.\n");
      return(NULL);
    }
 
  /* Calculate the non-client size. */
  MaxPos.x = WindowObject->WindowRect.left;
  MaxPos.y = WindowObject->WindowRect.top;
  DPRINT("NtUserCreateWindowEx(): About to get non-client size.\n");
  Result = WinPosGetNonClientSize(WindowObject->Self, 
				  &WindowObject->WindowRect,
				  &WindowObject->ClientRect);
  W32kOffsetRect(&WindowObject->WindowRect, 
		 MaxPos.x - WindowObject->WindowRect.left,
		 MaxPos.y - WindowObject->WindowRect.top);

  /* Send the CREATE message. */
  DPRINT("NtUserCreateWindowEx(): about to send CREATE message.\n");
  Result = W32kSendCREATEMessage(WindowObject->Self, &Cs);
  if (Result == (LRESULT)-1)
    {
      /* FIXME: Cleanup. */
      DPRINT("NtUserCreateWindowEx(): send CREATE message failed.\n");
      return(NULL);
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
      W32kCallWindowProc(NULL, WindowObject->Self, WM_SIZE, SIZE_RESTORED, 
			 lParam);
      lParam = 
	MAKE_LONG(WindowObject->ClientRect.left,
		  WindowObject->ClientRect.top);
      DPRINT("NtUserCreateWindow(): About to send WM_MOVE\n");
      W32kCallWindowProc(NULL, WindowObject->Self, WM_MOVE, 0, lParam);
    }

  /* Move from parent-client to screen coordinates */
  if (0 != (WindowObject->Style & WS_CHILD))
    {
    W32kOffsetRect(&WindowObject->WindowRect,
		   ParentWindow->ClientRect.left,
		   ParentWindow->ClientRect.top);
    W32kOffsetRect(&WindowObject->ClientRect,
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
	((WindowObject->Style & WS_CHILD) || W32kGetActiveWindow()) ?
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
      W32kCallWindowProc(NULL, WindowObject->Parent->Self,
			 WM_PARENTNOTIFY, 
			 MAKEWPARAM(WM_CREATE, WindowObject->IDMenu),
			 (LPARAM)WindowObject->Self);
    }

  if (dwStyle & WS_VISIBLE)
    {
      DPRINT("NtUserCreateWindow(): About to show window\n");
      WinPosShowWindow(WindowObject->Self, dwShowMode);
    }

  DPRINT("NtUserCreateWindow(): = %X\n", Handle);
  return((HWND)Handle);
}

DWORD STDCALL
NtUserDeferWindowPos(HDWP WinPosInfo,
		     HWND Wnd,
		     HWND WndInsertAfter,
		     LONG x,
		     LONG y,
		     LONG cx,
		     LONG cy,
		     UINT Flags)
{
  UNIMPLEMENTED

  return 0;
}

BOOLEAN STDCALL
NtUserDestroyWindow(HWND Wnd)
{
  W32kGraphicsCheck(FALSE);

  return 0;
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

HWND STDCALL
NtUserFindWindowEx(HWND hwndParent,
		   HWND hwndChildAfter,
		   LPCWSTR ucClassName,
		   LPCWSTR ucWindowName)
{
#if 0
  NTSTATUS status;
  HWND windowHandle;
  PWINDOW_OBJECT windowObject;
  PLIST_ENTRY currentEntry;
  PWNDCLASS_OBJECT classObject;
  
  W32kGuiCheck();
  
  status = ClassReferenceClassByNameOrAtom(&classObject, ucClassName->Buffer);
  if (!NT_SUCCESS(status))
    {
      return (HWND)0;
    }

  ExAcquireFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
  currentEntry = PsGetWin32Process()->WindowListHead.Flink;
  while (currentEntry != &PsGetWin32Process()->WindowListHead)
    {
      windowObject = CONTAINING_RECORD (currentEntry, WINDOW_OBJECT, 
					ListEntry);

      if (classObject == windowObject->Class &&
	  RtlCompareUnicodeString (ucWindowName, &windowObject->WindowName, 
				   TRUE) == 0)
	{
	  ObmCreateHandle(PsGetWin32Process()->HandleTable,
			  windowObject,
			  &windowHandle);
	  ExReleaseFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
	  ObmDereferenceObject (classObject);
      
	  return  windowHandle;
	}
      currentEntry = currentEntry->Flink;
  }
  ExReleaseFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
  
  ObmDereferenceObject (classObject);
#endif

  return  (HWND)0;
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
NtUserInternalGetWindowText(DWORD Unknown0,
			    DWORD Unknown1,
			    DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
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
    PWINDOW_OBJECT Window = W32kGetWindowObject(hWnd);
    ULONG uStyle, uExStyle;
    WINDOWPOS pWinPos;

    if (!Window) return FALSE;
    
    uStyle = Window->Style;
    uExStyle = Window->ExStyle;
    pWinPos.hwnd = hWnd;
    
    pWinPos.x = X;
    pWinPos.y = Y;
    if (nWidth > NtUserGetSystemMetrics(SM_CXMIN))
        pWinPos.cx = pWinPos.x + nWidth;
    else
        pWinPos.cx = pWinPos.x + NtUserGetSystemMetrics(SM_CXMIN);
        
    if (nHeight > NtUserGetSystemMetrics(SM_CYMIN))
        pWinPos.cy = pWinPos.x + nHeight;
    else
        pWinPos.cy = pWinPos.y + NtUserGetSystemMetrics(SM_CYMIN);
    W32kSendWINDOWPOSCHANGINGMessage(Window->Self, &pWinPos);
    
    Window->WindowRect.top = Window->ClientRect.top = pWinPos.y;
    Window->WindowRect.left = Window->ClientRect.left = pWinPos.x;
    Window->WindowRect.bottom = Window->ClientRect.bottom = pWinPos.cy;
    Window->WindowRect.right = Window->ClientRect.right = pWinPos.cx;
    
    if (!(uStyle & WS_THICKFRAME))
    {
      Window->ClientRect.top += NtUserGetSystemMetrics(SM_CYFIXEDFRAME);
      Window->ClientRect.bottom -= NtUserGetSystemMetrics(SM_CYFIXEDFRAME);
      Window->ClientRect.left += NtUserGetSystemMetrics(SM_CXFIXEDFRAME);
      Window->ClientRect.right -= NtUserGetSystemMetrics(SM_CXFIXEDFRAME);
    }
    else
    {
        Window->ClientRect.top += NtUserGetSystemMetrics(SM_CYSIZEFRAME);
        Window->ClientRect.bottom -= NtUserGetSystemMetrics(SM_CYSIZEFRAME);
        Window->ClientRect.left += NtUserGetSystemMetrics(SM_CXSIZEFRAME);
        Window->ClientRect.right -= NtUserGetSystemMetrics(SM_CXSIZEFRAME);
    }

    if (uStyle & WS_CAPTION)
       Window->ClientRect.top += NtUserGetSystemMetrics(SM_CYCAPTION) + 1;
    if ( Window->Class->Class.lpszMenuName)
    {
        Window->ClientRect.top += NtUserGetSystemMetrics(SM_CYMENU);
    }

    W32kSendWINDOWPOSCHANGEDMessage(Window->Self, &pWinPos);
    
    NtUserSendMessage(hWnd, WM_MOVE, 0, MAKEWORD(Window->ClientRect.left,
                                                 Window->ClientRect.top));
                                                 
    NtUserSendMessage(hWnd, WM_SIZE, 0, MAKEWORD(Window->ClientRect.right -
                                                 Window->ClientRect.left,
                                                 Window->ClientRect.bottom -
                                                 Window->ClientRect.top));

    /* FIXME:  Send WM_NCCALCSIZE */
    W32kReleaseWindowObject(Window);
    if (bRepaint) NtUserSendMessage(hWnd, WM_PAINT, 0, 0);
    return TRUE;
}

DWORD STDCALL
NtUserQueryWindow(DWORD Unknown0,
		  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserRealChildWindowFromPoint(DWORD Unknown0,
			       DWORD Unknown1,
			       DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

NTSTATUS STDCALL
NtUserRedrawWindow(HWND hWnd, CONST RECT *lprcUpdate, HRGN hrgnUpdate, UINT flags)
{
  RECT SafeUpdateRect;
  NTSTATUS Status;

  if (NULL != lprcUpdate)
    {
      Status = MmCopyFromCaller(&SafeUpdateRect, (PRECT) lprcUpdate, sizeof(RECT));
      if (! NT_SUCCESS(Status))
	{
	  return Status;
	}
    }

  return PaintRedrawWindow(hWnd, NULL == lprcUpdate ? NULL : &SafeUpdateRect, hrgnUpdate,
                           flags, 0) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
;
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
NtUserSetActiveWindow(DWORD Unknown0)
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

DWORD STDCALL
NtUserSetShellWindowEx(DWORD Unknown0,
		       DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowFNID(DWORD Unknown0,
		    DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

LONG STDCALL
NtUserGetWindowLong(HWND hWnd, DWORD Index)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  LONG Result;

  W32kGuiCheck();

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
	  Result = (LONG) WindowObject->WndProc;	
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

  W32kGuiCheck();

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
	  WindowObject->ExStyle = (DWORD) NewValue;
	  break;

	case GWL_STYLE:
	  OldValue = (LONG) WindowObject->Style;
	  WindowObject->Style = (DWORD) NewValue;
	  break;

	case GWL_WNDPROC:
	  /* FIXME: should check if window belongs to current process */
	  OldValue = (LONG) WindowObject->WndProc;
	  WindowObject->WndProc = (WNDPROC) NewValue;
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

DWORD STDCALL
NtUserSetWindowWord(DWORD Unknown0,
		    DWORD Unknown1,
		    DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL
NtUserShowWindow(HWND hWnd,
		 LONG nCmdShow)
{
  W32kGuiCheck();

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
    PWINDOW_OBJECT pWindow = W32kGetWindowObject( hWnd);

    if (!pWindow)
        return FALSE;
    if (pWindow->UpdateRegion)
        NtUserSendMessage( hWnd, WM_PAINT,0,0);
    W32kReleaseWindowObject(pWindow);
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
	return W32kGetDesktopWindow();
}
/* EOF */
