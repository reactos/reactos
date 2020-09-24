/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbDeviceKm.cpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

extern "C" {
#include <initguid.h>
}

#include "fxusbpch.hpp"


extern "C" {
#include "FxUsbDeviceKm.tmh"
}







#define UCHAR_MAX (0xff)


_Must_inspect_result_
NTSTATUS
FxUsbDevice::InitDevice(
    __in ULONG USBDClientContractVersionForWdfClient
    )
{
    URB urb;
    FxSyncRequest request(GetDriverGlobals(), NULL);
    WDF_REQUEST_SEND_OPTIONS options;
    NTSTATUS status;
    ULONG size;

    RtlZeroMemory(&urb, sizeof(urb));

    if (USBDClientContractVersionForWdfClient != USBD_CLIENT_CONTRACT_VERSION_INVALID) {

        //
        // Register with USBDEX.lib
        //
        status = USBD_CreateHandle(m_InStackDevice,
                                   m_TargetDevice,
                                   USBDClientContractVersionForWdfClient,
                                   GetDriverGlobals()->Tag,
                                   &m_USBDHandle);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "USBD_CreateHandle failed, %!STATUS!", status);
            goto Done;
        }

        m_UrbType = FxUrbTypeUsbdAllocated;
    }

    status = request.m_TrueRequest->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    UsbBuildGetDescriptorRequest(&urb,
                                 sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_DEVICE_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 &m_DeviceDescriptor,
                                 NULL,
                                 sizeof(m_DeviceDescriptor),
                                 NULL);

    FxFormatUsbRequest(request.m_TrueRequest, &urb, FxUrbTypeLegacy, NULL);

    WDF_REQUEST_SEND_OPTIONS_INIT(&options, 0);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options, WDF_REL_TIMEOUT_IN_SEC(5));

    status = SubmitSync(request.m_TrueRequest, &options);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve device descriptor, %!STATUS!", status);
        goto Done;
    }

    //
    // After successfully completing any default control URB, USBD/usbport fills
    // in the PipeHandle field of the URB (it is the same offset in every URB,
    // which this CASSERT verifies.  Since USBD_DEFAULT_PIPE_TRANSFER is not
    // supported by USBD (it is by USBPORT), this is the prescribed way of getting
    // the default control pipe so that you can set the PipeHandle field in
    // _URB_CONTROL_TRANSFER.
    //
    WDFCASSERT(FIELD_OFFSET(_URB_CONTROL_TRANSFER, PipeHandle) ==
               FIELD_OFFSET(_URB_CONTROL_DESCRIPTOR_REQUEST, Reserved));

    m_ControlPipe = urb.UrbControlDescriptorRequest.Reserved;

    USB_CONFIGURATION_DESCRIPTOR config;

    RtlZeroMemory(&config, sizeof(config));
    size = sizeof(config);

    UsbBuildGetDescriptorRequest(&urb,
                                 sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 &config,
                                 NULL,
                                 size,
                                 NULL);

    request.m_TrueRequest->GetSubmitFxIrp()->Reuse(STATUS_SUCCESS);
    request.m_TrueRequest->ClearFieldsForReuse();
    FxFormatUsbRequest(request.m_TrueRequest, &urb, FxUrbTypeLegacy, NULL);

    status = SubmitSync(request.m_TrueRequest, &options);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve config descriptor size, %!STATUS!", status);
        goto Done;
    }
    else if (urb.UrbControlDescriptorRequest.TransferBufferLength == 0) {
        //
        // Not enough info returned
        //
        status = STATUS_UNSUCCESSFUL;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve config descriptor size, zero bytes transferred, "
            " %!STATUS!", status);

        goto Done;
    }
    else if (config.wTotalLength < size) {
        //
        // Not enough info returned
        //
        status = STATUS_UNSUCCESSFUL;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve config descriptor size, config.wTotalLength %d < "
            "sizeof(config descriptor) (%d), %!STATUS!",
            config.wTotalLength, size, status);

        goto Done;
    }

    //
    // Allocate an additional memory at the end of the buffer so if we
    // accidentily access fields beyond the end of the buffer we don't crash

    //





    size = config.wTotalLength;
    ULONG paddedSize = size + sizeof(USB_DEVICE_DESCRIPTOR);

    m_ConfigDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)
        FxPoolAllocate(GetDriverGlobals(),
                       NonPagedPool,
                       paddedSize);

    if (m_ConfigDescriptor == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not allocate %d bytes for config descriptor, %!STATUS!",
            paddedSize, status);

        goto Done;
    }

    RtlZeroMemory(m_ConfigDescriptor, paddedSize);

    UsbBuildGetDescriptorRequest(&urb,
                                 sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                 0,
                                 0,
                                 m_ConfigDescriptor,
                                 NULL,
                                 size,
                                 NULL);

    request.m_TrueRequest->GetSubmitFxIrp()->Reuse(STATUS_SUCCESS);
    request.m_TrueRequest->ClearFieldsForReuse();
    FxFormatUsbRequest(request.m_TrueRequest, &urb, FxUrbTypeLegacy, NULL);

    status = SubmitSync(request.m_TrueRequest, &options);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve config descriptor, %!STATUS!", status);
        goto Done;
    } else if(m_ConfigDescriptor->wTotalLength != size) {
        //
        // Invalid wTotalLength
        //
        status = STATUS_DEVICE_DATA_ERROR;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Defective USB device reported two different config descriptor "
            "wTotalLength values: %d and %d, %!STATUS!",
            size, m_ConfigDescriptor->wTotalLength, status);
        goto Done;
    }

    //
    // Check to see if we are wait wake capable
    //
    if (m_ConfigDescriptor->bmAttributes & USB_CONFIG_REMOTE_WAKEUP) {
        m_Traits |= WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE;
    }

    //
    // Check to see if we are self or bus powered
    //
    USHORT deviceStatus;

    UsbBuildGetStatusRequest(&urb,
                             URB_FUNCTION_GET_STATUS_FROM_DEVICE,
                             0,
                             &deviceStatus,
                             NULL,
                             NULL);

    request.m_TrueRequest->GetSubmitFxIrp()->Reuse(STATUS_SUCCESS);
    request.m_TrueRequest->ClearFieldsForReuse();
    FxFormatUsbRequest(request.m_TrueRequest, &urb, FxUrbTypeLegacy, NULL);

    status = SubmitSync(request.m_TrueRequest, &options);
    if (NT_SUCCESS(status) && (deviceStatus & USB_GETSTATUS_SELF_POWERED)) {
        m_Traits |= WDF_USB_DEVICE_TRAIT_SELF_POWERED;
    }

    //
    // Revert back to success b/c we don't care if the usb device get status
    // fails
    //
    status = STATUS_SUCCESS;

    USB_BUS_INTERFACE_USBDI_V1 busIf;

    RtlZeroMemory(&busIf, sizeof(busIf));

    //
    // All PNP irps must have this initial status
    //
    request.m_TrueRequest->GetSubmitFxIrp()->Reuse(STATUS_NOT_SUPPORTED);
    request.m_TrueRequest->ClearFieldsForReuse();

    FxQueryInterface::_FormatIrp(
        request.m_TrueRequest->GetSubmitFxIrp()->GetIrp(),
        &USB_BUS_INTERFACE_USBDI_GUID,
        (PINTERFACE) &busIf,
        sizeof(busIf),
        USB_BUSIF_USBDI_VERSION_1);

    request.m_TrueRequest->VerifierSetFormatted();

    status = SubmitSync(request.m_TrueRequest);

    if (!NT_SUCCESS(status)) {
        //
        // Retry with the older interface
        //
        RtlZeroMemory(&busIf, sizeof(busIf));

        //
        // All PNP irps must have this initial status
        //

        request.m_TrueRequest->GetSubmitFxIrp()->Reuse(STATUS_NOT_SUPPORTED);
        request.m_TrueRequest->ClearFieldsForReuse();

        FxQueryInterface::_FormatIrp(
            request.m_TrueRequest->GetSubmitFxIrp()->GetIrp(),
            &USB_BUS_INTERFACE_USBDI_GUID,
            (PINTERFACE) &busIf,
            sizeof(USB_BUS_INTERFACE_USBDI_V0),
            USB_BUSIF_USBDI_VERSION_0);

        request.m_TrueRequest->VerifierSetFormatted();

        status = SubmitSync(request.m_TrueRequest);
    }

    if (NT_SUCCESS(status)) {
        //
        // Need to check for NULL b/c we may have only retrieved the V0 interface
        //
        if (busIf.IsDeviceHighSpeed != NULL &&
            busIf.IsDeviceHighSpeed(busIf.BusContext)) {
            m_Traits |= WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED;
        }

        //
        // Stash these off for later
        //
        m_QueryBusTime = busIf.QueryBusTime;
        m_BusInterfaceContext = busIf.BusContext;
        m_BusInterfaceDereference = busIf.InterfaceDereference;

        ASSERT(busIf.GetUSBDIVersion != NULL);
        busIf.GetUSBDIVersion(busIf.BusContext,
                              &m_UsbdVersionInformation,
                              &m_HcdPortCapabilities);
    }
    else if (status == STATUS_NOT_SUPPORTED) {
        //
        // We will only use m_ControlPipe on stacks which do not support
        // USBD_DEFAULT_PIPE_TRANSFER.  If all the QIs failed, then we know
        // definitively that we are running on USBD and we need m_ControlPipe
        // to be != NULL for later control transfers
        //
        ASSERT(m_ControlPipe != NULL);

        m_OnUSBD = TRUE;

        //
        // The QI failed with not supported, do not return error
        //
        status = STATUS_SUCCESS;
    }
    else {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Query Interface for bus returned error, %!STATUS!", status);
    }

Done:

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::GetString(
    __in_ecount(*NumCharacters) PUSHORT String,
    __in PUSHORT NumCharacters,
    __in UCHAR StringIndex,
    __in_opt USHORT LangID,
    __in_opt WDFREQUEST Request,
    __in_opt PWDF_REQUEST_SEND_OPTIONS Options
    )
{
    PUSB_STRING_DESCRIPTOR pDescriptor;
    PVOID buffer;
    _URB_CONTROL_DESCRIPTOR_REQUEST urb;
    WDF_REQUEST_SEND_OPTIONS options, *pOptions;
    USB_COMMON_DESCRIPTOR common;
    ULONG length;
    NTSTATUS status;

    FxSyncRequest request(GetDriverGlobals(), NULL, Request);

    //
    // FxSyncRequest always succeesds for KM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    buffer = NULL;

    status = request.m_TrueRequest->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    RtlZeroMemory(&urb, sizeof(urb));

    if (String != NULL) {
        length = sizeof(USB_STRING_DESCRIPTOR) + (*NumCharacters - 1) * sizeof(WCHAR);

        buffer = FxPoolAllocate(GetDriverGlobals(),
                                NonPagedPool,
                                length);

        if (buffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Done;
        }

        RtlZeroMemory(buffer, length);
        pDescriptor = (PUSB_STRING_DESCRIPTOR) buffer;
    }
    else {
        RtlZeroMemory(&common, sizeof(common));

        length = sizeof(USB_COMMON_DESCRIPTOR);
        pDescriptor = (PUSB_STRING_DESCRIPTOR) &common;
    }

    UsbBuildGetDescriptorRequest((PURB) &urb,
                                 sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST),
                                 USB_STRING_DESCRIPTOR_TYPE,
                                 StringIndex,
                                 LangID,
                                 pDescriptor,
                                 NULL,
                                 length,
                                 NULL);

    if (Options != NULL) {
        pOptions = Options;
    }
    else {
        WDF_REQUEST_SEND_OPTIONS_INIT(&options, 0);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options,
                                             WDF_REL_TIMEOUT_IN_SEC(2));

        pOptions = &options;
    }
#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "this annotation change in usb.h is communicated to usb team")
    FxFormatUsbRequest(request.m_TrueRequest, (PURB) &urb, FxUrbTypeLegacy, NULL);
    status = SubmitSync(request.m_TrueRequest, pOptions);

    if (NT_SUCCESS(status)) {
        USHORT numChars;

        //
        // Make sure we got an even number of bytes and that we got a header
        //
        if ((pDescriptor->bLength & 0x1) ||
            pDescriptor->bLength < sizeof(USB_COMMON_DESCRIPTOR)) {
            status = STATUS_DEVICE_DATA_ERROR;
        }
        else {
            //
            // bLength is the length of the entire descriptor.  Subtract off
            // the descriptor header and then divide by the size of a WCHAR.
            //
            numChars =
                (pDescriptor->bLength - sizeof(USB_COMMON_DESCRIPTOR)) / sizeof(WCHAR);

            if (String != NULL) {
                if (*NumCharacters >= numChars) {
                    length = numChars * sizeof(WCHAR);
                }
                else {
                    length = *NumCharacters * sizeof(WCHAR);
                    status = STATUS_BUFFER_OVERFLOW;
                }

                *NumCharacters = numChars;
                RtlCopyMemory(String, pDescriptor->bString, length);
            }
            else {
                *NumCharacters = numChars;
            }
        }
    }

    if (buffer != NULL) {
        FxPoolFree(buffer);
    }

Done:

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::FormatStringRequest(
    __in FxRequestBase* Request,
    __in FxRequestBuffer *RequestBuffer,
    __in UCHAR StringIndex,
    __in USHORT LangID
    )
/*++

Routine Description:
    Formats a request to retrieve a string from a string descriptor

Arguments:
    Request - request to format

    RequestBuffer - Buffer to be filled in when the request has completed

    StringIndex - index of the string

    LandID - language ID of the string to be retrieved

Return Value:
    NTSTATUS

  --*/
{
    FxUsbDeviceStringContext* pContext;
    NTSTATUS status;
    FX_URB_TYPE urbType;

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFUSBDEVICE %p, Request %p, setting target failed, "
                            "%!STATUS!", GetHandle(), Request, status);

        return status;
    }

    if (Request->HasContextType(FX_RCT_USB_STRING_REQUEST)) {
        pContext = (FxUsbDeviceStringContext*) Request->GetContext();
    }
    else {

        urbType = GetFxUrbTypeForRequest(Request);
        pContext = new(GetDriverGlobals()) FxUsbDeviceStringContext(urbType);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (urbType == FxUrbTypeUsbdAllocated) {
            status = pContext->AllocateUrb(m_USBDHandle);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "FxUsbDeviceStringContext::AllocateUrb failed, %!STATUS!", status);
                delete pContext;
                return status;
            }

            //
            // Since the AllocateUrb routine calls USBD_xxxUrbAllocate APIs to allocate an Urb, it is
            // important to release those resorces before the devnode is removed. Those
            // resoruces are removed at the time Request is disposed.
            //
            Request->EnableContextDisposeNotification();
        }

        Request->SetContext(pContext);
    }

    status = pContext->AllocateDescriptor(GetDriverGlobals(),
                                          RequestBuffer->GetBufferLength());
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pContext->StoreAndReferenceMemory(RequestBuffer);
    pContext->SetUrbInfo(StringIndex, LangID);

    if (pContext->m_Urb == &pContext->m_UrbLegacy) {
        urbType = FxUrbTypeLegacy;
    }
    else {
        urbType = FxUrbTypeUsbdAllocated;
    }

    FxFormatUsbRequest(Request, (PURB)pContext->m_Urb, urbType, m_USBDHandle);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::FormatControlRequest(
    __in FxRequestBase* Request,
    __in PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    __in FxRequestBuffer *RequestBuffer
    )
{
    FxUsbDeviceControlContext* pContext;
    NTSTATUS status;
    size_t bufferSize;
    FX_URB_TYPE urbType;

    bufferSize = RequestBuffer->GetBufferLength();

    //
    // We can only transfer 2 bytes worth of data, so if the buffer is larger,
    // fail here.
    //
    if (bufferSize > 0xFFFF) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Control transfer buffer is limited to 0xFFFF bytes in size, "
            "%I64d requested ", bufferSize);

        return STATUS_INVALID_PARAMETER;
    }

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFUSBDEVICE %p, Request %p, setting target failed, "
                            "%!STATUS!", GetHandle(), Request, status);

        return status;
    }

    if (Request->HasContextType(FX_RCT_USB_CONTROL_REQUEST)) {
        pContext = (FxUsbDeviceControlContext*) Request->GetContext();
    }
    else {

        urbType = GetFxUrbTypeForRequest(Request);
        pContext = new(GetDriverGlobals()) FxUsbDeviceControlContext(urbType);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (urbType == FxUrbTypeUsbdAllocated) {
            status = pContext->AllocateUrb(m_USBDHandle);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                    "FxUsbDeviceControlContext::AllocateUrb Failed, %!STATUS!", status);

                delete pContext;
                return status;
            }
            //
            // Since the AllocateUrb routine calls USBD_xxxUrbAllocate APIs to allocate an Urb, it is
            // important to release those resorces before the devnode is removed. Those
            // resoruces are removed at the time Request is disposed.
            //
            Request->EnableContextDisposeNotification();
        }

        Request->SetContext(pContext);
    }

    if (RequestBuffer->HasMdl()) {
        PMDL pMdl;

        pMdl = NULL;
        ASSERT(pContext->m_PartialMdl == NULL);

        status = RequestBuffer->GetOrAllocateMdl(GetDriverGlobals(),
                                                 &pMdl,
                                                 &pContext->m_PartialMdl,
                                                 &pContext->m_UnlockPages,
                                                 IoModifyAccess);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        ASSERT(pMdl != NULL);
    }

    pContext->StoreAndReferenceMemory(this, RequestBuffer, SetupPacket);

    if (pContext->m_Urb == &pContext->m_UrbLegacy) {
        urbType = FxUrbTypeLegacy;
    }
    else {
        urbType = FxUrbTypeUsbdAllocated;
    }

    FxFormatUsbRequest(Request, (PURB)pContext->m_Urb, urbType, m_USBDHandle);

    return STATUS_SUCCESS;
}

VOID
FxUsbDeviceControlContext::StoreAndReferenceMemory(
    __in FxUsbDevice* Device,
    __in FxRequestBuffer* Buffer,
    __in PWDF_USB_CONTROL_SETUP_PACKET SetupPacket
    )
{
    SetUsbType(WdfUsbRequestTypeDeviceControlTransfer);

    RtlZeroMemory(m_Urb, sizeof(*m_Urb));

    m_Urb->Hdr.Function = URB_FUNCTION_CONTROL_TRANSFER;
    m_Urb->Hdr.Length = sizeof(*m_Urb);

    __super::StoreAndReferenceMemory(Buffer);

    //
    // Set the values using what is stored in the buffer
    //
    Buffer->AssignValues(&m_Urb->TransferBuffer,
                         &m_Urb->TransferBufferMDL,
                         &m_Urb->TransferBufferLength);

    RtlCopyMemory(&m_Urb->SetupPacket[0],
                  &SetupPacket->Generic.Bytes[0],
                  sizeof(m_Urb->SetupPacket));

    //
    // also indicate the length of the buffer in the header
    //
    ((PWDF_USB_CONTROL_SETUP_PACKET) &m_Urb->SetupPacket[0])->Packet.wLength =
        (USHORT) m_Urb->TransferBufferLength;

    //
    // Control transfers are always short OK.  USBD_TRANSFER_DIRECTION_IN may
    // be OR'ed in later.
    //
    m_Urb->TransferFlags = USBD_SHORT_TRANSFER_OK;

    //
    // Get the direction out of the setup packet
    //
    if (SetupPacket->Packet.bm.Request.Dir == BMREQUEST_DEVICE_TO_HOST) {
        m_Urb->TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
    }

    if (Device->OnUSBD()) {
        m_Urb->PipeHandle = Device->GetControlPipeHandle();
    }
    else {
        //
        // USBPORT supports this flag
        //
        m_Urb->TransferFlags |= USBD_DEFAULT_PIPE_TRANSFER;
    }

    //
    // If we have built a partial MDL, use that instead.  TransferBufferLength
    // is still valid because the Offsets or length in Buffer will have been
    // used to create this PartialMdl by the caller.
    //
    if (m_PartialMdl != NULL) {
        m_Urb->TransferBufferMDL = m_PartialMdl;
    }
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::QueryUsbCapability(
    __in
    CONST GUID* CapabilityType,
    __in
    ULONG CapabilityBufferLength,
    __drv_when(CapabilityBufferLength == 0, __out_opt)
    __drv_when(CapabilityBufferLength != 0 && ResultLength == NULL, __out_bcount(CapabilityBufferLength))
    __drv_when(CapabilityBufferLength != 0 && ResultLength != NULL, __out_bcount_part_opt(CapabilityBufferLength, *ResultLength))
    PVOID CapabilityBuffer,
    __out_opt
    __drv_when(ResultLength != NULL,__deref_out_range(<=,CapabilityBufferLength))
    PULONG ResultLength
   )
{
    NTSTATUS status;

    if (ResultLength != NULL) {
        *ResultLength = 0;
    }

    if (GetUSBDHandle() == NULL) {
        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE must have been created using WdfUsbTargetDeviceCreateWithParameters, %!STATUS!",
            status);

        return status;
    }

    status = USBD_QueryUsbCapability(m_USBDHandle,
                                     CapabilityType,
                                     CapabilityBufferLength,
                                     (PUCHAR) CapabilityBuffer,
                                     ResultLength);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve capability %!GUID!, %!STATUS!",
            CapabilityType, status);
        goto exit;
    }

exit:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SelectConfigSingle(
    __in PWDF_OBJECT_ATTRIBUTES PipeAttributes,
    __in PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
/*++

Routine Description:
    This will configure the single inteface case and pick up the first available
    setting. If there are multiple settings on  a single interface device
    and the driver wants to pick one then the driver should use the multinterface
    option to initialize.

    This takes care of the simplest case only.  Configuring a multi interface
    device as a single interface device would be treated as an error.  There is
    duplication of code with the multi case but it is better to keep these two
    separate especially if more gets added.

Arguments:


Return Value:
    NTSTATUS

  --*/
{
    //
    // The array needs an extra element which is zero'd out to mark the end
    //
    USBD_INTERFACE_LIST_ENTRY listEntry[2];
    PURB urb;
    NTSTATUS status;

    RtlZeroMemory(&Params->Types.SingleInterface,
                  sizeof(Params->Types.SingleInterface));

    if (m_NumInterfaces > 1) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE %p cannot be auto configured for a single interface "
            "since there are %d interfaces on the device, %!STATUS!",
            GetHandle(), m_NumInterfaces, status);

        return status;
    }

    RtlZeroMemory(&listEntry[0], sizeof(listEntry));

    //
    // Use AlternateSetting 0 by default
    //
    listEntry[0].InterfaceDescriptor = m_Interfaces[0]->GetSettingDescriptor(0);

    if (listEntry[0].InterfaceDescriptor == NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE %p could not retrieve AlternateSetting 0 for "
            "bInterfaceNumber %d", GetHandle(),
            m_Interfaces[0]->m_InterfaceNumber);

        return STATUS_INVALID_PARAMETER;
    }

    urb = FxUsbCreateConfigRequest(GetDriverGlobals(),
                                   m_ConfigDescriptor,
                                   &listEntry[0],
                                   GetDefaultMaxTransferSize());

    if (urb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        status = SelectConfig(PipeAttributes, urb, FxUrbTypeLegacy, NULL);

        if (NT_SUCCESS(status)) {
            Params->Types.SingleInterface.NumberConfiguredPipes  =
                 m_Interfaces[0]->GetNumConfiguredPipes();

            Params->Types.SingleInterface.ConfiguredUsbInterface =
                m_Interfaces[0]->GetHandle();
        }

        FxPoolFree(urb);
        urb = NULL;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SelectConfigMulti(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
/*++

Routine Description:
    Selects the configuration as described by the parameter Params.  If there is a
    previous active configuration, the WDFUSBPIPEs for it are stopped and
    destroyed before the new configuration is selected

Arguments:
    PipesAttributes - object attributes to apply to each created WDFUSBPIPE

    Params -

Return Value:
    NTSTATUS

  --*/
{
    PUSBD_INTERFACE_LIST_ENTRY pList;
    PURB urb;
    NTSTATUS status;
    ULONG size;
    UCHAR i;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    Params->Types.MultiInterface.NumberOfConfiguredInterfaces = 0;

    //
    // The array needs an extra element which is zero'd out to mark the end
    //
    size = sizeof(USBD_INTERFACE_LIST_ENTRY) * (m_NumInterfaces + 1);
    pList = (PUSBD_INTERFACE_LIST_ENTRY) FxPoolAllocate(
        pFxDriverGlobals,
        NonPagedPool,
        size
        );

    if (pList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pList, size);

    if (Params->Type == WdfUsbTargetDeviceSelectConfigTypeMultiInterface) {
        for (i = 0; i < m_NumInterfaces; i++) {
            pList[i].InterfaceDescriptor =
                m_Interfaces[i]->GetSettingDescriptor(0);

            if (pList[i].InterfaceDescriptor == NULL) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "WDFUSBDEVICE %p could not retrieve AlternateSetting 0 for "
                    "bInterfaceNumber %d", GetHandle(),
                    m_Interfaces[i]->m_InterfaceNumber);

                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }
        }
    }
    else {
        //
        // Type is WdfUsbTargetDeviceSelectConfigTypeInterfacesPairs
        //
        UCHAR interfacePairsNum = 0;
        UCHAR bitArray[UCHAR_MAX/sizeof(UCHAR)];

        //
        // initialize the bit array
        //
        RtlZeroMemory(bitArray, sizeof(bitArray));
        //
        // Build a list of descriptors from the Setting pairs
        // passed in by the user. There could be interfaces not
        // covered in the setting/interface pairs array passed.
        // If that is the case return STATUS_INVALID_PARAMETER
        //
        for (i = 0; i < Params->Types.MultiInterface.NumberInterfaces ; i++) {
            PWDF_USB_INTERFACE_SETTING_PAIR settingPair;
            UCHAR interfaceNumber;
            UCHAR altSettingIndex;

            settingPair = &Params->Types.MultiInterface.Pairs[i];

            status = GetInterfaceNumberFromInterface(
                settingPair->UsbInterface,
                &interfaceNumber
                );

            //
            //convert the interface handle to interface number
            //
            if (NT_SUCCESS(status)) {
                altSettingIndex = settingPair->SettingIndex;

                //
                // do the following only if the bit is not already set
                //
                if (FxBitArraySet(&bitArray[0], interfaceNumber) == FALSE) {
                    pList[interfacePairsNum].InterfaceDescriptor =
                         FxUsbParseConfigurationDescriptor(
                             m_ConfigDescriptor,
                             interfaceNumber,
                             altSettingIndex
                             );

                    if (pList[interfacePairsNum].InterfaceDescriptor == NULL) {
                        status = STATUS_INVALID_PARAMETER;
                        DoTraceLevelMessage(
                            GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGIOTARGET,
                            "WDFUSBDEVICE %p could not retrieve "
                            "AlternateSetting %d for "
                            "bInterfaceNumber %d, returning %!STATUS!",
                            GetHandle(),
                            altSettingIndex, interfaceNumber, status);
                        goto Done;
                    }

                    interfacePairsNum++;
                }
            }
            else {
                goto Done;
            }
        }

        //
        // Check if there are any interfaces not specified by the array. If
        // there are, then select setting 0 for them.
        //
        if (m_NumInterfaces > interfacePairsNum) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFUSBDEVICE %p interface pairs set (%d) is not equal to actual "
                "# of interfaces (%d) reported by the device, %!STATUS!",
                GetObjectHandle(), interfacePairsNum, m_NumInterfaces, status);
            goto Done;
        }
    } //WdfUsbTargetDeviceSelectConfigTypeInterfacesPairs

    urb = FxUsbCreateConfigRequest(
        GetDriverGlobals(),
        m_ConfigDescriptor,
        pList,
        GetDefaultMaxTransferSize()
        );

    if (urb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        status = SelectConfig(
            PipesAttributes,
            urb,
            FxUrbTypeLegacy,
            &Params->Types.MultiInterface.NumberOfConfiguredInterfaces);

        FxPoolFree(urb);
        urb = NULL;
    }

Done:
    FxPoolFree(pList);
    pList = NULL;

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::Reset(
    VOID
    )
{
    FxIoContext context;
    FxSyncRequest request(GetDriverGlobals(), &context);
    FxRequestBuffer emptyBuffer;
    NTSTATUS status;

    //
    // FxSyncRequest always succeesds for KM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    status = FormatIoctlRequest(request.m_TrueRequest,
                                IOCTL_INTERNAL_USB_RESET_PORT,
                                TRUE,
                                &emptyBuffer,
                                &emptyBuffer);
    if (NT_SUCCESS(status)) {
        CancelSentIo();
        status = SubmitSyncRequestIgnoreTargetState(request.m_TrueRequest, NULL);
    }

    return status;
}


