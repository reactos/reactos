/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Query interfaces the root hub PDO answers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
UcxInterfaceNoOp(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/* Stack interface members. The Hub argument is unused unless noted. */

static
NTSTATUS
NTAPI
UcxHubDeviceCreate(
    _In_ UCXUSBDEVICE Hub,
    _In_ PUCXHUB_DEVICE_CREATE_INFO CreateInfo,
    _Out_ UCXUSBDEVICE* Device)
{
    return UcxUsbDevice::CreateFromHub(Hub, CreateInfo, Device);
}

static
VOID
NTAPI
UcxHubDeviceDelete(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device)
{
    UNREFERENCED_PARAMETER(Hub);

    UcxUsbDevice::FromHandle(Device)->DeleteFromHub();
}

static
VOID
NTAPI
UcxHubDeviceDisconnect(
    _In_ UCXUSBDEVICE Hub,
    _In_ ULONG PortNumber)
{
    UcxUsbDevice::FromHandle(Hub)->DisconnectPort(PortNumber);
}

static
VOID
NTAPI
UcxHubDeviceSetPdo(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ PDEVICE_OBJECT Pdo)
{
    UNREFERENCED_PARAMETER(Hub);

    UcxUsbDevice::FromHandle(Device)->SetPdo(Pdo);
}

static
NTSTATUS
NTAPI
UcxHubCreateControlEndpoint(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ ULONG MaxPacketSize,
    _Out_ UCXENDPOINT* Endpoint)
{
    UNREFERENCED_PARAMETER(Hub);

    return UcxEndpoint::CreateDefaultFromHub(Device, MaxPacketSize, Endpoint);
}

static
VOID
NTAPI
UcxHubAddress0Release(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device)
{
    UNREFERENCED_PARAMETER(Device);

    UcxUsbDevice::FromHandle(Hub)->m_Controller->Address0Release(Hub);
}

static
NTSTATUS
NTAPI
UcxHubEndpointCreate(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Descriptor,
    _In_ ULONG DescriptorBufferLength,
    _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion,
    _Out_ UCXENDPOINT* Endpoint)
{
    UNREFERENCED_PARAMETER(Hub);

    return UcxEndpoint::CreateFromHub(Device, Descriptor, DescriptorBufferLength, Companion, Endpoint);
}

static
VOID
NTAPI
UcxHubEndpointDelete(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(Hub);
    UNREFERENCED_PARAMETER(Device);

    UcxEndpoint::FromHandle(Endpoint)->DeleteFromHub();
}

/* Set when the hub hands the pipe to a client driver; never cleared */
static
VOID
NTAPI
UcxHubEndpointExposed(
    _In_ UCXENDPOINT Endpoint)
{
    UcxEndpoint::FromHandle(Endpoint)->m_ClientHoldsHandle = TRUE;
}

static
USBD_PIPE_HANDLE
NTAPI
UcxHubEndpointPipeHandle(
    _In_ UCXENDPOINT Endpoint)
{
    return UcxEndpoint::FromHandle(Endpoint)->m_Pipe.Handle();
}

static
ULONG
NTAPI
UcxHubEndpointMaxTransfer(
    _In_ UCXENDPOINT Endpoint)
{
    return UcxEndpoint::FromHandle(Endpoint)->m_Pipe.MaximumTransferSize;
}

static
PUCXHUB_WORKITEM
NTAPI
UcxHubWorkItemAllocate(
    _In_ UCXUSBDEVICE Hub,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG Flags)
{
    return UcxWorkItem::Allocate(UcxUsbDevice::FromHandle(Hub)->m_Controller, DeviceObject, Flags);
}

static
VOID
NTAPI
UcxHubWorkItemDelete(
    _In_ PUCXHUB_WORKITEM WorkItem)
{
    WorkItem->Free();
}

static
VOID
NTAPI
UcxHubWorkItemEnqueue(
    _In_ PUCXHUB_WORKITEM WorkItem,
    _In_ PUCXHUB_WORKITEM_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_ UCXHUB_WORKITEM_ENQUEUE_OPTIONS Options)
{
    WorkItem->Enqueue(Routine, Context, Options);
}

static
VOID
NTAPI
UcxHubWorkItemFlush(
    _In_ PUCXHUB_WORKITEM WorkItem)
{
    WorkItem->Flush();
}

static
VOID
NTAPI
UcxHubClearTtBufferComplete(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXENDPOINT Endpoint)
{
    UNREFERENCED_PARAMETER(Hub);

    UcxEndpoint::FromHandle(Endpoint)->ReleaseCancelSync();
}

/* Ancestors are not checked; a disconnect already marked every descendant */
static
BOOLEAN
NTAPI
UcxHubIsDeviceDisconnected(
    _In_ UCXUSBDEVICE Device)
{
    return UcxUsbDevice::FromHandle(Device)->m_Disconnected;
}

static
BOOLEAN
NTAPI
UcxHubStreamsSupported(
    _In_ UCXUSBDEVICE Hub)
{
    return UcxUsbDevice::FromHandle(Hub)->m_Controller->QueryStreamsSupported();
}

static
NTSTATUS
NTAPI
UcxHubBlockControllerIdle(
    _In_ UCXUSBDEVICE Hub,
    _Inout_ PUCXHUB_STOP_IDLE_CONTEXT Context)
{
    return UcxUsbDevice::FromHandle(Hub)->m_Controller->StopIdle(Context);
}

static
VOID
NTAPI
UcxHubAllowControllerIdle(
    _In_ UCXUSBDEVICE Hub,
    _Inout_ PUCXHUB_STOP_IDLE_CONTEXT Context)
{
    UcxUsbDevice::FromHandle(Hub)->m_Controller->ResumeIdle(Context);
}

static
VOID
NTAPI
UcxHubQueryControllerBus(
    _In_ UCXUSBDEVICE Hub,
    _Out_ PUCXHUB_CONTROLLER_INFO Info)
{
    UcxUsbDevice* HubDevice = UcxUsbDevice::FromHandle(Hub);

    HubDevice->m_Controller->GetInfo(Info, HubDevice->m_HubUsesGrownInterface);
}

static
ULONG64
NTAPI
UcxHubDeviceGetTimestamp(
    _In_ UCXUSBDEVICE Device)
{
    return UcxUsbDevice::FromHandle(Device)->Timestamp();
}

static
VOID
NTAPI
UcxHubEndpointSetPriority(
    _In_ UCXUSBDEVICE Hub,
    _In_ UCXUSBDEVICE Device,
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG Priority)
{
    UcxEndpoint* Context = UcxEndpoint::FromHandle(Endpoint);
    UCX_ENDPOINT_CHARACTERISTIC Characteristic;

    UNREFERENCED_PARAMETER(Hub);
    UNREFERENCED_PARAMETER(Device);

    if (Context->m_Callbacks.SetCharacteristic == NULL)
        return;

    RtlZeroMemory(&Characteristic, sizeof(Characteristic));
    Characteristic.Size = sizeof(Characteristic);
    Characteristic.CharacteristicType = UCX_ENDPOINT_CHARACTERISTIC_TYPE_PRIORITY;
    Characteristic.Priority = (UCX_CONTROLLER_ENDPOINT_CHARACTERISTIC_PRIORITY)Priority;

    Context->m_Callbacks.SetCharacteristic(Endpoint, &Characteristic);
}

/* Parent interface the root hub offers its hub */

static
BOOLEAN
NTAPI
UcxParentResetDuringResume(
    _In_ PVOID Context)
{
    UcxController* Controller = UcxRootHub::FromHandle((UCXROOTHUB)Context)->m_Controller;

    return InterlockedExchange(&Controller->m_RootHubResetSeen, 0) != 0;
}

static
BOOLEAN
NTAPI
UcxParentWasProgrammingLost(
    _In_ PVOID Context)
{
    UcxController* Controller = UcxRootHub::FromHandle((UCXROOTHUB)Context)->m_Controller;

    return InterlockedExchange(&Controller->m_DeviceContextsLost, 0) != 0;
}

/* The buffer stays valid until the root hub PDO releases its hardware */
static
VOID
NTAPI
UcxParentGetHubSymbolicName(
    _In_ PVOID Context,
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    UcxRootHub* RootHub = UcxRootHub::FromHandle((UCXROOTHUB)Context);

    RtlInitEmptyUnicodeString(SymbolicLinkName, NULL, 0);

    if (RootHub->m_SymbolicName != NULL)
        WdfStringGetUnicodeString(RootHub->m_SymbolicName, SymbolicLinkName);
}

/** Fills whichever parent layout the hub asked for; both share every field but one. */
template <typename T>
static
VOID
NTAPI
UcxFillParentInterface(
    _Out_ T* Parent,
    _In_ UcxRootHub* RootHub)
{
    const UCX_CONTROLLER_PCI_INFORMATION* Pci = &RootHub->m_Controller->m_Config.PciDeviceInfo;

    /* Wipes the hub's debug context too, harmlessly */
    RtlZeroMemory(Parent, sizeof(*Parent));

    Parent->Header.Size = sizeof(*Parent);
    Parent->Header.Version = UCXHUB_PARENT_INTERFACE_VERSION;
    Parent->Header.Context = RootHub->m_Handle;
    Parent->Header.InterfaceReference = UcxInterfaceNoOp;
    Parent->Header.InterfaceDereference = UcxInterfaceNoOp;

    Parent->HubDepth = 0;
    Parent->Hub = (UCXUSBDEVICE)RootHub->m_Handle;

    /* UCX wakes the root hub itself on a port change */
    Parent->ParentCanWake = TRUE;

    Parent->ParentResetDuringResume = UcxParentResetDuringResume;
    Parent->ParentLostStateDuringResume = UcxParentWasProgrammingLost;
    Parent->QueryHubLinkName = UcxParentGetHubSymbolicName;

    /* Taken from the PCI fields whatever the parent bus is */
    Parent->HubTopologyAddress.PciBusNumber = Pci->BusNumber;
    Parent->HubTopologyAddress.PciDeviceNumber = Pci->DeviceNumber;
    Parent->HubTopologyAddress.PciFunctionNumber = Pci->FunctionNumber;
}

NTSTATUS
NTAPI
UcxEvtQueryParentInterface(
    _In_ WDFDEVICE Device,
    _In_ LPGUID InterfaceType,
    _Inout_ PINTERFACE ExposedInterface,
    _Inout_opt_ PVOID ExposedInterfaceSpecificData)
{
    UcxRootHub* RootHub = UcxGetRootHubPdoContext(Device)->RootHub;

    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(ExposedInterfaceSpecificData);

    if (ExposedInterface->Size != sizeof(UCXHUB_PARENT_INTERFACE) &&
        ExposedInterface->Size != sizeof(UCXHUB_PARENT_INTERFACE_ORIGINAL))
    {
        DPRINT1("Parent interface size %u not accepted\n", ExposedInterface->Size);
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (ExposedInterface->Version != UCXHUB_PARENT_INTERFACE_VERSION)
    {
        DPRINT1("Parent interface version %u not supported\n", ExposedInterface->Version);
        return STATUS_NOT_SUPPORTED;
    }

    /* The current layout added the tunnel state, which is 0 for the root hub */
    if (ExposedInterface->Size == sizeof(UCXHUB_PARENT_INTERFACE))
        UcxFillParentInterface((PUCXHUB_PARENT_INTERFACE)ExposedInterface, RootHub);
    else
        UcxFillParentInterface((PUCXHUB_PARENT_INTERFACE_ORIGINAL)ExposedInterface, RootHub);

    DPRINT("Parent interface size %u handed to root hub %p's hub\n", ExposedInterface->Size, RootHub);
    return STATUS_SUCCESS;
}

/* Writes one stack interface slot if the requested size covers it */
#define UCX_FILL_SLOT(Interface, Limit, Field, Value)                              \
    do                                                                              \
    {                                                                               \
        if (FIELD_OFFSET(UCXHUB_STACK_INTERFACE, Field) + sizeof((Interface)->Field) <= (Limit)) \
            (Interface)->Field = (Value);                                           \
    } while (0)

/* Fills only what the requested size and version cover */
NTSTATUS
NTAPI
UcxEvtQueryStackInterface(
    _In_ WDFDEVICE Device,
    _In_ LPGUID InterfaceType,
    _Inout_ PINTERFACE ExposedInterface,
    _Inout_opt_ PVOID ExposedInterfaceSpecificData)
{
    PUCXHUB_STACK_INTERFACE Stack = (PUCXHUB_STACK_INTERFACE)ExposedInterface;
    ULONG Limit = ExposedInterface->Size;
    USHORT Version = ExposedInterface->Version;
    UcxUsbDevice* Hub;
    ULONG Filled;

    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(ExposedInterfaceSpecificData);

    if (Version < UCXHUB_STACK_INTERFACE_VERSION_1)
    {
        DPRINT1("Stack interface version %u too old, left unfilled\n", Version);
        return STATUS_SUCCESS;
    }

    Limit = min(Limit, (ULONG)sizeof(*Stack));

    Stack->Header.Context = Device;
    Stack->Header.InterfaceReference = UcxInterfaceNoOp;
    Stack->Header.InterfaceDereference = UcxInterfaceNoOp;

    UCX_FILL_SLOT(Stack, Limit, DeviceCreate, UcxHubDeviceCreate);
    UCX_FILL_SLOT(Stack, Limit, DeviceDelete, UcxHubDeviceDelete);
    UCX_FILL_SLOT(Stack, Limit, DeviceDisconnect, UcxHubDeviceDisconnect);
    UCX_FILL_SLOT(Stack, Limit, DeviceSetPdo, UcxHubDeviceSetPdo);
    UCX_FILL_SLOT(Stack, Limit, CreateControlEndpoint, UcxHubCreateControlEndpoint);
    UCX_FILL_SLOT(Stack, Limit, Address0Release, UcxHubAddress0Release);
    UCX_FILL_SLOT(Stack, Limit, EndpointCreate, UcxHubEndpointCreate);
    UCX_FILL_SLOT(Stack, Limit, EndpointDelete, UcxHubEndpointDelete);
    UCX_FILL_SLOT(Stack, Limit, MarkEndpointClientOwned, UcxHubEndpointExposed);
    UCX_FILL_SLOT(Stack, Limit, EndpointGetPipeHandle, UcxHubEndpointPipeHandle);
    UCX_FILL_SLOT(Stack, Limit, EndpointGetMaxTransferSize, UcxHubEndpointMaxTransfer);
    UCX_FILL_SLOT(Stack, Limit, WorkItemAllocate, UcxHubWorkItemAllocate);
    UCX_FILL_SLOT(Stack, Limit, WorkItemDelete, UcxHubWorkItemDelete);
    UCX_FILL_SLOT(Stack, Limit, WorkItemEnqueue, UcxHubWorkItemEnqueue);
    UCX_FILL_SLOT(Stack, Limit, WorkItemFlush, UcxHubWorkItemFlush);
    UCX_FILL_SLOT(Stack, Limit, ClearTtBufferComplete, UcxHubClearTtBufferComplete);
    UCX_FILL_SLOT(Stack, Limit, IsDeviceDisconnected, UcxHubIsDeviceDisconnected);
    UCX_FILL_SLOT(Stack, Limit, StreamsSupported, UcxHubStreamsSupported);
    UCX_FILL_SLOT(Stack, Limit, BlockControllerIdle, UcxHubBlockControllerIdle);
    UCX_FILL_SLOT(Stack, Limit, AllowControllerIdle, UcxHubAllowControllerIdle);
    UCX_FILL_SLOT(Stack, Limit, QueryControllerBus, UcxHubQueryControllerBus);
    UCX_FILL_SLOT(Stack, Limit, DeviceGetTimestamp, UcxHubDeviceGetTimestamp);

    Filled = min(Limit, (ULONG)FIELD_OFFSET(UCXHUB_STACK_INTERFACE, EndpointSetPriority));
    Stack->Header.Version = UCXHUB_STACK_INTERFACE_VERSION_1;

    if (Version >= UCXHUB_STACK_INTERFACE_VERSION_2)
    {
        UCX_FILL_SLOT(Stack, Limit, EndpointSetPriority, UcxHubEndpointSetPriority);
        Filled = min(Limit, (ULONG)FIELD_OFFSET(UCXHUB_STACK_INTERFACE, EndpointCreateEx));
        Stack->Header.Version = UCXHUB_STACK_INTERFACE_VERSION_2;
    }

    Stack->Header.Size = (USHORT)Filled;

    /* The hub's own half of the interface goes into its device context */
    Hub = UcxUsbDevice::FromHandle(Stack->Hub);
    NT_ASSERT(Hub->IsHub());
    Hub->m_HubContext = Stack->HubContext;
    Hub->m_HubClearTtBuffer = Stack->ClearTtBuffer;
    Hub->m_HubNoPingResponse = Stack->NoPingResponse;
    Hub->m_HubUsesGrownInterface = (ExposedInterface->Size > UCXHUB_STACK_INTERFACE_SIZE_ORIGINAL);

    DPRINT("Stack interface for hub %p, size %lu version %u\n", Hub, Filled, Stack->Header.Version);
    return STATUS_SUCCESS;
}

/* GUID_PNP_LOCATION_INTERFACE: the root hub is always USBROOT(0) */
static
NTSTATUS
NTAPI
UcxGetLocationString(
    _Inout_ PVOID Context,
    _Outptr_ PZZWSTR* LocationStrings)
{
    static const WCHAR Location[] = L"USBROOT(0)";
    PWCHAR Buffer;

    UNREFERENCED_PARAMETER(Context);

    *LocationStrings = NULL;

    /* One extra NUL makes it a multi string; PnP frees it */
    Buffer = (PWCHAR)ExAllocatePoolZero(PagedPool, sizeof(Location) + sizeof(WCHAR), UCX_POOL_TAG);
    if (Buffer == NULL)
    {
        DPRINT1("Root hub location string allocation failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Buffer, Location, sizeof(Location));
    *LocationStrings = Buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
UcxRootHub::AddQueryInterfaces()
{
    WDF_QUERY_INTERFACE_CONFIG Config;
    UCXHUB_PARENT_INTERFACE_ORIGINAL ParentTemplate;
    UCXHUB_STACK_INTERFACE StackTemplate;
    USBD_CLIENT_INTERFACE ClientTemplate;
    PNP_LOCATION_INTERFACE Location;
    NTSTATUS Status;

    /* Templates use the smallest accepted size; KMDF rejects anything below it */
    RtlZeroMemory(&ParentTemplate, sizeof(ParentTemplate));
    ParentTemplate.Header.Size = sizeof(ParentTemplate);
    ParentTemplate.Header.Version = UCXHUB_PARENT_INTERFACE_VERSION;
    WDF_QUERY_INTERFACE_CONFIG_INIT(&Config,
                                    (PINTERFACE)&ParentTemplate,
                                    &GUID_UCXHUB_PARENT_INTERFACE,
                                    UcxEvtQueryParentInterface);
    Config.ImportInterface = TRUE;
    Status = WdfDeviceAddQueryInterface(m_Pdo, &Config);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p parent interface add failed 0x%lx\n", m_Pdo, Status);
        return Status;
    }

    RtlZeroMemory(&StackTemplate, sizeof(StackTemplate));
    StackTemplate.Header.Size = UCXHUB_STACK_INTERFACE_SIZE_ORIGINAL;
    StackTemplate.Header.Version = UCXHUB_STACK_INTERFACE_VERSION_1;
    WDF_QUERY_INTERFACE_CONFIG_INIT(&Config,
                                    (PINTERFACE)&StackTemplate,
                                    &GUID_UCXHUB_STACK_INTERFACE,
                                    UcxEvtQueryStackInterface);
    Config.ImportInterface = TRUE;
    Status = WdfDeviceAddQueryInterface(m_Pdo, &Config);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p stack interface add failed 0x%lx\n", m_Pdo, Status);
        return Status;
    }

    WDF_QUERY_INTERFACE_CONFIG_INIT(&Config, NULL, &USB_BUS_INTERFACE_USBDI_GUID, UcxEvtQueryUsbdiInterface);
    Config.ImportInterface = TRUE;
    Status = WdfDeviceAddQueryInterface(m_Pdo, &Config);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p USBDI interface add failed 0x%lx\n", m_Pdo, Status);
        return Status;
    }

    RtlZeroMemory(&ClientTemplate, sizeof(ClientTemplate));
    ClientTemplate.Header.Size = sizeof(ClientTemplate);
    ClientTemplate.Header.Version = USBD_CLIENT_INTERFACE_VERSION;
    WDF_QUERY_INTERFACE_CONFIG_INIT(&Config,
                                    (PINTERFACE)&ClientTemplate,
                                    &GUID_USBD_CLIENT_INTERFACE,
                                    UcxEvtQueryUsbdClientInterface);
    Config.ImportInterface = TRUE;
    Status = WdfDeviceAddQueryInterface(m_Pdo, &Config);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p USBD client interface add failed 0x%lx\n", m_Pdo, Status);
        return Status;
    }

    RtlZeroMemory(&Location, sizeof(Location));
    Location.Size = sizeof(Location);
    Location.Version = PNP_LOCATION_INTERFACE_VERSION;
    Location.InterfaceReference = UcxInterfaceNoOp;
    Location.InterfaceDereference = UcxInterfaceNoOp;
    Location.GetLocationString = UcxGetLocationString;
    WDF_QUERY_INTERFACE_CONFIG_INIT(&Config, (PINTERFACE)&Location, &GUID_PNP_LOCATION_INTERFACE, NULL);
    Config.ImportInterface = FALSE;

    Status = WdfDeviceAddQueryInterface(m_Pdo, &Config);
    if (!NT_SUCCESS(Status))
        DPRINT1("Root hub PDO %p location interface add failed 0x%lx\n", m_Pdo, Status);

    return Status;
}
