/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP manager Root Port Arbiter
 * COPYRIGHT:   Copyright 2025-2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ARBITER_INSTANCE IopRootPortArbiter;

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

/*
 * A card that decodes only the low 10 or 12 address lines answers not just on
 * its own range but on every "alias" a multiple of 0x400 / 0x1000 above it, up
 * to the top of the 64 KB port space.  The arbiter has to reserve those aliases
 * as well, or another device is handed a port this card silently shadows.
 */
#define PORT_ALIAS_STRIDE_10_BIT    0x400
#define PORT_ALIAS_STRIDE_12_BIT    0x1000
#define PORT_MAX_ADDRESS            0xFFFF

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
 * space onto another - memory-mapped I/O ports being the usual
 * case - so the type is taken from the space the translation
 * actually returned rather than assumed.
 *
 * @return
 * Returns STATUS_SUCCESS, STATUS_INVALID_PARAMETER for a resource
 * type that carries no address, or STATUS_UNSUCCESSFUL when the
 * HAL declines the translation.
 */
static
NTSTATUS
IopArbPortTranslateAddress(
    _In_ UCHAR SourceType,
    _In_ PHYSICAL_ADDRESS SourceAddress,
    _Out_ PPHYSICAL_ADDRESS TranslatedAddress,
    _Out_ PUCHAR TranslatedType)
{
    ULONG AddressSpace;

    PAGED_CODE();

    if (SourceType == CmResourceTypePort)
        AddressSpace = 1; /* I/O port space */
    else if (SourceType == CmResourceTypeMemory || SourceType == CmResourceTypeMemoryLarge)
        AddressSpace = 0; /* system memory */
    else
        return STATUS_INVALID_PARAMETER;

    if (!HalTranslateBusAddress(Isa, 0, SourceAddress, &AddressSpace, TranslatedAddress))
        return STATUS_UNSUCCESSFUL;

    if (AddressSpace == 1 || AddressSpace == 3)
    {
        *TranslatedType = CmResourceTypePort;
    }
    else
    {
        *TranslatedType = (SourceType == CmResourceTypeMemoryLarge)
                          ? CmResourceTypeMemoryLarge
                          : CmResourceTypeMemory;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Walks the decode-alias chain of a port range: given the previous
 * alias - or the granted range's start on the first call -
 * produces the next one.
 *
 * @param[in] DescriptorFlags
 * The requirement's flags. Only 10-bit and 12-bit decode
 * requirements alias; a full 16-bit decoder owns exactly what it
 * asked for.
 *
 * @param[in] LastAlias
 * The previous alias start, or the granted range's start.
 *
 * @param[out] NextAlias
 * Receives the next alias start.
 *
 * @return
 * Returns TRUE with the next alias, or FALSE once the card decodes
 * fully or the next alias would leave I/O space.
 */
static
BOOLEAN
IopArbPortGetNextAlias(
    _In_ USHORT DescriptorFlags,
    _In_ UINT64 LastAlias,
    _Out_ PUINT64 NextAlias)
{
    UINT64 Next;

    PAGED_CODE();

    if (DescriptorFlags & CM_RESOURCE_PORT_10_BIT_DECODE)
        Next = LastAlias + PORT_ALIAS_STRIDE_10_BIT;
    else if (DescriptorFlags & CM_RESOURCE_PORT_12_BIT_DECODE)
        Next = LastAlias + PORT_ALIAS_STRIDE_12_BIT;
    else
        return FALSE;

    if (Next > PORT_MAX_ADDRESS)
        return FALSE;

    *NextAlias = Next;
    return TRUE;
}

/**
 * @brief
 * Extracts the placement window from one I/O port requirement.
 * The UnpackRequirement callback of the Root Port arbiter.
 *
 * @param[in] IoDescriptor
 * The requirement to decode.
 *
 * @param[out] OutMinimumAddress
 * Receives the lowest port address the requirement accepts.
 *
 * @param[out] OutMaximumAddress
 * Receives the highest port address the requirement accepts.
 *
 * @param[out] OutLength
 * Receives the number of consecutive ports wanted.
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
IopArbPortUnpackRequirements(
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

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Materialises the arbiter's chosen placement as an assigned CM
 * descriptor. The PackResource callback of the Root Port arbiter.
 *
 * @param[in] IoDescriptor
 * The requirement the placement satisfies. Type, Flags and
 * ShareDisposition come across unchanged, and so does the Length,
 * still in the encoding its Flags describe.
 *
 * @param[in] Start
 * The port address the arbiter settled on.
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
IopArbPortPackResource(
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
 * Reads the placement back out of an already-assigned descriptor -
 * a firmware boot configuration, typically - so the arbiter can
 * mark that span occupied. The inverse of IopArbPortPackResource
 * and the UnpackResource callback of the Root Port arbiter.
 *
 * @param[in] CmDescriptor
 * The assigned descriptor to decode.
 *
 * @param[out] Start
 * Receives the assigned port address.
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
IopArbPortUnpackResource(
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
 * callback of the Root Port arbiter.
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
 * first, so the devices with real freedom of movement are left to
 * absorb whatever space is still going.
 */
static
INT32
NTAPI
IopArbPortScoreRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UINT64 Length;
    UINT64 Alignment;
    UINT64 Minimum;
    UINT64 Maximum;
    UINT64 AlignedMinimum;
    UINT64 Span;
    UINT64 Placements;

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
    if (Placements > MAXLONG)
        return MAXLONG;

    return (INT32)Placements;
}

/**
 * @brief
 * Translates one registry allocation-ordering window from the
 * bus-relative addresses it is written in into the system-physical
 * space the arbiter allocates in. The TranslateOrdering function
 * of the Root Port arbiter.
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
IopArbPortTranslateOrdering(
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

    if (!NT_SUCCESS(IopArbPortTranslateAddress(SourceType,
                                               IoDescriptor->u.Generic.MinimumAddress,
                                               &OutIoDescriptor->u.Generic.MinimumAddress,
                                               &MinimumType)) ||
        !NT_SUCCESS(IopArbPortTranslateAddress(SourceType,
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
 * The Root Port arbiter's FindSuitableRange: the engine search,
 * widened for a device asking to keep the ports the firmware
 * already programmed it into.
 *
 * @param[in] Arbiter
 * The Root Port arbiter instance.
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
IopArbPortFindSuitableRange(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PAGED_CODE();

    if (ArbState->Entry != NULL && (ArbState->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG))
        ArbState->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;

    return ArbiterLibFindSuitableRange(Arbiter, ArbState);
}

/**
 * @brief
 * The Root Port arbiter's AddAllocation: records the granted range
 * in the tentative allocation, and behind it every port range a
 * partially-decoding ISA card would shadow.
 *
 * @param[in] Arbiter
 * The Root Port arbiter instance.
 *
 * @param[in,out] ArbState
 * The allocation state carrying the granted Start and End. Alias
 * ranges span the same width, belong to the same device, and are
 * tagged ARBITER_RANGE_PORT_ALIAS.
 */
static
VOID
NTAPI
IopArbPortAddAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PARBITER_ALTERNATIVE Alternative = ArbState->CurrentAlternative;
    PVOID Owner = ArbState->Entry ? ArbState->Entry->PhysicalDeviceObject : NULL;
    ULONG Flags = RTL_RANGE_LIST_ADD_IF_CONFLICT;
    UINT64 Alias;
    UINT64 Span;

    PAGED_CODE();

    if (Alternative != NULL && (Alternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED))
        Flags |= RTL_RANGE_LIST_ADD_SHARED;

    RtlAddRange(Arbiter->PossibleAllocation,
                ArbState->Start,
                ArbState->End,
                ArbState->RangeAttributes,
                Flags,
                NULL,
                Owner);

    if (Alternative == NULL)
        return;

    /* Alias the width actually granted, not the width asked for. */
    Span = ArbState->End - ArbState->Start;

    Alias = ArbState->Start;
    while (IopArbPortGetNextAlias(Alternative->Descriptor->Flags, Alias, &Alias))
    {
        RtlAddRange(Arbiter->PossibleAllocation,
                    Alias,
                    Alias + Span,
                    ArbState->RangeAttributes | ARBITER_RANGE_PORT_ALIAS,
                    Flags,
                    NULL,
                    Owner);
    }
}

/**
 * @brief
 * The Root Port arbiter's BacktrackAllocation: the exact inverse
 * of IopArbPortAddAllocation.
 *
 * @param[in] Arbiter
 * The Root Port arbiter instance.
 *
 * @param[in,out] ArbState
 * The allocation state whose placement is being withdrawn.
 *
 * @remarks
 * The alias chain is regenerated and removed along with the
 * granted range. Leaving the aliases behind would quietly wall off
 * the port space for every placement the engine tries afterwards.
 */
static
VOID
NTAPI
IopArbPortBacktrackAllocation(
    _In_ PARBITER_INSTANCE Arbiter,
    _Inout_ PARBITER_ALLOCATION_STATE ArbState)
{
    PARBITER_ALTERNATIVE Alternative = ArbState->CurrentAlternative;
    PVOID Owner = ArbState->Entry ? ArbState->Entry->PhysicalDeviceObject : NULL;
    UINT64 Alias;
    UINT64 Span;

    PAGED_CODE();

    if (Alternative != NULL)
    {
        Span = ArbState->End - ArbState->Start;

        Alias = ArbState->Start;
        while (IopArbPortGetNextAlias(Alternative->Descriptor->Flags, Alias, &Alias))
        {
            RtlDeleteRange(Arbiter->PossibleAllocation, Alias, Alias + Span, Owner);
        }
    }

    RtlDeleteRange(Arbiter->PossibleAllocation, ArbState->Start, ArbState->End, Owner);
}

/**
 * @brief Initialize the RootPortArbiter
 *
 * The root port arbiter owns the 16-bit x86 I/O port space and hands out
 * sub-ranges of it. It is the fallback for every device whose ports no closer
 * arbiter claims - root-enumerated and HAL-reported legacy hardware above all,
 * which is also the hardware that partially decodes its address lines and so
 * needs the aliasing this arbiter adds.
 *
 * @return NTSTATUS
 * @retval STATUS_SUCCESS
 * @retval STATUS_UNSUCCESSFUL
 * @retval STATUS_INSUFFICIENT_RESOURCES
 */
NTSTATUS
NTAPI
IopArbPortInitialize(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    IopRootPortArbiter.Name = L"RootPort";
    IopRootPortArbiter.UnpackRequirement = IopArbPortUnpackRequirements;
    IopRootPortArbiter.PackResource = IopArbPortPackResource;
    IopRootPortArbiter.UnpackResource = IopArbPortUnpackResource;
    IopRootPortArbiter.ScoreRequirement = IopArbPortScoreRequirement;

    /* Port-specific placement: boot-config leniency and ISA decode aliasing. */
    IopRootPortArbiter.FindSuitableRange = IopArbPortFindSuitableRange;
    IopRootPortArbiter.AddAllocation = IopArbPortAddAllocation;
    IopRootPortArbiter.BacktrackAllocation = IopArbPortBacktrackAllocation;

    Status = ArbiterLibInitializeInstance(&IopRootPortArbiter,
                                          NULL,
                                          CmResourceTypePort,
                                          IopRootPortArbiter.Name,
                                          L"Root",
                                          IopArbPortTranslateOrdering);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopArbPortInitialize: Failed with %X\n", Status);
    }

    return Status;
}
