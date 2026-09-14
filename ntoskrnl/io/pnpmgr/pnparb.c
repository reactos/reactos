/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP manager resource arbitration and assignment
 * COPYRIGHT:   Copyright 2005 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include <wdmguid.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define TAG_IO_ARBITER 'AbrI'

/* Initialized by IopInitializeArbiters */
extern ARBITER_INSTANCE IopRootBusNumberArbiter;
extern ARBITER_INSTANCE IopRootIrqArbiter;
extern ARBITER_INSTANCE IopRootDmaArbiter;
extern ARBITER_INSTANCE IopRootMemArbiter;
extern ARBITER_INSTANCE IopRootPortArbiter;

static const struct
{
    UCHAR ResourceType;
    PARBITER_INSTANCE Instance;
} IopRootArbiterTable[] =
{
    { CmResourceTypePort,      &IopRootPortArbiter },
    { CmResourceTypeInterrupt, &IopRootIrqArbiter },
    { CmResourceTypeMemory,    &IopRootMemArbiter },
    { CmResourceTypeDma,       &IopRootDmaArbiter },
    { CmResourceTypeBusNumber, &IopRootBusNumberArbiter },
};

/* Interfaces of the root arbiters, in the same order as IopRootArbiterTable */
static ARBITER_INTERFACE IopRootArbiterInterface[RTL_NUMBER_OF(IopRootArbiterTable)];

/*
 * Arbiter cached on a device node, with its own copy of the interface. An entry
 * without an interface records a device that has no arbiter for the type.
 */
typedef struct _IOP_ARBITER_ENTRY
{
    PI_RESOURCE_ARBITER_ENTRY Entry;
    ARBITER_INTERFACE Interface;
} IOP_ARBITER_ENTRY, *PIOP_ARBITER_ENTRY;

/* Resource types with a bit in the arbiter masks of a device node */
#define IOP_MASKED_RESOURCE_TYPES (RTL_FIELD_SIZE(DEVICE_NODE, NoArbiterMask) * 8)

/*
 * Until the boot bus extenders are loaded, the resources reported by the HAL
 * and the boot configurations of made up devices wait for the legacy bus
 * they are on, whose arbiters may not exist yet.
 */
typedef struct _IOP_PENDING_BOOT_CONFIG
{
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
} IOP_PENDING_BOOT_CONFIG, *PIOP_PENDING_BOOT_CONFIG;

static LIST_ENTRY IopPendingBootConfigList;
static BOOLEAN IopHoldRootBootConfigs = TRUE;

/* Raw resources reported by the HAL that are not reserved yet */
static PCM_RESOURCE_LIST IopHalPendingResources;

/* The device node that owns the resources reported by the HAL */
static PDEVICE_NODE IopHalOwnerNode;

/*
 * Resources a legacy driver claimed for no device. They are owned by a
 * device object of the PnP manager, whose device node is outside the device
 * tree.
 */
typedef struct _IOP_LEGACY_RESOURCE_OWNER
{
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_NODE DeviceNode;
} IOP_LEGACY_RESOURCE_OWNER, *PIOP_LEGACY_RESOURCE_OWNER;

static LIST_ENTRY IopLegacyResourceOwnerList;

/* TRUE once IopRegisterRootArbiters has initialized the lock and the lists */
static BOOLEAN IopResourceAssignmentReady = FALSE;

/* Serializes all arbiter operations */
static ERESOURCE IopResourceAssignmentLock;

/* TRUE when a device of the last assignment waits for a later one */
static BOOLEAN IopIsAssignmentRetryNeeded;

/*
 * One translation level of a device requirement. Every translator below the
 * arbiter adds a level, and the arbiter gets the entry of the top one.
 */
typedef struct _IOP_REQUIREMENT_LEVEL
{
    struct _IOP_REQUIREMENT_LEVEL *Lower;
    PTRANSLATOR_INTERFACE Translator;
    ARBITER_LIST_ENTRY Entry;
    CM_PARTIAL_RESOURCE_DESCRIPTOR Assignment;
} IOP_REQUIREMENT_LEVEL, *PIOP_REQUIREMENT_LEVEL;

typedef struct _IOP_REQUIREMENT
{
    IOP_REQUIREMENT_LEVEL Device;
    PIOP_REQUIREMENT_LEVEL Top;
    PPI_RESOURCE_ARBITER_ENTRY Arbiter;
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
} IOP_REQUIREMENT, *PIOP_REQUIREMENT;

/* Requirement descriptor that sets the legacy bus of the descriptors after it */
#define IOP_RESOURCE_TYPE_LEGACY_BUS 0xF0

/* pnpinit.c */
extern INTERFACE_TYPE PnpDefaultInterfaceType;

/* sdk/lib/rtl/memres.c */
ULONGLONG
NTAPI
RtlCmDecodeMemIoResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_opt_ PULONGLONG Start);

NTSTATUS
NTAPI
RtlIoEncodeMemIoResource(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Type,
    _In_ ULONGLONG Length,
    _In_ ULONGLONG Alignment,
    _In_ ULONGLONG MinimumAddress,
    _In_ ULONGLONG MaximumAddress);

/* FUNCTIONS ****************************************************************/

/* LOCKING *******************************************************************/

static
VOID
NTAPI
IopLockResourceAssignment(VOID)
{
    ASSERT(IopResourceAssignmentReady);

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopResourceAssignmentLock, TRUE);
}

static
VOID
NTAPI
IopUnlockResourceAssignment(VOID)
{
    ExReleaseResourceLite(&IopResourceAssignmentLock);
    KeLeaveCriticalRegion();
}

/**
 * @brief
 * Keeps the cached arbiters, translators and resource lists from being freed
 * or replaced, for a caller that only reads them.
 *
 * @return
 * TRUE if the lock was taken, which is not possible before the root arbiters
 * are registered.
 */
BOOLEAN
NTAPI
IopLockResourceHandlers(VOID)
{
    if (!IopResourceAssignmentReady)
        return FALSE;

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&IopResourceAssignmentLock, TRUE);
    return TRUE;
}

VOID
NTAPI
IopUnlockResourceHandlers(VOID)
{
    IopUnlockResourceAssignment();
}

/* ARBITER DISCOVERY ********************************************************/

/* The root arbiters are never freed, so there is nothing to count */
static
VOID
NTAPI
IopRootArbiterReference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

static
VOID
NTAPI
IopRootArbiterDereference(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
}

/* Large memory ranges are arbitrated with the memory ranges */
static
UCHAR
NTAPI
IopArbitratedType(
    _In_ UCHAR ResourceType)
{
    return (ResourceType == CmResourceTypeMemoryLarge) ? CmResourceTypeMemory : ResourceType;
}

/**
 * @brief
 * Returns the root arbiter interface for a resource type, or NULL.
 * CmResourceTypeMemoryLarge uses the memory arbiter.
 */
static
PARBITER_INTERFACE
NTAPI
IopGetRootArbiterInterface(
    _In_ UCHAR ResourceType)
{
    ULONG Index;

    PAGED_CODE();

    ResourceType = IopArbitratedType(ResourceType);

    for (Index = 0; Index < RTL_NUMBER_OF(IopRootArbiterTable); Index++)
    {
        if (IopRootArbiterTable[Index].ResourceType == ResourceType)
            return &IopRootArbiterInterface[Index];
    }

    return NULL;
}

static
VOID
NTAPI
IopInitializeArbiterEntry(
    _Out_ PPI_RESOURCE_ARBITER_ENTRY Entry,
    _In_ UCHAR ResourceType,
    _In_opt_ PARBITER_INTERFACE Interface,
    _In_ ULONG Level)
{
    Entry->ResourceType = ResourceType;
    Entry->ArbiterInterface = Interface;
    Entry->Level = Level;
    InitializeListHead(&Entry->ResourceList);
    InitializeListHead(&Entry->BestResourceList);
    InitializeListHead(&Entry->BestConfig);
    InitializeListHead(&Entry->ActiveArbiterList);
}

/* Returns the arbiter entry of a resource type cached on a device node, or NULL */
static
PPI_RESOURCE_ARBITER_ENTRY
NTAPI
IopFindArbiterEntry(
    _In_ PDEVICE_NODE Node,
    _In_ UCHAR ResourceType)
{
    PLIST_ENTRY ListEntry;

    for (ListEntry = Node->DeviceArbiterList.Flink;
         ListEntry != &Node->DeviceArbiterList;
         ListEntry = ListEntry->Flink)
    {
        PPI_RESOURCE_ARBITER_ENTRY Entry =
            CONTAINING_RECORD(ListEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);

        if (Entry->ResourceType == ResourceType)
            return Entry;
    }

    return NULL;
}

/**
 * @brief
 * Sends IRP_MN_QUERY_INTERFACE for GUID_ARBITER_INTERFACE_STANDARD to a device.
 *
 * @remarks
 * The interface version is 0. The PCI driver refuses any other version, and
 * PCI devices would then be assigned from the root arbiters instead.
 */
static
NTSTATUS
NTAPI
IopQueryArbiterInterface(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR ResourceType,
    _Out_ PARBITER_INTERFACE Interface)
{
    IO_STATUS_BLOCK IoStatusBlock;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;

    RtlZeroMemory(Interface, sizeof(*Interface));

    if (!IopCanQueryResourceHandlers(DeviceNode))
        return STATUS_NOT_SUPPORTED;

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.Parameters.QueryInterface.InterfaceType = &GUID_ARBITER_INTERFACE_STANDARD;
    Stack.Parameters.QueryInterface.Size = sizeof(*Interface);
    Stack.Parameters.QueryInterface.Version = 0;
    Stack.Parameters.QueryInterface.Interface = (PINTERFACE)Interface;
    Stack.Parameters.QueryInterface.InterfaceSpecificData = UlongToPtr(ResourceType);

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_INTERFACE,
                               &Stack);
    if (NT_SUCCESS(Status) && Interface->ArbiterHandler == NULL)
    {
        DPRINT1("%wZ returned an arbiter without handler\n", &DeviceNode->InstancePath);

        if (Interface->InterfaceDereference != NULL)
            Interface->InterfaceDereference(Interface->Context);

        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

/**
 * @brief
 * Adds the root arbiters to the arbiter list of the root device node.
 * Called once, after IopInitializeArbiters.
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
IopRegisterRootArbiters(
    _In_ PDEVICE_NODE RootNode)
{
    ULONG Index;

    for (Index = 0; Index < RTL_NUMBER_OF(IopRootArbiterTable); Index++)
    {
        PARBITER_INTERFACE Interface = &IopRootArbiterInterface[Index];
        PPI_RESOURCE_ARBITER_ENTRY Entry;

        Interface->Size = sizeof(*Interface);
        Interface->Version = 0;
        Interface->Context = IopRootArbiterTable[Index].Instance;
        Interface->InterfaceReference = IopRootArbiterReference;
        Interface->InterfaceDereference = IopRootArbiterDereference;
        Interface->ArbiterHandler = ArbiterLibHandler;
        Interface->Flags = 0;

        Entry = ExAllocatePoolZero(PagedPool, sizeof(*Entry), TAG_IO_ARBITER);
        if (Entry == NULL)
        {
            while (!IsListEmpty(&RootNode->DeviceArbiterList))
            {
                ExFreePoolWithTag(CONTAINING_RECORD(RemoveHeadList(&RootNode->DeviceArbiterList),
                                                    PI_RESOURCE_ARBITER_ENTRY,
                                                    DeviceArbiterList),
                                  TAG_IO_ARBITER);
            }

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IopInitializeArbiterEntry(Entry,
                                  IopRootArbiterTable[Index].ResourceType,
                                  Interface,
                                  RootNode->Level);
        InsertTailList(&RootNode->DeviceArbiterList, &Entry->DeviceArbiterList);
    }

    ExInitializeResourceLite(&IopResourceAssignmentLock);
    InitializeListHead(&IopPendingBootConfigList);
    InitializeListHead(&IopLegacyResourceOwnerList);
    IopResourceAssignmentReady = TRUE;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Handles IRP_MN_QUERY_INTERFACE on the root PDO for the root arbiters.
 *
 * @param[in] IoStack
 * The stack location of the IRP. InterfaceSpecificData is the resource type.
 *
 * @param[in] ExistingStatus
 * The current status of the IRP, returned when the query is not handled.
 *
 * @return
 * STATUS_SUCCESS if the interface was returned, STATUS_INVALID_PARAMETER for
 * a resource type without a root arbiter, ExistingStatus otherwise.
 */
NTSTATUS
NTAPI
IopArbiterQueryRootInterface(
    _In_ PIO_STACK_LOCATION IoStack,
    _In_ NTSTATUS ExistingStatus)
{
    PARBITER_INTERFACE Interface;
    PARBITER_INTERFACE Output;
    UCHAR ResourceType;
    BOOLEAN IsSupported;

    PAGED_CODE();

    IsSupported = IsEqualGUID(IoStack->Parameters.QueryInterface.InterfaceType,
                              &GUID_ARBITER_INTERFACE_STANDARD) &&
                  IoStack->Parameters.QueryInterface.Size >= sizeof(*Output) &&
                  IoStack->Parameters.QueryInterface.Interface != NULL;
    if (!IsSupported)
        return ExistingStatus;

    ResourceType = (UCHAR)(ULONG_PTR)IoStack->Parameters.QueryInterface.InterfaceSpecificData;

    Interface = IopGetRootArbiterInterface(ResourceType);
    if (Interface == NULL)
        return STATUS_INVALID_PARAMETER;

    /* The handler is NULL until IopRegisterRootArbiters has run */
    if (Interface->ArbiterHandler == NULL)
        return ExistingStatus;

    Output = (PARBITER_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
    *Output = *Interface;
    Output->InterfaceReference(Output->Context);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Returns the arbiter of a device node for a resource type. The stack of the
 * device is asked once, and the answer is cached on the device node.
 *
 * @return
 * STATUS_SUCCESS, STATUS_NOT_FOUND if the device has no arbiter for the type,
 * or STATUS_INSUFFICIENT_RESOURCES.
 */
static
NTSTATUS
NTAPI
IopGetDeviceArbiter(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR ResourceType,
    _Out_ PPI_RESOURCE_ARBITER_ENTRY *ArbiterEntry)
{
    USHORT TypeBit = (ResourceType < IOP_MASKED_RESOURCE_TYPES) ? (USHORT)(1 << ResourceType) : 0;
    PPI_RESOURCE_ARBITER_ENTRY Cached;
    PIOP_ARBITER_ENTRY NewEntry;
    ARBITER_INTERFACE Queried;
    NTSTATUS Status;

    *ArbiterEntry = NULL;

    Cached = IopFindArbiterEntry(DeviceNode, ResourceType);
    if (Cached != NULL)
    {
        if (Cached->ArbiterInterface == NULL)
            return STATUS_NOT_FOUND;

        *ArbiterEntry = Cached;
        return STATUS_SUCCESS;
    }

    if (DeviceNode->NoArbiterMask & TypeBit)
        return STATUS_NOT_FOUND;

    Status = IopQueryArbiterInterface(DeviceNode, ResourceType, &Queried);
    DeviceNode->QueryArbiterMask |= TypeBit;

    if (!NT_SUCCESS(Status))
    {
        DeviceNode->NoArbiterMask |= TypeBit;
        if (TypeBit != 0)
            return STATUS_NOT_FOUND;
    }

    NewEntry = ExAllocatePoolZero(PagedPool, sizeof(*NewEntry), TAG_IO_ARBITER);
    if (NewEntry == NULL)
    {
        if (NT_SUCCESS(Status) && Queried.InterfaceDereference != NULL)
            Queried.InterfaceDereference(Queried.Context);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewEntry->Interface = Queried;
    IopInitializeArbiterEntry(&NewEntry->Entry,
                              ResourceType,
                              NT_SUCCESS(Status) ? &NewEntry->Interface : NULL,
                              DeviceNode->Level);
    InsertTailList(&DeviceNode->DeviceArbiterList, &NewEntry->Entry.DeviceArbiterList);

    if (!NT_SUCCESS(Status))
        return STATUS_NOT_FOUND;

    *ArbiterEntry = &NewEntry->Entry;
    return STATUS_SUCCESS;
}

/* Releases the arbiters cached on a device node */
static
VOID
NTAPI
IopFreeDeviceNodeArbiters(
    _In_ PDEVICE_NODE DeviceNode)
{
    while (!IsListEmpty(&DeviceNode->DeviceArbiterList))
    {
        PPI_RESOURCE_ARBITER_ENTRY Entry =
            CONTAINING_RECORD(RemoveHeadList(&DeviceNode->DeviceArbiterList),
                              PI_RESOURCE_ARBITER_ENTRY,
                              DeviceArbiterList);

        if ((Entry->ArbiterInterface != NULL) &&
            (Entry->ArbiterInterface->InterfaceDereference != NULL))
        {
            Entry->ArbiterInterface->InterfaceDereference(Entry->ArbiterInterface->Context);
        }

        ExFreePoolWithTag(Entry, TAG_IO_ARBITER);
    }

    DeviceNode->NoArbiterMask = 0;
    DeviceNode->QueryArbiterMask = 0;
}

/**
 * @brief
 * Releases the arbiters and translators cached on a device node. The device
 * is asked again the next time they are needed.
 *
 * @remarks
 * The root arbiters cannot be asked for again, so the root keeps them.
 */
VOID
NTAPI
IopUncacheResourceHandlers(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    /* Nothing is cached before the root arbiters are registered */
    if (!IopResourceAssignmentReady || DeviceNode == IopRootDeviceNode)
        return;

    IopLockResourceAssignment();

    IopFreeDeviceNodeArbiters(DeviceNode);
    IopFreeDeviceNodeTranslators(DeviceNode);

    IopUnlockResourceAssignment();
}

/**
 * @brief
 * Asks an arbiter that only arbitrates some of the ranges of its type whether
 * it arbitrates a requirement.
 */
static
BOOLEAN
NTAPI
IopArbiterTakesRequirement(
    _In_ PARBITER_INTERFACE Interface,
    _Inout_ PARBITER_LIST_ENTRY Entry)
{
    ARBITER_PARAMETERS Parameters;
    LIST_ENTRY ArbitrationList;
    NTSTATUS Status;

    InitializeListHead(&ArbitrationList);
    InsertTailList(&ArbitrationList, &Entry->ListEntry);

    RtlZeroMemory(&Parameters, sizeof(Parameters));
    Parameters.Parameters.QueryArbitrate.ArbitrationList = &ArbitrationList;

    Status = Interface->ArbiterHandler(Interface->Context,
                                      ArbiterActionQueryArbitrate,
                                      &Parameters);

    RemoveEntryList(&Entry->ListEntry);
    InitializeListHead(&Entry->ListEntry);

    return NT_SUCCESS(Status);
}

/**
 * @brief
 * Translates the top level of a requirement with the translator of a bus,
 * which adds a level for the parent of the bus.
 *
 * @return
 * The status of IopTranslateRequirement, or STATUS_INSUFFICIENT_RESOURCES.
 */
static
NTSTATUS
NTAPI
IopAddRequirementLevel(
    _Inout_ PIOP_REQUIREMENT Requirement,
    _In_ PTRANSLATOR_INTERFACE Translator)
{
    PIOP_REQUIREMENT_LEVEL Lower = Requirement->Top;
    PIOP_REQUIREMENT_LEVEL Level;
    PIO_RESOURCE_DESCRIPTOR Alternatives;
    ULONG AlternativeCount;
    NTSTATUS Status;

    Status = IopTranslateRequirement(Translator,
                                     Lower->Entry.PhysicalDeviceObject,
                                     Lower->Entry.Alternatives,
                                     Lower->Entry.AlternativeCount,
                                     &Alternatives,
                                     &AlternativeCount);
    if (!NT_SUCCESS(Status))
        return Status;

    Level = ExAllocatePoolWithTag(PagedPool, sizeof(*Level), TAG_IO_ARBITER);
    if (Level == NULL)
    {
        ExFreePool(Alternatives);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Level = *Lower;
    Level->Lower = Lower;
    Level->Translator = Translator;
    InitializeListHead(&Level->Entry.ListEntry);
    Level->Entry.Alternatives = Alternatives;
    Level->Entry.AlternativeCount = AlternativeCount;
    Level->Entry.Assignment = &Level->Assignment;

    Requirement->Top = Level;

    return Status;
}

/**
 * @brief
 * Initializes a requirement of a device before its arbiter and translators
 * are found.
 *
 * @param[in] InterfaceType
 * The legacy bus of the requirement, used when the device tree does not lead
 * to a translator.
 */
static
VOID
NTAPI
IopInitializeRequirement(
    _Out_ PIOP_REQUIREMENT Requirement,
    _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_reads_(AlternativeCount) PIO_RESOURCE_DESCRIPTOR Alternatives,
    _In_ ULONG AlternativeCount)
{
    PARBITER_LIST_ENTRY Entry = &Requirement->Device.Entry;

    RtlZeroMemory(Requirement, sizeof(*Requirement));
    Requirement->Top = &Requirement->Device;
    Requirement->InterfaceType = InterfaceType;
    Requirement->BusNumber = BusNumber;

    InitializeListHead(&Entry->ListEntry);
    Entry->Alternatives = Alternatives;
    Entry->AlternativeCount = AlternativeCount;
    Entry->PhysicalDeviceObject = PhysicalDeviceObject;
    Entry->RequestSource = RequestSource;
    Entry->InterfaceType = InterfaceType;
    Entry->BusNumber = BusNumber;
    Entry->Assignment = &Requirement->Device.Assignment;
    Entry->Result = ArbiterResultUndefined;

    Requirement->Device.Assignment.Type = CmResourceTypeMaximum;
}

/* Returns the resource type the top level of a requirement is arbitrated as */
static
UCHAR
NTAPI
IopRequirementType(
    _In_ PIOP_REQUIREMENT Requirement)
{
    return IopArbitratedType(Requirement->Top->Entry.Alternatives[0].Type);
}

/**
 * @brief
 * Gives a requirement the arbiter of a device node, if the node has one for
 * its type and the arbiter takes it.
 *
 * @return
 * STATUS_SUCCESS, or STATUS_INSUFFICIENT_RESOURCES.
 */
static
NTSTATUS
NTAPI
IopTakeNodeArbiter(
    _Inout_ PIOP_REQUIREMENT Requirement,
    _In_ PDEVICE_NODE Node)
{
    PPI_RESOURCE_ARBITER_ENTRY Arbiter;
    NTSTATUS Status;

    /* The arbiters of a device are for its children */
    if (Node->PhysicalDeviceObject == Requirement->Device.Entry.PhysicalDeviceObject)
        return STATUS_SUCCESS;

    Status = IopGetDeviceArbiter(Node, IopRequirementType(Requirement), &Arbiter);
    if (Status == STATUS_INSUFFICIENT_RESOURCES)
        return Status;

    if (Arbiter == NULL)
        return STATUS_SUCCESS;

    /* An arbiter of only some ranges of its type is asked first */
    if (!(Arbiter->ArbiterInterface->Flags & ARBITER_PARTIAL) ||
        IopArbiterTakesRequirement(Arbiter->ArbiterInterface, &Requirement->Top->Entry))
    {
        Requirement->Arbiter = Arbiter;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Translates a requirement with the translator of a device node, as long as
 * the requirement has no arbiter.
 *
 * @param[in,out] Path
 * The path of the requirement, which may not use the legacy bus once a
 * translator was found.
 *
 * @param[out] IsComplete
 * Set to TRUE when the translator translated the requirement up to the root.
 *
 * @return
 * STATUS_SUCCESS, or the failure status of the translation.
 */
static
NTSTATUS
NTAPI
IopApplyNodeTranslator(
    _Inout_ PIOP_REQUIREMENT Requirement,
    _In_ PDEVICE_NODE Node,
    _Inout_ PIOP_RESOURCE_PATH Path,
    _Out_ PBOOLEAN IsComplete)
{
    PTRANSLATOR_INTERFACE Translator;
    NTSTATUS Status;

    *IsComplete = FALSE;

    Status = IopGetDeviceTranslator(Node, IopRequirementType(Requirement), &Translator);
    if (Status == STATUS_INSUFFICIENT_RESOURCES)
        return Status;

    if (!NT_SUCCESS(Status))
        return STATUS_SUCCESS;

    Path->CanUseLegacyBus = FALSE;

    /* The arbiter takes the requirement as it is at its own level */
    if (Requirement->Arbiter != NULL)
        return STATUS_SUCCESS;

    Status = IopAddRequirementLevel(Requirement, Translator);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Requirement translation by %wZ failed (Status 0x%08lx)\n",
                &Node->InstancePath, Status);
        return Status;
    }

    *IsComplete = (Status == STATUS_TRANSLATION_COMPLETE);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Finds the arbiter of a requirement and the translators below it, walking up
 * the device tree from the device node, whose own arbiters are skipped.
 *
 * @param[in] ListInterfaceType
 * The interface type of the whole requirements list.
 *
 * @remarks
 * Reaching the root without a translator restarts the walk at the legacy bus.
 * HAL reported resources start at the root, and skip the legacy bus when internal.
 *
 * @return
 * STATUS_SUCCESS, STATUS_RESOURCE_TYPE_NOT_FOUND if no arbiter was found, or
 * the failure status of a translator.
 */
static
NTSTATUS
NTAPI
IopFindRequirementHandlers(
    _Inout_ PIOP_REQUIREMENT Requirement,
    _In_ INTERFACE_TYPE ListInterfaceType)
{
    PDEVICE_OBJECT PhysicalDeviceObject = Requirement->Device.Entry.PhysicalDeviceObject;
    BOOLEAN IsHalReported = (Requirement->Device.Entry.RequestSource == ArbiterRequestHalReported);
    BOOLEAN IsTranslationComplete = FALSE;
    IOP_RESOURCE_PATH Path;
    PDEVICE_NODE Node;
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(Requirement->Top == &Requirement->Device && Requirement->Arbiter == NULL);

    Path.InterfaceType = Requirement->InterfaceType;
    Path.BusNumber = Requirement->BusNumber;
    Path.ListInterfaceType = ListInterfaceType;

    /* The HAL knows when a resource is on the internal bus */
    Path.CanUseLegacyBus = !(IsHalReported && Requirement->InterfaceType == Internal);

    if (PhysicalDeviceObject != NULL && !IsHalReported)
        Node = IopGetDeviceNode(PhysicalDeviceObject);
    else
        Node = IopRootDeviceNode;

    for (Node = IopResourcePathNode(&Path, Node);
         Node != NULL;
         Node = IopResourcePathNode(&Path, IopGetResourceParent(Node)))
    {
        if (Requirement->Arbiter == NULL)
        {
            Status = IopTakeNodeArbiter(Requirement, Node);
            if (!NT_SUCCESS(Status))
                return Status;
        }

        if (!IsTranslationComplete)
        {
            Status = IopApplyNodeTranslator(Requirement, Node, &Path, &IsTranslationComplete);
            if (!NT_SUCCESS(Status))
                return Status;
        }
    }

    if (Requirement->Arbiter == NULL)
    {
        DPRINT1("No arbiter for resource type %u\n", IopRequirementType(Requirement));
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}

/* Frees the translated levels of a requirement */
static
VOID
NTAPI
IopFreeRequirementLevels(
    _Inout_ PIOP_REQUIREMENT Requirement)
{
    while (Requirement->Top != &Requirement->Device)
    {
        PIOP_REQUIREMENT_LEVEL Level = Requirement->Top;

        Requirement->Top = Level->Lower;
        ExFreePool(Level->Entry.Alternatives);
        ExFreePoolWithTag(Level, TAG_IO_ARBITER);
    }

    Requirement->Arbiter = NULL;
}

/**
 * @brief
 * Translates the assignment the arbiter made in the top level of a
 * requirement back down to the device, with the translator of each level.
 *
 * @return
 * STATUS_SUCCESS, STATUS_INVALID_PARAMETER if a level has no assignment, or
 * the failure status of a translator.
 */
static
NTSTATUS
NTAPI
IopTranslateAssignmentToDevice(
    _Inout_ PIOP_REQUIREMENT Requirement)
{
    PIOP_REQUIREMENT_LEVEL Level;

    PAGED_CODE();

    for (Level = Requirement->Top; Level != NULL; Level = Level->Lower)
    {
        PIOP_REQUIREMENT_LEVEL Lower = Level->Lower;
        NTSTATUS Status;

        if (Level->Entry.AlternativeCount == 0 || Level->Assignment.Type == CmResourceTypeMaximum)
        {
            DPRINT1("Requirement level without assignment\n");
            return STATUS_INVALID_PARAMETER;
        }

        if (Lower == NULL)
            break;

        Status = Level->Translator->TranslateResources(Level->Translator->Context,
                                                       &Level->Assignment,
                                                       TranslateParentToChild,
                                                       Lower->Entry.AlternativeCount,
                                                       Lower->Entry.Alternatives,
                                                       Lower->Entry.PhysicalDeviceObject,
                                                       &Lower->Assignment);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    return STATUS_SUCCESS;
}

/* BOOT CONFIGURATION RESERVATION *******************************************/

/* The requirements of one configuration of a device */
typedef struct _IOP_CONFIGURATION
{
    PIOP_REQUIREMENT Requirements;
    PIO_RESOURCE_DESCRIPTOR Descriptors;
    ULONG Count;
    ULONG Priority;
    NTSTATUS Status;
} IOP_CONFIGURATION, *PIOP_CONFIGURATION;

/**
 * @brief
 * Calls an arbiter action. ArbitrationList is NULL for the commit and
 * rollback actions, which take no parameters.
 */
static
NTSTATUS
NTAPI
IopArbiterInvoke(
    _In_ PARBITER_INTERFACE Interface,
    _In_ ARBITER_ACTION Action,
    _In_opt_ PLIST_ENTRY ArbitrationList)
{
    ARBITER_PARAMETERS Parameters;

    /* The list is the first field of every parameter block that has one */
    RtlZeroMemory(&Parameters, sizeof(Parameters));
    Parameters.Parameters.TestAllocation.ArbitrationList = ArbitrationList;

    return Interface->ArbiterHandler(Interface->Context, Action, &Parameters);
}

/* Checks if a requirement of this type is given to an arbiter */
static
BOOLEAN
NTAPI
IopIsArbitratedType(
    _In_ UCHAR Type)
{
    return !(Type & CmResourceTypeNonArbitrated) && Type != CmResourceTypeNull;
}

/* Resources of an undefined interface are on the default bus */
static
INTERFACE_TYPE
NTAPI
IopResourceInterface(
    _In_ INTERFACE_TYPE InterfaceType)
{
    return (InterfaceType == InterfaceTypeUndefined) ? PnpDefaultInterfaceType : InterfaceType;
}

/* Returns the partial descriptor that follows Descriptor, past its device specific data */
static
PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
IopNextPartialDescriptor(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor)
{
    ULONG DataSize = 0;

    if (Descriptor->Type == CmResourceTypeDeviceSpecific)
        DataSize = Descriptor->u.DeviceSpecificData.DataSize;

    return (PCM_PARTIAL_RESOURCE_DESCRIPTOR)((PUCHAR)(Descriptor + 1) + DataSize);
}

/* Returns the full descriptor that follows Full, past the device specific data it holds */
static
PCM_FULL_RESOURCE_DESCRIPTOR
NTAPI
IopNextFullDescriptor(
    _In_ PCM_FULL_RESOURCE_DESCRIPTOR Full)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial = &Full->PartialResourceList.PartialDescriptors[0];
    ULONG Index;

    for (Index = 0; Index < Full->PartialResourceList.Count; Index++)
        Partial = IopNextPartialDescriptor(Partial);

    return (PCM_FULL_RESOURCE_DESCRIPTOR)Partial;
}

/**
 * @brief
 * Converts an assigned descriptor to a requirement that only fits at the
 * same place.
 *
 * @return
 * FALSE for a device specific descriptor, which has no requirement.
 */
static
BOOLEAN
NTAPI
IopCmToFixedRequirement(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm,
    _Out_ PIO_RESOURCE_DESCRIPTOR Io)
{
    ULONGLONG Start;
    ULONGLONG Length;

    RtlZeroMemory(Io, sizeof(*Io));
    Io->Option = IO_RESOURCE_PREFERRED;
    Io->Type = Cm->Type;
    Io->ShareDisposition = Cm->ShareDisposition;
    Io->Flags = Cm->Flags;

    switch (Cm->Type)
    {
        case CmResourceTypeDeviceSpecific:
            return FALSE;

        case CmResourceTypeInterrupt:
            if (Cm->Flags & CM_RESOURCE_INTERRUPT_MESSAGE)
            {
                /* The messages of a device are counted down from the message token */
                Io->u.Interrupt.MaximumVector = CM_RESOURCE_INTERRUPT_MESSAGE_TOKEN;
                Io->u.Interrupt.MinimumVector = CM_RESOURCE_INTERRUPT_MESSAGE_TOKEN -
                                                Cm->u.MessageInterrupt.Raw.MessageCount + 1;
                Io->u.Interrupt.AffinityPolicy = IrqPolicySpecifiedProcessors;
                Io->u.Interrupt.PriorityPolicy = IrqPriorityUndefined;
                Io->u.Interrupt.TargetedProcessors = Cm->u.MessageInterrupt.Raw.Affinity;
#if defined(NT_PROCESSOR_GROUPS)
                Io->u.Interrupt.Group = Cm->u.MessageInterrupt.Raw.Group;
#endif
                return TRUE;
            }

#if defined(_M_IX86)
            /* On x86 the raw level is the bus IRQ */
            Io->u.Interrupt.MinimumVector = Cm->u.Interrupt.Level;
#else
            Io->u.Interrupt.MinimumVector = Cm->u.Interrupt.Vector;
#endif
            Io->u.Interrupt.MaximumVector = Io->u.Interrupt.MinimumVector;
            return TRUE;

        case CmResourceTypePort:
        case CmResourceTypeMemory:
        case CmResourceTypeMemoryLarge:
            /* The length stays encoded, since the large memory flags are copied too */
            Length = RtlCmDecodeMemIoResource(Cm, &Start);
            Io->u.Generic.Length = Cm->u.Generic.Length;
            Io->u.Generic.Alignment = 1;
            Io->u.Generic.MinimumAddress.QuadPart = Start;
            Io->u.Generic.MaximumAddress.QuadPart = Start + Length - 1;
            return TRUE;

        case CmResourceTypeDma:
            if (Cm->Flags & CM_RESOURCE_DMA_V3)
            {
                /* Reserved overlays MaximumChannel, so the request line is the channel range */
                Io->u.DmaV3.RequestLine = Cm->u.DmaV3.RequestLine;
                Io->u.DmaV3.Reserved = Cm->u.DmaV3.RequestLine;
                Io->u.DmaV3.Channel = Cm->u.DmaV3.Channel;
                Io->u.DmaV3.TransferWidth = Cm->u.DmaV3.TransferWidth;
                return TRUE;
            }

            Io->u.Dma.MinimumChannel = Cm->u.Dma.Channel;
            Io->u.Dma.MaximumChannel = Cm->u.Dma.Channel;
            return TRUE;

        case CmResourceTypeBusNumber:
            Io->u.BusNumber.MinBusNumber = Cm->u.BusNumber.Start;
            Io->u.BusNumber.MaxBusNumber = Cm->u.BusNumber.Start + Cm->u.BusNumber.Length - 1;
            Io->u.BusNumber.Length = Cm->u.BusNumber.Length;
            return TRUE;

        default:
            RtlCopyMemory(Io->u.DevicePrivate.Data,
                          Cm->u.DevicePrivate.Data,
                          sizeof(Io->u.DevicePrivate.Data));
            return TRUE;
    }
}

/* A failed translation is retried later only when the translator asks for it */
static
NTSTATUS
NTAPI
IopTranslationFailureStatus(
    _In_ NTSTATUS Status)
{
    return (Status == STATUS_RETRY) ? STATUS_RETRY : STATUS_INSUFFICIENT_RESOURCES;
}

/**
 * @brief
 * Adds the top entry of a requirement to the arbitration list of its arbiter.
 * The first entry of an arbiter also adds it to the active arbiters, where
 * the arbiters closer to the root come first.
 */
static
VOID
NTAPI
IopQueueRequirement(
    _Inout_ PLIST_ENTRY ActiveArbiters,
    _In_ PIOP_REQUIREMENT Requirement)
{
    PPI_RESOURCE_ARBITER_ENTRY Arbiter = Requirement->Arbiter;
    PLIST_ENTRY Next;

    InsertTailList(&Arbiter->ResourceList, &Requirement->Top->Entry.ListEntry);

    if (!IsListEmpty(&Arbiter->ActiveArbiterList))
        return;

    for (Next = ActiveArbiters->Flink; Next != ActiveArbiters; Next = Next->Flink)
    {
        PPI_RESOURCE_ARBITER_ENTRY Active =
            CONTAINING_RECORD(Next, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);

        if (Active->Level >= Arbiter->Level)
            break;
    }

    /* Goes before the arbiters of the same depth that are already there */
    InsertTailList(Next, &Arbiter->ActiveArbiterList);
}

/* Empties the arbitration lists of the active arbiters */
static
VOID
NTAPI
IopDequeueRequirements(
    _Inout_ PLIST_ENTRY ActiveArbiters)
{
    while (!IsListEmpty(ActiveArbiters))
    {
        PPI_RESOURCE_ARBITER_ENTRY Arbiter =
            CONTAINING_RECORD(RemoveHeadList(ActiveArbiters),
                              PI_RESOURCE_ARBITER_ENTRY,
                              ActiveArbiterList);

        InitializeListHead(&Arbiter->ActiveArbiterList);

        while (!IsListEmpty(&Arbiter->ResourceList))
        {
            PLIST_ENTRY Entry = RemoveHeadList(&Arbiter->ResourceList);
            InitializeListHead(Entry);
        }
    }
}

/* Frees the requirements of a configuration */
static
VOID
NTAPI
IopFreeConfiguration(
    _Inout_ PIOP_CONFIGURATION Configuration)
{
    ULONG Index;

    if (Configuration->Requirements != NULL)
    {
        for (Index = 0; Index < Configuration->Count; Index++)
            IopFreeRequirementLevels(&Configuration->Requirements[Index]);

        ExFreePoolWithTag(Configuration->Requirements, TAG_IO_ARBITER);
        Configuration->Requirements = NULL;
    }

    if (Configuration->Descriptors != NULL)
    {
        ExFreePoolWithTag(Configuration->Descriptors, TAG_IO_ARBITER);
        Configuration->Descriptors = NULL;
    }

    Configuration->Count = 0;
}

/**
 * @brief
 * Builds a fixed requirement for each descriptor of an assigned resource
 * list, and finds the arbiter and translators of the arbitrated ones.
 *
 * @param[out] ArbitratedCount
 * Receives the number of requirements that have an arbiter.
 *
 * @return
 * STATUS_SUCCESS, STATUS_UNSUCCESSFUL for a list without descriptors, or the
 * failure status of the handler search.
 */
static
NTSTATUS
NTAPI
IopBuildFixedConfiguration(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _Out_ PIOP_CONFIGURATION Configuration,
    _Out_ PULONG ArbitratedCount)
{
    PCM_FULL_RESOURCE_DESCRIPTOR Full = &ResourceList->List[0];
    INTERFACE_TYPE ListInterfaceType;
    BOOLEAN IsAfterArbitrated = FALSE;
    ULONG Total = 0;
    ULONG ListIndex;
    NTSTATUS Status;

    RtlZeroMemory(Configuration, sizeof(*Configuration));
    *ArbitratedCount = 0;

    for (ListIndex = 0; ListIndex < ResourceList->Count; ListIndex++)
    {
        Total += Full->PartialResourceList.Count;
        Full = IopNextFullDescriptor(Full);
    }

    if (Total == 0)
        return STATUS_UNSUCCESSFUL;

    ListInterfaceType = IopResourceInterface(ResourceList->List[0].InterfaceType);

    Configuration->Requirements = ExAllocatePoolZero(PagedPool,
                                                     Total * sizeof(*Configuration->Requirements),
                                                     TAG_IO_ARBITER);
    Configuration->Descriptors = ExAllocatePoolZero(PagedPool,
                                                    Total * sizeof(*Configuration->Descriptors),
                                                    TAG_IO_ARBITER);
    if (Configuration->Requirements == NULL || Configuration->Descriptors == NULL)
    {
        IopFreeConfiguration(Configuration);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Full = &ResourceList->List[0];
    for (ListIndex = 0; ListIndex < ResourceList->Count; ListIndex++)
    {
        INTERFACE_TYPE InterfaceType = IopResourceInterface(Full->InterfaceType);
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm = &Full->PartialResourceList.PartialDescriptors[0];
        ULONG Index;

        for (Index = 0; Index < Full->PartialResourceList.Count; Index++)
        {
            PIO_RESOURCE_DESCRIPTOR Io = &Configuration->Descriptors[Configuration->Count];
            PIOP_REQUIREMENT Requirement = &Configuration->Requirements[Configuration->Count];

            if (!IopCmToFixedRequirement(Cm, Io))
            {
                Cm = IopNextPartialDescriptor(Cm);
                continue;
            }

            /* Private data describes the resource before it */
            if (Io->Type == CmResourceTypeDevicePrivate && Configuration->Count == 0)
            {
                IopFreeConfiguration(Configuration);
                return STATUS_INVALID_PARAMETER;
            }

            IopInitializeRequirement(Requirement,
                                     DeviceNode->PhysicalDeviceObject,
                                     RequestSource,
                                     InterfaceType,
                                     Full->BusNumber,
                                     Io,
                                     1);

            /* The arbiter is given the bus of the whole list */
            Requirement->Device.Entry.BusNumber = ResourceList->List[0].BusNumber;
            Configuration->Count++;

            if (!IopIsArbitratedType(Io->Type))
            {
                PCM_PARTIAL_RESOURCE_DESCRIPTOR Copy = &Requirement->Device.Assignment;

                Copy->Type = Io->Type;
                Copy->ShareDisposition = Io->ShareDisposition;
                Copy->Flags = Io->Flags;
                RtlCopyMemory(Copy->u.DevicePrivate.Data,
                              Io->u.DevicePrivate.Data,
                              sizeof(Copy->u.DevicePrivate.Data));

                /* Private data of an arbitrated resource belongs to the device alone */
                if (Io->Type == CmResourceTypeDevicePrivate && IsAfterArbitrated)
                    Copy->ShareDisposition = CmResourceShareDeviceExclusive;
                else
                    IsAfterArbitrated = FALSE;
            }
            else
            {
                IsAfterArbitrated = TRUE;

                Status = IopFindRequirementHandlers(Requirement, ListInterfaceType);
                if (!NT_SUCCESS(Status))
                {
                    IopFreeConfiguration(Configuration);
                    return Status;
                }

                (*ArbitratedCount)++;
            }

            Cm = IopNextPartialDescriptor(Cm);
        }

        Full = (PCM_FULL_RESOURCE_DESCRIPTOR)Cm;
    }

    return (Configuration->Count != 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/**
 * @brief
 * Builds the raw and translated resource lists of a configuration the
 * arbiters committed. Descriptors without an arbiter are copied.
 *
 * @param[out] ResourceList
 * Receives the raw list, or NULL. Can be NULL when only the translated list is needed.
 *
 * @return
 * STATUS_SUCCESS, STATUS_RETRY if a translator asked to be called again later,
 * or STATUS_INSUFFICIENT_RESOURCES. A configuration without descriptors gets
 * no lists.
 */
static
NTSTATUS
NTAPI
IopBuildResourceLists(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PIOP_CONFIGURATION Configuration,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _Out_opt_ PCM_RESOURCE_LIST *ResourceList,
    _Out_ PCM_RESOURCE_LIST *TranslatedList)
{
    PCM_RESOURCE_LIST Lists[2] = { NULL, NULL };
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Size;
    ULONG Index;

    PAGED_CODE();

    if (ResourceList != NULL)
        *ResourceList = NULL;
    *TranslatedList = NULL;

    if (Configuration->Count == 0)
        return STATUS_SUCCESS;

    Size = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors) +
           Configuration->Count *
               sizeof(Lists[0]->List[0].PartialResourceList.PartialDescriptors[0]);

    Lists[0] = ExAllocatePoolZero(PagedPool, Size, TAG_IO_ARBITER);
    Lists[1] = ExAllocatePoolZero(PagedPool, Size, TAG_IO_ARBITER);
    if (Lists[0] == NULL || Lists[1] == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(Lists); Index++)
    {
        Lists[Index]->Count = 1;
        Lists[Index]->List[0].InterfaceType = IopResourceInterface(InterfaceType);
        Lists[Index]->List[0].BusNumber = BusNumber;
        Lists[Index]->List[0].PartialResourceList.Version = 1;
        Lists[Index]->List[0].PartialResourceList.Revision = 1;
        Lists[Index]->List[0].PartialResourceList.Count = Configuration->Count;
    }

    for (Index = 0; Index < Configuration->Count; Index++)
    {
        PIOP_REQUIREMENT Requirement = &Configuration->Requirements[Index];
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Raw =
            &Lists[0]->List[0].PartialResourceList.PartialDescriptors[Index];
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated =
            &Lists[1]->List[0].PartialResourceList.PartialDescriptors[Index];

        if (Requirement->Arbiter == NULL)
        {
            *Raw = Requirement->Device.Assignment;
            *Translated = Requirement->Device.Assignment;
            continue;
        }

        /* The arbiter did not assign anything, so there is nothing to translate */
        if (Requirement->Top->Entry.Result == ArbiterResultNullRequest)
        {
            *Raw = Requirement->Top->Assignment;
            *Translated = Requirement->Top->Assignment;
            continue;
        }

        Status = IopTranslateAssignmentToDevice(Requirement);
        if (NT_SUCCESS(Status))
        {
            *Raw = Requirement->Device.Assignment;

            Status = IopTranslateResourceToRoot(DeviceNode,
                                                Requirement->InterfaceType,
                                                Requirement->BusNumber,
                                                Requirement->Device.Entry.RequestSource,
                                                Raw,
                                                Translated);
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Assigned resource %lu of %wZ was not translated (Status 0x%08lx)\n",
                    Index, &DeviceNode->InstancePath, Status);
            Status = IopTranslationFailureStatus(Status);
            goto Failure;
        }
    }

    if (ResourceList != NULL)
        *ResourceList = Lists[0];
    else
        ExFreePoolWithTag(Lists[0], TAG_IO_ARBITER);

    *TranslatedList = Lists[1];
    return STATUS_SUCCESS;

Failure:
    for (Index = 0; Index < RTL_NUMBER_OF(Lists); Index++)
    {
        if (Lists[Index] != NULL)
            ExFreePoolWithTag(Lists[Index], TAG_IO_ARBITER);
    }

    return Status;
}

/**
 * @brief
 * Reserves an assigned resource list in the arbiters of a device with the
 * BootAllocation action, so other devices are not given these resources.
 *
 * @param[out] TranslatedList
 * Receives the translated resources, or NULL when nothing was reserved
 * because the list has no arbitrated descriptor.
 *
 * @return
 * STATUS_SUCCESS, or the failure status of the reservation. The ranges some
 * arbiters took before a failure stay reserved.
 */
static
NTSTATUS
NTAPI
IopReserveResourceList(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _Out_ PCM_RESOURCE_LIST *TranslatedList)
{
    IOP_CONFIGURATION Configuration;
    LIST_ENTRY ActiveArbiters;
    PLIST_ENTRY ListEntry;
    ULONG ArbitratedCount;
    ULONG Index;
    NTSTATUS Status;

    PAGED_CODE();

    *TranslatedList = NULL;

    Status = IopBuildFixedConfiguration(DeviceNode,
                                        ResourceList,
                                        RequestSource,
                                        &Configuration,
                                        &ArbitratedCount);
    if (!NT_SUCCESS(Status))
        return Status;

    if (ArbitratedCount == 0)
        goto Cleanup;

    InitializeListHead(&ActiveArbiters);

    for (Index = 0; Index < Configuration.Count; Index++)
    {
        if (Configuration.Requirements[Index].Arbiter != NULL)
            IopQueueRequirement(&ActiveArbiters, &Configuration.Requirements[Index]);
    }

    for (ListEntry = ActiveArbiters.Flink;
         ListEntry != &ActiveArbiters;
         ListEntry = ListEntry->Flink)
    {
        PPI_RESOURCE_ARBITER_ENTRY Arbiter =
            CONTAINING_RECORD(ListEntry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
        NTSTATUS ArbiterStatus;

        ArbiterStatus = IopArbiterInvoke(Arbiter->ArbiterInterface,
                                         ArbiterActionBootAllocation,
                                         &Arbiter->ResourceList);
        if (!NT_SUCCESS(ArbiterStatus))
        {
            DPRINT1("Boot allocation of type %u failed for %wZ (Status 0x%08lx)\n",
                    Arbiter->ResourceType, &DeviceNode->InstancePath, ArbiterStatus);
            Status = ArbiterStatus;
        }
    }

    IopDequeueRequirements(&ActiveArbiters);

    if (!NT_SUCCESS(Status))
        goto Cleanup;

    IopDeviceNodeSetFlag(DeviceNode, DNF_BOOT_CONFIG_RESERVED);

    Status = IopBuildResourceLists(DeviceNode,
                                   &Configuration,
                                   ResourceList->List[0].InterfaceType,
                                   ResourceList->List[0].BusNumber,
                                   NULL,
                                   TranslatedList);

Cleanup:
    IopFreeConfiguration(&Configuration);
    return Status;
}

/**
 * @brief
 * Reserves the boot configuration of a device in its arbiters, and keeps its
 * translated boot configuration on the device node.
 *
 * @return
 * STATUS_SUCCESS, or the failure status. On failure the boot configuration of
 * the device is freed.
 */
static
NTSTATUS
NTAPI
IopReserveBootConfigNow(
    _In_ PDEVICE_NODE DeviceNode)
{
    PCM_RESOURCE_LIST Translated;
    NTSTATUS Status;

    PAGED_CODE();

    if (DeviceNode->BootResources == NULL)
        return STATUS_SUCCESS;

    Status = IopReserveResourceList(DeviceNode,
                                    DeviceNode->BootResources,
                                    ArbiterRequestPnpEnumerated,
                                    &Translated);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reserve the boot config of %wZ (Status 0x%08lx)\n",
                &DeviceNode->InstancePath, Status);

        ExFreePool(DeviceNode->BootResources);
        DeviceNode->BootResources = NULL;

        if (DeviceNode->BootResourcesTranslated != NULL)
        {
            ExFreePool(DeviceNode->BootResourcesTranslated);
            DeviceNode->BootResourcesTranslated = NULL;
        }

        return Status;
    }

    /* A list without arbitrated resources has no translated list */
    if (Translated != NULL)
    {
        if (DeviceNode->BootResourcesTranslated != NULL)
            ExFreePool(DeviceNode->BootResourcesTranslated);

        DeviceNode->BootResourcesTranslated = Translated;
    }

    return STATUS_SUCCESS;
}

/* LEGACY BOOT CONFIGURATIONS ***********************************************/

/* Returns the size of a full resource descriptor, with its device specific data */
static
ULONG
NTAPI
IopFullDescriptorSize(
    _In_ PCM_FULL_RESOURCE_DESCRIPTOR Full)
{
    return (ULONG)((PUCHAR)IopNextFullDescriptor(Full) - (PUCHAR)Full);
}

/**
 * @brief
 * Moves the full descriptors of one legacy bus out of a resource list.
 *
 * @param[in,out] ResourceList
 * The list to split. Receives the descriptors of the other buses, or NULL
 * when none are left.
 *
 * @return
 * The descriptors of the bus, or NULL when there are none or the allocation
 * failed. The caller frees the list.
 */
static
PCM_RESOURCE_LIST
NTAPI
IopTakeBusDescriptors(
    _Inout_ PCM_RESOURCE_LIST *ResourceList,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber)
{
    ULONG HeaderSize = FIELD_OFFSET(CM_RESOURCE_LIST, List);
    PCM_RESOURCE_LIST Source = *ResourceList;
    PCM_RESOURCE_LIST Lists[2];
    PCM_FULL_RESOURCE_DESCRIPTOR Full;
    PUCHAR Cursor[2];
    ULONG Sizes[2];
    ULONG Index;

    Sizes[0] = Sizes[1] = HeaderSize;

    Full = &Source->List[0];
    for (Index = 0; Index < Source->Count; Index++)
    {
        BOOLEAN IsMatch = (Full->InterfaceType == InterfaceType && Full->BusNumber == BusNumber);

        Sizes[IsMatch ? 0 : 1] += IopFullDescriptorSize(Full);
        Full = IopNextFullDescriptor(Full);
    }

    if (Sizes[0] == HeaderSize)
        return NULL;

    if (Sizes[1] == HeaderSize)
    {
        *ResourceList = NULL;
        return Source;
    }

    Lists[0] = ExAllocatePoolZero(PagedPool, Sizes[0], TAG_IO_ARBITER);
    Lists[1] = ExAllocatePoolZero(PagedPool, Sizes[1], TAG_IO_ARBITER);
    if (Lists[0] == NULL || Lists[1] == NULL)
    {
        DPRINT1("No memory to split the HAL resources of bus %d/%lu\n", InterfaceType, BusNumber);

        if (Lists[0] != NULL)
            ExFreePoolWithTag(Lists[0], TAG_IO_ARBITER);
        if (Lists[1] != NULL)
            ExFreePoolWithTag(Lists[1], TAG_IO_ARBITER);

        return NULL;
    }

    Cursor[0] = (PUCHAR)&Lists[0]->List[0];
    Cursor[1] = (PUCHAR)&Lists[1]->List[0];

    Full = &Source->List[0];
    for (Index = 0; Index < Source->Count; Index++)
    {
        BOOLEAN IsMatch = (Full->InterfaceType == InterfaceType && Full->BusNumber == BusNumber);
        ULONG Target = IsMatch ? 0 : 1;
        ULONG Size = IopFullDescriptorSize(Full);

        RtlCopyMemory(Cursor[Target], Full, Size);
        Cursor[Target] += Size;
        Lists[Target]->Count++;

        Full = IopNextFullDescriptor(Full);
    }

    ExFreePoolWithTag(Source, TAG_IO_ARBITER);
    *ResourceList = Lists[1];
    return Lists[0];
}

/**
 * @brief
 * Appends the full descriptors of one resource list to another.
 *
 * @return
 * The combined list, which replaces both lists, or NULL if the allocation
 * failed and both lists are kept.
 */
static
PCM_RESOURCE_LIST
NTAPI
IopAppendResourceList(
    _In_opt_ PCM_RESOURCE_LIST First,
    _In_opt_ PCM_RESOURCE_LIST Second)
{
    ULONG HeaderSize = FIELD_OFFSET(CM_RESOURCE_LIST, List);
    PCM_RESOURCE_LIST Combined;
    ULONG FirstSize;
    ULONG SecondSize;

    if (First == NULL)
        return Second;

    if (Second == NULL)
        return First;

    FirstSize = PnpDetermineResourceListSize(First);
    SecondSize = PnpDetermineResourceListSize(Second);

    Combined = ExAllocatePoolWithTag(PagedPool,
                                     FirstSize + SecondSize - HeaderSize,
                                     TAG_IO_ARBITER);
    if (Combined == NULL)
        return NULL;

    RtlCopyMemory(Combined, First, FirstSize);
    RtlCopyMemory((PUCHAR)Combined + FirstSize,
                  (PUCHAR)Second + HeaderSize,
                  SecondSize - HeaderSize);
    Combined->Count += Second->Count;

    ExFreePool(First);
    ExFreePool(Second);
    return Combined;
}

/**
 * @brief
 * Keeps a copy of the raw resources the HAL reports, to reserve them for
 * the HAL device node once the legacy buses they are on exist.
 */
VOID
NTAPI
IopSaveHalResources(
    _In_ PCM_RESOURCE_LIST RawResourceList,
    _In_ ULONG ResourceListSize)
{
    PCM_RESOURCE_LIST Copy;

    PAGED_CODE();

    Copy = ExAllocatePoolWithTag(PagedPool, ResourceListSize, TAG_IO_ARBITER);
    if (Copy == NULL)
        return;

    RtlCopyMemory(Copy, RawResourceList, ResourceListSize);

    if (IopHalPendingResources != NULL)
        ExFreePoolWithTag(IopHalPendingResources, TAG_IO_ARBITER);

    IopHalPendingResources = Copy;
}

/**
 * @brief
 * Makes the first started device of the root the owner of the resources the
 * HAL reports. Called right after the HAL has reported its device.
 */
CODE_SEG("INIT")
VOID
NTAPI
IopMarkHalDeviceNode(VOID)
{
    PDEVICE_NODE Node;

    PAGED_CODE();

    for (Node = IopRootDeviceNode->Child; Node != NULL; Node = Node->Sibling)
    {
        if ((Node->State == DeviceNodeStarted || Node->State == DeviceNodeStartPostWork) &&
            !(Node->Flags & DNF_LEGACY_DRIVER))
        {
            IopHalOwnerNode = Node;
            IopDeviceNodeSetFlag(Node, DNF_HAL_NODE);
            return;
        }
    }

    DPRINT1("No HAL device node was found\n");
}

/* Reserves the resources the HAL reported on one legacy bus */
static
VOID
NTAPI
IopReserveHalBusResources(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber)
{
    PCM_RESOURCE_LIST BusList;
    PCM_RESOURCE_LIST Translated;
    PCM_RESOURCE_LIST Combined;
    NTSTATUS Status;

    if (IopHalOwnerNode == NULL || IopHalPendingResources == NULL)
        return;

    BusList = IopTakeBusDescriptors(&IopHalPendingResources, InterfaceType, BusNumber);
    if (BusList == NULL)
        return;

    DPRINT("Reserving the HAL resources of bus %d/%lu\n", InterfaceType, BusNumber);

    IopDeviceNodeSetFlag(IopHalOwnerNode, DNF_HAS_BOOT_CONFIG);

    /* The ranges reserved before a failure stay reserved, so the list keeps describing them */
    Status = IopReserveResourceList(IopHalOwnerNode,
                                    BusList,
                                    ArbiterRequestHalReported,
                                    &Translated);
    if (!NT_SUCCESS(Status))
        DPRINT1("The HAL resources of bus %d/%lu were not all reserved (Status 0x%08lx)\n",
                InterfaceType, BusNumber, Status);
    else if (Translated != NULL)
    {
        Combined = IopAppendResourceList(IopHalOwnerNode->BootResourcesTranslated, Translated);
        if (Combined != NULL)
            IopHalOwnerNode->BootResourcesTranslated = Combined;
        else
            ExFreePoolWithTag(Translated, TAG_IO_ARBITER);
    }

    /* The boot configuration of the HAL grows with each bus */
    Combined = IopAppendResourceList(IopHalOwnerNode->BootResources, BusList);
    if (Combined != NULL)
        IopHalOwnerNode->BootResources = Combined;
    else
        ExFreePoolWithTag(BusList, TAG_IO_ARBITER);
}

/* Reserves the held back boot configurations of one legacy bus */
static
VOID
NTAPI
IopReserveBusBootConfigs(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber)
{
    PLIST_ENTRY ListEntry;

    IopReserveHalBusResources(InterfaceType, BusNumber);

    ListEntry = IopPendingBootConfigList.Flink;
    while (ListEntry != &IopPendingBootConfigList)
    {
        PIOP_PENDING_BOOT_CONFIG Pending =
            CONTAINING_RECORD(ListEntry, IOP_PENDING_BOOT_CONFIG, ListEntry);
        PDEVICE_NODE DeviceNode = IopGetDeviceNode(Pending->DeviceObject);

        ListEntry = ListEntry->Flink;

        if (DeviceNode != NULL &&
            DeviceNode->BootResources != NULL &&
            (DeviceNode->BootResources->List[0].InterfaceType != InterfaceType ||
             DeviceNode->BootResources->List[0].BusNumber != BusNumber))
        {
            continue;
        }

        if (DeviceNode != NULL &&
            DeviceNode->BootResources != NULL &&
            !(DeviceNode->Flags & DNF_BOOT_CONFIG_RESERVED))
        {
            DPRINT("Reserving the boot config of %wZ\n", &DeviceNode->InstancePath);
            IopReserveBootConfigNow(DeviceNode);
        }

        RemoveEntryList(&Pending->ListEntry);
        ObDereferenceObject(Pending->DeviceObject);
        ExFreePoolWithTag(Pending, TAG_IO_ARBITER);
    }
}

/**
 * @brief
 * Reserves the held back boot configurations on the legacy bus a started
 * device provides. An ISA bus also provides the EISA bus.
 */
VOID
NTAPI
IopReserveLegacyBusBootConfigs(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    if (DeviceNode->InterfaceType == InterfaceTypeUndefined)
        return;

    IopLockResourceAssignment();

    if (IopHoldRootBootConfigs)
    {
        if (DeviceNode->InterfaceType == Isa)
            IopReserveBusBootConfigs(Eisa, DeviceNode->BusNumber);

        IopReserveBusBootConfigs(DeviceNode->InterfaceType, DeviceNode->BusNumber);
    }

    IopUnlockResourceAssignment();
}

/**
 * @brief
 * Reserves the boot configuration of a newly enumerated device. For a made
 * up device the reservation waits for the legacy bus of its resources.
 *
 * @return
 * STATUS_SUCCESS, STATUS_INSUFFICIENT_RESOURCES if the device could not be
 * added to the pending list, or the failure status of the reservation.
 */
NTSTATUS
NTAPI
IopReserveBootConfig(
    _In_ PDEVICE_NODE DeviceNode)
{
    PIOP_PENDING_BOOT_CONFIG Pending;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (DeviceNode->BootResources == NULL ||
        PnpDetermineResourceListSize(DeviceNode->BootResources) == 0)
    {
        return STATUS_SUCCESS;
    }

    IopLockResourceAssignment();

    if (IopHoldRootBootConfigs && (DeviceNode->Flags & DNF_MADEUP))
    {
        Pending = ExAllocatePoolWithTag(PagedPool, sizeof(*Pending), TAG_IO_ARBITER);
        if (Pending != NULL)
        {
            ObReferenceObject(DeviceNode->PhysicalDeviceObject);
            Pending->DeviceObject = DeviceNode->PhysicalDeviceObject;
            InsertTailList(&IopPendingBootConfigList, &Pending->ListEntry);
        }
        else
        {
            ExFreePool(DeviceNode->BootResources);
            DeviceNode->BootResources = NULL;
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else if (!(DeviceNode->Flags & DNF_BOOT_CONFIG_RESERVED))
    {
        Status = IopReserveBootConfigNow(DeviceNode);
    }

    IopUnlockResourceAssignment();

    return Status;
}

/**
 * @brief
 * Reserves the boot configurations on internal bus 0 once the boot bus
 * extenders are loaded. From then on boot configurations are reserved when
 * the devices are enumerated, and the ones still held are never reserved.
 */
VOID
NTAPI
IopReserveDeferredBootConfigs(VOID)
{
    PAGED_CODE();

    IopLockResourceAssignment();

    if (!IopHoldRootBootConfigs)
    {
        IopUnlockResourceAssignment();
        return;
    }

    IopReserveBusBootConfigs(Internal, 0);

    IopHoldRootBootConfigs = FALSE;

    while (!IsListEmpty(&IopPendingBootConfigList))
    {
        PIOP_PENDING_BOOT_CONFIG Pending =
            CONTAINING_RECORD(RemoveHeadList(&IopPendingBootConfigList),
                              IOP_PENDING_BOOT_CONFIG,
                              ListEntry);

        ObDereferenceObject(Pending->DeviceObject);
        ExFreePoolWithTag(Pending, TAG_IO_ARBITER);
    }

    if (IopHalPendingResources != NULL)
    {
        DPRINT1("HAL resources on a bus that never started are not reserved\n");
        ExFreePoolWithTag(IopHalPendingResources, TAG_IO_ARBITER);
        IopHalPendingResources = NULL;
    }

    IopUnlockResourceAssignment();
}

/* RESOURCE ARBITRATION *****************************************************/

/* Returns the alternative list that follows List */
static
PIO_RESOURCE_LIST
NTAPI
IopNextAlternativeList(
    _In_ PIO_RESOURCE_LIST List)
{
    return (PIO_RESOURCE_LIST)&List->Descriptors[List->Count];
}

/**
 * @brief
 * Converts an assigned resource list to a requirements list with a single
 * configuration of fixed requirements, which has the given priority.
 *
 * @return
 * The requirements list, or NULL if the list has no descriptors or the
 * allocation failed. The caller frees the list.
 */
static
PIO_RESOURCE_REQUIREMENTS_LIST
NTAPI
IopCmListToIoRequirements(
    _In_ PCM_RESOURCE_LIST ResourceList,
    _In_ ULONG Priority)
{
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements;
    PCM_FULL_RESOURCE_DESCRIPTOR Full = &ResourceList->List[0];
    PIO_RESOURCE_DESCRIPTOR Io;
    ULONG Count = 0;
    ULONG ListIndex;

    for (ListIndex = 0; ListIndex < ResourceList->Count; ListIndex++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm = &Full->PartialResourceList.PartialDescriptors[0];
        ULONG Index;

        for (Index = 0; Index < Full->PartialResourceList.Count; Index++)
        {
            if (Cm->Type != CmResourceTypeDeviceSpecific)
                Count++;

            Cm = IopNextPartialDescriptor(Cm);
        }

        Full = (PCM_FULL_RESOURCE_DESCRIPTOR)Cm;
    }

    if (Count == 0)
        return NULL;

    /* A priority descriptor, and a legacy bus descriptor before each other bus */
    Count += ResourceList->Count;

    Requirements = ExAllocatePoolZero(PagedPool,
                                      FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST,
                                                   List[0].Descriptors) +
                                          Count * sizeof(*Io),
                                      TAG_IO_ARBITER);
    if (Requirements == NULL)
        return NULL;

    Requirements->InterfaceType = ResourceList->List[0].InterfaceType;
    Requirements->BusNumber = ResourceList->List[0].BusNumber;
    Requirements->AlternativeLists = 1;
    Requirements->List[0].Version = 1;
    Requirements->List[0].Revision = 1;

    Io = &Requirements->List[0].Descriptors[0];
    Io->Option = IO_RESOURCE_PREFERRED;
    Io->Type = CmResourceTypeConfigData;
    Io->ShareDisposition = CmResourceShareShared;
    Io->u.ConfigData.Priority = Priority;
    Io++;

    Full = &ResourceList->List[0];
    for (ListIndex = 0; ListIndex < ResourceList->Count; ListIndex++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm = &Full->PartialResourceList.PartialDescriptors[0];
        ULONG Index;

        if (ListIndex != 0)
        {
            Io->Option = IO_RESOURCE_PREFERRED;
            Io->Type = IOP_RESOURCE_TYPE_LEGACY_BUS;
            Io->ShareDisposition = CmResourceShareUndetermined;
            Io->u.DevicePrivate.Data[0] = IopResourceInterface(Full->InterfaceType);
            Io->u.DevicePrivate.Data[1] = Full->BusNumber;
            Io++;
        }

        for (Index = 0; Index < Full->PartialResourceList.Count; Index++)
        {
            if (IopCmToFixedRequirement(Cm, Io))
                Io++;

            Cm = IopNextPartialDescriptor(Cm);
        }

        Full = (PCM_FULL_RESOURCE_DESCRIPTOR)Cm;
    }

    Requirements->List[0].Count = (ULONG)(Io - Requirements->List[0].Descriptors);
    Requirements->ListSize = (ULONG)((PUCHAR)Io - (PUCHAR)Requirements);

    return Requirements;
}

/* Returns a copy of a requirements list, or NULL */
static
PIO_RESOURCE_REQUIREMENTS_LIST
NTAPI
IopCopyRequirementsList(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList)
{
    PIO_RESOURCE_REQUIREMENTS_LIST Copy;

    Copy = ExAllocatePoolWithTag(PagedPool, RequirementsList->ListSize, TAG_IO_ARBITER);
    if (Copy != NULL)
        RtlCopyMemory(Copy, RequirementsList, RequirementsList->ListSize);

    return Copy;
}

/* Checks if a boot configuration descriptor takes part in the matching */
static
BOOLEAN
NTAPI
IopIsMatchedCmType(
    _In_ UCHAR Type)
{
    return Type != CmResourceTypeNull &&
           Type != CmResourceTypeDeviceSpecific &&
           Type < CmResourceTypeMaximum;
}

/* Returns the share disposition used to compare a boot descriptor with a requirement */
static
UCHAR
NTAPI
IopComparableShare(
    _In_ UCHAR Share,
    _In_ UCHAR Other)
{
    if (Share == CmResourceShareUndetermined || Share > CmResourceShareShared)
        return Other;

    return Share;
}

/**
 * @brief
 * Makes a requirement the only choice of its group of alternatives, since a
 * descriptor of the boot configuration was matched to it.
 */
static
VOID
NTAPI
IopKeepMatchedAlternative(
    _Inout_ PIO_RESOURCE_LIST List,
    _Inout_ PIO_RESOURCE_DESCRIPTOR Matched,
    _Inout_ PULONG KeptCount)
{
    PIO_RESOURCE_DESCRIPTOR End = &List->Descriptors[List->Count];
    PIO_RESOURCE_DESCRIPTOR Other;

    /* The choices before it, back to the one that leads the group */
    if (Matched->Option & IO_RESOURCE_ALTERNATIVE)
    {
        for (Other = Matched - 1; Other >= List->Descriptors; Other--)
        {
            Other->Type = CmResourceTypeNull;
            (*KeptCount)--;

            if (!(Other->Option & IO_RESOURCE_ALTERNATIVE))
                break;
        }
    }

    Matched->Option = IO_RESOURCE_PREFERRED;

    /* The choices after it */
    for (Other = Matched + 1; Other < End && (Other->Option & IO_RESOURCE_ALTERNATIVE); Other++)
    {
        Other->Type = CmResourceTypeNull;
        (*KeptCount)--;
    }
}

/**
 * @brief
 * Looks for the requirement of a configuration that a boot configuration
 * descriptor fits, and fixes that requirement at the boot placement.
 *
 * @param[in,out] IsTaken
 * One entry per requirement descriptor of the list, set for the ones that a
 * boot configuration descriptor was already matched to.
 *
 * @param[in,out] IsExactMatch
 * Cleared when the requirement allows more than the boot placement.
 *
 * @return
 * TRUE if a requirement was found.
 */
static
BOOLEAN
NTAPI
IopMatchBootDescriptor(
    _Inout_ PIO_RESOURCE_LIST List,
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm,
    _Inout_updates_(List->Count) PBOOLEAN IsTaken,
    _Inout_ PULONG KeptCount,
    _Inout_ PBOOLEAN IsExactMatch)
{
    PIO_RESOURCE_DESCRIPTOR End = &List->Descriptors[List->Count];
    ULONGLONG BootStart = 0, BootEnd = 0, BootLength = 1;
    ULONG Pass;

    switch (Cm->Type)
    {
        case CmResourceTypePort:
        case CmResourceTypeMemory:
        case CmResourceTypeMemoryLarge:
            BootLength = RtlCmDecodeMemIoResource(Cm, &BootStart);
            BootEnd = BootStart + BootLength - 1;
            break;

        case CmResourceTypeInterrupt:
            BootStart = BootEnd = Cm->u.Interrupt.Vector;
            break;

        case CmResourceTypeDma:
            BootStart = BootEnd = Cm->u.Dma.Channel;
            break;

        case CmResourceTypeBusNumber:
            BootStart = Cm->u.BusNumber.Start;
            BootEnd = BootStart + Cm->u.BusNumber.Length - 1;
            BootLength = Cm->u.BusNumber.Length;
            break;
    }

    /* A requirement that starts at the boot placement is preferred over one that covers it */
    for (Pass = 0; Pass < 2; Pass++)
    {
        PIO_RESOURCE_DESCRIPTOR Io;

        if (Pass == 1)
            *IsExactMatch = FALSE;

        for (Io = List->Descriptors; Io < End; Io++)
        {
            ULONGLONG Minimum = 0, Maximum = 0, Length = 1, Alignment = 1;
            BOOLEAN IsFit;

            if (Io->Type != Cm->Type || IsTaken[Io - List->Descriptors])
                continue;

            if (IopComparableShare(Cm->ShareDisposition, Io->ShareDisposition) !=
                IopComparableShare(Io->ShareDisposition, Cm->ShareDisposition))
            {
                continue;
            }

            switch (Io->Type)
            {
                case CmResourceTypePort:
                case CmResourceTypeMemory:
                case CmResourceTypeMemoryLarge:
                    Length = RtlIoDecodeMemIoResource(Io, &Alignment, &Minimum, &Maximum);
                    break;

                case CmResourceTypeInterrupt:
                    Minimum = Io->u.Interrupt.MinimumVector;
                    Maximum = Io->u.Interrupt.MaximumVector;
                    break;

                case CmResourceTypeDma:
                    Minimum = Io->u.Dma.MinimumChannel;
                    Maximum = Io->u.Dma.MaximumChannel;
                    break;

                case CmResourceTypeBusNumber:
                    Minimum = Io->u.BusNumber.MinBusNumber;
                    Maximum = Io->u.BusNumber.MaxBusNumber;
                    Length = Io->u.BusNumber.Length;
                    break;
            }

            if (Pass == 0)
            {
                IsFit = (Minimum == BootStart && Maximum >= BootEnd && Length >= BootLength);
            }
            else
            {
                /* An alignment of 0 only fits a placement at 0 */
                IsFit = (Minimum <= BootStart && Maximum >= BootEnd && Length >= BootLength &&
                         (BootStart & (Alignment - 1)) == 0);
            }

            if (!IsFit)
                continue;

            if (Pass == 0 && Maximum != BootEnd)
                *IsExactMatch = FALSE;

            switch (Io->Type)
            {
                case CmResourceTypePort:
                case CmResourceTypeMemory:
                case CmResourceTypeMemoryLarge:
                    Io->u.Generic.MinimumAddress.QuadPart = BootStart;
                    Io->u.Generic.MaximumAddress.QuadPart = BootStart + Length - 1;
                    if (Pass == 0)
                        Io->u.Generic.Alignment = 1;
                    break;

                case CmResourceTypeInterrupt:
                case CmResourceTypeDma:
                    /* A requirement that starts at the boot placement keeps its range */
                    if (Pass == 1)
                    {
                        Io->u.Interrupt.MinimumVector = (ULONG)BootStart;
                        Io->u.Interrupt.MaximumVector = (ULONG)BootEnd;
                    }
                    break;

                case CmResourceTypeBusNumber:
                    Io->u.BusNumber.MinBusNumber = (ULONG)BootStart;
                    Io->u.BusNumber.MaxBusNumber = (ULONG)(BootStart + Length - 1);
                    break;
            }

            IsTaken[Io - List->Descriptors] = TRUE;
            Io->Flags = Cm->Flags;
            IopKeepMatchedAlternative(List, Io, KeptCount);
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief
 * Fixes the requirements of a device at the placement of an assigned resource
 * list. The configurations that do not cover every assigned descriptor are
 * dropped, and the others get the boot configuration priority.
 *
 * @param[in] RequirementsList
 * The requirements of the device, or NULL.
 *
 * @param[out] Filtered
 * Receives the new requirements list, or NULL when there is none.
 *
 * @param[out] IsExactMatch
 * Set to TRUE when a single configuration asks for exactly the assigned
 * resources.
 *
 * @return
 * STATUS_SUCCESS, or STATUS_INSUFFICIENT_RESOURCES.
 */
static
NTSTATUS
NTAPI
IopFilterRequirementsForConfig(
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList,
    _In_opt_ PCM_RESOURCE_LIST ResourceList,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Filtered,
    _Out_ PBOOLEAN IsExactMatch)
{
    PIO_RESOURCE_REQUIREMENTS_LIST Work;
    PIO_RESOURCE_REQUIREMENTS_LIST Result;
    PIO_RESOURCE_LIST List;
    PIO_RESOURCE_LIST Exact = NULL;
    PIO_RESOURCE_DESCRIPTOR Out;
    PBOOLEAN IsKept;
    PBOOLEAN IsTaken;
    ULONG LargestCount = 1;
    ULONG BootCount = 0;
    ULONG KeptLists = 0;
    ULONG KeptDescriptors = 0;
    ULONG Size;
    ULONG ListIndex;

    *Filtered = NULL;
    *IsExactMatch = FALSE;

    if (RequirementsList == NULL || RequirementsList->AlternativeLists == 0)
    {
        if (ResourceList != NULL && ResourceList->Count != 0)
            *Filtered = IopCmListToIoRequirements(ResourceList, LCPRI_BOOTCONFIG);

        return STATUS_SUCCESS;
    }

    Work = IopCopyRequirementsList(RequirementsList);
    if (Work == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* A list that claims more than it holds is left for the validation to reject */
    List = &Work->List[0];
    for (ListIndex = 0; ListIndex < Work->AlternativeLists; ListIndex++)
    {
        if ((PUCHAR)&List->Descriptors[0] > (PUCHAR)Work + Work->ListSize ||
            (PUCHAR)IopNextAlternativeList(List) > (PUCHAR)Work + Work->ListSize)
        {
            *Filtered = Work;
            return STATUS_SUCCESS;
        }

        List = IopNextAlternativeList(List);
    }

    if (ResourceList != NULL && ResourceList->Count != 0)
    {
        PCM_FULL_RESOURCE_DESCRIPTOR Full = &ResourceList->List[0];

        for (ListIndex = 0; ListIndex < ResourceList->Count; ListIndex++)
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm = &Full->PartialResourceList.PartialDescriptors[0];
            ULONG Index;

            for (Index = 0; Index < Full->PartialResourceList.Count; Index++)
            {
                if (IopIsMatchedCmType(Cm->Type))
                    BootCount++;

                Cm = IopNextPartialDescriptor(Cm);
            }

            Full = (PCM_FULL_RESOURCE_DESCRIPTOR)Cm;
        }
    }

    if (BootCount == 0)
    {
        *Filtered = Work;
        return STATUS_SUCCESS;
    }

    List = &Work->List[0];
    for (ListIndex = 0; ListIndex < Work->AlternativeLists; ListIndex++)
    {
        LargestCount = max(LargestCount, List->Count);
        List = IopNextAlternativeList(List);
    }

    IsKept = ExAllocatePoolZero(PagedPool,
                                Work->AlternativeLists * sizeof(*IsKept),
                                TAG_IO_ARBITER);
    IsTaken = ExAllocatePoolZero(PagedPool, LargestCount * sizeof(*IsTaken), TAG_IO_ARBITER);
    if (IsKept == NULL || IsTaken == NULL)
    {
        if (IsKept != NULL)
            ExFreePoolWithTag(IsKept, TAG_IO_ARBITER);
        if (IsTaken != NULL)
            ExFreePoolWithTag(IsTaken, TAG_IO_ARBITER);

        ExFreePoolWithTag(Work, TAG_IO_ARBITER);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    List = &Work->List[0];
    for (ListIndex = 0; ListIndex < Work->AlternativeLists; ListIndex++)
    {
        PIO_RESOURCE_LIST NextList = IopNextAlternativeList(List);
        PCM_FULL_RESOURCE_DESCRIPTOR Full = &ResourceList->List[0];
        ULONG KeptCount = List->Count;
        BOOLEAN IsListExact = TRUE;
        ULONG Matched = 0;
        ULONG Index;
        ULONG FullIndex;

        RtlZeroMemory(IsTaken, LargestCount * sizeof(*IsTaken));

        if (List->Count == 0)
        {
            List = NextList;
            continue;
        }

        for (FullIndex = 0; FullIndex < ResourceList->Count; FullIndex++)
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR Cm = &Full->PartialResourceList.PartialDescriptors[0];

            for (Index = 0; Index < Full->PartialResourceList.Count; Index++)
            {
                if (IopIsMatchedCmType(Cm->Type) &&
                    IopMatchBootDescriptor(List, Cm, IsTaken, &KeptCount, &IsListExact))
                {
                    Matched++;
                }

                Cm = IopNextPartialDescriptor(Cm);
            }

            Full = (PCM_FULL_RESOURCE_DESCRIPTOR)Cm;
        }

        /* A configuration that misses a descriptor of the boot configuration is dropped */
        IsKept[ListIndex] = (Matched == BootCount);

        if (IsKept[ListIndex] &&
            (KeptCount == BootCount ||
             (KeptCount == BootCount + 1 &&
              List->Descriptors[0].Type == CmResourceTypeConfigData)))
        {
            /* Only one configuration that asks for exactly the boot placement is kept */
            if (Exact != NULL)
            {
                IsKept[ListIndex] = FALSE;
            }
            else
            {
                Exact = List;
                if (IsListExact)
                    *IsExactMatch = TRUE;
            }
        }

        if (IsKept[ListIndex])
        {
            KeptLists++;
            KeptDescriptors += KeptCount;
        }

        List = NextList;
    }

    ExFreePoolWithTag(IsTaken, TAG_IO_ARBITER);

    if (KeptLists == 0)
    {
        ExFreePoolWithTag(IsKept, TAG_IO_ARBITER);
        ExFreePoolWithTag(Work, TAG_IO_ARBITER);
        *Filtered = IopCmListToIoRequirements(ResourceList, LCPRI_BOOTCONFIG);
        return STATUS_SUCCESS;
    }

    /* Each kept configuration may need a priority descriptor */
    Size = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List) +
           KeptLists * FIELD_OFFSET(IO_RESOURCE_LIST, Descriptors) +
           (KeptDescriptors + KeptLists) * sizeof(*Out);

    Result = ExAllocatePoolZero(PagedPool, Size, TAG_IO_ARBITER);
    if (Result == NULL)
    {
        ExFreePoolWithTag(IsKept, TAG_IO_ARBITER);
        ExFreePoolWithTag(Work, TAG_IO_ARBITER);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Result->InterfaceType = ResourceList->List[0].InterfaceType;
    Result->BusNumber = ResourceList->List[0].BusNumber;
    Result->SlotNumber = Work->SlotNumber;
    Result->AlternativeLists = KeptLists;

    if (KeptLists > 1)
        *IsExactMatch = FALSE;

    List = &Work->List[0];
    Out = (PIO_RESOURCE_DESCRIPTOR)&Result->List[0];
    for (ListIndex = 0; ListIndex < Work->AlternativeLists; ListIndex++)
    {
        PIO_RESOURCE_LIST NextList = IopNextAlternativeList(List);
        PIO_RESOURCE_LIST NewList = (PIO_RESOURCE_LIST)Out;
        PIO_RESOURCE_DESCRIPTOR Priority;
        ULONG Index;

        if (!IsKept[ListIndex])
        {
            List = NextList;
            continue;
        }

        /* A version of 0xFFFF is not valid */
        NewList->Version = (List->Version == 0xFFFF) ? 1 : List->Version;
        NewList->Revision = List->Revision;
        Out = Priority = &NewList->Descriptors[0];

        if (List->Descriptors[0].Type != CmResourceTypeConfigData)
        {
            Priority->Option = IO_RESOURCE_PREFERRED;
            Priority->Type = CmResourceTypeConfigData;
            Priority->ShareDisposition = CmResourceShareShared;
            Out++;
        }

        for (Index = 0; Index < List->Count; Index++)
        {
            if (List->Descriptors[Index].Type != CmResourceTypeNull)
                *Out++ = List->Descriptors[Index];
        }

        Priority->u.ConfigData.Priority = LCPRI_BOOTCONFIG;
        NewList->Count = (ULONG)(Out - NewList->Descriptors);

        List = NextList;
    }

    Result->ListSize = (ULONG)((PUCHAR)Out - (PUCHAR)Result);

    ExFreePoolWithTag(IsKept, TAG_IO_ARBITER);
    ExFreePoolWithTag(Work, TAG_IO_ARBITER);
    *Filtered = Result;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Appends the configurations of one requirements list to another.
 *
 * @return
 * A new requirements list, or NULL if both are empty or the allocation
 * failed. The caller frees the list.
 */
static
PIO_RESOURCE_REQUIREMENTS_LIST
NTAPI
IopMergeRequirementsLists(
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST First,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST Second)
{
    ULONG HeaderSize = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List);
    BOOLEAN HasFirst = (First != NULL && First->AlternativeLists != 0);
    BOOLEAN HasSecond = (Second != NULL && Second->AlternativeLists != 0);
    PIO_RESOURCE_REQUIREMENTS_LIST Merged;

    if (!HasFirst && !HasSecond)
        return NULL;

    if (!HasSecond)
        return IopCopyRequirementsList(First);

    if (!HasFirst)
        return IopCopyRequirementsList(Second);

    Merged = ExAllocatePoolWithTag(PagedPool,
                                   First->ListSize + Second->ListSize - HeaderSize,
                                   TAG_IO_ARBITER);
    if (Merged == NULL)
        return NULL;

    RtlCopyMemory(Merged, First, First->ListSize);
    RtlCopyMemory((PUCHAR)Merged + First->ListSize,
                  (PUCHAR)Second + HeaderSize,
                  Second->ListSize - HeaderSize);
    Merged->ListSize = First->ListSize + Second->ListSize - HeaderSize;
    Merged->AlternativeLists += Second->AlternativeLists;

    return Merged;
}

/**
 * @brief
 * Checks the descriptors of one configuration. A configuration may start with
 * its priority, private data always follows another descriptor, and an
 * alternative follows the arbitrated requirement it is a choice for.
 * Requirements that cannot have alternatives are marked preferred.
 *
 * @return
 * TRUE if the descriptors are valid.
 */
static
BOOLEAN
NTAPI
IopIsValidConfiguration(
    _Inout_ PIO_RESOURCE_LIST List)
{
    BOOLEAN CanHaveAlternative = FALSE;
    ULONG Start = (List->Descriptors[0].Type == CmResourceTypeConfigData) ? 1 : 0;
    ULONG Index;

    for (Index = Start; Index < List->Count; Index++)
    {
        PIO_RESOURCE_DESCRIPTOR Descriptor = &List->Descriptors[Index];

        if (Descriptor->Type == CmResourceTypeConfigData)
            return FALSE;

        if (Descriptor->Type == CmResourceTypeDevicePrivate)
        {
            if (Index == Start)
                return FALSE;

            CanHaveAlternative = FALSE;
        }
        else if (!IopIsArbitratedType(Descriptor->Type))
        {
            Descriptor->Option = IO_RESOURCE_PREFERRED;
            CanHaveAlternative = FALSE;
        }
        else if (!(Descriptor->Option & IO_RESOURCE_ALTERNATIVE))
        {
            CanHaveAlternative = TRUE;
        }
        else if (!CanHaveAlternative)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief
 * Checks the layout of a requirements list before configurations are built.
 *
 * @param[out] HasEmptyList
 * Set to TRUE when a configuration without descriptors is reached, which
 * means the device needs no resources. The lists after it are not checked.
 *
 * @return
 * STATUS_SUCCESS, or STATUS_INVALID_PARAMETER for a malformed list.
 */
static
NTSTATUS
NTAPI
IopValidateRequirementsList(
    _Inout_ PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList,
    _Out_ PBOOLEAN HasEmptyList)
{
    ULONG_PTR ListEnd = (ULONG_PTR)RequirementsList + RequirementsList->ListSize;
    PIO_RESOURCE_LIST List = &RequirementsList->List[0];
    ULONG ListIndex;

    *HasEmptyList = FALSE;

    for (ListIndex = 0; ListIndex < RequirementsList->AlternativeLists; ListIndex++)
    {
        ULONG_PTR Descriptors = (ULONG_PTR)&List->Descriptors[0];

        /* The header of the configuration and its descriptors must be inside the list */
        if (Descriptors > ListEnd)
            return STATUS_INVALID_PARAMETER;

        if (List->Count == 0)
        {
            *HasEmptyList = TRUE;
            return STATUS_SUCCESS;
        }

        if (List->Count > (ListEnd - Descriptors) / sizeof(List->Descriptors[0]) ||
            !IopIsValidConfiguration(List))
        {
            return STATUS_INVALID_PARAMETER;
        }

        List = IopNextAlternativeList(List);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Builds the requirements of one alternative configuration, and finds the
 * arbiter and translators of each arbitrated requirement.
 *
 * @param[in,out] InterfaceType
 * The legacy bus of the next requirement. A descriptor of type
 * IOP_RESOURCE_TYPE_LEGACY_BUS changes it, also for the next configurations.
 *
 * @param[in,out] NeedsResources
 * Set to TRUE if the configuration has a resource that must be assigned.
 *
 * @return
 * STATUS_SUCCESS, or STATUS_INSUFFICIENT_RESOURCES. The configuration cannot
 * be used when Configuration->Status is a failure.
 */
static
NTSTATUS
NTAPI
IopBuildConfiguration(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList,
    _In_ PIO_RESOURCE_LIST List,
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _Inout_ PINTERFACE_TYPE InterfaceType,
    _Inout_ PULONG BusNumber,
    _Out_ PIOP_CONFIGURATION Configuration,
    _Inout_ PBOOLEAN NeedsResources)
{
    INTERFACE_TYPE ListInterfaceType = IopResourceInterface(RequirementsList->InterfaceType);
    BOOLEAN IsAfterArbitrated = FALSE;
    ULONG Index = 0;

    RtlZeroMemory(Configuration, sizeof(*Configuration));
    Configuration->Priority = LCPRI_NORMAL;

    Configuration->Requirements = ExAllocatePoolZero(PagedPool,
                                                     List->Count *
                                                         sizeof(*Configuration->Requirements),
                                                     TAG_IO_ARBITER);
    if (Configuration->Requirements == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    if (List->Descriptors[0].Type == CmResourceTypeConfigData)
    {
        Configuration->Priority = List->Descriptors[0].u.ConfigData.Priority;
        Index++;
    }

    while (Index < List->Count)
    {
        PIO_RESOURCE_DESCRIPTOR Lead = &List->Descriptors[Index];
        PIOP_REQUIREMENT Requirement;
        ULONG AlternativeCount = 1;
        NTSTATUS Status;

        if (Lead->Type == IOP_RESOURCE_TYPE_LEGACY_BUS)
        {
            *InterfaceType = IopResourceInterface((INTERFACE_TYPE)Lead->u.DevicePrivate.Data[0]);
            *BusNumber = Lead->u.DevicePrivate.Data[1];
            Index++;
            continue;
        }

        if (IopIsArbitratedType(Lead->Type))
        {
            while (Index + AlternativeCount < List->Count &&
                   (List->Descriptors[Index + AlternativeCount].Option & IO_RESOURCE_ALTERNATIVE))
            {
                AlternativeCount++;
            }
        }

        Requirement = &Configuration->Requirements[Configuration->Count++];
        IopInitializeRequirement(Requirement,
                                 DeviceNode->PhysicalDeviceObject,
                                 RequestSource,
                                 *InterfaceType,
                                 *BusNumber,
                                 Lead,
                                 AlternativeCount);
        Requirement->Device.Entry.BusNumber = RequirementsList->BusNumber;
        Requirement->Device.Entry.SlotNumber = RequirementsList->SlotNumber;

        Index += AlternativeCount;

        if (!IopIsArbitratedType(Lead->Type))
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR Copy = &Requirement->Device.Assignment;

            Copy->Type = Lead->Type;
            Copy->ShareDisposition = Lead->ShareDisposition;
            Copy->Flags = Lead->Flags;

            /* The PCI driver needs its private data back to program bridge windows */
            RtlCopyMemory(Copy->u.DevicePrivate.Data,
                          Lead->u.DevicePrivate.Data,
                          sizeof(Copy->u.DevicePrivate.Data));

            /* Private data of an arbitrated resource belongs to the device alone */
            if (Lead->Type == CmResourceTypeDevicePrivate && IsAfterArbitrated)
                Copy->ShareDisposition = CmResourceShareDeviceExclusive;
            else
                IsAfterArbitrated = FALSE;

            if (Lead->Type == CmResourceTypeConnection)
                *NeedsResources = TRUE;

            continue;
        }

        IsAfterArbitrated = TRUE;
        *NeedsResources = TRUE;

        Status = IopFindRequirementHandlers(Requirement, ListInterfaceType);
        if (Status == STATUS_INSUFFICIENT_RESOURCES)
            return Status;

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Configuration of %wZ cannot be arbitrated (Status 0x%08lx)\n",
                    &DeviceNode->InstancePath, Status);
            Configuration->Status = Status;
            break;
        }
    }

    return STATUS_SUCCESS;
}

#ifndef NDEBUG
/**
 * @brief
 * Prints the committed ranges of a root arbiter that overlap a range.
 *
 * @remarks
 * Only valid for the root arbiters. The context of an arbiter from a bus
 * driver is private to that driver.
 */
static
VOID
NTAPI
IopArbiterReportOccupants(
    _In_ PARBITER_INTERFACE Interface,
    _In_ ULONGLONG Start,
    _In_ ULONGLONG End)
{
    PARBITER_INSTANCE Arbiter;
    RTL_RANGE_LIST_ITERATOR Iterator;
    PRTL_RANGE Range;
    BOOLEAN DidFindRange = FALSE;

    PAGED_CODE();

    if (Interface == NULL || Interface->Context == NULL)
        return;

    Arbiter = (PARBITER_INSTANCE)Interface->Context;
    if (Arbiter->Allocation == NULL)
        return;

    for (RtlGetFirstRange(Arbiter->Allocation, &Iterator, &Range);
         Range != NULL;
         RtlGetNextRange(&Iterator, &Range, TRUE))
    {
        if (Range->Start > End || Range->End < Start)
            continue;

        DidFindRange = TRUE;
        DPRINT("      used by %I64x..%I64x owner %p attr 0x%x flags 0x%x%s\n",
               Range->Start, Range->End, Range->Owner,
               Range->Attributes, Range->Flags,
               (Range->Attributes & ARBITER_RANGE_BOOT_ALLOCATED) ? " BOOT_ALLOCATED" : "");
    }

    if (!DidFindRange)
        DPRINT("      no committed range overlaps\n");
}

/* Prints the requirements an arbiter could not satisfy */
static
VOID
NTAPI
IopArbiterReportFailure(
    _In_ PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry,
    _In_ BOOLEAN UseBootRanges,
    _In_ PDEVICE_NODE DeviceNode)
{
    PARBITER_INTERFACE Interface = ArbiterEntry->ArbiterInterface;
    PLIST_ENTRY Link;
    BOOLEAN IsRootArbiter;

    PAGED_CODE();

    IsRootArbiter = (Interface == IopGetRootArbiterInterface(ArbiterEntry->ResourceType));

    DPRINT("Arbitration failed: type %u, boot ranges %s, device %wZ%s\n",
           ArbiterEntry->ResourceType, UseBootRanges ? "allowed" : "excluded",
           &DeviceNode->InstancePath, IsRootArbiter ? "" : " (bus arbiter)");

    for (Link = ArbiterEntry->ResourceList.Flink;
         Link != &ArbiterEntry->ResourceList;
         Link = Link->Flink)
    {
        PARBITER_LIST_ENTRY Entry = CONTAINING_RECORD(Link, ARBITER_LIST_ENTRY, ListEntry);
        ULONG Alternative;

        DPRINT("  entry: %lu alternative(s), flags 0x%lx%s, source %u, result %u\n",
               Entry->AlternativeCount, Entry->Flags,
               (Entry->Flags & ARBITER_FLAG_BOOT_CONFIG) ? " (BOOT_CONFIG)" : "",
               Entry->RequestSource, Entry->Result);

        for (Alternative = 0; Alternative < Entry->AlternativeCount; Alternative++)
        {
            PIO_RESOURCE_DESCRIPTOR Descriptor = &Entry->Alternatives[Alternative];

            switch (Descriptor->Type)
            {
                case CmResourceTypePort:
                case CmResourceTypeMemory:
                case CmResourceTypeMemoryLarge:
                {
                    ULONGLONG Alignment, Min, Max;
                    ULONGLONG Length = RtlIoDecodeMemIoResource(Descriptor, &Alignment, &Min, &Max);

                    DPRINT("    [%lu] type %u opt 0x%x flags 0x%x len 0x%I64x align 0x%I64x "
                           "%I64x..%I64x%s\n",
                           Alternative, Descriptor->Type, Descriptor->Option, Descriptor->Flags,
                           Length, Alignment, Min, Max,
                           (Max - Min + 1 == Length) ? " FIXED" : "");

                    if (IsRootArbiter)
                        IopArbiterReportOccupants(Interface, Min, Max);
                    break;
                }

                case CmResourceTypeInterrupt:
                    DPRINT("    [%lu] irq opt 0x%x flags 0x%x %lx..%lx\n",
                           Alternative, Descriptor->Option, Descriptor->Flags,
                           Descriptor->u.Interrupt.MinimumVector,
                           Descriptor->u.Interrupt.MaximumVector);
                    break;

                case CmResourceTypeBusNumber:
                    DPRINT("    [%lu] bus opt 0x%x len 0x%lx %lx..%lx\n",
                           Alternative, Descriptor->Option, Descriptor->u.BusNumber.Length,
                           Descriptor->u.BusNumber.MinBusNumber,
                           Descriptor->u.BusNumber.MaxBusNumber);
                    break;

                default:
                    DPRINT("    [%lu] type %u opt 0x%x\n",
                           Alternative, Descriptor->Type, Descriptor->Option);
                    break;
            }
        }
    }
}
#endif

/*
 * One device of a batch assignment. Its lists go to the device node once the
 * whole batch was arbitrated.
 */
typedef struct _IOP_DEVICE_ASSIGNMENT
{
    PDEVICE_NODE DeviceNode;
    ARBITER_REQUEST_SOURCE RequestSource;
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements;
    PIOP_CONFIGURATION Configurations;
    ULONG ConfigurationCount;
    ULONG UsableCount;
    ULONG Selected;
    BOOLEAN AreRequirementsOwned;
    BOOLEAN IsSkipped;
    BOOLEAN IsCommitted;
    NTSTATUS Status;
    PCM_RESOURCE_LIST ResourceList;
    PCM_RESOURCE_LIST TranslatedList;
} IOP_DEVICE_ASSIGNMENT, *PIOP_DEVICE_ASSIGNMENT;

/* Frees the configurations of a request */
static
VOID
NTAPI
IopFreeRequestConfigurations(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    ULONG Index;

    if (Request->Configurations == NULL)
        return;

    for (Index = 0; Index < Request->ConfigurationCount; Index++)
        IopFreeConfiguration(&Request->Configurations[Index]);

    ExFreePoolWithTag(Request->Configurations, TAG_IO_ARBITER);
    Request->Configurations = NULL;
    Request->ConfigurationCount = 0;
    Request->UsableCount = 0;
}

/* Frees what a request holds, except the lists it assigned */
static
VOID
NTAPI
IopFreeRequest(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    IopFreeRequestConfigurations(Request);

    if (Request->AreRequirementsOwned && Request->Requirements != NULL)
        ExFreePoolWithTag(Request->Requirements, TAG_IO_ARBITER);

    Request->Requirements = NULL;
    Request->AreRequirementsOwned = FALSE;
}

/**
 * @brief
 * Builds the configurations of a request and orders them by priority. Only
 * the configurations up to LCPRI_LASTSOFTCONFIG are tried.
 *
 * @remarks
 * A request without requirements, or whose configurations need no resources,
 * is ignored and succeeds without resources.
 */
static
VOID
NTAPI
IopPrepareRequest(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList = Request->Requirements;
    PIOP_CONFIGURATION Configurations;
    INTERFACE_TYPE InterfaceType;
    PIO_RESOURCE_LIST List;
    NTSTATUS FailureStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN NeedsResources = FALSE;
    BOOLEAN HasEmptyList;
    ULONG BusNumber;
    ULONG Count = 0;
    ULONG Index;
    NTSTATUS Status;

    Request->IsSkipped = TRUE;
    Request->Status = STATUS_SUCCESS;

    if (RequirementsList == NULL || RequirementsList->AlternativeLists == 0)
        return;

    Status = IopValidateRequirementsList(RequirementsList, &HasEmptyList);
    if (!NT_SUCCESS(Status) || HasEmptyList)
    {
        Request->Status = Status;
        return;
    }

    Configurations = ExAllocatePoolZero(PagedPool,
                                        RequirementsList->AlternativeLists *
                                            sizeof(*Configurations),
                                        TAG_IO_ARBITER);
    if (Configurations == NULL)
    {
        Request->Status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

    InterfaceType = IopResourceInterface(RequirementsList->InterfaceType);
    BusNumber = RequirementsList->BusNumber;

    List = &RequirementsList->List[0];
    for (Index = 0; Index < RequirementsList->AlternativeLists; Index++)
    {
        PIOP_CONFIGURATION Configuration = &Configurations[Count];

        Status = IopBuildConfiguration(Request->DeviceNode,
                                       RequirementsList,
                                       List,
                                       Request->RequestSource,
                                       &InterfaceType,
                                       &BusNumber,
                                       Configuration,
                                       &NeedsResources);
        if (!NT_SUCCESS(Status))
        {
            IopFreeConfiguration(Configuration);
            Request->Configurations = Configurations;
            Request->ConfigurationCount = Count;
            IopFreeRequestConfigurations(Request);
            Request->Status = Status;
            return;
        }

        /* A configuration whose handlers were not found is dropped */
        if (NT_SUCCESS(Configuration->Status))
        {
            Count++;
        }
        else
        {
            FailureStatus = Configuration->Status;
            IopFreeConfiguration(Configuration);
        }

        List = IopNextAlternativeList(List);
    }

    Request->Configurations = Configurations;
    Request->ConfigurationCount = Count;

    if (Count == 0)
    {
        IopFreeRequestConfigurations(Request);
        Request->Status = FailureStatus;
        return;
    }

    if (!NeedsResources)
    {
        IopFreeRequestConfigurations(Request);
        return;
    }

    /* Lower priorities are better, and equal ones keep the order of the list */
    for (Index = 1; Index < Count; Index++)
    {
        IOP_CONFIGURATION Moved = Configurations[Index];
        ULONG Slot = Index;

        while (Slot > 0 && Configurations[Slot - 1].Priority > Moved.Priority)
        {
            Configurations[Slot] = Configurations[Slot - 1];
            Slot--;
        }

        Configurations[Slot] = Moved;
    }

    while (Request->UsableCount < Count &&
           Configurations[Request->UsableCount].Priority <= LCPRI_LASTSOFTCONFIG)
    {
        Request->UsableCount++;
    }

    if (Request->UsableCount == 0)
    {
        DPRINT1("%wZ has no configuration that can be assigned\n",
                &Request->DeviceNode->InstancePath);
        IopFreeRequestConfigurations(Request);
        Request->Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        return;
    }

    Request->IsSkipped = FALSE;
}

/**
 * @brief
 * Returns how flexible a request is. Requests with one or two configurations
 * are equally urgent, the others are as flexible as their configuration count.
 */
static
ULONG
NTAPI
IopRequestFlexibility(
    _In_ PIOP_DEVICE_ASSIGNMENT Request)
{
    if (Request->ConfigurationCount <= 2)
        return 0;

    return Request->ConfigurationCount;
}

/**
 * @brief
 * Tests the requirements of a configuration with their arbiters. The arbiters
 * closer to the root test their requirements first.
 *
 * @param[out] ActiveArbiters
 * Receives the arbiters of the configuration when the test succeeded, for
 * IopKeepConfiguration.
 *
 * @return
 * STATUS_SUCCESS, or the failure status of the arbiter.
 */
static
NTSTATUS
NTAPI
IopTryConfiguration(
    _In_ PDEVICE_NODE DeviceNode,
    _Inout_ PIOP_CONFIGURATION Configuration,
    _In_ BOOLEAN UseBootRanges,
    _Out_ PLIST_ENTRY ActiveArbiters)
{
    PLIST_ENTRY ListEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;

    UNREFERENCED_PARAMETER(DeviceNode);
    UNREFERENCED_PARAMETER(UseBootRanges);

    InitializeListHead(ActiveArbiters);

    for (Index = 0; Index < Configuration->Count; Index++)
    {
        PIOP_REQUIREMENT Requirement = &Configuration->Requirements[Index];

        if (Requirement->Arbiter == NULL)
            continue;

        Requirement->Top->Entry.Result = ArbiterResultUndefined;
        Requirement->Top->Assignment.Type = CmResourceTypeMaximum;
        IopQueueRequirement(ActiveArbiters, Requirement);
    }

    for (ListEntry = ActiveArbiters->Flink;
         ListEntry != ActiveArbiters;
         ListEntry = ListEntry->Flink)
    {
        PPI_RESOURCE_ARBITER_ENTRY Arbiter =
            CONTAINING_RECORD(ListEntry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
        PLIST_ENTRY Tested;

        Status = IopArbiterInvoke(Arbiter->ArbiterInterface,
                                  ArbiterActionTestAllocation,
                                  &Arbiter->ResourceList);
        if (NT_SUCCESS(Status))
            continue;

#ifndef NDEBUG
        IopArbiterReportFailure(Arbiter, UseBootRanges, DeviceNode);
#endif

        for (Tested = ActiveArbiters->Flink; Tested != ListEntry; Tested = Tested->Flink)
        {
            PPI_RESOURCE_ARBITER_ENTRY Previous =
                CONTAINING_RECORD(Tested, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);

            IopArbiterInvoke(Previous->ArbiterInterface, ArbiterActionRollbackAllocation, NULL);
        }

        IopDequeueRequirements(ActiveArbiters);
        break;
    }

    return Status;
}

/**
 * @brief
 * Commits a configuration that the arbiters tested.
 *
 * @return
 * STATUS_SUCCESS, or the failure status of the last arbiter that failed.
 */
static
NTSTATUS
NTAPI
IopKeepConfiguration(
    _In_ PDEVICE_NODE DeviceNode,
    _Inout_ PLIST_ENTRY ActiveArbiters)
{
    PLIST_ENTRY ListEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    for (ListEntry = ActiveArbiters->Flink;
         ListEntry != ActiveArbiters;
         ListEntry = ListEntry->Flink)
    {
        PPI_RESOURCE_ARBITER_ENTRY Arbiter =
            CONTAINING_RECORD(ListEntry, PI_RESOURCE_ARBITER_ENTRY, ActiveArbiterList);
        NTSTATUS ArbiterStatus;

        ArbiterStatus = IopArbiterInvoke(Arbiter->ArbiterInterface,
                                         ArbiterActionCommitAllocation,
                                         NULL);
        if (!NT_SUCCESS(ArbiterStatus))
        {
            DPRINT1("Arbiter of type %u failed to commit for %wZ\n",
                    Arbiter->ResourceType, &DeviceNode->InstancePath);
            Status = ArbiterStatus;
        }
    }

    IopDequeueRequirements(ActiveArbiters);

    return Status;
}

/**
 * @brief
 * Finds the first configuration of a request that its arbiters accept.
 *
 * @param[out] Selected
 * Receives the index of the configuration.
 *
 * @return
 * STATUS_SUCCESS with the configuration tested but not committed,
 * STATUS_BAD_MCFG_TABLE, STATUS_INSUFFICIENT_RESOURCES, or STATUS_UNSUCCESSFUL.
 *
 * @remarks
 * When the best configuration has the boot configuration priority, a second
 * pass lets the arbiters give its requirements ranges reserved for boot
 * configurations.
 */
static
NTSTATUS
NTAPI
IopFindConfiguration(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request,
    _Out_ PULONG Selected,
    _Out_ PLIST_ENTRY ActiveArbiters)
{
    PIOP_CONFIGURATION Best = &Request->Configurations[0];
    BOOLEAN UseBootRanges = FALSE;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG Index;

    *Selected = MAXULONG;

    for (;;)
    {
        for (Index = 0; Index < Request->UsableCount; Index++)
        {
            Status = IopTryConfiguration(Request->DeviceNode,
                                          &Request->Configurations[Index],
                                          UseBootRanges,
                                          ActiveArbiters);
            if (NT_SUCCESS(Status))
            {
                *Selected = Index;
                return STATUS_SUCCESS;
            }

            if (Status == STATUS_INSUFFICIENT_RESOURCES)
                return Status;
        }

        /* Without the boot configuration priority a second pass tests the same again */
        if (UseBootRanges || Best->Priority != LCPRI_BOOTCONFIG)
            break;

        UseBootRanges = TRUE;

        for (Index = 0; Index < Best->Count; Index++)
        {
            if (Best->Requirements[Index].Arbiter != NULL)
                Best->Requirements[Index].Device.Entry.Flags |= ARBITER_FLAG_BOOT_CONFIG;
        }
    }

    DPRINT1("All %lu configurations failed for %wZ\n",
            Request->UsableCount, &Request->DeviceNode->InstancePath);

    return (Status == STATUS_BAD_MCFG_TABLE) ? Status : STATUS_UNSUCCESSFUL;
}

/**
 * @brief
 * Frees all ranges of a device in one arbiter. A test allocation with no
 * alternatives removes the ranges of the device, and the commit applies it.
 */
static
VOID
NTAPI
IopArbiterReleaseOwner(
    _In_ PARBITER_INTERFACE Interface,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    ARBITER_LIST_ENTRY Entry;
    LIST_ENTRY ArbitrationList;
    NTSTATUS Status;

    RtlZeroMemory(&Entry, sizeof(Entry));
    Entry.PhysicalDeviceObject = PhysicalDeviceObject;
    Entry.RequestSource = ArbiterRequestPnpEnumerated;
    Entry.Result = ArbiterResultUndefined;

    InitializeListHead(&ArbitrationList);
    InsertTailList(&ArbitrationList, &Entry.ListEntry);

    Status = IopArbiterInvoke(Interface, ArbiterActionTestAllocation, &ArbitrationList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Arbiter rejected the release request (Status 0x%08lx)\n", Status);
        IopArbiterInvoke(Interface, ArbiterActionRollbackAllocation, NULL);
        return;
    }

    IopArbiterInvoke(Interface, ArbiterActionCommitAllocation, NULL);
}

/**
 * @brief
 * Frees the assigned and boot reserved ranges of a device in every arbiter above
 * it. Without a bus arbiter below the root, the legacy bus arbiters are used too.
 */
static
VOID
NTAPI
IopArbiterReleaseResources(
    _In_ PDEVICE_NODE DeviceNode)
{
    PCM_RESOURCE_LIST ResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR Full = NULL;
    ULONG ListCount = 1;
    ULONG ListIndex;

    PAGED_CODE();

    if (DeviceNode->PhysicalDeviceObject == NULL)
        return;

    ResourceList = (DeviceNode->ResourceList != NULL) ? DeviceNode->ResourceList
                                                      : DeviceNode->BootResources;
    if (ResourceList != NULL && ResourceList->Count != 0)
    {
        ListCount = ResourceList->Count;
        Full = &ResourceList->List[0];
    }

    for (ListIndex = 0; ListIndex < ListCount; ListIndex++)
    {
        IOP_RESOURCE_PATH Path;
        PDEVICE_NODE Node;

        Path.InterfaceType = (Full != NULL) ? IopResourceInterface(Full->InterfaceType) : Isa;
        Path.BusNumber = (Full != NULL) ? Full->BusNumber : 0;
        Path.ListInterfaceType = Path.InterfaceType;
        Path.CanUseLegacyBus = TRUE;

        Node = (DeviceNode == IopRootDeviceNode) ? DeviceNode : IopGetResourceParent(DeviceNode);

        for (Node = IopResourcePathNode(&Path, Node);
             Node != NULL;
             Node = IopResourcePathNode(&Path, IopGetResourceParent(Node)))
        {
            PLIST_ENTRY ListEntry;

            for (ListEntry = Node->DeviceArbiterList.Flink;
                 ListEntry != &Node->DeviceArbiterList;
                 ListEntry = ListEntry->Flink)
            {
                PPI_RESOURCE_ARBITER_ENTRY Arbiter =
                    CONTAINING_RECORD(ListEntry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);

                if (Arbiter->ArbiterInterface == NULL)
                    continue;

                /* A path that has a bus arbiter does not go through the legacy bus */
                Path.CanUseLegacyBus = FALSE;
                IopArbiterReleaseOwner(Arbiter->ArbiterInterface, DeviceNode->PhysicalDeviceObject);
            }
        }

        if (Full != NULL)
            Full = IopNextFullDescriptor(Full);
    }

    IopDeviceNodeClearFlag(DeviceNode, DNF_BOOT_CONFIG_RESERVED);
}

/* Frees the ranges a device holds in the arbiters of one configuration */
static
VOID
NTAPI
IopReleaseConfiguration(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PIOP_CONFIGURATION Configuration)
{
    ULONG Index;

    for (Index = 0; Index < Configuration->Count; Index++)
    {
        PPI_RESOURCE_ARBITER_ENTRY Arbiter = Configuration->Requirements[Index].Arbiter;
        ULONG Earlier;

        if (Arbiter == NULL)
            continue;

        /* Each arbiter releases the device once */
        for (Earlier = 0; Earlier < Index; Earlier++)
        {
            if (Configuration->Requirements[Earlier].Arbiter == Arbiter)
                break;
        }

        if (Earlier == Index)
            IopArbiterReleaseOwner(Arbiter->ArbiterInterface, DeviceNode->PhysicalDeviceObject);
    }
}

/**
 * @brief
 * Gives back the ranges the arbiters committed for an assignment that failed
 * afterwards. The boot configuration they took with them is reserved again.
 */
static
VOID
NTAPI
IopReleaseFailedAssignment(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    PDEVICE_NODE DeviceNode = Request->DeviceNode;

    /* The arbiters of the configuration may be on another bus than the boot configuration */
    if (Request->IsCommitted)
        IopReleaseConfiguration(DeviceNode, &Request->Configurations[Request->Selected]);

    IopArbiterReleaseResources(DeviceNode);
    Request->IsCommitted = FALSE;

    if ((DeviceNode->Flags & DNF_HAS_BOOT_CONFIG) && DeviceNode->BootResources != NULL)
        IopReserveBootConfigNow(DeviceNode);
}

/* Builds the raw and translated resource lists of the configuration a request committed */
static
NTSTATUS
NTAPI
IopBuildRequestLists(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    return IopBuildResourceLists(Request->DeviceNode,
                                 &Request->Configurations[Request->Selected],
                                 Request->Requirements->InterfaceType,
                                 Request->Requirements->BusNumber,
                                 &Request->ResourceList,
                                 &Request->TranslatedList);
}

/**
 * @brief
 * Assigns and commits the resources of a request, and builds its raw and
 * translated resource lists. A request whose translator asked to be called
 * again keeps its configuration committed, with STATUS_RETRY.
 *
 * @param[in] CanFail
 * FALSE when another device of the batch already got resources, so a failure
 * waits for the next assignment instead of setting a problem.
 *
 * @return
 * TRUE if the arbiters accepted a configuration.
 */
static
BOOLEAN
NTAPI
IopAllocateRequest(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request,
    _In_ BOOLEAN CanFail)
{
    LIST_ENTRY ActiveArbiters;
    NTSTATUS Status;

    Status = IopFindConfiguration(Request, &Request->Selected, &ActiveArbiters);
    if (!NT_SUCCESS(Status))
    {
        /* The resources of other devices are never moved to make room */
        if (!CanFail)
            Request->Status = STATUS_RETRY;
        else if (Status == STATUS_INSUFFICIENT_RESOURCES || Status == STATUS_BAD_MCFG_TABLE)
            Request->Status = Status;
        else
            Request->Status = STATUS_CONFLICTING_ADDRESSES;

        return FALSE;
    }

    Status = IopKeepConfiguration(Request->DeviceNode, &ActiveArbiters);
    Request->IsCommitted = TRUE;

    if (!NT_SUCCESS(Status))
    {
        IopReleaseFailedAssignment(Request);
        Request->Status = STATUS_CONFLICTING_ADDRESSES;
        return TRUE;
    }

    Request->Status = IopBuildRequestLists(Request);
    if (!NT_SUCCESS(Request->Status) && Request->Status != STATUS_RETRY)
        IopReleaseFailedAssignment(Request);

    return TRUE;
}

/**
 * @brief
 * Picks the requests of a batch that are assigned now. The others get
 * STATUS_RETRY and wait for a later assignment.
 *
 * @remarks
 * Until the boot configurations are reserved, devices without requirements
 * go alone. Devices with a boot configuration go before the others.
 *
 * @return
 * FALSE if no request that needs resources is assigned now.
 */
static
BOOLEAN
NTAPI
IopDeferWaitingRequests(
    _Inout_updates_(Count) PIOP_DEVICE_ASSIGNMENT Requests,
    _In_ ULONG Count)
{
    BOOLEAN HasEmptyRequest = FALSE;
    BOOLEAN HasBootRequest = FALSE;
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
    {
        PIOP_DEVICE_ASSIGNMENT Request = &Requests[Index];

        if (IopHoldRootBootConfigs && NT_SUCCESS(Request->Status) && Request->Requirements == NULL)
            HasEmptyRequest = TRUE;

        if (!Request->IsSkipped && (Request->DeviceNode->Flags & DNF_HAS_BOOT_CONFIG))
            HasBootRequest = TRUE;
    }

    for (Index = 0; Index < Count; Index++)
    {
        PIOP_DEVICE_ASSIGNMENT Request = &Requests[Index];
        BOOLEAN IsWaiting;

        if (HasEmptyRequest)
            IsWaiting = (Request->Requirements != NULL);
        else
            IsWaiting = (!Request->IsSkipped && HasBootRequest &&
                         !(Request->DeviceNode->Flags & DNF_HAS_BOOT_CONFIG));

        if (IsWaiting)
        {
            IopFreeRequestConfigurations(Request);
            Request->IsSkipped = TRUE;
            Request->Status = STATUS_RETRY;
        }
    }

    return !HasEmptyRequest;
}

/**
 * @brief
 * Assigns the resources of a batch of requests. The least flexible devices
 * are assigned first, and devices with the same flexibility keep the order of
 * the batch.
 */
static
VOID
NTAPI
IopAllocateRequests(
    _Inout_updates_(Count) PIOP_DEVICE_ASSIGNMENT Requests,
    _In_ ULONG Count)
{
    PIOP_DEVICE_ASSIGNMENT *Order;
    BOOLEAN IsAnyAssigned = FALSE;
    ULONG OrderCount = 0;
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
    {
        if (NT_SUCCESS(Requests[Index].Status))
            IopPrepareRequest(&Requests[Index]);
        else
            Requests[Index].IsSkipped = TRUE;
    }

    if (!IopDeferWaitingRequests(Requests, Count))
        return;

    Order = ExAllocatePoolZero(PagedPool, Count * sizeof(*Order), TAG_IO_ARBITER);
    if (Order == NULL)
    {
        for (Index = 0; Index < Count; Index++)
        {
            if (!Requests[Index].IsSkipped)
                Requests[Index].Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        return;
    }

    for (Index = 0; Index < Count; Index++)
    {
        PIOP_DEVICE_ASSIGNMENT Request = &Requests[Index];
        ULONG Flexibility = IopRequestFlexibility(Request);
        ULONG Slot;

        if (Request->IsSkipped)
            continue;

        for (Slot = OrderCount; Slot > 0; Slot--)
        {
            if (IopRequestFlexibility(Order[Slot - 1]) <= Flexibility)
                break;

            Order[Slot] = Order[Slot - 1];
        }

        Order[Slot] = Request;
        OrderCount++;
    }

    for (Index = 0; Index < OrderCount; Index++)
    {
        if (IopAllocateRequest(Order[Index], !IsAnyAssigned))
            IsAnyAssigned = TRUE;
    }

    /* A translator may need the devices assigned after its own to be translated first */
    for (Index = 0; Index < OrderCount; Index++)
    {
        PIOP_DEVICE_ASSIGNMENT Request = Order[Index];

        if (!Request->IsCommitted || Request->Status != STATUS_RETRY)
            continue;

        Request->Status = IopBuildRequestLists(Request);
        if (!NT_SUCCESS(Request->Status))
            IopReleaseFailedAssignment(Request);
    }

    ExFreePoolWithTag(Order, TAG_IO_ARBITER);
}

/* RESOURCE ASSIGNMENT *****************************************************/

/**
 * @brief
 * Builds the RESOURCEMAP value name of a device from its PDO name, with room
 * left for the ".Translated" suffix. The caller frees the buffer.
 */
static
NTSTATUS
NTAPI
IopResourceMapValueName(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PUNICODE_STRING Name)
{
    static const UNICODE_STRING TranslatedSuffix = RTL_CONSTANT_STRING(L".Translated");
    NTSTATUS Status;
    ULONG Length = 0;

    RtlZeroMemory(Name, sizeof(*Name));

    /* Get the size of the PDO name */
    Status = IoGetDeviceProperty(DeviceNode->PhysicalDeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 0,
                                 NULL,
                                 &Length);
    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL)
        return NT_SUCCESS(Status) ? STATUS_UNSUCCESSFUL : Status;

    /* A device without a name has no values */
    if (Length < sizeof(UNICODE_NULL) || Length + TranslatedSuffix.Length > MAXUSHORT)
        return STATUS_UNSUCCESSFUL;

    Name->Buffer = ExAllocatePoolWithTag(PagedPool,
                                         Length + TranslatedSuffix.Length,
                                         TAG_IO_ARBITER);
    if (Name->Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Name->Length = 0;
    Name->MaximumLength = (USHORT)(Length + TranslatedSuffix.Length);

    Status = IoGetDeviceProperty(DeviceNode->PhysicalDeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 Length,
                                 Name->Buffer,
                                 &Length);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Name->Buffer, TAG_IO_ARBITER);
        Name->Buffer = NULL;
        return Status;
    }

    /* Remove the terminating NULL */
    Name->Length = (USHORT)(Length - sizeof(UNICODE_NULL));

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Writes a resource list to the value Name + Suffix, or deletes the value
 * when ResourceList is NULL. The length of Name is restored before returning.
 */
static
NTSTATUS
NTAPI
IopWriteResourceMapValue(
    _In_ HANDLE KeyHandle,
    _Inout_ PUNICODE_STRING Name,
    _In_ PCUNICODE_STRING Suffix,
    _In_opt_ PCM_RESOURCE_LIST ResourceList)
{
    USHORT BaseLength = Name->Length;
    NTSTATUS Status;

    RtlAppendUnicodeStringToString(Name, Suffix);

    if (ResourceList != NULL)
    {
        Status = ZwSetValueKey(KeyHandle,
                               Name,
                               0,
                               REG_RESOURCE_LIST,
                               ResourceList,
                               PnpDetermineResourceListSize(ResourceList));
    }
    else
    {
        Status = ZwDeleteValueKey(KeyHandle, Name);
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            Status = STATUS_SUCCESS;
    }

    Name->Length = BaseLength;

    return Status;
}

static
NTSTATUS
NTAPI
IopUpdateResourceMap(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCWSTR Level1Key,
    _In_ PCWSTR Level2Key)
{
    static const UNICODE_STRING RawSuffix = RTL_CONSTANT_STRING(L".Raw");
    static const UNICODE_STRING TranslatedSuffix = RTL_CONSTANT_STRING(L".Translated");
    NTSTATUS Status;
    ULONG Disposition;
    HANDLE PnpMgrLevel1, PnpMgrLevel2, ResourceMapKey;
    UNICODE_STRING KeyName;
    UNICODE_STRING NameU;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateKey(&ResourceMapKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&KeyName, Level1Key);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                               ResourceMapKey,
                               NULL);
    Status = ZwCreateKey(&PnpMgrLevel1,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    ZwClose(ResourceMapKey);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&KeyName, Level2Key);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                               PnpMgrLevel1,
                               NULL);
    Status = ZwCreateKey(&PnpMgrLevel2,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);
    ZwClose(PnpMgrLevel1);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IopResourceMapValueName(DeviceNode, &NameU);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(PnpMgrLevel2);
        return Status;
    }

    /* The values are deleted when the device has no resources */
    ASSERT(DeviceNode->ResourceList == NULL || DeviceNode->ResourceListTranslated != NULL);

    Status = IopWriteResourceMapValue(PnpMgrLevel2, &NameU, &RawSuffix,
                                      DeviceNode->ResourceList);
    if (NT_SUCCESS(Status))
    {
        Status = IopWriteResourceMapValue(PnpMgrLevel2, &NameU, &TranslatedSuffix,
                                          DeviceNode->ResourceListTranslated);
    }

    ZwClose(PnpMgrLevel2);
    ExFreePoolWithTag(NameU.Buffer, TAG_IO_ARBITER);

    return Status;
}

/* Writes the resources of a device to the PnP manager part of the RESOURCEMAP */
static
NTSTATUS
NTAPI
IopUpdateResourceMapForPnPDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    return IopUpdateResourceMap(DeviceNode, L"PnP Manager", L"PnpManager");
}

/* Sets the problem of a device whose resources were not assigned, unless it is retried later */
static
VOID
NTAPI
IopSetResourceProblem(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ NTSTATUS Status)
{
    ULONG Problem;

    switch (Status)
    {
        case STATUS_RETRY:
            return;

        case STATUS_DEVICE_CONFIGURATION_ERROR:
            Problem = CM_PROB_NO_SOFTCONFIG;
            break;

        case STATUS_PNP_BAD_MPS_TABLE:
        case STATUS_BAD_MCFG_TABLE:
            Problem = CM_PROB_BIOS_TABLE;
            break;

        case STATUS_PNP_TRANSLATION_FAILED:
            Problem = CM_PROB_TRANSLATION_FAILED;
            break;

        case STATUS_PNP_IRQ_TRANSLATION_FAILED:
            Problem = CM_PROB_IRQ_TRANSLATION_FAILED;
            break;

        case STATUS_RESOURCE_TYPE_NOT_FOUND:
            Problem = CM_PROB_UNKNOWN_RESOURCE;
            break;

        default:
            Problem = CM_PROB_NORMAL_CONFLICT;
            break;
    }

    PiSetDevNodeProblem(DeviceNode, Problem);
}

/* Opens the instance key of a device, or a subkey of it */
static
NTSTATUS
NTAPI
IopOpenInstanceSubkey(
    _In_ PDEVICE_NODE DeviceNode,
    _In_opt_ PCWSTR SubkeyName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE KeyHandle)
{
    UNICODE_STRING EnumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
    UNICODE_STRING Name;
    HANDLE EnumKey;
    HANDLE InstanceKey;
    NTSTATUS Status;

    *KeyHandle = NULL;

    Status = IopOpenRegistryKeyEx(&EnumKey, NULL, &EnumRoot, KEY_ENUMERATE_SUB_KEYS);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IopOpenRegistryKeyEx(&InstanceKey,
                                  EnumKey,
                                  &DeviceNode->InstancePath,
                                  (SubkeyName != NULL) ? KEY_CREATE_SUB_KEY : DesiredAccess);
    ZwClose(EnumKey);

    if (!NT_SUCCESS(Status))
        return Status;

    if (SubkeyName == NULL)
    {
        *KeyHandle = InstanceKey;
        return Status;
    }

    RtlInitUnicodeString(&Name, SubkeyName);
    Status = IopCreateRegistryKeyEx(KeyHandle,
                                    InstanceKey,
                                    &Name,
                                    DesiredAccess,
                                    REG_OPTION_VOLATILE,
                                    NULL);
    ZwClose(InstanceKey);

    return Status;
}

/* Writes a value to the Control key of a device, or deletes it when Data is NULL */
static
NTSTATUS
NTAPI
IopWriteControlValue(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCWSTR ValueName,
    _In_ ULONG Type,
    _In_reads_bytes_opt_(Size) PVOID Data,
    _In_ ULONG Size)
{
    UNICODE_STRING Name;
    HANDLE ControlKey;
    NTSTATUS Status;

    Status = IopOpenInstanceSubkey(DeviceNode, L"Control", KEY_SET_VALUE, &ControlKey);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&Name, ValueName);

    if (Data != NULL)
        Status = ZwSetValueKey(ControlKey, &Name, 0, Type, Data, Size);
    else
        Status = ZwDeleteValueKey(ControlKey, &Name);

    ZwClose(ControlKey);
    return Status;
}

/* Writes the assigned resources to AllocConfig, or deletes it without resources */
static
NTSTATUS
NTAPI
IopUpdateControlKeyWithResources(
    _In_ PDEVICE_NODE DeviceNode)
{
    return IopWriteControlValue(DeviceNode,
                                L"AllocConfig",
                                REG_RESOURCE_LIST,
                                DeviceNode->ResourceList,
                                PnpDetermineResourceListSize(DeviceNode->ResourceList));
}

/* Writes the boot configuration of a device to LogConf, or deletes it */
static
VOID
NTAPI
IopUpdateBootConfigValue(
    _In_ PDEVICE_NODE DeviceNode,
    _In_opt_ PCM_RESOURCE_LIST BootConfig)
{
    UNICODE_STRING LogConf = RTL_CONSTANT_STRING(L"LogConf");
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"BootConfig");
    HANDLE InstanceKey;
    HANDLE LogConfKey;
    NTSTATUS Status;

    Status = IopOpenInstanceSubkey(DeviceNode, NULL, KEY_CREATE_SUB_KEY, &InstanceKey);
    if (!NT_SUCCESS(Status))
        return;

    /* Without a boot configuration there is no reason to create the key */
    if (BootConfig != NULL)
        Status = IopCreateRegistryKeyEx(&LogConfKey, InstanceKey, &LogConf, KEY_SET_VALUE,
                                        REG_OPTION_NON_VOLATILE, NULL);
    else
        Status = IopOpenRegistryKeyEx(&LogConfKey, InstanceKey, &LogConf, KEY_SET_VALUE);

    ZwClose(InstanceKey);

    if (!NT_SUCCESS(Status))
        return;

    if (BootConfig != NULL)
    {
        ZwSetValueKey(LogConfKey,
                      &Name,
                      0,
                      REG_RESOURCE_LIST,
                      BootConfig,
                      PnpDetermineResourceListSize(BootConfig));
    }
    else
    {
        ZwDeleteValueKey(LogConfKey, &Name);
    }

    ZwClose(LogConfKey);
}

/**
 * @brief
 * Gives a requirements list to the driver stack of a device with
 * IRP_MN_FILTER_RESOURCE_REQUIREMENTS, and records the result in
 * FilteredConfigVector.
 *
 * @return
 * The filtered list, which replaces RequirementsList, or RequirementsList
 * when the stack failed the request. NULL means the device needs no resources.
 */
static
PIO_RESOURCE_REQUIREMENTS_LIST
NTAPI
IopFilterDeviceRequirements(
    _In_ PDEVICE_NODE DeviceNode,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList)
{
    PIO_RESOURCE_REQUIREMENTS_LIST Filtered;
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    DPRINT("Sending IRP_MN_FILTER_RESOURCE_REQUIREMENTS to device stack\n");

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.Parameters.FilterResourceRequirements.IoResourceRequirementList = RequirementsList;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_FILTER_RESOURCE_REQUIREMENTS,
                               &Stack);

    /* A failed filter is ignored, the device may still start without it */
    if (!NT_SUCCESS(Status))
        return RequirementsList;

    /* A driver that changes the list frees the one it was given */
    Filtered = (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;

    IopWriteControlValue(DeviceNode,
                         L"FilteredConfigVector",
                         REG_RESOURCE_REQUIREMENTS_LIST,
                         (Filtered != NULL) ? (PVOID)Filtered : (PVOID)L"",
                         (Filtered != NULL) ? Filtered->ListSize : 0);

    return Filtered;
}

/**
 * @brief
 * Prepares the requirements of a device for an assignment. The first time,
 * the boot configuration of a device that is not on a PCI bus is merged into
 * them, and they are given to the driver stack to filter.
 *
 * @remarks
 * A device without requirements asks for its boot configuration.
 */
static
NTSTATUS
NTAPI
IopGetDeviceRequirements(
    _In_ PDEVICE_NODE DeviceNode)
{
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements = DeviceNode->ResourceRequirements;
    PCM_RESOURCE_LIST BootConfig = DeviceNode->BootResources;
    PIO_RESOURCE_REQUIREMENTS_LIST Filtered;
    BOOLEAN IsExactMatch;
    NTSTATUS Status;

    if (Requirements != NULL && !(DeviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED))
        return STATUS_SUCCESS;

    if (BootConfig == NULL ||
        BootConfig->Count == 0 ||
        BootConfig->List[0].InterfaceType != PCIBus)
    {
        Status = IopFilterRequirementsForConfig(Requirements, BootConfig, &Filtered, &IsExactMatch);
        if (!NT_SUCCESS(Status))
            return Status;

        /* A made up device only gets the configurations that fit its boot configuration */
        if (!(DeviceNode->Flags & DNF_MADEUP) &&
            (!IsExactMatch || (Requirements != NULL && Requirements->AlternativeLists > 1)))
        {
            PIO_RESOURCE_REQUIREMENTS_LIST Merged = IopMergeRequirementsLists(Filtered,
                                                                             Requirements);
            BOOLEAN IsMergeNeeded = (Filtered != NULL && Filtered->AlternativeLists != 0) ||
                                    (Requirements != NULL && Requirements->AlternativeLists != 0);

            if (Filtered != NULL)
                ExFreePoolWithTag(Filtered, TAG_IO_ARBITER);

            /* Both lists empty merge to nothing, anything else needed memory */
            if (Merged == NULL && IsMergeNeeded)
                return STATUS_INSUFFICIENT_RESOURCES;

            Filtered = Merged;
        }

        if (Requirements != NULL)
            ExFreePool(Requirements);

        DeviceNode->ResourceRequirements = Requirements = Filtered;
    }

    DeviceNode->ResourceRequirements = IopFilterDeviceRequirements(DeviceNode, Requirements);
    IopDeviceNodeClearFlag(DeviceNode, DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED);

    return STATUS_SUCCESS;
}

/* Checks if a device was reported with IoReportDetectedDevice */
static
BOOLEAN
NTAPI
IopIsReportedDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    PKEY_VALUE_FULL_INFORMATION Value;
    BOOLEAN IsReported = FALSE;
    HANDLE InstanceKey;

    if (!(DeviceNode->Flags & DNF_MADEUP))
        return FALSE;

    if (!NT_SUCCESS(IopOpenInstanceSubkey(DeviceNode, NULL, KEY_QUERY_VALUE, &InstanceKey)))
        return FALSE;

    if (NT_SUCCESS(IopGetRegistryValue(InstanceKey, L"DeviceReported", &Value)))
    {
        IsReported = (Value->Type == REG_DWORD &&
                      Value->DataLength == sizeof(ULONG) &&
                      *(PULONG)((PUCHAR)Value + Value->DataOffset) != 0);
        ExFreePool(Value);
    }

    ZwClose(InstanceKey);
    return IsReported;
}

/* Frees the raw and translated resource lists assigned to a device */
static
VOID
NTAPI
IopFreeAssignedLists(
    _In_ PDEVICE_NODE DeviceNode)
{
    if (DeviceNode->ResourceListTranslated != NULL)
    {
        ExFreePool(DeviceNode->ResourceListTranslated);
        DeviceNode->ResourceListTranslated = NULL;
    }

    if (DeviceNode->ResourceList != NULL)
    {
        ExFreePool(DeviceNode->ResourceList);
        DeviceNode->ResourceList = NULL;
    }
}

/* Frees the previous assignment of a device, so it does not conflict with itself */
static
VOID
NTAPI
IopDiscardAssignment(
    _In_ PDEVICE_NODE DeviceNode)
{
    if (DeviceNode->ResourceList != NULL)
        IopArbiterReleaseResources(DeviceNode);

    IopFreeAssignedLists(DeviceNode);
}

/**
 * @brief
 * Gives the lists a request was assigned to its device node, which owns them
 * from then on, and records them in the registry.
 */
static
VOID
NTAPI
IopInstallAssignment(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    PDEVICE_NODE DeviceNode = Request->DeviceNode;
    BOOLEAN HadResources = (DeviceNode->ResourceList != NULL);

    IopFreeAssignedLists(DeviceNode);

    DeviceNode->ResourceList = Request->ResourceList;
    DeviceNode->ResourceListTranslated = Request->TranslatedList;
    Request->ResourceList = NULL;
    Request->TranslatedList = NULL;

    if (DeviceNode->ResourceList == NULL)
        IopDeviceNodeSetFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);

    /* The device works without its registry values, so failures are ignored */
    if (DeviceNode->ResourceList != NULL || HadResources)
    {
        IopUpdateResourceMapForPnPDevice(DeviceNode);
        IopUpdateControlKeyWithResources(DeviceNode);
    }
}

/* Stores the resources a request was assigned, or the problem of the device */
static
VOID
NTAPI
IopFinishResourceRequest(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    PDEVICE_NODE DeviceNode = Request->DeviceNode;

    if (!NT_SUCCESS(Request->Status))
    {
        DPRINT1("Failed to assign resources for %wZ (Status 0x%08lx)\n",
                &DeviceNode->InstancePath, Request->Status);
        IopSetResourceProblem(DeviceNode, Request->Status);
        return;
    }

    IopInstallAssignment(Request);
    PiSetDevNodeState(DeviceNode, DeviceNodeResourcesAssigned);
}

/**
 * @brief
 * Assigns resources to all devices of a subtree whose drivers are added.
 */
VOID
NTAPI
IopAssignResourcesToSubtree(
    _In_ PDEVICE_NODE SubtreeRoot)
{
    PIOP_DEVICE_ASSIGNMENT Requests = NULL;
    BOOLEAN IsAnyAssigned = FALSE;
    BOOLEAN IsAnyRetried = FALSE;
    PDEVICE_NODE Node;
    ULONG Capacity = 0;
    ULONG Count = 0;
    ULONG Index;
    KIRQL OldIrql;

    PAGED_CODE();

    /* The tree is walked again whenever the array is too small for its devices */
    for (;;)
    {
        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);

        Count = 0;
        Node = SubtreeRoot;
        while (Node != NULL)
        {
            if (Node->State == DeviceNodeDriversAdded && !(Node->Flags & DNF_HAS_PROBLEM))
            {
                if (Count < Capacity)
                {
                    ObReferenceObject(Node->PhysicalDeviceObject);
                    Requests[Count].DeviceNode = Node;
                }

                Count++;
            }

            if (Node->Child != NULL)
            {
                Node = Node->Child;
                continue;
            }

            while (Node != SubtreeRoot && Node->Sibling == NULL)
                Node = Node->Parent;

            Node = (Node == SubtreeRoot) ? NULL : Node->Sibling;
        }

        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        if (Count <= Capacity)
            break;

        if (Requests != NULL)
        {
            for (Index = 0; Index < Capacity; Index++)
                ObDereferenceObject(Requests[Index].DeviceNode->PhysicalDeviceObject);

            ExFreePoolWithTag(Requests, TAG_IO_ARBITER);
        }

        Capacity = Count;
        Requests = ExAllocatePoolZero(PagedPool, Capacity * sizeof(*Requests), TAG_IO_ARBITER);
        if (Requests == NULL)
            return;
    }

    if (Count == 0)
        goto Done;

    IopLockResourceAssignment();

    for (Index = 0; Index < Count; Index++)
    {
        PIOP_DEVICE_ASSIGNMENT Request = &Requests[Index];
        PDEVICE_NODE DeviceNode = Request->DeviceNode;

        IopDiscardAssignment(DeviceNode);

        /* Reported devices are treated as legacy devices by the arbiters */
        Request->RequestSource = IopIsReportedDevice(DeviceNode) ? ArbiterRequestLegacyReported
                                                                 : ArbiterRequestPnpEnumerated;

        Request->Status = IopGetDeviceRequirements(DeviceNode);
        Request->Requirements = DeviceNode->ResourceRequirements;
    }

    DPRINT("Arbitrating resources for %lu device(s)\n", Count);

    IopAllocateRequests(Requests, Count);

    for (Index = 0; Index < Count; Index++)
    {
        PIOP_DEVICE_ASSIGNMENT Request = &Requests[Index];

        if (Request->Status == STATUS_RETRY)
            IsAnyRetried = TRUE;

        if (NT_SUCCESS(Request->Status))
            IsAnyAssigned = TRUE;

        IopFinishResourceRequest(Request);
        IopFreeRequest(Request);

        if (Request->ResourceList != NULL)
            ExFreePoolWithTag(Request->ResourceList, TAG_IO_ARBITER);
        if (Request->TranslatedList != NULL)
            ExFreePoolWithTag(Request->TranslatedList, TAG_IO_ARBITER);
    }

    IopUnlockResourceAssignment();

    /* The devices that wait are tried again once the others are started */
    if (IsAnyRetried && IsAnyAssigned)
        IopIsAssignmentRetryNeeded = TRUE;

Done:
    for (Index = 0; Index < min(Count, Capacity); Index++)
        ObDereferenceObject(Requests[Index].DeviceNode->PhysicalDeviceObject);

    if (Requests != NULL)
        ExFreePoolWithTag(Requests, TAG_IO_ARBITER);
}

/**
 * @brief
 * Checks if the last assignment left devices for a later one, and forgets it.
 */
BOOLEAN
NTAPI
IopTakeAssignmentRetry(VOID)
{
    BOOLEAN IsNeeded = IopIsAssignmentRetryNeeded;

    IopIsAssignmentRetryNeeded = FALSE;
    return IsNeeded;
}

/**
 * @brief
 * Clears the resource problems of the devices that wait for resources, so the
 * next assignment tries them again after resources were freed.
 */
VOID
NTAPI
IopClearResourceConflictProblems(VOID)
{
    PDEVICE_NODE Node = IopRootDeviceNode;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);

    while (Node != NULL)
    {
        if (Node->State == DeviceNodeDriversAdded &&
            (Node->Flags & DNF_HAS_PROBLEM) &&
            (Node->Problem == CM_PROB_NORMAL_CONFLICT ||
             Node->Problem == CM_PROB_TRANSLATION_FAILED ||
             Node->Problem == CM_PROB_IRQ_TRANSLATION_FAILED))
            PiClearDevNodeProblem(Node);

        if (Node->Child != NULL)
        {
            Node = Node->Child;
            continue;
        }

        while (Node != IopRootDeviceNode && Node->Sibling == NULL)
            Node = Node->Parent;

        Node = (Node == IopRootDeviceNode) ? NULL : Node->Sibling;
    }

    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
}

/**
 * @brief
 * Frees the arbiter ranges and the assigned resources of a device, with the
 * resource assignment lock held. A made up device that is still present
 * reserves its boot configuration again, other devices lose it.
 */
static
VOID
NTAPI
IopReleaseDeviceNodeResources(
    _In_ PDEVICE_NODE DeviceNode)
{
    IopArbiterReleaseResources(DeviceNode);
    IopFreeAssignedLists(DeviceNode);

    if (DeviceNode->PhysicalDeviceObject != NULL && DeviceNode->InstancePath.Length != 0)
        IopUpdateControlKeyWithResources(DeviceNode);

    if ((DeviceNode->Flags & (DNF_MADEUP | DNF_DEVICE_GONE)) == DNF_MADEUP)
    {
        if ((DeviceNode->Flags & DNF_HAS_BOOT_CONFIG) && DeviceNode->BootResources != NULL)
            IopReserveBootConfigNow(DeviceNode);

        return;
    }

    IopDeviceNodeClearFlag(DeviceNode, DNF_HAS_BOOT_CONFIG | DNF_BOOT_CONFIG_RESERVED);

    if (DeviceNode->BootResources != NULL)
    {
        ExFreePool(DeviceNode->BootResources);
        DeviceNode->BootResources = NULL;
    }

    if (DeviceNode->BootResourcesTranslated != NULL)
    {
        ExFreePool(DeviceNode->BootResourcesTranslated);
        DeviceNode->BootResourcesTranslated = NULL;
    }
}

/**
 * @brief
 * Gives back the arbiter ranges of a device node that is freed, without
 * reserving anything again.
 */
VOID
NTAPI
IopDropDeviceNodeResources(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    if (!IopResourceAssignmentReady)
        return;

    if (DeviceNode->ResourceList == NULL && !(DeviceNode->Flags & DNF_BOOT_CONFIG_RESERVED))
        return;

    IopLockResourceAssignment();
    IopArbiterReleaseResources(DeviceNode);
    IopUnlockResourceAssignment();
}

/* Asks a device for the resources it decodes now, which become its boot configuration */
static
PCM_RESOURCE_LIST
NTAPI
IopQueryCurrentBootConfig(
    _In_ PDEVICE_NODE DeviceNode)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_RESOURCES,
                               NULL);
    if (!NT_SUCCESS(Status))
        return NULL;

    return (PCM_RESOURCE_LIST)IoStatusBlock.Information;
}

/**
 * @brief
 * Frees the resources of a device that is removed or restarted, and lets the
 * devices that could not be assigned resources try again.
 *
 * @param[in] ShouldReserveBootConfig
 * TRUE for a device its bus still enumerates. Its boot configuration is
 * queried again and reserved, since the hardware still decodes it.
 */
VOID
NTAPI
IopFreeDeviceResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN ShouldReserveBootConfig)
{
    PCM_RESOURCE_LIST BootConfig = NULL;
    BOOLEAN IsRequeried;

    PAGED_CODE();

    if (!IopResourceAssignmentReady)
        return;

    if (DeviceNode->ResourceList == NULL && !(DeviceNode->Flags & DNF_BOOT_CONFIG_RESERVED))
        return;

    /* A made up device keeps the boot configuration it was created with */
    IsRequeried = ShouldReserveBootConfig && !(DeviceNode->Flags & DNF_MADEUP);

    /* The device is asked while it still owns its resources */
    if (IsRequeried)
        BootConfig = IopQueryCurrentBootConfig(DeviceNode);

    IopLockResourceAssignment();

    IopReleaseDeviceNodeResources(DeviceNode);

    if (IsRequeried)
    {
        IopUpdateBootConfigValue(DeviceNode, BootConfig);

        if (BootConfig != NULL)
        {
            DeviceNode->BootResources = BootConfig;
            IopDeviceNodeSetFlag(DeviceNode, DNF_HAS_BOOT_CONFIG);
            IopReserveBootConfig(DeviceNode);
        }
    }

    IopUnlockResourceAssignment();

    if (IopRootDeviceNode != NULL)
    {
        PiQueueDeviceAction(IopRootDeviceNode->PhysicalDeviceObject,
                            PiActionAssignResources,
                            NULL,
                            NULL);
    }
}

/**
 * @brief
 * Builds the configurations a started device may move to. Only the ones that
 * keep its current resources are used, when there are any.
 *
 * @return
 * STATUS_SUCCESS, or the failure status. Request->IsSkipped is set when the
 * device needs no resources.
 */
static
NTSTATUS
NTAPI
IopPrepareReassignment(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    PDEVICE_NODE DeviceNode = Request->DeviceNode;
    PIO_RESOURCE_REQUIREMENTS_LIST Current = NULL;
    BOOLEAN IsExactMatch;
    NTSTATUS Status;

    Status = IopGetDeviceRequirements(DeviceNode);
    if (!NT_SUCCESS(Status))
        return Status;

    Request->Requirements = DeviceNode->ResourceRequirements;

    Status = IopFilterRequirementsForConfig(DeviceNode->ResourceRequirements,
                                            DeviceNode->ResourceList,
                                            &Current,
                                            &IsExactMatch);
    if (NT_SUCCESS(Status) && Current != NULL)
    {
        Request->Requirements = Current;
        Request->AreRequirementsOwned = (Current != DeviceNode->ResourceRequirements);
    }

    IopPrepareRequest(Request);
    return Request->Status;
}

/**
 * @brief
 * Moves a started device to the first configuration its arbiters accept. Its
 * current ranges are released first, so they can be part of the new one.
 *
 * @return
 * STATUS_SUCCESS with the lists of the request built, or the failure status.
 * On failure the device holds no range in the arbiters.
 */
static
NTSTATUS
NTAPI
IopCommitReassignment(
    _Inout_ PIOP_DEVICE_ASSIGNMENT Request)
{
    PDEVICE_NODE DeviceNode = Request->DeviceNode;
    LIST_ENTRY ActiveArbiters;
    NTSTATUS Status;

    if (DeviceNode->ResourceList != NULL)
        IopArbiterReleaseResources(DeviceNode);

    Status = IopFindConfiguration(Request, &Request->Selected, &ActiveArbiters);
    if (NT_SUCCESS(Status))
    {
        Status = IopKeepConfiguration(DeviceNode, &ActiveArbiters);
        Request->IsCommitted = TRUE;
    }

    if (NT_SUCCESS(Status))
        Status = IopBuildRequestLists(Request);

    if (!NT_SUCCESS(Status))
    {
        if (Request->IsCommitted)
            IopReleaseConfiguration(DeviceNode, &Request->Configurations[Request->Selected]);

        IopArbiterReleaseResources(DeviceNode);
        Request->IsCommitted = FALSE;
    }

    return Status;
}

/**
 * @brief
 * Gives a started device back the resources it had, after no new
 * configuration could be assigned.
 *
 * @return
 * STATUS_SUCCESS, or the failure status of the arbiters.
 */
static
NTSTATUS
NTAPI
IopRestoreAssignment(
    _In_ PDEVICE_NODE DeviceNode)
{
    IOP_DEVICE_ASSIGNMENT Restore;
    LIST_ENTRY ActiveArbiters;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(&Restore, sizeof(Restore));
    Restore.DeviceNode = DeviceNode;
    Restore.RequestSource = ArbiterRequestPnpEnumerated;
    Restore.Requirements = IopCmListToIoRequirements(DeviceNode->ResourceList, LCPRI_FORCECONFIG);
    Restore.AreRequirementsOwned = TRUE;

    if (Restore.Requirements != NULL)
    {
        IopPrepareRequest(&Restore);
        Status = Restore.Status;

        if (NT_SUCCESS(Status) && !Restore.IsSkipped)
        {
            Status = IopFindConfiguration(&Restore, &Restore.Selected, &ActiveArbiters);
            if (NT_SUCCESS(Status))
                Status = IopKeepConfiguration(DeviceNode, &ActiveArbiters);
        }
    }

    IopFreeRequest(&Restore);
    IopUpdateControlKeyWithResources(DeviceNode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to restore the resources of %wZ (Status 0x%08lx)\n",
                &DeviceNode->InstancePath, Status);
    }

    return Status;
}

/**
 * @brief
 * Assigns new resources to a started device whose requirements changed. The
 * current resources are kept when they still fit, and when nothing fits the
 * previous resources are assigned again.
 *
 * @param[out] Problem
 * Receives the problem of a device that must be removed, or 0.
 *
 * @return
 * TRUE if the device got new resources and must be started again.
 */
BOOLEAN
NTAPI
IopReallocateDeviceResources(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PULONG Problem)
{
    BOOLEAN WasResourceFree = !!(DeviceNode->Flags & DNF_NO_RESOURCE_REQUIRED);
    IOP_DEVICE_ASSIGNMENT Request;
    BOOLEAN IsRestarted = FALSE;
    NTSTATUS Status;

    PAGED_CODE();

    *Problem = 0;

    IopLockResourceAssignment();

    IopDeviceNodeClearFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);

    RtlZeroMemory(&Request, sizeof(Request));
    Request.DeviceNode = DeviceNode;
    Request.RequestSource = ArbiterRequestPnpEnumerated;

    Status = IopPrepareReassignment(&Request);
    if (NT_SUCCESS(Status) && !Request.IsSkipped)
    {
        if (NT_SUCCESS(IopCommitReassignment(&Request)))
        {
            IopInstallAssignment(&Request);
            IsRestarted = TRUE;
        }
        else if (DeviceNode->ResourceList != NULL &&
                 !NT_SUCCESS(IopRestoreAssignment(DeviceNode)))
        {
            *Problem = CM_PROB_NEED_RESTART;
        }
    }

    IopFreeRequest(&Request);

    if (!IsRestarted && WasResourceFree)
        IopDeviceNodeSetFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);

    IopUnlockResourceAssignment();

    return IsRestarted;
}

/* LEGACY RESOURCE CLAIMS ***************************************************/

/* Returns the device node of the resources a driver claimed for no device, or NULL */
static
PIOP_LEGACY_RESOURCE_OWNER
NTAPI
IopFindLegacyResourceOwner(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PLIST_ENTRY ListEntry;

    for (ListEntry = IopLegacyResourceOwnerList.Flink;
         ListEntry != &IopLegacyResourceOwnerList;
         ListEntry = ListEntry->Flink)
    {
        PIOP_LEGACY_RESOURCE_OWNER Owner =
            CONTAINING_RECORD(ListEntry, IOP_LEGACY_RESOURCE_OWNER, ListEntry);

        if (Owner->DriverObject == DriverObject)
            return Owner;
    }

    return NULL;
}

/**
 * @brief
 * Returns the device node that owns the legacy claim of a driver and device
 * pair, and creates it for a new claim.
 *
 * @param[in] CanCreate
 * FALSE to only look up an existing claim.
 *
 * @return
 * STATUS_SUCCESS, STATUS_NOT_FOUND when there is no claim to look up, or the
 * failure status of the creation.
 *
 * @remarks
 * Resources claimed for a device object are owned by that device object,
 * which gets a device node outside the device tree. Resources claimed for no
 * device are owned by a device object the PnP manager creates for the driver.
 */
static
NTSTATUS
NTAPI
IopGetLegacyDeviceNode(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN CanCreate,
    _Out_ PDEVICE_NODE *DeviceNode)
{
    PIOP_LEGACY_RESOURCE_OWNER Owner;
    PDEVICE_OBJECT Pdo;
    PDEVICE_NODE Node;
    NTSTATUS Status;

    *DeviceNode = NULL;

    if (DeviceObject != NULL)
    {
        Node = IopGetDeviceNode(DeviceObject);
        if (Node != NULL)
        {
            *DeviceNode = Node;
            return STATUS_SUCCESS;
        }

        if (!CanCreate)
            return STATUS_NOT_FOUND;

        /* A device of a bus always has its own device node */
        if (DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE)
            return STATUS_UNSUCCESSFUL;

        Node = PipAllocateDeviceNode(DeviceObject);
        if (Node == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        IopDeviceNodeSetFlag(Node, DNF_LEGACY_RESOURCE_DEVICENODE);
        *DeviceNode = Node;
        return STATUS_SUCCESS;
    }

    Owner = IopFindLegacyResourceOwner(DriverObject);
    if (Owner != NULL)
    {
        *DeviceNode = Owner->DeviceNode;
        return STATUS_SUCCESS;
    }

    if (!CanCreate)
        return STATUS_NOT_FOUND;

    Owner = ExAllocatePoolZero(PagedPool, sizeof(*Owner), TAG_IO_ARBITER);
    if (Owner == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = IoCreateDevice(IopRootDriverObject,
                            0,
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Owner, TAG_IO_ARBITER);
        return Status;
    }

    Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

    Node = PipAllocateDeviceNode(Pdo);
    if (Node == NULL)
    {
        IoDeleteDevice(Pdo);
        ExFreePoolWithTag(Owner, TAG_IO_ARBITER);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IopDeviceNodeSetFlag(Node, DNF_MADEUP | DNF_LEGACY_RESOURCE_DEVICENODE);
    PiSetDevNodeState(Node, DeviceNodeInitialized);

    Owner->DriverObject = DriverObject;
    Owner->DeviceNode = Node;
    InsertTailList(&IopLegacyResourceOwnerList, &Owner->ListEntry);

    *DeviceNode = Node;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Ends the legacy claim of a device node. The device node of a claim made
 * for no device is deleted with its device object, and the one of a claim
 * made for a device object is detached from it.
 */
static
VOID
NTAPI
IopEndLegacyClaim(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ PDEVICE_NODE DeviceNode)
{
    PIOP_LEGACY_RESOURCE_OWNER Owner = NULL;
    PLIST_ENTRY ListEntry;

    /* The device node of a PnP device is not the claim's */
    if (!(DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE))
        return;

    if (DeviceObject == NULL)
    {
        for (ListEntry = IopLegacyResourceOwnerList.Flink;
             ListEntry != &IopLegacyResourceOwnerList;
             ListEntry = ListEntry->Flink)
        {
            PIOP_LEGACY_RESOURCE_OWNER Entry =
                CONTAINING_RECORD(ListEntry, IOP_LEGACY_RESOURCE_OWNER, ListEntry);

            if (Entry->DeviceNode == DeviceNode)
            {
                Owner = Entry;
                break;
            }
        }

        if (Owner == NULL)
            return;

        RemoveEntryList(&Owner->ListEntry);
        ExFreePoolWithTag(Owner, TAG_IO_ARBITER);
    }

    IopDeviceNodeClearFlag(DeviceNode, DNF_LEGACY_RESOURCE_DEVICENODE);
    PiSetDevNodeState(DeviceNode, DeviceNodeRemoved);

    if (DeviceObject != NULL)
    {
        IopFreeDeviceNode(DeviceNode);
        return;
    }

    /* Deleting the device object also frees the device node */
    IoDeleteDevice(DeviceNode->PhysicalDeviceObject);
}

/* Returns a copy of a resource list, or NULL */
static
PCM_RESOURCE_LIST
NTAPI
IopDuplicateResourceList(
    _In_ PCM_RESOURCE_LIST ResourceList)
{
    ULONG Size = PnpDetermineResourceListSize(ResourceList);
    PCM_RESOURCE_LIST Copy;

    Copy = ExAllocatePoolWithTag(PagedPool, Size, TAG_IO_ARBITER);
    if (Copy != NULL)
        RtlCopyMemory(Copy, ResourceList, Size);

    return Copy;
}

/**
 * @brief
 * Updates the legacy claim of a driver and device pair. The previous
 * resources of the claim are freed first, and a claim that fails holds
 * nothing.
 *
 * @param[in] Requirements
 * The requirements of the claim, or NULL to end it.
 *
 * @param[in,out] AllocatedResources
 * If *AllocatedResources is not NULL, these resources are recorded for the
 * device instead of the assigned ones. Otherwise it receives the assigned
 * resources, which the caller frees.
 */
static
NTSTATUS
NTAPI
IopLegacyAllocate(
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST Requirements,
    _Inout_opt_ PCM_RESOURCE_LIST *AllocatedResources)
{
    IOP_DEVICE_ASSIGNMENT Request;
    PDEVICE_NODE DeviceNode;
    NTSTATUS Status;

    PAGED_CODE();

    IopLockResourceAssignment();

    /* Ending a claim that was never made has nothing to do */
    Status = IopGetLegacyDeviceNode(DriverObject, DeviceObject, Requirements != NULL, &DeviceNode);
    if (Status == STATUS_NOT_FOUND && Requirements == NULL)
        Status = STATUS_SUCCESS;

    if (DeviceNode == NULL)
        goto Done;

    if (DeviceNode->ResourceList != NULL || DeviceNode->BootResources != NULL)
        IopReleaseDeviceNodeResources(DeviceNode);

    if (Requirements == NULL)
    {
        IopEndLegacyClaim(DeviceObject, DeviceNode);
        goto Done;
    }

    RtlZeroMemory(&Request, sizeof(Request));
    Request.DeviceNode = DeviceNode;
    Request.RequestSource = RequestSource;
    Request.Requirements = Requirements;

    IopAllocateRequests(&Request, 1);

    Status = Request.Status;

    if (NT_SUCCESS(Status))
    {
        PCM_RESOURCE_LIST Recorded = (AllocatedResources != NULL && *AllocatedResources != NULL)
                                         ? *AllocatedResources
                                         : Request.ResourceList;

        if (Recorded != NULL)
            DeviceNode->ResourceList = IopDuplicateResourceList(Recorded);

        if (Recorded != NULL && DeviceNode->ResourceList == NULL)
        {
            IopReleaseFailedAssignment(&Request);
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            DeviceNode->ResourceListTranslated = Request.TranslatedList;
            Request.TranslatedList = NULL;

            if (AllocatedResources != NULL && *AllocatedResources == NULL)
            {
                *AllocatedResources = Request.ResourceList;
                Request.ResourceList = NULL;
            }

            /* The registry values are not needed for the claim to work */
            if (DeviceNode->ResourceList == NULL || DeviceNode->ResourceListTranslated != NULL)
                IopUpdateResourceMapForPnPDevice(DeviceNode);
        }
    }

    /* A claim that failed holds nothing */
    if (!NT_SUCCESS(Status))
        IopEndLegacyClaim(DeviceObject, DeviceNode);

    IopFreeRequest(&Request);

    if (Request.ResourceList != NULL)
        ExFreePoolWithTag(Request.ResourceList, TAG_IO_ARBITER);
    if (Request.TranslatedList != NULL)
        ExFreePoolWithTag(Request.TranslatedList, TAG_IO_ARBITER);

Done:
    IopUnlockResourceAssignment();
    return Status;
}

/**
 * @brief
 * Moves requirements on the internal bus to the default bus, for a second
 * try of a legacy claim.
 *
 * @param[in,out] AllocatedResources
 * The list the claim records, or NULL. When requirements were moved and the
 * list has internal descriptors too, it receives a copy with the same change,
 * which the caller frees.
 *
 * @return
 * TRUE if requirements were moved and the copy, when needed, was made.
 */
static
BOOLEAN
NTAPI
IopMoveInternalRequirements(
    _Inout_ PIO_RESOURCE_REQUIREMENTS_LIST Requirements,
    _Inout_opt_ PCM_RESOURCE_LIST *AllocatedResources)
{
    PIO_RESOURCE_LIST List = &Requirements->List[0];
    PCM_FULL_RESOURCE_DESCRIPTOR Full;
    PCM_RESOURCE_LIST Copy;
    BOOLEAN IsChanged = FALSE;
    ULONG ListIndex;

    if (Requirements->InterfaceType == Internal)
    {
        Requirements->InterfaceType = PnpDefaultInterfaceType;
        IsChanged = TRUE;
    }

    for (ListIndex = 0; ListIndex < Requirements->AlternativeLists; ListIndex++)
    {
        ULONG Index;

        for (Index = 0; Index < List->Count; Index++)
        {
            PIO_RESOURCE_DESCRIPTOR Descriptor = &List->Descriptors[Index];

            if (Descriptor->Type == IOP_RESOURCE_TYPE_LEGACY_BUS &&
                Descriptor->u.DevicePrivate.Data[0] == Internal)
            {
                Descriptor->u.DevicePrivate.Data[0] = PnpDefaultInterfaceType;
                IsChanged = TRUE;
            }
        }

        List = IopNextAlternativeList(List);
    }

    if (!IsChanged || AllocatedResources == NULL || *AllocatedResources == NULL)
        return IsChanged;

    Copy = IopDuplicateResourceList(*AllocatedResources);
    if (Copy == NULL)
        return FALSE;

    Full = &Copy->List[0];
    for (ListIndex = 0; ListIndex < Copy->Count; ListIndex++)
    {
        if (Full->InterfaceType == Internal)
            Full->InterfaceType = PnpDefaultInterfaceType;

        Full = IopNextFullDescriptor(Full);
    }

    *AllocatedResources = Copy;
    return TRUE;
}

/**
 * @brief
 * Assigns the resources a legacy driver requests with IoAssignResources.
 *
 * @param[in] Requirements
 * The requested resources, or NULL to free the previous claim.
 *
 * @param[out] AllocatedResources
 * If not NULL, receives the assigned resources. The caller frees them.
 */
NTSTATUS
NTAPI
IopLegacyAssignResources(
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PIO_RESOURCE_REQUIREMENTS_LIST Requirements,
    _Out_opt_ PCM_RESOURCE_LIST *AllocatedResources)
{
    PCM_RESOURCE_LIST Assigned = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    if (AllocatedResources != NULL)
        *AllocatedResources = NULL;

    if (!IopResourceAssignmentReady)
        return STATUS_UNSUCCESSFUL;

    Status = IopLegacyAllocate(RequestSource,
                               DriverObject,
                               DeviceObject,
                               Requirements,
                               &Assigned);

    if (AllocatedResources != NULL)
        *AllocatedResources = Assigned;
    else if (Assigned != NULL)
        ExFreePoolWithTag(Assigned, TAG_IO_ARBITER);

    return Status;
}

/**
 * @brief
 * Claims the resources a legacy driver reports with IoReportResourceUsage,
 * IoReportResourceForDetection or IoReportDetectedDevice. Resources that
 * conflict are not claimed.
 *
 * @param[in] ResourceList
 * The resources in use, or NULL to free the previous claim.
 *
 * @param[out] ConflictDetected
 * Set to TRUE if the resources were not claimed.
 *
 * @return
 * STATUS_SUCCESS, STATUS_CONFLICTING_ADDRESSES, STATUS_INSUFFICIENT_RESOURCES,
 * or STATUS_UNSUCCESSFUL before the arbiters exist.
 */
NTSTATUS
NTAPI
IopLegacyReportResources(
    _In_ ARBITER_REQUEST_SOURCE RequestSource,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PCM_RESOURCE_LIST ResourceList,
    _Out_ PBOOLEAN ConflictDetected)
{
    PIO_RESOURCE_REQUIREMENTS_LIST Requirements = NULL;
    PCM_RESOURCE_LIST Recorded = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    *ConflictDetected = TRUE;

    if (!IopResourceAssignmentReady)
        return STATUS_UNSUCCESSFUL;

    /* An empty list ends the claim like a NULL one */
    if (ResourceList != NULL &&
        ResourceList->Count != 0 &&
        ResourceList->List[0].PartialResourceList.Count != 0)
    {
        Requirements = IopCmListToIoRequirements(ResourceList, LCPRI_NORMAL);
        if (Requirements == NULL)
            return STATUS_UNSUCCESSFUL;

        Recorded = ResourceList;
    }

    Status = IopLegacyAllocate(RequestSource, DriverObject, DeviceObject, Requirements, &Recorded);

    /* A claim on the internal bus may still fit on the default bus */
    if (!NT_SUCCESS(Status) && Requirements != NULL &&
        IopMoveInternalRequirements(Requirements, &Recorded))
    {
        Status = IopLegacyAllocate(RequestSource,
                                   DriverObject,
                                   DeviceObject,
                                   Requirements,
                                   &Recorded);

        if (Recorded != ResourceList)
            ExFreePoolWithTag(Recorded, TAG_IO_ARBITER);
    }

    if (Requirements != NULL)
        ExFreePoolWithTag(Requirements, TAG_IO_ARBITER);

    if (NT_SUCCESS(Status))
    {
        *ConflictDetected = FALSE;
        return STATUS_SUCCESS;
    }

    return (Status == STATUS_INSUFFICIENT_RESOURCES) ? Status : STATUS_CONFLICTING_ADDRESSES;
}

/**
 * @brief
 * Ends the claim a driver that is deleted made for no device, so its device
 * object does not outlive the driver.
 */
VOID
NTAPI
IopReleaseLegacyDriverClaims(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    if (!IopResourceAssignmentReady)
        return;

    IopLegacyAllocate(ArbiterRequestUndefined, DriverObject, NULL, NULL, NULL);
}

/**
 * @brief
 * Ends the legacy claim of a device object that is deleted. Called before
 * its device node is freed.
 */
VOID
NTAPI
IopReleaseLegacyDeviceNode(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    IopLockResourceAssignment();

    IopReleaseDeviceNodeResources(DeviceNode);
    IopDeviceNodeClearFlag(DeviceNode, DNF_LEGACY_RESOURCE_DEVICENODE);
    PiSetDevNodeState(DeviceNode, DeviceNodeRemoved);

    IopUnlockResourceAssignment();
}

/**
 * @brief
 * Records the resources of a device reported with IoReportDetectedDevice. The
 * resources a driver did not assign itself are claimed like legacy resources.
 *
 * @return
 * STATUS_SUCCESS, or the failure status of the claim.
 */
NTSTATUS
NTAPI
IopReportDetectedResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_opt_ PCM_RESOURCE_LIST ResourceList,
    _In_ BOOLEAN ResourceAssigned)
{
    PDEVICE_OBJECT Pdo = DeviceNode->PhysicalDeviceObject;
    BOOLEAN IsConflicting;

    PAGED_CODE();

    if (ResourceAssigned)
    {
        IopDeviceNodeSetFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);
        IopWriteControlValue(DeviceNode,
                             L"AllocConfig",
                             REG_RESOURCE_LIST,
                             ResourceList,
                             PnpDetermineResourceListSize(ResourceList));
        return STATUS_SUCCESS;
    }

    if (ResourceList == NULL ||
        ResourceList->Count == 0 ||
        ResourceList->List[0].PartialResourceList.Count == 0)
    {
        IopDeviceNodeSetFlag(DeviceNode, DNF_NO_RESOURCE_REQUIRED);
        return STATUS_SUCCESS;
    }

    return IopLegacyReportResources(ArbiterRequestLegacyReported,
                                    Pdo->DriverObject,
                                    Pdo,
                                    ResourceList,
                                    &IsConflicting);
}

/**
 * @brief
 * Copies a resource list of a device for IoGetDeviceProperty. A missing list
 * returns 0 bytes.
 *
 * @param[in] Translated
 * For the allocated resources, the translated list that follows the raw one.
 *
 * @return
 * STATUS_SUCCESS, or STATUS_BUFFER_TOO_SMALL with the size in ResultLength.
 */
static
NTSTATUS
NTAPI
IopCopyDeviceProperty(
    _In_opt_ PVOID Data,
    _In_ ULONG Size,
    _In_opt_ PCM_RESOURCE_LIST Translated,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_opt_(BufferLength) PVOID PropertyBuffer,
    _Out_ PULONG ResultLength)
{
    ULONG TranslatedSize = (Translated != NULL) ? PnpDetermineResourceListSize(Translated) : 0;

    *ResultLength = (Data != NULL) ? Size + TranslatedSize : 0;

    if (*ResultLength > BufferLength)
        return STATUS_BUFFER_TOO_SMALL;

    if (*ResultLength != 0)
    {
        RtlCopyMemory(PropertyBuffer, Data, Size);
        RtlCopyMemory((PUCHAR)PropertyBuffer + Size, Translated, TranslatedSize);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Returns DevicePropertyResourceRequirements or DevicePropertyAllocatedResources
 * of a device. The allocated resources are the raw list followed by the
 * translated list, and are only returned when both exist.
 */
NTSTATUS
NTAPI
IopGetResourceProperty(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_opt_(BufferLength) PVOID PropertyBuffer,
    _Out_ PULONG ResultLength)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (DeviceProperty == DevicePropertyResourceRequirements)
    {
        PIO_RESOURCE_REQUIREMENTS_LIST Requirements = DeviceNode->ResourceRequirements;

        Status = IopCopyDeviceProperty(Requirements,
                                       (Requirements != NULL) ? Requirements->ListSize : 0,
                                       NULL,
                                       BufferLength,
                                       PropertyBuffer,
                                       ResultLength);
    }
    else if (DeviceNode->ResourceList != NULL && DeviceNode->ResourceListTranslated != NULL)
    {
        Status = IopCopyDeviceProperty(DeviceNode->ResourceList,
                                       PnpDetermineResourceListSize(DeviceNode->ResourceList),
                                       DeviceNode->ResourceListTranslated,
                                       BufferLength,
                                       PropertyBuffer,
                                       ResultLength);
    }
    else
    {
        *ResultLength = 0;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

/* EOF */
