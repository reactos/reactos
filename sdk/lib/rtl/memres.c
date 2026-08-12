/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Encode/decode helpers for memory and port resource descriptors.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * A plain Port/Memory descriptor stores its Length (and, for a requirement, its
 * Alignment) in a 32-bit field, so it can only describe regions up to 4 GB.  A
 * CmResourceTypeMemoryLarge descriptor reuses that same 32-bit field to hold a
 * bigger value by dropping the low bits that are known to be zero; the Flags say
 * how many were dropped:
 *
 *     CM_RESOURCE_MEMORY_LARGE_40 (0x200)  field holds bits [39:8]   (<< 8)
 *     CM_RESOURCE_MEMORY_LARGE_48 (0x400)  field holds bits [47:16]  (<< 16)
 *     CM_RESOURCE_MEMORY_LARGE_64 (0x800)  field holds bits [63:32]  (<< 32)
 */
static
ULONGLONG
RtlpDecodeMemIoValue(
    _In_ UCHAR Type,
    _In_ USHORT Flags,
    _In_ ULONG Encoded)
{
    if (Type == CmResourceTypePort || Type == CmResourceTypeMemory)
        return Encoded;

    if (Flags & CM_RESOURCE_MEMORY_LARGE_40)
        return (ULONGLONG)Encoded << 8;

    if (Flags & CM_RESOURCE_MEMORY_LARGE_48)
        return (ULONGLONG)Encoded << 16;

    if (Flags & CM_RESOURCE_MEMORY_LARGE_64)
        return (ULONGLONG)Encoded << 32;

    return 0;
}

/*
 * @implemented
 *
 * Decode an assigned (CM_PARTIAL_RESOURCE_DESCRIPTOR) memory or port descriptor:
 * return its full 64-bit length and, optionally, its start address.
 */
ULONGLONG
NTAPI
RtlCmDecodeMemIoResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _Out_opt_ PULONGLONG Start)
{
    if (Start != NULL)
        *Start = Descriptor->u.Generic.Start.QuadPart;

    return RtlpDecodeMemIoValue(Descriptor->Type,
                                Descriptor->Flags,
                                Descriptor->u.Generic.Length);
}

/*
 * @implemented
 *
 * Decode a requirement (IO_RESOURCE_DESCRIPTOR) memory or port descriptor:
 * return its full 64-bit length, maybe its (also large-encoded)
 * alignment and the 64-bit minimum/maximum address bounds.
 */
ULONGLONG
NTAPI
RtlIoDecodeMemIoResource(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _Out_opt_ PULONGLONG Alignment,
    _Out_opt_ PULONGLONG MinimumAddress,
    _Out_opt_ PULONGLONG MaximumAddress)
{
    if (Alignment != NULL)
    {
        *Alignment = RtlpDecodeMemIoValue(Descriptor->Type,
                                          Descriptor->Flags,
                                          Descriptor->u.Generic.Alignment);
    }

    if (MinimumAddress != NULL)
        *MinimumAddress = Descriptor->u.Generic.MinimumAddress.QuadPart;

    if (MaximumAddress != NULL)
        *MaximumAddress = Descriptor->u.Generic.MaximumAddress.QuadPart;

    return RtlpDecodeMemIoValue(Descriptor->Type,
                                Descriptor->Flags,
                                Descriptor->u.Generic.Length);
}

/*
 * @implemented
 *
 * Fill in a CM_PARTIAL_RESOURCE_DESCRIPTOR for a port or memory region.  When a
 * memory Length does not fit in 32 bits the descriptor is promoted to a
 * CmResourceTypeMemoryLarge form.
 *
 * Returns STATUS_INVALID_PARAMETER for an unsupported Type (or a port length
 * above 4 GB) and STATUS_UNSUCCESSFUL if the length cannot be represented by any
 * large form without dropping bits.
 */
NTSTATUS
NTAPI
RtlCmEncodeMemIoResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Type,
    _In_ ULONGLONG Length,
    _In_ ULONGLONG Start)
{
    if (Type != CmResourceTypePort &&
        Type != CmResourceTypeMemory &&
        Type != CmResourceTypeMemoryLarge)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* A port descriptor has only a 32-bit length and no large form. */
    if (Type == CmResourceTypePort)
    {
        if (Length > MAXULONG)
            return STATUS_INVALID_PARAMETER;

        Descriptor->Type = CmResourceTypePort;
        Descriptor->u.Generic.Start.QuadPart = Start;
        Descriptor->u.Generic.Length = (ULONG)Length;
        return STATUS_SUCCESS;
    }

    /* Drop any stale large-form flags and record the start address. */
    Descriptor->Flags &= ~CM_RESOURCE_MEMORY_LARGE;
    Descriptor->u.Generic.Start.QuadPart = Start;

    /* A length that fits 32 bits stays a plain memory descriptor. */
    if (Length <= MAXULONG)
    {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->u.Generic.Length = (ULONG)Length;
        return STATUS_SUCCESS;
    }

    /* Otherwise pick the tightest large form whose dropped low bits are zero. */
    if (Length <= ((ULONGLONG)MAXULONG << 8) && (Length & 0xFF) == 0)
    {
        Descriptor->u.Generic.Length = (ULONG)(Length >> 8);
        Descriptor->Flags |= CM_RESOURCE_MEMORY_LARGE_40;
    }
    else if (Length <= ((ULONGLONG)MAXULONG << 16) && (Length & 0xFFFF) == 0)
    {
        Descriptor->u.Generic.Length = (ULONG)(Length >> 16);
        Descriptor->Flags |= CM_RESOURCE_MEMORY_LARGE_48;
    }
    else if (Length <= ((ULONGLONG)MAXULONG << 32) && (Length & 0xFFFFFFFF) == 0)
    {
        Descriptor->u.Generic.Length = (ULONG)(Length >> 32);
        Descriptor->Flags |= CM_RESOURCE_MEMORY_LARGE_64;
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }

    Descriptor->Type = CmResourceTypeMemoryLarge;
    return STATUS_SUCCESS;
}

/*
 * A large descriptor expresses alignment at the same granularity as its length.
 * Scale the alignment up until it is a multiple of that granularity.
 */
static
BOOLEAN
RtlpEncodeLargeAlignment(
    _In_ ULONGLONG Alignment,
    _In_ ULONG Shift,
    _Out_ PULONG Encoded)
{
    ULONGLONG LowMask = ((ULONGLONG)1 << Shift) - 1;

    while ((Alignment & LowMask) != 0)
    {
        ULONGLONG Doubled = Alignment << 1;

        if (Doubled < Alignment)   /* overflowed past 64 bits */
            return FALSE;

        Alignment = Doubled;
    }

    *Encoded = (ULONG)(Alignment >> Shift);
    return TRUE;
}

/*
 * @implemented
 *
 * Fill in an IO_RESOURCE_DESCRIPTOR (a resource *requirement*) for a port or
 * memory range. Also encodes the alignment and the 64-bit address bounds.
 * A memory length above 4 GB is promoted to a CmResourceTypeMemoryLarge form,
 * which matters as drivers depend on this behavior.
 */
NTSTATUS
NTAPI
RtlIoEncodeMemIoResource(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Type,
    _In_ ULONGLONG Length,
    _In_ ULONGLONG Alignment,
    _In_ ULONGLONG MinimumAddress,
    _In_ ULONGLONG MaximumAddress)
{
    ULONG Shift;
    USHORT LargeFlag;

    if (Type != CmResourceTypePort &&
        Type != CmResourceTypeMemory &&
        Type != CmResourceTypeMemoryLarge)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* A port descriptor is limited to 32-bit length and alignment. */
    if (Type == CmResourceTypePort)
    {
        if (Length > MAXULONG || Alignment > MAXULONG)
            return STATUS_INVALID_PARAMETER;

        Descriptor->Type = CmResourceTypePort;
        Descriptor->u.Generic.MinimumAddress.QuadPart = MinimumAddress;
        Descriptor->u.Generic.MaximumAddress.QuadPart = MaximumAddress;
        Descriptor->u.Generic.Length = (ULONG)Length;
        Descriptor->u.Generic.Alignment = (ULONG)Alignment;
        return STATUS_SUCCESS;
    }

    /* Memory: reset any stale large-form flags and record the address bounds. */
    Descriptor->Flags &= ~CM_RESOURCE_MEMORY_LARGE;
    Descriptor->u.Generic.MinimumAddress.QuadPart = MinimumAddress;
    Descriptor->u.Generic.MaximumAddress.QuadPart = MaximumAddress;

    /* Both values fit 32 bits: a plain memory descriptor. */
    if (Length <= MAXULONG && Alignment <= MAXULONG)
    {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->u.Generic.Length = (ULONG)Length;
        Descriptor->u.Generic.Alignment = (ULONG)Alignment;
        return STATUS_SUCCESS;
    }

    /* Select the large form from the length; the alignment must fit that tier. */
    if (Length <= ((ULONGLONG)MAXULONG << 8))
    {
        if (Alignment > ((ULONGLONG)MAXULONG << 8))
            return STATUS_UNSUCCESSFUL;

        Shift = 8;
        LargeFlag = CM_RESOURCE_MEMORY_LARGE_40;
    }
    else if (Length <= ((ULONGLONG)MAXULONG << 16))
    {
        if (Alignment > ((ULONGLONG)MAXULONG << 16))
            return STATUS_UNSUCCESSFUL;

        Shift = 16;
        LargeFlag = CM_RESOURCE_MEMORY_LARGE_48;
    }
    else if (Length <= ((ULONGLONG)MAXULONG << 32))
    {
        Shift = 32;
        LargeFlag = CM_RESOURCE_MEMORY_LARGE_64;
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* The length must be exactly representable at this granularity. */
    if ((Length & (((ULONGLONG)1 << Shift) - 1)) != 0)
        return STATUS_UNSUCCESSFUL;

    if (!RtlpEncodeLargeAlignment(Alignment, Shift, &Descriptor->u.Generic.Alignment))
        return STATUS_UNSUCCESSFUL;

    Descriptor->Type = CmResourceTypeMemoryLarge;
    Descriptor->u.Generic.Length = (ULONG)(Length >> Shift);
    Descriptor->Flags |= LargeFlag;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * Round a requested length up to the nearest value the descriptor
 * 1:1 when it fits 32 bits, otherwise the next multiple of the
 * granularity.
 */
NTSTATUS
NTAPI
RtlFindClosestEncodableLength(
    _In_ ULONGLONG SourceLength,
    _Out_ PULONGLONG TargetLength)
{
    if (SourceLength <= MAXULONG)
        *TargetLength = SourceLength;
    else if (SourceLength <= ((ULONGLONG)MAXULONG << 8))
        *TargetLength = (SourceLength + 0xFF) & ~(ULONGLONG)0xFF;
    else if (SourceLength <= ((ULONGLONG)MAXULONG << 16))
        *TargetLength = (SourceLength + 0xFFFF) & ~(ULONGLONG)0xFFFF;
    else if (SourceLength <= ((ULONGLONG)MAXULONG << 32))
        *TargetLength = (SourceLength + 0xFFFFFFFF) & ~(ULONGLONG)0xFFFFFFFF;
    else
    {
        *TargetLength = 0;
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
