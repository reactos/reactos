/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for Rtl Range Lists
 * COPYRIGHT:   Copyright 2020 Thomas Faber (thomas.faber@reactos.org)
 */

#include <kmt_test.h>
#include <ndk/rtlfuncs.h>

static UCHAR MyUserData1, MyUserData2;
static UCHAR MyOwner1, MyOwner2;

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

    Stamp = RangeList.Stamp;

    /* Free it and check the result */
    RtlFreeRangeList(&RangeList);
    ok_eq_ulong(RangeList.Flags, 0UL);
    ok_eq_ulong(RangeList.Count, 0UL);
    ok_eq_ulong(RangeList.Stamp, Stamp);
    expect_range_entries(&RangeList, 0, NULL);
}
