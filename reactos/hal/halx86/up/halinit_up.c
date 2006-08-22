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
  /* FIXME: Big-ass hack. First, should be 0xFFFFFFFF, second, shouldnt' be done here */
  KeGetPcr()->IDR = 0xFFFFFFFA;

  /* Setup busy waiting */
  HalpCalibrateStallExecution();
}

/* EOF */
