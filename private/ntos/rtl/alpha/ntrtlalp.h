/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntrtlalp.h

Abstract:

    Alpha specific parts of ntrtlp.h.

Author:

    David N. Cutler (davec) 19-Apr-90

Revision History:

    Thomas Van Baak (tvb) 5-May-1992

        Adapted for Alpha AXP.

--*/

//
// Define exception routine function prototypes.
//

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG_PTR EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONG_PTR EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );

//
// Define procedure prototypes for exception filter and termination handler
// execution routines.
//

LONG
RtlpExecuteExceptionFilter (
    PEXCEPTION_POINTERS ExceptionPointers,
    EXCEPTION_FILTER ExceptionFilter,
    ULONG_PTR EstablisherFrame
    );

VOID
RtlpExecuteTerminationHandler (
    BOOLEAN AbnormalTermination,
    TERMINATION_HANDLER TerminationHandler,
    ULONG_PTR EstablisherFrame
    );

//
// Define function prototype for restore context.
//

VOID
RtlpRestoreContext (
    IN PCONTEXT Context
    );

#if DBG

//
// Define global flags to debug/validate exception handling for Alpha.
//

extern ULONG RtlDebugFlags;

//
// Print exception records as delivered by PALcode (KiDispatchException).
//

#define RTL_DBG_PAL_EXCEPTION               0x00001

//
// Software raised exceptions (RtlRaiseException, RtlRaiseStatus).
//

#define RTL_DBG_RAISE_EXCEPTION             0x00002

//
// Find a handler to take the exception (RtlDispatchException).
//

#define RTL_DBG_DISPATCH_EXCEPTION          0x00030
#define RTL_DBG_DISPATCH_EXCEPTION_DETAIL   0x00020

//
// Call handlers and unwind to a target frame (RtlUnwind).
//

#define RTL_DBG_UNWIND                      0x00300
#define RTL_DBG_UNWIND_DETAIL               0x00200

//
// Climb one frame up the call stack (RtlVirtualUnwind).
//

#define RTL_DBG_VIRTUAL_UNWIND              0x03000
#define RTL_DBG_VIRTUAL_UNWIND_DETAIL       0x02000

//
// Find the function entry for a given PC (RtlLookupFunctionEntry).
//

#define RTL_DBG_FUNCTION_ENTRY              0x30000
#define RTL_DBG_FUNCTION_ENTRY_DETAIL       0x20000

#endif // DBG
