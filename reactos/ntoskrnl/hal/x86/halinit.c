/* $Id: halinit.c,v 1.12 2000/07/01 18:23:06 ekohl Exp $
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
#include <internal/ke.h>
#include <internal/ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN
STDCALL
HalInitSystem (
	ULONG			BootPhase,
	PLOADER_PARAMETER_BLOCK	LoaderBlock
	)
{
   if (BootPhase == 0)
   {
      HalInitializeDisplay (LoaderBlock);
      HalpCalibrateStallExecution ();
      KeInitExceptions ();
      HalpInitIRQs ();
      KeLowerIrql(DISPATCH_LEVEL);
   }
   else
   {
      HalpInitBusHandlers ();

   }

   return TRUE;
}

/* EOF */
