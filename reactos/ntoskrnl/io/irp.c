/* $Id: irp.c,v 1.33 2000/12/23 02:37:39 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/irp.c
 * PURPOSE:         Handle IRPs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 *                  24/05/98: Created 
 */

/* NOTES *******************************************************************
 * 
 * Layout of an IRP 
 * 
 *             ################
 *             #    Headers   #
 *             ################
 *             #              #
 *             #   Variable   #
 *             # length list  #
 *             # of io stack  #
 *             #  locations   #
 *             #              #
 *             ################
 * 
 * 
 * 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/


VOID STDCALL
IoFreeIrp (PIRP Irp)
/*
 * FUNCTION: Releases a caller allocated irp
 * ARGUMENTS:
 *      Irp = Irp to free
 */
{
   ExFreePool(Irp);
}


PIRP STDCALL
IoMakeAssociatedIrp (PIRP Irp, CCHAR StackSize)
/*
 * FUNCTION: Allocates and initializes an irp to associated with a master irp
 * ARGUMENTS:
 *       Irp = Master irp
 *       StackSize = Number of stack locations to be allocated in the irp
 * RETURNS: The irp allocated
 */
{
   PIRP AssocIrp;
   
   AssocIrp = IoAllocateIrp(StackSize,FALSE);
   UNIMPLEMENTED;
}


VOID STDCALL
IoInitializeIrp (PIRP Irp, USHORT PacketSize, CCHAR StackSize)
/*
 * FUNCTION: Initalizes an irp allocated by the caller
 * ARGUMENTS:
 *          Irp = IRP to initalize
 *          PacketSize = Size in bytes of the IRP
 *          StackSize = Number of stack locations in the IRP
 */
{
   assert(Irp != NULL);

   memset(Irp, 0, PacketSize);
   Irp->Size = PacketSize;
   Irp->StackCount = StackSize;
   Irp->CurrentLocation = StackSize;
   Irp->Tail.Overlay.CurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
}


NTSTATUS FASTCALL
IofCallDriver (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
  * FUNCTION: Sends an IRP to the next lower driver
 */
{
   NTSTATUS Status;
   PDRIVER_OBJECT DriverObject;
   PIO_STACK_LOCATION Param;
   
//   DPRINT("IoCallDriver(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
   
   DriverObject = DeviceObject->DriverObject;
   Param = IoGetNextIrpStackLocation(Irp);
   
   Irp->Tail.Overlay.CurrentStackLocation--;
   Irp->CurrentLocation--;
   
//   DPRINT("MajorFunction %d\n", Param->MajorFunction);
//   DPRINT("DriverObject->MajorFunction[Param->MajorFunction] %x\n",
//	    DriverObject->MajorFunction[Param->MajorFunction]);
   Status = DriverObject->MajorFunction[Param->MajorFunction](DeviceObject,
							      Irp);
   return Status;
}


NTSTATUS
STDCALL
IoCallDriver (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	return IofCallDriver (
			DeviceObject,
			Irp
			);
}


PIRP
STDCALL
IoAllocateIrp (CCHAR StackSize, BOOLEAN ChargeQuota)
/*
 * FUNCTION: Allocates an IRP
 * ARGUMENTS:
 *          StackSize = the size of the stack required for the irp
 *          ChargeQuota = Charge allocation to current threads quota
 * RETURNS: Irp allocated
 */
{
   PIRP Irp;

#if 0
   DbgPrint("IoAllocateIrp(StackSize %d ChargeQuota %d)\n",
          StackSize,
	  ChargeQuota);
   KeDumpStackFrames(0,8);
#endif
   
   if (ChargeQuota)
     {
//	Irp = ExAllocatePoolWithQuota(NonPagedPool,IoSizeOfIrp(StackSize));
	Irp = ExAllocatePool(NonPagedPool,IoSizeOfIrp(StackSize));
     }
   else
     {	
	Irp = ExAllocatePool(NonPagedPool,IoSizeOfIrp(StackSize));
     }
      
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   IoInitializeIrp(Irp, IoSizeOfIrp(StackSize), StackSize);

//   DPRINT("Irp %x Irp->StackPtr %d\n", Irp, Irp->CurrentLocation);

   return Irp;
}


VOID IopCompleteRequest(struct _KAPC* Apc,
			PKNORMAL_ROUTINE* NormalRoutine,
			PVOID* NormalContext,
			PVOID* SystemArgument1,
			PVOID* SystemArgument2)
{
   DPRINT("IopCompleteRequest(Apc %x, SystemArgument1 %x, "
	  "(*SystemArgument1) %x\n", Apc, SystemArgument1,
	  *SystemArgument1);
   IoSecondStageCompletion((PIRP)(*SystemArgument1),
                           (KPRIORITY)(*SystemArgument2));
}


VOID
FASTCALL
IofCompleteRequest (PIRP Irp, CCHAR PriorityBoost)
/*
 * FUNCTION: Indicates the caller has finished all processing for a given
 * I/O request and is returning the given IRP to the I/O manager
 * ARGUMENTS:
 *         Irp = Irp to be cancelled
 *         PriorityBoost = Increment by which to boost the priority of the
 *                         thread making the request
 */
{
   unsigned int i;
   NTSTATUS Status;
   PKTHREAD Thread;
   
   DPRINT("IoCompleteRequest(Irp %x, PriorityBoost %d) Event %x THread %x\n",
	   Irp,PriorityBoost, Irp->UserEvent, PsGetCurrentThread());

   for (i=0;i<Irp->StackCount;i++)
     {
	if (Irp->Stack[i].CompletionRoutine != NULL)
	  {
	     Status = Irp->Stack[i].CompletionRoutine(
					     Irp->Stack[i].DeviceObject,
					     Irp,
					     Irp->Stack[i].CompletionContext);
	     if (Status == STATUS_MORE_PROCESSING_REQUIRED)
	       {
		  return;
	       }
	  }
	if (Irp->Stack[i].Control & SL_PENDING_RETURNED)
	  {
	     Irp->PendingReturned = TRUE;
	  }
     }

   if (Irp->PendingReturned)
     {
	DPRINT("Dispatching APC\n");
	Thread = &Irp->Tail.Overlay.Thread->Tcb;
	KeInitializeApc(&Irp->Tail.Apc,
			Thread,
			0,
			IopCompleteRequest,
			NULL,
			(PKNORMAL_ROUTINE)
			NULL,
			KernelMode,
			NULL);
	KeInsertQueueApc(&Irp->Tail.Apc,
			 (PVOID)Irp,
			 (PVOID)(ULONG)PriorityBoost,
			 KernelMode);
	DPRINT("Finished dispatching APC\n");
     }
   else
     {
	DPRINT("Calling completion routine directly\n");
	IoSecondStageCompletion(Irp,PriorityBoost);
	DPRINT("Finished completition routine\n");
     }
}


VOID
STDCALL
IoCompleteRequest (PIRP Irp, CCHAR PriorityBoost)
{
	IofCompleteRequest (
		Irp,
		PriorityBoost
		);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoIsOperationSynchronous@4
 *
 * DESCRIPTION
 *	Check if the I/O operation associated with the given IRP
 *	is synchronous.
 *
 * ARGUMENTS
 * 	Irp 	Packet to check.
 *
 * RETURN VALUE
 * 	TRUE if Irp's operation is synchronous; otherwise FALSE.
 */
BOOLEAN
STDCALL
IoIsOperationSynchronous (
	IN	PIRP	Irp
	)
{
	ULONG		Flags = 0;
	PFILE_OBJECT	FileObject = NULL;
	
	/*
	 * Check the associated FILE_OBJECT's
	 * flags first.
	 */
	FileObject = Irp->Tail.Overlay.OriginalFileObject;
	if (!(FO_SYNCHRONOUS_IO & FileObject->Flags))
	{
		/* Check IRP's flags. */
		Flags = Irp->Flags;
		if (!(	(IRP_SYNCHRONOUS_API | IRP_SYNCHRONOUS_PAGING_IO)
			& Flags
			))
		{
			return FALSE;
		}
	}
	/*
	 * Check more IRP's flags.
	 */
	Flags = Irp->Flags;
	if (	!(IRP_MOUNT_COMPLETION & Flags)
		|| (IRP_SYNCHRONOUS_PAGING_IO & Flags)
		)
	{
		return TRUE;
	}
	/*
	 * Otherwise, it is an
	 * asynchronous operation.
	 */
	return FALSE;
}


VOID
STDCALL
IoEnqueueIrp (
	PIRP	Irp
	)
{
	UNIMPLEMENTED;
}


VOID
STDCALL
IoSetTopLevelIrp (
	IN	PIRP	Irp
	)
{
	PETHREAD Thread;

	Thread = PsGetCurrentThread ();
	Thread->TopLevelIrp->TopLevelIrp = Irp;
}


PIRP
STDCALL
IoGetTopLevelIrp (
	VOID
	)
{
	return (PsGetCurrentThread ()->TopLevelIrp->TopLevelIrp);
}


VOID
STDCALL
IoQueueThreadIrp (
	PVOID	Unknown0
	)
{
	UNIMPLEMENTED;
}

/* EOF */
