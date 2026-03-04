/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         HdAudio main entry point
 * COPYRIGHT:       Copyright 2025 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

extern "C" DRIVER_INITIALIZE DriverEntry;

NTSTATUS
NTAPI
HDAUDIO_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHDAUDIO_DEVICE_EXTENSION DeviceExtension;

    // get current stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE || IoStack->MinorFunction == IRP_MN_STOP_DEVICE ||
        IoStack->MinorFunction == IRP_MN_SURPRISE_REMOVAL)
    {
        DeviceExtension = (PHDAUDIO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
        if (DeviceExtension->AdapterCommon)
        {
            DeviceExtension->AdapterCommon->Release();
            DeviceExtension->AdapterCommon = NULL;
        }
    }
    return PcDispatchIrp(DeviceObject, Irp);
}


NTSTATUS
NTAPI
HDAUDIO_DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    return PcDispatchIrp(DeviceObject, Irp);
}

 NTSTATUS
 NTAPI
 HDAUDIO_StartDevice(
     IN PDEVICE_OBJECT DeviceObject,
     IN PIRP Irp,
     IN PRESOURCELIST ResourceList)
 {
    NTSTATUS Status;
    PHDAUDIO_DEVICE_EXTENSION DeviceExtension;

    // get device extension
    DeviceExtension = (PHDAUDIO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // allocate common adapter
    Status = HDAUDIO_AllocateCommonAdapter(&DeviceExtension->AdapterCommon);
    if (!NT_SUCCESS(Status))
    {
        // failed to allocate common adapter
        DPRINT1("HDAUDIO: Failed to allocate common adapter %x\n", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // init common adapter
    Status = HDAUDIO_InitializeCommonAdapter(DeviceExtension->AdapterCommon, DeviceObject, Irp, ResourceList);
    if (!NT_SUCCESS(Status))
    {
        // failed to initialize common adapter
        DPRINT1("HDAUDIO: Failed to initialize common adapter with %x\n", Status);
        DeviceExtension->AdapterCommon->Release();
        DeviceExtension->AdapterCommon = NULL;
        return Status;
    }

    // register adapter power management
    Status = PcRegisterAdapterPowerManagement(PUNKNOWN(DeviceExtension->AdapterCommon), DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        // failed to register adapter power management
        DPRINT1("HDAUDIO: Failed to register adapter power management %x\n", Status);
        DeviceExtension->AdapterCommon->Release();
        DeviceExtension->AdapterCommon = NULL;
        return Status;
    }
    DPRINT1("HDAUDIO: StartDevice completed success\n");
    return Status;
 }


NTSTATUS
NTAPI
HDAUDIO_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    UNICODE_STRING SymbolicLinkName;
    NTSTATUS Status;

    // check parameters
    if (!DriverObject || !PhysicalDeviceObject)
        return STATUS_INVALID_PARAMETER;

    // add adapter device
    Status = PcAddAdapterDevice(DriverObject, PhysicalDeviceObject, HDAUDIO_StartDevice, 50, PORT_CLASS_DEVICE_EXTENSION_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: PcAddAdapterDevice failed with %x\n", Status);
        return Status;
    }

    // register device interface
    Status = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                       &GUID_HDAUDIO_BUS_INTERFACE_V2,
                                       NULL,
                                       &SymbolicLinkName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: IoRegisterDeviceInterface failed with %x\n", Status);
        return Status;
    }

    // enable device interface
    Status = IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: IoSetDeviceInterfaceState failed with %x\n", Status);
    }
    return Status;
}

VOID
NTAPI
HDAUDIO_Unload(
    IN PDRIVER_OBJECT DriverObject)
{
}

extern
"C"
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    DPRINT1("HDAUDIO: DriverEntry\n");

    Status = PcInitializeAdapterDriver(DriverObject, RegistryPath, HDAUDIO_AddDevice);
    if (NT_SUCCESS(Status))
    {
        DriverObject->DriverUnload = HDAUDIO_Unload;
        DriverObject->MajorFunction[IRP_MJ_PNP] = HDAUDIO_Pnp;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HDAUDIO_DeviceControl;
    }
    return Status;
}
