/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Interrupt (GSIV) arbiter and PCI interrupt routing
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <ndk/rtlfuncs.h>
#include <wdmguid.h>
#include "arbiter.h"
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <debug.h>

#ifndef CM_RESOURCE_INTERRUPT_MESSAGE
#define CM_RESOURCE_INTERRUPT_MESSAGE 0x0002
#endif

#ifndef PCI_CAPABILITY_ID_MSIX
#define PCI_CAPABILITY_ID_MSIX 0x11
#endif

/* Message interrupts are placed in a window above every wired GSIV */
#define UACPINT_MSI_GSIV_BASE   0xFFF00000ull
#define UACPINT_MSI_GSIV_LAST   0xFFFFFFFEull
#define UACPINT_MSI_MAX_COUNT   32
#define UACPINT_MSI_RECORD_MAX  64

/* Range attributes of a placed GSIV, telling how it is triggered */
#define UACPINT_RANGE_LEVEL     0x04
#define UACPINT_RANGE_LATCHED   0x08

/* Lines 0 to 15 are the legacy ISA inputs */
#define UACPINT_ISA_GSIV_COUNT  16

#define UACPINT_IRQ_LINK_MAX        32
#define UACPINT_IRQ_LINK_NONE       ((ULONG)-1)
#define UACPINT_LINK_CANDIDATE_MAX  16

/* Bridges a _PRT lookup climbs through before giving up */
#define UACPINT_PRT_MAX_DEPTH   8

#define UACPINT_IRQ_WRITTEN_MAX 256

/* PCI config dwords read by bus and slot */
#define UACPINT_PCI_CLASS_DWORD     FIELD_OFFSET(PCI_COMMON_CONFIG, RevisionID)
#define UACPINT_PCI_INTERRUPT_DWORD FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.InterruptLine)

/*
 * Arbiter WorkSpace: bits 0-7 hold the GetNextAllocationRange cursor,
 * the rest hold the _PRT result (0 unresolved, 1 unrouted, else GSIV + 2).
 */
#define UACPINT_WS_PRT_NONE     1
#define UACPINT_WS_PRT_BIAS     2
#define UACPINT_WS_CURSOR(Ws)   ((ULONG)((Ws) & 0xFF))
#define UACPINT_WS_PRT(Ws)      ((ULONG)((ULONG_PTR)(Ws) >> 8))
#define UACPINT_WS_SET_CURSOR(Ws, Value) \
    ((Ws) = ((Ws) & ~(ULONG_PTR)0xFF) | (ULONG_PTR)(Value))
#define UACPINT_WS_SET_PRT(Ws, Value) \
    ((Ws) = ((Ws) & (ULONG_PTR)0xFF) | ((ULONG_PTR)(Value) << 8))

/* GetNextAllocationRange cursor */
#define UACPINT_NEXT_INITIAL        0
#define UACPINT_NEXT_LINK_PREF      1
#define UACPINT_NEXT_STACK_UP       2   ///< unused stacking policy
#define UACPINT_NEXT_BOOT_CONFIG    3
#define UACPINT_NEXT_ALTERNATIVES   4

typedef struct _UACPINT_MSI_RECORD
{
    ULONGLONG Start;            ///< message window slot of the placement
    ULONG     MessageCount;
} UACPINT_MSI_RECORD, *PUACPINT_MSI_RECORD;

/* PCI interrupt link (PNP0C0F), decided once and cached */
typedef struct _UACPINT_IRQ_LINK
{
    uacpi_namespace_node *Node;
    ULONG                 Gsiv;     ///< UACPINT_IRQ_LINK_NONE until decided
} UACPINT_IRQ_LINK, *PUACPINT_IRQ_LINK;

/* Last connection data written for a device */
typedef struct _UACPINT_IRQ_WRITTEN
{
    PDEVICE_OBJECT Owner;
    ULONG          Start;
    ULONG          Length;
} UACPINT_IRQ_WRITTEN, *PUACPINT_IRQ_WRITTEN;

typedef struct _UACPINT_BRIDGE_CANDIDATE
{
    uacpi_namespace_node *Node;
    PDEVICE_OBJECT        Pdo;
} UACPINT_BRIDGE_CANDIDATE, *PUACPINT_BRIDGE_CANDIDATE;

typedef struct _UACPINT_MSI_DIAG_TARGET
{
    PDEVICE_OBJECT Pdo;
    CHAR           Name[8];
} UACPINT_MSI_DIAG_TARGET, *PUACPINT_MSI_DIAG_TARGET;

ULONG UacpiNtIrqArbEnabled = 1;
ULONG UacpiNtIrqArbVerbose = 1;
ULONG UacpiNtMsiDiagEnabled = 1;
ULONG UacpiNtMsiDiagDelaySeconds = 30;

static ARBITER_INSTANCE UacpiNtIrqArbiter;
static BOOLEAN UacpiNtIrqArbReady;
static PDEVICE_OBJECT UacpiNtIrqArbFdoSelf;

static UACPINT_MSI_RECORD UacpiNtMsiRecords[UACPINT_MSI_RECORD_MAX];
static ULONG UacpiNtMsiRecordCount;

static UACPINT_IRQ_LINK UacpiNtIrqLinks[UACPINT_IRQ_LINK_MAX];

/* Bumped per placed range so links spread over their _PRS choices */
static ULONG UacpiNtIrqLinkRotation;

/* PCIDeviceExclusionMask, ISA lines PCI links may not use */
static USHORT UacpiNtIrqPciExclusionMask;

static ULONG UacpiNtIrqSciGsiv = (ULONG)-1;

static UACPINT_IRQ_WRITTEN UacpiNtIrqWritten[UACPINT_IRQ_WRITTEN_MAX];
static ULONG UacpiNtIrqWrittenCount;

static KTIMER UacpiNtMsiDiagTimer;
static KDPC UacpiNtMsiDiagDpcObject;
static WORK_QUEUE_ITEM UacpiNtMsiDiagWork;
static LONG UacpiNtMsiDiagState;

static
VOID
NTAPI
UacpiNtMsiCountRecord(
    _In_ ULONGLONG Start,
    _In_ ULONG Count)
{
    ULONG Index;

    for (Index = 0; Index < UacpiNtMsiRecordCount; Index++)
    {
        if (UacpiNtMsiRecords[Index].Start == Start)
        {
            UacpiNtMsiRecords[Index].MessageCount = Count;
            return;
        }
    }

    if (UacpiNtMsiRecordCount >= UACPINT_MSI_RECORD_MAX)
    {
        DPRINT1("uACPI-NT: message record table full (%u), slot 0x%I64X not recorded\n",
                UACPINT_MSI_RECORD_MAX,
                Start);
        return;
    }

    UacpiNtMsiRecords[UacpiNtMsiRecordCount].Start = Start;
    UacpiNtMsiRecords[UacpiNtMsiRecordCount].MessageCount = Count;
    UacpiNtMsiRecordCount++;
}

static
NTSTATUS
NTAPI
UacpiNtMsiCountLookup(
    _In_ ULONGLONG Start,
    _Out_ PULONG Count)
{
    ULONG Index;

    *Count = 0;

    for (Index = 0; Index < UacpiNtMsiRecordCount; Index++)
    {
        if (UacpiNtMsiRecords[Index].Start == Start)
        {
            *Count = UacpiNtMsiRecords[Index].MessageCount;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

/* Message count asked for by the requirement span, 1 when out of range */
static
ULONG
NTAPI
UacpiNtMsiCountFromRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONGLONG Span;

    if (Descriptor->u.Interrupt.MaximumVector < Descriptor->u.Interrupt.MinimumVector)
        return 1;

    Span = (ULONGLONG)Descriptor->u.Interrupt.MaximumVector -
           Descriptor->u.Interrupt.MinimumVector + 1;
    if (Span == 0 || Span > UACPINT_MSI_MAX_COUNT)
        return 1;

    return (ULONG)Span;
}

static
NTSTATUS
NTAPI
UacpiNtIrqUnpackRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _Out_ PUINT64 Minimum,
    _Out_ PUINT64 Maximum,
    _Out_ PUINT64 Length,
    _Out_ PUINT64 Alignment)
{
    ULONGLONG Span = 0;
    ULONG Count;

    if (!Descriptor || Descriptor->Type != CmResourceTypeInterrupt)
        return STATUS_INVALID_PARAMETER;

    *Length = 1;
    *Alignment = 1;

    if (!(Descriptor->Flags & CM_RESOURCE_INTERRUPT_MESSAGE))
    {
        *Minimum = Descriptor->u.Interrupt.MinimumVector;
        *Maximum = Descriptor->u.Interrupt.MaximumVector;
        return STATUS_SUCCESS;
    }

    /* The PIC cannot deliver messages, an empty window sends it back to INTx */
    if (GlobalAcpiInterruptModel != 1)
    {
        DPRINT("uACPI-NT: message interrupt declined under the PIC model\n");
        *Minimum = 1;
        *Maximum = 0;
        return STATUS_SUCCESS;
    }

    /* A message request takes one slot, PackResource records the count */
    Count = UacpiNtMsiCountFromRequirement(Descriptor);
    if (Descriptor->u.Interrupt.MaximumVector >= Descriptor->u.Interrupt.MinimumVector)
    {
        Span = (ULONGLONG)Descriptor->u.Interrupt.MaximumVector -
               Descriptor->u.Interrupt.MinimumVector + 1;
    }

    DPRINT("uACPI-NT: message request flags 0x%X share %u min 0x%X max 0x%X span %u count %u%s\n",
           Descriptor->Flags,
           Descriptor->ShareDisposition,
           Descriptor->u.Interrupt.MinimumVector,
           Descriptor->u.Interrupt.MaximumVector,
           (ULONG)Span,
           Count,
           (Span > 1 && Count != (ULONG)Span) ? " (malformed span)" : "");

    *Minimum = UACPINT_MSI_GSIV_BASE;
    *Maximum = UACPINT_MSI_GSIV_LAST;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UacpiNtIrqPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR Requirement,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor)
{
    KAFFINITY Targeted;
    ULONG Count;

    if (!Requirement || !Descriptor)
        return STATUS_INVALID_PARAMETER;

    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->u.Interrupt.Vector = (ULONG)Start;

    if (Start < UACPINT_MSI_GSIV_BASE)
    {
        /* Raw line form, Level and Vector both carry the GSIV */
        Descriptor->u.Interrupt.Level = (ULONG)Start;
        Descriptor->u.Interrupt.Affinity = (KAFFINITY)-1;
        return STATUS_SUCCESS;
    }

    /* Raw message form, Level holds Group (0) low and MessageCount high */
    Count = UacpiNtMsiCountFromRequirement(Requirement);
    UacpiNtMsiCountRecord(Start, Count);

    Targeted = (KAFFINITY)Requirement->u.Interrupt.TargetedProcessors;

    Descriptor->Flags |= CM_RESOURCE_INTERRUPT_MESSAGE;
    Descriptor->u.Interrupt.Level = Count << 16;
    Descriptor->u.Interrupt.Affinity = Targeted ? Targeted : (KAFFINITY)-1;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
UacpiNtIrqUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT64 Length)
{
    ULONG Count;

    if (!Descriptor || Descriptor->Type != CmResourceTypeInterrupt)
        return STATUS_INVALID_PARAMETER;

    *Start = Descriptor->u.Interrupt.Vector;
    *Length = 1;

    if ((Descriptor->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) ||
        Descriptor->u.Interrupt.Vector >= UACPINT_MSI_GSIV_BASE)
    {
        Count = Descriptor->u.Interrupt.Level >> 16;
        if (Count != 0 && Count <= UACPINT_MSI_MAX_COUNT)
            *Length = Count;
    }

    return STATUS_SUCCESS;
}

/* Smaller ranges are arbitrated first, anything reaching past the ISA lines pays 5 */
static
INT32
NTAPI
UacpiNtIrqScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONG Minimum;
    ULONG Maximum;
    ULONG Score;

    if (!Descriptor || Descriptor->Type != CmResourceTypeInterrupt)
        return 0xFFFF;

    Minimum = Descriptor->u.Interrupt.MinimumVector;
    Maximum = Descriptor->u.Interrupt.MaximumVector;

    /* An inverted range wraps and lands on the clamp too */
    Score = Maximum - Minimum + 1;
    if (Maximum - Minimum == (ULONG)-1 || Score > 0xFFFF)
        Score = 0xFFFF;

    if (Maximum >= UACPINT_ISA_GSIV_COUNT)
        Score += 5;

    return (INT32)Score;
}

static
NTSTATUS
NTAPI
UacpiNtReadPciConfig(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    BUS_INTERFACE_STANDARD BusInterface;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT TopDevice;
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;

    RtlZeroMemory(&BusInterface, sizeof(BusInterface));
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    TopDevice = IoGetAttachedDeviceReference(Pdo);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, TopDevice, NULL, 0, NULL, &Event, &IoStatus);
    if (!Irp)
    {
        ObDereferenceObject(TopDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IoStack->Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
    IoStack->Parameters.QueryInterface.Size = sizeof(BusInterface);
    IoStack->Parameters.QueryInterface.Version = 1;
    IoStack->Parameters.QueryInterface.Interface = (PINTERFACE)&BusInterface;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(TopDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    ObDereferenceObject(TopDevice);

    if (!NT_SUCCESS(Status))
        return Status;

    if (!BusInterface.GetBusData)
    {
        Status = STATUS_NOT_SUPPORTED;
    }
    else if (BusInterface.GetBusData(BusInterface.Context,
                                     PCI_WHICHSPACE_CONFIG,
                                     Buffer,
                                     Offset,
                                     Length) != Length)
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (BusInterface.InterfaceDereference)
        BusInterface.InterfaceDereference(BusInterface.Context);

    return Status;
}

/* HAL config read by bus and slot, for paths where an IRP is not an option */
static
NTSTATUS
NTAPI
UacpiNtHalReadConfigDword(
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _In_ ULONG Offset,
    _Out_ PULONG Value)
{
    ULONG Read;

    *Value = 0;

    Read = HalGetBusDataByOffset(PCIConfiguration, Bus, Slot, Value, Offset, sizeof(*Value));

    return (Read == sizeof(*Value)) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/* PnP address (device high word, function low word) to a HAL slot number */
static
ULONG
NTAPI
UacpiNtPciSlotFromAddress(
    _In_ ULONG Address)
{
    PCI_SLOT_NUMBER Slot;

    Slot.u.AsULONG = 0;
    Slot.u.bits.DeviceNumber = (Address >> 16) & 0x1F;
    Slot.u.bits.FunctionNumber = Address & 0x7;
    return Slot.u.AsULONG;
}

ULONG
NTAPI
UacpiNtReadPciClassCode(
    _In_ PDEVICE_OBJECT Pdo)
{
    ULONG ClassDword = 0;

    if (!Pdo ||
        !NT_SUCCESS(UacpiNtReadPciConfig(Pdo,
                                         UACPINT_PCI_CLASS_DWORD,
                                         &ClassDword,
                                         sizeof(ClassDword))))
    {
        return 0xFFFF;
    }

    /* BaseClass << 8 | SubClass */
    return (((ClassDword >> 24) & 0xFF) << 8) | ((ClassDword >> 16) & 0xFF);
}

/* PCI root PDO whose _BBN (0 when absent) is Bus */
static
uacpi_namespace_node *
NTAPI
UacpiNtFindPciRootNode(
    _In_ ULONG Bus)
{
    uacpi_namespace_node *Roots[8];
    PUACPINT_PDO Child;
    PLIST_ENTRY Entry;
    uacpi_u64 BaseBus;
    ULONG Count = 0;
    ULONG Index;

    if (!GlobalAcpiFdo)
        return NULL;

    ExAcquireFastMutex(&GlobalAcpiFdo->ChildLock);
    for (Entry = GlobalAcpiFdo->ChildList.Flink;
         Entry != &GlobalAcpiFdo->ChildList && Count < RTL_NUMBER_OF(Roots);
         Entry = Entry->Flink)
    {
        Child = CONTAINING_RECORD(Entry, UACPINT_PDO, Link);
        if (UacpiNtHidIsPciRoot(Child->Hid))
            Roots[Count++] = Child->Node;
    }
    ExReleaseFastMutex(&GlobalAcpiFdo->ChildLock);

    /* _BBN runs AML, keep it out of the child lock */
    for (Index = 0; Index < Count; Index++)
    {
        if (uacpi_unlikely_error(uacpi_eval_simple_integer(Roots[Index], "_BBN", &BaseBus)))
            BaseBus = 0;

        if ((ULONG)BaseBus == Bus)
            return Roots[Index];
    }

    return NULL;
}

/* ACPI filtered PCI to PCI bridge whose secondary bus is Bus */
static
uacpi_namespace_node *
NTAPI
UacpiNtFindBridgeNodeForBus(
    _In_ ULONG Bus,
    _Out_ PDEVICE_OBJECT *BridgePdo)
{
    UACPINT_BRIDGE_CANDIDATE Candidates[32];
    PUACPINT_FLT Filter;
    PLIST_ENTRY Entry;
    UCHAR HeaderType;
    UCHAR SecondaryBus;
    ULONG Count = 0;
    ULONG Index;

    *BridgePdo = NULL;

    if (!GlobalAcpiFdo)
        return NULL;

    /* Config reads send IRPs, so snapshot the list first */
    ExAcquireFastMutex(&GlobalAcpiFdo->ChildLock);
    for (Entry = GlobalAcpiFdo->FilterList.Flink;
         Entry != &GlobalAcpiFdo->FilterList && Count < RTL_NUMBER_OF(Candidates);
         Entry = Entry->Flink)
    {
        Filter = CONTAINING_RECORD(Entry, UACPINT_FLT, Link);
        if (Filter->ForeignPdo && Filter->Node)
        {
            Candidates[Count].Node = Filter->Node;
            Candidates[Count].Pdo = Filter->ForeignPdo;
            Count++;
        }
    }
    ExReleaseFastMutex(&GlobalAcpiFdo->ChildLock);

    for (Index = 0; Index < Count; Index++)
    {
        HeaderType = 0;
        SecondaryBus = 0;

        if (!NT_SUCCESS(UacpiNtReadPciConfig(Candidates[Index].Pdo,
                                             FIELD_OFFSET(PCI_COMMON_CONFIG, HeaderType),
                                             &HeaderType,
                                             sizeof(HeaderType))))
        {
            continue;
        }

        if ((HeaderType & ~PCI_MULTIFUNCTION) != PCI_BRIDGE_TYPE)
            continue;

        if (!NT_SUCCESS(UacpiNtReadPciConfig(Candidates[Index].Pdo,
                                             FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SecondaryBus),
                                             &SecondaryBus,
                                             sizeof(SecondaryBus))))
        {
            continue;
        }

        if (SecondaryBus == Bus)
        {
            *BridgePdo = Candidates[Index].Pdo;
            return Candidates[Index].Node;
        }
    }

    return NULL;
}

static
VOID
NTAPI
UacpiNtIrqPolicyConfigure(VOID)
{
    HANDLE Key;
    ULONG Type;
    ULONG Mask = 0;
    ULONG Length = sizeof(Mask);

    if (!NT_SUCCESS(UacpiNtRegOpenKey(NULL,
                                      L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ACPI\\Parameters",
                                      KEY_READ,
                                      &Key)))
    {
        return;
    }

    if (NT_SUCCESS(UacpiNtRegQueryValue(Key, L"PCIDeviceExclusionMask", &Type, &Mask, &Length)) &&
        Type == REG_DWORD &&
        Length >= sizeof(USHORT))
    {
        UacpiNtIrqPciExclusionMask = (USHORT)Mask;
        DPRINT("uACPI-NT: PCI link exclusion mask 0x%04X\n", UacpiNtIrqPciExclusionMask);
    }

    ZwClose(Key);
}

static
PUACPINT_IRQ_LINK
NTAPI
UacpiNtLinkSlot(
    _In_ uacpi_namespace_node *Node,
    _In_ BOOLEAN Create)
{
    ULONG Index;

    for (Index = 0; Index < UACPINT_IRQ_LINK_MAX; Index++)
    {
        if (UacpiNtIrqLinks[Index].Node == Node)
            return &UacpiNtIrqLinks[Index];
    }

    if (!Create)
        return NULL;

    for (Index = 0; Index < UACPINT_IRQ_LINK_MAX; Index++)
    {
        if (!UacpiNtIrqLinks[Index].Node)
        {
            UacpiNtIrqLinks[Index].Node = Node;
            UacpiNtIrqLinks[Index].Gsiv = UACPINT_IRQ_LINK_NONE;
            return &UacpiNtIrqLinks[Index];
        }
    }

    return NULL;
}

/* The IRQs of the first interrupt descriptor in the link's _PRS */
static
ULONG
NTAPI
UacpiNtLinkCandidates(
    _In_ uacpi_namespace_node *Link,
    _Out_writes_to_(MaxCount, return) PULONG Candidates,
    _In_ ULONG MaxCount)
{
    uacpi_resources *Resources = NULL;
    uacpi_resource *Resource;
    ULONG Count = 0;
    ULONG Index;

    if (uacpi_unlikely_error(uacpi_get_possible_resources(Link, &Resources)) || !Resources)
        return 0;

    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        if (Resource->type == UACPI_RESOURCE_TYPE_IRQ)
        {
            for (Index = 0; Index < Resource->irq.num_irqs && Count < MaxCount; Index++)
                Candidates[Count++] = Resource->irq.irqs[Index];
            break;
        }

        if (Resource->type == UACPI_RESOURCE_TYPE_EXTENDED_IRQ)
        {
            for (Index = 0; Index < Resource->extended_irq.num_irqs && Count < MaxCount; Index++)
                Candidates[Count++] = Resource->extended_irq.irqs[Index];
            break;
        }
    }

    uacpi_free_resources(Resources);
    return Count;
}

/* _SRS the link onto Gsiv, with its own _CRS as the template */
static
NTSTATUS
NTAPI
UacpiNtLinkProgram(
    _In_ uacpi_namespace_node *Link,
    _In_ ULONG Gsiv)
{
    uacpi_resources *Resources = NULL;
    uacpi_resource *Resource;
    uacpi_status UacpiStatus;

    if (uacpi_unlikely_error(uacpi_get_current_resources(Link, &Resources)) || !Resources)
        return STATUS_NOT_FOUND;

    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        if (Resource->type == UACPI_RESOURCE_TYPE_IRQ && Resource->irq.num_irqs >= 1)
        {
            Resource->irq.num_irqs = 1;
            Resource->irq.irqs[0] = (uacpi_u8)Gsiv;
            break;
        }

        if (Resource->type == UACPI_RESOURCE_TYPE_EXTENDED_IRQ &&
            Resource->extended_irq.num_irqs >= 1)
        {
            Resource->extended_irq.num_irqs = 1;
            Resource->extended_irq.irqs[0] = Gsiv;
            break;
        }
    }

    UacpiStatus = uacpi_set_resources(Link, Resources);
    uacpi_free_resources(Resources);

    return uacpi_likely_success(UacpiStatus) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static
BOOLEAN
NTAPI
UacpiNtGsivHeldByLink(
    _In_ ULONG Gsiv)
{
    ULONG Index;

    for (Index = 0; Index < UACPINT_IRQ_LINK_MAX; Index++)
    {
        if (UacpiNtIrqLinks[Index].Node && UacpiNtIrqLinks[Index].Gsiv == Gsiv)
            return TRUE;
    }

    return FALSE;
}

static
BOOLEAN
NTAPI
UacpiNtLinkLineUsable(
    _In_ ULONG Gsiv,
    _In_ BOOLEAN AvoidOtherLinks)
{
    if (Gsiv == UacpiNtIrqSciGsiv)
        return FALSE;

    if (Gsiv < UACPINT_ISA_GSIV_COUNT && (UacpiNtIrqPciExclusionMask & (1u << Gsiv)))
        return FALSE;

    if (AvoidOtherLinks && UacpiNtGsivHeldByLink(Gsiv))
        return FALSE;

    return TRUE;
}

/*
 * Rotated pick from _PRS. The first pass avoids lines other links hold,
 * the second takes any allowed line.
 */
static
BOOLEAN
NTAPI
UacpiNtLinkChoose(
    _In_ uacpi_namespace_node *Link,
    _Out_ PULONG Chosen)
{
    ULONG Candidates[UACPINT_LINK_CANDIDATE_MAX];
    ULONG Count;
    ULONG Index;
    ULONG Gsiv;
    ULONG Pass;

    Count = UacpiNtLinkCandidates(Link, Candidates, RTL_NUMBER_OF(Candidates));
    if (Count == 0)
        return FALSE;

    for (Pass = 0; Pass < 2; Pass++)
    {
        for (Index = 0; Index < Count; Index++)
        {
            Gsiv = Candidates[(UacpiNtIrqLinkRotation + Index) % Count];
            if (UacpiNtLinkLineUsable(Gsiv, (BOOLEAN)(Pass == 0)))
            {
                *Chosen = Gsiv;
                return TRUE;
            }
        }
    }

    return FALSE;
}

/* The Index-th interrupt descriptor's first line in the link's _CRS */
static
BOOLEAN
NTAPI
UacpiNtLinkCurrentGsiv(
    _In_ uacpi_namespace_node *Link,
    _In_ ULONG Index,
    _Out_ PULONG Gsiv)
{
    uacpi_resources *Resources = NULL;
    uacpi_resource *Resource;
    BOOLEAN Found = FALSE;
    ULONG Seen = 0;

    if (uacpi_unlikely_error(uacpi_get_current_resources(Link, &Resources)) || !Resources)
        return FALSE;

    for (Resource = Resources->entries;
         Resource->type != UACPI_RESOURCE_TYPE_END_TAG;
         Resource = UACPI_NEXT_RESOURCE(Resource))
    {
        if (Resource->type == UACPI_RESOURCE_TYPE_IRQ)
        {
            if (Seen++ == Index && Resource->irq.num_irqs > 0)
            {
                *Gsiv = Resource->irq.irqs[0];
                Found = TRUE;
                break;
            }
        }
        else if (Resource->type == UACPI_RESOURCE_TYPE_EXTENDED_IRQ)
        {
            if (Seen++ == Index && Resource->extended_irq.num_irqs > 0)
            {
                *Gsiv = Resource->extended_irq.irqs[0];
                Found = TRUE;
                break;
            }
        }
    }

    uacpi_free_resources(Resources);
    return Found;
}

BOOLEAN
NTAPI
UacpiNtIrqLinkDecide(
    _In_ uacpi_namespace_node *Node,
    _Out_ PULONG Gsiv)
{
    PUACPINT_IRQ_LINK Slot;
    ULONG Current;
    ULONG Chosen = 0;

    Slot = UacpiNtLinkSlot(Node, TRUE);
    if (!Slot)
        return FALSE;

    if (Slot->Gsiv != UACPINT_IRQ_LINK_NONE)
    {
        *Gsiv = Slot->Gsiv;
        return TRUE;
    }

    if (!UacpiNtLinkCurrentGsiv(Node, 0, &Current))
        Current = UACPINT_IRQ_LINK_NONE;

    /* Without a usable _PRS the firmware line stands */
    if (!UacpiNtLinkChoose(Node, &Chosen))
    {
        if (Current == UACPINT_IRQ_LINK_NONE)
            return FALSE;
        Chosen = Current;
    }

    if (Chosen != Current)
    {
        if (NT_SUCCESS(UacpiNtLinkProgram(Node, Chosen)))
        {
            DPRINT("uACPI-NT: link %p moved from GSIV %u to %u (rotation %u)\n",
                   Node,
                   Current,
                   Chosen,
                   UacpiNtIrqLinkRotation);
        }
        else
        {
            DPRINT1("uACPI-NT: link %p _SRS to GSIV %u failed, staying on %u\n",
                    Node,
                    Chosen,
                    Current);
            if (Current == UACPINT_IRQ_LINK_NONE)
                return FALSE;
            Chosen = Current;
        }
    }

    Slot->Gsiv = Chosen;
    *Gsiv = Chosen;
    return TRUE;
}

/*
 * Firmware can put links back on their power on lines across S1 to S4.
 * Under APIC routing the HAL restores the redirection entries, under the
 * PIC the link registers are ours to restore.
 */
VOID
NTAPI
UacpiNtIrqLinksResume(VOID)
{
    PUACPINT_IRQ_LINK Slot;
    ULONG Index;

    if (GlobalAcpiInterruptModel != 0)
        return;

    for (Index = 0; Index < UACPINT_IRQ_LINK_MAX; Index++)
    {
        Slot = &UacpiNtIrqLinks[Index];
        if (!Slot->Node || Slot->Gsiv == UACPINT_IRQ_LINK_NONE)
            continue;

        if (NT_SUCCESS(UacpiNtLinkProgram(Slot->Node, Slot->Gsiv)))
            DPRINT("uACPI-NT: link %p restored to GSIV %u\n", Slot->Node, Slot->Gsiv);
        else
            DPRINT1("uACPI-NT: link %p restore to GSIV %u failed\n", Slot->Node, Slot->Gsiv);
    }
}

/* Only source index 0 is a steerable link, others are read from _CRS as is */
static
NTSTATUS
NTAPI
UacpiNtLinkNodeGsiv(
    _In_ uacpi_namespace_node *Link,
    _In_ ULONG Index,
    _Out_ PULONG Gsiv)
{
    BOOLEAN Found;

    if (Index != 0)
        Found = UacpiNtLinkCurrentGsiv(Link, Index, Gsiv);
    else
        Found = UacpiNtIrqLinkDecide(Link, Gsiv);

    return Found ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

static
VOID
NTAPI
UacpiNtPrtDump(
    _In_ uacpi_namespace_node *Node,
    _In_ uacpi_pci_routing_table *Prt,
    _In_ ULONG Device)
{
    uacpi_pci_routing_table_entry *Entry;
    uacpi_size Index;

    DPRINT("uACPI-NT: node %p _PRT has %u entries, matches for device %u:\n",
           Node,
           (ULONG)Prt->num_entries,
           Device);

    for (Index = 0; Index < Prt->num_entries; Index++)
    {
        Entry = &Prt->entries[Index];
        if ((Entry->address >> 16) != Device)
            continue;

        DPRINT("uACPI-NT:   address 0x%X pin INT%c source %p index %u\n",
               (ULONG)Entry->address,
               (char)('A' + (Entry->pin & 3)),
               Entry->source,
               (ULONG)Entry->index);
    }
}

/* Look up Device and Pin (1 based) in Node's _PRT, whose pins are 0 based */
static
NTSTATUS
NTAPI
UacpiNtCrackPrt(
    _In_ uacpi_namespace_node *Node,
    _In_ ULONG Device,
    _In_ UCHAR Pin,
    _Out_ PULONG Gsiv)
{
    uacpi_pci_routing_table *Prt = NULL;
    uacpi_pci_routing_table_entry *Entry;
    NTSTATUS Status = STATUS_NOT_FOUND;
    uacpi_size Index;
    ULONG Function;

    if (uacpi_unlikely_error(uacpi_get_pci_routing_table(Node, &Prt)) || !Prt)
    {
        if (UacpiNtIrqArbVerbose)
        {
            DPRINT("uACPI-NT: node %p has no _PRT (device %u INT%c)\n",
                   Node,
                   Device,
                   (char)('A' + (Pin - 1)));
        }
        return STATUS_NOT_FOUND;
    }

    if (UacpiNtIrqArbVerbose)
        UacpiNtPrtDump(Node, Prt, Device);

    for (Index = 0; Index < Prt->num_entries; Index++)
    {
        Entry = &Prt->entries[Index];

        /* Device in the high word, only the all functions forms match */
        Function = (ULONG)(Entry->address & 0xFFFF);
        if ((Entry->address >> 16) != Device || (Function != 0xFFFF && Function != 0))
            continue;

        if (Entry->pin != (uacpi_u8)(Pin - 1))
            continue;

        if (Entry->source)
        {
            Status = UacpiNtLinkNodeGsiv(Entry->source, Entry->index, Gsiv);
        }
        else
        {
            *Gsiv = Entry->index;
            Status = STATUS_SUCCESS;
        }
        break;
    }

    uacpi_free_pci_routing_table(Prt);
    return Status;
}

/*
 * Move one bridge up after a _PRT miss on the bridge. A PCI to PCI bridge
 * swizzles the pin, a CardBus bridge raises its own pin.
 */
static
NTSTATUS
NTAPI
UacpiNtPrtClimbBridge(
    _In_ PDEVICE_OBJECT BridgePdo,
    _Inout_ PULONG Bus,
    _Inout_ PULONG Device,
    _Inout_ PUCHAR Pin)
{
    ULONG BridgeAddress = 0;
    ULONG ClassDword = 0;
    ULONG PinDword = 0;
    ULONG Length;
    ULONG Slot;
    UCHAR SubClass;
    UCHAR NewPin;

    if (!NT_SUCCESS(IoGetDeviceProperty(BridgePdo,
                                        DevicePropertyBusNumber,
                                        sizeof(*Bus),
                                        Bus,
                                        &Length)) ||
        !NT_SUCCESS(IoGetDeviceProperty(BridgePdo,
                                        DevicePropertyAddress,
                                        sizeof(BridgeAddress),
                                        &BridgeAddress,
                                        &Length)))
    {
        return STATUS_NOT_FOUND;
    }

    Slot = UacpiNtPciSlotFromAddress(BridgeAddress);

    if (!NT_SUCCESS(UacpiNtHalReadConfigDword(*Bus, Slot, UACPINT_PCI_CLASS_DWORD, &ClassDword)))
    {
        if (UacpiNtIrqArbVerbose)
            DPRINT("uACPI-NT: bridge on bus %u class read failed\n", *Bus);
        return STATUS_NOT_FOUND;
    }

    SubClass = (UCHAR)(ClassDword >> 16);
    switch (SubClass)
    {
        case PCI_SUBCLASS_BR_PCI_TO_PCI:
            NewPin = (UCHAR)(((*Device + (*Pin - 1)) & 3) + 1);
            break;

        case PCI_SUBCLASS_BR_CARDBUS:
            if (!NT_SUCCESS(UacpiNtHalReadConfigDword(*Bus,
                                                      Slot,
                                                      UACPINT_PCI_INTERRUPT_DWORD,
                                                      &PinDword)))
            {
                return STATUS_NOT_FOUND;
            }

            NewPin = (UCHAR)(PinDword >> 8);
            if (NewPin == 0 || NewPin > 4)
                return STATUS_NOT_FOUND;
            break;

        default:
            if (UacpiNtIrqArbVerbose)
                DPRINT("uACPI-NT: bridge on bus %u subclass 0x%02X does not route\n", *Bus, SubClass);
            return STATUS_NOT_FOUND;
    }

    if (UacpiNtIrqArbVerbose)
    {
        DPRINT("uACPI-NT: bridge subclass %u %s device %u INT%c to INT%c, up to bus %u device %u\n",
               SubClass,
               (SubClass == PCI_SUBCLASS_BR_CARDBUS) ? "bridge pin" : "swizzle",
               *Device,
               (char)('A' + (*Pin - 1)),
               (char)('A' + (NewPin - 1)),
               *Bus,
               BridgeAddress >> 16);
    }

    *Pin = NewPin;
    *Device = BridgeAddress >> 16;
    return STATUS_SUCCESS;
}

/* Routed GSIV of a PCI function, climbing bridges until a _PRT answers */
static
NTSTATUS
NTAPI
UacpiNtPrtResolveGsiv(
    _In_ PDEVICE_OBJECT DevicePdo,
    _Out_ PULONG Gsiv)
{
    uacpi_namespace_node *Producer;
    PDEVICE_OBJECT BridgePdo;
    NTSTATUS Status;
    ULONG Bus = 0;
    ULONG Address = 0;
    ULONG Length = 0;
    ULONG PinDword = 0;
    ULONG Device;
    ULONG Depth;
    UCHAR Pin;

    Status = IoGetDeviceProperty(DevicePdo, DevicePropertyBusNumber, sizeof(Bus), &Bus, &Length);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IoGetDeviceProperty(DevicePdo, DevicePropertyAddress, sizeof(Address), &Address, &Length);
    if (!NT_SUCCESS(Status))
        return Status;

    Device = Address >> 16;

    /* The pin comes from the HAL, a bus interface IRP can fail this early */
    if (!NT_SUCCESS(UacpiNtHalReadConfigDword(Bus,
                                              UacpiNtPciSlotFromAddress(Address),
                                              UACPINT_PCI_INTERRUPT_DWORD,
                                              &PinDword)))
    {
        return STATUS_NOT_FOUND;
    }

    Pin = (UCHAR)(PinDword >> 8);
    if (Pin == 0 || Pin > 4)
        return STATUS_NOT_FOUND;

    if (UacpiNtIrqArbVerbose)
    {
        DPRINT("uACPI-NT: routing PDO %p bus %u device %u function %u INT%c\n",
               DevicePdo,
               Bus,
               Device,
               Address & 0xFFFF,
               (char)('A' + (Pin - 1)));
    }

    Status = STATUS_NOT_FOUND;
    for (Depth = 0; Depth < UACPINT_PRT_MAX_DEPTH; Depth++)
    {
        Producer = UacpiNtFindPciRootNode(Bus);
        if (Producer)
        {
            Status = UacpiNtCrackPrt(Producer, Device, Pin, Gsiv);
            if (UacpiNtIrqArbVerbose)
            {
                DPRINT("uACPI-NT: root bus %u _PRT device %u INT%c status 0x%lx GSIV %u\n",
                       Bus,
                       Device,
                       (char)('A' + (Pin - 1)),
                       Status,
                       NT_SUCCESS(Status) ? *Gsiv : 0);
            }
            break;
        }

        Producer = UacpiNtFindBridgeNodeForBus(Bus, &BridgePdo);
        if (!Producer || !BridgePdo)
        {
            if (UacpiNtIrqArbVerbose)
                DPRINT("uACPI-NT: no ACPI bridge produces bus %u\n", Bus);
            return STATUS_NOT_FOUND;
        }

        /* The bridge may route its secondary bus itself */
        Status = UacpiNtCrackPrt(Producer, Device, Pin, Gsiv);
        if (NT_SUCCESS(Status))
        {
            if (UacpiNtIrqArbVerbose)
            {
                DPRINT("uACPI-NT: bridge for bus %u _PRT device %u INT%c GSIV %u\n",
                       Bus,
                       Device,
                       (char)('A' + (Pin - 1)),
                       *Gsiv);
            }
            break;
        }

        if (!NT_SUCCESS(UacpiNtPrtClimbBridge(BridgePdo, &Bus, &Device, &Pin)))
            return STATUS_NOT_FOUND;
    }

    /* _PRT routed lines are level triggered, active low */
    if (NT_SUCCESS(Status))
        UacpiNtIrqLibNoteLevelGsiv(*Gsiv);

    return Status;
}

/*
 * A GSIV already placed as a shared level triggered line may be joined whatever
 * the new requirement's disposition says, because every device on the line sees
 * the same assertion. An edge triggered one may not: the second device would
 * lose its edges.
 */
static
BOOLEAN
NTAPI
UacpiNtIrqShareableRange(
    _In_ PVOID Context,
    _In_ PRTL_RANGE Range)
{
    UNREFERENCED_PARAMETER(Context);

    return (Range->Attributes & UACPINT_RANGE_LEVEL) && (Range->Flags & RTL_RANGE_SHARED);
}

/* Resolve the _PRT line once per entry. Boot reservations keep the firmware IRQ. */
static
NTSTATUS
NTAPI
UacpiNtIrqPreprocessEntry(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    ULONG_PTR WorkSpace;
    ULONG Gsiv;

    UNREFERENCED_PARAMETER(Arbiter);

    if (!State->Entry || !State->Entry->PhysicalDeviceObject)
        return STATUS_SUCCESS;

    /* Tag the triggering so UacpiNtIrqShareableRange can tell the two apart */
    State->RangeAttributes &= ~(UACPINT_RANGE_LEVEL | UACPINT_RANGE_LATCHED);
    if (State->Alternatives &&
        (State->Alternatives->Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED))
    {
        State->RangeAttributes |= UACPINT_RANGE_LATCHED;
    }
    else
    {
        State->RangeAttributes |= UACPINT_RANGE_LEVEL;
    }

    if (State->Flags & ARBITER_STATE_FLAG_BOOT)
        return STATUS_SUCCESS;

    if (UACPINT_WS_PRT(State->WorkSpace) != 0)
        return STATUS_SUCCESS;

    WorkSpace = State->WorkSpace;
    if (NT_SUCCESS(UacpiNtPrtResolveGsiv(State->Entry->PhysicalDeviceObject, &Gsiv)))
    {
        UACPINT_WS_SET_PRT(WorkSpace, (ULONG_PTR)Gsiv + UACPINT_WS_PRT_BIAS);
        DPRINT("uACPI-NT: PDO %p routed to GSIV %u\n",
               State->Entry->PhysicalDeviceObject,
               UACPINT_WS_PRT(WorkSpace) - UACPINT_WS_PRT_BIAS);
    }
    else
    {
        UACPINT_WS_SET_PRT(WorkSpace, UACPINT_WS_PRT_NONE);
    }

    State->WorkSpace = WorkSpace;
    return STATUS_SUCCESS;
}

/* The line this PDO holds from its boot configuration */
static
BOOLEAN
NTAPI
UacpiNtFindBootConfig(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PARBITER_ALLOCATION_STATE State,
    _Out_ PULONG Irq)
{
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;

    if (!Arbiter->Allocation || !State->Entry)
        return FALSE;

    if (!NT_SUCCESS(RtlGetFirstRange(Arbiter->Allocation, &Iterator, &Range)))
        return FALSE;

    while (Range)
    {
        if ((Range->Attributes & ARBITER_RANGE_BOOT_ALLOCATED) &&
            Range->Owner == State->Entry->PhysicalDeviceObject)
        {
            *Irq = (ULONG)Range->Start;
            return TRUE;
        }

        if (!NT_SUCCESS(RtlGetNextRange(&Iterator, &Range, TRUE)))
            break;
    }

    return FALSE;
}

static
BOOLEAN
NTAPI
UacpiNtFindIrqInAlternatives(
    _In_ PARBITER_ALLOCATION_STATE State,
    _In_ ULONG Irq,
    _Out_ PULONG Index)
{
    ULONG Alternative;

    for (Alternative = 0; Alternative < State->AlternativeCount; Alternative++)
    {
        if (State->Alternatives[Alternative].Minimum <= Irq &&
            State->Alternatives[Alternative].Maximum >= Irq)
        {
            *Index = Alternative;
            return TRUE;
        }
    }

    return FALSE;
}

/* _PRT routed devices try their routed line, then their boot line, then the library walk */
static
BOOLEAN
NTAPI
UacpiNtIrqGetNextAllocationRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    PARBITER_ALTERNATIVE Alternative;
    BOOLEAN HavePreferred;
    ULONG_PTR WorkSpace;
    ULONG Preferred = 0;
    ULONG Index = 0;

    Alternative = State->CurrentAlternative ? State->CurrentAlternative : State->Alternatives;

    if (State->AlternativeCount == 0 ||
        !Alternative ||
        !Alternative->Descriptor ||
        (Alternative->Descriptor->Flags & CM_RESOURCE_INTERRUPT_MESSAGE))
    {
        return ArbiterLibGetNextAllocationRange(Arbiter, State);
    }

    WorkSpace = State->WorkSpace;
    if (UACPINT_WS_PRT(WorkSpace) < UACPINT_WS_PRT_BIAS)
        return ArbiterLibGetNextAllocationRange(Arbiter, State);

    while (UACPINT_WS_CURSOR(WorkSpace) < UACPINT_NEXT_ALTERNATIVES)
    {
        switch (UACPINT_WS_CURSOR(WorkSpace))
        {
            case UACPINT_NEXT_INITIAL:
                UACPINT_WS_SET_CURSOR(WorkSpace, UACPINT_NEXT_LINK_PREF);
                HavePreferred = FALSE;
                break;

            case UACPINT_NEXT_LINK_PREF:
                UACPINT_WS_SET_CURSOR(WorkSpace, UACPINT_NEXT_BOOT_CONFIG);
                Preferred = UACPINT_WS_PRT(WorkSpace) - UACPINT_WS_PRT_BIAS;
                HavePreferred = TRUE;
                break;

            default:
                UACPINT_WS_SET_CURSOR(WorkSpace, UACPINT_NEXT_ALTERNATIVES);
                HavePreferred = UacpiNtFindBootConfig(Arbiter, State, &Preferred);
                break;
        }

        if (HavePreferred && UacpiNtFindIrqInAlternatives(State, Preferred, &Index))
        {
            State->CurrentMinimum = Preferred;
            State->CurrentMaximum = Preferred;
            State->CurrentAlternative = &State->Alternatives[Index];
            State->WorkSpace = WorkSpace;
            UacpiNtIrqLinkRotation++;
            return TRUE;
        }
    }

    State->WorkSpace = WorkSpace;
    if (!ArbiterLibGetNextAllocationRange(Arbiter, State))
        return FALSE;

    UacpiNtIrqLinkRotation++;
    return TRUE;
}

/* A clear LATCHED bit means level, unless a MADT override forced edge */
static
VOID
NTAPI
UacpiNtNotePlacedTriggering(
    _In_ PARBITER_ALLOCATION_STATE State)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;

    if (!State->CurrentAlternative)
        return;

    Descriptor = State->CurrentAlternative->Descriptor;
    if (Descriptor &&
        State->Start < UACPINT_MSI_GSIV_BASE &&
        !(Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED) &&
        !UacpiNtIrqLibGsivForcedEdge((ULONG)State->Start))
    {
        UacpiNtIrqLibNoteLevelGsiv((ULONG)State->Start);
    }
}

static
BOOLEAN
NTAPI
UacpiNtIrqFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE State)
{
    ULONG Gsiv;

    if (State->CurrentMinimum >= UACPINT_MSI_GSIV_BASE)
        return ArbiterLibFindSuitableRange(Arbiter, State);

    /* A routed device only fits on its routed line */
    if (UACPINT_WS_PRT(State->WorkSpace) >= UACPINT_WS_PRT_BIAS)
    {
        Gsiv = UACPINT_WS_PRT(State->WorkSpace) - UACPINT_WS_PRT_BIAS;
        if (Gsiv < State->CurrentMinimum || Gsiv > State->CurrentMaximum)
            return FALSE;

        State->Start = Gsiv;
        State->End = Gsiv;
        UacpiNtNotePlacedTriggering(State);
        return TRUE;
    }

    if (!ArbiterLibFindSuitableRange(Arbiter, State))
        return FALSE;

    UacpiNtNotePlacedTriggering(State);
    return TRUE;
}

static
NTSTATUS
NTAPI
UacpiNtIrqTestAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
#if (NTDDI_VERSION >= NTDDI_VISTA)
    _Inout_ PARBITER_TEST_ALLOCATION_PARAMETERS Parameters)
#else
    _Inout_ PLIST_ENTRY ArbitrationList)
#endif
{
    NTSTATUS Status;

#if (NTDDI_VERSION >= NTDDI_VISTA)
    Status = ArbiterLibTestAllocation(Arbiter, Parameters);
#else
    Status = ArbiterLibTestAllocation(Arbiter, ArbitrationList);
#endif

    /* Misses are routine probes, only passes are traced */
    if (UacpiNtIrqArbVerbose && NT_SUCCESS(Status))
        DPRINT("uACPI-NT: IRQ TestAllocation placed\n");

    return Status;
}

static
NTSTATUS
NTAPI
UacpiNtIrqRollbackAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    if (UacpiNtIrqArbVerbose)
        DPRINT("uACPI-NT: IRQ RollbackAllocation\n");

    return ArbiterLibRollbackAllocation(Arbiter);
}

static
BOOLEAN
NTAPI
UacpiNtConnectionDataChanged(
    _In_ PDEVICE_OBJECT Owner,
    _In_ ULONG Start,
    _In_ ULONG Length)
{
    ULONG Index;

    for (Index = 0; Index < UacpiNtIrqWrittenCount; Index++)
    {
        if (UacpiNtIrqWritten[Index].Owner == Owner)
        {
            return (BOOLEAN)(UacpiNtIrqWritten[Index].Start != Start ||
                             UacpiNtIrqWritten[Index].Length != Length);
        }
    }

    return TRUE;
}

static
VOID
NTAPI
UacpiNtConnectionDataRecord(
    _In_ PDEVICE_OBJECT Owner,
    _In_ ULONG Start,
    _In_ ULONG Length)
{
    PUACPINT_IRQ_WRITTEN Written;
    ULONG Index;

    for (Index = 0; Index < UacpiNtIrqWrittenCount; Index++)
    {
        if (UacpiNtIrqWritten[Index].Owner == Owner)
            break;
    }

    if (Index == UacpiNtIrqWrittenCount)
    {
        if (UacpiNtIrqWrittenCount >= UACPINT_IRQ_WRITTEN_MAX)
            return;
        UacpiNtIrqWrittenCount++;
    }

    Written = &UacpiNtIrqWritten[Index];
    Written->Owner = Owner;
    Written->Start = Start;
    Written->Length = Length;
}

/* After the library commit, write connection data for every new or moved placement */
static
NTSTATUS
NTAPI
UacpiNtIrqCommitAllocation(
    _In_ PARBITER_INSTANCE Arbiter)
{
    RTL_RANGE_LIST_ITERATOR Iterator;
    PDEVICE_OBJECT Owner;
    PRTL_RANGE Range;
    NTSTATUS WriteStatus;
    NTSTATUS Status;
    ULONG Written = 0;
    ULONG Start;
    ULONG Length;

    Status = ArbiterLibCommitAllocation(Arbiter);
    if (!NT_SUCCESS(Status))
        return Status;

    if (NT_SUCCESS(RtlGetFirstRange(Arbiter->Allocation, &Iterator, &Range)))
    {
        do
        {
            Owner = (PDEVICE_OBJECT)Range->Owner;
            Start = (ULONG)Range->Start;

            if (Range->Start < UACPINT_MSI_GSIV_BASE)
            {
                Length = (ULONG)(Range->End - Range->Start + 1);
            }
            else if (!NT_SUCCESS(UacpiNtMsiCountLookup(Range->Start, &Length)) || Length == 0)
            {
                DPRINT1("uACPI-NT: message placement 0x%I64X has no recorded count, using 1\n",
                        Range->Start);
                Length = 1;
            }

            /* The FDO's own reservations and unchanged placements are skipped */
            if (!Owner ||
                Owner == UacpiNtIrqArbFdoSelf ||
                !UacpiNtConnectionDataChanged(Owner, Start, Length))
            {
                continue;
            }

            /* A failed write stays out of the cache so the next commit retries it */
            WriteStatus = UacpiNtIrqLibWriteConnectionData(Owner, Start, Length);
            if (NT_SUCCESS(WriteStatus))
            {
                UacpiNtConnectionDataRecord(Owner, Start, Length);
                Written++;
            }
            else
            {
                DPRINT1("uACPI-NT: connection data for PDO %p GSIV %u failed 0x%lx\n",
                        Owner,
                        Start,
                        WriteStatus);
            }
        } while (NT_SUCCESS(RtlGetNextRange(&Iterator, &Range, TRUE)));
    }

    if (Written != 0)
        DPRINT("uACPI-NT: IRQ commit wrote connection data for %u device(s)\n", Written);

    return Status;
}

NTSTATUS
NTAPI
UacpiNtIrqArbiterInitialize(
    _In_ PUACPINT_FDO Fdo)
{
    struct acpi_fadt *Fadt = NULL;
    ULONG SciGsiv = (ULONG)-1;
    NTSTATUS Status;

    if (UacpiNtIrqArbReady)
        return STATUS_SUCCESS;

    RtlZeroMemory(&UacpiNtIrqArbiter, sizeof(UacpiNtIrqArbiter));

    UacpiNtIrqPolicyConfigure();

    /* The library only fills slots left NULL */
    UacpiNtIrqArbiter.UnpackRequirement = UacpiNtIrqUnpackRequirement;
    UacpiNtIrqArbiter.PackResource = UacpiNtIrqPackResource;
    UacpiNtIrqArbiter.UnpackResource = UacpiNtIrqUnpackResource;
    UacpiNtIrqArbiter.ScoreRequirement = UacpiNtIrqScoreRequirement;
    UacpiNtIrqArbiter.PreprocessEntry = UacpiNtIrqPreprocessEntry;
    UacpiNtIrqArbiter.GetNextAllocationRange = UacpiNtIrqGetNextAllocationRange;
    UacpiNtIrqArbiter.FindSuitableRange = UacpiNtIrqFindSuitableRange;
    UacpiNtIrqArbiter.CommitAllocation = UacpiNtIrqCommitAllocation;
    UacpiNtIrqArbiter.TestAllocation = UacpiNtIrqTestAllocation;
    UacpiNtIrqArbiter.RollbackAllocation = UacpiNtIrqRollbackAllocation;
    UacpiNtIrqArbiter.ConflictCallback = UacpiNtIrqShareableRange;
    UacpiNtIrqArbFdoSelf = Fdo->Shared.Self;

    Status = ArbiterLibInitializeInstance(&UacpiNtIrqArbiter,
                                          Fdo->Shared.Self,
                                          CmResourceTypeInterrupt,
                                          L"ACPI_IRQ",
                                          L"Root",
                                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: IRQ arbiter initialization failed 0x%lx\n", Status);
        return Status;
    }

    /* The SCI is shared, level triggered, and off limits to PCI links */
    if (uacpi_likely_success(uacpi_table_fadt(&Fadt)) && Fadt)
        SciGsiv = Fadt->sci_int;

    if (SciGsiv != (ULONG)-1)
    {
        RtlAddRange(UacpiNtIrqArbiter.Allocation,
                    SciGsiv,
                    SciGsiv,
                    UACPINT_RANGE_LEVEL,
                    RTL_RANGE_LIST_ADD_SHARED,
                    NULL,
                    Fdo->Shared.Self);
        UacpiNtIrqLibNoteLevelGsiv(SciGsiv);
        UacpiNtIrqSciGsiv = SciGsiv;
    }

    /* The PIC has no lines past 15, block up to the message window */
    if (GlobalAcpiInterruptModel == 0)
    {
        RtlAddRange(UacpiNtIrqArbiter.Allocation,
                    UACPINT_ISA_GSIV_COUNT,
                    UACPINT_MSI_GSIV_BASE - 1,
                    0,
                    0,
                    NULL,
                    Fdo->Shared.Self);
    }

    UacpiNtIrqArbReady = TRUE;
    UacpiNtMsiDiagArm();

    DPRINT("uACPI-NT: IRQ arbiter up, %s model, SCI GSIV %u %s\n",
           (GlobalAcpiInterruptModel == 1) ? "APIC" : "PIC",
           (SciGsiv != (ULONG)-1) ? SciGsiv : 0,
           (SciGsiv != (ULONG)-1) ? "reserved shared" : "unknown");
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UacpiNtIrqArbReference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

static
VOID
NTAPI
UacpiNtIrqArbDereference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

NTSTATUS
NTAPI
UacpiNtQueryIrqArbiter(
    _In_ PIO_STACK_LOCATION IoStack)
{
    PARBITER_INTERFACE Interface;

    if (!UacpiNtIrqArbEnabled || !UacpiNtIrqArbReady)
        return STATUS_NOT_SUPPORTED;

    if (!IsEqualGUID(IoStack->Parameters.QueryInterface.InterfaceType,
                     &GUID_ARBITER_INTERFACE_STANDARD))
    {
        return STATUS_NOT_SUPPORTED;
    }

    if ((ULONG_PTR)IoStack->Parameters.QueryInterface.InterfaceSpecificData !=
        CmResourceTypeInterrupt)
    {
        return STATUS_NOT_SUPPORTED;
    }

    Interface = (PARBITER_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
    if (IoStack->Parameters.QueryInterface.Size < sizeof(*Interface))
        return STATUS_BUFFER_TOO_SMALL;

    Interface->Size = sizeof(*Interface);
    Interface->Version = 1;
    Interface->Context = &UacpiNtIrqArbiter;
    Interface->InterfaceReference = UacpiNtIrqArbReference;
    Interface->InterfaceDereference = UacpiNtIrqArbDereference;
    Interface->ArbiterHandler = ArbiterLibHandler;
    Interface->Flags = 0;

    DPRINT("uACPI-NT: provided the interrupt arbiter interface\n");
    return STATUS_SUCCESS;
}

/* Dump an MSI-X table through a temporary uncached mapping */
static
VOID
NTAPI
UacpiNtDumpMsiXTable(
    _In_ PDEVICE_OBJECT Pdo,
    _In_z_ PCSTR Name,
    _In_ ULONG Bir,
    _In_ ULONG TableOffset,
    _In_ ULONG Entries)
{
    PHYSICAL_ADDRESS Physical;
    PULONG Entry;
    PUCHAR Table;
    ULONG BarLow = 0;
    ULONG BarHigh = 0;
    ULONG Programmed = 0;
    ULONG Unmasked = 0;
    ULONG AddressLow;
    ULONG AddressHigh;
    ULONG Data;
    ULONG Control;
    ULONG Shown;
    ULONG Bytes;
    ULONG Index;

    if (Entries == 0 || Entries > 256 || Bir > 5)
        return;

    if (!NT_SUCCESS(UacpiNtReadPciConfig(Pdo, 0x10 + Bir * 4, &BarLow, sizeof(BarLow))))
        return;

    if (BarLow & PCI_ADDRESS_IO_SPACE)
    {
        DPRINT("uACPI-NT: %s     table BAR%u is I/O space\n", Name, Bir);
        return;
    }

    if ((BarLow & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
        UacpiNtReadPciConfig(Pdo, 0x10 + (Bir + 1) * 4, &BarHigh, sizeof(BarHigh));

    Physical.LowPart = (BarLow & ~0xFul) + TableOffset;
    Physical.HighPart = (LONG)BarHigh;
    if (Physical.LowPart == TableOffset && BarHigh == 0)
    {
        DPRINT("uACPI-NT: %s     table BAR%u not assigned\n", Name, Bir);
        return;
    }

    Bytes = Entries * 16;
    Table = MmMapIoSpace(Physical, Bytes, MmNonCached);
    if (!Table)
    {
        DPRINT1("uACPI-NT: %s     mapping MSI-X table 0x%08X%08X (%u bytes) failed\n",
                Name,
                (ULONG)Physical.HighPart,
                Physical.LowPart,
                Bytes);
        return;
    }

    DPRINT("uACPI-NT: %s     MSI-X table at 0x%08X%08X, %u entries\n",
           Name,
           (ULONG)Physical.HighPart,
           Physical.LowPart,
           Entries);

    Shown = min(Entries, 32);
    for (Index = 0; Index < Entries; Index++)
    {
        Entry = (PULONG)(Table + Index * 16);
        AddressLow = READ_REGISTER_ULONG(&Entry[0]);
        AddressHigh = READ_REGISTER_ULONG(&Entry[1]);
        Data = READ_REGISTER_ULONG(&Entry[2]);
        Control = READ_REGISTER_ULONG(&Entry[3]);

        if (AddressLow != 0 || AddressHigh != 0)
            Programmed++;

        if (!(Control & 1))
            Unmasked++;

        if (Index < Shown)
        {
            DPRINT("uACPI-NT: %s       [%02u] address 0x%08X%08X data 0x%04X (vector 0x%02X) control 0x%X %s\n",
                   Name,
                   Index,
                   AddressHigh,
                   AddressLow,
                   Data & 0xFFFF,
                   Data & 0xFF,
                   Control,
                   (Control & 1) ? "masked" : "unmasked");
        }
    }

    DPRINT("uACPI-NT: %s     %u/%u entries programmed, %u unmasked\n",
           Name,
           Programmed,
           Entries,
           Unmasked);

    MmUnmapIoSpace(Table, Bytes);
}

static
VOID
NTAPI
UacpiNtDumpMsiCapability(
    _In_ PDEVICE_OBJECT Pdo,
    _In_z_ PCSTR Name,
    _In_ UCHAR Capability,
    _In_ USHORT Control)
{
    ULONG AddressLow = 0;
    ULONG AddressHigh = 0;
    ULONG Data = 0;
    BOOLEAN Is64Bit = (BOOLEAN)((Control & 0x80) != 0);

    UacpiNtReadPciConfig(Pdo, Capability + 4, &AddressLow, sizeof(AddressLow));
    if (Is64Bit)
    {
        UacpiNtReadPciConfig(Pdo, Capability + 8, &AddressHigh, sizeof(AddressHigh));
        UacpiNtReadPciConfig(Pdo, Capability + 12, &Data, sizeof(Data));
    }
    else
    {
        UacpiNtReadPciConfig(Pdo, Capability + 8, &Data, sizeof(Data));
    }

    DPRINT("uACPI-NT: %s   MSI at 0x%02X control 0x%04X enable %u capable %u granted %u %s address 0x%08X%08X data 0x%04X (vector 0x%02X)\n",
           Name,
           Capability,
           Control,
           Control & 1,
           1u << ((Control >> 1) & 7),
           1u << ((Control >> 4) & 7),
           Is64Bit ? "64-bit" : "32-bit",
           AddressHigh,
           AddressLow,
           Data & 0xFFFF,
           Data & 0xFF);
}

static
VOID
NTAPI
UacpiNtDumpMsiXCapability(
    _In_ PDEVICE_OBJECT Pdo,
    _In_z_ PCSTR Name,
    _In_ UCHAR Capability,
    _In_ USHORT Control)
{
    ULONG Table = 0;
    ULONG PendingBits = 0;
    ULONG TableSize = (Control & 0x7FF) + 1;

    UacpiNtReadPciConfig(Pdo, Capability + 4, &Table, sizeof(Table));
    UacpiNtReadPciConfig(Pdo, Capability + 8, &PendingBits, sizeof(PendingBits));

    DPRINT("uACPI-NT: %s   MSI-X at 0x%02X control 0x%04X enable %u function mask %u size %u table BIR %u offset 0x%X PBA BIR %u offset 0x%X\n",
           Name,
           Capability,
           Control,
           (Control >> 15) & 1,
           (Control >> 14) & 1,
           TableSize,
           Table & 7,
           Table & ~7u,
           PendingBits & 7,
           PendingBits & ~7u);

    UacpiNtDumpMsiXTable(Pdo, Name, Table & 7, Table & ~7u, TableSize);
}

/* Walk a function's capability list. PASSIVE_LEVEL only. */
static
VOID
NTAPI
UacpiNtDumpMsiState(
    _In_ PDEVICE_OBJECT Pdo,
    _In_z_ PCSTR Name)
{
    ULONG Ids = 0;
    ULONG CommandStatus = 0;
    ULONG ClassDword = 0;
    ULONG CapabilityDword = 0;
    ULONG PointerDword = 0;
    USHORT StatusBits;
    BOOLEAN Found = FALSE;
    UCHAR Capability;
    ULONG Guard;

    if (!Pdo)
        return;

    if (!NT_SUCCESS(UacpiNtReadPciConfig(Pdo, 0, &Ids, sizeof(Ids))))
    {
        DPRINT("uACPI-NT: %s PDO %p config space unreadable\n", Name, Pdo);
        return;
    }

    if ((USHORT)Ids == PCI_INVALID_VENDORID)
        return;

    UacpiNtReadPciConfig(Pdo, FIELD_OFFSET(PCI_COMMON_CONFIG, Command), &CommandStatus, sizeof(CommandStatus));
    UacpiNtReadPciConfig(Pdo, UACPINT_PCI_CLASS_DWORD, &ClassDword, sizeof(ClassDword));
    StatusBits = (USHORT)(CommandStatus >> 16);

    DPRINT("uACPI-NT: %s PDO %p %04X:%04X class 0x%04X command 0x%04X status 0x%04X INTx %s, INTx status %s\n",
           Name,
           Pdo,
           Ids & 0xFFFF,
           Ids >> 16,
           (((ClassDword >> 24) & 0xFF) << 8) | ((ClassDword >> 16) & 0xFF),
           CommandStatus & 0xFFFF,
           (ULONG)StatusBits,
           (CommandStatus & 0x400) ? "disabled (message mode)" : "enabled (line mode)",
           (StatusBits & PCI_STATUS_INTERRUPT_PENDING) ? "asserted" : "clear");

    if (!(StatusBits & PCI_STATUS_CAPABILITIES_LIST))
    {
        DPRINT("uACPI-NT: %s no capability list\n", Name);
        return;
    }

    if (!NT_SUCCESS(UacpiNtReadPciConfig(Pdo,
                                         FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.CapabilitiesPtr),
                                         &PointerDword,
                                         sizeof(PointerDword))))
    {
        return;
    }

    Capability = (UCHAR)(PointerDword & 0xFC);
    for (Guard = 0; Capability >= sizeof(PCI_COMMON_HEADER) && Guard < 48; Guard++)
    {
        if (!NT_SUCCESS(UacpiNtReadPciConfig(Pdo, Capability, &CapabilityDword, sizeof(CapabilityDword))))
            break;

        switch (CapabilityDword & 0xFF)
        {
            case PCI_CAPABILITY_ID_MSI:
                UacpiNtDumpMsiCapability(Pdo, Name, Capability, (USHORT)(CapabilityDword >> 16));
                Found = TRUE;
                break;

            case PCI_CAPABILITY_ID_MSIX:
                UacpiNtDumpMsiXCapability(Pdo, Name, Capability, (USHORT)(CapabilityDword >> 16));
                Found = TRUE;
                break;

            default:
                break;
        }

        Capability = (UCHAR)((CapabilityDword >> 8) & 0xFC);
    }

    if (!Found)
        DPRINT("uACPI-NT: %s   no MSI or MSI-X capability\n", Name);
}

/* Can the device stack hand out a DMA adapter and common buffers? */
static
VOID
NTAPI
UacpiNtDumpDmaAdapter(
    _In_ PDEVICE_OBJECT Pdo,
    _In_z_ PCSTR Name)
{
    static const ULONG Sizes[] = { 0x1000, 0x10000, 0x40000, 0x100000 };
    DEVICE_DESCRIPTION Description;
    PHYSICAL_ADDRESS Logical;
    PDMA_OPERATIONS Operations;
    PDMA_ADAPTER Adapter;
    ULONG MapRegisters = 0;
    PVOID Buffer;
    ULONG Index;

    if (!Pdo)
        return;

    RtlZeroMemory(&Description, sizeof(Description));
    Description.Version = DEVICE_DESCRIPTION_VERSION;
    Description.InterfaceType = PCIBus;
    Description.Master = TRUE;
    Description.ScatterGather = TRUE;
    Description.Dma32BitAddresses = FALSE;
    Description.Dma64BitAddresses = TRUE;
    Description.MaximumLength = 0x10000;
    Description.DmaChannel = 0;
    Description.DmaWidth = Width32Bits;
    Description.DmaSpeed = Compatible;

    Adapter = IoGetDmaAdapter(Pdo, &Description, &MapRegisters);
    if (!Adapter)
    {
        DPRINT1("uACPI-NT: %s PDO %p has no DMA adapter, its start will fail\n", Name, Pdo);
        return;
    }

    DPRINT("uACPI-NT: %s PDO %p DMA adapter %p, %u map registers\n",
           Name,
           Pdo,
           Adapter,
           MapRegisters);

    Operations = Adapter->DmaOperations;
    if (Operations && Operations->AllocateCommonBuffer)
    {
        /* Stop at the first size that fails */
        for (Index = 0; Index < RTL_NUMBER_OF(Sizes); Index++)
        {
            Logical.QuadPart = 0;
            Buffer = Operations->AllocateCommonBuffer(Adapter, Sizes[Index], &Logical, FALSE);
            if (!Buffer)
            {
                DPRINT1("uACPI-NT: %s   common buffer of %u bytes failed\n", Name, Sizes[Index]);
                break;
            }

            DPRINT("uACPI-NT: %s   common buffer of %u bytes at %p logical 0x%08X%08X\n",
                   Name,
                   Sizes[Index],
                   Buffer,
                   (ULONG)Logical.HighPart,
                   Logical.LowPart);

            if (Operations->FreeCommonBuffer)
                Operations->FreeCommonBuffer(Adapter, Sizes[Index], Logical, Buffer, FALSE);
        }
    }

    if (Operations && Operations->PutDmaAdapter)
        Operations->PutDmaAdapter(Adapter);
}

/* PIC model only, report committed ISA lines still masked in the 8259 IMRs */
static
VOID
NTAPI
UacpiNtDumpLineState(VOID)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    PDEVICE_OBJECT Owner;
    KAFFINITY Affinity;
    ULONG Polarity;
    ULONG Vector;
    ULONG Mode;
    ULONG Gsiv;
    ULONG Index;
    KIRQL Irql;
    UCHAR MasterMask;
    UCHAR SlaveMask;
    BOOLEAN Masked;

    if (GlobalAcpiInterruptModel != 0)
        return;

    MasterMask = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)0x21);
    SlaveMask = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)0xA1);

    DPRINT("uACPI-NT: wired line readback, PIC IMR master 0x%02X slave 0x%02X\n",
           MasterMask,
           SlaveMask);

    for (Index = 0; Index < UacpiNtIrqWrittenCount; Index++)
    {
        Gsiv = UacpiNtIrqWritten[Index].Start;
        Owner = UacpiNtIrqWritten[Index].Owner;

        /* Message runs are covered above and only 0 to 15 sit on an 8259 */
        if (Gsiv >= UACPINT_ISA_GSIV_COUNT)
            continue;

        if (Gsiv < 8)
            Masked = (BOOLEAN)((MasterMask >> Gsiv) & 1);
        else
            Masked = (BOOLEAN)((SlaveMask >> (Gsiv - 8)) & 1);

        if (!NT_SUCCESS(UacpiNtIrqLibResolveVector(Gsiv, &Vector, &Irql, &Affinity, &Polarity, &Mode)))
            Vector = 0;

        DPRINT("uACPI-NT: GSIV %2u vector 0x%02X owner %p %s\n",
               Gsiv,
               Vector,
               Owner,
               Masked ? "masked, nothing connected" : "unmasked");
    }

    DPRINT("uACPI-NT: wired line readback done\n");
#endif
}

static
VOID
NTAPI
UacpiNtMsiDiagWorker(
    _In_opt_ PVOID Context)
{
    UACPINT_MSI_DIAG_TARGET Targets[48];
    uacpi_object_name NodeName;
    PUACPINT_FLT Filter;
    PLIST_ENTRY Entry;
    ULONG Count = 0;
    ULONG Index;

    UNREFERENCED_PARAMETER(Context);

    if (!GlobalAcpiFdo)
        return;

    /* Hold references so the dump can send IRPs outside the lock */
    ExAcquireFastMutex(&GlobalAcpiFdo->ChildLock);
    for (Entry = GlobalAcpiFdo->FilterList.Flink;
         Entry != &GlobalAcpiFdo->FilterList && Count < RTL_NUMBER_OF(Targets);
         Entry = Entry->Flink)
    {
        Filter = CONTAINING_RECORD(Entry, UACPINT_FLT, Link);
        if (!Filter->ForeignPdo)
            continue;

        ObReferenceObject(Filter->ForeignPdo);
        Targets[Count].Pdo = Filter->ForeignPdo;

        if (Filter->Node)
        {
            NodeName = uacpi_namespace_node_name(Filter->Node);
            RtlCopyMemory(Targets[Count].Name, NodeName.text, sizeof(NodeName.text));
            Targets[Count].Name[sizeof(NodeName.text)] = ANSI_NULL;
        }
        else
        {
            Targets[Count].Name[0] = '?';
            Targets[Count].Name[1] = ANSI_NULL;
        }
        Count++;
    }
    ExReleaseFastMutex(&GlobalAcpiFdo->ChildLock);

    DPRINT("uACPI-NT: message interrupt readback, %u filtered function(s)\n", Count);
    for (Index = 0; Index < Count; Index++)
    {
        UacpiNtDumpMsiState(Targets[Index].Pdo, Targets[Index].Name);
        UacpiNtDumpDmaAdapter(Targets[Index].Pdo, Targets[Index].Name);
        ObDereferenceObject(Targets[Index].Pdo);
    }
    DPRINT("uACPI-NT: message interrupt readback done\n");

    UacpiNtDumpLineState();
}

/* Config reads need PASSIVE_LEVEL, so the timer only queues the worker */
static
VOID
NTAPI
UacpiNtMsiDiagDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ExQueueWorkItem(&UacpiNtMsiDiagWork, DelayedWorkQueue);
}

VOID
NTAPI
UacpiNtMsiDiagArm(VOID)
{
    LARGE_INTEGER DueTime;

    if (!UacpiNtMsiDiagEnabled)
        return;

    /* One shot: 0 idle, 1 armed, 2 disarmed */
    if (InterlockedCompareExchange(&UacpiNtMsiDiagState, 1, 0) != 0)
        return;

    ExInitializeWorkItem(&UacpiNtMsiDiagWork, UacpiNtMsiDiagWorker, NULL);
    KeInitializeDpc(&UacpiNtMsiDiagDpcObject, UacpiNtMsiDiagDpc, NULL);
    KeInitializeTimer(&UacpiNtMsiDiagTimer);

    DueTime.QuadPart = -((LONGLONG)UacpiNtMsiDiagDelaySeconds * 10 * 1000 * 1000);
    KeSetTimer(&UacpiNtMsiDiagTimer, DueTime, &UacpiNtMsiDiagDpcObject);

    DPRINT("uACPI-NT: message interrupt readback in %u second(s)\n", UacpiNtMsiDiagDelaySeconds);
}

VOID
NTAPI
UacpiNtMsiDiagDisarm(VOID)
{
    if (InterlockedCompareExchange(&UacpiNtMsiDiagState, 2, 1) == 1)
        KeCancelTimer(&UacpiNtMsiDiagTimer);
}
