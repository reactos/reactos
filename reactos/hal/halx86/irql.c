/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/irql.c
 * PURPOSE:         Implements IRQLs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

#define NR_IRQS         (16)
#define IRQ_BASE        (0x40)

/*
 * PURPOSE: Current irq level
 */
static KIRQL CurrentIrql = HIGH_LEVEL;

extern IMPORTED ULONG DpcQueueSize;

static ULONG HalpPendingInterruptCount[NR_IRQS];

#define DIRQL_TO_IRQ(x)  (PROFILE_LEVEL - x)
#define IRQ_TO_DIRQL(x)  (PROFILE_LEVEL - x)

VOID STDCALL
KiInterruptDispatch2 (ULONG Irq, KIRQL old_level);

/* FUNCTIONS ****************************************************************/

KIRQL STDCALL KeGetCurrentIrql (VOID)
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
  return(CurrentIrql);
}

VOID HalpInitPICs(VOID)
{
  memset(HalpPendingInterruptCount, 0, sizeof(HalpPendingInterruptCount));

  /* Initialization sequence */
  WRITE_PORT_UCHAR((PUCHAR)0x20, 0x11);
  WRITE_PORT_UCHAR((PUCHAR)0xa0, 0x11);
  /* Start of hardware irqs (0x24) */
  WRITE_PORT_UCHAR((PUCHAR)0x21, 0x40);
  WRITE_PORT_UCHAR((PUCHAR)0xa1, 0x48);
  /* 8259-1 is master */
  WRITE_PORT_UCHAR((PUCHAR)0x21, 0x4);
  /* 8259-2 is slave */
  WRITE_PORT_UCHAR((PUCHAR)0xa1, 0x2);
  /* 8086 mode */
  WRITE_PORT_UCHAR((PUCHAR)0x21, 0x1);
  WRITE_PORT_UCHAR((PUCHAR)0xa1, 0x1);   
  /* Enable all interrupts from PICs */
  WRITE_PORT_UCHAR((PUCHAR)0x21, 0x0);
  WRITE_PORT_UCHAR((PUCHAR)0xa1, 0x0);
  
  /* We can now enable interrupts */
  __asm__ __volatile__ ("sti\n\t");
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
      while (HalpPendingInterruptCount[i] > 0)
	{
	  /*
	   * For each deferred interrupt execute all the handlers at DIRQL.
	   */
	  CurrentIrql = IRQ_TO_DIRQL(i);
	  KiInterruptDispatch2(i, NewIrql);
	  HalpPendingInterruptCount[i]--;
	}
    }
}

VOID STATIC
HalpLowerIrql(KIRQL NewIrql)
{
  if (NewIrql > PROFILE_LEVEL)
    {
      CurrentIrql = NewIrql;
      return;
    }
  HalpExecuteIrqs(NewIrql);
  if (NewIrql >= DISPATCH_LEVEL)
    {
      CurrentIrql = NewIrql;
      return;
    }
  CurrentIrql = DISPATCH_LEVEL;
  if (DpcQueueSize > 0)
    {
      KiDispatchInterrupt();
    }
  if (NewIrql == APC_LEVEL)
    {
      CurrentIrql = NewIrql;
      return;
    }
  CurrentIrql = APC_LEVEL;
  if (KeGetCurrentThread() != NULL && 
      KeGetCurrentThread()->ApcState.KernelApcPending)
    {
      KiDeliverApc(0, 0, 0);
    }
  CurrentIrql = PASSIVE_LEVEL;
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
  KIRQL OldIrql;
  
  DPRINT("KfLowerIrql(NewIrql %d)\n", NewIrql);
  
  if (NewIrql > CurrentIrql)
    {
      DbgPrint ("(%s:%d) NewIrql %x CurrentIrql %x\n",
		__FILE__, __LINE__, NewIrql, CurrentIrql);
      KeBugCheck(0);
      for(;;);
    }
  
  HalpLowerIrql(NewIrql);
}


/**********************************************************************
 * NAME							EXPORTED
 *	KeLowerIrql
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
 */

VOID STDCALL
KeLowerIrql (KIRQL NewIrql)
{
  KfLowerIrql (NewIrql);
}


/**********************************************************************
 * NAME							EXPORTED
 *	KfRaiseIrql
 *
 * DESCRIPTION
 *	Raises the hardware priority (irql)
 *
 * ARGUMENTS
 *	NewIrql = Irql to raise to
 *
 * RETURN VALUE
 *	previous irq level
 *
 * NOTES
 *	Uses fastcall convention
 */

KIRQL FASTCALL
KfRaiseIrql (KIRQL	NewIrql)
{
  KIRQL OldIrql;
  
  DPRINT("KfRaiseIrql(NewIrql %d)\n", NewIrql);
  
  if (NewIrql < CurrentIrql)
    {
      DbgPrint ("%s:%d CurrentIrql %x NewIrql %x\n",
		__FILE__,__LINE__,CurrentIrql,NewIrql);
      KeBugCheck (0);
      for(;;);
    }
  
  OldIrql = CurrentIrql;
  CurrentIrql = NewIrql;
  return OldIrql;
}


/**********************************************************************
 * NAME							EXPORTED
 *	KeRaiseIrql
 *
 * DESCRIPTION
 *	Raises the hardware priority (irql)
 *
 * ARGUMENTS
 *	NewIrql = Irql to raise to
 *	OldIrql (OUT) = Caller supplied storage for the previous irql
 *
 * RETURN VALUE
 *	None
 *
 * NOTES
 *	Calls KfRaiseIrql
 */
VOID STDCALL
KeRaiseIrql (KIRQL	NewIrql,
	     PKIRQL	OldIrql)
{
  *OldIrql = KfRaiseIrql (NewIrql);
}


/**********************************************************************
 * NAME							EXPORTED
 *	KeRaiseIrqlToDpcLevel
 *
 * DESCRIPTION
 *	Raises the hardware priority (irql) to DISPATCH level
 *
 * ARGUMENTS
 *	None
 *
 * RETURN VALUE
 *	Previous irq level
 *
 * NOTES
 *	Calls KfRaiseIrql
 */

KIRQL STDCALL
KeRaiseIrqlToDpcLevel (VOID)
{
  return KfRaiseIrql (DISPATCH_LEVEL);
}


/**********************************************************************
 * NAME							EXPORTED
 *	KeRaiseIrqlToSynchLevel
 *
 * DESCRIPTION
 *	Raises the hardware priority (irql) to CLOCK2 level
 *
 * ARGUMENTS
 *	None
 *
 * RETURN VALUE
 *	Previous irq level
 *
 * NOTES
 *	Calls KfRaiseIrql
 */

KIRQL STDCALL
KeRaiseIrqlToSynchLevel (VOID)
{
  return KfRaiseIrql (CLOCK2_LEVEL);
}


BOOLEAN STDCALL 
HalBeginSystemInterrupt (ULONG Vector,
			 KIRQL Irql,
			 PKIRQL OldIrql)
{
  if (Vector < IRQ_BASE || Vector > IRQ_BASE + NR_IRQS)
    {
      return(FALSE);
    }
  
  /* Send EOI to the PICs */
  WRITE_PORT_UCHAR((PUCHAR)0x20,0x20);
  if ((Vector-IRQ_BASE)>=8)
    {
      WRITE_PORT_UCHAR((PUCHAR)0xa0,0x20);
    }
  
  if (CurrentIrql >= Irql)
    {
      HalpPendingInterruptCount[Vector - IRQ_BASE]++;
      return(FALSE);
    }
  *OldIrql = CurrentIrql;
  CurrentIrql = Irql;

  return(TRUE);
}


VOID STDCALL HalEndSystemInterrupt (KIRQL Irql, ULONG Unknown2)
/*
 * FUNCTION: Finish a system interrupt and restore the specified irq level.
 */
{
  HalpLowerIrql(Irql);
}

BOOLEAN STDCALL HalDisableSystemInterrupt (ULONG Vector,
					   ULONG Unknown2)
{
  ULONG irq;
  
  if (Vector < IRQ_BASE || Vector > IRQ_BASE + NR_IRQS)
    return FALSE;

  irq = Vector - IRQ_BASE;
  if (irq < 8)
     {
       WRITE_PORT_UCHAR((PUCHAR)0x21, 
			READ_PORT_UCHAR((PUCHAR)0x21)|(1<<irq));
     }
  else
    {
      WRITE_PORT_UCHAR((PUCHAR)0xa1, 
		       READ_PORT_UCHAR((PUCHAR)0xa1)|(1<<(irq-8)));
    }
  
  return TRUE;
}


BOOLEAN STDCALL HalEnableSystemInterrupt (ULONG Vector,
					  ULONG Unknown2,
					  ULONG Unknown3)
{
  ULONG irq;

  if (Vector < IRQ_BASE || Vector > IRQ_BASE + NR_IRQS)
    return FALSE;

  irq = Vector - IRQ_BASE;
  if (irq < 8)
    {
      WRITE_PORT_UCHAR((PUCHAR)0x21, 
		       READ_PORT_UCHAR((PUCHAR)0x21)&(~(1<<irq)));
    }
  else
     {
       WRITE_PORT_UCHAR((PUCHAR)0xa1, 
			READ_PORT_UCHAR((PUCHAR)0xa1)&(~(1<<(irq-8))));
     }

  return TRUE;
}

/* EOF */
