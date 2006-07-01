/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/device.c
 * PURPOSE:         Device Object Management, including Notifications and Queues.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ********************************************************************/

ULONG IopDeviceObjectNumber = 0;

typedef struct _SHUTDOWN_ENTRY
{
    LIST_ENTRY ShutdownList;
    PDEVICE_OBJECT DeviceObject;
} SHUTDOWN_ENTRY, *PSHUTDOWN_ENTRY;

LIST_ENTRY ShutdownListHead, LastChanceShutdownListHead;
KSPIN_LOCK ShutdownListLock;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
IoShutdownRegisteredDevices(VOID)
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

    /* Get the first entry and start looping */
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
        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            /* Wait on the driver */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }

        /* Free the shutdown entry and reset the event */
        ExFreePool(ShutdownEntry);
        KeClearEvent(&Event);

        /* Go to the next entry */
        ListEntry = ExInterlockedRemoveHeadList(&ShutdownListHead,
                                                &ShutdownListLock);
     }
}

NTSTATUS
FASTCALL
IopInitializeDevice(PDEVICE_NODE DeviceNode,
                    PDRIVER_OBJECT DriverObject)
{
   PDEVICE_OBJECT Fdo;
   NTSTATUS Status;
   BOOLEAN IsPnpDriver = FALSE;

   if (DriverObject->DriverExtension->AddDevice)
   {
      /* This is a Plug and Play driver */
      DPRINT("Plug and Play driver found\n");

      ASSERT(DeviceNode->PhysicalDeviceObject);

      DPRINT("Calling driver AddDevice entrypoint at %08lx\n",
         DriverObject->DriverExtension->AddDevice);

      IsPnpDriver = !IopDeviceNodeHasFlag(DeviceNode, DNF_LEGACY_DRIVER);
      Status = DriverObject->DriverExtension->AddDevice(
         DriverObject, IsPnpDriver ? DeviceNode->PhysicalDeviceObject : NULL);

      if (!NT_SUCCESS(Status))
      {
         return Status;
      }

      if (IsPnpDriver)
      {
         Fdo = IoGetAttachedDeviceReference(DeviceNode->PhysicalDeviceObject);

         if (Fdo == DeviceNode->PhysicalDeviceObject)
         {
            /* FIXME: What do we do? Unload the driver or just disable the device? */
            DbgPrint("An FDO was not attached\n");
            IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
            return STATUS_UNSUCCESSFUL;
         }

         if (Fdo->DeviceType == FILE_DEVICE_ACPI)
         {
            static BOOLEAN SystemPowerDeviceNodeCreated = FALSE;

            /* There can be only one system power device */
            if (!SystemPowerDeviceNodeCreated)
            {
               PopSystemPowerDeviceNode = DeviceNode;
               SystemPowerDeviceNodeCreated = TRUE;
            }
         }

         ObDereferenceObject(Fdo);
      }

      IopDeviceNodeSetFlag(DeviceNode, DNF_ADDED);
      IopDeviceNodeSetFlag(DeviceNode, DNF_NEED_ENUMERATION_ONLY);
   }

   return STATUS_SUCCESS;
}

NTSTATUS
IopStartDevice(
   PDEVICE_NODE DeviceNode)
{
   IO_STATUS_BLOCK IoStatusBlock;
   IO_STACK_LOCATION Stack;
   PDEVICE_OBJECT Fdo;
   NTSTATUS Status;

   DPRINT("Sending IRP_MN_START_DEVICE to driver\n");

   Fdo = IoGetAttachedDeviceReference(DeviceNode->PhysicalDeviceObject);
   Stack.Parameters.StartDevice.AllocatedResources = DeviceNode->ResourceList;
   Stack.Parameters.StartDevice.AllocatedResourcesTranslated = DeviceNode->ResourceListTranslated;

   Status = IopInitiatePnpIrp(
      Fdo,
      &IoStatusBlock,
      IRP_MN_START_DEVICE,
      &Stack);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopInitiatePnpIrp() failed\n");
   }
   else
   {
	  if (IopDeviceNodeHasFlag(DeviceNode, DNF_NEED_ENUMERATION_ONLY))
      {
         DPRINT("Device needs enumeration, invalidating bus relations\n");
         Status = IopInvalidateDeviceRelations(DeviceNode, BusRelations);
		 IopDeviceNodeClearFlag(DeviceNode, DNF_NEED_ENUMERATION_ONLY);
      }
   }

   ObDereferenceObject(Fdo);

   if (NT_SUCCESS(Status))
       DeviceNode->Flags |= DN_STARTED;

   return Status;
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
                               0,
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
        ZwClose(FileHandle);
    }

    /* Close the handle */
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
}

VOID
NTAPI
IopUnloadDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
    PDEVICE_OBJECT AttachedDeviceObject, LowestDeviceObject;
    PEXTENDED_DEVOBJ_EXTENSION ThisExtension, DeviceExtension;
    PDEVICE_NODE DeviceNode;
    BOOLEAN SafeToUnload = TRUE;

    /* Check if removal is pending */
    ThisExtension = IoGetDevObjExtension(DeviceObject);
    if (ThisExtension->ExtensionFlags & DOE_REMOVE_PENDING)
    {
        /* Get the PDO, extension, and node */
        LowestDeviceObject = IopGetLowestDevice(DeviceObject);
        DeviceExtension = IoGetDevObjExtension(LowestDeviceObject);
        DeviceNode = DeviceExtension->DeviceNode;

        /* The PDO needs a device node */
        ASSERT(DeviceNode != NULL);

        /* Loop all attached objects */
        AttachedDeviceObject = LowestDeviceObject;
        while (AttachedDeviceObject)
        {
            /* Make sure they're dereferenced */
            if (AttachedDeviceObject->ReferenceCount) return;
            AttachedDeviceObject = AttachedDeviceObject->AttachedDevice;
        }

        /* Loop all attached objects */
        AttachedDeviceObject = LowestDeviceObject;
        while (AttachedDeviceObject)
        {
            /* Get the device extension */
            DeviceExtension = IoGetDevObjExtension(AttachedDeviceObject);

            /* Remove the pending flag and set processed */
            DeviceExtension->ExtensionFlags &= ~DOE_REMOVE_PENDING;
            DeviceExtension->ExtensionFlags |= DOE_REMOVE_PROCESSED;
            AttachedDeviceObject = AttachedDeviceObject->AttachedDevice;
        }

        /*
         * FIXME: TODO HPOUSSIN
         * We need to parse/lock the device node, and if we have any pending
         * surprise removals, query all relationships and send IRP_MN_REMOVE_
         * _DEVICE to the devices related...
         */
        return;
    }

    /* Check if deletion is pending */
    if (ThisExtension->ExtensionFlags & DOE_DELETE_PENDING)
    {
        /* Make sure unload is pending */
        if (!(ThisExtension->ExtensionFlags & DOE_UNLOAD_PENDING) ||
            (DriverObject->Flags & DRVO_UNLOAD_INVOKED))
        {
            /* We can't unload anymore */
            SafeToUnload = FALSE;
        }

        /*
         * Check if we have an attached device and fail if we're attached
         * and still have a reference count.
         */
        AttachedDeviceObject = DeviceObject->AttachedDevice;
        if ((AttachedDeviceObject) && (DeviceObject->ReferenceCount)) return;

        /* Check if we have a Security Descriptor */
        if (DeviceObject->SecurityDescriptor)
        {
            /* Free it */
            ExFreePool(DeviceObject->SecurityDescriptor);
        }

        /* Remove the device from the list */
        IopEditDeviceList(DeviceObject->DriverObject, DeviceObject, IopRemove);

        /* Dereference the keep-alive */
        ObDereferenceObject(DeviceObject);

        /* If we're not unloading, stop here */
        if (!SafeToUnload) return;
    }

    /* Loop all the device objects */
    DeviceObject = DriverObject->DeviceObject;
    while (DeviceObject)
    {
        /*
         * Make sure we're not attached, having a reference count
         * or already deleting
         */
        if ((DeviceObject->ReferenceCount) ||
             (DeviceObject->AttachedDevice) ||
             (IoGetDevObjExtension(DeviceObject)->ExtensionFlags &
              (DOE_DELETE_PENDING | DOE_REMOVE_PENDING)))
        {
            /* We're not safe to unload, quit */
            return;
        }

        /* Check the next device */
        DeviceObject = DeviceObject->NextDevice;
    }

    /* Set the unload invoked flag */
    DriverObject->Flags |= DRVO_UNLOAD_INVOKED;

    /* Unload it */
    if (DriverObject->DriverUnload) DriverObject->DriverUnload(DriverObject);
}

VOID
NTAPI
IopDereferenceDeviceObject(IN PDEVICE_OBJECT DeviceObject,
                           IN BOOLEAN ForceUnload)
{
    /* Sanity check */
    ASSERT(DeviceObject->ReferenceCount);

    /* Dereference the device */
    DeviceObject->ReferenceCount--;

    /*
     * Check if we can unload it and it's safe to unload (or if we're forcing
     * an unload, which is OK too).
     */
    if (!(DeviceObject->ReferenceCount) &&
        ((ForceUnload) || (IoGetDevObjExtension(DeviceObject)->ExtensionFlags &
                           (DOE_UNLOAD_PENDING |
                            DOE_DELETE_PENDING |
                            DOE_REMOVE_PENDING |
                            DOE_REMOVE_PROCESSED))))
    {
        /* Unload it */
        IopUnloadDevice(DeviceObject);
    }
}

NTSTATUS
NTAPI
IopReferenceDeviceObject(IN PDEVICE_OBJECT DeviceObject)
{
    /* Make sure the object is valid */
    if ((IoGetDevObjExtension(DeviceObject)->ExtensionFlags &
        (DOE_UNLOAD_PENDING |
         DOE_DELETE_PENDING |
         DOE_REMOVE_PENDING |
         DOE_REMOVE_PROCESSED)) ||
        (DeviceObject->Flags & DO_DEVICE_INITIALIZING))
    {
        /* It's unloading or initializing, so fail */
        return STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        /* Increase reference count */
        DeviceObject->ReferenceCount++;
        return STATUS_SUCCESS;
    }
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
    if (!*AttachedDevice) Status = STATUS_NO_SUCH_DEVICE;

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
 * IoAttachDeviceToDeviceStack
 *
 * Status
 *    @implemented
 */
PDEVICE_OBJECT
NTAPI
IoAttachDeviceToDeviceStack(PDEVICE_OBJECT SourceDevice,
                            PDEVICE_OBJECT TargetDevice)
{
    NTSTATUS Status;
    PDEVICE_OBJECT LocalAttach;

    /* Attach it safely */
    Status = IoAttachDeviceToDeviceStackSafe(SourceDevice,
                                             TargetDevice,
                                             &LocalAttach);

    /* Return it */
    return LocalAttach;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAttachDeviceToDeviceStackSafe(IN PDEVICE_OBJECT SourceDevice,
                                IN PDEVICE_OBJECT TargetDevice,
                                OUT PDEVICE_OBJECT *AttachedToDeviceObject)
{
    PDEVICE_OBJECT AttachedDevice;
    PEXTENDED_DEVOBJ_EXTENSION SourceDeviceExtension;

    /* Get the Attached Device and source extension */
    AttachedDevice = IoGetAttachedDevice(TargetDevice);
    SourceDeviceExtension = IoGetDevObjExtension(SourceDevice);

    /* Make sure that it's in a correct state */
    if (!IoGetDevObjExtension(AttachedDevice)->ExtensionFlags &
        (DOE_UNLOAD_PENDING |
         DOE_DELETE_PENDING |
         DOE_REMOVE_PENDING |
         DOE_REMOVE_PROCESSED))
    {
        /* Update atached device fields */
        AttachedDevice->AttachedDevice = SourceDevice;
        AttachedDevice->Spare1++;

        /* Update the source with the attached data */
        SourceDevice->StackSize = AttachedDevice->StackSize + 1;
        SourceDevice->AlignmentRequirement = AttachedDevice->
                                             AlignmentRequirement;
        SourceDevice->SectorSize = AttachedDevice->SectorSize;

        /* Set the attachment in the device extension */
        SourceDeviceExtension->AttachedTo = AttachedDevice;
    }
    else
    {
        /* Device was unloading or being removed */
        AttachedDevice = NULL;
    }

    /* Return the attached device */
    *AttachedToDeviceObject = AttachedDevice;
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

    /* Initialize the Object Attributes */
    InitializeObjectAttributes(&ObjectAttributes, DeviceName, 0, NULL, NULL);

    /* Honor exclusive flag */
    if (Exclusive) ObjectAttributes.Attributes |= OBJ_EXCLUSIVE;

    /* Create a permanent object for named devices */
    if (DeviceName) ObjectAttributes.Attributes |= OBJ_PERMANENT;

    /* Align the Extension Size to 8-bytes */
    AlignedDeviceExtensionSize = (DeviceExtensionSize + 7) &~ 7;

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
    if (!NT_SUCCESS(Status)) return Status;

    /* Clear the whole Object and extension so we don't null stuff manually */
    RtlZeroMemory(CreatedDeviceObject, TotalSize);

    /*
     * Setup the Type and Size. Note that we don't use the aligned size,
     * because that's only padding for the DevObjExt and not part of the Object.
     */
    CreatedDeviceObject->Type = IO_TYPE_DEVICE;
    CreatedDeviceObject->Size = sizeof(DEVICE_OBJECT) + DeviceExtensionSize;

    /* The kernel extension is after the driver internal extension */
    DeviceObjectExtension = (PDEVOBJ_EXTENSION)
                            ((ULONG_PTR)(CreatedDeviceObject + 1) +
                             AlignedDeviceExtensionSize);

    /* Set the Type and Size. Question: why is Size 0 on Windows? */
    DeviceObjectExtension->Type = IO_TYPE_DEVICE_OBJECT_EXTENSION;
    DeviceObjectExtension->Size = 0;

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
    if (CreatedDeviceObject->DeviceType == FILE_DEVICE_DISK ||
        CreatedDeviceObject->DeviceType == FILE_DEVICE_VIRTUAL_DISK ||
        CreatedDeviceObject->DeviceType == FILE_DEVICE_CD_ROM ||
        CreatedDeviceObject->DeviceType == FILE_DEVICE_TAPE)
    {
        /* Create Vpb */
        IopAttachVpb(CreatedDeviceObject);

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
    if (CreatedDeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ||
        CreatedDeviceObject->DeviceType == FILE_DEVICE_FILE_SYSTEM ||
        CreatedDeviceObject->DeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM ||
        CreatedDeviceObject->DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM ||
        CreatedDeviceObject->DeviceType == FILE_DEVICE_TAPE_FILE_SYSTEM)
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
        /* Clear the device object and fail */
        *DeviceObject = NULL;
        return Status;
    }

    /* Now do the final linking */
    ObReferenceObject(DriverObject);
    CreatedDeviceObject->DriverObject = DriverObject;
    IopEditDeviceList(DriverObject, CreatedDeviceObject, IopAdd);

    /* Close the temporary handle and return to caller */
    NtClose(TempHandle);
    *DeviceObject = CreatedDeviceObject;
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
    /* Remove the attachment */
    IoGetDevObjExtension(TargetDevice->AttachedDevice)->AttachedTo = NULL;
    TargetDevice->AttachedDevice = NULL;

    /* Check if it's ok to delete this device */
    if ((IoGetDevObjExtension(TargetDevice)->ExtensionFlags &
        (DOE_UNLOAD_PENDING | DOE_DELETE_PENDING | DOE_REMOVE_PENDING)) &&
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

    /* Find out how many devices we'll enumerate */
    while ((CurrentDevice = CurrentDevice->NextDevice)) ActualDevices++;

    /* Go back to the first */
    CurrentDevice = DriverObject->DeviceObject;

    /* Start by at least returning this */
    *ActualNumberDeviceObjects = ActualDevices;

    /* Check if we can support so many */
    if ((ActualDevices * 4) > DeviceObjectListSize)
    {
        /* Fail because the buffer was too small */
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Check if the caller only wanted the size */
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
    /* Return the attached Device */
    return IoGetDevObjExtension(DeviceObject)->AttachedTo;
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

    /* Make sure there's a VPB */
    if (!FileSystemDeviceObject->Vpb) return STATUS_INVALID_PARAMETER;

    /* Acquire it */
    IoAcquireVpbSpinLock(&OldIrql);

    /* Get the Device Extension */
    DeviceExtension = IoGetDevObjExtension(FileSystemDeviceObject);

    /* Make sure this one has a VPB too */
    Vpb = DeviceExtension->Vpb;
    if (!Vpb) return STATUS_INVALID_PARAMETER;

    /* Make sure that it's mounted */
    if ((!Vpb->ReferenceCount) || (Vpb->Flags & VPB_MOUNTED))
    {
        /* It's not, so return failure */
        return STATUS_VOLUME_DISMOUNTED;
    }

    /* Return the Disk Device Object */
    *DiskDeviceObject = Vpb->RealDevice;

    /* Release the lock */
    IoReleaseVpbSpinLock(OldIrql);
    return STATUS_SUCCESS;
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
    if (DeviceExtension->ExtensionFlags & (DOE_UNLOAD_PENDING |
                                           DOE_DELETE_PENDING |
                                           DOE_REMOVE_PENDING |
                                           DOE_REMOVE_PROCESSED))
    {
        /* Get the Lower Device Object */
        LowerDeviceObject = DeviceExtension->AttachedTo;

        /* Reference it */
        ObReferenceObject(LowerDeviceObject);
    }

    /* Return it */
    return LowerDeviceObject;
}

/*
 * IoGetRelatedDeviceObject
 *
 * Remarks
 *    See "Windows NT File System Internals", page 633 - 634.
 *
 * Status
 *    @implemented
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

    /* Check if we were attached */
    if (DeviceObject->AttachedDevice)
    {
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
            ExFreePool(ShutdownEntry);
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
            ExFreePool(ShutdownEntry);
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Now remove the flag */
    DeviceObject->Flags &= ~DO_SHUTDOWN_REGISTERED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
IoSetStartIoAttributes(IN PDEVICE_OBJECT DeviceObject,
                       IN BOOLEAN DeferredStartIo,
                       IN BOOLEAN NonCancelable)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 *
 * FUNCTION: Dequeues the next packet from the given device object's
 * associated device queue according to a specified sort-key value and calls
 * the drivers StartIo routine with that IRP
 * ARGUMENTS:
 *      DeviceObject = Device object for which the irp is to dequeued
 *      Cancelable = True if IRPs in the key can be canceled
 *      Key = Sort key specifing which entry to remove from the queue
 */
VOID
STDCALL
IoStartNextPacketByKey(PDEVICE_OBJECT DeviceObject,
                       BOOLEAN Cancelable,
                       ULONG Key)
{
   PKDEVICE_QUEUE_ENTRY entry;
   PIRP Irp;

   entry = KeRemoveByKeyDeviceQueue(&DeviceObject->DeviceQueue,
				    Key);

   if (entry != NULL)
     {
	Irp = CONTAINING_RECORD(entry,
				IRP,
				Tail.Overlay.DeviceQueueEntry);
        DeviceObject->CurrentIrp = Irp;
	DPRINT("Next irp is 0x%p\n", Irp);
	DeviceObject->DriverObject->DriverStartIo(DeviceObject, Irp);
     }
   else
     {
	DPRINT("No next irp\n");
        DeviceObject->CurrentIrp = NULL;
     }
}

/*
 * @implemented
 *
 * FUNCTION: Removes the next packet from the device's queue and calls
 * the driver's StartIO
 * ARGUMENTS:
 *         DeviceObject = Device
 *         Cancelable = True if irps in the queue can be canceled
 */
VOID
STDCALL
IoStartNextPacket(PDEVICE_OBJECT DeviceObject,
                  BOOLEAN Cancelable)
{
   PKDEVICE_QUEUE_ENTRY entry;
   PIRP Irp;

   DPRINT("IoStartNextPacket(DeviceObject 0x%p, Cancelable %d)\n",
	  DeviceObject, Cancelable);

   entry = KeRemoveDeviceQueue(&DeviceObject->DeviceQueue);

   if (entry!=NULL)
     {
	Irp = CONTAINING_RECORD(entry,IRP,Tail.Overlay.DeviceQueueEntry);
        DeviceObject->CurrentIrp = Irp;
	DeviceObject->DriverObject->DriverStartIo(DeviceObject,Irp);
     }
   else
     {
        DeviceObject->CurrentIrp = NULL;
     }
}

/*
 * @implemented
 *
 * FUNCTION: Either call the device's StartIO routine with the packet or,
 * if the device is busy, queue it.
 * ARGUMENTS:
 *       DeviceObject = Device to start the packet on
 *       Irp = Irp to queue
 *       Key = Where to insert the irp
 *             If zero then insert in the tail of the queue
 *       CancelFunction = Optional function to cancel the irqp
 */
VOID
STDCALL
IoStartPacket(PDEVICE_OBJECT DeviceObject,
              PIRP Irp,
              PULONG Key,
              PDRIVER_CANCEL CancelFunction)
{
   BOOLEAN stat;
   KIRQL oldirql;

   DPRINT("IoStartPacket(Irp 0x%p)\n", Irp);

   ASSERT_IRQL(DISPATCH_LEVEL);

   IoAcquireCancelSpinLock(&oldirql);

   if (CancelFunction != NULL)
     {
	Irp->CancelRoutine = CancelFunction;
     }

   if (Key!=0)
     {
	stat = KeInsertByKeyDeviceQueue(&DeviceObject->DeviceQueue,
					&Irp->Tail.Overlay.DeviceQueueEntry,
					*Key);
     }
   else
     {
	stat = KeInsertDeviceQueue(&DeviceObject->DeviceQueue,
				   &Irp->Tail.Overlay.DeviceQueueEntry);
     }


   if (!stat)
     {
        IoReleaseCancelSpinLock(DISPATCH_LEVEL);
        DeviceObject->CurrentIrp = Irp;
	DeviceObject->DriverObject->DriverStartIo(DeviceObject,Irp);
	if (oldirql < DISPATCH_LEVEL)
	  {
            KeLowerIrql(oldirql);
        }
     }
   else
     {
        IoReleaseCancelSpinLock(oldirql);
     }
}

/* EOF */
