/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/cont.c
 * PURPOSE:         Manages non-cached memory
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PVOID MmAllocateNonCachedMemory(ULONG NumberOfBytes)
{
   UNIMPLEMENTED;
}

VOID MmFreeNonCachedMemory(PVOID BaseAddress, ULONG NumberOfBytes)
{
   UNIMPLEMENTED;
}
