// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#include "fbtusb.h"
#include "fbtpnp.h"
#include "fbtpwr.h"
#include "fbtdev.h"
#include "fbtrwr.h"
#include "fbtwmi.h"

#include "fbtusr.h"

// Read/Write handler
NTSTATUS NTAPI FreeBT_DispatchRead(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PMDL                    mdl;
    PURB                    urb;
    ULONG                   totalLength;
    ULONG                   stageLength;
    NTSTATUS                ntStatus;
    ULONG_PTR               virtualAddress;
    PFILE_OBJECT            fileObject;
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PIO_STACK_LOCATION      nextStack;
    PFREEBT_RW_CONTEXT      rwContext;
    //ULONG                   maxLength=0;

    urb = NULL;
    mdl = NULL;
    rwContext = NULL;
    totalLength = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchRead: Entered\n"));

    if (deviceExtension->DeviceState != Working)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchRead: Invalid device state\n"));
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto FreeBT_DispatchRead_Exit;

    }

    // Make sure that any selective suspend request has been completed.
    if (deviceExtension->SSEnable)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchRead: Waiting on the IdleReqPendEvent\n"));
        KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

    }

    rwContext = (PFREEBT_RW_CONTEXT) ExAllocatePool(NonPagedPool, sizeof(FREEBT_RW_CONTEXT));
    if (rwContext == NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchRead: Failed to alloc mem for rwContext\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto FreeBT_DispatchRead_Exit;

    }

    if (Irp->MdlAddress)
    {
        totalLength = MmGetMdlByteCount(Irp->MdlAddress);

    }

    FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchRead: Transfer data length = %d\n", totalLength));
    if (totalLength == 0)
    {
        ntStatus = STATUS_SUCCESS;
        ExFreePool(rwContext);
        goto FreeBT_DispatchRead_Exit;

    }

    virtualAddress = (ULONG_PTR) MmGetMdlVirtualAddress(Irp->MdlAddress);
    if (totalLength > deviceExtension->DataInPipe.MaximumPacketSize)
    {
        stageLength = deviceExtension->DataInPipe.MaximumPacketSize;

    }

    else
    {
        stageLength = totalLength;

    }

    mdl = IoAllocateMdl((PVOID) virtualAddress, totalLength, FALSE, FALSE, NULL);
    if (mdl == NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchRead: Failed to alloc mem for mdl\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        goto FreeBT_DispatchRead_Exit;

    }

    // map the portion of user-buffer described by an mdl to another mdl
    IoBuildPartialMdl(Irp->MdlAddress, mdl, (PVOID) virtualAddress, stageLength);
    urb = (PURB) ExAllocatePool(NonPagedPool, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    if (urb == NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchRead: Failed to alloc mem for urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        IoFreeMdl(mdl);
        goto FreeBT_DispatchRead_Exit;

    }

    UsbBuildInterruptOrBulkTransferRequest(
                            urb,
                            sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                            deviceExtension->DataInPipe.PipeHandle,
                            NULL,
                            mdl,
                            stageLength,
                            USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_IN,
                            NULL);

    // set FREEBT_RW_CONTEXT parameters.
    rwContext->Urb             = urb;
    rwContext->Mdl             = mdl;
    rwContext->Length          = totalLength - stageLength;
    rwContext->Numxfer         = 0;
    rwContext->VirtualAddress  = virtualAddress + stageLength;

    // use the original read/write irp as an internal device control irp
    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = (PVOID) urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)FreeBT_ReadCompletion,
                           rwContext,
                           TRUE,
                           TRUE,
                           TRUE);

    // We return STATUS_PENDING; call IoMarkIrpPending.
    IoMarkIrpPending(Irp);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    if (!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchRead: IoCallDriver fails with status %X\n", ntStatus));

        // if the device was yanked out, then the pipeInformation
        // field is invalid.
        // similarly if the request was cancelled, then we need not
        // invoked reset pipe/device.
        if((ntStatus != STATUS_CANCELLED) && (ntStatus != STATUS_DEVICE_NOT_CONNECTED))
        {
            ntStatus = FreeBT_ResetPipe(DeviceObject, deviceExtension->DataInPipe.PipeHandle);
            if(!NT_SUCCESS(ntStatus))
            {
                FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_DispatchRead: FreeBT_ResetPipe failed\n"));
                ntStatus = FreeBT_ResetDevice(DeviceObject);

            }

        }

        else
        {
            FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchRead: ntStatus is STATUS_CANCELLED or STATUS_DEVICE_NOT_CONNECTED\n"));

        }

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchRead::"));
    FreeBT_IoIncrement(deviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchRead: URB sent to lower driver, IRP is pending\n"));

    // we return STATUS_PENDING and not the status returned by the lower layer.
    return STATUS_PENDING;

FreeBT_DispatchRead_Exit:
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchRead: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI FreeBT_ReadCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    //ULONG               stageLength;
    NTSTATUS            ntStatus;
    //PIO_STACK_LOCATION  nextStack;
    PFREEBT_RW_CONTEXT  rwContext;
    PDEVICE_EXTENSION   deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    rwContext = (PFREEBT_RW_CONTEXT) Context;
    ntStatus = Irp->IoStatus.Status;

    UNREFERENCED_PARAMETER(DeviceObject);
    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_ReadCompletion: Entered\n"));

    if (NT_SUCCESS(ntStatus))
    {
        Irp->IoStatus.Information = rwContext->Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

    }

    else
    {
        Irp->IoStatus.Information = 0;
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_ReadCompletion: - failed with status = %X\n", ntStatus));

    }

    if (rwContext)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_ReadCompletion: ::"));
        FreeBT_IoDecrement(deviceExtension);

        ExFreePool(rwContext->Urb);
        IoFreeMdl(rwContext->Mdl);
        ExFreePool(rwContext);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_ReadCompletion: Leaving\n"));

    return ntStatus;

}

// Read/Write handler
NTSTATUS NTAPI FreeBT_DispatchWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PMDL                    mdl;
    PURB                    urb;
    ULONG                   totalLength;
    ULONG                   stageLength;
    NTSTATUS                ntStatus;
    ULONG_PTR               virtualAddress;
    PFILE_OBJECT            fileObject;
    PDEVICE_EXTENSION       deviceExtension;
    PIO_STACK_LOCATION      irpStack;
    PIO_STACK_LOCATION      nextStack;
    PFREEBT_RW_CONTEXT      rwContext;
    //ULONG                   maxLength=0;

    urb = NULL;
    mdl = NULL;
    rwContext = NULL;
    totalLength = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpStack->FileObject;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchWrite: Entered\n"));

    if (deviceExtension->DeviceState != Working)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_WriteDispatch: Invalid device state\n"));
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        goto FreeBT_DispatchWrite_Exit;

    }

    // Make sure that any selective suspend request has been completed.
    if (deviceExtension->SSEnable)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_WriteDispatch: Waiting on the IdleReqPendEvent\n"));
        KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

    }

    rwContext = (PFREEBT_RW_CONTEXT) ExAllocatePool(NonPagedPool, sizeof(FREEBT_RW_CONTEXT));
    if (rwContext == NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: Failed to alloc mem for rwContext\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto FreeBT_DispatchWrite_Exit;

    }

    if (Irp->MdlAddress)
    {
        totalLength = MmGetMdlByteCount(Irp->MdlAddress);

    }

    FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_WriteDispatch: Transfer data length = %d\n", totalLength));
    if (totalLength>FBT_HCI_DATA_MAX_SIZE)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_WriteDispatch: Buffer exceeds maximum packet length (%d), failing IRP\n", FBT_HCI_DATA_MAX_SIZE));
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        ExFreePool(rwContext);
        goto FreeBT_DispatchWrite_Exit;

    }

    if (totalLength<FBT_HCI_DATA_MIN_SIZE)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_WriteDispatch: Zero length buffer, completing IRP\n"));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        ExFreePool(rwContext);
        goto FreeBT_DispatchWrite_Exit;

    }

    virtualAddress = (ULONG_PTR) MmGetMdlVirtualAddress(Irp->MdlAddress);
    if (totalLength > deviceExtension->DataOutPipe.MaximumPacketSize)
    {
        stageLength = deviceExtension->DataOutPipe.MaximumPacketSize;

    }

    else
    {
        stageLength = totalLength;

    }

    mdl = IoAllocateMdl((PVOID) virtualAddress, totalLength, FALSE, FALSE, NULL);
    if (mdl == NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_WriteDispatch: Failed to alloc mem for mdl\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        goto FreeBT_DispatchWrite_Exit;

    }

    // map the portion of user-buffer described by an mdl to another mdl
    IoBuildPartialMdl(Irp->MdlAddress, mdl, (PVOID) virtualAddress, stageLength);
    urb = (PURB) ExAllocatePool(NonPagedPool, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    if (urb == NULL)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_WriteDispatch: Failed to alloc mem for urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        ExFreePool(rwContext);
        IoFreeMdl(mdl);
        goto FreeBT_DispatchWrite_Exit;

    }

    UsbBuildInterruptOrBulkTransferRequest(
                            urb,
                            sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                            deviceExtension->DataOutPipe.PipeHandle,
                            NULL,
                            mdl,
                            stageLength,
                            USBD_SHORT_TRANSFER_OK | USBD_TRANSFER_DIRECTION_OUT,
                            NULL);

    // set FREEBT_RW_CONTEXT parameters.
    rwContext->Urb             = urb;
    rwContext->Mdl             = mdl;
    rwContext->Length          = totalLength - stageLength;
    rwContext->Numxfer         = 0;
    rwContext->VirtualAddress  = virtualAddress + stageLength;

    // use the original read/write irp as an internal device control irp
    nextStack = IoGetNextIrpStackLocation(Irp);
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.Others.Argument1 = (PVOID) urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)FreeBT_WriteCompletion,
                           rwContext,
                           TRUE,
                           TRUE,
                           TRUE);

    // We return STATUS_PENDING; call IoMarkIrpPending.
    IoMarkIrpPending(Irp);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    if (!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_WriteDispatch: IoCallDriver fails with status %X\n", ntStatus));

        // if the device was yanked out, then the pipeInformation
        // field is invalid.
        // similarly if the request was cancelled, then we need not
        // invoked reset pipe/device.
        if((ntStatus != STATUS_CANCELLED) && (ntStatus != STATUS_DEVICE_NOT_CONNECTED))
        {
            ntStatus = FreeBT_ResetPipe(DeviceObject, deviceExtension->DataOutPipe.PipeHandle);
            if(!NT_SUCCESS(ntStatus))
            {
                FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_ResetPipe failed\n"));
                ntStatus = FreeBT_ResetDevice(DeviceObject);

            }

        }

        else
        {
            FreeBT_DbgPrint(3, ("FBTUSB: ntStatus is STATUS_CANCELLED or STATUS_DEVICE_NOT_CONNECTED\n"));

        }

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchWrite::"));
    FreeBT_IoIncrement(deviceExtension);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchWrite: URB sent to lower driver, IRP is pending\n"));

    // we return STATUS_PENDING and not the status returned by the lower layer.
    return STATUS_PENDING;

FreeBT_DispatchWrite_Exit:
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchWrite: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI FreeBT_WriteCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    ULONG               stageLength;
    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  nextStack;
    PFREEBT_RW_CONTEXT  rwContext;
    PDEVICE_EXTENSION   deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    rwContext = (PFREEBT_RW_CONTEXT) Context;
    ntStatus = Irp->IoStatus.Status;

    UNREFERENCED_PARAMETER(DeviceObject);
    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_WriteCompletion: Entered\n"));

    if (NT_SUCCESS(ntStatus))
    {
        if (rwContext)
        {
            rwContext->Numxfer += rwContext->Urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
            if (rwContext->Length)
            {
                // More data to transfer
                FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_WriteCompletion: Initiating next transfer\n"));
                if (rwContext->Length > deviceExtension->DataOutPipe.MaximumPacketSize)
                {
                    stageLength = deviceExtension->DataOutPipe.MaximumPacketSize;

                }

                else
                {
                    stageLength = rwContext->Length;

                }

                IoBuildPartialMdl(Irp->MdlAddress, rwContext->Mdl, (PVOID) rwContext->VirtualAddress, stageLength);

                // reinitialize the urb
                rwContext->Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = stageLength;
                rwContext->VirtualAddress += stageLength;
                rwContext->Length -= stageLength;

                nextStack = IoGetNextIrpStackLocation(Irp);
                nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                nextStack->Parameters.Others.Argument1 = rwContext->Urb;
                nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

                IoSetCompletionRoutine(Irp,
                                       FreeBT_ReadCompletion,
                                       rwContext,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            else
            {
                // No more data to transfer
                FreeBT_DbgPrint(1, ("FBTUSB: FreeNT_WriteCompletion: Write completed, %d bytes written\n", Irp->IoStatus.Information));
                Irp->IoStatus.Information = rwContext->Numxfer;

            }

        }

    }

    else
    {
        FreeBT_DbgPrint(1, ("FBTUSB: FreeNT_WriteCompletion - failed with status = %X\n", ntStatus));

    }

    if (rwContext)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_WriteCompletion: ::"));
        FreeBT_IoDecrement(deviceExtension);

        ExFreePool(rwContext->Urb);
        IoFreeMdl(rwContext->Mdl);
        ExFreePool(rwContext);

    }


    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_WriteCompletion: Leaving\n"));

    return ntStatus;

}

