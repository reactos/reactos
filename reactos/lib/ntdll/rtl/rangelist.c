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
/* $Id: rangelist.c,v 1.2 2004/05/14 12:11:52 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Range list implementation
 * FILE:              lib/ntdll/rtl/rangelist.c
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <ntdll/ntdll.h>


typedef struct _RTL_RANGE_ENTRY
{
  LIST_ENTRY Entry;
  RTL_RANGE Range;
} RTL_RANGE_ENTRY, *PRTL_RANGE_ENTRY;


/* FUNCTIONS ***************************************************************/

/*
 * TODO:
 *   - Check for overlapping ranges.
 *
 * @implemented
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
  PRTL_RANGE_ENTRY RangeEntry;
  PRTL_RANGE_ENTRY Previous;
  PRTL_RANGE_ENTRY Current;
  PLIST_ENTRY Entry;

  if (Start > End)
    return STATUS_INVALID_PARAMETER;

  /* Create new range entry */
  RangeEntry = RtlAllocateHeap (RtlGetProcessHeap(),
				0,
				sizeof(RTL_RANGE_ENTRY));
  if (RangeEntry == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  /* Initialize range entry */
  RangeEntry->Range.Start = Start;
  RangeEntry->Range.End = End;
  RangeEntry->Range.Attributes = Attributes;
  RangeEntry->Range.Flags = Flags;
  RangeEntry->Range.UserData = UserData;
  RangeEntry->Range.Owner = Owner;

  /* Insert range entry */
  if (RangeList->Count == 0)
    {
      InsertTailList (&RangeList->ListHead,
		      &RangeEntry->Entry);
      RangeList->Count++;
      RangeList->Stamp++;
      return STATUS_SUCCESS;
    }
  else
    {
      Previous = NULL;
      Entry = RangeList->ListHead.Flink;
      while (Entry != &RangeList->ListHead)
	{
	  Current = CONTAINING_RECORD (Entry, RTL_RANGE_ENTRY, Entry);
	  if (Current->Range.Start > RangeEntry->Range.End)
	    {
	      /* Insert before current */
	      DPRINT ("Insert before current\n");
	      InsertTailList (&Current->Entry,
			      &RangeEntry->Entry);

	      RangeList->Count++;
	      RangeList->Stamp++;
	      return STATUS_SUCCESS;
	    }

	  Previous = Current;
	  Entry = Entry->Flink;
	}

      DPRINT ("Insert tail\n");
      InsertTailList (&RangeList->ListHead,
		      &RangeEntry->Entry);
      RangeList->Count++;
      RangeList->Stamp++;
      return STATUS_SUCCESS;
    }

  return STATUS_UNSUCCESSFUL;
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
 * @implemented
 */
VOID STDCALL
RtlFreeRangeList (IN PRTL_RANGE_LIST RangeList)
{
  PLIST_ENTRY Entry;
#ifndef NDEBUG
  PRTL_RANGE_ENTRY Current;
#endif

  while (!IsListEmpty(&RangeList->ListHead))
    {
      Entry = RemoveHeadList (&RangeList->ListHead);

#ifndef NDEBUG
      Current = CONTAINING_RECORD (Entry, RTL_RANGE_ENTRY, Entry);
      DPRINT ("Range start: %I64u\n", Current->Range.Start);
      DPRINT ("Range end:   %I64u\n", Current->Range.End);
#endif

      RtlFreeHeap (RtlGetProcessHeap(),
		   0,
		   Entry);
    }

  RangeList->Count = 0;
  RangeList->Stamp++;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetFirstRange (IN PRTL_RANGE_LIST RangeList,
		  OUT PRTL_RANGE_LIST_ITERATOR Iterator,
		  OUT PRTL_RANGE *Range)
{
  Iterator->RangeListHead = &RangeList->ListHead;
  Iterator->MergedHead = NULL;
  Iterator->Stamp = RangeList->Stamp;
  if (IsListEmpty(&RangeList->ListHead))
    {
      Iterator->Current = NULL;
      *Range = NULL;
      return STATUS_NO_MORE_ENTRIES;
    }

  Iterator->Current = RangeList->ListHead.Flink;
  *Range = &((PRTL_RANGE_ENTRY)Iterator->Current)->Range;

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetNextRange (IN OUT PRTL_RANGE_LIST_ITERATOR Iterator,
		 OUT PRTL_RANGE *Range,
		 IN BOOLEAN MoveForwards)
{
  PRTL_RANGE_LIST RangeList;
  PLIST_ENTRY Next;

  RangeList = CONTAINING_RECORD(Iterator->RangeListHead, RTL_RANGE_LIST, ListHead);
  if (Iterator->Stamp != RangeList->Stamp)
    return STATUS_INVALID_PARAMETER;

  if (MoveForwards)
    {
      Next = ((PRTL_RANGE_ENTRY)Iterator->Current)->Entry.Flink;
    }
  else
    {
      Next = ((PRTL_RANGE_ENTRY)Iterator->Current)->Entry.Blink;
    }

  if (Next == Iterator->RangeListHead)
    return STATUS_NO_MORE_ENTRIES;

  Iterator->Current = Next;
  *Range = &((PRTL_RANGE_ENTRY)Next)->Range;

  return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID STDCALL
RtlInitializeRangeList (IN OUT PRTL_RANGE_LIST RangeList)
{
  InitializeListHead (&RangeList->ListHead);
  RangeList->Flags = 0;
  RangeList->Count = 0;
  RangeList->Stamp = 0;
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
