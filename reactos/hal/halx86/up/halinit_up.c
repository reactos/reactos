/* $Id$
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/hal/x86/halinit.c
 * PURPOSE:       Initalize the x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

VOID NTAPI HalpClockInterrupt(VOID);

/* FUNCTIONS ***************************************************************/

VOID
HalpInitPhase0(PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
  HalpInitPICs();

  /* Enable the clock interrupt */
#if 0
  ((PKIPCR)KeGetPcr())->IDT[IRQ2VECTOR(0)].ExtendedOffset =
      (USHORT)(((ULONG_PTR)HalpClockInterrupt >> 16) & 0xFFFF);
  ((PKIPCR)KeGetPcr())->IDT[IRQ2VECTOR(0)].Offset =
      (USHORT)HalpClockInterrupt;
#endif
  HalEnableSystemInterrupt(IRQ2VECTOR(0), CLOCK2_LEVEL, Latched);

  /* Setup busy waiting */
  HalpCalibrateStallExecution();
}

/* EOF */
