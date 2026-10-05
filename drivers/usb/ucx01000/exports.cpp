/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Class functions exported to UCX clients
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/*
 * The public names belong to the client side inline thunks, so the
 * implementations here carry a UcxApi prefix. Each one only finds the object
 * and forwards; the globals argument is ignored except by controller create.
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

PFN_UCXFUNC UcxExportTable[UcxFunctionTableNumEntries];

_Must_inspect_result_
static
BOOLEAN
NTAPI
UcxApiIoDeviceControl(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    return UcxDispatchUserIoctl(Device, Request, OutputBufferLength, InputBufferLength, IoControlCode);
}

_Must_inspect_result_
static
NTSTATUS
NTAPI
UcxApiControllerCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device,
    _In_ PUCX_CONTROLLER_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXCONTROLLER* Controller)
{
    return UcxController::Create(DriverGlobals, Device, Config, Attributes, Controller);
}

static
VOID
NTAPI
UcxApiControllerNeedsReset(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxController::FromHandle(Controller)->NeedsReset();
}

static
VOID
NTAPI
UcxApiControllerResetComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_RESET_COMPLETE_INFO Info)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxController::FromHandle(Controller)->ResetComplete(Info);
}

static
VOID
NTAPI
UcxApiControllerSetFailed(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxController::FromHandle(Controller)->SetFailed();
}

_Must_inspect_result_
static
NTSTATUS
NTAPI
UcxApiRootHubCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_ROOTHUB_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXROOTHUB* RootHub)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    return UcxRootHub::Create(Controller, Config, Attributes, RootHub);
}

static
VOID
NTAPI
UcxApiRootHubPortChanged(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXROOTHUB RootHub)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxRootHub::FromHandle(RootHub)->PortChanged();
}

_Must_inspect_result_
static
NTSTATUS
NTAPI
UcxApiUsbDeviceCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _Inout_ PUCXUSBDEVICE_INIT* Init,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXUSBDEVICE* UsbDevice)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    return UcxUsbDevice::Create(Controller, Init, Attributes, UsbDevice);
}

static
VOID
NTAPI
UcxApiUsbDeviceInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXUSBDEVICE_INIT Init,
    _In_ PUCX_USBDEVICE_EVENT_CALLBACKS Callbacks)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxUsbDevice::InitSetEventCallbacks(Init, Callbacks);
}

static
VOID
NTAPI
UcxApiUsbDeviceRemoteWakeNotification(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ ULONG Interface)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxUsbDevice::FromHandle(UsbDevice)->RemoteWakeNotification(Interface);
}

_Must_inspect_result_
static
NTSTATUS
NTAPI
UcxApiEndpointCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _Inout_ PUCXENDPOINT_INIT* Init,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXENDPOINT* Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    return UcxEndpoint::Create(UsbDevice, Init, Attributes, Endpoint);
}

_Must_inspect_result_
static
UCXSSTREAMS
NTAPI
UcxApiEndpointGetStaticStreamsReferenced(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ PVOID Tag)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    return UcxEndpoint::FromHandle(Endpoint)->GetStaticStreamsReferenced(Tag);
}

static
VOID
NTAPI
UcxApiEndpointNeedToCancelTransfers(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxEndpoint::FromHandle(Endpoint)->NeedToCancelTransfers();
}

static
VOID
NTAPI
UcxApiEndpointInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT Init,
    _In_ PUCX_ENDPOINT_EVENT_CALLBACKS Callbacks)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxEndpoint::InitSetEventCallbacks(Init, Callbacks);
}

static
VOID
NTAPI
UcxApiDefaultEndpointInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT Init,
    _In_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS Callbacks)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxEndpoint::InitSetDefaultEventCallbacks(Init, Callbacks);
}

static
VOID
NTAPI
UcxApiEndpointSetWdfIoQueue(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ WDFQUEUE Queue)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxEndpoint::FromHandle(Endpoint)->SetWdfIoQueue(Queue);
}

/* Valid from inside EvtEndpointPurge */
static
VOID
NTAPI
UcxApiEndpointPurgeComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxEndpoint::FromHandle(Endpoint)->Post(EpEvent::PurgeDone);
}

/* Valid from inside EvtEndpointAbort */
static
VOID
NTAPI
UcxApiEndpointAbortComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxEndpoint::FromHandle(Endpoint)->Post(EpEvent::AbortDone);
}

/* Consumed by the next failing transfer completion on the device */
static
VOID
NTAPI
UcxApiEndpointNoPingResponseError(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxEndpoint::FromHandle(Endpoint)->m_Device->m_PendingNoPingResponse = 1;
}

static
VOID
NTAPI
UcxApiStaticStreamsSetStreamInfo(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXSSTREAMS StaticStreams,
    _In_ PSTREAM_INFO StreamInfo)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxStaticStreams::FromHandle(StaticStreams)->SetStreamInfo(StreamInfo);
}

_Must_inspect_result_
static
NTSTATUS
NTAPI
UcxApiStaticStreamsCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _Inout_ PUCXSSTREAMS_INIT* Init,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXSSTREAMS* StaticStreams)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    return UcxStaticStreams::Create(Endpoint, Init, Attributes, StaticStreams);
}

/* Reserved for future preprocess hooks; UCX never touches the HCD's device init */
_Must_inspect_result_
static
NTSTATUS
NTAPI
UcxApiInitializeDeviceInit(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(DeviceInit);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UcxApiControllerSetIdStrings(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUNICODE_STRING Manufacturer,
    _In_ PUNICODE_STRING ModelName,
    _In_ PUNICODE_STRING ModelNumber)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    return UcxController::FromHandle(Controller)->SetIdStrings(Manufacturer, ModelName, ModelNumber);
}

static
VOID
NTAPI
UcxApiControllerNotifyTransportCharacteristicsChange(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS Characteristics)
{
    UNREFERENCED_PARAMETER(DriverGlobals);

    UcxController::FromHandle(Controller)->NotifyTransportCharacteristicsChange(Characteristics);
}

VOID
NTAPI
UcxBuildExportTable(VOID)
{
    PFN_UCXFUNC* Table = UcxExportTable;

    Table[UcxIoDeviceControlTableIndex] = (PFN_UCXFUNC)UcxApiIoDeviceControl;
    Table[UcxControllerCreateTableIndex] = (PFN_UCXFUNC)UcxApiControllerCreate;
    Table[UcxControllerNeedsResetTableIndex] = (PFN_UCXFUNC)UcxApiControllerNeedsReset;
    Table[UcxControllerResetCompleteTableIndex] = (PFN_UCXFUNC)UcxApiControllerResetComplete;
    Table[UcxControllerSetFailedTableIndex] = (PFN_UCXFUNC)UcxApiControllerSetFailed;
    Table[UcxRootHubCreateTableIndex] = (PFN_UCXFUNC)UcxApiRootHubCreate;
    Table[UcxRootHubPortChangedTableIndex] = (PFN_UCXFUNC)UcxApiRootHubPortChanged;
    Table[UcxUsbDeviceCreateTableIndex] = (PFN_UCXFUNC)UcxApiUsbDeviceCreate;
    Table[UcxUsbDeviceInitSetEventCallbacksTableIndex] = (PFN_UCXFUNC)UcxApiUsbDeviceInitSetEventCallbacks;
    Table[UcxUsbDeviceRemoteWakeNotificationTableIndex] = (PFN_UCXFUNC)UcxApiUsbDeviceRemoteWakeNotification;
    Table[UcxEndpointCreateTableIndex] = (PFN_UCXFUNC)UcxApiEndpointCreate;
    Table[UcxEndpointGetStaticStreamsReferencedTableIndex] = (PFN_UCXFUNC)UcxApiEndpointGetStaticStreamsReferenced;
    Table[UcxEndpointNeedToCancelTransfersTableIndex] = (PFN_UCXFUNC)UcxApiEndpointNeedToCancelTransfers;
    Table[UcxEndpointInitSetEventCallbacksTableIndex] = (PFN_UCXFUNC)UcxApiEndpointInitSetEventCallbacks;
    Table[UcxDefaultEndpointInitSetEventCallbacksTableIndex] =
        (PFN_UCXFUNC)UcxApiDefaultEndpointInitSetEventCallbacks;
    Table[UcxEndpointSetWdfIoQueueTableIndex] = (PFN_UCXFUNC)UcxApiEndpointSetWdfIoQueue;
    Table[UcxEndpointPurgeCompleteTableIndex] = (PFN_UCXFUNC)UcxApiEndpointPurgeComplete;
    Table[UcxEndpointAbortCompleteTableIndex] = (PFN_UCXFUNC)UcxApiEndpointAbortComplete;
    Table[UcxEndpointNoPingResponseErrorTableIndex] = (PFN_UCXFUNC)UcxApiEndpointNoPingResponseError;
    Table[UcxStaticStreamsSetStreamInfoTableIndex] = (PFN_UCXFUNC)UcxApiStaticStreamsSetStreamInfo;
    Table[UcxStaticStreamsCreateTableIndex] = (PFN_UCXFUNC)UcxApiStaticStreamsCreate;
    Table[UcxInitializeDeviceInitTableIndex] = (PFN_UCXFUNC)UcxApiInitializeDeviceInit;
    Table[UcxControllerSetIdStringsTableIndex] = (PFN_UCXFUNC)UcxApiControllerSetIdStrings;
    Table[UcxControllerNotifyTransportCharacteristicsChangeTableIndex] =
        (PFN_UCXFUNC)UcxApiControllerNotifyTransportCharacteristicsChange;
}
