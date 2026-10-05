/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Configuration and interface records behind client select requests
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "desccheck.h"
#include "devhcd.h"

#define NDEBUG
#include <debug.h>

/* Every endpoint reports at most this interval to the client, whatever its type */
#define CLIENT_INTERVAL_CAP     6

/* Devices below USB 2.5 pack extra transactions into wMaxPacketSize bits 12:11 */
#define BCD_USB_25              0x0250

/* Isochronous companions on SuperSpeedPlus have an extra descriptor */
#define COMPANION_LENGTH        6
#define SSP_COMPANION_LENGTH    8

/** The client URB of the request the device machine is serving. */
static
PURB
NTAPI
HubChildClientUrb(
    _In_ HubChild* Child)
{
    WDF_REQUEST_PARAMETERS Params;

    WDF_REQUEST_PARAMETERS_INIT(&Params);
    WdfRequestGetParameters(Child->m_ClientRequest, &Params);
    return (PURB)Params.Parameters.Others.Arg1;
}

/* The registry filter is a ULONG count followed by (interface, alternate) byte pairs */
static
BOOLEAN
NTAPI
HubChildSettingFiltered(
    _In_ HubChild* Child,
    _In_ UCHAR InterfaceNumber,
    _In_ UCHAR AlternateSetting)
{
    PULONG Filter = (PULONG)Child->m_AlternateSettingFilter;
    PUCHAR Pairs;
    ULONG Index;

    if (Filter == NULL)
        return FALSE;

    Pairs = (PUCHAR)(Filter + 1);
    for (Index = 0; Index < *Filter; Index++)
    {
        if (Pairs[Index * 2] == InterfaceNumber && Pairs[Index * 2 + 1] == AlternateSetting)
            return TRUE;
    }

    return FALSE;
}

/* UCX endpoint arrays */

static
VOID
NTAPI
HubChildFreeEndpointArrays(
    _In_ HubChild* Child)
{
    UCXENDPOINT** Arrays[] = { &Child->m_EndpointsToEnable,
                               &Child->m_EndpointsUnchanged,
                               &Child->m_EndpointsToDisable };
    ULONG Index;

    Child->m_EndpointArrayCapacity = 0;

    for (Index = 0; Index < RTL_NUMBER_OF(Arrays); Index++)
    {
        if (*Arrays[Index] != NULL)
        {
            ExFreePoolWithTag(*Arrays[Index], HUB_TAG_DEVICE);
            *Arrays[Index] = NULL;
        }
    }
}

/* All three arrays share one capacity and only grow; the old ones are freed after every new allocation succeeds */
static
NTSTATUS
NTAPI
HubChildSizeEndpointArrays(
    _In_ HubChild* Child,
    _In_ ULONG Count)
{
    UCXENDPOINT* Fresh[3];
    ULONG Index;

    Child->m_EnableCount = 0;
    Child->m_DisableCount = 0;
    Child->m_UnchangedCount = 0;

    if (Count <= Child->m_EndpointArrayCapacity)
        return STATUS_SUCCESS;

    for (Index = 0; Index < RTL_NUMBER_OF(Fresh); Index++)
    {
        Fresh[Index] = (UCXENDPOINT*)ExAllocatePoolWithTag(NonPagedPool,
                                                           Count * sizeof(UCXENDPOINT),
                                                           HUB_TAG_DEVICE);
        if (Fresh[Index] == NULL)
        {
            DPRINT1("Device %p no memory for %lu endpoint handles\n", Child, Count);
            while (Index-- != 0)
                ExFreePoolWithTag(Fresh[Index], HUB_TAG_DEVICE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    HubChildFreeEndpointArrays(Child);
    Child->m_EndpointsToEnable = Fresh[0];
    Child->m_EndpointsUnchanged = Fresh[1];
    Child->m_EndpointsToDisable = Fresh[2];
    Child->m_EndpointArrayCapacity = Count;
    return STATUS_SUCCESS;
}

VOID
NTAPI
HubChildCollectForDisable(
    _In_ HubChild* Child,
    _In_ HubInterface* Interface)
{
    HubPipe* Pipe;
    ULONG Index;

    for (Index = 0; Index < Interface->PipeCount; Index++)
    {
        Pipe = &Interface->Pipes[Index];

        switch (Pipe->State)
        {
            case PipeState::Enabled:
                Pipe->State = PipeState::PendingDisable;
                Child->m_EndpointsToDisable[Child->m_DisableCount++] = Pipe->Endpoint;
                break;

            case PipeState::Disabled:
                Child->m_EndpointsUnchanged[Child->m_UnchangedCount++] = Pipe->Endpoint;
                break;

            case PipeState::NotCreated:
                break;

            default:
                DPRINT1("Device %p pipe %lu in unexpected state %u\n", Child, Index, (UCHAR)Pipe->State);
                ASSERT(FALSE);
                break;
        }
    }
}

/* Records */

static
NTSTATUS
NTAPI
HubAllocateConfiguration(
    _In_ PUSB_CONFIGURATION_DESCRIPTOR Source,
    _Out_ HubConfiguration** Result)
{
    HubConfiguration* Config;
    WDFMEMORY Memory;
    ULONG Size;
    NTSTATUS Status;

    Size = FIELD_OFFSET(HubConfiguration, Descriptor) + Source->wTotalLength;
    Size = max(Size, (ULONG)sizeof(*Config));

    Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
                             NonPagedPool,
                             HUB_TAG_DEVICE,
                             Size,
                             &Memory,
                             (PVOID*)&Config);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Configuration record of %lu bytes not allocated 0x%lx\n", Size, Status);
        *Result = NULL;
        return Status;
    }

    RtlZeroMemory(Config, Size);
    Config->Memory = Memory;
    InitializeListHead(&Config->Interfaces);
    RtlCopyMemory(&Config->Descriptor, Source, Source->wTotalLength);

    *Result = Config;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HubAllocateInterface(
    _In_ PUSB_INTERFACE_DESCRIPTOR Descriptor,
    _In_ BOOLEAN HasAlternates,
    _Out_ HubInterface** Result)
{
    HubInterface* Interface;
    WDFMEMORY Memory;
    ULONG Size;
    NTSTATUS Status;

    Size = FIELD_OFFSET(HubInterface, Pipes) + Descriptor->bNumEndpoints * sizeof(HubPipe);
    Size = max(Size, (ULONG)sizeof(*Interface));

    Status = WdfMemoryCreate(WDF_NO_OBJECT_ATTRIBUTES,
                             NonPagedPool,
                             HUB_TAG_DEVICE,
                             Size,
                             &Memory,
                             (PVOID*)&Interface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Interface %u record not allocated 0x%lx\n", Descriptor->bInterfaceNumber, Status);
        *Result = NULL;
        return Status;
    }

    RtlZeroMemory(Interface, Size);
    Interface->Memory = Memory;
    Interface->Descriptor = Descriptor;
    Interface->PipeCount = Descriptor->bNumEndpoints;
    Interface->HasAlternateSettings = HasAlternates;
    Interface->NeedsSetInterface = HasAlternates;

    *Result = Interface;
    return STATUS_SUCCESS;
}

/* A half built configuration owns no UCX endpoints, so only memory goes */
static
VOID
NTAPI
HubFreeConfiguration(
    _In_ HubChild* Child,
    _In_ HubConfiguration* Config)
{
    PLIST_ENTRY Entry;

    while (!IsListEmpty(&Config->Interfaces))
    {
        Entry = RemoveHeadList(&Config->Interfaces);
        HubChildDeleteInterface(Child, CONTAINING_RECORD(Entry, HubInterface, Link));
    }

    WdfObjectDelete(Config->Memory);
}

/* Pipes */

/* TRUE when the descriptor type byte at Walk lies inside the buffer */
FORCEINLINE
BOOLEAN
NTAPI
HubTypeFits(
    _In_ PUCHAR Walk,
    _In_ PUCHAR End)
{
    return Walk + 1 < End;
}

/* Records the optional companions after an endpoint; a bad one is just skipped */
static
VOID
NTAPI
HubPickCompanions(
    _In_ HubPipe* Pipe,
    _Inout_ PUSBD_PIPE_INFORMATION Client,
    _In_ PUCHAR Walk,
    _In_ PUCHAR End)
{
    PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion;
    PUSB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR SspCompanion;

    if (!HubTypeFits(Walk, End))
        return;

    Companion = (PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR)Walk;
    if (Companion->bDescriptorType != USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR_TYPE ||
        Companion->bLength == 0 ||
        Walk + COMPANION_LENGTH > End)
    {
        return;
    }

    Pipe->Companion = Companion;

    if ((Pipe->Descriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK) != USB_ENDPOINT_TYPE_ISOCHRONOUS)
        return;

    if (!Pipe->ZeroBandwidth)
        Client->MaximumPacketSize = Companion->wBytesPerInterval;

    if (!Companion->bmAttributes.Isochronous.SspCompanion)
        return;

    Walk += Companion->bLength;
    if (!HubTypeFits(Walk, End))
        return;

    SspCompanion = (PUSB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR)Walk;
    if (SspCompanion->bDescriptorType == USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR_TYPE &&
        SspCompanion->bLength != 0 &&
        Walk + SSP_COMPANION_LENGTH <= End)
    {
        Pipe->IsochCompanion = SspCompanion;
    }
}

/* Companion descriptors are not consumed here; the next pipe's skip loop steps over them */
static
NTSTATUS
NTAPI
HubChildInitPipes(
    _In_ HubChild* Child,
    _Inout_ PUSBD_INTERFACE_INFORMATION Info,
    _Inout_ HubInterface* Interface,
    _In_ PUCHAR Walk,
    _In_ PUCHAR End)
{
    PUSBD_PIPE_INFORMATION Client;
    PUSB_ENDPOINT_DESCRIPTOR Endpoint;
    HubPipe* Pipe;
    USHORT Packet;
    ULONG Index;

    for (Index = 0; Index < Info->NumberOfPipes; Index++)
    {
        Client = &Info->Pipes[Index];
        Pipe = &Interface->Pipes[Index];

        if (Walk >= End)
        {
            DPRINT1("Device %p interface %u has fewer than %lu endpoint descriptors\n",
                    Child,
                    Info->InterfaceNumber,
                    Info->NumberOfPipes);
            return STATUS_UNSUCCESSFUL;
        }

        Pipe->PipeFlags = Client->PipeFlags;
        Pipe->State = PipeState::NotCreated;

        if (!HubTypeFits(Walk, End))
        {
            DPRINT1("Device %p interface %u descriptor runs past the configuration\n", Child, Info->InterfaceNumber);
            return STATUS_UNSUCCESSFUL;
        }

        while (((PUSB_COMMON_DESCRIPTOR)Walk)->bDescriptorType != USB_ENDPOINT_DESCRIPTOR_TYPE)
        {
            if (Walk[0] == 0)
            {
                DPRINT1("Device %p interface %u has a zero length descriptor\n", Child, Info->InterfaceNumber);
                return STATUS_UNSUCCESSFUL;
            }

            Walk += Walk[0];
            if (!HubTypeFits(Walk, End))
            {
                DPRINT1("Device %p interface %u has fewer than %lu endpoint descriptors\n",
                        Child,
                        Info->InterfaceNumber,
                        Info->NumberOfPipes);
                return STATUS_UNSUCCESSFUL;
            }
        }

        if (Walk + sizeof(USB_ENDPOINT_DESCRIPTOR) > End)
        {
            DPRINT1("Device %p interface %u endpoint descriptor runs past the configuration\n",
                    Child,
                    Info->InterfaceNumber);
            return STATUS_UNSUCCESSFUL;
        }

        Endpoint = (PUSB_ENDPOINT_DESCRIPTOR)Walk;

        /* The cached copy is what UCX sees, so the override goes there */
        if (Pipe->PipeFlags & USBD_PF_CHANGE_MAX_PACKET)
            Endpoint->wMaxPacketSize = Client->MaximumPacketSize;

        Pipe->Descriptor = Endpoint;
        Pipe->BytesToEnd = (ULONG)(End - Walk);

        Packet = Endpoint->wMaxPacketSize;
        if (Child->m_DeviceDescriptor.bcdUSB < BCD_USB_25)
            Packet = (USHORT)((Packet & 0x7FF) * (((Packet >> 11) & 3) + 1));

        Client->MaximumPacketSize = Packet;
        Client->Interval = min(Endpoint->bInterval, CLIENT_INTERVAL_CAP);
        Client->EndpointAddress = Endpoint->bEndpointAddress;
        Client->PipeType = (USBD_PIPE_TYPE)(Endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK);

        if (Client->MaximumPacketSize == 0)
            Pipe->ZeroBandwidth = TRUE;

        if (Endpoint->bLength == 0)
        {
            DPRINT1("Device %p endpoint 0x%x has a zero length\n", Child, Endpoint->bEndpointAddress);
            return STATUS_UNSUCCESSFUL;
        }

        Walk += Endpoint->bLength;
        HubPickCompanions(Pipe, Client, Walk, End);
    }

    return STATUS_SUCCESS;
}

/* Sanity checks */

static
BOOLEAN
NTAPI
HubInterfaceHasEndpointZero(
    _In_ HubInterface* Interface)
{
    ULONG Index;

    for (Index = 0; Index < Interface->PipeCount; Index++)
    {
        if ((Interface->Pipes[Index].Descriptor->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK) == 0)
            return TRUE;
    }

    return FALSE;
}

static
BOOLEAN
NTAPI
HubConfigHasEndpointZero(
    _In_ HubConfiguration* Config)
{
    PLIST_ENTRY Entry;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        if (HubInterfaceHasEndpointZero(CONTAINING_RECORD(Entry, HubInterface, Link)))
            return TRUE;
    }

    return FALSE;
}

/*
 * One bit per endpoint number for control, IN and OUT. A control endpoint
 * only conflicts with the control map, so the check depends on order.
 */
static
NTSTATUS
NTAPI
HubConfigCheckDuplicates(
    _In_ HubConfiguration* Config)
{
    USHORT Seen[3] = { 0, 0, 0 };
    PUSB_ENDPOINT_DESCRIPTOR Endpoint;
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    USHORT Bit;
    ULONG Map;
    ULONG Index;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            if (Interface->Pipes[Index].ZeroBandwidth)
                continue;

            Endpoint = Interface->Pipes[Index].Descriptor;
            Bit = (USHORT)(1 << (Endpoint->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK));

            if ((Endpoint->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_CONTROL)
                Map = 0;
            else if (USB_ENDPOINT_DIRECTION_IN(Endpoint->bEndpointAddress))
                Map = 1;
            else
                Map = 2;

            if ((Seen[0] | Seen[Map]) & Bit)
            {
                DPRINT1("Configuration %p uses endpoint 0x%x twice\n", Config, Endpoint->bEndpointAddress);
                return STATUS_UNSUCCESSFUL;
            }

            Seen[Map] |= Bit;
        }
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HubInterfacesShareEndpoints(
    _In_ HubInterface* First,
    _In_ HubInterface* Second)
{
    PUSB_ENDPOINT_DESCRIPTOR A;
    PUSB_ENDPOINT_DESCRIPTOR B;
    ULONG I;
    ULONG J;

    for (I = 0; I < First->PipeCount; I++)
    {
        A = First->Pipes[I].Descriptor;

        for (J = 0; J < Second->PipeCount; J++)
        {
            B = Second->Pipes[J].Descriptor;

            if (A->bEndpointAddress == B->bEndpointAddress &&
                A->wMaxPacketSize != 0 &&
                B->wMaxPacketSize != 0)
            {
                DPRINT1("Endpoint 0x%x shared by interface %u alternate %u and interface %u alternate %u\n",
                        A->bEndpointAddress,
                        First->Descriptor->bInterfaceNumber,
                        First->Descriptor->bAlternateSetting,
                        Second->Descriptor->bInterfaceNumber,
                        Second->Descriptor->bAlternateSetting);
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    return STATUS_SUCCESS;
}

static
VOID
NTAPI
HubChildRecordFailure(
    _In_ HubChild* Child,
    _In_ NTSTATUS Status)
{
    Child->m_LastNtStatus = Status;
    if (Child->m_LastUsbdStatus == 0)
        Child->m_LastUsbdStatus = HubNtStatusToUsbd(Status);
}

/* SELECT_CONFIGURATION */

/* On failure the old configuration is restored; interface handles already written to the URB stay stale */
BOOLEAN
DeviceMachine::PrepareConfigLists()
{
    HubChild* Child = m_Device;
    PURB Urb = HubChildClientUrb(Child);
    struct _URB_SELECT_CONFIGURATION* Select = &Urb->UrbSelectConfiguration;
    HubConfiguration* Config = NULL;
    PUSBD_INTERFACE_INFORMATION Info;
    PUSBD_INTERFACE_INFORMATION Next;
    PUSB_INTERFACE_DESCRIPTOR Descriptor;
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    PUCHAR UrbEnd;
    PUCHAR ConfigEnd;
    BOOLEAN HasAlternates;
    ULONG Total = 0;
    NTSTATUS Status;
    KIRQL Irql;

    Child->m_EnableCount = 0;

    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
    Child->ClearState(ChildState::ConfigurationValid);
    Child->m_OldConfig = Child->m_CurrentConfig;
    Child->m_CurrentConfig = NULL;
    Child->ClearState(ChildState::AltSettingFiltered);
    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

    Status = HubAllocateConfiguration(Select->ConfigurationDescriptor, &Config);
    if (!NT_SUCCESS(Status))
        goto Failed;

    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
    Child->m_CurrentConfig = Config;
    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

    ConfigEnd = (PUCHAR)&Config->Descriptor + Config->Descriptor.wTotalLength;
    UrbEnd = (PUCHAR)Urb + Urb->UrbHeader.Length;

    for (Info = &Select->Interface; (PUCHAR)Info + sizeof(USHORT) < UrbEnd; Info = Next)
    {
        Descriptor = HubDescFindInterface(&Config->Descriptor,
                                          &Config->Descriptor,
                                          Info->InterfaceNumber,
                                          Info->AlternateSetting,
                                          -1,
                                          -1,
                                          -1,
                                          &HasAlternates);
        if (Descriptor == NULL || Info->Length == 0)
        {
            DPRINT1("Device %p SELECT_CONFIGURATION names missing interface %u alternate %u\n",
                    Child,
                    Info->InterfaceNumber,
                    Info->AlternateSetting);
            Child->m_LastUsbdStatus = USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR;
            Status = STATUS_UNSUCCESSFUL;
            goto Failed;
        }

        if (HubChildSettingFiltered(Child, Info->InterfaceNumber, Info->AlternateSetting))
        {
            DPRINT("Device %p interface %u alternate %u is filtered\n",
                   Child,
                   Info->InterfaceNumber,
                   Info->AlternateSetting);
            Child->SetState(ChildState::AltSettingFiltered);
        }

        Info->NumberOfPipes = Descriptor->bNumEndpoints;
        Total += Descriptor->bNumEndpoints;
        Next = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)Info + Info->Length);

        Status = HubAllocateInterface(Descriptor, HasAlternates, &Interface);
        if (!NT_SUCCESS(Status))
            goto Failed;

        Info->InterfaceHandle = Interface;

        KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
        InsertTailList(&Config->Interfaces, &Interface->Link);
        KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

        Info->Class = Descriptor->bInterfaceClass;
        Info->SubClass = Descriptor->bInterfaceSubClass;
        Info->Protocol = Descriptor->bInterfaceProtocol;

        Status = HubChildInitPipes(Child, Info, Interface, (PUCHAR)Descriptor + Descriptor->bLength, ConfigEnd);
        if (!NT_SUCCESS(Status))
        {
            Child->m_LastUsbdStatus = USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR;
            goto Failed;
        }
    }

    /* There is never a descriptor for endpoint zero */
    if (HubConfigHasEndpointZero(Config))
    {
        DPRINT1("Device %p configuration has an endpoint zero descriptor\n", Child);
        Child->m_LastUsbdStatus = USBD_STATUS_BAD_ENDPOINT_ADDRESS;
        Status = STATUS_UNSUCCESSFUL;
        goto Failed;
    }

    Status = HubConfigCheckDuplicates(Config);
    if (!NT_SUCCESS(Status))
    {
        Child->m_LastUsbdStatus = USBD_STATUS_INVALID_CONFIGURATION_DESCRIPTOR;
        goto Failed;
    }

    Config->EndpointCount = Total;

    /* The old configuration's pipes are collected into the same arrays, so size for the larger count */
    Status = HubChildSizeEndpointArrays(Child,
                                        (Child->m_OldConfig != NULL) ? max(Total, Child->m_OldConfig->EndpointCount) : Total);
    if (!NT_SUCCESS(Status))
        goto Failed;

    if (Child->m_OldConfig != NULL)
    {
        Child->m_UnchangedCount = 0;
        Child->m_DisableCount = 0;

        for (Entry = Child->m_OldConfig->Interfaces.Flink;
             Entry != &Child->m_OldConfig->Interfaces;
             Entry = Entry->Flink)
        {
            HubChildCollectForDisable(Child, CONTAINING_RECORD(Entry, HubInterface, Link));
        }
    }

    Select->ConfigurationHandle = Config;
    return TRUE;

Failed:
    HubChildRecordFailure(Child, Status);

    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
    Child->m_CurrentConfig = Child->m_OldConfig;
    Child->m_OldConfig = NULL;
    Child->SetState(ChildState::ConfigurationValid);
    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

    if (Config != NULL)
        HubFreeConfiguration(Child, Config);

    Select->ConfigurationHandle = NULL;
    return FALSE;
}

/* SELECT_INTERFACE */

BOOLEAN
DeviceMachine::PrepareInterfaceLists()
{
    HubChild* Child = m_Device;
    PURB Urb = HubChildClientUrb(Child);
    PUSBD_INTERFACE_INFORMATION Info = &Urb->UrbSelectInterface.Interface;
    HubConfiguration* Config = Child->m_CurrentConfig;
    PUSB_INTERFACE_DESCRIPTOR Descriptor = NULL;
    HubInterface* Interface = NULL;
    HubInterface* Old = NULL;
    HubInterface* Other;
    PLIST_ENTRY Entry;
    BOOLEAN HasAlternates = FALSE;
    ULONG Total;
    ULONG Index;
    NTSTATUS Status;
    KIRQL Irql;

    Child->m_NewInterface = NULL;
    Child->m_EnableCount = 0;
    Child->ClearState(ChildState::AltSettingFiltered);

    /* The PDO completion path sets it again whatever happens here */
    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
    Child->ClearState(ChildState::ConfigurationValid);
    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

    if (Config != NULL)
    {
        Descriptor = HubDescFindInterface(&Config->Descriptor,
                                          &Config->Descriptor,
                                          Info->InterfaceNumber,
                                          Info->AlternateSetting,
                                          -1,
                                          -1,
                                          -1,
                                          &HasAlternates);
    }

    if (Descriptor == NULL)
    {
        DPRINT1("Device %p SELECT_INTERFACE names missing interface %u alternate %u\n",
                Child,
                Info->InterfaceNumber,
                Info->AlternateSetting);
        Child->m_LastUsbdStatus = USBD_STATUS_INTERFACE_NOT_FOUND;
        Status = STATUS_UNSUCCESSFUL;
        goto Failed;
    }

    if (HubChildSettingFiltered(Child, Info->InterfaceNumber, Info->AlternateSetting))
    {
        DPRINT("Device %p interface %u alternate %u is filtered\n",
               Child,
               Info->InterfaceNumber,
               Info->AlternateSetting);
        Child->SetState(ChildState::AltSettingFiltered);
    }

    Info->Length = (USHORT)(FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes) +
                            Descriptor->bNumEndpoints * sizeof(USBD_PIPE_INFORMATION));
    Info->Class = 0;
    Info->SubClass = 0;
    Info->Protocol = 0;
    Info->Reserved = 0;
    Info->InterfaceHandle = NULL;
    Info->NumberOfPipes = Descriptor->bNumEndpoints;

    Status = HubAllocateInterface(Descriptor, HasAlternates, &Interface);
    if (!NT_SUCCESS(Status))
        goto Failed;

    Child->m_NextInterface = HasAlternates ? Interface : NULL;

    Info->Class = Descriptor->bInterfaceClass;
    Info->SubClass = Descriptor->bInterfaceSubClass;
    Info->Protocol = Descriptor->bInterfaceProtocol;

    Status = HubChildInitPipes(Child,
                               Info,
                               Interface,
                               (PUCHAR)Descriptor + Descriptor->bLength,
                               (PUCHAR)&Config->Descriptor + Config->Descriptor.wTotalLength);
    if (!NT_SUCCESS(Status))
    {
        Child->m_LastUsbdStatus = USBD_STATUS_INTERFACE_NOT_FOUND;
        goto Failed;
    }

    if (HubInterfaceHasEndpointZero(Interface))
    {
        DPRINT1("Device %p interface %u has an endpoint zero descriptor\n", Child, Info->InterfaceNumber);
        Child->m_LastUsbdStatus = USBD_STATUS_BAD_ENDPOINT_ADDRESS;
        Status = STATUS_UNSUCCESSFUL;
        goto Failed;
    }

    /* The interface being replaced is only marked old once nothing can fail any more */
    Child->m_OldInterface = NULL;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Other = CONTAINING_RECORD(Entry, HubInterface, Link);

        if (Other->Descriptor->bInterfaceNumber == Info->InterfaceNumber)
        {
            Old = Other;
            continue;
        }

        Status = HubInterfacesShareEndpoints(Other, Interface);
        if (!NT_SUCCESS(Status))
            goto Failed;
    }

    Total = Config->EndpointCount + Interface->PipeCount;

    Status = HubChildSizeEndpointArrays(Child, Total);
    if (!NT_SUCCESS(Status))
        goto Failed;

    if (Old != NULL)
        Total -= Old->PipeCount;

    Child->m_UnchangedCount = 0;
    Child->m_DisableCount = 0;

    if (Old != NULL)
    {
        KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
        RemoveEntryList(&Old->Link);
        KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

        Child->m_OldInterface = Old;
        HubChildCollectForDisable(Child, Old);
    }

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Other = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Other->PipeCount; Index++)
            Child->m_EndpointsUnchanged[Child->m_UnchangedCount++] = Other->Pipes[Index].Endpoint;
    }

    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
    InsertTailList(&Config->Interfaces, &Interface->Link);
    Config->EndpointCount = Total;
    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

    Child->m_NewInterface = Interface;
    Info->InterfaceHandle = Interface;
    return TRUE;

Failed:
    HubChildRecordFailure(Child, Status);

    if (Interface != NULL)
    {
        if (Child->m_NextInterface == Interface)
            Child->m_NextInterface = NULL;

        WdfObjectDelete(Interface->Memory);
    }

    Info->InterfaceHandle = (USBD_INTERFACE_HANDLE)(LONG_PTR)-1;
    return FALSE;
}

/* Reset: the controller forgot every endpoint, so all of them are enabled again */
BOOLEAN
DeviceMachine::PrepareListsForReset()
{
    HubChild* Child = m_Device;
    HubConfiguration* Config = Child->m_CurrentConfig;
    HubInterface* Interface;
    HubPipe* Pipe;
    PLIST_ENTRY Entry;
    ULONG Index;

    Child->m_EnableCount = 0;
    Child->m_DisableCount = 0;
    Child->m_UnchangedCount = 0;

    if (Config == NULL)
        return TRUE;

    /* A failed earlier resize can leave the arrays short */
    if (!NT_SUCCESS(HubChildSizeEndpointArrays(Child, Config->EndpointCount)))
        return FALSE;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);
        if (Interface->HasAlternateSettings)
            Interface->NeedsSetInterface = TRUE;

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            Pipe = &Interface->Pipes[Index];

            if (Pipe->ZeroBandwidth)
            {
                Pipe->State = PipeState::Disabled;
                Child->m_EndpointsUnchanged[Child->m_UnchangedCount++] = Pipe->Endpoint;
            }
            else
            {
                Pipe->State = PipeState::PendingEnable;
                Child->m_EndpointsToEnable[Child->m_EnableCount++] = Pipe->Endpoint;
            }
        }
    }

    return TRUE;
}

/* Handles back to the client */

static
USBD_PIPE_HANDLE
NTAPI
HubChildExposePipe(
    _In_ HubChild* Child,
    _Inout_ HubPipe* Pipe)
{
    PUCXHUB_STACK_INTERFACE Stack = &Child->m_Hub->m_Stack;

    Pipe->PipeHandle = Stack->EndpointGetPipeHandle(Pipe->Endpoint);
    if (Pipe->PipeHandle == NULL)
        DPRINT1("Endpoint 0x%x has no pipe handle\n", Pipe->Descriptor->bEndpointAddress);

    Stack->MarkEndpointClientOwned(Pipe->Endpoint);
    return Pipe->PipeHandle;
}

/* The interface list was built in URB order, so both are walked side by side */
VOID
DeviceMachine::SetConfigInfoInRequest()
{
    HubChild* Child = m_Device;
    HubConfiguration* Config = Child->m_CurrentConfig;
    PURB Urb = HubChildClientUrb(Child);
    PUSBD_INTERFACE_INFORMATION Info = &Urb->UrbSelectConfiguration.Interface;
    HubInterface* Interface;
    HubPipe* Pipe;
    PLIST_ENTRY Entry;
    ULONG Index;

    if (Config == NULL)
        return;

    for (Entry = Config->Interfaces.Flink;
         Entry != &Config->Interfaces;
         Entry = Entry->Flink, Info = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)Info + Info->Length))
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            Pipe = &Interface->Pipes[Index];

            Info->Pipes[Index].PipeHandle = HubChildExposePipe(Child, Pipe);
            Info->Pipes[Index].MaximumTransferSize =
                Child->m_Hub->m_Stack.EndpointGetMaxTransferSize(Pipe->Endpoint);
        }
    }
}

/* MaximumTransferSize is left alone on SELECT_INTERFACE */
VOID
DeviceMachine::SetInterfaceInfoInRequest()
{
    HubChild* Child = m_Device;
    HubConfiguration* Config = Child->m_CurrentConfig;
    PURB Urb = HubChildClientUrb(Child);
    PUSBD_INTERFACE_INFORMATION Info = &Urb->UrbSelectInterface.Interface;
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    ULONG Index;

    if (Config == NULL)
        return;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);
        if (Interface->Descriptor->bInterfaceNumber != Info->InterfaceNumber)
            continue;

        for (Index = 0; Index < Interface->PipeCount; Index++)
            Info->Pipes[Index].PipeHandle = HubChildExposePipe(Child, &Interface->Pipes[Index]);

        return;
    }

    DPRINT1("Device %p selected interface %u is not in the configuration\n", Child, Info->InterfaceNumber);
    ASSERT(FALSE);
}
