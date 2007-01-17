/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/thrdschd.c
 * PURPOSE:         Kernel Thread Scheduler (Affinity, Priority, Scheduling)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];
ULONG PriorityListMask = 0;
ULONG KiIdleSummary;
ULONG KiIdleSMTSummary;

/* FUNCTIONS *****************************************************************/

static
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

static
VOID
KiRemoveFromThreadList(PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    RemoveEntryList(&Thread->WaitListEntry);
    if (IsListEmpty(&PriorityListHead[(ULONG)Thread->Priority])) {

        PriorityListMask &= ~(1 << Thread->Priority);
    }
}

static
PKTHREAD
KiScanThreadList(KPRIORITY Priority,
                 KAFFINITY Affinity)
{
    PKTHREAD current;
    ULONG Mask;

    Mask = (1 << Priority);

    if (PriorityListMask & Mask) {

        LIST_FOR_EACH(current, &PriorityListHead[Priority], KTHREAD, WaitListEntry) {

            if (current->State != Ready) {

                DPRINT1("%p/%d\n", current, current->State);
            }

            ASSERT(current->State == Ready);

            if (current->Affinity & Affinity) {

                KiRemoveFromThreadList(current);
                return(current);
            }
        }
    }

    return(NULL);
}

BOOLEAN
STDCALL
KiDispatchThreadNoLock(ULONG NewThreadStatus)
{
    KPRIORITY CurrentPriority;
    PKTHREAD Candidate;
    ULONG Affinity;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    BOOLEAN ApcState;

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
            KiReleaseDispatcherLockFromDpcLevel();
            return FALSE;
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

                KiIdleSummary &= ~Affinity;

            } else if (CurrentThread == IdleThread) {

                KiIdleSummary |= Affinity;
            }

            MmUpdatePageDir((PEPROCESS)PsGetCurrentProcess(),((PETHREAD)CurrentThread)->ThreadsProcess, sizeof(EPROCESS));

            /* Special note for Filip: This will release the Dispatcher DB Lock ;-) -- Alex */
            DPRINT("You are : %x, swapping to: %x.\n", OldThread, CurrentThread);
            KeGetCurrentPrcb()->CurrentThread = CurrentThread;
            ApcState = KiSwapContext(OldThread, CurrentThread);
            DPRINT("You are : %x, swapped from: %x\n", OldThread, CurrentThread);
            return ApcState;
        }
    }

    DPRINT1("CRITICAL: No threads are ready (CPU%d)\n", KeGetCurrentProcessorNumber());
    KEBUGCHECK(0);
    return FALSE;
}

VOID
NTAPI
KiDeferredReadyThread(IN PKTHREAD Thread)
{
    /* FIXME: Not yet implemented */
    KEBUGCHECK(0);
}

PKTHREAD
FASTCALL
KiSelectNextThread(IN PKPRCB Prcb)
{
    PKTHREAD Thread;

    /* Select a ready thread */
    Thread = KiSelectReadyThread(0, Prcb);
    if (!Thread)
    {
        /* Didn't find any, get the current idle thread */
        Thread = Prcb->IdleThread;

        /* Enable idle scheduling */
        InterlockedOr(&KiIdleSummary, Prcb->SetMember);
        Prcb->IdleSchedule = TRUE;

        /* FIXME: SMT support */
    }

    /* Sanity checks and return the thread */
    ASSERT(Thread != NULL);
    ASSERT((Thread->BasePriority == 0) || (Thread->Priority != 0));
    return Thread;
}

NTSTATUS
FASTCALL
KiSwapThread(IN PKTHREAD CurrentThread,
             IN PKPRCB Prcb)
{
    BOOLEAN ApcState;
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

#ifdef NEW_SCHEDULER

#else
    /* Find a new thread to run */
    ApcState = KiDispatchThreadNoLock(Waiting);
#endif

    /* Check if we need to deliver APCs */
    if (ApcState)
    {
        /* Lower to APC_LEVEL */
        KeLowerIrql(APC_LEVEL);

        /* Deliver APCs */
        KiDeliverApc(KernelMode, NULL, NULL);
        ASSERT(CurrentThread->WaitIrql == 0);
    }

    /* Lower IRQL back to what it was */
    KfLowerIrql(CurrentThread->WaitIrql);

    /* Return the wait status */
    return CurrentThread->WaitStatus;
}

VOID
STDCALL
KiDispatchThread(ULONG NewThreadStatus)
{
    KIRQL OldIrql;

    if (KeGetCurrentPrcb()->IdleThread == NULL) {
        return;
    }

    OldIrql = KiAcquireDispatcherLock();
    KiDispatchThreadNoLock(NewThreadStatus);
    KeLowerIrql(OldIrql);
}

VOID
NTAPI
KiReadyThread(IN PKTHREAD Thread)
{
    IN PKPROCESS Process = Thread->ApcState.Process;

    /* Check if the process is paged out */
    if (Process->State != ProcessInMemory)
    {
        /* We don't page out processes in ROS */
        ASSERT(FALSE);
    }
    else if (!Thread->KernelStackResident)
    {
        /* Increase the stack count */
        ASSERT(Process->StackCount != MAXULONG_PTR);
        Process->StackCount++;

        /* Set the thread to transition */
        ASSERT(Thread->State != Transition);
        Thread->State = Transition;

        /* The stack is always resident in ROS */
        ASSERT(FALSE);
    }
    else
    {
        /* Insert the thread on the deferred ready list */
#ifdef NEW_SCHEDULER
        KiInsertDeferredReadyList(Thread);
#else
        /* Insert the thread into the thread list */
        Thread->State = Ready;
        KiInsertIntoThreadList(Thread->Priority, Thread);
#endif
    }
}

VOID
NTAPI
KiAdjustQuantumThread(IN PKTHREAD Thread)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Acquire thread and PRCB lock */
    KiAcquireThreadLock(Thread);
    KiAcquirePrcbLock(Prcb);

    /* Don't adjust for RT threads */
    if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
        (Thread->BasePriority < (LOW_REALTIME_PRIORITY - 2)))
    {
        /* Decrease Quantum by one and see if we've ran out */
        if (--Thread->Quantum <= 0)
        {
            /* Return quantum */
            Thread->Quantum = Thread->QuantumReset;

            /* Calculate new Priority */
            Thread->Priority = KiComputeNewPriority(Thread, 1);

#ifdef NEW_SCHEDULER
            /* Check if there's no next thread scheduled */
            if (!Prcb->NextThread)
            {
                /* Select a ready thread and check if we found one */
                NextThread = KiSelectReadyThread(Thread->Priority, Prcb);
                if (NextThread)
                {
                    /* Set it on standby and switch to it */
                    NextThread->State = Standby;
                    Prcb->NextThread = NextThread;
                }
            }
            else
            {
                /* This thread can be preempted again */
                Thread->Preempted = FALSE;
            }
#else
            /* We need to dispatch a new thread */
            KiDispatchThread(Ready);
#endif
        }
    }

    /* Release locks */
    KiReleasePrcbLock(Prcb);
    KiReleaseThreadLock(Thread);
}

VOID
STDCALL
KiSetPriorityThread(IN PKTHREAD Thread,
                    IN KPRIORITY Priority,
                    OUT PBOOLEAN Released)
{
    PKPRCB Prcb;
    ULONG Processor;
    BOOLEAN RequestInterrupt = FALSE;
    KPRIORITY OldPriority;
    PKTHREAD NewThread;
    ASSERT((Priority >= 0) && (Priority <= HIGH_PRIORITY));

    /* Check if priority changed */
    if (Thread->Priority != Priority)
    {
        /* Loop priority setting in case we need to start over */
        for (;;)
        {
            /* Choose action based on thread's state */
            if (Thread->State == Ready)
            {
                /* Make sure we're not on the ready queue */
                if (Thread->ProcessReadyQueue)
                {
                    /* Get the PRCB for the thread and lock it */
                    Processor = Thread->NextProcessor;
                    Prcb = KiProcessorBlock[Processor];
                    KiAcquirePrcbLock(Prcb);

                    /* Make sure the thread is still ready and on this CPU */
                    if ((Thread->State == Ready) &&
                        (Thread->NextProcessor == Prcb->Number))
                    {
                        /* Sanity check */
                        ASSERT((Prcb->ReadySummary &
                                PRIORITY_MASK(Thread->Priority)));

                        /* Remove it from the current queue */
#ifdef NEW_SCHEDULER
                        if (RemoveEntryList(&Thread->WaitListEntry))
                        {
                            /* Update the ready summary */
                            Prcb->ReadySummary ^= PRIORITY_MASK(Thread->Priority);
                        }
#else
                        KiRemoveFromThreadList(Thread);
#endif

                        /* Update priority */
                        Thread->Priority = (SCHAR)Priority;

                        /* Re-insert it at its current priority */
#ifndef NEW_SCHEDULER
                        KiInsertIntoThreadList(Priority, Thread);
                        KiDispatchThreadNoLock(Ready);
                        *Released = TRUE;
#else
                        KiInsertDeferredReadyList(Thread);
#endif

                        /* Release the PRCB Lock */
                        KiReleasePrcbLock(Prcb);
                    }
                    else
                    {
                        /* Release the lock and loop again */
                        KEBUGCHECK(0);
                        KiReleasePrcbLock(Prcb);
                        continue;
                    }
                }
                else
                {
                    /* It's already on the ready queue, just update priority */
                    Thread->Priority = (SCHAR)Priority;
                }
            }
            else if (Thread->State == Standby)
            {
                /* Get the PRCB for the thread and lock it */
                KEBUGCHECK(0);
                Processor = Thread->NextProcessor;
                Prcb = KiProcessorBlock[Processor];
                KiAcquirePrcbLock(Prcb);

                /* Check if we're still the next thread to run */
                if (Thread == Prcb->NextThread)
                {
                    /* Get the old priority and update ours */
                    OldPriority = Thread->Priority;
                    Thread->Priority = (SCHAR)Priority;

                    /* Check if there was a change */
                    if (Priority < OldPriority)
                    {
                        /* Find a new thread */
                        NewThread = KiSelectReadyThread(Priority + 1, Prcb);
                        if (NewThread)
                        {
                            /* Found a new one, set it on standby */
                            NewThread->State = Standby;
                            Prcb->NextThread = NewThread;

                            /* Dispatch our thread */
                            KiInsertDeferredReadyList(Thread);
                        }
                    }

                    /* Release the PRCB lock */
                    KiReleasePrcbLock(Prcb);
                }
                else
                {
                    /* Release the lock and try again */
                    KiReleasePrcbLock(Prcb);
                    continue;
                }
            }
            else if (Thread->State == Running)
            {
                /* Get the PRCB for the thread and lock it */
                Processor = Thread->NextProcessor;
                Prcb = KiProcessorBlock[Processor];
                KiAcquirePrcbLock(Prcb);

                /* Check if we're still the current thread running */
                if (Thread == Prcb->CurrentThread)
                {
                    /* Get the old priority and update ours */
                    OldPriority = Thread->Priority;
                    Thread->Priority = (SCHAR)Priority;

                    /* Check if there was a change and there's no new thread */
                    if ((Priority < OldPriority) && !(Prcb->NextThread))
                    {
#ifdef NEW_SCHEDULER
                        /* Find a new thread */
                        NewThread = KiSelectReadyThread(Priority + 1, Prcb);
                        if (NewThread)
                        {
                            /* Found a new one, set it on standby */
                            NewThread->State = Standby;
                            Prcb->NextThread = NewThread;

                            /* Request an interrupt */
                            RequestInterrupt = TRUE;
                        }
#else
                        /* Check for threads with a higher priority */
                        if (PriorityListMask & ~((1 << (Priority + 1)) - 1))
                        {
                            /* Found a thread, is it us? */
                            if (Thread == KeGetCurrentThread())
                            {
                                /* Dispatch us */
                                KiDispatchThreadNoLock(Ready);
                                *Released = TRUE;
                                return;
                            }
                        }
#endif
                    }

                    /* Release the lock and check if we need an interrupt */
                    KiReleasePrcbLock(Prcb);
                    if (RequestInterrupt)
                    {
                        /* Check if we're running on another CPU */
                        if (KeGetCurrentProcessorNumber() != Processor)
                        {
                            /* We are, send an IPI */
                            KiIpiSendRequest(AFFINITY_MASK(Processor), IPI_DPC);
                        }
                    }
                }
                else
                {
                    /* Thread changed, release lock and restart */
                    KiReleasePrcbLock(Prcb);
                    continue;
                }
            }
            else if (Thread->State == DeferredReady)
            {
                /* FIXME: TODO */
                DPRINT1("Deferred state not yet supported\n");
                KEBUGCHECK(0);
            }
            else
            {
                /* Any other state, just change priority */
                Thread->Priority = (SCHAR)Priority;
            }

            /* If we got here, then thread state was consistent, so bail out */
            break;
        }
    }

    /* Return to caller */
    *Released = FALSE;
}

KAFFINITY
FASTCALL
KiSetAffinityThread(IN PKTHREAD Thread,
                    IN KAFFINITY Affinity)
{
    KAFFINITY OldAffinity;

    /* Get the current affinity */
    OldAffinity = Thread->UserAffinity;

    /* Make sure that the affinity is valid */
    if (((Affinity & Thread->ApcState.Process->Affinity) != (Affinity)) ||
        (!Affinity))
    {
        /* Bugcheck the system */
        KeBugCheck(INVALID_AFFINITY_SET);
    }

    /* Update the new affinity */
    Thread->UserAffinity = Affinity;

    /* Check if system affinity is disabled */
    if (!Thread->SystemAffinityActive)
    {
        /* FIXME: TODO */
        DPRINT1("Affinity support disabled!\n");
    }

    /* Return the old affinity */
    return OldAffinity;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtYieldExecution(VOID)
{
    //
    // TODO (nothing too hard, just want to test out other code)
    //
    //DPRINT1("NO YIELD PERFORMED! If you see this, contact Alex\n");
    //return STATUS_NO_YIELD_PERFORMED;
    KiDispatchThread(Ready);
    return STATUS_SUCCESS;
}


