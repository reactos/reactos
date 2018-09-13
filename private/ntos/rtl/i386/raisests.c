/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    raisests.c

Abstract:

    This module implements the routine that raises an exception given a
    specific status value.

Author:

    David N. Cutler (davec) 8-Aug-1990

Environment:

    Any mode.

Revision History:

--*/

#include "ntrtlp.h"

VOID
RtlRaiseStatus (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This function raises an exception with the specified status value. The
    exception is marked as continuable with no parameters.

Arguments:

    Status - Supplies the status value to be used as the exception code
        for the exception that is to be raised.

Return Value:

    None.

--*/

{

    EXCEPTION_RECORD ExceptionRecord;

    //
    // Construct an exception record.
    //

    ExceptionRecord.ExceptionCode = Status;
    ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    RtlRaiseException(&ExceptionRecord);
    return;
}
