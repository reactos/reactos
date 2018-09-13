/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    region.c

Abstract:

    This module implements a simple region buffer manager that is MP safe.

Author:

    David N. Cutler (davec) 25-Novy-1995

Revision History:

--*/

#include "exp.h"

VOID
ExInitializeRegion(
    IN PREGION_HEADER Region,
    IN ULONG BlockSize,
    IN PVOID Segment,
    IN ULONG SegmentSize
    )

/*++

Routine Description:

    This function initializes a region header.

Arguments:

    Region - Supplies the address of a region header to initialize.

    BlockSize - Supplies the block size of the allocatable units within
        the region. The size must be larger that the size of the initial
        segment, and must be 64-bit aligned.

    Segment - Supplies the address of a segment of storage.

    SegmentSize - Supplies the size in bytes of the segment.

Return Value:

    None.

--*/

{

    SINGLE_LIST_ENTRY FirstEntry;
    PSINGLE_LIST_ENTRY LastEntry;
    PSINGLE_LIST_ENTRY NextEntry;
    ULONG TotalSize;

    //
    // If the host system is a small system and the segment size is greater
    // than two pages, then bugcheck.
    //

    if ((MmQuerySystemSize() == MmSmallSystem) && (SegmentSize > (2 * PAGE_SIZE))) {
        KeBugCheckEx(NO_PAGES_AVAILABLE, 0x123, SegmentSize, 0, 0);
    }

    //
    // If the segment address is not quadword aligned, or the block size is
    // greater than the segment size minus size of the segment header, then
    // the region is not valid - bugcheck.
    //

    if ((((ULONG)Segment & 7) != 0) ||
        ((BlockSize & 7) != 0) ||
        (BlockSize > (SegmentSize - sizeof(REGION_SEGMENT_HEADER)))) {
        KeBugCheckEx(INVALID_REGION_OR_SEGMENT, (ULONG)Segment, SegmentSize, BlockSize, 0);
    }

    //
    // Initialize the region header.
    //

    ExInitializeSListHead(&Region->ListHead);
    Region->FirstSegment = (PREGION_SEGMENT_HEADER)Segment;
    Region->BlockSize = BlockSize;
    TotalSize = ((SegmentSize - sizeof(REGION_SEGMENT_HEADER)) / BlockSize) * BlockSize;
    Region->TotalSize = TotalSize;

    //
    // Initialize the segment free list.
    //

    FirstEntry.Next = NULL;
    NextEntry = (PSINGLE_LIST_ENTRY)((PCHAR)Segment + sizeof(REGION_SEGMENT_HEADER));
    LastEntry = (PSINGLE_LIST_ENTRY)((PCHAR)NextEntry + TotalSize);
    do {
       NextEntry->Next = FirstEntry.Next;
       FirstEntry.Next = NextEntry;
       NextEntry = (PSINGLE_LIST_ENTRY)((PCHAR)NextEntry + BlockSize);
    } while (NextEntry != LastEntry);
    Region->ListHead.Next.Next = FirstEntry.Next;
    return;
}

VOID
ExInterlockedExtendRegion(
    IN PREGION_HEADER Region,
    IN PVOID Segment,
    IN ULONG SegmentSize,
    IN PKSPIN_LOCK Lock
    )

/*++

Routine Description:

    This function extends a region by adding another segment's worth of
    blocks to the region.

Arguments:

    Region - Supplies the address of a region header.

    Segment - Supplies the address of a segment of storage.

    SegmentSize - Supplies the size in bytes of Segment.

    Lock - Supplies the address of a spinlock.

Return Value:

    None.

--*/

{

    ULONG BlockSize;
    SINGLE_LIST_ENTRY FirstEntry;
    KIRQL OldIrql;
    PSINGLE_LIST_ENTRY LastEntry;
    PSINGLE_LIST_ENTRY NextEntry;
    ULONG TotalSize;

    //
    // If the segment address is not quadword aligned, or the block size is
    // greater than the segment size minus size of the segment header, then
    // the region is not valid.
    //

    BlockSize = Region->BlockSize;
    if ((((ULONG)Segment & 7) != 0) ||
        ((BlockSize & 7) != 0) ||
        (BlockSize > (SegmentSize - sizeof(REGION_SEGMENT_HEADER)))) {
        KeBugCheckEx(INVALID_REGION_OR_SEGMENT, (ULONG)Segment, SegmentSize, BlockSize, 0);
    }

    //
    // Initialize the segment free list.
    //

    TotalSize = ((SegmentSize - sizeof(REGION_SEGMENT_HEADER)) / BlockSize) * BlockSize;
    FirstEntry.Next = NULL;
    NextEntry = (PSINGLE_LIST_ENTRY)((PCHAR)Segment + sizeof(REGION_SEGMENT_HEADER));
    LastEntry = (PSINGLE_LIST_ENTRY)((PCHAR)NextEntry + TotalSize);
    do {
       NextEntry->Next = FirstEntry.Next;
       FirstEntry.Next = NextEntry;
       NextEntry = (PSINGLE_LIST_ENTRY)((PCHAR)NextEntry + BlockSize);
    } while (NextEntry != LastEntry);

    //
    // Acquire the specified spinlock, insert the next segment in the segment
    // list, update the total segment size, and carefully merge the new segment
    // list with the current list.
    //
    // N.B. The merging of the free list is done very carefully such that
    //      manipulation of the free list can occur without using spinlocks.
    //

    ExAcquireSpinLock(Lock, &OldIrql);
    ((PREGION_SEGMENT_HEADER)Segment)->NextSegment = Region->FirstSegment;
    Region->FirstSegment = (PREGION_SEGMENT_HEADER)Segment;
    Region->TotalSize += TotalSize;
    NextEntry = (PSINGLE_LIST_ENTRY)((PCHAR)Segment + sizeof(REGION_SEGMENT_HEADER));
    LastEntry = (PSINGLE_LIST_ENTRY)((PCHAR)LastEntry - Region->BlockSize);
    do {

        //
        // The next entry in the region list head is a volatile entry and,
        // therefore, is read each time through the loop.
        //

        FirstEntry.Next = Region->ListHead.Next.Next;

        //
        // The last entry in the new segment list, i.e., the one with a link
        // of NULL, is merged with the current list by storing the captured
        // region next link in the free entry. A compare and exchange is then
        // done using the merged link address as the comparand and the head
        // of the new free list as the exchange value. This is safe since it
        // does not matter if the next link of the first region entry changes.
        //

        NextEntry->Next = FirstEntry.Next;
    } while (InterlockedCompareExchange((PVOID)&Region->ListHead.Next,
                                        LastEntry,
                                        FirstEntry.Next) != FirstEntry.Next);

    ExReleaseSpinLock(Lock, OldIrql);
    return;
}
