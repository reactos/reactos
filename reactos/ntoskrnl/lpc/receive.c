/* $Id: receive.c,v 1.2 2000/10/22 16:36:51 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>


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
NTSTATUS
STDCALL
NtReadRequestData (
	HANDLE		PortHandle,
	PLPC_MESSAGE	Message,
	ULONG		Index,
	PVOID		Buffer,
	ULONG		BufferLength,
	PULONG		Returnlength
	)
{
	UNIMPLEMENTED;
}


/* EOF */
