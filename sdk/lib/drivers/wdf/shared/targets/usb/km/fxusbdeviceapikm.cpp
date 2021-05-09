/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbDeviceAPI.cpp

Abstract:


Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbDeviceApiKm.tmh"
}

//
// Extern "C" all APIs
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceRetrieveCurrentFrameNumber)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __out
    PULONG CurrentFrameNumber
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, CurrentFrameNumber);

    return pUsbDevice->GetCurrentFrameNumber(CurrentFrameNumber);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceSendUrbSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_xcount("union bug in SAL")
    PURB Urb
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequestBuffer buf;
    NTSTATUS status;
    FxUsbDevice* pUsbDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxUsbUrbContext context;
    FxSyncRequest request(pFxDriverGlobals, &context, Request);

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p, Urb %p", UsbDevice, Urb);

    FxPointerNotNull(pFxDriverGlobals, Urb);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(pFxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    buf.SetBuffer(Urb, 0);

    status = FxFormatUrbRequest(pFxDriverGlobals,
                                pUsbDevice,
                                request.m_TrueRequest,
                                &buf,
                                pUsbDevice->GetUrbType(),
                                pUsbDevice->GetUSBDHandle());

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBDEVICE %p, WDFREQUEST %p being submitted",
            UsbDevice, request.m_TrueRequest->GetTraceObjectHandle());

        status = pUsbDevice->SubmitSync(request.m_TrueRequest, RequestOptions);
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Device %p, Urb %p, %!STATUS!",
                        UsbDevice, Urb, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceFormatRequestForUrb)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY UrbMemory,
    __in_opt
    PWDFMEMORY_OFFSET UrbOffsets
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxMemory* pMemory;
    FxUsbDevice* pUsbDevice;
    FxRequestBuffer buf;
    FxRequest* pRequest;
    NTSTATUS status;
    size_t bufferSize;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p, Request %p, Memory %p",
                        UsbDevice, Request, UrbMemory);

    FxPointerNotNull(pFxDriverGlobals, UrbMemory);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         UrbMemory,
                         IFX_TYPE_MEMORY,
                         (PVOID*) &pMemory);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    status = pMemory->ValidateMemoryOffsets(UrbOffsets);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    bufferSize = pMemory->GetBufferSize();
    if (UrbOffsets != NULL && UrbOffsets->BufferOffset > 0) {
        bufferSize -= UrbOffsets->BufferOffset;
    }

    if (bufferSize < sizeof(_URB_HEADER)) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "UrbMemory %p buffer size, %I64d, smaller then"
                            "_URB_HEADER, %!STATUS!",
                             UrbMemory, pMemory->GetBufferSize(), status);
        return status;
    }

    buf.SetMemory(pMemory, UrbOffsets);

    status = FxFormatUrbRequest(pFxDriverGlobals,
                                pUsbDevice,
                                pRequest,
                                &buf,
                                pUsbDevice->GetUrbType(),
                                pUsbDevice->GetUSBDHandle());

    if (NT_SUCCESS(status)) {
        FxUsbUrbContext* pContext;
        pContext = (FxUsbUrbContext*) pRequest->GetContext();

        pContext->SetUsbType(WdfUsbRequestTypeDeviceUrb);
        pContext->m_UsbParameters.Parameters.DeviceUrb.Buffer = UrbMemory;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p, Request %p, Memory %p, %!STATUS!",
                        UsbDevice, Request, UrbMemory, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceIsConnectedSynchronous)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pUsbDevice->IsConnected();

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceCyclePortSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice
    )
/*++

Routine Description:
    Synchronously cycles a device on a USB port.  This will cause the device
    to be surprise removed and reenumerated.  Very similar to the reenumerate
    interface we use in the pnp state machine.  Usually a driver will do this
    after it has downloaded firmware and wants to be reenumerated as a new
    device.




Arguments:
    UsbDevice - the IOTARGET representing the device

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pUsbDevice->CyclePort();

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfUsbTargetDeviceFormatRequestForCyclePort)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in
    WDFREQUEST Request
    )
/*++

Routine Description:
    Formats a WDFREQUEST so that it will cycle the port and reenumerate the
    device when sent.

Arguments:
    UsbDevice - the IOTARGET representing the device that will be reenumerated

    Request - the request which will be formatted

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    FxRequest* pRequest;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    status = pUsbDevice->FormatCycleRequest(pRequest);

    return status;
}

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetDeviceCreateUrb)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBDEVICE UsbDevice,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFMEMORY* UrbMemory,
    __deref_opt_out_bcount(sizeof(URB))
    PURB* Urb
    )
/*++

Routine Description:
    Creates a WDFUSBDEVICE handle for the client.

Arguments:
    Attributes - Attributes associated with this object

    UrbMemory - The returned handle to the caller for the allocated Urb

    Urb - (opt) Pointer to the associated urb buffer.

Return Value:
    STATUS_INVALID_PARAMETER - any required parameters are not present/invalid

    STATUS_INVALID_DEVICE_STATE - If the client did not specify a client contract verion while
        creating the WDFUSBDEVICE

    STATUS_INSUFFICIENT_RESOURCES - could not allocated the object that backs
        the handle

    STATUS_SUCCESS - success

    ...

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbDevice* pUsbDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   UsbDevice,
                                   FX_TYPE_IO_TARGET_USB_DEVICE,
                                   (PVOID*) &pUsbDevice,
                                   &pFxDriverGlobals);

    //
    // Basic parameter validation
    //
    FxPointerNotNull(pFxDriverGlobals, UrbMemory);

    if (pUsbDevice->GetUSBDHandle() == NULL) {
        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "USBDEVICE Must have been created with Client Contract Verion Info, %!STATUS!",
            status);

        return status;
    }

    status = pUsbDevice->CreateUrb(Attributes, UrbMemory, Urb);

    return status;
}

} // extern "C"
