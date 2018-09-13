/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    alignem.c

Abstract:

    This module implement the code necessary to emulate unaliged data
    references.

Author:

    David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to emulate an unaligned data reference to an
    address in the user part of the address space.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned if the data reference is successfully
    emulated. Otherwise, a value of FALSE is returned.

--*/

{

    ULONG BranchAddress;
    PUCHAR DataAddress;

    union {
        ULONGLONG Longlong;
        ULONG Long;
        USHORT Short;
    } DataReference;

    PUCHAR DataValue;
    PVOID ExceptionAddress;
    MIPS_INSTRUCTION FaultInstruction;
    ULONG Rt;
    KIRQL  OldIrql;

    //
    // If alignment profiling is active, then call the proper profile
    // routine.
    //

    if (KiProfileAlignmentFixup) {
        KiProfileAlignmentFixupCount += 1;
        if (KiProfileAlignmentFixupCount >= KiProfileAlignmentFixupInterval) {
            KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
            KiProfileAlignmentFixupCount = 0;
            KeProfileInterruptWithSource(TrapFrame, ProfileAlignmentFixup);
            KeLowerIrql(OldIrql);
        }
    }

    //
    // Save the original exception address in case another exception
    // occurs.
    //

    ExceptionAddress = ExceptionRecord->ExceptionAddress;

    //
    // Any exception that occurs during the attempted emulation of the
    // unaligned reference causes the emulation to be aborted. The new
    // exception code and information is copied to the original exception
    // record and a value of FALSE is returned.
    //

    try {

        //
        // If the exception PC is equal to the fault instruction address
        // plus four, then the misalignment exception occurred in the delay
        // slot of a branch instruction and the continuation address must
        // be computed by emulating the branch instruction. Note that it
        // is possible for an exception to occur when the branch instruction
        // is read from user memory.
        //

        if ((TrapFrame->Fir + 4) == (ULONG)ExceptionRecord->ExceptionAddress) {
            BranchAddress = KiEmulateBranch(ExceptionFrame, TrapFrame);

        } else {
            BranchAddress = TrapFrame->Fir + 4;
        }

        //
        // Compute the effective address of the reference and check to make
        // sure it is within the user part of the address space. Alignment
        // exceptions take precedence over memory management exceptions and
        // the address could be a system address.
        //

        FaultInstruction.Long = *((PULONG)ExceptionRecord->ExceptionAddress);
        DataAddress = (PUCHAR)KiGetRegisterValue64(FaultInstruction.i_format.Rs,
                                                   ExceptionFrame,
                                                   TrapFrame);

        DataAddress = (PUCHAR)((LONG)DataAddress +
                                    (LONG)FaultInstruction.i_format.Simmediate);

        //
        // The emulated data reference must be in user space and must be less
        // than 16 types from the end of user space.
        //

        if ((ULONG)DataAddress < 0x7ffffff0) {

            //
            // Dispatch on the opcode value.
            //

            DataValue = (PUCHAR)&DataReference;
            Rt = FaultInstruction.i_format.Rt;
            switch (FaultInstruction.i_format.Opcode) {

                //
                // Load halfword integer.
                //

            case LH_OP:
                DataValue[0] = DataAddress[0];
                DataValue[1] = DataAddress[1];
                KiSetRegisterValue64(Rt,
                                     (SHORT)DataReference.Short,
                                     ExceptionFrame,
                                     TrapFrame);

                break;

                //
                // Load halfword unsigned integer.
                //

            case LHU_OP:
                DataValue[0] = DataAddress[0];
                DataValue[1] = DataAddress[1];
                KiSetRegisterValue64(Rt,
                                     DataReference.Short,
                                     ExceptionFrame,
                                     TrapFrame);

                break;

                //
                // Load word floating.
                //

            case LWC1_OP:
                DataValue[0] = DataAddress[0];
                DataValue[1] = DataAddress[1];
                DataValue[2] = DataAddress[2];
                DataValue[3] = DataAddress[3];
                KiSetRegisterValue(Rt + 32,
                                   DataReference.Long,
                                   ExceptionFrame,
                                   TrapFrame);

                break;

                //
                // Load word integer.
                //

            case LW_OP:
                DataValue[0] = DataAddress[0];
                DataValue[1] = DataAddress[1];
                DataValue[2] = DataAddress[2];
                DataValue[3] = DataAddress[3];
                KiSetRegisterValue64(Rt,
                                     (LONG)DataReference.Long,
                                     ExceptionFrame,
                                     TrapFrame);

                break;

                //
                // Load double integer.
                //

            case LD_OP:
                DataValue[0] = DataAddress[0];
                DataValue[1] = DataAddress[1];
                DataValue[2] = DataAddress[2];
                DataValue[3] = DataAddress[3];
                DataValue[4] = DataAddress[4];
                DataValue[5] = DataAddress[5];
                DataValue[6] = DataAddress[6];
                DataValue[7] = DataAddress[7];
                KiSetRegisterValue64(Rt,
                                     DataReference.Longlong,
                                     ExceptionFrame,
                                     TrapFrame);

                break;

                //
                // Load double floating.
                //

            case LDC1_OP:
                Rt = (Rt & 0x1e) + 32;
                DataValue[0] = DataAddress[0];
                DataValue[1] = DataAddress[1];
                DataValue[2] = DataAddress[2];
                DataValue[3] = DataAddress[3];
                KiSetRegisterValue(Rt,
                                   DataReference.Long,
                                   ExceptionFrame,
                                   TrapFrame);

                DataValue[0] = DataAddress[4];
                DataValue[1] = DataAddress[5];
                DataValue[2] = DataAddress[6];
                DataValue[3] = DataAddress[7];
                KiSetRegisterValue(Rt + 1,
                                   DataReference.Long,
                                   ExceptionFrame,
                                   TrapFrame);

                break;

                //
                // Store halfword integer.
                //

            case SH_OP:
                DataReference.Longlong = KiGetRegisterValue64(Rt,
                                                              ExceptionFrame,
                                                              TrapFrame);

                DataAddress[0] = DataValue[0];
                DataAddress[1] = DataValue[1];
                break;

                //
                // Store word floating.
                //

            case SWC1_OP:
                DataReference.Long = KiGetRegisterValue(Rt + 32,
                                                        ExceptionFrame,
                                                        TrapFrame);

                DataAddress[0] = DataValue[0];
                DataAddress[1] = DataValue[1];
                DataAddress[2] = DataValue[2];
                DataAddress[3] = DataValue[3];
                break;

                //
                // Store word integer.
                //

            case SW_OP:
                DataReference.Longlong = KiGetRegisterValue64(Rt,
                                                              ExceptionFrame,
                                                              TrapFrame);

                DataAddress[0] = DataValue[0];
                DataAddress[1] = DataValue[1];
                DataAddress[2] = DataValue[2];
                DataAddress[3] = DataValue[3];
                break;

                //
                // Store double integer.
                //

            case SD_OP:
                DataReference.Longlong = KiGetRegisterValue64(Rt,
                                                              ExceptionFrame,
                                                              TrapFrame);

                DataAddress[0] = DataValue[0];
                DataAddress[1] = DataValue[1];
                DataAddress[2] = DataValue[2];
                DataAddress[3] = DataValue[3];
                DataAddress[4] = DataValue[4];
                DataAddress[5] = DataValue[5];
                DataAddress[6] = DataValue[6];
                DataAddress[7] = DataValue[7];
                break;

                //
                // Store double floating.
                //

            case SDC1_OP:
                Rt = (Rt & 0x1e) + 32;
                DataReference.Long = KiGetRegisterValue(Rt,
                                                        ExceptionFrame,
                                                        TrapFrame);

                DataAddress[0] = DataValue[0];
                DataAddress[1] = DataValue[1];
                DataAddress[2] = DataValue[2];
                DataAddress[3] = DataValue[3];
                DataReference.Long = KiGetRegisterValue(Rt + 1,
                                                        ExceptionFrame,
                                                        TrapFrame);

                DataAddress[4] = DataValue[0];
                DataAddress[5] = DataValue[1];
                DataAddress[6] = DataValue[2];
                DataAddress[7] = DataValue[3];
                break;

                //
                // All other instructions are not emulated.
                //

            default:
                return FALSE;
            }

            TrapFrame->Fir = BranchAddress;
            return TRUE;
        }

    //
    // If an exception occurs, then copy the new exception information to the
    // original exception record and handle the exception.
    //

    } except (KiCopyInformation(ExceptionRecord,
                               (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;
    }

    //
    // Return a value of FALSE.
    //

    return FALSE;
}
