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

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <ntos/minmax.h>
#include <halirq.h>
#include <hal.h>
#include <mps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/;


VOID STDCALL
KiInterruptDispatch2 (ULONG Irq, KIRQL old_level);

/* FUNCTIONS ****************************************************************/

KIRQL STDCALL KeGetCurrentIrql (VOID)
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
  KIRQL irql;
  ULONG Flags;

  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();
  irql = KeGetCurrentKPCR()->Irql;
  Ki386RestoreFlags(Flags);

  if (irql > HIGH_LEVEL)
    {
      DPRINT1 ("CurrentIrql %x\n", irql);
      KEBUGCHECK (0);
      for(;;);
    }
  return irql;
}


VOID KeSetCurrentIrql (KIRQL NewIrql)
/*
 * PURPOSE: Sets the current irq level without taking any action
 */
{
  ULONG Flags;
  if (NewIrql > HIGH_LEVEL)
    {
      DPRINT1 ("NewIrql %x\n", NewIrql);
      KEBUGCHECK (0);
      for(;;);
    }
  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();
  KeGetCurrentKPCR()->Irql = NewIrql;
  Ki386RestoreFlags(Flags);
}




VOID 
HalpLowerIrql(KIRQL NewIrql)
{
  PKPCR Pcr = KeGetCurrentKPCR();
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeSetCurrentIrql (NewIrql);
      APICWrite(APIC_TPR, IRQL2TPR (NewIrql) & APIC_TPR_PRI);
      return;
    }
  if (KeGetCurrentIrql() > APC_LEVEL)
  {
    KeSetCurrentIrql (DISPATCH_LEVEL);
    APICWrite(APIC_TPR, IRQL2TPR (DISPATCH_LEVEL) & APIC_TPR_PRI);
    if (Pcr->HalReserved[1])
    {
      Pcr->HalReserved[1] = 0;
      KiDispatchInterrupt();
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
  KIRQL oldIrql = KeGetCurrentIrql();
  if (NewIrql > oldIrql)
    {
      DPRINT1 ("NewIrql %x CurrentIrql %x\n", NewIrql, oldIrql);
      KEBUGCHECK (0);
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
  ULONG Flags;
  
  if (NewIrql < KeGetCurrentIrql ())
    {
      DPRINT1 ("CurrentIrql %x NewIrql %x\n", KeGetCurrentIrql (), NewIrql);
      KEBUGCHECK (0);
      for(;;);
    }
  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();
  if (NewIrql > DISPATCH_LEVEL)
    {
      APICWrite (APIC_TPR, IRQL2TPR (NewIrql) & APIC_TPR_PRI);
    }
  OldIrql = KeGetCurrentIrql ();
  KeSetCurrentIrql (NewIrql);
  Ki386RestoreFlags(Flags);
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
  ULONG Flags;
  DPRINT("Vector (0x%X)  Irql (0x%X)\n", Vector, Irql);

  if (KeGetCurrentIrql () >= Irql)
    {
      DPRINT1("current irql %d, new irql %d\n", KeGetCurrentIrql(), Irql);
      KEBUGCHECK(0);
    }

  if (Vector < FIRST_DEVICE_VECTOR ||
      Vector >= FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS) 
  {
    DPRINT1("Not a device interrupt, vector %x\n", Vector);
    return FALSE;
  }

  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();
  APICWrite (APIC_TPR, IRQL2TPR (Irql) & APIC_TPR_PRI);

  APICSendEOI();

  *OldIrql = KeGetCurrentIrql ();
  KeSetCurrentIrql (Irql);
  Ki386RestoreFlags(Flags);

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
}
  
BOOLEAN STDCALL
HalDisableSystemInterrupt (ULONG Vector,
			   KIRQL Irql)
{
  ULONG irq;

  DPRINT ("Vector (0x%X)\n", Vector);

  if (Vector < FIRST_DEVICE_VECTOR ||
      Vector >= FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS)  
  {
    DPRINT1("Not a device interrupt, vector=%x\n", Vector);
    return FALSE;
  }

  irq = VECTOR2IRQ (Vector);
  IOAPICMaskIrq (irq);

  return TRUE;  
}


BOOLEAN STDCALL
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

  irq = VECTOR2IRQ (Vector);
  IOAPICUnmaskIrq (irq);

  return TRUE;
}

VOID FASTCALL
HalRequestSoftwareInterrupt(IN KIRQL Request)
{
  ULONG Flags;
  switch (Request)
  {
    case APC_LEVEL:
      Ki386SaveFlags(Flags);
      Ki386DisableInterrupts();
      KeGetCurrentKPCR()->HalReserved[0] = 1;
      Ki386RestoreFlags(Flags);
      break;

    case DISPATCH_LEVEL:
      Ki386SaveFlags(Flags);
      Ki386DisableInterrupts();
      KeGetCurrentKPCR()->HalReserved[1] = 1;
      Ki386RestoreFlags(Flags);
      break;
      
    default:
      KEBUGCHECK(0);
  }
}
