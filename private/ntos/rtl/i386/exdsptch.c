/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exdsptch.c

Abstract:

    This module implements the dispatching of exception and the unwinding of
    procedure call frames.

Author:

    David N. Cutler (davec) 13-Aug-1989

Environment:

    Any mode.

Revision History:

    10 april 90 bryanwi

            Port to the 386.

--*/

#include "ntrtlp.h"


//
// Dispatcher context structure definition.
//

typedef struct _DISPATCHER_CONTEXT {
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    } DISPATCHER_CONTEXT;

//
// Execute handler for exception function prototype.
//

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PVOID DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );

//
// Execute handler for unwind function prototype.
//

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForUnwind (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PVOID DispatcherContext,
    IN PEXCEPTION_ROUTINE ExceptionRoutine
    );



BOOLEAN
RtlDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function attempts to dispatch an exception to a call frame based
    handler by searching backwards through the stack based call frames. The
    search begins with the frame specified in the context record and continues
    backward until either a handler is found that handles the exception, the
    stack is found to be invalid (i.e., out of limits or unaligned), or the end
    of the call hierarchy is reached.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    If the exception is handled by one of the frame based handlers, then
    a value of TRUE is returned. Otherwise a value of FALSE is returned.

--*/

{

    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    PEXCEPTION_REGISTRATION_RECORD NestedRegistration;
    ULONG HighAddress;
    ULONG HighLimit;
    ULONG LowLimit;
    EXCEPTION_RECORD ExceptionRecord1;

#if !defined(WX86_i386)

    ULONG Index;

#endif

    //
    // Get current stack limits.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);

    //
    // Start with the frame specified by the context record and search
    // backwards through the call frame hierarchy attempting to find an
    // exception handler that will handler the exception.
    //

    RegistrationPointer = RtlpGetRegistrationHead();
    NestedRegistration = 0;

    while (RegistrationPointer != EXCEPTION_CHAIN_END) {

        //
        // If the call frame is not within the specified stack limits or the
        // call frame is unaligned, then set the stack invalid flag in the
        // exception record and return FALSE. Else check to determine if the
        // frame has an exception handler.
        //

        HighAddress = (ULONG)RegistrationPointer +
            sizeof(EXCEPTION_REGISTRATION_RECORD);

        if ( ((ULONG)RegistrationPointer < LowLimit) ||
             (HighAddress > HighLimit) ||
             (((ULONG)RegistrationPointer & 0x3) != 0) ) {

#if defined(NTOS_KERNEL_RUNTIME)

            //
            // Allow for the possibility that the problem occured on the
            // DPC stack.
            //

            ULONG TestAddress = (ULONG)RegistrationPointer;

            if (((TestAddress & 0x3) == 0) &&
                KeGetCurrentIrql() >= DISPATCH_LEVEL) {

                PKPRCB Prcb = KeGetCurrentPrcb();
                ULONG DpcStack = (ULONG)Prcb->DpcStack;

                if ((Prcb->DpcRoutineActive) &&
                    (HighAddress <= DpcStack) &&
                    (TestAddress >= DpcStack - KERNEL_STACK_SIZE)) {

                    //
                    // This error occured on the DPC stack, switch
                    // stack limits to the DPC stack and restart 
                    // the loop.
                    // 

                    HighLimit = DpcStack;
                    LowLimit = DpcStack - KERNEL_STACK_SIZE;
                    continue;
                }
            }

#endif

            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            return FALSE;
        } 

#if !defined(WX86_i386)

        //
        // The handler must be executed by calling another routine
        // that is written in assembler. This is required because
        // up level addressing of the handler information is required
        // when a nested exception is encountered.
        //

        if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
            Index = RtlpLogExceptionHandler(
                            ExceptionRecord,
                            ContextRecord,
                            0,
                            (PULONG)RegistrationPointer,
                            4 * sizeof(ULONG));
                    // can't use sizeof(EXCEPTION_REGISTRATION_RECORD
                    // because we need the 2 dwords above it.
        }
#endif

        Disposition = RtlpExecuteHandlerForException(
            ExceptionRecord,
            (PVOID)RegistrationPointer,
            ContextRecord,
            (PVOID)&DispatcherContext,
            (PEXCEPTION_ROUTINE)RegistrationPointer->Handler);

#if !defined(WX86_i386)
        if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
            RtlpLogLastExceptionDisposition(Index, Disposition);
        }
#endif


        //
        // If the current scan is within a nested context and the frame
        // just examined is the end of the context region, then clear
        // the nested context frame and the nested exception in the
        // exception flags.
        //

        if (NestedRegistration == RegistrationPointer) {
            ExceptionRecord->ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
            NestedRegistration = 0;
        }

        //
        // Case on the handler disposition.
        //

        switch (Disposition) {

            //
            // The disposition is to continue execution. If the
            // exception is not continuable, then raise the exception
            // STATUS_NONCONTINUABLE_EXCEPTION. Otherwise return
            // TRUE.
            //

        case ExceptionContinueExecution :
            if ((ExceptionRecord->ExceptionFlags &
               EXCEPTION_NONCONTINUABLE) != 0) {
                ExceptionRecord1.ExceptionCode =
                                        STATUS_NONCONTINUABLE_EXCEPTION;
                ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord1.ExceptionRecord = ExceptionRecord;
                ExceptionRecord1.NumberParameters = 0;
                RtlRaiseException(&ExceptionRecord1);
            } else {
                return TRUE;
            }

            //
            // The disposition is to continue the search. Get next
            // frame address and continue the search.
            //

        case ExceptionContinueSearch :
            break;

            //
            // The disposition is nested exception. Set the nested
            // context frame to the establisher frame address and set
            // nested exception in the exception flags.
            //

        case ExceptionNestedException :
            ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;
            if (DispatcherContext.RegistrationPointer > NestedRegistration) {
                NestedRegistration = DispatcherContext.RegistrationPointer;
            }
            break;

            //
            // All other disposition values are invalid. Raise
            // invalid disposition exception.
            //

        default :
            ExceptionRecord1.ExceptionCode = STATUS_INVALID_DISPOSITION;
            ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord1.ExceptionRecord = ExceptionRecord;
            ExceptionRecord1.NumberParameters = 0;
            RtlRaiseException(&ExceptionRecord1);
            break;
        }

        //
        // If chain goes in wrong direction or loops, report an
        // invalid exception stack, otherwise go on to the next one.
        //

        RegistrationPointer = RegistrationPointer->Next;
    }
    return FALSE;
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
    walk through the procedure call frames is then performed to find the target
    of the unwind operation.

    N.B.    The captured context passed to unwinding handlers will not be
            a  completely accurate context set for the 386.  This is because
            there isn't a standard stack frame in which registers are stored.

            Only the integer registers are affected.  The segement and
            control registers (ebp, esp) will have correct values for
            the flat 32 bit environment.

    N.B.    If you change the number of arguments, make sure you change the
            adjustment of ESP after the call to RtlpCaptureContext (for
            STDCALL calling convention)

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
    PCONTEXT ContextRecord;
    CONTEXT ContextRecord1;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    PEXCEPTION_REGISTRATION_RECORD PriorPointer;
    ULONG HighAddress;
    ULONG HighLimit;
    ULONG LowLimit;
    EXCEPTION_RECORD ExceptionRecord1;
    EXCEPTION_RECORD ExceptionRecord2;

    //
    // Get current stack limits.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);

    //
    // If an exception record is not specified, then build a local exception
    // record for use in calling exception handlers during the unwind operation.
    //

    if (ARGUMENT_PRESENT(ExceptionRecord) == FALSE) {
        ExceptionRecord = &ExceptionRecord1;
        ExceptionRecord1.ExceptionCode = STATUS_UNWIND;
        ExceptionRecord1.ExceptionFlags = 0;
        ExceptionRecord1.ExceptionRecord = NULL;
        ExceptionRecord1.ExceptionAddress = RtlpGetReturnAddress();
        ExceptionRecord1.NumberParameters = 0;
    }

    //
    // If the target frame of the unwind is specified, then set EXCEPTION_UNWINDING
    // flag in the exception flags. Otherwise set both EXCEPTION_EXIT_UNWIND and
    // EXCEPTION_UNWINDING flags in the exception flags.
    //

    if (ARGUMENT_PRESENT(TargetFrame) == TRUE) {
        ExceptionRecord->ExceptionFlags |= EXCEPTION_UNWINDING;
    } else {
        ExceptionRecord->ExceptionFlags |= (EXCEPTION_UNWINDING |
                                                        EXCEPTION_EXIT_UNWIND);
    }

    //
    // Capture the context.
    //

    ContextRecord = &ContextRecord1;
    ContextRecord1.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_SEGMENTS;
    RtlpCaptureContext(ContextRecord);

#ifdef STD_CALL
    //
    // Adjust captured context to pop our arguments off the stack
    //
    ContextRecord->Esp += sizeof(TargetFrame) +
                          sizeof(TargetIp)    +
                          sizeof(ExceptionRecord) +
                          sizeof(ReturnValue);
#endif
    ContextRecord->Eax = (ULONG)ReturnValue;

    //
    // Scan backward through the call frame hierarchy, calling exception
    // handlers as they are encountered, until the target frame of the unwind
    // is reached.
    //

    RegistrationPointer = RtlpGetRegistrationHead();
    while (RegistrationPointer != EXCEPTION_CHAIN_END) {

        //
        // If this is the target of the unwind, then continue execution
        // by calling the continue system service.
        //

        if ((ULONG)RegistrationPointer == (ULONG)TargetFrame) {
            ZwContinue(ContextRecord, FALSE);

        //
        // If the target frame is lower in the stack than the current frame,
        // then raise STATUS_INVALID_UNWIND exception.
        //

        } else if ( (ARGUMENT_PRESENT(TargetFrame) == TRUE) &&
                    ((ULONG)TargetFrame < (ULONG)RegistrationPointer) ) {
            ExceptionRecord2.ExceptionCode = STATUS_INVALID_UNWIND_TARGET;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.NumberParameters = 0;
            RtlRaiseException(&ExceptionRecord2);
        }

        //
        // If the call frame is not within the specified stack limits or the
        // call frame is unaligned, then raise the exception STATUS_BAD_STACK.
        // Else restore the state from the specified frame to the context
        // record.
        //

        HighAddress = (ULONG)RegistrationPointer +
            sizeof(EXCEPTION_REGISTRATION_RECORD);

        if ( ((ULONG)RegistrationPointer < LowLimit) ||
             (HighAddress > HighLimit) ||
             (((ULONG)RegistrationPointer & 0x3) != 0) ) {

#if defined(NTOS_KERNEL_RUNTIME)

            //
            // Allow for the possibility that the problem occured on the
            // DPC stack.
            //

            ULONG TestAddress = (ULONG)RegistrationPointer;

            if (((TestAddress & 0x3) == 0) &&
                KeGetCurrentIrql() >= DISPATCH_LEVEL) {

                PKPRCB Prcb = KeGetCurrentPrcb();
                ULONG DpcStack = (ULONG)Prcb->DpcStack;

                if ((Prcb->DpcRoutineActive) &&
                    (HighAddress <= DpcStack) &&
                    (TestAddress >= DpcStack - KERNEL_STACK_SIZE)) {

                    //
                    // This error occured on the DPC stack, switch
                    // stack limits to the DPC stack and restart 
                    // the loop.
                    // 

                    HighLimit = DpcStack;
                    LowLimit = DpcStack - KERNEL_STACK_SIZE;
                    continue;
                }
            }

#endif

            ExceptionRecord2.ExceptionCode = STATUS_BAD_STACK;
            ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord2.ExceptionRecord = ExceptionRecord;
            ExceptionRecord2.NumberParameters = 0;
            RtlRaiseException(&ExceptionRecord2);
        } else {

            //
            // The handler must be executed by calling another routine
            // that is written in assembler. This is required because
            // up level addressing of the handler information is required
            // when a collided unwind is encountered.
            //

            Disposition = RtlpExecuteHandlerForUnwind(
                ExceptionRecord,
                (PVOID)RegistrationPointer,
                ContextRecord,
                (PVOID)&DispatcherContext,
                RegistrationPointer->Handler);

            //
            // Case on the handler disposition.
            //

            switch (Disposition) {

                //
                // The disposition is to continue the search. Get next
                // frame address and continue the search.
                //

            case ExceptionContinueSearch :
                break;

                //
                // The disposition is colided unwind. Maximize the target
                // of the unwind and change the context record pointer.
                //

            case ExceptionCollidedUnwind :

                //
                // Pick up the registration pointer that was active at
                // the time of the unwind, and simply continue.
                //

                RegistrationPointer = DispatcherContext.RegistrationPointer;
                break;


                //
                // All other disposition values are invalid. Raise
                // invalid disposition exception.
                //

            default :
                ExceptionRecord2.ExceptionCode = STATUS_INVALID_DISPOSITION;
                ExceptionRecord2.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord2.ExceptionRecord = ExceptionRecord;
                ExceptionRecord2.NumberParameters = 0;
                RtlRaiseException(&ExceptionRecord2);
                break;
            }

            //
            // Step to next registration record
            //

            PriorPointer = RegistrationPointer;
            RegistrationPointer = RegistrationPointer->Next;

            //
            // Unlink the unwind handler, since it's been called.
            //

            RtlpUnlinkHandler(PriorPointer);

            //
            // If chain goes in wrong direction or loops, raise an
            // exception.
            //

        }
    }

    if (TargetFrame == EXCEPTION_CHAIN_END) {

        //
        //  Caller simply wants to unwind all exception records.
        //  This differs from an exit_unwind in that no "exit" is desired.
        //  Do a normal continue, since we've effectively found the
        //  "target" the caller wanted.
        //

        ZwContinue(ContextRecord, FALSE);

    } else {

        //
        //  Either (1) a real exit unwind was performed, or (2) the
        //  specified TargetFrame is not present in the exception handler
        //  list.  In either case, give debugger and subsystem a chance
        //  to see the unwind.
        //

        ZwRaiseException(ExceptionRecord, ContextRecord, FALSE);

    }
    return;
}
