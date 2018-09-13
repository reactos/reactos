/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1998  Intel Corporation

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

Author:

    David N. Cutler 27-July-1990

Revision History:

--*/

#include "kdp.h"
#ifdef _GAMBIT_
#include "ssc.h"
#endif // _GAMBIT_


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

    BOOLEAN Completion = FALSE;
    BOOLEAN Enable;
    BOOLEAN UnloadSymbols = FALSE;
    ULONGLONG OldStIIP, OldStIPSR;
    STRING Input;
    STRING Output;
    PKPRCB Prcb;

    //
    // Synchronize processor execution, save processor state, enter debugger,
    // and flush the current TB.
    //

    KeFlushCurrentTb();

    //
    // If this is a breakpoint instruction, then check to determine if is
    // an internal command.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] >= DEBUG_PRINT_BREAKPOINT)) {

        //
        // Switch on the breakpoint code.
        //

        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            // Print a debug string.
            //
            // Arguments: IA64 passes arguments via RSE not GR's. Since arguments are not
            //            part of CONTEXT struct, they need to be copies Temp registers.
            //            (see NTOS/RTL/IA64/DEBUGSTB.S)
            //
            //   T0 - Supplies a pointer to an output string buffer.
            //   T1 - Supplies the length of the output string buffer.
            //

        case DEBUG_PRINT_BREAKPOINT:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);

            Output.Buffer = (PCHAR)ContextRecord->IntT0;
            Output.Length = (USHORT)ContextRecord->IntT1;

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
            // Print a debug prompt string, then input a string.
            //
            //   T0 - Supplies a pointer to an output string buffer.
            //   T1 - Supplies the length of the output string buffer..
            //   T2 - supplies a pointer to an input string buffer.
            //   T3 - Supplies the length of the input string bufffer.
            //

        case DEBUG_PROMPT_BREAKPOINT:

            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);

            Output.Buffer = (PCHAR)ContextRecord->IntT0;
            Output.Length = (USHORT)ContextRecord->IntT1;
            Input.Buffer = (PCHAR)ContextRecord->IntT2;
            Input.MaximumLength = (USHORT)ContextRecord->IntT3;

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
            //    T0 - Supplies a pointer to an output string descriptor.
            //    T1 - Supplies a the base address of the image.
            //

        case DEBUG_UNLOAD_SYMBOLS_BREAKPOINT:
            UnloadSymbols = TRUE;

            //
            // Fall through
            //

        case DEBUG_LOAD_SYMBOLS_BREAKPOINT:
    
            //
            // Advance to next instruction slot so that the BREAK instruction
            // does not get re-executed
            //

            Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
            Prcb = KeGetCurrentPrcb();
            KiSaveProcessorControlState(&Prcb->ProcessorState);
            OldStIPSR = ContextRecord->StIPSR;
            OldStIIP = ContextRecord->StIIP;
            RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                          ContextRecord,
                          sizeof(CONTEXT));

            if (KdDebuggerNotPresent == FALSE) {
                KdpReportLoadSymbolsStateChange((PSTRING)ContextRecord->IntT0,
                                                (PKD_SYMBOLS_INFO) ContextRecord->IntT1,
                                                UnloadSymbols,
                                                &Prcb->ProcessorState.ContextFrame);

            }

            RtlCopyMemory(ContextRecord,
                          &Prcb->ProcessorState.ContextFrame,
                          sizeof(CONTEXT));

            KiRestoreProcessorControlState(&Prcb->ProcessorState);
            KdExitDebugger(Enable);

            //
            // If the kernel debugger did not update the IP, then increment
            // past the breakpoint instruction.
            //

            if ((ContextRecord->StIIP == OldStIIP) &&
                ((ContextRecord->StIPSR & IPSR_RI_MASK) == (OldStIPSR & IPSR_RI_MASK))) { 
            	RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                               ContextRecord->StIPSR,
                               ContextRecord->StIIP);
            }

            return TRUE;

            //
            // Kernel breakin break
            //

        case BREAKIN_BREAKPOINT:

           //
           // Advance to next instruction slot so that the BREAK instruction
           // does not get re-executed
           //

           RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                              ContextRecord->StIPSR,
                              ContextRecord->StIIP);
           break;

            //
            // Unknown internal command.
            //

        default:
            break;
        }

    }

    //
    // Get here if single step or BREAKIN breakpoint
    //

    if  ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) ||
          (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)  ||
          (NtGlobalFlag & FLG_STOP_ON_EXCEPTION) ||
          SecondChance) {

         //
         // Report state change to kernel debugger on host
         //

         Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
         Prcb = KeGetCurrentPrcb();
         KiSaveProcessorControlState(&Prcb->ProcessorState);
          
         RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                          ContextRecord,
                          sizeof (CONTEXT));
      
         Completion = KdpReportExceptionStateChange(
                          ExceptionRecord,
                          &Prcb->ProcessorState.ContextFrame,
                          SecondChance);
      
         RtlCopyMemory(ContextRecord,
                          &Prcb->ProcessorState.ContextFrame,
                          sizeof (CONTEXT));
      
         KiRestoreProcessorControlState(&Prcb->ProcessorState);
         KdExitDebugger(Enable);
      
      
         KdpControlCPressed = FALSE;
     
    } else {

         //
         // This is real exception that user doesn't want to see,
         // so do NOT report it to debugger.
         //

         // return FALSE;
    }

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

    ULONG_PTR BreakpointCode;

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = (ULONG) ExceptionRecord->ExceptionInformation[0];

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
#ifdef _GAMBIT_
    PHYSICAL_ADDRESS PAddress;
#endif // _GAMBIT_

    //
    // Isolate the breakpoint code from the breakpoint instruction which
    // is stored by the exception dispatch code in the information field
    // of the exception record.
    //

    BreakpointCode = (ULONG) ExceptionRecord->ExceptionInformation[0];


    //
    // If the breakpoint is a debug print, debug load symbols, or debug
    // unload symbols, then return TRUE. Otherwise, return FALSE;
    //

    if ((BreakpointCode == DEBUG_PRINT_BREAKPOINT) ||
        (BreakpointCode == DEBUG_LOAD_SYMBOLS_BREAKPOINT) ||
        (BreakpointCode == DEBUG_UNLOAD_SYMBOLS_BREAKPOINT)) {

        //
        // Advance to next instruction slot so that the BREAK instruction
        // does not get re-executed
        //

        RtlIa64IncrementIP((ULONG_PTR)ExceptionRecord->ExceptionAddress >> 2,
                          ContextRecord->StIPSR,
                          ContextRecord->StIIP);


#ifdef _GAMBIT_
        if (BreakpointCode == DEBUG_PRINT_BREAKPOINT) {
            if ((PUCHAR)ContextRecord->IntT0) {
                PAddress = MmGetPhysicalAddress ((PUCHAR)ContextRecord->IntT0);
                if (PAddress.QuadPart != 0ULL) {
                    SscDbgPrintf((PVOID)PAddress.QuadPart);
                }
            }
        }
#endif // _GAMBIT_

        return TRUE;

    } else {
        return FALSE;
    }
}
