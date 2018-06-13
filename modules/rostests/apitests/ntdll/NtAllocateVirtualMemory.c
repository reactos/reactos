/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for NtAllocateVirtualMemory
 * COPYRIGHT:   Copyright 2011-2013 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2013-2014 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2015 Jérôme Gardou <jerome.gardou@reactos.org>
 *              Copyright 2018 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include "precomp.h"

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
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress == (PVOID)(Base & ~((ULONG_PTR)0xFFFF)), "Got back wrong base address: %p\n", BaseAddress);
    ok(Size == ExpectedSize, "Alloc of 0x%Ix: got back wrong size: 0x%Ix, expected 0x%Ix\n", InSize, Size, ExpectedSize);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);
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
    // Result varies between ReactOS and Windows.
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    { // Unexpected, cleanup.
        trace("ReactOS does not support \"case B\" yet\n");
        ok_ntstatus(Status, STATUS_FREE_VM_NOT_AT_BASE);
        // Release the 3 64k region.
        BaseAddress = UlongToPtr(0x50000000);
        Size = 0x30000;
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }
    else
    {
        trace("Windows supports \"case B\"\n");
        // Double-check that the 64k region in the middle was released.
        BaseAddress = UlongToPtr(0x50010000);
        Size = 0x10000;
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_MEMORY_NOT_ALLOCATED);
        // Release the 64k region at the start.
        BaseAddress = UlongToPtr(0x50000000);
        Size = 0x10000;
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
        // Release the 64k region at the end.
        BaseAddress = UlongToPtr(0x50020000);
        Size = 0x10000;
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }
}

static
VOID
CheckSomeDefaultAddresses(VOID)
{
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T Size;

    // NULL.

    /* Reserve memory dynamically, not at 0x00000000 */
    BaseAddress = NULL;
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok(BaseAddress != 0x00000000, "Unexpected BaseAddress = 0x00000000\n");
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    // Suffix meanings: nothing = x32 and x64, x32 = 32on32, x64 = 32on64, amd64 = 64on64.
    // The following checks assume very default addresses,
    // no address space layout randomization (ASLR).
    // PS: Actually, it looks like ASLR is active on 7+_x32/Vista+_x64/Vista+_amd64 test VMs...

    // 0x00000000: First 64k region.

    /* Reserve and commit memory at 0x00000000, after round down */
    BaseAddress = UlongToPtr(0x00000000 + 0x0FFF);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    // Result varies between Windows versions.
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        trace("0x00000000(+) is available, as on ReactOS and Windows XP/S03/Vista_x32/7_x32\n");
        // 0x00000000, 64k: Free.
    }
    else
    {
        trace("0x00000000(+) is not available, as on Windows 8.1_x32/Vista+_x64/Vista+_amd64\n");
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER_2);
        skip("Following tests on first 64k region would fail too\n");
        goto First64kIsUnavailable;
    }

    ok_ptr(BaseAddress, 0x00000000);

    // Double-check that it is not forbidden "in order to catch null pointer accesses".
    StartSeh()
        *(int*)UlongToPtr(0x00000000) = 1;
    EndSeh(STATUS_SUCCESS)

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Reserve memory above 0x00000000 */
    BaseAddress = UlongToPtr(0x00000000 + 0x1000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    // 0x00010000: Second 64k region.

    /* Reserve memory below 0x00010000 */
    BaseAddress = UlongToPtr(0x00010000 - 0x1000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

First64kIsUnavailable:
    /* Reserve memory at 0x00010000:
     * Windows NT legacy default executable image base */
    BaseAddress = UlongToPtr(0x00010000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    // Result varies between Windows versions.
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);
    if (!NT_SUCCESS(Status))
    {
        trace("0x00010000 is not available, as on ReactOS and Windows XP/S03/Vista/S08_x64/7/Vista_amd64/S08_amd64/7_amd64\n");
        // 0x00010000,  4k: Private Data.
        // 0x00011000, 60k: Unusable.
    }
    else
    {
        trace("0x00010000 is available, as on Windows 8.1/10_x64/8.1+_amd64\n");
        ok_ntstatus(Status, STATUS_SUCCESS);
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }

    // 0x00400000: Image base.

    /* Reserve memory below 0x00400000 */
    BaseAddress = UlongToPtr(0x00400000 - 0x1000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    // Result varies between Windows versions and sometimes between configurations/runs.
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        trace("Below 0x00400000 is available, as on ReactOS and Windows XP/S03/8.1+_amd64. (Windows 8.1/10_x64 vary.)\n");
        // ReactOS:
        // 0x003F0000, 64k: Free.
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }
    else
    {
        trace("Below 0x00400000 is not available, as on Windows Vista_amd64. (Windows Vista/S08_x64/7/S08_amd64/7_amd64 vary.)\n");
        // Windows XP on VirtualPC 2004:
        // 0x003F0000,  4k: Shareable.
        // 0x003F1000, 60k: Unusable.
        ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);
    }

    /* Reserve memory at 0x00400000:
     * Windows NT legacy default DLL image base,
     * (ReactOS and) Windows 95 new default executable image base */
    BaseAddress = UlongToPtr(0x00400000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    // Result varies between Windows versions and sometimes between configurations/runs.
    ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);
    if (!NT_SUCCESS(Status))
    {
        trace("0x00400000 is not available, as on ReactOS and Windows XP/S03/Vista_amd64. (Windows Vista/S08_x64/7/S08_amd64/7_amd64 vary.)\n");
    }
    else
    {
        trace("0x00400000 is available. (Windows 8.1/10_x64/8.1+_amd64 vary.)\n");
        ok_ntstatus(Status, STATUS_SUCCESS);
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }

    // 0x10000000: Free.

    /* Reserve memory below 0x10000000 */
    BaseAddress = UlongToPtr(0x10000000 - 0x1000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Reserve memory at 0x10000000:
     * Windows new default non-OS DLL image base */
    BaseAddress = UlongToPtr(0x10000000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

#ifdef _WIN64
    // 0x0000000140000000: 64-bit Exe image base.

    // Reserve memory below 0x0000000140000000.
    BaseAddress = (PVOID) (0x0000000140000000 - 0x1000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    // Result varies sometimes between runs.
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        trace("Below 0x0140000000 is available, as on Windows S08_amd64/8.1+_amd64. (Windows Vista_amd64/7_amd64 vary.)\n");
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }
    else
    {
        trace("Below 0x0140000000 is not available\n");
        ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);
    }

    // Reserve memory at 0x0000000140000000:
    // Windows 64-bit default executable image base.
    BaseAddress = (PVOID) 0x0000000140000000;
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    // Result varies sometimes between runs.
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        trace("0x0140000000 is available, as on Windows S08+_amd64. (Windows Vista_amd64 varies.)\n");
        Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
        ok_ntstatus(Status, STATUS_SUCCESS);
    }
    else
    {
        trace("0x0140000000 is not available\n");
        ok_ntstatus(Status, STATUS_CONFLICTING_ADDRESSES);
    }

    // 0x0000000180000000: 64-bit DLL image base.

    // Reserve memory below 0x0000000180000000.
    BaseAddress = (PVOID) (0x0000000180000000 - 0x1000);
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);

    // Reserve memory at 0x0000000180000000:
    // Windows 64-bit default DLL image base.
    BaseAddress = (PVOID) 0x0000000180000000;
    Size = 0x1000;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &BaseAddress,
                                     0,
                                     &Size,
                                     MEM_RESERVE,
                                     PAGE_READWRITE);
    ok_ntstatus(Status, STATUS_SUCCESS);
    Status = NtFreeVirtualMemory(NtCurrentProcess(), &BaseAddress, &Size, MEM_RELEASE);
    ok_ntstatus(Status, STATUS_SUCCESS);
#endif
}

#define RUNS 32

START_TEST(NtAllocateVirtualMemory)
{
    PVOID Mem1, Mem2;
    SIZE_T Size1, Size2;
    ULONG i;

    CheckAlignment();
    CheckAdjacentVADs();
    CheckSomeDefaultAddresses();

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
