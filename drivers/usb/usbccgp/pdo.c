/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbccgp/pdo.c
 * PURPOSE:     USB  device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 *              Cameron Gutman
 */

#include "usbccgp.h"

NTSTATUS
USBCCGP_PdoHandleQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    LPWSTR Buffer;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // is there a device description
    //
    if (PDODeviceExtension->FunctionDescriptor->FunctionDescription.Length)
    {
        //
        // allocate buffer
        //
        Buffer = AllocateItem(NonPagedPool, PDODeviceExtension->FunctionDescriptor->FunctionDescription.Length + sizeof(WCHAR));
        if (!Buffer)
        {
            //
            // no memory
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // copy buffer
        //
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        RtlCopyMemory(Buffer, PDODeviceExtension->FunctionDescriptor->FunctionDescription.Buffer, PDODeviceExtension->FunctionDescriptor->FunctionDescription.Length);
        return STATUS_SUCCESS;
    }

    //
    // FIXME use GenericCompositeUSBDeviceString
    //
    UNIMPLEMENTED
    return Irp->IoStatus.Status;
}

NTSTATUS
USBCCGP_PdoHandleDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;
    PIO_STACK_LOCATION IoStack;

    DPRINT1("USBCCGP_PdoHandleDeviceRelations\n");

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // check if relation type is BusRelations
    //
    if (IoStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
    {
        //
        // PDO handles only target device relation
        //
        return Irp->IoStatus.Status;
    }

    //
    // allocate device relations
    //
    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(PagedPool, sizeof(DEVICE_RELATIONS));
    if (!DeviceRelations)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize device relations
    //
    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceObject;
    ObReferenceObject(DeviceObject);

    //
    // store result
    //
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    //
    // completed successfully
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBCCGP_PdoHandleQueryId(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PUNICODE_STRING DeviceString = NULL;
    UNICODE_STRING TempString;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    NTSTATUS Status;
    LPWSTR Buffer;

    //
    // get current irp stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID)
    {
        //
        // handle query device id
        //
        Status = USBCCGP_SyncForwardIrp(PDODeviceExtension->NextDeviceObject, Irp);

        //
        // FIXME append interface id
        //
        ASSERT(FALSE);
        return Status;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {
        //
        // handle instance id
        //
        DeviceString = &PDODeviceExtension->FunctionDescriptor->HardwareId;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
    {
        //
        // handle instance id
        //
        RtlInitUnicodeString(&TempString, L"0000");
        DeviceString = &TempString;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs)
    {
        //
        // handle instance id
        //
        DeviceString = &PDODeviceExtension->FunctionDescriptor->CompatibleId;
    }

    //
    // sanity check
    //
    ASSERT(DeviceString != NULL);

    //
    // allocate buffer
    //
    Buffer = AllocateItem(NonPagedPool, DeviceString->Length + sizeof(WCHAR));
    if (!Buffer)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // copy buffer
    //
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    RtlCopyMemory(Buffer, DeviceString->Buffer, DeviceString->Length);
    return STATUS_SUCCESS;
}

NTSTATUS
PDO_HandlePnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    NTSTATUS Status;

    //
    // get current stack location
    //
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // get device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // sanity check
    //
    ASSERT(PDODeviceExtension->Common.IsFDO == FALSE);

    switch(IoStack->MinorFunction)
    {
       case IRP_MN_QUERY_DEVICE_RELATIONS:
       {
           //
           // handle device relations
           //
           Status = USBCCGP_PdoHandleDeviceRelations(DeviceObject, Irp);
           break;
       }
       case IRP_MN_QUERY_DEVICE_TEXT:
       {
           //
           // handle query device text
           //
           Status = USBCCGP_PdoHandleQueryDeviceText(DeviceObject, Irp);
           break;
       }
       case IRP_MN_QUERY_ID:
       {
           //
           // handle request
           //
           Status = USBCCGP_PdoHandleQueryId(DeviceObject, Irp);
           break;
       }
       case IRP_MN_REMOVE_DEVICE:
       {
           DPRINT1("IRP_MN_REMOVE_DEVICE\n");

           /* Complete the IRP */
           Irp->IoStatus.Status = STATUS_SUCCESS;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);

           /* Delete the device object */
           IoDeleteDevice(DeviceObject);

           return STATUS_SUCCESS;
       }
       case IRP_MN_QUERY_CAPABILITIES:
       {
           //
           // copy device capabilities
           //
           RtlCopyMemory(IoStack->Parameters.DeviceCapabilities.Capabilities, &PDODeviceExtension->Capabilities, sizeof(DEVICE_CAPABILITIES));

           /* Complete the IRP */
           Irp->IoStatus.Status = STATUS_SUCCESS;
           IoCompleteRequest(Irp, IO_NO_INCREMENT);
           return STATUS_SUCCESS;
       }
       case IRP_MN_START_DEVICE:
       {
           //
           // no-op for PDO
           //
           DPRINT1("[USBCCGP] PDO IRP_MN_START\n");
           Status = STATUS_SUCCESS;
           break;
       }
       default:
        {
            //
            // do nothing
            //
            Status = Irp->IoStatus.Status;
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


NTSTATUS
PDO_Dispatch(
    PDEVICE_OBJECT DeviceObject, 
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->MajorFunction)
    {
        case IRP_MJ_PNP:
            return PDO_HandlePnp(DeviceObject, Irp);
        default:
            DPRINT1("PDO_Dispatch Function %x not implemented\n", IoStack->MajorFunction);
            ASSERT(FALSE);
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
    }

}
