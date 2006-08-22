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
#include <ndk/asm.h>

/* GLOBALS ******************************************************************/

UCHAR Table[8] =
{
    0, 0,
    1, 1,
    2, 2, 2, 2
};

/* FUNCTIONS ****************************************************************/

extern ULONG KiI8259MaskTable[];

VOID HalpEndSystemInterrupt(KIRQL Irql)
/*
 * FUNCTION: Enable all irqs with higher priority.
 */
{
  ULONG flags;
  ULONG Mask;

  /* Interrupts should be disable while enabling irqs of both pics */
  Ki386SaveFlags(flags);
  Ki386DisableInterrupts();

  Mask = KeGetPcr()->IDR | KiI8259MaskTable[Irql];
  WRITE_PORT_UCHAR((PUCHAR)0x21,  (UCHAR)Mask);
  Mask >>= 8;
  WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)Mask);

  /* restore flags */
  Ki386RestoreFlags(flags);
}

VOID STATIC
HalpLowerIrql(KIRQL NewIrql)
{
    ULONG Mask;

    if (KeGetPcr()->Irql > DISPATCH_LEVEL)
    {
        Mask = KeGetPcr()->IDR | KiI8259MaskTable[NewIrql];
        WRITE_PORT_UCHAR((PUCHAR)0x21,  (UCHAR)Mask);
        Mask >>= 8;
        WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)Mask);
    }
  if (NewIrql >= PROFILE_LEVEL)
    {
      KeGetPcr()->Irql = NewIrql;
      return;
    }
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeGetPcr()->Irql = NewIrql;
      return;
    }
  KeGetPcr()->Irql = DISPATCH_LEVEL;
  if (Table[KeGetPcr()->IRR] >= NewIrql)
    {
      KeGetPcr()->IRR &= ~4;
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


VOID STDCALL HalEndSystemInterrupt (KIRQL Irql, ULONG Unknown2)
/*
 * FUNCTION: Finish a system interrupt and restore the specified irq level.
 */
{
    //DPRINT1("ENDING: %lx %lx\n", Irql, Unknown2);
  HalpLowerIrql(Irql);
  HalpEndSystemInterrupt(Irql);
}

/* EOF */
