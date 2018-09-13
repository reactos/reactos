/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    excpdbg.c

Abstract:

    This module implements an exception dispatcher logging facility

Author:

    Kent Forschmiedt (kentf) 05-Oct-1995

Revision History:

--*/

#include "ntrtlp.h"

PLAST_EXCEPTION_LOG RtlpExceptionLog;
ULONG RtlpExceptionLogCount;
ULONG RtlpExceptionLogSize;


VOID
RtlInitializeExceptionLog(
    IN ULONG Entries
    )
/*++

Routine Description:

    This routine allocates space for the exception dispatcher logging
    facility, and records the address and size of the log area in globals
    where they can be found by the debugger.

    If memory is not available, the table pointer will remain NULL
    and the logging functions will do nothing.

Arguments:

    Entries - Supplies the number of entries to allocate for

Return Value:

    None

--*/
{
#if defined(NTOS_KERNEL_RUNTIME)
    RtlpExceptionLog = (PLAST_EXCEPTION_LOG)ExAllocatePoolWithTag( NonPagedPool, sizeof(LAST_EXCEPTION_LOG) * Entries, 'gbdE' );
#else
    //RtlpExceptionLog = (PLAST_EXCEPTION_LOG)RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(LAST_EXCEPTION_LOG) * Entries );
#endif
    if (RtlpExceptionLog) {
        RtlpExceptionLogSize = Entries;
    }
}


ULONG
RtlpLogExceptionHandler(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN ULONG_PTR ControlPc,
    IN PVOID HandlerData,
    IN ULONG Size
    )
/*++

Routine Description:

    Records the dispatching of exceptions to frame-based handlers.
    The debugger may inspect the table later and interpret the data
    to discover the address of the filters and handlers.

Arguments:

    ExceptionRecord - Supplies an exception record

    ContextRecord - Supplies the context at the exception

    ControlPc - Supplies the PC where control left the frame being
        dispatched to.

    HandlerData - Supplies a pointer to the host-dependent exception
        data.  On the RISC machines this is a RUNTIME_FUNCTION record;
        on x86 it is the registration record from the stack frame.

    Size - Supplies the size of HandlerData

Returns:

    The index to the log entry used, so that if the handler returns
    a disposition it may be recorded.

--*/
{
#if !defined(NTOS_KERNEL_RUNTIME)

    return 0;

#else

    ULONG LogIndex;

    if (!RtlpExceptionLog) {
        return 0;
    }

    ASSERT(Size <= MAX_EXCEPTION_LOG_DATA_SIZE * sizeof(ULONG));

    do {
        LogIndex = RtlpExceptionLogCount;
    } while (LogIndex != (ULONG)InterlockedCompareExchange(
                                    (PLONG)&RtlpExceptionLogCount,
                                    ((LogIndex + 1) % MAX_EXCEPTION_LOG),
                                    LogIndex));

    //
    // the debugger will have to interpret the exception handler
    // data, because it cannot be done safely here.
    //

    RtlCopyMemory(RtlpExceptionLog[LogIndex].HandlerData,
                  HandlerData,
                  Size);
    RtlpExceptionLog[LogIndex].ExceptionRecord = *ExceptionRecord;
    RtlpExceptionLog[LogIndex].ContextRecord = *ContextRecord;
    RtlpExceptionLog[LogIndex].Disposition = -1;

    return LogIndex;
#endif  // !NTOS_KERNEL_RUNTIME
}


VOID
RtlpLogLastExceptionDisposition(
    ULONG LogIndex,
    EXCEPTION_DISPOSITION Disposition
    )
/*++

Routine Description:

    Records the disposition from an exception handler.

Arguments:

    LogIndex - Supplies the entry number of the exception log record.

    Disposition - Supplies the disposition code

Return Value:

    None

--*/

{
    // If MAX_EXCEPTION_LOG or more exceptions were dispatched while
    // this one was being handled, this disposition will get written
    // on the wrong record.  Oh well.
    if (RtlpExceptionLog) {
        RtlpExceptionLog[LogIndex].Disposition = Disposition;
    }
}

