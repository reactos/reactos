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

    Thomas Van Baak (tvb) 11-May-1992

        Adapted for Alpha AXP.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <alphaops.h>

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

    Process - Supplies an open handle to the target process. This argument
        is ignored by this function.

    Context - Supplies a pointer to a context record that is to be initialized.

    Parameter - Supplies an initial value for register A0.

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

    if (((ULONG_PTR)InitialSp & 0xF) != 0) {
        RtlRaiseStatus(STATUS_BAD_INITIAL_STACK);
    }
    if (((ULONG_PTR)InitialPc & 0x3) != 0) {
        RtlRaiseStatus(STATUS_BAD_INITIAL_PC);
    }

    //
    // Initialize the integer registers to contain their register number
    // (they must be initialized and using register numbers instead of 0
    // can be useful for debugging). Integer registers a0, ra, gp, and sp
    // are set later.
    //

    Context->IntV0 = 0;
    Context->IntT0 = 1;
    Context->IntT1 = 2;
    Context->IntT2 = 3;
    Context->IntT3 = 4;
    Context->IntT4 = 5;
    Context->IntT5 = 6;
    Context->IntT6 = 7;
    Context->IntT7 = 8;
    Context->IntS0 = 9;
    Context->IntS1 = 10;
    Context->IntS2 = 11;
    Context->IntS3 = 12;
    Context->IntS4 = 13;
    Context->IntS5 = 14;
    Context->IntFp = 15;
    Context->IntA1 = 17;
    Context->IntA2 = 18;
    Context->IntA3 = 19;
    Context->IntA4 = 20;
    Context->IntA5 = 21;
    Context->IntT8 = 22;
    Context->IntT9 = 23;
    Context->IntT10 = 24;
    Context->IntT11 = 25;
    Context->IntT12 = 27;
    Context->IntAt = 28;

    //
    // Initialize the floating point registers to contain the integer value
    // of their register number (they must be initialized and using register
    // numbers instead of 0 can be useful for debugging).
    //

    Context->FltF0 = 0;
    Context->FltF1 = 1;
    Context->FltF2 = 2;
    Context->FltF3 = 3;
    Context->FltF4 = 4;
    Context->FltF5 = 5;
    Context->FltF6 = 6;
    Context->FltF7 = 7;
    Context->FltF8 = 8;
    Context->FltF9 = 9;
    Context->FltF10 = 10;
    Context->FltF11 = 11;
    Context->FltF12 = 12;
    Context->FltF13 = 13;
    Context->FltF14 = 14;
    Context->FltF15 = 15;
    Context->FltF16 = 16;
    Context->FltF17 = 17;
    Context->FltF18 = 18;
    Context->FltF19 = 19;
    Context->FltF20 = 20;
    Context->FltF21 = 21;
    Context->FltF22 = 22;
    Context->FltF23 = 23;
    Context->FltF24 = 24;
    Context->FltF25 = 25;
    Context->FltF26 = 26;
    Context->FltF27 = 27;
    Context->FltF28 = 28;
    Context->FltF29 = 29;
    Context->FltF30 = 30;
    Context->FltF31 = 0;

    //
    // Initialize the control registers.
    //
    // Gp: will be set in LdrpInitialize at thread startup.
    // Ra: some debuggers compare for 1 as a top-of-stack indication.
    //
    // N.B. On 32-bit systems, ULONG becomes canonical longword with the
    //      (ULONGLONG)(LONG) cast.
    //

    Context->IntGp = 0;
    Context->IntSp = (ULONGLONG)(LONG_PTR)InitialSp;
    Context->IntRa = 1;
    Context->Fir = (ULONGLONG)(LONG_PTR)InitialPc;

    //
    // Set default Alpha floating point control register values.
    //

    Context->Fpcr = (ULONGLONG)0;
    ((PFPCR)(&Context->Fpcr))->DynamicRoundingMode = ROUND_TO_NEAREST;
    Context->SoftFpcr = (ULONGLONG)0;

    Context->Psr = 0;
    Context->ContextFlags = CONTEXT_FULL;

    //
    // Set the initial context of the thread in a machine specific way.
    //

    Context->IntA0 = (ULONGLONG)(LONG_PTR)Parameter;
}

NTSTATUS
RtlRemoteCall(
    HANDLE Process,
    HANDLE Thread,
    PVOID CallSite,
    ULONG ArgumentCount,
    PULONG_PTR Arguments,
    BOOLEAN PassContext,
    BOOLEAN AlreadySuspended
    )

/*++

Routine Description:

    This function calls a procedure in another thread/process by using
    NtGetContext and NtSetContext. Parameters are passed to the target
    procedure via the nonvolatile registers (s0 - s5).

Arguments:

    Process - Supplies an open handle to the target process.

    Thread - Supplies an open handle to the target thread within the target
        process.

    CallSite - Supplies the address of the procedure to call in the target
        process.

    ArgumentCount - Supplies the number of parameters to pass to the
        target procedure.

    Arguments - Supplies a pointer to the array of parameters to pass.

    PassContext - Supplies a boolean value that determines whether a parameter
        is to be passed that points to a context record. This parameter is
        ignored on MIPS and Alpha hosts.

    AlreadySuspended - Supplies a boolean value that determines whether the
        target thread is already in a suspended or waiting state.

Return Value:

    Status - Status value.

--*/

{

    NTSTATUS Status;
    CONTEXT Context;
    ULONG Index;
    ULONGLONG NewSp;

    if ((ArgumentCount > 6) ||
        (PassContext && (ArgumentCount > 5))) {
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
    // Get the current state of the target thread.
    //

    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread(Thread, &Context);
    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }
        return(Status);
    }

    if (AlreadySuspended) {
        Context.IntV0 = STATUS_ALERTED;
    }

    //
    // Pass the parameters to the other thread via the non-volatile registers
    // s0 - s5. The context record is passed on the stack of the target thread.
    //

    NewSp = Context.IntSp - sizeof(CONTEXT);
    Status = NtWriteVirtualMemory(Process, (PVOID)NewSp, &Context,
                                  sizeof(CONTEXT), NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }
        return(Status);
    }

    //
    // N.B. On 32-bit system each ULONG argument is converted to canonical
    //      form with the (ULONGLONG)(LONG) cast as required by the calling
    //      standard.
    //

    Context.IntSp = NewSp;

    if (PassContext) {
        Context.IntS0 = NewSp;
        for (Index = 0; Index < ArgumentCount; Index += 1) {
            (&Context.IntS1)[Index] = (ULONGLONG)(LONG_PTR)Arguments[Index];
        }

    } else {
        for (Index = 0; Index < ArgumentCount; Index += 1) {
            (&Context.IntS0)[Index] = (ULONGLONG)(LONG_PTR)Arguments[Index];
        }
    }

    //
    // Set the address of the target code into FIR and set the thread context
    // to cause the target procedure to be executed.
    //
    // N.B. The PVOID CallSite is stored as a canonical longword in order
    //      for Fir to be a valid 64-bit address.
    //

    Context.Fir = (ULONGLONG)(LONG_PTR)CallSite;
    Status = NtSetContextThread(Thread, &Context);
    if (AlreadySuspended == FALSE) {
        NtResumeThread(Thread, NULL);
    }
    return(Status);
}
