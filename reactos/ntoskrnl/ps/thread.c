/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/thread.c
 * PURPOSE:         Thread managment
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
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

/* GLOBALS ******************************************************************/

extern LIST_ENTRY PsActiveProcessHead;
extern PEPROCESS PsIdleProcess;

POBJECT_TYPE EXPORTED PsThreadType = NULL;

extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];
extern ULONG IdleProcessorMask;
extern LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];


BOOLEAN DoneInitYet = FALSE;
static GENERIC_MAPPING PiThreadMapping = {STANDARD_RIGHTS_READ | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
					  STANDARD_RIGHTS_WRITE | THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_ALERT |
                      THREAD_SET_INFORMATION | THREAD_SET_CONTEXT,
                      STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
					  THREAD_ALL_ACCESS};

/* FUNCTIONS ***************************************************************/

#ifdef KeGetCurrentThread
#undef KeGetCurrentThread
#endif
/*
 * @implemented
 */
PKTHREAD 
STDCALL 
KeGetCurrentThread(VOID)
{
#ifdef CONFIG_SMP
   ULONG Flags;
   PKTHREAD Thread;
   Ke386SaveFlags(Flags);
   Ke386DisableInterrupts();
   Thread = KeGetCurrentPrcb()->CurrentThread;
   Ke386RestoreFlags(Flags);
   return Thread;
#else
   return(KeGetCurrentPrcb()->CurrentThread);
#endif
}

/*
 * @implemented
 */
HANDLE STDCALL PsGetCurrentThreadId(VOID)
{
   return(PsGetCurrentThread()->Cid.UniqueThread);
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetThreadFreezeCount(
	PETHREAD Thread
	)
{
	return Thread->Tcb.FreezeCount;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetThreadHardErrorsAreDisabled(
    PETHREAD	Thread
	)
{
	return Thread->HardErrorsAreDisabled;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadId(
    PETHREAD	Thread
	)
{
	return Thread->Cid.UniqueThread;
}

/*
 * @implemented
 */
PEPROCESS
STDCALL
PsGetThreadProcess(
    PETHREAD	Thread
	)
{
	return Thread->ThreadsProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadProcessId(
    PETHREAD	Thread
	)
{
	return Thread->Cid.UniqueProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadSessionId(
    PETHREAD	Thread
	)
{
	return (HANDLE)Thread->ThreadsProcess->SessionId;
}

/*
 * @implemented
 */
PTEB
STDCALL
PsGetThreadTeb(
    PETHREAD	Thread
	)
{
	return Thread->Tcb.Teb;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetThreadWin32Thread(
    PETHREAD	Thread
	)
{
	return Thread->Tcb.Win32Thread;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
PsGetCurrentThreadPreviousMode (
    	VOID
	)
{
	return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackBase (
    	VOID
	)
{
	return PsGetCurrentThread()->Tcb.StackBase;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackLimit (
    	VOID
	)
{
	return (PVOID)PsGetCurrentThread()->Tcb.StackLimit;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
PsIsThreadTerminating(IN PETHREAD Thread)
{
  return (Thread->HasTerminated ? TRUE : FALSE);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsSystemThread(PETHREAD Thread)
{
    return (Thread->SystemThread ? TRUE: FALSE);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsThreadImpersonating(
    PETHREAD	Thread
	)
{
  return Thread->ActiveImpersonationInfo;
}

VOID PsDumpThreads(BOOLEAN IncludeSystem)
{
   PLIST_ENTRY AThread, AProcess;
   PEPROCESS Process;
   PETHREAD Thread;
   ULONG nThreads = 0;
   
   AProcess = PsActiveProcessHead.Flink;
   while(AProcess != &PsActiveProcessHead)
   {
     Process = CONTAINING_RECORD(AProcess, EPROCESS, ProcessListEntry);
     /* FIXME - skip suspended, ... processes? */
     if((Process != PsInitialSystemProcess) ||
        (Process == PsInitialSystemProcess && IncludeSystem))
     {
       AThread = Process->ThreadListHead.Flink;
       while(AThread != &Process->ThreadListHead)
       {
         Thread = CONTAINING_RECORD(AThread, ETHREAD, ThreadListEntry);

         nThreads++;
         DbgPrint("Thread->Tcb.State %d Affinity %08x Priority %d PID.TID %d.%d Name %.8s Stack: \n",
                  Thread->Tcb.State,
		  Thread->Tcb.Affinity,
		  Thread->Tcb.Priority,
                  Thread->ThreadsProcess->UniqueProcessId,
                  Thread->Cid.UniqueThread,
                  Thread->ThreadsProcess->ImageFileName);
         if(Thread->Tcb.State == THREAD_STATE_READY ||
            Thread->Tcb.State == THREAD_STATE_SUSPENDED ||
            Thread->Tcb.State == THREAD_STATE_BLOCKED)
         {
           ULONG i = 0;
           PULONG Esp = (PULONG)Thread->Tcb.KernelStack;
           PULONG Ebp = (PULONG)Esp[4];
           DbgPrint("Ebp 0x%.8X\n", Ebp);
           while(Ebp != 0 && Ebp >= (PULONG)Thread->Tcb.StackLimit)
           {
             DbgPrint("%.8X %.8X%s", Ebp[0], Ebp[1], (i % 8) == 7 ? "\n" : "  ");
             Ebp = (PULONG)Ebp[0];
             i++;
           }
           if((i % 8) != 0)
           {
             DbgPrint("\n");
           }
         }
         AThread = AThread->Flink;
       }
     }
     AProcess = AProcess->Flink;
   }
}

VOID
PsFreezeAllThreads(PEPROCESS Process)
     /*
      * Used by the debugging code to freeze all the process's threads
      * while the debugger is examining their state.
      */
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  PETHREAD current;

  oldIrql = KeAcquireDispatcherDatabaseLock();
  current_entry = Process->ThreadListHead.Flink;
  while (current_entry != &Process->ThreadListHead)
    {
      current = CONTAINING_RECORD(current_entry, ETHREAD,
				  ThreadListEntry);

      /*
       * We have to be careful here, we can't just set the freeze the
       * thread inside kernel mode since it may be holding a lock.
       */

      current_entry = current_entry->Flink;
    }

    KeReleaseDispatcherDatabaseLock(oldIrql);
}

ULONG
PsEnumThreadsByProcess(PEPROCESS Process)
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  ULONG Count = 0;

  oldIrql = KeAcquireDispatcherDatabaseLock();

  current_entry = Process->ThreadListHead.Flink;
  while (current_entry != &Process->ThreadListHead)
    {
      Count++;
      current_entry = current_entry->Flink;
    }
  
  KeReleaseDispatcherDatabaseLock(oldIrql);
  return Count;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
PsRemoveCreateThreadNotifyRoutine (
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;	
}

/*
 * @unimplemented
 */                       
ULONG
STDCALL
PsSetLegoNotifyRoutine(   	
	PVOID LegoNotifyRoutine  	 
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadHardErrorsAreDisabled(
    PETHREAD	Thread,
    BOOLEAN	HardErrorsAreDisabled
	)
{
	Thread->HardErrorsAreDisabled = HardErrorsAreDisabled;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadWin32Thread(
    PETHREAD	Thread,
    PVOID	Win32Thread
	)
{
	Thread->Tcb.Win32Thread = Win32Thread;
}

VOID
PsApplicationProcessorInit(VOID)
{
   KIRQL oldIrql;
   oldIrql = KeAcquireDispatcherDatabaseLock();
   IdleProcessorMask |= (1 << KeGetCurrentProcessorNumber());
   KeReleaseDispatcherDatabaseLock(oldIrql);
}

VOID INIT_FUNCTION
PsPrepareForApplicationProcessorInit(ULONG Id)
{
  PETHREAD IdleThread;
  PKPRCB Prcb = ((PKPCR)((ULONG_PTR)KPCR_BASE + Id * PAGE_SIZE))->Prcb;

  PsInitializeThread(PsIdleProcess,
		     &IdleThread,
		     NULL,
		     KernelMode,
		     FALSE);
  IdleThread->Tcb.State = THREAD_STATE_RUNNING;
  IdleThread->Tcb.FreezeCount = 0;
  IdleThread->Tcb.Affinity = 1 << Id;
  IdleThread->Tcb.UserAffinity = 1 << Id;
  IdleThread->Tcb.Priority = LOW_PRIORITY;
  IdleThread->Tcb.BasePriority = LOW_PRIORITY;
  Prcb->IdleThread = &IdleThread->Tcb;
  Prcb->CurrentThread = &IdleThread->Tcb;

  Ki386InitialStackArray[Id] = (PVOID)IdleThread->Tcb.StackLimit;

  DPRINT("IdleThread for Processor %d has PID %d\n",
	   Id, IdleThread->Cid.UniqueThread);
}

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
        NonPagedPool, sizeof(KAPC), TAG('K', 'a', 'p', 'c'));
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

VOID INIT_FUNCTION
PsInitThreadManagment(VOID)
/*
 * FUNCTION: Initialize thread managment
 */
{
   PETHREAD FirstThread;
   ULONG i;

   for (i=0; i < MAXIMUM_PRIORITY; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }

   PsThreadType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));

   PsThreadType->Tag = TAG('T', 'H', 'R', 'T');
   PsThreadType->TotalObjects = 0;
   PsThreadType->TotalHandles = 0;
   PsThreadType->PeakObjects = 0;
   PsThreadType->PeakHandles = 0;
   PsThreadType->PagedPoolCharge = 0;
   PsThreadType->NonpagedPoolCharge = sizeof(ETHREAD);
   PsThreadType->Mapping = &PiThreadMapping;
   PsThreadType->Dump = NULL;
   PsThreadType->Open = NULL;
   PsThreadType->Close = NULL;
   PsThreadType->Delete = PspDeleteThread;
   PsThreadType->Parse = NULL;
   PsThreadType->Security = NULL;
   PsThreadType->QueryName = NULL;
   PsThreadType->OkayToClose = NULL;
   PsThreadType->Create = NULL;
   PsThreadType->DuplicationNotify = NULL;

   RtlInitUnicodeString(&PsThreadType->TypeName, L"Thread");

   ObpCreateTypeObject(PsThreadType);

   PsInitializeThread(NULL, &FirstThread, NULL, KernelMode, TRUE);
   FirstThread->Tcb.State = THREAD_STATE_RUNNING;
   FirstThread->Tcb.FreezeCount = 0;
   FirstThread->Tcb.UserAffinity = (1 << 0);   /* Set the affinity of the first thread to the boot processor */
   FirstThread->Tcb.Affinity = (1 << 0);
   KeGetCurrentPrcb()->CurrentThread = (PVOID)FirstThread;

   DPRINT("FirstThread %x\n",FirstThread);

   DoneInitYet = TRUE;
   
   ExInitializeWorkItem(&PspReaperWorkItem, PspReapRoutine, NULL);
}

/**********************************************************************
 *	NtOpenThread/4
 *
 *	@implemented
 */
NTSTATUS STDCALL
NtOpenThread(OUT PHANDLE ThreadHandle,
	     IN	ACCESS_MASK DesiredAccess,
	     IN	POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	     IN	PCLIENT_ID ClientId  OPTIONAL)
{
   KPROCESSOR_MODE PreviousMode;
   CLIENT_ID SafeClientId;
   HANDLE hThread;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();

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
         ProbeForRead(ClientId,
                      sizeof(CLIENT_ID),
                      sizeof(ULONG));
         SafeClientId = *ClientId;
         ClientId = &SafeClientId;
       }
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

   if(!((ObjectAttributes == NULL) ^ (ClientId == NULL)))
   {
     DPRINT("NtOpenThread should be called with either ObjectAttributes or ClientId!\n");
     return STATUS_INVALID_PARAMETER;
   }

   if(ClientId != NULL)
   {
     PETHREAD Thread;

     Status = PsLookupThreadByThreadId(ClientId->UniqueThread,
                                       &Thread);
     if(NT_SUCCESS(Status))
     {
       Status = ObInsertObject(Thread,
                               NULL,
                               DesiredAccess,
                               0,
                               NULL,
                               &hThread);

       ObDereferenceObject(Thread);
     }
   }
   else
   {
     Status = ObOpenObjectByName(ObjectAttributes,
                                 PsThreadType,
                                 NULL,
                                 PreviousMode,
                                 DesiredAccess,
                                 NULL,
                                 &hThread);
   }

   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
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

NTSTATUS STDCALL
NtYieldExecution(VOID)
{
  KiDispatchThread(THREAD_STATE_READY);
  return(STATUS_SUCCESS);
}


/*
 * NOT EXPORTED
 */
NTSTATUS STDCALL
NtTestAlert(VOID)
{
  /* Check and Alert Thread if needed */
  return KeTestAlertThread(ExGetPreviousMode()) ? STATUS_ALERTED : STATUS_SUCCESS;
}

VOID
KeSetPreviousMode (ULONG Mode)
{
  PsGetCurrentThread()->Tcb.PreviousMode = (UCHAR)Mode;
}


/*
 * @implemented
 */
KPROCESSOR_MODE STDCALL
KeGetPreviousMode (VOID)
{
  return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}


/*
 * @implemented
 */
KPROCESSOR_MODE STDCALL
ExGetPreviousMode (VOID)
{
  return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/* EOF */
