/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntrtlmip.h

Abstract:

    MIPS specific parts of ntrtlp.h

Author:

    David N. Cutler (davec) 19-Apr-90

Revision History:

--*/

//
// Define exception routine function prototypes.
//

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );
