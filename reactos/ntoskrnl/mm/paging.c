/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/paging.c
 * PURPOSE:         Paging file functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtCreatePagingFile (
	IN	PUNICODE_STRING	PageFileName,
	IN	ULONG		MiniumSize,
	IN	ULONG		MaxiumSize,
	OUT	PULONG		ActualSize
	)
{
	UNIMPLEMENTED;
}
