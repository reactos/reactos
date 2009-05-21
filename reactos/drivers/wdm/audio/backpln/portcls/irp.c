/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/wdm/audio/backpln/portcls/irp.c
 * PURPOSE:         Port Class driver / IRP Handling
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 * HISTORY:
 *                  27 Jan 07   Created
 */


#include "private.h"
#include <portcls.h>

/*
    Handles IRP_MJ_CREATE, which occurs when someone wants to make use of
    a device.
*/
NTSTATUS
NTAPI
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("PortClsCreate called\n");

    return KsDispatchIrp(DeviceObject, Irp);
}


/*
    IRP_MJ_PNP handler
    Used for things like IRP_MN_START_DEVICE
*/
NTSTATUS
NTAPI
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    PIO_STACK_LOCATION IoStack;
    IResourceList* resource_list = NULL;

    DPRINT("PortClsPnp called\n");

    DeviceExt = (PPCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    ASSERT(DeviceExt);

    /*
        if IRP_MN_START_DEVICE, call the driver's customer start device routine.
        Before we do so, we must create a ResourceList to pass to the Start
        routine.
    */
    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            DPRINT("IRP_MN_START_DEVICE\n");

            /* Create the resource list */
            Status = PcNewResourceList(
                        &resource_list,
                        NULL,
                        PagedPool,
                        IoStack->Parameters.StartDevice.AllocatedResourcesTranslated,
                        IoStack->Parameters.StartDevice.AllocatedResources);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("PcNewResourceList failed [0x%8x]\n", Status);
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            /* forward irp to lower device object */
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);

            if (!NT_SUCCESS(Status))
            {
                /* lower device object failed to start */
                resource_list->lpVtbl->Release(resource_list);
                /* complete the request */
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                /* return result */
                return Status;
            }

            /* sanity check */
            ASSERT(DeviceExt->StartDevice);
            /* Call the StartDevice routine */
            DPRINT("Calling StartDevice at 0x%8p\n", DeviceExt->StartDevice);
            Status = DeviceExt->StartDevice(DeviceObject, Irp, resource_list);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("StartDevice returned a failure code [0x%8x]\n", Status);
                resource_list->lpVtbl->Release(resource_list);

                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            /* Assign the resource list to our extension */
            DeviceExt->resources = resource_list;

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;

        case IRP_MN_REMOVE_DEVICE:
            /* Clean up */
            DPRINT("IRP_MN_REMOVE_DEVICE\n");

            DeviceExt->resources->lpVtbl->Release(DeviceExt->resources);
            IoDeleteDevice(DeviceObject);

            /* Do not complete? */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;

        case IRP_MN_QUERY_INTERFACE:
            DPRINT("IRP_MN_QUERY_INTERFACE\n");
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            return Status;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT("IRP_MN_QUERY_DEVICE_RELATIONS\n");
            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NOT_SUPPORTED;
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            DPRINT("IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
       case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            DPRINT("IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = PcForwardIrpSynchronous(DeviceObject, Irp);
            return Status;
    }

    DPRINT1("unhandled function %u\n", IoStack->MinorFunction);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

/*
    Power management. Handles IRP_MJ_POWER
    (not implemented)
*/
NTSTATUS
NTAPI
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("PortClsPower called\n");

    /* TODO */

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/*
    System control. Handles IRP_MJ_SYSTEM_CONTROL
    (not implemented)
*/
NTSTATUS
NTAPI
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("PortClsSysControl called\n");

    /* TODO */

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


/*
    ==========================================================================
    API EXPORTS
    ==========================================================================
*/

/*
    Drivers may implement their own IRP handlers. If a driver decides to let
    PortCls handle the IRP, it can do so by calling this.
*/
NTSTATUS NTAPI
PcDispatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    DPRINT("PcDispatchIrp called - handling IRP in PortCls\n");

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch ( IoStack->MajorFunction )
    {
        /* PortCls */
        case IRP_MJ_CREATE :
            return PortClsCreate(DeviceObject, Irp);

        case IRP_MJ_PNP :
            return PortClsPnp(DeviceObject, Irp);

        case IRP_MJ_POWER :
            return PortClsPower(DeviceObject, Irp);

        case IRP_MJ_DEVICE_CONTROL:
            return KsDispatchIrp(DeviceObject, Irp);

        case IRP_MJ_CLOSE:
            return KsDispatchIrp(DeviceObject, Irp);

        case IRP_MJ_SYSTEM_CONTROL :
            return PortClsSysControl(DeviceObject, Irp);

        default:
            DPRINT1("Unhandled function %x\n", IoStack->MajorFunction);
            break;
    };

    /* If we reach here, we just complete the IRP */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcCompleteIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  NTSTATUS Status)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CompletionRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    if (Irp->PendingReturned == TRUE)
    {
        KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
PcForwardIrpSynchronous(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    KEVENT Event;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* initialize the notification event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, CompletionRoutine, (PVOID)&Event, TRUE, TRUE, TRUE);

    /* now call the driver */
    Status = IoCallDriver(DeviceExt->PrevDeviceObject, Irp);
    /* did the request complete yet */
    if (Status == STATUS_PENDING)
    {
        /* not yet, lets wait a bit */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }
    return Status;
}
