/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */

#include "wdmaud.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
WdmAudOpenSysAudioDevice(
    IN LPWSTR DeviceName,
    OUT PHANDLE Handle)
{
    UNICODE_STRING SymbolicLink;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    RtlInitUnicodeString(&SymbolicLink, DeviceName);
    InitializeObjectAttributes(&ObjectAttributes, &SymbolicLink, OBJ_OPENIF | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = IoCreateFile(Handle,
                          SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          0,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_NO_PARAMETER_CHECKING | IO_FORCE_ACCESS_CHECK);

    return Status;
}

NTSTATUS
NTAPI
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    DPRINT1("DeviceInterfaceChangeCallback called %p\n", Event);
    DbgBreakPoint();
    return STATUS_SUCCESS;
}

NTSTATUS
WdmAudOpenSysAudioDeviceInterfaces(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN LPWSTR SymbolicLinkList)
{
    SYSAUDIO_ENTRY * Entry;
    ULONG Length;

    DPRINT1("WdmAudOpenSysAudioDeviceInterfaces called\n");

    while(*SymbolicLinkList)
    {
        Length = wcslen(SymbolicLinkList) + 1;
        Entry = (SYSAUDIO_ENTRY*)AllocateItem(NonPagedPool, sizeof(SYSAUDIO_ENTRY) + Length * sizeof(WCHAR));
        if (!Entry)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Entry->SymbolicLink.Length = Length * sizeof(WCHAR);
        Entry->SymbolicLink.MaximumLength = Length * sizeof(WCHAR);
        Entry->SymbolicLink.Buffer = (LPWSTR) (Entry + 1);
        wcscpy(Entry->SymbolicLink.Buffer, SymbolicLinkList);

        InsertTailList(&DeviceExtension->SysAudioDeviceList, &Entry->Entry);

        DeviceExtension->NumSysAudioDevices++;
        SymbolicLinkList += Length;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
WdmAudOpenSysAudioDevices(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPWSTR SymbolicLinkList;
    SYSAUDIO_ENTRY * Entry;
    ULONG Length;
    HANDLE hSysAudio;
    PFILE_OBJECT FileObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\sysaudio\\GLOBAL");

    if (DeviceExtension->DeviceInterfaceSupport)
    {
        Status = IoGetDeviceInterfaces(&KSCATEGORY_SYSAUDIO,
                                       NULL,
                                       0,
                                       &SymbolicLinkList);

        if (NT_SUCCESS(Status))
        {
            WdmAudOpenSysAudioDeviceInterfaces(DeviceExtension, SymbolicLinkList);
            FreeItem(SymbolicLinkList);
        }


        Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                                PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                               (PVOID)&KSCATEGORY_SYSAUDIO,
                                                DeviceObject->DriverObject,
                                                DeviceInterfaceChangeCallback,
                                               (PVOID)DeviceExtension,
                                               &DeviceExtension->SysAudioNotification);
    }
    else
    {
            Entry = (SYSAUDIO_ENTRY*)AllocateItem(NonPagedPool, sizeof(SYSAUDIO_ENTRY));
            if (!Entry)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }


            Length = wcslen(DeviceName.Buffer) + 1;
            Entry->SymbolicLink.Length = 0;
            Entry->SymbolicLink.MaximumLength = Length * sizeof(WCHAR);
            Entry->SymbolicLink.Buffer = AllocateItem(NonPagedPool, Entry->SymbolicLink.MaximumLength);

            if (!Entry->SymbolicLink.Buffer)
            {
                FreeItem(Entry);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Status = RtlAppendUnicodeStringToString(&Entry->SymbolicLink, &DeviceName);

            if (!NT_SUCCESS(Status))
            {
                FreeItem(Entry->SymbolicLink.Buffer);
                FreeItem(Entry);
                return Status;
            }

            DPRINT("Opening device %S\n", Entry->SymbolicLink.Buffer);
            Status = WdmAudOpenSysAudioDevice(Entry->SymbolicLink.Buffer, &hSysAudio);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to open sysaudio %x\n", Status);
                FreeItem(Entry->SymbolicLink.Buffer);
                FreeItem(Entry);
                return Status;
            }

            InsertTailList(&DeviceExtension->SysAudioDeviceList, &Entry->Entry);
            DeviceExtension->NumSysAudioDevices++;

            /* get the file object */
            Status = ObReferenceObjectByHandle(hSysAudio, FILE_READ_DATA | FILE_WRITE_DATA, *IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to reference FileObject %x\n", Status);
                ZwClose(hSysAudio);
                return Status;
            }
            DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
            DeviceExtension->hSysAudio = hSysAudio;
            DeviceExtension->FileObject = FileObject;
    }

    return Status;
}

NTSTATUS
WdmAudRegisterDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    UNICODE_STRING SymbolicLinkName;

    Status = IoRegisterDeviceInterface(PhysicalDeviceObject, &KSCATEGORY_WDMAUD, NULL, &SymbolicLinkName);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
        RtlFreeUnicodeString(&SymbolicLinkName);
        //DeviceExtension->DeviceInterfaceSupport = TRUE;
        return Status;
    }

    return Status;
}

NTSTATUS
WdmAudOpenSysaudio(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_CLIENT *pClient)
{
    PWDMAUD_CLIENT Client;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (!DeviceExtension->NumSysAudioDevices)
    {
        /* wdmaud failed to open sysaudio */
        return STATUS_UNSUCCESSFUL;
    }

    /* sanity check */
    ASSERT(!IsListEmpty(&DeviceExtension->SysAudioDeviceList));

    /* allocate client context struct */
    Client = AllocateItem(NonPagedPool, sizeof(WDMAUD_CLIENT));

    /* check for allocation failure */
    if (!Client)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* zero client context struct */
    RtlZeroMemory(Client, sizeof(WDMAUD_CLIENT));

    /* initialize mixer event list */
    InitializeListHead(&Client->MixerEventList);

    /* store result */
    *pClient = Client;

    /* insert client into list */
    ExInterlockedInsertTailList(&DeviceExtension->WdmAudClientList, &Client->Entry, &DeviceExtension->Lock);

    /* done */
    return STATUS_SUCCESS;
}
