/* $Id: halinit_up.c,v 1.1 2004/12/03 20:10:45 gvg Exp $
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

#include <ddk/ntddk.h>
#include <hal.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID
HalpInitPhase0(VOID)
{
  HalpInitPICs();

  /* Setup busy waiting */
  HalpCalibrateStallExecution();
}

/* EOF */
