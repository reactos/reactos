/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Pools test routines KM-Test
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define TAG_POOLTEST 'tstP'

#define BASE_POOL_TYPE_MASK 1
#define QUOTA_POOL_MASK 8

static
LONG
GetRefCount(
    _In_ PVOID Object)
{
    POBJECT_HEADER Header = OBJECT_TO_OBJECT_HEADER(Object);
    return Header->PointerCount;
}

static VOID PoolsTest(VOID)
{
    PVOID Ptr;
    ULONG AllocSize, i, AllocNumber;
    PVOID *Allocs;

    // Stress-test nonpaged pool
    for (i=1; i<10000; i++)
    {
        // make up some increasing, a bit irregular size
        AllocSize = i*10;

        if (i % 10)
            AllocSize++;

        if (i % 25)
            AllocSize += 13;

        // start with non-paged pool
        Ptr = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_POOLTEST);

        // it may fail due to no-memory condition
        if (!Ptr) break;

        // try to fully fill it
        RtlFillMemory(Ptr, AllocSize, 0xAB);

        // free it
        ExFreePoolWithTag(Ptr, TAG_POOLTEST);
    }

    // now paged one
    for (i=1; i<10000; i++)
    {
        // make up some increasing, a bit irregular size
        AllocSize = i*50;

        if (i % 10)
            AllocSize++;

        if (i % 25)
            AllocSize += 13;

        // start with non-paged pool
        Ptr = ExAllocatePoolWithTag(PagedPool, AllocSize, TAG_POOLTEST);

        // it may fail due to no-memory condition
        if (!Ptr) break;

        // try to fully fill it
        RtlFillMemory(Ptr, AllocSize, 0xAB);

        // free it
        ExFreePoolWithTag(Ptr, TAG_POOLTEST);
    }

    // test super-big allocations
    /*AllocSize = 2UL * 1024 * 1024 * 1024;
    Ptr = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_POOLTEST);
    ok(Ptr == NULL, "Allocating 2Gb of nonpaged pool should fail\n");

    Ptr = ExAllocatePoolWithTag(PagedPool, AllocSize, TAG_POOLTEST);
    ok(Ptr == NULL, "Allocating 2Gb of paged pool should fail\n");*/

    // now test allocating lots of small/medium blocks
    AllocNumber = 100000;
    Allocs = ExAllocatePoolWithTag(PagedPool, sizeof(*Allocs) * AllocNumber, TAG_POOLTEST);

    // alloc blocks
    for (i=0; i<AllocNumber; i++)
    {
        AllocSize = 42;
        Allocs[i] = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_POOLTEST);
    }

    // now free them
    for (i=0; i<AllocNumber; i++)
    {
        ExFreePoolWithTag(Allocs[i], TAG_POOLTEST);
    }


    ExFreePoolWithTag(Allocs, TAG_POOLTEST);
}

static VOID PoolsCorruption(VOID)
{
    PULONG Ptr;
    ULONG AllocSize;

    // start with non-paged pool
    AllocSize = 4096 + 0x10;
    Ptr = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_POOLTEST);

    // touch all bytes, it shouldn't cause an exception
    RtlZeroMemory(Ptr, AllocSize);

/* TODO: These fail because accessing invalid memory doesn't necessarily
         cause an access violation */
#ifdef THIS_DOESNT_WORK
    // test buffer overrun, right after our allocation ends
    _SEH2_TRY
    {
        TestPtr = (PULONG)((PUCHAR)Ptr + AllocSize);
        //Ptr[4] = 0xd33dbeef;
        *TestPtr = 0xd33dbeef;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the status */
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

    ok(Status == STATUS_ACCESS_VIOLATION, "Exception should occur, but got Status 0x%08lX\n", Status);

    // test overrun in a distant byte range, but within 4096KB
    _SEH2_TRY
    {
        Ptr[2020] = 0xdeadb33f;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the status */
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;

    ok(Status == STATUS_ACCESS_VIOLATION, "Exception should occur, but got Status 0x%08lX\n", Status);
#endif

    // free the pool
    ExFreePoolWithTag(Ptr, TAG_POOLTEST);
}

static
VOID
TestPoolTags(VOID)
{
    PVOID Memory;

    Memory = ExAllocatePoolWithTag(PagedPool, 8, 'MyTa');
    ok_eq_tag(KmtGetPoolTag(Memory), 'MyTa');
    ExFreePoolWithTag(Memory, 'MyTa');

    Memory = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, 'MyTa');
    ok_eq_tag(KmtGetPoolTag(Memory), 'TooL');
    ExFreePoolWithTag(Memory, 'MyTa');

    Memory = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE - 3 * sizeof(PVOID), 'MyTa');
    ok_eq_tag(KmtGetPoolTag(Memory), 'TooL');
    ExFreePoolWithTag(Memory, 'MyTa');

    Memory = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE - 4 * sizeof(PVOID) + 1, 'MyTa');
    ok_eq_tag(KmtGetPoolTag(Memory), 'TooL');
    ExFreePoolWithTag(Memory, 'MyTa');

    Memory = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE - 4 * sizeof(PVOID), 'MyTa');
    ok_eq_tag(KmtGetPoolTag(Memory), 'MyTa');
    ExFreePoolWithTag(Memory, 'MyTa');
}

static
VOID
TestPoolQuota(VOID)
{
    PEPROCESS Process = PsGetCurrentProcess();
    PEPROCESS StoredProcess;
    PVOID Memory;
    LONG InitialRefCount;
    LONG RefCount;
    USHORT PoolType;

    InitialRefCount = GetRefCount(Process);

    /* We get some memory from this function, and it's properly aligned.
     * Also, it takes a reference to the process, and releases it on free */
    Memory = ExAllocatePoolWithQuotaTag(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                        sizeof(LIST_ENTRY),
                                        'tQmK');
    ok(Memory != NULL, "ExAllocatePoolWithQuotaTag returned NULL\n");
    if (!skip(Memory != NULL, "No memory\n"))
    {
        ok((ULONG_PTR)Memory % sizeof(LIST_ENTRY) == 0,
           "Allocation %p is badly aligned\n",
           Memory);
        RefCount = GetRefCount(Process);
        ok_eq_long(RefCount, InitialRefCount + 1);

        /* A pointer to the process is found right before the next pool header */
        StoredProcess = ((PVOID *)((ULONG_PTR)Memory + 2 * sizeof(LIST_ENTRY)))[-1];
        ok_eq_pointer(StoredProcess, Process);

        /* Pool type should have QUOTA_POOL_MASK set */
        PoolType = KmtGetPoolType(Memory);
        ok(PoolType != 0, "PoolType is 0\n");
        PoolType--;
        ok(PoolType & QUOTA_POOL_MASK, "PoolType = %x\n", PoolType);
        ok((PoolType & BASE_POOL_TYPE_MASK) == PagedPool, "PoolType = %x\n", PoolType);

        ExFreePoolWithTag(Memory, 'tQmK');
        RefCount = GetRefCount(Process);
        ok_eq_long(RefCount, InitialRefCount);
    }

    /* Large allocations are page-aligned, don't reference the process */
    Memory = ExAllocatePoolWithQuotaTag(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                        PAGE_SIZE,
                                        'tQmK');
    ok(Memory != NULL, "ExAllocatePoolWithQuotaTag returned NULL\n");
    if (!skip(Memory != NULL, "No memory\n"))
    {
        ok((ULONG_PTR)Memory % PAGE_SIZE == 0,
           "Allocation %p is badly aligned\n",
           Memory);
        RefCount = GetRefCount(Process);
        ok_eq_long(RefCount, InitialRefCount);
        ExFreePoolWithTag(Memory, 'tQmK');
        RefCount = GetRefCount(Process);
        ok_eq_long(RefCount, InitialRefCount);
    }

    /* Function raises by default */
    KmtStartSeh()
        Memory = ExAllocatePoolWithQuotaTag(PagedPool,
                                            0x7FFFFFFF,
                                            'tQmK');
        if (Memory)
            ExFreePoolWithTag(Memory, 'tQmK');
    KmtEndSeh(STATUS_INSUFFICIENT_RESOURCES);

    /* Function returns NULL with POOL_QUOTA_FAIL_INSTEAD_OF_RAISE */
    KmtStartSeh()
        Memory = ExAllocatePoolWithQuotaTag(PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                            0x7FFFFFFF,
                                            'tQmK');
        ok(Memory == NULL, "Successfully got 2GB block: %p\n", Memory);
        if (Memory)
            ExFreePoolWithTag(Memory, 'tQmK');
    KmtEndSeh(STATUS_SUCCESS);
}

static
VOID
TestBigPoolExpansion(VOID)
{
    POOL_TYPE PoolType;
    PVOID *BigAllocations;
    const ULONG MaxAllocations = 1024 * 128;
    ULONG NumAllocations;

    for (PoolType = NonPagedPool; PoolType <= PagedPool; PoolType++)
    {
        BigAllocations = ExAllocatePoolWithTag(PoolType,
                                               MaxAllocations * sizeof(*BigAllocations),
                                               'ABmK');

        /* Allocate a lot of pages (== big pool allocations) */
        for (NumAllocations = 0; NumAllocations < MaxAllocations; NumAllocations++)
        {
            BigAllocations[NumAllocations] = ExAllocatePoolWithTag(PoolType,
                                                                   PAGE_SIZE,
                                                                   'aPmK');
            if (BigAllocations[NumAllocations] == NULL)
            {
                NumAllocations--;
                break;
            }
        }

        trace("Got %lu allocations for PoolType %d\n", NumAllocations, PoolType);

        /* Free them */
        for (; NumAllocations < MaxAllocations; NumAllocations--)
        {
            ASSERT(BigAllocations[NumAllocations] != NULL);
            ExFreePoolWithTag(BigAllocations[NumAllocations],
                              'aPmK');
        }
        ExFreePoolWithTag(BigAllocations, 'ABmK');
    }
}

START_TEST(ExPools)
{
    PoolsTest();
    PoolsCorruption();
    TestPoolTags();
    TestPoolQuota();
    TestBigPoolExpansion();
}
