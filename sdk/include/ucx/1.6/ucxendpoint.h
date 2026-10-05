/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX endpoint object interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "ucxusbdevice.h"

WDF_EXTERN_C_START

typedef enum _ENDPOINT_RESET_FLAGS
{
    FlagEndpointResetPreserveTransferState = 0x1
} ENDPOINT_RESET_FLAGS;

typedef struct _ENDPOINTS_CONFIGURE_FAILURE_FLAGS
{
    ULONG InsufficientBandwidth:1;
    ULONG InsufficientHardwareResourcesForEndpoints:1;
    ULONG MaxExitLatencyTooLarge:1;
    ULONG Reserved:29;
} ENDPOINTS_CONFIGURE_FAILURE_FLAGS;

typedef struct _ENDPOINTS_CONFIGURE
{
    UCX_MGMT_HEADER_MEMBER;
    ULONG EndpointsToEnableCount;
    _Field_size_(EndpointsToEnableCount) UCXENDPOINT *EndpointsToEnable;
    ULONG EndpointsToDisableCount;
    _Field_size_(EndpointsToDisableCount) UCXENDPOINT *EndpointsToDisable;
    ULONG EndpointsEnabledAndUnchangedCount;
    _Field_size_(EndpointsEnabledAndUnchangedCount) UCXENDPOINT *EndpointsEnabledAndUnchanged;
    ENDPOINTS_CONFIGURE_FAILURE_FLAGS FailureFlags;
    ULONG ExitLatencyDelta;
    UCHAR ConfigurationValue;
    UCHAR InterfaceNumber;
    UCHAR AlternateSetting;
    ULONG Reserved1;
    PVOID Reserved2;
} ENDPOINTS_CONFIGURE, *PENDPOINTS_CONFIGURE;

typedef struct _ENDPOINT_RESET
{
    UCX_MGMT_HEADER_MEMBER;
    UCXENDPOINT Endpoint;
    ENDPOINT_RESET_FLAGS Flags;
} ENDPOINT_RESET, *PENDPOINT_RESET;

typedef struct _DEFAULT_ENDPOINT_UPDATE
{
    UCX_MGMT_HEADER_MEMBER;
    UCXENDPOINT DefaultEndpoint;
    ULONG MaxPacketSize;
} DEFAULT_ENDPOINT_UPDATE, *PDEFAULT_ENDPOINT_UPDATE;

typedef struct _UCX_ENDPOINT_ISOCH_TRANSFER_PATH_DELAYS
{
    ULONG MaximumSendPathDelayInMilliSeconds;
    ULONG MaximumCompletionPathDelayInMilliSeconds;
} UCX_ENDPOINT_ISOCH_TRANSFER_PATH_DELAYS, *PUCX_ENDPOINT_ISOCH_TRANSFER_PATH_DELAYS;

typedef enum _UCX_ENDPOINT_CHARACTERISTIC_TYPE
{
    UCX_ENDPOINT_CHARACTERISTIC_TYPE_PRIORITY = 1
} UCX_ENDPOINT_CHARACTERISTIC_TYPE;

typedef enum _UCX_ENDPOINT_CHARACTERISTIC_PRIORITY
{
    UCX_ENDPOINT_CHARACTERISTIC_PRIORITY_NONE = 0,
    UCX_ENDPOINT_CHARACTERISTIC_PRIORITY_BULK_VIDEO = 1,
    UCX_ENDPOINT_CHARACTERISTIC_PRIORITY_BULK_VOICE = 2,
    UCX_ENDPOINT_CHARACTERISTIC_PRIORITY_BULK_INTERACTIVE = 3
} UCX_CONTROLLER_ENDPOINT_CHARACTERISTIC_PRIORITY;

typedef struct _UCX_ENDPOINT_CHARACTERISTIC
{
    ULONG Size;
    UCX_ENDPOINT_CHARACTERISTIC_TYPE CharacteristicType;
    union
    {
        UCX_CONTROLLER_ENDPOINT_CHARACTERISTIC_PRIORITY Priority;
    };
} UCX_ENDPOINT_CHARACTERISTIC, *PUCX_ENDPOINT_CHARACTERISTIC;

typedef
_Function_class_(EVT_UCX_ENDPOINT_RESET)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_RESET(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_ENDPOINT_RESET *PFN_UCX_ENDPOINT_RESET;

typedef
_Function_class_(EVT_UCX_ENDPOINT_PURGE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_PURGE(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint);
typedef EVT_UCX_ENDPOINT_PURGE *PFN_UCX_ENDPOINT_PURGE;

typedef
_Function_class_(EVT_UCX_ENDPOINT_START)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_START(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint);
typedef EVT_UCX_ENDPOINT_START *PFN_UCX_ENDPOINT_START;

typedef
_Function_class_(EVT_UCX_ENDPOINT_ABORT)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_ABORT(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint);
typedef EVT_UCX_ENDPOINT_ABORT *PFN_UCX_ENDPOINT_ABORT;

typedef
_Function_class_(EVT_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS(
    _In_ UCXENDPOINT UcxEndpoint);
typedef EVT_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS *PFN_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS;

typedef
_Function_class_(EVT_UCX_DEFAULT_ENDPOINT_UPDATE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_DEFAULT_ENDPOINT_UPDATE(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_DEFAULT_ENDPOINT_UPDATE *PFN_UCX_DEFAULT_ENDPOINT_UPDATE;

/* SuperSpeed bulk streams */

typedef
_Function_class_(EVT_UCX_ENDPOINT_STATIC_STREAMS_ADD)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
EVT_UCX_ENDPOINT_STATIC_STREAMS_ADD(
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ ULONG NumberOfStreams,
    _In_ PUCXSSTREAMS_INIT UcxStaticStreamsInit);
typedef EVT_UCX_ENDPOINT_STATIC_STREAMS_ADD *PFN_UCX_ENDPOINT_STATIC_STREAMS_ADD;

typedef
_Function_class_(EVT_UCX_ENDPOINT_STATIC_STREAMS_ENABLE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_STATIC_STREAMS_ENABLE(
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ UCXSSTREAMS UcxStaticStreams,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_ENDPOINT_STATIC_STREAMS_ENABLE *PFN_UCX_ENDPOINT_STATIC_STREAMS_ENABLE;

typedef
_Function_class_(EVT_UCX_ENDPOINT_STATIC_STREAMS_DISABLE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_STATIC_STREAMS_DISABLE(
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ UCXSSTREAMS UcxStaticStreams,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_ENDPOINT_STATIC_STREAMS_DISABLE *PFN_UCX_ENDPOINT_STATIC_STREAMS_DISABLE;

/* Optional hooks */

typedef
_Function_class_(EVT_UCX_ENDPOINT_GET_ISOCH_TRANSFER_PATH_DELAYS)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS
NTAPI
EVT_UCX_ENDPOINT_GET_ISOCH_TRANSFER_PATH_DELAYS(
    _In_ UCXENDPOINT UcxEndpoint,
    _Inout_ PUCX_ENDPOINT_ISOCH_TRANSFER_PATH_DELAYS UcxEndpointTransferPathDelays);
typedef EVT_UCX_ENDPOINT_GET_ISOCH_TRANSFER_PATH_DELAYS *PFN_UCX_ENDPOINT_GET_ISOCH_TRANSFER_PATH_DELAYS;

typedef
_Function_class_(EVT_UCX_ENDPOINT_SET_CHARACTERISTIC)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_ENDPOINT_SET_CHARACTERISTIC(
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ PUCX_ENDPOINT_CHARACTERISTIC UcxEndpointCharacteristic);
typedef EVT_UCX_ENDPOINT_SET_CHARACTERISTIC *PFN_UCX_ENDPOINT_SET_CHARACTERISTIC;

typedef struct _UCX_ENDPOINT_EVENT_CALLBACKS
{
    ULONG Size;
    PFN_UCX_ENDPOINT_PURGE EvtEndpointPurge;
    PFN_UCX_ENDPOINT_START EvtEndpointStart;
    PFN_UCX_ENDPOINT_ABORT EvtEndpointAbort;
    PFN_UCX_ENDPOINT_RESET EvtEndpointReset;
    PFN_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS EvtEndpointOkToCancelTransfers;
    PFN_UCX_ENDPOINT_STATIC_STREAMS_ADD EvtEndpointStaticStreamsAdd;
    PFN_UCX_ENDPOINT_STATIC_STREAMS_ENABLE EvtEndpointStaticStreamsEnable;
    PFN_UCX_ENDPOINT_STATIC_STREAMS_DISABLE EvtEndpointStaticStreamsDisable;
    HANDLE Reserved1;
    PFN_UCX_ENDPOINT_GET_ISOCH_TRANSFER_PATH_DELAYS EvtEndpointGetIsochTransferPathDelays;
    PFN_UCX_ENDPOINT_SET_CHARACTERISTIC EvtEndpointSetCharacteristic;
} UCX_ENDPOINT_EVENT_CALLBACKS, *PUCX_ENDPOINT_EVENT_CALLBACKS;

FORCEINLINE
VOID
NTAPI
UCX_ENDPOINT_EVENT_CALLBACKS_INIT(
    _Out_ PUCX_ENDPOINT_EVENT_CALLBACKS Callbacks,
    _In_ PFN_UCX_ENDPOINT_PURGE EvtEndpointPurge,
    _In_ PFN_UCX_ENDPOINT_START EvtEndpointStart,
    _In_ PFN_UCX_ENDPOINT_ABORT EvtEndpointAbort,
    _In_ PFN_UCX_ENDPOINT_RESET EvtEndpointReset,
    _In_ PFN_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS EvtEndpointOkToCancelTransfers,
    _In_ PFN_UCX_ENDPOINT_STATIC_STREAMS_ADD EvtEndpointStaticStreamsAdd,
    _In_ PFN_UCX_ENDPOINT_STATIC_STREAMS_ENABLE EvtEndpointStaticStreamsEnable,
    _In_ PFN_UCX_ENDPOINT_STATIC_STREAMS_DISABLE EvtEndpointStaticStreamsDisable)
{
    RtlZeroMemory(Callbacks, sizeof(*Callbacks));
    Callbacks->Size = sizeof(*Callbacks);

    Callbacks->EvtEndpointStart = EvtEndpointStart;
    Callbacks->EvtEndpointPurge = EvtEndpointPurge;
    Callbacks->EvtEndpointAbort = EvtEndpointAbort;
    Callbacks->EvtEndpointReset = EvtEndpointReset;
    Callbacks->EvtEndpointOkToCancelTransfers = EvtEndpointOkToCancelTransfers;

    Callbacks->EvtEndpointStaticStreamsAdd = EvtEndpointStaticStreamsAdd;
    Callbacks->EvtEndpointStaticStreamsEnable = EvtEndpointStaticStreamsEnable;
    Callbacks->EvtEndpointStaticStreamsDisable = EvtEndpointStaticStreamsDisable;
}

typedef struct _UCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS
{
    ULONG Size;
    PFN_UCX_ENDPOINT_PURGE EvtEndpointPurge;
    PFN_UCX_ENDPOINT_START EvtEndpointStart;
    PFN_UCX_ENDPOINT_ABORT EvtEndpointAbort;
    PFN_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS EvtEndpointOkToCancelTransfers;
    PFN_UCX_DEFAULT_ENDPOINT_UPDATE EvtDefaultEndpointUpdate;
    HANDLE Reserved1;
} UCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS, *PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS;

FORCEINLINE
VOID
NTAPI
UCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS_INIT(
    _Out_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS Callbacks,
    _In_ PFN_UCX_ENDPOINT_PURGE EvtEndpointPurge,
    _In_ PFN_UCX_ENDPOINT_START EvtEndpointStart,
    _In_ PFN_UCX_ENDPOINT_ABORT EvtEndpointAbort,
    _In_ PFN_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS EvtEndpointOkToCancelTransfers,
    _In_ PFN_UCX_DEFAULT_ENDPOINT_UPDATE EvtDefaultEndpointUpdate)
{
    RtlZeroMemory(Callbacks, sizeof(*Callbacks));
    Callbacks->Size = sizeof(*Callbacks);

    Callbacks->EvtEndpointStart = EvtEndpointStart;
    Callbacks->EvtEndpointPurge = EvtEndpointPurge;
    Callbacks->EvtEndpointAbort = EvtEndpointAbort;
    Callbacks->EvtEndpointOkToCancelTransfers = EvtEndpointOkToCancelTransfers;
    Callbacks->EvtDefaultEndpointUpdate = EvtDefaultEndpointUpdate;
}

/* PEVT_ spellings from older headers */
typedef PFN_UCX_ENDPOINT_RESET PEVT_UCX_ENDPOINT_RESET;
typedef PFN_UCX_ENDPOINT_PURGE PEVT_UCX_ENDPOINT_PURGE;
typedef PFN_UCX_ENDPOINT_START PEVT_UCX_ENDPOINT_START;
typedef PFN_UCX_ENDPOINT_ABORT PEVT_UCX_ENDPOINT_ABORT;
typedef PFN_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS PEVT_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS;
typedef PFN_UCX_DEFAULT_ENDPOINT_UPDATE PEVT_UCX_DEFAULT_ENDPOINT_UPDATE;
typedef PFN_UCX_ENDPOINT_STATIC_STREAMS_ADD PEVT_UCX_ENDPOINT_STATIC_STREAMS_ADD;
typedef PFN_UCX_ENDPOINT_STATIC_STREAMS_ENABLE PEVT_UCX_ENDPOINT_STATIC_STREAMS_ENABLE;
typedef PFN_UCX_ENDPOINT_STATIC_STREAMS_DISABLE PEVT_UCX_ENDPOINT_STATIC_STREAMS_DISABLE;

/* Class entry points */

typedef
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXENDPOINTCREATE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _Inout_ PUCXENDPOINT_INIT *EndpointInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXENDPOINT *Endpoint);

typedef
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
UCXSSTREAMS
(NTAPI *PFN_UCXENDPOINTGETSTATICSTREAMSREFERENCED)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ PVOID Tag);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXENDPOINTNEEDTOCANCELTRANSFERS)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

typedef
VOID
(NTAPI *PFN_UCXENDPOINTINITSETEVENTCALLBACKS)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_ENDPOINT_EVENT_CALLBACKS EventCallbacks);

typedef
VOID
(NTAPI *PFN_UCXDEFAULTENDPOINTINITSETEVENTCALLBACKS)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS EventCallbacks);

typedef
VOID
(NTAPI *PFN_UCXENDPOINTSETWDFIOQUEUE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint,
    _In_ WDFQUEUE WdfQueue);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXENDPOINTPURGECOMPLETE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXENDPOINTABORTCOMPLETE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXENDPOINTNOPINGRESPONSEERROR)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXENDPOINT Endpoint);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
FORCEINLINE
NTSTATUS
NTAPI
UcxEndpointCreate(
    _In_ UCXUSBDEVICE UsbDevice,
    _Inout_ PUCXENDPOINT_INIT *EndpointInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXENDPOINT *Endpoint)
{
    PFN_UCXENDPOINTCREATE Create;

    Create = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTCREATE, UcxEndpointCreateTableIndex);
    return Create(UcxDriverGlobals, UsbDevice, EndpointInit, Attributes, Endpoint);
}

/** Returns the streams object with a reference held under Tag, or NULL. */
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
UCXSSTREAMS
NTAPI
UcxEndpointGetStaticStreamsReferenced(
    _In_ UCXENDPOINT Endpoint,
    _In_ PVOID Tag)
{
    PFN_UCXENDPOINTGETSTATICSTREAMSREFERENCED GetStreams;

    GetStreams = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTGETSTATICSTREAMSREFERENCED,
                                    UcxEndpointGetStaticStreamsReferencedTableIndex);
    return GetStreams(UcxDriverGlobals, Endpoint, Tag);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxEndpointNeedToCancelTransfers(
    _In_ UCXENDPOINT Endpoint)
{
    PFN_UCXENDPOINTNEEDTOCANCELTRANSFERS NeedToCancel;

    NeedToCancel = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTNEEDTOCANCELTRANSFERS,
                                      UcxEndpointNeedToCancelTransfersTableIndex);
    NeedToCancel(UcxDriverGlobals, Endpoint);
}

FORCEINLINE
VOID
NTAPI
UcxEndpointInitSetEventCallbacks(
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_ENDPOINT_EVENT_CALLBACKS EventCallbacks)
{
    PFN_UCXENDPOINTINITSETEVENTCALLBACKS SetCallbacks;

    SetCallbacks = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTINITSETEVENTCALLBACKS,
                                      UcxEndpointInitSetEventCallbacksTableIndex);
    SetCallbacks(UcxDriverGlobals, EndpointInit, EventCallbacks);
}

FORCEINLINE
VOID
NTAPI
UcxDefaultEndpointInitSetEventCallbacks(
    _Inout_ PUCXENDPOINT_INIT EndpointInit,
    _In_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS EventCallbacks)
{
    PFN_UCXDEFAULTENDPOINTINITSETEVENTCALLBACKS SetCallbacks;

    SetCallbacks = UCX_BOUND_FUNCTION(PFN_UCXDEFAULTENDPOINTINITSETEVENTCALLBACKS,
                                      UcxDefaultEndpointInitSetEventCallbacksTableIndex);
    SetCallbacks(UcxDriverGlobals, EndpointInit, EventCallbacks);
}

FORCEINLINE
VOID
NTAPI
UcxEndpointSetWdfIoQueue(
    _In_ UCXENDPOINT Endpoint,
    _In_ WDFQUEUE WdfQueue)
{
    PFN_UCXENDPOINTSETWDFIOQUEUE SetQueue;

    SetQueue = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTSETWDFIOQUEUE, UcxEndpointSetWdfIoQueueTableIndex);
    SetQueue(UcxDriverGlobals, Endpoint, WdfQueue);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxEndpointPurgeComplete(
    _In_ UCXENDPOINT Endpoint)
{
    PFN_UCXENDPOINTPURGECOMPLETE PurgeComplete;

    PurgeComplete = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTPURGECOMPLETE,
                                       UcxEndpointPurgeCompleteTableIndex);
    PurgeComplete(UcxDriverGlobals, Endpoint);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxEndpointAbortComplete(
    _In_ UCXENDPOINT Endpoint)
{
    PFN_UCXENDPOINTABORTCOMPLETE AbortComplete;

    AbortComplete = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTABORTCOMPLETE,
                                       UcxEndpointAbortCompleteTableIndex);
    AbortComplete(UcxDriverGlobals, Endpoint);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxEndpointNoPingResponseError(
    _In_ UCXENDPOINT Endpoint)
{
    PFN_UCXENDPOINTNOPINGRESPONSEERROR NoPingResponse;

    NoPingResponse = UCX_BOUND_FUNCTION(PFN_UCXENDPOINTNOPINGRESPONSEERROR,
                                        UcxEndpointNoPingResponseErrorTableIndex);
    NoPingResponse(UcxDriverGlobals, Endpoint);
}

WDF_EXTERN_C_END
