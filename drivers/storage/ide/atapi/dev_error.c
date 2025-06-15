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
    return (Request->Output.Error & IDE_ERROR_CRC_ERROR) &&
           (Request->Flags & REQUEST_FLAG_DMA);
}

extern
VOID
AtaReqCompleteRequest(
    _In_ PATA_DEVICE_REQUEST Request);

static
VOID
AtaDeviceCompleteFailedRequest(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.FailedRequest;
    UCHAR SrbStatus;
    SIZE_T RetryCount;

    ASSERT_REQUEST(Request);
    ASSERT(Request != PortData->Worker.InternalRequest);

    AtaFsmCompleteDeviceErrorEvent(PortData);

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
                /* PATA DMA timeout, attempt to retry in PIO mode */
                if (!IS_AHCI_EXT(PortData->ChanExt))
                {
                    SRB_SET_FLAGS(Request->Srb, SRB_FLAG_PIO_ONLY);
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

    ASSERT(PortData->Worker.PausedSlotsBitmap & (1 << Request->Slot));
    PortData->Worker.PausedSlotsBitmap &= ~(1 << Request->Slot);

    /* Fail the command with an error */
    Request->InternalState = REQUEST_STATE_FREEZE_QUEUE;
    AtaReqCompleteRequest(Request);
}

static
VOID
AtaDeviceSendRequestSense(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    PATA_DEVICE_REQUEST FailedRequest;
    PCDB Cdb;

    FailedRequest = PortData->Worker.FailedRequest;
    ASSERT_REQUEST(FailedRequest);

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

    AtaFsmIssueCommand(&PortData->Worker);
    AtaFsmSetLocalState(&PortData->Worker, DEVICE_STATE_REQUEST_SENSE);
}

static
VOID
AtaDeviceSendRequestSenseExt(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    PATA_DEVICE_REQUEST FailedRequest;

    Request->Flags = REQUEST_FLAG_SAVE_TASK_FILE | REQUEST_FLAG_LBA48;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_REQUEST_SENSE_DATA_EXT;

    FailedRequest = PortData->Worker.FailedRequest;
    ASSERT_REQUEST(FailedRequest);

    AtaFsmIssueCommand(&PortData->Worker);
    AtaFsmSetLocalState(&PortData->Worker, DEVICE_STATE_REQUEST_SENSE);
}

static
VOID
AtaDeviceHandleRequestSense(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;
    PATAPORT_DEVICE_EXTENSION DevExt = PortData->Worker.DevExt;
    PATA_DEVICE_REQUEST FailedRequest;
    PSCSI_REQUEST_BLOCK Srb;
    BOOLEAN Success;

    FailedRequest = PortData->Worker.FailedRequest;
    ASSERT_REQUEST(FailedRequest);

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

    Srb = FailedRequest->Srb;

    if (Success)
    {
        PSENSE_DATA SenseData = Srb->SenseInfoBuffer;

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

            // TODO Save the task file before REQUEST_SENSE and set the LBA field

            FailedRequest->SrbStatus = AtaReqSetFixedSenseData(Srb, SenseCode);
        }

        if (RTL_CONTAINS_FIELD(SenseData,
                               Srb->SenseInfoBufferLength,
                               AdditionalSenseCodeQualifier))
        {
            INFO("0x%02X: SK 0x%02X, ASC 0x%02X, ASCQ 0x%02X\n",
                 Srb->Cdb[0],
                 SenseData->SenseKey,
                 SenseData->AdditionalSenseCode,
                 SenseData->AdditionalSenseCodeQualifier);
        }
    }
    else
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
            FailedRequest->SrbStatus = AtaReqTranslateFixedError(FailedRequest);
        }
    }

    AtaDeviceCompleteFailedRequest(PortData);
}

static
VOID
AtaDeviceHandleNcqRecovery(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST LogPageRequest = PortData->Worker.InternalRequest;
    PATAPORT_DEVICE_EXTENSION DevExt = PortData->Worker.DevExt;
    PATA_DEVICE_REQUEST FailedRequest;
    PGP_LOG_NCQ_COMMAND_ERROR LogPage;
    ULONG i;
    UCHAR Crc;

    /* We failed at retrieving the log page */
    if (LogPageRequest->SrbStatus != SRB_STATUS_SUCCESS)
    {
        ERR("READ LOG EXT failure %02x\n", LogPageRequest->SrbStatus);
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
        WARN("Failed command %08lx not found in %08lx\n",
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
        AtaAhciSaveTaskFile(PortData, FailedRequest, FALSE);
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
        AtaDeviceSendRequestSenseExt(PortData);
        return;
    }
    else
    {
        FailedRequest->SrbStatus = AtaReqTranslateFixedError(FailedRequest);
    }

    TRACE("Complete Srb %p SK %02X ASC %02X ASCQ %02X\n",
          FailedRequest->Srb,
          LogPage->SenseKey,
          LogPage->ASC,
          LogPage->ASCQ);

    AtaDeviceCompleteFailedRequest(PortData);
    return;

Failure:
    AtaFsmCompleteDeviceErrorEvent(PortData);

    /* A port reset is required to abort all outstanding queued commands */
    AtaFsmResetPort(PortData, (ULONG)-1);
}

static
VOID
AtaDeviceSendReadNcqCommandErrorLog(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.InternalRequest;

    ASSERT(IS_AHCI_EXT(PortData->ChanExt));

    AtaReqBuildReadLogTaskFile(Request,
                               IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS,
                               0,
                               1);

    Request->Flags |= REQUEST_FLAG_HAS_LOCAL_BUFFER;
    Request->TimeOut = 3;

    AtaFsmIssueCommand(&PortData->Worker);
    AtaFsmSetLocalState(&PortData->Worker, DEVICE_STATE_NCQ_RECOVERY);
}

static
VOID
AtaDeviceLogEvent(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_ERROR_LOG_VALUE ErrorValue)
{
    ULONG ErrorCode, FinalStatus;
    PIO_ERROR_LOG_PACKET LogEntry;
    PVOID IoObject;
    UCHAR Size;

    if (IS_AHCI_EXT(DevExt))
        IoObject = DevExt->Common.Self; // AHCI port or PMP device
    else
        IoObject = DevExt->Device.ChanExt->Common.Self; // IDE controller

    Size = FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
    LogEntry = IoAllocateErrorLogEntry(IoObject, Size);
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
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG Mode, AllowedModesMask;
    PATAPORT_PORT_DATA PortData;

    /* Already in PIO mode */
    if (!_BitScanReverse(&Mode, DevExt->TransferModeSelectedBitmap) || !(Mode & ~PIO_ALL))
        return;

    /* Clear the current mode and the upper bits */
    AllowedModesMask = ((1 << Mode) - 1);

    if ((DevExt->TransferModeSupportedBitmap & UDMA_ALL) && !(AllowedModesMask & UDMA_ALL))
    {
        /*
         * It makes no sense to disable DMA for AHCI hard drives
         * because that would otherwise cause READ/WRITE commands to fail.
         */
        if (IS_AHCI(&DevExt->Device) && !IS_ATAPI(&DevExt->Device))
            return;

        /* From UDMA to PIO (skip WMDMA and SWDMA) */
        AllowedModesMask &= PIO_ALL;
    }
    else if (((DevExt->TransferModeSupportedBitmap & MWDMA_ALL) &&
              !(AllowedModesMask & MWDMA_ALL)) ||
             ((DevExt->TransferModeSupportedBitmap & SWDMA_ALL) &&
              !(AllowedModesMask & SWDMA_ALL)))
    {
        /* From WMDMA to PIO (skip SWDMA) or from SWDMA to PIO */
        AllowedModesMask &= PIO_ALL;
    }

    /* We are about to disable DMA, log the change */
    if ((DevExt->TransferModeSupportedBitmap & ~PIO_ALL) && !(AllowedModesMask & ~PIO_ALL))
    {
        WARN("Too many DMA failures, disabling DMA for '%s'\n", DevExt->FriendlyName);

        AtaDeviceLogEvent(DevExt, EVENT_CODE_DMA_DISABLE);
    }

    DevExt->TransferModeAllowedMask &= AllowedModesMask;

    PortData = DevExt->Device.PortData;

    /* Program the new port timings */
    _InterlockedOr(&PortData->Worker.EventsPending, ACTION_DEVICE_TIMING);
}

static
VOID
AtaDeviceAnalyzeDmaError(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ ATA_ERROR_LOG_VALUE ErrorValue)
{
    LARGE_INTEGER CurrentTime, TimeDifferenceMs;

    WARN("DMA error %lu\n", ErrorValue);

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
    if (IS_AHCI(&DevExt->Device))
    {
        /* Disable NCQ */
        if (DevExt->Device.DeviceFlags & DEVICE_NCQ)
        {
            DevExt->Device.DeviceFlags &= ~DEVICE_NCQ;

            ERR("NCQ disabled\n");
            AtaDeviceLogEvent(DevExt, EVENT_CODE_NCQ_DISABLE);
            return;
        }

        /* Try to reduce the interface speed */
        if (AtaAhciDowngradeInterfaceSpeed(DevExt->Device.PortData))
        {
            AtaDeviceLogEvent(DevExt, EVENT_CODE_DOWNSHIFT);
            return;
        }
    }

    /* Try to reduce the transfer speed */
    AtaDeviceDowngradeTransferSpeed(DevExt);
}

static
VOID
AtaDeviceAnalyzeDeviceError(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATA_DEVICE_REQUEST Request = PortData->Worker.FailedRequest;
    PATAPORT_DEVICE_EXTENSION DevExt;

    /* Handle failed queued commands */
    if (PortData->Worker.PausedQueuedSlotsBitmap != 0)
    {
        ATA_SCSI_ADDRESS AtaScsiAddress;

        AtaScsiAddress.AsULONG = 0;
        AtaScsiAddress.PathId = PortData->PortNumber;

        DevExt = AtaFdoFindNextDeviceByPath(PortData->ChanExt, &AtaScsiAddress, FALSE, NULL);
        ASSERT(DevExt);
        PortData->Worker.DevExt = DevExt;

        AtaDeviceSendReadNcqCommandErrorLog(PortData);
        return;
    }

    ASSERT_REQUEST(Request);

    DevExt = CONTAINING_RECORD(Request->Device, ATAPORT_DEVICE_EXTENSION, Device);
    PortData->Worker.DevExt = DevExt;

    switch (Request->SrbStatus)
    {
        /* Unexpected state */
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

                AtaDeviceAnalyzeDmaError(DevExt, Request, ErrorValue);
            }

            AtaDeviceCompleteFailedRequest(PortData);

            /* Issue a reset to recover a device */
            AtaFsmResetPort(PortData, (ULONG)-1);
            break;
        }

        /* General errors */
        case SRB_STATUS_ERROR:
        {
            if (AtaDeviceIsDmaCrcError(DevExt, Request))
            {
                AtaDeviceAnalyzeDmaError(DevExt, Request, EVENT_CODE_CRC_ERROR);
                AtaDeviceCompleteFailedRequest(PortData);

                /*
                 * Always do a port reset for DMA CRC errors.
                 * This is needed for recovery and for IDE_FEATURE_SET_TRANSFER_MODE
                 * in case the transfer mode is changed.
                 */
                AtaFsmResetPort(PortData, (ULONG)-1);
                break;
            }

            /* Send the recovery command to figure out why the current command failed */
            if (AtaDeviceRequestSenseNeeded(Request))
            {
                /* Handle failed ATAPI commands */
                AtaDeviceSendRequestSense(PortData);
                break;
            }
            else if (AtaDeviceRequestSenseNeededExt(DevExt, Request))
            {
                /* Handle failed non-queued commands */
                AtaDeviceSendRequestSenseExt(PortData);
                break;
            }
            else if (AtaDeviceFixedErrorNeeded(DevExt, Request))
            {
                Request->SrbStatus = AtaReqTranslateFixedError(Request);
            }

            /* Recovery command is not required, just complete the request with an error */
            __fallthrough;
        }

        default:
        {
            AtaDeviceCompleteFailedRequest(PortData);
            break;
        }
    }
}

VOID
AtaDeviceRecoveryRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData)
{
    switch (PortData->Worker.LocalState)
    {
        case DEVICE_STATE_NEED_RECOVERY:
            AtaDeviceAnalyzeDeviceError(PortData);
            break;
        case DEVICE_STATE_REQUEST_SENSE:
            AtaDeviceHandleRequestSense(PortData);
            break;
        case DEVICE_STATE_NCQ_RECOVERY:
            AtaDeviceHandleNcqRecovery(PortData);
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }
}
