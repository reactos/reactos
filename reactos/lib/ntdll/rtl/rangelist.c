/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id: rangelist.c,v 1.1 2004/02/01 20:48:06 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Range list implementation
 * FILE:              lib/ntdll/rtl/rangelist.c
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>


/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlAddRange (IN OUT PRTL_RANGE_LIST RangeList,
	     IN ULONGLONG Start,
	     IN ULONGLONG End,
	     IN UCHAR Attributes,
	     IN ULONG Flags,
	     IN PVOID UserData OPTIONAL,
	     IN PVOID Owner OPTIONAL)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlCopyRangeList (OUT PRTL_RANGE_LIST CopyRangeList,
		  IN PRTL_RANGE_LIST RangeList)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDeleteOwnersRanges (IN OUT PRTL_RANGE_LIST RangeList,
		       IN PVOID Owner)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDeleteRange (IN OUT PRTL_RANGE_LIST RangeList,
		IN ULONGLONG Start,
		IN ULONGLONG End,
		IN PVOID Owner)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlFindRange (IN PRTL_RANGE_LIST RangeList,
	      IN ULONGLONG Minimum,
	      IN ULONGLONG Maximum,
	      IN ULONG Length,
	      IN ULONG Alignment,
	      IN ULONG Flags,
	      IN UCHAR AttributeAvailableMask,
	      IN PVOID Context OPTIONAL,
	      IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
	      OUT PULONGLONG Start)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID STDCALL
RtlFreeRangeList (IN PRTL_RANGE_LIST RangeList)
{
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlGetFirstRange (IN PRTL_RANGE_LIST RangeList,
		  OUT PRTL_RANGE_LIST_ITERATOR Iterator,
		  OUT PRTL_RANGE *Range)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlGetNextRange (IN OUT PRTL_RANGE_LIST_ITERATOR Iterator,
		 OUT PRTL_RANGE *Range,
		 IN BOOLEAN MoveForwards)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID STDCALL
RtlInitializeRangeList (IN OUT PRTL_RANGE_LIST RangeList)
{
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlInvertRangeList (OUT PRTL_RANGE_LIST InvertedRangeList,
		    IN PRTL_RANGE_LIST RangeList)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlIsRangeAvailable (IN PRTL_RANGE_LIST RangeList,
		     IN ULONGLONG Start,
		     IN ULONGLONG End,
		     IN ULONG Flags,
		     IN UCHAR AttributeAvailableMask,
		     IN PVOID Context OPTIONAL,
		     IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
		     OUT PBOOLEAN Available)
{
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlMergeRangeLists (OUT PRTL_RANGE_LIST MergedRangeList,
		    IN PRTL_RANGE_LIST RangeList1,
		    IN PRTL_RANGE_LIST RangeList2,
		    IN ULONG Flags)
{
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
