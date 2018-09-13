/*++

Module Name:

    ntrtliae.h

Abstract:

    IA64 specific parts of ntrtlp.h

Author:

    William K. Cheung (wcheung) 16-Jan-96

Revision History:

--*/

VOID
Rtlp64GetStackLimits (
    OUT PULONGLONG LowLimit,
    OUT PULONGLONG HighLimit
    );

VOID
Rtlp64GetBStoreLimits(
    OUT PULONGLONG LowBStoreLimit, 
    OUT PULONGLONG HighBStoreLimit
    );

//
// Exception handling procedure prototypes.
//

VOID
RtlpUnlinkHandler (
    PEXCEPTION_REGISTRATION_RECORD UnlinkPointer
    );

PEXCEPTION_REGISTRATION_RECORD
RtlpGetRegistrationHead (
    VOID
    );

EXCEPTION_DISPOSITION
RtlpExecuteEmHandlerForException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONGLONG MemoryStackFp,
    IN ULONGLONG BackingStoreFp,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN ULONGLONG GlobalPointer,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );

EXCEPTION_DISPOSITION
RtlpExecuteEmHandlerForUnwind (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONGLONG MemoryStackFp,
    IN ULONGLONG BackingStoreFp,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN ULONGLONG GlobalPointer,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );

EXCEPTION_DISPOSITION
RtlpExecuteX86HandlerForException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );

EXCEPTION_DISPOSITION
RtlpExecuteX86HandlerForUnwind (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );
