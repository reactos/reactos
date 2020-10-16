/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbDeviceUm.cpp

Abstract:

Author:

Environment:

    User mode only

Revision History:

--*/
extern "C" {
#include <initguid.h>

}
#include "fxusbpch.hpp"
extern "C" {
#include "FxUsbDeviceUm.tmh"
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SendSyncRequest(
    __in FxSyncRequest* Request,
    __in ULONGLONG Time
    )
{
    NTSTATUS status;
    WDF_REQUEST_SEND_OPTIONS options;

    WDF_REQUEST_SEND_OPTIONS_INIT(&options, 0);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options, WDF_REL_TIMEOUT_IN_SEC(Time));

    status = SubmitSync(Request->m_TrueRequest, &options);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "FxUsbDevice SubmitSync failed");
        goto Done;
    }

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SendSyncUmUrb(
    __inout PUMURB Urb,
    __in ULONGLONG Time,
    __in_opt WDFREQUEST Request,
    __in_opt PWDF_REQUEST_SEND_OPTIONS Options
    )
{
    NTSTATUS status;
    WDF_REQUEST_SEND_OPTIONS options;
    FxSyncRequest request(GetDriverGlobals(), NULL, Request);

    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    if (NULL == Options) {
        Options = &options;
    }

    WDF_REQUEST_SEND_OPTIONS_INIT(Options, 0);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(Options, WDF_REL_TIMEOUT_IN_SEC(Time));

    status = request.m_TrueRequest->ValidateTarget(this);
    if (NT_SUCCESS(status)) {
        FxUsbUmFormatRequest(request.m_TrueRequest, &Urb->UmUrbHeader, m_pHostTargetFile);
        status = SubmitSync(request.m_TrueRequest, Options);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "FxUsbDevice SubmitSync failed");
            return status;
        }
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::InitDevice(
    __in ULONG USBDClientContractVersionForWdfClient
    )
{
    HRESULT hr = S_OK;
    NTSTATUS status = STATUS_SUCCESS;

    IWudfDevice* device = NULL;
    IWudfDeviceStack2* devstack2 = NULL;

    UMURB urb;
    USB_CONFIGURATION_DESCRIPTOR config;
    USHORT wTotalLength = 0;

    FxRequestBuffer buf;
    FxUsbDeviceControlContext context(FxUrbTypeLegacy);

    WDF_USB_CONTROL_SETUP_PACKET setupPacket;
    USHORT deviceStatus = 0;
    UCHAR deviceSpeed = 0;

    FxSyncRequest request(GetDriverGlobals(), NULL);
    FxSyncRequest request2(GetDriverGlobals(), &context);




    UNREFERENCED_PARAMETER(USBDClientContractVersionForWdfClient);

    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        goto Done;
    }

    status = request2.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize second FxSyncRequest");
        goto Done;
    }

    status = request.m_TrueRequest->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to validate FxSyncRequest target");
        goto Done;
    }

    status = request2.m_TrueRequest->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to validate second FxSyncRequest target");
        goto Done;
    }

    RtlZeroMemory(&urb, sizeof(urb));

    device = m_DeviceBase->GetDeviceObject();
    devstack2 = m_Device->GetDeviceStack2();

    if (m_pHostTargetFile == NULL) {

        //
        // Opens a handle on the reflector for USB side-band communication
        // based on the currently selected dispatcher type.
        //
        hr = devstack2->OpenUSBCommunicationChannel(device,
                                                    device->GetAttachedDevice(),
                                                    &m_pHostTargetFile);

        if (SUCCEEDED(hr)) {
            m_WinUsbHandle = (WINUSB_INTERFACE_HANDLE)m_pHostTargetFile->GetCreateContext();
        }
    }

    //
    // Get USB device descriptor
    //
    FxUsbUmInitDescriptorUrb(&urb,
                             m_WinUsbHandle,
                             USB_DEVICE_DESCRIPTOR_TYPE,
                             sizeof(m_DeviceDescriptor),
                             &m_DeviceDescriptor);
    FxUsbUmFormatRequest(request.m_TrueRequest,
                         &urb.UmUrbHeader,
                         m_pHostTargetFile,
                         TRUE);
    status = SendSyncRequest(&request, 5);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Get USB configuration descriptor
    //
    FxUsbUmInitDescriptorUrb(&urb,
                             m_WinUsbHandle,
                             USB_CONFIGURATION_DESCRIPTOR_TYPE,
                             sizeof(config),
                             &config);
    FxUsbUmFormatRequest(request.m_TrueRequest,
                         &urb.UmUrbHeader,
                         m_pHostTargetFile,
                         TRUE);
    status = SendSyncRequest(&request, 5);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    if (config.wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {

        //
        // Not enough info returned
        //
        status = STATUS_UNSUCCESSFUL;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve config descriptor size, config.wTotalLength %d < "
            "sizeof(config descriptor) (%d), %!STATUS!",
            config.wTotalLength, sizeof(USB_CONFIGURATION_DESCRIPTOR), status);
        goto Done;
    }

    wTotalLength = config.wTotalLength;
    m_ConfigDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)
                             FxPoolAllocate(GetDriverGlobals(),
                                            NonPagedPool,
                                            wTotalLength);
    if (NULL == m_ConfigDescriptor) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not allocate %d bytes for config descriptor, %!STATUS!",
            sizeof(USB_CONFIGURATION_DESCRIPTOR), status);
        goto Done;
    }

    //
    // Get USB configuration descriptor and subsequent interface descriptors, etc.
    //
    FxUsbUmInitDescriptorUrb(&urb,
                             m_WinUsbHandle,
                             USB_CONFIGURATION_DESCRIPTOR_TYPE,
                             wTotalLength,
                             m_ConfigDescriptor);
    FxUsbUmFormatRequest(request.m_TrueRequest,
                         &urb.UmUrbHeader,
                         m_pHostTargetFile,
                         TRUE);
    status = SendSyncRequest(&request, 5);
    if (!NT_SUCCESS(status)) {
        goto Done;
    } else if (m_ConfigDescriptor->wTotalLength != wTotalLength) {
        //
        // Invalid wTotalLength
        //
        status = STATUS_DEVICE_DATA_ERROR;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Defective USB device reported two different config descriptor "
            "wTotalLength values: %d and %d, %!STATUS!",
            wTotalLength, m_ConfigDescriptor->wTotalLength, status);
        goto Done;
    }

    m_NumInterfaces = m_ConfigDescriptor->bNumInterfaces;

    //
    // Check to see if we are wait wake capable
    //
    if (m_ConfigDescriptor->bmAttributes & USB_CONFIG_REMOTE_WAKEUP) {
        m_Traits |= WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE;
    }

    //
    // Get the device status to check if device is self-powered.
    //
    WDF_USB_CONTROL_SETUP_PACKET_INIT_GET_STATUS(&setupPacket,
                                                 BmRequestToDevice,
                                                 0); // Device status

    buf.SetBuffer(&deviceStatus, sizeof(USHORT));

    status = FormatControlRequest(request2.m_TrueRequest,
                                  &setupPacket,
                                  &buf);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    status = SendSyncRequest(&request2, 5);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    if (deviceStatus & USB_GETSTATUS_SELF_POWERED) {
        m_Traits |= WDF_USB_DEVICE_TRAIT_SELF_POWERED;
    }

    //
    // Get device speed information.
    //
    FxUsbUmInitInformationUrb(&urb,
                              m_WinUsbHandle,
                              sizeof(UCHAR),
                              &deviceSpeed);
    FxUsbUmFormatRequest(request.m_TrueRequest,
                         &urb.UmUrbHeader,
                         m_pHostTargetFile,
                         TRUE);
    status = SendSyncRequest(&request, 5);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    if (deviceSpeed == 3) {
        m_Traits |= WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED;
    }

    //
    // User-mode events must be initialized manually.
    //
    status = m_InterfaceIterationLock.Initialize();
    if (!NT_SUCCESS(status)) {
        goto Done;
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
    UMURB urb;
    PUSB_STRING_DESCRIPTOR pDescriptor;
    PVOID buffer = NULL;
    USB_COMMON_DESCRIPTOR common;
    ULONG length;
    NTSTATUS status;

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

    FxUsbUmInitDescriptorUrb(&urb,
                             m_WinUsbHandle,
                             USB_STRING_DESCRIPTOR_TYPE,
                             length,
                             pDescriptor);
    urb.UmUrbDescriptorRequest.Index = StringIndex;
    urb.UmUrbDescriptorRequest.LanguageID = LangID;

    status = SendSyncUmUrb(&urb, 2, Request, Options);
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
    NTSTATUS status;
    FxUsbDeviceStringContext* pContext;

    status = Request->ValidateTarget(this);
    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFUSBDEVICE %p, Request %p, setting target failed, "
                            "%!STATUS!", GetHandle(), Request, status);

        return status;
    }

    if (Request->HasContextType(FX_RCT_USB_STRING_REQUEST)) {
        pContext = (FxUsbDeviceStringContext*) Request->GetContext();
    }
    else {
        pContext = new(GetDriverGlobals()) FxUsbDeviceStringContext(FxUrbTypeLegacy);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Request->SetContext(pContext);
    }

    FxUsbUmInitDescriptorUrb(&pContext->m_UmUrb,
                             m_WinUsbHandle,
                             USB_STRING_DESCRIPTOR_TYPE,
                             0,
                             NULL);

    status = pContext->AllocateDescriptor(GetDriverGlobals(),
                                          RequestBuffer->GetBufferLength());
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pContext->StoreAndReferenceMemory(RequestBuffer);
    pContext->SetUrbInfo(StringIndex, LangID);

    FxUsbUmFormatRequest(Request, &pContext->m_UmUrb.UmUrbHeader, m_pHostTargetFile);

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
        pContext = new(GetDriverGlobals()) FxUsbDeviceControlContext(FxUrbTypeLegacy);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Request->SetContext(pContext);
    }

    FxUsbUmInitControlTransferUrb(&pContext->m_UmUrb,
                                  m_WinUsbHandle,
                                  0,
                                  NULL);

    pContext->StoreAndReferenceMemory(this, RequestBuffer, SetupPacket);

    FxUsbUmFormatRequest(Request, &pContext->m_UmUrb.UmUrbHeader, m_pHostTargetFile);

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

    FxUsbRequestContext::StoreAndReferenceMemory(Buffer); // __super call

    //
    // Convert WDF_USB_CONTROL_SETUP_PACKET to WINUSB_SETUP_PACKET
    //
    m_UmUrb.UmUrbControlTransfer.SetupPacket.RequestType = SetupPacket->Packet.bm.Byte;
    m_UmUrb.UmUrbControlTransfer.SetupPacket.Request = SetupPacket->Packet.bRequest;
    m_UmUrb.UmUrbControlTransfer.SetupPacket.Value = SetupPacket->Packet.wValue.Value;
    m_UmUrb.UmUrbControlTransfer.SetupPacket.Index = SetupPacket->Packet.wIndex.Value;

    //
    // Set the TransferBuffer values using what is stored in the Buffer
    //
    Buffer->AssignValues(&m_UmUrb.UmUrbControlTransfer.TransferBuffer,
                         NULL,
                         &m_UmUrb.UmUrbControlTransfer.TransferBufferLength);

    m_UmUrb.UmUrbControlTransfer.SetupPacket.Length =
        (USHORT)m_UmUrb.UmUrbControlTransfer.TransferBufferLength;
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

    //
    // We cannot send an actual query to the USB stack through winusb.
    // However, we have the information to handle this query. It is not
    // ideal to implement this API in this manner because we are making
    // assumptions about the behavior of USB stack that can change in future.
    // However, it is too late in the OS cycle to implement a correct solution.
    // The ideal way is for winusb to expose this information. We should
    // revisit this API in blue+1
    //
    if (RtlCompareMemory(CapabilityType,
                         &GUID_USB_CAPABILITY_DEVICE_CONNECTION_HIGH_SPEED_COMPATIBLE,
                         sizeof(GUID)) == sizeof(GUID)) {
        if (m_Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_NOT_SUPPORTED;
        }
    }
    else if (RtlCompareMemory(CapabilityType,
                                &GUID_USB_CAPABILITY_DEVICE_CONNECTION_SUPER_SPEED_COMPATIBLE,
                                sizeof(GUID)) == sizeof(GUID)) {
        if (m_DeviceDescriptor.bcdUSB >= 0x300) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_NOT_SUPPORTED;
        }
    }
    else if (RtlCompareMemory(CapabilityType,
                                &GUID_USB_CAPABILITY_SELECTIVE_SUSPEND,
                                sizeof(GUID)) == sizeof(GUID)) {
        //
        // Both EHCI as well as XHCI stack support selective suspend.
        // Since XHCI UCX interface is not open, there aren't any
        // third party controller drivers to worry about. This can
        // of course change in future
        //
        status = STATUS_SUCCESS;
    }
    else if (RtlCompareMemory(CapabilityType,
                                &GUID_USB_CAPABILITY_FUNCTION_SUSPEND,
                                sizeof(GUID)) == sizeof(GUID)) {
        //
        // Note that a SuperSpeed device will report a bcdUSB of 2.1
        // when working on a 2.0 port. Therefore a bcdUSB of 3.0 also
        // indicates that the device is actually working on 3.0, in
        // which case we always support function suspend
        //
        if (m_DeviceDescriptor.bcdUSB >= 0x300) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_NOT_SUPPORTED;
        }
    }
    else {
        //
        // We do not support chained MDLs or streams for a UMDF driver
        // GUID_USB_CAPABILITY_CHAINED_MDLS
        // GUID_USB_CAPABILITY_STATIC_STREAMS
        //
        status = STATUS_NOT_SUPPORTED;
    }

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
    Since the device is already configured, all this routine
    does is to make sure the alternate setting 0 is selected,
    in case the client driver selected some other alternate
    setting after the initial configuration

Arguments:


Return Value:
    NTSTATUS

  --*/
{
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

    //
    // Use AlternateSetting 0 by default
    //
    if (m_Interfaces[0]->GetSettingDescriptor(0) == NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE %p could not retrieve AlternateSetting 0 for "
            "bInterfaceNumber %d", GetHandle(),
            m_Interfaces[0]->m_InterfaceNumber);

        return STATUS_INVALID_PARAMETER;
    }

    status = m_Interfaces[0]->CheckAndSelectSettingByIndex(0);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBDEVICE %p set AlternateSetting 0 for interface 0"
            "failed, %!STATUS!", GetHandle(), status);

        return status;
    }

    if (PipeAttributes) {
        status = m_Interfaces[0]->UpdatePipeAttributes(PipeAttributes);
    }

    Params->Types.SingleInterface.ConfiguredUsbInterface =
        m_Interfaces[0]->GetHandle();

    Params->Types.SingleInterface.NumberConfiguredPipes =
        m_Interfaces[0]->GetNumConfiguredPipes();

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SelectConfigMulti(
    __in PWDF_OBJECT_ATTRIBUTES PipeAttributes,
    __in PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
/*++

Routine Description:
    Since the device is already configured, all this routine
    does is to make sure the alternate setting 0 is selected
    for all interfaces, in case the client driver selected some
    other alternate setting after the initial configuration


Arguments:
    PipeAttributes - Should be NULL

    Params -

Return Value:
    NTSTATUS

  --*/
{
    FxUsbInterface * pUsbInterface;
    NTSTATUS status;
    UCHAR i;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    Params->Types.MultiInterface.NumberOfConfiguredInterfaces = 0;

    if (Params->Type == WdfUsbTargetDeviceSelectConfigTypeMultiInterface) {
        for (i = 0; i < m_NumInterfaces; i++) {

            if (m_Interfaces[i]->GetSettingDescriptor(0) == NULL) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "WDFUSBDEVICE %p could not retrieve AlternateSetting 0 for "
                    "bInterfaceNumber %d", GetHandle(),
                    m_Interfaces[i]->m_InterfaceNumber);

                status = STATUS_INVALID_PARAMETER;
                goto Done;
            }

            status = m_Interfaces[i]->CheckAndSelectSettingByIndex(0);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "WDFUSBDEVICE %p set AlternateSetting 0 for bInterfaceNumber %d"
                    "failed, %!STATUS!",
                    GetHandle(), m_Interfaces[i]->m_InterfaceNumber, status);
                goto Done;
            }
            if (PipeAttributes) {
                status = m_Interfaces[i]->UpdatePipeAttributes(PipeAttributes);
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

            FxObjectHandleGetPtr(GetDriverGlobals(),
                                 settingPair->UsbInterface,
                                 FX_TYPE_USB_INTERFACE ,
                                 (PVOID*) &pUsbInterface);

            interfaceNumber = pUsbInterface->GetInterfaceNumber();
            altSettingIndex = settingPair->SettingIndex;

            //
            // do the following only if the bit is not already set
            //
            if (FxBitArraySet(&bitArray[0], interfaceNumber) == FALSE) {

                if (pUsbInterface->GetSettingDescriptor(altSettingIndex) == NULL) {
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

                //
                // Ensure alternate setting 0 is selected
                //
                status = pUsbInterface->CheckAndSelectSettingByIndex(
                                settingPair->SettingIndex);

                if (!NT_SUCCESS(status)) {
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                        "WDFUSBDEVICE %p set AlternateSetting %d for bInterfaceNumber %d"
                        "failed, %!STATUS!",
                        GetHandle(), altSettingIndex, m_Interfaces[i]->m_InterfaceNumber,
                        status);
                    goto Done;
                }

                if (PipeAttributes) {
                    status = pUsbInterface->UpdatePipeAttributes(PipeAttributes);
                }
            }

        }

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

    status = STATUS_SUCCESS;
    Params->Types.MultiInterface.NumberOfConfiguredInterfaces = m_NumInterfaces;

Done:
    return status;
}


_Must_inspect_result_
NTSTATUS
FxUsbDevice::Reset(
    VOID
    )
{
    UMURB urb;
    NTSTATUS status;

    RtlZeroMemory(&urb, sizeof(UMURB));

    urb.UmUrbSelectInterface.Hdr.Function = UMURB_FUNCTION_RESET_PORT;
    urb.UmUrbSelectInterface.Hdr.Length = sizeof(_UMURB_HEADER);

    status = SendSyncUmUrb(&urb, 2);

    return status;
}

