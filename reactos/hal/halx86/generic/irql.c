/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/irql.c
 * PURPOSE:         Implements IRQLs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

/*
 * FIXME: Use EISA_CONTROL STRUCTURE INSTEAD OF HARD-CODED OFFSETS 
*/

UCHAR Table[8] =
{
    0, 0,
    1, 1,
    2, 2, 2, 2
};

ULONG pic_mask = {0xFFFFFFFA};

static ULONG HalpPendingInterruptCount[NR_IRQS] = {0};

#define DIRQL_TO_IRQ(x)  (PROFILE_LEVEL - x)
#define IRQ_TO_DIRQL(x)  (PROFILE_LEVEL - x)

VOID STDCALL
KiInterruptDispatch2 (ULONG Irq, KIRQL old_level);

/* FUNCTIONS ****************************************************************/

extern ULONG KiI8259MaskTable[];

VOID HalpEndSystemInterrupt(KIRQL Irql)
/*
 * FUNCTION: Enable all irqs with higher priority.
 */
{
  ULONG flags;
  ULONG Mask;

  /* Interrupts should be disable while enabling irqs of both pics */
  Ki386SaveFlags(flags);
  Ki386DisableInterrupts();

  Mask = pic_mask | KiI8259MaskTable[Irql];
  WRITE_PORT_UCHAR((PUCHAR)0x21,  (UCHAR)Mask);
  Mask >>= 8;
  WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)Mask);

  /* restore flags */
  Ki386RestoreFlags(flags);
}

VOID STATIC
HalpExecuteIrqs(KIRQL NewIrql)
{
  ULONG IrqLimit, i;
  
  IrqLimit = min(PROFILE_LEVEL - NewIrql, NR_IRQS);

  /*
   * For each irq if there have been any deferred interrupts then now
   * dispatch them.
   */
  for (i = 0; i < IrqLimit; i++)
    {
      if (HalpPendingInterruptCount[i] > 0)
	{
	   KeGetPcr()->Irql = (KIRQL)IRQ_TO_DIRQL(i);

           while (HalpPendingInterruptCount[i] > 0)
	     {
	       /*
	        * For each deferred interrupt execute all the handlers at DIRQL.
	        */
	       HalpPendingInterruptCount[i]--;
	       KiInterruptDispatch2(i + IRQ_BASE, NewIrql);
	     }
	   KeGetPcr()->Irql--;
	   HalpEndSystemInterrupt(KeGetPcr()->Irql);
	}
    }

}

VOID STATIC
HalpLowerIrql(KIRQL NewIrql)
{
  if (NewIrql >= PROFILE_LEVEL)
    {
      KeGetPcr()->Irql = NewIrql;
      return;
    }
  HalpExecuteIrqs(NewIrql);
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeGetPcr()->Irql = NewIrql;
      return;
    }
  KeGetPcr()->Irql = DISPATCH_LEVEL;
  if (Table[KeGetPcr()->IRR] >= NewIrql)
    {
      KeGetPcr()->IRR &= ~4;
      KiDispatchInterrupt();
    }
  KeGetPcr()->Irql = APC_LEVEL;
  if (NewIrql == APC_LEVEL)
    {
      return;
    }
  if (KeGetCurrentThread() != NULL && 
      KeGetCurrentThread()->ApcState.KernelApcPending)
    {
      KiDeliverApc(KernelMode, NULL, NULL);
    }
  KeGetPcr()->Irql = PASSIVE_LEVEL;
}

/**********************************************************************
 * NAME							EXPORTED
 *	KfLowerIrql
 *
 * DESCRIPTION
 *	Restores the irq level on the current processor
 *
 * ARGUMENTS
 *	NewIrql = Irql to lower to
 *
 * RETURN VALUE
 *	None
 *
 * NOTES
 *	Uses fastcall convention
 */
VOID FASTCALL
KfLowerIrql (KIRQL	NewIrql)
{
  DPRINT("KfLowerIrql(NewIrql %d)\n", NewIrql);
  
  if (NewIrql > KeGetPcr()->Irql)
    {
      DbgPrint ("(%s:%d) NewIrql %x CurrentIrql %x\n",
		__FILE__, __LINE__, NewIrql, KeGetPcr()->Irql);
      KEBUGCHECK(0);
      for(;;);
    }
  
  HalpLowerIrql(NewIrql);
}


BOOLEAN STDCALL 
HalBeginSystemInterrupt (KIRQL Irql,
			 ULONG Vector,
			 PKIRQL OldIrql)
{
  ULONG irq;
  ULONG Mask;

  if (Vector < IRQ_BASE || Vector >= IRQ_BASE + NR_IRQS)
    {
      return(FALSE);
    }
  irq = Vector - IRQ_BASE;

  Mask = pic_mask | KiI8259MaskTable[Irql];
  WRITE_PORT_UCHAR((PUCHAR)0x21,  (UCHAR)Mask);
  Mask >>= 8;
  WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)Mask);

  if (irq < 8)
  {
     WRITE_PORT_UCHAR((PUCHAR)0x20, 0x60 | irq);
  }
  else
  {
     /* Send EOI to the PICs */
     WRITE_PORT_UCHAR((PUCHAR)0x20,0x62);
     WRITE_PORT_UCHAR((PUCHAR)0xa0,0x20);
  }

  *OldIrql = KeGetPcr()->Irql;
  KeGetPcr()->Irql = Irql;

  return(TRUE);
}


VOID STDCALL HalEndSystemInterrupt (KIRQL Irql, ULONG Unknown2)
/*
 * FUNCTION: Finish a system interrupt and restore the specified irq level.
 */
{
    //DPRINT1("ENDING: %lx %lx\n", Irql, Unknown2);
  HalpLowerIrql(Irql);
  HalpEndSystemInterrupt(Irql);
}

BOOLEAN
STDCALL
HalEnableSystemInterrupt(
  ULONG Vector,
  KIRQL Irql,
  KINTERRUPT_MODE InterruptMode)
{
  ULONG irq;
  ULONG Mask;

  if (Vector < IRQ_BASE || Vector >= IRQ_BASE + NR_IRQS)
    return FALSE;

  irq = Vector - IRQ_BASE;
  pic_mask &= ~(1 << irq);

  Mask = pic_mask | KiI8259MaskTable[KeGetPcr()->Irql];
  WRITE_PORT_UCHAR((PUCHAR)0x21,  (UCHAR)Mask);
  Mask >>= 8;
  WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)Mask);

  return TRUE;
}



/* EOF */
