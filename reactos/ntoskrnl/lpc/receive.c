/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/receive.c
 * PURPOSE:         Communication mechanism
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							SYSTEM
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
NtReadRequestData (HANDLE		PortHandle,
		   PPORT_MESSAGE	Message,
		   ULONG		Index,
		   PVOID		Buffer,
		   ULONG		BufferLength,
		   PULONG		Returnlength)
{
	UNIMPLEMENTED;
	return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
