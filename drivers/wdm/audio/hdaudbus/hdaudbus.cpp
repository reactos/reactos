/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            drivers/wdm/audio/hdaudbus/hdaudbus.cpp
* PURPOSE:         HDA Driver Entry
* PROGRAMMER:      Johannes Anderwald
*/
#include "hdaudbus.h"

DRIVER_DISPATCH HDA_Pnp;
DRIVER_DISPATCH HDA_SystemControl;
DRIVER_DISPATCH HDA_Power;
DRIVER_ADD_DEVICE HDA_AddDevice;
DRIVER_UNLOAD HDA_Unload;
extern "C" DRIVER_INITIALIZE DriverEntry;

PVOID
AllocateItem(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes)
{
    PVOID Item = ExAllocatePoolWithTag(PoolType, NumberOfBytes, TAG_HDA);
    if (!Item)
        return Item;

    RtlZeroMemory(Item, NumberOfBytes);
    return Item;
}

VOID
FreeItem(
    __drv_freesMem(Mem) PVOID Item)
{
    ExFreePool(Item);
}

NTSTATUS
HDA_FdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PHDA_FDO_DEVICE_EXTENSION FDODeviceExtension;
    ULONG CodecIndex, AFGIndex;
    PHDA_CODEC_ENTRY CodecEntry;
    PHDA_PDO_DEVICE_EXTENSION ChildDeviceExtension;

    FDODeviceExtension = static_cast<PHDA_FDO_DEVICE_EXTENSION>(DeviceObject->DeviceExtension);
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        Status = HDA_FDOStartDevice(DeviceObject, Irp);
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    case IRP_MN_REMOVE_DEVICE:
        return HDA_FDORemoveDevice(DeviceObject, Irp);
    case IRP_MN_SURPRISE_REMOVAL:
        for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
        {
            CodecEntry = FDODeviceExtension->Codecs[CodecIndex];

            ASSERT(CodecEntry->AudioGroupCount <= HDA_MAX_AUDIO_GROUPS);
            for (AFGIndex = 0; AFGIndex < CodecEntry->AudioGroupCount; AFGIndex++)
            {
                ChildDeviceExtension = static_cast<PHDA_PDO_DEVICE_EXTENSION>(CodecEntry->AudioGroups[AFGIndex]->ChildPDO->DeviceExtension);
                ChildDeviceExtension->ReportedMissing = TRUE;
            }
        }
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        /* handle bus device relations */
        if (IoStack->Parameters.QueryDeviceRelations.Type == BusRelations)
        {
            Status = HDA_FDOQueryBusRelations(DeviceObject, Irp);
            Irp->IoStatus.Status = Status;
            if (!NT_SUCCESS(Status))
            {
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
        }
        break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FDODeviceExtension->LowerDevice, Irp);
}

NTSTATUS
HDA_PdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_RELATIONS DeviceRelation;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        /* no op for pdo */
        Status = STATUS_SUCCESS;
        break;
    case IRP_MN_REMOVE_DEVICE:
        Status = HDA_PDORemoveDevice(DeviceObject);
        break;
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        Status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_BUS_INFORMATION:
        /* query bus information */
        Status = HDA_PDOQueryBusInformation(Irp);
        break;
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        /* query pnp state */
        Status = HDA_PDOQueryBusDevicePnpState(Irp);
        break;
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
        {
            /* handle target device relations */
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
        else
        {
            Status = Irp->IoStatus.Status;
        }
        break;
    case IRP_MN_QUERY_CAPABILITIES:
        /* query capabilities */
        Status = HDA_PDOQueryBusDeviceCapabilities(Irp);
        break;
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        /* no op */
        Status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_RESOURCES:
        /* no op */
        Status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_ID:
        Status = HDA_PDOQueryId(DeviceObject, Irp);
        break;
    case IRP_MN_QUERY_DEVICE_TEXT:
        Status = HDA_PDOHandleQueryDeviceText(Irp);
        break;
    case IRP_MN_QUERY_INTERFACE:
        Status = HDA_PDOHandleQueryInterface(DeviceObject, Irp);
        break;
    default:
        /* get default status */
        Status = Irp->IoStatus.Status;
        break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
HDA_Pnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PHDA_FDO_DEVICE_EXTENSION FDODeviceExtension;

    FDODeviceExtension = static_cast<PHDA_FDO_DEVICE_EXTENSION>(DeviceObject->DeviceExtension);

    if (FDODeviceExtension->IsFDO)
    {
        return HDA_FdoPnp(DeviceObject, Irp);
    }
    else
    {
        return HDA_PdoPnp(DeviceObject, Irp);
    }
}

NTSTATUS
NTAPI
HDA_SystemControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PHDA_FDO_DEVICE_EXTENSION FDODeviceExtension;

    FDODeviceExtension = static_cast<PHDA_FDO_DEVICE_EXTENSION>(DeviceObject->DeviceExtension);

    if (FDODeviceExtension->IsFDO)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(FDODeviceExtension->LowerDevice, Irp);
    }
    else
    {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}

NTSTATUS
NTAPI
HDA_Power(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PHDA_FDO_DEVICE_EXTENSION FDODeviceExtension;

    FDODeviceExtension = static_cast<PHDA_FDO_DEVICE_EXTENSION>(DeviceObject->DeviceExtension);

    if (FDODeviceExtension->IsFDO)
    {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(FDODeviceExtension->LowerDevice, Irp);
    }
    else
    {
        Status = Irp->IoStatus.Status;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}

NTSTATUS
NTAPI
HDA_AddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
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
    IoInitializeDpcRequest(DeviceObject, HDA_DpcForIsr);
    RtlZeroMemory(DeviceExtension->Codecs, sizeof(PHDA_CODEC_ENTRY) * (HDA_MAX_CODECS + 1));

    /* set device flags */
    DeviceObject->Flags |= DO_POWER_PAGABLE;

    return Status;
}

VOID
NTAPI
HDA_Unload(
    _In_ PDRIVER_OBJECT DriverObject)
{
}

extern "C"
{
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPathName)
{
    DriverObject->DriverUnload = HDA_Unload;
    DriverObject->DriverExtension->AddDevice = HDA_AddDevice;
    DriverObject->MajorFunction[IRP_MJ_POWER] = HDA_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HDA_SystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = HDA_Pnp;

    return STATUS_SUCCESS;
}

}
