/*
 * PROJECT:     ReactOS USB 3 stack
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Private communication between the hub driver and the USB controller extension
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <usbiodef.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(GUID_UCXHUB_STACK_INTERFACE,
            0x4fb2b0e1, 0x5fdd, 0x4dab, 0xa6, 0x00, 0x5e, 0x15, 0x16, 0xbb, 0xd8, 0xa7);

DEFINE_GUID(GUID_UCXHUB_PARENT_INTERFACE,
            0xa9264c93, 0xeccd, 0x4b40, 0x92, 0x9b, 0xb3, 0x54, 0xf1, 0xe9, 0xa2, 0xdc);

#define UCXHUB_FN_ADDRESS0_OWNERSHIP_ACQUIRE  0x401
#define UCXHUB_FN_ROOTHUB_GET_INFO            0x402
#define UCXHUB_FN_ROOTHUB_GET_20PORT_INFO     0x403
#define UCXHUB_FN_ROOTHUB_GET_30PORT_INFO     0x404
#define UCXHUB_FN_DEVICE_ENABLE               0x405
#define UCXHUB_FN_DEVICE_RESET                0x406
#define UCXHUB_FN_DEVICE_ADDRESS              0x407
#define UCXHUB_FN_DEVICE_UPDATE               0x408
#define UCXHUB_FN_DEVICE_HUB_INFO             0x409
#define UCXHUB_FN_DEVICE_DISABLE              0x40A
#define UCXHUB_FN_DEVICE_PURGE_IO             0x40B
#define UCXHUB_FN_DEVICE_START_IO             0x40C
#define UCXHUB_FN_ENDPOINTS_CONFIGURE         0x40D
#define UCXHUB_FN_DEFAULT_ENDPOINT_UPDATE     0x40E
#define UCXHUB_FN_ENDPOINT_RESET              0x40F
#define UCXHUB_FN_GET_HUB_INFO                0x410
#define UCXHUB_FN_QUERY_USB_CAPABILITY        0x412
#define UCXHUB_FN_SET_FUNCTION_HANDLE_DATA    0x413
#define UCXHUB_FN_DEVICE_TREE_PURGE_IO        0x414
#define UCXHUB_FN_DEVICE_ABORT_IO             0x415
#define UCXHUB_FN_GET_DUMP_DATA               0x501
#define UCXHUB_FN_FREE_DUMP_DATA              0x502
#define UCXHUB_FN_NOTIFY_FORWARD_PROGRESS     0x503

#define UCXHUB_IOCTL(Function) \
    CTL_CODE(FILE_DEVICE_USBEX, (Function), METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_UCXHUB_ADDRESS0_OWNERSHIP_ACQUIRE  UCXHUB_IOCTL(UCXHUB_FN_ADDRESS0_OWNERSHIP_ACQUIRE)
#define IOCTL_UCXHUB_ROOTHUB_GET_INFO            UCXHUB_IOCTL(UCXHUB_FN_ROOTHUB_GET_INFO)
#define IOCTL_UCXHUB_ROOTHUB_GET_20PORT_INFO     UCXHUB_IOCTL(UCXHUB_FN_ROOTHUB_GET_20PORT_INFO)
#define IOCTL_UCXHUB_ROOTHUB_GET_30PORT_INFO     UCXHUB_IOCTL(UCXHUB_FN_ROOTHUB_GET_30PORT_INFO)
#define IOCTL_UCXHUB_DEVICE_ENABLE               UCXHUB_IOCTL(UCXHUB_FN_DEVICE_ENABLE)
#define IOCTL_UCXHUB_DEVICE_RESET                UCXHUB_IOCTL(UCXHUB_FN_DEVICE_RESET)
#define IOCTL_UCXHUB_DEVICE_ADDRESS              UCXHUB_IOCTL(UCXHUB_FN_DEVICE_ADDRESS)
#define IOCTL_UCXHUB_DEVICE_UPDATE               UCXHUB_IOCTL(UCXHUB_FN_DEVICE_UPDATE)
#define IOCTL_UCXHUB_DEVICE_HUB_INFO             UCXHUB_IOCTL(UCXHUB_FN_DEVICE_HUB_INFO)
#define IOCTL_UCXHUB_DEVICE_DISABLE              UCXHUB_IOCTL(UCXHUB_FN_DEVICE_DISABLE)
#define IOCTL_UCXHUB_DEVICE_PURGE_IO             UCXHUB_IOCTL(UCXHUB_FN_DEVICE_PURGE_IO)
#define IOCTL_UCXHUB_DEVICE_START_IO             UCXHUB_IOCTL(UCXHUB_FN_DEVICE_START_IO)
#define IOCTL_UCXHUB_ENDPOINTS_CONFIGURE         UCXHUB_IOCTL(UCXHUB_FN_ENDPOINTS_CONFIGURE)
#define IOCTL_UCXHUB_DEFAULT_ENDPOINT_UPDATE     UCXHUB_IOCTL(UCXHUB_FN_DEFAULT_ENDPOINT_UPDATE)
#define IOCTL_UCXHUB_ENDPOINT_RESET              UCXHUB_IOCTL(UCXHUB_FN_ENDPOINT_RESET)
#define IOCTL_UCXHUB_GET_HUB_INFO                UCXHUB_IOCTL(UCXHUB_FN_GET_HUB_INFO)
#define IOCTL_UCXHUB_QUERY_USB_CAPABILITY        UCXHUB_IOCTL(UCXHUB_FN_QUERY_USB_CAPABILITY)
#define IOCTL_UCXHUB_SET_FUNCTION_HANDLE_DATA    UCXHUB_IOCTL(UCXHUB_FN_SET_FUNCTION_HANDLE_DATA)
#define IOCTL_UCXHUB_DEVICE_TREE_PURGE_IO        UCXHUB_IOCTL(UCXHUB_FN_DEVICE_TREE_PURGE_IO)
#define IOCTL_UCXHUB_DEVICE_ABORT_IO             UCXHUB_IOCTL(UCXHUB_FN_DEVICE_ABORT_IO)
#define IOCTL_UCXHUB_NOTIFY_FORWARD_PROGRESS     UCXHUB_IOCTL(UCXHUB_FN_NOTIFY_FORWARD_PROGRESS)

/* The two dump IOCTLs are buffered */
#define IOCTL_UCXHUB_GET_DUMP_DATA \
    CTL_CODE(FILE_DEVICE_USBEX, UCXHUB_FN_GET_DUMP_DATA, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_UCXHUB_FREE_DUMP_DATA \
    CTL_CODE(FILE_DEVICE_USBEX, UCXHUB_FN_FREE_DUMP_DATA, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* Asynchronous port reset, FILE_DEVICE_UNKNOWN function 1004 */
#define IOCTL_UCXHUB_RESET_PORT_ASYNC \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 1004, METHOD_NEITHER, FILE_ANY_ACCESS)

/* Written into Parameters.Others.Argument1 of a completed async port reset */
typedef union _UCXHUB_RESET_FLAGS
{
    ULONG AsUlong;
    struct
    {
        ULONG HostContextLost:1;
        ULONG PortPowerInterrupted:1;
        ULONG Reserved:30;
    };
} UCXHUB_RESET_FLAGS, *PUCXHUB_RESET_FLAGS;

/*
 * URB_HEADER.UsbdFlags is split between the two drivers: the low 24 bits
 * belong to the controller extension, which clears them on entry, the high
 * 8 bits to the hub.
 */
#define UCXHUB_URB_FLAGS_UCX_MASK               0x00FFFFFF
#define UCXHUB_URB_FLAGS_HUB_MASK               0xFF000000
#define UCXHUB_URB_FLAG_PEND_INTERRUPT_TX       0x80000000
#define UCXHUB_URB_FLAG_ALLOW_LARGE_TRANSFER    0x40000000

/* Handles the hub gives the controller extension to call it back with */
DECLARE_HANDLE(UCXHUB_HUB_CONTEXT);
DECLARE_HANDLE(UCXHUB_DEVICE_CONTEXT);

/** Describes a newly attached device to the stack interface's device create. */
typedef struct _UCXHUB_DEVICE_CREATE_INFO
{
    ULONG Size;
    USB_DEVICE_SPEED DeviceSpeed;
    ULONG PortNumber;
    UCXHUB_DEVICE_CONTEXT HubDeviceContext;
} UCXHUB_DEVICE_CREATE_INFO, *PUCXHUB_DEVICE_CREATE_INFO;

/** ROOTHUB_GET_INFO payload; the controller driver sees only the ROOTHUB_INFO part. */
typedef struct _UCXHUB_ROOTHUB_INFO
{
    ROOTHUB_INFO Info;
    USBD_PIPE_HANDLE InterruptPipe;
} UCXHUB_ROOTHUB_INFO, *PUCXHUB_ROOTHUB_INFO;

/** QueryControllerBus output: the parent bus identity of the controller. */
typedef struct _UCXHUB_CONTROLLER_INFO
{
    UCX_CONTROLLER_PARENT_BUS_TYPE Type;
    union
    {
        UCX_CONTROLLER_PCI_INFORMATION Pci;
        UCX_CONTROLLER_ACPI_INFORMATION Acpi;
    };
} UCXHUB_CONTROLLER_INFO, *PUCXHUB_CONTROLLER_INFO;

/*
 * The same answer as hubs that ask for the grown stack interface expect it:
 * the union moved to an 8 byte boundary.
 */
typedef struct _UCXHUB_CONTROLLER_INFO_V2
{
    UCX_CONTROLLER_PARENT_BUS_TYPE Type;
    ULONG Reserved;
    union
    {
        UCX_CONTROLLER_PCI_INFORMATION Pci;
        UCX_CONTROLLER_ACPI_INFORMATION Acpi;
        ULONG64 Alignment;
    };
} UCXHUB_CONTROLLER_INFO_V2, *PUCXHUB_CONTROLLER_INFO_V2;

typedef struct _UCXHUB_STOP_IDLE_CONTEXT
{
    BOOLEAN PowerReferenceAcquired;
} UCXHUB_STOP_IDLE_CONTEXT, *PUCXHUB_STOP_IDLE_CONTEXT;

/* QUERY_USB_CAPABILITY payload */
#define UCXHUB_QUERY_CAPABILITY_VERSION 1

typedef struct _UCXHUB_QUERY_CAPABILITY
{
    USHORT Version;
    USHORT Size;
    PVOID UsbdHandle;
    GUID CapabilityType;
    ULONG OutputBufferLength;
    ULONG ResultLength;
} UCXHUB_QUERY_CAPABILITY, *PUCXHUB_QUERY_CAPABILITY;

/* SET_FUNCTION_HANDLE_DATA input, in the system buffer */
typedef struct _UCXHUB_FUNCTION_DATA
{
    USHORT Version;
    USHORT Size;
    USBD_FUNCTION_HANDLE FunctionHandle;
    PDEVICE_OBJECT PhysicalDeviceObject;
} UCXHUB_FUNCTION_DATA, *PUCXHUB_FUNCTION_DATA;

/* GET_DUMP_DATA input; the rest of the dump contract is between the hub and the HCD */
typedef struct _UCXHUB_DUMP_DEVICE_INFO
{
    ULONG EndpointCount;
    PVOID EndpointInfo;
    UCHAR ConfigurationId;
    UCHAR InterfaceId;
    UCHAR AlternateSettingId;
    UCXUSBDEVICE Device;
} UCXHUB_DUMP_DEVICE_INFO, *PUCXHUB_DUMP_DEVICE_INFO;

/* NOTIFY_FORWARD_PROGRESS payload */
typedef struct _UCXHUB_FORWARD_PROGRESS_PIPE
{
    USBD_PIPE_HANDLE PipeHandle;
    ULONG ReservedTransferLimit;
} UCXHUB_FORWARD_PROGRESS_PIPE, *PUCXHUB_FORWARD_PROGRESS_PIPE;

typedef struct _UCXHUB_FORWARD_PROGRESS_INFO
{
    ULONG Version;
    ULONG Size;
    UCXUSBDEVICE Device;
    ULONG ControlPipeMaxTransferSize;
    ULONG NumberOfPipes;
    UCXHUB_FORWARD_PROGRESS_PIPE Pipes[ANYSIZE_ARRAY];
} UCXHUB_FORWARD_PROGRESS_INFO, *PUCXHUB_FORWARD_PROGRESS_INFO;

/*
 * Forward progress work items. Opaque to the hub; it only allocates, queues,
 * flushes and frees them through the stack interface.
 */
typedef struct _UCXHUB_WORKITEM *PUCXHUB_WORKITEM;

#define UCXHUB_WORKITEM_NO_FLAGS           0
#define UCXHUB_WORKITEM_FLAG_NEEDS_FLUSH   1

typedef enum _UCXHUB_WORKITEM_ENQUEUE_OPTIONS
{
    UcxHubWorkItemDefault = 0xCAD00000,
    UcxHubWorkItemForwardProgressNotRequired
} UCXHUB_WORKITEM_ENQUEUE_OPTIONS;

typedef
_Function_class_(UCXHUB_WORKITEM_ROUTINE)
VOID
NTAPI
UCXHUB_WORKITEM_ROUTINE(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context,
    _In_ PUCXHUB_WORKITEM WorkItem);
typedef UCXHUB_WORKITEM_ROUTINE *PUCXHUB_WORKITEM_ROUTINE;

/* Stack interface, hub side callbacks */

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_CLEAR_TT_BUFFER)(
    _In_ UCXHUB_HUB_CONTEXT HubContext,
    _In_ UCXHUB_DEVICE_CONTEXT DeviceContext,
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG EndpointNumber,
    _In_ ULONG TtPortNumber);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_NO_PING_RESPONSE)(
    _In_ UCXHUB_HUB_CONTEXT HubContext,
    _In_ UCXHUB_DEVICE_CONTEXT DeviceContext);

/* Stack interface, controller extension side */

typedef
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXHUB_DEVICE_CREATE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ PUCXHUB_DEVICE_CREATE_INFO CreateInfo,
    _Out_ UCXUSBDEVICE *Device);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_DEVICE_DELETE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_DEVICE_DISCONNECT)(
    _In_ UCXUSBDEVICE Hub,
    _In_ ULONG PortNumber);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_DEVICE_SET_PDO)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ PDEVICE_OBJECT Pdo);

typedef
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXHUB_DEFAULT_ENDPOINT_CREATE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ ULONG MaxPacketSize,
    _Out_ UCXENDPOINT *Endpoint);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_ADDRESS0_RELEASE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device);

typedef
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXHUB_ENDPOINT_CREATE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_reads_bytes_(DescriptorBufferLength) PUSB_ENDPOINT_DESCRIPTOR Descriptor,
    _In_ ULONG DescriptorBufferLength,
    _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion,
    _Out_ UCXENDPOINT *Endpoint);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_ENDPOINT_DELETE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ UCXENDPOINT Endpoint);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_ENDPOINT_EXPOSED)(
    _In_ UCXENDPOINT Endpoint);

typedef
USBD_PIPE_HANDLE
(NTAPI *PFN_UCXHUB_ENDPOINT_PIPE_HANDLE)(
    _In_ UCXENDPOINT Endpoint);

typedef
ULONG
(NTAPI *PFN_UCXHUB_ENDPOINT_MAX_TRANSFER)(
    _In_ UCXENDPOINT Endpoint);

typedef
PUCXHUB_WORKITEM
(NTAPI *PFN_UCXHUB_WORKITEM_ALLOCATE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG Flags);

typedef
VOID
(NTAPI *PFN_UCXHUB_WORKITEM_DELETE)(
    _In_ PUCXHUB_WORKITEM WorkItem);

typedef
VOID
(NTAPI *PFN_UCXHUB_WORKITEM_ENQUEUE)(
    _In_ PUCXHUB_WORKITEM WorkItem,
    _In_ PUCXHUB_WORKITEM_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_ UCXHUB_WORKITEM_ENQUEUE_OPTIONS Options);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_WORKITEM_FLUSH)(
    _In_ PUCXHUB_WORKITEM WorkItem);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_CLEAR_TT_BUFFER_COMPLETE)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXENDPOINT Endpoint);

typedef
BOOLEAN
(NTAPI *PFN_UCXHUB_IS_DEVICE_DISCONNECTED)(
    _In_ UCXUSBDEVICE Device);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI *PFN_UCXHUB_STREAMS_SUPPORTED)(
    _In_ UCXUSBDEVICE Hub);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCXHUB_CONTROLLER_STOP_IDLE)(
    _In_ UCXUSBDEVICE Hub,
    _Inout_ PUCXHUB_STOP_IDLE_CONTEXT StopIdleContext);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_CONTROLLER_RESUME_IDLE)(
    _In_ UCXUSBDEVICE Hub,
    _Inout_ PUCXHUB_STOP_IDLE_CONTEXT StopIdleContext);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_CONTROLLER_GET_INFO)(
    _In_ UCXUSBDEVICE Hub,
    _Out_ PUCXHUB_CONTROLLER_INFO ControllerInfo);

/** System time the controller extension last handled an event for the device, or 0. */
typedef
ULONG64
(NTAPI *PFN_UCXHUB_DEVICE_GET_TIMESTAMP)(
    _In_ UCXUSBDEVICE Device);

/** Priority 1 to 3, forwarded to the HCD's EvtEndpointSetCharacteristic. */
typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_ENDPOINT_SET_PRIORITY)(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG Priority);

/* Stack interface versions. Version 1 later grew a slot; see the size table below. */
#define UCXHUB_STACK_INTERFACE_VERSION_1   1000
#define UCXHUB_STACK_INTERFACE_VERSION_2   2000
#define UCXHUB_STACK_INTERFACE_VERSION_3   3000

/*
 * Everything the hub needs from the controller extension. The hub fills Hub,
 * HubContext, ClearTtBuffer and NoPingResponse; the rest is filled by the
 * controller extension, up to what the size and version requested allow.
 */
typedef struct _UCXHUB_STACK_INTERFACE
{
    INTERFACE Header;

    UCXUSBDEVICE Hub;
    UCXHUB_HUB_CONTEXT HubContext;
    PFN_UCXHUB_CLEAR_TT_BUFFER ClearTtBuffer;

    PFN_UCXHUB_DEVICE_CREATE DeviceCreate;
    PFN_UCXHUB_DEVICE_DELETE DeviceDelete;
    PFN_UCXHUB_DEVICE_DISCONNECT DeviceDisconnect;
    PFN_UCXHUB_DEVICE_SET_PDO DeviceSetPdo;
    PFN_UCXHUB_DEFAULT_ENDPOINT_CREATE CreateControlEndpoint;
    PFN_UCXHUB_ADDRESS0_RELEASE Address0Release;
    PFN_UCXHUB_ENDPOINT_CREATE EndpointCreate;
    PFN_UCXHUB_ENDPOINT_DELETE EndpointDelete;
    PFN_UCXHUB_ENDPOINT_EXPOSED MarkEndpointClientOwned;
    PFN_UCXHUB_ENDPOINT_PIPE_HANDLE EndpointGetPipeHandle;
    PFN_UCXHUB_ENDPOINT_MAX_TRANSFER EndpointGetMaxTransferSize;
    PVOID Unused;
    PFN_UCXHUB_WORKITEM_ALLOCATE WorkItemAllocate;
    PFN_UCXHUB_WORKITEM_DELETE WorkItemDelete;
    PFN_UCXHUB_WORKITEM_ENQUEUE WorkItemEnqueue;
    PFN_UCXHUB_WORKITEM_FLUSH WorkItemFlush;
    PFN_UCXHUB_CLEAR_TT_BUFFER_COMPLETE ClearTtBufferComplete;
    PFN_UCXHUB_NO_PING_RESPONSE NoPingResponse;
    PFN_UCXHUB_IS_DEVICE_DISCONNECTED IsDeviceDisconnected;
    PFN_UCXHUB_STREAMS_SUPPORTED StreamsSupported;
    PFN_UCXHUB_CONTROLLER_STOP_IDLE BlockControllerIdle;
    PFN_UCXHUB_CONTROLLER_RESUME_IDLE AllowControllerIdle;
    PFN_UCXHUB_CONTROLLER_GET_INFO QueryControllerBus;

    /* Present in version 1 once the size covers it */
    PFN_UCXHUB_DEVICE_GET_TIMESTAMP DeviceGetTimestamp;

    /* Version 2 */
    PFN_UCXHUB_ENDPOINT_SET_PRIORITY EndpointSetPriority;

    /* Version 3: endpoint create with an extra descriptor, not provided */
    PVOID EndpointCreateEx;
} UCXHUB_STACK_INTERFACE, *PUCXHUB_STACK_INTERFACE;

/* Size of the original version 1 layout, before DeviceGetTimestamp was added; still accepted */
#define UCXHUB_STACK_INTERFACE_SIZE_ORIGINAL \
    FIELD_OFFSET(UCXHUB_STACK_INTERFACE, DeviceGetTimestamp)

/* Parent interface, answered by whoever is a hub's parent */

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI *PFN_UCXHUB_WAS_HUB_RESET)(
    _In_ PVOID Context);

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI *PFN_UCXHUB_WAS_PROGRAMMING_LOST)(
    _In_ PVOID Context);

typedef
_IRQL_requires_(PASSIVE_LEVEL)
VOID
(NTAPI *PFN_UCXHUB_GET_SYMBOLIC_NAME)(
    _In_ PVOID Context,
    _In_ PUNICODE_STRING SymbolicLinkName);

#define UCXHUB_PARENT_INTERFACE_VERSION 1013

/*
 * Current layout. The original layout lacks the ULONG after the resume
 * callbacks; UCXHUB_PARENT_INTERFACE_ORIGINAL below describes it.
 */
typedef struct _UCXHUB_PARENT_INTERFACE
{
    INTERFACE Header;
    UCHAR HubDepth;
    UCXUSBDEVICE Hub;
    USB_DEVICE_SPEED HubSpeed;
    BOOLEAN ParentCanWake;
    BOOLEAN IsEnhancedSuperSpeed;
    PFN_UCXHUB_WAS_HUB_RESET ParentResetDuringResume;
    PFN_UCXHUB_WAS_PROGRAMMING_LOST ParentLostStateDuringResume;
    ULONG TunnelState;
    PVOID ConnectorId;
    USHORT FirstCompanionPort;
    USHORT LastCompanionPort;
    PFN_UCXHUB_GET_SYMBOLIC_NAME QueryHubLinkName;
    USB_TOPOLOGY_ADDRESS HubTopologyAddress;
    PVOID ChildHubObject;
} UCXHUB_PARENT_INTERFACE, *PUCXHUB_PARENT_INTERFACE;

typedef struct _UCXHUB_PARENT_INTERFACE_ORIGINAL
{
    INTERFACE Header;
    UCHAR HubDepth;
    UCXUSBDEVICE Hub;
    USB_DEVICE_SPEED HubSpeed;
    BOOLEAN ParentCanWake;
    BOOLEAN IsEnhancedSuperSpeed;
    PFN_UCXHUB_WAS_HUB_RESET ParentResetDuringResume;
    PFN_UCXHUB_WAS_PROGRAMMING_LOST ParentLostStateDuringResume;
    PVOID ConnectorId;
    USHORT FirstCompanionPort;
    USHORT LastCompanionPort;
    PFN_UCXHUB_GET_SYMBOLIC_NAME QueryHubLinkName;
    USB_TOPOLOGY_ADDRESS HubTopologyAddress;
    PVOID ChildHubObject;
} UCXHUB_PARENT_INTERFACE_ORIGINAL, *PUCXHUB_PARENT_INTERFACE_ORIGINAL;

/* Tunnel state of a root port as the hub and the HCD exchange it */
#define UCXHUB_TUNNEL_STATE_NOT_ANSWERED    0
#define UCXHUB_TUNNEL_STATE_NO_INFORMATION  1
#define UCXHUB_TUNNEL_STATE_TUNNELED        2
#define UCXHUB_TUNNEL_STATE_NATIVE          3
#define UCXHUB_TUNNEL_STATE_UNKNOWN         4

/*
 * The HCD answers UpdateTunnelState of USBDEVICE_UPDATE in the BOOLEAN the
 * public header calls Reserved and in the padding byte right after it.
 */
typedef struct _UCXHUB_UPDATE_TUNNEL_ANSWER
{
    BOOLEAN NativeLink;
    UCHAR TunnelState;
} UCXHUB_UPDATE_TUNNEL_ANSWER, *PUCXHUB_UPDATE_TUNNEL_ANSWER;

FORCEINLINE
PUCXHUB_UPDATE_TUNNEL_ANSWER
NTAPI
UcxHubUpdateTunnelAnswer(
    _In_ PUSBDEVICE_UPDATE Update)
{
    return (PUCXHUB_UPDATE_TUNNEL_ANSWER)&Update->Reserved;
}

#ifdef _WIN64
C_ASSERT(FIELD_OFFSET(UCXHUB_STACK_INTERFACE, QueryControllerBus) == 0xE8);
C_ASSERT(FIELD_OFFSET(UCXHUB_STACK_INTERFACE, DeviceGetTimestamp) == 0xF0);
C_ASSERT(sizeof(UCXHUB_STACK_INTERFACE) == 0x108);
C_ASSERT(FIELD_OFFSET(UCXHUB_PARENT_INTERFACE, TunnelState) == 0x48);
C_ASSERT(FIELD_OFFSET(UCXHUB_PARENT_INTERFACE, HubTopologyAddress) == 0x68);
C_ASSERT(sizeof(UCXHUB_PARENT_INTERFACE) == 0x90);
C_ASSERT(sizeof(UCXHUB_PARENT_INTERFACE_ORIGINAL) == 0x88);
C_ASSERT(sizeof(UCXHUB_QUERY_CAPABILITY) == 40);
C_ASSERT(sizeof(UCXHUB_ROOTHUB_INFO) == 24);
C_ASSERT(sizeof(UCXHUB_DEVICE_CREATE_INFO) == 24);
C_ASSERT(FIELD_OFFSET(USBDEVICE_UPDATE, Reserved) == 0x42);
C_ASSERT(sizeof(USBDEVICE_UPDATE) == 0x48);
#else
C_ASSERT(UCXHUB_STACK_INTERFACE_SIZE_ORIGINAL == 0x78);
C_ASSERT(sizeof(UCXHUB_PARENT_INTERFACE_ORIGINAL) == 0x58);
C_ASSERT(sizeof(UCXHUB_QUERY_CAPABILITY) == 32);
C_ASSERT(sizeof(UCXHUB_ROOTHUB_INFO) == 20);
C_ASSERT(sizeof(UCXHUB_DEVICE_CREATE_INFO) == 16);
C_ASSERT(FIELD_OFFSET(USBDEVICE_UPDATE, Reserved) == 0x2E);
C_ASSERT(sizeof(USBDEVICE_UPDATE) == 0x30);
#endif
C_ASSERT(FIELD_OFFSET(UCXHUB_UPDATE_TUNNEL_ANSWER, TunnelState) == 1);
C_ASSERT(sizeof(UCXHUB_CONTROLLER_INFO) == 28);
C_ASSERT(sizeof(UCXHUB_CONTROLLER_INFO_V2) == 32);
C_ASSERT(FIELD_OFFSET(UCXHUB_CONTROLLER_INFO_V2, Pci) == 8);

#ifdef __cplusplus
}
#endif
