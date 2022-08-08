/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/halx86/mp/mpsirql.c
 * PURPOSE:         Implements IRQLs for multiprocessor systems
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *     12/04/2001  CSH  Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/


/* FUNCTIONS ****************************************************************/

#undef KeGetCurrentIrql
KIRQL NTAPI KeGetCurrentIrql (VOID)
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
  KIRQL irql;
  ULONG Flags;

  Flags = __readeflags();
  _disable();

  irql = __readfsbyte(FIELD_OFFSET(KPCR, Irql));
  if (irql > HIGH_LEVEL)
    {
      DPRINT1 ("CurrentIrql %x\n", irql);
      ASSERT(FALSE);
    }
  if (Flags & EFLAGS_INTERRUPT_MASK)
    {
      _enable();
    }
  return irql;
}


#undef KeSetCurrentIrql
VOID KeSetCurrentIrql (KIRQL NewIrql)
/*
 * PURPOSE: Sets the current irq level without taking any action
 */
{
  ULONG Flags;
  if (NewIrql > HIGH_LEVEL)
  {
    DPRINT1 ("NewIrql %x\n", NewIrql);
    ASSERT(FALSE);
  }
  Flags = __readeflags();
  _disable();
  __writefsbyte(FIELD_OFFSET(KPCR, Irql), NewIrql);
  if (Flags & EFLAGS_INTERRUPT_MASK)
    {
      _enable();
    }
}

VOID
HalpLowerIrql(KIRQL NewIrql, BOOLEAN FromHalEndSystemInterrupt)
{
  ULONG Flags;
  UCHAR DpcRequested;
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeSetCurrentIrql (NewIrql);
      APICWrite(APIC_TPR, IRQL2TPR (NewIrql) & APIC_TPR_PRI);
      return;
    }
  Flags = __readeflags();
  if (KeGetCurrentIrql() > APC_LEVEL)
    {
      KeSetCurrentIrql (DISPATCH_LEVEL);
      APICWrite(APIC_TPR, IRQL2TPR (DISPATCH_LEVEL) & APIC_TPR_PRI);
      DpcRequested = __readfsbyte(FIELD_OFFSET(KPCR, HalReserved[HAL_DPC_REQUEST]));
      if (FromHalEndSystemInterrupt || DpcRequested)
        {
          __writefsbyte(FIELD_OFFSET(KPCR, HalReserved[HAL_DPC_REQUEST]), 0);
          _enable();
          KiDispatchInterrupt();
          if (!(Flags & EFLAGS_INTERRUPT_MASK))
            {
              _disable();
            }
	}
      KeSetCurrentIrql (APC_LEVEL);
    }
  if (NewIrql == APC_LEVEL)
    {
      return;
    }
  if (KeGetCurrentThread () != NULL &&
      KeGetCurrentThread ()->ApcState.KernelApcPending)
    {
      _enable();
      KiDeliverApc(KernelMode, NULL, NULL);
      if (!(Flags & EFLAGS_INTERRUPT_MASK))
        {
          _disable();
        }
    }
  KeSetCurrentIrql (PASSIVE_LEVEL);
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
  KIRQL oldIrql = KeGetCurrentIrql();
  if (NewIrql > oldIrql)
    {
      DPRINT1 ("NewIrql %x CurrentIrql %x\n", NewIrql, oldIrql);
      ASSERT(FALSE);
    }
  HalpLowerIrql (NewIrql, FALSE);
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
  ULONG Flags;

  Flags = __readeflags();
  _disable();

  OldIrql = KeGetCurrentIrql ();

  if (NewIrql < OldIrql)
    {
      DPRINT1 ("CurrentIrql %x NewIrql %x\n", KeGetCurrentIrql (), NewIrql);
      ASSERT(FALSE);
    }


  if (NewIrql > DISPATCH_LEVEL)
    {
      APICWrite (APIC_TPR, IRQL2TPR(NewIrql) & APIC_TPR_PRI);
    }
  KeSetCurrentIrql (NewIrql);
  if (Flags & EFLAGS_INTERRUPT_MASK)
    {
      _enable();
    }

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

KIRQL NTAPI
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

KIRQL NTAPI
KeRaiseIrqlToSynchLevel (VOID)
{
  return KfRaiseIrql (CLOCK2_LEVEL);
}


BOOLEAN NTAPI
HalBeginSystemInterrupt (KIRQL Irql,
			 ULONG Vector,
			 PKIRQL OldIrql)
{
  ULONG Flags;
  DPRINT("Vector (0x%X)  Irql (0x%X)\n", Vector, Irql);

  if (KeGetCurrentIrql () >= Irql)
  {
    DPRINT1("current irql %d, new irql %d\n", KeGetCurrentIrql(), Irql);
    ASSERT(FALSE);
  }

  Flags = __readeflags();
  if (Flags & EFLAGS_INTERRUPT_MASK)
  {
     DPRINT1("HalBeginSystemInterrupt was called with interrupt's enabled\n");
     ASSERT(FALSE);
  }
  APICWrite (APIC_TPR, IRQL2TPR (Irql) & APIC_TPR_PRI);
  *OldIrql = KeGetCurrentIrql ();
  KeSetCurrentIrql (Irql);
  return(TRUE);
}


VOID NTAPI
HalEndSystemInterrupt (KIRQL Irql,
                       IN PKTRAP_FRAME TrapFrame)
/*
 * FUNCTION: Finish a system interrupt and restore the specified irq level.
 */
{
  ULONG Flags;
  Flags = __readeflags();

  if (Flags & EFLAGS_INTERRUPT_MASK)
  {
     DPRINT1("HalEndSystemInterrupt was called with interrupt's enabled\n");
     ASSERT(FALSE);
  }
  APICSendEOI();
  HalpLowerIrql (Irql, TRUE);
}

VOID
NTAPI
HalDisableSystemInterrupt(ULONG Vector,
			  KIRQL Irql)
{
  ULONG irq;

  DPRINT ("Vector (0x%X)\n", Vector);

  if (Vector < FIRST_DEVICE_VECTOR ||
      Vector >= FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS)
  {
    DPRINT1("Not a device interrupt, vector=%x\n", Vector);
    ASSERT(FALSE);
    return;
  }

  irq = VECTOR2IRQ (Vector);
  IOAPICMaskIrq (irq);

  return;
}


BOOLEAN NTAPI
HalEnableSystemInterrupt (ULONG Vector,
			  KIRQL Irql,
			  KINTERRUPT_MODE InterruptMode)
{
  ULONG irq;

  if (Vector < FIRST_DEVICE_VECTOR ||
      Vector >= FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS)
  {
    DPRINT("Not a device interrupt\n");
    return FALSE;
  }

  /* FIXME: We must check if the requested and the assigned interrupt mode is the same */

  irq = VECTOR2IRQ (Vector);
  IOAPICUnmaskIrq (irq);

  return TRUE;
}

VOID FASTCALL
HalRequestSoftwareInterrupt(IN KIRQL Request)
{
  switch (Request)
  {
    case APC_LEVEL:
      __writefsbyte(FIELD_OFFSET(KPCR, HalReserved[HAL_APC_REQUEST]), 1);
      break;

    case DISPATCH_LEVEL:
      __writefsbyte(FIELD_OFFSET(KPCR, HalReserved[HAL_DPC_REQUEST]), 1);
      break;

    default:
      ASSERT(FALSE);
  }
}

VOID FASTCALL
HalClearSoftwareInterrupt(
  IN KIRQL Request)
{
  UNIMPLEMENTED;
}
