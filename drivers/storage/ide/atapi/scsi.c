/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI requests handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static DRIVER_CANCEL AtaReqWaitQueueCancelIo;
static DRIVER_LIST_CONTROL AtaReqPreparePrdTable;

static
VOID
AtaReqWaitQueueGetNextRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

static
NTSTATUS
AdaReqWaitQueueAddSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

/* FUNCTIONS ******************************************************************/

static
NTSTATUS
AtaSrbStatusToNtStatus(
    _In_ UCHAR SrbStatus)
{
    ASSERT(SRB_STATUS(SrbStatus) != SRB_STATUS_PENDING);

    /* We translate only the values that are actually used by the driver */
    switch (SRB_STATUS(SrbStatus))
    {
        case SRB_STATUS_SUCCESS:
            return STATUS_SUCCESS;

        case SRB_STATUS_TIMEOUT:
            return STATUS_IO_TIMEOUT;

        case SRB_STATUS_NO_DEVICE:
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
NTSTATUS
AtaReqRequeueRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _Out_ PUCHAR ResultSrbStatus)
{
    UCHAR SrbStatus;

    SrbStatus = SRB_STATUS(Request->SrbStatus);

    /* Complete the IRP immediately */
    if (SrbStatus != SRB_STATUS_BUSY && SrbStatus != SRB_STATUS_INSUFFICIENT_RESOURCES)
        return COMPLETE_IRP;

    /* Retry once for memory errors */
    if (SrbStatus == SRB_STATUS_INSUFFICIENT_RESOURCES)
    {
        if (SRB_GET_FLAGS(Request->Srb) & SRB_FLAG_RETRY)
        {
            ASSERT(0);
            return COMPLETE_IRP;
        }

        SRB_SET_FLAGS(Request->Srb, SRB_FLAG_RETRY);
    }

    /* Place the Srb back into the queue */
    if (AdaReqWaitQueueAddSrb(DevExt, Request->Srb) != STATUS_PENDING)
    {
        /* IRP was cancelled, update the status */
        *ResultSrbStatus = Request->Srb->SrbStatus;

        ASSERT(0);
        return COMPLETE_IRP;
    }

    return COMPLETE_NO_IRP;
}

static
VOID
AtaReqReleaseResources(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Request->Flags & REQUEST_FLAG_HAS_SG_LIST)
    {
        PDMA_ADAPTER AdapterObject = DevExt->ChanExt->AdapterObject;
        PDMA_OPERATIONS DmaOperations = AdapterObject->DmaOperations;

        ASSERT(Request->SgList);
        ASSERT(Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));

        DmaOperations->PutScatterGatherList(AdapterObject,
                                            Request->SgList,
                                            !!(Request->Flags & REQUEST_FLAG_DATA_IN));
#if DBG
        Request->SgList = NULL;
#endif
    }

    if (Request->Flags & REQUEST_FLAG_RESERVED_MAPPING)
    {
        MmUnmapReservedMapping(DevExt->ChanExt->ReservedVaSpace, ATAPORT_TAG, Request->Mdl);
        _InterlockedExchange(&DevExt->ChanExt->ReservedMappingLock, 0);
    }

    if (Request->Flags & REQUEST_FLAG_HAS_MDL)
    {
        ASSERT(Request->Mdl);

        IoFreeMdl(Request->Mdl);
#if DBG
        Request->Mdl = NULL;
#endif
    }

    Request->Flags &= ~(REQUEST_FLAG_HAS_SG_LIST |
                        REQUEST_FLAG_HAS_MDL |
                        REQUEST_FLAG_RESERVED_MAPPING);
}

VOID
AtaReqCompleteFailedRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    if (Request->Flags & REQUEST_FLAG_INTERNAL)
    {
        DevExt->ActiveSlotsBitmap = 0;
        DevExt->ActiveQueuedSlotsBitmap = 0;

        AtaReqReleaseResources(DevExt, Request);
        Request->Complete(Request);
    }
    else
    {
        if (!IS_ATAPI(DevExt))
        {

        }

        ASSERT(DevExt->WorkerContext.CurrentRequest == NULL);
        DevExt->WorkerContext.CurrentRequest = Request;

        /* Request arbitration from the device worker */
        AtaDeviceQueueAction(DevExt, ACTION_ERROR);
    }
}

VOID
AtaReqCompleteRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    UCHAR SrbStatus;
    ATA_COMPLETION_ACTION CompletionAction = COMPLETE_IRP;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (Request->InternalState != REQUEST_STATE_NOT_STARTED)
    {
        AtaReqReleaseResources(DevExt, Request);

        if (Request->Complete)
        {
            if (Request->Flags & REQUEST_FLAG_INTERNAL)
            {
                Request->Complete(Request);
                return;
            }
            else
            {
                KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);

                CompletionAction = Request->Complete(Request);

                KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);
            }
        }
    }

    Srb = Request->Srb;

    Irp = Request->Irp;
    Irp->IoStatus.Information = Request->DataTransferLength;

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

    /* Check if we need to retry later */
    if (Request->InternalState == REQUEST_STATE_NOT_STARTED ||
        Request->InternalState == REQUEST_STATE_REQUEUE)
    {
        CompletionAction = AtaReqRequeueRequest(DevExt, Request, &SrbStatus);
    }

    /* Release exclusive access to the slot queue */
    if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_EXCLUSIVE_MODE;
    }

    /* Freeze the Srb queue in case of device error */
    if ((Request->InternalState == REQUEST_STATE_FREEZE_QUEUE) &&
        !(Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        ASSERT(SrbStatus != SRB_STATUS_INSUFFICIENT_RESOURCES);

        /* DevExt->QueueFlags |= QUEUE_FLAG_FROZEN_QUEUE_LOCK; */
        /* SrbStatus |= SRB_STATUS_QUEUE_FROZEN; */
    }
    else
    {
        /*
         * Start the next request on the device queue.
         * It's important to do this before actually completing an IRP.
         */
        AtaReqWaitQueueGetNextRequest(DevExt);
    }

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
    {
        if (SrbStatus == SRB_STATUS_INSUFFICIENT_RESOURCES)
        {
            /* Special status for the upper class driver */
            Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Srb->SrbStatus = SrbStatus;
            Irp->IoStatus.Status = AtaSrbStatusToNtStatus(SrbStatus);
        }

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }
}

UCHAR
AtaReqSetFixedSenseData(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ SCSI_SENSE_CODE SenseCode)
{
    SENSE_DATA SenseData;

    if ((Srb->SenseInfoBuffer == NULL) || (Srb->SenseInfoBufferLength == 0))
        return SenseCode.SrbStatus;

    ASSERT(!(SenseCode.SrbStatus & SRB_STATUS_AUTOSENSE_VALID));

    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

    // TODO (fixed or descriptor format)
    RtlZeroMemory(&SenseData, sizeof(SenseData));
    SenseData.Valid = 1;
    SenseData.ErrorCode = SCSI_SENSE_ERRORCODE_FIXED_CURRENT;
    SenseData.SenseKey = SenseCode.SenseKey;
    SenseData.AdditionalSenseCode = SenseCode.AdditionalSenseCode;
    SenseData.AdditionalSenseCodeQualifier = SenseCode.AdditionalSenseCodeQualifier;
    SenseData.AdditionalSenseLength =
        sizeof(SenseData) - RTL_SIZEOF_THROUGH_FIELD(SENSE_DATA, AdditionalSenseLength);

    RtlCopyMemory(Srb->SenseInfoBuffer,
                  &SenseData,
                  min(Srb->SenseInfoBufferLength, sizeof(SenseData)));

    return SenseCode.SrbStatus | SRB_STATUS_AUTOSENSE_VALID;
}

VOID
AtaReqSetLbaInformation(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG64 Lba)
{
    PSENSE_DATA SenseData;

    if ((Srb->SenseInfoBuffer == NULL) || (Srb->SenseInfoBufferLength == 0))
        return;

    SenseData = Srb->SenseInfoBuffer;

    if (RTL_CONTAINS_FIELD(SenseData, Srb->SenseInfoBufferLength, Information))
    {
        ASSERT(SenseData->Valid);

        // TODO: What to do when updating information field with 64-bit LBA?
        SenseData->Information[0] = Lba << 24;
        SenseData->Information[1] = Lba << 16;
        SenseData->Information[2] = Lba << 8;
        SenseData->Information[4] = Lba << 0;
    }
}

BOOLEAN
AtaReqAllocateMdl(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PMDL Mdl;

    Mdl = IoAllocateMdl(Request->DataBuffer,
                        Request->DataTransferLength,
                        FALSE,
                        FALSE,
                        NULL);
    if (!Mdl)
        return FALSE;

    MmBuildMdlForNonPagedPool(Mdl);

    Request->Mdl = Mdl;
    Request->Flags |= REQUEST_FLAG_HAS_MDL;

    return TRUE;
}

VOID
AtaReqStartIo(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeAcquireSpinLockAtDpcLevel(&DevExt->DeviceLock);

    if ((DevExt->PortFlags & PORT_ACTIVE) || (Request->Flags & REQUEST_FLAG_INTERNAL))
    {
        UCHAR SrbStatus = DevExt->StartIo(DevExt, Request);

        if (SrbStatus != SRB_STATUS_PENDING)
        {
            ASSERT(SrbStatus != SRB_STATUS_SUCCESS);

            Request->SrbStatus = SrbStatus;
            AtaReqCompleteFailedRequest(DevExt, Request);
        }

        KeReleaseSpinLockFromDpcLevel(&DevExt->DeviceLock);
    }
    else
    {
        KeReleaseSpinLockFromDpcLevel(&DevExt->DeviceLock);

        /* If the queue is frozen, then force queue manager to requeue the SRB */
        Request->SrbStatus = SRB_STATUS_BUSY;
        Request->InternalState = REQUEST_STATE_REQUEUE;

        /* Defer the completion to a DPC to avoid the recursive call in some cases */
        AtaReqStartCompletionDpc(DevExt, Request);
    }
}

static
VOID
NTAPI
AtaReqPreparePrdTable(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PSCATTER_GATHER_LIST SgList,
    _In_ PVOID Context)
{
    PATA_DEVICE_REQUEST Request = Context;
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    Request->SgList = SgList;

    DevExt->PreparePrdTable(DevExt, Request, SgList);

    AtaReqStartIo(DevExt, Request);
}

static
PVOID
AtaReqMapBuffer(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    PVOID BaseAddress;
    ULONG PagesNeeded;

    BaseAddress = MmGetSystemAddressForMdlSafe(Request->Mdl, HighPagePriority);
    if (BaseAddress)
        return BaseAddress;

    ChanExt = DevExt->ChanExt;
    if (!ChanExt->ReservedVaSpace)
        return NULL;

    /* The system is low resources, handle it in a non-fatal way */
    PagesNeeded = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Request->Mdl),
                                                 MmGetMdlByteCount(Request->Mdl));
    if (PagesNeeded > ATA_RESERVED_PAGES)
        return NULL;

    /* Utilize the reserved mapping to overcome memory issues */
    if (!_InterlockedCompareExchange(&ChanExt->ReservedMappingLock, 1, 0))
    {
        BaseAddress = MmMapLockedPagesWithReservedMapping(ChanExt->ReservedVaSpace,
                                                          ATAPORT_TAG,
                                                          Request->Mdl,
                                                          MmCached);
        if (BaseAddress)
            Request->Flags |= REQUEST_FLAG_RESERVED_MAPPING;
        else
            _InterlockedExchange(&ChanExt->ReservedMappingLock, 0);
    }

    return BaseAddress;
}

static
BOOLEAN
AtaReqGetScatterGatherList(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PDRIVER_LIST_CONTROL ExecutionRoutine)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = DevExt->ChanExt;
    PDMA_ADAPTER AdapterObject = ChanExt->AdapterObject;
    PDMA_OPERATIONS DmaOperations = AdapterObject->DmaOperations;
    NTSTATUS Status;

    Status = DmaOperations->GetScatterGatherList(AdapterObject,
                                                 ChanExt->AdapterDeviceObject,
                                                 Request->Mdl,
                                                 Request->DataBuffer,
                                                 Request->DataTransferLength,
                                                 ExecutionRoutine,
                                                 Request,
                                                 !!(Request->Flags & REQUEST_FLAG_DATA_IN));
    if (NT_SUCCESS(Status))
    {
        Request->Flags |= REQUEST_FLAG_HAS_SG_LIST;
        return TRUE;
    }

    WARN("Failed to get the S/G list with status %lx\n", Status);
    return FALSE;
}

VOID
AtaReqSendRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PVOID BaseAddress;
    ULONG_PTR Offset;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* The HBA can only perform DMA I/O and PIO is not supported */
    if (IS_AHCI(DevExt))
    {
        Request->Flags |= REQUEST_FLAG_PROGRAM_DMA;
    }

    if (Request->Flags & REQUEST_FLAG_DMA)
    {
        ASSERT(Request->Flags & REQUEST_FLAG_PROGRAM_DMA);
    }

    /* Local buffer transfer */
    if (Request->Flags & REQUEST_FLAG_HAS_LOCAL_BUFFER)
    {
        /*
         * The local buffer is not supported on PCI IDE DMA transfers,
         * because the physical address of the buffer is NULL.
         * Getting the actual address is difficult,
         * because this would require the driver to allocate a common buffer.
         */
        ASSERT(IS_AHCI(DevExt) || !(Request->Flags & REQUEST_FLAG_DMA));

        if (Request->Flags & REQUEST_FLAG_PROGRAM_DMA)
        {
            /* DMA transfer */
            AtaReqPreparePrdTable(NULL, NULL, &DevExt->PortData->LocalSgList, Request);
        }
        else
        {
            /* PIO data transfer */
            Request->DataBuffer = DevExt->LocalBuffer;

            AtaReqStartIo(DevExt, Request);
        }
        return;
    }

    /* No data transfer */
    if (!(Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT)))
    {
        AtaReqStartIo(DevExt, Request);
        return;
    }

    /* DMA transfer, get the S/G list for the MDL */
    if (Request->Flags & REQUEST_FLAG_PROGRAM_DMA)
    {
        if (AtaReqGetScatterGatherList(DevExt, Request, AtaReqPreparePrdTable))
            return;

        /* The HBA can only perform DMA I/O and PIO is not supported */
        if (IS_AHCI(DevExt))
            goto CompleteNoMemory;

        /* DMA failed, attempt to fall back to PIO mode */
        if (!AtaPataPreparePioDataTransfer(DevExt, Request))
            goto CompleteNoMemory;
    }

    ASSERT(!IS_AHCI(DevExt));

    /* PIO data transfer */
    BaseAddress = AtaReqMapBuffer(DevExt, Request);
    if (!BaseAddress)
        goto CompleteNoMemory;

    /* Calculate the offset within DataBuffer */
    Offset = (ULONG_PTR)BaseAddress +
             (ULONG_PTR)Request->DataBuffer -
             (ULONG_PTR)MmGetMdlVirtualAddress(Request->Mdl);
    Request->DataBuffer = (PVOID)Offset;

    AtaReqStartIo(DevExt, Request);
    return;

CompleteNoMemory:
    Request->SrbStatus = SRB_STATUS_INSUFFICIENT_RESOURCES;
    Request->InternalState = REQUEST_STATE_REQUEUE;

    /*
     * Defer the completion to a DPC.
     * We cannot complete request with the queue spinlock held.
     */
    AtaReqStartCompletionDpc(DevExt, Request);
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

    while (TRUE)
    {
        PATA_DEVICE_REQUEST Request;
        PIRP Irp;
        PLIST_ENTRY Entry;

        KeAcquireSpinLockAtDpcLevel(&DevExt->CompletionLock);
        if (IsListEmpty(&DevExt->CompletionList))
            Entry = NULL;
        else
            Entry = RemoveHeadList(&DevExt->CompletionList);
        KeReleaseSpinLockFromDpcLevel(&DevExt->CompletionLock);

        if (!Entry)
            break;

        Irp = IRP_FROM_LIST_ENTRY(Entry);
        Request = Irp->Tail.Overlay.DriverContext[3];
        AtaReqCompleteRequest(DevExt, Request);
    }
}

VOID
AtaReqStartCompletionDpc(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeAcquireSpinLockAtDpcLevel(&DevExt->CompletionLock);

    Request->Irp->Tail.Overlay.DriverContext[3] = Request;
    InsertTailList(&DevExt->CompletionList, LIST_ENTRY_FROM_IRP(Request->Irp));

    KeReleaseSpinLockFromDpcLevel(&DevExt->CompletionLock);

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

    /*
     * Allocate slot in a circular fashion.
     * This is required to maintain CCS update.
     */
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
    if (AtaReqAllocateSlot(DevExt, Request))
    {
        DevExt->SendRequest(DevExt, Request);
    }
    else
    {
        ASSERT(!(DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT));

        DevExt->QueueFlags |= QUEUE_FLAG_FROZEN_SLOT;
        DevExt->PendingRequest = Request;
    }
}

static
ULONG
AtaReqTranslateRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR SrbStatus;

    Request->DevExt = DevExt;
    Request->Srb = Srb;
    Request->DataBuffer = Srb->DataBuffer;
    Request->DataTransferLength = Srb->DataTransferLength;
    Request->Irp = Srb->OriginalRequest;
    Request->Mdl = ((PIRP)(Srb->OriginalRequest))->MdlAddress;
    Request->TimeOut = Srb->TimeOutValue;
    Request->Complete = NULL;
    Request->Flags = 0;
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
            SrbStatus = AtaReqSmartIoControl(DevExt, Request, Srb);
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

#if DBG
    if (!(Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT)))
    {
        Request->SgList = NULL;
    }
#endif

    return SrbStatus;
}

static
VOID
AtaReqStartRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR SrbStatus;

    SrbStatus = AtaReqTranslateRequest(DevExt, Request, Srb);
    if (SrbStatus != SRB_STATUS_PENDING)
    {
        Request->SrbStatus = SrbStatus;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        /*
         * Defer the completion to a DPC.
         * We cannot complete request with the queue spinlock held.
         */
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
    /* C-LOOK: Move the head position to service other request */
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

        ASSERT(QueueEntry->Context == DevExt);

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
     * If the device queue is full or frozen,
     * the requests may take a long period of time to process,
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

    /* Try to unfreeze the slot queue */
    if (DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT)
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;
        AtaReqDispatchRequest(DevExt, DevExt->PendingRequest);
    }

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
        ASSERT(QueueEntry->Context == DevExt);

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

#if DBG
    DevExt->RequestsStarted++;
#endif

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
BOOLEAN
AtaReqWaitForOutstandingIoToComplete(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG TimeOut)
{
    KIRQL OldLevel;
    LARGE_INTEGER DueTime;
    PLARGE_INTEGER DueTimePtr;
    NTSTATUS Status;
    BOOLEAN DoWait;
    PATA_DEVICE_REQUEST Request;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);

    ASSERT(DevExt->QueueFlags & QUEUE_FLAGS_FROZEN);

    KeClearEvent(&DevExt->QueueStoppedEvent);

    /* Wait for all the active IRPs to finish executing */
    if (DevExt->FreeRequestsBitmap != DevExt->MaxRequestsBitmap)
    {
        DevExt->QueueFlags |= QUEUE_FLAG_SIGNAL_STOP;
        DoWait = TRUE;
    }
    else
    {
        KeSetEvent(&DevExt->QueueStoppedEvent, 0, FALSE);

        DoWait = FALSE;
    }

    if (DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT)
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;
        Request = DevExt->PendingRequest;
    }
    else
    {
        Request = NULL;
    }

    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

    if (!DoWait)
        return TRUE;

    /* Requeue the pending request */
    if (Request)
    {
        Request->SrbStatus = SRB_STATUS_BUSY;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        AtaReqCompleteRequest(DevExt, Request);
    }

    /* Set timeout (in seconds) */
    if (TimeOut != 0)
    {
        DueTime.QuadPart = Int32x32To64(TimeOut, -10000000);
        DueTimePtr = &DueTime;
    }
    else
    {
        DueTimePtr = NULL;
    }

    Status = KeWaitForSingleObject(&DevExt->QueueStoppedEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   DueTimePtr);
    if ((TimeOut != 0) && (Status == STATUS_TIMEOUT))
        return FALSE;

    return TRUE;
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFlushWaitQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATA_DEVICE_REQUEST Request;
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
        ASSERT(QueueEntry->Context == DevExt);

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

    /* The pending request belongs to the device queue and we should remove it */
    if (DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT)
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;
        Request = DevExt->PendingRequest;
    }
    else
    {
        Request = NULL;
    }

    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

    if (Request)
    {
        Request->SrbStatus = SRB_STATUS_ABORTED;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        AtaReqCompleteRequest(DevExt, Request);
    }
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleAttachReleaseDevice(
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
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleQuiesceDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PAGED_CODE();

    ASSERT(DevExt->QueueFlags & QUEUE_FLAG_FROZEN_QUEUE_LOCK);

    /* Wait for outstanding I/O requests to finish */
    if (!AtaReqWaitForOutstandingIoToComplete(DevExt, Srb->TimeOutValue))
    {
        ERR("Quiescence request timed out\n");

        Srb->SrbStatus = SRB_STATUS_TIMEOUT;
        return STATUS_IO_TIMEOUT;
    }

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

static
DECLSPEC_NOINLINE_FROM_NOT_PAGED
CODE_SEG("PAGE")
BOOLEAN
AtaPdoHandleIoControl(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ NTSTATUS* Status)
{
    PSRB_IO_CONTROL SrbControl = (PSRB_IO_CONTROL)Srb->DataBuffer;

    PAGED_CODE();

    switch (SrbControl->ControlCode)
    {
        case IOCTL_SCSI_MINIPORT_SMART_VERSION:
            *Status = AtaPdoHandleMiniportSmartVersion(DevExt, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_IDENTIFY:
            *Status = AtaPdoHandleMiniportIdentify(DevExt, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_DISABLE_SMART:
        case IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE:
        case IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE:
        case IOCTL_SCSI_MINIPORT_ENABLE_SMART:
        case IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS:
        case IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS:
        case IOCTL_SCSI_MINIPORT_READ_SMART_LOG:
        case IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS:
        case IOCTL_SCSI_MINIPORT_RETURN_STATUS:
        case IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES:
        case IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG:
        {
            /* Queue the request */
            return FALSE;
        }

        default:
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            *Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    return TRUE;
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
            Status = AtaPdoHandleAttachReleaseDevice(DevExt, Srb);
            break;
        }

        case SRB_FUNCTION_LOCK_QUEUE:
        {
            AtaReqFreezeQueue(DevExt, QUEUE_FLAG_FROZEN_QUEUE_LOCK);

            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
            break;
        }

        case SRB_FUNCTION_FLUSH_QUEUE:
        {
            AtaReqFlushWaitQueue(DevExt);
            __fallthrough;
        }
        case SRB_FUNCTION_UNLOCK_QUEUE:
        case SRB_FUNCTION_RELEASE_QUEUE:
        {
            AtaReqThawQueue(DevExt, QUEUE_FLAG_FROZEN_QUEUE_LOCK);

            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
            break;
        }

        case SRB_FUNCTION_QUIESCE_DEVICE:
        {
            Status = AtaPdoHandleQuiesceDevice(DevExt, Srb);
            break;
        }

        case SRB_FUNCTION_IO_CONTROL:
        {
            if (AtaPdoHandleIoControl(DevExt, Srb, &Status))
                break;

            __fallthrough;
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
