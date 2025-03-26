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
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ULONG PsMinimumWorkingSet, PsMaximumWorkingSet;

POBJECT_TYPE PsProcessType = NULL;

LIST_ENTRY PsActiveProcessHead;
KGUARDED_MUTEX PspActiveProcessMutex;

LARGE_INTEGER ShortPsLockDelay;

ULONG PsRawPrioritySeparation;
ULONG PsPrioritySeparation;
CHAR PspForegroundQuantum[3];

/* Fixed quantum table */
CHAR PspFixedQuantums[6] =
{
    /* Short quantums */
    3 * 6, /* Level 1 */
    3 * 6, /* Level 2 */
    3 * 6, /* Level 3 */

    /* Long quantums */
    6 * 6, /* Level 1 */
    6 * 6, /* Level 2 */
    6 * 6  /* Level 3 */
};

/* Variable quantum table */
CHAR PspVariableQuantums[6] =
{
    /* Short quantums */
    1 * 6, /* Level 1 */
    2 * 6, /* Level 2 */
    3 * 6, /* Level 3 */

    /* Long quantums */
    2 * 6, /* Level 1 */
    4 * 6, /* Level 2 */
    6 * 6  /* Level 3 */
};

/* Priority table */
KPRIORITY PspPriorityTable[PROCESS_PRIORITY_CLASS_ABOVE_NORMAL + 1] =
{
    8,
    4,
    8,
    13,
    24,
    6,
    10
};

/* PRIVATE FUNCTIONS *********************************************************/

PETHREAD
NTAPI
PsGetNextProcessThread(IN PEPROCESS Process,
                       IN PETHREAD Thread OPTIONAL)
{
    PETHREAD FoundThread = NULL;
    PLIST_ENTRY ListHead, Entry;
    PAGED_CODE();
    PSTRACE(PS_PROCESS_DEBUG,
            "Process: %p Thread: %p\n", Process, Thread);

    /* Lock the process */
    KeEnterCriticalRegion();
    ExAcquirePushLockShared(&Process->ProcessLock);

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

        /* Safe reference the thread */
        if (ObReferenceObjectSafe(FoundThread)) break;

        /* Nothing found, keep looping */
        FoundThread = NULL;
        Entry = Entry->Flink;
    }

    /* Unlock the process */
    ExReleasePushLockShared(&Process->ProcessLock);
    KeLeaveCriticalRegion();

    /* Check if we had a starting thread, and dereference it */
    if (Thread) ObDereferenceObject(Thread);

    /* Return what we found */
    return FoundThread;
}

PEPROCESS
NTAPI
PsGetNextProcess(IN PEPROCESS OldProcess)
{
    PLIST_ENTRY Entry;
    PEPROCESS FoundProcess = NULL;
    PAGED_CODE();
    PSTRACE(PS_PROCESS_DEBUG, "Process: %p\n", OldProcess);

    /* Acquire the Active Process Lock */
    KeAcquireGuardedMutex(&PspActiveProcessMutex);

    /* Check if we're already starting somewhere */
    if (OldProcess)
    {
        /* Start where we left off */
        Entry = OldProcess->ActiveProcessLinks.Flink;
    }
    else
    {
        /* Start at the beginning */
        Entry = PsActiveProcessHead.Flink;
    }

    /* Loop the process list */
    while (Entry != &PsActiveProcessHead)
    {
        /* Get the process */
        FoundProcess = CONTAINING_RECORD(Entry, EPROCESS, ActiveProcessLinks);

        /* Reference the process */
        if (ObReferenceObjectSafe(FoundProcess)) break;

        /* Nothing found, keep trying */
        FoundProcess = NULL;
        Entry = Entry->Flink;
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&PspActiveProcessMutex);

    /* Dereference the Process we had referenced earlier */
    if (OldProcess) ObDereferenceObject(OldProcess);
    return FoundProcess;
}

KPRIORITY
NTAPI
PspComputeQuantumAndPriority(IN PEPROCESS Process,
                             IN PSPROCESSPRIORITYMODE Mode,
                             OUT PUCHAR Quantum)
{
    ULONG i;
    UCHAR LocalQuantum, MemoryPriority;
    PAGED_CODE();
    PSTRACE(PS_PROCESS_DEBUG, "Process: %p Mode: %lx\n", Process, Mode);

    /* Check if this is a foreground process */
    if (Mode == PsProcessPriorityForeground)
    {
        /* Set the memory priority and use priority separation */
        MemoryPriority = MEMORY_PRIORITY_FOREGROUND;
        i = PsPrioritySeparation;
    }
    else
    {
        /* Set the background memory priority and no separation */
        MemoryPriority = MEMORY_PRIORITY_BACKGROUND;
        i = 0;
    }

    /* Make sure that the process mode isn't spinning */
    if (Mode != PsProcessPrioritySpinning)
    {
        /* Set the priority */
        MmSetMemoryPriorityProcess(Process, MemoryPriority);
    }

    /* Make sure that the process isn't idle */
    if (Process->PriorityClass != PROCESS_PRIORITY_CLASS_IDLE)
    {
        /* Does the process have a job? */
        if ((Process->Job) && (PspUseJobSchedulingClasses))
        {
            /* Use job quantum */
            LocalQuantum = PspJobSchedulingClasses[Process->Job->
                                                   SchedulingClass];
        }
        else
        {
            /* Use calculated quantum */
            LocalQuantum = PspForegroundQuantum[i];
        }
    }
    else
    {
        /* Process is idle, use default quantum */
        LocalQuantum = 6;
    }

    /* Return quantum to caller */
    *Quantum = LocalQuantum;

    /* Return priority */
    return PspPriorityTable[Process->PriorityClass];
}

VOID
NTAPI
PsChangeQuantumTable(IN BOOLEAN Immediate,
                     IN ULONG PrioritySeparation)
{
    PEPROCESS Process = NULL;
    ULONG i;
    UCHAR Quantum;
    PCHAR QuantumTable;
    PAGED_CODE();
    PSTRACE(PS_PROCESS_DEBUG,
            "%lx PrioritySeparation: %lx\n", Immediate, PrioritySeparation);

    /* Write the current priority separation */
    PsPrioritySeparation = PspPrioritySeparationFromMask(PrioritySeparation);

    /* Normalize it if it was too high */
    if (PsPrioritySeparation == 3) PsPrioritySeparation = 2;

    /* Get the quantum table to use */
    if (PspQuantumTypeFromMask(PrioritySeparation) == PSP_VARIABLE_QUANTUMS)
    {
        /* Use a variable table */
        QuantumTable = PspVariableQuantums;
    }
    else if (PspQuantumTypeFromMask(PrioritySeparation) == PSP_FIXED_QUANTUMS)
    {
        /* Use fixed table */
        QuantumTable = PspFixedQuantums;
    }
    else
    {
        /* Use default for the type of system we're on */
        QuantumTable = MmIsThisAnNtAsSystem() ? PspFixedQuantums : PspVariableQuantums;
    }

    /* Now check if we should use long or short */
    if (PspQuantumLengthFromMask(PrioritySeparation) == PSP_LONG_QUANTUMS)
    {
        /* Use long quantums */
        QuantumTable += 3;
    }
    else if (PspQuantumLengthFromMask(PrioritySeparation) == PSP_SHORT_QUANTUMS)
    {
        /* Keep existing table */
        NOTHING;
    }
    else
    {
        /* Use default for the type of system we're on */
        QuantumTable += MmIsThisAnNtAsSystem() ? 3 : 0;
    }

    /* Check if we're using long fixed quantums */
    if (QuantumTable == &PspFixedQuantums[3])
    {
        /* Use Job scheduling classes */
         PspUseJobSchedulingClasses = TRUE;
    }
    else
    {
        /* Otherwise, we don't */
        PspUseJobSchedulingClasses = FALSE;
    }

    /* Copy the selected table into the Foreground Quantum table */
    RtlCopyMemory(PspForegroundQuantum,
                  QuantumTable,
                  sizeof(PspForegroundQuantum));

    /* Check if we should apply these changes real-time */
    if (Immediate)
    {
        /* We are...loop every process */
        Process = PsGetNextProcess(Process);
        while (Process)
        {
            /* Use the priority separation if this is a foreground process */
            i = (Process->Vm.Flags.MemoryPriority ==
                 MEMORY_PRIORITY_BACKGROUND) ?
                 0: PsPrioritySeparation;

            /* Make sure that the process isn't idle */
            if (Process->PriorityClass != PROCESS_PRIORITY_CLASS_IDLE)
            {
                /* Does the process have a job? */
                if ((Process->Job) && (PspUseJobSchedulingClasses))
                {
                    /* Use job quantum */
                    Quantum = PspJobSchedulingClasses[Process->Job->SchedulingClass];
                }
                else
                {
                    /* Use calculated quantum */
                    Quantum = PspForegroundQuantum[i];
                }
            }
            else
            {
                /* Process is idle, use default quantum */
                Quantum = 6;
            }

            /* Now set the quantum */
            KeSetQuantumProcess(&Process->Pcb, Quantum);

            /* Get the next process */
            Process = PsGetNextProcess(Process);
        }
    }
}

NTSTATUS
NTAPI
PspCreateProcess(OUT PHANDLE ProcessHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                 IN HANDLE ParentProcess OPTIONAL,
                 IN ULONG Flags,
                 IN HANDLE SectionHandle OPTIONAL,
                 IN HANDLE DebugPort OPTIONAL,
                 IN HANDLE ExceptionPort OPTIONAL,
                 IN BOOLEAN InJob)
{
    HANDLE hProcess;
    PEPROCESS Process, Parent;
    PVOID ExceptionPortObject;
    PDEBUG_OBJECT DebugObject;
    PSECTION SectionObject;
    NTSTATUS Status, AccessStatus;
    ULONG_PTR DirectoryTableBase[2] = {0,0};
    KAFFINITY Affinity;
    HANDLE_TABLE_ENTRY CidEntry;
    PETHREAD CurrentThread = PsGetCurrentThread();
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    ULONG MinWs, MaxWs;
    ACCESS_STATE LocalAccessState;
    PACCESS_STATE AccessState = &LocalAccessState;
    AUX_ACCESS_DATA AuxData;
    UCHAR Quantum;
    BOOLEAN Result, SdAllocated;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    BOOLEAN NeedsPeb = FALSE;
    INITIAL_PEB InitialPeb;
    PAGED_CODE();
    PSTRACE(PS_PROCESS_DEBUG,
            "ProcessHandle: %p Parent: %p\n", ProcessHandle, ParentProcess);

    /* Validate flags */
    if (Flags & ~PROCESS_CREATE_FLAGS_LEGAL_MASK) return STATUS_INVALID_PARAMETER;

    /* Check for parent */
    if (ParentProcess)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(ParentProcess,
                                           PROCESS_CREATE_PROCESS,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Parent,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;

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
        Affinity = KeActiveProcessors;
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
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Clean up the Object */
    RtlZeroMemory(Process, sizeof(EPROCESS));

    /* Initialize pushlock and rundown protection */
    ExInitializeRundownProtection(&Process->RundownProtect);
    Process->ProcessLock.Value = 0;

    /* Setup the Thread List Head */
    InitializeListHead(&Process->ThreadListHead);

    /* Set up the Quota Block from the Parent */
    PspInheritQuota(Process, Parent);

    /* Set up Dos Device Map from the Parent */
    ObInheritDeviceMap(Parent, Process);

    /* Check if we have a parent */
    if (Parent)
    {
        /* Inherit PID and hard-error processing */
        Process->InheritedFromUniqueProcessId = Parent->UniqueProcessId;
        Process->DefaultHardErrorProcessing = Parent->DefaultHardErrorProcessing;
    }
    else
    {
        /* Use default hard-error processing */
        Process->DefaultHardErrorProcessing = SEM_FAILCRITICALERRORS;
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
        if (!NT_SUCCESS(Status)) goto CleanupWithRef;
    }
    else
    {
        /* Assume no section object */
        SectionObject = NULL;

        /* Is the parent the initial process?
         * Check for NULL also, as at initialization PsInitialSystemProcess is NULL */
        if (Parent != PsInitialSystemProcess && (Parent != NULL))
        {
            /* It's not, so acquire the process rundown */
            if (ExAcquireRundownProtection(&Parent->RundownProtect))
            {
                /* If the parent has a section, use it */
                SectionObject = Parent->SectionObject;
                if (SectionObject) ObReferenceObject(SectionObject);

                /* Release process rundown */
                ExReleaseRundownProtection(&Parent->RundownProtect);
            }

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
        if (!NT_SUCCESS(Status)) goto CleanupWithRef;

        /* Save the debug object */
        Process->DebugPort = DebugObject;

        /* Check if the caller doesn't want the debug stuff inherited */
        if (Flags & PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT)
        {
            /* Set the process flag */
            InterlockedOr((PLONG)&Process->Flags, PSF_NO_DEBUG_INHERIT_BIT);
        }
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
        if (!NT_SUCCESS(Status)) goto CleanupWithRef;

        /* Save the exception port */
        Process->ExceptionPort = ExceptionPortObject;
    }

    /* Save the pointer to the section object */
    Process->SectionObject = SectionObject;

    /* Set default exit code */
    Process->ExitStatus = STATUS_PENDING;

    /* Check if this is the initial process being built */
    if (Parent)
    {
        /* Create the address space for the child */
        if (!MmCreateProcessAddressSpace(MinWs,
                                         Process,
                                         DirectoryTableBase))
        {
            /* Failed */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CleanupWithRef;
        }
    }
    else
    {
        /* Otherwise, we are the boot process, we're already semi-initialized */
        Process->ObjectTable = CurrentProcess->ObjectTable;
        Status = MmInitializeHandBuiltProcess(Process, DirectoryTableBase);
        if (!NT_SUCCESS(Status)) goto CleanupWithRef;
    }

    /* We now have an address space */
    InterlockedOr((PLONG)&Process->Flags, PSF_HAS_ADDRESS_SPACE_BIT);

    /* Set the maximum WS */
    Process->Vm.MaximumWorkingSetSize = MaxWs;

    /* Now initialize the Kernel Process */
    KeInitializeProcess(&Process->Pcb,
                        PROCESS_PRIORITY_NORMAL,
                        Affinity,
                        DirectoryTableBase,
                        BooleanFlagOn(Process->DefaultHardErrorProcessing,
                                      SEM_NOALIGNMENTFAULTEXCEPT));

    /* Duplicate Parent Token */
    Status = PspInitializeProcessSecurity(Process, Parent);
    if (!NT_SUCCESS(Status)) goto CleanupWithRef;

    /* Set default priority class */
    Process->PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;

    /* Check if we have a parent */
    if (Parent)
    {
        /* Check our priority class */
        if (Parent->PriorityClass == PROCESS_PRIORITY_CLASS_IDLE ||
            Parent->PriorityClass == PROCESS_PRIORITY_CLASS_BELOW_NORMAL)
        {
            /* Normalize it */
            Process->PriorityClass = Parent->PriorityClass;
        }

        /* Initialize object manager for the process */
        Status = ObInitProcess(Flags & PROCESS_CREATE_FLAGS_INHERIT_HANDLES ?
                               Parent : NULL,
                               Process);
        if (!NT_SUCCESS(Status)) goto CleanupWithRef;
    }
    else
    {
        /* Do the second part of the boot process memory setup */
        Status = MmInitializeHandBuiltProcess2(Process);
        if (!NT_SUCCESS(Status)) goto CleanupWithRef;
    }

    /* Set success for now */
    Status = STATUS_SUCCESS;

    /* Check if this is a real user-mode process */
    if (SectionHandle)
    {
        /* Initialize the address space */
        Status = MmInitializeProcessAddressSpace(Process,
                                                 NULL,
                                                 SectionObject,
                                                 &Flags,
                                                 &Process->
                                                 SeAuditProcessCreationInfo.
                                                 ImageFileName);
        if (!NT_SUCCESS(Status)) goto CleanupWithRef;

        //
        // We need a PEB
        //
        NeedsPeb = TRUE;
    }
    else if (Parent)
    {
        /* Check if this is a child of the system process */
        if (Parent != PsInitialSystemProcess)
        {
            //
            // We need a PEB
            //
            NeedsPeb = TRUE;

            /* This is a clone! */
            ASSERTMSG("No support for cloning yet\n", FALSE);
        }
        else
        {
            /* This is the initial system process */
            Flags &= ~PROCESS_CREATE_FLAGS_LARGE_PAGES;
            Status = MmInitializeProcessAddressSpace(Process,
                                                     NULL,
                                                     NULL,
                                                     &Flags,
                                                     NULL);
            if (!NT_SUCCESS(Status)) goto CleanupWithRef;

            /* Create a dummy image file name */
            Process->SeAuditProcessCreationInfo.ImageFileName =
                ExAllocatePoolWithTag(PagedPool,
                                      sizeof(OBJECT_NAME_INFORMATION),
                                      TAG_SEPA);
            if (!Process->SeAuditProcessCreationInfo.ImageFileName)
            {
                /* Fail */
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto CleanupWithRef;
            }

            /* Zero it out */
            RtlZeroMemory(Process->SeAuditProcessCreationInfo.ImageFileName,
                          sizeof(OBJECT_NAME_INFORMATION));
        }
    }

#if MI_TRACE_PFNS
    /* Copy the process name now that we have it */
    memcpy(MiGetPfnEntry(Process->Pcb.DirectoryTableBase[0] >> PAGE_SHIFT)->ProcessName, Process->ImageFileName, 16);
    if (Process->Pcb.DirectoryTableBase[1]) memcpy(MiGetPfnEntry(Process->Pcb.DirectoryTableBase[1] >> PAGE_SHIFT)->ProcessName, Process->ImageFileName, 16);
    if (Process->WorkingSetPage) memcpy(MiGetPfnEntry(Process->WorkingSetPage)->ProcessName, Process->ImageFileName, 16);
#endif

    /* Check if we have a section object and map the system DLL */
    if (SectionObject) PspMapSystemDll(Process, NULL, FALSE);

    /* Create a handle for the Process */
    CidEntry.Object = Process;
    CidEntry.GrantedAccess = 0;
    Process->UniqueProcessId = ExCreateHandle(PspCidTable, &CidEntry);
    if (!Process->UniqueProcessId)
    {
        /* Fail */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupWithRef;
    }

    /* Set the handle table PID */
    Process->ObjectTable->UniqueProcessId = Process->UniqueProcessId;

    /* Check if we need to audit */
    if (SeDetailedAuditingWithToken(NULL)) SeAuditProcessCreate(Process);

    /* Check if the parent had a job */
    if ((Parent) && (Parent->Job))
    {
        /* FIXME: We need to insert this process */
        DPRINT1("Jobs not yet supported\n");
    }

    /* Create PEB only for User-Mode Processes */
    if ((Parent) && (NeedsPeb))
    {
        //
        // Set up the initial PEB
        //
        RtlZeroMemory(&InitialPeb, sizeof(INITIAL_PEB));
        InitialPeb.Mutant = (HANDLE)-1;
        InitialPeb.ImageUsesLargePages = 0; // FIXME: Not yet supported

        //
        // Create it only if we have an image section
        //
        if (SectionHandle)
        {
            //
            // Create it
            //
            Status = MmCreatePeb(Process, &InitialPeb, &Process->Peb);
            if (!NT_SUCCESS(Status)) goto CleanupWithRef;
        }
        else
        {
            //
            // We have to clone it
            //
            ASSERTMSG("No support for cloning yet\n", FALSE);
        }

    }

    /* The process can now be activated */
    KeAcquireGuardedMutex(&PspActiveProcessMutex);
    InsertTailList(&PsActiveProcessHead, &Process->ActiveProcessLinks);
    KeReleaseGuardedMutex(&PspActiveProcessMutex);

    /* Create an access state */
    Status = SeCreateAccessStateEx(CurrentThread,
                                   ((Parent) &&
                                   (Parent == PsInitialSystemProcess)) ?
                                    Parent : CurrentProcess,
                                   &LocalAccessState,
                                   &AuxData,
                                   DesiredAccess,
                                   &PsProcessType->TypeInfo.GenericMapping);
    if (!NT_SUCCESS(Status)) goto CleanupWithRef;

    /* Insert the Process into the Object Directory */
    Status = ObInsertObject(Process,
                            AccessState,
                            DesiredAccess,
                            1,
                            NULL,
                            &hProcess);

    /* Free the access state */
    if (AccessState) SeDeleteAccessState(AccessState);

    /* Cleanup on failure */
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Compute Quantum and Priority */
    ASSERT(IsListEmpty(&Process->ThreadListHead) == TRUE);
    Process->Pcb.BasePriority =
        (SCHAR)PspComputeQuantumAndPriority(Process,
                                            PsProcessPriorityBackground,
                                            &Quantum);
    Process->Pcb.QuantumReset = Quantum;

    /* Check if we have a parent other then the initial system process */
    Process->GrantedAccess = PROCESS_TERMINATE;
    if ((Parent) && (Parent != PsInitialSystemProcess))
    {
        /* Get the process's SD */
        Status = ObGetObjectSecurity(Process,
                                     &SecurityDescriptor,
                                     &SdAllocated);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, close the handle and clean up */
            ObCloseHandle(hProcess, PreviousMode);
            goto CleanupWithRef;
        }

        /* Create the subject context */
        SubjectContext.ProcessAuditId = Process;
        SubjectContext.PrimaryToken = PsReferencePrimaryToken(Process);
        SubjectContext.ClientToken = NULL;

        /* Do the access check */
        Result = SeAccessCheck(SecurityDescriptor,
                               &SubjectContext,
                               FALSE,
                               MAXIMUM_ALLOWED,
                               0,
                               NULL,
                               &PsProcessType->TypeInfo.GenericMapping,
                               PreviousMode,
                               &Process->GrantedAccess,
                               &AccessStatus);

        /* Dereference the token and let go the SD */
        ObFastDereferenceObject(&Process->Token,
                                SubjectContext.PrimaryToken);
        ObReleaseObjectSecurity(SecurityDescriptor, SdAllocated);

        /* Remove access if it failed */
        if (!Result) Process->GrantedAccess = 0;

        /* Give the process some basic access */
        Process->GrantedAccess |= (PROCESS_VM_OPERATION |
                                   PROCESS_VM_READ |
                                   PROCESS_VM_WRITE |
                                   PROCESS_QUERY_INFORMATION |
                                   PROCESS_TERMINATE |
                                   PROCESS_CREATE_THREAD |
                                   PROCESS_DUP_HANDLE |
                                   PROCESS_CREATE_PROCESS |
                                   PROCESS_SET_INFORMATION |
                                   STANDARD_RIGHTS_ALL |
                                   PROCESS_SET_QUOTA);
    }
    else
    {
        /* Set full granted access */
        Process->GrantedAccess = PROCESS_ALL_ACCESS;
    }

    /* Set the Creation Time */
    KeQuerySystemTime(&Process->CreateTime);

    /* Protect against bad user-mode pointer */
    _SEH2_TRY
    {
        /* Hacky way of returning the PEB to the user-mode creator */
        if ((Process->Peb) && (CurrentThread->Tcb.Teb))
        {
            CurrentThread->Tcb.Teb->NtTib.ArbitraryUserPointer = Process->Peb;
        }

        /* Save the process handle */
       *ProcessHandle = hProcess;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the exception code */
       Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Run the Notification Routines */
    PspRunCreateProcessNotifyRoutines(Process, TRUE);

    /* If 12 processes have been created, enough of user-mode is ready */
    if (++ProcessCount == 12) Ki386PerfEnd();

CleanupWithRef:
    /*
     * Dereference the process. For failures, kills the process and does
     * cleanup present in PspDeleteProcess. For success, kills the extra
     * reference added by ObInsertObject.
     */
    ObDereferenceObject(Process);

Cleanup:
    /* Dereference the parent */
    if (Parent) ObDereferenceObject(Parent);

    /* Return status to caller */
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsCreateSystemProcess(OUT PHANDLE ProcessHandle,
                      IN ACCESS_MASK DesiredAccess,
                      IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    /* Call the internal API */
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
    PSTRACE(PS_PROCESS_DEBUG, "ProcessId: %p\n", ProcessId);
    KeEnterCriticalRegion();

    /* Get the CID Handle Entry */
    CidEntry = ExMapHandleToPointer(PspCidTable, ProcessId);
    if (CidEntry)
    {
        /* Get the Process */
        FoundProcess = CidEntry->Object;

        /* Make sure it's really a process */
        if (FoundProcess->Pcb.Header.Type == ProcessObject)
        {
            /* Safe Reference and return it */
            if (ObReferenceObjectSafe(FoundProcess))
            {
                *Process = FoundProcess;
                Status = STATUS_SUCCESS;
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
    PSTRACE(PS_PROCESS_DEBUG, "Cid: %p\n", Cid);
    KeEnterCriticalRegion();

    /* Get the CID Handle Entry */
    CidEntry = ExMapHandleToPointer(PspCidTable, Cid->UniqueThread);
    if (CidEntry)
    {
        /* Get the Process */
        FoundThread = CidEntry->Object;

        /* Make sure it's really a thread and this process' */
        if ((FoundThread->Tcb.Header.Type == ThreadObject) &&
            (FoundThread->Cid.UniqueProcess == Cid->UniqueProcess))
        {
            /* Safe Reference and return it */
            if (ObReferenceObjectSafe(FoundThread))
            {
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
LARGE_INTEGER
NTAPI
PsGetProcessExitTime(VOID)
{
    return PsGetCurrentProcess()->ExitTime;
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
    return (BOOLEAN)Process->ProcessExiting;
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
HANDLE
NTAPI
PsGetCurrentProcessId(VOID)
{
    return (HANDLE)PsGetCurrentProcess()->UniqueProcessId;
}

/*
 * @implemented
 */
ULONG
NTAPI
PsGetCurrentProcessSessionId(VOID)
{
    return MmGetSessionId(PsGetCurrentProcess());
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
ULONG
NTAPI
PsGetProcessSessionId(IN PEPROCESS Process)
{
    return MmGetSessionId(Process);
}

/*
 * @implemented
 */
ULONG
NTAPI
PsGetProcessSessionIdEx(IN PEPROCESS Process)
{
    return MmGetSessionIdEx(Process);
}

/*
 * @implemented
 */
PVOID
NTAPI
PsGetCurrentProcessWin32Process(VOID)
{
    return PsGetCurrentProcess()->Win32Process;
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
NTAPI
PsIsProcessBeingDebugged(PEPROCESS Process)
{
    return Process->DebugPort != NULL;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
PsIsSystemProcess(IN PEPROCESS Process)
{
    /* Return if this is the System Process */
    return Process == PsInitialSystemProcess;
}

/*
 * @implemented
 */
VOID
NTAPI
PsSetProcessPriorityClass(PEPROCESS Process,
                          ULONG PriorityClass)
{
    Process->PriorityClass = (UCHAR)PriorityClass;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsSetProcessSecurityPort(PEPROCESS Process,
                         PVOID SecurityPort)
{
    Process->SecurityPort = SecurityPort;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsSetProcessWin32Process(
    _Inout_ PEPROCESS Process,
    _In_opt_ PVOID Win32Process,
    _In_opt_ PVOID OldWin32Process)
{
    NTSTATUS Status;

    /* Assume success */
    Status = STATUS_SUCCESS;

    /* Lock the process */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&Process->ProcessLock);

    /* Check if we set a new win32 process */
    if (Win32Process != NULL)
    {
        /* Check if the process is in the right state */
        if (((Process->Flags & PSF_PROCESS_DELETE_BIT) == 0) &&
            (Process->Win32Process == NULL))
        {
            /* Set the new win32 process */
            Process->Win32Process = Win32Process;
        }
        else
        {
            /* Otherwise fail */
            Status = STATUS_PROCESS_IS_TERMINATING;
        }
    }
    else
    {
        /* Reset the win32 process, did the caller specify the correct old value? */
        if (Process->Win32Process == OldWin32Process)
        {
            /* Yes, so reset the win32 process to NULL */
            Process->Win32Process = NULL;
        }
        else
        {
            /* Otherwise fail */
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    /* Unlock the process */
    ExReleasePushLockExclusive(&Process->ProcessLock);
    KeLeaveCriticalRegion();

    return Status;
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
 * @implemented
 */
VOID
NTAPI
PsSetProcessPriorityByClass(IN PEPROCESS Process,
                            IN PSPROCESSPRIORITYMODE Type)
{
    UCHAR Quantum;
    ULONG Priority;
    PSTRACE(PS_PROCESS_DEBUG, "Process: %p Type: %lx\n", Process, Type);

    /* Compute quantum and priority */
    Priority = PspComputeQuantumAndPriority(Process, Type, &Quantum);

    /* Set them */
    KeSetPriorityAndQuantumProcess(&Process->Pcb, Priority, Quantum);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateProcessEx(OUT PHANDLE ProcessHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                  IN HANDLE ParentProcess,
                  IN ULONG Flags,
                  IN HANDLE SectionHandle OPTIONAL,
                  IN HANDLE DebugPort OPTIONAL,
                  IN HANDLE ExceptionPort OPTIONAL,
                  IN BOOLEAN InJob)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    PSTRACE(PS_PROCESS_DEBUG,
            "ParentProcess: %p Flags: %lx\n", ParentProcess, Flags);

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe process handle */
            ProbeForWriteHandle(ProcessHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Make sure there's a parent process */
    if (!ParentProcess)
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
                IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                IN HANDLE ParentProcess,
                IN BOOLEAN InheritObjectTable,
                IN HANDLE SectionHandle OPTIONAL,
                IN HANDLE DebugPort OPTIONAL,
                IN HANDLE ExceptionPort OPTIONAL)
{
    ULONG Flags = 0;
    PSTRACE(PS_PROCESS_DEBUG,
            "Parent: %p Attributes: %p\n", ParentProcess, ObjectAttributes);

    /* Set new-style flags */
    if ((ULONG_PTR)SectionHandle & 1) Flags |= PROCESS_CREATE_FLAGS_BREAKAWAY;
    if ((ULONG_PTR)DebugPort & 1) Flags |= PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT;
    if (InheritObjectTable) Flags |= PROCESS_CREATE_FLAGS_INHERIT_HANDLES;

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
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    CLIENT_ID SafeClientId;
    ULONG Attributes = 0;
    HANDLE hProcess;
    BOOLEAN HasObjectName = FALSE;
    PETHREAD Thread = NULL;
    PEPROCESS Process = NULL;
    NTSTATUS Status;
    ACCESS_STATE AccessState;
    AUX_ACCESS_DATA AuxData;
    PAGED_CODE();
    PSTRACE(PS_PROCESS_DEBUG,
            "ClientId: %p Attributes: %p\n", ClientId, ObjectAttributes);

    /* Check if we were called from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the thread handle */
            ProbeForWriteHandle(ProcessHandle);

            /* Check for a CID structure */
            if (ClientId)
            {
                /* Probe and capture it */
                ProbeForRead(ClientId, sizeof(CLIENT_ID), sizeof(ULONG));
                SafeClientId = *ClientId;
                ClientId = &SafeClientId;
            }

            /*
             * Just probe the object attributes structure, don't capture it
             * completely. This is done later if necessary
             */
            ProbeForRead(ObjectAttributes,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));
            HasObjectName = (ObjectAttributes->ObjectName != NULL);

            /* Validate user attributes */
            Attributes = ObpValidateAttributes(ObjectAttributes->Attributes, PreviousMode);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* Otherwise just get the data directly */
        HasObjectName = (ObjectAttributes->ObjectName != NULL);

        /* Still have to sanitize attributes */
        Attributes = ObpValidateAttributes(ObjectAttributes->Attributes, PreviousMode);
    }

    /* Can't pass both, fail */
    if ((HasObjectName) && (ClientId)) return STATUS_INVALID_PARAMETER_MIX;

    /* Create an access state */
    Status = SeCreateAccessState(&AccessState,
                                 &AuxData,
                                 DesiredAccess,
                                 &PsProcessType->TypeInfo.GenericMapping);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if this is a debugger */
    if (SeSinglePrivilegeCheck(SeDebugPrivilege, PreviousMode))
    {
        /* Did he want full access? */
        if (AccessState.RemainingDesiredAccess & MAXIMUM_ALLOWED)
        {
            /* Give it to him */
            AccessState.PreviouslyGrantedAccess |= PROCESS_ALL_ACCESS;
        }
        else
        {
            /* Otherwise just give every other access he could want */
            AccessState.PreviouslyGrantedAccess |=
                AccessState.RemainingDesiredAccess;
        }

        /* The caller desires nothing else now */
        AccessState.RemainingDesiredAccess = 0;
    }

    /* Open by name if one was given */
    if (HasObjectName)
    {
        /* Open it */
        Status = ObOpenObjectByName(ObjectAttributes,
                                    PsProcessType,
                                    PreviousMode,
                                    &AccessState,
                                    0,
                                    NULL,
                                    &hProcess);

        /* Get rid of the access state */
        SeDeleteAccessState(&AccessState);
    }
    else if (ClientId)
    {
        /* Open by Thread ID */
        if (ClientId->UniqueThread)
        {
            /* Get the Process */
            Status = PsLookupProcessThreadByCid(ClientId, &Process, &Thread);
        }
        else
        {
            /* Get the Process */
            Status = PsLookupProcessByProcessId(ClientId->UniqueProcess,
                                                &Process);
        }

        /* Check if we didn't find anything */
        if (!NT_SUCCESS(Status))
        {
            /* Get rid of the access state and return */
            SeDeleteAccessState(&AccessState);
            return Status;
        }

        /* Open the Process Object */
        Status = ObOpenObjectByPointer(Process,
                                       Attributes,
                                       &AccessState,
                                       0,
                                       PsProcessType,
                                       PreviousMode,
                                       &hProcess);

        /* Delete the access state */
        SeDeleteAccessState(&AccessState);

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

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Use SEH for write back */
        _SEH2_TRY
        {
            /* Write back the handle */
            *ProcessHandle = hProcess;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status */
    return Status;
}

/* EOF */
