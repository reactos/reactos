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


#if DBG
static
VOID
USBSTOR_DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    FDPRINT(DBGLVL_PNP, "Dumping Device Descriptor %p\n", DeviceDescriptor);
    FDPRINT(DBGLVL_PNP, "bLength %x\n", DeviceDescriptor->bLength);
    FDPRINT(DBGLVL_PNP, "bDescriptorType %x\n", DeviceDescriptor->bDescriptorType);
    FDPRINT(DBGLVL_PNP, "bcdUSB %x\n", DeviceDescriptor->bcdUSB);
    FDPRINT(DBGLVL_PNP, "bDeviceClass %x\n", DeviceDescriptor->bDeviceClass);
    FDPRINT(DBGLVL_PNP, "bDeviceSubClass %x\n", DeviceDescriptor->bDeviceSubClass);
    FDPRINT(DBGLVL_PNP, "bDeviceProtocol %x\n", DeviceDescriptor->bDeviceProtocol);
    FDPRINT(DBGLVL_PNP, "bMaxPacketSize0 %x\n", DeviceDescriptor->bMaxPacketSize0);
    FDPRINT(DBGLVL_PNP, "idVendor %x\n", DeviceDescriptor->idVendor);
    FDPRINT(DBGLVL_PNP, "idProduct %x\n", DeviceDescriptor->idProduct);
    FDPRINT(DBGLVL_PNP, "bcdDevice %x\n", DeviceDescriptor->bcdDevice);
    FDPRINT(DBGLVL_PNP, "iManufacturer %x\n", DeviceDescriptor->iManufacturer);
    FDPRINT(DBGLVL_PNP, "iProduct %x\n", DeviceDescriptor->iProduct);
    FDPRINT(DBGLVL_PNP, "iSerialNumber %x\n", DeviceDescriptor->iSerialNumber);
    FDPRINT(DBGLVL_PNP, "bNumConfigurations %x\n", DeviceDescriptor->bNumConfigurations);
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

    FDPRINT(DBGLVL_PNP, "Handling FDO removal %p\n", DeviceObject);

    // FIXME: wait for devices finished processing
    for (Index = 0; Index < USB_MAXCHILDREN; Index++)
    {
        if (DeviceExtension->ChildPDO[Index] != NULL)
        {
            FDPRINT(DBGLVL_PNP, "Deleting PDO %p RefCount %x AttachedDevice %p \n", DeviceExtension->ChildPDO[Index], DeviceExtension->ChildPDO[Index]->ReferenceCount, DeviceExtension->ChildPDO[Index]->AttachedDevice);
            IoDeleteDevice(DeviceExtension->ChildPDO[Index]);
        }
    }

    // Freeing everything in DeviceExtension
    ASSERT(
        DeviceExtension->DeviceDescriptor &&
        DeviceExtension->ConfigurationDescriptor &&
        DeviceExtension->InterfaceInformation &&
        DeviceExtension->ResetDeviceWorkItem
    );

    ExFreePoolWithTag(DeviceExtension->DeviceDescriptor, USB_STOR_TAG);
    ExFreePoolWithTag(DeviceExtension->ConfigurationDescriptor, USB_STOR_TAG);
    ExFreePoolWithTag(DeviceExtension->InterfaceInformation, USB_STOR_TAG);
    IoFreeWorkItem(DeviceExtension->ResetDeviceWorkItem);

    if (DeviceExtension->SerialNumber)
    {
        ExFreePoolWithTag(DeviceExtension->SerialNumber, USB_STOR_TAG);
    }

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
    Status = USBSTOR_SyncForwardIrp(DeviceExtension->LowerDeviceObject, Irp);
    if (!NT_SUCCESS(Status))
    {
        ERR("USBSTOR_FdoHandleStartDevice Lower device failed to start %x\n", Status);
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
        ERR("USBSTOR_FdoHandleStartDevice failed to get device descriptor with %x\n", Status);
        return Status;
    }

#if DBG
    USBSTOR_DumpDeviceDescriptor(DeviceExtension->DeviceDescriptor);
#endif

    // Check that this device uses bulk transfers and is SCSI

    InterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)DeviceExtension->ConfigurationDescriptor + sizeof(USB_CONFIGURATION_DESCRIPTOR));
    ASSERT(InterfaceDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
    ASSERT(InterfaceDesc->bLength == sizeof(USB_INTERFACE_DESCRIPTOR));

    FDPRINT(DBGLVL_PNP, "bInterfaceSubClass %x\n", InterfaceDesc->bInterfaceSubClass);
    if (InterfaceDesc->bInterfaceProtocol != USB_PROTOCOL_BULK)
    {
        ERR("USB Device is not a bulk only device and is not currently supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    if (InterfaceDesc->bInterfaceSubClass == USB_SUBCLASS_UFI)
    {
        ERR("USB Floppy devices are not supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    // now select an interface
    Status = USBSTOR_SelectConfigurationAndInterface(DeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        // failed to get device descriptor
        ERR("Failed to select configuration / interface with %x\n", Status);
        return Status;
    }

    // check if we got a bulk in + bulk out endpoint
    Status = USBSTOR_GetPipeHandles(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        ERR("No pipe handles %x\n", Status);
        return Status;
    }

    Status = USBSTOR_GetMaxLUN(DeviceExtension->LowerDeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to get max lun %x\n", Status);
        return Status;
    }

    // now create for each LUN a device object, 1 minimum
    do
    {
        Status = USBSTOR_CreatePDO(DeviceObject, Index);

        if (!NT_SUCCESS(Status))
        {
            ERR("USBSTOR_CreatePDO failed for Index %lu with Status %x\n", Index, Status);
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
        ERR("USBSTOR_FdoHandleStartDevice failed to get device interface %x\n", Status);
        return Status;
    }
#endif

    //IoStartTimer(DeviceObject);

    FDPRINT(DBGLVL_PNP, "FDO is initialized\n");
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
            FDPRINT(DBGLVL_PNP, "IRP_MN_SURPRISE_REMOVAL %p\n", DeviceObject);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // forward irp to next device object
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            FDPRINT(DBGLVL_PNP, "IRP_MN_QUERY_DEVICE_RELATIONS %p Type: %u\n", DeviceObject, IoStack->Parameters.QueryDeviceRelations.Type);
            return USBSTOR_FdoHandleDeviceRelations(DeviceExtension, Irp);
        }
        case IRP_MN_STOP_DEVICE:
        {
            FDPRINT(DBGLVL_PNP, "IRP_MN_STOP_DEVICE\n");
            IoStopTimer(DeviceObject);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // forward irp to next device object
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            FDPRINT(DBGLVL_PNP, "IRP_MN_REMOVE_DEVICE\n");

            return USBSTOR_FdoHandleRemoveDevice(DeviceObject, DeviceExtension, Irp);
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            FDPRINT(DBGLVL_PNP, "IRP_MN_QUERY_CAPABILITIES\n");
            // FIXME: set custom capabilities
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            FDPRINT(DBGLVL_PNP, "IRP_MN_QUERY_STOP_DEVICE / IRP_MN_QUERY_REMOVE_DEVICE\n");
#if 0
            //
            // we can if nothing is pending
            //
            if (DeviceExtension->IrpPendingCount != 0 ||
                DeviceExtension->ActiveSrb != NULL)
#else
            if (TRUE)
#endif
            {
                /* We have pending requests */
                ERR("Failing removal/stop request due to pending requests present\n");
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
            FDPRINT(DBGLVL_PNP, "IRP_MN_START_DEVICE\n");
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
