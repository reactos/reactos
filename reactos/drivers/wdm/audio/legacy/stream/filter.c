/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/stream/filter.c
 * PURPOSE:         filter instance handling
 * PROGRAMMER:      Johannes Anderwald
 */


#include "stream.h"

NTSTATUS
NTAPI
FilterDispatch_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT1("FilterDispatch Called\n");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
FilterDispatch_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSTREAM_DEVICE_EXTENSION DeviceExtension;
    HW_STREAM_REQUEST_BLOCK_EXT RequestBlock;

   /* Get device extension */
    DeviceExtension = (PSTREAM_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (!DeviceExtension->DriverExtension->Data.FilterInstanceExtensionSize)
    {
        /* driver supports only one instance */
        if (DeviceExtension->InstanceCount)
        {
            /* there is already one instance open */
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        /* driver supports more than one filter instance */
        RtlZeroMemory(&RequestBlock, sizeof(HW_STREAM_REQUEST_BLOCK_EXT));

        /* set up request block */
        RequestBlock.Block.Command = SRB_CLOSE_DEVICE_INSTANCE;
        RequestBlock.Block.HwDeviceExtension = DeviceExtension->DeviceExtension;
        RequestBlock.Block.Irp = Irp;
        KeInitializeEvent(&RequestBlock.Event, SynchronizationEvent, FALSE);

        /*FIXME SYNCHRONIZATION */

        /* Send the request */
        DeviceExtension->DriverExtension->Data.HwReceivePacket((PHW_STREAM_REQUEST_BLOCK)&RequestBlock);
        if (RequestBlock.Block.Status == STATUS_PENDING)
        {
            /* Wait for the request */
            KeWaitForSingleObject(&RequestBlock.Event, Executive, KernelMode, FALSE, NULL);
        }
    }

    /* Increment total instance count */
    InterlockedDecrement(&DeviceExtension->InstanceCount);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static KSDISPATCH_TABLE DispatchTable =
{
    FilterDispatch_fnDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    FilterDispatch_fnClose,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

NTSTATUS
InitializeFilterWithKs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;
    PSTREAM_DEVICE_EXTENSION DeviceExtension;
    KSOBJECT_HEADER ObjectHeader;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PIO_STACK_LOCATION IoStack;
    HW_STREAM_REQUEST_BLOCK_EXT RequestBlock;
    PVOID HwInstanceExtension = NULL;

   /* Get device extension */
    DeviceExtension = (PSTREAM_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (!DeviceExtension->DriverExtension->Data.FilterInstanceExtensionSize)
    {
        /* driver supports only one instance */
        if (DeviceExtension->InstanceCount)
        {
            /* there is already one instance open */
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        /* driver supports more than one filter instance */
        RtlZeroMemory(&RequestBlock, sizeof(HW_STREAM_REQUEST_BLOCK_EXT));

        /* allocate instance extension */
        HwInstanceExtension = ExAllocatePool(NonPagedPool, DeviceExtension->DriverExtension->Data.FilterInstanceExtensionSize);
        if (!HwInstanceExtension)
        {
            /* Not enough memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Zero instance extension */
        RtlZeroMemory(HwInstanceExtension, DeviceExtension->DriverExtension->Data.FilterInstanceExtensionSize);

        /* set up request block */
        RequestBlock.Block.Command = SRB_OPEN_DEVICE_INSTANCE;
        RequestBlock.Block.HwDeviceExtension = DeviceExtension->DeviceExtension;
        RequestBlock.Block.Irp = Irp;
        RequestBlock.Block.HwInstanceExtension = HwInstanceExtension;
        KeInitializeEvent(&RequestBlock.Event, SynchronizationEvent, FALSE);

        /*FIXME SYNCHRONIZATION */

        /* Send the request */
        DeviceExtension->DriverExtension->Data.HwReceivePacket((PHW_STREAM_REQUEST_BLOCK)&RequestBlock);
        if (RequestBlock.Block.Status == STATUS_PENDING)
        {
            /* Wait for the request */
            KeWaitForSingleObject(&RequestBlock.Event, Executive, KernelMode, FALSE, NULL);
        }
        /* Check for success */
        if (!NT_SUCCESS(RequestBlock.Block.Status))
        {
            /* Resource is not available */
            ExFreePool(HwInstanceExtension);
            return RequestBlock.Block.Status;
        }
    }

    /* Allocate create item */
    CreateItem = ExAllocatePool(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM));
    if (!CreateItem)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Zero create item */
    RtlZeroMemory(CreateItem, sizeof(KSOBJECT_CREATE_ITEM));
    /* Set item class */
    RtlInitUnicodeString(&CreateItem->ObjectClass, L"STREAMCLASS");
    /* Get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* Create Ks streaming object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 1, CreateItem, Irp, &DispatchTable);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to create header */
        ExFreePool(CreateItem);
        if (HwInstanceExtension)
        {
            /* free instance buffer */
            ExFreePool(HwInstanceExtension);
        }
        return Status;
    }

    /* Store instance buffer in file object context */
    IoStack->FileObject->FsContext2 = HwInstanceExtension;

    /* Increment total instance count */
    InterlockedIncrement(&DeviceExtension->InstanceCount);
    /* Return result */
    return Status;

}

NTSTATUS
NTAPI
StreamClassCreateFilter(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    DPRINT1("StreamClassCreateFilter Called\n");

    /* FIXME Support Pins/Clocks */
    /* Init filter */
    Status = InitializeFilterWithKs(DeviceObject, Irp);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}



