/* $Id: halinit.c,v 1.18 2001/04/13 16:12:25 chorns Exp $
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
#include <internal/config.h>
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
   else
   {
      HalpInitBusHandlers ();
   }

   return TRUE;
}

/* EOF */
