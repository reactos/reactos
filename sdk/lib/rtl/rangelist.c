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

/* GLOBALS ******************************************************************/

//extern PAGED_LOOKASIDE_LIST RtlpRangeListEntryLookasideList;

/* TYPES ********************************************************************/

/* RTLP_RANGE_LIST_ENTRY == RTL_RANGE + ListEntry */

/*  FIXME ? ntddk.h */
/* Flags for public functions: RtlFindRange(), RtlIsRangeAvailable() */
#define RTL_RANGE_LIST_SHARED_OK           1
#define RTL_RANGE_LIST_NULL_CONFLICT_OK    2
/* ? ntddk.h */

/* RTLP_RANGE_LIST_ENTRY.PrivateFlags */
#define RTLP_ENTRY_IS_MERGED  1


/* FUNCTIONS ***************************************************************/

static
PRTLP_RANGE_LIST_ENTRY
FASTCALL
RtlpEntryFromLink(PVOID Link)
{
    return CONTAINING_RECORD(Link, RTLP_RANGE_LIST_ENTRY, ListEntry);
}

static
BOOLEAN
FASTCALL
IsRangesIntersection(
    _In_ PRTLP_RANGE_LIST_ENTRY Entry1,
    _In_ PRTLP_RANGE_LIST_ENTRY Entry2)
{
    /* Not intersection when:
       (Entry1->Start - Entry1->End) ... (Entry2->Start -
       (Entry2->Start - Entry2->End) ... (Entry1->Start -
    */

    if (((Entry2->Start > Entry1->Start && Entry2->Start > Entry1->End) ||
         (Entry1->Start > Entry2->Start && Entry1->Start > Entry2->End)))
    {
        /* No intersection */
        return FALSE;
    }
    else
    {
        /* Entry1 and Entry2 overlap */
        return TRUE;
    }
}

VOID
NTAPI
RtlpDeleteRangeListEntry(
    _In_ PRTLP_RANGE_LIST_ENTRY DelEntry)
{
    PRTLP_RANGE_LIST_ENTRY RtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextRtlEntry;

    PAGED_CODE_RTL();
    DPRINT("RtlpDeleteRangeListEntry: DelEntry %p\n", DelEntry);

    if (!(DelEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED))
    {
        goto Finish;
    }

    RtlEntry = RtlpEntryFromLink(DelEntry->Merged.ListHead.Flink);
    NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);

    while (&RtlEntry->ListEntry != &DelEntry->Merged.ListHead)
    {
        DPRINT("RtlpDeleteRangeListEntry: Free RtlEntry %p, [%I64X-%I64X]\n", RtlEntry, RtlEntry->Start, RtlEntry->End);

        //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, RtlEntry);
        RtlpFreeMemory(RtlEntry, 'elRR');

        RtlEntry = NextRtlEntry;
        NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);
    }

Finish:

    DPRINT("RtlpDeleteRangeListEntry: Free DelEntry %p, [%I64X-%I64X]\n", DelEntry, DelEntry->Start, DelEntry->End);

    //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, DelEntry);
    RtlpFreeMemory(DelEntry, 'elRR');
}

PRTLP_RANGE_LIST_ENTRY
NTAPI
RtlpCopyRangeListEntry(
    _In_ PRTLP_RANGE_LIST_ENTRY RtlEntry)
{
    PRTLP_RANGE_LIST_ENTRY NewRtlEntry;
    PRTLP_RANGE_LIST_ENTRY MergedRtlEntry;
    PRTLP_RANGE_LIST_ENTRY NewMergedRtlEntry;

    PAGED_CODE_RTL();
    ASSERT (RtlEntry);

    //NewRtlEntry = ExAllocateFromPagedLookasideList(&RtlpRangeListEntryLookasideList);
    NewRtlEntry = RtlpAllocateMemory(sizeof(RTLP_RANGE_LIST_ENTRY), 'elRR');
    if (!NewRtlEntry)
    {
        DPRINT1("RtlpCopyRangeListEntry: Allocate failed\n");
        return NULL;
    }

    DPRINT("RtlpCopyRangeListEntry: (%p) ==> (%p) [%I64X-%I64X]\n", RtlEntry, NewRtlEntry, RtlEntry->Start, RtlEntry->End);
    RtlCopyMemory(NewRtlEntry, RtlEntry, sizeof(RTLP_RANGE_LIST_ENTRY));

    NewRtlEntry->ListEntry.Flink = NULL;
    NewRtlEntry->ListEntry.Blink = NULL;

    if (!(RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED))
    {
        //DPRINT("RtlpCopyRangeListEntry: !RTLP_ENTRY_IS_MERGED. return %p, \n", NewRtlEntry);
        return NewRtlEntry;
    }

    InitializeListHead(&NewRtlEntry->Merged.ListHead);

    DPRINT("RtlpCopyRangeListEntry: copy Merged %p\n", &NewRtlEntry->Merged.ListHead);

    for (MergedRtlEntry = RtlpEntryFromLink(RtlEntry->Merged.ListHead.Flink);
         &MergedRtlEntry->ListEntry != &RtlEntry->Merged.ListHead;
         MergedRtlEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink))
    {
        //NewMergedRtlEntry = ExAllocateFromPagedLookasideList(&RtlpRangeListEntryLookasideList);
        NewMergedRtlEntry = RtlpAllocateMemory(sizeof(RTLP_RANGE_LIST_ENTRY), 'elRR');
        if (!NewMergedRtlEntry)
        {
            DPRINT1("RtlpCopyRangeListEntry: Allocate failed\n");
            RtlpDeleteRangeListEntry(NewRtlEntry);
            return NULL;
        }

        DPRINT("RtlpCopyRangeListEntry: (%p) ==> (%p) [%I64X-%I64X]\n", MergedRtlEntry, NewMergedRtlEntry, MergedRtlEntry->Start, MergedRtlEntry->End);
        RtlCopyMemory(NewMergedRtlEntry, MergedRtlEntry, sizeof(RTLP_RANGE_LIST_ENTRY));

        InsertTailList(&NewRtlEntry->Merged.ListHead, &NewMergedRtlEntry->ListEntry);
    }

    return NewRtlEntry;
}

PRTLP_RANGE_LIST_ENTRY
NTAPI
RtlpCreateRangeListEntry(
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ UCHAR Attributes,
    _In_ PVOID UserData,
    _In_ PVOID Owner)
{
    PRTLP_RANGE_LIST_ENTRY RtlEntry;

    PAGED_CODE_RTL();
    ASSERT(Start <= End);

    //RtlEntry = ExAllocateFromPagedLookasideList(&RtlpRangeListEntryLookasideList);
    RtlEntry = RtlpAllocateMemory(sizeof(RTLP_RANGE_LIST_ENTRY), 'elRR');
    if (!RtlEntry)
    {
        DPRINT1("RtlpCreateRangeListEntry: RtlpAllocateMemory failed\n");
        return NULL;
    }

    DPRINT("RtlpCreateRangeListEntry: %p [%I64X-%I64X], %X, %p, %p\n", RtlEntry, Start, End, Attributes, UserData, Owner);

    RtlEntry->Start = Start;
    RtlEntry->End = End;

    RtlEntry->Allocated.UserData = UserData;
    RtlEntry->Allocated.Owner = Owner;

    RtlEntry->ListEntry.Flink = NULL;
    RtlEntry->ListEntry.Blink = NULL;

    RtlEntry->PublicFlags = 0;
    RtlEntry->PrivateFlags = 0;

    RtlEntry->Attributes = Attributes;

    return RtlEntry;
}

NTSTATUS
NTAPI
RtlpAddToMergedRange(
    _In_ PRTLP_RANGE_LIST_ENTRY RtlEntry,
    _In_ PRTLP_RANGE_LIST_ENTRY AddRtlEntry,
    _In_ UCHAR Flags)
{
    PRTLP_RANGE_LIST_ENTRY MergedRtlEntry;
    PLIST_ENTRY ListEntry = NULL;
    BOOLEAN RtlEntryIsShared;
    BOOLEAN AddRtlEntryIsShared;
    BOOLEAN MergedRtlEntryIsShared;

    PAGED_CODE_RTL();
    DPRINT("RtlpAddToMergedRange: %p [%I64X-%I64X], %p [%I64X-%I64X]\n", RtlEntry, RtlEntry->Start, RtlEntry->End, AddRtlEntry, AddRtlEntry->Start, AddRtlEntry->End);

    ASSERT(RtlEntry);
    ASSERT(AddRtlEntry);
    ASSERT((RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED));

    AddRtlEntryIsShared = AddRtlEntry->PublicFlags & RTL_RANGE_SHARED;

    for (MergedRtlEntry = RtlpEntryFromLink(RtlEntry->Merged.ListHead.Flink);
         &MergedRtlEntry->ListEntry != &RtlEntry->Merged.ListHead;
         MergedRtlEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink))
    {
        MergedRtlEntryIsShared = MergedRtlEntry->PublicFlags & RTL_RANGE_SHARED;

        if (IsRangesIntersection(MergedRtlEntry, AddRtlEntry) &&
            !(AddRtlEntryIsShared && MergedRtlEntryIsShared))
        {
            if ((Flags & RTL_RANGE_LIST_ADD_IF_CONFLICT) == 0)
            {
                DPRINT1("RtlpAddToMergedRange: STATUS_RANGE_LIST_CONFLICT\n");
                return STATUS_RANGE_LIST_CONFLICT;
            }

            MergedRtlEntry->PublicFlags |= RTL_RANGE_CONFLICT;
            AddRtlEntry->PublicFlags |= RTL_RANGE_CONFLICT;
        }

        if (ListEntry == NULL && (MergedRtlEntry->Start > AddRtlEntry->Start))
        {
            ListEntry = MergedRtlEntry->ListEntry.Blink;
        }
    }

    if (ListEntry)
    {
        AddRtlEntry->ListEntry.Flink = ListEntry->Flink;
        AddRtlEntry->ListEntry.Blink = ListEntry;
        ListEntry->Flink->Blink = &AddRtlEntry->ListEntry;
        ListEntry->Flink = &AddRtlEntry->ListEntry;
    }
    else
    {
        InsertTailList(&RtlEntry->Merged.ListHead, &AddRtlEntry->ListEntry);
    }

    if (AddRtlEntry->Start < RtlEntry->Start)
    {
        RtlEntry->Start = AddRtlEntry->Start;
    }

    if (AddRtlEntry->End > RtlEntry->End)
    {
        RtlEntry->End = AddRtlEntry->End;
    }

    RtlEntryIsShared = (RtlEntry->PublicFlags & RTL_RANGE_SHARED);

    if (RtlEntryIsShared && !AddRtlEntryIsShared)
    {
        DPRINT("RtlpAddToMergedRange: Merged range no longer completely shared\n");
        RtlEntry->PublicFlags &= ~RTL_RANGE_SHARED;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlpConvertToMergedRange(
    _In_ PRTLP_RANGE_LIST_ENTRY RtlEntry)
{
    PRTLP_RANGE_LIST_ENTRY MergedRtlEntry;

    PAGED_CODE_RTL();

    ASSERT(RtlEntry);
    ASSERT(!(RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED));
    ASSERT(!(RtlEntry->PublicFlags & RTL_RANGE_CONFLICT));

    MergedRtlEntry = RtlpCopyRangeListEntry(RtlEntry);
    if (!MergedRtlEntry)
    {
        DPRINT1("RtlpConvertToMergedRange: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("RtlpConvertToMergedRange: (%p) ==> (%p) [%I64X-%I64X]\n", RtlEntry, MergedRtlEntry, MergedRtlEntry->Start, MergedRtlEntry->End);

    RtlEntry->PrivateFlags = RTLP_ENTRY_IS_MERGED;
    ASSERT(RtlEntry->PublicFlags == RTL_RANGE_SHARED || RtlEntry->PublicFlags == 0);

    InitializeListHead(&RtlEntry->Merged.ListHead);
    InsertHeadList(&RtlEntry->Merged.ListHead, &MergedRtlEntry->ListEntry);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI 
RtlpAddIntersectingRanges(
    _In_ PLIST_ENTRY ListHead,
    _In_ PRTLP_RANGE_LIST_ENTRY RtlEntry,
    _In_ PRTLP_RANGE_LIST_ENTRY AddRtlEntry,
    _In_ ULONG Flags)
{
    PRTLP_RANGE_LIST_ENTRY CurrentRtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextRtlEntry;
    PRTLP_RANGE_LIST_ENTRY MergedRtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextMergedRtlEntry;
    NTSTATUS Status;
    BOOLEAN AddRtlEntryIsShared;
    BOOLEAN MergedRtlEntryIsShared;

    PAGED_CODE_RTL();
    DPRINT("RtlpAddIntersectingRanges: [%X], %p, %p [%I64X-%I64X], %p [%I64X-%I64X]\n", Flags, ListHead, RtlEntry, RtlEntry->Start, RtlEntry->End, AddRtlEntry, AddRtlEntry->Start, AddRtlEntry->End);

    ASSERT(RtlEntry);
    ASSERT(AddRtlEntry);

    AddRtlEntryIsShared = (AddRtlEntry->PublicFlags & RTL_RANGE_SHARED);

    if ((Flags & RTL_RANGE_LIST_ADD_IF_CONFLICT) == 0)
    {
        for(CurrentRtlEntry = RtlEntry;
            &CurrentRtlEntry->ListEntry != ListHead;
            CurrentRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->ListEntry.Flink))
        {
            if (AddRtlEntry->End < CurrentRtlEntry->Start) {
                break;
            }

            if (CurrentRtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED)
            {
                for (MergedRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->Merged.ListHead.Flink);
                     &MergedRtlEntry->ListEntry != &CurrentRtlEntry->Merged.ListHead;
                     MergedRtlEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink))
                {
                    MergedRtlEntryIsShared = (MergedRtlEntry->PublicFlags & RTL_RANGE_SHARED);

                    if (IsRangesIntersection(MergedRtlEntry, AddRtlEntry) &&
                        !(AddRtlEntryIsShared && MergedRtlEntryIsShared))
                    {
                        DPRINT1("RtlpAddIntersectingRanges: STATUS_RANGE_LIST_CONFLICT\n");
                        return STATUS_RANGE_LIST_CONFLICT;
                    }
                }
            }
            else if (!AddRtlEntryIsShared ||
                     !(CurrentRtlEntry->PublicFlags & RTL_RANGE_SHARED))
            {
                DPRINT1("RtlpAddIntersectingRanges: STATUS_RANGE_LIST_CONFLICT\n");
                return STATUS_RANGE_LIST_CONFLICT;
            }
        }
    }

    if (!(RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED))
    {
        Status = RtlpConvertToMergedRange(RtlEntry);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlpAddIntersectingRanges: Status %X\n", Status);
            return Status;
        }
    }

    ASSERT((RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED));

    CurrentRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);
    NextRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->ListEntry.Flink);

    while (&CurrentRtlEntry->ListEntry != ListHead)
    {
        if (AddRtlEntry->End < CurrentRtlEntry->Start)
        {
            break;
        }

        if (CurrentRtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED)
        {
            DPRINT("RtlpAddIntersectingRanges: PrivateFlags & RTLP_ENTRY_IS_MERGED\n");
            DbgBreakPoint();

            MergedRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->Merged.ListHead.Flink);
            NextMergedRtlEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink);

            while (&MergedRtlEntry->ListEntry != &CurrentRtlEntry->Merged.ListHead)
            {
                RemoveEntryList(&MergedRtlEntry->ListEntry);

                DPRINT("RtlpAddIntersectingRanges: MergedRtlEntry %p [%I64X-%I64X]\n", MergedRtlEntry, MergedRtlEntry->Start, MergedRtlEntry->End);

                Status = RtlpAddToMergedRange(RtlEntry, MergedRtlEntry, Flags);
                ASSERT(NT_SUCCESS(Status));

                MergedRtlEntry = NextMergedRtlEntry;
                NextMergedRtlEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink);
            }

            ASSERT(IsListEmpty(&CurrentRtlEntry->Merged.ListHead));

            RemoveEntryList(&CurrentRtlEntry->ListEntry);

            //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, CurrentRtlEntry);
            RtlpFreeMemory(CurrentRtlEntry, 'elRR');
        }
        else
        {
            RemoveEntryList(&CurrentRtlEntry->ListEntry);

            DPRINT("RtlpAddIntersectingRanges: CurrentRtlEntry %p [%I64X-%I64X]\n", CurrentRtlEntry, CurrentRtlEntry->Start, CurrentRtlEntry->End);

            Status = RtlpAddToMergedRange(RtlEntry, CurrentRtlEntry, Flags);
            ASSERT(NT_SUCCESS(Status));
        }

        CurrentRtlEntry = NextRtlEntry;
        NextRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->ListEntry.Flink);
    }

    DPRINT("RtlpAddIntersectingRanges: AddRtlEntry %p [%I64X-%I64X]\n", AddRtlEntry, AddRtlEntry->Start, AddRtlEntry->End);

    Status = RtlpAddToMergedRange(RtlEntry, AddRtlEntry, Flags);
    ASSERT(NT_SUCCESS(Status));

    return Status;
}

NTSTATUS
NTAPI 
RtlpAddRange(
    _In_ PLIST_ENTRY ListHead,
    _In_ PRTLP_RANGE_LIST_ENTRY AddRtlEntry,
    _In_ ULONG Flags)
{
    PRTLP_RANGE_LIST_ENTRY RtlEntry;
  
    PAGED_CODE_RTL();
    ASSERT(AddRtlEntry);

    DPRINT("RtlpAddRange: [%X] %p, %p [%I64X-%I64X]\n", Flags, ListHead, AddRtlEntry, AddRtlEntry->Start, AddRtlEntry->End);

    AddRtlEntry->PublicFlags &= ~RTL_RANGE_CONFLICT;

    for (RtlEntry = RtlpEntryFromLink(ListHead->Flink);
         &RtlEntry->ListEntry != ListHead;
         RtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink))
    {
        if (AddRtlEntry->End < RtlEntry->Start)
        {
            DPRINT("RtlpAddRange: Add before. Entry [%I64X-%I64X], Add [%I64X-%I64X]\n", RtlEntry->Start, RtlEntry->End, AddRtlEntry->Start, AddRtlEntry->End);
            InsertHeadList(RtlEntry->ListEntry.Blink, &AddRtlEntry->ListEntry);
            return STATUS_SUCCESS;
        }

        if (IsRangesIntersection(RtlEntry, AddRtlEntry))
        {
            DPRINT("RtlpAddRange: IntersectingRanges. Entry [%I64X-%I64X], AddEntry [%I64X-%I64X]\n", RtlEntry->Start, RtlEntry->End, AddRtlEntry->Start, AddRtlEntry->End);
            return RtlpAddIntersectingRanges(ListHead, RtlEntry, AddRtlEntry, Flags);
        }
    }

    DPRINT("RtlpAddRange: Add to end ranges\n");
    InsertTailList(ListHead, &AddRtlEntry->ListEntry);

    return STATUS_SUCCESS;
}

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
 * TODO:
 *   - Support shared ranges.
 *
 * @implemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddRange(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ UCHAR Attributes,
    _In_ ULONG Flags,
    _In_opt_ PVOID UserData,
    _In_opt_ PVOID Owner)
{
    PRTLP_RANGE_LIST_ENTRY AddRtlEntry;
    NTSTATUS Status;

    PAGED_CODE_RTL();
    DPRINT("RtlAddRange: [%X] %p [%I64X-%I64X], %X, %X, %p, %p\n", Flags, RangeList, Start, End, RangeList->Count, Attributes, UserData, Owner);

    if (End < Start)
    {
        DPRINT1("RtlAddRange: STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    AddRtlEntry = RtlpCreateRangeListEntry(Start, End, Attributes, UserData, Owner);
    if (!AddRtlEntry)
    {
        DPRINT1("RtlAddRange: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Flags & RTL_RANGE_LIST_ADD_SHARED)
    {
        AddRtlEntry->PublicFlags |= RTL_RANGE_SHARED;
    }

    Status = RtlpAddRange(&RangeList->ListHead, AddRtlEntry, Flags);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAddRange: Status %X\n", Status);
        ASSERT(FALSE);
        //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, AddRtlEntry);
        RtlpFreeMemory(AddRtlEntry, 'elRR');
        return Status;
    }

    RangeList->Count++;
    RangeList->Stamp++;

    return Status;
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
NTSYSAPI
NTSTATUS
NTAPI
RtlCopyRangeList(
    _Out_ PRTL_RANGE_LIST CopyRangeList,
    _In_ PRTL_RANGE_LIST RangeList)
{
    PRTLP_RANGE_LIST_ENTRY RtlEntry;
    PRTLP_RANGE_LIST_ENTRY NewRtlEntry;

    PAGED_CODE_RTL();
    ASSERT(RangeList);
    ASSERT(CopyRangeList);

    DPRINT("RtlCopyRangeList: (%p) ==> (%p) [%X]\n", RangeList, CopyRangeList, RangeList->Count);

    if (CopyRangeList->Count)
    {
        DPRINT1("RtlCopyRangeList: STATUS_INVALID_PARAMETER. Count %X\n", CopyRangeList->Count);
        return STATUS_INVALID_PARAMETER;
    }

    CopyRangeList->Flags = RangeList->Flags;
    CopyRangeList->Count = RangeList->Count;
    CopyRangeList->Stamp = RangeList->Stamp;

    for (RtlEntry = RtlpEntryFromLink(RangeList->ListHead.Flink);
         &RtlEntry->ListEntry!= &RangeList->ListHead;
         RtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink))
    {
        DPRINT("RtlCopyRangeList: %p [%I64X-%I64X], %X, %X, %X\n", RtlEntry, RtlEntry->Start, RtlEntry->End, RtlEntry->Attributes, RtlEntry->PublicFlags, RtlEntry->PrivateFlags);

        NewRtlEntry = RtlpCopyRangeListEntry(RtlEntry);
        if (!NewRtlEntry)
        {
            DPRINT1("RtlCopyRangeList: STATUS_INSUFFICIENT_RESOURCES\n");
            RtlFreeRangeList(CopyRangeList);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        InsertTailList(&CopyRangeList->ListHead, &NewRtlEntry->ListEntry);
    }

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
RtlDeleteOwnersRanges(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ PVOID Owner)
{
    PRTLP_RANGE_LIST_ENTRY RangeListEntry;
    PRTLP_RANGE_LIST_ENTRY entry;
    PRTLP_RANGE_LIST_ENTRY MergedListEntry;
    PRTLP_RANGE_LIST_ENTRY mergedentry;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE_RTL();
    ASSERT(RangeList);

    DPRINT("RtlDeleteOwnersRanges: RangeList - %p, Owner - %p, RangeList->Count - %X\n",
           RangeList, Owner, RangeList->Count);

    for (RangeListEntry = CONTAINING_RECORD(RangeList->ListHead.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry),
                  entry = CONTAINING_RECORD(RangeListEntry->ListEntry.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry);
         &RangeListEntry->ListEntry != &RangeList->ListHead;
         RangeListEntry = entry,
                  entry = CONTAINING_RECORD(RangeListEntry->ListEntry.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry))
    {
        if (RangeListEntry->PrivateFlags & RTLP_RANGE_LIST_ENTRY_MERGED)
        {
            for (MergedListEntry = CONTAINING_RECORD(&RangeListEntry->Merged.ListHead.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry),
                     mergedentry = CONTAINING_RECORD(MergedListEntry->ListEntry.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry);
                 &MergedListEntry->ListEntry != &RangeListEntry->Merged.ListHead;
                 MergedListEntry = mergedentry,
                     mergedentry = CONTAINING_RECORD(MergedListEntry->ListEntry.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry))
            {
                if (MergedListEntry->Allocated.Owner == Owner)
                {
                    ASSERT(FALSE);
                }
            }
        }
        else if (RangeListEntry->Allocated.Owner == Owner)
        {
            DPRINT("RtlDeleteOwnersRanges: Deleting range (Start - %I64X, End - %I64X)\n",
                   RangeListEntry->Start, RangeListEntry->End);

            RemoveEntryList(&RangeListEntry->ListEntry);
            //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, RangeListEntry);
            RtlpFreeMemory(RangeListEntry, 'elRR');

            RangeList->Count--;
            RangeList->Stamp++;

            Status = STATUS_SUCCESS;
        }
    }

    return Status;
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
 *	Searches for an unused range.
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
 * TODO
 *	Support shared ranges and callback.
 *
 * @implemented
 */
NTSTATUS
NTAPI
RtlFindRange(IN PRTL_RANGE_LIST RangeList,
             IN ULONGLONG Minimum,
             IN ULONGLONG Maximum,
             IN ULONG Length,
             IN ULONG Alignment,
             IN ULONG Flags,
             IN UCHAR AttributeAvailableMask,
             IN PVOID Context OPTIONAL,
             IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
             OUT PULONGLONG Start)
{
    PRTL_RANGE_ENTRY CurrentEntry;
    PRTL_RANGE_ENTRY NextEntry;
    PLIST_ENTRY Entry;
    ULONGLONG RangeMin;
    ULONGLONG RangeMax;

    if (Alignment == 0 || Length == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (IsListEmpty(&RangeList->ListHead))
    {
        *Start = ROUND_DOWN(Maximum - (Length - 1), Alignment);
        return STATUS_SUCCESS;
    }

    NextEntry = NULL;
    Entry = RangeList->ListHead.Blink;
    while (Entry != &RangeList->ListHead)
    {
        CurrentEntry = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);

        RangeMax = NextEntry ? (NextEntry->Range.Start - 1) : Maximum;
        if (RangeMax + (Length - 1) < Minimum)
        {
            return STATUS_RANGE_NOT_FOUND;
        }

        RangeMin = ROUND_DOWN(RangeMax - (Length - 1), Alignment);
        if (RangeMin < Minimum ||
            (RangeMax - RangeMin) < (Length - 1))
        {
            return STATUS_RANGE_NOT_FOUND;
        }

        DPRINT("RangeMax: %I64x\n", RangeMax);
        DPRINT("RangeMin: %I64x\n", RangeMin);

        if (RangeMin > CurrentEntry->Range.End)
        {
            *Start = RangeMin;
            return STATUS_SUCCESS;
        }

        NextEntry = CurrentEntry;
        Entry = Entry->Blink;
    }

    RangeMax = NextEntry ? (NextEntry->Range.Start - 1) : Maximum;
    if (RangeMax + (Length - 1) < Minimum)
    {
        return STATUS_RANGE_NOT_FOUND;
    }

    RangeMin = ROUND_DOWN(RangeMax - (Length - 1), Alignment);
    if (RangeMin < Minimum ||
        (RangeMax - RangeMin) < (Length - 1))
    {
        return STATUS_RANGE_NOT_FOUND;
    }

    DPRINT("RangeMax: %I64x\n", RangeMax);
    DPRINT("RangeMin: %I64x\n", RangeMin);

    *Start = RangeMin;

    return STATUS_SUCCESS;
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
NTSYSAPI
VOID
NTAPI
RtlFreeRangeList(
    _In_ PRTL_RANGE_LIST RangeList)
{
    PRTLP_RANGE_LIST_ENTRY entry;
    PRTLP_RANGE_LIST_ENTRY RangeListEntry;

    PAGED_CODE_RTL();
    ASSERT(RangeList);

    DPRINT("RtlFreeRangeList: RangeList - %p, RangeList->Count - %X\n",
           RangeList, RangeList->Count);

    RangeList->Flags = 0;
    RangeList->Count = 0;

    for (RangeListEntry = CONTAINING_RECORD(RangeList->ListHead.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry),
                  entry = CONTAINING_RECORD(RangeListEntry->ListEntry.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry);
         &RangeListEntry->ListEntry != &RangeList->ListHead;
         RangeListEntry = entry,
                  entry = CONTAINING_RECORD(RangeListEntry->ListEntry.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry))
    {
        RemoveEntryList(&RangeListEntry->ListEntry);
        RtlpDeleteRangeListEntry(RangeListEntry);
    }
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

    Iterator->Current = RangeList->ListHead.Flink;
    *Range = &((PRTL_RANGE_ENTRY)Iterator->Current)->Range;

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
    PLIST_ENTRY Next;

    RangeList = CONTAINING_RECORD(Iterator->RangeListHead, RTL_RANGE_LIST, ListHead);
    if (Iterator->Stamp != RangeList->Stamp)
        return STATUS_INVALID_PARAMETER;

    if (Iterator->Current == NULL)
    {
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    if (MoveForwards)
    {
        Next = ((PRTL_RANGE_ENTRY)Iterator->Current)->Entry.Flink;
    }
    else
    {
        Next = ((PRTL_RANGE_ENTRY)Iterator->Current)->Entry.Blink;
    }

    if (Next == Iterator->RangeListHead)
    {
        Iterator->Current = NULL;
        *Range = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    Iterator->Current = Next;
    *Range = &((PRTL_RANGE_ENTRY)Next)->Range;

    return STATUS_SUCCESS;
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
 * TODO:
 *   - honor Flags and AttributeAvailableMask.
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
    PRTL_RANGE_ENTRY Current;
    PLIST_ENTRY Entry;

    *Available = TRUE;

    Entry = RangeList->ListHead.Flink;
    while (Entry != &RangeList->ListHead)
    {
        Current = CONTAINING_RECORD (Entry, RTL_RANGE_ENTRY, Entry);

        if (!((Current->Range.Start >= End && Current->Range.End > End) ||
              (Current->Range.Start <= Start && Current->Range.End < Start &&
               (!(Flags & RTL_RANGE_SHARED) ||
                !(Current->Range.Flags & RTL_RANGE_SHARED)))))
        {
            if (Callback != NULL)
            {
                *Available = Callback(Context,
                                      &Current->Range);
            }
            else
            {
                *Available = FALSE;
            }
        }

        Entry = Entry->Flink;
    }

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
NTSYSAPI
VOID
NTAPI
RtlInitializeRangeList(
    _Inout_ PRTL_RANGE_LIST RangeList)
{
    PAGED_CODE_RTL();
    DPRINT("RtlInitializeRangeList: RangeList %p\n", RangeList);

    ASSERT(RangeList);

    InitializeListHead(&RangeList->ListHead);
    RangeList->Flags = 0;
    RangeList->Count = 0;
    RangeList->Stamp = 0;
}

// !!! FIXME ==> PRTLP_RANGE_LIST_ENTRY

typedef struct _RTL_RANGE_ENTRY {
    RTL_RANGE   Range;
    USHORT      PrivateFlags;
    LIST_ENTRY  Entry;
    UCHAR       _PADDING0_[0x4];
} RTL_RANGE_ENTRY, *PRTL_RANGE_ENTRY;

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
RtlInvertRangeList(
    _Out_ PRTL_RANGE_LIST InvertedRangeList,
    _In_ PRTL_RANGE_LIST RangeList)
{
    PRTL_RANGE_ENTRY Previous;
    PRTL_RANGE_ENTRY Current;
    PLIST_ENTRY Entry;
    NTSTATUS Status;

    DPRINT("RtlInvertRangeList: InvertedRangeList %p, RangeList %p\n", InvertedRangeList, RangeList);

// FIXME None Reworked !
ASSERT(FALSE);

    /* Add leading and intermediate ranges */
    Previous = NULL;
    Entry = RangeList->ListHead.Flink;
    while (Entry != &RangeList->ListHead)
    {
        Current = CONTAINING_RECORD(Entry, RTL_RANGE_ENTRY, Entry);

        if (Previous == NULL)
        {
            if (Current->Range.Start != (ULONGLONG)0)
            {
                Status = RtlAddRange(InvertedRangeList,
                                     (ULONGLONG)0,
                                     Current->Range.Start - 1,
                                     0,
                                     0,
                                     NULL,
                                     NULL);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
        }
        else
        {
            if (Previous->Range.End + 1 != Current->Range.Start)
            {
                Status = RtlAddRange(InvertedRangeList,
                                     Previous->Range.End + 1,
                                     Current->Range.Start - 1,
                                     0,
                                     0,
                                     NULL,
                                     NULL);
                if (!NT_SUCCESS(Status))
                    return Status;
            }
        }

        Previous = Current;
        Entry = Entry->Flink;
    }

    /* Check if the list was empty */
    if (Previous == NULL)
    {
        /* We're done */
        return STATUS_SUCCESS;
    }

    /* Add trailing range */
    if (Previous->Range.End + 1 != (ULONGLONG)-1)
    {
        Status = RtlAddRange(InvertedRangeList,
                             Previous->Range.End + 1,
                             (ULONGLONG)-1,
                             0,
                             0,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    return STATUS_SUCCESS;
}

// !!! FIXME

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
RtlMergeRangeLists(
    _Out_ PRTL_RANGE_LIST MergedRangeList,
    _In_ PRTL_RANGE_LIST RangeList1,
    _In_ PRTL_RANGE_LIST RangeList2,
    _In_ ULONG Flags)
{
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE RtlRange;
    NTSTATUS Status;

    DPRINT("RtlMergeRangeLists: RangeList1 %p, RangeList2 %p\n", RangeList1, RangeList2);

// FIXME Nonereworked !
ASSERT(FALSE);

    /* Copy range list 1 to the merged range list */
    Status = RtlCopyRangeList(MergedRangeList, RangeList1);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Add range list 2 entries to the merged range list */
    Status = RtlGetFirstRange(RangeList2, &Iterator, &RtlRange);
    if (!NT_SUCCESS(Status))
        return (Status == STATUS_NO_MORE_ENTRIES) ? STATUS_SUCCESS : Status;

    while (TRUE)
    {
        Status = RtlAddRange(MergedRangeList,
                             RtlRange->Start,
                             RtlRange->End,
                             RtlRange->Attributes,
                             RtlRange->Flags | Flags,
                             RtlRange->UserData,
                             RtlRange->Owner);
        if (!NT_SUCCESS(Status))
            break;

        Status = RtlGetNextRange(&Iterator, &RtlRange, TRUE);
        if (!NT_SUCCESS(Status))
            break;
    }

    return (Status == STATUS_NO_MORE_ENTRIES) ? STATUS_SUCCESS : Status;
}

/* EOF */
