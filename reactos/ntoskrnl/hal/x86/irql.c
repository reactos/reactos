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
#include <internal/halio.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

/*
 * PURPOSE: Current irq level
 */
static KIRQL CurrentIrql = HIGH_LEVEL;

extern ULONG DpcQueueSize;

/* FUNCTIONS ****************************************************************/

#if 0
static unsigned int HiGetCurrentPICMask(void)
{
   unsigned int mask;
   
   mask = inb_p(0x21);
   mask = mask | (inb_p(0xa1)<<8);

   return mask;
}
#endif

static unsigned int HiSetCurrentPICMask(unsigned int mask)
{
   outb_p(0x21,mask & 0xff);
   outb_p(0xa1,(mask >> 8) & 0xff);

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
   
   if (CurrentIrql == HIGH_LEVEL)
     {
	HiSetCurrentPICMask(0xffff);
	return;
     }
   if (CurrentIrql > DISPATCH_LEVEL)
     {
	unsigned int current_mask = 0;
	
	for (i=(CurrentIrql-DISPATCH_LEVEL);i>DISPATCH_LEVEL;i--)
	  {
	     set_bit(NR_DEVICE_SPECIFIC_LEVELS - i,&current_mask);
	  }
	
	HiSetCurrentPICMask(current_mask);
	__asm__("sti\n\t");
	return;
     }
   
   if (CurrentIrql == DISPATCH_LEVEL)
     {
	HiSetCurrentPICMask(0);
	__asm__("sti\n\t");
	return;
     }
   
   HiSetCurrentPICMask(0);
   if (CurrentIrql == APC_LEVEL)
     {
	if (DpcQueueSize > 0 )
	  {
	     KeSetCurrentIrql(DISPATCH_LEVEL);
	     __asm__("sti\n\t");
	     KeDrainDpcQueue();
	     __asm__("cli\n\t");
	     KeSetCurrentIrql(PASSIVE_LEVEL);
	  }
	__asm__("sti\n\t");
	return;
     }
   
   if (CurrentIrql == PASSIVE_LEVEL && 
       CurrentThread != NULL && 
       CurrentThread->ApcState.KernelApcPending)
     {
	KeSetCurrentIrql(APC_LEVEL);
	__asm__("sti\n\t");
	KeCallApcsThread();
	__asm__("cli\n\t");
	KeSetCurrentIrql(PASSIVE_LEVEL);
	__asm__("sti\n\t");
     }
   else
     {
	__asm__("sti\n\t");
     }
}

VOID KeSetCurrentIrql(KIRQL newlvl)
/*
 * PURPOSE: Sets the current irq level without taking any action
 */
{
//   DPRINT("KeSetCurrentIrql(newlvl %x)\n",newlvl);
   CurrentIrql = newlvl;
}

KIRQL KeGetCurrentIrql()
/*
 * PURPOSE: Returns the current irq level
 * RETURNS: The current irq level
 */
{
   return(CurrentIrql);
}

VOID KeLowerIrql(KIRQL NewIrql)
/*
 * PURPOSE: Restores the irq level on the current processor
 * ARGUMENTS:
 *        NewIrql = Irql to lower to
 */
{
   KIRQL oldIrql;
   
//   DbgPrint(">");
   
   __asm__("cli\n\t");
   
   DPRINT("KeLowerIrql(NewIrql %d)\n", NewIrql);
//   DbgPrint("{");
//   KeDumpStackFrames(0,32);
//   DbgPrint("}\n");
   //DPRINT("NewIrql %x CurrentIrql %x\n",NewIrql,CurrentIrql);
   if (NewIrql > CurrentIrql)
     {
	DbgPrint("(%s:%d) NewIrql %x CurrentIrql %x\n",
		 __FILE__, __LINE__, NewIrql,CurrentIrql);
	KeDumpStackFrames(0,32);
	for(;;);
     }
   oldIrql = CurrentIrql;
   CurrentIrql = NewIrql;
   HiSwitchIrql(oldIrql);
}

VOID KeRaiseIrql(KIRQL NewIrql, PKIRQL OldIrql)
/*
 * FUNCTION: Raises the hardware priority (irql) 
 * ARGUMENTS:
 *         NewIrql = Irql to raise to
 *         OldIrql (OUT) = Caller supplied storage for the previous irql
 */
{
   /*
    * sanity check
    */
   
//   DbgPrint("<");
   
//  DPRINT("CurrentIrql %x NewIrql %x OldIrql %x\n",CurrentIrql,NewIrql,
//	 OldIrql);
//   DbgPrint("{");
//   KeDumpStackFrames(0,32);
//   DbgPrint("}\n");
   DPRINT("KeRaiseIrql(NewIrql %d, OldIrql %x, *OldIrql %d)\n",
	    NewIrql, OldIrql, *OldIrql);
   if (NewIrql < CurrentIrql)
     {
	DbgPrint("%s:%d CurrentIrql %x NewIrql %x OldIrql %x\n",
		 __FILE__,__LINE__,CurrentIrql,NewIrql,OldIrql);
	KeBugCheck(0);
	for(;;);
     }
   
   __asm__("cli\n\t");
   *OldIrql = CurrentIrql;
   CurrentIrql = NewIrql;
//   *OldIrql = InterlockedExchange(&CurrentIrql,NewIrql);
   DPRINT("NewIrql %x OldIrql %x CurrentIrql %x\n",NewIrql,*OldIrql,
          CurrentIrql);
   HiSwitchIrql(*OldIrql);
}



