/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/rangelist.c
 * PURPOSE:           Range list implementation
 * PROGRAMMERS:       No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* TYPES ********************************************************************/

typedef struct _RTL_RANGE_ENTRY
{
    LIST_ENTRY Entry;
    RTL_RANGE Range;
} RTL_RANGE_ENTRY, *PRTL_RANGE_ENTRY;

/* PRIVATE FUNCTIONS ********************************************************/

/**********************************************************************
 * NAME							PRIVATE
 * 	RtlpOverlaps
 *
 * DESCRIPTION
 *	Returns whether the closed interval [Start1, End1] intersects the
 *	closed interval [Start2, End2].
 */
static
BOOLEAN
RtlpOverlaps(
    _In_ ULONGLONG Start1,
    _In_ ULONGLONG End1,
    _In_ ULONGLONG Start2,
    _In_ ULONGLONG End2)
{
    return (Start1 <= End2 && Start2 <= End1);
}

/**********************************************************************
 * NAME							PRIVATE
 * 	RtlpWindowIsAvailable
 *
 * DESCRIPTION
 *	Shared availability test used by RtlIsRangeAvailable and RtlFindRange.
 *	Walks every entry that overlaps [Start, End] and decides whether the
 *	window is free.  An overlapping entry does NOT cause a conflict when:
 *	  - the caller passed RTL_RANGE_LIST_SHARED_OK and the entry is shared,
 *	  - the entry carries an attribute present in AttributeAvailableMask,
 *	  - the caller passed RTL_RANGE_LIST_NULL_CONFLICT_OK and the entry has
 *	    a NULL owner, or
 *	  - the conflict Callback returns TRUE (asking us to ignore it).
 *	When the window is not available, *ConflictStart (if supplied) receives
 *	the lowest Start among the conflicting entries so a caller searching
 *	top-down can jump past them.
 */
static
BOOLEAN
RtlpWindowIsAvailable(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ ULONG Flags,
    _In_ UCHAR AttributeAvailableMask,
    _In_opt_ PVOID Context,
    _In_opt_ PRTL_CONFLICT_RANGE_CALLBACK Callback,
    _Out_opt_ PULONGLONG ConflictStart)
{
    PLIST_ENTRY Entry;
    BOOLEAN Available = TRUE;
    ULONGLONG Lowest = 0;

    for (Entry = RangeList->ListHead.Flink;
         Entry != &RangeList->ListHead;
         Entry = Entry->Flink)
    {
        PRTL_RANGE_ENTRY Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);

        if (Current->Range.Start > End)
            break;

        /* Ignore entries that do not overlap the requested window */
        if (!RtlpOverlaps(Start, End, Current->Range.Start, Current->Range.End))
            continue;

        /* An overlapping entry that is treated as available is not a conflict */
        if ((Flags & RTL_RANGE_LIST_SHARED_OK) &&
            (Current->Range.Flags & RTL_RANGE_SHARED))
            continue;

        if (AttributeAvailableMask & Current->Range.Attributes)
            continue;

        if ((Flags & RTL_RANGE_LIST_NULL_CONFLICT_OK) &&
            Current->Range.Owner == NULL)
            continue;

        /* provide callers the ability to change code paths before we make the range as unavailable. */
        if (Callback != NULL && Callback(Context, &Current->Range))
            continue;

        /* This is a real conflict */
        if (Available || Current->Range.Start < Lowest)
            Lowest = Current->Range.Start;

        Available = FALSE;
    }

    if (ConflictStart != NULL && !Available)
        *ConflictStart = Lowest;

    return Available;
}

/**********************************************************************
 * NAME							PRIVATE
 * 	RtlpConflictsOnAdd
 *
 * DESCRIPTION
 *	Returns whether inserting [Start, End] would conflict with an existing
 *	entry.  Two shared ranges (the caller passing RTL_RANGE_LIST_ADD_SHARED
 *	against an already-shared entry) are allowed to coexist.
 */
static
BOOLEAN
RtlpConflictsOnAdd(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ ULONG Flags)
{
    PLIST_ENTRY Entry;

    for (Entry = RangeList->ListHead.Flink;
         Entry != &RangeList->ListHead;
         Entry = Entry->Flink)
    {
        PRTL_RANGE_ENTRY Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);

        if (Current->Range.Start > End)
            break;

        if (!RtlpOverlaps(Start, End, Current->Range.Start, Current->Range.End))
            continue;

        /* Overlapping shared ranges may coexist */
        if ((Flags & RTL_RANGE_LIST_ADD_SHARED) &&
            (Current->Range.Flags & RTL_RANGE_SHARED))
            continue;

        return TRUE;
    }

    return FALSE;
}

/* FUNCTIONS ***************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 * 	RtlAddRange
 *
 * DESCRIPTION
 *	Adds a range to a range list.
 *
 * ARGUMENTS
 *	RangeList		Range list.
 *	Start
 *	End
 *	Attributes
 *	Flags
 *	UserData
 *	Owner
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlAddRange(IN OUT PRTL_RANGE_LIST RangeList,
            IN ULONGLONG Start,
            IN ULONGLONG End,
            IN UCHAR Attributes,
            IN ULONG Flags,
            IN PVOID UserData OPTIONAL,
            IN PVOID Owner OPTIONAL)
{
    PRTL_RANGE_ENTRY RangeEntry;
    //PRTL_RANGE_ENTRY Previous;
    PRTL_RANGE_ENTRY Current;
    PLIST_ENTRY Entry;

    if (Start > End)
        return STATUS_INVALID_PARAMETER;

    /*
     * Unless the caller explicitly allows adding on top of a conflict, reject
     * a range that overlaps an incompatible existing entry.
     */
    if (!(Flags & RTL_RANGE_LIST_ADD_IF_CONFLICT) &&
        RtlpConflictsOnAdd(RangeList, Start, End, Flags))
    {
        return STATUS_RANGE_LIST_CONFLICT;
    }

    /* Create new range entry */
    RangeEntry = RtlpAllocateMemory(sizeof(RTL_RANGE_ENTRY), 'elRR');
    if (RangeEntry == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize range entry */
    RangeEntry->Range.Start = Start;
    RangeEntry->Range.End = End;
    RangeEntry->Range.Attributes = Attributes;
    RangeEntry->Range.UserData = UserData;
    RangeEntry->Range.Owner = Owner;

    RangeEntry->Range.Flags = 0;
    if (Flags & RTL_RANGE_LIST_ADD_SHARED)
        RangeEntry->Range.Flags |= RTL_RANGE_SHARED;

    /* Insert range entry */
    if (RangeList->Count == 0)
    {
        InsertTailList(&RangeList->ListHead,
                       &RangeEntry->Entry);
        RangeList->Count++;
        RangeList->Stamp++;
        return STATUS_SUCCESS;
    }
    else
    {
         //Previous = NULL;
        Entry = RangeList->ListHead.Flink;
        while (Entry != &RangeList->ListHead)
        {
            Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);
            if (Current->Range.Start > RangeEntry->Range.Start)
            {
                /* Insert before current */
                DPRINT("Insert before current\n");
                InsertTailList(&Current->Entry,
                               &RangeEntry->Entry);

                RangeList->Count++;
                RangeList->Stamp++;
                return STATUS_SUCCESS;
            }

            //Previous = Current;
            Entry = Entry->Flink;
        }

        DPRINT("Insert tail\n");
        InsertTailList(&RangeList->ListHead,
                       &RangeEntry->Entry);
        RangeList->Count++;
        RangeList->Stamp++;
        return STATUS_SUCCESS;
    }

    RtlpFreeMemory(RangeEntry, 0);

    return STATUS_UNSUCCESSFUL;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlCopyRangeList
 *
 * DESCRIPTION
 *	Copy a range list.
 *
 * ARGUMENTS
 *	CopyRangeList	Pointer to the destination range list.
 *	RangeList	Pointer to the source range list.
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlCopyRangeList(OUT PRTL_RANGE_LIST CopyRangeList,
                 IN PRTL_RANGE_LIST RangeList)
{
    PRTL_RANGE_ENTRY Current;
    PRTL_RANGE_ENTRY NewEntry;
    PLIST_ENTRY Entry;

    CopyRangeList->Flags = RangeList->Flags;

    Entry = RangeList->ListHead.Flink;
    while (Entry != &RangeList->ListHead)
    {
        Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);

        NewEntry = RtlpAllocateMemory(sizeof(RTL_RANGE_ENTRY), 'elRR');
        if (NewEntry == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(&NewEntry->Range,
                      &Current->Range,
                      sizeof(RTL_RANGE));

        InsertTailList(&CopyRangeList->ListHead,
                       &NewEntry->Entry);

        CopyRangeList->Count++;

        Entry = Entry->Flink;
    }

    CopyRangeList->Stamp++;

    return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlDeleteOwnersRanges
 *
 * DESCRIPTION
 *	Delete all ranges that belong to the given owner.
 *
 * ARGUMENTS
 *	RangeList	Pointer to the range list.
 *	Owner		User supplied value that identifies the owner
 *			of the ranges to be deleted.
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteOwnersRanges(IN OUT PRTL_RANGE_LIST RangeList,
                      IN PVOID Owner)
{
    PRTL_RANGE_ENTRY Current;
    PLIST_ENTRY Entry;
    PLIST_ENTRY Next;

    Entry = RangeList->ListHead.Flink;
    while (Entry != &RangeList->ListHead)
    {
        /* Capture the next link before we possibly free this entry */
        Next = Entry->Flink;

        Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);
        if (Current->Range.Owner == Owner)
        {
            RemoveEntryList(Entry);
            RtlpFreeMemory(Current, 0);

            RangeList->Count--;
            RangeList->Stamp++;
        }

        Entry = Next;
    }

    return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlDeleteRange
 *
 * DESCRIPTION
 *	Deletes a given range.
 *
 * ARGUMENTS
 *	RangeList	Pointer to the range list.
 *	Start		Start of the range to be deleted.
 *	End		End of the range to be deleted.
 *	Owner		Owner of the ranges to be deleted.
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteRange(IN OUT PRTL_RANGE_LIST RangeList,
               IN ULONGLONG Start,
               IN ULONGLONG End,
               IN PVOID Owner)
{
    PRTL_RANGE_ENTRY Current;
    PLIST_ENTRY Entry;

    Entry = RangeList->ListHead.Flink;
    while (Entry != &RangeList->ListHead)
    {
        Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);
        if (Current->Range.Start == Start &&
            Current->Range.End == End &&
            Current->Range.Owner == Owner)
        {
            RemoveEntryList(Entry);

            RtlpFreeMemory(Current, 0);

            RangeList->Count--;
            RangeList->Stamp++;
            return STATUS_SUCCESS;
        }

        Entry = Entry->Flink;
    }

    return STATUS_RANGE_NOT_FOUND;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlFindRange
 *
 * DESCRIPTION
 *	Searches (top-down) for an unused range, honoring the shared,
 *	null-conflict and attribute-availability rules and the conflict
 *	callback.
 *
 * ARGUMENTS
 *	RangeList		Pointer to the range list.
 *	Minimum
 *	Maximum
 *	Length
 *	Alignment
 *	Flags
 *	AttributeAvailableMask
 *	Context
 *	Callback
 *	Start
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlFindRange(IN PRTL_RANGE_LIST RangeList,
             IN ULONGLONG Minimum,
             IN ULONGLONG Maximum,
             IN ULONGLONG Length,
             IN ULONGLONG Alignment,
             IN ULONG Flags,
             IN UCHAR AttributeAvailableMask,
             IN PVOID Context OPTIONAL,
             IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
             OUT PULONGLONG Start)
{
    ULONGLONG Candidate;
    ULONGLONG CandidateEnd;
    ULONGLONG ConflictStart = 0;

    if (Alignment == 0 || Length == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* A window of Length can only end at Maximum if it also fits below it */
    if (Length - 1 > Maximum)
    {
        return STATUS_RANGE_NOT_FOUND;
    }

    Candidate = (Maximum - (Length - 1)) & ~(Alignment - 1);

    for (;;)
    {
        if (Candidate < Minimum)
        {
            return STATUS_RANGE_NOT_FOUND;
        }

        CandidateEnd = Candidate + (Length - 1);

        if (RtlpWindowIsAvailable(RangeList,
                                  Candidate,
                                  CandidateEnd,
                                  Flags,
                                  AttributeAvailableMask,
                                  Context,
                                  Callback,
                                  &ConflictStart))
        {
            DPRINT("Found range: %I64x\n", Candidate);
            *Start = Candidate;
            return STATUS_SUCCESS;
        }

        /*
         * Jump the window entirely below the lowest conflicting entry.
         * Because that entry overlaps the current window, this strictly
         * decreases Candidate, so the loop always terminates.
         */
        if (ConflictStart == 0)
        {
            return STATUS_RANGE_NOT_FOUND;
        }

        if (Length - 1 > ConflictStart - 1)
        {
            return STATUS_RANGE_NOT_FOUND;
        }

        Candidate = ((ConflictStart - 1) - (Length - 1)) & ~(Alignment - 1);
    }
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlFreeRangeList
 *
 * DESCRIPTION
 *	Deletes all ranges in a range list.
 *
 * ARGUMENTS
 *	RangeList	Pointer to the range list.
 *
 * RETURN VALUE
 *	None
 *
 * @implemented
 */
VOID
NTAPI
RtlFreeRangeList(IN PRTL_RANGE_LIST RangeList)
{
    PLIST_ENTRY Entry;
    PRTL_RANGE_ENTRY Current;

    while (!IsListEmpty(&RangeList->ListHead))
    {
        Entry = RemoveHeadList(&RangeList->ListHead);
        Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);

        DPRINT ("Range start: %I64u\n", Current->Range.Start);
        DPRINT ("Range end:   %I64u\n", Current->Range.End);

        RtlpFreeMemory(Current, 0);
    }

    RangeList->Flags = 0;
    RangeList->Count = 0;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetFirstRange
 *
 * DESCRIPTION
 *	Retrieves the first range of a range list.
 *
 * ARGUMENTS
 *	RangeList	Pointer to the range list.
 *	Iterator	Pointer to a user supplied list state buffer.
 *	Range		Pointer to the first range.
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetFirstRange(IN PRTL_RANGE_LIST RangeList,
                 OUT PRTL_RANGE_LIST_ITERATOR Iterator,
                 OUT PRTL_RANGE *Range)
{
    Iterator->RangeListHead = &RangeList->ListHead;
    Iterator->MergedHead = NULL;
    Iterator->Stamp = RangeList->Stamp;

    if (IsListEmpty(&RangeList->ListHead))
    {
        Iterator->Current = NULL;
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    *Range = &CONTAINING_RECORD(RangeList->ListHead.Flink, RTL_RANGE_ENTRY, Entry)->Range;
    Iterator->Current = *Range;

    return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetLastRange
 *
 * DESCRIPTION
 *	Retrieves the last range of a range list.  Combine with
 *	RtlGetNextRange(..., MoveForwards = FALSE) to walk a list backwards.
 *
 * ARGUMENTS
 *	RangeList	Pointer to the range list.
 *	Iterator	Pointer to a user supplied list state buffer.
 *	Range		Pointer to the last range.
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetLastRange(IN PRTL_RANGE_LIST RangeList,
                OUT PRTL_RANGE_LIST_ITERATOR Iterator,
                OUT PRTL_RANGE *Range)
{
    Iterator->RangeListHead = &RangeList->ListHead;
    Iterator->MergedHead = NULL;
    Iterator->Stamp = RangeList->Stamp;

    if (IsListEmpty(&RangeList->ListHead))
    {
        Iterator->Current = NULL;
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    *Range = &CONTAINING_RECORD(RangeList->ListHead.Blink, RTL_RANGE_ENTRY, Entry)->Range;
    Iterator->Current = *Range;

    return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetNextRange
 *
 * DESCRIPTION
 *	Retrieves the next (or previous) range of a range list.
 *
 * ARGUMENTS
 *	Iterator	Pointer to a user supplied list state buffer.
 *	Range		Pointer to the first range.
 *	MoveForwards	TRUE, get next range
 *			FALSE, get previous range
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlGetNextRange(IN OUT PRTL_RANGE_LIST_ITERATOR Iterator,
                OUT PRTL_RANGE *Range,
                IN BOOLEAN MoveForwards)
{
    PRTL_RANGE_LIST RangeList;
    PRTL_RANGE_ENTRY Current;
    PLIST_ENTRY Next;

    RangeList = CONTAINING_RECORD(Iterator->RangeListHead, RTL_RANGE_LIST, ListHead);
    if (Iterator->Stamp != RangeList->Stamp)
        return STATUS_INVALID_PARAMETER;

    if (Iterator->Current == NULL)
    {
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    /* Iterator->Current points at the RTL_RANGE; recover its entry */
    Current = CONTAINING_RECORD(Iterator->Current, RTL_RANGE_ENTRY, Range);
    if (MoveForwards)
    {
        Next = Current->Entry.Flink;
    }
    else
    {
        Next = Current->Entry.Blink;
    }

    if (Next == Iterator->RangeListHead)
    {
        Iterator->Current = NULL;
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    *Range = &CONTAINING_RECORD(Next, RTL_RANGE_ENTRY, Entry)->Range;
    Iterator->Current = *Range;

    return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlInitializeRangeList
 *
 * DESCRIPTION
 *	Initializes a range list.
 *
 * ARGUMENTS
 *	RangeList	Pointer to a user supplied range list.
 *
 * RETURN VALUE
 *	None
 *
 * @implemented
 */
VOID
NTAPI
RtlInitializeRangeList(IN OUT PRTL_RANGE_LIST RangeList)
{
    InitializeListHead(&RangeList->ListHead);
    RangeList->Flags = 0;
    RangeList->Count = 0;
    RangeList->Stamp = 0;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlInvertRangeListEx
 *
 * DESCRIPTION
 *	Inverts a range list, tagging the newly created gap ranges with the
 *	supplied Attributes / UserData / Owner.
 *
 * ARGUMENTS
 *	InvertedRangeList	Inverted range list.
 *	RangeList		Range list.
 *	Attributes		Attributes for the created gap ranges.
 *	UserData		UserData for the created gap ranges.
 *	Owner			Owner for the created gap ranges.
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlInvertRangeListEx(OUT PRTL_RANGE_LIST InvertedRangeList,
                     IN PRTL_RANGE_LIST RangeList,
                     IN UCHAR Attributes,
                     IN PVOID UserData OPTIONAL,
                     IN PVOID Owner OPTIONAL)
{
    PRTL_RANGE_ENTRY Current;
    PLIST_ENTRY Entry;
    ULONGLONG GapStart;
    NTSTATUS Status;

    /*
     * The list is sorted by ascending Start, but RtlAddRange permits
     * overlapping entries (RTL_RANGE_LIST_ADD_IF_CONFLICT)
     *
     * Whenever two ranges overlap:
     * walk the covered address space upward from 0: grab every range that
     * covers the current position... so we can emit the gap up to the next range that
     * starts beyond it.
     */
    GapStart = (ULONGLONG)0;

    for (;;)
    {
        ULONGLONG NextStart;
        BOOLEAN Absorbed;
        BOOLEAN Found;

        /* Advance GapStart past every range that covers it (handles overlaps). */
        do
        {
            Absorbed = FALSE;
            Entry = RangeList->ListHead.Flink;
            while (Entry != &RangeList->ListHead)
            {
                Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);
                if (Current->Range.Start <= GapStart &&
                    Current->Range.End >= GapStart)
                {
                    /* Covered all the way to the top: no gap remains. */
                    if (Current->Range.End == (ULONGLONG)-1)
                        return STATUS_SUCCESS;

                    GapStart = Current->Range.End + 1;
                    Absorbed = TRUE;
                }
                Entry = Entry->Flink;
            }
        }
        while (Absorbed);

        /* GapStart is now uncovered; find the nearest range starting above it. */
        NextStart = (ULONGLONG)-1;
        Found = FALSE;
        Entry = RangeList->ListHead.Flink;
        while (Entry != &RangeList->ListHead)
        {
            Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);
            if (Current->Range.Start > GapStart &&
                Current->Range.Start <= NextStart)
            {
                NextStart = Current->Range.Start;
                Found = TRUE;
            }
            Entry = Entry->Flink;
        }

        /* No further ranges: the gap runs to the top of the address space. */
        if (!Found)
        {
            return RtlAddRange(InvertedRangeList,
                               GapStart,
                               (ULONGLONG)-1,
                               Attributes,
                               RTL_RANGE_LIST_ADD_IF_CONFLICT,
                               UserData,
                               Owner);
        }

        Status = RtlAddRange(InvertedRangeList,
                             GapStart,
                             NextStart - 1,
                             Attributes,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             UserData,
                             Owner);
        if (!NT_SUCCESS(Status))
            return Status;

        /* Resume from the next covered region. */
        GapStart = NextStart;
    }
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlInvertRangeList
 *
 * DESCRIPTION
 *	Inverts a range list.
 *
 * ARGUMENTS
 *	InvertedRangeList	Inverted range list.
 *	RangeList		Range list.
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlInvertRangeList(OUT PRTL_RANGE_LIST InvertedRangeList,
                   IN PRTL_RANGE_LIST RangeList)
{
    return RtlInvertRangeListEx(InvertedRangeList,
                                RangeList,
                                0,
                                NULL,
                                NULL);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlIsRangeAvailable
 *
 * DESCRIPTION
 *	Checks whether a range is available or not.
 *
 * ARGUMENTS
 *	RangeList		Pointer to the range list.
 *	Start
 *	End
 *	Flags
 *	AttributeAvailableMask
 *	Context
 *	Callback
 *	Available
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlIsRangeAvailable(IN PRTL_RANGE_LIST RangeList,
                    IN ULONGLONG Start,
                    IN ULONGLONG End,
                    IN ULONG Flags,
                    IN UCHAR AttributeAvailableMask,
                    IN PVOID Context OPTIONAL,
                    IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
                    OUT PBOOLEAN Available)
{
    if (Start > End)
        return STATUS_INVALID_PARAMETER;

    *Available = RtlpWindowIsAvailable(RangeList,
                                       Start,
                                       End,
                                       Flags,
                                       AttributeAvailableMask,
                                       Context,
                                       Callback,
                                       NULL);

    return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlMergeRangeList
 *
 * DESCRIPTION
 *	Merges two range lists.
 *
 * ARGUMENTS
 *	MergedRangeList	Resulting range list.
 *	RangeList1	First range list.
 *	RangeList2	Second range list
 *	Flags
 *
 * RETURN VALUE
 *	Status
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlMergeRangeLists(OUT PRTL_RANGE_LIST MergedRangeList,
                   IN PRTL_RANGE_LIST RangeList1,
                   IN PRTL_RANGE_LIST RangeList2,
                   IN ULONG Flags)
{
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;
    NTSTATUS Status;

    /* Copy range list 1 to the merged range list */
    Status = RtlCopyRangeList(MergedRangeList,
                              RangeList1);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Add range list 2 entries to the merged range list */
    Status = RtlGetFirstRange(RangeList2,
                              &Iterator,
                              &Range);
    if (!NT_SUCCESS(Status))
        return (Status == STATUS_NO_MORE_ENTRIES) ? STATUS_SUCCESS : Status;

    while (TRUE)
    {
        Status = RtlAddRange(MergedRangeList,
                             Range->Start,
                             Range->End,
                             Range->Attributes,
                             Range->Flags | Flags,
                             Range->UserData,
                             Range->Owner);
        if (!NT_SUCCESS(Status))
            break;

        Status = RtlGetNextRange(&Iterator,
                                 &Range,
                                 TRUE);
        if (!NT_SUCCESS(Status))
            break;
    }

    return (Status == STATUS_NO_MORE_ENTRIES) ? STATUS_SUCCESS : Status;
}

/* EOF */
