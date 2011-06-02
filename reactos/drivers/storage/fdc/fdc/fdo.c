/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/storage/fdc/fdc/fdo.c
 * PURPOSE:         Floppy class driver FDO functions
 *
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <wdm.h>

#include "fdc.h"

#define NDEBUG
#include <debug.h>

static IO_COMPLETION_ROUTINE ForwardIrpAndWaitCompletion;

static NTSTATUS NTAPI
ForwardIrpAndWaitCompletion(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp,
                            IN PVOID Context)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	if (Irp->PendingReturned)
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS NTAPI
ForwardIrpAndWait(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
	KEVENT Event;
	NTSTATUS Status;
	PDEVICE_OBJECT LowerDevice = ((PFDC_FDO_EXTENSION)DeviceObject->DeviceExtension)->Ldo;
	ASSERT(LowerDevice);
    
	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);
    
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

static NTSTATUS
AddFloppyDiskDevice(PFDC_FDO_EXTENSION DevExt)
{
    NTSTATUS Status;
    PFDC_PDO_EXTENSION PdoDevExt;
    PDEVICE_OBJECT DeviceObject;
    
    Status = IoCreateDevice(DevExt->Common.DriverObject,
                            sizeof(FDC_PDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create PDO device (Status: 0x%x)\n", Status);
        return Status;
    }
    
    PdoDevExt = DeviceObject->DeviceExtension;
    
    PdoDevExt->Common.IsFDO = FALSE;
    PdoDevExt->Common.DeviceObject = DeviceObject;
    PdoDevExt->Common.DriverObject = DevExt->Common.DriverObject;
    PdoDevExt->FdoDevExt = DevExt;
    PdoDevExt->FloppyNumber = DevExt->FloppyDriveListCount++;
    
    ExInterlockedInsertTailList(&DevExt->FloppyDriveList,
                                &PdoDevExt->ListEntry,
                                &DevExt->FloppyDriveListLock);
    
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    return STATUS_SUCCESS;
}

static NTSTATUS
EnumerateDevices(PFDC_FDO_EXTENSION DevExt)
{
    /* FIXME: Hardcoded */
    if (DevExt->FloppyDriveListCount == 0)
        return AddFloppyDiskDevice(DevExt);
    
    return STATUS_SUCCESS;
}

static NTSTATUS
FdcFdoQueryBusRelations(PFDC_FDO_EXTENSION DevExt,
                        PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;
    KIRQL OldIrql;
    ULONG i;
    PFDC_PDO_EXTENSION PdoDevExt;
    PLIST_ENTRY ListEntry;
    NTSTATUS Status;
    
    Status = EnumerateDevices(DevExt);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device enumeration failed (Status: 0x%x)\n", Status);
        return Status;
    }
    
    KeAcquireSpinLock(&DevExt->FloppyDriveListLock, &OldIrql);

    DeviceRelations = ExAllocatePool(NonPagedPool,
                                     sizeof(DEVICE_RELATIONS) + sizeof(DeviceRelations->Objects) *
                                     (DevExt->FloppyDriveListCount - 1));
    if (!DeviceRelations)
    {
        DPRINT1("Failed to allocate memory for device relations\n");
        KeReleaseSpinLock(&DevExt->FloppyDriveListLock, OldIrql);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    DeviceRelations->Count = DevExt->FloppyDriveListCount;
    
    ListEntry = DevExt->FloppyDriveList.Flink;
    i = 0;
    while (ListEntry != &DevExt->FloppyDriveList)
    {
        PdoDevExt = CONTAINING_RECORD(ListEntry, FDC_PDO_EXTENSION, ListEntry);
        
        ObReferenceObject(PdoDevExt->Common.DeviceObject);
        
        DeviceRelations->Objects[i++] = PdoDevExt->Common.DeviceObject;

        ListEntry = ListEntry->Flink;
    }
    
    KeReleaseSpinLock(&DevExt->FloppyDriveListLock, OldIrql);
    
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    
    return STATUS_SUCCESS;
}

static NTSTATUS
FdcFdoStartDevice(PFDC_FDO_EXTENSION DevExt,
                  PIRP Irp,
                  PIO_STACK_LOCATION IrpSp)
{
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    ULONG i;
    
    ResourceList = &IrpSp->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;
    
    DPRINT1("Descriptor count: %d\n", ResourceList->Count);
    
    for (i = 0; i < ResourceList->Count; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor = &ResourceList->PartialDescriptors[i];
        
        if (PartialDescriptor->Type == CmResourceTypeDeviceSpecific)
        {
            RtlCopyMemory(&DevExt->FloppyDeviceData,
                          (PartialDescriptor + 1),
                          sizeof(CM_FLOPPY_DEVICE_DATA));
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS
FdcFdoPnpDispatch(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PFDC_FDO_EXTENSION DevExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = Irp->IoStatus.Status;
    
    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_START_DEVICE:
            DPRINT("Starting FDC FDO\n");
            
            Status = ForwardIrpAndWait(DeviceObject, Irp);
            if (NT_SUCCESS(Status))
            {
                Status = FdcFdoStartDevice(DevExt, Irp, IrpSp);
            }
            
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
            return Status;
        case IRP_MN_STOP_DEVICE:
            DPRINT("Stopping FDC FDO\n");
            /* We don't need to do anything here */
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
            /* We don't care */
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_REMOVE_DEVICE:
            DPRINT("Removing FDC FDO\n");
            
            /* Undo what we did in FdcAddDevice */
            IoDetachDevice(DevExt->Ldo);
            
            IoDeleteDevice(DeviceObject);
            
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_SURPRISE_REMOVAL:
            /* Nothing special to do here to deal with surprise removal */
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IrpSp->Parameters.QueryDeviceRelations.Type == BusRelations)
            {
                Status = FdcFdoQueryBusRelations(DevExt, Irp);
                
                Irp->IoStatus.Status = Status;
                
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                
                return Status;
            }
            break;
    }
    
    Irp->IoStatus.Status = Status;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DevExt->Ldo, Irp);
}

NTSTATUS
FdcFdoPowerDispatch(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PFDC_FDO_EXTENSION DevExt = DeviceObject->DeviceExtension;
    
    DPRINT1("Power request not handled\n");
    
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(DevExt->Ldo, Irp);
}

NTSTATUS
FdcFdoDeviceControlDispatch(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    /* FIXME: We don't handle any of these yet */
    
    DPRINT1("Device control request not handled\n");
    
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Irp->IoStatus.Status;
}

NTSTATUS
FdcFdoInternalDeviceControlDispatch(IN PDEVICE_OBJECT DeviceObject,
                                    IN PIRP Irp)
{
    /* FIXME: We don't handle any of these yet */
    
    DPRINT1("Internal device control request not handled\n");
    
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Irp->IoStatus.Status;
}