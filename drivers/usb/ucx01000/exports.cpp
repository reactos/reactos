/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Class functions exported to UCX clients
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/* Controller */

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxApiInitializeDeviceInit(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(DeviceInit);

    /* Lets clients get as far as UcxControllerCreate until this is real */
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
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
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    /* Not claimed; the client completes the request itself */
    return FALSE;
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
UcxApiControllerCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device,
    _In_ PUCX_CONTROLLER_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXCONTROLLER *Controller)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Config);
    UNREFERENCED_PARAMETER(Attributes);

    *Controller = NULL;
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerNeedsReset(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Controller);
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerResetComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_RESET_COMPLETE_INFO UcxControllerResetCompleteInfo)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Controller);
    UNREFERENCED_PARAMETER(UcxControllerResetCompleteInfo);
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerSetFailed(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Controller);
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
UcxApiControllerSetIdStrings(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUNICODE_STRING ManufacturerNameString,
    _In_ PUNICODE_STRING ModelNameString,
    _In_ PUNICODE_STRING ModelNumberString)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Controller);
    UNREFERENCED_PARAMETER(ManufacturerNameString);
    UNREFERENCED_PARAMETER(ModelNameString);
    UNREFERENCED_PARAMETER(ModelNumberString);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerNotifyTransportCharacteristicsChange(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS UcxControllerTransportCharacteristics)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Controller);
    UNREFERENCED_PARAMETER(UcxControllerTransportCharacteristics);
    UNIMPLEMENTED;
}

/* Root hub */

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
UcxApiRootHubCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_ROOTHUB_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXROOTHUB *RootHub)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Controller);
    UNREFERENCED_PARAMETER(Config);
    UNREFERENCED_PARAMETER(Attributes);

    *RootHub = NULL;
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiRootHubPortChanged(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXROOTHUB UcxRootHub)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(UcxRootHub);
    UNIMPLEMENTED;
}

/* USB devices */

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxApiUsbDeviceCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _Inout_ PUCXUSBDEVICE_INIT *UsbDeviceInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXUSBDEVICE *UsbDevice)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Controller);
    UNREFERENCED_PARAMETER(UsbDeviceInit);
    UNREFERENCED_PARAMETER(Attributes);

    *UsbDevice = NULL;
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
UcxApiUsbDeviceInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXUSBDEVICE_INIT UsbDeviceInit,
    _In_ PUCX_USBDEVICE_EVENT_CALLBACKS EventCallbacks)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(UsbDeviceInit);
    UNREFERENCED_PARAMETER(EventCallbacks);
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiUsbDeviceRemoteWakeNotification(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ ULONG Interface)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(UsbDevice);
    UNREFERENCED_PARAMETER(Interface);
    UNIMPLEMENTED;
}

/* Endpoints */

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxApiEndpointCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _Inout_ PUCXENDPOINT_INIT *EndpointInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXENDPOINT *Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(UsbDevice);
    UNREFERENCED_PARAMETER(EndpointInit);
    UNREFERENCED_PARAMETER(Attributes);

    *Endpoint = NULL;
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
UCXSSTREAMS
NTAPI
UcxApiEndpointGetStaticStreamsReferenced(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ PVOID Tag)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Endpoint);
    UNREFERENCED_PARAMETER(Tag);

    UNIMPLEMENTED;
    return NULL;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointNeedToCancelTransfers(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Endpoint);
    UNIMPLEMENTED;
}

VOID
NTAPI
UcxApiEndpointInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_ENDPOINT_EVENT_CALLBACKS EventCallbacks)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(EndpointInit);
    UNREFERENCED_PARAMETER(EventCallbacks);
    UNIMPLEMENTED;
}

VOID
NTAPI
UcxApiDefaultEndpointInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS EventCallbacks)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(EndpointInit);
    UNREFERENCED_PARAMETER(EventCallbacks);
    UNIMPLEMENTED;
}

VOID
NTAPI
UcxApiEndpointSetWdfIoQueue(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ WDFQUEUE WdfQueue)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Endpoint);
    UNREFERENCED_PARAMETER(WdfQueue);
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointPurgeComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Endpoint);
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointAbortComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Endpoint);
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointNoPingResponseError(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Endpoint);
    UNIMPLEMENTED;
}

/* Static streams */

VOID
NTAPI
UcxApiStaticStreamsSetStreamInfo(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXSSTREAMS StaticStreams,
    _In_ PSTREAM_INFO StreamInfo)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(StaticStreams);
    UNREFERENCED_PARAMETER(StreamInfo);
    UNIMPLEMENTED;
}

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxApiStaticStreamsCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _Inout_ PUCXSSTREAMS_INIT *StaticStreamsInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXSSTREAMS *StaticStreams)
{
    UNREFERENCED_PARAMETER(DriverGlobals);
    UNREFERENCED_PARAMETER(Endpoint);
    UNREFERENCED_PARAMETER(StaticStreamsInit);
    UNREFERENCED_PARAMETER(Attributes);

    *StaticStreams = NULL;
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
