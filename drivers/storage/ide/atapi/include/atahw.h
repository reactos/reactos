/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA definitions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#define PORT_MAX_RESET_RETRY_COUNT       5
#define PORT_MAX_PMP_DETECT_RETRY_COUNT  3
#define TARGET_MAX_RESET_RETRY_COUNT     3

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

/* This is an error in the DDK headers */
#undef IDE_COMMAND_READ_LOG_DMA_EXT
#define IDE_COMMAND_READ_LOG_DMA_EXT            0x47

#define IDE_COMMAND_REQUEST_SENSE_DATA_EXT      0x0B
#define IDE_COMMAND_READ_PORT_MULTIPLIER        0xE4
#define IDE_COMMAND_WRITE_PORT_MULTIPLIER       0xE8

/**
 * If a larger transfer is attempted, the 16-bit ByteCount register might overflow.
 * In this case we round down the length to the closest multiple of 2.
 */
#define ATAPI_MAX_DRQ_DATA_BLOCK   0xFFFE

#define SRB_FLAG_RETRY_COUNT_MASK  0x000000FF
#define SRB_FLAG_LOW_MEM_RETRY     0x00000100
#define SRB_FLAG_PIO_ONLY          0x00000200

#define SRB_SET_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension | (Flags)))

#define SRB_CLEAR_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension & ~(Flags)))

#define SRB_GET_FLAGS(Srb)   ((ULONG_PTR)(Srb)->SrbExtension)

extern const UCHAR AtapReadWriteCommandMap[12][2];

/**
 * See eSATA paper
 * http://download.microsoft.com/download/7/E/7/7E7662CF-CBEA-470B-A97E-CE7CE0D98DC2/eSATA.docx
 */
FORCEINLINE
BOOLEAN
AtaAhciIsPortRemovable(
    _In_ ULONG AhciCapabilities,
    _In_ ULONG CmdStatus)
{
    if (CmdStatus & AHCI_PXCMD_HPCP)
        return TRUE;

    if ((AhciCapabilities & AHCI_CAP_SXS) && (CmdStatus & AHCI_PXCMD_ESP))
        return TRUE;

    if ((AhciCapabilities & AHCI_CAP_SMPS) && (CmdStatus & AHCI_PXCMD_MPSP))
        return TRUE;

    return FALSE;
}

FORCEINLINE
UCHAR
AtaReadWriteCommand(
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG CmdEntry = (Request->Flags & REQUEST_FLAG_LBA48) ? 0 : 6;

    if (Request->Flags & REQUEST_FLAG_FUA)
    {
        CmdEntry += 3;
    }

    if (Request->Flags & REQUEST_FLAG_DMA)
    {
        CmdEntry += 2;
    }
    else if (Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE)
    {
        CmdEntry += 1;
    }

    return AtapReadWriteCommandMap[CmdEntry][(Request->Flags & REQUEST_FLAG_DATA_IN) ? 1 : 0];
}

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
