/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/zone.c
 * PURPOSE:         Implements zone buffers
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
ExExtendZone(PZONE_HEADER Zone,
             PVOID Segment,
             ULONG SegmentSize)
{
    ULONG_PTR Entry;
    ULONG i;

    /*
     * BlockSize and Segment must be 8-byte aligned.
     * Blocksize cannot exceed Segment Size.
     */
    if (((ULONG_PTR)Segment & 7) ||
        (SegmentSize & 7) ||
        (Zone->BlockSize > SegmentSize))
    {
        DPRINT1("Invalid ExExtendZone Alignment and/or Size\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Link the Zone and Segment */
    PushEntryList(&Zone->SegmentList,
                  &((PZONE_SEGMENT_HEADER)Segment)->SegmentList);

    /* Get to the first entry */
    Entry = (ULONG_PTR)Segment + sizeof(ZONE_SEGMENT_HEADER);

    /* Loop through the segments */
    for (i = sizeof(ZONE_SEGMENT_HEADER);
         i <= SegmentSize - Zone->BlockSize;
         i+= Zone->BlockSize)
    {
        /* Link the Free and Segment Lists */
        PushEntryList(&Zone->FreeList, (PSINGLE_LIST_ENTRY)Entry);

        /* Go to the next entry */
        Entry += Zone->BlockSize;
    }

    /* Update Segment Size */
    Zone->TotalSegmentSize += i;

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
ExInterlockedExtendZone(PZONE_HEADER Zone,
                        PVOID Segment,
                        ULONG SegmentSize,
                        PKSPIN_LOCK Lock)
{
    NTSTATUS Status;
    KIRQL OldIrql;

    /* Get the lock */
    KeAcquireSpinLock(Lock, &OldIrql);

    /* Extend the Zone */
    Status = ExExtendZone(Zone, Segment, SegmentSize);

    /* Release lock and return status */
    KeReleaseSpinLock(Lock, OldIrql);
    return Status;
}

/*
 * FUNCTION: Initalizes a zone header
 * ARGUMENTS:
 *          Zone = zone header to be initialized
 *          BlockSize = Size (in bytes) of the allocation size of the zone
 *          InitialSegment = Initial segment of storage allocated by the
 *                           caller
 *          InitialSegmentSize = Initial size of the segment
 *
 * @implemented
 */
NTSTATUS
NTAPI
ExInitializeZone(PZONE_HEADER Zone,
                 ULONG BlockSize,
                 PVOID InitialSegment,
                 ULONG InitialSegmentSize)
{
    ULONG i;
    ULONG_PTR Entry;

    /*
     * BlockSize and Segment must be 8-byte aligned.
     * Blocksize cannot exceed Segment Size.
     */
    if (((ULONG_PTR)InitialSegment & 7) ||
        (InitialSegmentSize & 7) ||
        (BlockSize > InitialSegmentSize))
    {
        DPRINT1("Invalid ExInitializeZone Alignment and/or Size\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Set the Zone Header */
    Zone->BlockSize = BlockSize;

    /* Link empty list */
    Zone->FreeList.Next = NULL;
    Zone->SegmentList.Next = NULL;
    PushEntryList(&Zone->SegmentList,
                  &((PZONE_SEGMENT_HEADER)InitialSegment)->SegmentList);
    ((PZONE_SEGMENT_HEADER)InitialSegment)->Reserved = NULL;

    /* Get first entry */
    Entry = (ULONG_PTR)InitialSegment + sizeof(ZONE_SEGMENT_HEADER);

    /* Loop through the segments */
    for (i = sizeof(ZONE_SEGMENT_HEADER);
         i <= InitialSegmentSize - BlockSize;
         i+= BlockSize)
    {
        /* Link the Free and Segment Lists */
        PushEntryList(&Zone->FreeList, (PSINGLE_LIST_ENTRY)Entry);

        /* Go to the next entry */
        Entry += Zone->BlockSize;
    }

    /* Update Segment Size */
    Zone->TotalSegmentSize += i;

    /* Return success */
    return STATUS_SUCCESS;
}

/* EOF */
