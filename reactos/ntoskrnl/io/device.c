/* $Id: device.c,v 1.21 2000/09/10 13:54:01 ekohl Exp $
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
#include <internal/ob.h>
#include <internal/ldr.h>
#include <internal/id.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>


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
  /* FIXME: this should lookup the filename from the registry and then call LdrLoadDriver  */
  return  LdrLoadDriver (DriverServiceName);
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

VOID
STDCALL
IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
				 PDRIVER_REINITIALIZE ReinitRoutine,
				 PVOID Context)
{
   UNIMPLEMENTED;
}

NTSTATUS IopDefaultDispatchFunction(PDEVICE_OBJECT DeviceObject,
				    PIRP Irp)
{
   Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
   Irp->IoStatus.Information = 0;
   
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS IoInitializeDriver(PDRIVER_INITIALIZE DriverEntry)
/*
 * FUNCTION: Called to initalize a loaded driver
 * ARGUMENTS:
 */
{
   NTSTATUS Status;
   PDRIVER_OBJECT DriverObject;
   ULONG i;
   
   DriverObject = ExAllocatePool(NonPagedPool,sizeof(DRIVER_OBJECT));
   if (DriverObject == NULL)
     {
	return STATUS_INSUFFICIENT_RESOURCES;
     }
   memset(DriverObject, 0, sizeof(DRIVER_OBJECT));
   
   DriverObject->Type = InternalDriverType;
   
   for (i=0; i<=IRP_MJ_MAXIMUM_FUNCTION; i++)
     {
	DriverObject->MajorFunction[i] = IopDefaultDispatchFunction;
     }

   DPRINT("Calling driver entrypoint at %08lx\n", DriverEntry);
   Status = DriverEntry(DriverObject, NULL);
   if (!NT_SUCCESS(Status))
     {
	ExFreePool(DriverObject);
	return(Status);
     }

   return(Status);
}

NTSTATUS
STDCALL
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

NTSTATUS IopCreateDevice(PVOID ObjectBody,
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


NTSTATUS
STDCALL
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

   CreatedDeviceObject->DeviceExtension = ExAllocatePool(NonPagedPool,
							 DeviceExtensionSize);
   if (DeviceExtensionSize > 0 && CreatedDeviceObject->DeviceExtension == NULL)
     {
	ExFreePool(CreatedDeviceObject);
	return(STATUS_INSUFFICIENT_RESOURCES);
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


VOID
STDCALL
IoSetDeviceToVerify (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
}

/* EOF */
