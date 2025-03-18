/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ATA/ATAPI programmed I/O driver.
 * COPYRIGHT:   Copyright 2019-2025 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* DDK */
#include <ata.h>
#include <scsi.h>

#include <hwide.h>
#include "hwidep.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

/* GLOBALS ********************************************************************/

/** IDE Channels base - Primary, Secondary, Tertiary, Quaternary */
static const IDE_REG AtapChannelBaseArray[] =
{
#if defined(SARCH_PC98)
    0x640
#elif defined(SARCH_XBOX)
    0x1F0
#else
    0x1F0, 0x170, 0x1E8, 0x168
#endif
};
#define CHANNEL_MAX_CHANNELS    RTL_NUMBER_OF(AtapChannelBaseArray)

static PHW_DEVICE_UNIT AtapUnits[CHANNEL_MAX_CHANNELS * CHANNEL_MAX_DEVICES];

/* PRIVATE FUNCTIONS **********************************************************/

#if defined(ATA_SUPPORT_32_BIT_IO)
static
inline
BOOLEAN
AtapIs32BitIoSupported(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
#if defined(ATA_ALWAYS_DO_32_BIT_IO)
    return TRUE;
#else
    return !!(DeviceUnit->P.Flags & ATA_DEVICE_FLAG_IO32);
#endif
}
#endif

static
VOID
AtapSelectDevice(
    _In_ PIDE_REGISTERS Registers,
    _In_range_(0, 3) UCHAR DeviceNumber)
{
#if defined(SARCH_PC98)
    /* Select the primary (0) or secondary (1) IDE channel */
    ATA_WRITE(0x432, DeviceNumber >> 1);
#endif

    ATA_WRITE(Registers->Device, (DEV_SLAVE(DeviceNumber) << 4) | IDE_DRIVE_SELECT);
    ATA_IO_WAIT();
}

static
BOOLEAN
AtapWaitForNotBusy(
    _In_ PIDE_REGISTERS Registers,
    _In_range_(>, 0) ULONG Timeout,
    _Out_opt_ PUCHAR Result)
{
    UCHAR IdeStatus;
    ULONG i;

    ASSERT(Timeout != 0);

    for (i = 0; i < Timeout; ++i)
    {
        IdeStatus = ATA_READ(Registers->Status);
        if (!(IdeStatus & IDE_STATUS_BUSY))
        {
            if (Result)
                *Result = IdeStatus;
            return TRUE;
        }

        if (IdeStatus == 0xFF)
            break;

        StallExecutionProcessor(10);
    }

    if (Result)
        *Result = IdeStatus;
    return FALSE;
}

static
BOOLEAN
AtapWaitForIdle(
    _In_ PIDE_REGISTERS Registers,
    _Out_ PUCHAR Result)
{
    UCHAR IdeStatus;
    ULONG i;

    for (i = 0; i < ATA_TIME_DRQ_CLEAR; ++i)
    {
        IdeStatus = ATA_READ(Registers->Status);
        if (!(IdeStatus & (IDE_STATUS_DRQ | IDE_STATUS_BUSY)))
        {
            *Result = IdeStatus;
            return TRUE;
        }

        StallExecutionProcessor(2);
    }

    *Result = IdeStatus;
    return FALSE;
}

static
VOID
AtapSendCdb(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request)
{
#if defined(ATA_SUPPORT_32_BIT_IO)
    if (AtapIs32BitIoSupported(DeviceUnit))
    {
        ATA_WRITE_BLOCK_32(DeviceUnit->Registers.Data,
                           Request->Cdb,
                           DeviceUnit->CdbSize / sizeof(USHORT));
    }
    else
#endif
    {
        ATA_WRITE_BLOCK_16(DeviceUnit->Registers.Data,
                           Request->Cdb,
                           DeviceUnit->CdbSize);
    }

    /*
     * In polled mode (interrupts disabled)
     * the NEC CDR-260 drive clears BSY before updating the interrupt reason register.
     * As a workaround, we will wait for the phase change.
     */
    if (DeviceUnit->P.Flags & ATA_DEVICE_IS_NEC_CDR260)
    {
        ULONG i;

        ATA_IO_WAIT();

        for (i = 0; i < ATA_TIME_PHASE_CHANGE; ++i)
        {
            UCHAR InterruptReason = ATA_READ(DeviceUnit->Registers.InterruptReason);
            if (InterruptReason != ATAPI_INT_REASON_COD)
                break;

            StallExecutionProcessor(10);
        }
    }
}

static
VOID
AtapPioDataIn(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ ULONG ByteCount)
{
    ByteCount = min(ByteCount, DeviceUnit->BytesToTransfer);

#if defined(ATA_SUPPORT_32_BIT_IO)
    if (AtapIs32BitIoSupported(DeviceUnit))
    {
        ATA_READ_BLOCK_32(DeviceUnit->Registers.Data,
                          (PULONG)DeviceUnit->DataBuffer,
                          ByteCount / sizeof(ULONG));
    }
    else
#endif
    {
        ATA_READ_BLOCK_16(DeviceUnit->Registers.Data,
                          (PUSHORT)DeviceUnit->DataBuffer,
                          ByteCount / sizeof(USHORT));
    }

    DeviceUnit->DataBuffer += ByteCount;
    DeviceUnit->BytesToTransfer -= ByteCount;
}

static
BOOLEAN
AtapWaitForRegisterAccess(
    _In_ PIDE_REGISTERS Registers,
    _In_range_(0, 3) UCHAR DeviceNumber)
{
    ULONG i;

    for (i = 0; i < ATA_TIME_RESET_SELECT; ++i)
    {
        /* Select the device again */
        AtapSelectDevice(Registers, DeviceNumber);

        /* Check whether the selection was successful */
        ATA_WRITE(Registers->ByteCountLow, 0xAA);
        ATA_WRITE(Registers->ByteCountLow, 0x55);
        ATA_WRITE(Registers->ByteCountLow, 0xAA);
        if (ATA_READ(Registers->ByteCountLow) == 0xAA)
            return TRUE;

        StallExecutionProcessor(10);
    }

    return FALSE;
}

static
VOID
AtapSoftwareReset(
    _In_ PIDE_REGISTERS Registers)
{
    ATA_WRITE(Registers->Control, IDE_DC_RESET_CONTROLLER | IDE_DC_ALWAYS);
    StallExecutionProcessor(20);
    ATA_WRITE(Registers->Control, IDE_DC_DISABLE_INTERRUPTS | IDE_DC_ALWAYS);
    StallExecutionProcessor(20);
}

static
BOOLEAN
AtapPerformSoftwareReset(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    PIDE_REGISTERS Registers = &DeviceUnit->Registers;

    ERR("Reset device at %X:%u\n", Registers->Data, DeviceUnit->DeviceNumber);

    /* Perform a software reset */
    AtapSoftwareReset(Registers);

    /* The reset will cause the master device to be selected */
    if (DEV_SLAVE(DeviceUnit->DeviceNumber))
    {
        if (!AtapWaitForRegisterAccess(Registers, DeviceUnit->DeviceNumber))
            return FALSE;
    }

    /* Now wait for busy to clear */
    if (!AtapWaitForNotBusy(Registers, ATA_TIME_BUSY_RESET, NULL))
        return FALSE;

    return TRUE;
}

static
UCHAR
AtapProcessAtapiRequest(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR IdeStatus)
{
    UCHAR InterruptReason;

    InterruptReason = ATA_READ(DeviceUnit->Registers.InterruptReason);
    InterruptReason &= ATAPI_INT_REASON_MASK;
    InterruptReason |= IdeStatus & IDE_STATUS_DRQ;

    switch (InterruptReason)
    {
        case ATAPI_INT_REASON_AWAIT_CDB:
        {
            if (!(Request->Flags & REQUEST_FLAG_AWAIT_CDB))
                return ATA_STATUS_RESET;

            Request->Flags &= ~REQUEST_FLAG_AWAIT_CDB;

            AtapSendCdb(DeviceUnit, Request);
            return ATA_STATUS_PENDING;
        }

        case ATAPI_INT_REASON_DATA_IN:
        {
            ULONG ByteCount;

            if (!Request->DataBuffer || (Request->Flags & REQUEST_FLAG_AWAIT_CDB))
                return ATA_STATUS_RESET;

            ByteCount = ATA_READ(DeviceUnit->Registers.ByteCountLow);
            ByteCount |= ATA_READ(DeviceUnit->Registers.ByteCountHigh) << 8;

            AtapPioDataIn(DeviceUnit, ByteCount);
            return ATA_STATUS_PENDING;
        }

        case ATAPI_INT_REASON_STATUS_NEC:
        {
            /* The NEC CDR-260 drive always clears CoD and IO on command completion */
            if (!(DeviceUnit->P.Flags & ATA_DEVICE_IS_NEC_CDR260))
                return ATA_STATUS_RESET;

            __fallthrough;
        }
        case ATAPI_INT_REASON_STATUS:
        {
            if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
                return ATA_STATUS_ERROR;

            break;
        }

        default:
            return ATA_STATUS_RESET;
    }

    return ATA_STATUS_SUCCESS;
}

static
UCHAR
AtapProcessAtaRequest(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR IdeStatus)
{
    /* Check for errors */
    if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
    {
        if (IdeStatus & IDE_STATUS_DRQ)
            return ATA_STATUS_RESET;
        else
            return ATA_STATUS_ERROR;
    }

    /* Read command */
    if (Request->DataBuffer)
    {
        if (!(IdeStatus & IDE_STATUS_DRQ))
            return ATA_STATUS_RESET;

        /* Read the next data block */
        AtapPioDataIn(DeviceUnit, DeviceUnit->DrqByteCount);

        if (DeviceUnit->BytesToTransfer != 0)
            return ATA_STATUS_PENDING;

        /* All data has been transferred, wait for DRQ to clear */
        if (!AtapWaitForIdle(&DeviceUnit->Registers, &IdeStatus))
            return ATA_STATUS_RESET;

        if (IdeStatus & (IDE_STATUS_ERROR | IDE_STATUS_DEVICE_FAULT))
            return ATA_STATUS_ERROR;
    }

    /* Status phase or non-data ATA command */
    return ATA_STATUS_SUCCESS;
}

static
BOOLEAN
AtapProcessRequest(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR IdeStatus)
{
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
        return AtapProcessAtapiRequest(DeviceUnit, Request, IdeStatus);
    else
        return AtapProcessAtaRequest(DeviceUnit, Request, IdeStatus);
}

static
VOID
AtapIssuePacketCommand(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request)
{
    USHORT ByteCount;

    Request->Flags |= REQUEST_FLAG_AWAIT_CDB;

    /*
     * If a larger transfer is attempted, the 16-bit ByteCount register might overflow.
     * In this case we round down the length to the closest multiple of 2.
     */
    ByteCount = min(Request->DataTransferLength, 0xFFFE);

    /* Prepare to transfer a device command */
    ATA_WRITE(DeviceUnit->Registers.ByteCountLow, (UCHAR)(ByteCount >> 0));
    ATA_WRITE(DeviceUnit->Registers.ByteCountHigh, (UCHAR)(ByteCount >> 8));
    ATA_WRITE(DeviceUnit->Registers.Features, IDE_FEATURE_PIO);
    ATA_WRITE(DeviceUnit->Registers.Command, IDE_COMMAND_ATAPI_PACKET);
}

static
VOID
AtapLoadTaskFile(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    ULONG i, BlockCount;

    /* Store the extra information in the second byte of FIFO for 48-bit commands */
    i = (Request->Flags & REQUEST_FLAG_LBA48) ? 2 : 1;

    while (i--)
    {
        ATA_WRITE(DeviceUnit->Registers.Features, TaskFile->Data[i].Feature);
        ATA_WRITE(DeviceUnit->Registers.SectorCount, TaskFile->Data[i].SectorCount);
        ATA_WRITE(DeviceUnit->Registers.LbaLow, TaskFile->Data[i].LowLba);
        ATA_WRITE(DeviceUnit->Registers.LbaMid, TaskFile->Data[i].MidLba);
        ATA_WRITE(DeviceUnit->Registers.LbaHigh, TaskFile->Data[i].HighLba);
    }
    if (Request->Flags & REQUEST_FLAG_SET_DEVICE_REGISTER)
    {
        ATA_WRITE(DeviceUnit->Registers.Device, TaskFile->DriveSelect);
    }
    ATA_WRITE(DeviceUnit->Registers.Command, TaskFile->Command);

    /* Set the byte count per DRQ data block */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE)
        BlockCount = DeviceUnit->MultiSectorTransfer;
    else
        BlockCount = 1;
    DeviceUnit->DrqByteCount = BlockCount * DeviceUnit->P.SectorSize;
}

static
UCHAR
AtapSendCommand(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request)
{
    UCHAR AtaStatus;

    DeviceUnit->BytesToTransfer = Request->DataTransferLength;
    DeviceUnit->DataBuffer = Request->DataBuffer;

    /* Select the device */
    AtapSelectDevice(&DeviceUnit->Registers, DeviceUnit->DeviceNumber);
    if (!AtapWaitForNotBusy(&DeviceUnit->Registers, ATA_TIME_BUSY_SELECT, NULL))
        return ATA_STATUS_RETRY;

    /* Always disable interrupts, otherwise it may lead to problems in underlying BIOS firmware */
    ATA_WRITE(DeviceUnit->Registers.Control, IDE_DC_DISABLE_INTERRUPTS | IDE_DC_ALWAYS);

    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
        AtapIssuePacketCommand(DeviceUnit, Request);
    else
        AtapLoadTaskFile(DeviceUnit, Request);

    while (TRUE)
    {
        UCHAR IdeStatus;

        ATA_IO_WAIT();

        if (!AtapWaitForNotBusy(&DeviceUnit->Registers, ATA_TIME_BUSY_POLL, &IdeStatus))
            return ATA_STATUS_RESET;

        AtaStatus = AtapProcessRequest(DeviceUnit, Request, IdeStatus);
        if (AtaStatus != ATA_STATUS_PENDING)
            break;
    }

    return AtaStatus;
}

static
VOID
AtapAtapiBuildRequestSense(
    _In_ PATA_DEVICE_REQUEST Request,
    _Out_ PSENSE_DATA SenseData)
{
    Request->Flags = REQUEST_FLAG_PACKET_COMMAND;
    Request->DataBuffer = SenseData;
    Request->DataTransferLength = sizeof(*SenseData);
    Request->Cdb[0] = SCSIOP_REQUEST_SENSE;
    Request->Cdb[4] = (UCHAR)Request->DataTransferLength;
}

static
BOOLEAN
AtapAtapiHandleError(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    ATA_DEVICE_REQUEST Request = { 0 };
    SENSE_DATA SenseData;

    AtapAtapiBuildRequestSense(&Request, &SenseData);
    if (AtapSendCommand(DeviceUnit, &Request) != ATA_STATUS_SUCCESS)
        return FALSE;

    ERR("SK %02X, ASC %02X, ASCQ %02X\n",
        SenseData.SenseKey,
        SenseData.AdditionalSenseCode,
        SenseData.AdditionalSenseCodeQualifier);

    return TRUE;
}

static
BOOLEAN
AtapIssueCommand(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG RetryCount;

    for (RetryCount = 0; RetryCount < 3; ++RetryCount)
    {
        UCHAR AtaStatus;

        AtaStatus = AtapSendCommand(DeviceUnit, Request);

        if ((AtaStatus != ATA_STATUS_SUCCESS) &&
            !(Request->Flags & REQUEST_FLAG_IDENTIFY_COMMAND))
        {
            ERR("ATA%s command %02X failed %u %02X:%02X at %X:%u\n",
                (Request->Flags & REQUEST_FLAG_PACKET_COMMAND) ? "PI" : "",
                (Request->Flags & REQUEST_FLAG_PACKET_COMMAND) ?
                Request->Cdb[0] : Request->TaskFile.Command,
                AtaStatus,
                ATA_READ(DeviceUnit->Registers.Status),
                ATA_READ(DeviceUnit->Registers.Error),
                DeviceUnit->Registers.Data,
                DeviceUnit->DeviceNumber);
        }

        switch (AtaStatus)
        {
            case ATA_STATUS_SUCCESS:
                return TRUE;

            case ATA_STATUS_RETRY:
                break;

            case ATA_STATUS_RESET:
            {
                if (Request->Flags & REQUEST_FLAG_IDENTIFY_COMMAND)
                {
                    /*
                     * Some controllers indicate status 0x00
                     * when the selected device does not exist,
                     * no point in going further.
                     */
                    if (ATA_READ(DeviceUnit->Registers.Status) == 0)
                        return FALSE;
                }

                /* Turn off various things and retry the command */
                DeviceUnit->MultiSectorTransfer = 0;
                DeviceUnit->P.Flags &= ~ATA_DEVICE_FLAG_IO32;

                if (!AtapPerformSoftwareReset(DeviceUnit))
                    return FALSE;

                break;
            }

            default:
            {
                if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
                {
                    if (!AtapAtapiHandleError(DeviceUnit))
                        return FALSE;
                }

                /* Only retry failed read commands */
                if (!(Request->Flags & REQUEST_FLAG_READ_COMMAND))
                    return FALSE;

                break;
            }
        }
    }

    return FALSE;
}

static
UCHAR
AtapGetReadCommand(
    _In_ PATA_DEVICE_REQUEST Request)
{
    static const UCHAR AtapReadCommandMap[2][2] =
    {
        /* Read                      Read EXT */
        { IDE_COMMAND_READ,          IDE_COMMAND_READ_EXT          }, // PIO single
        { IDE_COMMAND_READ_MULTIPLE, IDE_COMMAND_READ_MULTIPLE_EXT }, // PIO multiple
    };

    return AtapReadCommandMap[(Request->Flags & REQUEST_FLAG_READ_WRITE_MULTIPLE) ? 1 : 0]
                             [(Request->Flags & REQUEST_FLAG_LBA48) ? 1 : 0];
}

static
VOID
AtapBuildReadTaskFile(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG64 Lba,
    _In_ ULONG SectorCount)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    UCHAR DriveSelect;

    Request->Flags = REQUEST_FLAG_READ_COMMAND | REQUEST_FLAG_SET_DEVICE_REGISTER;

    if (DeviceUnit->MultiSectorTransfer != 0)
    {
        Request->Flags |= REQUEST_FLAG_READ_WRITE_MULTIPLE;
    }

    if (DeviceUnit->P.Flags & ATA_DEVICE_LBA)
    {
        DriveSelect = IDE_LBA_MODE;

        TaskFile->Data[0].SectorCount = (UCHAR)SectorCount;
        TaskFile->Data[0].LowLba = (UCHAR)Lba;              // LBA bits 0-7
        TaskFile->Data[0].MidLba = (UCHAR)(Lba >> 8);       // LBA bits 8-15
        TaskFile->Data[0].HighLba = (UCHAR)(Lba >> 16);     // LBA bits 16-23

        if ((DeviceUnit->P.Flags & ATA_DEVICE_LBA48) && (AtaCommandUseLba48(Lba, SectorCount)))
        {
            ASSERT((Lba + SectorCount) <= ATA_MAX_LBA_48);

            /* 48-bit command */
            TaskFile->Data[1].SectorCount = (UCHAR)(SectorCount >> 8);
            TaskFile->Data[1].LowLba = (UCHAR)(Lba >> 24);  // LBA bits 24-31
            TaskFile->Data[1].MidLba = (UCHAR)(Lba >> 32);  // LBA bits 32-39
            TaskFile->Data[1].HighLba = (UCHAR)(Lba >> 40); // LBA bits 40-47

            Request->Flags |= REQUEST_FLAG_LBA48;
        }
        else
        {
            ASSERT((Lba + SectorCount) <= ATA_MAX_LBA_28);

            /* 28-bit command */
            DriveSelect |= ((Lba >> 24) & 0x0F);      // LBA bits 24-27
        }
    }
    else
    {
        ULONG ChsTemp, Cylinder, Head, Sector;

        ChsTemp = (ULONG)Lba / DeviceUnit->P.SectorsPerTrack;

        /* Legacy CHS translation */
        Cylinder = ChsTemp / DeviceUnit->P.Heads;
        Head = ChsTemp % DeviceUnit->P.Heads;
        Sector = ((ULONG)Lba % DeviceUnit->P.SectorsPerTrack) + 1;

        ASSERT(Cylinder <= 65535 && Head <= 15 && Sector <= 255);

        TaskFile->Data[0].SectorCount = (UCHAR)SectorCount;
        TaskFile->Data[0].LowLba = (UCHAR)Sector;
        TaskFile->Data[0].MidLba = (UCHAR)Cylinder;
        TaskFile->Data[0].HighLba = (UCHAR)(Cylinder >> 8);

        DriveSelect = Head;
    }
    TaskFile->DriveSelect = DeviceUnit->DeviceSelect | DriveSelect;
    TaskFile->Command = AtapGetReadCommand(Request);
}

static
VOID
AtapBuildReadPacketCommand(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG64 Lba,
    _In_ ULONG SectorCount)
{
    Request->Flags = REQUEST_FLAG_READ_COMMAND | REQUEST_FLAG_PACKET_COMMAND;

    RtlZeroMemory(Request->Cdb, sizeof(Request->Cdb));

    if (Lba > MAXULONG)
    {
        /* READ (16) */
        Request->Cdb[0] = SCSIOP_READ16;
        Request->Cdb[2] = (UCHAR)(Lba >> 56);
        Request->Cdb[3] = (UCHAR)(Lba >> 48);
        Request->Cdb[4] = (UCHAR)(Lba >> 40);
        Request->Cdb[5] = (UCHAR)(Lba >> 32);
        Request->Cdb[6] = (UCHAR)(Lba >> 24);
        Request->Cdb[7] = (UCHAR)(Lba >> 16);
        Request->Cdb[8] = (UCHAR)(Lba >> 8);
        Request->Cdb[9] = (UCHAR)(Lba >> 0);
        Request->Cdb[10] = (UCHAR)(SectorCount >> 24);
        Request->Cdb[11] = (UCHAR)(SectorCount >> 16);
        Request->Cdb[12] = (UCHAR)(SectorCount >> 8);
        Request->Cdb[13] = (UCHAR)(SectorCount >> 0);
    }
    else
    {
        /* READ (10) */
        Request->Cdb[0] = SCSIOP_READ;
        Request->Cdb[2] = (UCHAR)(Lba >> 24);
        Request->Cdb[3] = (UCHAR)(Lba >> 16);
        Request->Cdb[4] = (UCHAR)(Lba >> 8);
        Request->Cdb[5] = (UCHAR)(Lba >> 0);
        Request->Cdb[7] = (UCHAR)(SectorCount >> 8);
        Request->Cdb[8] = (UCHAR)(SectorCount >> 0);
    }
}

static
BOOLEAN
AtapIsDevicePresent(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    PIDE_REGISTERS Registers = &DeviceUnit->Registers;
    UCHAR IdeStatus;

    AtapSelectDevice(Registers, DeviceUnit->DeviceNumber);

    IdeStatus = ATA_READ(Registers->Status);
    if (IdeStatus == 0xFF || IdeStatus == 0x7F)
        return FALSE;

    ATA_WRITE(Registers->ByteCountLow, 0x55);
    ATA_WRITE(Registers->ByteCountLow, 0xAA);
    ATA_WRITE(Registers->ByteCountLow, 0x55);
    if (ATA_READ(Registers->ByteCountLow) != 0x55)
        return FALSE;
    ATA_WRITE(Registers->ByteCountHigh, 0xAA);
    ATA_WRITE(Registers->ByteCountHigh, 0x55);
    ATA_WRITE(Registers->ByteCountHigh, 0xAA);
    if (ATA_READ(Registers->ByteCountHigh) != 0xAA)
        return FALSE;

    if (!AtapWaitForNotBusy(Registers, ATA_TIME_BUSY_ENUM, &IdeStatus))
    {
        ERR("Device %X:%u is busy %02x\n", Registers->Data, DeviceUnit->DeviceNumber, IdeStatus);

        /* Bring the device into a known state */
        if (!AtapPerformSoftwareReset(DeviceUnit))
            return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
AtapReadIdentifyData(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ UCHAR Command)
{
    ATA_DEVICE_REQUEST Request = { 0 };
    PIDENTIFY_DEVICE_DATA Id = &DeviceUnit->IdentifyDeviceData;

    /* Send the identify command */
    Request.Flags = REQUEST_FLAG_IDENTIFY_COMMAND;
    Request.DataBuffer = Id;
    Request.DataTransferLength = sizeof(*Id);
    Request.TaskFile.Command = Command;

    return AtapIssueCommand(DeviceUnit, &Request);
}

static
ATA_DEVICE_CLASS
AtapIdentifyDevice(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    if (!AtapIsDevicePresent(DeviceUnit))
        return DEV_NONE;

    /*
     * We don't check the device signature here,
     * because the NEC CDR-260 drive reports an ATA signature.
     */

    /* Check for ATA */
    if (AtapReadIdentifyData(DeviceUnit, IDE_COMMAND_IDENTIFY))
        return DEV_ATA;

    /* Check for ATAPI */
    if (AtapReadIdentifyData(DeviceUnit, IDE_COMMAND_ATAPI_IDENTIFY))
        return DEV_ATAPI;

    return DEV_NONE;
}

static
BOOLEAN
AtapAtapiReadCapacity16(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _Out_ PREAD_CAPACITY16_DATA CapacityData)
{
    ATA_DEVICE_REQUEST Request = { 0 };

    /* Send the SCSI READ CAPACITY(16) command */
    Request.Flags = REQUEST_FLAG_PACKET_COMMAND;
    Request.DataBuffer = CapacityData;
    Request.DataTransferLength = sizeof(*CapacityData);
    Request.Cdb[0] = SCSIOP_SERVICE_ACTION_IN16;
    Request.Cdb[1] = SERVICE_ACTION_READ_CAPACITY16;
    Request.Cdb[13] = sizeof(*CapacityData);

    return AtapIssueCommand(DeviceUnit, &Request);
}

static
BOOLEAN
AtapAtapiReadCapacity10(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _Out_ PREAD_CAPACITY_DATA CapacityData)
{
    ATA_DEVICE_REQUEST Request = { 0 };

    /* Send the SCSI READ CAPACITY(10) command */
    Request.Flags = REQUEST_FLAG_PACKET_COMMAND;
    Request.DataBuffer = CapacityData;
    Request.DataTransferLength = sizeof(*CapacityData);
    Request.Cdb[0] = SCSIOP_READ_CAPACITY;

    return AtapIssueCommand(DeviceUnit, &Request);
}

static
VOID
AtapAtapiDetectCapacity(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _Out_ PULONG64 TotalSectors,
    _Out_ PULONG SectorSize)
{
    union
    {
        READ_CAPACITY_DATA Cmd10;
        READ_CAPACITY16_DATA Cmd16;
    } CapacityData;
    ULONG LastLba;

    *TotalSectors = 0;
    *SectorSize = 0;

    if (!AtapAtapiReadCapacity10(DeviceUnit, &CapacityData.Cmd10))
        return;

    LastLba = RtlUlongByteSwap(CapacityData.Cmd10.LogicalBlockAddress);
    if (LastLba == MAXULONG)
    {
        if (!AtapAtapiReadCapacity16(DeviceUnit, &CapacityData.Cmd16))
            return;

        *TotalSectors = RtlUlonglongByteSwap(CapacityData.Cmd16.LogicalBlockAddress.QuadPart) + 1;
        *SectorSize = RtlUlongByteSwap(CapacityData.Cmd16.BytesPerBlock);
    }
    else
    {
        *TotalSectors = LastLba + 1;
        *SectorSize = RtlUlongByteSwap(CapacityData.Cmd10.BytesPerBlock);
    }

    /*
     * If device reports a non-zero block length, reset to defaults
     * (we use a READ command instead of READ CD).
     */
    if (*SectorSize != 0)
        *SectorSize = 2048;
}

static
BOOLEAN
AtapAtapiTestUnitReady(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    ATA_DEVICE_REQUEST Request = { 0 };

    /* Send the SCSI TEST UNIT READY command */
    Request.Flags = REQUEST_FLAG_PACKET_COMMAND;
    Request.Cdb[0] = SCSIOP_TEST_UNIT_READY;

    return AtapIssueCommand(DeviceUnit, &Request);
}

static
BOOLEAN
AtapAtapiRequestSense(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _Out_ PSENSE_DATA SenseData)
{
    ATA_DEVICE_REQUEST Request = { 0 };

    AtapAtapiBuildRequestSense(&Request, SenseData);
    return AtapIssueCommand(DeviceUnit, &Request);
}

static
BOOLEAN
AtapAtapiReadToc(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    ATA_DEVICE_REQUEST Request = { 0 };
    UCHAR DummyData[MAXIMUM_CDROM_SIZE];

    /* Send the SCSI READ TOC command */
    Request.Flags = REQUEST_FLAG_PACKET_COMMAND;
    Request.DataBuffer = DummyData;
    Request.DataTransferLength = sizeof(DummyData);
    Request.Cdb[0] = SCSIOP_READ_TOC;
    Request.Cdb[7] = (MAXIMUM_CDROM_SIZE >> 8) & 0xFF;
    Request.Cdb[8] = MAXIMUM_CDROM_SIZE & 0xFF;
    Request.Cdb[9] = READ_TOC_FORMAT_SESSION << 6;

    return AtapIssueCommand(DeviceUnit, &Request);
}

static
BOOLEAN
AtapAtapiReadyCheck(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    SENSE_DATA SenseData;

    if (!AtapAtapiTestUnitReady(DeviceUnit))
        return FALSE;

    if (!AtapAtapiRequestSense(DeviceUnit, &SenseData))
        return FALSE;

    if (SenseData.SenseKey == SCSI_SENSE_NOT_READY)
    {
        if (SenseData.AdditionalSenseCode == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE)
            return FALSE;

        if (SenseData.AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY)
        {
            switch (SenseData.AdditionalSenseCodeQualifier)
            {
                case SCSI_SENSEQ_BECOMING_READY:
                    /* Wait until the CD is spun up */
                    StallExecutionProcessor(2e6);
                    return FALSE;

                case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:
                    /* The drive needs to be spun up */
                    AtapAtapiReadToc(DeviceUnit);
                    return FALSE;

                default:
                    return FALSE;
            }
        }
    }

    return TRUE;
}

static
VOID
AtapAtapiClearUnitAttention(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    SENSE_DATA SenseData;
    ULONG i;

    for (i = 0; i < 5; ++i)
    {
        if (!AtapAtapiRequestSense(DeviceUnit, &SenseData))
            continue;

        if ((SenseData.SenseKey != SCSI_SENSE_UNIT_ATTENTION) &&
            (SenseData.AdditionalSenseCode != SCSI_ADSENSE_BUS_RESET))
        {
            break;
        }
    }
}

static
BOOLEAN
AtapAtapiInitDevice(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    PIDENTIFY_PACKET_DATA IdentifyPacketData = &DeviceUnit->IdentifyPacketData;
    ULONG i;

    DeviceUnit->CdbSize = AtaDevCdbSizeInWords(IdentifyPacketData);

    /* Clear the ATAPI 'Bus reset' indication */
    AtapAtapiClearUnitAttention(DeviceUnit);

    /* Make the device ready */
    for (i = 4; i > 0; i--)
    {
        if (AtapAtapiReadyCheck(DeviceUnit))
            break;
    }
    if (i == 0)
    {
        ERR("Device not ready\n");
        return FALSE;
    }

    /* Detect a medium's capacity */
    AtapAtapiDetectCapacity(DeviceUnit,
                            &DeviceUnit->P.TotalSectors,
                            &DeviceUnit->P.SectorSize);
    if (DeviceUnit->P.SectorSize == 0 || DeviceUnit->P.TotalSectors == 0)
    {
        TRACE("No media found\n");
        return FALSE;
    }

    DeviceUnit->P.Cylinders = MAXULONG;
    DeviceUnit->P.Heads = MAXULONG;
    DeviceUnit->P.SectorsPerTrack = MAXULONG;

    DeviceUnit->MaximumTransferLength = 0xFFFF;

    return TRUE;
}

static
VOID
AtapAtaSetMultipleMode(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    ATA_DEVICE_REQUEST Request = { 0 };

#if !defined(ATA_ENABLE_MULTIPLE_MODE)
    /* Inherit multiple mode from the state the BIOS firmware left the device in during boot */
    DeviceUnit->MultiSectorTransfer = AtaDevCurrentSectorsPerDrq(&DeviceUnit->IdentifyDeviceData);
    if (DeviceUnit->MultiSectorTransfer != 0)
        return;
#endif

    /* Use the maximum possible value */
    DeviceUnit->MultiSectorTransfer = AtaDevMaximumSectorsPerDrq(&DeviceUnit->IdentifyDeviceData);
    if (DeviceUnit->MultiSectorTransfer == 0)
        return;

    Request.TaskFile.Command = IDE_COMMAND_SET_MULTIPLE;
    Request.TaskFile.Data[0].SectorCount = DeviceUnit->MultiSectorTransfer;
    if (!AtapIssueCommand(DeviceUnit, &Request))
    {
        DeviceUnit->MultiSectorTransfer = 0;
    }
}

static
BOOLEAN
AtapAtaInitDevice(
    _In_ PHW_DEVICE_UNIT DeviceUnit)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DeviceUnit->IdentifyDeviceData;
    ULONG64 TotalSectors;
    USHORT Cylinders, Heads, SectorsPerTrack;

    DeviceUnit->MaximumTransferLength = 0xFF;

    if (AtaDevIsCurrentGeometryValid(IdentifyData))
        AtaDevCurrentChsTranslation(IdentifyData, &Cylinders, &Heads, &SectorsPerTrack);
    else
        AtaDevDefaultChsTranslation(IdentifyData, &Cylinders, &Heads, &SectorsPerTrack);

    /* Using LBA addressing mode */
    if (AtaDevHasLbaTranslation(IdentifyData))
    {
        DeviceUnit->P.Flags |= ATA_DEVICE_LBA;

        if (AtaDevHas48BitAddressFeature(IdentifyData))
        {
            /* Using LBA48 addressing mode */
            TotalSectors = AtaDevUserAddressableSectors48Bit(IdentifyData);
            ASSERT(TotalSectors <= ATA_MAX_LBA_48);

            DeviceUnit->P.Flags |= ATA_DEVICE_LBA48;
            DeviceUnit->MaximumTransferLength = 0x10000;
        }
        else
        {
            /* Using LBA28 addressing mode */
            TotalSectors = AtaDevUserAddressableSectors28Bit(IdentifyData);
            ASSERT(TotalSectors <= ATA_MAX_LBA_28);
        }
    }
    else
    {
        /* Using CHS addressing mode */
        TotalSectors = Cylinders * Heads * SectorsPerTrack;
    }

    if (TotalSectors == 0)
    {
        ERR("Unknown geometry\n");
        return FALSE;
    }

    DeviceUnit->P.TotalSectors = TotalSectors;
    DeviceUnit->P.Cylinders = Cylinders;
    DeviceUnit->P.Heads = Heads;
    DeviceUnit->P.SectorsPerTrack = SectorsPerTrack;
    DeviceUnit->P.SectorSize = AtaDevBytesPerLogicalSector(IdentifyData);
    ASSERT(DeviceUnit->P.SectorSize >= 512);
    DeviceUnit->P.SectorSize = max(DeviceUnit->P.SectorSize, 512);

    AtapAtaSetMultipleMode(DeviceUnit);

    TRACE("Multiple sector setting %u\n", DeviceUnit->MultiSectorTransfer);

    return TRUE;
}

static
BOOLEAN
AtapAnalyzeIdentifyData(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ ATA_DEVICE_CLASS DeviceClass)
{
    PIDENTIFY_DEVICE_DATA Id = &DeviceUnit->IdentifyDeviceData;
    ULONG i;

    /* Verify the checksum */
    if (!AtaDevIsIdentifyDataValid(Id))
    {
        ERR("Identify data CRC error\n");
        return FALSE;
    }

    if (DeviceClass == DEV_ATAPI)
    {
        DeviceUnit->P.Flags |= ATA_DEVICE_ATAPI | ATA_DEVICE_LBA;

        /* The returned string is not byteswapped */
        if (Id->ModelNumber[0] == 'N' &&
            Id->ModelNumber[1] == 'E' &&
            Id->ModelNumber[2] == 'C' &&
            Id->ModelNumber[3] == ' ')
        {
            DeviceUnit->P.Flags |= ATA_DEVICE_IS_NEC_CDR260;
        }
    }

    /* Swap byte order of the ASCII data */
    for (i = 0; i < sizeof(Id->SerialNumber) / 2; ++i)
        ((PUSHORT)Id->SerialNumber)[i] = RtlUshortByteSwap(((PUSHORT)Id->SerialNumber)[i]);

    for (i = 0; i < sizeof(Id->FirmwareRevision) / 2; ++i)
        ((PUSHORT)Id->FirmwareRevision)[i] = RtlUshortByteSwap(((PUSHORT)Id->FirmwareRevision)[i]);

    for (i = 0; i < sizeof(Id->ModelNumber) / 2; ++i)
        ((PUSHORT)Id->ModelNumber)[i] = RtlUshortByteSwap(((PUSHORT)Id->ModelNumber)[i]);

    TRACE("MN  '%.*s'\n", sizeof(Id->ModelNumber), Id->ModelNumber);
    TRACE("FR  '%.*s'\n", sizeof(Id->FirmwareRevision), Id->FirmwareRevision);
    TRACE("S/N '%.*s'\n", sizeof(Id->SerialNumber), Id->SerialNumber);

    return TRUE;
}

static
BOOLEAN
AtapInitDevice(
    _In_ PHW_DEVICE_UNIT DeviceUnit,
    _In_ ATA_DEVICE_CLASS DeviceClass)
{
    if (!AtapAnalyzeIdentifyData(DeviceUnit, DeviceClass))
        return FALSE;

    if (DeviceClass == DEV_ATAPI)
        return AtapAtapiInitDevice(DeviceUnit);
    else
        return AtapAtaInitDevice(DeviceUnit);
}

static
BOOLEAN
AtapIdentifyChannel(
    _In_ ULONG ChannelNumber,
    _Out_ PIDE_REGISTERS Registers)
{
    const IDE_REG IoBase = AtapChannelBaseArray[ChannelNumber];
    ULONG Spare;

#if defined(SARCH_PC98)
    if (ATA_READ(0x432) == 0xFF)
        return FALSE;
#endif

#if defined(SARCH_PC98)
    Spare = 2;
    Registers->Control     = 0x74C;
#else
    Spare = 1;
    Registers->Control     = IoBase + 0x206;
#endif
    Registers->Data        = IoBase + 0 * Spare;
    Registers->Error       = IoBase + 1 * Spare;
    Registers->SectorCount = IoBase + 2 * Spare;
    Registers->LbaLow      = IoBase + 3 * Spare;
    Registers->LbaMid      = IoBase + 4 * Spare;
    Registers->LbaHigh     = IoBase + 5 * Spare;
    Registers->Device      = IoBase + 6 * Spare;
    Registers->Status      = IoBase + 7 * Spare;

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN
AtaReadLogicalSectors(
    _In_ PDEVICE_UNIT DeviceUnit,
    _In_ ULONG64 SectorNumber,
    _In_ ULONG SectorCount,
    _Out_writes_bytes_all_(SectorCount * DeviceUnit->SectorSize) PVOID Buffer)
{
    PHW_DEVICE_UNIT Unit = (PHW_DEVICE_UNIT)DeviceUnit;
    ATA_DEVICE_REQUEST Request = { 0 };

    ASSERT((SectorNumber + SectorCount) <= Unit->P.TotalSectors);
    ASSERT(SectorCount != 0);

    while (SectorCount > 0)
    {
        ULONG BlockCount;

        BlockCount = min(SectorCount, Unit->MaximumTransferLength);

        Request.DataBuffer = Buffer;
        Request.DataTransferLength = BlockCount * Unit->P.SectorSize;

        if (Unit->P.Flags & ATA_DEVICE_ATAPI)
            AtapBuildReadPacketCommand(&Request, SectorNumber, BlockCount);
        else
            AtapBuildReadTaskFile(Unit, &Request, SectorNumber, BlockCount);

        if (!AtapIssueCommand(Unit, &Request))
            return FALSE;

        SectorNumber += BlockCount;
        SectorCount -= BlockCount;

        Buffer = (PVOID)((ULONG_PTR)Buffer + Unit->P.SectorSize);
    }

    return TRUE;
}

PDEVICE_UNIT
AtaGetDevice(
    _In_ UCHAR UnitNumber)
{
    if (UnitNumber < RTL_NUMBER_OF(AtapUnits))
        return (PDEVICE_UNIT)AtapUnits[UnitNumber];

    return NULL;
}

BOOLEAN
AtaInit(
    _Out_ PUCHAR DetectedCount)
{
    ULONG ChannelNumber;

    *DetectedCount = 0;

    /* Enumerate IDE channels */
    for (ChannelNumber = 0; ChannelNumber < CHANNEL_MAX_CHANNELS; ++ChannelNumber)
    {
        UCHAR DeviceNumber;
        IDE_REGISTERS Registers;

        if (!AtapIdentifyChannel(ChannelNumber, &Registers))
            continue;

        /* Check for devices attached to the bus */
        for (DeviceNumber = 0; DeviceNumber < CHANNEL_MAX_DEVICES; ++DeviceNumber)
        {
            PHW_DEVICE_UNIT DeviceUnit;
            ATA_DEVICE_CLASS DeviceClass;

            /* Allocate a new device unit structure */
            DeviceUnit = FrLdrTempAlloc(sizeof(*DeviceUnit), TAG_ATA_DEVICE);
            if (!DeviceUnit)
            {
                ERR("Failed to allocate device unit!\n");
                continue;
            }
            RtlZeroMemory(DeviceUnit, sizeof(*DeviceUnit));

            /* Perform a minimal initialization */
            RtlCopyMemory(&DeviceUnit->Registers, &Registers, sizeof(Registers));
            DeviceUnit->DeviceNumber = DeviceNumber;
            DeviceUnit->P.SectorSize = 512;
            DeviceUnit->DeviceSelect = (DEV_SLAVE(DeviceNumber) << 4) | IDE_DRIVE_SELECT;

            /* Let's see what kind of device this is */
            DeviceClass = AtapIdentifyDevice(DeviceUnit);
            if (DeviceClass == DEV_NONE)
                goto NextDevice;

            TRACE("Found %lu device at %X:%u\n", DeviceClass, Registers.Data, DeviceNumber);

            if (!AtapInitDevice(DeviceUnit, DeviceClass))
                goto NextDevice;

            TRACE("Total sectors %I64u of size %lu, CHS %lu:%lu:%lu, %lx\n",
                  DeviceUnit->P.TotalSectors,
                  DeviceUnit->P.SectorSize,
                  DeviceUnit->P.Cylinders,
                  DeviceUnit->P.Heads,
                  DeviceUnit->P.SectorsPerTrack,
                  DeviceUnit->P.Flags);

            AtapUnits[(*DetectedCount)++] = DeviceUnit;
            continue;

NextDevice:
            FrLdrTempFree(DeviceUnit, TAG_ATA_DEVICE);
        }
    }

    return (*DetectedCount > 0);
}
