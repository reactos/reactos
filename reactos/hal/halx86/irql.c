/* $Id: irql.c,v 1.16 2004/10/17 15:39:27 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/irql.c
 * PURPOSE:         Implements IRQLs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <ntos/minmax.h>
#include <hal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

#define NR_IRQS         (16)
#define IRQ_BASE        (0x40)

/*
 * PURPOSE: Current irq level
 */
static BOOLEAN ApcRequested = FALSE;
static BOOLEAN DpcRequested = FALSE;

typedef union
{
   USHORT both;
   struct
   {
      BYTE master;
      BYTE slave;
   };
}
PIC_MASK;
   
/* 
 * PURPOSE: - Mask for HalEnableSystemInterrupt and HalDisableSystemInterrupt
 *          - At startup enable timer and cascade 
 */
#if defined(__GNUC__)
static PIC_MASK pic_mask = {.both = 0xFFFA};
#else
static PIC_MASK pic_mask = { 0xFFFA };
#endif


/*
 * PURPOSE: Mask for disabling of acknowledged interrupts 
 */
#if defined(__GNUC__)
static PIC_MASK pic_mask_intr = {.both = 0x0000};
#else
static PIC_MASK pic_mask_intr = { 0 };
#endif

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
  return(KeGetCurrentKPCR()->Irql);
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
  /* Enable interrupts */
  WRITE_PORT_UCHAR((PUCHAR)0x21, pic_mask.master);
  WRITE_PORT_UCHAR((PUCHAR)0xa1, pic_mask.slave);
  
  /* We can now enable interrupts */
  Ki386EnableInterrupts();
}

VOID HalpEndSystemInterrupt(KIRQL Irql)
/*
 * FUNCTION: Enable all irqs with higher priority.
 */
{
  ULONG flags;
  const USHORT mask[] = 
  {
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0xc000, 0xe000, 0xf000,
     0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0,
     0xfff8, 0xfffc, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  };     

  /* Interrupts should be disable while enabling irqs of both pics */
  Ki386SaveFlags(flags);
  Ki386DisableInterrupts();

  pic_mask_intr.both &= mask[Irql];
  WRITE_PORT_UCHAR((PUCHAR)0x21, (UCHAR)(pic_mask.master|pic_mask_intr.master));
  WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)(pic_mask.slave|pic_mask_intr.slave));

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
	   KeGetCurrentKPCR()->Irql = (KIRQL)IRQ_TO_DIRQL(i);

           while (HalpPendingInterruptCount[i] > 0)
	     {
	       /*
	        * For each deferred interrupt execute all the handlers at DIRQL.
	        */
	       HalpPendingInterruptCount[i]--;
	       KiInterruptDispatch2(i, NewIrql);
	     }
	   KeGetCurrentKPCR()->Irql--;
	   HalpEndSystemInterrupt(KeGetCurrentKPCR()->Irql);
	}
    }

}

VOID STATIC
HalpLowerIrql(KIRQL NewIrql)
{
  if (NewIrql >= PROFILE_LEVEL)
    {
      KeGetCurrentKPCR()->Irql = NewIrql;
      return;
    }
  HalpExecuteIrqs(NewIrql);
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeGetCurrentKPCR()->Irql = NewIrql;
      return;
    }
  KeGetCurrentKPCR()->Irql = DISPATCH_LEVEL;
  KiDispatchInterrupt();
  KeGetCurrentKPCR()->Irql = APC_LEVEL;
  if (NewIrql == APC_LEVEL)
    {
      return;
    }
  if (KeGetCurrentThread() != NULL && 
      KeGetCurrentThread()->ApcState.KernelApcPending)
    {
      KiDeliverApc(0, 0, 0);
    }
  KeGetCurrentKPCR()->Irql = PASSIVE_LEVEL;
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
  
  if (NewIrql > KeGetCurrentKPCR()->Irql)
    {
      DbgPrint ("(%s:%d) NewIrql %x CurrentIrql %x\n",
		__FILE__, __LINE__, NewIrql, KeGetCurrentKPCR()->Irql);
      KEBUGCHECK(0);
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
  
  if (NewIrql < KeGetCurrentKPCR()->Irql)
    {
      DbgPrint ("%s:%d CurrentIrql %x NewIrql %x\n",
		__FILE__,__LINE__,KeGetCurrentKPCR()->Irql,NewIrql);
      KEBUGCHECK (0);
      for(;;);
    }
  
  OldIrql = KeGetCurrentKPCR()->Irql;
  KeGetCurrentKPCR()->Irql = NewIrql;
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
  ULONG irq;
  if (Vector < IRQ_BASE || Vector >= IRQ_BASE + NR_IRQS)
    {
      return(FALSE);
    }
  irq = Vector - IRQ_BASE;
  pic_mask_intr.both |= ((1 << irq) & 0xfffe);	// do not disable the timer interrupt

  if (irq < 8)
  {
     WRITE_PORT_UCHAR((PUCHAR)0x21, (UCHAR)(pic_mask.master|pic_mask_intr.master));
     WRITE_PORT_UCHAR((PUCHAR)0x20, 0x20);
  }
  else
  {
     WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)(pic_mask.slave|pic_mask_intr.slave));
     /* Send EOI to the PICs */
     WRITE_PORT_UCHAR((PUCHAR)0x20,0x20);
     WRITE_PORT_UCHAR((PUCHAR)0xa0,0x20);
  }
  
  if (KeGetCurrentKPCR()->Irql >= Irql)
    {
      HalpPendingInterruptCount[irq]++;
      return(FALSE);
    }
  *OldIrql = KeGetCurrentKPCR()->Irql;
  KeGetCurrentKPCR()->Irql = Irql;

  return(TRUE);
}


VOID STDCALL HalEndSystemInterrupt (KIRQL Irql, ULONG Unknown2)
/*
 * FUNCTION: Finish a system interrupt and restore the specified irq level.
 */
{
  HalpLowerIrql(Irql);
  HalpEndSystemInterrupt(Irql);
}
  
BOOLEAN STDCALL HalDisableSystemInterrupt (ULONG Vector,
					   ULONG Unknown2)
{
  ULONG irq;
  
  if (Vector < IRQ_BASE || Vector >= IRQ_BASE + NR_IRQS)
    return FALSE;

  irq = Vector - IRQ_BASE;
  pic_mask.both |= (1 << irq);
  if (irq < 8)
     {
      WRITE_PORT_UCHAR((PUCHAR)0x21, (UCHAR)(pic_mask.master|pic_mask_intr.slave));
     }
  else
    {
      WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)(pic_mask.slave|pic_mask_intr.slave));
    }
  
  return TRUE;
}


BOOLEAN STDCALL HalEnableSystemInterrupt (ULONG Vector,
					  ULONG Unknown2,
					  ULONG Unknown3)
{
  ULONG irq;

  if (Vector < IRQ_BASE || Vector >= IRQ_BASE + NR_IRQS)
    return FALSE;

  irq = Vector - IRQ_BASE;
  pic_mask.both &= ~(1 << irq);
  if (irq < 8)
    {
      WRITE_PORT_UCHAR((PUCHAR)0x21, (UCHAR)(pic_mask.master|pic_mask_intr.master));
    }
  else
     {
       WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)(pic_mask.slave|pic_mask_intr.slave));
     }

  return TRUE;
}


VOID FASTCALL
HalRequestSoftwareInterrupt(
  IN KIRQL Request)
{
  switch (Request)
  {
    case APC_LEVEL:
      ApcRequested = TRUE;
      break;

    case DISPATCH_LEVEL:
      DpcRequested = TRUE;
      break;
      
    default:
      KEBUGCHECK(0);
  }
}

/* EOF */
