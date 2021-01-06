/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferFromPool.cpp

Abstract:

    This module implements a frameworks managed FxMemoryBufferFromPool

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"

FxMemoryBufferFromPool::FxMemoryBufferFromPool(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t BufferSize
    ) :
    FxMemoryObject(FxDriverGlobals, sizeof(*this), BufferSize)
/*++

Routine Description:
    Constructor for this object.

Arguments:
    BufferSize - The buffer size associated with this object

Return Value:
    None

  --*/
{
    m_Pool = NULL;
}

FxMemoryBufferFromPool::FxMemoryBufferFromPool(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t BufferSize,
    __in USHORT ObjectSize
    ) :
    FxMemoryObject(FxDriverGlobals, ObjectSize, BufferSize)
/*++

Routine Description:
    Constructor for this object.

Arguments:
    BufferSize - The buffer size associated with this object

    ObjectSize - size of the derived object

Return Value:
    None

  --*/
{
    m_Pool = NULL;
}

FxMemoryBufferFromPool::~FxMemoryBufferFromPool()
/*++

Routine Description:
    Destructor for this object.  This function does nothing, it lets
    SelfDestruct do all the work.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (m_Pool != NULL) {
        MxMemory::MxFreePool(m_Pool);
    }
}

_Must_inspect_result_
NTSTATUS
FxMemoryBufferFromPool::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
    __in POOL_TYPE PoolType,
    __in ULONG PoolTag,
    __in size_t BufferSize,
    __out FxMemoryObject** Buffer
    )
{
    CfxDeviceBase* pDeviceBase;
    FxMemoryBufferFromPool* pBuffer;
    NTSTATUS status;
    BOOLEAN isPaged;

    isPaged = FxIsPagedPoolType(PoolType);

    if (isPaged) {
        pDeviceBase = FxDeviceBase::_SearchForDevice(FxDriverGlobals,
                                                     Attributes);
    }
    else {
        pDeviceBase = NULL;
    }

    //
    // FxMemoryBufferFromPool can handle paged allocation as well.  We only
    // use FxMemoryPagedBufferFromPool if we have an FxDeviceBase that we can use
    // in the dispose path, otherwise these 2 classes are the same.
    //
    if (pDeviceBase != NULL) {
        ASSERT(isPaged);
        pBuffer = new(FxDriverGlobals, Attributes)
            FxMemoryPagedBufferFromPool(FxDriverGlobals, BufferSize, pDeviceBase);
    }
    else {
        pBuffer = new(FxDriverGlobals, Attributes)
            FxMemoryBufferFromPool(FxDriverGlobals, BufferSize);
    }

    if (pBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = STATUS_SUCCESS;

    if (pBuffer->AllocateBuffer(PoolType, PoolTag) == FALSE) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(status)) {
        pBuffer->DeleteFromFailedCreate();
        return status;
    }

    if (isPaged) {
        //
        // Callbacks might be a bit excessive (passive dispose is what we are
        // really after) because this object has no callbacks, but this is being
        // proactive in case any callbacks are added.  If they are added, this
        // will take care of them w/out additional changes to object setup.
        //
        pBuffer->MarkPassiveCallbacks(ObjectDoNotLock);
    }

    *Buffer = pBuffer;

    return STATUS_SUCCESS;
}
