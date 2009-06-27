/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/process.c
 * PURPOSE:         Kernel Process Management and System Call Tables
 * PROGRAMMERS:     Alex Ionescu
 *                  Gregor Anich
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY KiProcessListHead;
LIST_ENTRY KiProcessInSwapListHead, KiProcessOutSwapListHead;
LIST_ENTRY KiStackInSwapListHead;
KEVENT KiSwapEvent;

KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];

PVOID KeUserApcDispatcher;
PVOID KeUserCallbackDispatcher;
PVOID KeUserExceptionDispatcher;
PVOID KeRaiseUserExceptionDispatcher;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KiAttachProcess(IN PKTHREAD Thread,
                IN PKPROCESS Process,
                IN PKLOCK_QUEUE_HANDLE ApcLock,
                IN PRKAPC_STATE SavedApcState)
{
#if 0
    PLIST_ENTRY ListHead, NextEntry;
    PKTHREAD CurrentThread;
#endif
    ASSERT(Process != Thread->ApcState.Process);

    /* Increase Stack Count */
    ASSERT(Process->StackCount != MAXULONG_PTR);
    Process->StackCount++;

    /* Swap the APC Environment */
    KiMoveApcState(&Thread->ApcState, SavedApcState);

    /* Reinitialize Apc State */
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = Process;
    Thread->ApcState.KernelApcInProgress = FALSE;
    Thread->ApcState.KernelApcPending = FALSE;
    Thread->ApcState.UserApcPending = FALSE;

    /* Update Environment Pointers if needed*/
    if (SavedApcState == &Thread->SavedApcState)
    {
        Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->
                                                          SavedApcState;
        Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->ApcState;
        Thread->ApcStateIndex = AttachedApcEnvironment;
    }

    /* Check if the process is paged in */
    if (Process->State == ProcessInMemory)
    {
        /* Scan the ready list */
#if 0
        ListHead = &Process->ReadyListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the thread */
            CurrentThread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);

            /* Remove it */
            RemoveEntryList(NextEntry);
            CurrentThread->ProcessReadyQueue = FALSE;

            /* Mark it ready */
            KiReadyThread(CurrentThread);

            /* Go to the next one */
            NextEntry = ListHead->Flink;
        }
#endif

        /* Release dispatcher lock */
        KiReleaseDispatcherLockFromDpcLevel();

        /* Release lock */
        KiReleaseApcLockFromDpcLevel(ApcLock);

        /* Swap Processes */
        KiSwapProcess(Process, SavedApcState->Process);

        /* Exit the dispatcher */
        KiExitDispatcher(ApcLock->OldIrql);
    }
    else
    {
        DPRINT1("Errr. ReactOS doesn't support paging out processes yet...\n");
        ASSERT(FALSE);
    }
}

VOID
NTAPI
KeInitializeProcess(IN OUT PKPROCESS Process,
                    IN KPRIORITY Priority,
                    IN KAFFINITY Affinity,
                    IN PULONG DirectoryTableBase,
                    IN BOOLEAN Enable)
{
#ifdef CONFIG_SMP
    ULONG i = 0;
    UCHAR IdealNode = 0;
    PKNODE Node;
#endif

    /* Initialize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Process->Header,
                                 ProcessObject,
                                 sizeof(KPROCESS),
                                 FALSE);

    /* Initialize Scheduler Data, Alignment Faults and Set the PDE */
    Process->Affinity = Affinity;
    Process->BasePriority = (CHAR)Priority;
    Process->QuantumReset = 6;
    Process->DirectoryTableBase[0] = DirectoryTableBase[0];
    Process->DirectoryTableBase[1] = DirectoryTableBase[1];
    Process->AutoAlignment = Enable;
#if defined(_M_IX86)
    Process->IopmOffset = KiComputeIopmOffset(IO_ACCESS_MAP_NONE);
#endif

    /* Initialize the lists */
    InitializeListHead(&Process->ThreadListHead);
    InitializeListHead(&Process->ProfileListHead);
    InitializeListHead(&Process->ReadyListHead);

    /* Initialize the current State */
    Process->State = ProcessInMemory;

    /* Check how many Nodes there are on the system */
#ifdef CONFIG_SMP
    if (KeNumberNodes > 1)
    {
        /* Set the new seed */
        KeProcessNodeSeed = (KeProcessNodeSeed + 1) / KeNumberNodes;
        IdealNode = KeProcessNodeSeed;

        /* Loop every node */
        do
        {
            /* Check if the affinity matches */
            if (KeNodeBlock[IdealNode]->ProcessorMask != Affinity) break;

            /* No match, try next Ideal Node and increase node loop index */
            IdealNode++;
            i++;

            /* Check if the Ideal Node is beyond the total number of nodes */
            if (IdealNode >= KeNumberNodes)
            {
                /* Normalize the Ideal Node */
                IdealNode -= KeNumberNodes;
            }
        } while (i < KeNumberNodes);
    }

    /* Set the ideal node and get the ideal node block */
    Process->IdealNode = IdealNode;
    Node = KeNodeBlock[IdealNode];
    ASSERT(Node->ProcessorMask & Affinity);

    /* Find the matching affinity set to calculate the thread seed */
    Affinity &= Node->ProcessorMask;
    Process->ThreadSeed = KeFindNextRightSetAffinity(Node->Seed,
                                                     (ULONG)Affinity);
    Node->Seed = Process->ThreadSeed;
#endif
}

ULONG
NTAPI
KeSetProcess(IN PKPROCESS Process,
             IN KPRIORITY Increment,
             IN BOOLEAN InWait)
{
    KIRQL OldIrql;
    ULONG OldState;
    ASSERT_PROCESS(Process);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock Dispatcher */
    OldIrql = KiAcquireDispatcherLock();

    /* Get Old State */
    OldState = Process->Header.SignalState;

    /* Signal the Process */
    Process->Header.SignalState = TRUE;

    /* Check if was unsignaled and has waiters */
    if (!(OldState) &&
        !(IsListEmpty(&Process->Header.WaitListHead)))
    {
        /* Unwait the threads */
        KxUnwaitThread(&Process->Header, Increment);
    }

    /* Release Dispatcher Database */
    KiReleaseDispatcherLock(OldIrql);

    /* Return the previous State */
    return OldState;
}

VOID
NTAPI
KeSetQuantumProcess(IN PKPROCESS Process,
                    IN UCHAR Quantum)
{
    KLOCK_QUEUE_HANDLE ProcessLock;
    PLIST_ENTRY NextEntry, ListHead;
    PKTHREAD Thread;
    ASSERT_PROCESS(Process);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLock(Process, &ProcessLock);

    /* Set new quantum */
    Process->QuantumReset = Quantum;

    /* Loop all child threads */
    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the thread */
        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

        /* Set quantum */
        Thread->QuantumReset = Quantum;

        /* Go to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Release lock */
    KiReleaseProcessLock(&ProcessLock);
}

KPRIORITY
NTAPI
KeSetPriorityAndQuantumProcess(IN PKPROCESS Process,
                               IN KPRIORITY Priority,
                               IN UCHAR Quantum OPTIONAL)
{
    KLOCK_QUEUE_HANDLE ProcessLock;
    KPRIORITY Delta;
    PLIST_ENTRY NextEntry, ListHead;
    KPRIORITY NewPriority, OldPriority;
    PKTHREAD Thread;
    ASSERT_PROCESS(Process);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check if the process already has this priority */
    if (Process->BasePriority == Priority) return Process->BasePriority;

    /* If the caller gave priority 0, normalize to 1 */
    if (!Priority) Priority = LOW_PRIORITY + 1;

    /* Lock the process */
    KiAcquireProcessLock(Process, &ProcessLock);

    /* Check if we are modifying the quantum too */
    if (Quantum) Process->QuantumReset = Quantum;

    /* Save the current base priority and update it */
    OldPriority = Process->BasePriority;
    Process->BasePriority = (SCHAR)Priority;

    /* Calculate the priority delta */
    Delta = Priority - OldPriority;

    /* Set the list head and list entry */
    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;

    /* Check if this is a real-time priority */
    if (Priority >= LOW_REALTIME_PRIORITY)
    {
        /* Loop the thread list */
        while (NextEntry != ListHead)
        {
            /* Get the thread */
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            /* Update the quantum if we had one */
            if (Quantum) Thread->QuantumReset = Quantum;

            /* Acquire the thread lock */
            KiAcquireThreadLock(Thread);

            /* Calculate the new priority */
            NewPriority = Thread->BasePriority + Delta;
            if (NewPriority < LOW_REALTIME_PRIORITY)
            {
                /* We're in real-time range, don't let it go below */
                NewPriority = LOW_REALTIME_PRIORITY;
            }
            else if (NewPriority > HIGH_PRIORITY)
            {
                /* We're going beyond the maximum priority, normalize */
                NewPriority = HIGH_PRIORITY;
            }

            /*
             * If priority saturation occured or the old priority was still in
             * the real-time range, don't do anything.
             */
            if (!(Thread->Saturation) || (OldPriority < LOW_REALTIME_PRIORITY))
            {
                /* Check if we had priority saturation */
                if (Thread->Saturation > 0)
                {
                    /* Boost priority to maximum */
                    NewPriority = HIGH_PRIORITY;
                }
                else if (Thread->Saturation < 0)
                {
                    /* If we had negative saturation, set minimum priority */
                    NewPriority = LOW_REALTIME_PRIORITY;
                }

                /* Update priority and quantum */
                Thread->BasePriority = (SCHAR)NewPriority;
                Thread->Quantum = Thread->QuantumReset;

                /* Disable decrements and update priority */
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority);
            }

            /* Release the thread lock */
            KiReleaseThreadLock(Thread);

            /* Go to the next thread */
            NextEntry = NextEntry->Flink;
        }
    }
    else
    {
        /* Loop the thread list */
        while (NextEntry != ListHead)
        {
            /* Get the thread */
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            /* Update the quantum if we had one */
            if (Quantum) Thread->QuantumReset = Quantum;

            /* Lock the thread */
            KiAcquireThreadLock(Thread);

            /* Calculate the new priority */
            NewPriority = Thread->BasePriority + Delta;
            if (NewPriority >= LOW_REALTIME_PRIORITY)
            {
                /* We're not real-time range, don't let it enter RT range */
                NewPriority = LOW_REALTIME_PRIORITY - 1;
            }
            else if (NewPriority <= LOW_PRIORITY)
            {
                /* We're going below the minimum priority, normalize */
                NewPriority = 1;
            }

            /*
             * If priority saturation occured or the old priority was still in
             * the real-time range, don't do anything.
             */
            if (!(Thread->Saturation) ||
                (OldPriority >= LOW_REALTIME_PRIORITY))
            {
                /* Check if we had priority saturation */
                if (Thread->Saturation > 0)
                {
                    /* Boost priority to maximum */
                    NewPriority = LOW_REALTIME_PRIORITY - 1;
                }
                else if (Thread->Saturation < 0)
                {
                    /* If we had negative saturation, set minimum priority */
                    NewPriority = 1;
                }

                /* Update priority and quantum */
                Thread->BasePriority = (SCHAR)NewPriority;
                Thread->Quantum = Thread->QuantumReset;

                /* Disable decrements and update priority */
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority);
            }

            /* Release the thread lock */
            KiReleaseThreadLock(Thread);

            /* Go to the next thread */
            NextEntry = NextEntry->Flink;
        }
    }

    /* Release Dispatcher Database */
    KiReleaseDispatcherLockFromDpcLevel();

    /* Release the process lock */
    KiReleaseProcessLockFromDpcLevel(&ProcessLock);
    KiExitDispatcher(ProcessLock.OldIrql);

    /* Return previous priority */
    return OldPriority;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeAttachProcess(IN PKPROCESS Process)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    PKTHREAD Thread = KeGetCurrentThread();
    ASSERT_PROCESS(Process);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Make sure that we are in the right page directory (ReactOS Mm Hack) */
    MiSyncForProcessAttach(Thread, (PEPROCESS)Process);

    /* Check if we're already in that process */
    if (Thread->ApcState.Process == Process) return;

    /* Check if a DPC is executing or if we're already attached */
    if ((Thread->ApcStateIndex != OriginalApcEnvironment) ||
        (KeIsExecutingDpc()))
    {
        /* Invalid attempt */
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     Thread->ApcStateIndex,
                     KeIsExecutingDpc());
    }
    else
    {
        /* Acquire APC Lock */
        KiAcquireApcLock(Thread, &ApcLock);

        /* Acquire the dispatcher lock */
        KiAcquireDispatcherLockAtDpcLevel();

        /* Legit attach attempt: do it! */
        KiAttachProcess(Thread, Process, &ApcLock, &Thread->SavedApcState);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeDetachProcess(VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();
    KLOCK_QUEUE_HANDLE ApcLock;
    PKPROCESS Process;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check if it's attached */
    if (Thread->ApcStateIndex == OriginalApcEnvironment) return;

    /* Acquire APC Lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Check for invalid attach attempts */
    if ((Thread->ApcState.KernelApcInProgress) ||
        !(IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) ||
        !(IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
    {
        /* Crash the system */
        KeBugCheck(INVALID_PROCESS_DETACH_ATTEMPT);
    }

    /* Get the process */
    Process = Thread->ApcState.Process;

    /* Acquire dispatcher lock */
    KiAcquireDispatcherLockAtDpcLevel();

    /* Decrease the stack count */
    ASSERT(Process->StackCount != 0);
    ASSERT(Process->State == ProcessInMemory);
    Process->StackCount--;

    /* Check if we can swap the process out */
    if (!Process->StackCount)
    {
        /* FIXME: Swap the process out */
    }

    /* Release dispatcher lock */
    KiReleaseDispatcherLockFromDpcLevel();

    /* Restore the APC State */
    KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
    Thread->SavedApcState.Process = NULL;
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStateIndex = OriginalApcEnvironment;

    /* Release lock */
    KiReleaseApcLockFromDpcLevel(&ApcLock);

    /* Swap Processes */
    KiSwapProcess(Thread->ApcState.Process, Process);

    /* Exit the dispatcher */
    KiExitDispatcher(ApcLock.OldIrql);

    /* Check if we have pending APCs */
    if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
    {
        /* What do you know, we do! Request them to be delivered */
        Thread->ApcState.KernelApcPending = TRUE;
        HalRequestSoftwareInterrupt(APC_LEVEL);
    }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeIsAttachedProcess(VOID)
{
    /* Return the APC State */
    return KeGetCurrentThread()->ApcStateIndex;
}

/*
 * @implemented
 */
VOID
NTAPI
KeStackAttachProcess(IN PKPROCESS Process,
                     OUT PRKAPC_STATE ApcState)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    PKTHREAD Thread = KeGetCurrentThread();
    ASSERT_PROCESS(Process);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Make sure that we are in the right page directory (ReactOS Mm Hack) */
    MiSyncForProcessAttach(Thread, (PEPROCESS)Process);

    /* Crash system if DPC is being executed! */
    if (KeIsExecutingDpc())
    {
        /* Executing a DPC, crash! */
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     Thread->ApcStateIndex,
                     KeIsExecutingDpc());
    }

    /* Check if we are already in the target process */
    if (Thread->ApcState.Process == Process)
    {
        /* Set magic value so we don't crash later when detaching */
        ApcState->Process = (PKPROCESS)1;
        return;
    }

    /* Acquire APC Lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Acquire dispatcher lock */
    KiAcquireDispatcherLockAtDpcLevel();

    /* Check if the Current Thread is already attached */
    if (Thread->ApcStateIndex != OriginalApcEnvironment)
    {
        /* We're already attached, so save the APC State into what we got */
        KiAttachProcess(Thread, Process, &ApcLock, ApcState);
    }
    else
    {
        /* We're not attached, so save the APC State into SavedApcState */
        KiAttachProcess(Thread, Process, &ApcLock, &Thread->SavedApcState);
        ApcState->Process = NULL;
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeUnstackDetachProcess(IN PRKAPC_STATE ApcState)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    PKTHREAD Thread = KeGetCurrentThread();
    PKPROCESS Process;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check for magic value meaning we were already in the same process */
    if (ApcState->Process == (PKPROCESS)1) return;

    /* Loop to make sure no APCs are pending  */
    for (;;)
    {
        /* Acquire APC Lock */
        KiAcquireApcLock(Thread, &ApcLock);

        /* Check if a kernel APC is pending */
        if (Thread->ApcState.KernelApcPending)
        {
            /* Check if kernel APC should be delivered */
            if (!(Thread->KernelApcDisable) && (ApcLock.OldIrql <= APC_LEVEL))
            {
                /* Release the APC lock so that the APC can be delivered */
                KiReleaseApcLock(&ApcLock);
                continue;
            }
        }

        /* Otherwise, break out */
        break;
    }

    /*
     * Check if the process isn't attacked, or has a Kernel APC in progress
     * or has pending APC of any kind.
     */
    if ((Thread->ApcStateIndex == OriginalApcEnvironment) ||
        (Thread->ApcState.KernelApcInProgress) ||
        (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) ||
        (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
    {
        /* Bugcheck the system */
        KeBugCheck(INVALID_PROCESS_DETACH_ATTEMPT);
    }

    /* Get the process */
    Process = Thread->ApcState.Process;

    /* Acquire dispatcher lock */
    KiAcquireDispatcherLockAtDpcLevel();

    /* Decrease the stack count */
    ASSERT(Process->StackCount != 0);
    ASSERT(Process->State == ProcessInMemory);
    Process->StackCount--;

    /* Check if we can swap the process out */
    if (!Process->StackCount)
    {
        /* FIXME: Swap the process out */
    }

    /* Release dispatcher lock */
    KiReleaseDispatcherLockFromDpcLevel();

    /* Check if there's an APC state to restore */
    if (ApcState->Process)
    {
        /* Restore the APC State */
        KiMoveApcState(ApcState, &Thread->ApcState);
    }
    else
    {
        /* The ApcState parameter is useless, so use the saved data and reset it */
        KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
        Thread->SavedApcState.Process = NULL;
        Thread->ApcStateIndex = OriginalApcEnvironment;
        Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
        Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    }

    /* Release lock */
    KiReleaseApcLockFromDpcLevel(&ApcLock);

    /* Swap Processes */
    KiSwapProcess(Thread->ApcState.Process, Process);

    /* Exit the dispatcher */
    KiExitDispatcher(ApcLock.OldIrql);

    /* Check if we have pending APCs */
    if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
    {
        /* What do you know, we do! Request them to be delivered */
        Thread->ApcState.KernelApcPending = TRUE;
        HalRequestSoftwareInterrupt(APC_LEVEL);
    }
}

/*
 * @implemented
 */
ULONG
NTAPI
KeQueryRuntimeProcess(IN PKPROCESS Process,
                      OUT PULONG UserTime)
{
    ULONG TotalUser, TotalKernel;
    KLOCK_QUEUE_HANDLE ProcessLock;
    PLIST_ENTRY NextEntry, ListHead;
    PKTHREAD Thread;

    ASSERT_PROCESS(Process);

    /* Initialize user and kernel times */
    TotalUser = Process->UserTime;
    TotalKernel = Process->KernelTime;

    /* Lock the process */
    KiAcquireProcessLock(Process, &ProcessLock);

    /* Loop all child threads and sum up their times */
    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the thread */
        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

        /* Sum up times */
        TotalKernel += Thread->KernelTime;
        TotalUser += Thread->UserTime;

        /* Go to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Release lock */
    KiReleaseProcessLock(&ProcessLock);

    /* Return the user time */
    *UserTime = TotalUser;

    /* Return the kernel time */
    return TotalKernel;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeAddSystemServiceTable(IN PULONG_PTR Base,
                        IN PULONG Count OPTIONAL,
                        IN ULONG Limit,
                        IN PUCHAR Number,
                        IN ULONG Index)
{
    PAGED_CODE();

    /* Check if descriptor table entry is free */
    if ((Index > SSDT_MAX_ENTRIES - 1) ||
        (KeServiceDescriptorTable[Index].Base) ||
        (KeServiceDescriptorTableShadow[Index].Base))
    {
        /* It's not, fail */
        return FALSE;
    }

    /* Initialize the shadow service descriptor table */
    KeServiceDescriptorTableShadow[Index].Base = Base;
    KeServiceDescriptorTableShadow[Index].Limit = Limit;
    KeServiceDescriptorTableShadow[Index].Number = Number;
    KeServiceDescriptorTableShadow[Index].Count = Count;
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRemoveSystemServiceTable(IN ULONG Index)
{
    PAGED_CODE();

    /* Make sure the Index is valid */
    if (Index > (SSDT_MAX_ENTRIES - 1)) return FALSE;

    /* Is there a Normal Descriptor Table? */
    if (!KeServiceDescriptorTable[Index].Base)
    {
        /* Not with the index, is there a shadow at least? */
        if (!KeServiceDescriptorTableShadow[Index].Base) return FALSE;
    }

    /* Now clear from the Shadow Table. */
    KeServiceDescriptorTableShadow[Index].Base = NULL;
    KeServiceDescriptorTableShadow[Index].Number = NULL;
    KeServiceDescriptorTableShadow[Index].Limit = 0;
    KeServiceDescriptorTableShadow[Index].Count = NULL;

    /* Check if we should clean from the Master one too */
    if (Index == 1)
    {
        KeServiceDescriptorTable[Index].Base = NULL;
        KeServiceDescriptorTable[Index].Number = NULL;
        KeServiceDescriptorTable[Index].Limit = 0;
        KeServiceDescriptorTable[Index].Count = NULL;
    }

    /* Return success */
    return TRUE;
}
/* EOF */
