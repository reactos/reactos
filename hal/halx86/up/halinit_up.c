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

/* FUNCTIONS ***************************************************************/

VOID
HalpInitPhase0(PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
  HalpInitPICs();

  /* Setup busy waiting */
  HalpCalibrateStallExecution();
}

VOID
HalpInitPhase1(VOID)
{
    /* Enable the clock interrupt */
    ((PKIPCR)KeGetPcr())->IDT[0x30].ExtendedOffset =
        (USHORT)(((ULONG_PTR)HalpClockInterrupt >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[0x30].Offset =
        (USHORT)HalpClockInterrupt;
    HalEnableSystemInterrupt(0x30, CLOCK2_LEVEL, Latched);
}

/* EOF */
