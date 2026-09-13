/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Resource translators and legacy bus lookup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <wdmguid.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define TAG_IO_TRANSLATOR 'rTpI'

/* Resource types with a bit in the translator and arbiter masks of a device node */
#define IOP_MASKED_RESOURCE_TYPES (RTL_FIELD_SIZE(DEVICE_NODE, NoTranslatorMask) * 8)

/* Bus number of a device node that provides no legacy bus */
#define IOP_NO_LEGACY_BUS_NUMBER 0xFFFFFFF0

/*
 * Translator of one resource type cached on a device node. A missing translator
 * is recorded in the node masks, or with an entry that is not present.
 */
typedef struct _IOP_TRANSLATOR_ENTRY
{
    LIST_ENTRY ListEntry;
    UCHAR ResourceType;
    BOOLEAN IsPresent;
    TRANSLATOR_INTERFACE Interface;
} IOP_TRANSLATOR_ENTRY, *PIOP_TRANSLATOR_ENTRY;

/* Bus device nodes of each legacy interface type, sorted by bus number */
static LIST_ENTRY IopLegacyBusLists[ACPIBus + 1];

/* Guards the legacy bus lists against buses that leave the device tree */
static ERESOURCE IopLegacyBusLock;

/* ROOT TRANSLATOR **********************************************************/

/* The root translator has no state, so there is nothing to count */
static
VOID
NTAPI
IopRootTranslatorReference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/* The root bus uses the same resources as the processor, so they are copied */
static
NTSTATUS
NTAPI
IopRootTranslateResources(
    _Inout_opt_ PVOID Context,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    _In_ RESOURCE_TRANSLATION_DIRECTION Direction,
    _In_opt_ ULONG AlternativesCount,
    _In_reads_opt_(AlternativesCount) IO_RESOURCE_DESCRIPTOR Alternatives[],
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Target)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Direction);
    UNREFERENCED_PARAMETER(AlternativesCount);
    UNREFERENCED_PARAMETER(Alternatives);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    *Target = *Source;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IopRootTranslateRequirements(
    _Inout_opt_ PVOID Context,
    _In_ PIO_RESOURCE_DESCRIPTOR Source,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PULONG TargetCount,
    _Out_ PIO_RESOURCE_DESCRIPTOR *Target)
{
    PIO_RESOURCE_DESCRIPTOR Copy;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    Copy = ExAllocatePoolWithTag(PagedPool, sizeof(*Copy), TAG_IO_TRANSLATOR);
    if (Copy == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    *Copy = *Source;
    *Target = Copy;
    *TargetCount = 1;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Handles IRP_MN_QUERY_INTERFACE on the root PDO for the root translator. The
 * same translator is returned for every resource type.
 *
 * @return
 * STATUS_SUCCESS if the interface was returned, ExistingStatus otherwise.
 */
NTSTATUS
NTAPI
IopTranslatorQueryRootInterface(
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ NTSTATUS ExistingStatus)
{
    PTRANSLATOR_INTERFACE Output;

    PAGED_CODE();

    if (!IsEqualGUID(IoStack->Parameters.QueryInterface.InterfaceType,
                     &GUID_TRANSLATOR_INTERFACE_STANDARD) ||
        IoStack->Parameters.QueryInterface.Size < sizeof(*Output) ||
        IoStack->Parameters.QueryInterface.Interface == NULL)
    {
        return ExistingStatus;
    }

    Output = (PTRANSLATOR_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
    Output->Size = sizeof(*Output);
    Output->Version = 0;
    Output->Context = NULL;
    Output->InterfaceReference = IopRootTranslatorReference;
    Output->InterfaceDereference = IopRootTranslatorReference;
    Output->TranslateResources = IopRootTranslateResources;
    Output->TranslateResourceRequirements = IopRootTranslateRequirements;

    return STATUS_SUCCESS;
}

/* DEVICE TRANSLATORS *******************************************************/

/**
 * @brief
 * Checks if the stack of a device node may be asked for an arbiter or a
 * translator. Device nodes of legacy resource claims and devices that were not
 * enumerated by a bus are never asked.
 */
BOOLEAN
NTAPI
IopCanQueryResourceHandlers(
    _In_ PDEVICE_NODE DeviceNode)
{
    return DeviceNode->PhysicalDeviceObject != NULL &&
           !(DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE) &&
           (DeviceNode->PhysicalDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE);
}

/* Returns the bit of a resource type in the masks of a device node, or 0 */
static
USHORT
NTAPI
IopResourceTypeBit(
    _In_ UCHAR ResourceType)
{
    return (ResourceType < IOP_MASKED_RESOURCE_TYPES) ? (USHORT)(1 << ResourceType) : 0;
}

static
PIOP_TRANSLATOR_ENTRY
NTAPI
IopFindTranslatorEntry(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR ResourceType)
{
    PLIST_ENTRY ListEntry;

    for (ListEntry = DeviceNode->DeviceTranslatorList.Flink;
         ListEntry != &DeviceNode->DeviceTranslatorList;
         ListEntry = ListEntry->Flink)
    {
        PIOP_TRANSLATOR_ENTRY Entry = CONTAINING_RECORD(ListEntry, IOP_TRANSLATOR_ENTRY, ListEntry);

        if (Entry->ResourceType == ResourceType)
            return Entry;
    }

    return NULL;
}

/**
 * @brief
 * Returns the cached answer of a device node for a translator.
 *
 * @param[out] Translator
 * Receives the translator, or NULL if the device has none.
 *
 * @return
 * FALSE if the device was not asked yet.
 */
static
BOOLEAN
NTAPI
IopGetCachedTranslator(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR ResourceType,
    _Out_ PTRANSLATOR_INTERFACE *Translator)
{
    USHORT TypeBit = IopResourceTypeBit(ResourceType);
    PIOP_TRANSLATOR_ENTRY Entry;

    *Translator = NULL;

    if (DeviceNode->NoTranslatorMask & TypeBit)
        return TRUE;

    /* A tracked type that was never asked for has no entry to look for */
    if (TypeBit != 0 && !(DeviceNode->QueryTranslatorMask & TypeBit))
        return FALSE;

    Entry = IopFindTranslatorEntry(DeviceNode, ResourceType);
    if (Entry == NULL)
        return FALSE;

    if (Entry->IsPresent)
        *Translator = &Entry->Interface;

    return TRUE;
}

/* Sends IRP_MN_QUERY_INTERFACE for GUID_TRANSLATOR_INTERFACE_STANDARD to a device */
static
NTSTATUS
NTAPI
IopQueryTranslatorInterface(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR ResourceType,
    _Out_ PTRANSLATOR_INTERFACE Interface)
{
    IO_STATUS_BLOCK IoStatusBlock;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;

    RtlZeroMemory(Interface, sizeof(*Interface));

    if (!IopCanQueryResourceHandlers(DeviceNode))
        return STATUS_NOT_SUPPORTED;

    Interface->Size = sizeof(*Interface);

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.Parameters.QueryInterface.InterfaceType = &GUID_TRANSLATOR_INTERFACE_STANDARD;
    Stack.Parameters.QueryInterface.Size = sizeof(*Interface);
    Stack.Parameters.QueryInterface.Version = 0;
    Stack.Parameters.QueryInterface.Interface = (PINTERFACE)Interface;
    Stack.Parameters.QueryInterface.InterfaceSpecificData = UlongToPtr(ResourceType);

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_INTERFACE,
                               &Stack);
    if (NT_SUCCESS(Status) &&
        (Interface->TranslateResources == NULL || Interface->TranslateResourceRequirements == NULL))
    {
        DPRINT1("%wZ returned a translator without handlers\n", &DeviceNode->InstancePath);

        if (Interface->InterfaceDereference != NULL)
            Interface->InterfaceDereference(Interface->Context);

        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

/**
 * @brief
 * Returns the translator of a device node for a resource type. The stack of
 * the device is asked once, and the answer is cached on the device node.
 *
 * @return
 * STATUS_SUCCESS, STATUS_NOT_FOUND if the device has no translator for the
 * type, or STATUS_INSUFFICIENT_RESOURCES.
 */
NTSTATUS
NTAPI
IopGetDeviceTranslator(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR ResourceType,
    _Out_ PTRANSLATOR_INTERFACE *Translator)
{
    USHORT TypeBit = IopResourceTypeBit(ResourceType);
    PIOP_TRANSLATOR_ENTRY Entry;
    TRANSLATOR_INTERFACE Queried;
    NTSTATUS Status;

    PAGED_CODE();

    if (IopGetCachedTranslator(DeviceNode, ResourceType, Translator))
        return (*Translator != NULL) ? STATUS_SUCCESS : STATUS_NOT_FOUND;

    Status = IopQueryTranslatorInterface(DeviceNode, ResourceType, &Queried);
    DeviceNode->QueryTranslatorMask |= TypeBit;

    if (!NT_SUCCESS(Status))
    {
        DeviceNode->NoTranslatorMask |= TypeBit;
        if (TypeBit != 0)
            return STATUS_NOT_FOUND;
    }

    Entry = ExAllocatePoolZero(PagedPool, sizeof(*Entry), TAG_IO_TRANSLATOR);
    if (Entry == NULL)
    {
        if (NT_SUCCESS(Status) && Queried.InterfaceDereference != NULL)
            Queried.InterfaceDereference(Queried.Context);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Entry->ResourceType = ResourceType;
    Entry->IsPresent = NT_SUCCESS(Status);
    Entry->Interface = Queried;
    InsertTailList(&DeviceNode->DeviceTranslatorList, &Entry->ListEntry);

    if (!Entry->IsPresent)
        return STATUS_NOT_FOUND;

    DPRINT("%wZ translates resource type %u\n", &DeviceNode->InstancePath, ResourceType);

    *Translator = &Entry->Interface;
    return STATUS_SUCCESS;
}

/* Releases the translators cached on a device node */
VOID
NTAPI
IopFreeDeviceNodeTranslators(
    _In_ PDEVICE_NODE DeviceNode)
{
    while (!IsListEmpty(&DeviceNode->DeviceTranslatorList))
    {
        PIOP_TRANSLATOR_ENTRY Entry =
            CONTAINING_RECORD(RemoveHeadList(&DeviceNode->DeviceTranslatorList),
                              IOP_TRANSLATOR_ENTRY,
                              ListEntry);

        if (Entry->IsPresent && Entry->Interface.InterfaceDereference != NULL)
            Entry->Interface.InterfaceDereference(Entry->Interface.Context);

        ExFreePoolWithTag(Entry, TAG_IO_TRANSLATOR);
    }

    DeviceNode->NoTranslatorMask = 0;
    DeviceNode->QueryTranslatorMask = 0;
}

/* LEGACY BUSES *************************************************************/

static
VOID
NTAPI
IopAcquireLegacyBuses(
    _In_ BOOLEAN IsExclusive)
{
    KeEnterCriticalRegion();

    if (IsExclusive)
        ExAcquireResourceExclusiveLite(&IopLegacyBusLock, TRUE);
    else
        ExAcquireResourceSharedLite(&IopLegacyBusLock, TRUE);
}

static
VOID
NTAPI
IopReleaseLegacyBuses(VOID)
{
    ExReleaseResourceLite(&IopLegacyBusLock);
    KeLeaveCriticalRegion();
}

/* Returns the list of the buses of a legacy interface type, or NULL */
static
PLIST_ENTRY
NTAPI
IopLegacyBusList(
    _In_ INTERFACE_TYPE InterfaceType)
{
    /* EISA buses are found as ISA buses */
    if (InterfaceType == Eisa)
        InterfaceType = Isa;

    /* A PnP bus has no legacy resources */
    if ((ULONG)InterfaceType >= RTL_NUMBER_OF(IopLegacyBusLists) || InterfaceType == PNPBus)
        return NULL;

    return &IopLegacyBusLists[InterfaceType];
}

/**
 * @brief
 * Returns the first bus of a sorted legacy bus list whose number is not below
 * BusNumber, or the list head when there is none.
 */
static
PLIST_ENTRY
NTAPI
IopLegacyBusPosition(
    _In_ PLIST_ENTRY List,
    _In_ ULONG BusNumber)
{
    PLIST_ENTRY ListEntry = List->Flink;

    while (ListEntry != List &&
           CONTAINING_RECORD(ListEntry, DEVICE_NODE, LegacyBusListEntry)->BusNumber < BusNumber)
    {
        ListEntry = ListEntry->Flink;
    }

    return ListEntry;
}

CODE_SEG("INIT")
VOID
NTAPI
IopInitializeLegacyBusLists(VOID)
{
    ULONG Index;

    for (Index = 0; Index < RTL_NUMBER_OF(IopLegacyBusLists); Index++)
        InitializeListHead(&IopLegacyBusLists[Index]);

    ExInitializeResourceLite(&IopLegacyBusLock);
}

/**
 * @brief
 * Returns the device node of the bus that provides a legacy bus, or the root
 * device node if no device provides it.
 */
static
PDEVICE_NODE
NTAPI
IopLookupLegacyBus(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber)
{
    PLIST_ENTRY List = IopLegacyBusList(InterfaceType);
    PDEVICE_NODE Found = IopRootDeviceNode;
    PLIST_ENTRY Position;

    if (List == NULL)
        return IopRootDeviceNode;

    IopAcquireLegacyBuses(FALSE);

    Position = IopLegacyBusPosition(List, BusNumber);
    if (Position != List &&
        CONTAINING_RECORD(Position, DEVICE_NODE, LegacyBusListEntry)->BusNumber == BusNumber)
    {
        Found = CONTAINING_RECORD(Position, DEVICE_NODE, LegacyBusListEntry);
    }

    IopReleaseLegacyBuses();

    return Found;
}

/* Adds a device node to the list of its legacy bus type, unless the bus is already there */
static
VOID
NTAPI
IopInsertLegacyBus(
    _In_ PDEVICE_NODE DeviceNode)
{
    PLIST_ENTRY List = IopLegacyBusList(DeviceNode->InterfaceType);
    PLIST_ENTRY Position;

    if (List == NULL)
        return;

    IopAcquireLegacyBuses(TRUE);

    Position = IopLegacyBusPosition(List, DeviceNode->BusNumber);
    if (Position == List ||
        CONTAINING_RECORD(Position, DEVICE_NODE, LegacyBusListEntry)->BusNumber !=
            DeviceNode->BusNumber)
    {
        InsertTailList(Position, &DeviceNode->LegacyBusListEntry);
    }

    IopReleaseLegacyBuses();
}

/**
 * @brief
 * Asks a device which legacy bus it provides, with
 * IRP_MN_QUERY_LEGACY_BUS_INFORMATION, and makes the device the owner of that
 * bus for resources that the device tree does not lead to.
 */
VOID
NTAPI
IopRegisterLegacyBus(
    _In_ PDEVICE_NODE DeviceNode)
{
    PLEGACY_BUS_INFORMATION BusInformation = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;

    PAGED_CODE();

    /* A device that is added again may provide another bus now */
    IopUnregisterLegacyBus(DeviceNode);

    RtlZeroMemory(&Stack, sizeof(Stack));
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_LEGACY_BUS_INFORMATION,
                               &Stack);
    if (NT_SUCCESS(Status))
        BusInformation = (PLEGACY_BUS_INFORMATION)IoStatusBlock.Information;

    if (BusInformation == NULL)
    {
        if (NT_SUCCESS(Status))
        {
            DPRINT1("%wZ succeeded IRP_MN_QUERY_LEGACY_BUS_INFORMATION without data\n",
                    &DeviceNode->InstancePath);
        }

        DeviceNode->InterfaceType = InterfaceTypeUndefined;
        DeviceNode->BusNumber = IOP_NO_LEGACY_BUS_NUMBER;
        return;
    }

    DeviceNode->InterfaceType = BusInformation->LegacyBusType;
    DeviceNode->BusNumber = BusInformation->BusNumber;
    ExFreePool(BusInformation);

    IopInsertLegacyBus(DeviceNode);
}

/* Removes a device node that leaves the device tree from the legacy bus lists */
VOID
NTAPI
IopUnregisterLegacyBus(
    _In_ PDEVICE_NODE DeviceNode)
{
    /* Most device nodes never provide a legacy bus */
    if (IsListEmpty(&DeviceNode->LegacyBusListEntry))
        return;

    IopAcquireLegacyBuses(TRUE);

    RemoveEntryList(&DeviceNode->LegacyBusListEntry);
    InitializeListHead(&DeviceNode->LegacyBusListEntry);

    IopReleaseLegacyBuses();
}

/**
 * @brief
 * Returns the bus a resource belongs to when the device tree does not lead to
 * a translator. An internal resource uses ISA bus 0 when there is no internal
 * bus.
 *
 * @param[in] ListInterfaceType
 * The interface type of the whole resource list, which decides the fallback.
 */
static
PDEVICE_NODE
NTAPI
IopFindResourceBus(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ INTERFACE_TYPE ListInterfaceType)
{
    PDEVICE_NODE Bus = IopLookupLegacyBus(InterfaceType, BusNumber);

    if (Bus == IopRootDeviceNode && ListInterfaceType == Internal)
        Bus = IopLookupLegacyBus(Isa, 0);

    return Bus;
}

/**
 * @brief
 * Returns the device node a resource path goes through for Node. The root is
 * replaced once by the bus that provides the legacy bus of the resource, as
 * long as the path is still allowed to use it.
 */
PDEVICE_NODE
NTAPI
IopResourcePathNode(
    _Inout_ PIOP_RESOURCE_PATH Path,
    _In_opt_ PDEVICE_NODE Node)
{
    if (Node != IopRootDeviceNode || !Path->CanUseLegacyBus)
        return Node;

    Path->CanUseLegacyBus = FALSE;
    return IopFindResourceBus(Path->InterfaceType, Path->BusNumber, Path->ListInterfaceType);
}

/**
 * @brief
 * Returns the device node above a device node for resources. Device nodes of
 * legacy resource claims are not linked into the device tree, and belong to
 * the root.
 */
PDEVICE_NODE
NTAPI
IopGetResourceParent(
    _In_ PDEVICE_NODE DeviceNode)
{
    if (DeviceNode->Parent == NULL && DeviceNode != IopRootDeviceNode)
        return IopRootDeviceNode;

    return DeviceNode->Parent;
}

/* TRANSLATION **************************************************************/

/* Changes a requirement so that it cannot be satisfied, its minimum is above its maximum */
static
VOID
NTAPI
IopMakeRequirementUnsatisfiable(
    _Inout_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    switch (Descriptor->Type)
    {
        case CmResourceTypePort:
        case CmResourceTypeMemory:
        case CmResourceTypeMemoryLarge:
            Descriptor->u.Generic.MinimumAddress.QuadPart = 2;
            Descriptor->u.Generic.MaximumAddress.QuadPart = 1;
            break;

        case CmResourceTypeInterrupt:
            Descriptor->u.Interrupt.MinimumVector = 2;
            Descriptor->u.Interrupt.MaximumVector = 1;
            break;

        case CmResourceTypeDma:
            Descriptor->u.Dma.MinimumChannel = 2;
            Descriptor->u.Dma.MaximumChannel = 1;
            break;

        case CmResourceTypeBusNumber:
            Descriptor->u.BusNumber.MinBusNumber = 2;
            Descriptor->u.BusNumber.MaxBusNumber = 1;
            break;

        default:
            DPRINT1("Cannot restrict requirement type %u\n", Descriptor->Type);
            break;
    }
}

/**
 * @brief
 * Makes room for Count more descriptors in a translated alternative array.
 *
 * @return
 * FALSE if the allocation failed. The array is kept then.
 */
static
BOOLEAN
NTAPI
IopGrowAlternatives(
    _Inout_ PIO_RESOURCE_DESCRIPTOR *Array,
    _Inout_ PULONG Capacity,
    _In_ ULONG Used,
    _In_ ULONG Count)
{
    PIO_RESOURCE_DESCRIPTOR Larger;
    ULONG NewCapacity;

    if (Used + Count <= *Capacity)
        return TRUE;

    NewCapacity = max(Used + Count, *Capacity * 2);
    Larger = ExAllocatePoolWithTag(PagedPool, NewCapacity * sizeof(*Larger), TAG_IO_TRANSLATOR);
    if (Larger == NULL)
        return FALSE;

    if (*Array != NULL)
    {
        RtlCopyMemory(Larger, *Array, Used * sizeof(*Larger));
        ExFreePoolWithTag(*Array, TAG_IO_TRANSLATOR);
    }

    *Array = Larger;
    *Capacity = NewCapacity;
    return TRUE;
}

/**
 * @brief
 * Translates the alternatives of a requirement to the resource space of the
 * parent bus. An alternative the translator does not translate is kept, but
 * cannot be satisfied.
 *
 * @param[out] Translated
 * Receives the translated alternatives, from paged pool.
 *
 * @return
 * STATUS_TRANSLATION_COMPLETE if an alternative was translated up to the root,
 * another success status if any alternative was translated, or the status of
 * the last alternative when none was.
 */
NTSTATUS
NTAPI
IopTranslateRequirement(
    _In_ PTRANSLATOR_INTERFACE Translator,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_reads_(AlternativeCount) PIO_RESOURCE_DESCRIPTOR Alternatives,
    _In_ ULONG AlternativeCount,
    _Out_ PIO_RESOURCE_DESCRIPTOR *Translated,
    _Out_ PULONG TranslatedCount)
{
    PIO_RESOURCE_DESCRIPTOR Output = NULL;
    NTSTATUS LastStatus = STATUS_UNSUCCESSFUL;
    NTSTATUS LastSuccess = STATUS_SUCCESS;
    BOOLEAN IsComplete = FALSE;
    BOOLEAN HasTranslation = FALSE;
    BOOLEAN IsOutOfMemory = FALSE;
    ULONG Capacity = 0;
    ULONG Used = 0;
    ULONG Index;

    PAGED_CODE();

    *Translated = NULL;
    *TranslatedCount = 0;

    if (AlternativeCount == 0)
        return STATUS_INVALID_PARAMETER;

    /* Every alternative takes at least one slot */
    if (!IopGrowAlternatives(&Output, &Capacity, 0, AlternativeCount))
        return STATUS_INSUFFICIENT_RESOURCES;

    for (Index = 0; Index < AlternativeCount && !IsOutOfMemory; Index++)
    {
        PIO_RESOURCE_DESCRIPTOR Descriptors = NULL;
        ULONG Count = 0;

        LastStatus = Translator->TranslateResourceRequirements(Translator->Context,
                                                               &Alternatives[Index],
                                                               PhysicalDeviceObject,
                                                               &Count,
                                                               &Descriptors);
        if (NT_SUCCESS(LastStatus))
        {
            LastSuccess = LastStatus;
            IsComplete |= (LastStatus == STATUS_TRANSLATION_COMPLETE);
        }

        if (NT_SUCCESS(LastStatus) && Count != 0)
        {
            if (IopGrowAlternatives(&Output, &Capacity, Used, Count))
            {
                RtlCopyMemory(&Output[Used], Descriptors, Count * sizeof(*Output));
                Used += Count;
                HasTranslation = TRUE;
            }
            else
            {
                IsOutOfMemory = TRUE;
            }
        }
        else if (IopGrowAlternatives(&Output, &Capacity, Used, 1))
        {
            DPRINT("Requirement %lu of %p was not translated (Status 0x%08lx)\n",
                   Index, PhysicalDeviceObject, LastStatus);

            Output[Used] = Alternatives[Index];
            IopMakeRequirementUnsatisfiable(&Output[Used]);
            Used++;
        }
        else
        {
            IsOutOfMemory = TRUE;
        }

        if (Descriptors != NULL)
            ExFreePool(Descriptors);
    }

    if (IsOutOfMemory)
    {
        ExFreePoolWithTag(Output, TAG_IO_TRANSLATOR);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!HasTranslation)
    {
        DPRINT1("No alternative of %p was translated (Status 0x%08lx)\n",
                PhysicalDeviceObject, LastStatus);

        if (!NT_SUCCESS(LastStatus))
        {
            ExFreePoolWithTag(Output, TAG_IO_TRANSLATOR);
            return LastStatus;
        }
    }

    *Translated = Output;
    *TranslatedCount = Used;

    if (!HasTranslation)
        return LastStatus;

    return IsComplete ? STATUS_TRANSLATION_COMPLETE : LastSuccess;
}

/**
 * @brief
 * Translates an assigned resource to the resource the processor uses, with the
 * translators cached on the device node and above it. When the root is
 * reached, the translation continues from the bus that provides the legacy bus
 * of the resource, unless the resource was reported by the HAL.
 *
 * @param[in] DeviceNode
 * The device the resource is assigned to, or NULL to start at the legacy bus.
 *
 * @param[out] Translated
 * Receives the translated resource.
 *
 * @return
 * The status of the last translator that was called, or its failure status.
 */
NTSTATUS
NTAPI
IopTranslateResourceToRoot(
    _In_opt_ PDEVICE_NODE DeviceNode,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Raw,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated)
{
    PDEVICE_OBJECT PhysicalDeviceObject = NULL;
    CM_PARTIAL_RESOURCE_DESCRIPTOR Current = *Raw;
    NTSTATUS Status = STATUS_SUCCESS;
    IOP_RESOURCE_PATH Path;
    PDEVICE_NODE Node;

    PAGED_CODE();

    Path.InterfaceType = InterfaceType;
    Path.BusNumber = BusNumber;
    Path.ListInterfaceType = InterfaceType;

    /* The HAL reports resources as the processor sees them */
    Path.CanUseLegacyBus = (RequestSource != ArbiterRequestHalReported);

    if (DeviceNode != NULL)
    {
        Node = DeviceNode;
        PhysicalDeviceObject = DeviceNode->PhysicalDeviceObject;
    }
    else
    {
        Node = IopLookupLegacyBus(InterfaceType, BusNumber);
    }

    for (Node = IopResourcePathNode(&Path, Node);
         Node != NULL && Status != STATUS_TRANSLATION_COMPLETE;
         Node = IopResourcePathNode(&Path, IopGetResourceParent(Node)))
    {
        CM_PARTIAL_RESOURCE_DESCRIPTOR Next;
        PTRANSLATOR_INTERFACE Translator;

        /* The translators of a path are looked up for the type the device was assigned */
        if (!IopGetCachedTranslator(Node, Raw->Type, &Translator) || Translator == NULL)
            continue;

        Status = Translator->TranslateResources(Translator->Context,
                                                &Current,
                                                TranslateChildToParent,
                                                0,
                                                NULL,
                                                PhysicalDeviceObject,
                                                &Next);
        if (!NT_SUCCESS(Status))
        {
            if (DeviceNode != NULL && Status != STATUS_RETRY)
            {
                DPRINT1("Translator of %wZ failed for %wZ, type %u data %lx %lx %lx"
                        " (Status 0x%08lx)\n",
                        &Node->InstancePath, &DeviceNode->InstancePath, Current.Type,
                        Current.u.DevicePrivate.Data[0], Current.u.DevicePrivate.Data[1],
                        Current.u.DevicePrivate.Data[2], Status);
            }

            return Status;
        }

        Current = Next;
    }

    *Translated = Current;
    return Status;
}

/* Converts an address space of IoTranslateBusAddress to a one byte resource */
static
BOOLEAN
NTAPI
IopAddressToResource(
    _In_ ULONG AddressSpace,
    _In_ PHYSICAL_ADDRESS Address,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource)
{
    RtlZeroMemory(Resource, sizeof(*Resource));
    Resource->ShareDisposition = CmResourceShareDeviceExclusive;
    Resource->u.Generic.Start = Address;
    Resource->u.Generic.Length = 1;

    if (AddressSpace == 0)
    {
        Resource->Type = CmResourceTypeMemory;
        Resource->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        return TRUE;
    }

    if (AddressSpace == 1)
    {
        Resource->Type = CmResourceTypePort;
        Resource->Flags = CM_RESOURCE_PORT_IO;
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief
 * Translates a resource with the translator of one bus. A translator that is
 * not cached yet is asked for, used once and released.
 *
 * @return
 * The status of the translator, or STATUS_SUCCESS when the bus has none.
 */
static
NTSTATUS
NTAPI
IopTranslateOnBus(
    _In_ PDEVICE_NODE Node,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Resource,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    _Out_ PBOOLEAN HasTranslator)
{
    TRANSLATOR_INTERFACE Queried;
    PTRANSLATOR_INTERFACE Translator;
    NTSTATUS Status;

    *HasTranslator = FALSE;

    if (IopGetCachedTranslator(Node, Resource->Type, &Translator))
    {
        if (Translator == NULL)
            return STATUS_SUCCESS;

        *HasTranslator = TRUE;

        return Translator->TranslateResources(Translator->Context,
                                              Resource,
                                              TranslateChildToParent,
                                              0,
                                              NULL,
                                              NULL,
                                              Translated);
    }

    if (!NT_SUCCESS(IopQueryTranslatorInterface(Node, Resource->Type, &Queried)))
        return STATUS_SUCCESS;

    *HasTranslator = TRUE;

    Status = Queried.TranslateResources(Queried.Context,
                                        Resource,
                                        TranslateChildToParent,
                                        0,
                                        NULL,
                                        NULL,
                                        Translated);

    if (Queried.InterfaceDereference != NULL)
        Queried.InterfaceDereference(Queried.Context);

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoTranslateBusAddress(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ PHYSICAL_ADDRESS BusAddress,
    _Inout_ PULONG AddressSpace,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress)
{

    CM_PARTIAL_RESOURCE_DESCRIPTOR Resource;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_NODE Node;

    /* Translators are only called at PASSIVE_LEVEL, and need the device tree */
    if (KeGetCurrentIrql() > PASSIVE_LEVEL || IopRootDeviceNode == NULL)
    {
        *TranslatedAddress = BusAddress;
        return TRUE;
    }

    if (!IopAddressToResource(*AddressSpace, BusAddress, &Resource))
        return FALSE;

    /* The bus and its parents must stay in the device tree during the walk */
    IopAcquireLegacyBuses(FALSE);

    /* The root bus does not translate, so the walk ends below it */
    for (Node = IopLookupLegacyBus(InterfaceType, BusNumber);
         Node != NULL && Node != IopRootDeviceNode && Status != STATUS_TRANSLATION_COMPLETE;
         Node = IopGetResourceParent(Node))
    {
        CM_PARTIAL_RESOURCE_DESCRIPTOR Next;
        BOOLEAN HasTranslator;

        Status = IopTranslateOnBus(Node, &Resource, &Next, &HasTranslator);
        if (!NT_SUCCESS(Status))
            break;

        if (!HasTranslator)
            continue;

        Resource = Next;
    }

    IopReleaseLegacyBuses();

    if (!NT_SUCCESS(Status))
        return FALSE;

    if (Resource.Type == CmResourceTypePort)
        *AddressSpace = 1;
    else if (Resource.Type == CmResourceTypeMemory || Resource.Type == CmResourceTypeMemoryLarge)
        *AddressSpace = 0;
    else
        return FALSE;

    *TranslatedAddress = Resource.u.Generic.Start;
    return TRUE;
}

/* EOF */
