/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS project
 * FILE:            ntoskrnl/ke/wait.c
 * PURPOSE:         Manages non-busy waiting
 *
 * PROGRAMMERS:     Alex Ionescu - Fixes and optimization.
 *                  Gunnar Dalsnes - Implementation
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static KSPIN_LOCK DispatcherDatabaseLock;

/* Tells us if the Timer or Event is a Syncronization or Notification Object */
#define TIMER_OR_EVENT_TYPE 0x7L

/* One of the Reserved Wait Blocks, this one is for the Thread's Timer */
#define TIMER_WAIT_BLOCK 0x3L

/* FUNCTIONS *****************************************************************/

VOID
inline
FASTCALL
KiCheckAlertability(BOOLEAN Alertable,
                    PKTHREAD CurrentThread,
                    KPROCESSOR_MODE WaitMode,
                    PNTSTATUS Status)
{
    /* At this point, we have to do a wait, so make sure we can make the thread Alertable if requested */
    if (Alertable) {

        /* If the Thread is Alerted, set the Wait Status accordingly */
        if (CurrentThread->Alerted[(int)WaitMode]) {

            CurrentThread->Alerted[(int)WaitMode] = FALSE;
            DPRINT("Thread was Alerted\n");
            *Status = STATUS_ALERTED;

        /* If there are User APCs Pending, then we can't really be alertable */
        } else if ((!IsListEmpty(&CurrentThread->ApcState.ApcListHead[UserMode])) &&
                    (WaitMode != KernelMode)) {

            DPRINT("APCs are Pending\n");
            CurrentThread->ApcState.UserApcPending = TRUE;
            *Status = STATUS_USER_APC;
        }

    /* If there are User APCs Pending and we are waiting in usermode, then we must notify the caller */
    } else if ((CurrentThread->ApcState.UserApcPending) && (WaitMode != KernelMode)) {
            DPRINT("APCs are Pending\n");
            *Status = STATUS_USER_APC;
    }
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
    NTSTATUS Status;

    DPRINT("Entering KeDelayExecutionThread\n");

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)  {

        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;

    } else {

        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Use built-in Wait block */
    TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];

    /* Start Wait Loop */
    do {

        /* We are going to wait no matter what (that's the point), so test Alertability */
        KiCheckAlertability(Alertable, CurrentThread, KernelMode, &Status);

        /* Set Timer */
        ThreadTimer = &CurrentThread->Timer;

        /* Setup the Wait Block */
        CurrentThread->WaitBlockList = TimerWaitBlock;
        TimerWaitBlock->Object = (PVOID)ThreadTimer;
        TimerWaitBlock->Thread = CurrentThread;
        TimerWaitBlock->WaitKey = (USHORT)STATUS_TIMEOUT;
        TimerWaitBlock->WaitType = WaitAny;
        TimerWaitBlock->NextWaitBlock = TimerWaitBlock;

        /* Link the timer to this Wait Block */
        InitializeListHead(&ThreadTimer->Header.WaitListHead);
        InsertTailList(&ThreadTimer->Header.WaitListHead, &TimerWaitBlock->WaitListEntry);

        /* Insert the Timer into the Timer Lists and enable it */
        if (!KiInsertTimer(ThreadTimer, *Interval)) {

            /* FIXME: The timer already expired, we should find a new ready thread */
            Status = STATUS_SUCCESS;
            break;
        }

        /* Handle Kernel Queues */
        if (CurrentThread->Queue) {

            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Block the Thread */
        DPRINT("Blocking the Thread: %d, %d, %x\n", Alertable, WaitMode, KeGetCurrentThread());
        KiBlockThread(&Status,
                      Alertable,
                      WaitMode,
                      DelayExecution);

        /* Check if we were executing an APC or if we timed out */
        if (Status != STATUS_KERNEL_APC) {

            /* This is a good thing */
            if (Status == STATUS_TIMEOUT) Status = STATUS_SUCCESS;

            /* Return Status */
            return Status;
        }

        DPRINT("Looping Again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();

    } while (TRUE);

    /* Release the Lock, we are done */
    DPRINT("Returning from KeDelayExecutionThread(), %x. Status: %d\n", KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;
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
    PDISPATCHER_HEADER CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    NTSTATUS Status;
    NTSTATUS WaitStatus;

    DPRINT("Entering KeWaitForSingleObject\n");

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)  {

        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;

    } else {

        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Start the actual Loop */
    do {

        /* Get the current Wait Status */
        WaitStatus = CurrentThread->WaitStatus;

        /* Append wait block to the KTHREAD wait block list */
        CurrentThread->WaitBlockList = WaitBlock = &CurrentThread->WaitBlock[0];

        /* Get the Current Object */
        CurrentObject = (PDISPATCHER_HEADER)Object;

        /* Check if the Object is Signaled */
        if (KiIsObjectSignaled(CurrentObject, CurrentThread)) {

            /* Just unwait this guy and exit */
            if (CurrentObject->SignalState != (LONG)MINLONG) {

                /* It has a normal signal state, so unwait it and return */
                KiSatisfyObjectWait(CurrentObject, CurrentThread);
                Status = STATUS_WAIT_0;
                goto DontWait;

            } else {

                /* Is this a Mutant? */
                if (CurrentObject->Type == MutantObject) {

                    /* According to wasm.ru, we must raise this exception (tested and true) */
                    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
                    ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                }
            }
        }

        /* Set up the Wait Block */
        WaitBlock->Object = CurrentObject;
        WaitBlock->Thread = CurrentThread;
        WaitBlock->WaitKey = (USHORT)(STATUS_WAIT_0);
        WaitBlock->WaitType = WaitAny;
        WaitBlock->NextWaitBlock = WaitBlock;

        /* Make sure we can satisfy the Alertable request */
        KiCheckAlertability(Alertable, CurrentThread, WaitMode, &Status);

        /* Set the Wait Status */
        CurrentThread->WaitStatus = Status;

        /* Enable the Timeout Timer if there was any specified */
        if (Timeout != NULL) {

            /* However if 0 timeout was specified, then we must fail since we need to peform a wait */
            if (!Timeout->QuadPart) {

                /* Return a timeout */
                Status = STATUS_TIMEOUT;
                goto DontWait;
            }

            /* Point to Timer Wait Block and Thread Timer */
            TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];
            ThreadTimer = &CurrentThread->Timer;

            /* Connect the Timer Wait Block */
            WaitBlock->NextWaitBlock = TimerWaitBlock;

            /* Set up the Timer Wait Block */
            TimerWaitBlock->Object = (PVOID)ThreadTimer;
            TimerWaitBlock->Thread = CurrentThread;
            TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
            TimerWaitBlock->WaitType = WaitAny;
            TimerWaitBlock->NextWaitBlock = WaitBlock;

            /* Link the timer to this Wait Block */
            InitializeListHead(&ThreadTimer->Header.WaitListHead);
            InsertTailList(&ThreadTimer->Header.WaitListHead, &TimerWaitBlock->WaitListEntry);

            /* Insert the Timer into the Timer Lists and enable it */
            if (!KiInsertTimer(ThreadTimer, *Timeout)) {

                /* Return a timeout if we couldn't insert the timer for some reason */
                Status = STATUS_TIMEOUT;
                goto DontWait;
            }
        }

        /* Link the Object to this Wait Block */
        InsertTailList(&CurrentObject->WaitListHead, &WaitBlock->WaitListEntry);

        /* Handle Kernel Queues */
        if (CurrentThread->Queue) {

            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Block the Thread */
        DPRINT("Blocking the Thread: %d, %d, %d, %x\n", Alertable, WaitMode, WaitReason, KeGetCurrentThread());
        KiBlockThread(&Status,
                      Alertable,
                      WaitMode,
                      (UCHAR)WaitReason);

        /* Check if we were executing an APC */
        if (Status != STATUS_KERNEL_APC) {

            /* Return Status */
            return Status;
        }

        DPRINT("Looping Again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();

    } while (TRUE);

    /* Release the Lock, we are done */
    DPRINT("Returning from KeWaitForMultipleObjects(), %x. Status: %d\n", KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(CurrentThread);

    /* Release & Return */
    DPRINT("Returning from KeWaitForMultipleObjects(), %x. Status: %d\n. We did not wait.", KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
KeWaitForMultipleObjects(ULONG Count,
                         PVOID Object[],
                         WAIT_TYPE WaitType,
                         KWAIT_REASON WaitReason,
                         KPROCESSOR_MODE WaitMode,
                         BOOLEAN Alertable,
                         PLARGE_INTEGER Timeout,
                         PKWAIT_BLOCK WaitBlockArray)
{
    PDISPATCHER_HEADER CurrentObject;
    PKWAIT_BLOCK WaitBlock;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER ThreadTimer;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    ULONG AllObjectsSignaled;
    ULONG WaitIndex;
    NTSTATUS Status;
    NTSTATUS WaitStatus;

    DPRINT("Entering KeWaitForMultipleObjects(Count %lu Object[] %p) "
            "PsGetCurrentThread() %x, Timeout %x\n", Count, Object, PsGetCurrentThread(), Timeout);

    /* Set the Current Thread */
    CurrentThread = KeGetCurrentThread();

    /* Check if the lock is already held */
    if (CurrentThread->WaitNext)  {

        /* Lock is held, disable Wait Next */
        DPRINT("Lock is held\n");
        CurrentThread->WaitNext = FALSE;

    } else {

        /* Lock not held, acquire it */
        DPRINT("Lock is not held, acquiring\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();
    }

    /* Make sure the Wait Count is valid for the Thread and Maximum Wait Objects */
    if (!WaitBlockArray) {

        /* Check in regards to the Thread Object Limit */
        if (Count > THREAD_WAIT_OBJECTS) {

            KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }

        /* Use the Thread's Wait Block */
        WaitBlockArray = &CurrentThread->WaitBlock[0];

    } else {

        /* Using our own Block Array. Check in regards to System Object Limit */
        if (Count > MAXIMUM_WAIT_OBJECTS) {

            KEBUGCHECK(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }
    }

    /* Start the actual Loop */
    do {

        /* Get the current Wait Status */
        WaitStatus = CurrentThread->WaitStatus;

        /* Append wait block to the KTHREAD wait block list */
        CurrentThread->WaitBlockList = WaitBlock = WaitBlockArray;

        /* Check if the wait is (already) satisfied */
        AllObjectsSignaled = TRUE;

        /* First, we'll try to satisfy the wait directly */
        for (WaitIndex = 0; WaitIndex < Count; WaitIndex++) {

            /* Get the Current Object */
            CurrentObject = (PDISPATCHER_HEADER)Object[WaitIndex];

            /* Check if the Object is Signaled */
            if (KiIsObjectSignaled(CurrentObject, CurrentThread)) {

                /* Check what kind of wait this is */
                if (WaitType == WaitAny) {

                    /* This is a Wait Any, so just unwait this guy and exit */
                    if (CurrentObject->SignalState != (LONG)MINLONG) {

                        /* It has a normal signal state, so unwait it and return */
                        KiSatisfyObjectWait(CurrentObject, CurrentThread);
                        Status = STATUS_WAIT_0 | WaitIndex;
                        goto DontWait;

                    } else {

                        /* Is this a Mutant? */
                        if (CurrentObject->Type == MutantObject) {

                            /* According to wasm.ru, we must raise this exception (tested and true) */
                            KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
                            ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                        }
                    }
                }

            } else {

                /* One of the objects isn't signaled... if this is a WaitAll, we will fail later */
                AllObjectsSignaled = FALSE;
            }

            /* Set up a Wait Block for this Object */
            WaitBlock->Object = CurrentObject;
            WaitBlock->Thread = CurrentThread;
            WaitBlock->WaitKey = (USHORT)(STATUS_WAIT_0 + WaitIndex);
            WaitBlock->WaitType = (USHORT)WaitType;
            WaitBlock->NextWaitBlock = WaitBlock + 1;

            /* Move to the next Wait Block */
            WaitBlock = WaitBlock->NextWaitBlock;
        }

        /* Return to the Root Wait Block */
        WaitBlock--;
        WaitBlock->NextWaitBlock = WaitBlockArray;

        /* Check if this is a Wait All and all the objects are signaled */
        if ((WaitType == WaitAll) && (AllObjectsSignaled)) {

            /* Return to the Root Wait Block */
            WaitBlock = CurrentThread->WaitBlockList;

            /* Satisfy their Waits and return to the caller */
            KiSatisifyMultipleObjectWaits(WaitBlock);
            Status = STATUS_WAIT_0;
            goto DontWait;
        }

        /* Make sure we can satisfy the Alertable request */
        KiCheckAlertability(Alertable, CurrentThread, WaitMode, &Status);

        /* Set the Wait Status */
        CurrentThread->WaitStatus = Status;

        /* Enable the Timeout Timer if there was any specified */
        if (Timeout != NULL) {

            /* However if 0 timeout was specified, then we must fail since we need to peform a wait */
            if (!Timeout->QuadPart) {

                /* Return a timeout */
                Status = STATUS_TIMEOUT;
                goto DontWait;
            }

            /* Point to Timer Wait Block and Thread Timer */
            TimerWaitBlock = &CurrentThread->WaitBlock[TIMER_WAIT_BLOCK];
            ThreadTimer = &CurrentThread->Timer;

            /* Connect the Timer Wait Block */
            WaitBlock->NextWaitBlock = TimerWaitBlock;

            /* Set up the Timer Wait Block */
            TimerWaitBlock->Object = (PVOID)ThreadTimer;
            TimerWaitBlock->Thread = CurrentThread;
            TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
            TimerWaitBlock->WaitType = WaitAny;
            TimerWaitBlock->NextWaitBlock = WaitBlockArray;

            /* Link the timer to this Wait Block */
            InitializeListHead(&ThreadTimer->Header.WaitListHead);

            /* Insert the Timer into the Timer Lists and enable it */
            if (!KiInsertTimer(ThreadTimer, *Timeout)) {

                /* Return a timeout if we couldn't insert the timer for some reason */
                Status = STATUS_TIMEOUT;
                goto DontWait;
            }
        }

        /* Insert into Object's Wait List*/
        WaitBlock = CurrentThread->WaitBlockList;
        do {

            /* Get the Current Object */
            CurrentObject = WaitBlock->Object;

            /* Link the Object to this Wait Block */
            InsertTailList(&CurrentObject->WaitListHead, &WaitBlock->WaitListEntry);

            /* Move to the next Wait Block */
            WaitBlock = WaitBlock->NextWaitBlock;
        } while (WaitBlock != WaitBlockArray);

        /* Handle Kernel Queues */
        if (CurrentThread->Queue) {

            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Block the Thread */
        DPRINT("Blocking the Thread: %d, %d, %d, %x\n", Alertable, WaitMode, 
                WaitReason, KeGetCurrentThread());
        KiBlockThread(&Status,
                      Alertable,
                      WaitMode,
                      (UCHAR)WaitReason);

        /* Check if we were executing an APC */
        DPRINT("Thread is back\n");
        if (Status != STATUS_KERNEL_APC) {

            /* Return Status */
            return Status;
        }

        DPRINT("Looping Again\n");
        CurrentThread->WaitIrql = KeAcquireDispatcherDatabaseLock();

    } while (TRUE);

    /* Release the Lock, we are done */
    DPRINT("Returning, %x. Status: %d\n",  KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;

DontWait:
    /* Adjust the Quantum */
    KiAdjustQuantumThread(CurrentThread);

    /* Release & Return */
    DPRINT("Returning, %x. Status: %d\n. We did not wait.", 
            KeGetCurrentThread(), Status);
    KeReleaseDispatcherDatabaseLock(CurrentThread->WaitIrql);
    return Status;
}

VOID
FASTCALL
KiSatisfyObjectWait(PDISPATCHER_HEADER Object,
                    PKTHREAD Thread)

{
    /* Special case for Mutants */
    if (Object->Type == MutantObject) {

        /* Decrease the Signal State */
        Object->SignalState--;

        /* Check if it's now non-signaled */
        if (Object->SignalState == 0) {

            /* Set the Owner Thread */
            ((PKMUTANT)Object)->OwnerThread = Thread;

            /* Disable APCs if needed */
            Thread->KernelApcDisable -= ((PKMUTANT)Object)->ApcDisable;

            /* Check if it's abandoned */
            if (((PKMUTANT)Object)->Abandoned) {

                /* Unabandon it */
                ((PKMUTANT)Object)->Abandoned = FALSE;

                /* Return Status */
                Thread->WaitStatus = STATUS_ABANDONED;
            }

            /* Insert it into the Mutant List */
            InsertHeadList(&Thread->MutantListHead, &((PKMUTANT)Object)->MutantListEntry);
        }

    } else if ((Object->Type & TIMER_OR_EVENT_TYPE) == EventSynchronizationObject) {

        /* These guys (Syncronization Timers and Events) just get un-signaled */
        Object->SignalState = 0;

    } else if (Object->Type == SemaphoreObject) {

        /* These ones can have multiple signalings, so we only decrease it */
        Object->SignalState--;
    }
}

VOID
FASTCALL
KiWaitTest(PDISPATCHER_HEADER Object,
           KPRIORITY Increment)
{
    PLIST_ENTRY WaitEntry;
    PLIST_ENTRY WaitList;
    PKWAIT_BLOCK CurrentWaitBlock;
    PKWAIT_BLOCK NextWaitBlock;
    PKTHREAD WaitThread;

    /* Loop the Wait Entries */
    DPRINT("KiWaitTest for Object: %x\n", Object);
    WaitList = &Object->WaitListHead;
    WaitEntry = WaitList->Flink;
    while ((WaitEntry != WaitList) && (Object->SignalState > 0)) {

        /* Get the current wait block */
        CurrentWaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        WaitThread = CurrentWaitBlock->Thread;

        /* Check the current Wait Mode */
        if (CurrentWaitBlock->WaitType == WaitAny) {

            /* Easy case, satisfy only this wait */
            DPRINT("Satisfiying a Wait any\n");
            WaitEntry = WaitEntry->Blink;
            KiSatisfyObjectWait(Object, WaitThread);

        } else {

            /* Everything must be satisfied */
            DPRINT("Checking for a Wait All\n");
            NextWaitBlock = CurrentWaitBlock->NextWaitBlock;

            /* Loop first to make sure they are valid */
            while (NextWaitBlock != CurrentWaitBlock) {

                /* Check if the object is signaled */
                DPRINT("Checking: %x %d\n", NextWaitBlock->Object, Object->SignalState);
                if (!KiIsObjectSignaled(NextWaitBlock->Object, WaitThread)) {

                    /* It's not, move to the next one */
                    DPRINT("One of the object is non-signaled, sorry.\n");
                    goto SkipUnwait;
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
    DPRINT("KiAbortWaitThread: %x, Status: %x, %x \n", Thread, WaitStatus, Thread->WaitBlockList);
    ASSERT((Thread->State == Waiting) == (Thread->WaitBlockList != NULL));

    /* Remove the Wait Blocks from the list */
    DPRINT("Removing waits\n");
    WaitBlock = Thread->WaitBlockList;
    do {

        /* Remove it */
        DPRINT("Removing Waitblock: %x, %x\n", WaitBlock, WaitBlock->NextWaitBlock);
        RemoveEntryList(&WaitBlock->WaitListEntry);

        /* Go to the next one */
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != Thread->WaitBlockList);

    /* Check if there's a Thread Timer */
    if (Thread->Timer.Header.Inserted) {

        /* Cancel the Thread Timer with the no-lock fastpath */
        DPRINT("Removing the Thread's Timer\n");
        Thread->Timer.Header.Inserted = FALSE;
        RemoveEntryList(&Thread->Timer.TimerListEntry);
    }

    /* Increment the Queue's active threads */
    if (Thread->Queue) {

        DPRINT("Incrementing Queue's active threads\n");
        Thread->Queue->CurrentCount++;
    }

    /* Reschedule the Thread */
    DPRINT("Unblocking the Thread\n");
    KiUnblockThread(Thread, &WaitStatus, 0);
}

BOOLEAN
inline
FASTCALL
KiIsObjectSignaled(PDISPATCHER_HEADER Object,
                   PKTHREAD Thread)
{
    /* Mutants are...well...mutants! */
   if (Object->Type == MutantObject) {

        /*
         * Because Cutler hates mutants, they are actually signaled if the Signal State is <= 0
         * Well, only if they are recursivly acquired (i.e if we own it right now).
         * Of course, they are also signaled if their signal state is 1.
         */
        if ((Object->SignalState <= 0 && ((PKMUTANT)Object)->OwnerThread == Thread) ||
            (Object->SignalState == 1)) {

            /* Signaled Mutant */
            return (TRUE);

        } else {

            /* Unsignaled Mutant */
            return (FALSE);
        }
    }

    /* Any other object is not a mutated freak, so let's use logic */
   return (!Object->SignalState <= 0);
}

VOID
inline
FASTCALL
KiSatisifyMultipleObjectWaits(PKWAIT_BLOCK WaitBlock)
{
    PKWAIT_BLOCK FirstBlock = WaitBlock;
    PKTHREAD WaitThread = WaitBlock->Thread;

    /* Loop through all the Wait Blocks, and wake each Object */
    do {

        /* Wake the Object */
        KiSatisfyObjectWait(WaitBlock->Object, WaitThread);
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != FirstBlock);
}

VOID
inline
FASTCALL
KeInitializeDispatcherHeader(DISPATCHER_HEADER* Header,
                             ULONG Type,
                             ULONG Size,
                             ULONG SignalState)
{
    Header->Type = (UCHAR)Type;
    Header->Absolute = 0;
    Header->Inserted = 0;
    Header->Size = (UCHAR)Size;
    Header->SignalState = SignalState;
    InitializeListHead(&(Header->WaitListHead));
}

KIRQL
inline
FASTCALL
KeAcquireDispatcherDatabaseLock(VOID)
{
    KIRQL OldIrql;

    KeAcquireSpinLock (&DispatcherDatabaseLock, &OldIrql);
    return OldIrql;
}

VOID
inline
FASTCALL
KeAcquireDispatcherDatabaseLockAtDpcLevel(VOID)
{
    KeAcquireSpinLockAtDpcLevel (&DispatcherDatabaseLock);
}

VOID
inline
FASTCALL
KeInitializeDispatcher(VOID)
{
    /* Initialize the Dispatcher Lock */
    KeInitializeSpinLock(&DispatcherDatabaseLock);
}

VOID
inline
FASTCALL
KeReleaseDispatcherDatabaseLock(KIRQL OldIrql)
{
    /* If it's the idle thread, dispatch */
    if (!KeIsExecutingDpc() && OldIrql < DISPATCH_LEVEL && KeGetCurrentThread() != NULL &&
        KeGetCurrentThread() == KeGetCurrentPrcb()->IdleThread) {

        KiDispatchThreadNoLock(Ready);
        KeLowerIrql(OldIrql);

    } else {

        /* Just release the spin lock */
        KeReleaseSpinLock(&DispatcherDatabaseLock, OldIrql);
    }
}

VOID
inline
FASTCALL
KeReleaseDispatcherDatabaseLockFromDpcLevel(VOID)
{
    KeReleaseSpinLockFromDpcLevel(&DispatcherDatabaseLock);
}

/* EOF */
