/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/fdo.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

#define NDEBUG
#include <debug.h>

VOID
USBSTOR_DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    DPRINT1("Dumping Device Descriptor %p\n", DeviceDescriptor);
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

NTSTATUS
USBSTOR_FdoHandleDeviceRelations(
    IN PFDO_DEVICE_EXTENSION DeviceExtension,
    IN OUT PIRP Irp)
{
    ULONG DeviceCount = 0;
    LONG Index;
    PDEVICE_RELATIONS DeviceRelations;
    PIO_STACK_LOCATION IoStack;

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // check if relation type is BusRelations
    //
    if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
    {
        //
        // FDO always only handles bus relations
        //
        return USBSTOR_SyncForwardIrp(DeviceExtension->LowerDeviceObject, Irp);
    }

    //
    // go through array and count device objects
    //
    for (Index = 0; Index < max(DeviceExtension->MaxLUN, 1); Index++)
    {
        if (DeviceExtension->ChildPDO[Index])
        {
            //
            // child pdo
            //
            DeviceCount++;
        }
    }

    //
    // allocate device relations
    //
    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(PagedPool, sizeof(DEVICE_RELATIONS) + (DeviceCount > 1 ? (DeviceCount-1) * sizeof(PDEVICE_OBJECT) : 0));
    if (!DeviceRelations)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // add device objects
    //
    for(Index = 0; Index < max(DeviceExtension->MaxLUN, 1); Index++)
    {
        if (DeviceExtension->ChildPDO[Index])
        {
            //
            // store child pdo
            //
            DeviceRelations->Objects[DeviceRelations->Count] = DeviceExtension->ChildPDO[Index];

            //
            // add reference
            //
            ObReferenceObject(DeviceExtension->ChildPDO[Index]);

            //
            // increment count
            //
            DeviceRelations->Count++;
        }
    }

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    //
    // request completed successfully
    //
    return STATUS_SUCCESS;
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

    /* FIXME: wait for devices finished processing */
    for(Index = 0; Index < 16; Index++)
    {
        if (DeviceExtension->ChildPDO[Index] != NULL)
        {
            DPRINT("Deleting PDO %p RefCount %x AttachedDevice %p \n", DeviceExtension->ChildPDO[Index], DeviceExtension->ChildPDO[Index]->ReferenceCount, DeviceExtension->ChildPDO[Index]->AttachedDevice);
            IoDeleteDevice(DeviceExtension->ChildPDO[Index]);
        }
    }

    /* Send the IRP down the stack */
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);

    /* Detach from the device stack */
    IoDetachDevice(DeviceExtension->LowerDeviceObject);

    /* Delete the device object */
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

    //
    // forward irp to lower device
    //
    Status = USBSTOR_SyncForwardIrp(DeviceExtension->LowerDeviceObject, Irp);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to start
        //
        DPRINT1("USBSTOR_FdoHandleStartDevice Lower device failed to start %x\n", Status);
        return Status;
    }

    //
    // initialize irp queue
    //
    USBSTOR_QueueInitialize(DeviceExtension);

    //
    // first get device & configuration & string descriptor
    //
    Status = USBSTOR_GetDescriptors(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get device descriptor
        //
        DPRINT1("USBSTOR_FdoHandleStartDevice failed to get device descriptor with %x\n", Status);
        return Status;
    }

    //
    // dump device descriptor
    //
    USBSTOR_DumpDeviceDescriptor(DeviceExtension->DeviceDescriptor);

    //
    // Check that this device uses bulk transfers and is SCSI
    //
    InterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)DeviceExtension->ConfigurationDescriptor + sizeof(USB_CONFIGURATION_DESCRIPTOR));

    //
    // sanity check
    //
    ASSERT(InterfaceDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE);
    ASSERT(InterfaceDesc->bLength == sizeof(USB_INTERFACE_DESCRIPTOR));

    DPRINT("bInterfaceSubClass %x\n", InterfaceDesc->bInterfaceSubClass);
    if (InterfaceDesc->bInterfaceProtocol != 0x50)
    {
        DPRINT1("USB Device is not a bulk only device and is not currently supported\n");
        return STATUS_NOT_SUPPORTED;
    }

    if (InterfaceDesc->bInterfaceSubClass != 0x06)
    {
        //
        // FIXME: need to pad CDBs to 12 byte
        // mode select commands must be translated from 1AH / 15h to 5AH / 55h
        //
        DPRINT1("[USBSTOR] Error: need to pad CDBs\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    //
    // now select an interface
    //
    Status = USBSTOR_SelectConfigurationAndInterface(DeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get device descriptor
        //
        DPRINT1("USBSTOR_FdoHandleStartDevice failed to select configuration / interface with %x\n", Status);
        return Status;
    }

    //
    // check if we got a bulk in + bulk out endpoint
    //
    Status = USBSTOR_GetPipeHandles(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get pipe handles descriptor
        //
        DPRINT1("USBSTOR_FdoHandleStartDevice no pipe handles %x\n", Status);
        return Status;
    }

    //
    // get num of lun which are supported
    //
    Status = USBSTOR_GetMaxLUN(DeviceExtension->LowerDeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get max LUN
        //
        DPRINT1("USBSTOR_FdoHandleStartDevice failed to get max lun %x\n", Status);
        return Status;
    }

    //
    // now create for each LUN a device object, 1 minimum
    //
    do
    {
        //
        // create pdo
        //
        Status = USBSTOR_CreatePDO(DeviceObject, Index);

        //
        // check for failure
        //
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to create child pdo
            //
            DPRINT1("USBSTOR_FdoHandleStartDevice USBSTOR_CreatePDO failed for Index %lu with Status %x\n", Index, Status);
            return Status;
        }

        //
        // increment pdo index
        //
        Index++;
        DeviceExtension->InstanceCount++;

    }while(Index < DeviceExtension->MaxLUN);

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


    //
    // start the timer
    //
    //IoStartTimer(DeviceObject);


    //
    // fdo is now initialized
    //
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

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(DeviceExtension->Common.IsFDO);

    switch(IoStack->MinorFunction)
    {
       case IRP_MN_SURPRISE_REMOVAL:
       {
           DPRINT("IRP_MN_SURPRISE_REMOVAL %p\n", DeviceObject);
           Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // forward irp to next device object
            //
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
       }
       case IRP_MN_QUERY_DEVICE_RELATIONS:
       {
           DPRINT("IRP_MN_QUERY_DEVICE_RELATIONS %p\n", DeviceObject);
           Status = USBSTOR_FdoHandleDeviceRelations(DeviceExtension, Irp);
           break;
       }
       case IRP_MN_STOP_DEVICE:
       {
           DPRINT1("USBSTOR_FdoHandlePnp: IRP_MN_STOP_DEVICE unimplemented\n");
           IoStopTimer(DeviceObject);
           Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // forward irp to next device object
            //
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
           //
           // FIXME: set custom capabilities
           //
           IoSkipCurrentIrpStackLocation(Irp);
           return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
       }
       case IRP_MN_QUERY_STOP_DEVICE:
       case IRP_MN_QUERY_REMOVE_DEVICE:
       {
#if 0
           //
           // we can if nothing is pending
           //
           if (DeviceExtension->IrpPendingCount != 0 ||
               DeviceExtension->CurrentSrb != NULL)
#else
           if (TRUE)
#endif
           {
               ///* We have pending requests */
               //DPRINT1("Failing removal/stop request due to pending requests present\n");
               //Status = STATUS_UNSUCCESSFUL;

               Irp->IoStatus.Status = STATUS_SUCCESS;
               IoSkipCurrentIrpStackLocation(Irp);
               return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
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
            //
            // forward irp to next device object
            //
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(DeviceExtension->LowerDeviceObject, Irp);
        }
    }

    //
    // complete request
    //
    if (Status != STATUS_PENDING)
    {
        //
        // store result
        //
        Irp->IoStatus.Status = Status;

        //
        // complete request
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    //
    // done processing
    //
    return Status;
}
