/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdtrap.c
 * PURPOSE:         KD64 Trap Handlers
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

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
    BOOLEAN Entered, Status;
    PKPRCB Prcb;
    NTSTATUS ExceptionCode = ExceptionRecord->ExceptionCode;

    /* Check if this is INT1 or 3, or if we're forced to handle it */
    if ((ExceptionCode == STATUS_BREAKPOINT) ||
        (ExceptionCode == STATUS_SINGLE_STEP) ||
        //(ExceptionCode == STATUS_ASSERTION_FAILURE) ||
        (NtGlobalFlag & FLG_STOP_ON_EXCEPTION))
    {
        /* Check if we can't really handle this */
        if ((SecondChanceException) ||
            (ExceptionCode == STATUS_PORT_DISCONNECTED) ||
            (NT_SUCCESS(ExceptionCode)))
        {
            /* Return false to have someone else take care of the exception */
            return FALSE;
        }
    }
    else if (SecondChanceException)
    {
        /* We won't bother unless this is second chance */
        return FALSE;
    }

    /* Enter the debugger */
    Entered = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /*
     * Get the KPRCB and save the CPU Control State manually instead of
     * using KiSaveProcessorState, since we already have a valid CONTEXT.
     */
    Prcb = KeGetCurrentPrcb();
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                  ContextRecord,
                  sizeof(CONTEXT));

    /* Report the new state */
    Status = KdpReportExceptionStateChange(ExceptionRecord,
                                           &Prcb->ProcessorState.
                                           ContextFrame,
                                           SecondChanceException);

    /* Now restore the processor state, manually again. */
    RtlCopyMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));
    //KiRestoreProcessorControlState(&Prcb->ProcessorState);

    /* Exit the debugger and clear the CTRL-C state */
    KdExitDebugger(Entered);
    KdpControlCPressed = FALSE;
    return Status;
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
    BOOLEAN Unload = FALSE;
    ULONG Eip, Eax;
    BOOLEAN Status = FALSE;

    /*
     * Check if we got a STATUS_BREAKPOINT with a SubID for Print, Prompt or
     * Load/Unload symbols.
     */
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK))
    {
        /* Save EIP */
        Eip = ContextRecord->Eip;

        /* Check what kind of operation was requested from us */
        switch (ExceptionRecord->ExceptionInformation[0])
        {
            /* DbgPrint */
            case BREAKPOINT_PRINT:

                /* Call the worker routine */
                Eax = KdpPrint(ContextRecord->Ebx,
                               ContextRecord->Edi,
                               (LPSTR)ExceptionRecord->ExceptionInformation[1],
                               (ULONG)ExceptionRecord->ExceptionInformation[2],
                               PreviousMode,
                               TrapFrame,
                               ExceptionFrame,
                               &Status);

                /* Update the return value for the caller */
                ContextRecord->Eax = Eax;
                break;

            /* DbgPrompt */
            case BREAKPOINT_PROMPT:

                /* Call the worker routine */
                while (TRUE);
                Eax = 0;
                Status = TRUE;

                /* Update the return value for the caller */
                ContextRecord->Eax = Eax;
                break;

            /* DbgUnloadSymbols */
            case BREAKPOINT_UNLOAD_SYMBOLS:

                /* Drop into the load case below, with the unload parameter */
                Unload = TRUE;

            /* DbgLoadSymbols */
            case BREAKPOINT_LOAD_SYMBOLS:

                /* Call the worker routine */
                KdpSymbol((PVOID)ExceptionRecord->ExceptionInformation[1],
                          (PVOID)ExceptionRecord->ExceptionInformation[2],
                          Unload,
                          PreviousMode,
                          ContextRecord,
                          TrapFrame,
                          ExceptionFrame);
                Status = TRUE;
                break;

            /* DbgCommandString*/
            case BREAKPOINT_COMMAND_STRING:

                /* Call the worker routine */
                while (TRUE);
                Status = TRUE;

            /* Anything else, do nothing */
            default:

                /* Get out */
                break;
        }

        /*
         * If EIP was not updated, we'll increment it ourselves so execution
         * continues past the breakpoint.
         */
        if (ContextRecord->Eip == Eip) ContextRecord->Eip++;
    }
    else
    {
        /* Call the worker routine */
        Status = KdpReport(TrapFrame,
                           ExceptionFrame,
                           ExceptionRecord,
                           ContextRecord,
                           PreviousMode,
                           SecondChanceException);
    }

    /* Return TRUE or FALSE to caller */
    return Status;
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
    ULONG ExceptionCommand = ExceptionRecord->ExceptionInformation[0];

    /* Check if this was a breakpoint due to DbgPrint or Load/UnloadSymbols */
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionCommand == BREAKPOINT_LOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_UNLOAD_SYMBOLS) ||
         (ExceptionCommand == BREAKPOINT_COMMAND_STRING) ||
         (ExceptionCommand == BREAKPOINT_PRINT)))
    {
        /* This we can handle: simply bump EIP */
        ContextRecord->Eip++;
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
             (KdEnableDebugger()) &&
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
