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
#include <internal/bitops.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/hal/mps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

extern ULONG DpcQueueSize;

static VOID KeSetCurrentIrql(KIRQL newlvl);

/* FUNCTIONS ****************************************************************/

#define IRQL2TPR(irql) (APIC_TPR_MIN + ((irql - DISPATCH_LEVEL - 1) << 4))

static VOID HiSetCurrentPriority(
  ULONG Priority)
{
//  DPRINT(" P(0x%X) \n", Priority);
  APICWrite(APIC_TPR, Priority & APIC_TPR_PRI);
}


static VOID HiSwitchIrql(KIRQL OldIrql, ULONG Flags)
/*
 * FUNCTION: Switches to the current irql
 * NOTE: Must be called with interrupt disabled
 */
{
   PKTHREAD CurrentThread;
   KIRQL CurrentIrql;

   CurrentIrql = KeGetCurrentKPCR()->Irql;

   if (CurrentIrql == HIGH_LEVEL)
     {
  /* Block all interrupts */
  HiSetCurrentPriority(APIC_TPR_MAX);
	return;
     }

  if (CurrentIrql == IPI_LEVEL)
     {
	HiSetCurrentPriority(APIC_TPR_MAX - 16);
	popfl(Flags);
	return;
     }

  if (CurrentIrql == CLOCK2_LEVEL)
     {
	HiSetCurrentPriority(APIC_TPR_MAX - 32);
	popfl(Flags);
	return;
     }

   if (CurrentIrql > DISPATCH_LEVEL)
     {
	HiSetCurrentPriority(IRQL2TPR(CurrentIrql));
	popfl(Flags);
	return;
     }

   /* Pass all interrupts */
   HiSetCurrentPriority(0);

   if (CurrentIrql == DISPATCH_LEVEL)
     {
	popfl(Flags);
	return;
     }

   if (CurrentIrql == APC_LEVEL)
     {
	if (DpcQueueSize > 0 )
	  {
	     KeSetCurrentIrql(DISPATCH_LEVEL);
	     __asm__("sti\n\t");
	     KiDispatchInterrupt();
	     __asm__("cli\n\t");
	     KeSetCurrentIrql(PASSIVE_LEVEL);
	  }
	popfl(Flags);
	return;
     }

  CurrentThread = KeGetCurrentThread();

  if (CurrentIrql == PASSIVE_LEVEL && 
       CurrentThread != NULL && 
       CurrentThread->ApcState.KernelApcPending)
     {
	KeSetCurrentIrql(APC_LEVEL);
	__asm__("sti\n\t");
	KiDeliverApc(0, 0, 0);
	__asm__("cli\n\t");
	KeSetCurrentIrql(PASSIVE_LEVEL);
	popfl(Flags);
     }
   else
     {
	popfl(Flags);
     }
}


KIRQL STDCALL KeGetCurrentIrql (VOID)
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
   return(KeGetCurrentKPCR()->Irql);
}


static VOID KeSetCurrentIrql(KIRQL newlvl)
/*
 * PURPOSE: Sets the current irq level without taking any action
 */
{
//   DPRINT("KeSetCurrentIrql(newlvl %x)\n",newlvl);

   KeGetCurrentKPCR()->Irql = newlvl;
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
KfLowerIrql (
	KIRQL	NewIrql
	)
{
  KIRQL CurrentIrql;
  KIRQL OldIrql;
  ULONG Flags;

  //DPRINT("KfLowerIrql(NewIrql %d)\n", NewIrql);
  //DbgPrint("KfLowerIrql(NewIrql %d)\n", NewIrql);
  //KeBugCheck(0);

  pushfl(Flags);
  __asm__ ("\n\tcli\n\t");

  CurrentIrql = KeGetCurrentKPCR()->Irql;
  
  if (NewIrql > CurrentIrql)
    {
      DbgPrint ("(%s:%d) NewIrql %x CurrentIrql %x\n",
		__FILE__, __LINE__, NewIrql, CurrentIrql);
      KeBugCheck(0);
      for(;;);
    }
  
  OldIrql = CurrentIrql;
  KeGetCurrentKPCR()->Irql = NewIrql;
  HiSwitchIrql(OldIrql, Flags);
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

VOID
STDCALL
KeLowerIrql (
	KIRQL	NewIrql
	)
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

KIRQL
FASTCALL
KfRaiseIrql (
	KIRQL	NewIrql
	)
{
  KIRQL CurrentIrql;
  KIRQL OldIrql;
  ULONG Flags;

  //DPRINT("KfRaiseIrql(NewIrql %d)\n", NewIrql);
  //DbgPrint("KfRaiseIrql(NewIrql %d)\n", NewIrql);
  //KeBugCheck(0);

  pushfl(Flags);
   __asm__ ("\n\tcli\n\t");

  CurrentIrql = KeGetCurrentKPCR()->Irql;

	if (NewIrql < CurrentIrql)
	{
		DbgPrint ("%s:%d CurrentIrql %x NewIrql %x\n",
		          __FILE__,__LINE__,CurrentIrql,NewIrql);
		KeBugCheck (0);
		for(;;);
	}

	OldIrql = CurrentIrql;
	KeGetCurrentKPCR()->Irql = NewIrql;

  //DPRINT("NewIrql %x OldIrql %x\n", NewIrql, OldIrql);
	HiSwitchIrql(OldIrql, Flags);
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

VOID
STDCALL
KeRaiseIrql (
	KIRQL	NewIrql,
	PKIRQL	OldIrql
	)
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

KIRQL
STDCALL
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

KIRQL
STDCALL
KeRaiseIrqlToSynchLevel (VOID)
{
	return KfRaiseIrql (CLOCK2_LEVEL);
}


BOOLEAN STDCALL HalBeginSystemInterrupt (ULONG Vector,
					 KIRQL Irql,
					 PKIRQL OldIrql)
{
  DPRINT("Vector (0x%X)  Irql (0x%X)\n",
    Vector, Irql);

  if (Vector < FIRST_DEVICE_VECTOR ||
    Vector > FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS) {
    DPRINT("Not a device interrupt\n");
	  return FALSE;
  }

  /*
   * Acknowledge the interrupt
   */
  APICSendEOI();

  *OldIrql = KeGetCurrentIrql();

  KeSetCurrentIrql(Irql);

  return TRUE;
}


VOID STDCALL HalEndSystemInterrupt (KIRQL Irql,
				    ULONG Unknown2)
{
   KeSetCurrentIrql(Irql);
}


BOOLEAN STDCALL HalDisableSystemInterrupt (ULONG Vector,
					   ULONG Unknown2)
{
  ULONG irq;

  DPRINT("Vector (0x%X)\n", Vector);

  if (Vector < FIRST_DEVICE_VECTOR ||
    Vector > FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS)  {
    DPRINT("Not a device interrupt\n");
	  return FALSE;
  }

  irq = VECTOR2IRQ(Vector);

  IOAPICMaskIrq(0, irq);

  return TRUE;
}


BOOLEAN STDCALL HalEnableSystemInterrupt (ULONG Vector,
					  ULONG Unknown2,
					  ULONG Unknown3)
{
  ULONG irq;

  DPRINT("Vector (0x%X)\n", Vector);

  if (Vector < FIRST_DEVICE_VECTOR ||
    Vector > FIRST_DEVICE_VECTOR + NUMBER_DEVICE_VECTORS) {
    DPRINT("Not a device interrupt\n");
	  return FALSE;
  }

  irq = VECTOR2IRQ(Vector);

  IOAPICUnmaskIrq(0, irq);

  return TRUE;
}

/* EOF */
