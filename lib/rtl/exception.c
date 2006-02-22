/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Runtime Library
 * PURPOSE:         User-Mode Exception Support
 * FILE:            lib/rtl/exception.c
 * PROGRAMERS:      Alex Ionescu (alex@relsoft.net)
 *                  David Welch <welch@cwcom.net>
 *                  Skywing <skywing@valhallalegends.com>
 *                  KJK::Hyperion <noog@libero.it>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{
    CONTEXT Context;
    NTSTATUS Status;
    DPRINT1("RtlRaiseException(Status %p)\n", ExceptionRecord);

    /* Capture the context */
    RtlCaptureContext(&Context);

    /* Save the exception address */
    ExceptionRecord->ExceptionAddress = RtlpGetExceptionAddress();
    DPRINT1("ExceptionAddress %p\n", ExceptionRecord->ExceptionAddress);

    /* Write the context flag */
    Context.ContextFlags = CONTEXT_FULL;

    /* Check if we're being debugged (user-mode only) */
    if (!RtlpCheckForActiveDebugger(TRUE))
    {
        /* Raise an exception immediately */
        Status = ZwRaiseException(ExceptionRecord, &Context, TRUE);
    }
    else
    {
        /* Dispatch the exception and check if we should continue */
        if (RtlDispatchException(ExceptionRecord, &Context))
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

    /* If we returned, raise a status  */
    RtlRaiseStatus(Status);
}

/*
 * @implemented
 */
VOID
NTAPI
RtlRaiseStatus(NTSTATUS Status)
{
    EXCEPTION_RECORD ExceptionRecord;
    CONTEXT Context;
    DPRINT1("RtlRaiseStatus(Status 0x%.08lx)\n", Status);

     /* Capture the context */
    RtlCaptureContext(&Context);

    /* Add one argument to ESP */
    Context.Esp += sizeof(PVOID);

    /* Create an exception record */
    ExceptionRecord.ExceptionAddress = RtlpGetExceptionAddress();
    ExceptionRecord.ExceptionCode  = Status;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    /* Write the context flag */
    Context.ContextFlags = CONTEXT_FULL;

    /* Check if we're being debugged (user-mode only) */
    if (!RtlpCheckForActiveDebugger(TRUE))
    {
        /* Raise an exception immediately */
        ZwRaiseException(&ExceptionRecord, &Context, TRUE);
    }
    else
    {
        /* Dispatch the exception */
        RtlDispatchException(&ExceptionRecord, &Context);

        /* Raise exception if we got here */
        Status = ZwRaiseException(&ExceptionRecord, &Context, FALSE);
    }

    /* If we returned, raise a status */
    RtlRaiseStatus(Status);
}

/*
* @unimplemented
*/
USHORT
NTAPI
RtlCaptureStackBackTrace(IN ULONG FramesToSkip,
                         IN ULONG FramesToCapture,
                         OUT PVOID *BackTrace,
                         OUT PULONG BackTraceHash OPTIONAL)
{
    UNIMPLEMENTED;
    return 0;
}

/*
* @unimplemented
*/
ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
LONG
NTAPI
RtlUnhandledExceptionFilter(IN struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
