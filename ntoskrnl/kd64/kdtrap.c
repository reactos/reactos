/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdtrap.c
 * PURPOSE:         KD64 Trap Handlers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

//
// Retrieves the ComponentId and Level for BREAKPOINT_PRINT
// and OutputString and OutputStringLength for BREAKPOINT_PROMPT.
//
#if defined(_X86_)

//
// EBX/EDI on x86
//
#define KdpGetParameterThree(Context)  ((Context)->Ebx)
#define KdpGetParameterFour(Context)   ((Context)->Edi)

#elif defined(_AMD64_)

//
// R8/R9 on AMD64
//
#define KdpGetParameterThree(Context)  ((Context)->R8)
#define KdpGetParameterFour(Context)   ((Context)->R9)

#elif defined(_ARM_)

//
// R3/R4 on ARM
//
#define KdpGetParameterThree(Context)  ((Context)->R3)
#define KdpGetParameterFour(Context)   ((Context)->R4)

#else
#error Unsupported Architecture
#endif

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
KdpReport(IN PKTRAP_FRAME TrapFrame,
          IN PKEXCEPTION_FRAME ExceptionFrame,
          IN PEXCEPTION_RECORD ExceptionRecord,
          IN PCONTEXT ContextRecord,
          IN KPROCESSOR_MODE PreviousMode,
          IN BOOLEAN SecondChanceException)
{
    BOOLEAN Enable, Handled;
    PKPRCB Prcb;
    NTSTATUS ExceptionCode;

    /*
     * Determine whether to pass the exception to the debugger.
     * First, check if this is a "debug exception", meaning breakpoint
     * (including debug service), single step and assertion failure exceptions.
     */
    ExceptionCode = ExceptionRecord->ExceptionCode;
    if ((ExceptionCode == STATUS_BREAKPOINT) ||
        (ExceptionCode == STATUS_SINGLE_STEP) ||
        (ExceptionCode == STATUS_ASSERTION_FAILURE))
    {
        /* This is a debug exception; we always pass them to the debugger */
    }
    else if (NtGlobalFlag & FLG_STOP_ON_EXCEPTION)
    {
        /*
         * Not a debug exception, but the stop-on-exception flag is set,
         * meaning the debugger requests that we pass it first chance
         * exceptions. However, some exceptions are always passed to the
         * exception handler first, namely exceptions with a code that isn't
         * an error or warning code, and also exceptions with the special
         * STATUS_PORT_DISCONNECTED code (an error code).
         */
        if ((SecondChanceException == FALSE) &&
            ((ExceptionCode == STATUS_PORT_DISCONNECTED) ||
             (NT_SUCCESS(ExceptionCode))))
        {
            /* Let the exception handler, if any, try to handle it */
            return FALSE;
        }
    }
    else if (SecondChanceException == FALSE)
    {
        /*
         * This isn't a debug exception and the stop-on-exception flag isn't set,
         * so don't bother handling it
         */
        return FALSE;
    }

    /* Enter the debugger */
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /*
     * Get the KPRCB and save the CPU Control State manually instead of
     * using KiSaveProcessorState, since we already have a valid CONTEXT.
     */
    Prcb = KeGetCurrentPrcb();
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    KdpMoveMemory(&Prcb->ProcessorState.ContextFrame,
                  ContextRecord,
                  sizeof(CONTEXT));

    /* Report the new state */
    Handled = KdpReportExceptionStateChange(ExceptionRecord,
                                            &Prcb->ProcessorState.
                                            ContextFrame,
                                            SecondChanceException);

    /* Now restore the processor state, manually again. */
    KdpMoveMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));
    KiRestoreProcessorControlState(&Prcb->ProcessorState);

    /* Exit the debugger and clear the CTRL-C state */
    KdExitDebugger(Enable);
    KdpControlCPressed = FALSE;
    return Handled;
}

BOOLEAN
NTAPI
KdpTrap(IN PKTRAP_FRAME TrapFrame,
        IN PKEXCEPTION_FRAME ExceptionFrame,
        IN PEXCEPTION_RECORD ExceptionRecord,
        IN PCONTEXT ContextRecord,
        IN KPROCESSOR_MODE PreviousMode,
        IN BOOLEAN SecondChanceException)
{
    BOOLEAN Unload;
    ULONG_PTR ProgramCounter;
    BOOLEAN Handled;
    NTSTATUS ReturnStatus;
    USHORT ReturnLength;

    /*
     * Check if we got a STATUS_BREAKPOINT with a SubID for Print, Prompt or
     * Load/Unload symbols. Make sure it isn't a software breakpoint as those
     * are handled by KdpReport.
     */
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK))
    {
        /* Save Program Counter */
        ProgramCounter = KeGetContextPc(ContextRecord);

        /* Check what kind of operation was requested from us */
        Unload = FALSE;
        switch (ExceptionRecord->ExceptionInformation[0])
        {
            /* DbgPrint */
            case BREAKPOINT_PRINT:

                /* Call the worker routine */
                ReturnStatus = KdpPrint((ULONG)KdpGetParameterThree(ContextRecord),
                                        (ULONG)KdpGetParameterFour(ContextRecord),
                                        (PCHAR)ExceptionRecord->ExceptionInformation[1],
                                        (USHORT)ExceptionRecord->ExceptionInformation[2],
                                        PreviousMode,
                                        TrapFrame,
                                        ExceptionFrame,
                                        &Handled);

                /* Update the return value for the caller */
                KeSetContextReturnRegister(ContextRecord, ReturnStatus);
                break;

            /* DbgPrompt */
            case BREAKPOINT_PROMPT:

                /* Call the worker routine */
                ReturnLength = KdpPrompt((PCHAR)ExceptionRecord->ExceptionInformation[1],
                                         (USHORT)ExceptionRecord->ExceptionInformation[2],
                                         (PCHAR)KdpGetParameterThree(ContextRecord),
                                         (USHORT)KdpGetParameterFour(ContextRecord),
                                         PreviousMode,
                                         TrapFrame,
                                         ExceptionFrame);
                Handled = TRUE;

                /* Update the return value for the caller */
                KeSetContextReturnRegister(ContextRecord, ReturnLength);
                break;

            /* DbgUnLoadImageSymbols */
            case BREAKPOINT_UNLOAD_SYMBOLS:

                /* Drop into the load case below, with the unload parameter */
                Unload = TRUE;

            /* DbgLoadImageSymbols */
            case BREAKPOINT_LOAD_SYMBOLS:

                /* Call the worker routine */
                KdpSymbol((PSTRING)ExceptionRecord->ExceptionInformation[1],
                          (PKD_SYMBOLS_INFO)ExceptionRecord->ExceptionInformation[2],
                          Unload,
                          PreviousMode,
                          ContextRecord,
                          TrapFrame,
                          ExceptionFrame);
                Handled = TRUE;
                break;

            /* DbgCommandString */
            case BREAKPOINT_COMMAND_STRING:

                /* Call the worker routine */
                KdpCommandString((PSTRING)ExceptionRecord->ExceptionInformation[1],
                                 (PSTRING)ExceptionRecord->ExceptionInformation[2],
                                 PreviousMode,
                                 ContextRecord,
                                 TrapFrame,
                                 ExceptionFrame);
                Handled = TRUE;
                break;

            /* Anything else, do nothing */
            default:

                /* Invalid debug service! Don't handle this! */
                Handled = FALSE;
                break;
        }

        /*
         * If the PC was not updated, we'll increment it ourselves so execution
         * continues past the breakpoint.
         */
        if (ProgramCounter == KeGetContextPc(ContextRecord))
        {
            /* Update it */
            KeSetContextPc(ContextRecord,
                           ProgramCounter + KD_BREAKPOINT_SIZE);
        }
    }
    else
    {
        /* Call the worker routine */
        Handled = KdpReport(TrapFrame,
                            ExceptionFrame,
                            ExceptionRecord,
                            ContextRecord,
                            PreviousMode,
                            SecondChanceException);
    }

    /* Return TRUE or FALSE to caller */
    return Handled;
}

BOOLEAN
NTAPI
KdpStub(IN PKTRAP_FRAME TrapFrame,
        IN PKEXCEPTION_FRAME ExceptionFrame,
        IN PEXCEPTION_RECORD ExceptionRecord,
        IN PCONTEXT ContextRecord,
        IN KPROCESSOR_MODE PreviousMode,
        IN BOOLEAN SecondChanceException)
{
    ULONG_PTR ExceptionCommand;

    /* Check if this was a breakpoint due to DbgPrint or Load/UnloadSymbols */
    ExceptionCommand = ExceptionRecord->ExceptionInformation[0];
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionCommand == BREAKPOINT_LOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_UNLOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_COMMAND_STRING) ||
         (ExceptionCommand == BREAKPOINT_PRINT)))
    {
        /* This we can handle: simply bump the Program Counter */
        KeSetContextPc(ContextRecord,
                       KeGetContextPc(ContextRecord) + KD_BREAKPOINT_SIZE);
        return TRUE;
    }
    else if (KdPitchDebugger)
    {
        /* There's no debugger, fail. */
        return FALSE;
    }
    else if ((KdAutoEnableOnEvent) &&
             (KdPreviouslyEnabled) &&
             !(KdDebuggerEnabled) &&
             (NT_SUCCESS(KdEnableDebugger())) &&
             (KdDebuggerEnabled))
    {
        /* Debugging was Auto-Enabled. We can now send this to KD. */
        return KdpTrap(TrapFrame,
                       ExceptionFrame,
                       ExceptionRecord,
                       ContextRecord,
                       PreviousMode,
                       SecondChanceException);
    }
    else
    {
        /* FIXME: All we can do in this case is trace this exception */
        return FALSE;
    }
}

BOOLEAN
NTAPI
KdIsThisAKdTrap(IN PEXCEPTION_RECORD ExceptionRecord,
                IN PCONTEXT Context,
                IN KPROCESSOR_MODE PreviousMode)
{
    /*
     * Determine if this is a valid debug service call and make sure that
     * it isn't a software breakpoint
     */
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK))
    {
        /* Then we have to handle it */
        return TRUE;
    }
    else
    {
        /* We don't have to handle it */
        return FALSE;
    }
}
