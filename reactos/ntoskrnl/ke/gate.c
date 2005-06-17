/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/gate.c
 * PURPOSE:         Implements the Gate Dispatcher Object
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

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
KeWaitForGate(PKGATE Gate,
              KWAIT_REASON WaitReason,
              KPROCESSOR_MODE WaitMode)
{
    KIRQL OldIrql;
    PKTHREAD CurrentThread = KeGetCurrentThread();
    PKWAIT_BLOCK GateWaitBlock;
    NTSTATUS Status;

    DPRINT("KeWaitForGate(Gate %x)\n", Gate);

    do
    {
        /* Lock the APC Queue */
        OldIrql = KeAcquireDispatcherDatabaseLock();
        
        /* Check if it's already signaled */
        if (!Gate->Header.SignalState)
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
        CurrentThread->WaitReason = WaitReason;
        CurrentThread->WaitMode = WaitMode;
        CurrentThread->WaitIrql = OldIrql;
        CurrentThread->GateObject = Gate;

        /* Insert into the Wait List */
        InsertTailList(&Gate->Header.WaitListHead, &GateWaitBlock->WaitListEntry);

        /* Handle Kernel Queues */
        if (CurrentThread->Queue)
        {
            DPRINT("Waking Queue\n");
            KiWakeQueue(CurrentThread->Queue);
        }

        /* Block the Thread */
        DPRINT("Blocking the Thread: %x\n", CurrentThread);
        KiBlockThread(&Status,
                      CurrentThread->Alertable,
                      WaitMode,
                      WaitReason);

        /* Check if we were executing an APC */
        if (Status != STATUS_KERNEL_APC) return;

        DPRINT("Looping Again\n");
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

    DPRINT("KeSignalGateBoostPriority(EveGate %x)\n", Gate);

    /* Acquire Dispatcher Database Lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Make sure we're not already signaled or that the list is empty */
    if (Gate->Header.SignalState) goto quit;

    /* If our wait list is empty, then signal the event and return */
    if (IsListEmpty(&Gate->Header.WaitListHead))
    {
        Gate->Header.SignalState = 1;
        goto quit;
    }

    /* Get WaitBlock */
    WaitBlock = CONTAINING_RECORD(Gate->Header.WaitListHead.Flink,
                                  KWAIT_BLOCK,
                                  WaitListEntry);
    /* Remove it */
    RemoveEntryList(&WaitBlock->WaitListEntry);

    /* Get the Associated thread */
    WaitThread = WaitBlock->Thread;

    /* Increment the Queue's active threads */
    if (WaitThread->Queue)
    {
        DPRINT("Incrementing Queue's active threads\n");
        WaitThread->Queue->CurrentCount++;
    }

    /* Reschedule the Thread */
    DPRINT("Unblocking the Thread\n");
    KiUnblockThread(WaitThread, &WaitStatus, EVENT_INCREMENT);
    return;

quit:
    /* Release the Dispatcher Database Lock */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/* EOF */
