/*++

Copyright (c) 1993  IBM Corporation and Microsoft Corporation

Module Name:

    exdsptch.c

Abstract:

    This module implements the dispatching of exception and the unwinding of
    procedure call frames for PowerPC.

Author:

    Rick Simpson  16-Aug-1993

    based on MIPS version by David N. Cutler (davec) 11-Sep-1990

Environment:

    Any mode.

Revision History:

    Tom Wood (twood) 19-Aug-1994
    Update to use RtlVirtualUnwind even when there isn't a function table
    entry.  Add stack limit parameters to RtlVirtualUnwind.

    Changed RtlLookupFunctionEntry to deal with the indirect entries.

--*/

#include "ntrtlp.h"

//
// Define local macros.
//

#ifndef ROS_DEBUG
#ifndef READ_ULONG
#define READ_ULONG(addr,dest) dest = (*((PULONG)(addr)))
#define READ_DOUBLE(addr,dest) dest = (*((PDOUBLE)(addr)))
#endif

#include "vunwind.c"
#endif // ROS_DEBUG

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
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL,
    IN ULONG LowStackLimit,
    IN ULONG HighStackLimit
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

    As each frame is encountered, the PC where control left the corresponding
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
    ControlPc = ContextRecord1.Iar;
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
        // Virtually unwind to the caller of the current routine to obtain
        // the virtual frame pointer of the establisher and the next PC.
        //

        NextPc = RtlVirtualUnwind(ControlPc,
                                  FunctionEntry,
                                  &ContextRecord1,
                                  &InFunction,
                                  &EstablisherFrame,
                                  NULL,
                                  LowLimit,
                                  HighLimit);

        //
        // If there is a function table entry for the routine, check if
        // there is an exception handler for the frame.
        //

        if (FunctionEntry != NULL) {
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
                ULONG Index;

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

                if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
                    Index = RtlpLogExceptionHandler(
                                    ExceptionRecord,
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
    } while (ContextRecord1.Gpr1 < HighLimit);

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
                    // have (within reason) discontiguous code segment(s). If
                    // PrologEndAddress is out of range, it is re-interpreted
                    // as a pointer to the primary function table entry for
                    // that function.  The out of range test takes into account
                    // the redundant encoding of millicode and glue code.
                    //

                    if (((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                         (FunctionEntry->PrologEndAddress > FunctionEntry->EndAddress)) &&
                        (FunctionEntry->PrologEndAddress & 3) == 0) {
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
    ControlPc = ContextRecord.Lr - 4;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL,
                              0,
                              0xffffffff);

    ContextRecord.Iar = NextPc + 4;
    ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.Iar;
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
    ControlPc = ContextRecord.Lr - 4;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL,
                              0,
                              0xffffffff);

    ContextRecord.Iar = NextPc + 4;
    ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.Iar;
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
    ControlPc = ContextRecord->Lr - 4;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL,
                              0,
                              0xffffffff);

    ControlPc = NextPc;
    ContextRecord->Iar = (ULONG)TargetIp;

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
                                       NULL,
                                       LowLimit,
                                       HighLimit);

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

                    ContextRecord->Gpr3 = (ULONG)ReturnValue;
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
                            NextPc = RtlVirtualUnwind(ControlPc,
                                                      FunctionEntry,
                                                      ContextRecord,
                                                      &InFunction,
                                                      &EstablisherFrame,
                                                      NULL,
                                                      0,
                                                      0xffffffff);
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
                        ContextRecord->Iar = (ULONG)TargetIp;
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
                    NextPc = RtlVirtualUnwind(ControlPc,
                                              FunctionEntry,
                                              ContextRecord,
                                              &InFunction,
                                              &EstablisherFrame,
                                              NULL,
                                              0,
                                              0xffffffff);
                }
            }

        } else {

            //
            // Set point at which control left the previous routine.
            //
            NextPc = RtlVirtualUnwind(ControlPc,
                                      FunctionEntry,
                                      ContextRecord,
                                      &InFunction,
                                      &EstablisherFrame,
                                      NULL,
                                      0,
                                      0xffffffff);

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
        //
        // Virtually unwind the target frame to recover the value of r.2.
        // We must take care to not unwind a glue sequence that may have
        // been used to reach the target frame.  This is done by giving
        // stack limit values that will regard any stack pointer as bad.
        //
        CONTEXT TocContext;
        RtlMoveMemory((PVOID)&TocContext, ContextRecord, sizeof(CONTEXT));
        ControlPc = ContextRecord->Iar;
        FunctionEntry = RtlLookupFunctionEntry(ControlPc);
        NextPc = RtlVirtualUnwind(ControlPc,
                                  FunctionEntry,
                                  &TocContext,
                                  &InFunction,
                                  &EstablisherFrame,
                                  NULL,
                                  0xffffffff,
                                  0);
        ContextRecord->Gpr2 = TocContext.Gpr2;
        ContextRecord->Gpr3 = (ULONG)ReturnValue;
        RtlpRestoreContext(ContextRecord, ExceptionRecord);

    } else {
        ZwRaiseException(ExceptionRecord, ContextRecord, FALSE);
    }
}

ULONG
RtlpVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL,
    IN ULONG LowStackLimit,
    IN ULONG HighStackLimit
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

    If the function is register save millicode, it is treated as a leaf
    function.  If the function is register restore millicode, the remaining
    body is executed forwards and the address where control left the
    previous frame is obtained from the final blr instruction.

    If the function was called via glue code and is not that glue code,
    the prologe of the glue code is executed backwards in addition to the
    above actions.

    Otherwise, an exception or interrupt entry to the system is being
    unwound and a specially coded prologue restores the return address
    twice. Once from the fault instruction address and once from the saved
    return address register. The first restore is returned as the function
    value and the second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

    N.B. This function copies the specified context record and only computes
         the establisher frame and whether control is actually in a function.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function or NULL if the function is a leaf function.

    ContextRecord - Supplies the address of a context record.

    InFunction - Supplies a pointer to a variable that receives whether the
        control PC is within the current function.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

    LowStackLimit, HighStackLimit - Range of valid values for the stack
        pointer.  This indicates whether it is valid to examine NextPc.

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
    return RtlVirtualUnwind(ControlPc,
                            FunctionEntry,
                            &LocalContext,
                            InFunction,
                            EstablisherFrame,
                            ContextPointers,
                            LowStackLimit,
                            HighStackLimit);
}
