/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 exception dispatching and raising
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
RtlpUnwindInternal(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue,
    _In_ PCONTEXT ContextRecord,
    _In_opt_ struct _UNWIND_HISTORY_TABLE *HistoryTable,
    _In_ ULONG HandlerType);

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
PVOID
NTAPI
RtlpGetExceptionAddress(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

/*! RtlDispatchException
 * \brief Runs the vectored and frame based handlers for an exception.
 */
BOOLEAN
NTAPI
RtlDispatchException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord)
{
    BOOLEAN Handled;

    if (RtlCallVectoredExceptionHandlers(ExceptionRecord, ContextRecord))
    {
        RtlCallVectoredContinueHandlers(ExceptionRecord, ContextRecord);
        return TRUE;
    }

    Handled = RtlpUnwindInternal(NULL,
                                 NULL,
                                 ExceptionRecord,
                                 NULL,
                                 ContextRecord,
                                 NULL,
                                 UNW_FLAG_EHANDLER);

    RtlCallVectoredContinueHandlers(ExceptionRecord, ContextRecord);

    return Handled;
}

/*! RtlRaiseException
 * \brief Raises a software exception on behalf of the caller.
 */
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord)
{
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG64 ImageBase, EstablisherFrame;
    PVOID HandlerData;
    CONTEXT Context;
    NTSTATUS Status;

    ExceptionRecord->ExceptionFlags |= EXCEPTION_SOFTWARE_ORIGINATE;

    /* The exception belongs to the caller, so unwind this function's own frame */
    RtlCaptureContext(&Context);

    FunctionEntry = RtlLookupFunctionEntry(Context.Pc, &ImageBase, NULL);
    if (FunctionEntry == NULL)
        RtlRaiseStatus(STATUS_BAD_FUNCTION_TABLE);

    RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                     ImageBase,
                     Context.Pc,
                     FunctionEntry,
                     &Context,
                     &HandlerData,
                     &EstablisherFrame,
                     NULL);

    ExceptionRecord->ExceptionAddress = (PVOID)Context.Pc;

    if (RtlpCheckForActiveDebugger())
    {
        /* Let the debugger see it first */
        Status = ZwRaiseException(ExceptionRecord, &Context, TRUE);
    }
    else if (RtlDispatchException(ExceptionRecord, &Context))
    {
        Status = ZwContinue(&Context, FALSE);
    }
    else
    {
        Status = ZwRaiseException(ExceptionRecord, &Context, FALSE);
    }

    RtlRaiseStatus(Status);
}
