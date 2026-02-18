/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SMART Feature Set support
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
VOID
AtaIdeRegsToTaskFile(
    _In_ ATAPORT_DEVICE_EXTENSION* __restrict DevExt,
    _In_ IDEREGS* __restrict IdeRegs,
    _Out_ ATA_DEVICE_REQUEST* __restrict Request)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    TaskFile->Feature = IdeRegs->bFeaturesReg;
    TaskFile->SectorCount = IdeRegs->bSectorCountReg;
    TaskFile->LowLba = IdeRegs->bSectorNumberReg;
    TaskFile->MidLba = IdeRegs->bCylLowReg;
    TaskFile->HighLba = IdeRegs->bCylHighReg;
    TaskFile->Command = IDE_COMMAND_SMART; // SMART_CMD

    /* Set the master/slave bit to the correct value */
    TaskFile->DriveSelect = IdeRegs->bDriveHeadReg & ~IDE_DRIVE_SELECT_SLAVE;
    TaskFile->DriveSelect |= DevExt->Device.DeviceSelect & IDE_DRIVE_SELECT_SLAVE;
}

static
VOID
AtaTaskFileToIdeRegs(
    _In_ ATA_DEVICE_REQUEST* __restrict Request,
    _Out_ IDEREGS* __restrict IdeRegs)
{
    PATA_TASKFILE TaskFile = &Request->Output;

    IdeRegs->bFeaturesReg = TaskFile->Feature;
    IdeRegs->bSectorCountReg = TaskFile->SectorCount;
    IdeRegs->bSectorNumberReg = TaskFile->LowLba;
    IdeRegs->bCylLowReg = TaskFile->MidLba;
    IdeRegs->bCylHighReg = TaskFile->HighLba;
    IdeRegs->bCommandReg = TaskFile->Command;
    IdeRegs->bDriveHeadReg = TaskFile->DriveSelect;
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteSmartIoControl(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PSENDCMDOUTPARAMS CmdOut;

    CmdOut = (PSENDCMDOUTPARAMS)(((PUCHAR)Request->Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));

    if (Request->SrbStatus == SRB_STATUS_SUCCESS)
    {
        CmdOut->DriverStatus.bDriverError = SMART_NO_ERROR;
        CmdOut->DriverStatus.bIDEError = 0;
    }
    else
    {
        CmdOut->DriverStatus.bDriverError = SMART_IDE_ERROR;
        CmdOut->DriverStatus.bIDEError = Request->Output.Error;
    }

    /* Return the SMART status */
    if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
    {
        if (Request->Flags & REQUEST_FLAG_HAS_TASK_FILE)
        {
            PIDEREGS IdeRegs = (PIDEREGS)&CmdOut->bBuffer;

            AtaTaskFileToIdeRegs(Request, IdeRegs);
        }
        else
        {
            CmdOut->DriverStatus.bDriverError = SMART_IDE_ERROR;
            CmdOut->DriverStatus.bIDEError = Request->Output.Error;
        }
    }

    return COMPLETE_IRP;
}

UCHAR
AtaReqSmartIoControl(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
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

    switch (Buffer.CmdIn->irDriveRegs.bFeaturesReg)
    {
        case READ_ATTRIBUTES:
        {
            Request->DataTransferLength = READ_ATTRIBUTE_BUFFER_SIZE;
            Request->Flags = REQUEST_FLAG_DATA_IN;
            break;
        }

        case READ_THRESHOLDS:
        {
            Request->DataTransferLength = READ_THRESHOLD_BUFFER_SIZE;
            Request->Flags = REQUEST_FLAG_DATA_IN;
            break;
        }

        case SMART_READ_LOG:
        {
            Request->DataTransferLength =
                Buffer.CmdIn->irDriveRegs.bSectorCountReg * SMART_LOG_SECTOR_SIZE;
            Request->Flags = REQUEST_FLAG_DATA_IN;
            break;
        }

        case SMART_WRITE_LOG:
        {
            Request->DataTransferLength =
                Buffer.CmdIn->irDriveRegs.bSectorCountReg * SMART_LOG_SECTOR_SIZE;
            Request->Flags = REQUEST_FLAG_DATA_OUT;
            break;
        }

        case RETURN_SMART_STATUS:
        {
            Request->DataTransferLength = sizeof(IDEREGS);
            Request->Flags = REQUEST_FLAG_SAVE_TASK_FILE;
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
            Request->Flags = 0;
            break;
        }

        default:
            return SRB_STATUS_INVALID_REQUEST;
    }

    if (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_SAVE_TASK_FILE))
    {
        if (Srb->DataTransferLength <
            (sizeof(SRB_IO_CONTROL) + sizeof(*Buffer.CmdOut) - 1 + Request->DataTransferLength))
        {
            return SRB_STATUS_INVALID_REQUEST;
        }

        Request->DataBuffer = Buffer.CmdOut->bBuffer;
    }
    else if (Request->Flags & REQUEST_FLAG_DATA_OUT)
    {
        if (Srb->DataTransferLength <
            (sizeof(SRB_IO_CONTROL) + sizeof(*Buffer.CmdIn) - 1 + Request->DataTransferLength))
        {
            return SRB_STATUS_INVALID_REQUEST;
        }

        Request->DataBuffer = Buffer.CmdIn->bBuffer;
    }

    Request->Complete = AtaReqCompleteSmartIoControl;

    AtaIdeRegsToTaskFile(DevExt, &Buffer.CmdIn->irDriveRegs, Request);

    return SRB_STATUS_PENDING;
}

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
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

    if (IS_ATAPI(&DevExt->Device) || (Buffer.CmdIn->irDriveRegs.bCommandReg != ID_CMD))
    {
        Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Buffer.CmdOut->cBufferSize = IDENTIFY_BUFFER_SIZE;
    Buffer.CmdOut->DriverStatus.bDriverError = 0;
    Buffer.CmdOut->DriverStatus.bIDEError = 0;

    RtlCopyMemory(Buffer.CmdOut->bBuffer, &DevExt->IdentifyDeviceData, IDENTIFY_BUFFER_SIZE);

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportSmartVersion(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Common.FdoExt;
    PATAPORT_PORT_DATA PortData = &ChanExt->PortData;
    PGETVERSIONINPARAMS VersionParameters;
    ULONG i;

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

    VersionParameters->bIDEDeviceMap = 0;

    /* Emulate the PATA behavior */
    for (i = 0; i < MAX_IDE_DEVICE; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        ULONG DeviceFlag;

        DevExt = AtaFdoFindDeviceByPath(ChanExt,
                                        AtaMarshallScsiAddress(PortData->PortNumber, i, 0),
                                        Srb);
        if (!DevExt)
            continue;

        DeviceFlag = 1 << i;
        if (PortData->PortNumber != 0)
            DeviceFlag <<= 2;
        if (IS_ATAPI(&DevExt->Device))
            DeviceFlag <<= 4;
        VersionParameters->bIDEDeviceMap |= DeviceFlag;

        IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Srb);
    }

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STATUS_SUCCESS;
}
