/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     USB4 host routers behind hub ports: binding, port mappings and power coupling
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* {9C0D3EAE-FBA6-4E08-9E8D-181CBB3997D6}: virtual power PDO of a USB4 host router */
static const GUID HubUsb4PowerPdoInterface =
    { 0x9c0d3eae, 0xfba6, 0x4e08, { 0x9e, 0x8d, 0x18, 0x1c, 0xbb, 0x39, 0x97, 0xd6 } };

/* Root port to host router names, written on the hub's own PDO */
static const DEVPROPKEY HubKeyUsb4PortMappings =
    { { 0xb5a825b8, 0x3907, 0x40b5, { 0xab, 0xca, 0x84, 0x51, 0xe2, 0x45, 0x3d, 0x5f } }, 1 };

/* PCI properties ucx01000 copies from the controller onto the root hub PDO */
static const DEVPROPKEY HubKeyPciSerialNumber =
    { { 0x3ab22e31, 0x8264, 0x4b4e, { 0x9a, 0xf5, 0xa8, 0xd2, 0xd8, 0xe3, 0x3e, 0x62 } }, 40 };
static const DEVPROPKEY HubKeyPciUsb4PortAttributes =
    { { 0x3ab22e31, 0x8264, 0x4b4e, { 0x9a, 0xf5, 0xa8, 0xd2, 0xd8, 0xe3, 0x3e, 0x62 } }, 42 };

/* Reference string of the power PDO interface: prefix, then ^<acpi>^<dvsec>^<cmid> */
#define HUB_USB4_REFERENCE_PREFIX       L"Usb4-Host-Interface-"
#define HUB_USB4_REFERENCE_PREFIX_CHARS 20
#define HUB_USB4_REFERENCE_CHARS        256
#define HUB_USB4_FIELD_SEPARATOR        L'^'

/* Symbolic link copy the open work item carries, and the name it opens */
#define HUB_USB4_LINK_CHARS             256
#define HUB_USB4_OPEN_NAME_CHARS        256

/* DVSEC port attributes: a 3 bit host index per port nibble, 7 for none */
#define HUB_USB4_HOST_INDEX_MASK        0x07
#define HUB_USB4_NO_HOST                0x07
#define HUB_USB4_DVSEC_NAME_CHARS       20

/* Room per port mapping entry beyond the host name: port number, '#' and the NUL */
#define HUB_USB4_MAPPING_EXTRA_BYTES    14

/** Lifecycle of the remote target on a host router's power PDO. */
enum class HubUsb4TargetState : ULONG
{
    Idle,
    Opening,
    Open,
    Closing,
    Closed
};

/** One USB4 host router some port of the hub belongs to; lives in a WDFMEMORY. */
struct HubUsb4Host
{
    /* Escaped name of the first port bound to this host */
    WDFSTRING Name;
    WDFIOTARGET Target;
    WDFWAITLOCK Lock;
    HubUsb4TargetState State;

    /* Physical device of the opened target, NULL while closed */
    PDEVICE_OBJECT PowerPdo;
    LONG PowerReferences;
    BOOLEAN RelationAdded;
};

struct HubUsb4TargetContext
{
    HubFdo* Hub;

    /* Set once the target is open */
    HubUsb4Host* Host;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubUsb4TargetContext, HubUsb4GetTargetContext);

struct HubUsb4OpenContext
{
    WCHAR SymbolicLink[HUB_USB4_LINK_CHARS];
    HubUsb4Host* Host;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubUsb4OpenContext, HubUsb4GetOpenContext);

static DRIVER_NOTIFICATION_CALLBACK_ROUTINE HubUsb4InterfaceChange;
static EVT_WDF_WORKITEM HubUsb4OpenWorkItem;
static EVT_WDF_IO_TARGET_QUERY_REMOVE HubUsb4TargetQueryRemove;
static EVT_WDF_IO_TARGET_REMOVE_CANCELED HubUsb4TargetRemoveCanceled;
static EVT_WDF_IO_TARGET_REMOVE_COMPLETE HubUsb4TargetRemoveComplete;

/* HOST RECORDS **************************************************************/

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubUsb4Initialize(
    _In_ HubFdo* Hub)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    Status = WdfCollectionCreate(&Attributes, &Hub->m_Usb4Hosts);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host list not created 0x%lx\n", Hub, Status);
        return Status;
    }

    Status = WdfWaitLockCreate(&Attributes, &Hub->m_Usb4HostsLock);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p USB4 host list lock not created 0x%lx\n", Hub, Status);

    return Status;
}

static
HubUsb4Host*
NTAPI
HubUsb4HostAt(
    _In_ HubFdo* Hub,
    _In_ ULONG Index)
{
    WDFOBJECT Item = WdfCollectionGetItem(Hub->m_Usb4Hosts, Index);

    if (Item == NULL)
        return NULL;

    return (HubUsb4Host*)WdfMemoryGetBuffer((WDFMEMORY)Item, NULL);
}

/* Case insensitive; the caller holds the host list lock */
static
HubUsb4Host*
NTAPI
HubUsb4FindHost(
    _In_ HubFdo* Hub,
    _In_ PCUNICODE_STRING Name)
{
    UNICODE_STRING HostName;
    HubUsb4Host* Host;
    ULONG Count = WdfCollectionGetCount(Hub->m_Usb4Hosts);
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
    {
        Host = HubUsb4HostAt(Hub, Index);
        if (Host == NULL)
            break;

        WdfStringGetUnicodeString(Host->Name, &HostName);
        if (RtlEqualUnicodeString(&HostName, Name, TRUE))
            return Host;
    }

    return NULL;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
HubUsb4Host*
NTAPI
HubUsb4FindPortHost(
    _In_ HubPort* Port)
{
    HubFdo* Hub = Port->m_Hub;
    UNICODE_STRING Name;

    WdfStringGetUnicodeString(Port->m_Usb4HostName, &Name);

    HubWaitLockGuard Guard(Hub->m_Usb4HostsLock);
    return HubUsb4FindHost(Hub, &Name);
}

/* The host router names fields the same way, so both sides compare equal */
static
VOID
NTAPI
HubUsb4EscapeName(
    _In_ WDFSTRING Name)
{
    UNICODE_STRING Text;
    ULONG Index;

    WdfStringGetUnicodeString(Name, &Text);

    for (Index = 0; Index < Text.Length / sizeof(WCHAR); Index++)
    {
        if (Text.Buffer[Index] == L'\\' || Text.Buffer[Index] == L'/' || Text.Buffer[Index] == L'^')
            Text.Buffer[Index] = L'#';
    }
}

/* The record starts with a target that is created but not opened */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubUsb4CreateHost(
    _In_ HubFdo* Hub,
    _In_ WDFSTRING Name)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    HubUsb4Host* Host;
    WDFMEMORY Memory;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    Status = WdfMemoryCreate(&Attributes, NonPagedPool, HUB_TAG_HUB, sizeof(*Host), &Memory, (PVOID*)&Host);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host record not created 0x%lx\n", Hub, Status);
        return Status;
    }

    RtlZeroMemory(Host, sizeof(*Host));

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubUsb4TargetContext);
    Attributes.ParentObject = Memory;

    Status = WdfIoTargetCreate(Hub->m_Device, &Attributes, &Host->Target);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host target not created 0x%lx\n", Hub, Status);
        goto Failed;
    }

    HubUsb4GetTargetContext(Host->Target)->Hub = Hub;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Memory;

    Status = WdfWaitLockCreate(&Attributes, &Host->Lock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host lock not created 0x%lx\n", Hub, Status);
        goto Failed;
    }

    Host->Name = Name;

    {
        HubWaitLockGuard Guard(Hub->m_Usb4HostsLock);
        Status = WdfCollectionAdd(Hub->m_Usb4Hosts, Memory);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host record not listed 0x%lx\n", Hub, Status);
        goto Failed;
    }

    return STATUS_SUCCESS;

Failed:
    WdfObjectDelete(Memory);
    return Status;
}

/**
 * @brief
 * Marks Port as tunneled through the host router Name; ports naming the same host share one record.
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubUsb4BindPort(
    _In_ HubFdo* Hub,
    _In_ HubPort* Port,
    _In_ WDFSTRING Name)
{
    UNICODE_STRING Text;
    HubUsb4Host* Host;
    NTSTATUS Status;

    PAGED_CODE();

    /* QUIRK: never cleared, even when no port is bound on a later start */
    if (!Hub->m_HasUsb4Ports)
    {
        DPRINT("Hub %p has ports behind a USB4 host router\n", Hub);
        Hub->m_HasUsb4Ports = TRUE;
    }

    HubUsb4EscapeName(Name);
    WdfStringGetUnicodeString(Name, &Text);

    {
        HubWaitLockGuard Guard(Hub->m_Usb4HostsLock);
        Host = HubUsb4FindHost(Hub, &Text);
    }

    if (Host == NULL)
    {
        Status = HubUsb4CreateHost(Hub, Name);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    Port->m_Usb4HostName = Name;
    Port->SetFlag(PortFlag::Usb4Host);

    DPRINT("Port %u belongs to USB4 host %wZ\n", Port->Number(), &Text);
    return STATUS_SUCCESS;
}

/* DVSEC MAPPING *************************************************************/

/**
 * @brief
 * Binds USB 3 ports to the host routers named by the USB4 DVSEC; an earlier ACPI mapping wins.
 */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4MapDvsecHosts(
    _In_ HubFdo* Hub)
{
    WDF_DEVICE_PROPERTY_DATA Property;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WCHAR Buffer[HUB_USB4_DVSEC_NAME_CHARS];
    UNICODE_STRING Text;
    WDFMEMORY Memory;
    WDFSTRING Name;
    DEVPROPTYPE Type;
    PUCHAR Nibbles;
    size_t Bytes;
    ULONG64 Serial = 0;
    ULONG Required;
    ULONG PortNumber;
    ULONG HostIndex;
    HubPort* Port;
    NTSTATUS Status;

    PAGED_CODE();

    {
        HubWaitLockGuard Guard(Hub->m_Usb4HostsLock);
        if (WdfCollectionGetCount(Hub->m_Usb4Hosts) != 0)
            return;
    }

    WDF_DEVICE_PROPERTY_DATA_INIT(&Property, &HubKeyPciSerialNumber);
    Status = WdfDeviceQueryPropertyEx(Hub->m_Device, &Property, sizeof(Serial), &Serial, &Required, &Type);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Hub %p PCI serial number query failed 0x%lx\n", Hub, Status);
        return;
    }

    WDF_DEVICE_PROPERTY_DATA_INIT(&Property, &HubKeyPciUsb4PortAttributes);
    Status = WdfDeviceAllocAndQueryPropertyEx(Hub->m_Device,
                                              &Property,
                                              PagedPool,
                                              WDF_NO_OBJECT_ATTRIBUTES,
                                              &Memory,
                                              &Type);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p has a PCI serial number but no USB4 port attributes 0x%lx\n", Hub, Status);
        return;
    }

    Nibbles = (PUCHAR)WdfMemoryGetBuffer(Memory, &Bytes);
    if (Nibbles == NULL)
    {
        DPRINT1("Hub %p USB4 port attributes are empty\n", Hub);
        WdfObjectDelete(Memory);
        return;
    }

    /* Port N is the low nibble of byte (N - 1) / 2 when N is odd, the high one when even */
    for (PortNumber = 1; PortNumber <= 2 * Bytes; PortNumber++)
    {
        HostIndex = Nibbles[(PortNumber - 1) / 2];
        if ((PortNumber & 1) == 0)
            HostIndex >>= 4;
        HostIndex &= HUB_USB4_HOST_INDEX_MASK;

        if (HostIndex == HUB_USB4_NO_HOST)
            continue;

        Port = Hub->FindPort(PortNumber);
        if (Port == NULL)
        {
            DPRINT1("Hub %p USB4 port attributes name port %lu, which does not exist\n", Hub, PortNumber);
            continue;
        }

        if (!Port->IsUsb30())
        {
            DPRINT1("Hub %p USB4 port attributes name port %lu, which is not a USB 3 port\n", Hub, PortNumber);
            continue;
        }

        RtlInitEmptyUnicodeString(&Text, Buffer, sizeof(Buffer));
        Status = RtlUnicodeStringPrintf(&Text, L"%I64x_%lu", Serial, HostIndex);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Port %lu USB4 host name not formatted 0x%lx\n", PortNumber, Status);
            break;
        }

        /* Parented to the hub so a restart does not leak them */
        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = Hub->m_Device;

        Status = WdfStringCreate(&Text, &Attributes, &Name);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Port %lu USB4 host name string not created 0x%lx\n", PortNumber, Status);
            break;
        }

        Status = HubUsb4BindPort(Hub, Port, Name);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Port %lu not bound to its USB4 host 0x%lx\n", PortNumber, Status);
            WdfObjectDelete(Name);
            break;
        }
    }

    WdfObjectDelete(Memory);
}

/* PORT MAPPING PROPERTY *****************************************************/

/**
 * @brief
 * Publishes "<port>#<host>" for every bound port on the hub's PDO.
 * QUIRK: with no bound port an older value is left in place.
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubUsb4AssignPortMappings(
    _In_ HubFdo* Hub)
{
    UNICODE_STRING Name;
    PLIST_ENTRY Entry;
    HubPort* Port;
    PWCHAR Buffer;
    PWCHAR Cursor;
    PWCHAR End;
    size_t Remaining;
    SIZE_T Total = 0;
    SIZE_T Add;
    ULONG Used;
    NTSTATUS Status;

    PAGED_CODE();

    for (Entry = Hub->m_Mux.Ports.Flink; Entry != &Hub->m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        if (Port->m_Usb4HostName == NULL)
            continue;

        WdfStringGetUnicodeString(Port->m_Usb4HostName, &Name);
        Add = Name.Length + HUB_USB4_MAPPING_EXTRA_BYTES;
        if (Total + Add < Total)
        {
            DPRINT1("Hub %p USB4 port mapping size overflows\n", Hub);
            return STATUS_INTEGER_OVERFLOW;
        }

        Total += Add;
    }

    if (Total == 0)
        return STATUS_SUCCESS;

    Total += sizeof(WCHAR);
    if (Total > MAXULONG)
    {
        DPRINT1("Hub %p USB4 port mapping is too large\n", Hub);
        return STATUS_INTEGER_OVERFLOW;
    }

    Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, Total, HUB_TAG_HUB);
    if (Buffer == NULL)
    {
        DPRINT1("Hub %p no memory for the USB4 port mapping\n", Hub);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Cursor = Buffer;
    Remaining = Total;
    Status = STATUS_SUCCESS;

    for (Entry = Hub->m_Mux.Ports.Flink; Entry != &Hub->m_Mux.Ports; Entry = Entry->Flink)
    {
        Port = CONTAINING_RECORD(Entry, HubPort, m_HubLink);
        if (Port->m_Usb4HostName == NULL)
            continue;

        WdfStringGetUnicodeString(Port->m_Usb4HostName, &Name);
        Status = RtlStringCbPrintfExW(Cursor, Remaining, &End, &Remaining, 0, L"%u#%wZ", Port->Number(), &Name);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Port %u USB4 port mapping for %wZ not formatted 0x%lx\n", Port->Number(), &Name, Status);
            goto Done;
        }

        /* This entry's NUL and the list's final NUL must still fit */
        if (Remaining < 2 * sizeof(WCHAR))
        {
            DPRINT1("Hub %p USB4 port mapping buffer overflowed\n", Hub);
            Status = STATUS_BUFFER_OVERFLOW;
            goto Done;
        }

        Cursor = End + 1;
        Remaining -= sizeof(WCHAR);
    }

    *Cursor = UNICODE_NULL;
    Used = (ULONG)((Cursor + 1 - Buffer) * sizeof(WCHAR));

    Status = IoSetDevicePropertyData(Hub->m_WdmPdo,
                                     &HubKeyUsb4PortMappings,
                                     LOCALE_NEUTRAL,
                                     0,
                                     DEVPROP_TYPE_STRING_LIST,
                                     Used,
                                     Buffer);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p USB4 port mapping property not written 0x%lx\n", Hub, Status);

Done:
    ExFreePoolWithTag(Buffer, HUB_TAG_HUB);
    return Status;
}

/* POWER COUPLING ************************************************************/

/* Without PoFx component relations the coupling to the host router's power PDO is only reported */
static
VOID
NTAPI
HubUsb4AddPowerRelation(
    _In_ HubFdo* Hub,
    _In_ HubUsb4Host* Host)
{
    DPRINT("Hub %p would depend on USB4 host router PDO %p; PoFx component relations are not available\n",
           Hub,
           Host->PowerPdo);
}

static
VOID
NTAPI
HubUsb4InvalidatePowerRelations(
    _In_ HubFdo* Hub)
{
    DPRINT("Hub %p power relations are not reported\n", Hub);
}

/* The caller holds the host's lock */
static
VOID
NTAPI
HubUsb4RemovePowerRelation(
    _In_ HubFdo* Hub,
    _In_ HubUsb4Host* Host)
{
    if (!Host->RelationAdded)
        return;

    DPRINT("Hub %p drops its dependency on USB4 host router PDO %p\n", Hub, Host->PowerPdo);
    Host->RelationAdded = FALSE;
}

/** A device behind a USB4 port holds its host router powered while tunneled or not known yet. */
BOOLEAN
NTAPI
HubUsb4NeedsHostPower(
    _In_ HubChild* Child)
{
    return Child->m_Port->HasFlag(PortFlag::Usb4Host) &&
           (Child->m_TunnelState == HUB_TUNNEL_STATE_TUNNELED ||
            Child->m_TunnelState == HUB_TUNNEL_STATE_UNKNOWN);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4AcquireHostPower(
    _In_ HubPort* Port)
{
    HubFdo* Hub = Port->m_Hub;
    HubUsb4Host* Host;

    PAGED_CODE();

    if (!Port->HasFlag(PortFlag::Usb4Host))
        return;

    Host = HubUsb4FindPortHost(Port);
    if (Host == NULL)
    {
        DPRINT1("Port %u has no USB4 host record\n", Port->Number());
        return;
    }

    HubWaitLockGuard Guard(Host->Lock);

    Host->PowerReferences += 1;
    if (Host->PowerReferences == 1 && Host->PowerPdo != NULL)
        HubUsb4AddPowerRelation(Hub, Host);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4ReleaseHostPower(
    _In_ HubPort* Port)
{
    HubFdo* Hub = Port->m_Hub;
    HubUsb4Host* Host;

    PAGED_CODE();

    if (!Port->HasFlag(PortFlag::Usb4Host))
        return;

    Host = HubUsb4FindPortHost(Port);
    if (Host == NULL)
        return;

    HubWaitLockGuard Guard(Host->Lock);

    Host->PowerReferences -= 1;
    if (Host->PowerReferences == 0 && Host->RelationAdded)
    {
        HubUsb4RemovePowerRelation(Hub, Host);
        HubUsb4InvalidatePowerRelations(Hub);
    }
}

/* A reference taken while the target was closed is applied once it opens */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubUsb4UpdatePowerRelations(
    _In_ HubFdo* Hub,
    _In_ HubUsb4Host* Host)
{
    {
        HubWaitLockGuard Guard(Host->Lock);

        if (Host->PowerReferences != 0 && !Host->RelationAdded)
            HubUsb4AddPowerRelation(Hub, Host);
    }

    HubUsb4InvalidatePowerRelations(Hub);
}

/* REMOTE TARGET *************************************************************/

/* Caller holds the host list lock. QUIRK: the power reference count is kept across a reopen. */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubUsb4CloseHost(
    _In_ HubUsb4TargetContext* Context)
{
    HubUsb4Host* Host = Context->Host;
    BOOLEAN Close = FALSE;

    if (Host == NULL)
        return;

    DPRINT("Hub %p closing USB4 host target %p\n", Context->Hub, Host->Target);

    {
        HubWaitLockGuard Guard(Host->Lock);

        HubUsb4RemovePowerRelation(Context->Hub, Host);
        Host->PowerPdo = NULL;

        if (Host->State != HubUsb4TargetState::Closing)
        {
            Host->State = HubUsb4TargetState::Closing;
            Close = TRUE;
        }
    }

    if (!Close)
        return;

    WdfIoTargetClose(Host->Target);

    HubWaitLockGuard Guard(Host->Lock);
    Host->State = HubUsb4TargetState::Closed;
}

static
NTSTATUS
NTAPI
HubUsb4TargetQueryRemove(
    _In_ WDFIOTARGET Target)
{
    DPRINT("USB4 host target %p query remove\n", Target);
    WdfIoTargetCloseForQueryRemove(Target);
    return STATUS_SUCCESS;
}

/* QUIRK: the target is not opened again; the next interface arrival does that */
static
VOID
NTAPI
HubUsb4TargetRemoveCanceled(
    _In_ WDFIOTARGET Target)
{
    DPRINT("USB4 host target %p remove canceled\n", Target);
}

static
VOID
NTAPI
HubUsb4TargetRemoveComplete(
    _In_ WDFIOTARGET Target)
{
    HubUsb4TargetContext* Context = HubUsb4GetTargetContext(Target);
    HubFdo* Hub = Context->Hub;

    DPRINT("USB4 host target %p removed\n", Target);

    {
        HubWaitLockGuard Guard(Hub->m_Usb4HostsLock);
        HubUsb4CloseHost(Context);
    }

    HubUsb4InvalidatePowerRelations(Hub);
}

/* The host router takes an open whose name ends in "USB-<anything>" as one from the USB hub */
static
VOID
NTAPI
HubUsb4OpenWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    HubUsb4OpenContext* Open = HubUsb4GetOpenContext(WorkItem);
    HubFdo* Hub = HubFdo::FromDevice((WDFDEVICE)WdfWorkItemGetParentObject(WorkItem));
    HubUsb4Host* Host = Open->Host;
    WCHAR Buffer[HUB_USB4_OPEN_NAME_CHARS];
    WDF_IO_TARGET_OPEN_PARAMS Params;
    UNICODE_STRING Name;
    NTSTATUS Status;

    RtlInitEmptyUnicodeString(&Name, Buffer, sizeof(Buffer));
    Status = RtlUnicodeStringPrintf(&Name, L"%wsUSB-%p", Open->SymbolicLink, Hub);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host target name not formatted 0x%lx\n", Hub, Status);
        goto Failed;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&Params, &Name, FILE_READ_DATA);
    Params.ShareAccess = FILE_SHARE_READ;
    Params.CreateOptions = FILE_NON_DIRECTORY_FILE;
    Params.EvtIoTargetQueryRemove = HubUsb4TargetQueryRemove;
    Params.EvtIoTargetRemoveCanceled = HubUsb4TargetRemoveCanceled;
    Params.EvtIoTargetRemoveComplete = HubUsb4TargetRemoveComplete;

    Status = WdfIoTargetOpen(Host->Target, &Params);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host target %wZ not opened 0x%lx\n", Hub, &Name, Status);
        goto Failed;
    }

    {
        HubWaitLockGuard ListGuard(Hub->m_Usb4HostsLock);
        HubWaitLockGuard HostGuard(Host->Lock);

        if (Host->State == HubUsb4TargetState::Opening)
            Host->State = HubUsb4TargetState::Open;

        Host->PowerPdo = WdfIoTargetWdmGetTargetPhysicalDevice(Host->Target);
        HubUsb4GetTargetContext(Host->Target)->Host = Host;
    }

    HubUsb4UpdatePowerRelations(Hub, Host);

    DPRINT("Hub %p opened USB4 host target %wZ, PDO %p\n", Hub, &Name, Host->PowerPdo);
    WdfObjectDelete(WorkItem);
    return;

Failed:
    {
        HubWaitLockGuard Guard(Host->Lock);
        Host->State = HubUsb4TargetState::Idle;
    }

    WdfObjectDelete(WorkItem);
}

/* Any failure puts the host back to idle so later arrivals are not ignored */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubUsb4QueueOpen(
    _In_ HubFdo* Hub,
    _In_ HubUsb4Host* Host,
    _In_ PCUNICODE_STRING SymbolicLink)
{
    WDF_WORKITEM_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    HubUsb4OpenContext* Open;
    WDFWORKITEM WorkItem;
    ULONG Chars;
    NTSTATUS Status;

    WDF_WORKITEM_CONFIG_INIT(&Config, HubUsb4OpenWorkItem);
    Config.AutomaticSerialization = TRUE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubUsb4OpenContext);
    Attributes.ParentObject = Hub->m_Device;

    Status = WdfWorkItemCreate(&Config, &Attributes, &WorkItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 host open work item not created 0x%lx\n", Hub, Status);
        goto Failed;
    }

    Open = HubUsb4GetOpenContext(WorkItem);
    Open->Host = Host;

    Chars = SymbolicLink->Length / sizeof(WCHAR);
    if ((SymbolicLink->Length & 1) != 0 ||
        SymbolicLink->Length > SymbolicLink->MaximumLength ||
        (SymbolicLink->Buffer == NULL && SymbolicLink->Length != 0))
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else if (Chars >= HUB_USB4_LINK_CHARS)
    {
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        RtlCopyMemory(Open->SymbolicLink, SymbolicLink->Buffer, SymbolicLink->Length);
        Open->SymbolicLink[Chars] = UNICODE_NULL;
        WdfWorkItemEnqueue(WorkItem);
        return;
    }

    DPRINT1("Hub %p USB4 host symbolic link %wZ not copied 0x%lx\n", Hub, SymbolicLink, Status);
    WdfObjectDelete(WorkItem);

Failed:
    HubWaitLockGuard Guard(Host->Lock);
    Host->State = HubUsb4TargetState::Idle;
}

/* INTERFACE WATCH ***********************************************************/

/**
 * @brief
 * A host router's power PDO came or went; its reference string picks the record by ACPI path or DVSEC name.
 */
static
NTSTATUS
NTAPI
HubUsb4InterfaceChange(
    _In_ PVOID NotificationStructure,
    _Inout_opt_ PVOID Context)
{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Change = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;
    HubFdo* Hub = HubFdo::FromDevice((WDFDEVICE)Context);
    WCHAR Reference[HUB_USB4_REFERENCE_CHARS];
    UNICODE_STRING AcpiName;
    UNICODE_STRING DvsecName;
    HubUsb4Host* Host = NULL;
    PWCHAR AcpiField;
    PWCHAR DvsecField = NULL;
    PWCHAR Mark;
    DEVPROPTYPE Type;
    ULONG Required;
    BOOLEAN Open = FALSE;
    NTSTATUS Status;

    PAGED_CODE();

    if (!IsEqualGUID(Change->InterfaceClassGuid, HubUsb4PowerPdoInterface))
    {
        DPRINT("Hub %p ignores an interface change of another class\n", Hub);
        return STATUS_SUCCESS;
    }

    if (HubDriver.GetInterfaceProperty == NULL)
    {
        DPRINT1("Hub %p cannot read USB4 power interface properties on this kernel\n", Hub);
        return STATUS_SUCCESS;
    }

    Status = HubDriver.GetInterfaceProperty(Change->SymbolicLinkName,
                                            &DEVPKEY_DeviceInterface_ReferenceString,
                                            LOCALE_NEUTRAL,
                                            0,
                                            sizeof(Reference),
                                            Reference,
                                            &Required,
                                            &Type);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 power interface reference string query failed 0x%lx\n", Hub, Status);
        return STATUS_SUCCESS;
    }

    if (Type != DEVPROP_TYPE_STRING)
    {
        DPRINT1("Hub %p USB4 power interface reference string has type 0x%lx\n", Hub, Type);
        return STATUS_SUCCESS;
    }

    Reference[RTL_NUMBER_OF(Reference) - 1] = UNICODE_NULL;

    if (_wcsnicmp(Reference, HUB_USB4_REFERENCE_PREFIX, HUB_USB4_REFERENCE_PREFIX_CHARS) != 0)
    {
        DPRINT1("Hub %p USB4 power interface has an invalid reference string %ws\n", Hub, Reference);
        return STATUS_SUCCESS;
    }

    Mark = wcschr(&Reference[HUB_USB4_REFERENCE_PREFIX_CHARS], HUB_USB4_FIELD_SEPARATOR);
    if (Mark == NULL)
    {
        DPRINT1("Hub %p USB4 power interface reference string %ws has no fields\n", Hub, Reference);
        return STATUS_SUCCESS;
    }

    AcpiField = Mark + 1;
    Mark = wcschr(AcpiField, HUB_USB4_FIELD_SEPARATOR);
    if (Mark != NULL)
    {
        *Mark = UNICODE_NULL;
        DvsecField = Mark + 1;

        Mark = wcschr(DvsecField, HUB_USB4_FIELD_SEPARATOR);
        if (Mark != NULL)
            *Mark = UNICODE_NULL;
    }

    RtlInitUnicodeString(&AcpiName, AcpiField);

    {
        HubWaitLockGuard Guard(Hub->m_Usb4HostsLock);

        if (AcpiName.Length != 0)
            Host = HubUsb4FindHost(Hub, &AcpiName);

        if (Host == NULL && DvsecField != NULL)
        {
            RtlInitUnicodeString(&DvsecName, DvsecField);
            Host = HubUsb4FindHost(Hub, &DvsecName);
        }
    }

    if (Host == NULL)
    {
        DPRINT1("Hub %p has no USB4 host for %ws\n", Hub, Reference);
        return STATUS_SUCCESS;
    }

    /* Removal is handled by the target's remove callbacks */
    if (!IsEqualGUID(Change->Event, GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        DPRINT("Hub %p USB4 host %p power interface removed\n", Hub, Host);
        return STATUS_SUCCESS;
    }

    DPRINT("Hub %p USB4 host %p power interface arrived\n", Hub, Host);

    {
        HubWaitLockGuard Guard(Host->Lock);

        if (Host->State == HubUsb4TargetState::Idle || Host->State == HubUsb4TargetState::Closed)
        {
            Host->State = HubUsb4TargetState::Opening;
            Open = TRUE;
        }
    }

    if (Open)
        HubUsb4QueueOpen(Hub, Host, Change->SymbolicLinkName);

    return STATUS_SUCCESS;
}

/* Registered from the hub's D0 entry since there is no PoFx callback */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4RegisterNotification(
    _In_ HubFdo* Hub)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (!Hub->m_HasUsb4Ports || Hub->m_Usb4Notification != NULL)
        return;

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&HubUsb4PowerPdoInterface,
                                            WdfDriverWdmGetDriverObject(WdfGetDriver()),
                                            HubUsb4InterfaceChange,
                                            Hub->m_Device,
                                            &Hub->m_Usb4Notification);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USB4 power interface notification not registered 0x%lx\n", Hub, Status);
        Hub->m_Usb4Notification = NULL;
    }
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4UnregisterNotification(
    _In_ HubFdo* Hub)
{
    PAGED_CODE();

    if (Hub->m_Usb4Notification == NULL)
        return;

    IoUnregisterPlugPlayNotification(Hub->m_Usb4Notification);
    Hub->m_Usb4Notification = NULL;
}

/* Hub cleanup: every opened host target is closed */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUsb4CloseAllHosts(
    _In_ HubFdo* Hub)
{
    HubUsb4Host* Host;
    ULONG Count;
    ULONG Index;

    PAGED_CODE();

    if (Hub->m_HasUsb4Ports && Hub->m_Usb4Hosts != NULL)
    {
        HubWaitLockGuard Guard(Hub->m_Usb4HostsLock);

        Count = WdfCollectionGetCount(Hub->m_Usb4Hosts);
        for (Index = 0; Index < Count; Index++)
        {
            Host = HubUsb4HostAt(Hub, Index);
            if (Host == NULL)
                break;

            if (Host->Target != NULL)
                HubUsb4CloseHost(HubUsb4GetTargetContext(Host->Target));
        }
    }

    HubUsb4InvalidatePowerRelations(Hub);
}
