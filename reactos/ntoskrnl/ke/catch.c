/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/catch.c
 * PURPOSE:         Exception handling
 *
 * PROGRAMMERS:     Anich Gregor
 *                  David Welch (welch@mcmail.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

ULONG
RtlpDispatchException(IN PEXCEPTION_RECORD  ExceptionRecord,
                      IN PCONTEXT  Context);

/*
 * @unimplemented
 */
VOID
STDCALL
KiCoprocessorError(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
KiUnexpectedInterrupt(VOID)
{
    UNIMPLEMENTED;
}

VOID
KiDispatchException(PEXCEPTION_RECORD ExceptionRecord,
                    PCONTEXT Context,
                    PKTRAP_FRAME Tf,
                    KPROCESSOR_MODE PreviousMode,
                    BOOLEAN SearchFrames)
{
    EXCEPTION_DISPOSITION Value;
    CONTEXT TContext;
    KD_CONTINUE_TYPE Action = kdHandleException;

    DPRINT("KiDispatchException() called\n");

    /* Increase number of Exception Dispatches */
    KeGetCurrentPrcb()->KeExceptionDispatchCount++;

    if (!Context)
    {
        /* Assume Full context */
        TContext.ContextFlags = CONTEXT_FULL;

        /* Check the mode */
        if (PreviousMode == UserMode)
        {
            /* Add Debugger Registers if this is User Mode */
            TContext.ContextFlags = TContext.ContextFlags | CONTEXT_DEBUGGER;
        }

        /* Convert the Trapframe into a Context */
        KeTrapFrameToContext(Tf, &TContext);

        /* Use local stack context */
        Context = &TContext;
    }

    /* Break into Debugger */
    Action = KdpEnterDebuggerException(ExceptionRecord,
                                       PreviousMode,
                                       Context,
                                       Tf,
                                       TRUE,
                                       TRUE);

    /* If the debugger said continue, then continue */
    if (Action == kdContinue) return;

    /* If the Debugger couldn't handle it... */
    if (Action != kdDoNotHandleException)
    {
        /* See what kind of Exception this is */
        if (PreviousMode == UserMode)
        {
            /* User mode exception, search the frames if we have to */
            if (SearchFrames)
            {
                PULONG Stack;
                ULONG CDest;
                char temp_space[12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)]; /* FIXME: HACKHACK */
                PULONG pNewUserStack = (PULONG)(Tf->Esp - (12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)));
                NTSTATUS StatusOfCopy;

                /* Enter Debugger if available */
                Action = KdpEnterDebuggerException(ExceptionRecord,
                                                   PreviousMode,
                                                   Context,
                                                   Tf,
                                                   TRUE,
                                                   FALSE);

                /* Exit if we're continuing */
                if (Action == kdContinue) return;

                /* FIXME: Forward exception to user mode debugger */

                /* FIXME: Check user mode stack for enough space */

                /* Let usermode try and handle the exception. Setup Stack */
                Stack = (PULONG)temp_space;
                CDest = 3 + (ROUND_UP(sizeof(EXCEPTION_RECORD), 4) / 4);
                /* Return Address */
                Stack[0] = 0;
                /* Pointer to EXCEPTION_RECORD structure */
                Stack[1] = (ULONG)&pNewUserStack[3];
                /* Pointer to CONTEXT structure */
                Stack[2] = (ULONG)&pNewUserStack[CDest];
                memcpy(&Stack[3], ExceptionRecord, sizeof(EXCEPTION_RECORD));
                memcpy(&Stack[CDest], Context, sizeof(CONTEXT));

                /* Copy Stack */
                StatusOfCopy = MmCopyToCaller(pNewUserStack,
                                              temp_space,
                                              (12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)));

                /* Check for success */
                if (NT_SUCCESS(StatusOfCopy))
                {
                    /* Set new Stack Pointer */
                    Tf->Esp = (ULONG)pNewUserStack;
                }
                else
                {
                    /*
                     * Now it really hit the ventilation device. Sorry,
                     * can do nothing but kill the sucker.
                     */
                    ZwTerminateThread(NtCurrentThread(), ExceptionRecord->ExceptionCode);
                    DPRINT1("User-mode stack was invalid. Terminating target thread\n");
                }
                /* Set EIP to the User-mode Dispathcer */
                Tf->Eip = (ULONG)LdrpGetSystemDllExceptionDispatcher();
                return;
            }

            /* FIXME: Forward the exception to the debugger */

            /* FIXME: Forward the exception to the process exception port */

            /* Enter KDB if available */
            Action = KdpEnterDebuggerException(ExceptionRecord,
                                                PreviousMode,
                                                Context,
                                                Tf,
                                                FALSE,
                                                FALSE);

            /* Exit if we're continuing */
            if (Action == kdContinue) return;

            /* Terminate the offending thread */
            DPRINT1("Unhandled UserMode exception, terminating thread\n");
            ZwTerminateThread(NtCurrentThread(), ExceptionRecord->ExceptionCode);
        }
        else
        {
            /* This is Kernel Mode */

            /* Enter KDB if available */
            Action = KdpEnterDebuggerException(ExceptionRecord,
                                                PreviousMode,
                                                Context,
                                                Tf,
                                                TRUE,
                                                FALSE);

            /* Exit if we're continuing */
            if (Action == kdContinue) return;

            /* Dispatch the Exception */
            Value = RtlpDispatchException (ExceptionRecord, Context);
            DPRINT("RtlpDispatchException() returned with 0x%X\n", Value);

            /* If RtlpDispatchException() did not handle the exception then bugcheck */
            if (Value != ExceptionContinueExecution ||
                0 != (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE))
            {
                DPRINT("ExceptionRecord->ExceptionAddress = 0x%x\n", ExceptionRecord->ExceptionAddress);

                /* Enter KDB if available */
                Action = KdpEnterDebuggerException(ExceptionRecord,
                                                   PreviousMode,
                                                   Context,
                                                   Tf,
                                                   FALSE,
                                                   FALSE);

                /* Exit if we're continuing */
                if (Action == kdContinue) return;

                KEBUGCHECKWITHTF(KMODE_EXCEPTION_NOT_HANDLED,
                                 ExceptionRecord->ExceptionCode,
                                 (ULONG)ExceptionRecord->ExceptionAddress,
                                 ExceptionRecord->ExceptionInformation[0],
                                 ExceptionRecord->ExceptionInformation[1],
                                 Tf);
            }
        }
    }
}

/* EOF */
