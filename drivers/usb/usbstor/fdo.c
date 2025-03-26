/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     USB block storage device driver.
 * COPYRIGHT:   2005-2006 James Tabor
 *              2011-2012 Michael Martin (michael.martin@reactos.org)
 *              2011-2013 Johannes Anderwald (johannes.anderwald@reactos.org)
 *              2019 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>


#if DBG
static
VOID
USBSTOR_DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    DPRINT("Dumping Device Descriptor %p\n", DeviceDescriptor);
    DPRINT("bLength %x\n", DeviceDescriptor->bLength);
    DPRINT("bDescriptorType %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT("bcdUSB %x\n", DeviceDescriptor->bcdUSB);
    DPRINT("bDeviceClass %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT("bDeviceSubClass %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT("bDeviceProtocol %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT("bMaxPacketSize0 %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT("idVendor %x\n", DeviceDescriptor->idVendor);
    DPRINT("idProduct %x\n", DeviceDescriptor->idProduct);
    DPRINT("bcdDevice %x\n", DeviceDescriptor->bcdDevice);
    DPRINT("iManufacturer %x\n", DeviceDescriptor->iManufacturer);
    DPRINT("iProduct %x\n", DeviceDescriptor->iProduct);
    DPRINT("iSerialNumber %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT("bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
}
#endif

NTSTATUS
USBSTOR_FdoHandleDeviceRelations(
    IN PFDO_DEVICE_EXTENSION DeviceExtension,
    IN OUT PIRP Irp)
{
    INT32 DeviceCount = 0;
    LONG Index;
    PDEVICE_RELATIONS DeviceRelations;
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // FDO always only handles bus relations
    if (IoStack->Parameters.QueryDeviceRelations.Type == BusRelations)
    {
        // go through array and count device objects
        for (Index = 0; Index < max(DeviceExtension->MaxLUN, 1); Index++)
        {
            if (DeviceExtension->ChildPDO[Index])
            {
                DeviceCount++;
            }
        }

        DeviceRelations = ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_RELATIONS) + (DeviceCount - 1) * sizeof(PDEVICE_OBJECT), USB_STOR_TAG);
        if (!DeviceRelations)
        {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DeviceRelations->Count = 0;

        // add device objects
        for (Index = 0; Index < max(DeviceExtension->MaxLUN, 1); Index++)
        {
            if (DeviceExtension->ChildPDO[Index])
            {
                // store child pdo
                DeviceRelations->Objects[DeviceRelations->Count] = DeviceExtension->ChildPDO[Index];

                // add reference
                ObReferenceObject(DeviceExtension->ChildPDO[Index]);

                DeviceRelations->Count++;
            }
        }

        Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);

    return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
}

NTSTATUS
USBSTOR_FdoHandleRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension,
    IN OUT PIRP Irp)
{
    NTSTATUS Status;
    ULONG Index;

    DPRINT("Handling FDO removal %p\n", DeviceObject);

    // FIXME: wait for devices finished processing
    for (Index = 0; Index < USB_MAXCHILDREN; Index++)
    {
        if (DeviceExtension->ChildPDO[Index] != NULL)
        {
            DPRINT("Deleting PDO %p RefCount %x AttachedDevice %p \n", DeviceExtension->ChildPDO[Index], DeviceExtension->ChildPDO[Index]->ReferenceCount, DeviceExtension->ChildPDO[Index]->AttachedDevice);
            IoDeleteDevice(DeviceExtension->ChildPDO[Index]);
        }
    }

    // Freeing everything in DeviceExtension
    if (DeviceExtension->DeviceDescriptor)
        ExFreePoolWithTag(DeviceExtension->DeviceDescriptor, USB_STOR_TAG);
    if (DeviceExtension->ConfigurationDescriptor)
        ExFreePoolWithTag(DeviceExtension->ConfigurationDescriptor, USB_STOR_TAG);
    if (DeviceExtension->InterfaceInformation)
        ExFreePoolWithTag(DeviceExtension->InterfaceInformation, USB_STOR_TAG);
    if (DeviceExtension->ResetDeviceWorkItem)
        IoFreeWorkItem(DeviceExtension->ResetDeviceWorkItem);
    if (DeviceExtension->SerialNumber)
        ExFreePoolWithTag(DeviceExtension->SerialNumber, USB_STOR_TAG);

    // Send the IRP down the stack
    IoSkipCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);

    // Detach from the device stack
    IoDetachDevice(DeviceExtension->LowerDeviceObject);

    IoDeleteDevice(DeviceObject);

    return Status;
}

NTSTATUS
USBSTOR_FdoHandleStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension,
    IN OUT PIRP Irp)
{
    PUSB_INTERFACE_DESCRIPTOR InterfaceDesc;
    NTSTATUS Status;
    UCHAR Index = 0;
    PIO_WORKITEM WorkItem;

    // forward irp to lower device
    if (!IoForwardIrpSynchronously(DeviceExtension->LowerDeviceObject, Irp))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBSTOR_FdoHandleStartDevice Lower device failed to start %x\n", Status);
        return Status;
    }

    if (!DeviceExtension->ResetDeviceWorkItem)
    {
        WorkItem = IoAllocateWorkItem(DeviceObject);
        DeviceExtension->ResetDeviceWorkItem = WorkItem;

        if (!WorkItem)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // initialize irp queue
    USBSTOR_QueueInitialize(DeviceExtension);

    // first get device & configuration & string descriptor
    Status = USBSTOR_GetDescriptors(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBSTOR_FdoHandleStartDevice failed to get device descriptor with %x\n", Status);
        return Status;
    }

#if DBG
    USBSTOR_DumpDeviceDescriptor(DeviceExtension->DeviceDescriptor);
#endif

    // Check that this device uses bulk transfers and is SCSI

    InterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)DeviceExtension->ConfigurationDescriptor + sizeof(USB_CONFIGURATION_DESCRIPTOR));
    ASSERT(InterfaceDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
    ASSERT(InterfaceDesc->bLength == sizeof(USB_INTERFACE_DESCRIPTOR));

    DPRINT("bInterfaceSubClass %x\n", InterfaceDesc->bInterfaceSubClass);
    if (InterfaceDesc->bInterfaceProtocol != USB_PROTOCOL_BULK)
    {
        DPRINT1("USB Device is not a bulk only device and is not currently supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    if (InterfaceDesc->bInterfaceSubClass == USB_SUBCLASS_UFI)
    {
        DPRINT1("USB Floppy devices are not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    // now select an interface
    Status = USBSTOR_SelectConfigurationAndInterface(DeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        // failed to get device descriptor
        DPRINT1("USBSTOR_FdoHandleStartDevice failed to select configuration / interface with %x\n", Status);
        return Status;
    }

    // check if we got a bulk in + bulk out endpoint
    Status = USBSTOR_GetPipeHandles(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBSTOR_FdoHandleStartDevice no pipe handles %x\n", Status);
        return Status;
    }

    Status = USBSTOR_GetMaxLUN(DeviceExtension->LowerDeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("USBSTOR_FdoHandleStartDevice failed to get max lun %x\n", Status);
        return Status;
    }

    // now create for each LUN a device object, 1 minimum
    do
    {
        Status = USBSTOR_CreatePDO(DeviceObject, Index);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("USBSTOR_FdoHandleStartDevice USBSTOR_CreatePDO failed for Index %lu with Status %x\n", Index, Status);
            return Status;
        }

        Index++;
        DeviceExtension->InstanceCount++;

    } while(Index < DeviceExtension->MaxLUN);

#if 0
    //
    // finally get usb device interface
    //
    Status = USBSTOR_GetBusInterface(DeviceExtension->LowerDeviceObject, &DeviceExtension->BusInterface);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to device interface
        //
        DPRINT1("USBSTOR_FdoHandleStartDevice failed to get device interface %x\n", Status);
        return Status;
    }
#endif

    //IoStartTimer(DeviceObject);

    DPRINT("USBSTOR_FdoHandleStartDevice FDO is initialized\n");
    return STATUS_SUCCESS;
}

NTSTATUS
USBSTOR_FdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->Common.IsFDO);

    switch(IoStack->MinorFunction)
    {
        case IRP_MN_SURPRISE_REMOVAL:
        {
            DPRINT("IRP_MN_SURPRISE_REMOVAL %p\n", DeviceObject);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // forward irp to next device object
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DPRINT("IRP_MN_QUERY_DEVICE_RELATIONS %p Type: %u\n", DeviceObject, IoStack->Parameters.QueryDeviceRelations.Type);
            return USBSTOR_FdoHandleDeviceRelations(DeviceExtension, Irp);
        }
        case IRP_MN_STOP_DEVICE:
        {
            DPRINT1("USBSTOR_FdoHandlePnp: IRP_MN_STOP_DEVICE unimplemented\n");
            IoStopTimer(DeviceObject);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // forward irp to next device object
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            DPRINT("IRP_MN_REMOVE_DEVICE\n");

            return USBSTOR_FdoHandleRemoveDevice(DeviceObject, DeviceExtension, Irp);
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            // FIXME: set custom capabilities
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            if (DeviceExtension->IrpPendingCount != 0 || DeviceExtension->ActiveSrb != NULL)
            {
                /* We have pending requests */
                DPRINT1("Failing removal/stop request due to pending requests present\n");
                Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                /* We're all clear */
                Irp->IoStatus.Status = STATUS_SUCCESS;

                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
            }
            break;
        }
        case IRP_MN_START_DEVICE:
        {
            Status = USBSTOR_FdoHandleStartDevice(DeviceObject, DeviceExtension, Irp);
            break;
        }
        default:
        {
            // forward irp to next device object
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}
