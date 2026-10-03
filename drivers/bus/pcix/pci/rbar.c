/*
 * PROJECT:     ReactOS PCI Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PCI Express Resizable BAR Support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define PCI_RBAR_SIZE_TO_LENGTH(Bit)    (1ULL << ((Bit) + 20))

/* FUNCTIONS ******************************************************************/

/**
 * @brief
 * Records which BARs of a new function can be resized and the sizes each
 * of them supports.
 *
 * @param[in,out] PdoExtension
 * The PDO extension of the function being enumerated.
 */
VOID
NTAPI
PciGetResizableBarCapability(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension)
{
    PPCI_RESIZABLE_BAR_STATE State = &PdoExtension->ResizableBarState;
    PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
    ULONG Offset, Entry, EntryCount, Control, SizeMask, BarIndex;
    PAGED_CODE();

    RtlZeroMemory(State, sizeof(*State));

    /* Bridges keep their windows after the first two BARs, so only endpoints are resized */
    if (PdoExtension->HeaderType != PCI_DEVICE_TYPE)
        return;

    Offset = PciReadDeviceExtendedCapability(PdoExtension,
                                             PCI_RBAR_EXTENDED_CAP_ID,
                                             &Header,
                                             sizeof(Header));
    if (!Offset)
        return;

    /* Only the first control register holds the number of entries */
    PciReadDeviceConfig(PdoExtension,
                        &Control,
                        Offset + PCI_RBAR_ENTRY_CONTROL(0),
                        sizeof(Control));
    EntryCount = (Control & PCI_RBAR_CONTROL_COUNT_MASK) >> PCI_RBAR_CONTROL_COUNT_SHIFT;
    if (EntryCount > PCI_RBAR_MAX_ENTRIES)
        EntryCount = PCI_RBAR_MAX_ENTRIES;

    State->CapabilityPtr = (USHORT)Offset;

    for (Entry = 0; Entry < EntryCount; Entry++)
    {
        PciReadDeviceConfig(PdoExtension,
                            &SizeMask,
                            Offset + PCI_RBAR_ENTRY_CAPABILITY(Entry),
                            sizeof(SizeMask));
        PciReadDeviceConfig(PdoExtension,
                            &Control,
                            Offset + PCI_RBAR_ENTRY_CONTROL(Entry),
                            sizeof(Control));

        SizeMask >>= PCI_RBAR_CAPABILITY_SIZES_SHIFT;
        BarIndex = Control & PCI_RBAR_CONTROL_BAR_INDEX_MASK;

        /* Skip entries with no sizes, a BAR that cannot exist, or a BAR already seen */
        if (!SizeMask ||
            (BarIndex >= PCI_RBAR_MAX_ENTRIES) ||
            State->SizeMask[BarIndex])
        {
            continue;
        }

        State->SizeMask[BarIndex] = SizeMask;
        State->EntryIndex[BarIndex] = (UCHAR)Entry;

        DPRINT1("PCI: BAR %lu of %p is resizable, sizes 0x%08lx\n",
                BarIndex,
                PdoExtension,
                SizeMask);
    }
}

static
BOOLEAN
NTAPI
PciFillResizedBarRequirement(
    _Out_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ PIO_RESOURCE_DESCRIPTOR Limit,
    _In_ ULONGLONG Length,
    _In_ UCHAR Option)
{
    NTSTATUS Status;

    *Descriptor = *Limit;
    Descriptor->Option = Option;
    Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

    /* A size of 4GB or more takes the large memory form */
    Status = RtlIoEncodeMemIoResource(Descriptor,
                                      CmResourceTypeMemory,
                                      Length,
                                      Length,
                                      Limit->u.Memory.MinimumAddress.QuadPart,
                                      Limit->u.Memory.MaximumAddress.QuadPart);
    return NT_SUCCESS(Status);
}

/**
 * @brief
 * Writes the requirements for the sizes a resizable BAR can grow to. They
 * go ahead of the default requirement, which the caller then lists as the
 * alternative. The largest size is preferred, and the smallest supported
 * size follows it when that is also bigger than the default.
 *
 * @param[in] PdoExtension
 * The PDO extension of the function.
 *
 * @param[in] BarIndex
 * The BAR the default requirement describes.
 *
 * @param[in] Limit
 * The default requirement of the BAR.
 *
 * @param[out] Descriptors
 * Receives up to two descriptors, or NULL to only count them.
 *
 * @return
 * The number of descriptors written, which is 0 when the BAR cannot grow.
 */
ULONG
NTAPI
PciAddResizableBarRequirements(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ ULONG BarIndex,
    _In_ PIO_RESOURCE_DESCRIPTOR Limit,
    _Out_writes_opt_(2) PIO_RESOURCE_DESCRIPTOR Descriptors)
{
    IO_RESOURCE_DESCRIPTOR Resized[2];
    ULONGLONG Largest, Smallest;
    ULONG SizeMask, Count;
    PAGED_CODE();

    /* Only a memory BAR asking for less than 4GB is offered other sizes */
    if ((BarIndex >= PCI_RBAR_MAX_ENTRIES) || (Limit->Type != CmResourceTypeMemory))
        return 0;

    SizeMask = PdoExtension->ResizableBarState.SizeMask[BarIndex];
    if (!SizeMask)
        return 0;

    /* Built even when only counting, so a size that cannot be described is never counted */
    Largest = PCI_RBAR_SIZE_TO_LENGTH(RtlFindMostSignificantBit(SizeMask));
    if ((Largest <= Limit->u.Memory.Length) ||
        !PciFillResizedBarRequirement(&Resized[0], Limit, Largest, IO_RESOURCE_PREFERRED))
    {
        return 0;
    }
    Count = 1;

    Smallest = PCI_RBAR_SIZE_TO_LENGTH(RtlFindLeastSignificantBit(SizeMask));
    if ((Smallest > Limit->u.Memory.Length) &&
        (Smallest < Largest) &&
        PciFillResizedBarRequirement(&Resized[1], Limit, Smallest, IO_RESOURCE_ALTERNATIVE))
    {
        Count++;
    }

    if (Descriptors)
        RtlCopyMemory(Descriptors, Resized, Count * sizeof(IO_RESOURCE_DESCRIPTOR));

    return Count;
}

static
VOID
NTAPI
PciSelectResizableBarSize(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ ULONG BarIndex,
    _In_ ULONGLONG Length)
{
    PPCI_RESIZABLE_BAR_STATE State = &PdoExtension->ResizableBarState;
    ULONGLONG Megabytes;
    ULONG SizeBit, Control, Offset;

    /* Only a power of two of at least a megabyte can be named in the size field */
    Megabytes = Length >> 20;
    if (!Megabytes || (Megabytes & (Megabytes - 1)) || (Length & 0xFFFFF))
        return;

    if (!(State->SizeMask[BarIndex] & Megabytes))
        return;

    SizeBit = (ULONG)RtlFindMostSignificantBit(Megabytes);

    Offset = State->CapabilityPtr + PCI_RBAR_ENTRY_CONTROL(State->EntryIndex[BarIndex]);
    PciReadDeviceConfig(PdoExtension, &Control, Offset, sizeof(Control));
    if (((Control & PCI_RBAR_CONTROL_SIZE_MASK) >> PCI_RBAR_CONTROL_SIZE_SHIFT) == SizeBit)
        return;

    Control &= ~PCI_RBAR_CONTROL_SIZE_MASK;
    Control |= SizeBit << PCI_RBAR_CONTROL_SIZE_SHIFT;
    PciWriteDeviceConfig(PdoExtension, &Control, Offset, sizeof(Control));

    DPRINT("PCI: BAR %lu of %p resized to %I64u MB\n", BarIndex, PdoExtension, Megabytes);
}

/**
 * @brief
 * Sets every resizable BAR to the length it was assigned. The memory decode
 * has to be off, since the size field changes what the BAR decodes. Critical
 * devices call this from the IPI that updates their hardware.
 *
 * @param[in] PdoExtension
 * The PDO extension of the function whose resources are being set.
 */
VOID
NTAPI
PciApplyResizableBarSizes(
    _In_ PPCI_PDO_EXTENSION PdoExtension)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Current;
    USHORT Command;
    ULONG BarIndex;

    if (!PdoExtension->ResizableBarState.CapabilityPtr || !PdoExtension->Resources)
        return;

    /* A function whose decodes could not be turned off keeps the sizes it has */
    PciReadDeviceConfig(PdoExtension,
                        &Command,
                        FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                        sizeof(Command));
    if (Command & PCI_ENABLE_MEMORY_SPACE)
    {
        DPRINT("PCI: Memory decode of %p is still on, not resizing BARs\n", PdoExtension);
        return;
    }

    for (BarIndex = 0; BarIndex < PCI_RBAR_MAX_ENTRIES; BarIndex++)
    {
        if (!PdoExtension->ResizableBarState.SizeMask[BarIndex])
            continue;

        Current = &PdoExtension->Resources->Current[BarIndex];
        if ((Current->Type != CmResourceTypeMemory) &&
            (Current->Type != CmResourceTypeMemoryLarge))
        {
            continue;
        }

        PciSelectResizableBarSize(PdoExtension,
                                  BarIndex,
                                  RtlCmDecodeMemIoResource(Current, NULL));
    }
}

/* EOF */
