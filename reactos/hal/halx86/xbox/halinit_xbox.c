/* $Id: halinit_xbox.c,v 1.2 2004/12/04 22:52:59 gvg Exp $
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
#include "halxbox.h"

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

VOID
HalpInitPhase0(VOID)
{
  HalpHooks.InitPciBus = HalpXboxInitPciBus;

  HalpInitPICs();

  /* Setup busy waiting */
  HalpCalibrateStallExecution();
}

/* EOF */
