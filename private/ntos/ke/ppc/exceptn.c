/*++

Copyright (c) 1993  IBM Corporation and Microsoft Corporation

Module Name:

    exceptn.c

Abstract:

    This module implements the code necessary to dispatch expections to the
    proper mode and invoke the exception dispatcher.

Author:

    Rick Simpson   2-Aug-1993
    Adapted from MIPS version by David N. Cutler (davec) 3-Apr-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#define _KXPPC_C_HEADER_
#include "kxppc.h"

BOOLEAN
KiEmulateDcbz (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    );

//
// Data misalignment exception (auto alignment fixup) control.
//
// If KiEnableAlignmentFaultExceptions is false, then no alignment
// exceptions are raised and all misaligned user and kernel mode data
// references are emulated.
//
// Otherwise if KiEnableAlignmentFaultExceptions is true, then the
// current thread automatic alignment fixup enable determines whether
// emulation is attempted in user mode.
//
// N.B. This default value may be reset from the Registry during init.
//

ULONG KiEnableAlignmentFaultExceptions = TRUE;

//
// Breakpoint is a trap word immediate with a TO field of all ones.
//

#define BREAK_INST  (TRAP_INSTR | TO_BREAKPOINT)

//
// Define multiply overflow and divide by zero breakpoint instruction values.
//

#define DIVIDE_BREAKPOINT   (TRAP_INSTR | TO_DIVIDE_BY_ZERO)
#define UDIVIDE_BREAKPOINT  (TRAP_INSTR | TO_UNCONDITIONAL_DIVIDE_BY_ZERO)

//
// Define external kernel breakpoint and breakin breakpoint instructions.
//

#define KERNEL_BREAKPOINT_INSTRUCTION (BREAK_INSTR | DEBUG_STOP_BREAKPOINT)
#define KDDEBUG_BREAKPOINT  (BREAK_INSTR | BREAKIN_BREAKPOINT)

//
// Define available hardware breakpoint register mask
//
ULONG KiBreakPoints;

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

    //
    // Set control information if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set machine state, instr address, link, count registers
        //

        ContextFrame->Msr = TrapFrame->Msr;
        ContextFrame->Iar = TrapFrame->Iar;
        ContextFrame->Lr  = TrapFrame->Lr;
        ContextFrame->Ctr = TrapFrame->Ctr;
    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Volatile integer regs in trap frame are 0..12
        //

        RtlMoveMemory (&ContextFrame->Gpr0, &TrapFrame->Gpr0,
                       sizeof (ULONG) * 13);

        //
        // Non-volatile integer regs in exception frame are 13..31
        //

        RtlMoveMemory (&ContextFrame->Gpr13, &ExceptionFrame->Gpr13,
                       sizeof (ULONG) * 19);

        //
        // The CR is made up of volatile and non-volatile fields,
        // but the entire CR is saved in the trap frame
        //

        ContextFrame->Cr = TrapFrame->Cr;

        //
        // Fixed Point Exception Register (XER) is part of the
        // integer state
        //

        ContextFrame->Xer = TrapFrame->Xer;
    }

    //
    // Set floating register contents if specified.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Volatile floating point regs in trap frame are 0..13
        //

        RtlMoveMemory(&ContextFrame->Fpr0, &TrapFrame->Fpr0,
                     sizeof(DOUBLE) * (14));

        //
        // Non-volatile floating point regs in exception frame are 14..31
        //

        RtlMoveMemory(&ContextFrame->Fpr14, &ExceptionFrame->Fpr14,
                     sizeof(DOUBLE) * (18));

        //
        // Set floating point status and control register.
        //

        ContextFrame->Fpscr = TrapFrame->Fpscr;
    }

    //
    // Fetch Dr register contents if requested.  Values may be trash.
    //

    if ((ContextFrame->ContextFlags & CONTEXT_DEBUG_REGISTERS) ==
        CONTEXT_DEBUG_REGISTERS) {

        ContextFrame->Dr0 = TrapFrame->Dr0;
        ContextFrame->Dr1 = TrapFrame->Dr1;
        ContextFrame->Dr2 = TrapFrame->Dr2;
        ContextFrame->Dr3 = TrapFrame->Dr3;
        ContextFrame->Dr6 = TrapFrame->Dr6;
        ContextFrame->Dr6 |= KiBreakPoints;
        ContextFrame->Dr5 = 0;            // Zero initialize unused regs
        ContextFrame->Dr4 = 0;

        //
        // If it's a user mode frame, and the thread doesn't have DRs set,
        // and we just return the trash in the frame, we risk accidentally
        // making the thread active with trash values on a set.  Therefore,
        // Dr7 must be set to the number of available data address breakpoint
        // registers if we get a non-active user mode frame.
        //

        if (((TrapFrame->PreviousMode) != KernelMode) &&
            (KeGetCurrentThread()->DebugActive)) {

            ContextFrame->Dr7 = TrapFrame->Dr7;
        } else {

            ContextFrame->Dr7 = 0;
        }
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
        // Set instruction address, link, count, and machine state registers
        //

        TrapFrame->Iar = ContextFrame->Iar;
        TrapFrame->Lr  = ContextFrame->Lr;
        TrapFrame->Ctr = ContextFrame->Ctr;
        TrapFrame->Msr = SANITIZE_MSR(ContextFrame->Msr, PreviousMode);
    }

    //
    // Set integer registers contents if specified.
    //

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Volatile integer regs are 0..12
        //

        RtlMoveMemory(&TrapFrame->Gpr0, &ContextFrame->Gpr0,
                     sizeof(ULONG) * (13));

        //
        // Non-volatile integer regs are 13..31
        //

        RtlMoveMemory(&ExceptionFrame->Gpr13, &ContextFrame->Gpr13,
                     sizeof(ULONG) * (19));

        //
        // Copy the Condition Reg and Fixed Point Exception Reg
        //

        TrapFrame->Cr = ContextFrame->Cr;
        TrapFrame->Xer = ContextFrame->Xer;
    }

    //
    // Set floating register contents if specified.
    //

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Volatile floating point regs are 0..13
        //

        RtlMoveMemory(&TrapFrame->Fpr0, &ContextFrame->Fpr0,
                     sizeof(DOUBLE) * (14));

        //
        // Non-volatile floating point regs are 14..31
        //

        RtlMoveMemory(&ExceptionFrame->Fpr14, &ContextFrame->Fpr14,
                     sizeof(DOUBLE) * (18));

        //
        // Set floating point status and control register.
        //

        TrapFrame->Fpscr = SANITIZE_FPSCR(ContextFrame->Fpscr, PreviousMode);
    }

    //
    // Set debug register state if specified.  If previous mode is user
    // mode (i.e. it's a user frame we're setting) and if effect will be to
    // cause at least one of the debug register enable bits in Dr7
    // to be set then set DebugActive to the enable bit mask.
    //

    if ((ContextFlags & CONTEXT_DEBUG_REGISTERS) == CONTEXT_DEBUG_REGISTERS) {

        //
        // Set the debug control register for the 601 and 604
        // indicating the number of address breakpoints supported.
        //

        TrapFrame->Dr0 = SANITIZE_DRADDR(ContextFrame->Dr0, PreviousMode);
        TrapFrame->Dr1 = SANITIZE_DRADDR(ContextFrame->Dr1, PreviousMode);
        TrapFrame->Dr2 = SANITIZE_DRADDR(ContextFrame->Dr2, PreviousMode);
        TrapFrame->Dr3 = SANITIZE_DRADDR(ContextFrame->Dr3, PreviousMode);
        TrapFrame->Dr6 = SANITIZE_DR6(ContextFrame->Dr6, PreviousMode);
        TrapFrame->Dr7 = SANITIZE_DR7(ContextFrame->Dr7, PreviousMode);

        if (PreviousMode != KernelMode) {
              KeGetPcr()->DebugActive = KeGetCurrentThread()->DebugActive =
                                        (UCHAR)(TrapFrame->Dr7 & DR7_ACTIVE);
        }
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

    If the exception is a data misalignment, this is the first chance for
    handling the exception, and the current thread has enabled automatic
    alignment fixup, then an attempt is made to emulate the unaligned
    reference.

    If the exception is a floating exception (N.B. the pseudo status
    STATUS_FLOAT_STACK_CHECK is used to signify this), we convert the
    exception code to the correct STATUS based on the FPSCR.
    It is up to the handler to figure out what to do to emulate/repair
    the operation.

    If the exception is neither a data misalignment nor a floating point
    exception and the the previous mode is kernel, then the exception
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
    LONG Length;
    BOOLEAN UserApcPending;

    //
    // If the exception is a data misalignment, this is the first chance for
    // handling the exception, and the current thread has enabled automatic
    // alignment fixup, then attempt to emulate the unaligned reference.
    //
    // We always emulate dcbz, even if the thread hasn't enabled automatic
    // alignment fixup.  This is because the hardware declares an alignment
    // fault if dcbz is attempted on noncached memory.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_DATATYPE_MISALIGNMENT) {
        if (FirstChance != FALSE) {

            //
            // If alignment fault exceptions are not enabled, then no exception
            // should be raised and the data reference should be emulated.
            //

            if ((KiEnableAlignmentFaultExceptions == FALSE) ||
                (KeGetCurrentThread()->AutoAlignment != FALSE) ||
                (KeGetCurrentThread()->ApcState.Process->AutoAlignment != FALSE)) {
                if (KiEmulateReference(ExceptionRecord, ExceptionFrame, TrapFrame) != FALSE) {
                    KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
                    goto Handled2;
                }
            } else {
                if (KiEmulateDcbz(ExceptionRecord, ExceptionFrame, TrapFrame) != FALSE) {
                    KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
                    goto Handled2;
                }
            }
        }
    }

    //
    // If the exception is a breakpoint, then translate it to an appropriate
    // exception code if it is a division by zero or an integer overflow
    // caused by multiplication.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) {

        ULONG Instr = ExceptionRecord->ExceptionInformation[0];

        if ((Instr & 0xffe0ffff) == DIVIDE_BREAKPOINT ||
            (Instr & 0xffe0ffff) == UDIVIDE_BREAKPOINT) {
            ExceptionRecord->ExceptionCode = STATUS_INTEGER_DIVIDE_BY_ZERO;
        } else if (Instr == KDDEBUG_BREAKPOINT) {
            TrapFrame->Iar += 4;
        }
    }

    //
    // If the exception is a floating point exception, then the
    // ExceptionCode was set to STATUS_FLOAT_STACK_CHECK.  We now sort
    // that out and set a more correct STATUS code.  We clear the
    // exception enable bit in the FPSCR of the exception being reported
    // to eliminate floating point exception recursion.
    //

    if (ExceptionRecord->ExceptionCode == STATUS_FLOAT_STACK_CHECK) {

        PFPSCR Fpscr = (PFPSCR)(&TrapFrame->Fpscr);

        if ((Fpscr->XE == 1) && (Fpscr->XX == 1)) {

            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            Fpscr->XE = 0;

        }
        else if ((Fpscr->ZE == 1) && (Fpscr->ZX == 1)) {

            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            Fpscr->ZE = 0;

        }
        else if ((Fpscr->UE == 1) && (Fpscr->UX == 1)) {

            ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
            Fpscr->UE = 0;

        }

        else if ((Fpscr->OE == 1) && (Fpscr->OX == 1)) {

            ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
            Fpscr->OE = 0;

        }
        else {

            // Must be some form of Invalid Operation

            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            Fpscr->VE = 0;
        }
    }

    //
    // Move machine state from trap and exception frames to a context frame,
    // and increment the number of exceptions dispatched.
    //

    ContextFrame.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
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
               ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) ||
                (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP))  &&
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
                ((ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) ||
                (ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)) &&
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
                TrapFrame->Fpscr = SANITIZE_FPSCR(TrapFrame->Fpscr, UserMode);
                goto Handled2;
            }

            //
            // Transfer exception information to the user stack, transition
            // to user mode, and attempt to dispatch the exception to a frame
            // based handler.
            //
            // We are running on the kernel stack now.  On the user stack, we
            // build a stack frame containing the following:
            //
            //               |                                   |
            //               |-----------------------------------|
            //               |                                   |
            //               |   Stack frame header              |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |                                   |
            //               |   Exception record                |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |                                   |
            //               |   Context record                  |
            //               |                                   |
            //               |                                   |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |   Saved TOC for backtrack         |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |                                   |
            //               |                                   |
            //               |   STK_SLACK_SPACE                 |
            //               |                                   |
            //               |                                   |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |                                   |
            //               |   User's stack frame              |
            //               |                                   |
            //               |                                   |
            //
            // This stack frame is for KiUserExceptionDispatcher, the assembly
            // langauge routine that effects transfer in user mode to
            // RtlDispatchException.  KiUserExceptionDispatcher is passed
            // pointers to the Exception Record and Context Record as
            // parameters.

        repeat:
            try {

                //
                // Compute positions on user stack of items shown above
                //

                ULONG Length = (sizeof (STACK_FRAME_HEADER) + sizeof (EXCEPTION_RECORD) +
                                sizeof (CONTEXT) + sizeof (ULONG) + STK_SLACK_SPACE + 7) & (~7);

                ULONG UserStack = (ContextFrame.Gpr1 & (~7)) - Length;
                ULONG ExceptSlot = UserStack + sizeof (STACK_FRAME_HEADER);
                ULONG ContextSlot = ExceptSlot + sizeof (EXCEPTION_RECORD);
                ULONG TocSlot = ContextSlot + sizeof (CONTEXT);

                //
                // Probe user stack area for writeability and then transfer the
                // exception record and context record to the user stack area.
                //

                ProbeForWrite((PCHAR) UserStack, ContextFrame.Gpr1 - UserStack, sizeof(QUAD));
                RtlMoveMemory((PVOID) ExceptSlot, ExceptionRecord, sizeof (EXCEPTION_RECORD));
                RtlMoveMemory((PVOID) ContextSlot, &ContextFrame, sizeof (CONTEXT));

                //
                // Fill in TOC value as if it had been saved by prologue to
                // KiUserExceptionDispatcher
                //

                *((PULONG) TocSlot) = ContextFrame.Gpr2;

                //
                // Set back chain from newly-constructed stack frame
                //

                *((PULONG) UserStack) = ContextFrame.Gpr1;

                //
                // Set address of exception record, context record,
                // and the new stack pointer in the current trap frame.
                //

                TrapFrame->Gpr1 = UserStack;        // Stack pointer
                TrapFrame->Gpr3 = ExceptSlot;       // First parameter
                TrapFrame->Gpr4 = ContextSlot;      // Second parameter

                //
                // Sanitize the floating status register so a recursive
                // exception will not occur.
                //

                TrapFrame->Fpscr = SANITIZE_FPSCR(ContextFrame.Fpscr, UserMode);

                //
                // Set the execution address and TOC pointer of the exception
                // routine that will call the exception dispatcher and then return
                // to the trap handler.  The trap handler will restore the exception
                // and trap frame context and continue execution in the routine
                // that will call the exception dispatcher.
                //

                {
                    PULONG FnDesc = (PULONG) KeUserExceptionDispatcher;
                    TrapFrame->Iar = FnDesc[0];
                    TrapFrame->Gpr2 = FnDesc[1];
                }

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
            TrapFrame->Fpscr = SANITIZE_FPSCR(TrapFrame->Fpscr, UserMode);
            goto Handled2;

        } else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE)) {
            //
            // If a user APC was not previously pending and one is now
            // pending, then the thread has been terminated and the PC
            // must be forced to a legal address so an infinite loop does
            // not occur for the case where a jump to an unmapped address
            // occurred.
            //

            if ((UserApcPending == FALSE) &&
                (KeGetCurrentThread()->ApcState.UserApcPending != FALSE)) {
// TEMPORARY .... PAT
// Commenting out reference to USPCR (a known legal address ..
//              TrapFrame->Iar = (ULONG)USPCR;
            }

            TrapFrame->Fpscr = SANITIZE_FPSCR(TrapFrame->Fpscr, UserMode);
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
    PULONG FnDesc;

    ASSERT(KeGetPreviousMode() == UserMode);

    TrapFrame = KeGetCurrentThread()->TrapFrame;
    FnDesc = (PULONG)KeRaiseUserExceptionDispatcher;

    TrapFrame->Iar = FnDesc[0];
    TrapFrame->Gpr2 = FnDesc[1];

    return(ExceptionCode);
}
