/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI/ATA Translation layer
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

const UCHAR AtapAtaCommand[12][2] =
{
    /* Write EXT                          Read EXT */
    { IDE_COMMAND_WRITE_EXT,              IDE_COMMAND_READ_EXT          },
    { IDE_COMMAND_WRITE_MULTIPLE_EXT,     IDE_COMMAND_READ_MULTIPLE_EXT },
    { IDE_COMMAND_WRITE_DMA_EXT,          IDE_COMMAND_READ_DMA_EXT      },

    /* Write FUA EXT                      Read FUA EXT */
    { 0,                                  0                             },
    { IDE_COMMAND_WRITE_MULTIPLE_FUA_EXT, 0                             },
    { IDE_COMMAND_WRITE_DMA_FUA_EXT,      0                             },

    /* Write                              Read */
    { IDE_COMMAND_WRITE,                  IDE_COMMAND_READ,             },
    { IDE_COMMAND_WRITE_MULTIPLE,         IDE_COMMAND_READ_MULTIPLE,    },
    { IDE_COMMAND_WRITE_DMA,              IDE_COMMAND_READ_DMA,         },

    /* Write FUA                          Read FUA */
    { 0,                                  0                             },
    { 0,                                  0                             },
    { 0,                                  0                             },
};

/* FUNCTIONS ******************************************************************/

static
ULONG
AtaReqCopySatlBuffer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
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
    {
        // TODO: try reserved
        Request->SrbStatus = SRB_STATUS_INSUFFICIENT_RESOURCES;
        return SRB_STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Srb->DataTransferLength > Length)
    {
        /* Underrun */
        BytesCount = Length;
        Request->SrbStatus = SRB_STATUS_DATA_OVERRUN;
    }
    else
    {
        /* Success or overrun */
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
ULONG
AtaReqTerminateCommand(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ UCHAR AdditionalSenseCode)
{
    PSENSE_DATA SenseData;
    ULONG SrbStatus;

    SrbStatus = SRB_STATUS_INVALID_REQUEST;
    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

    if (Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength >= sizeof(*SenseData))
    {
        SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

        SenseData = Srb->SenseInfoBuffer;

        RtlZeroMemory(SenseData, sizeof(*SenseData));
        SenseData->ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
        SenseData->SenseKey = SCSI_SENSE_ILLEGAL_REQUEST;
        SenseData->AdditionalSenseCode = AdditionalSenseCode;
    }

    return SrbStatus;
}

static
ULONG
AtaReqTerminateInvalidField(
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    return AtaReqTerminateCommand(Srb, SCSI_ADSENSE_INVALID_CDB);
}

static
ULONG
AtaReqTerminateInvalidOpCode(
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    return AtaReqTerminateCommand(Srb, SCSI_ADSENSE_ILLEGAL_COMMAND);
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteNoOp(
    _In_ PATA_DEVICE_REQUEST Request)
{
    UNREFERENCED_PARAMETER(Request);

    return COMPLETE_IRP;
}

static
ULONG
AtaReqBuildIdentifyCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    Request->DataBuffer = &DevExt->IdentifyDeviceData;
    Request->DataTransferLength = sizeof(DevExt->IdentifyDeviceData);
    Request->Mdl = IoAllocateMdl(Request->DataBuffer,
                                 Request->DataTransferLength,
                                 FALSE,
                                 FALSE,
                                 NULL);
    if (!Request->Mdl)
        return FALSE;

    MmBuildMdlForNonPagedPool(Request->Mdl);

    Request->Flags = REQUEST_FLAG_DATA_IN |
                     REQUEST_FLAG_SYNC_MODE |
                     REQUEST_FLAG_HAS_MDL |
                     REQUEST_EXCLUSIVE;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_IDENTIFY;

    return TRUE;
}

static
VOID
AtaReqBuildLbaTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG64 Lba,
    _In_ ULONG SectorCount)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    UCHAR DriveSelect;

    /* 28-bit or 48-bit command */
    if (DevExt->Flags & DEVICE_LBA_MODE)
    {
        Request->Flags |= REQUEST_FLAG_SET_DEVICE_REGISTER;
        DriveSelect = IDE_LBA_MODE;

        TaskFile->Feature = 0;
        TaskFile->SectorCount = (UCHAR)SectorCount;
        TaskFile->LowLba = (UCHAR)Lba;                // LBA bits 0-7
        TaskFile->MidLba = (UCHAR)(Lba >> 8);         // LBA bits 8-15
        TaskFile->HighLba = (UCHAR)(Lba >> 16);       // LBA bits 16-23

        /* 48-bit command */
        if ((DevExt->Flags & DEVICE_LBA48) &&
            ((Request->Flags & REQUEST_FLAG_FUA) || AtaCommandUseLba48(Lba, SectorCount)))
        {
            TaskFile->FeatureEx = 0;
            TaskFile->SectorCountEx = (UCHAR)(SectorCount >> 8);

            TaskFile->LowLbaEx = (UCHAR)(Lba >> 24);  // LBA bits 24-31
            TaskFile->MidLbaEx = (UCHAR)(Lba >> 32);  // LBA bits 32-39
            TaskFile->HighLbaEx = (UCHAR)(Lba >> 40); // LBA bits 40-47

            Request->Flags |= REQUEST_FLAG_LBA48;
        }
        else
        {
            /* 28-bit command */
            DriveSelect |= ((Lba >> 24) & 0x0F);      // LBA bits 24-27
        }
    }
    else
    {
        ULONG ChsTemp, Cylinder, Head, Sector;

        ChsTemp = (ULONG)Lba / DevExt->SectorsPerTrack;

        /* Legacy CHS translation */
        Cylinder = ChsTemp / DevExt->Heads;
        Head = ChsTemp % DevExt->Heads;
        Sector = ((ULONG)Lba % DevExt->SectorsPerTrack) + 1;

        TaskFile->Feature = 0;
        TaskFile->SectorCount = (UCHAR)SectorCount;
        TaskFile->LowLba = (UCHAR)Cylinder;
        TaskFile->MidLba = (UCHAR)Sector;
        TaskFile->HighLba = (UCHAR)(Sector >> 8);
        DriveSelect = (UCHAR)Head;
    }

    TaskFile->DriveSelect = DriveSelect | DevExt->DeviceSelect;
}

static
VOID
AtaReqBuildNcqTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ULONG64 Lba,
    _In_ ULONG SectorCount)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    ASSERT(!(DevExt->Flags & DEVICE_PIO_ONLY));

    Request->Flags |= REQUEST_FLAG_NCQ |
                      REQUEST_FLAG_DMA_ENABLED |
                      REQUEST_FLAG_LBA48 |
                      REQUEST_FLAG_SET_DEVICE_REGISTER;

    TaskFile->Feature = (UCHAR)SectorCount;
    TaskFile->FeatureEx = (UCHAR)(SectorCount >> 8);

    TaskFile->SectorCount = 0; // TODO RARC
    TaskFile->SectorCountEx = 0; // TODO PRIO
    // TODO ICC
    // TODO AUX

    TaskFile->LowLba = (UCHAR)Lba;              // LBA bits 0-7
    TaskFile->MidLba = (UCHAR)(Lba >> 8);       // LBA bits 8-15
    TaskFile->HighLba = (UCHAR)(Lba >> 16);     // LBA bits 16-23
    TaskFile->LowLbaEx = (UCHAR)(Lba >> 24);    // LBA bits 24-31
    TaskFile->MidLbaEx = (UCHAR)(Lba >> 32);    // LBA bits 32-39
    TaskFile->HighLbaEx = (UCHAR)(Lba >> 40);   // LBA bits 40-47

    TaskFile->DriveSelect = IDE_LBA_MODE;
    if (Request->Flags & REQUEST_FLAG_FUA)
        TaskFile->DriveSelect |= IDE_DEVICE_FUA_NCQ;

    if (Request->Flags & REQUEST_FLAG_DATA_OUT)
        TaskFile->Command = IDE_COMMAND_WRITE_FPDMA_QUEUED;
    else
        TaskFile->Command = IDE_COMMAND_READ_FPDMA_QUEUED;
}

static
ULONG
AtaReqReadWrite(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PCDB Cdb;
    ULONG64 Lba;
    ULONG SectorCount;

    Request->Complete = AtaReqCompleteNoOp;
    Request->DataBuffer = Srb->DataBuffer;
    Request->DataTransferLength = Srb->DataTransferLength;
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

    /* Ignore the transfer length field in the CDB, use the DataTransferLength field instead */
    SectorCount = Request->DataTransferLength + (DevExt->SectorSize - 1);
    SectorCount /= DevExt->SectorSize;

    /* Sector count must not exceed the maximum transfer length */
    ASSERT(SectorCount <= ATA_MAX_SECTORS_PER_IO);

    if (DevExt->Flags & DEVICE_NCQ)
    {
        AtaReqBuildNcqTaskFile(DevExt, Request, Lba, SectorCount);
    }
    else
    {
        if (!(DevExt->Flags & DEVICE_PIO_ONLY))
        {
            Request->Flags |= REQUEST_FLAG_DMA_ENABLED;
        }
        else if (DevExt->MultiSectorTransfer != 0)
        {
            Request->Flags |= REQUEST_FLAG_READ_WRITE_MULTIPLE;
        }

        AtaReqBuildLbaTaskFile(DevExt, Request, Lba, SectorCount);

        /* Choose the command opcode */
        Request->TaskFile.Command = AtaReadWriteCommand(Request, DevExt);
        if (Request->TaskFile.Command == 0)
            return AtaReqTerminateInvalidField(Srb);
    }

    return SRB_STATUS_PENDING;
}

static
ULONG
AtaReqSynchronizeCache(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    /* Prepare a non-data command */
    Request->Complete = AtaReqCompleteNoOp;
    Request->DataBuffer = NULL;
    Request->DataTransferLength = 0;
    Request->Flags = 0;

    /* NOTE: This command may take longer than 30 seconds to complete */
    if (DevExt->Flags & DEVICE_LBA48)
        Request->TaskFile.Command = IDE_COMMAND_FLUSH_CACHE_EXT;
    else
        Request->TaskFile.Command = IDE_COMMAND_FLUSH_CACHE;

    return SRB_STATUS_PENDING;
}

static
ULONG
AtaReqVerify(
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
    Request->Complete = AtaReqCompleteNoOp;
    Request->DataTransferLength = 0;
    Request->Flags = 0;

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

    /* Sector count must not exceed the maximum transfer length */
    ASSERT(VerificationLength <= ATA_MAX_SECTORS_PER_IO);

    AtaReqBuildLbaTaskFile(DevExt, Request, Lba, VerificationLength);

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
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;
    PSCSI_REQUEST_BLOCK Srb = Request->Srb;
    ULONG Length;

    AtaPdoSetAddressingMode(DevExt);

    if (Srb->Cdb[0] == SCSIOP_READ_CAPACITY)
    {
        PREAD_CAPACITY_DATA CapacityData;
        ULONG MaximumLba;

        MaximumLba = min(DevExt->TotalSectors - 1, MAXULONG);

        CapacityData = (PREAD_CAPACITY_DATA)DevExt->SatlScratchBuffer;
        CapacityData->LogicalBlockAddress = RtlUlongByteSwap(MaximumLba);
        CapacityData->BytesPerBlock = RtlUlongByteSwap(DevExt->SectorSize);

        Length = sizeof(*CapacityData);
    }
    else // SCSIOP_READ_CAPACITY16
    {
        PREAD_CAPACITY16_DATA CapacityData;
        ULONG LogicalPerPhysicalExponent, LogicalSectorsPerPhysicalSector;
        ULONG LogicalSectorAlignment, LowestAlignedBlock;
        ULONG64 MaximumLba;

        LogicalSectorAlignment = AtaLogicalSectorAlignment(&DevExt->IdentifyDeviceData);
        if (LogicalSectorAlignment != 0)
        {
            LogicalSectorsPerPhysicalSector =
                AtaLogicalSectorsPerPhysicalSector(&DevExt->IdentifyDeviceData,
                                                   &LogicalPerPhysicalExponent);


            LowestAlignedBlock = (LogicalSectorsPerPhysicalSector - LogicalSectorAlignment);
            LowestAlignedBlock %= LogicalSectorsPerPhysicalSector;
        }
        else
        {
            LowestAlignedBlock = 0;
        }

        MaximumLba = DevExt->TotalSectors - 1;

        CapacityData = (PREAD_CAPACITY16_DATA)DevExt->SatlScratchBuffer;

        RtlZeroMemory(CapacityData, sizeof(*CapacityData));
        CapacityData->LogicalBlockAddress.QuadPart = RtlUlonglongByteSwap(MaximumLba);
        CapacityData->BytesPerBlock = RtlUlongByteSwap(DevExt->SectorSize);
        CapacityData->LogicalPerPhysicalExponent = LogicalPerPhysicalExponent;
        CapacityData->LowestAlignedBlock_MSB = (UCHAR)(LowestAlignedBlock >> 8);
        CapacityData->LowestAlignedBlock_LSB = (UCHAR)LowestAlignedBlock;

        if (AtaHasTrimFunction(&DevExt->IdentifyDeviceData))
        {
            if (AtaHasDratFunction(&DevExt->IdentifyDeviceData))
            {
                CapacityData->LBPME = 1;

                if (AtaHasRzatFunction(&DevExt->IdentifyDeviceData))
                    CapacityData->LBPRZ = 1;
            }
        }

        Length = CdbGetAllocationLength16((PCDB)Srb->Cdb);
    }

    AtaReqCopySatlBuffer(DevExt, Request, DevExt->SatlScratchBuffer, Length);
    return COMPLETE_IRP;
}

static
ULONG
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
    /* PageData->D_SENSE = 1; // TODO */

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

    if (AtaDeviceIsVolatileWriteCacheEnabled(&DevExt->IdentifyDeviceData))
        PageData->WriteCacheEnable = 1;

    if (AtaDeviceIsReadLookAHeadEnabled(&DevExt->IdentifyDeviceData))
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

    DevExt = Request->DevExt;

    /* Determine the media status */
    if ((Request->TranslationState == 0) && (DevExt->Flags & DEVICE_HAS_MEDIA_STATUS))
    {
        Request->Flags = 0;
        Request->TaskFile.Command = IDE_COMMAND_GET_MEDIA_STATUS;

        ++Request->TranslationState;
        return COMPLETE_START_AGAIN;
    }

    RtlZeroMemory(DevExt->SatlScratchBuffer, sizeof(DevExt->SatlScratchBuffer));

    Cdb = (PCDB)Request->Srb->Cdb;
    Is6ByteCommand = (Cdb->AsByte[0] == SCSIOP_MODE_SENSE);

    Buffer = DevExt->SatlScratchBuffer;

    /* Media status outputs */
    if (Request->TranslationState != 0)
    {
        IsWriteProtected = (Request->Status & IDE_STATUS_ERROR) &&
                           !(Request->Error & IDE_ERROR_COMMAND_ABORTED) &&
                           (Request->Error & IDE_ERROR_WRITE_PROTECT);
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
        if (DevExt->Flags & DEVICE_HAS_FUA)
            ModeHeader.Header6->DeviceSpecificParameter |= MODE_DSP_FUA_SUPPORTED;

        Buffer += sizeof(*ModeHeader.Header6);
    }
    else
    {
        if (IsWriteProtected)
            ModeHeader.Header10->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
        if (DevExt->Flags & DEVICE_HAS_FUA)
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
        FormatDescriptor->BlockLength[0] = (UCHAR)(DevExt->SectorSize >> 16);
        FormatDescriptor->BlockLength[1] = (UCHAR)(DevExt->SectorSize >> 8);
        FormatDescriptor->BlockLength[2] = (UCHAR)DevExt->SectorSize;

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
        //Buffer += AtaReqPowerConditionModePage(ChanExt, DevExt, Buffer); todo
    }

    ModeDataLength = Buffer - DevExt->SatlScratchBuffer;

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

    AtaReqCopySatlBuffer(DevExt,
                         Request,
                         DevExt->SatlScratchBuffer,
                         Buffer - DevExt->SatlScratchBuffer);
    return COMPLETE_IRP;
}

static
ULONG
AtaReqModeSense(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN Is6ByteCommand)
{
    ULONG AllocationLength;
    PCDB Cdb;

    Cdb = (PCDB)Srb->Cdb;

    if (Is6ByteCommand)
        AllocationLength = CdbGetAllocationLength6(Cdb);
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
            if (!AtaDeviceHasSmartFeature(&DevExt->IdentifyDeviceData))
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

    Request->TranslationState = 0;
    Request->Complete = AtaReqCompleteModeSense;

    return SRB_STATUS_PENDING;
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
    InquiryData->RemovableMedia = AtaDeviceIsRemovable(IdentifyData);
    InquiryData->Versions = 0x07; // SPC-5
    InquiryData->ResponseDataFormat = 2; // This means "Complies to this standard"

    if (DevExt->Flags & DEVICE_NCQ)
        InquiryData->CommandQueue = 1;

    /* T10 vendor ID */
    RtlCopyMemory(InquiryData->VendorId, "ATA     ", RTL_FIELD_SIZE(INQUIRYDATA, VendorId));

    /* Product ID */
    AtaCopyAtaString(InquiryData->ProductId,
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
    AtaCopyAtaString(InquiryData->ProductRevisionLevel,
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

    if (AtaDeviceIsZonedDevice(IdentifyData))
    {
        /* ZBC BSR INCITS 536 revision 05 */
        *VersionDescriptor++ = 0x06;
        *VersionDescriptor++ = 0x24;
    }

    if (AtaDeviceHasIeee1667(IdentifyData))
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
VOID
AtaReqInquirySupportedPages(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_SUPPORTED_PAGES_PAGE SupportedPages;
    ULONG i = 0;

    SupportedPages = (PVPD_SUPPORTED_PAGES_PAGE)DevExt->SatlScratchBuffer;

    RtlZeroMemory(SupportedPages, sizeof(*SupportedPages));
    SupportedPages->SupportedPageList[i++] = VPD_SUPPORTED_PAGES;
    SupportedPages->SupportedPageList[i++] = VPD_SERIAL_NUMBER;
    SupportedPages->SupportedPageList[i++] = VPD_DEVICE_IDENTIFIERS;
    //SupportedPages->SupportedPageList[i++] = VPD_MODE_PAGE_POLICY; // TODO
    SupportedPages->SupportedPageList[i++] = VPD_ATA_INFORMATION;
    SupportedPages->SupportedPageList[i++] = VPD_BLOCK_LIMITS;
    SupportedPages->SupportedPageList[i++] = VPD_BLOCK_DEVICE_CHARACTERISTICS;
    SupportedPages->SupportedPageList[i++] = VPD_LOGICAL_BLOCK_PROVISIONING;
    //SupportedPages->SupportedPageList[i++] = VPD_ZONED_BLOCK_DEVICE_CHARACTERISTICS; // TODO
    //SupportedPages->SupportedPageList[i++] = 0x8A; // Power Condition VPD page
    SupportedPages->PageLength = i;
}

static
VOID
AtaReqInquirySerialNumber(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_SERIAL_NUMBER_PAGE SerialNumberPage;

    SerialNumberPage = (PVPD_SERIAL_NUMBER_PAGE)DevExt->SatlScratchBuffer;

    RtlZeroMemory(SerialNumberPage, FIELD_OFFSET(VPD_SERIAL_NUMBER_PAGE, SerialNumber));
    SerialNumberPage->PageLength = RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber);

    AtaCopyAtaString(SerialNumberPage->SerialNumber,
                     DevExt->IdentifyDeviceData.SerialNumber,
                     RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber));
}

static
VOID
AtaReqInquiryDeviceIdentifiers(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_IDENTIFICATION_PAGE DeviceIdentificationPage;
    PVPD_IDENTIFICATION_DESCRIPTOR Descriptor;
    UCHAR PageLength;

    PageLength = FIELD_OFFSET(VPD_IDENTIFICATION_PAGE, Descriptors) +
                 FIELD_OFFSET(VPD_IDENTIFICATION_DESCRIPTOR, Identifier);

    if (AtaDeviceHasWorldWideName(&DevExt->IdentifyDeviceData))
    {
        PageLength += RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, WorldWideName);
    }
    else
    {
        PageLength += RTL_FIELD_SIZE(INQUIRYDATA, VendorId) +
                      RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber) +
                      RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber);
    }

    DeviceIdentificationPage = (PVPD_IDENTIFICATION_PAGE)DevExt->SatlScratchBuffer;

    RtlZeroMemory(DeviceIdentificationPage, PageLength);
    DeviceIdentificationPage->PageLength =
        PageLength - RTL_SIZEOF_THROUGH_FIELD(VPD_IDENTIFICATION_PAGE, PageLength);

    Descriptor = (PVPD_IDENTIFICATION_DESCRIPTOR)&DeviceIdentificationPage->Descriptors[0];

    if (AtaDeviceHasWorldWideName(&DevExt->IdentifyDeviceData))
    {
        /* NAA descriptor */
        Descriptor->CodeSet = VpdCodeSetBinary;
        Descriptor->IdentifierType = VpdIdentifierTypeFCPHName;
        Descriptor->IdentifierLength = RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, WorldWideName);

        AtaCopyAtaString(Descriptor->Identifier,
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
        AtaCopyAtaString(Identifier,
                         DevExt->IdentifyDeviceData.ModelNumber,
                         RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber));
        Identifier += RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber);

        /* Serial number field */
        AtaCopyAtaString(Identifier,
                         DevExt->IdentifyDeviceData.SerialNumber,
                         RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber));
    }
}

static
VOID
AtaReqInquiryBlockLimits(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_BLOCK_LIMITS_PAGE BlockLimitsPage;
    USHORT PageLength;

    PageLength = sizeof(*BlockLimitsPage) -
                 RTL_SIZEOF_THROUGH_FIELD(VPD_BLOCK_LIMITS_PAGE, PageLength);

    BlockLimitsPage = (PVPD_BLOCK_LIMITS_PAGE)DevExt->SatlScratchBuffer;

    RtlZeroMemory(BlockLimitsPage, sizeof(*BlockLimitsPage));
    BlockLimitsPage->PageLength[0] = (UCHAR)(PageLength >> 8);
    BlockLimitsPage->PageLength[1] = (UCHAR)PageLength;

/* FIXME
    if (AtaHasTrimFunction(&DevExt->IdentifyDeviceData))
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
AtaReqInquiryBlockDeviceCharacteristics(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE Characteristics;
    USHORT MediumRotationRate;

    Characteristics = (PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE)DevExt->SatlScratchBuffer;

    RtlZeroMemory(Characteristics, sizeof(*Characteristics));
    Characteristics->PageLength =
        sizeof(*Characteristics) -
        RTL_SIZEOF_THROUGH_FIELD(VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE, PageLength);

    /* Word 217 */
    MediumRotationRate = DevExt->IdentifyDeviceData.NominalMediaRotationRate;
    Characteristics->MediumRotationRateMsb = (UCHAR)(MediumRotationRate >> 8);
    Characteristics->MediumRotationRateLsb = (UCHAR)MediumRotationRate;

    /* Bits 0:3 of word 168 */
    Characteristics->NominalFormFactor =
        DevExt->IdentifyDeviceData.NominalFormFactor;

    /* Bits 0:1 of word 69 */
    Characteristics->ZONED =
        DevExt->IdentifyDeviceData.AdditionalSupported.ZonedCapabilities;
}

static
VOID
AtaReqInquiryLogicalBlockProvisioning(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE LogicalBlockProvisioningPage;

    LogicalBlockProvisioningPage =
        (PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE)DevExt->SatlScratchBuffer;

    RtlZeroMemory(LogicalBlockProvisioningPage,
                  FIELD_OFFSET(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, ProvisioningGroupDescr));
    LogicalBlockProvisioningPage->PageLength[1] =
        FIELD_OFFSET(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, ProvisioningGroupDescr) -
        RTL_SIZEOF_THROUGH_FIELD(VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, PageLength);

    if (AtaHasTrimFunction(&DevExt->IdentifyDeviceData))
    {
/* TODO
        LogicalBlockProvisioningPage->LBPU = 1;
        LogicalBlockProvisioningPage->LBPWS = 1;
        LogicalBlockProvisioningPage->LBPWS10 = 1;
*/
        if (AtaHasDratFunction(&DevExt->IdentifyDeviceData))
            LogicalBlockProvisioningPage->ANC_SUP = 1;

        if (AtaHasRzatFunction(&DevExt->IdentifyDeviceData))
            LogicalBlockProvisioningPage->LBPRZ = 1;
    }
}

static
ULONG
AtaReqInquiry(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PVPD_SUPPORTED_PAGES_PAGE Buffer;
    PCDB Cdb = (PCDB)Srb->Cdb;
    ULONG Length;

    Length = (Cdb->AsByte[3] << 8) | Cdb->AsByte[4];

    if (Srb->DataTransferLength < Length)
        return AtaReqTerminateInvalidField(Srb);

    /* Return the standard INQUIRY data */
    if (!Cdb->CDB6INQUIRY3.EnableVitalProductData)
    {
        if (Cdb->CDB6INQUIRY3.PageCode != 0)
            return AtaReqTerminateInvalidField(Srb);

        return AtaReqCopySatlBuffer(DevExt,
                                    Request,
                                    &DevExt->InquiryData,
                                    sizeof(DevExt->InquiryData));
    }

    switch (Cdb->CDB6INQUIRY3.PageCode)
    {
        case VPD_SUPPORTED_PAGES:
        {
            AtaReqInquirySupportedPages(DevExt);
            break;
        }

        case VPD_SERIAL_NUMBER:
        {
            AtaReqInquirySerialNumber(DevExt);
            break;
        }

        case VPD_DEVICE_IDENTIFIERS:
        {
            AtaReqInquiryDeviceIdentifiers(DevExt);
            break;
        }

        case VPD_BLOCK_LIMITS:
        {
            AtaReqInquiryBlockLimits(DevExt);
            break;
        }

        case VPD_BLOCK_DEVICE_CHARACTERISTICS:
        {
            AtaReqInquiryBlockDeviceCharacteristics(DevExt);
            break;
        }

        case VPD_LOGICAL_BLOCK_PROVISIONING:
        {
            AtaReqInquiryLogicalBlockProvisioning(DevExt);
            break;
        }

        default:
            return AtaReqTerminateInvalidField(Srb);
    }

    /* Data bytes common to all VPD pages */
    Buffer = (PVPD_SUPPORTED_PAGES_PAGE)DevExt->SatlScratchBuffer;
    Buffer->DeviceType = DevExt->InquiryData.DeviceType;
    Buffer->DeviceTypeQualifier = DevExt->InquiryData.DeviceTypeQualifier;
    Buffer->PageCode = Cdb->CDB6INQUIRY3.PageCode;

    return AtaReqCopySatlBuffer(DevExt, Request, DevExt->SatlScratchBuffer, Length);
}

static
ULONG
AtaReqTestUnitReady(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    // FIXME
    return SRB_STATUS_SUCCESS;
}

static
ULONG
AtaReqExecuteScsiAta(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    switch (Srb->Cdb[0])
    {
        case SCSIOP_INQUIRY:
            return AtaReqInquiry(DevExt, Request, Srb);

        case SCSIOP_TEST_UNIT_READY:
            return AtaReqTestUnitReady(DevExt, Request, Srb);

        case SCSIOP_MODE_SENSE:
        case SCSIOP_MODE_SENSE10:
            return AtaReqModeSense(DevExt,
                                   Request,
                                   Srb,
                                   (Srb->Cdb[0] == SCSIOP_MODE_SENSE));

        case SCSIOP_SERVICE_ACTION_IN16:
        {
            if (((PCDB)Srb->Cdb)->READ_CAPACITY16.ServiceAction != SERVICE_ACTION_READ_CAPACITY16)
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
            return AtaReqReadWrite(DevExt, Request, Srb);

        case SCSIOP_SYNCHRONIZE_CACHE:
        case SCSIOP_SYNCHRONIZE_CACHE16:
            return AtaReqSynchronizeCache(DevExt, Request, Srb);

        case SCSIOP_VERIFY:
        case SCSIOP_VERIFY12:
        case SCSIOP_VERIFY16:
            return AtaReqVerify(DevExt, Request, Srb);

        // TODO: more commands

        default:
            break;
    }

    WARN("Unknown command %02x\n", Srb->Cdb[0]);

    return AtaReqTerminateInvalidOpCode(Srb);
}

static
ULONG
AtaReqPreparePacketCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    /* Prepare a packet command */
    Request->Complete = AtaReqCompleteNoOp;
    Request->DataBuffer = Srb->DataBuffer;
    Request->DataTransferLength = Srb->DataTransferLength;
    Request->Flags = REQUEST_FLAG_PACKET_COMMAND |
                     (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION);

    if ((Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) &&
        !(DevExt->Flags & DEVICE_PIO_ONLY) &&
        AtaPacketCommandUseDma(Srb->Cdb[0]))
    {
        Request->Flags |= REQUEST_FLAG_DMA_ENABLED;
    }

    RtlCopyMemory(Request->Cdb, Srb->Cdb, RTL_FIELD_SIZE(SCSI_REQUEST_BLOCK, Cdb));

    return SRB_STATUS_PENDING;
}

ULONG
AtaReqExecuteScsi(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    if (IS_ATAPI(DevExt))
        return AtaReqPreparePacketCommand(DevExt, Request, Srb);
    else
        return AtaReqExecuteScsiAta(DevExt, Request, Srb);
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteTaskFile(
    _In_ PATA_DEVICE_REQUEST Request)
{
    KeSetEvent(Request->Irp->UserEvent, IO_NO_INCREMENT, FALSE);

    return COMPLETE_NO_IRP;
}

ULONG
AtaReqPrepareTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    Request->Complete = AtaReqCompleteTaskFile;
    Request->DataBuffer = Srb->DataBuffer;
    Request->DataTransferLength = Srb->DataTransferLength;
    Request->Flags = (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) | REQUEST_EXCLUSIVE;

    if (Request->Flags & REQUEST_FLAG_DATA_IN)
    {
        Request->Mdl = IoAllocateMdl(Request->DataBuffer,
                                     Request->DataTransferLength,
                                     FALSE,
                                     FALSE,
                                     NULL);
        if (!Request->Mdl)
            return SRB_STATUS_INSUFFICIENT_RESOURCES;

        MmBuildMdlForNonPagedPool(Request->Mdl);

        Request->Flags |= REQUEST_FLAG_HAS_MDL;
    }

    if (SRB_GET_FLAGS(Srb) & SRB_FLAG_POLL)
    {
        Request->Flags |= REQUEST_FLAG_SYNC_MODE;
    }

    if (SRB_GET_FLAGS(Srb) & SRB_FLAG_PACKET_COMMAND)
    {
        Request->Flags |= REQUEST_FLAG_PACKET_COMMAND;

        RtlCopyMemory(&Request->Cdb, Srb->Cdb, Srb->CdbLength);
    }
    else
    {
        PATA_TASKFILE TaskFile = Request->Irp->UserBuffer;

        RtlCopyMemory(&Request->TaskFile, TaskFile, sizeof(*TaskFile));
    }

    return SRB_STATUS_PENDING;
}
