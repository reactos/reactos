/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ATA/ATAPI polled I/O driver.
 * COPYRIGHT:   Copyright 2019 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <hwide.h>

/* DDK */
#include <ata.h>
#include <scsi.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* GLOBALS ********************************************************************/

#define TAG_ATA_DEVICE 'DatA'
#define ATAPI_PACKET_SIZE(IdentifyData) (IdentifyData.AtapiCmdSize ? 16 : 12)

/* Used in WaitForFlags() and should be 31 seconds (31e5), but it's too much for polled I/O */
#define ATA_STATUS_TIMEOUT 36000
#define ATA_READ_TIMEOUT   4e5

#define AtaWritePort(Channel, Port, Data) \
    WRITE_PORT_UCHAR(UlongToPtr(BaseArray[(Channel)] + (Port)), (Data))

#define AtaReadPort(Channel, Port) \
    READ_PORT_UCHAR(UlongToPtr(BaseArray[(Channel)] + (Port)))

#define AtaWriteBuffer(Channel, Buffer, Count) \
    WRITE_PORT_BUFFER_USHORT(UlongToPtr(BaseArray[(Channel)] + IDX_IO1_o_Data), \
                             (PUSHORT)(Buffer), (Count)/sizeof(USHORT))

#define AtaReadBuffer(Channel, Buffer, Count) \
    READ_PORT_BUFFER_USHORT(UlongToPtr(BaseArray[(Channel)] + IDX_IO1_i_Data), \
                            (PUSHORT)(Buffer), (Count)/sizeof(USHORT))

/* IDE/ATA Channels base - Primary, Secondary, Tertiary, Quaternary */
static const ULONG BaseArray[] =
{
#if defined(SARCH_XBOX)
    0x1F0
#elif defined(SARCH_PC98)
    0x640, 0x640
#else
    0x1F0, 0x170, 0x1E8, 0x168
#endif
};

#define MAX_CHANNELS RTL_NUMBER_OF(BaseArray)
#define MAX_DEVICES  2 /* Master/Slave */

static PDEVICE_UNIT Units[MAX_CHANNELS * MAX_DEVICES];

/* PRIVATE PROTOTYPES *********************************************************/

static
BOOLEAN
WaitForFlags(
    IN UCHAR Channel,
    IN UCHAR Flags,
    IN UCHAR ExpectedValue,
    IN ULONG Timeout
);

static
BOOLEAN
SelectDevice(
    IN UCHAR Channel,
    IN UCHAR DeviceNumber
);

static
BOOLEAN
IdentifyDevice(
    IN UCHAR Channel,
    IN UCHAR DeviceNumber,
    OUT PDEVICE_UNIT *DeviceUnit
);

static
BOOLEAN
AtapiReadyCheck(
    IN OUT PDEVICE_UNIT DeviceUnit
);

static
BOOLEAN
AtapiReadLogicalSectorLBA(
    IN PDEVICE_UNIT DeviceUnit,
    IN ULONGLONG SectorNumber,
    OUT PVOID Buffer
);

static
BOOLEAN
AtaReadLogicalSectorsLBA(
    IN PDEVICE_UNIT DeviceUnit,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer
);

/* FUNCTIONS ******************************************************************/

/* Don't call this before running the system timer calibration and MM initialization */
BOOLEAN
AtaInit(OUT PUCHAR DetectedCount)
{
    UCHAR Channel, DeviceNumber;
    PDEVICE_UNIT DeviceUnit = NULL;

    TRACE("AtaInit()\n");

    *DetectedCount = 0;

    RtlZeroMemory(&Units, sizeof(Units));

    /* Detect and enumerate ATA/ATAPI devices */
    for (Channel = 0; Channel < MAX_CHANNELS; ++Channel)
    {
        for (DeviceNumber = 0; DeviceNumber < MAX_DEVICES; ++DeviceNumber)
        {
            if (IdentifyDevice(Channel, DeviceNumber, &DeviceUnit))
            {
                Units[(*DetectedCount)++] = DeviceUnit;
            }
        }
    }

    return (*DetectedCount > 0);
}

VOID
AtaFree(VOID)
{
    UCHAR i;

    for (i = 0; i < RTL_NUMBER_OF(Units); ++i)
    {
        if (Units[i])
            FrLdrTempFree(Units[i], TAG_ATA_DEVICE);
    }
}

PDEVICE_UNIT
AtaGetDevice(IN UCHAR UnitNumber)
{
    if (UnitNumber < RTL_NUMBER_OF(Units))
        return Units[UnitNumber];
    else
        return NULL;
}

BOOLEAN
AtaAtapiReadLogicalSectorsLBA(
    IN OUT PDEVICE_UNIT DeviceUnit,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    UCHAR RetryCount;
    BOOLEAN Success;

    if (DeviceUnit == NULL || SectorCount == 0)
        return FALSE;

    if (DeviceUnit->Flags & ATA_DEVICE_ATAPI)
    {
        if ((DeviceUnit->Flags & ATA_DEVICE_NO_MEDIA) || (DeviceUnit->Flags & ATA_DEVICE_NOT_READY))
        {
            /* Retry 4 times */
            for (RetryCount = 0; RetryCount < 4; ++RetryCount)
            {
                /* Make the device ready */
                if (AtapiReadyCheck(DeviceUnit))
                    break;
            }
            if (RetryCount >= 4)
            {
                ERR("AtaAtapiReadLogicalSectorsLBA(): Device not ready.\n");
                return FALSE;
            }
        }
        if (SectorNumber + SectorCount > DeviceUnit->TotalSectors + 1)
        {
            ERR("AtaAtapiReadLogicalSectorsLBA(): Attempt to read more than there is to read.\n");
            return FALSE;
        }

        while (SectorCount > 0)
        {
            /* Read a single sector */
            Success = AtapiReadLogicalSectorLBA(DeviceUnit, SectorNumber, Buffer);
            if (!Success)
                return FALSE;

            --SectorCount;
            ++SectorNumber;
            Buffer = (PVOID)((ULONG_PTR)Buffer + DeviceUnit->SectorSize);
        }
    }
    else
    {
        /* Retry 3 times */
        for (RetryCount = 0; RetryCount < 3; ++RetryCount)
        {
            /* Read a multiple sectors */
            Success = AtaReadLogicalSectorsLBA(DeviceUnit, SectorNumber, SectorCount, Buffer);
            if (Success)
                return TRUE;
        }
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
AtaReadLogicalSectorsLBA(
    IN PDEVICE_UNIT DeviceUnit,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    UCHAR Command;
    ULONG ChsTemp;
    USHORT Cylinder;
    UCHAR Head;
    UCHAR Sector;
    ULONG BlockCount;
    ULONG RemainingBlockCount;
    ULONGLONG Lba;
    BOOLEAN UseLBA48;

    UseLBA48 = (DeviceUnit->Flags & ATA_DEVICE_LBA48) &&
                (((SectorNumber + SectorCount) >= UINT64_C(0x0FFFFF80)) || SectorCount > 256);

    while (SectorCount > 0)
    {
    /* Prevent sector count overflow, divide it into maximum possible chunks and loop each one */
    if (UseLBA48)
        BlockCount = min(SectorCount, USHRT_MAX);
    else
        BlockCount = min(SectorCount, UCHAR_MAX);

    /* Convert LBA into a format CHS if needed */
    if (DeviceUnit->Flags & ATA_DEVICE_CHS)
    {
        ChsTemp = DeviceUnit->IdentifyData.SectorsPerTrack * DeviceUnit->IdentifyData.NumberOfHeads;
        if (ChsTemp)
        {
            Cylinder = SectorNumber / ChsTemp;
            Head = (SectorNumber % ChsTemp) / DeviceUnit->IdentifyData.SectorsPerTrack;
            Sector = (SectorNumber % DeviceUnit->IdentifyData.SectorsPerTrack) + 1;
        }
        else
        {
            Cylinder = 0;
            Head = 0;
            Sector = 1;
        }
        Lba = (Sector & 0xFF) | ((Cylinder & 0xFFFFF) << 8) | ((Head & 0x0F) << 24);
    }
    else
    {
        Lba = SectorNumber;
    }

    /* Select the drive */
    if (!SelectDevice(DeviceUnit->Channel, DeviceUnit->DeviceNumber))
        return FALSE;

    /* Disable interrupts */
#ifndef SARCH_PC98
    AtaWritePort(DeviceUnit->Channel, IDX_IO2_o_AltStatus, IDE_DC_DISABLE_INTERRUPTS);
    StallExecutionProcessor(1);
#endif

    if (UseLBA48)
    {
        /* FIFO */
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_Feature, 0);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_Feature, ATA_PIO);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_BlockCount, (BlockCount & 0xFF) >> 8);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_BlockCount, BlockCount & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_BlockNumber, (Lba >> 24) & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_BlockNumber, Lba & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_CylinderLow, (Lba >> 32) & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_CylinderLow, (Lba >> 8) & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_CylinderHigh, (Lba >> 40) & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_CylinderHigh, (Lba >> 16) & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_DriveSelect,
                     IDE_USE_LBA | (DeviceUnit->DeviceNumber ? IDE_DRIVE_2 : IDE_DRIVE_1));
        Command = IDE_COMMAND_READ_EXT;
    }
    else
    {
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_Feature, ATA_PIO);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_BlockCount, BlockCount & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_BlockNumber, Lba & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_CylinderLow, (Lba >> 8) & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_CylinderHigh, (Lba >> 16) & 0xFF);
        AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_DriveSelect,
                     ((Lba >> 24) & 0x0F) |
                     (DeviceUnit->Flags & ATA_DEVICE_CHS ? 0x00 : IDE_USE_LBA) |
                     (DeviceUnit->DeviceNumber ? IDE_DRIVE_SELECT_2 : IDE_DRIVE_SELECT_1));
        Command = IDE_COMMAND_READ;
    }

    /* Send read command */
    AtaWritePort(DeviceUnit->Channel, IDX_IO1_o_Command, Command);
    StallExecutionProcessor(5);

    for (RemainingBlockCount = BlockCount; RemainingBlockCount > 0; --RemainingBlockCount)
    {
        /* Wait for ready to transfer data block */
        if (!WaitForFlags(DeviceUnit->Channel, (IDE_STATUS_BUSY | IDE_STATUS_DRQ | IDE_STATUS_ERROR),
                          IDE_STATUS_DRQ, ATA_READ_TIMEOUT))
        {
            ERR("AtaReadLogicalSectorsLBA() failed. Status: 0x%02x, Error: 0x%02x\n",
                AtaReadPort(DeviceUnit->Channel, IDX_IO1_i_Status),
                AtaReadPort(DeviceUnit->Channel, IDX_IO1_i_Error));
            return FALSE;
        }

        /* Transfer the data block */
        AtaReadBuffer(DeviceUnit->Channel, Buffer, DeviceUnit->SectorSize);

        Buffer = (PVOID)((ULONG_PTR)Buffer + DeviceUnit->SectorSize);
    }

    SectorNumber += BlockCount;
    SectorCount -= BlockCount;
    }

    return TRUE;
}

static
BOOLEAN
AtaSendAtapiPacket(
    IN UCHAR Channel,
    IN PUCHAR AtapiPacket,
    IN UCHAR PacketSize,
    IN USHORT ByteCount)
{
    /* No DRQ for TEST UNIT READY */
    BOOLEAN NoData = (AtapiPacket[0] == SCSIOP_TEST_UNIT_READY);
    UCHAR ExpectedFlags = NoData ? IDE_STATUS_DRDY : (IDE_STATUS_DRQ | IDE_STATUS_DRDY);

    /* PIO mode */
    AtaWritePort(Channel, IDX_ATAPI_IO1_o_Feature, ATA_PIO);

    /* Maximum byte count that is to be transferred */
    AtaWritePort(Channel, IDX_ATAPI_IO1_o_ByteCountLow, ByteCount & 0xFF);
    AtaWritePort(Channel, IDX_ATAPI_IO1_o_ByteCountHigh, (ByteCount >> 8) & 0xFF);

    /* Prepare to transfer a device command via a command packet */
    AtaWritePort(Channel, IDX_ATAPI_IO1_o_Command, IDE_COMMAND_ATAPI_PACKET);
    StallExecutionProcessor(50);
    if (!WaitForFlags(Channel, (IDE_STATUS_BUSY | IDE_STATUS_DRQ | IDE_STATUS_ERROR),
                      IDE_STATUS_DRQ, NoData ? ATA_STATUS_TIMEOUT : ATA_READ_TIMEOUT))
    {
        ERR("AtaSendAtapiPacket(0x%x) failed. A device error occurred Status: 0x%02x, Error: 0x%02x\n",
            AtapiPacket[0], AtaReadPort(Channel, IDX_ATAPI_IO1_i_Status), AtaReadPort(Channel, IDX_ATAPI_IO1_i_Error));
        return FALSE;
    }

    /* Command packet transfer */
    AtaWriteBuffer(Channel, AtapiPacket, PacketSize);
    if (!WaitForFlags(Channel, (IDE_STATUS_BUSY | IDE_STATUS_DRQ | IDE_STATUS_DRDY | IDE_STATUS_ERROR),
                      ExpectedFlags, NoData ? ATA_STATUS_TIMEOUT : ATA_READ_TIMEOUT))
    {
        TRACE("AtaSendAtapiPacket(0x%x) failed. An execution error occurred Status: 0x%02x, Error: 0x%02x\n",
              AtapiPacket[0], AtaReadPort(Channel, IDX_ATAPI_IO1_i_Status), AtaReadPort(Channel, IDX_ATAPI_IO1_i_Error));
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
AtapiReadLogicalSectorLBA(
    IN PDEVICE_UNIT DeviceUnit,
    IN ULONGLONG SectorNumber,
    OUT PVOID Buffer)
{
    UCHAR AtapiPacket[16];
    USHORT DataSize;
    BOOLEAN Success;

    /* Select the drive */
    if (!SelectDevice(DeviceUnit->Channel, DeviceUnit->DeviceNumber))
        return FALSE;

    /* Disable interrupts */
    AtaWritePort(DeviceUnit->Channel, IDX_IO2_o_AltStatus, IDE_DC_DISABLE_INTERRUPTS);
    StallExecutionProcessor(1);

    /* Send the SCSI READ command */
    RtlZeroMemory(&AtapiPacket, sizeof(AtapiPacket));
#if defined(SARCH_PC98)
    AtapiPacket[0] = SCSIOP_READ;
    AtapiPacket[8] = 1;
    AtapiPacket[9] = 0;
#else
    AtapiPacket[0] = SCSIOP_READ12;
    AtapiPacket[8] = 0;
    AtapiPacket[9] = 1;
#endif
    AtapiPacket[2] = (SectorNumber >> 24) & 0xFF;
    AtapiPacket[3] = (SectorNumber >> 16) & 0xFF;
    AtapiPacket[4] = (SectorNumber >> 8) & 0xFF;
    AtapiPacket[5] = SectorNumber & 0xFF;
    Success = AtaSendAtapiPacket(DeviceUnit->Channel,
                                 AtapiPacket,
                                 ATAPI_PACKET_SIZE(DeviceUnit->IdentifyData),
                                 DeviceUnit->SectorSize);
    if (!Success)
    {
        ERR("AtapiReadLogicalSectorLBA() failed. A read error occurred.\n");
        return FALSE;
    }

    DataSize = (AtaReadPort(DeviceUnit->Channel, IDX_ATAPI_IO1_i_ByteCountHigh) << 8) |
                AtaReadPort(DeviceUnit->Channel, IDX_ATAPI_IO1_i_ByteCountLow);

    /* Transfer the data block */
    AtaReadBuffer(DeviceUnit->Channel, Buffer, DataSize);

    return TRUE;
}

static
VOID
AtapiCapacityDetect(
    IN PDEVICE_UNIT DeviceUnit,
    OUT PULONGLONG TotalSectors,
    OUT PULONG SectorSize)
{
    UCHAR AtapiPacket[16];
    UCHAR AtapiCapacity[8];

    /* Send the SCSI READ CAPACITY(10) command */
    RtlZeroMemory(&AtapiPacket, sizeof(AtapiPacket));
    AtapiPacket[0] = SCSIOP_READ_CAPACITY;
    if (AtaSendAtapiPacket(DeviceUnit->Channel, AtapiPacket, ATAPI_PACKET_SIZE(DeviceUnit->IdentifyData), 8))
    {
        AtaReadBuffer(DeviceUnit->Channel, &AtapiCapacity, 8);

        *TotalSectors = (AtapiCapacity[0] << 24) | (AtapiCapacity[1] << 16) |
                        (AtapiCapacity[2] << 8) | AtapiCapacity[3];

        *SectorSize = (AtapiCapacity[4] << 24) | (AtapiCapacity[5] << 16) |
                      (AtapiCapacity[6] << 8) | AtapiCapacity[7];
    }
    else
    {
        *TotalSectors = 0;
        *SectorSize = 0;
    }
}

static
BOOLEAN
AtapiReadyCheck(IN OUT PDEVICE_UNIT DeviceUnit)
{
    UCHAR AtapiPacket[16];
    UCHAR DummyData[MAXIMUM_CDROM_SIZE];
    SENSE_DATA SenseData;
    BOOLEAN Success;

    /* Select the drive */
    if (!SelectDevice(DeviceUnit->Channel, DeviceUnit->DeviceNumber))
        return FALSE;

    /* Send the SCSI TEST UNIT READY command */
    RtlZeroMemory(&AtapiPacket, sizeof(AtapiPacket));
    AtapiPacket[0] = SCSIOP_TEST_UNIT_READY;
    AtaSendAtapiPacket(DeviceUnit->Channel,
                       AtapiPacket,
                       ATAPI_PACKET_SIZE(DeviceUnit->IdentifyData),
                       0);

    /* Send the SCSI REQUEST SENSE command */
    RtlZeroMemory(&AtapiPacket, sizeof(AtapiPacket));
    RtlZeroMemory(&SenseData, SENSE_BUFFER_SIZE);
    AtapiPacket[0] = SCSIOP_REQUEST_SENSE;
    AtapiPacket[4] = SENSE_BUFFER_SIZE;
    Success = AtaSendAtapiPacket(DeviceUnit->Channel,
                                 AtapiPacket,
                                 ATAPI_PACKET_SIZE(DeviceUnit->IdentifyData),
                                 SENSE_BUFFER_SIZE);
    if (!Success)
        return FALSE;

    AtaReadBuffer(DeviceUnit->Channel, &SenseData, SENSE_BUFFER_SIZE);
    TRACE("SK 0x%x, ASC 0x%x, ASCQ 0x%x\n",
          SenseData.SenseKey,
          SenseData.AdditionalSenseCode,
          SenseData.AdditionalSenseCodeQualifier);

    if (SenseData.SenseKey == SCSI_SENSE_NOT_READY)
    {
        if (SenseData.AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY)
        {
            switch (SenseData.AdditionalSenseCodeQualifier)
            {
                case SCSI_SENSEQ_BECOMING_READY:
                    /* Wait until the CD is spun up */
                    StallExecutionProcessor(4e6);
                    return FALSE;

                case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:
                    /* The drive needs to be spun up, send the SCSI READ TOC command */
                    RtlZeroMemory(&AtapiPacket, sizeof(AtapiPacket));
                    AtapiPacket[0] = SCSIOP_READ_TOC;
                    AtapiPacket[7] = (MAXIMUM_CDROM_SIZE << 8) & 0xFF;
                    AtapiPacket[8] = MAXIMUM_CDROM_SIZE & 0xFF;
                    AtapiPacket[9] = READ_TOC_FORMAT_SESSION << 6;
                    Success = AtaSendAtapiPacket(DeviceUnit->Channel,
                                                 AtapiPacket,
                                                 ATAPI_PACKET_SIZE(DeviceUnit->IdentifyData),
                                                 MAXIMUM_CDROM_SIZE);
                    if (!Success)
                        return FALSE;

                    AtaReadBuffer(DeviceUnit->Channel, &DummyData, MAXIMUM_CDROM_SIZE);
                    /* fall through */

                default:
                    DeviceUnit->Flags &= ~ATA_DEVICE_NOT_READY;
                    return FALSE;

            }
        }
        else if (SenseData.AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE)
        {
            DeviceUnit->Flags |= ATA_DEVICE_NO_MEDIA;
            return FALSE;
        }
    }
    else
    {
        DeviceUnit->Flags &= ~ATA_DEVICE_NOT_READY;
    }

    if (DeviceUnit->Flags & ATA_DEVICE_NO_MEDIA)
    {
        /* Detect a medium's capacity */
        AtapiCapacityDetect(DeviceUnit, &DeviceUnit->TotalSectors, &DeviceUnit->SectorSize);

        /* If nothing was returned, reset to defaults */
        if (DeviceUnit->SectorSize == 0)
            DeviceUnit->SectorSize = 2048;
        if (DeviceUnit->TotalSectors == 0)
            DeviceUnit->TotalSectors = 0xFFFFFFFF;

        DeviceUnit->Flags &= ~ATA_DEVICE_NO_MEDIA;
    }

    return TRUE;
}

static
BOOLEAN
WaitForFlags(
    IN UCHAR Channel,
    IN UCHAR Flags,
    IN UCHAR ExpectedValue,
    IN ULONG Timeout)
{
    ASSERT(Timeout != 0);

    while (Timeout--)
    {
        if ((AtaReadPort(Channel, IDX_IO1_i_Status) & Flags) == ExpectedValue)
            return TRUE;
        else
            StallExecutionProcessor(10);
    }
    return FALSE;
}

static
VOID
AtaHardReset(IN UCHAR Channel)
{
    TRACE("AtaHardReset(Controller %d)\n", Channel);

    AtaWritePort(Channel, IDX_IO2_o_Control, IDE_DC_RESET_CONTROLLER);
    StallExecutionProcessor(200000);
    AtaWritePort(Channel, IDX_IO2_o_Control, IDE_DC_REENABLE_CONTROLLER);
    StallExecutionProcessor(1);
}

static
BOOLEAN
SelectDevice(IN UCHAR Channel, IN UCHAR DeviceNumber)
{
#if defined(SARCH_PC98)
    /* Select IDE Channel */
    WRITE_PORT_UCHAR((PUCHAR)IDE_IO_o_BankSelect, Channel);
    StallExecutionProcessor(1);
#endif

    AtaWritePort(Channel, IDX_IO1_o_DriveSelect,
                 DeviceNumber ? IDE_DRIVE_SELECT_2 : IDE_DRIVE_SELECT_1);
    StallExecutionProcessor(500);
    if (!WaitForFlags(Channel, (IDE_STATUS_BUSY | IDE_STATUS_DRQ), 0, ATA_STATUS_TIMEOUT))
    {
        TRACE("SelectDevice() failed. Device(%d:%d) is busy.\n", Channel, DeviceNumber);
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
IdentifyDevice(
    IN UCHAR Channel,
    IN UCHAR DeviceNumber,
    OUT PDEVICE_UNIT *DeviceUnit)
{
    UCHAR SignatureLow, SignatureHigh;
    IDENTIFY_DATA Id;
    INQUIRYDATA AtapiInquiry;
    BOOLEAN Success;
    UCHAR AtapiPacket[16];
    ULONG i;
    ULONG SectorSize;
    ULONGLONG TotalSectors;
    USHORT Flags = 0;

    TRACE("IdentifyDevice() Channel = %x, Device = %x, BaseIoAddress = 0x%x\n",
          Channel, DeviceNumber, BaseArray[Channel]);

    /* Reset the controller */
    AtaHardReset(Channel);

    /* Select the drive */
    if (!SelectDevice(Channel, DeviceNumber))
        goto Failure;

    /* Send the IDENTIFY DEVICE command */
    AtaWritePort(Channel, IDX_IO1_o_Command, IDE_COMMAND_IDENTIFY);
    StallExecutionProcessor(50);
    if (WaitForFlags(Channel, (IDE_STATUS_BUSY | IDE_STATUS_DRQ | IDE_STATUS_ERROR),
                     IDE_STATUS_DRQ, ATA_STATUS_TIMEOUT))
    {
        SignatureLow = AtaReadPort(Channel, IDX_IO1_i_CylinderLow);
        SignatureHigh = AtaReadPort(Channel, IDX_IO1_i_CylinderHigh);
        TRACE("IdentifyDevice(): SignatureLow = 0x%x, SignatureHigh = 0x%x\n", SignatureLow, SignatureHigh);
        if (SignatureLow == 0x00 && SignatureHigh == 0x00)
        {
            /* This is PATA */
            TRACE("IdentifyDevice(): Found PATA device at %d:%d\n", Channel, DeviceNumber);
            goto Identify;
        }
    }

    /* If not PATA, ATAPI maybe? Send the IDENTIFY PACKET DEVICE command */
    AtaWritePort(Channel, IDX_ATAPI_IO1_o_Command, IDE_COMMAND_ATAPI_IDENTIFY);
    StallExecutionProcessor(500);
    SignatureLow = AtaReadPort(Channel, IDX_IO1_i_CylinderLow);
    SignatureHigh = AtaReadPort(Channel, IDX_IO1_i_CylinderHigh);
    TRACE("IdentifyDevice(): SignatureLow = 0x%x, SignatureHigh = 0x%x\n", SignatureLow, SignatureHigh);
    /* Check for devices that implements the PACKET Command feature */
    if (SignatureLow == ATAPI_MAGIC_LSB && SignatureHigh == ATAPI_MAGIC_MSB)
    {
        /* This is ATAPI */
        Flags |= ATA_DEVICE_ATAPI | ATA_DEVICE_LBA | ATA_DEVICE_NOT_READY;
        TRACE("IdentifyDevice(): Found ATAPI device at %d:%d\n", Channel, DeviceNumber);
        goto Identify;
    }
    else
    {
        goto Failure;
    }

Identify:
    /* Receive parameter information from the device */
    AtaReadBuffer(Channel, &Id, IDENTIFY_DATA_SIZE);

    /* Swap byte order of the ASCII data */
    for (i = 0; i < RTL_NUMBER_OF(Id.SerialNumber); ++i)
        Id.SerialNumber[i] = RtlUshortByteSwap(Id.SerialNumber[i]);

    for (i = 0; i < RTL_NUMBER_OF(Id.FirmwareRevision); ++i)
        Id.FirmwareRevision[i] = RtlUshortByteSwap(Id.FirmwareRevision[i]);

    for (i = 0; i < RTL_NUMBER_OF(Id.ModelNumber); ++i)
        Id.ModelNumber[i] = RtlUshortByteSwap(Id.ModelNumber[i]);

    TRACE("S/N %.*s\n", sizeof(Id.SerialNumber), Id.SerialNumber);
    TRACE("FR %.*s\n", sizeof(Id.FirmwareRevision), Id.FirmwareRevision);
    TRACE("MN %.*s\n", sizeof(Id.ModelNumber), Id.ModelNumber);

    /* Detect the type of device */
    if (Flags & ATA_DEVICE_ATAPI)
    {
        /* Send the SCSI INQUIRY command */
        RtlZeroMemory(&AtapiPacket, sizeof(AtapiPacket));
        AtapiPacket[0] = SCSIOP_INQUIRY;
        AtapiPacket[4] = INQUIRYDATABUFFERSIZE;
        Success = AtaSendAtapiPacket(Channel, AtapiPacket, ATAPI_PACKET_SIZE(Id), INQUIRYDATABUFFERSIZE);
        if (!Success)
            goto Failure;

        AtaReadBuffer(Channel, &AtapiInquiry, INQUIRYDATABUFFERSIZE);
        if (AtapiInquiry.DeviceType != READ_ONLY_DIRECT_ACCESS_DEVICE)
        {
            TRACE("IdentifyDevice(): Unsupported device type 0x%x!\n", AtapiInquiry.DeviceType);
            goto Failure;
        }
    }

    /* Allocate a new device unit structure */
    *DeviceUnit = FrLdrTempAlloc(sizeof(DEVICE_UNIT), TAG_ATA_DEVICE);
    if (*DeviceUnit == NULL)
    {
        ERR("Failed to allocate device unit!\n");
        return FALSE;
    }

    RtlZeroMemory(*DeviceUnit, sizeof(DEVICE_UNIT));
    (*DeviceUnit)->Channel = Channel;
    (*DeviceUnit)->DeviceNumber = DeviceNumber;
    (*DeviceUnit)->IdentifyData = Id;

    /* Detect a medium's capacity */
    if (Flags & ATA_DEVICE_ATAPI)
    {
        AtapiCapacityDetect(*DeviceUnit, &TotalSectors, &SectorSize);
        if (SectorSize == 0 || TotalSectors == 0)
        {
            /* It's ok and can be used to show alert like "Please insert the CD" */
            TRACE("No media found.\n");
            Flags |= ATA_DEVICE_NO_MEDIA;
        }
    }
    else
    {
        if (Id.SupportLba || (Id.MajorRevision && Id.UserAddressableSectors))
        {
            if (Id.FeaturesSupport.Address48)
            {
                TRACE("Using LBA48 addressing mode.\n");
                Flags |= ATA_DEVICE_LBA48 | ATA_DEVICE_LBA;
                TotalSectors = Id.UserAddressableSectors48;
            }
            else
            {
                TRACE("Using LBA28 addressing mode.\n");
                Flags |= ATA_DEVICE_LBA;
                TotalSectors = Id.UserAddressableSectors;
            }

            /* LBA ATA drives always have a sector size of 512 */
            SectorSize = 512;
        }
        else
        {
            TRACE("Using CHS addressing mode.\n");
            Flags |= ATA_DEVICE_CHS;

            if (Id.UnformattedBytesPerSector == 0)
            {
                SectorSize = 512;
            }
            else
            {
                for (i = 1 << 15; i > 0; i >>= 1)
                {
                    if ((Id.UnformattedBytesPerSector & i) != 0)
                    {
                        SectorSize = i;
                        break;
                    }
                }
            }
            TotalSectors = Id.NumberOfCylinders * Id.NumberOfHeads * Id.SectorsPerTrack;
        }
    }
    TRACE("Sector size %d ; Total sectors %I64d\n", SectorSize, TotalSectors);

    (*DeviceUnit)->Flags = Flags;
    (*DeviceUnit)->SectorSize = SectorSize;
    (*DeviceUnit)->TotalSectors = TotalSectors;
    if (Flags & ATA_DEVICE_ATAPI)
    {
        (*DeviceUnit)->Cylinders = 0xFFFFFFFF;
        (*DeviceUnit)->Heads = 0xFFFFFFFF;
        (*DeviceUnit)->Sectors = 0xFFFFFFFF;
    }
    else
    {
        (*DeviceUnit)->Cylinders = Id.NumberOfCylinders;
        (*DeviceUnit)->Heads = Id.NumberOfHeads;
        (*DeviceUnit)->Sectors = Id.SectorsPerTrack;
    }

#if DBG
    DbgDumpBuffer(DPRINT_DISK, &Id, IDENTIFY_DATA_SIZE);
#endif

    TRACE("IdentifyDevice() done.\n");
    return TRUE;

Failure:
    TRACE("IdentifyDevice() done. No device present at %d:%d\n", Channel, DeviceNumber);
    return FALSE;
}
