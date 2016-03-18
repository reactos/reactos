/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * PURPOSE:         Parport driver loading/unloading
 */

#include "parport.h"

static DRIVER_UNLOAD DriverUnload;
static DRIVER_DISPATCH DispatchCreate;
static DRIVER_DISPATCH DispatchClose;
static DRIVER_DISPATCH DispatchCleanup;
static DRIVER_DISPATCH DispatchPnp;
static DRIVER_DISPATCH DispatchPower;
DRIVER_INITIALIZE DriverEntry;


/* FUNCTIONS ****************************************************************/

static
VOID
NTAPI
DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT("Parport DriverUnload\n");
}


static
NTSTATUS
NTAPI
DispatchCreate(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        return FdoCreate(DeviceObject, Irp);
    else
        return PdoCreate(DeviceObject, Irp);
}


static
NTSTATUS
NTAPI
DispatchClose(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        return FdoClose(DeviceObject, Irp);
    else
        return PdoClose(DeviceObject, Irp);
}


static
NTSTATUS
NTAPI
DispatchCleanup(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        return FdoCleanup(DeviceObject, Irp);
    else
        return PdoCleanup(DeviceObject, Irp);
}


static
NTSTATUS
NTAPI
DispatchRead(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        return FdoRead(DeviceObject, Irp);
    else
        return PdoRead(DeviceObject, Irp);
}


static
NTSTATUS
NTAPI
DispatchWrite(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        return FdoWrite(DeviceObject, Irp);
    else
        return PdoWrite(DeviceObject, Irp);
}


static
NTSTATUS
NTAPI
DispatchPnp(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        return FdoPnp(DeviceObject, Irp);
    else
        return PdoPnp(DeviceObject, Irp);
}


static
NTSTATUS
NTAPI
DispatchPower(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp)
{
    if (((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Common.IsFDO)
        return FdoPower(DeviceObject, Irp);
    else
        return PdoPower(DeviceObject, Irp);
}


NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegPath)
{
    ULONG i;

    DPRINT("Parport DriverEntry\n");

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->DriverExtension->AddDevice = AddDevice;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = ForwardIrpAndForget;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchWrite;
//    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
//    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = DispatchQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;

    return STATUS_SUCCESS;
}
