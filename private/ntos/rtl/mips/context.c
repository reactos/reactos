/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module implements user-mode callable context manipulation routines.

Author:

    Mark Lucovsky (markl) 20-Jun-1989

Revision History:

    David N. Cutler (davec) 18-Apr-1990

        Revise for MIPS environment.

--*/

#include <ntos.h>

VOID
RtlInitializeContext(
    IN HANDLE Process,
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL
    )

/*++

Routine Description:

    This function initializes a context structure so that it can be used in
    a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a pointer to a context record that is to be initialized.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

Return Value:

    Raises STATUS_BAD_INITIAL_STACK if the value of InitialSp is not properly
           aligned.

    Raises STATUS_BAD_INITIAL_PC if the value of InitialPc is not properly
           aligned.

--*/

{

    //
    // Check for proper initial stack and PC alignment.
    //

    if (((ULONG)InitialSp & 0x7) != 0) {
        RtlRaiseStatus(STATUS_BAD_INITIAL_STACK);
    }
    if (((ULONG)InitialPc & 0x3) != 0) {
        RtlRaiseStatus(STATUS_BAD_INITIAL_PC);
    }

    //
    // Initialize the integer registers to contain their register number.
    //

    Context->XIntZero = 0;
    Context->XIntAt = 1;
    Context->XIntV0 = 2;
    Context->XIntV1 = 3;
    Context->XIntA0 = 4;
    Context->XIntA1 = 5;
    Context->XIntA2 = 6;
    Context->XIntA3 = 7;
    Context->XIntT0 = 8;
    Context->XIntT1 = 9;
    Context->XIntT2 = 10;
    Context->XIntT3 = 11;
    Context->XIntT4 = 12;
    Context->XIntT5 = 13;
    Context->XIntT6 = 14;
    Context->XIntT7 = 15;
    Context->XIntS0 = 16;
    Context->XIntS1 = 17;
    Context->XIntS2 = 18;
    Context->XIntS3 = 19;
    Context->XIntS4 = 20;
    Context->XIntS5 = 21;
    Context->XIntS6 = 22;
    Context->XIntS7 = 23;
    Context->XIntT8 = 24;
    Context->XIntT9 = 25;
    Context->XIntS8 = 30;
    Context->XIntLo = 0;
    Context->XIntHi = 0;

    //
    // Initialize the floating point registers to contain zero in their upper
    // half and the integer value of their register number in the lower half.
    //

    Context->FltF0 = 0;
    Context->FltF1 = 0;
    Context->FltF2 = 2;
    Context->FltF3 = 0;
    Context->FltF4 = 4;
    Context->FltF5 = 0;
    Context->FltF6 = 6;
    Context->FltF7 = 0;
    Context->FltF8 = 8;
    Context->FltF9 = 0;
    Context->FltF10 = 10;
    Context->FltF11 = 0;
    Context->FltF12 = 12;
    Context->FltF13 = 0;
    Context->FltF14 = 14;
    Context->FltF15 = 0;
    Context->FltF16 = 16;
    Context->FltF17 = 0;
    Context->FltF18 = 18;
    Context->FltF19 = 0;
    Context->FltF20 = 20;
    Context->FltF21 = 0;
    Context->FltF22 = 22;
    Context->FltF23 = 0;
    Context->FltF24 = 24;
    Context->FltF25 = 0;
    Context->FltF26 = 26;
    Context->FltF27 = 0;
    Context->FltF28 = 28;
    Context->FltF29 = 0;
    Context->FltF30 = 30;
    Context->FltF31 = 0;
    Context->Fsr = 0;

    //
    // Initialize the control registers.
    //
    // N.B. The register gp is estabished at thread startup by the loader.
    //

    Context->XIntGp = 0;
    Context->XIntSp = (LONG)InitialSp;
    Context->XIntRa = 1;
    Context->Fir = (ULONG)InitialPc;
    Context->Psr = 0;
    Context->ContextFlags = CONTEXT_FULL;

    //
    // Set the initial context of the thread in a machine specific way.
    //

    Context->XIntA0 = (LONG)Parameter;
    Context->XIntSp -= KTRAP_FRAME_ARGUMENTS;
}

NTSTATUS
RtlRemoteCall(
    HANDLE Process,
    HANDLE Thread,
    PVOID CallSite,
    ULONG ArgumentCount,
    PULONG Arguments,
    BOOLEAN PassContext,
    BOOLEAN AlreadySuspended
    )

/*++

Routine Description:

    This function calls a procedure in another thread/process,  by using
    NtGetContext and NtSetContext. Parameters are passed to the target
    procedure via the nonvolatile registers (s0 - s7).

Arguments:

    Process - Supplies an open handle to the target process.

    Thread - Supplies an open handle to the target thread within the target
        process.

    CallSize - Supplies the address of the procedure to call in the target
        process.

    ArgumentCount - Supplies the number of 32 bit parameters to pass to the
        target procedure.

    Arguments - Supplies a pointer to the array of 32 bit parameters to pass.

    PassContext - Supplies a boolean value that determines whether a parameter
        is to be passed that points to a context record. This parameter is
        ignored on MIPS hosts.

    AlreadySuspended - Supplies a boolean value that determines whether the
        target thread is already in a suspended or waiting state.

Return Value:

    Status - Status value

--*/

{

    NTSTATUS Status;
    CONTEXT Context;
    ULONG NewSp;

    if (ArgumentCount > 8) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // If necessary, suspend the target thread before getting the thread's
    // current state.
    //

    if (AlreadySuspended == FALSE) {
        Status = NtSuspendThread(Thread, NULL);
        if (NT_SUCCESS(Status) == FALSE) {
            return(Status);
        }
    }

    //
    // Get the cuurent state of the target thread.
    //

    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(Thread, &Context);
    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }

        return Status;
    }

    if (AlreadySuspended) {
        Context.XIntV0 = (LONG)STATUS_ALERTED;
    }

    //
    // Pass the parameters to the other thread via the non-volatile registers
    // s0 - s7. The context record is passed on the stack of the target thread.
    //

    NewSp = (ULONG)(Context.XIntSp - sizeof(CONTEXT));
    Status = NtWriteVirtualMemory(Process,
                                  (PVOID)NewSp,
                                  &Context,
                                  sizeof(CONTEXT),
                                  NULL);

    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }

        return Status;
    }

    Context.XIntSp = (LONG)NewSp;
    if (PassContext) {
        Context.XIntS0 = (LONG)NewSp;
        RtlMoveMemory(&Context.XIntS1, Arguments, ArgumentCount * sizeof(ULONG));

    } else {
        RtlMoveMemory(&Context.XIntS0, Arguments, ArgumentCount * sizeof(ULONG));
    }

    //
    // Set the address of the target code into FIR and set the thread context
    // to cause the target procedure to be executed.
    //

    Context.Fir = (ULONG)CallSite;;
    Status = NtSetContextThread(Thread, &Context);
    if (AlreadySuspended == FALSE) {
        NtResumeThread(Thread, NULL);
    }

    return Status;
}
