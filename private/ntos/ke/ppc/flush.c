/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    flush.c

Abstract:

    This module implements PowerPc machine dependent kernel functions to flush
    the data and instruction caches and to flush I/O buffers.

Author:

    David N. Cutler (davec) 26-Apr-1990

Modified by:

    Pat Carr (patcarr@pets.sps.mot.com) 15-Aug-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define forward referenced prototyes.
//

VOID
KiChangeColorPageTarget (
    IN PULONG SignalDone,
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN PVOID PageFrame
    );

VOID
KiSweepDcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSweepIcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSweepIcacheRangeTarget (
    IN PULONG SignalDone,
    IN PVOID BaseAddress,
    IN PVOID Length,
    IN PVOID Parameter3
    );

VOID
KiFlushIoBuffersTarget (
    IN PULONG SignalDone,
    IN PVOID Mdl,
    IN PVOID ReadOperation,
    IN PVOID DmaOperation
    );

VOID
KeChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    )

/*++

Routine Description:

    This routine changes the color of a page.

Arguments:

    NewColor - Supplies the page aligned virtual address of the new color
        the page to change.

    OldColor - Supplies the page aligned virtual address of the old color
        of the page to change.

    PageFrame - Supplies the page frame number of the page that is changed.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= SYNCH_LEVEL);

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

#if !defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors and send the change color
    // parameters to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiChangeColorPageTarget,
                        (PVOID)NewColor,
                        (PVOID)OldColor,
                        (PVOID)PageFrame);
    }

#endif

#ifdef COLORED_PAGES
    //
    // Change the color of the page on the current processor.
    //

    HalChangeColorPage(NewColor, OldColor, PageFrame);

#endif

    //
    // Wait until all target processors have finished changing the color
    // of the page.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiChangeColorPageTarget (
    IN PULONG SignalDone,
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN PVOID PageFrame
    )

/*++

Routine Description:

    This is the target function for changing the color of a page.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    NewColor - Supplies the page aligned virtual address of the new color
        the page to change.

    OldColor - Supplies the page aligned virtual address of the old color
        of the page to change.

    PageFrame - Supplies the page frame number of the page that is changed.

Return Value:

    None.

--*/

{

    //
    // Change the color of the page on the current processor and clear
    // change color packet address to signal the source to continue.
    //

#if !defined(NT_UP)

#ifdef COLORED_PAGES
    HalChangeColorPage(NewColor, OldColor, (ULONG)PageFrame);
#endif

    KiIpiSignalPacketDone(SignalDone);

#endif

    return;
}

VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the data cache on all processors that are currently
    running threads which are children of the current process or flushes the
    data cache on all processors in the host configuration.

    N.B. PowerPC maintains cache coherency across processors however
    in this routine, the range of addresses being flushed is unknown
    so we must still broadcast the request to the other processors.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= SYNCH_LEVEL);

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

#if !defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors and send the sweep parameters
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepDcacheTarget,
                        NULL,
                        NULL,
                        NULL);
    }

#endif

    //
    // Sweep the data cache on the current processor.
    //

    HalSweepDcache();

    //
    // Wait until all target processors have finished sweeping the their
    // data cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiSweepDcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the data cache on target
    processors.

    N.B. PowerPC maintains cache coherency in the D-Cache across all
    processors.   This routine should not be used but is here in case
    it actually is used.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Sweep the data cache on the current processor and clear the sweep
    // data cache packet address to signal the source to continue.
    //

#if !defined(NT_UP)

    HalSweepDcache();
    KiIpiSignalPacketDone(SignalDone);

#endif

    return;
}

VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the instruction cache on all processors that are
    currently running threads which are children of the current process or
    flushes the instruction cache on all processors in the host configuration.

    N.B. Although PowerPC maintains cache coherency across processors, we
    use the flash invalidate function (h/w) for I-Cache sweeps which doesn't
    maintain coherency so we still do the MP I-Cache flush in s/w.   plj.

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= SYNCH_LEVEL);

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

#if !defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors and send the sweep parameters
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepIcacheTarget,
                        NULL,
                        NULL,
                        NULL);
    }

#endif

    //
    // Sweep the instruction cache on the current processor.
    //
    // If the processor is not a 601, flush the data cache first.
    //

    if ( ( (KeGetPvr() >> 16 ) & 0xffff ) > 1 ) {
        HalSweepDcache();
    }

    HalSweepIcache();

    //
    // Wait until all target processors have finished sweeping their
    // instruction caches.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiSweepIcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the instruction cache on
    target processors.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Sweep the instruction cache on the current processor and clear
    // the sweep instruction cache packet address to signal the source
    // to continue.
    //
    // If the processor is not a 601, flush the data cache first.
    //

    if ( ( (KeGetPvr() >> 16 ) & 0xffff ) > 1 ) {
        HalSweepDcache();
    }


#if !defined(NT_UP)

    HalSweepIcache();
    KiIpiSignalPacketDone(SignalDone);

#endif

    return;
}

VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This function is used to flush a range of virtual addresses from the
    primary instruction cache on all processors that are currently running
    threads which are children of the current process or flushes the range
    of virtual addresses from the primary instruction cache on all
    processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

Return Value:

    None.

--*/

{

    ULONG Offset;
    KIRQL OldIrql;
    KAFFINITY TargetProcessors;
    ULONG ProcessorType;
    ULONG DcacheAlignment;
    ULONG IcacheAlignment;

    ASSERT(KeGetCurrentIrql() <= SYNCH_LEVEL);

    //
    // If the length of the range is greater than the size of the primary
    // instruction cache, then flush the entire cache.
    //
    // N.B. It is assumed that the size of the primary instruction and
    //      data caches are the same.
    //

    if (Length > PCR->FirstLevelIcacheSize) {
        KeSweepIcache(AllProcessors);
        return;
    }

    ProcessorType = KeGetPvr() >> 16;

    if (ProcessorType != 1) {

	// PowerPc 601 has a unified cache; all others have dual caches.
        // Flush the Dcache prior to sweeping the Icache in case we need
        // to fetch a modified instruction currently Dcache resident.

        DcacheAlignment = PCR->DcacheAlignment;
        Offset = (ULONG)BaseAddress & DcacheAlignment;
        HalSweepDcacheRange(
            (PVOID)((ULONG)BaseAddress & ~DcacheAlignment),
            (Offset + Length + DcacheAlignment) & ~DcacheAlignment);
    }

#if 0

    //
    // PowerPC h/w maintains coherency across processors.  No need
    // to send IPI request.
    //

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors, and send the sweep range
    // parameters to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepIcacheRangeTarget,
                        (PVOID)BaseAddress,
                        (PVOID)Length,
                        NULL);
    }

#endif

    //
    // Flush the specified range of virtual addresses from the primary
    // instruction cache.
    //

    IcacheAlignment = PCR->IcacheAlignment;
    Offset = (ULONG)BaseAddress & IcacheAlignment;
    HalSweepIcacheRange((PVOID)((ULONG)BaseAddress & ~IcacheAlignment),
                        (Offset + Length + IcacheAlignment) & ~IcacheAlignment);

    //
    // Wait until all target processors have finished sweeping the specified
    // range of addresses from the instruction cache.
    //

#if 0

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiSweepIcacheRangeTarget (
    IN PULONG SignalDone,
    IN PVOID BaseAddress,
    IN PVOID Length,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping a range of addresses from the
    instruction cache.

    N.B. This routine is not used on PowerPC as the h/w can be relied
    upon to maintain cache coherency.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

#if 0

    ULONG Offset;
    ULONG IcacheAlignment;
    //
    // Sweep the specified instruction cache range on the current processor.
    //

    IcacheAlignment = PCR->IcacheAlignment;
    Offset = (ULONG)(BaseAddress) & IcacheAlignment;
    HalSweepIcacheRange((PVOID)((ULONG)(BaseAddress) & ~IcacheAlignment),
                        (Offset + (ULONG)Length + IcacheAlignment) & ~IcacheAlignment);

    KiIpiSignalPacketDone(SignalDone);

#endif

    return;
}

VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    )

/*++

Routine Description:

    This function flushes the I/O buffer specified by the memory descriptor
    list from the data cache on all processors.

Arguments:

    Mdl - Supplies a pointer to a memory descriptor list that describes the
        I/O buffer location.

    ReadOperation - Supplies a boolean value that determines whether the I/O
        operation is a read into memory.

    DmaOperation - Supplies a boolean value that determines whether the I/O
        operation is a DMA operation.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;
    ULONG MaxLocalSweep;

    ASSERT(KeGetCurrentIrql() <= SYNCH_LEVEL);

    //
    // If the operation is a DMA operation, then check if the flush
    // can be avoided because the host system supports the right set
    // of cache coherency attributes. Otherwise, the flush can also
    // be avoided if the operation is a programmed I/O and not a page
    // read.
    //

    if (DmaOperation != FALSE) {
        if (ReadOperation != FALSE) {

#if DBG

            //
            // Yes, it's a DMA operation, and yes, it's a read.  PPC
            // I-Caches do not snoop so this code is here only in debug
            // systems to ensure KiDmaIoCoherency is set reasonably.
            //

            if ((KiDmaIoCoherency & DMA_READ_ICACHE_INVALIDATE) != 0) {

                ASSERT((KiDmaIoCoherency & DMA_READ_DCACHE_INVALIDATE) != 0);

                return;
            }

#endif

            //
            // If the operation is NOT a page read, then the read will
            // not affect the I-Cache.  The PPC architecture ensures the
            // D-Cache will remain coherent.
            //

            if ((Mdl->MdlFlags & MDL_IO_PAGE_READ) == 0) {
                ASSERT((KiDmaIoCoherency & DMA_READ_DCACHE_INVALIDATE) != 0);
                return;
            }

        } else if ((KiDmaIoCoherency & DMA_WRITE_DCACHE_SNOOP) != 0) {
            return;
        }

    } else if ((Mdl->MdlFlags & MDL_IO_PAGE_READ) == 0) {
        return;
    }

    //
    // If the processor has a unified cache (currently the only
    // PowerPC to fall into this category is a 601) then there
    // are no problems with the I-Cache not snooping and D-Cache
    // coherency is architected.
    //

    if ((KeGetPvr() >> 16) == 1) {
        return;
    }

    //
    // Either the operation is a DMA operation and the right coherency
    // atributes are not supported by the host system, or the operation
    // is programmed I/O and a page read.
    //
    // If the amount of data to sweep is large, sweep the entire
    // data and inctruction caches on all processors, otherwise,
    // sweep the explicit range covered by the mdl.
    //
    // Sweeping the range covered by the mdl will be broadcast
    // to the other processors by the PPC h/w coherency mechanism.
    // (1 DCBST + 1 ICBI per block)
    // Sweeping the entire D-Cache involves (potentially) loading
    // and broadcasting a DCBST for each block in the D-Cache on
    // every processor.
    //
    // For this reason we only sweep all if the amount to flush
    // is greater than the First Level D Cache size * number of
    // processors in the system.
    //

    MaxLocalSweep = PCR->FirstLevelDcacheSize;

#if !defined(NT_UP)

    MaxLocalSweep *= KeNumberProcessors;

#endif

    if (Mdl->ByteCount > MaxLocalSweep) {

        //
        // Raise IRQL to synchronization level to prevent a context switch.
        //

        OldIrql = KeRaiseIrqlToSynchLevel();

#if !defined(NT_UP)

        //
        // Compute the set of target processors and send the sweep parameters
        // to the target processors, if any, for execution.
        //

        TargetProcessors = KeActiveProcessors & PCR->NotMember;
        if (TargetProcessors != 0) {

            KiIpiSendPacket(TargetProcessors,
                            KiFlushIoBuffersTarget,
                            (PVOID)Mdl,
                            (PVOID)((ULONG)ReadOperation),
                            (PVOID)((ULONG)DmaOperation));

        }

#endif

        //
        // Flush the caches on the current processor.
        //

        HalSweepDcache();

        HalSweepIcache();

        //
        // Wait until all target processors have finished
        // flushing their caches.
        //

#if !defined(NT_UP)

        if (TargetProcessors != 0) {
            KiIpiStallOnPacketTargets();
        }

#endif

        //
        // Lower IRQL to its previous level and return.
        //

        KeLowerIrql(OldIrql);

        return;
    }

    //
    // The amount of data to be flushed is sufficiently small that it
    // should be done on this processor only, allowing the h/w to ensure
    // coherency.
    //

    HalFlushIoBuffers(Mdl, ReadOperation, DmaOperation);

}

VOID
KiFlushIoBuffersTarget (
    IN PULONG SignalDone,
    IN PVOID Mdl,
    IN PVOID ReadOperation,
    IN PVOID DmaOperation
    )

/*++

Routine Description:

    This is the target function for flushing an I/O buffer on target
    processors.   On PowerPC this routine is only called when it has
    been determined that it is more efficient to sweep the entire
    cache than to sweep the range specified in the mdl.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Mdl - Supplies a pointer to a memory descriptor list that describes the
        I/O buffer location.

    ReadOperation - Supplies a boolean value that determines whether the I/O
        operation is a read into memory.

    DmaOperation - Supplies a boolean value that determines whether the I/O
        operation is a DMA operation.

Return Value:

    None.

--*/

{

    //
    // Flush the caches on the current processor.
    //

#if !defined(NT_UP)

    HalSweepDcache();

    HalSweepIcache();

    KiIpiSignalPacketDone(SignalDone);

#endif

    return;
}
