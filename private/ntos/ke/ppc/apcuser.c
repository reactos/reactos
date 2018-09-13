/*++

Copyright (c) 1993  IBM Corporation and Microsoft Corporation

Module Name:

    apcuser.c

Abstract:

    This module implements the machine dependent code necessary to initialize
    a user mode APC.

Author:

    Rick Simpson  25-Oct-1993

    based on MIPS version by David N. Cutler (davec) 23-Apr-1990

Environment:

    Kernel mode only, IRQL APC_LEVEL.

Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#define _KXPPC_C_HEADER_
#include "kxppc.h"

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
    PULONG PUserStack;

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
    // We build the following structure on the user stack:
    //
    //             |                               |
    //             |-------------------------------|
    //             |   Stack frame header          |
    //             |      Back chain points to     |
    //             |      user's stack frame       |
    //             | - - - - - - - - - - - - - - - |
    //             |   Context Frame               |
    //             |      Filled in with state     |
    //             |      of interrupted user      |
    //             |      program                  |
    //             | - - - - - - - - - - - - - - - |
    //             |   Trap Frame                  |
    //             |      Empty; for use by        |
    //             |      NtContinue eventually    |
    //             | - - - - - - - - - - - - - - - |
    //             |   Save word for TOC ptr       |
    //             | - - - - - - - - - - - - - - - |
    //             |   Canonical slack space       |
    //             |-------------------------------|
    //             |                               |
    //             |   Interrupted user's          |
    //             |   stack frame                 |
    //             |                               |
    //             |                               |
    //             |-------------------------------|
    //             |                               |

    try {

    //
    // Set pointer to KeUserApcDispatcher\'s function descriptor
    //    First word = address of entry point
    //    Second word = address of TOC
    //

    PULONG FnDesc = (PULONG) KeUserApcDispatcher;

    //
    // Compute length of context record and new aligned user stack pointer.
    //

    Length = (STK_MIN_FRAME + CONTEXT_LENGTH + KTRAP_FRAME_LENGTH +
                sizeof(ULONG) + STK_SLACK_SPACE + 7) & (-8);
    UserStack = (ContextRecord.Gpr1 & (~7)) - Length;

    //
    // Probe user stack area for writeability and then transfer the
    // context record to the user stack.
    //

    ProbeForWrite((PCHAR)UserStack, Length, sizeof(QUAD));
    RtlCopyMemory((PULONG)(UserStack + STK_MIN_FRAME), &ContextRecord, sizeof(CONTEXT));

    //
    // Set the back chain in the new stack frame, store the resume
    // address as if it were the LR value (for stack trace/unwind),
    // and fill in TOC value as if it had been saved by prologue.
    //

    PUserStack = (PULONG) UserStack;
    PUserStack[0] = ContextRecord.Gpr1;
    PUserStack[(STK_MIN_FRAME + CONTEXT_LENGTH +
                KTRAP_FRAME_LENGTH) / sizeof(ULONG)] = FnDesc[1];

    //
    // Set the address of the user APC routine, the APC parameters, the
    // new frame pointer, and the new stack pointer in the current trap
    // frame. Set the continuation address so control will be transfered
    // to the user APC dispatcher.
    //

    TrapFrame->Gpr1 = UserStack;                // stack pointer
    TrapFrame->Gpr2 = FnDesc[1];                // TOC address from descriptor
    TrapFrame->Gpr3 = (ULONG)NormalContext;     // 1st parameter
    TrapFrame->Gpr4 = (ULONG)SystemArgument1;   // 2nd parameter
    TrapFrame->Gpr5 = (ULONG)SystemArgument2;   // 3rd parameter
    TrapFrame->Gpr6 = (ULONG)NormalRoutine;     // 4th parameter
    TrapFrame->Iar  = FnDesc[0];                // entry point from descriptor

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

        ExceptionRecord.ExceptionAddress = (PVOID)(TrapFrame->Iar);
        KiDispatchException(&ExceptionRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }

    return;
}
