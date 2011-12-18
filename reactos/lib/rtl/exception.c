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

/* GLOBALS *****************************************************************/

PRTLP_UNHANDLED_EXCEPTION_FILTER RtlpUnhandledExceptionFilter;

/* FUNCTIONS ***************************************************************/

#if !defined(_M_IX86) && !defined(_M_AMD64)

/*
 * @implemented
 */
VOID
NTAPI
RtlRaiseException(IN PEXCEPTION_RECORD ExceptionRecord)
{
    CONTEXT Context;
    NTSTATUS Status;

    /* Capture the context */
    RtlCaptureContext(&Context);

    /* Save the exception address */
    ExceptionRecord->ExceptionAddress = _ReturnAddress();

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

    /* If we returned, raise a status */
    RtlRaiseStatus(Status);
}

#endif

#if !defined(_M_IX86)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4717) // RtlRaiseStatus is recursive by design
#endif

/*
 * @implemented
 */
VOID
NTAPI
RtlRaiseStatus(IN NTSTATUS Status)
{
    EXCEPTION_RECORD ExceptionRecord;
    CONTEXT Context;

     /* Capture the context */
    RtlCaptureContext(&Context);

    /* Create an exception record */
    ExceptionRecord.ExceptionAddress = _ReturnAddress();
    ExceptionRecord.ExceptionCode  = Status;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    /* Write the context flag */
    Context.ContextFlags = CONTEXT_FULL;

    /* Check if user mode debugger is active */
    if (RtlpCheckForActiveDebugger())
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

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

/*
 * @implemented
 */
USHORT
NTAPI
RtlCaptureStackBackTrace(IN ULONG FramesToSkip,
                         IN ULONG FramesToCapture,
                         OUT PVOID *BackTrace,
                         OUT PULONG BackTraceHash OPTIONAL)
{
    PVOID Frames[2 * 64];
    ULONG FrameCount;
    ULONG Hash = 0, i;

    /* Skip a frame for the caller */
    FramesToSkip++;

    /* Don't go past the limit */
    if ((FramesToCapture + FramesToSkip) >= 128) return 0;

    /* Do the back trace */
    FrameCount = RtlWalkFrameChain(Frames, FramesToCapture + FramesToSkip, 0);

    /* Make sure we're not skipping all of them */
    if (FrameCount <= FramesToSkip) return 0;

    /* Loop all the frames */
    for (i = 0; i < FramesToCapture; i++)
    {
        /* Don't go past the limit */
        if ((FramesToSkip + i) >= FrameCount) break;

        /* Save this entry and hash it */
        BackTrace[i] = Frames[FramesToSkip + i];
        Hash += PtrToUlong(BackTrace[i]);
    }

    /* Write the hash */
    if (BackTraceHash) *BackTraceHash = Hash;

    /* Clear the other entries and return count */
    RtlFillMemoryUlong(Frames, 128, 0);
    return (USHORT)i;
}

/*
 * @unimplemented
 */
LONG
NTAPI
RtlUnhandledExceptionFilter(IN struct _EXCEPTION_POINTERS* ExceptionInfo)
{
    /* This is used by the security cookie checks, and calso called externally */
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlSetUnhandledExceptionFilter(IN PRTLP_UNHANDLED_EXCEPTION_FILTER TopLevelExceptionFilter)
{
    /* Set the filter which is used by the CriticalSection package */
    RtlpUnhandledExceptionFilter = RtlEncodePointer(TopLevelExceptionFilter);
}
