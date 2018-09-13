/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    xxmpipi.c

Abstract:

    This module implements MIPS specific MP routine.

Author:

    David N. Cutler 24-Apr-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiRestoreProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function moves processor register state from the current
    processor context structure in the processor block to the
    specified trap and exception frames.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Get the address of the current processor block and move the
    // specified register state from the processor context structure
    // to the specified trap and exception frames
    //

    Prcb = KeGetCurrentPrcb();
    KeContextToKframes(TrapFrame,
                       ExceptionFrame,
                       &Prcb->ProcessorState.ContextFrame,
                       CONTEXT_FULL,
                       KernelMode);

    return;
}

VOID
KiSaveProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function moves processor register state from the specified trap
    and exception frames to the processor context structure in the current
    processor block.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Get the address of the current processor block and move the
    // specified register state from specified trap and exception
    // frames to the current processor context structure.
    //

    Prcb = KeGetCurrentPrcb();
    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame,
                         ExceptionFrame,
                         &Prcb->ProcessorState.ContextFrame);

    //
    // Save the current processor control state.
    //

    KiSaveProcessorControlState(&Prcb->ProcessorState);
    return;
}

VOID
KiSaveProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    )

/*++

Routine Description:

    This routine saves the processor's control state for debugger.

Arguments:

    ProcessorState (a0) - Supplies a pointer to the processor state.

Return Value:

    None.

--*/

{

    ULONG Index;

    //
    // Read Tb entries and store in the processor state structure.
    //

    for (Index = 0; Index < KeNumberTbEntries; Index += 1) {
        KiReadEntryTb(Index, &ProcessorState->TbEntry[Index]);
    }

    return;
}

BOOLEAN
KiIpiServiceRoutine (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:


    This function is called at IPI_LEVEL to process any outstanding
    interprocess request for the current processor.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame

Return Value:

    A value of TRUE is returned, if one of more requests were service.
    Otherwise, FALSE is returned.

--*/

{

    ULONG RequestSummary;

    //
    // Process any outstanding interprocessor requests.
    //

    RequestSummary = KiIpiProcessRequests();

    //
    // If freeze is requested, then freeze target execution.
    //

    if ((RequestSummary & IPI_FREEZE) != 0) {
        KiFreezeTargetExecution(TrapFrame, ExceptionFrame);
    }

    //
    // Return whether any requests were processed.
    //

    return (RequestSummary & ~IPI_FREEZE) != 0;
}
