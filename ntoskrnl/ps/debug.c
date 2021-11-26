/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/debug.c
 * PURPOSE:         Process Manager: Debugging Support (Set/Get Context)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

#if DBG
VOID
NTAPI
PspDumpThreads(BOOLEAN IncludeSystem)
{
    PLIST_ENTRY CurrentThread, CurrentProcess;
    PEPROCESS Process;
    PETHREAD Thread;
    ULONG nThreads = 0;

    /* Loop all Active Processes */
    CurrentProcess = PsActiveProcessHead.Flink;
    while(CurrentProcess != &PsActiveProcessHead)
    {
        /* Get the process */
        Process = CONTAINING_RECORD(CurrentProcess, EPROCESS, ActiveProcessLinks);

        /* Skip the Initial Process if requested */
        if((Process != PsInitialSystemProcess) ||
           (Process == PsInitialSystemProcess && IncludeSystem))
        {
            /* Loop all its threads */
            CurrentThread = Process->ThreadListHead.Flink;
            while(CurrentThread != &Process->ThreadListHead)
            {

                /* Get teh Thread */
                Thread = CONTAINING_RECORD(CurrentThread, ETHREAD, ThreadListEntry);
                nThreads++;

                /* Print the Info */
                DbgPrint("State %u Affinity %08x Priority %d PID.TID %d.%d Name %.8s Stack:\n",
                         Thread->Tcb.State,
                         Thread->Tcb.Affinity,
                         Thread->Tcb.Priority,
                         Thread->Cid.UniqueProcess,
                         Thread->Cid.UniqueThread,
                         Thread->ThreadsProcess->ImageFileName);

                /* Make sure it's not running */
                if(Thread->Tcb.State == Ready ||
                   Thread->Tcb.State == Standby ||
                   Thread->Tcb.State == Waiting)
                {
#ifdef _M_IX86
                    ULONG i = 0;
                    PULONG Esp = (PULONG)Thread->Tcb.KernelStack;
                    PULONG Ebp = (PULONG)Esp[4];

                    /* Print EBP */
                    DbgPrint("Ebp %p\n", Ebp);

                    /* Walk it */
                    while(Ebp != 0 && Ebp >= (PULONG)Thread->Tcb.StackLimit)
                    {
                        ULONG EbpContent[2];
                        ULONG MemoryCopied;
                        NTSTATUS Status;

                        /* Get stack frame content */
                        Status = KdpCopyMemoryChunks((ULONG64)(ULONG_PTR)Ebp,
                                                     EbpContent,
                                                     sizeof(EbpContent),
                                                     sizeof(EbpContent),
                                                     MMDBG_COPY_UNSAFE,
                                                     &MemoryCopied);
                        if (!NT_SUCCESS(Status) || (MemoryCopied < sizeof(EbpContent)))
                        {
                            break;
                        }

                        DbgPrint("%.8X %.8X%s", EbpContent[0], EbpContent[1], (i % 8) == 7 ? "\n" : "  ");
                        Ebp = (PULONG)EbpContent[0];
                        i++;
                    }

                    /* Print a new line if there's nothing */
                    if((i % 8) != 0) DbgPrint("\n");
#else
                    DbgPrint("FIXME: Backtrace skipped on non-x86\n");
#endif
                }

                /* Move to the next Thread */
                CurrentThread = CurrentThread->Flink;
            }
        }

        /* Move to the next Process */
        CurrentProcess = CurrentProcess->Flink;
    }
}
#endif

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsGetContextThread(IN PETHREAD Thread,
                   IN OUT PCONTEXT ThreadContext,
                   IN KPROCESSOR_MODE PreviousMode)
{
    GET_SET_CTX_CONTEXT GetSetContext;
    ULONG Size = 0, Flags = 0;
    NTSTATUS Status;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Set default length */
        Size = sizeof(CONTEXT);

        /* Read the flags */
        Flags = ProbeForReadUlong(&ThreadContext->ContextFlags);

#ifdef _M_IX86
        /* Check if the caller wanted extended registers */
        if ((Flags & CONTEXT_EXTENDED_REGISTERS) !=
            CONTEXT_EXTENDED_REGISTERS)
        {
            /* Cut them out of the size */
            Size = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        }
#endif

        /* Check if we came from user mode */
        if (PreviousMode != KernelMode)
        {
            /* Probe the context */
            ProbeForWrite(ThreadContext, Size, sizeof(ULONG));
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Initialize the wait event */
    KeInitializeEvent(&GetSetContext.Event, NotificationEvent, FALSE);

    /* Set the flags and previous mode */
    RtlZeroMemory(&GetSetContext.Context, Size);
    GetSetContext.Context.ContextFlags = Flags;
    GetSetContext.Mode = PreviousMode;

    /* Check if we're running in the same thread */
    if (Thread == PsGetCurrentThread())
    {
        /* Setup APC parameters manually */
        GetSetContext.Apc.SystemArgument1 = NULL;
        GetSetContext.Apc.SystemArgument2 = Thread;

        /* Enter a guarded region to simulate APC_LEVEL */
        KeEnterGuardedRegion();

        /* Manually call the APC */
        PspGetOrSetContextKernelRoutine(&GetSetContext.Apc,
                                        NULL,
                                        NULL,
                                        &GetSetContext.Apc.SystemArgument1,
                                        &GetSetContext.Apc.SystemArgument2);

        /* Leave the guarded region */
        KeLeaveGuardedRegion();

        /* We are done */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Initialize the APC */
        KeInitializeApc(&GetSetContext.Apc,
                        &Thread->Tcb,
                        OriginalApcEnvironment,
                        PspGetOrSetContextKernelRoutine,
                        NULL,
                        NULL,
                        KernelMode,
                        NULL);

        /* Queue it as a Get APC */
        if (!KeInsertQueueApc(&GetSetContext.Apc, NULL, Thread, 2))
        {
            /* It was already queued, so fail */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Wait for the APC to complete */
            Status = KeWaitForSingleObject(&GetSetContext.Event,
                                           0,
                                           KernelMode,
                                           FALSE,
                                           NULL);
        }
    }

    _SEH2_TRY
    {
        /* Copy the context */
        RtlCopyMemory(ThreadContext, &GetSetContext.Context, Size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsSetContextThread(IN PETHREAD Thread,
                   IN OUT PCONTEXT ThreadContext,
                   IN KPROCESSOR_MODE PreviousMode)
{
    GET_SET_CTX_CONTEXT GetSetContext;
    ULONG Size = 0, Flags = 0;
    NTSTATUS Status;

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Set default length */
        Size = sizeof(CONTEXT);

        /* Read the flags */
        Flags = ProbeForReadUlong(&ThreadContext->ContextFlags);

#ifdef _M_IX86
        /* Check if the caller wanted extended registers */
        if ((Flags & CONTEXT_EXTENDED_REGISTERS) !=
            CONTEXT_EXTENDED_REGISTERS)
        {
            /* Cut them out of the size */
            Size = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        }
#endif

        /* Check if we came from user mode */
        if (PreviousMode != KernelMode)
        {
            /* Probe the context */
            ProbeForRead(ThreadContext, Size, sizeof(ULONG));
        }

        /* Copy the context */
        RtlCopyMemory(&GetSetContext.Context, ThreadContext, Size);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Initialize the wait event */
    KeInitializeEvent(&GetSetContext.Event, NotificationEvent, FALSE);

    /* Set the flags and previous mode */
    GetSetContext.Context.ContextFlags = Flags;
    GetSetContext.Mode = PreviousMode;

    /* Check if we're running in the same thread */
    if (Thread == PsGetCurrentThread())
    {
        /* Setup APC parameters manually */
        GetSetContext.Apc.SystemArgument1 = UlongToPtr(1);
        GetSetContext.Apc.SystemArgument2 = Thread;

        /* Enter a guarded region to simulate APC_LEVEL */
        KeEnterGuardedRegion();

        /* Manually call the APC */
        PspGetOrSetContextKernelRoutine(&GetSetContext.Apc,
                                        NULL,
                                        NULL,
                                        &GetSetContext.Apc.SystemArgument1,
                                        &GetSetContext.Apc.SystemArgument2);

        /* Leave the guarded region */
        KeLeaveGuardedRegion();

        /* We are done */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Initialize the APC */
        KeInitializeApc(&GetSetContext.Apc,
                        &Thread->Tcb,
                        OriginalApcEnvironment,
                        PspGetOrSetContextKernelRoutine,
                        NULL,
                        NULL,
                        KernelMode,
                        NULL);

        /* Queue it as a Get APC */
        if (!KeInsertQueueApc(&GetSetContext.Apc, UlongToPtr(1), Thread, 2))
        {
            /* It was already queued, so fail */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Wait for the APC to complete */
            Status = KeWaitForSingleObject(&GetSetContext.Event,
                                           0,
                                           KernelMode,
                                           FALSE,
                                           NULL);
        }
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtGetContextThread(IN HANDLE ThreadHandle,
                   IN OUT PCONTEXT ThreadContext)
{
    PETHREAD Thread;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_GET_CONTEXT,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);

    if (!NT_SUCCESS(Status)) return Status;

    /* Make sure it's not a system thread */
    if (Thread->SystemThread)
    {
        /* Fail */
        Status = STATUS_INVALID_HANDLE;
    }
    else
    {
        /* Call the kernel API */
        Status = PsGetContextThread(Thread, ThreadContext, PreviousMode);
    }

    /* Dereference it and return */
    ObDereferenceObject(Thread);
    return Status;
}

NTSTATUS
NTAPI
NtSetContextThread(IN HANDLE ThreadHandle,
                   IN PCONTEXT ThreadContext)
{
    PETHREAD Thread;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PAGED_CODE();

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_CONTEXT,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);

    if (!NT_SUCCESS(Status)) return Status;

    /* Make sure it's not a system thread */
    if (Thread->SystemThread)
    {
        /* Fail */
        Status = STATUS_INVALID_HANDLE;
    }
    else
    {
        /* Call the kernel API */
        Status = PsSetContextThread(Thread, ThreadContext, PreviousMode);
    }

    /* Dereference it and return */
    ObDereferenceObject(Thread);
    return Status;
}

/* EOF */
