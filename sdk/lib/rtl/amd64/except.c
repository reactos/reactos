/*
* COPYRIGHT:         See COPYING in the top level directory
* PROJECT:           ReactOS Run-Time Library
* PURPOSE:           User-mode exception support for AMD64
* FILE:              lib/rtl/amd64/except.c
* PROGRAMERS:        Timo Kreuzer (timo.kreuzer@reactos.org)
*/

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

VOID
NTAPI
RtlRaiseException(IN PEXCEPTION_RECORD ExceptionRecord)
{
    CONTEXT Context;
    NTSTATUS Status = STATUS_INVALID_DISPOSITION;
    ULONG64 ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 EstablisherFrame;

    /* Capture the context */
    RtlCaptureContext(&Context);

    /* Get the function entry for this function */
    FunctionEntry = RtlLookupFunctionEntry(Context.Rip,
                                           &ImageBase,
                                           NULL);

    /* Check if we found it */
    if (FunctionEntry)
    {
        /* Unwind to the caller of this function */
        RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                         ImageBase,
                         Context.Rip,
                         FunctionEntry,
                         &Context,
                         &HandlerData,
                         &EstablisherFrame,
                         NULL);

        /* Save the exception address */
        ExceptionRecord->ExceptionAddress = (PVOID)Context.Rip;

        /* Write the context flag */
        Context.ContextFlags = CONTEXT_FULL;

        /* Check if user mode debugger is active */
        if (RtlpCheckForActiveDebugger())
        {
            /* Raise an exception immediately */
            Status = ZwRaiseException(ExceptionRecord, &Context, TRUE);
        }
        else
        {
            /* Dispatch the exception and check if we should continue */
            if (!RtlDispatchException(ExceptionRecord, &Context))
            {
                /* Raise the exception */
                Status = ZwRaiseException(ExceptionRecord, &Context, FALSE);
            }
            else
            {
                /* Continue, go back to previous context */
                Status = ZwContinue(&Context, FALSE);
            }
        }
    }

    /* If we returned, raise a status */
    RtlRaiseStatus(Status);
}

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

BOOLEAN
NTAPI
RtplUnwindInternal(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue,
    _In_ PCONTEXT ContextRecord,
    _In_opt_ struct _UNWIND_HISTORY_TABLE *HistoryTable,
    _In_ ULONG Flags);

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlDispatchException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord)
{
    /* Perform vectored exception handling for user mode */
    if (RtlCallVectoredExceptionHandlers(ExceptionRecord, ContextRecord))
    {
        /* Exception handled, now call vectored continue handlers */
        RtlCallVectoredContinueHandlers(ExceptionRecord, ContextRecord);

        /* Continue execution */
        return TRUE;
    }

    /* Call the internal unwind routine */
    return RtplUnwindInternal(NULL, // TargetFrame
                              NULL, // TargetIp
                              ExceptionRecord,
                              0, // ReturnValue
                              ContextRecord,
                              NULL, // HistoryTable
                              UNW_FLAG_EHANDLER);
}
