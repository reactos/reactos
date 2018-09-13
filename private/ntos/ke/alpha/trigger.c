/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    trigger.c

Abstract:

    This module implements functions that handle synchronous and asynchronous
    arithmetic exceptions. The Alpha SRM specifies certain code generation
    rules which if followed allow this code (in conjunction with internal
    processor register state) to effect a precise, synchronous exception
    given an imprecise, asynchronous exception. This capability is required
    for software emulation of the IEEE single and double floating operations.

Author:

    Thomas Van Baak (tvb) 5-Mar-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#include "alphaops.h"

//
// Define forward referenced function prototypes.
//

BOOLEAN
KiLocateTriggerPc (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKTRAP_FRAME TrapFrame
    );

//
// Define debugging macros.
//

#if DBG

extern ULONG RtlDebugFlags;
#define DBGPRINT ((RtlDebugFlags & 0x4) != 0) && DbgPrint

#else

#define DBGPRINT 0 && DbgPrint

#endif

//
// Define non-IEEE (a/k/a `high performance') arithmetic exception types.
// The PALcode exception record is extended by one word and the 4th word
// contains the reason the arithmetic exception is not an IEEE exception.
//

#define NON_IEEE(ExceptionRecord, Reason) \
    (ExceptionRecord)->NumberParameters = 4; \
    (ExceptionRecord)->ExceptionInformation[3] = (Reason);

#define TRIGGER_FLOATING_REGISTER_MASK_CLEAR 1
#define TRIGGER_INTEGER_REGISTER_MASK_SET 2
#define TRIGGER_NO_SOFTWARE_COMPLETION 3
#define TRIGGER_INVALID_INSTRUCTION_FOUND 4
#define TRIGGER_INSTRUCTION_FETCH_ERROR 5
#define TRIGGER_INSTRUCTION_NOT_FOUND 6
#define TRIGGER_SOURCE_IS_DESTINATION 7
#define TRIGGER_WRONG_INSTRUCTION 8

BOOLEAN
KiFloatingException (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame,
    IN BOOLEAN ImpreciseTrap,
    IN OUT PULONG SoftFpcrCopy
    )

/*++

Routine Description:

    This function is called to emulate a floating operation and convert the
    exception status to the proper value. If the exception is a fault, the
    faulting floating point instruction is emulated. If the exception is an
    imprecise trap, an attempt is made to locate and to emulate the original
    trapping floating point instruction.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

    ImpreciseTrap - Supplies a boolean value that specifies whether the
        exception is an imprecise trap.

    SoftFpcrCopy - Supplies a pointer to a longword variable that receives
        a copy of the software FPCR.

Return Value:

    A value of TRUE is returned if the floating exception is successfully
    emulated. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Status;
    PSW_FPCR SoftwareFpcr;
    PTEB Teb;

    try {

        //
        // Obtain a copy of the software FPCR longword from the TEB.
        //

        Teb = NtCurrentTeb();
        *SoftFpcrCopy = Teb->FpSoftwareStatusRegister;
        SoftwareFpcr = (PSW_FPCR)SoftFpcrCopy;
        DBGPRINT("KiFloatingException: SoftFpcr = %.8lx\n", *SoftFpcrCopy);

#if DBG
        //
        // If the floating emulation inhibit flag is set, then bypass all
        // software emulation by the kernel and return FALSE to raise the
        // original PALcode exception.
        //
        // N.B. This is for user-mode development and testing and is not
        //      part of the API.
        //

        if (SoftwareFpcr->NoSoftwareEmulation != 0) {
            DBGPRINT("KiFloatingException: NoSoftwareEmulation\n");
            return FALSE;
        }
#endif

        //
        // If the arithmetic exception is an imprecise trap, the address of
        // the trapping instruction is somewhere before the exception address.
        //
        // Otherwise the exception is a fault and the address of the faulting
        // instruction is the exception address.
        //

        if (ImpreciseTrap != FALSE) {

            //
            // If the arithmetic trap ignore mode is enabled, then do not
            // spend time to locate or to emulate the trapping instruction,
            // leave unpredictable results in the destination register, do
            // not set correct IEEE sticky bits in the software FPCR, leave
            // the hardware FPCR sticky status bits as they are, and return
            // TRUE to continue execution. It is assumed that user code will
            // check the hardware FPCR exception status bits to determine if
            // the instruction succeeded or not (Insignia SoftPc feature).
            //

            if (SoftwareFpcr->ArithmeticTrapIgnore != 0) {
                return TRUE;
            }

            //
            // Attempt to locate the trapping instruction. If the instruction
            // stream is such that this is not possible or was not intended,
            // then set an exception code that best reflects the exception
            // summary register bits and return FALSE to raise the exception.
            //
            // Otherwise emulate the trigger instruction in order to compute
            // the correct destination result value, the correct IEEE status
            // bits, and raise any enabled IEEE exceptions.
            //

            if (KiLocateTriggerPc(ExceptionRecord, TrapFrame) == FALSE) {
                KiSetFloatingStatus(ExceptionRecord);
                return FALSE;
            }
            Status = KiEmulateFloating(ExceptionRecord,
                                       ExceptionFrame,
                                       TrapFrame,
                                       SoftwareFpcr);

        } else {

            //
            // Attempt to emulate the faulting instruction in order to perform
            // floating operations not supported by EV4, to compute the correct
            // destination result value, the correct IEEE status bits, and
            // raise any enabled IEEE exceptions.
            //

            Status = KiEmulateFloating(ExceptionRecord,
                                       ExceptionFrame,
                                       TrapFrame,
                                       SoftwareFpcr);

            //
            // If the emulation resulted in a floating point exception and
            // the arithmetic trap ignore mode is enabled, then set the return
            // value to TRUE to suppress the exception and continue execution.
            //

            if ((Status == FALSE) &&
                (SoftwareFpcr->ArithmeticTrapIgnore != 0) &&
                (ExceptionRecord->ExceptionCode != STATUS_ILLEGAL_INSTRUCTION)) {
                Status = TRUE;
            }
        }

        //
        // Store the updated software FPCR longword in the TEB.
        //

        Teb->FpSoftwareStatusRegister = *SoftFpcrCopy;
        DBGPRINT("KiFloatingException: SoftFpcr = %.8lx\n", *SoftFpcrCopy);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception occurred accessing the TEB.
        //

        ExceptionRecord->ExceptionCode = GetExceptionCode();
        return FALSE;
    }

    return Status;
}

BOOLEAN
KiLocateTriggerPc (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to try to determine the precise location of the
    instruction that caused an arithmetic exception. The instruction that
    caused the trap to occur is known as the trigger instruction. On entry,
    the actual address of the trigger instruction is unknown and the exception
    address is the continuation address. The continuation address is the
    address of the instruction that would have executed had the trap not
    occurred. The instructions following the trigger instruction up to the
    continuation address are known as the trap shadow of the trigger
    instruction.

    Alpha AXP produces imprecise, asynchronous arithmetic exceptions. The
    exceptions are imprecise because the exception address when a trap is
    taken may be more than one instruction beyond the address of the
    instruction that actually caused the trap to occur.

    The arithmetic exceptions are traps (rather than faults) because the
    exception address is not the address of the trapping instruction
    itself, but the address of the next instruction to execute, which is
    always (at least) one instruction beyond the address of the trapping
    instruction.

    It is possible for multiple exceptions to occur and result in a single
    trap. This function only determines the address of the first trapping
    instruction.

    Unpredictable values may have been stored in the destination register
    of trapping instructions. Thus to insure that the trigger instruction
    can be located, and that the trigger instruction and any instructions
    in the trap shadow can be re-executed, certain restrictions are placed
    on the type of instructions or the mix of operands in the trap shadow.

    The code generation rules serve only to guarantee that the instruction
    backup algorithm and subsequent re-execution can always be successful.
    Hence the restrictions on such constructs as branches, jumps, and the
    re-use of source or destination operands within the trap shadow.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    If the trigger PC was precisely determined, the exception address in
    the exception record is set to the trigger PC, the continuation address
    in the trap frame is updated, and a value of TRUE is returned. Otherwise
    no values are stored and a value of FALSE is returned.

--*/

{

    PEXC_SUM ExceptionSummary;
    ULONG Fa;
    ULONG Fb;
    ULONG Fc;
    ULONG FloatRegisterTrashMask;
    ULONG FloatRegisterWriteMask;
    ALPHA_INSTRUCTION Instruction;
    ULONG IntegerRegisterWriteMask;
    ULONG Opcode;
    ULONG_PTR TrapShadowLowLimit;
    ULONG_PTR TriggerPc;
    KPROCESSOR_MODE PreviousMode;

    //
    // Obtain a copy of the float and integer register write mask registers
    // and the exception summary register from the exception record built by
    // PALcode.
    //

    FloatRegisterWriteMask = (ULONG)ExceptionRecord->ExceptionInformation[0];
    IntegerRegisterWriteMask = (ULONG)ExceptionRecord->ExceptionInformation[1];
    ExceptionSummary = (PEXC_SUM)&(ExceptionRecord->ExceptionInformation[2]);
    DBGPRINT("KiLocateTriggerPc: WriteMask %.8lx.%.8lx, ExceptionSummary %.8lx\n",
             FloatRegisterWriteMask, IntegerRegisterWriteMask,
             *(PULONG)ExceptionSummary);

    //
    // Capture previous mode from trap frame not current thread.
    //

    PreviousMode = (KPROCESSOR_MODE)(((PSR *)(&TrapFrame->Psr))->MODE);

    if (FloatRegisterWriteMask == 0) {

        //
        // It should not be possible to have a floating point exception without
        // at least one of the destination float register bits set. The trap
        // shadow is invalid.
        //

        DBGPRINT("KiLocateTriggerPc: FloatRegisterWriteMask clear\n");
        NON_IEEE(ExceptionRecord, TRIGGER_FLOATING_REGISTER_MASK_CLEAR);
        return FALSE;
    }
    if (IntegerRegisterWriteMask != 0) {

        //
        // It is not possible to precisely locate the trigger instruction
        // when the integer overflow bit is set. The trap shadow is invalid.
        //

        DBGPRINT("KiLocateTriggerPc: IntegerRegisterMask set.\n");
        NON_IEEE(ExceptionRecord, TRIGGER_INTEGER_REGISTER_MASK_SET);
        return FALSE;
    }
    if (ExceptionSummary->SoftwareCompletion == 0) {

        //
        // The exception summary software completion bit is the AND of the
        // /S bits of all trapping instructions in the trap shadow. Since
        // the software completion bit is not set, it can be assumed the
        // code that was executing does not want precise exceptions, or if
        // it does, the code does not comply with the Alpha AXP guidelines
        // for locating the trigger PC. The trap shadow is invalid.
        //

        DBGPRINT("KiLocateTriggerPc: SoftwareCompletion clear\n");
        NON_IEEE(ExceptionRecord, TRIGGER_NO_SOFTWARE_COMPLETION);
        return FALSE;
    }

    //
    // Search for the trigger instruction starting with the instruction before
    // the continuation PC (the instruction pointed to by Fir either did not
    // complete or did not even start). Limit the search to the arbitrary
    // limit of N instructions back from the current PC to prevent unbounded
    // searches. The search is complete when all trapping destination register
    // bits in the float write mask register have been accounted for.
    //

    FloatRegisterTrashMask = 0;
    TriggerPc = (ULONG_PTR)TrapFrame->Fir;
    TrapShadowLowLimit = TriggerPc - (500 * sizeof(ULONG));

    try {
        do {
            TriggerPc -= 4;
            if (TriggerPc < TrapShadowLowLimit) {

                //
                // The trigger PC is too far away from the exception PC to
                // be reasonable. The trap shadow is invalid.
                //

                DBGPRINT("KiLocateTriggerPc: Trap shadow too long\n");
                NON_IEEE(ExceptionRecord, TRIGGER_INSTRUCTION_NOT_FOUND);
                return FALSE;
            }

            if (PreviousMode != KernelMode) {
                Instruction.Long = ProbeAndReadUlong((PULONG)TriggerPc);
            } else {
                Instruction.Long = *((PULONG)TriggerPc);
            }

            //
            // Examine the opcode of this instruction to determine if the
            // trap shadow is invalid.
            //

            Opcode = Instruction.Memory.Opcode;
            if (Opcode == JMP_OP) {

                //
                // This is one of the jump instructions: jump, return, or
                // either form of jsr. The trap shadow is invalid.
                //

                DBGPRINT("KiLocateTriggerPc: Jump within Trap Shadow\n");
                NON_IEEE(ExceptionRecord, TRIGGER_INVALID_INSTRUCTION_FOUND);
                return FALSE;

            } else if ((Opcode >= BR_OP) && (Opcode <= BGT_OP)) {

                //
                // The instruction is one of 16 branch opcodes that consists
                // of BR, the 6 floating point branch, BSR, and the 8 integer
                // branch instructions. The trap shadow is invalid.
                //

                DBGPRINT("KiLocateTriggerPc: Branch within Trap Shadow\n");
                NON_IEEE(ExceptionRecord, TRIGGER_INVALID_INSTRUCTION_FOUND);
                return FALSE;

            } else if ((Instruction.Memory.Opcode == MEMSPC_OP) &&
                ((Instruction.Memory.MemDisp == TRAPB_FUNC) ||
                 (Instruction.Memory.MemDisp == EXCB_FUNC))) {

                //
                // The instruction is a type of TRAPB instruction. The trap
                // shadow is invalid.
                //

                DBGPRINT("KiLocateTriggerPc: Trapb within Trap Shadow\n");
                NON_IEEE(ExceptionRecord, TRIGGER_INVALID_INSTRUCTION_FOUND);
                return FALSE;

            } else if (Opcode == CALLPAL_OP) {

                //
                // The instruction is a Call PAL. The trap shadow is invalid.
                //

                DBGPRINT("KiLocateTriggerPc: Call PAL within Trap Shadow\n");
                NON_IEEE(ExceptionRecord, TRIGGER_INVALID_INSTRUCTION_FOUND);
                return FALSE;

            } else if ((Opcode == IEEEFP_OP) || (Opcode == FPOP_OP)) {

                //
                // The instruction is an IEEE floating point instruction.
                // Decode the destination register of the floating point
                // instruction in order to check against the register mask.
                //

                Fc = Instruction.FpOp.Fc;
                if (Fc != FZERO_REG) {
                    FloatRegisterTrashMask |= (1 << Fc);
                }
                FloatRegisterWriteMask &= ~(1 << Fc);
            }

        } while (FloatRegisterWriteMask != 0);

        //
        // If the instruction thought to be the trigger instruction does not
        // have the /S bit set, then the trap shadow is invalid (some other
        // instruction must have caused software completion bit to be set).
        //

        if ((Instruction.FpOp.Function & FP_TRAP_ENABLE_S) == 0) {
            DBGPRINT("KiLocateTriggerPc: Trigger instruction missing /S\n");
            NON_IEEE(ExceptionRecord, TRIGGER_WRONG_INSTRUCTION);
            return FALSE;
        }

        //
        // If either of the operand registers of the trigger instruction is
        // also the destination register of the trigger instruction or any
        // instruction in the trap shadow, then the trap shadow in invalid.
        // This is because the original value of the operand register(s) may
        // have been destroyed making it impossible to re-execute the trigger
        // instruction.
        //

        Fa = Instruction.FpOp.Fa;
        Fb = Instruction.FpOp.Fb;
        if ((FloatRegisterTrashMask & ((1 << Fa) | (1 << Fb))) != 0) {
            DBGPRINT("KiLocateTriggerPc: Source is destination\n");
            NON_IEEE(ExceptionRecord, TRIGGER_SOURCE_IS_DESTINATION);
            return FALSE;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception occurred while fetching the value of the
        // next previous instruction. The trap shadow is invalid.
        //

        DBGPRINT("KiLocateTriggerPc: Instruction fetch error\n");
        NON_IEEE(ExceptionRecord, TRIGGER_INSTRUCTION_FETCH_ERROR);
        return FALSE;
    }

    //
    // The trigger instruction was successfully located. Set the precise
    // exception address in the exception record, set the new continuation
    // address in the trap frame, and return a value of TRUE.
    //

    DBGPRINT("KiLocateTriggerPc: Exception PC = %p, Trigger PC = %p\n",
             ExceptionRecord->ExceptionAddress, TriggerPc);
    ExceptionRecord->ExceptionAddress = (PVOID)TriggerPc;
    TrapFrame->Fir = (ULONGLONG)(LONG_PTR)(TriggerPc + 4);
    return TRUE;
}

VOID
KiSetFloatingStatus (
    IN OUT PEXCEPTION_RECORD ExceptionRecord
    )

/*++

Routine Description:

    This function is called to convert the exception summary register bits
    into a status code value.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

Return Value:

    None.

--*/

{

    PEXC_SUM ExceptionSummary;

    //
    // Perform the following triage on the exception summary register to
    // report the type of exception, even if though the PC reported is
    // imprecise.
    //

    DBGPRINT("KiSetFloatingStatus: ExceptionSummary = %.8lx\n",
             ExceptionRecord->ExceptionInformation[2]);

    ExceptionSummary = (PEXC_SUM)(&ExceptionRecord->ExceptionInformation[2]);
    if (ExceptionSummary->InvalidOperation != 0) {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;

    } else if (ExceptionSummary->DivisionByZero != 0) {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;

    } else if (ExceptionSummary->Overflow != 0) {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;

    } else if (ExceptionSummary->Underflow != 0) {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;

    } else if (ExceptionSummary->InexactResult != 0) {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;

    } else if (ExceptionSummary->IntegerOverflow != 0) {
        ExceptionRecord->ExceptionCode = STATUS_INTEGER_OVERFLOW;

    } else {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_STACK_CHECK;
    }
}
