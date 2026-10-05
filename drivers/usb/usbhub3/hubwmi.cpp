/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     WMI providers, user notifications and the over current reset method
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Missing from our usbiodef.h */
static const GUID HubWmiSurpriseRemovalGuid =
    { 0x9bbbf831, 0xa2f2, 0x43b4, { 0x96, 0xd1, 0x86, 0x94, 0x4b, 0x59, 0x14, 0xb3 } };

/* Layouts user mode tools parse */
C_ASSERT(sizeof(USB_CONNECTION_NOTIFICATION) == 24);
C_ASSERT(sizeof(USB_BUS_NOTIFICATION) == 16);
C_ASSERT(FIELD_OFFSET(USB_ACQUIRE_INFO, Buffer) == 8);
C_ASSERT(sizeof(USB_PIPE_INFO) == 11);
C_ASSERT(sizeof(USB_HUB_PORT_INFORMATION) == 16);
C_ASSERT(sizeof(USB_DEVICE_NODE_INFO) == 1202);
C_ASSERT(FIELD_OFFSET(USB_DEVICE_NODE_INFO, UsbDeviceInfo.PipeList) == 1191);
C_ASSERT(FIELD_OFFSET(USB_DEVICE_NODE_INFO, HubDeviceInfo.PortInfo) == 211);

/* Smallest USB_ACQUIRE_INFO a name request may use */
#define HUB_WMI_MIN_ACQUIRE_SIZE    10

/* Byte field of a packed node info block */
#define HUB_WMI_FIELD(Info, Field) \
    ((PUCHAR)(Info) + FIELD_OFFSET(USB_DEVICE_NODE_INFO, Field))

/* Guards HubDriver.SurpriseRemovalWmi against the cleanup of its instance */
static KSPIN_LOCK HubWmiSurpriseLock;

/** Hub symbolic link without its "\??\" prefix. Bounded by the string length, not a NUL. */
static
PCWSTR
NTAPI
HubWmiHubName(
    _In_ HubFdo* Hub,
    _Out_ PULONG Length)
{
    PCWSTR Text = Hub->m_SymbolicLinkName.Buffer;
    ULONG Count = Hub->m_SymbolicLinkName.Length / sizeof(WCHAR);
    ULONG Start = 0;

    *Length = 0;
    if (Text == NULL)
        return NULL;

    if (Count != 0 && Text[0] == L'\\')
    {
        for (Start = 1; Start < Count; Start++)
        {
            if (Text[Start] == L'\\' || Text[Start] == UNICODE_NULL)
                break;
        }

        if (Start < Count && Text[Start] == L'\\')
            Start++;
    }

    *Length = (Count - Start) * sizeof(WCHAR);
    return Text + Start;
}

static
VOID
NTAPI
HubWmiFirePortEvent(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber,
    _In_ USB_NOTIFICATION_TYPE Type)
{
    WDFWMIINSTANCE Instance = Hub->m_WmiInstance;
    USB_CONNECTION_NOTIFICATION Event;
    ULONG NameLength;
    NTSTATUS Status;

    /* A failed registration leaves no instance */
    if (Instance == NULL)
        return;

    if (!WdfWmiProviderIsEnabled(WdfWmiInstanceGetProvider(Instance), WdfWmiEventControl))
    {
        DPRINT("Hub %p: nobody listens for WMI event %d on port %lu\n", Hub, Type, PortNumber);
        return;
    }

    RtlZeroMemory(&Event, sizeof(Event));
    HubWmiHubName(Hub, &NameLength);

    Event.NotificationType = Type;
    Event.ConnectionNumber = PortNumber;

    /* What a USB_HUB_NAME query reports: the length field, the name and a NUL */
    Event.HubNameLength = NameLength + sizeof(ULONG) + sizeof(WCHAR);

    Status = WdfWmiInstanceFireEvent(Instance, sizeof(Event), &Event);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p: WMI event %d on port %lu failed 0x%lx\n", Hub, Type, PortNumber, Status);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubWmiCreateInstance(
    _In_ WDFDEVICE Device,
    _In_ const GUID* Guid,
    _In_ ULONG ProviderFlags,
    _In_opt_ PFN_WDF_WMI_INSTANCE_QUERY_INSTANCE Query,
    _In_opt_ PFN_WDF_WMI_INSTANCE_EXECUTE_METHOD Method,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_opt_ WDFWMIINSTANCE* Instance)
{
    WDF_WMI_PROVIDER_CONFIG Provider;
    WDF_WMI_INSTANCE_CONFIG Config;

    WDF_WMI_PROVIDER_CONFIG_INIT(&Provider, Guid);
    Provider.Flags = ProviderFlags;

    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&Config, &Provider);
    Config.Register = TRUE;
    Config.EvtWmiInstanceQueryInstance = Query;
    Config.EvtWmiInstanceExecuteMethod = Method;

    return WdfWmiInstanceCreate(Device, &Config, Attributes, Instance);
}

VOID
NTAPI
HubMakeUsb2StyleDescriptor(
    _In_ HubFdo* Hub,
    _Out_ PUSB_HUB_DESCRIPTOR Descriptor)
{
    ULONG MaskBytes;

    RtlZeroMemory(Descriptor, sizeof(*Descriptor));

    if (Hub->IsRootHub())
    {
        Descriptor->bPowerOnToPowerGood = 2;
    }
    else if (Hub->m_Parent.HubSpeed == UsbHighSpeed || Hub->m_Parent.HubSpeed == UsbFullSpeed)
    {
        RtlCopyMemory(Descriptor, &Hub->m_HubDescriptor.Usb20, sizeof(*Descriptor));
        return;
    }
    else if (Hub->m_Parent.HubSpeed == UsbSuperSpeed)
    {
        Descriptor->wHubCharacteristics = Hub->m_HubDescriptor.Usb30.wHubCharacteristics & 0x1F;
        Descriptor->bPowerOnToPowerGood = Hub->m_HubDescriptor.Usb30.bPowerOnToPowerGood;
        Descriptor->bHubControlCurrent = Hub->m_HubDescriptor.Usb30.bHubControlCurrent;
    }
    else
    {
        /* A low speed hub cannot exist */
        return;
    }

    /* Always 9, even when more than 7 ports need a longer mask */
    Descriptor->bDescriptorLength = 9;
    Descriptor->bDescriptorType = USB_20_HUB_DESCRIPTOR_TYPE;
    Descriptor->bNumberOfPorts = (UCHAR)Hub->m_PortCount;

    /* All ports removable, followed by an all ones power control mask */
    MaskBytes = Descriptor->bNumberOfPorts / 8 + 1;
    RtlFillMemory(&Descriptor->bRemoveAndPowerMask[MaskBytes], MaskBytes, 0xFF);
}

/* Node information of a hub FDO */

_IRQL_requires_(PASSIVE_LEVEL)
static
USB_CONNECTION_STATUS
NTAPI
HubWmiPortEntry(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ USHORT PortNumber,
    _Out_ PUSHORT DeviceAddress)
{
    WDFDEVICE Child = NULL;
    HubPdo* Pdo;

    *DeviceAddress = 0;

    WdfFdoLockStaticChildListForIteration(Hub->m_Device);

    while ((Child = WdfFdoRetrieveNextStaticChild(Hub->m_Device, Child, WdfRetrievePresentChildren)) != NULL)
    {
        Pdo = HubPdo::FromDevice(Child);
        if (Pdo->m_PortNumber != PortNumber)
            continue;

        if (Pdo->m_Child != NULL)
            *DeviceAddress = Pdo->m_Child->m_Address;
        break;
    }

    WdfFdoUnlockStaticChildListFromIteration(Hub->m_Device);

    return Port->m_ConnectionStatus;
}

static
NTSTATUS
NTAPI
HubWmiQueryHubNode(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG OutBufferSize,
    _Out_writes_bytes_to_(OutBufferSize, *BufferUsed) PVOID OutBuffer,
    _Out_ PULONG BufferUsed)
{
    HubFdo* Hub = HubFdo::FromDevice(WdfWmiInstanceGetDevice(WmiInstance));
    PUSB_DEVICE_NODE_INFO Info = (PUSB_DEVICE_NODE_INFO)OutBuffer;
    PUSB_HUB_PORT_INFORMATION Entry;
    USB_HUB_DESCRIPTOR Descriptor;
    USHORT PortCount = (USHORT)Hub->m_PortCount;
    USHORT Index;
    USHORT Address;
    HubPort* Port;
    ULONG Needed;

    /* Sized as one full device block plus the extra ports, larger than the data */
    Needed = sizeof(USB_DEVICE_NODE_INFO) + ((ULONG)PortCount - 1) * sizeof(USB_HUB_PORT_INFORMATION);
    *BufferUsed = Needed;
    if (OutBufferSize < Needed)
    {
        DPRINT("Hub %p WMI node query needs %lu bytes, got %lu\n", Hub, Needed, OutBufferSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(OutBuffer, OutBufferSize);

    Info->Sig = USB_NODE_INFO_SIG;
    Info->LengthInBytes = Needed;
    Info->NodeType = HubDevice;
    Info->BusAddress = Hub->m_Parent.HubTopologyAddress;

    HubMakeUsb2StyleDescriptor(Hub, &Descriptor);
    Info->HubDeviceInfo.HubDescriptor = Descriptor;
    Info->HubDeviceInfo.HubNumber = Hub->m_HubNumber;
    Info->HubDeviceInfo.HubIsSelfPowered = (Hub->m_MaxPortPower != 100);
    Info->HubDeviceInfo.HubIsRootHub = Hub->IsRootHub();
    Info->HubDeviceInfo.NumberOfHubPorts = PortCount;

    /* Entries are numbered from 0 while ports start at 1: entry 0 stays empty and the last port is left out */
    Entry = (PUSB_HUB_PORT_INFORMATION)HUB_WMI_FIELD(Info, HubDeviceInfo.PortInfo);
    for (Index = 0; Index < PortCount; Index++, Entry++)
    {
        Entry->PortNumber = Index;
        Entry->ConnectionIndex = Index;

        Port = Hub->FindPort(Index);
        if (Port == NULL)
            continue;

        Entry->ConnectionStatus = HubWmiPortEntry(Hub, Port, Index, &Address);
        Entry->DeviceAddress = Address;
    }

    return STATUS_SUCCESS;
}

/* Node information of a child PDO */

/** Copies up to Size bytes of a device property; a failed query leaves the field zero. */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubWmiCopyProperty(
    _In_ WDFDEVICE Device,
    _In_ DEVICE_REGISTRY_PROPERTY Property,
    _Out_writes_bytes_(Size) PUCHAR Destination,
    _In_ ULONG Size)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFMEMORY Memory;
    size_t Length;
    PVOID Data;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Device;

    if (!NT_SUCCESS(WdfDeviceAllocAndQueryProperty(Device, Property, NonPagedPool, &Attributes, &Memory)))
        return;

    Data = WdfMemoryGetBuffer(Memory, &Length);
    RtlCopyMemory(Destination, Data, min(Length, (size_t)Size));
    WdfObjectDelete(Memory);
}

/** Endpoints of the current configuration, listed only when the first interface owns all of them. */
static
VOID
NTAPI
HubWmiCopyPipes(
    _In_ HubConfiguration* Config,
    _Out_ PUSB_PIPE_INFO Pipes)
{
    HubInterface* Interface;
    ULONG Index;

    if (IsListEmpty(&Config->Interfaces))
        return;

    Interface = CONTAINING_RECORD(Config->Interfaces.Flink, HubInterface, Link);
    if (Interface->PipeCount != Config->EndpointCount)
        return;

    for (Index = 0; Index < Interface->PipeCount; Index++)
        Pipes[Index].EndpointDescriptor = *Interface->Pipes[Index].Descriptor;
}

static
NTSTATUS
NTAPI
HubWmiQueryDeviceNode(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG OutBufferSize,
    _Out_writes_bytes_to_(OutBufferSize, *BufferUsed) PVOID OutBuffer,
    _Out_ PULONG BufferUsed)
{
    WDFDEVICE Device = WdfWmiInstanceGetDevice(WmiInstance);
    HubPdo* Pdo = HubPdo::FromDevice(Device);
    PUSB_DEVICE_NODE_INFO Info = (PUSB_DEVICE_NODE_INFO)OutBuffer;
    HubChild* Child;
    HubFdo* Hub;
    UCHAR Depth;
    ULONG Length;

    *BufferUsed = sizeof(USB_DEVICE_NODE_INFO);
    if (OutBufferSize < sizeof(USB_DEVICE_NODE_INFO))
        return STATUS_BUFFER_TOO_SMALL;

    Child = Pdo->m_Child;
    if (Child == NULL)
    {
        DPRINT1("PDO %p WMI node query without a device\n", Pdo);
        *BufferUsed = 0;
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    Hub = Child->m_Hub;
    RtlZeroMemory(OutBuffer, OutBufferSize);

    {
        HubSpinLockGuard Guard(&Child->m_ConfigLock);
        HubConfiguration* Config = Child->m_CurrentConfig;

        if (Child->HasState(ChildState::ConfigurationValid) && Config != NULL)
        {
            /* With no endpoints this is less than the unconfigured size */
            Length = FIELD_OFFSET(USB_DEVICE_NODE_INFO, UsbDeviceInfo.PipeList) +
                     Config->EndpointCount * sizeof(USB_PIPE_INFO);
            *BufferUsed = Length;
            if (Length > OutBufferSize)
                return STATUS_BUFFER_TOO_SMALL;

            Info->UsbDeviceInfo.CurrentConfigurationValue = Config->Descriptor.bConfigurationValue;
            Info->UsbDeviceInfo.NumberOfOpenPipes = Config->EndpointCount;
            HubWmiCopyPipes(Config, (PUSB_PIPE_INFO)HUB_WMI_FIELD(Info, UsbDeviceInfo.PipeList));
        }
    }

    Info->Sig = USB_NODE_INFO_SIG;
    Info->LengthInBytes = *BufferUsed;
    Info->NodeType = UsbDevice;

    Info->UsbDeviceInfo.DeviceDescriptor = Child->m_DeviceDescriptor;
    Info->UsbDeviceInfo.Speed = Child->Speed();
    Info->UsbDeviceInfo.PortNumber = Pdo->m_PortNumber;
    Info->UsbDeviceInfo.ConnectionIndex = Pdo->m_PortNumber;
    Info->UsbDeviceInfo.ConnectionStatus = Child->m_Port->m_ConnectionStatus;

    /* The device sits one tier below its hub */
    Info->BusAddress = Hub->m_Parent.HubTopologyAddress;
    Depth = Hub->m_Parent.HubDepth;
    if (Depth == 0)
        Info->BusAddress.RootHubPortNumber = Pdo->m_PortNumber;
    else if (Depth <= RTL_NUMBER_OF(Info->BusAddress.HubPortNumber))
        Info->BusAddress.HubPortNumber[Depth - 1] = Pdo->m_PortNumber;

    /* Truncated strings keep no NUL */
    HubWmiCopyProperty(Device,
                       DevicePropertyDeviceDescription,
                       HUB_WMI_FIELD(Info, DeviceDescription),
                       sizeof(Info->DeviceDescription));
    HubWmiCopyProperty(Device,
                       DevicePropertyHardwareID,
                       HUB_WMI_FIELD(Info, UsbDeviceInfo.PnpHardwareId),
                       sizeof(Info->UsbDeviceInfo.PnpHardwareId));
    HubWmiCopyProperty(Device,
                       DevicePropertyCompatibleIDs,
                       HUB_WMI_FIELD(Info, UsbDeviceInfo.PnpCompatibleId),
                       sizeof(Info->UsbDeviceInfo.PnpCompatibleId));

    if (Child->m_SerialNumber != NULL)
    {
        RtlCopyMemory(HUB_WMI_FIELD(Info, UsbDeviceInfo.SerialNumberId),
                      Child->m_SerialNumber,
                      min(Child->m_SerialNumberLength, sizeof(Info->UsbDeviceInfo.SerialNumberId)));
    }

    if (Child->m_FriendlyName != NULL)
    {
        RtlCopyMemory(HUB_WMI_FIELD(Info, UsbDeviceInfo.PnpDeviceDescription),
                      Child->m_FriendlyName,
                      min((ULONG)Child->m_FriendlyNameLength, sizeof(Info->UsbDeviceInfo.PnpDeviceDescription)));
    }

    return STATUS_SUCCESS;
}

/* GUID_USB_WMI_STD_DATA methods */

/** Accepts a reset only while the port machine waits for one after the over current popup. */
static
NTSTATUS
NTAPI
HubWmiResetOverCurrent(
    _In_ HubFdo* Hub,
    _In_ PUSB_CONNECTION_NOTIFICATION Request)
{
    HubPort* Port = Hub->FindPort(Request->ConnectionNumber);
    LONG Previous;

    if (Port == NULL)
    {
        DPRINT1("Hub %p over current reset for invalid port %lu\n", Hub, Request->ConnectionNumber);
        return STATUS_INVALID_PARAMETER;
    }

    Previous = InterlockedAnd(&Port->m_Flags, ~(LONG)PortFlag::OverCurrentResetArmed);
    if (Previous & (LONG)PortFlag::OverCurrentResetArmed)
    {
        DPRINT("Hub %p port %u over current reset requested by the user\n", Hub, Port->Number());
        Port->Post(PortEvent::UserOverCurrentReset);
    }

    return STATUS_SUCCESS;
}

/** Only the controller name length is real; both bandwidth fields read 0. */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubWmiBusInfo(
    _In_ HubFdo* Hub,
    _In_ ULONG OutBufferSize,
    _Inout_ PUSB_BUS_NOTIFICATION Notification,
    _Out_ PULONG BufferUsed)
{
    USB_HUB_NAME Name;
    NTSTATUS Status;

    if (OutBufferSize < sizeof(*Notification))
    {
        DPRINT1("Hub %p WMI bus info buffer of %lu bytes too small\n", Hub, OutBufferSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    Name.ActualLength = 0;
    Name.HubName[0] = UNICODE_NULL;

    Status = HubQueryControllerName(Hub, &Name, sizeof(Name));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p: controller name for WMI bus info failed 0x%lx\n", Hub, Status);
        return Status;
    }

    Notification->TotalBandwidth = 0;
    Notification->ConsumedBandwidth = 0;
    Notification->ControllerNameLength = Name.ActualLength;
    *BufferUsed = sizeof(*Notification);
    return STATUS_SUCCESS;
}

static
BOOLEAN
NTAPI
HubWmiAcquireSizeValid(
    _In_ PUSB_ACQUIRE_INFO Request,
    _In_ ULONG OutBufferSize)
{
    if (OutBufferSize >= HUB_WMI_MIN_ACQUIRE_SIZE &&
        Request->TotalSize >= HUB_WMI_MIN_ACQUIRE_SIZE &&
        Request->TotalSize <= OutBufferSize)
    {
        return TRUE;
    }

    DPRINT1("WMI name request size %lu does not fit buffer of %lu bytes\n",
            (OutBufferSize >= HUB_WMI_MIN_ACQUIRE_SIZE) ? Request->TotalSize : 0, OutBufferSize);
    return FALSE;
}

/** A short TotalSize truncates the name without saying so. */
static
NTSTATUS
NTAPI
HubWmiHubNameRequest(
    _In_ HubFdo* Hub,
    _In_ ULONG OutBufferSize,
    _Inout_ PUSB_ACQUIRE_INFO Request,
    _Out_ PULONG BufferUsed)
{
    PUCHAR Text = (PUCHAR)Request + FIELD_OFFSET(USB_ACQUIRE_INFO, Buffer);
    ULONG Room;
    ULONG NameLength;
    PCWSTR Name;
    ULONG Copy;

    if (!HubWmiAcquireSizeValid(Request, OutBufferSize))
        return STATUS_BUFFER_TOO_SMALL;

    Room = Request->TotalSize - FIELD_OFFSET(USB_ACQUIRE_INFO, Buffer);
    Name = HubWmiHubName(Hub, &NameLength);
    Copy = min(Room, NameLength);

    RtlZeroMemory(Text, Room);
    if (Copy != 0)
        RtlCopyMemory(Text, Name, Copy);

    *BufferUsed = FIELD_OFFSET(USB_ACQUIRE_INFO, Buffer) + Copy;
    return STATUS_SUCCESS;
}

/** The answer overlays a USB_HUB_NAME on TotalSize, so TotalSize comes back as the name length + 4. */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubWmiControllerNameRequest(
    _In_ HubFdo* Hub,
    _In_ ULONG OutBufferSize,
    _Inout_ PUSB_ACQUIRE_INFO Request,
    _Out_ PULONG BufferUsed)
{
    PUSB_HUB_NAME Name = (PUSB_HUB_NAME)((PUCHAR)Request + FIELD_OFFSET(USB_ACQUIRE_INFO, TotalSize));
    ULONG Total;
    NTSTATUS Status;

    if (!HubWmiAcquireSizeValid(Request, OutBufferSize))
        return STATUS_BUFFER_TOO_SMALL;

    Total = Request->TotalSize;
    Request->TotalSize = Total - sizeof(ULONG);

    Status = HubQueryControllerName(Hub, Name, Total - sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p: controller name for WMI failed 0x%lx\n", Hub, Status);
        return Status;
    }

    Name->ActualLength += sizeof(ULONG);
    *BufferUsed = Total;
    return STATUS_SUCCESS;
}

/** Dispatches on the request's notification type; MethodId and InBufferSize are ignored. */
static
NTSTATUS
NTAPI
HubWmiExecuteMethod(
    _In_ WDFWMIINSTANCE WmiInstance,
    _In_ ULONG MethodId,
    _In_ ULONG InBufferSize,
    _In_ ULONG OutBufferSize,
    _Inout_ PVOID Buffer,
    _Out_ PULONG BufferUsed)
{
    HubFdo* Hub;

    UNREFERENCED_PARAMETER(MethodId);
    UNREFERENCED_PARAMETER(InBufferSize);

    *BufferUsed = 0;
    if (OutBufferSize < sizeof(USB_NOTIFICATION))
    {
        DPRINT1("WMI method buffer of %lu bytes too small\n", OutBufferSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    Hub = HubFdo::FromDevice(WdfWmiInstanceGetDevice(WmiInstance));

    switch (((PUSB_NOTIFICATION)Buffer)->NotificationType)
    {
        case ResetOvercurrent:
            if (OutBufferSize < sizeof(USB_CONNECTION_NOTIFICATION))
            {
                DPRINT1("Hub %p over current reset buffer of %lu bytes too small\n", Hub, OutBufferSize);
                return STATUS_BUFFER_TOO_SMALL;
            }
            return HubWmiResetOverCurrent(Hub, (PUSB_CONNECTION_NOTIFICATION)Buffer);

        case AcquireBusInfo:
            return HubWmiBusInfo(Hub, OutBufferSize, (PUSB_BUS_NOTIFICATION)Buffer, BufferUsed);

        case AcquireHubName:
            return HubWmiHubNameRequest(Hub, OutBufferSize, (PUSB_ACQUIRE_INFO)Buffer, BufferUsed);

        case AcquireControllerName:
            return HubWmiControllerNameRequest(Hub, OutBufferSize, (PUSB_ACQUIRE_INFO)Buffer, BufferUsed);

        default:
            return STATUS_SUCCESS;
    }
}

/* Registration */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiRegister(
    _In_ HubFdo* Hub)
{
    WDFWMIINSTANCE Instance;
    NTSTATUS Status;

    /* Runs on every start; earlier instances stay registered */
    Status = HubWmiCreateInstance(Hub->m_Device,
                                  &GUID_USB_WMI_STD_DATA,
                                  0,
                                  NULL,
                                  HubWmiExecuteMethod,
                                  WDF_NO_OBJECT_ATTRIBUTES,
                                  &Instance);
    if (NT_SUCCESS(Status))
        Hub->m_WmiInstance = Instance;
    else
        DPRINT1("Hub %p: WMI notification provider failed 0x%lx\n", Hub, Status);

    Status = HubWmiCreateInstance(Hub->m_Device,
                                  &GUID_USB_WMI_NODE_INFO,
                                  0,
                                  HubWmiQueryHubNode,
                                  NULL,
                                  WDF_NO_OBJECT_ATTRIBUTES,
                                  NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p: WMI node info provider failed 0x%lx\n", Hub, Status);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiRegisterDevice(
    _In_ WDFDEVICE Pdo)
{
    NTSTATUS Status;

    Status = HubWmiCreateInstance(Pdo,
                                  &GUID_USB_WMI_NODE_INFO,
                                  0,
                                  HubWmiQueryDeviceNode,
                                  NULL,
                                  WDF_NO_OBJECT_ATTRIBUTES,
                                  NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("PDO %p: WMI node info provider failed 0x%lx\n", Pdo, Status);
}

/** Clears the driver wide handle along with its instance. */
static
VOID
NTAPI
HubWmiEvtSurpriseRemovalCleanup(
    _In_ WDFOBJECT Object)
{
    HubSpinLockGuard Guard(&HubWmiSurpriseLock);

    if (HubDriver.SurpriseRemovalWmi == (WDFWMIINSTANCE)Object)
        HubDriver.SurpriseRemovalWmi = NULL;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiRegisterBootSurpriseRemoval(
    _In_ WDFDEVICE Pdo)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFWMIINSTANCE Instance;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.EvtCleanupCallback = HubWmiEvtSurpriseRemovalCleanup;

    Status = HubWmiCreateInstance(Pdo,
                                  &HubWmiSurpriseRemovalGuid,
                                  WdfWmiProviderEventOnly,
                                  NULL,
                                  NULL,
                                  &Attributes,
                                  &Instance);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PDO %p: WMI surprise removal provider failed 0x%lx\n", Pdo, Status);
        return;
    }

    /* One for the whole driver; the latest boot device wins */
    HubSpinLockGuard Guard(&HubWmiSurpriseLock);
    HubDriver.SurpriseRemovalWmi = Instance;
}

/* Notifications */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWmiNotifyBootSurpriseRemoval(VOID)
{
    WDFWMIINSTANCE Instance;
    NTSTATUS Status;

    {
        HubSpinLockGuard Guard(&HubWmiSurpriseLock);

        /* Nothing to send when no boot device registered one */
        Instance = HubDriver.SurpriseRemovalWmi;
        if (Instance == NULL)
            return;

        WdfObjectReference(Instance);
    }

    if (WdfWmiProviderIsEnabled(WdfWmiInstanceGetProvider(Instance), WdfWmiEventControl))
    {
        Status = WdfWmiInstanceFireEvent(Instance, 0, NULL);
        if (!NT_SUCCESS(Status))
            DPRINT1("WMI surprise removal event failed 0x%lx\n", Status);
    }

    WdfObjectDereference(Instance);
}

VOID
NTAPI
HubWmiNotifyOverCurrent(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber)
{
    HubWmiFirePortEvent(Hub, PortNumber, OverCurrent);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyEnumerationFailure(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber)
{
    HubWmiFirePortEvent(Hub, PortNumber, EnumerationFailure);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyHubNestedTooDeeply(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber)
{
    HubWmiFirePortEvent(Hub, PortNumber, HubNestedTooDeeply);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyInsufficientPower(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber)
{
    HubWmiFirePortEvent(Hub, PortNumber, InsufficentPower);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubWmiNotifyInsufficientBandwidth(
    _In_ HubFdo* Hub,
    _In_ ULONG PortNumber)
{
    HubWmiFirePortEvent(Hub, PortNumber, InsufficentBandwidth);
}
