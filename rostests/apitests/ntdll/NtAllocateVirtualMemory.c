/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Stress Test for virtual memory allocation
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
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
        ASSERT(AllocationSize != 0);
        ASSERT(AllocationSize % PAGE_SIZE == 0);
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

VOID
CheckSize(ULONG_PTR Base, SIZE_T InSize, SIZE_T ExpectedSize)
{
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T Size;

    /* Reserve memory */
    BaseAddress = (PVOID)Base;
    Size = InSize;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_NOACCESS);
    ok(NT_SUCCESS(Status), "NtAllocateVirtualMemory failed!\n");
    ok(BaseAddress == (PVOID)(Base & ~((ULONG_PTR)0xFFFF)), "Got back wrong base address: %p\n", BaseAddress);
    ok(Size == ExpectedSize, "Alloc of 0x%Ix: got back wrong size: 0x%Ix, expected 0x%Ix\n", InSize, Size, ExpectedSize);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok(NT_SUCCESS(Status), "NtFreeVirtualMemory failed!\n");
}

VOID
CheckAlignment()
{
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T Size;

    CheckSize(0x50000000, 0x0001, 0x1000);
    CheckSize(0x50008000, 0x0001, 0x9000);
    CheckSize(0x50000010, 0x1000, 0x2000);
    CheckSize(0x50010000, 0x2000, 0x2000);
    CheckSize(0x5000FFFF, 0x3000, 0x13000);
    CheckSize(0x50001010, 0x7000, 0x9000);
    CheckSize(0x50001010, 0xC000, 0xe000);

    /* Reserve memory not aligned to allocation granularity */
    BaseAddress = UlongToPtr(0x50001010);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress == UlongToPtr(0x50000000), "Got back wrong base address: %p", BaseAddress);
    ok(Size == 0x3000, "Got back wrong size: 0x%Ix", Size);

    /* Try to reserve again in the same 64k region */
    BaseAddress = UlongToPtr(0x50008000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

    /* Commit memory */
    BaseAddress = UlongToPtr(0x50002000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress == UlongToPtr(0x50002000), "Got back wrong base address: %p", BaseAddress);
    ok(Size == 0x1000, "Got back wrong size: 0x%Ix", Size);

    /* Commit the same address again with a different protection */
    BaseAddress = UlongToPtr(0x50002000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress == UlongToPtr(0x50002000), "Got back wrong base address: %p", BaseAddress);
    ok(Size == 0x1000, "Got back wrong size: 0x%Ix", Size);

    /* Commit memory at a too high address */
    BaseAddress = UlongToPtr(0x50003000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

    /* Decommit the memory, even those pages that were not committed */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x3000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_DECOMMIT);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to release memory in a different 64k region */
    BaseAddress = UlongToPtr(0x50010000);
    Size = 0x1000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_MEMORY_NOT_ALLOCATED);

    /* Release the memory in the same 64k region at a different address */
    BaseAddress = UlongToPtr(0x50008000);
    Size = 0x1000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_MEMORY_NOT_ALLOCATED);

    /* Release the memory at the correct address but with wrong size */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x4000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_UNABLE_TO_FREE_VM);

    /* Release the memory */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x3000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Reserve and commit at once */
    BaseAddress = UlongToPtr(0x50004080);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress == UlongToPtr(0x50000000), "Got back wrong base address: %p", BaseAddress);
    ok(Size == 0x6000, "Got back wrong size: 0x%Ix", Size);

    _SEH2_TRY
    {
        *(int*)BaseAddress = 1;
        *(int*)UlongToPtr(0x50004080) = 1;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "Got exception\n");
    }
    _SEH2_END;

    /* Release the memory */
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

}

static
VOID
CheckAdjacentVADs()
{
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T Size;
    MEMORY_BASIC_INFORMATION MemoryBasicInfo;

    /* Reserve a full 64k region */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x10000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return;

    /* Reserve another 64k region, but with 64k between */
    BaseAddress = UlongToPtr(0x50020000);
    Size = 0x10000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return;

    /* Try to free the whole at once */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x30000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_UNABLE_TO_FREE_VM);

    /* Reserve the part in the middle */
    BaseAddress = UlongToPtr(0x50010000);
    Size = 0x10000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to commit memory covering 2 allocations */
    BaseAddress = UlongToPtr(0x50004000);
    Size = 0x10000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

    /* Commit a page */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Commit another page */
    BaseAddress = UlongToPtr(0x50002000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);

    _SEH2_TRY
    {
        *(int*)UlongToPtr(0x50000000) = 1;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ok(0, "Got exception\n");
    }
    _SEH2_END;

    _SEH2_TRY
    {
        (void)*(volatile int*)UlongToPtr(0x50002000);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Allocate 3 pages, on top of the previous 2 */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x3000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READONLY);
    ok_ntstatus(Status, STATUS_SUCCESS);

    _SEH2_TRY
    {
        *(int*)UlongToPtr(0x50000000) = 1;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);

    /* Commit a page at the end of the first region */
    BaseAddress = UlongToPtr(0x5000F000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_ptr(BaseAddress, UlongToPtr(0x5000F000));

    /* See where is the base of this newly committed area
     * (choose a base address in the middle of it) */
    Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                  UlongToPtr(0x5000F700),
                                  MemoryBasicInformation,
                                  &MemoryBasicInfo,
                                  sizeof(MemoryBasicInfo),
                                  NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* The base address is the beginning of the committed area */
    ok_ptr(MemoryBasicInfo.BaseAddress, UlongToPtr(0x5000F000));
    /* The allocation base address is the beginning of the whole region */
    ok_ptr(MemoryBasicInfo.AllocationBase, UlongToPtr(0x50000000));
    /* This is the protection of the memory when it was reserved. */
    ok_long(MemoryBasicInfo.AllocationProtect, PAGE_NOACCESS);
    /* This is the size of the committed region. (ie, smallest chunk size) */
    ok_long(MemoryBasicInfo.RegionSize, 0x1000);
    /* This is the state of the queried address */
    ok_long(MemoryBasicInfo.State, MEM_COMMIT);
    /* This is the protection of the queried address */
    ok_long(MemoryBasicInfo.Protect, PAGE_READWRITE);
    /* NtAllocateVirtualMemory makes it MEM_PRIVATE */
    ok_long(MemoryBasicInfo.Type, MEM_PRIVATE);

    /* Try to free the whole region at once */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x30000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_UNABLE_TO_FREE_VM);

    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x10000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    BaseAddress = UlongToPtr(0x50010000);
    Size = 0x10000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    BaseAddress = UlongToPtr(0x50020000);
    Size = 0x10000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Reserve 3 full 64k region */
    BaseAddress = UlongToPtr(0x50000000);
    Size = 0x30000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_NOACCESS);
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return;

    /* Release the 64k region in the middle */
    BaseAddress = UlongToPtr(0x50010000);
    Size = 0x10000;
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

}

#define RUNS 32

START_TEST(NtAllocateVirtualMemory)
{
    PVOID Mem1, Mem2;
    SIZE_T Size1, Size2;
    ULONG i;
    NTSTATUS Status;

    CheckAlignment();
    CheckAdjacentVADs();

    /* Reserve memory below 0x10000 */
    Mem1 = UlongToPtr(0xf000);
    Size1 = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Mem1,
                                     0,
                                     &Size1,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);

    /* Reserve memory at 0x10000 */
    Mem1 = UlongToPtr(0x10000);
    Size1 = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &Mem1,
                                     0,
                                     &Size1,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);

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
