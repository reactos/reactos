/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPagedLookasideList.cpp

Abstract:

    This module implements a frameworks managed FxPagedLookasideList

Author:

Environment:

    kernel mode only

Revision History:


--*/

#include "coreprivshared.hpp"

#include "FxPagedLookasideList.hpp"
#include "FxMemoryBufferFromLookaside.hpp"

FxPagedLookasideListFromPool::FxPagedLookasideListFromPool(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in ULONG PoolTag,
    __in FxDeviceBase* DeviceBase,
    __in FxDeviceBase* MemoryDeviceBase
    ) : FxLookasideListFromPool(FxDriverGlobals, sizeof(*this), PoolTag),
        m_RawBufferSize(0), m_MemoryDeviceBase(MemoryDeviceBase)
{
    SetDeviceBase(DeviceBase);

    //
    // Callbacks might be a bit excessive (passive dispose is what we are
    // really after) because this object has no callbacks, but this is being
    // proactive in case any callbacks are added.  If they are added, this
    // will take care of them w/out additional changes to object setup.
    //
    MarkPassiveCallbacks(ObjectDoNotLock);
}

FxPagedLookasideListFromPool::~FxPagedLookasideListFromPool(
    VOID
    )
{
    if (m_MemoryObjectSize != 0) {
        Mx::MxDeleteNPagedLookasideList(&m_ObjectLookaside);
    }

    if (m_RawBufferSize != 0) {
        Mx::MxDeletePagedLookasideList(&m_PoolLookaside);
    }
}

_Must_inspect_result_
NTSTATUS
FxPagedLookasideListFromPool::Initialize(
    __in size_t BufferSize,
    __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
    )
{
    size_t rawBufferSize;
    NTSTATUS status;

    if (BufferSize >= PAGE_SIZE) {
        //
        // We don't want to burn extra entire pages for tracking information
        // so we just use the size as is.
        //
        rawBufferSize = BufferSize;
    }
    else {
        //
        // Allocate extra space for tracking the allocation
        //
        status = FxPoolAddHeaderSize(GetDriverGlobals(),
                                     BufferSize,
                                     &rawBufferSize);

        if (!NT_SUCCESS(status)) {
            //
            // FxPoolAddHeaderSize logs to the IFR on error
            //
            return status;
        }
    }

    if (UsePagedBufferObject()) {
        status = InitializeLookaside(0,
                                     sizeof(FxMemoryPagedBufferFromPoolLookaside),
                                     MemoryAttributes);
    }
    else {
        status = InitializeLookaside(0,
                                     sizeof(FxMemoryBufferFromPoolLookaside),
                                     MemoryAttributes);
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // We use m_RawBufferSize == 0 as a condition not to delete the lookaside, so
    // only assign it a value once we know success is guaranteed.
    //
    m_BufferSize = BufferSize;
    m_RawBufferSize = rawBufferSize;

    //
    // Initialize a non paged pool with these characteristics.   All FxObject
    // derived objects must come from non paged pool.
    //
    Mx::MxInitializeNPagedLookasideList(&m_ObjectLookaside,
                                   NULL,
                                   NULL,
                                   0,
                                   m_MemoryObjectSize,
                                   m_PoolTag,
                                   0);

    //
    // Initialize a paged pool with these characteristics.
    //
    // Free and Allocate are left intentionally NULL so that we use the Ex
    // versions.
    //
    // bufferSize is used b/c it is full size of the object + pool requirements.
    // m_BufferSize is the size the client wants the buffer to be.
    //
    Mx::MxInitializePagedLookasideList(&m_PoolLookaside,
                                   NULL,
                                   NULL,
                                   0,
                                   m_RawBufferSize,
                                   m_PoolTag,
                                   0);

    return status;
}

#pragma prefast(push)



//This routine intentionally accesses the header of the allocated memory.
#pragma prefast(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)
PVOID
FxPagedLookasideListFromPool::InitPagedAlloc(
    __out_bcount(this->m_RawBufferSize) PVOID Alloc
    )
/*++

Routine Description:
    Initializes the object allocation so that it can be tracked and inserted
    in this drivers POOL.

Arguments:
    Alloc - the raw allocation

Return Value:
    the start of where the object memory should be, not necessarily == Alloc

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PFX_POOL_HEADER pHeader;
    PFX_POOL_TRACKER tracker;

    pFxDriverGlobals = GetDriverGlobals();

    RtlZeroMemory(Alloc, m_RawBufferSize);

    if (pFxDriverGlobals->IsPoolTrackingOn())  {
        //
        // PoolTracking is active, so format and insert
        // a tracker in the NonPagedHeader list of the pool.
        //
        tracker = (PFX_POOL_TRACKER) Alloc;
        pHeader  = WDF_PTR_ADD_OFFSET_TYPE(Alloc,
                                           sizeof(FX_POOL_TRACKER),
                                           PFX_POOL_HEADER);

        pHeader->Base = Alloc;
        pHeader->FxDriverGlobals = pFxDriverGlobals;

        FxPoolInsertPagedAllocateTracker(
            &pFxDriverGlobals->FxPoolFrameworks,
            tracker,
            m_RawBufferSize,
            m_PoolTag,
            _ReturnAddress());
    }
    else {
        //
        // PoolTracking is inactive, only format FX_POOL_HEADER area.
        //
        pHeader = (PFX_POOL_HEADER) Alloc;
        pHeader->Base = Alloc;
        pHeader->FxDriverGlobals = pFxDriverGlobals;
    }

    return &pHeader->AllocationStart[0];
}
#pragma prefast(pop)

_Must_inspect_result_
NTSTATUS
FxPagedLookasideListFromPool::Allocate(
    __out FxMemoryObject** PPMemory
    )
{
    FxMemoryBufferFromPoolLookaside* pBuffer;
    PVOID pObj, pBuf;

    //
    // Allocate the object which will contain the 2ndary allocation
    //
    pObj = FxAllocateFromNPagedLookasideList(&m_ObjectLookaside);

    if (pObj == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pObj = InitObjectAlloc(pObj);

    //
    // Create the 2ndary allocation (the one the driver writer uses), what will
    // be FxMemoryBufferFromPoolLookaside::m_Pool below.
    //
    pBuf = FxAllocateFromPagedLookasideList(&m_PoolLookaside);
    if (pBuf == NULL) {
        //
        // This case is safe because Reclaim doesn't treat the pointer as an
        // object, rather it just performs pointer math and then frees the alloc
        //
        Reclaim((FxMemoryBufferFromPoolLookaside*) pObj);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (m_BufferSize < PAGE_SIZE) {
        //
        // For allocations < PAGE, allocate a header, otherwise leave the
        // allocation alone.
        //
        pBuf = InitPagedAlloc(pBuf);
    }
    else {
        //
        // There is no tracking info before the real allocation start since we
        // don't want to burn an entire page.
        //
        DO_NOTHING();
    }

    //
    // Construct a new FxMemoryBufferFromPoolLookaside using the pool will allocated
    // above.
    //
    // Both objects will know that the base object is one allocation and the
    // buffer is another.  FxMemoryPagedBufferFromPoolLookaside also knows to
    // dispose itself on the owning FxDeviceBase* pointer.
    //
    if (UsePagedBufferObject()) {
        pBuffer = new(GetDriverGlobals(), pObj, &m_MemoryAttributes)
            FxMemoryPagedBufferFromPoolLookaside(GetDriverGlobals(),
                                                 this,
                                                 m_BufferSize,
                                                 pBuf,
                                                 m_MemoryDeviceBase);
    }
    else {
        pBuffer = new(GetDriverGlobals(), pObj, &m_MemoryAttributes)
            FxMemoryBufferFromPoolLookaside(GetDriverGlobals(),
                                            this,
                                            m_BufferSize,
                                            pBuf);
    }

    //
    // pBuffer might be displaced if there is a debug extension
    //
    ASSERT(_GetBase(pBuffer) == pObj);

    //
    // Callbacks might be a bit excessive (passive dispose is what we are
    // really after) because this object has no callbacks, but this is being
    // proactive in case any callbacks are added.  If they are added, this
    // will take care of them w/out additional changes to object setup.
    //
    pBuffer->MarkPassiveCallbacks(ObjectDoNotLock);

    *PPMemory = pBuffer;

    return STATUS_SUCCESS;
}

VOID
FxPagedLookasideListFromPool::Reclaim(
    __in FxMemoryBufferFromLookaside * Memory
    )
{
    _Reclaim(GetDriverGlobals(), &m_ObjectLookaside, Memory);
}
