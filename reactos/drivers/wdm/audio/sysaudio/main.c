/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/main.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  8 Jul 07    Started basic implementation
 */

#include <ntddk.h>
#include <ks.h>
#include <debug.h>
//#include <dxsdk/mediaobj.h>
#include "sysaudio.h"

const GUID KS_CATEGORY_AUDIO    = {0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};

VOID
NTAPI
SysAudio_Unload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT("SysAudio_Unload called\n");
}

NTSTATUS
NTAPI
SysAudio_Pnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\sysaudio");
    SYSAUDIODEVEXT *DeviceExtension;

    DPRINT1("SysAudio_Pnp called\n");


    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    DeviceExtension = (SYSAUDIODEVEXT*)DeviceObject->DeviceExtension;

    if (IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        if (DeviceExtension->EchoCancelNotificationEntry)
            IoUnregisterPlugPlayNotification(DeviceExtension->EchoCancelNotificationEntry);

        if (DeviceExtension->KsAudioNotificationEntry)
            IoUnregisterPlugPlayNotification(DeviceExtension->KsAudioNotificationEntry);

        IoDeleteSymbolicLink(&SymlinkName);
    }

    return KsDefaultDispatchPnp(DeviceObject, Irp);
}

NTSTATUS
NTAPI
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    DPRINT1("DeviceInterfaceChangeCallback called %p\n", Event);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DispatchCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT1("DispatchCreate\n");

    return STATUS_SUCCESS;
}
NTSTATUS
NTAPI
SysAudio_AddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\sysaudio");
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\sysaudio");
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT NextDeviceObject;
    KSOBJECT_CREATE_ITEM CreateItem;
    SYSAUDIODEVEXT *DeviceExtension;

    DPRINT("SysAudio_AddDevice called\n");

    /* create the device */
    Status = IoCreateDevice(DriverObject,
                            sizeof(SYSAUDIODEVEXT),
                            &DeviceName,
                            FILE_DEVICE_KS,
                            0,
                            FALSE,
                            &DeviceObject);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to create \\Device\\sysaudio !\n");
        return Status;
    }

    /* create the symbolic link */
    Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        DPRINT("Failed to create \\DosDevices\\sysaudio symlink!\n");
        return Status;
    }


    DeviceExtension = (SYSAUDIODEVEXT*)DeviceObject->DeviceExtension;
    /* initialize device extension */
    RtlZeroMemory(DeviceExtension, sizeof(SYSAUDIODEVEXT));

    KeInitializeMutex(&DeviceExtension->Mutex, 0);

    DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    /* initialize create item struct */
    RtlZeroMemory(&CreateItem, sizeof(KSOBJECT_CREATE_ITEM));
    CreateItem.Create = DispatchCreate;

    Status = KsAllocateDeviceHeader(&DeviceExtension->KsDeviceHeader,
                                    1,
                                    &CreateItem);

    if (NT_SUCCESS(Status))
    {
        NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
        if (NextDeviceObject)
        {
            /// FIXME
            /// KsSetDevicePnpAndBaseObject(DeviceExtension->KsDeviceHeader, NextDeviceObject, DeviceObject);
            ///

            DeviceExtension->NextDeviceObject = NextDeviceObject;

            Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                                    PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                                    (PVOID)&KS_CATEGORY_AUDIO,
                                                    DriverObject,
                                                    DeviceInterfaceChangeCallback,
                                                    (PVOID)DeviceExtension,
                                                    (PVOID*)&DeviceExtension->KsAudioNotificationEntry);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IoRegisterPlugPlayNotification failed with %x\n", Status);
                return Status;
            }
#if 0
            Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                                    PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                                    &DMOCATEGORY_ACOUSTIC_ECHO_CANCEL,
                                                    DriverObject,
                                                    DeviceInterfaceChangeCallback,
                                                    (PVOID)DeviceExtension,
                                                    &DeviceExtension->EchoCancelNotificationEntry);
#endif
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IoRegisterPlugPlayNotification failed with %x\n", Status);
                return Status;
            }

            /* set io flags */
            DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
            /* clear initializing flag */
            DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;
        }
    }



    DPRINT("Device SysAudio_AddDevice result %x\n", Status);

    return Status;
}

NTSTATUS NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    DPRINT("System audio graph builder (sysaudio) started\n");



    DPRINT1("Setting KS function handlers\n");
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);

    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->MajorFunction[IRP_MJ_PNP] = SysAudio_Pnp;
    DriverObject->DriverUnload = SysAudio_Unload;
    DriverObject->DriverExtension->AddDevice = SysAudio_AddDevice;

    return STATUS_SUCCESS;
}
