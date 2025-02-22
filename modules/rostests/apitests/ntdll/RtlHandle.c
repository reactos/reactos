/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for Rtl handle tables
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

typedef struct _TEST_HANDLE_ENTRY
{
    RTL_HANDLE_TABLE_ENTRY HandleEntry;
    ULONG Data;
} TEST_HANDLE_ENTRY, *PTEST_HANDLE_ENTRY;

START_TEST(RtlHandle)
{
    const ULONG MaxHandles = 2048;
    RTL_HANDLE_TABLE HandleTable;
    PUCHAR HandleBase;
    PRTL_HANDLE_TABLE_ENTRY HandleEntry;
    PTEST_HANDLE_ENTRY TestEntry;
    PTEST_HANDLE_ENTRY TestEntry2;
    ULONG Index;
    BOOLEAN Valid;
    ULONG i;

    /* Initialize handle table */
    RtlFillMemory(&HandleTable, sizeof(HandleTable), 0x55);
    RtlInitializeHandleTable(MaxHandles, sizeof(TEST_HANDLE_ENTRY), &HandleTable);
    ok(HandleTable.MaximumNumberOfHandles == MaxHandles, "MaximumNumberOfHandles = %lu\n", HandleTable.MaximumNumberOfHandles);
    ok(HandleTable.SizeOfHandleTableEntry == sizeof(TEST_HANDLE_ENTRY),
       "SizeOfHandleTableEntry = %lu\n", HandleTable.SizeOfHandleTableEntry);
    ok(HandleTable.Reserved[0] == 0, "Reserved[0] = 0x%lx\n", HandleTable.Reserved[0]);
    ok(HandleTable.Reserved[1] == 0, "Reserved[1] = 0x%lx\n", HandleTable.Reserved[1]);
    ok(HandleTable.CommittedHandles == NULL, "CommittedHandles = %p\n", HandleTable.CommittedHandles);
    ok(HandleTable.UnCommittedHandles == NULL, "UnCommittedHandles = %p\n", HandleTable.UnCommittedHandles);
    ok(HandleTable.MaxReservedHandles == NULL, "MaxReservedHandles = %p\n", HandleTable.MaxReservedHandles);
    ok(HandleTable.FreeHandles == NULL, "FreeHandles = %p\n", HandleTable.FreeHandles);

    /* Allocate a handle, now we have a committed page */
    HandleEntry = RtlAllocateHandle(&HandleTable, &Index);

    ok(HandleTable.CommittedHandles != NULL, "CommittedHandles = %p\n", HandleTable.CommittedHandles);
    HandleBase = (PUCHAR)HandleTable.CommittedHandles;

    trace("CommittedHandles = %p\n", HandleBase);
    ok((PUCHAR)HandleTable.UnCommittedHandles == HandleBase + PAGE_SIZE, "UnCommittedHandles = %p\n", HandleTable.UnCommittedHandles);
    ok((PUCHAR)HandleTable.MaxReservedHandles == HandleBase + MaxHandles * sizeof(TEST_HANDLE_ENTRY), "MaxReservedHandles = %p\n", HandleTable.MaxReservedHandles);
    ok((PUCHAR)HandleTable.FreeHandles == HandleBase + sizeof(TEST_HANDLE_ENTRY), "FreeHandles = %p\n", HandleTable.FreeHandles);

    ok((PUCHAR)HandleEntry == HandleBase, "HandleEntry = %p\n", HandleEntry);
    ok(Index == 0, "Index = %lu\n", Index);

    ok(HandleEntry->Flags == 0, "Flags = 0x%lx\n", HandleEntry->Flags);

    TestEntry = (PTEST_HANDLE_ENTRY)HandleEntry;
    ok(TestEntry->Data == 0, "Data = %lu\n", TestEntry->Data);
    TestEntry->Data = 0x87654321;

    /* Handle is not recognized as valid unless we set the valid flag */
    Valid = RtlIsValidHandle(&HandleTable, &TestEntry->HandleEntry);
    ok(Valid == FALSE, "Valid = %u\n", Valid);
    HandleEntry = InvalidPointer;
    Valid = RtlIsValidIndexHandle(&HandleTable, 0, &HandleEntry);
    ok(Valid == FALSE, "Valid = %u\n", Valid);
    ok(HandleEntry == InvalidPointer, "HandleEntry = %p\n", HandleEntry);

    TestEntry->HandleEntry.Flags = RTL_HANDLE_VALID;
    Valid = RtlIsValidHandle(&HandleTable, &TestEntry->HandleEntry);
    ok(Valid == TRUE, "Valid = %u\n", Valid);
    HandleEntry = InvalidPointer;
    Valid = RtlIsValidIndexHandle(&HandleTable, 0, &HandleEntry);
    ok(Valid == TRUE, "Valid = %u\n", Valid);
    ok(HandleEntry == &TestEntry->HandleEntry, "HandleEntry = %p\n", HandleEntry);

    /* Allocate a second handle */
    HandleEntry = RtlAllocateHandle(&HandleTable, &Index);

    ok((PUCHAR)HandleTable.CommittedHandles == HandleBase, "CommittedHandles = %p\n", HandleTable.CommittedHandles);
    ok((PUCHAR)HandleTable.UnCommittedHandles == HandleBase + PAGE_SIZE, "UnCommittedHandles = %p\n", HandleTable.UnCommittedHandles);
    ok((PUCHAR)HandleTable.MaxReservedHandles == HandleBase + MaxHandles * sizeof(TEST_HANDLE_ENTRY), "MaxReservedHandles = %p\n", HandleTable.MaxReservedHandles);
    ok((PUCHAR)HandleTable.FreeHandles == HandleBase + 2 * sizeof(TEST_HANDLE_ENTRY), "FreeHandles = %p\n", HandleTable.FreeHandles);

    ok((PUCHAR)HandleEntry == HandleBase + sizeof(TEST_HANDLE_ENTRY), "HandleEntry = %p\n", HandleEntry);
    ok(Index == 1, "Index = %lu\n", Index);

    TestEntry2 = (PTEST_HANDLE_ENTRY)HandleEntry;
    ok(TestEntry2->Data == 0, "Data = %lu\n", TestEntry2->Data);
    TestEntry2->Data = 0x87604321;

    TestEntry2->HandleEntry.Flags = RTL_HANDLE_VALID;
    Valid = RtlIsValidHandle(&HandleTable, &TestEntry2->HandleEntry);
    ok(Valid == TRUE, "Valid = %u\n", Valid);
    HandleEntry = NULL;
    Valid = RtlIsValidIndexHandle(&HandleTable, 1, &HandleEntry);
    ok(Valid == TRUE, "Valid = %u\n", Valid);
    ok(HandleEntry == &TestEntry2->HandleEntry, "HandleEntry = %p\n", HandleEntry);

    /* Free the first and allocate another */
    Valid = RtlFreeHandle(&HandleTable, &TestEntry->HandleEntry);
    ok(Valid == TRUE, "Valid = %u\n", Valid);

    HandleEntry = RtlAllocateHandle(&HandleTable, &Index);
    ok((PUCHAR)HandleEntry == HandleBase, "HandleEntry = %p\n", HandleEntry);
    ok(Index == 0, "Index = %lu\n", Index);
    ok(HandleEntry->Flags == 0, "Flags = 0x%lx\n", HandleEntry->Flags);

    TestEntry = (PTEST_HANDLE_ENTRY)HandleEntry;
    ok(TestEntry->Data == 0, "Data = %lu\n", TestEntry->Data);
    TestEntry->Data = 0x87650321;

    Valid = RtlFreeHandle(&HandleTable, &TestEntry2->HandleEntry);
    ok(Valid == TRUE, "Valid = %u\n", Valid);
    TestEntry->HandleEntry.Flags = RTL_HANDLE_VALID;
    Valid = RtlFreeHandle(&HandleTable, &TestEntry->HandleEntry);
    ok(Valid == TRUE, "Valid = %u\n", Valid);

    ok((PUCHAR)HandleTable.FreeHandles == HandleBase, "FreeHandles = %p\n", HandleTable.FreeHandles);

    /* Allocate all possible handles */
    for (i = 0; i < MaxHandles; i++)
    {
        const ULONG EntriesPerPage = PAGE_SIZE / sizeof(TEST_HANDLE_ENTRY);

        HandleEntry = RtlAllocateHandle(&HandleTable, &Index);
        ok(Index == i, "[%lu] Index = %lu\n", i, Index);
        ok((PUCHAR)HandleEntry == HandleBase + i * sizeof(TEST_HANDLE_ENTRY),
           "[%lu] HandleEntry = %p\n", i, HandleEntry);

        ok((PUCHAR)HandleTable.CommittedHandles == HandleBase, "[%lu] CommittedHandles = %p\n", i, HandleTable.CommittedHandles);
        ok((PUCHAR)HandleTable.UnCommittedHandles == HandleBase + PAGE_SIZE * (i / EntriesPerPage + 1) , "[%lu] UnCommittedHandles = %p\n", i, HandleTable.UnCommittedHandles);
        ok((PUCHAR)HandleTable.MaxReservedHandles == HandleBase + MaxHandles * sizeof(TEST_HANDLE_ENTRY), "[%lu] MaxReservedHandles = %p\n", i, HandleTable.MaxReservedHandles);
        if ((i + 1) % EntriesPerPage == 0)
        {
            ok(HandleTable.FreeHandles == NULL, "[%lu] FreeHandles = %p\n", i, HandleTable.FreeHandles);
        }
        else
        {
            ok((PUCHAR)HandleTable.FreeHandles == HandleBase + (i + 1) * sizeof(TEST_HANDLE_ENTRY), "[%lu] FreeHandles = %p\n", i, HandleTable.FreeHandles);
        }

        TestEntry = (PTEST_HANDLE_ENTRY)HandleEntry;
        ok(TestEntry->Data == 0, "[%lu] Data = 0x%lx\n", i, TestEntry->Data);
        TestEntry->Data = (i << 16) + (i + 1);

        Valid = RtlIsValidHandle(&HandleTable, &TestEntry->HandleEntry);
        ok(Valid == FALSE, "[%lu] Valid = %u\n", i, Valid);
        HandleEntry = InvalidPointer;
        Valid = RtlIsValidIndexHandle(&HandleTable, i, &HandleEntry);
        ok(Valid == FALSE, "[%lu] Valid = %u\n", i, Valid);
        ok(HandleEntry == InvalidPointer, "[%lu] HandleEntry = %p\n", i, HandleEntry);
    }

    /* Try one more */
    Index = 0x55555555;
    HandleEntry = RtlAllocateHandle(&HandleTable, &Index);
    ok(HandleEntry == NULL, "HandleEntry = %p\n", HandleEntry);
    ok(Index == 0x55555555, "Index = 0x%lx\n", Index);

    /* Free them all */
    for (i = 0; i < MaxHandles; i++)
    {
        TestEntry = (PTEST_HANDLE_ENTRY)HandleBase + i;

        ok(TestEntry->Data == (i << 16) + (i + 1), "[%lu] Data = %lu\n", i, TestEntry->Data);

        TestEntry->HandleEntry.Flags = RTL_HANDLE_VALID;

        Valid = RtlIsValidHandle(&HandleTable, &TestEntry->HandleEntry);
        ok(Valid == TRUE, "[%lu] Valid = %u\n", i, Valid);
        HandleEntry = InvalidPointer;
        Valid = RtlIsValidIndexHandle(&HandleTable, i, &HandleEntry);
        ok(Valid == TRUE, "[%lu] Valid = %u\n", i, Valid);
        ok(HandleEntry == &TestEntry->HandleEntry, "[%lu] HandleEntry = %p\n", i, HandleEntry);

        Valid = RtlFreeHandle(&HandleTable, &TestEntry->HandleEntry);
        ok(Valid == TRUE, "[%lu] Valid = %u\n", i, Valid);

        Valid = RtlIsValidHandle(&HandleTable, &TestEntry->HandleEntry);
        ok(Valid == FALSE, "[%lu] Valid = %u\n", i, Valid);
        HandleEntry = InvalidPointer;
        Valid = RtlIsValidIndexHandle(&HandleTable, i, &HandleEntry);
        ok(Valid == FALSE, "[%lu] Valid = %u\n", i, Valid);
        ok(HandleEntry == InvalidPointer, "[%lu] HandleEntry = %p\n", i, HandleEntry);
    }

    /* Check the memory commit once again */
    ok((PUCHAR)HandleTable.CommittedHandles == HandleBase, "[%lu] CommittedHandles = %p\n", i, HandleTable.CommittedHandles);
    ok((PUCHAR)HandleTable.UnCommittedHandles == HandleBase + MaxHandles * sizeof(TEST_HANDLE_ENTRY), "[%lu] UnCommittedHandles = %p\n", i, HandleTable.UnCommittedHandles);
    ok((PUCHAR)HandleTable.MaxReservedHandles == HandleBase + MaxHandles * sizeof(TEST_HANDLE_ENTRY), "[%lu] MaxReservedHandles = %p\n", i, HandleTable.MaxReservedHandles);
    ok((PUCHAR)HandleTable.FreeHandles == HandleBase + (i - 1) * sizeof(TEST_HANDLE_ENTRY), "[%lu] FreeHandles = %p\n", i, HandleTable.FreeHandles);

    /* Finally, destroy the table */
    RtlDestroyHandleTable(&HandleTable);
    ok((PUCHAR)HandleTable.CommittedHandles == HandleBase, "CommittedHandles = %p\n", HandleTable.CommittedHandles);
}
