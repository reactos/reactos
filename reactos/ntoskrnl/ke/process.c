/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/process.c
 * PURPOSE:         Kernel Process Management and System Call Tables
 * PROGRAMMERS:     Alex Ionescu
 *                  Gregor Anich
 */

/* INCLUDES ********(*********************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

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

/* FUNCTIONS *****************************************************************/

PKPROCESS
STDCALL
KeGetCurrentProcess(VOID)
{
    return(&(PsGetCurrentProcess()->Pcb));
}

static __inline
VOID
NTAPI
UpdatePageDirs(IN PKTHREAD Thread,
               IN PKPROCESS Process)
{
    /*
     * The stack and the thread structure of the current process may be
     * located in a page which is not present in the page directory of
     * the process we're attaching to. That would lead to a page fault
     * when this function returns. However, since the processor can't
     * call the page fault handler 'cause it can't push EIP on the stack,
     * this will show up as a stack fault which will crash the entire system.
     * To prevent this, make sure the page directory of the process we're
     * attaching to is up-to-date.
     */
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread->StackLimit, KERNEL_STACK_SIZE);
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));
}

VOID
NTAPI
KiAttachProcess(PKTHREAD Thread,
                PKPROCESS Process,
                PKLOCK_QUEUE_HANDLE ApcLock,
                PRKAPC_STATE SavedApcState)
{
    ASSERT(Process != Thread->ApcState.Process);
    DPRINT("KiAttachProcess(Thread: %x, Process: %x, SavedApcState: %x\n",
            Thread, Process, SavedApcState);

    /* Increase Stack Count */
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
        Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->SavedApcState;
        Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->ApcState;
        Thread->ApcStateIndex = AttachedApcEnvironment;
    }

    /* Check if the process is paged in */
    if (Process->State == ProcessInMemory)
    {
        /* FIXME: Scan the Ready Thread List once new scheduler is in */

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
        DbgBreakPoint();
    }
}

VOID
NTAPI
KeInitializeProcess(IN OUT PKPROCESS Process,
                    IN KPRIORITY Priority,
                    IN KAFFINITY Affinity,
                    IN LARGE_INTEGER DirectoryTableBase)
{
    ULONG i = 0;
    UCHAR IdealNode = 0;
    PKNODE Node;

    /* Initialize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Process->Header,
                                 ProcessObject,
                                 sizeof(KPROCESS),
                                 FALSE);

    /* Initialize Scheduler Data, Alignment Faults and Set the PDE */
    Process->Affinity = Affinity;
    Process->BasePriority = (CHAR)Priority;
    Process->QuantumReset = 6;
    Process->DirectoryTableBase = DirectoryTableBase;
    Process->AutoAlignment = TRUE;
    Process->IopmOffset = 0xFFFF;

    /* Initialize the lists */
    InitializeListHead(&Process->ThreadListHead);
    InitializeListHead(&Process->ProfileListHead);
    InitializeListHead(&Process->ReadyListHead);

    /* Initialize the current State */
    Process->State = ProcessInMemory;

    /* Check how many Nodes there are on the system */
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
}

ULONG
NTAPI
KeSetProcess(PKPROCESS Process,
             KPRIORITY Increment,
             BOOLEAN InWait)
{
    KIRQL OldIrql;
    ULONG OldState;

    /* Lock Dispatcher */
    OldIrql = KiAcquireDispatcherLock();

    /* Get Old State */
    OldState = Process->Header.SignalState;

    /* Signal the Process */
    Process->Header.SignalState = TRUE;
    if ((OldState == 0) && IsListEmpty(&Process->Header.WaitListHead) != TRUE)
    {
        /* Satisfy waits */
        KiWaitTest((PVOID)Process, Increment);
    }

    /* Release Dispatcher Database */
    KiReleaseDispatcherLock(OldIrql);

    /* Return the previous State */
    return OldState;
}

VOID
NTAPI
KiSwapProcess(PKPROCESS NewProcess,
              PKPROCESS OldProcess)
{
    DPRINT("Switching CR3 to: %x\n", NewProcess->DirectoryTableBase.u.LowPart);
    Ke386SetPageTableDirectory(NewProcess->DirectoryTableBase.u.LowPart);
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
    KPRIORITY Delta;
    PLIST_ENTRY NextEntry, ListHead;
    KPRIORITY NewPriority, OldPriority;
    KIRQL OldIrql;
    PKTHREAD Thread;
    BOOLEAN Released;
    ASSERT_PROCESS(Process);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check if the process already has this priority */
    if (Process->BasePriority == Priority)
    {
        /* Don't change anything */
        return Process->BasePriority;
    }

    /* If the caller gave priority 0, normalize to 1 */
    if (!Priority) Priority = 1;

    /* Lock Dispatcher */
    OldIrql = KiAcquireDispatcherLock();

    /* Check if we are modifying the quantum too */
    if (Quantum) Process->QuantumReset = Quantum;

    /* Save the current base priority and update it */
    OldPriority = Process->BasePriority;
    Process->BasePriority = Priority;

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
                Thread->BasePriority = NewPriority;
                Thread->Quantum = Thread->QuantumReset;

                /* Disable decrements and update priority */
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority, &Released);
            }

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
                Thread->BasePriority = NewPriority;
                Thread->Quantum = Thread->QuantumReset;

                /* Disable decrements and update priority */
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority, &Released);
            }

            /* Go to the next thread */
            NextEntry = NextEntry->Flink;
        }
    }

    /* Release Dispatcher Database */
    if (!Released) KiReleaseDispatcherLock(OldIrql);

    /* Return previous priority */
    return OldPriority;
}

/*
 * @implemented
 */
VOID
NTAPI
KeAttachProcess(PKPROCESS Process)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    PKTHREAD Thread;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    DPRINT("KeAttachProcess: %x\n", Process);

    /* Make sure that we are in the right page directory */
    Thread = KeGetCurrentThread();
    UpdatePageDirs(Thread, Process);

    /* Check if we're already in that process */
    if (Thread->ApcState.Process == Process) return;

    /* Acquire APC Lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Check if a DPC is executing or if we're already attached */
    if ((Thread->ApcStateIndex != OriginalApcEnvironment) ||
        (KeIsExecutingDpc()))
    {
        /* Invalid attempt */
        KEBUGCHECKEX(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     Thread->ApcStateIndex,
                     KeIsExecutingDpc());
    }
    else
    {
        /* Legit attach attempt: do it! */
        KiAttachProcess(Thread, Process, &ApcLock, &Thread->SavedApcState);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeDetachProcess (VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    DPRINT("KeDetachProcess()\n");

    /* Check if it's attached */
    if (Thread->ApcStateIndex == OriginalApcEnvironment) return;

    /* Acquire APC Lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* It is, decrease Stack Count */
    if(!(--Thread->ApcState.Process->StackCount))
    {
        /* FIXME: Swap the process out */
    }

    /* Restore the APC State */
    KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
    Thread->SavedApcState.Process = NULL;
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStateIndex = OriginalApcEnvironment;

    /* Release lock */
    KiReleaseApcLockFromDpcLevel(&ApcLock);

    /* Swap Processes */
    KiSwapProcess(Thread->ApcState.Process, Thread->ApcState.Process);

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
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Make sure that we are in the right page directory */
    UpdatePageDirs(Thread, Process);

    /* Crash system if DPC is being executed! */
    if (KeIsExecutingDpc())
    {
        /* Executing a DPC, crash! */
        KEBUGCHECKEX(INVALID_PROCESS_ATTACH_ATTEMPT,
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
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check for magic value meaning we were already in the same process */
    if (ApcState->Process == (PKPROCESS)1) return;

    /* Acquire APC Lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /*
     * Check if the process isn't attacked, or has a Kernel APC in progress
     * or has pending APC of any kind.
     */
    if ((Thread->ApcStateIndex == OriginalApcEnvironment) ||
        (Thread->ApcState.KernelApcInProgress) ||
        (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) ||
        (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
    {
        KEBUGCHECK(INVALID_PROCESS_DETACH_ATTEMPT);
    }

    /* Decrease Stack Count */
    if(!(--Thread->ApcState.Process->StackCount))
    {
        /* FIXME: Swap the process out */
    }

    if (ApcState->Process != NULL)
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
    KiSwapProcess(Thread->ApcState.Process, Thread->ApcState.Process);

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
KeAddSystemServiceTable(PULONG_PTR Base,
                        PULONG Count OPTIONAL,
                        ULONG Limit,
                        PUCHAR Number,
                        ULONG Index)
{
    /* Check if descriptor table entry is free */
    if ((Index > SSDT_MAX_ENTRIES - 1) ||
        (KeServiceDescriptorTable[Index].Base) ||
        (KeServiceDescriptorTableShadow[Index].Base))
    {
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
    /* Make sure the Index is valid */
    if (Index > SSDT_MAX_ENTRIES - 1) return FALSE;

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

    return TRUE;
}
/* EOF */
