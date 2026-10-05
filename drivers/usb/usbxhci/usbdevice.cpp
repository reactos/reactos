/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX USB device and endpoint creation callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbxhci.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
XhciEvtControllerUsbDeviceAdd(
    _In_ UCXCONTROLLER UcxController,
    _In_ PUCXUSBDEVICE_INFO UcxUsbDeviceInfo,
    _In_ PUCXUSBDEVICE_INIT UsbDeviceInit)
{
    UCX_USBDEVICE_EVENT_CALLBACKS Callbacks;
    UCXUSBDEVICE UsbDevice;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(UcxUsbDeviceInfo);

    UCX_USBDEVICE_EVENT_CALLBACKS_INIT(&Callbacks,
                                       XhciEvtUsbDeviceEndpointsConfigure,
                                       XhciEvtUsbDeviceEnable,
                                       XhciEvtUsbDeviceDisable,
                                       XhciEvtUsbDeviceReset,
                                       XhciEvtUsbDeviceAddress,
                                       XhciEvtUsbDeviceUpdate,
                                       XhciEvtUsbDeviceHubInfo,
                                       XhciEvtUsbDeviceDefaultEndpointAdd,
                                       XhciEvtUsbDeviceEndpointAdd);
    UcxUsbDeviceInitSetEventCallbacks(UsbDeviceInit, &Callbacks);

    Status = UcxUsbDeviceCreate(UcxController, &UsbDeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &UsbDevice);
    if (!NT_SUCCESS(Status))
        DPRINT1("UcxUsbDeviceCreate failed 0x%lx\n", Status);

    return Status;
}

/* Device management requests, each carrying the matching USBDEVICE_* payload */

VOID
NTAPI
XhciEvtUsbDeviceEndpointsConfigure(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxController);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtUsbDeviceEnable(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxController);

    /* Enable Slot command */
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtUsbDeviceDisable(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxController);

    /* Disable Slot command */
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtUsbDeviceReset(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxController);

    /* Reset Device command */
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtUsbDeviceAddress(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxController);

    /* Address Device command */
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtUsbDeviceUpdate(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxController);

    /* Evaluate Context command */
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

VOID
NTAPI
XhciEvtUsbDeviceHubInfo(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    UNREFERENCED_PARAMETER(UcxController);
    WdfRequestComplete(Request, STATUS_NOT_IMPLEMENTED);
}

/* Endpoint creation */

NTSTATUS
NTAPI
XhciEvtUsbDeviceDefaultEndpointAdd(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_ ULONG MaxPacketSize,
    _In_ PUCXENDPOINT_INIT UcxEndpointInit)
{
    UNREFERENCED_PARAMETER(UcxController);
    UNREFERENCED_PARAMETER(UcxUsbDevice);
    UNREFERENCED_PARAMETER(MaxPacketSize);
    UNREFERENCED_PARAMETER(UcxEndpointInit);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
XhciEvtUsbDeviceEndpointAdd(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_reads_bytes_(UsbEndpointDescriptorBufferLength) PUSB_ENDPOINT_DESCRIPTOR UsbEndpointDescriptor,
    _In_ ULONG UsbEndpointDescriptorBufferLength,
    _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR SuperSpeedEndpointCompanionDescriptor,
    _In_ PUCXENDPOINT_INIT UcxEndpointInit)
{
    UNREFERENCED_PARAMETER(UcxController);
    UNREFERENCED_PARAMETER(UcxUsbDevice);
    UNREFERENCED_PARAMETER(UsbEndpointDescriptor);
    UNREFERENCED_PARAMETER(UsbEndpointDescriptorBufferLength);
    UNREFERENCED_PARAMETER(SuperSpeedEndpointCompanionDescriptor);
    UNREFERENCED_PARAMETER(UcxEndpointInit);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
