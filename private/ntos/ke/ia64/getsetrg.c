/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    getsetrg.c

Abstract:

    This module implement the code necessary to get and set register values.
    These routines are used during the emulation of unaligned data references
    and floating point exceptions.

Author:

    David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "ntfpia64.h"



ULONGLONG
KiGetRegisterValue (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to get the value of a register from the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        returned. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The value of the specified register is returned as the function value.

--*/

{
    //
    // Dispatch on the register number.
    //

    if (Register == 0) {
        return 0;
    } else if (Register <= 3) {
        Register -= 1;
        return ( *(&TrapFrame->IntGp + Register) );
    } else if (Register <= 7) {
        Register -= 4;
        return ( *(&ExceptionFrame->IntS0 + Register) );
    } else if (Register <= 31) {
        Register -= 8;
        return ( *(&TrapFrame->IntV0 + Register) );
    }
    
    //
    // Register is the stacked register
    //
    //   (R32 - R127)
    //

    {
        PULONGLONG UserBStore, KernelBStore;
        ULONG RegisterOffset;
        ULONG i;
        ULONG SizeOfCurrentFrame;
        ULONG SizeOfDirty;
        LONG NumberOfDirty;

        SizeOfDirty = (ULONG)(TrapFrame->RsBSP - TrapFrame->RsBSPSTORE);
        NumberOfDirty = SizeOfDirty >> 3;
        SizeOfCurrentFrame = (ULONG)(TrapFrame->StIFS & 0x7F);

        if (TrapFrame->PreviousMode == UserMode) {

            //
            // PreviousMode is user
            //

            KernelBStore = (PULONGLONG)(PCR->InitialBStore + (TrapFrame->RsBSPSTORE & 0x1F8) + SizeOfDirty);

            UserBStore = (ULONGLONG *) TrapFrame->RsBSP; 

            RegisterOffset = Register - 32;

            do {

                KernelBStore = KernelBStore - 1;
                UserBStore = UserBStore - 1;
                NumberOfDirty = NumberOfDirty -1;
                
                SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

                if (((ULONG_PTR) KernelBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust UserBsp, by skipping RNAT
                    //

                    KernelBStore = KernelBStore -1;
                }

                if (((ULONG_PTR) UserBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust UserBsp, by skipping RNAT
                    //

                    UserBStore = UserBStore -1;
                }

            } while (RegisterOffset < SizeOfCurrentFrame); 

            if (NumberOfDirty >= 0) {
                
                return (*KernelBStore);

            } else {

                return (*UserBStore);
            }
            
        } else {

            //
            // PreviousMode is kernel
            //

            KernelBStore = (ULONGLONG *) TrapFrame->RsBSP;

            RegisterOffset = Register - 32;

            do {

                KernelBStore = KernelBStore - 1;

                SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

                if (((ULONG_PTR) KernelBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust UserBsp, by skipping RNAT
                    //

                    KernelBStore = KernelBStore -1;
                }
                
            } while (RegisterOffset < SizeOfCurrentFrame); 
            
            return (*KernelBStore);
        }
    }
}   


VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to set the value of a register in the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        stored. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    Value - Supplies the value to be stored in the specified register.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    //
    // Dispatch on the register number.
    //

    if (Register == 0) {
        return;
    } else if (Register <= 3) {
        Register -= 1;
        *(&TrapFrame->IntGp + Register) = Value;
        return;
    } else if (Register <= 7) {
        Register -= 4;
        *(&ExceptionFrame->IntS0 + Register) = Value;
        return;
    } else if (Register <= 31) {
        Register -= 8;
        *(&TrapFrame->IntV0 + Register) = Value;
        return;
    }

    //
    // Register is the stacked register
    //
    //   (R32 - R127)
    //

    {
        PULONGLONG UserBStore, KernelBStore;
        ULONG RegisterOffset;
        ULONG i;
        ULONG SizeOfCurrentFrame;
        ULONG SizeOfDirty;
        LONG NumberOfDirty;

        SizeOfDirty = (ULONG)(TrapFrame->RsBSP - TrapFrame->RsBSPSTORE);
        NumberOfDirty = SizeOfDirty >> 3;
        SizeOfCurrentFrame = (ULONG)(TrapFrame->StIFS & 0x7F);

        if (TrapFrame->PreviousMode == UserMode) {

            //
            // PreviousMode is user
            //

            KernelBStore = (PULONGLONG)(PCR->InitialBStore + (TrapFrame->RsBSPSTORE & 0x1F8) + SizeOfDirty);

            UserBStore = (ULONGLONG *) TrapFrame->RsBSP; 

            RegisterOffset = Register - 32;

            do {

                KernelBStore = KernelBStore - 1;
                UserBStore = UserBStore - 1;
                NumberOfDirty = NumberOfDirty -1;
                
                SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

                if (((ULONG_PTR) KernelBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust UserBsp, by skipping RNAT
                    //

                    KernelBStore = KernelBStore -1;
                }

                if (((ULONG_PTR) UserBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust UserBsp, by skipping RNAT
                    //

                    UserBStore = UserBStore -1;
                }

            } while (RegisterOffset < SizeOfCurrentFrame); 

            if (NumberOfDirty >= 0) {
                
                *KernelBStore = Value;

            } else {

                *UserBStore = Value;
            }
            
        } else {

            //
            // PreviousMode is kernel
            //

            KernelBStore = (ULONGLONG *) TrapFrame->RsBSP;

            RegisterOffset = Register - 32;

            do {

                KernelBStore = KernelBStore - 1;

                SizeOfCurrentFrame = SizeOfCurrentFrame - 1;

                if (((ULONG_PTR) KernelBStore & 0x1F8) == 0x1F8) {
                    
                    //
                    // Adjust UserBsp, by skipping RNAT
                    //

                    KernelBStore = KernelBStore -1;
                }
                
            } while (RegisterOffset < SizeOfCurrentFrame); 
            
            *KernelBStore = Value;
        }
    }
}   

#define GET_NAT_ADDRESS(addr) (((ULONG_PTR) (addr) >> 3) & 0x3F)

#define GET_NAT(Nats, addr) (UCHAR)((Nats >> GET_NAT_ADDRESS(addr)) & 1)


UCHAR
KiGetRegisterNaT (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )
{
    //
    // Dispatch on the register number.
    //

    switch (Register) {

        //
        // Integer register V0.
        //

    case 0:
        return 0;

        //
        // Integer Register R1 (GP)
        //

    case 1:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntGp));
        
        //
        // Integer Register R2 (T0)
        //

    case 2:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT0));

        //
        // Integer Register R3 (T1)
        //

    case 3:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT1));

        //
        // Integer Register R4 (S0)
        //

    case 4:
        return (GET_NAT(ExceptionFrame->IntNats, &ExceptionFrame->IntS0));

        //
        // Integer Register R5 (S1)
        //

    case 5:
        return (GET_NAT(ExceptionFrame->IntNats, &ExceptionFrame->IntS1));

        //
        // Integer Register R6 (S2)
        //

    case 6:
        return (GET_NAT(ExceptionFrame->IntNats, &ExceptionFrame->IntS2));

        //
        // Integer Register R7 (S3)
        //

    case 7:
        return (GET_NAT(ExceptionFrame->IntNats, &ExceptionFrame->IntS3));

        //
        // Integer Register R8 (V0)
        //

    case 8:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntV0));
        
        //
        // Integer Register R9 (T2)
        //
   
    case 9:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT2));

        //
        // Integer Register R10 (T3)
        //

    case 10:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT3));

        //
        // Integer Register R11 (T4)
        //

    case 11:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT4));

        //
        // Integer Register R12 (Sp)
        //

    case 12:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntSp));

        //
        // Integer Register R13 (Teb)
        //
   
    case 13:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntTeb));

        //
        // Integer Register R14 (T5)
        //
   
    case 14:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT5));

        //
        // Integer Register R15 (T6)
        //
   
    case 15:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT6));

        //
        // Integer Register R16 (T7)
        //
   
    case 16:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT7));

        //
        // Integer Register R17 (T8)
        //
   
    case 17:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT8));

        //
        // Integer Register R18 (T9)
        //
   
    case 18:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT9));

        //
        // Integer Register R19 (T10)
        //
   
    case 19:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT10));

        //
        // Integer Register R20 (T11)
        //
   
    case 20:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT11));

        //
        // Integer Register R21 (T12)
        //
   
    case 21:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT12));

        //
        // Integer Register R22 (T13)
        //
   
    case 22:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT13));

        //
        // Integer Register R23 (T14)
        //
   
    case 23:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT14));

        //
        // Integer Register R24 (T15)
        //
   
    case 24:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT15));

        //
        // Integer Register R25 (T16)
        //
   
    case 25:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT16));

        //
        // Integer Register R26 (T17)
        //
   
    case 26:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT17));

        //
        // Integer Register R27 (T18)
        //
   
    case 27:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT18));

        //
        // Integer Register R28 (T19)
        //
   
    case 28:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT19));

        //
        // Integer Register R29 (T20)
        //
   
    case 29:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT20));

        //
        // Integer Register R30 (T21)
        //
   
    case 30:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT21));

        //
        // Integer Register R31 (T22)
        //

    case 31:
        return (GET_NAT(TrapFrame->IntNats, &TrapFrame->IntT22));

    default:
        break;
   
    }

    return 0;
}


FLOAT128
KiGetFloatRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    )

{
    if (Register == 0) {
        FLOAT128 t = {0ULL,0ULL};
        return t;
    } else if (Register == 1) {
        FLOAT128 t = {0x8000000000000000ULL,0x000000000000FFFFULL}; // low,high
        return t;
    } else if (Register <= 5) {
        Register -= 2;
        return ( *(&ExceptionFrame->FltS0 + Register) );
    } else if (Register <= 15) {
        Register -= 6;
        return ( *(&TrapFrame->FltT0 + Register) );
    } else if (Register <= 31) {
        Register -= 16;
        return ( *(&ExceptionFrame->FltS4 + Register) );
    } else {
        PKHIGHER_FP_VOLATILE HigherVolatile;

        HigherVolatile = GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA();
        Register -= 32;
        return ( *(&HigherVolatile->FltF32 + Register) );
    }
}


VOID
KiSetFloatRegisterValue (
    IN ULONG Register,
    IN FLOAT128 Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    )

{
    if (Register <= 1) {
        return;
    } else if (Register <= 5) {
        Register -= 2;
        *(&ExceptionFrame->FltS0 + Register) = Value;
        return;
    } else if (Register <= 15) {
        Register -= 6;
        *(&TrapFrame->FltT0 + Register) = Value;
        return;
    } else if (Register <= 31) {
        Register -= 16;
        *(&ExceptionFrame->FltS4 + Register) = Value;
        return;
    } else {
        PKHIGHER_FP_VOLATILE HigherVolatile;

        HigherVolatile = GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA();
        Register -= 32;
        *(&HigherVolatile->FltF32 + Register) = Value;
        return;
    }
}

VOID
__cdecl
KeSaveStateForHibernate(
    IN PKPROCESSOR_STATE ProcessorState
    )
/*++

Routine Description:

    Saves all processor-specific state that must be preserved
    across an S4 state (hibernation).

Arguments:

    ProcessorState - Supplies the KPROCESSOR_STATE where the
        current CPU's state is to be saved.

Return Value:

    None.

--*/

{
    //
    // BUGBUG John Vert (jvert) 4/30/1998
    //  someone needs to implement this and probably put it in a more 
    //  appropriate file.
    
}


FLOAT128
get_fp_register (
    IN ULONG Register,
    IN PVOID FpState
    )
{
    return(KiGetFloatRegisterValue (
               Register, 
               ((PFLOATING_POINT_STATE)FpState)->ExceptionFrame,
               ((PFLOATING_POINT_STATE)FpState)->TrapFrame
               ));
}

VOID
set_fp_register (
    IN ULONG Register,
    IN FLOAT128 Value,
    IN PVOID FpState
    )
{
    KiSetFloatRegisterValue (
        Register, 
        Value,
        ((PFLOATING_POINT_STATE)FpState)->ExceptionFrame,
        ((PFLOATING_POINT_STATE)FpState)->TrapFrame
        );
}
