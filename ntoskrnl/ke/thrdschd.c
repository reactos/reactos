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

#ifdef _WIN64
# define InterlockedOrSetMember(Destination, SetMember) \
    InterlockedOr64((PLONG64)Destination, SetMember);
#else
# define InterlockedOrSetMember(Destination, SetMember) \
    InterlockedOr((PLONG)Destination, SetMember);
#endif

/* GLOBALS *******************************************************************/

KAFFINITY KiIdleSummary;
KAFFINITY KiIdleSMTSummary;

/* FUNCTIONS *****************************************************************/

PKTHREAD
FASTCALL
KiIdleSchedule(IN PKPRCB Prcb)
{
    /* FIXME: TODO */
    ASSERTMSG("SMP: Not yet implemented\n", FALSE);
    return NULL;
}

VOID
FASTCALL
KiProcessDeferredReadyList(IN PKPRCB Prcb)
{
    PSINGLE_LIST_ENTRY ListEntry;
    PKTHREAD Thread;

    /* Make sure there is something on the ready list */
    ASSERT(Prcb->DeferredReadyListHead.Next != NULL);

    /* Get the first entry and clear the list */
    ListEntry = Prcb->DeferredReadyListHead.Next;
    Prcb->DeferredReadyListHead.Next = NULL;

    /* Start processing loop */
    do
    {
        /* Get the thread and advance to the next entry */
        Thread = CONTAINING_RECORD(ListEntry, KTHREAD, SwapListEntry);
        ListEntry = ListEntry->Next;

        /* Make the thread ready */
        KiDeferredReadyThread(Thread);
    } while (ListEntry != NULL);

    /* Make sure the ready list is still empty */
    ASSERT(Prcb->DeferredReadyListHead.Next == NULL);
}

VOID
FASTCALL
KiQueueReadyThread(IN PKTHREAD Thread,
                   IN PKPRCB Prcb)
{
    /* Call the macro. We keep the API for compatibility with ASM code */
    KxQueueReadyThread(Thread, Prcb);
}

VOID
FASTCALL
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
                              LOW_REALTIME_PRIORITY - 3);

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

                /* Check if this is a foreground process */
                if (CONTAINING_RECORD(Thread->ApcState.Process, EPROCESS, Pcb)->
                    Vm.Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND)
                {
                    /* Apply the foreground boost */
                    OldPriority += PsPrioritySeparation;
                }

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

    /* Queue the thread on CPU 0 and get the PRCB and lock it */
    Thread->NextProcessor = 0;
    Prcb = KiProcessorBlock[0];
    KiAcquirePrcbLock(Prcb);

    /* Check if we have an idle summary */
    if (KiIdleSummary)
    {
        /* Clear it and set this thread as the next one */
        KiIdleSummary = 0;
        Thread->State = Standby;
        Prcb->NextThread = Thread;

        /* Unlock the PRCB and return */
        KiReleasePrcbLock(Prcb);
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
                KiIpiSend(AFFINITY_MASK(Thread->NextProcessor), IPI_DPC);
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
        InterlockedOrSetMember(&KiIdleSummary, Prcb->SetMember);
        Prcb->IdleSchedule = TRUE;

        /* FIXME: SMT support */
        ASSERTMSG("SMP: Not yet implemented\n", FALSE);
    }

    /* Sanity checks and return the thread */
    ASSERT(Thread != NULL);
    ASSERT((Thread->BasePriority == 0) || (Thread->Priority != 0));
    return Thread;
}

LONG_PTR
FASTCALL
KiSwapThread(IN PKTHREAD CurrentThread,
             IN PKPRCB Prcb)
{
    BOOLEAN ApcState = FALSE;
    KIRQL WaitIrql;
    LONG_PTR WaitStatus;
    PKTHREAD NextThread;
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
            InterlockedOrSetMember(&KiIdleSummary, Prcb->SetMember);

            /* Schedule the idle thread */
            NextThread = Prcb->IdleThread;
            Prcb->CurrentThread = NextThread;
            NextThread->State = Running;
        }
    }

    /* Sanity check and release the PRCB */
    ASSERT(CurrentThread != Prcb->IdleThread);
    KiReleasePrcbLock(Prcb);

    /* Save the wait IRQL */
    WaitIrql = CurrentThread->WaitIrql;

    /* Swap contexts */
    ApcState = KiSwapContext(WaitIrql, CurrentThread);

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
        KiInsertDeferredReadyList(Thread);
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

                        /* Update priority */
                        Thread->Priority = (SCHAR)Priority;

                        /* Re-insert it at its current priority */
                        KiInsertDeferredReadyList(Thread);

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
                    }

                    /* Release the lock and check if we need an interrupt */
                    KiReleasePrcbLock(Prcb);
                    if (RequestInterrupt)
                    {
                        /* Check if we're running on another CPU */
                        if (KeGetCurrentProcessorNumber() != Processor)
                        {
                            /* We are, send an IPI */
                            KiIpiSend(AFFINITY_MASK(Processor), IPI_DPC);
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
                ASSERT(FALSE);
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
#ifdef CONFIG_SMP
        /* FIXME: TODO */
        DPRINT1("Affinity support disabled!\n");
#endif
    }

    /* Return the old affinity */
    return OldAffinity;
}

//
// This macro exists because NtYieldExecution locklessly attempts to read from
// the KPRCB's ready summary, and the usual way of going through KeGetCurrentPrcb
// would require getting fs:1C first (or gs), and then doing another dereference.
// In an attempt to minimize the amount of instructions and potential race/tear
// that could happen, Windows seems to define this as a macro that directly acceses
// the ready summary through a single fs: read by going through the KPCR's PrcbData.
//
// See http://research.microsoft.com/en-us/collaboration/global/asia-pacific/programs/trk_case4_process-thread_management.pdf (DEAD_LINK)
//
// We need this per-arch because sometimes it's Prcb and sometimes PrcbData, and
// because on x86 it's FS, and on x64 it's GS (not sure what it is on ARM/PPC).
//
#ifdef _M_IX86
#define KiGetCurrentReadySummary() __readfsdword(FIELD_OFFSET(KIPCR, PrcbData.ReadySummary))
#elif _M_AMD64
#define KiGetCurrentReadySummary() __readgsdword(FIELD_OFFSET(KIPCR, Prcb.ReadySummary))
#else
#define KiGetCurrentReadySummary() KeGetCurrentPrcb()->ReadySummary
#endif

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtYieldExecution(VOID)
{
    NTSTATUS Status;
    KIRQL OldIrql;
    PKPRCB Prcb;
    PKTHREAD Thread, NextThread;

    /* NB: No instructions (other than entry code) should preceed this line */

    /* Fail if there's no ready summary */
    if (!KiGetCurrentReadySummary()) return STATUS_NO_YIELD_PERFORMED;

    /* Now get the current thread, set the status... */
    Status = STATUS_NO_YIELD_PERFORMED;
    Thread = KeGetCurrentThread();

    /* Raise IRQL to synch and get the KPRCB now */
    OldIrql = KeRaiseIrqlToSynchLevel();
    Prcb = KeGetCurrentPrcb();

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
            KiSwapContext(APC_LEVEL, Thread);
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
}
