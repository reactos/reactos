/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDmaEnablerAPI.cpp

Abstract:

    Base for WDF DMA Enabler object APIs

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#include "FxDmaPCH.hpp"

extern "C" {
#include "FxDmaEnablerAPI.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaEnablerCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDF_DMA_ENABLER_CONFIG * Config,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFDMAENABLER * DmaEnablerHandle
    )
{
    FxDmaEnabler *          pDmaEnabler;
    FxDeviceBase *          pDevice;
    NTSTATUS                status;
    WDFDMAENABLER           handle;
    PFX_DRIVER_GLOBALS      pFxDriverGlobals;
    FxObject     *          pParent;
    WDF_DMA_ENABLER_CONFIG  dmaConfig;

    //
    // Validate the Device handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE_BASE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, DmaEnablerHandle);
    FxPointerNotNull(pFxDriverGlobals, Config);

    *DmaEnablerHandle = NULL;

    status = FxValidateObjectAttributes(pFxDriverGlobals, Attributes);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Attributes != NULL && Attributes->ParentObject != NULL) {
        FxObjectHandleGetPtr(pFxDriverGlobals,
                             Attributes->ParentObject,
                             FX_TYPE_OBJECT,
                             (PVOID*)&pParent);

        if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11)) {
            FxDeviceBase * pSearchDevice;

            //
            // If a parent object is passed-in it must be descendent of device.
            // DmaEnabler stores device and uses it during dispose
            // (to remove it from dmaenabler list maintained at device level),
            // so DmaEnabler cannot outlive device.
            //

            pSearchDevice = FxDeviceBase::_SearchForDevice(pParent, NULL);

            if (pSearchDevice == NULL) {
                status = STATUS_WDF_OBJECT_ATTRIBUTES_INVALID;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "Attributes->ParentObject 0x%p must have WDFDEVICE as an "
                    "eventual ancestor, %!STATUS!",
                    Attributes->ParentObject, status);

                return status;
            }
            else if (pSearchDevice != pDevice) {
                status = STATUS_WDF_OBJECT_ATTRIBUTES_INVALID;

                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "Attributes->ParentObject 0x%p ancestor is WDFDEVICE %p, but "
                    "not the same WDFDEVICE 0x%p passed to WdfDmaEnablerCreate, "
                    "%!STATUS!",
                    Attributes->ParentObject, pSearchDevice->GetHandle(),
                    Device, status);

                return status;
            }
        }
        else {
            //
            // For < 1.11 drivers we only allow pDevice to be the parent
            // since that is what we were blindly setting the parent to.
            //
            // Using the passed-in parent for such drivers could cause
            // side-effects such as earlier deletion of DmaEnabler object. So
            // we don't do that.
            //
            // We cause this verifier breakpoint to warn downlevel drivers
            // that the parent they passed in gets ignored.
            //
            if (pParent != pDevice) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDMA,
                    "For drivers bound to version <= 1.9 "
                    "WdfDmaEnablerCreate uses WDFDEVICE as the "
                    "parent object for the dma enabler object. "
                    "Attributes->ParentObject 0x%p, which is different from "
                    "WDFDEVICE 0x%p, gets ignored. Please note that DmaEnabler "
                    "would be disposed only when device is disposed.",
                    Attributes->ParentObject, Device);

                if (pFxDriverGlobals->IsDownlevelVerificationEnabled()) {
                    FxVerifierDbgBreakPoint(pFxDriverGlobals);
                }
            }

            pParent = pDevice;
        }
    }
    else {
        pParent = pDevice;
    }

    {
        ULONG expectedSize = pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11) ?
            sizeof(WDF_DMA_ENABLER_CONFIG) :
            sizeof(WDF_DMA_ENABLER_CONFIG_V1_9);

        if (Config->Size != expectedSize) {
            status = STATUS_INFO_LENGTH_MISMATCH;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                "WDF_DMA_ENABLER_CONFIG Size 0x%x, expected 0x%x, %!STATUS!",
                Config->Size, expectedSize, status);

            return status;
        }


        //
        // Normalize DMA config structure if necessary.
        //
        if (Config->Size < sizeof(WDF_DMA_ENABLER_CONFIG)) {
            //
            // Init new fields to default values.
            //
            WDF_DMA_ENABLER_CONFIG_INIT(&dmaConfig,
                                        Config->Profile,
                                        Config->MaximumLength);
            //
            // Copy over existing fields and readjust the struct size.
            //
            RtlCopyMemory(&dmaConfig, Config, Config->Size);
            dmaConfig.Size = sizeof(dmaConfig);

            //
            // Use new config structure from now on.
            //
            Config = &dmaConfig;
        }
    }

    switch (Config->Profile) {
        case WdfDmaProfilePacket:
        case WdfDmaProfileScatterGather:
        case WdfDmaProfilePacket64:
        case WdfDmaProfileScatterGather64:
        case WdfDmaProfileScatterGather64Duplex:
        case WdfDmaProfileScatterGatherDuplex:
        case WdfDmaProfileSystem:
        case WdfDmaProfileSystemDuplex:
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                "DMA Profile value %d is unknown, %!STATUS!",
                Config->Profile, status);
            return status;
    }

    if (Config->MaximumLength == 0) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Config MaximumLength of zero is invalid, %!STATUS!",
            status);

        return status;
    }

    //
    // Ok: create a new DmaEnabler
    //
    pDmaEnabler = new(pFxDriverGlobals, Attributes)
        FxDmaEnabler( pFxDriverGlobals );

    if (pDmaEnabler == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Could not allocate memory for a WDFDMAENABLER, "
                            "%!STATUS!", status);

        return status;
    }

    //
    // Assign this FxDmaEnabler to its parent FxDevice object.
    //
    status = pDmaEnabler->Commit(Attributes, (WDFOBJECT*)&handle, pParent);

    if (NT_SUCCESS(status)) {
        //
        // Ok: start this DmaEnabler.
        //
        status = pDmaEnabler->Initialize( Config, pDevice );
    }

    if (NT_SUCCESS(status)) {
        //
        // Only return a valid handle on success.
        //
        *DmaEnablerHandle = handle;
    }
    else {
        pDmaEnabler->DeleteFromFailedCreate();
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaEnablerGetMaximumLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler
    )
{
    FxDmaEnabler * pDmaEnabler;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaEnabler,
                         FX_TYPE_DMA_ENABLER,
                         (PVOID *) &pDmaEnabler);

    return pDmaEnabler->GetMaximumLength();
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler
    )
{
    FxDmaEnabler * pDmaEnabler;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaEnabler,
                         FX_TYPE_DMA_ENABLER,
                         (PVOID *) &pDmaEnabler);

    return pDmaEnabler->GetMaxSGElements();
}


__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfDmaEnablerSetMaximumScatterGatherElements)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    __drv_when(MaximumElements == 0, __drv_reportError(MaximumElements cannot be zero))
    size_t MaximumElements
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDmaEnabler * pDmaEnabler;
    NTSTATUS      status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaEnabler,
                                   FX_TYPE_DMA_ENABLER,
                                   (PVOID *) &pDmaEnabler,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    if (MaximumElements == 0) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Cannot set MaximumElements of zero on WDFDMAENABLER %p",
            DmaEnabler);
        return;
    }

    pDmaEnabler->SetMaxSGElements(MaximumElements);
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaEnablerGetFragmentLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION   DmaDirection
    )
{
    FxDmaEnabler * pDmaEnabler;
    size_t length;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaEnabler,
                                   FX_TYPE_DMA_ENABLER,
                                   (PVOID *) &pDmaEnabler,
                                   &pFxDriverGlobals);

    switch (DmaDirection) {

    case WdfDmaDirectionReadFromDevice:
        length = pDmaEnabler->GetReadDmaDescription()->MaximumFragmentLength;
        break;

    case WdfDmaDirectionWriteToDevice:
        length = pDmaEnabler->GetWriteDmaDescription()->MaximumFragmentLength;
        break;

    default:
        length = 0;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Invalid value for Dma direction %d, %p",
                            DmaDirection, DmaEnabler);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        break;
    }

    return length;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDMA_ADAPTER
WDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    WDF_DMA_DIRECTION DmaDirection
    )
{
    PDMA_ADAPTER adapter;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDmaEnabler * pDmaEnabler;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaEnabler,
                                   FX_TYPE_DMA_ENABLER,
                                   (PVOID *) &pDmaEnabler,
                                   &pFxDriverGlobals);

    switch (DmaDirection) {

    case WdfDmaDirectionReadFromDevice:
        adapter = pDmaEnabler->GetReadDmaDescription()->AdapterObject;
        break;

    case WdfDmaDirectionWriteToDevice:
        adapter = pDmaEnabler->GetWriteDmaDescription()->AdapterObject;
        break;

    default:
        adapter = NULL;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Invalid value for Dma direction %d, %p",
                            DmaDirection, DmaEnabler);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        break;
    }

    return adapter;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaEnablerConfigureSystemProfile)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in
    PWDF_DMA_SYSTEM_PROFILE_CONFIG ProfileConfig,
    __in
    WDF_DMA_DIRECTION ConfigDirection
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDmaEnabler * pDmaEnabler;

    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaEnabler,
                                   FX_TYPE_DMA_ENABLER,
                                   (PVOID *) &pDmaEnabler,
                                   &pFxDriverGlobals);

    //
    // Verify the DMA config
    //

    if (ProfileConfig == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "ProfileConfig must be non-null, %!STATUS!",
                            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    if (ProfileConfig->Size != sizeof(WDF_DMA_SYSTEM_PROFILE_CONFIG))
    {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "WDF_DMA_SYSTEM_PROFILE_CONFIG Size 0x%x, expected 0x%x, %!STATUS!",
                            ProfileConfig->Size, sizeof(WDF_DMA_SYSTEM_PROFILE_CONFIG), status);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return status;
    }

    if (ProfileConfig->DmaDescriptor == NULL)
    {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "ProfileConfig (%p) may not have NULL DmaDescriptor, %!STATUS!",
                            ProfileConfig, status);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return status;
    }

    if (ConfigDirection != WdfDmaDirectionReadFromDevice &&
        ConfigDirection != WdfDmaDirectionWriteToDevice) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "ConfigDirection 0x%x is an invalid value, %!STATUS!",
            ConfigDirection, status);
        return status;
    }

    status = pDmaEnabler->ConfigureSystemAdapter(ProfileConfig,
                                                 ConfigDirection);
    return status;
}

} // extern "C"
