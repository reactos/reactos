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
KiRequestReschedule(CCHAR Processor)
{
    PKPCR Pcr;

    Pcr = (PKPCR)(KPCR_BASE + Processor * PAGE_SIZE);
    Pcr->Prcb->QuantumEnd = TRUE;
    KiIpiSendRequest(1 << Processor, IPI_DPC);
}

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

NTSTATUS
FASTCALL
KiSwapThread(IN PKTHREAD CurrentThread,
             IN PKPRCB Prcb)
{
    BOOLEAN ApcState;
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Find a new thread to run */
    ApcState = KiDispatchThreadNoLock(Waiting);

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
#if 0
        KiInsertDeferredReadyList(Thread);
#else
        /* Insert the thread into the thread list */
        Thread->State = Ready;
        KiInsertIntoThreadList(Thread->Priority, Thread);
#endif
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
                Thread->Priority = (SCHAR)Priority;
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
        Thread->Priority = (SCHAR)Priority;

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
                                KiReleaseDispatcherLockFromDpcLevel();
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
    *Released = FALSE;
    return;
}

KAFFINITY
NTAPI
KiSetAffinityThread(IN PKTHREAD Thread,
                    IN KAFFINITY Affinity,
                    PBOOLEAN Released)
{
    KAFFINITY OldAffinity;
    ULONG ProcessorMask;
    CCHAR i;
    PKPCR Pcr;

    /* Make sure that the affinity is valid */
    if (((Affinity & Thread->ApcState.Process->Affinity) != (Affinity)) ||
        (!Affinity))
    {
        /* Bugcheck the system */
        KeBugCheck(INVALID_AFFINITY_SET);
    }

    /* Get the old affinity */
    OldAffinity = Thread->UserAffinity;

    Thread->UserAffinity = Affinity;

    if (Thread->SystemAffinityActive == FALSE) {

        Thread->Affinity = Affinity;

        if (Thread->State == Running) {

            ProcessorMask = 1 << KeGetCurrentProcessorNumber();
            if (Thread == KeGetCurrentThread()) {

                if (!(Affinity & ProcessorMask)) {

                    KiDispatchThreadNoLock(Ready);
                    *Released = TRUE;
                    return OldAffinity;
                }

            } else {

                for (i = 0; i < KeNumberProcessors; i++) {

                    Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
                    if (Pcr->Prcb->CurrentThread == Thread) {

                        if (!(Affinity & ProcessorMask)) {

                            KiReleaseDispatcherLockFromDpcLevel();
                            KiRequestReschedule(i);
                            *Released = TRUE;
                            return OldAffinity;
                        }

                        break;
                    }
                }

                ASSERT (i < KeNumberProcessors);
            }
        }
    }

    *Released = FALSE;
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

