/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/irq.c
 * PURPOSE:         IRQ handling
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Hartmut Birr
 */

/*
 * NOTE: In general the PIC interrupt priority facilities are used to
 * preserve the NT IRQL semantics, global interrupt disables are only used
 * to keep the PIC in a consistent state
 *
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#if defined(KDBG) || defined(DBG)
#include <../dbg/kdb.h>
#endif /* KDBG */

#include <../hal/halx86/include/halirq.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#ifdef CONFIG_SMP

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

#else /* CONFIG_SMP */

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

#endif /* CONFIG_SMP */

/*
 * PURPOSE: Object describing each isr 
 * NOTE: The data in this table is only modified at passsive level but can
 * be accessed at any irq level.
 */

typedef struct
{
   LIST_ENTRY ListHead;
   KSPIN_LOCK Lock;
   ULONG Count;
}
ISR_TABLE, *PISR_TABLE;

#ifdef CONFIG_SMP
static ISR_TABLE IsrTable[NR_IRQS][MAXIMUM_PROCESSORS];
#else
static ISR_TABLE IsrTable[NR_IRQS][1];
#endif

#define TAG_ISR_LOCK     TAG('I', 'S', 'R', 'L')

/* FUNCTIONS ****************************************************************/

#define PRESENT (0x8000)
#define I486_INTERRUPT_GATE (0xe00)

VOID INIT_FUNCTION
KeInitInterrupts (VOID)
{
   int i, j;


   /*
    * Setup the IDT entries to point to the interrupt handlers
    */
   for (i=0;i<NR_IRQS;i++)
     {
	KiIdt[IRQ_BASE+i].a=(irq_handler[i]&0xffff)+(KERNEL_CS<<16);
	KiIdt[IRQ_BASE+i].b=(irq_handler[i]&0xffff0000)+PRESENT+
	                    I486_INTERRUPT_GATE;
#ifdef CONFIG_SMP
	for (j = 0; j < MAXIMUM_PROCESSORS; j++)
#else
	j = 0;
#endif
	  {
	    InitializeListHead(&IsrTable[i][j].ListHead);
            KeInitializeSpinLock(&IsrTable[i][j].Lock);
	    IsrTable[i][j].Count = 0;
	  }
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
  KIRQL oldlvl;
  PISR_TABLE CurrentIsr;

  DPRINT("I(0x%.08x, 0x%.08x)\n", vector, old_level);

  /*
   * Iterate the list until one of the isr tells us its device interrupted
   */
  CurrentIsr = &IsrTable[vector - IRQ_BASE][(ULONG)KeGetCurrentProcessorNumber()];

  KiAcquireSpinLock(&CurrentIsr->Lock);

  CurrentIsr->Count++;
  current = CurrentIsr->ListHead.Flink;

  while (current != &CurrentIsr->ListHead)
    {
      isr = CONTAINING_RECORD(current,KINTERRUPT,Entry);
      oldlvl = KeAcquireInterruptSpinLock(isr);
      if (isr->ServiceRoutine(isr, isr->ServiceContext))
        {
          KeReleaseInterruptSpinLock(isr, oldlvl);
	  break;
	}
      KeReleaseInterruptSpinLock(isr, oldlvl);
      current = current->Flink;
    }
  KiReleaseSpinLock(&CurrentIsr->Lock);
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
    
   KeGetCurrentPrcb()->InterruptCount++;

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

#ifndef CONFIG_SMP
   if (VECTOR2IRQ(vector) == 0)
   {
      KeIRQTrapFrameToTrapFrame(Trapframe, &KernelTrapFrame);
      KeUpdateSystemTime(&KernelTrapFrame, old_level);
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
   
	   Ke386EnableInterrupts();
           KiDeliverApc(KernelMode, NULL, NULL);
           Ke386DisableInterrupts();
           
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
   ULONG i, j;
   KIRQL oldlvl;
   BOOLEAN printed;
   
   for (i=0;i<NR_IRQS;i++)
     {
	printed = FALSE;
        KeRaiseIrql(VECTOR2IRQL(i + IRQ_BASE),&oldlvl);

	for (j=0; j < KeNumberProcessors; j++)
	  {
	    KiAcquireSpinLock(&IsrTable[i][j].Lock);

	    current_entry = IsrTable[i][j].ListHead.Flink;
	    current = CONTAINING_RECORD(current_entry,KINTERRUPT,Entry);
	    while (current_entry!=&(IsrTable[i][j].ListHead))
	      {
	        if (printed == FALSE)
		  {
		    printed = TRUE;
		    DPRINT("For irq %x:\n",i);
		  }
	        DPRINT("   Isr %x\n",current);
	        current_entry = current_entry->Flink;
	        current = CONTAINING_RECORD(current_entry,KINTERRUPT,Entry);
	      }
	    KiReleaseSpinLock(&IsrTable[i][j].Lock);
	  }
        KeLowerIrql(oldlvl);
     }
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeConnectInterrupt(PKINTERRUPT InterruptObject)
{
   KIRQL oldlvl,synch_oldlvl;
   PKINTERRUPT ListHead;
   ULONG Vector;
   PISR_TABLE CurrentIsr;
   BOOLEAN Result;

   DPRINT("KeConnectInterrupt()\n");

   Vector = InterruptObject->Vector;

   if (Vector < IRQ_BASE || Vector >= IRQ_BASE + NR_IRQS)
      return FALSE;

   Vector -= IRQ_BASE;

   ASSERT (InterruptObject->ProcessorNumber < KeNumberProcessors);

   KeSetSystemAffinityThread(1 << InterruptObject->ProcessorNumber);

   CurrentIsr = &IsrTable[Vector][(ULONG)InterruptObject->ProcessorNumber];

   KeRaiseIrql(VECTOR2IRQL(Vector + IRQ_BASE),&oldlvl);
   KiAcquireSpinLock(&CurrentIsr->Lock);

   /*
    * Check if the vector is already in use that we can share it
    */
   if (!IsListEmpty(&CurrentIsr->ListHead))
   {
      ListHead = CONTAINING_RECORD(CurrentIsr->ListHead.Flink,KINTERRUPT,Entry);
      if (InterruptObject->Shareable == FALSE || ListHead->Shareable==FALSE)
      {
         KiReleaseSpinLock(&CurrentIsr->Lock);
         KeLowerIrql(oldlvl);
         KeRevertToUserAffinityThread();
         return FALSE;
      }
   }

   synch_oldlvl = KeAcquireInterruptSpinLock(InterruptObject);

   DPRINT("%x %x\n",CurrentIsr->ListHead.Flink, CurrentIsr->ListHead.Blink);

   Result = HalEnableSystemInterrupt(Vector + IRQ_BASE, InterruptObject->Irql, InterruptObject->InterruptMode);
   if (Result)
   {
      InsertTailList(&CurrentIsr->ListHead,&InterruptObject->Entry);
      DPRINT("%x %x\n",InterruptObject->Entry.Flink, InterruptObject->Entry.Blink);
   }

   KeReleaseInterruptSpinLock(InterruptObject, synch_oldlvl);

   /*
    * Release the table spinlock
    */
   KiReleaseSpinLock(&CurrentIsr->Lock);
   KeLowerIrql(oldlvl);
   
   KeDumpIrqList();

   KeRevertToUserAffinityThread();

   return Result;
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
   KIRQL oldlvl,synch_oldlvl;
   PISR_TABLE CurrentIsr;

   DPRINT("KeDisconnectInterrupt\n");
   
   ASSERT (InterruptObject->ProcessorNumber < KeNumberProcessors);

   KeSetSystemAffinityThread(1 << InterruptObject->ProcessorNumber);

   CurrentIsr = &IsrTable[InterruptObject->Vector - IRQ_BASE][(ULONG)InterruptObject->ProcessorNumber];

   KeRaiseIrql(VECTOR2IRQL(InterruptObject->Vector),&oldlvl);
   KiAcquireSpinLock(&CurrentIsr->Lock);

   synch_oldlvl = KeAcquireInterruptSpinLock(InterruptObject);

   RemoveEntryList(&InterruptObject->Entry);
   if (IsListEmpty(&CurrentIsr->ListHead))
   {
      HalDisableSystemInterrupt(InterruptObject->Vector, 0);
   }
   KeReleaseInterruptSpinLock(InterruptObject, synch_oldlvl);

   /*
    * Release the table spinlock
    */
   KiReleaseSpinLock(&CurrentIsr->Lock);
   KeLowerIrql(oldlvl);
   
   KeRevertToUserAffinityThread();
}


/*
 * @implemented
 */
VOID
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
		      CHAR ProcessorNumber,
		      BOOLEAN FloatingSave)
{
   InterruptObject->ServiceRoutine = ServiceRoutine;
   InterruptObject->ServiceContext = ServiceContext;
   InterruptObject->ActualLock = SpinLock;
   InterruptObject->Vector = Vector;
   InterruptObject->Irql = Irql;
   InterruptObject->SynchLevel = SynchronizeIrql;
   InterruptObject->InterruptMode = InterruptMode;
   InterruptObject->Shareable = ShareVector;
   InterruptObject->ProcessorNumber = ProcessorNumber;
   InterruptObject->FloatingSave = FloatingSave;
}

VOID KePrintInterruptStatistic(VOID)
{
   ULONG i, j;

   for (j = 0; j < KeNumberProcessors; j++)
   {
      DPRINT1("CPU%d:\n", j);
      for (i = 0; i < NR_IRQS; i++)
      {
         if (IsrTable[i][j].Count)
	 {
	     DPRINT1("  Irq %x(%d): %d\n", i, VECTOR2IRQ(i + IRQ_BASE), IsrTable[i][j].Count);
	 }
      }
   }
}


/* EOF */
