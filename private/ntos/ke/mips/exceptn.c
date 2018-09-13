/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    exceptn.c

Abstract:

    This module implement the code necessary to dispatch expections to the
    proper mode and invoke the exception dispatcher.

Author:

    David N. Cutler (davec) 3-Apr-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#define HEADER_FILE
#include "kxmips.h"

//
// Define multiply overflow and divide by zero breakpoint instruction values.
//

#define KDDEBUG_BREAKPOINT ((SPEC_OP << 26) | (BREAKIN_BREAKPOINT << 16) | BREAK_OP)
#define DIVIDE_BREAKPOINT ((SPEC_OP << 26) | (DIVIDE_BY_ZERO_BREAKPOINT << 16) | BREAK_OP)
#define MULTIPLY_BREAKPOINT ((SPEC_OP << 26) | (MULTIPLY_OVERFLOW_BREAKPOINT << 16) | BREAK_OP)
#define OVERFLOW_BREAKPOINT ((SPEC_OP << 26) | (DIVIDE_OVERFLOW_BREAKPOINT << 16) | BREAK_OP)

//
// Define external kernel breakpoint instruction value.
//

#define KERNEL_BREAKPOINT_INSTRUCTION 0x16000d

VOID
KeContextFromKframes (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT ContextFrame
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified trap and exception frames
    frames into the specified context frame according to the specified context
    flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame from which volatile context
        should be copied into the context record.

    ExceptionFrame - Supplies a pointer to an exception frame from which context
        should be copied into the context record.

    ContextFrame - Supplies a pointer to the context frame that receives the
        context copied from the trap and exception frames.

Return Value:

    None.

--*/

{

    ULONG ContextFlags;

    //
    // Set control information if specified.
    //

    ContextFlags = ContextFrame->ContextFlags;
    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set integer register gp, ra, sp, FIR, and PSR.
        //

        ContextFrame->XIntGp = TrapFrame->XIntGp;
        ContextFrame->XIntSp = TrapFrame->XIntSp;
        ContextFrame->Fir = TrapFrame->Fir;
        ContextFrame->Psr = TrapFrame->Psr;
        ContextFrame->XIntRa = TrapFrame->XIntRa;
    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set integer registers zero, and, at - t9, k0, k1, lo, and hi.
        //

        ContextFrame->XIntZero = 0;
        ContextFrame->XIntAt = TrapFrame->XIntAt;
        ContextFrame->XIntV0 = TrapFrame->XIntV0;
        ContextFrame->XIntV1 = TrapFrame->XIntV1;
        ContextFrame->XIntA0 = TrapFrame->XIntA0;
        ContextFrame->XIntA1 = TrapFrame->XIntA1;
        ContextFrame->XIntA2 = TrapFrame->XIntA2;
        ContextFrame->XIntA3 = TrapFrame->XIntA3;
        ContextFrame->XIntT0 = TrapFrame->XIntT0;
        ContextFrame->XIntT1 = TrapFrame->XIntT1;
        ContextFrame->XIntT2 = TrapFrame->XIntT2;
        ContextFrame->XIntT3 = TrapFrame->XIntT3;
        ContextFrame->XIntT4 = TrapFrame->XIntT4;
        ContextFrame->XIntT5 = TrapFrame->XIntT5;
        ContextFrame->XIntT6 = TrapFrame->XIntT6;
        ContextFrame->XIntT7 = TrapFrame->XIntT7;
        ContextFrame->XIntT8 = TrapFrame->XIntT8;
        ContextFrame->XIntT9 = TrapFrame->XIntT9;
        ContextFrame->XIntK0 = 0;
        ContextFrame->XIntK1 = 0;
        ContextFrame->XIntLo = TrapFrame->XIntLo;
        ContextFrame->XIntHi = TrapFrame->XIntHi;

        //
        // Set integer registers s0 - s7, and s8.
        //

        ContextFrame->XIntS0 = TrapFrame->XIntS0;
        ContextFrame->XIntS1 = TrapFrame->XIntS1;
        ContextFrame->XIntS2 = TrapFrame->XIntS2;
        ContextFrame->XIntS3 = TrapFrame->XIntS3;
        ContextFrame->XIntS4 = TrapFrame->XIntS4;
        ContextFrame->XIntS5 = TrapFrame->XIntS5;
        ContextFrame->XIntS6 = TrapFrame->XIntS6;
        ContextFrame->XIntS7 = TrapFrame->XIntS7;
        ContextFrame->XIntS8 = TrapFrame->XIntS8;
    }

    //
    // Set floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set floating registers f0 - f19.
        //

        RtlMoveMemory(&ContextFrame->FltF0, &TrapFrame->FltF0,
                     sizeof(ULONG) * (20));

        //
        // Set floating registers f20 - f31.
        //

        RtlMoveMemory(&ContextFrame->FltF20, &ExceptionFrame->FltF20,
                     sizeof(ULONG) * (12));

        //
        // Set floating status register.
        //

        ContextFrame->Fsr = TrapFrame->Fsr;
    }

    return;
}

VOID
KeContextToKframes (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextFrame,
    IN ULONG ContextFlags,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified context frame into
    the specified trap and exception frames according to the specified context
    flags.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that receives the volatile
        context from the context record.

    ExceptionFrame - Supplies a pointer to an exception frame that receives
        the nonvolatile context from the context record.

    ContextFrame - Supplies a pointer to a context frame that contains the
        context that is to be copied into the trap and exception frames.

    ContextFlags - Supplies the set of flags that specify which parts of the
        context frame are to be copied into the trap and exception frames.

    PreviousMode - Supplies the processor mode for which the trap and exception
        frames are being built.

Return Value:

    None.

--*/

{

    //
    // Set control information if specified.
    //

    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set integer register gp, sp, ra, FIR, and PSR.
        //

        TrapFrame->XIntGp = ContextFrame->XIntGp;
        TrapFrame->XIntSp = ContextFrame->XIntSp;
        TrapFrame->Fir = ContextFrame->Fir;
        TrapFrame->Psr = SANITIZE_PSR(ContextFrame->Psr, PreviousMode);
        TrapFrame->XIntRa = ContextFrame->XIntRa;
    }

    //
    // Set integer registers contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set integer registers at - t9, lo, and hi.
        //

        TrapFrame->XIntAt = ContextFrame->XIntAt;
        TrapFrame->XIntV0 = ContextFrame->XIntV0;
        TrapFrame->XIntV1 = ContextFrame->XIntV1;
        TrapFrame->XIntA0 = ContextFrame->XIntA0;
        TrapFrame->XIntA1 = ContextFrame->XIntA1;
        TrapFrame->XIntA2 = ContextFrame->XIntA2;
        TrapFrame->XIntA3 = ContextFrame->XIntA3;
        TrapFrame->XIntT0 = ContextFrame->XIntT0;
        TrapFrame->XIntT1 = ContextFrame->XIntT1;
        TrapFrame->XIntT2 = ContextFrame->XIntT2;
        TrapFrame->XIntT3 = ContextFrame->XIntT3;
        TrapFrame->XIntT4 = ContextFrame->XIntT4;
        TrapFrame->XIntT5 = ContextFrame->XIntT5;
        TrapFrame->XIntT6 = ContextFrame->XIntT6;
        TrapFrame->XIntT7 = ContextFrame->XIntT7;
        TrapFrame->XIntT8 = ContextFrame->XIntT8;
        TrapFrame->XIntT9 = ContextFrame->XIntT9;
        TrapFrame->XIntLo = ContextFrame->XIntLo;
        TrapFrame->XIntHi = ContextFrame->XIntHi;

        //
        // Set integer registers s0 - s7, and s8.
        //

        TrapFrame->XIntS0 = ContextFrame->XIntS0;
        TrapFrame->XIntS1 = ContextFrame->XIntS1;
        TrapFrame->XIntS2 = ContextFrame->XIntS2;
        TrapFrame->XIntS3 = ContextFrame->XIntS3;
        TrapFrame->XIntS4 = ContextFrame->XIntS4;
        TrapFrame->XIntS5 = ContextFrame->XIntS5;
        TrapFrame->XIntS6 = ContextFrame->XIntS6;
        TrapFrame->XIntS7 = ContextFrame->XIntS7;
        TrapFrame->XIntS8 = ContextFrame->XIntS8;
    }

    //
    // Set floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set floating registers f0 - f19.
        //

        RtlMoveMemory(&TrapFrame->FltF0, &ContextFrame->FltF0,
                     sizeof(ULONG) * (20));

        //
        // Set floating registers f20 - f31.
        //

        RtlMoveMemory(&ExceptionFrame->FltF20, &ContextFrame->FltF20,
                     sizeof(ULONG) * (12));

        //
        // Set floating status register.
        //

        TrapFrame->Fsr = SANITIZE_FSR(ContextFrame->Fsr, PreviousMode);
    }

    return;
}

VOID
KiDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    )

/*++

Routine Description:

    This function is called to dispatch an exception to the proper mode and
    to cause the exception dispatcher to be called.

    If the exception is a data misalignment, the previous mode is user, this
    is the first chance for handling the exception, and the current thread
    has enabled automatic alignment fixup, then an attempt is made to emulate
    the unaligned reference. Data misalignment exceptions are never emulated
    for kernel mode.

    If the exception is a floating exception (N.B. the pseudo status
    STATUS_FLOAT_STACK_CHECK is used to signify this and is converted to the
    proper code by the floating emulation routine), then an attempt is made
    to emulate the floating operation if it is not implemented.

    If the exception is neither a data misalignment nor a floating point
    exception and the the previous mode is kernel, then the exception
    dispatcher is called directly to process the exception. Otherwise the
    exception record, exception frame, and trap frame contents are copied
    to the user mode stack. The contents of the exception frame and trap
    are then modified such that when control is returned, execution will
    commense in user mode in a routine which will call the exception
    dispatcher.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    PreviousMode - Supplies the previous processor mode.

    FirstChance - Supplies a boolean variable that specifies whether this
        is the first (TRUE) or second (FALSE) time that this exception has
        been processed.

Return Value:

    None.

--*/

{

    CONTEXT ContextFrame;
    PULONG Destination;
    EXCEPTION_RECORD ExceptionRecord1;
    ULONG Index;
    LONG Length;
    PULONGLONG Source;
    BOOLEAN UserApcPending;
    ULONG UserStack1;
    ULONG UserStack2;

    //
    // If the exception is an access violation, and the previous mode is
    // user mode, then attempt to emulate a load or store operation if
    // the exception address is at the end of a page.
    //
    // N.B. The following is a workaround for a r4000 chip bug where an
    //      address privilege violation is reported as a access violation
    //      on a load or store instruction that is the last instruction
    //      in a page.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) &&
        (((ULONG)ExceptionRecord->ExceptionAddress & 0xffc) == 0xffc) &&
        (PreviousMode != KernelMode) &&
        (KiEmulateReference(ExceptionRecord, ExceptionFrame, TrapFrame) != FALSE)) {
        KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
        goto Handled2;
    }

    //
    // If the exception is a data bus error, then process the error.
    //
    // N.B. A special exception code is used to signal a data bus error.
    //      This code is equivalent to the bug check code merged with a
    //      reserved facility code and the reserved bit set.
    //
    // N.B. If control returns, then it is assumed that the error has been
    //      corrected.
    //

    if (ExceptionRecord->ExceptionCode == (DATA_BUS_ERROR | 0xdfff0000)) {

        //
        // N.B. The following is a workaround for a r4000 chip bug where an
        //      address privilege violation is reported as a data bus error
        //      on a load or store instruction that is the last instruction
        //      in a page.
        //

        if ((ExceptionRecord->ExceptionInformation[1] < 0x80000000) &&
            (((ULONG)ExceptionRecord->ExceptionAddress & 0xffc) == 0xffc) &&
            (PreviousMode != KernelMode)) {
            if (KiEmulateReference(ExceptionRecord, ExceptionFrame, TrapFrame) != FALSE) {
                KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
                goto Handled2;
            }
        }

        KiDataBusError(ExceptionRecord, ExceptionFrame, TrapFrame);
        goto Handled2;
    }

    //
    // If the exception is an instruction bus error, then process the error.
    //
    // N.B. A special exception code is used to signal an instruction bus
    //      error. This code is equivalent to the bug check code merged
    //      with a reserved facility code and the reserved bit set.
    //
    // N.B. If control returns, then it is assumed that the error hand been
    //      corrected.
    //

    if (ExceptionRecord->ExceptionCode == (INSTRUCTION_BUS_ERROR | 0xdfff0000)) {
        KiInstructionBusError(ExceptionRecord, ExceptionFrame, TrapFrame);
        goto Handled2;
    }

    //
    // If the exception is a data misalignment, this is the first change for
    // handling the exception, and the current thread has enabled automatic
    // alignment fixup, then attempt to emulate the unaligned reference.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_DATATYPE_MISALIGNMENT) &&
        (FirstChance != FALSE) &&
        ((KeGetCurrentThread()->AutoAlignment != FALSE) ||
         (KeGetCurrentThread()->ApcState.Process->AutoAlignment != FALSE) ||
         (((ExceptionRecord->ExceptionInformation[1] & 0x7fff0000) == 0x7fff0000) &&
         (PreviousMode != KernelMode))) &&
        (KiEmulateReference(ExceptionRecord, ExceptionFrame, TrapFrame) != FALSE)) {
        KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
        goto Handled2;
    }

    //
    // If the exception is a floating exception, then attempt to emulate the
    // operation.
    //
    // N.B. The pseudo status STATUS_FLOAT_STACK_CHECK is used to signify
    //      that the exception is a floating exception and that this it the
    //      first chance for handling the exception. The floating emulation
    //      routine converts the status code to the proper floating status
    //      value.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_FLOAT_STACK_CHECK) &&
        (KiEmulateFloating(ExceptionRecord, ExceptionFrame, TrapFrame) != FALSE)) {
        TrapFrame->Fsr = SANITIZE_FSR(TrapFrame->Fsr, PreviousMode);
        goto Handled2;
    }

    //
    // If the exception is a breakpoint, then translate it to an appropriate
    // exception code if it is a division by zero or an integer overflow
    // caused by multiplication.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) {
        if (ExceptionRecord->ExceptionInformation[0] == DIVIDE_BREAKPOINT) {
            ExceptionRecord->ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;

        } else if ((ExceptionRecord->ExceptionInformation[0] == MULTIPLY_BREAKPOINT) ||
                   (ExceptionRecord->ExceptionInformation[0] == OVERFLOW_BREAKPOINT)) {
            ExceptionRecord->ExceptionCode = STATUS_INTEGER_OVERFLOW;

        } else if (ExceptionRecord->ExceptionInformation[0] == KDDEBUG_BREAKPOINT) {
            TrapFrame->Fir += 4;
        }
    }

    //
    // Move machine state from trap and exception frames to a context frame,
    // and increment the number of exceptions dispatched.
    //

    ContextFrame.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextFrame);
    KeGetCurrentPrcb()->KeExceptionDispatchCount += 1;

    //
    // Select the method of handling the exception based on the previous mode.
    //

    if (PreviousMode == KernelMode) {

        //
        // Previous mode was kernel.
        //
        // If this is the first chance, the kernel debugger is active, and
        // the exception is a kernel breakpoint, then give the kernel debugger
        // a chance to handle the exception.
        //
        // If this is the first chance and the kernel debugger is not active
        // or does not handle the exception, then attempt to find a frame
        // handler to handle the exception.
        //
        // If this is the second chance or the exception is not handled, then
        // if the kernel debugger is active, then give the kernel debugger a
        // second chance to handle the exception. If the kernel debugger does
        // not handle the exception, then bug check.
        //

        if (FirstChance != FALSE) {

            //
            // If the kernel debugger is active, the exception is a breakpoint,
            // and the breakpoint is handled by the kernel debugger, then give
            // the kernel debugger a chance to handle the exception.
            //

            if ((KiDebugRoutine != NULL) &&
               (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
               (KdIsThisAKdTrap(ExceptionRecord,
                                &ContextFrame,
                                KernelMode) != FALSE)) {

                if (((KiDebugRoutine) (TrapFrame,
                                       ExceptionFrame,
                                       ExceptionRecord,
                                       &ContextFrame,
                                       KernelMode,
                                       FALSE)) != FALSE) {

                    goto Handled1;
                }
            }

            //
            // This is the first chance to handle the exception.
            //

            if (RtlDispatchException(ExceptionRecord, &ContextFrame) != FALSE) {
                goto Handled1;
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        if (KiDebugRoutine != NULL) {
            if (((KiDebugRoutine) (TrapFrame,
                                   ExceptionFrame,
                                   ExceptionRecord,
                                   &ContextFrame,
                                   PreviousMode,
                                   TRUE)) != FALSE) {
                goto Handled1;
            }
        }

        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG)ExceptionRecord->ExceptionAddress,
                     ExceptionRecord->ExceptionInformation[0],
                     ExceptionRecord->ExceptionInformation[1]);

    } else {

        //
        // Previous mode was user.
        //
        // If this is the first chance, the kernel debugger is active, the
        // exception is a kernel breakpoint, and the current process is not
        // being debugged, or the current process is being debugged, but the
        // the breakpoint is not a kernel breakpoint instruction, then give
        // the kernel debugger a chance to handle the exception.
        //
        // If this is the first chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // transfer the exception information to the user stack, transition to
        // user mode, and attempt to dispatch the exception to a frame based
        // handler. If a frame based handler handles the exception, then continue
        // execution. Otherwise, execute the raise exception system service
        // which will call this routine a second time to process the exception.
        //
        // If this is the second chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // if the current process has a subsystem port, then send a message to
        // the subsystem port and wait for a reply. If the subsystem handles the
        // exception, then continue execution. Else terminate the thread.
        //

        if (FirstChance != FALSE) {

            //
            // If the kernel debugger is active, the exception is a kernel
            // breakpoint, and the current process is not being debugged,
            // or the current process is being debugged, but the breakpoint
            // is not a kernel breakpoint instruction, then give the kernel
            // debugger a chance to handle the exception.
            //

            if ((KiDebugRoutine != NULL) &&
               (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
               (KdIsThisAKdTrap(ExceptionRecord,
                                &ContextFrame,
                                UserMode) != FALSE) &&
               ((PsGetCurrentProcess()->DebugPort == NULL) ||
               ((PsGetCurrentProcess()->DebugPort != NULL) &&
               (ExceptionRecord->ExceptionInformation[0] !=
                                            KERNEL_BREAKPOINT_INSTRUCTION)))) {

               if (((KiDebugRoutine) (TrapFrame,
                                      ExceptionFrame,
                                      ExceptionRecord,
                                      &ContextFrame,
                                      UserMode,
                                      FALSE)) != FALSE) {

                    goto Handled1;
                }
            }

            //
            // This is the first chance to handle the exception.
            //

            if (DbgkForwardException(ExceptionRecord, TRUE, FALSE)) {
                TrapFrame->Fsr = SANITIZE_FSR(TrapFrame->Fsr, UserMode);
                goto Handled2;
            }

            //
            // Transfer exception information to the user stack, transition
            // to user mode, and attempt to dispatch the exception to a frame
            // based handler.
            //

        repeat:
            try {

                //
                // Coerce the 64-bit integer register context to 32-bits
                // and store in the 32-bit context area of the context
                // record.
                //
                // N.B. This only works becasue the 32- and 64-bit integer
                //      register context does not overlap in the context
                //      record.
                //

                Destination = &ContextFrame.IntZero;
                Source = &ContextFrame.XIntZero;
                for (Index = 0; Index < 32; Index += 1) {
                    *Destination++ = (ULONG)*Source++;
                }

                //
                // Compute length of exception record and new aligned stack
                // address.
                //

                Length = (sizeof(EXCEPTION_RECORD) + 7) & (~7);
                UserStack1 = (ULONG)(ContextFrame.XIntSp & (~7)) - Length;

                //
                // Probe user stack area for writeability and then transfer the
                // exception record to the user stack area.
                //

                ProbeForWrite((PCHAR)UserStack1, Length, sizeof(QUAD));
                RtlMoveMemory((PVOID)UserStack1, ExceptionRecord, Length);

                //
                // Compute length of context record and new aligned user stack
                // pointer.
                //

                Length = sizeof(CONTEXT);
                UserStack2 = UserStack1 - Length;

                //
                // Probe user stack area for writeability and then transfer the
                // context record to the user stack.
                //

                ProbeForWrite((PCHAR)UserStack2, Length, sizeof(QUAD));
                RtlMoveMemory((PVOID)UserStack2, &ContextFrame, sizeof(CONTEXT));

                //
                // Set address of exception record, context record, and the
                // and the new stack pointer in the current trap frame.
                //

                TrapFrame->XIntSp = (LONG)UserStack2;
                TrapFrame->XIntS8 = (LONG)UserStack2;
                TrapFrame->XIntS0 = (LONG)UserStack1;
                TrapFrame->XIntS1 = (LONG)UserStack2;

                //
                // Sanitize the floating status register so a recursive
                // exception will not occur.
                //

                TrapFrame->Fsr = SANITIZE_FSR(ContextFrame.Fsr, UserMode);

                //
                // Set the address of the exception routine that will call the
                // exception dispatcher and then return to the trap handler.
                // The trap handler will restore the exception and trap frame
                // context and continue execution in the routine that will
                // call the exception dispatcher.
                //

                TrapFrame->Fir = KeUserExceptionDispatcher;
                return;

            //
            // If an exception occurs, then copy the new exception information
            // to an exception record and handle the exception.
            //

            } except (KiCopyInformation(&ExceptionRecord1,
                               (GetExceptionInformation())->ExceptionRecord)) {

                //
                // If the exception is a stack overflow, then attempt
                // to raise the stack overflow exception. Otherwise,
                // the user's stack is not accessible, or is misaligned,
                // and second chance processing is performed.
                //

                if (ExceptionRecord1.ExceptionCode == STATUS_STACK_OVERFLOW) {
                    ExceptionRecord1.ExceptionAddress = ExceptionRecord->ExceptionAddress;
                    RtlMoveMemory((PVOID)ExceptionRecord,
                                  &ExceptionRecord1, sizeof(EXCEPTION_RECORD));
                    goto repeat;
                }
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        UserApcPending = KeGetCurrentThread()->ApcState.UserApcPending;
        if (DbgkForwardException(ExceptionRecord, TRUE, TRUE)) {
            TrapFrame->Fsr = SANITIZE_FSR(TrapFrame->Fsr, UserMode);
            goto Handled2;

        } else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE)) {

            //
            // If a user APC was not previously pending and one is now
            // pending, then the thread has been terminated and the PC
            // must be forced to a legal address so an infinite loop does
            // not occur for the case where a jump to an unmapped address
            // occured.
            //

            if ((UserApcPending == FALSE) &&
                (KeGetCurrentThread()->ApcState.UserApcPending != FALSE)) {
                TrapFrame->Fir = (ULONG)USPCR;
            }

            TrapFrame->Fsr = SANITIZE_FSR(TrapFrame->Fsr, UserMode);
            goto Handled2;

        } else {
            ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
            KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                         ExceptionRecord->ExceptionCode,
                         (ULONG)ExceptionRecord->ExceptionAddress,
                         ExceptionRecord->ExceptionInformation[0],
                         ExceptionRecord->ExceptionInformation[1]);
        }
    }

    //
    // Move machine state from context frame to trap and exception frames and
    // then return to continue execution with the restored state.
    //

Handled1:
    KeContextToKframes(TrapFrame, ExceptionFrame, &ContextFrame,
                       ContextFrame.ContextFlags, PreviousMode);

    //
    // Exception was handled by the debugger or the associated subsystem
    // and state was modified, if necessary, using the get state and set
    // state capabilities. Therefore the context frame does not need to
    // be transfered to the trap and exception frames.
    //

Handled2:
    return;
}

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    )

/*++

Routine Description:

    This function is called from an exception filter to copy the exception
    information from one exception record to another when an exception occurs.

Arguments:

    ExceptionRecord1 - Supplies a pointer to the destination exception record.

    ExceptionRecord2 - Supplies a pointer to the source exception record.

Return Value:

    A value of EXCEPTION_EXECUTE_HANDLER is returned as the function value.

--*/

{

    //
    // Copy one exception record to another and return value that causes
    // an exception handler to be executed.
    //

    RtlMoveMemory((PVOID)ExceptionRecord1,
                  (PVOID)ExceptionRecord2,
                  sizeof(EXCEPTION_RECORD));

    return EXCEPTION_EXECUTE_HANDLER;
}


NTSTATUS
KeRaiseUserException(
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This function causes an exception to be raised in the calling thread's
    usermode context. This is accomplished by editing the trap frame the
    kernel was entered with to point to trampoline code that raises the
    requested exception.

Arguments:

    ExceptionCode - Supplies the status value to be used as the exception
        code for the exception that is to be raised.

Return Value:

    The status value that should be returned by the caller.

--*/

{

    PKTRAP_FRAME TrapFrame;

    ASSERT(KeGetPreviousMode() == UserMode);

    TrapFrame = KeGetCurrentThread()->TrapFrame;
    TrapFrame->Fir = KeRaiseUserExceptionDispatcher;
    return ExceptionCode;
}
