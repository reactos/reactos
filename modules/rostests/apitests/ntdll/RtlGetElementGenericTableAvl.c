/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for RtlGetElementGenericTableAvl
 * COPYRIGHT:   Copyright 2026 Alex Mendoza
 */

#include "precomp.h"

typedef struct _TEST_ENTRY
{
    ULONG Key;
    ULONG Value;
} TEST_ENTRY, *PTEST_ENTRY;

static RTL_GENERIC_COMPARE_RESULTS
NTAPI
CompareRoutine(
    PRTL_AVL_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct)
{
    PTEST_ENTRY A = FirstStruct;
    PTEST_ENTRY B = SecondStruct;

    if (A->Key < B->Key)
        return GenericLessThan;
    if (A->Key > B->Key)
        return GenericGreaterThan;
    return GenericEqual;
}

static PVOID
NTAPI
AllocateRoutine(
    PRTL_AVL_TABLE Table,
    CLONG ByteSize)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), 0, ByteSize);
}

static VOID
NTAPI
FreeRoutine(
    PRTL_AVL_TABLE Table,
    PVOID Buffer)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
}

static void
check_element(
    PRTL_AVL_TABLE Table,
    ULONG Index,
    ULONG ExpectedKey,
    BOOLEAN ExpectNull)
{
    PTEST_ENTRY Entry;

    Entry = RtlGetElementGenericTableAvl(Table, Index);

    if (ExpectNull)
    {
        ok(Entry == NULL, "Index %lu: expected NULL, got %p\n", Index, Entry);
    }
    else
    {
        ok(Entry != NULL, "Index %lu: expected non-NULL\n", Index);
        if (Entry)
            ok(Entry->Key == ExpectedKey,
               "Index %lu: expected key %lu, got %lu\n",
               Index, ExpectedKey, Entry->Key);
    }
}

static void
TestEmptyAndOutOfRange(void)
{
    RTL_AVL_TABLE Table;

    RtlInitializeGenericTableAvl(&Table,
                                 CompareRoutine,
                                 AllocateRoutine,
                                 FreeRoutine,
                                 NULL);

    check_element(&Table, 0, 0, TRUE);
    check_element(&Table, 1, 0, TRUE);
    check_element(&Table, MAXULONG, 0, TRUE);
    check_element(&Table, 0x80000000, 0, TRUE);
}

static void
TestSingle(void)
{
    RTL_AVL_TABLE Table;
    TEST_ENTRY Entry = { 42, 100 };
    BOOLEAN NewElement;

    RtlInitializeGenericTableAvl(&Table,
                                 CompareRoutine,
                                 AllocateRoutine,
                                 FreeRoutine,
                                 NULL);

    RtlInsertElementGenericTableAvl(&Table, &Entry, sizeof(Entry), &NewElement);

    check_element(&Table, 0, 42, FALSE);
    check_element(&Table, 1, 0, TRUE);
    check_element(&Table, MAXULONG, 0, TRUE);
}

static void
TestSortedOrderAndDelete(void)
{
    RTL_AVL_TABLE Table;
    TEST_ENTRY Entries[] = { {30, 300}, {10, 100}, {20, 200}, {40, 400} };
    BOOLEAN NewElement;
    ULONG i;

    RtlInitializeGenericTableAvl(&Table,
                                 CompareRoutine,
                                 AllocateRoutine,
                                 FreeRoutine,
                                 NULL);

    for (i = 0; i < RTL_NUMBER_OF(Entries); i++)
    {
        RtlInsertElementGenericTableAvl(&Table,
                                        &Entries[i],
                                        sizeof(Entries[i]),
                                        &NewElement);
    }

    check_element(&Table, 0, 10, FALSE);
    check_element(&Table, 1, 20, FALSE);
    check_element(&Table, 2, 30, FALSE);
    check_element(&Table, 3, 40, FALSE);
    check_element(&Table, 4, 0, TRUE);

    /* Delete the element with key 20 */
    RtlDeleteElementGenericTableAvl(&Table, &Entries[2]);

    check_element(&Table, 0, 10, FALSE);
    check_element(&Table, 1, 30, FALSE);
    check_element(&Table, 2, 40, FALSE);
    check_element(&Table, 3, 0, TRUE);
}

static void
TestAccessPatterns(void)
{
    RTL_AVL_TABLE Table;
    TEST_ENTRY Entries[8];
    BOOLEAN NewElement;
    ULONG i;
    static const ULONG Expected[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    static const ULONG Order[] = { 3, 0, 5, 1, 7, 2, 6, 4 };

    RtlInitializeGenericTableAvl(&Table,
                                 CompareRoutine,
                                 AllocateRoutine,
                                 FreeRoutine,
                                 NULL);

    for (i = 0; i < RTL_NUMBER_OF(Entries); i++)
    {
        Entries[i].Key = (i + 1) * 10;
        Entries[i].Value = Entries[i].Key * 10;
        RtlInsertElementGenericTableAvl(&Table,
                                        &Entries[i],
                                        sizeof(Entries[i]),
                                        &NewElement);
    }

    /* Sequential forward */
    for (i = 0; i < RTL_NUMBER_OF(Expected); i++)
        check_element(&Table, i, Expected[i], FALSE);

    /* Sequential reverse */
    for (i = RTL_NUMBER_OF(Expected); i-- > 0; )
        check_element(&Table, i, Expected[i], FALSE);

    /* Random access */
    for (i = 0; i < RTL_NUMBER_OF(Order); i++)
        check_element(&Table, Order[i], Expected[Order[i]], FALSE);

    check_element(&Table, 8, 0, TRUE);
    check_element(&Table, 7, 80, FALSE);
}

START_TEST(RtlGetElementGenericTableAvl)
{
    TestEmptyAndOutOfRange();
    TestSingle();
    TestSortedOrderAndDelete();
    TestAccessPatterns();
}