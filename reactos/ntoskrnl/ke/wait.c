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

/* GLOBALS ******************************************************************/

KSPIN_LOCK DispatcherDatabaseLock;

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
    PLIST_ENTRY WaitEntry;
    PLIST_ENTRY WaitList;
    PKWAIT_BLOCK CurrentWaitBlock;
    PKWAIT_BLOCK NextWaitBlock;
    PKTHREAD WaitThread;
    PKMUTANT FirstObject = ObjectPointer, Object;

    /* Loop the Wait Entries */
    WaitList = &FirstObject->Header.WaitListHead;
    WaitEntry = WaitList->Flink;
    while ((FirstObject->Header.SignalState > 0) &&
           (WaitEntry != WaitList))
    {
        /* Get the current wait block */
        CurrentWaitBlock = CONTAINING_RECORD(WaitEntry,
                                             KWAIT_BLOCK,
                                             WaitListEntry);
        WaitThread = CurrentWaitBlock->Thread;

        /* Check the current Wait Mode */
        if (CurrentWaitBlock->WaitType == WaitAny)
        {
            /* Easy case, satisfy only this wait */
            WaitEntry = WaitEntry->Blink;
            KiSatisfyObjectWait(FirstObject, WaitThread);
        }
        else
        {
            /* Everything must be satisfied */
            NextWaitBlock = CurrentWaitBlock->NextWaitBlock;

            /* Loop first to make sure they are valid */
            while (NextWaitBlock != CurrentWaitBlock)
            {
                /* Make sure this isn't a timeout block */
                if (NextWaitBlock->WaitKey != STATUS_TIMEOUT)
                {
                    /* Get the object */
                    Object = NextWaitBlock->Object;

                    /* Check if this is a mutant */
                    if ((Object->Header.Type == MutantObject) &&
                        (Object->Header.SignalState <= 0) &&
                        (WaitThread == Object->OwnerThread))
                    {
                        /* It's a signaled mutant */
                    }
                    else if (Object->Header.SignalState <= 0)
                    {
                        /* Skip the unwaiting */
                        goto SkipUnwait;
                    }
                }

                /* Go to the next Wait block */
                NextWaitBlock = NextWaitBlock->NextWaitBlock;
            }

            /* All the objects are signaled, we can satisfy */
            WaitEntry = WaitEntry->Blink;
            KiWaitSatisfyAll(CurrentWaitBlock);
        }

        /* All waits satisfied, unwait the thread */
        KiAbortWaitThread(WaitThread, CurrentWaitBlock->WaitKey, Increment);

SkipUnwait:
        /* Next entry */
        WaitEntry = WaitEntry->Flink;
    }
}

/* Must be called with the dispatcher lock held */
VOID
FASTCALL
KiAbortWaitThread(IN PKTHREAD Thread,
                  IN NTSTATUS WaitStatus,
                  IN KPRIORITY Increment)
{
    PKWAIT_BLOCK WaitBlock;
    PKTIMER Timer;
    LONG NewPriority;

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

    /* Check if this is a non-RT thread */
    if (Thread->Priority < LOW_REALTIME_PRIORITY)
    {
        /* Check if boosting is enabled and we can boost */
        if (!(Thread->DisableBoost) && !(Thread->PriorityDecrement))
        {
            /* We can boost, so calculate the new priority */
            NewPriority = Thread->BasePriority + Increment;
            if (NewPriority > Thread->Priority)
            {
                /* Make sure the new priority wouldn't push the thread to RT */
                if (NewPriority >= LOW_REALTIME_PRIORITY)
                {
                    /* Set it just before the RT zone */
                    Thread->Priority = LOW_REALTIME_PRIORITY - 1;
                }
                else
                {
                    /* Otherwise, set our calculated priority */
                    Thread->Priority = NewPriority;
                }
            }
        }

        /* Check if this is a high-priority thread */
        if (Thread->BasePriority >= 14)
        {
            /* It is, simply reset the quantum */
            Thread->Quantum = Thread->QuantumReset;
        }
        else
        {
            /* Otherwise, decrease quantum */
            Thread->Quantum--;
            if (Thread->Quantum <= 0)
            {
                /* We've went below 0, reset it */
                Thread->Quantum = Thread->QuantumReset;

                /* Apply per-quantum priority decrement */
                Thread->Priority -= (Thread->PriorityDecrement + 1);
                if (Thread->Priority < Thread->BasePriority)
                {
                    /* We've went too low, reset it */
                    Thread->Priority = Thread->BasePriority;
                }

                /* Delete per-quantum decrement */
                Thread->PriorityDecrement = 0;
            }
        }
    }
    else
    {
        /* For real time threads, just reset the quantum */
        Thread->Quantum = Thread->QuantumReset;
    }

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

VOID
FASTCALL
KiExitDispatcher(IN KIRQL OldIrql)
{
    /* Check if it's the idle thread */
    if (!(KeIsExecutingDpc()) &&
        (OldIrql < DISPATCH_LEVEL) &&
        (KeGetCurrentThread()) &&
        (KeGetCurrentThread() == KeGetCurrentPrcb()->IdleThread))
    {
        /* Dispatch a new thread */
        KiDispatchThreadNoLock(Ready);
    }
    else
    {
        /* Otherwise just release the lock */
        KeReleaseDispatcherDatabaseLockFromDpcLevel();
    }

    /* Lower irql back */
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
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    BOOLEAN Swappable;

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        CurrentThread->WaitNext = FALSE;
    }
    else
    {
        /* Lock not held, acquire it */
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Use built-in Wait block */
    TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];

    /* Start Wait Loop */
    do
    {
        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            !(CurrentThread->SpecialApcDisable) &&
            (CurrentThread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
        }
        else
        {
            /* Check if we can do an alertable wait, if requested */
            KiCheckAlertability();

            /* Check if we can swap the thread's stack */
            CurrentThread->WaitListEntry.Flink = NULL;
            KiCheckThreadStackSwap(WaitMode, CurrentThread, Swappable);

            /* Set status */
            CurrentThread->WaitStatus = STATUS_WAIT_0;

            /* Set Timer */
            ThreadTimer = &CurrentThread->Timer;

            /* Setup the Wait Block */
            CurrentThread->WaitBlockList = TimerWaitBlock;
            TimerWaitBlock->NextWaitBlock = TimerWaitBlock;

            /* Link the timer to this Wait Block */
            ThreadTimer->Header.WaitListHead.Flink =
                &TimerWaitBlock->WaitListEntry;
            ThreadTimer->Header.WaitListHead.Blink =
                &TimerWaitBlock->WaitListEntry;

            /* Insert the Timer into the Timer Lists and enable it */
            if (!KiInsertTimer(ThreadTimer, *Interval))
            {
                /* FIXME: We should find a new ready thread */
                WaitStatus = STATUS_SUCCESS;
                break;
            }

            /* Handle Kernel Queues */
            if (CurrentThread->Queue) KiWakeQueue(CurrentThread->Queue);

            /* Setup the wait information */
            CurrentThread->Alertable = Alertable;
            CurrentThread->WaitMode = WaitMode;
            CurrentThread->WaitReason = DelayExecution;
            CurrentThread->WaitTime = ((PLARGE_INTEGER)&KeTickCount)->LowPart;
            CurrentThread->State = Waiting;

            /* Find a new thread to run */
            KiAddThreadToWaitList(CurrentThread, Swappable);
            WaitStatus = KiSwapThread();
            ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

            /* Check if we were executing an APC or if we timed out */
            if (WaitStatus != STATUS_KERNEL_APC)
            {
                /* This is a good thing */
                if (WaitStatus == STATUS_TIMEOUT) WaitStatus = STATUS_SUCCESS;

                /* Return Status */
                return WaitStatus;
            }

            /* Check if we had a timeout */
            DPRINT1("If you see this message, contact Alex ASAP\n");
            KEBUGCHECK(0);
        }

        /* Acquire again the lock */
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    } while (TRUE);

    /* Release the Lock, we are done */
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
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
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    BOOLEAN Swappable;

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        CurrentThread->WaitNext = FALSE;
    }
    else
    {
        /* Lock not held, acquire it */
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Start the actual Loop */
    WaitBlock = &CurrentThread->WaitBlock[0];
    do
    {
        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            !(CurrentThread->SpecialApcDisable) &&
            (CurrentThread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
        }
        else
        {
            /* Set default status */
            CurrentThread->WaitStatus = STATUS_WAIT_0;

            /* Get the Current Object */
            CurrentObject = (PKMUTANT)Object;

            /* Check if it's a mutant */
            if (CurrentObject->Header.Type == MutantObject)
            {
                /* Check its signal state or if we own it */
                if ((CurrentObject->Header.SignalState > 0) ||
                    (CurrentThread == CurrentObject->OwnerThread))
                {
                    /* Just unwait this guy and exit */
                    if (CurrentObject->Header.SignalState != (LONG)MINLONG)
                    {
                        /* It has a normal signal state. Unwait and return */
                        KiSatisfyMutantWait(CurrentObject, CurrentThread);
                        WaitStatus = CurrentThread->WaitStatus;
                        goto DontWait;
                    }
                    else
                    {
                        /* Raise an exception (see wasm.ru) */
                        KeReleaseDispatcherDatabaseLock(CurrentThread->
                                                        WaitIrql);
                        ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                   }
                }
            }
            else if (CurrentObject->Header.SignalState > 0)
            {
                /* Another satisfied object */
                KiSatisfyNonMutantWait(CurrentObject, CurrentThread);
                WaitStatus = STATUS_WAIT_0;
                goto DontWait;
            }

            /* Append wait block to the KTHREAD wait block list */
            CurrentThread->WaitBlockList = WaitBlock;

            /* Set up the Wait Block */
            WaitBlock->Object = CurrentObject;
            WaitBlock->WaitKey = (USHORT)(STATUS_SUCCESS);
            WaitBlock->WaitType = WaitAny;

            /* Make sure we can satisfy the Alertable request */
            KiCheckAlertability();

            /* Check if we can swap the thread's stack */
            CurrentThread->WaitListEntry.Flink = NULL;
            KiCheckThreadStackSwap(WaitMode, CurrentThread, Swappable);

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

                /* Point to Timer Wait Block and Thread Timer */
                TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];
                ThreadTimer = &CurrentThread->Timer;

                /* Connect the Timer Wait Block */
                WaitBlock->NextWaitBlock = TimerWaitBlock;

                /* Set up the Timer Wait Block */
                TimerWaitBlock->NextWaitBlock = WaitBlock;

                /* Link the timer to this Wait Block */
                ThreadTimer->Header.WaitListHead.Flink =
                    &TimerWaitBlock->WaitListEntry;
                ThreadTimer->Header.WaitListHead.Blink =
                    &TimerWaitBlock->WaitListEntry;

                /* Insert the Timer into the Timer Lists and enable it */
                if (!KiInsertTimer(ThreadTimer, *Timeout))
                {
                    /* Return a timeout if we couldn't insert the timer */
                    WaitStatus = STATUS_TIMEOUT;
                    goto DontWait;
                }
            }
            else
            {
                /* No timer block, so just set our wait block as next */
                WaitBlock->NextWaitBlock = WaitBlock;
            }

            /* Link the Object to this Wait Block */
            InsertTailList(&CurrentObject->Header.WaitListHead,
                           &WaitBlock->WaitListEntry);

            /* Handle Kernel Queues */
            if (CurrentThread->Queue) KiWakeQueue(CurrentThread->Queue);

            /* Setup the wait information */
            CurrentThread->Alertable = Alertable;
            CurrentThread->WaitMode = WaitMode;
            CurrentThread->WaitReason = WaitReason;
            CurrentThread->WaitTime = ((PLARGE_INTEGER)&KeTickCount)->LowPart;
            CurrentThread->State = Waiting;

            /* Find a new thread to run */
            KiAddThreadToWaitList(CurrentThread, Swappable);
            WaitStatus = KiSwapThread();
            ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

            /* Check if we were executing an APC */
            if (WaitStatus != STATUS_KERNEL_APC) return WaitStatus;

            /* Check if we had a timeout */
            if (Timeout)
            {
                DPRINT1("If you see this message, contact Alex ASAP\n");
                KEBUGCHECK(0);
            }
        }

        /* Acquire again the lock */
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    } while (TRUE);

    /* Release the Lock, we are done */
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(CurrentThread);

    /* Release & Return */
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
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
    PKTHREAD CurrentThread = KeGetCurrentThread();
    ULONG AllObjectsSignaled;
    ULONG WaitIndex;
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    BOOLEAN Swappable;

    /* Set the Current Thread */
    CurrentThread = KeGetCurrentThread();

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        CurrentThread->WaitNext = FALSE;
    }
    else
    {
        /* Lock not held, acquire it */
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

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
        WaitBlockArray = &CurrentThread->WaitBlock[0];
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

    /* Start the actual Loop */
    do
    {
        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            !(CurrentThread->SpecialApcDisable) &&
            (CurrentThread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
        }
        else
        {
            /* Append wait block to the KTHREAD wait block list */
            CurrentThread->WaitBlockList = WaitBlock = WaitBlockArray;

            /* Set default wait status */
            CurrentThread->WaitStatus = STATUS_WAIT_0;

            /* Check if the wait is (already) satisfied */
            AllObjectsSignaled = TRUE;

            /* First, we'll try to satisfy the wait directly */
            for (WaitIndex = 0; WaitIndex < Count; WaitIndex++)
            {
                /* Get the Current Object */
                CurrentObject = (PKMUTANT)Object[WaitIndex];

                /* Check the type of wait */
                if (WaitType == WaitAny)
                {
                    /* Check if the Object is a mutant */
                    if (CurrentObject->Header.Type == MutantObject)
                    {
                        /* Check if it's signaled */
                        if ((CurrentObject->Header.SignalState > 0) ||
                            (CurrentThread == CurrentObject->OwnerThread))
                        {
                            /* This is a Wait Any, so unwait this and exit */
                            if (CurrentObject->Header.SignalState !=
                                (LONG)MINLONG)
                            {
                                /* Normal signal state, unwait it and return */
                                KiSatisfyMutantWait(CurrentObject,
                                                    CurrentThread);
                                WaitStatus = CurrentThread->WaitStatus |
                                             WaitIndex;
                                goto DontWait;
                            }
                            else
                            {
                                /* Raise an exception (see wasm.ru) */
                                KeReleaseDispatcherDatabaseLock(CurrentThread->
                                                                WaitIrql);
                                ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                            }
                        }
                    }
                    else if (CurrentObject->Header.SignalState > 0)
                    {
                        /* Another signaled object, unwait and return */
                        KiSatisfyNonMutantWait(CurrentObject, CurrentThread);
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
                        if ((CurrentThread == CurrentObject->OwnerThread) &&
                            (CurrentObject->Header.SignalState == MINLONG))
                        {
                            /* Raise an exception */
                            KeReleaseDispatcherDatabaseLock(CurrentThread->
                                                            WaitIrql);
                            ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                        }
                        else if ((CurrentObject->Header.SignalState <= 0) &&
                                 (CurrentThread != CurrentObject->OwnerThread))
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
                WaitBlock->Object = CurrentObject;
                WaitBlock->Thread = CurrentThread;
                WaitBlock->WaitKey = (USHORT)WaitIndex;
                WaitBlock->WaitType = (USHORT)WaitType;
                WaitBlock->NextWaitBlock = WaitBlock + 1;

                /* Move to the next Wait Block */
                WaitBlock = WaitBlock->NextWaitBlock;
            }

            /* Return to the Root Wait Block */
            WaitBlock--;
            WaitBlock->NextWaitBlock = WaitBlockArray;

            /* Check if this is a Wait All and all the objects are signaled */
            if ((WaitType == WaitAll) && (AllObjectsSignaled))
            {
                /* Return to the Root Wait Block */
                WaitBlock = CurrentThread->WaitBlockList;

                /* Satisfy their Waits and return to the caller */
                KiWaitSatisfyAll(WaitBlock);
                WaitStatus = CurrentThread->WaitStatus;
                goto DontWait;
            }

            /* Make sure we can satisfy the Alertable request */
            KiCheckAlertability();

            /* Check if we can swap the thread's stack */
            CurrentThread->WaitListEntry.Flink = NULL;
            KiCheckThreadStackSwap(WaitMode, CurrentThread, Swappable);

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

                /* Point to Timer Wait Block and Thread Timer */
                TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];
                ThreadTimer = &CurrentThread->Timer;

                /* Connect the Timer Wait Block */
                WaitBlock->NextWaitBlock = TimerWaitBlock;

                /* Set up the Timer Wait Block */
                TimerWaitBlock->NextWaitBlock = WaitBlockArray;

                /* Initialize the list head */
                InitializeListHead(&ThreadTimer->Header.WaitListHead);

                /* Insert the Timer into the Timer Lists and enable it */
                if (!KiInsertTimer(ThreadTimer, *Timeout))
                {
                    /* Return a timeout if we couldn't insert the timer */
                    WaitStatus = STATUS_TIMEOUT;
                    goto DontWait;
                }
            }

            /* Insert into Object's Wait List*/
            WaitBlock = CurrentThread->WaitBlockList;
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
            if (CurrentThread->Queue) KiWakeQueue(CurrentThread->Queue);

            /* Setup the wait information */
            CurrentThread->Alertable = Alertable;
            CurrentThread->WaitMode = WaitMode;
            CurrentThread->WaitReason = WaitReason;
            CurrentThread->WaitTime = ((PLARGE_INTEGER)&KeTickCount)->LowPart;
            CurrentThread->State = Waiting;

            /* Find a new thread to run */
            KiAddThreadToWaitList(CurrentThread, Swappable);
            WaitStatus = KiSwapThread();
            ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

            /* Check if we were executing an APC */
            if (WaitStatus != STATUS_KERNEL_APC) return WaitStatus;

            /* Check if we had a timeout */
            if (Timeout)
            {
                DPRINT1("If you see this message, contact Alex ASAP\n");
                KEBUGCHECK(0);
            }

            /* Acquire again the lock */
            CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
        }
    } while (TRUE);

    /* Release the Lock, we are done */
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(CurrentThread);

    /* Release & Return */
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;
}

/* EOF */
