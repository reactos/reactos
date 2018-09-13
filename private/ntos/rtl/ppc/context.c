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
    Rick Simpson, Peter Johnston Conversion to PowerPC 11/5/93

        Revise for MIPS environment.

--*/

#include <ntos.h>
#define _KXPPC_C_HEADER_
#include <kxppc.h>

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
    // Initialize the integer and floating registers to contain zeroes.
    //

    RtlZeroMemory(Context, sizeof(CONTEXT));

    //
    // Initialize the control registers.
    //

    if ( ARGUMENT_PRESENT(InitialPc) ) {
        Context->Iar = (ULONG)InitialPc;
    }
    if ( ARGUMENT_PRESENT(InitialSp) ) {
        Context->Gpr1 = (ULONG)InitialSp - STK_MIN_FRAME;
    }

    Context->Msr =
        MASK_SPR(MSR_ILE,1)|
        MASK_SPR(MSR_FP,1) |
        MASK_SPR(MSR_FE0,1)|
        MASK_SPR(MSR_FE1,1)|
        MASK_SPR(MSR_ME,1) |
        MASK_SPR(MSR_IR,1) |
        MASK_SPR(MSR_DR,1) |
        MASK_SPR(MSR_PR,1) |
        MASK_SPR(MSR_LE,1);
    Context->ContextFlags = CONTEXT_FULL;

    //
    // Set the initial context of the thread in a machine specific way.
    //

    Context->Gpr3 = (ULONG)Parameter;
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

    if (AlreadySuspended ) {

        Context.Gpr3 = STATUS_ALERTED;
    }

    //
    // Pass the parameters to the other thread via the non-volatile registers
    // s0 - s7. The context record is passed on the stack of the target thread.
    //

    NewSp = Context.Gpr1 - sizeof(CONTEXT) - STK_MIN_FRAME;
    Status = NtWriteVirtualMemory(Process, (PVOID)(NewSp + STK_MIN_FRAME), &Context,
                                  sizeof(CONTEXT), NULL);
    Status = NtWriteVirtualMemory(Process, (PVOID)NewSp, &Context.Gpr1,
                                  sizeof(ULONG), NULL);
    if (NT_SUCCESS(Status) == FALSE) {
        if (AlreadySuspended == FALSE) {
            NtResumeThread(Thread, NULL);
        }
        return(Status);
    }

    Context.Gpr1 = NewSp;

    if (PassContext) {
        Context.Gpr14 = NewSp + STK_MIN_FRAME;
        RtlMoveMemory(&Context.Gpr15, Arguments, ArgumentCount * sizeof(ULONG));

    } else {

        RtlMoveMemory(&Context.Gpr14, Arguments, ArgumentCount * sizeof(ULONG));
    }

    //
    // Set the address of the target code into FIR and set the thread context
    // to cause the target procedure to be executed.
    //

    Context.Iar = (ULONG)CallSite;      // Set context will dereference the
    Context.Gpr2 = 0;                   // FNDESC in the remote threads context
    Status = NtSetContextThread(Thread, &Context);
    if (AlreadySuspended == FALSE) {
        NtResumeThread(Thread, NULL);
    }
    return(Status);
}
