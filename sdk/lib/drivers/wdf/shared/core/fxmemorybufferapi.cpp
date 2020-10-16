/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferApi.cpp

Abstract:

    This modules implements the C API's for the FxMemoryBuffer.

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxmemorybuffer.hpp"

extern "C" {
// #include "FxMemoryBufferAPI.tmh"
}

extern "C" {

_Must_inspect_result_
__drv_when(PoolType == 1 || PoolType == 257, __drv_maxIRQL(APC_LEVEL))
__drv_when(PoolType == 0 || PoolType == 256, __drv_maxIRQL(DISPATCH_LEVEL))
WDFAPI
NTSTATUS
STDCALL
WDFEXPORT(WdfMemoryCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    __drv_strictTypeMatch(__drv_typeExpr)
    POOL_TYPE PoolType,
    __in_opt
    ULONG PoolTag,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __out
    WDFMEMORY* Memory,
    __out_opt
    PVOID* Buffer
    )
/*++

Routine Description:
    Creates a WDFMEMORY handle based on the caller's specifications

Arguments:
    Attributes - Attributes associated with this object

    PoolType - The type of pool created

    PoolTag - Tag to use when allocating the memory.  If 0, then the frameworks
        tag is used

    BufferSize - The size of the buffer represented by the returned handle

    Memory - The returned handle to the caller

    Buffer - (opt) Pointer to the associated memory buffer.

Return Value:
    STATUS_INVALID_PARAMETER - any required parameters are not present

    STATUS_INSUFFICIENT_RESOURCES - could not allocated the object that backs
        the handle

    STATUS_SUCCESS - success

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxMemoryObject* pBuffer;
    WDFMEMORY hMemory;
    NTSTATUS status;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // Get the parent's globals if it is present
    //
    if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(pFxDriverGlobals,
                                                             Attributes))) {
        FxObject* pParent;

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       Attributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);
    }

    FxPointerNotNull(pFxDriverGlobals, Memory);

    if (FxIsPagedPoolType(PoolType)) {
        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    if (BufferSize == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "BufferSize == 0 not allowed,  %!STATUS!", status);
        return status;
    }

    *Memory = NULL;

    status = FxValidateObjectAttributes(pFxDriverGlobals, Attributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (PoolTag == 0) {
        PoolTag = pFxDriverGlobals->Tag;
    }

    FxVerifierCheckNxPoolType(pFxDriverGlobals, PoolType, PoolTag);

    status = FxMemoryObject::_Create(
        pFxDriverGlobals,
        Attributes,
        PoolType,
        PoolTag,
        BufferSize,
        &pBuffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pBuffer->Commit(Attributes, (WDFOBJECT*)&hMemory);

    if (NT_SUCCESS(status)) {
        *Memory = hMemory;
        if (Buffer != NULL) {
            *Buffer = pBuffer->GetBuffer();
        }
    }
    else {
        pBuffer->DeleteFromFailedCreate();
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PVOID
WDFAPI
STDCALL
WDFEXPORT(WdfMemoryGetBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY Memory,
    __out_opt
    size_t* BufferSize
    )
/*++

Routine Description:
    Retrieves the raw pointers associated with WDFMEMORY handle

Arguments:
    Memory - handle to the WDFMEMORY

    BufferSize - the size / length of the buffer

Return Value:
    raw buffer

  --*/
{
    DDI_ENTRY();

    IFxMemory* pMemory;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Memory,
                         IFX_TYPE_MEMORY,
                         (PVOID*)&pMemory);

    if (BufferSize != NULL) {
        *BufferSize = pMemory->GetBufferSize();
    }

    return pMemory->GetBuffer();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
WDFEXPORT(WdfMemoryCopyToBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY SourceMemory,
    __in
    size_t SourceOffset,
    __out_bcount( NumBytesToCopyTo)
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    )
/*++

Routine Description:
    Copies memory from a WDFMEMORY handle to a raw pointer

Arguments:
    SourceMemory - Memory handle whose contents we are copying from.

    SourceOffset - Offset into SourceMemory from which the copy starts.

    Buffer - Memory whose contents we are copying into

    NumBytesToCopyTo - Number of bytes to copy into buffer.

Return Value:

    STATUS_BUFFER_TOO_SMALL - SourceMemory is smaller than the requested number
        of bytes to be copied.

    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxMemory* pSource;
    WDFMEMORY_OFFSET srcOffsets;
    WDFMEMORY_OFFSET dstOffsets;
    NTSTATUS status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         SourceMemory,
                         IFX_TYPE_MEMORY,
                         (PVOID*) &pSource);

    pFxDriverGlobals = pSource->GetDriverGlobals();

    FxPointerNotNull(pFxDriverGlobals, Buffer);

    if (NumBytesToCopyTo == 0) {
        status =STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Zero bytes to copy not allowed, %!STATUS!", status);
        return status;
    }

    RtlZeroMemory(&srcOffsets, sizeof(srcOffsets));
    srcOffsets.BufferLength = NumBytesToCopyTo;
    srcOffsets.BufferOffset = SourceOffset;

    RtlZeroMemory(&dstOffsets, sizeof(dstOffsets));
    dstOffsets.BufferLength = NumBytesToCopyTo;
    dstOffsets.BufferOffset = 0;

    return pSource->CopyToPtr(&srcOffsets,
                              Buffer,
                              NumBytesToCopyTo,
                              &dstOffsets);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
WDFEXPORT(WdfMemoryCopyFromBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY DestinationMemory,
    __in
    size_t DestinationOffset,
    __in
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    )
/*++

Routine Description:
    Copies memory from a raw pointer into a WDFMEMORY handle

Arguments:
    DestinationMemory - Memory handle whose contents we are copying into

    DestinationOffset - Offset into DestinationMemory from which the copy
        starts.

    Buffer - Buffer whose context we are copying from

    NumBytesToCopyFrom - Number of bytes to copy from

Return Value:

    STATUS_INVALID_BUFFER_SIZE - DestinationMemory is not large enough to contain
        the number of bytes requested to be copied

    NTSATUS

  --*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDFMEMORY_OFFSET srcOffsets;
    WDFMEMORY_OFFSET dstOffsets;
    IFxMemory* pDest;
    NTSTATUS status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DestinationMemory,
                         IFX_TYPE_MEMORY,
                         (PVOID*) &pDest);

    pFxDriverGlobals = pDest->GetDriverGlobals();

    FxPointerNotNull(pFxDriverGlobals, Buffer);

    if (NumBytesToCopyFrom == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Zero bytes to copy not allowed, %!STATUS!", status);
        return status;
    }

    RtlZeroMemory(&srcOffsets, sizeof(srcOffsets));
    srcOffsets.BufferLength = NumBytesToCopyFrom;
    srcOffsets.BufferOffset = 0;

    RtlZeroMemory(&dstOffsets, sizeof(dstOffsets));
    dstOffsets.BufferLength = NumBytesToCopyFrom;
    dstOffsets.BufferOffset = DestinationOffset;

    return pDest->CopyFromPtr(&dstOffsets,
                              Buffer,
                              NumBytesToCopyFrom,
                              &srcOffsets);
}

} // extern "C"
