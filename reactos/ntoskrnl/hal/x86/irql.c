/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/irql.c
 * PURPOSE:         Implements IRQLs
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/bitops.h>
#include <internal/ke.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

/* FIXME: this should be in a header file */
#define NR_IRQS         (16)
#define IRQ_BASE        (0x40)

/*
 * PURPOSE: Current irq level
 */
static KIRQL CurrentIrql = HIGH_LEVEL;

extern ULONG DpcQueueSize;

static VOID KeSetCurrentIrql(KIRQL newlvl);

#define DIRQL_TO_IRQ(x)  (PROFILE_LEVEL - x)
#define IRQ_TO_DIRQL(x)  (PROFILE_LEVEL - x)

/* FUNCTIONS ****************************************************************/

VOID HalpInitPICs(VOID)
{
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
   /* Mask off all interrupts from PICs */
   WRITE_PORT_UCHAR((PUCHAR)0x21, 0xff);
   WRITE_PORT_UCHAR((PUCHAR)0xa1, 0xff);
}

static ULONG 
HiSetCurrentPICMask(unsigned int mask)
{
  WRITE_PORT_UCHAR((PUCHAR)0x21,mask & 0xff);
  WRITE_PORT_UCHAR((PUCHAR)0xa1,(mask >> 8) & 0xff);
  
  return mask;
}

static VOID HiSwitchIrql(KIRQL oldIrql)
/*
 * FUNCTION: Switches to the current irql
 * NOTE: Must be called with interrupt disabled
 */
{
   unsigned int i;
   PKTHREAD CurrentThread;
   
   CurrentThread = KeGetCurrentThread();
   
   /*
    * Disable all interrupts
    */
   if (CurrentIrql >= IPI_LEVEL)
     {
       HiSetCurrentPICMask(0xFFFF);
       __asm__("sti\n\t");
       return;
     }

   /*
    * Disable all interrupts but the timer
    */
   if (CurrentIrql == PROFILE_LEVEL ||
       CurrentIrql == CLOCK1_LEVEL ||
       CurrentIrql == CLOCK2_LEVEL)
     {
	HiSetCurrentPICMask(0xFFFE);
	__asm__("sti\n\t");
	return;
     }

   /*
    * Disable all interrupts of lesser priority
    */
   if (CurrentIrql > DISPATCH_LEVEL)
     {
       unsigned int current_mask = 0;
	
       for (i = CurrentIrql; i > (PROFILE_LEVEL - 15); i--)
	  {
	     current_mask = current_mask | (1 << (PROFILE_LEVEL - i));
	  }
	
       HiSetCurrentPICMask(current_mask);
       __asm__("sti\n\t");
       return;
     }
   
   /*
    * Enable all interrupts
    */
   if (CurrentIrql == DISPATCH_LEVEL)
     {
	HiSetCurrentPICMask(0);
	__asm__("sti\n\t");
	return;
     }
   
   /*
    * APCs are disabled but execute any pending DPCs
    */
   if (CurrentIrql == APC_LEVEL)
     {
       HiSetCurrentPICMask(0);
       __asm__("sti\n\t");
       if (DpcQueueSize > 0)
	  {
	    CurrentIrql = DISPATCH_LEVEL;
	    KiDispatchInterrupt();
	    CurrentIrql = APC_LEVEL;
	  }
	return;
     }
   
   /*
    * Execute any pending DPCs or APCs
    */
   if (CurrentIrql == PASSIVE_LEVEL)
     {
       if (DpcQueueSize > 0)
	 {
	   CurrentIrql = DISPATCH_LEVEL;
	   KiDispatchInterrupt();
	   CurrentIrql = PASSIVE_LEVEL;
	 }
       if (CurrentThread != NULL && 
	   CurrentThread->ApcState.KernelApcPending)
	 {
	   CurrentIrql = APC_LEVEL;
	   KiDeliverApc(0, 0, 0);
	   CurrentIrql = PASSIVE_LEVEL;
	 }
     }
}


KIRQL STDCALL 
KeGetCurrentIrql (VOID)
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
   return(CurrentIrql);
}


STATIC VOID 
KeSetCurrentIrql(KIRQL newlvl)
/*
 * PURPOSE: Sets the current irq level without taking any action
 */
{
  CurrentIrql = newlvl;
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
  
  __asm__("cli\n\t");
  
  DPRINT("KfLowerIrql(NewIrql %d)\n", NewIrql);
  
  if (NewIrql > CurrentIrql)
    {
      DbgPrint ("(%s:%d) NewIrql %x CurrentIrql %x\n",
		__FILE__, __LINE__, NewIrql, CurrentIrql);
      KeBugCheck(0);
      for(;;);
    }
  
  OldIrql = CurrentIrql;
  CurrentIrql = NewIrql;
  HiSwitchIrql(OldIrql);
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
  
  __asm__("cli\n\t");
  OldIrql = CurrentIrql;
  CurrentIrql = NewIrql;
  
  DPRINT ("NewIrql %x OldIrql %x CurrentIrql %x\n",
	  NewIrql, OldIrql, CurrentIrql);
  HiSwitchIrql(OldIrql);
  
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
  //	return KfRaiseIrql (CLOCK2_LEVEL);
  UNIMPLEMENTED;
}


BOOLEAN STDCALL 
HalBeginSystemInterrupt (ULONG Vector,
			 KIRQL Irql,
			 PKIRQL OldIrql)
{
  if (Vector < IRQ_BASE || Vector > IRQ_BASE + NR_IRQS)
    return FALSE;
  
  /* Send EOI to the PICs */
  WRITE_PORT_UCHAR((PUCHAR)0x20,0x20);
  if ((Vector-IRQ_BASE)>=8)
    {
      WRITE_PORT_UCHAR((PUCHAR)0xa0,0x20);
    }
  
  *OldIrql = KeGetCurrentIrql();
  if (Vector-IRQ_BASE != 0)
    {
      DPRINT("old_level %d\n",*OldIrql);
    }
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
