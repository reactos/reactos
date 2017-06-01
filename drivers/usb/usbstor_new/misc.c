/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/misc.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>

//
// driver verifier
//
IO_COMPLETION_ROUTINE SyncForwardIrpCompletionRoutine;

NTSTATUS
NTAPI
USBSTOR_SyncForwardIrpCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    if (Irp->PendingReturned)
    {
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
USBSTOR_SyncForwardIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    //
    // initialize event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // copy irp stack location
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // set completion routine
    //
    IoSetCompletionRoutine(Irp, USBSTOR_SyncForwardIrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);


    //
    // call driver
    //
    Status = IoCallDriver(DeviceObject, Irp);

    //
    // check if pending
    //
    if (Status == STATUS_PENDING)
    {
        //
        // wait for the request to finish
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        //
        // copy status code
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // done
    //
    return Status;
}

NTSTATUS
NTAPI
USBSTOR_GetBusInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSB_BUS_INTERFACE_USBDI_V2 BusInterface)
{
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;

    //
    // sanity checks
    //
    ASSERT(DeviceObject);
    ASSERT(BusInterface);


    //
    // initialize event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);


    //
    // create irp
    //
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    //
    // was irp built
    //
    if (Irp == NULL)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize request
    //
    Stack=IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack->Parameters.QueryInterface.InterfaceType = (LPGUID)&USB_BUS_INTERFACE_USBDI_GUID;
    Stack->Parameters.QueryInterface.Version = 2;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // call driver
    //
    Status= IoCallDriver(DeviceObject, Irp);

    //
    // did operation complete
    //
    if (Status == STATUS_PENDING)
    {
        //
        // wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        //
        // collect status
        //
        Status=IoStatus.Status;
    }

    return Status;
}

NTSTATUS
USBSTOR_SyncUrbRequest(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PURB UrbRequest)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    KEVENT Event;
    NTSTATUS Status;

    //
    // allocate irp
    //
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);


    //
    // get next stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // initialize stack location
    //
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)UrbRequest;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = UrbRequest->UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // setup completion routine
    //
    IoSetCompletionRoutine(Irp, USBSTOR_SyncForwardIrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);

    //
    // call driver
    //
    Status = IoCallDriver(DeviceObject, Irp);

    //
    // check if request is pending
    //
    if (Status == STATUS_PENDING)
    {
        //
        // wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        //
        // update status
        //
        Status = Irp->IoStatus.Status;
    }

    //
    // free irp
    //
    IoFreeIrp(Irp);

    //
    // done
    //
    return Status;
}

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN ULONG ItemSize)
{
    //
    // allocate item
    //
    PVOID Item = ExAllocatePoolWithTag(PoolType, ItemSize, USB_STOR_TAG);

    if (Item)
    {
        //
        // zero item
        //
        RtlZeroMemory(Item, ItemSize);
    }

    //
    // return element
    //
    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{
    //
    // free item
    //
    ExFreePoolWithTag(Item, USB_STOR_TAG);
}

NTSTATUS
USBSTOR_ClassRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension,
    IN UCHAR RequestType,
    IN USHORT Index,
    IN ULONG TransferFlags,
    IN ULONG TransferBufferLength,
    IN PVOID TransferBuffer)

{
    PURB Urb;
    NTSTATUS Status;

    //
    // first allocate urb
    //
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize vendor request
    //
    Urb->UrbControlVendorClassRequest.Hdr.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    Urb->UrbControlVendorClassRequest.Hdr.Function = URB_FUNCTION_CLASS_INTERFACE;
    Urb->UrbControlVendorClassRequest.TransferFlags = TransferFlags;
    Urb->UrbControlVendorClassRequest.TransferBufferLength = TransferBufferLength;
    Urb->UrbControlVendorClassRequest.TransferBuffer = TransferBuffer;
    Urb->UrbControlVendorClassRequest.Request = RequestType;
    Urb->UrbControlVendorClassRequest.Index = Index;

    //
    // submit request
    //
    Status = USBSTOR_SyncUrbRequest(DeviceObject, Urb);

    //
    // free urb
    //
    FreeItem(Urb);

    //
    // done
    //
    return Status;
}


NTSTATUS
USBSTOR_GetMaxLUN(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension)
{
    PUCHAR Buffer;
    NTSTATUS Status;

    //
    // allocate 1-byte buffer
    //
    Buffer = (PUCHAR)AllocateItem(NonPagedPool, sizeof(UCHAR));
    if (!Buffer)
    {
        //
        // no memory
        //
        FreeItem(Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // execute request
    //
    Status = USBSTOR_ClassRequest(DeviceObject, DeviceExtension, USB_BULK_GET_MAX_LUN, DeviceExtension->InterfaceInformation->InterfaceNumber, USBD_TRANSFER_DIRECTION_IN, sizeof(UCHAR), Buffer);

    DPRINT("MaxLUN: %x\n", *Buffer);

    if (NT_SUCCESS(Status))
    {
        if (*Buffer > 0xF)
        {
            //
            // invalid response documented in usb mass storage specification
            //
            Status = STATUS_DEVICE_DATA_ERROR;
        }
        else
        {
            //
            // store maxlun
            //
            DeviceExtension->MaxLUN = *Buffer;
        }
    }
    else
    {
        //
        // "USB Mass Storage Class. Bulk-Only Transport. Revision 1.0"
        // 3.2  Get Max LUN (class-specific request) :
        // Devices that do not support multiple LUNs may STALL this command.
        //
        USBSTOR_BulkResetDevice(DeviceExtension->LowerDeviceObject, DeviceExtension);

        DeviceExtension->MaxLUN = 0;
        Status = STATUS_SUCCESS;
    }

    //
    // free buffer
    //
    FreeItem(Buffer);

    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_BulkResetDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;

    //
    // execute request
    //
    Status = USBSTOR_ClassRequest(DeviceObject, DeviceExtension, USB_BULK_RESET_DEVICE, DeviceExtension->InterfaceInformation->InterfaceNumber, USBD_TRANSFER_DIRECTION_OUT, 0, NULL);

    //
    // done
    //
    return Status;

}

BOOLEAN
USBSTOR_IsFloppy(
    IN PUCHAR Buffer,
    IN ULONG BufferLength,
    OUT PUCHAR MediumTypeCode)
{
    PUFI_CAPACITY_FORMAT_HEADER FormatHeader;
    PUFI_CAPACITY_DESCRIPTOR Descriptor;
    ULONG Length, Index, BlockCount, BlockLength;

    //
    // get format header
    //
    FormatHeader = (PUFI_CAPACITY_FORMAT_HEADER)Buffer;

    //
    // sanity checks
    //
    ASSERT(FormatHeader->Reserved1 == 0x00);
    ASSERT(FormatHeader->Reserved2 == 0x00);
    ASSERT(FormatHeader->Reserved3 == 0x00);

    //
    // is there capacity data
    //
    if (!FormatHeader->CapacityLength)
    {
        //
        // no data provided
        //
        DPRINT1("[USBSTOR] No capacity length\n");
        return FALSE;
    }

    //
    // the format header are always 8 bytes in length
    //
    ASSERT((FormatHeader->CapacityLength & 0x7) == 0);
    DPRINT1("CapacityLength %x\n", FormatHeader->CapacityLength);

    //
    // grab length and locate first descriptor
    //
    Length = FormatHeader->CapacityLength;
    Descriptor = (PUFI_CAPACITY_DESCRIPTOR)(FormatHeader + 1);
    for(Index = 0; Index < Length / sizeof(UFI_CAPACITY_DESCRIPTOR); Index++)
    {
        //
        // blocks are little endian format
        //
        BlockCount = NTOHL(Descriptor->BlockCount);

        //
        // get block length
        //
        BlockLength = NTOHL((Descriptor->BlockLengthByte0 << 24 | Descriptor->BlockLengthByte1 << 16 | Descriptor->BlockLengthByte2 << 8));

        DPRINT1("BlockCount %x BlockLength %x Code %x\n", BlockCount, BlockLength, Descriptor->Code);

        if (BlockLength == 512 && BlockCount == 1440)
        {
            //
            // 720 KB DD
            //
            *MediumTypeCode = 0x1E;
            return TRUE;
        }
        else if (BlockLength == 1024 && BlockCount == 1232)
        {
            //
            // 1,25 MB
            //
            *MediumTypeCode = 0x93;
            return TRUE;
        }
        else if (BlockLength == 512 && BlockCount == 2880)
        {
            //
            // 1,44MB KB DD
            //
            *MediumTypeCode = 0x94;
            return TRUE;
        }

        //
        // move to next descriptor
        //
        Descriptor = (Descriptor + 1);
    }

    //
    // no floppy detected
    //
    return FALSE;
}
