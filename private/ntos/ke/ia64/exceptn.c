/*++

Module Name:

    exceptn.c

Abstract:

    This module implement the code necessary to dispatch expections to the
    proper mode and invoke the exception dispatcher.

Author:

    William K. Cheung (wcheung) 10-Nov-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "ntfpia64.h"

//
// IA64 data misalignment exception (auto alignment fixup) control.
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

#define DEFAULT_NO_ALIGNMENT_FIXUPS FALSE

ULONG KiEnableAlignmentFaultExceptions = DEFAULT_NO_ALIGNMENT_FIXUPS ? 1 : 2;

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
KiRestoreHigherFPVolatile(
    VOID
    );

LONG
fp_emulate (
    ULONG trap_type,
    PVOID pbundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    PVOID fp_state
    );

BOOLEAN
KiEmulateFloat (
    PEXCEPTION_RECORD ExceptionRecord,
    PKEXCEPTION_FRAME ExceptionFrame,
    PKTRAP_FRAME TrapFrame
    )
{

    FLOATING_POINT_STATE FpState;
    USHORT ISRCode;
    ULONG TrapType;
    PVOID ExceptionAddress;
    LONG Status = -1;

    FpState.ExceptionFrame = (PVOID)ExceptionFrame;
    FpState.TrapFrame = (PVOID)TrapFrame;

    if (ExceptionRecord->ExceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) {
        TrapType = 1;
        ExceptionAddress = (PVOID)TrapFrame->StIIP;
    } else {
        TrapType = 0;
        ExceptionAddress = (PVOID)TrapFrame->StIIPA;
    }

    if ((Status = fp_emulate(TrapType, ExceptionAddress,
                      &TrapFrame->StIPSR, &TrapFrame->StFPSR, &TrapFrame->StISR,
                      &TrapFrame->Preds, &TrapFrame->StIFS, (PVOID)&FpState)) == 0) {

       //
       // Exception was handled and state modified.
       // Therefore the context frame does not need to
       // be transfered to the trap and exception frames.
       //
       // Since it was fault, PC should be advanced
       //

       if (TrapType == 1) {
           KiAdvanceInstPointer(TrapFrame);
       }

       if (TrapFrame->StIPSR & (1 << PSR_MFH)) {

           //
           // high fp set is modified, reload high fp register set
           // must prevent interrupt during restore
           //

           __rsm ((1 << PSR_DFH) | (1 << PSR_I));
           KiRestoreHigherFPVolatile();
           __ssm ((1 << PSR_DFH) | (1 << PSR_I));
           __dsrlz();
       }

       return TRUE;
    }

    if (Status == -1) {
        KeBugCheckEx(FP_EMULATION_ERROR, 
                     (ULONG_PTR)TrapFrame, 
                     (ULONG_PTR)ExceptionFrame, 0, 0);
    } 

    ISRCode = (USHORT)TrapFrame->StISR;

    if (Status & 0x1) {

        ExceptionRecord->ExceptionInformation[4] = TrapFrame->StISR;

        if (!(Status & 0x4)) {
            if (TrapType == 1) {

                //
                // FP Fault
                //

                if (ISRCode & 0x11) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
                } else if (ISRCode & 0x22) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_DENORMAL_OPERAND;
                } else if (ISRCode & 0x44) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
                }

            } else {

                //
                // FP Trap
                //

                ISRCode = ISRCode >> 7;
                if (ISRCode & 0x11) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
                } else if (ISRCode & 0x22) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                } else if (ISRCode & 0x44) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                }

            }
        }

        if (Status & 0x2) {

            //
            // FP Fault To Trap
            //

            KiAdvanceInstPointer(TrapFrame);
            ExceptionRecord->ExceptionAddress =
                (PVOID) RtlIa64InsertIPSlotNumber
                            (TrapFrame->StIIP,
                             ((TrapFrame->StISR & ISR_EI_MASK) >> ISR_EI)
                            );
            if (!(Status & 0x4)) {
                ISRCode = ISRCode >> 7;
                if (ISRCode & 0x11) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
                } else if (ISRCode & 0x22) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                } else if (ISRCode & 0x44) {
                    ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                }
            } else {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;
            }
        }
    }

    return FALSE;
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
    STATUS_FLOAT_STACK_CHECK is used to signify this and is converted to the
    proper code by examiningg the main status field of the floating point 
    status register).

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
    EXCEPTION_RECORD ExceptionRecord1;
    PPLABEL_DESCRIPTOR Plabel;
    BOOLEAN UserApcPending;

    //
    // If the exception is a data misalignment, the previous mode was user,
    // this is the first chance for handling the exception, and the current
    // thread has enabled automatic alignment fixup, then attempt to emulate
    // the unaligned reference.
    //

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
                         ExceptionRecord->ExceptionInformation[1],
                         KiKernelFixupCount);
            }

        } else {
            KiUserFixupCount += 1;
            if ((KiUserFixupCount & KiUserFixupMask) == 0) {
                DbgPrint("KI: User  Fixup: Pid=0x%.3lx, Pc=%.16p, Address=%.16p ... Total=%ld\n",
                         PsGetCurrentProcess()->UniqueProcessId,
                         ExceptionRecord->ExceptionAddress,
                         ExceptionRecord->ExceptionInformation[1],
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
                                   TrapFrame) != FALSE) 
            {
                KeGetCurrentPrcb()->KeAlignmentFixupCount += 1;
                goto Handled2;
            }
        }
    }

    //
    // N.B. BREAKIN_BREAKPOINT check is in KdpTrap()
    //

    //
    // If the exception is a floating point exception, then the
    // ExceptionCode was set to STATUS_FLOAT_MULTIPLE_TRAPS or 
    // STATUS_FLOAT_MULTIPLE_FAULTS.
    //

    if ((ExceptionRecord->ExceptionCode == STATUS_FLOAT_MULTIPLE_FAULTS) ||
        (ExceptionRecord->ExceptionCode == STATUS_FLOAT_MULTIPLE_TRAPS)) {

        if (KiEmulateFloat(ExceptionRecord, ExceptionFrame, TrapFrame)) {

            //
            // Emulation is successful; continue execution
            //

            return;
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
            // This is the first chance to handle the exception.
            //
            // Note: RtlpCaptureRnats() flushes the RSE and captures the
            //       Nat bits of stacked registers in the RSE frame at
            //       which exception happens.
            //

            RtlpCaptureRnats(&ContextFrame);
            TrapFrame->RsRNAT = ContextFrame.RsRNAT;

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
                     (ULONG_PTR)ExceptionRecord->ExceptionAddress,
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
            // This is the first chance to handle the exception.
            //

            if (DbgkForwardException(ExceptionRecord, TRUE, FALSE)) {
                TrapFrame->StFPSR = SANITIZE_FSR(TrapFrame->StFPSR, UserMode);
                goto Handled2;
            }

            //
            // Transfer exception information to the user stack, transition
            // to user mode, and attempt to dispatch the exception to a frame
            // based handler.
            //
            //
            // We are running on the kernel stack now.  On the user stack, we
            // build a stack frame containing the following:
            //
            //               |                                   |
            //               |-----------------------------------|
            //               |                                   |
            //               |   User's stack frame              |
            //               |                                   |
            //               |-----------------------------------|
            //               |                                   |
            //               |   Context record                  |
            //               |                                   |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |                                   |
            //               |   Exception record                |
            //               |                                   |
            //               |- - - - - - - - - - - - - - - - - -|
            //               |   Stack Scratch Area              |
            //               |-----------------------------------|
            //               |                                   |
            //
            // This stack frame is for KiUserExceptionDispatcher, the assembly
            // langauge routine that effects transfer in user mode to
            // RtlDispatchException.  KiUserExceptionDispatcher is passed
            // pointers to the Exception Record and Context Record as
            // parameters.
            //

        repeat:
            try {

                //
                // Compute length of exception record and new aligned stack
                // address.
                //

                ULONG Length = (STACK_SCRATCH_AREA + 15 +
                                sizeof(EXCEPTION_REGISTRATION_RECORD) +
                                sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)) & ~(15);
                ULONGLONG UserStack = (ContextFrame.IntSp & (~15)) - Length;
                ULONGLONG ContextSlot = UserStack + STACK_SCRATCH_AREA;
                ULONGLONG ExceptSlot = ContextSlot + sizeof(CONTEXT);
                PULONGLONG PUserStack = (PULONGLONG) UserStack;
                
                //
                // Probe user stack area for writeability and then transfer the
                // exception record and conext record to the user stack area.
                //

                ProbeForWrite((PCHAR)UserStack, Length, sizeof(QUAD));
                RtlMoveMemory((PVOID)ContextSlot, &ContextFrame, 
                              sizeof(CONTEXT));
                RtlMoveMemory((PVOID)ExceptSlot, ExceptionRecord, 
                              sizeof(EXCEPTION_RECORD));

                //
                // Set address of exception record and context record in
                // the exception frame and the new stack pointer in the 
                // current trap frame.  Also set the initial frame size
                // to be zero.
                //
                // N.B. User exception dispatcher flushes the RSE
                //      and updates the BSPStore field upon entry.
                //

                TrapFrame->RsPFS = TrapFrame->StIFS;
                TrapFrame->StIFS &= 0xffffffc000000000;
                TrapFrame->StIPSR &= ~((0x3i64 << PSR_RI) | (0x1i64 << PSR_IS));
                TrapFrame->IntSp = UserStack;
                TrapFrame->IntNats = 0;

                ExceptionFrame->IntS0 = ExceptSlot;
                ExceptionFrame->IntS1 = ContextSlot;
                ExceptionFrame->IntNats = 0;

                //
                // Set the address and the gp of the exception routine that 
                // will call the exception dispatcher and then return to the 
                // trap handler.  The trap handler will restore the exception 
                // and trap frame context and continue execution in the routine
                // that will call the exception dispatcher.
                //

                Plabel = (PPLABEL_DESCRIPTOR)KeUserExceptionDispatcher;
                TrapFrame->StIIP = Plabel->EntryPoint;
                TrapFrame->IntGp = Plabel->GlobalPointer;

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
            TrapFrame->StFPSR = SANITIZE_FSR(TrapFrame->StFPSR, UserMode);
            goto Handled2;

        } else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE)) {
            TrapFrame->StFPSR = SANITIZE_FSR(TrapFrame->StFPSR, UserMode);
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

    ASSERT(KeGetPreviousMode() == UserMode);

    TrapFrame = KeGetCurrentThread()->TrapFrame;

    try {
        ProbeForWrite ((PVOID)TrapFrame->IntSp, 16, sizeof(QUAD));
        *(PULONGLONG)TrapFrame->IntSp = TrapFrame->BrRp;
        *(PULONGLONG)(TrapFrame->IntSp + 8) = TrapFrame->RsPFS;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return (ExceptionCode);
    }

    TrapFrame->StIIP = ((PPLABEL_DESCRIPTOR)KeRaiseUserExceptionDispatcher)->EntryPoint;
    TrapFrame->StIFS &= (0x3i64 << 62);
    return(ExceptionCode);
}
