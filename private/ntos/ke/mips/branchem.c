/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    branchem.c

Abstract:

    This module implement the code necessary to emulate branches when an
    alignment or floating exception occurs in the delay slot of a branch
    instruction.

Author:

    David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

ULONG
KiEmulateBranch (
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to emulate the branch instruction specified by
    the fault instruction address in the specified trap frame. The resultant
    branch destination address is computed and returned as the function value.

Arguments:

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The resultant target branch destination is returned as the function value.

--*/

{

    MIPS_INSTRUCTION BranchInstruction;
    ULONG BranchTaken;
    ULONG BranchNotTaken;
    ULONG RsValue;
    ULONG RtValue;

    //
    // Get the branch instruction at the fault address.
    //

    BranchInstruction.Long = *((PULONG)TrapFrame->Fir);

    //
    // Assume the branch instruction is a conditional branch and get the
    // Rs and Rt register values. Also compute the branch taken as well
    // as the branch not taken target addresses.
    //

    RsValue = KiGetRegisterValue(BranchInstruction.r_format.Rs,
                                 ExceptionFrame,
                                 TrapFrame);

    RtValue = KiGetRegisterValue(BranchInstruction.r_format.Rt,
                                 ExceptionFrame,
                                 TrapFrame);

    BranchTaken = (TrapFrame->Fir + 4) +
                             (LONG)(BranchInstruction.i_format.Simmediate << 2);
    BranchNotTaken = TrapFrame->Fir + 8;

    //
    // Dispatch on the opcode value.
    //
    // N.B. All branch likely instructions are guaranteed to branch since an
    //      exception would not have been generated in the delay slot if the
    //      the branch was not going to actually branch.
    //

    switch (BranchInstruction.r_format.Opcode) {

        //
        // Special opcode - dispatch on the function subopcode.
        //

    case SPEC_OP:
        switch (BranchInstruction.r_format.Function) {

            //
            // Jalr - jump and link register.
            //
            //  N.B. Ra has already been loaded by the hardware before the
            //       exception condition occurred.
            //

        case JALR_OP:

            //
            // Jr - jump register.
            //

        case JR_OP:
            return RsValue;

            //
            // All other instruction are illegal and should never happen.
            //

        default:
            return TrapFrame->Fir;
        }

        //
        // Jal - jump and link.
        //
        //  N.B. Ra has already been loaded by the hardware before the
        //       exception condition occurred.
        //

    case JAL_OP:

        //
        // J - jump.
        //

    case J_OP:
        return ((TrapFrame->Fir + 4) & 0xf0000000) |
                                        (BranchInstruction.j_format.Target << 2);

        //
        // Beq - branch equal.
        // Beql - branch equal likely.
        //

    case BEQ_OP:
    case BEQL_OP:
        if ((LONG)RsValue == (LONG)RtValue) {
            return BranchTaken;

        } else {
            return BranchNotTaken;
        }

        //
        // Bne - branch not equal.
        // Bnel - branch not equal likely.
        //

    case BNE_OP:
    case BNEL_OP:
        if ((LONG)RsValue != (LONG)RtValue) {
            return BranchTaken;

        } else {
            return BranchNotTaken;
        }

        //
        // Blez - branch less than or equal zero.
        // Blezl - branch less than or equal zero likely.
        //

    case BLEZ_OP:
    case BLEZL_OP:
        if ((LONG)RsValue <= 0) {
            return BranchTaken;

        } else {
            return BranchNotTaken;
        }

        //
        // Bgtz - branch greater than zero.
        // Bgtzl - branch greater than zero likely.
        //

    case BGTZ_OP:
    case BGTZL_OP:
        if ((LONG)RsValue > 0) {
            return BranchTaken;

        } else {
            return BranchNotTaken;
        }

        //
        // Branch conditional opcode - dispatch on the rt field.
        //

    case BCOND_OP:
        switch (BranchInstruction.r_format.Rt) {

            //
            // Bltzal - branch on less than zero and link.
            // Bltzall - branch on less than zero and link likely.
            //
            //  N.B. Ra has already been loaded by the hardware before the
            //       exception condition occurred.
            //

        case BLTZAL_OP:
        case BLTZALL_OP:

            //
            // Bltz - branch less than zero.
            // Bltzl - branch less than zero likely.
            //

        case BLTZ_OP:
        case BLTZL_OP:
            if ((LONG)RsValue < 0) {
                return BranchTaken;

            } else {
                return BranchNotTaken;
            }

            //
            // Bgezal - branch on greater than or euqal zero and link.
            // Bgezall - branch on greater than or equal zero and link likely.
            //
            //  N.B. Ra has already been loaded by the hardware before the
            //       exception condition occurred.
            //

        case BGEZAL_OP:
        case BGEZALL_OP:

            //
            // Bgez - branch greater than zero.
            // Bgezl - branch greater than zero likely.
            //

        case BGEZ_OP:
        case BGEZL_OP:
            if ((LONG)RsValue >=  0) {
                return BranchTaken;

            } else {
                return BranchNotTaken;
            }

            //
            // All other instructions are illegal and should not happen.
            //

        default:
            return TrapFrame->Fir;
        }

        //
        // Cop1 - coprocessor 1 branch operation.
        //
        // Bczf - Branch coprocessor z false.
        // Bczfl - Branch coprocessor z false likely.
        // Bczt - Branch coprocessor z true.
        // Bcztl - Branch coprocessor z true likely.
        //

    case COP1_OP:
        if ((BranchInstruction.Long & COPz_BC_MASK) == COPz_BF) {

            //
            // Branch on coprocessor 1 condition code false.
            //

            if (((PFSR)(&TrapFrame->Fsr))->CC == 0) {
                return BranchTaken;

            } else {
                return BranchNotTaken;
            }

        } else if ((BranchInstruction.Long & COPz_BC_MASK) == COPz_BT) {

            //
            // Branch of coprocessor 1 condition code true.
            //

            if (((PFSR)(&TrapFrame->Fsr))->CC != 0) {
                return BranchTaken;

            } else {
                return BranchNotTaken;
            }

        }

        //
        // All other instructions are illegal and should not happen.
        //

    default:
        return TrapFrame->Fir;
    }
}
