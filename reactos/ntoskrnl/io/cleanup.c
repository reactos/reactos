/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/cleanup.c
 * PURPOSE:        IRP cleanup
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 30/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/string.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID IoReadWriteCompletion(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp,
			   PIO_STACK_LOCATION IoStack)
{
   PFILE_OBJECT FileObject;
   
   FileObject = IoStack->FileObject;
   
   DPRINT("FileObject %x\n",FileObject);
   
   if (DeviceObject->Flags & DO_BUFFERED_IO && 
       IoStack->MajorFunction == IRP_MJ_READ)
     {
        DPRINT("Copying buffered io back to user\n");
	memcpy(Irp->UserBuffer,Irp->AssociatedIrp.SystemBuffer,
	       IoStack->Parameters.Read.Length);
	ExFreePool(Irp->AssociatedIrp.SystemBuffer);
     }
   if (DeviceObject->Flags & DO_DIRECT_IO)
     {
	DPRINT("Tearing down MDL\n");
	if (Irp->MdlAddress->MappedSystemVa!=NULL)
	  {	     
	     MmUnmapLockedPages(Irp->MdlAddress->MappedSystemVa,
				Irp->MdlAddress);
	  }
	MmUnlockPages(Irp->MdlAddress);
	ExFreePool(Irp->MdlAddress);
     }
   if (FileObject != NULL)
     {
	SET_LARGE_INTEGER_LOW_PART(FileObject->CurrentByteOffset,
          GET_LARGE_INTEGER_LOW_PART(FileObject->CurrentByteOffset) + 
          Irp->IoStatus.Information);
     }
}

VOID IoSecondStageCompletion(PIRP Irp, CCHAR PriorityBoost)
/*
 * FUNCTION: Performs the second stage of irp completion for read/write irps
 * ARGUMENTS:
 *          Irp = Irp to completion
 *          FromDevice = True if the operation transfered data from the device
 */
{
   PIO_STACK_LOCATION IoStack;
   PDEVICE_OBJECT DeviceObject;
   
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   
   DeviceObject = IoStack->DeviceObject;
   
   switch (IoStack->MajorFunction)
     {
      case IRP_MJ_CREATE:
	/* NOP */
	break;
	
      case IRP_MJ_READ:
      case IRP_MJ_WRITE:
	IoReadWriteCompletion(DeviceObject,Irp,IoStack);
	break;
	
      default:
     }

   if (Irp->UserIosb!=NULL)
     {
	*Irp->UserIosb=Irp->IoStatus;
     }
   if (Irp->UserEvent!=NULL)
     {
	KeSetEvent(Irp->UserEvent,PriorityBoost,FALSE);
     }
   
   IoFreeIrp(Irp);
}
