/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          hal/halx86/mp/halinit_mp.c
 * PURPOSE:       Initialize the x86 mp hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

extern BOOLEAN HaliFindSmpConfig(VOID);
ULONG_PTR KernelBase;

/* FUNCTIONS ****************************************************************/

VOID NTAPI HalpInitializePICs(IN BOOLEAN EnableInterrupts)
{
    UNIMPLEMENTED;
}

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
      ASSERT(FALSE);
   }

   MPSInitialized = TRUE;

   if (!HaliFindSmpConfig())
   {
      ASSERT(FALSE);
   }

   /* store the kernel base for later use */
   KernelBase = (ULONG_PTR)CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks)->DllBase;

}

VOID
HalpInitPhase1(VOID)
{
}

/* EOF */
