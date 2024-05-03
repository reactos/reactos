/*
 * PROJECT:     ReactOS ISA PnP Bus driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Resource management header file
 * COPYRIGHT:   Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 */

#pragma once

/** @brief Maximum size of resource data structure supported by the driver. */
#define ISAPNP_MAX_RESOURCEDATA 0x1000

/** @brief Maximum number of Start DF tags supported by the driver. */
#define ISAPNP_MAX_ALTERNATIVES 8

typedef struct _ISAPNP_IO
{
    USHORT CurrentBase;
    ISAPNP_IO_DESCRIPTION Description;
    UCHAR Index;
} ISAPNP_IO, *PISAPNP_IO;

typedef struct _ISAPNP_IRQ
{
    UCHAR CurrentNo;
    UCHAR CurrentType;
    ISAPNP_IRQ_DESCRIPTION Description;
    UCHAR Index;
} ISAPNP_IRQ, *PISAPNP_IRQ;

typedef struct _ISAPNP_DMA
{
    UCHAR CurrentChannel;
    ISAPNP_DMA_DESCRIPTION Description;
    UCHAR Index;
} ISAPNP_DMA, *PISAPNP_DMA;

typedef struct _ISAPNP_MEMRANGE
{
    ULONG CurrentBase;
    ULONG CurrentLength;
    ISAPNP_MEMRANGE_DESCRIPTION Description;
    UCHAR Index;
} ISAPNP_MEMRANGE, *PISAPNP_MEMRANGE;

typedef struct _ISAPNP_MEMRANGE32
{
    ULONG CurrentBase;
    ULONG CurrentLength;
    ISAPNP_MEMRANGE32_DESCRIPTION Description;
    UCHAR Index;
} ISAPNP_MEMRANGE32, *PISAPNP_MEMRANGE32;

typedef struct _ISAPNP_COMPATIBLE_ID_ENTRY
{
    UCHAR VendorId[3];
    USHORT ProdId;
    LIST_ENTRY IdLink;
} ISAPNP_COMPATIBLE_ID_ENTRY, *PISAPNP_COMPATIBLE_ID_ENTRY;

typedef struct _ISAPNP_ALTERNATIVES
{
    ISAPNP_IO_DESCRIPTION Io[ISAPNP_MAX_ALTERNATIVES];
    ISAPNP_IRQ_DESCRIPTION Irq[ISAPNP_MAX_ALTERNATIVES];
    ISAPNP_DMA_DESCRIPTION Dma[ISAPNP_MAX_ALTERNATIVES];
    ISAPNP_MEMRANGE_DESCRIPTION MemRange[ISAPNP_MAX_ALTERNATIVES];
    ISAPNP_MEMRANGE32_DESCRIPTION MemRange32[ISAPNP_MAX_ALTERNATIVES];
    UCHAR Priority[ISAPNP_MAX_ALTERNATIVES];
    UCHAR IoIndex;
    UCHAR IrqIndex;
    UCHAR DmaIndex;
    UCHAR MemRangeIndex;
    UCHAR MemRange32Index;

    _Field_range_(0, ISAPNP_MAX_ALTERNATIVES)
    UCHAR Count;
} ISAPNP_ALTERNATIVES, *PISAPNP_ALTERNATIVES;

typedef struct _ISAPNP_LOGICAL_DEVICE
{
    LIST_ENTRY DeviceLink;
    PDEVICE_OBJECT Pdo;

    ULONG Flags;
/** Cleared when the device is physically removed */
#define ISAPNP_PRESENT              0x00000001

/** Indicates if the parent card has multiple logical devices */
#define ISAPNP_HAS_MULTIPLE_LOGDEVS 0x00000002

/** Cleared when the device has no boot resources */
#define ISAPNP_HAS_RESOURCES        0x00000004

    /**
     * @name The card data.
     * @{
     */
    UCHAR CSN;
    UCHAR VendorId[3];
    USHORT ProdId;
    ULONG SerialNumber;
    /**@}*/

    /**
     * @name The logical device data.
     * @{
     */
    UCHAR LDN;
    UCHAR LogVendorId[3];
    USHORT LogProdId;
    PISAPNP_ALTERNATIVES Alternatives;
    PSTR FriendlyName;
    LIST_ENTRY CompatibleIdList;

    ISAPNP_IO Io[8];
    ISAPNP_IRQ Irq[2];
    ISAPNP_DMA Dma[2];
    ISAPNP_MEMRANGE MemRange[4];
    ISAPNP_MEMRANGE32 MemRange32[4];
    /**@}*/
} ISAPNP_LOGICAL_DEVICE, *PISAPNP_LOGICAL_DEVICE;

FORCEINLINE
BOOLEAN
HasIoAlternatives(
    _In_ PISAPNP_ALTERNATIVES Alternatives)
{
    return (Alternatives->Io[0].Length != 0);
}

FORCEINLINE
BOOLEAN
HasIrqAlternatives(
    _In_ PISAPNP_ALTERNATIVES Alternatives)
{
    return (Alternatives->Irq[0].Mask != 0);
}

FORCEINLINE
BOOLEAN
HasDmaAlternatives(
    _In_ PISAPNP_ALTERNATIVES Alternatives)
{
    return (Alternatives->Dma[0].Mask != 0);
}

FORCEINLINE
BOOLEAN
HasMemoryAlternatives(
    _In_ PISAPNP_ALTERNATIVES Alternatives)
{
    return (Alternatives->MemRange[0].Length != 0);
}

FORCEINLINE
BOOLEAN
HasMemory32Alternatives(
    _In_ PISAPNP_ALTERNATIVES Alternatives)
{
    return (Alternatives->MemRange32[0].Length != 0);
}
