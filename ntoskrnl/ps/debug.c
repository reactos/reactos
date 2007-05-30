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
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/* Thread "Set/Get Context" Context Structure */
typedef struct _GET_SET_CTX_CONTEXT
{
    KAPC Apc;
    KEVENT Event;
    KPROCESSOR_MODE Mode;
    CONTEXT Context;
} GET_SET_CTX_CONTEXT, *PGET_SET_CTX_CONTEXT;

/* PRIVATE FUNCTIONS *********************************************************/

#ifdef DBG
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
                DbgPrint("State %d Affinity %08x Priority %d PID.TID %d.%d Name %.8s Stack: \n",
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
                    ULONG i = 0;
                    PULONG Esp = (PULONG)Thread->Tcb.KernelStack;
                    PULONG Ebp = (PULONG)Esp[4];

                    /* Print EBP */
                    DbgPrint("Ebp 0x%.8X\n", Ebp);

                    /* Walk it */
                    while(Ebp != 0 && Ebp >= (PULONG)Thread->Tcb.StackLimit)
                    {
                        /* Print what's on the stack */
                        DbgPrint("%.8X %.8X%s", Ebp[0], Ebp[1], (i % 8) == 7 ? "\n" : "  ");
                        Ebp = (PULONG)Ebp[0];
                        i++;
                    }

                    /* Print a new line if there's nothing */
                    if((i % 8) != 0) DbgPrint("\n");
                }
            }

            /* Move to the next Thread */
         CurrentThread = CurrentThread->Flink;
        }

        /* Move to the next Process */
        CurrentProcess = CurrentProcess->Flink;
    }
}
#endif

VOID
NTAPI
PspGetOrSetContextKernelRoutine(IN PKAPC Apc,
                                IN OUT PKNORMAL_ROUTINE* NormalRoutine,
                                IN OUT PVOID* NormalContext,
                                IN OUT PVOID* SystemArgument1,
                                IN OUT PVOID* SystemArgument2)
{
#if defined(_M_IX86)
    PGET_SET_CTX_CONTEXT GetSetContext;
    PKEVENT Event;
    PCONTEXT Context;
    PKTHREAD Thread;
    KPROCESSOR_MODE Mode;
    PKTRAP_FRAME TrapFrame;
    PAGED_CODE();

    /* Get the Context Structure */
    GetSetContext = CONTAINING_RECORD(Apc, GET_SET_CTX_CONTEXT, Apc);
    Context = &GetSetContext->Context;
    Event = &GetSetContext->Event;
    Mode = GetSetContext->Mode;
    Thread = Apc->SystemArgument2;

    /* Get the trap frame */
    TrapFrame = (PKTRAP_FRAME)((ULONG_PTR)KeGetCurrentThread()->InitialStack -
                               sizeof (FX_SAVE_AREA) - sizeof (KTRAP_FRAME));

    /* Sanity check */
    ASSERT(((TrapFrame->SegCs & MODE_MASK) != KernelMode) ||
        (TrapFrame->EFlags & EFLAGS_V86_MASK));

    /* Check if it's a set or get */
    if (SystemArgument1)
    {
        /* Get the Context */
        KeTrapFrameToContext(TrapFrame, NULL, Context);
    }
    else
    {
        /* Set the Context */
        KeContextToTrapFrame(Context,
                             NULL,
                             TrapFrame,
                             Context->ContextFlags,
                             Mode);
    }

    /* Notify the Native API that we are done */
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
#else
    DPRINT1("PspGetOrSetContextKernelRoutine() not implemented!");
    for (;;);
#endif
}

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
    NTSTATUS Status = STATUS_SUCCESS;

    /* Enter SEH */
    _SEH_TRY
    {
        /* Set default ength */
        Size = sizeof(CONTEXT);

        /* Read the flags */
        Flags = ProbeForReadUlong(&ThreadContext->ContextFlags);

        /* Check if the caller wanted extended registers */
        if ((Flags & CONTEXT_EXTENDED_REGISTERS) !=
            CONTEXT_EXTENDED_REGISTERS)
        {
            /* Cut them out of the size */
            Size = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        }

        /* Check if we came from user mode */
        if (PreviousMode != KernelMode)
        {
            /* Probe the context */
            ProbeForWrite(ThreadContext, Size, sizeof(ULONG));
        }
    }
    _SEH_HANDLE
    {
        /* Get exception code */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Check if we got success */
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the wait event */
    KeInitializeEvent(&GetSetContext.Event, NotificationEvent, FALSE);

    /* Set the flags and previous mode */
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

    _SEH_TRY
    {
        /* Copy the context */
        RtlCopyMemory(ThreadContext, &GetSetContext.Context, Size);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

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
    NTSTATUS Status = STATUS_SUCCESS;

    /* Enter SEH */
    _SEH_TRY
    {
        /* Set default length */
        Size = sizeof(CONTEXT);

        /* Read the flags */
        Flags = ProbeForReadUlong(&ThreadContext->ContextFlags);

        /* Check if the caller wanted extended registers */
        if ((Flags & CONTEXT_EXTENDED_REGISTERS) !=
            CONTEXT_EXTENDED_REGISTERS)
        {
            /* Cut them out of the size */
            Size = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
        }

        /* Check if we came from user mode */
        if (PreviousMode != KernelMode)
        {
            /* Probe the context */
            ProbeForRead(ThreadContext, Size, sizeof(ULONG));
        }

        /* Copy the context */
        RtlCopyMemory(&GetSetContext.Context, ThreadContext, Size);
    }
    _SEH_HANDLE
    {
        /* Get exception code */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Check if we got success */
    if (!NT_SUCCESS(Status)) return Status;

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
