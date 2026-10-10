/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller FDO and UCX controller callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbxhci.h"

#define NDEBUG
#include <debug.h>

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
XhciCreateUcxController(
    _In_ PXHCI_CONTROLLER Controller)
{
    UCX_CONTROLLER_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    UCX_CONTROLLER_CONFIG_INIT(&Config, "ReactOS xHCI Host Controller");
    Config.EvtControllerQueryUsbCapability = XhciEvtControllerQueryUsbCapability;
    Config.EvtControllerGetCurrentFrameNumber = XhciEvtControllerGetCurrentFrameNumber;
    Config.EvtControllerUsbDeviceAdd = XhciEvtControllerUsbDeviceAdd;
    Config.EvtControllerReset = XhciEvtControllerReset;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_UCX_CONTROLLER_CONTEXT);

    Status = UcxControllerCreate(Controller->Device, &Config, &Attributes, &Controller->UcxController);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UcxControllerCreate failed 0x%lx\n", Status);
        return Status;
    }

    XhciGetUcxControllerContext(Controller->UcxController)->Controller = Controller;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
XhciEvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPower;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    PXHCI_CONTROLLER Controller;
    WDFDEVICE Device;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPower);
    PnpPower.EvtDevicePrepareHardware = XhciEvtDevicePrepareHardware;
    PnpPower.EvtDeviceReleaseHardware = XhciEvtDeviceReleaseHardware;
    PnpPower.EvtDeviceD0Entry = XhciEvtDeviceD0Entry;
    PnpPower.EvtDeviceD0Exit = XhciEvtDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPower);

    Status = UcxInitializeDeviceInit(DeviceInit);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UcxInitializeDeviceInit failed 0x%lx\n", Status);
        return Status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_CONTROLLER);

    Status = WdfDeviceCreate(&DeviceInit, &Attributes, &Device);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfDeviceCreate failed 0x%lx\n", Status);
        return Status;
    }

    Controller = XhciGetController(Device);
    Controller->Device = Device;

    /* UCX gets first look at every IOCTL sent to the controller */
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchParallel);
    QueueConfig.EvtIoDeviceControl = XhciEvtIoDeviceControl;

    Status = WdfIoQueueCreate(Device, &QueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfIoQueueCreate failed 0x%lx\n", Status);
        return Status;
    }

    Status = XhciCreateUcxController(Controller);
    if (!NT_SUCCESS(Status))
        return Status;

    return XhciRootHubCreate(Controller);
}

NTSTATUS
NTAPI
XhciEvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    /* Map the MMIO BAR, read capability registers, connect the interrupt */
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
XhciEvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
XhciEvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PreviousState);

    /* Reset and start the controller */
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
XhciEvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(TargetState);

    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
NTAPI
XhciEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    WDFDEVICE Device = WdfIoQueueGetDevice(Queue);

    if (UcxIoDeviceControl(Device, Request, OutputBufferLength, InputBufferLength, IoControlCode))
        return;

    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}

NTSTATUS
NTAPI
XhciEvtControllerQueryUsbCapability(
    _In_ UCXCONTROLLER UcxController,
    _In_ PGUID CapabilityType,
    _In_ ULONG OutputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _Out_ PULONG ResultLength)
{
    UNREFERENCED_PARAMETER(UcxController);
    UNREFERENCED_PARAMETER(CapabilityType);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(OutputBuffer);

    *ResultLength = 0;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
XhciEvtControllerGetCurrentFrameNumber(
    _In_ UCXCONTROLLER UcxController,
    _Out_ PULONG FrameNumber)
{
    UNREFERENCED_PARAMETER(UcxController);

    /* MFINDEX >> 3 once the runtime registers are mapped */
    *FrameNumber = 0;
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
XhciEvtControllerReset(
    _In_ UCXCONTROLLER UcxController)
{
    UCX_CONTROLLER_RESET_COMPLETE_INFO Info;

    UNIMPLEMENTED;

    UCX_CONTROLLER_RESET_COMPLETE_INFO_INIT(&Info, UcxControllerStateLost, TRUE);
    UcxControllerResetComplete(UcxController, &Info);
}
