/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/thread.c
 * PURPOSE:         Thread managment
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

extern LIST_ENTRY PsActiveProcessHead;
extern PEPROCESS PsIdleProcess;

POBJECT_TYPE EXPORTED PsThreadType = NULL;

PTEB
STDCALL
MmCreateTeb(PEPROCESS Process,
            PCLIENT_ID ClientId,
            PINITIAL_TEB InitialTeb);

/* FUNCTIONS ***************************************************************/

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

/*
 * @implemented
 */
HANDLE 
STDCALL 
PsGetCurrentThreadId(VOID)
{
    return(PsGetCurrentThread()->Cid.UniqueThread);
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetThreadFreezeCount(PETHREAD Thread)
{
    return Thread->Tcb.FreezeCount;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetThreadHardErrorsAreDisabled(PETHREAD Thread)
{
    return Thread->HardErrorsAreDisabled;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadId(PETHREAD Thread)
{
    return Thread->Cid.UniqueThread;
}

/*
 * @implemented
 */
PEPROCESS
STDCALL
PsGetThreadProcess(PETHREAD Thread)
{
    return Thread->ThreadsProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadProcessId(PETHREAD Thread)
{
    return Thread->Cid.UniqueProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadSessionId(PETHREAD Thread)
{
    return (HANDLE)Thread->ThreadsProcess->SessionId;
}

/*
 * @implemented
 */
PTEB
STDCALL
PsGetThreadTeb(PETHREAD Thread)
{
    return Thread->Tcb.Teb;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetThreadWin32Thread(PETHREAD Thread)
{
    return Thread->Tcb.Win32Thread;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
PsGetCurrentThreadPreviousMode(VOID)
{
    return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackBase(VOID)
{
    return PsGetCurrentThread()->Tcb.StackBase;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackLimit(VOID)
{
    return (PVOID)PsGetCurrentThread()->Tcb.StackLimit;
}

/*
 * @implemented
 */
BOOLEAN 
STDCALL
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
PsIsThreadImpersonating(PETHREAD Thread)
{
    return Thread->ActiveImpersonationInfo;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadHardErrorsAreDisabled(PETHREAD Thread,
                                 BOOLEAN HardErrorsAreDisabled)
{
    Thread->HardErrorsAreDisabled = HardErrorsAreDisabled;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadWin32Thread(PETHREAD Thread,
                       PVOID Win32Thread)
{
    Thread->Tcb.Win32Thread = Win32Thread;
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

    Thread->Tcb.Teb = MmCreateTeb(Process, &Thread->Cid, InitialTeb);

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

/*
 * @implemented
 */
KPROCESSOR_MODE 
STDCALL
ExGetPreviousMode (VOID)
{
    return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/* EOF */
