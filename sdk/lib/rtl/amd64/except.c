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

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlDispatchException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT Context)
{
    PEXCEPTION_ROUTINE ExceptionRoutine;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_RECORD LocalExceptionRecord;
    EXCEPTION_DISPOSITION Disposition;
    ULONG_PTR StackLow, StackHigh;
    CONTEXT UnwindContext;
    ULONG64 ImageBase, EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;

    __debugbreak();

    /* Perform vectored exception handling for user mode */
    if (RtlCallVectoredExceptionHandlers(ExceptionRecord, Context))
    {
        /* Exception handled, now call vectored continue handlers */
        RtlCallVectoredContinueHandlers(ExceptionRecord, Context);

        /* Continue execution */
        return TRUE;
    }

    /* Get the current stack limits and registration frame */
    RtlpGetStackLimits(&StackLow, &StackHigh);

    UnwindContext = *Context;
    DispatcherContext.ContextRecord = &UnwindContext;
    DispatcherContext.ScopeIndex = 0;

    /* Loop the frames */
    while (TRUE)
    {
        /* Save the current RIP before unwinding */
        DispatcherContext.ControlPc = UnwindContext.Rip;

        /* Lookup the FunctionEntry for the current RIP */
        FunctionEntry = RtlLookupFunctionEntry(UnwindContext.Rip, &ImageBase, NULL);

        /* Check if we found a function entry */
        if (FunctionEntry != NULL)
        {
            /* We have a function entry. Use it to do a virtual unwind */
            ExceptionRoutine = RtlVirtualUnwind(0,
                                                ImageBase,
                                                UnwindContext.Rip,
                                                FunctionEntry,
                                                &UnwindContext,
                                                &DispatcherContext.HandlerData,
                                                &EstablisherFrame,
                                                NULL);
            DPRINT("Nested funtion, new Rip = %p, new Rsp = %p\n", (PVOID)UnwindContext.Rip, (PVOID)UnwindContext.Rsp);
        }
        else
        {
            /* No function entry, so this must be a leaf function. Pop the return address from the stack */
            UnwindContext.Rip = *(DWORD64*)UnwindContext.Rsp;
            UnwindContext.Rsp += sizeof(DWORD64);
            DPRINT("Leaf funtion, new Rip = %p, new Rsp = %p\n", (PVOID)UnwindContext.Rip, (PVOID)UnwindContext.Rsp);
        }

        /* Check if new Rip is valid */
        if (UnwindContext.Rip == 0)
        {
            __debugbreak();
            break;
        }

        /* Check, if we have left our stack */
        if ((UnwindContext.Rsp < StackLow) || 
            (UnwindContext.Rsp > StackHigh) ||
            (UnwindContext.Rsp & 15))
        {
#if 0
            /* Check if this happened in the DPC Stack */
            if (RtlpHandleDpcStackException(RegistrationFrame,
                                            RegistrationFrameEnd,
                                            &StackLow,
                                            &StackHigh))
            {
                /* Use DPC Stack Limits and restart */
                continue;
            }
#endif
            __debugbreak();

            /* Set invalid stack and return false */
            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            return FALSE;
        }

        /* Check if we got an exception routine */
        if (ExceptionRoutine != NULL)
        {
            DispatcherContext.ImageBase = (PVOID)ImageBase;
            DispatcherContext.FunctionEntry = FunctionEntry;
            DispatcherContext.TargetIp = 0;
            DispatcherContext.LanguageHandler = ExceptionRoutine;
            DispatcherContext.HistoryTable = NULL;
            DispatcherContext.EstablisherFrame = (PVOID)EstablisherFrame;

            /* Check if logging is enabled */
            RtlpCheckLogException(ExceptionRecord,
                                  Context,
                                  &DispatcherContext,
                                  sizeof(DispatcherContext));

            /* Call the language specific handler */
            Disposition = ExceptionRoutine(ExceptionRecord,
                                           (PVOID)EstablisherFrame,
                                           Context,
                                           &DispatcherContext);

            /* Handle the dispositions */
            switch (Disposition)
            {
                /* Continue execution */
                case ExceptionContinueExecution:

                    /* Check if it was non-continuable */
                    if (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
                    {
                        /* Set up the exception record */
                        LocalExceptionRecord.ExceptionRecord = ExceptionRecord;
                        LocalExceptionRecord.ExceptionCode =
                            STATUS_NONCONTINUABLE_EXCEPTION;
                        LocalExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                        LocalExceptionRecord.NumberParameters = 0;

                        /* Raise the exception */
                        RtlRaiseException(&LocalExceptionRecord);
                    }
                    else
                    {
                        /* In user mode, call any registered vectored continue handlers */
                        RtlCallVectoredContinueHandlers(ExceptionRecord,
                                                        Context);

                        /* Execution continues */
                        return TRUE;
                    }

                    /* Continue searching */
                case ExceptionContinueSearch:
                    break;

                    /* Nested exception */
                case ExceptionNestedException:

                    /* Turn the nested flag on */
                    ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;

                    /* Update the current nested frame */
                    //if (DispatcherContext.RegistrationPointer > NestedFrame)
                    //{
                        /* Get the frame from the dispatcher context */
                        //NestedFrame = DispatcherContext.RegistrationPointer;
                    //}
                    __debugbreak();
                    break;

                    /* Anything else */
                default:

                    /* Set up the exception record */
                    LocalExceptionRecord.ExceptionRecord = ExceptionRecord;
                    LocalExceptionRecord.ExceptionCode = STATUS_INVALID_DISPOSITION;
                    LocalExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                    LocalExceptionRecord.NumberParameters = 0;

                    /* Raise the exception */
                    RtlRaiseException(&LocalExceptionRecord);
                    break;
            }
        }
    }

    /* Unhandled, return false */
    return FALSE;
}
