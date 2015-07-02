/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            drivers/wdm/audio/hdaudbus/hdaudbus.cpp
* PURPOSE:         HDA Driver Entry
* PROGRAMMER:      Johannes Anderwald
*/
#include "hdaudbus.h"


PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    PVOID Item = ExAllocatePoolWithTag(PoolType, NumberOfBytes, TAG_HDA);
    if (!Item)
        return Item;

    RtlZeroMemory(Item, NumberOfBytes);
    return Item;
}

VOID
FreeItem(
    IN PVOID Item)
{
    ExFreePool(Item);
}

NTSTATUS
NTAPI
HDA_SyncForwardIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    if (Irp->PendingReturned)
    {
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
HDA_SyncForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    KEVENT Event;
    NTSTATUS Status;

    /* Initialize event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Copy irp stack location */
    IoCopyCurrentIrpStackLocationToNext(Irp);

    /* Set completion routine */
    IoSetCompletionRoutine(Irp,
        HDA_SyncForwardIrpCompletionRoutine,
        &Event,
        TRUE,
        TRUE,
        TRUE);

    /* Call driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Check if pending */
    if (Status == STATUS_PENDING)
    {
        /* Wait for the request to finish */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

        /* Copy status code */
        Status = Irp->IoStatus.Status;
    }

    /* Done */
    return Status;
}

NTSTATUS
NTAPI
HDA_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_RELATIONS DeviceRelation;
    PHDA_FDO_DEVICE_EXTENSION FDODeviceExtension;
    //PHDA_PDO_DEVICE_EXTENSION ChildDeviceExtension;

    FDODeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    //ChildDeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (FDODeviceExtension->IsFDO)
    {
        if (IoStack->MinorFunction == IRP_MN_START_DEVICE)
        {
            Status = HDA_FDOStartDevice(DeviceObject, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)
        {
            /* handle bus device relations */
            if (IoStack->Parameters.QueryDeviceRelations.Type == BusRelations)
            {
                Status = HDA_FDOQueryBusRelations(DeviceObject, Irp);
            }
            else
            {
                Status = Irp->IoStatus.Status;
            }
        } 
        else
        {
            /* get default status */
            Status = Irp->IoStatus.Status;
        }
    }
    else
    {
        if (IoStack->MinorFunction == IRP_MN_START_DEVICE)
        {
            /* no op for pdo */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_BUS_INFORMATION)
        {
            /* query bus information */
            Status = HDA_PDOQueryBusInformation(Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
        {
            /* query pnp state */
            Status = HDA_PDOQueryBusDevicePnpState(Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)
        {
            if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
            {
                /* handle target device relations */
                ASSERT(IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation);
                ASSERT(Irp->IoStatus.Information == 0);

                /* allocate device relation */
                DeviceRelation = (PDEVICE_RELATIONS)AllocateItem(PagedPool, sizeof(DEVICE_RELATIONS));
                if (DeviceRelation)
                {
                    DeviceRelation->Count = 1;
                    DeviceRelation->Objects[0] = DeviceObject;

                    /* reference self */
                    ObReferenceObject(DeviceObject);

                    /* store result */
                    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelation;

                    /* done */
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    /* no memory */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_CAPABILITIES)
        {
            /* query capabilities */
            Status = HDA_PDOQueryBusDeviceCapabilities(Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_RESOURCE_REQUIREMENTS)
        {
            /* no op */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_RESOURCES)
        {
            /* no op */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_ID)
        {
            Status = HDA_PDOQueryId(DeviceObject, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_TEXT)
        {
            Status = HDA_PDOHandleQueryDeviceText(Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_INTERFACE)
        {
            Status = HDA_PDOHandleQueryInterface(DeviceObject, Irp);
        }
        else
        {
            /* get default status */
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);


    return Status;
}


//PDRIVER_ADD_DEVICE HDA_AddDevice;

NTSTATUS
NTAPI
HDA_AddDevice(
IN PDRIVER_OBJECT DriverObject,
IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT DeviceObject;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    /* create device object */
    Status = IoCreateDevice(DriverObject, sizeof(HDA_FDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* get device extension*/
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* init device extension*/
    DeviceExtension->IsFDO = TRUE;
    DeviceExtension->LowerDevice = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    RtlZeroMemory(DeviceExtension->Codecs, sizeof(PHDA_CODEC_ENTRY) * (HDA_MAX_CODECS + 1));

    /* set device flags */
    DeviceObject->Flags |= DO_POWER_PAGABLE;

    return Status;
}
extern "C"
{
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName)
{
    DriverObject->DriverExtension->AddDevice = HDA_AddDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP] = HDA_Pnp;

    return STATUS_SUCCESS;
}

}