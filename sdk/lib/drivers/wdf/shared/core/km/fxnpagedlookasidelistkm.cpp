/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxNPagedLookasideList.cpp

Abstract:

    This module implements a frameworks managed FxNPagedLookasideList

Author:

Environment:

    kernel mode only

Revision History:


--*/

#include "coreprivshared.hpp"

#include "fxnpagedlookasidelist.hpp"
#include "fxmemorybufferfromlookaside.hpp"

FxNPagedLookasideList::FxNPagedLookasideList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in ULONG PoolTag
    ) :
    FxLookasideList(FxDriverGlobals, sizeof(*this), PoolTag)
{
}

FxNPagedLookasideList::~FxNPagedLookasideList()
{
    if (m_MemoryObjectSize != 0) {
        ExDeleteNPagedLookasideList(&m_ObjectLookaside);
    }
}

_Must_inspect_result_
NTSTATUS
FxNPagedLookasideList::Initialize(
    __in size_t BufferSize,
    __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
    )
{
    NTSTATUS status;

    //
    // This type of class does not support BufferSize greater than 0xFFFF.
    // The verification below ensures this contract is respected in both
    // free and chk builds.
    //
    if (BufferSize > MAXUSHORT) {
        ASSERT(FALSE);
        status = STATUS_INVALID_BUFFER_SIZE;
        goto Done;
    }

    status = InitializeLookaside((USHORT) BufferSize,
                                 sizeof(FxMemoryBufferFromLookaside),
                                 MemoryAttributes);

    //
    // Must be called after InitializeLookaside so that m_MemoryObjectSize is
    // computed correctly.
    //
    if (NT_SUCCESS(status)) {
        ASSERT(m_MemoryObjectSize != 0);

        //
        // Initialize a non-paged pool with these characteristics.
        //
        ExInitializeNPagedLookasideList(&m_ObjectLookaside,
                                        NULL,
                                        NULL,
                                        0,
                                        m_MemoryObjectSize,
                                        m_PoolTag,
                                        0);
    }

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxNPagedLookasideList::Allocate(
    __out FxMemoryObject** PPMemory
    )
{
    FxMemoryBufferFromLookaside *pBuffer;
    PVOID p;

    if (PPMemory == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    *PPMemory = NULL;

    //
    // Get the raw memory allocation.
    //
    p = FxAllocateFromNPagedLookasideList(&m_ObjectLookaside);
    if (p == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    p = InitObjectAlloc(p);

    //
    // Construct new FxMemoryBufferFromLookaside
    //
    pBuffer = new(GetDriverGlobals(), p, m_BufferSize, &m_MemoryAttributes)
        FxMemoryBufferFromLookaside(GetDriverGlobals(), this, m_BufferSize);

    //
    // pBuffer might be displaced if there is a debug extension
    //
    ASSERT(_GetBase(pBuffer) == p);

    *PPMemory = pBuffer;

    return STATUS_SUCCESS;
}

VOID
FxNPagedLookasideList::Reclaim(
    __in FxMemoryBufferFromLookaside * Memory
    )
{
    _Reclaim(GetDriverGlobals(), &m_ObjectLookaside, Memory);
}

FxNPagedLookasideListFromPool::FxNPagedLookasideListFromPool(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in ULONG PoolTag
    ) : FxLookasideListFromPool(FxDriverGlobals, sizeof(*this), PoolTag)
{
}

FxNPagedLookasideListFromPool::~FxNPagedLookasideListFromPool(
    VOID
    )
{
    if (m_MemoryObjectSize != 0) {
        ExDeleteNPagedLookasideList(&m_ObjectLookaside);
    }

    if (m_BufferSize != 0) {
        ExDeleteNPagedLookasideList(&m_PoolLookaside);
    }
}

_Must_inspect_result_
NTSTATUS
FxNPagedLookasideListFromPool::Initialize(
    __in size_t BufferSize,
    __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
    )
{
    NTSTATUS status;

    status = InitializeLookaside(0,
                                 sizeof(FxMemoryBufferFromPoolLookaside),
                                 MemoryAttributes);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Initialize a non-paged pool with these characteristics.
    //
    ExInitializeNPagedLookasideList(&m_ObjectLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    m_MemoryObjectSize,
                                    m_PoolTag,
                                    0);

    m_BufferSize = BufferSize;

    //
    // Initialize a non-paged pool with these characteristics.
    //
    // Free and Allocate are left intentionally NULL so that we use the Ex
    // versions.  We want to use the Ex versions because the allocations are >=
    // PAGE_SIZE and we don't want to burn a whole page for pool tracking.
    //
    ExInitializeNPagedLookasideList(&m_PoolLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    m_BufferSize,
                                    m_PoolTag,
                                    0);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxNPagedLookasideListFromPool::Allocate(
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
    pBuf =  FxAllocateFromNPagedLookasideList(&m_PoolLookaside);
    if (pBuf == NULL) {
        //
        // This case is safe because Reclaim doesn't treat the pointer as an
        // object, rather it just performs pointer math and then frees the alloc
        //
        Reclaim((FxMemoryBufferFromPoolLookaside*) pObj);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Construct a new FxMemoryBufferFromLookaside using the pool allocated
    // above.
    //
    pBuffer = new(GetDriverGlobals(), pObj, &m_MemoryAttributes)
        FxMemoryBufferFromPoolLookaside(GetDriverGlobals(),
                                        this,
                                        m_BufferSize,
                                        pBuf);

    //
    // pBuffer might be displaced if there is a debug extension
    //
    ASSERT(_GetBase(pBuffer) == pObj);

    *PPMemory = pBuffer;

    return STATUS_SUCCESS;
}

VOID
FxNPagedLookasideListFromPool::Reclaim(
    __in FxMemoryBufferFromLookaside * Memory
    )
{
    _Reclaim(GetDriverGlobals(), &m_ObjectLookaside, Memory);
}
