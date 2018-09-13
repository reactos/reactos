/*++

Copyright (c) 1992-1994 Microsoft Corporation
Copyright (c) 1994 IBM Corporation

Module Name:

    flushtb.c

Abstract:

    This module implements machine dependent functions to flush the
    translation buffer and synchronize PIDs in an MP system.

Author:

    David N. Cutler (davec) 13-May-1989

    Modified for PowerPC by Mark Mergen (mergen@watson.ibm.com) 25-Aug-1994

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"

VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on all
    processors that are currently running threads which are children
    of the current process or flushes the entire translation buffer
    on all processors in the host configuration.

    N.B. The entire translation buffer on all processors in the host
         configuration is always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

Arguments:

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() <= SYNCH_LEVEL);

#if !defined(NT_UP)

    //
    // Raise IRQL to synchronization level to avoid a possible context switch.
    //

    OldIrql = KeRaiseIrqlToSynchLevel();

    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushEntireTb);

#endif

    //
    // Flush TB on current processor.
    // PowerPC hardware broadcasts if MP.
    //

    KeFlushCurrentTb();

#if !defined(NT_UP)

    //
    // Lower IRQL to previous level.
    //

    KeLowerIrql(OldIrql);

#endif

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
    children of the current process or flushes multiple entries from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entries on all processors in the host
         configuration are always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

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

    None.

--*/

{

    ULONG Index;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

    if (ARGUMENT_PRESENT(PtePointer)) {
        for (Index = 0; Index < Number; Index += 1) {
            *PtePointer[Index] = PteValue;
        }
    }

#if !defined(NT_UP)

    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushSingleTb);

#endif

    //
    // Flush the specified entries from the TB on the current processor.
    // PowerPC hardware broadcasts if MP.
    //

    for (Index = 0; Index < Number; Index += 1) {
        KiFlushSingleTb(Invalid, Virtual[Index]);
    }

    return;
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

    This function flushes a single entry from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a single entry from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entry on all processors in the host
         configuration is always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

Arguments:

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies a pointer to the page table entry which
        receives the specified value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    HARDWARE_PTE OldPte;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Collect call data.
    //

#if defined(_COLLECT_FLUSH_SINGLE_CALLDATA_)

    RECORD_CALL_DATA(&KiFlushSingleCallData);

#endif

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    OldPte = *PtePointer;
    *PtePointer = PteValue;

#if !defined(NT_UP)

    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushSingleTb);

#endif

    //
    // Flush the specified entry from the TB on the current processor.
    // PowerPC hardware broadcasts if MP.
    //

    KiFlushSingleTb(Invalid, Virtual);

    //
    // Return the previous page table entry value.
    //

    return OldPte;
}
