#include "common/fxpool.h"
#include "common/mxgeneral.h"
#include "common/mxmemory.h"
#include "common/fxpoolinlines.h"
#include "common/dbgtrace.h"
#include "common/fxmdl.h"
#include "common/fxverifier.h"


#define WDF_REQUIRED_PARAMETER_IS_NULL 4

//
// MessageId: WDF_VIOLATION
//
// MessageText:
//
//  WDF_VIOLATION
//
#define WDF_VIOLATION                    ((ULONG)0x0000010DL)

VOID
FxPoolFree(
    /*__in_xcount(ptr is at an offset from AllocationStart)*/ PVOID ptr
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
    if( ptr == NULL )
    {
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
    if( /*Mx::IsKM() &&*/ ( ((ULONG_PTR)ptr & (PAGE_SIZE-1)) == 0 ) )
    {
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
    if (pHeader->FxDriverGlobals->IsPoolTrackingOn())
    {

        pTracker = (PFX_POOL_TRACKER) pTrueBase;

        if (FxIsPagedPoolType(pTracker->PoolType))
        {
            //
            // Decommission this Paged Allocation tracker
            //
            FxPoolRemovePagedAllocateTracker(pTracker);
        }
        else
        {
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

    if (FxDriverGlobals->IsPoolTrackingOn())
    {
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

VOID
FxMdlDump(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    FxAllocatedMdls *pCur;
    BOOLEAN leak;

    if (FxDriverGlobals->DebugExtension == NULL)
    {
        return;
    }

    leak = FALSE;

    for (pCur = &FxDriverGlobals->DebugExtension->AllocatedMdls;
         pCur != NULL;
         pCur = pCur->Next)
    {
        ULONG i;

        for (i = 0; i < NUM_MDLS_IN_INFO; i++)
        {
            if (pCur->Info[i].Mdl != NULL)
            {
                leak = TRUE;

                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "PMDL 0x%p leaked, FxObject owner %p, Callers Address %p",
                    pCur->Info[i].Mdl, pCur->Info[i].Owner,
                    pCur->Info[i].Caller);
            }
        }
    }

    if (leak)
    {
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
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

    for (ple = Pool->PagedHead.Flink; ple != &Pool->PagedHead; ple = ple->Flink)
    {
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
         ple = ple->Flink)
    {
        pTracker = CONTAINING_RECORD(ple, FX_POOL_TRACKER, Link );

        // Leaker
        leak = TRUE;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "FX_POOL 0x%p leaked non-paged memory alloc 0x%p (tracking block %p)",
            Pool, pTracker+1, pTracker);
    }

    Pool->NonPagedLock.Release(oldIrql);

    if (leak)
    {
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return STATUS_MORE_ENTRIES;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}
