/* $Id: halinit.c,v 1.10 2000/04/08 19:08:50 ekohl Exp $
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
	ULONG		Phase,
	boot_param	*bp
	)
{
   if (Phase == 0)
   {
      HalInitializeDisplay (bp);
      HalpCalibrateStallExecution ();
      KeInitExceptions();
      KeInitIRQ();
      KeLowerIrql(DISPATCH_LEVEL);
   }
   else
   {
      HalpInitBusHandlers ();

      /*
       * Probe for a BIOS32 extension
       */
      Hal_bios32_probe();
   
      /*
       * Probe for buses attached to the computer
       * NOTE: Order is important here because ISA is the default
       */
#if 0
      if (HalPciProbe())
      {
           return TRUE;
      }
      HalIsaProbe();
#endif
   }

   return TRUE;
}

/* EOF */
