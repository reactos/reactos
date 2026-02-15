/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI/ATA Translation layer
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static const UCHAR AtapReadWriteCommandMap[12][2] =
{
    /* Write EXT                          Read EXT */
    { IDE_COMMAND_WRITE_EXT,              IDE_COMMAND_READ_EXT          }, // PIO single
    { IDE_COMMAND_WRITE_MULTIPLE_EXT,     IDE_COMMAND_READ_MULTIPLE_EXT }, // PIO multiple
    { IDE_COMMAND_WRITE_DMA_EXT,          IDE_COMMAND_READ_DMA_EXT      }, // DMA

    /* Write FUA EXT                      Read FUA EXT */
    { 0,                                  0                             }, // PIO single
    { IDE_COMMAND_WRITE_MULTIPLE_FUA_EXT, 0                             }, // PIO multiple
    { IDE_COMMAND_WRITE_DMA_FUA_EXT,      0                             }, // DMA

    /* Write                              Read */
    { IDE_COMMAND_WRITE,                  IDE_COMMAND_READ,             }, // PIO single
    { IDE_COMMAND_WRITE_MULTIPLE,         IDE_COMMAND_READ_MULTIPLE,    }, // PIO multiple
    { IDE_COMMAND_WRITE_DMA,              IDE_COMMAND_READ_DMA,         }, // DMA

    /* Write FUA                          Read FUA */
    { 0,                                  0                             }, // PIO single
    { 0,                                  0                             }, // PIO multiple
    { 0,                                  0                             }, // DMA
};

/* FUNCTIONS ******************************************************************/

static
inline
UCHAR
AtaReadWriteCommand(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PATAPORT_IO_CONTEXT Device)
{
    ULONG CmdEntry;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        if (Device->DeviceFlags & DEVICE_PIO_FOR_LBA48_XFER)
            Request->Flags &= ~REQUEST_FLAG_DMA;

        CmdEntry = 0;
    }
    else
    {
        CmdEntry = 6;
    }

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

BOOLEAN
AtaReqDmaTransferToPioTransfer(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;

    ASSERT(!(Device->DeviceFlags & DEVICE_PIO_VIA_DMA));
    ASSERT(Request->Flags & REQUEST_DMA_FLAGS);

    /* ATAPI commands */
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
    {
        Request->Flags &= ~REQUEST_DMA_FLAGS;
        return TRUE;
    }

    /*
     * For ATA commands there is no simple way to achieve this,
     * we have to fix the command opcode.
     * Determine if it is safe or allowed to change the command.
     */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE)
    {
        Request->Flags &= ~REQUEST_DMA_FLAGS;

        Request->TaskFile.Command = AtaReadWriteCommand(Request, Device);
        if (Request->TaskFile.Command == 0)
        {
            /* PIO is not available */
            Request->Flags |= REQUEST_DMA_FLAGS;
            return FALSE;
        }

        return TRUE;
    }

    /* PIO is not available */
    return FALSE;
}

static
ULONG64
AtaReqLbaFromTaskFile(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;
    ULONG64 Lba;

    if (Device->DeviceFlags & DEVICE_LBA_MODE)
    {
        Lba = ((ULONG64)TaskFile->LowLba << 0) |
              ((ULONG64)TaskFile->MidLba << 8) |
              ((ULONG64)TaskFile->HighLba << 16);

        if (Request->Flags & REQUEST_FLAG_LBA48)
        {
            /* 48-bit command */
            Lba |= ((ULONG64)TaskFile->LowLbaEx << 24) |
                   ((ULONG64)TaskFile->MidLbaEx << 32) |
                   ((ULONG64)TaskFile->HighLbaEx << 40);
        }
        else
        {
            /* 28-bit command */
            Lba |= (ULONG64)(TaskFile->DriveSelect & 0x0F) << 24;
        }
    }
    else
    {
        ULONG Cylinder, Head, Sector;

        /* Legacy CHS translation */
        Cylinder = TaskFile->LowLba;
        Head = TaskFile->DriveSelect & 0x0F;
        Sector = ((ULONG64)TaskFile->HighLba << 8) | TaskFile->MidLba;

        // TODO use 64bit math?
        Lba = (((Cylinder * Device->Heads) + Head) * Device->SectorsPerTrack) + (Sector - 1);
    }

    return Lba;
}

static
BOOLEAN
AtaReqTranslateFixedError(
    _In_ PATA_DEVICE_REQUEST Request,
    _Out_ PUCHAR SK,
    _Out_ PUCHAR ASK,
    _Out_ PUCHAR ASCQ)
{
    if (Request->Output.Status & IDE_STATUS_DEVICE_FAULT)
    {
        *SK = SCSI_SENSE_HARDWARE_ERROR;
        *ASK = SCSI_ADSENSE_INTERNAL_TARGET_FAILURE;
        *ASCQ = SCSI_SENSEQ_INTERNAL_TARGET_FAILURE;
    }
    else if (Request->Output.Error & IDE_ERROR_DATA_ERROR)
    {
        if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        {
            *SK = SCSI_SENSE_DATA_PROTECT;
            *ASK = SCSI_ADSENSE_WRITE_PROTECT;
            *ASCQ = 0;
        }
        else
        {
            *SK = SCSI_SENSE_MEDIUM_ERROR;
            *ASK = SCSI_ADSENSE_UNRECOVERED_ERROR;
            *ASCQ = SCSI_SENSEQ_UNRECOVERED_READ_ERROR;
        }
    }
    else if (Request->Output.Error & IDE_ERROR_ID_NOT_FOUND)
    {
        *SK = SCSI_SENSE_ILLEGAL_REQUEST;
        *ASK = SCSI_ADSENSE_ILLEGAL_BLOCK;
        *ASCQ = SCSI_SENSEQ_LOGICAL_ADDRESS_OUT_OF_RANGE;
    }
    else if (Request->Output.Error & IDE_ERROR_CRC_ERROR)
    {
        *SK = SCSI_SENSE_HARDWARE_ERROR;
        *ASK = SCSI_ADSENSE_LUN_COMMUNICATION;
        *ASCQ = SCSI_SESNEQ_COMM_CRC_ERROR;
    }
    /* Return vendor specific codes for obsolete bits */
    else if (Request->Output.Error & IDE_ERROR_MEDIA_CHANGE)
    {
        *SK = SCSI_SENSE_UNIT_ATTENTION;
        *ASK = SCSI_ADSENSE_MEDIUM_CHANGED;
        *ASCQ = 0;
    }
    else if (Request->Output.Error & IDE_ERROR_MEDIA_CHANGE_REQ)
    {
        *SK = SCSI_SENSE_UNIT_ATTENTION;
        *ASK = SCSI_ADSENSE_OPERATOR_REQUEST;
        *ASCQ = SCSI_SENSEQ_MEDIUM_REMOVAL;
    }
    else if (Request->Output.Error & IDE_ERROR_END_OF_MEDIA)
    {
        *SK = SCSI_SENSE_NOT_READY;
        *ASK = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
        *ASCQ = 0;
    }
    else if (Request->Output.Error & IDE_ERROR_ADDRESS_NOT_FOUND)
    {
        *SK = SCSI_SENSE_MEDIUM_ERROR;
        *ASK = SCSI_ADSENSE_ADDRESS_MARK_NOT_FOUND_FOR_DATA_FIELD;
        *ASCQ = 0;
    }
    /* The ABORT bit has low priority and indicates unknown error */
    else if (Request->Output.Error & IDE_ERROR_COMMAND_ABORTED)
    {
        *SK = SCSI_SENSE_ABORTED_COMMAND;
        *ASK = SCSI_ADSENSE_NO_SENSE;
        *ASCQ = 0;
    }
    else
    {
        *SK = SCSI_SENSE_ABORTED_COMMAND;
        *ASK = SCSI_ADSENSE_NO_SENSE;
        *ASCQ = 0;

        /* No sense data */
        return FALSE;
    }

    return TRUE;
}

UCHAR
AtaReqSetFixedAtaSenseData(
    _In_ PATA_DEVICE_REQUEST Request)
{
    SCSI_SENSE_CODE SenseCode;
    UCHAR SrbStatus, SK, ASK, ASCQ;

    ASSERT(!(Request->SrbStatus & SRB_STATUS_AUTOSENSE_VALID));

    if (!AtaReqTranslateFixedError(Request, &SK, &ASK, &ASCQ))
        return Request->SrbStatus;

    SenseCode.SrbStatus = Request->SrbStatus;
    SenseCode.SenseKey = SK;
    SenseCode.AdditionalSenseCode = ASK;
    SenseCode.AdditionalSenseCodeQualifier = ASCQ;

    SrbStatus = AtaReqSetFixedSenseData(Request->Srb, SenseCode);
    AtaReqSetLbaInformation(Request->Srb, AtaReqLbaFromTaskFile(Request));

    return SrbStatus;
}

static
ULONG
AtaReqCopySatlBuffer(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PVOID Buffer,
    _In_ ULONG Length)
{
    PSCSI_REQUEST_BLOCK Srb = Request->Srb;
    PMDL Mdl;
    PVOID BaseAddress;
    ULONG_PTR Offset;
    ULONG BytesCount;

    /* The driver can overwrite the Request->Mdl field during translation */
    Mdl = Request->Irp->MdlAddress;
    ASSERT(Mdl);

    BaseAddress = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
    if (!BaseAddress)
        return SRB_STATUS_INSUFFICIENT_RESOURCES;

    if (Srb->DataTransferLength < Length)
    {
        /* Overrun */
        BytesCount = Length;
        Request->SrbStatus = SRB_STATUS_DATA_OVERRUN;
    }
    else
    {
        /* Success or underrun */
        BytesCount = Srb->DataTransferLength;
        Request->SrbStatus = SRB_STATUS_SUCCESS;
    }
    Request->DataTransferLength = BytesCount;

    /* Calculate the offset within DataBuffer */
    Offset = (ULONG_PTR)BaseAddress +
             (ULONG_PTR)Srb->DataBuffer -
             (ULONG_PTR)MmGetMdlVirtualAddress(Mdl);

    RtlCopyMemory((PVOID)Offset, Buffer, BytesCount);
    return Request->SrbStatus;
}

static
UCHAR
AtaReqTerminateInvalidOpCode(
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    SCSI_SENSE_CODE SenseCode;

    SenseCode.SrbStatus = SRB_STATUS_INVALID_REQUEST;
    SenseCode.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
    SenseCode.AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_COMMAND;
    SenseCode.AdditionalSenseCodeQualifier = 0;

    return AtaReqSetFixedSenseData(Srb, SenseCode);
}

static
UCHAR
AtaReqTerminateInvalidField(
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    SCSI_SENSE_CODE SenseCode;

    SenseCode.SrbStatus = SRB_STATUS_INVALID_REQUEST;
    SenseCode.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
    SenseCode.AdditionalSenseCode = SCSI_ADSENSE_INVALID_CDB;
    SenseCode.AdditionalSenseCodeQualifier = 0;

    return AtaReqSetFixedSenseData(Srb, SenseCode);
}

/* static */
UCHAR
AtaReqTerminateInvalidFieldParameter(
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    SCSI_SENSE_CODE SenseCode;

    SenseCode.SrbStatus = SRB_STATUS_INVALID_REQUEST;
    SenseCode.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
    SenseCode.AdditionalSenseCode = SCSI_ADSENSE_INVALID_FIELD_PARAMETER_LIST;
    SenseCode.AdditionalSenseCodeQualifier = 0;

    return AtaReqSetFixedSenseData(Srb, SenseCode);
}

static
UCHAR
AtaReqTerminateInvalidRange(
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    SCSI_SENSE_CODE SenseCode;

    /*
     * This normally should not happen and only used
     * to aid in detecting bugs in the class drivers or software.
     */
    ASSERT(FALSE);

    SenseCode.SrbStatus = SRB_STATUS_INVALID_REQUEST;
    SenseCode.SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
    SenseCode.AdditionalSenseCode = SCSI_ADSENSE_ILLEGAL_BLOCK;
    SenseCode.AdditionalSenseCodeQualifier = 0;

    return AtaReqSetFixedSenseData(Srb, SenseCode);
}

static
BOOLEAN
AtaReqBuildIdentifyCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    Request->Flags = REQUEST_FLAG_DATA_IN | REQUEST_FLAG_EXCLUSIVE | REQUEST_FLAG_POLL;
    Request->DataBuffer = &DevExt->IdentifyDeviceData;
    Request->DataTransferLength = sizeof(DevExt->IdentifyDeviceData);

    if (!AtaReqAllocateMdl(Request))
        return FALSE;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_IDENTIFY;

    return TRUE;
}

VOID
AtaReqBuildReadLogTaskFile(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR LogAddress,
    _In_ UCHAR PageNumber,
    _In_ USHORT LogPageCount)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    /* PIO Data-In command */
    Request->Flags = REQUEST_FLAG_DATA_IN | REQUEST_FLAG_LBA48;
    Request->DataTransferLength = LogPageCount * IDE_GP_LOG_SECTOR_SIZE;

    TaskFile->Feature = 0;
    TaskFile->FeatureEx = 0;

    /* LOG PAGE COUNT */
    TaskFile->SectorCount = (UCHAR)LogPageCount;
    TaskFile->SectorCountEx = (UCHAR)(LogPageCount >> 8);

    TaskFile->LowLba = LogAddress; // LOG ADDRESS
    TaskFile->MidLba = PageNumber; // PAGE NUMBER
    TaskFile->HighLba = 0;         // Reserved
    TaskFile->LowLbaEx = 0;        // Reserved
    TaskFile->MidLbaEx = 0;        // PAGE NUMBER EX
    TaskFile->HighLbaEx = 0;       // Reserved
    TaskFile->Command = IDE_COMMAND_READ_LOG_EXT;
}

static
BOOLEAN
AtaReqBuildLbaTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG64 Lba,
    _In_ ULONG SectorCount)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    UCHAR DriveSelect;

    /* Sector count must not exceed the maximum transfer length */
    ASSERT(SectorCount <= ATA_MAX_SECTORS_PER_IO);

    if (SectorCount > ATA_MAX_SECTORS_PER_IO)
        return FALSE;

    Request->Flags |= REQUEST_FLAG_SET_DEVICE_REGISTER;

    /* 28-bit or 48-bit command */
    if (DevExt->Device.DeviceFlags & DEVICE_LBA_MODE)
    {
        DriveSelect = IDE_LBA_MODE;

        TaskFile->Feature = 0;
        TaskFile->SectorCount = (UCHAR)SectorCount;
        TaskFile->LowLba = (UCHAR)Lba;                // LBA bits 0-7
        TaskFile->MidLba = (UCHAR)(Lba >> 8);         // LBA bits 8-15
        TaskFile->HighLba = (UCHAR)(Lba >> 16);       // LBA bits 16-23

        if ((DevExt->Device.DeviceFlags & DEVICE_LBA48) &&
            ((Request->Flags & REQUEST_FLAG_FUA) || AtaCommandUseLba48(Lba, SectorCount)))
        {
            if ((Lba + SectorCount) > ATA_MAX_LBA_48)
                return FALSE;

            /* 48-bit command */
            TaskFile->FeatureEx = 0;
            TaskFile->SectorCountEx = (UCHAR)(SectorCount >> 8);
            TaskFile->LowLbaEx = (UCHAR)(Lba >> 24);  // LBA bits 24-31
            TaskFile->MidLbaEx = (UCHAR)(Lba >> 32);  // LBA bits 32-39
            TaskFile->HighLbaEx = (UCHAR)(Lba >> 40); // LBA bits 40-47

            Request->Flags |= REQUEST_FLAG_LBA48;
        }
        else
        {
            if ((Lba + SectorCount) > ATA_MAX_LBA_28)
                return FALSE;

            /* 28-bit command */
            DriveSelect |= ((Lba >> 24) & 0x0F);      // LBA bits 24-27
        }
    }
    else
    {
        ULONG ChsTemp, Cylinder, Head, Sector;

        if ((Lba + SectorCount) > ATA_MAX_LBA_28)
            return FALSE;

        ChsTemp = (ULONG)Lba / DevExt->Device.SectorsPerTrack;

        /* Legacy CHS translation */
        Cylinder = ChsTemp / DevExt->Device.Heads;
        Head = ChsTemp % DevExt->Device.Heads;
        Sector = ((ULONG)Lba % DevExt->Device.SectorsPerTrack) + 1;

        /* Check for the 137 GB limit */
        if (Cylinder > 65535 || Head > 15 || Sector > 255)
            return FALSE;

        TaskFile->Feature = 0;
        TaskFile->SectorCount = (UCHAR)SectorCount;
        TaskFile->LowLba = (UCHAR)Sector;
        TaskFile->MidLba = (UCHAR)Cylinder;
        TaskFile->HighLba = (UCHAR)(Cylinder >> 8);

        DriveSelect = Head;
    }
    TaskFile->DriveSelect = DevExt->Device.DeviceSelect | DriveSelect;

    return TRUE;
}

static
BOOLEAN
AtaReqBuildNcqReadWriteTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG64 Lba,
    _In_ ULONG SectorCount)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    ASSERT(DevExt->Device.PortData->PortFlags & PORT_FLAG_NCQ);
    ASSERT(!(DevExt->Device.DeviceFlags & DEVICE_PIO_ONLY));

    /* Sector count must not exceed the maximum transfer length */
    ASSERT(SectorCount <= ATA_MAX_SECTORS_PER_IO);

    if (SectorCount > ATA_MAX_SECTORS_PER_IO)
        return FALSE;

    if ((Lba + SectorCount) > ATA_MAX_LBA_48)
        return FALSE;

    Request->Flags |= REQUEST_FLAG_NCQ |
                      REQUEST_DMA_FLAGS |
                      REQUEST_FLAG_LBA48 |
                      REQUEST_FLAG_SET_DEVICE_REGISTER;

    TaskFile->Feature = (UCHAR)SectorCount;
    TaskFile->FeatureEx = (UCHAR)(SectorCount >> 8);

    TaskFile->SectorCount = 0; // TODO RARC
    TaskFile->SectorCountEx = 0; // TODO PRIO
    // TODO ICC
    // TODO AUX

    TaskFile->LowLba = (UCHAR)Lba;            // LBA bits 0-7
    TaskFile->MidLba = (UCHAR)(Lba >> 8);     // LBA bits 8-15
    TaskFile->HighLba = (UCHAR)(Lba >> 16);   // LBA bits 16-23
    TaskFile->LowLbaEx = (UCHAR)(Lba >> 24);  // LBA bits 24-31
    TaskFile->MidLbaEx = (UCHAR)(Lba >> 32);  // LBA bits 32-39
    TaskFile->HighLbaEx = (UCHAR)(Lba >> 40); // LBA bits 40-47

    TaskFile->DriveSelect = IDE_LBA_MODE;
    if (Request->Flags & REQUEST_FLAG_FUA)
        TaskFile->DriveSelect |= IDE_DEVICE_FUA_NCQ;

    if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        TaskFile->Command = IDE_COMMAND_WRITE_FPDMA_QUEUED;
    else
        TaskFile->Command = IDE_COMMAND_READ_FPDMA_QUEUED;

    return TRUE;
}

static
UCHAR
AtaReqScsiReadWrite(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PCDB Cdb;
    ULONG64 Lba;
    ULONG SectorCount;

    Request->Flags = REQUEST_FLAG_READ_WRITE;

    Cdb = (PCDB)Srb->Cdb;
    switch (Cdb->AsByte[0])
    {
        case SCSIOP_READ6:
        case SCSIOP_WRITE6:
        {
            Lba = CdbGetLogicalBlockAddress6(Cdb);
            break;
        }

        case SCSIOP_READ:
        case SCSIOP_WRITE:
        {
            Lba = CdbGetLogicalBlockAddress10(Cdb);

            if (Cdb->CDB10.ForceUnitAccess)
                Request->Flags |= REQUEST_FLAG_FUA;
            break;
        }

        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
        {
            Lba = CdbGetLogicalBlockAddress12(Cdb);

            if (Cdb->CDB12.ForceUnitAccess)
                Request->Flags |= REQUEST_FLAG_FUA;
            break;
        }

        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
        {
            Lba = CdbGetLogicalBlockAddress16(Cdb);

            if (Cdb->CDB16.ForceUnitAccess)
                Request->Flags |= REQUEST_FLAG_FUA;
            break;
        }

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }

    /* Check for write operations */
    if (Cdb->AsByte[0] & 0x02)
        Request->Flags |= REQUEST_FLAG_DATA_OUT;
    else
        Request->Flags |= REQUEST_FLAG_DATA_IN;

    /* FIXME */
    Request->Flags &= ~REQUEST_FLAG_FUA;

    SectorCount = Request->DataTransferLength + (DevExt->Device.SectorSize - 1);
    SectorCount /= DevExt->Device.SectorSize;

    if (DevExt->Device.DeviceFlags & DEVICE_NCQ)
    {
        if (!AtaReqBuildNcqReadWriteTaskFile(DevExt, Request, Lba, SectorCount))
            return AtaReqTerminateInvalidRange(Srb);
    }
    else
    {
        if (!AtaReqBuildLbaTaskFile(DevExt, Request, Lba, SectorCount))
            return AtaReqTerminateInvalidRange(Srb);

        if (!(DevExt->Device.DeviceFlags & DEVICE_PIO_ONLY) &&
            !(SRB_GET_FLAGS(Srb) & SRB_FLAG_PIO_RETRY))
        {
            Request->Flags |= REQUEST_DMA_FLAGS;
        }
        else if (DevExt->Device.MultiSectorCount != 0)
        {
            Request->Flags |= REQUEST_FLAG_READ_WRITE_MULTIPLE;
        }

        /* Choose the command opcode */
        Request->TaskFile.Command = AtaReadWriteCommand(Request, &DevExt->Device);
        if (Request->TaskFile.Command == 0)
            return AtaReqTerminateInvalidField(Srb);
    }

    return SRB_STATUS_PENDING;
}

static
UCHAR
AtaReqScsiSynchronizeCache(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR Command;

    if (!AtaDevIsVolatileWriteCacheEnabled(&DevExt->IdentifyDeviceData))
        return SRB_STATUS_SUCCESS;

    Command = AtaDeviceGetFlushCacheCommand(DevExt);
    if (Command == 0)
        return SRB_STATUS_SUCCESS;

    /* Prepare a non-data command */
    Request->TaskFile.Command = Command;

    /* NOTE: This command may take longer than 30 seconds to complete */
    return SRB_STATUS_PENDING;
}

static
UCHAR
AtaReqScsiVerify(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PCDB Cdb;
    ULONG64 Lba;
    ULONG VerificationLength;

    Cdb = (PCDB)Srb->Cdb;

    /* Byte-by-byte comparison not supported */
    if (Cdb->VERIFY16.ByteCheck || Cdb->VERIFY16.BlockVerify)
        return AtaReqTerminateInvalidField(Srb);

    /* Prepare a non-data command */
    switch (Cdb->AsByte[0])
    {
        case SCSIOP_VERIFY:
        {
            Lba = CdbGetLogicalBlockAddress10(Cdb);
            VerificationLength = CdbGetTransferLength10(Cdb);
            break;
        }

        case SCSIOP_VERIFY12:
        {
            Lba = CdbGetLogicalBlockAddress12(Cdb);
            VerificationLength = CdbGetTransferLength12(Cdb);
            break;
        }

        case SCSIOP_VERIFY16:
        {
            Lba = CdbGetLogicalBlockAddress16(Cdb);
            VerificationLength = CdbGetTransferLength16(Cdb);
            break;
        }

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }

    if (!AtaReqBuildLbaTaskFile(DevExt, Request, Lba, VerificationLength))
        return AtaReqTerminateInvalidRange(Srb);

    /* Choose the command opcode */
    if (Request->Flags & REQUEST_FLAG_LBA48)
        Request->TaskFile.Command = IDE_COMMAND_VERIFY_EXT;
    else
        Request->TaskFile.Command = IDE_COMMAND_VERIFY;

    return SRB_STATUS_PENDING;
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteReadCapacity(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    PSCSI_REQUEST_BLOCK Srb = Request->Srb;
    ULONG Length;

    // TODO check for status if (SrbStatus != SRB_STATUS_SUCCESS) return;

    DevExt = CONTAINING_RECORD(Request->Device, ATAPORT_DEVICE_EXTENSION, Device);

    AtaDeviceSetAddressingMode(DevExt);

    if (Srb->Cdb[0] == SCSIOP_READ_CAPACITY)
    {
        PREAD_CAPACITY_DATA CapacityData;
        ULONG MaximumLba;

        MaximumLba = min(DevExt->Device.TotalSectors - 1, MAXULONG);

        CapacityData = DevExt->Device.LocalBuffer;
        CapacityData->LogicalBlockAddress = RtlUlongByteSwap(MaximumLba);
        CapacityData->BytesPerBlock = RtlUlongByteSwap(DevExt->Device.SectorSize);

        Length = sizeof(*CapacityData);
    }
    else // SCSIOP_READ_CAPACITY16
    {
        PREAD_CAPACITY16_DATA CapacityData;
        ULONG LogicalPerPhysicalExponent, LogicalSectorsPerPhysicalSector;
        ULONG LogicalSectorAlignment, LowestAlignedBlock;
        ULONG64 MaximumLba;

        LogicalSectorAlignment = AtaDevLogicalSectorAlignment(&DevExt->IdentifyDeviceData);
        if (LogicalSectorAlignment != 0)
        {
            LogicalSectorsPerPhysicalSector =
                AtaDevLogicalSectorsPerPhysicalSector(&DevExt->IdentifyDeviceData,
                                                      &LogicalPerPhysicalExponent);


            LowestAlignedBlock = (LogicalSectorsPerPhysicalSector - LogicalSectorAlignment);
            LowestAlignedBlock %= LogicalSectorsPerPhysicalSector;
        }
        else
        {
            LowestAlignedBlock = 0;
        }

        MaximumLba = DevExt->Device.TotalSectors - 1;

        CapacityData = DevExt->Device.LocalBuffer;

        RtlZeroMemory(CapacityData, sizeof(*CapacityData));
        CapacityData->LogicalBlockAddress.QuadPart = RtlUlonglongByteSwap(MaximumLba);
        CapacityData->BytesPerBlock = RtlUlongByteSwap(DevExt->Device.SectorSize);
        CapacityData->LogicalPerPhysicalExponent = LogicalPerPhysicalExponent;
        CapacityData->LowestAlignedBlock_MSB = (UCHAR)(LowestAlignedBlock >> 8);
        CapacityData->LowestAlignedBlock_LSB = (UCHAR)LowestAlignedBlock;

        if (AtaDevHasTrimFunction(&DevExt->IdentifyDeviceData))
        {
            if (AtaDevHasDratFunction(&DevExt->IdentifyDeviceData))
            {
                CapacityData->LBPME = 1;

                if (AtaDevHasRzatFunction(&DevExt->IdentifyDeviceData))
                    CapacityData->LBPRZ = 1;
            }
        }

        Length = CdbGetAllocationLength16((PCDB)Srb->Cdb);
    }

    AtaReqCopySatlBuffer(Request, DevExt->Device.LocalBuffer, Length);
    return COMPLETE_IRP;
}

static
UCHAR
AtaReqScsiReadCapacity(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    if (Srb->Cdb[0] == SCSIOP_READ_CAPACITY)
    {
        if (Srb->DataTransferLength < sizeof(READ_CAPACITY_DATA))
            return AtaReqTerminateInvalidField(Srb);
    }
    else // SCSIOP_READ_CAPACITY16
    {
        if (Srb->DataTransferLength < CdbGetAllocationLength16((PCDB)Srb->Cdb))
            return AtaReqTerminateInvalidField(Srb);
    }

    /* Update the identify data */
    if (!AtaReqBuildIdentifyCommand(DevExt, Request))
        return SRB_STATUS_INSUFFICIENT_RESOURCES;

    Request->Complete = AtaReqCompleteReadCapacity;
    return SRB_STATUS_PENDING;
}

static
ULONG
AtaReqControlModePage(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PVOID Buffer)
{
    PMODE_CONTROL_PAGE PageData = Buffer;

    PageData->PageCode = MODE_PAGE_CONTROL;
    PageData->PageLength =
        sizeof(PageData) - RTL_SIZEOF_THROUGH_FIELD(MODE_CONTROL_PAGE, PageLength);

    PageData->QERR = 0; // TODO
    PageData->QueueAlgorithmModifier = 1;
    PageData->BusyTimeoutPeriod[0] = 0xFF;
    PageData->BusyTimeoutPeriod[1] = 0xFF;
    PageData->D_SENSE = !!(DevExt->Device.DeviceFlags & DEVICE_DESCRIPTOR_SENSE);

    return sizeof(PageData);
}

static
ULONG
AtaReqControlExtensionModePage(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PVOID Buffer)
{
    PMODE_CONTROL_EXTENSION_PAGE PageData = Buffer;

    PageData->SubPageFormat = 1;
    PageData->SubPageCode = 0x00;
    PageData->PageCode = MODE_PAGE_CONTROL;
    PageData->PageLength[1] =
        sizeof(PageData) - RTL_SIZEOF_THROUGH_FIELD(MODE_CONTROL_EXTENSION_PAGE, PageLength);

    return sizeof(PageData);
}

static
ULONG
AtaReqRwErrorRecoveryModePage(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PVOID Buffer)
{
    PMODE_READ_WRITE_RECOVERY_PAGE PageData = Buffer;

    PageData->PageCode = MODE_PAGE_ERROR_RECOVERY;
    PageData->PageLength =
        sizeof(PageData) - RTL_SIZEOF_THROUGH_FIELD(MODE_READ_WRITE_RECOVERY_PAGE, PageLength);

    PageData->AWRE = 1;

    return sizeof(PageData);
}

static
ULONG
AtaReqCachingModePage(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PVOID Buffer)
{
    PMODE_CACHING_PAGE_SPC5 PageData = Buffer;

    PageData->PageCode = MODE_PAGE_CACHING;
    PageData->PageLength =
        sizeof(PageData) - RTL_SIZEOF_THROUGH_FIELD(MODE_CACHING_PAGE_SPC5, PageLength);

    if (AtaDevIsVolatileWriteCacheEnabled(&DevExt->IdentifyDeviceData))
        PageData->WriteCacheEnable = 1;

    if (AtaDevIsReadLookAHeadEnabled(&DevExt->IdentifyDeviceData))
        PageData->DisableReadAHead = 1;

    return sizeof(PageData);
}

static
ULONG
AtaReqInformationalExceptionsControlModePage(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PVOID Buffer)
{
    PMODE_INFO_EXCEPTIONS PageData = Buffer;

    PageData->PageCode = MODE_PAGE_FAULT_REPORTING;
    PageData->PageLength =
        sizeof(PageData) - RTL_SIZEOF_THROUGH_FIELD(MODE_INFO_EXCEPTIONS, PageLength);

    /* Only report informational exception condition on request */
    PageData->ReportMethod = 6;

    return sizeof(PageData);
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteModeSense(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    UCHAR PageCode;
    USHORT ModeDataLength;
    PUCHAR Buffer;
    BOOLEAN Is6ByteCommand, IsWriteProtected;
    PCDB Cdb;
    union _HEADER
    {
        PMODE_PARAMETER_HEADER Header6;
        PMODE_PARAMETER_HEADER10 Header10;
    } ModeHeader;

    DevExt = CONTAINING_RECORD(Request->Device, ATAPORT_DEVICE_EXTENSION, Device);

    RtlZeroMemory(DevExt->Device.LocalBuffer, ATA_LOCAL_BUFFER_SIZE);

    Cdb = (PCDB)Request->Srb->Cdb;
    Is6ByteCommand = (Cdb->AsByte[0] == SCSIOP_MODE_SENSE); // todo move to trans fn

    Buffer = DevExt->Device.LocalBuffer;

    /* Media status outputs */
    if (Request->TranslationState != 0)
    {
        IsWriteProtected = FALSE;
        /* IsWriteProtected = (Request->Status & IDE_STATUS_ERROR) && */
                           /* !(Request->Error & IDE_ERROR_COMMAND_ABORTED) && */
                           /* (Request->Error & IDE_ERROR_WRITE_PROTECT); */
    }
    else
    {
        IsWriteProtected = FALSE;
    }

    ModeHeader.Header6 = (PMODE_PARAMETER_HEADER)Buffer;

    /* Mode parameter header */
    if (Is6ByteCommand)
    {
        if (IsWriteProtected)
            ModeHeader.Header6->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
        if (DevExt->Device.DeviceFlags & DEVICE_HAS_FUA)
            ModeHeader.Header6->DeviceSpecificParameter |= MODE_DSP_FUA_SUPPORTED;

        Buffer += sizeof(*ModeHeader.Header6);
    }
    else
    {
        if (IsWriteProtected)
            ModeHeader.Header10->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
        if (DevExt->Device.DeviceFlags & DEVICE_HAS_FUA)
            ModeHeader.Header10->DeviceSpecificParameter |= MODE_DSP_FUA_SUPPORTED;

        Buffer += sizeof(*ModeHeader.Header10);
    }

    /* Short LBA mode parameter block descriptor */
    if (!Cdb->MODE_SENSE.Dbd)
    {
        PFORMAT_DESCRIPTOR FormatDescriptor;

        if (Is6ByteCommand)
        {
            ModeHeader.Header6->BlockDescriptorLength = sizeof(*FormatDescriptor);
        }
        else
        {
            ModeHeader.Header10->BlockDescriptorLength[0] = (UCHAR)(sizeof(*FormatDescriptor) >> 8);
            ModeHeader.Header10->BlockDescriptorLength[1] = (UCHAR)sizeof(*FormatDescriptor);
        }

        FormatDescriptor = (PFORMAT_DESCRIPTOR)Buffer;
        FormatDescriptor->BlockLength[0] = (UCHAR)(DevExt->Device.SectorSize >> 16);
        FormatDescriptor->BlockLength[1] = (UCHAR)(DevExt->Device.SectorSize >> 8);
        FormatDescriptor->BlockLength[2] = (UCHAR)DevExt->Device.SectorSize;

        Buffer += sizeof(*FormatDescriptor);
    }

    PageCode = Cdb->MODE_SENSE.PageCode;

    /* Return SCSI mode pages */
    if (PageCode == MODE_PAGE_CONTROL || PageCode == MODE_SENSE_RETURN_ALL)
    {
        UCHAR SubPageCode = Cdb->MODE_SENSE.Reserved3;

        if (SubPageCode == 0x00 || SubPageCode == 0xFF)
            Buffer += AtaReqControlModePage(DevExt, Buffer);
        if (SubPageCode == 0x01 || SubPageCode == 0xFF)
            Buffer += AtaReqControlExtensionModePage(DevExt, Buffer);
    }
    if (PageCode == MODE_PAGE_ERROR_RECOVERY || PageCode == MODE_SENSE_RETURN_ALL)
    {
        Buffer += AtaReqRwErrorRecoveryModePage(DevExt, Buffer);
    }
    if (PageCode == MODE_PAGE_CACHING || PageCode == MODE_SENSE_RETURN_ALL)
    {
        Buffer += AtaReqCachingModePage(DevExt, Buffer);
    }
    if (PageCode == MODE_PAGE_FAULT_REPORTING || PageCode == MODE_SENSE_RETURN_ALL)
    {
        Buffer += AtaReqInformationalExceptionsControlModePage(DevExt, Buffer);
    }
    if (PageCode == MODE_PAGE_POWER_CONDITION || PageCode == MODE_SENSE_RETURN_ALL)
    {
        //Buffer += AtaReqPowerConditionModePage(ChanExt, DevExt, Buffer); TODO
    }

    ModeDataLength = Buffer - (PUCHAR)DevExt->Device.LocalBuffer;

    if (Is6ByteCommand)
    {
        ModeDataLength -= RTL_SIZEOF_THROUGH_FIELD(MODE_PARAMETER_HEADER, MediumType);

        ModeHeader.Header6->ModeDataLength = ModeDataLength;
    }
    else
    {
        ModeDataLength -= RTL_SIZEOF_THROUGH_FIELD(MODE_PARAMETER_HEADER10, MediumType);

        ModeHeader.Header10->ModeDataLength[0] = (UCHAR)(ModeDataLength >> 8);
        ModeHeader.Header10->ModeDataLength[1] = (UCHAR)ModeDataLength;
    }

    AtaReqCopySatlBuffer(Request,
                         DevExt->Device.LocalBuffer,
                         Buffer - (PUCHAR)DevExt->Device.LocalBuffer);
    return COMPLETE_IRP;
}

static
UCHAR
AtaReqScsiModeSense(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN Is6ByteCommand)
{
    ULONG AllocationLength;
    PCDB Cdb;

    Cdb = (PCDB)Srb->Cdb;

    if (Is6ByteCommand)
        AllocationLength = CdbGetAllocationLength6(Cdb); // todo not needed
    else
        AllocationLength = CdbGetAllocationLength10(Cdb);

    if (Srb->DataTransferLength < AllocationLength)
        return AtaReqTerminateInvalidField(Srb);

    if (Cdb->MODE_SENSE.Pc == MODE_SENSE_SAVED_VALUES)
        return AtaReqTerminateInvalidField(Srb);

    switch (Cdb->MODE_SENSE.PageCode)
    {
        case MODE_PAGE_FAULT_REPORTING:
        {
            if (!AtaDevHasSmartFeature(&DevExt->IdentifyDeviceData))
                return AtaReqTerminateInvalidField(Srb);

            break;
        }

        case MODE_PAGE_CONTROL:
        {
            UCHAR SubPageCode = Cdb->MODE_SENSE.Reserved3;

            if (SubPageCode != 0x00 && SubPageCode != 0x01 && SubPageCode != 0xFF)
                return AtaReqTerminateInvalidField(Srb);

            break;
        }

        case MODE_PAGE_ERROR_RECOVERY:
        case MODE_PAGE_POWER_CONDITION:
        case MODE_PAGE_CACHING:
        case MODE_SENSE_RETURN_ALL:
            break;

        default:
            return AtaReqTerminateInvalidField(Srb);
    }

    /* Update the identify data */
    if (!AtaReqBuildIdentifyCommand(DevExt, Request))
        return SRB_STATUS_INSUFFICIENT_RESOURCES;

    Request->Complete = AtaReqCompleteModeSense;

    return SRB_STATUS_PENDING;
}

static
UCHAR
AtaReqScsiModeSelect(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN Is6ByteCommand)
{
    // TODO
    return AtaReqTerminateInvalidField(Srb);
}

CODE_SEG("PAGE")
VOID
AtaCreateStandardInquiryData(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PIDENTIFY_DEVICE_DATA IdentifyData = &DevExt->IdentifyDeviceData;
    PINQUIRYDATA InquiryData = &DevExt->InquiryData;
    ULONG Offset;
    PUCHAR VersionDescriptor;

    PAGED_CODE();

    InquiryData->DeviceType = DIRECT_ACCESS_DEVICE;
    InquiryData->RemovableMedia = AtaDevIsRemovable(IdentifyData);
    InquiryData->Versions = 0x07; // SPC-5
    InquiryData->ResponseDataFormat = 2; // This means "Complies to this standard"

    if (DevExt->Device.DeviceFlags & DEVICE_NCQ)
        InquiryData->CommandQueue = 1;

    /* T10 vendor ID */
    RtlCopyMemory(InquiryData->VendorId, "ATA     ", RTL_FIELD_SIZE(INQUIRYDATA, VendorId));

    /* Product ID */
    AtaCopyIdStringUnsafe(InquiryData->ProductId,
                          IdentifyData->ModelNumber,
                          RTL_FIELD_SIZE(INQUIRYDATA, ProductId));

    /* Product revision level */
    if (IdentifyData->FirmwareRevision[4] == ' ' &&
        IdentifyData->FirmwareRevision[5] == ' ' &&
        IdentifyData->FirmwareRevision[6] == ' ' &&
        IdentifyData->FirmwareRevision[7] == ' ')
    {
        Offset = 0;
    }
    else
    {
        Offset = 4;
    }
    AtaCopyIdStringUnsafe(InquiryData->ProductRevisionLevel,
                          &IdentifyData->FirmwareRevision[Offset],
                          RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel));

    VersionDescriptor = &InquiryData->Reserved3[3];

    /* SAM-5 (no version claimed) */
    *VersionDescriptor++ = 0x00;
    *VersionDescriptor++ = 0xA0;

    /* SPC-5 (no version claimed) */
    *VersionDescriptor++ = 0x05;
    *VersionDescriptor++ = 0xC0;

    /* SBC-4 (no version claimed) */
    *VersionDescriptor++ = 0x06;
    *VersionDescriptor++ = 0x00;

    if (AtaDevIsZonedDevice(IdentifyData))
    {
        /* ZBC BSR INCITS 536 revision 05 */
        *VersionDescriptor++ = 0x06;
        *VersionDescriptor++ = 0x24;
    }

    if (AtaDevHasIeee1667(IdentifyData))
    {
        /* IEEE 1667 (no version claimed) */
        *VersionDescriptor++ = 0xFF;
        *VersionDescriptor++ = 0xC0;
    }

    if (IdentifyData->MajorRevision != 0xFFFF)
    {
        if (IdentifyData->MajorRevision & (1 << 8))
        {
            /* ATA/ATAPI-8 ATA8-ACS ATA/ATAPI Command Set (no version claimed) */
            *VersionDescriptor++ = 0x16;
            *VersionDescriptor++ = 0x23;
        }
        else if (IdentifyData->MajorRevision & (1 << 7))
        {
            if (IdentifyData->MinorRevision == 0x1D)
            {
                /* ATA/ATAPI-7 INCITS 397-2005 */
                *VersionDescriptor++ = 0x16;
                *VersionDescriptor++ = 0x1C;
            }
            else
            {
                /* ATA/ATAPI-7 (no version claimed) */
                *VersionDescriptor++ = 0x16;
                *VersionDescriptor++ = 0x00;
            }
        }
        else if (IdentifyData->MajorRevision & (1 << 6))
        {
            if (IdentifyData->MinorRevision == 0x22)
            {
                /* ATA/ATAPI-6 INCITS 361-2002 */
                *VersionDescriptor++ = 0x15;
                *VersionDescriptor++ = 0xFD;
            }
            else
            {
                /* ATA/ATAPI-6 (no version claimed) */
                *VersionDescriptor++ = 0x15;
                *VersionDescriptor++ = 0xE0;
            }
        }
    }

    InquiryData->AdditionalLength =
        ((ULONG_PTR)VersionDescriptor - (ULONG_PTR)InquiryData) -
        RTL_SIZEOF_THROUGH_FIELD(INQUIRYDATA, AdditionalLength);

    TRACE("VendorId: '%.*s'\n",
          RTL_FIELD_SIZE(INQUIRYDATA, VendorId), InquiryData->VendorId);
    TRACE("ProductId: '%.*s'\n",
          RTL_FIELD_SIZE(INQUIRYDATA, ProductId), InquiryData->ProductId);
    TRACE("ProductRevisionLevel: '%.*s'\n",
          RTL_FIELD_SIZE(INQUIRYDATA, ProductRevisionLevel), InquiryData->ProductRevisionLevel);
}

static
ULONG
AtaReqScsiReportLuns(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PCDB Cdb = (PCDB)Srb->Cdb;
    PLUN_LIST LunList;
    ULONG Length, LunListLength;

    Length = Cdb->REPORT_LUNS.AllocationLength[0] << 24 |
             Cdb->REPORT_LUNS.AllocationLength[1] << 16 |
             Cdb->REPORT_LUNS.AllocationLength[2] << 8 |
             Cdb->REPORT_LUNS.AllocationLength[3];

    if (Srb->DataTransferLength < Length)
        return AtaReqTerminateInvalidField(Srb);

    LunList = DevExt->Device.LocalBuffer;
    LunListLength = RTL_FIELD_SIZE(LUN_LIST, Lun[0]);

    RtlZeroMemory(LunList, sizeof(*LunList));
    LunList->LunListLength[0] = (UCHAR)(LunListLength >> 24);
    LunList->LunListLength[1] = (UCHAR)(LunListLength >> 16);
    LunList->LunListLength[2] = (UCHAR)(LunListLength >> 8);
    LunList->LunListLength[3] = (UCHAR)LunListLength;

    return AtaReqCopySatlBuffer(Request, DevExt->Device.LocalBuffer, Length);
}

static
VOID
AtaReqScsiInquirySupportedPages(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_SUPPORTED_PAGES_PAGE SupportedPages = DevExt->Device.LocalBuffer;
    ULONG i = 0;

    RtlZeroMemory(SupportedPages, sizeof(*SupportedPages));
    SupportedPages->SupportedPageList[i++] = VPD_SUPPORTED_PAGES;
    SupportedPages->SupportedPageList[i++] = VPD_SERIAL_NUMBER;
    SupportedPages->SupportedPageList[i++] = VPD_DEVICE_IDENTIFIERS;
    //SupportedPages->SupportedPageList[i++] = VPD_EXTENDED_INQUIRY_DATA; // TODO
    SupportedPages->SupportedPageList[i++] = VPD_ATA_INFORMATION;
    SupportedPages->SupportedPageList[i++] = VPD_BLOCK_LIMITS;
    SupportedPages->SupportedPageList[i++] = VPD_BLOCK_DEVICE_CHARACTERISTICS;
    SupportedPages->SupportedPageList[i++] = VPD_LOGICAL_BLOCK_PROVISIONING;
    SupportedPages->PageLength = i;
}

static
VOID
AtaReqScsiInquirySerialNumber(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_SERIAL_NUMBER_PAGE SerialNumberPage = DevExt->Device.LocalBuffer;

    RtlZeroMemory(SerialNumberPage, FIELD_OFFSET(VPD_SERIAL_NUMBER_PAGE, SerialNumber));
    SerialNumberPage->PageLength = RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber);

    AtaCopyIdStringUnsafe(SerialNumberPage->SerialNumber,
                          DevExt->IdentifyDeviceData.SerialNumber,
                          RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber));
}

static
VOID
AtaReqScsiInquiryDeviceIdentifiers(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_IDENTIFICATION_PAGE DeviceIdentificationPage = DevExt->Device.LocalBuffer;
    PVPD_IDENTIFICATION_DESCRIPTOR Descriptor;
    UCHAR PageLength;

    PageLength = FIELD_OFFSET(VPD_IDENTIFICATION_PAGE, Descriptors) +
                 FIELD_OFFSET(VPD_IDENTIFICATION_DESCRIPTOR, Identifier);

    if (AtaDevHasWorldWideName(&DevExt->IdentifyDeviceData))
    {
        PageLength += RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, WorldWideName);
    }
    else
    {
        PageLength += RTL_FIELD_SIZE(INQUIRYDATA, VendorId) +
                      RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber) +
                      RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber);
    }

    RtlZeroMemory(DeviceIdentificationPage, PageLength);
    DeviceIdentificationPage->PageLength =
        PageLength - RTL_SIZEOF_THROUGH_FIELD(VPD_IDENTIFICATION_PAGE, PageLength);

    Descriptor = (PVPD_IDENTIFICATION_DESCRIPTOR)&DeviceIdentificationPage->Descriptors[0];

    if (AtaDevHasWorldWideName(&DevExt->IdentifyDeviceData))
    {
        /* NAA descriptor */
        Descriptor->CodeSet = VpdCodeSetBinary;
        Descriptor->IdentifierType = VpdIdentifierTypeFCPHName;
        Descriptor->IdentifierLength = RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, WorldWideName);

        AtaCopyIdStringUnsafe(Descriptor->Identifier,
                              (PUCHAR)DevExt->IdentifyDeviceData.WorldWideName,
                              RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, WorldWideName));
    }
    else
    {
        PUCHAR Identifier;

        /* T10 vendor ID based descriptor */
        Descriptor->CodeSet = VpdCodeSetAscii;
        Descriptor->IdentifierType = VpdIdentifierTypeVendorId;
        Descriptor->IdentifierLength = RTL_FIELD_SIZE(INQUIRYDATA, VendorId) +
                                       RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber) +
                                       RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber);

        Identifier = &Descriptor->Identifier[0];

        /* T10 vendor ID */
        RtlCopyMemory(Identifier, "ATA     ", RTL_FIELD_SIZE(INQUIRYDATA, VendorId));
        Identifier += RTL_FIELD_SIZE(INQUIRYDATA, VendorId);

        /* Model number field */
        AtaCopyIdStringUnsafe(Identifier,
                              DevExt->IdentifyDeviceData.ModelNumber,
                              RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber));
        Identifier += RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber);

        /* Serial number field */
        AtaCopyIdStringUnsafe(Identifier,
                              DevExt->IdentifyDeviceData.SerialNumber,
                              RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber));
    }
}

static
VOID
AtaReqScsiInquiryBlockLimits(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_BLOCK_LIMITS_PAGE BlockLimitsPage = DevExt->Device.LocalBuffer;
    USHORT PageLength;

    PageLength = sizeof(*BlockLimitsPage) -
                 RTL_SIZEOF_THROUGH_FIELD(VPD_BLOCK_LIMITS_PAGE, PageLength);

    RtlZeroMemory(BlockLimitsPage, sizeof(*BlockLimitsPage));
    BlockLimitsPage->PageLength[0] = (UCHAR)(PageLength >> 8);
    BlockLimitsPage->PageLength[1] = (UCHAR)PageLength;

/* FIXME
    if (AtaDevHasTrimFunction(&DevExt->IdentifyDeviceData))
    {
        BlockLimitsPage->MaximumUnmapLBACount[0] = 0;
        BlockLimitsPage->MaximumUnmapLBACount[1] = 0;
        BlockLimitsPage->MaximumUnmapLBACount[2] = 0;
        BlockLimitsPage->MaximumUnmapLBACount[4] = 0;

        BlockLimitsPage->MaximumUnmapBlockDescriptorCount[0] = 0;
        BlockLimitsPage->MaximumUnmapBlockDescriptorCount[1] = 0;
        BlockLimitsPage->MaximumUnmapBlockDescriptorCount[2] = 0;
        BlockLimitsPage->MaximumUnmapBlockDescriptorCount[3] = 0;
    }
*/
}

static
VOID
AtaReqScsiInquiryBlockDeviceCharacteristics(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE Characteristics = DevExt->Device.LocalBuffer;
    USHORT MediumRotationRate;

    RtlZeroMemory(Characteristics, sizeof(*Characteristics));
    Characteristics->PageLength =
        sizeof(*Characteristics) -
        RTL_SIZEOF_THROUGH_FIELD(VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE, PageLength);

    MediumRotationRate = AtaDevMediumRotationRate(&DevExt->IdentifyDeviceData);
    Characteristics->MediumRotationRateMsb = (UCHAR)(MediumRotationRate >> 8);
    Characteristics->MediumRotationRateLsb = (UCHAR)MediumRotationRate;
    Characteristics->NominalFormFactor = AtaDevNominalFormFactor(&DevExt->IdentifyDeviceData);
    Characteristics->ZONED = AtaDevZonedCapabilities(&DevExt->IdentifyDeviceData);
}

static
VOID
AtaReqScsiInquiryLogicalBlockProvisioning(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE LogicalBlockProvisioningPage = DevExt->Device.LocalBuffer;

    RtlZeroMemory(LogicalBlockProvisioningPage,
                  FIELD_OFFSET(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, ProvisioningGroupDescr));
    LogicalBlockProvisioningPage->PageLength[1] =
        FIELD_OFFSET(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, ProvisioningGroupDescr) -
        RTL_SIZEOF_THROUGH_FIELD(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, PageLength);

    if (AtaDevHasTrimFunction(&DevExt->IdentifyDeviceData))
    {
/* TODO
        LogicalBlockProvisioningPage->LBPU = 1; // unmap
        LogicalBlockProvisioningPage->LBPWS = 1; // write same 16 + unmap
        LogicalBlockProvisioningPage->LBPWS10 = 1; // write same 10 + unmap
*/
        if (AtaDevHasDratFunction(&DevExt->IdentifyDeviceData))
            LogicalBlockProvisioningPage->ANC_SUP = 1;

        if (AtaDevHasRzatFunction(&DevExt->IdentifyDeviceData))
            LogicalBlockProvisioningPage->LBPRZ = 1;
    }
}

static
UCHAR
AtaReqScsiInquiry(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PVPD_SUPPORTED_PAGES_PAGE Buffer;
    PCDB Cdb = (PCDB)Srb->Cdb;
    ULONG Length;

    Length = (Cdb->AsByte[3] << 8) | Cdb->AsByte[4]; // TODO not needed?

    if (Srb->DataTransferLength < Length)
        return AtaReqTerminateInvalidField(Srb);

    /* Return the standard INQUIRY data */
    if (!Cdb->CDB6INQUIRY3.EnableVitalProductData)
    {
        if (Cdb->CDB6INQUIRY3.PageCode != 0)
            return AtaReqTerminateInvalidField(Srb);

        return AtaReqCopySatlBuffer(Request,
                                    &DevExt->InquiryData,
                                    sizeof(DevExt->InquiryData));
    }

    switch (Cdb->CDB6INQUIRY3.PageCode)
    {
        case VPD_SUPPORTED_PAGES:
        {
            AtaReqScsiInquirySupportedPages(DevExt);
            break;
        }
        case VPD_SERIAL_NUMBER:
        {
            AtaReqScsiInquirySerialNumber(DevExt);
            break;
        }
        case VPD_DEVICE_IDENTIFIERS:
        {
            AtaReqScsiInquiryDeviceIdentifiers(DevExt);
            break;
        }
        case VPD_EXTENDED_INQUIRY_DATA:
        {
            return AtaReqTerminateInvalidField(Srb); // TODO
        }
        case VPD_ATA_INFORMATION:
        {
            return AtaReqTerminateInvalidField(Srb); // TODO
        }
        case VPD_BLOCK_LIMITS:
        {
            AtaReqScsiInquiryBlockLimits(DevExt);
            break;
        }
        case VPD_BLOCK_DEVICE_CHARACTERISTICS:
        {
            AtaReqScsiInquiryBlockDeviceCharacteristics(DevExt);
            break;
        }
        case VPD_LOGICAL_BLOCK_PROVISIONING:
        {
            AtaReqScsiInquiryLogicalBlockProvisioning(DevExt);
            break;
        }

        default:
            return AtaReqTerminateInvalidField(Srb);
    }

    /* Data bytes common to all VPD pages */
    Buffer = DevExt->Device.LocalBuffer;
    Buffer->DeviceType = DevExt->InquiryData.DeviceType;
    Buffer->DeviceTypeQualifier = DevExt->InquiryData.DeviceTypeQualifier;
    Buffer->PageCode = Cdb->CDB6INQUIRY3.PageCode;

    return AtaReqCopySatlBuffer(Request, DevExt->Device.LocalBuffer, Length);
}

static
UCHAR
AtaReqScsiTestUnitReady(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    // FIXME
    return SRB_STATUS_SUCCESS;
}

static
UCHAR
AtaReqScsiMediumRemoval(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PCDB Cdb = (PCDB)Srb->Cdb;

    if (AtaDevHasRemovableMediaFeature(&DevExt->IdentifyDeviceData))
    {
        if (Cdb->MEDIA_REMOVAL.Prevent)
            Request->TaskFile.Command = IDE_COMMAND_DOOR_LOCK;
        else
            Request->TaskFile.Command = IDE_COMMAND_DOOR_UNLOCK;
        return SRB_STATUS_PENDING;
    }

    /* Pretend success */
    return SRB_STATUS_SUCCESS;
}

static
UCHAR
AtaReqScsiStartStopUnit(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PCDB Cdb = (PCDB)Srb->Cdb;

    if (Cdb->START_STOP.LoadEject)
    {
        Request->TaskFile.Command = IDE_COMMAND_MEDIA_EJECT;
        return SRB_STATUS_PENDING;
    }

    /* Adding more handling will conflict with the power manager, assume success */
    return SRB_STATUS_SUCCESS;
}

static
UCHAR
AtaReqScsiRequestSense(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    SCSI_SENSE_CODE SenseCode;

    SenseCode.SrbStatus = SRB_STATUS_SUCCESS;
    SenseCode.SenseKey = SCSI_SENSE_NO_SENSE;
    SenseCode.AdditionalSenseCode = SCSI_ADSENSE_NO_SENSE;
    SenseCode.AdditionalSenseCodeQualifier = 0;

    return AtaReqSetFixedSenseData(Srb, SenseCode);
}


static
BOOLEAN
AtaReqFilterPassThrough(
    _In_ PATA_TASKFILE TaskFile)
{
    switch (TaskFile->Command)
    {
        case IDE_COMMAND_SET_FEATURE:
        {
            if (TaskFile->Feature == IDE_FEATURE_SET_TRANSFER_MODE)
                return FALSE;

            break;
        }

        case IDE_COMMAND_SET_DRIVE_PARAMETERS:
        case IDE_COMMAND_SET_MULTIPLE:
            return FALSE;

        // TODO: Anything else to check?
        default:
            break;
    }

    return TRUE;
}

static
VOID
AtaReqSetAtaStatusSenseData(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR SK,
    _In_ UCHAR ASK,
    _In_ UCHAR ASCQ)
{
    PATA_TASKFILE TaskFile = &Request->Output;
    PSCSI_REQUEST_BLOCK Srb = Request->Srb;
    PDESCRIPTOR_SENSE_DATA SenseData;
    ULONG BufferLength;
    PSCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN Descriptor;

    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

    BufferLength = FIELD_OFFSET(DESCRIPTOR_SENSE_DATA, DescriptorBuffer);
    BufferLength += sizeof(*Descriptor);
    if (Srb->SenseInfoBufferLength < BufferLength)
        return;

    SenseData = Srb->SenseInfoBuffer;
    RtlZeroMemory(SenseData, BufferLength);

    /* Descriptor format sense data */
    SenseData->ErrorCode = SCSI_SENSE_ERRORCODE_DESCRIPTOR_CURRENT;
    SenseData->SenseKey = SK;
    SenseData->AdditionalSenseCode = ASK;
    SenseData->AdditionalSenseCodeQualifier = ASCQ;

    SenseData->AdditionalSenseLength =
        BufferLength - RTL_SIZEOF_THROUGH_FIELD(DESCRIPTOR_SENSE_DATA, AdditionalSenseLength);

    /* ATA Status Return sense data descriptor */
    Descriptor = (PSCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN)&SenseData->DescriptorBuffer[0];
    Descriptor->Header.DescriptorType = SCSI_SENSE_DESCRIPTOR_TYPE_ATA_STATUS_RETURN;
    Descriptor->Header.AdditionalLength = 0x0C;

    /* Return the 28-bit extended status */
    Descriptor->Error = TaskFile->Error;
    Descriptor->Device = TaskFile->DriveSelect;
    Descriptor->Status = TaskFile->Status;
    Descriptor->SectorCount7_0 = TaskFile->SectorCount;
    Descriptor->LbaLow7_0 = TaskFile->LowLba;
    Descriptor->LbaMid7_0 = TaskFile->MidLba;
    Descriptor->LbaHigh7_0 = TaskFile->HighLba;
    Descriptor->LbaLow15_8 = TaskFile->DriveSelect & 0x0F;

    /* Return the 48-bit extended status */
    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        Descriptor->Extend = 1;
        Descriptor->SectorCount15_8 = TaskFile->SectorCountEx;
        Descriptor->LbaLow15_8 = TaskFile->LowLbaEx;
        Descriptor->LbaMid15_8 = TaskFile->MidLba;
        Descriptor->LbaHigh15_8 = TaskFile->HighLbaEx;
    }

    Request->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
}

static
VOID
AtaReqSetAtaFixedSenseData(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR SK,
    _In_ UCHAR ASK,
    _In_ UCHAR ASCQ)
{
    PATA_TASKFILE TaskFile = &Request->Output;
    PSCSI_REQUEST_BLOCK Srb = Request->Srb;
    PSENSE_DATA SenseData;
    UCHAR Byte;

    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

    if (Srb->SenseInfoBufferLength < sizeof(*SenseData))
        return;

    SenseData = Srb->SenseInfoBuffer;
    RtlZeroMemory(SenseData, sizeof(*SenseData));

    /* Descriptor format sense data */
    SenseData->ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
    SenseData->SenseKey = SK;
    SenseData->AdditionalSenseCode = ASK;
    SenseData->AdditionalSenseCodeQualifier = ASCQ;

    SenseData->Information[0] = TaskFile->Error;
    SenseData->Information[1] = TaskFile->Status;
    SenseData->Information[2] = TaskFile->DriveSelect;
    SenseData->Information[3] = TaskFile->SectorCount;

    Byte = 0;

    /* Extend */
    if (Request->Flags & REQUEST_FLAG_LBA48)
        Byte |= 0x80;

    /* Count upper nonzero */
    if (Request->TaskFile.SectorCountEx != 0)
        Byte |= 0x40;

    /* LBA upper nonzero */
    if (TaskFile->LowLbaEx != 0 || TaskFile->LowLbaEx != 0 || TaskFile->HighLbaEx != 0)
        Byte |= 0x20;

    SenseData->CommandSpecificInformation[0] = Byte;
    SenseData->CommandSpecificInformation[1] = TaskFile->LowLba;
    SenseData->CommandSpecificInformation[2] = TaskFile->MidLba;
    SenseData->CommandSpecificInformation[3] = TaskFile->HighLba;

    Request->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteAtaPassThrough(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PCDB Cdb = (PCDB)Request->Srb->Cdb;

    // todo check has taskfile

    if ((Cdb->ATA_PASSTHROUGH16.CkCond == 1) || (Request->SrbStatus != SRB_STATUS_SUCCESS))
    {
        UCHAR SK, ASK, ASCQ;

        if (Request->SrbStatus != SRB_STATUS_SUCCESS)
        {
            AtaReqTranslateFixedError(Request, &SK, &ASK, &ASCQ);
        }
        else
        {
            SK = SCSI_SENSE_RECOVERED_ERROR;
            ASK = SCSI_ADSENSE_NO_SENSE;
            ASCQ = SCSI_SENSEQ_ATA_PASS_THROUGH_INFORMATION_AVAILABLE;
        }

        /* if (Request->Device->DeviceFlags & DEVICE_DESCRIPTOR_SENSE) */
            AtaReqSetAtaStatusSenseData(Request, SK, ASK, ASCQ);
        /* else */
            /* AtaReqSetAtaFixedSenseData(Request, SK, ASK, ASCQ); */
        UNREFERENCED_PARAMETER(AtaReqSetAtaFixedSenseData);
    }

    return COMPLETE_IRP;
}

static
UCHAR
AtaReqScsiAtaPassThrough(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    PCDB Cdb;

    Request->Flags = REQUEST_FLAG_SET_DEVICE_REGISTER |
                     REQUEST_FLAG_SAVE_TASK_FILE |
                     REQUEST_FLAG_PASSTHROUGH |
                     (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION);

    Cdb = (PCDB)Srb->Cdb;
    switch (Cdb->ATA_PASSTHROUGH16.Protocol)
    {
        case ATA_PASSTHROUGH_PROTOCOL_NON_DATA:
        {
            /* Check for data transfer */
            if (Cdb->ATA_PASSTHROUGH16.TLength != 0)
                return AtaReqTerminateInvalidField(Srb);

            break;
        }

        case ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_IN:
        {
            /* Check for write operation */
            if (!Cdb->ATA_PASSTHROUGH16.TDir)
                return AtaReqTerminateInvalidField(Srb);

            break;
        }

        case ATA_PASSTHROUGH_PROTOCOL_PIO_DATA_OUT:
        {
            /* Check for read operation */
            if (Cdb->ATA_PASSTHROUGH16.TDir)
                return AtaReqTerminateInvalidField(Srb);

            break;
        }

        case ATA_PASSTHROUGH_PROTOCOL_DMA:
        case ATA_PASSTHROUGH_PROTOCOL_UDMA_DATA_IN:
        case ATA_PASSTHROUGH_PROTOCOL_UDMA_DATA_OUT:
        {
            if (DevExt->Device.DeviceFlags & DEVICE_PIO_ONLY)
                return AtaReqTerminateInvalidField(Srb);

            Request->Flags |= REQUEST_DMA_FLAGS;
            break;
        }

        case ATA_PASSTHROUGH_PROTOCOL_NCQ:
        {
            if (!(DevExt->Device.DeviceFlags & DEVICE_NCQ))
                return AtaReqTerminateInvalidField(Srb);

            /*
             * Do not set the REQUEST_FLAG_LBA48 flag.
             * We can get here even when the command is SCSIOP_ATA_PASSTHROUGH12.
             */
            Request->Flags |= REQUEST_FLAG_NCQ | REQUEST_DMA_FLAGS;
            break;
        }

        default:
            return AtaReqTerminateInvalidField(Srb);
    }

    /*
     * NOTE: We ignore the T_LENGTH field in the CDB
     * and use the DataTransferLength field instead.
     */
    if ((Cdb->ATA_PASSTHROUGH16.TLength == 0) && (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION))
        return AtaReqTerminateInvalidField(Srb);

    if (Srb->Cdb[0] == SCSIOP_ATA_PASSTHROUGH12)
    {
        TaskFile->Feature     = Cdb->ATA_PASSTHROUGH12.Features;
        TaskFile->SectorCount = Cdb->ATA_PASSTHROUGH12.SectorCount;
        TaskFile->LowLba      = Cdb->ATA_PASSTHROUGH12.LbaLow;
        TaskFile->MidLba      = Cdb->ATA_PASSTHROUGH12.LbaMid;
        TaskFile->HighLba     = Cdb->ATA_PASSTHROUGH12.LbaHigh;
        TaskFile->DriveSelect = Cdb->ATA_PASSTHROUGH12.Device;
        TaskFile->Command     = Cdb->ATA_PASSTHROUGH12.Command;
    }
    else
    {
        TaskFile->Feature     = Cdb->ATA_PASSTHROUGH16.Features7_0;
        TaskFile->SectorCount = Cdb->ATA_PASSTHROUGH16.SectorCount7_0;
        TaskFile->LowLba      = Cdb->ATA_PASSTHROUGH16.LbaLow7_0;
        TaskFile->MidLba      = Cdb->ATA_PASSTHROUGH16.LbaMid7_0;
        TaskFile->HighLba     = Cdb->ATA_PASSTHROUGH16.LbaHigh7_0;
        TaskFile->DriveSelect = Cdb->ATA_PASSTHROUGH16.Device;
        TaskFile->Command     = Cdb->ATA_PASSTHROUGH16.Command;

        if (Cdb->ATA_PASSTHROUGH16.Extend)
        {
            TaskFile->FeatureEx     = Cdb->ATA_PASSTHROUGH16.Features15_8;
            TaskFile->SectorCountEx = Cdb->ATA_PASSTHROUGH16.SectorCount15_8;
            TaskFile->LowLbaEx      = Cdb->ATA_PASSTHROUGH16.LbaLow15_8;
            TaskFile->MidLbaEx      = Cdb->ATA_PASSTHROUGH16.LbaMid15_8;
            TaskFile->HighLbaEx     = Cdb->ATA_PASSTHROUGH16.LbaHigh15_8;

            Request->Flags |= REQUEST_FLAG_LBA48;
        }
    }

    if (!AtaReqFilterPassThrough(TaskFile))
        return AtaReqTerminateInvalidField(Srb);

    /* We can set the unique queue tag only after having the slot allocation done */
    if (Request->Flags & REQUEST_FLAG_NCQ)
        TaskFile->SectorCount &= ~0xF8;

    /* Set the master/slave bit to the correct value */
    TaskFile->DriveSelect &= ~IDE_DRIVE_SELECT_SLAVE;
    TaskFile->DriveSelect |= DevExt->Device.DeviceSelect & IDE_DRIVE_SELECT_SLAVE;

    TRACE("TF: Send %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          TaskFile->Command,
          TaskFile->Feature,
          TaskFile->LowLba,
          TaskFile->MidLba,
          TaskFile->HighLba,
          TaskFile->DriveSelect,
          TaskFile->SectorCount);

    Request->Complete = AtaReqCompleteAtaPassThrough;
    return SRB_STATUS_PENDING;
}

static
UCHAR
AtaReqExecuteScsiAta(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    switch (Srb->Cdb[0])
    {
        case SCSIOP_REPORT_LUNS:
            return AtaReqScsiReportLuns(DevExt, Request, Srb);

        case SCSIOP_INQUIRY:
            return AtaReqScsiInquiry(DevExt, Request, Srb);

        case SCSIOP_TEST_UNIT_READY:
            return AtaReqScsiTestUnitReady(DevExt, Request, Srb);

        case SCSIOP_MEDIUM_REMOVAL:
            return AtaReqScsiMediumRemoval(DevExt, Request, Srb);

        case SCSIOP_START_STOP_UNIT:
            return AtaReqScsiStartStopUnit(DevExt, Request, Srb);

        case SCSIOP_REQUEST_SENSE:
            return AtaReqScsiRequestSense(DevExt, Request, Srb);

        case SCSIOP_MODE_SENSE:
        case SCSIOP_MODE_SENSE10:
            return AtaReqScsiModeSense(DevExt,
                                       Request,
                                       Srb,
                                       (Srb->Cdb[0] == SCSIOP_MODE_SENSE));

        case SCSIOP_MODE_SELECT:
        case SCSIOP_MODE_SELECT10:
            return AtaReqScsiModeSelect(DevExt,
                                        Request,
                                        Srb,
                                        (Srb->Cdb[0] == SCSIOP_MODE_SELECT));

        case SCSIOP_SERVICE_ACTION_IN16:
        {
            UCHAR ServiceAction = ((PCDB)Srb->Cdb)->READ_CAPACITY16.ServiceAction;

            if (ServiceAction != SERVICE_ACTION_READ_CAPACITY16)
                break;

            __fallthrough;
        }
        case SCSIOP_READ_CAPACITY:
            return AtaReqScsiReadCapacity(DevExt, Request, Srb);

        case SCSIOP_READ6:
        case SCSIOP_WRITE6:
        case SCSIOP_READ:
        case SCSIOP_WRITE:
        case SCSIOP_READ12:
        case SCSIOP_WRITE12:
        case SCSIOP_READ16:
        case SCSIOP_WRITE16:
            return AtaReqScsiReadWrite(DevExt, Request, Srb);

        case SCSIOP_SYNCHRONIZE_CACHE:
        case SCSIOP_SYNCHRONIZE_CACHE16:
            return AtaReqScsiSynchronizeCache(DevExt, Request, Srb);

        case SCSIOP_VERIFY:
        case SCSIOP_VERIFY12:
        case SCSIOP_VERIFY16:
            return AtaReqScsiVerify(DevExt, Request, Srb);

        case SCSIOP_ATA_PASSTHROUGH12:
        case SCSIOP_ATA_PASSTHROUGH16:
            return AtaReqScsiAtaPassThrough(DevExt, Request, Srb);

        // TODO: more commands

        default:
            break;
    }

    WARN("Unknown command %02x\n", Srb->Cdb[0]);

    return AtaReqTerminateInvalidOpCode(Srb);
}

static
UCHAR
AtaReqPreparePacketCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    /* Prepare a packet command */
    Request->Flags = REQUEST_FLAG_PACKET_COMMAND |
                     (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION);

    if ((Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) &&
        AtaPacketCommandUseDma(Srb->Cdb[0]) &&
        !(DevExt->Device.DeviceFlags & DEVICE_PIO_ONLY) &&
        !(SRB_GET_FLAGS(Srb) & SRB_FLAG_PIO_RETRY))
    {
        Request->Flags |= REQUEST_DMA_FLAGS;
    }

    RtlCopyMemory(Request->Cdb, Srb->Cdb, RTL_FIELD_SIZE(SCSI_REQUEST_BLOCK, Cdb));

    return SRB_STATUS_PENDING;
}

static
UCHAR
AtaReqExecuteScsiAtapi(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    switch (Srb->Cdb[0])
    {
        case SCSIOP_ATA_PASSTHROUGH12:
        case SCSIOP_ATA_PASSTHROUGH16:
            return AtaReqScsiAtaPassThrough(DevExt, Request, Srb);

        case SCSIOP_GET_CONFIGURATION:
            /* FIXME: NP21/W emulation bugs */
            if (IsNEC_98)
                return AtaReqTerminateInvalidOpCode(Srb);
            break;

        // case SCSIOP_INQUIRY: TODO Xbox

        default:
            break;
    }

    return AtaReqPreparePacketCommand(DevExt, Request, Srb);
}

UCHAR
AtaReqExecuteScsi(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    if (IS_ATAPI(&DevExt->Device))
        return AtaReqExecuteScsiAtapi(DevExt, Request, Srb);
    else
        return AtaReqExecuteScsiAta(DevExt, Request, Srb);
}
