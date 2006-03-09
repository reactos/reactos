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

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/;


/* FUNCTIONS ****************************************************************/

#undef KeGetCurrentIrql
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

  irql = Ki386ReadFsByte(FIELD_OFFSET(KPCR, Irql));
  if (irql > HIGH_LEVEL)
    {
      DPRINT1 ("CurrentIrql %x\n", irql);
      KEBUGCHECK (0);
    }
  if (Flags & X86_EFLAGS_IF)
    {
      Ki386EnableInterrupts();
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
    KEBUGCHECK (0);
  }
  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();
  Ki386WriteFsByte(FIELD_OFFSET(KPCR, Irql), NewIrql);
  if (Flags & X86_EFLAGS_IF)
    {
      Ki386EnableInterrupts();
    }
}

VOID 
HalpLowerIrql(KIRQL NewIrql, BOOL FromHalEndSystemInterrupt)
{
  ULONG Flags;
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeSetCurrentIrql (NewIrql);
      APICWrite(APIC_TPR, IRQL2TPR (NewIrql) & APIC_TPR_PRI);
      return;
    }
  Ki386SaveFlags(Flags);
  if (KeGetCurrentIrql() > APC_LEVEL)
    {
      KeSetCurrentIrql (DISPATCH_LEVEL);
      APICWrite(APIC_TPR, IRQL2TPR (DISPATCH_LEVEL) & APIC_TPR_PRI);
      if (FromHalEndSystemInterrupt || Ki386ReadFsByte(FIELD_OFFSET(KIPCR, HalReserved[HAL_DPC_REQUEST])))
        {
          Ki386WriteFsByte(FIELD_OFFSET(KIPCR, HalReserved[HAL_DPC_REQUEST]), 0);
          Ki386EnableInterrupts();
          KiDispatchInterrupt();
          if (!(Flags & X86_EFLAGS_IF))
            {
              Ki386DisableInterrupts();
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
      Ki386EnableInterrupts();
      KiDeliverApc(KernelMode, NULL, NULL);
      if (!(Flags & X86_EFLAGS_IF))
        {
          Ki386DisableInterrupts();
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
      KEBUGCHECK (0);
    }
  HalpLowerIrql (NewIrql, FALSE);
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
#undef KeLowerIrql
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
 
  Ki386SaveFlags(Flags);
  Ki386DisableInterrupts();

  OldIrql = KeGetCurrentIrql ();

  if (NewIrql < OldIrql)
    {
      DPRINT1 ("CurrentIrql %x NewIrql %x\n", KeGetCurrentIrql (), NewIrql);
      KEBUGCHECK (0);
    }


  if (NewIrql > DISPATCH_LEVEL)
    {
      APICWrite (APIC_TPR, IRQL2TPR(NewIrql) & APIC_TPR_PRI);
    }
  KeSetCurrentIrql (NewIrql);
  if (Flags & X86_EFLAGS_IF)
    {
      Ki386EnableInterrupts();
    }

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
#undef KeRaiseIrql
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

  Ki386SaveFlags(Flags);
  if (Flags & X86_EFLAGS_IF)
  {
     DPRINT1("HalBeginSystemInterrupt was called with interrupt's enabled\n");
     KEBUGCHECK(0);
  }
  APICWrite (APIC_TPR, IRQL2TPR (Irql) & APIC_TPR_PRI);
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
  ULONG Flags;
  Ki386SaveFlags(Flags);

  if (Flags & X86_EFLAGS_IF)
  {
     DPRINT1("HalEndSystemInterrupt was called with interrupt's enabled\n");
     KEBUGCHECK(0);
  }
  APICSendEOI();
  HalpLowerIrql (Irql, TRUE);
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
      Ki386WriteFsByte(FIELD_OFFSET(KIPCR, HalReserved[HAL_APC_REQUEST]), 1);
      break;

    case DISPATCH_LEVEL:
      Ki386WriteFsByte(FIELD_OFFSET(KIPCR, HalReserved[HAL_DPC_REQUEST]), 1);
      break;
      
    default:
      KEBUGCHECK(0);
  }
}
