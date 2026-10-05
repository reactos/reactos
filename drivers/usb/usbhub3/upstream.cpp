/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Requests a hub sends to its parent: configuration, resets, hub info
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Hub class interface and its protocols */
#define HUB_CLASS_CODE                  9
#define HUB_PROTOCOL_SINGLE_TT          1
#define HUB_PROTOCOL_MULTI_TT           2
#define HUB_PROTOCOL_ANY                (-1)

/* A SuperSpeed hub status change endpoint carries at most two bytes */
#define HUB_30_MAX_STATUS_PACKET        2

static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubConfigurationComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubPipeResetComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubResetComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubParentInfoComplete;

/** Internal IOCTL to the hub's parent with Argument1 = Payload. FALSE when refused. */
static
BOOLEAN
NTAPI
HubSendToParent(
    _In_ HubFdo* Hub,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode,
    _In_ PVOID Payload,
    _In_ PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion,
    _In_ WDFCONTEXT Context)
{
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack.Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    Stack.Parameters.Others.Argument1 = Payload;
    WdfRequestWdmFormatUsingStackLocation(Request, &Stack);

    WdfRequestSetCompletionRoutine(Request, Completion, Context);

    if (WdfRequestSend(Request, WdfDeviceGetIoTarget(Hub->m_Device), WDF_NO_SEND_OPTIONS))
        return TRUE;

    /* A refusal with a success status leaves nothing to report */
    Status = WdfRequestGetStatus(Request);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p parent request 0x%lx not sent 0x%lx\n", Hub, IoControlCode, Status);

    return NT_SUCCESS(Status);
}

static
NTSTATUS
NTAPI
HubCreateParentRequest(
    _In_ HubFdo* Hub,
    _Out_ WDFREQUEST* Request)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    *Request = NULL;
    Status = WdfRequestCreate(&Attributes, WdfDeviceGetIoTarget(Hub->m_Device), Request);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p parent request create failed 0x%lx\n", Hub, Status);

    return Status;
}

/* Configuration */

/* High speed hubs prefer their multi TT interface */
static
PUSB_INTERFACE_DESCRIPTOR
NTAPI
HubFindHubInterface(
    _In_ HubFdo* Hub)
{
    static const LONG HighSpeedOrder[] = { HUB_PROTOCOL_MULTI_TT, HUB_PROTOCOL_SINGLE_TT, 0, HUB_PROTOCOL_ANY };
    static const LONG OtherOrder[] = { HUB_PROTOCOL_SINGLE_TT, 0, HUB_PROTOCOL_ANY };
    PUSB_CONFIGURATION_DESCRIPTOR Config = Hub->m_ConfigDescriptor;
    PUSB_INTERFACE_DESCRIPTOR Interface;
    const LONG* Order;
    ULONG Count;
    ULONG Index;

    switch (Hub->m_Parent.HubSpeed)
    {
        case UsbHighSpeed:
            Order = HighSpeedOrder;
            Count = RTL_NUMBER_OF(HighSpeedOrder);
            break;

        case UsbFullSpeed:
        case UsbSuperSpeed:
            Order = OtherOrder;
            Count = RTL_NUMBER_OF(OtherOrder);
            break;

        default:
            return NULL;
    }

    for (Index = 0; Index < Count; Index++)
    {
        Interface = USBD_ParseConfigurationDescriptorEx(Config, Config, -1, -1, HUB_CLASS_CODE, -1, Order[Index]);
        if (Interface != NULL)
            return Interface;
    }

    return NULL;
}

/* ConfigurationFailed is posted when the URB cannot be built */
VOID
HubMachine::SelectHubConfiguration()
{
    USBD_INTERFACE_LIST_ENTRY List[2];
    PUSB_INTERFACE_DESCRIPTOR Interface;
    WDFREQUEST Request = NULL;
    PURB Urb = NULL;

    Interface = HubFindHubInterface(m_Hub);
    if (Interface == NULL ||
        Interface->bInterfaceClass != HUB_CLASS_CODE ||
        Interface->bNumEndpoints == 0)
    {
        DPRINT1("Hub %p has no usable hub interface\n", m_Hub);
        goto Failed;
    }

    RtlZeroMemory(List, sizeof(List));
    List[0].InterfaceDescriptor = Interface;

    Urb = USBD_CreateConfigurationRequestEx(m_Hub->m_ConfigDescriptor, List);
    if (Urb == NULL)
    {
        DPRINT1("Hub %p select configuration URB not built\n", m_Hub);
        goto Failed;
    }

    DPRINT("Hub %p selecting interface %u protocol %u\n",
           m_Hub,
           Interface->bInterfaceNumber,
           Interface->bInterfaceProtocol);

    if (!NT_SUCCESS(HubCreateParentRequest(m_Hub, &Request)))
        goto Failed;

    if (HubSendToParent(m_Hub, Request, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, HubConfigurationComplete, Urb))
        return;

Failed:
    if (Request != NULL)
        WdfObjectDelete(Request);
    if (Urb != NULL)
        ExFreePool(Urb);

    m_Hub->Post(HubEvent::ConfigurationFailed);
}

/* Pipe 0 is the status change endpoint; a SuperSpeed hub may use at most two bytes, zero and one included */
static
VOID
NTAPI
HubConfigurationComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    PURB Urb = (PURB)Context;
    HubFdo* Hub = HubFdo::FromDevice(WdfIoTargetGetDevice(Target));
    NTSTATUS Status = Params->IoStatus.Status;
    PUSBD_PIPE_INFORMATION Pipe;

    if (NT_SUCCESS(Status))
    {
        Pipe = &Urb->UrbSelectConfiguration.Interface.Pipes[0];

        Hub->m_Interrupt.Pipe = Pipe->PipeHandle;
        Hub->m_ConfigurationHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;
        Hub->m_InterruptMaxPacket = Pipe->MaximumPacketSize;

        if (Hub->m_ParentInfo.DeviceDescriptor.bcdUSB >= 0x0300 &&
            Pipe->MaximumPacketSize != HUB_30_MAX_STATUS_PACKET)
        {
            DPRINT1("Hub %p SuperSpeed status endpoint packet size %u\n", Hub, Pipe->MaximumPacketSize);
        }

        if (Hub->m_ParentInfo.DeviceDescriptor.bcdUSB >= 0x0300 &&
            Pipe->MaximumPacketSize > HUB_30_MAX_STATUS_PACKET)
        {
            Status = STATUS_INVALID_HW_PROFILE;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p set configuration failed 0x%lx, URB status 0x%lx\n",
                Hub,
                Status,
                Urb->UrbHeader.Status);
    }

    WdfObjectDelete(Request);
    ExFreePool(Urb);

    Hub->Post(NT_SUCCESS(Status) ? HubEvent::ConfigurationSet : HubEvent::ConfigurationFailed);
}

/* Status change pipe reset, on the hub's own control request */

static
VOID
NTAPI
HubPipeResetComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);

    if (!NT_SUCCESS(Params->IoStatus.Status))
    {
        DPRINT1("Hub %p status change pipe reset failed 0x%lx, URB status 0x%lx\n",
                Hub,
                Params->IoStatus.Status,
                Hub->m_PipeUrb.Hdr.Status);
    }

    Hub->m_Control.Reuse();
    Hub->Post(NT_SUCCESS(Params->IoStatus.Status) ? HubEvent::PipeResetDone : HubEvent::PipeResetFailed);
}

VOID
HubMachine::ResetStatusChangePipe()
{
    struct _URB_PIPE_REQUEST* Urb = &m_Hub->m_PipeUrb;

    RtlZeroMemory(Urb, sizeof(*Urb));
    Urb->Hdr.Length = sizeof(*Urb);
    Urb->Hdr.Function = URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL;
    Urb->Hdr.UsbdDeviceHandle = m_Hub->UsbDevice();
    Urb->PipeHandle = m_Hub->m_Interrupt.Pipe;

    if (!HubSendToParent(m_Hub, m_Hub->m_Control.Request, IOCTL_INTERNAL_USB_SUBMIT_URB, Urb, HubPipeResetComplete, m_Hub))
    {
        m_Hub->m_Control.Reuse();
        m_Hub->Post(HubEvent::PipeResetFailed);
    }
}

/* Hub reset through the parent port */

static
VOID
NTAPI
HubResetComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;
    NTSTATUS Status = Params->IoStatus.Status;
    UCXHUB_RESET_FLAGS Flags;

    UNREFERENCED_PARAMETER(Target);

    if (NT_SUCCESS(Status))
        DPRINT("Hub %p reset by parent, flags 0x%lx\n", Hub, Hub->m_ResetFlags);
    else
        DPRINT1("Hub %p reset by parent failed 0x%lx\n", Hub, Status);

    Flags.AsUlong = Hub->m_ResetFlags;
    if (NT_SUCCESS(Status) && Flags.HostContextLost)
        Hub->MarkDevicesProgrammingLost();

    WdfObjectDelete(Request);

    if (NT_SUCCESS(Status))
        Hub->Post(HubEvent::ResetDone);
    else if (Status == STATUS_NO_SUCH_DEVICE)
        Hub->Post(HubEvent::ResetFailedOnRemoval);
    else
        Hub->Post(HubEvent::ResetFailed);
}

VOID
HubMachine::ResetHub()
{
    WDFREQUEST Request;

    if (!NT_SUCCESS(HubCreateParentRequest(m_Hub, &Request)))
    {
        m_Hub->Post(HubEvent::ResetFailed);
        return;
    }

    m_Hub->m_ResetFlags = 0;
    DPRINT("Hub %p asking parent for a reset\n", m_Hub);

    if (!HubSendToParent(m_Hub, Request, IOCTL_UCXHUB_RESET_PORT_ASYNC, &m_Hub->m_ResetFlags, HubResetComplete, m_Hub))
    {
        WdfObjectDelete(Request);
        m_Hub->Post(HubEvent::ResetFailed);
    }
}

/* Hub info from the parent hub, external hubs only */

/* Control transfers go straight to the root hub PDO; the previous target is closed on each start */
static
VOID
NTAPI
HubParentInfoComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;
    NTSTATUS Status = Params->IoStatus.Status;
    WDF_IO_TARGET_OPEN_PARAMS Open;
    WDFIOTARGET RootTarget;

    UNREFERENCED_PARAMETER(Target);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p parent hub info failed 0x%lx\n", Hub, Status);
    }
    else
    {
        Status = WdfIoTargetCreate(Hub->m_Device, WDF_NO_OBJECT_ATTRIBUTES, &RootTarget);
        if (!NT_SUCCESS(Status))
            DPRINT1("Hub %p root hub target create failed 0x%lx\n", Hub, Status);
    }

    if (NT_SUCCESS(Status))
    {
        WDF_IO_TARGET_OPEN_PARAMS_INIT_EXISTING_DEVICE(&Open, Hub->m_ParentInfo.RootHubPdo);

        Status = WdfIoTargetOpen(RootTarget, &Open);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Hub %p root hub target open on %p failed 0x%lx\n",
                    Hub,
                    Hub->m_ParentInfo.RootHubPdo,
                    Status);
            WdfObjectDelete(RootTarget);
        }
        else
        {
            if (Hub->m_RootHubTarget != NULL)
                WdfObjectDelete(Hub->m_RootHubTarget);
            Hub->m_RootHubTarget = RootTarget;
        }
    }

    if (NT_SUCCESS(Status) && (Hub->m_ParentInfo.Flags & HUB_PARENT_DISABLE_LPM))
        Hub->SetFlag(HubFlag::DisableLpm);

    WdfObjectDelete(Request);
    Hub->Post(NT_SUCCESS(Status) ? HubEvent::IoctlDone : HubEvent::IoctlFailed);
}

VOID
HubMachine::GetParentInfo()
{
    WDFREQUEST Request;

    if (!NT_SUCCESS(HubCreateParentRequest(m_Hub, &Request)))
    {
        m_Hub->Post(HubEvent::IoctlFailed);
        return;
    }

    if (!HubSendToParent(m_Hub, Request, IOCTL_UCXHUB_GET_HUB_INFO, &m_Hub->m_ParentInfo, HubParentInfoComplete, m_Hub))
    {
        WdfObjectDelete(Request);
        m_Hub->Post(HubEvent::IoctlFailed);
    }
}

/*
 * Both parent callbacks clear what they report, so this is called even when
 * the hub machine ignores the answer.
 */
BOOLEAN
HubMachine::WasResetByParent()
{
    PVOID Context = m_Hub->m_Parent.Header.Context;

    if (!m_Hub->m_Parent.ParentResetDuringResume(Context))
        return FALSE;

    if (m_Hub->m_Parent.ParentLostStateDuringResume(Context))
    {
        DPRINT("Hub %p lost controller programming on resume\n", m_Hub);
        m_Hub->MarkDevicesProgrammingLost();
    }

    return TRUE;
}
