/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    apcuser.c

Abstract:

    This module implements the machine dependent code necessary to initialize
    a user mode APC.

Author:

    David N. Cutler (davec) 23-Apr-1990

Environment:

    Kernel mode only, IRQL APC_LEVEL.

Revision History:

--*/

#include "ki.h"

VOID
KiInitializeUserApc (
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called to initialize the context for a user mode APC.

Arguments:

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    NormalRoutine - Supplies a pointer to the user mode APC routine.

    NormalContext - Supplies a pointer to the user context for the APC
        routine.

    SystemArgument1 - Supplies the first system supplied value.

    SystemArgument2 - Supplies the second system supplied value.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;
    EXCEPTION_RECORD ExceptionRecord;
    LONG Length;
    ULONG UserStack;

    //
    // Move the user mode state from the trap and exception frames to the
    // context frame.
    //

    ContextRecord.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextRecord);

    //
    // Transfer the context information to the user stack, initialize the
    // APC routine parameters, and modify the trap frame so execution will
    // continue in user mode at the user mode APC dispatch routine.
    //

    try {

        //
        // Compute length of context record and new aligned user stack pointer.
        //

        Length = sizeof(CONTEXT);
        UserStack = (ULONG)(ContextRecord.XIntSp & (~7)) - Length;

        //
        // Probe user stack area for writeability and then transfer the
        // context record to the user stack.
        //

        ProbeForWrite((PCHAR)UserStack, Length, sizeof(QUAD));
        RtlMoveMemory((PULONG)UserStack, &ContextRecord, sizeof(CONTEXT));

        //
        // Set the address of the user APC routine, the APC parameters, the
        // new frame pointer, and the new stack pointer in the current trap
        // frame. Set the continuation address so control will be transfered
        // to the user APC dispatcher.
        //

        TrapFrame->XIntSp = (LONG)UserStack;
        TrapFrame->XIntS8 = (LONG)UserStack;
        TrapFrame->XIntA0 = (LONG)NormalContext;
        TrapFrame->XIntA1 = (LONG)SystemArgument1;
        TrapFrame->XIntA2 = (LONG)SystemArgument2;
        TrapFrame->XIntA3 = (LONG)NormalRoutine;
        TrapFrame->Fir = KeUserApcDispatcher;

    //
    // If an exception occurs, then copy the exception information to an
    // exception record and handle the exception.
    //

    } except (KiCopyInformation(&ExceptionRecord,
                                (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Set the address of the exception to the current program address
        // and raise the exception by calling the exception dispatcher.
        //

        ExceptionRecord.ExceptionAddress = (PVOID)(TrapFrame->Fir);
        KiDispatchException(&ExceptionRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }

    return;
}
