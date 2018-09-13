/*++

Copyright (c) 1995  Digital Equipment Corporation

Module Name:

    byteem.c

Abstract:

    This module implements the code necessary to emulate the new set of Alpha
    byte and word instructions defined by ECO 81.

    N.B. This file must be compiled without the use of byte/word instructions
         to avoid fatal recursive exceptions.

Author:

    Wim Colgate (colgate) 18-May-1995
    Thomas Van Baak (tvb) 18-May-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define function prototypes for emulation routines written in assembler.
//

VOID
KiInterlockedStoreByte (
   IN PUCHAR Address,
   IN UCHAR Data
   );

VOID
KiInterlockedStoreWord (
   IN PUSHORT Address,
   IN USHORT Data
   );

BOOLEAN
KiEmulateByteWord (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME  ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine emulates Alpha instructions defined by ECO 81. This includes
    the load byte unsigned, store byte, load word unsigned, store word, sign
    extend byte, and sign extend word instructions.

    If a misaligned word access is detected the illegal instruction exception
    record is converted into data misalignment exception record, no emulation
    is performed, and a value of FALSE is returned. It is expected that the
    call to this function is followed by a check for a data misalignment
    exception and a call to the data misalignment emulation function if
    appropriate.

Arguments:

    ExceptionRecord - Supplies a pointer to the exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned if the instruction is successfully emulated,
    otherwise a value of FALSE is returned.

--*/

{
    ULONGLONG Data;
    ULONGLONG EffectiveAddress;
    PVOID ExceptionAddress;
    ALPHA_INSTRUCTION Instruction;
    KIRQL OldIrql;
    KPROCESSOR_MODE PreviousMode;

    //
    // Save original exception address in case another exception occurs.
    //

    ExceptionAddress = ExceptionRecord->ExceptionAddress;

    //
    // Any exception that occurs during the attempted emulation will cause
    // the emulation to be aborted. The new exception code and information
    // will be copied to the original exception record and FALSE will be
    // returned. If the memory access was not from kernel mode then probe
    // the effective address before performing the emulation.
    //

    //
    // Capture previous mode from trap frame not current thread.
    //

    PreviousMode = (KPROCESSOR_MODE)(((PSR *)(&TrapFrame->Psr))->MODE);

    try {

        //
        // Get faulting instruction and case on instruction type.
        //

        if (PreviousMode != KernelMode) {
            ProbeForRead(ExceptionAddress,
                         sizeof(ALPHA_INSTRUCTION),
                         sizeof(ALPHA_INSTRUCTION));
        }
        Instruction = *((PALPHA_INSTRUCTION)ExceptionAddress);
        switch (Instruction.Memory.Opcode) {

        //
        // Load/store operations.
        //

        case LDBU_OP :
        case LDWU_OP :
        case STB_OP :
        case STW_OP :


            //
            // Compute effective address and if the address is non-canonical
            // then change the exception code to STATUS_ACCESS_VIOLATION and
            // return FALSE.
            //

            EffectiveAddress = (ULONGLONG)Instruction.Memory.MemDisp +
                               KiGetRegisterValue(Instruction.Memory.Rb,
                                                  ExceptionFrame,
                                                  TrapFrame);

            if (EffectiveAddress != (ULONGLONG)(PVOID)EffectiveAddress) {
                ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
                ExceptionRecord->NumberParameters = 0;
                return FALSE;
            }

            //
            // Case on individual load/store instruction type.
            //

            switch (Instruction.Memory.Opcode) {

            //
            // Load byte unsigned.
            //

            case LDBU_OP :
                if (PreviousMode != KernelMode) {
                    ProbeForRead(EffectiveAddress,
                                 sizeof(UCHAR),
                                 sizeof(UCHAR));
                }
                Data = (ULONGLONG)*(PUCHAR)EffectiveAddress;
                KiSetRegisterValue(Instruction.Memory.Ra,
                                   Data,
                                   ExceptionFrame,
                                   TrapFrame);
                break;

            //
            // Load word unsigned.
            //

            case LDWU_OP :
                if (EffectiveAddress & 0x1) {
                    goto AlignmentFault;
                }
                if (PreviousMode != KernelMode) {
                    ProbeForRead((PUSHORT)EffectiveAddress,
                                 sizeof(USHORT),
                                 sizeof(UCHAR));
                }
                Data = (ULONGLONG)*(PUSHORT)EffectiveAddress;
                KiSetRegisterValue(Instruction.Memory.Ra,
                                   Data,
                                   ExceptionFrame,
                                   TrapFrame);
                break;

            //
            // Store byte.
            //

            case STB_OP :
                if (PreviousMode != KernelMode) {
                    ProbeForWrite((PUCHAR)EffectiveAddress,
                                  sizeof(UCHAR),
                                  sizeof(UCHAR));
                }
                Data = KiGetRegisterValue(Instruction.Memory.Ra,
                                          ExceptionFrame,
                                          TrapFrame);
                KiInterlockedStoreByte((PUCHAR)EffectiveAddress,
                                       (UCHAR)Data);
                break;

            //
            // Store word.
            //

            case STW_OP :
                if (EffectiveAddress & 0x1) {
                    goto AlignmentFault;
                }
                if (PreviousMode != KernelMode) {
                    ProbeForWrite((PUSHORT)EffectiveAddress,
                                  sizeof(USHORT),
                                  sizeof(UCHAR));
                }
                Data = KiGetRegisterValue(Instruction.Memory.Ra,
                                          ExceptionFrame,
                                          TrapFrame);
                KiInterlockedStoreWord((PUSHORT)EffectiveAddress,
                                       (USHORT)Data);
                break;
            }

            break;

        //
        // Sign extend operations.
        //

        case SEXT_OP :
            switch (Instruction.OpReg.Function) {

            //
            // Sign extend byte.
            //

            case SEXTB_FUNC :
                Data = KiGetRegisterValue(Instruction.OpReg.Rb,
                                          ExceptionFrame,
                                          TrapFrame);
                KiSetRegisterValue(Instruction.OpReg.Rc,
                                   (ULONGLONG)(CHAR)Data,
                                   ExceptionFrame,
                                   TrapFrame);
                break;

            //
            // Sign extend word.
            //

            case SEXTW_FUNC :
                Data = KiGetRegisterValue(Instruction.OpReg.Rb,
                                          ExceptionFrame,
                                          TrapFrame);
                KiSetRegisterValue(Instruction.OpReg.Rc,
                                   (ULONGLONG)(SHORT)Data,
                                   ExceptionFrame,
                                   TrapFrame);
                break;

            //
            // All other functions are not emulated.
            //

            default :
                return FALSE;
            }

            break;

        //
        // All other instructions are not emulated.
        //

        default :
            return FALSE;
        }

#if 0
        //
        // Call out to profile interrupt if byte/word emulation profiling is
        // active.
        //

        if (KiProfileByteWordEmulation != FALSE) {
            if (++KiProfileByteWordEmulationCount >=
                KiProfileByteWordEmulationInterval) {

                KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
                KiProfileByteWordEmulationCount = 0;
                KeProfileInterruptWithSource(TrapFrame,
                                             ProfileByteWordEmulation);
                KeLowerIrql(OldIrql);
            }
        }
#endif

        TrapFrame->Fir += 4;

        return TRUE;

    } except (KiCopyInformation(ExceptionRecord,
                                (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;

        return FALSE;
    }

AlignmentFault :

    //
    // A misaligned word access has been encountered. Change the illegal
    // instruction exception record into data misalignment exception record
    // (the format is defined by PALcode) and return FALSE.
    //

    ExceptionRecord->ExceptionCode = STATUS_DATATYPE_MISALIGNMENT;
    ExceptionRecord->NumberParameters = 3;
    ExceptionRecord->ExceptionInformation[0] = Instruction.Memory.Opcode;
    ExceptionRecord->ExceptionInformation[1] = Instruction.Memory.Ra;
    ExceptionRecord->ExceptionInformation[2] = (ULONG)EffectiveAddress;

    return FALSE;
}
