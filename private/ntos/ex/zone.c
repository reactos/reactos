/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    zone.c

Abstract:

    This module implements a simple zone buffer manager.  The primary
    consumer of this module is local LPC.

    The zone package provides a fast and efficient memory allocator for
    fixed-size 64-bit aligned blocks of storage.  The zone package does
    not provide any serialization over access to the zone header and
    associated free list and segment list.  It is the responsibility of
    the caller to provide any necessary serialization.

    The zone package views a zone as a set of fixed-size blocks of
    storage.  The block size of a zone is specified during zone
    initialization.  Storage is assigned to a zone during zone
    initialization and when a zone is extended.  In both of these cases,
    a segment and length are specified.

    The zone package uses the first ZONE_SEGMENT_HEADER portion of the
    segment for zone overhead.  The remainder of the segment is carved
    up into fixed-size blocks and each block is added to the free list
    maintained in the zone header.

    As long as a block is on the free list, the first SINGLE_LIST_ENTRY
    (32 bit) sized piece of the block is used as zone overhead.  The
    rest of the block is not used by the zone package and may be used by
    applications to cache information.  When a block is not on the free
    list, its entire contents are available to the application.

Author:

    Mark Lucovsky (markl) 13-May-1989

Revision History:

--*/

#include "exp.h"

NTSTATUS
ExInitializeZone(
    IN PZONE_HEADER Zone,
    IN ULONG BlockSize,
    IN PVOID InitialSegment,
    IN ULONG InitialSegmentSize
    )

/*++

Routine Description:

    This function initializes a zone header.  Once successfully
    initialized, blocks can be allocated and freed from the zone, and
    the zone can be extended.

Arguments:

    Zone - Supplies the address of a zone header to be initialized.

    BlockSize - Supplies the block size of the allocatable unit within
                the zone.  The size must be larger that the size of the
                initial segment, and must be 64-bit aligned.

    InitialSegment - Supplies the address of a segment of storage.  The
                     first ZONE_SEGMENT_HEADER-sized portion of the segment
                     is used by the zone allocator.  The remainder of
                     the segment is carved up into fixed size
                     (BlockSize) blocks and is made available for
                     allocation and deallocation from the zone.  The
                     address of the segment must be aligned on a 64-bit
                     boundary.

    InitialSegmentSize - Supplies the size in bytes of the InitialSegment.

Return Value:

    STATUS_UNSUCCESSFUL - BlockSize or InitialSegment was not aligned on
                          64-bit boundaries, or BlockSize was larger than
                          the initial segment size.

    STATUS_SUCCESS - The zone was successfully initialized.

--*/

{
    ULONG i;
    PCH p;

    if ( (BlockSize & 7) || ((ULONG_PTR)InitialSegment & 7) ||
         (BlockSize > InitialSegmentSize) ) {
#if DBG
        DbgPrint( "EX: ExInitializeZone( %x, %x, %x, %x ) - Invalid parameters.\n",
                  Zone, BlockSize, InitialSegment, InitialSegmentSize
                );
        DbgBreakPoint();
#endif
        return STATUS_INVALID_PARAMETER;
    }

    Zone->BlockSize = BlockSize;

    Zone->SegmentList.Next = &((PZONE_SEGMENT_HEADER) InitialSegment)->SegmentList;
    ((PZONE_SEGMENT_HEADER) InitialSegment)->SegmentList.Next = NULL;
    ((PZONE_SEGMENT_HEADER) InitialSegment)->Reserved = NULL;

    Zone->FreeList.Next = NULL;

    p = (PCH)InitialSegment + sizeof(ZONE_SEGMENT_HEADER);

    for (i = sizeof(ZONE_SEGMENT_HEADER);
         i <= InitialSegmentSize - BlockSize;
         i += BlockSize
        ) {
        ((PSINGLE_LIST_ENTRY)p)->Next = Zone->FreeList.Next;
        Zone->FreeList.Next = (PSINGLE_LIST_ENTRY)p;
        p += BlockSize;
    }
    Zone->TotalSegmentSize = i;

#if 0
    DbgPrint( "EX: ExInitializeZone( %lx, %lx, %lu, %lu, %lx )\n",
              Zone, InitialSegment, InitialSegmentSize,
              BlockSize, p
            );
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
ExExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize
    )

/*++

Routine Description:

    This function extends a zone by adding another segment's worth of
    blocks to the zone.

Arguments:

    Zone - Supplies the address of a zone header to be extended.

    Segment - Supplies the address of a segment of storage.  The first
              ZONE_SEGMENT_HEADER-sized portion of the segment is used by the
              zone allocator.  The remainder of the segment is carved up
              into fixed-size (BlockSize) blocks and is added to the
              zone.  The address of the segment must be aligned on a 64-
              bit boundary.

    SegmentSize - Supplies the size in bytes of Segment.

Return Value:

    STATUS_UNSUCCESSFUL - BlockSize or Segment was not aligned on
                          64-bit boundaries, or BlockSize was larger than
                          the segment size.

    STATUS_SUCCESS - The zone was successfully extended.

--*/

{
    ULONG i;
    PCH p;

    if ( ((ULONG_PTR)Segment & 7) ||
         (SegmentSize & 7) ||
         (Zone->BlockSize > SegmentSize) ) {
        return STATUS_UNSUCCESSFUL;
    }

    ((PZONE_SEGMENT_HEADER) Segment)->SegmentList.Next = Zone->SegmentList.Next;
    Zone->SegmentList.Next = &((PZONE_SEGMENT_HEADER) Segment)->SegmentList;

    p = (PCH)Segment + sizeof(ZONE_SEGMENT_HEADER);

    for (i = sizeof(ZONE_SEGMENT_HEADER);
         i <= SegmentSize - Zone->BlockSize;
         i += Zone->BlockSize
        ) {

        ((PSINGLE_LIST_ENTRY)p)->Next = Zone->FreeList.Next;
        Zone->FreeList.Next = (PSINGLE_LIST_ENTRY)p;
        p += Zone->BlockSize;
    }
    Zone->TotalSegmentSize += i;

#if 0
    DbgPrint( "EX: ExExtendZone( %lx, %lx, %lu, %lu, %lx )\n",
              Zone, Segment, SegmentSize, Zone->BlockSize, p
            );
#endif

    return STATUS_SUCCESS;
}



NTSTATUS
ExInterlockedExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize,
    IN PKSPIN_LOCK Lock
    )

/*++

Routine Description:

    This function extends a zone by adding another segment's worth of
    blocks to the zone.

Arguments:

    Zone - Supplies the address of a zone header to be extended.

    Segment - Supplies the address of a segment of storage.  The first
              ZONE_SEGMENT_HEADER-sized portion of the segment is used by the
              zone allocator.  The remainder of the segment is carved up
              into fixed-size (BlockSize) blocks and is added to the
              zone.  The address of the segment must be aligned on a 64-
              bit boundary.

    SegmentSize - Supplies the size in bytes of Segment.

    Lock - pointer to spinlock to use

Return Value:

    STATUS_UNSUCCESSFUL - BlockSize or Segment was not aligned on
                          64-bit boundaries, or BlockSize was larger than
                          the segment size.

    STATUS_SUCCESS - The zone was successfully extended.

--*/

{
    NTSTATUS Status;
    KIRQL OldIrql;

    ExAcquireSpinLock( Lock, &OldIrql );

    Status = ExExtendZone( Zone, Segment, SegmentSize );

    ExReleaseSpinLock( Lock, OldIrql );

    return Status;
}
