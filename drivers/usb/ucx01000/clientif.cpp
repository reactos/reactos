/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USBD client handles, XRB allocation and the USBDI bus interface
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

#define UCX_MAX_SELECT_PIPES       32
#define UCX_MAX_SELECT_INTERFACES  255

/* Bandwidth reported when the HCD does not take the question itself */
#define UCX_FAKE_TOTAL_BANDWIDTH   4000000

/* Options USBPORT miniports reported; clients still look at them */
#define UCX_UHCI_OPTION_FLAGS      0x000002C3
#define UCX_EHCI_OPTION_FLAGS      0x00080A95

static
BOOLEAN
NTAPI
UcxIsLowOrFullSpeed(
    _In_ UcxUsbDevice* Device)
{
    return Device->Speed() == UsbLowSpeed || Device->Speed() == UsbFullSpeed;
}

/* USBDI bus interface functions; the context is always a UCXUSBDEVICE */

static
VOID
USB_BUSIFFN
UcxBusGetUsbdiVersion(
    _In_ PVOID Context,
    _Out_opt_ PUSBD_VERSION_INFORMATION VersionInformation,
    _Out_opt_ PULONG HcdCapabilities)
{
    BOOLEAN Slow = UcxIsLowOrFullSpeed(UcxUsbDevice::FromHandle((UCXUSBDEVICE)Context));

    if (HcdCapabilities != NULL)
        *HcdCapabilities = 0;

    if (VersionInformation != NULL)
    {
        VersionInformation->USBDI_Version = UCX_USBDI_VERSION;
        VersionInformation->Supported_USB_Version = Slow ? 0x0110 : 0x0200;
    }
}

static
NTSTATUS
USB_BUSIFFN
UcxBusQueryBusTime(
    _In_ PVOID Context,
    _Out_opt_ PULONG CurrentFrame)
{
    if (CurrentFrame == NULL)
    {
        DPRINT1("QueryBusTime called without a frame pointer\n");
        return STATUS_INVALID_PARAMETER;
    }

    return UcxUsbDevice::FromHandle((UCXUSBDEVICE)Context)->m_Controller->GetCurrentFrameNumber(CurrentFrame);
}

static
NTSTATUS
USB_BUSIFFN
UcxBusSubmitIsoOutUrb(
    _In_ PVOID Context,
    _In_ PURB Urb)
{
    UNREFERENCED_PARAMETER(Context);

    DPRINT1("SubmitIsoOutUrb is not supported, URB %p\n", Urb);
    NT_ASSERT(FALSE);
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
USB_BUSIFFN
UcxBusQueryBusInformation(
    _In_ PVOID Context,
    _In_ ULONG Level,
    _Inout_ PVOID Buffer,
    _Out_ PULONG BufferLength,
    _Out_opt_ PULONG ActualLength)
{
    UcxController* Controller = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Context)->m_Controller;
    PUSB_BUS_INFORMATION_LEVEL_1 Level1;
    UNICODE_STRING Name;
    ULONG Needed;

    /* An HCD that tracks bandwidth itself is not second guessed */
    if (Controller->HcdGetBandwidthInformation() != NULL)
    {
        DPRINT1("Bus information query with HCD bandwidth tracking is not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    if (Level == 0)
    {
        if (ActualLength != NULL)
            *ActualLength = sizeof(USB_BUS_INFORMATION_LEVEL_0);

        if (*BufferLength < sizeof(USB_BUS_INFORMATION_LEVEL_0))
        {
            DPRINT("Bus information level 0 buffer too small, %lu bytes\n", *BufferLength);
            return STATUS_BUFFER_TOO_SMALL;
        }

        *BufferLength = sizeof(USB_BUS_INFORMATION_LEVEL_0);
        ((PUSB_BUS_INFORMATION_LEVEL_0)Buffer)->TotalBandwidth = UCX_FAKE_TOTAL_BANDWIDTH;
        ((PUSB_BUS_INFORMATION_LEVEL_0)Buffer)->ConsumedBandwidth = 0;
        return STATUS_SUCCESS;
    }

    if (Level != 1)
    {
        DPRINT1("Bus information level %lu is not supported\n", Level);
        return STATUS_NOT_SUPPORTED;
    }

    WdfStringGetUnicodeString(Controller->m_HostControllerInterfaceName, &Name);

    /* The WCHAR already inside the structure is not subtracted */
    Needed = sizeof(USB_BUS_INFORMATION_LEVEL_1) + Name.Length;
    if (ActualLength != NULL)
        *ActualLength = Needed;

    if (*BufferLength < Needed)
    {
        DPRINT("Bus information level 1 needs %lu bytes, got %lu\n", Needed, *BufferLength);
        return STATUS_BUFFER_TOO_SMALL;
    }

    *BufferLength = Needed;
    Level1 = (PUSB_BUS_INFORMATION_LEVEL_1)Buffer;
    Level1->TotalBandwidth = UCX_FAKE_TOTAL_BANDWIDTH;
    Level1->ConsumedBandwidth = 0;
    Level1->ControllerNameLength = Name.Length;
    RtlCopyMemory(Level1->ControllerNameUnicodeString, Name.Buffer, Name.Length);

    return STATUS_SUCCESS;
}

/* SuperSpeed and the root hub count as high speed here */
static
BOOLEAN
USB_BUSIFFN
UcxBusIsDeviceHighSpeed(
    _In_opt_ PVOID Context)
{
    if (Context == NULL)
        return FALSE;

    return !UcxIsLowOrFullSpeed(UcxUsbDevice::FromHandle((UCXUSBDEVICE)Context));
}

static
NTSTATUS
USB_BUSIFFN
UcxBusEnumLogEntry(
    _In_ PVOID Context,
    _In_ ULONG DriverTag,
    _In_ ULONG EnumTag,
    _In_ ULONG P1,
    _In_ ULONG P2)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DriverTag);
    UNREFERENCED_PARAMETER(EnumTag);
    UNREFERENCED_PARAMETER(P1);
    UNREFERENCED_PARAMETER(P2);

    return STATUS_SUCCESS;
}

/* USBPORT never counted microframes either */
static
NTSTATUS
USB_BUSIFFN
UcxBusQueryBusTimeEx(
    _In_opt_ PVOID Context,
    _Out_opt_ PULONG CurrentMicroFrame)
{
    if (CurrentMicroFrame == NULL)
    {
        DPRINT1("QueryBusTimeEx called without a microframe pointer\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (UcxIsLowOrFullSpeed(UcxUsbDevice::FromHandle((UCXUSBDEVICE)Context)))
    {
        DPRINT("QueryBusTimeEx is not supported for low or full speed device %p\n", Context);
        return STATUS_NOT_SUPPORTED;
    }

    *CurrentMicroFrame = 0;
    return STATUS_SUCCESS;
}

/** Answers as the USB 2.0 era controller a client of this speed would have seen. */
static
NTSTATUS
USB_BUSIFFN
UcxBusQueryControllerType(
    _In_opt_ PVOID Context,
    _Out_opt_ PULONG HcdiOptionFlags,
    _Out_opt_ PUSHORT PciVendorId,
    _Out_opt_ PUSHORT PciDeviceId,
    _Out_opt_ PUCHAR PciClass,
    _Out_opt_ PUCHAR PciSubClass,
    _Out_opt_ PUCHAR PciRevisionId,
    _Out_opt_ PUCHAR PciProgIf)
{
    UcxUsbDevice* Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Context);
    const UCX_CONTROLLER_PCI_INFORMATION* Pci = &Device->m_Controller->m_Config.PciDeviceInfo;
    BOOLEAN Slow = UcxIsLowOrFullSpeed(Device);

    if (HcdiOptionFlags != NULL)
        *HcdiOptionFlags = Slow ? UCX_UHCI_OPTION_FLAGS : UCX_EHCI_OPTION_FLAGS;
    if (PciVendorId != NULL)
        *PciVendorId = (USHORT)Pci->VendorId;
    if (PciDeviceId != NULL)
        *PciDeviceId = (USHORT)Pci->DeviceId;
    if (PciClass != NULL)
        *PciClass = PCI_CLASS_SERIAL_BUS_CTLR;
    if (PciSubClass != NULL)
        *PciSubClass = PCI_SUBCLASS_SB_USB;
    if (PciRevisionId != NULL)
        *PciRevisionId = (UCHAR)Pci->RevisionId;
    if (PciProgIf != NULL)
        *PciProgIf = Slow ? 0x00 : 0x20;

    return STATUS_SUCCESS;
}

/** The header always reports the last level that fit, even on failure. */
NTSTATUS
NTAPI
UcxEvtQueryUsbdiInterface(
    _In_ WDFDEVICE Device,
    _In_ LPGUID InterfaceType,
    _Inout_ PINTERFACE ExposedInterface,
    _Inout_opt_ PVOID ExposedInterfaceSpecificData)
{
    PUSB_BUS_INTERFACE_USBDI_V3 Bus = (PUSB_BUS_INTERFACE_USBDI_V3)ExposedInterface;
    USHORT RequestedSize = ExposedInterface->Size;
    USHORT RequestedVersion = ExposedInterface->Version;
    USHORT FilledSize = 0;
    USHORT FilledVersion = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    UCXUSBDEVICE BusContext;

    UNREFERENCED_PARAMETER(InterfaceType);

    /* A query without one is for the root hub itself */
    BusContext = (ExposedInterfaceSpecificData != NULL)
                     ? (UCXUSBDEVICE)ExposedInterfaceSpecificData
                     : (UCXUSBDEVICE)UcxGetRootHubPdoContext(Device)->RootHub->m_Handle;

    if (RequestedSize < sizeof(USB_BUS_INTERFACE_USBDI_V0))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        Bus->BusContext = BusContext;
        Bus->InterfaceReference = UcxInterfaceNoOp;
        Bus->InterfaceDereference = UcxInterfaceNoOp;
        Bus->GetUSBDIVersion = UcxBusGetUsbdiVersion;
        Bus->QueryBusTime = UcxBusQueryBusTime;
        Bus->SubmitIsoOutUrb = UcxBusSubmitIsoOutUrb;
        Bus->QueryBusInformation = UcxBusQueryBusInformation;
        FilledSize = sizeof(USB_BUS_INTERFACE_USBDI_V0);
        FilledVersion = USB_BUSIF_USBDI_VERSION_0;

        if (RequestedVersion >= USB_BUSIF_USBDI_VERSION_1)
        {
            if (RequestedSize < sizeof(USB_BUS_INTERFACE_USBDI_V1))
                Status = STATUS_BUFFER_OVERFLOW;
            else
            {
                Bus->IsDeviceHighSpeed = UcxBusIsDeviceHighSpeed;
                FilledSize = sizeof(USB_BUS_INTERFACE_USBDI_V1);
                FilledVersion = USB_BUSIF_USBDI_VERSION_1;
            }
        }

        if (NT_SUCCESS(Status) && RequestedVersion >= USB_BUSIF_USBDI_VERSION_2)
        {
            if (RequestedSize < sizeof(USB_BUS_INTERFACE_USBDI_V2))
                Status = STATUS_BUFFER_OVERFLOW;
            else
            {
                Bus->EnumLogEntry = UcxBusEnumLogEntry;
                FilledSize = sizeof(USB_BUS_INTERFACE_USBDI_V2);
                FilledVersion = USB_BUSIF_USBDI_VERSION_2;
            }
        }

        /* Anything newer is served as version 3 */
        if (NT_SUCCESS(Status) && RequestedVersion >= USB_BUSIF_USBDI_VERSION_3)
        {
            if (RequestedSize < sizeof(USB_BUS_INTERFACE_USBDI_V3))
                Status = STATUS_BUFFER_OVERFLOW;
            else
            {
                Bus->QueryBusTimeEx = UcxBusQueryBusTimeEx;
                Bus->QueryControllerType = UcxBusQueryControllerType;
                FilledSize = sizeof(USB_BUS_INTERFACE_USBDI_V3);
                FilledVersion = USB_BUSIF_USBDI_VERSION_3;
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBDI interface query size %u version %u failed 0x%lx, served version %u\n",
                RequestedSize, RequestedVersion, Status, FilledVersion);
    }

    ExposedInterface->Size = FilledSize;
    ExposedInterface->Version = FilledVersion;
    return Status;
}

/* USBD client interface */

static
VOID
NTAPI
UcxClientUnregister(
    _In_ USBD_CLIENT_HANDLE ClientHandle);

static
NTSTATUS
NTAPI
UcxClientAllocUrb(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _Out_ PURB* Urb);

static
NTSTATUS
NTAPI
UcxClientAllocIsochUrb(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ ULONG NumberOfIsochPackets,
    _Out_ PURB* Urb);

static
NTSTATUS
NTAPI
UcxClientSelectConfigXrbBuild(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    _In_ PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    _Out_ PURB* UrbOut);

static
NTSTATUS
NTAPI
UcxClientSelectInterfaceXrbBuild(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    _In_ PUSBD_INTERFACE_LIST_ENTRY InterfaceEntry,
    _Out_ PURB* UrbOut);

static
VOID
NTAPI
UcxClientReleaseUrb(
    _In_ PURB Urb);

static
NTSTATUS
NTAPI
UcxClientOffloadSelectXrbAllocate(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    _Inout_ PUSBD_INTERFACE_LIST_ENTRY InterfaceEntry,
    _In_ ULONG OffloadedEndpoints,
    _Out_ PURB* Urb)
{
    UNREFERENCED_PARAMETER(ClientHandle);
    UNREFERENCED_PARAMETER(ConfigurationHandle);
    UNREFERENCED_PARAMETER(InterfaceEntry);
    UNREFERENCED_PARAMETER(OffloadedEndpoints);

    /* No controller here offers endpoint offload */
    *Urb = NULL;
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NTAPI
UcxClientOffloadNotifyXrbAllocate(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ USBD_PIPE_HANDLE PipeHandle,
    _In_ ULONG Flags,
    _Out_ PURB* Urb)
{
    UNREFERENCED_PARAMETER(ClientHandle);
    UNREFERENCED_PARAMETER(PipeHandle);
    UNREFERENCED_PARAMETER(Flags);

    *Urb = NULL;
    return STATUS_NOT_SUPPORTED;
}

/** Answers the parts of a version 0x603 request that fit in its size, and reports what was filled. */
static
VOID
NTAPI
UcxFillClientInterface603(
    _Inout_ UcxUsbdHandle* Handle,
    _Inout_ PUSBD_CLIENT_INTERFACE Interface)
{
    PUSBD_CLIENT_INTERFACE_603 Extended = (PUSBD_CLIENT_INTERFACE_603)Interface;
    USHORT Size = sizeof(*Interface);
    USHORT Version = USBD_CLIENT_INTERFACE_VERSION;

    if (Interface->Header.Version >= USBD_CLIENT_INTERFACE_VERSION_603 &&
        Interface->Header.Size >= USBD_CLIENT_INTERFACE_603_SIZE_SECURE)
    {
        Extended->SecureIsochXrbAllocate = UcxClientAllocIsochUrb;
        if (Handle->m_VerifierEnabled)
            Handle->m_VerifierFailSecureTransfer = Extended->VerifierFailSecureTransferSupport;

        Size = USBD_CLIENT_INTERFACE_603_SIZE_SECURE;
        Version = USBD_CLIENT_INTERFACE_VERSION_603;

        if (Interface->Header.Size >= sizeof(*Extended))
        {
            Extended->OffloadSelectInterfaceXrbAllocate = UcxClientOffloadSelectXrbAllocate;
            Extended->OffloadNotificationXrbAllocate = UcxClientOffloadNotifyXrbAllocate;
            if (Handle->m_VerifierEnabled)
                Handle->m_VerifierFailEndpointOffload = Extended->VerifierFailEndpointOffload;

            Size = sizeof(*Extended);
        }
    }

    Interface->Header.Size = Size;
    Interface->Header.Version = Version;
}

NTSTATUS
NTAPI
UcxEvtQueryUsbdClientInterface(
    _In_ WDFDEVICE Device,
    _In_ LPGUID InterfaceType,
    _Inout_ PINTERFACE ExposedInterface,
    _Inout_opt_ PVOID ExposedInterfaceSpecificData)
{
    UcxRootHub* RootHub = UcxGetRootHubPdoContext(Device)->RootHub;
    UcxUsbDevice* UsbDevice;

    UNREFERENCED_PARAMETER(InterfaceType);

    UsbDevice = (ExposedInterfaceSpecificData != NULL)
                    ? UcxUsbDevice::FromHandle((UCXUSBDEVICE)ExposedInterfaceSpecificData)
                    : RootHub->Device();

    return UcxUsbdHandle::CreateFromQuery(RootHub, UsbDevice, (PUSBD_CLIENT_INTERFACE)ExposedInterface);
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UcxUsbdHandle::CreateFromQuery(
    _In_ UcxRootHub* RootHub,
    _In_ UcxUsbDevice* Device,
    _Inout_ PUSBD_CLIENT_INTERFACE Interface)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    UcxUsbdHandle* Handle;
    WDFMEMORY Memory;
    PVOID Buffer;
    NTSTATUS Status;

    PAGED_CODE();

    if (Interface->Header.Size < sizeof(*Interface))
    {
        DPRINT1("USBD client interface size %u, expected at least %u\n",
                Interface->Header.Size, (ULONG)sizeof(*Interface));
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Interface->Header.Version < USBD_CLIENT_INTERFACE_VERSION)
    {
        DPRINT1("USBD client interface version %u is not supported\n", Interface->Header.Version);
        return STATUS_NOT_SUPPORTED;
    }

    if (Interface->DeviceObject == NULL || Interface->PoolTag == 0)
    {
        DPRINT1("USBD client registration with device object %p, tag 0x%lx\n",
                Interface->DeviceObject, Interface->PoolTag);
        return STATUS_INVALID_PARAMETER;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Device->m_Handle;

    Status = WdfMemoryCreate(&Attributes, NonPagedPool, Interface->PoolTag, sizeof(*Handle), &Memory, &Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBD handle allocation for device %p failed 0x%lx\n", Device, Status);
        return Status;
    }

    /* The handle has to outlive the USB device it was registered against */
    WdfObjectReference(Memory);

    RtlZeroMemory(Buffer, sizeof(*Handle));
    Handle = new (Buffer) UcxUsbdHandle();
    Handle->m_ClientDeviceObject = Interface->DeviceObject;
    Handle->m_PoolTag = Interface->PoolTag;
    Handle->m_ContractVersion = Interface->ClientContractVersion;
    Handle->m_ClientContext = Interface->ClientContext;
    Handle->m_Device = Device;
    Handle->m_Controller = Device->m_Controller;
    Handle->m_Memory = Memory;
    Handle->m_RootHub = RootHub;
    Handle->m_RootHubPdo = RootHub->Pdo();
    KeInitializeSpinLock(&Handle->m_XrbLock);
    InitializeListHead(&Handle->m_XrbList);

    if (Interface->VerifierEnabled != 0)
    {
        Handle->m_VerifierEnabled = TRUE;
        Handle->m_VerifierFailRegistration = Interface->VerifierFailRegistration;
        Handle->m_VerifierFailChainedMdl = Interface->VerifierFailChainedMdlSupport;
        Handle->m_VerifierFailStaticStreamSupport = Interface->VerifierFailStaticStreamSupport;
        Handle->m_VerifierStaticStreamCountOverride = Interface->VerifierStaticStreamCountOverride;
        Handle->m_VerifierFailEnableStaticStreams = Interface->VerifierFailEnableStaticStreams;
    }

    Handle->m_TrackXrbs = Handle->m_VerifierEnabled || Device->m_Controller->m_DriverVerifierEnabled;

    /* Not undone when the registration then fails */
    if (Handle->m_VerifierEnabled)
        Device->TrackStaleHandles();

    if (UcxVerifierWantsFailure(Handle->m_VerifierFailRegistration))
    {
        DPRINT1("Verifier failed USBD client registration for device %p\n", Device);
        WdfObjectDelete(Memory);
        WdfObjectDereference(Memory);
        return UcxRandomErrorStatus();
    }

    Interface->Header.Context = NULL;
    Interface->Header.InterfaceReference = UcxInterfaceNoOp;
    Interface->Header.InterfaceDereference = UcxInterfaceNoOp;
    Interface->Handle = (USBD_CLIENT_HANDLE)Handle;
    Interface->Unregister = UcxClientUnregister;
    Interface->AllocUrb = UcxClientAllocUrb;
    Interface->AllocIsochUrb = UcxClientAllocIsochUrb;
    Interface->AllocSelectConfigUrb = UcxClientSelectConfigXrbBuild;
    Interface->AllocSelectInterfaceUrb = UcxClientSelectInterfaceXrbBuild;
    Interface->ReleaseUrb = UcxClientReleaseUrb;

    UcxFillClientInterface603(Handle, Interface);

    ObReferenceObject(Handle->m_ClientDeviceObject);

    {
        SpinLockGuard Guard(&Device->m_UsbdHandleLock);

        InsertTailList(&Device->m_UsbdHandleList, &Handle->m_DeviceLink);
    }

    InterlockedIncrement(&Device->m_Controller->m_UsbdInterfaceCount);
    DPRINT("USBD client %p registered handle %p on device %p\n", Interface->DeviceObject, Handle, Device);
    return STATUS_SUCCESS;
}

/* A handle orphaned by its device only has its storage left to release */
VOID
UcxUsbdHandle::Unregister()
{
    WDFMEMORY Memory = m_Memory;

    if (!m_Orphaned)
    {
        {
            SpinLockGuard Guard(&m_Device->m_UsbdHandleLock);

            RemoveEntryList(&m_DeviceLink);
            UcxClearListEntry(&m_DeviceLink);
        }

        ObDereferenceObject(m_ClientDeviceObject);
    }

    InterlockedDecrement(&m_Controller->m_UsbdInterfaceCount);

    /* Deleting the storage deletes every XRB still allocated under it */
    m_Unregistered = TRUE;
    WdfObjectDelete(Memory);
    WdfObjectDereference(Memory);
}

VOID
NTAPI
UcxDropLeakedUsbdHandles(
    _In_ UcxUsbDevice* Device)
{
    BOOLEAN Verifying = Device->m_Controller->m_DriverVerifierEnabled;
    UcxUsbdHandle* Handle;
    PLIST_ENTRY Entry;

    SpinLockGuard Guard(&Device->m_UsbdHandleLock);

    for (Entry = Device->m_UsbdHandleList.Flink; Entry != &Device->m_UsbdHandleList; Entry = Entry->Flink)
    {
        Handle = CONTAINING_RECORD(Entry, UcxUsbdHandle, m_DeviceLink);

        DPRINT1("USBD handle %p of client %p leaked on device %p\n", Handle, Handle->m_ClientDeviceObject, Device);

        if (Verifying || Handle->m_VerifierEnabled)
        {
            KeBugCheckEx(UCX_BUGCHECK_USB3,
                         UCX_USB3_HANDLE_LEAKED,
                         (ULONG_PTR)Handle,
                         (ULONG_PTR)Handle->m_ClientDeviceObject,
                         0);
        }

        ObDereferenceObject(Handle->m_ClientDeviceObject);
        Handle->m_Orphaned = TRUE;
    }
}

BOOLEAN
NTAPI
UcxAnyUsbdHandleHasVerifier(
    _In_ UcxUsbDevice* Device)
{
    PLIST_ENTRY Entry;

    for (Entry = Device->m_UsbdHandleList.Flink; Entry != &Device->m_UsbdHandleList; Entry = Entry->Flink)
    {
        if (CONTAINING_RECORD(Entry, UcxUsbdHandle, m_DeviceLink)->m_VerifierEnabled)
            return TRUE;
    }

    return FALSE;
}

/* XRB allocation */

NTSTATUS
UcxUsbdHandle::AllocateXrb(
    _In_ ULONG Size,
    _In_ ULONG Type,
    _Out_ UcxXrbPreamble** XrbOut)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    UcxXrbPreamble* Xrb;
    WDFMEMORY Memory;
    PVOID Buffer;
    NTSTATUS Status;

    *XrbOut = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = m_Memory;

    Status = WdfMemoryCreate(&Attributes, NonPagedPool, m_PoolTag, Size, &Memory, &Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("XRB allocation of %lu bytes for handle %p failed 0x%lx\n", Size, this, Status);
        return Status;
    }

    /* Both released by the free call, which may come after unregister */
    WdfObjectReference(Memory);
    WdfObjectReference(m_Memory);
    RtlZeroMemory(Buffer, Size);

    Xrb = (UcxXrbPreamble*)Buffer;
    Xrb->Signature = UCX_XRB_SIGNATURE;
    Xrb->TotalSize = Size;
    Xrb->ContractVersion = m_ContractVersion;
    Xrb->Handle = this;
    Xrb->State = UCX_XRB_IDLE;
    Xrb->Type = Type;
    Xrb->Memory = Memory;

    *XrbOut = Xrb;
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UcxDeleteXrbMemory(
    _In_ UcxXrbPreamble* Xrb)
{
    WDFMEMORY Memory = Xrb->Memory;
    UcxUsbdHandle* Handle = Xrb->Handle;
    WDFMEMORY HandleMemory = Handle->m_Memory;

    /* After unregister KMDF already deleted it along with the handle storage */
    if (!Handle->m_Unregistered)
        WdfObjectDelete(Memory);

    WdfObjectDereference(Memory);
    WdfObjectDereference(HandleMemory);
}

/* Done last, after every failure point */
VOID
UcxUsbdHandle::TrackXrb(
    _In_ UcxXrbPreamble* Xrb)
{
    if (!m_TrackXrbs)
        return;

    SpinLockGuard Guard(&m_XrbLock);
    InsertTailList(&m_XrbList, &Xrb->TrackingLink);
}

static
NTSTATUS
NTAPI
UcxClientAllocUrb(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _Out_ PURB* Urb)
{
    UcxUsbdHandle* Handle = UcxUsbdHandle::FromClientHandle(ClientHandle);
    WDF_OBJECT_ATTRIBUTES RequestAttributes;
    UcxXrbPreamble* Xrb;
    NTSTATUS Status;

    *Urb = NULL;

    Status = Handle->AllocateXrb(sizeof(UcxXrbPreamble) + sizeof(URB), UCX_XRB_KIND_PLAIN, &Xrb);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Carries the HCD's request context like every other request it gets */
    RequestAttributes = Handle->m_RootHub->m_Config.WdfRequestAttributes;
    RequestAttributes.ParentObject = Handle->m_RootHubPdo;

    Status = WdfRequestCreate(&RequestAttributes, NULL, &Xrb->Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("XRB request creation for handle %p failed 0x%lx\n", Handle, Status);
        UcxDeleteXrbMemory(Xrb);
        return Status;
    }

    Handle->TrackXrb(Xrb);
    *Urb = UcxUrbFromXrb(Xrb);
    return STATUS_SUCCESS;
}

/* Always one spare packet descriptor; isoch XRBs never get a preallocated request */
static
NTSTATUS
NTAPI
UcxClientAllocIsochUrb(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ ULONG NumberOfIsochPackets,
    _Out_ PURB* Urb)
{
    UcxUsbdHandle* Handle = UcxUsbdHandle::FromClientHandle(ClientHandle);
    UcxXrbPreamble* Xrb;
    NTSTATUS Status;

    *Urb = NULL;

    Status = Handle->AllocateXrb(sizeof(UcxXrbPreamble) + GET_ISO_URB_SIZE(NumberOfIsochPackets),
                                 UCX_XRB_KIND_ISO_PACKETS,
                                 &Xrb);
    if (!NT_SUCCESS(Status))
        return Status;

    Handle->TrackXrb(Xrb);
    *Urb = UcxUrbFromXrb(Xrb);
    return STATUS_SUCCESS;
}

/** Lays out one interface information block for a select request. */
static
VOID
NTAPI
UcxFillInterfaceInformation(
    _Out_ PUSBD_INTERFACE_INFORMATION Info,
    _In_ PUSB_INTERFACE_DESCRIPTOR Descriptor,
    _In_ ULONG Pipes)
{
    ULONG Index;

    Info->InterfaceNumber = Descriptor->bInterfaceNumber;
    Info->AlternateSetting = Descriptor->bAlternateSetting;
    Info->NumberOfPipes = Pipes;
    Info->Length = (USHORT)GET_USBD_INTERFACE_SIZE(Pipes);

    for (Index = 0; Index < Pipes; Index++)
    {
        Info->Pipes[Index].MaximumTransferSize = MAXULONG;
        Info->Pipes[Index].PipeFlags = 0;
    }
}

static
NTSTATUS
NTAPI
UcxClientSelectConfigXrbBuild(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    _In_ PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    _Out_ PURB* UrbOut)
{
    UcxUsbdHandle* Handle = UcxUsbdHandle::FromClientHandle(ClientHandle);
    BOOLEAN Strict = Handle->m_ContractVersion >= USBD_CLIENT_CONTRACT_VERSION_602;
    PUSBD_INTERFACE_LIST_ENTRY Entry;
    PUSBD_INTERFACE_INFORMATION Info;
    UcxXrbPreamble* Xrb;
    PUCHAR End;
    ULONG Interfaces = 0;
    ULONG Pipes = 0;
    USHORT Size;
    PURB Urb;
    NTSTATUS Status;

    *UrbOut = NULL;

    /* Sizes add up in 16 bits, as the structure lengths are 16 bit */
    Size = (USHORT)(sizeof(UcxXrbPreamble) + sizeof(struct _URB_SELECT_CONFIGURATION) -
                    sizeof(USBD_INTERFACE_INFORMATION));

    for (Entry = InterfaceList; Entry->InterfaceDescriptor != NULL; Entry++)
    {
        Pipes += Entry->InterfaceDescriptor->bNumEndpoints;
        if (Pipes > UCX_MAX_SELECT_PIPES)
        {
            DPRINT1("Select config for handle %p has too many pipes\n", Handle);
            return STATUS_INVALID_PARAMETER;
        }

        if (Strict && Entry->Interface != NULL)
        {
            DPRINT1("Select config entry %p for handle %p already has an interface\n", Entry, Handle);
            return STATUS_INVALID_PARAMETER;
        }

        Size += (USHORT)GET_USBD_INTERFACE_SIZE(Entry->InterfaceDescriptor->bNumEndpoints);

        if (Interfaces++ > UCX_MAX_SELECT_INTERFACES)
        {
            DPRINT1("Select config for handle %p has too many interfaces\n", Handle);
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (Strict && Entry->Interface != NULL)
    {
        DPRINT1("Select config list end %p for handle %p has an interface\n", Entry, Handle);
        return STATUS_INVALID_PARAMETER;
    }

    Status = Handle->AllocateXrb(Size, UCX_XRB_KIND_CONFIG, &Xrb);
    if (!NT_SUCCESS(Status))
        return Status;

    Urb = UcxUrbFromXrb(Xrb);
    End = (PUCHAR)Xrb + Size;
    Info = &Urb->UrbSelectConfiguration.Interface;

    for (Entry = InterfaceList; Entry->InterfaceDescriptor != NULL; Entry++)
    {
        /* The descriptor may have changed since it was sized */
        ULONG EntryPipes = Entry->InterfaceDescriptor->bNumEndpoints;

        if ((PUCHAR)Info + GET_USBD_INTERFACE_SIZE(EntryPipes) > End)
        {
            DPRINT1("Interface descriptor %p grew while building select config\n", Entry->InterfaceDescriptor);
            UcxDeleteXrbMemory(Xrb);
            return STATUS_INVALID_PARAMETER;
        }

        UcxFillInterfaceInformation(Info, Entry->InterfaceDescriptor, EntryPipes);
        Entry->Interface = Info;
        Info = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)Info + Info->Length);
    }

    Urb->UrbHeader.Length = (USHORT)(Size - sizeof(UcxXrbPreamble));
    Urb->UrbHeader.Function = URB_FUNCTION_SELECT_CONFIGURATION;
    Urb->UrbSelectConfiguration.ConfigurationDescriptor = ConfigurationDescriptor;

    Handle->TrackXrb(Xrb);
    *UrbOut = Urb;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UcxClientSelectInterfaceXrbBuild(
    _In_ USBD_CLIENT_HANDLE ClientHandle,
    _In_ USBD_CONFIGURATION_HANDLE ConfigurationHandle,
    _In_ PUSBD_INTERFACE_LIST_ENTRY InterfaceEntry,
    _Out_ PURB* UrbOut)
{
    UcxUsbdHandle* Handle = UcxUsbdHandle::FromClientHandle(ClientHandle);
    PUSBD_INTERFACE_INFORMATION Info;
    UcxXrbPreamble* Xrb;
    ULONG Pipes;
    USHORT Size;
    PURB Urb;
    NTSTATUS Status;

    *UrbOut = NULL;

    if (InterfaceEntry->InterfaceDescriptor == NULL || InterfaceEntry->Interface != NULL)
    {
        DPRINT1("Select interface entry %p for handle %p is malformed\n", InterfaceEntry, Handle);
        return STATUS_INVALID_PARAMETER;
    }

    Pipes = InterfaceEntry->InterfaceDescriptor->bNumEndpoints;
    if (Pipes > UCX_MAX_SELECT_PIPES)
    {
        DPRINT1("Select interface for handle %p has %lu pipes\n", Handle, Pipes);
        return STATUS_INVALID_PARAMETER;
    }

    /* With no pipes this is shorter than the URB structure, like the public macro */
    Size = (USHORT)(sizeof(UcxXrbPreamble) + GET_SELECT_INTERFACE_REQUEST_SIZE(Pipes));

    Status = Handle->AllocateXrb(Size, UCX_XRB_KIND_ALT_SETTING, &Xrb);
    if (!NT_SUCCESS(Status))
        return Status;

    Urb = UcxUrbFromXrb(Xrb);
    Info = &Urb->UrbSelectInterface.Interface;
    UcxFillInterfaceInformation(Info, InterfaceEntry->InterfaceDescriptor, Pipes);
    InterfaceEntry->Interface = Info;

    Urb->UrbHeader.Length = (USHORT)(Size - sizeof(UcxXrbPreamble));
    Urb->UrbHeader.Function = URB_FUNCTION_SELECT_INTERFACE;
    Urb->UrbSelectInterface.ConfigurationHandle = ConfigurationHandle;

    Handle->TrackXrb(Xrb);
    *UrbOut = Urb;
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UcxClientReleaseUrb(
    _In_ PURB Urb)
{
    UcxXrbPreamble* Xrb = UcxXrbFromUrb(Urb);
    UcxUsbdHandle* Handle = Xrb->Handle;

    if (Xrb->Signature != UCX_XRB_SIGNATURE)
    {
        DPRINT1("Freeing corrupted XRB for URB %p\n", Urb);
        KeBugCheckEx(UCX_BUGCHECK_USB3,
                     UCX_USB3_XRB_CORRUPTED,
                     0,
                     (ULONG_PTR)Urb,
                     (ULONG_PTR)Handle->m_ClientDeviceObject);
    }

    if (Xrb->State != UCX_XRB_IDLE)
    {
        DPRINT1("Freeing URB %p while it is still active\n", Urb);
        KeBugCheckEx(UCX_BUGCHECK_USB3,
                     UCX_USB3_ACTIVE_URB_REUSED,
                     0,
                     (ULONG_PTR)Urb,
                     (ULONG_PTR)Handle->m_ClientDeviceObject);
    }

    if (Handle->m_TrackXrbs)
    {
        SpinLockGuard Guard(&Handle->m_XrbLock);

        RemoveEntryList(&Xrb->TrackingLink);
        UcxClearListEntry(&Xrb->TrackingLink);
    }

    if (Xrb->Request != NULL)
    {
        WdfObjectDelete(Xrb->Request);
        Xrb->Request = NULL;
    }

    UcxDeleteXrbMemory(Xrb);
}

static
VOID
NTAPI
UcxClientUnregister(
    _In_ USBD_CLIENT_HANDLE ClientHandle)
{
    UcxUsbdHandle::FromClientHandle(ClientHandle)->Unregister();
}
