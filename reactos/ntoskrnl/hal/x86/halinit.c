/* $Id: halinit.c,v 1.23 2001/08/30 20:38:18 dwelch Exp $
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
#include <roscfg.h>
#include <internal/hal/hal.h>
#include <internal/ntoskrnl.h>

#ifdef MP
#include <internal/hal/mps.h>
#endif /* MP */

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ***************************************************************/

BOOLEAN STDCALL
HalInitSystem (ULONG BootPhase,
               PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  if (BootPhase == 0)
    {
      HalInitializeDisplay (LoaderBlock);
      
#ifdef MP
      
      HalpInitMPS();
      
#else

      HalpInitPICs();

      /* Setup busy waiting */
      HalpCalibrateStallExecution();

#endif /* MP */

    }
  else if (BootPhase == 1)
    {
      HalpInitBusHandlers ();
    }
  else
    {
      /* Enumerate the devices on the motherboard */
      HalpStartEnumerator();
    }

  return TRUE;
}

/* EOF */
