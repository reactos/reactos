/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/irp.c
 * PURPOSE:     IRP Handling.
 * PROGRAMMERS:
 *              Michael Martin
 */

#include "usbehci.h"

VOID
RequestCancel (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) PdoDeviceExtension->ControllerFdo->DeviceExtension;

    KIRQL OldIrql = Irp->CancelIrql;
    IoReleaseCancelSpinLock(DISPATCH_LEVEL);

    KeAcquireSpinLockAtDpcLevel(&FdoDeviceExtension->IrpQueueLock);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&FdoDeviceExtension->IrpQueueLock, OldIrql);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
QueueRequest(PFDO_DEVICE_EXTENSION DeviceExtension, PIRP Irp)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &OldIrql);

    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
    {
        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, OldIrql);
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        InsertTailList(&DeviceExtension->IrpQueue, &Irp->Tail.Overlay.ListEntry);
        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, OldIrql);
    }
}

VOID
CompletePendingRequest(PFDO_DEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY NextIrp = NULL;
    PIO_STACK_LOCATION Stack;
    KIRQL oldIrql;
    PIRP Irp = NULL;
    URB *Urb;

    KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &oldIrql);

    while(!IsListEmpty(&DeviceExtension->IrpQueue))
    {
        NextIrp = RemoveHeadList(&DeviceExtension->IrpQueue);
        Irp = CONTAINING_RECORD(NextIrp, IRP, Tail.Overlay.ListEntry);

        if (!Irp)
            break;

        Stack = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(Stack);

        Urb = (PURB) Stack->Parameters.Others.Argument1;
        ASSERT(Urb);

        /* FIXME: Fill in information for Argument1/URB */

        DPRINT("TransferBuffer %x\n", Urb->UrbControlDescriptorRequest.TransferBuffer);
        DPRINT("TransferBufferLength %x\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);
        DPRINT("Index %x\n", Urb->UrbControlDescriptorRequest.Index);
        DPRINT("DescriptorType %x\n",     Urb->UrbControlDescriptorRequest.DescriptorType);    
        DPRINT("LanguageId %x\n", Urb->UrbControlDescriptorRequest.LanguageId);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &oldIrql);
    }

    KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);
}

NTSTATUS
NTAPI
ArrivalNotificationCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID PContext)
{
    PDEVICE_OBJECT PortDeviceObject = (PDEVICE_OBJECT) PContext;
    IoFreeIrp(Irp);
    ObDereferenceObject(PortDeviceObject);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
DeviceArrivalWorkItem(PDEVICE_OBJECT DeviceObject, PVOID Context)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PIO_STACK_LOCATION IrpStack = NULL;
    PDEVICE_OBJECT PortDeviceObject = NULL;
    PIRP Irp = NULL;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)Context;

    PortDeviceObject = IoGetAttachedDeviceReference(FdoDeviceExtension->Pdo);

    if (!PortDeviceObject)
    {
        DPRINT1("Unable to notify Pdos parent of device arrival.\n");
        return;
    }

    if (PortDeviceObject == DeviceObject)
    {
        /* Piontless to send query relations to ourself */
        ObDereferenceObject(PortDeviceObject);
    }

    Irp = IoAllocateIrp(PortDeviceObject->StackSize, FALSE);

    if (!Irp)
    {
        DPRINT1("Unable to allocate IRP\n");
    }

    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)ArrivalNotificationCompletion,
                           (PVOID) PortDeviceObject,
                           TRUE,
                           TRUE,
                           TRUE);

    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    IrpStack->MajorFunction = IRP_MJ_PNP;
    IrpStack->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;

    IoCallDriver(PortDeviceObject, Irp);
}

