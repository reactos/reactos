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
/* $Id: dllmain.c,v 1.74 2004/05/21 10:09:31 weiden Exp $
 *
 *  Entry Point for win32k.sys
 */
#include <w32k.h>
#include <roscfg.h>

#ifdef __USE_W32API
typedef NTSTATUS (STDCALL *PW32_PROCESS_CALLBACK)(
   struct _EPROCESS *Process,
   BOOLEAN Create);

typedef NTSTATUS (STDCALL *PW32_THREAD_CALLBACK)(
   struct _ETHREAD *Thread,
   BOOLEAN Create);

VOID STDCALL
PsEstablishWin32Callouts(
   PW32_PROCESS_CALLBACK W32ProcessCallback,
   PW32_THREAD_CALLBACK W32ThreadCallback,
   PVOID Param3,
   PVOID Param4,
   ULONG W32ThreadSize,
   ULONG W32ProcessSize);
#endif

extern SSDT Win32kSSDT[];
extern SSPT Win32kSSPT[];
extern ULONG Win32kNumberOfSysCalls;

NTSTATUS STDCALL
Win32kProcessCallback (struct _EPROCESS *Process,
		     BOOLEAN Create)
{
  PW32PROCESS Win32Process;
  NTSTATUS Status;

#if 0
  DbgPrint ("Win32kProcessCallback() called\n");
#endif

  Win32Process = Process->Win32Process;
  if (Create)
    {
#if 0
      DbgPrint ("  Create process\n");
#endif

      InitializeListHead(&Win32Process->ClassListHead);
      ExInitializeFastMutex(&Win32Process->ClassListLock);
      
      InitializeListHead(&Win32Process->MenuListHead);
      ExInitializeFastMutex(&Win32Process->MenuListLock);      

      InitializeListHead(&Win32Process->PrivateFontListHead);
      ExInitializeFastMutex(&Win32Process->PrivateFontListLock);
      
      InitializeListHead(&Win32Process->CursorIconListHead);
      ExInitializeFastMutex(&Win32Process->CursorIconListLock);

      Win32Process->KeyboardLayout = W32kGetDefaultKeyLayout();
      Win32Process->WindowStation = NULL;
      if (Process->Win32WindowStation != NULL)
	{
	  Status = 
	    IntValidateWindowStationHandle(Process->Win32WindowStation,
					   UserMode,
					   GENERIC_ALL,
					   &Win32Process->WindowStation);
	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("Win32K: Failed to reference a window station for "
		       "process.\n");
	    }
	}
      
      /* setup process flags */
      Win32Process->Flags = 0;
    }
  else
    {
#if 0
      DbgPrint ("  Destroy process\n");
      DbgPrint ("  IRQ level: %lu\n", KeGetCurrentIrql ());
#endif
	  IntRemoveProcessWndProcHandles((HANDLE)Process->UniqueProcessId);
      IntCleanupMenus(Process, Win32Process);
      IntCleanupCurIcons(Process, Win32Process);

      CleanupForProcess(Process, Process->UniqueProcessId);

      IntGraphicsCheck(FALSE);
      
      /*
       * Deregister logon application automatically
       */
      if(LogonProcess == Win32Process)
      {
        LogonProcess = NULL;
      }
    }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
Win32kThreadCallback (struct _ETHREAD *Thread,
		    BOOLEAN Create)
{
  struct _EPROCESS *Process;
  PW32THREAD Win32Thread;
  NTSTATUS Status;

#if 0
  DbgPrint ("Win32kThreadCallback() called\n");
#endif

  Process = Thread->ThreadsProcess;
  Win32Thread = Thread->Win32Thread;
  if (Create)
    {
#if 0
      DbgPrint ("  Create thread\n");
#endif

      Win32Thread->IsExiting = FALSE;
      IntDestroyCaret(Win32Thread);
      Win32Thread->MessageQueue = MsqCreateMessageQueue(Thread);
      Win32Thread->KeyboardLayout = W32kGetDefaultKeyLayout();
      Win32Thread->MessagePumpHookValue = 0;
      InitializeListHead(&Win32Thread->WindowListHead);
      ExInitializeFastMutex(&Win32Thread->WindowListLock);

      /* By default threads get assigned their process's desktop. */
      Win32Thread->Desktop = NULL;
      Win32Thread->hDesktop = NULL;
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
	      DbgPrint("Win32K: Failed to reference a desktop for thread.\n");
	    }
	  
	  Win32Thread->hDesktop = Process->Win32Desktop;
	}
    }
  else
    {
#if 0
      DbgPrint ("  Destroy thread\n");
#endif

      Win32Thread->IsExiting = TRUE;
      HOOK_DestroyThreadHooks(Thread);
      RemoveTimersThread(Thread->Cid.UniqueThread);
      UnregisterThreadHotKeys(Thread);
      DestroyThreadWindows(Thread);
      IntBlockInput(Win32Thread, FALSE);
    }

  return STATUS_SUCCESS;
}


/*
 * This definition doesn't work
 */
// BOOL STDCALL DllMain(VOID)
NTSTATUS STDCALL
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
  PsEstablishWin32Callouts (Win32kProcessCallback,
			    Win32kThreadCallback,
			    0,
			    0,
			    sizeof(W32THREAD),
			    sizeof(W32PROCESS));
  
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

  Status = InitDesktopImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window station implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitWindowImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize window implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitMenuImpl();
  if (!NT_SUCCESS(Status))
  {
    DbgPrint("Failed to initialize menu implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitInputImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize input implementation.\n");
      return(Status);
    }

  Status = InitKeyboardImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize keyboard implementation.\n");
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

  Status = InitAcceleratorImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize accelerator implementation.\n");
      return(Status);
    }

  InitGdiObjectHandleTable ();

  /* Initialize FreeType library */
  if (! InitFontSupport())
    {
      DPRINT1("Unable to initialize font support\n");
      return STATUS_UNSUCCESSFUL;
    }

  /* Create stock objects, ie. precreated objects commonly
     used by win32 applications */
  CreateStockObjects();
  
  PREPARE_TESTS

  return STATUS_SUCCESS;
}


BOOLEAN STDCALL
Win32kInitialize (VOID)
{
  return TRUE;
}

/* EOF */
