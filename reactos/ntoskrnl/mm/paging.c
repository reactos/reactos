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

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmMapUserAddressesToPage (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID PageAddress
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
