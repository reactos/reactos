/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device event queue core logic
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/



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
    return ((DevExt->Flags & DEVICE_SENSE_DATA_REPORTING) &&
            (Request->Status & IDE_STATUS_INDEX) &&
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
    // TODO check SATA interrupt status bits
    return (Request->Error & IDE_ERROR_CRC_ERROR) &&
           (Request->Flags & REQUEST_FLAG_DMA);
}

static
VOID
AtaDeviceSaveCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    /* Update the NCQ state */
    if (DevExt->PortData->ActiveQueuedSlotsBitmap != 0)
        DevExt->PortData->PortFlags |= PORT_FLAG_NCQ;
    else
        DevExt->PortData->PortFlags &= ~PORT_FLAG_NCQ;

    /* Don't save the internal request */
    if (IsPowerOfTwo(DevExt->PortData->ActiveSlotsBitmap))
    {
        PATA_DEVICE_REQUEST Request;
        ULONG Slot;

        NT_VERIFY(_BitScanForward(&Slot, DevExt->PortData->ActiveSlotsBitmap) != 0);

        Request = DevExt->PortData->Slots[Slot];
        ASSERT(Request);

        if (Request->Flags & REQUEST_FLAG_INTERNAL)
        {
            DevExt->PortData->ActiveSlotsBitmap = 0;
        }
    }

    /* Save any commands pending on this device */
    DevExt->WorkerContext.PausedSlotsBitmap |= DevExt->PortData->ActiveSlotsBitmap;

    DevExt->PortData->ActiveSlotsBitmap = 0;
    DevExt->PortData->ActiveQueuedSlotsBitmap = 0;
}

static
inline
VOID
AtaDeviceIssueCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG Flags)
{
    DevExt->WorkerContext.Flags |= Flags | WORKER_NEED_REQUEST;
}

static
inline
VOID
AtaDeviceSetState(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_DEVICE_STATE State)
{
    DevExt->WorkerContext.State = State;
    DevExt->WorkerContext.LocalState = 0;
}

static
VOID
AtaDeviceClearAction(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_DEVICE_ACTION Action)
{
    KeAcquireSpinLockAtDpcLevel(&DevExt->PortData->PortLock);

    DevExt->WorkerContext.Actions &= ~Action;

    KeReleaseSpinLockFromDpcLevel(&DevExt->PortData->PortLock);
}

static
VOID
AtaDeviceSendInternalRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;

    Request->Flags |= REQUEST_FLAG_INTERNAL;

    if (DevExt->WorkerContext.Flags & WORKER_USE_FAILED_SLOT)
    {
        ASSERT(DevExt->WorkerContext.CurrentRequest);
        Request->Slot = DevExt->WorkerContext.CurrentRequest->Slot;
    }
    else
    {
        Request->Slot = 0;
    }
    DevExt->WorkerContext.OldRequest = DevExt->PortData->Slots[Request->Slot];
    DevExt->PortData->Slots[Request->Slot] = Request;

    DevExt->WorkerContext.Flags &= ~(WORKER_NEED_REQUEST | WORKER_USE_FAILED_SLOT);

    DevExt->SendRequest(DevExt, Request);
}

BOOLEAN
AtaDeviceIdentifyDataEqual(
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData1,
    _In_ PIDENTIFY_DEVICE_DATA IdentifyData2)
{
    if (!RtlEqualMemory(IdentifyData1->SerialNumber,
                        IdentifyData2->SerialNumber,
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, SerialNumber)))
    {
        return FALSE;
    }

    if (!RtlEqualMemory(IdentifyData1->FirmwareRevision,
                        IdentifyData2->FirmwareRevision,
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, FirmwareRevision) +
                        RTL_FIELD_SIZE(IDENTIFY_DEVICE_DATA, ModelNumber)))
    {
        return FALSE;
    }

    return TRUE;
}

VOID
AtaDeviceSetupIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ UCHAR Command)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;

    /*
     * For PATA devices, disable interrupts for the identify command and use polling instead.
     *
     * - On some single device 1 configurations or non-existent IDE channels
     *   the status register stuck permanently at a value of 0,
     *   and we incorrectly assume that the device is present.
     *   This will result in a taskfile timeout
     *   which must be avoided as it would cause hangs at boot time.
     *
     * - The NEC CDR-260 drive uses the Packet Command protocol (?) for this command,
     *   which causes a spurious (completion status) interrupt after reading the data buffer.
     */
    Request->Flags = REQUEST_FLAG_DATA_IN | REQUEST_FLAG_HAS_LOCAL_BUFFER;
    if (!IS_AHCI(DevExt))
        Request->Flags |= REQUEST_FLAG_POLL;
    Request->TimeOut = 10;
    Request->DataTransferLength = sizeof(DevExt->IdentifyDeviceData);

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = Command;
}

static
VOID
AtaDeviceSendRequestSense(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;
    PATA_DEVICE_REQUEST FailedRequest = DevExt->WorkerContext.CurrentRequest;
    PCDB Cdb;

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

    AtaDeviceIssueCommand(DevExt, WORKER_USE_FAILED_SLOT);
    AtaDeviceSetState(DevExt, DEVICE_STATE_REQUEST_SENSE);
}

static
VOID
AtaDeviceSendRequestSenseExt(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;

    Request->Flags = REQUEST_FLAG_SAVE_TASK_FILE | REQUEST_FLAG_LBA48;
    Request->TimeOut = 3;

    RtlZeroMemory(&Request->TaskFile, sizeof(Request->TaskFile));
    Request->TaskFile.Command = IDE_COMMAND_REQUEST_SENSE_DATA_EXT;

    AtaDeviceIssueCommand(DevExt, WORKER_USE_FAILED_SLOT);
    AtaDeviceSetState(DevExt, DEVICE_STATE_REQUEST_SENSE);
}

static
VOID
AtaDeviceHandleRequestSense(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;
    PATA_DEVICE_REQUEST FailedRequest = DevExt->WorkerContext.CurrentRequest;
    PSCSI_REQUEST_BLOCK Srb = FailedRequest->Srb;
    BOOLEAN Success;

    if (IS_ATAPI(DevExt))
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

    if (Success)
    {
        PSENSE_DATA SenseData = Srb->SenseInfoBuffer;

        if (IS_ATAPI(DevExt))
        {
            /* Copy the sense data from the local buffer */
            RtlCopyMemory(Srb->SenseInfoBuffer,
                          DevExt->LocalBuffer,
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
            INFO("0x%02x: SK 0x%x, ASC 0x%x, ASCQ 0x%x\n",
                 Srb->Cdb[0],
                 SenseData->SenseKey,
                 SenseData->AdditionalSenseCode,
                 SenseData->AdditionalSenseCodeQualifier);
        }
    }
    else
    {
        ERR("Request sense failed\n");

        if (IS_ATAPI(DevExt))
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

    AtaDeviceSetState(DevExt, DEVICE_STATE_RECOVERY_DONE);
}

static
VOID
AtaDeviceHandleNcqRecovery(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;
    PATA_DEVICE_REQUEST FailedRequest;
    PGP_LOG_NCQ_COMMAND_ERROR LogPage;
    ULONG i;
    UCHAR Crc;

    /* We failed at retrieving the log page */
    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        ERR("READ LOG EXT failure %02x\n", Request->SrbStatus);
        goto Failure;
    }

    LogPage = DevExt->LocalBuffer;

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
    if (!(DevExt->WorkerContext.PausedSlotsBitmap & (1 << LogPage->NcqTag)))
    {
        WARN("Failed command %08lx not found in %08lx\n",
             1 << LogPage->NcqTag, DevExt->WorkerContext.PausedSlotsBitmap);
        goto Failure;
    }
    FailedRequest = DevExt->PortData->Slots[LogPage->NcqTag];
    ASSERT(FailedRequest);
    ASSERT(!(FailedRequest->Flags & REQUEST_FLAG_INTERNAL));

    DevExt->WorkerContext.CurrentRequest = FailedRequest;

    /* Fail the command with an error */
    FailedRequest->Status = LogPage->Status;
    FailedRequest->Error = LogPage->Error;
    FailedRequest->SrbStatus = SRB_STATUS_ERROR;
    if (Request->Flags & REQUEST_FLAG_SAVE_TASK_FILE)
    {
        AtaAhciSaveTaskFile(DevExt->PortData, Request);
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
        AtaDeviceSendRequestSenseExt(DevExt);
        return;
    }
    else
    {
        FailedRequest->SrbStatus = AtaReqTranslateFixedError(FailedRequest);
    }

    TRACE("Complete Srb %p SK %u ASC %u ASCQ %u\n",
          FailedRequest->Srb,
          LogPage->SenseKey,
          LogPage->ASC,
          LogPage->ASCQ);

    AtaDeviceSetState(DevExt, DEVICE_STATE_RECOVERY_DONE);
    return;

Failure:
    /* A port reset is required to abort all outstanding queued commands */
    AtaDeviceSetState(DevExt, DEVICE_STATE_RESET);
}

static
VOID
AtaDeviceSendReadNcqCommandErrorLog(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;

    ASSERT(IS_AHCI(DevExt));

    AtaReqBuildReadLogTaskFile(Request,
                               IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS,
                               0,
                               1);

    Request->Flags |= REQUEST_FLAG_HAS_LOCAL_BUFFER;
    Request->TimeOut = 3;

    AtaDeviceIssueCommand(DevExt, 0);
    AtaDeviceSetState(DevExt, DEVICE_STATE_NCQ_RECOVERY);
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

    if (IS_AHCI(DevExt))
        IoObject = DevExt->Common.Self;
    else
        IoObject = DevExt->ChanExt->Common.Self;

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
BOOLEAN
AtaDeviceDowngradeTransferSpeed(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    return FALSE;
}

static
VOID
AtaDeviceDisableDma(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    WARN("DMA failures, disabling DMA for '%s'\n", DevExt->FriendlyName);

    AtaDeviceLogEvent(DevExt, EVENT_CODE_DMA_DISABLE);

    DevExt->Flags |= DEVICE_PIO_ONLY;

    /* Request again for a SRB translation */
    DevExt->WorkerContext.Flags |= WORKER_REQUEUE_REQUESTS;
}

static
VOID
AtaDeviceAnalyzeDmaError(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST FailedRequest)
{
    WARN("DMA error\n");

    if (FailedRequest->SrbStatus == SRB_STATUS_TIMEOUT)
    {

        AtaDeviceLogEvent(DevExt, EVENT_CODE_TIMEOUT);
    }
    else
    {
        AtaDeviceLogEvent(DevExt, EVENT_CODE_BAD_STATE);
    }

    if (IS_AHCI(DevExt))
    {
        if (AtaAhciDowngradeInterfaceSpeed(DevExt))
            return;
    }

    if (AtaDeviceDowngradeTransferSpeed(DevExt))
        return;

    if (!(DevExt->Flags & DEVICE_PIO_ONLY))
        AtaDeviceDisableDma(DevExt);
}

static
VOID
AtaDeviceAnalyzeDeviceError(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST FailedRequest;

    if (DevExt->PortData->PortFlags & PORT_FLAG_NCQ)
    {
        /* Handle failed queued commands */
        AtaDeviceSendReadNcqCommandErrorLog(DevExt);
        return;
    }

    FailedRequest = DevExt->WorkerContext.CurrentRequest;
    ASSERT(FailedRequest);
    ASSERT(!(FailedRequest->Flags & REQUEST_FLAG_INTERNAL));

    /* We silently ignore device errors caused by PASSTHROUGH commands */
    switch (FailedRequest->SrbStatus)
    {
        case SRB_STATUS_BUS_RESET:
        case SRB_STATUS_TIMEOUT:
        {
            /* DMA timeouts usually indicate a bad connection (a bad cable) */
            if ((FailedRequest->Flags & REQUEST_FLAG_DMA) &&
                !(FailedRequest->Flags & REQUEST_FLAG_PASSTHROUGH))
            {
                AtaDeviceAnalyzeDmaError(DevExt, FailedRequest);
            }

            /* Issue a reset to recover a device */
            AtaDeviceSetState(DevExt, DEVICE_STATE_RESET);
            break;
        }

        case SRB_STATUS_ERROR:
        {
            if (AtaDeviceIsDmaCrcError(DevExt, FailedRequest) &&
                !(FailedRequest->Flags & REQUEST_FLAG_PASSTHROUGH))
            {
                AtaDeviceAnalyzeDmaError(DevExt, FailedRequest);

                /* CRC error occurred, retry in PIO mode */
                if (!IS_AHCI(DevExt) && AtaPataPreparePioDataTransfer(DevExt, FailedRequest))
                {
                    FailedRequest->SrbStatus = SRB_STATUS_BUSY;

                    AtaDeviceSetState(DevExt, DEVICE_STATE_RECOVERY_DONE);
                    break;
                }
            }

            /* Send the recovery command to figure out why the current command failed */
            if (AtaDeviceRequestSenseNeeded(FailedRequest))
            {
                /* Handle failed ATAPI commands */
                AtaDeviceSendRequestSense(DevExt);
                break;
            }
            else if (AtaDeviceRequestSenseNeededExt(DevExt, FailedRequest))
            {
                /* Handle failed non-queued commands */
                AtaDeviceSendRequestSenseExt(DevExt);
                break;
            }

            /* Recovery command is not required, just complete the request with an error */
            __fallthrough;
        }
        default:
        {
            AtaDeviceSetState(DevExt, DEVICE_STATE_RECOVERY_DONE);
            break;
        }
    }
}

static
VOID
AtaDeviceCompleteFailedRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST FailedRequest)
{
    UCHAR SrbStatus;

    if (!FailedRequest)
        return;

    ASSERT(!(FailedRequest->Flags & REQUEST_FLAG_INTERNAL));

    SrbStatus = SRB_STATUS(FailedRequest->SrbStatus);

    if (FailedRequest->RetryCount++ >= 2)
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
            case SRB_STATUS_TIMEOUT:
            {
                /* DMA timeout, attempt to retry in PIO mode */
                if (!IS_AHCI(DevExt) && (FailedRequest->Flags & REQUEST_FLAG_DMA))
                {
                    (VOID)AtaPataPreparePioDataTransfer(DevExt, FailedRequest);
                }

                SrbStatus = SRB_STATUS_BUSY;
                break;
            }

            case SRB_STATUS_BUS_RESET:
            case SRB_STATUS_REQUEST_SENSE_FAILED:
            {
                SrbStatus = SRB_STATUS_BUSY;
                break;
            }

            default:
            {
                if (FailedRequest->Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_PAGING_IO))
                    SrbStatus = SRB_STATUS_BUSY;
                break;
            }
        }
    }

    if (SrbStatus == SRB_STATUS_BUSY)
        return;

    DevExt->WorkerContext.PausedSlotsBitmap &= ~(1 << FailedRequest->Slot);

    /* Fail the command with an error */
    FailedRequest->InternalState = REQUEST_STATE_FREEZE_QUEUE;
    AtaReqStartCompletionDpc(FailedRequest);
}

static
VOID
AtaDeviceHandleRecoveryDone(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ASSERT(DevExt->WorkerContext.CurrentRequest != NULL);

    AtaDeviceClearAction(DevExt, ACTION_ERROR);
    AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);
}

static
VOID
AtaDeviceHandleReset(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    BOOLEAN Success;

    if (IS_AHCI(DevExt))
        Success = AtaAhciResetDevice(DevExt);
    else
        Success = AtaPataResetDevice(DevExt);
    if (Success)
    {
        AtaDeviceClearAction(DevExt,
                             ACTION_RESET | ACTION_ERROR | ACTION_DEV_CHANGE | ACTION_CONFIG);
        AtaDeviceSetState(DevExt, DEVICE_STATE_ENUM);
    }
    else
    {
        DevExt->WorkerContext.Flags |= WORKER_DEVICE_CHANGE;
        AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);
    }
}

static
VOID
AtaDeviceHandleResetting(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    // TODO
}

static
VOID
AtaDeviceHandleEnum(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    BOOLEAN Success;

    if (IS_AHCI(DevExt))
        Success = AtaAhciEnumerateDevice(DevExt);
    else
        Success = AtaPataEnumerateDevice(DevExt);
    if (!Success)
    {
        AtaDeviceSetState(DevExt, DEVICE_STATE_RESET);
        return;
    }

    AtaDeviceClearAction(DevExt, ACTION_ENUM);

    if (DevExt->WorkerContext.ConnectionStatus == CONN_STATUS_NO_DEVICE)
    {
        DevExt->WorkerContext.Flags |= WORKER_DEVICE_CHANGE;
        AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);
        return;
    }

    AtaDeviceSetState(DevExt, DEVICE_STATE_IDENTIFY);
}

static
VOID
AtaDeviceHandleIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;
    PIDENTIFY_DEVICE_DATA IdentifyData = DevExt->LocalBuffer;

    HSM_BEGIN(DevExt);

    AtaDeviceSetupIdentify(DevExt,
                           (DevExt->WorkerContext.ConnectionStatus == CONN_STATUS_DEV_ATAPI)
                           ? IDE_COMMAND_ATAPI_IDENTIFY : IDE_COMMAND_IDENTIFY);
    HSM_ISSUE_COMMAND(DevExt);

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
    {
        DevExt->WorkerContext.Flags |= WORKER_DEVICE_CHANGE;
        AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);
        return;
    }

    /* Check if the device was replaced with another */
    if (!(DevExt->Flags & DEVICE_ENUM) &&
        !AtaDeviceIdentifyDataEqual(&DevExt->IdentifyDeviceData, IdentifyData))
    {
        INFO("Identify data has changed\n");

        DevExt->WorkerContext.Flags |= WORKER_DEVICE_CHANGE;
        AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);
        return;
    }

    /* Update identify data */
    RtlCopyMemory(&DevExt->IdentifyDeviceData,
                  IdentifyData,
                  sizeof(*IdentifyData));

    if (DevExt->Flags & DEVICE_ACTIVE)
        AtaDeviceSetState(DevExt, DEVICE_STATE_CONFIGURE);
    else
        AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);

    HSM_END(DevExt);
}

static
VOID
AtaDeviceHandleSetConfig(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request = DevExt->InternalRequest;
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    PIDENTIFY_DEVICE_DATA IdentifyData;

    /* Restore the ATA volatile settings */
    HSM_BEGIN(DevExt);

    DevExt->Flags &= ~DEVICE_HAS_MEDIA_STATUS;
    DevExt->MultiSectorTransfer = 0;

    /* Select a CHS translation (needs to be issued just after the identify command) */
    if (!IS_ATAPI(DevExt) && !(DevExt->Flags & DEVICE_LBA_MODE))
    {
        Request->Flags = REQUEST_FLAG_SET_DEVICE_REGISTER;
        Request->TimeOut = 5;

        RtlZeroMemory(TaskFile, sizeof(*TaskFile));
        TaskFile->Command = IDE_COMMAND_SET_DRIVE_PARAMETERS;
        TaskFile->SectorCount = (UCHAR)DevExt->SectorsPerTrack;
        TaskFile->DriveSelect = (UCHAR)(DevExt->Heads - 1) | DevExt->DeviceSelect;
        HSM_ISSUE_COMMAND(DevExt);

        if (Request->SrbStatus != SRB_STATUS_SUCCESS)
        {
            WARN("Device parameters failed 0x%02x 0x%02x\n",
                 Request->Error, Request->SrbStatus);
        }
        else
        {
            INFO("Device parameters initialized\n");
        }
    }

    /* Enable the 32-bit PIO mode for the best performance */
    if (!IS_AHCI(DevExt) &&
        !(DevExt->PortData->PortFlags & (PORT_FLAG_IO_MODE_DETECTED | PORT_FLAG_CBUS_IDE)))
    {
        /* Switch the controller over to 32-bit I/O mode */
        DevExt->PortData->PortFlags |= PORT_FLAG_IO32 | PORT_FLAG_IO_MODE_DETECTED;

        /* Send the identify command */
        Request->Flags = REQUEST_FLAG_DATA_IN | REQUEST_FLAG_HAS_LOCAL_BUFFER;
        if (!IS_AHCI(DevExt))
            Request->Flags |= REQUEST_FLAG_POLL;
        Request->TimeOut = 4;
        Request->DataTransferLength = sizeof(DevExt->IdentifyDeviceData);

        RtlZeroMemory(TaskFile, sizeof(*TaskFile));
        if (IS_ATAPI(DevExt))
            TaskFile->Command = IDE_COMMAND_IDENTIFY;
        else
            TaskFile->Command = IDE_COMMAND_ATAPI_IDENTIFY;
        HSM_ISSUE_COMMAND(DevExt);

        IdentifyData = DevExt->LocalBuffer;
        /*
         * Check if the controller is able to translate a 32-bit I/O cycle
         * into two 16-bit cycles.
         */
        if ((Request->SrbStatus == SRB_STATUS_SUCCESS) &&
            RtlEqualMemory(IdentifyData, &DevExt->IdentifyDeviceData, sizeof(*IdentifyData)))
        {
            INFO("32-bit I/O supported\n");
        }
        else
        {
            INFO("32-bit I/O is not supported\n");
            DevExt->PortData->PortFlags &= ~PORT_FLAG_IO32;
        }
    }

    /* Enable multiple mode */
    if (!IS_ATAPI(DevExt))
    {
        DevExt->MultiSectorTransfer = AtaDevMaximumSectorsPerDrq(&DevExt->IdentifyDeviceData);
        if (DevExt->MultiSectorTransfer != 0)
        {
            Request->Flags = 0;
            Request->TimeOut = 3;

            RtlZeroMemory(TaskFile, sizeof(*TaskFile));
            TaskFile->Command = IDE_COMMAND_SET_MULTIPLE;
            TaskFile->SectorCount = DevExt->MultiSectorTransfer;
            HSM_ISSUE_COMMAND(DevExt);

            if (Request->SrbStatus != SRB_STATUS_SUCCESS)
            {
                DevExt->MultiSectorTransfer = 0;

                WARN("Multiple mode failed 0x%02x 0x%02x\n",
                     Request->Error, Request->SrbStatus);
            }
            else
            {
                INFO("Multiple mode %u sectors\n", DevExt->MultiSectorTransfer);
            }
        }
    }

    /* Enable the MSN feature */
    if (AtaDevHasRemovableMediaStatusNotification(&DevExt->IdentifyDeviceData))
    {
        Request->Flags = 0;
        Request->TimeOut = 3;

        RtlZeroMemory(TaskFile, sizeof(*TaskFile));
        TaskFile->Command = IDE_COMMAND_SET_FEATURE;
        TaskFile->Feature = IDE_FEATURE_ENABLE_MSN;
        HSM_ISSUE_COMMAND(DevExt);

        if (Request->SrbStatus != SRB_STATUS_SUCCESS)
        {
            WARN("MSN setup failed 0x%02x 0x%02x\n",
                 Request->Error, Request->SrbStatus);
        }
        else
        {
            INFO("MSN enabled\n");
            DevExt->Flags |= DEVICE_HAS_MEDIA_STATUS;
        }
    }

    /* Lock the security mode feature commands (secure erase and friends) */
    if (!IS_ATAPI(DevExt) && !AtapInPEMode)
    {
        Request->Flags = 0;
        Request->TimeOut = 3;

        RtlZeroMemory(TaskFile, sizeof(*TaskFile));
        TaskFile->Command = IDE_COMMAND_SECURITY_FREEZE_LOCK;
        HSM_ISSUE_COMMAND(DevExt);
    }

    AtaDeviceClearAction(DevExt, ACTION_CONFIG);

    AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);

    HSM_END(DevExt);
}

static
BOOLEAN
AtaDeviceHandleNextAction(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ATA_DEVICE_STATE State;
    BOOLEAN EnumerateDevices;
    ULONG i, SlotsBitmap;

    KeAcquireSpinLockAtDpcLevel(&DevExt->PortData->PortLock);

    /* Device change action has the highest priority */
    if ((DevExt->WorkerContext.Flags & WORKER_DEVICE_CHANGE) ||
        (DevExt->WorkerContext.Actions & ACTION_DEV_CHANGE))
    {
        State = DEVICE_STATE_NONE;
    }
    else if (DevExt->WorkerContext.Actions & ACTION_RESET)
    {
        State = DEVICE_STATE_RESET;
    }
    else if (DevExt->WorkerContext.Actions & ACTION_INTERNAL_COMMAND)
    {
        State = DEVICE_STATE_INTERNAL_COMMAND;
    }
    else if (DevExt->WorkerContext.Actions & ACTION_ERROR)
    {
        State = DEVICE_STATE_NEED_RECOVERY;
    }
    else if (DevExt->WorkerContext.Actions & ACTION_ENUM)
    {
        State = DEVICE_STATE_ENUM;
    }
    else if (DevExt->WorkerContext.Actions & ACTION_CONFIG)
    {
        State = DEVICE_STATE_CONFIGURE;
    }
    else
    {
        State = DEVICE_STATE_NONE;
    }

    if (State != DEVICE_STATE_NONE)
    {
        KeReleaseSpinLockFromDpcLevel(&DevExt->PortData->PortLock);

        AtaDeviceSetState(DevExt, State);
        return TRUE;
    }

    AtaDeviceCompleteFailedRequest(DevExt, DevExt->WorkerContext.CurrentRequest);

    EnumerateDevices = !!(DevExt->WorkerContext.Actions & ACTION_DEV_CHANGE);
    SlotsBitmap = DevExt->WorkerContext.PausedSlotsBitmap;

    if (DevExt->WorkerContext.HandledActions & ACTION_INTERNAL_COMMAND)
        KeSetEvent(&DevExt->WorkerContext.CompletedEvent, 0, FALSE);
    if (DevExt->WorkerContext.HandledActions & ACTION_ENUM)
        KeSetEvent(&DevExt->WorkerContext.EnumeratedEvent, 0, FALSE);
    if (DevExt->WorkerContext.HandledActions & ACTION_CONFIG)
        KeSetEvent(&DevExt->WorkerContext.ConfiguredEvent, 0, FALSE);

    DevExt->WorkerContext.Actions = 0;
    DevExt->WorkerContext.HandledActions = 0;
    DevExt->WorkerContext.Flags = 0;
    DevExt->WorkerContext.PausedSlotsBitmap = 0;
    DevExt->WorkerContext.CurrentRequest = NULL;

    DevExt->PortData->PortFlags |= PORT_FLAG_ACTIVE;

    KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);
    DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_DEVICE_ERROR;
    KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

    KeReleaseSpinLockFromDpcLevel(&DevExt->PortData->PortLock);

    for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
    {
        PATA_DEVICE_REQUEST Request;

        if (!(SlotsBitmap & (1 << i)))
            continue;

        Request = DevExt->PortData->Slots[i];
        ASSERT(Request);

        Request->InternalState = REQUEST_STATE_NONE;

        if (EnumerateDevices)
        {
            Request->SrbStatus = SRB_STATUS_BUS_RESET;
            AtaReqStartCompletionDpc(Request);
        }
        else
        {
            /* Re-issue commands to the device */
            AtaReqStartIo(DevExt, Request);
        }
    }

    if (EnumerateDevices)
        IoInvalidateDeviceRelations(DevExt->ChanExt->Common.Self, BusRelations);

    /* Start the next request on the queue */
    AtaReqThawQueue(DevExt, 0xFFFFFFFF);

    return FALSE;
}

VOID
NTAPI
AtaDeviceWorker(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_DEVICE_EXTENSION DevExt = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    while (TRUE)
    {
        BOOLEAN DoProcessAgain = TRUE;

        switch (DevExt->WorkerContext.State)
        {
            case DEVICE_STATE_NEED_RECOVERY:
            {
                AtaDeviceAnalyzeDeviceError(DevExt);
                break;
            }
            case DEVICE_STATE_REQUEST_SENSE:
            {
                AtaDeviceHandleRequestSense(DevExt);
                break;
            }
            case DEVICE_STATE_NCQ_RECOVERY:
            {
                AtaDeviceHandleNcqRecovery(DevExt);
                break;
            }
            case DEVICE_STATE_RECOVERY_DONE:
            {
                AtaDeviceHandleRecoveryDone(DevExt);
                break;
            }
            case DEVICE_STATE_RESET:
            {
                AtaDeviceHandleReset(DevExt);
                break;
            }
            case DEVICE_STATE_RESETTING:
            {
                AtaDeviceHandleResetting(DevExt);
                break;
            }
            case DEVICE_STATE_ENUM:
            {
                AtaDeviceHandleEnum(DevExt);
                break;
            }
            case DEVICE_STATE_IDENTIFY:
            {
                AtaDeviceHandleIdentify(DevExt);
                break;
            }
            case DEVICE_STATE_CONFIGURE:
            {
                AtaDeviceHandleSetConfig(DevExt);
                break;
            }
            case DEVICE_STATE_INTERNAL_COMMAND:
            {
                HSM_BEGIN(DevExt);

                HSM_ISSUE_COMMAND(DevExt);

                DevExt->WorkerContext.CommandSuccess =
                    (DevExt->InternalRequest->SrbStatus == SRB_STATUS_SUCCESS);

                AtaDeviceClearAction(DevExt, ACTION_INTERNAL_COMMAND);
                AtaDeviceSetState(DevExt, DEVICE_STATE_NEXT_ACTION);

                HSM_END(DevExt);
                break;
            }
            case DEVICE_STATE_NEXT_ACTION:
            {
                DoProcessAgain = AtaDeviceHandleNextAction(DevExt);
                break;
            }

            default:
                ASSERT(FALSE);
                UNREACHABLE;
        }

        if (!DoProcessAgain)
            break;

        /* Check if new events have occurred */
        if (DevExt->WorkerContext.PortWorkerStopped)
            break;

        /* Perform I/O operation */
        if (DevExt->WorkerContext.Flags & WORKER_NEED_REQUEST)
        {
            AtaDeviceSendInternalRequest(DevExt);
            break;
        }
    }
}

static
VOID
AtaDeviceFreezeDeviceQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    /* Stop the Srb processing */
    AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_DEVICE_ERROR);

    /* Don't issue any new commands to the device */
    DevExt->PortData->PortFlags &= ~PORT_FLAG_ACTIVE;

    /* Save the pending commands and stop all timers */
    AtaDeviceSaveCommands(DevExt);
}

VOID
AtaDeviceTimeout(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG Slot)
{
    PATA_DEVICE_REQUEST Request;
    PATAPORT_DEVICE_EXTENSION DevExt;

    Request = PortData->Slots[Slot];
    ASSERT(Request);
    ASSERT(Request->Signature == ATA_DEVICE_REQUEST_SIGNATURE);

    Request->SrbStatus = SRB_STATUS_TIMEOUT;

    /* Set the ATA outputs to something meaningful */
    Request->Status = IDE_STATUS_ERROR;
    Request->Error = IDE_ERROR_COMMAND_ABORTED;

    DevExt = Request->DevExt;

    ERR("%sSlot %lu (%08lx) timed out\n", IS_ATAPI(DevExt) ? "" : "*", Slot, 1 << Slot);

    AtaReqCompleteFailedRequest(Request);
}

ATA_COMPLETION_ACTION
AdaDeviceCompleteInternalRequest(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;

    DevExt->PortData->Slots[Request->Slot] = DevExt->WorkerContext.OldRequest;

    /* Internal command timed out */
    if (Request->SrbStatus == SRB_STATUS_TIMEOUT)
    {
        DevExt->WorkerContext.Flags &= ~(WORKER_NEED_REQUEST | WORKER_NEED_SYSTEM_THREAD);
        AtaDeviceSetState(DevExt, DEVICE_STATE_RESET);
    }

    KeInsertQueueDpc(&DevExt->WorkerContext.Dpc, NULL, NULL);

    return COMPLETE_IRP;
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaDeviceQueueAction(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_DEVICE_ACTION Action)
{
    KIRQL OldLevel;
    BOOLEAN NeedLock;

    NeedLock = !!(Action & (ACTION_ENUM | ACTION_CONFIG | ACTION_INTERNAL_COMMAND));

    if (NeedLock)
        KeAcquireSpinLock(&DevExt->PortData->PortLock, &OldLevel);

    DevExt->WorkerContext.Actions |= Action;
    DevExt->WorkerContext.HandledActions |= Action;

    if (DevExt->WorkerContext.Flags & WORKER_IS_ACTIVE)
    {
        if (NeedLock)
            KeReleaseSpinLock(&DevExt->PortData->PortLock, OldLevel);
        return;
    }

    AtaDeviceFreezeDeviceQueue(DevExt);

    DevExt->WorkerContext.Flags = WORKER_IS_ACTIVE;

    if (NeedLock)
        KeReleaseSpinLock(&DevExt->PortData->PortLock, OldLevel);

    DevExt->WorkerContext.State = DEVICE_STATE_NEXT_ACTION;

    KeInsertQueueDpc(&DevExt->WorkerContext.Dpc, NULL, NULL);
}
