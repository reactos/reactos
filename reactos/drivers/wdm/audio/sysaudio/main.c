/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/main.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 * HISTORY:
 *                  8 Jul 07    Started basic implementation
 */

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#define YDEBUG
#include <debug.h>
//#include <dxsdk/mediaobj.h>
#include "sysaudio.h"


const GUID KSCATEGORY_SYSAUDIO                 = {0xA7C7A5B1L, 0x5AF3, 0x11D1, {0x9C, 0xED, 0x00, 0xA0, 0x24, 0xBF, 0x04, 0x07}};
const GUID KSCATEGORY_AUDIO_DEVICE             = {0xFBF6F530L, 0x07B9, 0x11D2, {0xA7, 0x1E, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};
const GUID KSCATEGORY_PREFERRED_WAVEOUT_DEVICE = {0xD6C5066EL, 0x72C1, 0x11D2, {0x97, 0x55, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};
const GUID KSCATEGORY_PREFERRED_WAVEIN_DEVICE  = {0xD6C50671L, 0x72C1, 0x11D2, {0x97, 0x55, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};
const GUID KSCATEGORY_PREFERRED_MIDIOUT_DEVICE = {0xD6C50674L, 0x72C1, 0x11D2, {0x97, 0x55, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};



VOID
NTAPI
SysAudio_Unload(IN PDRIVER_OBJECT DriverObject)
{
    DPRINT1("SysAudio_Unload called\n");
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

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT1("SysAudio_Pnp called for func %x\n", IrpStack->MinorFunction);

    DeviceExtension = (SYSAUDIODEVEXT*)DeviceObject->DeviceExtension;


    if (IrpStack->MinorFunction == IRP_MN_START_DEVICE)
    {
    DPRINT1("SysAudio_Pnp called for func IRP_MN_START_DEVICE\n");
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
	}

    if (IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        if (DeviceExtension->EchoCancelNotificationEntry)
            IoUnregisterPlugPlayNotification(DeviceExtension->EchoCancelNotificationEntry);

        if (DeviceExtension->KsAudioNotificationEntry)
            IoUnregisterPlugPlayNotification(DeviceExtension->KsAudioNotificationEntry);

        IoDeleteSymbolicLink(&SymlinkName);
    }
    else if (IrpStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    return KsDefaultDispatchPnp(DeviceObject, Irp);
}

NTSTATUS
NTAPI
SysAudio_InstallDevice(
    IN  PDRIVER_OBJECT  DriverObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\sysaudio");
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\sysaudio");
    PDEVICE_OBJECT DeviceObject;
    //PDEVICE_OBJECT NextDeviceObject;
    SYSAUDIODEVEXT *DeviceExtension;


    DPRINT1("SysAudio_InstallDevice called\n");

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

    /* register device interfaces */
    Status = SysAudioRegisterDeviceInterfaces(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed to register
         * create a hack interface
         */
        Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
        if (!NT_SUCCESS(Status))
        {
            IoDeleteDevice(DeviceObject);
            DPRINT("Failed to create sysaudio symlink!\n");
            return Status;
        }
    }

    DeviceExtension = (SYSAUDIODEVEXT*)DeviceObject->DeviceExtension;
    /* initialize device extension */
    RtlZeroMemory(DeviceExtension, sizeof(SYSAUDIODEVEXT));

    KeInitializeMutex(&DeviceExtension->Mutex, 0);
    //DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    InitializeListHead(&DeviceExtension->KsAudioDeviceList);

    Status = SysAudioAllocateDeviceHeader(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsAllocateDeviceHeader failed with %x\n", Status);
        goto cleanup;
    }

    Status = SysAudioRegisterNotifications(DriverObject,
                                         DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register device notifications\n");
        goto cleanup;
    }

     /* set io flags */
     DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
     /* clear initializing flag */
     DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    DPRINT("Device SysAudio_InstallDevice result %x\n", Status);
    return STATUS_SUCCESS;

cleanup:

    if (DeviceExtension->KsAudioNotificationEntry)
        IoUnregisterPlugPlayNotification(DeviceExtension->KsAudioNotificationEntry);

    if (DeviceExtension->EchoCancelNotificationEntry)
        IoUnregisterPlugPlayNotification(DeviceExtension->EchoCancelNotificationEntry);

    IoDeleteSymbolicLink(&SymlinkName);
    IoDeleteDevice(DeviceObject);
    return Status;
}

NTSTATUS NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    DPRINT1("System audio graph builder (sysaudio) started\n");

    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);

    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->MajorFunction[IRP_MJ_PNP] = SysAudio_Pnp;
    DriverObject->DriverUnload = SysAudio_Unload;

    return SysAudio_InstallDevice(DriverObject);
}
