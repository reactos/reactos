/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         NT Implementation of APCs
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Phillip Susi
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*++
 * KiCheckForKernelApcDelivery
 * @implemented NT 5.2
 *
 *     The KiCheckForKernelApcDelivery routine is called whenever APCs have just
 *     been re-enabled in Kernel Mode, such as after leaving a Critical or
 *     Guarded Region. It delivers APCs if the environment is right.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     This routine allows KeLeave/EnterCritical/GuardedRegion to be used as a
 *     macro from inside WIN32K or other Drivers, which will then only have to
 *     do an Import API call in the case where APCs are enabled again.
 *
 *--*/
VOID
STDCALL
KiCheckForKernelApcDelivery(VOID)
{
    /* We should only deliver at passive */
    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        /* Raise to APC and Deliver APCs, then lower back to Passive */
        KfRaiseIrql(APC_LEVEL);
        KiDeliverApc(KernelMode, 0, 0);
        KfLowerIrql(PASSIVE_LEVEL);
    }
    else
    {
        /*
         * If we're not at passive level it means someone raised IRQL
         * to APC level before the a critical or guarded section was entered
         * (e.g) by a fast mutex). This implies that the APCs shouldn't
         * be delivered now, but after the IRQL is lowered to passive
         * level again.
         */
        KeGetCurrentThread()->ApcState.KernelApcPending = TRUE;
        HalRequestSoftwareInterrupt(APC_LEVEL);
    }
}

/*++
 * KeEnterCriticalRegion
 * @implemented NT4
 *
 *     The KeEnterCriticalRegion routine temporarily disables the delivery of
 *     normal kernel APCs; special kernel-mode APCs are still delivered.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Highest-level drivers can call this routine while running in the context
 *     of the thread that requested the current I/O operation. Any caller of
 *     this routine should call KeLeaveCriticalRegion as quickly as possible.
 *
 *     Callers of KeEnterCriticalRegion must be running at IRQL <= APC_LEVEL.
 *
 *--*/
#undef KeEnterCriticalRegion
VOID
STDCALL
KeEnterCriticalRegion(VOID)
{
    /* Disable Kernel APCs */
    PKTHREAD Thread = KeGetCurrentThread();
    if (Thread) Thread->KernelApcDisable--;
}

/*++
 * KeLeaveCriticalRegion
 * @implemented NT4
 *
 *     The KeLeaveCriticalRegion routine reenables the delivery of normal
 *     kernel-mode APCs that were disabled by a call to KeEnterCriticalRegion.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Highest-level drivers can call this routine while running in the context
 *     of the thread that requested the current I/O operation.
 *
 *     Callers of KeLeaveCriticalRegion must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
#undef KeLeaveCriticalRegion
VOID
STDCALL
KeLeaveCriticalRegion (VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Check if Kernel APCs are now enabled */
    if((Thread) && (++Thread->KernelApcDisable == 0))
    {
        /* Check if we need to request an APC Delivery */
        if ((!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) &&
            (Thread->SpecialApcDisable == 0))
        {
            /* Check for the right environment */
            KiCheckForKernelApcDelivery();
        }
    }
}

/*++
 * KeInitializeApc
 * @implemented NT4
 *
 *     The The KeInitializeApc routine initializes an APC object, and registers
 *     the Kernel, Rundown and Normal routines for that object.
 *
 * Params:
 *     Apc - Pointer to a KAPC structure that represents the APC object to
 *           initialize. The caller must allocate storage for the structure
 *           from resident memory.
 *
 *     Thread - Thread to which to deliver the APC.
 *
 *     TargetEnvironment - APC Environment to be used.
 *
 *     KernelRoutine - Points to the KernelRoutine to associate with the APC.
 *                     This routine is executed for all APCs.
 *
 *     RundownRoutine - Points to the RundownRoutine to associate with the APC.
 *                      This routine is executed when the Thread exists with
 *                      the APC executing.
 *
 *     NormalRoutine - Points to the NormalRoutine to associate with the APC.
 *                     This routine is executed at PASSIVE_LEVEL. If this is
 *                     not specifed, the APC becomes a Special APC and the
 *                     Mode and Context parameters are ignored.
 *
 *     Mode - Specifies the processor mode at which to run the Normal Routine.
 *
 *     Context - Specifices the value to pass as Context parameter to the
 *               registered routines.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     The caller can queue an initialized APC with KeInsertQueueApc.
 *
 *     Storage for the APC object must be resident, such as nonpaged pool
 *     allocated by the caller.
 *
 *     Callers of this routine must be running at IRQL = PASSIVE_LEVEL.
 *
 *--*/
VOID
STDCALL
KeInitializeApc(IN PKAPC Apc,
                IN PKTHREAD Thread,
                IN KAPC_ENVIRONMENT TargetEnvironment,
                IN PKKERNEL_ROUTINE KernelRoutine,
                IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
                IN PKNORMAL_ROUTINE NormalRoutine,
                IN KPROCESSOR_MODE Mode,
                IN PVOID Context)
{
    DPRINT("KeInitializeApc(Apc %x, Thread %x, Environment %d, "
           "KernelRoutine %x, RundownRoutine %x, NormalRoutine %x, Mode %d, "
           "Context %x)\n",Apc,Thread,TargetEnvironment,KernelRoutine,RundownRoutine,
            NormalRoutine,Mode,Context);

    /* Set up the basic APC Structure Data */
    RtlZeroMemory(Apc, sizeof(KAPC));
    Apc->Type = ApcObject;
    Apc->Size = sizeof(KAPC);

    /* Set the Environment */
    if (TargetEnvironment == CurrentApcEnvironment) {

        Apc->ApcStateIndex = Thread->ApcStateIndex;

    } else {

        Apc->ApcStateIndex = TargetEnvironment;
    }

    /* Set the Thread and Routines */
    Apc->Thread = Thread;
    Apc->KernelRoutine = KernelRoutine;
    Apc->RundownRoutine = RundownRoutine;
    Apc->NormalRoutine = NormalRoutine;

    /* Check if this is a Special APC, in which case we use KernelMode and no Context */
    if (ARGUMENT_PRESENT(NormalRoutine)) {

        Apc->ApcMode = Mode;
        Apc->NormalContext = Context;

    } else {

        Apc->ApcMode = KernelMode;
    }
}

static
__inline
VOID
KiRequestApcInterrupt(IN PKTHREAD Thread)
{
#ifdef CONFIG_SMP
    PKPRCB Prcb, CurrentPrcb;
    LONG i;

    CurrentPrcb = KeGetCurrentPrcb();
    for (i = 0; i < KeNumberProcessors; i++)
    {
        Prcb = ((PKPCR)(KPCR_BASE + i * PAGE_SIZE))->Prcb;
        if (Prcb->CurrentThread == Thread)
        {
            ASSERT (CurrentPrcb != Prcb);
            KiIpiSendRequest(Prcb->SetMember, IPI_APC);
            break;
        }
    }
    ASSERT (i < KeNumberProcessors);
#else
    HalRequestSoftwareInterrupt(APC_LEVEL);
#endif
}

/*++
 * KiInsertQueueApc
 *
 *     The KiInsertQueueApc routine queues a APC for execution when the right
 *     scheduler environment exists.
 *
 * Params:
 *     Apc - Pointer to an initialized control object of type DPC for which the
 *           caller provides the storage.
 *
 *     PriorityBoost - Priority Boost to apply to the Thread.
 *
 * Returns:
 *     None
 *
 * Remarks:
 *     The APC will execute at APC_LEVEL for the KernelRoutine registered, and
 *     at PASSIVE_LEVEL for the NormalRoutine registered.
 *
 *     Callers of this routine must have locked the dipatcher database.
 *
 *--*/
VOID
FASTCALL
KiInsertQueueApc(PKAPC Apc,
                 KPRIORITY PriorityBoost)
{
    PKTHREAD Thread = Apc->Thread;
    PKAPC_STATE ApcState;
    KPROCESSOR_MODE ApcMode;
    PLIST_ENTRY ListHead, NextEntry;
    PKAPC QueuedApc;
    NTSTATUS Status;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Acquire the lock (only needed on MP) */
    KeAcquireSpinLockAtDpcLevel(&Thread->ApcQueueLock);

    /* Little undocumented feature: Special Apc State Index */
    if (Apc->ApcStateIndex == 3)
    {
        /* This tells us to use the thread's */
        Apc->ApcStateIndex = Thread->ApcStateIndex;
    }

    /* Get the APC State for this Index, and the mode too */
    ApcState = Thread->ApcStatePointer[(int)Apc->ApcStateIndex];
    ApcMode = Apc->ApcMode;

    /* Three scenarios:
     * 1) Kernel APC with Normal Routine or User APC = Put it at the end of the List
     * 2) User APC which is PsExitSpecialApc = Put it at the front of the List
     * 3) Kernel APC without Normal Routine = Put it at the end of the No-Normal Routine Kernel APC list
     */
    if (Apc->NormalRoutine)
    {
        /* Normal APC; is it the Thread Termination APC? */
        if ((ApcMode != KernelMode) && (Apc->KernelRoutine == PsExitSpecialApc))
        {
            /* Set User APC pending to true */
            Thread->ApcState.UserApcPending = TRUE;

            /* Insert it at the top of the list */
            InsertHeadList(&ApcState->ApcListHead[ApcMode], &Apc->ApcListEntry);

            /* Display debug message */
            DPRINT("Inserted the Thread Exit APC for '%.16s' into the Queue\n",
                   ((PETHREAD)Thread)->ThreadsProcess->ImageFileName);
        }
        else
        {
            /* Regular user or kernel Normal APC */
            InsertTailList(&ApcState->ApcListHead[ApcMode], &Apc->ApcListEntry);

            /* Display debug message */
            DPRINT("Inserted Normal APC for '%.16s' into the Queue\n",
                   ((PETHREAD)Thread)->ThreadsProcess->ImageFileName);
        }
    }
    else
    {
        /* Special APC, find the first Normal APC in the list */
        ListHead = &ApcState->ApcListHead[ApcMode];
        NextEntry = ListHead->Flink;
        while(NextEntry != ListHead)
        {
            /* Get the APC */
            QueuedApc = CONTAINING_RECORD(NextEntry, KAPC, ApcListEntry);

            /* Is this a Normal APC? If so, break */
            if (QueuedApc->NormalRoutine) break;

            /* Move to the next APC in the Queue */
            NextEntry = NextEntry->Flink;
        }

        /* Move to the APC before this one (ie: the last Special APC) */
        NextEntry = NextEntry->Blink;

        /* Insert us here */
        InsertHeadList(NextEntry, &Apc->ApcListEntry);
        DPRINT("Inserted Special APC for '%.16s' into the Queue\n",
               ((PETHREAD)Thread)->ThreadsProcess->ImageFileName);
    }

    /* Now check if the Apc State Indexes match */
    if (Thread->ApcStateIndex == Apc->ApcStateIndex)
    {
        /* Check that if the thread matches */
        if (Thread == KeGetCurrentThread())
        {
            /* Check if this is kernel mode */
            if (ApcMode == KernelMode)
            {
                /* All valid, a Kernel APC is pending now */
                Thread->ApcState.KernelApcPending = TRUE;

                /* Check if Special APCs are disabled */
                if (Thread->SpecialApcDisable == 0)
                {
                    /* They're not, so request the interrupt */
                    HalRequestSoftwareInterrupt(APC_LEVEL);
                }
            }
        }
        else
        {
            /* Check if this is a non-kernel mode APC */
            if (ApcMode != KernelMode)
            {
                /* Not a Kernel-Mode APC. Are we waiting in user-mode? */
                if ((Thread->State == Waiting) && (Thread->WaitMode == UserMode))
                {
                    /* The thread is waiting. Are we alertable, or is an APC pending */
                    if ((Thread->Alertable) || (Thread->ApcState.UserApcPending))
                    {
                        /* Set user-mode APC pending */
                        Thread->ApcState.UserApcPending = TRUE;
                        Status = STATUS_USER_APC;
                        goto Unwait;
                    }
                }
            }
            else
            {
                /* Kernel-mode APC, set us pending */
                Thread->ApcState.KernelApcPending = TRUE;

                /* Are we currently running? */
                if (Thread->State == Running)
                {
                    /* The thread is running, so send an APC request */
                    KiRequestApcInterrupt(Thread);
                }
                else
                {
                    /*
                     * If the thread is Waiting at PASSIVE_LEVEL AND
                     *      Special APCs are not disabled AND
                     *          He is a Normal APC AND
                     *              Kernel APCs are not disabled AND
                     *                  Kernel APC is not pending OR
                     *          He is a Special APC THEN
                     *              Unwait thread with STATUS_KERNEL_APC
                     */
                    if ((Thread->State == Waiting) &&
                        (Thread->WaitIrql == PASSIVE_LEVEL) &&
                        (!Thread->SpecialApcDisable) && 
                        ((!Apc->NormalRoutine) ||
                         ((!Thread->KernelApcDisable) &&
                         (!Thread->ApcState.KernelApcInProgress))))
                    {
                        /* We'll unwait with this status */
                        Status = STATUS_KERNEL_APC;

                        /* Wake up the thread */
Unwait:
                        DPRINT("Waking up Thread for %lx Delivery \n", Status);
                        KiAbortWaitThread(Thread, Status, PriorityBoost);
                    }
                    else
                    {
                        /* FIXME: Handle deferred ready sometime far far in the future */
                    }
                }
            }
        }
    }

    /* Return to caller */
    KeReleaseSpinLockFromDpcLevel(&Thread->ApcQueueLock);
    return;
}

/*++
 * KeInsertQueueApc
 * @implemented NT4
 *
 *     The KeInsertQueueApc routine queues a APC for execution when the right
 *     scheduler environment exists.
 *
 * Params:
 *     Apc - Pointer to an initialized control object of type DPC for which the
 *           caller provides the storage.
 *
 *     SystemArgument[1,2] - Pointer to a set of two parameters that contain
 *                           untyped data.
 *
 *     PriorityBoost - Priority Boost to apply to the Thread.
 *
 * Returns:
 *     If the APC is already inserted or APC queueing is disabled, FALSE.
 *     Otherwise, TRUE.
 *
 * Remarks:
 *     The APC will execute at APC_LEVEL for the KernelRoutine registered, and
 *     at PASSIVE_LEVEL for the NormalRoutine registered.
 *
 *     Callers of this routine must be running at IRQL = PASSIVE_LEVEL.
 *
 *--*/
BOOLEAN
STDCALL
KeInsertQueueApc(PKAPC Apc,
                 PVOID SystemArgument1,
                 PVOID SystemArgument2,
                 KPRIORITY PriorityBoost)
{
    PKTHREAD Thread = Apc->Thread;
    KLOCK_QUEUE_HANDLE ApcLock;
    BOOLEAN State = TRUE;

    /* Get the APC lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Make sure we can Queue APCs and that this one isn't already inserted */
    if ((Thread->ApcQueueable == FALSE) && (Apc->Inserted == TRUE))
    {
        /* Fail */
        State = FALSE;
    }
    else
    {
        /* Set the System Arguments and set it as inserted */
        Apc->SystemArgument1 = SystemArgument1;
        Apc->SystemArgument2 = SystemArgument2;
        Apc->Inserted = TRUE;

        /* Call the Internal Function */
        KiInsertQueueApc(Apc, PriorityBoost);
    }

    /* Release the APC lock and return success */
    KiReleaseApcLock(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return State;
}

/*++
 * KeFlushQueueApc
 *
 *     The KeFlushQueueApc routine flushes all APCs of the given processor mode
 *     from the specified Thread's APC queue.
 *
 * Params:
 *     Thread - Pointer to the thread whose APC queue will be flushed.
 *
 *     PreviousMode - Specifies which APC Queue to flush.
 *
 * Returns:
 *     A pointer to the first entry in the flushed APC queue.
 *
 * Remarks:
 *     If the routine returns NULL, it means that no APCs were to be flushed.
 *
 *     Callers of KeFlushQueueApc must be running at DISPATCH_LEVEL or lower.
 *
 *--*/
PLIST_ENTRY
STDCALL
KeFlushQueueApc(IN PKTHREAD Thread,
                IN KPROCESSOR_MODE PreviousMode)
{
    PKAPC Apc;
    PLIST_ENTRY FirstEntry, CurrentEntry;
    KLOCK_QUEUE_HANDLE ApcLock;

    /* Get the APC lock */
    KiAcquireApcLock(Thread, &ApcLock);

    if (IsListEmpty(&Thread->ApcState.ApcListHead[PreviousMode])) {
        FirstEntry = NULL;
    } else {
        FirstEntry = Thread->ApcState.ApcListHead[PreviousMode].Flink;
        RemoveEntryList(&Thread->ApcState.ApcListHead[PreviousMode]);
        CurrentEntry = FirstEntry;
        do {
            Apc = CONTAINING_RECORD(CurrentEntry, KAPC, ApcListEntry);
            Apc->Inserted = FALSE;
            CurrentEntry = CurrentEntry->Flink;
        } while (CurrentEntry != FirstEntry);
    }

    /* Release the lock */
    KiReleaseApcLock(&ApcLock);

    /* Return the first entry */
    return FirstEntry;
}

/*++
 * KeRemoveQueueApc
 *
 *     The KeRemoveQueueApc routine removes a given APC object from the system
 *     APC queue.
 *
 * Params:
 *     APC - Pointer to an initialized APC object that was queued by calling
 *           KeInsertQueueApc.
 *
 * Returns:
 *     TRUE if the APC Object is in the APC Queue. If it isn't, no operation is
 *     performed and FALSE is returned.
 *
 * Remarks:
 *     If the given APC Object is currently queued, it is removed from the queue
 *     and any calls to the registered routines are cancelled.
 *
 *     Callers of KeLeaveCriticalRegion can be running at any IRQL.
 *
 *--*/
BOOLEAN
STDCALL
KeRemoveQueueApc(PKAPC Apc)
{
    PKTHREAD Thread = Apc->Thread;
    PKAPC_STATE ApcState;
    BOOLEAN Inserted;
    KLOCK_QUEUE_HANDLE ApcLock;

    /* Get the APC lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Check if it's inserted */
    if ((Inserted = Apc->Inserted))
    {
        /* Remove it from the Queue*/
        Apc->Inserted = FALSE;
        ApcState = Thread->ApcStatePointer[(int)Apc->ApcStateIndex];
        RemoveEntryList(&Apc->ApcListEntry);

        /* If the Queue is completely empty, then no more APCs are pending */
        if (IsListEmpty(&Thread->ApcStatePointer[(int)Apc->ApcStateIndex]->ApcListHead[(int)Apc->ApcMode]))
        {
            /* Set the correct State based on the Apc Mode */
            if (Apc->ApcMode == KernelMode)
            {
                ApcState->KernelApcPending = FALSE;
            }
            else
            {
                ApcState->UserApcPending = FALSE;
            }
        }
    }

    /* Release the lock and return */
    KiReleaseApcLock(&ApcLock);
    return Inserted;
}

/*++
 * KiDeliverApc
 * @implemented @NT4
 *
 *     The KiDeliverApc routine is called from IRQL switching code if the
 *     thread is returning from an IRQL >= APC_LEVEL and Kernel-Mode APCs are
 *     pending.
 *
 * Params:
 *     DeliveryMode - Specifies the current processor mode.
 *
 *     Reserved - Pointer to the Exception Frame on non-i386 builds.
 *
 *     TrapFrame - Pointer to the Trap Frame.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     First, Special APCs are delivered, followed by Kernel-Mode APCs and
 *     User-Mode APCs. Note that the TrapFrame is only valid if the previous
 *     mode is User.
 *
 *     Upon entry, this routine executes at APC_LEVEL.
 *
 *--*/
VOID
STDCALL
KiDeliverApc(KPROCESSOR_MODE DeliveryMode,
             PVOID Reserved,
             PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;
    PKTRAP_FRAME OldTrapFrame;
    PLIST_ENTRY ApcListEntry;
    PKAPC Apc;
    KIRQL OldIrql;
    PKKERNEL_ROUTINE KernelRoutine;
    PVOID NormalContext;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    ASSERT_IRQL_EQUAL(APC_LEVEL);

    /* Save the old trap frame */
    OldTrapFrame = Thread->TrapFrame;

    /* Clear Kernel APC Pending */
    Thread->ApcState.KernelApcPending = FALSE;

    /* Check if Special APCs are disabled */
    if (Thread->SpecialApcDisable != 0) goto Quickie;

    /* Do the Kernel APCs first */
    while (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
    {
        /* Lock the APC Queue and Raise IRQL to Synch */
        KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);

        /* Get the next Entry */
        ApcListEntry = Thread->ApcState.ApcListHead[KernelMode].Flink;
        Apc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);

        /* Save Parameters so that it's safe to free the Object in Kernel Routine*/
        NormalRoutine = Apc->NormalRoutine;
        KernelRoutine = Apc->KernelRoutine;
        NormalContext = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;

        /* Special APC */
        if (!NormalRoutine)
        {
            /* Remove the APC from the list */
            RemoveEntryList(ApcListEntry);
            Apc->Inserted = FALSE;

            /* Go back to APC_LEVEL */
            KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);

            /* Call the Special APC */
            DPRINT("Delivering a Special APC: %x\n", Apc);
            KernelRoutine(Apc,
                          &NormalRoutine,
                          &NormalContext,
                          &SystemArgument1,
                          &SystemArgument2);
        }
        else
        {
            /* Normal Kernel APC, make sur APCs aren't disabled or in progress*/
            if ((Thread->ApcState.KernelApcInProgress) ||
                (Thread->KernelApcDisable))
            {
                /*
                 * DeliveryMode must be KernelMode in this case, since one may not
                 * return to umode while being inside a critical section or while
                 * a regular kmode apc is running (the latter should be impossible btw).
                 * -Gunnar
                 */
                ASSERT(DeliveryMode == KernelMode);

                /* Release lock and return */
                KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
                goto Quickie;
            }

            /* Dequeue the APC */
            RemoveEntryList(ApcListEntry);
            Apc->Inserted = FALSE;

            /* Go back to APC_LEVEL */
            KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);

            /* Call the Kernel APC */
            DPRINT("Delivering a Normal APC: %x\n", Apc);
            KernelRoutine(Apc,
                          &NormalRoutine,
                          &NormalContext,
                          &SystemArgument1,
                          &SystemArgument2);

            /* If There still is a Normal Routine, then we need to call this at PASSIVE_LEVEL */
            if (NormalRoutine)
            {
                /* At Passive Level, this APC can be prempted by a Special APC */
                Thread->ApcState.KernelApcInProgress = TRUE;
                KeLowerIrql(PASSIVE_LEVEL);

                /* Call and Raise IRQ back to APC_LEVEL */
                DPRINT("Calling the Normal Routine for a Normal APC: %x\n", Apc);
                NormalRoutine(NormalContext, SystemArgument1, SystemArgument2);
                KeRaiseIrql(APC_LEVEL, &OldIrql);
            }

            /* Set Kernel APC in progress to false and loop again */
            Thread->ApcState.KernelApcInProgress = FALSE;
        }
    }

    /* Now we do the User APCs */
    if ((DeliveryMode == UserMode) &&
        (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])) &&
         (Thread->ApcState.UserApcPending == TRUE))
    {
        /* Lock the APC Queue and Raise IRQL to Synch */
        KeAcquireSpinLock(&Thread->ApcQueueLock, &OldIrql);

        /* It's not pending anymore */
        Thread->ApcState.UserApcPending = FALSE;

        /* Get the APC Entry */
        ApcListEntry = Thread->ApcState.ApcListHead[UserMode].Flink;

        /* Is it empty now? */
        if (ApcListEntry == &Thread->ApcState.ApcListHead[UserMode])
        {
            /* Release the lock and return */
            KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);
            goto Quickie;
        }

        /* Get the actual APC object */
        Apc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);

        /* Save Parameters so that it's safe to free the Object in Kernel Routine*/
        NormalRoutine = Apc->NormalRoutine;
        KernelRoutine = Apc->KernelRoutine;
        NormalContext = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;

        /* Remove the APC from Queue, restore IRQL and call the APC */
        RemoveEntryList(ApcListEntry);
        Apc->Inserted = FALSE;
        KeReleaseSpinLock(&Thread->ApcQueueLock, OldIrql);

        DPRINT("Calling the Kernel Routine for for a User APC: %x\n", Apc);
        KernelRoutine(Apc,
                      &NormalRoutine,
                      &NormalContext,
                      &SystemArgument1,
                      &SystemArgument2);

        if (!NormalRoutine)
        {
            /* Check if more User APCs are Pending */
            KeTestAlertThread(UserMode);
        }
        else
        {
            /* Set up the Trap Frame and prepare for Execution in NTDLL.DLL */
            DPRINT("Delivering a User APC: %x\n", Apc);
            KiInitializeUserApc(Reserved,
                                TrapFrame,
                                NormalRoutine,
                                NormalContext,
                                SystemArgument1,
                                SystemArgument2);
        }
    }

Quickie:
    /* Make sure we're still in the same process */
    if (Process != Thread->ApcState.Process)
    {
        /* Erm, we got attached or something! BAD! */
        KEBUGCHECKEX(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     Thread->ApcStateIndex,
                     KeGetCurrentPrcb()->DpcRoutineActive);
    }

    /* Restore the trap frame */
    Thread->TrapFrame = OldTrapFrame;
    return;
}

VOID
STDCALL
KiFreeApcRoutine(PKAPC Apc,
                 PKNORMAL_ROUTINE* NormalRoutine,
                 PVOID* NormalContext,
                 PVOID* SystemArgument1,
                 PVOID* SystemArgument2)
{
    /* Free the APC and do nothing else */
    ExFreePool(Apc);
}

/*++
 * KiInitializeUserApc
 *
 *     Prepares the Context for a User-Mode APC called through NTDLL.DLL
 *
 * Params:
 *     Reserved - Pointer to the Exception Frame on non-i386 builds.
 *
 *     TrapFrame - Pointer to the Trap Frame.
 *
 *     NormalRoutine - Pointer to the NormalRoutine to call.
 *
 *     NormalContext - Pointer to the context to send to the Normal Routine.
 *
 *     SystemArgument[1-2] - Pointer to a set of two parameters that contain
 *                           untyped data.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
STDCALL
KiInitializeUserApc(IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN PKNORMAL_ROUTINE NormalRoutine,
                    IN PVOID NormalContext,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)
{
    CONTEXT Context;
    ULONG_PTR Stack;
    ULONG Size;

    DPRINT("KiInitializeUserApc(TrapFrame %x/%x)\n", TrapFrame,
            KeGetCurrentThread()->TrapFrame);

    /* Don't deliver APCs in V86 mode */
    if (TrapFrame->EFlags & X86_EFLAGS_VM) return;

    /* Save the full context */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* Protect with SEH */
    _SEH_TRY
    {
        /* Get the aligned size */
        Size = ((sizeof(CONTEXT) + 3) & ~3) + 4 * sizeof(ULONG_PTR);
        Stack = (Context.Esp & ~3) - Size;

        /* Probe and copy */
        ProbeForWrite((PVOID)Stack, Size, 4);
        RtlMoveMemory((PVOID)(Stack + 4 * sizeof(ULONG_PTR)),
                      &Context,
                      sizeof(CONTEXT));

        /* Run at APC dispatcher */
        TrapFrame->Eip = (ULONG)KeUserApcDispatcher;
        TrapFrame->HardwareEsp = Stack;

        /* Setup the stack */
        *(PULONG_PTR)(Stack + 0 * sizeof(ULONG_PTR)) = (ULONG_PTR)NormalRoutine;
        *(PULONG_PTR)(Stack + 1 * sizeof(ULONG_PTR)) = (ULONG_PTR)NormalContext;
        *(PULONG_PTR)(Stack + 2 * sizeof(ULONG_PTR)) = (ULONG_PTR)SystemArgument1;
        *(PULONG_PTR)(Stack + 3 * sizeof(ULONG_PTR)) = (ULONG_PTR)SystemArgument2;
    }
    _SEH_HANDLE
    {
        /* FIXME: Get the record and raise an exception */
    }
    _SEH_END;
}

/*++
 * KeAreApcsDisabled
 * @implemented NT4
 *
 *     Prepares the Context for a User-Mode APC called through NTDLL.DLL
 *
 * Params:
 *     None.
 *
 * Returns:
 *     KeAreApcsDisabled returns TRUE if the thread is within a critical region
 *     or a guarded region, and FALSE otherwise.
 *
 * Remarks:
 *     A thread running at IRQL = PASSIVE_LEVEL can use KeAreApcsDisabled to
 *     determine if normal kernel APCs are disabled. A thread that is inside a
 *     critical region has both user APCs and normal kernel APCs disabled, but
 *     not special kernel APCs. A thread that is inside a guarded region has
 *     all APCs disabled, including special kernel APCs.
 *
 *     Callers of this routine must be running at IRQL <= APC_LEVEL.
 *
 *--*/
BOOLEAN
STDCALL
KeAreApcsDisabled(VOID)
{
    /* Return the Kernel APC State */
    return KeGetCurrentThread()->CombinedApcDisable ? TRUE : FALSE;
}

/*++
 * NtQueueApcThread
 * NT4
 *
 *    This routine is used to queue an APC from user-mode for the specified
 *    thread.
 *
 * Params:
 *     Thread Handle - Handle to the Thread. This handle must have THREAD_SET_CONTEXT privileges.
 *
 *     ApcRoutine - Pointer to the APC Routine to call when the APC executes.
 *
 *     NormalContext - Pointer to the context to send to the Normal Routine.
 *
 *     SystemArgument[1-2] - Pointer to a set of two parameters that contain
 *                           untyped data.
 *
 * Returns:
 *     STATUS_SUCCESS or failure cute from associated calls.
 *
 * Remarks:
 *      The thread must enter an alertable wait before the APC will be
 *      delivered.
 *
 *--*/
NTSTATUS
STDCALL
NtQueueApcThread(HANDLE ThreadHandle,
         PKNORMAL_ROUTINE ApcRoutine,
         PVOID NormalContext,
         PVOID SystemArgument1,
         PVOID SystemArgument2)
{
    PKAPC Apc;
    PETHREAD Thread;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    /* Get ETHREAD from Handle */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                       THREAD_SET_CONTEXT,
                       PsThreadType,
                       PreviousMode,
                       (PVOID)&Thread,
                       NULL);

    /* Fail if the Handle is invalid for some reason */
    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    /* If this is a Kernel or System Thread, then fail */
    if (Thread->Tcb.Teb == NULL) {

        ObDereferenceObject(Thread);
        return STATUS_INVALID_HANDLE;
    }

    /* Allocate an APC */
    Apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG('P', 's', 'a', 'p'));
    if (Apc == NULL) {

        ObDereferenceObject(Thread);
        return(STATUS_NO_MEMORY);
    }

    /* Initialize and Queue a user mode apc (always!) */
    KeInitializeApc(Apc,
                    &Thread->Tcb,
                    OriginalApcEnvironment,
                    KiFreeApcRoutine,
                    NULL,
                    ApcRoutine,
                    UserMode,
                    NormalContext);

    if (!KeInsertQueueApc(Apc, SystemArgument1, SystemArgument2, IO_NO_INCREMENT)) {

        Status = STATUS_UNSUCCESSFUL;

    } else {

        Status = STATUS_SUCCESS;
    }

    /* Dereference Thread and Return */
    ObDereferenceObject(Thread);
    return Status;
}

static __inline
VOID RepairList(PLIST_ENTRY Original,
                PLIST_ENTRY Copy,
                KPROCESSOR_MODE Mode)
{
    /* Copy Source to Desination */
    if (IsListEmpty(&Original[(int)Mode])) {

        InitializeListHead(&Copy[(int)Mode]);

    } else {

        Copy[(int)Mode].Flink = Original[(int)Mode].Flink;
        Copy[(int)Mode].Blink = Original[(int)Mode].Blink;
        Original[(int)Mode].Flink->Blink = &Copy[(int)Mode];
        Original[(int)Mode].Blink->Flink = &Copy[(int)Mode];
    }
}

VOID
STDCALL
KiMoveApcState(PKAPC_STATE OldState,
               PKAPC_STATE NewState)
{
    /* Restore backup of Original Environment */
    *NewState = *OldState;

    /* Repair Lists */
    RepairList(NewState->ApcListHead, OldState->ApcListHead, KernelMode);
    RepairList(NewState->ApcListHead, OldState->ApcListHead, UserMode);
}

