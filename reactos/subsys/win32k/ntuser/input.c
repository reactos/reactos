/* $Id: input.c,v 1.1 2002/01/14 01:11:58 dwelch Exp $
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

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

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

  RtlInitUnicodeString(&KeyboardDeviceName, L"\\??\\Keyboard");
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
		      0);
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

NTSTATUS
InitInputImpl(VOID)
{
  NTSTATUS Status;

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
      NtClose(KeyboardThreadHandle);
    }
  return(STATUS_SUCCESS);
}

NTSTATUS
CleanupInputImp(VOID)
{
  return(STATUS_SUCCESS);
}

/* EOF */
