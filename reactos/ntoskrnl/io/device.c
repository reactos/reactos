/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/device.c
 * PURPOSE:        Manage devices
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

#include <internal/io.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


NTSTATUS STDCALL NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
   return(ZwUnloadDriver(DriverServiceName));
}

NTSTATUS STDCALL ZwUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
   UNIMPLEMENTED;
}

NTSTATUS NtLoadDriver(PUNICODE_STRING DriverServiceName)
{
   return(ZwLoadDriver(DriverServiceName));
}

NTSTATUS ZwLoadDriver(PUNICODE_STRING DriverServiceName)
/*
 * FUNCTION: Loads a driver
 * ARGUMENTS:
 *         DriverServiceName = Name of the service to load (registry key)
 * RETURNS: Status
 */
{
   UNIMPLEMENTED;
}

NTSTATUS IoAttachDeviceByPointer(PDEVICE_OBJECT SourceDevice,
				 PDEVICE_OBJECT TargetDevice)
{
   UNIMPLEMENTED;
}

VOID IoDeleteDevice(PDEVICE_OBJECT DeviceObject)
{
   UNIMPLEMENTED;
}

PDEVICE_OBJECT IoGetRelatedDeviceObject(PFILE_OBJECT FileObject)
{
   return(FileObject->DeviceObject);
}

NTSTATUS IoGetDeviceObjectPointer(PUNICODE_STRING ObjectName,
				  ACCESS_MASK DesiredAccess,
				  PFILE_OBJECT* FileObject,
				  PDEVICE_OBJECT* DeviceObject)
{
   UNIMPLEMENTED;
}

VOID IoDetachDevice(PDEVICE_OBJECT TargetDevice)
{
   UNIMPLEMENTED;
}

PDEVICE_OBJECT IoGetAttachedDevice(PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_OBJECT Current = DeviceObject;
   
   DPRINT("IoGetAttachDevice(DeviceObject %x)\n",DeviceObject);
   
   while (Current->AttachedDevice!=NULL)
     {
	Current = Current->AttachedDevice;
	DPRINT("Current %x\n",Current);
     }
   return(Current);
}

PDEVICE_OBJECT IoAttachDeviceToDeviceStack(PDEVICE_OBJECT SourceDevice,
					   PDEVICE_OBJECT TargetDevice)
{
   PDEVICE_OBJECT AttachedDevice = IoGetAttachedDevice(TargetDevice);
   
   DPRINT("IoAttachDeviceToDeviceStack(SourceDevice %x, TargetDevice %x)\n",
	  SourceDevice,TargetDevice);
   
   AttachedDevice->AttachedDevice = SourceDevice;
   SourceDevice->StackSize = AttachedDevice->StackSize + 1;
   SourceDevice->Vpb = AttachedDevice->Vpb;
   return(AttachedDevice);
}

VOID IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
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
   
   DriverObject->Type = ID_DRIVER_OBJECT;
   
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

NTSTATUS IoAttachDevice(PDEVICE_OBJECT SourceDevice,
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
   PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)ObjectBody;
   
   DPRINT("IopCreateDevice(ObjectBody %x, Parent %x, RemainingPath %w)\n",
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


NTSTATUS IoCreateDevice(PDRIVER_OBJECT DriverObject,
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

   if (DeviceName != NULL)
     {
	DPRINT("IoCreateDevice(DriverObject %x, DeviceName %w)\n",DriverObject,
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
					     IoDeviceType);
     }
   else
     {
	CreatedDeviceObject = ObCreateObject(&DeviceHandle,
					     0,
					     NULL,
					     IoDeviceType);
     }
					      
   *DeviceObject=NULL;
   
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
   
   CreatedDeviceObject->Type = ID_DEVICE_OBJECT;
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
