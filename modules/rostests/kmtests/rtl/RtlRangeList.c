/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for Rtl Range Lists
 * COPYRIGHT:   Copyright 2020 Thomas Faber (thomas.faber@reactos.org)
 */

#include <kmt_test.h>
#include <ndk/rtlfuncs.h>
#include <stdint.h>

static UCHAR MyUserData1, MyUserData2;
static UCHAR MyOwner1, MyOwner2;

/* Conflict callbacks: return TRUE to tell the range list to ignore the conflict. */
static
BOOLEAN
NTAPI
IgnoreConflictCallback(
    _In_ PVOID Context,
    _In_ struct _RTL_RANGE *Range)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Range);
    return TRUE;
}

static
BOOLEAN
NTAPI
KeepConflictCallback(
    _In_ PVOID Context,
    _In_ struct _RTL_RANGE *Range)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Range);
    return FALSE;
}

/* Helpers *******************************************************************/
static
NTSTATUS
RtlAddRangeWrapper(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _In_ const RTL_RANGE *Range,
    _In_ ULONG Flags)
{
    return RtlAddRange(RangeList,
                       Range->Start,
                       Range->End,
                       Range->Attributes,
                       Flags,
                       Range->UserData,
                       Range->Owner);
}

static
void
ExpectRange(
    _In_ PCSTR File,
    _In_ INT Line,
    _In_ ULONG Index,
    _In_ const RTL_RANGE *ActualRange,
    _In_ const RTL_RANGE *ExpectedRange)
{
    CHAR FileAndLine[128];
    RtlStringCbPrintfA(FileAndLine, sizeof(FileAndLine), "%s:%d", File, Line);

    KmtOk(ActualRange->Start == ExpectedRange->Start, FileAndLine,
        "[%lu] Start = 0x%I64x, expected 0x%I64x\n", Index, ActualRange->Start, ExpectedRange->Start);
    KmtOk(ActualRange->End == ExpectedRange->End, FileAndLine,
        "[%lu] End = 0x%I64x, expected 0x%I64x\n", Index, ActualRange->End, ExpectedRange->End);
    KmtOk(ActualRange->UserData == ExpectedRange->UserData, FileAndLine,
        "[%lu] UserData = %p, expected %p\n", Index, ActualRange->UserData, ExpectedRange->UserData);
    KmtOk(ActualRange->Owner == ExpectedRange->Owner, FileAndLine,
        "[%lu] Owner = %p, expected %p\n", Index, ActualRange->Owner, ExpectedRange->Owner);
    KmtOk(ActualRange->Attributes == ExpectedRange->Attributes, FileAndLine,
        "[%lu] Attributes = 0x%x, expected 0x%x\n", Index, ActualRange->Attributes, ExpectedRange->Attributes);
    KmtOk(ActualRange->Flags == ExpectedRange->Flags, FileAndLine,
        "[%lu] Flags = 0x%x, expected 0x%x\n", Index, ActualRange->Flags, ExpectedRange->Flags);
}

static
void
ExpectRangeEntryList(
    _In_ PCSTR File,
    _In_ INT Line,
    _In_ RTL_RANGE_LIST *RangeList,
    _In_ ULONG NumRanges,
    _In_reads_(NumRanges) const RTL_RANGE *Ranges)
{
    NTSTATUS Status;
    ULONG i;
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;
    CHAR FileAndLine[128];
    RtlStringCbPrintfA(FileAndLine, sizeof(FileAndLine), "%s:%d", File, Line);

    RtlFillMemory(&Iterator, sizeof(Iterator), 0x55);
    Range = KmtInvalidPointer;
    Status = RtlGetFirstRange(RangeList, &Iterator, &Range);
#ifdef _WIN64
    /* Padding at the end is uninitialized */
    C_ASSERT(sizeof(Iterator) == RTL_SIZEOF_THROUGH_FIELD(RTL_RANGE_LIST_ITERATOR, Stamp) + sizeof(ULONG));
    KmtOk((&Iterator.Stamp)[1] == 0x55555555, FileAndLine,
        "Padding is 0x%lx\n", (&Iterator.Stamp)[1]);
#endif

    for (i = 0; i < NumRanges; i++)
    {
        if (!KmtSkip(NT_SUCCESS(Status), FileAndLine, "Range does not have %lu element(s)\n", i + 1))
        {
            ExpectRange(File, Line, i, Range, &Ranges[i]);

            /* Validate iterator */
            KmtOk(Iterator.RangeListHead == &RangeList->ListHead, FileAndLine,
                "[%lu] Iterator.RangeListHead = %p, expected %p\n", i, Iterator.RangeListHead, &RangeList->ListHead);
            KmtOk(Iterator.MergedHead == NULL, FileAndLine,
                "[%lu] Iterator.MergedHead = %p\n", i, Iterator.MergedHead);
            KmtOk(Iterator.Current == Range, FileAndLine,
                "[%lu] Iterator.Current = %p, expected %p\n", i, Iterator.Current, Range);
            KmtOk(Iterator.Stamp == RangeList->Stamp, FileAndLine,
                "[%lu] Iterator.Stamp = %lu, expected %lu\n", i, Iterator.Stamp, RangeList->Stamp);
        }

        Range = KmtInvalidPointer;
        Status = RtlGetNextRange(&Iterator, &Range, TRUE);
    }

    /* Final iteration status */
    KmtOk(Status == STATUS_NO_MORE_ENTRIES, FileAndLine,
        "Status = 0x%lx after enumeration\n", Status);
    KmtOk(Range == NULL, FileAndLine,
        "[%lu] Range = %p\n", i, Range);
    KmtOk(Iterator.RangeListHead == &RangeList->ListHead, FileAndLine,
        "[%lu] Iterator.RangeListHead = %p, expected %p\n", i, Iterator.RangeListHead, &RangeList->ListHead);
    KmtOk(Iterator.MergedHead == NULL, FileAndLine,
        "[%lu] Iterator.MergedHead = %p\n", i, Iterator.MergedHead);
    KmtOk(Iterator.Current == NULL, FileAndLine,
        "[%lu] Iterator.Current = %p\n", i, Iterator.Current);
    KmtOk(Iterator.Stamp == RangeList->Stamp, FileAndLine,
        "[%lu] Iterator.Stamp = %lu, expected %lu\n", i, Iterator.Stamp, RangeList->Stamp);

    /* Try one more iteration */
    Range = KmtInvalidPointer;
    Status = RtlGetNextRange(&Iterator, &Range, TRUE);
    KmtOk(Status == STATUS_NO_MORE_ENTRIES, FileAndLine,
        "Status = 0x%lx after enumeration\n", Status);
    KmtOk(Range == NULL, FileAndLine,
        "[%lu] Range = %p\n", i, Range);
    KmtOk(Iterator.RangeListHead == &RangeList->ListHead, FileAndLine,
        "[%lu] Iterator.RangeListHead = %p, expected %p\n", i, Iterator.RangeListHead, &RangeList->ListHead);
    KmtOk(Iterator.MergedHead == NULL, FileAndLine,
        "[%lu] Iterator.MergedHead = %p\n", i, Iterator.MergedHead);
    KmtOk(Iterator.Current == NULL, FileAndLine,
        "[%lu] Iterator.Current = %p\n", i, Iterator.Current);
    KmtOk(Iterator.Stamp == RangeList->Stamp, FileAndLine,
        "[%lu] Iterator.Stamp = %lu, expected %lu\n", i, Iterator.Stamp, RangeList->Stamp);
}

#define expect_range_entries(RangeList, NumRanges, Ranges) \
    ExpectRangeEntryList(__FILE__, __LINE__, RangeList, NumRanges, Ranges)

/* Test functions ************************************************************/
static
void
TestStartGreaterThanEnd(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    ULONG StartStamp = RangeList->Stamp;

    Ranges[1].Start = 0x300;
    Ranges[1].End = 0x2ff;
    Ranges[1].Attributes = 2;
    Ranges[1].Flags = 0;
    Ranges[1].UserData = &MyUserData2;
    Ranges[1].Owner = &MyOwner2;

    /* Start > End bails out early with invalid parameter */
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], 0);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* List should be unchanged */
    ok_eq_ulong(RangeList->Flags, 0UL);
    ok_eq_ulong(RangeList->Count, 1UL);
    ok_eq_ulong(RangeList->Stamp, StartStamp);
    expect_range_entries(RangeList, 1, &Ranges[0]);
}

static
void
TestStartEqualsEnd(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    ULONG StartStamp = RangeList->Stamp;

    Ranges[1].Start = 0x300;
    Ranges[1].End = 0x300;
    Ranges[1].Attributes = 0xff;
    Ranges[1].Flags = 0;
    Ranges[1].UserData = &MyUserData2;
    Ranges[1].Owner = &MyOwner2;

    /* Start == End is valid */
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* List now has two entries */
    ok_eq_ulong(RangeList->Flags, 0UL);
    ok_eq_ulong(RangeList->Count, 2UL);
    ok_eq_ulong(RangeList->Stamp, StartStamp + 1);
    expect_range_entries(RangeList, 2, &Ranges[0]);

    /* Delete our new entry -- List goes back to one entry */
    Status = RtlDeleteRange(RangeList, Ranges[1].Start, Ranges[1].End, Ranges[1].Owner);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList->Flags, 0UL);
    ok_eq_ulong(RangeList->Count, 1UL);
    ok_eq_ulong(RangeList->Stamp, StartStamp + 2);
    expect_range_entries(RangeList, 1, &Ranges[0]);
}

static
void
TestSharedFlag(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    ULONG StartStamp = RangeList->Stamp;

    Ranges[1].Start = 0x300;
    Ranges[1].End = 0x400;
    Ranges[1].Attributes = 2;
    Ranges[1].Flags = RTL_RANGE_SHARED;
    Ranges[1].UserData = &MyUserData2;
    Ranges[1].Owner = &MyOwner2;

    /* Pass in the shared flag */
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], RTL_RANGE_LIST_ADD_SHARED);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* List now has two entries */
    ok_eq_ulong(RangeList->Flags, 0UL);
    ok_eq_ulong(RangeList->Count, 2UL);
    ok_eq_ulong(RangeList->Stamp, StartStamp + 1);
    expect_range_entries(RangeList, 2, &Ranges[0]);

    /* Delete our new entry -- List goes back to one entry */
    Status = RtlDeleteRange(RangeList, Ranges[1].Start, Ranges[1].End, Ranges[1].Owner);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList->Flags, 0UL);
    ok_eq_ulong(RangeList->Count, 1UL);
    ok_eq_ulong(RangeList->Stamp, StartStamp + 2);
    expect_range_entries(RangeList, 1, &Ranges[0]);
}

static
void
TestIsAvailable(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    BOOLEAN Available;
    ULONG StartStamp = RangeList->Stamp;

#define is_range_available(RangeList, Start, End, pAvail)   \
    RtlIsRangeAvailable(RangeList,                          \
                        Start,                              \
                        End,                                \
                        0,                                  \
                        0,                                  \
                        NULL,                               \
                        NULL,                               \
                        pAvail)

    /* Single item range before Start */
    Status = is_range_available(RangeList,
                                Ranges[0].Start - 1,
                                Ranges[0].Start - 1,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* Single item range at Start */
    Status = is_range_available(RangeList,
                                Ranges[0].Start,
                                Ranges[0].Start,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Single item range at End */
    Status = is_range_available(RangeList,
                                Ranges[0].End,
                                Ranges[0].End,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Single item range after End */
    Status = is_range_available(RangeList,
                                Ranges[0].End + 1,
                                Ranges[0].End + 1,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* Range ending before Start */
    Status = is_range_available(RangeList,
                                0x0,
                                Ranges[0].Start - 1,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* Range ending at Start */
    Status = is_range_available(RangeList,
                                0x0,
                                Ranges[0].Start,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Range ending in the middle */
    Status = is_range_available(RangeList,
                                0x0,
                                (Ranges[0].Start + Ranges[0].End) / 2,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Range going all the way through */
    Status = is_range_available(RangeList,
                                0x0,
                                Ranges[0].End + 0x100,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Range starting in the middle */
    Status = is_range_available(RangeList,
                                (Ranges[0].Start + Ranges[0].End) / 2,
                                Ranges[0].End + 0x100,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Range starting at End */
    Status = is_range_available(RangeList,
                                Ranges[0].End,
                                Ranges[0].End + 0x100,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Range starting after End */
    Status = is_range_available(RangeList,
                                Ranges[0].End + 1,
                                Ranges[0].End + 0x100,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* Start > End, at start */
    Status = is_range_available(RangeList,
                                Ranges[0].Start,
                                Ranges[0].Start - 1,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* Start > End, at start */
    Status = is_range_available(RangeList,
                                Ranges[0].Start + 1,
                                Ranges[0].Start,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Start > End, at end */
    Status = is_range_available(RangeList,
                                Ranges[0].End + 1,
                                Ranges[0].End,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* Start > End, at end */
    Status = is_range_available(RangeList,
                                Ranges[0].End,
                                Ranges[0].End - 1,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Start > End, through the range */
    Status = is_range_available(RangeList,
                                Ranges[0].End + 1,
                                Ranges[0].Start - 1,
                                &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* AttributesAvailableMask will make our range available */
    Status = RtlIsRangeAvailable(RangeList,
                                 0x0,
                                 Ranges[0].End + 0x100,
                                 0,
                                 Ranges[0].Attributes,
                                 NULL,
                                 NULL,
                                 &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* AttributesAvailableMask with additional bits */
    Status = RtlIsRangeAvailable(RangeList,
                                 0x0,
                                 Ranges[0].End + 0x100,
                                 0,
                                 0xFF,
                                 NULL,
                                 NULL,
                                 &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    ok_eq_ulong(RangeList->Stamp, StartStamp);
}

static
void
TestAddConflict(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    ULONG StartStamp = RangeList->Stamp;

    /* A range overlapping our [0x100, 0x200] entry */
    Ranges[1].Start = 0x180;
    Ranges[1].End = 0x280;
    Ranges[1].Attributes = 0;
    Ranges[1].Flags = 0;
    Ranges[1].UserData = &MyUserData2;
    Ranges[1].Owner = &MyOwner2;

    /* Without RTL_RANGE_LIST_ADD_IF_CONFLICT the add is rejected */
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], 0);
    ok_eq_hex(Status, STATUS_RANGE_LIST_CONFLICT);
    ok_eq_ulong(RangeList->Count, 1UL);
    ok_eq_ulong(RangeList->Stamp, StartStamp);
    expect_range_entries(RangeList, 1, &Ranges[0]);

    /* With the flag it succeeds */
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], RTL_RANGE_LIST_ADD_IF_CONFLICT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList->Count, 2UL);

    /* Restore the single-entry list */
    Status = RtlDeleteRange(RangeList, Ranges[1].Start, Ranges[1].End, Ranges[1].Owner);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList->Count, 1UL);
    expect_range_entries(RangeList, 1, &Ranges[0]);
}

static
void
TestFindRange(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    ULONGLONG Start;
    RTL_RANGE_LIST EmptyList;

    /* Invalid parameters */
    Start = 0x55;
    Status = RtlFindRange(RangeList, 0x0, 0x1000, 0 /* Length */, 1, 0, 0, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = RtlFindRange(RangeList, 0x0, 0x1000, 0x10, 0 /* Alignment */, 0, 0, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    /* Top-down search finds the highest aligned hole below Maximum */
    Start = 0x55;
    Status = RtlFindRange(RangeList, 0x0, 0x1000, 0x10, 1, 0, 0, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulonglong(Start, 0xFF1ULL);

    /* Alignment is honored */
    Start = 0x55;
    Status = RtlFindRange(RangeList, 0x0, 0x1000, 0x10, 0x100, 0, 0, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulonglong(Start, 0xF00ULL);

    /* Capping Maximum at the occupied range forces the search below it */
    Start = 0x55;
    Status = RtlFindRange(RangeList, 0x0, 0x200, 0x10, 1, 0, 0, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulonglong(Start, 0xF0ULL);

    /* No room below and the range is opaque -> not found */
    Start = 0x55;
    Status = RtlFindRange(RangeList, 0x100, 0x200, 0x10, 1, 0, 0, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_RANGE_NOT_FOUND);

    /* AttributeAvailableMask makes the occupied range transparent -> place on top of it */
    Start = 0x55;
    Status = RtlFindRange(RangeList, 0x0, 0x200, 0x10, 1, 0, Ranges[0].Attributes, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulonglong(Start, 0x1F1ULL);

    /* Empty list: the whole [Minimum, Maximum] window is free */
    RtlInitializeRangeList(&EmptyList);
    Start = 0x55;
    Status = RtlFindRange(&EmptyList, 0x0, 0xFFF, 0x10, 1, 0, 0, NULL, NULL, &Start);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulonglong(Start, 0xFF0ULL);
    RtlFreeRangeList(&EmptyList);

    /* Searching never mutates the list */
    expect_range_entries(RangeList, 1, &Ranges[0]);
}

static
void
TestGetLastRange(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;
    RTL_RANGE_LIST EmptyList;

    /* Single entry: last == first */
    Range = KmtInvalidPointer;
    Status = RtlGetLastRange(RangeList, &Iterator, &Range);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (Range != NULL && Range != KmtInvalidPointer)
    {
        ok_eq_ulonglong(Range->Start, Ranges[0].Start);
        ok_eq_ulonglong(Range->End, Ranges[0].End);
    }
    ok_eq_pointer(Iterator.RangeListHead, &RangeList->ListHead);
    ok_eq_pointer(Iterator.MergedHead, NULL);

    /* Add a higher second entry */
    Ranges[1].Start = 0x300;
    Ranges[1].End = 0x400;
    Ranges[1].Attributes = 2;
    Ranges[1].Flags = 0;
    Ranges[1].UserData = &MyUserData2;
    Ranges[1].Owner = &MyOwner2;
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* GetLastRange returns the higher entry */
    Range = KmtInvalidPointer;
    Status = RtlGetLastRange(RangeList, &Iterator, &Range);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (Range != NULL && Range != KmtInvalidPointer)
    {
        ok_eq_ulonglong(Range->Start, Ranges[1].Start);
        ok_eq_ulonglong(Range->End, Ranges[1].End);
    }

    /* Walking backwards reaches the first entry, then stops */
    Range = KmtInvalidPointer;
    Status = RtlGetNextRange(&Iterator, &Range, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (Range != NULL && Range != KmtInvalidPointer)
    {
        ok_eq_ulonglong(Range->Start, Ranges[0].Start);
        ok_eq_ulonglong(Range->End, Ranges[0].End);
    }
    Range = KmtInvalidPointer;
    Status = RtlGetNextRange(&Iterator, &Range, FALSE);
    ok_eq_hex(Status, STATUS_NO_MORE_ENTRIES);
    ok_eq_pointer(Range, NULL);

    /* Restore the single-entry list */
    Status = RtlDeleteRange(RangeList, Ranges[1].Start, Ranges[1].End, Ranges[1].Owner);
    ok_eq_hex(Status, STATUS_SUCCESS);
    expect_range_entries(RangeList, 1, &Ranges[0]);

    /* Empty list */
    RtlInitializeRangeList(&EmptyList);
    Range = KmtInvalidPointer;
    Status = RtlGetLastRange(&EmptyList, &Iterator, &Range);
    ok_eq_hex(Status, STATUS_NO_MORE_ENTRIES);
    ok_eq_pointer(Range, NULL);
    ok_eq_pointer(Iterator.Current, NULL);
    RtlFreeRangeList(&EmptyList);
}

static
void
TestSharedNullCallback(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;
    BOOLEAN Available;

    /* A shared range */
    Ranges[1].Start = 0x300;
    Ranges[1].End = 0x400;
    Ranges[1].Attributes = 0;
    Ranges[1].Flags = RTL_RANGE_SHARED;
    Ranges[1].UserData = &MyUserData2;
    Ranges[1].Owner = &MyOwner2;
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], RTL_RANGE_LIST_ADD_SHARED);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Not available without RTL_RANGE_LIST_SHARED_OK */
    Status = RtlIsRangeAvailable(RangeList, 0x300, 0x400, 0, 0, NULL, NULL, &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Available with RTL_RANGE_LIST_SHARED_OK */
    Status = RtlIsRangeAvailable(RangeList, 0x300, 0x400, RTL_RANGE_LIST_SHARED_OK, 0, NULL, NULL, &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* A callback that ignores the conflict makes it available */
    Status = RtlIsRangeAvailable(RangeList, 0x300, 0x400, 0, 0, NULL, IgnoreConflictCallback, &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    /* A callback that keeps the conflict leaves it unavailable */
    Status = RtlIsRangeAvailable(RangeList, 0x300, 0x400, 0, 0, NULL, KeepConflictCallback, &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    Status = RtlDeleteRange(RangeList, Ranges[1].Start, Ranges[1].End, Ranges[1].Owner);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* A NULL-owner range */
    Ranges[1].Start = 0x500;
    Ranges[1].End = 0x600;
    Ranges[1].Attributes = 0;
    Ranges[1].Flags = 0;
    Ranges[1].UserData = NULL;
    Ranges[1].Owner = NULL;
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Not available without RTL_RANGE_LIST_NULL_CONFLICT_OK */
    Status = RtlIsRangeAvailable(RangeList, 0x500, 0x600, 0, 0, NULL, NULL, &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, FALSE);

    /* Available with RTL_RANGE_LIST_NULL_CONFLICT_OK */
    Status = RtlIsRangeAvailable(RangeList, 0x500, 0x600, RTL_RANGE_LIST_NULL_CONFLICT_OK, 0, NULL, NULL, &Available);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_bool(Available, TRUE);

    Status = RtlDeleteRange(RangeList, 0x500, 0x600, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    expect_range_entries(RangeList, 1, &Ranges[0]);
}

static
void
TestDeleteOwnersRanges(
    _Inout_ PRTL_RANGE_LIST RangeList,
    _Inout_ PRTL_RANGE Ranges)
{
    NTSTATUS Status;

    /* Two entries owned by MyOwner2, adjacent in iteration order */
    Ranges[1].Start = 0x300;
    Ranges[1].End = 0x400;
    Ranges[1].Attributes = 0;
    Ranges[1].Flags = 0;
    Ranges[1].UserData = &MyUserData2;
    Ranges[1].Owner = &MyOwner2;
    Status = RtlAddRangeWrapper(RangeList, &Ranges[1], 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Ranges[2].Start = 0x500;
    Ranges[2].End = 0x600;
    Ranges[2].Attributes = 0;
    Ranges[2].Flags = 0;
    Ranges[2].UserData = &MyUserData2;
    Ranges[2].Owner = &MyOwner2;
    Status = RtlAddRangeWrapper(RangeList, &Ranges[2], 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList->Count, 3UL);

    /*
     * Deleting MyOwner2 removes both entries in a single pass. Deleting two
     * consecutively-matched entries exercises the delete-during-iteration path
     * (which must capture Flink before freeing the current entry).
     */
    Status = RtlDeleteOwnersRanges(RangeList, &MyOwner2);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList->Count, 1UL);
    expect_range_entries(RangeList, 1, &Ranges[0]);

    /* Deleting an owner with no ranges changes nothing */
    Status = RtlDeleteOwnersRanges(RangeList, &MyOwner2);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList->Count, 1UL);
    expect_range_entries(RangeList, 1, &Ranges[0]);
}

static
void
TestInvertRangeList(void)
{
    NTSTATUS Status;
    RTL_RANGE_LIST List;
    RTL_RANGE_LIST Inverted;
    RTL_RANGE Expected[3];

    RtlInitializeRangeList(&List);
    Status = RtlAddRange(&List, 0x100, 0x200, 0, RTL_RANGE_LIST_ADD_IF_CONFLICT, NULL, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = RtlAddRange(&List, 0x400, 0x500, 0, RTL_RANGE_LIST_ADD_IF_CONFLICT, NULL, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Plain invert: leading, middle and trailing gaps, Attributes 0 / Owner NULL */
    RtlInitializeRangeList(&Inverted);
    Status = RtlInvertRangeList(&Inverted, &List);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(Inverted.Count, 3UL);
    Expected[0].Start = 0x0;   Expected[0].End = 0xFF;
    Expected[1].Start = 0x201; Expected[1].End = 0x3FF;
    Expected[2].Start = 0x501; Expected[2].End = ULLONG_MAX;
    Expected[0].Attributes = Expected[1].Attributes = Expected[2].Attributes = 0;
    Expected[0].Flags = Expected[1].Flags = Expected[2].Flags = 0;
    Expected[0].UserData = Expected[1].UserData = Expected[2].UserData = NULL;
    Expected[0].Owner = Expected[1].Owner = Expected[2].Owner = NULL;
    expect_range_entries(&Inverted, 3, Expected);
    RtlFreeRangeList(&Inverted);

    /* Ex invert: the gap ranges carry the supplied Attributes / UserData / Owner */
    RtlInitializeRangeList(&Inverted);
    Status = RtlInvertRangeListEx(&Inverted, &List, 0x5, &MyUserData1, &MyOwner1);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(Inverted.Count, 3UL);
    Expected[0].Attributes = Expected[1].Attributes = Expected[2].Attributes = 0x5;
    Expected[0].UserData = Expected[1].UserData = Expected[2].UserData = &MyUserData1;
    Expected[0].Owner = Expected[1].Owner = Expected[2].Owner = &MyOwner1;
    expect_range_entries(&Inverted, 3, Expected);
    RtlFreeRangeList(&Inverted);

    RtlFreeRangeList(&List);

    /* Inverting an empty list yields the whole address space */
    RtlInitializeRangeList(&List);
    RtlInitializeRangeList(&Inverted);
    Status = RtlInvertRangeList(&Inverted, &List);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(Inverted.Count, 1UL);
    Expected[0].Start = 0x0;
    Expected[0].End = ULLONG_MAX;
    Expected[0].Attributes = 0;
    Expected[0].Flags = 0;
    Expected[0].UserData = NULL;
    Expected[0].Owner = NULL;
    expect_range_entries(&Inverted, 1, Expected);
    RtlFreeRangeList(&Inverted);
    RtlFreeRangeList(&List);
}

/* Entry point ***************************************************************/
START_TEST(RtlRangeList)
{
    NTSTATUS Status;
    RTL_RANGE_LIST RangeList;
    RTL_RANGE Ranges[5];
    ULONG Stamp;

    RtlFillMemory(&RangeList, sizeof(RangeList), 0x55);
    RtlInitializeRangeList(&RangeList);
    ok(IsListEmpty(&RangeList.ListHead),
       "RangeList.ListHead %p %p %p, expected empty\n",
       &RangeList.ListHead, RangeList.ListHead.Flink, RangeList.ListHead.Blink);
    ok_eq_ulong(RangeList.Flags, 0UL);
    ok_eq_ulong(RangeList.Count, 0UL);
    ok_eq_ulong(RangeList.Stamp, 0UL);
#ifdef _WIN64
    /* Padding at the end is uninitialized */
    C_ASSERT(sizeof(RangeList) == RTL_SIZEOF_THROUGH_FIELD(RTL_RANGE_LIST, Stamp) + sizeof(ULONG));
    ok_eq_ulong((&RangeList.Stamp)[1], 0x55555555UL);
#endif

    /* Add a simple range */
    Ranges[0].Start = 0x100;
    Ranges[0].End = 0x200;
    Ranges[0].Attributes = 1;
    Ranges[0].Flags = 0;
    Ranges[0].UserData = &MyUserData1;
    Ranges[0].Owner = &MyOwner1;
    Status = RtlAddRangeWrapper(&RangeList, &Ranges[0], 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(RangeList.Flags, 0UL);
    ok_eq_ulong(RangeList.Count, 1UL);
    ok_eq_ulong(RangeList.Stamp, 1UL);
    expect_range_entries(&RangeList, 1, &Ranges[0]);

    /*
     * Individual tests.
     * These should always leave the list with our single start entry.
     * Stamp may change between tests.
     */
    TestStartGreaterThanEnd(&RangeList, Ranges);
    TestStartEqualsEnd(&RangeList, Ranges);
    TestSharedFlag(&RangeList, Ranges);
    TestIsAvailable(&RangeList, Ranges);
    TestAddConflict(&RangeList, Ranges);
    TestFindRange(&RangeList, Ranges);
    TestGetLastRange(&RangeList, Ranges);
    TestSharedNullCallback(&RangeList, Ranges);
    TestDeleteOwnersRanges(&RangeList, Ranges);
    TestInvertRangeList();

    Stamp = RangeList.Stamp;

    /* Free it and check the result */
    RtlFreeRangeList(&RangeList);
    ok_eq_ulong(RangeList.Flags, 0UL);
    ok_eq_ulong(RangeList.Count, 0UL);
    ok_eq_ulong(RangeList.Stamp, Stamp);
    expect_range_entries(&RangeList, 0, NULL);
}
