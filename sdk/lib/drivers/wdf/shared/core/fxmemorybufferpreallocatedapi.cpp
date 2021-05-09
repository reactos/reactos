/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxMemoryBufferPreallocatedApi.cpp

Abstract:

    This modules implements the C API's for the FxMemoryBufferPreallocated.

Author:


Environment:

    kernel mode only

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxmemorybufferpreallocated.hpp"

extern "C" {
// #include "FxMemoryBufferPreallocatedAPI.tmh"
}

extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
STDCALL
WDFEXPORT(WdfMemoryCreatePreallocated)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    PVOID Buffer,
    __in
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    __out   //deref cud be null if unable to allocate memory
    WDFMEMORY* PMemory
    )/*++

Routine Description:
    External API provided to the client driver to create a WDFMEMORY object
    whose associated buffers are supplied by the caller.  This API is provided
    so that the caller does not need to allocate a new buffer every time she
    wants to pass a WDFMEMORY object to an API which requires it.  It is up to
    the client driver to free the Buffer and Context at the appropriate time.

Arguments:
    Attributes - Context to associate with the returned WDFMEMORY handle

    Buffer - Buffer to associate with the returned WDFMEMORY handle

    BufferSize - Size of Buffer in bytes

    PMemory  - Handle to be returned to the caller

Return Value:
    STATUS_INVALID_PARAMETER - if required parameters are incorrect

    STATUS_INSUFFICIENT_RESOURCES - if no resources are available

    STATUS_SUCCESS  - success

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxMemoryBufferPreallocated *pBuffer;
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

    FxPointerNotNull(pFxDriverGlobals, Buffer);
    FxPointerNotNull(pFxDriverGlobals, PMemory);

    *PMemory = NULL;

    if (BufferSize == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Zero BufferSize not allowed, %!STATUS!", status);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, Attributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pBuffer = new(pFxDriverGlobals, Attributes)
            FxMemoryBufferPreallocated(pFxDriverGlobals, Buffer, BufferSize);

    if (pBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pBuffer->Commit(Attributes, (WDFOBJECT*)&hMemory);

    if (NT_SUCCESS(status)) {
        *PMemory = hMemory;
    }
    else {
        pBuffer->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
STDCALL
WDFEXPORT(WdfMemoryAssignBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY Memory,
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize)
    PVOID Buffer,
    _In_
    __drv_when(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxMemoryBufferPreallocated* pMemory;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Memory,
                                   FX_TYPE_MEMORY_PREALLOCATED,
                                   (PVOID*) &pMemory,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Buffer);

    if (BufferSize == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Zero BufferSize not allowed, %!STATUS!", status);
        return status;
    }

    pMemory->UpdateBuffer(Buffer, BufferSize);

    return STATUS_SUCCESS;
}

} // extern "C"
