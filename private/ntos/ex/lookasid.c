/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    lookasid.c

Abstract:

    This module implements lookaside list function.

Author:

    David N. Cutler (davec) 19-Feb-1995

Revision History:

--*/

#include "exp.h"

//
// Define maximum dynamic adjustment scan periods.
//

#define MAXIMUM_SCAN_PERIOD 4

//
// Define Minimum lookaside list depth.
//

#define MINIMUM_LOOKASIDE_DEPTH 4

//
// Define minimum allocation threshold.
//

#define MINIMUM_ALLOCATION_THRESHOLD 25

//
// Define forward referenced function prototypes.
//

LOGICAL
ExpComputeLookasideDepth (
    IN ULONG Allocates,
    IN ULONG Misses,
    IN USHORT MaximumDepth,
    IN OUT PUSHORT Depth
    );

PVOID
ExpDummyAllocate (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

LOGICAL
ExpScanGeneralLookasideList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK SpinLock
    );

LOGICAL
ExpScanPoolLookasideList (
    IN PLIST_ENTRY ListHead
    );

//
// Define the global nonpaged and paged lookaside list data.
//

LIST_ENTRY ExNPagedLookasideListHead;
KSPIN_LOCK ExNPagedLookasideLock;
LIST_ENTRY ExPagedLookasideListHead;
KSPIN_LOCK ExPagedLookasideLock;
LIST_ENTRY ExPoolLookasideListHead;

//
// Define lookaside list dynamic adjustment data.
//

ULONG ExpAdjustScanPeriod = 1;
ULONG ExpCurrentScanPeriod = 1;

VOID
ExAdjustLookasideDepth (
    VOID
    )

/*++

Routine Description:

    This function is called periodically to adjust the maximum depth of
    all lookaside lists.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LOGICAL Changes;

    //
    // Decrement the scan period and check if it is time to dynamically
    // adjust the maximum depth of lookaside lists.
    //

    ExpCurrentScanPeriod -= 1;
    if (ExpCurrentScanPeriod == 0) {
        Changes = FALSE;

        //
        // Scan the general paged and nonpaged lookaside lists.
        //

        Changes |= ExpScanGeneralLookasideList(&ExNPagedLookasideListHead,
                                               &ExNPagedLookasideLock);

        Changes |= ExpScanGeneralLookasideList(&ExPagedLookasideListHead,
                                               &ExPagedLookasideLock);

        //
        // Scan the pool paged and nonpaged lookaside lists;
        //

        Changes |= ExpScanPoolLookasideList(&ExPoolLookasideListHead);

        //
        // If any changes were made to the depth of any lookaside list during
        // this scan period, then lower the scan period to the minimum value.
        // Otherwise, attempt to raise the scan period.
        //

        if (Changes != FALSE) {
            ExpAdjustScanPeriod = 1;

        } else {
            if (ExpAdjustScanPeriod != MAXIMUM_SCAN_PERIOD) {
                ExpAdjustScanPeriod += 1;
            }
        }

        ExpCurrentScanPeriod = ExpAdjustScanPeriod;
    }

    return;
}

LOGICAL
ExpComputeLookasideDepth (
    IN ULONG Allocates,
    IN ULONG Misses,
    IN USHORT MaximumDepth,
    IN OUT PUSHORT Depth
    )

/*++

Routine Description:

    This function computes the target depth of a lookaside list given the
    total allocations and misses during the last scan period and the current
    depth.

Arguments:

    Allocates - Supplies the total number of allocations during the last
        scan period.

    Misses - Supplies the total number of allocate misses during the last
        scan period.

    MaximumDepth - Supplies the maximum depth the lookaside list is allowed
        to reach.

    Depth - Supplies a pointer to the current lookaside list depth which
        receives the target depth.

Return Value:

    If the target depth is greater than the current depth, then a value of
    TRUE is returned as the function value. Otherwise, a value of FALSE is
    returned.

--*/

{

    LOGICAL Changes;
    ULONG Ratio;
    ULONG Target;

    //
    // If the allocate rate is less than the mimimum threshold, then lower
    // the maximum depth of the lookaside list. Otherwise, if the miss rate
    // is less than .5%, then lower the maximum depth. Otherwise, raise the
    // maximum depth based on the miss rate.
    //

    Changes = FALSE;
    if (Misses >= Allocates) {
        Misses = Allocates;
    }

    if (Allocates == 0) {
        Allocates = 1;
    }

    Ratio = (Misses * 1000) / Allocates;
    Target = *Depth;
    if ((Allocates / ExpAdjustScanPeriod) < MINIMUM_ALLOCATION_THRESHOLD) {
        if (Target > (MINIMUM_LOOKASIDE_DEPTH + 10)) {
            Target -= 10;

        } else {
            Target = MINIMUM_LOOKASIDE_DEPTH;
        }

    } else if (Ratio < 5) {
        if (Target > (MINIMUM_LOOKASIDE_DEPTH + 1)) {
            Target -= 1;

        } else {
            Target = MINIMUM_LOOKASIDE_DEPTH;
        }

    } else {
        Changes = TRUE;
        Target += ((Ratio * MaximumDepth) / (1000 * 2)) + 5;
        if (Target > MaximumDepth) {
            Target = MaximumDepth;
        }
    }

    *Depth = (USHORT)Target;
    return Changes;
}

LOGICAL
ExpScanGeneralLookasideList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function scans the specified list of general lookaside descriptors
    and adjusts the maximum depth as necessary.

Arguments:

    ListHead - Supplies the address of the listhead for a list of lookaside
        descriptors.

    SpinLock - Supplies the address of the spinlock to be used to synchronize
        access to the list of lookaside descriptors.

Return Value:

    A value of TRUE is returned if the maximum depth of any lookaside list
    is changed. Otherwise, a value of FALSE is returned.

--*/

{

    ULONG Allocates;
    LOGICAL Changes;
    PLIST_ENTRY Entry;
    PPAGED_LOOKASIDE_LIST Lookaside;
    ULONG Misses;
    KIRQL OldIrql;

    //
    // Raise IRQL and acquire the specified spinlock.
    //

    Changes = FALSE;
    ExAcquireSpinLock(SpinLock, &OldIrql);

    //
    // Scan the specified list of lookaside descriptors and adjust the
    // maximum depth as necessary.
    //
    // N.B. All lookaside list descriptor are treated as if they were
    //      paged descriptor even though they may be nonpaged descriptors.
    //      This is possible since both structures are identical except
    //      for the locking fields which are the last structure fields.

    Entry = ListHead->Flink;
    while (Entry != ListHead) {
        Lookaside = CONTAINING_RECORD(Entry,
                                      PAGED_LOOKASIDE_LIST,
                                      L.ListEntry);

        //
        // Compute the total number of allocations and misses per second for
        // this scan period.
        //

        Allocates = Lookaside->L.TotalAllocates - Lookaside->L.LastTotalAllocates;
        Lookaside->L.LastTotalAllocates = Lookaside->L.TotalAllocates;
        Misses = Lookaside->L.AllocateMisses - Lookaside->L.LastAllocateMisses;
        Lookaside->L.LastAllocateMisses = Lookaside->L.AllocateMisses;

        //
        // Compute target depth of lookaside list.
        //

        Changes |= ExpComputeLookasideDepth(Allocates,
                                            Misses,
                                            Lookaside->L.MaximumDepth,
                                            &Lookaside->L.Depth);

        Entry = Entry->Flink;
    }

    //
    // Release spinlock, lower IRQL, and return function value.
    //

    ExReleaseSpinLock(SpinLock, OldIrql);
    return Changes;
}

LOGICAL
ExpScanPoolLookasideList (
    IN PLIST_ENTRY ListHead
    )

/*++

Routine Description:

    This function scans the specified pool lookaside descriptors and adjusts
    the maximum depth as necessary.

Arguments:

    ListHead - Supplies a pointer to the list of pool lookaside structures.

Return Value:

    A value of TRUE is returned if the maximum depth of any lookaside list
    is changed. Otherwise, a value of FALSE is returned.

--*/

{

    ULONG Allocates;
    LOGICAL Changes;
    PNPAGED_LOOKASIDE_LIST Lookaside;
    PLIST_ENTRY NextEntry;
    ULONG Misses;

    //
    // Scan the specified list of lookaside descriptors and adjust the
    // maximum depth as necessary.
    //

    Changes = FALSE;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead) {
        Lookaside = CONTAINING_RECORD(NextEntry,
                                      NPAGED_LOOKASIDE_LIST,
                                      L.ListEntry);

        //
        // Compute the total number of allocations and misses per second for
        // this scan period.
        //

        Allocates = Lookaside->L.TotalAllocates - Lookaside->L.LastTotalAllocates;
        Lookaside->L.LastTotalAllocates = Lookaside->L.TotalAllocates;
        Misses = Allocates - (Lookaside->L.AllocateHits - Lookaside->L.LastAllocateHits);
        Lookaside->L.LastAllocateHits = Lookaside->L.AllocateHits;

        //
        // Compute target depth of lookaside list.
        //

        Changes |= ExpComputeLookasideDepth(Allocates,
                                            Misses,
                                            Lookaside->L.MaximumDepth,
                                            &Lookaside->L.Depth);

        NextEntry = NextEntry->Flink;
    }

    return Changes;
}

VOID
ExInitializeNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    )

/*++

Routine Description:

    This function initializes a nonpaged lookaside list structure and inserts
    the structure in the system nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Allocate - Supplies an optional pointer to an allocate function.

    Free - Supplies an optional pointer to a free function.

    Flags - Supplies the pool allocation flags which are merged with the
        pool allocation type (NonPagedPool) to control pool allocation.

    Size - Supplies the size for the lookaside list entries.

    Tag - Supplies the pool tag for the lookaside list entries.

    Depth - Supplies the maximum depth of the lookaside list.

Return Value:

    None.

--*/

{

    //
    // Initialize the lookaside list structure.
    //

    ExInitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.Depth = MINIMUM_LOOKASIDE_DEPTH;
    Lookaside->L.MaximumDepth = 256; //Depth;
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = NonPagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = (ULONG)Size;
    if (Allocate == NULL) {
        Lookaside->L.Allocate = ExAllocatePoolWithTag;

    } else {
        Lookaside->L.Allocate = Allocate;
    }

    if (Free == NULL) {
        Lookaside->L.Free = ExFreePool;

    } else {
        Lookaside->L.Free = Free;
    }

    Lookaside->L.LastTotalAllocates = 0;
    Lookaside->L.LastAllocateMisses = 0;
    KeInitializeSpinLock(&Lookaside->Lock);

    //
    // Insert the lookaside list structure is the system nonpaged lookaside
    // list.
    //

    ExInterlockedInsertTailList(&ExNPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExNPagedLookasideLock);
    return;
}

VOID
ExDeleteNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes a paged lookaside structure from the system paged
    lookaside list and frees any entries specified by the lookaside structure.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

Return Value:

    None.

--*/

{

    PVOID Entry;
    KIRQL OldIrql;

    //
    // Acquire the nonpaged system lookaside list lock and remove the
    // specified lookaside list structure from the list.
    //

    ExAcquireSpinLock(&ExNPagedLookasideLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    ExReleaseSpinLock(&ExNPagedLookasideLock, OldIrql);

    //
    // Remove all pool entries from the specified lookaside structure
    // and free them.
    //

    Lookaside->L.Allocate = ExpDummyAllocate;
    while ((Entry = ExAllocateFromNPagedLookasideList(Lookaside)) != NULL) {
        (Lookaside->L.Free)(Entry);
    }

    return;
}

VOID
ExInitializePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    )

/*++

Routine Description:

    This function initializes a paged lookaside list structure and inserts
    the structure in the system paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

    Allocate - Supplies an optional pointer to an allocate function.

    Free - Supplies an optional pointer to a free function.

    Flags - Supplies the pool allocation flags which are merged with the
        pool allocation type (NonPagedPool) to control pool allocation.

    Size - Supplies the size for the lookaside list entries.

    Tag - Supplies the pool tag for the lookaside list entries.

    Depth - Supplies the maximum depth of the lookaside list.

Return Value:

    None.

--*/

{

    //
    // Initialize the lookaside list structure.
    //

    ExInitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.Depth = MINIMUM_LOOKASIDE_DEPTH;
    Lookaside->L.MaximumDepth = 256; //Depth;
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = PagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = (ULONG)Size;
    if (Allocate == NULL) {
        Lookaside->L.Allocate = ExAllocatePoolWithTag;

    } else {
        Lookaside->L.Allocate = Allocate;
    }

    if (Free == NULL) {
        Lookaside->L.Free = ExFreePool;

    } else {
        Lookaside->L.Free = Free;
    }

    Lookaside->L.LastTotalAllocates = 0;
    Lookaside->L.LastAllocateMisses = 0;
    ExInitializeFastMutex(&Lookaside->Lock);

    //
    // Insert the lookaside list structure in the system paged lookaside
    // list.
    //

    ExInterlockedInsertTailList(&ExPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExPagedLookasideLock);
    return;
}

VOID
ExDeletePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes a paged lookaside structure from the system paged
    lookaside list and frees any entries specified by the lookaside structure.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    None.

--*/

{

    PVOID Entry;
    KIRQL OldIrql;

    //
    // Acquire the paged system lookaside list lock and remove the
    // specified lookaside list structure from the list.
    //

    ExAcquireSpinLock(&ExPagedLookasideLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    ExReleaseSpinLock(&ExPagedLookasideLock, OldIrql);

    //
    // Remove all pool entries from the specified lookaside structure
    // and free them.
    //

    Lookaside->L.Allocate = ExpDummyAllocate;
    while ((Entry = ExAllocateFromPagedLookasideList(Lookaside)) != NULL) {
        (Lookaside->L.Free)(Entry);
    }

    return;
}

#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_IA64_)


PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;

    if (Isx86FeaturePresent(KF_CMPXCHG8B)) {
        if ((Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead,
                                                NULL)) == NULL) {

            Lookaside->L.AllocateMisses += 1;
            Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                            Lookaside->L.Size,
                                            Lookaside->L.Tag);
        }

        return Entry;
    }

    ExAcquireFastMutex(&Lookaside->Lock);
    Entry = PopEntryList(&Lookaside->L.ListHead.Next);
    if (Entry == NULL) {
        ExReleaseFastMutex(&Lookaside->Lock);
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);

    } else {
        Lookaside->L.ListHead.Depth -= 1;
        ExReleaseFastMutex(&Lookaside->Lock);
    }

    return Entry;
}

VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{

    Lookaside->L.TotalFrees += 1;

    if (Isx86FeaturePresent(KF_CMPXCHG8B)) {
        if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
            Lookaside->L.FreeMisses += 1;
            (Lookaside->L.Free)(Entry);

        } else {
            ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                        (PSINGLE_LIST_ENTRY)Entry,
                                        NULL);
        }

        return;
    }

    ExAcquireFastMutex(&Lookaside->Lock);
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        ExReleaseFastMutex(&Lookaside->Lock);
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {
        PushEntryList(&Lookaside->L.ListHead.Next, (PSINGLE_LIST_ENTRY)Entry);
        Lookaside->L.ListHead.Depth += 1;
        ExReleaseFastMutex(&Lookaside->Lock);
    }

    return;
}

#endif

PVOID
ExpDummyAllocate (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This function is a dummy allocation routine which is used to empty
    a lookaside list.

Arguments:

    PoolType - Supplies the type of pool to allocate.

    NumberOfBytes - supplies the number of bytes to allocate.

    Tag - supplies the pool tag.

Return Value:

    NULL is returned as the function value.

--*/

{

    return NULL;
}
