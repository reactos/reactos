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

#define NDEBUG
#include <hal.h>

/* FUNCTIONS ***************************************************************/

VOID
HalpInitPhase0(VOID)
{
  HalpInitPICs();

  /* Setup busy waiting */
  HalpCalibrateStallExecution();
}

/* EOF */
