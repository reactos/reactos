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
/* $Id: input.c,v 1.15 2003/10/18 20:41:10 vizzini Exp $
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
#include <win32k/userobj.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/msgqueue.h>
#include <ddk/ntddmou.h>
#include <include/mouse.h>
#include <include/input.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static HANDLE MouseDeviceHandle;
static HANDLE KeyboardThreadHandle;
static CLIENT_ID KeyboardThreadId;
static HANDLE KeyboardDeviceHandle;
static KEVENT InputThreadsStart;
static BOOLEAN InputThreadsRunning = FALSE;
PUSER_MESSAGE_QUEUE pmPrimitiveMessageQueue = 0;

/* FUNCTIONS *****************************************************************/

VOID STDCALL_FUNC STATIC
KeyboardThreadMain(PVOID StartContext)
{
  UNICODE_STRING KeyboardDeviceName;
  OBJECT_ATTRIBUTES KeyboardObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;

  RtlInitUnicodeStringFromLiteral(&KeyboardDeviceName, L"\\??\\Keyboard");
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
      DbgPrint( "Input Thread Waiting for start event\n" );
      Status = KeWaitForSingleObject(&InputThreadsStart,
				     0,
				     UserMode,
				     TRUE,
				     NULL);
      DbgPrint( "Input Thread Starting...\n" );

      /*
       * Receive and process keyboard input.
       */
      while (InputThreadsRunning)
	{
	  KEY_EVENT_RECORD KeyEvent;
	  LPARAM lParam = 0;
	  BOOLEAN SysKey;
	  
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
	  
    SysKey = KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
	DPRINT( "Key: %s\n", KeyEvent.bKeyDown ? "down" : "up" );

	  /*
	   * Post a keyboard message.
	   */
	  if (KeyEvent.bKeyDown)
	    {
	      lParam = KeyEvent.wRepeatCount | 
		      ((KeyEvent.wVirtualScanCode << 16) & 0x00FF0000) | 0x40000000;

	      /* Bit 24 indicates if this is an extended key */
        if (KeyEvent.dwControlKeyState & ENHANCED_KEY)
          {
            lParam |= (1 << 24);
          }

        if (SysKey)
          {
            lParam |= (1 << 29);  /* Context mode. 1 if ALT if pressed while the key is pressed */
          }

	MsqPostKeyboardMessage(SysKey ? WM_SYSKEYDOWN : WM_KEYDOWN, 
			       KeyEvent.wVirtualKeyCode, 
			       lParam);
	    }
	  else
	    {
	      lParam = KeyEvent.wRepeatCount | 
		      ((KeyEvent.wVirtualScanCode << 16) & 0x00FF0000) | 0xC0000000;

	      /* Bit 24 indicates if this is an extended key */
        if (KeyEvent.dwControlKeyState & ENHANCED_KEY)
          {
            lParam |= (1 << 24);
          }

        if (SysKey)
          {
            lParam |= (1 << 29);  /* Context mode. 1 if ALT if pressed while the key is pressed */
          }
	      MsqPostKeyboardMessage(SysKey ? WM_SYSKEYUP : WM_KEYUP, KeyEvent.wVirtualKeyCode, 
				     lParam);
	    }
	}
      DbgPrint( "Input Thread Stopped...\n" );
    }
}

NTSTATUS STDCALL
NtUserAcquireOrReleaseInputOwnership(BOOLEAN Release)
{
  if (Release && InputThreadsRunning && !pmPrimitiveMessageQueue)
    {
      DbgPrint( "Releasing input: PM = %08x\n", pmPrimitiveMessageQueue );
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
  UNICODE_STRING MouseDeviceName;
  OBJECT_ATTRIBUTES MouseObjectAttributes;
  IO_STATUS_BLOCK Iosb;
  PIRP Irp;
  PFILE_OBJECT FileObject;
  GDI_INFORMATION GdiInfo;
  KEVENT IoEvent;
  PIO_STACK_LOCATION StackPtr;

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

  /*
   * Connect to the mouse class driver.
   * Failures here don't result in a failure return, the system must be
   * able to operate without mouse
   */  
  RtlInitUnicodeStringFromLiteral(&MouseDeviceName, L"\\??\\MouseClass");
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
		      0);
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
       DPRINT1("Win32K: Failed to reference mouse file object.\n");
       NtClose(MouseDeviceHandle);
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

   /* Initialize the default keyboard layout */
   (VOID)W32kGetDefaultKeyLayout();
   
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

/* EOF */
