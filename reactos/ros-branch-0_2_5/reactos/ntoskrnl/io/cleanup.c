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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID IoDeviceControlCompletion(PDEVICE_OBJECT DeviceObject,
			       PIRP Irp,
			       PIO_STACK_LOCATION IoStack)
{
   ULONG IoControlCode;
   ULONG OutputBufferLength;

   if (IoStack->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)
     {
       IoControlCode = 
	 ((PEXTENDED_IO_STACK_LOCATION)IoStack)->Parameters.FileSystemControl.FsControlCode;
       OutputBufferLength = 
	 ((PEXTENDED_IO_STACK_LOCATION)IoStack)->Parameters.FileSystemControl.OutputBufferLength;
     }
   else
     {
       IoControlCode = IoStack->Parameters.DeviceIoControl.IoControlCode;
       if (NT_SUCCESS(Irp->IoStatus.Status))
         {
           OutputBufferLength = Irp->IoStatus.Information;
           if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < OutputBufferLength)
             {
               OutputBufferLength = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
             }
         }
       else
         {
           OutputBufferLength = 0;
         }
     }
   
   switch (IO_METHOD_FROM_CTL_CODE(IoControlCode))
     {
      case METHOD_BUFFERED:
	DPRINT ("Using METHOD_BUFFERED!\n");
	
	/* copy output buffer back and free it */
	if (Irp->AssociatedIrp.SystemBuffer)
	  {
	     if (OutputBufferLength)
	       {
		  RtlCopyMemory(Irp->UserBuffer,
				Irp->AssociatedIrp.SystemBuffer,
				OutputBufferLength);
	       }
	     ExFreePool (Irp->AssociatedIrp.SystemBuffer);
	  }
	break;
	
      case METHOD_IN_DIRECT:
	DPRINT ("Using METHOD_IN_DIRECT!\n");
	/* use the same code as for METHOD_OUT_DIRECT */
	
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
      IoFreeMdl(Irp->MdlAddress);
   }
}

VOID IoVolumeInformationCompletion(PDEVICE_OBJECT DeviceObject,
				   PIRP Irp,
				   PIO_STACK_LOCATION IoStack)
{
}


VOID STDCALL
IoSecondStageCompletion_KernelApcRoutine(
    IN PKAPC Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )
{
   PIRP Irp;

   Irp = CONTAINING_RECORD(Apc, IRP, Tail.Apc);
   IoFreeIrp(Irp);
}


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
 * Called as a special kernel APC kernel-routine or directly from IofCompleteRequest()
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

   if (Apc) DPRINT("IoSecondStageCompletition with APC: %x\n", Apc);
   
   OriginalFileObject = (PFILE_OBJECT)(*SystemArgument1);
   DPRINT("OriginalFileObject: %x\n", OriginalFileObject);

   Irp = CONTAINING_RECORD(Apc, IRP, Tail.Apc);
   DPRINT("Irp: %x\n", Irp);
   
   /*
    * Note that we'll never see irp's flagged IRP_PAGING_IO (IRP_MOUNT_OPERATION)
    * or IRP_CLOSE_OPERATION (IRP_MJ_CLOSE and IRP_MJ_CLEANUP) here since their
    * cleanup/completion is fully taken care of in IoCompleteRequest.
    * -Gunnar
    */
    
   /* 
   Remove synchronous irp's from the threads cleanup list.
   To synchronize with the code inserting the entry, this code must run 
   at APC_LEVEL
   */
   if (!IsListEmpty(&Irp->ThreadListEntry))
   {
     RemoveEntryList(&Irp->ThreadListEntry);
     InitializeListHead(&Irp->ThreadListEntry);
   }
   
   IoStack =  (PIO_STACK_LOCATION)(Irp+1) + Irp->CurrentLocation;
   DeviceObject = IoStack->DeviceObject;

   DPRINT("IoSecondStageCompletion(Irp %x, MajorFunction %x)\n", Irp, IoStack->MajorFunction);

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
      case IRP_MJ_FILE_SYSTEM_CONTROL:
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
      if (Irp->RequestorMode == KernelMode)
      {
	*Irp->UserIosb = Irp->IoStatus;
      }
      else
      {
	DPRINT("Irp->RequestorMode == UserMode\n");
	MmSafeCopyToUser(Irp->UserIosb,
			 &Irp->IoStatus,
			 sizeof(IO_STATUS_BLOCK));
      }
   }

   if (Irp->UserEvent)
   {
      KeSetEvent(Irp->UserEvent,0,FALSE);
   }

   //Windows NT File System Internals, page 169
   if (OriginalFileObject)
   {
      if (Irp->UserEvent == NULL)
      {
         KeSetEvent(&OriginalFileObject->Event,0,FALSE);
      }
      else if (OriginalFileObject->Flags & FO_SYNCHRONOUS_IO && Irp->UserEvent != &OriginalFileObject->Event)
      {
         KeSetEvent(&OriginalFileObject->Event,0,FALSE);
      }
   }

   //Windows NT File System Internals, page 154
   if (OriginalFileObject)   
   {
      // if the event is not the one in the file object, it needs dereferenced
      if (Irp->UserEvent && Irp->UserEvent != &OriginalFileObject->Event)
      {
         ObDereferenceObject(Irp->UserEvent);
      }
  
      ObDereferenceObject(OriginalFileObject);
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
                        CurrentApcEnvironment,
                        IoSecondStageCompletion_KernelApcRoutine,
                        IoSecondStageCompletion_RundownApcRoutine,
                        UserApcRoutine,
                        UserMode,
                        UserApcContext);

      KeInsertQueueApc( &Irp->Tail.Apc,
                        Irp->UserIosb,
                        NULL,
                        2);

      //NOTE: kernel (or rundown) routine frees the IRP

      return;

   }

   IoFreeIrp(Irp);
}
