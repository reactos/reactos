/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include "sysaudio.h"

#define NDEBUG
#include <debug.h>

const GUID GUID_DEVICE_INTERFACE_ARRIVAL       = {0xCB3A4004L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID GUID_DEVICE_INTERFACE_REMOVAL       = {0xCB3A4005L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID KS_CATEGORY_AUDIO                   = {0x6994AD04L, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KS_CATEGORY_TOPOLOGY                = {0xDDA54A40, 0x1E4C, 0x11D1, {0xA0, 0x50, 0x40, 0x57, 0x05, 0xC1, 0x00, 0x00}};
const GUID DMOCATEGORY_ACOUSTIC_ECHO_CANCEL    = {0xBF963D80L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};

NTSTATUS
OpenDevice(
    IN PUNICODE_STRING DeviceName,
    IN PHANDLE HandleOut,
    IN PFILE_OBJECT * FileObjectOut)
{
    NTSTATUS Status;
    HANDLE NodeHandle;
    PFILE_OBJECT FileObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    InitializeObjectAttributes(&ObjectAttributes, DeviceName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwCreateFile(&NodeHandle,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          0,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);


    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateFile failed with %x %S\n", Status, DeviceName->Buffer);
        return Status;
    }

    Status = ObReferenceObjectByHandle(NodeHandle, GENERIC_READ | GENERIC_WRITE, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(NodeHandle);
        DPRINT("ObReferenceObjectByHandle failed with %x\n", Status);
        return Status;
    }

    *HandleOut = NodeHandle;
    *FileObjectOut = FileObject;
    return Status;
}

NTSTATUS
InsertAudioDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DeviceName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSAUDIO_DEVICE_ENTRY DeviceEntry = NULL;

    /* a new device has arrived */
    DeviceEntry = AllocateItem(NonPagedPool, sizeof(KSAUDIO_DEVICE_ENTRY));
    if (!DeviceEntry)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize audio device entry */
    RtlZeroMemory(DeviceEntry, sizeof(KSAUDIO_DEVICE_ENTRY));

    /* set device name */
    DeviceEntry->DeviceName.Length = 0;
    DeviceEntry->DeviceName.MaximumLength = DeviceName->MaximumLength + 10 * sizeof(WCHAR);

    DeviceEntry->DeviceName.Buffer = AllocateItem(NonPagedPool, DeviceEntry->DeviceName.MaximumLength);

    if (!DeviceEntry->DeviceName.Buffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    /* open device */
    Status = OpenDevice(DeviceName, &DeviceEntry->Handle, &DeviceEntry->FileObject);
    if (NT_SUCCESS(Status))
    {
        /* copy device name */
        RtlAppendUnicodeStringToString(&DeviceEntry->DeviceName, DeviceName);
    }
    else
    {
        /* the device name needs to be prefixed */
        RtlAppendUnicodeToString(&DeviceEntry->DeviceName, L"\\??\\");
        RtlAppendUnicodeStringToString(&DeviceEntry->DeviceName, DeviceName);

        /* open device */
        Status = OpenDevice(&DeviceEntry->DeviceName, &DeviceEntry->Handle, &DeviceEntry->FileObject);
    }

    if (!NT_SUCCESS(Status))
    {
        goto cleanup;
    }

    /* fetch device extension */
    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;
    /* insert new audio device */
    ExInterlockedInsertTailList(&DeviceExtension->KsAudioDeviceList, &DeviceEntry->Entry, &DeviceExtension->Lock);
    InterlockedIncrement((PLONG)&DeviceExtension->NumberOfKsAudioDevices);

    DPRINT("Successfully opened audio device %u Device %S\n", DeviceExtension->NumberOfKsAudioDevices, DeviceEntry->DeviceName.Buffer);
    return Status;

cleanup:
    if (DeviceEntry)
    {
        if (DeviceEntry->DeviceName.Buffer)
            FreeItem(DeviceEntry->DeviceName.Buffer);

        FreeItem(DeviceEntry);
    }

    return Status;

}


NTSTATUS
NTAPI
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;

    Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    if (IsEqualGUIDAligned(&Event->Event,
                           &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        Status = InsertAudioDevice(DeviceObject, Event->SymbolicLinkName);
        return Status;
    }
    else
    {
        DPRINT("Remove interface to audio device!\n");
        UNIMPLEMENTED;
        return STATUS_SUCCESS;
    }


}

NTSTATUS
SysAudioRegisterNotifications(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PSYSAUDIODEVEXT DeviceExtension;

    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&KS_CATEGORY_AUDIO,
                                            DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceObject,
                                            (PVOID*)&DeviceExtension->KsAudioNotificationEntry);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoRegisterPlugPlayNotification failed with %x\n", Status);
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&DMOCATEGORY_ACOUSTIC_ECHO_CANCEL,
                                            DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceObject,
                                            (PVOID*)&DeviceExtension->EchoCancelNotificationEntry);

    if (!NT_SUCCESS(Status))
    {
        /* ignore failure for now */
        DPRINT("IoRegisterPlugPlayNotification failed for DMOCATEGORY_ACOUSTIC_ECHO_CANCEL\n", Status);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
SysAudioRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    UNICODE_STRING SymbolicLink;

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_MIDIOUT_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_PREFERRED_MIDIOUT_DEVICE interface Status %x\n", Status);
        return Status;
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEIN_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_PREFERRED_WAVEIN_DEVICE interface Status %x\n", Status);
        return Status;
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEOUT_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_PREFERRED_WAVEOUT_DEVICE interface Status %x\n", Status);
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_SYSAUDIO, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_SYSAUDIO interface Status %x\n", Status);
    }

    return Status;
}
