/*
 * PROJECT:     ReactOS ISA PnP Bus driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware definitions
 * COPYRIGHT:   Copyright 2010 Cameron Gutman <cameron.gutman@reactos.org>
 *              Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2021 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define ISAPNP_ADDRESS_PCAT    0x279
#define ISAPNP_WRITE_DATA_PCAT 0xA79

#define ISAPNP_ADDRESS_PC98    0x259
#define ISAPNP_WRITE_DATA_PC98 0xA59

#define ISAPNP_READPORT 0x00
#define ISAPNP_SERIALISOLATION 0x01
#define ISAPNP_CONFIGCONTROL 0x02
#define ISAPNP_WAKE 0x03
#define ISAPNP_RESOURCEDATA 0x04
#define ISAPNP_STATUS 0x05
#define ISAPNP_CARDSELECTNUMBER 0x06
#define ISAPNP_LOGICALDEVICENUMBER 0x07

#define ISAPNP_ACTIVATE 0x30
#define ISAPNP_IORANGECHECK 0x31

#define ISAPNP_MEMBASE(n) (0x40 + ((n) * 8))
#define ISAPNP_MEMCONTROL(n) (0x42 + ((n) * 8))
#define    MEMORY_UPPER_LIMIT 0x01
#define    MEMORY_USE_8_BIT_DECODER 0x00
#define    MEMORY_USE_16_BIT_DECODER 0x02
#define    MEMORY_USE_32_BIT_DECODER 0x06
#define ISAPNP_MEMLIMIT(n) (0x43 + ((n) * 8))
#define ISAPNP_IOBASE(n) (0x60 + ((n)*2))
#define ISAPNP_IRQNO(n) (0x70 + ((n)*2))
#define ISAPNP_IRQTYPE(n) (0x71 + ((n) * 2))
#define    IRQTYPE_LOW_LEVEL 0x01
#define    IRQTYPE_HIGH_EDGE 0x02
#define ISAPNP_DMACHANNEL(n) (0x74 + (n))
#define    DMACHANNEL_NONE 4
#define ISAPNP_MEMBASE32(n) ((n) == 0 ? 0x76 : (0x70 + (n) * 16))
#define ISAPNP_MEMCONTROL32(n) ((n) == 0 ? 0x7A : (0x74 + (n) * 16))
#define ISAPNP_MEMLIMIT32(n) ((n) == 0 ? 0x7B : (0x75 + (n) * 16))

#define ISAPNP_CONFIG_RESET (1 << 0)
#define ISAPNP_CONFIG_WAIT_FOR_KEY (1 << 1)
#define ISAPNP_CONFIG_RESET_CSN (1 << 2)

#define ISAPNP_LFSR_SEED 0x6A

#define ISAPNP_IS_SMALL_TAG(t) (!((t) & 0x80))
#define ISAPNP_SMALL_TAG_NAME(t) (((t) >> 3) & 0xF)
#define ISAPNP_SMALL_TAG_LEN(t) (((t) & 0x7))
#define ISAPNP_TAG_PNPVERNO 0x01
#define ISAPNP_TAG_LOGDEVID 0x02
#define ISAPNP_TAG_COMPATDEVID 0x03
#define ISAPNP_TAG_IRQ 0x04
#define ISAPNP_TAG_DMA 0x05
#define ISAPNP_TAG_STARTDEP 0x06
#define ISAPNP_TAG_ENDDEP 0x07
#define ISAPNP_TAG_IOPORT 0x08
#define ISAPNP_TAG_FIXEDIO 0x09
#define ISAPNP_TAG_END 0x0F

#define ISAPNP_IS_LARGE_TAG(t) (((t) & 0x80))
#define ISAPNP_LARGE_TAG_NAME(t) (t)
#define ISAPNP_TAG_MEMRANGE 0x81
#define    MEMRANGE_16_BIT_MEMORY_MASK (0x10 | 0x08)
#define       MEMRANGE_32_BIT_MEMORY_ONLY 0x18
#define ISAPNP_TAG_ANSISTR 0x82
#define ISAPNP_TAG_UNICODESTR 0x83
#define ISAPNP_TAG_MEM32RANGE 0x85
#define ISAPNP_TAG_FIXEDMEM32RANGE 0x86

#define RANGE_LENGTH_TO_LENGTH(RangeLength) ((~(RangeLength) + 1) & 0xFFFFFF)
#define LENGTH_TO_RANGE_LENGTH(Length) (~(Length) + 1)

#include <pshpack1.h>

typedef struct _ISAPNP_IDENTIFIER
{
    USHORT VendorId;
    USHORT ProdId;
    ULONG Serial;
    UCHAR Checksum;
} ISAPNP_IDENTIFIER, *PISAPNP_IDENTIFIER;

typedef struct _ISAPNP_LOGDEVID
{
    USHORT VendorId;
    USHORT ProdId;
    USHORT Flags;
} ISAPNP_LOGDEVID, *PISAPNP_LOGDEVID;

typedef struct _ISAPNP_COMPATID
{
    USHORT VendorId;
    USHORT ProdId;
} ISAPNP_COMPATID, *PISAPNP_COMPATID;

typedef struct _ISAPNP_IO_DESCRIPTION
{
    UCHAR Information;
    USHORT Minimum;
    USHORT Maximum;
    UCHAR Alignment;
    UCHAR Length;
} ISAPNP_IO_DESCRIPTION, *PISAPNP_IO_DESCRIPTION;

typedef struct _ISAPNP_FIXED_IO_DESCRIPTION
{
    USHORT IoBase;
    UCHAR Length;
} ISAPNP_FIXED_IO_DESCRIPTION, *PISAPNP_FIXED_IO_DESCRIPTION;

typedef struct _ISAPNP_IRQ_DESCRIPTION
{
    USHORT Mask;
    UCHAR Information;
} ISAPNP_IRQ_DESCRIPTION, *PISAPNP_IRQ_DESCRIPTION;

typedef struct _ISAPNP_DMA_DESCRIPTION
{
    UCHAR Mask;
    UCHAR Information;
} ISAPNP_DMA_DESCRIPTION, *PISAPNP_DMA_DESCRIPTION;

typedef struct _ISAPNP_MEMRANGE_DESCRIPTION
{
    UCHAR Information;
    USHORT Minimum;
    USHORT Maximum;
    USHORT Alignment;
    USHORT Length;
} ISAPNP_MEMRANGE_DESCRIPTION, *PISAPNP_MEMRANGE_DESCRIPTION;

typedef struct _ISAPNP_MEMRANGE32_DESCRIPTION
{
    UCHAR Information;
    ULONG Minimum;
    ULONG Maximum;
    ULONG Alignment;
    ULONG Length;
} ISAPNP_MEMRANGE32_DESCRIPTION, *PISAPNP_MEMRANGE32_DESCRIPTION;

typedef struct _ISAPNP_FIXEDMEMRANGE_DESCRIPTION
{
    UCHAR Information;
    ULONG MemoryBase;
    ULONG Length;
} ISAPNP_FIXEDMEMRANGE_DESCRIPTION, *PISAPNP_FIXEDMEMRANGE_DESCRIPTION;

#include <poppack.h>

#ifdef __cplusplus
}
#endif
