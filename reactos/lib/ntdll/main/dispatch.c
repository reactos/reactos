/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT Library
 * FILE:            lib/ntdll/main/dispatch.c
 * PURPOSE:         User-Mode NT Dispatchers
 * PROGRAMERS:      Alex Ionescu (alex@relsoft.net)
 *                  David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

typedef NTSTATUS (NTAPI *USER_CALL)(PVOID Argument, ULONG ArgumentLength);

EXCEPTION_DISPOSITION NTAPI
RtlpExecuteVectoredExceptionHandlers(IN PEXCEPTION_RECORD  ExceptionRecord,
                                     IN PCONTEXT  Context);

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
                          PCONTEXT Context)
{
    EXCEPTION_RECORD NestedExceptionRecord;
    NTSTATUS Status;

    /* call the vectored exception handlers */
    if(RtlpExecuteVectoredExceptionHandlers(ExceptionRecord,
                                            Context) != ExceptionContinueExecution)
    {
        goto ContinueExecution;
    }
    else
    {
        /* Dispatch the exception and check the result */
        if(RtlDispatchException(ExceptionRecord, Context))
        {
ContinueExecution:
            /* Continue executing */
            Status = NtContinue(Context, FALSE);
        }
        else
        {
            /* Raise an exception */
            Status = NtRaiseException(ExceptionRecord, Context, FALSE);
        }
    }

    /* Setup the Exception record */
    NestedExceptionRecord.ExceptionCode = Status;
    NestedExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    NestedExceptionRecord.ExceptionRecord = ExceptionRecord;
    NestedExceptionRecord.NumberParameters = Status;

    /* Raise the exception */
    RtlRaiseException(&NestedExceptionRecord);
}

/*
 * @implemented
 */
VOID
NTAPI
KiRaiseUserExceptionDispatcher(VOID)
{
    EXCEPTION_RECORD ExceptionRecord;

    /* Setup the exception record */
    ExceptionRecord.ExceptionCode = ((PTEB)NtCurrentTeb())->ExceptionCode;
    ExceptionRecord.ExceptionFlags = 0;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.NumberParameters = 0;

    /* Raise the exception */
    RtlRaiseException(&ExceptionRecord);
}

/*
 * @implemented
 */
VOID
NTAPI
KiUserCallbackDispatcher(ULONG Index,
                         PVOID Argument,
                         ULONG ArgumentLength)
{
    /* Return with the result of the callback function */
    ZwCallbackReturn(NULL,
                     0,
                     ((USER_CALL)(NtCurrentPeb()->KernelCallbackTable[Index]))
                     (Argument, ArgumentLength));
}
