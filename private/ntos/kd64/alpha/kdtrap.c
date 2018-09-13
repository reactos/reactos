/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992-1993  Digital Equipment Corporation

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

Author:

    David N. Cutler 27-July-1990
    Joe Notarangelo 24-June-1992  (ALPHA version)

Revision History:

--*/

#include "kdp.h"


BOOLEAN
KdpTrap (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This routine is called whenever a exception is dispatched and the kernel
    debugger is active.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to a exception frame that describes
        the trap.

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

    SecondChance - Supplies a boolean value that determines whether this is
        the second chance (TRUE) that the exception has been raised.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    BOOLEAN Completion;
    BOOLEAN Enable;
    BOOLEAN UnloadSymbols = FALSE;
    STRING Input;
    ULONGLONG OldFir;
    STRING Output;
    PKPRCB Prcb;

    //
    // Enter debugger and synchronize processor execution.
    //

    //
    // If this is a breakpoint instruction, then check to determine if is
    // an internal command.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] >= DEBUG_PRINT_BREAKPOINT)){

        //
        // Switch on the breakpoint code.
        //

        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            // Print a debug string.
            //
            // Arguments:
            //
            //   a0 - Supplies a pointer to an output string buffer.
            //   a1 - Supplies the length of the output string buffer.
            //

        case DEBUG_PRINT_BREAKPOINT:

            ContextRecord->Fir += 4;
            Output.Buffer = (PCHAR)ContextRecord->IntA0;
            Output.Length = (USHORT)ContextRecord->IntA1;

            KdLogDbgPrint(&Output);

            if (KdDebuggerNotPresent == FALSE) {

                Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
                if (KdpPrintString(&Output)) {
                    ContextRecord->IntV0 = (ULONG)STATUS_BREAKPOINT;
                } else {
                    ContextRecord->IntV0 = (ULONG)STATUS_SUCCESS;
                }
                KdExitDebugger(Enable);

            } else {
                ContextRecord->IntV0 = (ULONG)STATUS_DEVICE_NOT_CONNECTED;
            }

            return TRUE;


            //
            // Stop in the debugger
            //
            // As this is not a normal breakpoint we must increment the
            // context past the breakpoint instruction
            //

        case BREAKIN_BREAKPOINT:

            ContextRecord->Fir += 4;
            break;

            //
            // Print a debug prompt string, then input a string.
            //
            //   a0 - Supplies a pointer to an output string buffer.
            //   a1 - Supplies the length of the output string buffer..
            //   a2 - supplies a pointer to an input string buffer.
            //   a3 - Supplies the length of the input string bufffer.
            //

        case DEBUG_PROMPT_BREAKPOINT:

            ContextRecord->Fir += 4;
            Output.Buffer = (PCHAR)ContextRecord->IntA0;
            Output.Length = (USHORT)ContextRecord->IntA1;
            Input.Buffer = (PCHAR)ContextRecord->IntA2;
            Input.MaximumLength = (USHORT)ContextRecord->IntA3;

            KdLogDbgPrint(&Output);

            Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

            KdpPromptString(&Output, &Input);

            ContextRecord->IntV0 = Input.Length;

            KdExitDebugger(Enable);
            return TRUE;

            //
            // Load the symbolic information for an image.
            //
            // Arguments:
            //
            //    a0 - Supplies a pointer to an output string descriptor.
            //    a1 - Supplies a the base address of the image.
            //    a2 - Supplies the current process id.
            //

        case DEBUG_UNLOAD_SYMBOLS_BREAKPOINT:

            UnloadSymbols = TRUE;

            //
            // Fall through
            //

        case DEBUG_LOAD_SYMBOLS_BREAKPOINT:

            Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
            Prcb = KeGetCurrentPrcb();
            OldFir = ContextRecord->Fir;
            RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                          ContextRecord,
                          sizeof(CONTEXT));

            if (KdDebuggerNotPresent == FALSE) {

                KdpReportLoadSymbolsStateChange((PSTRING)ContextRecord->IntA0,
                                                (PKD_SYMBOLS_INFO) ContextRecord->IntA1,
                                                UnloadSymbols,
                                                &Prcb->ProcessorState.ContextFrame);

            }

            RtlCopyMemory(ContextRecord,
                          &Prcb->ProcessorState.ContextFrame,
                          sizeof(CONTEXT));

            KdExitDebugger(Enable);

            //
            // If the kernel debugger did not update the FIR, then increment
            // past the breakpoint instruction.
            //

            if (ContextRecord->Fir == OldFir) {
                ContextRecord->Fir += 4;
            }

            return TRUE;

            //
            // Unknown internal command.
            //

        default:

            break;
        }
    }

    //
    // Report state change to kernel debugger on host machine.
    //

    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
    Prcb = KeGetCurrentPrcb();

    RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                    ContextRecord,
                    sizeof (CONTEXT));

    Completion = KdpReportExceptionStateChange(ExceptionRecord,
                                               &Prcb->ProcessorState.ContextFrame,
                                               SecondChance);

    RtlCopyMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));

    KdExitDebugger(Enable);

    KdpControlCPressed = FALSE;

    //
    // Always return TRUE if this is the first chance to handle the
    // exception.  Otherwise, return the completion status of the
    // state change reporting.
    //

    if( SecondChance ){
        return Completion;
    } else {
        return TRUE;
    }
}

BOOLEAN
KdIsThisAKdTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This routine is called whenever a user-mode exception occurs and
    it might be a kernel debugger exception (Like DbgPrint/DbgPrompt ).

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

Return Value:

    A value of TRUE is returned if this is for the kernel debugger.
    Otherwise, a value of FALSE is returned.

--*/

{

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    //
    // Switch on the breakpoint code.
    //

    switch (ExceptionRecord->ExceptionInformation[0]) {

        //
        // Kernel breakpoint code.
        //

    case KERNEL_BREAKPOINT:
    case BREAKIN_BREAKPOINT:
#if DEVL
        return TRUE;
#else
        if (PreviousMode == KernelMode) {
            return TRUE;

        } else {
            return FALSE;
        }
#endif

        //
        // Debug print code.
        //

    case DEBUG_PRINT_BREAKPOINT:
        return TRUE;

        //
        // Debug prompt code.
        //
    case DEBUG_PROMPT_BREAKPOINT:
        return TRUE;

        //
        // Debug stop code.
        //

    case DEBUG_STOP_BREAKPOINT:
#if DEVL
        return TRUE;
#else
        if (PreviousMode == KernelMode) {
            return TRUE;

        } else {
            return FALSE;
        }
#endif

        //
        // Debug load symbols code.
        //

    case DEBUG_LOAD_SYMBOLS_BREAKPOINT:
        if (PreviousMode == KernelMode) {
            return TRUE;

        } else {
            return FALSE;
        }

        //
        // Debug unload symbols code.
        //

    case DEBUG_UNLOAD_SYMBOLS_BREAKPOINT:
        if (PreviousMode == KernelMode) {
            return TRUE;

        } else {
            return FALSE;
        }
        //
        // All other codes.
        //

    default:
        return FALSE;
    }
}

BOOLEAN
KdpStub (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    This routine provides a kernel debugger stub routine that catchs debug
    prints in checked systems when the kernel debugger is not active.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to a exception frame that describes
        the trap.

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

    SecondChance - Supplies a boolean value that determines whether this is
        the second chance (TRUE) that the exception has been raised.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    ULONG_PTR BreakpointCode;

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = ExceptionRecord->ExceptionInformation[0];

    //
    // If the breakpoint is a debug print, debug load symbols, or debug
    // unload symbols, then return TRUE. Otherwise, return FALSE;
    //

    if ((BreakpointCode == DEBUG_PRINT_BREAKPOINT) ||
        (BreakpointCode == DEBUG_LOAD_SYMBOLS_BREAKPOINT) ||
        (BreakpointCode == DEBUG_UNLOAD_SYMBOLS_BREAKPOINT) ||
        (BreakpointCode == KERNEL_BREAKPOINT)) {
        ContextRecord->Fir += 4;
        return TRUE;
    } else {
        if ( (BreakpointCode == DEBUG_STOP_BREAKPOINT) &&
             (PreviousMode == KernelMode) ){
             ContextRecord->Fir += 4;
             return TRUE;
        } else {
            return FALSE;
        }
    }
}
