/*++

Copyright (c) 1998  Intel Corporation
Copyright (c) 1990  Microsoft Corporation

Module Name:

    psctxi64.c

Abstract:

    This module implements function to get and set the context of a thread.

Author:

    David N. Cutler (davec) 1-Oct-1990

Revision History:

--*/
#include "psp.h"
#include <ia64.h>


#define ALIGN_NATS(Result, Source, Start, AddressOffset, Mask)    \
    if (AddressOffset == Start) {                                       \
        Result = (ULONGLONG)Source;                                     \
    } else if (AddressOffset < Start) {                                 \
        Result = (ULONGLONG)(Source << (Start - AddressOffset));        \
    } else {                                                            \
        Result = (ULONGLONG)((Source >> (AddressOffset - Start)) |      \
                             (Source << (64 + Start - AddressOffset))); \
    }                                                                   \
    Result = Result & (ULONGLONG)Mask

#define EXTRACT_NATS(Result, Source, Start, AddressOffset, Mask)        \
    Result = (ULONGLONG)(Source & (ULONGLONG)Mask);                     \
    if (AddressOffset < Start) {                                        \
        Result = Result >> (Start - AddressOffset);                     \
    } else if (AddressOffset > Start) {                                 \
        Result = ((Result << (AddressOffset - Start)) |                 \
                  (Result >> (64 + Start - AddressOffset)));            \
    }


VOID
KiGetDebugContext (
    IN PKTRAP_FRAME TrapFrame,
    IN OUT PCONTEXT ContextFrame
    );

VOID
KiSetDebugContext (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PCONTEXT ContextFrame,
    IN KPROCESSOR_MODE ProcessorMode
    );

VOID
KiFlushUserRseState (
    IN PKTRAP_FRAME TrapFrame
    );

VOID
PspGetContext (
    IN PKTRAP_FRAME                   TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN OUT PCONTEXT                   ContextEM
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified trap frame
    and nonvolatile context to the specified context record.

Arguments:

    TrapFrame -         Supplies a pointer to a trap frame.

    ContextPointers -   Supplies the address of context pointers record.

    ContextEM -         Supplies the address of a context record.

Return Value:

    None.

    N.B. The side effect of this routine is that the dirty user stacked
         registers that were flushed into the kernel backing store are
         copied backed into the user backing store and the trap frame
         will be modified as a result of that.

--*/

{

    ULONGLONG IntNats1, IntNats2 = 0;
    USHORT R1Offset, R4Offset;
    SHORT Temp, BsFrameSize;

    if ((ContextEM->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        ContextEM->IntGp = TrapFrame->IntGp;
        ContextEM->IntSp = TrapFrame->IntSp;
        ContextEM->ApUNAT = TrapFrame->ApUNAT;
        ContextEM->BrRp = TrapFrame->BrRp;
        ContextEM->ApCCV = TrapFrame->ApCCV;
        ContextEM->ApDCR = TrapFrame->ApDCR;

        ContextEM->StFPSR = TrapFrame->StFPSR;
        ContextEM->StIPSR = TrapFrame->StIPSR;
        ContextEM->StIIP = TrapFrame->StIIP;
        ContextEM->StIFS = TrapFrame->StIFS;

        //
        // Get RSE control states from the trap frame.
        //

        ContextEM->RsPFS = TrapFrame->RsPFS;
        ContextEM->RsRSC = TrapFrame->RsRSC;
        ContextEM->RsRNAT = TrapFrame->RsRNAT;
        
        if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {
            BsFrameSize = (SHORT) (TrapFrame->StIFS >> PFS_SIZE_SHIFT) & PFS_SIZE_MASK;
        } else {
            BsFrameSize = (SHORT) TrapFrame->StIFS & PFS_SIZE_MASK;
        }
        Temp = BsFrameSize - (SHORT)((TrapFrame->RsBSP >> 3) & NAT_BITS_PER_RNAT_REG);
        while (Temp > 0) {
            BsFrameSize++;
            Temp -= NAT_BITS_PER_RNAT_REG;
        }

        ContextEM->RsBSP = TrapFrame->RsBSP - (BsFrameSize * 8);
        ContextEM->RsBSPSTORE = ContextEM->RsBSP;

        //
        // Get preserved applicaton registers
        //

        ContextEM->ApLC = *ContextPointers->ApLC;
        ContextEM->ApEC = (*ContextPointers->ApEC >> PFS_EC_SHIFT) & PFS_EC_MASK;

        //
        // Get iA status
        //

        ContextEM->StFSR = *ContextPointers->StFSR;
        ContextEM->StFIR = *ContextPointers->StFIR;
        ContextEM->StFDR = *ContextPointers->StFDR;
        ContextEM->Cflag = *ContextPointers->Cflag;

    }

    if ((ContextEM->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        ContextEM->IntT0 = TrapFrame->IntT0;
        ContextEM->IntT1 = TrapFrame->IntT1;
        ContextEM->IntT2 = TrapFrame->IntT2;
        ContextEM->IntT3 = TrapFrame->IntT3;
        ContextEM->IntT4 = TrapFrame->IntT4;
        ContextEM->IntV0 = TrapFrame->IntV0;
        ContextEM->IntTeb = TrapFrame->IntTeb;
        ContextEM->Preds = TrapFrame->Preds;
        
        //
        // t5 - t22
        //

        memcpy(&ContextEM->IntT5, &TrapFrame->IntT5, 18*sizeof(ULONGLONG));


        //
        // Get branch registers
        //

        ContextEM->BrT0 = TrapFrame->BrT0;
        ContextEM->BrT1 = TrapFrame->BrT1;

        ContextEM->BrS0 = *ContextPointers->BrS0;
        ContextEM->BrS1 = *ContextPointers->BrS1;
        ContextEM->BrS2 = *ContextPointers->BrS2;
        ContextEM->BrS3 = *ContextPointers->BrS3;
        ContextEM->BrS4 = *ContextPointers->BrS4;

        //
        // Get integer registers s0 - s3 from exception frame.
        //

        ContextEM->IntS0 = *ContextPointers->IntS0;
        ContextEM->IntS1 = *ContextPointers->IntS1;
        ContextEM->IntS2 = *ContextPointers->IntS2;
        ContextEM->IntS3 = *ContextPointers->IntS3;
        IntNats2 |= (((*ContextPointers->IntS0Nat >> (((ULONG_PTR)ContextPointers->IntS0 & 0x1F8) >> 3)) & 0x1) << 4);
        IntNats2 |= (((*ContextPointers->IntS1Nat >> (((ULONG_PTR)ContextPointers->IntS1 & 0x1F8) >> 3)) & 0x1) << 5);
        IntNats2 |= (((*ContextPointers->IntS2Nat >> (((ULONG_PTR)ContextPointers->IntS2 & 0x1F8) >> 3)) & 0x1) << 6);
        IntNats2 |= (((*ContextPointers->IntS3Nat >> (((ULONG_PTR)ContextPointers->IntS3 & 0x1F8) >> 3)) & 0x1) << 7);

        //
        // Get the integer nats field in the context 
        // *ContextPointers->IntNats has Nats for preserved regs
        //

        R1Offset = (USHORT)((ULONG_PTR)(&TrapFrame->IntGp) >> 3) & 0x3f;
        R4Offset = (USHORT)((ULONG_PTR)(ContextPointers->IntS0) >> 3) & 0x3f;
        ALIGN_NATS(IntNats1, TrapFrame->IntNats, 1, R1Offset, 0xFFFFFF0E);

        ContextEM->IntNats = IntNats1 | IntNats2;

#ifdef DEBUG
        DbgPrint("PspGetContext INTEGER: R1Offset = 0x%x, TF->IntNats = 0x%I64x, IntNats1 = 0x%I64x\n",
               R1Offset, TrapFrame->IntNats, IntNats1);
#endif

    }

    if ((ContextEM->ContextFlags & CONTEXT_LOWER_FLOATING_POINT) == CONTEXT_LOWER_FLOATING_POINT) {

        //
        // Get EM + ia32 FP status
        //
        
        ContextEM->StFPSR = TrapFrame->StFPSR;
        ContextEM->StFSR  = *ContextPointers->StFSR;
        ContextEM->StFIR  = *ContextPointers->StFIR;
        ContextEM->StFDR  = *ContextPointers->StFDR;

        //
        // Get floating registers fs0 - fs19
        //

        ContextEM->FltS0 = *ContextPointers->FltS0;
        ContextEM->FltS1 = *ContextPointers->FltS1;
        ContextEM->FltS2 = *ContextPointers->FltS2;
        ContextEM->FltS3 = *ContextPointers->FltS3;

        ContextEM->FltS4 = *ContextPointers->FltS4;
        ContextEM->FltS5 = *ContextPointers->FltS5;
        ContextEM->FltS6 = *ContextPointers->FltS6;
        ContextEM->FltS7 = *ContextPointers->FltS7;

        ContextEM->FltS8 = *ContextPointers->FltS8;
        ContextEM->FltS9 = *ContextPointers->FltS9;
        ContextEM->FltS10 = *ContextPointers->FltS10;
        ContextEM->FltS11 = *ContextPointers->FltS11;

        ContextEM->FltS12 = *ContextPointers->FltS12;
        ContextEM->FltS13 = *ContextPointers->FltS13;
        ContextEM->FltS14 = *ContextPointers->FltS14;
        ContextEM->FltS15 = *ContextPointers->FltS15;

        ContextEM->FltS16 = *ContextPointers->FltS16;
        ContextEM->FltS17 = *ContextPointers->FltS17;
        ContextEM->FltS18 = *ContextPointers->FltS18;
        ContextEM->FltS19 = *ContextPointers->FltS19;

        //
        // Get floating registers ft0 - ft9 from trap frame.
        //

        RtlCopyIa64FloatRegisterContext(&ContextEM->FltT0,
                                        &TrapFrame->FltT0,
                                        sizeof(FLOAT128) * (10));
    }

    if ((ContextEM->ContextFlags & CONTEXT_HIGHER_FLOATING_POINT) == CONTEXT_HIGHER_FLOATING_POINT) {

        //
        // Get EM + ia32 FP status
        //
        
        ContextEM->StFPSR = TrapFrame->StFPSR;
        ContextEM->StFSR  = *ContextPointers->StFSR;
        ContextEM->StFIR  = *ContextPointers->StFIR;
        ContextEM->StFDR  = *ContextPointers->StFDR;

        //
        // Get floating regs f32 - f127 from higher floating point save area
        //

        if (TrapFrame->PreviousMode == UserMode) {
            RtlCopyIa64FloatRegisterContext(
                &ContextEM->FltF32, 
                (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(),
                96*sizeof(FLOAT128)
                );
        }
    }

    //
    // Get h/w debug register context
    //

    if ((ContextEM->ContextFlags & CONTEXT_DEBUG) == CONTEXT_DEBUG) {
        KiGetDebugContext(TrapFrame, ContextEM);
    }

    return;
}

VOID
PspSetContext (
    IN OUT PKTRAP_FRAME               TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN PCONTEXT                       ContextEM,
    IN KPROCESSOR_MODE                ProcessorMode
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified context
    record to the specified trap frame and nonvolatile context.

    We're expecting a plabel to have been passed in the IIP (we won't have a valid
    Global pointer) and if we have a plabel we fill in the correct Global pointer and
    IIP. Technically, the GP is part of the CONTEXT_CONTROL with the EM architecture
    so we only need to check to see if CONTEXT_CONTROL has been specified. 

Arguments:

    TrapFrame -           Supplies the address of the trap frame.

    ContextPointers -   Supplies the address of context pointers record.

    ContextEM -         Supplies the address of a context record.

    ProcessorMode -     Supplies the processor mode to use when sanitizing
                        the PSR and FSR.

Return Value:

    None.

--*/

{
    SHORT BsFrameSize;
    SHORT TempFrameSize;
    USHORT R1Offset, R4Offset;
    USHORT RNatSaveIndex;

    if ((ContextEM->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        if (ContextEM->IntGp == 0) {
            try {

                //
                // Make sure we have read access to the plabel
                //

                ProbeForRead(ContextEM->StIIP, 
                             sizeof(PLABEL_DESCRIPTOR),
                             sizeof(ULONGLONG));
                ContextEM->IntGp = ((PPLABEL_DESCRIPTOR)(ContextEM->StIIP))->GlobalPointer;
                ContextEM->StIIP = ((PPLABEL_DESCRIPTOR)(ContextEM->StIIP))->EntryPoint;

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // Remote thread does not have access to the plabel. Just
                // let the thread take the exception.
                //

                ;
            }
        }

        TrapFrame->IntGp = ContextEM->IntGp;
        TrapFrame->IntSp = ContextEM->IntSp;
        TrapFrame->ApUNAT = ContextEM->ApUNAT;
        TrapFrame->BrRp = ContextEM->BrRp;
        TrapFrame->ApCCV = ContextEM->ApCCV;
        TrapFrame->ApDCR = SANITIZE_DCR(ContextEM->ApDCR, ProcessorMode);

        //
        // Set preserved applicaton registers.
        //

        *ContextPointers->ApLC = ContextEM->ApLC;
        *ContextPointers->ApEC &= ~(PFS_EC_MASK << PFS_EC_SHIFT);
        *ContextPointers->ApEC |= ((ContextEM->ApEC & PFS_EC_MASK) << PFS_EC_SHIFT);

        TrapFrame->StFPSR = SANITIZE_FSR(ContextEM->StFPSR, ProcessorMode);
        TrapFrame->StIIP = ContextEM->StIIP;
        TrapFrame->StIFS = SANITIZE_IFS(ContextEM->StIFS, ProcessorMode);
        TrapFrame->StIPSR = SANITIZE_PSR(ContextEM->StIPSR, ProcessorMode);

        if (((TrapFrame->StIPSR >> PSR_RI) & 3) == 3) {

            //
            // 3 is an invalid slot number; sanitize it to zero
            //

            TrapFrame->StIPSR &= ~(3i64 << PSR_RI);
        }

        TrapFrame->RsPFS = ContextEM->RsPFS;

        if (TRAP_FRAME_TYPE(TrapFrame) == SYSCALL_FRAME) {
            if (TrapFrame->StIPSR & (1i64 << PSR_SS)) {

                //
                // if the psr.ss bit is set when the target thread is in a
                // system call, the psr.lp bit has to be set in order for
                // the target thread to pick up its setting when it returns
                // from the system call.  i.e. psr.ss is not saved and
                // and restored by the EPC system call handler.
                //

                TrapFrame->StIPSR |= (1i64 << PSR_LP);
            }

            //
            // in the case of EPC system call, the frame size that needs to be
            // adjusted is the local frame size, not the total framze size.
            //

            BsFrameSize = (SHORT) (ContextEM->StIFS >> PFS_SIZE_SHIFT) & PFS_SIZE_MASK;
        } else {
            BsFrameSize = (SHORT) ContextEM->StIFS & PFS_SIZE_MASK;
        }
        RNatSaveIndex = (USHORT)((ContextEM->RsBSP >> 3) & NAT_BITS_PER_RNAT_REG);

        TempFrameSize = RNatSaveIndex + BsFrameSize - NAT_BITS_PER_RNAT_REG;
        while (TempFrameSize >= 0) {
            BsFrameSize++;
            TempFrameSize -= NAT_BITS_PER_RNAT_REG;
        }

        TrapFrame->RsBSPSTORE = ContextEM->RsBSPSTORE + BsFrameSize * 8;
        TrapFrame->RsBSP = TrapFrame->RsBSPSTORE;
        TrapFrame->RsRSC = ContextEM->RsRSC;
        TrapFrame->RsRNAT = ContextEM->RsRNAT;

#ifdef DEBUG
        DbgPrint ("PspSetContext CONTROL: TrapFrame->RsRNAT = 0x%I64x\n",
                TrapFrame->RsRNAT);
#endif

        //
        // DebugActive controls h/w debug registers. Set if new psr.db = 1
        //

        KeGetCurrentThread()->DebugActive = ((TrapFrame->StIPSR & (1I64 << PSR_DB)) != 0);

        //
        // Set iA status
        // *** TBD SANATIZE??
        //

        *ContextPointers->StFSR = ContextEM->StFSR;
        *ContextPointers->StFIR = ContextEM->StFIR;
        *ContextPointers->StFDR = ContextEM->StFDR;
        *ContextPointers->Cflag = ContextEM->Cflag;

    }

    if ((ContextEM->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        TrapFrame->IntT0 = ContextEM->IntT0;
        TrapFrame->IntT1 = ContextEM->IntT1;
        TrapFrame->IntT2 = ContextEM->IntT2;
        TrapFrame->IntT3 = ContextEM->IntT3;
        TrapFrame->IntT4 = ContextEM->IntT4;
        TrapFrame->IntV0 = ContextEM->IntV0;
        TrapFrame->IntTeb = ContextEM->IntTeb;
        TrapFrame->Preds = ContextEM->Preds;

        //
        //  t5 - t2
        //

        RtlCopyMemory(&TrapFrame->IntT5, &ContextEM->IntT5, 18*sizeof(ULONGLONG));

        //
        // Set the integer nats fields
        //

        R1Offset = (USHORT)((ULONG_PTR)(&TrapFrame->IntGp) >> 3) & 0x3f;

        EXTRACT_NATS(TrapFrame->IntNats, ContextEM->IntNats,
                     1, R1Offset, 0xFFFFFF0E);

        //
        // Set the preserved integer NAT fields
        //

        R4Offset = (USHORT)((ULONG_PTR)(ContextPointers->IntS0) >> 3) & 0x3f;

        //
        // BUGBUG: TBD
        //
        // EXTRACT_NATS(*ContextPointers->IntNats, ContextEM->IntNats,
        //           4, R4Offset, 0xF0);

        *ContextPointers->IntS0 = ContextEM->IntS0;
        *ContextPointers->IntS1 = ContextEM->IntS1;
        *ContextPointers->IntS2 = ContextEM->IntS2;
        *ContextPointers->IntS3 = ContextEM->IntS3;

        *ContextPointers->IntS0Nat &= ~(0x1 << (((ULONG_PTR)ContextPointers->IntS0 & 0x1F8) >> 3));
        *ContextPointers->IntS1Nat &= ~(0x1 << (((ULONG_PTR)ContextPointers->IntS1 & 0x1F8) >> 3));
        *ContextPointers->IntS2Nat &= ~(0x1 << (((ULONG_PTR)ContextPointers->IntS2 & 0x1F8) >> 3));
        *ContextPointers->IntS3Nat &= ~(0x1 << (((ULONG_PTR)ContextPointers->IntS3 & 0x1F8) >> 3));

        *ContextPointers->IntS0Nat |= (((ContextEM->IntNats >> 4) & 0x1) << (((ULONG_PTR)ContextPointers->IntS0 & 0x1F8) >> 3));
        *ContextPointers->IntS1Nat |= (((ContextEM->IntNats >> 4) & 0x1) << (((ULONG_PTR)ContextPointers->IntS1 & 0x1F8) >> 3));
        *ContextPointers->IntS2Nat |= (((ContextEM->IntNats >> 4) & 0x1) << (((ULONG_PTR)ContextPointers->IntS2 & 0x1F8) >> 3));
        *ContextPointers->IntS3Nat |= (((ContextEM->IntNats >> 4) & 0x1) << (((ULONG_PTR)ContextPointers->IntS3 & 0x1F8) >> 3));

#ifdef DEBUG
        DbgPrint("PspSetContext INTEGER: R1Offset = 0x%x, TF->IntNats = 0x%I64x, Context->IntNats = 0x%I64x\n",
               R1Offset, TrapFrame->IntNats, ContextEM->IntNats);
#endif

        *ContextPointers->BrS0 = ContextEM->BrS0;
        *ContextPointers->BrS1 = ContextEM->BrS1;
        *ContextPointers->BrS2 = ContextEM->BrS2;
        *ContextPointers->BrS3 = ContextEM->BrS3;
        *ContextPointers->BrS4 = ContextEM->BrS4;
        TrapFrame->BrT0 = ContextEM->BrT0;
        TrapFrame->BrT1 = ContextEM->BrT1;
    }

    if ((ContextEM->ContextFlags & CONTEXT_LOWER_FLOATING_POINT) == CONTEXT_LOWER_FLOATING_POINT) {

        TrapFrame->StFPSR = SANITIZE_FSR(ContextEM->StFPSR, ProcessorMode);
        *ContextPointers->StFSR = ContextEM->StFSR;
        *ContextPointers->StFIR = ContextEM->StFIR;
        *ContextPointers->StFDR = ContextEM->StFDR;

        //
        // Set floating registers fs0 - fs19.
        //

        *ContextPointers->FltS0 = ContextEM->FltS0;
        *ContextPointers->FltS1 = ContextEM->FltS1;
        *ContextPointers->FltS2 = ContextEM->FltS2;
        *ContextPointers->FltS3 = ContextEM->FltS3;

        *ContextPointers->FltS4 = ContextEM->FltS4;
        *ContextPointers->FltS5 = ContextEM->FltS5;
        *ContextPointers->FltS6 = ContextEM->FltS6;
        *ContextPointers->FltS7 = ContextEM->FltS7;

        *ContextPointers->FltS8 = ContextEM->FltS8;
        *ContextPointers->FltS9 = ContextEM->FltS9;
        *ContextPointers->FltS10 = ContextEM->FltS10;
        *ContextPointers->FltS11 = ContextEM->FltS11;

        *ContextPointers->FltS12 = ContextEM->FltS12;
        *ContextPointers->FltS13 = ContextEM->FltS13;
        *ContextPointers->FltS14 = ContextEM->FltS14;
        *ContextPointers->FltS15 = ContextEM->FltS15;

        *ContextPointers->FltS16 = ContextEM->FltS16;
        *ContextPointers->FltS17 = ContextEM->FltS17;
        *ContextPointers->FltS18 = ContextEM->FltS18;
        *ContextPointers->FltS19 = ContextEM->FltS19;

        //
        // Set floating registers ft0 - ft9.
        //

        RtlCopyIa64FloatRegisterContext(&TrapFrame->FltT0, 
                                        &ContextEM->FltT0,
                                        sizeof(FLOAT128) * (10));
    }

    if ((ContextEM->ContextFlags & CONTEXT_HIGHER_FLOATING_POINT) == CONTEXT_HIGHER_FLOATING_POINT) {


        TrapFrame->StFPSR = SANITIZE_FSR(ContextEM->StFPSR, ProcessorMode);
        *ContextPointers->StFSR = ContextEM->StFSR;
        *ContextPointers->StFIR = ContextEM->StFIR;
        *ContextPointers->StFDR = ContextEM->StFDR;

        if (ProcessorMode == UserMode) {

            //
            // Update the higher floating point save area (f32-f127) and 
            // set the corresponding modified bit in the PSR to 1.
            //

            RtlCopyIa64FloatRegisterContext(
                (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(),
                &ContextEM->FltF32,
                96*sizeof(FLOAT128));

            //
            // set the dfh bit to force a reload of the high fp register
            // set on the next user access
            //

            TrapFrame->StIPSR |= (1i64 << PSR_DFH);
        }

    }

    //
    // Set debug register contents if specified.
    //

    if ((ContextEM->ContextFlags & CONTEXT_DEBUG) == CONTEXT_DEBUG) {
        KiSetDebugContext (TrapFrame, ContextEM, ProcessorMode);
    }

    return;
}

VOID
PspGetSetContextSpecialApcMain (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function either captures the user mode state of the current
    thread, or sets the user mode state of the current thread. The
    operation type is determined by the value of SystemArgument1. A
    zero value is used for get context, and a nonzero value is used
    for set context.

Arguments:

    Apc - Supplies a pointer to the APC control object that caused entry
          into this routine.

    NormalRoutine - Supplies a pointer to the normal routine function that
        was specified when the APC was initialized. This parameter is not
        used.

    NormalContext - Supplies a pointer to an arbitrary data structure that
        was specified when the APC was initialized. This parameter is not
        used.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointers to two
        arguments that contain untyped data.
        The first arguement is used to distinguish between get and set requests.
            A value of zero  signifies that GetThreadContext was requested.
            A non-zero value signifies that SetThreadContext was requested.
        The Second arguement has the thread handle. The second arguement is
        not used.

Return Value:

    None.

--*/

{
    PGETSETCONTEXT                ContextInfo;
    KNONVOLATILE_CONTEXT_POINTERS ContextPointers;  // Not currently used, needed later.
    CONTEXT                       ContextRecord;
    ULONGLONG                     ControlPc;
    FRAME_POINTERS                EstablisherFrame;
    PRUNTIME_FUNCTION             FunctionEntry;
    BOOLEAN                       InFunction;
    PETHREAD                      Thread;
    PKTRAP_FRAME                  TrFrame1;
    ULONGLONG                     ImageBase;
    ULONGLONG                     TargetGp;

    //
    // Get the address of the context frame and compute the address of the
    // system entry trap frame.
    //

    ContextInfo = CONTAINING_RECORD(Apc, GETSETCONTEXT, Apc);

    TrFrame1 = (PKTRAP_FRAME)(((ULONG_PTR)PsGetCurrentThread()->Tcb.InitialStack
                    - KTHREAD_STATE_SAVEAREA_LENGTH - KTRAP_FRAME_LENGTH));

    //
    // Capture the current thread context and set the initial control PC
    // value.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = ContextRecord.BrRp;

    //
    // Initialize context pointers for the nonvolatile integer and floating
    // registers.
    //

    ContextPointers.FltS0 = &ContextRecord.FltS0;
    ContextPointers.FltS1 = &ContextRecord.FltS1;
    ContextPointers.FltS2 = &ContextRecord.FltS2;
    ContextPointers.FltS3 = &ContextRecord.FltS3;
    ContextPointers.FltS4 = &ContextRecord.FltS4;
    ContextPointers.FltS5 = &ContextRecord.FltS5;
    ContextPointers.FltS6 = &ContextRecord.FltS6;
    ContextPointers.FltS7 = &ContextRecord.FltS7;
    ContextPointers.FltS8 = &ContextRecord.FltS8;
    ContextPointers.FltS9 = &ContextRecord.FltS9;
    ContextPointers.FltS10 = &ContextRecord.FltS10;
    ContextPointers.FltS11 = &ContextRecord.FltS11;
    ContextPointers.FltS12 = &ContextRecord.FltS12;
    ContextPointers.FltS13 = &ContextRecord.FltS13;
    ContextPointers.FltS14 = &ContextRecord.FltS14;
    ContextPointers.FltS15 = &ContextRecord.FltS15;
    ContextPointers.FltS16 = &ContextRecord.FltS16;
    ContextPointers.FltS17 = &ContextRecord.FltS17;
    ContextPointers.FltS18 = &ContextRecord.FltS18;
    ContextPointers.FltS19 = &ContextRecord.FltS19;

    ContextPointers.IntS0 = &ContextRecord.IntS0;
    ContextPointers.IntS1 = &ContextRecord.IntS1;
    ContextPointers.IntS2 = &ContextRecord.IntS2;
    ContextPointers.IntS3 = &ContextRecord.IntS3;
    ContextPointers.IntSp = &ContextRecord.IntSp;

    ContextPointers.BrS0 = &ContextRecord.BrS0;
    ContextPointers.BrS1 = &ContextRecord.BrS1;
    ContextPointers.BrS2 = &ContextRecord.BrS2;
    ContextPointers.BrS3 = &ContextRecord.BrS3;
    ContextPointers.BrS4 = &ContextRecord.BrS4;

    ContextPointers.ApLC = &ContextRecord.ApLC;
    ContextPointers.ApEC = &ContextRecord.ApEC;

    ContextPointers.StFSR = &ContextRecord.StFSR;
    ContextPointers.StFIR = &ContextRecord.StFIR;
    ContextPointers.StFDR = &ContextRecord.StFDR;
    ContextPointers.Cflag = &ContextRecord.Cflag;

    //
    // Start with the frame specified by the context record and virtually
    // unwind call frames until the system entry trap frame is encountered.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, &TargetGp);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the address
        // where control left the caller.
        //

        if (FunctionEntry != NULL) {
            ControlPc = RtlVirtualUnwind(ImageBase,
                                         ControlPc,
                                         FunctionEntry,
                                         &ContextRecord,
                                         &InFunction,
                                         &EstablisherFrame,
                                         &ContextPointers);

        } else {
            SHORT BsFrameSize, TempFrameSize;

            ControlPc = ContextRecord.BrRp;
            ContextRecord.StIFS = ContextRecord.RsPFS;
            BsFrameSize = (SHORT)(ContextRecord.StIFS >> PFS_SIZE_SHIFT) & PFS_SIZE_MASK;
            TempFrameSize = BsFrameSize - (SHORT)((ContextRecord.RsBSP >> 3) & NAT_BITS_PER_RNAT_REG);
            while (TempFrameSize > 0) {
                BsFrameSize++;
                TempFrameSize -= NAT_BITS_PER_RNAT_REG;
            }
            ContextRecord.RsBSP -= BsFrameSize * sizeof(ULONGLONG);
        }

    } while ((PVOID)ContextRecord.IntSp != TrFrame1);

    //
    // Process GetThreadContext or SetThreadContext as specified.
    //

    if (*SystemArgument1 != 0) {

        //
        // Set Context from proper Context mode
        //

        PspSetContext(TrFrame1, &ContextPointers, &ContextInfo->Context,
                      ContextInfo->Mode);

    } else {

        //
        // Get Context from proper Context mode
        //

        KiFlushUserRseState(TrFrame1);
        PspGetContext(TrFrame1, &ContextPointers, &ContextInfo->Context);

    }

    KeSetEvent(&ContextInfo->OperationComplete, 0, FALSE);
    return;
}
