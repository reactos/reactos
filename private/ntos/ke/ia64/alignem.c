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

#define OPCODE_MASK      0x1EF00000000

#define LD_OP            0x08000000000 
#define LDS_OP           0x08100000000
#define LDA_OP           0x08200000000
#define LDSA_OP          0x08300000000
#define LDBIAS_OP        0x08400000000
#define LDACQ_OP         0x08500000000
#define LDCCLR_OP        0x08800000000
#define LDCNC_OP         0x08900000000
#define LDCCLRACQ_OP     0x08A00000000
#define ST_OP            0x08C00000000
#define STREL_OP         0x08D00000000

#define LD_IMM_OP        0x0A000000000 
#define LDS_IMM_OP       0x0A100000000
#define LDA_IMM_OP       0x0A200000000
#define LDSA_IMM_OP      0x0A300000000
#define LDBIAS_IMM_OP    0x0A400000000
#define LDACQ_IMM_OP     0x0A500000000
#define LDCCLR_IMM_OP    0x0A800000000
#define LDCNC_IMM_OP     0x0A900000000
#define LDCCLRACQ_IMM_OP 0x0AA00000000
#define ST_IMM_OP        0x0AC00000000
#define STREL_IMM_OP     0x0AD00000000

#define LDF_OP           0x0C000000000
#define LDFS_OP          0x0C100000000
#define LDFA_OP          0x0C200000000
#define LDFSA_OP         0x0C300000000
#define LDFCCLR_OP       0x0C800000000
#define LDFCNC_OP        0x0C900000000
#define STF_OP           0x0CC00000000

#define LDF_IMM_OP       0x0E000000000
#define LDFS_IMM_OP      0x0E100000000
#define LDFA_IMM_OP      0x0E200000000
#define LDFSA_IMM_OP     0x0E300000000
#define LDFCCLR_IMM_OP   0x0E800000000
#define LDFCNC_IMM_OP    0x0E900000000
#define STF_IMM_OP       0x0EC00000000

typedef struct _INST_FORMAT {
    union {
        struct {
            ULONGLONG qp:   6;
            ULONGLONG r1:   7;
            ULONGLONG r2:   7;
            ULONGLONG r3:   7;
            ULONGLONG x:    1;
            ULONGLONG hint: 2;
            ULONGLONG x6:   6;
            ULONGLONG m:    1;
            ULONGLONG Op:   4;
            ULONGLONG Rsv: 23; 
        } i_field;
        ULONGLONG Ulong64;
    } u;
} INST_FORMAT;

VOID
KiEmulateLoad(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateStore(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateLoadFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateStoreFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    );

VOID
KiEmulateLoadFloat80(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateLoadFloatInt(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateLoadFloat32(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateLoadFloat64(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloat80(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloatInt(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloat32(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );

VOID
KiEmulateStoreFloat64(
    IN PVOID UnalignedAddress, 
    OUT PVOID FloatData
    );


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

    PVOID EffectiveAddress;
    PVOID ExceptionAddress;
    KIRQL  OldIrql;

    KPROCESSOR_MODE PreviousMode;
    INST_FORMAT FaultInstruction;
    ULONGLONG Opcode;
    ULONGLONG Reg2Value;
    ULONGLONG Reg3Value;
    ULONGLONG BundleLow;
    ULONGLONG BundleHigh;
    ULONGLONG Syllable;
    ULONGLONG Data = 0;
    FLOAT128 FloatData = {0, 0};
    ULONG OpSize;
    ULONG ImmValue;
    ULONG Length;


    //
    // Must flush the RSE to synchronize the RSE and backing store contents
    //

    KiFlushRse();

    //
    // Call out to profile interrupt if alignment profiling is active
    //

    if (KiProfileAlignmentFixup) {

        if (++KiProfileAlignmentFixupCount >= KiProfileAlignmentFixupInterval) {

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

    EffectiveAddress = (PVOID) ExceptionRecord->ExceptionInformation[1]; 
    ExceptionAddress = (PVOID) TrapFrame->StIIP;

    //
    // Capture previous mode from trap frame not current thread.
    //

    PreviousMode = (KPROCESSOR_MODE) TrapFrame->PreviousMode;

    //
    // Any exception that occurs during the attempted emulation of the
    // unaligned reference causes the emulation to be aborted. The new
    // exception code and information is copied to the original exception
    // record and a value of FALSE is returned.
    //

    try {


        BundleLow = *((ULONGLONG *)ExceptionAddress);
        BundleHigh = *(((ULONGLONG *)ExceptionAddress) + 1);

        Syllable = (TrapFrame->StIPSR >> PSR_RI) & 0x3;

        switch (Syllable) {
        case 0: 
            FaultInstruction.u.Ulong64 = (BundleLow >> 5);
            break;
        case 1:
            FaultInstruction.u.Ulong64 = (BundleLow >> 46) | (BundleHigh << 18);
            break;
        case 2:
            FaultInstruction.u.Ulong64 = (BundleHigh >> 23);
        case 3: 
        default: 
            return FALSE;
        }
    
        Opcode = FaultInstruction.u.Ulong64 & OPCODE_MASK;
        OpSize = (ULONG)FaultInstruction.u.i_field.x6 & 0x3;

        switch (Opcode) {

        //    
        // speculative and speculative advanced load
        //

        case LDS_OP:
        case LDSA_OP:    
        case LDS_IMM_OP:
        case LDSA_IMM_OP:
        case LDFS_OP:
        case LDFSA_OP:
        case LDFS_IMM_OP:

            //
            // return NaT value to the target register
            //

            TrapFrame->StIPSR |= (1i64 << PSR_ED);

            return TRUE;

        //
        // normal, advance, and check load
        //

        case LD_OP:
        case LDA_OP:
        case LDBIAS_OP:
        case LDCCLR_OP:
        case LDCNC_OP:
        case LDACQ_OP:
        case LDCCLRACQ_OP:

            if (FaultInstruction.u.i_field.x == 1) {
                
                //
                // xField must be 0
                //

                return FALSE;
            }
    
            if( PreviousMode != KernelMode ){
                ProbeForRead( EffectiveAddress,
                              1 << OpSize,
                              sizeof(UCHAR) );
            }
            KiEmulateLoad(EffectiveAddress, OpSize, &Data);
            KiSetRegisterValue( (ULONG) FaultInstruction.u.i_field.r1,
                                Data,
                                ExceptionFrame,
                                TrapFrame );

            if (FaultInstruction.u.i_field.m == 1) {

                //
                // Update the address register (R3)
                //
                

                Reg2Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                                ExceptionFrame,
                                                TrapFrame );

                Reg3Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                                ExceptionFrame,
                                                TrapFrame );

                //
                // register update form
                //

                Reg3Value = Reg2Value + Reg3Value;

                KiSetRegisterValue ((ULONG) FaultInstruction.u.i_field.r3, 
                                    Reg3Value,
                                    ExceptionFrame,
                                    TrapFrame);
            }

            if ((Opcode == LDACQ_OP) || (Opcode == LDCCLRACQ_OP)) {

                //
                // all future access should occur after unaligned memory accesses
                //

                __mf();
            }

            break;

        //
        // normal, advance, and check load
        //     immidiate updated form
        //

        case LD_IMM_OP:
        case LDA_IMM_OP:
        case LDBIAS_IMM_OP:
        case LDCCLR_IMM_OP:
        case LDCNC_IMM_OP:
        case LDACQ_IMM_OP:
        case LDCCLRACQ_IMM_OP:

            if( PreviousMode != KernelMode ){
                ProbeForRead( EffectiveAddress,
                              1 << OpSize,
                              sizeof(UCHAR) );
            }
            KiEmulateLoad(EffectiveAddress, OpSize, &Data);
            KiSetRegisterValue( (ULONG) FaultInstruction.u.i_field.r1,
                                Data,
                                ExceptionFrame,
                                TrapFrame );

            //
            // Update the address register R3
            //

            Reg3Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                            ExceptionFrame,
                                            TrapFrame );

            //
            // immediate update form
            //

            ImmValue = (ULONG)(FaultInstruction.u.i_field.r2 
                             + (FaultInstruction.u.i_field.x << 7));

            if (FaultInstruction.u.i_field.m == 1) {

                ImmValue = 0xFFFFFFFFFFFFFF00 | ImmValue;

            } 

            Reg3Value = Reg3Value + ImmValue;

            KiSetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                Reg3Value,
                                ExceptionFrame,
                                TrapFrame );
            
            if ((Opcode == LDACQ_IMM_OP) || (Opcode == LDCCLRACQ_IMM_OP)) {

                //
                // all future access should occur after unaligned memory accesses
                //

                __mf();
            }

            break;

        case LDF_OP:
        case LDFA_OP:
        case LDFCCLR_OP:
        case LDFCNC_OP:

            if (FaultInstruction.u.i_field.x == 1) {

                //
                // floating point load pair
                //

                if (FaultInstruction.u.i_field.m == 1) {

                    //
                    // m field must be zero
                    //

                    return FALSE;
                
                }

                if( PreviousMode != KernelMode ){

                    switch (OpSize) {
                    case 0: return FALSE;
                    case 1: Length = 8; break;
                    case 2: Length = 4; break;
                    case 3: Length = 8; break;
                    default: 
                        return FALSE;
                    }

                    ProbeForRead( EffectiveAddress,
                                  Length << 1,
                                  sizeof(UCHAR) );
                }

                //
                // emulate the 1st half of the pair
                //

                KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
                KiSetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r1,
                                         FloatData,
                                         ExceptionFrame,
                                         TrapFrame );

                //
                // emulate the 2nd half of the pair
                //

                EffectiveAddress = (PVOID)((ULONG_PTR)EffectiveAddress + Length);

                KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
                KiSetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                         FloatData,
                                         ExceptionFrame,
                                         TrapFrame );

            } else {

                //
                // floating point single load
                //

                if( PreviousMode != KernelMode ){

                    switch (OpSize) {
                    case 0: Length = 16; break;
                    case 1: Length = 8; break;
                    case 2: Length = 4; break;
                    case 3: Length = 8; break;
                    default: 
                        return FALSE;
                    }

                    ProbeForRead( EffectiveAddress,
                                  Length,
                                  sizeof(UCHAR) );
                }

                KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
                KiSetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r1,
                                         FloatData,
                                         ExceptionFrame,
                                         TrapFrame );

                if (FaultInstruction.u.i_field.m == 1) {
                    
                    //
                    // update the address register (R3)
                    //

                    Reg2Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                                    ExceptionFrame,
                                                    TrapFrame );
                
                    Reg3Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                                    ExceptionFrame,
                                                    TrapFrame );
                    //
                    // register update form
                    //
                
                    Reg3Value = Reg2Value + Reg3Value;

                    KiSetRegisterValue ((ULONG) FaultInstruction.u.i_field.r3, 
                                        Reg3Value,
                                        ExceptionFrame,
                                        TrapFrame);
                }
            }
                
            break;

        //    
        // normal, advanced and checked floating point load 
        //    immediate updated form
        //

        case LDF_IMM_OP:
        case LDFA_IMM_OP:
        case LDFCCLR_IMM_OP:
        case LDFCNC_IMM_OP:

            if (FaultInstruction.u.i_field.x == 1) {

                //
                // floating point load pair
                //

                if (FaultInstruction.u.i_field.m == 0) {

                    //
                    // m field must be one
                    //

                    return FALSE;
                
                }

                if( PreviousMode != KernelMode ){

                    switch (OpSize) {
                    case 0: return FALSE;
                    case 1: Length = 8; break;
                    case 2: Length = 8; break;
                    case 3: Length = 4; break;
                    default: 
                        return FALSE;
                    }

                    ProbeForRead( EffectiveAddress,
                                  Length << 1,
                                  sizeof(UCHAR) );
                }

                //
                // emulate the 1st half of the pair
                //

                KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
                KiSetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r1,
                                         FloatData,
                                         ExceptionFrame,
                                         TrapFrame );

                EffectiveAddress = (PVOID)((ULONG_PTR)EffectiveAddress + Length);

                //
                // emulate the 2nd half of the pair
                //

                KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
                KiSetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                         FloatData,
                                         ExceptionFrame,
                                         TrapFrame );

                //
                // Update the address register (R3)
                //

                Reg3Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                                ExceptionFrame,
                                                TrapFrame );

                //
                // immediate update form
                //

                ImmValue = Length << 1;
                
                Reg3Value = Reg3Value + ImmValue;

                KiSetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                    Reg3Value,
                                    ExceptionFrame,
                                    TrapFrame );

            } else {
                
                //
                // floating point single load
                // 

                if( PreviousMode != KernelMode ){
                    
                    switch (OpSize) {
                    case 0: Length = 16; break;
                    case 1: Length = 8; break;
                    case 2: Length = 4; break;
                    case 3: Length = 8; break;
                    default: 
                        return FALSE;
                    }

                    ProbeForRead( EffectiveAddress,
                                  Length,
                                  sizeof(UCHAR) );
                }
                KiEmulateLoadFloat(EffectiveAddress, OpSize, &FloatData);
                KiSetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r1,
                                         FloatData,
                                         ExceptionFrame,
                                         TrapFrame );

                //
                // Update the address register (R3)
                //

                Reg3Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                                ExceptionFrame,
                                                TrapFrame );

                //
                // immediate update form
                //

                ImmValue = (ULONG)(FaultInstruction.u.i_field.r2 
                             + (FaultInstruction.u.i_field.x << 7));

                if (FaultInstruction.u.i_field.m == 1) {

                    ImmValue = 0xFFFFFFFFFFFFFF00 | ImmValue;

                } 

                Reg3Value = Reg3Value + ImmValue;

                KiSetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                    Reg3Value,
                                    ExceptionFrame,
                                    TrapFrame );
            }
             
            break;


        case STREL_OP:

             __mf();

        case ST_OP:

            if (FaultInstruction.u.i_field.x == 1) {
                
                //
                // xField must be 0
                //

                return FALSE;
            }

            if (FaultInstruction.u.i_field.m == 1) {

                //
                // no register update form defined
                //

                return FALSE;
            }
    
            if( PreviousMode != KernelMode ){
                ProbeForWrite( EffectiveAddress,
                               1 << OpSize,
                               sizeof(UCHAR) );
            }
            Data = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                       ExceptionFrame,
                                       TrapFrame );

            KiEmulateStore( EffectiveAddress, OpSize, &Data);

            break;
            
        case STREL_IMM_OP:

            __mf();

        case ST_IMM_OP:

            if( PreviousMode != KernelMode ){
                ProbeForWrite( EffectiveAddress,
                               1 << OpSize,
                               sizeof(UCHAR) );
            }
            Data = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                       ExceptionFrame,
                                       TrapFrame );

            KiEmulateStore( EffectiveAddress, OpSize, &Data);

            //
            // update the address register (R3)
            //

            Reg3Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                       ExceptionFrame,
                                       TrapFrame );

            //
            // immediate update form
            //

            ImmValue = (ULONG)(FaultInstruction.u.i_field.r1 
                             + (FaultInstruction.u.i_field.x << 7));

            if (FaultInstruction.u.i_field.m == 1) {

                ImmValue = 0xFFFFFFFFFFFFFF00 | ImmValue;

            } 

            Reg3Value = Reg3Value + ImmValue;

            KiSetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                Reg3Value,
                                ExceptionFrame,
                                TrapFrame );
            
            break;
            

        case STF_OP:    
    
            if (FaultInstruction.u.i_field.x) {

                //
                // x field must be 0 
                //

                return FALSE;
            }

            if (FaultInstruction.u.i_field.m) {

                //
                // no register update form defined
                //

                return FALSE;
            }

            if( PreviousMode != KernelMode ){

                switch (OpSize) {
                case 0: Length = 16; break;
                case 1: Length = 8; break;
                case 2: Length = 4; break;
                case 3: Length = 8; break;
                default: 
                    return FALSE;
                }
                
                ProbeForWrite( EffectiveAddress,
                               Length,
                               sizeof(UCHAR) );
            }
            FloatData = KiGetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                             ExceptionFrame,
                                             TrapFrame );

            KiEmulateStoreFloat( EffectiveAddress, OpSize, &FloatData);

            break;
            
        case STF_IMM_OP:    

            if( PreviousMode != KernelMode ){

                switch (OpSize) {
                case 0: Length = 16; break;
                case 1: Length = 8; break;
                case 2: Length = 4; break;
                case 3: Length = 8; break;
                default: 
                    return FALSE;
                }
                
                ProbeForWrite( EffectiveAddress,
                               Length,
                               sizeof(UCHAR) );
            }
            FloatData = KiGetFloatRegisterValue( (ULONG) FaultInstruction.u.i_field.r2,
                                             ExceptionFrame,
                                             TrapFrame );

            KiEmulateStoreFloat( EffectiveAddress, OpSize, &FloatData);

            //
            // update the address register (R3)
            //

            Reg3Value = KiGetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                            ExceptionFrame,
                                            TrapFrame );
            //
            // immediate update form
            //

            ImmValue = (ULONG)(FaultInstruction.u.i_field.r1 
                             + (FaultInstruction.u.i_field.x << 7));

            if (FaultInstruction.u.i_field.m == 1) {

                ImmValue = 0xFFFFFFFFFFFFFF00 | ImmValue;

            }

            Reg3Value = Reg3Value + ImmValue;

            KiSetRegisterValue( (ULONG) FaultInstruction.u.i_field.r3,
                                Reg3Value,
                                ExceptionFrame,
                                TrapFrame );
            
            break;
            
        default:

            return FALSE;

        }

        //
        // advance instruction pointer
        //

        KiAdvanceInstPointer(TrapFrame);

        return TRUE;

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


VOID
KiEmulateLoad(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    )

/*++

Routine Description:

    This routine returns the integer value stored at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to data value.

    OperandSize - Supplies the size of data to be loaded

    Data - Supplies a pointer to be filled for data
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/

{
    PUCHAR Source;
    PUCHAR Destination;
    ULONG i;

    Source = (PUCHAR) UnalignedAddress; 
    Destination = (PUCHAR) Data;
    OperandSize = 1 << OperandSize; 

    for (i = 0; i < OperandSize; i++) {

        *Destination++ = *Source++;

    }

    return;
}


VOID
KiEmulateStore(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    )
/*++

Routine Description:

    This routine store the integer value at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to be stored

    OperandSize - Supplies the size of data to be storeed

    Data - Supplies a pointer to data value
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/
{
    PUCHAR Source;
    PUCHAR Destination;
    ULONG i;

    Source = (PUCHAR) Data; 
    Destination = (PUCHAR) UnalignedAddress;
    OperandSize = 1 << OperandSize; 

    for (i = 0; i < OperandSize; i++) {

        *Destination++ = *Source++;

    }

    return;
}


VOID
KiEmulateLoadFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN OUT PVOID Data
    )

/*++

Routine Description:

    This routine returns the floating point value stored at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to floating point data value.

    OperandSize - Supplies the size of data to be loaded

    Data - Supplies a pointer to be filled for data
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/

{
    FLOAT128 FloatData;

    RtlMoveMemory(&FloatData, UnalignedAddress, sizeof(FLOAT128));

    switch (OperandSize) {

    case 0:
        KiEmulateLoadFloat80(&FloatData, Data);
        return;

    case 1:
        KiEmulateLoadFloatInt(&FloatData, Data);
        return;

    case 2:
        KiEmulateLoadFloat32(&FloatData, Data);
        return;

    case 3: 
        KiEmulateLoadFloat64(&FloatData, Data);
        return;

    default:
        return;
    }
}

VOID
KiEmulateStoreFloat(
    IN PVOID UnalignedAddress,
    IN ULONG OperandSize,
    IN PVOID Data
    )

/*++

Routine Description:

    This routine stores the floating point value stored at the unaligned
    address passed in UnalignedAddress.

Arguments:

    UnalignedAddress - Supplies a pointer to be stored.

    OperandSize - Supplies the size of data to be loaded

    Data - Supplies a pointer to floating point data
   
Return Value:

    The value at the address pointed to by UnalignedAddress.

--*/

{
    FLOAT128 FloatData;
    ULONG Length;

    switch (OperandSize) {

    case 0:
        KiEmulateStoreFloat80(&FloatData, Data);
        Length = 16;
        break;

    case 1:
        KiEmulateStoreFloatInt(&FloatData, Data);
        Length = 8;
        break;

    case 2:
        KiEmulateStoreFloat32(&FloatData, Data);
        Length = 4;
        break;

    case 3: 
        KiEmulateStoreFloat64(&FloatData, Data);
        Length = 8;
        break;

    default:
        return;
    }

    RtlMoveMemory(UnalignedAddress, &FloatData, Length);
}

