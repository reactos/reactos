/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    chandler.c

Abstract:

    This module implements the C specific exception handler that provides
    structured condition handling for the C language.

Author:

    David N. Cutler (davec) 11-Sep-1990

Environment:

    Any mode.

Revision History:

--*/

#include "nt.h"


//
// Define procedure prototypes for exception filter and termination handler
// execution routines defined in jmpunwnd.s
//

LONG
__C_ExecuteExceptionFilter (
    PEXCEPTION_POINTERS ExceptionPointers,
    EXCEPTION_FILTER ExceptionFilter,
    ULONG EstablisherFrame
    );

VOID
__C_ExecuteTerminationHandler (
    BOOLEAN AbnormalTermination,
    TERMINATION_HANDLER TerminationHandler,
    ULONG EstablisherFrame
    );

EXCEPTION_DISPOSITION
__C_specific_handler (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext
    )

/*++

Routine Description:

    This function scans the scope tables associated with the specified
    procedure and calls exception and termination handlers as necessary.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    EstablisherFrame - Supplies a pointer to frame of the establisher function.

    ContextRecord - Supplies a pointer to a context record.

    DispatcherContext - Supplies a pointer to the exception dispatcher or
        unwind dispatcher context.

Return Value:

    If the exception is handled by one of the exception filter routines, then
    there is no return from this routine and RtlUnwind is called. Otherwise,
    an exception disposition value of continue execution or continue search is
    returned.

--*/

{

    ULONG ControlPc;
    EXCEPTION_FILTER ExceptionFilter;
    EXCEPTION_POINTERS ExceptionPointers;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG Index;
    PSCOPE_TABLE ScopeTable;
    ULONG TargetPc;
    TERMINATION_HANDLER TerminationHandler;
    LONG Value;

    //
    // Get address of where control left the establisher, the address of the
    // function table entry that describes the function, and the address of
    // the scope table.
    //

    ControlPc = DispatcherContext->ControlPc;
    FunctionEntry = DispatcherContext->FunctionEntry;
    ScopeTable = (PSCOPE_TABLE)(FunctionEntry->HandlerData);

    //
    // If an unwind is not in progress, then scan the scope table and call
    // the appropriate exception filter routines. Otherwise, scan the scope
    // table and call the appropriate termination handlers using the target
    // PC obtained from the context record.
    // are called.
    //

    if (IS_DISPATCHING(ExceptionRecord->ExceptionFlags)) {

        //
        // Scan the scope table and call the appropriate exception filter
        // routines.
        //

        ExceptionPointers.ExceptionRecord = ExceptionRecord;
        ExceptionPointers.ContextRecord = ContextRecord;
        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress) &&
                (ScopeTable->ScopeRecord[Index].JumpTarget != 0)) {

                //
                // Call the exception filter routine.
                //

                ExceptionFilter =
                    (EXCEPTION_FILTER)ScopeTable->ScopeRecord[Index].HandlerAddress;
                Value = __C_ExecuteExceptionFilter(&ExceptionPointers,
                                                   ExceptionFilter,
                                                   (ULONG)EstablisherFrame);

                //
                // If the return value is less than zero, then dismiss the
                // exception. Otherwise, if the value is greater than zero,
                // then unwind to the target exception handler. Otherwise,
                // continue the search for an exception filter.
                //

                if (Value < 0) {
                    return ExceptionContinueExecution;

                } else if (Value > 0) {
                    RtlUnwind2(EstablisherFrame,
                               (PVOID)ScopeTable->ScopeRecord[Index].JumpTarget,
                               ExceptionRecord,
                               (PVOID)ExceptionRecord->ExceptionCode,
                               ContextRecord);
                }
            }
        }

    } else {

        //
        // Scan the scope table and call the appropriate termination handler
        // routines.
        //

        TargetPc = ContextRecord->Fir;
        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress)) {

                //
                // If the target PC is within the same scope the control PC
                // is within, then this is an uplevel goto out of an inner try
                // scope or a long jump back into a try scope. Terminate the
                // scan termination handlers.
                //
                // N.B. The target PC can be just beyond the end of the scope,
                //      in which case it is a leave from the scope.
                //


                if ((TargetPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                   (TargetPc <= ScopeTable->ScopeRecord[Index].EndAddress)) {
                    break;

                } else {

                    //
                    // If the scope table entry describes an exception filter
                    // and the associated exception handler is the target of
                    // the unwind, then terminate the scan for termination
                    // handlers. Otherwise, if the scope table entry describes
                    // a termination handler, then record the address of the
                    // end of the scope as the new control PC address and call
                    // the termination handler.
                    //

                    if (ScopeTable->ScopeRecord[Index].JumpTarget != 0) {
                        if (TargetPc == ScopeTable->ScopeRecord[Index].JumpTarget) {
                            break;
                        }

                    } else {
                        DispatcherContext->ControlPc =
                                ScopeTable->ScopeRecord[Index].EndAddress + 4;
                        TerminationHandler =
                            (TERMINATION_HANDLER)ScopeTable->ScopeRecord[Index].HandlerAddress;
                        __C_ExecuteTerminationHandler(TRUE,
                                                      TerminationHandler,
                                                      (ULONG)EstablisherFrame);
                    }
                }
            }
        }
    }

    //
    // Continue search for exception or termination handlers.
    //

    return ExceptionContinueSearch;
}
