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
#if defined (_M_X86)
        ContextRecord->Eip++;
#elif defined (_M_AMD64)
        ContextRecord->Rip++;
#else
#error Unknown platform
#endif
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
