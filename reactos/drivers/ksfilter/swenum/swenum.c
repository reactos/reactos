/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/swenum/swenum.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "precomp.h"

const GUID KSMEDIUMSETID_Standard = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};


NTSTATUS
NTAPI
SwDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{

    UNIMPLEMENTED;

    /* just complete the irp */
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* done */
    return STATUS_SUCCESS;

}

NTSTATUS
NTAPI
SwDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    BOOLEAN ChildDevice;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT PnpDeviceObject = NULL;

    /* check if the device object is a child device */
    Status = KsIsBusEnumChildDevice(DeviceObject, &ChildDevice);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* let ks handle it */
    Status = KsServiceBusEnumPnpRequest(DeviceObject, Irp);

    if (!NT_SUCCESS(Status))
    {
        /* invalid request or not supported */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* get bus enum pnp object */
    Status = KsGetBusEnumPnpDeviceObject(DeviceObject, &PnpDeviceObject);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed to get pnp object */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* sanity check */
    ASSERT(PnpDeviceObject);

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        /* delete the device */
        IoDeleteDevice(DeviceObject);
    }

    /* skip current location */
    IoSkipCurrentIrpStackLocation(Irp);
    /* call the pnp device object */
    return IoCallDriver(PnpDeviceObject, Irp);
}

NTSTATUS
NTAPI
SwDispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    BOOLEAN ChildDevice;
    PDEVICE_OBJECT PnpDeviceObject;

    /* check if the device object is a child device */
    Status = KsIsBusEnumChildDevice(DeviceObject, &ChildDevice);

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        if (!ChildDevice)
        {
            /* bus devices dont support internal requests */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        /* get bus enum pnp object */
        Status = KsGetBusEnumPnpDeviceObject(DeviceObject, &PnpDeviceObject);

        /* check for success */
        if (NT_SUCCESS(Status))
        {
            /* skip current location */
            IoSkipCurrentIrpStackLocation(Irp);
            /* call the pnp device object */
            return IoCallDriver(PnpDeviceObject, Irp);
        }

    }

    /* complete the request */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;

}

NTSTATUS
NTAPI
SwDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SWENUM_INSTALL_INTERFACE)
    {
        /* install interface */
        Status = KsInstallBusEnumInterface(Irp);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SWENUM_REMOVE_INTERFACE)
    {
        /* remove interface */
        Status = KsRemoveBusEnumInterface(Irp);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SWENUM_GET_BUS_ID)
    {
        /* get bus id */
        return KsGetBusEnumIdentifier(Irp);
    }

    /* store result */
    Irp->IoStatus.Status = Status;

    /* complete irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* done */
    return Status;
}


NTSTATUS
NTAPI
SwDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    BOOLEAN ChildDevice;

    /* check if the device object is a child device */
    Status = KsIsBusEnumChildDevice(DeviceObject, &ChildDevice);

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        if (ChildDevice)
        {
            /* child devices cant create devices */
            Irp->IoStatus.Status = STATUS_OBJECT_NAME_NOT_FOUND;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
        /* perform the create request */
        Status = KsServiceBusEnumCreateRequest(DeviceObject, Irp);
    }

    /* check the irp is pending */
    if (Status != STATUS_PENDING)
    {
        /* irp is ok to complete */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}


NTSTATUS
NTAPI
SwDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    /* just complete the irp */
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* complete the irp */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* done */
    return STATUS_SUCCESS;

}

NTSTATUS
NTAPI
SwAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT FunctionalDeviceObject;

    /* create the device */
    Status = IoCreateDevice(DriverObject, sizeof(KSDEVICE_HEADER), NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &FunctionalDeviceObject);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* create the bus enum object */
    Status = KsCreateBusEnumObject(L"SW", FunctionalDeviceObject, PhysicalDeviceObject, NULL, &KSMEDIUMSETID_Standard, L"Devices");

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        /* set device flags */
        FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
        FunctionalDeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;
    }
    else
    {
        /* failed to create bus enum object */
        IoDeleteDevice(FunctionalDeviceObject);
    }

    /* done */
    return Status;
}

VOID
NTAPI
SwUnload(
    IN  PDRIVER_OBJECT DriverObject)
{
    /* nop */
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName)
{

    /* setup add device routine */
    DriverObject->DriverExtension->AddDevice = SwAddDevice;

    /* setup unload routine */
    DriverObject->DriverUnload = SwUnload;

    /* misc irp handling routines */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = SwDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SwDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_PNP] = SwDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = SwDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SwDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = SwDispatchSystemControl;


    return STATUS_SUCCESS;
}

