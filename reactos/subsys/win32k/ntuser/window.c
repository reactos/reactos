/* $Id: window.c,v 1.7 2002/06/18 21:51:11 dwelch Exp $
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

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
W32kOffsetRect(LPRECT Rect, INT x, INT y)
{
  Rect->left += x;
  Rect->right += x;
  Rect->top += y;
  Rect->bottom += y;
  return(TRUE);
}

HWND
W32kGetActiveWindow(VOID)
{
}

WNDPROC
W32kGetWindowProc(HWND Wnd)
{
}

NTSTATUS
InitWindowImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS
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
  UNICODE_STRING WindowName;
  NTSTATUS Status;
  HANDLE Handle;
  POINT MaxSize, MaxPos, MinTrack, MaxTrack;
  CREATESTRUCT Cs;
  LRESULT Result;
  
  DPRINT("NtUserCreateWindowEx\n");

  /* Initialize gui state if necessary. */
  W32kGuiCheck();

  if (!RtlCreateUnicodeString(&WindowName, lpWindowName->Buffer))
    {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return((HWND)0);
    }

  /* FIXME: Validate the parent window. */

  /* Check the class. */
  Status = ClassReferenceClassByNameOrAtom(&ClassObject, lpClassName->Buffer);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&WindowName);
      return((HWND)0);
    }

  /* Check the window station. */
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
    ObmCreateObject(PsGetWin32Process()->HandleTable, &Handle, otWindow, 
		    sizeof(WINDOW_OBJECT));
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
  WindowObject->Style = dwStyle;
  WindowObject->x = x;
  WindowObject->y = y;
  WindowObject->Width = nWidth;
  WindowObject->Height = nHeight;
  WindowObject->Parent = hWndParent;
  WindowObject->Menu = hMenu;
  WindowObject->Instance = hInstance;
  WindowObject->Parameters = lpParam;
  WindowObject->Self = Handle;

  /* FIXME: Add the window parent. */

  RtlInitUnicodeString(&WindowObject->WindowName, WindowName.Buffer);
  RtlFreeUnicodeString(&WindowName);
  
  if (ClassObject->Class.cbWndExtra != 0)
    {
      WindowObject->ExtraData = 
	ExAllocatePool(PagedPool, 
		       ClassObject->Class.cbWndExtra * sizeof(DWORD));
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
  ExAcquireFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
  InsertTailList (&PsGetWin32Process()->WindowListHead, 
		  &WindowObject->ListEntry);
  ExReleaseFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);

  /* FIXME: Maybe allocate a DCE for this window. */

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
      DPRINT("NtUserCreateWindow(): About to send WM_SIZE\n");
      W32kCallWindowProc(NULL, WindowObject->Self, WM_MOVE, 0, lParam);
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
  UNIMPLEMENTED

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
		   PUNICODE_STRING ucClassName,
		   PUNICODE_STRING ucWindowName,
		   DWORD Unknown4)
{
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
NtUserGetWindowDC(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
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

DWORD STDCALL
NtUserMoveWindow(DWORD Unknown0,
		 DWORD Unknown1,
		 DWORD Unknown2,
		 DWORD Unknown3,
		 DWORD Unknown4,
		 DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
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

DWORD STDCALL
NtUserRedrawWindow(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2,
		   DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

UINT STDCALL
NtUserRegisterWindowMessage(LPCWSTR MessageName)
{
  UNIMPLEMENTED

  return(0);
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

DWORD STDCALL
NtUserGetWindowLong(HWND hWnd, DWORD Index)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  DWORD Result;

  DPRINT("NtUserGetWindowLong(hWnd %X, Index %d)\n", hWnd, Index);
  
  W32kGuiCheck();

  Status = ObmReferenceObjectByHandle(PsGetWin32Process()->HandleTable,
				      hWnd,
				      otWindow,
				      (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtUserGetWindowLong(): Bad handle.\n");
      return(0);
    }

  switch (Index)
    {
    case GWL_EXSTYLE:
      {
	Result = (DWORD)WindowObject->ExStyle;
	break;
      }

    case GWL_STYLE:
      {
	Result = (DWORD)WindowObject->Style;
	break;
      }

    case GWL_WNDPROC:
      {
	Result = (DWORD)WindowObject->Class->Class.lpfnWndProc;	
	break;
      }

    default:
      {
	DPRINT1("NtUserGetWindowLong(): Unsupported index %d\n", Index);
	Result = 0;
	break;
      }
    }

  ObmDereferenceObject(WindowObject);
  DPRINT("NtUserGetWindowLong(): %X\n", Result);
  return(Result);
}

DWORD STDCALL
NtUserSetWindowLong(DWORD Unknown0,
		    DWORD Unknown1,
		    DWORD Unknown2,
		    DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowPos(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2,
		   DWORD Unknown3,
		   DWORD Unknown4,
		   DWORD Unknown5,
		   DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
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
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  
  W32kGuiCheck();

  Status = ObmReferenceObjectByHandle(PsGetWin32Process()->HandleTable,
				      hWnd,
				      otWindow,
				      (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }

  return TRUE;
}

DWORD STDCALL
NtUserShowWindowAsync(DWORD Unknown0,
		      DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
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

/* EOF */
