/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Stress Test for virtual memory allocation
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define WIN32_NO_STATUS
#include <stdio.h>
#include <wine/test.h>
#include <ndk/rtlfuncs.h>
#include <ndk/mmfuncs.h>

static PVOID Allocations[4096] = { NULL };
static ULONG CurrentAllocation = 0;

static
VOID
ValidateAllocations(VOID)
{
    ULONG i;

    ASSERT(CurrentAllocation < sizeof(Allocations) / sizeof(Allocations[0]));
    for (i = 0; i < CurrentAllocation; ++i)
    {
        PUCHAR UserBuffer = Allocations[i];
        SIZE_T AllocationSize;
        SIZE_T DataSize;

        if (UserBuffer == NULL)
            continue;

        AllocationSize = ((PSIZE_T)UserBuffer)[-2];
        DataSize = ((PSIZE_T)UserBuffer)[-1];
        ASSERT(DataSize != 0);
        ASSERT(((SIZE_T)UserBuffer + DataSize) % PAGE_SIZE == 0);
    }
}

static
PVOID
Allocate(
    SIZE_T DataSize)
{
    NTSTATUS Status;
    PVOID AllocationStart = NULL;
    SIZE_T AllocationSize = PAGE_ROUND_UP(DataSize + PAGE_SIZE + 2 * sizeof(SIZE_T));
    PVOID FirstPageStart;
    SIZE_T NumberOfPages = AllocationSize / PAGE_SIZE;
    SIZE_T Size;
    PUCHAR UserBuffer;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &AllocationStart, 0, &AllocationSize, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    FirstPageStart = (PUCHAR)AllocationStart + AllocationSize - PAGE_SIZE * NumberOfPages;
    Size = (NumberOfPages - 1) * PAGE_SIZE;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &FirstPageStart, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        AllocationSize = 0;
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &AllocationStart, &AllocationSize, MEM_RELEASE);
        ASSERT(Status == STATUS_SUCCESS);
        return NULL;
    }
    ASSERT(Size % sizeof(ULONG) == 0);
    ASSERT(RtlCompareMemoryUlong(FirstPageStart, Size, 0) == Size);

    UserBuffer = AllocationStart;
    UserBuffer += AllocationSize - PAGE_SIZE - DataSize;
    RtlFillMemory(FirstPageStart, UserBuffer - (PUCHAR)FirstPageStart, 0xae);
    RtlZeroMemory(UserBuffer, DataSize);
    ((PSIZE_T)UserBuffer)[-2] = AllocationSize;
    ((PSIZE_T)UserBuffer)[-1] = DataSize;

    Allocations[CurrentAllocation++] = UserBuffer;
    ValidateAllocations();
    return UserBuffer;
}

static
VOID
Free(
    PVOID UserBuffer)
{
    NTSTATUS Status;
    PVOID AllocationStart;
    SIZE_T Zero = 0;
    SIZE_T AllocationSize;
    SIZE_T DataSize;
    ULONG i;

    AllocationSize = ((PSIZE_T)UserBuffer)[-2];
    DataSize = ((PSIZE_T)UserBuffer)[-1];
    ASSERT(DataSize != 0);

    AllocationStart = (PUCHAR)UserBuffer + DataSize + PAGE_SIZE - AllocationSize;
    ASSERT((SIZE_T)AllocationStart % PAGE_SIZE == 0);

    RtlFillMemory(UserBuffer, DataSize, 0xbe);
    ((PSIZE_T)UserBuffer)[-1] = 0;
    ((PSIZE_T)UserBuffer)[-2] = 0xFAFBFCFD;

    for (i = 0; i < CurrentAllocation; ++i)
        if (Allocations[i] == UserBuffer)
        {
            Allocations[i] = NULL;
            break;
        }
    ValidateAllocations();

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &AllocationStart, &Zero, MEM_RELEASE);
    ASSERT(Status == STATUS_SUCCESS);
}

static
PVOID
ReAllocate(
    PVOID OldUserBuffer,
    SIZE_T NewDataSize)
{
    PVOID NewUserBuffer;
    SIZE_T OldDataSize;

    OldDataSize = ((PSIZE_T)OldUserBuffer)[-1];
    ASSERT(OldDataSize != 0);

    NewUserBuffer = Allocate(NewDataSize);
    ASSERT(((PSIZE_T)OldUserBuffer)[-1] == OldDataSize);
    RtlCopyMemory(NewUserBuffer, OldUserBuffer, min(OldDataSize, NewDataSize));
    ASSERT(((PSIZE_T)OldUserBuffer)[-1] == OldDataSize);
    Free(OldUserBuffer);
    return NewUserBuffer;
}

static
VOID
AccessMemory1(
    PVOID UserBuffer,
    SIZE_T DataSize)
{
    PBYTE Buffer = UserBuffer;
    SIZE_T i;

    for (i = 0; i < DataSize; ++i)
        Buffer[i] = LOBYTE(i);
}

static
BOOLEAN
CheckMemory1(
    PVOID UserBuffer,
    SIZE_T DataSize)
{
    PBYTE Buffer = UserBuffer;
    SIZE_T i;

    for (i = 0; i < DataSize; ++i)
        if (Buffer[i] != LOBYTE(i))
        {
            trace("Mismatch in region %p at index %lu. Value=%02x\n", UserBuffer, (ULONG)i, Buffer[i]);
            ASSERT(FALSE);
            return FALSE;
        }
    return TRUE;
}

static
VOID
AccessMemory2(
    PVOID UserBuffer,
    SIZE_T DataSize)
{
    PBYTE Buffer = UserBuffer;
    SIZE_T i;

    for (i = 0; i < DataSize; ++i)
        Buffer[i] = UCHAR_MAX - LOBYTE(i);
}

static
BOOLEAN
CheckMemory2(
    PVOID UserBuffer,
    SIZE_T DataSize)
{
    PBYTE Buffer = UserBuffer;
    SIZE_T i;

    for (i = 0; i < DataSize; ++i)
        if (Buffer[i] != UCHAR_MAX - LOBYTE(i))
        {
            trace("Mismatch in region %p at index %lu. Value=%02x\n", UserBuffer, (ULONG)i, Buffer[i]);
            ASSERT(FALSE);
            return FALSE;
        }
    return TRUE;
}

#define RUNS 32

START_TEST(NtAllocateVirtualMemory)
{
    PVOID Mem1, Mem2;
    SIZE_T Size1, Size2;
    ULONG i;

    Size1 = 32;
    Mem1 = Allocate(Size1);
    AccessMemory1(Mem1, Size1);
    Size2 = 128;
    Mem2 = Allocate(Size2);
    AccessMemory2(Mem2, Size2);
    for (i = 0; i < RUNS; ++i)
    {
        PVOID New;
        ok(CheckMemory1(Mem1, Size1) == TRUE, "CheckMemory1 failure\n");
        New = ReAllocate(Mem1, Size1 * 3 / 2);
        if (New == NULL)
        {
            skip("Realloc failure\n");
            break;
        }
        Mem1 = New;
        ok(CheckMemory1(Mem1, Size1) == TRUE, "CheckMemory1 failure\n");
        Size1 = Size1 * 3 / 2;
        AccessMemory1(Mem1, Size1);

        ok(CheckMemory2(Mem2, Size2) == TRUE, "CheckMemory2 failure\n");
        New = ReAllocate(Mem2, Size2 + 128);
        if (New == NULL)
        {
            skip("Realloc failure\n");
            break;
        }
        Mem2 = New;
        ok(CheckMemory2(Mem2, Size2) == TRUE, "CheckMemory2 failure\n");
        Size2 += 128;
        AccessMemory2(Mem2, Size2);
    }
    ok(CheckMemory2(Mem2, Size2) == TRUE, "CheckMemory2 failure\n");
    Free(Mem2);
    ok(CheckMemory1(Mem1, Size1) == TRUE, "CheckMemory1 failure\n");
    Free(Mem1);
}
