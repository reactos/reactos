/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/device.c
 * PURPOSE:         Manage devices
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ********************************************************************/

#define TAG_DEVICE_EXTENSION   TAG('D', 'E', 'X', 'T')

static ULONG IopDeviceObjectNumber = 0;

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS FASTCALL
IopInitializeDevice(
   PDEVICE_NODE DeviceNode,
   PDRIVER_OBJECT DriverObject)
{
   IO_STATUS_BLOCK IoStatusBlock;
   IO_STACK_LOCATION Stack;
   PDEVICE_OBJECT Fdo;
   NTSTATUS Status;

   if (DriverObject->DriverExtension->AddDevice)
   {
      /* This is a Plug and Play driver */
      DPRINT("Plug and Play driver found\n");

      ASSERT(DeviceNode->PhysicalDeviceObject);

      DPRINT("Calling driver AddDevice entrypoint at %08lx\n",
         DriverObject->DriverExtension->AddDevice);

      Status = DriverObject->DriverExtension->AddDevice(
         DriverObject, DeviceNode->PhysicalDeviceObject);

      if (!NT_SUCCESS(Status))
      {
         return Status;
      }

      Fdo = IoGetAttachedDeviceReference(DeviceNode->PhysicalDeviceObject);

      if (Fdo == DeviceNode->PhysicalDeviceObject)
      {
         /* FIXME: What do we do? Unload the driver or just disable the device? */
         DbgPrint("An FDO was not attached\n");
         IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
         return STATUS_UNSUCCESSFUL;
      }

      IopDeviceNodeSetFlag(DeviceNode, DNF_ADDED);

      DPRINT("Sending IRP_MN_START_DEVICE to driver\n");

      /* FIXME: Should be DeviceNode->ResourceList */
      Stack.Parameters.StartDevice.AllocatedResources = DeviceNode->BootResources;
      /* FIXME: Should be DeviceNode->ResourceListTranslated */
      Stack.Parameters.StartDevice.AllocatedResourcesTranslated = DeviceNode->BootResources;

      Status = IopInitiatePnpIrp(
         Fdo,
         &IoStatusBlock,
         IRP_MN_START_DEVICE,
         &Stack);

      if (!NT_SUCCESS(Status))
      {
          DPRINT("IopInitiatePnpIrp() failed\n");
          ObDereferenceObject(Fdo);
          return Status;
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

      if (Fdo->DeviceType == FILE_DEVICE_BUS_EXTENDER ||
          Fdo->DeviceType == FILE_DEVICE_ACPI)
      {
         DPRINT("Bus extender found\n");

         Status = IopInvalidateDeviceRelations(DeviceNode, BusRelations);
         if (!NT_SUCCESS(Status))
         {
            ObDereferenceObject(Fdo);
            return Status;
         }
      }

      ObDereferenceObject(Fdo);
   }

   return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
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

    DPRINT("IoGetDeviceObjectPointer(ObjectName %wZ, DesiredAccess %x," 
            "FileObject %p DeviceObject %p)\n",
            ObjectName, DesiredAccess, FileObject, DeviceObject);

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

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile failed, Status: 0x%x\n", Status);
        return Status;
    }

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
STDCALL
IoAttachDevice(PDEVICE_OBJECT SourceDevice,
               PUNICODE_STRING TargetDeviceName,
               PDEVICE_OBJECT *AttachedDevice)
{
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PDEVICE_OBJECT TargetDevice;
     
    /* Call the helper routine for an attach operation */
    DPRINT("IoAttachDevice\n");
    Status = IopGetDeviceObjectPointer(TargetDeviceName, 
                                       FILE_READ_ATTRIBUTES, 
                                       &FileObject,
                                       &TargetDevice,
                                       IO_ATTACH_DEVICE_API);
   
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get Device Object\n");
        return Status;
    }
    
    /* Attach the device */
    IoAttachDeviceToDeviceStackSafe(SourceDevice, TargetDevice, AttachedDevice);

    /* Derference it */
    ObDereferenceObject(FileObject);
    return STATUS_SUCCESS;
}

/*
 * IoAttachDeviceByPointer
 *
 * Status
 *    @implemented
 */
NTSTATUS 
STDCALL
IoAttachDeviceByPointer(IN PDEVICE_OBJECT SourceDevice,
                        IN PDEVICE_OBJECT TargetDevice)
{
    PDEVICE_OBJECT AttachedDevice;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IoAttachDeviceByPointer(SourceDevice %x, TargetDevice %x)\n",
            SourceDevice, TargetDevice);

    /* Do the Attach */
    AttachedDevice = IoAttachDeviceToDeviceStack(SourceDevice, TargetDevice);
    if (AttachedDevice == NULL) Status = STATUS_NO_SUCH_DEVICE;

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
STDCALL
IoAttachDeviceToDeviceStack(PDEVICE_OBJECT SourceDevice,
                            PDEVICE_OBJECT TargetDevice)
{
    NTSTATUS Status;
    PDEVICE_OBJECT LocalAttach;
    
    /* Attach it safely */
    DPRINT("IoAttachDeviceToDeviceStack\n");
    Status = IoAttachDeviceToDeviceStackSafe(SourceDevice,
                                             TargetDevice,
                                             &LocalAttach);
                                             
    /* Return it */
    DPRINT("IoAttachDeviceToDeviceStack DONE: %x\n", LocalAttach);
    return LocalAttach;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoAttachDeviceToDeviceStackSafe(IN PDEVICE_OBJECT SourceDevice,
                                IN PDEVICE_OBJECT TargetDevice,
                                OUT PDEVICE_OBJECT *AttachedToDeviceObject)
{
    PDEVICE_OBJECT AttachedDevice;
    PDEVOBJ_EXTENSION SourceDeviceExtension;
   
    DPRINT("IoAttachDeviceToDeviceStack(SourceDevice %x, TargetDevice %x)\n",
            SourceDevice, TargetDevice);

    /* Get the Attached Device and source extension */
    AttachedDevice = IoGetAttachedDevice(TargetDevice);
    SourceDeviceExtension = SourceDevice->DeviceObjectExtension;
    
    /* Make sure that it's in a correct state */
    if (!(AttachedDevice->DeviceObjectExtension->ExtensionFlags & 
        (DOE_UNLOAD_PENDING | DOE_DELETE_PENDING | 
         DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED)))
    {
        /* Update fields */
        AttachedDevice->AttachedDevice = SourceDevice;
        SourceDevice->AttachedDevice = NULL;
        SourceDevice->StackSize = AttachedDevice->StackSize + 1;
        SourceDevice->AlignmentRequirement = AttachedDevice->AlignmentRequirement;
        SourceDevice->SectorSize = AttachedDevice->SectorSize;
        SourceDevice->Vpb = AttachedDevice->Vpb;
        
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
STDCALL
IoCreateDevice(PDRIVER_OBJECT DriverObject,
               ULONG DeviceExtensionSize,
               PUNICODE_STRING DeviceName,
               DEVICE_TYPE DeviceType,
               ULONG DeviceCharacteristics,
               BOOLEAN Exclusive,
               PDEVICE_OBJECT *DeviceObject)
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
   
    ASSERT_IRQL(PASSIVE_LEVEL);
    DPRINT("IoCreateDevice(DriverObject %x)\n",DriverObject);
   
    /* Generate a name if we have to */
    if (DeviceCharacteristics & FILE_AUTOGENERATED_DEVICE_NAME)
    {
        swprintf(AutoNameBuffer,
                 L"\\Device\\%08lx",
                 InterlockedIncrementUL(&IopDeviceObjectNumber));         
        RtlInitUnicodeString(&AutoName, AutoNameBuffer);
        DeviceName = &AutoName;
   }
   
    /* Initialize the Object Attributes */
    InitializeObjectAttributes(&ObjectAttributes, DeviceName, 0, NULL, NULL);
    
    /* Honor exclusive flag */
    ObjectAttributes.Attributes |= OBJ_EXCLUSIVE;
    
    /* Create a permanent object for named devices */
    if (DeviceName != NULL)
    {
        ObjectAttributes.Attributes |= OBJ_PERMANENT;
    }
    
    /* Align the Extension Size to 8-bytes */
    AlignedDeviceExtensionSize = (DeviceExtensionSize + 7) &~ 7;
    DPRINT("AlignedDeviceExtensionSize %x\n", AlignedDeviceExtensionSize);
    
    /* Total Size */
    TotalSize = AlignedDeviceExtensionSize + 
                sizeof(DEVICE_OBJECT) + sizeof(DEVOBJ_EXTENSION);
    DPRINT("TotalSize %x\n", TotalSize);

    /* Create the Device Object */
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
        DPRINT1("IoCreateDevice() ObCreateObject failed, status: 0x%08X\n", Status);
        return Status;
    }
    
    /* Clear the whole Object and extension so we don't null stuff manually */
    RtlZeroMemory(CreatedDeviceObject, TotalSize);
    DPRINT("CreatedDeviceObject %x\n", CreatedDeviceObject);
  
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
    DPRINT("DeviceObjectExtension %x\n", DeviceObjectExtension);
    DeviceObjectExtension->Type = IO_TYPE_DEVICE_OBJECT_EXTENSION;
    DeviceObjectExtension->Size = 0;
                                     
    /* Link the Object and Extension */
    DeviceObjectExtension->DeviceObject = CreatedDeviceObject;
    CreatedDeviceObject->DeviceObjectExtension = DeviceObjectExtension;
                             
    /* Set Device Object Data */    
    CreatedDeviceObject->DeviceType = DeviceType;
    CreatedDeviceObject->Characteristics = DeviceCharacteristics;
    CreatedDeviceObject->DeviceExtension = CreatedDeviceObject + 1;
    CreatedDeviceObject->StackSize = 1;
    CreatedDeviceObject->AlignmentRequirement = 1; /* FIXME */
    
    /* Set the Flags */
    /* FIXME: After the Driver is Loaded, the flag below should be removed */
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
        case FILE_DEVICE_DISK_FILE_SYSTEM:
        case FILE_DEVICE_DISK:
        case FILE_DEVICE_VIRTUAL_DISK:
            CreatedDeviceObject->SectorSize  = 512;
            break;

        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            CreatedDeviceObject->SectorSize = 2048;
            break;
    }
    
    /* Create the Device Queue */
    KeInitializeDeviceQueue(&CreatedDeviceObject->DeviceQueue);

    /* Insert the Object */
    Status = ObInsertObject(CreatedDeviceObject,
                            NULL,
                            FILE_READ_DATA | FILE_WRITE_DATA,
                            0,
                            NULL,
                            &TempHandle);
                            
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Cannot insert Device Object into Handle Table\n");
        *DeviceObject = NULL;
        return Status;
    }
    
    /* Now do the final linking */
    ObReferenceObject(DriverObject);
    CreatedDeviceObject->DriverObject = DriverObject;
    CreatedDeviceObject->NextDevice = DriverObject->DeviceObject;
    DriverObject->DeviceObject = CreatedDeviceObject;
    
    /* Close the temporary handle, but do an extra reference first so it doesn't die */
    ObReferenceObject(CreatedDeviceObject);
    NtClose(TempHandle);
    
    /* Return to caller */
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
STDCALL
IoDeleteDevice(PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_OBJECT Previous;

   if (DeviceObject->Flags & DO_SHUTDOWN_REGISTERED)
      IoUnregisterShutdownNotification(DeviceObject);

   /* Remove the timer if it exists */
   if (DeviceObject->Timer)
   {
      IopRemoveTimerFromTimerList(DeviceObject->Timer);
      ExFreePool(DeviceObject->Timer);
   }

   /* Remove device from driver device list */
   Previous = DeviceObject->DriverObject->DeviceObject;
   if (Previous == DeviceObject)
   {
      DeviceObject->DriverObject->DeviceObject = DeviceObject->NextDevice;
   }
   else
   {
      while (Previous->NextDevice != DeviceObject)
         Previous = Previous->NextDevice;
      Previous->NextDevice = DeviceObject->NextDevice;
   }
   
   /* I guess this should be removed later... but it shouldn't cause problems */
   DeviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_DELETE_PENDING;
   
   /* Make the object temporary. This should automatically remove the device
      from the namespace */
   ObMakeTemporaryObject(DeviceObject);
   
   /* Dereference the driver object */
   ObDereferenceObject(DeviceObject->DriverObject);
   
   /* Remove the keep-alive reference */
   ObDereferenceObject(DeviceObject);
}

/*
 * IoDetachDevice
 *
 * Status
 *    @implemented
 */
VOID
STDCALL
IoDetachDevice(PDEVICE_OBJECT TargetDevice)
{   
    DPRINT("IoDetachDevice(TargetDevice %x)\n", TargetDevice);   
    
    /* Remove the attachment */
    TargetDevice->AttachedDevice->DeviceObjectExtension->AttachedTo = NULL;
    TargetDevice->AttachedDevice = NULL;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoEnumerateDeviceObjectList(IN  PDRIVER_OBJECT DriverObject,
                            IN  PDEVICE_OBJECT *DeviceObjectList,
                            IN  ULONG DeviceObjectListSize,
                            OUT PULONG ActualNumberDeviceObjects)
{
    ULONG ActualDevices = 1;
    PDEVICE_OBJECT CurrentDevice = DriverObject->DeviceObject;
    
    DPRINT1("IoEnumerateDeviceObjectList\n");
    
    /* Find out how many devices we'll enumerate */
    while ((CurrentDevice = CurrentDevice->NextDevice))
    {
        ActualDevices++;
    }
    
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
STDCALL
IoGetAttachedDevice(PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT Current = DeviceObject;
   
    /* Get the last attached device */
    while (Current->AttachedDevice) 
    {
        Current = Current->AttachedDevice;
    }
    
    /* Return it */
    return Current;
}

/*
 * IoGetAttachedDeviceReference
 *
 * Status
 *    @implemented
 */
PDEVICE_OBJECT 
STDCALL
IoGetAttachedDeviceReference(PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT Current = IoGetAttachedDevice(DeviceObject);
   
    /* Reference the ATtached Device */
    ObReferenceObject(Current);
    return Current;
}

/*
 * @implemented
 */
PDEVICE_OBJECT
STDCALL
IoGetDeviceAttachmentBaseRef(IN PDEVICE_OBJECT DeviceObject)
{
    /* Return the attached Device */
    return (DeviceObject->DeviceObjectExtension->AttachedTo);
}

/*
 * IoGetDeviceObjectPointer
 *
 * Status
 *    @implemented
 */
NTSTATUS 
STDCALL
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
STDCALL
IoGetDiskDeviceObject(IN  PDEVICE_OBJECT FileSystemDeviceObject,
                      OUT PDEVICE_OBJECT *DiskDeviceObject)
{
    PDEVOBJ_EXTENSION DeviceExtension;
    PVPB Vpb;
    KIRQL OldIrql;
    
    /* Make sure there's a VPB */
    if (!FileSystemDeviceObject->Vpb) return STATUS_INVALID_PARAMETER;
    
    /* Acquire it */
    IoAcquireVpbSpinLock(&OldIrql);
    
    /* Get the Device Extension */
    DeviceExtension = FileSystemDeviceObject->DeviceObjectExtension;
    
    /* Make sure this one has a VPB too */
    Vpb = DeviceExtension->Vpb;
    if (!Vpb) return STATUS_INVALID_PARAMETER;
    
    /* Make sure someone it's mounted */
    if ((!Vpb->ReferenceCount) || (Vpb->Flags & VPB_MOUNTED)) return STATUS_VOLUME_DISMOUNTED;
    
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
STDCALL
IoGetLowerDeviceObject(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVOBJ_EXTENSION DeviceExtension = DeviceObject->DeviceObjectExtension;
    PDEVICE_OBJECT LowerDeviceObject = NULL;
    
    /* Make sure it's not getting deleted */
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
STDCALL
IoGetRelatedDeviceObject(IN PFILE_OBJECT FileObject)
{
    PDEVICE_OBJECT DeviceObject = FileObject->DeviceObject;
    
    /* Get logical volume mounted on a physical/virtual/logical device */
    if (FileObject->Vpb && FileObject->Vpb->DeviceObject)
    {
        DeviceObject = FileObject->Vpb->DeviceObject;
    }

    /*
     * Check if file object has an associated device object mounted by some
     * other file system.
     */
    if (FileObject->DeviceObject->Vpb && 
        FileObject->DeviceObject->Vpb->DeviceObject)
    {
        DeviceObject = FileObject->DeviceObject->Vpb->DeviceObject;
    }

    /* Return the highest attached device */
    return IoGetAttachedDevice(DeviceObject);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoRegisterLastChanceShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
IoSetStartIoAttributes(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeferredStartIo,
    IN BOOLEAN NonCancelable
    )
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
IoSynchronousInvalidateDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_RELATION_TYPE Type
    )
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoValidateDeviceIoControlAccess(
    IN  PIRP    Irp,
    IN  ULONG   RequiredAccess
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
