/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module implements user-mode callable context manipulation routines.
    The interfaces exported from this module are portable, but they must
    be re-implemented for each architecture.

Author:

    Mark Lucovsky (markl) 20-Jun-1989

Revision History:

    Bryan Willman (bryanwi) 8-Mar-90

	Ported to the 80386

--*/

#include "ntrtlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlInitializeContext)
#pragma alloc_text(PAGE,RtlRemoteCall)
#endif


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

    This function initializes a context structure so that it can
    be used in a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a context buffer to be initialized by this routine.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

Return Value:

    Raises STATUS_BAD_INITIAL_STACK if the value of InitialSp is not properly
           aligned.

    Raises STATUS_BAD_INITIAL_PC if the value of InitialPc is not properly
           aligned.

--*/

{
    RTL_PAGED_CODE();

    Context->Eax = 0L;
    Context->Ebx = 1L;
    Context->Ecx = 2L;
    Context->Edx = 3L;
    Context->Esi = 4L;
    Context->Edi = 5L;
    Context->Ebp = 0L;

    Context->SegGs = 0;
    Context->SegFs = KGDT_R3_TEB;
    Context->SegEs = KGDT_R3_DATA;
    Context->SegDs = KGDT_R3_DATA;
    Context->SegSs = KGDT_R3_DATA;
    Context->SegCs = KGDT_R3_CODE;

    Context->EFlags = 0x200L;	    // force interrupts on, clear all else.

    //
    // Even though these are optional, they are used as is, since NULL
    // is what these would have been initialized to anyway
    //

    Context->Esp = (ULONG) InitialSp;
    Context->Eip = (ULONG) InitialPc;

    //
    // add code to check alignment and raise exception...
    //

    Context->ContextFlags = CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS;

    //
    // Set the initial context of the thread in a machine specific way.
    // ie, pass the initial parameter to the start address
    //

    Context->Esp -= sizeof(Parameter);
    ZwWriteVirtualMemory(Process,
			 (PVOID)Context->Esp,
			 (PVOID)&Parameter,
			 sizeof(Parameter),
			 NULL);
    Context->Esp -= sizeof(Parameter); // Reserve room for ret address


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

    This function calls a procedure in another thread/process, using
    NtGetContext and NtSetContext.  Parameters are passed to the
    target procedure via its stack.

Arguments:

    Process - Handle of the target process

    Thread - Handle of the target thread within that process

    CallSite - Address of the procedure to call in the target process.

    ArgumentCount - Number of 32 bit parameters to pass to the target
                    procedure.

    Arguments - Pointer to the array of 32 bit parameters to pass.

    PassContext - TRUE if an additional parameter is to be passed that
        points to a context record.

    AlreadySuspended - TRUE if the target thread is already in a suspended
                       or waiting state.

Return Value:

    Status - Status value

--*/

{
    NTSTATUS Status;
    CONTEXT Context;
    ULONG NewSp;
    ULONG ArgumentsCopy[5];

    RTL_PAGED_CODE();

    if (ArgumentCount > 4)
        return STATUS_INVALID_PARAMETER;

    //
    // If necessary, suspend the guy before with we mess with his stack.
    //
    if (!AlreadySuspended) {
        Status = NtSuspendThread( Thread, NULL );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }
        }


    //
    // Get the context record for the target thread.
    //

    Context.ContextFlags = CONTEXT_FULL;
    Status = NtGetContextThread( Thread, &Context );
    if (!NT_SUCCESS( Status )) {
        if (!AlreadySuspended) {
            NtResumeThread( Thread, NULL );
            }
        return( Status );
        }


    //
    //	Pass all parameters on the stack, regardless of whether a
    //	a context record is passed.
    //

    //
    //	Put Context Record on stack first, so it is above other args.
    //
    NewSp = Context.Esp;
    if (PassContext) {
	NewSp -= sizeof( CONTEXT );
	Status = NtWriteVirtualMemory( Process,
				       (PVOID)NewSp,
				       &Context,
				       sizeof( CONTEXT ),
				       NULL
				    );
	if (!NT_SUCCESS( Status )) {
            if (!AlreadySuspended) {
                NtResumeThread( Thread, NULL );
                }
	    return( Status );
	    }
        ArgumentsCopy[0] = NewSp;   // pass pointer to context
        RtlMoveMemory(&(ArgumentsCopy[1]),Arguments,ArgumentCount*sizeof( ULONG ));
        ArgumentCount++;
	}
    else {
        RtlMoveMemory(ArgumentsCopy,Arguments,ArgumentCount*sizeof( ULONG ));
        }

    //
    //	Copy the arguments onto the target stack
    //
    if (ArgumentCount) {
        NewSp -= ArgumentCount * sizeof( ULONG );
        Status = NtWriteVirtualMemory( Process,
                                       (PVOID)NewSp,
                                       ArgumentsCopy,
                                       ArgumentCount * sizeof( ULONG ),
                                       NULL
                                     );
        if (!NT_SUCCESS( Status )) {
            if (!AlreadySuspended) {
                NtResumeThread( Thread, NULL );
                }
            return( Status );
            }
        }

    //
    // Set the address of the target code into Eip, the new target stack
    // into Esp, and reload context to make it happen.
    //
    Context.Esp = NewSp;
    Context.Eip = (ULONG)CallSite;
    Status = NtSetContextThread( Thread, &Context );
    if (!AlreadySuspended) {
        NtResumeThread( Thread, NULL );
        }

    return( Status );
}
