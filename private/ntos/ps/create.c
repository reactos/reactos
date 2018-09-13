/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    create.c

Abstract:

    Process and Thread Creation.

Author:

    Mark Lucovsky (markl) 20-Apr-1989

Revision History:

--*/

#include "psp.h"

//
// This should really be in MM.H, but it cant be there yet.
//
extern PVOID MmWorkingSetList;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtCreateThread)
#pragma alloc_text(PAGE, NtCreateProcess)
#pragma alloc_text(PAGE, PsCreateSystemThread)
#pragma alloc_text(PAGE, PspCreateThread)
#pragma alloc_text(PAGE, PsCreateSystemProcess)
#pragma alloc_text(PAGE, PspCreateProcess)
#pragma alloc_text(PAGE, PspUserThreadStartup)
#pragma alloc_text(PAGE, PsLockProcess)
#pragma alloc_text(PAGE, PsUnlockProcess)
#pragma alloc_text(PAGE, PsSetLoadImageNotifyRoutine)
#pragma alloc_text(PAGE, PsCallImageNotifyRoutines)
#endif


extern UNICODE_STRING CmCSDVersionString;

LCID PsDefaultSystemLocaleId;
LCID PsDefaultThreadLocaleId;
LANGID PsDefaultUILanguageId;
LANGID PsInstallUILanguageId;

//
// The following two globals are present to make it easier to change
// working set sizes when debugging.
//

ULONG PsMinimumWorkingSet = 20;
ULONG PsMaximumWorkingSet = 45;

BOOLEAN PsImageNotifyEnabled;

//
// Define the local storage for the process lock fast mutex.
//

FAST_MUTEX PspProcessLockMutex;

NTSTATUS
NtCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB InitialTeb,
    IN BOOLEAN CreateSuspended
    )

/*++

Routine Description:

    This system service API creates and initializes a thread object.

Arguments:

    ThreadHandle - Returns the handle for the new thread.

    DesiredAccess - Supplies the desired access modes to the new thread.

    ObjectAttributes - Supplies the object attributes of the new thread.

    ProcessHandle - Supplies a handle to the process that the thread is being
                    created within.

    ClientId - Returns the CLIENT_ID of the new thread.

    ThreadContext - Supplies an initial context for the new thread.

    InitialTeb - Supplies the initial contents for the thread's TEB.

    CreateSuspended - Supplies a value that controls whether or not a
                      thread is created in a suspended state.

Return Value:

    TBD

--*/

{
    NTSTATUS st;
    INITIAL_TEB CapturedInitialTeb;

    PAGED_CODE();

    if ( KeGetPreviousMode() != KernelMode ) {

        //
        // Probe all arguments
        //

        try {
            ProbeForWriteHandle(ThreadHandle);

            if ( ARGUMENT_PRESENT(ClientId) ) {
                ProbeForWriteUlong((PULONG)ClientId);
                ProbeForWrite(ClientId, sizeof(CLIENT_ID), sizeof(ULONG));
            }

            if ( ARGUMENT_PRESENT(ThreadContext) ) {
                ProbeForRead(ThreadContext, sizeof(CONTEXT), CONTEXT_ALIGN);
                }
            else {
                return STATUS_INVALID_PARAMETER;
                }
            ProbeForRead(InitialTeb, sizeof(InitialTeb->OldInitialTeb), sizeof(ULONG));
            CapturedInitialTeb.OldInitialTeb = InitialTeb->OldInitialTeb;
            if (CapturedInitialTeb.OldInitialTeb.OldStackBase == NULL &&
                CapturedInitialTeb.OldInitialTeb.OldStackLimit == NULL
               ) {
                ProbeForRead(InitialTeb, sizeof(INITIAL_TEB), sizeof(ULONG));
                CapturedInitialTeb = *InitialTeb;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    } else {
        if (InitialTeb->OldInitialTeb.OldStackBase == NULL &&
            InitialTeb->OldInitialTeb.OldStackLimit == NULL
           ) {
            CapturedInitialTeb = *InitialTeb;
        } else {
            CapturedInitialTeb.OldInitialTeb = InitialTeb->OldInitialTeb;
        }
    }

    st = PspCreateThread (
            ThreadHandle,
            DesiredAccess,
            ObjectAttributes,
            ProcessHandle,
            NULL,
            ClientId,
            ThreadContext,
            &CapturedInitialTeb,
            CreateSuspended,
            NULL,
            NULL
            );

    return st;
}

NTSTATUS
PsCreateSystemThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine creates and starts a system thread.

Arguments:

    ThreadHandle - Returns the handle for the new thread.

    DesiredAccess - Supplies the desired access modes to the new thread.

    ObjectAttributes - Supplies the object attributes of the new thread.

    ProcessHandle - Supplies a handle to the process that the thread is being
                    created within.  If this parameter is not specified, then
                    the initial system process is used.

    ClientId - Returns the CLIENT_ID of the new thread.

    StartRoutine - Supplies the address of the system thread start routine.

    StartContext - Supplies context for a system thread start routine.

Return Value:

    TBD

--*/

{
    NTSTATUS st;
    HANDLE SystemProcess;
    PEPROCESS ProcessPointer;

    PAGED_CODE();

    ProcessPointer = NULL;
    if (ARGUMENT_PRESENT(ProcessHandle)) {
        SystemProcess = ProcessHandle;
    } else {
        SystemProcess = NULL;
        ProcessPointer = PsInitialSystemProcess;
    }

    st = PspCreateThread (
        ThreadHandle,
        DesiredAccess,
        ObjectAttributes,
        SystemProcess,
        ProcessPointer,
        ClientId,
        NULL,
        NULL,
        FALSE,
        StartRoutine,
        StartContext
        );

    return st;
}


BOOLEAN
PspMarkProcessIdValid(
    IN PHANDLE_TABLE_ENTRY HandleEntry,
    IN ULONG_PTR Parameter
    )
{
    HandleEntry->Object = (PVOID)Parameter;
    return TRUE;
}




NTSTATUS
PspCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    IN PEPROCESS ProcessPointer,
    OUT PCLIENT_ID ClientId OPTIONAL,
    IN PCONTEXT ThreadContext OPTIONAL,
    IN PINITIAL_TEB InitialTeb OPTIONAL,
    IN BOOLEAN CreateSuspended,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine creates and initializes a thread object. It implements the
    foundation for NtCreateThread and for PsCreateSystemThread.

Arguments:

    ThreadHandle - Returns the handle for the new thread.

    DesiredAccess - Supplies the desired access modes to the new thread.

    ObjectAttributes - Supplies the object attributes of the new thread.

    ProcessHandle - Supplies a handle to the process that the thread is being
                    created within.

    ClientId - Returns the CLIENT_ID of the new thread.

    ThreadContext - Supplies a pointer to a context frame that represents the
                initial user-mode context for a user-mode thread. The absence
                of this parameter indicates that a system thread is being
                created.

    InitialTeb - Supplies the contents of certain fields for the new threads
                 TEB. This parameter is only examined if both a trap and
                 exception frame were specified.

    CreateSuspended - Supplies a value that controls whether or not a user-mode
                      thread is created in a suspended state.

    StartRoutine - Supplies the address of the system thread start routine.

    StartContext - Supplies context for a system thread start routine.

Return Value:

    TBD

--*/

{

    HANDLE_TABLE_ENTRY CidEntry;
    NTSTATUS st;
    PETHREAD Thread;
    PEPROCESS Process;
    PVOID KernelStack;
    PTEB Teb;
    INITIAL_TEB ITeb;
    KPROCESSOR_MODE PreviousMode;
    HANDLE LocalThreadHandle;
    BOOLEAN AccessCheck;
    BOOLEAN MemoryAllocated;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    NTSTATUS accesst;
    LARGE_INTEGER NullTime;
    LARGE_INTEGER CreateTime;
    BOOLEAN NeedToFixProcessId = FALSE;

    PAGED_CODE();

    NullTime.LowPart = 0;
    NullTime.HighPart = 0;

    if ( StartRoutine ) {
        PreviousMode = KernelMode;
    } else {
        PreviousMode = KeGetPreviousMode();
    }

    Teb = NULL;

    Thread = NULL;
    Process = NULL;
    KernelStack = NULL;

    if ( ProcessHandle ) {
        //
        // Process object reference count is biased by one for each thread.
        // This accounts for the pointer given to the kernel that remains
        // in effect until the thread terminates (and becomes signaled)
        //

        st = ObReferenceObjectByHandle(
                    ProcessHandle,
                    PROCESS_CREATE_THREAD,
                    PsProcessType,
                    PreviousMode,
                    (PVOID *)&Process,
                    NULL
                    );
        }
    else {
        if ( StartRoutine ) {
            ObReferenceObject(ProcessPointer);
            Process = ProcessPointer;
            st = STATUS_SUCCESS;
            }
        else {
            st = STATUS_INVALID_HANDLE;
            }
        }
    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    //
    // If the previous mode is user and the target process is the system
    // process, then the operation cannot be performed.
    //

    if ((PreviousMode != KernelMode) && (Process == PsInitialSystemProcess)) {
        ObDereferenceObject(Process);
        return STATUS_INVALID_HANDLE;
    }

    st = ObCreateObject(
             PreviousMode,
             PsThreadType,
             ObjectAttributes,
             PreviousMode,
             NULL,
             (ULONG) sizeof(ETHREAD),
             0,
             0,
             (PVOID *)&Thread
             );
    if ( !NT_SUCCESS( st ) ) {
        ObDereferenceObject(Process);
        return st;
    }

    RtlZeroMemory(Thread,sizeof(ETHREAD));
    CidEntry.Object = Thread;
    CidEntry.GrantedAccess = 0;
    Thread->Cid.UniqueThread = ExCreateHandle(PspCidTable,&CidEntry);

    if ( !Thread->Cid.UniqueThread ) {
        ObDereferenceObject(Process);
        ObDereferenceObject(Thread);
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    // Initialize Mm
    //

    Thread->ReadClusterSize = MmReadClusterSize;

    //
    // Initialize LPC
    //

    KeInitializeSemaphore(&Thread->LpcReplySemaphore,0L,1L);
    InitializeListHead( &Thread->LpcReplyChain );

    //
    // Initialize Io
    //

    InitializeListHead(&Thread->IrpList);

    //
    // Initialize Registry
    //

    InitializeListHead(&Thread->PostBlockList);


    //
    // Initialize Security
    //

    PspInitializeThreadSecurity( Process, Thread );

    //

    InitializeListHead(&Thread->TerminationPortList);

    KeInitializeSpinLock(&Thread->ActiveTimerListLock);
    InitializeListHead(&Thread->ActiveTimerListHead);

    //
    // Allocate Kernel Stack
    //

    KernelStack = MmCreateKernelStack(FALSE);
    if ( !KernelStack ) {
        ObDereferenceObject(Process);
        ObDereferenceObject(Thread);

        return STATUS_UNSUCCESSFUL;
    }

    st = PsLockProcess(Process,KernelMode,PsLockPollOnTimeout);
    if ( st != STATUS_SUCCESS ) {
        MmDeleteKernelStack(KernelStack, FALSE);
        ObDereferenceObject(Process);
        ObDereferenceObject(Thread);

        return STATUS_PROCESS_IS_TERMINATING;
        }


    //
    // If the process does not have its part of the client id, then
    // assign one
    //

    if ( !Process->UniqueProcessId ) {
        CidEntry.Object = (PVOID)PSP_INVALID_ID;
        CidEntry.GrantedAccess = 0;
        Process->UniqueProcessId = ExCreateHandle(PspCidTable,&CidEntry);
        ExSetHandleTableOwner( Process->ObjectTable, Process->UniqueProcessId );
        if (!Process->UniqueProcessId) {
            PsUnlockProcess(Process);

            MmDeleteKernelStack(KernelStack, FALSE);
            ObDereferenceObject(Process);
            ObDereferenceObject(Thread);

            return STATUS_UNSUCCESSFUL;
        }

        NeedToFixProcessId = TRUE;

        PERFINFO_PROCESS_CREATE(Process);
    }
    Thread->Cid.UniqueProcess = Process->UniqueProcessId;

    Thread->ThreadsProcess = Process;

    if (ARGUMENT_PRESENT(ThreadContext)) {

        //
        // FIX. Handle exception on bad context
        //

        //
        // User-mode thread
        //

        try {

            ITeb = *InitialTeb;

            Teb = MmCreateTeb ( Process, &ITeb, &Thread->Cid );

            //
            // Initialize kernel thread object for user mode thread.
            //

            Thread->StartAddress = (PVOID)CONTEXT_TO_PROGRAM_COUNTER(ThreadContext);
#if defined(_IA64_)
            Thread->Win32StartAddress = (PVOID)ThreadContext->IntT0;
#endif // _IA64_

#if defined(_X86_)
            Thread->Win32StartAddress = (PVOID)ThreadContext->Eax;
#endif // _X86_

#if defined(_MIPS_)
            Thread->Win32StartAddress = (PVOID)ThreadContext->XIntA0;
#endif // _MIPS_

#if defined(_ALPHA_)
            Thread->Win32StartAddress = (PVOID)ThreadContext->IntA0;
#endif // _ALPHA_

#if defined(_PPC_)
            Thread->Win32StartAddress = (PVOID)ThreadContext->Gpr3;
#endif // _PPC_

            (VOID)
            KeInitializeThread(
                &Thread->Tcb,
                KernelStack,
                PspUserThreadStartup,
                (PKSTART_ROUTINE)NULL,
                (PVOID)CONTEXT_TO_PROGRAM_COUNTER(ThreadContext),
                ThreadContext,
                Teb,
                &Process->Pcb
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {


            if ( Teb ) {
                MmDeleteTeb(Process, Teb);
            }

            PsUnlockProcess(Process);

             MmDeleteKernelStack(KernelStack, FALSE);
             Thread->Tcb.InitialStack = NULL;
             ObDereferenceObject(Thread);

             return GetExceptionCode();
        }

    } else {

        //
        // Initialize kernel thread object for kernel mode thread.
        //

        Thread->StartAddress = (PVOID)StartRoutine;
        KeInitializeThread(
            &Thread->Tcb,
            KernelStack,
            PspSystemThreadStartup,
            StartRoutine,
            StartContext,
            NULL,
            NULL,
            &Process->Pcb
            );

    }

    InsertTailList(&Process->ThreadListHead,&Thread->ThreadListEntry);

    if ( NeedToFixProcessId ) {

        //
        // Now that the thread is really there and will rundown through exit,
        // open up the process id
        //

        ExChangeHandle(PspCidTable, Thread->Cid.UniqueProcess, PspMarkProcessIdValid, (ULONG_PTR)Process);

        if (PspCreateProcessNotifyRoutineCount != 0) {
            ULONG i;

            for (i=0; i<PSP_MAX_CREATE_PROCESS_NOTIFY; i++) {
                if (PspCreateProcessNotifyRoutine[i] != NULL) {
                    (*PspCreateProcessNotifyRoutine[i])( Process->InheritedFromUniqueProcessId,
                                                         Process->UniqueProcessId,
                                                         TRUE
                                                       );
                    }
                }
            }


        }

    //
    // If the process has a job with a completion port,
    // AND if the process is really considered to be in the Job, AND
    // the process has not reported, report in
    //
    // This should really be done in add process to job, but can't
    // in this path because the process's ID isn't assigned until this point
    // in time
    //

    if ( Process->Job
         && Process->Job->CompletionPort
         && !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)
         && !(Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)) {

        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NEW_PROCESS_REPORTED);

        ExAcquireFastMutex(&Process->Job->MemoryLimitsLock);
        if (Process->Job->CompletionPort != NULL) {
            IoSetIoCompletion(
                Process->Job->CompletionPort,
                Process->Job->CompletionKey,
                (PVOID)Process->UniqueProcessId,
                STATUS_SUCCESS,
                JOB_OBJECT_MSG_NEW_PROCESS,
                FALSE
                );
        }
        ExReleaseFastMutex(&Process->Job->MemoryLimitsLock);
        }

    PERFINFO_THREAD_CREATE(Thread, InitialTeb);

    //
    // Notify registered callout routines of thread creation.
    //

    if (PspCreateThreadNotifyRoutineCount != 0) {
        ULONG i;

        for (i=0; i<PSP_MAX_CREATE_THREAD_NOTIFY; i++) {
            if (PspCreateThreadNotifyRoutine[i] != NULL) {
                (*PspCreateThreadNotifyRoutine[i])( Thread->Cid.UniqueProcess,
                                                    Thread->Cid.UniqueThread,
                                                    TRUE
                                                   );
            }
        }
    }

    (VOID) KeEnableApcQueuingThread(&Thread->Tcb);

    if (CreateSuspended) {
        (VOID) KeSuspendThread(&Thread->Tcb);
    }


    PsUnlockProcess(Process);

    //
    // Failures that occur after this point cause the thread to
    // go through PspExitThread
    //

    //
    // Reference count of thread is biased once for itself, and
    // once to account for its Cid Handle
    //
    // Note:
    //      if this fails we should do punt so we only have one
    //      cleanup path that really goes through PspExitThread ?
    //      Remember to free spin locks, cid...
    //

    ObReferenceObject(Thread);
    ObReferenceObject(Thread);
    ObReferenceObject(Thread);

    st = ObInsertObject(
            Thread,
            NULL,
            DesiredAccess,
            0,
            (PVOID *)NULL,
            &LocalThreadHandle
            );

    if ( !NT_SUCCESS(st) ) {

        //
        // The insert failed. Terminate the thread.
        //

        //
        // This trick us used so that Dbgk doesn't report
        // events for dead threads
        //

        Thread->DeadThread = TRUE;
        Thread->HasTerminated = TRUE;

        if (CreateSuspended) {
            (VOID) KeResumeThread(&Thread->Tcb);
        }

    } else {

        try {

            *ThreadHandle = LocalThreadHandle;
            if (ARGUMENT_PRESENT(ClientId)) {
                *ClientId = Thread->Cid;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {

            if ( GetExceptionCode() == STATUS_QUOTA_EXCEEDED ) {

                //
                // This trick us used so that Dbgk doesn't report
                // events for dead threads
                //

                Thread->DeadThread = TRUE;
                Thread->HasTerminated = TRUE;

                if (CreateSuspended) {
                    (VOID) KeResumeThread(&Thread->Tcb);
                }
                KeReadyThread(&Thread->Tcb);
                ObDereferenceObject(Thread);
                ZwClose(LocalThreadHandle);
                return GetExceptionCode();
            }
        }
    }

    KeQuerySystemTime(&CreateTime);
    ASSERT ((CreateTime.HighPart & 0xf0000000) == 0);
    PS_SET_THREAD_CREATE_TIME(Thread, CreateTime);

    if ( !Thread->DeadThread ) {
        st = ObGetObjectSecurity(
                Thread,
                &SecurityDescriptor,
                &MemoryAllocated
                );
        if ( !NT_SUCCESS(st) ) {
            //
            // This trick us used so that Dbgk doesn't report
            // events for dead threads
            //

            Thread->DeadThread = TRUE;
            Thread->HasTerminated = TRUE;

            if (CreateSuspended) {
                (VOID) KeResumeThread(&Thread->Tcb);
            }
            KeReadyThread(&Thread->Tcb);
            ObDereferenceObject(Thread);
            ZwClose(LocalThreadHandle);
            return st;
            }

        //
        // Compute the subject security context
        //

        SubjectContext.ProcessAuditId = Process;
        SubjectContext.PrimaryToken = PsReferencePrimaryToken(Process);
        SubjectContext.ClientToken = NULL;

        AccessCheck = SeAccessCheck(
                        SecurityDescriptor,
                        &SubjectContext,
                        FALSE,
                        MAXIMUM_ALLOWED,
                        0,
                        NULL,
                        &PsThreadType->TypeInfo.GenericMapping,
                        PreviousMode,
                        &Thread->GrantedAccess,
                        &accesst
                        );
        PsDereferencePrimaryToken(SubjectContext.PrimaryToken);
        ObReleaseObjectSecurity(
            SecurityDescriptor,
            MemoryAllocated
            );

        if ( !AccessCheck ) {
            Thread->GrantedAccess = 0;
            }

        if ( (Thread->GrantedAccess & THREAD_SET_THREAD_TOKEN) == 0 )
        {
            DbgPrint( "SE: Warning, new thread does not have SET_THREAD_TOKEN for itself\n" );
            DbgPrint( "SE: Check that thread %x.%x isn't in some weird state\n",
                        PsGetCurrentThread()->Cid.UniqueProcess,
                        PsGetCurrentThread()->Cid.UniqueThread );

        }

        Thread->GrantedAccess |= (THREAD_TERMINATE | THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION);

        }
    else {
        Thread->GrantedAccess = THREAD_ALL_ACCESS;
        }
    KeReadyThread(&Thread->Tcb);
    ObDereferenceObject(Thread);

    return st;
}

NTSTATUS
NtCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
    )

/*++

Routine Description:

    This routine creates a process object.

Arguments:

    ProcessHandle - Returns the handle for the new process.

    DesiredAccess - Supplies the desired access modes to the new process.

    ObjectAttributes - Supplies the object attributes of the new process.
    .
    .
    .

Return Value:

    TBD

--*/

{
    NTSTATUS st;

    PAGED_CODE();

    if ( KeGetPreviousMode() != KernelMode ) {

        //
        // Probe all arguments
        //

        try {
            ProbeForWriteHandle(ProcessHandle);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    if ( ARGUMENT_PRESENT(ParentProcess) ) {
        st = PspCreateProcess(
                ProcessHandle,
                DesiredAccess,
                ObjectAttributes,
                ParentProcess,
                InheritObjectTable,
                SectionHandle,
                DebugPort,
                ExceptionPort
                );
    } else {
        st = STATUS_INVALID_PARAMETER;
    }

    return st;
}


NTSTATUS
PsCreateSystemProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    )

/*++

Routine Description:

    This routine creates a system process object. A system process
    has an address space that is initialized to an empty address space
    that maps the system.

    The process inherits its access token and other attributes from the
    initial system process. The process is created with an empty handle table.

Arguments:

    ProcessHandle - Returns the handle for the new process.

    DesiredAccess - Supplies the desired access modes to the new process.

    ObjectAttributes - Supplies the object attributes of the new process.


Return Value:

    TBD

--*/

{
    NTSTATUS st;

    PAGED_CODE();

    st = PspCreateProcess(
            ProcessHandle,
            DesiredAccess,
            ObjectAttributes,
            PspInitialSystemProcessHandle,
            FALSE,
            (HANDLE) NULL,
            (HANDLE) NULL,
            (HANDLE) NULL
            );

    return st;
}



NTSTATUS
PspCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess OPTIONAL,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
    )

/*++

Routine Description:

    This routine creates and initializes a process object.  It implements the
    foundation for NtCreateProcess and for system initialization process
    creation.

Arguments:

    ProcessHandle - Returns the handle for the new process.

    DesiredAccess - Supplies the desired access modes to the new process.

    ObjectAttributes - Supplies the object attributes of the new process.

    ParentProcess - Supplies a handle to the process' parent process.  If this
                    parameter is not specified, then the process has no parent
                    and is created using the system address space.

    SectionHandle - Supplies a handle to a section object to be used to create
                    the process' address space.  If this parameter is not
                    specified, then the address space is simply a clone of the
                    parent process' address space.

    DebugPort - Supplies a handle to a port object that will be used as the
                process' debug port.

    ExceptionPort - Supplies a handle to a port object that will be used as the
                    process' exception port.

Return Value:

    TBD

--*/

{
    NTSTATUS st;
    PEPROCESS Process;
    PEPROCESS Parent;
    KAFFINITY Affinity;
    KPRIORITY BasePriority;
    PVOID SectionToMap;
    PVOID ExceptionPortObject;
    PVOID DebugPortObject;
    ULONG WorkingSetMinimum, WorkingSetMaximum;
    HANDLE LocalProcessHandle;
    KPROCESSOR_MODE PreviousMode;
    HANDLE NewSection;
    NTSTATUS DuplicateStatus;
    INITIAL_PEB InitialPeb;
    BOOLEAN CreatePeb;
    ULONG_PTR DirectoryTableBase[2];
    BOOLEAN AccessCheck;
    BOOLEAN MemoryAllocated;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    NTSTATUS accesst;
    NTSTATUS savedst;
    BOOLEAN BreakAwayRequested;
    PUNICODE_STRING AuditName = NULL ;

    PAGED_CODE();

    BreakAwayRequested = FALSE;
    CreatePeb = FALSE;
    DirectoryTableBase[0] = 0;
    DirectoryTableBase[1] = 0;
    PreviousMode = KeGetPreviousMode();

    //
    // Parent
    //

    if (ARGUMENT_PRESENT(ParentProcess) ) {
        st = ObReferenceObjectByHandle(
                ParentProcess,
                PROCESS_CREATE_PROCESS,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Parent,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
        }

        //
        // Until CSR understands priority class, don't
        // inherit base priority. This just makes things
        // worse !
        //

        BasePriority = (KPRIORITY) NORMAL_BASE_PRIORITY;

        //
        //BasePriority = Parent->Pcb.BasePriority;
        //

        Affinity = Parent->Pcb.Affinity;

        WorkingSetMinimum = PsMinimumWorkingSet;              // FIXFIX
        WorkingSetMaximum = PsMaximumWorkingSet;


    } else {

        Parent = NULL;
        Affinity = KeActiveProcessors;
        BasePriority = (KPRIORITY) NORMAL_BASE_PRIORITY;

        WorkingSetMinimum = PsMinimumWorkingSet;              // FIXFIX
        WorkingSetMaximum = PsMaximumWorkingSet;
    }

    //
    // Section
    //

    if (ARGUMENT_PRESENT(SectionHandle) ) {

        //
        // Use Object manager tag bits to indicate that breakaway is
        // desired
        //

        if ( (UINT_PTR)SectionHandle & 1 ) {
            BreakAwayRequested = TRUE;
            }

        st = ObReferenceObjectByHandle(
                SectionHandle,
                SECTION_MAP_EXECUTE,
                MmSectionObjectType,
                PreviousMode,
                (PVOID *)&SectionToMap,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            if (Parent) {
                ObDereferenceObject(Parent);
            }
            return st;
        }
    } else {
        SectionToMap = NULL;
    }

    //
    // DebugPort
    //

    if (ARGUMENT_PRESENT(DebugPort) ) {
        st = ObReferenceObjectByHandle (
                DebugPort,
                0,
                LpcPortObjectType,
                KeGetPreviousMode(),
                (PVOID *)&DebugPortObject,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            if (Parent) {
                ObDereferenceObject(Parent);
            }
            if (SectionToMap) {
                ObDereferenceObject(SectionToMap);
            }
            return st;
        }
    } else {
        DebugPortObject = NULL;
    }

    //
    // ExceptionPort
    //

    if (ARGUMENT_PRESENT(ExceptionPort) ) {
        st = ObReferenceObjectByHandle (
                ExceptionPort,
                0,
                LpcPortObjectType,
                KeGetPreviousMode(),
                (PVOID *)&ExceptionPortObject,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            if (Parent) {
                ObDereferenceObject(Parent);
            }
            if (SectionToMap) {
                ObDereferenceObject(SectionToMap);
            }
            if (DebugPortObject) {
                ObDereferenceObject(DebugPortObject);
            }

            return st;
        }
    } else {
        ExceptionPortObject = NULL;
    }

    st = ObCreateObject(
           KeGetPreviousMode(),
           PsProcessType,
           ObjectAttributes,
           KeGetPreviousMode(),
           NULL,
           (ULONG) sizeof(EPROCESS),
           0,
           0,
           (PVOID *)&Process
           );
    if ( !NT_SUCCESS( st ) ) {
        if (Parent) {
            ObDereferenceObject(Parent);
        }
        if (SectionToMap) {
            ObDereferenceObject(SectionToMap);
        }
        if (DebugPortObject) {
            ObDereferenceObject(DebugPortObject);
        }
        if (ExceptionPortObject) {
            ObDereferenceObject(ExceptionPortObject);
        }
        return st;
    }

    //
    // The process object is created set to NULL. Errors
    // That occur after this step cause the process delete
    // routine to be entered.
    //
    // Teardown actions that occur in the process delete routine
    // do not need to be performed inline.
    //

    RtlZeroMemory(Process,sizeof(EPROCESS));

    InitializeListHead(&Process->ThreadListHead);
    Process->CreateProcessReported = FALSE;
    Process->DebugPort = DebugPortObject;
    Process->ExceptionPort = ExceptionPortObject;


    PspInheritQuota(Process,Parent);
    ObInheritDeviceMap(Process,Parent);

    if ( Parent ) {
        Process->DefaultHardErrorProcessing = Parent->DefaultHardErrorProcessing;
        Process->InheritedFromUniqueProcessId = Parent->UniqueProcessId;
        Process->SessionId = Parent->SessionId;

    } else {
        Process->DefaultHardErrorProcessing = 1;
        Process->InheritedFromUniqueProcessId = NULL;
    }

    Process->ExitStatus = STATUS_PENDING;
    Process->LockCount = 1;
    Process->LockOwner = NULL;
    KeInitializeEvent(&Process->LockEvent, SynchronizationEvent, FALSE);

    //
    //  Initialize the security fields of the process
    //  The parent may be null exactly once (during system init).
    //  Thereafter, a parent is always required so that we have a
    //  security context to duplicate for the new process.
    //

    st = PspInitializeProcessSecurity( Parent, Process );

    if (!NT_SUCCESS(st)) {


        if ( Parent ) {
            ObDereferenceObject(Parent);
        }

        if (SectionToMap) {
            ObDereferenceObject(SectionToMap);
        }

        ObDereferenceObject(Process);
        return st;
    }

    //
    // Clone parent's object table.
    // If no parent (booting) then use the current object table created in
    // ObInitSystem.
    //

    if (Parent) {


        //
        // Calculate address space
        //
        //      If Parent == PspInitialSystem
        //

        if (!MmCreateProcessAddressSpace(WorkingSetMinimum,
                                         Process,
                                         &DirectoryTableBase[0])) {

            ObDereferenceObject(Parent);
            if (SectionToMap) {
                ObDereferenceObject(SectionToMap);
            }

            PspDeleteProcessSecurity( Process );

            ObDereferenceObject(Process);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {

        Process->ObjectTable = PsGetCurrentProcess()->ObjectTable;

        DirectoryTableBase[0] = PsGetCurrentProcess()->Pcb.DirectoryTableBase[0];
        DirectoryTableBase[1] = PsGetCurrentProcess()->Pcb.DirectoryTableBase[1];

        //
        // Initialize the Working Set Mutex and address creation mutex
        // for this "hand built" process.
        // Normally, the call the MmInitializeAddressSpace initializes the
        // working set mutex, however, in this case, we have already initialized
        // the address space and we are now creating a second process using
        // the address space of the idle thread.
        //

        //KeInitializeMutant(&Process->WorkingSetLock, FALSE);
        ExInitializeFastMutex(&Process->WorkingSetLock);

        ExInitializeFastMutex(&Process->AddressCreationLock);

        KeInitializeSpinLock (&Process->HyperSpaceLock);

        //
        // Initialize virtual address descriptor root.
        //

        ASSERT (Process->VadRoot == NULL);
        Process->Vm.WorkingSetSize = PsGetCurrentProcess()->Vm.WorkingSetSize;
        KeQuerySystemTime(&Process->Vm.LastTrimTime);
        Process->Vm.VmWorkingSetList = MmWorkingSetList;
    }

    Process->Vm.MaximumWorkingSetSize = WorkingSetMaximum;

    KeInitializeProcess(
        &Process->Pcb,
        BasePriority,
        Affinity,
        &DirectoryTableBase[0],
        (BOOLEAN)(Process->DefaultHardErrorProcessing & PROCESS_HARDERROR_ALIGNMENT_BIT)
        );
    Process->Pcb.ThreadQuantum = PspForegroundQuantum[0];
    Process->PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;

    if (Parent) {

        //
        // this used to happen in basesrv\srvtask.c
        //
        if ( Parent->PriorityClass == PROCESS_PRIORITY_CLASS_IDLE ||
             Parent->PriorityClass == PROCESS_PRIORITY_CLASS_BELOW_NORMAL ) {
             Process->PriorityClass = Parent->PriorityClass;
             }
        //
        // if address space creation worked, then when going through
        // delete, we will attach. Of course, attaching means that the kprocess
        // must be initialized, so we delay the object stuff till here.

        //
        st = ObInitProcess(InheritObjectTable ? Parent : (PEPROCESS)NULL,Process);

        if (!NT_SUCCESS(st)) {

            ObDereferenceObject(Parent);
            if (SectionToMap) {
                ObDereferenceObject(SectionToMap);
            }

            PspDeleteProcessSecurity( Process );
            ObDereferenceObject(Process);
            return st;
        }
    }

    st = STATUS_SUCCESS;
    savedst = STATUS_SUCCESS;

    //
    // Initialize the process address space
    // The address space has four possibilities
    //
    //      1 - Boot Process. Address space is initialized during
    //          MmInit. Parent is not specified.
    //
    //      2 - System Process. Address space is a virgin address
    //          space that only maps system space. Process is same
    //          as PspInitialSystemProcess.
    //
    //      3 - User Process (Cloned Address Space). Address space
    //          is cloned from the specified process.
    //
    //      4 - User Process (New Image Address Space). Address space
    //          is initialized so that it maps the specified section.
    //

    if ( SectionToMap ) {
        //
        // User Process (New Image Address Space). Don't specify Process to
        // clone, just SectionToMap.
        //

        st = MmInitializeProcessAddressSpace(
                Process,
                NULL,
                SectionToMap,
                &AuditName
                );

        ObDereferenceObject(SectionToMap);
        ObInitProcess2(Process);

        if ( NT_SUCCESS(st) ) {

            //
            // In order to support relocating executables, the proper status
            // (STATUS_IMAGE_NOT_AT_BASE) must be returned, so save it here.
            //

            savedst = st;

            st = PspMapSystemDll(Process,NULL);
        }

        CreatePeb = TRUE;

        goto insert_process;
    }

    if ( Parent ) {

        if ( Parent != PsInitialSystemProcess ) {

            Process->SectionBaseAddress = Parent->SectionBaseAddress;

            //
            // User Process ( Cloned Address Space ).  Don't specify section to
            // map, just Process to clone.
            //

            st = MmInitializeProcessAddressSpace(
                    Process,
                    Parent,
                    NULL,
                    NULL
                    );

            CreatePeb = TRUE;

        } else {

            //
            // System Process.  Don't specify Process to clone or section to map
            //

            st = MmInitializeProcessAddressSpace(
                    Process,
                    NULL,
                    NULL,
                    NULL
                    );
        }
    }

insert_process:

    //
    // If MmInitializeProcessAddressSpace was NOT successful, then
    // dereference and exit.
    //

    if ( !NT_SUCCESS(st) ) {

        if (Parent) {
            ObDereferenceObject(Parent);
        }

        KeAttachProcess(&Process->Pcb);
        ObKillProcess(FALSE, Process);
        KeDetachProcess();

        PspDeleteProcessSecurity( Process );
        ObDereferenceObject(Process);
        return st;
    }

    //
    // Reference count of process is not biased here. Each thread in the
    // process bias the reference count when they are created.
    //

    st = ObInsertObject(
            Process,
            NULL,
            DesiredAccess,
            0,
            (PVOID *)NULL,
            &LocalProcessHandle
            );


    if ( !NT_SUCCESS(st) ) {
        if (Parent) {
            ObDereferenceObject(Parent);
        }
        return st;
    }

    //
    // See if the parent has a job. If so reference the job
    // and add the process in.
    //

    if ( Parent && Parent->Job && !(Parent->Job->LimitFlags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK) ) {

        if ( BreakAwayRequested ) {
            if ( !(Parent->Job->LimitFlags & JOB_OBJECT_LIMIT_BREAKAWAY_OK) ) {
                st = STATUS_ACCESS_DENIED;
                if (Parent) {
                    ObDereferenceObject(Parent);
                    }
                ZwClose(LocalProcessHandle);
                return st;
                }
            }
        else {
            ObReferenceObject(Parent->Job);
            Process->Job = Parent->Job;
            st = PspAddProcessToJob(Process->Job,Process);
            if ( !NT_SUCCESS(st) ) {
                if (Parent) {
                    ObDereferenceObject(Parent);
                    }
                ZwClose(LocalProcessHandle);
                return st;
                }
            }
        }

    PsSetProcessPriorityByClass(Process,PsProcessPriorityBackground);

    ExAcquireFastMutex(&PspActiveProcessMutex);
    InsertTailList(&PsActiveProcessHead,&Process->ActiveProcessLinks);
    ExReleaseFastMutex(&PspActiveProcessMutex);

    if (Parent && CreatePeb ) {

        //
        // For processes created w/ a section,
        // a new "virgin" PEB is created. Otherwise,
        // for forked processes, uses inherited PEB
        // with an updated mutant.

        RtlZeroMemory(&InitialPeb, FIELD_OFFSET(INITIAL_PEB, Mutant));
        InitialPeb.Mutant = (HANDLE)(-1);
        if ( SectionToMap ) {

            try {
                Process->Peb = MmCreatePeb(Process,&InitialPeb);
            } except(EXCEPTION_EXECUTE_HANDLER) {
                ObDereferenceObject(Parent);
                ZwClose(LocalProcessHandle);
                return GetExceptionCode();
            }

        } else {

            InitialPeb.InheritedAddressSpace = TRUE;

            Process->Peb = Parent->Peb;

            ZwWriteVirtualMemory(
                LocalProcessHandle,
                Process->Peb,
                &InitialPeb,
                sizeof(INITIAL_PEB),
                NULL
                );
        }

        //
        // The new process should have a handle to its
        // section. The section is either from the specified
        // section, or the section of its parent.
        //

        if ( ARGUMENT_PRESENT(SectionHandle) ) {
            DuplicateStatus = ZwDuplicateObject(
                                NtCurrentProcess(),
                                SectionHandle,
                                LocalProcessHandle,
                                &NewSection,
                                0L,
                                0L,
                                DUPLICATE_SAME_ACCESS
                                );
        } else {

            DuplicateStatus = ZwDuplicateObject(
                                ParentProcess,
                                Parent->SectionHandle,
                                LocalProcessHandle,
                                &NewSection,
                                0L,
                                0L,
                                DUPLICATE_SAME_ACCESS
                                );
        }

        if ( NT_SUCCESS(DuplicateStatus) ) {
            Process->SectionHandle = NewSection;
        }

        ObDereferenceObject(Parent);
    }

    if ( Parent && ParentProcess != PspInitialSystemProcessHandle ) {

        st = ObGetObjectSecurity(
                Process,
                &SecurityDescriptor,
                &MemoryAllocated
                );
        if ( !NT_SUCCESS(st) ) {
            ZwClose(LocalProcessHandle);
            return st;
            }

        //
        // Compute the subject security context
        //

        SubjectContext.ProcessAuditId = Process;
        SubjectContext.PrimaryToken = PsReferencePrimaryToken(Process);
        SubjectContext.ClientToken = NULL;
        AccessCheck = SeAccessCheck(
                        SecurityDescriptor,
                        &SubjectContext,
                        FALSE,
                        MAXIMUM_ALLOWED,
                        0,
                        NULL,
                        &PsProcessType->TypeInfo.GenericMapping,
                        PreviousMode,
                        &Process->GrantedAccess,
                        &accesst
                        );
        PsDereferencePrimaryToken(SubjectContext.PrimaryToken);
        ObReleaseObjectSecurity(
            SecurityDescriptor,
            MemoryAllocated
            );

        if ( !AccessCheck ) {
            Process->GrantedAccess = 0;
            }

        //
        // It does not make any sense to create a process that can not
        // do anything to itself
        //

        Process->GrantedAccess |= (PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | PROCESS_CREATE_THREAD | PROCESS_DUP_HANDLE | PROCESS_CREATE_PROCESS | PROCESS_SET_INFORMATION);

    } else {
        Process->GrantedAccess = PROCESS_ALL_ACCESS;
    }

    if ( SeDetailedAuditing ) {

        SeAuditProcessCreation( Process, Parent, AuditName );
    } 

    KeQuerySystemTime(&Process->CreateTime);

    try {
        *ProcessHandle = LocalProcessHandle;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        return st;
    }

    if (savedst != STATUS_SUCCESS) {
        st = savedst;
    }

    return st;

}

NTSTATUS
PsSetCreateProcessNotifyRoutine(
    IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
    IN BOOLEAN Remove
    )

/*++

Routine Description:

    This function allows an installable file system to hook into process
    creation and deletion to track those events against their own internal
    data structures.

Arguments:

    NotifyRoutine - Supplies the address of a routine which is called at
        process creation and deletion. The routine is passed the unique Id
        of the created or deleted process and the parent process if it was
        created with the inherit handles option. If it was created without
        the inherit handle options, then the parent process Id will be NULL.
        The third parameter passed to the notify routine is TRUE if the process
        is being created and FALSE if it is being deleted.

        The callout for creation happens just after the first thread in the
        process has been created. The callout for deletion happens after the
        last thread in a process has terminated and the address space is about
        to be deleted. It is possible to get a deletion call without a creation
        call if the pathological case where a process is created and deleted
        without a thread ever being created.

    Remove - FALSE specifies to install the callout and TRUE specifies to
        remove the callout that mat

Return Value:

    STATUS_SUCCESS if successful, and STATUS_INVALID_PARAMETER if not.

--*/

{

    ULONG i;

    for (i=0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++) {
        if (Remove) {
            if (PspCreateProcessNotifyRoutine[i] == NotifyRoutine) {
                PspCreateProcessNotifyRoutine[i] = NULL;
                PspCreateProcessNotifyRoutineCount -= 1;
                return STATUS_SUCCESS;
            }

        } else {
            if (PspCreateProcessNotifyRoutine[i] == NULL) {
                PspCreateProcessNotifyRoutine[i] = NotifyRoutine;
                PspCreateProcessNotifyRoutineCount += 1;
                return STATUS_SUCCESS;
            }
        }
    }

    return Remove ? STATUS_PROCEDURE_NOT_FOUND : STATUS_INVALID_PARAMETER;
}

NTSTATUS
PsSetCreateThreadNotifyRoutine(
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    )

/*++

Routine Description:

    This function allows an installable file system to hook into thread
    creation and deletion to track those events against their own internal
    data structures.

Arguments:

    NotifyRoutine - Supplies the address of the routine which is called at
        thread creation and deletion. The routine is passed the unique Id
        of the created or deleted thread and the unique Id of the containing
        process. The third parameter passed to the notify routine is TRUE if
        the thread is being created and FALSE if it is being deleted.

Return Value:

    STATUS_SUCCESS if successful, and STATUS_INSUFFICIENT_RESOURCES if not.

--*/

{

    ULONG i;
    NTSTATUS Status;

    Status = STATUS_INSUFFICIENT_RESOURCES;
    for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i += 1) {
        if (PspCreateThreadNotifyRoutine[i] == NULL) {
            PspCreateThreadNotifyRoutine[i] = NotifyRoutine;
            PspCreateThreadNotifyRoutineCount += 1;
            Status = STATUS_SUCCESS;
            break;
        }
    }

    return Status;
}

VOID
PspUserThreadStartup(
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This function is called by the kernel to start a user-mode thread.

Arguments:

    StartRoutine - Ignored.

    StartContext - Supplies the initial pc value for the thread.

Return Value:

    None.

--*/

{
    PETHREAD Thread;
    PKAPC StartApc;
    PEPROCESS Process;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(StartRoutine);

    Process = PsGetCurrentProcess();

    //
    // All threads start with an APC at LdrInitializeThunk
    //

    MmAllowWorkingSetExpansion();

    Thread = PsGetCurrentThread();

    if ( !Thread->DeadThread && !Thread->HasTerminated ) {

        StartApc = ExAllocatePool(NonPagedPoolMustSucceed,(ULONG)sizeof(KAPC));

        ((PTEB)PsGetCurrentThread()->Tcb.Teb)->CurrentLocale = PsDefaultThreadLocaleId;

        KeInitializeApc(
            StartApc,
            &Thread->Tcb,
            OriginalApcEnvironment,
            PspNullSpecialApc,
            NULL,
            PspSystemDll.LoaderInitRoutine,
            UserMode,
            NULL
            );

        if ( !KeInsertQueueApc(StartApc,(PVOID) PspSystemDll.DllBase,NULL,0) ) {
            ExFreePool(StartApc);
        } else {
            Thread->Tcb.ApcState.UserApcPending = TRUE;
        }
    } else {

        if ( !Thread->DeadThread ) {

            //
            // If DeadThread is not set, then it means the thread was terminated before
            // it started, but creation was ok. Need to let debuggers see these threads
            // for an instant because if they are the last to exit, the exitprocess
            // message gets nuked
            //

            KeLowerIrql(0);
            DbgkCreateThread(StartContext);

            }
        PspExitThread(STATUS_THREAD_IS_TERMINATING);
    }

    KeLowerIrql(0);

    DbgkCreateThread(StartContext);

    if ( Process->Pcb.UserTime == 0 ) {
        Process->Pcb.UserTime = 1;
    }

}



ULONG
PspUnhandledExceptionInSystemThread(
    IN PEXCEPTION_POINTERS ExceptionPointers
    )
{
    KdPrint(("PS: Unhandled Kernel Mode Exception Pointers = 0x%p\n", ExceptionPointers));
    KdPrint(("Code %x Addr %p Info0 %p Info1 %p Info2 %p Info3 %p\n",
        ExceptionPointers->ExceptionRecord->ExceptionCode,
        (ULONG_PTR)ExceptionPointers->ExceptionRecord->ExceptionAddress,
        ExceptionPointers->ExceptionRecord->ExceptionInformation[0],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[1],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[2],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[3]
        ));

    KeBugCheckEx(
        KMODE_EXCEPTION_NOT_HANDLED,
        ExceptionPointers->ExceptionRecord->ExceptionCode,
        (ULONG_PTR)ExceptionPointers->ExceptionRecord->ExceptionAddress,
        ExceptionPointers->ExceptionRecord->ExceptionInformation[0],
        ExceptionPointers->ExceptionRecord->ExceptionInformation[1]
        );
    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
PspSystemThreadStartup(
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    )

/*++

Routine Description:

    This function is called by the kernel to start a system thread.

Arguments:

    StartRoutine - Supplies the address of the system threads entry point.

    StartContext - Supplies a context value for the system thread.

Return Value:

    None.

--*/

{

        PETHREAD Thread;

        MmAllowWorkingSetExpansion();

        KeLowerIrql(0);

        Thread = PsGetCurrentThread();

        try {
            if ( !Thread->DeadThread && !Thread->HasTerminated ) {
                (StartRoutine)(StartContext);
                }
            }
        except (PspUnhandledExceptionInSystemThread(GetExceptionInformation())) {
            KeBugCheck(KMODE_EXCEPTION_NOT_HANDLED);
            }
        PspExitThread(STATUS_SUCCESS);

}

NTSTATUS
PsLockProcess(
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE WaitMode,
    IN PSLOCKPROCESSMODE LockMode
    )

/*++

Routine Description:

    This function is used to lock the process from create/delete and to
    freeze threads from entering/exiting the process.

Arguments:

    Process - Pointer to the process to lock

    WaitMode - Supplies the processor mode to issue the wait under

    LockMode - The type of lock to attempt

                PsLockPollOnTimeout - Use a timeout and poll for the lock
                    bailing if the process exits.

                PsLockReturnTimeout - Do not poll, just timeout wait and
                    return if you can not get the lock.

                PsLockWaitForever - Wait without a timeout


Return Value:

    STATUS_SUCCESS - You got the lock, you must call PsUnlocProcess later on

    STATUS_TIMEOUT - You requested PsLockReturnTimeout, and the lock was not available

    STATUS_PROCESS_IS_TERMINATING - The process you are trying to lock is terminating

--*/

{

    LARGE_INTEGER DueTime;
    NTSTATUS Status;
    PLARGE_INTEGER Timeout;
    PETHREAD Thread;
    PSLOCKPROCESSMODE LocalLockMode;
    BOOLEAN WaitSuccess;

    PAGED_CODE();

    LocalLockMode = LockMode;
    if ( LockMode == PsLockIAmExiting ) {
        LocalLockMode = PsLockWaitForever;
        }

    Thread = PsGetCurrentThread();

retry:
    //
    // Acquire process lock fast mutex to synchronize access to the ownership,
    // lock count, and synchronization event of the specified process.
    //

    KeEnterCriticalRegion();

    ExAcquireFastMutexUnsafe(&PspProcessLockMutex);

    //
    // Check if the process lock can be acquired.
    //

    if (Process->LockCount != 1) {

        //
        // The process lock is currently owned.
        //
        // If the lock mode is return timeout, then release the process lock
        // fast mutex and return timeout. Otherwise, set the timout value,
        // decrement the lock count, release the process lock fast mutex, and
        // wait for the process event.
        //

        if (LocalLockMode == PsLockReturnTimeout) {
            ExReleaseFastMutexUnsafe(&PspProcessLockMutex);
            KeLeaveCriticalRegion();
            return STATUS_TIMEOUT;

        } else {

            //
            // If the lock mode is not wait forever, then set the timeout
            // value to one second. Otherwise set the timeout to forever.
            //

            if (LocalLockMode != PsLockWaitForever) {
                DueTime.QuadPart = - 10 * 1000 * 1000;
                Timeout = &DueTime;

            } else {
                Timeout = NULL;
            }

            //
            // Decrement the lock count and loop waiting for the process to
            // terminate or the lock to be granted.
            //

            Process->LockCount -= 1;
            do {

                WaitSuccess = FALSE;
                //
                // If the specified process has exited, then set the
                // completion status and exit the loop.
                //

                if (Process->ExitTime.QuadPart != 0 && LocalLockMode != PsLockWaitForever) {
                    Status = STATUS_PROCESS_IS_TERMINATING;
                    break;
                }

                //
                // Release the process lock fast mutex and wait for the
                // lock to be granted or time out to occur.
                //

                ExReleaseFastMutexUnsafe(&PspProcessLockMutex);
rewait:
                Status = KeWaitForSingleObject(&Process->LockEvent,
                                               Executive,
                                               WaitMode,
                                               FALSE,
                                               Timeout);

                //
                // If waitmode is user-mode, then we can pop out of the wait with
                // status_user_apc. If callers are using PsLockWaitForever, they
                // don't expect this API to return without holding the lock, so
                // we need to re-execute the wait. Only place waitforever is used
                // in conjunction with UserMode is exit where all we want to do
                // with user-mode is allow our stack to swap, not service an exitapc
                //
                if ( Status == STATUS_USER_APC
                     && WaitMode == UserMode
                     && LocalLockMode == PsLockWaitForever ) {
                    WaitMode = KernelMode;
                    goto rewait;
                    }

                //
                // Reacquire the process lock fast mutex and continue the
                // loop if timeout has occured.
                //
                //

                ExAcquireFastMutexUnsafe(&PspProcessLockMutex);

                //
                // If the specified process has exited, then set the
                // completion status and exit the loop. This test needs
                // to be repeated so we catch the non-timed out wait
                // case where the process terminated and the lock was released
                // during the wait period
                //

                if ( Process->ExitTime.QuadPart != 0
                     && LocalLockMode != PsLockWaitForever ) {

                    if ( Status == STATUS_SUCCESS ) {
                        //
                        // We came out of the wait due to someone setting the
                        // event. Since we are now going to bail,
                        // we need to set the event to pass ownership on to someone else.
                        //
                        WaitSuccess = TRUE;
                        }
                    Status = STATUS_PROCESS_IS_TERMINATING;
                    break;
                }

            } while (Status == STATUS_TIMEOUT);

            //
            // If the completion status is success, then the lock has been
            // granted and the owner is set. Otherwise, a user APC is pending
            // or the process has terminated and the lock request should be
            // dropped.
            //

            if (Status != STATUS_SUCCESS) {
                Status = STATUS_PROCESS_IS_TERMINATING;
                Process->LockCount += 1;

                if ( Process->LockCount == 1 ) {
                    //
                    // No one else is involved with the lock. We were granted owner
                    // ship. We need to make sure the event is NOT signalled
                    //
                    // Is doesn't matter if we bail in this case due to a successful
                    // wait, timeout. A lock count of 1 is the initial state
                    // of a free lock.

                    KeClearEvent(&Process->LockEvent);
                } else {

                    //
                    // A LockCount !=1 means that others are involved
                    // in the lock. We requested ownership of the lock
                    // by decrementing lockcount.
                    //
                    // We were granted ownership IF our wait was satisfied.
                    // Since we are bailing, we need to pass this ownership along
                    // to the next waiter.
                    //
                    // If we were not granted ownership (wait timed out, or we
                    // bailed before the wait) then we don't have to do anything
                    // more than increment the lock count
                    //

                    if ( WaitSuccess ) {
                        KeSetEvent(&Process->LockEvent, 0, FALSE);
                    }
                }

            } else {
                Process->LockOwner = KeGetCurrentThread();
            }
        }

    } else {

        //
        // The process lock is not currently owned.
        //
        // If the lock mode is not wait forever and the process has already
        // terminated, then set the completion status to process terminating.
        // Otherwise, decrement the lock count, set the lock owner, and set
        // the completion status to success.
        //

        if ((LocalLockMode != PsLockWaitForever) &&
            ( (Process->ExitTime.QuadPart != 0 || KeReadStateProcess(&Process->Pcb) != FALSE) )
           ) {
            Status = STATUS_PROCESS_IS_TERMINATING;

        } else {
            Process->LockCount -= 1;
            Process->LockOwner = KeGetCurrentThread();
            Status = STATUS_SUCCESS;
        }
    }

    //
    // Release the process lock fast mutex and return the completion
    // status.
    //

    ExReleaseFastMutexUnsafe(&PspProcessLockMutex);

    if (Status != STATUS_SUCCESS) {
        KeLeaveCriticalRegion();
    } else {

        //
        // We don't want to be "frozen" with the process lock held, so now
        // that we own the lock, check to see if we have a pending freeze count,
        // and if we do and dropping the lock will help, then drop it
        //

        if ((LockMode != PsLockIAmExiting) &&
            (Thread->Tcb.FreezeCount != 0) &&
            (Thread->Tcb.KernelApcDisable == (ULONG) -1) ) {
            PsUnlockProcess(Process);
            goto retry;
        }
    }

    return Status;
}



VOID
PsUnlockProcess(
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function is the opposite of a successful call to PsLockProcess. It
    simply releases the createdelete lock for a process.

Arguments:

    Process - Supplies the address of the process whose create/delete
        lock is to be released.

Return Value:

    None.

--*/

{

    PAGED_CODE();

    //
    // Acquire process lock fast mutex to synchronize access to the ownership,
    // lock count, and synchronization event of the specified process.
    //

    ExAcquireFastMutexUnsafe(&PspProcessLockMutex);

    //
    // Increment the lock count and clear the lock owner. If the lock count
    // is less than one, then set the lock event.
    //

    Process->LockCount += 1;
    Process->LockOwner = NULL;
    if (Process->LockCount != 1) {
        KeSetEvent(&Process->LockEvent, 0, FALSE);
    }

    //
    // Release the process lock fast mutex and return.
    //


    ExReleaseFastMutexUnsafe(&PspProcessLockMutex);
    KeLeaveCriticalRegion();
    return;
}


HANDLE
PsGetCurrentProcessId( VOID )
{
    return PsGetCurrentThread()->Cid.UniqueProcess;
}

HANDLE
PsGetCurrentThreadId( VOID )
{
    return PsGetCurrentThread()->Cid.UniqueThread;
}

BOOLEAN
PsGetVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    )
{
    if (ARGUMENT_PRESENT(MajorVersion)) {
        *MajorVersion = NtMajorVersion;
    }

    if (ARGUMENT_PRESENT(MinorVersion)) {
        *MinorVersion = NtMinorVersion;
    }

    if (ARGUMENT_PRESENT(BuildNumber)) {
        *BuildNumber = NtBuildNumber & 0x3FFF;
    }

    if (ARGUMENT_PRESENT(CSDVersion)) {
        *CSDVersion = CmCSDVersionString;
    }
    return (NtBuildNumber >> 28) == 0xC;
}

NTSTATUS
PsSetLoadImageNotifyRoutine(
    IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    )

/*++

Routine Description:

    This function allows a device driver to get notified for
    image loads. The notify is issued for both kernel and user
    mode image loads system-wide.

Arguments:

    NotifyRoutine - Supplies the address of a routine which is called at
        image load. The routine is passed information describing the
        image being loaded.

        The callout for creation happens just after the image is loaded
        into memory but before executiona of the image.

    Remove - FALSE specifies to install the callout and TRUE specifies to
        remove the callout that mat

Return Value:

    STATUS_SUCCESS if successful, and STATUS_INVALID_PARAMETER if not.

--*/

{

    ULONG i;
    NTSTATUS Status;

    PAGED_CODE();

    Status = STATUS_INSUFFICIENT_RESOURCES;
    for (i=0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++) {
        if (PspLoadImageNotifyRoutine[i] == NULL) {
            PspLoadImageNotifyRoutine[i] = NotifyRoutine;
            PspLoadImageNotifyRoutineCount += 1;
            Status = STATUS_SUCCESS;
            PsImageNotifyEnabled = TRUE;
            break;
        }
    }

    return Status;
}

VOID
PsCallImageNotifyRoutines(
    IN PUNICODE_STRING FullImageName,
    IN HANDLE ProcessId,                // pid into which image is being mapped
    IN PIMAGE_INFO ImageInfo
    )
/*++

Routine Description:

    This function actually calls the registered image notify functions (on behalf)
    of mapview.c and sysload.c

Arguments:

    FullImageName - The name of the image being loaded

    ProcessId - The process that the image is being loaded into (0 for driver loads)

    ImageInfo - Various flags for the image

Return Value:

    None.

--*/

{
    int i;

    PAGED_CODE();

    if ( PsImageNotifyEnabled ) {
        for (i=0; i<PSP_MAX_LOAD_IMAGE_NOTIFY; i++) {
            if (PspLoadImageNotifyRoutine[i] != NULL) {
               (*PspLoadImageNotifyRoutine[i])(
                    (PUNICODE_STRING) FullImageName,
                    ProcessId,
                    ImageInfo
                    );
                }
            }
        }
}
