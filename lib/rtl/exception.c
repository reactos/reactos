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

    /* Capture the context and fixup ESP */
    RtlCaptureContext(&Context);
    Context.Esp += sizeof(ULONG);

    /* Save the exception address */
    ExceptionRecord->ExceptionAddress = RtlpGetExceptionAddress();

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

    /* We should never return */
    while (TRUE);
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
 * @implemented
 */
ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    PULONG Stack, NewStack;
    ULONG Eip;
    ULONG_PTR StackBegin, StackEnd;
    BOOLEAN Result, StopSearch = FALSE;
    ULONG i = 0;

    /* Get current EBP */
#if defined __GNUC__
    __asm__("mov %%ebp, %0" : "=r" (Stack) : );
#elif defined(_MSC_VER)
    __asm mov Stack, ebp
#endif

    /* Set it as the stack begin limit as well */
    StackBegin = (ULONG_PTR)Stack;

    /* Check if we're called for non-logging mode */
    if (!Flags)
    {
        /* Get the actual safe limits */
        Result = RtlpCaptureStackLimits((ULONG_PTR)Stack,
                                        &StackBegin,
                                        &StackEnd);
        if (!Result) return 0;
    }

    /* Loop the frames */
    for (i = 0; i < Count; i++)
    {
        /* Check if we're past the stack */
        if ((ULONG_PTR)Stack >= StackEnd) break;

        /* Check if this is the first entry */
#if 0
        if (!i)
        {
            if ((ULONG_PTR)Stack != StackBegin) break;
        }
        else
        {
            if ((ULONG_PTR)Stack == StackBegin) break;
        }
#endif

        /* Make sure there's enough frames */
        if ((StackEnd - (ULONG_PTR)Stack) < (2 * sizeof(ULONG_PTR))) break;

        /* Get new stack and EIP */
        NewStack = (PULONG)Stack[0];
        Eip = Stack[1];

        /* Check if the new pointer is above the oldone and past the end */
        if (!((Stack < NewStack) && ((ULONG_PTR)NewStack < StackEnd)))
        {
            /* Stop searching after this entry */
            StopSearch = TRUE;
        }

        /* Also make sure that the EIP isn't a stack address */
        if ((StackBegin < Eip) && (Eip < StackEnd)) break;

        /* Save this frame */
        Callers[i] = (PVOID)Eip;

        /* Check if we should continue */
        if (StopSearch) break;

        /* Move to the next stack */
        Stack = NewStack;
    }

    /* Return frames parsed */
    return i;
}

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
    UNIMPLEMENTED;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
