/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/mpsirql.c
 * PURPOSE:         Implements IRQLs for multiprocessor systems
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *     12/04/2001  CSH  Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <ntos/minmax.h>
#include <mps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/;

#define IRQ_BASE    (0x30)
#define NR_VECTORS  (0x100 - IRQ_BASE)

extern IMPORTED ULONG DpcQueueSize;

static ULONG HalpPendingInterruptCount[NR_VECTORS];

static VOID KeSetCurrentIrql (KIRQL newlvl);

VOID STDCALL
KiInterruptDispatch2 (ULONG Irq, KIRQL old_level);

#define IRQL2TPR(irql) (FIRST_DEVICE_VECTOR + ((irql - DISPATCH_LEVEL /* 2 */ - 1) * 8))

/* FUNCTIONS ****************************************************************/

KIRQL STDCALL KeGetCurrentIrql (VOID)
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
  if (KeGetCurrentKPCR ()->Irql > HIGH_LEVEL)
    {
      DPRINT1 ("CurrentIrql %x\n", KeGetCurrentKPCR ()->Irql);
      KeBugCheck (0);
      for(;;);
    }

   return(KeGetCurrentKPCR ()->Irql);
}


static VOID KeSetCurrentIrql (KIRQL NewIrql)
/*
 * PURPOSE: Sets the current irq level without taking any action
 */
{
  if (NewIrql > HIGH_LEVEL)
    {
      DPRINT1 ("NewIrql %x\n", NewIrql);
      KeBugCheck (0);
      for(;;);
    }

   KeGetCurrentKPCR ()->Irql = NewIrql;
}


VOID HalpEndSystemInterrupt (KIRQL Irql)
/*
 * FUNCTION: Enable all irqs with higher priority.
 */
{
  /* Interrupts should be disabled while enabling irqs */
  __asm__("pushf\n\t");
  __asm__("cli\n\t");
  APICWrite (APIC_TPR, IRQL2TPR (Irql) & APIC_TPR_PRI);
  __asm__("popf\n\t");
}


VOID STATIC
HalpExecuteIrqs(KIRQL NewIrql)
{
  ULONG VectorLimit, i;
  
  VectorLimit = min(IRQL2VECTOR (NewIrql), NR_VECTORS);

  /*
   * For each vector if there have been any deferred interrupts then now
   * dispatch them.
   */
  for (i = 0; i < VectorLimit; i++)
    {
      if (HalpPendingInterruptCount[i] > 0)
	{
	   KeSetCurrentIrql (VECTOR2IRQL (i));

           while (HalpPendingInterruptCount[i] > 0)
	     {
	       /*
	        * For each deferred interrupt execute all the handlers at DIRQL.
	        */
	       KiInterruptDispatch2 (i, NewIrql);
	       HalpPendingInterruptCount[i]--;
	     }
	   KeSetCurrentIrql (KeGetCurrentIrql () - 1);
	   HalpEndSystemInterrupt (KeGetCurrentIrql ());
	}
    }

}


VOID STATIC
HalpLowerIrql(KIRQL NewIrql)
{
  if (NewIrql >= PROFILE_LEVEL)
    {
      KeSetCurrentIrql (NewIrql);
      return;
    }
  HalpExecuteIrqs (NewIrql);
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeSetCurrentIrql (NewIrql);
      return;
    }
  KeSetCurrentIrql (DISPATCH_LEVEL);
  if (DpcQueueSize > 0)
    {
      KiDispatchInterrupt ();
    }
  KeSetCurrentIrql (APC_LEVEL);
  if (NewIrql == APC_LEVEL)
    {
      return;
    }
  if (KeGetCurrentThread () != NULL && 
      KeGetCurrentThread ()->ApcState.KernelApcPending)
    {
      KiDeliverApc (0, 0, 0);
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
  KIRQL OldIrql;

  if (NewIrql > KeGetCurrentIrql ())
    {
      DPRINT1 ("NewIrql %x CurrentIrql %x\n", NewIrql, KeGetCurrentIrql ());
      KeBugCheck (0);
      for(;;);
    }
  
  HalpLowerIrql (NewIrql);
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
  
  if (NewIrql < KeGetCurrentIrql ())
    {
      DPRINT1 ("CurrentIrql %x NewIrql %x\n", KeGetCurrentIrql (), NewIrql);
      KeBugCheck (0);
      for(;;);
    }
  
  OldIrql = KeGetCurrentIrql ();
  KeSetCurrentIrql (NewIrql);
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
  DPRINT("Vector (0x%X)  Irql (0x%X)\n", Vector, Irql);

  if (Vector < FIRST_DEVICE_VECTOR ||
    Vector >= FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS) {
    DPRINT("Not a device interrupt\n");
	  return FALSE;
  }

  HalDisableSystemInterrupt (Vector, 0);

  APICSendEOI();

  if (KeGetCurrentIrql () >= Irql)
    {
      HalpPendingInterruptCount[Vector]++;
      return(FALSE);
    }
  *OldIrql = KeGetCurrentIrql ();
  KeSetCurrentIrql (Irql);

  return(TRUE);
}


VOID STDCALL
HalEndSystemInterrupt (KIRQL Irql,
	ULONG Unknown2)
/*
 * FUNCTION: Finish a system interrupt and restore the specified irq level.
 */
{
  HalpLowerIrql (Irql);
  HalpEndSystemInterrupt (Irql);
}
  
BOOLEAN STDCALL
HalDisableSystemInterrupt (ULONG Vector,
	ULONG Unknown2)
{
  ULONG irq;

  DPRINT ("Vector (0x%X)\n", Vector);

  if (Vector < FIRST_DEVICE_VECTOR ||
    Vector >= FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS)  {
    DPRINT("Not a device interrupt\n");
	  return FALSE;
  }

  irq = VECTOR2IRQ (Vector);
  IOAPICMaskIrq (ThisCPU (), irq);

  return TRUE;  
}


BOOLEAN STDCALL
HalEnableSystemInterrupt (ULONG Vector,
	ULONG Unknown2,
	ULONG Unknown3)
{
  ULONG irq;

  DPRINT ("Vector (0x%X)\n", Vector);

  if (Vector < FIRST_DEVICE_VECTOR ||
    Vector >= FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS) {
    DPRINT("Not a device interrupt\n");
	  return FALSE;
  }

  irq = VECTOR2IRQ (Vector);
  IOAPICUnmaskIrq (ThisCPU (), irq);

  return TRUE;
}
