/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    flush.c

Abstract:

    This module implements Alpha AXP machine dependent kernel functions to flush
    the data and instruction caches and to flush I/O buffers.

Author:

    David N. Cutler (davec) 26-Apr-1990
    Joe Notarangelo  29-Nov-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"


//
// Define forward referenced prototypes.
//

VOID
KiSweepDcacheTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Count,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSweepIcacheTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Count,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushIoBuffersTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Mdl,
    IN PVOID ReadOperation,
    IN PVOID DmaOperation
    );

VOID
KiSynchronizeMemoryAccessTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

ULONG KiSynchronizeMemoryCallCount = 0;


VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the data cache on all processors that are currently
    running threads which are children of the current process or flushes the
    data cache on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= KiSynchIrql);

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

    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, SweepDcache);

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
        KiIpiStallOnPacketTargets(TargetProcessors);
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
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the data cache on target
    processors.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed

    Parameter1 - Parameter3 - not used

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
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, SweepDcache);

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

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= KiSynchIrql);

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

    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, SweepIcache);

#endif

    //
    // Sweep the instruction cache on the current processor.
    //

    KiImb();

    //
    // Wait until all target processors have finished sweeping the their
    // instruction cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
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
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the instruction cache on
    target processors.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed

    Parameter1 - Parameter3 - not used


Return Value:

    None.

--*/

{

    //
    // Sweep the instruction cache on the current processor and clear
    // the sweep instruction cache packet address to signal the source
    // to continue.
    //

#if !defined(NT_UP)

    KiImb();
    KiIpiSignalPacketDone(SignalDone);
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, SweepIcache);

#endif

    return;
}

VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG_PTR Length
    )

/*++

Routine Description:

    This function flushes the an range of virtual addresses from the primary
    instruction cache on all processors that are currently running threads
    which are children of the current process or flushes the range of virtual
    addresses from the primary instruction cache on all processors in the host
    configuration.

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

    KeSweepIcache(AllProcessors);
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

    Alpha requires that caches be coherent with respect to I/O. All that
    this routine needs to do is execute a memory barrier on the current
    processor. However, in order to maintain i-stream coherency, all
    processors must execute the IMB PAL call in the case of page reads.
    Thus, all processors are IPI'd to perform the IMB for any flush
    that is a DmaOperation, a ReadOperation, and an MDL_IO_PAGE_READ.


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

    ASSERT(KeGetCurrentIrql() <= KiSynchIrql);

    KiMb();

    //
    // If the operation is a DMA operation, then check if the flush
    // can be avoided because the host system supports the right set
    // of cache coherency attributes. Otherwise, the flush can also
    // be avoided if the operation is a programmed I/O and not a page
    // read.
    //

    if (DmaOperation != FALSE) {
        if (ReadOperation != FALSE) {
            if ((KiDmaIoCoherency & DMA_READ_ICACHE_INVALIDATE) != 0) {

                ASSERT((KiDmaIoCoherency & DMA_READ_DCACHE_INVALIDATE) != 0);

                return;

            } else if (((Mdl->MdlFlags & MDL_IO_PAGE_READ) == 0) &&
                ((KiDmaIoCoherency & DMA_READ_DCACHE_INVALIDATE) != 0)) {
                return;
            }

        } else if ((KiDmaIoCoherency & DMA_WRITE_DCACHE_SNOOP) != 0) {
            return;
        }

    } else if ((Mdl->MdlFlags & MDL_IO_PAGE_READ) == 0) {
        return;
    }

    //
    // Either the operation is a DMA operation and the right coherency
    // attributes are not supported by the host system, or the operation
    // is programmed I/O and a page read.
    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors, and send the flush I/O
    // parameters to the target processors, if any, for execution.
    //

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushIoBuffersTarget,
                        (PVOID)Mdl,
                        ULongToPtr((ULONG)ReadOperation),
                        ULongToPtr((ULONG)DmaOperation));
    }

#endif

    //
    // Flush I/O buffer on current processor.
    //

    HalFlushIoBuffers(Mdl, ReadOperation, DmaOperation);

    //
    // Wait until all target processors have finished flushing the
    // specified I/O buffer.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

    return;
}

#if !defined(NT_UP)

VOID
KiFlushIoBuffersTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Mdl,
    IN PVOID ReadOperation,
    IN PVOID DmaOperation
    )

/*++

Routine Description:

    This is the target function for flushing an I/O buffer on target
    processors.

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
    // Flush the specified I/O buffer on the current processor.
    //

    HalFlushIoBuffers((PMDL)Mdl,
                      (BOOLEAN)((ULONG_PTR)ReadOperation),
                      (BOOLEAN)((ULONG_PTR)DmaOperation));

    KiIpiSignalPacketDone(SignalDone);
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushIoBuffers);

    return;
}

#endif

VOID
KeSynchronizeMemoryAccess (
    VOID
    )

/*++

Routine Description:

    This function synchronizes memory access across all processors in the
    host configurarion.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= KiSynchIrql);

    KiSynchronizeMemoryCallCount += 1;

    //
    // Raise IRQL to synchronization level to prevent a context switch.
    //

#if !defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

    //
    // Compute the set of target processors and send the synchronize message
    // to the target processors, if any, for execution.
    //

    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSynchronizeMemoryAccessTarget,
                        NULL,
                        NULL,
                        NULL);
    }

    //
    // On an MP system an implicit memory barrier is executed during the
    // end of the IPI message. On a UP system, a memory barrier must be
    // executed.
    //

#else

    __MB();

#endif

    //
    // Wait until all target processors have finished sweeping the their
    // data cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

#if !defined(NT_UP)


VOID
KiSynchronizeMemoryAccessTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This function performs no operation, but an implicit memory barrier
    is executed when the IPI message is received.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed

    Parameter1 - Parameter3 - not used

Return Value:

    None.

--*/

{

    KiIpiSignalPacketDone(SignalDone);
    return;
}

#endif
