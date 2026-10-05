/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Hub level requests to the USB controller extension
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Room for every SuperSpeedPlus speed a root port can report */
#define HUB_ROOT_PORT_MAX_SPEEDS        48

/* What the hub promises to need on its control pipe while paging */
#define HUB_FORWARD_PROGRESS_CONTROL    4096

/* Hub class code and the protocol of a multi TT hub interface */
#define HUB_INTERFACE_CLASS             9
#define HUB_PROTOCOL_MULTI_TT           2

static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubRootHubInfoComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubIoctlComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubRoot20PortsComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubRoot30PortsComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubControllerHubInfoComplete;

/*
 * Every hub level request is a fresh WDFREQUEST deleted by its completion.
 * Any failure before the send reports IoctlFailed right away.
 */
static
VOID
NTAPI
HubSendUcxIoctl(
    _In_ HubFdo* Hub,
    _In_ WDFIOTARGET Target,
    _In_ ULONG IoControlCode,
    _In_ PVOID Payload,
    _In_ WDFREQUEST Request,
    _In_ PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion)
{
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack.Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    Stack.Parameters.Others.Argument1 = Payload;
    WdfRequestWdmFormatUsingStackLocation(Request, &Stack);


    WdfRequestSetCompletionRoutine(Request, Completion, Hub);

    if (WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS))
        return;

    Status = WdfRequestGetStatus(Request);
    DPRINT1("Hub %p UCX request 0x%lx not sent 0x%lx\n", Hub, IoControlCode, Status);

    WdfObjectDelete(Request);
    Hub->Post(HubEvent::IoctlFailed);
}

static
NTSTATUS
NTAPI
HubCreateUcxRequest(
    _In_ HubFdo* Hub,
    _In_ WDFIOTARGET Target,
    _Out_ WDFREQUEST* Request)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    *Request = NULL;
    Status = WdfRequestCreate(&Attributes, Target, Request);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p UCX request create failed 0x%lx\n", Hub, Status);

    return Status;
}

static
VOID
NTAPI
HubIoctlComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;

    UNREFERENCED_PARAMETER(Target);

    Hub->Post(NT_SUCCESS(Params->IoStatus.Status) ? HubEvent::IoctlDone : HubEvent::IoctlFailed);
    WdfObjectDelete(Request);
}

static
VOID
NTAPI
HubRoot20PortsComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    if (!NT_SUCCESS(Params->IoStatus.Status))
        DPRINT1("Hub %p root hub 2.0 port info failed 0x%lx\n", Context, Params->IoStatus.Status);

    HubIoctlComplete(Request, Target, Params, Context);
}

static
VOID
NTAPI
HubRoot30PortsComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    if (!NT_SUCCESS(Params->IoStatus.Status))
        DPRINT1("Hub %p root hub 3.0 port info failed 0x%lx\n", Context, Params->IoStatus.Status);

    HubIoctlComplete(Request, Target, Params, Context);
}

static
VOID
NTAPI
HubControllerHubInfoComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    if (!NT_SUCCESS(Params->IoStatus.Status))
        DPRINT1("Hub %p hub info update in UCX failed 0x%lx\n", Context, Params->IoStatus.Status);

    HubIoctlComplete(Request, Target, Params, Context);
}

VOID
HubMachine::QueryControllerInfo()
{
    /* A hub with the full size stack interface gets the 8 byte aligned layout */
    m_Hub->m_Stack.QueryControllerBus(m_Hub->UsbDevice(), (PUCXHUB_CONTROLLER_INFO)&m_Hub->m_ControllerInfo);
}

/* The root hub reports its ports, latencies and status change pipe */
static
VOID
NTAPI
HubRootHubInfoComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;
    BOOLEAN Succeeded = NT_SUCCESS(Params->IoStatus.Status);

    UNREFERENCED_PARAMETER(Target);

    if (Succeeded)
    {
        Hub->m_Interrupt.Pipe = Hub->m_RootHubInfo.InterruptPipe;
        Hub->m_ParentInfo.U1ExitLatency = Hub->m_RootHubInfo.Info.MaxU1ExitLatency;
        Hub->m_ParentInfo.U2ExitLatency = Hub->m_RootHubInfo.Info.MaxU2ExitLatency;
        DPRINT("Hub %p root hub has %u 2.0 and %u 3.0 ports\n",
               Hub,
               Hub->m_RootHubInfo.Info.NumberOf20Ports,
               Hub->m_RootHubInfo.Info.NumberOf30Ports);
    }
    else
    {
        DPRINT1("Hub %p root hub info failed 0x%lx\n", Hub, Params->IoStatus.Status);
    }

    Hub->Post(Succeeded ? HubEvent::IoctlDone : HubEvent::IoctlFailed);
    WdfObjectDelete(Request);
}

VOID
HubMachine::GetRootHubInfo()
{
    WDFIOTARGET Target = WdfDeviceGetIoTarget(m_Hub->m_Device);
    WDFREQUEST Request;

    if (!NT_SUCCESS(HubCreateUcxRequest(m_Hub, Target, &Request)))
    {
        m_Hub->Post(HubEvent::IoctlFailed);
        return;
    }

    m_Hub->m_RootHubInfo.Info.Size = sizeof(m_Hub->m_RootHubInfo);

    HubSendUcxIoctl(m_Hub,
                    Target,
                    IOCTL_UCXHUB_ROOTHUB_GET_INFO,
                    &m_Hub->m_RootHubInfo,
                    Request,
                    HubRootHubInfoComplete);
}

/* The port info block outlives the request; each call allocates one under the FDO, which frees them */
VOID
HubMachine::GetRootHub20Ports()
{
    ULONG Count = m_Hub->m_RootHubInfo.Info.NumberOf20Ports;
    WDFIOTARGET Target = WdfDeviceGetIoTarget(m_Hub->m_Device);
    WDF_OBJECT_ATTRIBUTES Attributes;
    PROOTHUB_20PORTS_INFO Payload;
    PROOTHUB_20PORT_INFO Entries;
    WDFMEMORY Block = NULL;
    WDFMEMORY PayloadMemory;
    WDFREQUEST Request;
    PVOID Buffer;
    ULONG Index;

    if (Count == 0)
    {
        m_Hub->m_Root20Ports = NULL;
        m_Hub->Post(HubEvent::IoctlDone);
        return;
    }

    if (!NT_SUCCESS(HubCreateUcxRequest(m_Hub, Target, &Request)))
    {
        m_Hub->Post(HubEvent::IoctlFailed);
        return;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = m_Hub->m_Device;

    if (!NT_SUCCESS(WdfMemoryCreate(&Attributes,
                                    NonPagedPool,
                                    HUB_TAG_PORT,
                                    Count * (sizeof(ROOTHUB_20PORT_INFO) + sizeof(PVOID)),
                                    &Block,
                                    &Buffer)))
    {
        DPRINT1("Hub %p no memory for %lu root 2.0 ports\n", m_Hub, Count);
        goto Failed;
    }

    m_Hub->m_Root20Ports = (PROOTHUB_20PORT_INFO*)Buffer;
    Entries = (PROOTHUB_20PORT_INFO)(m_Hub->m_Root20Ports + Count);

    for (Index = 0; Index < Count; Index++)
    {
        m_Hub->m_Root20Ports[Index] = &Entries[Index];
        Entries[Index].MinorRevision = 0;
        Entries[Index].HubDepth = 0;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Request;

    if (!NT_SUCCESS(WdfMemoryCreate(&Attributes,
                                    NonPagedPool,
                                    HUB_TAG_PORT,
                                    sizeof(*Payload),
                                    &PayloadMemory,
                                    (PVOID*)&Payload)))
    {
        DPRINT1("Hub %p no memory for the 2.0 port info request\n", m_Hub);
        goto Failed;
    }

    Payload->Size = sizeof(*Payload);
    Payload->NumberOfPorts = (USHORT)Count;
    Payload->PortInfoSize = sizeof(*Entries);
    Payload->PortInfoArray = m_Hub->m_Root20Ports;

    HubSendUcxIoctl(m_Hub, Target, IOCTL_UCXHUB_ROOTHUB_GET_20PORT_INFO, Payload, Request, HubRoot20PortsComplete);
    return;

Failed:
    WdfObjectDelete(Request);
    if (Block != NULL)
    {
        WdfObjectDelete(Block);
        m_Hub->m_Root20Ports = NULL;
    }
    m_Hub->Post(HubEvent::IoctlFailed);
}

/* Like the 2.0 request, with room for each port's SuperSpeedPlus speeds */
VOID
HubMachine::GetRootHub30Ports()
{
    ULONG Count = m_Hub->m_RootHubInfo.Info.NumberOf30Ports;
    WDFIOTARGET Target = WdfDeviceGetIoTarget(m_Hub->m_Device);
    WDF_OBJECT_ATTRIBUTES Attributes;
    PROOTHUB_30PORTS_INFO Payload;
    PROOTHUB_30PORT_INFO_EX Entries;
    PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED Speeds;
    WDFMEMORY Block = NULL;
    WDFMEMORY PayloadMemory;
    WDFREQUEST Request;
    PVOID Buffer;
    ULONG Index;

    if (Count == 0)
    {
        m_Hub->m_Root30Ports = NULL;
        m_Hub->Post(HubEvent::IoctlDone);
        return;
    }

    if (!NT_SUCCESS(HubCreateUcxRequest(m_Hub, Target, &Request)))
    {
        m_Hub->Post(HubEvent::IoctlFailed);
        return;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = m_Hub->m_Device;

    if (!NT_SUCCESS(WdfMemoryCreate(&Attributes,
                                    NonPagedPool,
                                    HUB_TAG_PORT,
                                    Count * (sizeof(PVOID) + sizeof(ROOTHUB_30PORT_INFO_EX) +
                                             HUB_ROOT_PORT_MAX_SPEEDS * sizeof(*Speeds)),
                                    &Block,
                                    &Buffer)))
    {
        DPRINT1("Hub %p no memory for %lu root 3.0 ports\n", m_Hub, Count);
        goto Failed;
    }

    m_Hub->m_Root30Ports = (PROOTHUB_30PORT_INFO_EX*)Buffer;
    Entries = (PROOTHUB_30PORT_INFO_EX)(m_Hub->m_Root30Ports + Count);
    Speeds = (PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED)(Entries + Count);

    for (Index = 0; Index < Count; Index++)
    {
        m_Hub->m_Root30Ports[Index] = &Entries[Index];
        Entries[Index].Info.MinorRevision = 0;
        Entries[Index].Info.HubDepth = 0;
        Entries[Index].Speeds = &Speeds[Index * HUB_ROOT_PORT_MAX_SPEEDS];
        Entries[Index].SpeedsCount = 0;
        Entries[Index].MaxSpeedsCount = HUB_ROOT_PORT_MAX_SPEEDS;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Request;

    if (!NT_SUCCESS(WdfMemoryCreate(&Attributes,
                                    NonPagedPool,
                                    HUB_TAG_PORT,
                                    sizeof(*Payload),
                                    &PayloadMemory,
                                    (PVOID*)&Payload)))
    {
        DPRINT1("Hub %p no memory for the 3.0 port info request\n", m_Hub);
        goto Failed;
    }

    Payload->Size = sizeof(*Payload);
    Payload->NumberOfPorts = (USHORT)Count;
    Payload->PortInfoSize = sizeof(*Entries);
    Payload->PortInfoArray = (PROOTHUB_30PORT_INFO*)m_Hub->m_Root30Ports;

    HubSendUcxIoctl(m_Hub, Target, IOCTL_UCXHUB_ROOTHUB_GET_30PORT_INFO, Payload, Request, HubRoot30PortsComplete);
    return;

Failed:
    WdfObjectDelete(Request);
    if (Block != NULL)
    {
        WdfObjectDelete(Block);
        m_Hub->m_Root30Ports = NULL;
    }
    m_Hub->Post(HubEvent::IoctlFailed);
}

/* Any interface of the configuration with the multi TT hub protocol */
static
BOOLEAN
NTAPI
HubHasMultiTtInterface(
    _In_ PUSB_CONFIGURATION_DESCRIPTOR Config)
{
    PUCHAR Current = (PUCHAR)Config;
    PUCHAR End = Current + Config->wTotalLength;
    PUSB_COMMON_DESCRIPTOR Common;
    PUSB_INTERFACE_DESCRIPTOR Interface;

    while (Current + sizeof(*Common) <= End)
    {
        Common = (PUSB_COMMON_DESCRIPTOR)Current;
        if (Common->bLength == 0)
            break;

        if (Common->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE &&
            Current + sizeof(*Interface) <= End)
        {
            Interface = (PUSB_INTERFACE_DESCRIPTOR)Current;
            if (Interface->bInterfaceClass == HUB_INTERFACE_CLASS &&
                Interface->bInterfaceProtocol == HUB_PROTOCOL_MULTI_TT)
            {
                return TRUE;
            }
        }

        Current += Common->bLength;
    }

    return FALSE;
}

/* External hubs only; TT flags are only ever set */
VOID
HubMachine::UpdateControllerHubInfo()
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    PUSBDEVICE_HUB_INFO Payload;
    WDFMEMORY PayloadMemory;
    WDFREQUEST Request;
    ULONG Ports = m_Hub->m_PortCount;

    if (!NT_SUCCESS(HubCreateUcxRequest(m_Hub, m_Hub->m_RootHubTarget, &Request)))
    {
        m_Hub->Post(HubEvent::IoctlFailed);
        return;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Request;

    if (!NT_SUCCESS(WdfMemoryCreate(&Attributes,
                                    NonPagedPool,
                                    HUB_TAG_HUB,
                                    sizeof(*Payload),
                                    &PayloadMemory,
                                    (PVOID*)&Payload)))
    {
        DPRINT1("Hub %p no memory for the UCX hub info request\n", m_Hub);
        WdfObjectDelete(Request);
        m_Hub->Post(HubEvent::IoctlFailed);
        return;
    }

    RtlZeroMemory(Payload, sizeof(*Payload));
    Payload->Header.Size = sizeof(*Payload);
    Payload->Header.Hub = m_Hub->UsbDevice();
    Payload->Header.UsbDevice = m_Hub->UsbDevice();
    Payload->NumberOfPorts = Ports;

    if (m_Hub->m_Parent.HubSpeed == UsbHighSpeed)
    {
        m_Hub->SetFlag(HubFlag::TtHub);
        Payload->NumberOfTTs = 1;

        if (m_Hub->m_ConfigDescriptor != NULL && HubHasMultiTtInterface(m_Hub->m_ConfigDescriptor))
        {
            m_Hub->SetFlag(HubFlag::MultiTtHub);
            Payload->NumberOfTTs = Ports;
        }

        Payload->TTThinkTime = (m_Hub->m_HubDescriptor.Usb20.wHubCharacteristics >> 5) & 0x3;
    }

    DPRINT("Hub %p telling UCX of %lu ports, %lu TTs\n", m_Hub, Ports, (ULONG)Payload->NumberOfTTs);
    HubSendUcxIoctl(m_Hub,
                    m_Hub->m_RootHubTarget,
                    IOCTL_UCXHUB_DEVICE_HUB_INFO,
                    Payload,
                    Request,
                    HubControllerHubInfoComplete);
}

/* The one pipe the hub needs while paging: its status change pipe */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubRequestReservedIo(
    _In_ HubFdo* Hub)
{
    UCXHUB_FORWARD_PROGRESS_INFO Info;
    WDF_MEMORY_DESCRIPTOR Argument;
    NTSTATUS Status;

    PAGED_CODE();

    RtlZeroMemory(&Info, sizeof(Info));
    Info.Version = 1;
    Info.Size = sizeof(Info);
    Info.Device = Hub->UsbDevice();
    Info.ControlPipeMaxTransferSize = HUB_FORWARD_PROGRESS_CONTROL;
    Info.NumberOfPipes = 1;
    Info.Pipes[0].PipeHandle = Hub->m_Interrupt.Pipe;
    Info.Pipes[0].ReservedTransferLimit = Hub->IsRootHub() ? Hub->m_Interrupt.BitmapMaxBytes
                                                                    : Hub->m_InterruptMaxPacket;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&Argument, &Info, sizeof(Info));

    Status = WdfIoTargetSendInternalIoctlOthersSynchronously(Hub->m_RootHubTarget,
                                                             NULL,
                                                             IOCTL_UCXHUB_NOTIFY_FORWARD_PROGRESS,
                                                             &Argument,
                                                             NULL,
                                                             NULL,
                                                             NULL,
                                                             NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p forward progress notification failed 0x%lx\n", Hub, Status);
}
