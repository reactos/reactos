/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxNPagedLookasideListUm.cpp

Abstract:

    This module implements a frameworks managed FxNPagedLookasideList

Author:

Environment:

    user mode only

Revision History:


--*/

#include "coreprivshared.hpp"

#include "FxNPagedLookasideList.hpp"
#include "FxMemoryBufferFromLookaside.hpp"

FxNPagedLookasideList::FxNPagedLookasideList(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in ULONG PoolTag
    ) :
    FxLookasideList(FxDriverGlobals, sizeof(*this), PoolTag)
{
}

FxNPagedLookasideList::~FxNPagedLookasideList()
{
}

_Must_inspect_result_
NTSTATUS
FxNPagedLookasideList::Initialize(
    __in size_t BufferSize,
    __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
    )
{
    NTSTATUS status;

    status = InitializeLookaside((USHORT) BufferSize,
                                 sizeof(FxMemoryBufferFromLookaside),
                                 MemoryAttributes);

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
    p = FxAllocateFromNPagedLookasideList(NULL, m_MemoryObjectSize);
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
    UfxVerifierTrapNotImpl();
}

FxNPagedLookasideListFromPool::~FxNPagedLookasideListFromPool(
    VOID
    )
{
    UfxVerifierTrapNotImpl();
}

_Must_inspect_result_
NTSTATUS
FxNPagedLookasideListFromPool::Initialize(
    __in size_t BufferSize,
    __in PWDF_OBJECT_ATTRIBUTES MemoryAttributes
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
FxNPagedLookasideListFromPool::Allocate(
    __out FxMemoryObject** PPMemory
    )
{
    UfxVerifierTrapNotImpl();
    return STATUS_UNSUCCESSFUL;
}

VOID
FxNPagedLookasideListFromPool::Reclaim(
    __in FxMemoryBufferFromLookaside * Memory
    )
{
    UfxVerifierTrapNotImpl();
}

