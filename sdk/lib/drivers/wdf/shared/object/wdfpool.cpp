/*++

Copyright (c) Microsoft Corporation

Module Name:

    wdfpool.c

Abstract:

    This module implements the driver frameworks pool routines.

Author:




Environment:

    Both kernel and user mode

Revision History:






--*/

#include "fxobjectpch.hpp"

// We use DoTraceMessage
extern "C" {

#if defined(EVENT_TRACING)
#include "wdfpool.tmh"
#endif

}

BOOLEAN
FxIsPagedPoolType(
    __in POOL_TYPE Type
    )
/*++

Routine Description:

    Return whether paged pool is specified by POOL_TYPE

Arguments:

    Type - POOL_TYPE

Returns:
    TRUE - Paged Pool,FALSE - Non-Paged Pool

--*/
{
    //
    // Cleaner than doing (Type & 0x01)
    //
    switch( Type & (~POOL_COLD_ALLOCATION) ) {
    case PagedPool:
    case PagedPoolCacheAligned:
        return TRUE;

    default:
        return FALSE;
    }
}


PVOID
FxPoolAllocator(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL  Pool,
    __in POOL_TYPE Type,
    __in SIZE_T    Size,
    __in ULONG     Tag,
    __in PVOID     Caller
    )
/*++

Routine Description:

    Allocates system pool tracked in a FX_POOL tracking object.

Arguments:

    Pool    - FX_POOL object for tracking allocations

    Type    - POOL_TYPE from ntddk.h

    Size    - Size in bytes of the allocation

    Tag     - Caller specified additional tag value for debugging/tracing

    Caller  - Caller's address

Returns:

    NULL - Could not allocate pool
    !NULL - Pointer to pool of minimum Size bytes

Remarks:

    In kernel mode this routine conditionally adds header on top iff the
    allocation size is < PAGE_SIZE. If the allocation size is >= PAGE_SIZE
    the caller would expect a page aligned pointer, hence no header is added.
    In addition, ExAllocatePool* functions guarantee that a buffer < PAGE_SIZE
    doesn't straddle page boundary. This allows FxPoolFree to determine whether
    a header is added to buffer or not based on whether the pointer passed in
    is page aligned or not. (In addition, when pool tracking is ON, this
    routine adds pool tracking header based on whether additional space for this
    header will push the buffer size beyond PAGE_SIZE, which is an optimization.)

    Such guarantees are not available with user mode allocator, hence in case
    of user mode we always add the header. (In user mode a buffer < PAGE_SIZE
    can straddle page boundary and the pointer returned may happen to be page
    aligned, causing FxPoolFree to free the wrong pointer.)

--*/
{
    PVOID ptr;
    PCHAR pTrueBase;
    PFX_POOL_TRACKER pTracker;
    PFX_POOL_HEADER pHeader;
    NTSTATUS status;
    SIZE_T allocationSize;


    ptr = NULL;

    //
    // Allocations of a zero request size are invalid.
    //
    // Besides, with a zero request size, special pool could place us
    // at the end of a page, and adding our header would give us a page
    // aligned address, which is ambiguous with large allocations.
    //
    if (Size == 0) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPOOL,
                            "Invalid Allocation Size of 0 requested");
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return NULL;
    }

    if (FxDriverGlobals->IsPoolTrackingOn()) {

        if (FxDriverGlobals->FxVerifierOn &&
            (FxDriverGlobals->WdfVerifierAllocateFailCount != 0xFFFFFFFF)) {

            //
            // If the registry key VerifierAllocateFailCount is set, all allocations
            // after the specified count are failed.
            //
            // This is a brutal test, but also ensures the framework can cleanup
            // under low memory conditions as well.
            //
            if (FxDriverGlobals->WdfVerifierAllocateFailCount == 0) {
                DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPOOL,
                                    "Allocation Fail Count exceeded");
                return NULL;
            }

            // Decrement the count
            InterlockedDecrement(&FxDriverGlobals->WdfVerifierAllocateFailCount);
        }

        //
        // (Kernel mode only) PAGE_SIZE or greater allocations can not have our
        // header since this would break the system allocators contract
        // that PAGE_SIZE or greater allocations start on a whole page boundary
        //

        //
        // For allocations less than a page size that will not fit with our
        // header, we round up to a non-tracked whole page allocation so
        // we don't burn two pages for this boundary condition.
        //

        // This if is the same as
        // Size + sizeof(FX_POOL_TRACKER) + FX_POOL_HEADER_SIZE >= PAGE_SIZE
        // BUT with no integer overflow
        if (Mx::IsKM() &&
            (Size >= PAGE_SIZE - sizeof(FX_POOL_TRACKER) - FX_POOL_HEADER_SIZE)
            ) {

            //
            // Ensure that we ask for at least a page to ensure the
            // allocation starts on a whole page.
            //
            if (Size < PAGE_SIZE) {
                Size = PAGE_SIZE;
            }

            ptr = MxMemory::MxAllocatePoolWithTag(Type, Size, Tag);

            //
            // The current system allocator returns paged aligned memory
            // in this case, which we rely on to detect whether our header
            // is present or not in FxPoolFree
            //
            ASSERT(((ULONG_PTR)ptr & (PAGE_SIZE-1)) == 0);
        }
        else {

            status = RtlSIZETAdd(Size,
                                 sizeof(FX_POOL_TRACKER) + FX_POOL_HEADER_SIZE,
                                 &allocationSize);

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPOOL,
                    "overflow: allocation tracker (%d) + header (%d) + pool "
                    "request (%I64d)", sizeof(FX_POOL_TRACKER),
                    FX_POOL_HEADER_SIZE, Size);

                return NULL;
            }

            pTrueBase = (PCHAR) MxMemory::MxAllocatePoolWithTag(
                Type,
                allocationSize,
                Tag
                );

            if (pTrueBase == NULL) {
                return NULL;
            }

            pTracker = (PFX_POOL_TRACKER) pTrueBase;
            pHeader  = WDF_PTR_ADD_OFFSET_TYPE(pTrueBase,
                                               sizeof(FX_POOL_TRACKER),
                                               PFX_POOL_HEADER);
            pHeader->Base            = pTrueBase;
            pHeader->FxDriverGlobals = FxDriverGlobals;

            //
            // Adjust the pointer to what we return to the driver
            //
            ptr = &pHeader->AllocationStart[0];

            //
            // Ensure the pointer we are returning is aligned on the proper
            // boundary.
            //
            ASSERT( ((ULONG_PTR) ptr & (MEMORY_ALLOCATION_ALIGNMENT-1)) == 0);

            //
            // Ensure the pointer is still not page aligned after
            // our adjustment. Otherwise the pool free code will
            // get confused and call ExFreePool on the wrong ptr.
            //
            if (Mx::IsKM()) {
                ASSERT(((ULONG_PTR)ptr & (PAGE_SIZE-1)) != 0 );
            }

            //
            // We must separate paged and non-paged pool since
            // the lock held differs as to whether we can accept
            // page faults and block in the allocator.
            //
            if (FxIsPagedPoolType(Type)) {
                //
                // Format and insert the Tracker in the PagedHeader list.
                //
                FxPoolInsertPagedAllocateTracker(Pool,
                                                 pTracker,
                                                 Size,
                                                 Tag,
                                                 Caller);
            }
            else {
                //
                // Format and insert the Tracker in the NonPagedHeader list.
                //
                FxPoolInsertNonPagedAllocateTracker(Pool,
                                                    pTracker,
                                                    Size,
                                                    Tag,
                                                    Caller);
            }
        }
    }
    else {
        //
        // No pool tracking...
        //

        if ((Size < PAGE_SIZE) || Mx::IsUM())
        {
            //
            // (Kernel mode only) See if adding our header promotes us past a
            // page boundary
            //
            status = RtlSIZETAdd(Size,
                                 FX_POOL_HEADER_SIZE,
                                 &allocationSize);

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPOOL,
                    "overflow: header + pool request (%I64d)", Size);

                return NULL;
            }

        }
        else {
            //
            // Is the raw request for alloc >= PAGE_SIZE ?  Then just use it.
            //
            allocationSize = Size;
        }

        //
        // Is cooked size for alloc >= PAGE_SIZE ?  Then just do it.
        //
        if (allocationSize >= PAGE_SIZE && Mx::IsKM())
        {
            //
            // Important to use allocationSize so that we get a page aligned
            // allocation so that we know to just free the memory pointer as is
            // when it is freed.
            //
            ptr = MxMemory::MxAllocatePoolWithTag(Type, allocationSize, Tag);
            ASSERT(((ULONG_PTR)ptr & (PAGE_SIZE-1)) == 0);
        }
        else {
            pTrueBase = (PCHAR) MxMemory::MxAllocatePoolWithTag(Type,
                                                      allocationSize,
                                                      Tag);

            if (pTrueBase != NULL) {

                pHeader = (PFX_POOL_HEADER) pTrueBase;
                pHeader->Base            = pTrueBase;
                pHeader->FxDriverGlobals = FxDriverGlobals;

                ptr = &pHeader->AllocationStart[0];

                if (Mx::IsKM()) {
                    //
                    // Ensure the pointer is still not page aligned after
                    // our adjustment. Otherwise the pool free code will
                    // get confused and call ExFreePool on the wrong ptr.
                    //
                    ASSERT( ((ULONG_PTR)ptr & (PAGE_SIZE-1)) != 0 );
                }
            }
        }
    }

    return ptr;
}

VOID
FxPoolFree(
    __in_xcount(ptr is at an offset from AllocationStart) PVOID ptr
    )
/*++

Routine Description:

    Release tracked pool

Arguments:

    Pool - FX_POOL object allocation is tracked in

    ptr - Pointer to pool to release

Returns:

Remarks:
    In kernel mode the pointer passed in may or may not have a header before
    it depending upon whether the pointer is page aligned or not.

    In user mode the pointer passed in always has a header before it. See
    remarks for FxPoolAllocator.

--*/
{
    PFX_POOL_HEADER pHeader;
    PVOID pTrueBase;
    PFX_POOL_TRACKER pTracker;

    //
    // Null pointers are always bad
    //
    if( ptr == NULL ) {
        ASSERTMSG("NULL pointer freed\n", FALSE);
        Mx::MxBugCheckEx(WDF_VIOLATION,
                     WDF_REQUIRED_PARAMETER_IS_NULL,
                     (ULONG_PTR)NULL,
                     (ULONG_PTR)_ReturnAddress(),
                     (ULONG_PTR)NULL
                     );
    }

    //
    // (Kernel mode only) If ptr is aligned on page boundry (indicates
    // it was > PAGE_SIZE allocation)
    // then there will be no common header...just free the memory without
    // further processing.
    //
    if( Mx::IsKM() && ( ((ULONG_PTR)ptr & (PAGE_SIZE-1)) == 0 ) ) {
        MxMemory::MxFreePool(ptr);
        return;
    }

    //
    // Ensure the pointer we are returning is aligned on the proper
    // boundary.
    //
    ASSERT( ((ULONG_PTR) ptr & (MEMORY_ALLOCATION_ALIGNMENT-1)) == 0);

    //
    // Dereference the Common header which all <PAGE_SIZE allcations will have.
    //
    pHeader = CONTAINING_RECORD(ptr, FX_POOL_HEADER, AllocationStart);
    pTrueBase = pHeader->Base;

    //
    // If PoolTracker is on then Base must point to it's header.
    // This is currently the only option for this area...may change later.
    //
    if (pHeader->FxDriverGlobals->IsPoolTrackingOn()) {

        pTracker = (PFX_POOL_TRACKER) pTrueBase;

        if (FxIsPagedPoolType(pTracker->PoolType)) {
            //
            // Decommission this Paged Allocation tracker
            //
            FxPoolRemovePagedAllocateTracker(pTracker);
        }
        else {
            //
            // Decommission this NonPaged Allocation tracker
            //
            FxPoolRemoveNonPagedAllocateTracker(pTracker);
        }

        //
        // Scrub the pool to zeros to catch destructed objects
        // by NULL'ing the v-table ptr
        //
        RtlZeroMemory(pTracker, pTracker->Size + sizeof(FX_POOL_TRACKER));
    }

    MxMemory::MxFreePool(pTrueBase);
}

NTSTATUS
FxPoolDump(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL  Pool
    )
/*++

Routine Description:

    Dump the FX_POOL tracking object

Arguments:

    Pool    - FX_POOL object for tracking allocations

Returns:

    STATUS_SUCCESS

--*/
{
    PFX_POOL_TRACKER pTracker;
    PLIST_ENTRY ple;
    KIRQL oldIrql;
    BOOLEAN leak;

    //
    // Dump usage information
    //
    DoTraceLevelMessage(
        FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
        "FxPoolDump: "
        "NonPagedBytes %I64d, PagedBytes %I64d, "
        "NonPagedAllocations %d, PagedAllocations %d,"
        "PeakNonPagedBytes %I64d, PeakPagedBytes %I64d,"
        "FxPoolDump: PeakNonPagedAllocations %d, PeakPagedAllocations %d",
        Pool->NonPagedBytes, Pool->PagedBytes,
        Pool->NonPagedAllocations, Pool->PagedAllocations,
        Pool->PeakNonPagedBytes, Pool->PeakPagedBytes,
        Pool->PeakNonPagedAllocations, Pool->PeakPagedAllocations
        );

    leak = FALSE;

    //
    // Check paged pool for leaks
    //
    Pool->PagedLock.Acquire();

    for (ple = Pool->PagedHead.Flink; ple != &Pool->PagedHead; ple = ple->Flink) {
        pTracker = CONTAINING_RECORD(ple, FX_POOL_TRACKER, Link);

        // Leaker
        leak = TRUE;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "FX_POOL 0x%p leaked paged memory alloc 0x%p (tracking block %p)",
            Pool, pTracker + 1, pTracker);
    }

    Pool->PagedLock.Release();

    //
    // Check non-paged pool for leaks
    //

    Pool->NonPagedLock.Acquire(&oldIrql);

    for (ple = Pool->NonPagedHead.Flink;
         ple != &Pool->NonPagedHead;
         ple = ple->Flink) {
        pTracker = CONTAINING_RECORD(ple, FX_POOL_TRACKER, Link );

        // Leaker
        leak = TRUE;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "FX_POOL 0x%p leaked non-paged memory alloc 0x%p (tracking block %p)",
            Pool, pTracker+1, pTracker);
    }

    Pool->NonPagedLock.Release(oldIrql);

    if (leak) {
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return STATUS_MORE_ENTRIES;
    }
    else {
        return STATUS_SUCCESS;
    }
}

_Must_inspect_result_
NTSTATUS
FxPoolInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL Pool
    )
/*++

Routine Description:
    Initialize the FX_POOL tracking object

Arguments:
    Pool    - FX_POOL object for tracking allocations

Returns:
    STATUS_SUCCESS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPOOL,
                        "Initializing Pool 0x%p, Tracking %d",
                        Pool, FxDriverGlobals->IsPoolTrackingOn());

    Pool->NonPagedLock.Initialize();

    InitializeListHead( &Pool->NonPagedHead );

    status = Pool->PagedLock.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPOOL,
                            "Initializing paged lock failed for Pool 0x%p, "
                            "status %!STATUS!",
                            Pool, status);
        goto exit;
    }

    InitializeListHead( &Pool->PagedHead );

    // Pool usage information
    Pool->NonPagedBytes = 0;
    Pool->PagedBytes = 0;

    Pool->NonPagedAllocations = 0;
    Pool->PagedAllocations = 0;

    Pool->PeakNonPagedBytes = 0;
    Pool->PeakPagedBytes = 0;

    Pool->PeakNonPagedAllocations = 0;
    Pool->PeakPagedAllocations = 0;

exit:
    if (!NT_SUCCESS(status)) {
        //
        // We disable pool tracking if we could not initialize the locks needed
        //
        // If we don't do this we would need another flag to make FxPoolDestroy
        // not access the locks
        //
        FxDriverGlobals->FxPoolTrackingOn = FALSE;
    }

    //
    // FxPoolDestroy will always be called even if we fail FxPoolInitialize
    //
    // FxPoolDestroy will uninitialize locks both in success and failure
    // cases
    //

    return status;
}

VOID
FxPoolDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_POOL  Pool
    )
/*++

Routine Description:
    Destroy the FX_POOL tracking object

Arguments:
    Pool    - FX_POOL object for tracking allocations

Returns:
    STATUS_SUCCESS

--*/
{
    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGPOOL,
                        "Destroying Pool 0x%p", Pool);

    if (FxDriverGlobals->IsPoolTrackingOn()) {
        FxPoolDump(FxDriverGlobals, Pool);

#if FX_CORE_MODE==FX_CORE_KERNEL_MODE
        FxMdlDump(FxDriverGlobals);
#endif
        //
        // We don't automatically free memory items since we don't
        // know what they contain, and who is still referencing them.
        //
    }

    Pool->PagedLock.Uninitialize();
    Pool->NonPagedLock.Uninitialize();

    return;
}

_Must_inspect_result_
NTSTATUS
FxPoolPackageInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
/*++

Routine Description:
    Initialize the pool support package at startup time.

    This must be called before the first allocation.

Arguments:
    FxDriverGlobals - DriverGlobals

Returns:
    STATUS_SUCCESS

--*/
{
    return FxPoolInitialize(FxDriverGlobals, &FxDriverGlobals->FxPoolFrameworks);
}

VOID
FxPoolPackageDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
/*++

Routine Description:
    Destroy the pool support package at unload time

    This must be after the last free

Arguments:
    FxDriverGlobals - Driver's globals

Returns:
    STATUS_SUCCESS

--*/
{
    FxPoolDestroy(FxDriverGlobals, &FxDriverGlobals->FxPoolFrameworks);
    return;
}

