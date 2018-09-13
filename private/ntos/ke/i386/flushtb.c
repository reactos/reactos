/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1990  Microsoft Corporation

Module Name:

    tbflush.c

Abstract:

    This module implements machine dependent functions to flush
    the translation buffers in an Intel x86 system.

    N.B. This module contains only MP versions of the TB flush routines.
         The UP versions are macros in ke.h
         KeFlushEntireTb remains a routine for the UP system since it is
         exported from the kernel for backwards compatibility.

Author:

    David N. Cutler (davec) 13-May-1989

Environment:

    Kernel mode only.

Revision History:

    Shie-Lin Tzong (shielint) 30-Aug-1990
        Implement MP version of KeFlushSingleTb and KeFlushEntireTb.

--*/

#include "ki.h"

VOID
KiFlushTargetEntireTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Invalid,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushTargetMultipleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );


VOID
KiFlushTargetSingleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

HARDWARE_PTE
KiFlushSingleTbSynchronous (
    IN PVOID Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE PtePointer,
    IN HARDWARE_PTE PteValue
    );

VOID
KiFlushTargetSingleTbSynchronous (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
Ki386UseSynchronousTbFlush (
    IN volatile PLONG Number
    );

#if defined(NT_UP)
#undef KeFlushEntireTb
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,Ki386UseSynchronousTbFlush)
#endif


VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on all processors
    that are currently running threads which are child of the current process
    or flushes the entire translation buffer on all processors in the host
    configuration.

Arguments:

    Invalid - Supplies a boolean value that specifies the reason for flushing
        the translation buffer.

    AllProcessors - Supplies a boolean value that determines which translation
        buffers are to be flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;

    //
    // Compute the target set of processors, disable context switching,
    // and send the flush entire parameters to the target processors,
    // if any, for execution.
    //

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        Prcb = KeGetCurrentPrcb();
        TargetProcessors = KeActiveProcessors;

    } else {
        KiLockContextSwap(&OldIrql);
        Prcb = KeGetCurrentPrcb();
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushTargetEntireTb,
                        NULL,
                        NULL,
                        NULL);

        IPI_INSTRUMENT_COUNT (Prcb->Number, FlushEntireTb);
    }

#endif

    //
    // Flush TB on current processor.
    //

    KeFlushCurrentTb();

    //
    // Wait until all target processors have finished and complete packet.
    //

#if defined(NT_UP)

    KeLowerIrql(OldIrql);

#else

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

#endif

    return;
}

#if !defined(NT_UP)


VOID
KiFlushTargetEntireTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing the entire TB.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush the entire TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);
    KeFlushCurrentTb();
    return;
}

VOID
KeFlushMultipleTb (
    IN ULONG Number,
    IN PVOID *Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE *PtePointer OPTIONAL,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes multiple entries from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a multiple entries from
    the translation buffer on all processors in the host configuration.

Arguments:

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies an optional pointer to an array of pointers to
       page table entries that receive the specified page table entry
       value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    ULONG Index;
    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;

    //
    // Compute target set of processors.
    //

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        Prcb = KeGetCurrentPrcb();
        TargetProcessors = KeActiveProcessors;

    } else {
        KiLockContextSwap(&OldIrql);
        Prcb = KeGetCurrentPrcb();
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= ~Prcb->SetMember;

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

    if (ARGUMENT_PRESENT(PtePointer)) {
        for (Index = 0; Index < Number; Index += 1) {
            KI_FILL_PTE(PtePointer[Index], PteValue);
        }
    }

    //
    // If any target processors are specified, then send a flush multiple
    // packet to the target set of processors.
    //

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushTargetMultipleTb,
                        (PVOID)Invalid,
                        (PVOID)Number,
                        (PVOID)Virtual);

        IPI_INSTRUMENT_COUNT (Prcb->Number, FlushMultipleTb);
    }

    //
    // Flush the specified entries from the TB on the current processor.
    //

    for (Index = 0; Index < Number; Index += 1) {
        KiFlushSingleTb(Invalid, Virtual[Index]);
    }

    //
    // Wait until all target processors have finished and complete packet.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Release the context swap lock.
    //

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

    return;
}

VOID
KiFlushTargetMultipleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Invalid,
    IN PVOID Number,
    IN PVOID Virtual
    )

/*++

Routine Description:

    This is the target function for flushing multiple TB entries.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Invalid - Supplies a bollean value that determines whether the virtual
        address is invalid.

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

Return Value:

    None.

--*/

{

    ULONG Index;
    PVOID VirtualAddress[FLUSH_MULTIPLE_MAXIMUM];

    //
    // Capture the virtual addresses that are to be flushed from the TB
    // on the current processor and signal pack done.
    //

    for (Index = 0; Index < (ULONG) Number; Index += 1) {
        VirtualAddress[Index] = ((PVOID *)(Virtual))[Index];
    }

    KiIpiSignalPacketDone(SignalDone);

    //
    // Flush the specified virtual address for the TB on the current
    // processor.
    //

    for (Index = 0; Index < (ULONG) Number; Index += 1) {
        KiFlushSingleTb((BOOLEAN)Invalid, VirtualAddress [Index]);
    }
}

HARDWARE_PTE
KeFlushSingleTb (
    IN PVOID Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE PtePointer,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes a single entry from translation buffer (TB) on all
    processors that are currently running threads which are child of the current
    process or flushes the entire translation buffer on all processors in the
    host configuration.

Arguments:

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Invalid - Supplies a boolean value that specifies the reason for flushing
        the translation buffer.

    AllProcessors - Supplies a boolean value that determines which translation
        buffers are to be flushed.

    PtePointer - Address of Pte to update with new value.

    PteValue - New value to put in the Pte.  Will simply be assigned to
        *PtePointer, in a fashion correct for the hardware.

Return Value:

    Returns the contents of the PtePointer before the new value
    is stored.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    HARDWARE_PTE OldPteValue;
    KAFFINITY TargetProcessors;

    //
    // Compute target set of processors.
    //

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        Prcb = KeGetCurrentPrcb();
        TargetProcessors = KeActiveProcessors;

    } else {
        KiLockContextSwap(&OldIrql);
        Prcb = KeGetCurrentPrcb();
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= ~Prcb->SetMember;

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    KI_SWAP_PTE(PtePointer, PteValue, OldPteValue);

    //
    // If any target processors are specified, then send a flush single
    // packet to the target set of processors.
    //

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushTargetSingleTb,
                        (PVOID)Invalid,
                        (PVOID)Virtual,
                        NULL);

        IPI_INSTRUMENT_COUNT(Prcb->Number, FlushSingleTb);
    }


    //
    // Flush the specified entry from the TB on the current processor.
    //

    KiFlushSingleTb(Invalid, Virtual);

    //
    // Wait until all target processors have finished and complete packet.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Release the context swap lock.
    //

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

    return(OldPteValue);
}

VOID
KiFlushTargetSingleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Invalid,
    IN PVOID VirtualAddress,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Invalid - Supplies a bollean value that determines whether the virtual
        address is invalid.

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush a single entry from the TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);
    KiFlushSingleTb((BOOLEAN)Invalid, (PVOID)VirtualAddress);
}

HARDWARE_PTE
KiFlushSingleTbSynchronous (
    IN PVOID Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE PtePointer,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function is a slow synchronous version of KeFlushSingleTb.  We need
    this function as many P6's don't actually know how to deal with PTEs in
    an MP safe manner.

Arguments:

    See KeFlushSingleTb

Return Value:

    See KeFlushSingleTb

--*/
{

    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    HARDWARE_PTE OldPteValue;
    KAFFINITY TargetProcessors;

    //
    // Synchronize will all other single flush calls (and other
    // IPIs which use reverse stalls)
    //

    KiLockContextSwap(&OldIrql);

    //
    // Compute target set of processors.
    //

    Prcb = KeGetCurrentPrcb();
    if (AllProcessors != FALSE) {
        TargetProcessors = KeActiveProcessors;
    } else {
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    TargetProcessors &= ~Prcb->SetMember;

    //
    // If any target processors are specified, then send a flush single
    // packet to the target set of processors.
    //

    if (TargetProcessors != 0) {
        KiIpiSendSynchronousPacket(Prcb,
                                   TargetProcessors,
                                   KiFlushTargetSingleTbSynchronous,
                                   (PVOID)Invalid,
                                   (PVOID)Virtual,
                                   (PVOID)&Prcb->ReverseStall
                                   );

        IPI_INSTRUMENT_COUNT(Prcb->Number, FlushSingleTb);

        //
        // Wait for the target processors to stall
        //

        KiIpiStallOnPacketTargets(TargetProcessors);

        //
        // Capture the previous contents of the page table entry and set the
        // page table entry to the new value.
        //

        KI_SWAP_PTE(PtePointer, PteValue, OldPteValue);

        //
        // Notify all prcessors it's time to go
        //

        Prcb->ReverseStall += 1;

    } else {

        //
        // Capture the previous contents of the page table entry and set the
        // page table entry to the new value.
        //

        KI_SWAP_PTE(PtePointer, PteValue, OldPteValue);
    }

    //
    // Flush the specified entry from the TB on the current processor.
    //

    KiFlushSingleTb(Invalid, Virtual);

    //
    // Done
    //

    KiUnlockContextSwap(OldIrql);
    return(OldPteValue);
}

VOID
KiFlushTargetSingleTbSynchronous (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Invalid,
    IN PVOID VirtualAddress,
    IN PVOID Proceed
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry synchronously.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Invalid - Supplies a bollean value that determines whether the virtual
        address is invalid.

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush a single entry from the TB on the current processor.
    //

    KiIpiSignalPacketDoneAndStall(SignalDone, Proceed);
    KiFlushSingleTb((BOOLEAN)Invalid, (PVOID)VirtualAddress);
}


VOID
Ki386UseSynchronousTbFlush (
    IN volatile PLONG Number
    )
{
    PKPRCB              Prcb;
    volatile PUCHAR     Patch;

    Prcb = KeGetCurrentPrcb();
    Patch = (PUCHAR) KeFlushSingleTb;

    //
    // Signal we're here and wait for others
    //

    InterlockedDecrement (Number);
    while (*Number) ;

    //
    // If this is processor 0 apply the patch
    //

    if (Prcb->Number == 0) {
        *((PULONG) &Patch[1]) = ((ULONG) &KiFlushSingleTbSynchronous) - ((ULONG) Patch) - 5;
        Patch[0] = 0xe9;
    }

    //
    // Wait for processor 0 to complete installation of handler
    //

    while (Patch[0] != 0xe9) ;
}

#endif
