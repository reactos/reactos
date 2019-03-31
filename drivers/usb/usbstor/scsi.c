/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     USB block storage device driver.
 * COPYRIGHT:   2005-2006 James Tabor
 *              2011-2012 Michael Martin (michael.martin@reactos.org)
 *              2011-2013 Johannes Anderwald (johannes.anderwald@reactos.org)
 *              2017 Vadim Galyant
 *              2019 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>


static
NTSTATUS
USBSTOR_SrbStatusToNtStatus(
    IN PSCSI_REQUEST_BLOCK Srb)
{
    UCHAR SrbStatus;

    SrbStatus = SRB_STATUS(Srb->SrbStatus);

    switch (SrbStatus)
    {
        case SRB_STATUS_SUCCESS:
            return STATUS_SUCCESS;

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

NTSTATUS
USBSTOR_BuildCBW(
    IN ULONG Tag,
    IN ULONG DataTransferLength,
    IN UCHAR LUN,
    IN UCHAR CommandBlockLength,
    IN PUCHAR CommandBlock,
    IN OUT PCBW Control)
{
    ASSERT(CommandBlockLength <= 16);

    Control->Signature = CBW_SIGNATURE;
    Control->Tag = Tag;
    Control->DataTransferLength = DataTransferLength;
    Control->Flags = (CommandBlock[0] != SCSIOP_WRITE) ? 0x80 : 0x00;
    Control->LUN = (LUN & MAX_LUN);
    Control->CommandBlockLength = CommandBlockLength;

    RtlCopyMemory(Control->CommandBlock, CommandBlock, CommandBlockLength);

    return STATUS_SUCCESS;
}

PIRP_CONTEXT
USBSTOR_AllocateIrpContext()
{
    PIRP_CONTEXT Context;

    Context = (PIRP_CONTEXT)AllocateItem(NonPagedPool, sizeof(IRP_CONTEXT));
    if (!Context)
    {
        return NULL;
    }

    Context->cbw = (PCBW)AllocateItem(NonPagedPool, 512);
    if (!Context->cbw)
    {
        FreeItem(Context);
        return NULL;
    }

    return Context;
}

static
BOOLEAN
USBSTOR_IsCSWValid(
    PIRP_CONTEXT Context)
{
    if (Context->csw->Signature != CSW_SIGNATURE)
    {
        DPRINT1("[USBSTOR] Expected Signature %x but got %x\n", CSW_SIGNATURE, Context->csw->Signature);
        return FALSE;
    }

    if (Context->csw->Tag != (ULONG_PTR)Context->csw)
    {
        DPRINT1("[USBSTOR] Expected Tag %Ix but got %x\n", (ULONG_PTR)Context->csw, Context->csw->Tag);
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
USBSTOR_QueueWorkItem(
    PIRP_CONTEXT Context,
    PIRP Irp)
{
    PERRORHANDLER_WORKITEM_DATA ErrorHandlerWorkItemData;

    ErrorHandlerWorkItemData = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERRORHANDLER_WORKITEM_DATA), USB_STOR_TAG);
    if (!ErrorHandlerWorkItemData)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // error handling started
    Context->FDODeviceExtension->SrbErrorHandlingActive = TRUE;

    // srb error handling finished
    Context->FDODeviceExtension->TimerWorkQueueEnabled = FALSE;

    // Initialize and queue the work item to handle the error
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
    PUFI_CAPACITY_RESPONSE Response;
    NTSTATUS Status;

    Context = (PIRP_CONTEXT)Ctx;

    if (Context->TransferBufferMDL && Context->TransferBufferMDL != Context->Irp->MdlAddress)
    {
        IoFreeMdl(Context->TransferBufferMDL);
    }

    DPRINT("USBSTOR_CSWCompletionRoutine Irp %p Ctx %p Status %x\n", Irp, Ctx, Irp->IoStatus.Status);

    // first check for Irp errors
    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        if (USBD_STATUS(Context->Urb.UrbHeader.Status) == USBD_STATUS(USBD_STATUS_STALL_PID))
        {
            if (Context->RetryCount < 2)
            {
                ++Context->RetryCount;

                // clear stall and resend cbw
                Context->ErrorIndex = 1;
                Status = USBSTOR_QueueWorkItem(Context, Irp);
                ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
        else
        {
            DPRINT1("USBSTOR_CSWCompletionRoutine: Urb.Hdr.Status - %x\n", Context->Urb.UrbHeader.Status);
        }

        // perform reset recovery
        Context->ErrorIndex = 2;
        Status = USBSTOR_QueueWorkItem(Context, NULL);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // now check the CSW packet validity
    if (!USBSTOR_IsCSWValid(Context) || Context->csw->Status == CSW_STATUS_PHASE_ERROR)
    {
        // perform reset recovery
        Context->ErrorIndex = 2;
        Status = USBSTOR_QueueWorkItem(Context, NULL);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Request = IoStack->Parameters.Scsi.Srb;
    ASSERT(Request);

    // finally check for CSW errors
    if (Context->csw->Status == CSW_STATUS_COMMAND_PASSED)
    {
        // read capacity needs special work
        if (Request->Cdb[0] == SCSIOP_READ_CAPACITY)
        {
            // get output buffer
            Response = (PUFI_CAPACITY_RESPONSE)Context->TransferData;

            // store in pdo
            Context->PDODeviceExtension->BlockLength = NTOHL(Response->BlockLength);
            Context->PDODeviceExtension->LastLogicBlockAddress = NTOHL(Response->LastLogicalBlockAddress);
        }

        Status = USBSTOR_SrbStatusToNtStatus(Request);
    }
    else if (Context->csw->Status == CSW_STATUS_COMMAND_FAILED)
    {
        // the command is correct but with failed status - issue request sense
        DPRINT("USBSTOR_CSWCompletionRoutine: CSW_STATUS_COMMAND_FAILED\n");

        ASSERT(Context->FDODeviceExtension->ActiveSrb == Request);
        
        // setting a generic error status, additional information
        // should be read by higher-level driver from SenseInfoBuffer
        Request->SrbStatus = SRB_STATUS_ERROR;
        Request->ScsiStatus = 2;
        Request->DataTransferLength = 0;

        DPRINT("Flags: %x SBL: %x, buf: %p\n", Request->SrbFlags, Request->SenseInfoBufferLength, Request->SenseInfoBuffer);

        if (!(Request->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) &&
              Request->SenseInfoBufferLength &&
              Request->SenseInfoBuffer)
        {
            // TODO: issue request sense
        }

        Status = STATUS_IO_DEVICE_ERROR;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Request->DataTransferLength;

    FreeItem(Context->cbw);

    // terminate current request
    USBSTOR_QueueTerminateRequest(Context->PDODeviceExtension->LowerDeviceObject, Irp);
    USBSTOR_QueueNextRequest(Context->PDODeviceExtension->LowerDeviceObject);

    FreeItem(Context);
    return Status;
}

VOID
USBSTOR_SendCSW(
    PIRP_CONTEXT Context,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetNextIrpStackLocation(Irp);

    // now initialize the urb for sending the csw
    UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle,
                                           Context->csw,
                                           NULL,
                                           512, //FIXME
                                           USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    // initialize stack location
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)&Context->Urb;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = Context->Urb.UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSetCompletionRoutine(Irp, USBSTOR_CSWCompletionRoutine, Context, TRUE, TRUE, TRUE);

    IoCallDriver(Context->FDODeviceExtension->LowerDeviceObject, Irp);
}

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

    Context = (PIRP_CONTEXT)Ctx;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK Request = IoStack->Parameters.Scsi.Srb;

    if (NT_SUCCESS(Irp->IoStatus.Status))
    {
        if (Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferLength < Request->DataTransferLength)
        {
            Request->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        }
        else
        {
            Request->SrbStatus = SRB_STATUS_SUCCESS;
        }

        Request->DataTransferLength = Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferLength;
        USBSTOR_SendCSW(Context, Irp);
    }
    else if (USBD_STATUS(Context->Urb.UrbHeader.Status) == USBD_STATUS(USBD_STATUS_STALL_PID))
    {
        ++Context->RetryCount;

        Request->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        Request->DataTransferLength = Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferLength;

        // clear stall and resend cbw
        Context->ErrorIndex = 1;
        Status = USBSTOR_QueueWorkItem(Context, Irp);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
    }
    else
    {
        // perform reset recovery
        Context->ErrorIndex = 2;
        Status = USBSTOR_QueueWorkItem(Context, NULL);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

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
    PSCSI_REQUEST_BLOCK Request;
    UCHAR Code;
    USBD_PIPE_HANDLE PipeHandle;

    DPRINT("USBSTOR_CBWCompletionRoutine Irp %p Ctx %p Status %x\n", Irp, Ctx, Irp->IoStatus.Status);

    Context = (PIRP_CONTEXT)Ctx;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Request = IoStack->Parameters.Scsi.Srb;

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        // perform reset recovery
        Context->ErrorIndex = 2;
        Status = USBSTOR_QueueWorkItem(Context, NULL);
        ASSERT(Status == STATUS_MORE_PROCESSING_REQUIRED);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // is there data to be submitted
    if (Context->TransferDataLength)
    {
        // get command code
        Code = Context->cbw->CommandBlock[0];

        if (Code == SCSIOP_WRITE)
        {
            // write request - use bulk out pipe
            PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkOutPipeIndex].PipeHandle;
        }
        else
        {
            // default bulk in pipe
            PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle;
        }

        // now initialize the urb for sending data
        UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                               sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                               PipeHandle,
                                               NULL,
                                               Context->TransferBufferMDL,
                                               Context->TransferDataLength,
                                               ((Code == SCSIOP_WRITE) ? USBD_TRANSFER_DIRECTION_OUT : (USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK)),
                                               NULL);

        IoSetCompletionRoutine(Irp, USBSTOR_DataCompletionRoutine, Context, TRUE, TRUE, TRUE);
    }
    else
    {
        if (NT_SUCCESS(Irp->IoStatus.Status))
        {
            Request->SrbStatus = SRB_STATUS_SUCCESS;
        }

        // now initialize the urb for sending the csw
        UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                               sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                               Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle,
                                               Context->csw,
                                               NULL,
                                               512, //FIXME
                                               USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
                                               NULL);

        IoSetCompletionRoutine(Irp, USBSTOR_CSWCompletionRoutine, Context, TRUE, TRUE, TRUE);
    }

    IoStack = IoGetNextIrpStackLocation(Irp);

    // initialize stack location
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)&Context->Urb;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = Context->Urb.UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCallDriver(Context->FDODeviceExtension->LowerDeviceObject, Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
DumpCBW(
    PUCHAR Block)
{
    DPRINT("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        Block[0] & 0xFF, Block[1] & 0xFF, Block[2] & 0xFF, Block[3] & 0xFF, Block[4] & 0xFF, Block[5] & 0xFF, Block[6] & 0xFF, Block[7] & 0xFF, Block[8] & 0xFF, Block[9] & 0xFF,
        Block[10] & 0xFF, Block[11] & 0xFF, Block[12] & 0xFF, Block[13] & 0xFF, Block[14] & 0xFF, Block[15] & 0xFF, Block[16] & 0xFF, Block[17] & 0xFF, Block[18] & 0xFF, Block[19] & 0xFF,
        Block[20] & 0xFF, Block[21] & 0xFF, Block[22] & 0xFF, Block[23] & 0xFF, Block[24] & 0xFF, Block[25] & 0xFF, Block[26] & 0xFF, Block[27] & 0xFF, Block[28] & 0xFF, Block[29] & 0xFF,
        Block[30] & 0xFF);

}

NTSTATUS
USBSTOR_SendCBW(
    PIRP_CONTEXT Context,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetNextIrpStackLocation(Irp);

    // initialize stack location
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)&Context->Urb;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = Context->Urb.UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSetCompletionRoutine(Irp, USBSTOR_CBWCompletionRoutine, Context, TRUE, TRUE, TRUE);

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
    PUCHAR MdlVirtualAddress;

    Context = USBSTOR_AllocateIrpContext();
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    USBSTOR_BuildCBW(PtrToUlong(Context->cbw),
                     TransferDataLength,
                     PDODeviceExtension->LUN,
                     CommandLength,
                     Command,
                     Context->cbw);

    DPRINT("CBW %p\n", Context->cbw);
    DumpCBW((PUCHAR)Context->cbw);

    // now initialize the urb
    UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle,
                                           Context->cbw,
                                           NULL,
                                           sizeof(CBW),
                                           USBD_TRANSFER_DIRECTION_OUT,
                                           NULL);

    // initialize rest of context
    Context->Irp = OriginalRequest;
    Context->TransferData = TransferData;
    Context->TransferDataLength = TransferDataLength;
    Context->FDODeviceExtension = FDODeviceExtension;
    Context->PDODeviceExtension = PDODeviceExtension;
    Context->RetryCount = RetryCount;

    // is there transfer data
    if (Context->TransferDataLength)
    {
        // check if the original request already does have an mdl associated
        if ((OriginalRequest->MdlAddress != NULL) &&
            (Context->TransferData == NULL || Command[0] == SCSIOP_READ || Command[0] == SCSIOP_WRITE))
        {
            // Sanity check that the Mdl does describe the TransferData for read/write
            if (CommandLength == UFI_READ_WRITE_CMD_LEN)
            {
                MdlVirtualAddress = MmGetMdlVirtualAddress(OriginalRequest->MdlAddress);

                // is there an offset
                if (MdlVirtualAddress != Context->TransferData)
                {
                    // lets build an mdl
                    Context->TransferBufferMDL = IoAllocateMdl(Context->TransferData, MmGetMdlByteCount(OriginalRequest->MdlAddress), FALSE, FALSE, NULL);
                    if (!Context->TransferBufferMDL)
                    {
                        FreeItem(Context->cbw);
                        FreeItem(Context);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    IoBuildPartialMdl(OriginalRequest->MdlAddress, Context->TransferBufferMDL, Context->TransferData, Context->TransferDataLength);
                }
            }

            if (!Context->TransferBufferMDL)
            {
                // I/O paging request
                Context->TransferBufferMDL = OriginalRequest->MdlAddress;
            }
        }
        else
        {
            // allocate mdl for buffer, buffer must be allocated from NonPagedPool
            Context->TransferBufferMDL = IoAllocateMdl(Context->TransferData, Context->TransferDataLength, FALSE, FALSE, NULL);
            if (!Context->TransferBufferMDL)
            {
                FreeItem(Context->cbw);
                FreeItem(Context);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            MmBuildMdlForNonPagedPool(Context->TransferBufferMDL);
        }
    }

    return USBSTOR_SendCBW(Context, OriginalRequest);
}

NTSTATUS
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

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Request = IoStack->Parameters.Scsi.Srb;
    pCDB = (PCDB)Request->Cdb;

    DPRINT("USBSTOR_HandleExecuteSCSI Operation Code %x, Length %lu\n", pCDB->CDB10.OperationCode, Request->DataTransferLength);

    // check that we're sending to the right LUN
    ASSERT(pCDB->CDB10.LogicalUnitNumber == (PDODeviceExtension->LUN & MAX_LUN));
    Status = USBSTOR_SendRequest(DeviceObject, Irp, Request->CdbLength, (PUCHAR)pCDB, Request->DataTransferLength, Request->DataBuffer, RetryCount);

    return Status;
}
