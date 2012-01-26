/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbccgp/fdo.c
 * PURPOSE:     USB  device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 *              Cameron Gutman
 */

#include "usbccgp.h"

NTSTATUS
NTAPI
FDO_QueryCapabilitiesCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    //
    // set event
    //
    KeSetEvent((PRKEVENT)Context, 0, FALSE);

    //
    // completion is done in the HidClassFDO_QueryCapabilities routine
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
FDO_QueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PDEVICE_CAPABILITIES Capabilities)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // init event
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // now allocte the irp
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
    // get next stack location
    //
    IoStack = IoGetNextIrpStackLocation(Irp);

    //
    // init stack location
    //
    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    IoStack->Parameters.DeviceCapabilities.Capabilities = Capabilities;

    //
    // set completion routine
    //
    IoSetCompletionRoutine(Irp, FDO_QueryCapabilitiesCompletionRoutine, (PVOID)&Event, TRUE, TRUE, TRUE);

    //
    // init capabilities
    //
    RtlZeroMemory(Capabilities, sizeof(DEVICE_CAPABILITIES));
    Capabilities->Size = sizeof(DEVICE_CAPABILITIES);
    Capabilities->Version = 1; // FIXME hardcoded constant
    Capabilities->Address = MAXULONG;
    Capabilities->UINumber = MAXULONG;

    //
    // pnp irps have default completion code
    //
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    //
    // call lower  device
    //
    Status = IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        //
        // wait for completion
        //
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    //
    // get status
    //
    Status = Irp->IoStatus.Status;

    //
    // complete request
    //
    IoFreeIrp(Irp);

    //
    // done
    //
    return Status;
}

NTSTATUS
FDO_DeviceRelations(
    PDEVICE_OBJECT DeviceObject, 
    PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FDO_StartDevice(
    PDEVICE_OBJECT DeviceObject, 
    PIRP Irp)
{
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    //
    // first start lower device
    //
    Status = USBCCGP_SyncForwardIrp(FDODeviceExtension->NextDeviceObject, Irp);

    if (!NT_SUCCESS(Status))
    {
        //
        // failed to start lower device
        //
        DPRINT1("FDO_StartDevice lower device failed to start with %x\n", Status);
        return Status;
    }

    // get descriptors
    Status = USBCCGP_GetDescriptors(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        // failed to start lower device
        DPRINT1("FDO_StartDevice failed to get descriptors with %x\n", Status);
        return Status;
    }

    // get capabilities
    Status = FDO_QueryCapabilities(DeviceObject, &FDODeviceExtension->Capabilities);
    if (!NT_SUCCESS(Status))
    {
        // failed to start lower device
        DPRINT1("FDO_StartDevice failed to get capabilities with %x\n", Status);
        return Status;
    }

    // now select the configuration
    Status = USBCCGP_SelectConfiguration(DeviceObject, FDODeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        // failed to select interface
        DPRINT1("FDO_StartDevice failed to get capabilities with %x\n", Status);
        return Status;
    }

    //
    // FIXME: parse usb interface association descriptor
    // and create PDO for each function
    //
    ASSERT(FALSE);
    return Status;
}


NTSTATUS
FDO_HandlePnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    // get device extension
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);


    // get stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            Status = FDO_StartDevice(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            Status = FDO_DeviceRelations(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            //
            // copy capabilities
            //
            RtlCopyMemory(IoStack->Parameters.DeviceCapabilities.Capabilities, &FDODeviceExtension->Capabilities, sizeof(DEVICE_CAPABILITIES));
            Status = USBCCGP_SyncForwardIrp(FDODeviceExtension->NextDeviceObject, Irp);
            if (NT_SUCCESS(Status))
            {
                //
                // surprise removal ok
                //
                IoStack->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
            }
            break;
       }
       default:
       {
            //
            // forward irp to next device object
            //
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
       }

    }

    //
    // complete request
    //
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;


}

NTSTATUS
FDO_Dispatch(
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
            return FDO_HandlePnp(DeviceObject, Irp);
        default:
            DPRINT1("FDO_Dispatch Function %x not implemented\n", IoStack->MajorFunction);
            ASSERT(FALSE);
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
    }
}


