/*
 *  ReactOS kernel
 *  Copyright (C) 2011 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/mountmgr/notify.c
 * PURPOSE:          Mount Manager - Notifications handlers
 * PROGRAMMER:       Pierre Schweitzer (pierre.schweitzer@reactos.org)
 *                   Alex Ionescu (alex.ionescu@reactos.org)
 */

#include "mntmgr.h"

#include <ioevent.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
VOID
SendOnlineNotification(IN PUNICODE_STRING SymbolicName)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get device object */
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* And attached device object */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    /* And send VOLUME_ONLINE */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_VOLUME_ONLINE,
                                        DeviceObject,
                                        NULL, 0,
                                        NULL, 0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        goto Cleanup;
    }

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->FileObject = FileObject;

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

Cleanup:
    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);

    return;
}

/*
 * @implemented
 */
VOID
NTAPI
SendOnlineNotificationWorker(IN PVOID Parameter)
{
    KIRQL OldIrql;
    PLIST_ENTRY Head;
    PDEVICE_EXTENSION DeviceExtension;
    PONLINE_NOTIFICATION_WORK_ITEM WorkItem;
    PONLINE_NOTIFICATION_WORK_ITEM NewWorkItem;

    WorkItem = (PONLINE_NOTIFICATION_WORK_ITEM)Parameter;
    DeviceExtension = WorkItem->DeviceExtension;

    /* First, send the notification */
    SendOnlineNotification(&(WorkItem->SymbolicName));

    KeAcquireSpinLock(&(DeviceExtension->WorkerLock), &OldIrql);
    /* If there are no notifications running any longer, reset event */
    if (--DeviceExtension->OnlineNotificationCount == 0)
    {
        KeSetEvent(&(DeviceExtension->OnlineNotificationEvent), 0, FALSE);
    }

    /* If there are still notifications in queue */
    if (!IsListEmpty(&(DeviceExtension->OnlineNotificationListHead)))
    {
        /* Queue a new one for execution */
        Head = RemoveHeadList(&(DeviceExtension->OnlineNotificationListHead));
        NewWorkItem = CONTAINING_RECORD(Head, ONLINE_NOTIFICATION_WORK_ITEM, WorkItem.List);
        KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);
        NewWorkItem->WorkItem.List.Blink = NULL;
        NewWorkItem->WorkItem.List.Flink = NULL;
        ExQueueWorkItem(&NewWorkItem->WorkItem, DelayedWorkQueue);
    }
    else
    {
        /* Mark it's over */
        DeviceExtension->OnlineNotificationWorkerActive = 0;
        KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);
    }

    FreePool(WorkItem->SymbolicName.Buffer);
    FreePool(WorkItem);

    return;
}

/*
 * @implemented
 */
VOID
PostOnlineNotification(IN PDEVICE_EXTENSION DeviceExtension,
                       IN PUNICODE_STRING SymbolicName)
{
    KIRQL OldIrql;
    PONLINE_NOTIFICATION_WORK_ITEM WorkItem;

    /* Allocate a notification work item */
    WorkItem = AllocatePool(sizeof(ONLINE_NOTIFICATION_WORK_ITEM));
    if (!WorkItem)
    {
        return;
    }

    ExInitializeWorkItem(&WorkItem->WorkItem, SendOnlineNotificationWorker, WorkItem);
    WorkItem->DeviceExtension = DeviceExtension;
    WorkItem->SymbolicName.Length = SymbolicName->Length;
    WorkItem->SymbolicName.MaximumLength = SymbolicName->Length + sizeof(WCHAR);
    WorkItem->SymbolicName.Buffer = AllocatePool(WorkItem->SymbolicName.MaximumLength);
    if (!WorkItem->SymbolicName.Buffer)
    {
        FreePool(WorkItem);
        return;
    }

    RtlCopyMemory(WorkItem->SymbolicName.Buffer, SymbolicName->Buffer, SymbolicName->Length);
    WorkItem->SymbolicName.Buffer[SymbolicName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    KeAcquireSpinLock(&(DeviceExtension->WorkerLock), &OldIrql);
    DeviceExtension->OnlineNotificationCount++;

    /* If no worker are active */
    if (DeviceExtension->OnlineNotificationWorkerActive == 0)
    {
        /* Queue that one for execution */
        DeviceExtension->OnlineNotificationWorkerActive = 1;
        ExQueueWorkItem(&WorkItem->WorkItem, DelayedWorkQueue);
    }
    else
    {
        /* Otherwise, just put it in the queue list */
        InsertTailList(&(DeviceExtension->OnlineNotificationListHead), &(WorkItem->WorkItem.List));
    }

    KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);

    return;
}

/*
 * @implemented
 */
VOID
WaitForOnlinesToComplete(IN PDEVICE_EXTENSION DeviceExtension)
{
    KIRQL OldIrql;

    KeInitializeEvent(&(DeviceExtension->OnlineNotificationEvent), NotificationEvent, FALSE);

    KeAcquireSpinLock(&(DeviceExtension->WorkerLock), &OldIrql);

    /* Just wait all the worker are done */
    if (DeviceExtension->OnlineNotificationCount != 1)
    {
        DeviceExtension->OnlineNotificationCount--;
        KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);

        KeWaitForSingleObject(&(DeviceExtension->OnlineNotificationEvent),
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        KeAcquireSpinLock(&(DeviceExtension->WorkerLock), &OldIrql);
        DeviceExtension->OnlineNotificationCount++;
    }

    KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MountMgrTargetDeviceNotification(IN PVOID NotificationStructure,
                                 IN PVOID Context)
{
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_INFORMATION DeviceInformation;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification;

    DeviceInformation = Context;
    DeviceExtension = DeviceInformation->DeviceExtension;
    Notification = NotificationStructure;

    /* If it's to signal that removal is complete, then, execute the function */
    if (IsEqualGUID(&(Notification->Event), &GUID_TARGET_DEVICE_REMOVE_COMPLETE))
    {
        MountMgrMountedDeviceRemoval(DeviceExtension, Notification->SymbolicLinkName);
    }
    /* It it's to signal that a volume has been mounted
     * Verify if a database sync is required and execute it
     */
    else if (IsEqualGUID(&(Notification->Event), &GUID_IO_VOLUME_MOUNT))
    {
        /* If we were already mounted, then mark us unmounted */
        if (InterlockedCompareExchange(&(DeviceInformation->MountState),
                                       FALSE,
                                       FALSE) == TRUE)
        {
            InterlockedDecrement(&(DeviceInformation->MountState));
        }
        /* Otherwise, start mounting the device and first, reconcile its DB if required */
        else
        {
            if (DeviceInformation->NeedsReconcile)
            {
                DeviceInformation->NeedsReconcile = FALSE;
                ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInformation);
            }
        }
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
RegisterForTargetDeviceNotification(IN PDEVICE_EXTENSION DeviceExtension,
                                    IN PDEVICE_INFORMATION DeviceInformation)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;

    /* Get device object */
    Status = IoGetDeviceObjectPointer(&(DeviceInformation->DeviceName),
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* And simply register for notifications */
    Status = IoRegisterPlugPlayNotification(EventCategoryTargetDeviceChange,
                                            0, FileObject,
                                            DeviceExtension->DriverObject,
                                            MountMgrTargetDeviceNotification,
                                            DeviceInformation,
                                            &(DeviceInformation->TargetDeviceNotificationEntry));
    if (!NT_SUCCESS(Status))
    {
        DeviceInformation->TargetDeviceNotificationEntry = NULL;
    }

    ObDereferenceObject(FileObject);

    return;
}

/*
 * @implemented
 */
VOID
MountMgrNotify(IN PDEVICE_EXTENSION DeviceExtension)
{
    PIRP Irp;
    KIRQL OldIrql;
    LIST_ENTRY CopyList;
    PLIST_ENTRY NextEntry;

    /* Increase the epic number */
    DeviceExtension->EpicNumber++;

    InitializeListHead(&CopyList);

    /* Copy all the pending IRPs for notification */
    IoAcquireCancelSpinLock(&OldIrql);
    while (!IsListEmpty(&(DeviceExtension->IrpListHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceExtension->IrpListHead));
        Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);
        IoSetCancelRoutine(Irp, NULL);
        InsertTailList(&CopyList, &(Irp->Tail.Overlay.ListEntry));
    }
    IoReleaseCancelSpinLock(OldIrql);

    /* Then, notify them one by one */
    while (!IsListEmpty(&CopyList))
    {
        NextEntry = RemoveHeadList(&CopyList);
        Irp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

        *((PULONG)Irp->AssociatedIrp.SystemBuffer) = DeviceExtension->EpicNumber;
        Irp->IoStatus.Information = sizeof(DeviceExtension->EpicNumber);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

/*
 * @implemented
 */
VOID
MountMgrNotifyNameChange(IN PDEVICE_EXTENSION DeviceExtension,
                         IN PUNICODE_STRING DeviceName,
                         IN BOOLEAN ValidateVolume)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_INFORMATION DeviceInformation;
    TARGET_DEVICE_CUSTOM_NOTIFICATION DeviceNotification;

    /* If we have to validate volume */
    if (ValidateVolume)
    {
        /* Then, ensure we can find the device */
        for (NextEntry = DeviceExtension->DeviceListHead.Flink;
             NextEntry != &DeviceExtension->DeviceListHead;
             NextEntry = NextEntry->Flink)
        {
            DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);
            if (RtlCompareUnicodeString(DeviceName, &(DeviceInformation->DeviceName), TRUE) == 0)
            {
                break;
            }
        }

        /* No need to notify for a PnP device or if we didn't find the device */
        if (NextEntry == &(DeviceExtension->DeviceListHead) ||
            !DeviceInformation->ManuallyRegistered)
        {
            return;
        }
    }

    /* Then, get device object */
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Set up empty IRP (yes, yes!) */
    Irp = IoBuildDeviceIoControlRequest(0,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return;
    }

    Stack = IoGetNextIrpStackLocation(Irp);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    /* Properly set it, we want to query device relations */
    Stack->MajorFunction = IRP_MJ_PNP;
    Stack->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    Stack->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    Stack->FileObject = FileObject;

    /* And call driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);

    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Validate device return */
    DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
    if (DeviceRelations->Count < 1)
    {
        ExFreePool(DeviceRelations);
        return;
    }

    DeviceObject = DeviceRelations->Objects[0];
    ExFreePool(DeviceRelations);

    /* Set up real notification */
    DeviceNotification.Version = 1;
    DeviceNotification.Size = sizeof(TARGET_DEVICE_CUSTOM_NOTIFICATION);
    DeviceNotification.Event = GUID_IO_VOLUME_NAME_CHANGE;
    DeviceNotification.FileObject = NULL;
    DeviceNotification.NameBufferOffset = -1;

    /* And report */
    IoReportTargetDeviceChangeAsynchronous(DeviceObject,
                                           &DeviceNotification,
                                           NULL, NULL);

    ObDereferenceObject(DeviceObject);

    return;
}

/*
 * @implemented
 */
VOID
RemoveWorkItem(IN PUNIQUE_ID_WORK_ITEM WorkItem)
{
    PDEVICE_EXTENSION DeviceExtension = WorkItem->DeviceExtension;

    KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);

    /* If even if being worked, it's too late */
    if (WorkItem->Event)
    {
        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);
        KeSetEvent(WorkItem->Event, 0, FALSE);
    }
    else
    {
        /* Otherwise, remove it from the list, and delete it */
        RemoveEntryList(&(WorkItem->UniqueIdWorkerItemListEntry));
        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);
        IoFreeIrp(WorkItem->Irp);
        FreePool(WorkItem->DeviceName.Buffer);
        FreePool(WorkItem->IrpBuffer);
        FreePool(WorkItem);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
UniqueIdChangeNotifyWorker(IN PDEVICE_OBJECT DeviceObject,
                           IN PVOID Context)
{
    PUNIQUE_ID_WORK_ITEM WorkItem = Context;
    PMOUNTDEV_UNIQUE_ID OldUniqueId, NewUniqueId;
    PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT UniqueIdChange;

    UNREFERENCED_PARAMETER(DeviceObject);

    /* Validate worker */
    if (!NT_SUCCESS(WorkItem->Irp->IoStatus.Status))
    {
        RemoveWorkItem(WorkItem);
        return;
    }

    UniqueIdChange = WorkItem->Irp->AssociatedIrp.SystemBuffer;
    /* Get the old unique ID */
    OldUniqueId = AllocatePool(UniqueIdChange->OldUniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
    if (!OldUniqueId)
    {
        RemoveWorkItem(WorkItem);
        return;
    }

    OldUniqueId->UniqueIdLength = UniqueIdChange->OldUniqueIdLength;
    RtlCopyMemory(OldUniqueId->UniqueId,
                  (PVOID)((ULONG_PTR)UniqueIdChange + UniqueIdChange->OldUniqueIdOffset),
                  UniqueIdChange->OldUniqueIdLength);

    /* Get the new unique ID */
    NewUniqueId = AllocatePool(UniqueIdChange->NewUniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
    if (!NewUniqueId)
    {
        FreePool(OldUniqueId);
        RemoveWorkItem(WorkItem);
        return;
    }

    NewUniqueId->UniqueIdLength = UniqueIdChange->NewUniqueIdLength;
    RtlCopyMemory(NewUniqueId->UniqueId,
                  (PVOID)((ULONG_PTR)UniqueIdChange + UniqueIdChange->NewUniqueIdOffset),
                  UniqueIdChange->NewUniqueIdLength);

    /* Call the real worker */
    MountMgrUniqueIdChangeRoutine(WorkItem->DeviceExtension, OldUniqueId, NewUniqueId);
    IssueUniqueIdChangeNotifyWorker(WorkItem, NewUniqueId);

    FreePool(NewUniqueId);
    FreePool(OldUniqueId);

    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
UniqueIdChangeNotifyCompletion(IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP Irp,
                               IN PVOID Context)
{
    PUNIQUE_ID_WORK_ITEM WorkItem = Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    /* Simply queue the work item */
    IoQueueWorkItem(WorkItem->WorkItem,
                    UniqueIdChangeNotifyWorker,
                    DelayedWorkQueue,
                    WorkItem);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*
 * @implemented
 */
VOID
IssueUniqueIdChangeNotifyWorker(IN PUNIQUE_ID_WORK_ITEM WorkItem,
                                IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    PIRP Irp;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    PDEVICE_OBJECT DeviceObject;

    /* Get the device object */
    Status = IoGetDeviceObjectPointer(&(WorkItem->DeviceName),
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        RemoveWorkItem(WorkItem);
        return;
    }

    /* And then, the attached device */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    /* Initialize the IRP */
    Irp = WorkItem->Irp;
    IoInitializeIrp(Irp, IoSizeOfIrp(WorkItem->StackSize), (CCHAR)WorkItem->StackSize);

    if (InterlockedExchange((PLONG)&(WorkItem->Event), 0) != 0)
    {
        ObDereferenceObject(FileObject);
        ObDereferenceObject(DeviceObject);
        RemoveWorkItem(WorkItem);
        return;
    }

    Irp->AssociatedIrp.SystemBuffer = WorkItem->IrpBuffer;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, UniqueId, UniqueId->UniqueIdLength + sizeof(USHORT));

    Stack = IoGetNextIrpStackLocation(Irp);

    Stack->Parameters.DeviceIoControl.InputBufferLength = UniqueId->UniqueIdLength + sizeof(USHORT);
    Stack->Parameters.DeviceIoControl.OutputBufferLength = WorkItem->IrpBufferLength;
    Stack->Parameters.DeviceIoControl.Type3InputBuffer = 0;
    Stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY;
    Stack->MajorFunction = IRP_MJ_DEVICE_CONTROL;

    Status = IoSetCompletionRoutineEx(WorkItem->DeviceExtension->DeviceObject,
                                      Irp,
                                      UniqueIdChangeNotifyCompletion,
                                      WorkItem,
                                      TRUE, TRUE, TRUE);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(FileObject);
        ObDereferenceObject(DeviceObject);
        RemoveWorkItem(WorkItem);
        return;
    }

    /* Call the driver */
    IoCallDriver(DeviceObject, Irp);
    ObDereferenceObject(FileObject);
    ObDereferenceObject(DeviceObject);
}

/*
 * @implemented
 */
VOID
IssueUniqueIdChangeNotify(IN PDEVICE_EXTENSION DeviceExtension,
                          IN PUNICODE_STRING DeviceName,
                          IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    NTSTATUS Status;
    PVOID IrpBuffer = NULL;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PUNIQUE_ID_WORK_ITEM WorkItem = NULL;

    /* Get the associated device object */
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* And then, get attached device */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    ObDereferenceObject(FileObject);

    /* Allocate a work item */
    WorkItem = AllocatePool(sizeof(UNIQUE_ID_WORK_ITEM));
    if (!WorkItem)
    {
        ObDereferenceObject(DeviceObject);
        return;
    }

    WorkItem->Event = NULL;
    WorkItem->WorkItem = IoAllocateWorkItem(DeviceExtension->DeviceObject);
    if (!WorkItem->WorkItem)
    {
        ObDereferenceObject(DeviceObject);
        goto Cleanup;
    }

    WorkItem->DeviceExtension = DeviceExtension;
    WorkItem->StackSize = DeviceObject->StackSize;
    /* Already provide the IRP */
    WorkItem->Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    ObDereferenceObject(DeviceObject);

    if (!WorkItem->Irp)
    {
        goto Cleanup;
    }

    /* Ensure it has enough space */
    IrpBuffer = AllocatePool(sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT) + 1024);
    if (!IrpBuffer)
    {
        goto Cleanup;
    }

    WorkItem->DeviceName.Length = DeviceName->Length;
    WorkItem->DeviceName.MaximumLength = DeviceName->Length + sizeof(WCHAR);
    WorkItem->DeviceName.Buffer = AllocatePool(WorkItem->DeviceName.MaximumLength);
    if (!WorkItem->DeviceName.Buffer)
    {
        goto Cleanup;
    }

    RtlCopyMemory(WorkItem->DeviceName.Buffer, DeviceName->Buffer, DeviceName->Length);
    WorkItem->DeviceName.Buffer[DeviceName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    WorkItem->IrpBuffer = IrpBuffer;
    WorkItem->IrpBufferLength = sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT) + 1024;

    /* Add the worker in the list */
    KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);
    InsertHeadList(&(DeviceExtension->UniqueIdWorkerItemListHead), &(WorkItem->UniqueIdWorkerItemListEntry));
    KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

    /* And call the worker */
    IssueUniqueIdChangeNotifyWorker(WorkItem, UniqueId);

    return;

Cleanup:
    if (IrpBuffer)
    {
        FreePool(IrpBuffer);
    }

    if (WorkItem->Irp)
    {
        IoFreeIrp(WorkItem->Irp);
    }

    if (WorkItem->WorkItem)
    {
        IoFreeWorkItem(WorkItem->WorkItem);
    }

    if (WorkItem)
    {
        FreePool(WorkItem);
    }
}
