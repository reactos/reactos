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

    Thomas Van Baak (tvb) 29-Apr-1992

        Adapted for Alpha AXP.

--*/

#include "nt.h"

//
// Define procedure prototypes for exception filter and termination handler
// execution routines defined in jmpuwind.s.
//

LONG
__C_ExecuteExceptionFilter (
    PEXCEPTION_POINTERS ExceptionPointers,
    EXCEPTION_FILTER ExceptionFilter,
    ULONG_PTR EstablisherFrame
    );

VOID
__C_ExecuteTerminationHandler (
    BOOLEAN AbnormalTermination,
    TERMINATION_HANDLER TerminationHandler,
    ULONG_PTR EstablisherFrame
    );

EXCEPTION_DISPOSITION
__C_specific_handler (
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN void *EstablisherFrame,
    IN OUT struct _CONTEXT *ContextRecord,
    IN OUT struct _DISPATCHER_CONTEXT *DispatcherContext
    )

/*++

Routine Description:

    This function scans the scope tables associated with the specified
    procedure and calls exception and termination handlers as necessary.

    This language specific exception handler function is called on a
    per-frame basis and in two different cases:

    First, the IS_DISPATCHING case, it is called by the exception
    dispatcher, RtlDispatchException, via the short assembler routine,
    __C_ExecuteHandlerForException, when trying to locate exception
    filters within the given frame.

    Second, the IS_UNWINDING case, it is called by the frame unwinder,
    RtlUnwind, via the short assembler routine, __C_ExecuteHandlerForUnwind,
    when unwinding the stack and trying to locate termination handlers
    within the given frame.

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

    ULONG_PTR ControlPc;
    EXCEPTION_FILTER ExceptionFilter;
    EXCEPTION_POINTERS ExceptionPointers;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG Index;
    PSCOPE_TABLE ScopeTable;
    ULONG_PTR TargetPc;
    TERMINATION_HANDLER TerminationHandler;
    LONG Value;

    //
    // Get the address of where control left the establisher, the address of
    // the function table entry that describes the function, and the address of
    // the scope table.
    //

    ControlPc = DispatcherContext->ControlPc;
    FunctionEntry = DispatcherContext->FunctionEntry;
    ScopeTable = (PSCOPE_TABLE)(FunctionEntry->HandlerData);

    //
    // The scope table HandlerAddress is either the address of an exception
    // filter or a termination handler. The C compiler wraps the code in the
    // exception filter expression or the termination handler clause within
    // an internal C function. The value of the scope table JumpTarget field
    // is used to distinguish an exception filter function (JumpTarget non zero)
    // from a termination handler function (JumpTarget is zero).
    //

    //
    // If an unwind is not in progress, then scan the scope table and call
    // the appropriate exception filter routines. Otherwise, scan the scope
    // table and call the appropriate termination handlers using the target
    // PC obtained from the context record.
    //

    if (IS_DISPATCHING(ExceptionRecord->ExceptionFlags)) {

        //
        // Set up the ExceptionPointers structure that is passed as the argument
        // to the exception filter. It is used by the compiler to implement the
        // intrinsic functions exception_code() and exception_info().
        //

        ExceptionPointers.ExceptionRecord = ExceptionRecord;
        ExceptionPointers.ContextRecord = ContextRecord;

        //
        // Scan the scope table and call the appropriate exception filter
        // routines. The scope table entries are known to be sorted by
        // increasing EndAddress order. Thus a linear scan will naturally
        // hit inner scope exception clauses before outer scope clauses.
        //

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
                                                   (ULONG_PTR)EstablisherFrame);

                //
                // If the return value is less than zero, then dismiss the
                // exception. Otherwise, if the value is greater than zero,
                // then unwind to the target exception handler corresponding
                // to the exception filter. Otherwise, continue the search for
                // an exception filter.
                //

                //
                // Exception filters will usually return one of the following
                // defines, although the decision below is made only by sign:
                //
                //  #define EXCEPTION_EXECUTE_HANDLER     1
                //  #define EXCEPTION_CONTINUE_SEARCH     0
                //  #define EXCEPTION_CONTINUE_EXECUTION -1
                //

                if (Value < 0) {
                    return ExceptionContinueExecution;

                } else if (Value > 0) {

                    //
                    // Set the return value for the unwind to the exception
                    // code so the exception handler clause can retrieve it
                    // from v0. This is how GetExceptionCode() is implemented
                    // in exception handler clauses.
                    //

                    RtlUnwind2(EstablisherFrame,
                               (PVOID)ScopeTable->ScopeRecord[Index].JumpTarget,
                               ExceptionRecord,
                               ULongToPtr( ExceptionRecord->ExceptionCode ),
                               ContextRecord);
                }
            }
        }

    } else {

        //
        // Scan the scope table and call the appropriate termination handler
        // routines.
        //

        TargetPc = (ULONG_PTR)ContextRecord->Fir;
        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress)) {

                //
                // If the target PC is within the same scope the control PC
                // is within, then this is an uplevel goto out of an inner try
                // scope or a long jump back into a try scope. Terminate the
                // scan for termination handlers - because any other handlers
                // will be outside the scope of both the goto and its label.
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
                    // Recording a new control PC is necessary to ensure that
                    // termination handlers are called only once even when a
                    // collided unwind occurs.
                    //

                    if (ScopeTable->ScopeRecord[Index].JumpTarget != 0) {

                        //
                        // try/except - exception filter (JumpTarget != 0).
                        // After the exception filter is called, the exception
                        // handler clause is executed by the call to unwind
                        // above. Having reached this point in the scan of the
                        // scope tables, any other termination handlers will
                        // be outside the scope of the try/except.
                        //

                        if (TargetPc == ScopeTable->ScopeRecord[Index].JumpTarget) {
                            break;
                        }

                    } else {

                        //
                        // try/finally - termination handler (JumpTarget == 0).
                        //

                        //
                        // Unless the termination handler results in a long
                        // jump, execution will resume at the instruction after
                        // the exception handler clause.
                        //
                        // ## tvb - I'm still suspicious of the +4 below.

                        DispatcherContext->ControlPc =
                                ScopeTable->ScopeRecord[Index].EndAddress + 4;
                        TerminationHandler =
                            (TERMINATION_HANDLER)ScopeTable->ScopeRecord[Index].HandlerAddress;
                        __C_ExecuteTerminationHandler(TRUE,
                                                      TerminationHandler,
                                                      (ULONG_PTR)EstablisherFrame);
                    }
                }
            }
        }
    }

    //
    // Continue search for exception filters or termination handlers.
    //

    return ExceptionContinueSearch;
}
