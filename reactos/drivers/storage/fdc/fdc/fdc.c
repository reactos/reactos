/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/storage/fdc/fdc/fdc.c
 * PURPOSE:         Floppy class driver entry point
 *
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <ntddk.h>

#include "fdc.h"

#define NDEBUG
#include <debug.h>

static NTSTATUS
NTAPI
FdcAddDevice(IN PDRIVER_OBJECT DriverObject,
             IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PFDC_FDO_EXTENSION DeviceExtension;
    
    Status = IoCreateDevice(DriverObject,
                            sizeof(FDC_FDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create device object (Status: 0x%x)\n", Status);
        return Status;
    }
    
    DeviceExtension = DeviceObject->DeviceExtension;
    
    DeviceExtension->Common.IsFDO = TRUE;
    DeviceExtension->Common.DeviceObject = DeviceObject;
    DeviceExtension->Common.DriverObject = DriverObject;
    
    InitializeListHead(&DeviceExtension->FloppyDriveList);
    DeviceExtension->FloppyDriveListCount = 0;
    KeInitializeSpinLock(&DeviceExtension->FloppyDriveListLock);
    
    DeviceObject->Flags |= DO_POWER_PAGABLE;
    
    DeviceExtension->Ldo = IoAttachDeviceToDeviceStack(DeviceObject,
                                                       PhysicalDeviceObject);
    if (!DeviceExtension->Ldo)
    {
        DPRINT1("Failed to attach to device stack\n");
        IoDeleteDevice(DeviceObject);
        return STATUS_UNSUCCESSFUL;
    }
    
    DPRINT("FdcAddDevice completed successfully\n");
    
    return STATUS_SUCCESS;
}

static NTSTATUS
NTAPI
FdcPnpDispatch(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    PFDC_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;

    if (DevExt->IsFDO)
        return FdcFdoPnpDispatch(DeviceObject, Irp);
    else
        return FdcPdoPnpDispatch(DeviceObject, Irp);
}

static NTSTATUS
NTAPI
FdcPowerDispatch(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PFDC_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;
    
    if (DevExt->IsFDO)
        return FdcFdoPowerDispatch(DeviceObject, Irp);
    else
        return FdcPdoPowerDispatch(DeviceObject, Irp);
}

static NTSTATUS
NTAPI
FdcDeviceControlDispatch(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    PFDC_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;
    
    if (DevExt->IsFDO)
        return FdcFdoDeviceControlDispatch(DeviceObject, Irp);
    else
        return FdcPdoDeviceControlDispatch(DeviceObject, Irp);
}

static NTSTATUS
NTAPI
FdcInternalDeviceControlDispatch(IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp)
{
    PFDC_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;
    
    if (DevExt->IsFDO)
        return FdcFdoInternalDeviceControlDispatch(DeviceObject, Irp);
    else
        return FdcPdoInternalDeviceControlDispatch(DeviceObject, Irp);
}

static NTSTATUS
NTAPI
FdcCreateClose(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;
    
    return STATUS_SUCCESS;
}

static VOID
NTAPI
FdcUnload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT1("FDC Unloaded\n");
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegPath)
{
    DriverObject->DriverUnload = FdcUnload;
    DriverObject->DriverExtension->AddDevice = FdcAddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FdcCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FdcCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FdcDeviceControlDispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = FdcInternalDeviceControlDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = FdcPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = FdcPowerDispatch;
    
    return STATUS_SUCCESS;
}