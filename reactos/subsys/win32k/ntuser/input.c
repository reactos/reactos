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
/* $Id: input.c,v 1.29 2004/04/29 20:41:03 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <internal/safe.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/desktop.h>
#include <include/msgqueue.h>
#include <ddk/ntddmou.h>
#include <include/mouse.h>
#include <include/input.h>
#include <include/hotkey.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define ENABLEMOUSEGDICALLBACK 1

static HANDLE MouseDeviceHandle;
#if !ENABLEMOUSEGDICALLBACK
static HANDLE MouseThreadHandle;
static CLIENT_ID MouseThreadId;
#endif
static HANDLE KeyboardThreadHandle;
static CLIENT_ID KeyboardThreadId;
static HANDLE KeyboardDeviceHandle;
static KEVENT InputThreadsStart;
static BOOLEAN InputThreadsRunning = FALSE;
PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue = 0;

/* FUNCTIONS *****************************************************************/

#if !ENABLEMOUSEGDICALLBACK
VOID STDCALL_FUNC STATIC
MouseThreadMain(PVOID StartContext)
{
  UNICODE_STRING MouseDeviceName;
  OBJECT_ATTRIBUTES MouseObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  
  RtlRosInitUnicodeStringFromLiteral(&MouseDeviceName, L"\\??\\Mouse"); /* FIXME - does win use the same? */
  InitializeObjectAttributes(&MouseObjectAttributes,
                             &MouseDeviceName,
                             0,
                             NULL,
                             NULL);
  Status = NtOpenFile(&MouseDeviceHandle,
                      FILE_ALL_ACCESS,
                      &MouseObjectAttributes,
                      &Iosb,
                      0,
                      FILE_SYNCHRONOUS_IO_ALERT);
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("Win32K: Failed to open mouse.\n");
    return; //(Status);
  }
  
  for(;;)
  {
    /*
     * Wait to start input.
     */
    DPRINT("Mouse Input Thread Waiting for start event\n");
    Status = KeWaitForSingleObject(&InputThreadsStart,
                                   0,
                                   KernelMode,
                                   TRUE,
                                   NULL);
    DPRINT("Mouse Input Thread Starting...\n");
    
    /*
     * Receive and process keyboard input.
     */
    while(InputThreadsRunning)
    {
      MOUSE_INPUT_DATA MouseInput;
      Status = NtReadFile(MouseDeviceHandle,
                          NULL,
                          NULL,
                          NULL,
                          &Iosb,
                          &MouseInput,
                          sizeof(MOUSE_INPUT_DATA),
                          NULL,
                          NULL);
      if(Status == STATUS_ALERTED && !InputThreadsRunning)
      {
        break;
      }
      if(Status == STATUS_PENDING)
      {
        NtWaitForSingleObject(MouseDeviceHandle, FALSE, NULL);
        Status = Iosb.Status;
      }
      if(!NT_SUCCESS(Status))
      {
        DPRINT1("Win32K: Failed to read from mouse.\n");
        return; //(Status);
      }
      DPRINT("MouseEvent\n");
      
      MouseGDICallBack(&MouseInput, sizeof(MOUSE_INPUT_DATA));
    }
    DPRINT("Mouse Input Thread Stopped...\n");
  }
}
#endif

STATIC VOID STDCALL
KeyboardThreadMain(PVOID StartContext)
{
  UNICODE_STRING KeyboardDeviceName;
  OBJECT_ATTRIBUTES KeyboardObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  MSG msg;
  PUSER_MESSAGE_QUEUE FocusQueue;
  struct _ETHREAD *FocusThread;
  
  RtlRosInitUnicodeStringFromLiteral(&KeyboardDeviceName, L"\\??\\Keyboard");
  InitializeObjectAttributes(&KeyboardObjectAttributes,
			     &KeyboardDeviceName,
			     0,
			     NULL,
			     NULL);
  Status = NtOpenFile(&KeyboardDeviceHandle,
		      FILE_ALL_ACCESS,
		      &KeyboardObjectAttributes,
		      &Iosb,
		      0,
		      FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Win32K: Failed to open keyboard.\n");
      return; //(Status);
    }

  for (;;)
    {
      /*
       * Wait to start input.
       */
      DPRINT( "Keyboard Input Thread Waiting for start event\n" );
      Status = KeWaitForSingleObject(&InputThreadsStart,
				     0,
				     KernelMode,
				     TRUE,
				     NULL);
      DPRINT( "Keyboard Input Thread Starting...\n" );

      /*
       * Receive and process keyboard input.
       */
      while (InputThreadsRunning)
	{
	  KEY_EVENT_RECORD KeyEvent;
	  LPARAM lParam = 0;
	  UINT fsModifiers;
	  struct _ETHREAD *Thread;
	  HWND hWnd;
	  int id;

	  Status = NtReadFile (KeyboardDeviceHandle,
			       NULL,
			       NULL,
			       NULL,
			       &Iosb,
			       &KeyEvent,
			       sizeof(KEY_EVENT_RECORD),
			       NULL,
			       NULL);
	  DPRINT( "KeyRaw: %s %04x\n",
		 KeyEvent.bKeyDown ? "down" : "up",
		 KeyEvent.wVirtualScanCode );

	  if (Status == STATUS_ALERTED && !InputThreadsRunning)
	    {
	      break;
	    }
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("Win32K: Failed to read from keyboard.\n");
	      return; //(Status);
	    }

	  DPRINT( "Key: %s\n", KeyEvent.bKeyDown ? "down" : "up" );

	  fsModifiers = 0;
	  if (KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
	    fsModifiers |= MOD_ALT;

	  if (KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
	    fsModifiers |= MOD_CONTROL;

	  if (KeyEvent.dwControlKeyState & SHIFT_PRESSED)
	    fsModifiers |= MOD_SHIFT;

	  /* FIXME: Support MOD_WIN */

	  lParam = KeyEvent.wRepeatCount | 
	    ((KeyEvent.wVirtualScanCode << 16) & 0x00FF0000) | 0x40000000;
	  
	  /* Bit 24 indicates if this is an extended key */
	  if (KeyEvent.dwControlKeyState & ENHANCED_KEY)
	    {
	      lParam |= (1 << 24);
	    }
	  
	  if (fsModifiers & MOD_ALT)
	    {
	      /* Context mode. 1 if ALT if pressed while the key is pressed */
	      lParam |= (1 << 29);
	    }
	  
	  if(KeyEvent.bKeyDown && (fsModifiers & MOD_ALT)) 
	    msg.message = WM_SYSKEYDOWN;
	  else if(KeyEvent.bKeyDown)
	    msg.message = WM_KEYDOWN;
	  else if(fsModifiers & MOD_ALT)
	    msg.message = WM_SYSKEYUP;
	  else
	    msg.message = WM_KEYUP;

	  /* Find the target thread whose locale is in effect */
	  if (!IntGetScreenDC())
	    {
	      FocusQueue = W32kGetPrimitiveMessageQueue();
	    }
	  else
	    {
      	      FocusQueue = IntGetFocusMessageQueue();
	    }

	  if (!FocusQueue) continue;

	  msg.wParam = KeyEvent.wVirtualKeyCode;
	  msg.lParam = lParam;
	  msg.hwnd = FocusQueue->FocusWindow;

	  FocusThread = FocusQueue->Thread;

	  if (FocusThread && FocusThread->Win32Thread && 
	      FocusThread->Win32Thread->KeyboardLayout) 
	    {
	      W32kKeyProcessMessage(&msg,
				    FocusThread->Win32Thread->KeyboardLayout);
	    } 
	  else
	    continue;
	  
	  if (GetHotKey(InputWindowStation,
			fsModifiers,
			msg.wParam,
			&Thread,
			&hWnd,
			&id))
	    {
	      if (KeyEvent.bKeyDown)
		{
		  DPRINT("Hot key pressed (hWnd %lx, id %d)\n", hWnd, id);
		  MsqPostHotKeyMessage (Thread,
					hWnd,
					(WPARAM)id,
					MAKELPARAM((WORD)fsModifiers, 
						   (WORD)msg.wParam));
		}
	    }
	  else 
	    {
	      /*
	       * Post a keyboard message.
	       */
	      MsqPostKeyboardMessage(msg.message,msg.wParam,msg.lParam);
	    }
	}
      DPRINT( "KeyboardInput Thread Stopped...\n" );
    }
}


NTSTATUS STDCALL
NtUserAcquireOrReleaseInputOwnership(BOOLEAN Release)
{
  if (Release && InputThreadsRunning && !pmPrimitiveMessageQueue)
    {
      DPRINT( "Releasing input: PM = %08x\n", pmPrimitiveMessageQueue );
      KeClearEvent(&InputThreadsStart);
      InputThreadsRunning = FALSE;
      
      NtAlertThread(KeyboardThreadHandle);
    }
  else if (!Release && !InputThreadsRunning)
    {
      InputThreadsRunning = TRUE;
      KeSetEvent(&InputThreadsStart, IO_NO_INCREMENT, FALSE);
    }

  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
InitInputImpl(VOID)
{
  NTSTATUS Status;
#if ENABLEMOUSEGDICALLBACK
  UNICODE_STRING MouseDeviceName;
  OBJECT_ATTRIBUTES MouseObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  PIRP Irp;
  PFILE_OBJECT FileObject;
  GDI_INFORMATION GdiInfo;
  KEVENT IoEvent;
  PIO_STACK_LOCATION StackPtr;
#endif

  KeInitializeEvent(&InputThreadsStart, NotificationEvent, FALSE);

  Status = PsCreateSystemThread(&KeyboardThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&KeyboardThreadId,
				KeyboardThreadMain,
				NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Win32K: Failed to create keyboard thread.\n");
  }

  /* Initialize the default keyboard layout */
  (VOID)W32kGetDefaultKeyLayout();
  
#if ENABLEMOUSEGDICALLBACK
  /*
   * Connect to the mouse class driver.
   * Failures here don't result in a failure return, the system must be
   * able to operate without mouse
   */  
  RtlRosInitUnicodeStringFromLiteral(&MouseDeviceName, L"\\??\\MouseClass");
  InitializeObjectAttributes(&MouseObjectAttributes,
			     &MouseDeviceName,
			     0,
			     NULL,
			     NULL);
  Status = ZwOpenFile(&MouseDeviceHandle,
		      FILE_ALL_ACCESS,
		      &MouseObjectAttributes,
		      &Iosb,
		      0,
		      FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Win32K: Failed to open mouse.\n");
      return STATUS_SUCCESS;
    }
  Status = ObReferenceObjectByHandle(MouseDeviceHandle,
				     FILE_READ_DATA | FILE_WRITE_DATA,
				     IoFileObjectType,
				     KernelMode,
				     (PVOID *) &FileObject,
				     NULL);
   
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("Win32K: Failed to reference mouse file object. (0x%X)\n", Status);
       ZwClose(MouseDeviceHandle);
       return STATUS_SUCCESS;
     }
   KeInitializeEvent(&IoEvent, FALSE, NotificationEvent);
   GdiInfo.CallBack = MouseGDICallBack;
   Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_MOUSE_CONNECT,
				       FileObject->DeviceObject,
				       &GdiInfo,
				       sizeof(GdiInfo),
				       NULL,
				       0,
				       TRUE,
				       &FileObject->Event,
				       &Iosb);

   //trigger FileObject/Event dereferencing
   Irp->Tail.Overlay.OriginalFileObject = FileObject;

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   StackPtr->DeviceObject = FileObject->DeviceObject;
   StackPtr->Parameters.DeviceIoControl.InputBufferLength = sizeof(GdiInfo);
   StackPtr->Parameters.DeviceIoControl.OutputBufferLength = 0;

   Status = IoCallDriver(FileObject->DeviceObject, Irp);
   if (Status == STATUS_PENDING)
     {
       KeWaitForSingleObject(&FileObject->Event, Executive, KernelMode, FALSE,
			     NULL);
       Status = Iosb.Status;
     }
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("Win32K: Failed to connect to mouse driver.\n");
       ObDereferenceObject(&FileObject);
       NtClose(MouseDeviceHandle);
       return STATUS_SUCCESS;
     }
#else
  Status = PsCreateSystemThread(&MouseThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&MouseThreadId,
				MouseThreadMain,
				NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Win32K: Failed to create mouse thread.\n");
  }
#endif
  
  return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupInputImp(VOID)
{
  return(STATUS_SUCCESS);
}

BOOL
STDCALL
NtUserDragDetect(
  HWND hWnd,
  LONG x,
  LONG y)
{
  UNIMPLEMENTED
  return 0;
}

BOOL FASTCALL
IntBlockInput(PW32THREAD W32Thread, BOOL BlockIt)
{
  PW32THREAD OldBlock;
  ASSERT(W32Thread);
  
  if(!W32Thread->Desktop || (W32Thread->IsExiting && BlockIt))
  {
    /*
     * fail blocking if exiting the thread
     */
    
    return FALSE;
  }
  
  /*
   * FIXME - check access rights of the window station
   *         e.g. services running in the service window station cannot block input
   */
  if(!ThreadHasInputAccess(W32Thread) ||
     !IntIsActiveDesktop(W32Thread->Desktop))
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  
  ASSERT(W32Thread->Desktop);
  OldBlock = W32Thread->Desktop->BlockInputThread;
  if(OldBlock)
  {
    if(OldBlock != W32Thread)
    {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
    }
    W32Thread->Desktop->BlockInputThread = (BlockIt ? W32Thread : NULL);
    return OldBlock == NULL;
  }
  
  W32Thread->Desktop->BlockInputThread = (BlockIt ? W32Thread : NULL);
  return OldBlock == NULL;
}

BOOL
STDCALL
NtUserBlockInput(
  BOOL BlockIt)
{
  return IntBlockInput(PsGetWin32Thread(), BlockIt);
}

BOOL FASTCALL
IntMouseInput(MOUSEINPUT *mi)
{
  return FALSE;
}

BOOL FASTCALL
IntKeyboardInput(KEYBDINPUT *ki)
{
  return FALSE;
}

UINT
STDCALL
NtUserSendInput(
  UINT nInputs,
  LPINPUT pInput,
  INT cbSize)
{
  PW32THREAD W32Thread;
  UINT cnt;
  
  W32Thread = PsGetWin32Thread();
  ASSERT(W32Thread);
  
  if(!W32Thread->Desktop)
  {
    return 0;
  }
  
  if(!nInputs || !pInput || (cbSize != sizeof(INPUT)))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  /*
   * FIXME - check access rights of the window station
   *         e.g. services running in the service window station cannot block input
   */
  if(!ThreadHasInputAccess(W32Thread) ||
     !IntIsActiveDesktop(W32Thread->Desktop))
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    return 0;
  }
  
  cnt = 0;
  while(nInputs--)
  {
    INPUT SafeInput;
    NTSTATUS Status;
    
    Status = MmCopyFromCaller(&SafeInput, pInput++, sizeof(INPUT));
    if(!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return cnt;
    }
    
    switch(SafeInput.type)
    {
      case INPUT_MOUSE:
        if(IntMouseInput(&SafeInput.mi))
        {
          cnt++;
        }
        break;
      case INPUT_KEYBOARD:
        if(IntKeyboardInput(&SafeInput.ki))
        {
          cnt++;
        }
        break;
      case INPUT_HARDWARE:
        break;
#ifndef NDEBUG
      default:
        DPRINT1("SendInput(): Invalid input type: 0x%x\n", SafeInput.type);
        break;
#endif
    }
  }
  
  return cnt;
}

/* EOF */
