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
/* $Id: irq.c,v 1.55 2004/11/10 02:51:00 ion Exp $
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

#include <ntoskrnl.h>
#ifdef KDBG
#include <../dbg/kdb.h>
#endif /* KDBG */

#include <../hal/halx86/include/halirq.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#ifdef MP

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
  "movl %eax,%gs\n\t" \
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

static ULONG irq_handler[ROUND_UP(NR_IRQS, 16)] = {
  L16(3), L16(4), L16(5), L16(6),
  L16(7), L16(8), L16(9), L16(A),
  L16(B), L16(C), L16(D), L16(E)
};

#undef L
#undef L16

#else /* MP */

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

/* FUNCTIONS ****************************************************************/

#define PRESENT (0x8000)
#define I486_INTERRUPT_GATE (0xe00)

VOID INIT_FUNCTION
KeInitInterrupts (VOID)
{
   int i;

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

STATIC VOID 
KeIRQTrapFrameToTrapFrame(PKIRQ_TRAPFRAME IrqTrapFrame,
			  PKTRAP_FRAME TrapFrame)
{
   TrapFrame->Gs     = (USHORT)IrqTrapFrame->Gs;
   TrapFrame->Fs     = (USHORT)IrqTrapFrame->Fs;
   TrapFrame->Es     = (USHORT)IrqTrapFrame->Es;
   TrapFrame->Ds     = (USHORT)IrqTrapFrame->Ds;
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

STATIC VOID
KeTrapFrameToIRQTrapFrame(PKTRAP_FRAME TrapFrame,
			  PKIRQ_TRAPFRAME IrqTrapFrame)
{
   IrqTrapFrame->Gs     = TrapFrame->Gs;
   IrqTrapFrame->Fs     = TrapFrame->Fs;
   IrqTrapFrame->Es     = TrapFrame->Es;
   IrqTrapFrame->Ds     = TrapFrame->Ds;
   IrqTrapFrame->Eax    = TrapFrame->Eax;
   IrqTrapFrame->Ecx    = TrapFrame->Ecx;
   IrqTrapFrame->Edx    = TrapFrame->Edx;
   IrqTrapFrame->Ebx    = TrapFrame->Ebx;
   IrqTrapFrame->Esp    = TrapFrame->Esp;
   IrqTrapFrame->Ebp    = TrapFrame->Ebp;
   IrqTrapFrame->Esi    = TrapFrame->Esi;
   IrqTrapFrame->Edi    = TrapFrame->Edi;
   IrqTrapFrame->Eip    = TrapFrame->Eip;
   IrqTrapFrame->Cs     = TrapFrame->Cs;
   IrqTrapFrame->Eflags = TrapFrame->Eflags;
}

VOID STDCALL
KiInterruptDispatch2 (ULONG vector, KIRQL old_level)
/*
 * FUNCTION: Calls all the interrupt handlers for a given irq.
 * ARGUMENTS:
 *        vector - The number of the vector to call handlers for.
 *        old_level - The irql of the processor when the irq took place.
 * NOTES: Must be called at DIRQL.
 */
{
  PKINTERRUPT isr;
  PLIST_ENTRY current;

  DPRINT("I(0x%.08x, 0x%.08x)\n", vector, old_level);

  /*
   * Iterate the list until one of the isr tells us its device interrupted
   */
  current = isr_table[vector - IRQ_BASE].Flink;
  isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);

  while (current != &isr_table[vector - IRQ_BASE] && 
         !isr->ServiceRoutine(isr, isr->ServiceContext))
    {
      current = current->Flink;
      isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
    }
}

VOID 
KiInterruptDispatch (ULONG vector, PKIRQ_TRAPFRAME Trapframe)
/*
 * FUNCTION: Calls the irq specific handler for an irq
 * ARGUMENTS:
 *         irq = IRQ that has interrupted
 */
{
   KIRQL old_level;
   KTRAP_FRAME KernelTrapFrame;
   PKTHREAD CurrentThread;
   PKTRAP_FRAME OldTrapFrame=NULL;

   /*
    * At this point we have interrupts disabled, nothing has been done to
    * the PIC.
    */
    
   KeGetCurrentKPCR()->PrcbData.InterruptCount++;

   /*
    * Notify the rest of the kernel of the raised irq level. For the
    * default HAL this will send an EOI to the PIC and alter the IRQL.
    */
   if (!HalBeginSystemInterrupt (vector,
				 VECTOR2IRQL(vector),
				 &old_level))
     {
       return;
     }


   /*
    * Enable interrupts
    * NOTE: Only higher priority interrupts will get through
    */
   Ke386EnableInterrupts();

#ifndef MP
   if (VECTOR2IRQ(vector) == 0)
   {
      KeIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
      KeUpdateSystemTime(&KernelTrapFrame, old_level);
#ifdef KDBG
      KdbProfileInterrupt(Trapframe->Eip);
#endif /* KDBG */
   }
   else
#endif
   {
     /*
      * Actually call the ISR.
      */
     KiInterruptDispatch2(vector, old_level);
   }

   /*
    * End the system interrupt.
    */
   Ke386DisableInterrupts();

   HalEndSystemInterrupt (old_level, 0);

   if (old_level==PASSIVE_LEVEL && Trapframe->Cs != KERNEL_CS)
     {
       CurrentThread = KeGetCurrentThread();
       if (CurrentThread!=NULL && CurrentThread->Alerted[1])
         {
           DPRINT("PID: %d, TID: %d CS %04x/%04x\n", 
	          ((PETHREAD)CurrentThread)->ThreadsProcess->UniqueProcessId,
		  ((PETHREAD)CurrentThread)->Cid.UniqueThread,
		  Trapframe->Cs, 
		  CurrentThread->TrapFrame ? CurrentThread->TrapFrame->Cs : 0);
	   if (CurrentThread->TrapFrame == NULL)
	     {
	       OldTrapFrame = CurrentThread->TrapFrame;
	       KeIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
	       CurrentThread->TrapFrame = &KernelTrapFrame;
	     }
	   
           KiDeliverApc(KernelMode, NULL, NULL);
           
	   ASSERT(KeGetCurrentThread() == CurrentThread);
           if (CurrentThread->TrapFrame == &KernelTrapFrame)
	     {
               KeTrapFrameToIRQTrapFrame(&KernelTrapFrame, Trapframe);
	       CurrentThread->TrapFrame = OldTrapFrame;
	     }
	 }
     }
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

/*
 * @implemented
 */
BOOLEAN STDCALL
KeConnectInterrupt(PKINTERRUPT InterruptObject)
{
   KIRQL oldlvl;
   KIRQL synch_oldlvl;
   PKINTERRUPT ListHead;
   ULONG Vector;

   DPRINT("KeConnectInterrupt()\n");

   Vector = InterruptObject->Vector;

   if (Vector < IRQ_BASE && Vector >= IRQ_BASE + NR_IRQS)
      return FALSE;

   Vector -= IRQ_BASE;

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
	return FALSE;
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
   KiAcquireSpinLock(InterruptObject->IrqLock);
   DPRINT("%x %x\n",isr_table[Vector].Flink,isr_table[Vector].Blink);
   if (IsListEmpty(&isr_table[Vector]))
   {
      HalEnableSystemInterrupt(Vector + IRQ_BASE, 0, 0);
   }
   InsertTailList(&isr_table[Vector],&InterruptObject->Entry);
   DPRINT("%x %x\n",InterruptObject->Entry.Flink,
          InterruptObject->Entry.Blink);
   KiReleaseSpinLock(InterruptObject->IrqLock);
   KeLowerIrql(synch_oldlvl);
   
   /*
    * Release the table spinlock
    */
   KeReleaseSpinLock(&isr_table_lock,oldlvl);
   
   KeDumpIrqList();

   return TRUE;
}


/*
 * @implemented
 */
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
   KiAcquireSpinLock(InterruptObject->IrqLock);
   RemoveEntryList(&InterruptObject->Entry);
   if (IsListEmpty(&isr_table[InterruptObject->Vector - IRQ_BASE]))
   {
      HalDisableSystemInterrupt(InterruptObject->Vector, 0);
   }
   KiReleaseSpinLock(InterruptObject->IrqLock);
   KeLowerIrql(oldlvl);
}


/*
 * @implemented
 */
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
   InterruptObject->FloatingSave = FloatingSave;

   return STATUS_SUCCESS;
}

/* EOF */
