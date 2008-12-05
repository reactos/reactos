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
               IN NTSTATUS WaitStatus)
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
    KeWaitForSingleObject(&FastMutex->Gate,
                          WrMutex,
                          KernelMode,
                          FALSE,
                          NULL);
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

    /* Make sure we're at synchronization level */
    ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);

    /* Check if we have deferred threads */
    KiCheckDeferredReadyList(Prcb);

    /* Check if we were called at dispatcher level or higher */
    if (OldIrql >= DISPATCH_LEVEL)
    {
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
    if (!Prcb->NextThread) goto Quickie;

    /* Lock the PRCB */
    KiAcquirePrcbLock(Prcb);

    /* Get the next and current threads now */
    NextThread = Prcb->NextThread;
    Thread = Prcb->CurrentThread;

    /* Set current thread's swap busy to true */
    KiSetThreadSwapBusy(Thread);

    /* Switch threads in PRCB */
    Prcb->NextThread = NULL;
    Prcb->CurrentThread = NextThread;

    /* Set thread to running */
    NextThread->State = Running;

    /* Queue it on the ready lists */
    KxQueueReadyThread(Thread, Prcb);

    /* Set wait IRQL */
    Thread->WaitIrql = OldIrql;

    /* Swap threads and check if APCs were pending */
    PendingApc = KiSwapContext(Thread, NextThread);
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
    KeLowerIrql(OldIrql);
}

/* PUBLIC FUNCTIONS **********************************************************/

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

    /* If this is a user-mode wait of 0 seconds, yield execution */
    if (!(Interval->QuadPart) && (WaitMode != KernelMode))
    {
        /* Make sure the wait isn't alertable or interrupting an APC */
        if (!(Alertable) && !(Thread->ApcState.UserApcPending))
        {
            /* Yield execution */
            NtYieldExecution();
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
            WaitStatus = KiSwapThread(Thread, KeGetCurrentPrcb());

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
        KiAcquireDispatcherLockAtDpcLevel();
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
    KiReleaseDispatcherLockFromDpcLevel();
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
    LARGE_INTEGER DueTime, NewDueTime, InterruptTime;
    PLARGE_INTEGER OriginalDueTime = Timeout;
    ULONG Hand = 0;

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
                    if (CurrentObject->Header.SignalState != (LONG)MINLONG)
                    {
                        /* It has a normal signal state. Unwait and return */
                        KiSatisfyMutantWait(CurrentObject, Thread);
                        WaitStatus = Thread->WaitStatus;
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
                KiReleaseDispatcherLockFromDpcLevel();
            }

            /* Do the actual swap */
            WaitStatus = KiSwapThread(Thread, KeGetCurrentPrcb());

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
        KiAcquireDispatcherLockAtDpcLevel();
    }

    /* Wait complete */
    KiReleaseDispatcherLock(Thread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Release dispatcher lock but maintain high IRQL */
    KiReleaseDispatcherLockFromDpcLevel();

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
    LARGE_INTEGER DueTime, NewDueTime, InterruptTime;
    ULONG Index, Hand = 0;

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
                                WaitStatus = Thread->WaitStatus | Index;
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
                    WaitStatus = Thread->WaitStatus;
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
                KiReleaseDispatcherLockFromDpcLevel();
            }

            /* Swap the thread */
            WaitStatus = KiSwapThread(Thread, KeGetCurrentPrcb());

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
        KiAcquireDispatcherLockAtDpcLevel();
    }

    /* We are done */
    KiReleaseDispatcherLock(Thread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Release dispatcher lock but maintain high IRQL */
    KiReleaseDispatcherLockFromDpcLevel();

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
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check the previous mode */
    if(PreviousMode != KernelMode)
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
            /* Get SEH exception */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        if (!NT_SUCCESS(Status)) return Status;
   }

   /* Call the Kernel Function */
   Status = KeDelayExecutionThread(PreviousMode,
                                   Alertable,
                                   DelayInterval);

   /* Return Status */
   return Status;
}

/* EOF */
