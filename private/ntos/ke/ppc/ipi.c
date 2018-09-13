/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    xxmpipi.c

Abstract:

    This module implements PowerPC specific MP routine.

Author:

    Pat Carr  15-Aug-1994

Based on MIPS version authored by:

    David N. Cutler 24-Apr-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define forward reference function prototypes.
//

VOID
KiRestoreProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    );

VOID
KiSaveProcessorControlState (
    IN PKPROCESSOR_STATE ProcessorState
    );

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

#if !defined(NT_UP)

    KeContextToKframes(TrapFrame,
                       ExceptionFrame,
                       &Prcb->ProcessorState.ContextFrame,
                       CONTEXT_FULL,
                       KernelMode);

#endif

    //
    // Restore the current processor control state.
    // Currently, the primary use is to allow the kernel
    // debugger to set hardware debug registers. Still
    // investigating whether this is required for MP systems.
    //

    KiRestoreProcessorControlState(&Prcb->ProcessorState);
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
    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_FULL |
                                                     CONTEXT_DEBUG_REGISTERS;
    KeContextFromKframes(TrapFrame,
                         ExceptionFrame,
                         &Prcb->ProcessorState.ContextFrame);

    //
    // Save the current processor control state.
    //
    Prcb->ProcessorState.SpecialRegisters.KernelDr6 =
                                       Prcb->ProcessorState.ContextFrame.Dr6;
    KiSaveProcessorControlState(&Prcb->ProcessorState);
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
