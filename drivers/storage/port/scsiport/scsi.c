/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     SCSI Port driver SCSI requests handling
 * COPYRIGHT:   Eric Kohl (eric.kohl@reactos.org)
 *              Aleksey Bragin (aleksey@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "scsiport.h"

#define NDEBUG
#include <debug.h>


static
NTSTATUS
SpiStatusSrbToNt(
    _In_ UCHAR SrbStatus)
{
    switch (SRB_STATUS(SrbStatus))
    {
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

static
NTSTATUS
SpiHandleAttachRelease(
    _In_ PSCSI_PORT_LUN_EXTENSION LunExtension,
    _Inout_ PIRP Irp)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension =
        LunExtension->Common.LowerDevice->DeviceExtension;
    PDEVICE_OBJECT DeviceObject;
    KIRQL Irql;

    /* Get pointer to the SRB */
    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK Srb = IrpStack->Parameters.Scsi.Srb;

    /* Get spinlock */
    KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

    /* Release, if asked */
    if (Srb->Function == SRB_FUNCTION_RELEASE_DEVICE)
    {
        LunExtension->DeviceClaimed = FALSE;
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
        Srb->SrbStatus = SRB_STATUS_SUCCESS;

        return STATUS_SUCCESS;
    }

    /* Attach, if not already claimed */
    if (LunExtension->DeviceClaimed)
    {
        KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
        Srb->SrbStatus = SRB_STATUS_BUSY;

        return STATUS_DEVICE_BUSY;
    }

    /* Save the device object */
    DeviceObject = LunExtension->Common.DeviceObject;

    if (Srb->Function == SRB_FUNCTION_CLAIM_DEVICE)
        LunExtension->DeviceClaimed = TRUE;

    if (Srb->Function == SRB_FUNCTION_ATTACH_DEVICE)
        LunExtension->Common.DeviceObject = Srb->DataBuffer;

    Srb->DataBuffer = DeviceObject;

    KeReleaseSpinLock(&DeviceExtension->SpinLock, Irql);
    Srb->SrbStatus = SRB_STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

/**********************************************************************
 * NAME                         INTERNAL
 *  ScsiPortDispatchScsi
 *
 * DESCRIPTION
 *  Answer requests for SCSI calls
 *
 * RUN LEVEL
 *  PASSIVE_LEVEL
 *
 * ARGUMENTS
 *  Standard dispatch arguments
 *
 * RETURNS
 *  NTSTATUS
 */

NTSTATUS
NTAPI
ScsiPortDispatchScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PSCSI_PORT_DEVICE_EXTENSION portExt;
    PSCSI_PORT_LUN_EXTENSION lunExt;
    PIO_STACK_LOCATION Stack;
    PSCSI_REQUEST_BLOCK Srb;
    KIRQL Irql;
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP NextIrp, IrpList;
    PKDEVICE_QUEUE_ENTRY Entry;

    DPRINT("ScsiPortDispatchScsi(DeviceObject %p  Irp %p)\n", DeviceObject, Irp);

    Stack = IoGetCurrentIrpStackLocation(Irp);
    Srb = Stack->Parameters.Scsi.Srb;
    lunExt = DeviceObject->DeviceExtension;
    ASSERT(!lunExt->Common.IsFDO);
    portExt = lunExt->Common.LowerDevice->DeviceExtension;

    if (Srb == NULL)
    {
        DPRINT1("ScsiPortDispatchScsi() called with Srb = NULL!\n");
        Status = STATUS_UNSUCCESSFUL;

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    DPRINT("Srb: %p, Srb->Function: %lu\n", Srb, Srb->Function);

    Srb->PathId = lunExt->PathId;
    Srb->TargetId = lunExt->TargetId;
    Srb->Lun = lunExt->Lun;

    if (lunExt == NULL)
    {
        DPRINT("ScsiPortDispatchScsi() called with an invalid LUN\n");
        Status = STATUS_NO_SUCH_DEVICE;

        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    switch (Srb->Function)
    {
        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:
            DPRINT("  SRB_FUNCTION_SHUTDOWN or FLUSH\n");
            if (portExt->CachesData == FALSE)
            {
                /* All success here */
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
            /* Fall through to a usual execute operation */

        case SRB_FUNCTION_EXECUTE_SCSI:
        case SRB_FUNCTION_IO_CONTROL:
            DPRINT("  SRB_FUNCTION_EXECUTE_SCSI or SRB_FUNCTION_IO_CONTROL\n");
            /* Mark IRP as pending in all cases */
            IoMarkIrpPending(Irp);

            if (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
            {
                /* Start IO directly */
                IoStartPacket(portExt->Common.DeviceObject, Irp, NULL, NULL);
            }
            else
            {
                KIRQL oldIrql;

                /* We need to be at DISPATCH_LEVEL */
                KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

                /* Insert IRP into the queue */
                if (!KeInsertByKeyDeviceQueue(&lunExt->DeviceQueue,
                                              &Irp->Tail.Overlay.DeviceQueueEntry,
                                              Srb->QueueSortKey))
                {
                    /* It means the queue is empty, and we just start this request */
                    IoStartPacket(portExt->Common.DeviceObject, Irp, NULL, NULL);
                }

                /* Back to the old IRQL */
                KeLowerIrql(oldIrql);
            }
            return STATUS_PENDING;

        case SRB_FUNCTION_CLAIM_DEVICE:
        case SRB_FUNCTION_ATTACH_DEVICE:
            DPRINT("  SRB_FUNCTION_CLAIM_DEVICE or ATTACH\n");

            /* Reference device object and keep the device object */
            Status = SpiHandleAttachRelease(lunExt, Irp);
            break;

        case SRB_FUNCTION_RELEASE_DEVICE:
            DPRINT("  SRB_FUNCTION_RELEASE_DEVICE\n");

            /* Dereference device object and clear the device object */
            Status = SpiHandleAttachRelease(lunExt, Irp);
            break;

        case SRB_FUNCTION_RELEASE_QUEUE:
            DPRINT("  SRB_FUNCTION_RELEASE_QUEUE\n");

            /* Guard with the spinlock */
            KeAcquireSpinLock(&portExt->SpinLock, &Irql);

            if (!(lunExt->Flags & LUNEX_FROZEN_QUEUE))
            {
                DPRINT("Queue is not frozen really\n");

                KeReleaseSpinLock(&portExt->SpinLock, Irql);
                Srb->SrbStatus = SRB_STATUS_SUCCESS;
                Status = STATUS_SUCCESS;
                break;

            }

            /* Unfreeze the queue */
            lunExt->Flags &= ~LUNEX_FROZEN_QUEUE;

            if (lunExt->SrbInfo.Srb == NULL)
            {
                /* Get next logical unit request */
                SpiGetNextRequestFromLun(portExt, lunExt);

                /* SpiGetNextRequestFromLun() releases the spinlock */
                KeLowerIrql(Irql);
            }
            else
            {
                DPRINT("The queue has active request\n");
                KeReleaseSpinLock(&portExt->SpinLock, Irql);
            }

            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            Status = STATUS_SUCCESS;
            break;

        case SRB_FUNCTION_FLUSH_QUEUE:
            DPRINT("  SRB_FUNCTION_FLUSH_QUEUE\n");

            /* Guard with the spinlock */
            KeAcquireSpinLock(&portExt->SpinLock, &Irql);

            if (!(lunExt->Flags & LUNEX_FROZEN_QUEUE))
            {
                DPRINT("Queue is not frozen really\n");

                KeReleaseSpinLock(&portExt->SpinLock, Irql);
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            /* Make sure there is no active request */
            ASSERT(lunExt->SrbInfo.Srb == NULL);

            /* Compile a list from the device queue */
            IrpList = NULL;
            while ((Entry = KeRemoveDeviceQueue(&lunExt->DeviceQueue)) != NULL)
            {
                NextIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DeviceQueueEntry);

                /* Get the Srb */
                Stack = IoGetCurrentIrpStackLocation(NextIrp);
                Srb = Stack->Parameters.Scsi.Srb;

                /* Set statuse */
                Srb->SrbStatus = SRB_STATUS_REQUEST_FLUSHED;
                NextIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;

                /* Add then to the list */
                NextIrp->Tail.Overlay.ListEntry.Flink = (PLIST_ENTRY)IrpList;
                IrpList = NextIrp;
            }

            /* Unfreeze the queue */
            lunExt->Flags &= ~LUNEX_FROZEN_QUEUE;

            /* Release the spinlock */
            KeReleaseSpinLock(&portExt->SpinLock, Irql);

            /* Complete those requests */
            while (IrpList)
            {
                NextIrp = IrpList;
                IrpList = (PIRP)NextIrp->Tail.Overlay.ListEntry.Flink;

                IoCompleteRequest(NextIrp, 0);
            }

            Status = STATUS_SUCCESS;
            break;

        default:
            DPRINT1("SRB function not implemented (Function %lu)\n", Srb->Function);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

VOID
SpiGetNextRequestFromLun(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PSCSI_PORT_LUN_EXTENSION LunExtension)
{
    PIO_STACK_LOCATION IrpStack;
    PIRP NextIrp;
    PKDEVICE_QUEUE_ENTRY Entry;
    PSCSI_REQUEST_BLOCK Srb;


    /* If LUN is not active or queue is more than maximum allowed  */
    if (LunExtension->QueueCount >= LunExtension->MaxQueueCount ||
        !(LunExtension->Flags & SCSI_PORT_LU_ACTIVE))
    {
        /* Release the spinlock and exit */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        return;
    }

    /* Check if we can get a next request */
    if (LunExtension->Flags &
        (LUNEX_NEED_REQUEST_SENSE | LUNEX_BUSY |
         LUNEX_FULL_QUEUE | LUNEX_FROZEN_QUEUE | LUNEX_REQUEST_PENDING))
    {
        /* Pending requests can only be started if the queue is empty */
        if (IsListEmpty(&LunExtension->SrbInfo.Requests) &&
            !(LunExtension->Flags &
              (LUNEX_BUSY | LUNEX_FROZEN_QUEUE | LUNEX_FULL_QUEUE | LUNEX_NEED_REQUEST_SENSE)))
        {
            /* Make sure we have SRB */
            ASSERT(LunExtension->SrbInfo.Srb == NULL);

            /* Clear active and pending flags */
            LunExtension->Flags &= ~(LUNEX_REQUEST_PENDING | SCSI_PORT_LU_ACTIVE);

            /* Get next Irp, and clear pending requests list */
            NextIrp = LunExtension->PendingRequest;
            LunExtension->PendingRequest = NULL;

            /* Set attempt counter to zero */
            LunExtension->AttemptCount = 0;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Start the next pending request */
            IoStartPacket(DeviceExtension->Common.DeviceObject, NextIrp, (PULONG)NULL, NULL);

            return;
        }
        else
        {
            /* Release the spinlock, without clearing any flags and exit */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            return;
        }
    }

    /* Reset active flag */
    LunExtension->Flags &= ~SCSI_PORT_LU_ACTIVE;

    /* Set attempt counter to zero */
    LunExtension->AttemptCount = 0;

    /* Remove packet from the device queue */
    Entry = KeRemoveByKeyDeviceQueue(&LunExtension->DeviceQueue, LunExtension->SortKey);

    if (Entry != NULL)
    {
        /* Get pointer to the next irp */
        NextIrp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DeviceQueueEntry);

        /* Get point to the SRB */
        IrpStack = IoGetCurrentIrpStackLocation(NextIrp);
        Srb = (PSCSI_REQUEST_BLOCK)IrpStack->Parameters.Others.Argument1;

        /* Set new key*/
        LunExtension->SortKey = Srb->QueueSortKey;
        LunExtension->SortKey++;

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        /* Start the next pending request */
        IoStartPacket(DeviceExtension->Common.DeviceObject, NextIrp, (PULONG)NULL, NULL);
    }
    else
    {
        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }
}

IO_COMPLETION_ROUTINE SpiSenseCompletionRoutine;

NTSTATUS
NTAPI
SpiSenseCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_opt_ PVOID Context)
{
    PIO_STACK_LOCATION ioStack = IoGetNextIrpStackLocation(Irp);
    PSCSI_PORT_LUN_EXTENSION lunExt = ioStack->DeviceObject->DeviceExtension;
    PSCSI_PORT_DEVICE_EXTENSION portExt = lunExt->Common.LowerDevice->DeviceExtension;
    PSCSI_REQUEST_BLOCK Srb = (PSCSI_REQUEST_BLOCK)Context;
    PSCSI_REQUEST_BLOCK InitialSrb;
    PIRP InitialIrp;

    DPRINT("SpiCompletionRoutine() entered, IRP %p \n", Irp);

    if ((Srb->Function == SRB_FUNCTION_RESET_BUS) ||
        (Srb->Function == SRB_FUNCTION_ABORT_COMMAND))
    {
        /* Deallocate SRB and IRP and exit */
        ExFreePool(Srb);
        IoFreeIrp(Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get a pointer to the SRB and IRP which were initially sent */
    InitialSrb = *((PVOID *)(Srb+1));
    InitialIrp = InitialSrb->OriginalRequest;

    if ((SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS) ||
        (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN))
    {
        /* Sense data is OK */
        InitialSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;

        /* Set length to be the same */
        InitialSrb->SenseInfoBufferLength = (UCHAR)Srb->DataTransferLength;
    }

    /* Make sure initial SRB's queue is frozen */
    ASSERT(InitialSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN);

    // The queue is frozen, but the SRB had a SRB_FLAGS_NO_QUEUE_FREEZE => unfreeze the queue
    if ((InitialSrb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE) &&
        (InitialSrb->SrbStatus & SRB_STATUS_QUEUE_FROZEN))
    {
        KIRQL irql;

        KeAcquireSpinLock(&portExt->SpinLock, &irql);

        ASSERT(lunExt->Flags & LUNEX_FROZEN_QUEUE);

        lunExt->Flags &= ~LUNEX_FROZEN_QUEUE;
        lunExt->Flags &= ~LUNEX_NEED_REQUEST_SENSE;

        // SpiGetNextRequestFromLun releases the lock
        SpiGetNextRequestFromLun(portExt, lunExt);
        KeLowerIrql(irql);

        InitialSrb->SrbStatus &= ~SRB_STATUS_QUEUE_FROZEN;
    }

    /* Complete this request */
    IoCompleteRequest(InitialIrp, IO_DISK_INCREMENT);

    /* Deallocate everything (internal) */
    ExFreePool(Srb);

    if (Irp->MdlAddress != NULL)
    {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static
VOID
SpiSendRequestSense(
    _In_ PSCSI_PORT_LUN_EXTENSION LunExtension,
    _In_ PSCSI_REQUEST_BLOCK InitialSrb)
{
    PSCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    LARGE_INTEGER LargeInt;
    PVOID *Ptr;

    DPRINT("SpiSendRequestSense() entered, InitialSrb %p\n", InitialSrb);

    /* Allocate Srb */
    Srb = ExAllocatePoolWithTag(NonPagedPool, sizeof(SCSI_REQUEST_BLOCK) + sizeof(PVOID), TAG_SCSIPORT);
    RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

    /* Allocate IRP */
    LargeInt.QuadPart = (LONGLONG) 1;
    Irp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ,
                                        LunExtension->Common.DeviceObject,
                                        InitialSrb->SenseInfoBuffer,
                                        InitialSrb->SenseInfoBufferLength,
                                        &LargeInt,
                                        NULL);

    IoSetCompletionRoutine(Irp,
                           SpiSenseCompletionRoutine,
                           Srb,
                           TRUE,
                           TRUE,
                           TRUE);

    if (!Srb)
    {
        DPRINT("SpiSendRequestSense() failed, Srb %p\n", Srb);
        return;
    }

    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->MajorFunction = IRP_MJ_SCSI;

    /* Put Srb address into Irp... */
    IrpStack->Parameters.Others.Argument1 = (PVOID)Srb;

    /* ...and vice versa */
    Srb->OriginalRequest = Irp;

    /* Save Srb */
    Ptr = (PVOID *)(Srb+1);
    *Ptr = InitialSrb;

    /* Build CDB for REQUEST SENSE */
    Srb->CdbLength = 6;
    Cdb = (PCDB)Srb->Cdb;

    Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
    Cdb->CDB6INQUIRY.LogicalUnitNumber = 0;
    Cdb->CDB6INQUIRY.Reserved1 = 0;
    Cdb->CDB6INQUIRY.PageCode = 0;
    Cdb->CDB6INQUIRY.IReserved = 0;
    Cdb->CDB6INQUIRY.AllocationLength = (UCHAR)InitialSrb->SenseInfoBufferLength;
    Cdb->CDB6INQUIRY.Control = 0;

    /* Set address */
    Srb->TargetId = InitialSrb->TargetId;
    Srb->Lun = InitialSrb->Lun;
    Srb->PathId = InitialSrb->PathId;

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    /* Timeout will be 2 seconds */
    Srb->TimeOutValue = 2;

    /* No auto request sense */
    Srb->SenseInfoBufferLength = 0;
    Srb->SenseInfoBuffer = NULL;

    /* Set necessary flags */
    Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_BYPASS_FROZEN_QUEUE |
                    SRB_FLAGS_DISABLE_DISCONNECT;

    // pass some InitialSrb flags
    if (InitialSrb->SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER)
        Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    if (InitialSrb->SrbFlags & SRB_FLAGS_BYPASS_LOCKED_QUEUE)
        Srb->SrbFlags |= SRB_FLAGS_BYPASS_LOCKED_QUEUE;

    if (InitialSrb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE)
        Srb->SrbFlags |= SRB_FLAGS_NO_QUEUE_FREEZE;

    Srb->DataBuffer = InitialSrb->SenseInfoBuffer;

    /* Fill the transfer length */
    Srb->DataTransferLength = InitialSrb->SenseInfoBufferLength;

    /* Clear statuses */
    Srb->ScsiStatus = Srb->SrbStatus = 0;
    Srb->NextSrb = 0;

    /* Call the driver */
    (VOID)IoCallDriver(LunExtension->Common.DeviceObject, Irp);

    DPRINT("SpiSendRequestSense() done\n");
}


static
VOID
SpiProcessCompletedRequest(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PSCSI_REQUEST_BLOCK_INFO SrbInfo,
    _Out_ PBOOLEAN NeedToCallStartIo)
{
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    LONG Result;
    PIRP Irp;
    //ULONG SequenceNumber;

    Srb = SrbInfo->Srb;
    Irp = Srb->OriginalRequest;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* Get Lun extension */
    LunExtension = IoStack->DeviceObject->DeviceExtension;
    ASSERT(LunExtension && !LunExtension->Common.IsFDO);

    if (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION &&
        DeviceExtension->MapBuffers &&
        Irp->MdlAddress)
    {
        /* MDL is shared if transfer is broken into smaller parts */
        Srb->DataBuffer = (PCCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress) +
            ((PCCHAR)Srb->DataBuffer - SrbInfo->DataOffset);

        /* In case of data going in, flush the buffers */
        if (Srb->SrbFlags & SRB_FLAGS_DATA_IN)
        {
            KeFlushIoBuffers(Irp->MdlAddress,
                             TRUE,
                             FALSE);
        }
    }

    /* Flush adapter if needed */
    if (SrbInfo->BaseOfMapRegister)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Clear the request */
    SrbInfo->Srb = NULL;

    /* If disconnect is disabled... */
    if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
    {
        /* Acquire the spinlock since we mess with flags */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* Set corresponding flag */
        DeviceExtension->Flags |= SCSI_PORT_DISCONNECT_ALLOWED;

        /* Clear the timer if needed */
        if (!(DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET))
            DeviceExtension->TimerCount = -1;

        /* Spinlock is not needed anymore */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        if (!(DeviceExtension->Flags & SCSI_PORT_REQUEST_PENDING) &&
            !(DeviceExtension->Flags & SCSI_PORT_DEVICE_BUSY) &&
            !(*NeedToCallStartIo))
        {
            /* We're not busy, but we have a request pending */
            IoStartNextPacket(DeviceExtension->Common.DeviceObject, FALSE);
        }
    }

    /* Scatter/gather */
    if (Srb->SrbFlags & SRB_FLAGS_SGLIST_FROM_POOL)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Acquire spinlock (we're freeing SrbExtension) */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    /* Free it (if needed) */
    if (Srb->SrbExtension)
    {
        if (Srb->SenseInfoBuffer != NULL && DeviceExtension->SupportsAutoSense)
        {
            ASSERT(Srb->SenseInfoBuffer == NULL || SrbInfo->SaveSenseRequest != NULL);

            if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
            {
                /* Copy sense data to the buffer */
                RtlCopyMemory(SrbInfo->SaveSenseRequest,
                              Srb->SenseInfoBuffer,
                              Srb->SenseInfoBufferLength);
            }

            /* And restore the pointer */
            Srb->SenseInfoBuffer = SrbInfo->SaveSenseRequest;
        }

        /* Put it into the free srb extensions list */
        *((PVOID *)Srb->SrbExtension) = DeviceExtension->FreeSrbExtensions;
        DeviceExtension->FreeSrbExtensions = Srb->SrbExtension;
    }

    /* Save transfer length in the IRP */
    Irp->IoStatus.Information = Srb->DataTransferLength;

    //SequenceNumber = SrbInfo->SequenceNumber;
    SrbInfo->SequenceNumber = 0;

    /* Decrement the queue count */
    LunExtension->QueueCount--;

    /* Free Srb, if needed*/
    if (Srb->QueueTag != SP_UNTAGGED)
    {
        /* Put it into the free list */
        SrbInfo->Requests.Blink = NULL;
        SrbInfo->Requests.Flink = (PLIST_ENTRY)DeviceExtension->FreeSrbInfo;
        DeviceExtension->FreeSrbInfo = SrbInfo;
    }

    /* SrbInfo is not used anymore */
    SrbInfo = NULL;

    if (DeviceExtension->Flags & SCSI_PORT_REQUEST_PENDING)
    {
        /* Clear the flag */
        DeviceExtension->Flags &= ~SCSI_PORT_REQUEST_PENDING;

        /* Note the caller about StartIo */
        *NeedToCallStartIo = TRUE;
    }

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SUCCESS)
    {
        /* Start the packet */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        if (!(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) &&
            LunExtension->RequestTimeout == -1)
        {
            /* Start the next packet */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);
        }
        else
        {
            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        }

        DPRINT("IoCompleting request IRP 0x%p\n", Irp);

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);

        /* Decrement number of active requests, and analyze the result */
        Result = InterlockedDecrement(&DeviceExtension->ActiveRequestCounter);

        if (Result < 0 &&
            !DeviceExtension->MapRegisters &&
            DeviceExtension->AdapterObject != NULL)
        {
            /* Nullify map registers */
            DeviceExtension->MapRegisterBase = NULL;
            IoFreeAdapterChannel(DeviceExtension->AdapterObject);
        }

         /* Exit, we're done */
        return;
    }

    /* Decrement number of active requests, and analyze the result */
    Result = InterlockedDecrement(&DeviceExtension->ActiveRequestCounter);

    if (Result < 0 &&
        !DeviceExtension->MapRegisters &&
        DeviceExtension->AdapterObject != NULL)
    {
        /* Result is negative, so this is a slave, free map registers */
        DeviceExtension->MapRegisterBase = NULL;
        IoFreeAdapterChannel(DeviceExtension->AdapterObject);
    }

    /* Convert status */
    Irp->IoStatus.Status = SpiStatusSrbToNt(Srb->SrbStatus);

    /* It's not a bypass, it's busy or the queue is full? */
    if ((Srb->ScsiStatus == SCSISTAT_BUSY ||
         Srb->SrbStatus == SRB_STATUS_BUSY ||
         Srb->ScsiStatus == SCSISTAT_QUEUE_FULL) &&
         !(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE))
    {

        DPRINT("Busy SRB status %x\n", Srb->SrbStatus);

        /* Requeue, if needed */
        if (LunExtension->Flags & (LUNEX_FROZEN_QUEUE | LUNEX_BUSY))
        {
            DPRINT("it's being requeued\n");

            Srb->SrbStatus = SRB_STATUS_PENDING;
            Srb->ScsiStatus = 0;

            if (!KeInsertByKeyDeviceQueue(&LunExtension->DeviceQueue,
                                          &Irp->Tail.Overlay.DeviceQueueEntry,
                                          Srb->QueueSortKey))
            {
                /* It's a big f.ck up if we got here */
                Srb->SrbStatus = SRB_STATUS_ERROR;
                Srb->ScsiStatus = SCSISTAT_BUSY;

                ASSERT(FALSE);
                goto Error;
            }

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        }
        else if (LunExtension->AttemptCount++ < 20)
        {
            /* LUN is still busy */
            Srb->ScsiStatus = 0;
            Srb->SrbStatus = SRB_STATUS_PENDING;

            LunExtension->BusyRequest = Irp;
            LunExtension->Flags |= LUNEX_BUSY;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        }
        else
        {
Error:
            /* Freeze the queue*/
            Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
            LunExtension->Flags |= LUNEX_FROZEN_QUEUE;

            /* "Unfull" the queue */
            LunExtension->Flags &= ~LUNEX_FULL_QUEUE;

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Return status that the device is not ready */
            Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
            IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        }

        return;
    }

    /* Start the next request, if LUN is idle, and this is sense request */
    if (((Srb->ScsiStatus != SCSISTAT_CHECK_CONDITION) ||
        (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) ||
        !Srb->SenseInfoBuffer || !Srb->SenseInfoBufferLength)
        && (Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        if (LunExtension->RequestTimeout == -1)
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);
        else
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }
    else
    {
        /* Freeze the queue */
        Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
        LunExtension->Flags |= LUNEX_FROZEN_QUEUE;

        /* Do we need a request sense? */
        if (Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
            !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
            Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength)
        {
            /* If LUN is busy, we have to requeue it in order to allow request sense */
            if (LunExtension->Flags & LUNEX_BUSY)
            {
                DPRINT("Requeuing busy request to allow request sense\n");

                if (!KeInsertByKeyDeviceQueue(&LunExtension->DeviceQueue,
                    &LunExtension->BusyRequest->Tail.Overlay.DeviceQueueEntry,
                    Srb->QueueSortKey))
                {
                    /* We should never get here */
                    ASSERT(FALSE);

                    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
                    return;

                }

                /* Clear busy flags */
                LunExtension->Flags &= ~(LUNEX_FULL_QUEUE | LUNEX_BUSY);
            }

            /* Release the spinlock */
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

            /* Send RequestSense */
            SpiSendRequestSense(LunExtension, Srb);

            /* Exit */
            return;
        }

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    /* Complete the request */
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
}

BOOLEAN
NTAPI
ScsiPortStartPacket(
    _In_ PVOID Context)
{
    PIO_STACK_LOCATION IrpStack;
    PSCSI_REQUEST_BLOCK Srb;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    PSCSI_PORT_COMMON_EXTENSION CommonExtension = DeviceObject->DeviceExtension;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    BOOLEAN Result;
    BOOLEAN StartTimer;

    DPRINT("ScsiPortStartPacket() called\n");

    IrpStack = IoGetCurrentIrpStackLocation(DeviceObject->CurrentIrp);
    Srb = IrpStack->Parameters.Scsi.Srb;

    if (CommonExtension->IsFDO) // IsFDO
    {
        DeviceExtension = DeviceObject->DeviceExtension;
        LunExtension = IrpStack->DeviceObject->DeviceExtension;
        ASSERT(LunExtension && !LunExtension->Common.IsFDO);
    }
    else
    {
        LunExtension = DeviceObject->DeviceExtension;
        DeviceExtension = LunExtension->Common.LowerDevice->DeviceExtension;
    }

    /* Check if we are in a reset state */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_RESET)
    {
        /* Mark the we've got requests while being in the reset state */
        DeviceExtension->InterruptData.Flags |= SCSI_PORT_RESET_REQUEST;
        return TRUE;
    }

    /* Set the time out value */
    DeviceExtension->TimerCount = Srb->TimeOutValue;

    /* We are busy */
    DeviceExtension->Flags |= SCSI_PORT_DEVICE_BUSY;

    if (LunExtension->RequestTimeout != -1)
    {
        /* Timer already active */
        StartTimer = FALSE;
    }
    else
    {
        /* It hasn't been initialized yet */
        LunExtension->RequestTimeout = Srb->TimeOutValue;
        StartTimer = TRUE;
    }

    if (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
    {
        /* Handle bypass-requests */

        /* Is this an abort request? */
        if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
        {
            /* Get pointer to SRB info structure */
            SrbInfo = SpiGetSrbData(DeviceExtension, LunExtension, Srb->QueueTag);

            /* Check if the request is still "active" */
            if (SrbInfo == NULL ||
                SrbInfo->Srb == NULL ||
                !(SrbInfo->Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE))
            {
                /* It's not, mark it as active then */
                Srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

                if (StartTimer)
                    LunExtension->RequestTimeout = -1;

                DPRINT("Request has been already completed, but abort request came\n");
                Srb->SrbStatus = SRB_STATUS_ABORT_FAILED;

                /* Notify about request complete */
                ScsiPortNotification(RequestComplete,
                                     DeviceExtension->MiniPortDeviceExtension,
                                     Srb);

                /* and about readiness for the next request */
                ScsiPortNotification(NextRequest,
                                     DeviceExtension->MiniPortDeviceExtension);

                /* They might ask for some work, so queue the DPC for them */
                IoRequestDpc(DeviceExtension->Common.DeviceObject, NULL, NULL);

                /* We're done in this branch */
                return TRUE;
            }
        }
        else
        {
            /* Add number of queued requests */
            LunExtension->QueueCount++;
        }

        /* Bypass requests don't need request sense */
        LunExtension->Flags &= ~LUNEX_NEED_REQUEST_SENSE;

        /* Is disconnect disabled for this request? */
        if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        {
            /* Set the corresponding flag */
            DeviceExtension->Flags &= ~SCSI_PORT_DISCONNECT_ALLOWED;
        }

        /* Transfer timeout value from Srb to Lun */
        LunExtension->RequestTimeout = Srb->TimeOutValue;
    }
    else
    {
        if (Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
        {
            /* It's a disconnect, so no more requests can go */
            DeviceExtension->Flags &= ~SCSI_PORT_DISCONNECT_ALLOWED;
        }

        LunExtension->Flags |= SCSI_PORT_LU_ACTIVE;

        /* Increment queue count */
        LunExtension->QueueCount++;

        /* If it's tagged - special thing */
        if (Srb->QueueTag != SP_UNTAGGED)
        {
            SrbInfo = &DeviceExtension->SrbInfo[Srb->QueueTag - 1];

            /* Chek for consistency */
            ASSERT(SrbInfo->Requests.Blink == NULL);

            /* Insert it into the list of requests */
            InsertTailList(&LunExtension->SrbInfo.Requests, &SrbInfo->Requests);
        }
    }

    /* Mark this Srb active */
    Srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;

    /* Call HwStartIo routine */
    Result = DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
                                        Srb);

    /* If notification is needed, then request a DPC */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
        IoRequestDpc(DeviceExtension->Common.DeviceObject, NULL, NULL);

    return Result;
}

BOOLEAN
NTAPI
SpiSaveInterruptData(IN PVOID Context)
{
    PSCSI_PORT_SAVE_INTERRUPT InterruptContext = Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo, NextSrbInfo;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    BOOLEAN IsTimed;

    /* Get pointer to the device extension */
    DeviceExtension = InterruptContext->DeviceExtension;

    /* If we don't have anything pending - return */
    if (!(DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED))
        return FALSE;

    /* Actually save the interrupt data */
    *InterruptContext->InterruptData = DeviceExtension->InterruptData;

    /* Clear the data stored in the device extension */
    DeviceExtension->InterruptData.Flags &=
        (SCSI_PORT_RESET | SCSI_PORT_RESET_REQUEST | SCSI_PORT_DISABLE_INTERRUPTS);
    DeviceExtension->InterruptData.CompletedAbort = NULL;
    DeviceExtension->InterruptData.ReadyLun = NULL;
    DeviceExtension->InterruptData.CompletedRequests = NULL;

    /* Loop through the list of completed requests */
    SrbInfo = InterruptContext->InterruptData->CompletedRequests;

    while (SrbInfo)
    {
        /* Make sure we have SRV */
        ASSERT(SrbInfo->Srb);

        /* Get SRB and LunExtension */
        Srb = SrbInfo->Srb;

        PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Srb->OriginalRequest);
        LunExtension = IoStack->DeviceObject->DeviceExtension;
        ASSERT(LunExtension && !LunExtension->Common.IsFDO);

        /* We have to check special cases if request is unsuccessful*/
        if (Srb->SrbStatus != SRB_STATUS_SUCCESS)
        {
            /* Check if we need request sense by a few conditions */
            if (Srb->SenseInfoBuffer && Srb->SenseInfoBufferLength &&
                Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
                !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID))
            {
                if (LunExtension->Flags & LUNEX_NEED_REQUEST_SENSE)
                {
                    /* It means: we tried to send REQUEST SENSE, but failed */

                    Srb->ScsiStatus = 0;
                    Srb->SrbStatus = SRB_STATUS_REQUEST_SENSE_FAILED;
                }
                else
                {
                    /* Set the corresponding flag, so that REQUEST SENSE
                       will be sent */
                    LunExtension->Flags |= LUNEX_NEED_REQUEST_SENSE;
                }

            }

            /* Check for a full queue */
            if (Srb->ScsiStatus == SCSISTAT_QUEUE_FULL)
            {
                /* TODO: Implement when it's encountered */
                ASSERT(FALSE);
            }
        }

        /* Let's decide if we need to watch timeout or not */
        if (Srb->QueueTag == SP_UNTAGGED)
        {
            IsTimed = TRUE;
        }
        else
        {
            if (LunExtension->SrbInfo.Requests.Flink == &SrbInfo->Requests)
                IsTimed = TRUE;
            else
                IsTimed = FALSE;

            /* Remove it from the queue */
            RemoveEntryList(&SrbInfo->Requests);
        }

        if (IsTimed)
        {
            /* We have to maintain timeout counter */
            if (IsListEmpty(&LunExtension->SrbInfo.Requests))
            {
                LunExtension->RequestTimeout = -1;
            }
            else
            {
                NextSrbInfo = CONTAINING_RECORD(LunExtension->SrbInfo.Requests.Flink,
                                                SCSI_REQUEST_BLOCK_INFO,
                                                Requests);

                Srb = NextSrbInfo->Srb;

                /* Update timeout counter */
                LunExtension->RequestTimeout = Srb->TimeOutValue;
            }
        }

        SrbInfo = SrbInfo->CompletedRequests;
    }

    return TRUE;
}

VOID
NTAPI
ScsiPortDpcForIsr(
    _In_ PKDPC Dpc,
    _In_ PDEVICE_OBJECT DpcDeviceObject,
    _Inout_ PIRP DpcIrp,
    _In_opt_ PVOID DpcContext)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension = DpcDeviceObject->DeviceExtension;
    SCSI_PORT_INTERRUPT_DATA InterruptData;
    SCSI_PORT_SAVE_INTERRUPT Context;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    BOOLEAN NeedToStartIo;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    LARGE_INTEGER TimerValue;

    DPRINT("ScsiPortDpcForIsr(Dpc %p  DpcDeviceObject %p  DpcIrp %p  DpcContext %p)\n",
           Dpc, DpcDeviceObject, DpcIrp, DpcContext);

    /* We need to acquire spinlock */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    RtlZeroMemory(&InterruptData, sizeof(SCSI_PORT_INTERRUPT_DATA));

TryAgain:

    /* Interrupt structure must be snapshotted, and only then analyzed */
    Context.InterruptData = &InterruptData;
    Context.DeviceExtension = DeviceExtension;

    if (!KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                                SpiSaveInterruptData,
                                &Context))
    {
        /* Nothing - just return (don't forget to release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
        DPRINT("ScsiPortDpcForIsr() done\n");
        return;
    }

    /* If flush of adapters is needed - do it */
    if (InterruptData.Flags & SCSI_PORT_FLUSH_ADAPTERS)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Check for IoMapTransfer */
    if (InterruptData.Flags & SCSI_PORT_MAP_TRANSFER)
    {
        /* TODO: Implement */
        ASSERT(FALSE);
    }

    /* Check if timer is needed */
    if (InterruptData.Flags & SCSI_PORT_TIMER_NEEDED)
    {
        /* Save the timer routine */
        DeviceExtension->HwScsiTimer = InterruptData.HwScsiTimer;

        if (InterruptData.MiniportTimerValue == 0)
        {
            /* Cancel the timer */
            KeCancelTimer(&DeviceExtension->MiniportTimer);
        }
        else
        {
            /* Convert timer value */
            TimerValue.QuadPart = Int32x32To64(InterruptData.MiniportTimerValue, -10);

            /* Set the timer */
            KeSetTimer(&DeviceExtension->MiniportTimer,
                       TimerValue,
                       &DeviceExtension->MiniportTimerDpc);
        }
    }

    /* If it's ready for the next request */
    if (InterruptData.Flags & SCSI_PORT_NEXT_REQUEST_READY)
    {
        /* Check for a duplicate request (NextRequest+NextLuRequest) */
        if ((DeviceExtension->Flags &
            (SCSI_PORT_DEVICE_BUSY | SCSI_PORT_DISCONNECT_ALLOWED)) ==
            (SCSI_PORT_DEVICE_BUSY | SCSI_PORT_DISCONNECT_ALLOWED))
        {
            /* Clear busy flag set by ScsiPortStartPacket() */
            DeviceExtension->Flags &= ~SCSI_PORT_DEVICE_BUSY;

            if (!(InterruptData.Flags & SCSI_PORT_RESET))
            {
                /* Ready for next, and no reset is happening */
                DeviceExtension->TimerCount = -1;
            }
        }
        else
        {
            /* Not busy, but not ready for the next request */
            DeviceExtension->Flags &= ~SCSI_PORT_DEVICE_BUSY;
            InterruptData.Flags &= ~SCSI_PORT_NEXT_REQUEST_READY;
        }
    }

    /* Any resets? */
    if (InterruptData.Flags & SCSI_PORT_RESET_REPORTED)
    {
        /* Hold for a bit */
        DeviceExtension->TimerCount = 4;
    }

    /* Any ready LUN? */
    if (InterruptData.ReadyLun != NULL)
    {

        /* Process all LUNs from the list*/
        while (TRUE)
        {
            /* Remove it from the list first (as processed) */
            LunExtension = InterruptData.ReadyLun;
            InterruptData.ReadyLun = LunExtension->ReadyLun;
            LunExtension->ReadyLun = NULL;

            /* Get next request for this LUN */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension);

            /* Still ready requests exist?
               If yes - get spinlock, if no - stop here */
            if (InterruptData.ReadyLun != NULL)
                KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);
            else
                break;
        }
    }
    else
    {
        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    /* If we ready for next packet, start it */
    if (InterruptData.Flags & SCSI_PORT_NEXT_REQUEST_READY)
        IoStartNextPacket(DeviceExtension->Common.DeviceObject, FALSE);

    NeedToStartIo = FALSE;

    /* Loop the completed request list */
    while (InterruptData.CompletedRequests)
    {
        /* Remove the request */
        SrbInfo = InterruptData.CompletedRequests;
        InterruptData.CompletedRequests = SrbInfo->CompletedRequests;
        SrbInfo->CompletedRequests = NULL;

        /* Process it */
        SpiProcessCompletedRequest(DeviceExtension,
                                  SrbInfo,
                                  &NeedToStartIo);
    }

    /* Loop abort request list */
    while (InterruptData.CompletedAbort)
    {
        LunExtension = InterruptData.CompletedAbort;

        /* Remove the request */
        InterruptData.CompletedAbort = LunExtension->CompletedAbortRequests;

        /* Get spinlock since we're going to change flags */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* TODO: Put SrbExtension to the list of free extensions */
        ASSERT(FALSE);
    }

    /* If we need - call StartIo routine */
    if (NeedToStartIo)
    {
        /* Make sure CurrentIrp is not null! */
        ASSERT(DpcDeviceObject->CurrentIrp != NULL);
        ScsiPortStartIo(DpcDeviceObject, DpcDeviceObject->CurrentIrp);
    }

    /* Everything has been done, check */
    if (InterruptData.Flags & SCSI_PORT_ENABLE_INT_REQUEST)
    {
        /* Synchronize using spinlock */
        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

        /* Request an interrupt */
        DeviceExtension->HwInterrupt(DeviceExtension->MiniPortDeviceExtension);

        ASSERT(DeviceExtension->Flags & SCSI_PORT_DISABLE_INT_REQUESET);

        /* Should interrupts be enabled again? */
        if (DeviceExtension->Flags & SCSI_PORT_DISABLE_INT_REQUESET)
        {
            /* Clear this flag */
            DeviceExtension->Flags &= ~SCSI_PORT_DISABLE_INT_REQUESET;

            /* Call a special routine to do this */
            ASSERT(FALSE);
#if 0
            KeSynchronizeExecution(DeviceExtension->Interrupt,
                                   SpiEnableInterrupts,
                                   DeviceExtension);
#endif
        }

        /* If we need a notification again - loop */
        if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
            goto TryAgain;

        /* Release the spinlock */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    DPRINT("ScsiPortDpcForIsr() done\n");
}

static
PSCSI_REQUEST_BLOCK_INFO
SpiAllocateSrbStructures(
    _Inout_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PSCSI_PORT_LUN_EXTENSION LunExtension,
    _Inout_ PSCSI_REQUEST_BLOCK Srb)
{
    PCHAR SrbExtension;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;

    /* Spinlock must be held while this function executes */
    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    /* Allocate SRB data structure */
    if (DeviceExtension->NeedSrbDataAlloc)
    {
        /* Treat the abort request in a special way */
        if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
        {
            SrbInfo = SpiGetSrbData(DeviceExtension, LunExtension, Srb->QueueTag);
        }
        else if (Srb->SrbFlags &
                 (SRB_FLAGS_QUEUE_ACTION_ENABLE | SRB_FLAGS_NO_QUEUE_FREEZE) &&
                 !(Srb->SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT)
                 )
        {
            /* Do not process tagged commands if need request sense is set */
            if (LunExtension->Flags & LUNEX_NEED_REQUEST_SENSE)
            {
                ASSERT(!(LunExtension->Flags & LUNEX_REQUEST_PENDING));

                LunExtension->PendingRequest = Srb->OriginalRequest;
                LunExtension->Flags |= LUNEX_REQUEST_PENDING | SCSI_PORT_LU_ACTIVE;

                /* Release the spinlock and return */
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                return NULL;
            }

            ASSERT(LunExtension->SrbInfo.Srb == NULL);
            SrbInfo = DeviceExtension->FreeSrbInfo;

            if (SrbInfo == NULL)
            {
                /* No SRB structures left in the list. We have to leave
                   and wait while we are called again */

                DeviceExtension->Flags |= SCSI_PORT_REQUEST_PENDING;
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                return NULL;
            }

            DeviceExtension->FreeSrbInfo = (PSCSI_REQUEST_BLOCK_INFO)SrbInfo->Requests.Flink;

            /* QueueTag must never be 0, so +1 to it */
            Srb->QueueTag = (UCHAR)(SrbInfo - DeviceExtension->SrbInfo) + 1;
        }
        else
        {
            /* Usual untagged command */
            if (
                (!IsListEmpty(&LunExtension->SrbInfo.Requests) ||
                LunExtension->Flags & LUNEX_NEED_REQUEST_SENSE) &&
                !(Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
                )
            {
                /* Mark it as pending and leave */
                ASSERT(!(LunExtension->Flags & LUNEX_REQUEST_PENDING));
                LunExtension->Flags |= LUNEX_REQUEST_PENDING | SCSI_PORT_LU_ACTIVE;
                LunExtension->PendingRequest = Srb->OriginalRequest;

                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
                return(NULL);
            }

            Srb->QueueTag = SP_UNTAGGED;
            SrbInfo = &LunExtension->SrbInfo;
        }
    }
    else
    {
        Srb->QueueTag = SP_UNTAGGED;
        SrbInfo = &LunExtension->SrbInfo;
    }

    /* Allocate SRB extension structure */
    if (DeviceExtension->NeedSrbExtensionAlloc)
    {
        /* Check the list of free extensions */
        SrbExtension = DeviceExtension->FreeSrbExtensions;

        /* If no free extensions... */
        if (SrbExtension == NULL)
        {
            /* Free SRB data */
            if (Srb->Function != SRB_FUNCTION_ABORT_COMMAND &&
                Srb->QueueTag != SP_UNTAGGED)
            {
                SrbInfo->Requests.Blink = NULL;
                SrbInfo->Requests.Flink = (PLIST_ENTRY)DeviceExtension->FreeSrbInfo;
                DeviceExtension->FreeSrbInfo = SrbInfo;
            }

            /* Return, in order to be called again later */
            DeviceExtension->Flags |= SCSI_PORT_REQUEST_PENDING;
            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
            return NULL;
        }

        /* Remove that free SRB extension from the list (since
           we're going to use it) */
        DeviceExtension->FreeSrbExtensions = *((PVOID *)SrbExtension);

        /* Spinlock can be released now */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        Srb->SrbExtension = SrbExtension;

        if (Srb->SenseInfoBuffer != NULL &&
            DeviceExtension->SupportsAutoSense)
        {
            /* Store pointer to the SenseInfo buffer */
            SrbInfo->SaveSenseRequest = Srb->SenseInfoBuffer;

            /* Does data fit the buffer? */
            if (Srb->SenseInfoBufferLength > sizeof(SENSE_DATA))
            {
                /* No, disabling autosense at all */
                Srb->SrbFlags |= SRB_FLAGS_DISABLE_AUTOSENSE;
            }
            else
            {
                /* Yes, update the buffer pointer */
                Srb->SenseInfoBuffer = SrbExtension + DeviceExtension->SrbExtensionSize;
            }
        }
    }
    else
    {
        /* Cleanup... */
        Srb->SrbExtension = NULL;
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }

    return SrbInfo;
}

VOID
NTAPI
ScsiPortStartIo(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_REQUEST_BLOCK Srb;
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    LONG CounterResult;
    NTSTATUS Status;

    DPRINT("ScsiPortStartIo() called!\n");

    DeviceExtension = DeviceObject->DeviceExtension;
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    LunExtension = IrpStack->DeviceObject->DeviceExtension;

    ASSERT(DeviceExtension->Common.IsFDO);
    ASSERT(!LunExtension->Common.IsFDO);

    DPRINT("LunExtension %p DeviceExtension %p\n", LunExtension, DeviceExtension);

    Srb = IrpStack->Parameters.Scsi.Srb;

    /* Apply "default" flags */
    Srb->SrbFlags |= DeviceExtension->SrbFlags;

    if (DeviceExtension->NeedSrbDataAlloc ||
        DeviceExtension->NeedSrbExtensionAlloc)
    {
        /* Allocate them */
        SrbInfo = SpiAllocateSrbStructures(DeviceExtension,
                                           LunExtension,
                                           Srb);

        /* Couldn't alloc one or both data structures, return */
        if (SrbInfo == NULL)
        {
            /* We have to call IoStartNextPacket, because this request
               was not started */
            if (LunExtension->Flags & LUNEX_REQUEST_PENDING)
                IoStartNextPacket(DeviceObject, FALSE);

            return;
        }
    }
    else
    {
        /* No allocations are needed */
        SrbInfo = &LunExtension->SrbInfo;
        Srb->SrbExtension = NULL;
        Srb->QueueTag = SP_UNTAGGED;
    }

    /* Increase sequence number of SRB */
    if (!SrbInfo->SequenceNumber)
    {
        /* Increase global sequence number */
        DeviceExtension->SequenceNumber++;

        /* Assign it */
        SrbInfo->SequenceNumber = DeviceExtension->SequenceNumber;
    }

    /* Check some special SRBs */
    if (Srb->Function == SRB_FUNCTION_ABORT_COMMAND)
    {
        /* Some special handling */
        DPRINT1("Abort command! Unimplemented now\n");
    }
    else
    {
        SrbInfo->Srb = Srb;
    }

    if (Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION)
    {
        // Store the MDL virtual address in SrbInfo structure
        SrbInfo->DataOffset = MmGetMdlVirtualAddress(Irp->MdlAddress);

        if (DeviceExtension->MapBuffers)
        {
            /* Calculate offset within DataBuffer */
            SrbInfo->DataOffset = MmGetSystemAddressForMdl(Irp->MdlAddress);
            Srb->DataBuffer = SrbInfo->DataOffset +
                (ULONG)((PUCHAR)Srb->DataBuffer -
                (PUCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress));
        }

        if (DeviceExtension->AdapterObject)
        {
            /* Flush buffers */
            KeFlushIoBuffers(Irp->MdlAddress,
                             Srb->SrbFlags & SRB_FLAGS_DATA_IN ? TRUE : FALSE,
                             TRUE);
        }

        if (DeviceExtension->MapRegisters)
        {
            /* Calculate number of needed map registers */
            SrbInfo->NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                    Srb->DataBuffer,
                    Srb->DataTransferLength);

            /* Allocate adapter channel */
            Status = IoAllocateAdapterChannel(DeviceExtension->AdapterObject,
                                              DeviceExtension->Common.DeviceObject,
                                              SrbInfo->NumberOfMapRegisters,
                                              SpiAdapterControl,
                                              SrbInfo);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IoAllocateAdapterChannel() failed!\n");

                Srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
                ScsiPortNotification(RequestComplete,
                                     DeviceExtension + 1,
                                     Srb);

                ScsiPortNotification(NextRequest,
                                     DeviceExtension + 1);

                /* Request DPC for that work */
                IoRequestDpc(DeviceExtension->Common.DeviceObject, NULL, NULL);
            }

            /* Control goes to SpiAdapterControl */
            return;
        }
    }

    /* Increase active request counter */
    CounterResult = InterlockedIncrement(&DeviceExtension->ActiveRequestCounter);

    if (CounterResult == 0 &&
        DeviceExtension->AdapterObject != NULL &&
        !DeviceExtension->MapRegisters)
    {
        IoAllocateAdapterChannel(
            DeviceExtension->AdapterObject,
            DeviceObject,
            DeviceExtension->PortCapabilities.MaximumPhysicalPages,
            ScsiPortAllocateAdapterChannel,
            LunExtension
            );

        return;
    }

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->SpinLock);

    if (!KeSynchronizeExecution(DeviceExtension->Interrupt[0],
                                ScsiPortStartPacket,
                                DeviceObject))
    {
        DPRINT("Synchronization failed!\n");

        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        /* Release the spinlock only */
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->SpinLock);
    }


    DPRINT("ScsiPortStartIo() done\n");
}
