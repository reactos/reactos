/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     QUERY_INTERFACE handlers for PDOs and filters
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <wdmguid.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

/* PCI root _OSC support and control DWORDs */
#define UACPINT_PCI_OSC_SUPPORT             0x0000001Fu
#define UACPINT_PCI_OSC_CONTROL             0x0000001Du

/* OS support DWORD for the platform wide \_SB._OSC */
#define UACPINT_PLATFORM_OSC_SUPPORT        0x00000B75u

/* _OSC status DWORD bits 1 to 3 report a rejected request */
#define UACPINT_OSC_ERROR_MASK              0x0000000Eu

/* PM1_EN bit that keeps PCI Express devices from waking the system */
#define UACPINT_PM1_EN_PCIEXP_WAKE_DIS      0x4000u

/* FADT flag that says PCIEXP_WAKE_DIS is implemented */
#define UACPINT_FADT_PCI_EXP_WAK            (1u << 14)

/*
 * pci.sys's copy ends at OscControlGranted on every release, newer WDKs
 * append more members, so never write past it.
 */
#define UACPINT_PCI_ROOT_CAPS_LENGTH \
    RTL_SIZEOF_THROUGH_FIELD(PCI_ROOT_BUS_HARDWARE_CAPABILITY, OscControlGranted)

C_ASSERT(UACPINT_PCI_ROOT_CAPS_LENGTH == 0x24);

/* The WDK only declares the reset interface from Windows 10 on */
#if (NTDDI_VERSION < NTDDI_WINTHRESHOLD)

#define DEVICE_RESET_INTERFACE_VERSION 1

typedef enum _DEVICE_RESET_TYPE
{
    FunctionLevelDeviceReset,
    PlatformLevelDeviceReset
} DEVICE_RESET_TYPE;

typedef
NTSTATUS
NTAPI
DEVICE_RESET_HANDLER(
    _In_ PVOID InterfaceContext,
    _In_ DEVICE_RESET_TYPE ResetType,
    _In_ ULONG Flags,
    _In_opt_ PVOID ResetParameters);

typedef DEVICE_RESET_HANDLER *PDEVICE_RESET_HANDLER;

typedef struct _DEVICE_RESET_INTERFACE_STANDARD
{
    USHORT                 Size;
    USHORT                 Version;
    PVOID                  Context;
    PINTERFACE_REFERENCE   InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    PDEVICE_RESET_HANDLER  DeviceReset;
    ULONG                  SupportedResetTypes;
    PVOID                  Reserved;
} DEVICE_RESET_INTERFACE_STANDARD, *PDEVICE_RESET_INTERFACE_STANDARD;

#endif

/* GUID_DEVICE_RESET_INTERFACE_STANDARD */
static const GUID UacpiNtDeviceResetGuid =
    { 0x649fdf26, 0x3bc0, 0x4813, { 0xad, 0x24, 0x7e, 0x0c, 0x1e, 0xda, 0x3f, 0xa3 } };

/*
 * Connected GPE, the opaque handle GpeConnectVector hands out
 */
typedef struct _UACPINT_GPE_CONNECTION
{
    ULONG                Index;
    PGPE_SERVICE_ROUTINE Service;
    PVOID                ServiceContext;
    KDPC                 Dpc;           ///< runs Service at DISPATCH_LEVEL
    WORK_QUEUE_ITEM      FinishItem;    ///< re-enables the GPE at PASSIVE_LEVEL
    volatile LONG        Busy;          ///< fired and not yet re-enabled
} UACPINT_GPE_CONNECTION, *PUACPINT_GPE_CONNECTION;

/*
 * Bus capability record returned by the PCI root _DSM function 4
 */
typedef struct _UACPINT_PCI_BUS_CAPS
{
    USHORT Type;
    USHORT Reserved;
    ULONG  Length;
    USHORT CapabilityId;
    USHORT CapabilityLength;
    ULONG  CurrentSpeedAndMode;
    ULONG  SupportedSpeedsAndModes;
    ULONG  Attributes;
} UACPINT_PCI_BUS_CAPS;

typedef UACPINT_PCI_BUS_CAPS UNALIGNED *PUACPINT_PCI_BUS_CAPS;

C_ASSERT(sizeof(UACPINT_PCI_BUS_CAPS) == 0x18);

/* Slots seen answering a vendor ID read, one bit per slot on buses 0 to 7 */
static UCHAR UacpiNtPciSlotPresent[8][256 / 8];

/* PM1_EN ports for ExpressWakeControl, 0 when absent */
/* Zeroed static storage is a released spin lock, queries must not reinitialize it */
static KSPIN_LOCK UacpiNtPm1EnableLock;
static USHORT UacpiNtPm1aEnablePort;
static USHORT UacpiNtPm1bEnablePort;

static
PUACPINT_FLT
NTAPI
UacpiNtFindFilterByNode(
    _In_opt_ PUACPINT_FDO Fdo,
    _In_ uacpi_namespace_node *Node)
{
    PUACPINT_FLT Found = NULL;
    PUACPINT_FLT Filter;
    PLIST_ENTRY Entry;

    if (!Fdo)
        return NULL;

    ExAcquireFastMutex(&Fdo->ChildLock);
    for (Entry = Fdo->FilterList.Flink; Entry != &Fdo->FilterList; Entry = Entry->Flink)
    {
        Filter = CONTAINING_RECORD(Entry, UACPINT_FLT, Link);
        if (Filter->Node == Node)
        {
            Found = Filter;
            break;
        }
    }
    ExReleaseFastMutex(&Fdo->ChildLock);

    return Found;
}

PUACPINT_PDO
NTAPI
UacpiNtFindPdoByNode(
    _In_opt_ PUACPINT_FDO Fdo,
    _In_ uacpi_namespace_node *Node)
{
    PUACPINT_PDO Found = NULL;
    PUACPINT_PDO Pdo;
    PLIST_ENTRY Entry;

    if (!Fdo)
        return NULL;

    ExAcquireFastMutex(&Fdo->ChildLock);
    for (Entry = Fdo->ChildList.Flink; Entry != &Fdo->ChildList; Entry = Entry->Flink)
    {
        Pdo = CONTAINING_RECORD(Entry, UACPINT_PDO, Link);
        if (Pdo->Node == Node)
        {
            Found = Pdo;
            break;
        }
    }
    ExReleaseFastMutex(&Fdo->ChildLock);

    return Found;
}

VOID
NTAPI
UacpiNtRouteNotify(
    _In_ uacpi_namespace_node *Node,
    _In_ ULONG Value)
{
    PUACPINT_FLT Filter;
    PUACPINT_PDO Pdo;

    Pdo = UacpiNtFindPdoByNode(GlobalAcpiFdo, Node);
    if (!Pdo)
    {
        /* Not one of ours, it may be a foreign PDO we filter */
        Filter = UacpiNtFindFilterByNode(GlobalAcpiFdo, Node);
        if (!Filter)
            return;

        if (Value == 2 && Filter->Wake.WaitWakeIrp)
            UacpiNtWakeComplete(&Filter->Wake);

        if (Filter->NotifyRoutine)
            Filter->NotifyRoutine(Filter->NotifyContext, Value);
        return;
    }

    /* Notify 2 is a device wake from the armed _PRW GPE */
    if (Value == 2 && Pdo->Wake.WaitWakeIrp)
        UacpiNtWakeComplete(&Pdo->Wake);

    /* 0x80 is a press or lid change, 0x02 a wake */
    if (Pdo->ButtonCaps != 0 && (Value == 0x80 || Value == 0x02))
        UacpiNtButtonNotify(Pdo, Value);

    /* 0x80 is a temperature change, 0x81 a trip point change */
    if (Pdo->IsThermalZone && (Value == 0x80 || Value == 0x81))
        UacpiNtThermalNotify(Pdo, Value);

    if (Pdo->NotifyRoutine)
        Pdo->NotifyRoutine(Pdo->NotifyContext, Value);
}

/* uacpi_finish_handling_gpe takes a mutex, so the re-enable runs here */
static
VOID
NTAPI
UacpiNtGpeFinishWorker(
    _In_ PVOID Parameter)
{
    PUACPINT_GPE_CONNECTION Connection = Parameter;

    (VOID)uacpi_finish_handling_gpe(uacpi_namespace_root(), (uacpi_u16)Connection->Index);
    InterlockedExchange(&Connection->Busy, 0);
}

static
VOID
NTAPI
UacpiNtGpeDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PUACPINT_GPE_CONNECTION Connection = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Consumers expect (GpeVectorObject, ServiceContext) like on acpi.sys */
    if (Connection->Service)
        Connection->Service(Connection, Connection->ServiceContext);

    ExQueueWorkItem(&Connection->FinishItem, DelayedWorkQueue);
}

static
uacpi_interrupt_ret
UacpiNtGpeHandler(
    _In_ uacpi_handle Context,
    _In_ uacpi_namespace_node *GpeDevice,
    _In_ uacpi_u16 Index)
{
    PUACPINT_GPE_CONNECTION Connection = Context;

    UNREFERENCED_PARAMETER(GpeDevice);
    UNREFERENCED_PARAMETER(Index);

    InterlockedExchange(&Connection->Busy, 1);
    KeInsertQueueDpc(&Connection->Dpc, NULL, NULL);

    /* No UACPI_GPE_REENABLE, the finish worker does that */
    return UACPI_INTERRUPT_HANDLED;
}

static
NTSTATUS
NTAPI
UacpiNtGpeConnectVector(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG GpeNumber,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN Shareable,
    _In_ PGPE_SERVICE_ROUTINE ServiceRoutine,
    _In_opt_ PVOID ServiceContext,
    _Out_opt_ PVOID ObjectContext)
{
    PUACPINT_GPE_CONNECTION Connection;
    uacpi_gpe_triggering Triggering;
    uacpi_status UacpiStatus;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Shareable);

    Connection = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Connection), UACPINT_POOL_TAG);
    if (!Connection)
        return STATUS_INSUFFICIENT_RESOURCES;

    Connection->Index = GpeNumber;
    Connection->Service = ServiceRoutine;
    Connection->ServiceContext = ServiceContext;
    Connection->Busy = 0;
    KeInitializeDpc(&Connection->Dpc, UacpiNtGpeDpc, Connection);
    ExInitializeWorkItem(&Connection->FinishItem, UacpiNtGpeFinishWorker, Connection);

    Triggering = (Mode == Latched) ? UACPI_GPE_TRIGGERING_EDGE : UACPI_GPE_TRIGGERING_LEVEL;
    UacpiStatus = uacpi_install_gpe_handler(uacpi_namespace_root(),
                                            (uacpi_u16)GpeNumber,
                                            Triggering,
                                            UacpiNtGpeHandler,
                                            Connection);
    if (uacpi_unlikely_error(UacpiStatus))
    {
        ExFreePoolWithTag(Connection, UACPINT_POOL_TAG);
        return STATUS_UNSUCCESSFUL;
    }

    /* ObjectContext is really the PVOID slot for the handle */
    if (ObjectContext)
        *(PVOID *)ObjectContext = Connection;

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UacpiNtGpeDisconnectVector(
    _In_ PVOID ObjectContext)
{
    PUACPINT_GPE_CONNECTION Connection = ObjectContext;
    LARGE_INTEGER Delay;

    if (!Connection)
        return STATUS_INVALID_PARAMETER;

    (VOID)uacpi_uninstall_gpe_handler(uacpi_namespace_root(),
                                      (uacpi_u16)Connection->Index,
                                      UacpiNtGpeHandler);

    /* Drain a GPE that already fired, first its DPC and then its re-enable */
    KeFlushQueuedDpcs();
    Delay.QuadPart = -10000;
    while (InterlockedCompareExchange(&Connection->Busy, 0, 0) != 0)
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);

    ExFreePoolWithTag(Connection, UACPINT_POOL_TAG);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UacpiNtGpeEnableEvent(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID ObjectContext)
{
    PUACPINT_GPE_CONNECTION Connection = ObjectContext;
    uacpi_status UacpiStatus;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!Connection)
        return STATUS_INVALID_PARAMETER;

    UacpiStatus = uacpi_enable_gpe(uacpi_namespace_root(), (uacpi_u16)Connection->Index);
    return uacpi_likely_success(UacpiStatus) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
UacpiNtGpeDisableEvent(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID ObjectContext)
{
    PUACPINT_GPE_CONNECTION Connection = ObjectContext;
    uacpi_status UacpiStatus;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!Connection)
        return STATUS_INVALID_PARAMETER;

    UacpiStatus = uacpi_disable_gpe(uacpi_namespace_root(), (uacpi_u16)Connection->Index);
    return uacpi_likely_success(UacpiStatus) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
UacpiNtGpeClearStatus(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID ObjectContext)
{
    PUACPINT_GPE_CONNECTION Connection = ObjectContext;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!Connection)
        return STATUS_INVALID_PARAMETER;

    (VOID)uacpi_clear_gpe(uacpi_namespace_root(), (uacpi_u16)Connection->Index);
    return STATUS_SUCCESS;
}

/* PDOs and filters both take Notify registrations, FALSE for anything else */
static
BOOLEAN
NTAPI
UacpiNtSetNotifyHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PDEVICE_NOTIFY_CALLBACK Routine,
    _In_opt_ PVOID Context)
{
    PUACPINT_SHARED Shared = DeviceObject->DeviceExtension;
    PUACPINT_FLT Filter;
    PUACPINT_PDO Pdo;

    switch (Shared->Type)
    {
        case UacpiNtDevObjPdo:
            Pdo = (PUACPINT_PDO)Shared;
            Pdo->NotifyRoutine = Routine;
            Pdo->NotifyContext = Context;
            return TRUE;

        case UacpiNtDevObjFilter:
            Filter = (PUACPINT_FLT)Shared;
            Filter->NotifyRoutine = Routine;
            Filter->NotifyContext = Context;
            return TRUE;

        default:
            return FALSE;
    }
}

static
NTSTATUS
NTAPI
UacpiNtRegisterNotify(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDEVICE_NOTIFY_CALLBACK NotificationHandler,
    _In_opt_ PVOID NotificationContext)
{
    if (!UacpiNtSetNotifyHandler(DeviceObject, NotificationHandler, NotificationContext))
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UacpiNtUnregisterNotify(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PDEVICE_NOTIFY_CALLBACK NotificationHandler)
{
    UNREFERENCED_PARAMETER(NotificationHandler);

    UacpiNtSetNotifyHandler(DeviceObject, NULL, NULL);
}

/* Version 2 slots whose argument lists differ from version 1 */
static
NTSTATUS
NTAPI
UacpiNtGpeDisconnectVector2(
    _In_ PVOID Context,
    _In_ PVOID ObjectContext)
{
    UNREFERENCED_PARAMETER(Context);

    return UacpiNtGpeDisconnectVector(ObjectContext);
}

static
VOID
NTAPI
UacpiNtUnregisterNotify2(
    _In_ PVOID Context)
{
    UacpiNtSetNotifyHandler((PDEVICE_OBJECT)Context, NULL, NULL);
}

/* Interfaces here are not reference counted */
static
VOID
NTAPI
UacpiNtInterfaceNop(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/*
 * Interrupt translator. Vector holds the GSIV on the way in, irqlib.c maps
 * it to a vector and writes the connection data.
 */
static
NTSTATUS
NTAPI
UacpiNtTranslateResources(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[],
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    KAFFINITY Affinity = 0;
    KIRQL Irql = PASSIVE_LEVEL;
    ULONG Polarity = 0;
    ULONG Vector = 0;
    ULONG Mode = 0;
    ULONG Count;
    ULONG Gsiv;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(AlternativesCount);
    UNREFERENCED_PARAMETER(Alternatives);

    *Target = *Source;

    /*
     * acpi.sys fails the reverse direction and the PnP manager fails the device
     * on that, so the identity copy stands until it is known to be reached.
     */
    if (Direction != TranslateChildToParent || Source->Type != CmResourceTypeInterrupt)
        return STATUS_SUCCESS;

    Gsiv = Source->u.Interrupt.Vector;

    DPRINT("irqTrans: PDO %p class 0x%04lx GSIV %lu flags 0x%x share %u%s\n",
           PhysicalDeviceObject,
           UacpiNtReadPciClassCode(PhysicalDeviceObject),
           Gsiv,
           Source->Flags,
           Source->ShareDisposition,
           (Source->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) ? " MSI" : " line");

    if ((Source->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) || Gsiv >= 0xFFF00000)
    {
        /* The message count is in the high USHORT of Level */
        Count = Source->u.Interrupt.Level >> 16;
        if (Count == 0 || Count > 32)
            Count = 1;

        Status = UacpiNtIrqLibResolveMessageVector(PhysicalDeviceObject,
                                                   Gsiv,
                                                   Count,
                                                   &Vector,
                                                   &Irql,
                                                   &Affinity);
    }
    else
    {
        /* A level descriptor makes the GSIV level triggered, active low */
        if (!(Source->Flags & CM_RESOURCE_INTERRUPT_LATCHED))
            UacpiNtIrqLibNoteLevelGsiv(Gsiv);

        Count = 1;
        Status = UacpiNtIrqLibResolveVector(Gsiv, &Vector, &Irql, &Affinity, &Polarity, &Mode);
    }

    /* acpi.sys leaves an unassigned descriptor alone, failing it fails the device */
    if (!NT_SUCCESS(Status) || Irql == PASSIVE_LEVEL)
    {
        DPRINT("irqTrans: GSIV 0x%lx x%lu has no vector yet\n", Gsiv, Count);
        return STATUS_SUCCESS;
    }

    Target->u.Interrupt.Level = Irql;
    Target->u.Interrupt.Vector = Vector;
    Target->u.Interrupt.Affinity = Affinity;

    if (PhysicalDeviceObject)
        UacpiNtIrqLibWriteConnectionData(PhysicalDeviceObject, Gsiv, Count);

    return STATUS_TRANSLATION_COMPLETE;
}

/* Requirements pass through, the arbiter works in GSIV space */
static
NTSTATUS
NTAPI
UacpiNtTranslateRequirements(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_writes_(*TargetCount) PIO_RESOURCE_DESCRIPTOR *Target)
{
    PIO_RESOURCE_DESCRIPTOR Copy;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    Copy = ExAllocatePoolWithTag(PagedPool, sizeof(*Copy), UACPINT_POOL_TAG);
    if (!Copy)
        return STATUS_INSUFFICIENT_RESOURCES;

    *Copy = *Source;
    *TargetCount = 1;
    *Target = Copy;
    return STATUS_TRANSLATION_COMPLETE;
}

NTSTATUS
NTAPI
UacpiNtBuildIrqTranslator(
    _In_z_ const char *TraceName,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PTRANSLATOR_INTERFACE Translator;

    Translator = (PTRANSLATOR_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
    if (IoStack->Parameters.QueryInterface.Size < sizeof(*Translator))
        return STATUS_BUFFER_TOO_SMALL;

    /* The PDO arrives with each call, so PDOs and filters share one Context */
    Translator->Size = sizeof(*Translator);
    Translator->Version = 1;
    Translator->Context = NULL;
    Translator->InterfaceReference = UacpiNtInterfaceNop;
    Translator->InterfaceDereference = UacpiNtInterfaceNop;
    Translator->TranslateResources = UacpiNtTranslateResources;
    Translator->TranslateResourceRequirements = UacpiNtTranslateRequirements;

    DPRINT("Interrupt translator provided to %s\n", TraceName);
    return STATUS_SUCCESS;
}

/* Logs PCI slots that stop answering vendor ID reads, safe at DISPATCH_LEVEL */
static
VOID
NTAPI
UacpiNtPciTrackPresence(
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _In_ ULONG Returned)
{
    USHORT VendorId;
    UCHAR Bit;

    if (Returned < Length && Length > 2)
    {
        DPRINT1("uACPI-NT: short PCI config read, bus %lu slot %lu.%lu offset 0x%lx length %lu returned %lu\n",
                Bus, Slot & 0x1F, (Slot >> 5) & 0x7, Offset, Length, Returned);
    }

    if (Offset != 0 || Length < 2 || Bus >= 8 || Slot >= 256)
        return;

    VendorId = *(USHORT UNALIGNED *)Buffer;
    Bit = (UCHAR)(1 << (Slot & 7));

    if (Returned >= 2 && VendorId != 0xFFFF && VendorId != 0)
    {
        UacpiNtPciSlotPresent[Bus][Slot >> 3] |= Bit;
    }
    else if (UacpiNtPciSlotPresent[Bus][Slot >> 3] & Bit)
    {
        DPRINT1("uACPI-NT: PCI device at bus %lu slot %lu.%lu vanished, vendor read returned %lu data 0x%04x\n",
                Bus, Slot & 0x1F, (Slot >> 5) & 0x7, Returned, VendorId);
        UacpiNtPciSlotPresent[Bus][Slot >> 3] &= (UCHAR)~Bit;
    }
}

/* Raw config cycles of Length bytes, pci.sys bugchecks on a short access */
static
ULONG
NTAPI
UacpiNtPciReadConfig(
    _In_ PVOID Context,
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    ULONG Returned;

    Returned = UacpiNtHalPciReadConfig(Context, Bus, Slot, Buffer, Offset, Length);
    UacpiNtPciTrackPresence(Bus, Slot, Buffer, Offset, Length, Returned);
    return Returned;
}

static
ULONG
NTAPI
UacpiNtPciWriteConfig(
    _In_ PVOID Context,
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    return UacpiNtHalPciWriteConfig(Context, Bus, Slot, Buffer, Offset, Length);
}

/* Interrupt Line fixups do nothing, _PRT owns interrupt routing */
static
VOID
NTAPI
UacpiNtPciPinToLine(
    _In_ PVOID Context,
    _In_ PPCI_COMMON_CONFIG PciData)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PciData);
}

static
VOID
NTAPI
UacpiNtPciLineToPin(
    _In_ PVOID Context,
    _In_ PPCI_COMMON_CONFIG PciNewData,
    _In_ PPCI_COMMON_CONFIG PciOldData)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PciNewData);
    UNREFERENCED_PARAMETER(PciOldData);
}

static
uacpi_object *
NTAPI
UacpiNtCreateBufferObject(
    _In_reads_bytes_(Length) const VOID *Data,
    _In_ SIZE_T Length)
{
    uacpi_data_view View;

    View.const_bytes = Data;
    View.length = Length;
    return uacpi_object_create_buffer(View);
}

/* TRUE when Object is a buffer, View then describes its contents */
static
BOOLEAN
NTAPI
UacpiNtGetBufferObject(
    _In_opt_ uacpi_object *Object,
    _Out_ uacpi_data_view *View)
{
    View->const_bytes = NULL;
    View->length = 0;

    if (!Object || uacpi_object_get_type(Object) != UACPI_OBJECT_BUFFER)
        return FALSE;

    return uacpi_likely_success(uacpi_object_get_buffer(Object, View));
}

/*
 * Evaluates Method with Args and drops the argument references. Result is
 * NULL on failure, otherwise the caller releases it.
 */
static
NTSTATUS
NTAPI
UacpiNtEvaluateWithArgs(
    _In_ uacpi_namespace_node *Node,
    _In_z_ const char *Method,
    _Inout_updates_(Count) uacpi_object **Args,
    _In_ ULONG Count,
    _Out_ uacpi_object **Result)
{
    uacpi_object_array Array;
    uacpi_status UacpiStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;

    *Result = NULL;

    for (i = 0; i < Count; i++)
    {
        if (!Args[i])
            Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(Status))
    {
        Array.objects = Args;
        Array.count = Count;

        UacpiStatus = uacpi_eval(Node, Method, &Array, Result);
        if (uacpi_unlikely_error(UacpiStatus))
        {
            if (*Result)
            {
                uacpi_object_unref(*Result);
                *Result = NULL;
            }

            if (UacpiStatus == UACPI_STATUS_NOT_FOUND)
                Status = STATUS_NOT_IMPLEMENTED;
            else
                Status = STATUS_UNSUCCESSFUL;
        }
    }

    for (i = 0; i < Count; i++)
    {
        if (Args[i])
            uacpi_object_unref(Args[i]);
    }

    return Status;
}

static
NTSTATUS
NTAPI
UacpiNtEvaluateOsc(
    _In_ uacpi_namespace_node *Node,
    _In_ const GUID *Uuid,
    _In_reads_(Count) const ULONG *Capabilities,
    _In_ ULONG Count,
    _Out_ uacpi_object **Result)
{
    uacpi_object *Args[4];

    Args[0] = UacpiNtCreateBufferObject(Uuid, sizeof(*Uuid));
    Args[1] = uacpi_object_create_integer(1);
    Args[2] = uacpi_object_create_integer(Count);
    Args[3] = UacpiNtCreateBufferObject(Capabilities, Count * sizeof(*Capabilities));

    return UacpiNtEvaluateWithArgs(Node, "_OSC", Args, RTL_NUMBER_OF(Args), Result);
}

/* Platform wide \_SB._OSC feature declaration, nothing happens without one */
VOID
NTAPI
UacpiNtPlatformOscNegotiate(VOID)
{
    uacpi_namespace_node *SbNode;
    uacpi_object *Result;
    uacpi_data_view View;
    ULONG Capabilities[2];
    ULONG OscStatus;

    SbNode = uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
    if (!SbNode)
        return;

    Capabilities[0] = 0;
    Capabilities[1] = UACPINT_PLATFORM_OSC_SUPPORT;

    if (!NT_SUCCESS(UacpiNtEvaluateOsc(SbNode,
                                       &UacpiNtSbOscUuid,
                                       Capabilities,
                                       RTL_NUMBER_OF(Capabilities),
                                       &Result)))
    {
        return;
    }

    if (UacpiNtGetBufferObject(Result, &View) && View.length >= sizeof(Capabilities))
    {
        OscStatus = *(const ULONG UNALIGNED *)View.const_bytes;
        DPRINT("\\_SB._OSC support 0x%lx status 0x%lx%s\n",
               Capabilities[1],
               OscStatus,
               (OscStatus & UACPINT_OSC_ERROR_MASK) ? " (rejected)" : "");
    }

    if (Result)
        uacpi_object_unref(Result);
}

/* One PCI root _OSC pass, Control is the request in and the grant out */
static
NTSTATUS
NTAPI
UacpiNtPciRootOscPass(
    _In_ PUACPINT_PDO Pdo,
    _In_ BOOLEAN Query,
    _Inout_ PULONG Control)
{
    uacpi_object *Result;
    uacpi_data_view View;
    ULONG Capabilities[3];
    NTSTATUS Status;

    /* Bit 0 of the status DWORD marks a query */
    Capabilities[0] = Query ? 1 : 0;
    Capabilities[1] = UACPINT_PCI_OSC_SUPPORT;
    Capabilities[2] = *Control;

    Status = UacpiNtEvaluateOsc(Pdo->Node,
                                &UacpiNtPciRootOscUuid,
                                Capabilities,
                                RTL_NUMBER_OF(Capabilities),
                                &Result);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = STATUS_UNSUCCESSFUL;
    if (UacpiNtGetBufferObject(Result, &View) && View.length >= sizeof(Capabilities))
    {
        RtlCopyMemory(Capabilities, View.const_bytes, sizeof(Capabilities));
        if (!(Capabilities[0] & UACPINT_OSC_ERROR_MASK))
        {
            *Control = Capabilities[2];
            Status = STATUS_SUCCESS;
        }
    }

    if (Result)
        uacpi_object_unref(Result);
    return Status;
}

static
VOID
NTAPI
UacpiNtPciRootOscNegotiate(
    _Inout_ PUACPINT_PDO Pdo)
{
    NTSTATUS Status;
    ULONG Control;

    if (Pdo->HostBridge.OscEvaluated)
        return;

    Pdo->HostBridge.OscEvaluated = TRUE;
    Pdo->HostBridge.OscControlGranted = 0;

    /* Query for the whole control set first, no _OSC grants nothing */
    Control = UACPINT_PCI_OSC_CONTROL;
    Status = UacpiNtPciRootOscPass(Pdo, TRUE, &Control);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_NOT_IMPLEMENTED)
            DPRINT1("%s: _OSC query failed 0x%lx\n", Pdo->Name, Status);
        return;
    }

    /* Commit, a query answer beyond the request commits nothing */
    if ((Control | UACPINT_PCI_OSC_CONTROL) != UACPINT_PCI_OSC_CONTROL)
        Control = 0;

    Status = UacpiNtPciRootOscPass(Pdo, FALSE, &Control);
    if (NT_SUCCESS(Status))
        Pdo->HostBridge.OscControlGranted = Control;

    DPRINT("%s: _OSC granted 0x%02lx\n", Pdo->Name, Pdo->HostBridge.OscControlGranted);
}

/* PCI root _DSM revision 1 with no function arguments */
static
NTSTATUS
NTAPI
UacpiNtPciRootDsm(
    _In_ PUACPINT_PDO Pdo,
    _In_ ULONG Function,
    _Out_ uacpi_object **Result)
{
    uacpi_object_array Empty = { 0 };
    uacpi_object *Args[4];

    Args[0] = UacpiNtCreateBufferObject(&UacpiNtPciRootDsmUuid, sizeof(UacpiNtPciRootDsmUuid));
    Args[1] = uacpi_object_create_integer(1);
    Args[2] = uacpi_object_create_integer(Function);
    Args[3] = uacpi_object_create_package(Empty);

    return UacpiNtEvaluateWithArgs(Pdo->Node, "_DSM", Args, RTL_NUMBER_OF(Args), Result);
}

/* TRUE when View holds a well formed bus capability record */
static
BOOLEAN
NTAPI
UacpiNtPciRootTakeBusCaps(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ const uacpi_data_view *View)
{
    PUACPINT_PCI_BUS_CAPS Caps;

    if (View->length < sizeof(*Caps))
        return FALSE;

    Caps = (PUACPINT_PCI_BUS_CAPS)View->const_bytes;
    if (Caps->Type != 1 ||
        Caps->Reserved != 0 ||
        Caps->Length < sizeof(*Caps) ||
        Caps->CapabilityId != 1 ||
        Caps->CapabilityLength < 0x10)
    {
        return FALSE;
    }

    Pdo->HostBridge.CurrentSpeedAndMode = Caps->CurrentSpeedAndMode;
    Pdo->HostBridge.SupportedSpeedsAndModes = Caps->SupportedSpeedsAndModes;
    Pdo->HostBridge.BusCapAttributes = Caps->Attributes;
    Pdo->HostBridge.BusCapsFound = TRUE;

    DPRINT("%s: _DSM bus caps current %lu supported 0x%lx attributes 0x%lx\n",
           Pdo->Name,
           Pdo->HostBridge.CurrentSpeedAndMode,
           Pdo->HostBridge.SupportedSpeedsAndModes,
           Pdo->HostBridge.BusCapAttributes);
    return TRUE;
}

static
VOID
NTAPI
UacpiNtPciRootDsmEvaluate(
    _Inout_ PUACPINT_PDO Pdo)
{
    uacpi_object_array Package;
    uacpi_object *Result;
    uacpi_data_view View;
    BOOLEAN Supported;
    NTSTATUS Status;
    uacpi_size i;

    if (Pdo->HostBridge.DsmEvaluated)
        return;

    Pdo->HostBridge.DsmEvaluated = TRUE;
    Pdo->HostBridge.BusCapsFound = FALSE;

    /* Function 0 is the supported function bitmap, bit 4 is the one needed */
    Status = UacpiNtPciRootDsm(Pdo, 0, &Result);
    Supported = NT_SUCCESS(Status) &&
                UacpiNtGetBufferObject(Result, &View) &&
                View.length != 0 &&
                (View.const_bytes[0] & 0x10);
    if (Result)
        uacpi_object_unref(Result);
    if (!Supported)
        return;

    /* Function 4 is a package holding the record, malformed data is ignored */
    Status = UacpiNtPciRootDsm(Pdo, 4, &Result);
    if (!NT_SUCCESS(Status) || !Result)
        return;

    if (uacpi_object_get_type(Result) == UACPI_OBJECT_PACKAGE &&
        uacpi_likely_success(uacpi_object_get_package(Result, &Package)))
    {
        for (i = 0; i < Package.count; i++)
        {
            if (UacpiNtGetBufferObject(Package.objects[i], &View) &&
                UacpiNtPciRootTakeBusCaps(Pdo, &View))
            {
                break;
            }
        }
    }

    uacpi_object_unref(Result);
}

/* pci.sys calls this at PASSIVE_LEVEL, so the lazy AML evaluation is fine */
static
VOID
NTAPI
UacpiNtPciRootBusCapability(
    _In_ PVOID Context,
    _Out_ PPCI_ROOT_BUS_HARDWARE_CAPABILITY HardwareCapability)
{
    PUACPINT_PDO Pdo = Context;

    RtlZeroMemory(HardwareCapability, UACPINT_PCI_ROOT_CAPS_LENGTH);

    UacpiNtPciRootOscNegotiate(Pdo);
    UacpiNtPciRootDsmEvaluate(Pdo);

    HardwareCapability->OscFeatureSupport.u.AsULONG = UACPINT_PCI_OSC_SUPPORT;
    HardwareCapability->OscControlRequest.u.AsULONG = UACPINT_PCI_OSC_CONTROL;
    HardwareCapability->OscControlGranted.u.AsULONG = Pdo->HostBridge.OscControlGranted;

    HardwareCapability->SecondaryInterface = Pdo->HostBridge.IsExpress ? PciExpress : PciConventional;
    if (HardwareCapability->SecondaryInterface == PciExpress || !Pdo->HostBridge.BusCapsFound)
        return;

    HardwareCapability->BusCapabilitiesFound = TRUE;
    HardwareCapability->CurrentSpeedAndMode = Pdo->HostBridge.CurrentSpeedAndMode;
    HardwareCapability->SupportedSpeedsAndModes = Pdo->HostBridge.SupportedSpeedsAndModes;

    if (Pdo->HostBridge.BusCapAttributes & 0x4)
        HardwareCapability->DeviceIDMessagingCapable = TRUE;

    if (Pdo->HostBridge.BusCapAttributes & 0x1)
        HardwareCapability->SecondaryBusWidth = BusWidth64Bits;
}

static
VOID
NTAPI
UacpiNtPciExpressWakeUpdate(
    _In_ USHORT Port,
    _In_ BOOLEAN EnableWake)
{
    USHORT Value;

    if (!Port)
        return;

    Value = READ_PORT_USHORT((PUSHORT)(ULONG_PTR)Port);
    if (EnableWake)
        Value = (USHORT)(Value & ~UACPINT_PM1_EN_PCIEXP_WAKE_DIS);
    else
        Value = (USHORT)(Value | UACPINT_PM1_EN_PCIEXP_WAKE_DIS);
    WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)Port, Value);
}

static
VOID
NTAPI
UacpiNtPciExpressWakeControl(
    _In_ PVOID Context,
    _In_ BOOLEAN EnableWake)
{
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Context);

    KeAcquireSpinLock(&UacpiNtPm1EnableLock, &OldIrql);
    UacpiNtPciExpressWakeUpdate(UacpiNtPm1aEnablePort, EnableWake);
    UacpiNtPciExpressWakeUpdate(UacpiNtPm1bEnablePort, EnableWake);
    KeReleaseSpinLock(&UacpiNtPm1EnableLock, OldIrql);
}

/* PCI_BUS_INTERFACE_STANDARD(2) for PCI roots, pci.sys needs it at AddDevice */
static
NTSTATUS
NTAPI
UacpiNtBuildPciBusInterface(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIO_STACK_LOCATION IoStack)
{
    static const uacpi_char *const ExpressIds[] = { "PNP0A08", UACPI_NULL };
    PPCI_BUS_INTERFACE_STANDARD BusInterface;
    struct acpi_fadt *Fadt = NULL;
    BOOLEAN ExpressWake = FALSE;
    BOOLEAN IsVersion2;
    uacpi_u64 BaseBus;
    ULONG Size;

    BusInterface = (PPCI_BUS_INTERFACE_STANDARD)IoStack->Parameters.QueryInterface.Interface;
    Size = IoStack->Parameters.QueryInterface.Size;

    /* Version 1 ends at ExpressWakeControl and the WDK only declares version 2 */
    IsVersion2 = IsEqualGUID(IoStack->Parameters.QueryInterface.InterfaceType,
                             &GUID_PCI_BUS_INTERFACE_STANDARD2);

    /* Everything through LineToPin is the minimum */
    if (Size < (ULONG)FIELD_OFFSET(PCI_BUS_INTERFACE_STANDARD, RootBusCapability))
        return STATUS_INVALID_PARAMETER;

    if (IoStack->Parameters.QueryInterface.Version > PCI_BUS_INTERFACE_STANDARD_VERSION)
        return STATUS_NOINTERFACE;

    /* No _BBN means bus 0 */
    if (uacpi_unlikely_error(uacpi_eval_simple_integer(Pdo->Node, "_BBN", &BaseBus)))
        BaseBus = 0;
    Pdo->PciRootBaseBus = (UCHAR)BaseBus;

    /* PNP0A08 in _HID or _CID marks a PCI Express root */
    if (_stricmp(Pdo->Hid, "PNP0A08") == 0)
        Pdo->HostBridge.IsExpress = TRUE;
    else
        Pdo->HostBridge.IsExpress = (uacpi_device_matches_pnp_id(Pdo->Node, ExpressIds) == UACPI_TRUE);

    UacpiNtPciRootOscNegotiate(Pdo);

    /* FADT PCI_EXP_WAK gates ExpressWakeControl */
    if (uacpi_likely_success(uacpi_table_fadt(&Fadt)) && Fadt)
    {
        if (Fadt->flags & UACPINT_FADT_PCI_EXP_WAK)
            ExpressWake = TRUE;

        /* PM1_EN is the upper half of each PM1 event block */
        if (Fadt->pm1a_evt_blk != 0 && Fadt->pm1_evt_len >= 4)
            UacpiNtPm1aEnablePort = (USHORT)(Fadt->pm1a_evt_blk + Fadt->pm1_evt_len / 2);
        if (Fadt->pm1b_evt_blk != 0 && Fadt->pm1_evt_len >= 4)
            UacpiNtPm1bEnablePort = (USHORT)(Fadt->pm1b_evt_blk + Fadt->pm1_evt_len / 2);
    }

    BusInterface->Size = (USHORT)FIELD_OFFSET(PCI_BUS_INTERFACE_STANDARD, RootBusCapability);
    BusInterface->Version = PCI_BUS_INTERFACE_STANDARD_VERSION;
    BusInterface->Context = Pdo;
    BusInterface->InterfaceReference = UacpiNtInterfaceNop;
    BusInterface->InterfaceDereference = UacpiNtInterfaceNop;
    BusInterface->ReadConfig = UacpiNtPciReadConfig;
    BusInterface->WriteConfig = UacpiNtPciWriteConfig;
    BusInterface->PinToLine = UacpiNtPciPinToLine;
    BusInterface->LineToPin = UacpiNtPciLineToPin;

    if (Size >= (ULONG)FIELD_OFFSET(PCI_BUS_INTERFACE_STANDARD, ExpressWakeControl))
    {
        BusInterface->Size = (USHORT)FIELD_OFFSET(PCI_BUS_INTERFACE_STANDARD, ExpressWakeControl);
        BusInterface->RootBusCapability = UacpiNtPciRootBusCapability;
    }

    /* NULL tells pci.sys ExpressWakeControl is not implemented */
    if (Size >= (ULONG)PCI_BUS_INTERFACE_STANDARD_VERSION_1_LENGTH)
    {
        if (ExpressWake)
        {
            BusInterface->Size = (USHORT)PCI_BUS_INTERFACE_STANDARD_VERSION_1_LENGTH;
            BusInterface->ExpressWakeControl = UacpiNtPciExpressWakeControl;
        }
        else
        {
            BusInterface->ExpressWakeControl = NULL;
        }
    }

    /* Not implemented, Size stops short of it and pci.sys checks for NULL */
    if (IsVersion2 && Size >= sizeof(*BusInterface))
        BusInterface->PrepareMultistageResume = NULL;

    DPRINT("PCI bus interface provided to %s, base bus %u%s\n",
           Pdo->Name,
           Pdo->PciRootBaseBus,
           Pdo->HostBridge.IsExpress ? ", express" : "");
    return STATUS_SUCCESS;
}

/* Sends a synchronous PnP IRP to the top of Target's stack */
static
NTSTATUS
NTAPI
UacpiNtSendPnpIrp(
    _In_ PDEVICE_OBJECT Target,
    _In_ PIO_STACK_LOCATION Stack)
{
    PIO_STACK_LOCATION NextStack;
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT TopDevice;
    NTSTATUS Status;
    KEVENT Event;
    PIRP Irp;

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    TopDevice = IoGetAttachedDeviceReference(Target);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, TopDevice, NULL, 0, NULL, &Event, &IoStatus);
    if (!Irp)
    {
        ObDereferenceObject(TopDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* PnP IRPs start out as not supported */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    NextStack = IoGetNextIrpStackLocation(Irp);
    *NextStack = *Stack;
    NextStack->CompletionRoutine = NULL;
    NextStack->Context = NULL;
    NextStack->Control = 0;

    Status = IoCallDriver(TopDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    ObDereferenceObject(TopDevice);
    return Status;
}

/* The filter that enumerated Pdo, else the root FDO with IsRoot set */
static
PDEVICE_OBJECT
NTAPI
UacpiNtPdoParentDevice(
    _In_ PUACPINT_PDO Pdo,
    _Out_ PBOOLEAN IsRoot)
{
    PUACPINT_FLT Filter = NULL;

    *IsRoot = TRUE;
    if (!Pdo->Parent)
        return NULL;

    if (Pdo->ScopeNode)
        Filter = UacpiNtFindFilterByNode(Pdo->Parent, Pdo->ScopeNode);

    if (Filter)
    {
        *IsRoot = FALSE;
        return Filter->Shared.Self;
    }

    return Pdo->Parent->Shared.Self;
}

/* Only a platform level reset belongs to firmware, it runs _RST */
static
NTSTATUS
NTAPI
UacpiNtDeviceReset(
    _In_ PVOID InterfaceContext,
    _In_ DEVICE_RESET_TYPE ResetType,
    _In_ ULONG Flags,
    _In_opt_ PVOID ResetParameters)
{
    uacpi_namespace_node *Node = InterfaceContext;
    uacpi_status UacpiStatus;

    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(ResetParameters);

    if (!Node)
        return STATUS_INVALID_PARAMETER;

    if (ResetType != PlatformLevelDeviceReset)
        return STATUS_NOT_SUPPORTED;

    UacpiStatus = uacpi_eval_simple(Node, "_RST", UACPI_NULL);
    DPRINT("_RST returned %d\n", (int)UacpiStatus);

    return uacpi_likely_success(UacpiStatus) ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

static
BOOLEAN
NTAPI
UacpiNtNodeHasReset(
    _In_opt_ uacpi_namespace_node *Node)
{
    uacpi_namespace_node *ResetNode = UACPI_NULL;

    if (!Node)
        return FALSE;

    return uacpi_likely_success(uacpi_namespace_node_find(Node, "_RST", &ResetNode)) &&
           ResetNode != UACPI_NULL;
}

NTSTATUS
NTAPI
UacpiNtBuildDeviceResetInterface(
    _In_ uacpi_namespace_node *Node,
    _In_z_ const char *TraceName,
    _In_ PIO_STACK_LOCATION IoStack)
{
    PDEVICE_RESET_INTERFACE_STANDARD ResetInterface;
    USHORT Size = IoStack->Parameters.QueryInterface.Size;

    if (!IsEqualGUID(IoStack->Parameters.QueryInterface.InterfaceType, &UacpiNtDeviceResetGuid))
        return STATUS_NOT_SUPPORTED;

    /* Nothing firmware can reset, let whoever else is asked answer */
    if (!UacpiNtNodeHasReset(Node))
        return STATUS_NOT_SUPPORTED;

    if (Size < FIELD_OFFSET(DEVICE_RESET_INTERFACE_STANDARD, SupportedResetTypes))
        return STATUS_BUFFER_TOO_SMALL;

    ResetInterface = (PDEVICE_RESET_INTERFACE_STANDARD)IoStack->Parameters.QueryInterface.Interface;
    RtlZeroMemory(ResetInterface, Size);
    ResetInterface->Size = Size;
    ResetInterface->Version = DEVICE_RESET_INTERFACE_VERSION;
    ResetInterface->Context = Node;
    ResetInterface->InterfaceReference = UacpiNtInterfaceNop;
    ResetInterface->InterfaceDereference = UacpiNtInterfaceNop;
    ResetInterface->DeviceReset = UacpiNtDeviceReset;

    if (Size >= RTL_SIZEOF_THROUGH_FIELD(DEVICE_RESET_INTERFACE_STANDARD, SupportedResetTypes))
        ResetInterface->SupportedResetTypes = 1u << PlatformLevelDeviceReset;

    DPRINT("%s: device reset interface provided (_RST)\n", TraceName);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UacpiNtBuildAcpiInterface(
    _In_ PDEVICE_OBJECT Context,
    _In_z_ const char *TraceName,
    _In_ PIO_STACK_LOCATION IoStack)
{
    const GUID *InterfaceType = IoStack->Parameters.QueryInterface.InterfaceType;
    PACPI_INTERFACE_STANDARD2 Interface2;
    PACPI_INTERFACE_STANDARD Interface;

    /* Context is typed PDEVICE_OBJECT and has to be the PDO */
    if (IsEqualGUID(InterfaceType, &GUID_ACPI_INTERFACE_STANDARD))
    {
        Interface = (PACPI_INTERFACE_STANDARD)IoStack->Parameters.QueryInterface.Interface;
        if (IoStack->Parameters.QueryInterface.Size < sizeof(*Interface))
            return STATUS_BUFFER_TOO_SMALL;

        Interface->Size = sizeof(*Interface);
        Interface->Version = 1;
        Interface->Context = Context;
        Interface->InterfaceReference = UacpiNtInterfaceNop;
        Interface->InterfaceDereference = UacpiNtInterfaceNop;
        Interface->GpeConnectVector = UacpiNtGpeConnectVector;
        Interface->GpeDisconnectVector = UacpiNtGpeDisconnectVector;
        Interface->GpeEnableEvent = UacpiNtGpeEnableEvent;
        Interface->GpeDisableEvent = UacpiNtGpeDisableEvent;
        Interface->GpeClearStatus = UacpiNtGpeClearStatus;
        Interface->RegisterForDeviceNotifications = UacpiNtRegisterNotify;
        Interface->UnregisterForDeviceNotifications = UacpiNtUnregisterNotify;

        DPRINT("ACPI interface provided to %s\n", TraceName);
        return STATUS_SUCCESS;
    }

    /* Same Context, only two slots need their own version 2 routines */
    if (IsEqualGUID(InterfaceType, &GUID_ACPI_INTERFACE_STANDARD2))
    {
        Interface2 = (PACPI_INTERFACE_STANDARD2)IoStack->Parameters.QueryInterface.Interface;
        if (IoStack->Parameters.QueryInterface.Size < sizeof(*Interface2))
            return STATUS_BUFFER_TOO_SMALL;

        Interface2->Size = sizeof(*Interface2);
        Interface2->Version = 2;
        Interface2->Context = Context;
        Interface2->InterfaceReference = UacpiNtInterfaceNop;
        Interface2->InterfaceDereference = UacpiNtInterfaceNop;
        Interface2->GpeConnectVector = (PGPE_CONNECT_VECTOR2)UacpiNtGpeConnectVector;
        Interface2->GpeDisconnectVector = UacpiNtGpeDisconnectVector2;
        Interface2->GpeEnableEvent = (PGPE_ENABLE_EVENT2)UacpiNtGpeEnableEvent;
        Interface2->GpeDisableEvent = (PGPE_DISABLE_EVENT2)UacpiNtGpeDisableEvent;
        Interface2->GpeClearStatus = (PGPE_CLEAR_STATUS2)UacpiNtGpeClearStatus;
        Interface2->RegisterForDeviceNotifications =
            (PREGISTER_FOR_DEVICE_NOTIFICATIONS2)UacpiNtRegisterNotify;
        Interface2->UnregisterForDeviceNotifications = UacpiNtUnregisterNotify2;

        DPRINT("ACPI interface 2 provided to %s\n", TraceName);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
UacpiNtPdoQueryInterface(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    const GUID *InterfaceType = IoStack->Parameters.QueryInterface.InterfaceType;
    IO_STACK_LOCATION Forward;
    PDEVICE_OBJECT Parent;
    BOOLEAN IsRoot;
    NTSTATUS Status;

    /* x64 IoGetDmaAdapter needs an answer, the parent stack has one */
    if (IsEqualGUID(InterfaceType, &GUID_BUS_INTERFACE_STANDARD))
    {
        Status = STATUS_NOINTERFACE;
        Parent = UacpiNtPdoParentDevice(Pdo, &IsRoot);
        if (Parent)
        {
            Forward = *IoStack;
            if (IsRoot)
                Forward.Parameters.QueryInterface.InterfaceSpecificData = Pdo->Shared.Self;
            Status = UacpiNtSendPnpIrp(Parent, &Forward);
        }

        DPRINT("%s: bus interface query sent to the %s stack, 0x%lx\n",
               Pdo->Name,
               IsRoot ? "root" : "filter",
               Status);
        Irp->IoStatus.Status = Status;
        return Status;
    }

    if (IsEqualGUID(InterfaceType, &GUID_ACPI_INTERFACE_STANDARD) ||
        IsEqualGUID(InterfaceType, &GUID_ACPI_INTERFACE_STANDARD2))
    {
        return UacpiNtBuildAcpiInterface(Pdo->Shared.Self, Pdo->Name, IoStack);
    }

    if ((IsEqualGUID(InterfaceType, &GUID_PCI_BUS_INTERFACE_STANDARD) ||
         IsEqualGUID(InterfaceType, &GUID_PCI_BUS_INTERFACE_STANDARD2)) &&
        UacpiNtHidIsPciRoot(Pdo->Hid))
    {
        return UacpiNtBuildPciBusInterface(Pdo, IoStack);
    }

    /* Memory, I/O and bus number arbiters, the IRQ ones live in fdo.c and filter.c */
    if (IsEqualGUID(InterfaceType, &GUID_ARBITER_INTERFACE_STANDARD))
    {
        Status = UacpiNtQueryResArbiter(Pdo, IoStack);
        if (Status != STATUS_NOT_SUPPORTED)
            return Status;
    }

    /* Not ours, the IRP status stays as it was */
    return Irp->IoStatus.Status;
}
