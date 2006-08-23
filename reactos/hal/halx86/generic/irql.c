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

typedef VOID (*PSW_HANDLER)(VOID);
extern PSW_HANDLER SoftIntHandlerTable[];

/* FUNCTIONS ****************************************************************/

extern ULONG KiI8259MaskTable[];

VOID STATIC
HalpLowerIrql(KIRQL NewIrql)
{
    ULONG Mask;
    ULONG Flags;

    Ki386SaveFlags(Flags);
    Ki386DisableInterrupts();

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
      Ki386RestoreFlags(Flags);
      return;
    }
  if (NewIrql >= DISPATCH_LEVEL)
    {
      KeGetPcr()->Irql = NewIrql;
      Ki386RestoreFlags(Flags);
      return;
    }
  KeGetPcr()->Irql = NewIrql;
  if (Table[KeGetPcr()->IRR] > NewIrql)
    {
              SoftIntHandlerTable[Table[KeGetPcr()->IRR]]();
    }
  Ki386RestoreFlags(Flags);
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
}

/* EOF */
