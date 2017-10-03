/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/filters/kmixer/filter.c
 * PURPOSE:         Filter File Context Header header
 * PROGRAMMER:      Johannes Anderwald
 */

#include "kmixer.h"

#include <swenum.h>

#define YDEBUG
#include <debug.h>

NTSTATUS
NTAPI
Dispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED;
    DbgBreakPoint();

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Dispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static KSDISPATCH_TABLE DispatchTable =
{
    Dispatch_fnDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    Dispatch_fnClose,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastWriteFailure,
    KsDispatchFastWriteFailure,
};

NTSTATUS
NTAPI
DispatchCreateKMixPin(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;

    DPRINT("DispatchCreateKMix entered\n");

    /* create the pin */
    Status = CreatePin(Irp);

    /* save result */
    Irp->IoStatus.Status = Status;
    /* complete the request */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* done */
    return Status;
}

NTSTATUS
NTAPI
DispatchCreateKMixAllocator(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;

    /* create the allocator */
    Status = KsCreateDefaultAllocator(Irp);

    /* save result */
    Irp->IoStatus.Status = Status;
    /* complete the request */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    /* done */
    return Status;
}

NTSTATUS
NTAPI
DispatchCreateKMix(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    KSOBJECT_HEADER ObjectHeader;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PKMIXER_DEVICE_EXT DeviceExtension;

    DPRINT("DispatchCreateKMix entered\n");

    /* check if the request was from usermode */
    if (Irp->RequestorMode == UserMode)
    {
        /* deny access from usermode */
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* get device extension */
    DeviceExtension = (PKMIXER_DEVICE_EXT)DeviceObject->DeviceExtension;

#if 0
    /* reference the software bus object */
    Status = KsReferenceSoftwareBusObject(DeviceExtension->KsDeviceHeader);

    if (!NT_SUCCESS(Status))
    {
        /* failed to reference bus object */
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
#endif

   /* allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM) * 2);
    if (!CreateItem)
    {
        /* not enough memory */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* zero create struct */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM) * 2);

    /* initialize pin create item */
    CreateItem[0].Create = DispatchCreateKMixPin;
    RtlInitUnicodeString(&CreateItem[0].ObjectClass, KSSTRING_Pin);
    CreateItem[1].Create = DispatchCreateKMixAllocator;
    RtlInitUnicodeString(&CreateItem[1].ObjectClass, KSSTRING_Allocator);

    /* allocate object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 2, CreateItem, Irp, &DispatchTable);

    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate object header */
        ExFreePool(CreateItem);
        KsDereferenceSoftwareBusObject(DeviceExtension->KsDeviceHeader);
    }

    DPRINT("KsAllocateObjectHeader result %x\n", Status);
    /* complete the irp */
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
KMixAllocateDeviceHeader(
    IN PKMIXER_DEVICE_EXT DeviceExtension)
{
    NTSTATUS Status;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM) * 2);
    if (!CreateItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize create item struct */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM) * 2);
    CreateItem[0].Create = DispatchCreateKMix;
    RtlInitUnicodeString(&CreateItem[0].ObjectClass, L"GLOBAL");
    CreateItem[1].Create = DispatchCreateKMix;
    RtlInitUnicodeString(&CreateItem[1].ObjectClass, KSSTRING_Filter);

    Status = KsAllocateDeviceHeader(&DeviceExtension->KsDeviceHeader,
                                    2,
                                    CreateItem);
    return Status;
}

