/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#include <rtl.h>

VOID RtlListInitializeHead(PLIST_ITEM ListHead)
{
	ListHead->ListPrev = NULL;
	ListHead->ListNext = NULL;
}

VOID RtlListInsertHead(PLIST_ITEM ListHead, PLIST_ITEM Entry)
{
	ListHead = RtlListGetHead(ListHead);
	ListHead->ListPrev = Entry;
	Entry->ListNext = ListHead;
	Entry->ListPrev = NULL;
}

VOID RtlListInsertTail(PLIST_ITEM ListHead, PLIST_ITEM Entry)
{
	ListHead = RtlListGetTail(ListHead);
	ListHead->ListNext = Entry;
	Entry->ListNext = NULL;
	Entry->ListPrev = ListHead;
}

PLIST_ITEM RtlListRemoveHead(PLIST_ITEM ListHead)
{
	PLIST_ITEM OldListHead = RtlListGetHead(ListHead);

	ListHead = ListHead->ListNext;
	ListHead->ListPrev = NULL;

	return OldListHead;
}

PLIST_ITEM RtlListRemoveTail(PLIST_ITEM ListHead)
{
	PLIST_ITEM ListTail;

	ListTail = RtlListGetTail(ListHead);
	ListHead = ListTail->ListPrev;
	ListHead->ListNext = NULL;

	return ListTail;
}

PLIST_ITEM RtlListGetHead(PLIST_ITEM ListHead)
{
	while (ListHead->ListPrev != NULL)
	{
		ListHead = ListHead->ListPrev;
	}

	return ListHead;
}

PLIST_ITEM RtlListGetTail(PLIST_ITEM ListHead)
{
	while (ListHead->ListNext != NULL)
	{
		ListHead = ListHead->ListNext;
	}

	return ListHead;
}

BOOL RtlListIsEmpty(PLIST_ITEM ListHead)
{
	if (ListHead == NULL)
	{
		return TRUE;
	}

	return (ListHead->ListNext == NULL);
}

U32 RtlListCountEntries(PLIST_ITEM ListHead)
{
	U32		Count = 0;

	while (ListHead != NULL)
	{
		Count++;
		ListHead = ListHead->ListNext;
	}

	return Count;
}

PLIST_ITEM RtlListGetPrevious(PLIST_ITEM ListEntry)
{
	return ListEntry->ListPrev;
}

PLIST_ITEM RtlListGetNext(PLIST_ITEM ListEntry)
{
	return ListEntry->ListNext;
}

PLIST_ITEM RtlListRemoveEntry(PLIST_ITEM ListEntry)
{
	PLIST_ITEM	ListNext = RtlListGetNext(ListEntry);
	PLIST_ITEM	ListPrev = RtlListGetPrevious(ListEntry);

	if (ListPrev != NULL)
	{
		ListPrev->ListNext = ListNext;
	}

	if (ListNext != NULL)
	{
		ListNext->ListPrev = ListPrev;
	}

	return ListNext;
}

VOID RtlListInsertEntry(PLIST_ITEM InsertAfter, PLIST_ITEM ListEntry)
{
	PLIST_ITEM	ListNext = RtlListGetNext(InsertAfter);

	InsertAfter->ListNext = ListEntry;
	ListEntry->ListPrev = InsertAfter;
	ListEntry->ListNext = ListNext;
}

VOID RtlListMoveEntryPrevious(PLIST_ITEM ListEntry)
{
	PLIST_ITEM	ListPrev = RtlListGetPrevious(ListEntry);

	if (ListPrev == NULL)
	{
		return;
	}

	//
	// Move the previous entry after this one
	//
	RtlListRemoveEntry(ListPrev);
	RtlListInsertEntry(ListEntry, ListPrev);
}

VOID RtlListMoveEntryNext(PLIST_ITEM ListEntry)
{
	PLIST_ITEM	ListNext = RtlListGetNext(ListEntry);

	if (ListNext == NULL)
	{
		return;
	}

	//
	// Move this entry after the next entry
	//
	RtlListRemoveEntry(ListEntry);
	RtlListInsertEntry(ListNext, ListEntry);
}
