/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/wait.c
 * PURPOSE:         Manages waiting for Dispatcher Objects
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Gunnar Dalsnes
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

#if DBG && defined(_M_IX86)
static
BOOLEAN
NTAPI
KiIsThreadTrapFrameReadable(
    _In_opt_ PKTHREAD Thread,
    _In_opt_ PKTRAP_FRAME TrapFrame)
{
    ULONG_PTR Frame;
    ULONG_PTR InitialStack;

    if ((Thread == NULL) || (TrapFrame == NULL))
        return FALSE;

    Frame = (ULONG_PTR)TrapFrame;
    InitialStack = (ULONG_PTR)Thread->InitialStack;

    if (InitialStack < sizeof(KTRAP_FRAME))
        return FALSE;

    return (((ULONG_PTR)Thread->StackLimit <= Frame) &&
            (Frame <= (InitialStack - sizeof(KTRAP_FRAME))));
}

static
VOID
NTAPI
KiTraceDispatcherTrapFrame(
    _In_ ULONG Event,
    _In_ PKPRCB Prcb,
    _In_ KIRQL OldIrql)
{
    PKTHREAD Thread;
    PKTRAP_FRAME TrapFrame;
    ULONG_PTR TrapEip;
    ULONG_PTR TrapSegCs;
    ULONG_PTR TrapEFlags;
    ULONG_PTR TrapStack;
    ULONG_PTR LinkedTrapFrame;

    Thread = Prcb->CurrentThread;
    TrapFrame = Thread != NULL ? Thread->TrapFrame : NULL;
    TrapEip = 0;
    TrapSegCs = 0;
    TrapEFlags = 0;
    TrapStack = 0;
    LinkedTrapFrame = 0;

    if (KiIsThreadTrapFrameReadable(Thread, TrapFrame))
    {
        TrapEip = TrapFrame->Eip;
        TrapSegCs = TrapFrame->SegCs;
        TrapEFlags = TrapFrame->EFlags;
        TrapStack = TrapFrame->PreviousPreviousMode == KernelMode ?
                    TrapFrame->TempEsp :
                    TrapFrame->HardwareEsp;
        LinkedTrapFrame = TrapFrame->Edx;
    }

    KiI386BootTraceRecord(Event,
                          OldIrql,
                          (ULONG_PTR)Thread,
                          (ULONG_PTR)TrapFrame,
                          TrapEip,
                          TrapSegCs,
                          TrapEFlags);
    KiI386BootTraceRecord(Event + 1,
                          (ULONG_PTR)Thread,
                          Thread != NULL ? (ULONG_PTR)Thread->KernelStack : 0,
                          Thread != NULL ? (ULONG_PTR)Thread->InitialStack : 0,
                          Thread != NULL ? (ULONG_PTR)Thread->StackLimit : 0,
                          LinkedTrapFrame,
                          TrapStack);
}

static
VOID
NTAPI
KiTraceDispatcherSwitchOwnership(
    _In_ ULONG Event,
    _In_ PKPRCB Prcb,
    _In_ PKTHREAD Thread,
    _In_ PKTHREAD NextThread,
    _In_ KIRQL OldIrql)
{
    KiI386BootTraceRecord(Event,
                          OldIrql,
                          (ULONG_PTR)Prcb,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          (ULONG_PTR)Thread,
                          (ULONG_PTR)NextThread);
    KiI386BootTraceRecord(Event + 1,
                          (ULONG_PTR)Thread,
                          Thread != NULL ? (ULONG_PTR)Thread->KernelStack : 0,
                          Thread != NULL ? (ULONG_PTR)Thread->InitialStack : 0,
                          Thread != NULL ? (ULONG_PTR)Thread->StackLimit : 0,
                          Thread != NULL ? (ULONG_PTR)Thread->TrapFrame : 0,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
    KiI386BootTraceRecord(Event + 2,
                          (ULONG_PTR)NextThread,
                          NextThread != NULL ? (ULONG_PTR)NextThread->KernelStack : 0,
                          NextThread != NULL ? (ULONG_PTR)NextThread->InitialStack : 0,
                          NextThread != NULL ? (ULONG_PTR)NextThread->StackLimit : 0,
                          NextThread != NULL ? (ULONG_PTR)NextThread->TrapFrame : 0,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
}
#endif

VOID
FASTCALL
KiWaitTest(IN PVOID ObjectPointer,
           IN KPRIORITY Increment)
{
    PLIST_ENTRY WaitEntry, WaitList;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD WaitThread;
    PKMUTANT FirstObject = ObjectPointer;
    NTSTATUS WaitStatus;

    /* Loop the Wait Entries */
    WaitList = &FirstObject->Header.WaitListHead;
    WaitEntry = WaitList->Flink;
    while ((FirstObject->Header.SignalState > 0) && (WaitEntry != WaitList))
    {
        /* Get the current wait block */
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        WaitThread = WaitBlock->Thread;
        WaitStatus = STATUS_KERNEL_APC;

        /* Check the current Wait Mode */
        if (WaitBlock->WaitType == WaitAny)
        {
            /* Easy case, satisfy only this wait */
            WaitStatus = (NTSTATUS)WaitBlock->WaitKey;
            KiSatisfyObjectWait(FirstObject, WaitThread);
        }

        /* Now do the rest of the unwait */
        KiUnwaitThread(WaitThread, WaitStatus, Increment);
        WaitEntry = WaitList->Flink;
    }
}

VOID
FASTCALL
KiUnlinkThread(IN PKTHREAD Thread,
               IN LONG_PTR WaitStatus)
{
    PKWAIT_BLOCK WaitBlock;
    PKTIMER Timer;

    /* Update wait status */
    Thread->WaitStatus |= WaitStatus;

    /* Remove the Wait Blocks from the list */
    WaitBlock = Thread->WaitBlockList;
    do
    {
        /* Remove it */
        RemoveEntryList(&WaitBlock->WaitListEntry);

        /* Go to the next one */
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != Thread->WaitBlockList);

    /* Remove the thread from the wait list! */
    if (Thread->WaitListEntry.Flink) RemoveEntryList(&Thread->WaitListEntry);

    /* Check if there's a Thread Timer */
    Timer = &Thread->Timer;
    if (Timer->Header.Inserted) KxRemoveTreeTimer(Timer);

    /* Increment the Queue's active threads */
    if (Thread->Queue) Thread->Queue->CurrentCount++;
}

/* Must be called with the dispatcher lock held */
VOID
FASTCALL
KiUnwaitThread(IN PKTHREAD Thread,
               IN LONG_PTR WaitStatus,
               IN KPRIORITY Increment)
{
    /* Unlink the thread */
    KiUnlinkThread(Thread, WaitStatus);

    /* Tell the scheduler do to the increment when it readies the thread */
    ASSERT(Increment >= 0);
    Thread->AdjustIncrement = (SCHAR)Increment;
    Thread->AdjustReason = AdjustUnwait;

    /* Reschedule the Thread */
    KiReadyThread(Thread);
}

VOID
FASTCALL
KiAcquireFastMutex(IN PFAST_MUTEX FastMutex)
{
    /* Increase contention count */
    FastMutex->Contention++;

    /* Wait for the event */
    KeWaitForSingleObject(&FastMutex->Event,
                          WrMutex,
                          KernelMode,
                          FALSE,
                          NULL);
}

VOID
FASTCALL
KiAcquireGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    ULONG BitsToRemove, BitsToAdd;
    LONG OldValue, NewValue;

    /* We depend on these bits being just right */
    C_ASSERT((GM_LOCK_WAITER_WOKEN * 2) == GM_LOCK_WAITER_INC);

    /* Increase the contention count */
    GuardedMutex->Contention++;

    /* Start by unlocking the Guarded Mutex */
    BitsToRemove = GM_LOCK_BIT;
    BitsToAdd = GM_LOCK_WAITER_INC;

    /* Start change loop */
    for (;;)
    {
        /* Loop sanity checks */
        ASSERT((BitsToRemove == GM_LOCK_BIT) ||
               (BitsToRemove == (GM_LOCK_BIT | GM_LOCK_WAITER_WOKEN)));
        ASSERT((BitsToAdd == GM_LOCK_WAITER_INC) ||
               (BitsToAdd == GM_LOCK_WAITER_WOKEN));

        /* Get the Count Bits */
        OldValue = GuardedMutex->Count;

        /* Start internal bit change loop */
        for (;;)
        {
            /* Check if the Guarded Mutex is locked */
            if (OldValue & GM_LOCK_BIT)
            {
                /* Sanity check */
                ASSERT((BitsToRemove == GM_LOCK_BIT) ||
                       ((OldValue & GM_LOCK_WAITER_WOKEN) != 0));

                /* Unlock it by removing the Lock Bit */
                NewValue = OldValue ^ BitsToRemove;
                NewValue = InterlockedCompareExchange(&GuardedMutex->Count,
                                                      NewValue,
                                                      OldValue);
                if (NewValue == OldValue) return;
            }
            else
            {
                /* The Guarded Mutex isn't locked, so simply set the bits */
                NewValue = OldValue + BitsToAdd;
                NewValue = InterlockedCompareExchange(&GuardedMutex->Count,
                                                      NewValue,
                                                      OldValue);
                if (NewValue == OldValue) break;
            }

            /* Old value changed, loop again */
            OldValue = NewValue;
        }

        /* Now we have to wait for it */
        KeWaitForGate(&GuardedMutex->Gate, WrGuardedMutex, KernelMode);
        ASSERT((GuardedMutex->Count & GM_LOCK_WAITER_WOKEN) != 0);

        /* Ok, the wait is done, so set the new bits */
        BitsToRemove = GM_LOCK_BIT | GM_LOCK_WAITER_WOKEN;
        BitsToAdd = GM_LOCK_WAITER_WOKEN;
    }
}

//
// This routine exits the dispatcher after a compatible operation and
// swaps the context to the next scheduled thread on the current CPU if
// one is available.
//
// It does NOT attempt to scan for a new thread to schedule.
//
VOID
FASTCALL
KiExitDispatcher(IN KIRQL OldIrql)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD Thread, NextThread;
    BOOLEAN PendingApc;

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE240,
                          OldIrql,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          Prcb->DpcRoutineActive,
                          (ULONG_PTR)Prcb,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
#endif

    /* Make sure we're at synchronization level */
    ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE246,
                          OldIrql,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          Prcb->DpcRoutineActive,
                          (ULONG_PTR)Prcb,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
    KiI386BootTraceRecord(0xE247,
                          OldIrql,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          (ULONG_PTR)Prcb->DeferredReadyListHead.Next,
                          Prcb->ReadySummary,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
#endif

    /* Check if we have deferred threads */
    KiCheckDeferredReadyList(Prcb);

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE248,
                          OldIrql,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          (ULONG_PTR)Prcb->DeferredReadyListHead.Next,
                          Prcb->ReadySummary,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
    KiTraceDispatcherTrapFrame(0xE249, Prcb, OldIrql);
#endif

    /* Check if we were called at dispatcher level or higher */
    if (OldIrql >= DISPATCH_LEVEL)
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE241,
                              OldIrql,
                              (ULONG_PTR)Prcb->CurrentThread,
                              (ULONG_PTR)Prcb->NextThread,
                              Prcb->DpcRoutineActive,
                              Prcb->ReadySummary,
                              (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
#endif

        /* Check if we have a thread to schedule, and that no DPC is active */
        if ((Prcb->NextThread) && !(Prcb->DpcRoutineActive))
        {
            /* Request DPC interrupt */
            HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
        }

        /* Lower IRQL and exit */
        goto Quickie;
    }

    /* Make sure there's a new thread scheduled */
    if (!Prcb->NextThread)
    {
#if DBG && defined(_M_IX86)
        KiI386BootTraceRecord(0xE242,
                              OldIrql,
                              (ULONG_PTR)Prcb->CurrentThread,
                              0,
                              Prcb->DpcRoutineActive,
                              Prcb->ReadySummary,
                              (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
#endif
        goto Quickie;
    }

    /* Lock the PRCB */
    KiAcquirePrcbLock(Prcb);

    /* Get the next and current threads now */
    NextThread = Prcb->NextThread;
    Thread = Prcb->CurrentThread;

#if DBG && defined(_M_IX86)
    KiTraceDispatcherSwitchOwnership(0xE24B,
                                     Prcb,
                                     Thread,
                                     NextThread,
                                     OldIrql);
#endif

    /* Set current thread's swap busy to true */
    KiSetThreadSwapBusy(Thread);

    /* Switch threads in PRCB */
    Prcb->NextThread = NULL;
    Prcb->CurrentThread = NextThread;

#if DBG && defined(_M_IX86)
    KiTraceDispatcherSwitchOwnership(0xE24E,
                                     Prcb,
                                     Thread,
                                     NextThread,
                                     OldIrql);
#endif

    /* Set thread to running */
    NextThread->State = Running;

    /* Queue it on the ready lists */
    KxQueueReadyThread(Thread, Prcb);

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE251,
                          OldIrql,
                          (ULONG_PTR)Thread,
                          Thread->State,
                          Thread->Affinity,
                          Thread->NextProcessor,
                          (ULONG_PTR)Prcb->CurrentThread);
#endif

    /* Set wait IRQL */
    Thread->WaitIrql = OldIrql;

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE243,
                          OldIrql,
                          (ULONG_PTR)Thread,
                          (ULONG_PTR)NextThread,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
#endif

    /* Swap threads and check if APCs were pending */
    PendingApc = KiSwapContext(OldIrql, Thread);

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE244,
                          OldIrql,
                          (ULONG_PTR)Thread,
                          (ULONG_PTR)NextThread,
                          PendingApc,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
#endif

    if (PendingApc)
    {
        /* Lower only to APC */
        KeLowerIrql(APC_LEVEL);

        /* Deliver APCs */
        KiDeliverApc(KernelMode, NULL, NULL);
        ASSERT(OldIrql == PASSIVE_LEVEL);
    }

    /* Lower IRQl back */
Quickie:
#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xE245,
                          OldIrql,
                          (ULONG_PTR)Prcb->CurrentThread,
                          (ULONG_PTR)Prcb->NextThread,
                          Prcb->DpcRoutineActive,
                          KeGetCurrentIrql(),
                          (ULONG_PTR)KeGetPcr()->NtTib.ExceptionList);
#endif
    KeLowerIrql(OldIrql);
}

/* PUBLIC FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
KeIsWaitListEmpty(IN PVOID Object)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeDelayExecutionThread(IN KPROCESSOR_MODE WaitMode,
                       IN BOOLEAN Alertable,
                       IN PLARGE_INTEGER Interval OPTIONAL)
{
    PKTIMER Timer;
    PKWAIT_BLOCK TimerBlock;
    PKTHREAD Thread = KeGetCurrentThread();
    NTSTATUS WaitStatus;
    BOOLEAN Swappable;
    PLARGE_INTEGER OriginalDueTime;
    LARGE_INTEGER DueTime, NewDueTime, InterruptTime;
    ULONG Hand = 0;

    if (Thread->WaitNext)
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    else
        ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* If this is a user-mode wait of 0 seconds, yield execution */
    if (!(Interval->QuadPart) && (WaitMode != KernelMode))
    {
        /* Make sure the wait isn't alertable or interrupting an APC */
        if (!(Alertable) && !(Thread->ApcState.UserApcPending))
        {
            /* Yield execution */
            return NtYieldExecution();
        }
    }

    /* Setup the original time and timer/wait blocks */
    OriginalDueTime = Interval;
    Timer = &Thread->Timer;
    TimerBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];

    /* Check if the lock is already held */
    if (!Thread->WaitNext) goto WaitStart;

    /*  Otherwise, we already have the lock, so initialize the wait */
    Thread->WaitNext = FALSE;
    KxDelayThreadWait();

    /* Start wait loop */
    for (;;)
    {
        /* Disable pre-emption */
        Thread->Preempted = FALSE;

        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((Thread->ApcState.KernelApcPending) && !(Thread->SpecialApcDisable) &&
            (Thread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KiReleaseDispatcherLock(Thread->WaitIrql);
        }
        else
        {
            /* Check if we have to bail out due to an alerted state */
            WaitStatus = KiCheckAlertability(Thread, Alertable, WaitMode);
            if (WaitStatus != STATUS_WAIT_0) break;

            /* Check if the timer expired */
            InterruptTime.QuadPart = KeQueryInterruptTime();
            if ((ULONGLONG)InterruptTime.QuadPart >= Timer->DueTime.QuadPart)
            {
                /* It did, so we don't need to wait */
                goto NoWait;
            }

            /* It didn't, so activate it */
            Timer->Header.Inserted = TRUE;

            /* Handle Kernel Queues */
            if (Thread->Queue) KiActivateWaiterQueue(Thread->Queue);

            /* Setup the wait information */
            Thread->State = Waiting;

            /* Add the thread to the wait list */
            KiAddThreadToWaitList(Thread, Swappable);

            /* Insert the timer and swap the thread */
            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
            KiSetThreadSwapBusy(Thread);
            KxInsertTimer(Timer, Hand);
            WaitStatus = (NTSTATUS)KiSwapThread(Thread, KeGetCurrentPrcb());

            /* Check if were swapped ok */
            if (WaitStatus != STATUS_KERNEL_APC)
            {
                /* This is a good thing */
                if (WaitStatus == STATUS_TIMEOUT) WaitStatus = STATUS_SUCCESS;

                /* Return Status */
                return WaitStatus;
            }

            /* Recalculate due times */
            Interval = KiRecalculateDueTime(OriginalDueTime,
                                            &DueTime,
                                            &NewDueTime);
        }

WaitStart:
        /* Setup a new wait */
        Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
        KxDelayThreadWait();
        KiAcquireDispatcherLockAtSynchLevel();
    }

    /* We're done! */
    KiReleaseDispatcherLock(Thread->WaitIrql);
    return WaitStatus;

NoWait:
    /* There was nothing to wait for. Did we have a wait interval? */
    if (!Interval->QuadPart)
    {
        /* Unlock the dispatcher and do a yield */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        return NtYieldExecution();
    }

    /* Unlock the dispatcher and adjust the quantum for a no-wait */
    KiReleaseDispatcherLockFromSynchLevel();
    KiAdjustQuantumThread(Thread);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeWaitForSingleObject(IN PVOID Object,
                      IN KWAIT_REASON WaitReason,
                      IN KPROCESSOR_MODE WaitMode,
                      IN BOOLEAN Alertable,
                      IN PLARGE_INTEGER Timeout OPTIONAL)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PKMUTANT CurrentObject = (PKMUTANT)Object;
    PKWAIT_BLOCK WaitBlock = &Thread->WaitBlock[0];
    PKWAIT_BLOCK TimerBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    PKTIMER Timer = &Thread->Timer;
    NTSTATUS WaitStatus;
    BOOLEAN Swappable;
    LARGE_INTEGER DueTime = {{0}}, NewDueTime, InterruptTime;
    PLARGE_INTEGER OriginalDueTime = Timeout;
    ULONG Hand = 0;

    if (Thread->WaitNext)
        ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);
    else
        ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL ||
               (KeGetCurrentIrql() == DISPATCH_LEVEL &&
                Timeout && Timeout->QuadPart == 0));

    /* Check if the lock is already held */
    if (!Thread->WaitNext) goto WaitStart;

    /*  Otherwise, we already have the lock, so initialize the wait */
    Thread->WaitNext = FALSE;
    KxSingleThreadWait();

    /* Start wait loop */
    for (;;)
    {
        /* Disable pre-emption */
        Thread->Preempted = FALSE;

        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((Thread->ApcState.KernelApcPending) && !(Thread->SpecialApcDisable) &&
            (Thread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KiReleaseDispatcherLock(Thread->WaitIrql);
        }
        else
        {
            /* Sanity check */
            ASSERT(CurrentObject->Header.Type != QueueObject);

            /* Check if it's a mutant */
            if (CurrentObject->Header.Type == MutantObject)
            {
                /* Check its signal state or if we own it */
                if ((CurrentObject->Header.SignalState > 0) ||
                    (Thread == CurrentObject->OwnerThread))
                {
                    /* Just unwait this guy and exit */
                    if (CurrentObject->Header.SignalState != MINLONG)
                    {
                        /* It has a normal signal state. Unwait and return */
                        KiSatisfyMutantWait(CurrentObject, Thread);
                        WaitStatus = (NTSTATUS)Thread->WaitStatus;
                        goto DontWait;
                    }
                    else
                    {
                        /* Raise an exception */
                        KiReleaseDispatcherLock(Thread->WaitIrql);
                        ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                   }
                }
            }
            else if (CurrentObject->Header.SignalState > 0)
            {
                /* Another satisfied object */
                KiSatisfyNonMutantWait(CurrentObject);
                WaitStatus = STATUS_WAIT_0;
                goto DontWait;
            }

            /* Make sure we can satisfy the Alertable request */
            WaitStatus = KiCheckAlertability(Thread, Alertable, WaitMode);
            if (WaitStatus != STATUS_WAIT_0) break;

            /* Enable the Timeout Timer if there was any specified */
            if (Timeout)
            {
                /* Check if the timer expired */
                InterruptTime.QuadPart = KeQueryInterruptTime();
                if ((ULONGLONG)InterruptTime.QuadPart >=
                    Timer->DueTime.QuadPart)
                {
                    /* It did, so we don't need to wait */
                    WaitStatus = STATUS_TIMEOUT;
                    goto DontWait;
                }

                /* It didn't, so activate it */
                Timer->Header.Inserted = TRUE;
            }

            /* Link the Object to this Wait Block */
            InsertTailList(&CurrentObject->Header.WaitListHead,
                           &WaitBlock->WaitListEntry);

            /* Handle Kernel Queues */
            if (Thread->Queue) KiActivateWaiterQueue(Thread->Queue);

            /* Setup the wait information */
            Thread->State = Waiting;

            /* Add the thread to the wait list */
            KiAddThreadToWaitList(Thread, Swappable);

            /* Activate thread swap */
            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
            KiSetThreadSwapBusy(Thread);

            /* Check if we have a timer */
            if (Timeout)
            {
                /* Insert it */
                KxInsertTimer(Timer, Hand);
            }
            else
            {
                /* Otherwise, unlock the dispatcher */
                KiReleaseDispatcherLockFromSynchLevel();
            }

            /* Do the actual swap */
            WaitStatus = (NTSTATUS)KiSwapThread(Thread, KeGetCurrentPrcb());

            /* Check if we were executing an APC */
            if (WaitStatus != STATUS_KERNEL_APC) return WaitStatus;

            /* Check if we had a timeout */
            if (Timeout)
            {
                /* Recalculate due times */
                Timeout = KiRecalculateDueTime(OriginalDueTime,
                                               &DueTime,
                                               &NewDueTime);
            }
        }
WaitStart:
        /* Setup a new wait */
        Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
        KxSingleThreadWait();
        KiAcquireDispatcherLockAtSynchLevel();
    }

    /* Wait complete */
    KiReleaseDispatcherLock(Thread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Release dispatcher lock but maintain high IRQL */
    KiReleaseDispatcherLockFromSynchLevel();

    /* Adjust the Quantum and return the wait status */
    KiAdjustQuantumThread(Thread);
    return WaitStatus;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeWaitForMultipleObjects(IN ULONG Count,
                         IN PVOID Object[],
                         IN WAIT_TYPE WaitType,
                         IN KWAIT_REASON WaitReason,
                         IN KPROCESSOR_MODE WaitMode,
                         IN BOOLEAN Alertable,
                         IN PLARGE_INTEGER Timeout OPTIONAL,
                         OUT PKWAIT_BLOCK WaitBlockArray OPTIONAL)
{
    PKMUTANT CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK TimerBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    PKTIMER Timer = &Thread->Timer;
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    BOOLEAN Swappable;
    PLARGE_INTEGER OriginalDueTime = Timeout;
    LARGE_INTEGER DueTime = {{0}}, NewDueTime, InterruptTime;
    ULONG Index, Hand = 0;

    if (Thread->WaitNext)
        ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);
    else if (!Timeout || (Timeout->QuadPart != 0))
    {
        ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    }
    else
        ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    /* Make sure the Wait Count is valid */
    if (!WaitBlockArray)
    {
        /* Check in regards to the Thread Object Limit */
        if (Count > THREAD_WAIT_OBJECTS)
        {
            /* Bugcheck */
            KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }

        /* Use the Thread's Wait Block */
        WaitBlockArray = &Thread->WaitBlock[0];
    }
    else
    {
        /* Using our own Block Array, so check with the System Object Limit */
        if (Count > MAXIMUM_WAIT_OBJECTS)
        {
            /* Bugcheck */
            KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }
    }

    /* Sanity check */
    ASSERT(Count != 0);

    /* Check if the lock is already held */
    if (!Thread->WaitNext) goto WaitStart;

    /*  Otherwise, we already have the lock, so initialize the wait */
    Thread->WaitNext = FALSE;
    /*  Note that KxMultiThreadWait is a macro, defined in ke_x.h, that  */
    /*  uses  (and modifies some of) the following local                 */
    /*  variables:                                                       */
    /*  Thread, Index, WaitBlock, Timer, Timeout, Hand and Swappable.    */
    /*  If it looks like this code doesn't actually wait for any objects */
    /*  at all, it's because the setup is done by that macro.            */
    KxMultiThreadWait();

    /* Start wait loop */
    for (;;)
    {
        /* Disable pre-emption */
        Thread->Preempted = FALSE;

        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((Thread->ApcState.KernelApcPending) && !(Thread->SpecialApcDisable) &&
            (Thread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KiReleaseDispatcherLock(Thread->WaitIrql);
        }
        else
        {
            /* Check what kind of wait this is */
            Index = 0;
            if (WaitType == WaitAny)
            {
                /* Loop blocks */
                do
                {
                    /* Get the Current Object */
                    CurrentObject = (PKMUTANT)Object[Index];
                    ASSERT(CurrentObject->Header.Type != QueueObject);

                    /* Check if the Object is a mutant */
                    if (CurrentObject->Header.Type == MutantObject)
                    {
                        /* Check if it's signaled */
                        if ((CurrentObject->Header.SignalState > 0) ||
                            (Thread == CurrentObject->OwnerThread))
                        {
                            /* This is a Wait Any, so unwait this and exit */
                            if (CurrentObject->Header.SignalState !=
                                (LONG)MINLONG)
                            {
                                /* Normal signal state, unwait it and return */
                                KiSatisfyMutantWait(CurrentObject, Thread);
                                WaitStatus = (NTSTATUS)Thread->WaitStatus | Index;
                                goto DontWait;
                            }
                            else
                            {
                                /* Raise an exception (see wasm.ru) */
                                KiReleaseDispatcherLock(Thread->WaitIrql);
                                ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                            }
                        }
                    }
                    else if (CurrentObject->Header.SignalState > 0)
                    {
                        /* Another signaled object, unwait and return */
                        KiSatisfyNonMutantWait(CurrentObject);
                        WaitStatus = Index;
                        goto DontWait;
                    }

                    /* Go to the next block */
                    Index++;
                } while (Index < Count);
            }
            else
            {
                /* Loop blocks */
                do
                {
                    /* Get the Current Object */
                    CurrentObject = (PKMUTANT)Object[Index];
                    ASSERT(CurrentObject->Header.Type != QueueObject);

                    /* Check if we're dealing with a mutant again */
                    if (CurrentObject->Header.Type == MutantObject)
                    {
                        /* Check if it has an invalid count */
                        if ((Thread == CurrentObject->OwnerThread) &&
                            (CurrentObject->Header.SignalState == (LONG)MINLONG))
                        {
                            /* Raise an exception */
                            KiReleaseDispatcherLock(Thread->WaitIrql);
                            ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                        }
                        else if ((CurrentObject->Header.SignalState <= 0) &&
                                 (Thread != CurrentObject->OwnerThread))
                        {
                            /* We don't own it, can't satisfy the wait */
                            break;
                        }
                    }
                    else if (CurrentObject->Header.SignalState <= 0)
                    {
                        /* Not signaled, can't satisfy */
                        break;
                    }

                    /* Go to the next block */
                    Index++;
                } while (Index < Count);

                /* Check if we've went through all the objects */
                if (Index == Count)
                {
                    /* Loop wait blocks */
                    WaitBlock = WaitBlockArray;
                    do
                    {
                        /* Get the object and satisfy it */
                        CurrentObject = (PKMUTANT)WaitBlock->Object;
                        KiSatisfyObjectWait(CurrentObject, Thread);

                        /* Go to the next block */
                        WaitBlock = WaitBlock->NextWaitBlock;
                    } while(WaitBlock != WaitBlockArray);

                    /* Set the wait status and get out */
                    WaitStatus = (NTSTATUS)Thread->WaitStatus;
                    goto DontWait;
                }
            }

            /* Make sure we can satisfy the Alertable request */
            WaitStatus = KiCheckAlertability(Thread, Alertable, WaitMode);
            if (WaitStatus != STATUS_WAIT_0) break;

            /* Enable the Timeout Timer if there was any specified */
            if (Timeout)
            {
                /* Check if the timer expired */
                InterruptTime.QuadPart = KeQueryInterruptTime();
                if ((ULONGLONG)InterruptTime.QuadPart >=
                    Timer->DueTime.QuadPart)
                {
                    /* It did, so we don't need to wait */
                    WaitStatus = STATUS_TIMEOUT;
                    goto DontWait;
                }

                /* It didn't, so activate it */
                Timer->Header.Inserted = TRUE;

                /* Link the wait blocks */
                WaitBlock->NextWaitBlock = TimerBlock;
            }

            /* Insert into Object's Wait List*/
            WaitBlock = WaitBlockArray;
            do
            {
                /* Get the Current Object */
                CurrentObject = WaitBlock->Object;

                /* Link the Object to this Wait Block */
                InsertTailList(&CurrentObject->Header.WaitListHead,
                               &WaitBlock->WaitListEntry);

                /* Move to the next Wait Block */
                WaitBlock = WaitBlock->NextWaitBlock;
            } while (WaitBlock != WaitBlockArray);

            /* Handle Kernel Queues */
            if (Thread->Queue) KiActivateWaiterQueue(Thread->Queue);

            /* Setup the wait information */
            Thread->State = Waiting;

            /* Add the thread to the wait list */
            KiAddThreadToWaitList(Thread, Swappable);

            /* Activate thread swap */
            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
            KiSetThreadSwapBusy(Thread);

            /* Check if we have a timer */
            if (Timeout)
            {
                /* Insert it */
                KxInsertTimer(Timer, Hand);
            }
            else
            {
                /* Otherwise, unlock the dispatcher */
                KiReleaseDispatcherLockFromSynchLevel();
            }

            /* Swap the thread */
            WaitStatus = (NTSTATUS)KiSwapThread(Thread, KeGetCurrentPrcb());

            /* Check if we were executing an APC */
            if (WaitStatus != STATUS_KERNEL_APC) return WaitStatus;

            /* Check if we had a timeout */
            if (Timeout)
            {
                /* Recalculate due times */
                Timeout = KiRecalculateDueTime(OriginalDueTime,
                                               &DueTime,
                                               &NewDueTime);
            }
        }

WaitStart:
        /* Setup a new wait */
        Thread->WaitIrql = KeRaiseIrqlToSynchLevel();
        KxMultiThreadWait();
        KiAcquireDispatcherLockAtSynchLevel();
    }

    /* We are done */
    KiReleaseDispatcherLock(Thread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Release dispatcher lock but maintain high IRQL */
    KiReleaseDispatcherLockFromSynchLevel();

    /* Adjust the Quantum and return the wait status */
    KiAdjustQuantumThread(Thread);
    return WaitStatus;
}

NTSTATUS
NTAPI
NtDelayExecution(IN BOOLEAN Alertable,
                 IN PLARGE_INTEGER DelayInterval)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER SafeInterval;
    NTSTATUS Status;

    /* Check the previous mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe and capture the time out */
            SafeInterval = ProbeForReadLargeInteger(DelayInterval);
            DelayInterval = &SafeInterval;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
   }

   /* Call the Kernel Function */
   Status = KeDelayExecutionThread(PreviousMode,
                                   Alertable,
                                   DelayInterval);

   /* Return Status */
   return Status;
}

/* EOF */
