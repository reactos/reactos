/* $Id: halinit.c,v 1.16 2000/10/22 13:27:34 ekohl Exp $
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
