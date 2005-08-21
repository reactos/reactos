/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kthread.c
 * PURPOSE:         Microkernel thread support
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FIXME: NDK */
#define MAXIMUM_SUSPEND_COUNT 0x7F
#define THREAD_ALERT_INCREMENT 2

extern EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];

/*
 * PURPOSE: List of threads associated with each priority level
 */
LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];
static ULONG PriorityListMask = 0;
ULONG IdleProcessorMask = 0;
extern PETHREAD PspReaperList;

/* FUNCTIONS *****************************************************************/

STATIC
VOID
KiRequestReschedule(CCHAR Processor)
{
    PKPCR Pcr;

    Pcr = (PKPCR)(KPCR_BASE + Processor * PAGE_SIZE);
    Pcr->Prcb->QuantumEnd = TRUE;
    KiIpiSendRequest(1 << Processor, IPI_REQUEST_DPC);
}

STATIC
VOID
KiInsertIntoThreadList(KPRIORITY Priority,
                       PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    ASSERT(Thread->Priority == Priority);

    if (Priority >= MAXIMUM_PRIORITY || Priority < LOW_PRIORITY) {

        DPRINT1("Invalid thread priority (%d)\n", Priority);
        KEBUGCHECK(0);
    }

    InsertTailList(&PriorityListHead[Priority], &Thread->WaitListEntry);
    PriorityListMask |= (1 << Priority);
}

STATIC
VOID
KiRemoveFromThreadList(PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    RemoveEntryList(&Thread->WaitListEntry);
    if (IsListEmpty(&PriorityListHead[(ULONG)Thread->Priority])) {

        PriorityListMask &= ~(1 << Thread->Priority);
    }
}

STATIC
PKTHREAD
KiScanThreadList(KPRIORITY Priority,
                 KAFFINITY Affinity)
{
    PLIST_ENTRY current_entry;
    PKTHREAD current;
    ULONG Mask;

    Mask = (1 << Priority);

    if (PriorityListMask & Mask) {

        current_entry = PriorityListHead[Priority].Flink;

        while (current_entry != &PriorityListHead[Priority]) {

            current = CONTAINING_RECORD(current_entry, KTHREAD, WaitListEntry);

            if (current->State != Ready) {

                DPRINT1("%d/%d\n", &current, current->State);
            }

            ASSERT(current->State == Ready);

            if (current->Affinity & Affinity) {

                KiRemoveFromThreadList(current);
                return(current);
            }

            current_entry = current_entry->Flink;
        }
    }

    return(NULL);
}

VOID
STDCALL
KiDispatchThreadNoLock(ULONG NewThreadStatus)
{
    KPRIORITY CurrentPriority;
    PKTHREAD Candidate;
    ULONG Affinity;
    PKTHREAD CurrentThread = KeGetCurrentThread();

    DPRINT("KiDispatchThreadNoLock() %d/%d/%d/%d\n", KeGetCurrentProcessorNumber(),
            CurrentThread, NewThreadStatus, CurrentThread->State);

    CurrentThread->State = (UCHAR)NewThreadStatus;

    if (NewThreadStatus == Ready) {

        KiInsertIntoThreadList(CurrentThread->Priority,
                               CurrentThread);
    }

    Affinity = 1 << KeGetCurrentProcessorNumber();

    for (CurrentPriority = HIGH_PRIORITY; CurrentPriority >= LOW_PRIORITY; CurrentPriority--) {

        Candidate = KiScanThreadList(CurrentPriority, Affinity);

        if (Candidate == CurrentThread) {

            Candidate->State = Running;
            KeReleaseDispatcherDatabaseLockFromDpcLevel();
            return;
        }

        if (Candidate != NULL) {

            PKTHREAD OldThread;
            PKTHREAD IdleThread;

            DPRINT("Scheduling %x(%d)\n",Candidate, CurrentPriority);

            Candidate->State = Running;

            OldThread = CurrentThread;
            CurrentThread = Candidate;
            IdleThread = KeGetCurrentPrcb()->IdleThread;

            if (OldThread == IdleThread) {

                IdleProcessorMask &= ~Affinity;

            } else if (CurrentThread == IdleThread) {

                IdleProcessorMask |= Affinity;
            }

            MmUpdatePageDir(PsGetCurrentProcess(),((PETHREAD)CurrentThread)->ThreadsProcess, sizeof(EPROCESS));

            /* Special note for Filip: This will release the Dispatcher DB Lock ;-) -- Alex */
            DPRINT("You are : %x, swapping to: %x\n", OldThread, CurrentThread);
            KiArchContextSwitch(CurrentThread);
            DPRINT("You are : %x, swapped from: %x\n", OldThread, CurrentThread);
            return;
        }
    }

    DPRINT1("CRITICAL: No threads are ready (CPU%d)\n", KeGetCurrentProcessorNumber());
    KEBUGCHECK(0);
}

VOID
STDCALL
KiBlockThread(PNTSTATUS Status,
              UCHAR Alertable,
              ULONG WaitMode,
              UCHAR WaitReason)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK WaitBlock;

    if (Thread->ApcState.KernelApcPending) {

        DPRINT("Dispatching Thread as ready (APC!)\n");

        /* Remove Waits */
        WaitBlock = Thread->WaitBlockList;
        do {
            RemoveEntryList (&WaitBlock->WaitListEntry);
            WaitBlock = WaitBlock->NextWaitBlock;
        } while (WaitBlock != Thread->WaitBlockList);
        Thread->WaitBlockList = NULL;

        /* Dispatch it and return status */
        KiDispatchThreadNoLock (Ready);
        if (Status != NULL) *Status = STATUS_KERNEL_APC;

    } else {

        /* Set the Thread Data as Requested */
        DPRINT("Dispatching Thread as blocked: %d\n", Thread->WaitStatus);
        Thread->Alertable = Alertable;
        Thread->WaitMode = (UCHAR)WaitMode;
        Thread->WaitReason = WaitReason;

        /* Dispatch it and return status */
        KiDispatchThreadNoLock(Waiting);
        DPRINT("Dispatching Thread as blocked: %d\n", Thread->WaitStatus);
        if (Status != NULL) *Status = Thread->WaitStatus;
    }

    DPRINT("Releasing Dispatcher Lock\n");
    KfLowerIrql(Thread->WaitIrql);
}

VOID
STDCALL
KiDispatchThread(ULONG NewThreadStatus)
{
    KIRQL OldIrql;

    if (KeGetCurrentPrcb()->IdleThread == NULL) {
        return;
    }

    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiDispatchThreadNoLock(NewThreadStatus);
    KeLowerIrql(OldIrql);
}

VOID
STDCALL
KiUnblockThread(PKTHREAD Thread,
                PNTSTATUS WaitStatus,
                KPRIORITY Increment)
{
    if (Terminated == Thread->State) {

        DPRINT("Can't unblock thread 0x%x because it's terminating\n",
               Thread);

    } else if (Ready == Thread->State ||
               Running == Thread->State) {

        DPRINT("Can't unblock thread 0x%x because it's %s\n",
               Thread, (Thread->State == Ready ? "ready" : "running"));

    } else {

        LONG Processor;
        KAFFINITY Affinity;

        /* FIXME: This propably isn't the right way to do it... */
        /* No it's not... i'll fix it later-- Alex */
        if (Thread->Priority < LOW_REALTIME_PRIORITY &&
            Thread->BasePriority < LOW_REALTIME_PRIORITY - 2) {

            if (!Thread->PriorityDecrement && !Thread->DisableBoost) {

                Thread->Priority = Thread->BasePriority + Increment;
                Thread->PriorityDecrement = Increment;
            }

            /* Also decrease quantum */
            Thread->Quantum--;

        } else {

            Thread->Quantum = Thread->QuantumReset;
        }

        if (WaitStatus != NULL) {

            Thread->WaitStatus = *WaitStatus;
        }

        Thread->State = Ready;
        KiInsertIntoThreadList(Thread->Priority, Thread);
        Processor = KeGetCurrentProcessorNumber();
        Affinity = Thread->Affinity;

        if (!(IdleProcessorMask & (1 << Processor) & Affinity) &&
             (IdleProcessorMask & ~(1 << Processor) & Affinity)) {

            LONG i;

            for (i = 0; i < KeNumberProcessors - 1; i++) {

                Processor++;

                if (Processor >= KeNumberProcessors) {

                    Processor = 0;
                }

                if (IdleProcessorMask & (1 << Processor) & Affinity) {
#if 0
                    /* FIXME:
                     *   Reschedule the threads on an other processor
                     */
                    KeReleaseDispatcherDatabaseLockFromDpcLevel();
                    KiRequestReschedule(Processor);
                    KeAcquireDispatcherDatabaseLockAtDpcLevel();
#endif
                    break;
                }
            }
        }
    }
}

VOID
STDCALL
KiAdjustQuantumThread(IN PKTHREAD Thread)
{
    KPRIORITY Priority;

    /* Don't adjust for RT threads */
    if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
        Thread->BasePriority < LOW_REALTIME_PRIORITY - 2)
    {
        /* Decrease Quantum by one and see if we've ran out */
        if (--Thread->Quantum <= 0)
        {
            /* Return quantum */
            Thread->Quantum = Thread->QuantumReset;

            /* Calculate new Priority */
            Priority = Thread->Priority - (Thread->PriorityDecrement + 1);

            /* Normalize it if we've gone too low */
            if (Priority < Thread->BasePriority) Priority = Thread->BasePriority;

            /* Reset the priority decrement, we've done it */
            Thread->PriorityDecrement = 0;

            /* Set the new priority, if needed */
            if (Priority != Thread->Priority)
            {
                /* 
                 * FIXME: This should be a call to KiSetPriorityThread but
                 * due to the current ""scheduler"" in ROS, it can't be done
                 * cleanly since it actualyl dispatches threads instead.
                 */
                Thread->Priority = Priority;
            }
            else
            {
                /* FIXME: Priority hasn't changed, find a new thread */
            }
        }
    }

    /* Nothing to do... */
    return;
}


VOID
STDCALL
KiSuspendThreadKernelRoutine(PKAPC Apc,
                             PKNORMAL_ROUTINE* NormalRoutine,
                             PVOID* NormalContext,
                             PVOID* SystemArgument1,
                             PVOID* SystemArguemnt2)
{
}

VOID
STDCALL
KiSuspendThreadNormalRoutine(PVOID NormalContext,
                             PVOID SystemArgument1,
                             PVOID SystemArgument2)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();

    /* Non-alertable kernel-mode suspended wait */
    DPRINT("Waiting...\n");
    KeWaitForSingleObject(&CurrentThread->SuspendSemaphore,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);
    DPRINT("Done Waiting\n");
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
    PLIST_ENTRY CurrentEntry;
    PKMUTANT Mutant;

    DPRINT("KeRundownThread: %x\n", Thread);

    /* Lock the Dispatcher Database */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    while (!IsListEmpty(&Thread->MutantListHead)) {

        /* Get the Mutant */
	CurrentEntry = RemoveHeadList(&Thread->MutantListHead);
        Mutant = CONTAINING_RECORD(CurrentEntry, KMUTANT, MutantListEntry);
        ASSERT(Mutant->ApcDisable == 0);

        /* Uncondtionally abandon it */
        DPRINT("Abandonning the Mutant\n");
        Mutant->Header.SignalState = 1;
        Mutant->Abandoned = TRUE;
        Mutant->OwnerThread = NULL;
        RemoveEntryList(&Mutant->MutantListEntry);

        /* Check if the Wait List isn't empty */
        DPRINT("Checking whether to wake the Mutant\n");
        if (!IsListEmpty(&Mutant->Header.WaitListHead)) {

            /* Wake the Mutant */
            DPRINT("Waking the Mutant\n");
            KiWaitTest(&Mutant->Header, MUTANT_INCREMENT);
        }
    }

    /* Release the Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

ULONG
STDCALL
KeResumeThread(PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    DPRINT("KeResumeThread (Thread %p called). %x, %x\n", Thread,
            Thread->SuspendCount, Thread->FreezeCount);

    /* Lock the Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Check if it existed */
    if (PreviousCount) {

        Thread->SuspendCount--;

        /* Decrease the current Suspend Count and Check Freeze Count */
        if ((!Thread->SuspendCount) && (!Thread->FreezeCount)) {

            /* Signal the Suspend Semaphore */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
        }
    }

    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

BOOLEAN
STDCALL
KiInsertQueueApc(PKAPC Apc,
                 KPRIORITY PriorityBoost);

/*
 * Used by the debugging code to freeze all the process's threads
 * while the debugger is examining their state.
 */
VOID
STDCALL
KeFreezeAllThreads(PKPROCESS Process)
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PKTHREAD Current;
    PKTHREAD CurrentThread = KeGetCurrentThread();

    /* Acquire Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Loop the Process's Threads */
    CurrentEntry = Process->ThreadListHead.Flink;
    while (CurrentEntry != &Process->ThreadListHead)
    {
        /* Get the Thread */
        Current = CONTAINING_RECORD(CurrentEntry, KTHREAD, ThreadListEntry);

        /* Make sure it's not ours */
        if (Current == CurrentThread) continue;

        /* Make sure it wasn't already frozen, and that it's not suspended */
        if (!(++Current->FreezeCount) && !(Current->SuspendCount))
        {
            /* Insert the APC */
            if (!KiInsertQueueApc(&Current->SuspendApc, IO_NO_INCREMENT))
            {
                /* Unsignal the Semaphore, the APC already got inserted */
                Current->SuspendSemaphore.Header.SignalState--;
            }
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    /* Release the lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

NTSTATUS
STDCALL
KeSuspendThread(PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    DPRINT("KeSuspendThread (Thread %p called). %x, %x\n", Thread, Thread->SuspendCount, Thread->FreezeCount);

    /* Lock the Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Handle the maximum */
    if (PreviousCount == MAXIMUM_SUSPEND_COUNT)
    {
        /* Raise an exception */
        KeReleaseDispatcherDatabaseLock(OldIrql);
        ExRaiseStatus(STATUS_SUSPEND_COUNT_EXCEEDED);
    }

    /* Increment it */
    Thread->SuspendCount++;

    /* Check if we should suspend it */
    if (!PreviousCount && !Thread->FreezeCount) {

        /* Insert the APC */
        if (!KiInsertQueueApc(&Thread->SuspendApc, IO_NO_INCREMENT)) {

            /* Unsignal the Semaphore, the APC already got inserted */
            Thread->SuspendSemaphore.Header.SignalState--;
        }
    }

    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

ULONG
STDCALL
KeForceResumeThread(IN PKTHREAD Thread)
{
    KIRQL OldIrql;
    ULONG PreviousCount;

    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount + Thread->FreezeCount;

    /* If the thread is suspended, wake it up!!! */
    if (PreviousCount) {

        /* Unwait it completely */
        Thread->SuspendCount = 0;
        Thread->FreezeCount = 0;

        /* Signal and satisfy */
        Thread->SuspendSemaphore.Header.SignalState++;
        KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
    }

    /* Release Lock and return the Old State */
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

ULONG
STDCALL
KeAlertResumeThread(IN PKTHREAD Thread)
{
    ULONG PreviousCount;
    KIRQL OldIrql;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiAcquireSpinLock(&Thread->ApcQueueLock);

    /* Return if Thread is already alerted. */
    if (Thread->Alerted[KernelMode] == FALSE) {

        /* If it's Blocked, unblock if it we should */
        if (Thread->State == Waiting &&  Thread->Alertable) {

            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);

        } else {

            /* If not, simply Alert it */
            Thread->Alerted[KernelMode] = TRUE;
        }
    }

    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount;

    /* If the thread is suspended, decrease one of the suspend counts */
    if (PreviousCount) {

        /* Decrease count. If we are now zero, unwait it completely */
        if (--Thread->SuspendCount) {

            /* Signal and satisfy */
            Thread->SuspendSemaphore.Header.SignalState++;
            KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);
        }
    }

    /* Release Locks and return the Old State */
    KiReleaseSpinLock(&Thread->ApcQueueLock);
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return PreviousCount;
}

BOOLEAN
STDCALL
KeAlertThread(PKTHREAD Thread,
              KPROCESSOR_MODE AlertMode)
{
    KIRQL OldIrql;
    BOOLEAN PreviousState;

    /* Acquire the Dispatcher Database Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Save the Previous State */
    PreviousState = Thread->Alerted[AlertMode];

    /* Return if Thread is already alerted. */
    if (PreviousState == FALSE) {

        /* If it's Blocked, unblock if it we should */
        if (Thread->State == Waiting &&
            (AlertMode == KernelMode || Thread->WaitMode == AlertMode) &&
            Thread->Alertable) {

            DPRINT("Aborting Wait\n");
            KiAbortWaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);

        } else {

            /* If not, simply Alert it */
            Thread->Alerted[AlertMode] = TRUE;
        }
    }

    /* Release the Dispatcher Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return the old state */
    return PreviousState;
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

/*
 * FUNCTION: Initialize the microkernel state of the thread
 */
VOID
STDCALL
KeInitializeThread(PKPROCESS Process,
                   PKTHREAD Thread,
                   PKSYSTEM_ROUTINE SystemRoutine,
                   PKSTART_ROUTINE StartRoutine,
                   PVOID StartContext,
                   PCONTEXT Context,
                   PVOID Teb,
                   PVOID KernelStack)
{
    /* Initalize the Dispatcher Header */
    DPRINT("Initializing Dispatcher Header for New Thread: %x in Process: %x\n", Thread, Process);
    KeInitializeDispatcherHeader(&Thread->DispatcherHeader,
                                 ThreadObject,
                                 sizeof(KTHREAD),
                                 FALSE);

    DPRINT("Thread Header Created. SystemRoutine: %x, StartRoutine: %x with Context: %x\n",
            SystemRoutine, StartRoutine, StartContext);
    DPRINT("UserMode Information. Context: %x, Teb: %x\n", Context, Teb);

    /* Initialize the Mutant List */
    InitializeListHead(&Thread->MutantListHead);

    /* Setup the Service Descriptor Table for Native Calls */
    Thread->ServiceTable = KeServiceDescriptorTable;

    /* Setup APC Fields */
    InitializeListHead(&Thread->ApcState.ApcListHead[0]);
    InitializeListHead(&Thread->ApcState.ApcListHead[1]);
    Thread->ApcState.Process = Process;
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStateIndex = OriginalApcEnvironment;
    KeInitializeSpinLock(&Thread->ApcQueueLock);

    /* Initialize the Suspend APC */
    KeInitializeApc(&Thread->SuspendApc,
                    Thread,
                    OriginalApcEnvironment,
                    KiSuspendThreadKernelRoutine,
                    NULL,
                    KiSuspendThreadNormalRoutine,
                    KernelMode,
                    NULL);

    /* Initialize the Suspend Semaphore */
    KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 128);

    /* FIXME OPTIMIZATION OF DOOM. DO NOT ENABLE FIXME */
#if 0
    Thread->WaitBlock[3].Object = (PVOID)&Thread->Timer;
    Thread->WaitBlock[3].Thread = Thread;
    Thread->WaitBlock[3].WaitKey = STATUS_TIMEOUT;
    Thread->WaitBlock[3].WaitType = WaitAny;
    Thread->WaitBlock[3].NextWaitBlock = NULL;
    InsertTailList(&Thread->Timer.Header.WaitListHead,
                   &Thread->WaitBlock[3].WaitListEntry);
#endif
    KeInitializeTimer(&Thread->Timer);

    /* Set the TEB */
    Thread->Teb = Teb;

    /* Set the Thread Stacks */
    Thread->InitialStack = (PCHAR)KernelStack + MM_STACK_SIZE;
    Thread->StackBase = (PCHAR)KernelStack + MM_STACK_SIZE;
    Thread->StackLimit = (ULONG_PTR)KernelStack;
    Thread->KernelStackResident = TRUE;

    /*
     * Establish the pde's for the new stack and the thread structure within the
     * address space of the new process. They are accessed while taskswitching or
     * while handling page faults. At this point it isn't possible to call the
     * page fault handler for the missing pde's.
     */
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread->StackLimit, MM_STACK_SIZE);
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));

    /* Initalize the Thread Context */
    DPRINT("Initializing the Context for the thread: %x\n", Thread);
    KiArchInitThreadWithContext(Thread,
                                SystemRoutine,
                                StartRoutine,
                                StartContext,
                                Context);

    /* Setup scheduler Fields based on Parent */
    DPRINT("Thread context created, setting Scheduler Data\n");
    Thread->BasePriority = Process->BasePriority;
    Thread->Quantum = Process->QuantumReset;
    Thread->QuantumReset = Process->QuantumReset;
    Thread->Affinity = Process->Affinity;
    Thread->Priority = Process->BasePriority;
    Thread->UserAffinity = Process->Affinity;
    Thread->DisableBoost = Process->DisableBoost;
    Thread->AutoAlignment = Process->AutoAlignment;
    Thread->Iopl = Process->Iopl;

    /* Set the Thread to initalized */
    Thread->State = Initialized;

    /*
     * Insert the Thread into the Process's Thread List
     * Note, this is the KTHREAD Thread List. It is removed in
     * ke/kthread.c!KeTerminateThread.
     */
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);
    DPRINT("Thread initalized\n");
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
CCHAR
STDCALL
KeSetIdealProcessorThread(IN PKTHREAD Thread,
                          IN CCHAR Processor)
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

VOID
STDCALL
KiSetPriorityThread(PKTHREAD Thread,
                    KPRIORITY Priority,
                    PBOOLEAN Released)
{
    KPRIORITY OldPriority = Thread->Priority;
    ULONG Mask;
    int i;
    PKPCR Pcr;
    DPRINT("Changing prio to : %lx\n", Priority);

    /* Check if priority changed */
    if (OldPriority != Priority)
    {
        /* Set it */
        Thread->Priority = Priority;

        /* Choose action based on thread's state */
        if (Thread->State == Ready)
        {
            /* Remove it from the current queue */
            KiRemoveFromThreadList(Thread);
            
            /* Re-insert it at its current priority */
            KiInsertIntoThreadList(Priority, Thread);

            /* Check if the old priority was lower */
            if (KeGetCurrentThread()->Priority < Priority)
            {
                /* Dispatch it immediately */
                KiDispatchThreadNoLock(Ready);
                *Released = TRUE;
                return;
            }
        }
        else if (Thread->State == Running)
        {
            /* Check if the new priority is lower */
            if (Priority < OldPriority)
            {
                /* Check for threads with a higher priority */
                Mask = ~((1 << (Priority + 1)) - 1);
                if (PriorityListMask & Mask)
                {
                    /* Found a thread, is it us? */
                    if (Thread == KeGetCurrentThread())
                    {
                        /* Dispatch us */
                        KiDispatchThreadNoLock(Ready);
                        *Released = TRUE;
                        return;
                    } 
                    else
                    {
                        /* Loop every CPU */
                        for (i = 0; i < KeNumberProcessors; i++)
                        {
                            /* Get the PCR for this CPU */
                            Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);

                            /* Reschedule if the new one is already on a CPU */
                            if (Pcr->Prcb->CurrentThread == Thread)
                            {
                                KeReleaseDispatcherDatabaseLockFromDpcLevel();
                                KiRequestReschedule(i);
                                *Released = TRUE;
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    /* Return to caller */
    return;
}

/*
 * Sets thread's base priority relative to the process' base priority
 * Should only be passed in THREAD_PRIORITY_ constants in pstypes.h
 *
 * @implemented
 */
LONG
STDCALL
KeSetBasePriorityThread (PKTHREAD Thread,
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
    Thread->DecrementCount = 0;
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
    Thread->DecrementCount = 0;
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
NTSTATUS
STDCALL
KeSetAffinityThread(PKTHREAD Thread,
                    KAFFINITY Affinity)
{
    KIRQL OldIrql;
    LONG i;
    PKPCR Pcr;
    KAFFINITY ProcessorMask;

    DPRINT("KeSetAffinityThread(Thread %x, Affinity %x)\n", Thread, Affinity);

    /* Verify correct affinity */
    if ((Affinity & Thread->ApcStatePointer[0]->Process->Affinity) !=
        Affinity || !Affinity)
    {
        KEBUGCHECK(INVALID_AFFINITY_SET);
    }

    OldIrql = KeAcquireDispatcherDatabaseLock();

    Thread->UserAffinity = Affinity;

    if (Thread->SystemAffinityActive == FALSE) {

        Thread->Affinity = Affinity;

        if (Thread->State == Running) {

            ProcessorMask = 1 << KeGetCurrentKPCR()->Number;
            if (Thread == KeGetCurrentThread()) {

                if (!(Affinity & ProcessorMask)) {

                    KiDispatchThreadNoLock(Ready);
                    KeLowerIrql(OldIrql);
                    return STATUS_SUCCESS;
                }

            } else {

                for (i = 0; i < KeNumberProcessors; i++) {

                    Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
                    if (Pcr->Prcb->CurrentThread == Thread) {

                        if (!(Affinity & ProcessorMask)) {

                            KeReleaseDispatcherDatabaseLockFromDpcLevel();
                            KiRequestReschedule(i);
                            KeLowerIrql(OldIrql);
                            return STATUS_SUCCESS;
                        }

                        break;
                    }
                }

                ASSERT (i < KeNumberProcessors);
            }
        }
    }

    KeReleaseDispatcherDatabaseLock(OldIrql);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
 /* The Increment Argument seems to be ignored by NT and always 0 when called */
VOID
STDCALL
KeTerminateThread(IN KPRIORITY Increment)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();

    /* Lock the Dispatcher Database and the APC Queue */
    DPRINT("Terminating\n");
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Remove the thread from the list */
    RemoveEntryList(&Thread->ThreadListEntry);

    /* Insert into the Reaper List */
    DPRINT("List: %p\n", PspReaperList);
    ((PETHREAD)Thread)->ReaperLink = PspReaperList;
    PspReaperList = (PETHREAD)Thread;
    DPRINT("List: %p\n", PspReaperList);

    /* Check if it's active */
    if (PspReaping == FALSE) {

        /* Activate it. We use the internal function for speed, and use the Hyper Critical Queue */
        PspReaping = TRUE;
        DPRINT("Terminating\n");
        KiInsertQueue(&ExWorkerQueue[HyperCriticalWorkQueue].WorkerQueue,
                      &PspReaperWorkItem.List,
                      FALSE);
    }

    /* Handle Kernel Queues */
    if (Thread->Queue) {

        DPRINT("Waking Queue\n");
        RemoveEntryList(&Thread->QueueListEntry);
        KiWakeQueue(Thread->Queue);
    }

    /* Signal the thread */
    Thread->DispatcherHeader.SignalState = TRUE;
    if (IsListEmpty(&Thread->DispatcherHeader.WaitListHead) != TRUE) {

        /* Satisfy waits */
        KiWaitTest((PVOID)Thread, Increment);
    }

    /* Find a new Thread */
    KiDispatchThreadNoLock(Terminated);
}

/*
 * FUNCTION: Tests whether there are any pending APCs for the current thread
 * and if so the APCs will be delivered on exit from kernel mode
 */
BOOLEAN
STDCALL
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN OldState;

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    OldIrql = KeAcquireDispatcherDatabaseLock();
    KiAcquireSpinLock(&Thread->ApcQueueLock);

    /* Save the old State */
    OldState = Thread->Alerted[AlertMode];

    /* If the Thread is Alerted, Clear it */
    if (OldState) {

        Thread->Alerted[AlertMode] = FALSE;

    } else if ((AlertMode != KernelMode) && (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]))) {

        /* If the mode is User and the Queue isn't empty, set Pending */
        Thread->ApcState.UserApcPending = TRUE;
    }

    /* Release Locks and return the Old State */
    KiReleaseSpinLock(&Thread->ApcQueueLock);
    KeReleaseDispatcherDatabaseLock(OldIrql);
    return OldState;
}

VOID
KiServiceCheck (VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Check if we need to inialize Win32 for this Thread */
    if (Thread->ServiceTable != KeServiceDescriptorTableShadow) {

        /* We do. Initialize it and save the new table */
        PsInitWin32Thread((PETHREAD)Thread);
        Thread->ServiceTable = KeServiceDescriptorTableShadow;
    }
}

/*
 *
 * NOT EXPORTED
 */
NTSTATUS
STDCALL
NtAlertResumeThread(IN  HANDLE ThreadHandle,
                    OUT PULONG SuspendCount)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status;
    ULONG PreviousState;

    /* Check if parameters are valid */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWriteUlong(SuspendCount);

        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;
    }

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);

    /* Check for Success */
    if (NT_SUCCESS(Status)) {

        /* Call the Kernel Function */
        PreviousState = KeAlertResumeThread(&Thread->Tcb);

        /* Dereference Object */
        ObDereferenceObject(Thread);

        if (SuspendCount) {

            _SEH_TRY {

                *SuspendCount = PreviousState;

            } _SEH_HANDLE {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 *
 * EXPORTED
 */
NTSTATUS
STDCALL
NtAlertThread (IN HANDLE ThreadHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status;

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);

    /* Check for Success */
    if (NT_SUCCESS(Status)) {

        /*
         * Do an alert depending on the processor mode. If some kmode code wants to
         * enforce a umode alert it should call KeAlertThread() directly. If kmode
         * code wants to do a kmode alert it's sufficient to call it with Zw or just
         * use KeAlertThread() directly
         */
        KeAlertThread(&Thread->Tcb, PreviousMode);

        /* Dereference Object */
        ObDereferenceObject(Thread);
    }

    /* Return status */
    return Status;
}

NTSTATUS
STDCALL
NtDelayExecution(IN BOOLEAN Alertable,
                 IN PLARGE_INTEGER DelayInterval)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeInterval;
    NTSTATUS Status;

    /* Check if parameters are valid */
    if(PreviousMode != KernelMode) {

        Status = STATUS_SUCCESS;
        
        _SEH_TRY {

            /* make a copy on the kernel stack and let DelayInterval point to it so
               we don't need to wrap KeDelayExecutionThread in SEH! */
            SafeInterval = ProbeForReadLargeInteger(DelayInterval);
            DelayInterval = &SafeInterval;

        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;
        
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
   }

   /* Call the Kernel Function */
   Status = KeDelayExecutionThread(PreviousMode,
                                   Alertable,
                                   DelayInterval);

   /* Return Status */
   return Status;
}
