/*
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

#include <internal/string.h>
#include <ddk/ntddk.h>

//#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

PDEVICE_OBJECT IoGetDeviceToVerify(PETHREAD Thread)
/*
 * FUNCTION: Returns a pointer to the device, representing a removable-media
 * device, that is the target of the given thread's I/O request
 */
{
   UNIMPLEMENTED;
}

VOID IoFreeIrp(PIRP Irp)
/*
 * FUNCTION: Releases a caller allocated irp
 * ARGUMENTS:
 *      Irp = Irp to free
 */
{
   ExFreePool(Irp);
}

PIRP IoMakeAssociatedIrp(PIRP Irp, CCHAR StackSize)
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

VOID IoMarkIrpPending(PIRP Irp)
/*
 * FUNCTION: Marks the specified irp, indicating further processing will
 * be required by other driver routines
 * ARGUMENTS:
 *      Irp = Irp to mark
 */
{
   IoGetCurrentIrpStackLocation(Irp)->Control |= SL_PENDING_RETURNED;
}

USHORT IoSizeOfIrp(CCHAR StackSize)
/*
 * FUNCTION:  Determines the size of an IRP
 * ARGUMENTS: 
 *           StackSize = number of stack locations in the IRP
 * RETURNS: The size of the IRP in bytes 
 */
{
   return(sizeof(IRP)+((StackSize-1)*sizeof(IO_STACK_LOCATION)));
}

VOID IoInitializeIrp(PIRP Irp, USHORT PacketSize, CCHAR StackSize)
/*
 * FUNCTION: Initalizes an irp allocated by the caller
 * ARGUMENTS:
 *          Irp = IRP to initalize
 *          PacketSize = Size in bytes of the IRP
 *          StackSize = Number of stack locations in the IRP
 */
{
   assert(Irp!=NULL);
   memset(Irp,0,PacketSize);
   Irp->StackCount=StackSize;
   Irp->CurrentLocation=StackSize;
   Irp->Tail.Overlay.CurrentStackLocation=IoGetCurrentIrpStackLocation(Irp);
}

PIO_STACK_LOCATION IoGetCurrentIrpStackLocation(PIRP Irp)
/*
 * FUNCTION: Gets a pointer to the callers location in the I/O stack in
 * the given IRP
 * ARGUMENTS:
 *         Irp = Points to the IRP
 * RETURNS: A pointer to the stack location
 */
{
   return(&Irp->Stack[Irp->CurrentLocation]);
}


VOID IoSetNextIrpStackLocation(PIRP Irp)
{
   Irp->CurrentLocation--;
   Irp->Tail.Overlay.CurrentStackLocation--;
}

PIO_STACK_LOCATION IoGetNextIrpStackLocation(PIRP Irp)
/*
 * FUNCTION: Gives a higher level driver access to the next lower driver's 
 * I/O stack location
 * ARGUMENTS: 
 *           Irp = points to the irp
 * RETURNS: A pointer to the stack location 
 */
{
   assert(Irp!=NULL);
   DPRINT("Irp %x Irp->StackPtr %x\n",Irp,Irp->CurrentLocation);
   return(&Irp->Stack[Irp->CurrentLocation-1]);
}

NTSTATUS IoCallDriver(PDEVICE_OBJECT DevObject, PIRP irp)
/*
 * FUNCTION: Sends an IRP to the next lower driver
 */
{
   PDRIVER_OBJECT drv = DevObject->DriverObject;
   IO_STACK_LOCATION* param = IoGetNextIrpStackLocation(irp);
   DPRINT("Deviceobject %x\n",DevObject);
   DPRINT("Irp %x\n",irp);
   irp->Tail.Overlay.CurrentStackLocation--;
   irp->CurrentLocation--;
   DPRINT("Io stack address %x\n",param);
   DPRINT("Function %d Routine %x\n",param->MajorFunction,
	  drv->MajorFunction[param->MajorFunction]);
   DPRINT("IRP_MJ_CREATE %d\n",IRP_MJ_CREATE);

   return(drv->MajorFunction[param->MajorFunction](DevObject,irp));
}

PIRP IoAllocateIrp(CCHAR StackSize, BOOLEAN ChargeQuota)
/*
 * FUNCTION: Allocates an IRP
 * ARGUMENTS:
 *          StackSize = the size of the stack required for the irp
 *          ChargeQuota = Charge allocation to current threads quota
 * RETURNS: Irp allocated
 */
{
   PIRP Irp;
   
   DPRINT("IoAllocateIrp(StackSize %d ChargeQuota %d)\n",StackSize,
	  ChargeQuota);
   if (ChargeQuota)
     {
	Irp = ExAllocatePoolWithQuota(NonPagedPool,IoSizeOfIrp(StackSize));
     }
   else
     {	
	Irp = ExAllocatePool(NonPagedPool,IoSizeOfIrp(StackSize));
     }
      
   if (Irp==NULL)
     {
	return(NULL);
     }
   
   Irp->StackCount=StackSize;
   Irp->CurrentLocation=StackSize;

   DPRINT("Irp %x Irp->StackPtr %d\n",Irp,Irp->CurrentLocation);
   return(Irp);
}

VOID IoSetCompletionRoutine(PIRP Irp,
			    PIO_COMPLETION_ROUTINE CompletionRoutine,
			    PVOID Context,
			    BOOLEAN InvokeOnSuccess,
			    BOOLEAN InvokeOnError,
			    BOOLEAN InvokeOnCancel)
{
   IO_STACK_LOCATION* param = IoGetNextIrpStackLocation(Irp);
   
   param->CompletionRoutine=CompletionRoutine;
   param->CompletionContext=Context;
   if (InvokeOnSuccess)
     {
	param->Control = SL_INVOKE_ON_SUCCESS;
     }
   if (InvokeOnError)
     {
	param->Control = param->Control | SL_INVOKE_ON_ERROR;
     }
   if (InvokeOnCancel)
     {
	param->Control = param->Control | SL_INVOKE_ON_CANCEL;
     }
}

VOID IoCompleteRequest(PIRP Irp, CCHAR PriorityBoost)
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
   
   DPRINT("IoCompleteRequest(Irp %x, PriorityBoost %d)\n",
	  Irp,PriorityBoost);
   for (i=0;i<Irp->StackCount;i++)
     {
	if (Irp->Stack[i].CompletionRoutine!=NULL)
	  {
	     Irp->Stack[i].CompletionRoutine(Irp->Stack[i].DeviceObject,Irp,
					     Irp->Stack[i].CompletionContext);
	  }
     }
   
   if (Irp->UserEvent!=NULL)
     {
	KeSetEvent(Irp->UserEvent,PriorityBoost,FALSE);
     }
}
