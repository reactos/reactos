/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP manager Root Memory Arbiter
 * COPYRIGHT:   Copyright 2025-2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootMemArbiter;

ULONGLONG
NTAPI
RtlIoDecodeMemIoResource(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _Out_opt_ PULONGLONG Alignment,
    _Out_opt_ PULONGLONG MinimumAddress,
    _Out_opt_ PULONGLONG MaximumAddress);

ULONGLONG
NTAPI
RtlCmDecodeMemIoResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_opt_ PULONGLONG Start);

/* The last address a card that only wires up 24 address lines can reach. */
#define MEMORY_24_BIT_MAX_ADDRESS   0xFFFFFF

/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * Translates one bus-relative address to a system-physical one on
 * the ISA bus, and reports the space it landed in as a CM resource
 * type.
 *
 * @param[in] SourceType
 * The CM resource type of the address being translated.
 *
 * @param[in] SourceAddress
 * The bus-relative address.
 *
 * @param[out] TranslatedAddress
 * Receives the system-physical address.
 *
 * @param[out] TranslatedType
 * Receives the post-translation resource type. A HAL may map one
 * space onto another, so the type is taken from the space the
 * translation actually returned rather than assumed, e.g IOPorts, MMIO, etc.
 *
 * @return
 * Returns STATUS_SUCCESS, STATUS_INVALID_PARAMETER for a resource
 * type that carries no address or for an address space the HAL is
 * not expected to report, or STATUS_UNSUCCESSFUL when the HAL
 * declines the translation.
 */
static
NTSTATUS
IopArbMemTranslateAddress(
    _In_ UCHAR SourceType,
    _In_ PHYSICAL_ADDRESS SourceAddress,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress,
    _Out_ PUCHAR TranslatedType)
{
    ULONG AddressSpace;

    PAGED_CODE();

    if (SourceType == CmResourceTypeMemory || SourceType == CmResourceTypeMemoryLarge)
        AddressSpace = 0; /* System memory */
    else if (SourceType == CmResourceTypePort)
        AddressSpace = 1; /* I/O port space */
    else
        return STATUS_INVALID_PARAMETER;

    if (!HalTranslateBusAddress(Isa, 0, SourceAddress, &AddressSpace, TranslatedAddress))
        return STATUS_UNSUCCESSFUL;

    /* The HAL reports back the space it landed in; only these two are expected. */
    if (AddressSpace == 1)
    {
        *TranslatedType = CmResourceTypePort;
    }
    else if (AddressSpace == 0)
    {
        *TranslatedType = (SourceType == CmResourceTypeMemoryLarge)
                          ? CmResourceTypeMemoryLarge
                          : CmResourceTypeMemory;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Extracts the placement window from one memory requirement.
 * The UnpackRequirement callback of the Root Memory arbiter.
 *
 * @param[in] IoDescriptor
 * The requirement to decode.
 *
 * @param[out] OutMinimumAddress
 * Receives the lowest physical address the requirement accepts.
 *
 * @param[out] OutMaximumAddress
 * Receives the highest physical address the requirement accepts.
 * A CM_RESOURCE_MEMORY_24 card only wires up 24 address lines and
 * cannot be reached above 16 MB whatever its descriptor claims, so
 * its window is clamped here rather than left for the placement
 * search to get wrong.
 *
 * @param[out] OutLength
 * Receives the size of the window wanted.
 *
 * @param[out] OutAlignment
 * Receives the requirement's alignment; a zero alignment means the
 * device does not care and is normalised to byte alignment, since
 * the placement arithmetic divides by it.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
static
NTSTATUS
NTAPI
IopArbMemUnpackRequirements(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _Out_ PUINT64 OutMinimumAddress,
    _Out_ PUINT64 OutMaximumAddress,
    _Out_ PUINT64 OutLength,
    _Out_ PUINT64 OutAlignment)
{
    PAGED_CODE();

    *OutLength = RtlIoDecodeMemIoResource(IoDescriptor,
                                          OutAlignment,
                                          OutMinimumAddress,
                                          OutMaximumAddress);

    if (*OutAlignment == 0)
        *OutAlignment = 1;

    if (IoDescriptor->Type == CmResourceTypeMemory &&
        (IoDescriptor->Flags & CM_RESOURCE_MEMORY_24) &&
        *OutMaximumAddress > MEMORY_24_BIT_MAX_ADDRESS)
    {
        *OutMaximumAddress = MEMORY_24_BIT_MAX_ADDRESS;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Materialises the arbiter's chosen placement as an assigned CM
 * descriptor. The PackResource callback of the Root Memory arbiter.
 *
 * @param[in] IoDescriptor
 * The requirement the placement satisfies. Type, Flags and
 * ShareDisposition come across unchanged - and have to, since for
 * a large-memory descriptor the Flags are what say how the Length
 * beside them is encoded.
 *
 * @param[in] Start
 * The physical address the arbiter settled on.
 *
 * @param[out] CmDescriptor
 * Receives the assigned descriptor.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
static
NTSTATUS
NTAPI
IopArbMemPackResource(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor,
    _In_ UINT64 Start,
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PAGED_CODE();

    CmDescriptor->Type = IoDescriptor->Type;
    CmDescriptor->Flags = IoDescriptor->Flags;
    CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;

    CmDescriptor->u.Generic.Start.QuadPart = Start;
    CmDescriptor->u.Generic.Length = IoDescriptor->u.Generic.Length;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Reads the placement back out of an already-assigned descriptor
 * (a firmware boot configuration, typically) so the arbiter can
 * mark that span occupied. The inverse of IopArbMemPackResource
 * and the UnpackResource callback of the Root Memory arbiter.
 *
 * @param[in] CmDescriptor
 * The assigned descriptor to decode.
 *
 * @param[out] Start
 * Receives the assigned physical address.
 *
 * @param[out] OutLength
 * Receives the assigned length.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
static
NTSTATUS
NTAPI
IopArbMemUnpackResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor,
    _Out_ PUINT64 Start,
    _Out_ PUINT64 OutLength)
{
    PAGED_CODE();

    *OutLength = RtlCmDecodeMemIoResource(CmDescriptor, Start);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Scores how constrained a requirement is: the number of distinct
 * aligned addresses it could be placed at inside its own window,
 * ignoring what is already allocated. The ScoreRequirement
 * callback of the Root Memory arbiter.
 *
 * @param[in] IoDescriptor
 * The requirement to score.
 *
 * @return
 * Returns the placement count, saturated to MAXLONG, or -1 for a
 * window that cannot hold the requirement at all - which the
 * engine treats as a bad configuration and fails the arbitration
 * on.
 *
 * @remarks
 * The engine places the most constrained device (the lowest score)
 * first, PCI devices have a habit of absorbing a remaining range.
 */
static
INT32
NTAPI
IopArbMemScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UINT64 Length, Alignment;
    UINT64 Minimum, Maximum, AlignedMinimum;
    UINT64 Span, Placements;

    PAGED_CODE();

    Length = RtlIoDecodeMemIoResource(IoDescriptor, &Alignment, &Minimum, &Maximum);

    if (Alignment == 0)
        Alignment = 1;

    /* Round the window's base up to the first address the device can sit at. */
    AlignedMinimum = (Minimum + Alignment - 1) & ~(Alignment - 1);
    if (AlignedMinimum < Minimum || AlignedMinimum > Maximum)
        return -1;

    /* Count in addresses above the base, so a full-width window cannot overflow. */
    Span = Maximum - AlignedMinimum;

    if (Length != 0)
    {
        if (Length - 1 > Span)
            return -1;

        Span -= Length - 1;
    }

    Placements = Span / Alignment + 1;
    return (INT32)min(Placements, MAXLONG);
}

/**
 * @brief
 * Translates one registry allocation-ordering window from the
 * bus-relative addresses it is written in into the system-physical
 * space the arbiter allocates in. The TranslateOrdering function
 * of the Root Memory arbiter.
 *
 * @param[out] OutIoDescriptor
 * Receives the translated copy. An entry whose endpoints cannot
 * both be translated is marked CmResourceTypeNull for the ordering
 * reader to drop; otherwise it takes the translated type.
 *
 * @param[in] IoDescriptor
 * The ordering window. A descriptor that carries no address passes
 * through untouched.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
static
NTSTATUS
NTAPI
IopArbMemTranslateOrdering(
    _Out_ PIO_RESOURCE_DESCRIPTOR OutIoDescriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UCHAR SourceType;
    UCHAR MinimumType;
    UCHAR MaximumType;

    PAGED_CODE();

    *OutIoDescriptor = *IoDescriptor;

    SourceType = IoDescriptor->Type;
    if (SourceType != CmResourceTypePort &&
        SourceType != CmResourceTypeMemory &&
        SourceType != CmResourceTypeMemoryLarge)
    {
        return STATUS_SUCCESS;
    }

    MinimumType = SourceType;
    MaximumType = SourceType;

    if (!NT_SUCCESS(IopArbMemTranslateAddress(SourceType,
                                              IoDescriptor->u.Generic.MinimumAddress,
                                              &OutIoDescriptor->u.Generic.MinimumAddress,
                                              &MinimumType)) ||
        !NT_SUCCESS(IopArbMemTranslateAddress(SourceType,
                                              IoDescriptor->u.Generic.MaximumAddress,
                                              &OutIoDescriptor->u.Generic.MaximumAddress,
                                              &MaximumType)))
    {
        OutIoDescriptor->Type = CmResourceTypeNull;
    }
    else
    {
        OutIoDescriptor->Type = MaximumType;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * The Root Memory arbiter's FindSuitableRange: the engine search,
 * incremented for a device asking to keep the MMIO window the firmware
 * already programmed it into.
 *
 * @param[in] Arbiter
 * The Root Memory arbiter instance.
 *
 * @param[in,out] ArbState
 * The allocation state of the requirement being placed.
 *
 * @return
 * Returns TRUE if a placement was found, FALSE otherwise.
 *
 * @remarks
 * A boot configuration is held in the range list as an
 * ARBITER_RANGE_BOOT_ALLOCATED range and so reads as occupied. A
 * request carrying ARBITER_FLAG_BOOT_CONFIG is precisely the one
 * asking for that space back, so those ranges are made available
 * to it. The engine already extends the same courtesy to legacy
 * request sources.
 */
static
BOOLEAN
NTAPI
IopArbMemFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    if (ArbState->Entry != NULL && (ArbState->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG))
        ArbState->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;

    return ArbiterLibFindSuitableRange(Arbiter, ArbState);
}

/* Hardware IDs of PCI and PCI Express host bridges */
static const PCWSTR IopArbMemHostBridgeIds[] =
{
    L"ACPI\\PNP0A03",
    L"ACPI\\PNP0A08",
};

/* Checks one ID of a hardware ID list against the host bridge IDs */
static
BOOLEAN
NTAPI
IopArbMemIsHostBridgeId(
    _In_ PCWSTR Id)
{
    ULONG Index;

    for (Index = 0; Index < RTL_NUMBER_OF(IopArbMemHostBridgeIds); Index++)
    {
        if (_wcsicmp(Id, IopArbMemHostBridgeIds[Index]) == 0)
            return TRUE;
    }

    return FALSE;
}

/**
 * @brief
 * Checks if a device is a PCI host bridge from the HardwareID value of its
 * instance key. A list that is not terminated is only read up to its end.
 */
static
BOOLEAN
NTAPI
IopArbMemIsHostBridge(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    UNICODE_STRING EnumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    PKEY_VALUE_FULL_INFORMATION Value;
    BOOLEAN IsHostBridge = FALSE;
    HANDLE EnumKey;
    HANDLE InstanceKey;
    NTSTATUS Status;

    PAGED_CODE();

    if (DeviceNode == NULL || DeviceNode->InstancePath.Length == 0)
        return FALSE;

    Status = IopOpenRegistryKeyEx(&EnumKey, NULL, &EnumRoot, KEY_ENUMERATE_SUB_KEYS);
    if (!NT_SUCCESS(Status))
        return FALSE;

    Status = IopOpenRegistryKeyEx(&InstanceKey,
                                  EnumKey,
                                  &DeviceNode->InstancePath,
                                  KEY_QUERY_VALUE);
    ZwClose(EnumKey);
    if (!NT_SUCCESS(Status))
        return FALSE;

    Status = IopGetRegistryValue(InstanceKey, L"HardwareID", &Value);
    ZwClose(InstanceKey);
    if (!NT_SUCCESS(Status))
        return FALSE;

    if (Value->Type == REG_MULTI_SZ)
    {
        PCWSTR Id = (PCWSTR)((PUCHAR)Value + Value->DataOffset);
        PCWSTR End = Id + Value->DataLength / sizeof(WCHAR);

        while (Id < End && *Id != UNICODE_NULL && !IsHostBridge)
        {
            PCWSTR Next = Id;

            while (Next < End && *Next != UNICODE_NULL)
                Next++;

            if (Next == End)
                break;

            IsHostBridge = IopArbMemIsHostBridgeId(Id);
            Id = Next + 1;
        }
    }

    ExFreePool(Value);
    return IsHostBridge;
}

/**
 * @brief
 * Removes the conflicts inside the PCI configuration space window from the
 * conflicts of a host bridge. Its memory window contains the configuration
 * space window, which is reserved at initialization.
 */
static
VOID
NTAPI
IopArbMemRemoveMmConfigConflicts(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Inout_ PULONG ConflictCount,
    _Inout_updates_opt_(*ConflictCount) PARBITER_CONFLICT_INFO Conflicts)
{
    ULONG Count = *ConflictCount;
    ULONG Index;

    PAGED_CODE();

    if (Conflicts == NULL || Count == 0)
        return;

    if (!IopArbMemIsHostBridge(PhysicalDeviceObject))
        return;

    for (Index = Count; Index > 0; Index--)
    {
        PARBITER_CONFLICT_INFO Conflict = &Conflicts[Index - 1];

        if (ArbiterLibIsConflictWithMmConfigRange(Conflict->Start, Conflict->End))
            *Conflict = Conflicts[--Count];
    }

    *ConflictCount = Count;
}

/* The QueryConflict callback of the Root Memory arbiter */
#if (NTDDI_VERSION >= NTDDI_VISTA)
static
NTSTATUS
NTAPI
IopArbMemQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_QUERY_CONFLICT_PARAMETERS Parameters)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = ArbiterLibQueryConflict(Arbiter, Parameters);
    if (!NT_SUCCESS(Status))
        return Status;

    IopArbMemRemoveMmConfigConflicts(Parameters->PhysicalDeviceObject,
                                     Parameters->ConflictCount,
                                     *Parameters->Conflicts);

    return STATUS_SUCCESS;
}
#else
static
NTSTATUS
NTAPI
IopArbMemQueryConflict(
    _In_ PARBITER_INSTANCE Arbiter,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    _Out_ PULONG ConflictCount,
    _Out_ PARBITER_CONFLICT_INFO *Conflicts)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = ArbiterLibQueryConflict(Arbiter,
                                     PhysicalDeviceObject,
                                     ConflictingResource,
                                     ConflictCount,
                                     Conflicts);
    if (!NT_SUCCESS(Status))
        return Status;

    IopArbMemRemoveMmConfigConflicts(PhysicalDeviceObject, ConflictCount, *Conflicts);

    return STATUS_SUCCESS;
}
#endif

/* Returns the width of the physical addresses the kernel can map */
static
ULONG
NTAPI
IopArbMemPhysicalAddressBits(VOID)
{
#if defined(_M_IX86)
    /* The x86 kernel has no PAE, so nothing above 4 GB can be mapped */
    return 32;
#elif defined(_M_AMD64)
    CPU_INFO CpuInfo;

    KiCpuId(&CpuInfo, 0x80000000);
    if (CpuInfo.Eax < 0x80000008)
        return 36;

    KiCpuId(&CpuInfo, 0x80000008);
    return CpuInfo.Eax & 0xFF;
#else
    return 64;
#endif
}

/**
 * @brief
 * Records the physical addresses above the reach of the kernel as
 * Arbiters\InaccessibleRange\PhysicalAddress, for the memory arbiter to keep.
 */
static
VOID
NTAPI
IopArbMemPublishInaccessibleRange(VOID)
{
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Arbiters\\InaccessibleRange");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"PhysicalAddress");
    IO_RESOURCE_REQUIREMENTS_LIST Requirements;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Bits = IopArbMemPhysicalAddressBits();
    NTSTATUS Status;
    HANDLE Key;

    if (Bits == 0 || Bits >= 64)
        return;

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwCreateKey(&Key,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the inaccessible range key (Status 0x%08lx)\n", Status);
        return;
    }

    RtlZeroMemory(&Requirements, sizeof(Requirements));
    Requirements.ListSize = sizeof(Requirements);
    Requirements.AlternativeLists = 1;
    Requirements.List[0].Count = 1;
    Requirements.List[0].Descriptors[0].Type = CmResourceTypeMemory;
    Requirements.List[0].Descriptors[0].u.Memory.MinimumAddress.QuadPart = 1LL << Bits;
    Requirements.List[0].Descriptors[0].u.Memory.MaximumAddress.QuadPart = -1LL;

    Status = ZwSetValueKey(Key, &ValueName, 0, REG_RESOURCE_REQUIREMENTS_LIST,
                           &Requirements, sizeof(Requirements));
    if (!NT_SUCCESS(Status))
        DPRINT1("Failed to record the inaccessible range (Status 0x%08lx)\n", Status);

    ZwClose(Key);
}

/**
 * @brief Initialize the RootMemoryArbiter
 *
 * The root memory arbiter owns CPU physical address space and hands each
 * device the MMIO window it decodes. It is the fallback for every device whose
 * memory no closer arbiter claims.
 *
 * The physical page 0 is reserved with no attributes at all, so that no
 * availability mask can hand it out: the real-mode interrupt vector table and
 * the BIOS data area live there, and a device decoding over them corrupts the
 * machine rather than merely failing.
 *
 * The PCI configuration space window is reserved as a boot allocated range,
 * and the addresses above the reach of the processor are never handed out.
 *
 * @return NTSTATUS
 * @retval STATUS_SUCCESS
 * @retval STATUS_UNSUCCESSFUL
 * @retval STATUS_INSUFFICIENT_RESOURCES
 */
NTSTATUS
NTAPI
IopArbMemInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    IopArbMemPublishInaccessibleRange();

    IopRootMemArbiter.Name = L"RootMemory";
    IopRootMemArbiter.UnpackRequirement = IopArbMemUnpackRequirements;
    IopRootMemArbiter.PackResource = IopArbMemPackResource;
    IopRootMemArbiter.UnpackResource = IopArbMemUnpackResource;
    IopRootMemArbiter.ScoreRequirement = IopArbMemScoreRequirement;
    IopRootMemArbiter.FindSuitableRange = IopArbMemFindSuitableRange;
    IopRootMemArbiter.QueryConflict = IopArbMemQueryConflict;

    Status = ArbiterLibInitializeInstance(&IopRootMemArbiter,
                                          NULL,
                                          CmResourceTypeMemory,
                                          IopRootMemArbiter.Name,
                                          L"Root",
                                          IopArbMemTranslateOrdering);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbMemInitialize: Failed with %X\n", Status);
        return Status;
    }

    Status = RtlAddRange(IopRootMemArbiter.Allocation, 0ULL, 0xFFFULL, 0, 0, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbMemInitialize: Reserving page 0 failed with %X\n", Status);
        return Status;
    }

    Status = ArbiterLibAddInaccessibleAllocationRange(&IopRootMemArbiter,
                                                      L"PhysicalAddress",
                                                      IopRootMemArbiter.Allocation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbMemInitialize: Reserving the inaccessible range failed with %X\n", Status);
        return Status;
    }

    Status = ArbiterLibAddMmConfigRangeAsBootReserved(&IopRootMemArbiter,
                                                     IopRootMemArbiter.Allocation);
    if (!NT_SUCCESS(Status))
        DPRINT1("IopArbMemInitialize: Reserving MmConfigRange failed with %X\n", Status);

    return Status;
}
