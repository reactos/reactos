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

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


VOID IoDeviceControlCompletion(PDEVICE_OBJECT DeviceObject,
			       PIRP Irp,
			       PIO_STACK_LOCATION IoStack)
{
   ULONG IoControlCode;
   
   IoControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;
   
   switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
     {
      case METHOD_BUFFERED:
	DPRINT ("Using METHOD_BUFFERED!\n");
	
	/* copy output buffer back and free it */
	if (Irp->AssociatedIrp.SystemBuffer)
	  {
	     if (IoStack->Parameters.DeviceIoControl.OutputBufferLength)
	       {
		  RtlCopyMemory(Irp->UserBuffer,
				Irp->AssociatedIrp.SystemBuffer,
				IoStack->Parameters.DeviceIoControl.
				OutputBufferLength);
	       }
	     ExFreePool (Irp->AssociatedIrp.SystemBuffer);
	  }
	break;
	
      case METHOD_IN_DIRECT:
	DPRINT ("Using METHOD_IN_DIRECT!\n");
	
	/* free input buffer (control buffer) */
	if (Irp->AssociatedIrp.SystemBuffer)
	  ExFreePool (Irp->AssociatedIrp.SystemBuffer);
	
	/* free output buffer (data transfer buffer) */
	if (Irp->MdlAddress)
	  IoFreeMdl (Irp->MdlAddress);
	break;
	
      case METHOD_OUT_DIRECT:
	DPRINT ("Using METHOD_OUT_DIRECT!\n");
	
	/* free input buffer (control buffer) */
	if (Irp->AssociatedIrp.SystemBuffer)
	  ExFreePool (Irp->AssociatedIrp.SystemBuffer);
	
	/* free output buffer (data transfer buffer) */
	if (Irp->MdlAddress)
	  IoFreeMdl (Irp->MdlAddress);
	break;
	
      case METHOD_NEITHER:
	DPRINT ("Using METHOD_NEITHER!\n");
	/* nothing to do */
	break;
     }
}

VOID IoReadWriteCompletion(PDEVICE_OBJECT DeviceObject,
			   PIRP Irp,
			   PIO_STACK_LOCATION IoStack)
{   
   PFILE_OBJECT FileObject;
   
   FileObject = IoStack->FileObject;
   
   if (DeviceObject->Flags & DO_BUFFERED_IO)     
     {
	if (IoStack->MajorFunction == IRP_MJ_READ)
	  {
	     DPRINT("Copying buffered io back to user\n");
	     memcpy(Irp->UserBuffer,Irp->AssociatedIrp.SystemBuffer,
		    IoStack->Parameters.Read.Length);
	  }
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
        FileObject->CurrentByteOffset.u.LowPart =
          FileObject->CurrentByteOffset.u.LowPart +
          Irp->IoStatus.Information;
     }
}

VOID IoVolumeInformationCompletion(PDEVICE_OBJECT DeviceObject,
				   PIRP Irp,
				   PIO_STACK_LOCATION IoStack)
{
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
   PFILE_OBJECT FileObject;
   
   IoStack = IoGetCurrentIrpStackLocation(Irp);
   
   DeviceObject = IoStack->DeviceObject;
   
   switch (IoStack->MajorFunction)
     {
      case IRP_MJ_CREATE:
      case IRP_MJ_FLUSH_BUFFERS:
	  /* NOP */
	break;
	
      case IRP_MJ_READ:
      case IRP_MJ_WRITE:
	IoReadWriteCompletion(DeviceObject,Irp,IoStack);
	break;
	
      case IRP_MJ_DEVICE_CONTROL:
      case IRP_MJ_INTERNAL_DEVICE_CONTROL:
	IoDeviceControlCompletion(DeviceObject, Irp, IoStack);
	break;
	
      case IRP_MJ_QUERY_VOLUME_INFORMATION:
      case IRP_MJ_SET_VOLUME_INFORMATION:
	IoVolumeInformationCompletion(DeviceObject, Irp, IoStack);
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

   FileObject = IoStack->FileObject;
   
   if (FileObject != NULL && IoStack->MajorFunction != IRP_MJ_CLOSE)
     {
	ObDereferenceObject(FileObject);
     }
   
   IoFreeIrp(Irp);
}
