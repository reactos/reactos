/* $Id: halinit.c,v 1.15 2000/08/30 19:33:28 dwelch Exp $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/hal/x86/halinit.c
 * PURPOSE:       Initalize the uniprocessor, x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal.h>
#include <internal/ntoskrnl.h>
#include <internal/halio.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN STDCALL
HalInitSystem (ULONG			BootPhase,
	       PLOADER_PARAMETER_BLOCK	LoaderBlock)
{
   if (BootPhase == 0)
   {
      HalInitializeDisplay (LoaderBlock);
      HalpInitPICs ();
   }
   else
   {
      HalpInitBusHandlers ();

   }

   return TRUE;
}

/* EOF */
