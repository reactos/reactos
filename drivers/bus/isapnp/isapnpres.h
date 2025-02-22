/*
 * PROJECT:     ReactOS ISA PnP Bus driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Resource management header file
 * COPYRIGHT:   Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 */

#pragma once

/** @brief Maximum size of resource data structure supported by the driver. */
#define ISAPNP_MAX_RESOURCEDATA   0x1000

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

typedef enum
{
    dfNotStarted,
    dfStarted,
    dfDone
} ISAPNP_DEPENDENT_FUNCTION_STATE;

typedef struct _ISAPNP_RESOURCE
{
    UCHAR Type;
#define ISAPNP_RESOURCE_TYPE_END               0
#define ISAPNP_RESOURCE_TYPE_IO                1
#define ISAPNP_RESOURCE_TYPE_IRQ               2
#define ISAPNP_RESOURCE_TYPE_DMA               3
#define ISAPNP_RESOURCE_TYPE_MEMRANGE          4
#define ISAPNP_RESOURCE_TYPE_MEMRANGE32        5
#define ISAPNP_RESOURCE_TYPE_START_DEPENDENT   6
#define ISAPNP_RESOURCE_TYPE_END_DEPENDENT     7

    union
    {
        ISAPNP_IO_DESCRIPTION IoDescription;
        ISAPNP_IRQ_DESCRIPTION IrqDescription;
        ISAPNP_DMA_DESCRIPTION DmaDescription;
        ISAPNP_MEMRANGE_DESCRIPTION MemRangeDescription;
        ISAPNP_MEMRANGE32_DESCRIPTION MemRange32Description;
        UCHAR Priority;
    };
} ISAPNP_RESOURCE, *PISAPNP_RESOURCE;

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

/** The card implements 24-bit memory decoder */
#define ISAPNP_HAS_MEM24_DECODER    0x00000008

/** The card implements 32-bit memory decoder */
#define ISAPNP_HAS_MEM32_DECODER    0x00000010

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
    PISAPNP_RESOURCE Resources;
    PSTR FriendlyName;
    LIST_ENTRY CompatibleIdList;

    ISAPNP_IO Io[8];
    ISAPNP_IRQ Irq[2];
    ISAPNP_DMA Dma[2];
    ISAPNP_MEMRANGE MemRange[4];
    ISAPNP_MEMRANGE32 MemRange32[4];
    /**@}*/
} ISAPNP_LOGICAL_DEVICE, *PISAPNP_LOGICAL_DEVICE;
