/*
 * PROJECT:         ReactOS Kernel Streaming Mixer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/wdm/audio/filters/kmixer/kmixer.c
 * PURPOSE:         main entry point
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "kmixer.h"

#define YDEBUG
#include <debug.h>

NTSTATUS
NTAPI
KMix_Pnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KMix_Pnp called for func %x\n", IrpStack->MinorFunction);

    if (IrpStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    return KsDefaultDispatchPnp(DeviceObject, Irp);
}

VOID
NTAPI
KMix_Unload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT1("SysAudio_Unload called\n");
}

NTSTATUS
NTAPI
KMix_AddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\kmixer");
    PDEVICE_OBJECT DeviceObject;
    PKMIXER_DEVICE_EXT DeviceExtension;
    PDEVICE_OBJECT NextDeviceObject;

    DPRINT1("KMix_InstallDevice called\n");

    /* create the device */
    Status = IoCreateDevice(DriverObject,
                            sizeof(KMIXER_DEVICE_EXT),
                            &DeviceName,
                            FILE_DEVICE_KS,
                            0,
                            FALSE,
                            &DeviceObject);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to create \\Device\\kmixer !\n");
        return Status;
    }

    DeviceExtension = (PKMIXER_DEVICE_EXT)DeviceObject->DeviceExtension;
    /* initialize device extension */
    RtlZeroMemory(DeviceExtension, sizeof(KMIXER_DEVICE_EXT));


    Status = KMixAllocateDeviceHeader(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KMixAllocateDeviceHeader failed with %x\n", Status);
        goto cleanup;
    }

     /* set io flags */
     DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;

     /* clear initializing flag */
     DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

     NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
     KsSetDevicePnpAndBaseObject(DeviceExtension->KsDeviceHeader, NextDeviceObject, DeviceObject);

    DPRINT("KMix_InstallDevice result %x\n", Status);
    return STATUS_SUCCESS;

cleanup:

    IoDeleteDevice(DeviceObject);
    return Status;
}


NTSTATUS
NTAPI
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPathName)
{
    DPRINT1("KMixer.sys loaded\n");

    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);

    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->MajorFunction[IRP_MJ_PNP] = KMix_Pnp;
    DriverObject->DriverUnload = KMix_Unload;
    DriverObject->DriverExtension->AddDevice = KMix_AddDevice;

    return STATUS_SUCCESS;
}
