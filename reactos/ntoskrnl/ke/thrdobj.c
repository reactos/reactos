/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/thrdobj.c
 * PURPOSE:         Implements routines to manage the Kernel Thread Object
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];
extern LIST_ENTRY PspReaperListHead;

ULONG KiMask32Array[MAXIMUM_PRIORITY] =
{
    0x1,        0x2,       0x4,       0x8,       0x10,       0x20,
    0x40,       0x80,      0x100,     0x200,     0x4000,     0x800,
    0x1000,     0x2000,    0x4000,    0x8000,    0x10000,    0x20000,
    0x40000,    0x80000,   0x100000,  0x200000,  0x400000,   0x800000,
    0x1000000,  0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000,
    0x40000000, 0x80000000
};

/* FUNCTIONS *****************************************************************/

UCHAR
NTAPI
KeFindNextRightSetAffinity(IN UCHAR Number,
                           IN ULONG Set)
{
    ULONG Bit, Result;
    ASSERT(Set != 0);

    /* Calculate the mask */
    Bit = (AFFINITY_MASK(Number) - 1) & Set;

    /* If it's 0, use the one we got */
    if (!Bit) Bit = Set;

    /* Now find the right set and return it */
    BitScanReverse(&Result, Bit);
    return (UCHAR)Result;
}


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

VOID
STDCALL
KeSetPreviousMode(ULONG Mode)
{
    PsGetCurrentThread()->Tcb.PreviousMode = (UCHAR)Mode;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
KeGetPreviousMode(VOID)
{
    return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}

BOOLEAN
STDCALL
KeDisableThreadApcQueueing(IN PKTHREAD Thread)
{
    KIRQL OldIrql;
    BOOLEAN PreviousState;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save old state */
    PreviousState = Thread->ApcQueueable;

    /* Disable it now */
    Thread->ApcQueueable = FALSE;

    /* Release the Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return old state */
    return PreviousState;
}

VOID
STDCALL
KeRundownThread(VOID)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    PLIST_ENTRY NextEntry, ListHead;
    PKMUTANT Mutant;
    DPRINT("KeRundownThread: %x\n", Thread);

    /* Optimized path if nothing is on the list at the moment */
    if (IsListEmpty(&Thread->MutantListHead)) return;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get the List Pointers */
    ListHead = &Thread->MutantListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the Mutant */
        Mutant = CONTAINING_RECORD(NextEntry, KMUTANT, MutantListEntry);
        DPRINT1("Mutant: %p. Type, Size %x %x\n",
                 Mutant,
                 Mutant->Header.Type,
                 Mutant->Header.Size);

        /* Make sure it's not terminating with APCs off */
        if (Mutant->ApcDisable)
        {
            /* Bugcheck the system */
            KEBUGCHECKEX(0,//THREAD_TERMINATE_HELD_MUTEX,
                         (ULONG_PTR)Thread,
                         (ULONG_PTR)Mutant,
                         0,
                         0);
        }

        /* Now we can remove it */
        RemoveEntryList(&Mutant->MutantListEntry);

        /* Unconditionally abandon it */
        DPRINT("Abandonning the Mutant\n");
        Mutant->Header.SignalState = 1;
        Mutant->Abandoned = TRUE;
        Mutant->OwnerThread = NULL;

        /* Check if the Wait List isn't empty */
        DPRINT("Checking whether to wake the Mutant\n");
        if (!IsListEmpty(&Mutant->Header.WaitListHead))
        {
            /* Wake the Mutant */
            DPRINT("Waking the Mutant\n");
            KiWaitTest(&Mutant->Header, MUTANT_INCREMENT);
        }

        /* Move on */
        NextEntry = NextEntry->Flink;
    }

    /* Release the Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/*
 * Used by the debugging code to freeze all the process's threads
 * while the debugger is examining their state.
 */
VOID
NTAPI
KeFreezeAllThreads(IN PKPROCESS Process)
{
    KLOCK_QUEUE_HANDLE LockHandle, ApcLock;
    PKTHREAD Current, CurrentThread = KeGetCurrentThread();
    PLIST_ENTRY ListHead, NextEntry;
    LONG OldCount;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLock(Process, &LockHandle);

    /* If someone is already trying to free us, try again */
    while (CurrentThread->FreezeCount)
    {
        /* Release and re-acquire the process lock so the APC will go through */
        KiReleaseProcessLock(&LockHandle);
        KiAcquireProcessLock(Process, &LockHandle);
    }

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Loop the Process's Threads */
    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current thread */
        Current = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

        /* Lock it */
        KiAcquireApcLockAtDpcLevel(Current, &ApcLock);

        /* Make sure it's not ours, and check if APCs are enabled */
        if ((Current != CurrentThread) && (Current->ApcQueueable))
        {
            /* Sanity check */
            OldCount = Current->SuspendCount;
            ASSERT(OldCount != MAXIMUM_SUSPEND_COUNT);

            /* Increase the freeze count */
            Current->FreezeCount++;

            /* Make sure it wasn't already suspended */
            if (!(OldCount) && !(Current->SuspendCount))
            {
                /* Did we already insert it? */
                if (!Current->SuspendApc.Inserted)
                {
                    /* Insert the APC */
                    Current->SuspendApc.Inserted = TRUE;
                    KiInsertQueueApc(&Current->SuspendApc, IO_NO_INCREMENT);
                }
                else
                {
                    /* Lock the dispatcher */
                    KiAcquireDispatcherLockAtDpcLevel();

                    /* Unsignal the semaphore, the APC was already inserted */
                    Current->SuspendSemaphore.Header.SignalState--;

                    /* Release the dispatcher */
                    KiReleaseDispatcherLockFromDpcLevel();
                }
            }
        }

        /* Release the APC lock */
        KiReleaseApcLockFromDpcLevel(&ApcLock);
    }

    /* Release the process lock and exit the dispatcher */
    KiReleaseProcessLock(&LockHandle);
    KiExitDispatcher(LockHandle.OldIrql);
}

ULONG
NTAPI
KeResumeThread(IN PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    ULONG PreviousCount;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the APC Queue */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Check if it existed */
    if (PreviousCount)
    {
        /* Decrease the suspend count */
        Thread->SuspendCount--;

        /* Check if the thrad is still suspended or not */
        if ((!Thread->SuspendCount) && (!Thread->FreezeCount))
        {
            /* Acquire the dispatcher lock */
            KiAcquireDispatcherLockAtDpcLevel();

            /* Signal the Suspend Semaphore */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);

            /* Release the dispatcher lock */
            KiReleaseDispatcherLockFromDpcLevel();
        }
    }

    /* Release APC Queue lock and return the Old State */
    KiReleaseApcLockFromDpcLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return PreviousCount;
}

VOID
NTAPI
KiSuspendRundown(IN PKAPC Apc)
{
    /* Does nothing */
    UNREFERENCED_PARAMETER(Apc);
}

VOID
NTAPI
KiSuspendNop(IN PKAPC Apc,
             IN PKNORMAL_ROUTINE *NormalRoutine,
             IN PVOID *NormalContext,
             IN PVOID *SystemArgument1,
             IN PVOID *SystemArgument2)
{
    /* Does nothing */
    UNREFERENCED_PARAMETER(Apc);
    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
}

VOID
NTAPI
KiSuspendThread(IN PVOID NormalContext,
                IN PVOID SystemArgument1,
                IN PVOID SystemArgument2)
{
    /* Non-alertable kernel-mode suspended wait */
    KeWaitForSingleObject(&KeGetCurrentThread()->SuspendSemaphore,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);
}

NTSTATUS
NTAPI
KeSuspendThread(PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    ULONG PreviousCount;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the APC Queue */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Handle the maximum */
    if (PreviousCount == MAXIMUM_SUSPEND_COUNT)
    {
        /* Raise an exception */
        KiReleaseApcLock(&ApcLock);
        RtlRaiseStatus(STATUS_SUSPEND_COUNT_EXCEEDED);
    }

    /* Should we bother to queue at all? */
    if (Thread->ApcQueueable)
    {
        /* Increment the suspend count */
        Thread->SuspendCount++;

        /* Check if we should suspend it */
        if (!(PreviousCount) && !(Thread->FreezeCount))
        {
            /* Is the APC already inserted? */
            if (!Thread->SuspendApc.Inserted)
            {
                /* Not inserted, insert it */
                Thread->SuspendApc.Inserted = TRUE;
                KiInsertQueueApc(&Thread->SuspendApc, IO_NO_INCREMENT);
            }
            else
            {
                /* Lock the dispatcher */
                KiAcquireDispatcherLockAtDpcLevel();

                /* Unsignal the semaphore, the APC was already inserted */
                Thread->SuspendSemaphore.Header.SignalState--;

                /* Release the dispatcher */
                KiReleaseDispatcherLockFromDpcLevel();
            }
        }
    }

    /* Release Lock and return the Old State */
    KiReleaseApcLockFromDpcLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return PreviousCount;
}

ULONG
NTAPI
KeForceResumeThread(IN PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    ULONG PreviousCount;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the APC Queue */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount + Thread->FreezeCount;

    /* If the thread is suspended, wake it up!!! */
    if (PreviousCount)
    {
        /* Unwait it completely */
        Thread->SuspendCount = 0;
        Thread->FreezeCount = 0;

        /* Lock the dispatcher */
        KiAcquireDispatcherLockAtDpcLevel();

        /* Signal and satisfy */
        Thread->SuspendSemaphore.Header.SignalState++;
        KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);

        /* Release the dispatcher */
        KiReleaseDispatcherLockFromDpcLevel();
    }

    /* Release Lock and return the Old State */
    KiReleaseApcLockFromDpcLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return PreviousCount;
}

ULONG
NTAPI
KeAlertResumeThread(IN PKTHREAD Thread)
{
    ULONG PreviousCount;
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    KiAcquireApcLock(Thread, &ApcLock);
    KiAcquireDispatcherLockAtDpcLevel();

    /* Return if Thread is already alerted. */
    if (!Thread->Alerted[KernelMode])
    {
        /* If it's Blocked, unblock if it we should */
        if ((Thread->State == Waiting) && (Thread->Alertable))
        {
            /* Abort the wait */
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);
        }
        else
        {
            /* If not, simply Alert it */
            Thread->Alerted[KernelMode] = TRUE;
        }
    }

    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount;

    /* If the thread is suspended, decrease one of the suspend counts */
    if (PreviousCount)
    {
        /* Decrease count. If we are now zero, unwait it completely */
        if (--Thread->SuspendCount)
        {
            /* Signal and satisfy */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
        }
    }

    /* Release Locks and return the Old State */
    KiReleaseDispatcherLockFromDpcLevel();
    KiReleaseApcLockFromDpcLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return PreviousCount;
}

BOOLEAN
NTAPI
KeAlertThread(IN PKTHREAD Thread,
              IN KPROCESSOR_MODE AlertMode)
{
    BOOLEAN PreviousState;
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    KiAcquireApcLock(Thread, &ApcLock);
    KiAcquireDispatcherLockAtDpcLevel();

    /* Save the Previous State */
    PreviousState = Thread->Alerted[AlertMode];

    /* Check if it's already alerted */
    if (!PreviousState)
    {
        /* Check if the thread is alertable, and blocked in the given mode */
        if ((Thread->State == Waiting) &&
            ((AlertMode == KernelMode) || (Thread->WaitMode == AlertMode)) &&
            (Thread->Alertable))
        {
            /* Abort the wait to alert the thread */
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);
        }
        else
        {
            /* Otherwise, merely set the alerted state */
            Thread->Alerted[AlertMode] = TRUE;
        }
    }

    /* Release the Dispatcher Lock */
    KiReleaseDispatcherLockFromDpcLevel();
    KiReleaseApcLockFromDpcLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);

    /* Return the old state */
    return PreviousState;
}

BOOLEAN
NTAPI
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode)
{
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN OldState;
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    KiAcquireApcLock(Thread, &ApcLock);
    KiAcquireDispatcherLockAtDpcLevel();

    /* Save the old State */
    OldState = Thread->Alerted[AlertMode];

    /* Check the Thread is alerted */
    if (OldState)
    {
        /* Disable alert for this mode */
        Thread->Alerted[AlertMode] = FALSE;
    }
    else if ((AlertMode != KernelMode) &&
             (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
    {
        /* If the mode is User and the Queue isn't empty, set Pending */
        Thread->ApcState.UserApcPending = TRUE;
    }

    /* Release Locks and return the Old State */
    KiReleaseDispatcherLockFromDpcLevel();
    KiReleaseApcLockFromDpcLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return OldState;
}

/*
 * @unimplemented
 */
VOID
STDCALL
KeCapturePersistentThreadState(IN PVOID CurrentThread,
                               IN ULONG Setting1,
                               IN ULONG Setting2,
                               IN ULONG Setting3,
                               IN ULONG Setting4,
                               IN ULONG Setting5,
                               IN PVOID ThreadState)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
KeUninitThread(IN PKTHREAD Thread)
{
    /* Delete the stack */
    MmDeleteKernelStack(Thread->StackBase, FALSE);
    Thread->InitialStack = NULL;
}

NTSTATUS
NTAPI
KeInitThread(IN OUT PKTHREAD Thread,
             IN PVOID KernelStack,
             IN PKSYSTEM_ROUTINE SystemRoutine,
             IN PKSTART_ROUTINE StartRoutine,
             IN PVOID StartContext,
             IN PCONTEXT Context,
             IN PVOID Teb,
             IN PKPROCESS Process)
{
    BOOLEAN AllocatedStack = FALSE;
    ULONG i;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER Timer;
    NTSTATUS Status;

    /* Initalize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Thread->DispatcherHeader,
                                 ThreadObject,
                                 sizeof(KTHREAD) / sizeof(LONG),
                                 FALSE);

    /* Initialize the Mutant List */
    InitializeListHead(&Thread->MutantListHead);

    /* Initialize the wait blocks */
    for (i = 0; i< (THREAD_WAIT_OBJECTS + 1); i++)
    {
        /* Put our pointer */
        Thread->WaitBlock[i].Thread = Thread;
    }

    /* Set swap settings */
    Thread->EnableStackSwap = FALSE;//TRUE;
    Thread->IdealProcessor = 1;
    Thread->SwapBusy = FALSE;
    Thread->AdjustReason = 0;

    /* Initialize the lock */
    KeInitializeSpinLock(&Thread->ThreadLock);

    /* Setup the Service Descriptor Table for Native Calls */
    Thread->ServiceTable = KeServiceDescriptorTable;

    /* Setup APC Fields */
    InitializeListHead(&Thread->ApcState.ApcListHead[0]);
    InitializeListHead(&Thread->ApcState.ApcListHead[1]);
    Thread->ApcState.Process = Process;
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStateIndex = OriginalApcEnvironment;
    Thread->ApcQueueable = TRUE;
    KeInitializeSpinLock(&Thread->ApcQueueLock);

    /* Initialize the Suspend APC */
    KeInitializeApc(&Thread->SuspendApc,
                    Thread,
                    OriginalApcEnvironment,
                    KiSuspendNop,
                    KiSuspendRundown,
                    KiSuspendThread,
                    KernelMode,
                    NULL);

    /* Initialize the Suspend Semaphore */
    KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 2);

    /* Setup the timer */
    Timer = &Thread->Timer;
    KeInitializeTimer(Timer);
    TimerWaitBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    TimerWaitBlock->Object = Timer;
    TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
    TimerWaitBlock->WaitType = WaitAny;
    TimerWaitBlock->NextWaitBlock = NULL;

    /* Link the two wait lists together */
    TimerWaitBlock->WaitListEntry.Flink = &Timer->Header.WaitListHead;
    TimerWaitBlock->WaitListEntry.Blink = &Timer->Header.WaitListHead;

    /* Set the TEB */
    Thread->Teb = Teb;

    /* Check if we have a kernel stack */
    if (!KernelStack)
    {
        /* We don't, allocate one */
        KernelStack = (PVOID)((ULONG_PTR)MmCreateKernelStack(FALSE) +
                              KERNEL_STACK_SIZE);
        if (!KernelStack) return STATUS_INSUFFICIENT_RESOURCES;

        /* Remember for later */
        AllocatedStack = TRUE;
    }

    /* Set the Thread Stacks */
    Thread->InitialStack = (PCHAR)KernelStack;
    Thread->StackBase = (PCHAR)KernelStack;
    Thread->StackLimit = (ULONG_PTR)KernelStack - KERNEL_STACK_SIZE;
    Thread->KernelStackResident = TRUE;

    /* ROS Mm HACK */
    MmUpdatePageDir((PEPROCESS)Process,
                    (PVOID)Thread->StackLimit,
                    KERNEL_STACK_SIZE);
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));

    /* Enter SEH to avoid crashes due to user mode */
    Status = STATUS_SUCCESS;
    _SEH_TRY
    {
        /* Initalize the Thread Context */
        Ke386InitThreadWithContext(Thread,
                                   SystemRoutine,
                                   StartRoutine,
                                   StartContext,
                                   Context);
    }
    _SEH_HANDLE
    {
        /* Set failure status */
        Status = STATUS_UNSUCCESSFUL;

        /* Check if a stack was allocated */
        if (AllocatedStack)
        {
            /* Delete the stack */
            MmDeleteKernelStack(Thread->StackBase, FALSE);
            Thread->InitialStack = NULL;
        }
    }
    _SEH_END;

    /* Set the Thread to initalized */
    Thread->State = Initialized;
    return Status;
}

VOID
NTAPI
KeInitializeThread(IN PKPROCESS Process,
                   IN OUT PKTHREAD Thread,
                   IN PKSYSTEM_ROUTINE SystemRoutine,
                   IN PKSTART_ROUTINE StartRoutine,
                   IN PVOID StartContext,
                   IN PCONTEXT Context,
                   IN PVOID Teb,
                   IN PVOID KernelStack)
{
    /* Initailize and start the thread on success */
    if (NT_SUCCESS(KeInitThread(Thread,
                                KernelStack,
                                SystemRoutine,
                                StartRoutine,
                                StartContext,
                                Context,
                                Teb,
                                Process)))
    {
        /* Start it */
        KeStartThread(Thread);
    }
}

VOID
NTAPI
KeStartThread(IN OUT PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE LockHandle;
#ifdef CONFIG_SMP
    PKNODE Node;
    PKPRCB NodePrcb;
    ULONG Set, Mask;
#endif
    UCHAR IdealProcessor = 0;
    PKPROCESS Process = Thread->ApcState.Process;

    /* Setup static fields from parent */
    Thread->Iopl = Process->Iopl;
    Thread->Quantum = Process->QuantumReset;
    Thread->QuantumReset = Process->QuantumReset;
    Thread->SystemAffinityActive = FALSE;

    /* Lock the process */
    KiAcquireProcessLock(Process, &LockHandle);

    /* Setup volatile data */
    Thread->Priority = Process->BasePriority;
    Thread->BasePriority = Process->BasePriority;
    Thread->Affinity = Process->Affinity;
    Thread->UserAffinity = Process->Affinity;

#ifdef CONFIG_SMP
    /* Get the KNODE and its PRCB */
    Node = KeNodeBlock[Process->IdealNode];
    NodePrcb = (PKPRCB)(KPCR_BASE + (Process->ThreadSeed * PAGE_SIZE));

    /* Calculate affinity mask */
    Set = ~NodePrcb->MultiThreadProcessorSet;
    Mask = (ULONG)(Node->ProcessorMask & Process->Affinity);
    Set &= Mask;
    if (Set) Mask = Set;

    /* Get the new thread seed */
    IdealProcessor = KeFindNextRightSetAffinity(Process->ThreadSeed, Mask);
    Process->ThreadSeed = IdealProcessor;

    /* Sanity check */
    ASSERT((Thread->UserAffinity & AFFINITY_MASK(IdealProcessor)));
#endif

    /* Set the Ideal Processor */
    Thread->IdealProcessor = IdealProcessor;
    Thread->UserIdealProcessor = IdealProcessor;

    /* Lock the Dispatcher Database */
    KiAcquireDispatcherLockAtDpcLevel();

    /* Insert the thread into the process list */
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);

    /* Increase the stack count */
    ASSERT(Process->StackCount != MAXULONG_PTR);
    Process->StackCount++;

    /* Release locks and return */
    KiReleaseDispatcherLockFromDpcLevel();
    KiReleaseProcessLock(&LockHandle);
}

/*
 * @implemented
 */
KPRIORITY
STDCALL
KeQueryPriorityThread (IN PKTHREAD Thread)
{
    return Thread->Priority;
}

/*
 * @implemented
 */
ULONG
STDCALL
KeQueryRuntimeThread(IN PKTHREAD Thread,
                     OUT PULONG UserTime)
{
    /* Return the User Time */
    *UserTime = Thread->UserTime;

    /* Return the Kernel Time */
    return Thread->KernelTime;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeSetKernelStackSwapEnable(IN BOOLEAN Enable)
{
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN PreviousState;
    KIRQL OldIrql;

     /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save Old State */
    PreviousState = Thread->EnableStackSwap;

    /* Set New State */
    Thread->EnableStackSwap = Enable;

    /* No, Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return Old State */
    return PreviousState;
}

/*
 * @implemented
 */
VOID
STDCALL
KeRevertToUserAffinityThread(VOID)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    KIRQL OldIrql;

    ASSERT(CurrentThread->SystemAffinityActive != FALSE);

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Return to User Affinity */
    CurrentThread->Affinity = CurrentThread->UserAffinity;

    /* Disable System Affinity */
    CurrentThread->SystemAffinityActive = FALSE;

    /* Check if we need to Dispatch a New thread */
    if (CurrentThread->Affinity & (1 << KeGetCurrentProcessorNumber())) {

        /* No, just release */
        KeReleaseDispatcherDatabaseLock(OldIrql);

    } else {

        /* We need to dispatch a new thread */
        CurrentThread->WaitIrql = OldIrql;
        KiDispatchThreadNoLock(Ready);
        KeLowerIrql(OldIrql);
    }
}

/*
 * @implemented
 */
UCHAR
STDCALL
KeSetIdealProcessorThread(IN PKTHREAD Thread,
                          IN UCHAR Processor)
{
    CCHAR PreviousIdealProcessor;
    KIRQL OldIrql;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save Old Ideal Processor */
    PreviousIdealProcessor = Thread->IdealProcessor;

    /* Set New Ideal Processor */
    Thread->IdealProcessor = Processor;

    /* Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return Old Ideal Processor */
    return PreviousIdealProcessor;
}

/*
 * @implemented
 */
VOID
STDCALL
KeSetSystemAffinityThread(IN KAFFINITY Affinity)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();
    KIRQL OldIrql;

    ASSERT(Affinity & ((1 << KeNumberProcessors) - 1));

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Set the System Affinity Specified */
    CurrentThread->Affinity = Affinity;

    /* Enable System Affinity */
    CurrentThread->SystemAffinityActive = TRUE;

    /* Check if we need to Dispatch a New thread */
    if (Affinity & (1 << KeGetCurrentProcessorNumber())) {

        /* No, just release */
        KeReleaseDispatcherDatabaseLock(OldIrql);

    } else {

        /* We need to dispatch a new thread */
        CurrentThread->WaitIrql = OldIrql;
        KiDispatchThreadNoLock(Ready);
        KeLowerIrql(OldIrql);
    }
}

LONG
STDCALL
KeQueryBasePriorityThread(IN PKTHREAD Thread)
{
    LONG BasePriorityIncrement;
    KIRQL OldIrql;
    PKPROCESS Process;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get the Process */
    Process = Thread->ApcStatePointer[0]->Process;

    /* Calculate the BPI */
    BasePriorityIncrement = Thread->BasePriority - Process->BasePriority;

    /* If saturation occured, return the SI instead */
    if (Thread->Saturation) BasePriorityIncrement = (HIGH_PRIORITY + 1) / 2 *
                                                    Thread->Saturation;

    /* Release Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return Increment */
    return BasePriorityIncrement;
}

/*
 * @implemented
 */
LONG
STDCALL
KeSetBasePriorityThread(PKTHREAD Thread,
                        LONG Increment)
{
    KIRQL OldIrql;
    PKPROCESS Process;
    KPRIORITY Priority;
    KPRIORITY CurrentBasePriority;
    KPRIORITY BasePriority;
    BOOLEAN Released = FALSE;
    LONG CurrentIncrement;
       
    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get the process and calculate current BP and BPI */
    Process = Thread->ApcStatePointer[0]->Process;
    CurrentBasePriority = Thread->BasePriority;
    CurrentIncrement = CurrentBasePriority - Process->BasePriority;

    /* Change to use the SI if Saturation was used */
    if (Thread->Saturation) CurrentIncrement = (HIGH_PRIORITY + 1) / 2 *
                                               Thread->Saturation;

    /* Now check if saturation is being used for the new value */
    if (abs(Increment) >= ((HIGH_PRIORITY + 1) / 2))
    {
        /* Check if we need positive or negative saturation */
        Thread->Saturation = (Increment > 0) ? 1 : -1;
    }

    /* Normalize the Base Priority */
    BasePriority = Process->BasePriority + Increment;
    if (Process->BasePriority >= LOW_REALTIME_PRIORITY)
    {
        /* Check if it's too low */
        if (BasePriority < LOW_REALTIME_PRIORITY)
            BasePriority = LOW_REALTIME_PRIORITY;

        /* Check if it's too high */
        if (BasePriority > HIGH_PRIORITY) BasePriority = HIGH_PRIORITY;

        /* We are at RTP, so use the raw BP */
        Priority = BasePriority;
    }
    else
    {
        /* Check if it's entering RTP */
        if (BasePriority >= LOW_REALTIME_PRIORITY)
            BasePriority = LOW_REALTIME_PRIORITY - 1;

        /* Check if it's too low */
        if (BasePriority <= LOW_PRIORITY)
            BasePriority = 1;

        /* If Saturation is used, then use the raw BP */
        if (Thread->Saturation)
        {
            Priority = BasePriority;
        }
        else
        {
            /* Calculate the new priority */
            Priority = Thread->Priority + (BasePriority - CurrentBasePriority)-
                       Thread->PriorityDecrement;

            /* Make sure it won't enter RTP ranges */
            if (Priority >= LOW_REALTIME_PRIORITY)
                Priority = LOW_REALTIME_PRIORITY - 1;
        }
    }

    /* Finally set the new base priority */
    Thread->BasePriority = BasePriority;

    /* Reset the decrements */
    Thread->PriorityDecrement = 0;

    /* If the priority will change, reset quantum and change it for real */
    if (Priority != Thread->Priority)
    {
        Thread->Quantum = Thread->QuantumReset;
        KiSetPriorityThread(Thread, Priority, &Released);
    }

    /* Release Lock if needed */
    if (!Released)
    {
        KeReleaseDispatcherDatabaseLock(OldIrql);
    }
    else
    {
        KeLowerIrql(OldIrql);
    }

    /* Return the Old Increment */
    return CurrentIncrement;
}

/*
 * @implemented
 */
KPRIORITY
STDCALL
KeSetPriorityThread(PKTHREAD Thread,
                    KPRIORITY Priority)
{
    KPRIORITY OldPriority;
    BOOLEAN Released = FALSE;
    KIRQL OldIrql;

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the old Priority */
    OldPriority = Thread->Priority;

    /* Reset the Quantum and Decrements */
    Thread->Quantum = Thread->QuantumReset;
    Thread->PriorityDecrement = 0;

    /* Set the new Priority */
    KiSetPriorityThread(Thread, Priority, &Released);

    /* Release Lock if needed */
    if (!Released)
    {
        KeReleaseDispatcherDatabaseLock(OldIrql);
    }
    else
    {
        KeLowerIrql(OldIrql);
    }

    /* Return Old Priority */
    return OldPriority;
}

/*
 * @implemented
 *
 * Sets thread's affinity
 */
KAFFINITY
STDCALL
KeSetAffinityThread(PKTHREAD Thread,
                    KAFFINITY Affinity)
{
    KIRQL OldIrql;
    KAFFINITY OldAffinity;
    BOOLEAN Released;

    DPRINT("KeSetAffinityThread(Thread %x, Affinity %x)\n", Thread, Affinity);

    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Call the internal function */
    OldAffinity = KiSetAffinityThread(Thread, Affinity, &Released);

    /* Release Lock if needed */
    if (!Released)
    {
        KeReleaseDispatcherDatabaseLock(OldIrql);
    }
    else
    {
        KeLowerIrql(OldIrql);
    }

    return OldAffinity;
}

/*
 * @implemented
 */
VOID
NTAPI
KeTerminateThread(IN KPRIORITY Increment)
{
    PLIST_ENTRY *ListHead;
    PETHREAD Entry, SavedEntry;
    PETHREAD *ThreadAddr;
    KLOCK_QUEUE_HANDLE LockHandle;
    PKTHREAD Thread = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLock(Process, &LockHandle);

    /* Make sure we won't get Swapped */
    KiSetThreadSwapBusy(Thread);

    /* Save the Kernel and User Times */
    Process->KernelTime += Thread->KernelTime;
    Process->UserTime += Thread->UserTime;

    /* Get the current entry and our Port */
    Entry = (PETHREAD)PspReaperListHead.Flink;
    ThreadAddr = &((PETHREAD)Thread)->ReaperLink;

    /* Add it to the reaper's list */
    do
    {
        /* Get the list head */
        ListHead = &PspReaperListHead.Flink;

        /* Link ourselves */
        *ThreadAddr = Entry;
        SavedEntry = Entry;

        /* Now try to do the exchange */
        Entry = InterlockedCompareExchangePointer(ListHead, ThreadAddr, Entry);

        /* Break out if the change was succesful */
    } while (Entry != SavedEntry);

    /* Acquire the dispatcher lock */
    KiAcquireDispatcherLockAtDpcLevel();

    /* Check if the reaper wasn't active */
    if (!Entry)
    {
        /* Activate it as a work item, directly through its Queue */
        KiInsertQueue(&ExWorkerQueue[HyperCriticalWorkQueue].WorkerQueue,
                      &PspReaperWorkItem.List,
                      FALSE);
    }

    /* Check the thread has an associated queue */
    if (Thread->Queue)
    {
        /* Remove it from the list, and handle the queue */
        RemoveEntryList(&Thread->QueueListEntry);
        KiWakeQueue(Thread->Queue);
    }

    /* Signal the thread */
    Thread->DispatcherHeader.SignalState = TRUE;
    if (IsListEmpty(&Thread->DispatcherHeader.WaitListHead) != TRUE)
    {
        /* Satisfy waits */
        KiWaitTest((PVOID)Thread, Increment);
    }

    /* Remove the thread from the list */
    RemoveEntryList(&Thread->ThreadListEntry);

    /* Release the process lock */
    KiReleaseProcessLock(&LockHandle);

    /* Set us as terminated, decrease the Process's stack count */
    Thread->State = Terminated;

    /* Decrease stack count */
    ASSERT(Process->StackCount != 0);
    ASSERT(Process->State == ProcessInMemory);
    Process->StackCount--;
    if (!Process->StackCount)
    {
        /* FIXME: Swap stacks */
    }

    /* Rundown arch-specific parts */
    KiRundownThread(Thread);

    /* Swap to a new thread */
    KiReleaseDispatcherLockFromDpcLevel();
    KiSwapThread(Thread, KeGetCurrentPrcb());
}
