/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Generic list functions
 * COPYRIGHT:   Copyright 2008-2018 Christoph von Wittich <christoph at reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "genlist.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PGENERIC_LIST
NTAPI
CreateGenericList(VOID)
{
    PGENERIC_LIST List;

    List = RtlAllocateHeap(ProcessHeap, 0, sizeof(GENERIC_LIST));
    if (List == NULL)
        return NULL;

    InitializeListHead(&List->ListHead);
    List->NumOfEntries = 0;
    List->CurrentEntry = NULL;

    return List;
}

VOID
NTAPI
DestroyGenericList(
    IN OUT PGENERIC_LIST List,
    IN BOOLEAN FreeData)
{
    PGENERIC_LIST_ENTRY ListEntry;
    PLIST_ENTRY Entry;

    /* Release list entries */
    while (!IsListEmpty(&List->ListHead))
    {
        Entry = RemoveHeadList(&List->ListHead);
        ListEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);

        /* Release user data */
        if (FreeData && ListEntry->Data != NULL)
            RtlFreeHeap(ProcessHeap, 0, ListEntry->Data);

        /* Release list entry */
        RtlFreeHeap(ProcessHeap, 0, ListEntry);
    }

    /* Release list head */
    RtlFreeHeap(ProcessHeap, 0, List);
}

BOOLEAN
NTAPI
AppendGenericListEntry(
    IN OUT PGENERIC_LIST List,
    IN PVOID Data,
    IN BOOLEAN Current)
{
    PGENERIC_LIST_ENTRY Entry;

    Entry = RtlAllocateHeap(ProcessHeap, 0, sizeof(GENERIC_LIST_ENTRY));
    if (Entry == NULL)
        return FALSE;

    Entry->List = List;
    Entry->Data = Data;
    Entry->UiData = 0;

    InsertTailList(&List->ListHead, &Entry->Entry);
    ++List->NumOfEntries;

    if (Current || List->CurrentEntry == NULL)
        List->CurrentEntry = Entry;

    return TRUE;
}

VOID
NTAPI
SetCurrentListEntry(
    IN PGENERIC_LIST List,
    IN PGENERIC_LIST_ENTRY Entry)
{
    if (!Entry || (Entry->List != List))
        return;
    List->CurrentEntry = Entry;
}

PGENERIC_LIST_ENTRY
NTAPI
GetCurrentListEntry(
    IN PGENERIC_LIST List)
{
    return List->CurrentEntry;
}

PGENERIC_LIST_ENTRY
NTAPI
GetFirstListEntry(
    IN PGENERIC_LIST List)
{
    if (IsListEmpty(&List->ListHead))
        return NULL;

    return CONTAINING_RECORD(List->ListHead.Flink, GENERIC_LIST_ENTRY, Entry);
}

PGENERIC_LIST_ENTRY
NTAPI
GetNextListEntry(
    IN PGENERIC_LIST_ENTRY Entry)
{
    PLIST_ENTRY Next = Entry->Entry.Flink;

    if (Next == &Entry->List->ListHead)
        return NULL;

    return CONTAINING_RECORD(Next, GENERIC_LIST_ENTRY, Entry);
}

PVOID
NTAPI
GetListEntryData(
    IN PGENERIC_LIST_ENTRY Entry)
{
    return Entry->Data;
}

ULONG_PTR
GetListEntryUiData(
    IN PGENERIC_LIST_ENTRY Entry)
{
    return Entry->UiData;
}

ULONG
NTAPI
GetNumberOfListEntries(
    IN PGENERIC_LIST List)
{
    return List->NumOfEntries;
}

/* EOF */
