/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdtrap.c

Abstract:

    This module contains code to implement the target side of the portable
    kernel debugger.

Author:

    Bryan M. Willman (bryanwi) 25-Sep-90

Revision History:

--*/

#include "kdp.h"

//
// externs
//
extern PUCHAR  KdpCopyDataToStack(PUCHAR, ULONG);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdpTrap)
#pragma alloc_text(PAGEKD, KdIsThisAKdTrap)
#endif




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
    ULONG   RetValue;
    STRING  String, ReplyString;
    PUCHAR  Buffer;
    PKD_SYMBOLS_INFO SymbolInfo;
    PVOID   SavedEsp;
    PKPRCB  Prcb;
    ULONG   OldEip;

    _asm {
        //
        // Save esp on ebp frame so c-runtime registers are restored correctly
        //

        mov     SavedEsp, esp
    }

    //
    // Print, Prompt, Load symbols, Unload symbols, are all special
    // cases of STATUS_BREAKPOINT
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK)) {

        //
        // We have one of the support functions.
        //

        if (KdDebuggerNotPresent  &&
            ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_PROMPT) {
            ContextRecord->Eip++;
            return(TRUE);
        }


        //
        // Since some of these functions can be entered from user mode,
        // we hold off entering the debugger until the user mode buffers
        // are copied.  (Because they may not be present in memory, and
        // they must be paged in before we raise Irql to the
        // Highest level.)
        //
        //

        OldEip = ContextRecord->Eip;

        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            //  ExceptionInformation[1] is PSTRING to print
            //

            case BREAKPOINT_PRINT:

                if (PreviousMode == UserMode) {

                    EXCEPTION_RECORD exr;
                    //
                    // Move user mode parameters to kernel stack
                    //

                    try {
                        String = *((PSTRING)ExceptionRecord->ExceptionInformation[1]);
                        if (String.Length > 512) {
                            break;
                        }
                        ProbeForRead(String.Buffer, String.Length, sizeof(UCHAR));
                        String.Buffer =
                            KdpCopyDataToStack(String.Buffer, String.Length);

                    } except ((exr = *((GetExceptionInformation())->ExceptionRecord), EXCEPTION_EXECUTE_HANDLER)) {

                        //
                        // If an exception occurs then don't handle
                        // this DebugService request.
                        //

                        break;
                    }

                } else {
                    String = *((PSTRING)ExceptionRecord->ExceptionInformation[1]);
                }

                KdLogDbgPrint(&String);

                if ((NtGlobalFlag & FLG_DISABLE_DBGPRINT) == 0) {
                    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
                    if (KdpPrintString(&String)) {
                        ContextRecord->Eax = (ULONG)(STATUS_BREAKPOINT);
                    } else {
                        ContextRecord->Eax = STATUS_SUCCESS;
                    }
                    KdExitDebugger(Enable);
                }

                Completion = TRUE;
                break;

            //
            //  ExceptionInformation[1] is prompt string,
            //  ExceptionInformation[2] is return string
            //

            case BREAKPOINT_PROMPT:
                if (PreviousMode == UserMode) {

                    //
                    // Move user mode parameters to kernel stack
                    //


                    try {
                        String = *((PSTRING)ExceptionRecord->ExceptionInformation[1]);
                        if (String.Length > 512) {
                            break;
                        }
                        ProbeForRead(String.Buffer, String.Length, sizeof(CHAR));
                        String.Buffer =
                            KdpCopyDataToStack(String.Buffer, String.Length);

                        ReplyString = *((PSTRING)ExceptionRecord->ExceptionInformation[2]);
                        if (ReplyString.MaximumLength > 512) {
                            break;
                        }
                        ProbeForWrite(ReplyString.Buffer,
                                      ReplyString.MaximumLength,
                                      sizeof(CHAR));
                        Buffer = ReplyString.Buffer;
                        ReplyString.Buffer =
                            KdpCopyDataToStack(
                                ReplyString.Buffer,
                                ReplyString.MaximumLength
                            );

                    } except (EXCEPTION_EXECUTE_HANDLER) {

                        //
                        // If an exception occurs then don't handle
                        // this DebugService request.
                        //

                        break;
                    }
                } else {
                    String = *((PSTRING)ExceptionRecord->ExceptionInformation[1]);
                    ReplyString = *((PSTRING)ExceptionRecord->ExceptionInformation[2]);
                }

                //
                // Prompt, keep prompting until no breakin seen.
                //

                KdLogDbgPrint(&String);

                Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
                do {
                    RetValue = KdpPromptString(&String, &ReplyString);
                } while (RetValue == TRUE);

                ContextRecord->Eax = ReplyString.Length;
                KdExitDebugger(Enable);

                if (PreviousMode == UserMode) {

                    //
                    // Restore user mode return parameters
                    //

                    try {
                        KdpQuickMoveMemory(
                            Buffer,
                            ReplyString.Buffer,
                            ReplyString.Length
                        );
                    } except (EXCEPTION_EXECUTE_HANDLER) {

                        //
                        // If an exception occurs then don't handle
                        // this DebugService request.
                        //

                        break;
                    }
                }

                Completion = TRUE;
                break;

            //
            //  ExceptionInformation[1] is file name of new module
            //  ExceptionInformaiton[2] is the base of the dll
            //

            case BREAKPOINT_UNLOAD_SYMBOLS:
                UnloadSymbols = TRUE;

                //
                // Fall through
                //

            case BREAKPOINT_LOAD_SYMBOLS:

                if (PreviousMode != KernelMode) {
                    break;
                }

                Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

                //
                // Save and restore the processor context in case the
                // kernel debugger has been configured to stop on dll
                // loads.
                //

                Prcb = KeGetCurrentPrcb();
                KiSaveProcessorControlState(&Prcb->ProcessorState);
                RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                              ContextRecord,
                              sizeof(CONTEXT));

                SymbolInfo = (PKD_SYMBOLS_INFO)ExceptionRecord->ExceptionInformation[2];
                Completion =
                    KdpReportLoadSymbolsStateChange((PSTRING)ExceptionRecord->ExceptionInformation[1],
                                                    SymbolInfo,
                                                    UnloadSymbols,
                                                    &Prcb->ProcessorState.ContextFrame);

                RtlCopyMemory(ContextRecord,
                              &Prcb->ProcessorState.ContextFrame,
                              sizeof (CONTEXT) );

                KiRestoreProcessorControlState(&Prcb->ProcessorState);

                KdExitDebugger(Enable);
                break;

            //
            //  Unknown command
            //

            default:
                // return FALSE
                break;
        }
        //
        // If the kernel debugger did not update the EIP, then increment
        // past the breakpoint instruction.
        //

        if (ContextRecord->Eip == OldEip) {
            ContextRecord->Eip++;
        }


    } else {

        if  ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) ||
             (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)  ||
             (NtGlobalFlag & FLG_STOP_ON_EXCEPTION) ||
             SecondChance) {

            if (!SecondChance &&
                (ExceptionRecord->ExceptionCode == STATUS_PORT_DISCONNECTED ||
                 NT_SUCCESS( ExceptionRecord->ExceptionCode )
                )
               ) {
                //
                // User does not really want to see these either.
                // so do NOT report it to debugger.
                //

                return FALSE;
                }

            //
            // Report state change to kernel debugger on host
            //


            Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
            Prcb = KeGetCurrentPrcb();
            KiSaveProcessorControlState(&Prcb->ProcessorState);
            RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                          ContextRecord,
                          sizeof (CONTEXT));

            Completion =
                KdpReportExceptionStateChange(ExceptionRecord,
                                              &Prcb->ProcessorState.ContextFrame,
                                              SecondChance);

            RtlCopyMemory(ContextRecord,
                          &Prcb->ProcessorState.ContextFrame,
                          sizeof(CONTEXT));

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
    }

    _asm {
        mov     esp, SavedEsp
    }
    return Completion;

    UNREFERENCED_PARAMETER(PreviousMode);
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
    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        (ExceptionRecord->ExceptionInformation[0] != BREAKPOINT_BREAK)) {

        return TRUE;
    } else {
        return FALSE;
    }
    UNREFERENCED_PARAMETER(ContextRecord);
}

BOOLEAN
KdpCheckTracePoint(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord
    );

VOID
SaveSymLoad(
    IN PSTRING PathName,
    IN PVOID BaseOfDll,
    IN LONG ProcessId,
    IN BOOLEAN UnloadSymbols
    );

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

    This routine provides a kernel debugger stub routine to catch debug
    prints in a checked system when the kernel debugger is not active.

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
    PULONG  SymbolArgs;
    //
    // If the breakpoint is a debug print, then return TRUE. Otherwise,
    // return FALSE.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->NumberParameters > 0) &&
        ((ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_LOAD_SYMBOLS)||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_UNLOAD_SYMBOLS)||
         (ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_PRINT))) {

        ContextRecord->Eip++;
        return(TRUE);
    } else if (KdPitchDebugger == TRUE) {
        return(FALSE);
    } else {
        return(KdpCheckTracePoint(ExceptionRecord,ContextRecord));
    }
}
