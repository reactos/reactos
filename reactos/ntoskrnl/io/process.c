/* $Id: process.c,v 1.8 2000/07/04 01:29:05 ekohl Exp $
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
	PULONG	LowLimit,
	PULONG	HighLimit
	)
{
	*LowLimit = (ULONG)NtCurrentTeb ()->Tib.StackLimit;
	*HighLimit = (ULONG)NtCurrentTeb ()->Tib.StackBase;
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
