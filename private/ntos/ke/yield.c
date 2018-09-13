/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    yield.c

Abstract:

    This module implements the function to yield execution for one quantum
    to any other runnable thread.

Author:

    David N. Cutler (davec) 15-Mar-1996

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

NTSTATUS
NtYieldExecution (
    VOID
    )

/*++

Routine Description:

    This function yields execution to any ready thread for up to one
    quantum.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PRKPRCB Prcb;
    KPRIORITY Priority;
    NTSTATUS Status;
    PRKTHREAD Thread;

    //
    // If any other threads are ready, then attempt to yield execution.
    //

    Status = STATUS_NO_YIELD_PERFORMED;
    if (KiReadySummary != 0) {

        //
        // If a thread has not already been selected for execution, then
        // attempt to select another thread for execution.
        //

        Thread = KeGetCurrentThread();
        KiLockDispatcherDatabase(&Thread->WaitIrql);
        Prcb = KeGetCurrentPrcb();
        if (Prcb->NextThread == NULL) {
            Prcb->NextThread = KiFindReadyThread(Thread->NextProcessor, 1);
        }

        //
        // If a new thread has been selected for execution, then switch
        // immediately to the selected thread.
        //

        if (Prcb->NextThread != NULL) {

            //
            // Give the current thread a new quantum, simulate a quantum
            // end, insert the current thread in the appropriate ready list,
            // and switch context to selected thread.
            //

            Thread->Quantum = Thread->ApcState.Process->ThreadQuantum;
            Thread->State = Ready;
            Priority = Thread->Priority;
            if (Priority < LOW_REALTIME_PRIORITY) {
                Priority = Priority - Thread->PriorityDecrement - 1;
                if (Priority < Thread->BasePriority) {
                    Priority = Thread->BasePriority;
                }

                Thread->PriorityDecrement = 0;

            }

            Thread->Priority = (SCHAR)Priority;

            InsertTailList(&KiDispatcherReadyListHead[Priority],
                           &Thread->WaitListEntry);

            SetMember(Priority, KiReadySummary);
            KiSwapThread();
            Status = STATUS_SUCCESS;

        } else {
            KiUnlockDispatcherDatabase(Thread->WaitIrql);
        }
    }

    return Status;
}
