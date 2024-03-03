/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA definitions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#define ATA_MAX_LBA_28      0x0FFFFFFFULL
#define ATA_MAX_LBA_48      (1ULL << 48)

#define IDE_DRIVE_SELECT_SLAVE       0x10
#define IDE_HIGH_ORDER_BYTE          0x80
#define IDE_DRIVE_SELECT             0xA0

#define IDE_ERROR_WRITE_PROTECT      0x40

#define IDE_FEATURE_PIO         0x00
#define IDE_FEATURE_DMA         0x01
#define IDE_FEATURE_DMADIR      0x04

#define IDE_DEVICE_FUA_NCQ      0x80

#define IDE_DC_ALWAYS           0x08

#define IDE_COMMAND_REQUEST_SENSE_DATA_EXT      0x0B

#define SRB_FLAG_RETRY_COUNT_MASK  0x000000FF
#define SRB_FLAG_LOW_MEM_RETRY     0x00000100
#define SRB_FLAG_PIO_RETRY         0x00000200

#define SRB_SET_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension | (Flags)))

#define SRB_CLEAR_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension & ~(Flags)))

#define SRB_GET_FLAGS(Srb)   ((ULONG_PTR)(Srb)->SrbExtension)

FORCEINLINE
BOOLEAN
AtaPacketCommandUseDma(
    _In_ UCHAR OpCode)
{
    return (OpCode == SCSIOP_READ6 ||
            OpCode == SCSIOP_WRITE6 ||
            OpCode == SCSIOP_READ ||
            OpCode == SCSIOP_WRITE ||
            OpCode == SCSIOP_READ12 ||
            OpCode == SCSIOP_WRITE12 ||
            OpCode == SCSIOP_READ16 ||
            OpCode == SCSIOP_WRITE16 ||
            OpCode == SCSIOP_READ_CD ||
            OpCode == SCSIOP_READ_CD_MSF);
}

FORCEINLINE
BOOLEAN
AtaCommandUseLba48(
    _In_ ULONG64 SectorNumber,
    _In_ ULONG SectorCount)
{
    /* Use the 48-bit command when reasonable */
    return (((SectorNumber + SectorCount) >= ATA_MAX_LBA_28) || (SectorCount > 0x100));
}
