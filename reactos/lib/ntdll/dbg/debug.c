/* $Id: debug.c,v 1.1 2000/04/14 01:43:05 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/dbg/debug.c
 * PURPOSE:         User mode debugger support functions
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  14/04/2000 Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/dbg.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
DbgUiContinue (
	PCLIENT_ID	ClientId,
	ULONG		ContinueStatus
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
