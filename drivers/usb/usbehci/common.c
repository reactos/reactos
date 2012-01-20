/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/common.c
 * PURPOSE:     Common operations.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#define INITGUID
#include "usbehci.h"
#include <wdmguid.h>
#include <stdio.h>

VOID
DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    DPRINT1("Dumping Device Descriptor %x\n", DeviceDescriptor);
    DPRINT1("bLength %x\n", DeviceDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT1("bcdUSB %x\n", DeviceDescriptor->bcdUSB);
    DPRINT1("bDeviceClass %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT1("bDeviceSubClass %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT1("bDeviceProtocol %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT1("bMaxPacketSize0 %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT1("idVendor %x\n", DeviceDescriptor->idVendor);
    DPRINT1("idProduct %x\n", DeviceDescriptor->idProduct);
    DPRINT1("bcdDevice %x\n", DeviceDescriptor->bcdDevice);
    DPRINT1("iManufacturer %x\n", DeviceDescriptor->iManufacturer);
    DPRINT1("iProduct %x\n", DeviceDescriptor->iProduct);
    DPRINT1("iSerialNumber %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT1("bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
}

VOID
DumpFullConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
    LONG i, j;

    DPRINT1("Dumping ConfigurationDescriptor %x\n", ConfigurationDescriptor);
    DPRINT1("bLength %x\n", ConfigurationDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DPRINT1("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DPRINT1("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DPRINT1("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DPRINT1("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DPRINT1("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DPRINT1("MaxPower %x\n", ConfigurationDescriptor->MaxPower);

    InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) ((ULONG_PTR)ConfigurationDescriptor + sizeof(USB_CONFIGURATION_DESCRIPTOR));

    for (i=0; i < ConfigurationDescriptor->bNumInterfaces; i++)
    {
        DPRINT1("- Dumping InterfaceDescriptor %x\n", InterfaceDescriptor);
        DPRINT1("  bLength %x\n", InterfaceDescriptor->bLength);
        DPRINT1("  bDescriptorType %x\n", InterfaceDescriptor->bDescriptorType);
        DPRINT1("  bInterfaceNumber %x\n", InterfaceDescriptor->bInterfaceNumber);
        DPRINT1("  bAlternateSetting %x\n", InterfaceDescriptor->bAlternateSetting);
        DPRINT1("  bNumEndpoints %x\n", InterfaceDescriptor->bNumEndpoints);
        DPRINT1("  bInterfaceClass %x\n", InterfaceDescriptor->bInterfaceClass);
        DPRINT1("  bInterfaceSubClass %x\n", InterfaceDescriptor->bInterfaceSubClass);
        DPRINT1("  bInterfaceProtocol %x\n", InterfaceDescriptor->bInterfaceProtocol);
        DPRINT1("  iInterface %x\n", InterfaceDescriptor->iInterface);

        EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)InterfaceDescriptor + sizeof(USB_INTERFACE_DESCRIPTOR));

        for (j=0; j < InterfaceDescriptor->bNumEndpoints; j++)
        {
            DPRINT1("   bLength %x\n", EndpointDescriptor->bLength);
            DPRINT1("   bDescriptorType %x\n", EndpointDescriptor->bDescriptorType);
            DPRINT1("   bEndpointAddress %x\n", EndpointDescriptor->bEndpointAddress);
            DPRINT1("   bmAttributes %x\n", EndpointDescriptor->bmAttributes);
            DPRINT1("   wMaxPacketSize %x\n", EndpointDescriptor->wMaxPacketSize);
            DPRINT1("   bInterval %x\n", EndpointDescriptor->bInterval);
            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR) ((ULONG_PTR)EndpointDescriptor + sizeof(USB_ENDPOINT_DESCRIPTOR));
        }
        InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)(ULONG_PTR)EndpointDescriptor;
    }

}

VOID
DumpQueueHead(PQUEUE_HEAD QueueHead)
{
    DPRINT1("Dumping QueueHead %x\n", QueueHead);
    DPRINT1("    CurrentLinkPointer %x\n", QueueHead->CurrentLinkPointer);
    DPRINT1("    NextPointer %x\n", QueueHead->NextPointer);
    DPRINT1("    AlternateNextPointer %x\n", QueueHead->AlternateNextPointer);
    DPRINT1("    HorizontalLinkPointer %x\n", QueueHead->HorizontalLinkPointer);
    DPRINT1("    Active %x\n", QueueHead->Token.Bits.Active);
    DPRINT1("    Halted %x\n", QueueHead->Token.Bits.Halted);
    DPRINT1("    DataBufferError %x\n", QueueHead->Token.Bits.DataBufferError);
    DPRINT1("    BabbleDetected %x\n", QueueHead->Token.Bits.BabbleDetected);
    DPRINT1("    TransactionError %x\n", QueueHead->Token.Bits.TransactionError);
    DPRINT1("    MissedMicroFrame %x\n", QueueHead->Token.Bits.MissedMicroFrame);
    DPRINT1("    PingState %x\n", QueueHead->Token.Bits.PingState);        
    DPRINT1("    SplitTransactionState %x\n", QueueHead->Token.Bits.SplitTransactionState);
    DPRINT1("    ErrorCounter %x\n", QueueHead->Token.Bits.ErrorCounter);
    DPRINT1("    First TransferDescriptor %x\n", QueueHead->FirstTransferDescriptor);
}

VOID
DumpTransferDescriptor(PQUEUE_TRANSFER_DESCRIPTOR TransferDescriptor)
{
    DPRINT1("Dumping Descriptor %x\n", TransferDescriptor);
    DPRINT1("    Active %x\n", TransferDescriptor->Token.Bits.Active);
    DPRINT1("    Halted %x\n", TransferDescriptor->Token.Bits.Halted);
    DPRINT1("    DataBufferError %x\n", TransferDescriptor->Token.Bits.DataBufferError);
    DPRINT1("    BabbleDetected %x\n", TransferDescriptor->Token.Bits.BabbleDetected);
    DPRINT1("    TransactionError %x\n", TransferDescriptor->Token.Bits.TransactionError);
    DPRINT1("    MissedMicroFrame %x\n", TransferDescriptor->Token.Bits.MissedMicroFrame);
    DPRINT1("    PingState %x\n", TransferDescriptor->Token.Bits.PingState);        
    DPRINT1("    SplitTransactionState %x\n", TransferDescriptor->Token.Bits.SplitTransactionState);
    DPRINT1("    ErrorCounter %x\n", TransferDescriptor->Token.Bits.ErrorCounter);
}

NTSTATUS NTAPI
GetBusInterface(PDEVICE_OBJECT DeviceObject, PBUS_INTERFACE_STANDARD busInterface)
{
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    PIO_STACK_LOCATION Stack;

    if ((!DeviceObject) || (!busInterface))
        return STATUS_UNSUCCESSFUL;

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

    Stack=IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack->Parameters.QueryInterface.InterfaceType = (LPGUID)&GUID_BUS_INTERFACE_STANDARD;
    Stack->Parameters.QueryInterface.Version = 1;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)busInterface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    Irp->IoStatus.Status=STATUS_NOT_SUPPORTED ;

    Status=IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        Status=IoStatus.Status;
    }

    return Status;
}

NTSTATUS NTAPI
ForwardAndWaitCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PKEVENT Event)
{
    if (Irp->PendingReturned)
    {
        KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS NTAPI
ForwardAndWait(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtensions;
    KEVENT Event;
    NTSTATUS Status;


    DeviceExtensions = DeviceObject->DeviceExtension; 
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE)ForwardAndWaitCompletionRoutine, &Event, TRUE, TRUE, TRUE);
    Status = IoCallDriver(DeviceExtensions->LowerDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    return Status;
}

NTSTATUS NTAPI
ForwardIrpAndForget(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice;

    LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

/* Copied fom trunk PCI drivers */
NTSTATUS
DuplicateUnicodeString(ULONG Flags, PCUNICODE_STRING SourceString, PUNICODE_STRING DestinationString)
{
    if (SourceString == NULL || DestinationString == NULL
     || SourceString->Length > SourceString->MaximumLength
     || (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL)
     || Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING || Flags >= 4)
    {
        return STATUS_INVALID_PARAMETER;
    }


    if ((SourceString->Length == 0)
     && (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }
    else
    {
        USHORT DestMaxLength = SourceString->Length;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestMaxLength += sizeof(UNICODE_NULL);

        DestinationString->Buffer = ExAllocatePoolWithTag(NonPagedPool, DestMaxLength, USB_POOL_TAG);
        if (DestinationString->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = DestMaxLength;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}

