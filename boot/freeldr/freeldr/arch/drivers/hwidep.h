/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Private header for ATA/ATAPI programmed I/O driver.
 * COPYRIGHT:   Copyright 2019-2025 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define TAG_ATA_DEVICE 'dATA'

#define ATA_STATUS_SUCCESS   0
#define ATA_STATUS_PENDING   1
#define ATA_STATUS_ERROR     2
#define ATA_STATUS_RESET     3
#define ATA_STATUS_RETRY     4

#if defined(SARCH_PC98)
/* Master/Slave devices for Bank 0 and Bank 1 */
#define CHANNEL_MAX_DEVICES       4
#define DEV_SLAVE(DeviceNumber)   ((DeviceNumber) & 1)
#else
/* Master/Slave devices */
#define CHANNEL_MAX_DEVICES       2
#define DEV_SLAVE(DeviceNumber)   (DeviceNumber)
#endif

#if defined(SARCH_XBOX)
/* It's safe to enable the multiple mode */
#define ATA_ENABLE_MULTIPLE_MODE
#endif

/* Delay of 400ns */
#if defined(SARCH_PC98)
#define ATA_IO_WAIT()    WRITE_PORT_UCHAR((PUCHAR)0x5F, 0)
#else
#define ATA_IO_WAIT()    StallExecutionProcessor(1)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
/* x86 I/O address space spans from 0 to 0xFFFF */
typedef USHORT IDE_REG;
#else
typedef ULONG_PTR IDE_REG;
#endif

#define ATA_MAX_LBA_28    0x0FFFFFFFULL
#define ATA_MAX_LBA_48    (1ULL << 48)

#define IDE_FEATURE_PIO         0x00

#define IDE_DC_ALWAYS           0x08

#define IDE_DRIVE_SELECT        0xA0

#define ATAPI_INT_REASON_COD              0x01
#define ATAPI_INT_REASON_IO               0x02
#define ATAPI_INT_REASON_MASK             (ATAPI_INT_REASON_IO | ATAPI_INT_REASON_COD)

#define ATAPI_INT_REASON_STATUS_NEC       0x00
#define ATAPI_INT_REASON_STATUS           (ATAPI_INT_REASON_IO | ATAPI_INT_REASON_COD)
#define ATAPI_INT_REASON_AWAIT_CDB        (IDE_STATUS_DRQ | ATAPI_INT_REASON_COD)
#define ATAPI_INT_REASON_DATA_IN          (ATAPI_INT_REASON_IO | IDE_STATUS_DRQ)

#define MAXIMUM_CDROM_SIZE      804 // == sizeof(CDROM_TOC)

#define ATA_TIME_BUSY_SELECT    2000    ///< 20 ms
#define ATA_TIME_BUSY_POLL      500000  ///< 5 s
#define ATA_TIME_BUSY_ENUM      100     ///< 1 ms
#define ATA_TIME_BUSY_RESET     1000000 ///< 10 s
#define ATA_TIME_RESET_SELECT   200000  ///< 2 s
#define ATA_TIME_DRQ_CLEAR      100     ///< 200 us
#define ATA_TIME_PHASE_CHANGE   100     ///< 1 ms

#define ATA_WRITE(Port, Value) \
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)(Port), (Value))

#define ATA_WRITE_BLOCK_16(Port, Buffer, Count) \
    WRITE_PORT_BUFFER_USHORT((PUSHORT)(ULONG_PTR)(Port), (PUSHORT)(Buffer), (Count))

#define ATA_WRITE_BLOCK_32(Port, Buffer, Count) \
    WRITE_PORT_BUFFER_ULONG((PULONG)(ULONG_PTR)(Port), (PULONG)(Buffer), (Count))

#define ATA_READ(Port) \
    READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)(Port))

#define ATA_READ_BLOCK_16(Port, Buffer, Count) \
    READ_PORT_BUFFER_USHORT((PUSHORT)(ULONG_PTR)(Port), (PUSHORT)(Buffer), (Count))

#define ATA_READ_BLOCK_32(Port, Buffer, Count) \
    READ_PORT_BUFFER_ULONG((PULONG)(ULONG_PTR)(Port), (PULONG)(Buffer), (Count))

typedef enum _ATA_DEVICE_CLASS
{
    DEV_ATA,
    DEV_ATAPI,
    DEV_NONE,
} ATA_DEVICE_CLASS, *PATA_DEVICE_CLASS;

typedef struct _IDE_REGISTERS
{
    IDE_REG Data;
    union
    {
        IDE_REG Features;
        IDE_REG Error;
    };
    union
    {
        IDE_REG SectorCount;
        IDE_REG InterruptReason;
    };
    IDE_REG LbaLow;             ///< LBA bits 0-7, 24-31
    union
    {
        IDE_REG LbaMid;         ///< LBA bits 8-15, 32-39
        IDE_REG ByteCountLow;
        IDE_REG SignatureLow;
    };
    union
    {
        IDE_REG LbaHigh;        ///< LBA bits 16-23, 40-47
        IDE_REG ByteCountHigh;
        IDE_REG SignatureHigh;
    };
    IDE_REG Device;
    union
    {
        IDE_REG Command;
        IDE_REG Status;
    };
    union
    {
        IDE_REG Control;
        IDE_REG AlternateStatus;
    };
} IDE_REGISTERS, *PIDE_REGISTERS;

typedef struct _ATA_TASKFILE
{
    UCHAR DriveSelect;
    UCHAR Command;
    struct
    {
        UCHAR Feature;
        UCHAR SectorCount;
        UCHAR LowLba;
        UCHAR MidLba;
        UCHAR HighLba;
    } Data[2]; // 0 - low part, 1 - high part
} ATA_TASKFILE, *PATA_TASKFILE;

typedef struct _ATA_DEVICE_REQUEST
{
    union
    {
        UCHAR Cdb[16];
        ATA_TASKFILE TaskFile;
    };
    PVOID DataBuffer;
    ULONG Flags;
#define REQUEST_FLAG_LBA48                  0x00000001
#define REQUEST_FLAG_READ_WRITE_MULTIPLE    0x00000002
#define REQUEST_FLAG_PACKET_COMMAND         0x00000004
#define REQUEST_FLAG_SET_DEVICE_REGISTER    0x00000008
#define REQUEST_FLAG_AWAIT_CDB              0x00000010
#define REQUEST_FLAG_READ_COMMAND           0x00000020
#define REQUEST_FLAG_IDENTIFY_COMMAND       0x00000040

    ULONG DataTransferLength;
} ATA_DEVICE_REQUEST, *PATA_DEVICE_REQUEST;

typedef struct _HW_DEVICE_UNIT
{
    /* Public data, must be the first member */
    DEVICE_UNIT P;

    IDE_REGISTERS Registers;
    ULONG BytesToTransfer;
    PUCHAR DataBuffer;
    ULONG DrqByteCount;
    ULONG MaximumTransferLength;
    UCHAR DeviceNumber;
    UCHAR DeviceSelect;
    UCHAR CdbSize;
    UCHAR MultiSectorTransfer;
    union
    {
        IDENTIFY_DEVICE_DATA IdentifyDeviceData;
        IDENTIFY_PACKET_DATA IdentifyPacketData;
    };
} HW_DEVICE_UNIT, *PHW_DEVICE_UNIT;

FORCEINLINE
BOOLEAN
AtaDevIsIdentifyDataValid(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    ULONG i;
    UCHAR Crc;

    /* Bits 0:8 of word 255 */
    if (IdentifyData->Signature != 0xA5)
    {
        /* The integrity word is missing, assume the data provided by the device is valid */
        return TRUE;
    }

    /* Verify the checksum */
    Crc = 0;
    for (i = 0; i < sizeof(*IdentifyData); ++i)
    {
        Crc += ((PUCHAR)IdentifyData)[i];
    }

    return (Crc == 0);
}

FORCEINLINE
UCHAR
AtaDevCdbSizeInWords(
    _In_ PIDENTIFY_PACKET_DATA IdentifyPacketData)
{
    /* Bits 0:2 of word 0 */
    return (IdentifyPacketData->GeneralConfiguration.PacketType != 0) ? 8 : 6;
}

FORCEINLINE
BOOLEAN
AtaDevHasLbaTranslation(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Bit 9 of word 49 */
    return IdentifyData->Capabilities.LbaSupported;
}

FORCEINLINE
ULONG
AtaDevUserAddressableSectors28Bit(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Words 60-61 */
    return IdentifyData->UserAddressableSectors;
}

FORCEINLINE
ULONG64
AtaDevUserAddressableSectors48Bit(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Words 100-103 */
    return ((ULONG64)IdentifyData->Max48BitLBA[1] << 32) | IdentifyData->Max48BitLBA[0];
}

FORCEINLINE
BOOLEAN
AtaDevHas48BitAddressFeature(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    /* Word 83: 15 = 0, 14 = 1 */
   if (IdentifyData->CommandSetSupport.WordValid83 == 1)
   {
        /* Bit 10 of word 83 */
        return IdentifyData->CommandSetSupport.BigLba;
   }

   return FALSE;
}

FORCEINLINE
BOOLEAN
AtaDevIsCurrentGeometryValid(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    return ((IdentifyData->TranslationFieldsValid & 1) &&
            (IdentifyData->NumberOfCurrentCylinders != 0) &&
            (IdentifyData->NumberOfCurrentCylinders <= 63) &&
            (IdentifyData->NumberOfCurrentHeads != 0) &&
            (IdentifyData->NumberOfCurrentHeads <= 16) &&
            (IdentifyData->CurrentSectorsPerTrack != 0));
}

FORCEINLINE
VOID
AtaDevDefaultChsTranslation(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData,
    _Out_ PUSHORT Cylinders,
    _Out_ PUSHORT Heads,
    _Out_ PUSHORT SectorsPerTrack)
{
    /* Word 1 */
    *Cylinders = IdentifyData->NumCylinders;

    /* Word 3 */
    *Heads = IdentifyData->NumHeads;

    /* Word 6 */
    *SectorsPerTrack = IdentifyData->NumSectorsPerTrack;
}

FORCEINLINE
VOID
AtaDevCurrentChsTranslation(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData,
    _Out_ PUSHORT Cylinders,
    _Out_ PUSHORT Heads,
    _Out_ PUSHORT SectorsPerTrack)
{
    /* Word 54 */
    *Cylinders = IdentifyData->NumberOfCurrentCylinders;

    /* Word 55 */
    *Heads = IdentifyData->NumberOfCurrentHeads;

    /* Word 55 */
    *SectorsPerTrack = IdentifyData->CurrentSectorsPerTrack;
}

FORCEINLINE
UCHAR
AtaDevCurrentSectorsPerDrq(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    UCHAR MultiSectorCurrent;

    /* Bit 8 of word 59 */
    if (!(IdentifyData->MultiSectorSettingValid))
        return 0;

    /* The word 59 should be a power of 2 */
    MultiSectorCurrent = IdentifyData->CurrentMultiSectorSetting;

    if ((MultiSectorCurrent > 0) && ((MultiSectorCurrent & (MultiSectorCurrent - 1)) == 0))
        return MultiSectorCurrent;

    return 0;
}

FORCEINLINE
UCHAR
AtaDevMaximumSectorsPerDrq(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    UCHAR MultiSectorMax;

    /* The word 47 should be a power of 2 */
    MultiSectorMax = IdentifyData->MaximumBlockTransfer;

    if ((MultiSectorMax > 0) && ((MultiSectorMax & (MultiSectorMax - 1)) == 0))
        return MultiSectorMax;

    return 0;
}

FORCEINLINE
ULONG
AtaDevBytesPerLogicalSector(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData)
{
    ULONG WordCount;

    /* Word 106: 15 = 0, 14 = 1, 12 = 1 */
    if (IdentifyData->PhysicalLogicalSectorSize.Reserved1 == 1 &&
        IdentifyData->PhysicalLogicalSectorSize.LogicalSectorLongerThan256Words)
    {
        /* Words 116-117 */
        WordCount = IdentifyData->WordsPerLogicalSector[0];
        WordCount |= (ULONG)IdentifyData->WordsPerLogicalSector[1] << 16;
    }
    else
    {
        /* 256 words = 512 bytes */
        WordCount = 256;
    }

    return WordCount * sizeof(USHORT);
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
