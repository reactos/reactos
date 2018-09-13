/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993, 1994  Digital Equipment Corporation

Module Name:

    exceptn.c

Abstract:

    This module implements the code necessary to dispatch exceptions to the
    proper mode and invoke the exception dispatcher.

Author:

    David N. Cutler (davec) 3-Apr-1990

Environment:

    Kernel mode only.

Revision History:

    Thomas Van Baak (tvb) 12-May-1992

        Adapted for Alpha AXP.

--*/

#include "ki.h"

VOID
KiMachineCheck (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

//
// Alpha data misalignment exception (auto alignment fixup) control.
//
// If KiEnableAlignmentFaultExceptions is 0, then no alignment
// exceptions are raised and all misaligned user and kernel mode data
// references are emulated. This is consistent with NT/Alpha version
// 3.1 behavior.
//
// If KiEnableAlignmentFaultExceptions is 1, then the
// current thread automatic alignment fixup enable determines whether
// emulation is attempted in user mode. This is consistent with NT/Mips
// behavior.
//
// If KiEnableAlignmentFaultExceptions is 2 and the process is being
// debugged, the current thread automatic alignment fixup enable determines
// whether emulation is attempted in user mode. This allows developers to
// easily find and fix alignment problems.
//
// If KiEnableAlignmentFaultExceptions is 2 and the process is NOT being
// debugged, then no alignment exceptions are raised and all misaligned
// user and kernel mode data references are emulated.
//
// N.B. This default value may be reset from the Registry during init.
//

#if defined(_AXP64_) && defined(_AXP64_FIXFIX)

//
// On AXP64, the default is to disable alignment fault fixups.
//
// FIXFIX: This code is currently disabled by _AXP64_FIXFIX.

#define DEFAULT_NO_ALIGNMENT_FIXUPS TRUE
#else
#define DEFAULT_NO_ALIGNMENT_FIXUPS FALSE
#endif

ULONG KiEnableAlignmentFaultExceptions = DEFAULT_NO_ALIGNMENT_FIXUPS ? 1 : 0;

#if DBG

//
// Set KiBreakOnAlignmentFault to TRUE to stop the debugger on each alignment
// fault.
//

BOOLEAN KiBreakOnAlignmentFault = FALSE;

//
// Alignment fixups are counted by mode and reported at intervals.
//
// N.B. Set masks to 0 to see every exception (set to 0x7 to see every
//      8th, etc.).
//

ULONG KiKernelFixupCount = 0;
ULONG KiKernelFixupMask = DEFAULT_NO_ALIGNMENT_FIXUPS ? 0 : 0x7f;

ULONG KiUserFixupCount = 0;
ULONG KiUserFixupMask = DEFAULT_NO_ALIGNMENT_FIXUPS ? 0 : 0x3ff;

#endif

VOID
KeContextFromKframes (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT ContextFrame
    )

/*++

Routine Description:

    This routine moves the selected contents of the specified trap and exception
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

    //
    // Set control information if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set integer register gp, ra, sp, FIR, and PSR from trap frame.
        //

        ContextFrame->IntGp = TrapFrame->IntGp;
        ContextFrame->IntSp = TrapFrame->IntSp;
        ContextFrame->IntRa = TrapFrame->IntRa;
        ContextFrame->Fir = TrapFrame->Fir;
        ContextFrame->Psr = TrapFrame->Psr;
    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set volatile integer registers v0 and t0 - t7 from trap frame.
        //

        ContextFrame->IntV0 = TrapFrame->IntV0;
        ContextFrame->IntT0 = TrapFrame->IntT0;
        ContextFrame->IntT1 = TrapFrame->IntT1;
        ContextFrame->IntT2 = TrapFrame->IntT2;
        ContextFrame->IntT3 = TrapFrame->IntT3;
        ContextFrame->IntT4 = TrapFrame->IntT4;
        ContextFrame->IntT5 = TrapFrame->IntT5;
        ContextFrame->IntT6 = TrapFrame->IntT6;
        ContextFrame->IntT7 = TrapFrame->IntT7;

        //
        // Set nonvolatile integer registers s0 - s5 from exception frame.
        //

        ContextFrame->IntS0 = ExceptionFrame->IntS0;
        ContextFrame->IntS1 = ExceptionFrame->IntS1;
        ContextFrame->IntS2 = ExceptionFrame->IntS2;
        ContextFrame->IntS3 = ExceptionFrame->IntS3;
        ContextFrame->IntS4 = ExceptionFrame->IntS4;
        ContextFrame->IntS5 = ExceptionFrame->IntS5;

        //
        // Set volatile integer registers a0 - a5, and t8 - t11 from trap
        // frame.
        //

        ContextFrame->IntA0 = TrapFrame->IntA0;
        ContextFrame->IntA1 = TrapFrame->IntA1;
        ContextFrame->IntA2 = TrapFrame->IntA2;
        ContextFrame->IntA3 = TrapFrame->IntA3;
        ContextFrame->IntA4 = TrapFrame->IntA4;
        ContextFrame->IntA5 = TrapFrame->IntA5;

        ContextFrame->IntT8 = TrapFrame->IntT8;
        ContextFrame->IntT9 = TrapFrame->IntT9;
        ContextFrame->IntT10 = TrapFrame->IntT10;
        ContextFrame->IntT11 = TrapFrame->IntT11;

        //
        // Set volatile integer registers fp, t12 and at from trap frame.
        // Set integer register zero.
        //

        ContextFrame->IntFp = TrapFrame->IntFp;
        ContextFrame->IntT12 = TrapFrame->IntT12;
        ContextFrame->IntAt = TrapFrame->IntAt;
        ContextFrame->IntZero = 0;
    }

    //
    // Set floating register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set volatile floating registers f0 - f1 from trap frame.
        // Set volatile floating registers f10 - f30 from trap frame.
        // Set floating zero register f31 to 0.
        //

        ContextFrame->FltF0 = TrapFrame->FltF0;
        ContextFrame->FltF1 = TrapFrame->FltF1;
        RtlMoveMemory(&ContextFrame->FltF10, &TrapFrame->FltF10,
                      sizeof(ULONGLONG) * 21);
        ContextFrame->FltF31 = 0;

        //
        // Set nonvolatile floating registers f2 - f9 from exception frame.
        //

        ContextFrame->FltF2 = ExceptionFrame->FltF2;
        ContextFrame->FltF3 = ExceptionFrame->FltF3;
        ContextFrame->FltF4 = ExceptionFrame->FltF4;
        ContextFrame->FltF5 = ExceptionFrame->FltF5;
        ContextFrame->FltF6 = ExceptionFrame->FltF6;
        ContextFrame->FltF7 = ExceptionFrame->FltF7;
        ContextFrame->FltF8 = ExceptionFrame->FltF8;
        ContextFrame->FltF9 = ExceptionFrame->FltF9;

        //
        // Set floating point control register from trap frame.
        // Clear software floating point control register in context frame
        // (if necessary, it can be set to the proper value by the caller).
        //

        ContextFrame->Fpcr = TrapFrame->Fpcr;
        ContextFrame->SoftFpcr = 0;
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
        // Set integer register gp, sp, ra, FIR, and PSR in trap frame.
        //

        TrapFrame->IntGp = ContextFrame->IntGp;
        TrapFrame->IntSp = ContextFrame->IntSp;
        TrapFrame->IntRa = ContextFrame->IntRa;
        TrapFrame->Fir = ContextFrame->Fir;
        TrapFrame->Psr = SANITIZE_PSR(ContextFrame->Psr, PreviousMode);
    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set volatile integer registers v0 and t0 - t7 in trap frame.
        //

        TrapFrame->IntV0 = ContextFrame->IntV0;
        TrapFrame->IntT0 = ContextFrame->IntT0;
        TrapFrame->IntT1 = ContextFrame->IntT1;
        TrapFrame->IntT2 = ContextFrame->IntT2;
        TrapFrame->IntT3 = ContextFrame->IntT3;
        TrapFrame->IntT4 = ContextFrame->IntT4;
        TrapFrame->IntT5 = ContextFrame->IntT5;
        TrapFrame->IntT6 = ContextFrame->IntT6;
        TrapFrame->IntT7 = ContextFrame->IntT7;

        //
        // Set nonvolatile integer registers s0 - s5 in exception frame.
        //

        ExceptionFrame->IntS0 = ContextFrame->IntS0;
        ExceptionFrame->IntS1 = ContextFrame->IntS1;
        ExceptionFrame->IntS2 = ContextFrame->IntS2;
        ExceptionFrame->IntS3 = ContextFrame->IntS3;
        ExceptionFrame->IntS4 = ContextFrame->IntS4;
        ExceptionFrame->IntS5 = ContextFrame->IntS5;

        //
        // Set volatile integer registers a0 - a5, and t8 - t11 in trap frame.
        //

        TrapFrame->IntA0 = ContextFrame->IntA0;
        TrapFrame->IntA1 = ContextFrame->IntA1;
        TrapFrame->IntA2 = ContextFrame->IntA2;
        TrapFrame->IntA3 = ContextFrame->IntA3;
        TrapFrame->IntA4 = ContextFrame->IntA4;
        TrapFrame->IntA5 = ContextFrame->IntA5;

        TrapFrame->IntT8 = ContextFrame->IntT8;
        TrapFrame->IntT9 = ContextFrame->IntT9;
        TrapFrame->IntT10 = ContextFrame->IntT10;
        TrapFrame->IntT11 = ContextFrame->IntT11;

        //
        // Set volatile integer registers fp, t12 and at in trap frame.
        //

        TrapFrame->IntFp = ContextFrame->IntFp;
        TrapFrame->IntT12 = ContextFrame->IntT12;
        TrapFrame->IntAt = ContextFrame->IntAt;
    }

    //
    // Set floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set volatile floating registers f0 - f1 in trap frame.
        // Set volatile floating registers f10 - f30 in trap frame.
        //

        TrapFrame->FltF0 = ContextFrame->FltF0;
        TrapFrame->FltF1 = ContextFrame->FltF1;
        RtlMoveMemory(&TrapFrame->FltF10, &ContextFrame->FltF10,
                      sizeof(ULONGLONG) * 21);

        //
        // Set nonvolatile floating registers f2 - f9 in exception frame.
        //

        ExceptionFrame->FltF2 = ContextFrame->FltF2;
        ExceptionFrame->FltF3 = ContextFrame->FltF3;
        ExceptionFrame->FltF4 = ContextFrame->FltF4;
        ExceptionFrame->FltF5 = ContextFrame->FltF5;
        ExceptionFrame->FltF6 = ContextFrame->FltF6;
        ExceptionFrame->FltF7 = ContextFrame->FltF7;
        ExceptionFrame->FltF8 = ContextFrame->FltF8;
        ExceptionFrame->FltF9 = ContextFrame->FltF9;

        //
        // Set floating point control register in trap frame.
        //

        TrapFrame->Fpcr = ContextFrame->Fpcr;
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

    If the exception is a floating not implemented exception, then an attempt
    is made to emulate the floating operation. If the exception is an
    arithmetic exception, then an attempt is made to convert the imprecise
    exception into a precise exception, and then emulate the floating
    operation in order to obtain the proper IEEE results and exceptions.
    Floating exceptions are never emulated for kernel mode.

    If the exception is neither a data misalignment nor a floating point
    exception and the previous mode is kernel, then the exception
    dispatcher is called directly to process the exception. Otherwise the
    exception record, exception frame, and trap frame contents are copied
    to the user mode stack. The contents of the exception frame and trap
    are then modified such that when control is returned, execution will
    commence in user mode in a routine which will call the exception
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
    EXCEPTION_RECORD ExceptionRecord1;
    PEXC_SUM ExceptionSummary;
    LONG Length;
    ULONG SoftFpcr;
    ULONGLONG UserStack1;
    ULONGLONG UserStack2;

    //
    // If the exception is an illegal instruction exception, then check for
    // a byte/word instruction that should be emulated.
    //
    // N.B. The exception code STATUS_ILLEGAL_INSTRUCTION may be converted
    //      into STATUS_DATATYPE_MISALIGNMENT in the case of unaligned word
    //      access.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_ILLEGAL_INSTRUCTION) {
        if (KiEmulateByteWord(ExceptionRecord,
                              ExceptionFrame,
                              TrapFrame) != FALSE) {
            KeGetCurrentPrcb()->KeByteWordEmulationCount += 1;
            goto Handled2;
        }
    }

#if DBG
    if (KiBreakOnAlignmentFault != FALSE) {

        if (ExceptionRecord->ExceptionCode == STATUS_DATATYPE_MISALIGNMENT) {

            DbgPrint("KI: Alignment fault exr = %p Pc = %p Address = %p\n",
                     ExceptionRecord,
                     ExceptionRecord->ExceptionAddress,
                     ExceptionRecord->ExceptionInformation[2]);
    
            DbgBreakPoint();
        }
    }
#endif

    //
    // If the exception is a data misalignment, the previous mode was user,
    // this is the first chance for handling the exception, and the current
    // thread has enabled automatic alignment fixup, then attempt to emulate
    // the unaligned reference.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_DATATYPE_MISALIGNMENT) &&
        (FirstChance != FALSE)) {

#if DBG

        //
        // Count alignment faults by mode and display them at intervals.
        //

        if (PreviousMode == KernelMode) {
            KiKernelFixupCount += 1;
            if ((KiKernelFixupCount & KiKernelFixupMask) == 0) {
                DbgPrint("KI: Kernel Fixup: Pid=0x%.3lx, Pc=%.16p, Address=%.16p ... Total=%ld\n",
                         PsGetCurrentProcess()->UniqueProcessId,
                         ExceptionRecord->ExceptionAddress,
                         ExceptionRecord->ExceptionInformation[2],
                         KiKernelFixupCount);
            }

        } else {
            KiUserFixupCount += 1;
            if ((KiUserFixupCount & KiUserFixupMask) == 0) {
                DbgPrint("KI: User  Fixup: Pid=0x%.3lx, Pc=%.16p, Address=%.16p ... Total=%ld\n",
                         PsGetCurrentProcess()->UniqueProcessId,
                         ExceptionRecord->ExceptionAddress,
                         ExceptionRecord->ExceptionInformation[2],
                         KiUserFixupCount);
            }
        }

#endif

        //
        // If alignment fault exceptions are not enabled, then no exception
        // should be raised and the data reference should be emulated.
        //
        // We will emulate the reference if
        //      KiEnableAlignmentFaultExceptions == 0 (always fix up all faults, the default)
        // OR   The thread has explicitly enabled alignment fixups
        // OR   KiEnableAlignmentFaultExceptions == 2 and the process is not being debugged
        //

        if ( (KiEnableAlignmentFaultExceptions == 0) ||

             ((KeGetCurrentThread()->AutoAlignment != FALSE) ||
              (KeGetCurrentThread()->ApcState.Process->AutoAlignment != FALSE)) ||

             (((PsGetCurrentProcess()->DebugPort == NULL) || (PreviousMode == KernelMode)) &&
              (KiEnableAlignmentFaultExceptions == 2)) ) {

            if (KiEmulateReference(ExceptionRecord,
                                   ExceptionFrame,
                                   TrapFrame,
                                   FALSE) != FALSE) {
                KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
                goto Handled2;
            }
        }
    }

    //
    // If the exception is a data bus error then a machine check has
    // been trapped by the PALcode. The error will be forwarded to the
    // HAL eventually for logging or handling. If the handler returns
    // it is assumed that the HAL successfully handled the error and
    // execution may resume.
    //
    // N.B. A special exception code is used to signal a data bus error.
    //      This code is equivalent to the bug check code merged with a
    //      reserved facility code and the reserved bit set.
    //

    if (ExceptionRecord->ExceptionCode == (DATA_BUS_ERROR | 0xdfff0000)) {
        KiMachineCheck(ExceptionRecord, ExceptionFrame, TrapFrame);
        goto Handled2;
    }

    //
    // Initialize the copy of the software FPCR. The proper value is set
    // if a floating emulation operation is performed. Case on arithmetic
    // exception codes that require special handling by the kernel.
    //

    SoftFpcr = 0;
    switch (ExceptionRecord->ExceptionCode) {

        //
        // If the exception is a gentrap, then attempt to translate the
        // Alpha specific gentrap value to a status code value. This
        // exception is a precise trap.
        //
        // N.B. STATUS_ALPHA_GENTRAP is a pseudo status code generated by
        //      PALcode when a callpal gentrap is executed. The status is
        //      visible in user mode only when the gentrap code value is
        //      unrecognized.
        //

    case STATUS_ALPHA_GENTRAP :
        switch (ExceptionRecord->ExceptionInformation[0]) {
        case GENTRAP_INTEGER_OVERFLOW :
            ExceptionRecord->ExceptionCode = STATUS_INTEGER_OVERFLOW;
            break;

        case GENTRAP_INTEGER_DIVIDE_BY_ZERO :
            ExceptionRecord->ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
            break;

        case GENTRAP_FLOATING_OVERFLOW :
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
            break;

        case GENTRAP_FLOATING_DIVIDE_BY_ZERO :
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            break;

        case GENTRAP_FLOATING_UNDERFLOW :
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
            break;

        case GENTRAP_FLOATING_INVALID_OPERAND :
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            break;

        case GENTRAP_FLOATING_INEXACT_RESULT :
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            break;
        }
        break;

        //
        // If the exception is an unimplemented floating operation, then
        // PALcode has detected a subsetted floating point operation. These
        // include attempts to use round to plus or minus infinity rounding
        // modes on EV4. This exception is a fault.
        //
        // If the previous mode was user, an attempt is made to emulate the
        // operation. If the emulation is successful, the continuation
        // address is incremented to the next instruction.
        //
        // N.B. STATUS_ALPHA_FLOATING_NOT_IMPLEMENTED is a pseudo status code
        //      generated by PALcode. The status is never visible outside of
        //      this handler because the floating emulation routine converts
        //      the status code to the proper floating status value.
        //

    case STATUS_ALPHA_FLOATING_NOT_IMPLEMENTED :
        if (PreviousMode != KernelMode) {
            if (KiFloatingException(ExceptionRecord,
                                    ExceptionFrame,
                                    TrapFrame,
                                    FALSE,
                                    &SoftFpcr) != FALSE) {
                TrapFrame->Fir += 4;
                goto Handled2;
            }

        } else {
            ExceptionRecord->ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
        }

        break;

        //
        // If the exception is an arithmetic exception, then one or more
        // integer overflow or floating point traps has occurred. This
        // exception is an imprecise (asynchronous) trap. Attempt to locate 
        // the original trapping instruction and emulate the instruction.
        //
        // N.B. STATUS_ALPHA_ARITHMETIC_EXCEPTION is a pseudo status code
        //      generated by PALcode. The status is never visible outside of
        //      this handler because the floating emulation routine converts
        //      the status code to the proper floating status value.
        //

    case STATUS_ALPHA_ARITHMETIC_EXCEPTION :
        if (KiFloatingException(ExceptionRecord,
                                ExceptionFrame,
                                TrapFrame,
                                TRUE,
                                &SoftFpcr) != FALSE) {
            goto Handled2;
        }
        break;
    }

    //
    // Move machine state from trap and exception frames to a context frame,
    // and increment the number of exceptions dispatched.
    //
    // Explicitly set the value of the software FPCR in the context frame
    // (because it is not a hardware register and thus not present in the
    // trap or exception frames).
    //

    ContextFrame.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextFrame);
    KeGetCurrentPrcb()->KeExceptionDispatchCount += 1;
    ContextFrame.SoftFpcr = (ULONGLONG)SoftFpcr;

    //
    // Select the method of handling the exception based on the previous mode.
    //

    if (PreviousMode == KernelMode) {

        //
        // If the kernel debugger is active, the exception is a breakpoint,
        // the breakpoint is handled by the kernel debugger, and this is the
        // first chance, then give the kernel debugger a chance to handle
        // the exception.
        //

        if ((FirstChance != FALSE) && (KiDebugRoutine != NULL) &&
           (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
           (KdIsThisAKdTrap(ExceptionRecord,
                            &ContextFrame,
                            KernelMode) != FALSE) &&

           (((KiDebugRoutine) (TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &ContextFrame,
                               KernelMode,
                               FALSE)) != FALSE)) {

            goto Handled1;
        }


        //
        // Previous mode was kernel.
        //
        // If this is the first chance, then attempt to dispatch the exception
        // to a frame based handler. If the exception is handled, then continue
        // execution.
        //
        // If this is the second chance or the exception is not handled,
        // then if the kernel debugger is active, then give the kernel
        // debugger a second chance to handle the exception. If the kernel
        // debugger handles the exception, then continue execution. Otherwise
        // bug check.
        //

        if (FirstChance != FALSE) {

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

        if ((KiDebugRoutine != NULL) &&
           (((KiDebugRoutine) (TrapFrame,
                               ExceptionFrame,
                               ExceptionRecord,
                               &ContextFrame,
                               PreviousMode,
                                   TRUE)) != FALSE)) {

            goto Handled1;
        }

        KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                     ExceptionRecord->ExceptionCode,
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
                     ExceptionRecord->ExceptionInformation[0],
                     ExceptionRecord->ExceptionInformation[1]);

    } else {

        //
        // If the kernel debugger is active, the exception is a breakpoint,
        // the breakpoint is handled by the kernel debugger, and this is the
        // first chance, then give the kernel debugger a chance to handle
        // the exception.
        //

        if ((FirstChance != FALSE) &&
            (KiDebugRoutine != NULL) &&
            (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
            (KdIsThisAKdTrap(ExceptionRecord,
                             &ContextFrame,
                             UserMode) != FALSE) &&

            ((PsGetCurrentProcess()->DebugPort == NULL) ||
             ((PsGetCurrentProcess()->DebugPort != NULL) &&
              (ExceptionRecord->ExceptionInformation[0] !=
                                            DEBUG_STOP_BREAKPOINT)))) {

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
        // Previous mode was user.
        //
        // If this is the first chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Otherwise
        // transfer the exception information to the user stack, transition to
        // user mode, and attempt to dispatch the exception to a frame based
        // handler. If a frame based handler handles the exception, then continue
        // execution. Otherwise, execute the raise exception system service
        // which will call this routine a second time to process the exception.
        //
        // If this is the second chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Otherwise
        // if the current process has a subsystem port, then send a message to
        // the subsystem port and wait for a reply. If the subsystem handles the
        // exception, then continue execution. Otherwise terminate the thread.
        //

        if (FirstChance != FALSE) {

            //
            // This is the first chance to handle the exception.
            //

            if (DbgkForwardException(ExceptionRecord, TRUE, FALSE)) {
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
                // Compute length of exception record and new aligned stack
                // address.
                //

                Length = (sizeof(EXCEPTION_RECORD) + 15) & (~15);
                UserStack1 = (ContextFrame.IntSp & ~((ULONG_PTR)15)) - Length;

                //
                // Probe user stack area for writability and then transfer the
                // exception record to the user stack area.
                //

                ProbeForWrite((PCHAR)UserStack1, Length, sizeof(QUAD));
                RtlMoveMemory((PVOID)UserStack1, ExceptionRecord, Length);

                //
                // Compute length of context record and new aligned user stack
                // pointer.
                //

                Length = (sizeof(CONTEXT) + 15) & (~15);
                UserStack2 = UserStack1 - Length;

                //
                // Probe user stack area for writability and then transfer the
                // context record to the user stack.
                //

                ProbeForWrite((PCHAR)UserStack2, Length, sizeof(QUAD));
                RtlMoveMemory((PVOID)UserStack2, &ContextFrame, sizeof(CONTEXT));

                //
                // Set address of exception record, context record, and the
                // and the new stack pointer in the current trap frame.
                //

                TrapFrame->IntSp = UserStack2;
                TrapFrame->IntFp = UserStack2;
                ExceptionFrame->IntS0 = UserStack1;
                ExceptionFrame->IntS1 = UserStack2;

                //
                // Set the address of the exception routine that will call the
                // exception dispatcher and then return to the trap handler.
                // The trap handler will restore the exception and trap frame
                // context and continue execution in the routine that will
                // call the exception dispatcher.
                //

                TrapFrame->Fir = (ULONGLONG)(LONG_PTR)KeUserExceptionDispatcher;
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

        if (DbgkForwardException(ExceptionRecord, TRUE, TRUE)) {
            goto Handled2;

        } else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE)) {
            goto Handled2;

        } else {
            ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
            KeBugCheckEx(KMODE_EXCEPTION_NOT_HANDLED,
                         ExceptionRecord->ExceptionCode,
                         (ULONG_PTR)ExceptionRecord->ExceptionAddress,
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
    // be transferred to the trap and exception frames.
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

    This function causes an exception to be raised in the calling thread's user-mode
    context. It does this by editing the trap frame the kernel was entered with to
    point to trampoline code that raises the requested exception.

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

    TrapFrame->Fir = (ULONGLONG)(LONG_PTR)KeRaiseUserExceptionDispatcher;
    return(ExceptionCode);
}

#if 0

LOGICAL
BdReportExceptionStateChange (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord
    );

LOGICAL
BdReportLoadSymbolsStateChange (
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN LOGICAL UnloadSymbols,
    IN OUT PCONTEXT ContextRecord
    );

LOGICAL
BdPrintString (
    IN PSTRING Output
    );

LOGICAL
BdPromptString (
    IN PSTRING Output,
    IN OUT PSTRING Input
    );

LOGICAL
BdTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    )

/*++

Routine Description:

    This routine is called whenever a exception is dispatched and the kernel
    debugger is active.

Arguments:

    FirmwareFrame - Supplies a pointer to a firmware frame that describes the
        trap.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    CONTEXT ContextFrame;
    LOGICAL Completion;
    PCONTEXT ContextRecord;
    STRING Input;
    ULONGLONG OldFir;
    STRING Output;
    PKD_SYMBOLS_INFO SymbolInfo;
    LOGICAL UnloadSymbols;

    //
    // Set address of context record and set context flags.
    //

    ContextRecord = &ContextFrame;
    ContextRecord->ContextFlags = CONTEXT_FULL;

    //
    // Print, prompt, load symbols, and unload symbols are all special cases
    // of breakpoint.
    //

//    BlPrint("bd: debug code entered with type %lx, p1 %lx\r\n",
//            (ULONG)FirmwareFrame->Type,
//            (ULONG)FirmwareFrame->Param1);

    if ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) &&
        (ExceptionRecord->ExceptionInformation[0] >= DEBUG_PRINT_BREAKPOINT)) {

        //
        // Switch on the breakpoint code.
        //

        UnloadSymbols = FALSE;
        switch (ExceptionRecord->ExceptionInformation[0]) {

            //
            // Print:
            //
            // Arguments:
            //
            //   a0 - Supplies a pointer to an output string buffer.
            //   a1 - Supplies the length of the output string buffer.
            //

        case DEBUG_PRINT_BREAKPOINT:
//            BlPrint("bd/debug: print\r\n");
            Output.Buffer = (PCHAR)TrapFrame->IntA0;
            Output.Length = (USHORT)TrapFrame->IntA1;
            if (BdPrintString(&Output)) {
                TrapFrame->IntV0 = STATUS_BREAKPOINT;

            } else {
                TrapFrame->IntV0 = STATUS_SUCCESS;
            }

            TrapFrame->Fir += 4;
//            BlPrint("bd/debug: exit - print\r\n");
            KeSweepCurrentIcache();
            return TRUE;

            //
            // Stop in debugger:
            //
            // As this is not a normal breakpoint we must increment the
            // context past the breakpoint instruction.
            //

        case BREAKIN_BREAKPOINT:
            TrapFrame->Fir += 4;
            break;

            //
            // Prompt:
            //
            //   a0 - Supplies a pointer to an output string buffer.
            //   a1 - Supplies the length of the output string buffer..
            //   a2 - supplies a pointer to an input string buffer.
            //   a3 - Supplies the length of the input string bufffer.
            //

        case DEBUG_PROMPT_BREAKPOINT:
//            BlPrint("bd/debug: prompt\r\n");
            Output.Buffer = (PCHAR)TrapFrame->IntA0;
            Output.Length = (USHORT)TrapFrame->IntA1;
            Input.Buffer = (PCHAR)TrapFrame->IntA2;
            Input.MaximumLength = (USHORT)TrapFrame->IntA3;

            //
            // Prompt and keep prompting until no breakin seen.
            //

            do {
            } while(BdPromptString(&Output, &Input) != FALSE);

            TrapFrame->IntV0 = Input.Length;
            TrapFrame->Fir += 4;
//            BlPrint("bd/debug: exit - prompt\r\n");
            KeSweepCurrentIcache();
            return TRUE;

            //
            // Unload Symbols:
            //
            // Arguments:
            //
            //    a0 - Supplies a pointer to the image path string descriptor.
            //    a1 - Supplies a pointer to he symbol information.
            //

        case DEBUG_UNLOAD_SYMBOLS_BREAKPOINT:
//            BlPrint("bd/debug: unload\r\n");
            UnloadSymbols = TRUE;

            //
            // Fall through to load symbol case.
            //

        case DEBUG_LOAD_SYMBOLS_BREAKPOINT:
//            BlPrint("bd/debug: load\r\n");
            KeContextFromKframes(TrapFrame, ExceptionFrame, ContextRecord);
            OldFir = ContextRecord->Fir;
            SymbolInfo = (PKD_SYMBOLS_INFO)ContextRecord->IntA1;
            BdReportLoadSymbolsStateChange((PSTRING)ContextRecord->IntA0,
                                           SymbolInfo,
                                           UnloadSymbols,
                                           ContextRecord);


            //
            // If the kernel debugger did not update the FIR, then increment
            // past the breakpoint instruction.
            //

            if (ContextRecord->Fir == OldFir) {
                ContextRecord->Fir += 4;
            }

            KeContextToKframes(TrapFrame,
                               ExceptionFrame,
                               ContextRecord,
                               ContextRecord->ContextFlags,
                               PreviousMode);

//            BlPrint("bd/debug: exit - load/unload\r\n");
            KeSweepCurrentIcache();
            return TRUE;

            //
            // Unknown internal command.
            //

        default:
            break;
        }
    }

    //
    // Report state change to kernel debugger on host machine.
    //

//    BlPrint("bd/debug: report\r\n");
    KeContextFromKframes(TrapFrame, ExceptionFrame, ContextRecord);
    Completion = BdReportExceptionStateChange(ExceptionRecord,
                                              ContextRecord);

    KeContextToKframes(TrapFrame,
                       ExceptionFrame,
                       ContextRecord,
                       ContextRecord->ContextFlags,
                       PreviousMode);

//    BlPrint("bd/debug: exit - report\r\n");
    KeSweepCurrentIcache();
    return TRUE;
}

LOGICAL
BdStub (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    )

/*++

Routine Description:

    This routine provides a kernel debugger stub routine that catchs debug
    prints in checked systems when the kernel debugger is not active.

Arguments:

    FirmwareFrame - Supplies a pointer to a firmware frame that describes
        the trap.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{

    return FALSE;
}

#endif
