/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/apc.c
 * PURPOSE:         Implements the Asyncronous Procedure Call mechanism
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/*++
 * @name KiCheckForKernelApcDelivery
 * @implemented NT 5.2
 *
 *     The KiCheckForKernelApcDelivery routine is called whenever APCs have
 *     just been re-enabled in Kernel Mode, such as after leaving a Critical or
 *     Guarded Region. It delivers APCs if the environment is right.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks This routine allows KeLeave/EnterCritical/GuardedRegion to be used
 *          as macro from inside WIN32K or other Drivers, which will then only
 *          have do an Import API call in the case where APCs are enabled again.
 *
 *--*/
VOID
NTAPI
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
 * @name KiInsertQueueApc
 *
 *     The KiInsertQueueApc routine queues a APC for execution when the right
 *     scheduler environment exists.
 *
 * @param Apc
 *        Pointer to an initialized control object of type DPC for which the
 *        caller provides the storage.
 *
 * @param PriorityBoost
 *        Priority Boost to apply to the Thread.
 *
 * @return None
 *
 * @remarks The APC will execute at APC_LEVEL for the KernelRoutine registered,
 *          and at PASSIVE_LEVEL for the NormalRoutine registered.
 *
 *          Callers of this routine must have locked the dipatcher database.
 *
 *--*/
VOID
FASTCALL
KiInsertQueueApc(IN PKAPC Apc,
                 IN KPRIORITY PriorityBoost)
{
    PKTHREAD Thread = Apc->Thread;
    PKAPC_STATE ApcState;
    KPROCESSOR_MODE ApcMode;
    PLIST_ENTRY ListHead, NextEntry;
    PKAPC QueuedApc;
    NTSTATUS Status;
    BOOLEAN RequestInterrupt = FALSE;

    /*
     * Check if the caller wanted this APC to use the thread's environment at
     * insertion time.
     */
    if (Apc->ApcStateIndex == InsertApcEnvironment)
    {
        /* Copy it over */
        Apc->ApcStateIndex = Thread->ApcStateIndex;
    }

    /* Get the APC State for this Index, and the mode too */
    ApcState = Thread->ApcStatePointer[(UCHAR)Apc->ApcStateIndex];
    ApcMode = Apc->ApcMode;

    /* The APC must be "inserted" already */
    ASSERT(Apc->Inserted == TRUE);

    /* Three scenarios:
     * 1) Kernel APC with Normal Routine or User APC = Put it at the end of the List
     * 2) User APC which is PsExitSpecialApc = Put it at the front of the List
     * 3) Kernel APC without Normal Routine = Put it at the end of the No-Normal Routine Kernel APC list
     */
    if (Apc->NormalRoutine)
    {
        /* Normal APC; is it the Thread Termination APC? */
        if ((ApcMode != KernelMode) &&
            (Apc->KernelRoutine == PsExitSpecialApc))
        {
            /* Set User APC pending to true */
            Thread->ApcState.UserApcPending = TRUE;

            /* Insert it at the top of the list */
            InsertHeadList(&ApcState->ApcListHead[ApcMode],
                           &Apc->ApcListEntry);
        }
        else
        {
            /* Regular user or kernel Normal APC */
            InsertTailList(&ApcState->ApcListHead[ApcMode],
                           &Apc->ApcListEntry);
        }
    }
    else
    {
        /* Special APC, find the first Normal APC in the list */
        ListHead = &ApcState->ApcListHead[ApcMode];
        NextEntry = ListHead->Blink;
        while (NextEntry != ListHead)
        {
            /* Get the APC */
            QueuedApc = CONTAINING_RECORD(NextEntry, KAPC, ApcListEntry);

            /* Is this a Normal APC? If so, break */
            if (QueuedApc->NormalRoutine) break;

            /* Move to the next APC in the Queue */
            NextEntry = NextEntry->Blink;
        }

        /* Insert us here */
        InsertHeadList(NextEntry, &Apc->ApcListEntry);
    }

    /* Now check if the Apc State Indexes match */
    if (Thread->ApcStateIndex == Apc->ApcStateIndex)
    {
        /* Check that if the thread matches */
        if (Thread == KeGetCurrentThread())
        {
            /* Sanity check */
            ASSERT(Thread->State == Running);

            /* Check if this is kernel mode */
            if (ApcMode == KernelMode)
            {
                /* All valid, a Kernel APC is pending now */
                Thread->ApcState.KernelApcPending = TRUE;

                /* Check if Special APCs are disabled */
                if (!Thread->SpecialApcDisable)
                {
                    /* They're not, so request the interrupt */
                    HalRequestSoftwareInterrupt(APC_LEVEL);
                }
            }
        }
        else
        {
            /* Acquire the dispatcher lock */
            KiAcquireDispatcherLock();

            /* Check if this is a kernel-mode APC */
            if (ApcMode == KernelMode)
            {
                /* Kernel-mode APC, set us pending */
                Thread->ApcState.KernelApcPending = TRUE;

                /* Are we currently running? */
                if (Thread->State == Running)
                {
                    /* The thread is running, so remember to send a request */
                    RequestInterrupt = TRUE;
                }
                else if ((Thread->State == Waiting) &&
                         (Thread->WaitIrql == PASSIVE_LEVEL) &&
                         !(Thread->SpecialApcDisable) &&
                         (!(Apc->NormalRoutine) ||
                          (!(Thread->KernelApcDisable) &&
                           !(Thread->ApcState.KernelApcInProgress))))
                {
                    /* We'll unwait with this status */
                    Status = STATUS_KERNEL_APC;

                    /* Wake up the thread */
Unwait:
                    KiUnwaitThread(Thread, Status, PriorityBoost);
                }
                else if (Thread->State == GateWait)
                {
                    /* We were in a gate wait. FIXME: Handle this */
                    DPRINT1("Not yet supported -- Report this to Alex\n");
                    while (TRUE);
                }
            }
            else if ((Thread->State == Waiting) &&
                     (Thread->WaitMode == UserMode) &&
                     ((Thread->Alertable) ||
                      (Thread->ApcState.UserApcPending)))
            {
                /* Set user-mode APC pending */
                Thread->ApcState.UserApcPending = TRUE;
                Status = STATUS_USER_APC;
                goto Unwait;
            }

            /* Release dispatcher lock */
            KiReleaseDispatcherLockFromDpcLevel();

            /* Check if an interrupt was requested */
            KiRequestApcInterrupt(RequestInterrupt, Thread->NextProcessor);
        }
    }
}

/*++
 * @name KiDeliverApc
 * @implemented @NT4
 *
 *     The KiDeliverApc routine is called from IRQL switching code if the
 *     thread is returning from an IRQL >= APC_LEVEL and Kernel-Mode APCs are
 *     pending.
 *
 * @param DeliveryMode
 *        Specifies the current processor mode.
 *
 * @param ExceptionFrame
 *        Pointer to the Exception Frame on non-i386 builds.
 *
 * @param TrapFrame
 *        Pointer to the Trap Frame.
 *
 * @return None.
 *
 * @remarks First, Special APCs are delivered, followed by Kernel-Mode APCs and
 *          User-Mode APCs. Note that the TrapFrame is only valid if the
 *          delivery mode is User-Mode.
 *          Upon entry, this routine executes at APC_LEVEL.
 *
 *--*/
VOID
NTAPI
KiDeliverApc(IN KPROCESSOR_MODE DeliveryMode,
             IN PKEXCEPTION_FRAME ExceptionFrame,
             IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;
    PKTRAP_FRAME OldTrapFrame;
    PLIST_ENTRY ApcListEntry;
    PKAPC Apc;
    KLOCK_QUEUE_HANDLE ApcLock;
    PKKERNEL_ROUTINE KernelRoutine;
    PVOID NormalContext;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    ASSERT_IRQL_EQUAL(APC_LEVEL);

    /* Save the old trap frame and set current one */
    OldTrapFrame = Thread->TrapFrame;
    Thread->TrapFrame = TrapFrame;

    /* Clear Kernel APC Pending */
    Thread->ApcState.KernelApcPending = FALSE;

    /* Check if Special APCs are disabled */
    if (Thread->SpecialApcDisable) goto Quickie;

    /* Do the Kernel APCs first */
    while (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
    {
        /* Lock the APC Queue */
        KiAcquireApcLockAtApcLevel(Thread, &ApcLock);

        /* Check if the list became empty now */
        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
        {
            /* It is, release the lock and break out */
            KiReleaseApcLock(&ApcLock);
            break;
        }

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

            /* Rrelease the APC lock */
            KiReleaseApcLock(&ApcLock);

            /* Call the Special APC */
            KernelRoutine(Apc,
                          &NormalRoutine,
                          &NormalContext,
                          &SystemArgument1,
                          &SystemArgument2);

            /* Make sure it returned correctly */
            if (KeGetCurrentIrql() != ApcLock.OldIrql)
            {
                KeBugCheckEx(IRQL_UNEXPECTED_VALUE,
                             (KeGetCurrentIrql() << 16) |
                             (ApcLock.OldIrql << 8),
                             (ULONG_PTR)KernelRoutine,
                             (ULONG_PTR)Apc,
                             (ULONG_PTR)NormalRoutine);
            }
        }
        else
        {
            /* Normal Kernel APC, make sure it's safe to deliver */
            if ((Thread->ApcState.KernelApcInProgress) ||
                (Thread->KernelApcDisable))
            {
                /* Release lock and return */
                KiReleaseApcLock(&ApcLock);
                goto Quickie;
            }

            /* Dequeue the APC */
            RemoveEntryList(ApcListEntry);
            Apc->Inserted = FALSE;

            /* Go back to APC_LEVEL */
            KiReleaseApcLock(&ApcLock);

            /* Call the Kernel APC */
            KernelRoutine(Apc,
                          &NormalRoutine,
                          &NormalContext,
                          &SystemArgument1,
                          &SystemArgument2);

            /* Make sure it returned correctly */
            if (KeGetCurrentIrql() != ApcLock.OldIrql)
            {
                KeBugCheckEx(IRQL_UNEXPECTED_VALUE,
                             (KeGetCurrentIrql() << 16) |
                             (ApcLock.OldIrql << 8),
                             (ULONG_PTR)KernelRoutine,
                             (ULONG_PTR)Apc,
                             (ULONG_PTR)NormalRoutine);
            }

            /* Check if There still is a Normal Routine */
            if (NormalRoutine)
            {
                /* At Passive Level, an APC can be prempted by a Special APC */
                Thread->ApcState.KernelApcInProgress = TRUE;
                KeLowerIrql(PASSIVE_LEVEL);

                /* Call and Raise IRQ back to APC_LEVEL */
                NormalRoutine(NormalContext, SystemArgument1, SystemArgument2);
                KeRaiseIrql(APC_LEVEL, &ApcLock.OldIrql);
            }

            /* Set Kernel APC in progress to false and loop again */
            Thread->ApcState.KernelApcInProgress = FALSE;
        }
    }

    /* Now we do the User APCs */
    if ((DeliveryMode == UserMode) &&
        !(IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])) &&
         (Thread->ApcState.UserApcPending))
    {
        /* Lock the APC Queue */
        KiAcquireApcLockAtApcLevel(Thread, &ApcLock);

        /* It's not pending anymore */
        Thread->ApcState.UserApcPending = FALSE;

        /* Check if the list became empty now */
        if (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]))
        {
            /* It is, release the lock and break out */
            KiReleaseApcLock(&ApcLock);
            goto Quickie;
        }

        /* Get the actual APC object */
        ApcListEntry = Thread->ApcState.ApcListHead[UserMode].Flink;
        Apc = CONTAINING_RECORD(ApcListEntry, KAPC, ApcListEntry);

        /* Save Parameters so that it's safe to free the Object in Kernel Routine*/
        NormalRoutine = Apc->NormalRoutine;
        KernelRoutine = Apc->KernelRoutine;
        NormalContext = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;

        /* Remove the APC from Queue, and release the lock */
        RemoveEntryList(ApcListEntry);
        Apc->Inserted = FALSE;
        KiReleaseApcLock(&ApcLock);

        /* Call the kernel routine */
        KernelRoutine(Apc,
                      &NormalRoutine,
                      &NormalContext,
                      &SystemArgument1,
                      &SystemArgument2);

        /* Check if there's no normal routine */
        if (!NormalRoutine)
        {
            /* Check if more User APCs are Pending */
            KeTestAlertThread(UserMode);
        }
        else
        {
            /* Set up the Trap Frame and prepare for Execution in NTDLL.DLL */
            KiInitializeUserApc(ExceptionFrame,
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
}

FORCEINLINE
VOID
RepairList(IN PLIST_ENTRY Original,
           IN PLIST_ENTRY Copy,
           IN KPROCESSOR_MODE Mode)
{
    /* Check if the list for this mode is empty */
    if (IsListEmpty(&Original[Mode]))
    {
        /* It is, all we need to do is initialize it */
        InitializeListHead(&Copy[Mode]);
    }
    else
    {
        /* Copy the lists */
        Copy[Mode].Flink = Original[Mode].Flink;
        Copy[Mode].Blink = Original[Mode].Blink;
        Original[Mode].Flink->Blink = &Copy[Mode];
        Original[Mode].Blink->Flink = &Copy[Mode];
    }
}

VOID
NTAPI
KiMoveApcState(PKAPC_STATE OldState,
               PKAPC_STATE NewState)
{
    /* Restore backup of Original Environment */
    *NewState = *OldState;

    /* Repair Lists */
    RepairList(NewState->ApcListHead, OldState->ApcListHead, KernelMode);
    RepairList(NewState->ApcListHead, OldState->ApcListHead, UserMode);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name KeEnterCriticalRegion
 * @implemented NT4
 *
 *     The KeEnterCriticalRegion routine temporarily disables the delivery of
 *     normal kernel APCs; special kernel-mode APCs are still delivered.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Highest-level drivers can call this routine while running in the
 *          context of the thread that requested the current I/O operation.
 *          Any caller of this routine should call KeLeaveCriticalRegion as
 *          quickly as possible.
 *
 *          Callers of KeEnterCriticalRegion must be running at IRQL <=
 *          APC_LEVEL.
 *
 *--*/
VOID
NTAPI
_KeEnterCriticalRegion(VOID)
{
    /* Use inlined function */
    KeEnterCriticalRegion();
}

/*++
 * KeLeaveCriticalRegion
 * @implemented NT4
 *
 *     The KeLeaveCriticalRegion routine reenables the delivery of normal
 *     kernel-mode APCs that were disabled by a call to KeEnterCriticalRegion.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks Highest-level drivers can call this routine while running in the
 *          context of the thread that requested the current I/O operation.
 *
 *          Callers of KeLeaveCriticalRegion must be running at IRQL <=
 *          DISPATCH_LEVEL.
 *
 *--*/
VOID
NTAPI
_KeLeaveCriticalRegion(VOID)
{
    /* Use inlined version */
    KeLeaveCriticalRegion();
}

/*++
 * KeInitializeApc
 * @implemented NT4
 *
 *     The The KeInitializeApc routine initializes an APC object, and registers
 *     the Kernel, Rundown and Normal routines for that object.
 *
 * @param Apc
 *        Pointer to a KAPC structure that represents the APC object to
 *        initialize. The caller must allocate storage for the structure
 *        from resident memory.
 *
 * @param Thread
 *        Thread to which to deliver the APC.
 *
 * @param TargetEnvironment
 *        APC Environment to be used.
 *
 * @param KernelRoutine
 *        Points to the KernelRoutine to associate with the APC.
 *        This routine is executed for all APCs.
 *
 * @param RundownRoutine
 *        Points to the RundownRoutine to associate with the APC.
 *        This routine is executed when the Thread exists during APC execution.
 *
 * @param NormalRoutine
 *        Points to the NormalRoutine to associate with the APC.
 *        This routine is executed at PASSIVE_LEVEL. If this is not specifed,
 *        the APC becomes a Special APC and the Mode and Context parameters are
 *        ignored.
 *
 * @param Mode
 *        Specifies the processor mode at which to run the Normal Routine.
 *
 * @param Context
 *        Specifices the value to pass as Context parameter to the registered
 *        routines.
 *
 * @return None.
 *
 * @remarks The caller can queue an initialized APC with KeInsertQueueApc.
 *
 *--*/
VOID
NTAPI
KeInitializeApc(IN PKAPC Apc,
                IN PKTHREAD Thread,
                IN KAPC_ENVIRONMENT TargetEnvironment,
                IN PKKERNEL_ROUTINE KernelRoutine,
                IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
                IN PKNORMAL_ROUTINE NormalRoutine,
                IN KPROCESSOR_MODE Mode,
                IN PVOID Context)
{
    /* Sanity check */
    ASSERT(TargetEnvironment <= InsertApcEnvironment);

    /* Set up the basic APC Structure Data */
    Apc->Type = ApcObject;
    Apc->Size = sizeof(KAPC);

    /* Set the Environment */
    if (TargetEnvironment == CurrentApcEnvironment)
    {
        /* Use the current one for the thread */
        Apc->ApcStateIndex = Thread->ApcStateIndex;
    }
    else
    {
        /* Sanity check */
        ASSERT((TargetEnvironment <= Thread->ApcStateIndex) ||
               (TargetEnvironment == InsertApcEnvironment));

        /* Use the one that was given */
        Apc->ApcStateIndex = TargetEnvironment;
    }

    /* Set the Thread and Routines */
    Apc->Thread = Thread;
    Apc->KernelRoutine = KernelRoutine;
    Apc->RundownRoutine = RundownRoutine;
    Apc->NormalRoutine = NormalRoutine;

    /* Check if this is a special APC */
    if (NormalRoutine)
    {
        /* It's a normal one. Set the context and mode */
        Apc->ApcMode = Mode;
        Apc->NormalContext = Context;
    }
    else
    {
        /* It's a special APC, which can only be kernel mode */
        Apc->ApcMode = KernelMode;
        Apc->NormalContext = NULL;
    }

    /* The APC is not inserted*/
    Apc->Inserted = FALSE;
}

/*++
 * @name KeInsertQueueApc
 * @implemented NT4
 *
 *     The KeInsertQueueApc routine queues a APC for execution when the right
 *     scheduler environment exists.
 *
 * @param Apc
 *        Pointer to an initialized control object of type DPC for which the
 *        caller provides the storage.
 *
 * @param SystemArgument[1,2]
 *        Pointer to a set of two parameters that contain untyped data.
 *
 * @param PriorityBoost
 *        Priority Boost to apply to the Thread.
 *
 * @return If the APC is already inserted or APC queueing is disabled, FALSE.
 *         Otherwise, TRUE.
 *
 * @remarks The APC will execute at APC_LEVEL for the KernelRoutine registered,
 *          and at PASSIVE_LEVEL for the NormalRoutine registered.
 *
 *          Callers of this routine must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
KeInsertQueueApc(IN PKAPC Apc,
                 IN PVOID SystemArgument1,
                 IN PVOID SystemArgument2,
                 IN KPRIORITY PriorityBoost)
{
    PKTHREAD Thread = Apc->Thread;
    KLOCK_QUEUE_HANDLE ApcLock;
    BOOLEAN State = TRUE;
    ASSERT_APC(Apc);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Get the APC lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Make sure we can Queue APCs and that this one isn't already inserted */
    if (!(Thread->ApcQueueable) && (Apc->Inserted))
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
    KiReleaseApcLockFromDpcLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return State;
}

/*++
 * @name KeFlushQueueApc
 * @implemented NT4
 *
 *     The KeFlushQueueApc routine flushes all APCs of the given processor mode
 *     from the specified Thread's APC queue.
 *
 * @param Thread
 *        Pointer to the thread whose APC queue will be flushed.
 *
 * @paramt PreviousMode
 *         Specifies which APC Queue to flush.
 *
 * @return A pointer to the first entry in the flushed APC queue.
 *
 * @remarks If the routine returns NULL, it means that no APCs were flushed.
 *          Callers of this routine must be running at DISPATCH_LEVEL or lower.
 *
 *--*/
PLIST_ENTRY
NTAPI
KeFlushQueueApc(IN PKTHREAD Thread,
                IN KPROCESSOR_MODE PreviousMode)
{
    PKAPC Apc;
    PLIST_ENTRY FirstEntry, CurrentEntry;
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Check if this was user mode */
    if (PreviousMode == UserMode)
    {
        /* Get the APC lock */
        KiAcquireApcLock(Thread, &ApcLock);

        /* Select user list and check if it's empty */
        if (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]))
        {
            /* Don't return anything */
            FirstEntry = NULL;
            goto FlushDone;
        }
    }
    else
    {
        /* Select kernel list and check if it's empty */
        if (IsListEmpty( &Thread->ApcState.ApcListHead[KernelMode]))
        {
            /* Don't return anything */
            return NULL;
        }

        /* Otherwise, acquire the APC lock */
        KiAcquireApcLock(Thread, &ApcLock);
    }

    /* Get the first entry and check if the list is empty now */
    FirstEntry = Thread->ApcState.ApcListHead[PreviousMode].Flink;
    if (FirstEntry == &Thread->ApcState.ApcListHead[PreviousMode])
    {
        /* It is, clear the returned entry */
        FirstEntry = NULL;
    }
    else
    {
        /* It's not, remove the first entry */
        RemoveEntryList(&Thread->ApcState.ApcListHead[PreviousMode]);

        /* Loop all the entries */
        CurrentEntry = FirstEntry;
        do
        {
            /* Get the APC and make it un-inserted */
            Apc = CONTAINING_RECORD(CurrentEntry, KAPC, ApcListEntry);
            Apc->Inserted = FALSE;

            /* Get the next entry */
            CurrentEntry = CurrentEntry->Flink;
        } while (CurrentEntry != FirstEntry);

        /* Re-initialize the list */
        InitializeListHead(&Thread->ApcState.ApcListHead[PreviousMode]);
    }

    /* Release the lock */
FlushDone:
    KiReleaseApcLock(&ApcLock);

    /* Return the first entry */
    return FirstEntry;
}

/*++
 * @name KeRemoveQueueApc
 * @implemented NT4
 *
 *     The KeRemoveQueueApc routine removes a given APC object from the system
 *     APC queue.
 *
 * @params Apc
 *         Pointer to an initialized APC object that was queued by calling
 *         KeInsertQueueApc.
 *
 * @return TRUE if the APC Object is in the APC Queue. Otherwise, no operation
 *         is performed and FALSE is returned.
 *
 * @remarks If the given APC Object is currently queued, it is removed from the
 *          queue and any calls to the registered routines are cancelled.
 *
 *          Callers of this routine must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
KeRemoveQueueApc(IN PKAPC Apc)
{
    PKTHREAD Thread = Apc->Thread;
    PKAPC_STATE ApcState;
    BOOLEAN Inserted;
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_APC(Apc);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Get the APC lock */
    KiAcquireApcLock(Thread, &ApcLock);

    /* Check if it's inserted */
    Inserted = Apc->Inserted;
    if (Inserted)
    {
        /* Set it as non-inserted and get the APC state */
        Apc->Inserted = FALSE;
        ApcState = Thread->ApcStatePointer[(UCHAR)Apc->ApcStateIndex];

        /* Acquire the dispatcher lock and remove it from the list */
        KiAcquireDispatcherLockAtDpcLevel();
        if (RemoveEntryList(&ApcState->ApcListHead[Apc->ApcMode]))
        {
            /* Set the correct state based on the APC Mode */
            if (Apc->ApcMode == KernelMode)
            {
                /* No more pending kernel APCs */
                ApcState->KernelApcPending = FALSE;
            }
            else
            {
                /* No more pending user APCs */
                ApcState->UserApcPending = FALSE;
            }
        }

        /* Release dispatcher lock */
        KiReleaseDispatcherLockFromDpcLevel();
    }

    /* Release the lock and return */
    KiReleaseApcLock(&ApcLock);
    return Inserted;
}

/*++
 * @name KeAreApcsDisabled
 * @implemented NT4
 *
 *     The KeAreApcsDisabled routine returns whether kernel APC delivery is
 *     disabled for the current thread.
 *
 * @param None.
 *
 * @return KeAreApcsDisabled returns TRUE if the thread is within a critical
 *         region or a guarded region, and FALSE otherwise.
 *
 * @remarks A thread running at IRQL = PASSIVE_LEVEL can use KeAreApcsDisabled
 *          determine if normal kernel APCs are disabled.
 *
 *          A thread that is inside critical region has both user APCs and
 *          normal kernel APCs disabled, but not special kernel APCs.
 *
 *          A thread that is inside a guarded region has all APCs disabled,
 *          including special kernel APCs.
 *
 *          Callers of this routine must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
KeAreApcsDisabled(VOID)
{
    /* Return the Kernel APC State */
    return KeGetCurrentThread()->CombinedApcDisable ? TRUE : FALSE;
}

/*++
 * @name KeAreAllApcsDisabled
 * @implemented NT5.1
 *
 *    The KeAreAllApcsDisabled routine returns whether the calling thread is
 *    inside a guarded region or running at IRQL = APC_LEVEL, which disables
 *    all APC delivery.
 *
 * @param None.
 *
 * @return KeAreAllApcsDisabled returns TRUE if the thread is within a guarded
 *         guarded region or running at IRQL >= APC_LEVEL, and FALSE otherwise.
 *
 * @remarks A thread running at IRQL = PASSIVE_LEVEL can use this routine to
 *          determine if all APCs delivery is disabled.
 *
 *          Callers of this routine must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
BOOLEAN
NTAPI
KeAreAllApcsDisabled(VOID)
{
    /* Return the Special APC State */
    return ((KeGetCurrentThread()->SpecialApcDisable) ||
            (KeGetCurrentIrql() >= APC_LEVEL)) ? TRUE : FALSE;
}




