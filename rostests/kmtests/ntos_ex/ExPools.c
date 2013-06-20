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
    Allocs = ExAllocatePoolWithTag(PagedPool, sizeof(Allocs) * AllocNumber, TAG_POOLTEST);

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

START_TEST(ExPools)
{
    PoolsTest();
    PoolsCorruption();
    TestPoolTags();
}
