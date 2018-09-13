/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    jumps.c

Abstract:

    This module implements the C runtime library functions for set jump and
    long jump that are compatible with structured exception handling.

Author:

    David N. Cutler (davec) 15-Sep-1990

Environment:

    Any mode.

Revision History:

    Tom Wood  23-Aug-1994
    Add stack limit parameters to RtlVirtualUnwind.

--*/

#include "ntrtlp.h"
#include "setjmp.h"

VOID
longjmp (
    IN jmp_buf JumpBuffer,
    IN int ReturnValue
    )

/*++

Routine Description:

    This function executes a long jump operation by virtually unwinding to
    the caller of the corresponding call to set jump and then calling unwind
    to transfer control to the jump target.

Arguments:


    JumpBuffer - Supplies the address of a jump buffer that contains the
        virtual frame pointer and target address.

        N.B. This is an array of double to force quadword alignment.

    ReturnValue - Supplies the value that is to be returned to the caller
        of set jump.

Return Value:

    None.

--*/

{

    PULONG JumpArray;

    //
    // If the specified return value is zero, then set it to one.
    //

    if (ReturnValue == 0) {
        ReturnValue = 1;
    }

    //
    // Unwind to the caller of set jump and return the specified value.
    // There is no return from unwind.
    //

    JumpArray = (PULONG)&JumpBuffer[0];
    RtlUnwind((PVOID)JumpArray[0],
              (PVOID)JumpArray[1],
              NULL,
              (PVOID)ReturnValue);
}

int
setjmp (
    IN jmp_buf JumpBuffer
    )

/*++

Routine Description:

    This function performs a set jump operation by capturing the current
    context, virtualy unwinding to the caller of set jump, and returns zero
    to the caller.

Arguments:

    JumpBuffer - Supplies the address of a jump buffer to store the virtual
        frame pointer and target address of the caller.

        N.B. This is an array of double to force quadword alignment.

Return Value:

    A value of zero is returned.

--*/

{

    CONTEXT ContextRecord;
    ULONG EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    PULONG JumpArray;
    ULONG NextPc;

    //
    // Capture the current context, virtually unwind to the caller of set
    // jump, and return zero to the caller.
    //

    JumpArray = (PULONG)&JumpBuffer[0];
    RtlCaptureContext(&ContextRecord);
    NextPc = ContextRecord.Lr - 4;
    FunctionEntry = RtlLookupFunctionEntry(NextPc);
    NextPc = RtlVirtualUnwind(NextPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL,
                              0,
                              0xffffffff);

    JumpArray[1] = NextPc + 4;
    FunctionEntry = RtlLookupFunctionEntry(NextPc);
    NextPc = RtlVirtualUnwind(NextPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL,
                              0,
                              0xffffffff);

    JumpArray[0] = EstablisherFrame;
    return 0;
}
