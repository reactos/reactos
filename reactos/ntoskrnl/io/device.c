/* $Id: device.c,v 1.43 2002/06/10 23:03:33 ekohl Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/device.c
 * PURPOSE:        Manage devices
 * PROGRAMMER:     David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ldr.h>
#include <internal/id.h>
#include <internal/pool.h>
#include <internal/registry.h>

#include <roscfg.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_DRIVER             TAG('D', 'R', 'V', 'R')
#define TAG_DRIVER_EXTENSION   TAG('D', 'R', 'V', 'E')
#define TAG_DEVICE_EXTENSION   TAG('D', 'E', 'X', 'T')

#define DRIVER_REGISTRY_KEY_BASENAME  L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
IoAttachDeviceByPointer(IN PDEVICE_OBJECT SourceDevice,
			IN PDEVICE_OBJECT TargetDevice)
{
	PDEVICE_OBJECT AttachedDevice;

	DPRINT("IoAttachDeviceByPointer(SourceDevice %x, TargetDevice %x)\n",
	       SourceDevice,
	       TargetDevice);

	AttachedDevice = IoAttachDeviceToDeviceStack (SourceDevice,
	                                              TargetDevice);
	if (AttachedDevice == NULL)
		return STATUS_NO_SUCH_DEVICE;

	return STATUS_SUCCESS;
}


VOID STDCALL
IoDeleteDevice(PDEVICE_OBJECT DeviceObject)
{
	PDEVICE_OBJECT Previous;

	if (DeviceObject->Flags & DO_SHUTDOWN_REGISTERED)
		IoUnregisterShutdownNotification(DeviceObject);

	/* remove the timer if it exists */
	if (DeviceObject->Timer)
	{
		IoStopTimer(DeviceObject);
		ExFreePool(DeviceObject->Timer);
	}

	/* free device extension */
	if (DeviceObject->DeviceObjectExtension)
		ExFreePool (DeviceObject->DeviceObjectExtension);

	/* remove device from driver device list */
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

	ObDereferenceObject (DeviceObject);
}


PDEVICE_OBJECT
STDCALL
IoGetRelatedDeviceObject (
	IN	PFILE_OBJECT	FileObject
	)
{
	return (FileObject->DeviceObject);
}


NTSTATUS
STDCALL
IoGetDeviceObjectPointer (
	IN	PUNICODE_STRING	ObjectName,
	IN	ACCESS_MASK	DesiredAccess,
	OUT	PFILE_OBJECT	* FileObject,
	OUT	PDEVICE_OBJECT	* DeviceObject)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK StatusBlock;
	PFILE_OBJECT LocalFileObject;
	HANDLE FileHandle;
	NTSTATUS Status;

	DPRINT("IoGetDeviceObjectPointer(ObjectName %wZ, DesiredAccess %x, FileObject %p DeviceObject %p)\n",
	       ObjectName,
	       DesiredAccess,
	       FileObject,
	       DeviceObject);

	InitializeObjectAttributes (&ObjectAttributes,
	                            ObjectName,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtOpenFile (&FileHandle,
	                     DesiredAccess,
	                     &ObjectAttributes,
	                     &StatusBlock,
	                     0,
	                     FILE_NON_DIRECTORY_FILE);
	if (!NT_SUCCESS(Status))
		return Status;

	Status = ObReferenceObjectByHandle (FileHandle,
	                                    0,
	                                    IoFileObjectType,
	                                    KernelMode,
	                                    (PVOID*)&LocalFileObject,
	                                    NULL);
	if (NT_SUCCESS(Status))
	{
		*DeviceObject = IoGetRelatedDeviceObject (LocalFileObject);
		*FileObject = LocalFileObject;
	}
	NtClose (FileHandle);

	return Status;
}


VOID
STDCALL
IoDetachDevice(PDEVICE_OBJECT TargetDevice)
{
   UNIMPLEMENTED;
}


PDEVICE_OBJECT
STDCALL
IoGetAttachedDevice(PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_OBJECT Current = DeviceObject;
   
//   DPRINT("IoGetAttachedDevice(DeviceObject %x)\n",DeviceObject);
   
   while (Current->AttachedDevice!=NULL)
     {
	Current = Current->AttachedDevice;
//	DPRINT("Current %x\n",Current);
     }

//   DPRINT("IoGetAttachedDevice() = %x\n",DeviceObject);
   return(Current);
}

PDEVICE_OBJECT
STDCALL
IoGetAttachedDeviceReference(PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_OBJECT Current = DeviceObject;

   while (Current->AttachedDevice!=NULL)
     {
	Current = Current->AttachedDevice;
     }

   ObReferenceObject(Current);
   return(Current);
}

PDEVICE_OBJECT STDCALL
IoAttachDeviceToDeviceStack(PDEVICE_OBJECT SourceDevice,
			    PDEVICE_OBJECT TargetDevice)
{
   PDEVICE_OBJECT AttachedDevice;
   
   DPRINT("IoAttachDeviceToDeviceStack(SourceDevice %x, TargetDevice %x)\n",
	  SourceDevice,TargetDevice);

   AttachedDevice = IoGetAttachedDevice(TargetDevice);
   AttachedDevice->AttachedDevice = SourceDevice;
   SourceDevice->AttachedDevice = NULL;
   SourceDevice->StackSize = AttachedDevice->StackSize + 1;
   SourceDevice->Vpb = AttachedDevice->Vpb;
   return(AttachedDevice);
}

VOID STDCALL
IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
				 PDRIVER_REINITIALIZE ReinitRoutine,
				 PVOID Context)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL
IopDefaultDispatchFunction(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
IopCreateDriverObject(PDRIVER_OBJECT *DriverObject,
		      PUNICODE_STRING ServiceName,
		      BOOLEAN FileSystem)
{
  PDRIVER_OBJECT Object;
  HANDLE DriverHandle = 0;
  ULONG i;
  WCHAR NameBuffer[MAX_PATH];
  UNICODE_STRING DriverName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;

  DPRINT1("IopCreateDriverObject(%p '%wZ' %x)\n", DriverObject, ServiceName, FileSystem);

  *DriverObject = NULL;

  /*  Create ModuleName string  */
  if (ServiceName != NULL)
    {
      if (FileSystem)
	wcscpy(NameBuffer, FILESYSTEM_ROOT_NAME);
      else
	wcscpy(NameBuffer, DRIVER_ROOT_NAME);
      wcscat(NameBuffer, ServiceName->Buffer);

      RtlInitUnicodeString(&DriverName,
			   NameBuffer);
      DPRINT1("Driver name: '%wZ'\n", &DriverName);
    }

  /*  Initialize ObjectAttributes for ModuleObject  */
  InitializeObjectAttributes(&ObjectAttributes,
			     (ServiceName != NULL)? &DriverName : NULL,
			     OBJ_PERMANENT,
			     NULL,
			     NULL);

  /*  Create module object  */
  Status = ObCreateObject(&DriverHandle,
                          STANDARD_RIGHTS_REQUIRED,
                          &ObjectAttributes,
                          IoDriverObjectType,
                          (PVOID*)&Object);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  NtClose(DriverHandle);

  /* Create driver extension */
  Object->DriverExtension = (PDRIVER_EXTENSION)
    ExAllocatePoolWithTag(NonPagedPool,
       sizeof(DRIVER_EXTENSION),
       TAG_DRIVER_EXTENSION);
  if (Object->DriverExtension == NULL)
    {
      ExFreePool(Object);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  RtlZeroMemory(Object->DriverExtension, sizeof(DRIVER_EXTENSION));

  Object->Type = InternalDriverType;

  for (i=0; i<=IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
       Object->MajorFunction[i] = (PDRIVER_DISPATCH) IopDefaultDispatchFunction;
    }

  *DriverObject = Object;

  return STATUS_SUCCESS;
}

NTSTATUS
IopAttachFilterDrivers(PDEVICE_NODE DeviceNode,
                       BOOLEAN Lower)
{
  return STATUS_SUCCESS;
}

NTSTATUS 
IopInitializeDevice(PDEVICE_NODE DeviceNode,
                    BOOLEAN BootDriversOnly)
{
  IO_STATUS_BLOCK	IoStatusBlock;
  PDRIVER_OBJECT DriverObject;
  IO_STACK_LOCATION Stack;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;

  DriverObject = DeviceNode->DriverObject;

  if (DriverObject->DriverExtension->AddDevice)
    {
      /* This is a Plug and Play driver */
      DPRINT("Plug and Play driver found\n");

      assert(DeviceNode->Pdo);

      DPRINT("Calling driver AddDevice entrypoint at %08lx\n",
        DriverObject->DriverExtension->AddDevice);
      Status = DriverObject->DriverExtension->AddDevice(
        DriverObject, DeviceNode->Pdo);
      if (!NT_SUCCESS(Status))
        {
	        return(Status);
        }

      IopDeviceNodeSetFlag(DeviceNode, DNF_ADDED);

      DPRINT("Sending IRP_MN_START_DEVICE to driver\n");

      Fdo = IoGetAttachedDeviceReference(DeviceNode->Pdo);

      if (Fdo == DeviceNode->Pdo)
        {
          /* FIXME: What do we do? Unload the driver or just disable the device? */
          DbgPrint("An FDO was not attached\n");
          KeBugCheck(0);
        }

      /* FIXME: Put some resources in the IRP for the device */
      Stack.Parameters.StartDevice.AllocatedResources = NULL;
      Stack.Parameters.StartDevice.AllocatedResourcesTranslated = NULL;

      Status = IopInitiatePnpIrp(
        Fdo,
        &IoStatusBlock,
        IRP_MN_START_DEVICE,
        &Stack);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IopInitiatePnpIrp() failed\n");
          ObDereferenceObject(Fdo);
	        return(Status);
        }

      if (Fdo->DeviceType == FILE_DEVICE_BUS_EXTENDER)
        {
          DPRINT("Bus extender found\n");

          Status = IopInterrogateBusExtender(
            DeviceNode, Fdo, BootDriversOnly);
          if (!NT_SUCCESS(Status))
            {
              ObDereferenceObject(Fdo);
	            return(Status);
            }
        }
      else if (Fdo->DeviceType == FILE_DEVICE_ACPI)
        {
#ifdef ACPI
          static BOOLEAN SystemPowerDeviceNodeCreated = FALSE;

          /* There can be only one system power device */
          if (!SystemPowerDeviceNodeCreated)
            {
              PopSystemPowerDeviceNode = DeviceNode;
              SystemPowerDeviceNodeCreated = TRUE;
            }
#endif /* ACPI */
        }
      ObDereferenceObject(Fdo);
    }

  return STATUS_SUCCESS;
}

NTSTATUS
IopInitializeService(
  PDEVICE_NODE DeviceNode,
  PUNICODE_STRING ImagePath)
{
  PMODULE_OBJECT ModuleObject;
  NTSTATUS Status;

  ModuleObject = LdrGetModuleObject(&DeviceNode->ServiceName);
  if (ModuleObject == NULL)
  {
    /* The module is currently not loaded, so load it now */

    Status = LdrLoadModule(ImagePath, &ModuleObject);
    if (!NT_SUCCESS(Status))
    {
      /* FIXME: Log the error */
      CPRINT("Driver load failed\n");
      return(Status);
    }

    Status = IopInitializeDriver(ModuleObject->EntryPoint, DeviceNode);
    if (!NT_SUCCESS(Status))
    {
      /* FIXME: Log the error */
      CPRINT("A driver failed to initialize\n");
      return(Status);
    }
    ObDereferenceObject(ModuleObject);
  }

  Status = IopInitializeDevice(DeviceNode, TRUE);

  return(Status);
}

NTSTATUS
IopInitializeDeviceNodeService(PDEVICE_NODE DeviceNode)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  UNICODE_STRING ImagePath;
  HANDLE KeyHandle;
  NTSTATUS Status;

  Status = RtlpGetRegistryHandle(
    RTL_REGISTRY_SERVICES,
	  DeviceNode->ServiceName.Buffer,
		TRUE,
		&KeyHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlpGetRegistryHandle() failed (Status %x)\n", Status);
      return(Status);
    }

  RtlZeroMemory(QueryTable, sizeof(QueryTable));

  RtlInitUnicodeString(&ImagePath, NULL);

  QueryTable[0].Name = L"ImagePath";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = &ImagePath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
				  (PWSTR)KeyHandle,
				  QueryTable,
				  NULL,
				  NULL);
  NtClose(KeyHandle);

  DPRINT("RtlQueryRegistryValues() returned status %x\n", Status);

  if (NT_SUCCESS(Status))
    {
      DPRINT("Got ImagePath %S\n", ImagePath.Buffer);

      Status = IopInitializeService(DeviceNode, &ImagePath);

      RtlFreeUnicodeString(&ImagePath);
    }

  return(Status);
}

NTSTATUS
IopInitializeDriver(PDRIVER_INITIALIZE DriverEntry,
		    PDEVICE_NODE DeviceNode)
/*
 * FUNCTION: Called to initalize a loaded driver
 * ARGUMENTS:
 *   DriverEntry = Pointer to driver entry routine
 *   DeviceNode  = Pointer to device node
 */
{
  WCHAR RegistryKeyBuffer[MAX_PATH];
  PDRIVER_OBJECT DriverObject;
  UNICODE_STRING RegistryKey;
  NTSTATUS Status;

  DPRINT("IopInitializeDriver(DriverEntry %08lx, DeviceNode %08lx)\n",
    DriverEntry, DeviceNode);

  Status = IopCreateDriverObject(&DriverObject, &DeviceNode->ServiceName, FALSE);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DeviceNode->DriverObject = DriverObject;

  if (DeviceNode->ServiceName.Buffer)
    {
      wcscpy(RegistryKeyBuffer, DRIVER_REGISTRY_KEY_BASENAME);
      wcscat(RegistryKeyBuffer, DeviceNode->ServiceName.Buffer);
      RtlInitUnicodeString(&RegistryKey, RegistryKeyBuffer);
    }
  else
    {
      RtlInitUnicodeString(&RegistryKey, NULL);
    }

  DPRINT("RegistryKey: %wZ\n", &RegistryKey);
  DPRINT("Calling driver entrypoint at %08lx\n", DriverEntry);

  Status = DriverEntry(DriverObject, &RegistryKey);
  if (!NT_SUCCESS(Status))
    {
      DeviceNode->DriverObject = NULL;
      ExFreePool(DriverObject->DriverExtension);
      ExFreePool(DriverObject);
      return(Status);
    }

  Status = IopInitializeDevice(DeviceNode, TRUE);

  return(Status);
}


NTSTATUS STDCALL
IoAttachDevice(PDEVICE_OBJECT SourceDevice,
	       PUNICODE_STRING TargetDevice,
	       PDEVICE_OBJECT* AttachedDevice)
/*
 * FUNCTION: Layers a device over the highest device in a device stack
 * ARGUMENTS:
 *       SourceDevice = Device to attached
 *       TargetDevice = Name of the target device
 *       AttachedDevice (OUT) = Caller storage for the device attached to
 */
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
IopCreateDevice(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes)
{
   
   DPRINT("IopCreateDevice(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	  ObjectBody, Parent, RemainingPath);
   
   if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
IoCreateDevice(PDRIVER_OBJECT DriverObject,
	       ULONG DeviceExtensionSize,
	       PUNICODE_STRING DeviceName,
	       DEVICE_TYPE DeviceType,
	       ULONG DeviceCharacteristics,
	       BOOLEAN Exclusive,
	       PDEVICE_OBJECT* DeviceObject)
/*
 * FUNCTION: Allocates memory for and intializes a device object for use for
 * a driver
 * ARGUMENTS:
 *         DriverObject : Driver object passed by iomgr when the driver was
 *                        loaded
 *         DeviceExtensionSize : Number of bytes for the device extension
 *         DeviceName : Unicode name of device
 *         DeviceType : Device type
 *         DeviceCharacteristics : Bit mask of device characteristics
 *         Exclusive : True if only one thread can access the device at a
 *                     time
 * RETURNS:
 *         Success or failure
 *         DeviceObject : Contains a pointer to allocated device object
 *                        if the call succeeded
 * NOTES: See the DDK documentation for more information
 */
{
   PDEVICE_OBJECT CreatedDeviceObject;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE DeviceHandle;
   NTSTATUS Status;
   
   assert_irql(PASSIVE_LEVEL);
   
   if (DeviceName != NULL)
     {
	DPRINT("IoCreateDevice(DriverObject %x, DeviceName %S)\n",DriverObject,
	       DeviceName->Buffer);
     }
   else
     {
	DPRINT("IoCreateDevice(DriverObject %x)\n",DriverObject);
     }
   
   if (DeviceName != NULL)
     {
	InitializeObjectAttributes(&ObjectAttributes,DeviceName,0,NULL,NULL);
	Status = ObCreateObject(&DeviceHandle,
				0,
				&ObjectAttributes,
				IoDeviceObjectType,
				(PVOID*)&CreatedDeviceObject);
     }
   else
     {
	Status = ObCreateObject(&DeviceHandle,
				0,
				NULL,
				IoDeviceObjectType,
				(PVOID*)&CreatedDeviceObject);
     }
   
   *DeviceObject = NULL;
   
   if (!NT_SUCCESS(Status))
     {
	return(Status);
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
    ExAllocatePoolWithTag(NonPagedPool, DeviceExtensionSize,
			  TAG_DEVICE_EXTENSION);
  if (DeviceExtensionSize > 0 && CreatedDeviceObject->DeviceExtension == NULL)
    {
      ExFreePool(CreatedDeviceObject);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  if (DeviceExtensionSize > 0)
    {
      RtlZeroMemory(CreatedDeviceObject->DeviceExtension,
		    DeviceExtensionSize);
    }

  CreatedDeviceObject->AttachedDevice = NULL;
  CreatedDeviceObject->DeviceType = DeviceType;
  CreatedDeviceObject->StackSize = 1;
  CreatedDeviceObject->AlignmentRequirement = 1;
  KeInitializeDeviceQueue(&CreatedDeviceObject->DeviceQueue);
  
  KeInitializeEvent(&CreatedDeviceObject->DeviceLock,
		    SynchronizationEvent,
		    TRUE);
  
  /* FIXME: Do we need to add network drives too?! */
  if (CreatedDeviceObject->DeviceType == FILE_DEVICE_DISK ||
      CreatedDeviceObject->DeviceType == FILE_DEVICE_CD_ROM ||
      CreatedDeviceObject->DeviceType == FILE_DEVICE_TAPE)
    {
      IoAttachVpb(CreatedDeviceObject);
    }
  
  *DeviceObject = CreatedDeviceObject;
  
  return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
IoOpenDeviceInstanceKey (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


DWORD
STDCALL
IoQueryDeviceEnumInfo (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
  UNIMPLEMENTED;
  return 0;
}


/* EOF */
