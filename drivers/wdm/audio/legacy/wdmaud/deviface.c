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

const GUID GUID_DEVICE_INTERFACE_ARRIVAL       = {0xCB3A4004L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID GUID_DEVICE_INTERFACE_REMOVAL       = {0xCB3A4005L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};

BOOLEAN
IsEntryAlreadyInList(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING NewSymbolicLink)
{
    PLIST_ENTRY Entry;
    PSYSAUDIO_ENTRY DeviceEntry;

    Entry = DeviceExtension->SysAudioDeviceList.Flink;
    while (Entry != &DeviceExtension->SysAudioDeviceList)
    {
        DeviceEntry = CONTAINING_RECORD(Entry, SYSAUDIO_ENTRY, Entry);

        if (RtlEqualUnicodeString(&DeviceEntry->SymbolicLink, NewSymbolicLink, TRUE))
        {
            return TRUE;
        }
        Entry = Entry->Flink;
    }
    return FALSE;
}

NTSTATUS
WdmAudOpenSysAudioDevice(
    IN PUNICODE_STRING DeviceName,
    OUT PHANDLE Handle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes, DeviceName, OBJ_OPENIF | OBJ_KERNEL_HANDLE, NULL, NULL);

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
    PWDMAUD_DEVICE_EXTENSION DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)Context;
    SYSAUDIO_ENTRY * Entry;
    NTSTATUS Status;

    DPRINT("DeviceInterfaceChangeCallback called %p\n", Event);

    if (IsEqualGUIDAligned(&Event->Event, &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        if (!IsEntryAlreadyInList(DeviceExtension, Event->SymbolicLinkName))
        {
            Entry = (SYSAUDIO_ENTRY*)AllocateItem(NonPagedPool, sizeof(SYSAUDIO_ENTRY) + Event->SymbolicLinkName->Length);
            if (!Entry)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlInitEmptyUnicodeString(&Entry->SymbolicLink, (PWCHAR)(Entry + 1), Event->SymbolicLinkName->Length);
            RtlCopyUnicodeString(&Entry->SymbolicLink, Event->SymbolicLinkName);

            Status = WdmAudOpenSysAudioDevice(&Entry->SymbolicLink, &Entry->hSysAudio);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to open sysaudio %x\n", Status);
                FreeItem(Entry);
                return Status;
            }

            /* get the file object */
            Status = ObReferenceObjectByHandle(Entry->hSysAudio, FILE_READ_DATA | FILE_WRITE_DATA, *IoFileObjectType, KernelMode, (PVOID*)&Entry->FileObject, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to reference FileObject %x\n", Status);
                ZwClose(Entry->hSysAudio);
                FreeItem(Entry);
                return Status;
            }

            InsertTailList(&DeviceExtension->SysAudioDeviceList, &Entry->Entry);
            DeviceExtension->NumSysAudioDevices++;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
WdmAudOpenSysAudioDeviceInterfaces(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN LPWSTR SymbolicLinkList)
{
    SYSAUDIO_ENTRY * Entry;
    NTSTATUS Status;
    UNICODE_STRING NewEntry;

    while(*SymbolicLinkList)
    {
        RtlInitUnicodeString(&NewEntry, SymbolicLinkList);
        if (!IsEntryAlreadyInList(DeviceExtension, &NewEntry))
        {
            Entry = (SYSAUDIO_ENTRY*)AllocateItem(NonPagedPool, sizeof(SYSAUDIO_ENTRY) + NewEntry.Length);
            if (!Entry)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            RtlInitEmptyUnicodeString(&Entry->SymbolicLink, (PWCHAR)(Entry + 1), NewEntry.Length);
            RtlCopyUnicodeString(&Entry->SymbolicLink, &NewEntry);

            Status = WdmAudOpenSysAudioDevice(&Entry->SymbolicLink, &Entry->hSysAudio);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to open sysaudio %x\n", Status);
                FreeItem(Entry);
                return Status;
            }

            /* get the file object */
            Status = ObReferenceObjectByHandle(Entry->hSysAudio, FILE_READ_DATA | FILE_WRITE_DATA, *IoFileObjectType, KernelMode, (PVOID*)&Entry->FileObject, NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to reference FileObject %x\n", Status);
                ZwClose(Entry->hSysAudio);
                FreeItem(Entry);
                return Status;
            }

            InsertTailList(&DeviceExtension->SysAudioDeviceList, &Entry->Entry);
            DeviceExtension->NumSysAudioDevices++;
        }
        SymbolicLinkList += wcslen(SymbolicLinkList) + 1;
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
        DeviceExtension->DeviceInterfaceSupport = TRUE;
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
