/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/base/bug.cc
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PVOID ExAllocateFromNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside)
{
   UNIMPLEMENTED;
}

PVOID ExAllocateFromPagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside)
{
   UNIMPLEMENTED;
}

VOID ExDeleteNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside)
{
   UNIMPLEMENTED;
}

VOID ExDeletePagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside)
{
   UNIMPLEMENTED;
}

VOID ExFreeToNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside,
				 PVOID Entry)
{
   UNIMPLEMENTED;
}

VOID ExFreeToPagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside,
				 PVOID Entry)
{
   UNIMPLEMENTED;
}

VOID ExInitializeNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside,
				     PALLOCATE_FUNCTION Allocate,
				     PFREE_FUNCTION Free,
				     ULONG Flags,
				     ULONG Size,
				     ULONG Tag,
				     USHORT Depth)
{
   UNIMPLEMENTED
}

VOID ExInitializePagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside,
				    PALLOCATE_FUNCTION Allocate,
				    PFREE_FUNCTION Free,
				    ULONG Flags,
				    ULONG Size,
				    ULONG Tag,
				    USHORT Depth)
{
   UNIMPLEMENTED
}
