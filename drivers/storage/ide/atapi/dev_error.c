/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device I/O error handling
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
AtaDeviceRequestSenseNeeded(
    _In_ PATA_DEVICE_REQUEST Request)
{
    return ((Request->Flags & REQUEST_FLAG_PACKET_COMMAND) &&
            !(Request->Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
            (Request->Srb->SenseInfoBuffer != NULL) &&
            (Request->Srb->SenseInfoBufferLength != 0));
}

static
BOOLEAN
AtaDeviceRequestSenseNeededExt(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    /* NOTE: The ERROR bit and the SENSE DATA AVAILABLE bit may both be set to one */
    return ((DevExt->Device.DeviceFlags & DEVICE_SENSE_DATA_REPORTING) &&
            /* (Request->Status & IDE_STATUS_INDEX) && */
            !(Request->Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
            (Request->Srb->SenseInfoBuffer != NULL) &&
            (Request->Srb->SenseInfoBufferLength != 0));
}

static
BOOLEAN
AtaDeviceFixedErrorNeeded(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    return (!(Request->Flags & REQUEST_FLAG_PACKET_COMMAND) &&
            !(Request->Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
            (Request->Srb->SenseInfoBuffer != NULL) &&
            (Request->Srb->SenseInfoBufferLength != 0));
}

static
BOOLEAN
AtaDeviceIsDmaCrcError(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    // TODO: Check also for SATA SErr or AHCI interrupt status bits
    return (Request->Output.Error & IDE_ERROR_CRC_ERROR) &&
           (Request->Flags & REQUEST_FLAG_DMA);
}

static
VOID
AtaDeviceHandleRequestSense(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    PATA_DEVICE_REQUEST FailedRequest = PortData->Worker.FailedRequest;
    PSCSI_REQUEST_BLOCK Srb = FailedRequest->Srb;
    PSENSE_DATA SenseData;
    BOOLEAN Success;

    if (IS_ATAPI(&DevExt->Device))
    {
        Success = (Request->SrbStatus == SRB_STATUS_SUCCESS) ||
                  (Request->SrbStatus == SRB_STATUS_DATA_OVERRUN);
    }
    else
    {
        Success = (Request->SrbStatus == SRB_STATUS_SUCCESS) &&
                  (Request->Flags & REQUEST_FLAG_HAS_TASK_FILE) &&
                  (Request->TaskFile.HighLba != 0);
    }

    if (!Success)
    {
        ERR("Request sense failed\n");

        if (IS_ATAPI(&DevExt->Device))
        {
            Srb->ScsiStatus = SCSISTAT_GOOD;
            FailedRequest->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;
        }
        else
        {
            /* Request sense failed, translate the original ATA device error */
            FailedRequest->SrbStatus = AtaReqSetFixedAtaSenseData(FailedRequest);
        }

        return;
    }

    SenseData = Srb->SenseInfoBuffer;

    if (IS_ATAPI(&DevExt->Device))
    {
        /* Copy the sense data from the local buffer */
        RtlCopyMemory(Srb->SenseInfoBuffer,
                      DevExt->Device.LocalBuffer,
                      min(Srb->SenseInfoBufferLength, ATA_LOCAL_BUFFER_SIZE));

        Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        FailedRequest->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
    }
    else
    {
        SCSI_SENSE_CODE SenseCode;

        /* Copy the sense code from the task file registers */
        SenseCode.SrbStatus = SRB_STATUS_ERROR;
        SenseCode.SenseKey = Request->TaskFile.HighLba;
        SenseCode.AdditionalSenseCode = Request->TaskFile.MidLba;
        SenseCode.AdditionalSenseCodeQualifier = Request->TaskFile.LowLba;
        FailedRequest->SrbStatus = AtaReqSetFixedSenseData(Srb, SenseCode);
    }

    if (RTL_CONTAINS_FIELD(SenseData,
                           Srb->SenseInfoBufferLength,
                           AdditionalSenseCodeQualifier))
    {
        /* INFO("0x%02X: SK 0x%02X, ASC 0x%02X, ASCQ 0x%02X\n", */
             /* Srb->Cdb[0], */
             /* SenseData->SenseKey, */
             /* SenseData->AdditionalSenseCode, */
             /* SenseData->AdditionalSenseCodeQualifier); */
    }
}

static
NTSTATUS
AtaDeviceSendRequestSense(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    PATA_DEVICE_REQUEST FailedRequest = PortData->Worker.FailedRequest;
    NTSTATUS Status;
    PCDB Cdb;

    ASSERT_REQUEST(FailedRequest);
    ASSERT(FailedRequest->Srb->SenseInfoBufferLength > 0);

    Request->Flags = REQUEST_FLAG_DATA_IN |
                     REQUEST_FLAG_PACKET_COMMAND |
                     REQUEST_FLAG_HAS_LOCAL_BUFFER;
    Request->TimeOut = 3;
    Request->DataTransferLength = FailedRequest->Srb->SenseInfoBufferLength;

    Cdb = (PCDB)Request->Cdb;
    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    Cdb->CDB6INQUIRY.Reserved1 = 0;
    Cdb->CDB6INQUIRY.PageCode = 0;
    Cdb->CDB6INQUIRY.IReserved = 0;
    Cdb->CDB6INQUIRY.AllocationLength = (UCHAR)Request->DataTransferLength;
    Cdb->CDB6INQUIRY.Control = 0;

    Status = AtaPortSendRequest(PortData, DevExt);

    AtaDeviceHandleRequestSense(PortData, DevExt);
    return Status;
}

static
NTSTATUS
AtaDeviceSendRequestSenseExt(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;
    NTSTATUS Status;

    Request->Flags = REQUEST_FLAG_SAVE_TASK_FILE | REQUEST_FLAG_LBA48;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_REQUEST_SENSE_DATA_EXT;

    Status = AtaPortSendRequest(PortData, DevExt);

    AtaDeviceHandleRequestSense(PortData, DevExt);
    return Status;
}

static
NTSTATUS
AtaDeviceSendReadNcqCommandErrorLog(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = &PortData->Worker.InternalRequest;

    AtaReqBuildReadLogTaskFile(Request,
                               IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS,
                               0,
                               1);
    Request->Flags |= REQUEST_FLAG_HAS_LOCAL_BUFFER;
    Request->TimeOut = 3;

    return AtaPortSendRequest(PortData, DevExt);
}

static
VOID
AtaDeviceSaveLogPageTaskFile(
    _In_ GP_LOG_NCQ_COMMAND_ERROR* __restrict LogPage,
    _Inout_ ATA_DEVICE_REQUEST* __restrict Request)
{
    PATA_TASKFILE TaskFile = &Request->Output;

    TaskFile->SectorCount = LogPage->Count7_0;
    TaskFile->LowLba = LogPage->LBA7_0;
    TaskFile->MidLba = LogPage->LBA15_8;
    TaskFile->HighLba = LogPage->LBA23_16;
    TaskFile->DriveSelect = LogPage->Device;

    if (Request->Flags & REQUEST_FLAG_LBA48)
    {
        TaskFile->FeatureEx = 0; // Reserved
        TaskFile->SectorCountEx = LogPage->Count15_8;
        TaskFile->LowLbaEx = LogPage->LBA31_24;
        TaskFile->MidLbaEx = LogPage->LBA39_32;
        TaskFile->HighLbaEx = LogPage->LBA47_40;
    }
}

static
VOID
AtaDeviceLogEvent(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_ERROR_LOG_VALUE ErrorValue)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->Common.FdoExt;
    ULONG ErrorCode, FinalStatus;
    PIO_ERROR_LOG_PACKET LogEntry;
    UCHAR Size;

    Size = FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
    LogEntry = IoAllocateErrorLogEntry(ChanExt->Common.Self, Size);
    if (!LogEntry)
        return;

    switch (ErrorValue)
    {
        case EVENT_CODE_TIMEOUT:
            ErrorCode = IO_ERR_TIMEOUT;
            FinalStatus = STATUS_IO_TIMEOUT;
            break;
        case EVENT_CODE_CRC_ERROR:
            ErrorCode = IO_ERR_PARITY;
            FinalStatus = STATUS_IO_DEVICE_ERROR;
            break;

        default:
            ErrorCode = IO_ERR_CONTROLLER_ERROR;
            FinalStatus = STATUS_IO_DEVICE_ERROR;
            break;
    }

    RtlZeroMemory(LogEntry, Size);
    LogEntry->FinalStatus = FinalStatus;
    LogEntry->ErrorCode = ErrorCode;
    LogEntry->MajorFunctionCode = IRP_MJ_SCSI;
    LogEntry->UniqueErrorValue = ErrorValue;

    IoWriteErrorLogEntry(LogEntry);
}

static
VOID
AtaDeviceDowngradeTransferSpeed(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG Mode, AllowedModesMask;

    /* Already in PIO mode */
    if (!_BitScanReverse(&Mode, DevExt->TransferModeSelectedBitmap & ~PIO_ALL))
        return;

    /* Clear the current mode and the upper bits */
    AllowedModesMask = ((1 << Mode) - 1);

    if ((DevExt->TransferModeSupportedBitmap & UDMA_ALL) && !(AllowedModesMask & UDMA_ALL))
    {
        /*
         * It makes no sense to disable DMA for AHCI hard drives
         * because that would otherwise cause READ/WRITE commands to fail.
         */
        if ((PortData->PortFlags & PORT_FLAG_PIO_VIA_DMA) && !IS_ATAPI(&DevExt->Device))
            return;

        /* From UDMA to PIO (intentionally skip MWDMA and SWDMA) */
        AllowedModesMask &= PIO_ALL;
    }
    else if (((DevExt->TransferModeSupportedBitmap & MWDMA_ALL) &&
              !(AllowedModesMask & MWDMA_ALL)) ||
             ((DevExt->TransferModeSupportedBitmap & SWDMA_ALL) &&
              !(AllowedModesMask & SWDMA_ALL)))
    {
        /* From MWDMA to PIO (intentionally skip SWDMA) or from SWDMA to PIO */
        AllowedModesMask &= PIO_ALL;
    }

    /* We are about to disable DMA, log the change */
    if ((DevExt->TransferModeSupportedBitmap & ~PIO_ALL) && !(AllowedModesMask & ~PIO_ALL))
    {
        WARN("Too many DMA failures, disabling DMA for '%s'\n", DevExt->FriendlyName);

        AtaDeviceLogEvent(DevExt, EVENT_CODE_DMA_DISABLE);
    }

    WARN("Downgrading DMA speed from %lu for '%s'\n", Mode, DevExt->FriendlyName);

    DevExt->TransferModeAllowedMask &= AllowedModesMask;

    /* Program the new timings */
    _InterlockedOr(&PortData->Worker.EventsPending, ACTION_PORT_TIMING | ACTION_DEVICE_CONFIG);
    _InterlockedOr(&DevExt->Worker.EventsPending, ACTION_DEVICE_CONFIG);

    /* Request a QBR to ensure that storprop.dll updates its data from the registry */
    // FIXME: Bug in the PnP manager?
    //PortData->Worker.Flags |= WORKER_FLAG_NEED_RESCAN;
}

static
VOID
AtaDeviceAnalyzeDmaError(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ATA_ERROR_LOG_VALUE ErrorValue)
{
    LARGE_INTEGER CurrentTime, TimeDifferenceMs;

    WARN("DMA error %lu on '%s'\n", ErrorValue, DevExt->FriendlyName);

    /* We silently ignore device errors caused by PASSTHROUGH commands came from user mode */
    if (Request->Flags & REQUEST_FLAG_PASSTHROUGH)
        return;

    KeQuerySystemTime(&CurrentTime);
    TimeDifferenceMs.QuadPart = (CurrentTime.QuadPart - DevExt->LastDmaErrorTime.QuadPart) / 10000;
    DevExt->LastDmaErrorTime.QuadPart = CurrentTime.QuadPart;

    AtaDeviceLogEvent(DevExt, ErrorValue);

    /* Ignore all occasional DMA errors we encounter */
    if (TimeDifferenceMs.QuadPart >= (10LL * 60000LL)) // 10 min
        return;

    /* DMA timeouts and DMA CRC errors usually indicate a bad connection (a bad cable) */

    /* Try to disable NCQ */
    if (DevExt->Device.DeviceFlags & DEVICE_NCQ)
    {
        DevExt->Device.DeviceFlags &= ~DEVICE_NCQ;

        ERR("NCQ disabled for '%s'\n", DevExt->FriendlyName);
        AtaDeviceLogEvent(DevExt, EVENT_CODE_NCQ_DISABLE);
        return;
    }

    /* Try to reduce the interface speed */
    if (PortData->DowngradeInterfaceSpeed(PortData->ChannelContext))
    {
        WARN("Downgrading interface for '%s'\n", DevExt->FriendlyName);
        AtaDeviceLogEvent(DevExt, EVENT_CODE_DOWNSHIFT);
        return;
    }

    /* Try to reduce the transfer speed */
    AtaDeviceDowngradeTransferSpeed(PortData, DevExt);
}

static
VOID
AtaDeviceCompleteFailedRequest(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.FailedRequest;
    UCHAR SrbStatus;
    SIZE_T RetryCount;
    KIRQL OldIrql;

    ASSERT_REQUEST(Request);
    ASSERT(Request != &PortData->Worker.InternalRequest);

    SrbStatus = SRB_STATUS(Request->SrbStatus);

    RetryCount = SRB_GET_FLAGS(Request->Srb) & SRB_FLAG_RETRY_COUNT_MASK;
    RetryCount++;
    SRB_CLEAR_FLAGS(Request->Srb, SRB_FLAG_RETRY_COUNT_MASK);
    SRB_SET_FLAGS(Request->Srb, RetryCount);

    if (RetryCount > 3)
    {
        ASSERT(SrbStatus != SRB_STATUS_BUSY);
    }
    else
    {
        /*
         * We usually do not want to retry a request, because the upper class driver
         * is intended to effectively analyze errors and sense data.
         */
        switch (SrbStatus)
        {
            case SRB_STATUS_BUS_RESET:
            case SRB_STATUS_TIMEOUT:
            {
                /* DMA timeout, attempt to retry in PIO mode */
                if (!(PortData->PortFlags & PORT_FLAG_PIO_VIA_DMA))
                {
                    SRB_SET_FLAGS(Request->Srb, SRB_FLAG_PIO_RETRY);
                }

                SrbStatus = SRB_STATUS_BUSY;
                break;
            }

            case SRB_STATUS_REQUEST_SENSE_FAILED:
            {
                SrbStatus = SRB_STATUS_BUSY;
                break;
            }

            default:
            {
                /* Retry paging I/O operations only */
                if (Request->Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_PAGING_IO))
                    SrbStatus = SRB_STATUS_BUSY;
                break;
            }
        }
    }

    SrbStatus |= Request->SrbStatus & (SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_QUEUE_FROZEN);
    Request->SrbStatus = SrbStatus;

    if (SRB_STATUS(SrbStatus) == SRB_STATUS_BUSY)
        return;

    ASSERT(SRB_STATUS(SrbStatus) != SRB_STATUS_SUCCESS);

    /* Fail the command with an error */
    Request->InternalState = REQUEST_STATE_FREEZE_QUEUE;

    OldIrql = KeAcquireInterruptSpinLock(PortData->InterruptObject);

    ASSERT(PortData->Worker.PausedSlotsBitmap & (1 << Request->Slot));
    PortData->Worker.PausedSlotsBitmap &= ~(1 << Request->Slot);

    KeReleaseInterruptSpinLock(PortData->InterruptObject, OldIrql);
}

static
NTSTATUS
AtaDeviceNcqRecovery(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST FailedRequest;
    PGP_LOG_NCQ_COMMAND_ERROR LogPage;
    NTSTATUS Status;
    ULONG i;
    UCHAR Crc;

    Status = AtaDeviceSendReadNcqCommandErrorLog(PortData, DevExt);
    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;
    /* We failed at retrieving the log page */
    if (!NT_SUCCESS(Status))
    {
        ERR("READ LOG EXT failure\n");
        goto Failure;
    }

    LogPage = DevExt->Device.LocalBuffer;

    /* It's not ours */
    if (LogPage->NonQueuedCmd)
    {
        ERR("Unexpected NQ bit in the log page 0x10 structure\n");
        goto Failure;
    }

    /* Verify the checksum */
    Crc = 0;
    for (i = 0; i < IDE_GP_LOG_SECTOR_SIZE; ++i)
    {
        Crc += ((PUCHAR)LogPage)[i];
    }
    if (Crc != 0)
    {
        ERR("CRC error in the log page 0x10 structure\n");
        goto Failure;
    }

    /* Find the failed queued command */
    if (!(PortData->Worker.PausedSlotsBitmap & (1 << LogPage->NcqTag)))
    {
        ERR("Failed command %08lx not found in %08lx\n",
             1 << LogPage->NcqTag, PortData->Worker.PausedSlotsBitmap);
        goto Failure;
    }

    FailedRequest = PortData->Slots[LogPage->NcqTag];
    ASSERT_REQUEST(FailedRequest);
    PortData->Worker.FailedRequest = FailedRequest;

    /* Fail the command with an error */
    FailedRequest->SrbStatus = SRB_STATUS_ERROR;
    FailedRequest->Output.Status = LogPage->Status;
    FailedRequest->Output.Error = LogPage->Error;
    if (FailedRequest->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
    {
        AtaDeviceSaveLogPageTaskFile(LogPage, FailedRequest);
    }

    if ((LogPage->SenseKey != 0) && AtaDevHasNcqAutosense(&DevExt->IdentifyDeviceData))
    {
        SCSI_SENSE_CODE SenseCode;

        /* Set the sense data returned by the device */
        SenseCode.SrbStatus = FailedRequest->SrbStatus;
        SenseCode.SenseKey = LogPage->SenseKey;
        SenseCode.AdditionalSenseCode = LogPage->ASC;
        SenseCode.AdditionalSenseCodeQualifier = LogPage->ASCQ;
        FailedRequest->SrbStatus = AtaReqSetFixedSenseData(FailedRequest->Srb, SenseCode);

        /* Set the "Final LBA in Error" field */
        AtaReqSetLbaInformation(FailedRequest->Srb,
                                ((ULONG64)(((PUCHAR)LogPage)[17]) << 0) |
                                ((ULONG64)(((PUCHAR)LogPage)[18]) << 8) |
                                ((ULONG64)(((PUCHAR)LogPage)[19]) << 16) |
                                ((ULONG64)(((PUCHAR)LogPage)[20]) << 24) |
                                ((ULONG64)(((PUCHAR)LogPage)[21]) << 32) |
                                ((ULONG64)(((PUCHAR)LogPage)[22]) << 48));
    }
    else if (AtaDeviceRequestSenseNeededExt(DevExt, FailedRequest))
    {
        /* Get sense data */
        Status = AtaDeviceSendRequestSenseExt(PortData, DevExt);
        if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
            return Status;
    }
    else
    {
        FailedRequest->SrbStatus = AtaReqSetFixedAtaSenseData(FailedRequest);
    }

    /* TRACE("CH %lu: Complete Srb %p SK %02X ASC %02X ASCQ %02X\n", */
          /* PortData->PortNumber, */
          /* FailedRequest->Srb, */
          /* LogPage->SenseKey, */
          /* LogPage->ASC, */
          /* LogPage->ASCQ); */

    AtaDeviceCompleteFailedRequest(PortData);
    return STATUS_SUCCESS;

Failure:
    /* A port reset is required to abort all outstanding queued commands */
    _InterlockedOr(&PortData->Worker.EventsPending, ACTION_PORT_RESET);
    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaDeviceGenericRecovery(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.FailedRequest;
    NTSTATUS Status;

    switch (Request->SrbStatus)
    {
        /* Unexpected channel/device state */
        case SRB_STATUS_BUS_RESET:
        case SRB_STATUS_TIMEOUT:
        {
            if (Request->Flags & REQUEST_FLAG_DMA)
            {
                ATA_ERROR_LOG_VALUE ErrorValue;

                if (Request->SrbStatus == SRB_STATUS_TIMEOUT)
                    ErrorValue = EVENT_CODE_TIMEOUT;
                else
                    ErrorValue = EVENT_CODE_BAD_STATE;
                AtaDeviceAnalyzeDmaError(PortData, DevExt, Request, ErrorValue);
            }
            break;
        }

        /* General errors */
        case SRB_STATUS_ERROR:
        {
            if (AtaDeviceIsDmaCrcError(DevExt, Request))
            {
                AtaDeviceAnalyzeDmaError(PortData, DevExt, Request, EVENT_CODE_CRC_ERROR);
                break;
            }

            /* Send the recovery command to figure out why the current command failed */
            if (AtaDeviceRequestSenseNeeded(Request))
            {
                /* Handle failed ATAPI commands */
                Status = AtaDeviceSendRequestSense(PortData, DevExt);
                break;
            }
            else if (AtaDeviceRequestSenseNeededExt(DevExt, Request))
            {
                /* Handle failed non-queued commands */
                Status = AtaDeviceSendRequestSenseExt(PortData, DevExt);
                break;
            }
            else if (AtaDeviceFixedErrorNeeded(DevExt, Request))
            {
                Request->SrbStatus = AtaReqSetFixedAtaSenseData(Request);
            }

            /* Recovery command is not required, just complete the request with an error */
            __fallthrough;
        }

        default:
            break;
    }

    if (Status == STATUS_ADAPTER_HARDWARE_ERROR)
        return Status;

    AtaDeviceCompleteFailedRequest(PortData);
    return STATUS_SUCCESS;
}

NTSTATUS
AtaPortDeviceProcessError(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST FailedRequest = PortData->Worker.FailedRequest;
    NTSTATUS Status;

    ASSERT_REQUEST(FailedRequest);

    if (FailedRequest->Flags & REQUEST_FLAG_NCQ)
        Status = AtaDeviceNcqRecovery(PortData, DevExt);
    else
        Status = AtaDeviceGenericRecovery(PortData, DevExt);

    return Status;
}
