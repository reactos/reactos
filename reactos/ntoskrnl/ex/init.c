/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/init.c
 * PURPOSE:         Executive initalization
 * 
 * PROGRAMMERS:     Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
ExInit2(VOID)
{
  ExpInitLookasideLists();
}

VOID INIT_FUNCTION
ExInit3 (VOID)
{
  ExInitializeWorkerThreads();
  ExpInitializeEventImplementation();
  ExpInitializeEventPairImplementation();
  ExpInitializeMutantImplementation();
  ExpInitializeSemaphoreImplementation();
  ExpInitializeTimerImplementation();
  LpcpInitSystem();
  ExpInitializeProfileImplementation();
  ExpWin32kInit();
  ExpInitUuids();
}


/*
 * @implemented
 */
BOOLEAN STDCALL
ExIsProcessorFeaturePresent(IN ULONG ProcessorFeature)
{
  if (ProcessorFeature >= PROCESSOR_FEATURE_MAX)
    return(FALSE);

  return(SharedUserData->ProcessorFeatures[ProcessorFeature]);
}


VOID STDCALL
ExPostSystemEvent (ULONG	Unknown1,
		   ULONG	Unknown2,
		   ULONG	Unknown3)
{
  /* doesn't do anything */
}

/* EOF */
