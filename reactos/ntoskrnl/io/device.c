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

NTSTATUS STDCALL
IopCreateDevice(
   PVOID ObjectBody,
   PVOID Parent,
   PWSTR RemainingPath,
   POBJECT_ATTRIBUTES ObjectAttributes)
{
   DPRINT("IopCreateDevice(ObjectBody %x, Parent %x, RemainingPath %S)\n",
      ObjectBody, Parent, RemainingPath);
   
   if (RemainingPath != NULL && wcschr(RemainingPath + 1, '\\') != NULL)
      return STATUS_OBJECT_PATH_NOT_FOUND;
   
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
    DPRINT1("flags %d\n", AttachedDevice->DeviceObjectExtension->ExtensionFlags);
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
 * IoDeleteDevice
 *
 * Status
 *    @implemented
 */
VOID STDCALL
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

   /* Free device extension */
   if (DeviceObject->DeviceObjectExtension)
      ExFreePool(DeviceObject->DeviceObjectExtension);

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
   ObDereferenceObject(DeviceObject);
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
NTSTATUS STDCALL
IoCreateDevice(
   PDRIVER_OBJECT DriverObject,
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
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   if (DeviceName != NULL)
   {
      DPRINT("IoCreateDevice(DriverObject %x, DeviceName %S)\n",
         DriverObject, DeviceName->Buffer);
   }
   else
   {
      DPRINT("IoCreateDevice(DriverObject %x)\n",DriverObject);
   }
   
   if (DeviceCharacteristics & FILE_AUTOGENERATED_DEVICE_NAME)
   {
      swprintf(AutoNameBuffer,
               L"\\Device\\%08lx",
               InterlockedIncrementUL(&IopDeviceObjectNumber));
      RtlInitUnicodeString(&AutoName,
                           AutoNameBuffer);
      DeviceName = &AutoName;
   }
   
   if (DeviceName != NULL)
   {
      InitializeObjectAttributes(&ObjectAttributes, DeviceName, 0, NULL, NULL);
      Status = ObCreateObject(
         KernelMode,
         IoDeviceObjectType,
         &ObjectAttributes,
         KernelMode,
         NULL,
         sizeof(DEVICE_OBJECT),
         0,
         0,
         (PVOID*)&CreatedDeviceObject);
   }
   else
   {
      Status = ObCreateObject(
         KernelMode,
         IoDeviceObjectType,
         NULL,
         KernelMode,
         NULL,
         sizeof(DEVICE_OBJECT),
         0,
         0,
         (PVOID*)&CreatedDeviceObject);
   }
   
   *DeviceObject = NULL;
   
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IoCreateDevice() ObCreateObject failed, status: 0x%08X\n", Status);
      return Status;
   }
  
   if (DriverObject->DeviceObject == NULL)
   {
      DriverObject->DeviceObject = CreatedDeviceObject;
      CreatedDeviceObject->NextDevice = NULL;
   }
   else
   {
      CreatedDeviceObject->NextDevice = DriverObject->DeviceObject;
      DriverObject->DeviceObject = CreatedDeviceObject;
   }
  
   CreatedDeviceObject->Type = DeviceType;
   CreatedDeviceObject->DriverObject = DriverObject;
   CreatedDeviceObject->CurrentIrp = NULL;
   CreatedDeviceObject->Flags = 0;

   CreatedDeviceObject->DeviceExtension = 
      ExAllocatePoolWithTag(
         NonPagedPool,
         DeviceExtensionSize,
         TAG_DEVICE_EXTENSION);

   if (DeviceExtensionSize > 0 && CreatedDeviceObject->DeviceExtension == NULL)
   {
      ExFreePool(CreatedDeviceObject);
      DPRINT("IoCreateDevice() ExAllocatePoolWithTag failed, returning: 0x%08X\n", STATUS_INSUFFICIENT_RESOURCES);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   if (DeviceExtensionSize > 0)
   {
      RtlZeroMemory(CreatedDeviceObject->DeviceExtension, DeviceExtensionSize);
   }

   CreatedDeviceObject->Size = sizeof(DEVICE_OBJECT) + DeviceExtensionSize;
   CreatedDeviceObject->ReferenceCount = 1;
   CreatedDeviceObject->AttachedDevice = NULL;
   CreatedDeviceObject->DeviceType = DeviceType;
   CreatedDeviceObject->StackSize = 1;
   CreatedDeviceObject->AlignmentRequirement = 1;
   CreatedDeviceObject->Characteristics = DeviceCharacteristics;
   CreatedDeviceObject->Timer = NULL;
   CreatedDeviceObject->Vpb = NULL;
   KeInitializeDeviceQueue(&CreatedDeviceObject->DeviceQueue);
  
   KeInitializeEvent(
      &CreatedDeviceObject->DeviceLock,
      SynchronizationEvent,
      TRUE);
  
   /* FIXME: Do we need to add network drives too?! */
   if (CreatedDeviceObject->DeviceType == FILE_DEVICE_DISK ||
       CreatedDeviceObject->DeviceType == FILE_DEVICE_CD_ROM ||
       CreatedDeviceObject->DeviceType == FILE_DEVICE_TAPE)
   {
      IoAttachVpb(CreatedDeviceObject);
   }
   CreatedDeviceObject->SectorSize = 512; /* FIXME */
  
   DeviceObjectExtension =
      ExAllocatePoolWithTag(
         NonPagedPool,
         sizeof(DEVOBJ_EXTENSION),
         TAG_DEVICE_EXTENSION);

   RtlZeroMemory(DeviceObjectExtension, sizeof(DEVOBJ_EXTENSION));
   DeviceObjectExtension->Type = 0 /* ?? */;
   DeviceObjectExtension->Size = sizeof(DEVOBJ_EXTENSION);
   DeviceObjectExtension->DeviceObject = CreatedDeviceObject;
   DeviceObjectExtension->DeviceNode = NULL;

   CreatedDeviceObject->DeviceObjectExtension = DeviceObjectExtension;

   *DeviceObject = CreatedDeviceObject;
  
   return STATUS_SUCCESS;
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
