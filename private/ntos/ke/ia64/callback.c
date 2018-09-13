/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    callback.c

Abstract:

    This module implements user mode call back services.

Author:

    William K. Cheung (wcheung) 30-Oct-1995

    based on David N. Cutler (davec) 29-Oct-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

NTSTATUS
KeUserModeCallback (
    IN ULONG ApiNumber,
    IN PVOID InputBuffer,
    IN ULONG InputLength,
    OUT PVOID *OutputBuffer,
    IN PULONG OutputLength
    )

/*++

Routine Description:

    This function call out from kernel mode to a user mode function.

Arguments:

    ApiNumber - Supplies the API number.

    InputBuffer - Supplies a pointer to a structure that is copied
        to the user stack.

    InputLength - Supplies the length of the input structure.

    Outputbuffer - Supplies a pointer to a variable that receives
        the address of the output buffer.

    Outputlength - Supplies a pointer to a variable that receives
        the length of the output buffer.

Return Value:

    If the callout cannot be executed, then an error status is
    returned. Otherwise, the status returned by the callback function
    is returned.

--*/

{
    PUCALLOUT_FRAME CalloutFrame;
    ULONGLONG OldStack;
    ULONGLONG NewStack;
    ULONGLONG OldStIFS;
    PKTRAP_FRAME TrapFrame;
    NTSTATUS Status;
    ULONG Length;
    SHORT BsFrameSize;
    USHORT TearPointOffset;

    ASSERT(KeGetPreviousMode() == UserMode);

    //
    // Get the user mode stack pointer and attempt to copy input buffer
    // to the user stack.
    //

    TrapFrame = KeGetCurrentThread()->TrapFrame;
    OldStack = TrapFrame->IntSp;
    OldStIFS = TrapFrame->StIFS;

    try {

        //
        // Compute new user mode stack address, probe for writability,
        // and copy the input buffer to the user stack.
        //
        // N.B. EM requires stacks to be 16-byte aligned, therefore
        //      the input length must be rounded up to a 16-byte boundary.
        //

        Length =  (InputLength + 16 - 1 + sizeof(UCALLOUT_FRAME) + STACK_SCRATCH_AREA) & ~(16 - 1);
        NewStack = OldStack - Length;
        CalloutFrame = (PUCALLOUT_FRAME)(NewStack + STACK_SCRATCH_AREA);
        ProbeForWrite((PVOID)NewStack, Length, sizeof(QUAD));
        RtlCopyMemory(CalloutFrame + 1, InputBuffer, InputLength);

        //
        // Fill in the callout arguments.
        //

        CalloutFrame->Buffer = (PVOID)(CalloutFrame + 1);
        CalloutFrame->Length = InputLength;
        CalloutFrame->ApiNumber = ApiNumber;
        CalloutFrame->IntSp = OldStack;
        CalloutFrame->RsPFS = TrapFrame->StIFS;
        CalloutFrame->BrRp = TrapFrame->BrRp;

      if (PsGetCurrentProcess()->DebugPort != NULL) {
        KiFlushRse();
        BsFrameSize = (SHORT)(TrapFrame->RsBSP - TrapFrame->RsBSPSTORE);
        if (BsFrameSize) {

            ULONGLONG TopBound, BottomBound;
            ULONGLONG UserRnats1, UserRnats2;
            ULONGLONG Mask;
            ULONGLONG CurrentRNAT = 0;
            SHORT RNatSaveIndex;

            //
            // Copy the dirty stacked registers back into the
            // user backing store
            //

            TearPointOffset = (USHORT) TrapFrame->RsBSPSTORE & 0x1F8;
            RtlCopyMemory((PVOID)(TrapFrame->RsBSPSTORE),
                         (PVOID)(PCR->InitialBStore + TearPointOffset),
                         BsFrameSize);

            TopBound = TrapFrame->RsBSP | RNAT_ALIGNMENT;
            BottomBound = TrapFrame->RsBSPSTORE | RNAT_ALIGNMENT;

            RNatSaveIndex = TearPointOffset >> 3;
            Mask = (((1ULL << (NAT_BITS_PER_RNAT_REG - RNatSaveIndex)) - 1) << RNatSaveIndex);
            UserRnats1 = TrapFrame->RsRNAT & ((1ULL << RNatSaveIndex) - 1);

            if (TopBound > BottomBound) {

                //
                // user dirty stacked GR span across at least one RNAT
                // boundary; need to deposit the valid RNAT bits from
                // the trap frame into the kernel backing store.  Also,
                // the RNAT field in the trap frame has to be updated.
                //

                UserRnats2 = *(PULONGLONG)BottomBound & Mask;
                *(PULONGLONG)BottomBound = UserRnats1 | UserRnats2;
                TrapFrame->RsRNAT = CurrentRNAT;
                
            } else {

                //
                // user stacked register region does not span across an
                // RNAT boundary; combine the RNAT fields from both the
                // trap frame and the context frame.
                //

                UserRnats2 = CurrentRNAT & Mask;
                TrapFrame->RsRNAT = UserRnats1 | UserRnats2;

            }

            TrapFrame->RsBSPSTORE = TrapFrame->RsBSP;
        }
      }

    //
    // If an exception occurs during the probe of the user stack, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Call user mode.
    //

    TrapFrame->StIFS = 0xC000000000000000i64;
    TrapFrame->IntSp = NewStack;
    Status = KiCallUserMode(OutputBuffer, OutputLength);

    //
    // When returning from user mode, any drawing done to the GDI TEB
    // batch must be flushed.
    //

    if (((PTEB)KeGetCurrentThread()->Teb)->GdiBatchCount > 0) {

        //
        // call GDI batch flush routine
        //

        KeGdiFlushUserBatch();
    }

    TrapFrame->IntSp = OldStack;
    TrapFrame->StIFS = OldStIFS;
    return Status;

}

NTSTATUS
NtW32Call (
    IN ULONG ApiNumber,
    IN PVOID InputBuffer,
    IN ULONG InputLength,
    OUT PVOID *OutputBuffer,
    OUT PULONG OutputLength
    )

/*++

Routine Description:

    This function calls a W32 function.

    N.B. ************** This is a temporary service *****************

Arguments:

    ApiNumber - Supplies the API number.

    InputBuffer - Supplies a pointer to a structure that is copied to
        the user stack.

    InputLength - Supplies the length of the input structure.

    Outputbuffer - Supplies a pointer to a variable that recevies the
        output buffer address.

    Outputlength - Supplies a pointer to a variable that recevies the
        output buffer length.

Return Value:

    TBS.

--*/

{

    PVOID ValueBuffer;
    ULONG ValueLength;
    NTSTATUS Status;

    ASSERT(KeGetPreviousMode() == UserMode);

    //
    // If the current thread is not a GUI thread, then fail the service
    // since the thread does not have a large stack.
    //

    if (KeGetCurrentThread()->Win32Thread == (PVOID)&KeServiceDescriptorTable[0]) {
        return STATUS_NOT_IMPLEMENTED;
    }

    //
    // Probe the output buffer address and length for writeability.
    //

    try {
        ProbeForWriteUlong((PULONG)OutputBuffer);
        ProbeForWriteUlong(OutputLength);

    //
    // If an exception occurs during the probe of the output buffer or
    // length, then always handle the exception and return the exception
    // code as the status value.
    //

    } except(EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Call out to user mode specifying the input buffer and API number.
    //

    Status = KeUserModeCallback(ApiNumber,
                                InputBuffer,
                                InputLength,
                                &ValueBuffer,
                                &ValueLength);

    //
    // If the callout is successful, then the output buffer address and
    // length.
    //

    if (NT_SUCCESS(Status)) {
        try {
            *OutputBuffer = ValueBuffer;
            *OutputLength = ValueLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {
        }
    }

    return Status;
}
