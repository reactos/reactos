/* $Id: halinit.c,v 1.7 1999/12/12 03:48:47 phreak Exp $
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
HalInitSystem (ULONG Phase, boot_param *bp)
{
   if (Phase == 0)
   {
      HalInitializeDisplay (bp);
   }
   else
   {
      KeInitExceptions();
      KeInitIRQ();
      KeLowerIrql(DISPATCH_LEVEL);
   
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
