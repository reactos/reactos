/* $Id: lookas.c,v 1.2 2000/06/07 13:05:09 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/lookas.c
 * PURPOSE:         Lookaside lists
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PVOID
STDCALL
ExAllocateFromNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside
	)
{
   UNIMPLEMENTED;
}

PVOID
STDCALL
ExAllocateFromPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	)
{
   UNIMPLEMENTED;
}

VOID
STDCALL
ExDeleteNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside
	)
{
   UNIMPLEMENTED;
}

VOID
STDCALL
ExDeletePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	)
{
   UNIMPLEMENTED;
}

VOID
STDCALL
ExFreeToNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside,
	PVOID			Entry
	)
{
   UNIMPLEMENTED;
}

VOID
STDCALL
ExFreeToPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PVOID			Entry
	)
{
   UNIMPLEMENTED;
}

VOID
STDCALL
ExInitializeNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth)
{
   UNIMPLEMENTED
}

VOID
STDCALL
ExInitializePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth
	)
{
   UNIMPLEMENTED
}

/* EOF */
