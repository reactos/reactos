/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/cntrller.c
 * PURPOSE:         Implements the controller object
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct
/*
 * PURPOSE: A entry in the queue waiting for a controller object
 */
{
   KDEVICE_QUEUE_ENTRY Entry;
   PDEVICE_OBJECT DeviceObject;
   PDRIVER_CONTROL ExecutionRoutine;
   PVOID Context;
} CONTROLLER_QUEUE_ENTRY, *PCONTROLLER_QUEUE_ENTRY;

/* FUNCTIONS *****************************************************************/

VOID IoAllocateController(PCONTROLLER_OBJECT ControllerObject,
			  PDEVICE_OBJECT DeviceObject,
			  PDRIVER_CONTROL ExecutionRoutine,
			  PVOID Context)
/*
 * FUNCTION: Sets up a call to a driver-supplied ControllerControl routine
 * as soon as the device controller, represented by the given controller
 * object, is available to carry out an I/O operation for the target device,
 * represented by the given device object.
 * ARGUMENTS:
 *       ControllerObject = Driver created controller object
 *       DeviceObject = Target device for the current irp
 *       ExecutionRoutine = Routine to be called when the device is available
 *       Context = Driver supplied context to be passed on to the above routine
 * NOTE: Is the below implementation correct. 
 */
{
   PCONTROLLER_QUEUE_ENTRY entry;
   IO_ALLOCATION_ACTION Result;
   
   assert_irql(DISPATCH_LEVEL);
   
   entry=ExAllocatePool(NonPagedPool,sizeof(CONTROLLER_QUEUE_ENTRY));
   assert(entry!=NULL);
   
   entry->DeviceObject = DeviceObject;
   entry->ExecutionRoutine = ExecutionRoutine;
   entry->Context = Context;
   
   if (KeInsertDeviceQueue(&ControllerObject->DeviceWaitQueue,&entry->Entry))
     {
	return;
     }   
   Result = ExecutionRoutine(DeviceObject,DeviceObject->CurrentIrp,
			     NULL,Context);
   if (Result == DeallocateObject)
     {
	IoFreeController(ControllerObject);
     }
   ExFreePool(entry);
}

PCONTROLLER_OBJECT IoCreateController(ULONG Size)
/*
 * FUNCTION: Allocates memory and initializes a controller object
 * ARGUMENTS:
 *        Size = Size (in bytes) to be allocated for the controller extension
 * RETURNS: A pointer to the created object
 */
{
   PCONTROLLER_OBJECT controller;
   
   assert_irql(PASSIVE_LEVEL);
   
   controller = ExAllocatePool(NonPagedPool,sizeof(CONTROLLER_OBJECT));
   if (controller==NULL)
     {
	return(NULL);
     }
   
   controller->ControllerExtension=ExAllocatePool(NonPagedPool,Size);
   if (controller->ControllerExtension==NULL)
     {
	ExFreePool(controller);
	return(NULL);
     }

   KeInitializeDeviceQueue(&controller->DeviceWaitQueue);
   return(controller);
}

VOID IoDeleteController(PCONTROLLER_OBJECT ControllerObject)
/*
 * FUNCTION: Removes a given controller object from the system
 * ARGUMENTS:
 *        ControllerObject = Controller object to be released
 */
{
   assert_irql(PASSIVE_LEVEL);

   ExFreePool(ControllerObject->ControllerExtension);
   ExFreePool(ControllerObject);
}

VOID IoFreeController(PCONTROLLER_OBJECT ControllerObject)
/*
 * FUNCTION: Releases a previously allocated controller object when a 
 * device has finished an I/O request
 * ARGUMENTS:
 *       ControllerObject = Controller object to be released
 */
{
   PKDEVICE_QUEUE_ENTRY QEntry;
   CONTROLLER_QUEUE_ENTRY* Entry;
   IO_ALLOCATION_ACTION Result;

   do
     {
	QEntry = KeRemoveDeviceQueue(&ControllerObject->DeviceWaitQueue);
	Entry = CONTAINING_RECORD(QEntry,CONTROLLER_QUEUE_ENTRY,Entry);
	if (QEntry==NULL)
	  {
	     return;
	  }
	Result = Entry->ExecutionRoutine(Entry->DeviceObject,
					 Entry->DeviceObject->CurrentIrp,
					 NULL,					 
					 Entry->Context);
	ExFreePool(Entry);
     } while (Result == DeallocateObject);
}

