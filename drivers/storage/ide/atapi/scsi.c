/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI requests handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static DRIVER_CANCEL AtaReqDeviceQueueCancelIo;
static DRIVER_LIST_CONTROL AtaReqPreparePrdTable;
static DRIVER_CONTROL AtaReqStartIoSerialized;

static
NTSTATUS
AtaReqDeviceQueueAddSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

static
VOID
AtaReqDispatchRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN ReleaseDeviceQueueLock);

static
BOOLEAN
AtaReqDeviceQueueDispatchNextRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

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

    /* Only requeue busy requests */
    if ((SrbStatus != SRB_STATUS_BUSY) && (SrbStatus != SRB_STATUS_INSUFFICIENT_RESOURCES))
        return COMPLETE_IRP;

    /* Retry at least once if the request was rejected due to lack of resources */
    if (SrbStatus == SRB_STATUS_INSUFFICIENT_RESOURCES)
    {
        /* We have no chance to dispatch it */
        if (SRB_GET_FLAGS(Request->Srb) & SRB_FLAG_RETRY)
        {
            ASSERT(0);
            return COMPLETE_IRP;
        }

        SRB_SET_FLAGS(Request->Srb, SRB_FLAG_RETRY);
    }

    ASSERT(!(Request->Flags & (REQUEST_FLAG_HAS_SG_LIST |
                               REQUEST_FLAG_HAS_MDL |
                               REQUEST_FLAG_HAS_RESERVED_MAPPING)));

    /* Place the Srb back into the queue */
    if (AtaReqDeviceQueueAddSrb(DevExt, Request->Srb) != STATUS_PENDING)
    {
        /* IRP was cancelled, update the status */
        *ResultSrbStatus = Request->Srb->SrbStatus;

        return COMPLETE_IRP;
    }

    return COMPLETE_NO_IRP;
}

static
BOOLEAN
AtaReqPortQueueListDispatchNextRequest(
    _In_ PATAPORT_PORT_DATA PortData)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    PATA_DEVICE_REQUEST Request;
    PLIST_ENTRY Entry;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (IsListEmpty(&PortData->PortQueueList))
        return FALSE;

    Entry = RemoveHeadList(&PortData->PortQueueList);

    Request = CONTAINING_RECORD(Entry, ATA_DEVICE_REQUEST, PortEntry);
    ASSERT(Request);
    ASSERT(Request->Signature == ATA_DEVICE_REQUEST_SIGNATURE);

    DevExt = Request->DevExt;

    /* Try to unfreeze the device queue */
    DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;

    AtaReqDispatchRequest(DevExt, Request, FALSE);
    return TRUE;
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
    }

    if (Request->Flags & REQUEST_FLAG_HAS_RESERVED_MAPPING)
    {
        MmUnmapReservedMapping(DevExt->ChanExt->ReservedVaSpace, ATAPORT_TAG, Request->Mdl);
        _InterlockedExchange(&DevExt->ChanExt->ReservedMappingLock, 0);
    }

    if (Request->Flags & REQUEST_FLAG_HAS_MDL)
    {
        ASSERT(Request->Mdl);

        IoFreeMdl(Request->Mdl);
    }

#if DBG
    Request->Mdl = NULL;
    Request->SgList = NULL;
#endif

    Request->Flags &= ~(REQUEST_FLAG_HAS_SG_LIST |
                        REQUEST_FLAG_HAS_MDL |
                        REQUEST_FLAG_HAS_RESERVED_MAPPING);
}

VOID
AtaReqCompleteFailedRequest(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;

    if (Request->Flags & REQUEST_FLAG_INTERNAL)
    {
        DevExt->PortData->ActiveSlotsBitmap = 0;
        DevExt->PortData->ActiveQueuedSlotsBitmap = 0;

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
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    UCHAR SrbStatus;
    ATA_COMPLETION_ACTION CompletionAction = COMPLETE_IRP;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    AtaReqReleaseResources(DevExt, Request);

    if (Request->InternalState != REQUEST_STATE_NOT_STARTED)
    {
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

    /* Release the request back to the device queue */
    ASSERT(!(DevExt->FreeRequestsBitmap & (1 << Request->Tag)));
    DevExt->FreeRequestsBitmap |= (1 << Request->Tag);

    /* Release the slot back to the port queue */
    if (Request->InternalState != REQUEST_STATE_NOT_STARTED)
    {
        PATAPORT_PORT_DATA PortData = DevExt->PortData;

        KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);

        /* Release exclusive access to the port queue */
        if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
        {
            PortData->QueueFlags &= ~PORT_QUEUE_FLAG_EXCLUSIVE_MODE;
        }

        if (Request->Flags & REQUEST_FLAG_NCQ)
            ++PortData->AllocatedSlots;
        else
            --PortData->AllocatedSlots;

        ASSERT(!(PortData->FreeSlotsBitmap & (1 << Request->Slot)));
        PortData->FreeSlotsBitmap |= (1 << Request->Slot);
#if DBG
        PortData->Slots[Request->Slot] = NULL;
#endif

        /* Start the next request on the port queue */
        if (!AtaReqPortQueueListDispatchNextRequest(PortData))
            KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);
    }

    /* Check if we need to retry later */
    if (Request->InternalState == REQUEST_STATE_NOT_STARTED ||
        Request->InternalState == REQUEST_STATE_REQUEUE)
    {
        CompletionAction = AtaReqRequeueRequest(DevExt, Request, &SrbStatus);
    }

#if DBG
    if (CompletionAction == COMPLETE_IRP)
        ++DevExt->RequestsCompleted;
#endif

    /* Freeze the Srb queue in case of device error */
    if ((Request->InternalState == REQUEST_STATE_FREEZE_QUEUE) &&
        !(Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        ASSERT(SrbStatus != SRB_STATUS_INSUFFICIENT_RESOURCES);

        /* DevExt->QueueFlags |= QUEUE_FLAG_FROZEN_QUEUE_FREEZE; */
        /* SrbStatus |= SRB_STATUS_QUEUE_FROZEN; */
    }

    /* Signal the event when the list of active IRPs is empty */
    if ((DevExt->QueueFlags & QUEUE_FLAG_SIGNAL_STOP) &&
        (DevExt->FreeRequestsBitmap == DevExt->MaxRequestsBitmap))
    {
        DevExt->QueueFlags &= ~QUEUE_FLAG_SIGNAL_STOP;

        KeSetEvent(&DevExt->QueueStoppedEvent, 0, FALSE);
    }

    ASSERT(!(Request->Flags & (REQUEST_FLAG_HAS_SG_LIST |
                               REQUEST_FLAG_HAS_MDL |
                               REQUEST_FLAG_HAS_RESERVED_MAPPING)));

    /*
     * Start the next request on the device queue.
     * It's important to do this before actually completing an IRP.
     */
    if (!AtaReqDeviceQueueDispatchNextRequest(DevExt))
        KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

    /*
     * Complete the IRP outside of the spinlock to avoid deadlocks.
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
        SenseData->Information[0] = (UCHAR)(Lba << 24);
        SenseData->Information[1] = (UCHAR)(Lba << 16);
        SenseData->Information[2] = (UCHAR)(Lba << 8);
        SenseData->Information[3] = (UCHAR)(Lba << 0);
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
    PATAPORT_PORT_DATA PortData = DevExt->PortData;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    KeAcquireSpinLockAtDpcLevel(&PortData->PortLock);

    if ((PortData->PortFlags & PORT_FLAG_ACTIVE) || (Request->Flags & REQUEST_FLAG_INTERNAL))
    {
        UCHAR SrbStatus = DevExt->StartIo(PortData, Request);

        if (SrbStatus != SRB_STATUS_PENDING)
        {
            ASSERT(SrbStatus != SRB_STATUS_SUCCESS);

            Request->SrbStatus = SrbStatus;
            AtaReqCompleteFailedRequest(Request);
        }

        KeReleaseSpinLockFromDpcLevel(&PortData->PortLock);
    }
    else
    {
        KeReleaseSpinLockFromDpcLevel(&PortData->PortLock);

        /* If the queue is frozen, then force queue manager to requeue the SRB */
        Request->SrbStatus = SRB_STATUS_BUSY;
        Request->InternalState = REQUEST_STATE_REQUEUE;

        /* Defer the completion to a DPC to avoid the recursive call in some cases */
        AtaReqStartCompletionDpc(Request);
    }
}

static
IO_ALLOCATION_ACTION
NTAPI
AtaReqStartIoSerialized(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID Context)
{
    PATA_DEVICE_REQUEST Request = Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(MapRegisterBase);

    AtaReqStartIo(Request->DevExt, Request);

    return KeepObject;
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

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    Request->SgList = SgList;

    DevExt->PreparePrdTable(DevExt, Request, SgList);

    if (DevExt->Flags & DEVICE_SIMPLEX_DMA)
    {
        PATAPORT_PORT_DATA PortData = DevExt->PortData;

        IoAllocateController(PortData->Pata.PciIdeInterface.ControllerObject,
                             PortData->Pata.PciIdeInterface.DeviceObject,
                             AtaReqStartIoSerialized,
                             Request);
    }
    else
    {
        AtaReqStartIo(DevExt, Request);
    }
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
            Request->Flags |= REQUEST_FLAG_HAS_RESERVED_MAPPING;
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

    ASSERT(Request->Mdl);

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
         * because this would require the PCIIDEX driver to allocate a common buffer.
         */
        ASSERT(IS_AHCI(DevExt) || !(Request->Flags & REQUEST_FLAG_DMA));

        ASSERT(Request->Flags & REQUEST_FLAG_INTERNAL);

        if (Request->Flags & REQUEST_FLAG_PROGRAM_DMA)
        {
            /* DMA transfer */
            AtaReqPreparePrdTable(NULL, NULL, &DevExt->PortData->Ahci.LocalSgList, Request);
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

        /* S/G list construction failed, attempt to fall back to PIO mode */
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
    AtaReqStartCompletionDpc(Request);
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
    PSLIST_ENTRY FirstEntry, NextEntry;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    FirstEntry = ExInterlockedFlushSList(&DevExt->CompletionQueueList);

    for (NextEntry = FirstEntry; NextEntry != NULL; NextEntry = NextEntry->Next)
    {
        PATA_DEVICE_REQUEST Request;

        Request = CONTAINING_RECORD(NextEntry, ATA_DEVICE_REQUEST, CompletionEntry);
        ASSERT(Request);
        ASSERT(Request->Signature == ATA_DEVICE_REQUEST_SIGNATURE);
        ASSERT(Request->DevExt == DevExt);

        AtaReqCompleteRequest(Request);
    }
}

VOID
AtaReqStartCompletionDpc(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_DEVICE_EXTENSION DevExt = Request->DevExt;

    InterlockedPushEntrySList(&DevExt->CompletionQueueList, &Request->CompletionEntry);
    KeInsertQueueDpc(&DevExt->CompletionDpc, NULL, NULL);
}

static
BOOLEAN
AtaReqAllocateSlot(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ULONG SlotMask, SlotNumber, SlotsBitmap;

    if (PortData->QueueFlags & PORT_QUEUE_FLAG_EXCLUSIVE_MODE)
        return FALSE;

    /* Gain exclusive access to the slot queue */
    if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
    {
        /* Check if we have any outstanding commands */
        if (PortData->AllocatedSlots != 0)
            return FALSE;
    }

    SlotsBitmap = PortData->FreeSlotsBitmap;

    if (Request->Flags & REQUEST_FLAG_NCQ)
    {
        /* Check if we have any outstanding non-queued commands */
        if (PortData->AllocatedSlots > 0)
            return FALSE;

        /* The device capacity may be less than the total HBA capacity */
        SlotsBitmap &= DevExt->MaxQueuedSlotsBitmap;
    }
    else
    {
        /* Check if we have any outstanding native queued commands */
        if (PortData->AllocatedSlots < 0)
            return FALSE;
    }

    /* Mask off previously issued commands */
    SlotMask = ~(0xFFFFFFFF >> (AHCI_MAX_COMMAND_SLOTS - (PortData->LastUsedSlot + 1)));
    if (!(SlotsBitmap & SlotMask))
        SlotMask = 0xFFFFFFFF;

    /* Allocate slot in a circular fashion. This is required to maintain CCS update */
    if (!_BitScanForward(&SlotNumber, SlotsBitmap & SlotMask))
        return FALSE;

    /* The slot can safely be consumed at this point */
    if (Request->Flags & REQUEST_FLAG_NCQ)
        --PortData->AllocatedSlots;
    else
        ++PortData->AllocatedSlots;

    if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
        PortData->QueueFlags |= PORT_QUEUE_FLAG_EXCLUSIVE_MODE;

    ASSERT(PortData->LastUsedSlot < AHCI_MAX_COMMAND_SLOTS);
    ASSERT(IsPowerOfTwo(SlotsBitmap) || (SlotNumber != PortData->LastUsedSlot));
    ASSERT(PortData->FreeSlotsBitmap & (1 << SlotNumber));
    ASSERT(PortData->Slots[SlotNumber] == NULL);

    PortData->LastUsedSlot = SlotNumber;
    PortData->FreeSlotsBitmap &= ~(1 << SlotNumber);
    PortData->Slots[SlotNumber] = Request;

    Request->Slot = SlotNumber;

    return TRUE;
}

static
VOID
AtaReqDispatchRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN ReleaseDeviceQueueLock)
{
    PATAPORT_PORT_DATA PortData = DevExt->PortData;
    BOOLEAN Success;

    Success = AtaReqAllocateSlot(DevExt, PortData, Request);
    if (!Success)
    {
        /*
         * If all slots are busy, overloading the port can starve other incoming I/O requests:
         * - An IDE channel can only deal with one active request at a time.
         * - A SATA Port Multiplier shares the bandwidth with up to 15 devices.
         * In order to avoid starvation of the PDO,
         * put the request on the high-priority port queue.
         */
        InsertTailList(&PortData->PortQueueList, &Request->PortEntry);

        /*
         * Freeze the device queue to ensure
         * that only one request per device is queued to the port queue.
         */
        ASSERT(!(DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT));
        DevExt->QueueFlags |= QUEUE_FLAG_FROZEN_SLOT;
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    if (ReleaseDeviceQueueLock)
        KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

    if (Success)
    {
        DevExt->SendRequest(DevExt, Request);
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
            ASSERT(FALSE);
            UNREACHABLE;
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
    UCHAR SrbStatus;

    SrbStatus = AtaReqTranslateRequest(DevExt, Request, Srb);

    if (SrbStatus != SRB_STATUS_PENDING)
    {
        KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

        Request->SrbStatus = SrbStatus;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        /* Defer the completion to a DPC to avoid the recursive call in some cases */
        AtaReqStartCompletionDpc(Request);
    }
    else
    {
        KeAcquireSpinLockAtDpcLevel(&DevExt->PortData->QueueLock);

        AtaReqDispatchRequest(DevExt, Request, TRUE);
    }
}

static
PATA_DEVICE_REQUEST
AtaReqRemovePortRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    PATAPORT_PORT_DATA PortData = DevExt->PortData;
    PLIST_ENTRY Entry;
    PATA_DEVICE_REQUEST Request, Result = NULL;

    if (!(DevExt->QueueFlags & QUEUE_FLAG_FROZEN_SLOT))
        return NULL;

    DevExt->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;

    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);

    for (Entry = PortData->PortQueueList.Flink;
         Entry != &PortData->PortQueueList;
         Entry = Entry->Flink)
    {
        Request = CONTAINING_RECORD(Entry, ATA_DEVICE_REQUEST, PortEntry);

        ASSERT(Request);
        ASSERT(Request->Signature == ATA_DEVICE_REQUEST_SIGNATURE);

        if (Request->DevExt != DevExt)
            continue;

        RemoveEntryList(&Request->PortEntry);

        Result = Request;
        break;
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    ASSERT(Result != NULL);

    return Result;
}

static
VOID
AtaReqDeviceQueueRemoveEntry(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PREQUEST_QUEUE_ENTRY QueueEntry)
{
    /* C-LOOK: Move the head position to service other request */
    if (&QueueEntry->ListEntry == DevExt->NextDeviceQueueEntry)
    {
        DevExt->NextDeviceQueueEntry = DevExt->NextDeviceQueueEntry->Flink;
    }

    if (RemoveEntryList(&QueueEntry->ListEntry))
    {
        DevExt->NextDeviceQueueEntry = NULL;
    }
}

static
VOID
NTAPI
AtaReqDeviceQueueCancelIo(
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
    AtaReqDeviceQueueRemoveEntry(DevExt, QueueEntry);
    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

    Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
    ASSERT(Srb);
    ASSERT(Srb->OriginalRequest == Irp);

    Srb->SrbStatus = SRB_STATUS_ABORTED;
    Srb->InternalStatus = STATUS_CANCELLED;

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

static
VOID
AtaReqDeviceQueueInsertSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PIRP Irp)
{
    PLIST_ENTRY Entry;
    PREQUEST_QUEUE_ENTRY QueueEntry;
    ULONG SortKey;

    SortKey = Srb->QueueSortKey;

    /* Use the C-LOOK hard disk scheduling algorithm to service the PDO requests */
    for (Entry = DevExt->DeviceQueueList.Flink;
         Entry != &DevExt->DeviceQueueList;
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

    if (!DevExt->NextDeviceQueueEntry)
        DevExt->NextDeviceQueueEntry = &QueueEntry->ListEntry;
}

static
NTSTATUS
AtaReqDeviceQueueAddSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PIRP Irp = Srb->OriginalRequest;

    AtaReqDeviceQueueInsertSrb(DevExt, Srb, Irp);

    /*
     * If the device queue is full or frozen,
     * the requests may take a long period of time to process,
     * and therefore we have to do the cancellation ourselves.
     */
    (VOID)IoSetCancelRoutine(Irp, AtaReqDeviceQueueCancelIo);

    /* This IRP has already been cancelled */
    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
    {
        /* Remove the IRP from the queue */
        AtaReqDeviceQueueRemoveEntry(DevExt, QUEUE_ENTRY_FROM_IRP(Irp));

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
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PULONG Tag)
{
    ULONG RequestsBitmap;

#if 1
    if (DevExt->QueueFlags & QUEUE_FLAGS_FROZEN)
        return FALSE;
#else
    /* Check if the queue is actually frozen and the request is not a bypass */
    if ((DevExt->QueueFlags & QUEUE_FLAGS_FROZEN) &&
        !((DevExt->QueueFlags & QUEUE_FLAGS_BYPASS_FROZEN) ==
        (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)) &&
        !((DevExt->QueueFlags & QUEUE_FLAGS_BYPASS_LOCKED) ==
          (Srb->SrbFlags & SRB_FLAGS_BYPASS_LOCKED_QUEUE)))
    {
        return FALSE;
    }
#endif

    RequestsBitmap = DevExt->FreeRequestsBitmap;

    /*
     * Stack-based request allocation.
     * It will return us the last request structure in the CPU cacheline.
     */
    return _BitScanForward(Tag, RequestsBitmap);
}

static
inline
PATA_DEVICE_REQUEST
AtaReqAllocateRequestFromTag(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG Tag)
{
    PATA_DEVICE_REQUEST Request;

    ASSERT(DevExt->FreeRequestsBitmap & (1 << Tag));

    DevExt->FreeRequestsBitmap &= ~(1 << Tag);

    Request = &DevExt->Requests[Tag];
    ASSERT(Request);
    ASSERT(Request->Signature == ATA_DEVICE_REQUEST_SIGNATURE);

    Request->Tag = Tag;

    return Request;
}

static
BOOLEAN
AtaReqDeviceQueueDispatchNextRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt)
{
    ULONG Tag;

    // TODO: Handle bypass

    if (!DevExt->NextDeviceQueueEntry)
        return FALSE;

    if (!AtaReqGetTagForRequest(DevExt, NULL, &Tag))
        return FALSE;

    while (TRUE)
    {
        PREQUEST_QUEUE_ENTRY QueueEntry;
        PIRP Irp;
        PSCSI_REQUEST_BLOCK Srb;
        PATA_DEVICE_REQUEST Request;

        if (!DevExt->NextDeviceQueueEntry)
            break;

        QueueEntry = CONTAINING_RECORD(DevExt->NextDeviceQueueEntry,
                                       REQUEST_QUEUE_ENTRY,
                                       ListEntry);
        ASSERT(QueueEntry->Context == DevExt);

        AtaReqDeviceQueueRemoveEntry(DevExt, QueueEntry);

        /* Clear our cancel routine */
        Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
        if (!IoSetCancelRoutine(Irp, NULL))
        {
            /* We're already canceled, reset the list entry to point to itself */
            InitializeListHead(&QueueEntry->ListEntry);
            continue;
        }

        Request = AtaReqAllocateRequestFromTag(DevExt, Tag);

        Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
        ASSERT(Srb);
        ASSERT(Srb->OriginalRequest == Irp);

        AtaReqStartRequest(DevExt, Request, Srb);
        return TRUE;
    }

    return FALSE;
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
    ++DevExt->RequestsStarted;
#endif

    if (!AtaReqGetTagForRequest(DevExt, Srb, &Tag))
    {
        Status = AtaReqDeviceQueueAddSrb(DevExt, Srb);

        KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);
    }
    else
    {
        Request = AtaReqAllocateRequestFromTag(DevExt, Tag);
        AtaReqStartRequest(DevExt, Request, Srb);

        Status = STATUS_PENDING;
    }

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
    KIRQL OldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&DevExt->QueueLock);

    DevExt->QueueFlags &= ~ReasonFlags;

    if (!AtaReqDeviceQueueDispatchNextRequest(DevExt))
        KeReleaseSpinLockFromDpcLevel(&DevExt->QueueLock);

    KeLowerIrql(OldIrql);
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

    Request = AtaReqRemovePortRequest(DevExt);

    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

    if (!DoWait)
        return TRUE;

    /* Requeue the pending request */
    if (Request)
    {
        Request->SrbStatus = SRB_STATUS_BUSY;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        AtaReqCompleteRequest(Request);
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
AtaReqFlushDeviceQueue(
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

        if (!DevExt->NextDeviceQueueEntry)
            break;

        QueueEntry = CONTAINING_RECORD(DevExt->NextDeviceQueueEntry,
                                       REQUEST_QUEUE_ENTRY,
                                       ListEntry);
        ASSERT(QueueEntry->Context == DevExt);

        AtaReqDeviceQueueRemoveEntry(DevExt, QueueEntry);

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
        ASSERT(Srb);
        ASSERT(Srb->OriginalRequest == Irp);

        Srb->SrbStatus = SRB_STATUS_ABORTED;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = Srb->DataTransferLength;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        KeAcquireSpinLock(&DevExt->QueueLock, &OldLevel);
    }

    Request = AtaReqRemovePortRequest(DevExt);

    KeReleaseSpinLock(&DevExt->QueueLock, OldLevel);

    if (Request)
    {
        Request->SrbStatus = SRB_STATUS_ABORTED;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        AtaReqCompleteRequest(Request);
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
            AtaReqFlushDeviceQueue(DevExt);
            __fallthrough;
        }
        case SRB_FUNCTION_UNLOCK_QUEUE:
        case SRB_FUNCTION_RELEASE_QUEUE:
        {
            ULONG Reason;

            if (Srb->Function == SRB_FUNCTION_UNLOCK_QUEUE)
                Reason = QUEUE_FLAG_FROZEN_QUEUE_LOCK;
            else
                Reason = QUEUE_FLAG_FROZEN_QUEUE_FREEZE;

            AtaReqThawQueue(DevExt, Reason);

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

VOID
NTAPI
AtaReqPortIoTimer(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PATAPORT_CHANNEL_EXTENSION ChanExt = Context;
    PATAPORT_PORT_DATA PortData;
    ULONG i;

    PortData = ChanExt->PortData;

    for (i = 0; i < ChanExt->NumberOfPorts; ++i)
    {
        KeAcquireSpinLockAtDpcLevel(&PortData->PortLock);

        for (i = 0; i < AHCI_MAX_COMMAND_SLOTS; ++i)
        {
            if (!(PortData->ActiveSlotsBitmap & (1 << i)))
                continue;

            if ((PortData->TimerCount[i] > 0) && (--PortData->TimerCount[i] == 0))
            {
                /* Handle timeout of an active command */
                AtaDeviceTimeout(PortData, i);
            }
        }

        KeReleaseSpinLockFromDpcLevel(&PortData->PortLock);

        ++PortData;
    }
}
