/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/halx86/mp/processor_mp.c
 * PURPOSE:         Intel MultiProcessor specification support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           Parts adapted from linux SMP code
 * UPDATE HISTORY:
 *     22/05/1998  DW   Created
 *     12/04/2001  CSH  Added MultiProcessor specification support
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID NTAPI
HalInitializeProcessor(ULONG ProcessorNumber,
                       PLOADER_PARAMETER_BLOCK LoaderBlock)
{
   ULONG CPU;

   DPRINT("HalInitializeProcessor(%x %x)\n", ProcessorNumber, LoaderBlock);

   CPU = ThisCPU();
   if (OnlineCPUs & (1 << CPU))
   {
      ASSERT(FALSE);
   }

   if (ProcessorNumber == 0)
   {
      HaliInitBSP();
   }
   else
   {
      APICSetup();

      DPRINT("CPU %d says it is now booted.\n", CPU);
 
      APICCalibrateTimer(CPU);
   }

   /* This processor is now booted */
   CPUMap[CPU].Flags |= CPU_ENABLED;
   OnlineCPUs |= (1 << CPU);

   /* Setup busy waiting */
   //HalpCalibrateStallExecution();
}

BOOLEAN NTAPI
HalAllProcessorsStarted (VOID)
{
    ULONG CPUs = 0, i;

    DPRINT("HalAllProcessorsStarted()\n");
    for (i = 0; i < 32; i++)
    {
       if (OnlineCPUs & (1 << i))
       {
          CPUs++;
       }
    }
    if (CPUs > CPUCount)
    {
       ASSERT(FALSE);
    }
    else if (CPUs == CPUCount)
    {

       IOAPICEnable();
       IOAPICSetupIds();
       if (CPUCount > 1)
       {
          APICSyncArbIDs();
       }
       IOAPICSetupIrqs();

       return TRUE;
    }
    return FALSE;
}

BOOLEAN
NTAPI
HalStartNextProcessor(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState)
{
   ULONG CPU;

   DPRINT("HalStartNextProcessor(%x %x)\n", LoaderBlock, ProcessorState);

   for (CPU = 0; CPU < CPUCount; CPU++)
   {
      if (!(OnlineCPUs & (1<<CPU)))
      {
         break;
      }
   }

   if (CPU >= CPUCount)
   {
      ASSERT(FALSE);
   }

   DPRINT1("Attempting to boot CPU %d\n", CPU);

   HaliStartApplicationProcessor(CPU, (ULONG)ProcessorState);

   return TRUE;
}

VOID
NTAPI
HalProcessorIdle(VOID)
{
    UNIMPLEMENTED;
}

/* EOF */
