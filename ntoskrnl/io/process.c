/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/process.c
 * PURPOSE:         Process functions that, bizarrely, are in the iomgr
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PVOID STDCALL
IoGetInitialStack(VOID)
{
  return(PsGetCurrentThread()->Tcb.InitialStack);
}


/*
 * @implemented
 */
VOID STDCALL
IoGetStackLimits(OUT PULONG LowLimit,
		 OUT PULONG HighLimit)
{
  *LowLimit = (ULONG)NtCurrentTeb()->Tib.StackLimit;
  *HighLimit = (ULONG)NtCurrentTeb()->Tib.StackBase;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
IoIsSystemThread(
    IN PETHREAD Thread
    )
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @implemented
 */
PEPROCESS STDCALL
IoThreadToProcess(IN PETHREAD Thread)
{
  return(Thread->ThreadsProcess);
}


/*
 * @implemented
 */
PEPROCESS STDCALL
IoGetRequestorProcess(IN PIRP Irp)
{
  return(Irp->Tail.Overlay.Thread->ThreadsProcess);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoSetThreadHardErrorMode@4
 *
 * ARGUMENTS
 * 	HardErrorEnabled
 * 		TRUE : enable hard errors processing;
 * 		FALSE: do NOT process hard errors.
 *
 * RETURN VALUE
 * 	Previous value for the current thread's hard errors
 * 	processing policy.
 *
 * @implemented
 */
BOOLEAN STDCALL
IoSetThreadHardErrorMode(IN BOOLEAN HardErrorEnabled)
{
  BOOLEAN PreviousHEM = (BOOLEAN)(NtCurrentTeb()->HardErrorDisabled);

  NtCurrentTeb()->HardErrorDisabled = ((TRUE == HardErrorEnabled) ? FALSE : TRUE);

  return((TRUE == PreviousHEM) ? FALSE : TRUE);
}

/* EOF */
