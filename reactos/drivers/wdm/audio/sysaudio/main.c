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

    /* Get current irp stack */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    /* Fetch the device extension */
    DeviceExtension = (SYSAUDIODEVEXT*)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension);

    if (IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        /* Unregister the echo cancel hook */
        if (DeviceExtension->EchoCancelNotificationEntry)
            IoUnregisterPlugPlayNotification(DeviceExtension->EchoCancelNotificationEntry);

        /* Unregister the ks audio hook */
        if (DeviceExtension->KsAudioNotificationEntry)
            IoUnregisterPlugPlayNotification(DeviceExtension->KsAudioNotificationEntry);

        /* Destroy our symbolic link */
        IoDeleteSymbolicLink(&SymlinkName);
    }
    else if (IrpStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
    {
        /* Sysaudio can not be disabled */
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    /* Perform default pnp actions */
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
    SYSAUDIODEVEXT *DeviceExtension;


    DPRINT("SysAudio_InstallDevice called\n");

    /* Create the device */
    Status = IoCreateDevice(DriverObject,
                            sizeof(SYSAUDIODEVEXT),
                            &DeviceName,
                            FILE_DEVICE_KS,
                            0,
                            FALSE,
                            &DeviceObject);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to create \\Device\\sysaudio !\n");
        return Status;
    }

    /* Register device interfaces */
    Status = SysAudioRegisterDeviceInterfaces(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to register
         * Create a hack interface
         */
        Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
        if (!NT_SUCCESS(Status))
        {
            IoDeleteDevice(DeviceObject);
            DPRINT("Failed to create sysaudio symlink!\n");
            return Status;
        }
    }
    /* Acquire device extension */
    DeviceExtension = (SYSAUDIODEVEXT*)DeviceObject->DeviceExtension;
    /* Initialize device extension */
    RtlZeroMemory(DeviceExtension, sizeof(SYSAUDIODEVEXT));

    /* Initialize the mutex */
    KeInitializeSpinLock(&DeviceExtension->Lock);

    /* Initialize the ks audio device list */
    InitializeListHead(&DeviceExtension->KsAudioDeviceList);

    /* Allocate kernel streaming device header */
    Status = SysAudioAllocateDeviceHeader(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsAllocateDeviceHeader failed with %x\n", Status);
        goto cleanup;
    }

    /* Register device notification hooks */
    Status = SysAudioRegisterNotifications(DriverObject,
                                           DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register device notifications\n");
        goto cleanup;
    }

    /* Load kmixer */
    Status = SysAudioOpenKMixer(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SysAudioOpenKMixer failed with %x\n", Status);
        goto cleanup;
    }

     /* set io flags */
     DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
     /* clear initializing flag */
     DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    /* Done */
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

NTSTATUS
NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    DPRINT("System audio graph builder (sysaudio) started\n");

    /* Let ks handle these */
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);

    /* Let ks handle these */
    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;

    /* Use provided ks unload function */
    DriverObject->DriverUnload = KsNullDriverUnload;

    /* Sysaudio needs to do work on pnp, so handle it */
    DriverObject->MajorFunction[IRP_MJ_PNP] = SysAudio_Pnp;

    /* Call our initialization function */
    return SysAudio_InstallDevice(DriverObject);
}
