/* $Id: irp.c,v 1.47 2003/01/25 16:16:54 hbirr Exp $
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
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_IRP     TAG('I', 'R', 'P', ' ')


/* FUNCTIONS ****************************************************************/


VOID STDCALL
IoFreeIrp(PIRP Irp)
/*
 * FUNCTION: Releases a caller allocated irp
 * ARGUMENTS:
 *      Irp = Irp to free
 */
{
  ExFreePool(Irp);
}


PIRP STDCALL
IoMakeAssociatedIrp(PIRP Irp,
		    CCHAR StackSize)
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
  return NULL;
}


VOID STDCALL
IoInitializeIrp(PIRP Irp,
		USHORT PacketSize,
		CCHAR StackSize)
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
  Irp->Tail.Overlay.CurrentStackLocation = &Irp->Stack[(ULONG)StackSize];
}


NTSTATUS FASTCALL
IofCallDriver(PDEVICE_OBJECT DeviceObject,
	      PIRP Irp)
/*
  * FUNCTION: Sends an IRP to the next lower driver
 */
{
  PDRIVER_OBJECT DriverObject;
  PIO_STACK_LOCATION Param;
  
  DPRINT("IofCallDriver(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);
  
  assert(Irp);
  assert(DeviceObject);

  DriverObject = DeviceObject->DriverObject;

  assert(DriverObject);

  IoSetNextIrpStackLocation(Irp);
  Param = IoGetCurrentIrpStackLocation(Irp);

  DPRINT("IrpSp 0x%X\n", Param);
  
  Param->DeviceObject = DeviceObject;

  DPRINT("MajorFunction %d\n", Param->MajorFunction);
  DPRINT("DriverObject->MajorFunction[Param->MajorFunction] %x\n",
    DriverObject->MajorFunction[Param->MajorFunction]);
  
  return DriverObject->MajorFunction[Param->MajorFunction](DeviceObject, Irp);
}


NTSTATUS
STDCALL
IoCallDriver (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  return(IofCallDriver(DeviceObject,
		       Irp));
}


PIRP STDCALL
IoAllocateIrp(CCHAR StackSize,
	      BOOLEAN ChargeQuota)
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
//      Irp = ExAllocatePoolWithQuota(NonPagedPool,IoSizeOfIrp(StackSize));
      Irp = ExAllocatePoolWithTag(NonPagedPool,
				  IoSizeOfIrp(StackSize),
				  TAG_IRP);
    }
  else
    {
      Irp = ExAllocatePoolWithTag(NonPagedPool,
				  IoSizeOfIrp(StackSize),
				  TAG_IRP);
    }

  if (Irp==NULL)
    {
      return(NULL);
    }

  IoInitializeIrp(Irp,
		  IoSizeOfIrp(StackSize),
		  StackSize);

//  DPRINT("Irp %x Irp->StackPtr %d\n", Irp, Irp->CurrentLocation);

  return(Irp);
}


VOID STDCALL
IopCompleteRequest(struct _KAPC* Apc,
		   PKNORMAL_ROUTINE* NormalRoutine,
		   PVOID* NormalContext,
		   PVOID* SystemArgument1,
		   PVOID* SystemArgument2)
{
  DPRINT("IopCompleteRequest(Apc %x, SystemArgument1 %x, (*SystemArgument1) %x\n",
	 Apc,
	 SystemArgument1,
	 *SystemArgument1);
  IoSecondStageCompletion((PIRP)(*SystemArgument1),
			  (KPRIORITY)(*SystemArgument2));
}


VOID FASTCALL
IofCompleteRequest(PIRP Irp,
		   CCHAR PriorityBoost)
/*
 * FUNCTION: Indicates the caller has finished all processing for a given
 * I/O request and is returning the given IRP to the I/O manager
 * ARGUMENTS:
 *         Irp = Irp to be cancelled
 *         PriorityBoost = Increment by which to boost the priority of the
 *                         thread making the request
 */
{
   ULONG             i;
   NTSTATUS          Status;
   PDEVICE_OBJECT    DeviceObject;

   DPRINT("IoCompleteRequest(Irp %x, PriorityBoost %d) Event %x THread %x\n",
      Irp,PriorityBoost, Irp->UserEvent, PsGetCurrentThread());

   assert(Irp->CancelRoutine == NULL);
   assert(Irp->IoStatus.Status != STATUS_PENDING);

   if (IoGetCurrentIrpStackLocation(Irp)->Control & SL_PENDING_RETURNED)
   {
      Irp->PendingReturned = TRUE;
   }

   for (i=Irp->CurrentLocation;i<(ULONG)Irp->StackCount;i++)
   {
      /*
      Completion routines expect the current irp stack location to be the same as when
      IoSetCompletionRoutine was called to set them. A side effect is that completion
      routines set by highest level drivers without their own stack location will receive
      an invalid current stack location (at least it should be considered as invalid).
      Since the DeviceObject argument passed is taken from the current stack, this value
      is also invalid (NULL).
      */
      if (Irp->CurrentLocation < Irp->StackCount - 1)
      {
         IoSetPreviousIrpStackLocation(Irp);
         DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;
      }
      else
      {
         DeviceObject = NULL;
      }

      if (Irp->Stack[i].CompletionRoutine != NULL &&
         ((NT_SUCCESS(Irp->IoStatus.Status) && (Irp->Stack[i].Control & SL_INVOKE_ON_SUCCESS)) ||
         (!NT_SUCCESS(Irp->IoStatus.Status) && (Irp->Stack[i].Control & SL_INVOKE_ON_ERROR)) ||
         (Irp->Cancel && (Irp->Stack[i].Control & SL_INVOKE_ON_CANCEL))))
      {
         Status = Irp->Stack[i].CompletionRoutine(DeviceObject,
                                                  Irp,
                                                  Irp->Stack[i].CompletionContext);

         if (Status == STATUS_MORE_PROCESSING_REQUIRED)
         {
            return;
         }
      }
   
      if (IoGetCurrentIrpStackLocation(Irp)->Control & SL_PENDING_RETURNED)
      {
         Irp->PendingReturned = TRUE;
      }
   }

   if (Irp->PendingReturned)
     {
	DPRINT("Dispatching APC\n");
	KeInitializeApc(&Irp->Tail.Apc,
			&Irp->Tail.Overlay.Thread->Tcb,
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


VOID STDCALL
IoCompleteRequest(PIRP Irp,
		  CCHAR PriorityBoost)
{
  IofCompleteRequest(Irp,
		     PriorityBoost);
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
BOOLEAN STDCALL
IoIsOperationSynchronous(IN PIRP Irp)
{
  PFILE_OBJECT FileObject = NULL;
  ULONG Flags = 0;

  /* Check the FILE_OBJECT's flags first. */
  FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
  if (!(FO_SYNCHRONOUS_IO & FileObject->Flags))
    {
      /* Check IRP's flags. */
      Flags = Irp->Flags;
      if (!((IRP_SYNCHRONOUS_API | IRP_SYNCHRONOUS_PAGING_IO) & Flags))
	{
	  return(FALSE);
	}
    }

  /* Check more IRP's flags. */
  Flags = Irp->Flags;
  if (!(IRP_PAGING_IO & Flags)
      || (IRP_SYNCHRONOUS_PAGING_IO & Flags))
    {
      return(TRUE);
    }

  /* Otherwise, it is an asynchronous operation. */
  return(FALSE);
}


VOID STDCALL
IoEnqueueIrp(IN PIRP Irp)
{
  UNIMPLEMENTED;
}


VOID STDCALL
IoSetTopLevelIrp(IN PIRP Irp)
{
  PETHREAD Thread;

  Thread = PsGetCurrentThread();
  Thread->TopLevelIrp->TopLevelIrp = Irp;
}


PIRP STDCALL
IoGetTopLevelIrp(VOID)
{
  return(PsGetCurrentThread()->TopLevelIrp->TopLevelIrp);
}


VOID STDCALL
IoQueueThreadIrp(IN PIRP Irp)
{
  UNIMPLEMENTED;
}

/*
NTSTATUS
STDCALL
IoSetDeviceInterfaceState(IN PUNICODE_STRING SymbolicLinkName, IN BOOLEAN Enable)
{
  UNIMPLEMENTED;
	return 0;
}

NTSTATUS
STDCALL
IoGetDeviceProperty(
  IN PDEVICE_OBJECT DeviceObject,
  IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
  IN ULONG BufferLength,
  OUT PVOID PropertyBuffer,
  OUT PULONG ResultLength)
{
  UNIMPLEMENTED;
	return 0;
}

NTSTATUS
STDCALL
IoOpenDeviceRegistryKey(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG DevInstKeyType,
  IN ACCESS_MASK DesiredAccess,
  OUT PHANDLE DevInstRegKey)
{
  UNIMPLEMENTED;
	return 0;
}
 */
/* EOF */
