/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    unwindr.c

Abstract:

    This module implements two alternate versions of the unwind function
    required by Alpha AXP during the GEM compiler transition period. The
    code is adapted from RtlUnwind (exdsptch.c). These functions can be
    deleted if GEM uses scope table based structured exception handling
    and can materialize virtual frame pointers.

Author:

    Thomas Van Baak (tvb) 18-Nov-1992

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

ULONG RtlDebugFlags;

#endif

#define Virtual VirtualFramePointer
#define Real RealFramePointer

VOID
RtlUnwindRfp (
    IN PVOID TargetRealFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetRealFrame parameter is not specified, then the exit
    unwind flag is also set in the exception flags of the exception record.
    A backward scan through the procedure call frames is then performed to
    find the target of the unwind operation.

    As each frame is encountered, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

    This function is identical to RtlUnwind except that the TargetRealFrame
    parameter is the real frame pointer instead of the virtual frame pointer.

Arguments:

    TargetRealFrame - Supplies an optional pointer to the call frame that is
        the target of the unwind. If this parameter is not specified, then an
        exit unwind is performed.

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

    CONTEXT ContextRecord1;
    CONTEXT ContextRecord2;
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
    ULONG_PTR LastPc;
    ULONG_PTR LowLimit;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("\nRtlUnwindRfp(TargetRealFrame = %p, TargetIp = %p,, ReturnValue = %lx)\n",
                 TargetRealFrame, TargetIp, ReturnValue);
    }
#endif

    //
    // Get current stack limits, capture the current context, virtually
    // unwind to the caller of this routine, get the initial PC value, and
    // set the unwind target address.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlCaptureContext(&ContextRecord1);
    ControlPc = (ULONG_PTR)ContextRecord1.IntRa;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    LastPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              &ContextRecord1,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ControlPc = LastPc;
    ContextRecord1.Fir = (ULONGLONG)(LONG_PTR)TargetIp;

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
    if (ARGUMENT_PRESENT(TargetRealFrame) == FALSE) {
        ExceptionRecord->ExceptionFlags |= EXCEPTION_EXIT_UNWIND;
    }

    //
    // Scan backward through the call frame hierarchy and call exception
    // handlers until the target frame of the unwind is reached.
    //

    do {

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwindRfp: Loop: FrameDepth = %d, Rfp = %p, sp = %p, ControlPc = %p\n",
                 FrameDepth, EstablisherFrame.Real, (ULONG_PTR)ContextRecord1.IntSp, ControlPc);
        FrameDepth -= 1;
    }
#endif

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc);

        //
        // If there is a function table entry for the routine, then copy the
        // context record, virtually unwind to the caller of the current
        // routine to obtain the real frame pointer of the establisher and
        // check if there is an exception handler for the frame.
        //

        if (FunctionEntry != NULL) {
            RtlMoveMemory(&ContextRecord2, &ContextRecord1, sizeof(CONTEXT));
            LastPc = RtlVirtualUnwind(ControlPc,
                                      FunctionEntry,
                                      &ContextRecord1,
                                      &InFunction,
                                      &EstablisherFrame,
                                      NULL);

            //
            // If the real and virtual frame pointers are not within the
            // specified stack limits, the frame pointers are unaligned, or
            // the target frame is below the real frame and an exit unwind is
            // not being performed, then raise the exception STATUS_BAD_STACK.
            // Otherwise, check to determine if the current routine has an
            // exception handler.
            //

            if ((EstablisherFrame.Real < LowLimit) ||
                (EstablisherFrame.Virtual > HighLimit) ||
                (EstablisherFrame.Real > EstablisherFrame.Virtual) ||
                ((ARGUMENT_PRESENT(TargetRealFrame) != FALSE) &&
                 ((ULONG_PTR)TargetRealFrame < EstablisherFrame.Real)) ||
                ((EstablisherFrame.Virtual & 0xF) != 0) ||
                ((EstablisherFrame.Real & 0xF) != 0)) {

#if DBG
                DbgPrint("\n****** Warning - bad stack or target frame (unwind).\n");
                DbgPrint("  EstablisherFrame Virtual = %p, Real = %p\n",
                         EstablisherFrame.Virtual, EstablisherFrame.Real);
                DbgPrint("  TargetRealFrame = %p\n", TargetRealFrame);
                if ((ARGUMENT_PRESENT(TargetRealFrame) != FALSE) &&
                    ((ULONG_PTR)TargetRealFrame < EstablisherFrame.Real)) {
                    DbgPrint("  TargetRealFrame is below EstablisherFrame.Real!\n");
                }
                DbgPrint("  Previous EstablisherFrame (sp) = %p\n",
                         (ULONG_PTR)ContextRecord2.IntSp);
                DbgPrint("  LowLimit = %p, HighLimit = %p\n",
                         LowLimit, HighLimit);
                DbgPrint("  LastPc = %p, ControlPc = %p\n",
                         LastPc, ControlPc);
                DbgPrint("  Now raising STATUS_BAD_STACK exception.\n");
#endif

                RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);

            } else if (IS_HANDLER_DEFINED(FunctionEntry) && InFunction) {

#if DBG
    if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION_DETAIL) {
    DbgPrint("RtlUnwindRfp: ExceptionHandler = %p, HandlerData = %p\n",
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
                DispatcherContext.ContextRecord = &ContextRecord2;

                //
                // Call the exception handler.
                //

                do {

                    //
                    // If the establisher frame is the target of the unwind
                    // operation, then set the target unwind flag.
                    //

                    if ((ULONG_PTR)TargetRealFrame == EstablisherFrame.Real) {
                        ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
                    }

                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    //
                    // Set the specified return value in case the exception
                    // handler directly continues execution.
                    //

                    ContextRecord2.IntV0 = (ULONGLONG)(LONG_PTR)ReturnValue;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwindRfp: calling RtlpExecuteHandlerForUnwind, ControlPc = %p\n", ControlPc);
    }
#endif

                    Disposition =
                        RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                    EstablisherFrame.Virtual,
                                                    &ContextRecord2,
                                                    &DispatcherContext,
                                                    RF_EXCEPTION_HANDLER(FunctionEntry));

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwindRfp: RtlpExecuteHandlerForUnwind returned Disposition = %lx\n", Disposition);
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
                        // Continue the search for a handler or continue
                        // execution.
                        //

                    case ExceptionContinueSearch :
                        break;

                        //
                        // The disposition is collided unwind.
                        //
                        // Set the target of the current unwind to the context
                        // record of the previous unwind, virtually unwind to
                        // the caller of the old routine, and reexecute the
                        // exception handler from the collided frame with the
                        // collided unwind flag set in the exception record.
                        //

                    case ExceptionCollidedUnwind :
                        ControlPc = DispatcherContext.ControlPc;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        RtlMoveMemory(&ContextRecord1,
                                      DispatcherContext.ContextRecord,
                                      sizeof(CONTEXT));

                        ContextRecord1.Fir = (ULONGLONG)(LONG_PTR)TargetIp;
                        RtlMoveMemory(&ContextRecord2,
                                      &ContextRecord1,
                                      sizeof(CONTEXT));

                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        LastPc = RtlVirtualUnwind(ControlPc,
                                                  FunctionEntry,
                                                  &ContextRecord1,
                                                  &InFunction,
                                                  &EstablisherFrame,
                                                  NULL);
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
            }

        } else {

            //
            // Set point at which control left the previous routine.
            //

            LastPc = (ULONG_PTR)ContextRecord1.IntRa - 4;

            //
            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            //

            if (LastPc == ControlPc) {

#if DBG
                ULONG Count;
                DbgPrint("\n****** Warning - malformed function table (unwind).\n");
                DbgPrint("ControlPc = %p, %p", LastPc, ControlPc);
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

        ControlPc = LastPc;

    } while ((EstablisherFrame.Real < HighLimit) &&
             (EstablisherFrame.Real != (ULONG_PTR)TargetRealFrame));

    //
    // If the establisher stack pointer is equal to the target frame
    // pointer, then continue execution. Otherwise, an exit unwind was
    // performed or the target of the unwind did not exist and the
    // debugger and subsystem are given a second chance to handle the
    // unwind.
    //

    if (EstablisherFrame.Real == (ULONG_PTR)TargetRealFrame) {
        ContextRecord2.IntV0 = (ULONGLONG)(LONG_PTR)ReturnValue;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("RtlUnwindRfp: finished unwinding, and calling RtlpRestoreContext\n");
    }
#endif

        RtlpRestoreContext(&ContextRecord2);

    } else {

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("RtlUnwindRfp: finished unwinding, but calling ZwRaiseException\n");
    }
#endif

        ZwRaiseException(ExceptionRecord, &ContextRecord1, FALSE);
    }
}

VOID
RtlUnwindReturn (
    IN PVOID TargetFrame,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. A backward scan through the procedure call frames is then
    performed to find the target of the unwind operation. When the target
    frame is reached, a return is made to the caller of the target frame
    with the return value specified by the return value parameter.

    As each frame is encountered, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

    This function is identical to RtlUnwind except control resumes in the
    caller of the target frame, not in the target frame itself.

Arguments:

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord1;
    CONTEXT ContextRecord2;
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
    ULONG_PTR LastPc;
    ULONG_PTR LowLimit;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("\nRtlUnwindReturn(TargetFrame = %p,, ReturnValue = %lx)\n",
                 TargetFrame, ReturnValue);
    }
#endif

    //
    // Get current stack limits, capture the current context, virtually
    // unwind to the caller of this routine, get the initial PC value, and
    // set the unwind target address.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlCaptureContext(&ContextRecord1);
    ControlPc = (ULONG_PTR)ContextRecord1.IntRa;
    FunctionEntry = RtlLookupFunctionEntry(ControlPc);
    LastPc = RtlVirtualUnwind(ControlPc,
                              FunctionEntry,
                              &ContextRecord1,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ControlPc = LastPc;
    ContextRecord1.Fir = 0;

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
    // A target frame of the unwind is specified so a normal unwind is
    // being performed.
    //

    ExceptionFlags = EXCEPTION_UNWINDING;

    //
    // Scan backward through the call frame hierarchy and call exception
    // handlers until the target frame of the unwind is reached.
    //

    do {

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwindReturn: Loop: FrameDepth = %d, sp = %p, ControlPc = %p\n",
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
        // If there is a function table entry for the routine, then copy the
        // context record, virtually unwind to the caller of the current
        // routine to obtain the virtual frame pointer of the establisher and
        // check if there is an exception handler for the frame.
        //

        if (FunctionEntry != NULL) {
            RtlMoveMemory(&ContextRecord2, &ContextRecord1, sizeof(CONTEXT));
            LastPc = RtlVirtualUnwind(ControlPc,
                                      FunctionEntry,
                                      &ContextRecord1,
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

            if ((EstablisherFrame.Virtual < LowLimit) ||
                (EstablisherFrame.Virtual > HighLimit) ||
                ((ULONG_PTR)TargetFrame < EstablisherFrame.Virtual) ||
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
                         (ULONG_PTR)ContextRecord2.IntSp);
                DbgPrint("  LowLimit = %p, HighLimit = %p\n",
                         LowLimit, HighLimit);
                DbgPrint("  LastPc = %p, ControlPc = %p\n",
                         LastPc, ControlPc);
                DbgPrint("  Now raising STATUS_BAD_STACK exception.\n");
#endif

                RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);

            } else if (IS_HANDLER_DEFINED(FunctionEntry) && InFunction) {

#if DBG
    if (RtlDebugFlags & RTL_DBG_DISPATCH_EXCEPTION_DETAIL) {
    DbgPrint("RtlUnwindReturn: ExceptionHandler = %p, HandlerData = %p\n",
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
                DispatcherContext.ContextRecord = &ContextRecord2;

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

                    ContextRecord2.IntV0 = (ULONGLONG)(LONG_PTR)ReturnValue;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwindReturn: calling RtlpExecuteHandlerForUnwind, ControlPc = %p\n", ControlPc);
    }
#endif

                    Disposition =
                        RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                    EstablisherFrame.Virtual,
                                                    &ContextRecord2,
                                                    &DispatcherContext,
                                                    RF_EXCEPTION_HANDLER(FunctionEntry));

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND_DETAIL) {
        DbgPrint("RtlUnwindReturn: RtlpExecuteHandlerForUnwind returned Disposition = %lx\n", Disposition);
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
                        // Continue the search for a handler or continue
                        // execution.
                        //

                    case ExceptionContinueSearch :
                        break;

                        //
                        // The disposition is collided unwind.
                        //
                        // Set the target of the current unwind to the context
                        // record of the previous unwind, virtually unwind to
                        // the caller of the old routine, and reexecute the
                        // exception handler from the collided frame with the
                        // collided unwind flag set in the exception record.
                        //

                    case ExceptionCollidedUnwind :
                        ControlPc = DispatcherContext.ControlPc;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        RtlMoveMemory(&ContextRecord1,
                                      DispatcherContext.ContextRecord,
                                      sizeof(CONTEXT));

                        ContextRecord1.Fir = 0;
                        RtlMoveMemory(&ContextRecord2,
                                      &ContextRecord1,
                                      sizeof(CONTEXT));

                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        LastPc = RtlVirtualUnwind(ControlPc,
                                                  FunctionEntry,
                                                  &ContextRecord1,
                                                  &InFunction,
                                                  &EstablisherFrame,
                                                  NULL);
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
            }

        } else {

            //
            // Set point at which control left the previous routine.
            //

            LastPc = (ULONG_PTR)ContextRecord1.IntRa - 4;

            //
            // If the next control PC is the same as the old control PC, then
            // the function table is not correctly formed.
            //

            if (LastPc == ControlPc) {
#if DBG
                ULONG Count;
                DbgPrint("\n****** Warning - malformed function table (unwind).\n");
                DbgPrint("ControlPc = %p, %p", LastPc, ControlPc);
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

        ControlPc = LastPc;

    } while ((EstablisherFrame.Virtual < HighLimit) &&
             (EstablisherFrame.Virtual != (ULONG_PTR)TargetFrame));

    //
    // If the establisher stack pointer is equal to the target frame pointer,
    // then continue execution at the point where the call to the target frame
    // was made. Otherwise the target of the unwind did not exist and the
    // debugger and subsystem are given a second chance to handle the unwind.
    //

    if ((ULONG_PTR)ContextRecord1.IntSp == (ULONG_PTR)TargetFrame) {
        ContextRecord1.IntV0 = (ULONGLONG)(LONG_PTR)ReturnValue;

        //
        // Set the continuation address to the address after the point where
        // control left the previous frame and entered the target frame.
        //

        ContextRecord1.Fir = (ULONGLONG)(LONG_PTR)LastPc + 4;

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("RtlUnwindReturn: finished unwinding, and calling RtlpRestoreContext\n");
    }
#endif

        RtlpRestoreContext(&ContextRecord1);

    } else {

#if DBG
    if (RtlDebugFlags & RTL_DBG_UNWIND) {
        DbgPrint("RtlUnwindReturn: finished unwinding, but calling ZwRaiseException\n");
    }
#endif

        ZwRaiseException(ExceptionRecord, &ContextRecord1, FALSE);
    }
}
