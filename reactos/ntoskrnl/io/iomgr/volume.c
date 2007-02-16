/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/volume.c
 * PURPOSE:         Volume and File System I/O Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, IoInitFileSystemImplementation)
#pragma alloc_text(INIT, IoInitVpbImplementation)
#endif

/* GLOBALS ******************************************************************/

ERESOURCE FileSystemListLock;
LIST_ENTRY IopDiskFsListHead, IopNetworkFsListHead;
LIST_ENTRY IopCdRomFsListHead, IopTapeFsListHead;
KGUARDED_MUTEX FsChangeNotifyListLock;
LIST_ENTRY FsChangeNotifyListHead;
KSPIN_LOCK IoVpbLock;

/* PRIVATE FUNCTIONS *********************************************************/

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
            if (!NT_SUCCESS(Status)) return NULL;

            /* Otherwise we were alerted */
            *Status = STATUS_WRONG_VOLUME;
            return NULL;
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

NTSTATUS
NTAPI
IopCreateVpb(IN PDEVICE_OBJECT DeviceObject)
{
    PVPB Vpb;

    /* Allocate the Vpb */
    Vpb = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(VPB),
                                TAG_VPB);
    if (!Vpb) return STATUS_UNSUCCESSFUL;

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

VOID
NTAPI
IopDereferenceVpb(IN PVPB Vpb)
{
    KIRQL OldIrql;

    /* Lock the VPBs and decrease references */
    IoAcquireVpbSpinLock(&OldIrql);
    Vpb->ReferenceCount--;

    /* Check if we're out of references */
    if (!Vpb->ReferenceCount)
    {
        /* FIXME: IMPLEMENT CLEANUP! */
        KEBUGCHECK(0);
    }

    /* Release VPB lock */
    IoReleaseVpbSpinLock(OldIrql);
}

BOOLEAN
NTAPI
IopReferenceVpbForVerify(IN PDEVICE_OBJECT DeviceObject,
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
IopInitializeVpbForMount(IN PDEVICE_OBJECT DeviceObject,
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

VOID
NTAPI
IopNotifyFileSystemChange(IN PDEVICE_OBJECT DeviceObject,
                          IN BOOLEAN DriverActive)
{
    PFS_CHANGE_NOTIFY_ENTRY ChangeEntry;
    PLIST_ENTRY ListEntry;

    /* Acquire the notification lock */
    KeAcquireGuardedMutex(&FsChangeNotifyListLock);

    /* Loop the list */
    ListEntry = FsChangeNotifyListHead.Flink;
    while (ListEntry != &FsChangeNotifyListHead)
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

    /* Release the lock */
    KeReleaseGuardedMutex(&FsChangeNotifyListLock);
}

VOID
NTAPI
IoShutdownRegisteredFileSystems(VOID)
{
    PLIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK StatusBlock;
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;

    /* Lock the FS List and initialize an event to wait on */
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&FileSystemListLock,TRUE);
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Get the first entry and start looping */
    ListEntry = IopDiskFsListHead.Flink;
    while (ListEntry != &IopDiskFsListHead)
    {
        /* Get the device object */
        DeviceObject = CONTAINING_RECORD(ListEntry,
                                         DEVICE_OBJECT,
                                         Queue.ListEntry);

        /* Check if we're attached */
        if (DeviceObject->AttachedDevice)
        {
            /* Get the attached device */
            DeviceObject = IoGetAttachedDevice(DeviceObject);
        }

        /* Build the shutdown IRP and call the driver */
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
                                           DeviceObject,
                                           NULL,
                                           0,
                                           NULL,
                                           &Event,
                                           &StatusBlock);
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait on the driver */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }

        /* Reset the event */
        KeClearEvent(&Event);

        /* Go to the next entry */
        ListEntry = ListEntry->Flink;
    }

    /* Release the lock */
    ExReleaseResourceLite(&FileSystemListLock);
    KeLeaveCriticalRegion();
}

VOID
NTAPI
IopLoadFileSystem(IN PDEVICE_OBJECT DeviceObject)
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
}

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
    ULONG FsStackOverhead;
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
    ExAcquireResourceSharedLite(&FileSystemListLock, TRUE);

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
            FsList = &IopDiskFsListHead;
        }
        else if (DeviceObject->DeviceType == FILE_DEVICE_CD_ROM)
        {
            /* Use the CD-ROM list */
            FsList = &IopCdRomFsListHead;
        }
        else
        {
            /* It's gotta be a tape... */
            FsList = &IopTapeFsListHead;
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

            /* Check if mounting was successful */
            if (NT_SUCCESS(Status))
            {
                /* Mount the VPB */
                *Vpb = IopInitializeVpbForMount(DeviceObject,
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

                /* Otherwise, check if we need to load the FS driver */
                if (Status == STATUS_FS_DRIVER_REQUIRED)
                {
                    /* We need to release the lock */
                    ExReleaseResourceLite(&FileSystemListLock);

                    /* Release the device lock if we're holding it */
                    if (!DeviceIsLocked)
                    {
                        KeSetEvent(&DeviceObject->DeviceLock, 0, FALSE);
                    }

                    /* Load the FS */
                    IopLoadFileSystem(ParentFsDeviceObject);

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
                    ExAcquireResourceSharedLite(&FileSystemListLock, TRUE);

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
    ExReleaseResourceLite(&FileSystemListLock);
    KeLeaveCriticalRegion();

    /* Release the device lock if we're holding it */
    if (!DeviceIsLocked) KeSetEvent(&DeviceObject->DeviceLock, 0, FALSE);

    /* Check if we failed to mount the boot partition */
    if ((!NT_SUCCESS(Status)) &&
        (DeviceObject->Flags & DO_SYSTEM_BOOT_PARTITION))
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

/* PUBLIC FUNCTIONS **********************************************************/

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
    NTSTATUS Status = STATUS_SUCCESS, VpbStatus;
    PDEVICE_OBJECT FileSystemDeviceObject;
    PVPB Vpb, NewVpb;
    BOOLEAN WasNotMounted = TRUE;

    /* Wait on the device lock */
    KeWaitForSingleObject(&DeviceObject->DeviceLock,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Reference the VPB */
    if (IopReferenceVpbForVerify(DeviceObject, &FileSystemDeviceObject, &Vpb))
    {
        /* Initialize the event */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        /* Find the actual File System DO */
        WasNotMounted = FALSE;
        FileSystemDeviceObject = DeviceObject->Vpb->DeviceObject;
        while (FileSystemDeviceObject->AttachedDevice)
        {
            /* Go to the next one */
            FileSystemDeviceObject = FileSystemDeviceObject->AttachedDevice;
        }

        /* Allocate the IRP */
        Irp = IoAllocateIrp(FileSystemDeviceObject->StackSize, FALSE);
        if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

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
        IopDereferenceVpb(Vpb);
    }

    /* Check if we had the wrong volume or didn't mount at all */
    if ((Status == STATUS_WRONG_VOLUME) || (WasNotMounted))
    {
        /* Create a VPB */
        VpbStatus = IopCreateVpb(DeviceObject);
        if (NT_SUCCESS(VpbStatus))
        {
            /* Mount it */
            VpbStatus = IopMountVolume(DeviceObject,
                                       AllowRawMount,
                                       TRUE,
                                       FALSE,
                                       &NewVpb);
        }

        /* If we failed, remove the verify flag */
        if (!NT_SUCCESS(VpbStatus)) DeviceObject->Flags &= ~DO_VERIFY_VOLUME;
    }

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
    ExAcquireResourceExclusiveLite(&FileSystemListLock, TRUE);

    /* Check what kind of FS this is */
    if (DeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM)
    {
        /* Use the disk list */
        FsList = &IopDiskFsListHead;
    }
    else if (DeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM)
    {
        /* Use the network device list */
        FsList = &IopNetworkFsListHead;
    }
    else if (DeviceObject->DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM)
    {
        /* Use the CD-ROM list */
        FsList = &IopCdRomFsListHead;
    }
    else if (DeviceObject->DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM)
    {
        /* Use the tape list */
        FsList = &IopTapeFsListHead;
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

    /* Clear the initializing flag */
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /* Release the FS Lock */
    ExReleaseResourceLite(&FileSystemListLock);
    KeLeaveCriticalRegion();

    /* Notify file systems of the addition */
    IopNotifyFileSystemChange(DeviceObject, TRUE);
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
    ExAcquireResourceExclusiveLite(&FileSystemListLock, TRUE);

    /* Simply remove the entry */
    RemoveEntryList(&DeviceObject->Queue.ListEntry);

    /* And notify all registered file systems */
    IopNotifyFileSystemChange(DeviceObject, FALSE);

    /* Then release the lock */
    ExReleaseResourceLite(&FileSystemListLock);
    KeLeaveCriticalRegion();
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoRegisterFsRegistrationChange(IN PDRIVER_OBJECT DriverObject,
                               IN PDRIVER_FS_NOTIFICATION FSDNotificationProc)
{
    PFS_CHANGE_NOTIFY_ENTRY Entry;
    PAGED_CODE();

    /* Allocate a notification entry */
    Entry = ExAllocatePoolWithTag(PagedPool,
                                  sizeof(FS_CHANGE_NOTIFY_ENTRY),
                                  TAG_FS_CHANGE_NOTIFY);
    if (!Entry) return(STATUS_INSUFFICIENT_RESOURCES);

    /* Save the driver object and notification routine */
    Entry->DriverObject = DriverObject;
    Entry->FSDNotificationProc = FSDNotificationProc;

    /* Insert it into the notification list */
    KeAcquireGuardedMutex(&FsChangeNotifyListLock);
    InsertTailList(&FsChangeNotifyListHead, &Entry->FsChangeNotifyList);
    KeReleaseGuardedMutex(&FsChangeNotifyListLock);

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
    KeAcquireGuardedMutex(&FsChangeNotifyListLock);

    /* Loop the list */
    NextEntry = FsChangeNotifyListHead.Flink;
    while (NextEntry != &FsChangeNotifyListHead)
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
    KeReleaseGuardedMutex(&FsChangeNotifyListLock);
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
    KeAcquireSpinLock(&IoVpbLock, Irql);
}

/*
 * @implemented
 */
VOID
NTAPI
IoReleaseVpbSpinLock(IN KIRQL Irql)
{
    /* Just release the lock */
    KeReleaseSpinLock(&IoVpbLock, Irql);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoSetSystemPartition(IN PUNICODE_STRING VolumeNameString)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoVolumeDeviceToDosName(IN PVOID VolumeDeviceObject,
                        OUT PUNICODE_STRING DosName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
