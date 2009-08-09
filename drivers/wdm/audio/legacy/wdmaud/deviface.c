/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

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
        Entry = (SYSAUDIO_ENTRY*)ExAllocatePool(NonPagedPool, sizeof(SYSAUDIO_ENTRY) + Length * sizeof(WCHAR));
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
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\sysaudio");

    if (DeviceExtension->DeviceInterfaceSupport)
    {
        Status = IoGetDeviceInterfaces(&KSCATEGORY_SYSAUDIO,
                                       NULL,
                                       0,
                                       &SymbolicLinkList);

        if (NT_SUCCESS(Status))
        {
            WdmAudOpenSysAudioDeviceInterfaces(DeviceExtension, SymbolicLinkList);
            ExFreePool(SymbolicLinkList);
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
            Length = wcslen(DeviceName.Buffer) + 1;
            Entry = (SYSAUDIO_ENTRY*)ExAllocatePool(NonPagedPool, sizeof(SYSAUDIO_ENTRY) + Length * sizeof(WCHAR));
            if (!Entry)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Entry->SymbolicLink.Length = Entry->SymbolicLink.MaximumLength = Length * sizeof(WCHAR);
            Entry->SymbolicLink.MaximumLength += sizeof(WCHAR);
            Entry->SymbolicLink.Buffer = (LPWSTR) (Entry + 1);

            wcscpy(Entry->SymbolicLink.Buffer, DeviceName.Buffer);

            InsertTailList(&DeviceExtension->SysAudioDeviceList, &Entry->Entry);
            DeviceExtension->NumSysAudioDevices++;
    }
    return Status;
}

NTSTATUS
WdmAudRegisterDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\wdmaud");
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\wdmaud");
    UNICODE_STRING SymbolicLinkName;

    Status = IoRegisterDeviceInterface(PhysicalDeviceObject, &KSCATEGORY_WDMAUD, NULL, &SymbolicLinkName);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
        RtlFreeUnicodeString(&SymbolicLinkName);
        DeviceExtension->DeviceInterfaceSupport = TRUE;
        return Status;
    }

    /* failed to register device interface
     * create a symbolic link instead 
     */
    DeviceExtension->DeviceInterfaceSupport = FALSE;

    Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(PhysicalDeviceObject); //FIXME
        DPRINT("Failed to create wdmaud symlink!\n");
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
    NTSTATUS Status;
    HANDLE hSysAudio;
    PSYSAUDIO_ENTRY SysEntry;
    PFILE_OBJECT FileObject;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (!DeviceExtension->NumSysAudioDevices)
        return STATUS_UNSUCCESSFUL;

    ASSERT(!IsListEmpty(&DeviceExtension->SysAudioDeviceList));

    Client = ExAllocatePool(NonPagedPool, sizeof(WDMAUD_CLIENT));
    if (!Client)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Client, sizeof(WDMAUD_CLIENT));


    /* open the first sysaudio device available */
    SysEntry = (PSYSAUDIO_ENTRY)DeviceExtension->SysAudioDeviceList.Flink;

    DPRINT1("Opening device %S\n", SysEntry->SymbolicLink.Buffer);
    Status = WdmAudOpenSysAudioDevice(SysEntry->SymbolicLink.Buffer, &hSysAudio);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open sysaudio %x\n", Status);
        ExFreePool(Client);
        return Status;
    }

    /* get the file object */
    Status = ObReferenceObjectByHandle(hSysAudio, FILE_READ_DATA | FILE_WRITE_DATA, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference FileObject %x\n", Status);
        ExFreePool(Client);
        ZwClose(hSysAudio);
        return Status;
    }

    Client->hSysAudio = hSysAudio;
    Client->FileObject = FileObject;
    Client->hProcess = PsGetCurrentProcessId();

    *pClient = Client;

    return STATUS_SUCCESS;
}



