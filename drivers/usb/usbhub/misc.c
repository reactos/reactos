/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/fdo.c
 * PURPOSE:         Misc helper functions
 * PROGRAMMERS:
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbhub.h"

#define NDEBUG
#include <debug.h>

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

//----------------------------------------------------------------------------------------
VOID
DumpConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    DPRINT1("Dumping ConfigurationDescriptor %x\n", ConfigurationDescriptor);
    DPRINT1("bLength %x\n", ConfigurationDescriptor->bLength);
    DPRINT1("bDescriptorType %x\n", ConfigurationDescriptor->bDescriptorType);
    DPRINT1("wTotalLength %x\n", ConfigurationDescriptor->wTotalLength);
    DPRINT1("bNumInterfaces %x\n", ConfigurationDescriptor->bNumInterfaces);
    DPRINT1("bConfigurationValue %x\n", ConfigurationDescriptor->bConfigurationValue);
    DPRINT1("iConfiguration %x\n", ConfigurationDescriptor->iConfiguration);
    DPRINT1("bmAttributes %x\n", ConfigurationDescriptor->bmAttributes);
    DPRINT1("MaxPower %x\n", ConfigurationDescriptor->MaxPower);
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

NTSTATUS
NTAPI
ForwardIrpAndWaitCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ForwardIrpAndWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, ForwardIrpAndWaitCompletion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = Irp->IoStatus.Status;
    }

    return Status;
}

NTSTATUS
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice = ((PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDeviceObject;

    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}

NTSTATUS
SubmitRequestToRootHub(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG IoControlCode,
    OUT PVOID OutParameter1,
    OUT PVOID OutParameter2)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Build Control Request
    //
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        RootHubDeviceObject,
                                        NULL, 0,
                                        NULL, 0,
                                        TRUE,
                                        &Event,
                                        &IoStatus);

    if (Irp == NULL)
    {
        DPRINT("Usbhub: IoBuildDeviceIoControlRequest() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the status block before sending the IRP
    //
    IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoStatus.Information = 0;

    //
    // Get Next Stack Location and Initialize it
    //
    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->Parameters.Others.Argument1 = OutParameter1;
    Stack->Parameters.Others.Argument2 = OutParameter2;

    //
    // Call RootHub
    //
    Status = IoCallDriver(RootHubDeviceObject, Irp);

    //
    // Its ok to block here as this function is called in an nonarbitrary thread
    //
    if    (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    //
    // The IO Manager will free the IRP
    //

    return Status;
}

NTSTATUS
NTAPI
FDO_QueryInterfaceCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    /* Set event */
    KeSetEvent((PRKEVENT)Context, 0, FALSE);

    /* Completion is done in the HidClassFDO_QueryCapabilities routine */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
FDO_QueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_BUS_INTERFACE_USBDI_V2 Interface)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PHUB_DEVICE_EXTENSION HubDeviceExtension;

    /* Get device extension */
    HubDeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(HubDeviceExtension->Common.IsFDO);

    /* Init event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Now allocte the irp */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        /* No memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);

    /* Init stack location */
    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IoStack->Parameters.QueryInterface.Interface = (PINTERFACE)Interface;
    IoStack->Parameters.QueryInterface.InterfaceType = &USB_BUS_INTERFACE_USBDI_GUID;
    IoStack->Parameters.QueryInterface.Version = USB_BUSIF_USBDI_VERSION_2;
    IoStack->Parameters.QueryInterface.Size = sizeof(USB_BUS_INTERFACE_USBDI_V2);


    /* Set completion routine */
    IoSetCompletionRoutine(Irp,
                           FDO_QueryInterfaceCompletionRoutine,
                           (PVOID)&Event,
                           TRUE,
                           TRUE,
                           TRUE);

    /* Pnp irps have default completion code */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    /* Call lower device */
    Status = IoCallDriver(HubDeviceExtension->LowerDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    /* Get status */
    Status = Irp->IoStatus.Status;

    /* Complete request */
    IoFreeIrp(Irp);

    /* Done */
    return Status;
}
