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
RtlpDeleteFromMergedRange(
    _In_ PRTLP_RANGE_LIST_ENTRY RtlEntry,
    _In_ PRTLP_RANGE_LIST_ENTRY MergedRtlEntry)
{
    PRTLP_RANGE_LIST_ENTRY CurrentRtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextRtlEntry;
    LIST_ENTRY TmpList;
    NTSTATUS Status;

    PAGED_CODE_RTL();
    DPRINT("Deleting merged range %p [%I64X-%I64X] from %p\n", MergedRtlEntry, MergedRtlEntry->Start, MergedRtlEntry->End, RtlEntry);

    ASSERT((RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED));

    InitializeListHead(&TmpList);
    RemoveEntryList(&MergedRtlEntry->ListEntry);

    CurrentRtlEntry = RtlpEntryFromLink(RtlEntry->Merged.ListHead.Flink);
    NextRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->ListEntry.Flink);

    while (&CurrentRtlEntry->ListEntry != &RtlEntry->Merged.ListHead)
    {
        RemoveEntryList(&CurrentRtlEntry->ListEntry);
        CurrentRtlEntry->PublicFlags &= ~RTL_RANGE_CONFLICT;

        DPRINT("RtlpDeleteFromMergedRange: Add %p to TmpList\n", CurrentRtlEntry);

        Status = RtlpAddRange(&TmpList, CurrentRtlEntry, RTL_RANGE_LIST_ADD_IF_CONFLICT);

        if (!NT_SUCCESS(Status))
        {
            if (Status != STATUS_INSUFFICIENT_RESOURCES)
            {
                DPRINT1("RtlpDeleteFromMergedRange: Status %X\n", Status);
                ASSERT(Status == STATUS_INSUFFICIENT_RESOURCES);
            }
            else
            {
                DPRINT1("RtlpDeleteFromMergedRange: STATUS_INSUFFICIENT_RESOURCES\n");
            }

            CurrentRtlEntry = RtlpEntryFromLink(TmpList.Flink);
            NextRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->ListEntry.Flink);

            while (&TmpList != &CurrentRtlEntry->ListEntry)
            {
                Status = RtlpAddToMergedRange(RtlEntry, CurrentRtlEntry, RTL_RANGE_LIST_ADD_IF_CONFLICT);
                ASSERT(NT_SUCCESS(Status));

                CurrentRtlEntry = NextRtlEntry;
                NextRtlEntry = RtlpEntryFromLink(CurrentRtlEntry->ListEntry.Flink);
            }

            return RtlpAddToMergedRange(RtlEntry, MergedRtlEntry, RTL_RANGE_LIST_ADD_IF_CONFLICT);
        }

        CurrentRtlEntry = NextRtlEntry;
        NextRtlEntry = RtlpEntryFromLink(NextRtlEntry->ListEntry.Flink);
    }

    if (IsListEmpty(&TmpList))
    {
        DPRINT("RtlpDeleteFromMergedRange: IsListEmpty(&TmpList)\n");
        RemoveEntryList(&RtlEntry->ListEntry);
    }
    else
    {
        PLIST_ENTRY Flink;
        PLIST_ENTRY Blink;

        DPRINT("RtlpDeleteFromMergedRange: Add entry from TmpList\n");

        Blink = RtlEntry->ListEntry.Blink;
        Flink = RtlEntry->ListEntry.Flink;

        Blink->Flink = TmpList.Flink;
        TmpList.Flink->Blink = Blink;

        Flink->Blink = TmpList.Blink;
        TmpList.Blink->Flink = Flink;
    }

    //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, MergedRtlEntry);
    RtlpFreeMemory(MergedRtlEntry, 'elRR');
    //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, RtlEntry);
    RtlpFreeMemory(RtlEntry, 'elRR');

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlDeleteOwnersRanges(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ PVOID Owner)
{
    PRTLP_RANGE_LIST_ENTRY RtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextRtlEntry;
    PRTLP_RANGE_LIST_ENTRY MergedRtlEntry;
    PRTLP_RANGE_LIST_ENTRY MergedNextEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE_RTL();
    ASSERT(RangeList);

    DPRINT("RtlDeleteOwnersRanges: RangeList %p, Owner %p, [%X]\n", RangeList, Owner, RangeList->Count);

START:

    RtlEntry = RtlpEntryFromLink(RangeList->ListHead.Flink);
    NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);

    while (&RtlEntry->ListEntry != &RangeList->ListHead)
    {
        if (RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED)
        {
            MergedRtlEntry = RtlpEntryFromLink(RtlEntry->Merged.ListHead.Flink);
            MergedNextEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink);

            while (&MergedRtlEntry->ListEntry != &RtlEntry->Merged.ListHead)
            {
                if (MergedRtlEntry->Allocated.Owner == Owner)
                {
                    DPRINT("RtlDeleteOwnersRanges: Deleting merged range [%I64X-%I64X]\n", MergedRtlEntry->Start, MergedRtlEntry->End);

                    Status = RtlpDeleteFromMergedRange(RtlEntry, MergedRtlEntry);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT("RtlDeleteOwnersRanges: Status %X\n", Status);
                        return Status;
                    }

                    RangeList->Count--;
                    RangeList->Stamp++;

                    goto START;
                }

                MergedRtlEntry = MergedNextEntry;
                MergedNextEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink);
            }
        }
        else if (RtlEntry->Allocated.Owner == Owner)
        {
            DPRINT("RtlDeleteOwnersRanges: Deleting range [%I64X-%I64X]\n", RtlEntry->Start, RtlEntry->End);

            RemoveEntryList(&RtlEntry->ListEntry);
            //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, RtlEntry);
            RtlpFreeMemory(RtlEntry, 'elRR');

            RangeList->Count--;
            RangeList->Stamp++;

            Status = STATUS_SUCCESS;
        }

        RtlEntry = NextRtlEntry;
        NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);
    }

    DPRINT("RtlDeleteOwnersRanges: return Status %X\n", Status);

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
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRange(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ PVOID Owner)
{
    PRTLP_RANGE_LIST_ENTRY RtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextRtlEntry;
    PRTLP_RANGE_LIST_ENTRY MergedRtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextMergedRtlEntry;
    NTSTATUS Status = STATUS_RANGE_NOT_FOUND;

    PAGED_CODE_RTL();
    DPRINT1("RtlDeleteRange: %p [%I64X-%I64X] %p\n", RangeList, Start, End, Owner);
    ASSERT(RangeList);

    RtlEntry = RtlpEntryFromLink(RangeList->ListHead.Flink);
    NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);

    while (&RangeList->ListHead != &RtlEntry->ListEntry && End >= RtlEntry->Start)
    {
        DPRINT("RtlDeleteRange: %p [%I64X-%I64X] %p\n", RtlEntry, RtlEntry->Start, RtlEntry->End, Owner);

        if (RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED)
        {
            if (Start >= RtlEntry->Start && End <= RtlEntry->End)
            {
                MergedRtlEntry = RtlpEntryFromLink(RtlEntry->Merged.ListHead.Flink);
                NextMergedRtlEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink);

                while (&RtlEntry->Merged.ListHead != &MergedRtlEntry->ListEntry)
                {
                    if (MergedRtlEntry->Start == Start &&
                        MergedRtlEntry->End == End &&
                        MergedRtlEntry->Allocated.Owner == Owner)
                    {
                        Status = RtlpDeleteFromMergedRange(MergedRtlEntry, RtlEntry);
                        goto Exit;
                    }

                    MergedRtlEntry = NextMergedRtlEntry;
                    NextMergedRtlEntry = RtlpEntryFromLink(MergedRtlEntry->ListEntry.Flink);
                }
            }
        }
        else if (RtlEntry->Start == Start &&
                 RtlEntry->End == End &&
                 RtlEntry->Allocated.Owner == Owner)
        {
            RemoveEntryList(&RtlEntry->ListEntry);

            DPRINT("RtlDeleteRange: Free %p [%I64X-%I64X] %p\n", RtlEntry, RtlEntry->Start, RtlEntry->End, Owner);
            //ExFreeToPagedLookasideList(&RtlpRangeListEntryLookasideList, RtlEntry);
            RtlpFreeMemory(RtlEntry, 'elRR');

            Status = STATUS_SUCCESS;
            break;
        }

        RtlEntry = NextRtlEntry;
        NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);
    }

Exit:

    if (NT_SUCCESS(Status))
    {
        RangeList->Count++;
        RangeList->Stamp++;
    }

    DPRINT("RtlDeleteRange: return Status %X\n", Status);
    return Status;
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
NTSYSAPI
NTSTATUS
NTAPI
RtlGetNextRange(
    _Inout_ PRTL_RANGE_LIST_ITERATOR Iterator,
    _Out_ PRTL_RANGE * OutRange,
    _In_ BOOLEAN MoveForwards)
{
    PRTLP_RANGE_LIST_ENTRY CurrentRtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextRtlEntry;
    PRTL_RANGE_LIST RangeList;
    PLIST_ENTRY ListEntry;

    PAGED_CODE_RTL();
    //DPRINT("RtlGetNextRange: Iterator %p, RangeListHead %p, Current %p, MergedHead %p, Stamp %X, MoveForwards %X\n", Iterator, Iterator->RangeListHead, Iterator->Current, Iterator->MergedHead, Iterator->Stamp, MoveForwards);

    RangeList = CONTAINING_RECORD((Iterator->RangeListHead), RTL_RANGE_LIST, ListHead);

    if (RangeList->Stamp != Iterator->Stamp)
    {
        DPRINT1("RtlGetNextRange: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    if (!Iterator->Current)
    {
        DPRINT1("RtlGetNextRange: return STATUS_NO_MORE_ENTRIES\n");
        *OutRange = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    CurrentRtlEntry = (PRTLP_RANGE_LIST_ENTRY)Iterator->Current;

    if (MoveForwards)
    {
       ListEntry = CurrentRtlEntry->ListEntry.Flink;
    }
    else
    {
       ListEntry = CurrentRtlEntry->ListEntry.Blink;
    }

    NextRtlEntry = RtlpEntryFromLink(ListEntry);
    ASSERT(NextRtlEntry);

    if (Iterator->MergedHead)
    {
        PRTLP_RANGE_LIST_ENTRY MergedRtlEntry;

        if (&NextRtlEntry->ListEntry != Iterator->MergedHead)
        {
            Iterator->Current = NextRtlEntry;
            *OutRange = (PRTL_RANGE)NextRtlEntry;
            DPRINT("RtlGetNextRange: %p, %p, %p, *OutRange %p [%I64X-%I64X]\n", Iterator->RangeListHead, Iterator->Current, Iterator->MergedHead, *OutRange, NextRtlEntry->Start, NextRtlEntry->End);
            return STATUS_SUCCESS;
        }

        MergedRtlEntry = CONTAINING_RECORD(Iterator->MergedHead, RTLP_RANGE_LIST_ENTRY, Merged.ListHead);

        if (MoveForwards)
        {
            ListEntry = MergedRtlEntry->ListEntry.Flink;
        }
        else
        {
            ListEntry = MergedRtlEntry->ListEntry.Blink;
        }

        NextRtlEntry = RtlpEntryFromLink(ListEntry);
        DPRINT("RtlGetNextRange: %p, %p, %p [%I64X-%I64X]\n", MergedRtlEntry, ListEntry, NextRtlEntry, NextRtlEntry->Start, NextRtlEntry->End);

        Iterator->MergedHead = NULL;
    }

    if (&NextRtlEntry->ListEntry == Iterator->RangeListHead)
    {
        DPRINT("RtlGetNextRange: return STATUS_NO_MORE_ENTRIES\n");
        Iterator->Current = NULL;
        *OutRange = NULL;
        return STATUS_NO_MORE_ENTRIES;
    }

    if (NextRtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED)
    {
        ASSERT(!Iterator->MergedHead);
        Iterator->MergedHead = &NextRtlEntry->Merged.ListHead;

        if (MoveForwards)
        {
            ListEntry = NextRtlEntry->Merged.ListHead.Flink;
        }
        else
        {
            ListEntry = NextRtlEntry->Merged.ListHead.Blink;
        }
    }
    else
    {
        ListEntry = &NextRtlEntry->ListEntry;
    }

    NextRtlEntry = RtlpEntryFromLink(ListEntry);
    DPRINT("RtlGetNextRange: %p, %p, *OutRange %X [%I64X-%I64X]\n", Iterator->Current, Iterator->MergedHead, NextRtlEntry, NextRtlEntry->Start, NextRtlEntry->End);

    Iterator->Current = NextRtlEntry;
    *OutRange = (PRTL_RANGE)NextRtlEntry;

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
RtlpIsRangeAvailable(
    _Inout_ PRTL_RANGE_LIST_ITERATOR Iterator,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ UCHAR AttributeAvailableMask,
    _In_ BOOLEAN Flag1,
    _In_ BOOLEAN Flag2,
    _In_ BOOLEAN MoveForwards,
    _In_ PVOID Context,
    _In_ PRTL_CONFLICT_RANGE_CALLBACK Callback)
{
    PRTL_RANGE RtlRange;

    PAGED_CODE_RTL();
    DPRINT("RtlpIsRangeAvailable: %p [%I64X-%I64X], %X, %X, Forwards %X, %p\n", Iterator, Start, End, Flag1, Flag2, MoveForwards, Callback);

    ASSERT(Iterator);

    for (RtlRange = Iterator->Current;
         RtlRange;
         RtlGetNextRange(Iterator, &RtlRange, MoveForwards))
    {
        if (MoveForwards)
        {
            if (!Iterator->MergedHead && End < RtlRange->Start)
            {
                return TRUE;
            }
        }
        else if (!Iterator->MergedHead && Start > RtlRange->End)
        {
            return TRUE;
        }

        if ((RtlRange->Start > Start && RtlRange->Start > End) ||
            (RtlRange->Start < Start && RtlRange->End < Start))
        {
            continue;
        }

        DPRINT("RtlpIsRangeAvailable: Intersection [%I64X-%I64X] and [%I64X-%I64X]\n", Start, End, RtlRange->Start, RtlRange->End);

        if (Flag1 && (RtlRange->Flags & 1)) // FIXME
        {
            continue;
        }

        if ((AttributeAvailableMask & RtlRange->Attributes))
        {
            continue;
        }

        if (Flag2 == 0)
        {
            if (!Callback)
            {
                return FALSE;
            }

            if (Callback(Context, NULL) == FALSE)
            {
                return FALSE;
            }

            DPRINT("RtlpIsRangeAvailable: conflict [%I64X-%I64X] and [%I64X-%I64X]\n", Start, End, RtlRange->Start, RtlRange->End);
        }
        else if (RtlRange->Owner)
        {
            if (!Callback)
            {
                return FALSE;
            }

            if (Callback(Context, RtlRange) == FALSE)
            {
                return FALSE;
            }

            DPRINT("RtlpIsRangeAvailable: conflict [%I64X-%I64X] and [%I64X-%I64X]\n", Start, End, RtlRange->Start, RtlRange->End);
        }
        else
        {
            continue;
        }

        return FALSE;
    }

    return TRUE;
}

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLastRange(
    _In_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE_LIST_ITERATOR Iterator,
    _Out_ PRTL_RANGE * OutRange)
{
    PRTLP_RANGE_LIST_ENTRY RtlEntry;

    PAGED_CODE_RTL();
    DPRINT("RtlGetLastRange: RangeList %p, Iterator %p\n", RangeList, Iterator);

    Iterator->RangeListHead = &RangeList->ListHead;
    Iterator->Stamp = RangeList->Stamp;

    if (IsListEmpty(&RangeList->ListHead))
    {
        DPRINT("RtlGetLastRange: return STATUS_NO_MORE_ENTRIES\n");

        Iterator->Current = NULL;
        Iterator->MergedHead = NULL;
        *OutRange = NULL;

        return STATUS_NO_MORE_ENTRIES;
    }

    RtlEntry = RtlpEntryFromLink(RangeList->ListHead.Blink);
    //DPRINT("RtlGetLastRange: Iterator->RangeListHead %p, Iterator->Stamp %X, RtlEntry %p\n", Iterator->RangeListHead, Iterator->Stamp, RtlEntry);

    if (RtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED)
    {
        ASSERT(!IsListEmpty(&RtlEntry->Merged.ListHead));

        Iterator->MergedHead = &RtlEntry->Merged.ListHead;
        Iterator->Current = RtlpEntryFromLink(RtlEntry->Merged.ListHead.Blink);

        DPRINT("RtlGetLastRange: MergedHead %X, Current %p\n", Iterator->MergedHead, Iterator->Current);
    }
    else
    {
        Iterator->MergedHead = NULL;
        Iterator->Current = RtlEntry;
        //DPRINT("RtlGetLastRange: Iterator->MergedHead %p, Iterator->Current %p\n", Iterator->MergedHead, Iterator->Current);
    }

    *OutRange = (PRTL_RANGE)Iterator->Current;

    DPRINT("RtlGetLastRange: *OutRange %p, [%I64X-%I64X]\n", Iterator->Current, (*OutRange)->Start, (*OutRange)->End);

    return STATUS_SUCCESS;
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
NTSYSAPI
NTSTATUS
NTAPI
RtlFindRange(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Minimum,
    _In_ ULONGLONG Maximum,
    _In_ ULONG Length,
    _In_ ULONG Alignment,
    _In_ ULONG Flags,
    _In_ UCHAR AttributeAvailableMask,
    _In_ PVOID Context OPTIONAL,
    _In_ PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
    _Out_ PULONGLONG OutStart)
{
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE TmpRange;
    ULONGLONG Start;
    ULONGLONG End;

    PAGED_CODE_RTL();
    DPRINT("RtlFindRange: [%X] %p, %X, [%I64X-%I64X], %X, %X\n", Flags, RangeList, RangeList->Count, Minimum, Maximum, Length, Alignment);

    ASSERT(RangeList);
    ASSERT(OutStart);
    ASSERT(Alignment > 0);
    ASSERT(Length > 0);

    Start = Maximum - (Length - 1) - ((Maximum - (Length - 1)) % Alignment);

    if ((Minimum > Maximum) ||
        ((Maximum - Minimum) < (Length - 1)) ||
        ((Minimum + Alignment) < Minimum) ||
        (Start < Minimum) ||
        Length == 0 ||
        Alignment == 0)
    {
      #ifdef NDEBUG
        DPRINT1("RtlFindRange: [%X] %p, %X, [%I64X-%I64X], %X, %X\n", Flags, RangeList, RangeList->Count, Minimum, Maximum, Length, Alignment);
      #endif

        DPRINT1("RtlFindRange: Testing: STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    End = Start + Length - 1;
    RtlGetLastRange(RangeList, &Iterator, &TmpRange);

    do
    {
        DPRINT("RtlFindRange: Testing [%I64X-%I64X]\n", Start, End);

        if (RtlpIsRangeAvailable(&Iterator,
                                 Start,
                                 End,
                                 AttributeAvailableMask,
                                 ((Flags & RTL_RANGE_LIST_SHARED_OK) != 0),
                                 ((Flags & RTL_RANGE_LIST_NULL_CONFLICT_OK) != 0),
                                 FALSE,
                                 Context,
                                 Callback))
        {
            *OutStart = Start;
            ASSERT((*OutStart >= Minimum) && (*OutStart + Length - 1 <= Maximum));
            DPRINT("RtlFindRange: return STATUS_SUCCESS\n");
            return STATUS_SUCCESS;
        }

        Start = ((PRTLP_RANGE_LIST_ENTRY)(Iterator.Current))->Start;
        DPRINT("RtlFindRange: Iterator Start %I64X\n", Start);

        if (Start - Length > Start) {
            DPRINT("RtlFindRange: break\n");
            break;
        }

        Start = Start - Length - (Start % Alignment);
        End = Start + Length - 1;

        DPRINT("RtlFindRange: [%I64X-%I64X], Minimum %I64X\n", Start, End, Minimum);
    }
    while (Start >= Minimum);

    DPRINT("RtlFindRange: return STATUS_UNSUCCESSFUL\n");

    return STATUS_UNSUCCESSFUL;
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
    PRTLP_RANGE_LIST_ENTRY RtlEntry;
    PRTLP_RANGE_LIST_ENTRY NextRtlEntry;

    PAGED_CODE_RTL();
    ASSERT(RangeList);

    DPRINT("RtlFreeRangeList: RangeList %p, Count %X\n", RangeList, RangeList->Count);

    RangeList->Flags = 0;
    RangeList->Count = 0;

    RtlEntry = CONTAINING_RECORD(RangeList->ListHead.Flink, RTLP_RANGE_LIST_ENTRY, ListEntry);
    NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);

    while (&RtlEntry->ListEntry != &RangeList->ListHead)
    {
        RemoveEntryList(&RtlEntry->ListEntry);
        RtlpDeleteRangeListEntry(RtlEntry);

        RtlEntry = NextRtlEntry;
        NextRtlEntry = RtlpEntryFromLink(RtlEntry->ListEntry.Flink);
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
NTSYSAPI
NTSTATUS
NTAPI
RtlGetFirstRange(
    _In_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE_LIST_ITERATOR Iterator,
    _Out_ PRTL_RANGE * OutRange)
{
    PRTLP_RANGE_LIST_ENTRY FirstRtlEntry;

    Iterator->RangeListHead = &RangeList->ListHead;
    Iterator->Stamp = RangeList->Stamp;

    PAGED_CODE_RTL();
    DPRINT("RtlGetFirstRange: %p, %X, Iterator %p\n", RangeList, RangeList->Count, Iterator);

    if (IsListEmpty(&RangeList->ListHead))
    {
        Iterator->Current = NULL;
        Iterator->MergedHead = NULL;

        *OutRange = NULL;

        return STATUS_NO_MORE_ENTRIES;
    }

    FirstRtlEntry = RtlpEntryFromLink(RangeList->ListHead.Flink);

    if (FirstRtlEntry->PrivateFlags & RTLP_ENTRY_IS_MERGED)
    {
        ASSERT(!IsListEmpty(&FirstRtlEntry->Merged.ListHead));

        Iterator->MergedHead = &FirstRtlEntry->Merged.ListHead;
        Iterator->Current = RtlpEntryFromLink(FirstRtlEntry->Merged.ListHead.Flink);
    }
    else
    {
        Iterator->MergedHead = NULL;
        Iterator->Current = FirstRtlEntry;
    }

    *OutRange = Iterator->Current;

    DPRINT("RtlGetFirstRange: *OutRange %p\n", *OutRange);

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
NTSYSAPI
NTSTATUS
NTAPI
RtlIsRangeAvailable(
    _In_ PRTL_RANGE_LIST RangeList,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End,
    _In_ ULONG Flags,
    _In_ UCHAR AttributeAvailableMask,
    _In_ PVOID Context OPTIONAL,
    _In_ PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
    _Out_ PBOOLEAN Available)
{
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE RtlRange;
    NTSTATUS Status;

    PAGED_CODE_RTL();
    DPRINT("RtlIsRangeAvailable: [%X] %p, [%I64X-%I64X], %X\n", Flags, RangeList, Start, End, AttributeAvailableMask);

    ASSERT(RangeList);
    ASSERT(Available);

    Status = RtlGetFirstRange(RangeList, &Iterator, &RtlRange);

    if (Status == STATUS_NO_MORE_ENTRIES)
    {
        *Available = TRUE;
        return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlIsRangeAvailable: Status %X\n", Status);
        return Status;
    }

    *Available = RtlpIsRangeAvailable(&Iterator,
                                      Start,
                                      End,
                                      AttributeAvailableMask,
                                      ((Flags & RTL_RANGE_LIST_SHARED_OK) != 0),
                                      ((Flags & RTL_RANGE_LIST_NULL_CONFLICT_OK) != 0),
                                      TRUE,
                                      Context,
                                      Callback);
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
