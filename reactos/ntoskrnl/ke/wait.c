/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/wait.c
 * PURPOSE:         Manages waiting for Dispatcher Objects
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

KSPIN_LOCK DispatcherDatabaseLock;

/* Tells us if the Timer or Event is a Syncronization or Notification Object */
#define TIMER_OR_EVENT_TYPE 0x7L

/* One of the Reserved Wait Blocks, this one is for the Thread's Timer */
#define TIMER_WAIT_BLOCK 0x3L

/* FUNCTIONS *****************************************************************/

BOOLEAN
__inline
FASTCALL
KiCheckAlertability(BOOLEAN Alertable,
                    PKTHREAD Thread,
                    KPROCESSOR_MODE WaitMode,
                    PNTSTATUS Status)
{
    /*
     * At this point, we have to do a wait, so make sure we can make
     * the thread Alertable if requested.
     */
    if (Alertable)
    {
        /* If the Thread is Alerted, set the Wait Status accordingly */
        if (Thread->Alerted[(int)WaitMode])
        {
            Thread->Alerted[(int)WaitMode] = FALSE;
            DPRINT("Thread was Alerted in the specified Mode\n");
            *Status = STATUS_ALERTED;
            return TRUE;
        }
        else if ((WaitMode != KernelMode) &&
                (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
        {
            /* If there are User APCs Pending, then we can't really be alertable */
            DPRINT("APCs are Pending\n");
            Thread->ApcState.UserApcPending = TRUE;
            *Status = STATUS_USER_APC;
            return TRUE;
        }
        else if (Thread->Alerted[KernelMode])
        {
            /* 
             * The thread is not alerted in the mode given, but it is alerted
             * in kernel-mode.
             */
            Thread->Alerted[KernelMode] = FALSE;
            DPRINT("Thread was Alerted in Kernel-Mode\n");
            *Status = STATUS_ALERTED;
            return TRUE;
        }
    }
    else if ((WaitMode != KernelMode) &&
             (Thread->ApcState.UserApcPending))
    {
        /*
         * If there are User APCs Pending and we are waiting in usermode,
         * then we must notify the caller
         */
        DPRINT("APCs are Pending\n");
        *Status = STATUS_USER_APC;
        return TRUE;
    }

    /* Stay in the loop */
    return FALSE;
}

/*
 * @implemented
 *
 * FUNCTION: Puts the current thread into an alertable or nonalertable
 * wait state for a given internal
 * ARGUMENTS:
 *          WaitMode = Processor mode in which the caller is waiting
 *          Altertable = Specifies if the wait is alertable
 *          Interval = Specifies the interval to wait
 * RETURNS: Status
 */
NTSTATUS
STDCALL
KeDelayExecutionThread(KPROCESSOR_MODE WaitMode,
                       BOOLEAN Alertable,
                       PLARGE_INTEGER Interval)
{
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    DPRINT("Entering KeDelayExecutionThread\n");

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;
    }
    else
    {
        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Use built-in Wait block */
    TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];

    /* Start Wait Loop */
    do
    {
        /* Check if a kernel APC is pending and we were below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            (CurrentThread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
            goto SkipWait;
        }

        /* Chceck if we can do an alertable wait, if requested */
        if (KiCheckAlertability(Alertable, CurrentThread, WaitMode, &WaitStatus)) break;

        /* Set status */
        CurrentThread->WaitStatus = STATUS_WAIT_0;

        /* Set Timer */
        ThreadTimer = &CurrentThread->Timer;

        /* Setup the Wait Block */
        CurrentThread->WaitBlockList = TimerWaitBlock;
        TimerWaitBlock->NextWaitBlock = TimerWaitBlock;

        /* Link the timer to this Wait Block */
        ThreadTimer->Header.WaitListHead.Flink = &TimerWaitBlock->WaitListEntry;
        ThreadTimer->Header.WaitListHead.Blink = &TimerWaitBlock->WaitListEntry;

        /* Insert the Timer into the Timer Lists and enable it */
        if (!KiInsertTimer(ThreadTimer, *Interval))
        {
            /* FIXME: The timer already expired, we should find a new ready thread */
            WaitStatus = STATUS_SUCCESS;
            break;
        }

        /* Handle Kernel Queues */
        if (CurrentThread->Queue)
        {
            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Setup the wait information */
        CurrentThread->Alertable = Alertable;
        CurrentThread->WaitMode = WaitMode;
        CurrentThread->WaitReason = DelayExecution;
        CurrentThread->WaitTime = ((PLARGE_INTEGER)&KeTickCount)->LowPart;
        CurrentThread->State = Waiting;

        /* Find a new thread to run */
        DPRINT("Swapping threads\n");
        WaitStatus = KiSwapThread();

        /* Check if we were executing an APC or if we timed out */
        if (WaitStatus != STATUS_KERNEL_APC)
        {
            /* This is a good thing */
            if (WaitStatus == STATUS_TIMEOUT) WaitStatus = STATUS_SUCCESS;

            /* Return Status */
            return WaitStatus;
        }

        /* FIXME: Fixup interval */

        /* Acquire again the lock */
SkipWait:
        DPRINT("Looping again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }
    while (TRUE);

    /* Release the Lock, we are done */
    DPRINT("Returning from KeDelayExecutionThread(), %x. Status: %d\n",
            KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;
}

/*
 * @implemented
 *
 * FUNCTION: Puts the current thread into a wait state until the
 * given dispatcher object is set to signalled
 * ARGUMENTS:
 *         Object = Object to wait on
 *         WaitReason = Reason for the wait (debugging aid)
 *         WaitMode = Can be KernelMode or UserMode, if UserMode then
 *                    user-mode APCs can be delivered and the thread's
 *                    stack can be paged out
 *         Altertable = Specifies if the wait is a alertable
 *         Timeout = Optional timeout value
 * RETURNS: Status
 */
NTSTATUS
STDCALL
KeWaitForSingleObject(PVOID Object,
                      KWAIT_REASON WaitReason,
                      KPROCESSOR_MODE WaitMode,
                      BOOLEAN Alertable,
                      PLARGE_INTEGER Timeout)
{
    PKMUTANT CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    DPRINT("Entering KeWaitForSingleObject\n");

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;
    }
    else
    {
        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Start the actual Loop */
    do
    {
        /* Check if a kernel APC is pending and we were below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            (CurrentThread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
            goto SkipWait;
        }

        /* Set default status */
        CurrentThread->WaitStatus = STATUS_WAIT_0;

        /* Append wait block to the KTHREAD wait block list */
        CurrentThread->WaitBlockList = WaitBlock = &CurrentThread->WaitBlock[0];

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
                    /* It has a normal signal state, so unwait it and return */
                    KiSatisfyMutantWait(CurrentObject, CurrentThread);
                    WaitStatus = CurrentThread->WaitStatus;
                    goto DontWait;
                }
                else
                {
                    /* According to wasm.ru, we must raise this exception (tested and true) */
                    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
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

        /* Set up the Wait Block */
        WaitBlock->Object = CurrentObject;
        WaitBlock->Thread = CurrentThread;
        WaitBlock->WaitKey = (USHORT)(STATUS_SUCCESS);
        WaitBlock->WaitType = WaitAny;
        WaitBlock->NextWaitBlock = WaitBlock;

        /* Make sure we can satisfy the Alertable request */
        if (KiCheckAlertability(Alertable, CurrentThread, WaitMode, &WaitStatus)) break;

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
            ThreadTimer->Header.WaitListHead.Flink = &TimerWaitBlock->WaitListEntry;
            ThreadTimer->Header.WaitListHead.Blink = &TimerWaitBlock->WaitListEntry;

            /* Insert the Timer into the Timer Lists and enable it */
            if (!KiInsertTimer(ThreadTimer, *Timeout))
            {
                /* Return a timeout if we couldn't insert the timer */
                WaitStatus = STATUS_TIMEOUT;
                goto DontWait;
            }
        }

        /* Link the Object to this Wait Block */
        InsertTailList(&CurrentObject->Header.WaitListHead,
                       &WaitBlock->WaitListEntry);

        /* Handle Kernel Queues */
        if (CurrentThread->Queue)
        {
            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Setup the wait information */
        CurrentThread->Alertable = Alertable;
        CurrentThread->WaitMode = WaitMode;
        CurrentThread->WaitReason = WaitReason;
        CurrentThread->WaitTime = ((PLARGE_INTEGER)&KeTickCount)->LowPart;
        CurrentThread->State = Waiting;

        /* Find a new thread to run */
        DPRINT("Swapping threads\n");
        WaitStatus = KiSwapThread();

        /* Check if we were executing an APC */
        if (WaitStatus != STATUS_KERNEL_APC)
        {
            /* Return Status */
            return WaitStatus;
        }

        /* Check if we had a timeout */
        if (Timeout)
        {
             /* FIXME: Fixup interval */
        }

        /* Acquire again the lock */
SkipWait:
        DPRINT("Looping again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }
    while (TRUE);

    /* Release the Lock, we are done */
    DPRINT("Returning from KeWaitForMultipleObjects(), %x. Status: %d\n",
            KeGetCurrentThread(), WaitStatus);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(CurrentThread);

    /* Release & Return */
    DPRINT("Quick-return from KeWaitForMultipleObjects(), %x. Status: %d\n.",
            KeGetCurrentThread(), WaitStatus);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
KeWaitForMultipleObjects(ULONG Count,
                         PVOID Object[],
                         WAIT_TYPE WaitType,
                         KWAIT_REASON WaitReason,
                         KPROCESSOR_MODE WaitMode,
                         BOOLEAN Alertable,
                         PLARGE_INTEGER Timeout,
                         PKWAIT_BLOCK WaitBlockArray)
{
    PKMUTANT CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    ULONG AllObjectsSignaled;
    ULONG WaitIndex;
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    DPRINT("Entering KeWaitForMultipleObjects(Count %lu Object[] %p) "
           "PsGetCurrentThread() %x, Timeout %x\n",
           Count, Object, PsGetCurrentThread(), Timeout);

    /* Set the Current Thread */
    CurrentThread = KeGetCurrentThread();

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)
    {
        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;
    }
    else
    {
        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Make sure the Wait Count is valid for the Thread and Maximum Wait Objects */
    if (!WaitBlockArray)
    {
        /* Check in regards to the Thread Object Limit */
        if (Count > THREAD_WAIT_OBJECTS) KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);

        /* Use the Thread's Wait Block */
        WaitBlockArray = &CurrentThread->WaitBlock[0];
    }
    else
    {
        /* Using our own Block Array. Check in regards to System Object Limit */
        if (Count > MAXIMUM_WAIT_OBJECTS) KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
    }

    /* Start the actual Loop */
    do
    {
        /* Check if a kernel APC is pending and we were below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            (CurrentThread->WaitIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher */
            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
            goto SkipWait;
        }

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
                        /* This is a Wait Any, so just unwait this and exit */
                        if (CurrentObject->Header.SignalState != (LONG)MINLONG)
                        {
                            /* Normal signal state, so unwait it and return */
                            KiSatisfyMutantWait(CurrentObject, CurrentThread);
                            WaitStatus = CurrentThread->WaitStatus | WaitIndex;
                            goto DontWait;
                        }
                        else
                        {
                            /* According to wasm.ru, we must raise this exception (tested and true) */
                            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
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
                        KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
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
            KiSatisifyMultipleObjectWaits(WaitBlock);
            WaitStatus = CurrentThread->WaitStatus;
            goto DontWait;
        }

        /* Make sure we can satisfy the Alertable request */
        if (KiCheckAlertability(Alertable, CurrentThread, WaitMode, &WaitStatus)) break;

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
        }
        while (WaitBlock != WaitBlockArray);

        /* Handle Kernel Queues */
        if (CurrentThread->Queue)
        {
            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Setup the wait information */
        CurrentThread->Alertable = Alertable;
        CurrentThread->WaitMode = WaitMode;
        CurrentThread->WaitReason = WaitReason;
        CurrentThread->WaitTime = ((PLARGE_INTEGER)&KeTickCount)->LowPart;
        CurrentThread->State = Waiting;

        /* Find a new thread to run */
        DPRINT("Swapping threads\n");
        WaitStatus = KiSwapThread();

        /* Check if we were executing an APC */
        DPRINT("Thread is back\n");
        if (WaitStatus != STATUS_KERNEL_APC)
        {
            /* Return Status */
            return WaitStatus;
        }

        /* Check if we had a timeout */
        if (Timeout)
        {
             /* FIXME: Fixup interval */
        }

        /* Acquire again the lock */
SkipWait:
        DPRINT("Looping again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }
    while (TRUE);

    /* Release the Lock, we are done */
    DPRINT("Returning, %x. Status: %d\n",  KeGetCurrentThread(), WaitStatus);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(CurrentThread);

    /* Release & Return */
    DPRINT("Returning, %x. Status: %d\n. We did not wait.",
            KeGetCurrentThread(), WaitStatus);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return WaitStatus;
}

VOID
FASTCALL
KiWaitTest(PVOID ObjectPointer,
           KPRIORITY Increment)
{
    PLIST_ENTRY WaitEntry;
    PLIST_ENTRY WaitList;
    PKWAIT_BLOCK CurrentWaitBlock;
    PKWAIT_BLOCK NextWaitBlock;
    PKTHREAD WaitThread;
    PKMUTANT FirstObject = ObjectPointer, Object;

    /* Loop the Wait Entries */
    DPRINT("KiWaitTest for Object: %x\n", FirstObject);
    WaitList = &FirstObject->Header.WaitListHead;
    WaitEntry = WaitList->Flink;
    while ((FirstObject->Header.SignalState > 0) && (WaitEntry != WaitList))
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
            DPRINT("Satisfiying a Wait any\n");
            WaitEntry = WaitEntry->Blink;
            KiSatisfyObjectWait(FirstObject, WaitThread);
        }
        else
        {
            /* Everything must be satisfied */
            DPRINT("Checking for a Wait All\n");
            NextWaitBlock = CurrentWaitBlock->NextWaitBlock;

            /* Loop first to make sure they are valid */
            while (NextWaitBlock != CurrentWaitBlock)
            {
                /* Check if the object is signaled */
                Object = NextWaitBlock->Object;
                DPRINT("Checking: %p %d\n",
                        Object, Object->Header.SignalState);
                if (NextWaitBlock->WaitKey != STATUS_TIMEOUT)
                {
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
            DPRINT("Satisfiying a Wait All\n");
            WaitEntry = WaitEntry->Blink;
            KiSatisifyMultipleObjectWaits(CurrentWaitBlock);
        }

        /* All waits satisfied, unwait the thread */
        DPRINT("Unwaiting the Thread\n");
        KiAbortWaitThread(WaitThread, CurrentWaitBlock->WaitKey, Increment);

SkipUnwait:
        /* Next entry */
        WaitEntry = WaitEntry->Flink;
    }

    DPRINT("Done\n");
}

/* Must be called with the dispatcher lock held */
VOID
FASTCALL
KiAbortWaitThread(PKTHREAD Thread,
                  NTSTATUS WaitStatus,
                  KPRIORITY Increment)
{
    PKWAIT_BLOCK WaitBlock;

    /* If we are blocked, we must be waiting on something also */
    DPRINT("KiAbortWaitThread: %x, Status: %x, %x \n",
            Thread, WaitStatus, Thread->WaitBlockList);

    /* Remove the Wait Blocks from the list */
    DPRINT("Removing waits\n");
    WaitBlock = Thread->WaitBlockList;
    do
    {
        /* Remove it */
        DPRINT("Removing Waitblock: %x, %x\n",
                WaitBlock, WaitBlock->NextWaitBlock);
        RemoveEntryList(&WaitBlock->WaitListEntry);

        /* Go to the next one */
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != Thread->WaitBlockList);

    /* Check if there's a Thread Timer */
    if (Thread->Timer.Header.Inserted)
    {
        /* Cancel the Thread Timer with the no-lock fastpath */
        DPRINT("Removing the Thread's Timer\n");
        Thread->Timer.Header.Inserted = FALSE;
        RemoveEntryList(&Thread->Timer.TimerListEntry);
    }

    /* Increment the Queue's active threads */
    if (Thread->Queue)
    {
        DPRINT("Incrementing Queue's active threads\n");
        Thread->Queue->CurrentCount++;
    }

    /* Reschedule the Thread */
    DPRINT("Unblocking the Thread\n");
    KiUnblockThread(Thread, &WaitStatus, 0);
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
KiExitDispatcher(KIRQL OldIrql)
{
    /* If it's the idle thread, dispatch */
    if (!(KeIsExecutingDpc()) &&
        (OldIrql < DISPATCH_LEVEL) &&
        (KeGetCurrentThread()) &&
        (KeGetCurrentThread() == KeGetCurrentPrcb()->IdleThread))
    {
        KiDispatchThreadNoLock(Ready);
    }

    /* Lower irql back */
    KeLowerIrql(OldIrql);
}

/* EOF */
