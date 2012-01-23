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
    Control->Flags = 0x80;
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
    PERRORHANDLER_WORKITEM_DATA ErrorHandlerWorkItemData;
    NTSTATUS Status;
    PURB Urb;

    DPRINT("USBSTOR_CSWCompletionRoutine Irp %p Ctx %p\n", Irp, Ctx);

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

    if (Context->Irp)
    {
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

        Urb = &Context->Urb;

        //
        // get SCSI command data block
        //
        pCDB = (PCDB)Request->Cdb;

        //
        // check status
        //
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Status %x\n", Status);
            DPRINT1("UrbStatus %x\n", Urb->UrbHeader.Status);

            //
            // Check for errors that can be handled
            // FIXME: Verify all usb errors that can be recovered via pipe reset/port reset/controller reset
            //
            if ((Urb->UrbHeader.Status & USB_RECOVERABLE_ERRORS) == Urb->UrbHeader.Status)
            {
                DPRINT1("Attempting Error Recovery\n");
                //
                // If a Read Capacity Request free TransferBuffer
                //
                if (pCDB->AsByte[0] == SCSIOP_READ_CAPACITY)
                {
                    FreeItem(Context->TransferData);
                }

                //
                // Clean up the rest
                //
                FreeItem(Context->cbw);
                FreeItem(Context);

                //
                // Allocate Work Item Data
                //
                ErrorHandlerWorkItemData = ExAllocatePoolWithTag(NonPagedPool, sizeof(ERRORHANDLER_WORKITEM_DATA), USB_STOR_TAG);
                if (!ErrorHandlerWorkItemData)
                {
                    DPRINT1("Failed to allocate memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    //
                    // Initialize and queue the work item to handle the error
                    //
                    ExInitializeWorkItem(&ErrorHandlerWorkItemData->WorkQueueItem,
                                        ErrorHandlerWorkItemRoutine,
                                        ErrorHandlerWorkItemData);
    
                    ErrorHandlerWorkItemData->DeviceObject = Context->FDODeviceExtension->FunctionalDeviceObject;
                    ErrorHandlerWorkItemData->Irp = Irp;
                    ErrorHandlerWorkItemData->Context = Context;
                    DPRINT1("Queuing WorkItemROutine\n");
                    ExQueueWorkItem(&ErrorHandlerWorkItemData->WorkQueueItem, DelayedWorkQueue);

                    return STATUS_MORE_PROCESSING_REQUIRED;
                }
            }
        }

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
    }

    //
    // free cbw
    //
    FreeItem(Context->cbw);


    if (Context->Irp)
    {
        //
        // FIXME: check status
        //
        Context->Irp->IoStatus.Status = Irp->IoStatus.Status;
        Context->Irp->IoStatus.Information = Context->TransferDataLength;

        //
        // complete request
        //
        IoCompleteRequest(Context->Irp, IO_NO_INCREMENT);

        //
        // terminate current request
        //
        USBSTOR_QueueTerminateRequest(Context->PDODeviceExtension->LowerDeviceObject, TRUE);

        //
        // start next request
        //
        USBSTOR_QueueNextRequest(Context->PDODeviceExtension->LowerDeviceObject);
    }

    if (Context->Event)
    {
        //
        // signal event
        //
        KeSetEvent(Context->Event, 0, FALSE);
    }


    //
    // free context
    //
    FreeItem(Context);

    //
    // done
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
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
    PIO_STACK_LOCATION IoStack;

    DPRINT("USBSTOR_DataCompletionRoutine Irp %p Ctx %p\n", Irp, Ctx);

    //
    // access context
    //
    Context = (PIRP_CONTEXT)Ctx;

    //
    // get next stack location
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

    DPRINT("USBSTOR_CBWCompletionRoutine Irp %p Ctx %p\n", Irp, Ctx);

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
                                               USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
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
USBSTOR_SendRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP OriginalRequest,
    IN OPTIONAL PKEVENT Event,
    IN UCHAR CommandLength,
    IN PUCHAR Command,
    IN ULONG TransferDataLength,
    IN PUCHAR TransferData)
{
    PIRP_CONTEXT Context;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
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
    USBSTOR_BuildCBW(0xDEADDEAD, // FIXME tag
                     TransferDataLength,
                     PDODeviceExtension->LUN,
                     CommandLength,
                     Command,
                     Context->cbw);

    //
    // now initialize the urb
    //
    UsbBuildInterruptOrBulkTransferRequest(&Context->Urb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle,
                                           Context->cbw,
                                           NULL,
                                           sizeof(CBW),
                                           USBD_TRANSFER_DIRECTION_OUT | USBD_SHORT_TRANSFER_OK,
                                           NULL);

    //
    // initialize rest of context
    //
    Context->Irp = OriginalRequest;
    Context->TransferData = TransferData;
    Context->TransferDataLength = TransferDataLength;
    Context->FDODeviceExtension = FDODeviceExtension;
    Context->PDODeviceExtension = PDODeviceExtension;
    Context->Event = Event;

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

    if (OriginalRequest)
    {
        //
        // mark orignal irp as pending
        //
        IoMarkIrpPending(OriginalRequest);
    }

    //
    // call driver
    //
    IoCallDriver(FDODeviceExtension->LowerDeviceObject, Irp);

    //
    // done
    //
    return STATUS_PENDING;
}

NTSTATUS
USBSTOR_SendInquiryCmd(
    IN PDEVICE_OBJECT DeviceObject)
{
    UFI_INQUIRY_CMD Cmd;
    NTSTATUS Status;
    KEVENT Event;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PUFI_INQUIRY_RESPONSE Response;


    //
    // allocate inquiry response
    //
    Response = AllocateItem(NonPagedPool, PAGE_SIZE);
    if (!Response)
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
    // initialize inquiry cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_INQUIRY_CMD));
    Cmd.Code = SCSIOP_INQUIRY;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.AllocationLength = sizeof(UFI_INQUIRY_RESPONSE);

    //
    // initialize event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // now send the request
    //
    Status = USBSTOR_SendRequest(DeviceObject, NULL, &Event, UFI_INQUIRY_CMD_LEN, (PUCHAR)&Cmd, sizeof(UFI_INQUIRY_RESPONSE), (PUCHAR)Response);

    //
    // wait for the action to complete
    //
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    DPRINT1("Response %p\n", Response);
    DPRINT1("DeviceType %x\n", Response->DeviceType);
    DPRINT1("RMB %x\n", Response->RMB);
    DPRINT1("Version %x\n", Response->Version);
    DPRINT1("Format %x\n", Response->Format);
    DPRINT1("Length %x\n", Response->Length);
    DPRINT1("Reserved %x\n", Response->Reserved);
    DPRINT1("Vendor %c%c%c%c%c%c%c%c\n", Response->Vendor[0], Response->Vendor[1], Response->Vendor[2], Response->Vendor[3], Response->Vendor[4], Response->Vendor[5], Response->Vendor[6], Response->Vendor[7]);
    DPRINT1("Product %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", Response->Product[0], Response->Product[1], Response->Product[2], Response->Product[3],
                                                          Response->Product[4], Response->Product[5], Response->Product[6], Response->Product[7], 
                                                          Response->Product[8], Response->Product[9], Response->Product[10], Response->Product[11],
                                                          Response->Product[12], Response->Product[13], Response->Product[14], Response->Product[15]);

    DPRINT1("Revision %c%c%c%c\n", Response->Revision[0], Response->Revision[1], Response->Revision[2], Response->Revision[3]);

    //
    // store inquiry data
    //
    PDODeviceExtension->InquiryData = (PVOID)Response;

    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_SendCapacityCmd(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
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
    Response = (PUFI_CAPACITY_RESPONSE)AllocateItem(NonPagedPool, sizeof(UFI_CAPACITY_RESPONSE));
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
    return USBSTOR_SendRequest(DeviceObject, Irp, NULL, UFI_INQUIRY_CMD_LEN, (PUCHAR)&Cmd, sizeof(UFI_CAPACITY_RESPONSE), (PUCHAR)Response);
}

NTSTATUS
USBSTOR_SendModeSenseCmd(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
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
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    //
    // terminate current request
    //
    USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, TRUE);

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
    UNIMPLEMENTED

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
USBSTOR_SendReadWriteCmd(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UFI_READ_WRITE_CMD Cmd;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PCDB pCDB;
    ULONG BlockCount;
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
    DPRINT("USBSTOR_SendReadWriteCmd DataTransferLength %lu, BlockLength %lu\n", Request->DataTransferLength, PDODeviceExtension->BlockLength);

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

    DPRINT("USBSTOR_SendReadWriteCmd BlockAddress %x%x%x%x BlockCount %lu BlockLength %lu\n", Cmd.LogicalBlockByte0, Cmd.LogicalBlockByte1, Cmd.LogicalBlockByte2, Cmd.LogicalBlockByte3, BlockCount, PDODeviceExtension->BlockLength);

    //
    // send request
    //
    return USBSTOR_SendRequest(DeviceObject, Irp, NULL, UFI_READ_WRITE_CMD_LEN, (PUCHAR)&Cmd, Request->DataTransferLength, (PUCHAR)Request->DataBuffer);
}

NTSTATUS
USBSTOR_SendTestUnitCmd(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
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
    return USBSTOR_SendRequest(DeviceObject, Irp, NULL, UFI_TEST_UNIT_CMD_LEN, (PUCHAR)&Cmd, 0, NULL);
}


NTSTATUS
USBSTOR_HandleExecuteSCSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCDB pCDB;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;

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

    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;

    DPRINT("USBSTOR_HandleExecuteSCSI Operation Code %x\n", pCDB->AsByte[0]);

    if (pCDB->AsByte[0] == SCSIOP_READ_CAPACITY)
    {
        //
        // sanity checks
        //
        ASSERT(Request->DataBuffer);

        DPRINT("SCSIOP_READ_CAPACITY Length %\n", Request->DataTransferLength);
        Status = USBSTOR_SendCapacityCmd(DeviceObject, Irp);
    }
    else if (pCDB->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE)
    {
        DPRINT1("SCSIOP_MODE_SENSE DataTransferLength %lu\n", Request->DataTransferLength);
        ASSERT(pCDB->MODE_SENSE.AllocationLength == Request->DataTransferLength);
        ASSERT(Request->DataBuffer);

        //
        // send mode sense command
        //
        Status = USBSTOR_SendModeSenseCmd(DeviceObject, Irp);
    }
    else if (pCDB->MODE_SENSE.OperationCode == SCSIOP_READ ||  pCDB->MODE_SENSE.OperationCode == SCSIOP_WRITE)
    {
        DPRINT("SCSIOP_READ / SCSIOP_WRITE DataTransferLength %lu\n", Request->DataTransferLength);

        //
        // send read / write command
        //
        Status = USBSTOR_SendReadWriteCmd(DeviceObject, Irp);
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
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // terminate current request
        //
        USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, TRUE);

        //
        // start next request
        //
        USBSTOR_QueueNextRequest(PDODeviceExtension->LowerDeviceObject);

        return STATUS_SUCCESS;
    }
    else if (pCDB->MODE_SENSE.OperationCode == SCSIOP_TEST_UNIT_READY)
    {
        DPRINT("SCSIOP_TEST_UNIT_READY\n");

        //
        // send test unit command
        //
        Status = USBSTOR_SendTestUnitCmd(DeviceObject, Irp);
    }
    else
    {
        UNIMPLEMENTED;
        Request->SrbStatus = SRB_STATUS_ERROR;
        Status = STATUS_NOT_SUPPORTED;
        DbgBreakPoint();
    }

    return Status;
}
