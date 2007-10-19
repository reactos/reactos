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

typedef union
{
   USHORT both;
   struct
   {
      UCHAR master;
      UCHAR slave;
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

#ifdef _MSC_VER

#define KiInterruptDispatch2(x, y)

#else

VOID STDCALL
KiInterruptDispatch2 (ULONG Irq, KIRQL old_level);

#endif

/* FUNCTIONS ****************************************************************/

#undef KeGetCurrentIrql
KIRQL STDCALL KeGetCurrentIrql (VOID)
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
  return(KeGetPcr()->Irql);
}

VOID NTAPI HalpInitPICs(VOID)
{
  memset(HalpPendingInterruptCount, 0, sizeof(HalpPendingInterruptCount));

  /* Initialization sequence */
  WRITE_PORT_UCHAR((PUCHAR)0x20, 0x11);
  WRITE_PORT_UCHAR((PUCHAR)0xa0, 0x11);
  /* Start of hardware irqs (0x24) */
  WRITE_PORT_UCHAR((PUCHAR)0x21, IRQ_BASE);
  WRITE_PORT_UCHAR((PUCHAR)0xa1, IRQ_BASE + 8);
  /* 8259-1 is master */
  WRITE_PORT_UCHAR((PUCHAR)0x21, 0x4);
  /* 8259-2 is slave */
  WRITE_PORT_UCHAR((PUCHAR)0xa1, 0x2);
  /* 8086 mode */
  WRITE_PORT_UCHAR((PUCHAR)0x21, 0x1);
  WRITE_PORT_UCHAR((PUCHAR)0xa1, 0x1);
  /* Enable interrupts */
  WRITE_PORT_UCHAR((PUCHAR)0x21, 0xFF);
  WRITE_PORT_UCHAR((PUCHAR)0xa1, 0xFF);

  /* We can now enable interrupts */
  _enable();
}

VOID HalpEndSystemInterrupt(KIRQL Irql)
/*
 * FUNCTION: Enable all irqs with higher priority.
 */
{
  const USHORT mask[] =
  {
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0xc000, 0xe000, 0xf000,
     0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0,
     0xfff8, 0xfffc, 0xfffe, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  };

  /* Interrupts should be disable while enabling irqs of both pics */
  _disable();

  pic_mask_intr.both &= mask[Irql];
  WRITE_PORT_UCHAR((PUCHAR)0x21, (UCHAR)(pic_mask.master|pic_mask_intr.master));
  WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)(pic_mask.slave|pic_mask_intr.slave));

  /* restore ints */
  _enable();
}

VOID
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
	       //HalpHardwareInt[i]();
	     }
	   //KeGetPcr()->Irql--;
	   //HalpEndSystemInterrupt(KeGetPcr()->Irql);
	}
    }

}

VOID
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
  if (((PKIPCR)KeGetPcr())->HalReserved[HAL_DPC_REQUEST])
    {
      ((PKIPCR)KeGetPcr())->HalReserved[HAL_DPC_REQUEST] = FALSE;
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

  if (NewIrql < KeGetPcr()->Irql)
    {
      DbgPrint ("%s:%d CurrentIrql %x NewIrql %x\n",
		__FILE__,__LINE__,KeGetPcr()->Irql,NewIrql);
      KEBUGCHECK (0);
      for(;;);
    }

  OldIrql = KeGetPcr()->Irql;
  KeGetPcr()->Irql = NewIrql;
  return OldIrql;
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
  return KfRaiseIrql (DISPATCH_LEVEL);
}


BOOLEAN STDCALL
HalBeginSystemInterrupt (KIRQL Irql,
			 ULONG Vector,
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
#if 0
  if (KeGetPcr()->Irql >= Irql)
    {
      HalpPendingInterruptCount[irq]++;
      return(FALSE);
    }
#endif
  *OldIrql = KeGetPcr()->Irql;
  KeGetPcr()->Irql = Irql;

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

BOOLEAN
STDCALL
HalDisableSystemInterrupt(
  ULONG Vector,
  KIRQL Irql)
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


BOOLEAN
STDCALL
HalEnableSystemInterrupt(
  ULONG Vector,
  KIRQL Irql,
  KINTERRUPT_MODE InterruptMode)
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
      ((PKIPCR)KeGetPcr())->HalReserved[HAL_APC_REQUEST] = TRUE;
      break;

    case DISPATCH_LEVEL:
      ((PKIPCR)KeGetPcr())->HalReserved[HAL_DPC_REQUEST] = TRUE;
      break;

    default:
      KEBUGCHECK(0);
  }
}

VOID FASTCALL
HalClearSoftwareInterrupt(
  IN KIRQL Request)
{
  switch (Request)
  {
    case APC_LEVEL:
      ((PKIPCR)KeGetPcr())->HalReserved[HAL_APC_REQUEST] = FALSE;
      break;

    case DISPATCH_LEVEL:
      ((PKIPCR)KeGetPcr())->HalReserved[HAL_DPC_REQUEST] = FALSE;
      break;

    default:
      KEBUGCHECK(0);
  }
}

/* EOF */
