/* $Id: process.c,v 1.3 2000/05/13 01:48:59 ekohl Exp $
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
	*StackLimit = NtCurrentTeb ()->StackLimit;
	*StackBase = NtCurrentTeb ()->StackBase;
}


PEPROCESS
STDCALL
IoThreadToProcess (
	IN	PETHREAD	Thread
	)
{
	UNIMPLEMENTED;
	return (NULL);
}


PEPROCESS
STDCALL
IoGetRequestorProcess (
	IN	PIRP	Irp
	)
{
	return (Irp->Tail.Overlay.Thread->ThreadsProcess);
}


VOID
STDCALL
IoSetThreadHardErrorMode (
	IN	PVOID	Unknown0
	)
{
	UNIMPLEMENTED;
}


/* EOF */
