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

static
NTSTATUS
USBSTOR_IssueBulkOrInterruptRequest(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp,
    IN USBD_PIPE_HANDLE PipeHandle,
    IN ULONG TransferFlags,
    IN ULONG TransferBufferLength,
    IN PVOID TransferBuffer,
    IN PMDL TransferBufferMDL,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PIRP_CONTEXT Context)
{
    PIO_STACK_LOCATION NextStack;

    RtlZeroMemory(&Context->Urb, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));

    Context->Urb.UrbHeader.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    Context->Urb.UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;

    Context->Urb.UrbBulkOrInterruptTransfer.PipeHandle = PipeHandle;
    Context->Urb.UrbBulkOrInterruptTransfer.TransferFlags = TransferFlags;
    Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferLength = TransferBufferLength;
    Context->Urb.UrbBulkOrInterruptTransfer.TransferBuffer = TransferBuffer;
    Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferMDL = TransferBufferMDL;

    NextStack = IoGetNextIrpStackLocation(Irp);
    NextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    NextStack->Parameters.Others.Argument1 = &Context->Urb;

    IoSetCompletionRoutine(Irp,
                           CompletionRoutine,
                           Context,
                           TRUE,
                           TRUE,
                           TRUE);

    return IoCallDriver(FDODeviceExtension->LowerDeviceObject, Irp);
}

static
BOOLEAN
USBSTOR_IsCSWValid(
    PIRP_CONTEXT Context)
{
    if (Context->csw.Signature != CSW_SIGNATURE)
    {
        DPRINT1("[USBSTOR] Expected Signature %x but got %x\n", CSW_SIGNATURE, Context->csw.Signature);
        return FALSE;
    }

    if (Context->csw.Tag != PtrToUlong(&Context->csw))
    {
        DPRINT1("[USBSTOR] Expected Tag %Ix but got %x\n", PtrToUlong(&Context->csw), Context->csw.Tag);
        return FALSE;
    }

    return TRUE;
}

static
NTSTATUS
USBSTOR_IssueRequestSense(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp,
    IN PIRP_CONTEXT Context);

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
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PSCSI_REQUEST_BLOCK Request;

    Context = (PIRP_CONTEXT)Ctx;

    DPRINT("USBSTOR_CSWCompletionRoutine Irp %p Ctx %p Status %x\n", Irp, Ctx, Irp->IoStatus.Status);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;
    FDODeviceExtension = Context->FDODeviceExtension;
    Request = IoStack->Parameters.Scsi.Srb;
    ASSERT(Request);

    // first check for Irp errors
    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        if (USBD_STATUS(Context->Urb.UrbHeader.Status) == USBD_STATUS(USBD_STATUS_STALL_PID))
        {
            if (Context->StallRetryCount < 2)
            {
                ++Context->StallRetryCount;

                // clear stall and resend cbw
                USBSTOR_QueueResetPipe(FDODeviceExtension, Context);

                return STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
        else
        {
            DPRINT1("USBSTOR_CSWCompletionRoutine: Urb.Hdr.Status - %x\n", Context->Urb.UrbHeader.Status);
        }

        goto ResetRecovery;
    }

    // now check the CSW packet validity
    if (!USBSTOR_IsCSWValid(Context) || Context->csw.Status == CSW_STATUS_PHASE_ERROR)
    {
        goto ResetRecovery;
    }

    // finally check for CSW errors
    if (Context->csw.Status == CSW_STATUS_COMMAND_PASSED)
    {
        // should happen only when a sense request was sent
        if (Request != FDODeviceExtension->ActiveSrb)
        {
            ASSERT(IoStack->Parameters.Scsi.Srb == &Context->SenseSrb);
            FDODeviceExtension->ActiveSrb->SenseInfoBufferLength = Request->DataTransferLength;
            Request = FDODeviceExtension->ActiveSrb;
            IoStack->Parameters.Scsi.Srb = Request;
            Request->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        }

        Irp->IoStatus.Status = USBSTOR_SrbStatusToNtStatus(Request);
    }
    else if (Context->csw.Status == CSW_STATUS_COMMAND_FAILED)
    {
        // the command is correct but with failed status - issue request sense
        DPRINT("USBSTOR_CSWCompletionRoutine: CSW_STATUS_COMMAND_FAILED\n");

        ASSERT(FDODeviceExtension->ActiveSrb == Request);

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
            USBSTOR_IssueRequestSense(FDODeviceExtension, Irp, Context);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
    }

    Irp->IoStatus.Information = Request->DataTransferLength;

    // terminate current request
    USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, Irp);
    USBSTOR_QueueNextRequest(PDODeviceExtension->LowerDeviceObject);

    ExFreePoolWithTag(Context, USB_STOR_TAG);
    return STATUS_CONTINUE_COMPLETION;

ResetRecovery:

    Request = FDODeviceExtension->ActiveSrb;
    IoStack->Parameters.Scsi.Srb = Request;
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
    Request->SrbStatus = SRB_STATUS_BUS_RESET;

    USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, Irp);
    USBSTOR_QueueResetDevice(FDODeviceExtension);

    ExFreePoolWithTag(Context, USB_STOR_TAG);
    return STATUS_CONTINUE_COMPLETION;
}

NTSTATUS
USBSTOR_SendCSWRequest(
    PIRP_CONTEXT Context,
    PIRP Irp)
{
    return USBSTOR_IssueBulkOrInterruptRequest(Context->FDODeviceExtension,
                                               Irp,
                                               Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle,
                                               USBD_TRANSFER_DIRECTION_IN,
                                               sizeof(CSW),
                                               &Context->csw,
                                               NULL,
                                               USBSTOR_CSWCompletionRoutine,
                                               Context);
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
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;

    DPRINT("USBSTOR_DataCompletionRoutine Irp %p Ctx %p Status %x\n", Irp, Ctx, Irp->IoStatus.Status);

    Context = (PIRP_CONTEXT)Ctx;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Request = IoStack->Parameters.Scsi.Srb;
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;

    // for Sense Request a partial MDL was already freed (if existed)
    if (Request == Context->FDODeviceExtension->ActiveSrb &&
        Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferMDL != Irp->MdlAddress)
    {
        IoFreeMdl(Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferMDL);
    }

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
        USBSTOR_SendCSWRequest(Context, Irp);
    }
    else if (USBD_STATUS(Context->Urb.UrbHeader.Status) == USBD_STATUS(USBD_STATUS_STALL_PID))
    {
        ++Context->StallRetryCount;

        Request->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        Request->DataTransferLength = Context->Urb.UrbBulkOrInterruptTransfer.TransferBufferLength;

        // clear stall and resend cbw
        USBSTOR_QueueResetPipe(Context->FDODeviceExtension, Context);
    }
    else
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
        Request->SrbStatus = SRB_STATUS_BUS_RESET;

        USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, Irp);
        USBSTOR_QueueResetDevice(Context->FDODeviceExtension);

        ExFreePoolWithTag(Context, USB_STOR_TAG);
        return STATUS_CONTINUE_COMPLETION;
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
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    USBD_PIPE_HANDLE PipeHandle;
    ULONG TransferFlags;
    PMDL Mdl = NULL;
    PVOID TransferBuffer = NULL;

    DPRINT("USBSTOR_CBWCompletionRoutine Irp %p Ctx %p Status %x\n", Irp, Ctx, Irp->IoStatus.Status);

    Context = (PIRP_CONTEXT)Ctx;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Request = IoStack->Parameters.Scsi.Srb;
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        goto ResetRecovery;
    }

    // a request without the buffer AND not a sense request
    // for a sense request we provide just a TransferBuffer, an Mdl will be allocated by usbport (see below)
    if (!Irp->MdlAddress && Request == Context->FDODeviceExtension->ActiveSrb)
    {
        Request->SrbStatus = SRB_STATUS_SUCCESS;
        USBSTOR_SendCSWRequest(Context, Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    // a request with the data buffer

    if ((Request->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_IN)
    {
        PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkInPipeIndex].PipeHandle;
        TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
    }
    else if ((Request->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) == SRB_FLAGS_DATA_OUT)
    {
        PipeHandle = Context->FDODeviceExtension->InterfaceInformation->Pipes[Context->FDODeviceExtension->BulkOutPipeIndex].PipeHandle;
        TransferFlags = USBD_TRANSFER_DIRECTION_OUT;
    }
    else
    {
        // we check the validity of a request in disk.c so we should never be here
        DPRINT1("Warning: shouldn't be here\n");
        goto ResetRecovery;
    }

    // if it is not a Sense Request
    if (Request == Context->FDODeviceExtension->ActiveSrb)
    {
        if (MmGetMdlVirtualAddress(Irp->MdlAddress) == Request->DataBuffer)
        {
            Mdl = Irp->MdlAddress;
        }
        else
        {
            Mdl = IoAllocateMdl(Request->DataBuffer,
                                Request->DataTransferLength,
                                FALSE,
                                FALSE,
                                NULL);

            if (Mdl)
            {
                IoBuildPartialMdl(Irp->MdlAddress,
                                  Mdl,
                                  Request->DataBuffer,
                                  Request->DataTransferLength);
            }
        }

        if (!Mdl)
        {
            DPRINT1("USBSTOR_CBWCompletionRoutine: Mdl - %p\n", Mdl);
            goto ResetRecovery;
        }
    }
    else
    {
        ASSERT(Request->DataBuffer);
        TransferBuffer = Request->DataBuffer;
    }

    USBSTOR_IssueBulkOrInterruptRequest(Context->FDODeviceExtension,
                                        Irp,
                                        PipeHandle,
                                        TransferFlags,
                                        Request->DataTransferLength,
                                        TransferBuffer,
                                        Mdl,
                                        USBSTOR_DataCompletionRoutine,
                                        Context);

    return STATUS_MORE_PROCESSING_REQUIRED;

ResetRecovery:
    Request = Context->FDODeviceExtension->ActiveSrb;
    IoStack->Parameters.Scsi.Srb = Request;
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
    Request->SrbStatus = SRB_STATUS_BUS_RESET;

    USBSTOR_QueueTerminateRequest(PDODeviceExtension->LowerDeviceObject, Irp);
    USBSTOR_QueueResetDevice(Context->FDODeviceExtension);

    ExFreePoolWithTag(Context, USB_STOR_TAG);
    return STATUS_CONTINUE_COMPLETION;
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

static
NTSTATUS
USBSTOR_SendCBWRequest(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp,
    IN PIRP_CONTEXT Context)
{
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;

    RtlZeroMemory(&Context->cbw, sizeof(CBW));
    RtlZeroMemory(&Context->Urb, sizeof(URB));

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    PDODeviceExtension = IoStack->DeviceObject->DeviceExtension;
    Request = IoStack->Parameters.Scsi.Srb;

    Context->cbw.Signature = CBW_SIGNATURE;
    Context->cbw.Tag = PtrToUlong(&Context->cbw);
    Context->cbw.DataTransferLength = Request->DataTransferLength;
    Context->cbw.Flags = ((UCHAR)Request->SrbFlags & SRB_FLAGS_UNSPECIFIED_DIRECTION) << 1;
    Context->cbw.LUN = PDODeviceExtension->LUN;
    Context->cbw.CommandBlockLength = Request->CdbLength;

    RtlCopyMemory(&Context->cbw.CommandBlock, Request->Cdb, Request->CdbLength);

    DPRINT("CBW for IRP %p\n", Irp);
    DumpCBW((PUCHAR)&Context->cbw);

    // initialize rest of context
    Context->Irp = Irp;
    Context->FDODeviceExtension = FDODeviceExtension;
    Context->StallRetryCount = 0;

    return USBSTOR_IssueBulkOrInterruptRequest(
        FDODeviceExtension,
        Irp,
        FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle,
        USBD_TRANSFER_DIRECTION_OUT,
        sizeof(CBW),
        &Context->cbw,
        NULL,
        USBSTOR_CBWCompletionRoutine,
        Context);
}

static
NTSTATUS
USBSTOR_IssueRequestSense(
    IN PFDO_DEVICE_EXTENSION FDODeviceExtension,
    IN PIRP Irp,
    IN PIRP_CONTEXT Context)
{
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK CurrentSrb;
    PSCSI_REQUEST_BLOCK SenseSrb;

    DPRINT("USBSTOR_IssueRequestSense: \n");

    CurrentSrb = FDODeviceExtension->ActiveSrb;
    SenseSrb = &Context->SenseSrb;
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    IoStack->Parameters.Scsi.Srb = SenseSrb;

    RtlZeroMemory(SenseSrb, sizeof(*SenseSrb));

    SenseSrb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    SenseSrb->Length = sizeof(*SenseSrb);
    SenseSrb->CdbLength = CDB6GENERIC_LENGTH;
    SenseSrb->SrbFlags = SRB_FLAGS_DATA_IN |
                         SRB_FLAGS_NO_QUEUE_FREEZE |
                         SRB_FLAGS_DISABLE_AUTOSENSE;

    ASSERT(CurrentSrb->SenseInfoBufferLength);
    ASSERT(CurrentSrb->SenseInfoBuffer);
    DPRINT("SenseInfoBuffer %x, SenseInfoBufferLength %x\n", CurrentSrb->SenseInfoBuffer, CurrentSrb->SenseInfoBufferLength);

    SenseSrb->DataTransferLength = CurrentSrb->SenseInfoBufferLength;
    SenseSrb->DataBuffer = CurrentSrb->SenseInfoBuffer;

    SrbGetCdb(SenseSrb)->CDB6GENERIC.OperationCode = SCSIOP_REQUEST_SENSE;
    SrbGetCdb(SenseSrb)->AsByte[4] = CurrentSrb->SenseInfoBufferLength;

    return USBSTOR_SendCBWRequest(FDODeviceExtension, Irp, Context);
}

NTSTATUS
USBSTOR_HandleExecuteSCSI(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PSCSI_REQUEST_BLOCK Request;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PIRP_CONTEXT Context;

    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    Request = IoStack->Parameters.Scsi.Srb;

    DPRINT("USBSTOR_HandleExecuteSCSI Operation Code %x, Length %lu\n", SrbGetCdb(Request)->CDB10.OperationCode, Request->DataTransferLength);

    // check that we're sending to the right LUN
    ASSERT(SrbGetCdb(Request)->CDB10.LogicalUnitNumber == (PDODeviceExtension->LUN & MAX_LUN));
    Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(IRP_CONTEXT), USB_STOR_TAG);

    if (!Context)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Status = USBSTOR_SendCBWRequest(PDODeviceExtension->LowerDeviceObject->DeviceExtension, Irp, Context);
    }

    return Status;
}
