/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/volume.c
 * PURPOSE:         Volume and File System I/O Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Eric Kohl
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ERESOURCE IopDatabaseResource;
LIST_ENTRY IopDiskFileSystemQueueHead, IopNetworkFileSystemQueueHead;
LIST_ENTRY IopCdRomFileSystemQueueHead, IopTapeFileSystemQueueHead;
LIST_ENTRY IopFsNotifyChangeQueueHead;
ULONG IopFsRegistrationOps;

/* PRIVATE FUNCTIONS *********************************************************/

/*
 * @halfplemented
 */
VOID
NTAPI
IopDecrementDeviceObjectRef(IN PDEVICE_OBJECT DeviceObject,
                            IN BOOLEAN UnloadIfUnused)
{
    KIRQL OldIrql;

    /* Acquire lock */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);
    ASSERT(DeviceObject->ReferenceCount > 0);

    if (--DeviceObject->ReferenceCount > 0)
    {
        KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
        return;
    }

    /* Release lock */
    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);

    /* Here, DO is not referenced any longer, check if we have to unload it */
    if (UnloadIfUnused || IoGetDevObjExtension(DeviceObject)->ExtensionFlags &
                          (DOE_UNLOAD_PENDING | DOE_DELETE_PENDING | DOE_REMOVE_PENDING))
    {
        /* Unload the driver */
        IopUnloadDevice(DeviceObject);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
IopDecrementDeviceObjectHandleCount(IN PDEVICE_OBJECT DeviceObject)
{
    /* Just decrease reference count */
    IopDecrementDeviceObjectRef(DeviceObject, FALSE);
}

/*
 * @implemented
 */
PVPB
NTAPI
IopCheckVpbMounted(IN POPEN_PACKET OpenPacket,
                   IN PDEVICE_OBJECT DeviceObject,
                   IN PUNICODE_STRING RemainingName,
                   OUT PNTSTATUS Status)
{
    BOOLEAN Alertable, Raw;
    KIRQL OldIrql;
    PVPB Vpb = NULL;

    /* Lock the VPBs */
    IoAcquireVpbSpinLock(&OldIrql);

    /* Set VPB mount settings */
    Raw = !RemainingName->Length && !OpenPacket->RelatedFileObject;
    Alertable = (OpenPacket->CreateOptions & FILE_SYNCHRONOUS_IO_ALERT) ?
                TRUE: FALSE;

    /* Start looping until the VPB is mounted */
    while (!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
    {
        /* Release the lock */
        IoReleaseVpbSpinLock(OldIrql);

        /* Mount the volume */
        *Status = IopMountVolume(DeviceObject,
                                 Raw,
                                 FALSE,
                                 Alertable,
                                 &Vpb);

        /* Check if we failed or if we were alerted */
        if (!(NT_SUCCESS(*Status)) ||
            (*Status == STATUS_USER_APC) ||
            (*Status == STATUS_ALERTED))
        {
            /* Dereference the device, since IopParseDevice referenced it */
            IopDereferenceDeviceObject(DeviceObject, FALSE);

            /* Check if it was a total failure */
            if (!NT_SUCCESS(*Status)) return NULL;

            /* Otherwise we were alerted */
            *Status = STATUS_WRONG_VOLUME;
            return NULL;
        }
        /*
         * In case IopMountVolume returns a valid VPB
         * Then, the volume is mounted, return it
         */
        else if (Vpb != NULL)
        {
            return Vpb;
        }

        /* Re-acquire the lock */
        IoAcquireVpbSpinLock(&OldIrql);
    }

    /* Make sure the VPB isn't locked */
    Vpb = DeviceObject->Vpb;
    if (Vpb->Flags & VPB_LOCKED)
    {
        /* We're locked, so fail */
        *Status = STATUS_ACCESS_DENIED;
        Vpb = NULL;
    }
    else
    {
        /* Success! Reference the VPB */
        Vpb->ReferenceCount++;
    }

    /* Release the lock and return the VPB */
    IoReleaseVpbSpinLock(OldIrql);
    return Vpb;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IopCreateVpb(IN PDEVICE_OBJECT DeviceObject)
{
    PVPB Vpb;

    /* Allocate the Vpb */
    Vpb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(VPB),
                                TAG_VPB);
    if (!Vpb) return STATUS_INSUFFICIENT_RESOURCES;

    /* Clear it so we don't waste time manually */
    RtlZeroMemory(Vpb, sizeof(VPB));

    /* Set the Header and Device Field */
    Vpb->Type = IO_TYPE_VPB;
    Vpb->Size = sizeof(VPB);
    Vpb->RealDevice = DeviceObject;

    /* Link it to the Device Object */
    DeviceObject->Vpb = Vpb;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IopDereferenceVpbAndFree(IN PVPB Vpb)
{
    KIRQL OldIrql;

    /* Lock the VPBs and decrease references */
    IoAcquireVpbSpinLock(&OldIrql);
    Vpb->ReferenceCount--;

    /* Check if we're out of references */
    if (!Vpb->ReferenceCount && Vpb->RealDevice->Vpb == Vpb &&
        !(Vpb->Flags & VPB_PERSISTENT))
    {
        /* Release VPB lock */
        IoReleaseVpbSpinLock(OldIrql);

        /* And free VPB */
        ExFreePoolWithTag(Vpb, TAG_VPB);
    }
    else
    {
        /* Release VPB lock */
        IoReleaseVpbSpinLock(OldIrql);
    }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IopReferenceVerifyVpb(IN PDEVICE_OBJECT DeviceObject,
                      OUT PDEVICE_OBJECT *FileSystemObject,
                      OUT PVPB *Vpb)
{
    KIRQL OldIrql;
    PVPB LocalVpb;
    BOOLEAN Result = FALSE;

    /* Lock the VPBs and assume failure */
    IoAcquireVpbSpinLock(&OldIrql);
    *Vpb = NULL;
    *FileSystemObject = NULL;

    /* Get the VPB and make sure it's mounted */
    LocalVpb = DeviceObject->Vpb;
    if ((LocalVpb) && (LocalVpb->Flags & VPB_MOUNTED))
    {
        /* Return it */
        *Vpb = LocalVpb;
        *FileSystemObject = LocalVpb->DeviceObject;

        /* Reference it */
        LocalVpb->ReferenceCount++;
        Result = TRUE;
    }

    /* Release the VPB lock and return status */
    IoReleaseVpbSpinLock(OldIrql);
    return Result;
}

PVPB
NTAPI
IopMountInitializeVpb(IN PDEVICE_OBJECT DeviceObject,
                      IN PDEVICE_OBJECT AttachedDeviceObject,
                      IN BOOLEAN Raw)
{
    KIRQL OldIrql;
    PVPB Vpb;

    /* Lock the VPBs */
    IoAcquireVpbSpinLock(&OldIrql);
    Vpb = DeviceObject->Vpb;

    /* Set the VPB as mounted and possibly raw */
    Vpb->Flags |= VPB_MOUNTED | (Raw ? VPB_RAW_MOUNT : 0);

    /* Set the stack size */
    Vpb->DeviceObject->StackSize = AttachedDeviceObject->StackSize;

    /* Add one for the FS Driver */
    Vpb->DeviceObject->StackSize++;

    /* Set the VPB in the device extension */
    IoGetDevObjExtension(Vpb->DeviceObject)->Vpb = Vpb;

    /* Reference it */
    Vpb->ReferenceCount++;

    /* Release the VPB lock and return it */
    IoReleaseVpbSpinLock(OldIrql);
    return Vpb;
}

/*
 * @implemented
 */
FORCEINLINE
VOID
IopNotifyFileSystemChange(IN PDEVICE_OBJECT DeviceObject,
                          IN BOOLEAN DriverActive)
{
    PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;
    PLIST_ENTRY ListEntry;

    /* Loop the list */
    ListEntry = IopFsNotifyChangeQueueHead.Flink;
    while (ListEntry != &IopFsNotifyChangeQueueHead)
    {
        /* Get the entry */
        ChangeEntry = CONTAINING_RECORD(ListEntry,
                                        FS_CHANGE_NOTIFY_ENTRY,
                                        FsChangeNotifyList);

        /* Call the notification procedure */
        ChangeEntry->FSDNotificationProc(DeviceObject, DriverActive);

        /* Go to the next entry */
        ListEntry = ListEntry->Flink;
    }
}

/*
 * @implemented
 */
ULONG
FASTCALL
IopInterlockedIncrementUlong(IN KSPIN_LOCK_QUEUE_NUMBER Queue,
                             IN PULONG Ulong)
{
    KIRQL Irql;
    ULONG OldValue;

    Irql = KeAcquireQueuedSpinLock(Queue);
    OldValue = (*Ulong)++;
    KeReleaseQueuedSpinLock(Queue, Irql);

    return OldValue;
}

/*
 * @implemented
 */
ULONG
FASTCALL
IopInterlockedDecrementUlong(IN KSPIN_LOCK_QUEUE_NUMBER Queue,
                             IN PULONG Ulong)
{
    KIRQL Irql;
    ULONG OldValue;

    Irql = KeAcquireQueuedSpinLock(Queue);
    OldValue = (*Ulong)--;
    KeReleaseQueuedSpinLock(Queue, Irql);

    return OldValue;
}

/*
 * @implemented
 */
VOID
NTAPI
IopShutdownBaseFileSystems(IN PLIST_ENTRY ListHead)
{
    PLIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK StatusBlock;
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Get the first entry and start looping */
    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead)
    {
        /* Get the device object */
        DeviceObject = CONTAINING_RECORD(ListEntry,
                                         DEVICE_OBJECT,
                                         Queue.ListEntry);

        /* Go to the next entry */
        ListEntry = ListEntry->Flink;

        /* Get the attached device */
        DeviceObject = IoGetAttachedDevice(DeviceObject);

        ObReferenceObject(DeviceObject);
        IopInterlockedIncrementUlong(LockQueueIoDatabaseLock, (PULONG)&DeviceObject->ReferenceCount);

        /* Build the shutdown IRP and call the driver */
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
                                           DeviceObject,
                                           NULL,
                                           0,
                                           NULL,
                                           &Event,
                                           &StatusBlock);
        if (Irp)
        {
            Status = IoCallDriver(DeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                /* Wait on the driver */
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            }
        }

        /* Reset the event */
        KeClearEvent(&Event);

        IopDecrementDeviceObjectRef(DeviceObject, FALSE);
        ObDereferenceObject(DeviceObject);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
IopLoadFileSystemDriver(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION StackPtr;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    PDEVICE_OBJECT AttachedDeviceObject = DeviceObject;
    PAGED_CODE();

    /* Loop as long as we're attached */
    while (AttachedDeviceObject->AttachedDevice)
    {
        /* Get the attached device object */
        AttachedDeviceObject = AttachedDeviceObject->AttachedDevice;
    }

    /* Initialize the event and build the IRP */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IRP_MJ_DEVICE_CONTROL,
                                        AttachedDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (Irp)
    {
        /* Set the major and minor functions */
        StackPtr = IoGetNextIrpStackLocation(Irp);
        StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
        StackPtr->MinorFunction = IRP_MN_LOAD_FILE_SYSTEM;

        /* Call the driver */
        Status = IoCallDriver(AttachedDeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait on it */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
    }

    /* Dereference DO - FsRec? - Comment out call, since it breaks up 2nd stage boot, needs more research. */
//  IopDecrementDeviceObjectRef(AttachedDeviceObject, TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IopMountVolume(IN PDEVICE_OBJECT DeviceObject,
               IN BOOLEAN AllowRawMount,
               IN BOOLEAN DeviceIsLocked,
               IN BOOLEAN Alertable,
               OUT PVPB *Vpb)
{
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PLIST_ENTRY FsList, ListEntry;
    LIST_ENTRY LocalList;
    PDEVICE_OBJECT AttachedDeviceObject = DeviceObject;
    PDEVICE_OBJECT FileSystemDeviceObject, ParentFsDeviceObject;
    ULONG FsStackOverhead, RegistrationOps;
    PAGED_CODE();

    /* Check if the device isn't already locked */
    if (!DeviceIsLocked)
    {
        /* Lock it ourselves */
        Status = KeWaitForSingleObject(&DeviceObject->DeviceLock,
                                       Executive,
                                       KeGetPreviousMode(),
                                       Alertable,
                                       NULL);
        if ((Status == STATUS_ALERTED) || (Status == STATUS_USER_APC))
        {
            /* Don't mount if we were interrupted */
            return Status;
        }
    }

    /* Acquire the FS Lock*/
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&IopDatabaseResource, TRUE);

    /* Make sure we weren't already mounted */
    if (!(DeviceObject->Vpb->Flags & (VPB_MOUNTED | VPB_REMOVE_PENDING)))
    {
        /* Initialize the event to wait on */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* Remove the verify flag and get the actual device to mount */
        DeviceObject->Flags &= ~DO_VERIFY_VOLUME;
        while (AttachedDeviceObject->AttachedDevice)
        {
            /* Get the next one */
            AttachedDeviceObject = AttachedDeviceObject->AttachedDevice;
        }

        /* Reference it */
        ObReferenceObject(AttachedDeviceObject);

        /* For a mount operation, this can only be a Disk, CD-ROM or tape */
        if ((DeviceObject->DeviceType == FILE_DEVICE_DISK) ||
            (DeviceObject->DeviceType == FILE_DEVICE_VIRTUAL_DISK))
        {
            /* Use the disk list */
            FsList = &IopDiskFileSystemQueueHead;
        }
        else if (DeviceObject->DeviceType == FILE_DEVICE_CD_ROM)
        {
            /* Use the CD-ROM list */
            FsList = &IopCdRomFileSystemQueueHead;
        }
        else
        {
            /* It's gotta be a tape... */
            FsList = &IopTapeFileSystemQueueHead;
        }

        /* Now loop the fs list until one of the file systems accepts us */
        Status = STATUS_UNSUCCESSFUL;
        ListEntry = FsList->Flink;
        while ((ListEntry != FsList) && !(NT_SUCCESS(Status)))
        {
            /*
             * If we're not allowed to mount this volume and this is our last
             * (but not only) chance to mount it...
             */
            if (!(AllowRawMount) &&
                (ListEntry->Flink == FsList) &&
                (ListEntry != FsList->Flink))
            {
                /* Then fail this mount request */
                break;
            }

            /*
             * Also check if this is a raw mount and there are other file
             * systems on the list.
             */
            if ((DeviceObject->Vpb->Flags & VPB_RAW_MOUNT) &&
                (ListEntry->Flink != FsList))
            {
                /* Then skip this entry */
                ListEntry = ListEntry->Flink;
                continue;
            }

            /* Get the Device Object for this FS */
            FileSystemDeviceObject = CONTAINING_RECORD(ListEntry,
                                                       DEVICE_OBJECT,
                                                       Queue.ListEntry);
            ParentFsDeviceObject = FileSystemDeviceObject;

            /*
             * If this file system device is attached to some other device,
             * then we must make sure to increase the stack size for the IRP.
             * The default is +1, for the FS device itself.
             */
            FsStackOverhead = 1;
            while (FileSystemDeviceObject->AttachedDevice)
            {
                /* Get the next attached device and increase overhead */
                FileSystemDeviceObject = FileSystemDeviceObject->
                                         AttachedDevice;
                FsStackOverhead++;
            }

            /* Clear the event */
            KeClearEvent(&Event);

            /* Allocate the IRP */
            Irp = IoAllocateIrp(AttachedDeviceObject->StackSize +
                                (UCHAR)FsStackOverhead,
                                TRUE);
            if (!Irp)
            {
                /* Fail */
                Status =  STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            /* Setup the IRP */
            Irp->UserIosb = &IoStatusBlock;
            Irp->UserEvent = &Event;
            Irp->Tail.Overlay.Thread = PsGetCurrentThread();
            Irp->Flags = IRP_MOUNT_COMPLETION | IRP_SYNCHRONOUS_PAGING_IO;
            Irp->RequestorMode = KernelMode;

            /* Get the I/O Stack location and set it up */
            StackPtr = IoGetNextIrpStackLocation(Irp);
            StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
            StackPtr->MinorFunction = IRP_MN_MOUNT_VOLUME;
            StackPtr->Flags = AllowRawMount;
            StackPtr->Parameters.MountVolume.Vpb = DeviceObject->Vpb;
            StackPtr->Parameters.MountVolume.DeviceObject =
                AttachedDeviceObject;

            /* Save registration operations */
            RegistrationOps = IopFsRegistrationOps;

            /* Release locks */
            IopInterlockedIncrementUlong(LockQueueIoDatabaseLock, (PULONG)&DeviceObject->ReferenceCount);
            ExReleaseResourceLite(&IopDatabaseResource);

            /* Call the driver */
            Status = IoCallDriver(FileSystemDeviceObject, Irp);
            if (Status == STATUS_PENDING)
            {
                /* Wait on it */
                KeWaitForSingleObject(&Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;
            }

            ExAcquireResourceSharedLite(&IopDatabaseResource, TRUE);
            IopInterlockedDecrementUlong(LockQueueIoDatabaseLock, (PULONG)&DeviceObject->ReferenceCount);

            /* Check if mounting was successful */
            if (NT_SUCCESS(Status))
            {
                /* Mount the VPB */
                *Vpb = IopMountInitializeVpb(DeviceObject,
                                             AttachedDeviceObject,
                                             (DeviceObject->Vpb->Flags &
                                              VPB_RAW_MOUNT));
            }
            else
            {
                /* Check if we failed because of the user */
                if ((IoIsErrorUserInduced(Status)) &&
                    (IoStatusBlock.Information == 1))
                {
                    /* Break out and fail */
                    break;
                }

                /* If there were registration operations in the meanwhile */
                if (RegistrationOps != IopFsRegistrationOps)
                {
                    /* We need to setup a local list to pickup where we left */
                    LocalList.Flink = FsList->Flink;
                    ListEntry = &LocalList;

                    Status = STATUS_UNRECOGNIZED_VOLUME;
                }

                /* Otherwise, check if we need to load the FS driver */
                if (Status == STATUS_FS_DRIVER_REQUIRED)
                {
                    /* We need to release the lock */
                    IopInterlockedIncrementUlong(LockQueueIoDatabaseLock, (PULONG)&DeviceObject->ReferenceCount);
                    ExReleaseResourceLite(&IopDatabaseResource);

                    /* Release the device lock if we're holding it */
                    if (!DeviceIsLocked)
                    {
                        KeSetEvent(&DeviceObject->DeviceLock, 0, FALSE);
                    }

                    /* Leave critical section */
                    KeLeaveCriticalRegion();

                    /* Load the FS */
                    IopLoadFileSystemDriver(ParentFsDeviceObject);

                    /* Check if the device isn't already locked */
                    if (!DeviceIsLocked)
                    {
                        /* Lock it ourselves */
                        Status = KeWaitForSingleObject(&DeviceObject->
                                                       DeviceLock,
                                                       Executive,
                                                       KeGetPreviousMode(),
                                                       Alertable,
                                                       NULL);
                        if ((Status == STATUS_ALERTED) ||
                            (Status == STATUS_USER_APC))
                        {
                            /* Don't mount if we were interrupted */
                            ObDereferenceObject(AttachedDeviceObject);
                            return Status;
                        }
                    }

                    /* Reacquire the lock */
                    KeEnterCriticalRegion();
                    ExAcquireResourceSharedLite(&IopDatabaseResource, TRUE);

                    /* When we released the lock, make sure nobody beat us */
                    if (DeviceObject->Vpb->Flags & VPB_MOUNTED)
                    {
                        /* Someone did, break out */
                        Status = STATUS_SUCCESS;
                        break;
                    }

                    /* Start over by setting a failure */
                    Status = STATUS_UNRECOGNIZED_VOLUME;

                    /* We need to setup a local list to pickup where we left */
                    LocalList.Flink = FsList->Flink;
                    ListEntry = &LocalList;
                }

                /*
                 * Check if we failed with any other error then an unrecognized
                 * volume, and if this request doesn't allow mounting the raw
                 * file system.
                 */
                if (!(AllowRawMount) &&
                    (Status != STATUS_UNRECOGNIZED_VOLUME) &&
                    (FsRtlIsTotalDeviceFailure(Status)))
                {
                    /* Break out and give up */
                    break;
                }
            }

            /* Go to the next FS entry */
            ListEntry = ListEntry->Flink;
        }

        /* Dereference the device if we failed */
        if (!NT_SUCCESS(Status)) ObDereferenceObject(AttachedDeviceObject);
    }
    else if (DeviceObject->Vpb->Flags & VPB_REMOVE_PENDING)
    {
        /* Someone wants to remove us */
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
    }
    else
    {
        /* Someone already mounted us */
        Status = STATUS_SUCCESS;
    }

    /* Release the FS lock */
    ExReleaseResourceLite(&IopDatabaseResource);
    KeLeaveCriticalRegion();

    /* Release the device lock if we're holding it */
    if (!DeviceIsLocked) KeSetEvent(&DeviceObject->DeviceLock, 0, FALSE);

    /* Check if we failed to mount the boot partition */
    if ((!NT_SUCCESS(Status)) &&
        (DeviceObject->Flags & DO_SYSTEM_BOOT_PARTITION) &&
        ExpInitializationPhase < 2)
    {
        /* Bugcheck the system */
        KeBugCheckEx(INACCESSIBLE_BOOT_DEVICE,
                     (ULONG_PTR)DeviceObject,
                     Status,
                     0,
                     0);
    }

    /* Return the mount status */
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
IopNotifyAlreadyRegisteredFileSystems(IN PLIST_ENTRY ListHead,
                                      IN PDRIVER_FS_NOTIFICATION DriverNotificationRoutine,
                                      BOOLEAN SkipRawFs)
{
    PLIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;

    /* Browse the whole list */
    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead)
    {
        /* Check if we reached rawfs and if we have to skip it */
        if (ListEntry->Flink == ListHead && SkipRawFs)
        {
            return;
        }

        /* Otherwise, get DO and notify */
        DeviceObject = CONTAINING_RECORD(ListEntry,
                                         DEVICE_OBJECT,
                                         Queue.ListEntry);

        DriverNotificationRoutine(DeviceObject, TRUE);

        /* Go to the next entry */
        ListEntry = ListEntry->Flink;
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoEnumerateRegisteredFiltersList(OUT PDRIVER_OBJECT *DriverObjectList,
                                 IN ULONG DriverObjectListSize,
                                 OUT PULONG ActualNumberDriverObjects)
{
    PLIST_ENTRY ListEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;
    ULONG ListSize = 0, MaximumSize = DriverObjectListSize / sizeof(PDRIVER_OBJECT);

    /* Acquire the FS lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopDatabaseResource, TRUE);

    /* Browse the whole list */
    ListEntry = IopFsNotifyChangeQueueHead.Flink;
    while (ListEntry != &IopFsNotifyChangeQueueHead)
    {
        ChangeEntry = CONTAINING_RECORD(ListEntry,
                                        FS_CHANGE_NOTIFY_ENTRY,
                                        FsChangeNotifyList);

        /* If buffer is still big enough */
        if (ListSize < MaximumSize)
        {
            /* Reference the driver object */
            ObReferenceObject(ChangeEntry->DriverObject);
            /* And pass it to the caller */
            DriverObjectList[ListSize] = ChangeEntry->DriverObject;
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        /* Increase size counter */
        ListSize++;

        /* Go to the next entry */
        ListEntry = ListEntry->Flink;
    }

    /* Return list size */
    *ActualNumberDriverObjects = ListSize;

    /* Release the FS lock */
    ExReleaseResourceLite(&IopDatabaseResource);
    KeLeaveCriticalRegion();

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoVerifyVolume(IN PDEVICE_OBJECT DeviceObject,
               IN BOOLEAN AllowRawMount)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION StackPtr;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status, VpbStatus;
    PDEVICE_OBJECT FileSystemDeviceObject;
    PVPB Vpb, NewVpb;
    //BOOLEAN WasNotMounted = TRUE;

    /* Wait on the device lock */
    Status = KeWaitForSingleObject(&DeviceObject->DeviceLock,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    ASSERT(Status == STATUS_SUCCESS);

    /* Reference the VPB */
    if (IopReferenceVerifyVpb(DeviceObject, &FileSystemDeviceObject, &Vpb))
    {
        /* Initialize the event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* Find the actual File System DO */
        //WasNotMounted = FALSE;
        FileSystemDeviceObject = DeviceObject->Vpb->DeviceObject;
        while (FileSystemDeviceObject->AttachedDevice)
        {
            /* Go to the next one */
            FileSystemDeviceObject = FileSystemDeviceObject->AttachedDevice;
        }

        /* Allocate the IRP */
        Irp = IoAllocateIrp(FileSystemDeviceObject->StackSize, FALSE);
        if (!Irp)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Release;
        }

        /* Set it up */
        Irp->UserIosb = &IoStatusBlock;
        Irp->UserEvent = &Event;
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        Irp->Flags = IRP_MOUNT_COMPLETION | IRP_SYNCHRONOUS_PAGING_IO;
        Irp->RequestorMode = KernelMode;

        /* Get the I/O Stack location and set it */
        StackPtr = IoGetNextIrpStackLocation(Irp);
        StackPtr->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
        StackPtr->MinorFunction = IRP_MN_VERIFY_VOLUME;
        StackPtr->Flags = AllowRawMount ? SL_ALLOW_RAW_MOUNT : 0;
        StackPtr->Parameters.VerifyVolume.Vpb = Vpb;
        StackPtr->Parameters.VerifyVolume.DeviceObject =
            DeviceObject->Vpb->DeviceObject;

        /* Call the driver */
        Status = IoCallDriver(FileSystemDeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait on it */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        /* Dereference the VPB */
        IopDereferenceVpbAndFree(Vpb);
    }

    /* Check if we had the wrong volume or didn't mount at all */
    if (Status == STATUS_WRONG_VOLUME)
    {
        /* Create a VPB */
        VpbStatus = IopCreateVpb(DeviceObject);
        if (NT_SUCCESS(VpbStatus))
        {
            PoVolumeDevice(DeviceObject);

            /* Mount it */
            VpbStatus = IopMountVolume(DeviceObject,
                                       AllowRawMount,
                                       TRUE,
                                       FALSE,
                                       &NewVpb);

            /* If we got a new VPB, dereference it */
            if (NewVpb)
            {
                IopInterlockedDecrementUlong(LockQueueIoVpbLock, &NewVpb->ReferenceCount);
            }
        }

        /* If we failed, remove the verify flag */
        if (!NT_SUCCESS(VpbStatus)) DeviceObject->Flags &= ~DO_VERIFY_VOLUME;
    }

Release:
    /* Signal the device lock and return */
    KeSetEvent(&DeviceObject->DeviceLock, IO_NO_INCREMENT, FALSE);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
IoRegisterFileSystem(IN PDEVICE_OBJECT DeviceObject)
{
    PLIST_ENTRY FsList = NULL;
    PAGED_CODE();

    /* Acquire the FS lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopDatabaseResource, TRUE);

    /* Check what kind of FS this is */
    if (DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM)
    {
        /* Use the disk list */
        FsList = &IopDiskFileSystemQueueHead;
    }
    else if (DeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM)
    {
        /* Use the network device list */
        FsList = &IopNetworkFileSystemQueueHead;
    }
    else if (DeviceObject->DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM)
    {
        /* Use the CD-ROM list */
        FsList = &IopCdRomFileSystemQueueHead;
    }
    else if (DeviceObject->DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM)
    {
        /* Use the tape list */
        FsList = &IopTapeFileSystemQueueHead;
    }

    /* Make sure that we have a valid list */
    if (FsList)
    {
        /* Check if we should insert it at the top or bottom of the list */
        if (DeviceObject->Flags & DO_LOW_PRIORITY_FILESYSTEM)
        {
            /* At the bottom */
            InsertTailList(FsList->Blink, &DeviceObject->Queue.ListEntry);
        }
        else
        {
            /* On top */
            InsertHeadList(FsList, &DeviceObject->Queue.ListEntry);
        }
    }

    /* Update operations counter */
    IopFsRegistrationOps++;

    /* Clear the initializing flag */
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Notify file systems of the addition */
    IopNotifyFileSystemChange(DeviceObject, TRUE);

    /* Release the FS Lock */
    ExReleaseResourceLite(&IopDatabaseResource);
    KeLeaveCriticalRegion();

    /* Ensure driver won't be unloaded */
    IopInterlockedIncrementUlong(LockQueueIoDatabaseLock, (PULONG)&DeviceObject->ReferenceCount);
}

/*
 * @implemented
 */
VOID
NTAPI
IoUnregisterFileSystem(IN PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();

    /* Acquire the FS lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopDatabaseResource, TRUE);

    /* Simply remove the entry - if queued */
    if (DeviceObject->Queue.ListEntry.Flink)
    {
        RemoveEntryList(&DeviceObject->Queue.ListEntry);
    }

    /* And notify all registered file systems */
    IopNotifyFileSystemChange(DeviceObject, FALSE);

    /* Update operations counter */
    IopFsRegistrationOps++;

    /* Then release the lock */
    ExReleaseResourceLite(&IopDatabaseResource);
    KeLeaveCriticalRegion();

    /* Decrease reference count to allow unload */
    IopInterlockedDecrementUlong(LockQueueIoDatabaseLock, (PULONG)&DeviceObject->ReferenceCount);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoRegisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
                               IN PDRIVER_FS_NOTIFICATION DriverNotificationRoutine)
{
    PFS_CHANGE_NOTIFY_ENTRY Entry;
    PAGED_CODE();

    /* Acquire the list lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopDatabaseResource, TRUE);

    /* Check if that driver is already registered (successive calls)
     * See MSDN note: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ioregisterfsregistrationchange
     */
    if (!IsListEmpty(&IopFsNotifyChangeQueueHead))
    {
        Entry = CONTAINING_RECORD(IopFsNotifyChangeQueueHead.Blink,
                                  FS_CHANGE_NOTIFY_ENTRY,
                                  FsChangeNotifyList);

        if (Entry->DriverObject == DriverObject &&
            Entry->FSDNotificationProc == DriverNotificationRoutine)
        {
            /* Release the lock */
            ExReleaseResourceLite(&IopDatabaseResource);

            return STATUS_DEVICE_ALREADY_ATTACHED;
        }
    }

    /* Allocate a notification entry */
    Entry = ExAllocatePoolWithTag(PagedPool,
                                  sizeof(FS_CHANGE_NOTIFY_ENTRY),
                                  TAG_FS_CHANGE_NOTIFY);
    if (!Entry)
    {
        /* Release the lock */
        ExReleaseResourceLite(&IopDatabaseResource);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Save the driver object and notification routine */
    Entry->DriverObject = DriverObject;
    Entry->FSDNotificationProc = DriverNotificationRoutine;

    /* Insert it into the notification list */
    InsertTailList(&IopFsNotifyChangeQueueHead, &Entry->FsChangeNotifyList);

    /* Start notifying all already present FS */
    IopNotifyAlreadyRegisteredFileSystems(&IopNetworkFileSystemQueueHead, DriverNotificationRoutine, FALSE);
    IopNotifyAlreadyRegisteredFileSystems(&IopCdRomFileSystemQueueHead, DriverNotificationRoutine, TRUE);
    IopNotifyAlreadyRegisteredFileSystems(&IopDiskFileSystemQueueHead, DriverNotificationRoutine, TRUE);
    IopNotifyAlreadyRegisteredFileSystems(&IopTapeFileSystemQueueHead, DriverNotificationRoutine, TRUE);

    /* Release the lock */
    ExReleaseResourceLite(&IopDatabaseResource);
    KeLeaveCriticalRegion();

    /* Reference the driver */
    ObReferenceObject(DriverObject);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IoUnregisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
                                 IN PDRIVER_FS_NOTIFICATION FSDNotificationProc)
{
    PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;
    PLIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Acquire the list lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopDatabaseResource, TRUE);

    /* Loop the list */
    NextEntry = IopFsNotifyChangeQueueHead.Flink;
    while (NextEntry != &IopFsNotifyChangeQueueHead)
    {
        /* Get the entry */
        ChangeEntry = CONTAINING_RECORD(NextEntry,
                                        FS_CHANGE_NOTIFY_ENTRY,
                                        FsChangeNotifyList);

        /* Check if it matches this de-registration */
        if ((ChangeEntry->DriverObject == DriverObject) &&
            (ChangeEntry->FSDNotificationProc == FSDNotificationProc))
        {
            /* It does, remove it from the list */
            RemoveEntryList(&ChangeEntry->FsChangeNotifyList);
            ExFreePoolWithTag(ChangeEntry, TAG_FS_CHANGE_NOTIFY);
            break;
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock and dereference the driver */
    ExReleaseResourceLite(&IopDatabaseResource);
    KeLeaveCriticalRegion();

    /* Dereference the driver */
    ObDereferenceObject(DriverObject);
}

/*
 * @implemented
 */
VOID
NTAPI
IoAcquireVpbSpinLock(OUT PKIRQL Irql)
{
    /* Simply acquire the lock */
    *Irql = KeAcquireQueuedSpinLock(LockQueueIoVpbLock);
}

/*
 * @implemented
 */
VOID
NTAPI
IoReleaseVpbSpinLock(IN KIRQL Irql)
{
    /* Just release the lock */
    KeReleaseQueuedSpinLock(LockQueueIoVpbLock, Irql);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoSetSystemPartition(IN PUNICODE_STRING VolumeNameString)
{
    NTSTATUS Status;
    HANDLE RootHandle, KeyHandle;
    UNICODE_STRING HKLMSystem, KeyString;
    WCHAR Buffer[sizeof(L"SystemPartition") / sizeof(WCHAR)];

    RtlInitUnicodeString(&HKLMSystem, L"\\REGISTRY\\MACHINE\\SYSTEM");

    /* Open registry to save data (HKLM\SYSTEM) */
    Status = IopOpenRegistryKeyEx(&RootHandle, 0, &HKLMSystem, KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Create or open Setup subkey */
    KeyString.Buffer = Buffer;
    KeyString.Length = sizeof(L"Setup") - sizeof(UNICODE_NULL);
    KeyString.MaximumLength = sizeof(L"Setup");
    RtlCopyMemory(Buffer, L"Setup", sizeof(L"Setup"));
    Status = IopCreateRegistryKeyEx(&KeyHandle,
                                    RootHandle,
                                    &KeyString,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    NULL);
    ZwClose(RootHandle);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Store caller value */
    KeyString.Length = sizeof(L"SystemPartition") - sizeof(UNICODE_NULL);
    KeyString.MaximumLength = sizeof(L"SystemPartition");
    RtlCopyMemory(Buffer, L"SystemPartition", sizeof(L"SystemPartition"));
    Status = ZwSetValueKey(KeyHandle,
                           &KeyString,
                           0,
                           REG_SZ,
                           VolumeNameString->Buffer,
                           VolumeNameString->Length + sizeof(UNICODE_NULL));
    ZwClose(KeyHandle);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoVolumeDeviceToDosName(
    _In_ PVOID VolumeDeviceObject,
    _Out_ _When_(return==0, _At_(DosName->Buffer, __drv_allocatesMem(Mem)))
        PUNICODE_STRING DosName)
{
    NTSTATUS Status;
    ULONG Length;
    KEVENT Event;
    PIRP Irp;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING MountMgrDevice;
    MOUNTMGR_VOLUME_PATHS VolumePath;
    PMOUNTMGR_VOLUME_PATHS VolumePathPtr;
    /*
     * This variable is used to query the device name.
     * It's based on MOUNTDEV_NAME (mountmgr.h).
     * Doing it this way prevents memory allocation.
     * The device name won't be longer.
     */
    struct
    {
        USHORT NameLength;
        WCHAR DeviceName[256];
    } DeviceName;

    PAGED_CODE();

    /* First, retrieve the corresponding device name */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        VolumeDeviceObject,
                                        NULL, 0,
                                        &DeviceName, sizeof(DeviceName),
                                        FALSE, &Event, &IoStatusBlock);
    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(VolumeDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Retrieve the MountMgr controlling device */
    RtlInitUnicodeString(&MountMgrDevice, MOUNTMGR_DEVICE_NAME);
    Status = IoGetDeviceObjectPointer(&MountMgrDevice, FILE_READ_ATTRIBUTES,
                                      &FileObject, &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Now, query the MountMgr for the DOS path */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                        DeviceObject,
                                        &DeviceName, sizeof(DeviceName),
                                        &VolumePath, sizeof(VolumePath),
                                        FALSE, &Event, &IoStatusBlock);
    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* The only tolerated failure here is buffer too small, which is expected */
    if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_OVERFLOW))
    {
        goto Quit;
    }

    /* Compute the needed size to store the DOS path.
     * Even if MOUNTMGR_VOLUME_PATHS allows bigger name lengths
     * than MAXUSHORT, we can't use them, because we have to return
     * this in an UNICODE_STRING that stores length in a USHORT. */
    Length = FIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz) + VolumePath.MultiSzLength;
    if (Length > MAXUSHORT)
    {
        Status = STATUS_INVALID_BUFFER_SIZE;
        goto Quit;
    }

    /* Allocate the buffer, even in case of success,
     * because it is returned to the caller */
    VolumePathPtr = ExAllocatePoolWithTag(PagedPool, Length, TAG_DEV2DOS);
    if (!VolumePathPtr)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Re-query the DOS path with the proper size */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                        DeviceObject,
                                        &DeviceName, sizeof(DeviceName),
                                        VolumePathPtr, Length,
                                        FALSE, &Event, &IoStatusBlock);
    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ReleaseMemory;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }
    if (!NT_SUCCESS(Status))
    {
        goto ReleaseMemory;
    }

    /* Set the output string. Discount the last two
     * NUL-terminators from the multi-string length. */
    DosName->Length = (USHORT)VolumePathPtr->MultiSzLength - 2 * sizeof(UNICODE_NULL);
    DosName->MaximumLength = DosName->Length + sizeof(UNICODE_NULL);
    /* Recycle our MOUNTMGR_VOLUME_PATHS as the output buffer
     * and move the NUL-terminated string to the beginning */
    DosName->Buffer = (PWSTR)VolumePathPtr;
    RtlMoveMemory(DosName->Buffer, VolumePathPtr->MultiSz, DosName->Length);
    DosName->Buffer[DosName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Don't release the buffer, just dereference the FO and return success */
    Status = STATUS_SUCCESS;
    goto Quit;

ReleaseMemory:
    ExFreePoolWithTag(VolumePathPtr, TAG_DEV2DOS);

Quit:
    ObDereferenceObject(FileObject);
    return Status;
}

/* EOF */
