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

    DPRINT1("DispatchCreateSysAudio entered\n");
    DbgBreakPoint();

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

