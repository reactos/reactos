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
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FASTCALL
KiWaitSatisfyAll(PKWAIT_BLOCK FirstBlock)
{
    PKWAIT_BLOCK WaitBlock = FirstBlock;
    PKTHREAD WaitThread = WaitBlock->Thread;

    /* Loop through all the Wait Blocks, and wake each Object */
    do
    {
        /* Make sure it hasn't timed out */
        if (WaitBlock->WaitKey != STATUS_TIMEOUT)
        {
            /* Wake the Object */
            KiSatisfyObjectWait((PKMUTANT)WaitBlock->Object, WaitThread);
        }

        /* Move to the next block */
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != FirstBlock);
}

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
    if (Timer->Header.Inserted)
    {
        /* Remove the timer */
        Timer->Header.Inserted = FALSE;
        RemoveEntryList(&Timer->TimerListEntry);
        //KiRemoveTimer(Timer);
    }

    /* Increment the Queue's active threads */
    if (Thread->Queue) Thread->Queue->CurrentCount++;
}

/* Must be called with the dispatcher lock held */
VOID
FASTCALL
KiUnwaitThread(IN PKTHREAD Thread,
               IN NTSTATUS WaitStatus,
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
    ASSERT_IRQL(SYNCH_LEVEL);

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
    PKTIMER ThreadTimer;
    PKTHREAD Thread = KeGetCurrentThread();
    NTSTATUS WaitStatus;
    BOOLEAN Swappable;
    PLARGE_INTEGER OriginalDueTime = Interval;
    LARGE_INTEGER DueTime, NewDueTime;

    /* Check if the lock is already held */
    if (Thread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        Thread->WaitNext = FALSE;
        Swappable = KxDelayThreadWait(Thread, Alertable, WaitMode);
    }
    else
    {
        /* Lock not held, acquire it */
WaitStart:
        Thread->WaitIrql = KiAcquireDispatcherLock();
        Swappable = KxDelayThreadWait(Thread, Alertable, WaitMode);
    }

    /* Check if a kernel APC is pending and we're below APC_LEVEL */
    if ((Thread->ApcState.KernelApcPending) && !(Thread->SpecialApcDisable) &&
        (Thread->WaitIrql < APC_LEVEL))
    {
        /* Unlock the dispatcher */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        goto WaitStart;
    }

    /* Check if we have to bail out due to an alerted state */
    WaitStatus = KiCheckAlertability(Thread, Alertable, WaitMode);
    if (WaitStatus != STATUS_WAIT_0)
    {
        /* Unlock the dispatcher and return */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        return WaitStatus;
    }

    /* Set Timer */
    ThreadTimer = &Thread->Timer;

    /* Insert the Timer into the Timer Lists and enable it */
    if (!KiInsertTimer(ThreadTimer, *Interval))
    {
        /* FIXME: We should find a new ready thread */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        return STATUS_WAIT_0;
    }

    /* Save due time */
    DueTime.QuadPart = ThreadTimer->DueTime.QuadPart;

    /* Handle Kernel Queues */
    if (Thread->Queue) KiActivateWaiterQueue(Thread->Queue);

    /* Setup the wait information */
    Thread->State = Waiting;

    /* Add the thread to the wait list */
    KiAddThreadToWaitList(Thread, Swappable);

    /* Swap the thread */
    ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
    KiSetThreadSwapBusy(Thread);
    WaitStatus = KiSwapThread(Thread, KeGetCurrentPrcb());

    /* Check if we were executing an APC or if we timed out */
    if (WaitStatus == STATUS_KERNEL_APC)
    {
        /* Recalculate due times */
        Interval = KiRecalculateDueTime(OriginalDueTime,
                                        &DueTime,
                                        &NewDueTime);
        goto WaitStart;
    }

    /* This is a good thing */
    if (WaitStatus == STATUS_TIMEOUT) WaitStatus = STATUS_SUCCESS;

    /* Return Status */
    return WaitStatus;
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
    PKMUTANT CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD Thread = KeGetCurrentThread();
    NTSTATUS WaitStatus;
    BOOLEAN Swappable;
    LARGE_INTEGER DueTime, NewDueTime;
    PLARGE_INTEGER OriginalDueTime = Timeout;

    /* Get wait block */
    WaitBlock = &Thread->WaitBlock[0];

    /* Check if the lock is already held */
    if (Thread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        Thread->WaitNext = FALSE;
        Swappable = KxSingleThreadWait(Thread,
                                       WaitBlock,
                                       Object,
                                       Timeout,
                                       Alertable,
                                       WaitReason,
                                       WaitMode);
    }
    else
    {
StartWait:
        /* Lock not held, acquire it */
        Thread->WaitIrql = KiAcquireDispatcherLock();
        Swappable = KxSingleThreadWait(Thread,
                                       WaitBlock,
                                       Object,
                                       Timeout,
                                       WaitReason,
                                       WaitMode,
                                       Alertable);
    }

    /* Check if a kernel APC is pending and we're below APC_LEVEL */
    if ((Thread->ApcState.KernelApcPending) && !(Thread->SpecialApcDisable) &&
        (Thread->WaitIrql < APC_LEVEL))
    {
        /* Unlock the dispatcher and wait again */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        goto StartWait;
    }

    /* Get the Current Object */
    CurrentObject = (PKMUTANT)Object;
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
        KiSatisfyNonMutantWait(CurrentObject, Thread);
        WaitStatus = STATUS_WAIT_0;
        goto DontWait;
    }

    /* Make sure we can satisfy the Alertable request */
    WaitStatus = KiCheckAlertability(Thread, Alertable, WaitMode);
    if (WaitStatus != STATUS_WAIT_0)
    {
        /* Unlock the dispatcher and return */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        return WaitStatus;
    }

    /* Enable the Timeout Timer if there was any specified */
    if (Timeout)
    {
        /* Fail if the timeout interval is actually 0 */
        if (!Timeout->QuadPart)
        {
            /* Return a timeout */
            WaitStatus = STATUS_TIMEOUT;
            goto DontWait;
        }

        /* Insert the Timer into the Timer Lists and enable it */
        ThreadTimer = &Thread->Timer;
        if (!KiInsertTimer(ThreadTimer, *Timeout))
        {
            /* Return a timeout if we couldn't insert the timer */
            WaitStatus = STATUS_TIMEOUT;
            goto DontWait;
        }

        /* Set the current due time */
        DueTime.QuadPart = ThreadTimer->DueTime.QuadPart;
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

    /* Swap the thread */
    ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
    KiSetThreadSwapBusy(Thread);
    WaitStatus = KiSwapThread(Thread, KeGetCurrentPrcb());

    /* Check if we were executing an APC */
    if (WaitStatus == STATUS_KERNEL_APC)
    {
        /* Check if we had a timeout */
        if (Timeout)
        {
            /* Recalculate due times */
            Timeout = KiRecalculateDueTime(OriginalDueTime,
                                           &DueTime,
                                           &NewDueTime);
        }

        /* Wait again */
        goto StartWait;
    }

    /* Wait complete */
    return WaitStatus;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(Thread);

    /* Release & Return */
    KiReleaseDispatcherLock(Thread->WaitIrql);
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
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD Thread = KeGetCurrentThread();
    ULONG AllObjectsSignaled;
    ULONG WaitIndex;
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    BOOLEAN Swappable;
    PLARGE_INTEGER OriginalDueTime = Timeout;
    LARGE_INTEGER DueTime, NewDueTime;

    /* Make sure the Wait Count is valid */
    if (!WaitBlockArray)
    {
        /* Check in regards to the Thread Object Limit */
        if (Count > THREAD_WAIT_OBJECTS)
        {
            /* Bugcheck */
            KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
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
            KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }
    }

    /* Sanity check */
    ASSERT(Count != 0);

    /* Check if the lock is already held */
    if (Thread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        Thread->WaitNext = FALSE;
    }
    else
    {
        /* Lock not held, acquire it */
StartWait:
        Thread->WaitIrql = KiAcquireDispatcherLock();
    }

    /* Prepare for the wait */
    Swappable = KxMultiThreadWait(Thread,
                                  WaitBlockArray,
                                  Alertable,
                                  WaitReason,
                                  WaitMode);

    /* Check if a kernel APC is pending and we're below APC_LEVEL */
    if ((Thread->ApcState.KernelApcPending) && !(Thread->SpecialApcDisable) &&
        (Thread->WaitIrql < APC_LEVEL))
    {
        /* Unlock the dispatcher */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        goto StartWait;
    }

    /* Append wait block to the KTHREAD wait block list */
    WaitBlock = WaitBlockArray;

    /* Check if the wait is (already) satisfied */
    AllObjectsSignaled = TRUE;

    /* First, we'll try to satisfy the wait directly */
    for (WaitIndex = 0; WaitIndex < Count; WaitIndex++)
    {
        /* Get the Current Object */
        CurrentObject = (PKMUTANT)Object[WaitIndex];
        ASSERT(CurrentObject->Header.Type != QueueObject);

        /* Check the type of wait */
        if (WaitType == WaitAny)
        {
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
                        WaitStatus = Thread->WaitStatus | WaitIndex;
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
                KiSatisfyNonMutantWait(CurrentObject, Thread);
                WaitStatus = WaitIndex;
                goto DontWait;
            }
        }
        else
        {
            /* Check if we're dealing with a mutant again */
            if (CurrentObject->Header.Type == MutantObject)
            {
                /* Check if it has an invalid count */
                if ((Thread == CurrentObject->OwnerThread) &&
                    (CurrentObject->Header.SignalState == MINLONG))
                {
                    /* Raise an exception */
                    KiReleaseDispatcherLock(Thread->WaitIrql);
                    ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                }
                else if ((CurrentObject->Header.SignalState <= 0) &&
                         (Thread != CurrentObject->OwnerThread))
                {
                    /* We don't own it, can't satisfy the wait */
                    AllObjectsSignaled = FALSE;
                }
            }
            else if (CurrentObject->Header.SignalState <= 0)
            {
                /* Not signaled, can't satisfy */
                AllObjectsSignaled = FALSE;
            }
        }

        /* Set up a Wait Block for this Object */
        WaitBlock = &WaitBlockArray[WaitIndex];
        WaitBlock->Object = CurrentObject;
        WaitBlock->Thread = Thread;
        WaitBlock->WaitKey = (USHORT)WaitIndex;
        WaitBlock->WaitType = (UCHAR)WaitType;
        WaitBlock->NextWaitBlock = &WaitBlockArray[WaitIndex + 1];
    }

    /* Check if this is a Wait All and all the objects are signaled */
    if ((WaitType == WaitAll) && (AllObjectsSignaled))
    {
        /* Return to the Root Wait Block */
        WaitBlock->NextWaitBlock = &WaitBlockArray[0];

        /* Satisfy their Waits and return to the caller */
        KiWaitSatisfyAll(WaitBlock);
        WaitStatus = Thread->WaitStatus;
        goto DontWait;
    }

    /* Make sure we can satisfy the Alertable request */
    WaitStatus = KiCheckAlertability(Thread, Alertable, WaitMode);
    if (WaitStatus != STATUS_WAIT_0)
    {
        /* Unlock the dispatcher and return */
        KiReleaseDispatcherLock(Thread->WaitIrql);
        return WaitStatus;
    }

    /* Enable the Timeout Timer if there was any specified */
    if (Timeout)
    {
        /* Make sure the timeout interval isn't actually 0 */
        if (!Timeout->QuadPart)
        {
            /* Return a timeout */
            WaitStatus = STATUS_TIMEOUT;
            goto DontWait;
        }

        /* Link timer wait block */
        TimerWaitBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
        WaitBlock->NextWaitBlock = TimerWaitBlock;

        /* Use this the timer block for linking below */
        WaitBlock = TimerWaitBlock;

        /* Insert the Timer into the Timer Lists and enable it */
        ThreadTimer = &Thread->Timer;
        if (!KiInsertTimer(ThreadTimer, *Timeout))
        {
            /* Return a timeout if we couldn't insert the timer */
            WaitStatus = STATUS_TIMEOUT;
            goto DontWait;
        }

        /* Set the current due time */
        DueTime.QuadPart = ThreadTimer->DueTime.QuadPart;
    }

    /* Link to the Root Wait Block */
    WaitBlock->NextWaitBlock = &WaitBlockArray[0];

    /* Insert into Object's Wait List*/
    WaitBlock = &WaitBlockArray[0];
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

    /* Swap the thread */
    ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);
    KiSetThreadSwapBusy(Thread);
    WaitStatus = KiSwapThread(Thread, KeGetCurrentPrcb());

    /* Check if we were executing an APC */
    if (WaitStatus == STATUS_KERNEL_APC)
    {
        /* Check if we had a timeout */
        if (Timeout)
        {
            /* Recalculate due times */
            Timeout = KiRecalculateDueTime(OriginalDueTime,
                                           &DueTime,
                                           &NewDueTime);
        }

        /* Wait again */
        goto StartWait;
    }

    /* We are done */
    return WaitStatus;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(Thread);

    /* Release & Return */
    KiReleaseDispatcherLock(Thread->WaitIrql);
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
        _SEH_TRY
        {
            /* Probe and capture the time out */
            SafeInterval = ProbeForReadLargeInteger(DelayInterval);
            DelayInterval = &SafeInterval;
        }
        _SEH_HANDLE
        {
            /* Get SEH exception */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
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
