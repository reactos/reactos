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
/* $Id: input.c,v 1.6 2003/05/18 17:16:17 ea Exp $
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

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static HANDLE MouseDeviceHandle;
static HANDLE KeyboardThreadHandle;
static CLIENT_ID KeyboardThreadId;
static HANDLE KeyboardDeviceHandle;
static KEVENT InputThreadsStart;
static BOOLEAN InputThreadsRunning = FALSE;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL STATIC
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
      DbgPrint("W32K: Failed to open keyboard.\n");
      return(Status);
    }

  for (;;)
    {
      /*
       * Wait to start input.
       */
      Status = KeWaitForSingleObject(&InputThreadsStart,
				     0,
				     UserMode,
				     TRUE,
				     NULL);
      /*
       * Receive and process keyboard input.
       */
      while (InputThreadsRunning)
	{
	  KEY_EVENT_RECORD KeyEvent;
	  LPARAM lParam;
	  
	  Status = NtReadFile (KeyboardDeviceHandle, 
			       NULL,
			       NULL,
			       NULL,
			       &Iosb,
			       &KeyEvent,
			       sizeof(KEY_EVENT_RECORD),
			       NULL,
			       NULL);
	  if (Status == STATUS_ALERTED && !InputThreadsRunning)
	    {
	      break;
	    }
	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("W32K: Failed to read from keyboard.\n");
	      return(Status);
	    }
	  
	  /*
	   * Post a keyboard message.
	   */
	  if (KeyEvent.bKeyDown)
	    {
	      /* FIXME: Bit 24 indicates if this is an extended key. */
	      lParam = KeyEvent.wRepeatCount | 
		((KeyEvent.wVirtualScanCode << 16) & 0x00FF0000) | 0x40000000;
	      MsqPostKeyboardMessage(WM_KEYDOWN, KeyEvent.wVirtualKeyCode, 
				     lParam);
	    }
	  else
	    {
	      /* FIXME: Bit 24 indicates if this is an extended key. */
	      lParam = KeyEvent.wRepeatCount | 
		((KeyEvent.wVirtualScanCode << 16) & 0x00FF0000) | 0xC0000000;
	      MsqPostKeyboardMessage(WM_KEYUP, KeyEvent.wVirtualKeyCode, 
				     lParam);
	    }
	}
    }
}

NTSTATUS STDCALL
NtUserAcquireOrReleaseInputOwnership(BOOLEAN Release)
{
  if (Release && InputThreadsRunning)
    {
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
      DbgPrint("W32K: Failed to create keyboard thread.\n");
    }

  /*
   * Connect to the mouse class driver.
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
      DbgPrint("W32K: Failed to open mouse.\n");
      return(Status);
    }
  Status = ObReferenceObjectByHandle(MouseDeviceHandle,
				     FILE_READ_DATA | FILE_WRITE_DATA,
				     IoFileObjectType,
				     KernelMode,
				     (PVOID *) &FileObject,
				     NULL);
   
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("W32K: Failed to reference mouse file object.\n");
       return(Status);
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
       DbgPrint("W32K: Failed to connect to mouse driver.\n");
       return(Status);
     }
   
   return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupInputImp(VOID)
{
  return(STATUS_SUCCESS);
}

/* EOF */
