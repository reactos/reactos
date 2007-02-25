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

ULONG KiIdleSummary;
ULONG KiIdleSMTSummary;

/* FUNCTIONS *****************************************************************/

VOID
FASTCALL
KiQueueReadyThread(IN PKTHREAD Thread,
                   IN PKPRCB Prcb)
{
    /* Call the macro. We keep the API for compatibility with ASM code */
    KxQueueReadyThread(Thread, Prcb);
}

VOID
NTAPI
KiDeferredReadyThread(IN PKTHREAD Thread)
{
    PKPRCB Prcb;
    BOOLEAN Preempted;
    ULONG Processor = 0;
    KPRIORITY OldPriority;
    PKTHREAD NextThread;

    /* Sanity checks */
    ASSERT(Thread->State == DeferredReady);
    ASSERT((Thread->Priority >= 0) && (Thread->Priority <= HIGH_PRIORITY));

    /* Check if we have any adjusts to do */
    if (Thread->AdjustReason == AdjustBoost)
    {
        /* Lock the thread */
        KiAcquireThreadLock(Thread);

        /* Check if the priority is low enough to qualify for boosting */
        if ((Thread->Priority <= Thread->AdjustIncrement) &&
            (Thread->Priority < (LOW_REALTIME_PRIORITY - 3)) &&
            !(Thread->DisableBoost))
        {
            /* Calculate the new priority based on the adjust increment */
            OldPriority = min(Thread->AdjustIncrement + 1,
                              LOW_REALTIME_PRIORITY - 1);

            /* Make sure we're not decreasing outside of the priority range */
            ASSERT((Thread->PriorityDecrement >= 0) &&
                   (Thread->PriorityDecrement <= Thread->Priority));

            /* Calculate the new priority decrement based on the boost */
            Thread->PriorityDecrement += ((SCHAR)OldPriority - Thread->Priority);

            /* Again verify that this decrement is valid */
            ASSERT((Thread->PriorityDecrement >= 0) &&
                   (Thread->PriorityDecrement <= OldPriority));

            /* Set the new priority */
            Thread->Priority = (SCHAR)OldPriority;
        }

        /* We need 4 quanta, make sure we have them, then decrease by one */
        if (Thread->Quantum < 4) Thread->Quantum = 4;
        Thread->Quantum--;

        /* Make sure the priority is still valid */
        ASSERT((Thread->Priority >= 0) && (Thread->Priority <= HIGH_PRIORITY));

        /* Release the lock and clear the adjust reason */
        KiReleaseThreadLock(Thread);
        Thread->AdjustReason = AdjustNone;
    }
    else if (Thread->AdjustReason == AdjustUnwait)
    {
        /* Acquire the thread lock and check if this is a real-time thread */
        KiAcquireThreadLock(Thread);
        if (Thread->Priority < LOW_REALTIME_PRIORITY)
        {
            /* It's not real time, but is it time critical? */
            if (Thread->BasePriority >= (LOW_REALTIME_PRIORITY - 2))
            {
                /* It is, so simply reset its quantum */
                Thread->Quantum = Thread->QuantumReset;
            }
            else
            {
                /* Has the priority been adjusted previously? */
                if (!(Thread->PriorityDecrement) && (Thread->AdjustIncrement))
                {
                    /* Yes, reset its quantum */
                    Thread->Quantum = Thread->QuantumReset;
                }

                /* Wait code already handles quantum adjustment during APCs */
                if (Thread->WaitStatus != STATUS_KERNEL_APC)
                {
                    /* Decrease the quantum by one and check if we're out */
                    if (--Thread->Quantum <= 0)
                    {
                        /* We are, reset the quantum and get a new priority */
                        Thread->Quantum = Thread->QuantumReset;
                        Thread->Priority = KiComputeNewPriority(Thread, 1);
                    }
                }
            }

            /* Now check if we have no decrement and boosts are enabled */
            if (!(Thread->PriorityDecrement) && !(Thread->DisableBoost))
            {
                /* Make sure we have an increment */
                ASSERT(Thread->AdjustIncrement >= 0);

                /* Calculate the new priority after the increment */
                OldPriority = Thread->BasePriority + Thread->AdjustIncrement;

                /* Check if this new priority is higher */
                if (OldPriority > Thread->Priority)
                {
                    /* Make sure we don't go into the real time range */
                    if (OldPriority >= LOW_REALTIME_PRIORITY)
                    {
                        /* Normalize it back down one notch */
                        OldPriority = LOW_REALTIME_PRIORITY - 1;
                    }

                    /* Check if the priority is higher then the boosted base */
                    if (OldPriority > (Thread->BasePriority +
                                       Thread->AdjustIncrement))
                    {
                        /* Setup a priority decrement to nullify the boost  */
                        Thread->PriorityDecrement = ((SCHAR)OldPriority -
                                                    Thread->BasePriority -
                                                    Thread->AdjustIncrement);
                    }

                    /* Make sure that the priority decrement is valid */
                    ASSERT((Thread->PriorityDecrement >= 0) &&
                           (Thread->PriorityDecrement <= OldPriority));

                    /* Set this new priority */
                    Thread->Priority = (SCHAR)OldPriority;
                }
            }
        }
        else
        {
            /* It's a real-time thread, so just reset its quantum */
            Thread->Quantum = Thread->QuantumReset;
        }

        /* Make sure the priority makes sense */
        ASSERT((Thread->Priority >= 0) && (Thread->Priority <= HIGH_PRIORITY));

        /* Release the thread lock and reset the adjust reason */
        KiReleaseThreadLock(Thread);
        Thread->AdjustReason = AdjustNone;
    }

    /* Clear thread preemption status and save current values */
    Preempted = Thread->Preempted;
    OldPriority = Thread->Priority;
    Thread->Preempted = FALSE;

    /* Queue the thread on CPU 0 and get the PRCB */
    Thread->NextProcessor = 0;
    Prcb = KiProcessorBlock[0];

    /* Check if we have an idle summary */
    if (KiIdleSummary)
    {
        /* Clear it and set this thread as the next one */
        KiIdleSummary = 0;
        Thread->State = Standby;
        Prcb->NextThread = Thread;
        return;
    }

    /* Set the CPU number */
    Thread->NextProcessor = (UCHAR)Processor;

    /* Get the next scheduled thread */
    NextThread = Prcb->NextThread;
    if (NextThread)
    {
        /* Sanity check */
        ASSERT(NextThread->State == Standby);

        /* Check if priority changed */
        if (OldPriority > NextThread->Priority)
        {
            /* Preempt the thread */
            NextThread->Preempted = TRUE;

            /* Put this one as the next one */
            Thread->State = Standby;
            Prcb->NextThread = Thread;

            /* Set it in deferred ready mode */
            NextThread->State = DeferredReady;
            NextThread->DeferredProcessor = Prcb->Number;
            KiReleasePrcbLock(Prcb);
            KiDeferredReadyThread(NextThread);
            return;
        }
    }
    else
    {
        /* Set the next thread as the current thread */
        NextThread = Prcb->CurrentThread;
        if (OldPriority > NextThread->Priority)
        {
            /* Preempt it if it's already running */
            if (NextThread->State == Running) NextThread->Preempted = TRUE;

            /* Set the thread on standby and as the next thread */
            Thread->State = Standby;
            Prcb->NextThread = Thread;

            /* Release the lock */
            KiReleasePrcbLock(Prcb);

            /* Check if we're running on another CPU */
            if (KeGetCurrentProcessorNumber() != Thread->NextProcessor)
            {
                /* We are, send an IPI */
                KiIpiSendRequest(AFFINITY_MASK(Thread->NextProcessor), IPI_DPC);
            }
            return;
        }
    }

    /* Sanity check */
    ASSERT((OldPriority >= 0) && (OldPriority <= HIGH_PRIORITY));

    /* Set this thread as ready */
    Thread->State = Ready;
    Thread->WaitTime = KeTickCount.LowPart;

    /* Insert this thread in the appropriate order */
    Preempted ? InsertHeadList(&Prcb->DispatcherReadyListHead[OldPriority],
                               &Thread->WaitListEntry) :
                InsertTailList(&Prcb->DispatcherReadyListHead[OldPriority],
                               &Thread->WaitListEntry);

    /* Update the ready summary */
    Prcb->ReadySummary |= PRIORITY_MASK(OldPriority);

    /* Sanity check */
    ASSERT(OldPriority == Thread->Priority);

    /* Release the lock */
    KiReleasePrcbLock(Prcb);
}

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

    InsertTailList(&KeGetCurrentPrcb()->DispatcherReadyListHead[Priority], &Thread->WaitListEntry);
    KeGetCurrentPrcb()->ReadySummary |= (1 << Priority);
}

VOID
KiRemoveFromThreadList(PKTHREAD Thread)
{
    ASSERT(Ready == Thread->State);
    RemoveEntryList(&Thread->WaitListEntry);
    if (IsListEmpty(&KeGetCurrentPrcb()->DispatcherReadyListHead[Thread->Priority])) {

        KeGetCurrentPrcb()->ReadySummary &= ~(1 << Thread->Priority);
    }
}

PKTHREAD
KiScanThreadList(KPRIORITY Priority,
                 KAFFINITY Affinity)
{
    PKTHREAD current;
    ULONG Mask;

    Mask = (1 << Priority);

    if (KeGetCurrentPrcb()->ReadySummary & Mask) {

        LIST_FOR_EACH(current, &KeGetCurrentPrcb()->DispatcherReadyListHead[Priority], KTHREAD, WaitListEntry) {

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
        InterlockedOr((PLONG) &KiIdleSummary, Prcb->SetMember);
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
    BOOLEAN ApcState = FALSE;
    KIRQL WaitIrql;
    LONG_PTR WaitStatus;
    PKTHREAD NextThread;
#ifdef NEW_SCHEDULER
    PEPROCESS HackOfDoom = PsGetCurrentProcess();
#endif
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Acquire the PRCB lock */
    KiAcquirePrcbLock(Prcb);

    /* Get the next thread */
    NextThread = Prcb->NextThread;
    if (NextThread)
    {
        /* Already got a thread, set it up */
        Prcb->NextThread = NULL;
        Prcb->CurrentThread = NextThread;
        NextThread->State = Running;
    }
    else
    {
#ifdef NEW_SCHEDULER
        /* Try to find a ready thread */
        NextThread = KiSelectReadyThread(0, Prcb);
        if (NextThread)
        {
            /* Switch to it */
            Prcb->CurrentThread = NextThread;
            NextThread->State = Running;
        }
        else
        {
            /* Set the idle summary */
            InterlockedOr((PLONG)&KiIdleSummary, Prcb->SetMember);

            /* Schedule the idle thread */
            NextThread = Prcb->IdleThread;
            Prcb->CurrentThread = NextThread;
            NextThread->State = Running;
        }
#else
        /* Find a new thread to run */
        ApcState = KiDispatchThreadNoLock(Waiting);
#endif
    }

    /* Sanity check and release the PRCB */
    ASSERT(CurrentThread != Prcb->IdleThread);
    KiReleasePrcbLock(Prcb);

    /* Save the wait IRQL */
    WaitIrql = CurrentThread->WaitIrql;

#ifdef NEW_SCHEDULER
    /* REACTOS Mm Hack of Doom */
    MmUpdatePageDir(HackOfDoom,((PETHREAD)NextThread)->ThreadsProcess, sizeof(EPROCESS));

    /* Swap contexts */
    ApcState = KiSwapContext(CurrentThread, NextThread);
#endif

    /* Get the wait status */
    WaitStatus = CurrentThread->WaitStatus;

    /* Check if we need to deliver APCs */
    if (ApcState)
    {
        /* Lower to APC_LEVEL */
        KeLowerIrql(APC_LEVEL);

        /* Deliver APCs */
        KiDeliverApc(KernelMode, NULL, NULL);
        ASSERT(WaitIrql == 0);
    }

    /* Lower IRQL back to what it was and return the wait status */
    KeLowerIrql(WaitIrql);
    return WaitStatus;
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
        KEBUGCHECK(0);
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
        KEBUGCHECK(0);
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
    PKTHREAD NextThread;

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
            NextThread = NULL;
            KiDispatchThread(Ready);
#endif
        }
    }

    /* Release locks */
    KiReleasePrcbLock(Prcb);
    KiReleaseThreadLock(Thread);
    KiExitDispatcher(Thread->WaitIrql);
}

VOID
FASTCALL
KiSetPriorityThread(IN PKTHREAD Thread,
                    IN KPRIORITY Priority)
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
                if (!Thread->ProcessReadyQueue)
                {
                    /* Get the PRCB for the thread and lock it */
                    Processor = Thread->NextProcessor;
                    Prcb = KiProcessorBlock[Processor];
                    KiAcquirePrcbLock(Prcb);

                    /* Make sure the thread is still ready and on this CPU */
                    if ((Thread->State == Ready) &&
                        (Thread->NextProcessor == Prcb->Number))
                    {
#ifdef NEW_SCHEDULER
                        /* Sanity check */
                        ASSERT((Prcb->ReadySummary &
                                PRIORITY_MASK(Thread->Priority)));

                        /* Remove it from the current queue */
                        if (RemoveEntryList(&Thread->WaitListEntry))
                        {
                            /* Update the ready summary */
                            Prcb->ReadySummary ^= PRIORITY_MASK(Thread->
                                                                Priority);
                        }
#else
                        KiRemoveFromThreadList(Thread);
#endif

                        /* Update priority */
                        Thread->Priority = (SCHAR)Priority;

                        /* Re-insert it at its current priority */
#ifndef NEW_SCHEDULER
                        KiInsertIntoThreadList(Priority, Thread);
                        //KiDispatchThreadNoLock(Ready);
#else
                        KiInsertDeferredReadyList(Thread);
#endif

                        /* Release the PRCB Lock */
                        KiReleasePrcbLock(Prcb);
                    }
                    else
                    {
                        /* Release the lock and loop again */
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
                        if (KeGetCurrentPrcb()->ReadySummary & ~((1 << (Priority + 1)) - 1))
                        {
                            /* Found a thread, is it us? */
                            if (Thread == KeGetCurrentThread())
                            {
                                /* Dispatch us */
                                //KiDispatchThreadNoLock(Ready);
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
#ifdef NEW_SCHEDULER
    NTSTATUS Status = STATUS_NO_YIELD_PERFORMED;
    KIRQL OldIrql;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD Thread = KeGetCurrentThread(), NextThread;

    /* Fail if there's no ready summary */
    if (!Prcb->ReadySummary) return Status;

    /* Raise IRQL to synch */
    OldIrql = KeRaiseIrqlToSynchLevel();

    /* Now check if there's still a ready summary */
    if (Prcb->ReadySummary)
    {
        /* Acquire thread and PRCB lock */
        KiAcquireThreadLock(Thread);
        KiAcquirePrcbLock(Prcb);

        /* Find a new thread to run if none was selected */
        if (!Prcb->NextThread) Prcb->NextThread = KiSelectReadyThread(1, Prcb);

        /* Make sure we still have a next thread to schedule */
        NextThread = Prcb->NextThread;
        if (NextThread)
        {
            /* Reset quantum and recalculate priority */
            Thread->Quantum = Thread->QuantumReset;
            Thread->Priority = KiComputeNewPriority(Thread, 1);

            /* Release the thread lock */
            KiReleaseThreadLock(Thread);

            /* Set context swap busy */
            KiSetThreadSwapBusy(Thread);

            /* Set the new thread as running */
            Prcb->NextThread = NULL;
            Prcb->CurrentThread = NextThread;
            NextThread->State = Running;

            /* Setup a yield wait and queue the thread */
            Thread->WaitReason = WrYieldExecution;
            KxQueueReadyThread(Thread, Prcb);

            /* Make it wait at APC_LEVEL */
            Thread->WaitIrql = APC_LEVEL;

            /* Sanity check */
            ASSERT(OldIrql <= DISPATCH_LEVEL);

            /* Swap to new thread */
            KiSwapContext(Thread, NextThread);
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Release the PRCB and thread lock */
            KiReleasePrcbLock(Prcb);
            KiReleaseThreadLock(Thread);
        }
    }

    /* Lower IRQL and return */
    KeLowerIrql(OldIrql);
    return Status;
#else
    KiDispatchThread(Ready);
    return STATUS_SUCCESS;
#endif
}

