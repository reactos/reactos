/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    range.c

Abstract:

    Kernel-mode range list support for arbiters

Author:

    Andy Thornton (andrewth) 02/17/97

Revision History:

--*/

#include "ntrtlp.h"
#include "range.h"

#if DBG

//
// Debug print level:
//    -1 = no messages
//     0 = vital messages only
//     1 = call trace
//     2 = verbose messages
//

LONG RtlRangeDebugLevel = 0;

#endif

NTSTATUS
RtlpAddRange(
    IN OUT PLIST_ENTRY ListHead,
    IN PRTLP_RANGE_LIST_ENTRY Entry,
    IN ULONG AddRangeFlags
    );

NTSTATUS
RtlpAddToMergedRange(
    IN PRTLP_RANGE_LIST_ENTRY Merged,
    IN PRTLP_RANGE_LIST_ENTRY Entry,
    IN ULONG AddRangeFlags
    );

NTSTATUS
RtlpConvertToMergedRange(
    IN PRTLP_RANGE_LIST_ENTRY Entry
    );

PRTLP_RANGE_LIST_ENTRY
RtlpCreateRangeListEntry(
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN UCHAR Attributes,
    IN PVOID UserData,
    IN PVOID Owner
    );

NTSTATUS
RtlpAddIntersectingRanges(
    IN PLIST_ENTRY ListHead,
    IN PRTLP_RANGE_LIST_ENTRY First,
    IN PRTLP_RANGE_LIST_ENTRY Entry,
    IN ULONG AddRangeFlags
    );

NTSTATUS
RtlpDeleteFromMergedRange(
    IN PRTLP_RANGE_LIST_ENTRY Delete,
    IN PRTLP_RANGE_LIST_ENTRY Merged
    );

PRTLP_RANGE_LIST_ENTRY
RtlpCopyRangeListEntry(
    PRTLP_RANGE_LIST_ENTRY Entry
    );

VOID
RtlpDeleteRangeListEntry(
    IN PRTLP_RANGE_LIST_ENTRY Entry
    );

BOOLEAN
RtlpIsRangeAvailable(
    IN PRTL_RANGE_LIST_ITERATOR Iterator,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN UCHAR AttributeAvailableMask,
    IN BOOLEAN SharedOK,
    IN BOOLEAN NullConflictOK,
    IN BOOLEAN Forward,
    IN PVOID Context OPTIONAL,
    IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL
    );

#if DBG

VOID
RtlpDumpRangeListEntry(
    LONG Level,
    PRTLP_RANGE_LIST_ENTRY Entry,
    BOOLEAN Indent
    );

VOID
RtlpDumpRangeList(
    LONG Level,
    PRTL_RANGE_LIST RangeList
    );

#else

#define RtlpDumpRangeListEntry(Level, Entry, Indent)
#define RtlpDumpRangeList(Level, RangeList)

#endif // DBG

//
// Make everything pageable or init
//

#ifdef ALLOC_PRAGMA

#if defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(INIT, RtlInitializeRangeListPackage)
#endif

#pragma alloc_text(PAGE, RtlpAddRange)
#pragma alloc_text(PAGE, RtlpAddToMergedRange)
#pragma alloc_text(PAGE, RtlpConvertToMergedRange)
#pragma alloc_text(PAGE, RtlpCreateRangeListEntry)
#pragma alloc_text(PAGE, RtlpAddIntersectingRanges)
#pragma alloc_text(PAGE, RtlpDeleteFromMergedRange)
#pragma alloc_text(PAGE, RtlpCopyRangeListEntry)
#pragma alloc_text(PAGE, RtlpDeleteRangeListEntry)
#pragma alloc_text(PAGE, RtlpIsRangeAvailable)

#if DBG
#pragma alloc_text(PAGE, RtlpDumpRangeListEntry)
#pragma alloc_text(PAGE, RtlpDumpRangeList)
#endif

#pragma alloc_text(PAGE, RtlInitializeRangeList)
#pragma alloc_text(PAGE, RtlAddRange)
#pragma alloc_text(PAGE, RtlDeleteRange)
#pragma alloc_text(PAGE, RtlDeleteOwnersRanges)
#pragma alloc_text(PAGE, RtlCopyRangeList)
#pragma alloc_text(PAGE, RtlFreeRangeList)
#pragma alloc_text(PAGE, RtlIsRangeAvailable)
#pragma alloc_text(PAGE, RtlFindRange)
#pragma alloc_text(PAGE, RtlGetFirstRange)
#pragma alloc_text(PAGE, RtlGetNextRange)
#pragma alloc_text(PAGE, RtlMergeRangeLists)
#pragma alloc_text(PAGE, RtlInvertRangeList)

#endif // ALLOC_PRAGMA

//
// Range List memory allocation
//

#if defined(NTOS_KERNEL_RUNTIME)

//
// The kernel mode range list API uses a lookaside list to speed allocation
// of range list entries.  The PAGED_LOOKASIDE_LIST structure should be non-paged.
//

#define RTLP_RANGE_LIST_ENTRY_LOOKASIDE_DEPTH   16

PAGED_LOOKASIDE_LIST RtlpRangeListEntryLookasideList;

VOID
RtlInitializeRangeListPackage(
    VOID
    )
/*++

Routine Description:

    This routine initializes the stuctures required by the range list
    APIs.  It is called during system initialization (Phase1Initialization)
    and should be before any of the range list apis are called.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ExInitializePagedLookasideList(
        &RtlpRangeListEntryLookasideList,
        NULL,
        NULL,
        0,
        sizeof(RTLP_RANGE_LIST_ENTRY),
        RTL_RANGE_LIST_ENTRY_TAG,
        RTLP_RANGE_LIST_ENTRY_LOOKASIDE_DEPTH
        );

}

//
// PRANGE_LIST_ENTRY
// RtlpAllocateRangeListEntry(
//     VOID
//     )
//
#define RtlpAllocateRangeListEntry()                                    \
    (PRTLP_RANGE_LIST_ENTRY) ExAllocateFromPagedLookasideList(          \
        &RtlpRangeListEntryLookasideList                                \
        )

//
// VOID
// RtlpFreeRangeListEntry(
//     IN PRTLP_RANGE_LIST_ENTRY Entry
//     )
//
#define RtlpFreeRangeListEntry(Entry)                                   \
    ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, (Entry))


//
// PVOID
// RtlpRangeListAllocatePool(
//     IN ULONG Size
//     )
//
#define RtlpRangeListAllocatePool(Size)                                 \
    ExAllocatePoolWithTag(PagedPool, (Size), RTL_RANGE_LIST_MISC_TAG)

//
// VOID
// RtlpRangeListFreePool(
//     IN PVOID Free
//     )
//
#define RtlpRangeListFreePool(Free)                                     \
    ExFreePool(Free)


#else // defined(NTOS_KERNEL_RUNTIME)


//
// Usermode range lists use the standard Rtl heap for allocations
//

//
// PRANGE_LIST_ENTRY
// RtlpAllocateRangeListEntry(
//     VOID
//     );
//
#define RtlpAllocateRangeListEntry()                                    \
    (PRTLP_RANGE_LIST_ENTRY) RtlAllocateHeap(                           \
        RtlProcessHeap(),                                               \
        RTL_RANGE_LIST_ENTRY_TAG,                                       \
        sizeof(RTLP_RANGE_LIST_ENTRY)                                   \
        )

//
// VOID
// RtlpFreeRangeListEntry(
//     IN PRTLP_RANGE_LIST_ENTRY Entry
//     )
//
#define RtlpFreeRangeListEntry(Entry)                                   \
    RtlFreeHeap( RtlProcessHeap(), 0, (Entry) )

//
// PVOID
// RtlpRangeListAllocatePool(
//     IN ULONG Size
//     )
//
#define RtlpRangeListAllocatePool(Size)                                 \
    RtlAllocateHeap(RtlProcessHeap(), RTL_RANGE_LIST_MISC_TAG, (Size))

//
// VOID
// RtlpRangeListFreePool(
//     IN PVOID Free
//     )
//
#define RtlpRangeListFreePool(Free)                                     \
    RtlFreeHeap( RtlProcessHeap(), 0, (Free) )


#endif // defined(NTOS_KERNEL_RUNTIME)

VOID
RtlInitializeRangeList(
    IN OUT PRTL_RANGE_LIST RangeList
    )
/*++

Routine Description:

    This routine initializes a range list.  It must be called before the range
    list is passed to any of the other range list functions.  Initially the
    range list contains no ranges

Arguments:

    RangeList - Pointer to a user allocated RTL_RANGE_LIST structre to be
        initialized.

Return Value:

    None.

--*/
{
    RTL_PAGED_CODE();

    ASSERT(RangeList);

    DEBUG_PRINT(1, ("RtlInitializeRangeList(0x%08x)\n", RangeList));

    InitializeListHead(&RangeList->ListHead);
    RangeList->Flags = 0;
    RangeList->Count = 0;
    RangeList->Stamp = 0;
}

NTSTATUS
RtlAddRange(
    IN OUT PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN UCHAR Attributes,
    IN ULONG Flags,
    IN PVOID UserData, OPTIONAL
    IN PVOID Owner     OPTIONAL
    )
/*++

Routine Description:

    This routine adds a new range with the specified properties to a range list.

Arguments:

    RangeList - Pointer to the range list to which the new range is to be added.
        It must have been previously initialized using RtlInitializeRangeList.

    Start - The location of the start of the new range.

    End - The location of the end of the new range.

    Flags - These determine the range's properties and how it is added:

        RTL_RANGE_LIST_ADD_IF_CONFLICT - The range should be added even if it
            overlaps another range.  In this case the RTL_RANGE_CONFLICT flag
            is set.

        RTL_RANGE_LIST_ADD_SHARED - The range is marked as an RTL_RANGE_SHARED
            and will successfully be added if it overlaps another shared range.
            It can be speficied in conjunction with the above ADD_IF_CONFLICT
            flag in which case if the range overlaps a non-shared range it will
            be marked as both RTL_RANGE_SHARED and RTL_RANGE_CONFLICT.

    UserData - Extra data to be stored with the range.  The system will not
        attempt to interpret it.

    Owner - A cookie that represents the entity that owns this range.  (A
        pointer to some object is the most likley).  The system will not
        attempt to interpret it, just use it to distinguish the range from
        another with the same start and end.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INVALID_PARAMETER
    STATUS_RANGE_LIST_CONFLICT
    STATUS_INSUFFICIENT_RESOURCES

--*/
{

    NTSTATUS status;
    PRTLP_RANGE_LIST_ENTRY newEntry = NULL;

    RTL_PAGED_CODE();

    DEBUG_PRINT(1,
        ("RtlAddRange(0x%08x, 0x%I64x, 0x%I64x, 0x%08x, 0x%08x, 0x%08x)\n",
        RangeList,
        Start,
        End,
        Flags,
        UserData,
        Owner
        ));

    //
    // Validate parameters
    //

    if (End < Start) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Create a new entry
    //

    if (!(newEntry = RtlpCreateRangeListEntry(Start, End, Attributes, UserData, Owner))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Mark the new entry as shared if appropriate
    //

    if (Flags & RTL_RANGE_LIST_ADD_SHARED) {
        newEntry->PublicFlags |= RTL_RANGE_SHARED;
    }

    status = RtlpAddRange(&RangeList->ListHead,
                        newEntry,
                        Flags
                        );

    if (NT_SUCCESS(status)) {

        //
        // We added a range so bump the count
        //
        RangeList->Count++;
        RangeList->Stamp++;

    } else {

        //
        // We didn't add a range so free up the entry
        //

        RtlpFreeRangeListEntry(newEntry);
    }

    return status;

}

NTSTATUS
RtlpAddRange(
    IN OUT PLIST_ENTRY ListHead,
    IN PRTLP_RANGE_LIST_ENTRY Entry,
    IN ULONG AddRangeFlags
    )
/*++

Routine Description:

    This routine implement the AddRange operation adding the range in the
    appropriate place in the sorted range list, converting ranges to merged
    ranges and setting RTL_RANGE_CONFLICT flags as necessary.

Arguments:

    ListHead - The list of the range list to which the range should be added.

    Entry - The new entry to be added to the range list

    AddRangeFlags - The Flags argument to RtlAddRange, see above.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_RANGE_LIST_CONFLICT
    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTLP_RANGE_LIST_ENTRY previous, current;
    ULONGLONG start, end;

    DEBUG_PRINT(2,
                ("RtlpAddRange(0x%08x, 0x%08x{0x%I64x-0x%I64x}, 0x%08x)\n",
                ListHead,
                Entry,
                Entry->Start,
                Entry->End,
                AddRangeFlags
                ));

    RTL_PAGED_CODE();
    ASSERT(Entry);

    start = Entry->Start;
    end = Entry->End;

    //
    // Clear the conflict flag if it was left around
    //

    Entry->PublicFlags &= ~RTL_RANGE_CONFLICT;

    //
    // Iterate through the list and find where to insert the entry
    //

    FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY, ListHead, current) {

        if (end < current->Start) {

            //
            // The new range is completely before this one
            //

            DEBUG_PRINT(2, ("Completely before\n"));

            InsertEntryList(current->ListEntry.Blink,
                            &Entry->ListEntry
                            );

            goto exit;

        } else if (RANGE_INTERSECT(Entry, current)) {

            status = RtlpAddIntersectingRanges(ListHead,
                       current,
                       Entry,
                       AddRangeFlags);

            goto exit;

        }
    }

    //
    // New range is after all existing ranges
    //

    DEBUG_PRINT(2, ("After all existing ranges\n"));

    InsertTailList(ListHead,
                   &Entry->ListEntry
                  );

exit:

    return status;

}

NTSTATUS
RtlpAddToMergedRange(
    IN PRTLP_RANGE_LIST_ENTRY Merged,
    IN PRTLP_RANGE_LIST_ENTRY Entry,
    IN ULONG AddRangeFlags
    )
/*++

Routine Description:

    This routine adds a new range to a merged range, setting the
    RTL_RANGE_CONFLICT flags if necessary.

Arguments:

    Merged - The merged range to which Entry should be added.

    Entry - The new entry to be added to the range list

    AddRangeFlags - The Flags argument to RtlAddRange, see above.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_RANGE_LIST_CONFLICT - indictates that the range was not added because
        it conflicted with another range and conflicts are not allowed

--*/
{
    PRTLP_RANGE_LIST_ENTRY current;
    PLIST_ENTRY insert = NULL;
    BOOLEAN entryShared;

    RTL_PAGED_CODE();
    ASSERT(Merged);
    ASSERT(Entry);
    ASSERT(MERGED(Merged));

    entryShared = SHARED(Entry);

    //
    // Insert it into the merged list, this is sorted in order of start
    //

    FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY, &Merged->Merged.ListHead, current) {

        //
        // Do we conflict?
        //

        if (RANGE_INTERSECT(current, Entry)
        && !(entryShared && SHARED(current))) {

            //
            // Are conflicts ok?
            //

            if (AddRangeFlags & RTL_RANGE_LIST_ADD_IF_CONFLICT) {

                //
                // Yes - Mark both entries as conflicting
                //

                current->PublicFlags |= RTL_RANGE_CONFLICT;
                Entry->PublicFlags |= RTL_RANGE_CONFLICT;

            } else {

                //
                // No - Fail
                //

                return STATUS_RANGE_LIST_CONFLICT;

            }
        }

        //
        // Have we not yet found the insertion point and just passed it?
        //

        if (!insert && current->Start > Entry->Start) {

            //
            // Insert is before current
            //

            insert = current->ListEntry.Blink;
        }
    }

    //
    // Did we find where to insert the new range?
    //

    if (!insert) {

        //
        // New range is after all existing ranges
        //

        InsertTailList(&Merged->Merged.ListHead,
                       &Entry->ListEntry
                      );

    } else {

        //
        // Insert in the list
        //

        InsertEntryList(insert,
                        &Entry->ListEntry
                        );
    }


    //
    // Expand the merged range if necessary
    //

    if (Entry->Start < Merged->Start) {
        Merged->Start = Entry->Start;
    }

    if (Entry->End > Merged->End) {
        Merged->End = Entry->End;
    }

    //
    // If we just added a shared range to a completely shared merged
    // range then the shared flag can stay otherwise it must go
    //

    if (SHARED(Merged) && !entryShared) {

        DEBUG_PRINT(2,
            ("RtlpAddToMergedRange: Merged range no longer completely shared\n"));

        Merged->PublicFlags &= ~RTL_RANGE_SHARED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlpConvertToMergedRange(
    IN PRTLP_RANGE_LIST_ENTRY Entry
    )
/*++

Routine Description:

    This converts a non-merged range into a merged range with one member, the
    range that was just converted.

Arguments:

    Entry - The entry to be converted into a merged range.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    PRTLP_RANGE_LIST_ENTRY newEntry;

    RTL_PAGED_CODE();
    ASSERT(Entry);
    ASSERT(!MERGED(Entry));
    ASSERT(!CONFLICT(Entry));

    //
    // Create a new entry
    //

    if (!(newEntry = RtlpCopyRangeListEntry(Entry))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Convert the current entry into a merged one NB. Throw away all the
    // private flags but leave the public ones as they can only be shared.
    //

    InitializeListHead(&Entry->Merged.ListHead);
    Entry->PrivateFlags = RTLP_RANGE_LIST_ENTRY_MERGED;

    ASSERT(Entry->PublicFlags == RTL_RANGE_SHARED || Entry->PublicFlags == 0);

    //
    // Add the range
    //

    InsertHeadList(&Entry->Merged.ListHead,
                   &newEntry->ListEntry
                   );


    return STATUS_SUCCESS;
}

PRTLP_RANGE_LIST_ENTRY
RtlpCreateRangeListEntry(
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN UCHAR Attributes,
    IN PVOID UserData,
    IN PVOID Owner
    )
/*++

Routine Description:

    This routine allocates a new range list entry and fills it in the the data
    provided.

Arguments:

    Start - The location of the start of the new range.

    End - The location of the end of the new range.

    Attributes - Extra data (normally flags) to be stored with the range. The
        system will not attempt to interpret it.

    UserData - Extra data to be stored with the range.  The system will not
        attempt to interpret it.

    Owner - A cookie that represents the entity that owns this range.  (A
        pointer to some object is the most likley).  The system will not
        attempt to interpret it, just use it to distinguish the range from
        another with the same start and end.

Return Value:

    Pointer to the new range list entry or NULL if a new entry could not be
    allocated.

--*/
{
    PRTLP_RANGE_LIST_ENTRY entry;

    RTL_PAGED_CODE();
    ASSERT(Start <= End);

    //
    // Allocate a new entry
    //

    if (entry = RtlpAllocateRangeListEntry()) {

        //
        // Fill in the details
        //

#if DBG
        entry->ListEntry.Flink = NULL;
        entry->ListEntry.Blink = NULL;
#endif

        entry->PublicFlags = 0;
        entry->PrivateFlags = 0;
        entry->Start = Start;
        entry->End = End;
        entry->Allocated.UserData = UserData;
        entry->Allocated.Owner = Owner;
        entry->Attributes = Attributes;
    }

    return entry;

}

NTSTATUS
RtlpAddIntersectingRanges(
    IN PLIST_ENTRY ListHead,
    IN PRTLP_RANGE_LIST_ENTRY First,
    IN PRTLP_RANGE_LIST_ENTRY Entry,
    IN ULONG AddRangeFlags
    )
/*++

Routine Description:

    This routine adds a range to a range list when the new range overlaps
    an existing range.  Ranges are converted to mergedranges and the
    RTL_RANGE_CONFLICT flag is set as necessary.

Arguments:

    ListHead - The list of the range list to which the range should be added.

    First - The first range that intersects

    Entry - The new range to be added

    AddRangeFlags - The Flags argument to RtlAddRange, see above.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INSUFFICIENT_RESOURCES
    STATUS_RANGE_LIST_CONFLICT

--*/
{
    NTSTATUS status;
    PRTLP_RANGE_LIST_ENTRY current, next, currentMerged, nextMerged;
    BOOLEAN entryShared;

    RTL_PAGED_CODE();
    ASSERT(First);
    ASSERT(Entry);

    entryShared = SHARED(Entry);

    //
    // If we care about conflicts see if we conflict with anyone
    //

    if (!(AddRangeFlags & RTL_RANGE_LIST_ADD_IF_CONFLICT)) {

        //
        // Examine all ranges after the first intersecting one
        //

        current = First;
        FOR_REST_IN_LIST(RTLP_RANGE_LIST_ENTRY, ListHead, current) {

            if (Entry->End < current->Start) {

                //
                // We don't intersect anymore so there arn't any more
                // conflicts
                //

                break;

            } else if (MERGED(current)) {

                //
                // Check if any of the merged ranges conflict
                //

                FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY,
                                &current->Merged.ListHead,
                                currentMerged) {

                    //
                    // Do we conflict?
                    //

                    if (RANGE_INTERSECT(currentMerged, Entry)
                    && !(entryShared && SHARED(currentMerged))) {

                        //
                        // We conflict with one of the merged ranges
                        //

                        return STATUS_RANGE_LIST_CONFLICT;

                    }
                }

            } else if (!(entryShared && SHARED(current))) {

                //
                // We conflict with a non shared region in the  main list.
                //

                return STATUS_RANGE_LIST_CONFLICT;
            }
        }
    }

    //
    // Ok - either we didn't find any conflicts or we don't care about
    // them.  Now its safe to perform the merge.   Make the first
    // overlapping range into a header if it is not already one and then
    // add the rest of the ranges
    //

    if (!MERGED(First)) {

        status = RtlpConvertToMergedRange(First);

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

    }

    ASSERT(MERGED(First));

    current = RANGE_LIST_ENTRY_FROM_LIST_ENTRY(First->ListEntry.Flink);

    //
    // Consider the entries between the one following first and the last
    // intersecting one.
    //

    FOR_REST_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY, ListHead, current, next) {

         if (Entry->End < current->Start) {

            //
            // We don't intersect any more
            //

            break;
         }

        if (MERGED(current)) {

            //
            // Add all the merged ranges to the new entry
            //

            FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                                 &current->Merged.ListHead,
                                 currentMerged,
                                 nextMerged) {

                //
                // Remove the entry from the current list
                //

                RemoveEntryList(&currentMerged->ListEntry);

                //
                // Add the entry to the new merged range
                //

                status = RtlpAddToMergedRange(First,
                                            currentMerged,
                                            AddRangeFlags
                                            );

                //
                // We should not be able to fail the add but just to be
                // on the safe side...
                //

                ASSERT(NT_SUCCESS(status));

            }

            //
            // Remove and free the now empty header
            //

            ASSERT(IsListEmpty(&current->Merged.ListHead));

            RemoveEntryList(&current->ListEntry);
            RtlpFreeRangeListEntry(current);

        } else {

            //
            // Remove the entry from the main list
            //

            RemoveEntryList(&current->ListEntry);

            //
            // Add the entry to the new merged range
            //

            status = RtlpAddToMergedRange(First,
                                        current,
                                        AddRangeFlags
                                        );

            //
            // We should not be able to fail the add but just to be
            // on the safe side...
            //

            ASSERT(NT_SUCCESS(status));

        }
    }

    //
    // Finally add the entry that did the overlapping
    //

    status = RtlpAddToMergedRange(First,
                                Entry,
                                AddRangeFlags
                                );

    ASSERT(NT_SUCCESS(status));

cleanup:

    return status;

}

NTSTATUS
RtlDeleteRange(
    IN OUT PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN PVOID Owner
    )
/*++

Routine Description:

    This routine deletes a range from a range list.

Arguments:

    Start - The location of the start of the range to be deleted.

    End - The location of the end of the range to be deleted.

    Owner -  The owner of the range to be deleted, used to distinguish the
    range from another with the same start and end.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INSUFFICIENT_RESOURCES
    STATUS_RANGE_LIST_CONFLICT

--*/
{
    NTSTATUS status = STATUS_RANGE_NOT_FOUND;
    PRTLP_RANGE_LIST_ENTRY current, next, currentMerged, nextMerged;

    RTL_PAGED_CODE();
    ASSERT(RangeList);

    DEBUG_PRINT(1,
        ("RtlDeleteRange(0x%08x, 0x%I64x, 0x%I64x, 0x%08x)\n",
        RangeList,
        Start,
        End,
        Owner
        ));


    FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                         &RangeList->ListHead,
                         current,
                         next) {

        //
        // We're passed all possible intersections
        //

        if (End < current->Start) {

            //
            // We didn't find a match
            //

            break;
        }

        if (MERGED(current)) {

            //
            // COuld our range exist in this merged range?
            //

            if (Start >= current->Start && End <= current->End) {

                FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                                     &current->Merged.ListHead,
                                     currentMerged,
                                     nextMerged) {

                    if (currentMerged->Start == Start
                    && currentMerged->End == End
                    && currentMerged->Allocated.Owner == Owner) {

                        //
                        // This is the range - delete it and rebuild the merged
                        // range appropriately
                        //

                        status = RtlpDeleteFromMergedRange(currentMerged,
                                                         current
                                                         );
                        goto exit;
                    }

                }
            }

        } else if (current->Start == Start
               && current->End == End
               && current->Allocated.Owner == Owner) {

            //
            // This is the range - delete it!
            //

            RemoveEntryList(&current->ListEntry);
            RtlpFreeRangeListEntry(current);
            status = STATUS_SUCCESS;
            goto exit;
        }
    }

exit:

    if (NT_SUCCESS(status)) {

        //
        // We have removed a range so decrement the count in the header
        //

        RangeList->Count--;
        RangeList->Stamp++;

    }

    return status;
}

NTSTATUS
RtlDeleteOwnersRanges(
    IN OUT PRTL_RANGE_LIST RangeList,
    IN PVOID Owner
    )
/*++

Routine Description:

    This routine deletes all the ranges owned by a specific owner from a range
    list.

Arguments:

    Owner -  The owner of the ranges to be deleted.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTLP_RANGE_LIST_ENTRY current, next, currentMerged, nextMerged;

    RTL_PAGED_CODE();
    ASSERT(RangeList);

    DEBUG_PRINT(1,
                ("RtlDeleteOwnersRanges(0x%08x, 0x%08x)\n",
                RangeList,
                Owner
                ));

findNext:

    FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                         &RangeList->ListHead,
                         current,
                         next) {

        if (MERGED(current)) {

            FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                                 &current->Merged.ListHead,
                                 currentMerged,
                                 nextMerged) {

                if (currentMerged->Allocated.Owner == Owner) {

                    //
                    // This is the range - delete it and rebuild the merged
                    // range appropriately
                    //

                    DEBUG_PRINT(2,
                        ("RtlDeleteOwnersRanges: Deleting merged range \
                            (Start=%I64x, End=%I64x)\n",
                        currentMerged->Start,
                        currentMerged->End
                        ));

                    status = RtlpDeleteFromMergedRange(currentMerged,
                                                     current
                                                     );
                    if (!NT_SUCCESS(status)) {
                        goto cleanup;
                    }

                    RangeList->Count--;
                    RangeList->Stamp++;

                    //
                    // Can't keep scanning the list as it might have changed
                    // underneath us - start from the beginning again...
                    //
                    // BUGBUG - could keep a last safe position to go from...
                    //
                    goto findNext;

                }
            }

        } else if (current->Allocated.Owner == Owner) {

            //
            // This is the range - delete it!
            //

            RemoveEntryList(&current->ListEntry);
            RtlpFreeRangeListEntry(current);

            DEBUG_PRINT(2,
                ("RtlDeleteOwnersRanges: Deleting range (Start=%I64x,End=%I64x)\n",
                current->Start,
                current->End
                ));

            RangeList->Count--;
            RangeList->Stamp++;

            status = STATUS_SUCCESS;

        }
    }

cleanup:

    return status;

}

NTSTATUS
RtlpDeleteFromMergedRange(
    IN PRTLP_RANGE_LIST_ENTRY Delete,
    IN PRTLP_RANGE_LIST_ENTRY Merged
    )
/*++

Routine Description:

    This routine deletes a range from a merged range and rebuilds the merged
    range as appropriate.  This includes adding new merged and unmerged ranges.
    If no ranges are left in the merged range it will be deleted.

Arguments:

    BUGBUG these comments are not for this routine.

    Start - The location of the start of the range to be deleted.

    End - The location of the end of the range to be deleted.

    Owner -  The owner of the range to be deleted, used to distinguish the
    range from another with the same start and end.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTLP_RANGE_LIST_ENTRY current, next;
    LIST_ENTRY keepList;
    PLIST_ENTRY previousInsert, nextInsert;

    RTL_PAGED_CODE();
    ASSERT(MERGED(Merged));

    //
    // Remove the entry
    //

    RemoveEntryList(&Delete->ListEntry);

    //
    // Initialize the temporary list where the new list will be build
    //

    InitializeListHead(&keepList);

    //
    // Add the previously merged ranges into the keep list and put
    // any duplicates of the delete range into the delete list
    //

    FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                         &Merged->Merged.ListHead,
                         current,
                         next) {

        //
        // Add it to the keepList.  Explicitly remove the entries from the
        // list so it is still valid if we need to rebuild it.
        //

        RemoveEntryList(&current->ListEntry);

        //
        // Clear the conflict flag - if there is still a conflict RtlpAddRange
        // should set it again.
        //

        current->PublicFlags &= ~RTL_RANGE_CONFLICT;

        status = RtlpAddRange(&keepList,
                              current,
                              RTL_RANGE_LIST_ADD_IF_CONFLICT
                             );

        if (!NT_SUCCESS(status)) {
            //
            // This should only happen if we run out of pool
            //
            goto cleanup;
        }
    }

    if (!IsListEmpty(&keepList)) {

        //
        // Everything went well so splice this temporary list into the
        // main list where Merged used to be
        //

        previousInsert = Merged->ListEntry.Blink;
        nextInsert = Merged->ListEntry.Flink;

        previousInsert->Flink = keepList.Flink;
        keepList.Flink->Blink = previousInsert;

        nextInsert->Blink = keepList.Blink;
        keepList.Blink->Flink = nextInsert;

    } else {

        RemoveEntryList(&Merged->ListEntry);

    }

    //
    // Finally free the range we deleted and the merged range we have orphaned
    //

    RtlpFreeRangeListEntry(Delete);
    RtlpFreeRangeListEntry(Merged);

    return status;

cleanup:

    //
    // Things went wrong - should only be a STATUS_INSUFFICIENT_RESOURCES
    // Reconstruct the list as it was before the call using the keepList and
    // deleteList.
    //

    ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);

    //
    // Add all the ranges we moved to the keepList back into Merged
    //

    FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY, &keepList, current, next) {

        status = RtlpAddToMergedRange(Merged,
                                    current,
                                    RTL_RANGE_LIST_ADD_IF_CONFLICT
                                    );

        ASSERT(NT_SUCCESS(status));
    }

    //
    // And the one were meant to delete
    //

    status = RtlpAddToMergedRange(Merged,
                                  Delete,
                                  RTL_RANGE_LIST_ADD_IF_CONFLICT
                                 );

    return status;
}

PRTLP_RANGE_LIST_ENTRY
RtlpCopyRangeListEntry(
    PRTLP_RANGE_LIST_ENTRY Entry
    )
/*++

Routine Description:

    This routine copies a range list entry.  If the entry is merged all the
    member ranges are copied too.

Arguments:

    Entry - the range list entry to be copied.

Return Value:

    Pointer to the new range list entry or NULL if a new entry could not be
    allocated.

--*/
{
    PRTLP_RANGE_LIST_ENTRY newEntry;

    RTL_PAGED_CODE();
    ASSERT(Entry);

    if (newEntry = RtlpAllocateRangeListEntry()) {

        RtlCopyMemory(newEntry, Entry, sizeof(RTLP_RANGE_LIST_ENTRY));

#if DBG
        newEntry->ListEntry.Flink = NULL;
        newEntry->ListEntry.Blink = NULL;
#endif

        if (MERGED(Entry)) {

            //
            // Copy the merged list
            //

            PRTLP_RANGE_LIST_ENTRY current, newMerged;

            InitializeListHead(&newEntry->Merged.ListHead);

            FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY,
                            &Entry->Merged.ListHead,
                            current) {

                //
                // Allocate a new entry and copy the contents
                //

                newMerged = RtlpAllocateRangeListEntry();

                if (!newMerged) {
                    goto cleanup;
                }

                RtlCopyMemory(newMerged, current, sizeof(RTLP_RANGE_LIST_ENTRY));

                //
                // Insert the new entry
                //

                InsertTailList(&newEntry->Merged.ListHead, &newMerged->ListEntry);
            }
        }
    }

    return newEntry;

cleanup:

    //
    // Free the partially build copy
    //

    RtlpDeleteRangeListEntry(newEntry);

    return NULL;

}

NTSTATUS
RtlCopyRangeList(
    OUT PRTL_RANGE_LIST CopyRangeList,
    IN PRTL_RANGE_LIST RangeList
    )
/*++

Routine Description:

    This routine copies a range list.

Arguments:

    CopyRangeList - An initialized but empty range list where RangeList should
        be copied to.

    RangeList - The range list that is to be copied.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INSUFFICIENT_RESOURCES
    STATUS_INVALID_PARAMETER

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    PRTLP_RANGE_LIST_ENTRY current, newEntry, currentMerged, newMerged;

    RTL_PAGED_CODE();
    ASSERT(RangeList);
    ASSERT(CopyRangeList);


    DEBUG_PRINT(1,
                ("RtlCopyRangeList(0x%08x, 0x%08x)\n",
                CopyRangeList,
                RangeList
                ));

    //
    // Sanity checks...
    //

    if (CopyRangeList->Count != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Copy the header information
    //

    CopyRangeList->Flags = RangeList->Flags;
    CopyRangeList->Count = RangeList->Count;
    CopyRangeList->Stamp = RangeList->Stamp;

    //
    // Perform the copy
    //

    FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY, &RangeList->ListHead, current) {

        //
        // Copy the current entry
        //

        newEntry = RtlpCopyRangeListEntry(current);

        if (!newEntry) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        //
        // Add it into the list
        //

        InsertTailList(&CopyRangeList->ListHead, &newEntry->ListEntry);
    }

    return status;

cleanup:

    //
    // Free up the partially complete range list
    //

    RtlFreeRangeList(CopyRangeList);
    return status;

}

VOID
RtlpDeleteRangeListEntry(
    IN PRTLP_RANGE_LIST_ENTRY Entry
    )
/*++

Routine Description:

    This routine deletes a range list entry - if the entry is merged then all
    the member ranges will be deleted as well.  The entry will not be removed
    from any list before deletion - this should be done before calling this
    routine.

Arguments:

    Entry - The entry to be deleted.

Return Value:

    None

--*/

{
    RTL_PAGED_CODE();

    if (MERGED(Entry)) {

        PRTLP_RANGE_LIST_ENTRY current, next;

        //
        // Free all member ranges first
        //

        FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                             &Entry->Merged.ListHead,
                             current,
                             next) {

            RtlpFreeRangeListEntry(current);
        }
    }

    RtlpFreeRangeListEntry(Entry);
}

VOID
RtlFreeRangeList(
    IN PRTL_RANGE_LIST RangeList
    )
/*++

Routine Description:

    This routine deletes all the ranges in a range list.

Arguments:

    RangeList - the range list to operate on.

Return Value:

    None

--*/
{

    PRTLP_RANGE_LIST_ENTRY current, next;

    //
    // Sanity checks...
    //

    RTL_PAGED_CODE();
    ASSERT(RangeList);

    //
    // Clean up the header information
    //

    RangeList->Flags = 0;
    RangeList->Count = 0;

    FOR_ALL_IN_LIST_SAFE(RTLP_RANGE_LIST_ENTRY,
                         &RangeList->ListHead,
                         current,
                         next) {

        //
        // Delete the current entry
        //

        RemoveEntryList(&current->ListEntry);
        RtlpDeleteRangeListEntry(current);
    }
}

NTSTATUS
RtlIsRangeAvailable(
    IN PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN ULONG Flags,
    IN UCHAR AttributeAvailableMask,
    IN PVOID Context OPTIONAL,
    IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
    OUT PBOOLEAN Available
    )
/*++

Routine Description:

    This routine determines if a given range is available.

Arguments:

    RangeList - The range list to test availability on.

    Start - The start of the range to test for availability.

    End - The end of the range to test for availability.

    Flags - Modify the behaviour of the routine.

        RTL_RANGE_LIST_SHARED_OK - indicates that shared ranges should be
            considered to be available.

    AttributeAvailableMask - Any range encountered with any of these bits set will be
        consideredto be available.

    Available -  Pointer to a boolean which will be set to TRUE if the range
        is available, otherwise FALSE;

Return Value:

    Status code that indicates whether or not the function was successful:

--*/
{
    NTSTATUS status;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE dummy;

    RTL_PAGED_CODE();

    ASSERT(RangeList);
    ASSERT(Available);

    DEBUG_PRINT(1,
        ("RtlIsRangeAvailable(0x%08x, 0x%I64x, 0x%I64x, 0x%08x, 0x%08x)\n",
        RangeList,
        Start,
        End,
        Flags,
        Available
        ));

    //
    // Initialize iterator to the start of the list
    //
    status = RtlGetFirstRange(RangeList, &iterator, &dummy);


    if (status == STATUS_NO_MORE_ENTRIES) {
        //
        // The range list is empty therefore the range is available
        //

        *Available = TRUE;
        return STATUS_SUCCESS;

    } else if (!NT_SUCCESS(status)) {

        return status;

    }

    *Available = RtlpIsRangeAvailable(&iterator,
                                      Start,
                                      End,
                                      AttributeAvailableMask,
                                      (BOOLEAN)(Flags & RTL_RANGE_LIST_SHARED_OK),
                                      (BOOLEAN)(Flags & RTL_RANGE_LIST_NULL_CONFLICT_OK),
                                      TRUE,
                                      Context,
                                      Callback
                                      );

    return STATUS_SUCCESS;

}

BOOLEAN
RtlpIsRangeAvailable(
    IN PRTL_RANGE_LIST_ITERATOR Iterator,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN UCHAR AttributeAvailableMask,
    IN BOOLEAN SharedOK,
    IN BOOLEAN NullConflictOK,
    IN BOOLEAN Forward,
    IN PVOID Context OPTIONAL,
    IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL
    )
/*++

Routine Description:

    This routine determines if a given range is available.

Arguments:

    Iterator - An iterator set to the first range to test in the range list.

    Start - The start of the range to test for availability.

    End - The end of the range to test for availability.

    AttributeAvailableMask - Any range encountered with any of these bits set will be
        considered to be available.

    SharedOK - Indicated whether or not shared ranges are considered to be
        available.

Return Value:

    TRUE if the range is available, FALSE otherwise.

--*/
{
    PRTL_RANGE current;

    RTL_PAGED_CODE();

    ASSERT(Iterator);

    FOR_REST_OF_RANGES(Iterator, current, Forward) {

        //
        // If we have passed all possible intersections then break out.  This
        // can't be done in a merged region because of possible overlaps.
        //

        if (Forward) {
            if (!Iterator->MergedHead && End < current->Start) {
                break;
            }
        } else {
            if (!Iterator->MergedHead && Start > current->End) {
                break;
            }
        }

        //
        // Do we intersect?
        //
        if (RANGE_LIMITS_INTERSECT(Start, End, current->Start, current->End)) {

            DEBUG_PRINT(2,
                ("Intersection 0x%I64x-0x%I64x and 0x%I64x-0x%I64x\n",
                Start,
                End,
                current->Start,
                current->End
                ));

            //
            // Is the intersection not Ok because it is with a non-shared
            // region or we don't want a shared region? Or the user said that
            // it should be considered available because of the user flags set.
            //

            if (!((SharedOK && (current->Flags & RTL_RANGE_SHARED))
                  || (current->Attributes & AttributeAvailableMask)
                  || (NullConflictOK && (current->Owner == NULL))
                  )
                )  {

                //
                // If the caller provided a callback to support extra conflict
                // semantics call it
                //

                if (ARGUMENT_PRESENT(Callback)) {
                    if ((*Callback)(Context, (PRTL_RANGE)current)) {

                    DEBUG_PRINT(2,
                        ("User provided callback overrode conflict\n",
                        Start,
                        End,
                        current->Start,
                        current->End
                        ));

                        continue;
                    }
                }

                return FALSE;
            }
        }
    }


    return TRUE;
}

NTSTATUS
RtlFindRange(
    IN PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Minimum,
    IN ULONGLONG Maximum,
    IN ULONG Length,
    IN ULONG Alignment,
    IN ULONG Flags,
    IN UCHAR AttributeAvailableMask,
    IN PVOID Context OPTIONAL,
    IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
    OUT PULONGLONG Start
    )
/*++

Routine Description:

    This routine finds the first available range that meets the criterion specified.

Arguments:

    RangeList - The range list to find a range in.

    Minimum - The minimum acceptable value of the start of the range.

    Maximum - The maximum acceptable value of the end of the range.

    Length - The length of the range required.

    Alignmnent - The alignment of the start of the range.

    Flags - Modify the behaviour of the routine.

        RTL_RANGE_LIST_SHARED_OK - indicates that shared ranges should be
            considered to be available.

    AttributeAvailableMask - Any range encountered with any of these bits set will be
        considered to be available.

    Start - Pointer to a ULONGLONG where the start value will be returned on
        success.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_UNSUCCESSFUL
    STATUS_INVALID_PARAMETER

--*/
{

    ULONGLONG start, end;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE dummy;
    BOOLEAN sharedOK, nullConflictOK;

    RTL_PAGED_CODE();

    ASSERT(RangeList);
    ASSERT(Start);
    ASSERT(Alignment > 0);
    ASSERT(Length > 0);

    DEBUG_PRINT(1,
        ("RtlFindRange(0x%08x, 0x%I64x, 0x%I64x, 0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
        RangeList,
        Minimum,
        Maximum,
        Length,
        Alignment,
        Flags,
        Start
        ));

    //
    // Search from high to low, Align start if necessary
    //

    start = Maximum - (Length - 1);
    start -= start % Alignment;

    //
    // Valiate parameters
    //

    if ((Minimum > Maximum)
    || (Maximum - Minimum < Length - 1)
    || (Minimum + Alignment < Minimum)
    || (start < Minimum)
    || (Length == 0)
    || (Alignment == 0)) {

        return STATUS_INVALID_PARAMETER;
    }

    sharedOK = (BOOLEAN) Flags & RTL_RANGE_LIST_SHARED_OK;
    nullConflictOK = (BOOLEAN) Flags & RTL_RANGE_LIST_NULL_CONFLICT_OK;
    //
    // Calculate the end
    //

    end = start + Length - 1;

    //
    // Initialze the iterator to the end of the list
    //

    RtlGetLastRange(RangeList, &iterator, &dummy);

    //
    // Keep trying to find ranges until we run out of room or we
    // wrap around
    //

    do {

        DEBUG_PRINT(2,
            ("RtlFindRange: Testing range %I64x-%I64x\n",
            start,
            end
            ));

        if (RtlpIsRangeAvailable(&iterator,
                                 start,
                                 end,
                                 AttributeAvailableMask,
                                 sharedOK,
                                 nullConflictOK,
                                 FALSE,
                                 Context,
                                 Callback)) {

            *Start = start;

            //
            // Assert our result, if we produced one, is in the in
            // the range specified
            //

            ASSERT(*Start >= Minimum && *Start + Length - 1 <= Maximum);

            return STATUS_SUCCESS;
        }

        //
        // Find a suitable range starting from the one we conflicted with,
        // that is the current range in the iterator - this breaks the
        // abstraction of the iterator in the name of efficiency.
        //

        start = ((PRTLP_RANGE_LIST_ENTRY)(iterator.Current))->Start;
        if ((start - Length) > start) {

            //
            // Wrapped, fail.
            //

            break;
        }

        start -= Length;
        start -= start % Alignment;
        end = start + Length - 1;

    } while ( start >= Minimum );

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
RtlGetFirstRange(
    IN PRTL_RANGE_LIST RangeList,
    OUT PRTL_RANGE_LIST_ITERATOR Iterator,
    OUT PRTL_RANGE *Range
    )
/*++

Routine Description:

    This routine extracts the first range in a range list.  If there are no
    ranges then STATUS_NO_MORE_ENTRIES is returned.

Arguments:

    RangeList - The range list to operate on.

    Iterator - On success this contains the state of the iteration and can be
        passed to RtlGetNextRange.

    Range - On success this contains a pointer to the first range

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_NO_MORE_ENTRIES

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTLP_RANGE_LIST_ENTRY first;

    RTL_PAGED_CODE();

    //
    // Fill in the first part of the iterator
    //

    Iterator->RangeListHead = &RangeList->ListHead;
    Iterator->Stamp = RangeList->Stamp;

    if (!IsListEmpty(&RangeList->ListHead)) {

        first = RANGE_LIST_ENTRY_FROM_LIST_ENTRY(RangeList->ListHead.Flink);

        //
        // Fill in the iterator and update to point to the first merged
        // range if we are merged
        //

        if (MERGED(first)) {

            ASSERT(!IsListEmpty(&first->Merged.ListHead));

            Iterator->MergedHead = &first->Merged.ListHead;
            Iterator->Current = RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
                                    first->Merged.ListHead.Flink
                                    );

        } else {

            Iterator->MergedHead = NULL;
            Iterator->Current = first;
        }

        *Range = (PRTL_RANGE) Iterator->Current;

    } else {

        Iterator->Current = NULL;
        Iterator->MergedHead = NULL;

        *Range = NULL;

        status = STATUS_NO_MORE_ENTRIES;
    }

    return status;
}

NTSTATUS
RtlGetLastRange(
    IN PRTL_RANGE_LIST RangeList,
    OUT PRTL_RANGE_LIST_ITERATOR Iterator,
    OUT PRTL_RANGE *Range
    )
/*++

Routine Description:

    This routine extracts the first range in a range list.  If there are no
    ranges then STATUS_NO_MORE_ENTRIES is returned.

Arguments:

    RangeList - The range list to operate on.

    Iterator - On success this contains the state of the iteration and can be
        passed to RtlGetNextRange.

    Range - On success this contains a pointer to the first range

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_NO_MORE_ENTRIES

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PRTLP_RANGE_LIST_ENTRY first;

    RTL_PAGED_CODE();

    //
    // Fill in the first part of the iterator
    //

    Iterator->RangeListHead = &RangeList->ListHead;
    Iterator->Stamp = RangeList->Stamp;

    if (!IsListEmpty(&RangeList->ListHead)) {

        first = RANGE_LIST_ENTRY_FROM_LIST_ENTRY(RangeList->ListHead.Blink);

        //
        // Fill in the iterator and update to point to the first merged
        // range if we are merged
        //

        if (MERGED(first)) {

            ASSERT(!IsListEmpty(&first->Merged.ListHead));

            Iterator->MergedHead = &first->Merged.ListHead;
            Iterator->Current = RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
                                    first->Merged.ListHead.Blink
                                    );

        } else {

            Iterator->MergedHead = NULL;
            Iterator->Current = first;
        }

        *Range = (PRTL_RANGE) Iterator->Current;

    } else {

        Iterator->Current = NULL;
        Iterator->MergedHead = NULL;

        *Range = NULL;

        status = STATUS_NO_MORE_ENTRIES;
    }

    return status;
}

NTSTATUS
RtlGetNextRange(
    IN OUT PRTL_RANGE_LIST_ITERATOR Iterator,
    OUT    PRTL_RANGE *Range,
    IN     BOOLEAN MoveForwards
    )
/*++

Routine Description:

    This routine extracts the next range in a range list.  If there are no
    more ranges then STATUS_NO_MORE_ENTRIES is returned.

Arguments:

    Iterator     - The iterator filled in by RtlGet{First|Next}Range which will
                   be update on success.
    Range        - On success this contains a pointer to the next range
    MoveForwards - If true, go forwards thru the list, otherwise,
                   go backwards.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_NO_MORE_ENTRIES
    STATUS_INVALID_PARAMETER

Note:

    Add/Delete operations can not be performed on the list between calls to
    RtlGetFirstRange / RtlGetNextRange and RtlGetNextRange / RtlGetNextRange.
    If such calls are made the routine will detect and fail the call.

--*/
{
    PRTLP_RANGE_LIST_ENTRY mergedEntry, next;
    PLIST_ENTRY entry;

    RTL_PAGED_CODE();

    //
    // Make sure that we haven't changed the list between calls
    //

    if (RANGE_LIST_FROM_LIST_HEAD(Iterator->RangeListHead)->Stamp !=
            Iterator->Stamp) {

        ASSERTMSG(
            "RtlGetNextRange: Add/Delete operations have been performed while \
            iterating through a list\n", FALSE);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // If we have already reached the end of the list then return
    //

    if (!Iterator->Current) {
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    entry = &((PRTLP_RANGE_LIST_ENTRY)(Iterator->Current))->ListEntry;
    next = RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
               MoveForwards ? entry->Flink : entry->Blink);

    ASSERT(next);

    //
    // Are we in a merged range?
    //
    if (Iterator->MergedHead) {

        //
        // Have we reached the end of the merged range?
        //
        if (&next->ListEntry == Iterator->MergedHead) {

            //
            // Get back to the merged entry
            //
            mergedEntry = CONTAINING_RECORD(
                              Iterator->MergedHead,
                              RTLP_RANGE_LIST_ENTRY,
                              Merged.ListHead
                              );

            //
            // Move on to the next entry in the main list
            //

            next = MoveForwards ?
                       RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
                           mergedEntry->ListEntry.Flink
                           )
                   :   RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
                           mergedEntry->ListEntry.Blink
                           );
            Iterator->MergedHead = NULL;

        } else {

            //
            // There are merged ranges left - return the next one
            //
            Iterator->Current = next;
            *Range = (PRTL_RANGE) next;

            return STATUS_SUCCESS;
        }
    }

    //
    // Have we reached the end of the main list?
    //
    if (&next->ListEntry == Iterator->RangeListHead) {

        //
        // Tell the caller there are no more ranges
        //
        Iterator->Current = NULL;
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;

    } else {

        //
        // Is the next range merged?
        //

        if (MERGED(next)) {

            //
            // Goto the first merged entry
            //
            ASSERT(!Iterator->MergedHead);

            Iterator->MergedHead = &next->Merged.ListHead;
            Iterator->Current = MoveForwards ?
                                    RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
                                        next->Merged.ListHead.Flink
                                        )
                                :   RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
                                        next->Merged.ListHead.Blink
                                        );
        } else {

            //
            // Go to the next entry in the main list
            //

            Iterator->Current = RANGE_LIST_ENTRY_FROM_LIST_ENTRY(
                                    &next->ListEntry
                                    );
        }

        *Range = (PRTL_RANGE) Iterator->Current;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlMergeRangeLists(
    OUT PRTL_RANGE_LIST MergedRangeList,
    IN PRTL_RANGE_LIST RangeList1,
    IN PRTL_RANGE_LIST RangeList2,
    IN ULONG Flags
    )
/*++

Routine Description:

    This routine merges two range lists into one.

Arguments:

    MergedRangeList - An empty range list where on success the result of the
        merge will be placed.

    RangeList1 - One of the range lists to be merged.

    RangeList2 - The other the range list to merged.

    Flags - Modifies the behaviour of the routine:

        RTL_RANGE_LIST_MERGE_IF_CONFLICT - Merged ranges even if the conflict.

Return Value:

    Status code that indicates whether or not the function was successful:

    STATUS_INSUFFICIENT_RESOURCES
    STATUS_RANGE_LIST_CONFLICT

--*/
{
    NTSTATUS status;
    PRTLP_RANGE_LIST_ENTRY current, currentMerged, newEntry;
    ULONG addFlags;

    RTL_PAGED_CODE();

    DEBUG_PRINT(1,
            ("RtlMergeRangeList(0x%08x, 0x%08x, 0x%08x, 0x%08x)\n",
            MergedRangeList,
            RangeList1,
            RangeList2,
            Flags
            ));

    //
    // Copy the first range list
    //

    status = RtlCopyRangeList(MergedRangeList, RangeList1);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Add all ranges from 2nd list
    //

    FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY, &RangeList2->ListHead, current) {

        if (MERGED(current)) {

            FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY,
                            &current->Merged.ListHead,
                            currentMerged) {

                if (!(newEntry = RtlpCopyRangeListEntry(currentMerged))) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto cleanup;
                }

                if (CONFLICT(currentMerged)) {

                    //
                    // If a range was already conflicting in then it will conflict in
                    // the merged range list - allow this.
                    //

                    addFlags = Flags | RTL_RANGE_LIST_ADD_IF_CONFLICT;
                } else {

                    addFlags = Flags;
                }

                status = RtlpAddRange(&MergedRangeList->ListHead,
                                      newEntry,
                                      addFlags
                                      );

            }

        } else {


            if (!(newEntry = RtlpCopyRangeListEntry(current))){
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto cleanup;
            }

            if (CONFLICT(current)) {

                //
                // If a range was already conflicting in then it will conflict in
                // the merged range list - allow this.
                //

                addFlags = Flags | RTL_RANGE_LIST_ADD_IF_CONFLICT;
            } else {
                addFlags = Flags;
            }

            status = RtlpAddRange(&MergedRangeList->ListHead,
                                  newEntry,
                                  addFlags
                                  );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }
        }

    }
    //
    // Correct the count
    //

    MergedRangeList->Count += RangeList2->Count;
    MergedRangeList->Stamp += RangeList2->Count;

    return status;

cleanup:

    //
    // Something went wrong... Free up what we have built of the
    // new list and return the error
    //

    RtlFreeRangeList(MergedRangeList);

    return status;

}

NTSTATUS
RtlInvertRangeList(
    OUT PRTL_RANGE_LIST InvertedRangeList,
    IN PRTL_RANGE_LIST RangeList
    )
/*

Routine Description:

    This inverts a range list so that all areas which are allocated
    in InvertedRangeList will not be in RangeList, and vice
    versa. The ranges in InvertedRangeList will all be owned by NULL.

Arguments:

    InvertedRangeList - a pointer to an empty Range List to be filled
        with the inverted list

    RangeList - a pointer to the Range List to be inverted

Return Value:

    Status of operation.

*/

{

    PRTLP_RANGE_LIST_ENTRY currentRange;
    ULONGLONG currentStart = 0;
    NTSTATUS status;

    RTL_PAGED_CODE();

    //
    // if Inverted List does not start out empty, the inverted list
    // is meaningless
    //

    ASSERT(InvertedRangeList->Count == 0);

    //
    // iterate through all elements of the ReverseAllocation
    // adding the unallocated part before the current element
    // to the RealAllocation
    //

    FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY,
                    &RangeList->ListHead,
                    currentRange) {

        if (currentRange->Start > currentStart) {

            //
            // we want a NULL range owner to show that the
            // range is unavailable
            //
            status = RtlAddRange(InvertedRangeList,
                                 currentStart,
                                 currentRange->Start-1,
                                 0,            // Attributes
                                 0,            // Flags
                                 0,            // UserData
                                 NULL);        // Owner

            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

        currentStart = currentRange->End + 1;
    }

    //
    // add the portion of the address space above the last
    // element in the ReverseAllocation to the RealAllocation
    //
    // unless we've wrapped, in which case we've already added
    // the last element
    //

    if (currentStart > (currentStart - 1)) {

        status = RtlAddRange(InvertedRangeList,
                             currentStart,
                             MAX_ULONGLONG,
                             0,
                             0,
                             0,
                             NULL);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;

}


#if DBG

VOID
RtlpDumpRangeListEntry(
    LONG Level,
    PRTLP_RANGE_LIST_ENTRY Entry,
    BOOLEAN Indent
    )
{
    PWSTR indentString;
    PRTLP_RANGE_LIST_ENTRY current;

    RTL_PAGED_CODE();

    if (Indent) {
        indentString = L"\t\t";
    } else {
        indentString = L"";
    }
    //
    // Print the range
    //

    DEBUG_PRINT(Level,
                ("%sRange (0x%08x): 0x%I64x-0x%I64x\n",
                indentString,
                Entry,
                Entry->Start,
                Entry->End
                ));

    //
    // Print the flags
    //

    DEBUG_PRINT(Level, ("%s\tPrivateFlags: ", indentString));

    if (MERGED(Entry)) {
        DEBUG_PRINT(Level, ("MERGED "));

    }

    DEBUG_PRINT(Level, ("\n%s\tPublicFlags: ", indentString));

    if (SHARED(Entry)) {
        DEBUG_PRINT(Level, ("SHARED "));
    }

    if (CONFLICT(Entry)) {
        DEBUG_PRINT(Level, ("CONFLICT "));
    }

    DEBUG_PRINT(Level, ("\n"));


    if (MERGED(Entry)) {

        DEBUG_PRINT(Level, ("%sMerged entries:\n", indentString));

        //
        // Print the merged entries
        //

        FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY,
                        &Entry->Merged.ListHead,
                        current) {
            RtlpDumpRangeListEntry(Level, current, TRUE);
        }


    } else {

        //
        // Print the other data
        //

        DEBUG_PRINT(Level,
            ("%s\tUserData: 0x%08x\n\tOwner: 0x%08x\n",
            indentString,
            Entry->Allocated.UserData,
            Entry->Allocated.Owner
            ));
    }
}

VOID
RtlpDumpRangeList(
    LONG Level,
    PRTL_RANGE_LIST RangeList
    )

{
    PRTLP_RANGE_LIST_ENTRY current, currentMerged;

    RTL_PAGED_CODE();

    DEBUG_PRINT(Level,
                ("*** Range List (0x%08x) - Count: %i\n",
                RangeList,
                RangeList->Count
                ));

    FOR_ALL_IN_LIST(RTLP_RANGE_LIST_ENTRY, &RangeList->ListHead, current) {

        //
        // Print the entry
        //

        RtlpDumpRangeListEntry(Level, current, FALSE);
    }
}

#endif
