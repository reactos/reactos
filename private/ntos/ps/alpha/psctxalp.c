/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    psctxalp.c

Abstract:

    This module implements functions to get and set the context of a thread.

Author:

    David N. Cutler (davec) 1-Oct-1990

Revision History:

    Thomas Van Baak (tvb) 18-May-1992

        Adapted for Alpha AXP.

--*/

#include "psp.h"

VOID
PspGetContext (
    IN PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified trap frame
    and nonvolatile context to the specified context record.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ContextPointers - Supplies the address of context pointers record.

    ContextRecord - Supplies the address of a context record.

Return Value:

    None.

--*/

{

    //
    // Get control information if specified.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Get integer registers gp, ra, sp, FIR, and PSR from trap frame.
        //

        ContextRecord->IntGp = TrapFrame->IntGp;
        ContextRecord->IntSp = TrapFrame->IntSp;
        ContextRecord->IntRa = TrapFrame->IntRa;
        ContextRecord->Fir = TrapFrame->Fir;
        ContextRecord->Psr = TrapFrame->Psr;
    }

    //
    // Get integer register contents if specified.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Get volatile integer registers v0 and t0 - t7 from trap frame.
        //

        ContextRecord->IntV0 = TrapFrame->IntV0;
        ContextRecord->IntT0 = TrapFrame->IntT0;
        ContextRecord->IntT1 = TrapFrame->IntT1;
        ContextRecord->IntT2 = TrapFrame->IntT2;
        ContextRecord->IntT3 = TrapFrame->IntT3;
        ContextRecord->IntT4 = TrapFrame->IntT4;
        ContextRecord->IntT5 = TrapFrame->IntT5;
        ContextRecord->IntT6 = TrapFrame->IntT6;
        ContextRecord->IntT7 = TrapFrame->IntT7;

        //
        // Get nonvolatile integer registers s0 - s5 through context pointers.
        //

        ContextRecord->IntS0 = *ContextPointers->IntS0;
        ContextRecord->IntS1 = *ContextPointers->IntS1;
        ContextRecord->IntS2 = *ContextPointers->IntS2;
        ContextRecord->IntS3 = *ContextPointers->IntS3;
        ContextRecord->IntS4 = *ContextPointers->IntS4;
        ContextRecord->IntS5 = *ContextPointers->IntS5;

        //
        // Get volatile integer registers fp/s6, a0 - a5, and t8 - t12 from
        // trap frame.
        //

        ContextRecord->IntFp = TrapFrame->IntFp;

        ContextRecord->IntA0 = TrapFrame->IntA0;
        ContextRecord->IntA1 = TrapFrame->IntA1;
        ContextRecord->IntA2 = TrapFrame->IntA2;
        ContextRecord->IntA3 = TrapFrame->IntA3;
        ContextRecord->IntA4 = TrapFrame->IntA4;
        ContextRecord->IntA5 = TrapFrame->IntA5;

        ContextRecord->IntT8 = TrapFrame->IntT8;
        ContextRecord->IntT9 = TrapFrame->IntT9;
        ContextRecord->IntT10 = TrapFrame->IntT10;
        ContextRecord->IntT11 = TrapFrame->IntT11;
        ContextRecord->IntT12 = TrapFrame->IntT12;

        //
        // Get volatile integer register AT from trap frame.
        // Get integer register zero.
        //

        ContextRecord->IntAt = TrapFrame->IntAt;
        ContextRecord->IntZero = 0;
    }

    //
    // Get floating register contents if specified.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Get volatile floating registers f0 - f1 from trap frame.
        //

        ContextRecord->FltF0 = TrapFrame->FltF0;
        ContextRecord->FltF1 = TrapFrame->FltF1;

        //
        // Get volatile floating registers f10 - f30 from trap frame.
        // This assumes that f10 - f30 are contiguous in the trap frame.
        //

        ASSERT((&ContextRecord->FltF30 - &ContextRecord->FltF10) == 20);
        RtlMoveMemory(&ContextRecord->FltF10, &TrapFrame->FltF10,
                      sizeof(ULONGLONG) * 21);

        ContextRecord->FltF31 = 0;

        //
        // Get nonvolatile floating registers f2 - f9 through context pointers.
        //

        ContextRecord->FltF2 = *ContextPointers->FltF2;
        ContextRecord->FltF3 = *ContextPointers->FltF3;
        ContextRecord->FltF4 = *ContextPointers->FltF4;
        ContextRecord->FltF5 = *ContextPointers->FltF5;
        ContextRecord->FltF6 = *ContextPointers->FltF6;
        ContextRecord->FltF7 = *ContextPointers->FltF7;
        ContextRecord->FltF8 = *ContextPointers->FltF8;
        ContextRecord->FltF9 = *ContextPointers->FltF9;

        //
        // Get floating point control register from trap frame.
        // Get the current software FPCR value from the TEB.
        //

        ContextRecord->Fpcr = TrapFrame->Fpcr;
        try {
            ContextRecord->SoftFpcr =
                (ULONGLONG)(NtCurrentTeb()->FpSoftwareStatusRegister);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            ContextRecord->SoftFpcr = 0;
        }
    }

    return;
}

VOID
PspSetContext (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE ProcessorMode
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified context
    record to the specified trap frame and nonvolatile context.

Arguments:

    TrapFrame - Supplies the address of a trap frame.

    ContextPointers - Supplies the address of a context pointers record.

    ContextRecord - Supplies the address of a context record.

    ProcessorMode - Supplies the processor mode to use when sanitizing
        the PSR and FSR.

Return Value:

    None.

--*/

{

    //
    // Set control information if specified.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set integer registers gp, sp, ra, FIR, and PSR in trap frame.
        //

        TrapFrame->IntGp = ContextRecord->IntGp;
        TrapFrame->IntSp = ContextRecord->IntSp;
        TrapFrame->IntRa = ContextRecord->IntRa;
        TrapFrame->Fir = ContextRecord->Fir;
        TrapFrame->Psr = SANITIZE_PSR(ContextRecord->Psr, ProcessorMode);
    }

    //
    // Set integer register contents if specified.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set volatile integer registers v0 and t0 - t7 in trap frame.
        //

        TrapFrame->IntV0 = ContextRecord->IntV0;
        TrapFrame->IntT0 = ContextRecord->IntT0;
        TrapFrame->IntT1 = ContextRecord->IntT1;
        TrapFrame->IntT2 = ContextRecord->IntT2;
        TrapFrame->IntT3 = ContextRecord->IntT3;
        TrapFrame->IntT4 = ContextRecord->IntT4;
        TrapFrame->IntT5 = ContextRecord->IntT5;
        TrapFrame->IntT6 = ContextRecord->IntT6;
        TrapFrame->IntT7 = ContextRecord->IntT7;

        //
        // Set nonvolatile integer registers s0 - s5 through context pointers.
        //

        *ContextPointers->IntS0 = ContextRecord->IntS0;
        *ContextPointers->IntS1 = ContextRecord->IntS1;
        *ContextPointers->IntS2 = ContextRecord->IntS2;
        *ContextPointers->IntS3 = ContextRecord->IntS3;
        *ContextPointers->IntS4 = ContextRecord->IntS4;
        *ContextPointers->IntS5 = ContextRecord->IntS5;

        //
        // Set volatile integer registers fp/s6, a0 - a5, t8 - t12, and AT
        // in trap frame.
        //

        TrapFrame->IntFp = ContextRecord->IntFp;

        TrapFrame->IntA0 = ContextRecord->IntA0;
        TrapFrame->IntA1 = ContextRecord->IntA1;
        TrapFrame->IntA2 = ContextRecord->IntA2;
        TrapFrame->IntA3 = ContextRecord->IntA3;
        TrapFrame->IntA4 = ContextRecord->IntA4;
        TrapFrame->IntA5 = ContextRecord->IntA5;

        TrapFrame->IntT8 = ContextRecord->IntT8;
        TrapFrame->IntT9 = ContextRecord->IntT9;
        TrapFrame->IntT10 = ContextRecord->IntT10;
        TrapFrame->IntT11 = ContextRecord->IntT11;
        TrapFrame->IntT12 = ContextRecord->IntT12;

        TrapFrame->IntAt = ContextRecord->IntAt;
    }

    //
    // Set floating register contents if specified.
    //

    if ((ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set volatile floating registers f0 - f1 in trap frame.
        //

        TrapFrame->FltF0 = ContextRecord->FltF0;
        TrapFrame->FltF1 = ContextRecord->FltF1;

        //
        // Set volatile floating registers f10 - f30 in trap frame.
        // This assumes that f10 - f30 are contiguous in the trap frame.
        //

        ASSERT((&ContextRecord->FltF30 - &ContextRecord->FltF10) == 20);
        RtlMoveMemory(&TrapFrame->FltF10, &ContextRecord->FltF10,
                      sizeof(ULONGLONG) * 21);

        //
        // Set nonvolatile floating registers f2 - f9 through context pointers.
        //

        *ContextPointers->FltF2 = ContextRecord->FltF2;
        *ContextPointers->FltF3 = ContextRecord->FltF3;
        *ContextPointers->FltF4 = ContextRecord->FltF4;
        *ContextPointers->FltF5 = ContextRecord->FltF5;
        *ContextPointers->FltF6 = ContextRecord->FltF6;
        *ContextPointers->FltF7 = ContextRecord->FltF7;
        *ContextPointers->FltF8 = ContextRecord->FltF8;
        *ContextPointers->FltF9 = ContextRecord->FltF9;

        //
        // Set floating point control register in trap frame.
        // Set the current software FPCR value in the TEB.
        //

        TrapFrame->Fpcr = ContextRecord->Fpcr;
        try {
            NtCurrentTeb()->FpSoftwareStatusRegister =
                (ULONG)ContextRecord->SoftFpcr;

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
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

    SystemArgument1, SystemArgument2 - Supplies a set of two pointer to two
        arguments that contain untyped data. These parameters are not used.

Return Value:

    None.

--*/

{

    PGETSETCONTEXT ContextBlock;
    KNONVOLATILE_CONTEXT_POINTERS ContextPointers;
    CONTEXT ContextRecord;
    ULONG_PTR ControlPc;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    PETHREAD Thread;
    ULONGLONG TrapFrame1;
    ULONGLONG TrapFrame2;

    //
    // Get the address of the context block and compute the address of the
    // system entry trap frame.
    //

    ContextBlock = CONTAINING_RECORD(Apc, GETSETCONTEXT, Apc);
    Thread = PsGetCurrentThread();
    TrapFrame1 = (ULONGLONG)(LONG_PTR)Thread->Tcb.InitialStack - KTRAP_FRAME_LENGTH;

    //
    // The lower bounds for locating the first system service trap frame on
    // the kernel stack is one trap frame below TrapFrame1.
    //

    TrapFrame2 = TrapFrame1 - KTRAP_FRAME_LENGTH;

    //
    // Capture the current thread context and set the initial control PC
    // value.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = (ULONG_PTR)ContextRecord.IntRa;

    //
    // Initialize context pointers for the nonvolatile integer and floating
    // registers.
    //

    ContextPointers.IntS0 = &ContextRecord.IntS0;
    ContextPointers.IntS1 = &ContextRecord.IntS1;
    ContextPointers.IntS2 = &ContextRecord.IntS2;
    ContextPointers.IntS3 = &ContextRecord.IntS3;
    ContextPointers.IntS4 = &ContextRecord.IntS4;
    ContextPointers.IntS5 = &ContextRecord.IntS5;

    ContextPointers.FltF2 = &ContextRecord.FltF2;
    ContextPointers.FltF3 = &ContextRecord.FltF3;
    ContextPointers.FltF4 = &ContextRecord.FltF4;
    ContextPointers.FltF5 = &ContextRecord.FltF5;
    ContextPointers.FltF6 = &ContextRecord.FltF6;
    ContextPointers.FltF7 = &ContextRecord.FltF7;
    ContextPointers.FltF8 = &ContextRecord.FltF8;
    ContextPointers.FltF9 = &ContextRecord.FltF9;

    //
    // Start with the frame specified by the context record and virtually
    // unwind call frames until the system entry trap frame is encountered.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the address
        // where control left the caller. Otherwise, the function is a leaf
        // function and the return address register contains the address of
        // where control left the caller.
        //

        if (FunctionEntry != NULL) {
            ControlPc = RtlVirtualUnwind(ControlPc,
                                         FunctionEntry,
                                         &ContextRecord,
                                         &InFunction,
                                         &EstablisherFrame,
                                         &ContextPointers);

        } else {
            ControlPc = (ULONG_PTR)ContextRecord.IntRa;
        }

        //
        // N.B. The virtual frame pointer of the kernel frame just below the
        // trap frame may not be exactly equal to TrapFrame1 since the trap
        // code can allocate additional stack space (for local variables or
        // system service arguments). Therefore we must check for a range of
        // values at or below TrapFrame1, but also above TrapFrame2 (so in
        // case there is more than one trap frame on the stack, we get the
        // one at the top.
        //

    } while ((ContextRecord.IntSp != TrapFrame1) &&
             ((ContextRecord.IntSp < TrapFrame2) ||
              (ControlPc < PCR->SystemServiceDispatchStart) ||
              (ControlPc >= PCR->SystemServiceDispatchEnd)));

    //
    // If system argument one is nonzero, then set the context of the current
    // thread. Otherwise, get the context of the current thread.
    //

    if (Apc->SystemArgument1 != 0) {

        //
        // Set context of current thread.
        //

        PspSetContext((PKTRAP_FRAME)TrapFrame1,
                      &ContextPointers,
                      &ContextBlock->Context,
                      ContextBlock->Mode);

    } else {

        //
        // Get context of current thread.
        //

        PspGetContext((PKTRAP_FRAME)TrapFrame1,
                      &ContextPointers,
                      &ContextBlock->Context);
    }

    KeSetEvent(&ContextBlock->OperationComplete, 0, FALSE);
    return;
}
