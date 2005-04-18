/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/create.c
 * PURPOSE:         Thread managment
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
 *                  Skywing
 */

/*
 * NOTE:
 *
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 *
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBAL *******************************************************************/

#define MAX_THREAD_NOTIFY_ROUTINE_COUNT    8
#define TAG_KAPC TAG('k','p','a','p') /* kpap - kernel ps apc */

static ULONG PiThreadNotifyRoutineCount = 0;
static PCREATE_THREAD_NOTIFY_ROUTINE
PiThreadNotifyRoutine[MAX_THREAD_NOTIFY_ROUTINE_COUNT];

ULONG
STDCALL
KeSuspendThread(PKTHREAD Thread);
/* FUNCTIONS ***************************************************************/

VOID
PiBeforeBeginThread(CONTEXT c)
{
    KeLowerIrql(PASSIVE_LEVEL);
}

NTSTATUS
PsInitializeThread (
    PEPROCESS Process,
    PETHREAD* ThreadPtr,
    POBJECT_ATTRIBUTES ObjectAttributes,
    KPROCESSOR_MODE AccessMode,
    BOOLEAN First )
{
    PETHREAD Thread;
    NTSTATUS Status;
    KIRQL oldIrql;

    PAGED_CODE();

    if (Process == NULL)
    {
        Process = PsInitialSystemProcess;
    }

    /*
    * Create and initialize thread
    */
    Status = ObCreateObject(AccessMode,
        PsThreadType,
        ObjectAttributes,
        KernelMode,
        NULL,
        sizeof(ETHREAD),
        0,
        0,
        (PVOID*)&Thread);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    /*
    * Reference process
    */
    ObReferenceObjectByPointer(Process,
        PROCESS_CREATE_THREAD,
        PsProcessType,
        KernelMode);

    Thread->ThreadsProcess = Process;
    Thread->Cid.UniqueThread = NULL;
    Thread->Cid.UniqueProcess = (HANDLE)Thread->ThreadsProcess->UniqueProcessId;

    DPRINT("Thread = %x\n",Thread);

    KeInitializeThread(&Process->Pcb, &Thread->Tcb, First);
    InitializeListHead(&Thread->ActiveTimerListHead);
    KeInitializeSpinLock(&Thread->ActiveTimerListLock);
    InitializeListHead(&Thread->IrpList);
    Thread->DeadThread = FALSE;
    Thread->HasTerminated = FALSE;
    Thread->Tcb.Win32Thread = NULL;
    DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);


    Thread->Tcb.BasePriority = (CHAR)Process->Pcb.BasePriority;
    Thread->Tcb.Priority = Thread->Tcb.BasePriority;

    /*
    * Local Procedure Call facility (LPC)
    */
    KeInitializeSemaphore  (& Thread->LpcReplySemaphore, 0, LONG_MAX);
    Thread->LpcReplyMessage = NULL;
    Thread->LpcReplyMessageId = 0; /* not valid */
    /* Thread->LpcReceiveMessageId = 0; */
    Thread->LpcExitThreadCalled = FALSE;
    Thread->LpcReceivedMsgIdValid = FALSE;

    oldIrql = KeAcquireDispatcherDatabaseLock();
    InsertTailList(&Process->ThreadListHead,
        &Thread->ThreadListEntry);
    KeReleaseDispatcherDatabaseLock(oldIrql);

    *ThreadPtr = Thread;

    return STATUS_SUCCESS;
}


static NTSTATUS
PsCreateTeb (
    HANDLE ProcessHandle,
    PTEB *TebPtr,
    PETHREAD Thread,
    PINITIAL_TEB InitialTeb )
{
    PEPROCESS Process;
    NTSTATUS Status;
    ULONG ByteCount;
    ULONG RegionSize;
    ULONG TebSize;
    PVOID TebBase;
    TEB Teb;

    PAGED_CODE();

    TebSize = PAGE_SIZE;

    if (NULL == Thread->ThreadsProcess)
    {
    /* We'll be allocating a 64k block here and only use 4k of it, but this
    path should almost never be taken. Actually, I never saw it was taken,
    so maybe we should just ASSERT(NULL != Thread->ThreadsProcess) and
        move on */
        TebBase = NULL;
        Status = ZwAllocateVirtualMemory(ProcessHandle,
            &TebBase,
            0,
            &TebSize,
            MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
            PAGE_READWRITE);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("Failed to allocate virtual memory for TEB\n");
            return Status;
        }
    }
    else
    {
        Process = Thread->ThreadsProcess;
        PsLockProcess(Process, FALSE);
        if (NULL == Process->TebBlock ||
            Process->TebBlock == Process->TebLastAllocated)
        {
            Process->TebBlock = NULL;
            RegionSize = MM_VIRTMEM_GRANULARITY;
            Status = ZwAllocateVirtualMemory(ProcessHandle,
                &Process->TebBlock,
                0,
                &RegionSize,
                MEM_RESERVE | MEM_TOP_DOWN,
                PAGE_READWRITE);
            if (! NT_SUCCESS(Status))
            {
                PsUnlockProcess(Process);
                DPRINT1("Failed to reserve virtual memory for TEB\n");
                return Status;
            }
            Process->TebLastAllocated = (PVOID) ((char *) Process->TebBlock + RegionSize);
        }
        TebBase = (PVOID) ((char *) Process->TebLastAllocated - PAGE_SIZE);
        Status = ZwAllocateVirtualMemory(ProcessHandle,
            &TebBase,
            0,
            &TebSize,
            MEM_COMMIT,
            PAGE_READWRITE);
        if (! NT_SUCCESS(Status))
        {
            DPRINT1("Failed to commit virtual memory for TEB\n");
            return Status;
        }
        Process->TebLastAllocated = TebBase;
        PsUnlockProcess(Process);
    }

    DPRINT ("TebBase %p TebSize %lu\n", TebBase, TebSize);
    ASSERT(NULL != TebBase && PAGE_SIZE <= TebSize);

    RtlZeroMemory(&Teb, sizeof(TEB));
    /* set all pointers to and from the TEB */
    Teb.Tib.Self = TebBase;
    if (Thread->ThreadsProcess)
    {
        Teb.Peb = Thread->ThreadsProcess->Peb; /* No PEB yet!! */
    }
    DPRINT("Teb.Peb %x\n", Teb.Peb);

    /* store stack information from InitialTeb */
    if(InitialTeb != NULL)
    {
        /* fixed-size stack */
        if(InitialTeb->StackBase && InitialTeb->StackLimit)
        {
            Teb.Tib.StackBase = InitialTeb->StackBase;
            Teb.Tib.StackLimit = InitialTeb->StackLimit;
            Teb.DeallocationStack = InitialTeb->StackLimit;
        }
        /* expandable stack */
        else
        {
            Teb.Tib.StackBase = InitialTeb->StackCommit;
            Teb.Tib.StackLimit = InitialTeb->StackCommitMax;
            Teb.DeallocationStack = InitialTeb->StackReserved;
        }
    }

    /* more initialization */
    Teb.Cid.UniqueThread = Thread->Cid.UniqueThread;
    Teb.Cid.UniqueProcess = Thread->Cid.UniqueProcess;
    Teb.CurrentLocale = PsDefaultThreadLocaleId;

    /* Terminate the exception handler list */
    Teb.Tib.ExceptionList = (PVOID)-1;

    DPRINT("sizeof(TEB) %x\n", sizeof(TEB));

    /* write TEB data into teb page */
    Status = NtWriteVirtualMemory(ProcessHandle,
        TebBase,
        &Teb,
        sizeof(TEB),
        &ByteCount);

    if (!NT_SUCCESS(Status))
    {
        /* free TEB */
        DPRINT1 ("Writing TEB failed!\n");

        RegionSize = 0;
        NtFreeVirtualMemory(ProcessHandle,
            TebBase,
            &RegionSize,
            MEM_RELEASE);

        return Status;
    }

    if (TebPtr != NULL)
    {
        *TebPtr = (PTEB)TebBase;
    }

    DPRINT("TEB allocated at %p\n", TebBase);

    return Status;
}


VOID STDCALL
LdrInitApcRundownRoutine ( PKAPC Apc )
{
    ExFreePool(Apc);
}


VOID STDCALL
LdrInitApcKernelRoutine (
    PKAPC Apc,
    PKNORMAL_ROUTINE* NormalRoutine,
    PVOID* NormalContext,
    PVOID* SystemArgument1,
    PVOID* SystemArgument2)
{
    ExFreePool(Apc);
}


NTSTATUS STDCALL
NtCreateThread (
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB InitialTeb,
    IN BOOLEAN CreateSuspended )
{
    HANDLE hThread;
    CONTEXT SafeContext;
    INITIAL_TEB SafeInitialTeb;
    PEPROCESS Process;
    PETHREAD Thread;
    PTEB TebBase;
    PKAPC LdrInitApc;
    KIRQL oldIrql;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if(ThreadContext == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PreviousMode = ExGetPreviousMode();

    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(ThreadHandle,
                sizeof(HANDLE),
                sizeof(ULONG));
            if(ClientId != NULL)
            {
                ProbeForWrite(ClientId,
                    sizeof(CLIENT_ID),
                    sizeof(ULONG));
            }
            ProbeForRead(ThreadContext,
                sizeof(CONTEXT),
                sizeof(ULONG));
            SafeContext = *ThreadContext;
            ThreadContext = &SafeContext;
            ProbeForRead(InitialTeb,
                sizeof(INITIAL_TEB),
                sizeof(ULONG));
            SafeInitialTeb = *InitialTeb;
            InitialTeb = &SafeInitialTeb;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    DPRINT("NtCreateThread(ThreadHandle %x, PCONTEXT %x)\n",
        ThreadHandle,ThreadContext);

    Status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_CREATE_THREAD,
        PsProcessType,
        PreviousMode,
        (PVOID*)&Process,
        NULL);
    if(!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Status = PsLockProcess(Process, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Process);
        return(Status);
    }

    if(Process->ExitTime.QuadPart != 0)
    {
        PsUnlockProcess(Process);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    PsUnlockProcess(Process);

    Status = PsInitializeThread(Process,
        &Thread,
        ObjectAttributes,
        PreviousMode,
        FALSE);

    ObDereferenceObject(Process);

    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    /* create a client id handle */
    Status = PsCreateCidHandle (
        Thread, PsThreadType, &Thread->Cid.UniqueThread);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Thread);
        return Status;
    }

    Status = KiArchInitThreadWithContext(&Thread->Tcb, ThreadContext);
    if (!NT_SUCCESS(Status))
    {
        PsDeleteCidHandle(Thread->Cid.UniqueThread, PsThreadType);
        ObDereferenceObject(Thread);
        return(Status);
    }

    Status = PsCreateTeb(ProcessHandle,
        &TebBase,
        Thread,
        InitialTeb);
    if (!NT_SUCCESS(Status))
    {
        PsDeleteCidHandle(Thread->Cid.UniqueThread, PsThreadType);
        ObDereferenceObject(Thread);
        return(Status);
    }
    Thread->Tcb.Teb = TebBase;

    Thread->StartAddress = NULL;

    /*
    * Maybe send a message to the process's debugger
    */
    DbgkCreateThread((PVOID)ThreadContext->Eip);

    /*
    * First, force the thread to be non-alertable for user-mode alerts.
    */
    Thread->Tcb.Alertable = FALSE;

    /*
    * If the thread is to be created suspended then queue an APC to
    * do the suspend before we run any userspace code.
    */
    if (CreateSuspended)
    {
        KeSuspendThread(&Thread->Tcb);
    }

    /*
    * Queue an APC to the thread that will execute the ntdll startup
    * routine.
    */
    LdrInitApc = ExAllocatePoolWithTag (
        NonPagedPool, sizeof(KAPC), TAG_KAPC );
    KeInitializeApc (
        LdrInitApc,
        &Thread->Tcb,
        OriginalApcEnvironment,
        LdrInitApcKernelRoutine,
        LdrInitApcRundownRoutine,
        LdrpGetSystemDllEntryPoint(),
        UserMode,
        NULL );
    KeInsertQueueApc(LdrInitApc, NULL, NULL, IO_NO_INCREMENT);
    /*
    * The thread is non-alertable, so the APC we added did not set UserApcPending to TRUE.
    * We must do this manually. Do NOT attempt to set the Thread to Alertable before the call,
    * doing so is a blatant and erronous hack.
    */
    Thread->Tcb.ApcState.UserApcPending = TRUE;
    Thread->Tcb.Alerted[KernelMode] = TRUE;

    oldIrql = KeAcquireDispatcherDatabaseLock ();
    KiUnblockThread(&Thread->Tcb, NULL, 0);
    KeReleaseDispatcherDatabaseLock(oldIrql);

    Status = ObInsertObject((PVOID)Thread,
        NULL,
        DesiredAccess,
        0,
        NULL,
        &hThread);
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            if(ClientId != NULL)
            {
                *ClientId = Thread->Cid;
            }
            *ThreadHandle = hThread;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsCreateSystemThread (
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    HANDLE ProcessHandle,
    PCLIENT_ID ClientId,
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext )
/*
 * FUNCTION: Creates a thread which executes in kernel mode
 * ARGUMENTS:
 *       ThreadHandle (OUT) = Caller supplied storage for the returned thread
 *                            handle
 *       DesiredAccess = Requested access to the thread
 *       ObjectAttributes = Object attributes (optional)
 *       ProcessHandle = Handle of process thread will run in
 *                       NULL to use system process
 *       ClientId (OUT) = Caller supplied storage for the returned client id
 *                        of the thread (optional)
 *       StartRoutine = Entry point for the thread
 *       StartContext = Argument supplied to the thread when it begins
 *                     execution
 * RETURNS: Success or failure status
 */
{
    PETHREAD Thread;
    NTSTATUS Status;
    KIRQL oldIrql;

    PAGED_CODE();

    DPRINT("PsCreateSystemThread(ThreadHandle %x, ProcessHandle %x)\n",
        ThreadHandle,ProcessHandle);

    Status = PsInitializeThread(
        NULL,
        &Thread,
        ObjectAttributes,
        KernelMode,
        FALSE);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    /* Set the thread as a system thread */
    Thread->SystemThread = TRUE;

    Status = PsCreateCidHandle(Thread,
        PsThreadType,
        &Thread->Cid.UniqueThread);
    if(!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Thread);
        return Status;
    }

    Thread->StartAddress = StartRoutine;
    Status = KiArchInitThread (
        &Thread->Tcb, StartRoutine, StartContext);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Thread);
        return(Status);
    }

    if (ClientId != NULL)
    {
        *ClientId=Thread->Cid;
    }

    oldIrql = KeAcquireDispatcherDatabaseLock ();
    KiUnblockThread(&Thread->Tcb, NULL, 0);
    KeReleaseDispatcherDatabaseLock(oldIrql);

    Status = ObInsertObject(
        (PVOID)Thread,
        NULL,
        DesiredAccess,
        0,
        NULL,
        ThreadHandle);

    /* don't dereference the thread, the initial reference serves as the keep-alive
    reference which will be removed by the thread reaper */

    return Status;
}


VOID STDCALL
PspRunCreateThreadNotifyRoutines (
    PETHREAD CurrentThread,
    BOOLEAN Create )
{
    ULONG i;
    CLIENT_ID Cid = CurrentThread->Cid;

    for (i = 0; i < PiThreadNotifyRoutineCount; i++)
    {
        PiThreadNotifyRoutine[i](Cid.UniqueProcess, Cid.UniqueThread, Create);
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsSetCreateThreadNotifyRoutine (
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine )
{
    if (PiThreadNotifyRoutineCount >= MAX_THREAD_NOTIFY_ROUTINE_COUNT)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    PiThreadNotifyRoutine[PiThreadNotifyRoutineCount] = NotifyRoutine;
    PiThreadNotifyRoutineCount++;

    return(STATUS_SUCCESS);
}

/* EOF */
