/* $Id: device.c,v 1.27 2001/05/01 23:08:19 chorns Exp $
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
#include <internal/ob.h>
#include <internal/ldr.h>
#include <internal/id.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <internal/config.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_DRIVER             TAG('D', 'R', 'V', 'R')
#define TAG_DRIVER_EXTENSION   TAG('D', 'R', 'V', 'E')
#define TAG_DEVICE_EXTENSION   TAG('D', 'E', 'X', 'T')

/* FUNCTIONS ***************************************************************/


NTSTATUS STDCALL NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
   UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtLoadDriver
 *
 * DESCRIPTION
 * 	Loads a device driver.
 * 	
 * ARGUMENTS
 *	DriverServiceName
 *		Name of the service to load (registry key).
 *		
 * RETURN VALUE
 * 	Status.
 *
 * REVISIONS
 */
NTSTATUS
STDCALL
NtLoadDriver (
	PUNICODE_STRING	DriverServiceName
	)
{
  PDEVICE_NODE DeviceNode;
  NTSTATUS Status;

  /* FIXME: this should lookup the filename from the registry and then call LdrLoadDriver  */

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
  return(Status);
    }

  Status = LdrLoadDriver (DriverServiceName, DeviceNode, FALSE);
  if (!NT_SUCCESS(Status))
    {
  IopFreeDeviceNode(DeviceNode);
  DPRINT("Driver load failed, status (%x)\n", Status);
    }

  return Status;
}


NTSTATUS
STDCALL
IoAttachDeviceByPointer (
	IN	PDEVICE_OBJECT	SourceDevice,
	IN	PDEVICE_OBJECT	TargetDevice
	)
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


VOID
STDCALL
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
IopCreateDriverObject(PDRIVER_OBJECT *DriverObject)
{
  PDRIVER_OBJECT Object;
  ULONG i;

  Object = ExAllocatePoolWithTag(NonPagedPool,
       sizeof(DRIVER_OBJECT),
       TAG_DRIVER);
  if (Object == NULL)
    {
	return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory(Object, sizeof(DRIVER_OBJECT));

  Object->DriverExtension = (PDRIVER_EXTENSION)
    ExAllocatePoolWithTag(NonPagedPool,
       sizeof(DRIVER_EXTENSION),
       TAG_DRIVER_EXTENSION);
  if (Object->DriverExtension == NULL)
    {
  ExFreePool(Object);
	return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory(Object->DriverExtension, sizeof(DRIVER_EXTENSION));

  Object->Type = InternalDriverType;
   
  for (i=0; i<=IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
  Object->MajorFunction[i] = IopDefaultDispatchFunction;
    }

  *DriverObject = Object;

  return STATUS_SUCCESS;
}

NTSTATUS 
IopInitializeDriver(PDRIVER_INITIALIZE DriverEntry,
                    PDEVICE_NODE DeviceNode,
                    BOOLEAN BootDriversOnly)
/*
 * FUNCTION: Called to initalize a loaded driver
 * ARGUMENTS:
 */
{
  IO_STATUS_BLOCK	IoStatusBlock;
  PDRIVER_OBJECT DriverObject;
  PIO_STACK_LOCATION IrpSp;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  DPRINT("IopInitializeDriver (DriverEntry %08lx, DeviceNode %08lx, "
    "BootDriversOnly %d)\n", DriverEntry, DeviceNode, BootDriversOnly);

  Status = IopCreateDriverObject(&DriverObject);
  if (!NT_SUCCESS(Status))
    {
  return(Status);
    }

  DPRINT("Calling driver entrypoint at %08lx\n", DriverEntry);
  Status = DriverEntry(DriverObject, NULL);
  if (!NT_SUCCESS(Status))
    {
  ExFreePool(DriverObject->DriverExtension);
	ExFreePool(DriverObject);
	return(Status);
    }

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
          ExFreePool(DriverObject->DriverExtension);
	        ExFreePool(DriverObject);
	        return(Status);
        }
      DPRINT("Sending IRP_MN_START_DEVICE to driver\n");

      Fdo = IoGetAttachedDeviceReference(DeviceNode->Pdo);

      if (Fdo == DeviceNode->Pdo)
        {
          /* FIXME: What do we do? Unload the driver? */
          DbgPrint("An FDO was not attached\n");
          KeBugCheck(0);
        }

      KeInitializeEvent(&Event,
	      NotificationEvent,
	      FALSE);

      Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
        Fdo,
	      NULL,
	      0,
	      NULL,
	      &Event,
	      &IoStatusBlock);

      IrpSp = IoGetNextIrpStackLocation(Irp);
      IrpSp->MinorFunction = IRP_MN_START_DEVICE;

      /* FIXME: Put some resources in the IRP for the device */

	    Status = IoCallDriver(Fdo, Irp);

	    if (Status == STATUS_PENDING)
	      {
		      KeWaitForSingleObject(&Event,
		                       Executive,
		                       KernelMode,
		                       FALSE,
		                       NULL);
          Status = IoStatusBlock.Status;
	      }
      if (!NT_SUCCESS(Status))
        {
          ObDereferenceObject(Fdo);
          ExFreePool(DriverObject->DriverExtension);
	        ExFreePool(DriverObject);
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
              ExFreePool(DriverObject->DriverExtension);
	            ExFreePool(DriverObject);
	            return(Status);
            }
          else
            {
#ifdef ACPI
              static BOOLEAN SystemPowerDeviceNodeCreated = FALSE;

              /* The system power device node is the first bus enumerator
                 device node created after the root device node */
              if (!SystemPowerDeviceNodeCreated)
                {
                  PopSystemPowerDeviceNode = DeviceNode;
                  SystemPowerDeviceNodeCreated = TRUE;
                }
#endif /* ACPI */
            }
        }
      ObDereferenceObject(Fdo);
    }

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

NTSTATUS 
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
   
   if (Parent != NULL && RemainingPath != NULL)
     {
	ObAddEntryDirectory(Parent, ObjectBody, RemainingPath+1);
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
	CreatedDeviceObject = ObCreateObject(&DeviceHandle,
					     0,
					     &ObjectAttributes,
					     IoDeviceObjectType);
     }
   else
     {
	CreatedDeviceObject = ObCreateObject(&DeviceHandle,
					     0,
					     NULL,
					     IoDeviceObjectType);
     }
   
   *DeviceObject = NULL;
   
   if (CreatedDeviceObject == NULL)
     {
	return(STATUS_INSUFFICIENT_RESOURCES);
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
   
   if (CreatedDeviceObject->DeviceType == FILE_DEVICE_DISK)
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
	return (STATUS_NOT_IMPLEMENTED);
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


PDEVICE_OBJECT STDCALL
IoGetDeviceToVerify (PETHREAD Thread)
/*
 * FUNCTION: Returns a pointer to the device, representing a removable-media
 * device, that is the target of the given thread's I/O request
 */
{
   return Thread->DeviceToVerify;
}


VOID STDCALL
IoSetDeviceToVerify (IN PETHREAD Thread,
		     IN PDEVICE_OBJECT DeviceObject)
{
   Thread->DeviceToVerify = DeviceObject;
}


VOID STDCALL
IoSetHardErrorOrVerifyDevice(PIRP Irp,
			     PDEVICE_OBJECT DeviceObject)
{
   Irp->Tail.Overlay.Thread->DeviceToVerify = DeviceObject;
}


/* EOF */
