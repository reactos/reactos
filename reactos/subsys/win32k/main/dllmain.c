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
/* $Id: dllmain.c,v 1.40 2003/06/25 22:37:07 gvg Exp $
 *
 *  Entry Point for win32k.sys
 */

#undef WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <ddk/winddi.h>

#include <win32k/win32k.h>

#include <include/winsta.h>
#include <include/class.h>
#include <include/window.h>
#include <include/object.h>
#include <include/input.h>
#include <include/timer.h>
#include <include/text.h>

#define NDEBUG
#include <win32k/debug1.h>

extern SSDT Win32kSSDT[];
extern SSPT Win32kSSPT[];
extern ULONG Win32kNumberOfSysCalls;

PEPROCESS W32kDeviceProcess;


NTSTATUS STDCALL
W32kProcessCallback (struct _EPROCESS *Process,
		     BOOLEAN Create)
{
  PW32PROCESS Win32Process;
  NTSTATUS Status;

#if 0
  DbgPrint ("W32kProcessCallback() called\n");
#endif

  Win32Process = Process->Win32Process;
  if (Create)
    {
#if 0
      DbgPrint ("  Create process\n");
#endif

      InitializeListHead(&Win32Process->ClassListHead);
      ExInitializeFastMutex(&Win32Process->ClassListLock);

      Win32Process->WindowStation = NULL;
      if (Process->Win32WindowStation != NULL)
	{
	  Status = 
	    ValidateWindowStationHandle(Process->Win32WindowStation,
					UserMode,
					GENERIC_ALL,
					&Win32Process->WindowStation);
	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("W32K: Failed to reference a window station for "
		       "process.\n");
	    }
	}
    }
  else
    {
#if 0
      DbgPrint ("  Destroy process\n");
      DbgPrint ("  IRQ level: %lu\n", KeGetCurrentIrql ());
#endif

      CleanupForProcess(Process, Process->UniqueProcessId);
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
W32kThreadCallback (struct _ETHREAD *Thread,
		    BOOLEAN Create)
{
  struct _EPROCESS *Process;
  PW32THREAD Win32Thread;
  NTSTATUS Status;

#if 0
  DbgPrint ("W32kThreadCallback() called\n");
#endif

  Process = Thread->ThreadsProcess;
  Win32Thread = Thread->Win32Thread;
  if (Create)
    {
#if 0
      DbgPrint ("  Create thread\n");
#endif

      Win32Thread->MessageQueue = MsqCreateMessageQueue();
      InitializeListHead(&Win32Thread->WindowListHead);
      ExInitializeFastMutex(&Win32Thread->WindowListLock);

      /* By default threads get assigned their process's desktop. */
      Win32Thread->Desktop = NULL;
      if (Process->Win32Desktop != NULL)
	{
	  Status = ObReferenceObjectByHandle(Process->Win32Desktop,
					     GENERIC_ALL,
					     ExDesktopObjectType,
					     UserMode,
					     (PVOID*)&Win32Thread->Desktop,
					     NULL);
	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("W32K: Failed to reference a desktop for thread.\n");
	    }
	}
    }
  else
    {
#if 0
      DbgPrint ("  Destroy thread\n");
#endif

      RemoveTimersThread(Thread->Cid.UniqueThread);
      DestroyThreadWindows(Thread);
    }

  return STATUS_SUCCESS;
}


/*
 * This definition doesn't work
 */
// WINBOOL STDCALL DllMain(VOID)
NTSTATUS
STDCALL
DllMain (
  IN	PDRIVER_OBJECT	DriverObject,
  IN	PUNICODE_STRING	RegistryPath)
{
  NTSTATUS Status;
  BOOLEAN Result;

  /*
   * Register user mode call interface
   * (system service table index = 1)
   */
  Result = KeAddSystemServiceTable (Win32kSSDT,
				    NULL,
				    Win32kNumberOfSysCalls,
				    Win32kSSPT,
				    1);
  if (Result == FALSE)
    {
      DbgPrint("Adding system services failed!\n");
      return STATUS_UNSUCCESSFUL;
    }

  /*
   * Register our per-process and per-thread structures.
   */
  PsEstablishWin32Callouts (W32kProcessCallback,
			    W32kThreadCallback,
			    0,
			    0,
			    sizeof(W32THREAD),
			    sizeof(W32PROCESS));

  WinPosSetupInternalPos();

  Status = InitWindowStationImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window station implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitClassImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window class implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitWindowImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitInputImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize input implementation.\n");
      return(Status);
    }

  Status = MsqInitializeImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize message queue implementation.\n");
      return(Status);
    }

  Status = InitTimerImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize timer implementation.\n");
      return(Status);
    }

  return STATUS_SUCCESS;
}


BOOLEAN
STDCALL
W32kInitialize (VOID)
{
  DPRINT("in W32kInitialize\n");

  W32kDeviceProcess = PsGetCurrentProcess();

  InitGdiObjectHandleTable ();

  // Initialize FreeType library
  if(!InitFontSupport()) return FALSE;

  // Create stock objects, ie. precreated objects commonly used by win32 applications
  CreateStockObjects();

  return TRUE;
}

/* EOF */
