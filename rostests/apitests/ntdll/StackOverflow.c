/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for stack overflow
 * PROGRAMMER:      Jérôme Gardou
 */

#define WIN32_NO_STATUS
#include <apitest.h>
#include <stdio.h>
#include <ndk/rtlfuncs.h>
#include <ndk/mmfuncs.h>

static int iteration = 0;
static PVOID StackAllocationBase;
static PVOID LastStackAllocation;
static ULONG StackSize;

static
void
infinite_recursive(void)
{
    MEMORY_BASIC_INFORMATION MemoryBasicInfo;
    NTSTATUS Status;
    char Buffer[0x500];

    sprintf(Buffer, "Iteration %d.\n", iteration++);

    Status = NtQueryVirtualMemory(
        NtCurrentProcess(),
        &Buffer[0],
        MemoryBasicInformation,
        &MemoryBasicInfo,
        sizeof(MemoryBasicInfo),
        NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    /* This never changes */
    ok_ptr(MemoryBasicInfo.AllocationBase, StackAllocationBase);
    /* Stack is committed one page at a time */
    ok_ptr(MemoryBasicInfo.BaseAddress, (PVOID)PAGE_ROUND_DOWN(&Buffer[0]));
    /* This is the protection of the memory when it was reserved. */
    ok_long(MemoryBasicInfo.AllocationProtect, PAGE_READWRITE);
    /* Windows commits the whole used stack at once, +2 pages. */
#if 0
    ok_long(MemoryBasicInfo.RegionSize, ((ULONG_PTR)StackAllocationBase + StackSize + 2* PAGE_SIZE) - PAGE_ROUND_DOWN(&Buffer[0]));
#endif
    /* This is the state of the queried address */
    ok_long(MemoryBasicInfo.State, MEM_COMMIT);
    /* This is the protection of the queried address */
    ok_long(MemoryBasicInfo.Protect, PAGE_READWRITE);
    /* Of course this is private memory. */
    ok_long(MemoryBasicInfo.Type, MEM_PRIVATE);

    LastStackAllocation = &Buffer[-0x500];

    infinite_recursive();
}

START_TEST(StackOverflow)
{
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemoryBasicInfo;

    /* Get the base of the stack */
    Status = NtQueryVirtualMemory(
        NtCurrentProcess(),
        &Status,
        MemoryBasicInformation,
        &MemoryBasicInfo,
        sizeof(MemoryBasicInfo),
        NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);
    StackAllocationBase = MemoryBasicInfo.AllocationBase;
    trace("Stack allocation base is %p.\n", StackAllocationBase);
    StackSize = MemoryBasicInfo.RegionSize;

    /* Check TEB attributes */
    ok_ptr(NtCurrentTeb()->DeallocationStack, StackAllocationBase);
    ok_ptr(NtCurrentTeb()->NtTib.StackBase, (PVOID)((ULONG_PTR)MemoryBasicInfo.BaseAddress + MemoryBasicInfo.RegionSize));
    ok_ptr(NtCurrentTeb()->NtTib.StackLimit, (PVOID)((ULONG_PTR)MemoryBasicInfo.BaseAddress - PAGE_SIZE));
    trace("Guaranteed stack size is %lu.\n", NtCurrentTeb()->GuaranteedStackBytes);

    /* Get its size */
    Status = NtQueryVirtualMemory(
        NtCurrentProcess(),
        StackAllocationBase,
        MemoryBasicInformation,
        &MemoryBasicInfo,
        sizeof(MemoryBasicInfo),
        NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* This is the complete stack size */
    StackSize += MemoryBasicInfo.RegionSize;
    trace("Stack size is 0x%lx.\n", StackSize);

    trace("Stack limit %p, stack base %p.\n", NtCurrentTeb()->NtTib.StackLimit, NtCurrentTeb()->NtTib.StackBase);

    /* Take a look at what is beyond the stack limit */
    Status = NtQueryVirtualMemory(
        NtCurrentProcess(),
        NtCurrentTeb()->NtTib.StackLimit,
        MemoryBasicInformation,
        &MemoryBasicInfo,
        sizeof(MemoryBasicInfo),
        NULL);
    ok_ntstatus(Status, STATUS_SUCCESS);

    ok_ptr(MemoryBasicInfo.BaseAddress, NtCurrentTeb()->NtTib.StackLimit);
    ok_ptr(MemoryBasicInfo.AllocationBase, StackAllocationBase);
    ok_long(MemoryBasicInfo.AllocationProtect, PAGE_READWRITE);
    ok_long(MemoryBasicInfo.RegionSize, 2 * PAGE_SIZE);
    ok_long(MemoryBasicInfo.State, MEM_COMMIT);
    ok_long(MemoryBasicInfo.Protect, PAGE_READWRITE);
    ok_long(MemoryBasicInfo.Type, MEM_PRIVATE);

    /* Accessing below stack limit is OK, as long as we don't starve the reserved space. */
    _SEH2_TRY
    {
        volatile CHAR* Pointer = (PVOID)((ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit - PAGE_SIZE / 2);
        CHAR Value = *Pointer;
        (void)Value;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_SUCCESS);

    _SEH2_TRY
    {
        infinite_recursive();
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        trace("Exception after %d iteration.\n", iteration);
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ok_ntstatus(Status, STATUS_STACK_OVERFLOW);

    /* Windows lets 2 pages between the reserved memory and the smallest possible stack address */
    ok((ULONG_PTR)LastStackAllocation > (ULONG_PTR)StackAllocationBase, "\n");
    ok_long(PAGE_ROUND_DOWN(LastStackAllocation), (ULONG_PTR)StackAllocationBase + 2 * PAGE_SIZE);

    /* And in fact, this is the true condition of the stack overflow */
    ok_ptr(NtCurrentTeb()->NtTib.StackLimit, (PVOID)((ULONG_PTR)StackAllocationBase + PAGE_SIZE));

    trace("Stack limit %p, stack base %p.\n", NtCurrentTeb()->NtTib.StackLimit, NtCurrentTeb()->NtTib.StackBase);

    /* Of course, accessing above the stack limit is OK. */
    _SEH2_TRY
    {
        volatile CHAR* Pointer = (PVOID)((ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit + PAGE_SIZE / 2);
        CHAR Value = *Pointer;
        (void)Value;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* But once stack is starved, it's starved. */
    _SEH2_TRY
    {
        volatile CHAR* Pointer = (PVOID)((ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit - PAGE_SIZE / 2);
        CHAR Value = *Pointer;
        (void)Value;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        trace("Exception after %d iteration.\n", iteration);
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    ok_ntstatus(Status, STATUS_ACCESS_VIOLATION);
}
