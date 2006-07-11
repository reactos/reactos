/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/gate.c
 * PURPOSE:         Implements the Gate Dispatcher Object
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#define NTDDI_VERSION NTDDI_WS03
#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID
FASTCALL
KeInitializeGate(PKGATE Gate)
{
    DPRINT("KeInitializeGate(Gate %x)\n", Gate);

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
    KIRQL OldIrql;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    PKWAIT_BLOCK GateWaitBlock;
    NTSTATUS Status;
    ASSERT_GATE(Gate);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Start wait loop */
    do
    {
        /* Lock the dispatcher */
        OldIrql = KeAcquireDispatcherDatabaseLock();

        /* Check if a kernel APC is pending and we're below APC_LEVEL */
        if ((CurrentThread->ApcState.KernelApcPending) &&
            !(CurrentThread->SpecialApcDisable) && (OldIrql < APC_LEVEL))
        {
            /* Unlock the dispatcher, this will fire the APC */
            KeReleaseDispatcherDatabaseLock(OldIrql);
        }
        else
        {
            /* Check if it's already signaled */
            if (Gate->Header.SignalState)
            {
                /* Unsignal it */
                Gate->Header.SignalState = 0;

                /* Unlock the Queue and return */
                KeReleaseDispatcherDatabaseLock(OldIrql);
                return;
            }

            /* Setup a Wait Block */
            GateWaitBlock = &CurrentThread->WaitBlock[0];
            GateWaitBlock->Object = (PVOID)Gate;
            GateWaitBlock->Thread = CurrentThread;

            /* Set the Thread Wait Data */
            CurrentThread->WaitMode = WaitMode;
            CurrentThread->WaitReason = WaitReason;
            CurrentThread->WaitIrql = OldIrql;
            CurrentThread->State = GateWait;
            CurrentThread->GateObject = Gate;

            /* Insert into the Wait List */
            InsertTailList(&Gate->Header.WaitListHead,
                           &GateWaitBlock->WaitListEntry);

            /* Handle Kernel Queues */
            if (CurrentThread->Queue) KiWakeQueue(CurrentThread->Queue);

            /* Find a new thread to run */
            Status = KiSwapThread();

            /* Check if we were executing an APC */
            if (Status != STATUS_KERNEL_APC) return;
        }
    } while (TRUE);
}

VOID
FASTCALL
KeSignalGateBoostPriority(PKGATE Gate)
{
    PKTHREAD WaitThread;
    PKWAIT_BLOCK WaitBlock;
    KIRQL OldIrql;
    NTSTATUS WaitStatus = STATUS_SUCCESS;
    ASSERT_GATE(Gate);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Acquire Dispatcher Database Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Make sure we're not already signaled or that the list is empty */
    if (Gate->Header.SignalState) goto quit;

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

        /* Reschedule the Thread */
        KiUnblockThread(WaitThread, &WaitStatus, EVENT_INCREMENT);
    }

quit:
    /* Release the Dispatcher Database Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/* EOF */
