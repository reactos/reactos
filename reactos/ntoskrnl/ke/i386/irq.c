/* $Id: irq.c,v 1.5 2001/02/06 00:11:19 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/irq.c
 * PURPOSE:         IRQ handling
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *             29/05/98: Created
 */

/*
 * NOTE: In general the PIC interrupt priority facilities are used to
 * preserve the NT IRQL semantics, global interrupt disables are only used
 * to keep the PIC in a consistent state
 *
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/i386/segment.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define NR_IRQS         (16)
#define IRQ_BASE        (0x40)

 void irq_handler_0(void);
 void irq_handler_1(void);
 void irq_handler_2(void);
 void irq_handler_3(void);
 void irq_handler_4(void);
 void irq_handler_5(void);
 void irq_handler_6(void);
 void irq_handler_7(void);
 void irq_handler_8(void);
 void irq_handler_9(void);
 void irq_handler_10(void);
 void irq_handler_11(void);
 void irq_handler_12(void);
 void irq_handler_13(void);
 void irq_handler_14(void);
 void irq_handler_15(void);

static unsigned int irq_handler[NR_IRQS]=
        {
                (int)&irq_handler_0,
                (int)&irq_handler_1,
                (int)&irq_handler_2,
                (int)&irq_handler_3,
                (int)&irq_handler_4,
                (int)&irq_handler_5,
                (int)&irq_handler_6,
                (int)&irq_handler_7,
                (int)&irq_handler_8,
                (int)&irq_handler_9,
                (int)&irq_handler_10,
                (int)&irq_handler_11,
                (int)&irq_handler_12,
                (int)&irq_handler_13,
                (int)&irq_handler_14,
                (int)&irq_handler_15,
        };

/*
 * PURPOSE: Object describing each isr 
 * NOTE: The data in this table is only modified at passsive level but can
 * be accessed at any irq level.
 */

static LIST_ENTRY isr_table[NR_IRQS]={{NULL,NULL},};
static PKSPIN_LOCK isr_lock[NR_IRQS] = {NULL,};
static KSPIN_LOCK isr_table_lock = {0,};


/* FUNCTIONS ****************************************************************/

#define PRESENT (0x8000)
#define I486_INTERRUPT_GATE (0xe00)

VOID KeInitInterrupts (VOID)
{
   int i;
   
   DPRINT("KeInitInterrupts ()\n",0);
   
   /*
    * Setup the IDT entries to point to the interrupt handlers
    */
   for (i=0;i<NR_IRQS;i++)
     {
	KiIdt[IRQ_BASE+i].a=(irq_handler[i]&0xffff)+(KERNEL_CS<<16);
	KiIdt[IRQ_BASE+i].b=(irq_handler[i]&0xffff0000)+PRESENT+
	                    I486_INTERRUPT_GATE;
	InitializeListHead(&isr_table[i]);
     }
}

typedef struct _KIRQ_TRAPFRAME
{
   ULONG Magic;
   ULONG Fs;
   ULONG Es;
   ULONG Ds;
   ULONG Eax;
   ULONG Ecx;
   ULONG Edx;
   ULONG Ebx;
   ULONG Esp;
   ULONG Ebp;
   ULONG Esi;
   ULONG Edi;
   ULONG Eip;
   ULONG Cs;
   ULONG Eflags;
} KIRQ_TRAPFRAME, *PKIRQ_TRAPFRAME;

VOID 
KiInterruptDispatch (ULONG irq, PKIRQ_TRAPFRAME Trapframe)
/*
 * FUNCTION: Calls the irq specific handler for an irq
 * ARGUMENTS:
 *         irq = IRQ that has interrupted
 */
{
   KIRQL old_level;
   PKINTERRUPT isr;
   PLIST_ENTRY current;
   
//   DbgPrint("{");
   
   /*
    * Notify the rest of the kernel of the raised irq level
    */
   HalBeginSystemInterrupt (irq+IRQ_BASE,
			    HIGH_LEVEL-irq,
			    &old_level);
   
   /*
    * Enable interrupts
    * NOTE: Only higher priority interrupts will get through
    */
   __asm__("sti\n\t");

   if (irq==0)
   {
        KiUpdateSystemTime();
   }
   else
   {
      DPRINT("KiInterruptDispatch(irq %d)\n",irq);
      /*
       * Iterate the list until one of the isr tells us its device interrupted
       */
      current = isr_table[irq].Flink;
      isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
      DPRINT("current %x isr %x\n",current,isr);
      while (current!=(&isr_table[irq]) && 
	     !isr->ServiceRoutine(isr,isr->ServiceContext))
	{
	   current = current->Flink;
	   isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
	   DPRINT("current %x isr %x\n",current,isr);
	}
   }
   
   /*
    * Disable interrupts
    */
   __asm__("cli\n\t");
   
   /*
    * Unmask the related irq
    */
   HalEnableSystemInterrupt (irq + IRQ_BASE, 0, 0);
   
   /*
    * If the processor level will drop below dispatch level on return then
    * issue a DPC queue drain interrupt
    */
   if (old_level < DISPATCH_LEVEL)
     {
	HalEndSystemInterrupt (DISPATCH_LEVEL, 0);
	__asm__("sti\n\t");

	if (KeGetCurrentThread() != NULL)
	  {
	     KeGetCurrentThread()->LastEip = Trapframe->Eip;
	  }
	KiDispatchInterrupt();
	if (irq == 0)
	  {
	    PsDispatchThread(THREAD_STATE_RUNNABLE);
	  }
	if (KeGetCurrentThread() != NULL &&
	    KeGetCurrentThread()->Alerted[1] != 0 &&
	    Trapframe->Cs != KERNEL_CS)
	  {
	    HalEndSystemInterrupt (APC_LEVEL, 0);
	    KiDeliverNormalApc();
	  }
     }

   HalEndSystemInterrupt (old_level, 0);
}


static VOID 
KeDumpIrqList(VOID)
{
   PKINTERRUPT current;
   PLIST_ENTRY current_entry;
   unsigned int i;
   
   for (i=0;i<NR_IRQS;i++)
     {
	DPRINT("For irq %x ",i);
	current_entry = isr_table[i].Flink;
	current = CONTAINING_RECORD(current,KINTERRUPT,Entry);
	while (current_entry!=(&isr_table[i]))
	  {
	     DPRINT("Isr %x ",current);
	     current_entry = current_entry->Flink;
	     current = CONTAINING_RECORD(current_entry,KINTERRUPT,Entry);
	  }
	DPRINT("\n",0);
     }
}


NTSTATUS STDCALL
KeConnectInterrupt(PKINTERRUPT InterruptObject)
{
   KIRQL oldlvl;
   KIRQL synch_oldlvl;
   PKINTERRUPT ListHead;
   ULONG Vector;

   DPRINT("KeConnectInterrupt()\n");

   Vector = InterruptObject->Vector;

   /*
    * Acquire the table spinlock
    */
   KeAcquireSpinLock(&isr_table_lock,&oldlvl);
   
   /*
    * Check if the vector is already in use that we can share it
    */
   ListHead = CONTAINING_RECORD(isr_table[Vector].Flink,KINTERRUPT,Entry);
   if (!IsListEmpty(&isr_table[Vector]) &&
       (InterruptObject->Shareable == FALSE || ListHead->Shareable==FALSE))
     {
	KeReleaseSpinLock(&isr_table_lock,oldlvl);
	return(STATUS_INVALID_PARAMETER);
     }
   else
     {
	isr_lock[Vector]=ExAllocatePool(NonPagedPool,sizeof(KSPIN_LOCK));
	KeInitializeSpinLock(isr_lock[Vector]);
     }

   InterruptObject->IrqLock = isr_lock[Vector];

   KeRaiseIrql(InterruptObject->SynchLevel,&synch_oldlvl);
   KeAcquireSpinLockAtDpcLevel(InterruptObject->IrqLock);
   DPRINT("%x %x\n",isr_table[Vector].Flink,isr_table[Vector].Blink);
   InsertTailList(&isr_table[Vector],&InterruptObject->Entry);
   DPRINT("%x %x\n",InterruptObject->Entry.Flink,
          InterruptObject->Entry.Blink);
   KeReleaseSpinLockFromDpcLevel(InterruptObject->IrqLock);
   KeLowerIrql(synch_oldlvl);
   
   /*
    * Release the table spinlock
    */
   KeReleaseSpinLock(&isr_table_lock,oldlvl);
   
   KeDumpIrqList();

   return STATUS_SUCCESS;
}


VOID STDCALL
KeDisconnectInterrupt(PKINTERRUPT InterruptObject)
/*
 * FUNCTION: Releases a drivers isr
 * ARGUMENTS:
 *        InterruptObject = isr to release
 */
{
   KIRQL oldlvl;
   
   KeRaiseIrql(InterruptObject->SynchLevel,&oldlvl);
   KeAcquireSpinLockAtDpcLevel(InterruptObject->IrqLock);
   RemoveEntryList(&InterruptObject->Entry);
   KeReleaseSpinLockFromDpcLevel(InterruptObject->IrqLock);
   KeLowerIrql(oldlvl);
}


NTSTATUS
STDCALL
KeInitializeInterrupt(PKINTERRUPT InterruptObject,
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
{
   InterruptObject->ServiceContext = ServiceContext;
   InterruptObject->ServiceRoutine = ServiceRoutine;
   InterruptObject->Vector = Vector;
   InterruptObject->ProcessorEnableMask = ProcessorEnableMask;
   InterruptObject->SynchLevel = SynchronizeIrql;
   InterruptObject->Shareable = ShareVector;
   InterruptObject->FloatingSave = FALSE;

   return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
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
    * Check the parameters
    */
   if (Vector >= NR_IRQS)
     {
	return(STATUS_INVALID_PARAMETER);
     }
   if (FloatingSave == TRUE)
     {
	return(STATUS_INVALID_PARAMETER);
     }
   
   /*
    * Initialize interrupt object
    */
   Interrupt=ExAllocatePool(NonPagedPool,sizeof(KINTERRUPT));
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

   Status = KeConnectInterrupt(Interrupt);
   if (!NT_SUCCESS(Status))
     {
	ExFreePool(Interrupt);
	return Status;
     }

   *InterruptObject = Interrupt;

   return(STATUS_SUCCESS);
}


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
