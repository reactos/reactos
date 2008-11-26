/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005 ReactOS Team
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
/* $Id$
 *
 *  Entry Point for win32k.sys
 */

#include <w32k.h>
#include <include/napi.h>

#define NDEBUG
#include <debug.h>

PGDI_HANDLE_TABLE INTERNAL_CALL GDIOBJ_iAllocHandleTable(OUT PSECTION_OBJECT *SectionObject);
BOOL INTERNAL_CALL GDI_CleanupForProcess (struct _EPROCESS *Process);
/* FIXME */
PGDI_HANDLE_TABLE GdiHandleTable = NULL;
PSECTION_OBJECT GdiTableSection = NULL;

LIST_ENTRY GlobalDriverListHead;

HANDLE GlobalUserHeap = NULL;
PSECTION_OBJECT GlobalUserHeapSection = NULL;

PSERVERINFO gpsi = NULL; // Global User Server Information.

HSEMAPHORE hsemDriverMgmt = NULL;

SHORT gusLanguageID;

extern ULONG_PTR Win32kSSDT[];
extern UCHAR Win32kSSPT[];
extern ULONG Win32kNumberOfSysCalls;

NTSTATUS
STDCALL
Win32kProcessCallback(struct _EPROCESS *Process,
                      BOOLEAN Create)
{
    PW32PROCESS Win32Process;
    DECLARE_RETURN(NTSTATUS);

    DPRINT("Enter Win32kProcessCallback\n");
    UserEnterExclusive();

    /* Get the Win32 Process */
    Win32Process = PsGetProcessWin32Process(Process);

    /* Allocate one if needed */
    if (!Win32Process)
    {
        /* FIXME - lock the process */
        Win32Process = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(W32PROCESS),
                                             TAG('W', '3', '2', 'p'));

        if (Win32Process == NULL) RETURN( STATUS_NO_MEMORY);

        RtlZeroMemory(Win32Process, sizeof(W32PROCESS));

        PsSetProcessWin32Process(Process, Win32Process);
        /* FIXME - unlock the process */
    }

  if (Create)
    {
      SIZE_T ViewSize = 0;
      LARGE_INTEGER Offset;
      PVOID UserBase = NULL;
      NTSTATUS Status;
      extern PSECTION_OBJECT GlobalUserHeapSection;
      DPRINT("Creating W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());

      /* map the global heap into the process */
      Offset.QuadPart = 0;
      Status = MmMapViewOfSection(GlobalUserHeapSection,
                                  PsGetCurrentProcess(),
                                  &UserBase,
                                  0,
                                  0,
                                  &Offset,
                                  &ViewSize,
                                  ViewUnmap,
                                  SEC_NO_CHANGE,
                                  PAGE_EXECUTE_READ); /* would prefer PAGE_READONLY, but thanks to RTL heaps... */
      if (!NT_SUCCESS(Status))
      {
          DPRINT1("Failed to map the global heap! 0x%x\n", Status);
          RETURN(Status);
      }
      Win32Process->HeapMappings.Next = NULL;
      Win32Process->HeapMappings.KernelMapping = (PVOID)GlobalUserHeap;
      Win32Process->HeapMappings.UserMapping = UserBase;
      Win32Process->HeapMappings.Count = 1;

      InitializeListHead(&Win32Process->ClassList);

      InitializeListHead(&Win32Process->MenuListHead);

      InitializeListHead(&Win32Process->PrivateFontListHead);
      ExInitializeFastMutex(&Win32Process->PrivateFontListLock);

      InitializeListHead(&Win32Process->DriverObjListHead);
      ExInitializeFastMutex(&Win32Process->DriverObjListLock);

      Win32Process->KeyboardLayout = W32kGetDefaultKeyLayout();

      if(Process->Peb != NULL)
      {
        /* map the gdi handle table to user land */
        Process->Peb->GdiSharedHandleTable = GDI_MapHandleTable(GdiTableSection, Process);
        Process->Peb->GdiDCAttributeList = GDI_BATCH_LIMIT;
      }

      /* setup process flags */
      Win32Process->Flags = 0;
    }
  else
    {
      DPRINT("Destroying W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());
      IntCleanupMenus(Process, Win32Process);
      IntCleanupCurIcons(Process, Win32Process);
      IntEngCleanupDriverObjs(Process, Win32Process);
      CleanupMonitorImpl();

      /* no process windows should exist at this point, or the function will assert! */
      DestroyProcessClasses(Win32Process);

      GDI_CleanupForProcess(Process);

      co_IntGraphicsCheck(FALSE);

      /*
       * Deregister logon application automatically
       */
      if(LogonProcess == Win32Process)
      {
        LogonProcess = NULL;
      }

      if (Win32Process->ProcessInfo != NULL)
      {
          UserHeapFree(Win32Process->ProcessInfo);
          Win32Process->ProcessInfo = NULL;
      }
    }

  RETURN( STATUS_SUCCESS);

CLEANUP:
  UserLeave();
  DPRINT("Leave Win32kProcessCallback, ret=%i\n",_ret_);
  END_CLEANUP;
}


NTSTATUS
STDCALL
Win32kThreadCallback(struct _ETHREAD *Thread,
                     PSW32THREADCALLOUTTYPE Type)
{
    struct _EPROCESS *Process;
    PTHREADINFO Win32Thread;
    DECLARE_RETURN(NTSTATUS);

    DPRINT("Enter Win32kThreadCallback\n");
    UserEnterExclusive();

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread */
    Win32Thread = PsGetThreadWin32Thread(Thread);

    /* Allocate one if needed */
    if (!Win32Thread)
    {
        /* FIXME - lock the process */
        Win32Thread = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(THREADINFO),
                                            TAG('W', '3', '2', 't'));

        if (Win32Thread == NULL) RETURN( STATUS_NO_MEMORY);

        RtlZeroMemory(Win32Thread, sizeof(THREADINFO));

        PsSetThreadWin32Thread(Thread, Win32Thread);
        /* FIXME - unlock the process */
    }
  if (Type == PsW32ThreadCalloutInitialize)
    {
      HWINSTA hWinSta = NULL;
      PTEB pTeb;
      HDESK hDesk = NULL;
      NTSTATUS Status;
      PUNICODE_STRING DesktopPath;
      PRTL_USER_PROCESS_PARAMETERS ProcessParams = (Process->Peb ? Process->Peb->ProcessParameters : NULL);

      DPRINT("Creating W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

      InitializeListHead(&Win32Thread->WindowListHead);
      InitializeListHead(&Win32Thread->W32CallbackListHead);
      InitializeListHead(&Win32Thread->PtiLink);

      /*
       * inherit the thread desktop and process window station (if not yet inherited) from the process startup
       * info structure. See documentation of CreateProcess()
       */
      DesktopPath = (ProcessParams ? ((ProcessParams->DesktopInfo.Length > 0) ? &ProcessParams->DesktopInfo : NULL) : NULL);
      Status = IntParseDesktopPath(Process,
                                   DesktopPath,
                                   &hWinSta,
                                   &hDesk);
      if(NT_SUCCESS(Status))
      {
        if(hWinSta != NULL)
        {
          if(Process != CsrProcess)
          {
            HWINSTA hProcessWinSta = (HWINSTA)InterlockedCompareExchangePointer((PVOID)&Process->Win32WindowStation, (PVOID)hWinSta, NULL);
            if(hProcessWinSta != NULL)
            {
              /* our process is already assigned to a different window station, we don't need the handle anymore */
              NtClose(hWinSta);
            }
          }
          else
          {
            NtClose(hWinSta);
          }
        }

        if (hDesk != NULL)
        {
          PDESKTOP DesktopObject;
          Win32Thread->Desktop = NULL;
          Status = ObReferenceObjectByHandle(hDesk,
                                             0,
                                             ExDesktopObjectType,
                                             KernelMode,
                                             (PVOID*)&DesktopObject,
                                             NULL);
          NtClose(hDesk);
          if(NT_SUCCESS(Status))
          {
            if (!IntSetThreadDesktop(DesktopObject,
                                     FALSE))
            {
              DPRINT1("Unable to set thread desktop\n");
            }
          }
          else
          {
            DPRINT1("Unable to reference thread desktop handle 0x%x\n", hDesk);
          }
        }
      }
      Win32Thread->IsExiting = FALSE;
      co_IntDestroyCaret(Win32Thread);
      Win32Thread->ppi = PsGetCurrentProcessWin32Process();
      pTeb = NtCurrentTeb();
      if (pTeb)
      {
          Win32Thread->pClientInfo = (PCLIENTINFO)pTeb->Win32ClientInfo;
          Win32Thread->pClientInfo->pClientThreadInfo = NULL;
      }
      Win32Thread->MessageQueue = MsqCreateMessageQueue(Thread);
      Win32Thread->KeyboardLayout = W32kGetDefaultKeyLayout();
      if (Win32Thread->ThreadInfo)
      {
          Win32Thread->ThreadInfo->ClientThreadInfo.dwcPumpHook = 0;
          Win32Thread->pClientInfo->pClientThreadInfo = &Win32Thread->ThreadInfo->ClientThreadInfo;
      }
    }
  else
    {
      PSINGLE_LIST_ENTRY e;

      DPRINT("Destroying W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

      Win32Thread->IsExiting = TRUE;
      HOOK_DestroyThreadHooks(Thread);
      UnregisterThreadHotKeys(Thread);
      /* what if this co_ func crash in umode? what will clean us up then? */
      co_DestroyThreadWindows(Thread);
      IntBlockInput(Win32Thread, FALSE);
      MsqDestroyMessageQueue(Win32Thread->MessageQueue);
      IntCleanupThreadCallbacks(Win32Thread);

      /* cleanup user object references stack */
      e = PopEntryList(&Win32Thread->ReferencesList);
      while (e)
      {
         PUSER_REFERENCE_ENTRY ref = CONTAINING_RECORD(e, USER_REFERENCE_ENTRY, Entry);
         DPRINT("thread clean: remove reference obj 0x%x\n",ref->obj);
         UserDereferenceObject(ref->obj);

         e = PopEntryList(&Win32Thread->ReferencesList);
      }

      IntSetThreadDesktop(NULL,
                          TRUE);

      if (Win32Thread->ThreadInfo != NULL)
      {
          UserHeapFree(Win32Thread->ThreadInfo);
          Win32Thread->ThreadInfo = NULL;
      }

      PsSetThreadWin32Thread(Thread, NULL);
    }

  RETURN( STATUS_SUCCESS);

CLEANUP:
  UserLeave();
  DPRINT("Leave Win32kThreadCallback, ret=%i\n",_ret_);
  END_CLEANUP;
}

/* Only used in ntuser/input.c KeyboardThreadMain(). If it's
   not called there anymore, please delete */
NTSTATUS
Win32kInitWin32Thread(PETHREAD Thread)
{
  PEPROCESS Process;

  Process = Thread->ThreadsProcess;

  if (Process->Win32Process == NULL)
    {
      /* FIXME - lock the process */
      Process->Win32Process = ExAllocatePool(NonPagedPool, sizeof(W32PROCESS));

      if (Process->Win32Process == NULL)
	return STATUS_NO_MEMORY;

      RtlZeroMemory(Process->Win32Process, sizeof(W32PROCESS));
      /* FIXME - unlock the process */

      Win32kProcessCallback(Process, TRUE);
    }

  if (Thread->Tcb.Win32Thread == NULL)
    {
      Thread->Tcb.Win32Thread = ExAllocatePool (NonPagedPool, sizeof(THREADINFO));
      if (Thread->Tcb.Win32Thread == NULL)
	return STATUS_NO_MEMORY;

      RtlZeroMemory(Thread->Tcb.Win32Thread, sizeof(THREADINFO));

      Win32kThreadCallback(Thread, PsW32ThreadCalloutInitialize);
    }

  return(STATUS_SUCCESS);
}


/*
 * This definition doesn't work
 */
NTSTATUS STDCALL
DriverEntry (
  IN	PDRIVER_OBJECT	DriverObject,
  IN	PUNICODE_STRING	RegistryPath)
{
  NTSTATUS Status;
  BOOLEAN Result;
  WIN32_CALLOUTS_FPNS CalloutData = {0};
  PVOID GlobalUserHeapBase = NULL;

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
      DPRINT1("Adding system services failed!\n");
      return STATUS_UNSUCCESSFUL;
    }

    /*
     * Register Object Manager Callbacks
     */
    CalloutData.WindowStationParseProcedure = IntWinStaObjectParse;
    CalloutData.WindowStationDeleteProcedure = IntWinStaObjectDelete;
    CalloutData.DesktopDeleteProcedure = IntDesktopObjectDelete;
    CalloutData.ProcessCallout = Win32kProcessCallback;
    CalloutData.ThreadCallout = Win32kThreadCallback;
    CalloutData.BatchFlushRoutine = NtGdiFlushUserBatch;

    /*
     * Register our per-process and per-thread structures.
     */
    PsEstablishWin32Callouts((PWIN32_CALLOUTS_FPNS)&CalloutData);

    GlobalUserHeap = UserCreateHeap(&GlobalUserHeapSection,
                                    &GlobalUserHeapBase,
                                    1 * 1024 * 1024); /* FIXME - 1 MB for now... */
    if (GlobalUserHeap == NULL)
    {
        DPRINT1("Failed to initialize the global heap!\n");
        return STATUS_UNSUCCESSFUL;
    }

  /* Initialize a list of loaded drivers in Win32 subsystem */
  InitializeListHead(&GlobalDriverListHead);

  if(!hsemDriverMgmt) hsemDriverMgmt = EngCreateSemaphore();

  GdiHandleTable = GDIOBJ_iAllocHandleTable(&GdiTableSection);
  if (GdiHandleTable == NULL)
  {
      DPRINT1("Failed to initialize the GDI handle table.\n");
      return STATUS_UNSUCCESSFUL;
  }

  Status = InitUserImpl();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to initialize user implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitHotkeyImpl();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to initialize hotkey implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitWindowStationImpl();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to initialize window station implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitDesktopImpl();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to initialize desktop implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitWindowImpl();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to initialize window implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitMenuImpl();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to initialize menu implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = InitInputImpl();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to initialize input implementation.\n");
      return(Status);
    }

  Status = InitKeyboardImpl();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to initialize keyboard implementation.\n");
      return(Status);
    }

  Status = InitMonitorImpl();
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Failed to initialize monitor implementation!\n");
      return STATUS_UNSUCCESSFUL;
    }

  Status = MsqInitializeImpl();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to initialize message queue implementation.\n");
      return(Status);
    }

  Status = InitTimerImpl();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to initialize timer implementation.\n");
      return(Status);
    }

  Status = InitAcceleratorImpl();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to initialize accelerator implementation.\n");
      return(Status);
    }

  Status = InitGuiCheckImpl();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to initialize GUI check implementation.\n");
      return(Status);
    }

  Status = InitDcImpl();
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to initialize Device context implementation!\n");
    return STATUS_UNSUCCESSFUL;
  }

  /* Initialize FreeType library */
  if (! InitFontSupport())
    {
      DPRINT1("Unable to initialize font support\n");
      return STATUS_UNSUCCESSFUL;
    }

  /* Create stock objects, ie. precreated objects commonly
     used by win32 applications */
  CreateStockObjects();
  CreateSysColorObjects();

  gusLanguageID = IntGdiGetLanguageID();

  return STATUS_SUCCESS;
}

/* EOF */
