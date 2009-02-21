/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/dispatcher.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#define YDEBUG
#include <debug.h>
#include "sysaudio.h"

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    DPRINT1("Dispatch_fnDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);

    IoStack = IoGetCurrentIrpStackLocation(Irp);


    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
       return SysAudioHandleProperty(DeviceObject, Irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT1("Dispatch_fnRead called DeviceObject %p Irp %p\n", DeviceObject);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT1("Dispatch_fnWrite called DeviceObject %p Irp %p\n", DeviceObject);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT1("Dispatch_fnFlush called DeviceObject %p Irp %p\n", DeviceObject);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT1("Dispatch_fnClose called DeviceObject %p Irp %p\n", DeviceObject);


    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT1("Dispatch_fnQuerySecurity called DeviceObject %p Irp %p\n", DeviceObject);


    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Dispatch_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{

    DPRINT1("Dispatch_fnSetSecurity called DeviceObject %p Irp %p\n", DeviceObject);

    return STATUS_SUCCESS;
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
    DPRINT1("Dispatch_fnFastDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);


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
    DPRINT1("Dispatch_fnFastRead called DeviceObject %p Irp %p\n", DeviceObject);

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
    DPRINT1("Dispatch_fnFastWrite called DeviceObject %p Irp %p\n", DeviceObject);

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

VOID
NTAPI
CreatePinWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID  Context)
{
    NTSTATUS Status;
    HANDLE PinHandle;
    HANDLE * Handels;
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PPIN_WORKER_CONTEXT WorkerContext = (PPIN_WORKER_CONTEXT)Context;

    Handels = ExAllocatePool(NonPagedPool, (WorkerContext->Entry->NumberOfPins + 1) * sizeof(HANDLE));
    if (!Handels)
    {
        DPRINT1("No Memory \n");
        WorkerContext->Irp->IoStatus.Status = STATUS_NO_MEMORY;
        WorkerContext->Irp->IoStatus.Information = 0;
        IoCompleteRequest(WorkerContext->Irp, IO_SOUND_INCREMENT);
        return;
    }


    Status = KsCreatePin(WorkerContext->Entry->Handle, WorkerContext->PinConnect, GENERIC_READ | GENERIC_WRITE, &PinHandle);
    DPRINT1("KsCreatePin status %x\n", Status);

    if (NT_SUCCESS(Status))
    {
         Status = ObReferenceObjectByHandle(PinHandle, GENERIC_READ | GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
         if (NT_SUCCESS(Status))
         {
             Status = CreateDispatcher(WorkerContext->Irp, PinHandle, FileObject);
             DPRINT1("Pins %x\n", WorkerContext->Entry->NumberOfPins);
             if (WorkerContext->Entry->NumberOfPins)
             {
                 RtlMoveMemory(Handels, WorkerContext->Entry->Pins, WorkerContext->Entry->NumberOfPins * sizeof(HANDLE));
                 ExFreePool(WorkerContext->Entry->Pins);
             }
             Handels[WorkerContext->Entry->NumberOfPins-1] = PinHandle;
             WorkerContext->Entry->Pins = Handels;
             WorkerContext->Entry->NumberOfPins++;
         }
    }

    DPRINT1("CreatePinWorkerRoutine completing irp\n");
    WorkerContext->Irp->IoStatus.Status = Status;
    WorkerContext->Irp->IoStatus.Information = 0;

    Irp = WorkerContext->Irp;
    ExFreePool(Context);

    IoCompleteRequest(Irp, IO_SOUND_INCREMENT);
}

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
    ULONG Length, DeviceIndex;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSAUDIO_DEVICE_ENTRY Entry;
    KSPIN_CONNECT * PinConnect;
    PIO_WORKITEM WorkItem;
    PPIN_WORKER_CONTEXT Context;
    static LPWSTR KS_NAME_PIN = L"{146F1A80-4791-11D0-A5D6-28DB04C10000}";

    IoStatus = IoGetCurrentIrpStackLocation(Irp);
    Buffer = IoStatus->FileObject->FileName.Buffer;

    DPRINT1("DispatchCreateSysAudio entered\n");

    if (Buffer)
    {
        /* is the request for a new pin */
        if (!wcsncmp(KS_NAME_PIN, Buffer, wcslen(KS_NAME_PIN)))
        {
            Client = (PSYSAUDIO_CLIENT)Irp->Tail.Overlay.OriginalFileObject->FsContext2;
            DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;
            if (Client)
            {
                ASSERT(Client->NumDevices >= 1);
                DeviceIndex = Client->Devices[Client->NumDevices-1];
            }
            else
            {
                DPRINT1("Warning: using HACK\n");
                DeviceIndex = 0;
            }
            ASSERT(DeviceIndex < DeviceExtension->NumberOfKsAudioDevices);
            Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, DeviceIndex);
            ASSERT(Entry);

            Length = (IoStatus->FileObject->FileName.Length - ((wcslen(KS_NAME_PIN)+1) * sizeof(WCHAR)));
            PinConnect = ExAllocatePool(NonPagedPool, Length);
            if (!PinConnect)
            {
                Irp->IoStatus.Status = STATUS_NO_MEMORY;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_NO_MEMORY;
            }
            RtlMoveMemory(PinConnect, IoStatus->FileObject->FileName.Buffer + (wcslen(KS_NAME_PIN)+1), Length);
            Context = ExAllocatePool(NonPagedPool, sizeof(PIN_WORKER_CONTEXT));
            if (!Context)
            {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            Context->PinConnect = PinConnect;
            Context->Entry = Entry;
            Context->Irp = Irp;

            WorkItem = IoAllocateWorkItem(DeviceObject);
            if (!WorkItem)
            {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            IoQueueWorkItem(WorkItem, CreatePinWorkerRoutine, DelayedWorkQueue, (PVOID)Context);
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            return STATUS_PENDING;
        }
    }

    /* allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
    if (!CreateItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    Client = ExAllocatePool(NonPagedPool, sizeof(SYSAUDIO_CLIENT));
    if (!Client)
    {
        ExFreePool(CreateItem);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    /* initialize client struct */
    RtlZeroMemory(Client, sizeof(SYSAUDIO_CLIENT));

    /* zero create struct */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM));

    /* store create context */
    CreateItem->Context = (PVOID)Client;

    /* store the object in FsContext */
    IoStatus->FileObject->FsContext2 = (PVOID)Client;

    /* allocate object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 1, CreateItem, Irp, &DispatchTable);

    DPRINT1("KsAllocateObjectHeader result %x\n", Status);
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

