/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/device.c
 * PURPOSE:         Device Object Management, including Notifications and Queues.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Pierre Schweitzer
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG IopDeviceObjectNumber = 0;
LIST_ENTRY ShutdownListHead, LastChanceShutdownListHead;
KSPIN_LOCK ShutdownListLock;
extern LIST_ENTRY IopDiskFileSystemQueueHead;
extern LIST_ENTRY IopCdRomFileSystemQueueHead;
extern LIST_ENTRY IopTapeFileSystemQueueHead;
extern ERESOURCE IopDatabaseResource;

#define DACL_SET 4

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
IopReadyDeviceObjects(IN PDRIVER_OBJECT Driver)
{
    PDEVICE_OBJECT DeviceObject;
    PAGED_CODE();

    /* Set the driver as initialized */
    Driver->Flags |= DRVO_INITIALIZED;
    DeviceObject = Driver->DeviceObject;
    while (DeviceObject)
    {
        /* Set every device as initialized too */
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        DeviceObject = DeviceObject->NextDevice;
    }
}

VOID
NTAPI
IopDeleteDevice(IN PVOID ObjectBody)
{
    PDEVICE_OBJECT DeviceObject = ObjectBody;
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
    PAGED_CODE();

    /* Cleanup and free the device node */
    if (DeviceNode)
        IopFreeDeviceNode(DeviceNode);

    /* Dereference the driver object, referenced in IoCreateDevice */
    if (DeviceObject->DriverObject)
        ObDereferenceObject(DeviceObject->DriverObject);
}


PDEVICE_OBJECT
NTAPI
IopAttachDeviceToDeviceStackSafe(IN PDEVICE_OBJECT SourceDevice,
                                 IN PDEVICE_OBJECT TargetDevice,
                                 OUT PDEVICE_OBJECT *AttachedToDeviceObject OPTIONAL)
{
    PDEVICE_OBJECT AttachedDevice;
    PEXTENDED_DEVOBJ_EXTENSION SourceDeviceExtension;

    /* Get the Attached Device and source extension */
    AttachedDevice = IoGetAttachedDevice(TargetDevice);
    SourceDeviceExtension = IoGetDevObjExtension(SourceDevice);
    ASSERT(SourceDeviceExtension->AttachedTo == NULL);

    /* Make sure that it's in a correct state */
    if ((AttachedDevice->Flags & DO_DEVICE_INITIALIZING) ||
        (IoGetDevObjExtension(AttachedDevice)->ExtensionFlags &
         (DOE_UNLOAD_PENDING |
          DOE_DELETE_PENDING |
          DOE_REMOVE_PENDING |
          DOE_REMOVE_PROCESSED)))
    {
        /* Device was unloading or being removed */
        AttachedDevice = NULL;
    }
    else
    {
        /* Update atached device fields */
        AttachedDevice->AttachedDevice = SourceDevice;
        AttachedDevice->Spare1++;

        /* Update the source with the attached data */
        SourceDevice->StackSize = AttachedDevice->StackSize + 1;
        SourceDevice->AlignmentRequirement = AttachedDevice->
                                             AlignmentRequirement;
        SourceDevice->SectorSize = AttachedDevice->SectorSize;

        /* Check for pending start flag */
        if (IoGetDevObjExtension(AttachedDevice)->ExtensionFlags &
            DOE_START_PENDING)
        {
            /* Propagate */
            IoGetDevObjExtension(SourceDevice)->ExtensionFlags |=
                DOE_START_PENDING;
        }

        /* Set the attachment in the device extension */
        SourceDeviceExtension->AttachedTo = AttachedDevice;
    }

    /* Return the attached device */
    if (AttachedToDeviceObject) *AttachedToDeviceObject = AttachedDevice;
    return AttachedDevice;
}

VOID
NTAPI
IoShutdownPnpDevices(VOID)
{
    /* This routine is only used by Driver Verifier to validate shutdown */
    return;
}

VOID
NTAPI
IoShutdownSystem(IN ULONG Phase)
{
    PLIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    PSHUTDOWN_ENTRY ShutdownEntry;
    IO_STATUS_BLOCK StatusBlock;
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;

    /* Initialize an event to wait on */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* What phase? */
    if (Phase == 0)
    {
        /* Shutdown PnP */
        IoShutdownPnpDevices();

        /* Loop first-chance shutdown notifications */
        ListEntry = ExInterlockedRemoveHeadList(&ShutdownListHead,
                                                &ShutdownListLock);
        while (ListEntry)
        {
            /* Get the shutdown entry */
            ShutdownEntry = CONTAINING_RECORD(ListEntry,
                                              SHUTDOWN_ENTRY,
                                              ShutdownList);

            /* Get the attached device */
            DeviceObject = IoGetAttachedDevice(ShutdownEntry->DeviceObject);

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

            /* Remove the flag */
            ShutdownEntry->DeviceObject->Flags &= ~DO_SHUTDOWN_REGISTERED;

            /* Get rid of our reference to it */
            ObDereferenceObject(ShutdownEntry->DeviceObject);

            /* Free the shutdown entry and reset the event */
            ExFreePoolWithTag(ShutdownEntry, TAG_SHUTDOWN_ENTRY);
            KeClearEvent(&Event);

            /* Go to the next entry */
            ListEntry = ExInterlockedRemoveHeadList(&ShutdownListHead,
                                                    &ShutdownListLock);
         }
    }
    else if (Phase == 1)
    {
        /* Acquire resource forever */
        ExAcquireResourceExclusiveLite(&IopDatabaseResource, TRUE);

        /* Shutdown disk file systems */
        IopShutdownBaseFileSystems(&IopDiskFileSystemQueueHead);

        /* Shutdown cdrom file systems */
        IopShutdownBaseFileSystems(&IopCdRomFileSystemQueueHead);

        /* Shutdown tape filesystems */
        IopShutdownBaseFileSystems(&IopTapeFileSystemQueueHead);

        /* Loop last-chance shutdown notifications */
        ListEntry = ExInterlockedRemoveHeadList(&LastChanceShutdownListHead,
                                                &ShutdownListLock);
        while (ListEntry)
        {
            /* Get the shutdown entry */
            ShutdownEntry = CONTAINING_RECORD(ListEntry,
                                              SHUTDOWN_ENTRY,
                                              ShutdownList);

            /* Get the attached device */
            DeviceObject = IoGetAttachedDevice(ShutdownEntry->DeviceObject);

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

            /* Remove the flag */
            ShutdownEntry->DeviceObject->Flags &= ~DO_SHUTDOWN_REGISTERED;

            /* Get rid of our reference to it */
            ObDereferenceObject(ShutdownEntry->DeviceObject);

            /* Free the shutdown entry and reset the event */
            ExFreePoolWithTag(ShutdownEntry, TAG_SHUTDOWN_ENTRY);
            KeClearEvent(&Event);

            /* Go to the next entry */
            ListEntry = ExInterlockedRemoveHeadList(&LastChanceShutdownListHead,
                                                    &ShutdownListLock);
         }

    }
}

NTSTATUS
NTAPI
IopGetDeviceObjectPointer(IN PUNICODE_STRING ObjectName,
                          IN ACCESS_MASK DesiredAccess,
                          OUT PFILE_OBJECT *FileObject,
                          OUT PDEVICE_OBJECT *DeviceObject,
                          IN ULONG AttachFlag)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    PFILE_OBJECT LocalFileObject;
    HANDLE FileHandle;
    NTSTATUS Status;

    /* Open the Device */
    InitializeObjectAttributes(&ObjectAttributes,
                               ObjectName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&FileHandle,
                        DesiredAccess,
                        &ObjectAttributes,
                        &StatusBlock,
                        0,
                        FILE_NON_DIRECTORY_FILE | AttachFlag);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&LocalFileObject,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Return the requested data */
        *DeviceObject = IoGetRelatedDeviceObject(LocalFileObject);
        *FileObject = LocalFileObject;
    }

    /* Close the handle */
    ZwClose(FileHandle);

    return Status;
}

PDEVICE_OBJECT
NTAPI
IopGetLowestDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT LowestDevice;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;

    /* Get the current device and its extension */
    LowestDevice = DeviceObject;
    DeviceExtension = IoGetDevObjExtension(LowestDevice);

    /* Keep looping as long as we're attached */
    while (DeviceExtension->AttachedTo)
    {
        /* Get the lowest device and its extension */
        LowestDevice = DeviceExtension->AttachedTo;
        DeviceExtension = IoGetDevObjExtension(LowestDevice);
    }

    /* Return the lowest device */
    return LowestDevice;
}

VOID
NTAPI
IopEditDeviceList(IN PDRIVER_OBJECT DriverObject,
                  IN PDEVICE_OBJECT DeviceObject,
                  IN IOP_DEVICE_LIST_OPERATION Type)
{
    PDEVICE_OBJECT Previous;
    KIRQL OldIrql;

    /* Lock the Device list while we edit it */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);

    /* Check the type of operation */
    if (Type == IopRemove)
    {
        /* Get the current device and check if it's the current one */
        Previous = DeviceObject->DriverObject->DeviceObject;
        if (Previous == DeviceObject)
        {
            /* It is, simply unlink this one directly */
            DeviceObject->DriverObject->DeviceObject =
                DeviceObject->NextDevice;
        }
        else
        {
            /* It's not, so loop until we find the device */
            while (Previous->NextDevice != DeviceObject)
            {
                /* Not this one, keep moving */
                if (!Previous->NextDevice)
                {
                    DPRINT1("Failed to remove PDO %p (not found)\n",
                            DeviceObject);

                    ASSERT(FALSE);
                    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
                    return;
                }
                Previous = Previous->NextDevice;
            }

            /* We found it, now unlink us */
            Previous->NextDevice = DeviceObject->NextDevice;
        }
    }
    else
    {
        /* Link the device object and the driver object */
        DeviceObject->NextDevice = DriverObject->DeviceObject;
        DriverObject->DeviceObject = DeviceObject;
    }

    /* Release the device list lock */
    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
}

VOID
NTAPI
IopUnloadDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    PEXTENDED_DEVOBJ_EXTENSION ThisExtension = IoGetDevObjExtension(DeviceObject);

    /* Check if deletion is pending */
    if (ThisExtension->ExtensionFlags & DOE_DELETE_PENDING)
    {
        if (DeviceObject->AttachedDevice)
        {
            DPRINT("Device object is in the middle of a device stack\n");
            return;
        }

        if (DeviceObject->ReferenceCount)
        {
            DPRINT("Device object still has %d references\n", DeviceObject->ReferenceCount);
            return;
        }

        /* Check if we have a Security Descriptor */
        if (DeviceObject->SecurityDescriptor)
        {
            /* Dereference it */
            ObDereferenceSecurityDescriptor(DeviceObject->SecurityDescriptor, 1);
        }

        /* Remove the device from the list */
        IopEditDeviceList(DeviceObject->DriverObject, DeviceObject, IopRemove);

        /* Dereference the keep-alive */
        ObDereferenceObject(DeviceObject);
    }

    /* We can't unload a non-PnP driver here */
    if (DriverObject->Flags & DRVO_LEGACY_DRIVER)
    {
        DPRINT("Not a PnP driver! '%wZ' will not be unloaded!\n", &DriverObject->DriverName);
        return;
    }

    /* Return if we've already called unload (maybe we're in it?) */
    if (DriverObject->Flags & DRVO_UNLOAD_INVOKED) return;

    /* We can't unload unless there's an unload handler */
    if (!DriverObject->DriverUnload)
    {
        DPRINT1("No DriverUnload function on PnP driver! '%wZ' will not be unloaded!\n", &DriverObject->DriverName);
        return;
    }

    /* Bail if there are still devices present */
    if (DriverObject->DeviceObject)
    {
        DPRINT("Devices still present! '%wZ' will not be unloaded!\n", &DriverObject->DriverName);
        return;
    }

    DPRINT1("Unloading driver '%wZ' (automatic)\n", &DriverObject->DriverName);

    /* Set the unload invoked flag */
    DriverObject->Flags |= DRVO_UNLOAD_INVOKED;

    /* Unload it */
    DriverObject->DriverUnload(DriverObject);

    /* Make object temporary so it can be deleted */
    ObMakeTemporaryObject(DriverObject);
}

VOID
NTAPI
IopDereferenceDeviceObject(IN PDEVICE_OBJECT DeviceObject,
                           IN BOOLEAN ForceUnload)
{
    /* Sanity check */
    ASSERT(DeviceObject->ReferenceCount);

    /* Dereference the device */
    InterlockedDecrement(&DeviceObject->ReferenceCount);

    /*
     * Check if we can unload it and it's safe to unload (or if we're forcing
     * an unload, which is OK too).
     */
    ASSERT(!ForceUnload);
    if (!(DeviceObject->ReferenceCount) &&
        (IoGetDevObjExtension(DeviceObject)->ExtensionFlags & DOE_DELETE_PENDING))
    {
        /* Unload it */
        IopUnloadDevice(DeviceObject);
    }
}

VOID
NTAPI
IopStartNextPacketByKey(IN PDEVICE_OBJECT DeviceObject,
                        IN BOOLEAN Cancelable,
                        IN ULONG Key)
{
    PKDEVICE_QUEUE_ENTRY Entry;
    PIRP Irp;
    KIRQL OldIrql;

    /* Acquire the cancel lock if this is cancelable */
    if (Cancelable) IoAcquireCancelSpinLock(&OldIrql);

    /* Clear the current IRP */
    DeviceObject->CurrentIrp = NULL;

    /* Remove an entry from the queue */
    Entry = KeRemoveByKeyDeviceQueue(&DeviceObject->DeviceQueue, Key);
    if (Entry)
    {
        /* Get the IRP and set it */
        Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DeviceQueueEntry);
        DeviceObject->CurrentIrp = Irp;

        /* Check if this is a cancelable packet */
        if (Cancelable)
        {
            /* Check if the caller requested no cancellation */
            if (IoGetDevObjExtension(DeviceObject)->StartIoFlags &
                DOE_SIO_NO_CANCEL)
            {
                /* He did, so remove the cancel routine */
                Irp->CancelRoutine = NULL;
            }

            /* Release the cancel lock */
            IoReleaseCancelSpinLock(OldIrql);
        }

        /* Call the Start I/O Routine */
        DeviceObject->DriverObject->DriverStartIo(DeviceObject, Irp);
    }
    else
    {
        /* Otherwise, release the cancel lock if we had acquired it */
        if (Cancelable) IoReleaseCancelSpinLock(OldIrql);
    }
}

VOID
NTAPI
IopStartNextPacket(IN PDEVICE_OBJECT DeviceObject,
                   IN BOOLEAN Cancelable)
{
    PKDEVICE_QUEUE_ENTRY Entry;
    PIRP Irp;
    KIRQL OldIrql;

    /* Acquire the cancel lock if this is cancelable */
    if (Cancelable) IoAcquireCancelSpinLock(&OldIrql);

    /* Clear the current IRP */
    DeviceObject->CurrentIrp = NULL;

    /* Remove an entry from the queue */
    Entry = KeRemoveDeviceQueue(&DeviceObject->DeviceQueue);
    if (Entry)
    {
        /* Get the IRP and set it */
        Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.DeviceQueueEntry);
        DeviceObject->CurrentIrp = Irp;

        /* Check if this is a cancelable packet */
        if (Cancelable)
        {
            /* Check if the caller requested no cancellation */
            if (IoGetDevObjExtension(DeviceObject)->StartIoFlags &
                DOE_SIO_NO_CANCEL)
            {
                /* He did, so remove the cancel routine */
                Irp->CancelRoutine = NULL;
            }

            /* Release the cancel lock */
            IoReleaseCancelSpinLock(OldIrql);
        }

        /* Call the Start I/O Routine */
        DeviceObject->DriverObject->DriverStartIo(DeviceObject, Irp);
    }
    else
    {
        /* Otherwise, release the cancel lock if we had acquired it */
        if (Cancelable) IoReleaseCancelSpinLock(OldIrql);
    }
}

VOID
NTAPI
IopStartNextPacketByKeyEx(IN PDEVICE_OBJECT DeviceObject,
                          IN ULONG Key,
                          IN ULONG Flags)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    ULONG CurrentKey = Key;
    ULONG CurrentFlags = Flags;

    /* Get the device extension and start the packet loop */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);
    while (TRUE)
    {
        /* Increase the count */
        if (InterlockedIncrement(&DeviceExtension->StartIoCount) > 1)
        {
            /*
             * We've already called the routine once...
             * All we have to do is save the key and add the new flags
             */
            DeviceExtension->StartIoFlags |= CurrentFlags;
            DeviceExtension->StartIoKey = CurrentKey;
        }
        else
        {
            /* Mask out the current packet flags and key */
            DeviceExtension->StartIoFlags &= ~(DOE_SIO_WITH_KEY |
                                               DOE_SIO_NO_KEY |
                                               DOE_SIO_CANCELABLE);
            DeviceExtension->StartIoKey = 0;

            /* Check if this is a packet start with key */
            if (Flags & DOE_SIO_WITH_KEY)
            {
                /* Start the packet with a key */
                IopStartNextPacketByKey(DeviceObject,
                                        (Flags & DOE_SIO_CANCELABLE) ?
                                        TRUE : FALSE,
                                        CurrentKey);
            }
            else if (Flags & DOE_SIO_NO_KEY)
            {
                /* Start the packet */
                IopStartNextPacket(DeviceObject,
                                   (Flags & DOE_SIO_CANCELABLE) ?
                                   TRUE : FALSE);
            }
        }

        /* Decrease the Start I/O count and check if it's 0 now */
        if (!InterlockedDecrement(&DeviceExtension->StartIoCount))
        {
            /* Get the current active key and flags */
            CurrentKey = DeviceExtension->StartIoKey;
            CurrentFlags = DeviceExtension->StartIoFlags & (DOE_SIO_WITH_KEY |
                                                            DOE_SIO_NO_KEY |
                                                            DOE_SIO_CANCELABLE);

            /* Check if we should still loop */
            if (!(CurrentFlags & (DOE_SIO_WITH_KEY | DOE_SIO_NO_KEY))) break;
        }
        else
        {
            /* There are still Start I/Os active, so quit this loop */
            break;
        }
    }
}

NTSTATUS
NTAPI
IopGetRelatedTargetDevice(IN PFILE_OBJECT FileObject,
                          OUT PDEVICE_NODE *DeviceNode)
{
    NTSTATUS Status;
    IO_STACK_LOCATION Stack = {0};
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_OBJECT DeviceObject = NULL;

    ASSERT(FileObject);

    /* Get DeviceObject related to given FileObject */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject) return STATUS_NO_SUCH_DEVICE;

    /* Define input parameters */
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    Stack.Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    Stack.FileObject = FileObject;

    /* Call the driver to query all relations (IRP_MJ_PNP) */
    Status = IopSynchronousCall(DeviceObject,
                                &Stack,
                                (PVOID)&DeviceRelations);
    if (!NT_SUCCESS(Status)) return Status;

    /* Make sure it's not NULL and contains only one object */
    ASSERT(DeviceRelations);
    ASSERT(DeviceRelations->Count == 1);

    /* Finally get the device node */
    *DeviceNode = IopGetDeviceNode(DeviceRelations->Objects[0]);
    if (!*DeviceNode) Status = STATUS_NO_SUCH_DEVICE;

    /* Free the DEVICE_RELATIONS structure, it's not needed anymore */
    ExFreePool(DeviceRelations);

    return Status;
}

BOOLEAN
NTAPI
IopVerifyDeviceObjectOnStack(IN PDEVICE_OBJECT BaseDeviceObject,
                             IN PDEVICE_OBJECT TopDeviceObjectHint)
{
    KIRQL OldIrql;
    BOOLEAN Result;
    PDEVICE_OBJECT LoopObject;

    ASSERT(BaseDeviceObject != NULL);

    Result = FALSE;
    /* Simply loop on the device stack and try to find our hint */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);
    for (LoopObject = BaseDeviceObject; ; LoopObject = LoopObject->AttachedDevice)
    {
        /* It was found, it's a success */
        if (LoopObject == TopDeviceObjectHint)
        {
            Result = TRUE;
            break;
        }

        /* End of the stack, that's a failure - default */
        if (LoopObject == NULL)
        {
            break;
        }
    }
    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);

    return Result;
}

NTSTATUS
NTAPI
IopCreateSecurityDescriptorPerType(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                   IN SECURITY_DESCRIPTOR_TYPE Type,
                                   OUT PULONG OutputFlags)
{
    PACL Dacl;
    NTSTATUS Status;

    /* Select the DACL the caller wants */
    switch (Type)
    {
        case RestrictedPublic:
            Dacl = SePublicDefaultDacl;
            break;

        case UnrestrictedPublic:
            Dacl = SePublicDefaultUnrestrictedDacl;
            break;

        case RestrictedPublicOpen:
            Dacl = SePublicOpenDacl;
            break;

        case UnrestrictedPublicOpen:
            Dacl = SePublicOpenUnrestrictedDacl;
            break;

        case SystemDefault:
            Dacl = SeSystemDefaultDacl;
            break;

        default:
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
    }

    /* Create the SD and set the DACL caller wanted */
    Status = RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Dacl, FALSE);

    /* We've set DACL */
    if (OutputFlags) *OutputFlags |= DACL_SET;

    /* Done */
    return Status;
}

PSECURITY_DESCRIPTOR
NTAPI
IopCreateDefaultDeviceSecurityDescriptor(IN DEVICE_TYPE DeviceType,
                                         IN ULONG DeviceCharacteristics,
                                         IN BOOLEAN HasDeviceName,
                                         IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                         OUT PACL * OutputDacl,
                                         OUT PULONG OutputFlags)
{
    PACL Dacl;
    ULONG AceId;
    NTSTATUS Status;
    PACCESS_ALLOWED_ACE Ace;
    BOOLEAN AdminsSet, WorldSet;

    PAGED_CODE();

    /* Zero our output vars */
    if (OutputFlags) *OutputFlags = 0;

    *OutputDacl = NULL;

    /* For FSD, easy use SePublicDefaultUnrestrictedDacl */
    if (DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM ||
        DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM ||
        DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ||
        DeviceType == FILE_DEVICE_FILE_SYSTEM)
    {
        Status = IopCreateSecurityDescriptorPerType(SecurityDescriptor,
                                                    UnrestrictedPublic,
                                                    OutputFlags);
        goto Quit;
    }
    /* For storage devices with a name and floppy attribute,
     * use SePublicOpenUnrestrictedDacl
     */
    else if ((DeviceType != FILE_DEVICE_VIRTUAL_DISK &&
              DeviceType != FILE_DEVICE_MASS_STORAGE &&
              DeviceType != FILE_DEVICE_CD_ROM &&
              DeviceType != FILE_DEVICE_DISK &&
              DeviceType != FILE_DEVICE_DFS_FILE_SYSTEM &&
              DeviceType != FILE_DEVICE_NETWORK &&
              DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM) ||
              (HasDeviceName && BooleanFlagOn(DeviceCharacteristics, FILE_FLOPPY_DISKETTE)))
    {
        Status = IopCreateSecurityDescriptorPerType(SecurityDescriptor,
                                                    UnrestrictedPublicOpen,
                                                    OutputFlags);
        goto Quit;
    }

    /* The rest...
     * We will rely on SePublicDefaultUnrestrictedDacl as well
     */
    Dacl = ExAllocatePoolWithTag(PagedPool, SePublicDefaultUnrestrictedDacl->AclSize, 'eSoI');
    if (Dacl == NULL)
    {
        return NULL;
    }

    /* Copy our DACL */
    RtlCopyMemory(Dacl, SePublicDefaultUnrestrictedDacl, SePublicDefaultUnrestrictedDacl->AclSize);

    /* Now, browse the DACL to make sure we have everything we want in them,
     * including permissions
     */
    AceId = 0;
    AdminsSet = FALSE;
    WorldSet = FALSE;
    while (NT_SUCCESS(RtlGetAce(Dacl, AceId, (PVOID *)&Ace)))
    {
        /* Admins must acess and in RWX, set it */
        if (RtlEqualSid(SeAliasAdminsSid, &Ace->SidStart))
        {
            SetFlag(Ace->Mask, (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE));
            AdminsSet = TRUE;
        }

        /* World can read a CD_ROM device */
        if (DeviceType == FILE_DEVICE_CD_ROM && RtlEqualSid(SeWorldSid, &Ace->SidStart))
        {
            SetFlag(Ace->Mask, GENERIC_READ);
            WorldSet = TRUE;
        }

        ++AceId;
    }

    /* AdminSid was present and set (otherwise, we have big trouble) */
    ASSERT(AdminsSet);

    /* If CD_ROM device, we've set world permissions */
    if (DeviceType == FILE_DEVICE_CD_ROM) ASSERT(WorldSet);

    /* Now our DACL is correct, setup the security descriptor */
    RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, Dacl, FALSE);

    /* We've set DACL */
    if (OutputFlags) *OutputFlags |= DACL_SET;

    /* Return DACL to allow later freeing */
    *OutputDacl = Dacl;
    Status = STATUS_SUCCESS;

Quit:
    /* Only return SD if we succeed */
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    return SecurityDescriptor;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * IoAttachDevice
 *
 * Layers a device over the highest device in a device stack.
 *
 * Parameters
 *    SourceDevice
 *       Device to be attached.
 *
 *    TargetDevice
 *       Name of the target device.
 *
 *    AttachedDevice
 *       Caller storage for the device attached to.
 *
 * Status
 *    @implemented
 */
NTSTATUS
NTAPI
IoAttachDevice(PDEVICE_OBJECT SourceDevice,
               PUNICODE_STRING TargetDeviceName,
               PDEVICE_OBJECT *AttachedDevice)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject = NULL;
    PDEVICE_OBJECT TargetDevice = NULL;

    /* Call the helper routine for an attach operation */
    Status = IopGetDeviceObjectPointer(TargetDeviceName,
                                       FILE_READ_ATTRIBUTES,
                                       &FileObject,
                                       &TargetDevice,
                                       IO_ATTACH_DEVICE_API);
    if (!NT_SUCCESS(Status)) return Status;

    /* Attach the device */
    Status = IoAttachDeviceToDeviceStackSafe(SourceDevice,
                                             TargetDevice,
                                             AttachedDevice);

    /* Dereference it */
    ObDereferenceObject(FileObject);
    return Status;
}

/*
 * IoAttachDeviceByPointer
 *
 * Status
 *    @implemented
 */
NTSTATUS
NTAPI
IoAttachDeviceByPointer(IN PDEVICE_OBJECT SourceDevice,
                        IN PDEVICE_OBJECT TargetDevice)
{
    PDEVICE_OBJECT AttachedDevice;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Do the Attach */
    AttachedDevice = IoAttachDeviceToDeviceStack(SourceDevice, TargetDevice);
    if (!AttachedDevice) Status = STATUS_NO_SUCH_DEVICE;

    /* Return the status */
    return Status;
}

/*
 * @implemented
 */
PDEVICE_OBJECT
NTAPI
IoAttachDeviceToDeviceStack(IN PDEVICE_OBJECT SourceDevice,
                            IN PDEVICE_OBJECT TargetDevice)
{
    /* Attach it safely */
    return IopAttachDeviceToDeviceStackSafe(SourceDevice,
                                            TargetDevice,
                                            NULL);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAttachDeviceToDeviceStackSafe(IN PDEVICE_OBJECT SourceDevice,
                                IN PDEVICE_OBJECT TargetDevice,
                                IN OUT PDEVICE_OBJECT *AttachedToDeviceObject)
{
    /* Call the internal function */
    if (!IopAttachDeviceToDeviceStackSafe(SourceDevice,
                                          TargetDevice,
                                          AttachedToDeviceObject))
    {
        /* Nothing found */
        return STATUS_NO_SUCH_DEVICE;
    }

    /* Success! */
    return STATUS_SUCCESS;
}

/*
 * IoCreateDevice
 *
 * Allocates memory for and intializes a device object for use for
 * a driver.
 *
 * Parameters
 *    DriverObject
 *       Driver object passed by IO Manager when the driver was loaded.
 *
 *    DeviceExtensionSize
 *       Number of bytes for the device extension.
 *
 *    DeviceName
 *       Unicode name of device.
 *
 *    DeviceType
 *       Device type of the new device.
 *
 *    DeviceCharacteristics
 *       Bit mask of device characteristics.
 *
 *    Exclusive
 *       TRUE if only one thread can access the device at a time.
 *
 *    DeviceObject
 *       On successful return this parameter is filled by pointer to
 *       allocated device object.
 *
 * Status
 *    @implemented
 */
NTSTATUS
NTAPI
IoCreateDevice(IN PDRIVER_OBJECT DriverObject,
               IN ULONG DeviceExtensionSize,
               IN PUNICODE_STRING DeviceName,
               IN DEVICE_TYPE DeviceType,
               IN ULONG DeviceCharacteristics,
               IN BOOLEAN Exclusive,
               OUT PDEVICE_OBJECT *DeviceObject)
{
    WCHAR AutoNameBuffer[20];
    UNICODE_STRING AutoName;
    PDEVICE_OBJECT CreatedDeviceObject;
    PDEVOBJ_EXTENSION DeviceObjectExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    ULONG AlignedDeviceExtensionSize;
    ULONG TotalSize;
    HANDLE TempHandle;
    PACL Dacl;
    SECURITY_DESCRIPTOR SecurityDescriptor, *ReturnedSD;
    PAGED_CODE();

    /* Check if we have to generate a name */
    if (DeviceCharacteristics & FILE_AUTOGENERATED_DEVICE_NAME)
    {
        /* Generate it */
        swprintf(AutoNameBuffer,
                 L"\\Device\\%08lx",
                 InterlockedIncrementUL(&IopDeviceObjectNumber));

        /* Initialize the name */
        RtlInitUnicodeString(&AutoName, AutoNameBuffer);
        DeviceName = &AutoName;
    }

    /* Get the security descriptor */
    ReturnedSD = IopCreateDefaultDeviceSecurityDescriptor(DeviceType,
                                                          DeviceCharacteristics,
                                                          DeviceName != NULL,
                                                          &SecurityDescriptor,
                                                          &Dacl,
                                                          NULL);

    /* Initialize the Object Attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               DeviceName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               ReturnedSD);

    /* Honor exclusive flag */
    if (Exclusive) ObjectAttributes.Attributes |= OBJ_EXCLUSIVE;

    /* Create a permanent object for named devices */
    if (DeviceName) ObjectAttributes.Attributes |= OBJ_PERMANENT;

    /* Align the Extension Size to 8-bytes */
    AlignedDeviceExtensionSize = ALIGN_UP_BY(DeviceExtensionSize,
                                             MEMORY_ALLOCATION_ALIGNMENT);

    /* Total Size */
    TotalSize = AlignedDeviceExtensionSize +
                sizeof(DEVICE_OBJECT) +
                sizeof(EXTENDED_DEVOBJ_EXTENSION);

    /* Create the Device Object */
    *DeviceObject = NULL;
    Status = ObCreateObject(KernelMode,
                            IoDeviceObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            TotalSize,
                            0,
                            0,
                            (PVOID*)&CreatedDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        if (Dacl != NULL) ExFreePoolWithTag(Dacl, 'eSoI');

        return Status;
    }

    /* Clear the whole Object and extension so we don't null stuff manually */
    RtlZeroMemory(CreatedDeviceObject, TotalSize);

    /*
     * Setup the Type and Size. Note that we don't use the aligned size,
     * because that's only padding for the DevObjExt and not part of the Object.
     */
    CreatedDeviceObject->Type = IO_TYPE_DEVICE;
    CreatedDeviceObject->Size = sizeof(DEVICE_OBJECT) + (USHORT)DeviceExtensionSize;

    /* The kernel extension is after the driver internal extension */
    DeviceObjectExtension = (PDEVOBJ_EXTENSION)
                            ((ULONG_PTR)(CreatedDeviceObject + 1) +
                             AlignedDeviceExtensionSize);

    /* Set the Type and Size. Question: why is Size 0 on Windows? */
    DeviceObjectExtension->Type = IO_TYPE_DEVICE_OBJECT_EXTENSION;
    DeviceObjectExtension->Size = 0;

    /* Initialize with Power Manager */
    PoInitializeDeviceObject(DeviceObjectExtension);

    /* Link the Object and Extension */
    DeviceObjectExtension->DeviceObject = CreatedDeviceObject;
    CreatedDeviceObject->DeviceObjectExtension = DeviceObjectExtension;

    /* Set Device Object Data */
    CreatedDeviceObject->DeviceType = DeviceType;
    CreatedDeviceObject->Characteristics = DeviceCharacteristics;
    CreatedDeviceObject->DeviceExtension = DeviceExtensionSize ?
                                           CreatedDeviceObject + 1 :
                                           NULL;
    CreatedDeviceObject->StackSize = 1;
    CreatedDeviceObject->AlignmentRequirement = 0;

    /* Set the Flags */
    CreatedDeviceObject->Flags = DO_DEVICE_INITIALIZING;
    if (Exclusive) CreatedDeviceObject->Flags |= DO_EXCLUSIVE;
    if (DeviceName) CreatedDeviceObject->Flags |= DO_DEVICE_HAS_NAME;

    /* Attach a Vpb for Disks and Tapes, and create the Device Lock */
    if ((CreatedDeviceObject->DeviceType == FILE_DEVICE_DISK) ||
        (CreatedDeviceObject->DeviceType == FILE_DEVICE_VIRTUAL_DISK) ||
        (CreatedDeviceObject->DeviceType == FILE_DEVICE_CD_ROM) ||
        (CreatedDeviceObject->DeviceType == FILE_DEVICE_TAPE))
    {
        /* Create Vpb */
        Status = IopCreateVpb(CreatedDeviceObject);
        if (!NT_SUCCESS(Status))
        {
            if (Dacl != NULL) ExFreePoolWithTag(Dacl, 'eSoI');

            /* Dereference the device object and fail */
            ObDereferenceObject(CreatedDeviceObject);
            return Status;
        }

        /* Initialize Lock Event */
        KeInitializeEvent(&CreatedDeviceObject->DeviceLock,
                          SynchronizationEvent,
                          TRUE);
    }

    /* Set the right Sector Size */
    switch (DeviceType)
    {
        /* All disk systems */
        case FILE_DEVICE_DISK_FILE_SYSTEM:
        case FILE_DEVICE_DISK:
        case FILE_DEVICE_VIRTUAL_DISK:

            /* The default is 512 bytes */
            CreatedDeviceObject->SectorSize  = 512;
            break;

        /* CD-ROM file systems */
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:

            /* The default is 2048 bytes */
            CreatedDeviceObject->SectorSize = 2048;
    }

    /* Create the Device Queue */
    if ((CreatedDeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) ||
        (CreatedDeviceObject->DeviceType == FILE_DEVICE_FILE_SYSTEM) ||
        (CreatedDeviceObject->DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM) ||
        (CreatedDeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) ||
        (CreatedDeviceObject->DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM))
    {
        /* Simple FS Devices, they don't need a real Device Queue */
        InitializeListHead(&CreatedDeviceObject->Queue.ListEntry);
    }
    else
    {
        /* An actual Device, initialize its DQ */
        KeInitializeDeviceQueue(&CreatedDeviceObject->DeviceQueue);
    }

    /* Insert the Object */
    Status = ObInsertObject(CreatedDeviceObject,
                            NULL,
                            FILE_READ_DATA | FILE_WRITE_DATA,
                            1,
                            (PVOID*)&CreatedDeviceObject,
                            &TempHandle);
    if (!NT_SUCCESS(Status))
    {
        if (Dacl != NULL) ExFreePoolWithTag(Dacl, 'eSoI');

        return Status;
    }

    /* Now do the final linking */
    ObReferenceObject(DriverObject);
    ASSERT((DriverObject->Flags & DRVO_UNLOAD_INVOKED) == 0);
    CreatedDeviceObject->DriverObject = DriverObject;
    IopEditDeviceList(DriverObject, CreatedDeviceObject, IopAdd);

    /* Link with the power manager */
    if (CreatedDeviceObject->Vpb) PoVolumeDevice(CreatedDeviceObject);

    /* Close the temporary handle and return to caller */
    ObCloseHandle(TempHandle, KernelMode);
    *DeviceObject = CreatedDeviceObject;

    if (Dacl != NULL) ExFreePoolWithTag(Dacl, 'eSoI');

    return STATUS_SUCCESS;
}

/*
 * IoDeleteDevice
 *
 * Status
 *    @implemented
 */
VOID
NTAPI
IoDeleteDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PIO_TIMER Timer;

    /* Check if the device is registered for shutdown notifications */
    if (DeviceObject->Flags & DO_SHUTDOWN_REGISTERED)
    {
        /* Call the shutdown notifications */
        IoUnregisterShutdownNotification(DeviceObject);
    }

    /* Check if it has a timer */
    Timer = DeviceObject->Timer;
    if (Timer)
    {
        /* Remove it and free it */
        IopRemoveTimerFromTimerList(Timer);
        ExFreePoolWithTag(Timer, TAG_IO_TIMER);
    }

    /* Check if the device has a name */
    if (DeviceObject->Flags & DO_DEVICE_HAS_NAME)
    {
        /* It does, make it temporary so we can remove it */
        ObMakeTemporaryObject(DeviceObject);
    }

    /* Set the pending delete flag */
    IoGetDevObjExtension(DeviceObject)->ExtensionFlags |= DOE_DELETE_PENDING;

    /* Unlink with the power manager */
    if (DeviceObject->Vpb) PoRemoveVolumeDevice(DeviceObject);

    /* Check if the device object can be unloaded */
    if (!DeviceObject->ReferenceCount) IopUnloadDevice(DeviceObject);
}

/*
 * IoDetachDevice
 *
 * Status
 *    @implemented
 */
VOID
NTAPI
IoDetachDevice(IN PDEVICE_OBJECT TargetDevice)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;

    /* Sanity check */
    DeviceExtension = IoGetDevObjExtension(TargetDevice->AttachedDevice);
    ASSERT(DeviceExtension->AttachedTo == TargetDevice);

    /* Remove the attachment */
    DeviceExtension->AttachedTo = NULL;
    TargetDevice->AttachedDevice = NULL;

    /* Check if it's ok to delete this device */
    if ((IoGetDevObjExtension(TargetDevice)->ExtensionFlags & DOE_DELETE_PENDING) &&
        !(TargetDevice->ReferenceCount))
    {
        /* It is, do it */
        IopUnloadDevice(TargetDevice);
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoEnumerateDeviceObjectList(IN  PDRIVER_OBJECT DriverObject,
                            IN  PDEVICE_OBJECT *DeviceObjectList,
                            IN  ULONG DeviceObjectListSize,
                            OUT PULONG ActualNumberDeviceObjects)
{
    ULONG ActualDevices = 1;
    PDEVICE_OBJECT CurrentDevice = DriverObject->DeviceObject;
    KIRQL OldIrql;

    /* Lock the Device list while we enumerate it */
    OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);

    /* Find out how many devices we'll enumerate */
    while ((CurrentDevice = CurrentDevice->NextDevice)) ActualDevices++;

    /* Go back to the first */
    CurrentDevice = DriverObject->DeviceObject;

    /* Start by at least returning this */
    *ActualNumberDeviceObjects = ActualDevices;

    /* Check if we can support so many */
    if ((ActualDevices * sizeof(PDEVICE_OBJECT)) > DeviceObjectListSize)
    {
        /* Fail because the buffer was too small */
        KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Check if the caller wanted the device list */
    if (DeviceObjectList)
    {
        /* Loop through all the devices */
        while (ActualDevices)
        {
            /* Reference each Device */
            ObReferenceObject(CurrentDevice);

            /* Add it to the list */
            *DeviceObjectList = CurrentDevice;

            /* Go to the next one */
            CurrentDevice = CurrentDevice->NextDevice;
            ActualDevices--;
            DeviceObjectList++;
        }
    }

    /* Release the device list lock */
    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);

    /* Return the status */
    return STATUS_SUCCESS;
}

/*
 * IoGetAttachedDevice
 *
 * Status
 *    @implemented
 */
PDEVICE_OBJECT
NTAPI
IoGetAttachedDevice(PDEVICE_OBJECT DeviceObject)
{
    /* Get the last attached device */
    while (DeviceObject->AttachedDevice)
    {
        /* Move to the next one */
        DeviceObject = DeviceObject->AttachedDevice;
    }

    /* Return it */
    return DeviceObject;
}

/*
 * IoGetAttachedDeviceReference
 *
 * Status
 *    @implemented
 */
PDEVICE_OBJECT
NTAPI
IoGetAttachedDeviceReference(PDEVICE_OBJECT DeviceObject)
{
    /* Reference the Attached Device */
    DeviceObject = IoGetAttachedDevice(DeviceObject);
    ObReferenceObject(DeviceObject);
    return DeviceObject;
}

/*
 * @implemented
 */
PDEVICE_OBJECT
NTAPI
IoGetDeviceAttachmentBaseRef(IN PDEVICE_OBJECT DeviceObject)
{
    /* Reference the lowest attached device */
    DeviceObject = IopGetLowestDevice(DeviceObject);
    ObReferenceObject(DeviceObject);
    return DeviceObject;
}

/*
 * IoGetDeviceObjectPointer
 *
 * Status
 *    @implemented
 */
NTSTATUS
NTAPI
IoGetDeviceObjectPointer(IN PUNICODE_STRING ObjectName,
                         IN ACCESS_MASK DesiredAccess,
                         OUT PFILE_OBJECT *FileObject,
                         OUT PDEVICE_OBJECT *DeviceObject)
{
    /* Call the helper routine for a normal operation */
    return IopGetDeviceObjectPointer(ObjectName,
                                     DesiredAccess,
                                     FileObject,
                                     DeviceObject,
                                     0);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoGetDiskDeviceObject(IN PDEVICE_OBJECT FileSystemDeviceObject,
                      OUT PDEVICE_OBJECT *DiskDeviceObject)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    PVPB Vpb;
    KIRQL OldIrql;
    NTSTATUS Status;

    /* Make sure there's a VPB */
    if (!FileSystemDeviceObject->Vpb) return STATUS_INVALID_PARAMETER;

    /* Acquire it */
    IoAcquireVpbSpinLock(&OldIrql);

    /* Get the Device Extension */
    DeviceExtension = IoGetDevObjExtension(FileSystemDeviceObject);

    /* Make sure this one has a VPB too */
    Vpb = DeviceExtension->Vpb;
    if (Vpb)
    {
        /* Make sure that it's mounted */
        if ((Vpb->ReferenceCount) &&
            (Vpb->Flags & VPB_MOUNTED))
        {
            /* Return the Disk Device Object */
            *DiskDeviceObject = Vpb->RealDevice;

            /* Reference it and return success */
            ObReferenceObject(Vpb->RealDevice);
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* It's not, so return failure */
            Status = STATUS_VOLUME_DISMOUNTED;
        }
    }
    else
    {
        /* Fail */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Release the lock */
    IoReleaseVpbSpinLock(OldIrql);
    return Status;
}

/*
 * @implemented
 */
PDEVICE_OBJECT
NTAPI
IoGetLowerDeviceObject(IN PDEVICE_OBJECT DeviceObject)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    PDEVICE_OBJECT LowerDeviceObject = NULL;

    /* Make sure it's not getting deleted */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);
    if (!(DeviceExtension->ExtensionFlags & (DOE_UNLOAD_PENDING |
                                           DOE_DELETE_PENDING |
                                           DOE_REMOVE_PENDING |
                                           DOE_REMOVE_PROCESSED)))
    {
        /* Get the Lower Device Object */
        LowerDeviceObject = DeviceExtension->AttachedTo;

        /* Check that we got a valid device object */
        if (LowerDeviceObject)
        {
            /* We did so let's reference it */
            ObReferenceObject(LowerDeviceObject);
        }
    }

    /* Return it */
    return LowerDeviceObject;
}

/*
 * @implemented
 */
PDEVICE_OBJECT
NTAPI
IoGetRelatedDeviceObject(IN PFILE_OBJECT FileObject)
{
    PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;

    /* Check if we have a VPB with a device object */
    if ((FileObject->Vpb) && (FileObject->Vpb->DeviceObject))
    {
        /* Then use the DO from the VPB */
        ASSERT(!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN));
        DeviceObject = FileObject->Vpb->DeviceObject;
    }
    else if (!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN) &&
              (FileObject->DeviceObject->Vpb) &&
              (FileObject->DeviceObject->Vpb->DeviceObject))
    {
        /* The disk device actually has a VPB, so get the DO from there */
        DeviceObject = FileObject->DeviceObject->Vpb->DeviceObject;
    }
    else
    {
        /* Otherwise, this was a direct open */
        DeviceObject = FileObject->DeviceObject;
    }

    /* Sanity check */
    ASSERT(DeviceObject != NULL);

    /* Check if we were attached */
    if (DeviceObject->AttachedDevice)
    {
        /* Check if the file object has an extension present */
        if (FileObject->Flags & FO_FILE_OBJECT_HAS_EXTENSION)
        {
            /* Sanity check, direct open files can't have this */
            ASSERT(!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN));

            /* Check if the extension is really present */
            if (FileObject->FileObjectExtension)
            {
                PFILE_OBJECT_EXTENSION FileObjectExtension;

                /* Cast the buffer to something we understand */
                FileObjectExtension = FileObject->FileObjectExtension;

                /* Check if have a valid replacement top level device */
                if (FileObjectExtension->TopDeviceObjectHint &&
                    IopVerifyDeviceObjectOnStack(DeviceObject,
                                                 FileObjectExtension->TopDeviceObjectHint))
                {
                    /* Use this instead of returning the top level device */
                    return FileObjectExtension->TopDeviceObjectHint;
                }
            }
        }

        /* Return the highest attached device */
        DeviceObject = IoGetAttachedDevice(DeviceObject);
    }

    /* Return the DO we found */
    return DeviceObject;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoGetRelatedTargetDevice(IN PFILE_OBJECT FileObject,
                         OUT PDEVICE_OBJECT *DeviceObject)
{
    NTSTATUS Status;
    PDEVICE_NODE DeviceNode = NULL;

    /* Call the internal helper function */
    Status = IopGetRelatedTargetDevice(FileObject, &DeviceNode);
    if (NT_SUCCESS(Status) && DeviceNode)
    {
        *DeviceObject = DeviceNode->PhysicalDeviceObject;
    }
    return Status;
}

/*
 * @implemented
 */
PDEVICE_OBJECT
NTAPI
IoGetBaseFileSystemDeviceObject(IN PFILE_OBJECT FileObject)
{
    PDEVICE_OBJECT DeviceObject;

    /*
    * If the FILE_OBJECT's VPB is defined,
    * get the device from it.
    */
    if ((FileObject->Vpb) && (FileObject->Vpb->DeviceObject))
    {
        /* Use the VPB's Device Object's */
        DeviceObject = FileObject->Vpb->DeviceObject;
    }
    else if (!(FileObject->Flags & FO_DIRECT_DEVICE_OPEN) &&
             (FileObject->DeviceObject->Vpb) &&
             (FileObject->DeviceObject->Vpb->DeviceObject))
    {
        /* Use the VPB's File System Object */
        DeviceObject = FileObject->DeviceObject->Vpb->DeviceObject;
    }
    else
    {
        /* Use the FO's Device Object */
        DeviceObject = FileObject->DeviceObject;
    }

    /* Return the device object we found */
    ASSERT(DeviceObject != NULL);
    return DeviceObject;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoRegisterLastChanceShutdownNotification(IN PDEVICE_OBJECT DeviceObject)
{
    PSHUTDOWN_ENTRY Entry;

    /* Allocate the shutdown entry */
    Entry = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(SHUTDOWN_ENTRY),
                                  TAG_SHUTDOWN_ENTRY);
    if (!Entry) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set the DO */
    Entry->DeviceObject = DeviceObject;

    /* Reference it so it doesn't go away */
    ObReferenceObject(DeviceObject);

    /* Insert it into the list */
    ExInterlockedInsertHeadList(&LastChanceShutdownListHead,
                                &Entry->ShutdownList,
                                &ShutdownListLock);

    /* Set the shutdown registered flag */
    DeviceObject->Flags |= DO_SHUTDOWN_REGISTERED;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoRegisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
    PSHUTDOWN_ENTRY Entry;

    /* Allocate the shutdown entry */
    Entry = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(SHUTDOWN_ENTRY),
                                  TAG_SHUTDOWN_ENTRY);
    if (!Entry) return STATUS_INSUFFICIENT_RESOURCES;

    /* Set the DO */
    Entry->DeviceObject = DeviceObject;

    /* Reference it so it doesn't go away */
    ObReferenceObject(DeviceObject);

    /* Insert it into the list */
    ExInterlockedInsertHeadList(&ShutdownListHead,
                                &Entry->ShutdownList,
                                &ShutdownListLock);

    /* Set the shutdown registered flag */
    DeviceObject->Flags |= DO_SHUTDOWN_REGISTERED;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IoUnregisterShutdownNotification(PDEVICE_OBJECT DeviceObject)
{
    PSHUTDOWN_ENTRY ShutdownEntry;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;

    /* Remove the flag */
    DeviceObject->Flags &= ~DO_SHUTDOWN_REGISTERED;

    /* Acquire the shutdown lock and loop the shutdown list */
    KeAcquireSpinLock(&ShutdownListLock, &OldIrql);
    NextEntry = ShutdownListHead.Flink;
    while (NextEntry != &ShutdownListHead)
    {
        /* Get the entry */
        ShutdownEntry = CONTAINING_RECORD(NextEntry,
                                          SHUTDOWN_ENTRY,
                                          ShutdownList);

        /* Get if the DO matches */
        if (ShutdownEntry->DeviceObject == DeviceObject)
        {
            /* Remove it from the list */
            RemoveEntryList(NextEntry);
            NextEntry = NextEntry->Blink;

            /* Free the entry */
            ExFreePoolWithTag(ShutdownEntry, TAG_SHUTDOWN_ENTRY);

            /* Get rid of our reference to it */
            ObDereferenceObject(DeviceObject);
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Now loop the last chance list */
    NextEntry = LastChanceShutdownListHead.Flink;
    while (NextEntry != &LastChanceShutdownListHead)
    {
        /* Get the entry */
        ShutdownEntry = CONTAINING_RECORD(NextEntry,
                                          SHUTDOWN_ENTRY,
                                          ShutdownList);

        /* Get if the DO matches */
        if (ShutdownEntry->DeviceObject == DeviceObject)
        {
            /* Remove it from the list */
            RemoveEntryList(NextEntry);
            NextEntry = NextEntry->Blink;

            /* Free the entry */
            ExFreePoolWithTag(ShutdownEntry, TAG_SHUTDOWN_ENTRY);

            /* Get rid of our reference to it */
            ObDereferenceObject(DeviceObject);
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Release the shutdown lock */
    KeReleaseSpinLock(&ShutdownListLock, OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
IoSetStartIoAttributes(IN PDEVICE_OBJECT DeviceObject,
                       IN BOOLEAN DeferredStartIo,
                       IN BOOLEAN NonCancelable)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;

    /* Get the Device Extension */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);

    /* Set the flags the caller requested */
    DeviceExtension->StartIoFlags |= (DeferredStartIo) ? DOE_SIO_DEFERRED : 0;
    DeviceExtension->StartIoFlags |= (NonCancelable) ? DOE_SIO_NO_CANCEL : 0;
}

/*
 * @implemented
 */
VOID
NTAPI
IoStartNextPacketByKey(IN PDEVICE_OBJECT DeviceObject,
                       IN BOOLEAN Cancelable,
                       IN ULONG Key)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;

    /* Get the Device Extension */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);

    /* Check if deferred start was requested */
    if (DeviceExtension->StartIoFlags & DOE_SIO_DEFERRED)
    {
        /* Call our internal function to handle the defered case */
        IopStartNextPacketByKeyEx(DeviceObject,
                                  Key,
                                  DOE_SIO_WITH_KEY |
                                  (Cancelable ? DOE_SIO_CANCELABLE : 0));
    }
    else
    {
        /* Call the normal routine */
        IopStartNextPacketByKey(DeviceObject, Cancelable, Key);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
IoStartNextPacket(IN PDEVICE_OBJECT DeviceObject,
                  IN BOOLEAN Cancelable)
{
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;

    /* Get the Device Extension */
    DeviceExtension = IoGetDevObjExtension(DeviceObject);

    /* Check if deferred start was requested */
    if (DeviceExtension->StartIoFlags & DOE_SIO_DEFERRED)
    {
        /* Call our internal function to handle the defered case */
        IopStartNextPacketByKeyEx(DeviceObject,
                                  0,
                                  DOE_SIO_NO_KEY |
                                  (Cancelable ? DOE_SIO_CANCELABLE : 0));
    }
    else
    {
        /* Call the normal routine */
        IopStartNextPacket(DeviceObject, Cancelable);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
IoStartPacket(IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp,
              IN PULONG Key,
              IN PDRIVER_CANCEL CancelFunction)
{
    BOOLEAN Stat;
    KIRQL OldIrql, CancelIrql;

    /* Raise to dispatch level */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* Check if we should acquire the cancel lock */
    if (CancelFunction)
    {
        /* Acquire and set it */
        IoAcquireCancelSpinLock(&CancelIrql);
        Irp->CancelRoutine = CancelFunction;
    }

    /* Check if we have a key */
    if (Key)
    {
        /* Insert by key */
        Stat = KeInsertByKeyDeviceQueue(&DeviceObject->DeviceQueue,
                                        &Irp->Tail.Overlay.DeviceQueueEntry,
                                        *Key);
    }
    else
    {
        /* Insert without a key */
        Stat = KeInsertDeviceQueue(&DeviceObject->DeviceQueue,
                                   &Irp->Tail.Overlay.DeviceQueueEntry);
    }

    /* Check if this was a first insert */
    if (!Stat)
    {
        /* Set the IRP */
        DeviceObject->CurrentIrp = Irp;

        /* Check if this is a cancelable packet */
        if (CancelFunction)
        {
            /* Check if the caller requested no cancellation */
            if (IoGetDevObjExtension(DeviceObject)->StartIoFlags &
                DOE_SIO_NO_CANCEL)
            {
                /* He did, so remove the cancel routine */
                Irp->CancelRoutine = NULL;
            }

            /* Release the cancel lock */
            IoReleaseCancelSpinLock(CancelIrql);
        }

        /* Call the Start I/O function */
        DeviceObject->DriverObject->DriverStartIo(DeviceObject, Irp);
    }
    else
    {
        /* The packet was inserted... check if we have a cancel function */
        if (CancelFunction)
        {
            /* Check if the IRP got cancelled */
            if (Irp->Cancel)
            {
                /*
                 * Set the cancel IRQL, clear the currnet cancel routine and
                 * call ours
                 */
                Irp->CancelIrql = CancelIrql;
                Irp->CancelRoutine = NULL;
                CancelFunction(DeviceObject, Irp);
            }
            else
            {
                /* Otherwise, release the lock */
                IoReleaseCancelSpinLock(CancelIrql);
            }
        }
    }

    /* Return back to previous IRQL */
    KeLowerIrql(OldIrql);
}

#if defined (_WIN64)
ULONG
NTAPI
IoWMIDeviceObjectToProviderId(
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return 0;
}
#endif

/* EOF */
