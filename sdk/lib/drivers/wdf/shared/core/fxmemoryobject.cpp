/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryObject.cpp

Abstract:

    This module implements a frameworks managed FxMemoryObject

Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
// #include "FxMemoryObject.tmh"
}

FxMemoryObject::FxMemoryObject(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in size_t BufferSize
    ) :
    // intentionally do not pass IFX_TYPE_MEMORY to the base constructor
    // because we need to pass the interface back when converting from
    // handle to object and that will require a different this pointer offset
    // which will be handled by QueryInterface
    FxObject(FX_TYPE_OBJECT, ObjectSize, FxDriverGlobals),
    m_BufferSize(BufferSize)
{
    //
    // Since we are passing the generic object type FX_TYPE_OBJECT to FxObject,
    // we need to figure out on our own if need to allocate a tag tracker or not.
    //
    if (IsDebug()) {
        AllocateTagTracker(IFX_TYPE_MEMORY);
    }
}

_Must_inspect_result_
NTSTATUS
FxMemoryObject::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
    __in POOL_TYPE PoolType,
    __in ULONG PoolTag,
    __in size_t BufferSize,
    __out FxMemoryObject** Object
    )
{
    //
    // If the buffer is
    // a) a PAGE or larger
    // b) we are debugging allocations
    // c) less then a page and pageable
    //
    // separate the object from its memory so that we can assure
    //
    // 1)  the buffer pointer is PAGE aligned
    // 2)  the kernel's buffer overrun/underrun checking can be used
    // 3)  for case c), that the object is non pageable while the memory pointer
    //     it returns to the driver is pageable
    //

    //
    // By placing FxIsPagedPool last in the list, BufferSize < PAGE_SIZE
    //
    if (BufferSize >= PAGE_SIZE ||
        (FxDriverGlobals->FxVerifierOn && FxDriverGlobals->FxPoolTrackingOn) ||
        FxIsPagedPoolType(PoolType)) {
        return FxMemoryBufferFromPool::_Create(
            FxDriverGlobals,
            Attributes,
            PoolType,
            PoolTag,
            BufferSize,
            Object);
    }
    else {

        //
        // Before the changes for NxPool this code path assumed NonPagedPool
        //
        // To maintain compatibility with existing behavior (and add on NxPool
        // options we pass in PoolType to FxMemoryBuffer::_Create but
        // normalize NonPagedPool variants to NonPagedPool.
        //
        switch(PoolType)
        {
            case NonPagedPoolBaseMustSucceed:
            case NonPagedPoolBaseCacheAligned:
            case NonPagedPoolBaseCacheAlignedMustS:
                PoolType = NonPagedPool;
        }

        return FxMemoryBuffer::_Create(
            FxDriverGlobals,
            Attributes,
            PoolTag,
            BufferSize,
            PoolType,
            Object);
    }
}

_Must_inspect_result_
NTSTATUS
IFxMemory::CopyFromPtr(
    __in_opt PWDFMEMORY_OFFSET DestinationOffsets,
    __in_bcount(SourceBufferLength) PVOID SourceBuffer,
    __in size_t  SourceBufferLength,
    __in_opt PWDFMEMORY_OFFSET SourceOffsets
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    //
    // We read from the supplied buffer writing to the current FxMemoryBuffer
    //
    if (GetFlags() & IFxMemoryFlagReadOnly) {
        //
        // FxMemoryBuffer is not writeable
        //
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Target WDFMEMORY 0x%p is ReadOnly", GetHandle());
        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return STATUS_ACCESS_VIOLATION;
    }

    return _CopyPtrToPtr(
        SourceBuffer,
        SourceBufferLength,
        SourceOffsets,
        GetBuffer(),
        GetBufferSize(),
        DestinationOffsets
        );
}

_Must_inspect_result_
NTSTATUS
IFxMemory::CopyToPtr(
    __in_opt PWDFMEMORY_OFFSET SourceOffsets,
    __out_bcount(DestinationBufferLength)PVOID DestinationBuffer,
    __in size_t DestinationBufferLength,
    __in_opt PWDFMEMORY_OFFSET DestinationOffsets
    )
/*++

Routine Description:

    Worker routine for the various copy APIs.  Verifies that a copy will not
    overrun a buffer, or write into a read only memory buffer

Arguments:

    SourceOffsets - Offsets into SourceBuffer from which the copy starts.  If
        NULL, an offset of 0 is used.

    DestinationBuffer - Memory whose contents we are copying into

    DestinationBufferLength - Size of DestinationBuffer in bytes

    DestinationOffsets - Offsets into DestinationMemory from which the copy
        starts and indicates how many bytes to copy.  If length is 0 or
        parameter is NULL, the entire length of DestinationMemory is used.

Return Value:
    STATUS_BUFFER_TOO_SMALL - SourceMemory is smaller then the requested number
        of bytes to be copied.
    STATUS_INVALID_BUFFER_SIZE - DestinationMemory is not large enough to contain
        the number of bytes requested to be copied
    NTSTATUS

  --*/
{
    //
    // We are reading from the FxMemoryBuffer, so no need to check for ReadOnly
    //
    return _CopyPtrToPtr(
        GetBuffer(),
        GetBufferSize(),
        SourceOffsets,
        DestinationBuffer,
        DestinationBufferLength,
        DestinationOffsets
        );
}

_Must_inspect_result_
NTSTATUS
IFxMemory::_CopyPtrToPtr(
    __in_bcount(SourceBufferLength)PVOID SourceBuffer,
    __in size_t SourceBufferLength,
    __in_opt PWDFMEMORY_OFFSET SourceOffsets,
    __out_bcount(DestinationBufferLength) PVOID DestinationBuffer,
    __in size_t DestinationBufferLength,
    __in_opt PWDFMEMORY_OFFSET DestinationOffsets
    )
/*++

Routine Description:
    Worker routine for the various copy APIs.  Verifies that a copy will not
    overrun a buffer.

Arguments:
    SourceBuffer - Memory whose contents we are copying from.

    SourceBufferLength - Size of SourceBuffer in bytes

    SourceOffsets - Offsets into SourceBuffer from which the copy starts.  If
        NULL, an offset of 0 is used.

    DestinationBuffer - Memory whose contents we are copying into

    DestinationBufferLength - Size of DestinationBuffer in bytes

    DestinationOffsets - Offsets into DestinationMemory from which the copy
        starts and indicates how many bytes to copy.  If length is 0 or
        parameter is NULL, the entire length of DestinationMemory is used.

Return Value:
    STATUS_BUFFER_TOO_SMALL - SourceMemory is smaller then the requested number
        of bytes to be copied.
    STATUS_INVALID_BUFFER_SIZE - DestinationMemory is not large enough to contain
        the number of bytes requested to be copied
    NTSTATUS

  --*/
{
    size_t srcSize, copyLength;
    PUCHAR pSrcBuf, pDstBuf;

    if (SourceBuffer == NULL) {
       return STATUS_INVALID_PARAMETER;
    }

    pSrcBuf = (PUCHAR) SourceBuffer;
    srcSize = SourceBufferLength;

    pDstBuf = (PUCHAR) DestinationBuffer;
    copyLength = DestinationBufferLength;

    if (SourceOffsets != NULL) {
        if (SourceOffsets->BufferOffset != 0) {
            if (SourceOffsets->BufferOffset >= srcSize) {
                //
                // Offset is beyond end of buffer
                //
                return STATUS_BUFFER_TOO_SMALL;
            }

            //
            // Adjust the start and source size to reflect the offset info the
            // source
            //
            pSrcBuf += SourceOffsets->BufferOffset;
            srcSize -= SourceOffsets->BufferOffset;
        }
    }

    if (DestinationOffsets != NULL) {
        if (DestinationOffsets->BufferOffset != 0) {
            if (DestinationOffsets->BufferOffset >= copyLength) {
                //
                // Offset is beyond end of buffer
                //
                return STATUS_INVALID_BUFFER_SIZE;
            }

            //
            // Adjust the start and copy length to reflect the offset info the
            // destination
            //
            pDstBuf += DestinationOffsets->BufferOffset;
            copyLength -= DestinationOffsets->BufferOffset;
        }

        //
        // Non zero buffer length overrides previously calculated copy length
        //
        if (DestinationOffsets->BufferLength != 0) {
            //
            // Is the desired buffer length greater than the amount of buffer
            // available?
            //
            if (DestinationOffsets->BufferLength > copyLength) {
                return STATUS_INVALID_BUFFER_SIZE;
            }

            copyLength = DestinationOffsets->BufferLength;
        }
    }

    //
    // Compare the final computed copy length against the length of the source
    // buffer.
    //
    if (copyLength > srcSize) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(pDstBuf, pSrcBuf, copyLength);

    return STATUS_SUCCESS;
}
