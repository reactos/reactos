/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Generic Table Implementation
 * FILE:              lib/rtl/genertictbl.c
 */

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>
/* FUNCTIONS *****************************************************************/

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlDeleteElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlDeleteElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTable (
	PRTL_GENERIC_TABLE Table,
	BOOLEAN Restart
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableAvl (
	PRTL_AVL_TABLE Table,
	BOOLEAN Restart
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableLikeADirectory (
	IN PRTL_AVL_TABLE Table,
	IN PRTL_AVL_MATCH_FUNCTION MatchFunction,
	IN PVOID MatchData,
	IN ULONG NextFlag,
	IN OUT PVOID *RestartKey,
	IN OUT PULONG DeleteCount,
	IN OUT PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplaying (
	PRTL_GENERIC_TABLE Table,
	PVOID *RestartKey
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlEnumerateGenericTableWithoutSplayingAvl (
	PRTL_AVL_TABLE Table,
	PVOID *RestartKey
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlGetElementGenericTable(
	PRTL_GENERIC_TABLE Table,
	ULONG I
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlGetElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	ULONG I
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
VOID
STDCALL
RtlInitializeGenericTable (
	PRTL_GENERIC_TABLE Table,
	PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
	PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
	PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
	PVOID TableContext
	)
{
	UNIMPLEMENTED;
}

/*
* @unimplemented
*/
VOID
STDCALL
RtlInitializeGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
	PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
	PRTL_AVL_FREE_ROUTINE FreeRoutine,
	PVOID TableContext
	)
{
	UNIMPLEMENTED;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID NodeOrParent,
	TABLE_SEARCH_RESULT SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlInsertElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	ULONG BufferSize,
	PBOOLEAN NewElement OPTIONAL,
	PVOID NodeOrParent,
	TABLE_SEARCH_RESULT SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}


/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlIsGenericTableEmpty (
	PRTL_GENERIC_TABLE Table
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlIsGenericTableEmptyAvl (
	PRTL_AVL_TABLE Table
	)
{
	UNIMPLEMENTED;
	return FALSE;
}


/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTable (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTableAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTableFull (
	PRTL_GENERIC_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
PVOID
STDCALL
RtlLookupElementGenericTableFullAvl (
	PRTL_AVL_TABLE Table,
	PVOID Buffer,
	OUT PVOID *NodeOrParent,
	OUT TABLE_SEARCH_RESULT *SearchResult
	)
{
	UNIMPLEMENTED;
	return 0;
}


/*
* @unimplemented
*/
ULONG
STDCALL
RtlNumberGenericTableElements(
	PRTL_GENERIC_TABLE Table
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
* @unimplemented
*/
ULONG
STDCALL
RtlNumberGenericTableElementsAvl (
	PRTL_AVL_TABLE Table
	)
{
	UNIMPLEMENTED;
	return 0;
}

/* EOF */
