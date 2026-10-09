/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCX USB device object interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "ucxglobals.h"
#include "ucxfuncenum.h"
#include "ucxobjects.h"

WDF_EXTERN_C_START

/* C reaches the header fields through an unnamed member, C++ through Header */
#ifdef __cplusplus
#define UCX_MGMT_HEADER_MEMBER USBDEVICE_MGMT_HEADER Header
#define UCX_ANONYMOUS_FLAGS Flags
#else
#define UCX_MGMT_HEADER_MEMBER USBDEVICE_MGMT_HEADER
#define UCX_ANONYMOUS_FLAGS
#endif

typedef struct _USBDEVICE_MGMT_HEADER
{
    ULONG Size;
    UCXUSBDEVICE Hub;
    UCXUSBDEVICE UsbDevice;
} USBDEVICE_MGMT_HEADER, *PUSBDEVICE_MGMT_HEADER;

typedef struct _ADDRESS0_OWNERSHIP_ACQUIRE
{
    UCX_MGMT_HEADER_MEMBER;
} ADDRESS0_OWNERSHIP_ACQUIRE, *PADDRESS0_OWNERSHIP_ACQUIRE;

typedef struct _USBDEVICE_ENABLE_FAILURE_FLAGS
{
    ULONG InsufficientHardwareResourcesForDefaultEndpoint:1;
    ULONG InsufficientHardwareResourcesForDevice:1;
    ULONG Reserved:30;
} USBDEVICE_ENABLE_FAILURE_FLAGS;

typedef struct _USBDEVICE_ENABLE
{
    UCX_MGMT_HEADER_MEMBER;
    UCXENDPOINT DefaultEndpoint;
    USBDEVICE_ENABLE_FAILURE_FLAGS FailureFlags;
} USBDEVICE_ENABLE, *PUSBDEVICE_ENABLE;

typedef struct _USBDEVICE_RESET
{
    UCX_MGMT_HEADER_MEMBER;
    UCXENDPOINT DefaultEndpoint;
    ULONG EndpointsToDisableCount;
    _Field_size_(EndpointsToDisableCount) UCXENDPOINT *EndpointsToDisable;
} USBDEVICE_RESET, *PUSBDEVICE_RESET;

typedef struct _USBDEVICE_PURGEIO
{
    UCX_MGMT_HEADER_MEMBER;
    BOOLEAN OnSuspend;
} USBDEVICE_PURGEIO, *PUSBDEVICE_PURGEIO;

typedef struct _USBDEVICE_ABORTIO
{
    UCX_MGMT_HEADER_MEMBER;
} USBDEVICE_ABORTIO, *PUSBDEVICE_ABORTIO;

typedef struct _USBDEVICE_STARTIO
{
    UCX_MGMT_HEADER_MEMBER;
} USBDEVICE_STARTIO, *PUSBDEVICE_STARTIO;

typedef struct _USBDEVICE_TREE_PURGEIO
{
    UCX_MGMT_HEADER_MEMBER;
} USBDEVICE_TREE_PURGEIO, *PUSBDEVICE_TREE_PURGEIO;

typedef struct _USBDEVICE_ADDRESS
{
    UCX_MGMT_HEADER_MEMBER;
    ULONG Reserved;
    ULONG Address;
} USBDEVICE_ADDRESS, *PUSBDEVICE_ADDRESS;

typedef struct _USBDEVICE_UPDATE_FLAGS
{
    ULONG UpdateDeviceDescriptor:1;
    ULONG UpdateBosDescriptor:1;
    ULONG UpdateMaxExitLatency:1;
    ULONG UpdateIsHub:1;
    ULONG UpdateAllowIoOnInvalidPipeHandles:1;
    ULONG Update20HardwareLpmParameters:1;
    ULONG UpdateRootPortResumeTime:1;
    ULONG UpdateTunnelState:1;
    ULONG Reserved:25;
} USBDEVICE_UPDATE_FLAGS;

typedef struct _USBDEVICE_UPDATE_FAILURE_FLAGS
{
    ULONG MaxExitLatencyTooLarge:1;
    ULONG Reserved:31;
} USBDEVICE_UPDATE_FAILURE_FLAGS;

/** As defined by the USB 2.0 LPM ECN. */
typedef struct _USBDEVICE_UPDATE_20_HARDWARE_LPM_PARAMETERS
{
    ULONG HardwareLpmEnable:1;
    ULONG RemoteWakeEnable:1;
    ULONG HostInitiatedResumeDurationMode:1;
    ULONG BestEffortServiceLatency:4;
    ULONG BestEffortServiceLatencyDeep:4;
    ULONG L1Timeout:8;
    ULONG Reserved:13;
} USBDEVICE_UPDATE_20_HARDWARE_LPM_PARAMETERS;

typedef struct _USBDEVICE_UPDATE
{
    UCX_MGMT_HEADER_MEMBER;
    USBDEVICE_UPDATE_FLAGS Flags;
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;
    PUSB_BOS_DESCRIPTOR BosDescriptor;
    ULONG MaxExitLatency;
    BOOLEAN IsHub;
    USBDEVICE_UPDATE_FAILURE_FLAGS FailureFlags;
    USBDEVICE_UPDATE_20_HARDWARE_LPM_PARAMETERS Usb20HardwareLpmParameters;
    USHORT RootPortResumeTime;
    BOOLEAN Reserved;
} USBDEVICE_UPDATE, *PUSBDEVICE_UPDATE;

/* The flags spill into a second ULONG; the hub and controller drivers depend on it */
C_ASSERT(sizeof(USBDEVICE_UPDATE_FLAGS) == 8);
#ifdef _WIN64
C_ASSERT(FIELD_OFFSET(USBDEVICE_UPDATE, DeviceDescriptor) == 0x20);
#else
C_ASSERT(FIELD_OFFSET(USBDEVICE_UPDATE, DeviceDescriptor) == 0x14);
#endif

typedef struct _USBDEVICE_HUB_INFO
{
    UCX_MGMT_HEADER_MEMBER;
    ULONG NumberOfPorts;
    ULONG NumberOfTTs;
    ULONG TTThinkTime;
} USBDEVICE_HUB_INFO, *PUSBDEVICE_HUB_INFO;

typedef enum _UCX_USBDEVICE_RECOVERY_ACTION
{
    UcxUsbDeviceRecoverActionNone = 0,
    UcxUsbDeviceRecoverActionFunctionLevelDeviceReset = 1,
    UcxUsbDeviceRecoverActionPlatformLevelDeviceReset = 2
} UCX_USBDEVICE_RECOVERY_ACTION;

typedef struct _USBDEVICE_DISABLE
{
    UCX_MGMT_HEADER_MEMBER;
    UCXENDPOINT DefaultEndpoint;
    UCX_USBDEVICE_RECOVERY_ACTION UsbDeviceRecoveryAction;
} USBDEVICE_DISABLE, *PUSBDEVICE_DISABLE;

/* Root port plus five hubs */
#define MAX_USB_DEVICE_DEPTH 6

typedef struct _USB_DEVICE_PORT_PATH
{
    ULONG Size;
    ULONG PortPathDepth;
    ULONG TTHubDepth;
    ULONG PortPath[MAX_USB_DEVICE_DEPTH];
} USB_DEVICE_PORT_PATH, *PUSB_DEVICE_PORT_PATH;

typedef struct _UCXUSBDEVICE_INFO
{
    ULONG Size;
    USB_DEVICE_SPEED DeviceSpeed;
    UCXUSBDEVICE TtHub;
    USB_DEVICE_PORT_PATH PortPath;
} UCXUSBDEVICE_INFO, *PUCXUSBDEVICE_INFO;

typedef enum _UCX_USBDEVICE_CHARACTERISTIC_TYPE
{
    UCX_USBDEVICE_CHARACTERISTIC_TYPE_PATH_DELAY = 1
} UCX_USBDEVICE_CHARACTERISTIC_TYPE;

typedef struct _UCX_USBDEVICE_CHARACTERISTIC_PATH_DELAY
{
    ULONG MaximumSendPathDelayInMilliSeconds;
    ULONG MaximumCompletionPathDelayInMilliSeconds;
} UCX_USBDEVICE_CHARACTERISTIC_PATH_DELAY, *PUCX_USBDEVICE_CHARACTERISTIC_PATH_DELAY;

typedef struct _UCX_USBDEVICE_CHARACTERISTIC
{
    ULONG Size;
    UCX_USBDEVICE_CHARACTERISTIC_TYPE CharacteristicType;
    union
    {
        UCX_USBDEVICE_CHARACTERISTIC_PATH_DELAY PathDelay;
    };
} UCX_USBDEVICE_CHARACTERISTIC, *PUCX_USBDEVICE_CHARACTERISTIC;

typedef
_Function_class_(EVT_UCX_USBDEVICE_ENDPOINTS_CONFIGURE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_ENDPOINTS_CONFIGURE(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_USBDEVICE_ENDPOINTS_CONFIGURE *PFN_UCX_USBDEVICE_ENDPOINTS_CONFIGURE;

typedef
_Function_class_(EVT_UCX_USBDEVICE_ENABLE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_ENABLE(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_USBDEVICE_ENABLE *PFN_UCX_USBDEVICE_ENABLE;

typedef
_Function_class_(EVT_UCX_USBDEVICE_DISABLE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_DISABLE(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_USBDEVICE_DISABLE *PFN_UCX_USBDEVICE_DISABLE;

typedef
_Function_class_(EVT_UCX_USBDEVICE_RESET)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_RESET(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_USBDEVICE_RESET *PFN_UCX_USBDEVICE_RESET;

typedef
_Function_class_(EVT_UCX_USBDEVICE_ADDRESS)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_ADDRESS(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_USBDEVICE_ADDRESS *PFN_UCX_USBDEVICE_ADDRESS;

typedef
_Function_class_(EVT_UCX_USBDEVICE_UPDATE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_UPDATE(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_USBDEVICE_UPDATE *PFN_UCX_USBDEVICE_UPDATE;

typedef
_Function_class_(EVT_UCX_USBDEVICE_HUB_INFO)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_HUB_INFO(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request);
typedef EVT_UCX_USBDEVICE_HUB_INFO *PFN_UCX_USBDEVICE_HUB_INFO;

typedef
_Function_class_(EVT_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
EVT_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_ ULONG MaxPacketSize,
    _In_ PUCXENDPOINT_INIT UcxEndpointInit);
typedef EVT_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD *PFN_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD;

typedef
_Function_class_(EVT_UCX_USBDEVICE_ENDPOINT_ADD)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
EVT_UCX_USBDEVICE_ENDPOINT_ADD(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_reads_bytes_(UsbEndpointDescriptorBufferLength) PUSB_ENDPOINT_DESCRIPTOR UsbEndpointDescriptor,
    _In_ ULONG UsbEndpointDescriptorBufferLength,
    _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR SuperSpeedEndpointCompanionDescriptor,
    _In_ PUCXENDPOINT_INIT UcxEndpointInit);
typedef EVT_UCX_USBDEVICE_ENDPOINT_ADD *PFN_UCX_USBDEVICE_ENDPOINT_ADD;

/* Optional hooks */

typedef
_Function_class_(EVT_UCX_USBDEVICE_SUSPEND)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_SUSPEND(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice);
typedef EVT_UCX_USBDEVICE_SUSPEND *PFN_UCX_USBDEVICE_SUSPEND;

typedef
_Function_class_(EVT_UCX_USBDEVICE_RESUME)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
EVT_UCX_USBDEVICE_RESUME(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice);
typedef EVT_UCX_USBDEVICE_RESUME *PFN_UCX_USBDEVICE_RESUME;

typedef
_Function_class_(EVT_UCX_USBDEVICE_GET_CHARACTERISTIC)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI
EVT_UCX_USBDEVICE_GET_CHARACTERISTIC(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _Inout_ PUCX_USBDEVICE_CHARACTERISTIC UcxUsbDeviceCharacteristic);
typedef EVT_UCX_USBDEVICE_GET_CHARACTERISTIC *PFN_UCX_USBDEVICE_GET_CHARACTERISTIC;

typedef struct _UCX_USBDEVICE_EVENT_CALLBACKS
{
    ULONG Size;
    PFN_UCX_USBDEVICE_ENDPOINTS_CONFIGURE EvtUsbDeviceEndpointsConfigure;
    PFN_UCX_USBDEVICE_ENABLE EvtUsbDeviceEnable;
    PFN_UCX_USBDEVICE_DISABLE EvtUsbDeviceDisable;
    PFN_UCX_USBDEVICE_RESET EvtUsbDeviceReset;
    PFN_UCX_USBDEVICE_ADDRESS EvtUsbDeviceAddress;
    PFN_UCX_USBDEVICE_UPDATE EvtUsbDeviceUpdate;
    PFN_UCX_USBDEVICE_HUB_INFO EvtUsbDeviceHubInfo;
    PFN_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD EvtUsbDeviceDefaultEndpointAdd;
    PFN_UCX_USBDEVICE_ENDPOINT_ADD EvtUsbDeviceEndpointAdd;
    PFN_UCX_USBDEVICE_SUSPEND EvtUsbDeviceSuspend;
    PFN_UCX_USBDEVICE_RESUME EvtUsbDeviceResume;
    PFN_UCX_USBDEVICE_GET_CHARACTERISTIC EvtUsbDeviceGetCharacteristic;
} UCX_USBDEVICE_EVENT_CALLBACKS, *PUCX_USBDEVICE_EVENT_CALLBACKS;

/** Fills in the required callbacks only; the optional hooks are set afterwards. */
FORCEINLINE
VOID
NTAPI
UCX_USBDEVICE_EVENT_CALLBACKS_INIT(
    _Out_ PUCX_USBDEVICE_EVENT_CALLBACKS Callbacks,
    _In_ PFN_UCX_USBDEVICE_ENDPOINTS_CONFIGURE EvtUsbDeviceEndpointsConfigure,
    _In_ PFN_UCX_USBDEVICE_ENABLE EvtUsbDeviceEnable,
    _In_ PFN_UCX_USBDEVICE_DISABLE EvtUsbDeviceDisable,
    _In_ PFN_UCX_USBDEVICE_RESET EvtUsbDeviceReset,
    _In_ PFN_UCX_USBDEVICE_ADDRESS EvtUsbDeviceAddress,
    _In_ PFN_UCX_USBDEVICE_UPDATE EvtUsbDeviceUpdate,
    _In_ PFN_UCX_USBDEVICE_HUB_INFO EvtUsbDeviceHubInfo,
    _In_ PFN_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD EvtUsbDeviceDefaultEndpointAdd,
    _In_ PFN_UCX_USBDEVICE_ENDPOINT_ADD EvtUsbDeviceEndpointAdd)
{
    RtlZeroMemory(Callbacks, sizeof(*Callbacks));
    Callbacks->Size = sizeof(*Callbacks);

    Callbacks->EvtUsbDeviceEnable = EvtUsbDeviceEnable;
    Callbacks->EvtUsbDeviceDisable = EvtUsbDeviceDisable;
    Callbacks->EvtUsbDeviceReset = EvtUsbDeviceReset;
    Callbacks->EvtUsbDeviceAddress = EvtUsbDeviceAddress;
    Callbacks->EvtUsbDeviceUpdate = EvtUsbDeviceUpdate;
    Callbacks->EvtUsbDeviceHubInfo = EvtUsbDeviceHubInfo;
    Callbacks->EvtUsbDeviceEndpointsConfigure = EvtUsbDeviceEndpointsConfigure;
    Callbacks->EvtUsbDeviceDefaultEndpointAdd = EvtUsbDeviceDefaultEndpointAdd;
    Callbacks->EvtUsbDeviceEndpointAdd = EvtUsbDeviceEndpointAdd;
}

/* PEVT_ spellings from older headers */
typedef PFN_UCX_USBDEVICE_ENDPOINTS_CONFIGURE PEVT_UCX_USBDEVICE_ENDPOINTS_CONFIGURE;
typedef PFN_UCX_USBDEVICE_ENABLE PEVT_UCX_USBDEVICE_ENABLE;
typedef PFN_UCX_USBDEVICE_DISABLE PEVT_UCX_USBDEVICE_DISABLE;
typedef PFN_UCX_USBDEVICE_RESET PEVT_UCX_USBDEVICE_RESET;
typedef PFN_UCX_USBDEVICE_ADDRESS PEVT_UCX_USBDEVICE_ADDRESS;
typedef PFN_UCX_USBDEVICE_UPDATE PEVT_UCX_USBDEVICE_UPDATE;
typedef PFN_UCX_USBDEVICE_HUB_INFO PEVT_UCX_USBDEVICE_HUB_INFO;
typedef PFN_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD PEVT_UCX_USBDEVICE_DEFAULT_ENDPOINT_ADD;
typedef PFN_UCX_USBDEVICE_ENDPOINT_ADD PEVT_UCX_USBDEVICE_ENDPOINT_ADD;
typedef PFN_UCX_USBDEVICE_GET_CHARACTERISTIC PEVT_UCX_USBDEVICE_GET_CHARACTERISTIC;

/* Class entry points */

typedef
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXUSBDEVICECREATE)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXCONTROLLER Controller,
    _Inout_ PUCXUSBDEVICE_INIT *UsbDeviceInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXUSBDEVICE *UsbDevice);

typedef
VOID
(NTAPI *PFN_UCXUSBDEVICEINITSETEVENTCALLBACKS)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _Inout_ PUCXUSBDEVICE_INIT UsbDeviceInit,
    _In_ PUCX_USBDEVICE_EVENT_CALLBACKS EventCallbacks);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXUSBDEVICEREMOTEWAKENOTIFICATION)(
    _In_ PUCX_DRIVER_GLOBALS DriverGlobals,
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ ULONG Interface);

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
FORCEINLINE
NTSTATUS
NTAPI
UcxUsbDeviceCreate(
    _In_ UCXCONTROLLER Controller,
    _Inout_ PUCXUSBDEVICE_INIT *UsbDeviceInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXUSBDEVICE *UsbDevice)
{
    PFN_UCXUSBDEVICECREATE Create;

    Create = UCX_BOUND_FUNCTION(PFN_UCXUSBDEVICECREATE, UcxUsbDeviceCreateTableIndex);
    return Create(UcxDriverGlobals, Controller, UsbDeviceInit, Attributes, UsbDevice);
}

FORCEINLINE
VOID
NTAPI
UcxUsbDeviceInitSetEventCallbacks(
    _Inout_ PUCXUSBDEVICE_INIT UsbDeviceInit,
    _In_ PUCX_USBDEVICE_EVENT_CALLBACKS EventCallbacks)
{
    PFN_UCXUSBDEVICEINITSETEVENTCALLBACKS SetCallbacks;

    SetCallbacks = UCX_BOUND_FUNCTION(PFN_UCXUSBDEVICEINITSETEVENTCALLBACKS,
                                      UcxUsbDeviceInitSetEventCallbacksTableIndex);
    SetCallbacks(UcxDriverGlobals, UsbDeviceInit, EventCallbacks);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
FORCEINLINE
VOID
NTAPI
UcxUsbDeviceRemoteWakeNotification(
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ ULONG Interface)
{
    PFN_UCXUSBDEVICEREMOTEWAKENOTIFICATION Notify;

    Notify = UCX_BOUND_FUNCTION(PFN_UCXUSBDEVICEREMOTEWAKENOTIFICATION,
                                UcxUsbDeviceRemoteWakeNotificationTableIndex);
    Notify(UcxDriverGlobals, UsbDevice, Interface);
}

WDF_EXTERN_C_END
