/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: irq.c,v 1.24 2002/09/08 10:23:29 chorns Exp $
 *
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
#include <roscfg.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/i386/segment.h>
#include <internal/pool.h>

#ifdef MP
#include <internal/hal/mps.h>
#endif /* MP */

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#ifdef MP

/* 
 * FIXME: This does not work if we have more than 24 IRQs (ie. more than one 
 * I/O APIC) 
 */
#define VECTOR2IRQ(vector) (((vector) - 0x31) / 8)
#define VECTOR2IRQL(vector) (4 + VECTOR2IRQ(vector))

#define IRQ_BASE  FIRST_DEVICE_VECTOR
#define NR_IRQS   0x100 - 0x30

#define __STR(x) #x
#define STR(x) __STR(x)

#define INT_NAME(intnum) _KiUnexpectedInterrupt##intnum
#define INT_NAME2(intnum) KiUnexpectedInterrupt##intnum

#define BUILD_COMMON_INTERRUPT_HANDLER() \
__asm__( \
  "_KiCommonInterrupt:\n\t" \
  "cld\n\t" \
  "pushl %ds\n\t" \
  "pushl %es\n\t" \
  "pushl %fs\n\t" \
  "pushl %gs\n\t" \
  "movl	$0xceafbeef,%eax\n\t" \
  "pushl %eax\n\t" \
  "movl	$" STR(KERNEL_DS) ",%eax\n\t" \
  "movl	%eax,%ds\n\t" \
  "movl	%eax,%es\n\t" \
  "movl	$" STR(PCR_SELECTOR) ",%eax\n\t" \
  "movl	%eax,%fs\n\t" \
  "pushl %esp\n\t" \
  "pushl %ebx\n\t" \
  "call	_KiInterruptDispatch\n\t" \
  "popl	%eax\n\t" \
  "popl	%eax\n\t" \
  "popl	%eax\n\t" \
  "popl	%gs\n\t" \
  "popl	%fs\n\t" \
  "popl	%es\n\t" \
  "popl	%ds\n\t" \
  "popa\n\t" \
  "iret\n\t");

#define BUILD_INTERRUPT_HANDLER(intnum) \
VOID INT_NAME2(intnum)(VOID); \
__asm__( \
  STR(INT_NAME(intnum)) ":\n\t" \
  "pusha\n\t" \
  "movl $0x" STR(intnum) ",%ebx\n\t" \
  "jmp _KiCommonInterrupt");


/* Interrupt handlers and declarations */

#define B(x,y) \
  BUILD_INTERRUPT_HANDLER(x##y)

#define B16(x) \
  B(x,0) B(x,1) B(x,2) B(x,3) \
  B(x,4) B(x,5) B(x,6) B(x,7) \
  B(x,8) B(x,9) B(x,A) B(x,B) \
  B(x,C) B(x,D) B(x,E) B(x,F)


BUILD_COMMON_INTERRUPT_HANDLER()
B16(3) B16(4) B16(5) B16(6)
B16(7) B16(8) B16(9) B16(A)
B16(B) B16(C) B16(D) B16(E)
B16(F)

#undef B
#undef B16


/* Interrupt handler list */

#define L(x,y) \
  (ULONG)& INT_NAME2(x##y)

#define L16(x) \
	L(x,0), L(x,1), L(x,2), L(x,3), \
	L(x,4), L(x,5), L(x,6), L(x,7), \
	L(x,8), L(x,9), L(x,A), L(x,B), \
	L(x,C), L(x,D), L(x,E), L(x,F)

static ULONG irq_handler[NR_IRQS] = {
  L16(3), L16(4), L16(5), L16(6),
  L16(7), L16(8), L16(9), L16(A),
  L16(B), L16(C), L16(D), L16(E),
  L16(F)
};

#undef L
#undef L16

#else /* MP */

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

#endif /* MP */

/*
 * PURPOSE: Object describing each isr 
 * NOTE: The data in this table is only modified at passsive level but can
 * be accessed at any irq level.
 */

static LIST_ENTRY isr_table[NR_IRQS]={{NULL,NULL},};
static PKSPIN_LOCK isr_lock[NR_IRQS] = {NULL,};
static KSPIN_LOCK isr_table_lock = {0,};

#define TAG_ISR_LOCK     TAG('I', 'S', 'R', 'L')
#define TAG_KINTERRUPT   TAG('K', 'I', 'S', 'R')

/* FUNCTIONS ****************************************************************/

#define PRESENT (0x8000)
#define I486_INTERRUPT_GATE (0xe00)

VOID KeInitInterrupts (VOID)
{
   int i;

#ifdef MP

   /*
    * Setup the IDT entries to point to the interrupt handlers
    */
   for (i=0;i<NR_IRQS;i++)
     {
	KiIdt[0x30+i].a=(irq_handler[i]&0xffff)+(KERNEL_CS<<16);
	KiIdt[0x30+i].b=(irq_handler[i]&0xffff0000)+PRESENT+
	                    I486_INTERRUPT_GATE;
	InitializeListHead(&isr_table[i]);
     }

#else

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

#endif

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

#ifdef DBG

VOID
KeIRQTrapFrameToTrapFrame(PKIRQ_TRAPFRAME IrqTrapFrame,
  PKTRAP_FRAME TrapFrame)
{
   TrapFrame->Fs     = IrqTrapFrame->Fs;
   TrapFrame->Fs     = IrqTrapFrame->Es;
   TrapFrame->Ds     = IrqTrapFrame->Ds;
   TrapFrame->Eax    = IrqTrapFrame->Eax;
   TrapFrame->Ecx    = IrqTrapFrame->Ecx;
   TrapFrame->Edx    = IrqTrapFrame->Edx;
   TrapFrame->Ebx    = IrqTrapFrame->Ebx;
   TrapFrame->Esp    = IrqTrapFrame->Esp;
   TrapFrame->Ebp    = IrqTrapFrame->Ebp;
   TrapFrame->Esi    = IrqTrapFrame->Esi;
   TrapFrame->Edi    = IrqTrapFrame->Edi;
   TrapFrame->Eip    = IrqTrapFrame->Eip;
   TrapFrame->Cs     = IrqTrapFrame->Cs;
   TrapFrame->Eflags = IrqTrapFrame->Eflags;
}

#endif

#ifdef MP

VOID
KiInterruptDispatch (ULONG Vector, PKIRQ_TRAPFRAME Trapframe)
/*
 * FUNCTION: Calls the irq specific handler for an irq
 * ARGUMENTS:
 *         Vector    = Interrupt vector
 *         Trapframe = CPU context
 */
{
   KIRQL old_level;
   PKINTERRUPT isr;
   PLIST_ENTRY current;
   ULONG irq;

#ifdef DBG

   KTRAP_FRAME KernelTrapFrame;

   KeIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
   KeGetCurrentThread()->TrapFrame = &KernelTrapFrame;

#endif /* DBG */

   DPRINT("I(%d) ", Vector);

   /*
    * Notify the rest of the kernel of the raised irq level
    */
   HalBeginSystemInterrupt (Vector,
			    VECTOR2IRQL(Vector),
			    &old_level);

   irq = VECTOR2IRQ(Vector);

   /*
    * Enable interrupts
    * NOTE: Only higher priority interrupts will get through
    */
   __asm__("sti\n\t");

   if (irq == 0)
     {
       if (KeGetCurrentProcessorNumber() == 0)
         {
       KiUpdateSystemTime(old_level, Trapframe->Eip);
         }
     }
   else
     {
      DPRINT("KiInterruptDispatch(Vector %d)\n", Vector);
      /*
       * Iterate the list until one of the isr tells us its device interrupted
       */
      current = isr_table[irq].Flink;
      isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
      //DPRINT("current %x isr %x\n",current,isr);
      while (current!=(&isr_table[irq]) && 
	     !isr->ServiceRoutine(isr,isr->ServiceContext))
	{
	   current = current->Flink;
	   isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
	   //DPRINT("current %x isr %x\n",current,isr);
	}
    }
   /*
    * Disable interrupts
    */
   __asm__("cli\n\t");

   /*
    * Unmask the related irq
    */
   HalEnableSystemInterrupt (Vector, 0, 0);

   /*
    * If the processor level will drop below dispatch level on return then
    * issue a DPC queue drain interrupt
    */

   __asm__("sti\n\t");

   if (old_level < DISPATCH_LEVEL)
     {

  HalEndSystemInterrupt (DISPATCH_LEVEL, 0);

 	if (KeGetCurrentThread() != NULL)
	  {
	     KeGetCurrentThread()->LastEip = Trapframe->Eip;
	  }
	KiDispatchInterrupt();
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

#else /* MP */

VOID STDCALL
KiInterruptDispatch2 (ULONG Irq, KIRQL old_level)
/*
 * FUNCTION: Calls all the interrupt handlers for a given irq.
 * ARGUMENTS:
 *        Irq - The number of the irq to call handlers for.
 *        old_level - The irql of the processor when the irq took place.
 * NOTES: Must be called at DIRQL.
 */
{
  PKINTERRUPT isr;
  PLIST_ENTRY current;

  if (Irq == 0)
    {
      KiUpdateSystemTime(old_level, 0);
    }
  else
    {
      /*
       * Iterate the list until one of the isr tells us its device interrupted
       */
      current = isr_table[Irq].Flink;
      isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
      while (current != &isr_table[Irq] && 
	     !isr->ServiceRoutine(isr, isr->ServiceContext))
	{
	  current = current->Flink;
	  isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
	}
   }
}

VOID 
KiInterruptDispatch (ULONG irq, PKIRQ_TRAPFRAME Trapframe)
/*
 * FUNCTION: Calls the irq specific handler for an irq
 * ARGUMENTS:
 *         irq = IRQ that has interrupted
 */
{
   KIRQL old_level;

   /*
    * At this point we have interrupts disabled, nothing has been done to
    * the PIC.
    */

   /*
    * Notify the rest of the kernel of the raised irq level. For the
    * default HAL this will send an EOI to the PIC and alter the IRQL.
    */
   if (!HalBeginSystemInterrupt (irq + IRQ_BASE,
				 PROFILE_LEVEL - irq,
				 &old_level))
     {
       return;
     }

   /*
    * Enable interrupts
    * NOTE: Only higher priority interrupts will get through
    */
   __asm__("sti\n\t");

   /*
    * Actually call the ISR.
    */
   KiInterruptDispatch2(irq, old_level);

   /*
    * End the system interrupt.
    */
   HalEndSystemInterrupt (old_level, 0);

   /*
    * Maybe do a reschedule as well.
    */
   if (old_level < DISPATCH_LEVEL && irq == 0)
     {
       PsDispatchThread(THREAD_STATE_READY);
     }
}

#endif /* MP */

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
	current = CONTAINING_RECORD(current_entry,KINTERRUPT,Entry);
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
	isr_lock[Vector] =
	  ExAllocatePoolWithTag(NonPagedPool, sizeof(KSPIN_LOCK),
				TAG_ISR_LOCK);
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
