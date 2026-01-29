/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI I/O queue and requests handling
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

SLIST_HEADER AtapCompletionQueueList;
KDPC AtapCompletionDpc;

static DRIVER_CANCEL AtaReqDeviceQueueCancelIo;
static DRIVER_LIST_CONTROL AtaReqPreparePrdTable;
static DRIVER_CONTROL AtaReqCallSendRequestSerialized;

static
NTSTATUS
AtaReqDeviceQueueAddSrb(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb);

static
VOID
AtaReqDispatchRequest(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN DoReleaseDeviceQueueLock);

static
BOOLEAN
AtaReqDeviceQueueDispatchNextRequest(
    _In_ PATAPORT_IO_CONTEXT Device);

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
VOID
AtaDeviceCheckPowerState(
    _In_ PATAPORT_IO_CONTEXT Device)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    NTSTATUS Status;
    POWER_STATE PowerState;

   /*
    * The device might have been powered down as a part of an idle detection logic,
    * so we have to power up the device again.
    */
    if (!(Device->QueueFlags & QUEUE_FLAG_FROZEN_POWER))
        return;

    DevExt = CONTAINING_RECORD(Device, ATAPORT_DEVICE_EXTENSION, Device);
    if (DevExt->Common.DevicePowerState == PowerDeviceD0)
        return;

    if (DevExt->Common.SystemPowerState != PowerSystemWorking)
        return;

    INFO("Powering up idle device\n");

    PowerState.DeviceState = PowerDeviceD0;
    Status = PoRequestPowerIrp(DevExt->Common.Self,
                               IRP_MN_SET_POWER,
                               PowerState,
                               NULL,
                               NULL,
                               NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to power up device '%s' %lx\n", DevExt->FriendlyName, Status);
    }
}

static
VOID
AtaDeviceQueueEmptyEvent(
    _In_ PATAPORT_IO_CONTEXT Device)
{
    PSCSI_REQUEST_BLOCK Srb;

    KeSetEvent(&Device->QueueStoppedEvent, 0, FALSE);

    Srb = Device->QuiescenceSrb;
    if (Srb)
    {
        PIRP Irp = Srb->OriginalRequest;

        Device->QuiescenceSrb = NULL;

        KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);

        ASSERT(Srb->OriginalRequest == Irp);

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        KeAcquireSpinLockAtDpcLevel(&Device->QueueLock);
    }
}

static
NTSTATUS
AtaReqRequeueRequest(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request,
    _Out_ PUCHAR ResultSrbStatus)
{
    UCHAR SrbStatus;

    SrbStatus = SRB_STATUS(*ResultSrbStatus);

    /* Only requeue busy requests */
    if ((SrbStatus != SRB_STATUS_BUSY) && (SrbStatus != SRB_STATUS_INSUFFICIENT_RESOURCES))
        return COMPLETE_IRP;

    /* Retry at least once if the request was rejected due to lack of resources */
    if (SrbStatus == SRB_STATUS_INSUFFICIENT_RESOURCES)
    {
        if (SRB_GET_FLAGS(Request->Srb) & SRB_FLAG_LOW_MEM_RETRY)
        {
            /* We have no chance to dispatch it */
            return COMPLETE_IRP;
        }

        WARN("Retrying operation\n");

        SRB_SET_FLAGS(Request->Srb, SRB_FLAG_LOW_MEM_RETRY);
    }

    ASSERT(!(Request->Flags & (REQUEST_FLAG_HAS_SG_LIST |
                               REQUEST_FLAG_HAS_MDL |
                               REQUEST_FLAG_HAS_RESERVED_MAPPING)));

    /* Place the Srb back into the queue */
    if (AtaReqDeviceQueueAddSrb(Device, Request->Srb) != STATUS_PENDING)
    {
        /* We failed because of the IRP was cancelled, just update the status */
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
    PATAPORT_IO_CONTEXT Device;
    PATA_DEVICE_REQUEST Request;
    PLIST_ENTRY Entry;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (IsListEmpty(&PortData->PortQueueList))
        return FALSE;

    Entry = RemoveHeadList(&PortData->PortQueueList);

    Request = CONTAINING_RECORD(Entry, ATA_DEVICE_REQUEST, PortEntry);
    ASSERT_REQUEST(Request);

    Device = (PATAPORT_IO_CONTEXT)Request->Device;

    /* Try to unfreeze the device queue */
    Device->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;
    AtaReqDispatchRequest(Device, Request, FALSE);

    return TRUE;
}

static
VOID
AtaReqReleaseResources(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    if (Request->Flags & REQUEST_FLAG_HAS_SG_LIST)
    {
        PDMA_ADAPTER DmaAdapter = Device->PortData->DmaAdapter;
        PDMA_OPERATIONS DmaOperations = DmaAdapter->DmaOperations;

        ASSERT(Request->SgList);
        ASSERT(Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));

        DmaOperations->PutScatterGatherList(DmaAdapter,
                                            Request->SgList,
                                            !!(Request->Flags & REQUEST_FLAG_DATA_IN));
    }
    else if (Request->Flags & REQUEST_FLAG_HAS_RESERVED_MAPPING)
    {
        MmUnmapReservedMapping(Device->PortData->ReservedVaSpace, ATAPORT_TAG, Request->Mdl);
        _InterlockedExchange(&Device->PortData->ReservedMappingLock, 0);
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

static
VOID
AtaReqCompleteRequest(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    UCHAR SrbStatus;
    ATA_COMPLETION_ACTION CompletionAction = COMPLETE_IRP;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

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
                KeAcquireSpinLockAtDpcLevel(&Device->QueueLock);
                CompletionAction = Request->Complete(Request);
                KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);
            }
        }

        if (Device->PortData->PortFlags & PORT_FLAG_IS_SIMPLEX)
        {
            IoFreeController(Device->PortData->HwSyncObject);
        }
    }

    AtaReqReleaseResources(Device, Request);

    Srb = Request->Srb;

    Irp = Request->Irp;
    Irp->IoStatus.Information = Request->DataTransferLength;

    KeAcquireSpinLockAtDpcLevel(&Device->QueueLock);

    /* Release the request back to the device queue */
    ASSERT(!(Device->FreeRequestsBitmap & Request->Tag));
    Device->FreeRequestsBitmap |= Request->Tag;

    /* Release the slot back to the port queue */
    if (Request->InternalState != REQUEST_STATE_NOT_STARTED)
    {
        PATAPORT_PORT_DATA PortData = Device->PortData;

        KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);

        /* Stop the timer tied to the completed slot */
        PortData->ActiveTimersBitmap &= ~(1 << Request->Slot);

        /* Release exclusive access to the port queue */
        if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
            PortData->QueueFlags &= ~PORT_QUEUE_FLAG_EXCLUSIVE_MODE;

        /* Release the slot */
        ASSERT(!(PortData->FreeSlotsBitmap & (1 << Request->Slot)));
        PortData->FreeSlotsBitmap |= (1 << Request->Slot);
#if DBG
        PortData->Slots[Request->Slot] = NULL;
#endif
        if (Request->Flags & REQUEST_FLAG_NCQ)
            --PortData->AllocatedSlots;
        else
            ++PortData->AllocatedSlots;

        PortData->AllocateSlot(PortData->ChannelContext, Request, FALSE);

        if ((PortData->QueueFlags & PORT_QUEUE_FLAG_SIGNAL_STOP) && AtaPortQueueEmpty(PortData))
        {
            PortData->QueueFlags &= ~PORT_QUEUE_FLAG_SIGNAL_STOP;
            KeSetEvent(&PortData->QueueStoppedEvent, 0, FALSE);
        }

        /* Start the next request on the port queue */
        if (!AtaReqPortQueueListDispatchNextRequest(PortData))
            KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);
    }

    /* Check if we need to retry later */
    if (Request->InternalState == REQUEST_STATE_NOT_STARTED ||
        Request->InternalState == REQUEST_STATE_REQUEUE)
    {
        CompletionAction = AtaReqRequeueRequest(Device, Request, &Request->SrbStatus);
    }

#if DBG
    if (CompletionAction == COMPLETE_IRP)
        ++Device->Statistics.RequestsCompleted;
#endif

    /* Freeze the Srb queue in case of device error */
    if ((Request->InternalState == REQUEST_STATE_FREEZE_QUEUE) &&
        !(Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        ASSERT(SRB_STATUS(Request->SrbStatus) != SRB_STATUS_INSUFFICIENT_RESOURCES);

        Device->QueueFlags |= QUEUE_FLAG_FROZEN_QUEUE_FREEZE;
        Request->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
    }

    /* Signal the event when the list of active IRPs is empty */
    if ((Device->QueueFlags & QUEUE_FLAG_SIGNAL_STOP) &&
        (Device->FreeRequestsBitmap == Device->MaxRequestsBitmap))
    {
        Device->QueueFlags &= ~QUEUE_FLAG_SIGNAL_STOP;
        AtaDeviceQueueEmptyEvent(Device);
    }

    ASSERT(!(Request->Flags & (REQUEST_FLAG_HAS_SG_LIST |
                               REQUEST_FLAG_HAS_MDL |
                               REQUEST_FLAG_HAS_RESERVED_MAPPING)));

    SrbStatus = Request->SrbStatus;

    /*
     * Start the next request on the device queue.
     * It's important to do this before actually completing an IRP.
     */
    if (!AtaReqDeviceQueueDispatchNextRequest(Device))
        KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);

    /*
     * Complete the IRP outside of the spinlock to avoid deadlocks.
     * A new SCSI device I/O request might sent immediately after the IRP completed.
     */
    if (CompletionAction == COMPLETE_IRP)
    {
        if (SRB_STATUS(SrbStatus) == SRB_STATUS_INSUFFICIENT_RESOURCES)
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

    // TODO D_SENSE (fixed or descriptor format)
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

    // TODO D_SENSE (fixed or descriptor format)
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

static
VOID
AtaReqStartIo(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;
    PATAPORT_PORT_DATA PortData = Device->PortData;
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(PortData->Slots[Request->Slot] == Request);

    PortData->PrepareIo(PortData->ChannelContext, Request);

    OldIrql = KeAcquireInterruptSpinLock(PortData->InterruptObject);

    ASSERT(!(PortData->ActiveSlotsBitmap & (1 << Request->Slot)));

    if ((PortData->InterruptFlags & PORT_INT_FLAG_IS_IO_ACTIVE) ||
        (Request->Flags & REQUEST_FLAG_INTERNAL))
    {
        PortData->ActiveSlotsBitmap |= 1 << Request->Slot;

        if (!PortData->StartIo(PortData->ChannelContext, Request))
        {
            PortData->TimerCount[Request->Slot] = Request->TimeOut;
        }

        KeReleaseInterruptSpinLock(PortData->InterruptObject, OldIrql);
    }
    else
    {
        PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;

        /*
         * Needed to prevent an infinite loop that happens
         * when the queue manager keeps retrying the SRB at dispatch level.
         */
        _InterlockedOr(&Device->QueueFlags, QUEUE_FLAG_FROZEN_PORT_BUSY);

        KeReleaseInterruptSpinLock(PortData->InterruptObject, OldIrql);

        /* If the queue is frozen, then force queue manager to requeue the SRB */
        Request->SrbStatus = SRB_STATUS_BUSY;
        Request->InternalState = REQUEST_STATE_REQUEUE;

        /* Defer the completion to a DPC to avoid the recursive call in some cases */
        AtaReqStartCompletionDpc(Request);
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
    PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;
    PATAPORT_PORT_DATA PortData = Device->PortData;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    Request->SgList = SgList;
    Request->Flags |= REQUEST_FLAG_HAS_SG_LIST;

    PortData->PreparePrdTable(PortData->ChannelContext, Request, SgList);
    AtaReqStartIo(Request);
}

static
PVOID
AtaReqMapBuffer(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_PORT_DATA PortData;
    PVOID BaseAddress;
    ULONG PagesNeeded;

    BaseAddress = MmGetSystemAddressForMdlSafe(Request->Mdl, HighPagePriority);
    if (BaseAddress)
        return BaseAddress;

    PortData = Device->PortData;
    if (!PortData->ReservedVaSpace)
        return NULL;

    /* The system is low resources, handle it in a non-fatal way */
    PagesNeeded = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Request->Mdl),
                                                 MmGetMdlByteCount(Request->Mdl));
    if (PagesNeeded > ATA_RESERVED_PAGES)
        return NULL;

    /* Utilize the reserved mapping to overcome memory issues */
    if (!_InterlockedCompareExchange(&PortData->ReservedMappingLock, 1, 0))
    {
        BaseAddress = MmMapLockedPagesWithReservedMapping(PortData->ReservedVaSpace,
                                                          ATAPORT_TAG,
                                                          Request->Mdl,
                                                          MmCached);
        if (BaseAddress)
            Request->Flags |= REQUEST_FLAG_HAS_RESERVED_MAPPING;
        else
            _InterlockedExchange(&PortData->ReservedMappingLock, 0);
    }

    return BaseAddress;
}

static
BOOLEAN
AtaReqGetScatterGatherList(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request)
{
    PDMA_ADAPTER DmaAdapter = PortData->DmaAdapter;
    PDMA_OPERATIONS DmaOperations = DmaAdapter->DmaOperations;
    NTSTATUS Status;

    ASSERT(Request->Mdl);

    Status = DmaOperations->GetScatterGatherList(DmaAdapter,
                                                 PortData->ChannelObject,
                                                 Request->Mdl,
                                                 Request->DataBuffer,
                                                 Request->DataTransferLength,
                                                 AtaReqPreparePrdTable,
                                                 Request,
                                                 !!(Request->Flags & REQUEST_FLAG_DATA_IN));
    if (NT_SUCCESS(Status))
        return TRUE;

    WARN("Failed to get the S/G list with status %lx\n", Status);
    return FALSE;
}

VOID
AtaReqSendRequest(
    _In_ PATA_DEVICE_REQUEST Request)
{
    PATAPORT_IO_CONTEXT Device = (PATAPORT_IO_CONTEXT)Request->Device;

    PVOID BaseAddress;
    ULONG_PTR Offset;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* The channel can only perform DMA I/O and PIO is not supported */
    if (Device->DeviceFlags & DEVICE_PIO_VIA_DMA)
        Request->Flags |= REQUEST_FLAG_PROGRAM_DMA;

    if (Request->Flags & REQUEST_FLAG_DMA)
        ASSERT(Request->Flags & REQUEST_FLAG_PROGRAM_DMA);

    if (!(Request->Flags & REQUEST_FLAG_NO_KEEP_AWAKE))
    {
        PULONG PowerIdleCounter = Device->PowerIdleCounter;

        if (PowerIdleCounter)
            PoSetDeviceBusy(PowerIdleCounter);
    }

    /* Local buffer transfer */
    if (Request->Flags & REQUEST_FLAG_HAS_LOCAL_BUFFER)
    {
        ASSERT(Request->Flags & REQUEST_FLAG_INTERNAL);

        if (Request->Flags & REQUEST_FLAG_PROGRAM_DMA)
        {
            /* DMA transfer */
            Device->PortData->LocalSgList.Elements[0].Length = Request->DataTransferLength;
            AtaReqPreparePrdTable(NULL, NULL, &Device->PortData->LocalSgList, Request);
        }
        else
        {
            /* PIO data transfer */
            Request->DataBuffer = Device->LocalBuffer;
            AtaReqStartIo(Request);
        }
        return;
    }

    /* No data transfer */
    if (!(Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT)))
    {
        AtaReqStartIo(Request);
        return;
    }

    /* DMA transfer, get the S/G list for the MDL */
    if (Request->Flags & REQUEST_FLAG_PROGRAM_DMA)
    {
        if (AtaReqGetScatterGatherList(Device->PortData, Request))
            return;

        /* This channel can only perform DMA I/O and PIO is not supported */
        if (Device->DeviceFlags & DEVICE_PIO_VIA_DMA)
            goto CompleteNoMemory;

        /* S/G list construction failed, attempt to fall back to PIO mode */
        if (!AtaReqDmaTransferToPioTransfer(Request))
            goto CompleteNoMemory;
    }

    /* PIO data transfer path */
    ASSERT(!(Device->DeviceFlags & DEVICE_PIO_VIA_DMA));

    BaseAddress = AtaReqMapBuffer(Device, Request);
    if (!BaseAddress)
        goto CompleteNoMemory;

    /* Calculate the offset within DataBuffer */
    Offset = (ULONG_PTR)BaseAddress +
             (ULONG_PTR)Request->DataBuffer -
             (ULONG_PTR)MmGetMdlVirtualAddress(Request->Mdl);
    Request->DataBuffer = (PVOID)Offset;

    AtaReqStartIo(Request);
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

static
IO_ALLOCATION_ACTION
NTAPI
AtaReqCallSendRequestSerialized(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID Context)
{
    PATA_DEVICE_REQUEST Request = Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(MapRegisterBase);

    AtaReqSendRequest(Request);
    return KeepObject;
}

VOID
NTAPI
AtaReqCompletionDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PSLIST_ENTRY CurrentEntry, NextEntry;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    CurrentEntry = ExInterlockedFlushSList(&AtapCompletionQueueList);
    while (CurrentEntry)
    {
        PATA_DEVICE_REQUEST Request;

        NextEntry = CurrentEntry->Next;

        Request = CONTAINING_RECORD(CurrentEntry, ATA_DEVICE_REQUEST, CompletionEntry);
        ASSERT_REQUEST(Request);

        AtaReqCompleteRequest(Request);

        CurrentEntry = NextEntry;
    }
}

VOID
AtaReqStartCompletionDpc(
    _In_ PATA_DEVICE_REQUEST Request)
{
    ASSERT_REQUEST(Request);

    InterlockedPushEntrySList(&AtapCompletionQueueList, &Request->CompletionEntry);
    KeInsertQueueDpc(&AtapCompletionDpc, NULL, NULL);
}

/*
 * The control flow is designed such that
 * we can just return FALSE without having to undo the failed slot allocation later.
 */
static
BOOLEAN
AtaReqAllocateSlot(
    _In_ PATAPORT_IO_CONTEXT Device,
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

    /* Queued command */
    if (Request->Flags & REQUEST_FLAG_NCQ)
    {
        ULONG QueueDepth;

        /* Check if we have any outstanding non-queued commands */
        if (PortData->AllocatedSlots < 0)
            return FALSE;

        QueueDepth = Device->TransportFlags & DEVICE_QUEUE_DEPTH_MASK;
        QueueDepth >>= DEVICE_QUEUE_DEPTH_SHIFT;

        /* The device capacity may be less than the total HBA capacity */
        if (PortData->AllocatedSlots >= QueueDepth)
            return FALSE;
    }
    else
    {
        /* Check if we have any outstanding native queued commands */
        if (PortData->AllocatedSlots > 0)
            return FALSE;
    }

    SlotsBitmap = PortData->FreeSlotsBitmap;

    /* Mask off previously issued commands */
    SlotMask = ~(0xFFFFFFFF >> (MAX_SLOTS - (PortData->LastUsedSlot + 1)));
    if (!(SlotsBitmap & SlotMask))
        SlotMask = 0xFFFFFFFF;

    /* Allocate slot in a circular fashion. This is required to maintain CCS update */
    if (!_BitScanForward(&SlotNumber, SlotsBitmap & SlotMask))
        return FALSE;

    if (!PortData->AllocateSlot(PortData->ChannelContext, Request, TRUE))
        return FALSE;

    /* The slot can safely be consumed at this point */
    if (Request->Flags & REQUEST_FLAG_NCQ)
        ++PortData->AllocatedSlots;
    else
        --PortData->AllocatedSlots;

    if (Request->Flags & REQUEST_EXCLUSIVE_ACCESS_FLAGS)
        PortData->QueueFlags |= PORT_QUEUE_FLAG_EXCLUSIVE_MODE;

    ASSERT(PortData->LastUsedSlot < MAX_SLOTS);
    ASSERT(IsPowerOfTwo(SlotsBitmap) || (SlotNumber != PortData->LastUsedSlot));
    ASSERT(PortData->FreeSlotsBitmap & (1 << SlotNumber));
    ASSERT(PortData->Slots[SlotNumber] == NULL);

    Request->Slot = SlotNumber;

    PortData->LastUsedSlot = SlotNumber;
    PortData->FreeSlotsBitmap &= ~(1 << SlotNumber);
    PortData->Slots[SlotNumber] = Request;

    return TRUE;
}

static
VOID
AtaReqDispatchRequest(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN DoReleaseDeviceQueueLock)
{
    PATAPORT_PORT_DATA PortData = Device->PortData;
    BOOLEAN Success;

    Success = AtaReqAllocateSlot(Device, PortData, Request);
    if (Success)
    {
        PortData->ActiveTimersBitmap |= 1 << Request->Slot;
    }
    else
    {
        /*
         * If all slots are busy, overloading the port can starve other incoming I/O requests:
         * - An IDE channel can only deal with one active request at a time.
         * - A SATA Port Multiplier shares the bandwidth with up to 15 devices.
         * In order to avoid starvation of the device,
         * put the request on the high-priority port queue.
         */
        InsertTailList(&PortData->PortQueueList, &Request->PortEntry);

        /*
         * Freeze the device queue to ensure
         * that only one request per device is queued to the port queue.
         */
        ASSERT(!(Device->QueueFlags & QUEUE_FLAG_FROZEN_SLOT));
        Device->QueueFlags |= QUEUE_FLAG_FROZEN_SLOT;
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    if (DoReleaseDeviceQueueLock)
        KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);

    if (Success)
    {
        if (PortData->PortFlags & PORT_FLAG_IS_SIMPLEX)
        {
            IoAllocateController(PortData->HwSyncObject,
                                 PortData->ChannelObject,
                                 AtaReqCallSendRequestSerialized,
                                 Request);
        }
        else
        {
            AtaReqSendRequest(Request);
        }
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
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATAPORT_DEVICE_EXTENSION DevExt;
    UCHAR SrbStatus;

    DevExt = CONTAINING_RECORD(Device, ATAPORT_DEVICE_EXTENSION, Device);

    SrbStatus = AtaReqTranslateRequest(DevExt, Request, Srb);

    if (SrbStatus != SRB_STATUS_PENDING)
    {
        KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);

        Request->SrbStatus = SrbStatus;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        /* Defer the completion to a DPC to avoid the recursive call in some cases */
        AtaReqStartCompletionDpc(Request);
    }
    else
    {
        Request->Flags |= Srb->SrbFlags & REQUEST_FLAG_NO_KEEP_AWAKE;

        KeAcquireSpinLockAtDpcLevel(&Device->PortData->QueueLock);

        AtaReqDispatchRequest(Device, Request, TRUE);
    }
}

static
PATA_DEVICE_REQUEST
AtaReqRemovePortRequest(
    _In_ PATAPORT_IO_CONTEXT Device)
{
    PATAPORT_PORT_DATA PortData = Device->PortData;
    PLIST_ENTRY Entry;
    PATA_DEVICE_REQUEST Request, Result = NULL;

    if (!(Device->QueueFlags & QUEUE_FLAG_FROZEN_SLOT))
        return NULL;

    Device->QueueFlags &= ~QUEUE_FLAG_FROZEN_SLOT;

    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);

    for (Entry = PortData->PortQueueList.Flink;
         Entry != &PortData->PortQueueList;
         Entry = Entry->Flink)
    {
        Request = CONTAINING_RECORD(Entry, ATA_DEVICE_REQUEST, PortEntry);
        ASSERT_REQUEST(Request);

        if (Request->Device != (PATA_IO_CONTEXT_COMMON)Device)
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
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PREQUEST_QUEUE_ENTRY QueueEntry)
{
    RemoveEntryList(&QueueEntry->ListEntry);
}

static
VOID
NTAPI
AtaReqDeviceQueueCancelIo(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ _IRQL_uses_cancel_ PIRP Irp)
{
    PATAPORT_IO_CONTEXT Device;
    KIRQL OldLevel;
    PSCSI_REQUEST_BLOCK Srb;
    PREQUEST_QUEUE_ENTRY QueueEntry;

    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    QueueEntry = QUEUE_ENTRY_FROM_IRP(Irp);
    Device = QueueEntry->Context;

    KeAcquireSpinLock(&Device->QueueLock, &OldLevel);
    AtaReqDeviceQueueRemoveEntry(Device, QueueEntry);
    KeReleaseSpinLock(&Device->QueueLock, OldLevel);

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
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PIRP Irp)
{
    PLIST_ENTRY Entry;
    PREQUEST_QUEUE_ENTRY QueueEntry;
    ULONG SortKey;

    SortKey = Srb->QueueSortKey;

    /* Use the C-LOOK hard disk scheduling algorithm to service the PDO requests */
    for (Entry = Device->DeviceQueueList.Flink;
         Entry != &Device->DeviceQueueList;
         Entry = Entry->Flink)
    {
        QueueEntry = CONTAINING_RECORD(Entry, REQUEST_QUEUE_ENTRY, ListEntry);
        ASSERT(QueueEntry->Context == Device);

        if (QueueEntry->SortKey > SortKey)
            break;
    }

    QueueEntry = QUEUE_ENTRY_FROM_IRP(Irp);
    QueueEntry->Context = Device;
    QueueEntry->SortKey = SortKey;

    InsertTailList(Entry, &QueueEntry->ListEntry);
}

static
NTSTATUS
AtaReqDeviceQueueAddSrb(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PIRP Irp = Srb->OriginalRequest;

    if (Device->QueueFlags & QUEUE_FLAG_FROZEN_REMOVED)
    {
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        return STATUS_NO_SUCH_DEVICE;
    }

    AtaReqDeviceQueueInsertSrb(Device, Srb, Irp);

    AtaDeviceCheckPowerState(Device);

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
        AtaReqDeviceQueueRemoveEntry(Device, QUEUE_ENTRY_FROM_IRP(Irp));

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
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PULONG Tag)
{
    /* Check if the queue is actually frozen and the request is not a bypass */
    if (Device->QueueFlags & (QUEUE_FLAGS_FROZEN &
                              ~(QUEUE_FLAG_FROZEN_QUEUE_LOCK | QUEUE_FLAG_FROZEN_QUEUE_FREEZE)))
    {
        return FALSE;
    }
    if ((Device->QueueFlags & QUEUE_FLAG_FROZEN_QUEUE_FREEZE) &&
        !(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE))
    {
        return FALSE;
    }
    if ((Device->QueueFlags & QUEUE_FLAG_FROZEN_QUEUE_LOCK) &&
        !(Srb->SrbFlags & SRB_FLAGS_BYPASS_LOCKED_QUEUE))
    {
        return FALSE;
    }

    /*
     * Stack-based request allocation.
     * It will return us the last request structure in the CPU cache.
     */
    return _BitScanForward(Tag, Device->FreeRequestsBitmap);
}

static
inline
PATA_DEVICE_REQUEST
AtaReqAllocateRequestFromTag(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ ULONG Tag)
{
    PATA_DEVICE_REQUEST Request;

    ASSERT(Device->FreeRequestsBitmap & (1 << Tag));

    Device->FreeRequestsBitmap &= ~(1 << Tag);

    Request = &Device->Requests[Tag];
    ASSERT_REQUEST(Request);

    Request->Tag = 1 << Tag;

    return Request;
}

static
BOOLEAN
AtaReqDeviceQueueDispatchNextRequest(
    _In_ PATAPORT_IO_CONTEXT Device)
{
    PREQUEST_QUEUE_ENTRY QueueEntry;
    PIRP Irp;
    PSCSI_REQUEST_BLOCK Srb;
    PATA_DEVICE_REQUEST Request;
    ULONG Tag;

    if (Device->FreeRequestsBitmap == 0)
        return FALSE;

    /* Find a bypass request to dispatch */
    if ((Device->QueueFlags & (QUEUE_FLAG_FROZEN_QUEUE_FREEZE | QUEUE_FLAG_FROZEN_QUEUE_LOCK)) &&
        !(Device->QueueFlags & QUEUE_FLAGS_FROZEN_NOT_BYPASS))
    {
        PLIST_ENTRY Entry;

        for (Entry = Device->DeviceQueueList.Flink;
             Entry != &Device->DeviceQueueList;
             Entry = Entry->Flink)
        {
            QueueEntry = CONTAINING_RECORD(Entry, REQUEST_QUEUE_ENTRY, ListEntry);
            ASSERT(QueueEntry->Context == Device);

            Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
            Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;

            ASSERT(Srb);
            ASSERT(Srb->OriginalRequest == Irp);

            if (!AtaReqGetTagForRequest(Device, Srb, &Tag))
                continue;

            AtaReqDeviceQueueRemoveEntry(Device, QueueEntry);

            /* Clear our cancel routine */
            if (!IoSetCancelRoutine(Irp, NULL))
            {
                /* We're already canceled, reset the list entry to point to itself */
                InitializeListHead(&QueueEntry->ListEntry);
                continue;
            }

            Request = AtaReqAllocateRequestFromTag(Device, Tag);

            AtaReqStartRequest(Device, Request, Srb);
            return TRUE;
        }
    }
    else
    {
        PLIST_ENTRY Entry;

        if (Device->QueueFlags & QUEUE_FLAGS_FROZEN)
            return FALSE;

        NT_VERIFY(_BitScanForward(&Tag, Device->FreeRequestsBitmap));

        for (Entry = Device->DeviceQueueList.Flink;
             Entry != &Device->DeviceQueueList;
             Entry = Entry->Flink)
        {
            QueueEntry = CONTAINING_RECORD(Entry,
                                           REQUEST_QUEUE_ENTRY,
                                           ListEntry);
            ASSERT(QueueEntry->Context == Device);

            Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
            Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;

            ASSERT(Srb);
            ASSERT(Srb->OriginalRequest == Irp);

            AtaReqDeviceQueueRemoveEntry(Device, QueueEntry);

            /* Clear our cancel routine */
            if (!IoSetCancelRoutine(Irp, NULL))
            {
                /* We're already canceled, reset the list entry to point to itself */
                InitializeListHead(&QueueEntry->ListEntry);
                continue;
            }

            Request = AtaReqAllocateRequestFromTag(Device, Tag);

            AtaReqStartRequest(Device, Request, Srb);
            return TRUE;
        }
    }

    return FALSE;
}

static
NTSTATUS
AtaReqStartSrb(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PATA_DEVICE_REQUEST Request;
    ULONG Tag;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&Device->QueueLock);

#if DBG
    ++Device->Statistics.RequestsStarted;
#endif

    if (!AtaReqGetTagForRequest(Device, Srb, &Tag))
    {
        Status = AtaReqDeviceQueueAddSrb(Device, Srb);

        KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);
    }
    else
    {
        Request = AtaReqAllocateRequestFromTag(Device, Tag);
        AtaReqStartRequest(Device, Request, Srb);

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
    PATAPORT_IO_CONTEXT Device = &DevExt->Device;
    KIRQL OldLevel;

    KeAcquireSpinLock(&Device->QueueLock, &OldLevel);
    _InterlockedOr(&Device->QueueFlags, ReasonFlags);
    KeReleaseSpinLock(&Device->QueueLock, OldLevel);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqThawQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags)
{
    PATAPORT_IO_CONTEXT Device = &DevExt->Device;
    PATAPORT_PORT_DATA PortData;
    KIRQL OldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&Device->QueueLock);

    if (Device->FreeRequestsBitmap != Device->MaxRequestsBitmap)
        AtaDeviceCheckPowerState(Device);

    PortData = Device->PortData;
    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);
    if (!AtaReqPortQueueListDispatchNextRequest(PortData))
        KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);

    _InterlockedAnd(&Device->QueueFlags, ~ReasonFlags);

    if (!AtaReqDeviceQueueDispatchNextRequest(Device))
        KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);

    KeLowerIrql(OldIrql);
}

/* Must not be paged */
DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqWaitForOutstandingIoToComplete(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    KIRQL OldIrql;
    BOOLEAN DoWait;
    PATA_DEVICE_REQUEST Request;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&Device->QueueLock);

    ASSERT(Device->QueueFlags & QUEUE_FLAGS_FROZEN);

    KeClearEvent(&Device->QueueStoppedEvent);

    if (Srb)
        Device->QuiescenceSrb = Srb;

    /* Wait for all the active IRPs to finish executing */
    if (Device->FreeRequestsBitmap != Device->MaxRequestsBitmap)
    {
        Device->QueueFlags |= QUEUE_FLAG_SIGNAL_STOP;
        DoWait = !Srb;
    }
    else
    {
        AtaDeviceQueueEmptyEvent(Device);

        KeSetEvent(&Device->QueueStoppedEvent, 0, FALSE);
        DoWait = FALSE;
    }

    Request = AtaReqRemovePortRequest(Device);

    KeReleaseSpinLockFromDpcLevel(&Device->QueueLock);
    KeLowerIrql(OldIrql);

    /* Requeue the pending request */
    if (Request)
    {
        Request->SrbStatus = SRB_STATUS_BUSY;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        AtaReqCompleteRequest(Request);
        KeLowerIrql(OldIrql);
    }

    if (!DoWait)
        return;

    KeWaitForSingleObject(&Device->QueueStoppedEvent, Executive, KernelMode, FALSE, NULL);
}

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFlushDeviceQueue(
    _In_ PATAPORT_IO_CONTEXT Device)
{
    PATA_DEVICE_REQUEST Request;
    KIRQL OldLevel;
    PLIST_ENTRY Entry;

    KeAcquireSpinLock(&Device->QueueLock, &OldLevel);

    for (Entry = Device->DeviceQueueList.Flink;
         Entry != &Device->DeviceQueueList;
         Entry = Entry->Flink)
    {
        PREQUEST_QUEUE_ENTRY QueueEntry;
        PIRP Irp;
        PSCSI_REQUEST_BLOCK Srb;

        QueueEntry = CONTAINING_RECORD(Entry, REQUEST_QUEUE_ENTRY, ListEntry);
        ASSERT(QueueEntry->Context == Device);

        AtaReqDeviceQueueRemoveEntry(Device, QueueEntry);

        /* Clear our cancel routine */
        Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
        if (!IoSetCancelRoutine(Irp, NULL))
        {
            /* We're already canceled, reset the list entry to point to itself */
            InitializeListHead(&QueueEntry->ListEntry);
            continue;
        }

        KeReleaseSpinLock(&Device->QueueLock, OldLevel);

        Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
        ASSERT(Srb);
        ASSERT(Srb->OriginalRequest == Irp);

        Srb->SrbStatus = SRB_STATUS_ABORTED;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = Srb->DataTransferLength;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        KeAcquireSpinLock(&Device->QueueLock, &OldLevel);
    }

    Request = AtaReqRemovePortRequest(Device);

    KeReleaseSpinLock(&Device->QueueLock, OldLevel);

    if (Request)
    {
        KIRQL OldIrql;

        Request->SrbStatus = SRB_STATUS_ABORTED;
        Request->InternalState = REQUEST_STATE_NOT_STARTED;

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        AtaReqCompleteRequest(Request);
        KeLowerIrql(OldIrql);
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
    ULONG Status, SrbStatus;

    PAGED_CODE();

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
    Srb->SrbStatus = SrbStatus;
    return Status;
}

static
NTSTATUS
AtaPdoHandleQuiesceDevice(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PIRP Irp,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(Device->QueueFlags & QUEUE_FLAG_FROZEN_QUEUE_LOCK);

    IoMarkIrpPending(Irp);

    /* Wait for outstanding I/O requests to finish */
    AtaReqWaitForOutstandingIoToComplete(Device, Srb);

    Srb->SrbStatus = SRB_STATUS_PENDING;
    return STATUS_PENDING;
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
            ASSERT(DevExt->Device.QueueFlags & QUEUE_FLAGS_FROZEN);
            AtaReqFlushDeviceQueue(&DevExt->Device);
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
            Status = AtaPdoHandleQuiesceDevice(&DevExt->Device, Irp, Srb);
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
            AtaScsiAddress = DevExt->Device.AtaScsiAddress;
            Srb->PathId = AtaScsiAddress.PathId;
            Srb->TargetId = AtaScsiAddress.TargetId;
            Srb->Lun = AtaScsiAddress.Lun;

            /* This field is used by the driver to mark internal requests */
            Srb->SrbExtension = NULL;

            /*
             * NOTE: Disk I/O requests need a lot of the kernel stack space.
             * We should avoid nesting several levels deep in the call chain.
             */
            Status = AtaReqStartSrb(&DevExt->Device, Srb);
            break;
        }

        default:
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

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
AtaPortIoTimer(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PATAPORT_PORT_DATA PortData = Context;
    ULONG Slot, ActiveTimersBitmap;

    KeAcquireSpinLockAtDpcLevel(&PortData->QueueLock);

    /* Check timeouts */
    ActiveTimersBitmap = PortData->ActiveTimersBitmap;
    while (_BitScanForward(&Slot, ActiveTimersBitmap) != 0)
    {
        ActiveTimersBitmap &= ~(1 << Slot);

        /* Decrease the timeout counter */
        if ((PortData->TimerCount[Slot] > 0) && (--PortData->TimerCount[Slot] == 0))
        {
            KIRQL OldIrql = KeAcquireInterruptSpinLock(PortData->InterruptObject);

            /* Handle timeout of an active command */
            if (PortData->ActiveSlotsBitmap & (1 << Slot))
                AtaPortTimeout(PortData, Slot);

            KeReleaseInterruptSpinLock(PortData->InterruptObject, OldIrql);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&PortData->QueueLock);
}
