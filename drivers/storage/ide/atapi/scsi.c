/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI requests handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

extern VOID
AtaAhciExecuteCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

extern VOID
AtaAhciPortResumeCommands(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

static DRIVER_CANCEL AtaReqWaitQueueCancelIo;

static
VOID
AtaReqWaitQueueGetNextRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

static
NTSTATUS
AdaReqWaitQueueAddSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

static
VOID
AtaReqDispatchRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

ULONG
AtaReqPrepareDataTransfer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request);

/* FUNCTIONS ******************************************************************/

static
NTSTATUS
AtaSrbStatusToNtStatus(
    _In_ UCHAR SrbStatus)
{
    ASSERT(SRB_STATUS(SrbStatus) != SRB_STATUS_PENDING);

    switch (SRB_STATUS(SrbStatus))
    {
        case SRB_STATUS_SUCCESS:
            return STATUS_SUCCESS;

        case SRB_STATUS_TIMEOUT:
        case SRB_STATUS_COMMAND_TIMEOUT:
            return STATUS_IO_TIMEOUT;

        case SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
        case SRB_STATUS_BAD_FUNCTION:
            return STATUS_INVALID_DEVICE_REQUEST;

        case SRB_STATUS_NO_DEVICE:
        case SRB_STATUS_INVALID_LUN:
        case SRB_STATUS_INVALID_TARGET_ID:
        case SRB_STATUS_NO_HBA:
            return STATUS_DEVICE_DOES_NOT_EXIST;

        case SRB_STATUS_DATA_OVERRUN:
            return STATUS_BUFFER_OVERFLOW;

        case SRB_STATUS_SELECTION_TIMEOUT:
            return STATUS_DEVICE_NOT_CONNECTED;

        default:
            break;
    }

    return STATUS_IO_DEVICE_ERROR;
}

static
ATA_COMPLETION_ACTION
AtaReqRequeueRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    NTSTATUS Status;

    if (SRB_GET_FLAGS(Request->Srb) & SRB_FLAG_RETRY)
        return COMPLETE_IRP;

    /* Retry once */
    SRB_SET_FLAGS(Request->Srb, SRB_FLAG_RETRY);

    /* Place the Srb back into the queue */
    Status = AdaReqWaitQueueAddSrb(DevExt, Request->Srb);
    if (Status != STATUS_PENDING)
        return COMPLETE_IRP;

    return COMPLETE_NO_IRP;
}

static
PMDL
AtaAllocateMdl(
    _In_ PVOID Buffer,
    _In_ ULONG Length)
{
    PMDL Mdl;

    Mdl = IoAllocateMdl(Buffer, Length, FALSE, FALSE, NULL);
    if (!Mdl)
        return NULL;

    MmBuildMdlForNonPagedPool(Mdl);

    return Mdl;
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteReadLogExt(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;
    PGP_LOG_NCQ_COMMAND_ERROR LogPage;
    ULONG i;
    UCHAR Crc;

    if (Request->SrbStatus != SRB_STATUS_SUCCESS)
        return COMPLETE_IRP;

    LogPage = DevExt->PortData->LogBuffer;

    if (LogPage->UNL)
        return COMPLETE_IRP;

    Crc = 0;
    for (i = 0; i < IDE_GP_LOG_SECTOR_SIZE; ++i)
    {
        Crc += ((PUCHAR)LogPage)[i];
    }
    if (Crc != 0)
    {
        ERR("CRC error in the log page 0x10 structure\n", Crc);
        return COMPLETE_IRP;
    }

    if (DevExt->PausedSlotsBitmap & (1 << LogPage->NcqTag))
    {
        WARN("Failed command %08lx not found\n", 1 << LogPage->NcqTag);
        return COMPLETE_IRP;
    }

    if (LogPage->SenseKey != 0)
    {
        // TODO A.14.6
    }

    Request->SrbStatus = SRB_STATUS_ERROR;
    return COMPLETE_IRP;
}

static
ULONG
AtaReqSendReadLogExt(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATA_TASKFILE TaskFile = &Request->TaskFile;

    /* PIO Data-In command  */
    Request->Flags = REQUEST_FLAG_DATA_IN |
                     REQUEST_FLAG_LBA48 |
                     REQUEST_FLAG_RECOVERY |
                     REQUEST_BYPASS_ACTIVE_QUEUE;
    Request->Complete = AtaReqCompleteReadLogExt;

    Request->Mdl = AtaAllocateMdl(DevExt->PortData->LogBuffer, IDE_GP_LOG_SECTOR_SIZE);
    if (!Request->Mdl)
        return SRB_STATUS_INSUFFICIENT_RESOURCES;

    Request->Flags |= REQUEST_FLAG_HAS_MDL;
    Request->DataBuffer = DevExt->PortData->LogBuffer;
    Request->DataTransferLength = IDE_GP_LOG_SECTOR_SIZE;
    Request->TimeOut = 3;

    TaskFile->Feature = 0;
    TaskFile->FeatureEx = 0;

    /* LOG PAGE COUNT */
    TaskFile->SectorCount = 1;
    TaskFile->SectorCountEx = 0;

    TaskFile->LowLba = IDE_GP_LOG_NCQ_COMMAND_ERROR_ADDRESS; // LOG ADDRESS
    TaskFile->MidLba = 0;    // PAGE NUMBER
    TaskFile->HighLba = 0;   // Reserved
    TaskFile->LowLbaEx = 0;  // Reserved
    TaskFile->MidLbaEx = 0;  // PAGE NUMBER EX
    TaskFile->HighLbaEx = 0; // Reserved
    TaskFile->Command = IDE_COMMAND_READ_LOG_EXT;

    return AtaReqPrepareDataTransfer(DevExt, Request);
}

static
ATA_COMPLETION_ACTION
AtaReqCompleteRequestSense(
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Request->SrbStatus == SRB_STATUS_SUCCESS)
    {
        Request->Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        Request->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
    }
    else
    {
        Request->Srb->ScsiStatus = SCSISTAT_GOOD;
        Request->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;
    }

    return COMPLETE_IRP;
}

static
ULONG
AtaReqSendRequestSense(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PSCSI_REQUEST_BLOCK Srb = Request->Srb;
    PCDB Cdb;

    /* Don't use DMA for the transfer to avoid DMA errors */
    Request->Flags = REQUEST_FLAG_DATA_IN |
                     REQUEST_FLAG_PACKET_COMMAND |
                     REQUEST_FLAG_RECOVERY |
                     REQUEST_BYPASS_ACTIVE_QUEUE;
    Request->Complete = AtaReqCompleteRequestSense;

    Request->Mdl = AtaAllocateMdl(Srb->SenseInfoBuffer, Srb->SenseInfoBufferLength);
    if (!Request->Mdl)
        return SRB_STATUS_INSUFFICIENT_RESOURCES;

    Request->Flags |= REQUEST_FLAG_HAS_MDL;
    Request->DataBuffer = Srb->SenseInfoBuffer;
    Request->DataTransferLength = Srb->SenseInfoBufferLength;
    Request->TimeOut = 3;

    /* Build CDB for REQUEST SENSE */
    Cdb = (PCDB)Request->Cdb;
    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    Cdb->CDB6INQUIRY.Reserved1 = 0;
    Cdb->CDB6INQUIRY.PageCode = 0;
    Cdb->CDB6INQUIRY.IReserved = 0;
    Cdb->CDB6INQUIRY.AllocationLength = (UCHAR)Request->DataTransferLength;
    Cdb->CDB6INQUIRY.Control = 0;

    return AtaReqPrepareDataTransfer(DevExt, Request);
}

static
VOID
AtaReqReleaseResources(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
    {
        PDMA_ADAPTER AdapterObject = DevExt->ChanExt->AdapterObject;
        PDMA_OPERATIONS DmaOperations = AdapterObject->DmaOperations;

        DmaOperations->PutScatterGatherList(AdapterObject,
                                            Request->SgList,
                                            !!(Request->Flags & REQUEST_FLAG_DATA_IN));
    }

    if (Request->Flags & REQUEST_FLAG_HAS_MDL)
    {
        IoFreeMdl(Request->Mdl);
    }

    if (Request->Flags & REQUEST_FLAG_RESERVED_MAPPING)
    {
        MmUnmapReservedMapping(DevExt->ChanExt->ReservedVaSpace, ATAPORT_TAG, Request->Mdl);
    }
}

VOID
AtaReqCompleteRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PSCSI_REQUEST_BLOCK Srb;
    ATA_COMPLETION_ACTION CompletionAction;
    PIRP Irp;
    ULONG SrbStatus;

    if (Request->InternalState == REQUEST_STATE_NOT_STARTED)
    {
        CompletionAction = COMPLETE_IRP;
    }
    else
    {
        AtaReqReleaseResources(DevExt, Request);

        CompletionAction = Request->Complete(Request);

        if (Request->InternalState == REQUEST_STATE_NEED_RECOVERY)
        {
            ULONG SrbStatus = (ULONG)-1;

            if ((Request->Flags & REQUEST_FLAG_PACKET_COMMAND) &&
                (Request->Srb->SenseInfoBuffer != NULL) &&
                (Request->Srb->SenseInfoBufferLength != 0))
            {
                /* Handle failed ATAPI commands */
                SrbStatus = AtaReqSendRequestSense(DevExt, Request);
            }
            else if (Request->Flags & REQUEST_FLAG_NCQ)
            {
                /* Handle failed queued commands */
                SrbStatus = AtaReqSendReadLogExt(DevExt, Request);
            }

            if (SrbStatus == SRB_STATUS_PENDING)
            {
                /* Wait for the recovery command to complete */
                Request->InternalState = REQUEST_STATE_RECOVERY;
                return;
            }
            else if (SrbStatus != (ULONG)-1)
            {
                /* Failed to send the recovery command */
                AtaReqCompleteRequest(DevExt, Request);
                return;
            }
            else
            {
                /* Recovery command is not required, just complete the request with an error */
                Request->Flags |= REQUEST_BYPASS_ACTIVE_QUEUE;
            }
        }

        if (Request->Flags & REQUEST_BYPASS_ACTIVE_QUEUE)
        {
            AtaAhciPortResumeCommands(DevExt);
        }
    }

    Srb = Request->Srb;
    Srb->SrbStatus = Request->SrbStatus;

    Irp = Request->Irp;
    Irp->IoStatus.Information = Request->DataTransferLength;
    Irp->IoStatus.Status = AtaSrbStatusToNtStatus(Srb->SrbStatus);

    SrbStatus = SRB_STATUS(Request->SrbStatus);

    KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);

    /* Release the request back to the queue */
    ASSERT(!(DevExt->FreeRequestsBitmap & (1 << Request->Tag)));
    DevExt->FreeRequestsBitmap |= (1 << Request->Tag);

    /* Release the slot back to the queue */
    if (Request->InternalState != REQUEST_STATE_NOT_STARTED)
    {
        if (Request->Flags & REQUEST_FLAG_NCQ)
            ++DevExt->AllocatedSlots;
        else
            --DevExt->AllocatedSlots;

        ASSERT(!(DevExt->FreeSlotsBitmap & (1 << Request->Slot)));
        DevExt->FreeSlotsBitmap |= (1 << Request->Slot);
#if DBG
        DevExt->Slots[Request->Slot] = NULL;
#endif
    }

    /* Release exclusive access to the slot queue */
    if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_EXCLUSIVE_MODE;
    }

    /* Check if we need to retry later */
    if (SrbStatus != SRB_STATUS_SUCCESS && SrbStatus != SRB_STATUS_DATA_OVERRUN)
    {
        if ((SrbStatus == STATUS_INSUFFICIENT_RESOURCES) ||
            (SrbStatus == SRB_STATUS_REQUEST_SENSE_FAILED) ||
            (Request->Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_PAGING_IO)))
        {
            CompletionAction = AtaReqRequeueRequest(DevExt, Request);

            if ((CompletionAction == COMPLETE_IRP) && (SrbStatus == STATUS_INSUFFICIENT_RESOURCES))
            {
                Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
                Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    /* Freeze the Srb queue in case of device error */
    if ((SrbStatus == SRB_STATUS_ERROR) && !(Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        /* DevExt->QueueFlags |= QUEUE_FLAG_FROZEN_QUEUE_LOCK; */
        /* Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN; */
    }

    /* Try to unfreeze the slot queue */
    if (DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT)
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;
        AtaReqDispatchRequest(DevExt, DevExt->PendingRequest);
    }

    /*
     * Start the next request on the queue.
     * It's important to do this before actually completing an IRP.
     */
    AtaReqWaitQueueGetNextRequest(DevExt);

    /* Signal the event when the list of active IRPs is empty */
    if ((DevExt->QueueFlags & QUEUE_FLAG_SIGNAL_STOP) &&
        (DevExt->FreeRequestsBitmap == DevExt->MaxRequestsBitmap))
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_SIGNAL_STOP;

        KeSetEvent(&DevExt->QueueStoppedEvent, 0, FALSE);
    }

#if DBG
    if (CompletionAction == COMPLETE_IRP)
        DevExt->RequestsCompleted++;
#endif

    KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

    /*
     * Complete the IRP outside of the spinlock.
     * A new SCSI device I/O request might send immediately after the IRP completed.
     */
    if (CompletionAction == COMPLETE_IRP)
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
}

ULONG
AtaReqPrepareDataTransfer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;
    NTSTATUS Status;
    PDMA_ADAPTER AdapterObject = ChanExt->AdapterObject;
    PDMA_OPERATIONS DmaOperations = AdapterObject->DmaOperations;

    Status = DmaOperations->GetScatterGatherList(AdapterObject,
                                                 ChanExt->AdapterDeviceObject,
                                                 Request->Mdl,
                                                 Request->DataBuffer,
                                                 Request->DataTransferLength,
                                                 AtaAhciPreparePrdTable,
                                                 Request,
                                                 !!(Request->Flags & REQUEST_FLAG_DATA_IN));
    if (NT_SUCCESS(Status))
        return SRB_STATUS_PENDING;

    return SRB_STATUS_INSUFFICIENT_RESOURCES;
}

VOID
NTAPI
AtaReqCompletionDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_DEVICE_EXTENSION DevExt = DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);

    while (!IsListEmpty(&DevExt->CompletionList))
    {
        PATA_DEVICE_REQUEST Request;
        PIRP Irp;
        PLIST_ENTRY Entry;

        Entry = RemoveHeadList(&DevExt->CompletionList);

        KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

        Irp = IRP_FROM_LIST_ENTRY(Entry);
        Request = Irp->Tail.Overlay.DriverContext[3];
        AtaReqCompleteRequest(DevExt, Request);

        KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);
    }

    KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);
}

static
VOID
AtaReqStartCompletionDpc(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
   /*
    * We cannot complete request with the queue spinlock held.
    * That's why we need to defer the completion to a DPC.
    */
    Request->Irp->Tail.Overlay.DriverContext[3] = Request;
    InsertTailList(&DevExt->CompletionList, LIST_ENTRY_FROM_IRP(Request->Irp));

    KeInsertQueueDpc(&DevExt->CompletionDpc, NULL, NULL);
}

static
BOOLEAN
AtaReqAllocateSlot(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG SlotMask, SlotNumber, SlotsBitmap;

    if (DevExt->QueueFlags & QUEUE_FLAG_EXCLUSIVE_MODE)
        return FALSE;

    /* Gain exclusive access to the slot queue */
    if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
    {
        /* Check if we have any outstanding commands */
        if (DevExt->AllocatedSlots != 0)
            return FALSE;

        DevExt->QueueFlags |= QUEUE_FLAG_EXCLUSIVE_MODE;
    }

    SlotsBitmap = DevExt->FreeSlotsBitmap;

    if (Request->Flags & REQUEST_FLAG_NCQ)
    {
        /* Check if we have any outstanding non-queued commands */
        if (DevExt->AllocatedSlots > 0)
            return FALSE;

        --DevExt->AllocatedSlots;

        /* The device capacity may be less than the total HBA capacity */
        SlotsBitmap &= DevExt->MaxQueuedSlotsBitmap;
    }
    else
    {
        /* Check if we have any outstanding native queued commands */
        if (DevExt->AllocatedSlots < 0)
            return FALSE;

        ++DevExt->AllocatedSlots;
    }

    /* Mask off previously issued commands */
    SlotMask = ~(0xFFFFFFFF >> (AHCI_MAX_COMMAND_SLOTS - (DevExt->LastUsedSlot + 1)));
    if (!(SlotsBitmap & SlotMask))
        SlotMask = 0xFFFFFFFF;

    /* Allocate slot in a circular fashion */
    NT_VERIFY(_BitScanForward(&SlotNumber, SlotsBitmap & SlotMask) != 0);

    ASSERT(DevExt->LastUsedSlot < AHCI_MAX_COMMAND_SLOTS);
    ASSERT(SlotNumber != DevExt->LastUsedSlot);
    ASSERT(DevExt->FreeSlotsBitmap & (1 << SlotNumber));
    ASSERT(DevExt->Slots[SlotNumber] == NULL);

    DevExt->LastUsedSlot = SlotNumber;
    DevExt->FreeSlotsBitmap &= ~(1 << SlotNumber);
    DevExt->Slots[SlotNumber] = Request;

    Request->Slot = SlotNumber;

    return TRUE;
}

static
VOID
AtaReqDispatchRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG SrbStatus;

    if (!AtaReqAllocateSlot(DevExt, Request))
    {
        ASSERT(!(DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT));

        DevExt->QueueFlags |= QUEUE_FLAG_FROZEN_SLOT;
        DevExt->PendingRequest = Request;
        return;
    }

    if (Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT))
    {
        /* Data is transferred to or from the device with this I/O request */
        SrbStatus = AtaReqPrepareDataTransfer(DevExt, Request);
    }
    else
    {
#if DBG
        Request->SgList = NULL;
#endif

        /* No data transfer */
        AtaAhciExecuteCommand(DevExt, Request);

        SrbStatus = SRB_STATUS_PENDING;
    }

    if (SrbStatus != SRB_STATUS_PENDING)
    {
        Request->SrbStatus = SrbStatus;
        AtaReqStartCompletionDpc(DevExt, Request);
    }
}

static
ULONG
AtaReqTranslateRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    ULONG SrbStatus;

    Request->Srb = Srb;
    Request->DataTransferLength = Srb->DataTransferLength;
    Request->Irp = Srb->OriginalRequest;
    Request->Mdl = Request->Irp->MdlAddress;
    Request->TimeOut = Srb->TimeOutValue;
    Request->DevExt = DevExt;
    Request->State = 0;

    switch (Srb->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            SrbStatus = AtaReqExecuteScsi(DevExt, Request, Srb);
            break;
        }

        case SRB_FUNCTION_IO_CONTROL:
        {
            if (SRB_GET_FLAGS(Srb) & SRB_FLAG_INTERNAL)
                SrbStatus = AtaReqPrepareTaskFile(DevExt, Request, Srb);
            else
            {
                ASSERT(0);
                SrbStatus = SRB_STATUS_ABORTED;
            }
            break;
        }

        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:
        {
            SrbStatus = SRB_STATUS_SUCCESS;
            break;
        }

        default:
        {
            ERR("%u\n", Srb->Function);
            SrbStatus = SRB_STATUS_ABORTED;
        }
    }

    return SrbStatus;
}

static
VOID
AtaReqStartRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    ULONG SrbStatus;

    SrbStatus = AtaReqTranslateRequest(DevExt, Request, Srb);
    if (SrbStatus != SRB_STATUS_PENDING)
    {
        Request->SrbStatus = SrbStatus;
        Request->Flags = 0;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        AtaReqStartCompletionDpc(DevExt, Request);
    }
    else
    {
        AtaReqDispatchRequest(DevExt, Request);
    }
}

static
VOID
AtaReqWaitQueueRemoveEntry(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PREQUEST_QUEUE_ENTRY QueueEntry)
{
    /* Move the head position to service other request */
    if (&QueueEntry->ListEntry == DevExt->NextWaitSrbEntry)
    {
        DevExt->NextWaitSrbEntry = DevExt->NextWaitSrbEntry->Flink;
    }

    if (RemoveEntryList(&QueueEntry->ListEntry))
    {
        DevExt->NextWaitSrbEntry = NULL;
    }
}

static
VOID
NTAPI
AtaReqWaitQueueCancelIo(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ _IRQL_uses_cancel_ PIRP Irp)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    KIRQL OldLevel;
    PSCSI_REQUEST_BLOCK Srb;
    PREQUEST_QUEUE_ENTRY QueueEntry;

    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    QueueEntry = QUEUE_ENTRY_FROM_IRP(Irp);
    DevExt = QueueEntry->Context;

    KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);
    AtaReqWaitQueueRemoveEntry(DevExt, QueueEntry);
    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

    Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
    ASSERT(Srb->OriginalRequest == Irp);

    Srb->SrbStatus = SRB_STATUS_ABORTED;
    Srb->InternalStatus = STATUS_CANCELLED;

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

static
VOID
AtaReqWaitQueueInsertSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PIRP Irp)
{
    PLIST_ENTRY Entry;
    PREQUEST_QUEUE_ENTRY QueueEntry;
    ULONG SortKey;

    SortKey = Srb->QueueSortKey;

    /* Use the C-LOOK hard disk scheduling algorithm to service the PDO requests */
    for (Entry = DevExt->WaitSrbList.Flink;
         Entry != &DevExt->WaitSrbList;
         Entry = Entry->Flink)
    {
        QueueEntry = CONTAINING_RECORD(Entry, REQUEST_QUEUE_ENTRY, ListEntry);

        if (QueueEntry->SortKey > SortKey)
            break;
    }

    QueueEntry = QUEUE_ENTRY_FROM_IRP(Irp);
    QueueEntry->Context = DevExt;
    QueueEntry->SortKey = SortKey;

    InsertTailList(Entry, &QueueEntry->ListEntry);

    if (!DevExt->NextWaitSrbEntry)
        DevExt->NextWaitSrbEntry = &QueueEntry->ListEntry;
}

static
NTSTATUS
AdaReqWaitQueueAddSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PIRP Irp = Srb->OriginalRequest;

    AtaReqWaitQueueInsertSrb(DevExt, Srb, Irp);

    /*
     * If the device queue is full, the requests may take a long period of time to process,
     * and therefore we have to do the cancellation ourselves.
     */
    (VOID)IoSetCancelRoutine(Irp, AtaReqWaitQueueCancelIo);

    /* This IRP has already been cancelled */
    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
    {
        /* Remove the IRP from the queue */
        AtaReqWaitQueueRemoveEntry(DevExt, QUEUE_ENTRY_FROM_IRP(Irp));

        Srb->SrbStatus = SRB_STATUS_ABORTED;
        Srb->InternalStatus = STATUS_CANCELLED;

        return STATUS_CANCELLED;
    }

    return STATUS_PENDING;
}

static
inline
UCHAR
AtaReqGetTagForRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PULONG Tag)
{
    ULONG RequestsBitmap;

    if (DevExt->QueueFlags & QUEUE_FLAGS_FROZEN)
        return FALSE;

    RequestsBitmap = DevExt->FreeRequestsBitmap;

    /*
     * For the NCQ mode we limit the number of requests
     * by the number of supported queued commands
     * to avoid not being able to allocate a slot
     * from the slots bitmap in AtaReqAllocateSlot().
     */
    if (DevExt->AllocatedSlots < 0)
    {
        RequestsBitmap &= DevExt->MaxQueuedSlotsBitmap;
    }

    /* Stack-based request allocation */
    return _BitScanForward(Tag, RequestsBitmap);
}

static
PATA_DEVICE_REQUEST
AtaReqAllocateRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG Tag)
{
    PATA_DEVICE_REQUEST Request;

    ASSERT(DevExt->FreeRequestsBitmap & (1 << Tag));

    DevExt->FreeRequestsBitmap &= ~(1 << Tag);

    Request = &DevExt->Requests[Tag];
    Request->Tag = Tag;

    return Request;
}

static
VOID
AtaReqWaitQueueGetNextRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG Tag;

    if (!DevExt->NextWaitSrbEntry)
        return;

    if (!AtaReqGetTagForRequest(DevExt, &Tag))
        return;

    while (TRUE)
    {
        PREQUEST_QUEUE_ENTRY QueueEntry;
        PIRP Irp;
        PSCSI_REQUEST_BLOCK Srb;
        PATA_DEVICE_REQUEST Request;

        if (!DevExt->NextWaitSrbEntry)
            break;

        QueueEntry = CONTAINING_RECORD(DevExt->NextWaitSrbEntry, REQUEST_QUEUE_ENTRY, ListEntry);

        AtaReqWaitQueueRemoveEntry(DevExt, QueueEntry);

        /* Clear our cancel routine */
        Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
        if (!IoSetCancelRoutine(Irp, NULL))
        {
            /* We're already canceled, reset the list entry to point to itself */
            InitializeListHead(&QueueEntry->ListEntry);
            continue;
        }

        Request = AtaReqAllocateRequest(DevExt, Tag);

        Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
        ASSERT(Srb);
        ASSERT(Srb->OriginalRequest == Irp);

        AtaReqStartRequest(DevExt, Request, Srb);
        break;
    }
}

DECLSPEC_NOINLINE_FROM_PAGED
NTSTATUS
AtaReqStartSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PATA_DEVICE_REQUEST Request;
    ULONG Tag;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);

    DevExt->RequestsStarted++;

    if (!AtaReqGetTagForRequest(DevExt, &Tag))
    {
        Status = AdaReqWaitQueueAddSrb(DevExt, Srb);
    }
    else
    {
        Request = AtaReqAllocateRequest(DevExt, Tag);
        AtaReqStartRequest(DevExt, Request, Srb);

        Status = STATUS_PENDING;
    }

    KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);
    KeLowerIrql(OldIrql);

    return Status;
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFreezeQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags)
{
    KIRQL OldLevel;

    KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);
    DevExt->QueueFlags |= ReasonFlags;
    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqThawQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags)
{
    KIRQL OldLevel;

    KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);

    DevExt->QueueFlags &= ~ReasonFlags;

    AtaReqWaitQueueGetNextRequest(DevExt);

    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFreezeQueueAndWait(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags)
{
    KIRQL OldLevel;
    BOOLEAN DoWait = FALSE;

    PAGED_CODE();

    KeClearEvent(&DevExt->QueueStoppedEvent);

    KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);

    /* Mark the Srb queue as paused */
    DevExt->QueueFlags |= ReasonFlags;

    /* Wait for all the active IRPs to finish executing */
    if (DevExt->FreeRequestsBitmap != DevExt->MaxRequestsBitmap)
    {
        DevExt->QueueFlags |= QUEUE_FLAG_SIGNAL_STOP;
        DoWait = TRUE;
    }

    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

    if (DoWait)
        KeWaitForSingleObject(&DevExt->QueueStoppedEvent, Executive, KernelMode, FALSE, NULL);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqAbortWaitQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    KIRQL OldLevel;

    KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);

    ASSERT(DevExt->QueueFlags & QUEUE_FLAGS_FROZEN);

    while (TRUE)
    {
        PREQUEST_QUEUE_ENTRY QueueEntry;
        PIRP Irp;
        PSCSI_REQUEST_BLOCK Srb;

        if (!DevExt->NextWaitSrbEntry)
            break;

        QueueEntry = CONTAINING_RECORD(DevExt->NextWaitSrbEntry, REQUEST_QUEUE_ENTRY, ListEntry);

        AtaReqWaitQueueRemoveEntry(DevExt, QueueEntry);

        /* Clear our cancel routine */
        Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
        if (!IoSetCancelRoutine(Irp, NULL))
        {
            /* We're already canceled, reset the list entry to point to itself */
            InitializeListHead(&QueueEntry->ListEntry);
            continue;
        }

        KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

        Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
        ASSERT(Srb->OriginalRequest == Irp);

        Srb->SrbStatus = SRB_STATUS_ABORTED;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = Srb->DataTransferLength;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);
    }

    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoAttachReleaseDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;
    ULONG Status, SrbStatus;

    PAGED_CODE();

    ExAcquireFastMutex(&ChanExt->DeviceSyncMutex);

    if (Srb->Function == SRB_FUNCTION_RELEASE_DEVICE)
    {
        DevExt->DeviceClaimed = FALSE;

        SrbStatus = SRB_STATUS_SUCCESS;
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (DevExt->DeviceClaimed)
    {
        SrbStatus = SRB_STATUS_BUSY;
        Status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    DevExt->DeviceClaimed = TRUE;
    Srb->DataBuffer = DevExt->Common.Self;

    SrbStatus = SRB_STATUS_SUCCESS;
    Status = STATUS_SUCCESS;

Exit:
    ExReleaseFastMutex(&ChanExt->DeviceSyncMutex);

    Srb->SrbStatus = SrbStatus;
    return Status;
}

static
NTSTATUS
AtaPdoDispatchScsi(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;

    Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
    ASSERT(Srb);
    ASSERT(Srb->OriginalRequest == Irp);

    Status = IoAcquireRemoveLock(&DevExt->Common.RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        Status = STATUS_NO_SUCH_DEVICE;

        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    switch (Srb->Function)
    {
        case SRB_FUNCTION_CLAIM_DEVICE:
        case SRB_FUNCTION_RELEASE_DEVICE:
        {
            Status = AtaPdoAttachReleaseDevice(DevExt, Srb);
            break;
        }

        case SRB_FUNCTION_LOCK_QUEUE:
        {
            AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_QUEUE_LOCK);

            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
            break;
        }

        case SRB_FUNCTION_UNLOCK_QUEUE:
        case SRB_FUNCTION_RELEASE_QUEUE:
        {
            AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_QUEUE_LOCK);

            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
            break;
        }

        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            ATA_SCSI_ADDRESS AtaScsiAddress;

            IoMarkIrpPending(Irp);

            /* Set the SCSI address to the correct value */
            AtaScsiAddress = DevExt->AtaScsiAddress;
            Srb->PathId = AtaScsiAddress.PathId;
            Srb->TargetId = AtaScsiAddress.TargetId;
            Srb->Lun = AtaScsiAddress.Lun;

            /* This field is used by the driver to mark internal requests */
            Srb->SrbExtension = NULL;

            /*
             * NOTE: Disk I/O requests need a lot of the kernel stack space.
             * We should avoid nesting several levels deep in the call chain.
             */
            Status = AtaReqStartSrb(DevExt, Srb);

            if (Status == STATUS_PENDING)
            {
                IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);
                return Status;
            }
            break;
        }

        default:
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    IoReleaseRemoveLock(&DevExt->Common.RemoveLock, Irp);

    return Status;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaFdoDispatchScsi(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;

    UNREFERENCED_PARAMETER(ChanExt);

    PAGED_CODE();

    /* Drivers should not call the FDO */
    ASSERT(FALSE);

    Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
    ASSERT(Srb);
    ASSERT(Srb->OriginalRequest == Irp);

    Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
    Status = STATUS_NO_SUCH_DEVICE;

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
AtaDispatchScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    if (IS_FDO(DeviceObject->DeviceExtension))
        return AtaFdoDispatchScsi(DeviceObject->DeviceExtension, Irp);
    else
        return AtaPdoDispatchScsi(DeviceObject->DeviceExtension, Irp);
}
