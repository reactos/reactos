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
    PKTHREAD CurrentThread = KeGetCurrentThread();
    PKWAIT_BLOCK GateWaitBlock;
    NTSTATUS Status;
    ASSERT_GATE(Gate);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Start wait loop */
    do
    {
        /* Acquire the APC lock */
        KiAcquireApcLock(CurrentThread, &ApcLock);

        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            !(CurrentThread->SpecialApcDisable) &&
            (ApcLock.OldIrql < APC_LEVEL))
        {
            /* Release the lock, this will fire the APC */
            KiReleaseApcLock(&ApcLock);
        }
        else
        {
            /* Check if it's already signaled */
            if (Gate->Header.SignalState)
            {
                /* Unsignal it */
                Gate->Header.SignalState = 0;

                /* Release the APC lock and return */
                KiReleaseApcLock(&ApcLock);
                return;
            }

            /* Setup a Wait Block */
            GateWaitBlock = &CurrentThread->WaitBlock[0];
            GateWaitBlock->Object = (PVOID)Gate;
            GateWaitBlock->Thread = CurrentThread;

            /* Set the Thread Wait Data */
            CurrentThread->WaitMode = WaitMode;
            CurrentThread->WaitReason = WaitReason;
            CurrentThread->WaitIrql = ApcLock.OldIrql;
            CurrentThread->State = GateWait;
            CurrentThread->GateObject = Gate;

            /* Insert into the Wait List */
            InsertTailList(&Gate->Header.WaitListHead,
                           &GateWaitBlock->WaitListEntry);

            /* Handle Kernel Queues */
            if (CurrentThread->Queue) KiWakeQueue(CurrentThread->Queue);

            /* Release the APC lock but stay at DPC level */
            KiReleaseApcLockFromDpcLevel(&ApcLock);

            /* Find a new thread to run */
            Status = KiSwapThread(CurrentThread, KeGetCurrentPrcb());

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
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    ASSERT_GATE(Gate);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Raise to synch level */
    OldIrql = KeRaiseIrqlToSynchLevel();

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

        /* Remove it */
        RemoveEntryList(&WaitBlock->WaitListEntry);

        /* Get the Associated thread */
        WaitThread = WaitBlock->Thread;

        /* Increment the Queue's active threads */
        if (WaitThread->Queue) WaitThread->Queue->CurrentCount++;

        /* FIXME: This isn't really correct!!! */
        KiAbortWaitThread(WaitThread, WaitStatus, EVENT_INCREMENT);
    }

    /* Exit the dispatcher */
    KiExitDispatcher(OldIrql);
}

/* EOF */
