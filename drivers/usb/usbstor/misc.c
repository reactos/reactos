/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     USB block storage device driver.
 * COPYRIGHT:   2005-2006 James Tabor
 *              2011-2012 Michael Martin (michael.martin@reactos.org)
 *              2011-2013 Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>


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
USBSTOR_GetBusInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSB_BUS_INTERFACE_USBDI_V2 BusInterface)
{
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;

    ASSERT(DeviceObject);
    ASSERT(BusInterface);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);
    if (Irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // initialize request
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack->Parameters.QueryInterface.InterfaceType = (LPGUID)&USB_BUS_INTERFACE_USBDI_GUID;
    Stack->Parameters.QueryInterface.Version = 2;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
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

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoStack = IoGetNextIrpStackLocation(Irp);

    // initialize stack location
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    IoStack->Parameters.Others.Argument1 = (PVOID)UrbRequest;
    IoStack->Parameters.DeviceIoControl.InputBufferLength = UrbRequest->UrbHeader.Length;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSetCompletionRoutine(Irp, USBSTOR_SyncForwardIrpCompletionRoutine, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    IoFreeIrp(Irp);
    return Status;
}

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN ULONG ItemSize)
{
    PVOID Item = ExAllocatePoolWithTag(PoolType, ItemSize, USB_STOR_TAG);

    if (Item)
    {
        RtlZeroMemory(Item, ItemSize);
    }

    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{
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

    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Urb->UrbControlVendorClassRequest.Hdr.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
    Urb->UrbControlVendorClassRequest.Hdr.Function = URB_FUNCTION_CLASS_INTERFACE;
    Urb->UrbControlVendorClassRequest.TransferFlags = TransferFlags;
    Urb->UrbControlVendorClassRequest.TransferBufferLength = TransferBufferLength;
    Urb->UrbControlVendorClassRequest.TransferBuffer = TransferBuffer;
    Urb->UrbControlVendorClassRequest.Request = RequestType;
    Urb->UrbControlVendorClassRequest.Index = Index;

    Status = USBSTOR_SyncUrbRequest(DeviceObject, Urb);

    FreeItem(Urb);
    return Status;
}

NTSTATUS
USBSTOR_GetMaxLUN(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension)
{
    PUCHAR Buffer;
    NTSTATUS Status;

    Buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, sizeof(UCHAR), USB_STOR_TAG);
    if (!Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = USBSTOR_ClassRequest(DeviceObject, DeviceExtension, USB_BULK_GET_MAX_LUN, DeviceExtension->InterfaceInformation->InterfaceNumber, USBD_TRANSFER_DIRECTION_IN, sizeof(UCHAR), Buffer);

    DPRINT("MaxLUN: %x\n", *Buffer);

    if (NT_SUCCESS(Status))
    {
        if (*Buffer > MAX_LUN)
        {
            // invalid response documented in usb mass storage specification
            Status = STATUS_DEVICE_DATA_ERROR;
        }
        else
        {
            // store maxlun
            DeviceExtension->MaxLUN = *Buffer;
        }
    }
    else
    {
        // "USB Mass Storage Class. Bulk-Only Transport. Revision 1.0"
        // 3.2  Get Max LUN (class-specific request) :
        // Devices that do not support multiple LUNs may STALL this command.
        USBSTOR_ResetDevice(DeviceExtension->LowerDeviceObject, DeviceExtension);

        DeviceExtension->MaxLUN = 0;
        Status = STATUS_SUCCESS;
    }

    ExFreePoolWithTag(Buffer, USB_STOR_TAG);
    return Status;
}

NTSTATUS
USBSTOR_ResetDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;

    Status = USBSTOR_ClassRequest(DeviceObject, DeviceExtension, USB_BULK_RESET_DEVICE, DeviceExtension->InterfaceInformation->InterfaceNumber, USBD_TRANSFER_DIRECTION_OUT, 0, NULL);
    return Status;
}

// if somebody wants to add UFI support, here is a useful function
#if 0
BOOLEAN
USBSTOR_IsFloppy(
    IN PUCHAR Buffer,
    IN ULONG BufferLength,
    OUT PUCHAR MediumTypeCode)
{
    PUFI_CAPACITY_FORMAT_HEADER FormatHeader;
    PUFI_CAPACITY_DESCRIPTOR Descriptor;
    ULONG Length, Index, BlockCount, BlockLength;

    FormatHeader = (PUFI_CAPACITY_FORMAT_HEADER)Buffer;
    ASSERT(FormatHeader->Reserved1 == 0x00);
    ASSERT(FormatHeader->Reserved2 == 0x00);
    ASSERT(FormatHeader->Reserved3 == 0x00);

    // is there capacity data
    if (!FormatHeader->CapacityLength)
    {
        DPRINT1("[USBSTOR] No capacity length\n");
        return FALSE;
    }

    // the format header are always 8 bytes in length
    ASSERT((FormatHeader->CapacityLength & 0x7) == 0);
    DPRINT1("CapacityLength %x\n", FormatHeader->CapacityLength);

    // grab length and locate first descriptor
    Length = FormatHeader->CapacityLength;
    Descriptor = (PUFI_CAPACITY_DESCRIPTOR)(FormatHeader + 1);
    for (Index = 0; Index < Length / sizeof(UFI_CAPACITY_DESCRIPTOR); Index++)
    {
        // blocks are little endian format
        BlockCount = NTOHL(Descriptor->BlockCount);
        BlockLength = NTOHL((Descriptor->BlockLengthByte0 << 24 | Descriptor->BlockLengthByte1 << 16 | Descriptor->BlockLengthByte2 << 8));

        DPRINT1("BlockCount %x BlockLength %x Code %x\n", BlockCount, BlockLength, Descriptor->Code);

        if (BlockLength == 512 && BlockCount == 1440)
        {
            // 720 KB DD
            *MediumTypeCode = 0x1E;
            return TRUE;
        }
        else if (BlockLength == 1024 && BlockCount == 1232)
        {
            // 1,25 MB
            *MediumTypeCode = 0x93;
            return TRUE;
        }
        else if (BlockLength == 512 && BlockCount == 2880)
        {
            // 1,44MB KB DD
            *MediumTypeCode = 0x94;
            return TRUE;
        }

        Descriptor++;
    }

    return FALSE;
}
#endif
