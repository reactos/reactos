/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPagedLookasideList.hpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _FXPAGEDLOOKASIDELIST_H_
#define _FXPAGEDLOOKASIDELIST_H_

class FxPagedLookasideListFromPool : public FxLookasideListFromPool {

    friend FxMemoryBufferFromPoolLookaside;

public:
    FxPagedLookasideListFromPool(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in ULONG PoolTag,
        __in FxDeviceBase* DeviceBase,
        __in FxDeviceBase* MemoryDeviceBase
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in size_t BufferSize,
        __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Allocate(
        __out FxMemoryObject** PPMemory
        );

protected:
    ~FxPagedLookasideListFromPool(
        VOID
        );

    virtual
    VOID
    Reclaim(
        __in FxMemoryBufferFromLookaside* Memory
        );

    virtual
    VOID
    ReclaimPool(
        __inout PVOID Pool
        )
    {
        if (m_BufferSize < PAGE_SIZE) {
            PFX_POOL_HEADER pHeader;

            //
            // For < PAGE_SIZE, we track the allocation
            //
            pHeader = CONTAINING_RECORD(Pool, FX_POOL_HEADER, AllocationStart);

            //
            // If PoolTracker is on then do....
            //
            if (GetDriverGlobals()->IsPoolTrackingOn()) {
                //
                // Decommission this Paged Allocation tracker
                //
#pragma prefast(suppress:__WARNING_BUFFER_UNDERFLOW, "Accessing pool's header to reclaim pool");
                FxPoolRemovePagedAllocateTracker((PFX_POOL_TRACKER) pHeader->Base);
            }

#pragma prefast(suppress:__WARNING_BUFFER_UNDERFLOW, "Accessing pool's header to reclaim pool");
            FxFreeToPagedLookasideList(&m_PoolLookaside, pHeader->Base);
        }
        else {
            //
            // Page or greater size has no allocation tracking info prepended
            // to the allocation.
            //
            FxFreeToPagedLookasideList(&m_PoolLookaside, Pool);
        }
    }

    PVOID
    InitPagedAlloc(
        __out_bcount(this->m_RawBufferSize) PVOID Alloc
        );

    BOOLEAN
    UsePagedBufferObject(
        VOID
        )
    {
        return m_MemoryDeviceBase != NULL ? TRUE : FALSE;
    }

protected:
    //
    // m_BufferSize is the size of the allocation that client sees.
    // m_RawBufferSize is m_BufferSize + whatever else we need to track the
    // allocation.
    //
    size_t m_RawBufferSize;

    FxDeviceBase* m_MemoryDeviceBase;

    NPAGED_LOOKASIDE_LIST m_ObjectLookaside;

    PAGED_LOOKASIDE_LIST m_PoolLookaside;
};

#endif // _FXPAGEDLOOKASIDELIST_H_
