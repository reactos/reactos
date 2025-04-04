/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA hardware
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#define ATA_MAX_LBA_28 0x0FFFFFFFULL
#define ATA_MAX_LBA_48 (1ULL << 48)

#define IDE_DRIVE_SELECT_SLAVE       0x10
#define IDE_HIGH_ORDER_BYTE          0x80
#define IDE_DRIVE_SELECT             0xA0

#define IDE_ERROR_WRITE_PROTECT      0x40

#define IDE_FEATURE_PIO         0x00
#define IDE_FEATURE_DMA         0x01
#define IDE_FEATURE_DMADIR      0x04

#define IDE_DEVICE_FUA_NCQ      0x80

#define IDE_DC_ALWAYS           0x08

#undef IDE_COMMAND_READ_LOG_DMA_EXT
#define IDE_COMMAND_READ_LOG_DMA_EXT            0x47

#define IDE_COMMAND_REQUEST_SENSE_DATA_EXT      0x0B
#define IDE_COMMAND_READ_PORT_MULTIPLIER        0xE4
#define IDE_COMMAND_WRITE_PORT_MULTIPLIER       0xE8

#define SRB_FLAG_RETRY             0x00000001

#define SRB_SET_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension | (Flags)))

#define SRB_CLEAR_FLAGS(Srb, Flags) \
    ((Srb)->SrbExtension = (PVOID)((ULONG_PTR)(Srb)->SrbExtension & ~(Flags)))

#define SRB_GET_FLAGS(Srb) \
    ((ULONG_PTR)(Srb)->SrbExtension)

extern const UCHAR AtapReadWriteCommandMap[12][2];

FORCEINLINE
ATA_DEVICE_CLASS
AtaPdoGetDeviceClass(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    if (IS_ATAPI(DevExt))
        return DEV_ATAPI;

    return DEV_ATA;
}

FORCEINLINE
BOOLEAN
AtaPdoIsSerialNumberValid(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    return (DevExt->SerialNumber[0] != ANSI_NULL);
}

FORCEINLINE
ULONG
AtaFdoMaxDeviceCount(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    if (IS_AHCI(ChanExt))
        return AHCI_MAX_PORTS;
#if defined(_M_IX86)
    else if (ChanExt->PortData->PortFlags & PORT_FLAG_CBUS_IDE)
        return CHANNEL_PC98_MAX_DEVICES;
#endif
    else
        return CHANNEL_PCAT_MAX_DEVICES;
}

FORCEINLINE
UCHAR
AtaReadWriteCommand(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
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
