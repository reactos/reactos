/*
 * PROJECT:     ReactOS InPort (Bus) Mouse Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Plug and Play requests handling
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "inport.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InPortPnp)
#pragma alloc_text(PAGE, InPortStartDevice)
#pragma alloc_text(PAGE, InPortRemoveDevice)
#endif

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
InPortStartDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PCM_RESOURCE_LIST AllocatedResources, AllocatedResourcesTranslated;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor, DescriptorTranslated;
    ULONG i;
    ULONG RawVector;
    BOOLEAN FoundBasePort = FALSE, FoundIrq = FALSE;
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    ASSERT(DeviceExtension->State == dsStopped);

    if (!IoForwardIrpSynchronously(DeviceExtension->Ldo, Irp))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Complete;
    }
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
       DPRINT1("LDO failed to start 0x%X\n", Status);
       goto Complete;
    }

    AllocatedResources = IrpSp->Parameters.StartDevice.AllocatedResources;
    AllocatedResourcesTranslated = IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated;
    if (!AllocatedResources || !AllocatedResourcesTranslated)
    {
        DPRINT1("No allocated resources\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Complete;
    }

    if (AllocatedResources->Count != 1)
        DPRINT1("Expected FullList count is 1, got %d\n", AllocatedResources->Count);

    for (i = 0; i < AllocatedResources->List[0].PartialResourceList.Count; i++)
    {
        Descriptor = &AllocatedResources->List[0].PartialResourceList.PartialDescriptors[i];
        DescriptorTranslated = &AllocatedResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];

        switch (Descriptor->Type)
        {
            case CmResourceTypePort:
            {
                DPRINT("[%p:%X:%X] I/O ports at [%p-%p]\n",
                       Descriptor,
                       Descriptor->ShareDisposition,
                       Descriptor->Flags,
                       Descriptor->u.Port.Start.LowPart,
                       Descriptor->u.Port.Start.LowPart + (Descriptor->u.Port.Length - 1));

                if (!FoundBasePort)
                {
                    DeviceExtension->IoBase = ULongToPtr(Descriptor->u.Port.Start.u.LowPart);

                    FoundBasePort = TRUE;
                }

                break;
            }

            case CmResourceTypeInterrupt:
            {
                DPRINT("[%p:%X:%X] INT Vec %d Lev %d Aff %IX\n",
                       Descriptor,
                       Descriptor->ShareDisposition,
                       Descriptor->Flags,
                       Descriptor->u.Interrupt.Vector,
                       Descriptor->u.Interrupt.Level,
                       Descriptor->u.Interrupt.Affinity);

                if (!FoundIrq)
                {
                    DeviceExtension->InterruptVector = DescriptorTranslated->u.Interrupt.Vector;
                    DeviceExtension->InterruptLevel = (KIRQL)DescriptorTranslated->u.Interrupt.Level;
                    if (DescriptorTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
                        DeviceExtension->InterruptMode = Latched;
                    else
                        DeviceExtension->InterruptMode = LevelSensitive;
                    DeviceExtension->InterruptShared = (DescriptorTranslated->ShareDisposition == CmResourceShareShared);
                    DeviceExtension->InterruptAffinity = DescriptorTranslated->u.Interrupt.Affinity;
                    RawVector = Descriptor->u.Interrupt.Vector;

                    FoundIrq = TRUE;
                }

                break;
            }

            default:
                DPRINT("[%p:%X:%X] Unrecognized resource type %X\n",
                       Descriptor,
                       Descriptor->ShareDisposition,
                       Descriptor->Flags,
                       Descriptor->Type);
                break;
        }
    }

    if (!FoundBasePort || !FoundIrq)
    {
        DPRINT1("The device resources were not found\n");
        Status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Complete;
    }

    DPRINT("I/O base at %p\n", DeviceExtension->IoBase);
    DPRINT("IRQ %d\n", RawVector);

    Status = InPortWmiRegistration(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WMI registration failed 0x%X\n", Status);
        goto Complete;
    }

    InPortInitializeMouse(DeviceExtension);

    Status = IoConnectInterrupt(&DeviceExtension->InterruptObject,
                                InPortIsr,
                                DeviceExtension,
                                NULL,
                                DeviceExtension->InterruptVector,
                                DeviceExtension->InterruptLevel,
                                DeviceExtension->InterruptLevel,
                                DeviceExtension->InterruptMode,
                                DeviceExtension->InterruptShared,
                                DeviceExtension->InterruptAffinity,
                                FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not connect to interrupt %d\n", DeviceExtension->InterruptVector);
        goto Complete;
    }

    KeSynchronizeExecution(DeviceExtension->InterruptObject,
                           InPortStartMouse,
                           DeviceExtension);

    DeviceExtension->State = dsStarted;

Complete:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
InPortRemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    BOOLEAN IsStarted;
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    InPortWmiDeRegistration(DeviceExtension);

    IsStarted = (DeviceExtension->State == dsStarted);

    DeviceExtension->State = dsRemoved;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(DeviceExtension->Ldo, Irp);

    IoReleaseRemoveLockAndWait(&DeviceExtension->RemoveLock, Irp);

    /* Device is active */
    if (IsStarted)
    {
        KeSynchronizeExecution(DeviceExtension->InterruptObject,
                               InPortStopMouse,
                               DeviceExtension);

        IoDisconnectInterrupt(DeviceExtension->InterruptObject);

        /* Flush DPC for ISR */
        KeFlushQueuedDpcs();
    }

    IoDetachDevice(DeviceExtension->Ldo);
    IoDeleteDevice(DeviceObject);

    return Status;
}

NTSTATUS
NTAPI
InPortPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    DPRINT("%s(%p, %p) %X\n",
           __FUNCTION__, DeviceObject, Irp, IrpSp->MinorFunction);

    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            Status = InPortStartDevice(DeviceObject, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
            return InPortRemoveDevice(DeviceObject, Irp);

        case IRP_MN_QUERY_STOP_DEVICE:
            /* Device cannot work with other resources */
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->Ldo, Irp);
            break;

        default:
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->Ldo, Irp);
            break;
    }

    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);

    return Status;
}
