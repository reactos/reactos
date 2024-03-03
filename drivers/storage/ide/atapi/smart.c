/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SMART Feature Set support
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
ATA_COMPLETION_STATUS
AtaReqCompleteSmartCommand(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    PSENDCMDOUTPARAMS CmdOut = Request->OldDataBuffer;

    if (Request->SrbStatus == SRB_STATUS_SUCCESS)
    {
        CmdOut->DriverStatus.bDriverError = SMART_NO_ERROR;
        CmdOut->DriverStatus.bIDEError = 0;
    }
    else
    {
        CmdOut->DriverStatus.bDriverError = SMART_IDE_ERROR;
        CmdOut->DriverStatus.bIDEError = Request->Error;
    }

    /* Return the SMART status */
    if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
    {
        if (Request->Flags & REQUEST_FLAG_HAS_TASK_FILE)
        {
            PIDEREGS IdeRegs = Request->DataBuffer;

            IdeRegs->bFeaturesReg = Request->TaskFile.Feature;
            IdeRegs->bSectorCountReg = Request->TaskFile.SectorCount;
            IdeRegs->bSectorNumberReg = Request->TaskFile.LowLba;
            IdeRegs->bCylLowReg = Request->TaskFile.MidLba;
            IdeRegs->bCylHighReg = Request->TaskFile.HighLba;
            IdeRegs->bCommandReg = Request->TaskFile.Command;
            IdeRegs->bDriveHeadReg = Request->TaskFile.DriveSelect;
        }
        else
        {
            CmdOut->DriverStatus.bDriverError = SMART_IDE_ERROR;
            CmdOut->DriverStatus.bIDEError = Request->Error;
        }
    }

    return COMPLETE_IRP;
}

ULONG
AtaReqIoControl(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    union _PARAMS
    {
        PSENDCMDINPARAMS CmdIn;
        PSENDCMDOUTPARAMS CmdOut;
    } Buffer;

    if (Srb->DataTransferLength < (sizeof(SRB_IO_CONTROL) + sizeof(*Buffer.CmdIn) - 1))
        return SRB_STATUS_INVALID_REQUEST;

    Buffer.CmdIn = (PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));

    if (Buffer.CmdIn->irDriveRegs.bCommandReg != SMART_CMD)
        return SRB_STATUS_INVALID_REQUEST;

    Request->Complete = AtaReqCompleteSmartCommand;
    Request->DeviceExtension = DeviceExtension;
    Request->Mdl = Request->Irp->MdlAddress;
    Request->OldDataBuffer = Buffer.CmdOut;

    switch (Buffer.CmdIn->irDriveRegs.bFeaturesReg)
    {
        case READ_ATTRIBUTES:
        case READ_THRESHOLDS:
        case SMART_READ_LOG:
        {
            if (Buffer.CmdIn->irDriveRegs.bFeaturesReg == SMART_READ_LOG)
            {
                Request->DataTransferLength =
                    Buffer.CmdIn->irDriveRegs.bSectorCountReg * SMART_LOG_SECTOR_SIZE;
            }
            else
            {
                Request->DataTransferLength = READ_ATTRIBUTE_BUFFER_SIZE;
            }

            Request->DataBuffer = Buffer.CmdOut->bBuffer;
            Request->Flags = REQUEST_FLAG_ASYNC_MODE | REQUEST_FLAG_DATA_IN;
            break;
        }

        case SMART_WRITE_LOG:
        {
            Request->DataTransferLength =
                Buffer.CmdIn->irDriveRegs.bSectorCountReg * SMART_LOG_SECTOR_SIZE;
            Request->DataBuffer = Buffer.CmdOut->bBuffer;
            Request->Flags = REQUEST_FLAG_ASYNC_MODE | REQUEST_FLAG_DATA_OUT;
            break;
        }

        case RETURN_SMART_STATUS:
        {
            Request->DataTransferLength = sizeof(IDEREGS);
            Request->DataBuffer = Buffer.CmdOut->bBuffer;
            Request->Flags = REQUEST_FLAG_ASYNC_MODE | REQUEST_FLAG_SAVE_TASK_FILE;
            break;
        }

        case EXECUTE_OFFLINE_DIAGS:
        {
            UCHAR Subcommand = Buffer.CmdIn->irDriveRegs.bSectorNumberReg;

            if (Subcommand == SMART_SHORT_SELFTEST_CAPTIVE ||
                Subcommand == SMART_EXTENDED_SELFTEST_CAPTIVE)
            {
                return SRB_STATUS_INVALID_REQUEST;
            }

            __fallthrough;
        }
        case ENABLE_DISABLE_AUTOSAVE:
        case SAVE_ATTRIBUTE_VALUES:
        case ENABLE_SMART:
        case DISABLE_SMART:
        case ENABLE_DISABLE_AUTO_OFFLINE:
        {
            Request->DataTransferLength = 0;
            Request->DataBuffer = NULL;
            Request->Flags = REQUEST_FLAG_ASYNC_MODE;
            break;
        }

        default:
            return SRB_STATUS_INVALID_REQUEST;
    }

    if (Request->Flags & (REQUEST_FLAG_DATA_OUT | REQUEST_FLAG_SAVE_TASK_FILE))
    {
        if (Srb->DataTransferLength <
            (sizeof(SRB_IO_CONTROL) + sizeof(*Buffer.CmdOut) - 1 + Request->DataTransferLength))
        {
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            return STATUS_BUFFER_TOO_SMALL;
        }
    }

    TaskFile->Feature = Buffer.CmdIn->irDriveRegs.bFeaturesReg;
    TaskFile->SectorCount = Buffer.CmdIn->irDriveRegs.bSectorCountReg;
    TaskFile->LowLba = Buffer.CmdIn->irDriveRegs.bSectorNumberReg;
    TaskFile->MidLba = Buffer.CmdIn->irDriveRegs.bCylLowReg;
    TaskFile->HighLba = Buffer.CmdIn->irDriveRegs.bCylHighReg;
    TaskFile->Command = Buffer.CmdIn->irDriveRegs.bCommandReg;
    TaskFile->DriveSelect = Buffer.CmdIn->irDriveRegs.bDriveHeadReg;

    return SRB_STATUS_PENDING;
}

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    union _PARAMS
    {
        PSENDCMDINPARAMS CmdIn;
        PSENDCMDOUTPARAMS CmdOut;
    } Buffer;

    PAGED_CODE();

    if (Srb->DataTransferLength <
        (sizeof(SRB_IO_CONTROL) + sizeof(*Buffer.CmdOut) - 1 + IDENTIFY_BUFFER_SIZE))
    {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STATUS_BUFFER_TOO_SMALL;
    }

    Buffer.CmdIn = (PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));

    if (Buffer.CmdIn->irDriveRegs.bCommandReg != ID_CMD)
    {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Buffer.CmdOut->cBufferSize = IDENTIFY_BUFFER_SIZE;
    Buffer.CmdOut->DriverStatus.bDriverError = 0;
    Buffer.CmdOut->DriverStatus.bIDEError = 0;

    RtlCopyMemory(Buffer.CmdOut->bBuffer,
                  &DeviceExtension->IdentifyDeviceData,
                  IDENTIFY_BUFFER_SIZE);

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportSmartVersion(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PGETVERSIONINPARAMS VersionParameters;
    PATAPORT_DEVICE_EXTENSION DeviceExtension;
    ULONG MaxDevices, DeviceNumber;
    UCHAR DeviceBitmap;

    PAGED_CODE();

    if (Srb->DataTransferLength < (sizeof(*VersionParameters) + sizeof(SRB_IO_CONTROL)))
    {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STATUS_BUFFER_TOO_SMALL;
    }

    VersionParameters = (PGETVERSIONINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));

    /* SMART 1.03 */
    VersionParameters->bVersion = 1;
    VersionParameters->bRevision = 1;
    VersionParameters->bReserved = 0;

    VersionParameters->fCapabilities = (CAP_ATA_ID_CMD | CAP_ATAPI_ID_CMD | CAP_SMART_CMD);

    DeviceBitmap = 0;
    MaxDevices = AtaFdoMaxDeviceCount(ChannelExtension);

    for (DeviceNumber = 0; DeviceNumber < MaxDevices; ++DeviceNumber)
    {
        ATA_SCSI_ADDRESS AtaScsiAddress;

        AtaScsiAddress = AtaMarshallScsiAddress(ChannelExtension->PathId, DeviceNumber, 0);

        DeviceExtension = AtaFdoFindDeviceByPath(ChannelExtension, AtaScsiAddress);
        if (DeviceExtension)
        {
            if (IS_ATAPI(DeviceExtension))
                DeviceBitmap |= (1 << DeviceNumber) << 4;
            else
                DeviceBitmap |= (1 << DeviceNumber);
        }
    }

    /* Secondary channel bits */
    if ((ChannelExtension->PathId == 1) && (!(ChannelExtension->Flags & CHANNEL_CBUS_IDE)))
        DeviceBitmap <<= 2;

    VersionParameters->bIDEDeviceMap = DeviceBitmap;

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STATUS_SUCCESS;
}
