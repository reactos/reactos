/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

Author:

    Chuck Bauman 10-Jan-1994

Revision History:

    Based on David N. Cutler MIPS version 27-July-1990

--*/

#include "kdp.h"


//
// Define breakpoint instruction masks.
//
#define BREAKPOINT_CODE_MASK 0xffff

//
// globals
//
ULONG           KdpPageInAddress;
WORK_QUEUE_ITEM KdpPageInWorkItem;

//
// externs
//
extern  BOOLEAN KdpControlCPressed;



#pragma optimize( "", off )
VOID
KdpPageInData (
    IN PUCHAR volatile DataAddress
    )

/*++

Routine Description:

    This routine is called to page in data at the supplied address.
    It is called either directly from KdpTrap() or from a worker
    thread that is queued by KdpTrap().

Arguments:

    DataAddress - Supplies a pointer to the data to be paged in.

Return Value:

    None.

--*/

{
    if (MmIsSystemAddressAccessable(DataAddress)) {
        UCHAR c = *DataAddress;
        DataAddress = &c;
    }
    KdpControlCPending = TRUE;
}
#pragma optimize( "", on )


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
    ULONG OldIar;
    STRING Input;
    STRING Output;
    PKPRCB Prcb;

    //
    // Synchronize processor execution, save processor state, enter debugger,
    // and flush the current TB.
    //

re_enter_debugger:
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
    Prcb = KeGetCurrentPrcb();
    KiSaveProcessorState(TrapFrame, ExceptionFrame);
    KeFlushCurrentTb();

    //
    // If this is a breakpoint instruction, then check to determine if is
    // an internal command.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
       ((ExceptionRecord->ExceptionInformation[0] & BREAKPOINT_CODE_MASK)
                              >= DEBUG_PRINT_BREAKPOINT)) {

        //
        // Switch on the breakpoint code.
        //

        switch (ExceptionRecord->ExceptionInformation[0] & BREAKPOINT_CODE_MASK) {

            //
            // Print a debug string.
            //
            // Arguments:
            //
            //   a0 - Supplies a pointer to an output string buffer.
            //   a1 - Supplies the length of the output string buffer.
            //

        case DEBUG_PRINT_BREAKPOINT:
            ContextRecord->Iar += 4;
            Output.Buffer = (PCHAR)ContextRecord->Gpr3;
            Output.Length = (USHORT)ContextRecord->Gpr4;
            if (KdDebuggerNotPresent == FALSE) {
                if (KdpPrintString(&Output)) {
                    ContextRecord->Gpr3 = (ULONG)STATUS_BREAKPOINT;

                } else {
                    ContextRecord->Gpr3 = (ULONG)STATUS_SUCCESS;
                }

            } else {
                ContextRecord->Gpr3 = (ULONG)STATUS_DEVICE_NOT_CONNECTED;
            }

            KiRestoreProcessorState(TrapFrame, ExceptionFrame);
            KdExitDebugger(Enable);
            return TRUE;

            //
            // Print a debug prompt string, then input a string.
            //
            //   r.3 - Supplies a pointer to an output string buffer.
            //   r.4 - Supplies the length of the output string buffer..
            //   r.5 - supplies a pointer to an input string buffer.
            //   r.6 - Supplies the length of the input string bufffer.
            //

        case DEBUG_PROMPT_BREAKPOINT:
            ContextRecord->Iar += 4;
            Output.Buffer = (PCHAR)ContextRecord->Gpr3;
            Output.Length = (USHORT)ContextRecord->Gpr4;
            Input.Buffer = (PCHAR)ContextRecord->Gpr5;
            Input.MaximumLength = (USHORT)ContextRecord->Gpr6;
            KdpPromptString(&Output, &Input);
            ContextRecord->Gpr3 = Input.Length;
            KiRestoreProcessorState(TrapFrame, ExceptionFrame);
            KdExitDebugger(Enable);
            return TRUE;

            //
            // Load the symbolic information for an image.
            //
            // Arguments:
            //
            //    r.3 - Supplies a pointer to an output string descriptor.
            //    r.4 - Supplies a the base address of the image.
            //

        case DEBUG_UNLOAD_SYMBOLS_BREAKPOINT:
            UnloadSymbols = TRUE;

            //
            // Fall through
            //

        case DEBUG_LOAD_SYMBOLS_BREAKPOINT:
            OldIar = ContextRecord->Iar;
            if (KdDebuggerNotPresent == FALSE) {
                KdpReportLoadSymbolsStateChange((PSTRING)ContextRecord->Gpr3,
                                                (PKD_SYMBOLS_INFO) ContextRecord->Gpr4,
                                                UnloadSymbols,
                                                &Prcb->ProcessorState.ContextFrame);

            }

            RtlCopyMemory(ContextRecord,
                          &Prcb->ProcessorState.ContextFrame,
                          sizeof(CONTEXT));

            KiRestoreProcessorState(TrapFrame, ExceptionFrame);
            KdExitDebugger(Enable);

            //
            // If the kernel debugger did not update the IAR, then increment
            // past the breakpoint instruction.
            //

            if (ContextRecord->Iar == OldIar) {
                ContextRecord->Iar += 4;
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

    Completion = KdpReportExceptionStateChange(
                    ExceptionRecord,
                    &Prcb->ProcessorState.ContextFrame,
                    SecondChance);

    RtlCopyMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));

    KiRestoreProcessorState(TrapFrame, ExceptionFrame);
    KdExitDebugger(Enable);

    //
    // check to see if the user of the remote debugger
    // requested memory to be paged in
    //
    if (KdpPageInAddress) {

        if (KeGetCurrentIrql() <= APC_LEVEL) {

            //
            // if the IQRL is below DPC level then cause
            // the page fault to occur and then re-enter
            // the debugger.  this whole process is transparent
            // to the user.
            //
            KdpPageInData( (PUCHAR)KdpPageInAddress );
            KdpPageInAddress = 0;
            KdpControlCPending = FALSE;
            goto re_enter_debugger;

        } else {

            //
            // we cannot take a page fault
            // here so a worker item is queued to take the
            // page fault.  after the worker item takes the
            // page fault it sets the contol-c flag so that
            // the user re-enters the debugger just as if
            // control-c was pressed.
            //
            if (KdpControlCPressed) {
                ExInitializeWorkItem(
                    &KdpPageInWorkItem,
                    (PWORKER_THREAD_ROUTINE) KdpPageInData,
                    (PVOID) KdpPageInAddress
                    );
                ExQueueWorkItem( &KdpPageInWorkItem, DelayedWorkQueue );
                KdpPageInAddress = 0;
            }
        }
    }

    KdpControlCPressed = FALSE;

    return Completion;
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

    ULONG BreakpointCode;

    //
    // Determine if this is hardware debug register breakpoint
    //

    if (ContextRecord->Dr6 !=0) {      // Debug Register Breakpoint
        if ((PreviousMode == KernelMode) ||
            (KeGetCurrentThread()->DebugActive == 0)) { // No DR set for thread
            return TRUE;
        } else {                       // User mode and DR set for thread
            return FALSE;
        }
    }

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = ExceptionRecord->ExceptionInformation[0] & BREAKPOINT_CODE_MASK;

    //
    // Switch on the breakpoint code.
    //

    switch (BreakpointCode) {

        //
        // Kernel breakpoint codes.
        //

    case BREAKIN_BREAKPOINT:
    case KERNEL_BREAKPOINT:

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

    case SINGLE_STEP_BREAKPOINT:
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

    ULONG BreakpointCode;

    //
    // If it isn't a real breakpoint or doesn't have any params, get out.
    //

    if ((ExceptionRecord->ExceptionCode != STATUS_BREAKPOINT) ||
        (ExceptionRecord->NumberParameters == 0)) {
        return FALSE;
    }

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = ExceptionRecord->ExceptionInformation[0] & BREAKPOINT_CODE_MASK;


    //
    // If the breakpoint is a debug print, debug load symbols, or debug
    // unload symbols, then return TRUE. Otherwise, return FALSE;
    //

    if ((BreakpointCode == DEBUG_PRINT_BREAKPOINT) ||
        (BreakpointCode == DEBUG_LOAD_SYMBOLS_BREAKPOINT) ||
        (BreakpointCode == DEBUG_UNLOAD_SYMBOLS_BREAKPOINT)) {
        ContextRecord->Iar += 4;
        return TRUE;

    } else {
        return FALSE;
    }
}
