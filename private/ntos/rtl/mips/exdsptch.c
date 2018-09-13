/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    exdsptch.c

Abstract:

    This module implements the dispatching of exception and the unwinding of
    procedure call frames.

Author:

    David N. Cutler (davec) 11-Sep-1990

Environment:

    Any mode.

Revision History:

--*/

#include "ntrtlp.h"

//
// Define local macros.
//
// Raise noncontinuable exception with associated exception record.
//

#define RAISE_EXCEPTION(Status, ExceptionRecordt) { \
    EXCEPTION_RECORD ExceptionRecordn; \
                                            \
    ExceptionRecordn.ExceptionCode = Status; \
    ExceptionRecordn.ExceptionFlags = EXCEPTION_NONCONTINUABLE; \
    ExceptionRecordn.ExceptionRecord = ExceptionRecordt; \
    ExceptionRecordn.NumberParameters = 0; \
    RtlRaiseException(&ExceptionRecordn); \
    }

//
// Define stack register and zero register numbers.
//

#define RA 0x1f                         // integer register 31
#define SP 0x1d                         // integer register 29
#define ZERO 0x0                        // integer register 0

//
// Define saved register masks.
//

#define SAVED_FLOATING_MASK 0xfff00000  // saved floating registers
#define SAVED_INTEGER_MASK 0xf3ffff02   // saved integer registers

//
// Define private function prototypes.
//

VOID
RtlpRestoreContext (
    IN PCONTEXT Context,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL
    );

VOID
RtlpRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    );

VOID
RtlpRaiseStatus (
    IN NTSTATUS Status
    );

ULONG
RtlpVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    );

ULONG
RtlpVirtualUnwind32 (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    );


BOOLEAN
RtlDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function attempts to dispatch an exception to a frame based
    handler by searching backwards through the stack based call frames.
    The search begins with the frame specified in the context record and
    continues backward until either a handler is found that handles the
    exception, the stack is found to be invalid (i.e., out of limits or
    unaligned), or the end of the call hierarchy is reached.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called. If the
    handler does not handle the exception, then the prologue of the routine
    is executed backwards to "unwind" the effect of the prologue and then
    the next frame is examined.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    If the exception is handled by one of the frame based handlers, then
    a value of TRUE is returned. Otherwise a value of FALSE is returned.

--*/

{

    CONTEXT ContextRecord1;
    ULONG ControlPc;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    ULONG EstablisherFrame;
    ULONG ExceptionFlags;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG Index;
    BOOLEAN InFunction;
    ULONG HighLimit;
    ULONG LowLimit;
    ULONG NestedFrame;
    ULONG NextPc;

    //
    // Get current stack limits, copy the context record, get the initial
    // PC value, capture the exception flags, and set the nested exception
    // frame pointer.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlMoveMemory(&ContextRecord1, ContextRecord, sizeof(CONTEXT));
    ControlPc = ContextRecord1.Fir;
    ExceptionFlags = ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;
    NestedFrame = 0;

    //
    // Start with the frame specified by the context record and search
    // backwards through the call frame hierarchy attempting to find an
    // exception handler that will handle the exception.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the virtual
        // frame pointer of the establisher and check if there is an exception
        // handler for the frame.
        //

        if (FunctionEntry != NULL) {
            NextPc = RtlVirtualUnwind(ControlPc | 1,
                                      FunctionEntry,
                                      &ContextRecord1,
                                      &InFunction,
                                      &EstablisherFrame,
                                      NULL);

            //
            // If the virtual frame pointer is not within the specified stack
            // limits or the virtual frame pointer is unaligned, then set the
            // stack invalid flag in the exception record and return exception
            // not handled. Otherwise, check if the current routine has an
            // exception handler.
            //

            if ((EstablisherFrame < LowLimit) || (EstablisherFrame > HighLimit) ||
               ((EstablisherFrame & 0x7) != 0)) {
                ExceptionFlags |= EXCEPTION_STACK_INVALID;
                break;

            } else if ((FunctionEntry->ExceptionHandler != NULL) && InFunction) {

                //
                // The frame has an exception handler. The handler must be
                // executed by calling another routine that is written in
                // assembler. This is required because up level addressing
                // of the handler information is required when a nested
                // exception is encountered.
                //

                DispatcherContext.ControlPc = ControlPc;
                DispatcherContext.FunctionEntry = FunctionEntry;
                DispatcherContext.EstablisherFrame = EstablisherFrame;
                DispatcherContext.ContextRecord = ContextRecord;
                ExceptionRecord->ExceptionFlags = ExceptionFlags;

                //
                // If requested log exception.
                //

                if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
                    Index = RtlpLogExceptionHandler(ExceptionRecord,
                                                    ContextRecord,
                                                    ControlPc,
                                                    FunctionEntry,
                                                    sizeof(RUNTIME_FUNCTION));
                }

                Disposition =
                    RtlpExecuteHandlerForException(ExceptionRecord,
                                                   EstablisherFrame,
                                                   ContextRecord,
                                                   &DispatcherContext,
                                                   FunctionEntry->ExceptionHandler);

                if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
                    RtlpLogLastExceptionDisposition(Index, Disposition);
                }

                ExceptionFlags |=
                    (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

                //
                // If the current scan is within a nested context and the frame
                // just examined is the end of the nested region, then clear
                // the nested context frame and the nested exception flag in
                // the exception flags.
                //

                if (NestedFrame == EstablisherFrame) {
                    ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
                    NestedFrame = 0;
                }

                //
                // Case on the handler disposition.
                //

                switch (Disposition) {

                    //
                    // The disposition is to continue execution.
                    //
                    // If the exception is not continuable, then raise the
                    // exception STATUS_NONCONTINUABLE_EXCEPTION. Otherwise
                    // return exception handled.
                    //

                case ExceptionContinueExecution :
                    if ((ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0) {
                        RAISE_EXCEPTION(STATUS_NONCONTINUABLE_EXCEPTION, ExceptionRecord);

                    } else {
                        return TRUE;
                    }

                    //
                    // The disposition is to continue the search.
                    //
                    // Get next frame address and continue the search.
                    //

                case ExceptionContinueSearch :
                    break;

                    //
                    // The disposition is nested exception.
                    //
                    // Set the nested context frame to the establisher frame
                    // address and set the nested exception flag in the
                    // exception flags.
                    //

                case ExceptionNestedException :
                    ExceptionFlags |= EXCEPTION_NESTED_CALL;
                    if (DispatcherContext.EstablisherFrame > NestedFrame) {
                        NestedFrame = DispatcherContext.EstablisherFrame;
                    }

                    break;

                    //
                    // All other disposition values are invalid.
                    //
                    // Raise invalid disposition exception.
                    //

                default :
                    RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                }
            }

        } else {

            //
            // Set point at which control left the previous routine.
            //

            NextPc = (ULONG)(ContextRecord1.XIntRa - 4);

            //
            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            //

            if (NextPc == ControlPc) {
                break;
            }
        }

        //
        // Set point at which control left the previous routine.
        //

        ControlPc = NextPc;
    } while ((ULONG)ContextRecord1.XIntSp < HighLimit);

    //
    // Set final exception flags and return exception not handled.
    //

    ExceptionRecord->ExceptionFlags = ExceptionFlags;
    return FALSE;
}

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{

    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    ULONG SizeOfExceptionTable;
    LONG High;
    PVOID ImageBase;
    LONG Low;
    LONG Middle;
    USHORT i;

    //
    // Search for the image that includes the specified PC value.
    //

    ImageBase = RtlPcToFileHeader((PVOID)ControlPc, &ImageBase);

    //
    // If an image is found that includes the specified PC, then locate the
    // function table for the image.
    //

    if (ImageBase != NULL) {
        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                         ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                         &SizeOfExceptionTable);

        //
        // If a function table is located, then search the function table
        // for a function table entry for the specified PC.
        //

        if (FunctionTable != NULL) {

            //
            // Initialize search indicies.
            //

            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;

            //
            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            //

            while (High >= Low) {

                //
                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                //

                Middle = (Low + High) >> 1;
                FunctionEntry = &FunctionTable[Middle];
                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;

                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;

                } else {

                    //
                    // The capability exists for more than one function entry
                    // to map to the same function. This permits a function to
                    // have discontiguous code segments described by separate
                    // function table entries. If the ending prologue address
                    // is not within the limits of the begining and ending
                    // address of the function able entry, then the prologue
                    // ending address is the address of a function table entry
                    // that accurately describes the ending prologue address.
                    //

                    if ((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                        (FunctionEntry->PrologEndAddress > FunctionEntry->EndAddress)) {
                        FunctionEntry = (PRUNTIME_FUNCTION)FunctionEntry->PrologEndAddress;
                    }

                    return FunctionEntry;
                }
            }
        }
    }

    //
    // A function table entry for the specified PC was not found.
    //

    return NULL;
}

VOID
RtlRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    )

/*++

Routine Description:

    This function raises a software exception by building a context record
    and calling the raise exception system service.

    N.B. This routine is a shell routine that simply calls another routine
         to do the real work. The reason this is done is to avoid a problem
         in try/finally scopes where the last statement in the scope is a
         call to raise an exception.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

Return Value:

    None.

--*/

{

    RtlpRaiseException(ExceptionRecord);
    return;
}

VOID
RtlpRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    )

/*++

Routine Description:

    This function raises a software exception by building a context record
    and calling the raise exception system service.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

Return Value:

    None.

--*/

{

    ULONG ControlPc;
    CONTEXT ContextRecord;
    ULONG EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG NextPc;
    NTSTATUS Status;

    //
    // Capture the current context, virtually unwind to the caller of this
    // routine, set the fault instruction address to that of the caller, and
    // call the raise exception system service.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = (ULONG)(ContextRecord.XIntRa - 4);
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc | 1,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ContextRecord.Fir = NextPc + 4;
    ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.Fir;
    Status = ZwRaiseException(ExceptionRecord, &ContextRecord, TRUE);

    //
    // There should never be a return from this system service unless
    // there is a problem with the argument list itself. Raise another
    // exception specifying the status value returned.
    //

    RtlRaiseStatus(Status);
    return;
}

VOID
RtlRaiseStatus (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This function raises an exception with the specified status value. The
    exception is marked as noncontinuable with no parameters.

    N.B. This routine is a shell routine that simply calls another routine
         to do the real work. The reason this is done is to avoid a problem
         in try/finally scopes where the last statement in the scope is a
         call to raise an exception.

Arguments:

    Status - Supplies the status value to be used as the exception code
        for the exception that is to be raised.

Return Value:

    None.

--*/

{

    RtlpRaiseStatus(Status);
    return;
}

VOID
RtlpRaiseStatus (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This function raises an exception with the specified status value. The
    exception is marked as noncontinuable with no parameters.

Arguments:

    Status - Supplies the status value to be used as the exception code
        for the exception that is to be raised.

Return Value:

    None.

--*/

{

    ULONG ControlPc;
    CONTEXT ContextRecord;
    ULONG EstablisherFrame;
    EXCEPTION_RECORD ExceptionRecord;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG NextPc;

    //
    // Construct an exception record.
    //

    ExceptionRecord.ExceptionCode = Status;
    ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    //
    // Capture the current context, virtually unwind to the caller of this
    // routine, set the fault instruction address to that of the caller, and
    // call the raise exception system service.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = (ULONG)(ContextRecord.XIntRa - 4);
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc | 1,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ContextRecord.Fir = NextPc + 4;
    ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Fir;
    Status = ZwRaiseException(&ExceptionRecord, &ContextRecord, TRUE);

    //
    // There should never be a return from this system service unless
    // there is a problem with the argument list itself. Raise another
    // exception specifying the status value returned.
    //

    RtlRaiseStatus(Status);
    return;
}

VOID
RtlUnwind (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    scan through the procedure call frames is then performed to find the target
    of the unwind operation.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

    N.B. This routine is provided for backward compatibility with release 1.

Arguments:

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;

    //
    // Call real unwind routine specifying a context record as an
    // extra argument.
    //

    RtlUnwind2(TargetFrame,
               TargetIp,
               ExceptionRecord,
               ReturnValue,
               &ContextRecord);

    return;
}

VOID
RtlUnwind2 (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    scan through the procedure call frames is then performed to find the target
    of the unwind operation.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

Arguments:

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

    ContextRecord - Supplies a pointer to a context record that can be used
        to store context druing the unwind operation.

Return Value:

    None.

--*/

{

    ULONG ControlPc;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    ULONG EstablisherFrame;
    ULONG ExceptionFlags;
    EXCEPTION_RECORD ExceptionRecord1;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG HighLimit;
    ULONG LowLimit;
    ULONG NextPc;

    //
    // Get current stack limits, capture the current context, virtually
    // unwind to the caller of this routine, get the initial PC value, and
    // set the unwind target address.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlCaptureContext(ContextRecord);
    ControlPc = (ULONG)(ContextRecord->XIntRa - 4);
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc | 1,
                              FunctionEntry,
                              ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ControlPc = NextPc;
    ContextRecord->Fir = (ULONG)TargetIp;

    //
    // If an exception record is not specified, then build a local exception
    // record for use in calling exception handlers during the unwind operation.
    //

    if (ARGUMENT_PRESENT(ExceptionRecord) == FALSE) {
        ExceptionRecord = &ExceptionRecord1;
        ExceptionRecord1.ExceptionCode = STATUS_UNWIND;
        ExceptionRecord1.ExceptionRecord = NULL;
        ExceptionRecord1.ExceptionAddress = (PVOID)ControlPc;
        ExceptionRecord1.NumberParameters = 0;
    }

    //
    // If the target frame of the unwind is specified, then a normal unwind
    // is being performed. Otherwise, an exit unwind is being performed.
    //

    ExceptionFlags = EXCEPTION_UNWINDING;
    if (ARGUMENT_PRESENT(TargetFrame) == FALSE) {
        ExceptionRecord->ExceptionFlags |= EXCEPTION_EXIT_UNWIND;
    }

    //
    // Scan backward through the call frame hierarchy and call exception
    // handlers until the target frame of the unwind is reached.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the routine to obtain the virtual frame
        // pointer of the establisher, but don't update the context record.
        //

        if (FunctionEntry != NULL) {
            NextPc = RtlpVirtualUnwind(ControlPc,
                                       FunctionEntry,
                                       ContextRecord,
                                       &InFunction,
                                       &EstablisherFrame,
                                       NULL);

            //
            // If the virtual frame pointer is not within the specified stack
            // limits, the virtual frame pointer is unaligned, or the target
            // frame is below the virtual frame and an exit unwind is not being
            // performed, then raise the exception STATUS_BAD_STACK. Otherwise,
            // check to determine if the current routine has an exception
            // handler.
            //

            if ((EstablisherFrame < LowLimit) || (EstablisherFrame > HighLimit) ||
               ((ARGUMENT_PRESENT(TargetFrame) != FALSE) &&
               ((ULONG)TargetFrame < EstablisherFrame)) ||
               ((EstablisherFrame & 0x7) != 0)) {
                RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);

            } else if ((FunctionEntry->ExceptionHandler != NULL) && InFunction) {

                //
                // The frame has an exception handler.
                //
                // The control PC, establisher frame pointer, the address
                // of the function table entry, and the address of the
                // context record are all stored in the dispatcher context.
                // This information is used by the unwind linkage routine
                // and can be used by the exception handler itself.
                //
                // A linkage routine written in assembler is used to actually
                // call the actual exception handler. This is required by the
                // exception handler that is associated with the linkage
                // routine so it can have access to two sets of dispatcher
                // context when it is called.
                //

                DispatcherContext.ControlPc = ControlPc;
                DispatcherContext.FunctionEntry = FunctionEntry;
                DispatcherContext.EstablisherFrame = EstablisherFrame;
                DispatcherContext.ContextRecord = ContextRecord;

                //
                // Call the exception handler.
                //

                do {

                    //
                    // If the establisher frame is the target of the unwind
                    // operation, then set the target unwind flag.
                    //

                    if ((ULONG)TargetFrame == EstablisherFrame) {
                        ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
                    }

                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    //
                    // Set the specified return value in case the exception
                    // handler directly continues execution.
                    //

                    ContextRecord->XIntV0 = (LONG)ReturnValue;
                    Disposition =
                        RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                    EstablisherFrame,
                                                    ContextRecord,
                                                    &DispatcherContext,
                                                    FunctionEntry->ExceptionHandler);

                    //
                    // Clear target unwind and collided unwind flags.
                    //

                    ExceptionFlags &= ~(EXCEPTION_COLLIDED_UNWIND |
                                                        EXCEPTION_TARGET_UNWIND);

                    //
                    // Case on the handler disposition.
                    //

                    switch (Disposition) {

                        //
                        // The disposition is to continue the search.
                        //
                        // If the target frame has not been reached, then
                        // virtually unwind to the caller of the current
                        // routine, update the context record, and continue
                        // the search for a handler.
                        //

                    case ExceptionContinueSearch :
                        if (EstablisherFrame != (ULONG)TargetFrame) {
                            NextPc = RtlVirtualUnwind(ControlPc | 1,
                                                      FunctionEntry,
                                                      ContextRecord,
                                                      &InFunction,
                                                      &EstablisherFrame,
                                                      NULL);
                        }

                        break;

                        //
                        // The disposition is collided unwind.
                        //
                        // Set the target of the current unwind to the context
                        // record of the previous unwind, and reexecute the
                        // exception handler from the collided frame with the
                        // collided unwind flag set in the exception record.
                        //

                    case ExceptionCollidedUnwind :
                        ControlPc = DispatcherContext.ControlPc;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        ContextRecord = DispatcherContext.ContextRecord;
                        ContextRecord->Fir = (ULONG)TargetIp;
                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        EstablisherFrame = DispatcherContext.EstablisherFrame;
                        break;

                        //
                        // All other disposition values are invalid.
                        //
                        // Raise invalid disposition exception.
                        //

                    default :
                        RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                    }

                } while ((ExceptionFlags & EXCEPTION_COLLIDED_UNWIND) != 0);

            } else {

                //
                // If the target frame has not been reached, then virtually unwind to the
                // caller of the current routine and update the context record.
                //

                if (EstablisherFrame != (ULONG)TargetFrame) {
                    NextPc = RtlVirtualUnwind(ControlPc | 1,
                                              FunctionEntry,
                                              ContextRecord,
                                              &InFunction,
                                              &EstablisherFrame,
                                              NULL);
                }
            }

        } else {

            //
            // Set point at which control left the previous routine.
            //

            NextPc = (ULONG)(ContextRecord->XIntRa - 4);

            //
            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            //

            if (NextPc == ControlPc) {
                RtlRaiseStatus(STATUS_BAD_FUNCTION_TABLE);
            }
        }

        //
        // Set point at which control left the previous routine.
        //
        // N.B. Make sure the address is in the delay slot of the jal
        //      to prevent the boundary condition of the return address
        //      being at the front of a try body.
        //

        ControlPc = NextPc;

    } while ((EstablisherFrame < HighLimit) &&
            (EstablisherFrame != (ULONG)TargetFrame));

    //
    // If the establisher stack pointer is equal to the target frame
    // pointer, then continue execution. Otherwise, an exit unwind was
    // performed or the target of the unwind did not exist and the
    // debugger and subsystem are given a second chance to handle the
    // unwind.
    //

    if (EstablisherFrame == (ULONG)TargetFrame) {
        ContextRecord->XIntV0 = (LONG)ReturnValue;
        RtlpRestoreContext(ContextRecord, ExceptionRecord);

    } else {
        ZwRaiseException(ExceptionRecord, ContextRecord, FALSE);
    }
}

ULONG
RtlVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    )

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

    N.B. This routine handles 64-bit context records.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

        N.B. The low order bit of this argument is used to denote the
             context record type. If the low order bit is clear, then
             the context record contains 32-bit information. Otherwise,
             it contains 64-bit information.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    InFunction - Supplies a pointer to a variable that receives whether the
        control PC is within the current function.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/

{

    ULONG Address;
    ULONG DecrementOffset;
    ULONG DecrementRegister;
    PULONG FloatingRegister;
    ULONG Function;
    MIPS_INSTRUCTION Instruction;
    PULONGLONG IntegerRegister;
    ULONG NextPc;
    LONG Offset;
    ULONG Opcode;
    ULONG Rd;
    BOOLEAN RestoredRa;
    BOOLEAN RestoredSp;
    ULONG Rs;
    ULONG Rt;

    //
    // If the low order bit of the control PC is clear, then the context
    // record format is 32-bit. Otherwise, the context record format is
    // 64-bits.
    //

    if ((ControlPc & 1) == 0) {
        return RtlpVirtualUnwind32(ControlPc,
                                   FunctionEntry,
                                   ContextRecord,
                                   InFunction,
                                   EstablisherFrame,
                                   ContextPointers);

    } else {

        //
        // Set the base address of the integer and floating register arrays.
        //

        FloatingRegister = &ContextRecord->FltF0;
        IntegerRegister = &ContextRecord->XIntZero;

        //
        // If the instruction at the point where control left the specified
        // function is a return, then any saved registers have been restored
        // with the possible exception of the stack pointer and the control
        // PC is not considered to be in the function (i.e., an epilogue).
        //

        ControlPc &= ~1;
        if (*((PULONG)ControlPc) == JUMP_RA) {
            *InFunction = FALSE;
            Instruction.Long = *((PULONG)ControlPc + 1);
            Opcode = Instruction.i_format.Opcode;
            Offset = Instruction.i_format.Simmediate;
            Rd = Instruction.r_format.Rd;
            Rs = Instruction.i_format.Rs;
            Rt = Instruction.i_format.Rt;
            Function = Instruction.r_format.Function;

            //
            // If the opcode is an add immediate unsigned op and both the
            // source and destination registers are SP, then add the signed
            // offset value to SP. Otherwise, if the opcode is a special op,
            // the operation is an add unsigned, and the source and destination
            // registers are both SP, then add the register specified by Rd to
            // SP.
            //

            if ((Opcode == ADDIU_OP) && (Rt == SP) && (Rs == SP)) {
                IntegerRegister[SP] += Offset;

            } else if ((Opcode == SPEC_OP) && (Function == ADDU_OP) &&
                       (Rd == SP) && (Rs == SP)) {
                IntegerRegister[SP] += IntegerRegister[Rt];
            }

            *EstablisherFrame = (ULONG)ContextRecord->XIntSp;
            return (ULONG)ContextRecord->XIntRa;
        }

        //
        // If the address where control left the specified function is outside
        // the limits of the prologue, then the control PC is considered to be
        // within the function and the control address is set to the end of
        // the prologue. Otherwise, the control PC is not considered to be
        // within the function (i.e., it is within the prologue).
        //

        if ((ControlPc < FunctionEntry->BeginAddress) ||
            (ControlPc >= FunctionEntry->PrologEndAddress)) {
            *InFunction = TRUE;
            ControlPc = FunctionEntry->PrologEndAddress;

        } else {
            *InFunction = FALSE;
        }

        //
        // Scan backward through the prologue and reload callee registers that
        // were stored.
        //

        DecrementRegister = 0;
        *EstablisherFrame = (ULONG)ContextRecord->XIntSp;
        NextPc = (ULONG)(ContextRecord->XIntRa - 4);
        RestoredRa = FALSE;
        RestoredSp = FALSE;
        while (ControlPc > FunctionEntry->BeginAddress) {

            //
            // Get instruction value, decode fields, case of opcode value, and
            // reverse store operations.
            //

            ControlPc -= 4;
            Instruction.Long = *((PULONG)ControlPc);
            Opcode = Instruction.i_format.Opcode;
            Offset = Instruction.i_format.Simmediate;
            Rd = Instruction.r_format.Rd;
            Rs = Instruction.i_format.Rs;
            Rt = Instruction.i_format.Rt;
            Address = (ULONG)(Offset + IntegerRegister[Rs]);
            if (Opcode == SW_OP) {

                //
                // Store word.
                //
                // If the base register is SP and the source register is an
                // integer register, then reload the register value.
                //

                if (Rs == SP) {
                    IntegerRegister[Rt] = *((PLONG)Address);

                    //
                    // If the destination register is RA and this is the first
                    // time that RA is being restored, then set the address of
                    // where control left the previous frame. Otherwise, this
                    // is an interrupt or exception and the return PC should be
                    // biased by 4. Otherwise, if the destination register is
                    // SP and this is the first time that SP is being restored,
                    // then set the establisher frame pointer.
                    //

                    if (Rt == RA) {
                        if (RestoredRa == FALSE) {
                            NextPc = (ULONG)(ContextRecord->XIntRa - 4);
                            RestoredRa = TRUE;

                        } else {
                            NextPc += 4;
                        }

                    } else if (Rt == SP) {
                        if (RestoredSp == FALSE) {
                            *EstablisherFrame = (ULONG)ContextRecord->XIntSp;
                            RestoredSp = TRUE;
                        }
                    }

                    //
                    // If a context pointer record is specified, then record
                    // the address where the destination register contents
                    // are stored.
                    //

                    if (ARGUMENT_PRESENT(ContextPointers)) {
                        ContextPointers->XIntegerContext[Rt] = (PULONGLONG)Address;
                    }
                }

            } else if (Opcode == SD_OP) {

                //
                // Store double.
                //
                // If the base register is SP and the source register is an
                // integer register, then reload the register value.
                //

                if (Rs == SP) {
                    IntegerRegister[Rt] = *((PULONGLONG)Address);

                    //
                    // If the destination register is RA and this is the first
                    // time that RA is being restored, then set the address of
                    // where control left the previous frame. Otherwise, this
                    // is an interrupt or exception and the return PC should be
                    // biased by 4. Otherwise, if the destination register is
                    // SP and this is the first time that SP is being restored,
                    // then set the establisher frame pointer.
                    //

                    if (Rt == RA) {
                        if (RestoredRa == FALSE) {
                            NextPc = (ULONG)(ContextRecord->XIntRa - 4);
                            RestoredRa = TRUE;

                        } else {
                            NextPc += 4;
                        }

                    } else if (Rt == SP) {
                        if (RestoredSp == FALSE) {
                            *EstablisherFrame = (ULONG)ContextRecord->XIntSp;
                            RestoredSp = TRUE;
                        }
                    }

                    //
                    // If a context pointer record is specified, then record
                    // the address where the destination register contents
                    // are stored.
                    //
                    // N.B. The low order bit of the address is set to indicate
                    //      a store double operation.
                    //

                    if (ARGUMENT_PRESENT(ContextPointers)) {
                        ContextPointers->XIntegerContext[Rt] = (PLONGLONG)((ULONG)Address | 1);
                    }
                }

            } else if (Opcode == SWC1_OP) {

                //
                // Store word coprocessor 1.
                //
                // If the base register is SP and the source register is a
                // floating register, then reload the register value.
                //

                if (Rs == SP) {
                    FloatingRegister[Rt] = *((PULONG)Address);

                    //
                    // If a context pointer record is specified, then record
                    // the address where the destination register contents
                    // are stored.
                    //

                    if (ARGUMENT_PRESENT(ContextPointers)) {
                        ContextPointers->FloatingContext[Rt] = (PULONG)Address;
                    }
                }

            } else if (Opcode == SDC1_OP) {

                //
                // Store double coprocessor 1.
                //
                // If the base register is SP and the source register is a
                // floating register, then reload the register and the next
                // register values.
                //

                if (Rs == SP) {
                    FloatingRegister[Rt] = *((PULONG)Address);
                    FloatingRegister[Rt + 1] = *((PULONG)(Address + 4));

                    //
                    // If a context pointer record is specified, then record
                    // the address where the destination registers contents
                    // are stored.
                    //

                    if (ARGUMENT_PRESENT(ContextPointers)) {
                        ContextPointers->FloatingContext[Rt] = (PULONG)Address;
                        ContextPointers->FloatingContext[Rt + 1] = (PULONG)(Address + 4);
                    }
                }

            } else if (Opcode == ADDIU_OP) {

                //
                // Add immediate unsigned.
                //
                // If both the source and destination registers are SP, then
                // a standard stack allocation was performed and the signed
                // displacement value should be subtracted from SP. Otherwise,
                // if the destination register is the decrement register and
                // the source register is zero, then add the decrement value
                // to SP.
                //

                if ((Rs == SP) && (Rt == SP)) {
                    IntegerRegister[SP] -= Offset;
                    if (RestoredSp == FALSE) {
                        *EstablisherFrame = (ULONG)ContextRecord->XIntSp;
                        RestoredSp = TRUE;
                    }

                } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                    IntegerRegister[SP] += Offset;
                    if (RestoredSp == FALSE) {
                        *EstablisherFrame = (ULONG)ContextRecord->XIntSp;
                        RestoredSp = TRUE;
                    }
                }

            } else if (Opcode == ORI_OP) {

                //
                // Or immediate.
                //
                // If both the destination and source registers are the decrement
                // register, then save the decrement value. Otherwise, if the
                // destination register is the decrement register and the source
                // register is zero, then add the decrement value to SP.
                //

                if ((Rs == DecrementRegister) && (Rt == DecrementRegister)) {
                    DecrementOffset = (Offset & 0xffff);

                } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                    IntegerRegister[SP] += (Offset & 0xffff);
                    if (RestoredSp == FALSE) {
                        *EstablisherFrame = (ULONG)ContextRecord->XIntSp;
                        RestoredSp = TRUE;
                    }
                }

            } else if (Opcode == SPEC_OP) {

                //
                // Special operation.
                //
                // The real opcode is in the function field of special opcode
                // instructions.
                //

                Function = Instruction.r_format.Function;
                if ((Function == ADDU_OP) || (Function == OR_OP)) {

                    //
                    // Add unsigned or an or operation.
                    //
                    // If one of the source registers is ZERO, then the
                    // operation is a move operation and the destination
                    // register should be moved to the appropriate source
                    // register.
                    //

                    if (Rt == ZERO) {
                        IntegerRegister[Rs] = IntegerRegister[Rd];

                        //
                        // If the destination register is RA and this is the
                        // first time that RA is being restored, then set the
                        // address of where control left the previous frame.
                        // Otherwise, this an interrupt or exception and the
                        // return PC should be biased by 4.
                        //

                        if (Rs == RA) {
                            if (RestoredRa == FALSE) {
                                NextPc = (ULONG)(ContextRecord->XIntRa - 4);
                                RestoredRa = TRUE;

                            } else {
                                NextPc += 4;
                            }
                        }

                    } else if (Rs == ZERO) {
                        IntegerRegister[Rt] = IntegerRegister[Rd];

                        //
                        // If the destination register is RA and this is the
                        // first time that RA is being restored, then set the
                        // address of where control left the previous frame.
                        // Otherwise, this an interrupt or exception and the
                        // return PC should be biased by 4.
                        //

                        if (Rt == RA) {
                            if (RestoredRa == FALSE) {
                                NextPc = (ULONG)(ContextRecord->XIntRa - 4);
                                RestoredRa = TRUE;

                            } else {
                                NextPc += 4;
                            }
                        }
                    }

                } else if (Function == SUBU_OP) {

                    //
                    // Subtract unsigned.
                    //
                    // If the destination register is SP and the source register
                    // is SP, then a stack allocation greater than 32kb has been
                    // performed and source register number of the decrement must
                    // be saved for later use.
                    //

                    if ((Rd == SP) && (Rs == SP)) {
                        DecrementRegister = Rt;
                    }
                }

            } else if (Opcode == LUI_OP) {

                //
                // Load upper immediate.
                //
                // If the destination register is the decrement register, then
                // compute the decrement value, add it from SP, and clear the
                // decrement register number.
                //

                if (Rt == DecrementRegister) {
                    DecrementRegister = 0;
                    IntegerRegister[SP] += (LONG)(DecrementOffset + (Offset << 16));
                    if (RestoredSp == FALSE) {
                        *EstablisherFrame = (ULONG)(ContextRecord->XIntSp);
                        RestoredSp = TRUE;
                    }
                }
            }
        }

        //
        // Make sure that integer register zero is really zero.
        //

        ContextRecord->XIntZero = 0;
        return NextPc;
    }
}

ULONG
RtlpVirtualUnwind32 (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    )

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

    N.B. This routine handles 32-bit context records.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    InFunction - Supplies a pointer to a variable that receives whether the
        control PC is within the current function.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/

{

    ULONG Address;
    ULONG DecrementOffset;
    ULONG DecrementRegister;
    PULONG FloatingRegister;
    ULONG Function;
    MIPS_INSTRUCTION Instruction;
    PULONG IntegerRegister;
    ULONG NextPc;
    LONG Offset;
    ULONG Opcode;
    ULONG Rd;
    BOOLEAN RestoredRa;
    BOOLEAN RestoredSp;
    ULONG Rs;
    ULONG Rt;

    //
    // Set the base address of the integer and floating register arrays.
    //

    FloatingRegister = &ContextRecord->FltF0;
    IntegerRegister = &ContextRecord->IntZero;

    //
    // If the instruction at the point where control left the specified
    // function is a return, then any saved registers have been restored
    // with the possible exception of the stack pointer and the control
    // PC is not considered to be in the function (i.e., an epilogue).
    //

    if (*((PULONG)ControlPc) == JUMP_RA) {
        *InFunction = FALSE;
        Instruction.Long = *((PULONG)ControlPc + 1);
        Opcode = Instruction.i_format.Opcode;
        Offset = Instruction.i_format.Simmediate;
        Rd = Instruction.r_format.Rd;
        Rs = Instruction.i_format.Rs;
        Rt = Instruction.i_format.Rt;
        Function = Instruction.r_format.Function;

        //
        // If the opcode is an add immediate unsigned op and both the source
        // and destination registers are SP, then add the signed offset value
        // to SP. Otherwise, if the opcode is a special op, the operation is
        // an add unsigned, and the source and destination registers are both
        // SP, then add the register specified by Rd to SP.
        //

        if ((Opcode == ADDIU_OP) && (Rt == SP) && (Rs == SP)) {
            IntegerRegister[SP] += Offset;

        } else if ((Opcode == SPEC_OP) && (Function == ADDU_OP) &&
                   (Rd == SP) && (Rs == SP)) {
            IntegerRegister[SP] += IntegerRegister[Rt];
        }

        *EstablisherFrame = ContextRecord->IntSp;
        return ContextRecord->IntRa;
    }

    //
    // If the address where control left the specified function is outside
    // the limits of the prologue, then the control PC is considered to be
    // within the function and the control address is set to the end of
    // the prologue. Otherwise, the control PC is not considered to be
    // within the function (i.e., it is within the prologue).
    //

    if ((ControlPc < FunctionEntry->BeginAddress) ||
        (ControlPc >= FunctionEntry->PrologEndAddress)) {
        *InFunction = TRUE;
        ControlPc = FunctionEntry->PrologEndAddress;

    } else {
        *InFunction = FALSE;
    }

    //
    // Scan backward through the prologue and reload callee registers that
    // were stored.
    //

    DecrementRegister = 0;
    *EstablisherFrame = ContextRecord->IntSp;
    NextPc = ContextRecord->IntRa - 4;
    RestoredRa = FALSE;
    RestoredSp = FALSE;
    while (ControlPc > FunctionEntry->BeginAddress) {

        //
        // Get instruction value, decode fields, case of opcode value, and
        // reverse store operations.
        //

        ControlPc -= 4;
        Instruction.Long = *((PULONG)ControlPc);
        Opcode = Instruction.i_format.Opcode;
        Offset = Instruction.i_format.Simmediate;
        Rd = Instruction.r_format.Rd;
        Rs = Instruction.i_format.Rs;
        Rt = Instruction.i_format.Rt;
        Address = Offset + IntegerRegister[Rs];
        if (Opcode == SW_OP) {

            //
            // Store word.
            //
            // If the base register is SP and the source register is an
            // integer register, then reload the register value.
            //

            if (Rs == SP) {
                IntegerRegister[Rt] = *((PULONG)Address);

                //
                // If the destination register is RA and this is the first
                // time that RA is being restored, then set the address of
                // where control left the previous frame. Otherwise, this
                // is an interrupt or exception and the return PC should be
                // biased by 4. Otherwise, if the destination register is
                // SP and this is the first time that SP is being restored,
                // then set the establisher frame pointer.
                //

                if (Rt == RA) {
                    if (RestoredRa == FALSE) {
                        NextPc = ContextRecord->IntRa - 4;
                        RestoredRa = TRUE;

                    } else {
                        NextPc += 4;
                    }

                } else if (Rt == SP) {
                    if (RestoredSp == FALSE) {
                        *EstablisherFrame = ContextRecord->IntSp;
                        RestoredSp = TRUE;
                    }
                }

                //
                // If a context pointer record is specified, then record
                // the address where the destination register contents
                // are stored.
                //

                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->XIntegerContext[Rt] = (PULONGLONG)Address;
                }
            }

        } else if (Opcode == SWC1_OP) {

            //
            // Store word coprocessor 1.
            //
            // If the base register is SP and the source register is a
            // floating register, then reload the register value.
            //

            if (Rs == SP) {
                FloatingRegister[Rt] = *((PULONG)Address);

                //
                // If a context pointer record is specified, then record
                // the address where the destination register contents
                // are stored.
                //

                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->FloatingContext[Rt] = (PULONG)Address;
                }
            }

        } else if (Opcode == SDC1_OP) {

            //
            // Store double coprocessor 1.
            //
            // If the base register is SP and the source register is a
            // floating register, then reload the register and the next
            // register values.
            //

            if (Rs == SP) {
                FloatingRegister[Rt] = *((PULONG)Address);
                FloatingRegister[Rt + 1] = *((PULONG)(Address + 4));

                //
                // If a context pointer record is specified, then record
                // the address where the destination registers contents
                // are stored.
                //

                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->FloatingContext[Rt] = (PULONG)Address;
                    ContextPointers->FloatingContext[Rt + 1] = (PULONG)(Address + 4);
                }
            }

        } else if (Opcode == ADDIU_OP) {

            //
            // Add immediate unsigned.
            //
            // If both the source and destination registers are SP, then
            // a standard stack allocation was performed and the signed
            // displacement value should be subtracted from SP. Otherwise,
            // if the destination register is the decrement register and
            // the source register is zero, then add the decrement value
            // to SP.
            //

            if ((Rs == SP) && (Rt == SP)) {
                IntegerRegister[SP] -= Offset;
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }

            } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                IntegerRegister[SP] += Offset;
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }
            }

        } else if (Opcode == ORI_OP) {

            //
            // Or immediate.
            //
            // If both the destination and source registers are the decrement
            // register, then save the decrement value. Otherwise, if the
            // destination register is the decrement register and the source
            // register is zero, then add the decrement value to SP.
            //

            if ((Rs == DecrementRegister) && (Rt == DecrementRegister)) {
                DecrementOffset = (Offset & 0xffff);

            } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                IntegerRegister[SP] += (Offset & 0xffff);
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }
            }

        } else if (Opcode == SPEC_OP) {

            //
            // Special operation.
            //
            // The real opcode is in the function field of special opcode
            // instructions.
            //

            Function = Instruction.r_format.Function;
            if ((Function == ADDU_OP) || (Function == OR_OP)) {

                //
                // Add unsigned or an or operation.
                //
                // If one of the source registers is ZERO, then the
                // operation is a move operation and the destination
                // register should be moved to the appropriate source
                // register.
                //

                if (Rt == ZERO) {
                    IntegerRegister[Rs] = IntegerRegister[Rd];

                    //
                    // If the destination register is RA and this is the
                    // first time that RA is being restored, then set the
                    // address of where control left the previous frame.
                    // Otherwise, this an interrupt or exception and the
                    // return PC should be biased by 4.
                    //

                    if (Rs == RA) {
                        if (RestoredRa == FALSE) {
                            NextPc = ContextRecord->IntRa - 4;
                            RestoredRa = TRUE;

                        } else {
                            NextPc += 4;
                        }
                    }

                } else if (Rs == ZERO) {
                    IntegerRegister[Rt] = IntegerRegister[Rd];

                    //
                    // If the destination register is RA and this is the
                    // first time that RA is being restored, then set the
                    // address of where control left the previous frame.
                    // Otherwise, this an interrupt or exception and the
                    // return PC should be biased by 4.
                    //

                    if (Rt == RA) {
                        if (RestoredRa == FALSE) {
                            NextPc = ContextRecord->IntRa - 4;
                            RestoredRa = TRUE;

                        } else {
                            NextPc += 4;
                        }
                    }
                }

            } else if (Function == SUBU_OP) {

                //
                // Subtract unsigned.
                //
                // If the destination register is SP and the source register
                // is SP, then a stack allocation greater than 32kb has been
                // performed and source register number of the decrement must
                // be saved for later use.
                //

                if ((Rd == SP) && (Rs == SP)) {
                    DecrementRegister = Rt;
                }
            }

        } else if (Opcode == LUI_OP) {

            //
            // Load upper immediate.
            //
            // If the destination register is the decrement register, then
            // compute the decrement value, add it from SP, and clear the
            // decrement register number.
            //

            if (Rt == DecrementRegister) {
                DecrementRegister = 0;
                IntegerRegister[SP] += (DecrementOffset + (Offset << 16));
                if (RestoredSp == FALSE) {
                    *EstablisherFrame = ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }
            }
        }
    }

    //
    // Make sure that integer register zero is really zero.
    //

    ContextRecord->IntZero = 0;
    return NextPc;
}

ULONG
RtlpVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    )

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

    N.B. This function copies the specified context record and only computes
         the establisher frame and whether control is actually in a function.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    InFunction - Supplies a pointer to a variable that receives whether the
        control PC is within the current function.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/

{

    CONTEXT LocalContext;

    //
    // Copy the context record so updates will not be reflected in the
    // original copy and then virtually unwind to the caller of the
    // specified control point.
    //

    RtlMoveMemory((PVOID)&LocalContext, ContextRecord, sizeof(CONTEXT));
    return RtlVirtualUnwind(ControlPc | 1,
                            FunctionEntry,
                            &LocalContext,
                            InFunction,
                            EstablisherFrame,
                            ContextPointers);
}
