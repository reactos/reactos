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

/* TRUE once IopRegisterRootArbiters has initialized the lock and the lists */
static BOOLEAN IopResourceAssignmentReady = FALSE;

/* Serializes all arbiter operations */
static ERESOURCE IopResourceAssignmentLock;

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
        ExFreePool(IopHalPendingResources);
        IopHalPendingResources = NULL;
    }

    IopUnlockResourceAssignment();
}

/* LEGACY RESOURCE HANDLING *************************************************/

FORCEINLINE
PIO_RESOURCE_LIST
IopGetNextResourceList(
    _In_ const IO_RESOURCE_LIST *ResourceList)
{
    ASSERT((ResourceList->Count > 0) && (ResourceList->Count < 1000));
    return (PIO_RESOURCE_LIST)(
        &ResourceList->Descriptors[ResourceList->Count]);
}

static
BOOLEAN
IopCheckDescriptorForConflict(
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc,
    OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
    CM_RESOURCE_LIST CmList;
    NTSTATUS Status;

    CmList.Count = 1;
    CmList.List[0].InterfaceType = InterfaceTypeUndefined;
    CmList.List[0].BusNumber = 0;
    CmList.List[0].PartialResourceList.Version = 1;
    CmList.List[0].PartialResourceList.Revision = 1;
    CmList.List[0].PartialResourceList.Count = 1;
    CmList.List[0].PartialResourceList.PartialDescriptors[0] = *CmDesc;

    Status = IopDetectResourceConflict(&CmList, TRUE, ConflictingDescriptor);
    if (Status == STATUS_CONFLICTING_ADDRESSES)
        return TRUE;

    return FALSE;
}

static
BOOLEAN
IopFindBusNumberResource(
    IN PIO_RESOURCE_DESCRIPTOR IoDesc,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
    ULONG Start;
    CM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDesc;

    ASSERT(IoDesc->Type == CmDesc->Type);
    ASSERT(IoDesc->Type == CmResourceTypeBusNumber);

    for (Start = IoDesc->u.BusNumber.MinBusNumber;
         Start <= IoDesc->u.BusNumber.MaxBusNumber - IoDesc->u.BusNumber.Length + 1;
         Start++)
    {
        CmDesc->u.BusNumber.Length = IoDesc->u.BusNumber.Length;
        CmDesc->u.BusNumber.Start = Start;

        if (IopCheckDescriptorForConflict(CmDesc, &ConflictingDesc))
        {
            Start += ConflictingDesc.u.BusNumber.Start + ConflictingDesc.u.BusNumber.Length;
        }
        else
        {
            DPRINT1("Satisfying bus number requirement with 0x%x (length: 0x%x)\n", Start, CmDesc->u.BusNumber.Length);
            return TRUE;
        }
    }

    return FALSE;
}

static
BOOLEAN
IopFindMemoryResource(
    IN PIO_RESOURCE_DESCRIPTOR IoDesc,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
    ULONGLONG Start;
    CM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDesc;

    ASSERT(IoDesc->Type == CmDesc->Type);
    ASSERT(IoDesc->Type == CmResourceTypeMemory);

    /* HACK */
    if (IoDesc->u.Memory.Alignment == 0)
        IoDesc->u.Memory.Alignment = 1;

    for (Start = (ULONGLONG)IoDesc->u.Memory.MinimumAddress.QuadPart;
         Start <= (ULONGLONG)IoDesc->u.Memory.MaximumAddress.QuadPart - IoDesc->u.Memory.Length + 1;
         Start += IoDesc->u.Memory.Alignment)
    {
        CmDesc->u.Memory.Length = IoDesc->u.Memory.Length;
        CmDesc->u.Memory.Start.QuadPart = (LONGLONG)Start;

        if (IopCheckDescriptorForConflict(CmDesc, &ConflictingDesc))
        {
            Start += (ULONGLONG)ConflictingDesc.u.Memory.Start.QuadPart +
                     ConflictingDesc.u.Memory.Length;
        }
        else
        {
            DPRINT1("Satisfying memory requirement with 0x%I64x (length: 0x%x)\n", Start, CmDesc->u.Memory.Length);
            return TRUE;
        }
    }

    return FALSE;
}

static
BOOLEAN
IopFindPortResource(
    IN PIO_RESOURCE_DESCRIPTOR IoDesc,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
    ULONGLONG Start;
    CM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDesc;

    ASSERT(IoDesc->Type == CmDesc->Type);
    ASSERT(IoDesc->Type == CmResourceTypePort);

    /* HACK */
    if (IoDesc->u.Port.Alignment == 0)
        IoDesc->u.Port.Alignment = 1;

    for (Start = (ULONGLONG)IoDesc->u.Port.MinimumAddress.QuadPart;
         Start <= (ULONGLONG)IoDesc->u.Port.MaximumAddress.QuadPart - IoDesc->u.Port.Length + 1;
         Start += IoDesc->u.Port.Alignment)
    {
        CmDesc->u.Port.Length = IoDesc->u.Port.Length;
        CmDesc->u.Port.Start.QuadPart = (LONGLONG)Start;

        if (IopCheckDescriptorForConflict(CmDesc, &ConflictingDesc))
        {
            Start += (ULONGLONG)ConflictingDesc.u.Port.Start.QuadPart + ConflictingDesc.u.Port.Length;
        }
        else
        {
            DPRINT("Satisfying port requirement with 0x%I64x (length: 0x%x)\n", Start, CmDesc->u.Port.Length);
            return TRUE;
        }
    }

    DPRINT1("IopFindPortResource failed!\n");
    return FALSE;
}

static
BOOLEAN
IopFindDmaResource(
    IN PIO_RESOURCE_DESCRIPTOR IoDesc,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
    ULONG Channel;

    ASSERT(IoDesc->Type == CmDesc->Type);
    ASSERT(IoDesc->Type == CmResourceTypeDma);

    for (Channel = IoDesc->u.Dma.MinimumChannel;
         Channel <= IoDesc->u.Dma.MaximumChannel;
         Channel++)
    {
        CmDesc->u.Dma.Channel = Channel;
        CmDesc->u.Dma.Port = 0;

        if (!IopCheckDescriptorForConflict(CmDesc, NULL))
        {
            DPRINT1("Satisfying DMA requirement with channel 0x%x\n", Channel);
            return TRUE;
        }
    }

    return FALSE;
}

static
BOOLEAN
IopFindInterruptResource(
    IN PIO_RESOURCE_DESCRIPTOR IoDesc,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc)
{
    ULONG Vector;

    ASSERT(IoDesc->Type == CmDesc->Type);
    ASSERT(IoDesc->Type == CmResourceTypeInterrupt);

    for (Vector = IoDesc->u.Interrupt.MinimumVector;
         Vector <= IoDesc->u.Interrupt.MaximumVector;
         Vector++)
    {
        CmDesc->u.Interrupt.Vector = Vector;
        CmDesc->u.Interrupt.Level = Vector;
        CmDesc->u.Interrupt.Affinity = (KAFFINITY)-1;

        if (!IopCheckDescriptorForConflict(CmDesc, NULL))
        {
            DPRINT1("Satisfying interrupt requirement with IRQ 0x%x\n", Vector);
            return TRUE;
        }
    }

    DPRINT1("Failed to satisfy interrupt requirement with IRQ 0x%x-0x%x\n",
            IoDesc->u.Interrupt.MinimumVector,
            IoDesc->u.Interrupt.MaximumVector);
    return FALSE;
}

NTSTATUS NTAPI
IopFixupResourceListWithRequirements(
    IN PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList,
    OUT PCM_RESOURCE_LIST *ResourceList)
{
    ULONG i, OldCount;
    BOOLEAN AlternateRequired = FALSE;
    PIO_RESOURCE_LIST ResList;

    /* Save the initial resource count when we got here so we can restore if an alternate fails */
    if (*ResourceList != NULL)
        OldCount = (*ResourceList)->List[0].PartialResourceList.Count;
    else
        OldCount = 0;

    ResList = &RequirementsList->List[0];
    for (i = 0; i < RequirementsList->AlternativeLists; i++, ResList = IopGetNextResourceList(ResList))
    {
        ULONG ii;

        /* We need to get back to where we were before processing the last alternative list */
        if (OldCount == 0 && *ResourceList != NULL)
        {
            /* Just free it and kill the pointer */
            ExFreePool(*ResourceList);
            *ResourceList = NULL;
        }
        else if (OldCount != 0)
        {
            PCM_RESOURCE_LIST NewList;

            /* Let's resize it */
            (*ResourceList)->List[0].PartialResourceList.Count = OldCount;

            /* Allocate the new smaller list */
            NewList = ExAllocatePool(PagedPool, PnpDetermineResourceListSize(*ResourceList));
            if (!NewList)
                return STATUS_NO_MEMORY;

            /* Copy the old stuff back */
            RtlCopyMemory(NewList, *ResourceList, PnpDetermineResourceListSize(*ResourceList));

            /* Free the old one */
            ExFreePool(*ResourceList);

            /* Store the pointer to the new one */
            *ResourceList = NewList;
        }

        for (ii = 0; ii < ResList->Count; ii++)
        {
            ULONG iii;
            PCM_PARTIAL_RESOURCE_LIST PartialList = (*ResourceList) ? &(*ResourceList)->List[0].PartialResourceList : NULL;
            PIO_RESOURCE_DESCRIPTOR IoDesc = &ResList->Descriptors[ii];
            BOOLEAN Matched = FALSE;

            /* Skip alternates if we don't need one */
            if (!AlternateRequired && (IoDesc->Option & IO_RESOURCE_ALTERNATIVE))
            {
                DPRINT("Skipping unneeded alternate\n");
                continue;
            }

            /* Check if we couldn't satsify a requirement or its alternates */
            if (AlternateRequired && !(IoDesc->Option & IO_RESOURCE_ALTERNATIVE))
            {
                DPRINT1("Unable to satisfy preferred resource or alternates in list %lu\n", i);

                /* Break out of this loop and try the next list */
                break;
            }

            for (iii = 0; PartialList && iii < PartialList->Count && !Matched; iii++)
            {
                /* Partial resource descriptors can be of variable size (CmResourceTypeDeviceSpecific),
                   but only one is allowed and it must be the last one in the list! */
                PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc = &PartialList->PartialDescriptors[iii];

                /* First check types */
                if (IoDesc->Type != CmDesc->Type)
                    continue;

                switch (IoDesc->Type)
                {
                    case CmResourceTypeInterrupt:
                        /* Make sure it satisfies our vector range */
                        if (CmDesc->u.Interrupt.Vector >= IoDesc->u.Interrupt.MinimumVector &&
                            CmDesc->u.Interrupt.Vector <= IoDesc->u.Interrupt.MaximumVector)
                        {
                            /* Found it */
                            Matched = TRUE;
                        }
                        else
                        {
                            DPRINT("Interrupt - Not a match! 0x%x not inside 0x%x to 0x%x\n",
                                   CmDesc->u.Interrupt.Vector,
                                   IoDesc->u.Interrupt.MinimumVector,
                                   IoDesc->u.Interrupt.MaximumVector);
                        }
                        break;

                    case CmResourceTypeMemory:
                    case CmResourceTypePort:
                        /* Make sure the length matches and it satisfies our address range */
                        if (CmDesc->u.Memory.Length == IoDesc->u.Memory.Length &&
                            (ULONGLONG)CmDesc->u.Memory.Start.QuadPart >= (ULONGLONG)IoDesc->u.Memory.MinimumAddress.QuadPart &&
                            (ULONGLONG)CmDesc->u.Memory.Start.QuadPart + CmDesc->u.Memory.Length - 1 <= (ULONGLONG)IoDesc->u.Memory.MaximumAddress.QuadPart)
                        {
                            /* Found it */
                            Matched = TRUE;
                        }
                        else
                        {
                            DPRINT("Memory/Port - Not a match! 0x%I64x with length 0x%x not inside 0x%I64x to 0x%I64x with length 0x%x\n",
                                   CmDesc->u.Memory.Start.QuadPart,
                                   CmDesc->u.Memory.Length,
                                   IoDesc->u.Memory.MinimumAddress.QuadPart,
                                   IoDesc->u.Memory.MaximumAddress.QuadPart,
                                   IoDesc->u.Memory.Length);
                        }
                        break;

                    case CmResourceTypeBusNumber:
                        /* Make sure the length matches and it satisfies our bus number range */
                        if (CmDesc->u.BusNumber.Length == IoDesc->u.BusNumber.Length &&
                            CmDesc->u.BusNumber.Start >= IoDesc->u.BusNumber.MinBusNumber &&
                            CmDesc->u.BusNumber.Start + CmDesc->u.BusNumber.Length - 1 <= IoDesc->u.BusNumber.MaxBusNumber)
                        {
                            /* Found it */
                            Matched = TRUE;
                        }
                        else
                        {
                            DPRINT("Bus Number - Not a match! 0x%x with length 0x%x not inside 0x%x to 0x%x with length 0x%x\n",
                                   CmDesc->u.BusNumber.Start,
                                   CmDesc->u.BusNumber.Length,
                                   IoDesc->u.BusNumber.MinBusNumber,
                                   IoDesc->u.BusNumber.MaxBusNumber,
                                   IoDesc->u.BusNumber.Length);
                        }
                        break;

                    case CmResourceTypeDma:
                        /* Make sure it fits in our channel range */
                        if (CmDesc->u.Dma.Channel >= IoDesc->u.Dma.MinimumChannel &&
                            CmDesc->u.Dma.Channel <= IoDesc->u.Dma.MaximumChannel)
                        {
                            /* Found it */
                            Matched = TRUE;
                        }
                        else
                        {
                            DPRINT("DMA - Not a match! 0x%x not inside 0x%x to 0x%x\n",
                                   CmDesc->u.Dma.Channel,
                                   IoDesc->u.Dma.MinimumChannel,
                                   IoDesc->u.Dma.MaximumChannel);
                        }
                        break;

                    default:
                        /* Other stuff is fine */
                        Matched = TRUE;
                        break;
                }
            }

            /* Check if we found a matching descriptor */
            if (!Matched)
            {
                PCM_RESOURCE_LIST NewList;
                CM_PARTIAL_RESOURCE_DESCRIPTOR NewDesc;
                PCM_PARTIAL_RESOURCE_DESCRIPTOR DescPtr;
                BOOLEAN FoundResource = TRUE;

                /* Setup the new CM descriptor */
                NewDesc.Type = IoDesc->Type;
                NewDesc.Flags = IoDesc->Flags;
                NewDesc.ShareDisposition = IoDesc->ShareDisposition;

                /* Let'se see if we can find a resource to satisfy this */
                switch (IoDesc->Type)
                {
                    case CmResourceTypeInterrupt:
                        /* Find an available interrupt */
                        if (!IopFindInterruptResource(IoDesc, &NewDesc))
                        {
                            DPRINT1("Failed to find an available interrupt resource (0x%x to 0x%x)\n",
                                    IoDesc->u.Interrupt.MinimumVector, IoDesc->u.Interrupt.MaximumVector);

                            FoundResource = FALSE;
                        }
                        break;

                    case CmResourceTypePort:
                        /* Find an available port range */
                        if (!IopFindPortResource(IoDesc, &NewDesc))
                        {
                            DPRINT1("Failed to find an available port resource (0x%I64x to 0x%I64x length: 0x%x)\n",
                                    IoDesc->u.Port.MinimumAddress.QuadPart, IoDesc->u.Port.MaximumAddress.QuadPart,
                                    IoDesc->u.Port.Length);

                            FoundResource = FALSE;
                        }
                        break;

                    case CmResourceTypeMemory:
                        /* Find an available memory range */
                        if (!IopFindMemoryResource(IoDesc, &NewDesc))
                        {
                            DPRINT1("Failed to find an available memory resource (0x%I64x to 0x%I64x length: 0x%x)\n",
                                    IoDesc->u.Memory.MinimumAddress.QuadPart, IoDesc->u.Memory.MaximumAddress.QuadPart,
                                    IoDesc->u.Memory.Length);

                            FoundResource = FALSE;
                        }
                        break;

                    case CmResourceTypeBusNumber:
                        /* Find an available bus address range */
                        if (!IopFindBusNumberResource(IoDesc, &NewDesc))
                        {
                            DPRINT1("Failed to find an available bus number resource (0x%x to 0x%x length: 0x%x)\n",
                                    IoDesc->u.BusNumber.MinBusNumber, IoDesc->u.BusNumber.MaxBusNumber,
                                    IoDesc->u.BusNumber.Length);

                            FoundResource = FALSE;
                        }
                        break;

                    case CmResourceTypeDma:
                        /* Find an available DMA channel */
                        if (!IopFindDmaResource(IoDesc, &NewDesc))
                        {
                            DPRINT1("Failed to find an available dma resource (0x%x to 0x%x)\n",
                                    IoDesc->u.Dma.MinimumChannel, IoDesc->u.Dma.MaximumChannel);

                            FoundResource = FALSE;
                        }
                        break;

                    default:
                        DPRINT1("Unsupported resource type: %x\n", IoDesc->Type);
                        FoundResource = FALSE;
                        break;
                }

                /* Check if it's missing and required */
                if (!FoundResource && IoDesc->Option == 0)
                {
                    /* Break out of this loop and try the next list */
                    DPRINT1("Unable to satisfy required resource in list %lu\n", i);
                    break;
                }
                else if (!FoundResource)
                {
                    /* Try an alternate for this preferred descriptor */
                    AlternateRequired = TRUE;
                    continue;
                }
                else
                {
                    /* Move on to the next preferred or required descriptor after this one */
                    AlternateRequired = FALSE;
                }

                /* Figure out what we need */
                if (PartialList == NULL)
                {
                    /* We need a new list */
                    NewList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST));
                    if (!NewList)
                        return STATUS_NO_MEMORY;

                    /* Set it up */
                    NewList->Count = 1;
                    NewList->List[0].InterfaceType = RequirementsList->InterfaceType;
                    NewList->List[0].BusNumber = RequirementsList->BusNumber;
                    NewList->List[0].PartialResourceList.Version = 1;
                    NewList->List[0].PartialResourceList.Revision = 1;
                    NewList->List[0].PartialResourceList.Count = 1;

                    /* Set our pointer */
                    DescPtr = &NewList->List[0].PartialResourceList.PartialDescriptors[0];
                }
                else
                {
                    /* Allocate the new larger list */
                    NewList = ExAllocatePool(PagedPool, PnpDetermineResourceListSize(*ResourceList) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                    if (!NewList)
                        return STATUS_NO_MEMORY;

                    /* Copy the old stuff back */
                    RtlCopyMemory(NewList, *ResourceList, PnpDetermineResourceListSize(*ResourceList));

                    /* Set our pointer */
                    DescPtr = &NewList->List[0].PartialResourceList.PartialDescriptors[NewList->List[0].PartialResourceList.Count];

                    /* Increment the descriptor count */
                    NewList->List[0].PartialResourceList.Count++;

                    /* Free the old list */
                    ExFreePool(*ResourceList);
                }

                /* Copy the descriptor in */
                *DescPtr = NewDesc;

                /* Store the new list */
                *ResourceList = NewList;
            }
        }

        /* Check if we need an alternate with no resources left */
        if (AlternateRequired)
        {
            DPRINT1("Unable to satisfy preferred resource or alternates in list %lu\n", i);

            /* Try the next alternate list */
            continue;
        }

        /* We're done because we satisfied one of the alternate lists */
        return STATUS_SUCCESS;
    }

    /* We ran out of alternates */
    DPRINT1("Out of alternate lists!\n");

    /* Free the list */
    if (*ResourceList)
    {
        ExFreePool(*ResourceList);
        *ResourceList = NULL;
    }

    /* Fail */
    return STATUS_CONFLICTING_ADDRESSES;
}

static
BOOLEAN
IopCheckResourceDescriptor(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc,
    IN PCM_RESOURCE_LIST ResourceList,
    IN BOOLEAN Silent,
    OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
    ULONG i, ii;
    BOOLEAN Result = FALSE;
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;

    FullDescriptor = &ResourceList->List[0];
    for (i = 0; i < ResourceList->Count; i++)
    {
        PCM_PARTIAL_RESOURCE_LIST ResList = &FullDescriptor->PartialResourceList;
        FullDescriptor = CmiGetNextResourceDescriptor(FullDescriptor);

        for (ii = 0; ii < ResList->Count; ii++)
        {
            /* Partial resource descriptors can be of variable size (CmResourceTypeDeviceSpecific),
               but only one is allowed and it must be the last one in the list! */
            PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc2 = &ResList->PartialDescriptors[ii];

            /* We don't care about shared resources */
            if (ResDesc->ShareDisposition == CmResourceShareShared &&
                ResDesc2->ShareDisposition == CmResourceShareShared)
                continue;

            /* Make sure we're comparing the same types */
            if (ResDesc->Type != ResDesc2->Type)
                continue;

            switch (ResDesc->Type)
            {
                case CmResourceTypeMemory:
                {
                    /* NOTE: ranges are in a form [x1;x2) */
                    UINT64 rStart = (UINT64)ResDesc->u.Memory.Start.QuadPart;
                    UINT64 rEnd = (UINT64)ResDesc->u.Memory.Start.QuadPart
                                  + ResDesc->u.Memory.Length;
                    UINT64 r2Start = (UINT64)ResDesc2->u.Memory.Start.QuadPart;
                    UINT64 r2End = (UINT64)ResDesc2->u.Memory.Start.QuadPart
                                   + ResDesc2->u.Memory.Length;

                    if (rStart < r2End && r2Start < rEnd)
                    {
                        if (!Silent)
                        {
                            DPRINT1("Resource conflict: Memory (0x%I64x to 0x%I64x vs. 0x%I64x to 0x%I64x)\n",
                                    rStart, rEnd, r2Start, r2End);
                        }

                        Result = TRUE;

                        goto ByeBye;
                    }
                    break;
                }
                case CmResourceTypePort:
                {
                    /* NOTE: ranges are in a form [x1;x2) */
                    UINT64 rStart = (UINT64)ResDesc->u.Port.Start.QuadPart;
                    UINT64 rEnd = (UINT64)ResDesc->u.Port.Start.QuadPart
                                  + ResDesc->u.Port.Length;
                    UINT64 r2Start = (UINT64)ResDesc2->u.Port.Start.QuadPart;
                    UINT64 r2End = (UINT64)ResDesc2->u.Port.Start.QuadPart
                                   + ResDesc2->u.Port.Length;

                    if (rStart < r2End && r2Start < rEnd)
                    {
                        if (!Silent)
                        {
                            DPRINT1("Resource conflict: Port (0x%I64x to 0x%I64x vs. 0x%I64x to 0x%I64x)\n",
                                    rStart, rEnd, r2Start, r2End);
                        }

                        Result = TRUE;

                        goto ByeBye;
                    }
                    break;
                }
                case CmResourceTypeInterrupt:
                {
                    if (ResDesc->u.Interrupt.Vector == ResDesc2->u.Interrupt.Vector)
                    {
                        if (!Silent)
                        {
                            DPRINT1("Resource conflict: IRQ (0x%x 0x%x vs. 0x%x 0x%x)\n",
                                    ResDesc->u.Interrupt.Vector, ResDesc->u.Interrupt.Level,
                                    ResDesc2->u.Interrupt.Vector, ResDesc2->u.Interrupt.Level);
                        }

                        Result = TRUE;

                        goto ByeBye;
                    }
                    break;
                }
                case CmResourceTypeBusNumber:
                {
                    /* NOTE: ranges are in a form [x1;x2) */
                    UINT32 rStart = ResDesc->u.BusNumber.Start;
                    UINT32 rEnd = ResDesc->u.BusNumber.Start + ResDesc->u.BusNumber.Length;
                    UINT32 r2Start = ResDesc2->u.BusNumber.Start;
                    UINT32 r2End = ResDesc2->u.BusNumber.Start + ResDesc2->u.BusNumber.Length;

                    if (rStart < r2End && r2Start < rEnd)
                    {
                        if (!Silent)
                        {
                            DPRINT1("Resource conflict: Bus number (0x%x to 0x%x vs. 0x%x to 0x%x)\n",
                                    rStart, rEnd, r2Start, r2End);
                        }

                        Result = TRUE;

                        goto ByeBye;
                    }
                    break;
                }
                case CmResourceTypeDma:
                {
                    if (ResDesc->u.Dma.Channel == ResDesc2->u.Dma.Channel)
                    {
                        if (!Silent)
                        {
                            DPRINT1("Resource conflict: Dma (0x%x 0x%x vs. 0x%x 0x%x)\n",
                                    ResDesc->u.Dma.Channel, ResDesc->u.Dma.Port,
                                    ResDesc2->u.Dma.Channel, ResDesc2->u.Dma.Port);
                        }

                        Result = TRUE;

                        goto ByeBye;
                    }
                    break;
                }
            }
        }
    }

ByeBye:

    if (Result && ConflictingDescriptor)
    {
        RtlCopyMemory(ConflictingDescriptor,
                      ResDesc,
                      sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    }

    // Hacked, because after fixing resource list parsing
    // we actually detect resource conflicts
    return Silent ? Result : FALSE; // Result;
}

static
NTSTATUS
IopUpdateControlKeyWithResources(
    IN PDEVICE_NODE DeviceNode)
{
    UNICODE_STRING EnumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
    UNICODE_STRING Control = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"AllocConfig");
    HANDLE EnumKey, InstanceKey, ControlKey;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Open the Enum key */
    Status = IopOpenRegistryKeyEx(&EnumKey, NULL, &EnumRoot, KEY_ENUMERATE_SUB_KEYS);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open the instance key (eg. Root\PNP0A03) */
    Status = IopOpenRegistryKeyEx(&InstanceKey, EnumKey, &DeviceNode->InstancePath, KEY_ENUMERATE_SUB_KEYS);
    ZwClose(EnumKey);

    if (!NT_SUCCESS(Status))
        return Status;

    /* Create/Open the Control key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &Control,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               InstanceKey,
                               NULL);
    Status = ZwCreateKey(&ControlKey,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ZwClose(InstanceKey);

    if (!NT_SUCCESS(Status))
        return Status;

    /* Write the resource list */
    Status = ZwSetValueKey(ControlKey,
                           &ValueName,
                           0,
                           REG_RESOURCE_LIST,
                           DeviceNode->ResourceList,
                           PnpDetermineResourceListSize(DeviceNode->ResourceList));
    ZwClose(ControlKey);

    if (!NT_SUCCESS(Status))
        return Status;

    return STATUS_SUCCESS;
}

static
NTSTATUS
IopFilterResourceRequirements(
    IN PDEVICE_NODE DeviceNode)
{
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    DPRINT("Sending IRP_MN_FILTER_RESOURCE_REQUIREMENTS to device stack\n");

    Stack.Parameters.FilterResourceRequirements.IoResourceRequirementList = DeviceNode->ResourceRequirements;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_FILTER_RESOURCE_REQUIREMENTS,
                               &Stack);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_SUPPORTED)
    {
        DPRINT1("IopInitiatePnpIrp(IRP_MN_FILTER_RESOURCE_REQUIREMENTS) failed\n");
        return Status;
    }
    else if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        DeviceNode->ResourceRequirements = (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
IopUpdateResourceMap(
    IN PDEVICE_NODE DeviceNode,
    PWCHAR Level1Key,
    PWCHAR Level2Key)
{
    NTSTATUS Status;
    ULONG Disposition;
    HANDLE PnpMgrLevel1, PnpMgrLevel2, ResourceMapKey;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE,
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

    if (DeviceNode->ResourceList)
    {
        UNICODE_STRING NameU;
        UNICODE_STRING RawSuffix, TranslatedSuffix;
        ULONG OldLength = 0;

        ASSERT(DeviceNode->ResourceListTranslated);

        RtlInitUnicodeString(&TranslatedSuffix, L".Translated");
        RtlInitUnicodeString(&RawSuffix, L".Raw");

        Status = IoGetDeviceProperty(DeviceNode->PhysicalDeviceObject,
                                     DevicePropertyPhysicalDeviceObjectName,
                                     0,
                                     NULL,
                                     &OldLength);
        if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            ASSERT(OldLength);

            NameU.Buffer = ExAllocatePool(PagedPool, OldLength + TranslatedSuffix.Length);
            if (!NameU.Buffer)
            {
                ZwClose(PnpMgrLevel2);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            NameU.Length = 0;
            NameU.MaximumLength = (USHORT)OldLength + TranslatedSuffix.Length;

            Status = IoGetDeviceProperty(DeviceNode->PhysicalDeviceObject,
                                         DevicePropertyPhysicalDeviceObjectName,
                                         NameU.MaximumLength,
                                         NameU.Buffer,
                                         &OldLength);
            if (!NT_SUCCESS(Status))
            {
                ZwClose(PnpMgrLevel2);
                ExFreePool(NameU.Buffer);
                return Status;
            }
        }
        else if (!NT_SUCCESS(Status))
        {
            /* Some failure */
            ZwClose(PnpMgrLevel2);
            return Status;
        }
        else
        {
            /* This should never happen */
            ASSERT(FALSE);
        }

        NameU.Length = (USHORT)OldLength - sizeof(UNICODE_NULL); /* Remove final NULL */

        RtlAppendUnicodeStringToString(&NameU, &RawSuffix);

        Status = ZwSetValueKey(PnpMgrLevel2,
                               &NameU,
                               0,
                               REG_RESOURCE_LIST,
                               DeviceNode->ResourceList,
                               PnpDetermineResourceListSize(DeviceNode->ResourceList));
        if (!NT_SUCCESS(Status))
        {
            ZwClose(PnpMgrLevel2);
            ExFreePool(NameU.Buffer);
            return Status;
        }

        /* "Remove" the suffix by setting the length back to what it used to be */
        NameU.Length = (USHORT)OldLength - sizeof(UNICODE_NULL); /* Remove final NULL */

        RtlAppendUnicodeStringToString(&NameU, &TranslatedSuffix);

        Status = ZwSetValueKey(PnpMgrLevel2,
                               &NameU,
                               0,
                               REG_RESOURCE_LIST,
                               DeviceNode->ResourceListTranslated,
                               PnpDetermineResourceListSize(DeviceNode->ResourceListTranslated));
        ZwClose(PnpMgrLevel2);
        ExFreePool(NameU.Buffer);

        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        ZwClose(PnpMgrLevel2);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopUpdateResourceMapForPnPDevice(
    IN PDEVICE_NODE DeviceNode)
{
    return IopUpdateResourceMap(DeviceNode, L"PnP Manager", L"PnpManager");
}

static
NTSTATUS
IopTranslateDeviceResources(
   IN PDEVICE_NODE DeviceNode)
{
   PCM_PARTIAL_RESOURCE_LIST pPartialResourceList;
   PCM_PARTIAL_RESOURCE_DESCRIPTOR DescriptorRaw, DescriptorTranslated;
   PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
   ULONG i, j, ListSize;
   NTSTATUS Status;

   if (!DeviceNode->ResourceList)
   {
      DeviceNode->ResourceListTranslated = NULL;
      return STATUS_SUCCESS;
   }

   /* That's easy to translate a resource list. Just copy the
    * untranslated one and change few fields in the copy
    */
   ListSize = PnpDetermineResourceListSize(DeviceNode->ResourceList);

   DeviceNode->ResourceListTranslated = ExAllocatePool(PagedPool, ListSize);
   if (!DeviceNode->ResourceListTranslated)
   {
      Status = STATUS_NO_MEMORY;
      goto cleanup;
   }
   RtlCopyMemory(DeviceNode->ResourceListTranslated, DeviceNode->ResourceList, ListSize);

   FullDescriptor = &DeviceNode->ResourceList->List[0];
   for (i = 0; i < DeviceNode->ResourceList->Count; i++)
   {
      pPartialResourceList = &FullDescriptor->PartialResourceList;
      FullDescriptor = CmiGetNextResourceDescriptor(FullDescriptor);

      for (j = 0; j < pPartialResourceList->Count; j++)
      {
        /* Partial resource descriptors can be of variable size (CmResourceTypeDeviceSpecific),
           but only one is allowed and it must be the last one in the list! */
         DescriptorRaw = &pPartialResourceList->PartialDescriptors[j];

         /* Calculate the location of the translated resource descriptor */
         DescriptorTranslated = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)(
             (PUCHAR)DeviceNode->ResourceListTranslated +
             ((PUCHAR)DescriptorRaw - (PUCHAR)DeviceNode->ResourceList));

         switch (DescriptorRaw->Type)
         {
            case CmResourceTypePort:
            {
               ULONG AddressSpace = 1; /* IO space */
               if (!HalTranslateBusAddress(
                  DeviceNode->ResourceList->List[i].InterfaceType,
                  DeviceNode->ResourceList->List[i].BusNumber,
                  DescriptorRaw->u.Port.Start,
                  &AddressSpace,
                  &DescriptorTranslated->u.Port.Start))
               {
                  Status = STATUS_UNSUCCESSFUL;
                  DPRINT1("Failed to translate port resource (Start: 0x%I64x)\n", DescriptorRaw->u.Port.Start.QuadPart);
                  goto cleanup;
               }

               if (AddressSpace == 0)
               {
                   DPRINT1("Guessed incorrect address space: 1 -> 0\n");

                   /* FIXME: I think all other CM_RESOURCE_PORT_XXX flags are
                    * invalid for this state but I'm not 100% sure */
                   DescriptorRaw->Flags =
                   DescriptorTranslated->Flags = CM_RESOURCE_PORT_MEMORY;
               }
               break;
            }
            case CmResourceTypeInterrupt:
            {
               KIRQL Irql;
               DescriptorTranslated->u.Interrupt.Vector = HalGetInterruptVector(
                  DeviceNode->ResourceList->List[i].InterfaceType,
                  DeviceNode->ResourceList->List[i].BusNumber,
                  DescriptorRaw->u.Interrupt.Level,
                  DescriptorRaw->u.Interrupt.Vector,
                  &Irql,
                  &DescriptorTranslated->u.Interrupt.Affinity);
               DescriptorTranslated->u.Interrupt.Level = Irql;
               if (!DescriptorTranslated->u.Interrupt.Vector)
               {
                   Status = STATUS_UNSUCCESSFUL;
                   DPRINT1("Failed to translate interrupt resource (Vector: 0x%x | Level: 0x%x)\n", DescriptorRaw->u.Interrupt.Vector,
                                                                                                   DescriptorRaw->u.Interrupt.Level);
                   goto cleanup;
               }
               break;
            }
            case CmResourceTypeMemory:
            {
               ULONG AddressSpace = 0; /* Memory space */
               if (!HalTranslateBusAddress(
                  DeviceNode->ResourceList->List[i].InterfaceType,
                  DeviceNode->ResourceList->List[i].BusNumber,
                  DescriptorRaw->u.Memory.Start,
                  &AddressSpace,
                  &DescriptorTranslated->u.Memory.Start))
               {
                  Status = STATUS_UNSUCCESSFUL;
                  DPRINT1("Failed to translate memory resource (Start: 0x%I64x)\n", DescriptorRaw->u.Memory.Start.QuadPart);
                  goto cleanup;
               }

               if (AddressSpace != 0)
               {
                   DPRINT1("Guessed incorrect address space: 0 -> 1\n");

                   /* This should never happen for memory space */
                   ASSERT(FALSE);
               }
            }

            case CmResourceTypeDma:
            case CmResourceTypeBusNumber:
            case CmResourceTypeDevicePrivate:
            case CmResourceTypeDeviceSpecific:
               /* Nothing to do */
               break;
            default:
               DPRINT1("Unknown resource descriptor type 0x%x\n", DescriptorRaw->Type);
               Status = STATUS_NOT_IMPLEMENTED;
               goto cleanup;
         }
      }
   }
   return STATUS_SUCCESS;

cleanup:
   /* Yes! Also delete ResourceList because ResourceList and
    * ResourceListTranslated should be a pair! */
   ExFreePool(DeviceNode->ResourceList);
   DeviceNode->ResourceList = NULL;
   if (DeviceNode->ResourceListTranslated)
   {
      ExFreePool(DeviceNode->ResourceListTranslated);
      DeviceNode->ResourceList = NULL;
   }
   return Status;
}

NTSTATUS
NTAPI
IopAssignDeviceResources(
   IN PDEVICE_NODE DeviceNode)
{
   NTSTATUS Status;
   ULONG ListSize;

   Status = IopFilterResourceRequirements(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   if (!DeviceNode->BootResources && !DeviceNode->ResourceRequirements)
   {
      /* No resource needed for this device */
      DeviceNode->ResourceList = NULL;
      DeviceNode->ResourceListTranslated = NULL;
      PiSetDevNodeState(DeviceNode, DeviceNodeResourcesAssigned);
      DeviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;

      return STATUS_SUCCESS;
   }

   if (DeviceNode->BootResources)
   {
       ListSize = PnpDetermineResourceListSize(DeviceNode->BootResources);

       DeviceNode->ResourceList = ExAllocatePool(PagedPool, ListSize);
       if (!DeviceNode->ResourceList)
       {
           Status = STATUS_NO_MEMORY;
           goto ByeBye;
       }

       RtlCopyMemory(DeviceNode->ResourceList, DeviceNode->BootResources, ListSize);

       Status = IopDetectResourceConflict(DeviceNode->ResourceList, FALSE, NULL);
       if (!NT_SUCCESS(Status))
       {
           DPRINT1("Boot resources for %wZ cause a resource conflict!\n", &DeviceNode->InstancePath);
           ExFreePool(DeviceNode->ResourceList);
           DeviceNode->ResourceList = NULL;
       }
   }
   else
   {
       /* We'll make this from the requirements */
       DeviceNode->ResourceList = NULL;
   }

   /* No resources requirements */
   if (!DeviceNode->ResourceRequirements)
       goto Finish;

   /* Call HAL to fixup our resource requirements list */
   HalAdjustResourceList(&DeviceNode->ResourceRequirements);

   /* Add resource requirements that aren't in the list we already got */
   Status = IopFixupResourceListWithRequirements(DeviceNode->ResourceRequirements,
                                                 &DeviceNode->ResourceList);
   if (!NT_SUCCESS(Status))
   {
       DPRINT1("Failed to fixup a resource list from supplied resources for %wZ\n", &DeviceNode->InstancePath);
       DeviceNode->Problem = CM_PROB_NORMAL_CONFLICT;
       goto ByeBye;
   }

   /* IopFixupResourceListWithRequirements should NEVER give us a conflicting list */
   ASSERT(IopDetectResourceConflict(DeviceNode->ResourceList, FALSE, NULL) != STATUS_CONFLICTING_ADDRESSES);

Finish:
   Status = IopTranslateDeviceResources(DeviceNode);
   if (!NT_SUCCESS(Status))
   {
       DeviceNode->Problem = CM_PROB_TRANSLATION_FAILED;
       DPRINT1("Failed to translate resources for %wZ\n", &DeviceNode->InstancePath);
       goto ByeBye;
   }

   Status = IopUpdateResourceMapForPnPDevice(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   Status = IopUpdateControlKeyWithResources(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   PiSetDevNodeState(DeviceNode, DeviceNodeResourcesAssigned);

   return STATUS_SUCCESS;

ByeBye:
   if (DeviceNode->ResourceList)
   {
      ExFreePool(DeviceNode->ResourceList);
      DeviceNode->ResourceList = NULL;
   }

   DeviceNode->ResourceListTranslated = NULL;

   return Status;
}

static
BOOLEAN
IopCheckForResourceConflict(
   IN PCM_RESOURCE_LIST ResourceList1,
   IN PCM_RESOURCE_LIST ResourceList2,
   IN BOOLEAN Silent,
   OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   ULONG i, ii;
   BOOLEAN Result = FALSE;
   PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;

   FullDescriptor = &ResourceList1->List[0];
   for (i = 0; i < ResourceList1->Count; i++)
   {
      PCM_PARTIAL_RESOURCE_LIST ResList = &FullDescriptor->PartialResourceList;
      FullDescriptor = CmiGetNextResourceDescriptor(FullDescriptor);

      for (ii = 0; ii < ResList->Count; ii++)
      {
        /* Partial resource descriptors can be of variable size (CmResourceTypeDeviceSpecific),
           but only one is allowed and it must be the last one in the list! */
         PCM_PARTIAL_RESOURCE_DESCRIPTOR ResDesc = &ResList->PartialDescriptors[ii];

         Result = IopCheckResourceDescriptor(ResDesc,
                                             ResourceList2,
                                             Silent,
                                             ConflictingDescriptor);
         if (Result) goto ByeBye;
      }
   }

ByeBye:

   return Result;
}

NTSTATUS NTAPI
IopDetectResourceConflict(
   IN PCM_RESOURCE_LIST ResourceList,
   IN BOOLEAN Silent,
   OUT OPTIONAL PCM_PARTIAL_RESOURCE_DESCRIPTOR ConflictingDescriptor)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName;
   HANDLE ResourceMapKey = NULL, ChildKey2 = NULL, ChildKey3 = NULL;
   ULONG KeyInformationLength, RequiredLength, KeyValueInformationLength, KeyNameInformationLength;
   PKEY_BASIC_INFORMATION KeyInformation;
   PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
   PKEY_VALUE_BASIC_INFORMATION KeyNameInformation;
   ULONG ChildKeyIndex1 = 0, ChildKeyIndex2, ChildKeyIndex3;
   NTSTATUS Status;

   RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP");
   InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              NULL,
                              NULL);
   Status = ZwOpenKey(&ResourceMapKey, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &ObjectAttributes);
   if (!NT_SUCCESS(Status))
   {
      /* The key is missing which means we are the first device */
      return STATUS_SUCCESS;
   }

   while (TRUE)
   {
      Status = ZwEnumerateKey(ResourceMapKey,
                              ChildKeyIndex1,
                              KeyBasicInformation,
                              NULL,
                              0,
                              &RequiredLength);
      if (Status == STATUS_NO_MORE_ENTRIES)
          break;
      else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
      {
          KeyInformationLength = RequiredLength;
          KeyInformation = ExAllocatePoolWithTag(PagedPool,
                                                 KeyInformationLength,
                                                 TAG_IO);
          if (!KeyInformation)
          {
              Status = STATUS_INSUFFICIENT_RESOURCES;
              goto cleanup;
          }

          Status = ZwEnumerateKey(ResourceMapKey,
                                  ChildKeyIndex1,
                                  KeyBasicInformation,
                                  KeyInformation,
                                  KeyInformationLength,
                                  &RequiredLength);
      }
      else
         goto cleanup;
      ChildKeyIndex1++;
      if (!NT_SUCCESS(Status))
      {
          ExFreePoolWithTag(KeyInformation, TAG_IO);
          goto cleanup;
      }

      KeyName.Buffer = KeyInformation->Name;
      KeyName.MaximumLength = KeyName.Length = (USHORT)KeyInformation->NameLength;
      InitializeObjectAttributes(&ObjectAttributes,
                                 &KeyName,
                                 OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                 ResourceMapKey,
                                 NULL);
      Status = ZwOpenKey(&ChildKey2,
                         KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                         &ObjectAttributes);
      ExFreePoolWithTag(KeyInformation, TAG_IO);
      if (!NT_SUCCESS(Status))
          goto cleanup;

      ChildKeyIndex2 = 0;
      while (TRUE)
      {
          Status = ZwEnumerateKey(ChildKey2,
                                  ChildKeyIndex2,
                                  KeyBasicInformation,
                                  NULL,
                                  0,
                                  &RequiredLength);
          if (Status == STATUS_NO_MORE_ENTRIES)
              break;
          else if (Status == STATUS_BUFFER_TOO_SMALL)
          {
              KeyInformationLength = RequiredLength;
              KeyInformation = ExAllocatePoolWithTag(PagedPool,
                                                     KeyInformationLength,
                                                     TAG_IO);
              if (!KeyInformation)
              {
                  Status = STATUS_INSUFFICIENT_RESOURCES;
                  goto cleanup;
              }

              Status = ZwEnumerateKey(ChildKey2,
                                      ChildKeyIndex2,
                                      KeyBasicInformation,
                                      KeyInformation,
                                      KeyInformationLength,
                                      &RequiredLength);
          }
          else
              goto cleanup;
          ChildKeyIndex2++;
          if (!NT_SUCCESS(Status))
          {
              ExFreePoolWithTag(KeyInformation, TAG_IO);
              goto cleanup;
          }

          KeyName.Buffer = KeyInformation->Name;
          KeyName.MaximumLength = KeyName.Length = (USHORT)KeyInformation->NameLength;
          InitializeObjectAttributes(&ObjectAttributes,
                                     &KeyName,
                                     OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                     ChildKey2,
                                     NULL);
          Status = ZwOpenKey(&ChildKey3, KEY_QUERY_VALUE, &ObjectAttributes);
          ExFreePoolWithTag(KeyInformation, TAG_IO);
          if (!NT_SUCCESS(Status))
              goto cleanup;

          ChildKeyIndex3 = 0;
          while (TRUE)
          {
              Status = ZwEnumerateValueKey(ChildKey3,
                                           ChildKeyIndex3,
                                           KeyValuePartialInformation,
                                           NULL,
                                           0,
                                           &RequiredLength);
              if (Status == STATUS_NO_MORE_ENTRIES)
                  break;
              else if (Status == STATUS_BUFFER_TOO_SMALL)
              {
                  KeyValueInformationLength = RequiredLength;
                  KeyValueInformation = ExAllocatePoolWithTag(PagedPool,
                                                              KeyValueInformationLength,
                                                              TAG_IO);
                  if (!KeyValueInformation)
                  {
                      Status = STATUS_INSUFFICIENT_RESOURCES;
                      goto cleanup;
                  }

                  Status = ZwEnumerateValueKey(ChildKey3,
                                               ChildKeyIndex3,
                                               KeyValuePartialInformation,
                                               KeyValueInformation,
                                               KeyValueInformationLength,
                                               &RequiredLength);
              }
              else
                  goto cleanup;
              if (!NT_SUCCESS(Status))
              {
                  ExFreePoolWithTag(KeyValueInformation, TAG_IO);
                  goto cleanup;
              }

              Status = ZwEnumerateValueKey(ChildKey3,
                                           ChildKeyIndex3,
                                           KeyValueBasicInformation,
                                           NULL,
                                           0,
                                           &RequiredLength);
              if (Status == STATUS_BUFFER_TOO_SMALL)
              {
                  KeyNameInformationLength = RequiredLength;
                  KeyNameInformation = ExAllocatePoolWithTag(PagedPool,
                                                             KeyNameInformationLength + sizeof(WCHAR),
                                                             TAG_IO);
                  if (!KeyNameInformation)
                  {
                      Status = STATUS_INSUFFICIENT_RESOURCES;
                      goto cleanup;
                  }

                  Status = ZwEnumerateValueKey(ChildKey3,
                                               ChildKeyIndex3,
                                               KeyValueBasicInformation,
                                               KeyNameInformation,
                                               KeyNameInformationLength,
                                               &RequiredLength);
              }
              else
                  goto cleanup;
              ChildKeyIndex3++;
              if (!NT_SUCCESS(Status))
              {
                  ExFreePoolWithTag(KeyNameInformation, TAG_IO);
                  goto cleanup;
              }

              KeyNameInformation->Name[KeyNameInformation->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

              /* Skip translated entries */
              if (wcsstr(KeyNameInformation->Name, L".Translated"))
              {
                  ExFreePoolWithTag(KeyNameInformation, TAG_IO);
                  ExFreePoolWithTag(KeyValueInformation, TAG_IO);
                  continue;
              }

              ExFreePoolWithTag(KeyNameInformation, TAG_IO);

              if (IopCheckForResourceConflict(ResourceList,
                                              (PCM_RESOURCE_LIST)KeyValueInformation->Data,
                                              Silent,
                                              ConflictingDescriptor))
              {
                  ExFreePoolWithTag(KeyValueInformation, TAG_IO);
                  Status = STATUS_CONFLICTING_ADDRESSES;
                  goto cleanup;
              }

              ExFreePoolWithTag(KeyValueInformation, TAG_IO);
          }
      }
   }

cleanup:
   if (ResourceMapKey != NULL)
       ObCloseHandle(ResourceMapKey, KernelMode);
   if (ChildKey2 != NULL)
       ObCloseHandle(ChildKey2, KernelMode);
   if (ChildKey3 != NULL)
       ObCloseHandle(ChildKey3, KernelMode);

   if (Status == STATUS_NO_MORE_ENTRIES)
       Status = STATUS_SUCCESS;

   return Status;
}

