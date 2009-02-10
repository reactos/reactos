/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/multimedia/portcls/irp.c
 * PURPOSE:         Port Class driver / IRP Handling
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */


#include "private.h"
#include <portcls.h>

/*
    A safe place for IRPs to be bounced to, if no handler has been
    set. Whether this is a good idea or not...?
*/
#if 0
static
NTSTATUS
NTAPI
IrpStub(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DPRINT1("IRP Stub called\n");

    return status;
}
#endif

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
    DPRINT1("PortClsCreate called\n");

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

    DPRINT1("PortClsPnp called\n");

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

            /* Assign the resource list to our extension */
            DeviceExt->resources = resource_list;

            ASSERT(DeviceExt->StartDevice);

            /* Call the StartDevice routine */
            DPRINT("Calling StartDevice at 0x%8p\n", DeviceExt->StartDevice);
            Status = DeviceExt->StartDevice(DeviceObject, Irp, resource_list);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("StartDevice returned a failure code [0x%8x]\n", Status);
                //resource_list->lpVtbl->Release(resource_list);

                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

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
            return STATUS_SUCCESS;

        case IRP_MN_QUERY_INTERFACE:
            DPRINT1("FIXME: IRP_MN_QUERY_INTERFACE: call next lower device object\n");
            /* FIXME
             * call next lower device object */
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            return Irp->IoStatus.Status;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            return Irp->IoStatus.Status;
    }

    DPRINT1("unhandled function %u\n", IoStack->MinorFunction);
    return STATUS_SUCCESS;
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
    DPRINT1("PortClsPower called\n");

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
    DPRINT1("PortClsSysControl called\n");

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

    DPRINT1("PcDispatchIrp called - handling IRP in PortCls\n");

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

static
NTSTATUS
NTAPI
IrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    KeSetEvent((PRKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_SUCCESS;
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

    DPRINT1("PcForwardIrpSynchronous\n");

    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
return STATUS_SUCCESS;
    /* initialize the notification event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* setup a completion routine */
    IoSetCompletionRoutine(Irp, IrpCompletionRoutine, (PVOID)&Event, TRUE, FALSE, FALSE);

    /* copy the current stack location */
    IoCopyCurrentIrpStackLocationToNext(Irp);

    DPRINT1("PcForwardIrpSynchronous %p Irp %p\n", DeviceExt->PrevDeviceObject, Irp);

    /* now call the driver */
    Status = IoCallDriver(DeviceExt->PrevDeviceObject, Irp);
    /* did the request complete yet */
    if (Status == STATUS_PENDING)
    {
        /* not yet, lets wait a bit */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = STATUS_SUCCESS;
    }
    DPRINT1("Returning status %x\n", Status);
    return Status;
}
