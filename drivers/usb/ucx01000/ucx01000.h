/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Common declarations for ucx01000
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <ntddk.h>
#include <windef.h>
#include <fxldr.h>
#include <wdf.h>
#include <ucxclass.h>

#define UCX_POOL_TAG 'xcUR'

#define UCX_CLASS_MAJOR_VERSION 1
#define UCX_CLASS_MINOR_VERSION 7

extern "C" WDF_CLASS_LIBRARY_INFO UcxClassLibraryInfo;


_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxOnClassLoad(VOID);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxOnClassUnload(VOID);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxOnClientBind(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _Out_ PWDF_COMPONENT_GLOBALS *ComponentGlobals);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxOnClientUnbind(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_COMPONENT_GLOBALS *ComponentGlobals);

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
    _In_ ULONG IoControlCode);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
UcxApiControllerCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ WDFDEVICE Device,
    _In_ PUCX_CONTROLLER_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXCONTROLLER *Controller);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerNeedsReset(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerResetComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_RESET_COMPLETE_INFO UcxControllerResetCompleteInfo);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerSetFailed(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
UcxApiRootHubCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_ROOTHUB_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXROOTHUB *RootHub);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiRootHubPortChanged(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXROOTHUB UcxRootHub);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxApiUsbDeviceCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _Inout_ PUCXUSBDEVICE_INIT *UsbDeviceInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXUSBDEVICE *UsbDevice);

VOID
NTAPI
UcxApiUsbDeviceInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXUSBDEVICE_INIT UsbDeviceInit,
    _In_ PUCX_USBDEVICE_EVENT_CALLBACKS EventCallbacks);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiUsbDeviceRemoteWakeNotification(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ ULONG Interface);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxApiEndpointCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _Inout_ PUCXENDPOINT_INIT *EndpointInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXENDPOINT *Endpoint);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
UCXSSTREAMS
NTAPI
UcxApiEndpointGetStaticStreamsReferenced(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ PVOID Tag);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointNeedToCancelTransfers(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

VOID
NTAPI
UcxApiEndpointInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_ENDPOINT_EVENT_CALLBACKS EventCallbacks);

VOID
NTAPI
UcxApiDefaultEndpointInitSetEventCallbacks(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS EventCallbacks);

VOID
NTAPI
UcxApiEndpointSetWdfIoQueue(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ WDFQUEUE WdfQueue);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointPurgeComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointAbortComplete(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiEndpointNoPingResponseError(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

VOID
NTAPI
UcxApiStaticStreamsSetStreamInfo(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXSSTREAMS StaticStreams,
    _In_ PSTREAM_INFO StreamInfo);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxApiStaticStreamsCreate(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _Inout_ PUCXSSTREAMS_INIT *StaticStreamsInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXSSTREAMS *StaticStreams);

_IRQL_requires_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
UcxApiInitializeDeviceInit(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PWDFDEVICE_INIT DeviceInit);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
UcxApiControllerSetIdStrings(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUNICODE_STRING ManufacturerNameString,
    _In_ PUNICODE_STRING ModelNameString,
    _In_ PUNICODE_STRING ModelNumberString);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
UcxApiControllerNotifyTransportCharacteristicsChange(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _In_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS UcxControllerTransportCharacteristics);
