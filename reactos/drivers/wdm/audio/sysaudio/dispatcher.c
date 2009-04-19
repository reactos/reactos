/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/dispatcher.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include "sysaudio.h"

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
       return SysAudioHandleProperty(DeviceObject, Irp);
    }

    /* unsupported request */
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    /* unsupported request */
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    /* unsupported request */
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PSYSAUDIO_CLIENT Client;
    PKSAUDIO_DEVICE_ENTRY Entry;
    PIO_STACK_LOCATION IoStatus;
    ULONG Index, SubIndex;
    PSYSAUDIODEVEXT DeviceExtension;
    PDISPATCH_CONTEXT DispatchContext;


    IoStatus = IoGetCurrentIrpStackLocation(Irp);

    Client = (PSYSAUDIO_CLIENT)IoStatus->FileObject->FsContext2;
    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;


    DPRINT("Client %p NumDevices %u\n", Client, Client->NumDevices);
    for(Index = 0; Index < Client->NumDevices; Index++)
    {
        DPRINT("Index %u Device %u Handels Count %u\n", Index, Client->Devs[Index].DeviceId, Client->Devs[Index].ClientHandlesCount);
        if (Client->Devs[Index].ClientHandlesCount)
        {
            Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, Client->Devs[Index].DeviceId);
            ASSERT(Entry != NULL);

            for(SubIndex = 0; SubIndex < Client->Devs[Index].ClientHandlesCount; SubIndex++)
            {
                if (Client->Devs[Index].ClientHandles[SubIndex].PinId == (ULONG)-1)
                    continue;

                if (Client->Devs[Index].ClientHandles[SubIndex].bHandle)
                {
                    DPRINT("Closing handle %p\n", Client->Devs[Index].ClientHandles[SubIndex].hPin);

                    ZwClose(Client->Devs[Index].ClientHandles[SubIndex].hPin);
                    Entry->Pins[Client->Devs[Index].ClientHandles[SubIndex].PinId].References--;
                }
                else
                {
                    /* this is a pin which can only be instantiated once
                     * so we just need to release the reference count on that pin
                     */
                    Entry->Pins[Client->Devs[Index].ClientHandles[SubIndex].PinId].References--;

                    DispatchContext = (PDISPATCH_CONTEXT)Client->Devs[Index].ClientHandles[SubIndex].DispatchContext;

                    if (DispatchContext->MixerFileObject)
                        ObDereferenceObject(DispatchContext->MixerFileObject);

                    if (DispatchContext->hMixerPin)
                        ZwClose(DispatchContext->hMixerPin);

                    if (DispatchContext->FileObject)
                        ObDereferenceObject(DispatchContext->FileObject);

                    ExFreePool(DispatchContext);

                    DPRINT("Index %u DeviceIndex %u Pin %u References %u\n", Index, Client->Devs[Index].DeviceId, SubIndex, Entry->Pins[Client->Devs[Index].ClientHandles[SubIndex].PinId].References);
                    if (!Entry->Pins[Client->Devs[Index].ClientHandles[SubIndex].PinId].References)
                    {
                        DPRINT("Closing pin %p\n", Entry->Pins[Client->Devs[Index].ClientHandles[SubIndex].PinId].PinHandle);

                        ZwClose(Entry->Pins[Client->Devs[Index].ClientHandles[SubIndex].PinId].PinHandle);
                        Entry->Pins[Client->Devs[Index].ClientHandles[SubIndex].PinId].PinHandle = NULL;
                    }
                }
            }
            ExFreePool(Client->Devs[Index].ClientHandles);
        }
    }

    if (Client->Devs)
        ExFreePool(Client->Devs);

    ExFreePool(Client);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT("Dispatch_fnQuerySecurity called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{

    DPRINT("Dispatch_fnSetSecurity called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
Dispatch_fnFastDeviceIoControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    DPRINT("Dispatch_fnFastDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);


    return FALSE;
}


BOOLEAN
NTAPI
Dispatch_fnFastRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    DPRINT("Dispatch_fnFastRead called DeviceObject %p Irp %p\n", DeviceObject);

    return FALSE;

}

BOOLEAN
NTAPI
Dispatch_fnFastWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    DPRINT("Dispatch_fnFastWrite called DeviceObject %p Irp %p\n", DeviceObject);

    return FALSE;
}

static KSDISPATCH_TABLE DispatchTable =
{
    Dispatch_fnDeviceIoControl,
    Dispatch_fnRead,
    Dispatch_fnWrite,
    Dispatch_fnFlush,
    Dispatch_fnClose,
    Dispatch_fnQuerySecurity,
    Dispatch_fnSetSecurity,
    Dispatch_fnFastDeviceIoControl,
    Dispatch_fnFastRead,
    Dispatch_fnFastWrite,
};

NTSTATUS
NTAPI
DispatchCreateSysAudio(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    KSOBJECT_HEADER ObjectHeader;
    PSYSAUDIO_CLIENT Client;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PIO_STACK_LOCATION IoStatus;
    LPWSTR Buffer;
    PSYSAUDIODEVEXT DeviceExtension;
    ULONG Index;

    static LPWSTR KS_NAME_PIN = L"{146F1A80-4791-11D0-A5D6-28DB04C10000}";

    IoStatus = IoGetCurrentIrpStackLocation(Irp);
    Buffer = IoStatus->FileObject->FileName.Buffer;

    DPRINT("DispatchCreateSysAudio entered\n");

    if (Buffer)
    {
        /* is the request for a new pin */
        if (!wcsncmp(KS_NAME_PIN, Buffer, wcslen(KS_NAME_PIN)))
        {
            Status = CreateDispatcher(Irp);
            DPRINT("Virtual pin Status %x FileObject %p\n", Status, IoStatus->FileObject);

            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }

    /* allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
    if (!CreateItem)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Client = ExAllocatePool(NonPagedPool, sizeof(SYSAUDIO_CLIENT));
    if (!Client)
    {
        ExFreePool(CreateItem);

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get device extension */
    DeviceExtension = (PSYSAUDIODEVEXT) DeviceObject->DeviceExtension;

    Client->NumDevices = DeviceExtension->NumberOfKsAudioDevices;
    /* has sysaudio found any devices */
    if (Client->NumDevices)
    {
        Client->Devs = ExAllocatePool(NonPagedPool, sizeof(SYSAUDIO_CLIENT_HANDELS) * Client->NumDevices);
        if (!Client->Devs)
        {
            ExFreePool(CreateItem);
            ExFreePool(Client);
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        /* no devices yet available */
        Client->Devs = NULL;
    }

    /* Initialize devs array */
    for(Index = 0; Index < Client->NumDevices; Index++)
    {
        Client->Devs[Index].DeviceId = Index;
        Client->Devs[Index].ClientHandles = NULL;
        Client->Devs[Index].ClientHandlesCount = 0;
    }

    /* zero create struct */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM));

    /* store create context */
    CreateItem->Context = (PVOID)Client;

    /* store the object in FsContext */
    IoStatus->FileObject->FsContext2 = (PVOID)Client;

    /* allocate object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 1, CreateItem, Irp, &DispatchTable);

    DPRINT("KsAllocateObjectHeader result %x\n", Status);
    /* complete the irp */
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
SysAudioAllocateDeviceHeader(
    IN SYSAUDIODEVEXT *DeviceExtension)
{
    NTSTATUS Status;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
    if (!CreateItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize create item struct */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM));
    CreateItem->Create = DispatchCreateSysAudio;
    RtlInitUnicodeString(&CreateItem->ObjectClass, L"SysAudio");

    Status = KsAllocateDeviceHeader(&DeviceExtension->KsDeviceHeader,
                                    1,
                                    CreateItem);
    return Status;
}

NTSTATUS
SysAudioOpenKMixer(
    IN SYSAUDIODEVEXT *DeviceExtension)
{
    NTSTATUS Status;
    
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\kmixer");
    UNICODE_STRING DevicePath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\kmixer");

    Status = ZwLoadDriver(&DevicePath);

    if (NT_SUCCESS(Status))
    {
        Status = OpenDevice(&DeviceName, &DeviceExtension->KMixerHandle, &DeviceExtension->KMixerFileObject);
        if (!NT_SUCCESS(Status))
        {
            DeviceExtension->KMixerHandle = NULL;
            DeviceExtension->KMixerFileObject = NULL;
        }
    }

    DPRINT("Status %lx KMixerHandle %p KMixerFileObject %p\n", Status, DeviceExtension->KMixerHandle, DeviceExtension->KMixerFileObject);
    return STATUS_SUCCESS;
}
