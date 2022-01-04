/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCommonBufferAPI.cpp

Abstract:

    Base for WDF CommonBuffer APIs

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#include "fxdmapch.hpp"

extern "C" {
// #include "FxCommonBufferAPI.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfCommonBufferCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFCOMMONBUFFER * CommonBufferHandle
    )
{
    FxCommonBuffer  * pComBuf;
    FxDmaEnabler    * pDmaEnabler;
    NTSTATUS          status;
    WDFOBJECT         handle;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    //
    // Get validate DmaEnabler handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaEnabler,
                                   FX_TYPE_DMA_ENABLER,
                                   (PVOID *) &pDmaEnabler,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, CommonBufferHandle);

    *CommonBufferHandle = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Basic parameter validation
    //
    if (Length == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Length is 0, %!STATUS!", status);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        Attributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED
                                        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Create a new CommonBuffer object
    //
    pComBuf = new(pFxDriverGlobals, Attributes)
        FxCommonBuffer(pFxDriverGlobals, pDmaEnabler);

    if (pComBuf == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Could not allocate memory for a WDFCOMMONBUFFER, "
                            "%!STATUS!", status);
        return status;
    }

    //
    // Assign this FxCommonBuffer to its parent FxDmaEnabler object.
    //
    status = pComBuf->Commit(Attributes, (WDFOBJECT*)&handle, pDmaEnabler);

    if (NT_SUCCESS(status)) {
        //
        // Ok: now allocate a CommonBuffer via this DmaEnabler
        //
        status = pComBuf->AllocateCommonBuffer( Length );
    }

    if (NT_SUCCESS(status)) {
        //
        // Only return a valid handle on success.
        //
        *CommonBufferHandle = (WDFCOMMONBUFFER) handle;
    }
    else {
        pComBuf->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfCommonBufferCreateWithConfig)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    __in
    PWDF_COMMON_BUFFER_CONFIG Config,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFCOMMONBUFFER * CommonBufferHandle
    )
{
    FxCommonBuffer  * pComBuf;
    FxDmaEnabler    * pDmaEnabler;
    NTSTATUS          status;
    WDFOBJECT         handle;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    //
    // Get validate DmaEnabler handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaEnabler,
                                   FX_TYPE_DMA_ENABLER,
                                   (PVOID *) &pDmaEnabler,
                                   &pFxDriverGlobals);

    //
    // Basic parameter validation
    //
    FxPointerNotNull(pFxDriverGlobals, Config);

    if (Config->Size != sizeof(WDF_COMMON_BUFFER_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "WDF_COMMON_BUFFER_CONFIG Size 0x%x, expected 0x%x, %!STATUS!",
            Config->Size, sizeof(WDF_COMMON_BUFFER_CONFIG), status);

        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, CommonBufferHandle);

    *CommonBufferHandle = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Length == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Length is 0, %!STATUS!", status);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        Attributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED
                                        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pComBuf = new(pFxDriverGlobals, Attributes)
        FxCommonBuffer(pFxDriverGlobals, pDmaEnabler);

    if (pComBuf == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Could not allocate memory for a WDFCOMMONBUFFER, "
                            "%!STATUS!", status);
        return status;
    }

    //
    // Assign this FxCommonBuffer to its parent FxDmaEnabler object.
    //
    status = pComBuf->Commit(Attributes, (WDFOBJECT*)&handle, pDmaEnabler);

    if (NT_SUCCESS(status)) {
        //
        // Set the alignment value before calling AllocateCommonBuffer.
        //
        pComBuf->SetAlignment(Config->AlignmentRequirement);
        status = pComBuf->AllocateCommonBuffer( Length );
    }

    if (NT_SUCCESS(status)) {
        //
        // Only return a valid handle on success.
        //
        *CommonBufferHandle = (WDFCOMMONBUFFER) handle;
    }
    else {
        pComBuf->DeleteFromFailedCreate();
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PVOID
STDCALL
WDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    FxCommonBuffer  * pComBuf;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         CommonBuffer,
                         FX_TYPE_COMMON_BUFFER,
                         (PVOID *) &pComBuf);

    return pComBuf->GetAlignedVirtualAddress();
}

__drv_maxIRQL(DISPATCH_LEVEL)
PHYSICAL_ADDRESS
STDCALL
WDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    FxCommonBuffer  * pComBuf;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         CommonBuffer,
                         FX_TYPE_COMMON_BUFFER,
                         (PVOID *) &pComBuf);

    return pComBuf->GetAlignedLogicalAddress();
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
STDCALL
WDFEXPORT(WdfCommonBufferGetLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOMMONBUFFER CommonBuffer
    )
{
    FxCommonBuffer  * pComBuf;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         CommonBuffer,
                         FX_TYPE_COMMON_BUFFER,
                         (PVOID *) &pComBuf);

    return pComBuf->GetLength();
}

} // extern "C"
