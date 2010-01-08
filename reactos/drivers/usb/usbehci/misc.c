
#include "usbehci.h"

VOID
QueueRequest(PFDO_DEVICE_EXTENSION DeviceExtension, PIRP Irp)
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &oldIrql);
    InsertTailList(&DeviceExtension->IrpQueue, &Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);
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

    /* No Irps in Queue? */
    if (IsListEmpty(&DeviceExtension->IrpQueue))
    {
        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);
        return;
    }

    NextIrp = RemoveHeadList(&DeviceExtension->IrpQueue);
    while(TRUE)
    {
        Irp = CONTAINING_RECORD(NextIrp, IRP, Tail.Overlay.ListEntry);

        if (!Irp)
            break;

        /* FIXME: Handle cancels */
        /*if (!IoSetCancelRoutine(Irp, NULL))
        {

        }*/


        Stack = IoGetCurrentIrpStackLocation(Irp);

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
    IoFreeIrp(Irp);
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

    Irp = IoAllocateIrp(PortDeviceObject->StackSize, FALSE);

    if (!Irp)
    {
        DPRINT1("Unable to allocate IRP\n");
    }

    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)ArrivalNotificationCompletion,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    IrpStack->MajorFunction = IRP_MJ_PNP;
    IrpStack->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;

    IoCallDriver(PortDeviceObject, Irp);
}

/*
   Get SymblicName from Parameters in Registry Key
   Caller is responsible for freeing pool of returned pointer
*/
PWSTR
GetSymbolicName(PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    HANDLE DevInstRegKey;
    UNICODE_STRING SymbolicName;
    PKEY_VALUE_PARTIAL_INFORMATION KeyPartInfo;
    ULONG SizeNeeded;
    PWCHAR SymbolicNameString = NULL;

    Status = IoOpenDeviceRegistryKey(DeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &DevInstRegKey);

    DPRINT("IoOpenDeviceRegistryKey PLUGPLAY_REGKEY_DEVICE Status %x\n", Status);

    if (NT_SUCCESS(Status))
    {
        RtlInitUnicodeString(&SymbolicName, L"SymbolicName");
        Status = ZwQueryValueKey(DevInstRegKey,
                                 &SymbolicName,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &SizeNeeded);

        DPRINT("ZwQueryValueKey status %x, %d\n", Status, SizeNeeded);

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            KeyPartInfo = (PKEY_VALUE_PARTIAL_INFORMATION ) ExAllocatePool(PagedPool, SizeNeeded);
            if (!KeyPartInfo)
            {
                DPRINT1("OUT OF MEMORY\n");
                return NULL;
            }
            else
            {
                 Status = ZwQueryValueKey(DevInstRegKey,
                                          &SymbolicName,
                                          KeyValuePartialInformation,
                                          KeyPartInfo,
                                          SizeNeeded,
                                          &SizeNeeded);

                 SymbolicNameString = ExAllocatePool(PagedPool, (KeyPartInfo->DataLength + sizeof(WCHAR)));
                 if (!SymbolicNameString)
                 {
                     return NULL;
                 }
                 RtlZeroMemory(SymbolicNameString, KeyPartInfo->DataLength + 2);
                 RtlCopyMemory(SymbolicNameString, KeyPartInfo->Data, KeyPartInfo->DataLength);
            }

            ExFreePool(KeyPartInfo);
        }

        ZwClose(DevInstRegKey);
    }

    return SymbolicNameString;
}

/*
   Get Physical Device Object Name from registry
   Caller is responsible for freeing pool
*/
PWSTR
GetPhysicalDeviceObjectName(PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PWSTR ObjectName = NULL;
    ULONG SizeNeeded;

    Status = IoGetDeviceProperty(DeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 0,
                                 NULL,
                                 &SizeNeeded);

    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPRINT1("Expected STATUS_BUFFER_TOO_SMALL, got %x!\n", Status);
        return NULL;
    }

    ObjectName = (PWSTR) ExAllocatePool(PagedPool, SizeNeeded + sizeof(WCHAR));
    if (!ObjectName)
    {
        DPRINT1("Out of memory\n");
        return NULL;
    }

    Status = IoGetDeviceProperty(DeviceObject,
                                 DevicePropertyPhysicalDeviceObjectName,
                                 SizeNeeded,
                                 ObjectName,
                                 &SizeNeeded);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to Get Property\n");
        return NULL;
    }

    return ObjectName;
}

