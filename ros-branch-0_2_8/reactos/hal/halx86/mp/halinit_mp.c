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

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

extern BOOLEAN HaliFindSmpConfig(VOID);
ULONG_PTR KernelBase;

/***************************************************************************/
VOID
HalpInitPhase0(PLOADER_PARAMETER_BLOCK LoaderBlock)

{
   static BOOLEAN MPSInitialized = FALSE;


   /* Only initialize MP system once. Once called the first time,
      each subsequent call is part of the initialization sequence
      for an application processor. */

   DPRINT("HalpInitPhase0()\n");


   if (MPSInitialized) 
   {
      KEBUGCHECK(0);
   }

   MPSInitialized = TRUE;

   if (!HaliFindSmpConfig())
   {
      KEBUGCHECK(0);
   }

   /* store the kernel base for later use */
   KernelBase = ((PLOADER_MODULE)LoaderBlock->ModsAddr)[0].ModStart;

}

/* EOF */
