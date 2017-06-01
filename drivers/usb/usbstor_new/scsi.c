/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/pdo.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>

VOID
DumpCBW(
    PUCHAR Block)
{
    DPRINT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        Block[0], Block[1], Block[2], Block[3], Block[4], Block[5], Block[6], Block[7], Block[8], Block[9],
        Block[10], Block[11], Block[12], Block[13], Block[14], Block[15], Block[16], Block[17], Block[18], Block[19],
        Block[20], Block[21], Block[22], Block[23], Block[24], Block[25], Block[26], Block[27], Block[28], Block[29], Block[30]);
}

NTSTATUS
NTAPI
USBSTOR_SrbStatusToNtStatus(
    IN PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR SrbStatus;

    SrbStatus = SRB_STATUS(Srb->SrbStatus);

    DPRINT("USBSTOR_SrbStatusToNtStatus: Srb - %p, SrbStatus - %x\n",
           Srb,
           SrbStatus);

    switch (SrbStatus)
    {
        case SRB_STATUS_DATA_OVERRUN:
            return STATUS_BUFFER_OVERFLOW;

        case SRB_STATUS_BAD_FUNCTION:
        case SRB_STATUS_BAD_SRB_BLOCK_LENGTH:
            return STATUS_INVALID_DEVICE_REQUEST;

        case SRB_STATUS_INVALID_LUN:
        case SRB_STATUS_INVALID_TARGET_ID:
        case SRB_STATUS_NO_HBA:
        case SRB_STATUS_NO_DEVICE:
            return STATUS_DEVICE_DOES_NOT_EXIST;

        case SRB_STATUS_TIMEOUT:
            return STATUS_IO_TIMEOUT;

        case SRB_STATUS_BUS_RESET:
        case SRB_STATUS_COMMAND_TIMEOUT:
        case SRB_STATUS_SELECTION_TIMEOUT:
            return STATUS_DEVICE_NOT_CONNECTED;

        default:
            return STATUS_IO_DEVICE_ERROR;
    }
}

VOID
NTAPI
USBSTOR_HandleCDBComplete(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR OperationCode;
    UCHAR SrbStatus;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

    DPRINT("USBSTOR_HandleCDBComplete: ... \n");

    InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)(
                           (ULONG_PTR)FDODeviceExtension->ConfigurationDescriptor +
                           sizeof(USB_CONFIGURATION_DESCRIPTOR));

    if (InterfaceDescriptor->bInterfaceSubClass != USB_SUBCLASS_SCSI)
    {
        OperationCode = Srb->Cdb[0];

        if (OperationCode != FDODeviceExtension->CurrentCdb.CDB6GENERIC.OperationCode)
        {
            if (OperationCode == SCSIOP_MODE_SENSE10)
            {
                DPRINT("USBSTOR_HandleCDBComplete: FIXME SCSIOP_MODE_SENSE10\n");
            }

            RtlCopyMemory(Srb->Cdb,
                          &FDODeviceExtension->CurrentCdb,
                          sizeof(CDB));
        }
    }

    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS &&
        !(Srb->SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE))
    {
        Srb->SrbStatus |= SRB_STATUS_QUEUE_FROZEN;
        DPRINT("USBSTOR_HandleCDBComplete: FIXME Queue Freeze\n");
    }

    SrbStatus = SRB_STATUS(Srb->SrbStatus);

    if (SrbStatus != SRB_STATUS_SUCCESS)
    {
        SrbStatus = USBSTOR_SrbStatusToNtStatus(Srb);
        Irp->IoStatus.Status = SrbStatus;
    }
}

BOOLEAN
NTAPI
USBSTOR_IsRequestTimeOut(
    IN PDEVICE_OBJECT FdoDevice,
    IN PIRP Irp,
    IN OUT NTSTATUS * OutStatus)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    KIRQL OldIrql;
    PSCSI_REQUEST_BLOCK Srb;
    BOOLEAN Result;
    BOOLEAN IsTimeOut;
    PIO_STACK_LOCATION IoStack;

    DPRINT("USBSTOR_IsRequestTimeOut: ... \n");

    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    IsTimeOut = FALSE;

    KeAcquireSpinLock(&FDODeviceExtension->StorSpinLock, &OldIrql);

    FDODeviceExtension->Flags &= ~USBSTOR_FDO_FLAGS_TRANSFER_FINISHED;

    if (FDODeviceExtension->Flags & USBSTOR_FDO_FLAGS_DEVICE_RESETTING)
    {
        IsTimeOut = TRUE;
    }

    KeReleaseSpinLock(&FDODeviceExtension->StorSpinLock, OldIrql);

    if (IsTimeOut)
    {
        Srb = FDODeviceExtension->CurrentSrb;

        DPRINT1("USBSTOR_IsRequestTimeOut: Irp - %p, Srb - %p\n", Irp, Srb);

        IoStack = Irp->Tail.Overlay.CurrentStackLocation;
        IoStack->Parameters.Scsi.Srb = Srb;

        Irp->IoStatus.Status = STATUS_IO_TIMEOUT;
        Irp->IoStatus.Information = 0;

        Srb->SrbStatus = SRB_STATUS_TIMEOUT;

        USBSTOR_HandleCDBComplete(FDODeviceExtension, Irp, Srb);

        *OutStatus = STATUS_MORE_PROCESSING_REQUIRED;

        KeSetEvent(&FDODeviceExtension->TimeOutEvent,
                   IO_NO_INCREMENT,
                   FALSE);

        Result = TRUE;
    }
    else
    {
        FDODeviceExtension->CurrentIrp = NULL;
        Result = FALSE;
    }

    return Result;
}

NTSTATUS
NTAPI
USBSTOR_IssueBulkOrInterruptRequest(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp,
    IN USBD_PIPE_HANDLE PipeHandle,
    IN ULONG TransferFlags,
    IN ULONG TransferBufferLength,
    IN PVOID TransferBuffer,
    IN PMDL TransferBufferMDL,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context)
{
    PIO_STACK_LOCATION NextStack;
    KIRQL OldIrql;

    DPRINT("USBSTOR_IssueBulkOrInterruptRequest: ... \n");

    RtlZeroMemory(&FDODeviceExtension->Urb,
                  sizeof (struct _URB_BULK_OR_INTERRUPT_TRANSFER));

    FDODeviceExtension->Urb.Hdr.Length = sizeof (struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    FDODeviceExtension->Urb.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;

    FDODeviceExtension->Urb.PipeHandle = PipeHandle;
    FDODeviceExtension->Urb.TransferFlags = TransferFlags;
    FDODeviceExtension->Urb.TransferBufferLength = TransferBufferLength;
    FDODeviceExtension->Urb.TransferBuffer = TransferBuffer;
    FDODeviceExtension->Urb.TransferBufferMDL = TransferBufferMDL;

    NextStack = IoGetNextIrpStackLocation(Irp); 
    NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL; 
    NextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB; 
    NextStack->Parameters.Others.Argument1 = &FDODeviceExtension->Urb; 

    IoSetCompletionRoutine(Irp,
                           CompletionRoutine,
                           Context,
                           TRUE,
                           TRUE,
                           TRUE); 

    KeAcquireSpinLock(&FDODeviceExtension->StorSpinLock, &OldIrql);
    FDODeviceExtension->Flags |= USBSTOR_FDO_FLAGS_TRANSFER_FINISHED;
    FDODeviceExtension->CurrentIrp = Irp;
    KeReleaseSpinLock(&FDODeviceExtension->StorSpinLock, OldIrql);

    return IoCallDriver(FDODeviceExtension->LowerDeviceObject, Irp);
}

NTSTATUS
USBSTOR_BuildCBW(
    IN ULONG Tag,
    IN ULONG DataTransferLength,
    IN UCHAR LUN,
    IN UCHAR CommandBlockLength,
    IN PUCHAR CommandBlock,
    IN OUT PCBW Control)
{
    //
    // sanity check
    //
    ASSERT(CommandBlockLength <= 16);

    //
    // now initialize CBW
    //
    Control->Signature = CBW_SIGNATURE;
    Control->Tag = Tag;
    Control->DataTransferLength = DataTransferLength;
    Control->Flags = (CommandBlock[0] != SCSIOP_WRITE) ? 0x80 : 0x00;
    Control->LUN = (LUN & MAX_LUN);
    Control->CommandBlockLength = CommandBlockLength;

    //
    // copy command block
    //
    RtlCopyMemory(Control->CommandBlock, CommandBlock, CommandBlockLength);

    //
    // done
    //
    return STATUS_SUCCESS;
}

PIRP_CONTEXT
USBSTOR_AllocateIrpContext()
{
    PIRP_CONTEXT Context;

    //
    // allocate irp context
    //
    Context = (PIRP_CONTEXT)AllocateItem(NonPagedPool, sizeof(IRP_CONTEXT));
    if (!Context)
    {
        //
        // no memory
        //
        return NULL;
    }

    //
    // allocate cbw block
    //
    Context->cbw = (PCBW)AllocateItem(NonPagedPool, 512);
    if (!Context->cbw)
    {
        //
        // no memory
        //
        FreeItem(Context);
        return NULL;
    }

    //
    // done
    //
    return Context;

}

BOOLEAN
USBSTOR_IsCSWValid(
    PIRP_CONTEXT Context)
{
    //
    // sanity checks
    //
    if (Context->csw->Signature != CSW_SIGNATURE)
    {
        DPRINT1("[USBSTOR] Expected Signature %x but got %x\n", CSW_SIGNATURE, Context->csw->Signature);
        return FALSE;
    }

    if (Context->csw->Tag != (ULONG)Context->csw)
    {
        DPRINT1("[USBSTOR] Expected Tag %x but got %x\n", (ULONG)Context->csw, Context->csw->Tag);
        return FALSE;
    }

    if (Context->csw->Status != 0x00)
    {
        DPRINT1("[USBSTOR] Expected Status 0x00 but got %x\n", Context->csw->Status);
        return FALSE;
    }

    //
    // CSW is valid
    //
    return TRUE;

}

NTSTATUS
USBSTOR_QueueWorkItem(
    PIRP_CONTEXT Context,
    PIRP Irp)
{
    PERRORHANDLER_WORKITEM_DATA ErrorHandlerWorkItemData;

    //
    // Allocate Work Item Data
    //
    ErrorHandlerWorkItemData = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERRORHANDLER_WORKITEM_DATA), USB_STOR_TAG);
    if (!ErrorHandlerWorkItemData)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // error handling started
    //
    Context->FDODeviceExtension->SrbErrorHandlingActive = TRUE;

    //
    // srb error handling finished
    //
    Context->FDODeviceExtension->TimerWorkQueueEnabled = FALSE;

    //
    // Initialize and queue the work item to handle the error
    //
    ExInitializeWorkItem(&ErrorHandlerWorkItemData->WorkQueueItem,
                         ErrorHandlerWorkItemRoutine,
                         ErrorHandlerWorkItemData);

    ErrorHandlerWorkItemData->DeviceObject = Context->FDODeviceExtension->FunctionalDeviceObject;
    ErrorHandlerWorkItemData->Context = Context;
    ErrorHandlerWorkItemData->Irp = Irp;
    ErrorHandlerWorkItemData->DeviceObject = Context->FDODeviceExtension->FunctionalDeviceObject;

    DPRINT1("Queuing WorkItemROutine\n");
    ExQueueWorkItem(&ErrorHandlerWorkItemData->WorkQueueItem, DelayedWorkQueue);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


//
// driver verifier
//
IO_COMPLETION_ROUTINE USBSTOR_CSWCompletionRoutine;

NTSTATUS
NTAPI
USBSTOR_CSWCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Ctx)
{
    PIRP_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PCDB pCDB;
    PREAD_CAPACITY_DATA_EX CapacityDataEx;
    PREAD_CAPACITY_DATA CapacityData;
    PUFI_CAPACITY_RESPONSE Response;
    NTSTATUS Status;

    //
    // access context
    //
    Context = (PIRP_CONTEXT)Ctx;

    //
    // is there a mdl
    //
    if (Context->TransferBufferMDL)
    {
        //
        // is there an irp associated
        //
        if (Context->Irp)
        {
            //
            // did we allocate the mdl
            //
            if (Context->TransferBufferMDL != Context->Irp->MdlAddress)
            {
                //
                // free mdl
                //
                IoFreeMdl(Context->TransferBufferMDL);
            }
        }
        else
        {
            //
            // free mdl
            //
            IoFreeMdl(Context->TransferBufferMDL);
        }
    }

    DPRINT("USBSTOR_CSWCompletionRoutine Status %x\n", Irp->IoStatus.Status);

    if (!NT_SUCCESS(Irp->IoStatus.Information))
    {
        if (Context->ErrorIndex == 0)
        {
            //
            // increment error index
            //
            Context->ErrorIndex = 1;

            //
            // clear stall and resend cbw
            //
            Status = USBSTOR_QueueWorkItem(Context, Irp);
            ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        //
        // perform reset recovery
        //
        Context->ErrorIndex = 2;
        IoFreeIrp(Irp);
        Status = USBSTOR_QueueWorkItem(Context, NULL);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (!USBSTOR_IsCSWValid(Context))
    {
        //
        // perform reset recovery
        //
        Context->ErrorIndex = 2;
        IoFreeIrp(Irp);
        Status = USBSTOR_QueueWorkItem(Context, NULL);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Context->Irp);

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;
    ASSERT(Request);

    Status = Irp->IoStatus.Status;

    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;
    Request->SrbStatus = SRB_STATUS_SUCCESS;

    //
    // read capacity needs special work
    //
    if (pCDB->AsByte[0] == SCSIOP_READ_CAPACITY)
    {
        //
        // get output buffer
        //
        Response = (PUFI_CAPACITY_RESPONSE)Context->TransferData;

        //
        // store in pdo
        //
        Context->PDODeviceExtension->BlockLength = NTOHL(Response->BlockLength);
        Context->PDODeviceExtension->LastLogicBlockAddress = NTOHL(Response->LastLogicalBlockAddress);

        if (Request->DataTransferLength == sizeof(READ_CAPACITY_DATA_EX))
        {
            //
            // get input buffer
            //
            CapacityDataEx = (PREAD_CAPACITY_DATA_EX)Request->DataBuffer;

            //
            // set result
            //
            CapacityDataEx->BytesPerBlock = Response->BlockLength;
            CapacityDataEx->LogicalBlockAddress.QuadPart = Response->LastLogicalBlockAddress;
            Irp->IoStatus.Information = sizeof(READ_CAPACITY_DATA_EX);
       }
       else
       {
            //
            // get input buffer
            //
            CapacityData = (PREAD_CAPACITY_DATA)Request->DataBuffer;

            //
            // set result
            //
            CapacityData->BytesPerBlock = Response->BlockLength;
            CapacityData->LogicalBlockAddress = Response->LastLogicalBlockAddress;
            Irp->IoStatus.Information = sizeof(READ_CAPACITY_DATA);
       }

       //
       // free response
       //
       FreeItem(Context->TransferData);
    }

    //
    // free cbw
    //
    FreeItem(Context->cbw);

    //
    // FIXME: check status
    //
    Context->Irp->IoStatus.Status = Irp->IoStatus.Status;
    Context->Irp->IoStatus.Information = Context->TransferDataLength;

    //
    // terminate current request
    //
    USBSTOR_QueueTerminateRequest(Context->PDODeviceExtension->LowerDeviceObject, Context->Irp);

    //
    // complete request
    //
    IoCompleteRequest(Context->Irp, IO_NO_INCREMENT);

    //
    // start next request
    //
    USBSTOR_QueueNextRequest(Context->PDODeviceExtension->LowerDeviceObject);

    //
    // free our allocated irp
    //
    IoFreeIrp(Irp);

    //
    // free context
    //
    FreeItem(Context);

    //
    // done
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
USBSTOR_SendCSW(
    PIRP_CONTEXT Context,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    //
    // get next irp stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // now initialize the urb for sending the csw
    //
    UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle,
                                           Context->csw,
                                           NULL,
                                           512, //FIXME
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    //
    // initialize stack location
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)&Context->Urb;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = Context->Urb.UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;


    //
    // setup completion routine
    //
    IoSetCompletionRoutine(Irp, USBSTOR_CSWCompletionRoutine, Context, TRUE, TRUE, TRUE);

    //
    // call driver
    //
    IoCallDriver(Context->FDODeviceExtension->LowerDeviceObject, Irp);
}


//
// driver verifier
//
IO_COMPLETION_ROUTINE USBSTOR_DataCompletionRoutine;

NTSTATUS
NTAPI
USBSTOR_DataCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Ctx)
{
    PIRP_CONTEXT Context;
    NTSTATUS Status;


    DPRINT("USBSTOR_DataCompletionRoutine Irp %p Ctx %p Status %x\n", Irp, Ctx, Irp->IoStatus.Status);

    //
    // access context
    //
    Context = (PIRP_CONTEXT)Ctx;

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        //
        // clear stall and resend cbw
        //
        Context->ErrorIndex = 1;
        Status = USBSTOR_QueueWorkItem(Context, Irp);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // send csw
    //
    USBSTOR_SendCSW(Context, Irp);

    //
    // cancel completion
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//
// driver verifier
//
IO_COMPLETION_ROUTINE USBSTOR_CBWCompletionRoutine;

NTSTATUS
NTAPI
USBSTOR_CBWCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Ctx)
{
    PIRP_CONTEXT Context;
    PIO_STACK_LOCATION IoStack;
    UCHAR Code;
    USBD_PIPE_HANDLE PipeHandle;

    DPRINT("USBSTOR_CBWCompletionRoutine Irp %p Ctx %p Status %x\n", Irp, Ctx, Irp->IoStatus.Status);

    //
    // access context
    //
    Context = (PIRP_CONTEXT)Ctx;

    //
    // get next stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // is there data to be submitted
    //
    if (Context->TransferDataLength)
    {
        //
        // get command code
        //
        Code = Context->cbw->CommandBlock[0];

        if (Code == SCSIOP_WRITE)
        {
            //
            // write request use bulk out pipe
            //
            PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkOutPipeIndex].PipeHandle;
        }
        else
        {
            //
            // default bulk in pipe
            //
            PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle;
        }

        //
        // now initialize the urb for sending data
        //
        UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                               sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                               PipeHandle,
                                               NULL,
                                               Context->TransferBufferMDL,
                                               Context->TransferDataLength,
                                               ((Code == SCSIOP_WRITE) ? USBD_TRANSFER_DIRECTION_OUT : (USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK)),
                                               NULL);

        //
        // setup completion routine
        //
        IoSetCompletionRoutine(Irp, USBSTOR_DataCompletionRoutine, Context, TRUE, TRUE, TRUE);
    }
    else
    {
        //
        // now initialize the urb for sending the csw
        //

        UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                               sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                               Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle,
                                               Context->csw,
                                               NULL,
                                               512, //FIXME
                                               USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                               NULL);

        //
        // setup completion routine
        //
        IoSetCompletionRoutine(Irp, USBSTOR_CSWCompletionRoutine, Context, TRUE, TRUE, TRUE);
    }

    //
    // initialize stack location
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)&Context->Urb;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = Context->Urb.UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // call driver
    //
    IoCallDriver(Context->FDODeviceExtension->LowerDeviceObject, Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
USBSTOR_SendCBW(
    PIRP_CONTEXT Context,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    //
    // get next stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // initialize stack location
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)&Context->Urb;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = Context->Urb.UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // setup completion routine
    //
    IoSetCompletionRoutine(Irp, USBSTOR_CBWCompletionRoutine, Context, TRUE, TRUE, TRUE);

    //
    // call driver
    //
    return IoCallDriver(Context->FDODeviceExtension->LowerDeviceObject, Irp);
}

NTSTATUS
USBSTOR_SendRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP OriginalRequest,
    IN UCHAR CommandLength,
    IN PUCHAR Command,
    IN ULONG TransferDataLength,
    IN PUCHAR TransferData,
    IN ULONG RetryCount)
{
    PIRP_CONTEXT Context;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PIRP Irp;
    PUCHAR MdlVirtualAddress;

    //
    // first allocate irp context
    //
    Context = USBSTOR_AllocateIrpContext();
    if (!Context)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // now build the cbw
    //
    USBSTOR_BuildCBW((ULONG)Context->cbw,
                     TransferDataLength,
                     PDODeviceExtension->LUN,
                     CommandLength,
                     Command,
                     Context->cbw);

    DPRINT("CBW %p\n", Context->cbw);
    DumpCBW((PUCHAR)Context->cbw);

    //
    // now initialize the urb
    //
    UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle,
                                           Context->cbw,
                                           NULL,
                                           sizeof(CBW),
                                           USBD_TRANSFER_DIRECTION_OUT,
                                           NULL);

    //
    // initialize rest of context
    //
    Context->Irp = OriginalRequest;
    Context->TransferData = TransferData;
    Context->TransferDataLength = TransferDataLength;
    Context->FDODeviceExtension = FDODeviceExtension;
    Context->PDODeviceExtension = PDODeviceExtension;
    Context->RetryCount = RetryCount;

    //
    // is there transfer data
    //
    if (Context->TransferDataLength)
    {
        //
        // check if the original request already does have an mdl associated
        //
        if (OriginalRequest)
        {
            if ((OriginalRequest->MdlAddress != NULL) &&
                (Context->TransferData == NULL || Command[0] == SCSIOP_READ || Command[0] == SCSIOP_WRITE))
            {
                //
                // Sanity check that the Mdl does describe the TransferData for read/write
                //
                if (CommandLength == UFI_READ_WRITE_CMD_LEN)
                {
                    MdlVirtualAddress = MmGetMdlVirtualAddress(OriginalRequest->MdlAddress);

                    //
                    // is there an offset
                    //
                    if (MdlVirtualAddress != Context->TransferData)
                    {
                        //
                        // lets build an mdl
                        //
                        Context->TransferBufferMDL = IoAllocateMdl(Context->TransferData, MmGetMdlByteCount(OriginalRequest->MdlAddress), FALSE, FALSE, NULL);
                        if (!Context->TransferBufferMDL)
                        {
                            //
                            // failed to allocate MDL
                            //
                            FreeItem(Context->cbw);
                            FreeItem(Context);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }

                        //
                        // now build the partial mdl
                        //
                        IoBuildPartialMdl(OriginalRequest->MdlAddress, Context->TransferBufferMDL, Context->TransferData, Context->TransferDataLength);
                    }
                }

                if (!Context->TransferBufferMDL)
                {
                    //
                    // I/O paging request
                    //
                    Context->TransferBufferMDL = OriginalRequest->MdlAddress;
                }
            }
            else
            {
                //
                // allocate mdl for buffer, buffer must be allocated from NonPagedPool
                //
                Context->TransferBufferMDL = IoAllocateMdl(Context->TransferData, Context->TransferDataLength, FALSE, FALSE, NULL);
                if (!Context->TransferBufferMDL)
                {
                    //
                    // failed to allocate MDL
                    //
                    FreeItem(Context->cbw);
                    FreeItem(Context);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                //
                // build mdl for nonpaged pool
                //
                MmBuildMdlForNonPagedPool(Context->TransferBufferMDL);
            }
        }
        else
        {
            //
            // allocate mdl for buffer, buffer must be allocated from NonPagedPool
            //
            Context->TransferBufferMDL = IoAllocateMdl(Context->TransferData, Context->TransferDataLength, FALSE, FALSE, NULL);
            if (!Context->TransferBufferMDL)
            {
                //
                // failed to allocate MDL
                //
                FreeItem(Context->cbw);
                FreeItem(Context);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // build mdl for nonpaged pool
            //
            MmBuildMdlForNonPagedPool(Context->TransferBufferMDL);
        }
    }

    //
    // now allocate the request
    //
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        FreeItem(Context->cbw);
        FreeItem(Context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (OriginalRequest)
    {
        //
        // mark orignal irp as pending
        //
        IoMarkIrpPending(OriginalRequest);
    }

    //
    // send request
    //
    USBSTOR_SendCBW(Context, Irp);

    //
    // done
    //
    return STATUS_PENDING;
}

NTSTATUS
USBSTOR_SendFormatCapacity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG RetryCount)
{
    UFI_READ_FORMAT_CAPACITY Cmd;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // initialize inquiry cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_READ_FORMAT_CAPACITY));
    Cmd.Code = SCSIOP_READ_FORMATTED_CAPACITY;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.AllocationLengthMsb = HTONS(Request->DataTransferLength & 0xFFFF) >> 8;
    Cmd.AllocationLengthLsb = HTONS(Request->DataTransferLength & 0xFFFF) & 0xFF;

    //
    // now send the request
    //
    return USBSTOR_SendRequest(DeviceObject, Irp, UFI_READ_FORMAT_CAPACITY_CMD_LEN, (PUCHAR)&Cmd, Request->DataTransferLength, (PUCHAR)Request->DataBuffer, RetryCount);
}

NTSTATUS
USBSTOR_SendInquiry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG RetryCount)
{
    UFI_INQUIRY_CMD Cmd;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // initialize inquiry cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_INQUIRY_CMD));
    Cmd.Code = SCSIOP_INQUIRY;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.AllocationLength = sizeof(UFI_INQUIRY_RESPONSE);

    //
    // sanity check
    //
    ASSERT(Request->DataTransferLength >= sizeof(UFI_INQUIRY_RESPONSE));

    //
    // now send the request
    //
    return USBSTOR_SendRequest(DeviceObject, Irp, UFI_INQUIRY_CMD_LEN, (PUCHAR)&Cmd, Request->DataTransferLength, (PUCHAR)Request->DataBuffer, RetryCount);
}

NTSTATUS
USBSTOR_SendCapacity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG RetryCount)
{
    UFI_CAPACITY_CMD Cmd;
    PUFI_CAPACITY_RESPONSE Response;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // allocate capacity response
    //
    Response = (PUFI_CAPACITY_RESPONSE)AllocateItem(NonPagedPool, PAGE_SIZE);
    if (!Response)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize capacity cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_INQUIRY_CMD));
    Cmd.Code = SCSIOP_READ_CAPACITY;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);

    //
    // send request, response will be freed in completion routine
    //
    return USBSTOR_SendRequest(DeviceObject, Irp, UFI_READ_CAPACITY_CMD_LEN, (PUCHAR)&Cmd, sizeof(UFI_CAPACITY_RESPONSE), (PUCHAR)Response, RetryCount);
}

NTSTATUS
USBSTOR_SendModeSense(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG RetryCount)
{
#if 0
    UFI_SENSE_CMD Cmd;
    NTSTATUS Status;
    PVOID Response;
    PCBW OutControl;
    PCDB pCDB;
    PUFI_MODE_PARAMETER_HEADER Header;
#endif
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    RtlZeroMemory(Request->DataBuffer, Request->DataTransferLength);
    Request->SrbStatus = SRB_STATUS_SUCCESS;
    Irp->IoStatus.Information = Request->DataTransferLength;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // start next request
    //
    USBSTOR_QueueNextRequest(PDODeviceExtension->LowerDeviceObject);

    return STATUS_SUCCESS;

#if 0
    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // allocate sense response from non paged pool
    //
    Response = (PUFI_CAPACITY_RESPONSE)AllocateItem(NonPagedPool, Request->DataTransferLength);
    if (!Response)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // sanity check
    //


    // Supported pages
    // MODE_PAGE_ERROR_RECOVERY
    // MODE_PAGE_FLEXIBILE
    // MODE_PAGE_LUN_MAPPING
    // MODE_PAGE_FAULT_REPORTING
    // MODE_SENSE_RETURN_ALL

    //
    // initialize mode sense cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_INQUIRY_CMD));
    Cmd.Code = SCSIOP_MODE_SENSE;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.PageCode = pCDB->MODE_SENSE.PageCode;
    Cmd.PC = pCDB->MODE_SENSE.Pc;
    Cmd.AllocationLength = HTONS(pCDB->MODE_SENSE.AllocationLength);

    DPRINT1("PageCode %x\n", pCDB->MODE_SENSE.PageCode);
    DPRINT1("PC %x\n", pCDB->MODE_SENSE.Pc);

    //
    // now send mode sense cmd
    //
    Status = USBSTOR_SendCBW(DeviceObject, UFI_SENSE_CMD_LEN, (PUCHAR)&Cmd, Request->DataTransferLength, &OutControl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendCapacityCmd> USBSTOR_SendCBW failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    //
    // now send data block response
    //
    Status = USBSTOR_SendData(DeviceObject, Request->DataTransferLength, Response);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendCapacityCmd> USBSTOR_SendData failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    Header = (PUFI_MODE_PARAMETER_HEADER)Response;

    //
    // TODO: build layout
    //
    // first struct is the header
    // MODE_PARAMETER_HEADER / _MODE_PARAMETER_HEADER10
    //
    // followed by
    // MODE_PARAMETER_BLOCK
    //
    //
    UNIMPLEMENTED;

    //
    // send csw
    //
    Status = USBSTOR_SendCSW(DeviceObject, OutControl, 512, &CSW);

    DPRINT1("------------------------\n");
    DPRINT1("CSW %p\n", &CSW);
    DPRINT1("Signature %x\n", CSW.Signature);
    DPRINT1("Tag %x\n", CSW.Tag);
    DPRINT1("DataResidue %x\n", CSW.DataResidue);
    DPRINT1("Status %x\n", CSW.Status);

    //
    // FIXME: handle error
    //
    ASSERT(CSW.Status == 0);
    ASSERT(CSW.DataResidue == 0);

    //
    // calculate transfer length
    //
    *TransferBufferLength = Request->DataTransferLength - CSW.DataResidue;

    //
    // copy buffer
    //
    RtlCopyMemory(Request->DataBuffer, Response, *TransferBufferLength);

    //
    // free item
    //
    FreeItem(OutControl);

    //
    // free response
    //
    FreeItem(Response);

    //
    // done
    //
    return Status;
#endif
}

NTSTATUS
USBSTOR_SendReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG RetryCount)
{
    UFI_READ_WRITE_CMD Cmd;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PCDB pCDB;
    ULONG BlockCount, Temp;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // informal debug print
    //
    DPRINT("USBSTOR_SendReadWrite DataTransferLength %lu, BlockLength %lu\n", Request->DataTransferLength, PDODeviceExtension->BlockLength);

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->BlockLength);

    //
    // block count
    //
    BlockCount = Request->DataTransferLength / PDODeviceExtension->BlockLength;

    //
    // initialize read cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_READ_WRITE_CMD));
    Cmd.Code = pCDB->AsByte[0];
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.ContiguousLogicBlocksByte0 = pCDB->CDB10.TransferBlocksMsb;
    Cmd.ContiguousLogicBlocksByte1 = pCDB->CDB10.TransferBlocksLsb;
    Cmd.LogicalBlockByte0 = pCDB->CDB10.LogicalBlockByte0;
    Cmd.LogicalBlockByte1 = pCDB->CDB10.LogicalBlockByte1;
    Cmd.LogicalBlockByte2 = pCDB->CDB10.LogicalBlockByte2;
    Cmd.LogicalBlockByte3 = pCDB->CDB10.LogicalBlockByte3;

    //
    // sanity check
    //
    Temp = (Cmd.ContiguousLogicBlocksByte0 << 8 | Cmd.ContiguousLogicBlocksByte1);
    ASSERT(Temp == BlockCount);

    DPRINT("USBSTOR_SendReadWrite BlockAddress %x%x%x%x BlockCount %lu BlockLength %lu\n", Cmd.LogicalBlockByte0, Cmd.LogicalBlockByte1, Cmd.LogicalBlockByte2, Cmd.LogicalBlockByte3, BlockCount, PDODeviceExtension->BlockLength);

    //
    // send request
    //
    return USBSTOR_SendRequest(DeviceObject, Irp, UFI_READ_WRITE_CMD_LEN, (PUCHAR)&Cmd, Request->DataTransferLength, (PUCHAR)Request->DataBuffer, RetryCount);
}

NTSTATUS
USBSTOR_SendTestUnit(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN ULONG RetryCount)
{
    UFI_TEST_UNIT_CMD Cmd;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = (PSCSI_REQUEST_BLOCK)IoStack->Parameters.Others.Argument1;

    //
    // no transfer length
    //
    ASSERT(Request->DataTransferLength == 0);

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // initialize test unit cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_TEST_UNIT_CMD));
    Cmd.Code = SCSIOP_TEST_UNIT_READY;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);

    //
    // send the request
    //
    return USBSTOR_SendRequest(DeviceObject, Irp, UFI_TEST_UNIT_CMD_LEN, (PUCHAR)&Cmd, 0, NULL, RetryCount);
}

NTSTATUS
USBSTOR_SendUnknownRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp,
    IN ULONG RetryCount)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    UFI_UNKNOWN_CMD Cmd;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = IoStack->Parameters.Others.Argument1;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // check that we're sending to the right LUN
    //
    ASSERT(Request->Cdb[1] == (PDODeviceExtension->LUN & MAX_LUN));

    //
    // sanity check
    //
    ASSERT(Request->CdbLength <= sizeof(UFI_UNKNOWN_CMD));

    //
    // initialize test unit cmd
    //
    RtlCopyMemory(&Cmd, Request->Cdb, Request->CdbLength);

    //
    // send the request
    //
    return USBSTOR_SendRequest(DeviceObject, Irp, Request->CdbLength, (PUCHAR)&Cmd, Request->DataTransferLength, Request->DataBuffer, RetryCount);
}



NTSTATUS
NTAPI
USBSTOR_CswCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID CompletionContext)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK CurrentSrb;
    PDEVICE_OBJECT FdoDevice;
    //KIRQL OldIrql;
    PIO_STACK_LOCATION IoStack;

    DPRINT("USBSTOR_CswCompletion: ... \n");

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    Srb = IoStack->Parameters.Scsi.Srb;

    FdoDevice = PDODeviceExtension->LowerDeviceObject;
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    if (USBSTOR_IsRequestTimeOut(FdoDevice, Irp, &Status))
    {
        return Status;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT("USBSTOR_CswCompletion: Irp->IoStatus.Status - %p\n",
               Irp->IoStatus.Status);

        if (USBD_STATUS(FDODeviceExtension->Urb.Hdr.Status) ==
             USBD_STATUS(USBD_STATUS_STALL_PID))
        {
            if (FDODeviceExtension->RetryCount < 2)
            {
                ++FDODeviceExtension->RetryCount;
                USBSTOR_BulkQueueResetPipe(FDODeviceExtension);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
        else
        {
            DPRINT("USBSTOR_CswCompletion: Urb.Hdr.Status - %p\n",
                   FDODeviceExtension->Urb.Hdr.Status);

            goto ResetRecovery;
        }
    }

    if (FDODeviceExtension->BulkBuffer.Cbw.Flags == CSW_STATUS_COMMAND_PASSED)
    {
        if (Srb != FDODeviceExtension->CurrentSrb)
        {
            FDODeviceExtension->CurrentSrb->SenseInfoBufferLength = Srb->DataTransferLength;
            Srb = FDODeviceExtension->CurrentSrb;
            IoStack->Parameters.Scsi.Srb = Srb;
            Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }

        Status = STATUS_SUCCESS;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = Srb->DataTransferLength;

        USBSTOR_HandleCDBComplete(FDODeviceExtension, Irp, Srb);
    }
    else
    {
        if (FDODeviceExtension->BulkBuffer.Cbw.Flags == CSW_STATUS_COMMAND_FAILED &&
             Srb == FDODeviceExtension->CurrentSrb)
        {
            Srb->SrbStatus = SRB_STATUS_ERROR;
            Srb->ScsiStatus = 2;

            Srb->DataTransferLength = 0;

            if (!(Srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
                  Srb->SenseInfoBufferLength &&
                  Srb->SenseInfoBuffer)
            {
                USBSTOR_IssueRequestSense(FDODeviceExtension, Irp);
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            Status = STATUS_IO_DEVICE_ERROR;
            Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
            Irp->IoStatus.Information = 0;

            USBSTOR_HandleCDBComplete(FDODeviceExtension, Irp, Srb);
        }
        else
        {
            goto ResetRecovery;
        }
    }

    //
    // terminate request
    //
    USBSTOR_QueueTerminateRequest(FdoDevice, Irp);

    //
    // queue next request
    //
    USBSTOR_QueueNextRequest(FdoDevice);

    if (Status)
    {
        DPRINT("USBSTOR_CswCompletion: Status -%p\n", Status);
    }

    return Status;

ResetRecovery:

    CurrentSrb = FDODeviceExtension->CurrentSrb;
    IoStack->Parameters.Scsi.Srb = CurrentSrb;
    Status = STATUS_IO_DEVICE_ERROR;
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
    CurrentSrb->SrbStatus = SRB_STATUS_BUS_RESET;

    USBSTOR_HandleCDBComplete(FDODeviceExtension, Irp, CurrentSrb);
    USBSTOR_QueueResetDevice(FDODeviceExtension, PDODeviceExtension);

    return Status;
}

NTSTATUS
NTAPI
USBSTOR_CswTransfer(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp)
{
    PVOID TransferBuffer;
    PUSBD_PIPE_INFORMATION PipeIn;
    ULONG TransferBufferLength;
    ULONG TransferFlags;
    UCHAR PipeIndex;

    DPRINT("USBSTOR_CswTransfer: ... \n");

    TransferBuffer = &FDODeviceExtension->BulkBuffer.Csw;

    PipeIndex = FDODeviceExtension->BulkInPipeIndex;
    PipeIn = &FDODeviceExtension->InterfaceInformation->Pipes[PipeIndex];

    if (PipeIn->MaximumPacketSize == 512)
    {
        TransferFlags = USBD_SHORT_TRANSFER_OK;
        TransferBufferLength = 512;
    }
    else
    {
        TransferFlags = 0;
        TransferBufferLength = sizeof(CSW);
    }

    return USBSTOR_IssueBulkOrInterruptRequest(FDODeviceExtension,
                                               Irp,
                                               PipeIn->PipeHandle,
                                               TransferFlags,
                                               TransferBufferLength,
                                               TransferBuffer,
                                               NULL,//TransferBufferMDL
                                               USBSTOR_CswCompletion,
                                               NULL);//Context
}

NTSTATUS
NTAPI
USBSTOR_DataCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PDEVICE_OBJECT FdoDevice;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PSCSI_REQUEST_BLOCK CurrentSrb;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    DPRINT("USBSTOR_DataCompletion: ... \n");

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FdoDevice = PDODeviceExtension->LowerDeviceObject;
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    Srb = IoStack->Parameters.Scsi.Srb;

    if (Srb == FDODeviceExtension->CurrentSrb &&
        FDODeviceExtension->Urb.TransferBufferMDL != Irp->MdlAddress)
    {
        IoFreeMdl(FDODeviceExtension->Urb.TransferBufferMDL);
    }

    if (USBSTOR_IsRequestTimeOut(FdoDevice, Irp, &Status))
    {
        return Status;
    }

    if (NT_SUCCESS(Irp->IoStatus.Status))
    {
        if (FDODeviceExtension->Urb.TransferBufferLength < Srb->DataTransferLength)
        {
            Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        }
        else
        {
            Srb->SrbStatus = SRB_STATUS_SUCCESS;
        }

        Srb->DataTransferLength = FDODeviceExtension->Urb.TransferBufferLength;

        USBSTOR_CswTransfer(FDODeviceExtension, Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (USBD_STATUS(FDODeviceExtension->Urb.Hdr.Status) == 
        USBD_STATUS(USBD_STATUS_STALL_PID))
    {
        ++FDODeviceExtension->RetryCount;

        Srb->DataTransferLength = FDODeviceExtension->Urb.TransferBufferLength;
        Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;

        USBSTOR_BulkQueueResetPipe(FDODeviceExtension);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    CurrentSrb = FDODeviceExtension->CurrentSrb;
    IoStack->Parameters.Scsi.Srb = CurrentSrb;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;

    CurrentSrb->SrbStatus = SRB_STATUS_BUS_RESET;

    USBSTOR_HandleCDBComplete(FDODeviceExtension, Irp, CurrentSrb);
    USBSTOR_QueueResetDevice(FDODeviceExtension, PDODeviceExtension);

    return STATUS_IO_DEVICE_ERROR;
}

NTSTATUS
NTAPI
USBSTOR_DataTransfer(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp)
{
    PSCSI_REQUEST_BLOCK Srb;
    PUSBD_PIPE_INFORMATION Pipe;
    PMDL Mdl;
    ULONG TransferFlags;
    PVOID TransferBuffer;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    UCHAR PipeIndex;

    DPRINT("USBSTOR_DataTransfer: ... \n");

    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    Srb = IoStack->Parameters.Scsi.Srb;

    if ((Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_IN)
    {
        PipeIndex = FDODeviceExtension->BulkInPipeIndex;
        TransferFlags = USBD_SHORT_TRANSFER_OK;
    }
    else if ((Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_OUT)
    {
        PipeIndex = FDODeviceExtension->BulkOutPipeIndex;
        TransferFlags = 0;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    Mdl = NULL;
    TransferBuffer = NULL;

    Pipe = &FDODeviceExtension->InterfaceInformation->Pipes[PipeIndex];

    if (Srb == FDODeviceExtension->CurrentSrb)
    {
        if (MmGetMdlVirtualAddress(Irp->MdlAddress) == Srb->DataBuffer)
        {
            Mdl = Irp->MdlAddress;
        }
        else
        {
            Mdl = IoAllocateMdl(Srb->DataBuffer,
                                Srb->DataTransferLength,
                                FALSE,
                                FALSE,
                                NULL);
    
            if (Mdl)
            {
                IoBuildPartialMdl(Irp->MdlAddress,
                                  Mdl,
                                  Srb->DataBuffer,
                                  Srb->DataTransferLength);
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (!Mdl)
        {
            DPRINT("USBSTOR_DataTransfer: Mdl - %p\n", Mdl);
            return Status;
        }
    }
    else
    {
        TransferBuffer = Srb->DataBuffer;

        if (!Srb->DataBuffer)
        {
            DPRINT("USBSTOR_DataTransfer: Srb->DataBuffer == NULL!!!\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    USBSTOR_IssueBulkOrInterruptRequest(FDODeviceExtension,
                                        Irp,
                                        Pipe->PipeHandle,
                                        TransferFlags,
                                        Srb->DataTransferLength,
                                        TransferBuffer,
                                        Mdl,
                                        USBSTOR_DataCompletion,
                                        NULL);//Context

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBSTOR_CbwCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT FdoDevice;
    PSCSI_REQUEST_BLOCK CurrentSrb;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    NTSTATUS Status;
    PSCSI_REQUEST_BLOCK Srb;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;

    DPRINT("USBSTOR_CbwCompletion: ... \n");

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FdoDevice = PDODeviceExtension->LowerDeviceObject;
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    Srb = IoStack->Parameters.Scsi.Srb;

    if (USBSTOR_IsRequestTimeOut(FdoDevice, Irp, &Status))
    {
        return Status;
    }

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        CurrentSrb = FDODeviceExtension->CurrentSrb;
        IoStack->Parameters.Scsi.Srb = CurrentSrb;

        Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Information = 0;

        CurrentSrb->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_HandleCDBComplete(FDODeviceExtension, Irp, CurrentSrb);
        USBSTOR_QueueResetDevice(FDODeviceExtension, PDODeviceExtension);

        return STATUS_IO_DEVICE_ERROR;
    }

    if (Irp->MdlAddress || Srb != FDODeviceExtension->CurrentSrb)
    {
        Status = USBSTOR_DataTransfer(FDODeviceExtension, Irp);

        if (!NT_SUCCESS(Status))
        {
            CurrentSrb = FDODeviceExtension->CurrentSrb;
            IoStack->Parameters.Scsi.Srb = CurrentSrb;

            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;

            CurrentSrb->SrbStatus = SRB_STATUS_ERROR;

            USBSTOR_HandleCDBComplete(FDODeviceExtension, Irp, CurrentSrb);
            USBSTOR_QueueResetDevice(FDODeviceExtension, PDODeviceExtension);

            return Status;
        }
    }
    else
    {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        USBSTOR_CswTransfer(FDODeviceExtension, Irp);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBSTOR_CbwTransfer(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PCBW Cbw;
    PUSBD_PIPE_INFORMATION Pipe;
    UCHAR PipeIndex;

    DPRINT("USBSTOR_CbwTransfer: ... \n");

    FDODeviceExtension->RetryCount = 0;

    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    PDODeviceExtension = IoStack->DeviceObject->DeviceExtension;
    Srb = IoStack->Parameters.Scsi.Srb;

    Cbw = &FDODeviceExtension->BulkBuffer.Cbw;

    Cbw->Signature = CBW_SIGNATURE;
    Cbw->Tag = (ULONG)Irp;
    Cbw->DataTransferLength = Srb->DataTransferLength;
    Cbw->Flags = ((UCHAR)Srb->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) << 1;
    Cbw->LUN = PDODeviceExtension->LUN;
    Cbw->CommandBlockLength = Srb->CdbLength;

    RtlCopyMemory(FDODeviceExtension->BulkBuffer.Cbw.CommandBlock,
                  Srb->Cdb,
                  sizeof(CDB));

    PipeIndex = FDODeviceExtension->BulkOutPipeIndex;
    Pipe = &FDODeviceExtension->InterfaceInformation->Pipes[PipeIndex];

    return USBSTOR_IssueBulkOrInterruptRequest(FDODeviceExtension,
                                               Irp,
                                               Pipe->PipeHandle,
                                               0,//TransferFlags ?USBD_TRANSFER_DIRECTION_OUT? FIXME
                                               sizeof(CBW),
                                               &FDODeviceExtension->BulkBuffer.Cbw,
                                               NULL,//TransferBufferMDL
                                               USBSTOR_CbwCompletion,
                                               NULL);//Context
}

NTSTATUS
NTAPI
USBSTOR_IssueRequestSense(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK CurrentSrb;
    PSCSI_REQUEST_BLOCK SenseSrb;

    DPRINT("USBSTOR_IssueRequestSense: \n");

    CurrentSrb = FDODeviceExtension->CurrentSrb;
    SenseSrb = &FDODeviceExtension->SenseSrb;
    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    IoStack->Parameters.Scsi.Srb = SenseSrb;

    RtlZeroMemory(SenseSrb, sizeof(FDODeviceExtension->SenseSrb));

    SenseSrb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    SenseSrb->Length = sizeof(FDODeviceExtension->SenseSrb);
    SenseSrb->CdbLength = 12;

    SenseSrb->SrbFlags = SRB_FLAGS_DATA_IN |
                         SRB_FLAGS_NO_QUEUE_FREEZE |
                         SRB_FLAGS_DISABLE_AUTOSENSE;

    SenseSrb->DataTransferLength = CurrentSrb->SenseInfoBufferLength;
    SenseSrb->DataBuffer = CurrentSrb->SenseInfoBuffer;

    SenseSrb->Cdb[0] = SCSIOP_REQUEST_SENSE;
    SenseSrb->Cdb[4] = CurrentSrb->SenseInfoBufferLength;

    return USBSTOR_CbwTransfer(FDODeviceExtension, Irp);
}

VOID
USBSTOR_HandleExecuteSCSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG RetryCount)
{
    PCDB pCDB;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get device extensions
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDODeviceExtension = PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get request block
    //
    Request = IoStack->Parameters.Scsi.Srb;

    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;

    DPRINT("USBSTOR_HandleExecuteSCSI Operation Code %x\n", pCDB->AsByte[0]);


    if (pCDB->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE)
    {
        //FIXME CheckModeSenseDataProtect

        DPRINT("SCSIOP_MODE_SENSE DataTransferLength %lu\n", Request->DataTransferLength);
        ASSERT(pCDB->MODE_SENSE.AllocationLength == Request->DataTransferLength);
        ASSERT(Request->DataBuffer);

        //
        // send mode sense command
        //
        Status = USBSTOR_SendModeSense(DeviceObject, Irp, RetryCount);
        return;
    }

    if (FDODeviceExtension->DriverFlags == 1) // BulkOnly (FIXME const)
    {
        Status = USBSTOR_CbwTransfer(FDODeviceExtension, Irp);
        DPRINT("USBSTOR_HandleExecuteSCSI: Status - %08X\n", Status);
        return;
    }

    DPRINT1("USBSTOR_HandleExecuteSCSI: Not BulkOnly. UNIMPLEMENTED. FIXME. \n");

    ASSERT(FALSE);

#if 0 // Old code
    if (pCDB->AsByte[0] == SCSIOP_READ_CAPACITY)
    {
        //
        // sanity checks
        //
        ASSERT(Request->DataBuffer);

        DPRINT("SCSIOP_READ_CAPACITY Length %lu\n", Request->DataTransferLength);
        Status = USBSTOR_SendCapacity(DeviceObject, Irp, RetryCount);
    }
    else if (pCDB->AsByte[0] == SCSIOP_READ_FORMATTED_CAPACITY)
    {
        DPRINT("SCSIOP_READ_FORMATTED_CAPACITY DataTransferLength %lu\n", Request->DataTransferLength);

        //
        // send read format capacity
        //
        Status = USBSTOR_SendFormatCapacity(DeviceObject, Irp, RetryCount);
    }
    else if (pCDB->AsByte[0] == SCSIOP_INQUIRY)
    {
        DPRINT("SCSIOP_INQUIRY DataTransferLength %lu\n", Request->DataTransferLength);

        //
        // send read format capacity
        //
        Status = USBSTOR_SendInquiry(DeviceObject, Irp, RetryCount);
    }
    else if (pCDB->MODE_SENSE.OperationCode == SCSIOP_READ ||  pCDB->MODE_SENSE.OperationCode == SCSIOP_WRITE)
    {
        DPRINT("SCSIOP_READ / SCSIOP_WRITE DataTransferLength %lu\n", Request->DataTransferLength);

        //
        // send read / write command
        //
        Status = USBSTOR_SendReadWrite(DeviceObject, Irp, RetryCount);
    }
    else if (pCDB->AsByte[0] == SCSIOP_MEDIUM_REMOVAL)
    {
        DPRINT("SCSIOP_MEDIUM_REMOVAL\n");

        //
        // just complete the request
        //
        Request->SrbStatus = SRB_STATUS_SUCCESS;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = Request->DataTransferLength;
        USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // start next request
        //
        USBSTOR_QueueNextRequest(PDODeviceExtension->LowerDeviceObject);

        return;// STATUS_SUCCESS;
    }
    else if (pCDB->MODE_SENSE.OperationCode == SCSIOP_TEST_UNIT_READY)
    {
        DPRINT("SCSIOP_TEST_UNIT_READY\n");

        //
        // send test unit command
        //
        Status = USBSTOR_SendTestUnit(DeviceObject, Irp, RetryCount);
    }
    else if (pCDB->CDB6INQUIRY.OperationCode == SCSIOP_REQUEST_SENSE)
    {
        DPRINT("SCSIOP_REQUEST_SENSE\n");

        //
        // send test unit command
        //
        Status = USBSTOR_SendUnknownRequest(DeviceObject, Irp, RetryCount);
    }
    else
    {
        // Unknown request. Simply forward
        DPRINT1("Forwarding unknown Operation Code %x\n", pCDB->AsByte[0]);
        Status = USBSTOR_SendUnknownRequest(DeviceObject, Irp, RetryCount);
    }

    DPRINT("USBSTOR_HandleExecuteSCSI: Status - %08X\n", Status);
#endif
}
