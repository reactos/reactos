/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/process.c
 * PURPOSE:         Process Manager: Process Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#define LockEvent Spare0[0]
#define LockCount Spare0[1]
#define LockOwner Spare0[2]

/* GLOBALS ******************************************************************/

PEPROCESS PsInitialSystemProcess = NULL;
PEPROCESS PsIdleProcess = NULL;
POBJECT_TYPE PsProcessType = NULL;
extern PHANDLE_TABLE PspCidTable;
extern POBJECT_TYPE DbgkDebugObjectType;

EPROCESS_QUOTA_BLOCK PspDefaultQuotaBlock;

ULONG PsMinimumWorkingSet, PsMaximumWorkingSet;

LIST_ENTRY PsActiveProcessHead;
FAST_MUTEX PspActiveProcessMutex;
LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;

/* INTERNAL FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
PspDeleteLdt(PEPROCESS Process)
{
    /* FIXME */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PspDeleteVdmObjects(PEPROCESS Process)
{
    /* FIXME */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
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
        KeClearEvent(Process->LockEvent);
      }

      return STATUS_SUCCESS;
    }
    else
    {
      if(++Attempts > 2)
      {
        Status = KeWaitForSingleObject(Process->LockEvent,
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
NTAPI
PsUnlockProcess(PEPROCESS Process)
{
  PAGED_CODE();

  ASSERT(Process->LockOwner == KeGetCurrentThread());

  if(InterlockedDecrementUL(&Process->LockCount) == 0)
  {
    (void)InterlockedExchangePointer(&Process->LockOwner, NULL);
    KeSetEvent(Process->LockEvent, IO_NO_INCREMENT, FALSE);
  }

  KeLeaveCriticalRegion();
}

PETHREAD
NTAPI
PsGetNextProcessThread(IN PEPROCESS Process,
                       IN PETHREAD Thread OPTIONAL)
{
    PETHREAD FoundThread = NULL;
    PLIST_ENTRY ListHead, Entry;
    PAGED_CODE();

    /* Lock the process */
    PsLockProcess(Process, FALSE);

    /* Check if we're already starting somewhere */
    if (Thread)
    {
        /* Start where we left off */
        Entry = Thread->ThreadListEntry.Flink;
    }
    else
    {
        /* Start at the beginning */
        Entry = Process->ThreadListHead.Flink;
    }

    /* Set the list head and start looping */
    ListHead = &Process->ThreadListHead;
    while (ListHead != Entry)
    {
        /* Get the Thread */
        FoundThread = CONTAINING_RECORD(Entry, ETHREAD, ThreadListEntry);

        /* Reference the thread. FIXME: Race, use ObSafeReferenceObject */
        ObReferenceObject(FoundThread);
        break;
    }

    /* Unlock the process */
    PsUnlockProcess(Process);

    /* Check if we had a starting thread, and dereference it */
    if (Thread) ObDereferenceObject(Thread);

    /* Return what we found */
    return FoundThread;
}

PEPROCESS
NTAPI
PsGetNextProcess(PEPROCESS OldProcess)
{
    PEPROCESS NextProcess;
    NTSTATUS Status;
    PAGED_CODE();

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
NTAPI
PspCreateProcess(OUT PHANDLE ProcessHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                 IN HANDLE ParentProcess  OPTIONAL,
                 IN DWORD Flags,
                 IN HANDLE SectionHandle  OPTIONAL,
                 IN HANDLE DebugPort  OPTIONAL,
                 IN HANDLE ExceptionPort OPTIONAL,
                 IN BOOLEAN InJob)
{
    HANDLE hProcess;
    PEPROCESS Process, Parent;
    PEPORT ExceptionPortObject;
    PDBGK_DEBUG_OBJECT DebugObject;
    PSECTION_OBJECT SectionObject;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PHYSICAL_ADDRESS DirectoryTableBase;
    KAFFINITY Affinity;
    HANDLE_TABLE_ENTRY CidEntry;
    PETHREAD CurrentThread;
    PEPROCESS CurrentProcess;
    ULONG MinWs, MaxWs;
    PAGED_CODE();
    DirectoryTableBase.QuadPart = 0;

    /* Get the current thread, process and cpu ring mode */
    CurrentThread = PsGetCurrentThread();
    ASSERT(&CurrentThread->Tcb == KeGetCurrentThread());
    PreviousMode = ExGetPreviousMode();
    ASSERT((CurrentThread) == PsGetCurrentThread());
    CurrentProcess = (PEPROCESS)CurrentThread->Tcb.ApcState.Process;

    /* Validate flags */
    if (Flags & ~PS_ALL_FLAGS) return STATUS_INVALID_PARAMETER;

    /* Check for parent */
    if(ParentProcess)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(ParentProcess,
                                           PROCESS_CREATE_PROCESS,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Parent,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference the parent process: Status: 0x%x\n", Status);
            return Status;
        }

        /* If this process should be in a job but the parent isn't */
        if ((InJob) && (!Parent->Job))
        {
            /* This is illegal. Dereference the parent and fail */
            ObDereferenceObject(Parent);
            return STATUS_INVALID_PARAMETER;
        }

        /* Inherit Parent process's Affinity. */
        Affinity = Parent->Pcb.Affinity;
    }
    else
    {
        /* We have no parent */
        Parent = NULL;
#ifdef CONFIG_SMP        
        /*
         * FIXME: Only the boot cpu is initialized in the early boot phase. 
    */
        Affinity = 0xffffffff;
#else
        Affinity = KeActiveProcessors;
#endif
    }

    /* Save working set data */
    MinWs = PsMinimumWorkingSet;
    MaxWs = PsMaximumWorkingSet;

    /* Create the Object */
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
        goto Cleanup;
    }

    /* Clean up the Object */
    RtlZeroMemory(Process, sizeof(EPROCESS));

    /* Setup the Thread List Head */
    InitializeListHead(&Process->ThreadListHead);

    /* Set up the Quota Block from the Parent */
    PspInheritQuota(Process, Parent);

    /* Set up Dos Device Map from the Parent */
    ObInheritDeviceMap(Parent, Process);

    /* Check if we have a parent */
    if (Parent)
    {
        /* Ineherit PID and Hard Error Processing */
        Process->InheritedFromUniqueProcessId = Parent->UniqueProcessId;
        Process->DefaultHardErrorProcessing = Parent->DefaultHardErrorProcessing;
    }
    else
    {
        /* Use default hard error processing */
        Process->DefaultHardErrorProcessing = TRUE;
    }

    /* Check for a section handle */
    if (SectionHandle)
    {
        /* Get a pointer to it */
        Status = ObReferenceObjectByHandle(SectionHandle,
                                           SECTION_MAP_EXECUTE,
                                           MmSectionObjectType,
                                           PreviousMode,
                                           (PVOID*)&SectionObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference process image section: Status: 0x%x\n", Status);
            goto CleanupWithRef;
        }
    }
    else
    {
        /* Is the parent the initial process? */
        if (Parent != PsInitialSystemProcess)
        {
            /* It's not, so acquire the process rundown */
            // FIXME

            /* If the parent has a section, use it */
            SectionObject = Parent->SectionObject;
            if (SectionObject) ObReferenceObject(SectionObject);

            /* Release process rundown */
            // FIXME

            /* If we don't have a section object */
            if (!SectionObject)
            {
                /* Then the process is in termination, so fail */
                Status = STATUS_PROCESS_IS_TERMINATING;
                goto CleanupWithRef;
            }
        }
    }

    /* Save the pointer to the section object */
    Process->SectionObject = SectionObject;

    /* Check for the debug port */
    if (DebugPort)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(DebugPort,
                                           DEBUG_OBJECT_ADD_REMOVE_PROCESS,
                                           DbgkDebugObjectType,
                                           PreviousMode,
                                           (PVOID*)&DebugObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference the debug port: Status: 0x%x\n", Status);
            goto CleanupWithRef;
        }

        /* Save the debug object */
        Process->DebugPort = DebugObject;

        /* Check if the caller doesn't want the debug stuff inherited */
        if (Flags & PS_NO_DEBUG_INHERIT) InterlockedOr((PLONG)&Process->Flags, 2);
    }
    else
    {
        /* Do we have a parent? Copy his debug port */
        if (Parent) DbgkCopyProcessDebugPort(Process, Parent);
    }

    /* Now check for an exception port */
    if (ExceptionPort)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(ExceptionPort,
                                           PORT_ALL_ACCESS,
                                           LpcPortObjectType,
                                           PreviousMode,
                                           (PVOID*)&ExceptionPortObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reference the exception port: Status: 0x%x\n", Status);
            goto CleanupWithRef;
        }

        /* Save the exception port */
        Process->ExceptionPort = ExceptionPortObject;
    }

    /* Save the pointer to the section object */
    Process->SectionObject = SectionObject;

    /* Setup the Lock Event */
    Process->LockEvent = ExAllocatePoolWithTag(PagedPool,
                                               sizeof(KEVENT),
                                               TAG('P', 's', 'L', 'k'));
    KeInitializeEvent(Process->LockEvent, SynchronizationEvent, FALSE);

    /* Set default exit code */
    Process->ExitStatus = STATUS_TIMEOUT;

    /* Create or Clone the Handle Table */
    ObpCreateHandleTable(Parent, Process);

    /* Set Process's Directory Base */
    MmCopyMmInfo(Parent ? Parent : PsInitialSystemProcess,
                 Process,
                 &DirectoryTableBase);

    /* We now have an address space */
    InterlockedOr((PLONG)&Process->Flags, 0x40000);

    /* Set the maximum WS */
    Process->Vm.MaximumWorkingSetSize = MaxWs;

    /* Now initialize the Kernel Process */
    KeInitializeProcess(&Process->Pcb,
                        PROCESS_PRIORITY_NORMAL,
                        Affinity,
                        DirectoryTableBase);

    /* Duplicate Parent Token */
    Status = PspInitializeProcessSecurity(Process, Parent);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PspInitializeProcessSecurity failed (Status %x)\n", Status);
        goto CleanupWithRef;
    }

    /* Set default priority class */
    Process->PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;

    /* Create the Process' Address Space */
    Status = MmCreateProcessAddressSpace(Process, (PROS_SECTION_OBJECT)SectionObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Address Space\n");
        goto CleanupWithRef;
    }

    /* Check for parent again */
#if 0
    if (!Parent)
    {
        /* Allocate our Audit info */
        Process->SeAuditProcessCreationInfo.ImageFileName = 
            ExAllocatePoolWithTag(PagedPool,
                                  sizeof(SE_AUDIT_PROCESS_CREATION_INFO),
                                  TAG_SEPA);
        RtlZeroMemory(Process->SeAuditProcessCreationInfo.ImageFileName,
                      sizeof(SE_AUDIT_PROCESS_CREATION_INFO));
    }
    else
    {
        /* Allocate our Audit info */
        Process->SeAuditProcessCreationInfo.ImageFileName = 
            ExAllocatePoolWithTag(PagedPool,
                                  sizeof(SE_AUDIT_PROCESS_CREATION_INFO) + 
                                  Parent->SeAuditProcessCreationInfo.
                                  ImageFileName->Name.MaximumLength,
                                  TAG_SEPA);

        /* Copy from parent */
        RtlCopyMemory(Process->SeAuditProcessCreationInfo.ImageFileName,
                      Parent->SeAuditProcessCreationInfo.ImageFileName,
                      sizeof(SE_AUDIT_PROCESS_CREATION_INFO) + 
                      Parent->SeAuditProcessCreationInfo.ImageFileName->
                      Name.MaximumLength);

        /* Update buffer pointer */
        Process->SeAuditProcessCreationInfo.ImageFileName->Name.Buffer = 
            (PVOID)(Process->SeAuditProcessCreationInfo.ImageFileName + 1);
    }
#endif

    /* Check if we have a section object */
    if (SectionObject)
    {
        /* Map the System Dll */
        PspMapSystemDll(Process, NULL);
    }

    /* Create a handle for the Process */
    CidEntry.Object = Process;
    CidEntry.GrantedAccess = 0;
    Process->UniqueProcessId = ExCreateHandle(PspCidTable, &CidEntry);
    if(!Process->UniqueProcessId)
    {
        DPRINT1("Failed to create CID handle\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupWithRef;
    }

    /* FIXME: Insert into Job Object */

    /* Create PEB only for User-Mode Processes */
    if (Parent)
    {
        Status = MmCreatePeb(Process);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtCreateProcess() Peb creation failed: Status %x\n",Status);
            goto CleanupWithRef;
        }
    }

    /* The process can now be activated */
    ExAcquireFastMutex(&PspActiveProcessMutex);
    InsertTailList(&PsActiveProcessHead, &Process->ActiveProcessLinks);
    ExReleaseFastMutex(&PspActiveProcessMutex);

    /* FIXME: SeCreateAccessStateEx */

    /* Insert the Process into the Object Directory */
    Status = ObInsertObject(Process,
                            NULL,
                            DesiredAccess,
                            1,
                            (PVOID*)&Process,
                            &hProcess);

    /* FIXME: Compute Quantum and Priority */

    /* 
     * FIXME: ObGetObjectSecurity(Process, &SecurityDescriptor)
     *        SeAccessCheck
     */
    ObReferenceObject(Process); // <- Act as if we called ObGetObjectSecurity

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Set the Creation Time */
        KeQuerySystemTime(&Process->CreateTime);

        /* Protect against bad user-mode pointer */
        _SEH_TRY
        {
            /* Save the process handle */
           *ProcessHandle = hProcess;
        }
        _SEH_HANDLE
        {
           Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

CleanupWithRef:
    /* 
     * Dereference the process. For failures, kills the process and does
     * cleanup present in PspDeleteProcess. For success, kills the extra
     * reference added by ObGetObjectSecurity
     */
    ObDereferenceObject(Process);

Cleanup:
    /* Dereference the parent */
    if (Parent) ObDereferenceObject(Parent);

    /* Return status to caller */
    return Status;
}

/* PUBLIC FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsCreateSystemProcess(PHANDLE ProcessHandle,
                      ACCESS_MASK DesiredAccess,
                      POBJECT_ATTRIBUTES ObjectAttributes)
{
    return PspCreateProcess(ProcessHandle,
                            DesiredAccess,
                            ObjectAttributes,
                            NULL,
                            0,
                            NULL,
                            NULL,
                            NULL,
                            FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
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
        FoundProcess = CidEntry->Object;

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

    /* Return to caller */
    KeLeaveCriticalRegion();
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
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
        FoundThread = CidEntry->Object;

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
    
    /* Return to caller */
    KeLeaveCriticalRegion();
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
NTAPI
PsGetProcessCreateTimeQuadPart(PEPROCESS Process)
{
    return Process->CreateTime.QuadPart;
}

/*
 * @implemented
 */
PVOID
NTAPI
PsGetProcessDebugPort(PEPROCESS Process)
{
    return Process->DebugPort;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
PsGetProcessExitProcessCalled(PEPROCESS Process)
{
    return Process->ProcessExiting;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsGetProcessExitStatus(PEPROCESS Process)
{
    return Process->ExitStatus;
}

/*
 * @implemented
 */
HANDLE
NTAPI
PsGetProcessId(PEPROCESS Process)
{
    return (HANDLE)Process->UniqueProcessId;
}

/*
 * @implemented
 */
LPSTR
NTAPI
PsGetProcessImageFileName(PEPROCESS Process)
{
    return (LPSTR)Process->ImageFileName;
}

/*
 * @implemented
 */
HANDLE
NTAPI
PsGetProcessInheritedFromUniqueProcessId(PEPROCESS Process)
{
    return Process->InheritedFromUniqueProcessId;
}

/*
 * @implemented
 */
PEJOB
NTAPI
PsGetProcessJob(PEPROCESS Process)
{
    return Process->Job;
}

/*
 * @implemented
 */
PPEB
NTAPI
PsGetProcessPeb(PEPROCESS Process)
{
    return Process->Peb;
}

/*
 * @implemented
 */
ULONG
NTAPI
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
NTAPI
PsGetCurrentProcessSessionId(VOID)
{
    return PsGetCurrentProcess()->Session;
}

/*
 * @implemented
 */
PVOID
NTAPI
PsGetProcessSectionBaseAddress(PEPROCESS Process)
{
    return Process->SectionBaseAddress;
}

/*
 * @implemented
 */
PVOID
NTAPI
PsGetProcessSecurityPort(PEPROCESS Process)
{
    return Process->SecurityPort;
}

/*
 * @implemented
 */
HANDLE
NTAPI
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
NTAPI
PsGetProcessWin32Process(PEPROCESS Process)
{
    return Process->Win32Process;
}

/*
 * @implemented
 */
PVOID
NTAPI
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
NTAPI
PsSetProcessPriorityClass(PEPROCESS Process,
                          ULONG PriorityClass)
{
    Process->PriorityClass = PriorityClass;
}

/*
 * @implemented
 */
VOID
NTAPI
PsSetProcessSecurityPort(PEPROCESS Process,
                         PVOID SecurityPort)
{
    Process->SecurityPort = SecurityPort;
}

/*
 * @implemented
 */
VOID
NTAPI
PsSetProcessWin32Process(PEPROCESS Process,
                         PVOID Win32Process)
{
    Process->Win32Process = Win32Process;
}

/*
 * @implemented
 */
VOID
NTAPI
PsSetProcessWindowStation(PEPROCESS Process,
                          PVOID WindowStation)
{
    Process->Win32WindowStation = WindowStation;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
PsSetProcessPriorityByClass(IN PEPROCESS Process,
                            IN ULONG Type)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateProcessEx(OUT PHANDLE ProcessHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                  IN HANDLE ParentProcess,
                  IN ULONG Flags,
                  IN HANDLE SectionHandle OPTIONAL,
                  IN HANDLE DebugPort OPTIONAL,
                  IN HANDLE ExceptionPort OPTIONAL,
                  IN BOOLEAN InJob)
{
    KPROCESSOR_MODE PreviousMode  = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Check parameters */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(ProcessHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Make sure there's a parent process */
    if(!ParentProcess)
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
                                  Flags,
                                  SectionHandle,
                                  DebugPort,
                                  ExceptionPort,
                                  InJob);
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateProcess(OUT PHANDLE ProcessHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                IN HANDLE ParentProcess,
                IN BOOLEAN InheritObjectTable,
                IN HANDLE SectionHandle  OPTIONAL,
                IN HANDLE DebugPort  OPTIONAL,
                IN HANDLE ExceptionPort  OPTIONAL)
{
    ULONG Flags = 0;

    /* Set new-style flags */
    if ((ULONG)SectionHandle & 1) Flags = PS_REQUEST_BREAKAWAY;
    if ((ULONG)DebugPort & 1) Flags |= PS_NO_DEBUG_INHERIT;
    if (InheritObjectTable) Flags |= PS_INHERIT_HANDLES;

    /* Call the new API */
    return NtCreateProcessEx(ProcessHandle,
                             DesiredAccess,
                             ObjectAttributes,
                             ParentProcess,
                             Flags,
                             SectionHandle,
                             DebugPort,
                             ExceptionPort,
                             FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenProcess(OUT PHANDLE ProcessHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes,
              IN PCLIENT_ID ClientId)
{
    KPROCESSOR_MODE PreviousMode;
    CLIENT_ID SafeClientId;
    ULONG Attributes = 0;
    HANDLE hProcess;
    BOOLEAN HasObjectName = FALSE;
    PETHREAD Thread = NULL;
    PEPROCESS Process = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    /* Probe the paraemeters */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(ProcessHandle);

            if(ClientId != NULL)
            {
                ProbeForRead(ClientId,
                             sizeof(CLIENT_ID),
                             sizeof(ULONG));

                SafeClientId = *ClientId;
                ClientId = &SafeClientId;
            }

            /* just probe the object attributes structure, don't capture it
               completely. This is done later if necessary */
            ProbeForRead(ObjectAttributes,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));
            HasObjectName = (ObjectAttributes->ObjectName != NULL);
            Attributes = ObjectAttributes->Attributes;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        HasObjectName = (ObjectAttributes->ObjectName != NULL);
        Attributes = ObjectAttributes->Attributes;
    }

    if (HasObjectName && ClientId != NULL)
    {
        /* can't pass both, n object name and a client id */
        return STATUS_INVALID_PARAMETER_MIX;
    }

    /* Open by name if one was given */
    if (HasObjectName)
    {
        /* Open it */
        Status = ObOpenObjectByName(ObjectAttributes,
                                    PsProcessType,
                                    PreviousMode,
                                    NULL,
                                    DesiredAccess,
                                    NULL,
                                    &hProcess);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Could not open object by name\n");
        }
    }
    else if (ClientId != NULL)
    {
        /* Open by Thread ID */
        if (ClientId->UniqueThread)
        {
            /* Get the Process */
            Status = PsLookupProcessThreadByCid(ClientId,
                                                &Process,
                                                &Thread);
        }
        else
        {
            /* Get the Process */
            Status = PsLookupProcessByProcessId(ClientId->UniqueProcess,
                                                &Process);
        }

        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to find process\n");
            return Status;
        }

        /* Open the Process Object */
        Status = ObOpenObjectByPointer(Process,
                                       Attributes,
                                       NULL,
                                       DesiredAccess,
                                       PsProcessType,
                                       PreviousMode,
                                       &hProcess);
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Failure to open process\n");
        }

        /* Dereference the thread if we used it */
        if (Thread) ObDereferenceObject(Thread);

        /* Dereference the Process */
        ObDereferenceObject(Process);
    }
    else
    {
        /* neither an object name nor a client id was passed */
        return STATUS_INVALID_PARAMETER_MIX;
    }

    /* Write back the handle */
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            *ProcessHandle = hProcess;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    return Status;
}
/* EOF */
