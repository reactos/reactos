/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    exdsptch.c

Abstract:

    This module implements the dispatching of exceptions and the unwinding of
    procedure call frames.

Author:

    David N. Cutler (davec) 11-Sep-1990

Environment:

    Any mode.

Revision History:

    Thomas Van Baak (tvb) 13-May-1992

        Adapted for Alpha AXP.

    Florence Lee (Digital) 10-Apr-1997

        Add support for dynamic function tables (user mode only)
        1) Modify RtlLookupFunctionEntry to search dynamic function tables
        2) Add RtlAddFunctionTable: Add an array of RUNTIME_FUNCTION entries
           to the dynamic function table list.
        3) Add RtlDeleteFunctionTable: Remove dynamic from table the
           dynamic function table list.

    Monty VanderBilt (Digital) 16-Jun-1997

        1) Use macros defined in ntalpha.h to access RUNTIME_FUNCTION
           fields without low order bits used for other information
        2) Modify RtlLookupFunctionEntry() and RtlVirtualUnwind() to
           handle the variations of secondary function entry types.
           
    Monty VanderBilt (Compaq) 10-Aug-1999
    
        ECO (Engineering Change Order) numbers refer to Alpha NT calling standard changes 
        ECO 11: Support for compiler optimizations: tail calls, floating return sequences, shrinkwrap.
        ECO 12: Minor change to ECO 11 moving StackAllocation field from the 3rd to 4th longword
                in the secondary function entry.
        ECO 14: Fixed return function entries to support (among other uses) exception handling in
                instrumentation code.
           
--*/

#include "ntrtlp.h"
int __cdecl sprintf(char *, const char *, ...);

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
// Determine if ExceptionHandler is defined
//

#define IS_HANDLER_DEFINED(FunctionEntry) \
    (RF_EXCEPTION_HANDLER(FunctionEntry) != 0)

#if DBG

//
// Maintain a short history of PC's for malformed function table errors.
//

#define PC_HISTORY_DEPTH 4

//
// Definition of global flag to debug/validate exception handling.
// See ntrtlalp.h for the bit definitions in this flag word.
//

ULONG RtlDebugFlags = 0;

void
ShowRuntimeFunction(
    PRUNTIME_FUNCTION FunctionEntry,
    PSTR Label
    );

#endif

#define Virtual VirtualFramePointer
#define Real RealFramePointer

//
// Define private function prototypes.
//

VOID
RtlpRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    );

VOID
RtlpRaiseStatus (
    IN NTSTATUS Status
    );

ULONG_PTR
RtlpVirtualUnwind (
    IN ULONG_PTR ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame
    );

#if !defined(NTOS_KERNEL_RUNTIME)

//
// List head for DYNAMIC_FUNCTION_TABLE entries.
//

LIST_ENTRY DynamicFunctionTable;

PRUNTIME_FUNCTION
RtlLookupDynamicFunctionEntry(
    IN ULONG_PTR ControlPc
    );

#endif

PRUNTIME_FUNCTION
RtlLookupStaticFunctionEntry(
    IN ULONG_PTR ControlPc,
    OUT PBOOLEAN InImage
    );

VOID
RtlGetUnwindFunctionEntry(
    IN ULONG_PTR ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    OUT PRUNTIME_FUNCTION UnwindFunctionEntry,
    OUT PULONG StackAdjust,
    OUT PULONG_PTR FixedReturn
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
    ULONG_PTR ControlPc;
#if DBG
    ULONG_PTR ControlPcHistory[PC_HISTORY_DEPTH];
    ULONG ControlPcHistoryIndex = 0;
#endif
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    FRAME_POINTERS EstablisherFrame;
    ULONG ExceptionFlags;
#if DBG
    LONG FrameDepth = 0;
#endif
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG_PTR HighLimit;
    BOOLEAN InFunction;
    ULONG_PTR LowLimit;
    ULONG_PTR NestedFrame;
    ULONG_PTR NextPc;

    //
    // Get current stack limits, copy the context record, get the initial
    // PC value, capture the exception flags, and set the nested exception
    // frame pointer.
    //
    // The initial PC value is obtained from ExceptionAddress rather than
    // from ContextRecord.Fir since some Alpha exceptions are asynchronous.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlMoveMemory(&ContextRecord1, ContextRecord, sizeof(CONTEXT));
    ControlPc = (ULONG_PTR)ExceptionRecord->ExceptionAddress;

#if DBG
    if ((ULONG_PTR)ExceptionRecord->ExceptionAddress != (ULONG_PTR)ContextRecord->Fir) {
        DbgPrint("RtlDispatchException: ExceptionAddress = %p, Fir = %p\n",
                 ExceptionRecord->ExceptionAddress, (ULONG_PTR)ContextRecord->Fir);
    }
#endif

    ExceptionFlags = ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;
    NestedFrame = 0;

#if DBG
    if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION) {
        DbgPrint("\nRtlDispatchException(ExceptionRecord = %p, ContextRecord = %p)\n",
                 ExceptionRecord, ContextRecord);
        DbgPrint("RtlDispatchException: ControlPc = %p, ExceptionRecord->ExceptionCode = %lx\n",
                 ControlPc, ExceptionRecord->ExceptionCode);
    }
#endif

    //
    // Start with the frame specified by the context record and search
    // backwards through the call frame hierarchy attempting to find an
    // exception handler that will handle the exception.
    //

    do {
#if DBG
        if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION_DETAIL) {
            DbgPrint("RtlDispatchException: Loop: FrameDepth = %d, sp = %p, ControlPc = %p\n",
                     FrameDepth, (ULONG_PTR)ContextRecord1.IntSp, ControlPc);
            FrameDepth -= 1;
        }
#endif

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
            NextPc = RtlVirtualUnwind(ControlPc,
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

            if ((EstablisherFrame.Virtual < LowLimit) ||
                (EstablisherFrame.Virtual > HighLimit) ||
                ((EstablisherFrame.Virtual & 0xF) != 0)) {

#if DBG
                DbgPrint("\n****** Warning - stack invalid (exception).\n");
                DbgPrint("  EstablisherFrame.Virtual = %p, EstablisherFrame.Real = %p\n",
                         EstablisherFrame.Virtual, EstablisherFrame.Real);
                DbgPrint("  LowLimit = %p, HighLimit = %p\n",
                         LowLimit, HighLimit);
                DbgPrint("  NextPc = %p, ControlPc = %p\n",
                         NextPc, ControlPc);
                DbgPrint("  Now setting EXCEPTION_STACK_INVALID flag.\n");
#endif

                ExceptionFlags |= EXCEPTION_STACK_INVALID;
                break;

            } else if (IS_HANDLER_DEFINED(FunctionEntry) && InFunction) {

                ULONG Index;

#if DBG
                if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION_DETAIL) {
                    DbgPrint("RtlDispatchException: ExceptionHandler = %p, HandlerData = %p\n",
                             FunctionEntry->ExceptionHandler, FunctionEntry->HandlerData);
                }
#endif

                //
                // The frame has an exception handler. The handler must be
                // executed by calling another routine that is written in
                // assembler. This is required because up level addressing
                // of the handler information is required when a nested
                // exception is encountered.
                //

                DispatcherContext.ControlPc = ControlPc;
                DispatcherContext.FunctionEntry = FunctionEntry;
                DispatcherContext.EstablisherFrame = EstablisherFrame.Virtual;
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

#if DBG
                if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION_DETAIL) {
                    DbgPrint("RtlDispatchException: calling RtlpExecuteHandlerForException, ControlPc = %lx Handler = %lx\n",
                             ControlPc, RF_EXCEPTION_HANDLER(FunctionEntry) );
                }
#endif
                Disposition =
                    RtlpExecuteHandlerForException(ExceptionRecord,
                                                   EstablisherFrame.Virtual,
                                                   ContextRecord,
                                                   &DispatcherContext,
                                                   RF_EXCEPTION_HANDLER(FunctionEntry));
#if DBG
                if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION_DETAIL) {
                    DbgPrint("RtlDispatchException: RtlpExecuteHandlerForException returned Disposition = %lx\n", Disposition);
                }
#endif

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

                if (NestedFrame == EstablisherFrame.Virtual) {
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
#if DBG
                        if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION) {
                            DbgPrint("RtlDispatchException: returning TRUE\n");
                        }
#endif
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

            NextPc = (ULONG_PTR)ContextRecord1.IntRa - 4;

            //
            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            //

            if (NextPc == ControlPc) {
#if DBG
                ULONG Count;
                DbgPrint("\n****** Warning - malformed function table (exception).\n");
                DbgPrint("ControlPc = %p, NextPc %p", NextPc, ControlPc);
                for (Count = 0; Count < PC_HISTORY_DEPTH; Count += 1) {
                    if (ControlPcHistoryIndex > 0) {
                        ControlPcHistoryIndex -= 1;
                        ControlPc = ControlPcHistory[ControlPcHistoryIndex % PC_HISTORY_DEPTH];
                        DbgPrint(", %p", ControlPc);
                    }
                }
                DbgPrint(ControlPcHistoryIndex == 0 ? ".\n" : ", ...\n");
#endif
                break;
            }
        }

        //
        // Set point at which control left the previous routine.
        //

#if DBG
        ControlPcHistory[ControlPcHistoryIndex % PC_HISTORY_DEPTH] = ControlPc;
        ControlPcHistoryIndex += 1;
#endif
        ControlPc = NextPc;

    } while ((ULONG_PTR)ContextRecord1.IntSp < HighLimit);

    //
    // Set final exception flags and return exception not handled.
    //

    ExceptionRecord->ExceptionFlags = ExceptionFlags;
#if DBG
    if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION) {
        DbgPrint("RtlDispatchException: returning FALSE\n");
    }
#endif
    return FALSE;
}

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG_PTR ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables (static and dynamic)
    for an entry that corresponds to the specified PC value. If the entry is for a
    secondary function entry then the primary function table entry is obtained
    via an indirection through the PrologEndAddress. RtlLookupDirectFunctionEntry()
    performs the same function without the indirection to the primary function table.
    Because RtlLookupFunctionEntry() always returns the primary function entry, it
    has the property such that

        RtlLookupFunctionEntry(Pc1) == RtlLookupFunctionEntry(Pc2)

        implies

        Pc1 and Pc2 are in the same procedure.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the primary function table
    entry that corresponds to the specified PC is returned.

--*/

{
    PRUNTIME_FUNCTION FunctionEntry;

    // Look for a static or dynamic function entry

    FunctionEntry = RtlLookupDirectFunctionEntry( ControlPc );

    if (FunctionEntry != NULL) {

        //
        // The capability exists for more than one function entry
        // to map to the same function. This permits a function to
        // have discontiguous code segments described by separate
        // function table entries. If the ending prologue address
        // is not within the limits of the begining and ending
        // address of the function able entry, then the prologue
        // ending address is the address of the primary function
        // table entry that accurately describes the ending prologue
        // address.
        //

        if ((RF_PROLOG_END_ADDRESS(FunctionEntry) <  RF_BEGIN_ADDRESS(FunctionEntry)) ||
            (RF_PROLOG_END_ADDRESS(FunctionEntry) >= RF_END_ADDRESS(FunctionEntry))) {
#if DBG
            ShowRuntimeFunction(FunctionEntry, "RtlLookupFunctionEntry: secondary entry" );
#endif
            // Officially the PrologEndAddress field in secondary function entries
            // doesn't have the exception mode bits there have been some versions
            // of alpha tools that put them there. Strip them off to be safe.

            FunctionEntry = (PRUNTIME_FUNCTION)RF_PROLOG_END_ADDRESS(FunctionEntry);
        } else if (RF_IS_FIXED_RETURN(FunctionEntry)) {
            ULONG_PTR FixedReturn = RF_FIXED_RETURN(FunctionEntry);
    
#if DBG
            ShowRuntimeFunction(FunctionEntry, "LookupFunctionEntry: fixed return entry");
#endif
            // Recursively call LookupFunctionEntry to ensure we get a primary function entry here.
            // Check for incorrectly formed function entry where the fixed return points to itself.
    
            if ((FixedReturn <  RF_BEGIN_ADDRESS(FunctionEntry)) ||
                (FixedReturn >= RF_END_ADDRESS(FunctionEntry))) {
                FunctionEntry = RtlLookupFunctionEntry( RF_FIXED_RETURN(FunctionEntry) );
            }
        }
#if DBG
        else {
            ShowRuntimeFunction(FunctionEntry, "RtlLookupFunctionEntry: primary entry" );
        }
#endif
    }

#if DBG
    if (RtlDebugFlags & RTL_DBG_FUNCTION_ENTRY) {
        DbgPrint("RtlLookupFunctionEntry: returning FunctionEntry = %lx\n", FunctionEntry);
    }
#endif
    return FunctionEntry;
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

#if DBG
    if (RtlDebugFlags & RTL_DBG_RAISE_EXCEPTION) {
        DbgPrint("RtlRaiseException(ExceptionRecord = %p) Status = %lx\n",
                 ExceptionRecord, ExceptionRecord->ExceptionCode);
    }
#endif

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

    ULONG_PTR ControlPc;
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG_PTR NextPc;
    NTSTATUS Status;

    //
    // Capture the current context, virtually unwind to the caller of this
    // routine, set the fault instruction address to that of the caller, and
    // call the raise exception system service.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = (ULONG_PTR)ContextRecord.IntRa - 4;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ContextRecord.Fir = (ULONGLONG)(LONG_PTR)NextPc + 4;
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

#if DBG
    if (RtlDebugFlags & RTL_DBG_RAISE_EXCEPTION) {
        DbgPrint("RtlRaiseStatus(Status = %lx)\n", Status);
    }
#endif

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

    ULONG_PTR ControlPc;
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    EXCEPTION_RECORD ExceptionRecord;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG_PTR NextPc;

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
    ControlPc = (ULONG_PTR)ContextRecord.IntRa - 4;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ContextRecord.Fir = (ULONGLONG)(LONG_PTR)NextPc + 4;
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
    ULONG_PTR ControlPc;
#if DBG
    ULONG_PTR ControlPcHistory[PC_HISTORY_DEPTH];
    ULONG ControlPcHistoryIndex = 0;
#endif
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    FRAME_POINTERS EstablisherFrame;
    ULONG ExceptionFlags;
    EXCEPTION_RECORD ExceptionRecord1;
#if DBG
    LONG FrameDepth = 0;
#endif
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG_PTR HighLimit;
    BOOLEAN InFunction;
    ULONG_PTR LowLimit;
    ULONG_PTR NextPc;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("\nRtlUnwind(TargetFrame = %p, TargetIp = %p,, ReturnValue = %lx)\n",
                 TargetFrame, TargetIp, ReturnValue);
    }
#endif

    //
    // Get current stack limits, capture the current context, virtually
    // unwind to the caller of this routine, get the initial PC value, and
    // set the unwind target address.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlCaptureContext(ContextRecord);
    ControlPc = (ULONG_PTR)ContextRecord->IntRa - 4;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    NextPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ControlPc = NextPc;
    ContextRecord->Fir = (ULONGLONG)(LONG_PTR)TargetIp;

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

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwind: Loop: FrameDepth = %d, sp = %p, ControlPc = %p\n",
                 FrameDepth, (ULONG_PTR)ContextRecord->IntSp, ControlPc);
        FrameDepth -= 1;
    }
#endif

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
                                       &EstablisherFrame);

            //
            // If the virtual frame pointer is not within the specified stack
            // limits, the virtual frame pointer is unaligned, or the target
            // frame is below the virtual frame and an exit unwind is not being
            // performed, then raise the exception STATUS_BAD_STACK. Otherwise,
            // check to determine if the current routine has an exception
            // handler.
            //

            if ((EstablisherFrame.Virtual < LowLimit) ||
                (EstablisherFrame.Virtual > HighLimit) ||
                ((ARGUMENT_PRESENT(TargetFrame) != FALSE) &&
                 ((ULONG_PTR)TargetFrame < EstablisherFrame.Virtual)) ||
                ((EstablisherFrame.Virtual & 0xF) != 0)) {

#if DBG
                DbgPrint("\n****** Warning - bad stack or target frame (unwind).\n");
                DbgPrint("  EstablisherFrame Virtual = %p, Real = %p\n",
                         EstablisherFrame.Virtual, EstablisherFrame.Real);
                DbgPrint("  TargetFrame = %p\n", TargetFrame);
                if ((ARGUMENT_PRESENT(TargetFrame) != FALSE) &&
                    ((ULONG_PTR)TargetFrame < EstablisherFrame.Virtual)) {
                    DbgPrint("  TargetFrame is below EstablisherFrame!\n");
                }
                DbgPrint("  Previous EstablisherFrame (sp) = %p\n",
                         (ULONG_PTR)ContextRecord->IntSp);
                DbgPrint("  LowLimit = %p, HighLimit = %p\n",
                         LowLimit, HighLimit);
                DbgPrint("  NextPc = %p, ControlPc = %p\n",
                         NextPc, ControlPc);
                DbgPrint("  Now raising STATUS_BAD_STACK exception.\n");
#endif

                RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);

            } else if (IS_HANDLER_DEFINED(FunctionEntry) && InFunction) {

#if DBG
    if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION_DETAIL) {
    DbgPrint("RtlUnwind: ExceptionHandler = %p, HandlerData = %p\n",
             FunctionEntry->ExceptionHandler, FunctionEntry->HandlerData);
}
#endif

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
                DispatcherContext.EstablisherFrame = EstablisherFrame.Virtual;
                DispatcherContext.ContextRecord = ContextRecord;

                //
                // Call the exception handler.
                //

                do {

                    //
                    // If the establisher frame is the target of the unwind
                    // operation, then set the target unwind flag.
                    //

                    if ((ULONG_PTR)TargetFrame == EstablisherFrame.Virtual) {
                        ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
                    }

                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    //
                    // Set the specified return value in case the exception
                    // handler directly continues execution.
                    //

                    ContextRecord->IntV0 = (ULONGLONG)(LONG_PTR)ReturnValue;
#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwind: calling RtlpExecuteHandlerForUnwind, ControlPc = %p\n", ControlPc);
    }
#endif
                    Disposition =
                        RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                    EstablisherFrame.Virtual,
                                                    ContextRecord,
                                                    &DispatcherContext,
                                                    RF_EXCEPTION_HANDLER(FunctionEntry));
#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwind: RtlpExecuteHandlerForUnwind returned Disposition = %lx\n", Disposition);
    }
#endif

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
                        if (EstablisherFrame.Virtual != (ULONG_PTR)TargetFrame) {
                            NextPc = RtlVirtualUnwind(ControlPc,
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
                        ContextRecord->Fir = (ULONGLONG)(LONG_PTR)TargetIp;
                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        EstablisherFrame.Virtual = DispatcherContext.EstablisherFrame;
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
                // Virtually unwind to the caller of the current routine and
                // update the context record.
                //

                if (EstablisherFrame.Virtual != (ULONG_PTR)TargetFrame) {
                    NextPc = RtlVirtualUnwind(ControlPc,
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

            NextPc = (ULONG_PTR)ContextRecord->IntRa - 4;

            //
            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            //

            if (NextPc == ControlPc) {
#if DBG
                ULONG Count;
                DbgPrint("\n****** Warning - malformed function table (unwind).\n");
                DbgPrint("ControlPc = %p, %p", NextPc, ControlPc);
                for (Count = 0; Count < PC_HISTORY_DEPTH; Count += 1) {
                    if (ControlPcHistoryIndex > 0) {
                        ControlPcHistoryIndex -= 1;
                        ControlPc = ControlPcHistory[ControlPcHistoryIndex % PC_HISTORY_DEPTH];
                        DbgPrint(", %p", ControlPc);
                    }
                }
                DbgPrint(ControlPcHistoryIndex == 0 ? ".\n" : ", ...\n");
                DbgPrint("  Now raising STATUS_BAD_FUNCTION_TABLE exception.\n");
#endif
                RtlRaiseStatus(STATUS_BAD_FUNCTION_TABLE);
            }
        }

        //
        // Set point at which control left the previous routine.
        //

#if DBG
        ControlPcHistory[ControlPcHistoryIndex % PC_HISTORY_DEPTH] = ControlPc;
        ControlPcHistoryIndex += 1;
#endif
        ControlPc = NextPc;

    } while ((EstablisherFrame.Virtual < HighLimit) &&
             (EstablisherFrame.Virtual != (ULONG_PTR)TargetFrame));

    //
    // If the establisher stack pointer is equal to the target frame
    // pointer, then continue execution. Otherwise, an exit unwind was
    // performed or the target of the unwind did not exist and the
    // debugger and subsystem are given a second chance to handle the
    // unwind.
    //

    if (EstablisherFrame.Virtual == (ULONG_PTR)TargetFrame) {
        ContextRecord->IntV0 = (ULONGLONG)(LONG_PTR)ReturnValue;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("RtlUnwind: finished unwinding, and calling RtlpRestoreContext(%lx)\n",ContextRecord);
    }
#endif

        RtlpRestoreContext(ContextRecord);

    } else {

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("RtlUnwind: finished unwinding, but calling ZwRaiseException\n");
    }
#endif

        ZwRaiseException(ExceptionRecord, ContextRecord, FALSE);
    }

}

#if DBG
//
// Define an array of symbolic names for the integer registers.
//

PCHAR RtlpIntegerRegisterNames[32] = {
    "v0",  "t0",  "t1",  "t2",  "t3",  "t4",  "t5",  "t6",      // 0 - 7
    "t7",  "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "fp",      // 8 - 15
    "a0",  "a1",  "a2",  "a3",  "a4",  "a5",  "t8",  "t9",      // 16 - 23
    "t10", "t11", "ra",  "t12", "at",  "gp",  "sp",  "zero",    // 24 - 31
};

//
// This function disassembles the instruction at the given address. It is
// only used for debugging and recognizes only those few instructions that
// are relevant during reverse execution of the prologue by virtual unwind.
//

VOID
_RtlpDebugDisassemble (
    IN ULONG_PTR ControlPc,
    IN PCONTEXT ContextRecord
    )

{
    UCHAR Comments[50];
    PULONGLONG FloatingRegister;
    ULONG Function;
    ULONG Hint;
    ULONG Literal8;
    ALPHA_INSTRUCTION Instruction;
    PULONGLONG IntegerRegister;
    LONG  Offset16;
    UCHAR Operands[20];
    ULONG Opcode;
    PCHAR OpName;
    ULONG Ra;
    ULONG Rb;
    ULONG Rc;
    PCHAR RaName;
    PCHAR RbName;
    PCHAR RcName;

    if (RtlDebugFlags & RTL_DBG_VIRTUAL_UNWIND_DETAIL) {
        Instruction.Long = *((PULONG)ControlPc);
        Hint = Instruction.Jump.Hint;
        Literal8 = Instruction.OpLit.Literal;
        Offset16 = Instruction.Memory.MemDisp;
        Opcode = Instruction.Memory.Opcode;
        Ra = Instruction.OpReg.Ra;
        RaName = RtlpIntegerRegisterNames[Ra];
        Rb = Instruction.OpReg.Rb;
        RbName = RtlpIntegerRegisterNames[Rb];
        Rc = Instruction.OpReg.Rc;
        RcName = RtlpIntegerRegisterNames[Rc];

        IntegerRegister = &ContextRecord->IntV0;
        FloatingRegister = &ContextRecord->FltF0;

        OpName = NULL;
        switch (Opcode) {
        case JMP_OP :
            if (Instruction.Jump.Function == RET_FUNC) {
                OpName = "ret";
                sprintf(Operands, "%s, (%s), %04lx", RaName, RbName, Hint);
                sprintf(Comments, "%s = %Lx", RbName, IntegerRegister[Rb]);
            }
            break;

        case LDAH_OP :
        case LDA_OP :
        case STQ_OP :
            if (Opcode == LDA_OP) {
                OpName = "lda";

            } else if (Opcode == LDAH_OP) {
                OpName = "ldah";

            } else if (Opcode == STQ_OP) {
                OpName = "stq";
            }
            sprintf(Operands, "%s, $%d(%s)", RaName, Offset16, RbName);
            sprintf(Comments, "%s = %Lx", RaName, IntegerRegister[Ra]);
            break;

        case ARITH_OP :
        case BIT_OP :
            Function = Instruction.OpReg.Function;
            if ((Opcode == ARITH_OP) && (Function == ADDQ_FUNC)) {
                    OpName = "addq";

            } else if ((Opcode == ARITH_OP) && (Function == SUBQ_FUNC)) {
                    OpName = "subq";

            } else if ((Opcode == BIT_OP) && (Function == BIS_FUNC)) {
                    OpName = "bis";

            } else {
                break;
            }
            if (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT) {
                sprintf(Operands, "%s, %s, %s", RaName, RbName, RcName);

            } else {
                sprintf(Operands, "%s, $%d, %s", RaName, Literal8, RcName);
            }
            sprintf(Comments, "%s = %Lx", RcName, IntegerRegister[Rc]);
            break;

        case FPOP_OP :
            if (Instruction.FpOp.Function == CPYS_FUNC) {
                OpName = "cpys";
                sprintf(Operands, "f%d, f%d, f%d", Ra, Rb, Rc);
                sprintf(Comments, "f%d = %Lx", Rc, FloatingRegister[Rc]);
            }
            break;

        case STT_OP :
            OpName = "stt";
            sprintf(Operands, "f%d, $%d(%s)", Ra, Offset16, RbName);
            sprintf(Comments, "f%d = %Lx", Ra, FloatingRegister[Ra]);
            break;
        }
        if (OpName == NULL) {
            OpName = "???";
            sprintf(Operands, "...");
            sprintf(Comments, "Unknown to virtual unwind.");
        }
        DbgPrint("    %p: %08lx  %-5s %-16s // %s\n",
                 ControlPc, Instruction.Long, OpName, Operands, Comments);
    }
    return;
}

#define _RtlpFoundTrapFrame(NextPc) \
    if (RtlDebugFlags & RTL_DBG_VIRTUAL_UNWIND) { \
        DbgPrint("    *** Looks like a trap frame (fake prologue), Fir = %lx\n", \
                 NextPc); \
    }

#define _RtlpVirtualUnwindExit(NextPc, ContextRecord, EstablisherFrame) \
    if (RtlDebugFlags & RTL_DBG_VIRTUAL_UNWIND) { \
        DbgPrint("RtlVirtualUnwind: EstablisherFrame Virtual = %08lx, Real = %08lx\n", \
        (EstablisherFrame)->Virtual, (EstablisherFrame)->Real); \
        DbgPrint("RtlVirtualUnwind: returning NextPc = %p, sp = %p\n\n", \
                 NextPc, (ULONG_PTR)ContextRecord->IntSp); \
    }

#else

#define _RtlpDebugDisassemble(ControlPc, ContextRecord)
#define _RtlpFoundTrapFrame(NextPc)
#define _RtlpVirtualUnwindExit(NextPc, ContextRecord, EstablisherFrame)

#endif

ULONG_PTR
RtlVirtualUnwind (
    IN ULONG_PTR ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    )

/*++

Routine Description:

    This function virtually unwinds the specified function by executing its
    prologue code backwards. Given the current context and the instructions
    that preserve registers in the prologue, it is possible to recreate the
    nonvolatile context at the point the function was called.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is placed in the updated context record.

    During the unwind, the virtual and real frame pointers for the function
    are calculated and returned in the given frame pointers structure.

    If a context pointers record is specified, then the address where each
    register is restored from is recorded in the appropriate element of the
    context pointers record.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    InFunction - Supplies a pointer to a variable that receives whether the
        control PC is within the current function.

    EstablisherFrame - Supplies a pointer to a frame pointers structure
        that will receive the values for the virtual frame pointer and the
        real frame pointer. The value of the real frame pointer is reliable
        only when InFunction is TRUE.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

    The address where control left the previous frame is returned as the
    function value.

Implementation Notes:

    N.B. "where control left" is not the "return address" of the call in the
    previous frame. For normal frames, NextPc points to the last instruction
    that completed in the previous frame (the JSR/BSR). The difference between
    NextPc and NextPc + 4 (return address) is important for correct behavior
    in boundary cases of exception addresses and scope tables.

    For exception and interrupt frames, NextPc is obtained from the trap frame
    contination address (Fir). For faults and synchronous traps, NextPc is both
    the last instruction to execute in the previous frame and the next
    instruction to execute if the function were to return. For asynchronous
    traps, NextPc is the continuation address. It is the responsibility of the
    compiler to insert TRAPB instructions to insure asynchronous traps do not
    occur outside the scope from the instruction(s) that caused them.

    N.B. in this and other files where RtlVirtualUnwind is used, the variable
    named NextPc is perhaps more accurately, LastPc - the last PC value in
    the previous frame, or CallPc - the address of the call instruction, or
    ControlPc - the address where control left the previous frame. Instead
    think of NextPc as the next PC to use in another call to virtual unwind.

    The Alpha version of virtual unwind is similar in design, but slightly
    more complex than the Mips version. This is because Alpha compilers
    are given more flexibility to optimize generated code and instruction
    sequences, including within procedure prologues. In addition, because of
    the current inability of the GEM compiler to materialize virtual frame
    pointers, this function must manage both virtual and real frame pointers.

--*/

{

    ULONG_PTR Address;
    ULONG DecrementOffset;
    ULONG DecrementRegister;
    ALPHA_INSTRUCTION FollowingInstruction;
    PULONGLONG FloatingRegister;
    ULONG_PTR FrameSize;
    ULONG Function;
    ALPHA_INSTRUCTION Instruction;
    PULONGLONG IntegerRegister;
    ULONG Literal8;
    ULONG_PTR NextPc;
    LONG Offset16;
    ULONG Opcode;
    ULONG Ra;
    ULONG Rb;
    ULONG Rc;
    BOOLEAN RestoredRa;
    BOOLEAN RestoredSp;
    RUNTIME_FUNCTION UnwindFunctionEntry;
    ULONG StackAdjust;
    ULONG_PTR FixedReturn;

#if DBG
    if (RtlDebugFlags & RTL_DBG_VIRTUAL_UNWIND) {
        DbgPrint("\nRtlVirtualUnwind(ControlPc = %p, FunctionEntry = %p,) sp = %p\n",
                 ControlPc, FunctionEntry, (ULONG_PTR)ContextRecord->IntSp);
    }
#endif

    // Construct a function entry suitable for unwinding from ControlPc

    RtlGetUnwindFunctionEntry( ControlPc, FunctionEntry, &UnwindFunctionEntry, &StackAdjust, &FixedReturn );

#if DBG
    ShowRuntimeFunction(&UnwindFunctionEntry, "RtlVirtualUnwind: unwind function entry" );
#endif
    //
    // Set the base address of the integer and floating register arrays within
    // the context record. Each set of 32 registers is known to be contiguous.
    //

    IntegerRegister = &ContextRecord->IntV0;
    FloatingRegister = &ContextRecord->FltF0;

    //
    // Handle the epilogue case where the next instruction is a return.
    //
    // Exception handlers cannot be called if the ControlPc is within the
    // epilogue because exception handlers expect to operate with a current
    // stack frame. The value of SP is not current within the epilogue.
    //

    Instruction.Long = *((PULONG)ControlPc);
    if (IS_RETURN_0001_INSTRUCTION(Instruction.Long)) {
        Rb = Instruction.Jump.Rb;
        NextPc = (ULONG_PTR)IntegerRegister[Rb] - 4;

        //
        // The instruction at the point where control left the specified
        // function is a return, so any saved registers have already been
        // restored, and the stack pointer has already been adjusted. The
        // stack does not need to be unwound in this case and the saved
        // return address register is returned as the function value.
        //
        // In fact, reverse execution of the prologue is not possible in
        // this case: the stack pointer has already been incremented and
        // so, for this frame, neither a valid stack pointer nor frame
        // pointer exists from which to begin reverse execution of the
        // prologue. In addition, the integrity of any data on the stack
        // below the stack pointer is never guaranteed (due to interrupts
        // and exceptions).
        //
        // The epilogue instruction sequence is:
        //
        // ==>  ret   zero, (Ra), 1     // return
        // or
        //
        //      mov   ra, Rx            // save return address
        //      ...
        // ==>  ret   zero, (Rx), 1     // return
        //

        EstablisherFrame->Real = 0;
        EstablisherFrame->Virtual = (ULONG_PTR)ContextRecord->IntSp;
        *InFunction = FALSE;
        _RtlpDebugDisassemble(ControlPc, ContextRecord);
        _RtlpVirtualUnwindExit(NextPc, ContextRecord, EstablisherFrame);
        return NextPc;
    }

    //
    // Handle the epilogue case where the next two instructions are a stack
    // frame deallocation and a return.
    //

    FollowingInstruction.Long = *((PULONG)(ControlPc + 4));
    if (IS_RETURN_0001_INSTRUCTION(FollowingInstruction.Long)) {
        Rb = FollowingInstruction.Jump.Rb;
        NextPc = (ULONG_PTR)IntegerRegister[Rb] - 4;

        //
        // The second instruction following the point where control
        // left the specified function is a return. If the instruction
        // before the return is a stack increment instruction, then all
        // saved registers have already been restored except for SP.
        // The value of the stack pointer register cannot be recovered
        // through reverse execution of the prologue because in order
        // to begin reverse execution either the stack pointer or the
        // frame pointer (if any) must still be valid.
        //
        // Instead, the effect that the stack increment instruction
        // would have had on the context is manually applied to the
        // current context. This is forward execution of the epilogue
        // rather than reverse execution of the prologue.
        //
        // In an epilogue, as in a prologue, the stack pointer is always
        // adjusted with a single instruction: either an immediate-value
        // (lda) or a register-value (addq) add instruction.
        //

        Function = Instruction.OpReg.Function;
        Offset16 = Instruction.Memory.MemDisp;
        Opcode = Instruction.OpReg.Opcode;
        Ra = Instruction.OpReg.Ra;
        Rb = Instruction.OpReg.Rb;
        Rc = Instruction.OpReg.Rc;

        if ((Opcode == LDA_OP) && (Ra == SP_REG)) {

            //
            // Load Address instruction.
            //
            // Since the destination (Ra) register is SP, an immediate-
            // value stack deallocation operation is being performed. The
            // displacement value should be added to SP. The displacement
            // value is assumed to be positive. The amount of stack
            // deallocation possible using this instruction ranges from
            // 16 to 32752 (32768 - 16) bytes. The base register (Rb) is
            // usually SP, but may be another register.
            //
            // The epilogue instruction sequence is:
            //
            // ==>  lda   sp, +N(sp)        // deallocate stack frame
            //      ret   zero, (ra)        // return
            // or
            //
            // ==>  lda   sp, +N(Rx)        // restore SP and deallocate frame
            //      ret   zero, (ra)        // return
            //

            ContextRecord->IntSp = Offset16 + IntegerRegister[Rb];
            EstablisherFrame->Real = 0;
            EstablisherFrame->Virtual = (ULONG_PTR)ContextRecord->IntSp;
            *InFunction = FALSE;
            _RtlpDebugDisassemble(ControlPc, ContextRecord);
            _RtlpDebugDisassemble(ControlPc + 4, ContextRecord);
            _RtlpVirtualUnwindExit(NextPc, ContextRecord, EstablisherFrame);
            return NextPc;

        } else if ((Opcode == ARITH_OP) && (Function == ADDQ_FUNC) &&
                   (Rc == SP_REG) &&
                   (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT)) {

            //
            // Add Quadword instruction.
            //
            // Since both source operands are registers, and the
            // destination register is SP, a register-value stack
            // deallocation is being performed. The value of the two
            // source registers should be added and this is the new
            // value of SP. One of the source registers is usually SP,
            // but may be another register.
            //
            // The epilogue instruction sequence is:
            //
            //      ldiq  Rx, N             // set [large] frame size
            //      ...
            // ==>  addq  sp, Rx, sp        // deallocate stack frame
            //      ret   zero, (ra)        // return
            // or
            //
            // ==>  addq  Rx, Ry, sp        // restore SP and deallocate frame
            //      ret   zero, (ra)        // return
            //

            ContextRecord->IntSp = IntegerRegister[Ra] + IntegerRegister[Rb];
            EstablisherFrame->Real = 0;
            EstablisherFrame->Virtual = (ULONG_PTR)ContextRecord->IntSp;
            *InFunction = FALSE;
            _RtlpDebugDisassemble(ControlPc, ContextRecord);
            _RtlpDebugDisassemble(ControlPc + 4, ContextRecord);
            _RtlpVirtualUnwindExit(NextPc, ContextRecord, EstablisherFrame);
            return NextPc;
        }
    }

    //
    // By default set the frame pointers to the current value of SP.
    //
    // When a procedure is called, the value of SP before the stack
    // allocation instruction is the virtual frame pointer. When reverse
    // executing instructions in the prologue, the value of SP before the
    // stack allocation instruction is encountered is the real frame
    // pointer. This is the current value of SP unless the procedure uses
    // a frame pointer (e.g., FP_REG).
    //

    EstablisherFrame->Real = (ULONG_PTR)ContextRecord->IntSp;
    EstablisherFrame->Virtual = (ULONG_PTR)ContextRecord->IntSp;

    //
    // If the address where control left the specified function is beyond
    // the end of the prologue, then the control PC is considered to be
    // within the function and the control address is set to the end of
    // the prologue. Otherwise, the control PC is not considered to be
    // within the function (i.e., the prologue).
    //
    // N.B. PrologEndAddress is equal to BeginAddress for a leaf function.
    //
    // The low-order two bits of PrologEndAddress are reserved for the IEEE
    // exception mode and so must be masked out.
    //

    if ((ControlPc < UnwindFunctionEntry.BeginAddress) ||
        (ControlPc >= UnwindFunctionEntry.PrologEndAddress)) {
        *InFunction = TRUE;
        ControlPc = (UnwindFunctionEntry.PrologEndAddress & (~(UINT_PTR)0x3));

    } else {
        *InFunction = FALSE;
    }

    //
    // Scan backward through the prologue to reload callee saved registers
    // that were stored or copied and to increment the stack pointer if it
    // was decremented.
    //

    DecrementRegister = ZERO_REG;
    NextPc = (ULONG_PTR)ContextRecord->IntRa - 4;
    RestoredRa = FALSE;
    RestoredSp = FALSE;
    while (ControlPc > UnwindFunctionEntry.BeginAddress) {

        //
        // Get instruction value, decode fields, case on opcode value, and
        // reverse register store and stack decrement operations.
        // N.B. The location of Opcode, Ra, Rb, and Rc is the same across
        // all opcode formats. The same is not true for Function.
        //

        ControlPc -= 4;
        Instruction.Long = *((PULONG)ControlPc);
        Function = Instruction.OpReg.Function;
        Literal8 = Instruction.OpLit.Literal;
        Offset16 = Instruction.Memory.MemDisp;
        Opcode = Instruction.OpReg.Opcode;
        Ra = Instruction.OpReg.Ra;
        Rb = Instruction.OpReg.Rb;
        Rc = Instruction.OpReg.Rc;

        //
        // Compare against each instruction type that will affect the context
        // and that is allowed in a prologue. Any other instructions found
        // in the prologue will be ignored since they are assumed to have no
        // effect on the context.
        //

        switch (Opcode) {

        case STQ_OP :

            //
            // Store Quad instruction.
            //
            // If the base register is SP, then reload the source register
            // value from the value stored on the stack.
            //
            // The prologue instruction sequence is:
            //
            // ==>  stq   Rx, N(sp)         // save integer register Rx
            //

            if ((Rb == SP_REG) && (Ra != ZERO_REG)) {

                //
                // Reload the register by retrieving the value previously
                // stored on the stack.
                //

                Address = Offset16 + (LONG_PTR)ContextRecord->IntSp;
                IntegerRegister[Ra] = *((PULONGLONG)Address);

                //
                // If the destination register is RA and this is the first
                // time that RA is being restored, then set the address of
                // where control left the previous frame. Otherwise, if this
                // is the second time RA is being restored, then the first
                // one was an interrupt or exception address and the return
                // PC should not have been biased by 4.
                //

                if (Ra == RA_REG) {
                    if (RestoredRa == FALSE) {
                        NextPc = (ULONG_PTR)ContextRecord->IntRa - 4;
                        RestoredRa = TRUE;

                    } else {
                        NextPc += 4;
                        _RtlpFoundTrapFrame(NextPc);
                    }

                //
                // Otherwise, if the destination register is SP and this is
                // the first time that SP is being restored, then set the
                // establisher frame pointers.
                //

                } else if ((Ra == SP_REG) && (RestoredSp == FALSE)) {
                    EstablisherFrame->Virtual = (ULONG_PTR)ContextRecord->IntSp;
                    EstablisherFrame->Real = (ULONG_PTR)ContextRecord->IntSp;
                    RestoredSp = TRUE;
                }

                //
                // If a context pointer record is specified, then record
                // the address where the destination register contents
                // are stored.
                //

                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->IntegerContext[Ra] = (PULONGLONG)Address;
                }
                _RtlpDebugDisassemble(ControlPc, ContextRecord);
            }
            break;

        case LDAH_OP :
            Offset16 <<= 16;

        case LDA_OP :

            //
            // Load Address High, Load Address instruction.
            //
            // There are several cases where the lda and/or ldah instructions
            // are used: one to decrement the stack pointer directly, and the
            // others to load immediate values into another register and that
            // register is then used to decrement the stack pointer.
            //
            // In the examples below, as a single instructions or as a pair,
            // a lda may be substituted for a ldah and visa-versa.
            //

            if (Ra == SP_REG) {
                if (Rb == SP_REG) {

                    //
                    // If both the destination (Ra) and base (Rb) registers
                    // are SP, then a standard stack allocation was performed
                    // and the negated displacement value is the stack frame
                    // size. The amount of stack allocation possible using
                    // the lda instruction ranges from 16 to 32768 bytes and
                    // the amount of stack allocation possible using the ldah
                    // instruction ranges from 65536 to 2GB in multiples of
                    // 65536 bytes. It is rare for the ldah instruction to be
                    // used in this manner.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  lda   sp, -N(sp)    // allocate stack frame
                    //

                    FrameSize = -Offset16;
                    goto StackAllocation;

                } else {

                    //
                    // The destination register is SP and the base register
                    // is not SP, so this instruction must be the second
                    // half of an instruction pair to allocate a large size
                    // (>32768 bytes) stack frame. Save the displacement value
                    // as the partial decrement value and postpone adjusting
                    // the value of SP until the first instruction of the pair
                    // is encountered.
                    //
                    // The prologue instruction sequence is:
                    //
                    //      ldah  Rx, -N(sp)    // prepare new SP (upper)
                    // ==>  lda   sp, sN(Rx)    // allocate stack frame
                    //

                    DecrementRegister = Rb;
                    DecrementOffset = Offset16;
                    _RtlpDebugDisassemble(ControlPc, ContextRecord);
                }

            } else if (Ra == DecrementRegister) {
                if (Rb == DecrementRegister) {

                    //
                    // Both the destination and base registers are the
                    // decrement register, so this instruction exists as the
                    // second half of a two instruction pair to load a
                    // 31-bit immediate value into the decrement register.
                    // Save the displacement value as the partial decrement
                    // value.
                    //
                    // The prologue instruction sequence is:
                    //
                    //      ldah  Rx, +N(zero)      // set frame size (upper)
                    // ==>  lda   Rx, sN(Rx)        // set frame size (+lower)
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    //

                    DecrementOffset += Offset16;
                    _RtlpDebugDisassemble(ControlPc, ContextRecord);

                } else if (Rb == ZERO_REG) {

                    //
                    // The destination register is the decrement register and
                    // the base register is zero, so this instruction exists
                    // to load an immediate value into the decrement register.
                    // The stack frame size is the new displacement value added
                    // to the previous displacement value, if any.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  lda   Rx, +N(zero)      // set frame size
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    // or
                    //
                    // ==>  ldah  Rx, +N(zero)      // set frame size (upper)
                    //      lda   Rx, sN(Rx)        // set frame size (+lower)
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    //

                    FrameSize = (Offset16 + DecrementOffset);
                    goto StackAllocation;

                } else if (Rb == SP_REG) {

                    //
                    // The destination (Ra) register is SP and the base (Rb)
                    // register is the decrement register, so a two
                    // instruction, large size (>32768 bytes) stack frame
                    // allocation was performed. Add the new displacement
                    // value to the previous displacement value. The negated
                    // displacement value is the stack frame size.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  ldah  Rx, -N(sp)    // prepare new SP (upper)
                    //      lda   sp, sN(Rx)    // allocate stack frame
                    //

                    FrameSize = -(Offset16 + (LONG)DecrementOffset);
                    goto StackAllocation;
                }
            }
            break;

        case ARITH_OP :

            if ((Function == ADDQ_FUNC) &&
                (Instruction.OpReg.RbvType != RBV_REGISTER_FORMAT)) {

                //
                // Add Quadword (immediate) instruction.
                //
                // If the first source register is zero, and the second
                // operand is a literal, and the destination register is
                // the decrement register, then the instruction exists
                // to load an unsigned immediate value less than 256 into
                // the decrement register. The immediate value is the stack
                // frame size.
                //
                // The prologue instruction sequence is:
                //
                // ==>  addq  zero, N, Rx       // set frame size
                //      ...
                //      subq  sp, Rx, sp        // allocate stack frame
                //

                if ((Ra == ZERO_REG) && (Rc == DecrementRegister)) {
                    FrameSize = Literal8;
                    goto StackAllocation;
                }

            } else if ((Function == SUBQ_FUNC) &&
                       (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT)) {

                //
                // Subtract Quadword (register) instruction.
                //
                // If both source operands are registers and the first
                // source (minuend) register and the destination
                // (difference) register are both SP, then a register value
                // stack allocation was performed and the second source
                // (subtrahend) register value will be added to SP when its
                // value is known. Until that time save the register number of
                // this decrement register.
                //
                // The prologue instruction sequence is:
                //
                //      ldiq  Rx, N             // set frame size
                //      ...
                // ==>  subq  sp, Rx, sp        // allocate stack frame
                //

                if ((Ra == SP_REG) && (Rc == SP_REG)) {
                    DecrementRegister = Rb;
                    DecrementOffset = 0;
                    _RtlpDebugDisassemble(ControlPc, ContextRecord);
                }
            }
            break;

        case BIT_OP :

            //
            // If the second operand is a register the bit set instruction
            // may be a register move instruction, otherwise if the second
            // operand is a literal, the bit set instruction may be a load
            // immediate value instruction.
            //

            if ((Function == BIS_FUNC) && (Rc != ZERO_REG)) {
                if (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT) {

                    //
                    // Bit Set (register move) instruction.
                    //
                    // If both source registers are the same register, or
                    // one of the source registers is zero, then this is a
                    // register move operation. Restore the value of the
                    // source register by copying the current destination
                    // register value back to the source register.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  bis   Rx, Rx, Ry        // copy register Rx
                    // or
                    //
                    // ==>  bis   Rx, zero, Ry      // copy register Rx
                    // or
                    //
                    // ==>  bis   zero, Rx, Ry      // copy register Rx
                    //

                    if (Ra == ZERO_REG) {

                        //
                        // Map the third case above to the first case.
                        //

                        Ra = Rb;

                    } else if (Rb == ZERO_REG) {

                        //
                        // Map the second case above to the first case.
                        //

                        Rb = Ra;
                    }

                    if ((Ra == Rb) && (Ra != ZERO_REG)) {
                        IntegerRegister[Ra] = IntegerRegister[Rc];

                        //
                        // If the destination register is RA and this is the
                        // first time that RA is being restored, then set the
                        // address of where control left the previous frame.
                        // Otherwise, if this is the second time RA is being
                        // restored, then the first one was an interrupt or
                        // exception address and the return PC should not
                        // have been biased by 4.
                        //

                        if (Ra == RA_REG) {
                            if (RestoredRa == FALSE) {
                                NextPc = (ULONG_PTR)ContextRecord->IntRa - 4;
                                RestoredRa = TRUE;

                            } else {
                                NextPc += 4;
                                _RtlpFoundTrapFrame(NextPc);
                            }

                        //
                        // If the source register is SP and this is the first
                        // time SP is set, then this is a frame pointer set
                        // instruction. Reset the frame pointers to this new
                        // value of SP.
                        //

                        } else if ((Ra == SP_REG) && (RestoredSp == FALSE)) {
                            EstablisherFrame->Virtual = (ULONG_PTR)ContextRecord->IntSp;
                            EstablisherFrame->Real = (ULONG_PTR)ContextRecord->IntSp;
                            RestoredSp = TRUE;
                        }

                        _RtlpDebugDisassemble(ControlPc, ContextRecord);
                    }

                } else {

                    //
                    // Bit Set (load immediate) instruction.
                    //
                    // If the first source register is zero, and the second
                    // operand is a literal, and the destination register is
                    // the decrement register, then this instruction exists
                    // to load an unsigned immediate value less than 256 into
                    // the decrement register. The decrement register value is
                    // the stack frame size.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  bis   zero, N, Rx       // set frame size
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    //

                    if ((Ra == ZERO_REG) && (Rc == DecrementRegister)) {
                        FrameSize = Literal8;
StackAllocation:
                        //
                        // Add the frame size to SP to reverse the stack frame
                        // allocation, leave the real frame pointer as is, set
                        // the virtual frame pointer with the updated SP value,
                        // and clear the decrement register.
                        //

                        ContextRecord->IntSp += FrameSize;
                        EstablisherFrame->Virtual = (ULONG_PTR)ContextRecord->IntSp;
                        DecrementRegister = ZERO_REG;
                        _RtlpDebugDisassemble(ControlPc, ContextRecord);
                    }
                }
            }
            break;

        case STT_OP :

            //
            // Store T-Floating (quadword integer) instruction.
            //
            // If the base register is SP, then reload the source register
            // value from the value stored on the stack.
            //
            // The prologue instruction sequence is:
            //
            // ==>  stt   Fx, N(sp)         // save floating register Fx
            //

            if ((Rb == SP_REG) && (Ra != FZERO_REG)) {

                //
                // Reload the register by retrieving the value previously
                // stored on the stack.
                //

                Address = Offset16 + (LONG_PTR)ContextRecord->IntSp;
                FloatingRegister[Ra] = *((PULONGLONG)Address);

                //
                // If a context pointer record is specified, then record
                // the address where the destination register contents are
                // stored.
                //

                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->FloatingContext[Ra] = (PULONGLONG)Address;
                }
                _RtlpDebugDisassemble(ControlPc, ContextRecord);
            }
            break;

        case FPOP_OP :

            //
            // N.B. The floating operate function field is not the same as
            // the integer operate nor the jump function fields.
            //

            if (Instruction.FpOp.Function == CPYS_FUNC) {

                //
                // Copy Sign (floating-point move) instruction.
                //
                // If both source registers are the same register, then this is
                // a floating-point register move operation. Restore the value
                // of the source register by copying the current destination
                // register value to the source register.
                //
                // The prologue instruction sequence is:
                //
                // ==>  cpys  Fx, Fx, Fy        // copy floating register Fx
                //

                if ((Ra == Rb) && (Ra != FZERO_REG)) {
                    FloatingRegister[Ra] = FloatingRegister[Rc];
                    _RtlpDebugDisassemble(ControlPc, ContextRecord);
                }
            }

        default :
            break;
        }
    }
    
    // Check for exlicit stack adjust amount
    
    if (StackAdjust) {
        ContextRecord->IntSp += StackAdjust;
    }

    if (FixedReturn != 0) {
        NextPc = FixedReturn;
    }
    
    _RtlpVirtualUnwindExit(NextPc, ContextRecord, EstablisherFrame);
    return NextPc;
}

ULONG_PTR
RtlpVirtualUnwind (
    IN ULONG_PTR ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame
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
    return RtlVirtualUnwind(ControlPc,
                            FunctionEntry,
                            &LocalContext,
                            InFunction,
                            EstablisherFrame,
                            NULL);
}

PRUNTIME_FUNCTION
RtlLookupDirectFunctionEntry (
    IN ULONG_PTR ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables (static and dynamic)
    for an entry that corresponds to the specified PC value.

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
    BOOLEAN InImage;

    //
    // look for function entry in static function tables
    //

    FunctionEntry = RtlLookupStaticFunctionEntry( ControlPc, &InImage );

#if !defined(NTOS_KERNEL_RUNTIME)
    //
    // If not in static image range and no static function entry
    // found then look for a dynamic function entry
    //

    if (FunctionEntry == NULL && !InImage) {

        FunctionEntry = RtlLookupDynamicFunctionEntry( ControlPc );

    }
#endif

#if DBG
    if (RtlDebugFlags & RTL_DBG_FUNCTION_ENTRY) {
        DbgPrint("RtlLookupDirectFunctionEntry: returning FunctionEntry = %p\n", FunctionEntry);
    }
#endif

    return FunctionEntry;
}

PRUNTIME_FUNCTION
RtlLookupStaticFunctionEntry(
    IN ULONG_PTR ControlPc,
    OUT PBOOLEAN InImage
    )

/*++

Routine Description:

    This function searches the currently active static function tables for an
    entry that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

    InImage - Address to recieve a flag indicating whether the ControlPc
        was in the range of a static function table

Return Value:

    If there is no entry in the static function tables for the specified PC,
    then NULL is returned. Otherwise, the address of the function table entry
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

#if DBG
    if (RtlDebugFlags & RTL_DBG_FUNCTION_ENTRY) {
        DbgPrint("RtlLookupStaticFunctionEntry(ControlPc = %p) ImageBase = %p\n",
                 ControlPc, ImageBase);
    }
#endif

    //
    // If an image is found that includes the specified PC, then locate the
    // function table for the image.
    //

    *InImage = (ImageBase != NULL);
    FunctionEntry = NULL;
    if (ImageBase != NULL) {
        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                         ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                         &SizeOfExceptionTable);
#if DBG
        if (RtlDebugFlags & RTL_DBG_FUNCTION_ENTRY_DETAIL) {
            DbgPrint("RtlLookupStaticFunctionEntry: FunctionTable = %p, SizeOfExceptionTable = %lx\n",
                     FunctionTable, SizeOfExceptionTable);
        }
#endif

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
                if (ControlPc < RF_BEGIN_ADDRESS(FunctionEntry)) {
                    High = Middle - 1;

                } else if (ControlPc >= RF_END_ADDRESS(FunctionEntry)) {
                    Low = Middle + 1;

                } else {
                    return FunctionEntry;
                }
            } // while (High >= Low)
        } // FunctionTable != NULL
    } // ImageBase != NULL
    return NULL;
}

VOID
RtlGetUnwindFunctionEntry(
    IN ULONG_PTR ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    OUT PRUNTIME_FUNCTION UnwindFunctionEntry,
    OUT PULONG StackAdjust,
    OUT PULONG_PTR FixedReturn
    )
/*++

Routine Description:

    This function returns a function entry (RUNTIME_FUNCTION) suitable
    for unwinding from ControlPc. It encapsulates the handling of primary
    and secondary function entries so that this processing is not duplicated
    in RtlVirtualUnwind and other similar functions.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    UnwindFunctionEntry - Supplies the address of a function table entry which
        will be setup with appropriate fields for unwinding from ControlPc

    StackAdjust - Receives the optional stack adjustment amount specified
        in RF_NULL_CONTEXT type secondary function entries. Will be zero
        if no null-context stack adjustment is required.
        
    FixedReturn - Receives the return address specified by fixed-return functin
        entries. Will be zero if a fixed return address was not specified.

Return Value:

    None.

--*/

{
    ULONG EntryType = 0;
    PRUNTIME_FUNCTION SecondaryFunctionEntry = NULL;
    ULONG_PTR AlternateProlog;

    *FixedReturn = 0;
    *StackAdjust = 0;
    
    // FunctionEntry should never be null, but if it is create one that
    // looks like a leaf entry for ControlPc

    if (FunctionEntry == NULL) {
#if DBG
        DbgPrint("\n****** Warning - Null function table entry for unwinding.\n");
#endif
        UnwindFunctionEntry->BeginAddress     = ControlPc;
        UnwindFunctionEntry->EndAddress       = ControlPc+4;
        UnwindFunctionEntry->ExceptionHandler = NULL;
        UnwindFunctionEntry->HandlerData      = 0;
        UnwindFunctionEntry->PrologEndAddress = ControlPc;
        return;
    }

    //
    // Because of the secondary-to-primary function entry indirection applied by
    // RtlLookupFunctionEntry() ControlPc may not be within the range described
    // by the supplied function entry. Call RtlLookupDirectFunctionEntry()
    // to recover the actual (secondary) function entry.  If we don't get a
    // valid associated function entry then process the unwind with the one
    // supplied, trusting that the caller has supplied the given entry intentionally.
    //
    // A secondary function entry is a RUNTIME_FUNCTION entry where
    // PrologEndAddress is not in the range of BeginAddress to EndAddress.
    // There are three types of secondary function entries. They are
    // distinquished by the Entry Type field (2 bits):
    //
    // RF_NOT_CONTIGUOUS - discontiguous code
    // RF_ALT_ENT_PROLOG - alternate entry point prologue
    // RF_NULL_CONTEXT   - null-context code
    //

    if ((ControlPc <  RF_BEGIN_ADDRESS(FunctionEntry)) ||
        (ControlPc >= RF_END_ADDRESS(FunctionEntry))) {

        // ControlPC is not in the range of the supplied function entry.
        // Get the actual function entry which is expected to be the
        // associated secondary function entry.

#if DBG
        if (RtlDebugFlags & RTL_DBG_FUNCTION_ENTRY) {
            DbgPrint("\nGetUnwindFunctionEntry:RtlLookupDirectFunctionEntry(ControlPc=%lx)\n", ControlPc);
        }
#endif
        SecondaryFunctionEntry = RtlLookupDirectFunctionEntry( ControlPc );

        if (SecondaryFunctionEntry) {
            
#if DBG
            ShowRuntimeFunction(SecondaryFunctionEntry, "GetUnwindFunctionEntry: LookupDirectFunctionEntry");
#endif
            
            // If this is a null-context tail region then unwind with a null-context-like descriptor

            if ((ControlPc >= RF_END_ADDRESS(SecondaryFunctionEntry)-(RF_NULL_CONTEXT_COUNT(SecondaryFunctionEntry)*4)) &&
                (ControlPc <  RF_END_ADDRESS(SecondaryFunctionEntry))) {

                // Use the secondary function entry with PrologEndAddress = BeginAddress.
                // This ensures that the prologue is not reverse executed.

                UnwindFunctionEntry->BeginAddress     = RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
                UnwindFunctionEntry->EndAddress       = RF_END_ADDRESS(SecondaryFunctionEntry);
                UnwindFunctionEntry->ExceptionHandler = 0;
                UnwindFunctionEntry->HandlerData      = 0;
                UnwindFunctionEntry->PrologEndAddress = RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
                return;
            }
            
            if ((SecondaryFunctionEntry->PrologEndAddress < RF_BEGIN_ADDRESS(SecondaryFunctionEntry)) ||
                (SecondaryFunctionEntry->PrologEndAddress > RF_END_ADDRESS(SecondaryFunctionEntry))) {
                
                // Got a secondary function entry as expected. But if indirection doesn't point
                // to FunctionEntry then ignore it and use the caller supplied FunctionEntry.

                if (RF_PROLOG_END_ADDRESS(SecondaryFunctionEntry) != (ULONG_PTR)FunctionEntry) {
#if DBG
                    DbgPrint("RtlGetUnwindFunctionEntry: unexpected secondary function entry from RtlLookupDirectFunctionEntry\n");
#endif
                    SecondaryFunctionEntry = NULL;
                }
            } else {

                // Got a primary function entry. The only valid type is a
                // Fixed Return Function Entry, which if present gets processed
                // at the end.  Even if it is not a fixed return function entry,
                // use it, since its prolog matches up with the control PC.

                FunctionEntry = SecondaryFunctionEntry;
                SecondaryFunctionEntry = NULL;

#if DBG
                if (!RF_FIXED_RETURN(FunctionEntry)) {
                DbgPrint("RtlGetUnwindFunctionEntry: unexpected primary function entry from RtlLookupDirectFunctionEntry\n");
                }
#endif
            }
#if DBG
        } else {
            DbgPrint("GetUnwindFunctionEntry: LookupDirectFunctionEntry returned NULL\n");
#endif
        }
        
    } else {

        // ControlPC is in the range of the supplied function entry.
        // Check if it is a secondary function entry. If so, get the
        // associated primary function entry.

        // If this is a null-context tail region then unwind with a null-context-like descriptor

        if ((ControlPc >= RF_END_ADDRESS(FunctionEntry)-(RF_NULL_CONTEXT_COUNT(FunctionEntry)*4)) &&
            (ControlPc <  RF_END_ADDRESS(FunctionEntry))) {
            
            // Create the unwind function entry with PrologEndAddress = BeginAddress.
            // This ensures that the prologue is not reverse executed.

            UnwindFunctionEntry->BeginAddress     = RF_BEGIN_ADDRESS(FunctionEntry);
            UnwindFunctionEntry->EndAddress       = RF_END_ADDRESS(FunctionEntry);
            UnwindFunctionEntry->ExceptionHandler = 0;
            UnwindFunctionEntry->HandlerData      = 0;
            UnwindFunctionEntry->PrologEndAddress = RF_BEGIN_ADDRESS(FunctionEntry);
            return;
        }
        
        if ((FunctionEntry->PrologEndAddress < RF_BEGIN_ADDRESS(FunctionEntry)) ||
            (FunctionEntry->PrologEndAddress > RF_END_ADDRESS(FunctionEntry))) {

            SecondaryFunctionEntry = FunctionEntry;
            FunctionEntry = (PRUNTIME_FUNCTION)RF_PROLOG_END_ADDRESS(SecondaryFunctionEntry);
#if DBG
            DbgPrint("RtlGetUnwindFunctionEntry: received secondary function entry\n");
#endif
        }
    }

#if DBG
    if (ControlPc & 0x3) {
        DbgPrint("RtlGetUnwindFunctionEntry: Warning - Invalid ControlPc %lx for unwinding.\n", ControlPc);
    } else if (RF_BEGIN_ADDRESS(FunctionEntry) >= RF_END_ADDRESS(FunctionEntry)) {
        ShowRuntimeFunction(FunctionEntry, "RtlGetUnwindFunctionEntry: Warning - BeginAddress < EndAddress.");
    } else if (FunctionEntry->PrologEndAddress < RF_BEGIN_ADDRESS(FunctionEntry)) {
        ShowRuntimeFunction(FunctionEntry, "RtlGetUnwindFunctionEntry: Warning - PrologEndAddress < BeginAddress.");
    } else if (FunctionEntry->PrologEndAddress > RF_END_ADDRESS(FunctionEntry)) {
        ShowRuntimeFunction(FunctionEntry, "RtlGetUnwindFunctionEntry: Warning - PrologEndAddress > EndAddress.");
    }
#endif

    // FunctionEntry is now the primary function entry and if SecondaryFunctionEntry is
    // not NULL then it is the secondary function entry that contains the ControlPC. Setup a
    // copy of the FunctionEntry suitable for unwinding. By default use the supplied FunctionEntry.

    if (SecondaryFunctionEntry) {

        // Extract the secondary function entry type.

        EntryType = RF_ENTRY_TYPE(SecondaryFunctionEntry);

        if (EntryType == RF_NOT_CONTIGUOUS) {
            // The exception happened in the body of the procedure but in a non-contiguous
            // section of code. Regardless of what entry point was used, it is normally valid
            // to unwind using the primary entry point prologue. The only exception is when an
            // alternate prologue is specified However, there may be an
            // alternate prologue end addresss specified in which case unwind using this
            // block as though it were the primary.
    
            AlternateProlog = RF_ALT_PROLOG(SecondaryFunctionEntry);
    
            if ((AlternateProlog >= RF_BEGIN_ADDRESS(SecondaryFunctionEntry)) &&
                (AlternateProlog <  RF_END_ADDRESS(SecondaryFunctionEntry))) {

                // If the control PC is in the alternate prologue, use the secondary.
                // The control Pc is not in procedure context.

                if ((ControlPc >= RF_BEGIN_ADDRESS(SecondaryFunctionEntry)) &&
                    (ControlPc <  AlternateProlog)) {

                    UnwindFunctionEntry->BeginAddress     = RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
                    UnwindFunctionEntry->EndAddress       = RF_END_ADDRESS(SecondaryFunctionEntry);
                    UnwindFunctionEntry->ExceptionHandler = 0;
                    UnwindFunctionEntry->HandlerData      = 0;
                    UnwindFunctionEntry->PrologEndAddress = AlternateProlog;
                    return;
                }
            }

            // Fall out of the if statement to pick up the primary function entry below.
            // This code is in-procedure-context and subject to the primary's prologue
            // and exception handlers.
        
        
        } else if (EntryType == RF_ALT_ENT_PROLOG) {
            
            // Exception occured in an alternate entry point prologue.
            // Use the secondary function entry with a fixed-up PrologEndAddress.

            UnwindFunctionEntry->BeginAddress     = RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->EndAddress       = RF_END_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->ExceptionHandler = 0;
            UnwindFunctionEntry->HandlerData      = 0;
            UnwindFunctionEntry->PrologEndAddress = RF_END_ADDRESS(UnwindFunctionEntry);
            
            // Check for an alternate prologue.
            
            AlternateProlog = RF_ALT_PROLOG(SecondaryFunctionEntry);
            if (AlternateProlog >= UnwindFunctionEntry->BeginAddress &&
                AlternateProlog <  UnwindFunctionEntry->EndAddress ) {
                // The prologue is only part of the procedure
                UnwindFunctionEntry->PrologEndAddress = AlternateProlog;
            }

            return;
        
        } else if (EntryType == RF_NULL_CONTEXT) {

            // Exception occured in null-context code associated with a primary function.
            // Use the secondary function entry with a PrologEndAddress = BeginAddress.
            // There is no prologue for null-context code.

            *StackAdjust = RF_STACK_ADJUST(SecondaryFunctionEntry);
            UnwindFunctionEntry->BeginAddress     = RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->EndAddress       = RF_END_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->ExceptionHandler = 0;
            UnwindFunctionEntry->HandlerData      = 0;
            UnwindFunctionEntry->PrologEndAddress = RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
            return;
        }

    }
    
    // Use the primary function entry
    
    *UnwindFunctionEntry = *FunctionEntry;
    UnwindFunctionEntry->EndAddress = RF_END_ADDRESS(UnwindFunctionEntry);  // Remove null-context count
    
    // If the primary has a fixed return address, pull that out now.

    if (RF_IS_FIXED_RETURN(FunctionEntry)) {
        *FixedReturn = RF_FIXED_RETURN(FunctionEntry);
        UnwindFunctionEntry->ExceptionHandler = 0;
        UnwindFunctionEntry->HandlerData      = 0;
    }

    // If the ControlPc is in the primary Null context, return null context.
    // Otherwise, remove Null context count

    if ((ControlPc >= RF_END_ADDRESS(FunctionEntry)-(RF_NULL_CONTEXT_COUNT(FunctionEntry)*4)) &&
        (ControlPc <  RF_END_ADDRESS(FunctionEntry))) {

        // Exception occured in null-context code associated with a primary function.
        // Create the unwind function entry with a PrologEndAddress = BeginAddress.
        // This ensures that the prologue is not reverse executed.

        UnwindFunctionEntry->EndAddress       = RF_END_ADDRESS(FunctionEntry);
        UnwindFunctionEntry->ExceptionHandler = 0;
        UnwindFunctionEntry->HandlerData      = 0;
        UnwindFunctionEntry->PrologEndAddress = RF_BEGIN_ADDRESS(FunctionEntry);
    }
    else {
        UnwindFunctionEntry->EndAddress       = RF_END_ADDRESS(FunctionEntry);
    }
}
#if !defined(NTOS_KERNEL_RUNTIME)

PLIST_ENTRY
RtlGetFunctionTableListHead (
    VOID
    )

/*++

Routine Description:

    Return the address of the dynamic function table list head.

Return value:

    Address of dynamic function table list head.

--*/
{
    return &DynamicFunctionTable;
}

BOOLEAN
RtlAddFunctionTable(
    IN PRUNTIME_FUNCTION FunctionTable,
    IN ULONG EntryCount
    )

/*++

Routine Description:

    Add a dynamic function table to the dynamic function table list. Dynamic
    function tables describe code generated at run-time. The dynamic function
    tables are searched via a call to RtlLookupDynamicFunctionEntry().
    Normally this is only invoked via calls to RtlLookupFunctionEntry().

    The FunctionTable entries need not be sorted in any particular order. The
    list is scanned for a Min and Max address range and whether or not it is
    sorted. If the latter RtlLookupDynamicFunctionEntry() uses a binary
    search, otherwise it uses a linear search.

    The dynamic function entries will be searched only after a search
    through the static function entries associated with all current
    process images has failed.

Arguments:

   FunctionTable       Address of an array of function entries where
                       each element is of type RUNTIME_FUNCTION.

   EntryCount          The number of function entries in the array

Return value:

   TRUE                if RtlAddFunctionTable completed successfully
   FALSE               if RtlAddFunctionTable completed unsuccessfully

--*/
{
    PDYNAMIC_FUNCTION_TABLE pNew;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG i;

    if (EntryCount == 0)
        return FALSE;

    //
    // Make sure the link list is initialized;
    //

    if (DynamicFunctionTable.Flink == NULL) {
       InitializeListHead(&DynamicFunctionTable);
    }

    //
    //  Allocate memory for this link list entry
    //

    pNew = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(DYNAMIC_FUNCTION_TABLE) );

    if (pNew != NULL) {
        pNew->FunctionTable = FunctionTable;
        pNew->EntryCount = EntryCount;
        NtQuerySystemTime( &pNew->TimeStamp );

        //
        // Scan the function table for Minimum/Maximum and to determine
        // if it is sorted. If the latter, we can perform a binary search.
        //

        FunctionEntry = FunctionTable;
        pNew->MinimumAddress = RF_BEGIN_ADDRESS(FunctionEntry);
        pNew->MaximumAddress = RF_END_ADDRESS(FunctionEntry);
        pNew->Sorted = TRUE;
        FunctionEntry++;

        for (i = 1; i < EntryCount; FunctionEntry++, i++) {
            if (pNew->Sorted && FunctionEntry->BeginAddress < FunctionTable[i-1].BeginAddress) {
                pNew->Sorted = FALSE;
            }
            if (FunctionEntry->BeginAddress < pNew->MinimumAddress) {
                pNew->MinimumAddress = RF_BEGIN_ADDRESS(FunctionEntry);
            }
            if (FunctionEntry->EndAddress > pNew->MaximumAddress) {
                pNew->MaximumAddress = RF_END_ADDRESS(FunctionEntry);
            }
        }

        //
        // Insert the new entry in the dynamic function table list.
        // Protect the insertion with the loader lock.
        //

        RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        InsertTailList((PLIST_ENTRY)&DynamicFunctionTable, (PLIST_ENTRY)pNew);
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

        return TRUE;
    } else {
        return FALSE;
    }
}

BOOLEAN
RtlDeleteFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable
    )
{

/*++

Routine Description:

    Remove a dynamic function table from the dynamic function table list.

Arguments:

   FunctionTable       Address of an array of function entries that
                       was passed in a previous call to RtlAddFunctionTable

Return Value

    TRUE - If function completed successfully
    FALSE - If function completed unsuccessfully

--*/

    PDYNAMIC_FUNCTION_TABLE CurrentEntry;
    PLIST_ENTRY Head;
    PLIST_ENTRY Next;
    BOOLEAN Status = FALSE;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    //
    // Search the dynamic function table list for a match on the the function
    // table address.
    //

    Head = &DynamicFunctionTable;
    for (Next = Head->Blink; Next != Head; Next = Next->Blink) {
        CurrentEntry = CONTAINING_RECORD(Next,DYNAMIC_FUNCTION_TABLE,Links);
        if (CurrentEntry->FunctionTable == FunctionTable) {
            RemoveEntryList((PLIST_ENTRY)CurrentEntry);
            RtlFreeHeap( RtlProcessHeap(), 0, CurrentEntry );
            Status = TRUE;
            break;
        }
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return Status;
}

PRUNTIME_FUNCTION
RtlLookupDynamicFunctionEntry(
    IN ULONG_PTR ControlPc
    )

/*++

Routine Description:

  This function searches through the dynamic function entry
  tables and returns the function entry address that corresponds
  to the specified ControlPc. This routine does NOT perform the
  secondary function entry indirection. That is performed
  by RtlLookupFunctionEntry().

  Argument:

     ControlPc           Supplies a ControlPc.

  Return Value

     NULL - No function entry found that contains the ControlPc.

     NON-NULL - Address of the function entry that describes the
                code containing ControlPC.

--*/

{
    PDYNAMIC_FUNCTION_TABLE CurrentEntry;
    PLIST_ENTRY Next,Head;
    PRUNTIME_FUNCTION FunctionTable;
    PRUNTIME_FUNCTION FunctionEntry;
    LONG High;
    LONG Low;
    LONG Middle;

    if (DynamicFunctionTable.Flink == NULL)
        return NULL;

    if (RtlTryEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock) ) {

        //
        //  Search the tree starting from the head, continue until the entry
        //  is found or we reach the end of the list.
        //

        Head = &DynamicFunctionTable;
        for (Next = Head->Blink; Next != Head; Next = Next->Blink) {
            CurrentEntry = CONTAINING_RECORD(Next,DYNAMIC_FUNCTION_TABLE,Links);
            FunctionTable = CurrentEntry->FunctionTable;

            //
            // Check if the ControlPC is within the range of this function table
            //

            if ((ControlPc >= CurrentEntry->MinimumAddress) &&
                (ControlPc <  CurrentEntry->MaximumAddress) ) {

                // If this function table is sorted do a binary search.

                if (CurrentEntry->Sorted) {

                    //
                    // Perform binary search on the function table for a function table
                    // entry that subsumes the specified PC.
                    //

                    Low = 0;
                    High = CurrentEntry->EntryCount -1 ;
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
                        if (ControlPc < RF_BEGIN_ADDRESS(FunctionEntry)) {
                            High = Middle - 1;

                        } else if (ControlPc >= RF_END_ADDRESS(FunctionEntry)) {
                            Low = Middle + 1;

                        } else {
#if DBG
                            ShowRuntimeFunction(FunctionEntry, "RtlLookupDynamicFunctionEntry: binary search" );
#endif
                            RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                            return FunctionEntry;
                        }
                    }

                } else {    // Not sorted. Do linear search.

                    PRUNTIME_FUNCTION LastFunctionEntry = &FunctionTable[CurrentEntry->EntryCount];

                    for (FunctionEntry = FunctionTable; FunctionEntry < LastFunctionEntry; FunctionEntry++) {
                        if ((ControlPc >= RF_BEGIN_ADDRESS(FunctionEntry)) &&
                            (ControlPc <  RF_END_ADDRESS(FunctionEntry))) {
#if DBG
                            ShowRuntimeFunction(FunctionEntry, "RtlLookupDynamicFunctionEntry: linear search" );
#endif
                            RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                            return FunctionEntry;
                        }
                    }
                } // binary/linear search
            } // if in range
        } // for (... Next != Head ...)

        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    } // LoaderLock

    
    return NULL;
}
#endif
#if DBG

void
ShowRuntimeFunction( PRUNTIME_FUNCTION FunctionEntry, PSTR Label )
{
    BOOLEAN Secondary;
    BOOLEAN FixedReturn;
    ULONG EntryType;
    ULONG NullCount;
    
    if (!(RtlDebugFlags & RTL_DBG_FUNCTION_ENTRY_DETAIL)) {
        return;
    }
    
    if (FunctionEntry) {
        Secondary = FALSE;
        FixedReturn = FALSE;
        EntryType = 0;
        NullCount = 0;
        
        DbgPrint("    %lx: %s\n", FunctionEntry, Label );
        
        if ((RF_PROLOG_END_ADDRESS(FunctionEntry) < RF_BEGIN_ADDRESS(FunctionEntry)) ||
            (RF_PROLOG_END_ADDRESS(FunctionEntry) > RF_END_ADDRESS(FunctionEntry))) {
            Secondary = TRUE;
            EntryType = RF_ENTRY_TYPE(FunctionEntry);
        } else if (RF_IS_FIXED_RETURN(FunctionEntry)) {
            FixedReturn = TRUE;
        }
        NullCount = RF_NULL_CONTEXT_COUNT(FunctionEntry);
    
        DbgPrint("    BeginAddress     = %lx\n", FunctionEntry->BeginAddress);
        DbgPrint("    EndAddress       = %lx", FunctionEntry->EndAddress);
        if (NullCount) {
            DbgPrint(" %d null-context instructions", NullCount);
        }
        DbgPrint("\n");
        DbgPrint("    ExceptionHandler = %lx", FunctionEntry->ExceptionHandler);
        if (FunctionEntry->ExceptionHandler != NULL) {
            if (Secondary) {
                ULONG_PTR AlternateProlog = RF_ALT_PROLOG(FunctionEntry);
    
                switch( EntryType ) {
                case RF_NOT_CONTIGUOUS:
                case RF_ALT_ENT_PROLOG:
                    
                    if ((AlternateProlog >= RF_BEGIN_ADDRESS(FunctionEntry)) &&
                        (AlternateProlog <= RF_END_ADDRESS(FunctionEntry))) {
                            DbgPrint(" alternate PrologEndAddress");
                    }
                    break;
                case RF_NULL_CONTEXT:
                    DbgPrint(" stack adjustment");
                default:
                    DbgPrint(" invalid entry type");
                }
            } else if (FixedReturn) {
                DbgPrint(" fixed return address");
            }
        }
        DbgPrint("\n");
        DbgPrint("    HandlerData      = %lx", FunctionEntry->HandlerData);
        if (Secondary) {
            DbgPrint(" type %d: ", EntryType);
            if      (EntryType == RF_NOT_CONTIGUOUS) DbgPrint("RF_NOT_CONTIGUOUS");
            else if (EntryType == RF_ALT_ENT_PROLOG) DbgPrint("RF_ALT_ENT_PROLOG");
            else if (EntryType == RF_NULL_CONTEXT)   DbgPrint("RF_NULL_CONTEXT");
            else DbgPrint("***INVALID***");
        }
        DbgPrint("\n");
        DbgPrint("    PrologEndAddress = %lx\n",   FunctionEntry->PrologEndAddress );
    }
}
#endif

