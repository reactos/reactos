/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           mkernel/iomgr/device.cc
 * PURPOSE:        Manage devices
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

#include <internal/kernel.h>
#include <internal/iomgr.h>
#include <internal/objmgr.h>
#include <internal/string.h>

//#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

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

PDEVICE_OBJECT IoAttachDeviceToDeviceStack(PDEVICE_OBJECT SourceDevice,
					   PDEVICE_OBJECT TargetDevice)
{
   UNIMPLEMENTED;
}

VOID IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
				      PDRIVER_REINITIALIZE ReinitRoutine,
				      PVOID Context)
{
   UNIMPLEMENTED;
}


NTSTATUS InitalizeLoadedDriver(PDRIVER_INITIALIZE entry)
/*
 * FUNCTION: Called to initalize a loaded driver
 * ARGUMENTS:
 */
{
   NTSTATUS ret;
   PDRIVER_OBJECT DriverObject;
   
   /*
    * Allocate memory for a driver object
    * NOTE: The object only becomes system visible once the associated
    * device objects are intialized
    */
   DriverObject=ExAllocatePool(NonPagedPool,sizeof(DRIVER_OBJECT));
   if (DriverObject==NULL)
     {
	printk("%s:%d\n",__FILE__,__LINE__);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   memset(DriverObject,sizeof(DRIVER_OBJECT),0);
   
   /*
    * Initalize the driver
    * FIXME: Registry in general please
    */
   if ((ret=entry(DriverObject,NULL))!=STATUS_SUCCESS)
     {
        DPRINT("Failed to load driver (status %x)\n",ret);
	ExFreePool(DriverObject);
	return(ret);
     }
   return(STATUS_SUCCESS);
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
   PDEVICE_OBJECT dev;
   OBJECT_ATTRIBUTES dev_attr;
   HANDLE devh;
   
   InitializeObjectAttributes(&dev_attr,DeviceName,0,NULL,NULL);
   dev = ObGenericCreateObject(&devh,0,&dev_attr,OBJTYP_DEVICE);
					      
   *DeviceObject=NULL;
   
   if (dev==NULL)
     {
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   if (DriverObject->DeviceObject==NULL)
     {
	DriverObject->DeviceObject = dev;
	dev->NextDevice=NULL;
     }
   else
     {
	dev->NextDevice=DriverObject->DeviceObject;
	DriverObject->DeviceObject=dev;
     }
   
   dev->DriverObject = DriverObject;
   dev->CurrentIrp=NULL;
   dev->Flags=0;

   dev->DeviceExtension=ExAllocatePool(NonPagedPool,DeviceExtensionSize);
   if (DeviceExtensionSize > 0 && dev->DeviceExtension==NULL)
     {
	ExFreePool(dev);
	return(STATUS_INSUFFICIENT_RESOURCES);
     }
   
   dev->DeviceType=DeviceType;
   dev->StackSize=1;
   dev->AlignmentRequirement=1;
   
   *DeviceObject=dev;
   
   return(STATUS_SUCCESS);
}
