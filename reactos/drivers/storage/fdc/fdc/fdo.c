/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/fdo.c
 * PURPOSE:        Functional Device Object routines
 * PROGRAMMERS:    Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include "fdc.h"

/* FUNCTIONS ******************************************************************/

static IO_COMPLETION_ROUTINE ForwardIrpAndWaitCompletion;

static
NTSTATUS
NTAPI
ForwardIrpAndWaitCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    if (Irp->PendingReturned)
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
ForwardIrpAndWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
    KEVENT Event;
    NTSTATUS Status;

    ASSERT(LowerDevice);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);

    DPRINT("Calling lower device %p\n", LowerDevice);
    IoSetCompletionRoutine(Irp, ForwardIrpAndWaitCompletion, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(LowerDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        if (NT_SUCCESS(Status))
            Status = Irp->IoStatus.Status;
    }

    return Status;
}


NTSTATUS
NTAPI
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;

    ASSERT(LowerDevice);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(LowerDevice, Irp);
}


NTSTATUS
NTAPI
FdcAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo)
{
    PFDO_DEVICE_EXTENSION DeviceExtension = NULL;
    PDEVICE_OBJECT Fdo = NULL;
    NTSTATUS Status;

    DPRINT1("FdcAddDevice()\n");

    ASSERT(DriverObject);
    ASSERT(Pdo);

    /* Create functional device object */
    Status = IoCreateDevice(DriverObject,
                            sizeof(FDO_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (NT_SUCCESS(Status))
    {
        DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;
        RtlZeroMemory(DeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

        DeviceExtension->Common.IsFDO = TRUE;

        DeviceExtension->Fdo = Fdo;
        DeviceExtension->Pdo = Pdo;


        Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
            IoDeleteDevice(Fdo);
            return Status;
        }


        Fdo->Flags |= DO_DIRECT_IO;
        Fdo->Flags |= DO_POWER_PAGABLE;

        Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return Status;
}


static
NTSTATUS
FdcFdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PCM_RESOURCE_LIST ResourceListTranslated)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptorTranslated;
    ULONG i;

    DPRINT1("FdcFdoStartDevice called\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    ASSERT(DeviceExtension);

    if (ResourceList == NULL ||
        ResourceListTranslated == NULL)
    {
        DPRINT1("No allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ResourceList->Count != 1)
    {
        DPRINT1("Wrong number of allocated resources sent to driver\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ResourceList->List[0].PartialResourceList.Version != 1 ||
        ResourceList->List[0].PartialResourceList.Revision != 1 ||
        ResourceListTranslated->List[0].PartialResourceList.Version != 1 ||
        ResourceListTranslated->List[0].PartialResourceList.Revision != 1)
    {
        DPRINT1("Revision mismatch: %u.%u != 1.1 or %u.%u != 1.1\n",
                ResourceList->List[0].PartialResourceList.Version,
                ResourceList->List[0].PartialResourceList.Revision,
                ResourceListTranslated->List[0].PartialResourceList.Version,
                ResourceListTranslated->List[0].PartialResourceList.Revision);
        return STATUS_REVISION_MISMATCH;
    }

    for (i = 0; i < ResourceList->List[0].PartialResourceList.Count; i++)
    {
        PartialDescriptor = &ResourceList->List[0].PartialResourceList.PartialDescriptors[i];
        PartialDescriptorTranslated = &ResourceListTranslated->List[0].PartialResourceList.PartialDescriptors[i];

        switch (PartialDescriptor->Type)
        {
            case CmResourceTypePort:
                DPRINT1("Port: 0x%lx (%lu)\n",
                        PartialDescriptor->u.Port.Start.u.LowPart,
                        PartialDescriptor->u.Port.Length);
                break;

            case CmResourceTypeInterrupt:
                DPRINT1("Interrupt: Level %lu  Vector %lu\n",
                        PartialDescriptorTranslated->u.Interrupt.Level,
                        PartialDescriptorTranslated->u.Interrupt.Vector);
                break;

            case CmResourceTypeDma:
                DPRINT1("Dma: Channel %lu\n",
                        PartialDescriptor->u.Dma.Channel);
                break;
        }
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
FdcFdoQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_RELATIONS *DeviceRelations)
{
    DPRINT1("FdcFdoQueryBusRelations() called\n");
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
FdcFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_RELATIONS DeviceRelations = NULL;
    ULONG_PTR Information = 0;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    DPRINT1("FdcFdoPnp()\n");

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            DPRINT1("  IRP_MN_START_DEVICE received\n");
            /* Call lower driver */
            Status = ForwardIrpAndWait(DeviceObject, Irp);
            if (NT_SUCCESS(Status))
            {
                Status = FdcFdoStartDevice(DeviceObject,
                                           IrpSp->Parameters.StartDevice.AllocatedResources,
                                           IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated);
            }
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            DPRINT1("  IRP_MN_QUERY_REMOVE_DEVICE\n");
            break;

        case IRP_MN_REMOVE_DEVICE:
            DPRINT1("  IRP_MN_REMOVE_DEVICE received\n");
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            DPRINT1("  IRP_MN_CANCEL_REMOVE_DEVICE\n");
            break;

        case IRP_MN_STOP_DEVICE:
            DPRINT1("  IRP_MN_STOP_DEVICE received\n");
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            DPRINT1("  IRP_MN_QUERY_STOP_DEVICE received\n");
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            DPRINT1("  IRP_MN_CANCEL_STOP_DEVICE\n");
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT1("  IRP_MN_QUERY_DEVICE_RELATIONS\n");

            switch (IrpSp->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                    DPRINT1("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
                    Status = FdcFdoQueryBusRelations(DeviceObject, &DeviceRelations);
                    Information = (ULONG_PTR)DeviceRelations;
                    break;

                case RemovalRelations:
                    DPRINT1("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceObject, Irp);

                default:
                    DPRINT1("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                            IrpSp->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceObject, Irp);
            }
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            DPRINT1("  IRP_MN_SURPRISE_REMOVAL received\n");
            break;

        default:
            DPRINT("  Unknown IOCTL 0x%lx\n", IrpSp->MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/* EOF */