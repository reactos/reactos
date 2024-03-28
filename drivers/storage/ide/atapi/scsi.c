/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI requests handling
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

static const ULONG AtapPollIntervalMs[] =
{
    20, 50, 75, 100
};

/* FUNCTIONS ******************************************************************/

static
ULONG
AtaReqSendRequest(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    ULONG SrbStatus;
    KIRQL OldIrql;

    if (Request->Flags & REQUEST_FLAG_ASYNC_MODE)
    {
        OldIrql = KeAcquireInterruptSpinLock(ChannelExtension->InterruptObject);

        SrbStatus = AtaExecuteCommand(ChannelExtension);

        KeReleaseInterruptSpinLock(ChannelExtension->InterruptObject, OldIrql);
    }
    else
    {
        SrbStatus = AtaExecuteCommand(ChannelExtension);

        /* Queue a DPC to poll for the status register */
        if (SrbStatus == SRB_STATUS_PENDING)
        {
            ASSERT(!(Request->Flags & REQUEST_FLAG_DMA_ENABLED));

            ChannelExtension->PollCount = 0;
            ChannelExtension->PollInterval = 0;
            ChannelExtension->PollState = POLL_REQUEST;

            Request->Flags |= REQUEST_FLAG_POLLING_ACTIVE;
            KeInsertQueueDpc(&ChannelExtension->PollingTimerDpc, NULL, NULL);
        }
    }

    return SrbStatus;
}

static
VOID
AtaReqStartIo(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    ULONG SrbStatus;

    /*
     * HACK: In an emulated PC-98 system IDE interrupts are often lost.
     * The reason for this problem is unknown (not the driver's fault).
     * Always poll for the command completion as a workaround.
     */
    if (IsNEC_98)
        Request->Flags &= ~REQUEST_FLAG_ASYNC_MODE;

    /* TODO */
    if (IS_AHCI(ChannelExtension))
        Request->Flags &= ~REQUEST_FLAG_ASYNC_MODE;

    ChannelExtension->BytesToTransfer = Request->DataTransferLength;
    ChannelExtension->DataBuffer = Request->DataBuffer;

    KeAcquireSpinLockAtDpcLevel(&ChannelExtension->ChannelLock);

    /* Set the time out value */
    ChannelExtension->TimerCount = Request->Srb->TimeOutValue;

    SrbStatus = AtaReqSendRequest(ChannelExtension);

    /* Complete immediately if we don't need to wait for an interrupt to occur */
    if (SrbStatus != SRB_STATUS_PENDING)
    {
        Request->SrbStatus = SrbStatus;

        Request->Flags |= REQUEST_FLAG_COMPLETED;
        KeInsertQueueDpc(&ChannelExtension->Dpc, NULL, NULL);
    }

    KeReleaseSpinLockFromDpcLevel(&ChannelExtension->ChannelLock);
}

IO_ALLOCATION_ACTION
NTAPI
AtaReqStartIoSync(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID Context)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(MapRegisterBase);

    AtaReqStartIo(ChannelExtension);

    return KeepObject;
}

VOID
AtaReqDataTransferReady(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    if (ChannelExtension->Flags & CHANNEL_SIMPLEX_DMA)
    {
        // TODO
    }
    else
    {
        AtaReqStartIo(ChannelExtension);
    }
}

static
PVOID
AtaReqMapBuffer(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PMDL Mdl)
{
    PVOID BaseAddress;
    ULONG PagesNeeded;

    BaseAddress = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
    if (BaseAddress)
        return BaseAddress;

    if (!ChannelExtension->ReservedVaSpace)
        return NULL;

    /* The system is low resources, handle it in a non-fatal way */
    PagesNeeded = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Mdl),
                                                 MmGetMdlByteCount(Mdl));
    if (PagesNeeded > ATA_RESERVED_PAGES)
        return NULL;

    /* Need to retry to overcome memory issues */
    BaseAddress = MmMapLockedPagesWithReservedMapping(ChannelExtension->ReservedVaSpace,
                                                      IDEPORT_TAG,
                                                      Mdl,
                                                      MmCached);
    if (BaseAddress)
    {
        ChannelExtension->Request.Flags |= REQUEST_FLAG_RESERVED_MAPPING;
    }

    return BaseAddress;
}

static
BOOLEAN
AtaReqPreparePioDataTransfer(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;

    ASSERT(Request->Flags & (REQUEST_FLAG_DATA_IN | REQUEST_FLAG_DATA_OUT));

    Request->Flags &= ~REQUEST_FLAG_DMA_ENABLED;

    /* TODO: ATAPI passthrough support? */
    if (Request->Flags & REQUEST_FLAG_PACKET_COMMAND)
        return TRUE;

    /* For ATA R/W commands, we have to fix the command opcode */
    if (Request->Flags & REQUEST_FLAG_READ_WRITE)
    {
        Request->TaskFile.Command = AtaReadWriteCommand(Request, Request->DeviceExtension);
        if (Request->TaskFile.Command == 0)
            return FALSE;

        return TRUE;
    }

    /* PIO is not available */
    return FALSE;
}

static
ULONG
AtaReqPrepareDataTransfer(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;

    /* DMA transfer */
    if (Request->Flags & REQUEST_FLAG_DMA_ENABLED)
    {
        PPCIIDE_INTERFACE PciIdeDma = &ChannelExtension->PciIdeInterface;
        NTSTATUS Status;

        Status = PciIdeDma->AdapterObject->
            DmaOperations->GetScatterGatherList(PciIdeDma->AdapterObject,
                                                PciIdeDma->DeviceObject,
                                                Request->Mdl,
                                                Request->DataBuffer,
                                                Request->DataTransferLength,
                                                AtaPciIdeDmaPreparePrdTable,
                                                ChannelExtension,
                                                !!(Request->Flags & REQUEST_FLAG_DATA_IN));
        if (NT_SUCCESS(Status))
            return SRB_STATUS_PENDING;

        /* Fall back to PIO */
        if (!AtaReqPreparePioDataTransfer(ChannelExtension))
            return SRB_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* PIO transfer */
    if (Request->Mdl)
    {
        PVOID BaseAddress;
        ULONG_PTR Offset;

        BaseAddress = AtaReqMapBuffer(ChannelExtension, Request->Mdl);
        if (!BaseAddress)
        {
            /* We have no chance to dispatch it */
            return SRB_STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Calculate the offset within DataBuffer */
        Offset = (ULONG_PTR)BaseAddress +
                 (ULONG_PTR)Request->DataBuffer -
                 (ULONG_PTR)MmGetMdlVirtualAddress(Request->Mdl);

        Request->DataBuffer = (PVOID)Offset;
    }

    AtaReqDataTransferReady(ChannelExtension);
    return SRB_STATUS_PENDING;
}

static
ULONG
AtaReqDispatch(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;

    if (Request->Flags & SRB_FLAGS_UNSPECIFIED_DIRECTION)
    {
        /* Data is transferred to or from the device with this I/O request */
        return AtaReqPrepareDataTransfer(ChannelExtension);
    }
    else
    {
        /* No data transfer */
        Request->Flags &= ~REQUEST_FLAG_DMA_ENABLED;
        AtaReqDataTransferReady(ChannelExtension);

        return SRB_STATUS_PENDING;
    }
}

static
ATA_COMPLETION_STATUS
AtaReqCompletePassthrough(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;

    if (Request->Flags & REQUEST_FLAG_HAS_TASK_FILE)
    {
        PIDEREGS IdeRegs = (PIDEREGS)&Request->Srb->Cdb[8];

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
        // TODO
    }

    return COMPLETE_IRP;
}

static
ULONG
AtaReqPassthrough(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    PATA_TASKFILE TaskFile = &Request->TaskFile;
    PIDEREGS IdeRegs;
    UCHAR AtaFlags;

    Request->Complete = AtaReqCompletePassthrough;
    Request->DeviceExtension = DeviceExtension;
    Request->DataBuffer = Srb->DataBuffer;
    Request->DataTransferLength = Srb->DataTransferLength;
    Request->Mdl = Request->Irp->MdlAddress;
    Request->Flags = REQUEST_FLAG_ASYNC_MODE |
                     REQUEST_FLAG_SET_DEVICE_REGISTER |
                     REQUEST_FLAG_SAVE_TASK_FILE |
                     (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION);

    AtaFlags = Srb->Cdb[7];
    if ((AtaFlags & ATA_FLAGS_USE_DMA) && !(DeviceExtension->Flags & DEVICE_PIO_ONLY))
    {
        Request->Flags |= REQUEST_FLAG_DMA_ENABLED;
    }
/*
    // TODO Check for a command opcode
    if (!(AtaFlags & ATA_FLAGS_NO_MULTIPLE) && (DeviceExtension->MultiSectorTransfer != 0))
    {
        Request->Flags |= REQUEST_FLAG_READ_WRITE_MULTIPLE;
    }
*/

    IdeRegs = (PIDEREGS)&Srb->Cdb[8];

    TRACE("TF: %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          IdeRegs->bFeaturesReg,
          IdeRegs->bSectorCountReg,
          IdeRegs->bSectorNumberReg,
          IdeRegs->bCylLowReg,
          IdeRegs->bCylHighReg,
          IdeRegs->bCommandReg,
          IdeRegs->bDriveHeadReg);

    TaskFile->Feature = IdeRegs->bFeaturesReg;
    TaskFile->SectorCount = IdeRegs->bSectorCountReg;
    TaskFile->LowLba = IdeRegs->bSectorNumberReg;
    TaskFile->MidLba = IdeRegs->bCylLowReg;
    TaskFile->HighLba = IdeRegs->bCylHighReg;
    TaskFile->Command = IdeRegs->bCommandReg;
    TaskFile->DriveSelect = IdeRegs->bDriveHeadReg;
    if (AtaFlags & ATA_FLAGS_48BIT_COMMAND)
    {
        Request->Flags |= REQUEST_FLAG_LBA48;

        IdeRegs = (PIDEREGS)&Srb->Cdb;
        TaskFile->FeatureEx = IdeRegs->bFeaturesReg;
        TaskFile->SectorCountEx = IdeRegs->bSectorCountReg;
        TaskFile->LowLbaEx = IdeRegs->bSectorNumberReg;
        TaskFile->MidLbaEx = IdeRegs->bCylLowReg;
        TaskFile->HighLbaEx = IdeRegs->bCylHighReg;
        TaskFile->Command = IdeRegs->bCommandReg;
        TaskFile->DriveSelect = IdeRegs->bDriveHeadReg;

    TRACE("TF: %02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          IdeRegs->bFeaturesReg,
          IdeRegs->bSectorCountReg,
          IdeRegs->bSectorNumberReg,
          IdeRegs->bCylLowReg,
          IdeRegs->bCylHighReg,
          IdeRegs->bCommandReg,
          IdeRegs->bDriveHeadReg);
    }

    return SRB_STATUS_PENDING;
}

static
ULONG
AtaReqTranslateSrb(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    ULONG SrbStatus;

    Request->Irp = Srb->OriginalRequest;

    switch (Srb->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            if (Srb->SrbFlags & SRB_FLAGS_PASSTHROUGH)
                SrbStatus = AtaReqPassthrough(ChannelExtension, DeviceExtension, Srb);
            else
                SrbStatus = AtaReqExecuteScsi(ChannelExtension, DeviceExtension, Srb);
            break;
        }

        case SRB_FUNCTION_IO_CONTROL:
        {
            if (SRB_GET_FLAGS(Srb) & SRB_FLAG_INTERNAL)
                SrbStatus = AtaReqPrepareTaskFile(ChannelExtension, DeviceExtension, Srb);
            else
                SrbStatus = AtaReqIoControl(ChannelExtension, DeviceExtension, Srb);
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
            ASSERT(0);
        }
    }

    return SrbStatus;
}

static
VOID
AtaFdoStartSrb(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    ULONG SrbStatus;

    SrbStatus = AtaReqTranslateSrb(ChannelExtension, DeviceExtension, Srb);
    if (SrbStatus == SRB_STATUS_PENDING)
    {
        SrbStatus = AtaReqDispatch(ChannelExtension, DeviceExtension);
    }

    if (SrbStatus != SRB_STATUS_PENDING)
    {
        if (SrbStatus == SRB_STATUS_INSUFFICIENT_RESOURCES)
        {
            Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
            Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            Srb->SrbStatus = SrbStatus;
        }

        ChannelExtension->Request.Flags |= REQUEST_FLAG_COMPLETED;
        KeInsertQueueDpc(&ChannelExtension->Dpc, (PVOID)(ULONG_PTR)1, NULL);
    }
}

static
VOID
AtaPdoQueueGetNextRequest(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension)
{
    PREQUEST_QUEUE_ENTRY QueueEntry;
    PIRP Irp;

    if (DeviceExtension->QueueFlags & QUEUE_FLAG_FROZEN)
        return;

    while (TRUE)
    {
        if (IsListEmpty(&DeviceExtension->RequestList))
            break;

        QueueEntry = (PREQUEST_QUEUE_ENTRY)RemoveHeadList(&DeviceExtension->RequestList);

        /* Clear our cancel routine */
        Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
        if (!IoSetCancelRoutine(Irp, NULL))
        {
            /* We're already canceled, reset the list entry to point to itself */
            InitializeListHead(&QueueEntry->ListEntry);
            continue;
        }

        InsertTailList(&ChannelExtension->RequestList, &QueueEntry->ListEntry);

        ChannelExtension->RequestBitmap |= ATA_REQUEST_MASK_FROM_DEVICE(QueueEntry->Context);
        break;
    }
}

static
VOID
AtaFdoQueueGetNextRequest(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    PREQUEST_QUEUE_ENTRY QueueEntry;
    PIRP Irp;
    PSCSI_REQUEST_BLOCK Srb;
    PIO_STACK_LOCATION IoStack;

    KeAcquireSpinLockAtDpcLevel(&ChannelExtension->QueueLock);

    AtaPdoQueueGetNextRequest(ChannelExtension, Request->DeviceExtension);

    if (IsListEmpty(&ChannelExtension->RequestList))
    {
        Request->Srb = NULL;

        KeReleaseSpinLockFromDpcLevel(&ChannelExtension->QueueLock);
        return;
    }

    QueueEntry = (PREQUEST_QUEUE_ENTRY)RemoveHeadList(&ChannelExtension->RequestList);

    ChannelExtension->RequestBitmap &=
        ~(ATA_REQUEST_MASK_FROM_DEVICE(Request->DeviceExtension) |
          ATA_REQUEST_MASK_FROM_DEVICE(QueueEntry->Context));

    Irp = IRP_FROM_QUEUE_ENTRY(QueueEntry);
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Srb = IoStack->Parameters.Scsi.Srb;
    ASSERT(Srb->OriginalRequest == Irp);

    Request->Srb = Srb;

    KeReleaseSpinLockFromDpcLevel(&ChannelExtension->QueueLock);

    AtaFdoStartSrb(ChannelExtension, QueueEntry->Context, Srb);
}

static
VOID
NTAPI
AtaPdoQueueCancelIo(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ _IRQL_uses_cancel_ PIRP Irp)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension;
    PATAPORT_DEVICE_EXTENSION DeviceExtension;
    KIRQL OldLevel;
    PREQUEST_QUEUE_ENTRY QueueEntry;
    PSCSI_REQUEST_BLOCK Srb;

    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    QueueEntry = QUEUE_ENTRY_FROM_IRP(Irp);
    DeviceExtension = QueueEntry->Context;

    Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
    ASSERT(Srb->OriginalRequest == Irp);

    ChannelExtension = DeviceExtension->ChannelExtension;

    KeAcquireSpinLock(&ChannelExtension->QueueLock, &OldLevel);
    RemoveEntryList(&QueueEntry->ListEntry);
    KeReleaseSpinLock(&ChannelExtension->QueueLock, OldLevel);

    Srb->SrbStatus = SRB_STATUS_ABORTED;
    Srb->InternalStatus = STATUS_CANCELLED;

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

static
VOID
AtaPdoQueueInsertSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PIRP Irp)
{
    PREQUEST_QUEUE_ENTRY QueueEntry;

    QueueEntry = QUEUE_ENTRY_FROM_IRP(Irp);
    QueueEntry->Context = DeviceExtension;

    InsertTailList(&DeviceExtension->RequestList, &QueueEntry->ListEntry);
}

static
ATA_QUEUE_RESULT
AtaPdoQueueAddSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PIRP Irp = Srb->OriginalRequest;

    /* The FDO queue is full, the request will stay on the PDO queue */
    AtaPdoQueueInsertSrb(DeviceExtension, Srb, Srb->OriginalRequest);

    /*
     * If the FDO queue is full, the requests may take a long period of time to process,
     * and therefore we have to do the cancellation ourselves.
     */
    (VOID)IoSetCancelRoutine(Irp, AtaPdoQueueCancelIo);
    if (Irp->Cancel)
    {
        /* This IRP has already been cancelled */
        if (IoSetCancelRoutine(Irp, NULL))
        {
            PREQUEST_QUEUE_ENTRY QueueEntry;

            /* Remove the IRP from the queue */
            QueueEntry = QUEUE_ENTRY_FROM_IRP(Irp);
            RemoveEntryList(&QueueEntry->ListEntry);

            return QueueItemCancelled;
        }
    }

    return QueueItemQueued;
}

static
ATA_QUEUE_RESULT
AtaQueueAddSrb(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PREQUEST_QUEUE_ENTRY QueueEntry;

    /* Check if the request should wait for the PDO queue to be unfrozen */
    if (DeviceExtension->QueueFlags & QUEUE_FLAG_FROZEN)
        goto ToPdoQueue;

    /* The FDO queue is empty */
    if (!ChannelExtension->Request.Srb)
    {
        /* An IDE channel can only deal with one active request at a time */
        ChannelExtension->Request.Srb = Srb;

        return QueueItemStarted;
    }

    /*
     * Put the SRB in queue if there are no requests from this device in the queue
     * to balance the load a bit within devices on that IDE channel.
     */
    if (ChannelExtension->RequestBitmap & ATA_REQUEST_MASK_FROM_DEVICE(DeviceExtension))
    {
ToPdoQueue:
        return AtaPdoQueueAddSrb(DeviceExtension, Srb);
    }

    ChannelExtension->RequestBitmap |= ATA_REQUEST_MASK_FROM_DEVICE(DeviceExtension);

    QueueEntry = QUEUE_ENTRY_FROM_IRP(Srb->OriginalRequest);
    QueueEntry->Context = DeviceExtension;
    InsertTailList(&ChannelExtension->RequestList, &QueueEntry->ListEntry);

    return QueueItemQueued;
}

DECLSPEC_NOINLINE_FROM_PAGED
NTSTATUS
AtaPdoStartSrb(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = DeviceExtension->ChannelExtension;
    KIRQL OldIrql;
    ATA_QUEUE_RESULT QueueResult;
    NTSTATUS Status;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    KeAcquireSpinLockAtDpcLevel(&ChannelExtension->QueueLock);
    QueueResult = AtaQueueAddSrb(ChannelExtension, DeviceExtension, Srb);
    KeReleaseSpinLockFromDpcLevel(&ChannelExtension->QueueLock);

    switch (QueueResult)
    {
        case QueueItemQueued:
        {
            Status = STATUS_PENDING;
            break;
        }

        case QueueItemCancelled:
        {
            Srb->SrbStatus = SRB_STATUS_ABORTED;
            Srb->InternalStatus = STATUS_CANCELLED;

            Status = STATUS_CANCELLED;
            break;
        }

        case QueueItemStarted:
        {
            AtaFdoStartSrb(DeviceExtension->ChannelExtension, DeviceExtension, Srb);

            Status = STATUS_PENDING;
            break;
        }

        default:
            ASSERT(FALSE);
            UNREACHABLE;
    }

    KeLowerIrql(OldIrql);

    return Status;
}

static
BOOLEAN
AtaPdoHandleIoControl(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ NTSTATUS* Status)
{
    PSRB_IO_CONTROL SrbControl = (PSRB_IO_CONTROL)Srb->DataBuffer;

    switch (SrbControl->ControlCode)
    {
        case IOCTL_SCSI_MINIPORT_SMART_VERSION:
            *Status = AtaPdoHandleMiniportSmartVersion(DeviceExtension->ChannelExtension, Srb);
            break;

        case IOCTL_SCSI_MINIPORT_IDENTIFY:
            *Status = AtaPdoHandleMiniportIdentify(DeviceExtension, Srb);
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
DECLSPEC_NOINLINE_FROM_PAGED
CODE_SEG("PAGE")
NTSTATUS
AtaPdoAttachReleaseDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PAGED_CODE();

    if (Srb->Function == SRB_FUNCTION_RELEASE_DEVICE)
    {
        DeviceExtension->DeviceClaimed = FALSE;

        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        return STATUS_SUCCESS;
    }

    if (DeviceExtension->DeviceClaimed)
    {
        Srb->SrbStatus = SRB_STATUS_BUSY;
        return STATUS_DEVICE_BUSY;
    }

    if (Srb->Function == SRB_FUNCTION_CLAIM_DEVICE)
    {
        DeviceExtension->DeviceClaimed = TRUE;
    }

    Srb->DataBuffer = DeviceExtension->Common.Self;

    Srb->SrbStatus = SRB_STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

static
NTSTATUS
AtaPdoDispatchScsi(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;

    Srb = IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb;
    ASSERT(Srb);
    ASSERT(Srb->OriginalRequest == Irp);

    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
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
            Status = AtaPdoAttachReleaseDevice(DeviceExtension, Srb);
            break;

        case SRB_FUNCTION_IO_CONTROL:
        {
            if (AtaPdoHandleIoControl(DeviceExtension, Srb, &Status))
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
            AtaScsiAddress = DeviceExtension->AtaScsiAddress;
            Srb->PathId = AtaScsiAddress.PathId;
            Srb->TargetId = AtaScsiAddress.TargetId;
            Srb->Lun = AtaScsiAddress.Lun;

            /* This field is used by the driver to mark internal requests */
            Srb->SrbExtension = NULL;

            /*
             * NOTE: Disk I/O requests need a lot of the kernel stack space.
             * We should avoid nesting several levels deep in the call chain.
             */
            Status = AtaPdoStartSrb(DeviceExtension, Srb);

            if (Status == STATUS_PENDING)
            {
                IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
                return Status;
            }
            break;
        }

        default:
            Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static
NTSTATUS
AtaFdoDispatchScsi(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;

    UNREFERENCED_PARAMETER(ChannelExtension);

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

static
BOOLEAN
AtaReqPollForComplete(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    UCHAR Status;
    BOOLEAN RequestCompleted;

    while (TRUE)
    {
        /* Do a quick check here for a busy */
        Status = ATA_READ(ChannelExtension->Registers.Status);
        ATA_WAIT_ON_BUSY(ChannelExtension, &Status, ATA_TIME_BUSY_POLL);

        /* Device is still busy, schedule a timer to retry the poll */
        if (Status & IDE_STATUS_BUSY)
        {
            RequestCompleted = FALSE;
            break;
        }

        RequestCompleted = AtaProcessRequest(ChannelExtension, Status, 0);
        if (RequestCompleted)
            break;

        /* Restart with default period */
        ChannelExtension->PollInterval = 0;
        ChannelExtension->PollCount = 0;
    }

    return RequestCompleted;
}

static
BOOLEAN
AtaReqPollForRetry(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    ULONG SrbStatus;

    ASSERT(Request->Srb);

    /* Retry once again */
    SrbStatus = AtaReqSendRequest(ChannelExtension);
    if (SrbStatus == SRB_STATUS_PENDING)
    {
        if (!(Request->Flags & REQUEST_FLAG_ASYNC_MODE))
            return TRUE;

        return FALSE;
    }

    switch (ChannelExtension->PollCount)
    {
        case 12: /* 500 ms */
        {
            ERR("Busy device\n");

            if (!AtaDevicePresent(ChannelExtension))
            {
                SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
            }
            break;
        }

        case 25: /* 1 sec */
        {
            ERR("Busy timeout\n");

            SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
            break;
        }

        default:
            break;
    }

    if (SrbStatus != SRB_STATUS_BUSY)
    {
        Request->SrbStatus = SrbStatus;

        Request->Flags |= REQUEST_FLAG_COMPLETED;
        KeInsertQueueDpc(&ChannelExtension->Dpc, NULL, NULL);
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
AtaPollingTimerDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = DeferredContext;
    BOOLEAN Done;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (!(ChannelExtension->Request.Flags & REQUEST_FLAG_POLLING_ACTIVE))
        return;

    ASSERT(ChannelExtension->Request.Srb);

    KeAcquireSpinLockAtDpcLevel(&ChannelExtension->ChannelLock);

    switch (ChannelExtension->PollState)
    {
        case POLL_REQUEST:
            Done = AtaReqPollForComplete(ChannelExtension);
            break;

        case POLL_RETRY:
            Done = AtaReqPollForRetry(ChannelExtension);
            break;

        DEFAULT_UNREACHABLE;
    }

    /* Retry after the time interval */
    if (!Done)
    {
        LARGE_INTEGER DueTime;
        ULONG MillisecondsPeriod;

        if (ChannelExtension->PollCount == 25)
        {
            ChannelExtension->PollCount = 0;

            if (ChannelExtension->PollInterval < (RTL_NUMBER_OF(AtapPollIntervalMs) - 1))
                ++ChannelExtension->PollInterval;
        }
        else
        {
            ++ChannelExtension->PollCount;
        }
        MillisecondsPeriod = AtapPollIntervalMs[ChannelExtension->PollInterval];

        DueTime.QuadPart = UInt32x32To64(MillisecondsPeriod, -10000);
        KeSetTimer(&ChannelExtension->PollingTimer,
                   DueTime,
                   &ChannelExtension->PollingTimerDpc);
    }

    KeReleaseSpinLockFromDpcLevel(&ChannelExtension->ChannelLock);
}

static
NTSTATUS
AtaSrbStatusToNtStatus(
    _In_ UCHAR SrbStatus)
{
    switch (SRB_STATUS(SrbStatus))
    {
        case SRB_STATUS_SUCCESS:
            return STATUS_SUCCESS;

        case SRB_STATUS_PENDING:
        {
            ASSERT(0);
            return STATUS_PENDING;
        }

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
            return STATUS_IO_DEVICE_ERROR;
    }

    return STATUS_IO_DEVICE_ERROR;
}

VOID
NTAPI
AtaDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = DeferredContext;
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    ATA_COMPLETION_STATUS CompletionStatus;
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Check if we had completed the request before */
    if (!(Request->Flags & REQUEST_FLAG_COMPLETED))
        return;

    /* Handle the simple case here */
    if (SystemArgument1)
    {
        Srb = Request->Srb;
        goto CompleteIrpSimple;
    }

    KeAcquireSpinLockAtDpcLevel(&ChannelExtension->ChannelLock);

    if (ChannelExtension->TimerCount != TIMER_STATE_TIMEDOUT)
    {
        /* Normal completion, clear the request timer */
        ChannelExtension->TimerCount = TIMER_STATE_STOPPED;
    }
    else
    {
        /* The request has timed out */
        Request->SrbStatus = SRB_STATUS_TIMEOUT;

        ERR("Timed out req %p Tid %u\n", Request->Srb, Request->Srb->TargetId);
    }

    KeReleaseSpinLockFromDpcLevel(&ChannelExtension->ChannelLock);

    if (Request->Flags & REQUEST_FLAG_REQUEST_SENSE)
    {
        PSCSI_REQUEST_BLOCK Srb = Request->Srb;

        Srb->SenseInfoBufferLength = Request->DataTransferLength;
        Request->DataTransferLength = Request->OldDataTransferLength;

        if (Request->SrbStatus == SRB_STATUS_SUCCESS)
        {
            Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
            Request->SrbStatus = SRB_STATUS_ERROR;
            Request->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }
        else
        {
            Srb->ScsiStatus = SCSISTAT_GOOD;
            Request->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;
        }
    }

    if (Request->Flags & REQUEST_FLAG_RESERVED_MAPPING)
    {
        MmUnmapReservedMapping(ChannelExtension->ReservedVaSpace,
                               IDEPORT_TAG,
                               Request->Mdl);
    }

    if (Request->Flags & REQUEST_FLAG_DMA_ENABLED)
    {
        PPCIIDE_INTERFACE PciIdeDma = &ChannelExtension->PciIdeInterface;

        PciIdeDma->AdapterObject->
            DmaOperations->PutScatterGatherList(PciIdeDma->AdapterObject,
                                                Request->SgList,
                                                !!(Request->Flags & REQUEST_FLAG_DATA_IN));
    }

    /* Prepare the SRB and IRP associated with the request */
    CompletionStatus = Request->Complete(ChannelExtension);

    switch (CompletionStatus)
    {
        case COMPLETE_IRP:
        {
            Srb = Request->Srb;
            Srb->DataTransferLength = Request->DataTransferLength;
            Srb->SrbStatus = Request->SrbStatus;

CompleteIrpSimple:
            Irp = Request->Irp;
            Irp->IoStatus.Information = Srb->DataTransferLength;
            Irp->IoStatus.Status = AtaSrbStatusToNtStatus(Srb->SrbStatus);

            /*
             * Start the next request on the queue.
             * It's important to do this before actually completing an IRP.
             */
            AtaFdoQueueGetNextRequest(ChannelExtension);

            /*
             * Complete the current request. A new SCSI device I/O request
             * might send immediately after the IRP completed.
             */
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
            break;
        }

        case COMPLETE_NO_IRP:
        {
            AtaFdoQueueGetNextRequest(ChannelExtension);
            break;
        }

        case COMPLETE_START_AGAIN:
        {
            ASSERT(Request->Mdl == NULL);
            ASSERT(!(Request->Flags & REQUEST_FLAG_DMA_ENABLED));

            /* Handle multiple ATA command sequences */
            Request->Flags &= ~REQUEST_FLAG_COMPLETED;
            AtaReqStartIo(ChannelExtension);
            break;
        }

        DEFAULT_UNREACHABLE;
    }
}

static
VOID
AtaReqTimeout(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    PATA_DEVICE_REQUEST Request = &ChannelExtension->Request;
    KIRQL OldIrql;
    UCHAR Status;

    OldIrql = KeAcquireInterruptSpinLock(ChannelExtension->InterruptObject);

    /* The interrupt handler has just completed the request */
    if (Request->Flags & REQUEST_FLAG_COMPLETED)
    {
        KeReleaseInterruptSpinLock(ChannelExtension->InterruptObject, OldIrql);
        return;
    }

    /* Try to complete the request manually. It's possible we lost the completion interrupt here */
    if (Request->Flags & REQUEST_FLAG_ASYNC_MODE)
    {
        Status = ATA_READ(ChannelExtension->Registers.Status);
        if (!(Status & IDE_STATUS_BUSY))
        {
            /* Run the interrupt handler */
            ChannelExtension->ServiceRoutine(NULL, ChannelExtension);

            if (Request->Flags & REQUEST_FLAG_COMPLETED)
            {
                WARN("Recovered from error\n");

                TRACE("Status 0x%02x\n", Request->Status);
                TRACE("Error 0x%02x\n", Request->Error);
                KeReleaseInterruptSpinLock(ChannelExtension->InterruptObject, OldIrql);
                return;
            }

            TRACE("Not completed\n");
        }
    }

    /* Make sure that the request handler won't be invoked from the interrupt handler */
    ChannelExtension->CommandFlags = CMD_FLAG_NONE;

    Request->Flags &= ~REQUEST_FLAG_POLLING_ACTIVE;
    Request->Flags |= REQUEST_FLAG_COMPLETED;

    KeReleaseInterruptSpinLock(ChannelExtension->InterruptObject, OldIrql);

    if (!(Request->Flags & REQUEST_FLAG_ASYNC_MODE))
    {
        KeCancelTimer(&ChannelExtension->PollingTimer);
    }

    Request->Status = IDE_STATUS_ERROR;
    Request->Error = IDE_ERROR_COMMAND_ABORTED;

    KeInsertQueueDpc(&ChannelExtension->Dpc, NULL, NULL);
}

VOID
NTAPI
AtaIoTimer(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PATAPORT_CHANNEL_EXTENSION ChannelExtension = Context;

    KeAcquireSpinLockAtDpcLevel(&ChannelExtension->ChannelLock);

    if (ChannelExtension->TimerCount > 0)
    {
        if (--ChannelExtension->TimerCount == 0)
        {
            ChannelExtension->TimerCount = TIMER_STATE_TIMEDOUT;

            AtaReqTimeout(ChannelExtension);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&ChannelExtension->ChannelLock);
}
