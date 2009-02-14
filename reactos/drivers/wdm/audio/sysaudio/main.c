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

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#define YDEBUG
#include <debug.h>
//#include <dxsdk/mediaobj.h>
#include "sysaudio.h"

const GUID DMOCATEGORY_ACOUSTIC_ECHO_CANCEL    = {0xBF963D80L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSCATEGORY_SYSAUDIO                 = {0xA7C7A5B1L, 0x5AF3, 0x11D1, {0x9C, 0xED, 0x00, 0xA0, 0x24, 0xBF, 0x04, 0x07}};
const GUID KSCATEGORY_AUDIO_DEVICE             = {0xFBF6F530L, 0x07B9, 0x11D2, {0xA7, 0x1E, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};
const GUID KSCATEGORY_PREFERRED_WAVEOUT_DEVICE = {0xD6C5066EL, 0x72C1, 0x11D2, {0x97, 0x55, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};
const GUID KSCATEGORY_PREFERRED_WAVEIN_DEVICE  = {0xD6C50671L, 0x72C1, 0x11D2, {0x97, 0x55, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};
const GUID KSCATEGORY_PREFERRED_MIDIOUT_DEVICE = {0xD6C50674L, 0x72C1, 0x11D2, {0x97, 0x55, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}};
const GUID KS_CATEGORY_AUDIO                   = {0x6994AD04L, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID GUID_DEVICE_INTERFACE_ARRIVAL       = {0xCB3A4004L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID GUID_DEVICE_INTERFACE_REMOVAL       = {0xCB3A4005L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};


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
	DbgBreakPoint();

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
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event;
    SYSAUDIODEVEXT *DeviceExtension = (SYSAUDIODEVEXT*)Context;
    NTSTATUS Status = STATUS_SUCCESS;

    Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    if (IsEqualGUIDAligned(&Event->Event,
                           &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        /* a new device has arrived */

        PFILE_OBJECT FileObject = NULL;
        PKSAUDIO_DEVICE_ENTRY DeviceEntry;
        HANDLE NodeHandle;
        IO_STATUS_BLOCK IoStatusBlock;
        OBJECT_ATTRIBUTES ObjectAttributes;

        DeviceEntry = ExAllocatePool(NonPagedPool, sizeof(KSAUDIO_DEVICE_ENTRY));
        if (!DeviceEntry)
            return STATUS_INSUFFICIENT_RESOURCES;

        DeviceEntry->DeviceName.Length = 0;
        DeviceEntry->DeviceName.MaximumLength = Event->SymbolicLinkName->Length + 5 * sizeof(WCHAR);
        DeviceEntry->DeviceName.Buffer = ExAllocatePool(NonPagedPool, DeviceEntry->DeviceName.MaximumLength);
        if (!DeviceEntry->DeviceName.Buffer)
        {
            ExFreePool(DeviceEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(RtlAppendUnicodeToString(&DeviceEntry->DeviceName, L"\\??\\")))
        {

            ExFreePool(DeviceEntry->DeviceName.Buffer);
            ExFreePool(DeviceEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&DeviceEntry->DeviceName, Event->SymbolicLinkName)))
        {

            ExFreePool(DeviceEntry->DeviceName.Buffer);
            ExFreePool(DeviceEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DPRINT1("Sym %wZ\n", &DeviceEntry->DeviceName);

        InitializeObjectAttributes(&ObjectAttributes, &DeviceEntry->DeviceName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

        Status = ZwCreateFile(&NodeHandle,
                              GENERIC_READ | GENERIC_WRITE,
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
            DPRINT1("ZwCreateFile failed with %x\n", Status);
            ExFreePool(DeviceEntry);
            return Status;
        }

        Status = ObReferenceObjectByHandle(NodeHandle, GENERIC_READ | GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
        if (!NT_SUCCESS(Status))
        {
            ZwClose(NodeHandle);
            ExFreePool(DeviceEntry);
            DPRINT1("ObReferenceObjectByHandle failed with %x\n", Status);
            return Status;
        }

        DeviceEntry->Handle = NodeHandle;
        DeviceEntry->FileObject = FileObject;

        InsertTailList(&DeviceExtension->KsAudioDeviceList, &DeviceEntry->Entry);
        DeviceExtension->NumberOfKsAudioDevices++;

        DPRINT1("Successfully opened audio device handle %p file object %p device object %p\n", NodeHandle, FileObject, FileObject->DeviceObject);
        return Status;
    }
    else if (IsEqualGUIDAligned(&Event->Event,
                                &GUID_DEVICE_INTERFACE_REMOVAL))
    {
        DPRINT1("Remove interface to audio device!\n");
        ///FIXME
        ///
        return STATUS_SUCCESS;
    }
    else
    {
        UNICODE_STRING EventName, InterfaceGuid;

        RtlStringFromGUID(&Event->Event, &EventName);
        RtlStringFromGUID(&Event->InterfaceClassGuid, &InterfaceGuid);
        DPRINT1("Unknown event: Event %wZ GUID %wZ\n", &EventName, &InterfaceGuid);
        return STATUS_SUCCESS;
    }


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
SysAudio_InstallDevice(
    IN  PDRIVER_OBJECT  DriverObject)
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\sysaudio");
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\sysaudio");
    PDEVICE_OBJECT DeviceObject;
    //PDEVICE_OBJECT NextDeviceObject;
    KSOBJECT_CREATE_ITEM CreateItem;
    SYSAUDIODEVEXT *DeviceExtension;
    UNICODE_STRING SymbolicLink;

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
    //DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    InitializeListHead(&DeviceExtension->KsAudioDeviceList);

    /* initialize create item struct */
    RtlZeroMemory(&CreateItem, sizeof(KSOBJECT_CREATE_ITEM));
    CreateItem.Create = DispatchCreate;

    Status = KsAllocateDeviceHeader(&DeviceExtension->KsDeviceHeader,
                                    1,
                                    &CreateItem);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsAllocateDeviceHeader failed with %x\n", Status);
        goto cleanup;
    }

#if 0
    //NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    /// FIXME
    /// KsSetDevicePnpAndBaseObject(DeviceExtension->KsDeviceHeader, NextDeviceObject, DeviceObject);
    ///
    /// DeviceExtension->NextDeviceObject = NextDeviceObject;
#endif

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
        goto cleanup;
    }

#if 0
    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&DMOCATEGORY_ACOUSTIC_ECHO_CANCEL,
                                            DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceExtension,
                                            (PVOID*)&DeviceExtension->EchoCancelNotificationEntry);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterPlugPlayNotification failed with %x\n", Status);
        goto cleanup;
    }
#endif

     Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_MIDIOUT_DEVICE, NULL, &SymbolicLink);
     if (NT_SUCCESS(Status))
     {
         IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
         RtlFreeUnicodeString(&SymbolicLink);
     }
     else
     {
         DPRINT1("Failed to register KSCATEGORY_PREFERRED_MIDIOUT_DEVICE interface Status %x\n", Status);
     }

     Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEIN_DEVICE, NULL, &SymbolicLink);
     if (NT_SUCCESS(Status))
     {
         IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
         RtlFreeUnicodeString(&SymbolicLink);
     }
     else
     {
         DPRINT1("Failed to register KSCATEGORY_PREFERRED_WAVEIN_DEVICE interface Status %x\n", Status);
     }

     Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEOUT_DEVICE, NULL, &SymbolicLink);
     if (NT_SUCCESS(Status))
     {
         IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
         RtlFreeUnicodeString(&SymbolicLink);
     }
     else
     {
         DPRINT1("Failed to register KSCATEGORY_PREFERRED_WAVEOUT_DEVICE interface Status %x\n", Status);
     }

     Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_SYSAUDIO, NULL, &SymbolicLink);
     if (NT_SUCCESS(Status))
     {
         IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
         RtlFreeUnicodeString(&SymbolicLink);
     }
     else
     {
         DPRINT1("Failed to register KSCATEGORY_SYSAUDIO interface Status %x\n", Status);
     }


     /* set io flags */
     DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
     /* clear initializing flag */
     DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    DPRINT("Device SysAudio_AddDevice result %x\n", Status);
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
SysAudio_Stub(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT1("SysAudio_Stub called\n");

    /* TODO */

	Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    DPRINT1("System audio graph builder (sysaudio) started\n");

    DPRINT1("Setting KS function handlers\n");

#if KS_IMPLEMENTED
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_WRITE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
#else
    DriverObject->MajorFunction[IRP_MJ_CREATE] = SysAudio_Stub;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SysAudio_Stub;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = SysAudio_Stub;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SysAudio_Stub;
#endif


    DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->MajorFunction[IRP_MJ_PNP] = SysAudio_Pnp;
    DriverObject->DriverUnload = SysAudio_Unload;

    return SysAudio_InstallDevice(DriverObject);
}
