/*++

Copyright (c) 1993-1995  Microsoft Corporation

Module Name:

    xipi.c

Abstract:

    This module implements portable interprocessor interrup routines.

Author:

    David N. Cutler (davec) 24-Apr-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define forward reference function prototypes.
//

VOID
KiIpiGenericCallTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID BroadcastFunction,
    IN PVOID Context,
    IN PVOID Parameter3
    );

ULONG_PTR
KiIpiGenericCall (
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG_PTR Context
    )

/*++

Routine Description:

    This function executes the specified function on every processor in
    the host configuration in a synchronous manner, i.e., the function
    is executed on each target in series with the execution of the source
    processor.

Arguments:

    BroadcastFunction - Supplies the address of function that is executed
        on each of the target processors.

    Context - Supplies the value of the context parameter that is passed
        to each function.

Return Value:

    The value returned by the specified function on the source processor
    is returned as the function value.

--*/

{

    KIRQL OldIrql;
    ULONG_PTR Status;
    KAFFINITY TargetProcessors;

    //
    // Raise IRQL to the higher of the current level and synchronization
    // level to avoid a possible context switch.
    //

    KeRaiseIrql((KIRQL)(max(KiSynchIrql, KeGetCurrentIrql())), &OldIrql);

    //
    // Initialize the broadcast packet, compute the set of target processors,
    // and sent the packet to the target processors for execution.
    //

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors & ~KeGetCurrentPrcb()->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiIpiGenericCallTarget,
                        (PVOID)BroadcastFunction,
                        (PVOID)Context,
                        NULL);
    }

#endif

    //
    // Execute function of source processor and capture return status.
    //

    Status = BroadcastFunction(Context);

    //
    // Wait until all of the target processors have finished capturing the
    // function parameters.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Lower IRQL to its previous level and return the function execution
    // status.
    //

    KeLowerIrql(OldIrql);
    return Status;
}

#if !defined(NT_UP)


VOID
KiIpiGenericCallTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID BroadcastFunction,
    IN PVOID Context,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This function is the target jacket function to execute a broadcast
    function on a set of target processors. The broadcast packet address
    is obtained, the specified parameters are captured, the broadcast
    packet address is cleared to signal the source processor to continue,
    and the specified function is executed.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    BroadcastFunction - Supplies the address of function that is executed
        on each of the target processors.

    Context - Supplies the value of the context parameter that is passed
        to each function.

    Parameter3 - Not used.

Return Value:

    None

--*/

{

    //
    // Execute the specified function.
    //

    ((PKIPI_BROADCAST_WORKER)(BroadcastFunction))((ULONG_PTR)Context);
    KiIpiSignalPacketDone(SignalDone);
    return;
}



VOID
KiIpiStallOnPacketTargets (
    KAFFINITY TargetSet
    )

/*++

Routine Description:

    This function waits until the specified set of processors have signaled
    their completion of a requested function.

    N.B. The exact protocol used between the source and the target of an
         interprocessor request is not specified. Minimally the source
         must construct an appropriate packet and send the packet to a set
         of specified targets. Each target receives the address of the packet
         address as an argument, and minimally must clear the packet address
         when the mutually agreed upon protocol allows. The target has three
         options:

         1. Capture necessary information, release the source by clearing
            the packet address, execute the request in parallel with the
            source, and return from the interrupt.

         2. Execute the request in series with the source, release the
            source by clearing the packet address, and return from the
            interrupt.

         3. Execute the request in series with the source, release the
            source, wait for a reply from the source based on a packet
            parameter, and return from the interrupt.

    This function is provided to enable the source to synchronize with the
    target for cases 2 and 3 above.

    N.B. There is no support for method 3 above.

Arguments:

    TargetSet - Supplies the the target set of IPI processors.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Wait until the target set of processors is zero in the current
    // processor's packet.
    //

    Prcb = KeGetCurrentPrcb();
    while (Prcb->TargetSet != 0) {
        KeYieldProcessor();
    }

    //
    // If the target set is equal to the entire set of processors, then
    // update the memory barrier time stamp.
    //

#if defined(_ALPHA_)

    if ((TargetSet | PCR->SetMember ) == KeActiveProcessors) {
        InterlockedIncrement(&KiMbTimeStamp);
    }

#endif

    return;
}

#endif
