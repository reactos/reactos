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
#include <internal/ob.h>
#include <internal/mm.h>
#include <internal/ps.h>

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
	
	/* free input buffer (data transfer buffer) */
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
	/* FIXME: Is the MDL destroyed on a paging i/o, check all cases. */
	DPRINT("Tearing down MDL\n");
	if (Irp->MdlAddress->MappedSystemVa != NULL)
	  {	     
	     MmUnmapLockedPages(Irp->MdlAddress->MappedSystemVa,
				Irp->MdlAddress);
	  }
	MmUnlockPages(Irp->MdlAddress);
	ExFreePool(Irp->MdlAddress);
     }
}

VOID IoVolumeInformationCompletion(PDEVICE_OBJECT DeviceObject,
				   PIRP Irp,
				   PIO_STACK_LOCATION IoStack)
{
}


/*
 * @implemented
 */
VOID STDCALL
IoSecondStageCompletion_KernelApcRoutine(
    IN PKAPC Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )
{
   IoFreeIrp((PIRP)(*SystemArgument1));
}


/*
 * @implemented
 */
VOID STDCALL
IoSecondStageCompletion_RundownApcRoutine(
   IN PKAPC Apc
   )
{
   PIRP Irp;

   Irp = CONTAINING_RECORD(Apc, IRP, Tail.Apc);
   IoFreeIrp(Irp);
}


/*
 * FUNCTION: Performs the second stage of irp completion for read/write irps
 * 
 * Called as a special kernel APC or directly from IofCompleteRequest()
 *
 * @implemented
 */
VOID STDCALL
IoSecondStageCompletion(
   PKAPC Apc,
   PKNORMAL_ROUTINE* NormalRoutine,
   PVOID* NormalContext,
   PVOID* SystemArgument1,
   PVOID* SystemArgument2)

{
   PIO_STACK_LOCATION   IoStack;
   PDEVICE_OBJECT       DeviceObject;
   PFILE_OBJECT         OriginalFileObject;
   PIRP                 Irp;
   CCHAR                PriorityBoost;

   OriginalFileObject = (PFILE_OBJECT)(*NormalContext);
   Irp = (PIRP)(*SystemArgument1);
   PriorityBoost = (CCHAR)(LONG)(*SystemArgument2);
   
   IoStack = &Irp->Stack[(ULONG)Irp->CurrentLocation];
   DeviceObject = IoStack->DeviceObject;

   DPRINT("IoSecondStageCompletion(Irp %x, PriorityBoost %d)\n", Irp, PriorityBoost);

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
   break;
     }
   
   if (Irp->UserIosb!=NULL)
   {
      *Irp->UserIosb=Irp->IoStatus;
   }

   if (Irp->UserEvent)
   {
      KeSetEvent(Irp->UserEvent,PriorityBoost,FALSE);
   }

   //Windows NT File System Internals, page 169
   if (OriginalFileObject)
   {
      if (Irp->UserEvent == NULL)
      {
         KeSetEvent(&OriginalFileObject->Event,PriorityBoost,FALSE);         
      }
      else if (OriginalFileObject->Flags & FO_SYNCHRONOUS_IO && Irp->UserEvent != &OriginalFileObject->Event)
      {
         KeSetEvent(&OriginalFileObject->Event,PriorityBoost,FALSE);         
      }
   }

   //Windows NT File System Internals, page 154
   if (!(Irp->Flags & IRP_PAGING_IO) && OriginalFileObject)
   {
      // if the event is not the one in the file object, it needs dereferenced   
      if (Irp->UserEvent && Irp->UserEvent != &OriginalFileObject->Event)   
      {
         ObDereferenceObject(Irp->UserEvent);
      }
  
      if (IoStack->MajorFunction != IRP_MJ_CLOSE)
      {
         ObDereferenceObject(OriginalFileObject);
      }
   }

   if (Irp->Overlay.AsynchronousParameters.UserApcRoutine != NULL)
   {
      PKNORMAL_ROUTINE UserApcRoutine;
      PVOID UserApcContext;
   
      DPRINT("Dispatching user APC\n");

      UserApcRoutine = (PKNORMAL_ROUTINE)Irp->Overlay.AsynchronousParameters.UserApcRoutine;
      UserApcContext = (PVOID)Irp->Overlay.AsynchronousParameters.UserApcContext;

      KeInitializeApc(  &Irp->Tail.Apc,
                        KeGetCurrentThread(),
                        OriginalApcEnvironment,
                        IoSecondStageCompletion_KernelApcRoutine,
                        IoSecondStageCompletion_RundownApcRoutine,
                        UserApcRoutine,
                        UserMode,
                        UserApcContext);

      KeInsertQueueApc( &Irp->Tail.Apc,
                        Irp,
                        NULL,
                        PriorityBoost);

      //NOTE: kernel (or rundown) routine frees the IRP

      return;

   }

   IoFreeIrp(Irp);
   
}
