/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/process.c
 * PURPOSE:         Process managment
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PEPROCESS EXPORTED PsInitialSystemProcess = NULL;
PEPROCESS PsIdleProcess = NULL;
POBJECT_TYPE EXPORTED PsProcessType = NULL;
extern PHANDLE_TABLE PspCidTable;

EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;

LIST_ENTRY PsActiveProcessHead;
FAST_MUTEX PspActiveProcessMutex;
LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;

/* INTERNAL FUNCTIONS *****************************************************************/

NTSTATUS
PsLockProcess(PEPROCESS Process, BOOLEAN Timeout)
{
  ULONG Attempts = 0;
  PKTHREAD PrevLockOwner;
  NTSTATUS Status = STATUS_UNSUCCESSFUL;
  PLARGE_INTEGER Delay = (Timeout ? &PsLockTimeout : NULL);
  PKTHREAD CallingThread = KeGetCurrentThread();

  PAGED_CODE();

  KeEnterCriticalRegion();

  for(;;)
  {
    PrevLockOwner = (PKTHREAD)InterlockedCompareExchangePointer(
      &Process->LockOwner, CallingThread, NULL);
    if(PrevLockOwner == NULL || PrevLockOwner == CallingThread)
    {
      /* we got the lock or already locked it */
      if(InterlockedIncrementUL(&Process->LockCount) == 1)
      {
        KeClearEvent(&Process->LockEvent);
      }

      return STATUS_SUCCESS;
    }
    else
    {
      if(++Attempts > 2)
      {
        Status = KeWaitForSingleObject(&Process->LockEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       Delay);
        if(!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
        {
#ifndef NDEBUG
          if(Status == STATUS_TIMEOUT)
          {
            DPRINT1("PsLockProcess(0x%x) timed out!\n", Process);
          }
#endif
          KeLeaveCriticalRegion();
          break;
        }
      }
      else
      {
        KeDelayExecutionThread(KernelMode, FALSE, &ShortPsLockDelay);
      }
    }
  }

  return Status;
}

VOID
PsUnlockProcess(PEPROCESS Process)
{
  PAGED_CODE();

  ASSERT(Process->LockOwner == KeGetCurrentThread());

  if(InterlockedDecrementUL(&Process->LockCount) == 0)
  {
    InterlockedExchangePointer(&Process->LockOwner, NULL);
    KeSetEvent(&Process->LockEvent, IO_NO_INCREMENT, FALSE);
  }

  KeLeaveCriticalRegion();
}

PEPROCESS
STDCALL
PsGetNextProcess(PEPROCESS OldProcess)
{
    PEPROCESS NextProcess;
    NTSTATUS Status;

    /* Check if we have a previous process */
    if (OldProcess == NULL)
    {
        /* We don't, start with the Idle Process */
        Status = ObReferenceObjectByPointer(PsIdleProcess,
                                            PROCESS_ALL_ACCESS,
                                            PsProcessType,
                                            KernelMode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PsGetNextProcess(): ObReferenceObjectByPointer failed for PsIdleProcess\n");
            KEBUGCHECK(0);
        }

        return PsIdleProcess;
    }

    /* Acquire the Active Process Lock */
    ExAcquireFastMutex(&PspActiveProcessMutex);

    /* Start at the previous process */
    NextProcess = OldProcess;

    /* Loop until we fail */
    while (1)
    {
        /* Get the Process Link */
        PLIST_ENTRY Flink = (NextProcess == PsIdleProcess ? PsActiveProcessHead.Flink :
                             NextProcess->ActiveProcessLinks.Flink);

        /* Move to the next Process if we're not back at the beginning */
        if (Flink != &PsActiveProcessHead)
        {
            NextProcess = CONTAINING_RECORD(Flink, EPROCESS, ActiveProcessLinks);
        }
        else
        {
            NextProcess = NULL;
            break;
        }

        /* Reference the Process */
        Status = ObReferenceObjectByPointer(NextProcess,
                                            PROCESS_ALL_ACCESS,
                                            PsProcessType,
                                            KernelMode);

        /* Exit the loop if the reference worked, keep going if there's an error */
        if (NT_SUCCESS(Status)) break;
    }

    /* Release the lock */
    ExReleaseFastMutex(&PspActiveProcessMutex);

    /* Reference the Process we had referenced earlier */
    ObDereferenceObject(OldProcess);
    return(NextProcess);
}

NTSTATUS
STDCALL
PspCreateProcess(OUT PHANDLE ProcessHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                 IN HANDLE ParentProcess  OPTIONAL,
                 IN BOOLEAN InheritObjectTable,
                 IN HANDLE SectionHandle  OPTIONAL,
                 IN HANDLE DebugPort  OPTIONAL,
                 IN HANDLE ExceptionPort  OPTIONAL)
{
    HANDLE hProcess;
    PEPROCESS Process;
    PEPROCESS pParentProcess;
    PEPORT pDebugPort = NULL;
    PEPORT pExceptionPort = NULL;
    PSECTION_OBJECT SectionObject = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PHYSICAL_ADDRESS DirectoryTableBase;
    KAFFINITY Affinity;
    HANDLE_TABLE_ENTRY CidEntry;
    DirectoryTableBase.QuadPart = (ULONGLONG)0;

    DPRINT("PspCreateProcess(ObjectAttributes %x)\n", ObjectAttributes);

    /* Reference the Parent if there is one */
    if(ParentProcess != NULL)
    {
        Status = ObReferenceObjectByHandle(ParentProcess,
                                           PROCESS_CREATE_PROCESS,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&pParentProcess,
                                           NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference the parent process: Status: 0x%x\n", Status);
            return(Status);
        }

        /* Inherit Parent process's Affinity. */
        Affinity = pParentProcess->Pcb.Affinity;

    }
    else
    {
        pParentProcess = NULL;
        Affinity = KeActiveProcessors;
    }

    /* Add the debug port */
    if (DebugPort != NULL)
    {
        Status = ObReferenceObjectByHandle(DebugPort,
                                           PORT_ALL_ACCESS,
                                           LpcPortObjectType,
                                           PreviousMode,
                                           (PVOID*)&pDebugPort,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
                DPRINT1("Failed to reference the debug port: Status: 0x%x\n", Status);
                goto exitdereferenceobjects;
        }
    }

    /* Add the exception port */
    if (ExceptionPort != NULL)
    {
        Status = ObReferenceObjectByHandle(ExceptionPort,
                                           PORT_ALL_ACCESS,
                                           LpcPortObjectType,
                                           PreviousMode,
                                           (PVOID*)&pExceptionPort,
                                           NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference the exception port: Status: 0x%x\n", Status);
            goto exitdereferenceobjects;
        }
    }

    /* Add the Section */
    if (SectionHandle != NULL)
    {
        Status = ObReferenceObjectByHandle(SectionHandle,
                                           0,
                                           MmSectionObjectType,
                                           PreviousMode,
                                           (PVOID*)&SectionObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference process image section: Status: 0x%x\n", Status);
            goto exitdereferenceobjects;
        }
    }

    /* Create the Object */
    DPRINT("Creating Process Object\n");
    Status = ObCreateObject(PreviousMode,
                            PsProcessType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(EPROCESS),
                            0,
                            0,
                            (PVOID*)&Process);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create process object, Status: 0x%x\n", Status);
        goto exitdereferenceobjects;
    }

    /* Clean up the Object */
    DPRINT("Cleaning Process Object\n");
    RtlZeroMemory(Process, sizeof(EPROCESS));

    /* Inherit stuff from the Parent since we now have the object created */
    if (pParentProcess)
    {
        Process->InheritedFromUniqueProcessId = pParentProcess->UniqueProcessId;
        Process->Session = pParentProcess->Session;
    }

    /* Set up the Quota Block from the Parent */
    PspInheritQuota(Process, pParentProcess);

    /* FIXME: Set up Dos Device Map from the Parent
    ObInheritDeviceMap(Parent, Process) */

    /* Set the Process' LPC Ports */
    Process->DebugPort = pDebugPort;
    Process->ExceptionPort = pExceptionPort;

    /* Setup the Lock Event */
    DPRINT("Initialzing Process Lock\n");
    KeInitializeEvent(&Process->LockEvent, SynchronizationEvent, FALSE);

    /* Setup the Thread List Head */
    DPRINT("Initialzing Process ThreadListHead\n");
    InitializeListHead(&Process->ThreadListHead);

    /* Create or Clone the Handle Table */
    DPRINT("Initialzing Process Handle Table\n");
    ObCreateHandleTable(pParentProcess, InheritObjectTable,  Process);
    DPRINT("Handle Table: %x\n", Process->ObjectTable);

    /* Set Process's Directory Base */
    DPRINT("Initialzing Process Directory Base\n");
    MmCopyMmInfo(pParentProcess ? pParentProcess : PsInitialSystemProcess,
                 Process,
                 &DirectoryTableBase);

    /* Now initialize the Kernel Process */
    DPRINT("Initialzing Kernel Process\n");
    KeInitializeProcess(&Process->Pcb,
                        PROCESS_PRIO_NORMAL,
                        Affinity,
                        DirectoryTableBase);

    /* Duplicate Parent Token */
    DPRINT("Initialzing Process Token\n");
    Status = PspInitializeProcessSecurity(Process, pParentProcess);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("PspInitializeProcessSecurity failed (Status %x)\n", Status);
        ObDereferenceObject(Process);
        goto exitdereferenceobjects;
    }

    /* Create the Process' Address Space */
    DPRINT("Initialzing Process Address Space\n");
    Status = MmCreateProcessAddressSpace(Process, SectionObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Address Space\n");
        ObDereferenceObject(Process);
        goto exitdereferenceobjects;
    }

    if (SectionObject)
    {
        /* Map the System Dll */
        DPRINT("Mapping System DLL\n");
        PspMapSystemDll(Process, NULL);
    }

    /* Create a handle for the Process */
    DPRINT("Initialzing Process CID Handle\n");
    CidEntry.u1.Object = Process;
    CidEntry.u2.GrantedAccess = 0;
    Process->UniqueProcessId = ExCreateHandle(PspCidTable, &CidEntry);
    DPRINT("Created CID: %d\n", Process->UniqueProcessId);
    if(!Process->UniqueProcessId)
    {
        DPRINT1("Failed to create CID handle\n");
        ObDereferenceObject(Process);
        goto exitdereferenceobjects;
    }

    /* FIXME: Insert into Job Object */

    /* Create PEB only for User-Mode Processes */
    if (pParentProcess)
    {
        DPRINT("Creating PEB\n");
        Status = MmCreatePeb(Process);
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("NtCreateProcess() Peb creation failed: Status %x\n",Status);
            ObDereferenceObject(Process);
            goto exitdereferenceobjects;
        }

        /* Let's take advantage of this time to kill the reference too */
        ObDereferenceObject(pParentProcess);
        pParentProcess = NULL;
    }

    /* W00T! The process can now be activated */
    DPRINT("Inserting into Active Process List\n");
    ExAcquireFastMutex(&PspActiveProcessMutex);
    InsertTailList(&PsActiveProcessHead, &Process->ActiveProcessLinks);
    ExReleaseFastMutex(&PspActiveProcessMutex);

    /* FIXME: SeCreateAccessStateEx */

    /* Insert the Process into the Object Directory */
    DPRINT("Inserting Process Object\n");
    Status = ObInsertObject(Process,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &hProcess);
    if (!NT_SUCCESS(Status))
    {
       DPRINT1("Could not get a handle to the Process Object\n");
       ObDereferenceObject(Process);
       goto exitdereferenceobjects;
    }

    /* Set the Creation Time */
    KeQuerySystemTime(&Process->CreateTime);

    DPRINT("Done. Returning handle: %x\n", hProcess);
    _SEH_TRY
    {
       *ProcessHandle = hProcess;
    }
    _SEH_HANDLE
    {
       Status = _SEH_GetExceptionCode();
    } _SEH_END;

    /* FIXME: ObGetObjectSecurity(Process, &SecurityDescriptor)
              SeAccessCheck
    */
    ObDereferenceObject(Process);
    return Status;

exitdereferenceobjects:
    if(SectionObject != NULL) ObDereferenceObject(SectionObject);
    if(pExceptionPort != NULL) ObDereferenceObject(pExceptionPort);
    if(pDebugPort != NULL) ObDereferenceObject(pDebugPort);
    if(pParentProcess != NULL) ObDereferenceObject(pParentProcess);
    return Status;
}

/* PUBLIC FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsCreateSystemProcess(PHANDLE ProcessHandle,
                      ACCESS_MASK DesiredAccess,
                      POBJECT_ATTRIBUTES ObjectAttributes)
{
    return PspCreateProcess(ProcessHandle,
                            DesiredAccess,
                            ObjectAttributes,
                            NULL, /* no parent process */
                            FALSE,
                            NULL,
                            NULL,
                            NULL);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsLookupProcessByProcessId(IN HANDLE ProcessId,
                           OUT PEPROCESS *Process)
{
    PHANDLE_TABLE_ENTRY CidEntry;
    PEPROCESS FoundProcess;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PAGED_CODE();
    
    KeEnterCriticalRegion();

    /* Get the CID Handle Entry */
    if ((CidEntry = ExMapHandleToPointer(PspCidTable,
                                         ProcessId)))
    {
        /* Get the Process */
        FoundProcess = CidEntry->u1.Object;

        /* Make sure it's really a process */
        if (FoundProcess->Pcb.Header.Type == ProcessObject)
        {
            /* Reference and return it */
            ObReferenceObject(FoundProcess);
            *Process = FoundProcess;
            Status = STATUS_SUCCESS;
        }

        /* Unlock the Entry */
        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }
    
    KeLeaveCriticalRegion();

    /* Return to caller */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsLookupProcessThreadByCid(IN PCLIENT_ID Cid,
                           OUT PEPROCESS *Process OPTIONAL,
                           OUT PETHREAD *Thread)
{
    PHANDLE_TABLE_ENTRY CidEntry;
    PETHREAD FoundThread;
    NTSTATUS Status = STATUS_INVALID_CID;
    PAGED_CODE();
    
    KeEnterCriticalRegion();

    /* Get the CID Handle Entry */
    if ((CidEntry = ExMapHandleToPointer(PspCidTable,
                                          Cid->UniqueThread)))
    {
        /* Get the Process */
        FoundThread = CidEntry->u1.Object;

        /* Make sure it's really a thread and this process' */
        if ((FoundThread->Tcb.DispatcherHeader.Type == ThreadObject) &&
            (FoundThread->Cid.UniqueProcess == Cid->UniqueProcess))
        {
            /* Reference and return it */
            ObReferenceObject(FoundThread);
            *Thread = FoundThread;
            Status = STATUS_SUCCESS;

            /* Check if we should return the Process too */
            if (Process)
            {
                /* Return it and reference it */
                *Process = FoundThread->ThreadsProcess;
                ObReferenceObject(*Process);
            }
        }

        /* Unlock the Entry */
        ExUnlockHandleTableEntry(PspCidTable, CidEntry);
    }
    
    KeLeaveCriticalRegion();

    /* Return to caller */
    return Status;
}

/*
 * FUNCTION: Returns a pointer to the current process
 *
 * @implemented
 */
PEPROCESS STDCALL
IoGetCurrentProcess(VOID)
{
   if (PsGetCurrentThread() == NULL ||
       PsGetCurrentThread()->Tcb.ApcState.Process == NULL)
     {
	return(PsInitialSystemProcess);
     }
   else
     {
	return(PEPROCESS)(PsGetCurrentThread()->Tcb.ApcState.Process);
     }
}

/*
 * @implemented
 */
LARGE_INTEGER STDCALL
PsGetProcessExitTime(VOID)
{
  LARGE_INTEGER Li;
  Li.QuadPart = PsGetCurrentProcess()->ExitTime.QuadPart;
  return Li;
}

/*
 * @implemented
 */
LONGLONG
STDCALL
PsGetProcessCreateTimeQuadPart(PEPROCESS Process)
{
    return Process->CreateTime.QuadPart;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessDebugPort(PEPROCESS Process)
{
    return Process->DebugPort;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetProcessExitProcessCalled(PEPROCESS Process)
{
    return Process->ProcessExiting;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsGetProcessExitStatus(PEPROCESS Process)
{
    return Process->ExitStatus;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessId(PEPROCESS Process)
{
    return (HANDLE)Process->UniqueProcessId;
}

/*
 * @implemented
 */
LPSTR
STDCALL
PsGetProcessImageFileName(PEPROCESS Process)
{
    return (LPSTR)Process->ImageFileName;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessInheritedFromUniqueProcessId(PEPROCESS Process)
{
    return Process->InheritedFromUniqueProcessId;
}

/*
 * @implemented
 */
PEJOB
STDCALL
PsGetProcessJob(PEPROCESS Process)
{
    return Process->Job;
}

/*
 * @implemented
 */
PPEB
STDCALL
PsGetProcessPeb(PEPROCESS Process)
{
    return Process->Peb;
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetProcessPriorityClass(PEPROCESS Process)
{
    return Process->PriorityClass;
}

/*
 * @implemented
 */
HANDLE STDCALL
PsGetCurrentProcessId(VOID)
{
    return((HANDLE)PsGetCurrentProcess()->UniqueProcessId);
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetCurrentProcessSessionId(VOID)
{
    return PsGetCurrentProcess()->Session;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessSectionBaseAddress(PEPROCESS Process)
{
    return Process->SectionBaseAddress;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessSecurityPort(PEPROCESS Process)
{
    return Process->SecurityPort;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessSessionId(PEPROCESS Process)
{
    return (HANDLE)Process->Session;
}

struct _W32THREAD*
STDCALL
PsGetWin32Thread(VOID)
{
    return(PsGetCurrentThread()->Tcb.Win32Thread);
}

struct _W32PROCESS*
STDCALL
PsGetWin32Process(VOID)
{
    return (struct _W32PROCESS*)PsGetCurrentProcess()->Win32Process;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessWin32Process(PEPROCESS Process)
{
    return Process->Win32Process;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessWin32WindowStation(PEPROCESS Process)
{
    return Process->Win32WindowStation;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsProcessBeingDebugged(PEPROCESS Process)
{
    return FALSE; //Process->IsProcessBeingDebugged;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessPriorityClass(PEPROCESS Process,
                          ULONG PriorityClass)
{
    Process->PriorityClass = PriorityClass;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessSecurityPort(PEPROCESS Process,
                         PVOID SecurityPort)
{
    Process->SecurityPort = SecurityPort;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessWin32Process(PEPROCESS Process,
                         PVOID Win32Process)
{
    Process->Win32Process = Win32Process;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessWindowStation(PEPROCESS Process,
                          PVOID WindowStation)
{
    Process->Win32WindowStation = WindowStation;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
PsSetProcessPriorityByClass(IN PEPROCESS Process,
                            IN ULONG Type)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * FUNCTION: Creates a process.
 * ARGUMENTS:
 *        ProcessHandle (OUT) = Caller supplied storage for the resulting
 *                              handle
 *        DesiredAccess = Specifies the allowed or desired access to the
 *                        process can be a combination of
 *                        STANDARD_RIGHTS_REQUIRED| ..
 *        ObjectAttribute = Initialized attributes for the object, contains
 *                          the rootdirectory and the filename
 *        ParentProcess = Handle to the parent process.
 *        InheritObjectTable = Specifies to inherit the objects of the parent
 *                             process if true.
 *        SectionHandle = Handle to a section object to back the image file
 *        DebugPort = Handle to a DebugPort if NULL the system default debug
 *                    port will be used.
 *        ExceptionPort = Handle to a exception port.
 * REMARKS:
 *        This function maps to the win32 CreateProcess.
 * RETURNS: Status
 *
 * @implemented
 */
NTSTATUS
STDCALL
NtCreateProcess(OUT PHANDLE ProcessHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                IN HANDLE ParentProcess,
                IN BOOLEAN InheritObjectTable,
                IN HANDLE SectionHandle  OPTIONAL,
                IN HANDLE DebugPort  OPTIONAL,
                IN HANDLE ExceptionPort  OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode  = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Check parameters */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(ProcessHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Make sure there's a parent process */
    if(ParentProcess == NULL)
    {
        /* Can't create System Processes like this */
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* Create a user Process */
        Status = PspCreateProcess(ProcessHandle,
                                  DesiredAccess,
                                  ObjectAttributes,
                                  ParentProcess,
                                  InheritObjectTable,
                                  SectionHandle,
                                  DebugPort,
                                  ExceptionPort);
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtOpenProcess(OUT PHANDLE ProcessHandle,
              IN  ACCESS_MASK DesiredAccess,
              IN  POBJECT_ATTRIBUTES ObjectAttributes,
              IN  PCLIENT_ID ClientId)
{
    KPROCESSOR_MODE PreviousMode  = ExGetPreviousMode();
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PEPROCESS Process;
    PETHREAD Thread = NULL;

    DPRINT("NtOpenProcess(ProcessHandle %x, DesiredAccess %x, "
           "ObjectAttributes %x, ClientId %x { UniP %d, UniT %d })\n",
           ProcessHandle, DesiredAccess, ObjectAttributes, ClientId,
           ClientId->UniqueProcess, ClientId->UniqueThread);

    PAGED_CODE();

    /* Open by name if one was given */
    DPRINT("Checking type\n");
    if (ObjectAttributes->ObjectName)
    {
        /* Open it */
        DPRINT("Opening by name\n");
        Status = ObOpenObjectByName(ObjectAttributes,
                                    PsProcessType,
                                    NULL,
                                    PreviousMode,
                                    DesiredAccess,
                                    NULL,
                                    ProcessHandle);

        if (Status != STATUS_SUCCESS)
        {
            DPRINT1("Could not open object by name\n");
        }

        /* Return Status */
        DPRINT("Found: %x\n", ProcessHandle);
        return(Status);
    }
    else if (ClientId)
    {
        /* Open by Thread ID */
        if (ClientId->UniqueThread)
        {
            /* Get the Process */
            DPRINT("Opening by Thread ID: %x\n", ClientId->UniqueThread);
            Status = PsLookupProcessThreadByCid(ClientId,
                                                &Process,
                                                &Thread);
            DPRINT("Found: %x\n", Process);
        }
        else
        {
            /* Get the Process */
            DPRINT("Opening by Process ID: %x\n", ClientId->UniqueProcess);
            Status = PsLookupProcessByProcessId(ClientId->UniqueProcess,
                                                &Process);
            DPRINT("Found: %x\n", Process);
        }

        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to find process\n");
            return Status;
        }

        /* Open the Process Object */
        Status = ObOpenObjectByPointer(Process,
                                       ObjectAttributes->Attributes,
                                       NULL,
                                       DesiredAccess,
                                       PsProcessType,
                                       PreviousMode,
                                       ProcessHandle);
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to open process\n");
        }

        /* Dereference the thread if we used it */
        if (Thread) ObDereferenceObject(Thread);

        /* Dereference the Process */
        ObDereferenceObject(Process);
    }

    return Status;
}
/* EOF */
