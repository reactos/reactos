/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/irq.c
 * PURPOSE:         IRQ handling
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define TAG_KINTERRUPT   TAG('K', 'I', 'S', 'R')

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
IoConnectInterrupt(PKINTERRUPT* InterruptObject,
		   PKSERVICE_ROUTINE ServiceRoutine,
		   PVOID ServiceContext,
		   PKSPIN_LOCK SpinLock,
		   ULONG Vector,
		   KIRQL Irql,
		   KIRQL SynchronizeIrql,
		   KINTERRUPT_MODE InterruptMode,
		   BOOLEAN ShareVector,
		   KAFFINITY ProcessorEnableMask,
		   BOOLEAN FloatingSave)
/*
 * FUNCTION: Registers a driver's isr to be called when its device interrupts
 * ARGUMENTS:
 *        InterruptObject (OUT) = Points to the interrupt object created on 
 *                                return
 *        ServiceRoutine = Routine to be called when the device interrupts
 *        ServiceContext = Parameter to be passed to ServiceRoutine
 *        SpinLock = Initalized spinlock that will be used to synchronize
 *                   access between the isr and other driver routines. This is
 *                   required if the isr handles more than one vector or the
 *                   driver has more than one isr
 *        Vector = Interrupt vector to allocate 
 *                 (returned from HalGetInterruptVector)
 *        Irql = DIRQL returned from HalGetInterruptVector
 *        SynchronizeIrql = DIRQL at which the isr will execute. This must
 *                          be the highest of all the DIRQLs returned from
 *                          HalGetInterruptVector if the driver has multiple
 *                          isrs
 *        InterruptMode = Specifies if the interrupt is LevelSensitive or
 *                        Latched
 *        ShareVector = Specifies if the vector can be shared
 *        ProcessorEnableMask = Processors on the isr can run
 *        FloatingSave = TRUE if the floating point stack should be saved when
 *                       the isr runs. Must be false for x86 drivers
 * RETURNS: Status
 * IRQL: PASSIVE_LEVEL
 */
{
   PKINTERRUPT Interrupt;
   NTSTATUS Status = STATUS_SUCCESS;
   
   ASSERT_IRQL(PASSIVE_LEVEL);
   
   DPRINT("IoConnectInterrupt(Vector %x)\n",Vector);
   
   /*
    * Initialize interrupt object
    */
   Interrupt=ExAllocatePoolWithTag(NonPagedPool,sizeof(KINTERRUPT),
				   TAG_KINTERRUPT);
   if (Interrupt==NULL)
     {
	return(STATUS_INSUFFICIENT_RESOURCES);
     }

   Status = KeInitializeInterrupt(Interrupt,
				  ServiceRoutine,
				  ServiceContext,
				  SpinLock,
				  Vector,
				  Irql,
				  SynchronizeIrql,
				  InterruptMode,
				  ShareVector,
				  ProcessorEnableMask,
				  FloatingSave);
   if (!NT_SUCCESS(Status))
     {
	ExFreePool(Interrupt);
	return Status;
     }

   if (!KeConnectInterrupt(Interrupt))
     {
	ExFreePool(Interrupt);
	return STATUS_INVALID_PARAMETER;
     }

   *InterruptObject = Interrupt;

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
IoDisconnectInterrupt(PKINTERRUPT InterruptObject)
/*
 * FUNCTION: Releases a drivers isr
 * ARGUMENTS:
 *        InterruptObject = isr to release
 */
{
  KeDisconnectInterrupt(InterruptObject);
  ExFreePool(InterruptObject);
}

/* EOF */
