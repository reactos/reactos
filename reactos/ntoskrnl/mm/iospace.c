/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/iospace.c
 * PURPOSE:         Mapping io space
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PVOID MmMapIoSpace(PHYSICAL_ADDRESS PhysicalAddress,
		   ULONG NumberOfBytes,
		   BOOLEAN CacheEnable)
{
   UNIMPLEMENTED;
}
 
VOID MmUnmapIoSpace(PVOID BaseAddress, ULONG NumberOfBytes)
{
   UNIMPLEMENTED;
}
