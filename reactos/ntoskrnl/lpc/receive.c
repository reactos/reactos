/* $Id: receive.c,v 1.4 2002/09/07 15:12:58 chorns Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/receive.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
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
 *
 */
NTSTATUS STDCALL
NtReadRequestData (HANDLE		PortHandle,
		   PLPC_MESSAGE	Message,
		   ULONG		Index,
		   PVOID		Buffer,
		   ULONG		BufferLength,
		   PULONG		Returnlength)
{
	UNIMPLEMENTED;
}


/* EOF */
