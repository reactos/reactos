/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */

#include "wdmaud.h"

#define YDEBUG
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
WdmAudOpenSysAudioDevices(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LPWSTR SymbolicLinkList;
    HANDLE hSysAudio;
    PFILE_OBJECT FileObject;

    Status = IoGetDeviceInterfaces(&KSCATEGORY_SYSAUDIO,
                                   NULL,
                                   0,
                                   &SymbolicLinkList);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    while (*SymbolicLinkList != UNICODE_NULL)
    {
        DPRINT("Opening device %S\n", SymbolicLinkList);
        Status = WdmAudOpenSysAudioDevice(SymbolicLinkList, &hSysAudio);
        if (NT_SUCCESS(Status))
        {
            break;
        }
        else
        {
            DPRINT1("Failed to open sysaudio %x\n", Status);
            continue;
        }
    }

    if (!hSysAudio)
    {
        DPRINT1("Failed to find sysaudio devices %x\n", Status);
        FreeItem(SymbolicLinkList);
        return Status;
    }

    DeviceExtension->NumSysAudioDevices++;

    /* get the file object */
    Status = ObReferenceObjectByHandle(hSysAudio,
                                       FILE_READ_DATA | FILE_WRITE_DATA,
                                       *IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference FileObject %x\n", Status);
        ZwClose(hSysAudio);
        FreeItem(SymbolicLinkList);
        return Status;
    }

    DeviceExtension->hSysAudio = hSysAudio;
    DeviceExtension->FileObject = FileObject;

    return STATUS_SUCCESS;
}

NTSTATUS
GetSysAudioDeviceInterface(
    OUT LPWSTR* SymbolicLinkList)
{
    NTSTATUS Status;

    /* Get SysAudio device interface */
    Status = IoGetDeviceInterfaces(&KSCATEGORY_SYSAUDIO, NULL, 0, SymbolicLinkList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoGetDeviceInterfaces failed with 0x%lx\n", Status);
        return Status;
    }

    DPRINT("Got SysAudio device interface %ls\n", *SymbolicLinkList);
    return STATUS_SUCCESS;
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
