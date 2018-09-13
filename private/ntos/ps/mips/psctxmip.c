/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psctxmip.c

Abstract:

    This module implements function to get and set the context of a thread.

Author:

    David N. Cutler (davec) 1-Oct-1990

Revision History:

--*/

#include "psp.h"

ULONGLONG
PspGetSavedValue (
    IN PVOID ContextPointer
    )

/*++

Routine Description:

    This function loads the context value specified by the argument
    pointer.

    N.B. The low bit of the pointer specifies the operand type.

Arguments:

    ContextPointer - Supplies a pointer to the context value that is
        loaded.

Return Value:

    The value specified by the argument pointer.

--*/

{

    //
    // If the low bit of the argument pointer is zero, then the argument
    // value is 32-bits. Otherwise, the argument value is 64-bits.
    //

    if (((ULONG)ContextPointer & 1) != 0) {
        return *((PULONGLONG)((ULONG)ContextPointer & ~1));

    } else {
        return *((PLONG)ContextPointer);
    }
}

VOID
PspSetSavedValue (
    IN ULONGLONG ContextValue,
    IN PVOID ContextPointer
    )

/*++

Routine Description:

    This function stores the context value specified in the location
    specified by the argument pointer.

    N.B. The low bit of the pointer specifies the operand type.

Arguments:

    ContextValue - Supplies the context value to be stored.

    ContextPointer - Supplies a pointer to the context value that is
        stored.

Return Value:

    None.

--*/

{

    //
    // If the low bit of the argument pointer is zero, then the argument
    // value is 32-bits. Otherwise, the argument value is 64-bits.
    //

    if (((ULONG)ContextPointer & 1) != 0) {
        *((PULONGLONG)((ULONG)ContextPointer & ~1)) = ContextValue;

    } else {
        *((PULONG)ContextPointer) = (ULONG)ContextValue;
    }
}

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

    ULONG ContextFlags;
    LONG Index;

    //
    // Get context.
    //

    ContextFlags = ContextRecord->ContextFlags;
    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Get integer registers gp, sp, ra, FIR, and PSR.
        //

        ContextRecord->Fir = TrapFrame->Fir;
        ContextRecord->Psr = TrapFrame->Psr;
        if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
            ContextRecord->IntGp = (ULONG)TrapFrame->XIntGp;
            ContextRecord->IntSp = (ULONG)TrapFrame->XIntSp;
            ContextRecord->IntRa = (ULONG)TrapFrame->XIntRa;

        } else {
            ContextRecord->XIntGp = TrapFrame->XIntGp;
            ContextRecord->XIntSp = TrapFrame->XIntSp;
            ContextRecord->XIntRa = TrapFrame->XIntRa;
        }
    }

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Get integer registers zero, and, at - t9, k0, k1, lo, and hi.
        //

        if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
            ContextRecord->IntZero = 0;
            ContextRecord->IntAt = (ULONG)TrapFrame->XIntAt;
            ContextRecord->IntV0 = (ULONG)TrapFrame->XIntV0;
            ContextRecord->IntV1 = (ULONG)TrapFrame->XIntV1;
            ContextRecord->IntA0 = (ULONG)TrapFrame->XIntA0;
            ContextRecord->IntA1 = (ULONG)TrapFrame->XIntA1;
            ContextRecord->IntA2 = (ULONG)TrapFrame->XIntA2;
            ContextRecord->IntA3 = (ULONG)TrapFrame->XIntA3;
            ContextRecord->IntT0 = (ULONG)TrapFrame->XIntT0;
            ContextRecord->IntT1 = (ULONG)TrapFrame->XIntT1;
            ContextRecord->IntT2 = (ULONG)TrapFrame->XIntT2;
            ContextRecord->IntT3 = (ULONG)TrapFrame->XIntT3;
            ContextRecord->IntT4 = (ULONG)TrapFrame->XIntT4;
            ContextRecord->IntT5 = (ULONG)TrapFrame->XIntT5;
            ContextRecord->IntT6 = (ULONG)TrapFrame->XIntT6;
            ContextRecord->IntT7 = (ULONG)TrapFrame->XIntT7;
            ContextRecord->IntT8 = (ULONG)TrapFrame->XIntT8;
            ContextRecord->IntT9 = (ULONG)TrapFrame->XIntT9;
            ContextRecord->IntK0 = 0;
            ContextRecord->IntK1 = 0;
            ContextRecord->IntLo = (ULONG)TrapFrame->XIntLo;
            ContextRecord->IntHi = (ULONG)TrapFrame->XIntHi;

        } else {
            ContextRecord->XIntZero = 0;
            ContextRecord->XIntAt = TrapFrame->XIntAt;
            ContextRecord->XIntV0 = TrapFrame->XIntV0;
            ContextRecord->XIntV1 = TrapFrame->XIntV1;
            ContextRecord->XIntA0 = TrapFrame->XIntA0;
            ContextRecord->XIntA1 = TrapFrame->XIntA1;
            ContextRecord->XIntA2 = TrapFrame->XIntA2;
            ContextRecord->XIntA3 = TrapFrame->XIntA3;
            ContextRecord->XIntT0 = TrapFrame->XIntT0;
            ContextRecord->XIntT1 = TrapFrame->XIntT1;
            ContextRecord->XIntT2 = TrapFrame->XIntT2;
            ContextRecord->XIntT3 = TrapFrame->XIntT3;
            ContextRecord->XIntT4 = TrapFrame->XIntT4;
            ContextRecord->XIntT5 = TrapFrame->XIntT5;
            ContextRecord->XIntT6 = TrapFrame->XIntT6;
            ContextRecord->XIntT7 = TrapFrame->XIntT7;
            ContextRecord->XIntT8 = TrapFrame->XIntT8;
            ContextRecord->XIntT9 = TrapFrame->XIntT9;
            ContextRecord->XIntK0 = 0;
            ContextRecord->XIntK1 = 0;
            ContextRecord->XIntLo = TrapFrame->XIntLo;
            ContextRecord->XIntHi = TrapFrame->XIntHi;
        }

        //
        // Get nonvolatile integer registers s0 - s7, and s8.
        //

        if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
            Index = 7;
            do {
                if (TrapFrame->SavedFlag == 0) {
                    (&ContextRecord->IntS0)[Index] =
                        (ULONG)PspGetSavedValue((&ContextPointers->XIntS0)[Index]);

                } else {
                    (&ContextRecord->IntS0)[Index] = (ULONG)(&TrapFrame->XIntS0)[Index];
                }

                Index -= 1;
            } while (Index >= 0);
            ContextRecord->IntS8 = (ULONG)TrapFrame->XIntS8;

        } else {
            Index = 7;
            do {
                if (TrapFrame->SavedFlag == 0) {
                    (&ContextRecord->XIntS0)[Index] =
                        PspGetSavedValue((&ContextPointers->XIntS0)[Index]);

                } else {
                    (&ContextRecord->XIntS0)[Index] = (&TrapFrame->XIntS0)[Index];
                }

                Index -= 1;
            } while (Index >= 0);
            ContextRecord->XIntS8 = TrapFrame->XIntS8;
        }
    }

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Get volatile floating registers f0 - f19.
        //

        RtlMoveMemory(&ContextRecord->FltF0, &TrapFrame->FltF0,
                     sizeof(ULONG) * (20));

        //
        // Get nonvolatile floating registers f20 - f31.
        //

        ContextRecord->FltF20 = *ContextPointers->FltF20;
        ContextRecord->FltF21 = *ContextPointers->FltF21;
        ContextRecord->FltF22 = *ContextPointers->FltF22;
        ContextRecord->FltF23 = *ContextPointers->FltF23;
        ContextRecord->FltF24 = *ContextPointers->FltF24;
        ContextRecord->FltF25 = *ContextPointers->FltF25;
        ContextRecord->FltF26 = *ContextPointers->FltF26;
        ContextRecord->FltF27 = *ContextPointers->FltF27;
        ContextRecord->FltF28 = *ContextPointers->FltF28;
        ContextRecord->FltF29 = *ContextPointers->FltF29;
        ContextRecord->FltF30 = *ContextPointers->FltF30;
        ContextRecord->FltF31 = *ContextPointers->FltF31;

        //
        // Get floating status register.
        //

        ContextRecord->Fsr = TrapFrame->Fsr;
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

    ULONG ContextFlags;
    LONG Index;

    //
    // Set context.
    //

    ContextFlags = ContextRecord->ContextFlags;
    if ((ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set integer registers gp, sp, ra, FIR, and PSR.
        //

        TrapFrame->Fir = ContextRecord->Fir;
        TrapFrame->Psr = SANITIZE_PSR(ContextRecord->Psr, ProcessorMode);
        if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
            TrapFrame->XIntGp = (LONG)ContextRecord->IntGp;
            TrapFrame->XIntSp = (LONG)ContextRecord->IntSp;
            TrapFrame->XIntRa = (LONG)ContextRecord->IntRa;

        } else {
            TrapFrame->XIntGp = ContextRecord->XIntGp;
            TrapFrame->XIntSp = ContextRecord->XIntSp;
            TrapFrame->XIntRa = ContextRecord->XIntRa;
        }
    }

    if ((ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set integer registers at - t9, lo, and hi.
        //

        if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
            TrapFrame->XIntAt = (LONG)ContextRecord->IntAt;
            TrapFrame->XIntV0 = (LONG)ContextRecord->IntV0;
            TrapFrame->XIntV1 = (LONG)ContextRecord->IntV1;
            TrapFrame->XIntA0 = (LONG)ContextRecord->IntA0;
            TrapFrame->XIntA1 = (LONG)ContextRecord->IntA1;
            TrapFrame->XIntA2 = (LONG)ContextRecord->IntA2;
            TrapFrame->XIntA3 = (LONG)ContextRecord->IntA3;
            TrapFrame->XIntT0 = (LONG)ContextRecord->IntT0;
            TrapFrame->XIntT1 = (LONG)ContextRecord->IntT1;
            TrapFrame->XIntT2 = (LONG)ContextRecord->IntT2;
            TrapFrame->XIntT3 = (LONG)ContextRecord->IntT3;
            TrapFrame->XIntT4 = (LONG)ContextRecord->IntT4;
            TrapFrame->XIntT5 = (LONG)ContextRecord->IntT5;
            TrapFrame->XIntT6 = (LONG)ContextRecord->IntT6;
            TrapFrame->XIntT7 = (LONG)ContextRecord->IntT7;
            TrapFrame->XIntT8 = (LONG)ContextRecord->IntT8;
            TrapFrame->XIntT9 = (LONG)ContextRecord->IntT9;
            TrapFrame->XIntLo = (LONG)ContextRecord->IntLo;
            TrapFrame->XIntHi = (LONG)ContextRecord->IntHi;

        } else {
            TrapFrame->XIntAt = ContextRecord->XIntAt;
            TrapFrame->XIntV0 = ContextRecord->XIntV0;
            TrapFrame->XIntV1 = ContextRecord->XIntV1;
            TrapFrame->XIntA0 = ContextRecord->XIntA0;
            TrapFrame->XIntA1 = ContextRecord->XIntA1;
            TrapFrame->XIntA2 = ContextRecord->XIntA2;
            TrapFrame->XIntA3 = ContextRecord->XIntA3;
            TrapFrame->XIntT0 = ContextRecord->XIntT0;
            TrapFrame->XIntT1 = ContextRecord->XIntT1;
            TrapFrame->XIntT2 = ContextRecord->XIntT2;
            TrapFrame->XIntT3 = ContextRecord->XIntT3;
            TrapFrame->XIntT4 = ContextRecord->XIntT4;
            TrapFrame->XIntT5 = ContextRecord->XIntT5;
            TrapFrame->XIntT6 = ContextRecord->XIntT6;
            TrapFrame->XIntT7 = ContextRecord->XIntT7;
            TrapFrame->XIntT8 = ContextRecord->XIntT8;
            TrapFrame->XIntT9 = ContextRecord->XIntT9;
            TrapFrame->XIntLo = ContextRecord->XIntLo;
            TrapFrame->XIntHi = ContextRecord->XIntHi;
        }

        //
        // Set nonvolatile integer registers s0 - s7, and s8.
        //

        if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
            Index = 7;
            do {
                if (TrapFrame->SavedFlag == 0) {
                    PspSetSavedValue((LONG)(&ContextRecord->IntS0)[Index],
                                     (&ContextPointers->XIntS0)[Index]);

                } else {
                    (&TrapFrame->XIntS0)[Index] = (LONG)(&ContextRecord->IntS0)[Index];
                }

                Index -= 1;
            } while (Index >= 0);
            TrapFrame->XIntS8 = (LONG)ContextRecord->IntS8;

        } else {
            Index = 7;
            do {
                if (TrapFrame->SavedFlag == 0) {
                    PspSetSavedValue((&ContextRecord->XIntS0)[Index],
                                     (&ContextPointers->XIntS0)[Index]);

                } else {
                    (&TrapFrame->XIntS0)[Index] = (&ContextRecord->XIntS0)[Index];
                }

                Index -= 1;
            } while (Index >= 0);
            TrapFrame->XIntS8 = ContextRecord->XIntS8;
        }
    }

    if ((ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set volatile floating registers f0 - f19.
        //

        RtlMoveMemory(&TrapFrame->FltF0, &ContextRecord->FltF0,
                     sizeof(ULONG) * (20));

        //
        // Set nonvolatile floating registers f20 - f31.
        //

        *ContextPointers->FltF20 = ContextRecord->FltF20;
        *ContextPointers->FltF21 = ContextRecord->FltF21;
        *ContextPointers->FltF22 = ContextRecord->FltF22;
        *ContextPointers->FltF23 = ContextRecord->FltF23;
        *ContextPointers->FltF24 = ContextRecord->FltF24;
        *ContextPointers->FltF25 = ContextRecord->FltF25;
        *ContextPointers->FltF26 = ContextRecord->FltF26;
        *ContextPointers->FltF27 = ContextRecord->FltF27;
        *ContextPointers->FltF28 = ContextRecord->FltF28;
        *ContextPointers->FltF29 = ContextRecord->FltF29;
        *ContextPointers->FltF30 = ContextRecord->FltF30;
        *ContextPointers->FltF31 = ContextRecord->FltF31;

        //
        // Set floating status register.
        //

        TrapFrame->Fsr = SANITIZE_FSR(ContextRecord->Fsr, ProcessorMode);
    }

    return;
}

VOID
PspGetSetContextSpecialApc (
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
    ULONG ControlPc;
    ULONG EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    PETHREAD Thread;
    ULONG TrapFrame1;
    ULONG TrapFrame2;

    //
    // Get the address of the context block and compute the address of the
    // system entry trap frame.
    //

    ContextBlock = CONTAINING_RECORD(Apc, GETSETCONTEXT, Apc);
    Thread = PsGetCurrentThread();
    TrapFrame1 = (ULONG)Thread->Tcb.InitialStack - KTRAP_FRAME_LENGTH;
    TrapFrame2 = (ULONG)Thread->Tcb.InitialStack - KTRAP_FRAME_LENGTH - KTRAP_FRAME_ARGUMENTS;

    //
    // Capture the current thread context and set the initial control PC
    // value.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = (ULONG)ContextRecord.XIntRa;

    //
    // Initialize context pointers for the nonvolatile integer and floating
    // registers.
    //

    ContextPointers.XIntS0 = &ContextRecord.XIntS0;
    ContextPointers.XIntS1 = &ContextRecord.XIntS1;
    ContextPointers.XIntS2 = &ContextRecord.XIntS2;
    ContextPointers.XIntS3 = &ContextRecord.XIntS3;
    ContextPointers.XIntS4 = &ContextRecord.XIntS4;
    ContextPointers.XIntS5 = &ContextRecord.XIntS5;
    ContextPointers.XIntS6 = &ContextRecord.XIntS6;
    ContextPointers.XIntS7 = &ContextRecord.XIntS7;

    ContextPointers.FltF20 = &ContextRecord.FltF20;
    ContextPointers.FltF21 = &ContextRecord.FltF21;
    ContextPointers.FltF22 = &ContextRecord.FltF22;
    ContextPointers.FltF23 = &ContextRecord.FltF23;
    ContextPointers.FltF24 = &ContextRecord.FltF24;
    ContextPointers.FltF25 = &ContextRecord.FltF25;
    ContextPointers.FltF26 = &ContextRecord.FltF26;
    ContextPointers.FltF27 = &ContextRecord.FltF27;
    ContextPointers.FltF28 = &ContextRecord.FltF28;
    ContextPointers.FltF29 = &ContextRecord.FltF29;
    ContextPointers.FltF30 = &ContextRecord.FltF30;
    ContextPointers.FltF31 = &ContextRecord.FltF31;

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
            ControlPc = RtlVirtualUnwind(ControlPc | 1,
                                         FunctionEntry,
                                         &ContextRecord,
                                         &InFunction,
                                         &EstablisherFrame,
                                         &ContextPointers);

        } else {
            ControlPc = (ULONG)ContextRecord.XIntRa;
        }

    } while (((ULONG)ContextRecord.XIntSp != TrapFrame1) &&
             (((ULONG)ContextRecord.XIntSp != TrapFrame2) ||
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
