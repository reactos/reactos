/* $Id: process.c,v 1.2 2000/03/26 19:38:26 ea Exp $
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
	PVOID	* Minimum, /* guess */
	PVOID	* Maximum  /* guess */
	)
{
	/* FIXME: */
	*Minimum = NULL;
	*Maximum = NULL;
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
