/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/usbstor.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#define INITGUID
#include "usbstor.h"

/* PUBLIC AND PRIVATE FUNCTIONS **********************************************/

NTSTATUS STDCALL
IrpStub(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS STDCALL
AddDevice(IN PDRIVER_OBJECT DriverObject,
          IN PDEVICE_OBJECT pdo)
{
    return STATUS_SUCCESS;
}

VOID STDCALL
DriverUnload(PDRIVER_OBJECT DriverObject)
{
}

VOID STDCALL
StartIo(PUSBSTOR_DEVICE_EXTENSION DeviceExtension,
        PIRP Irp)
{
}

static NTSTATUS STDCALL
DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
DispatchCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_SUCCESS;
}


static NTSTATUS STDCALL
DispatchScsi(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
DispatchReadWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
DispatchSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL
DispatchPower(PDEVICE_OBJECT fido, PIRP Irp)
{
    DPRINT1("USBSTOR: IRP_MJ_POWER unimplemented\n");
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}



/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegPath)
{
    ULONG i;

    DPRINT("********* USB Storage *********\n");

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->DriverExtension->AddDevice = AddDevice;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = IrpStub;

    DriverObject->DriverStartIo = (PVOID)StartIo;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_READ] = DispatchReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchReadWrite;

    /* Scsi Miniport support */
    DriverObject->MajorFunction[IRP_MJ_SCSI] = DispatchScsi;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DispatchSystemControl;

    DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;

    return STATUS_SUCCESS;
}

