/*
 * COPYRIGHT:                See COPYING in the top level directory
 * PROJECT:                  ReactOS kernel
 * FILE:                     ntoskrnl/io/queue.c
 * PURPOSE:                  Implement device queueing
 * PROGRAMMER:               David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID IoStartNextPacketByKey(PDEVICE_OBJECT DeviceObject,
			    BOOLEAN Cancelable,
			    ULONG Key)
/*
 * FUNCTION: Dequeues the next packet from the given device object's
 * associated device queue according to a specified sort-key value and calls
 * the drivers StartIo routine with that IRP
 * ARGUMENTS:
 *      DeviceObject = Device object for which the irp is to dequeued
 *      Cancelable = True if IRPs in the key can be canceled
 *      Key = Sort key specifing which entry to remove from the queue
 */
{
   PKDEVICE_QUEUE_ENTRY entry;
   PIRP Irp;
   KIRQL oldirql;
   
   entry = KeRemoveByKeyDeviceQueue(&DeviceObject->DeviceQueue,Key);
   
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

VOID IoStartNextPacket(PDEVICE_OBJECT DeviceObject, BOOLEAN Cancelable)
/*
 * FUNCTION: Removes the next packet from the device's queue and calls
 * the driver's StartIO
 * ARGUMENTS:
 *         DeviceObject = Device
 *         Cancelable = True if irps in the queue can be canceled
 */
{
   PKDEVICE_QUEUE_ENTRY entry;
   PIRP Irp;
   KIRQL oldirql;
   
   DPRINT("IoStartNextPacket(DeviceObject %x, Cancelable %d)\n",
	  DeviceObject,Cancelable);
   
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

VOID IoStartPacket(PDEVICE_OBJECT DeviceObject,
		   PIRP Irp, PULONG Key, PDRIVER_CANCEL CancelFunction)
/*
 * FUNCTION: Either call the device's StartIO routine with the packet or,
 * if the device is busy, queue it.
 * ARGUMENTS:
 *       DeviceObject = Device to start the packet on
 *       Irp = Irp to queue
 *       Key = Where to insert the irp
 *             If zero then insert in the tail of the queue
 *       CancelFunction = Optional function to cancel the irqp
 */
{
   BOOLEAN stat;
   KIRQL oldirql;

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
   
   IoReleaseCancelSpinLock(oldirql);
   
   if (!stat)
     {			   
        DeviceObject->CurrentIrp = Irp;
	DeviceObject->DriverObject->DriverStartIo(DeviceObject,Irp);
     }
}


