/* $Id: process.c,v 1.6 2000/06/12 14:57:10 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/process.c
 * PURPOSE:         Process functions that, bizarrely, are in the iomgr
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PEPROCESS
STDCALL
IoGetCurrentProcess (
	VOID
	)
{
	return (PsGetCurrentProcess());
}


PVOID
STDCALL
IoGetInitialStack (
	VOID
	)
{
   UNIMPLEMENTED;
}


VOID
STDCALL
IoGetStackLimits (
	PVOID	*StackLimit,
	PVOID	*StackBase
	)
{
	*StackLimit = NtCurrentTeb ()->Tib.StackLimit;
	*StackBase = NtCurrentTeb ()->Tib.StackBase;
}


PEPROCESS
STDCALL
IoThreadToProcess (
	IN	PETHREAD	Thread
	)
{
	return (Thread->ThreadsProcess);
}


PEPROCESS
STDCALL
IoGetRequestorProcess (
	IN	PIRP	Irp
	)
{
	return (Irp->Tail.Overlay.Thread->ThreadsProcess);
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
 */
BOOLEAN
STDCALL
EXPORTED
IoSetThreadHardErrorMode (
	IN	BOOLEAN	HardErrorEnabled
	)
{
	BOOLEAN PreviousHEM = NtCurrentTeb ()->HardErrorDisabled;
	
	NtCurrentTeb ()->HardErrorDisabled = (
		(TRUE == HardErrorEnabled)
		 ? FALSE
		 : TRUE
		 );
	return (
		(TRUE == PreviousHEM)
			? FALSE
			: TRUE
			);
}


/* EOF */
