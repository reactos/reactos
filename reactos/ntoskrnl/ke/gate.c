/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/gate.c
 * PURPOSE:         Implements the Gate Dispatcher Object
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NTDDI_VERSION NTDDI_WS03
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID
FASTCALL
KeInitializeGate(IN PKGATE Gate)
{
    /* Initialize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Gate->Header,
                                 GateObject,
                                 sizeof(Gate) / sizeof(ULONG),
                                 0);
}

VOID
FASTCALL
KeWaitForGate(IN PKGATE Gate,
              IN KWAIT_REASON WaitReason,
              IN KPROCESSOR_MODE WaitMode)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    PKTHREAD Thread = KeGetCurrentThread();
    PKWAIT_BLOCK GateWaitBlock;
    NTSTATUS Status;
    PKQUEUE Queue;
    ASSERT_GATE(Gate);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    ASSERT(FALSE);

    /* Start wait loop */
    do
    {
        /* Acquire the APC lock */
        KiAcquireApcLock(Thread, &ApcLock);

        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((Thread->ApcState.KernelApcPending) &&
            !(Thread->SpecialApcDisable) &&
            (ApcLock.OldIrql < APC_LEVEL))
        {
            /* Release the lock, this will fire the APC */
            KiReleaseApcLock(&ApcLock);
        }
        else
        {
            /* Check if we have a queue and lock the dispatcher if so */
            Queue = Thread->Queue;
            if (Queue) KiAcquireDispatcherLockAtDpcLevel();

            /* Lock the thread */
            KiAcquireThreadLock(Thread);

            /* Lock the gate */
            KiAcquireDispatcherObject(&Gate->Header);

            /* Check if it's already signaled */
            if (Gate->Header.SignalState)
            {
                /* Unsignal it */
                Gate->Header.SignalState = 0;

                /* Release the gate and thread locks */
                KiReleaseDispatcherObject(&Gate->Header);
                KiReleaseThreadLock(Thread);

                /* Release the APC lock and return */
                KiReleaseApcLock(&ApcLock);
                return;
            }

            /* Setup a Wait Block */
            GateWaitBlock = &Thread->WaitBlock[0];
            GateWaitBlock->Object = (PVOID)Gate;
            GateWaitBlock->Thread = Thread;

            /* Set the Thread Wait Data */
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = WaitReason;
            Thread->WaitIrql = ApcLock.OldIrql;
            Thread->State = GateWait;
            Thread->GateObject = Gate;

            /* Insert into the Wait List */
            InsertTailList(&Gate->Header.WaitListHead,
                           &GateWaitBlock->WaitListEntry);

            /* Release the gate lock */
            KiReleaseDispatcherObject(&Gate->Header);

            /* Set swap busy */
            KiSetThreadSwapBusy(Thread);

            /* Release the thread lock */
            KiReleaseThreadLock(Thread);

            /* Check if we had a queue */
            if (Queue)
            {
                /* Wake it up */
                KiActivateWaiterQueue(Queue);

                /* Release the dispatcher lock */
                KiReleaseDispatcherLockFromDpcLevel();
            }

            /* Release the APC lock but stay at DPC level */
            KiReleaseApcLockFromDpcLevel(&ApcLock);

            /* Find a new thread to run */
            Status = KiSwapThread(Thread, KeGetCurrentPrcb());

            /* Check if we were executing an APC */
            if (Status != STATUS_KERNEL_APC) return;
        }
    } while (TRUE);
}

VOID
FASTCALL
KeSignalGateBoostPriority(IN PKGATE Gate)
{
    PKTHREAD WaitThread;
    PKWAIT_BLOCK WaitBlock;
    KIRQL OldIrql;
    ASSERT_GATE(Gate);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    ASSERT(FALSE);

    /* Start entry loop */
    for (;;)
    {
        /* Raise to synch level */
        OldIrql = KeRaiseIrqlToSynchLevel();

        /* Lock the gate */
        KiAcquireDispatcherObject(&Gate->Header);

        /* Make sure we're not already signaled or that the list is empty */
        if (Gate->Header.SignalState)
        {
            /* Lower IRQL and quit */
            KeLowerIrql(OldIrql);
            return;
        }

        /* Check if our wait list is empty */
        if (IsListEmpty(&Gate->Header.WaitListHead))
        {
            /* It is, so signal the event */
            Gate->Header.SignalState = 1;
        }
        else
        {
            /* Get WaitBlock */
            WaitBlock = CONTAINING_RECORD(Gate->Header.WaitListHead.Flink,
                                          KWAIT_BLOCK,
                                          WaitListEntry);

            /* Get the Associated thread */
            WaitThread = WaitBlock->Thread;

            /* Check to see if the waiting thread is locked */
            if (!KiTryThreadLock(WaitThread))
            {
                /* Unlock the gate */
                KiReleaseDispatcherObject(&Gate->Header);

                /* Lower IRQL and loop again */
                KeLowerIrql(OldIrql);
                continue;
            }

            /* Remove it */
            RemoveEntryList(&WaitBlock->WaitListEntry);

            /* Clear wait status */
            WaitThread->WaitStatus = 0;

            /* Set state and CPU */
            WaitThread->State = DeferredReady;
            WaitThread->DeferredProcessor = KeGetCurrentPrcb()->Number;

            /* Release the gate lock */
            KiReleaseDispatcherObject(&Gate->Header);

            /* Release the thread lock */
            KiReleaseThreadLock(WaitThread);

            /* FIXME: Boosting */

            /* Check if we have a queue */
            if (WaitThread->Queue)
            {
                /* Acquire the dispatcher lock */
                KiAcquireDispatcherLockAtDpcLevel();

                /* Check if we still have one */
                if (WaitThread->Queue)
                {
                    /* Increment active threads */
                    WaitThread->Queue->CurrentCount++;
                }

                /* Release lock */
                KiReleaseDispatcherLockFromDpcLevel();
            }

            /* Make the thread ready */
            KiReadyThread(WaitThread);

            /* Exit the dispatcher */
            KiExitDispatcher(OldIrql);
        }
    }

    /* If we got here, then there's no rescheduling. */
    KiReleaseDispatcherObject(&Gate->Header);
    KeLowerIrql(OldIrql);
}

/* EOF */
