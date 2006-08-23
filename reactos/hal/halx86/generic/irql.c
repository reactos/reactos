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

typedef VOID (*PSW_HANDLER)(VOID);
extern PSW_HANDLER SoftIntHandlerTable[];
extern ULONG KiI8259MaskTable[];
extern UCHAR SoftIntByteTable[];

/* FUNCTIONS ****************************************************************/

VOID FASTCALL
KfLowerIrql (KIRQL	NewIrql)
{
    ULONG Mask;
    ULONG Flags;
    UCHAR Pending;

    Ki386SaveFlags(Flags);
    Ki386DisableInterrupts();

    if (KeGetPcr()->Irql > DISPATCH_LEVEL)
    {
        Mask = KeGetPcr()->IDR | KiI8259MaskTable[NewIrql];
        WRITE_PORT_UCHAR((PUCHAR)0x21,  (UCHAR)Mask);
        Mask >>= 8;
        WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)Mask);
    }


  KeGetPcr()->Irql = NewIrql;
  Pending = SoftIntByteTable[KeGetPcr()->IRR];
  if (Pending > NewIrql)
    {
              SoftIntHandlerTable[Pending]();
    }
  Ki386RestoreFlags(Flags);
}

VOID STDCALL HalEndSystemInterrupt (KIRQL Irql, ULONG Unknown2)
{
    ULONG Mask;
    ULONG Flags;
    UCHAR Pending;

    Ki386SaveFlags(Flags);
    Ki386DisableInterrupts();

    if (KeGetPcr()->Irql > DISPATCH_LEVEL)
    {
        Mask = KeGetPcr()->IDR | KiI8259MaskTable[Irql];
        WRITE_PORT_UCHAR((PUCHAR)0x21,  (UCHAR)Mask);
        Mask >>= 8;
        WRITE_PORT_UCHAR((PUCHAR)0xa1, (UCHAR)Mask);
    }


  KeGetPcr()->Irql = Irql;
  Pending = SoftIntByteTable[KeGetPcr()->IRR];
  if (Pending > Irql)
    {
              SoftIntHandlerTable[Pending]();
    }
  Ki386RestoreFlags(Flags);
}


/* EOF */
