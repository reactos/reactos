/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/genlist.c
 * PURPOSE:         Generic list functions
 * PROGRAMMERS:     Eric Kohl
 *                  Christoph von Wittich <christoph at reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "genlist.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

PGENERIC_LIST
CreateGenericList(VOID)
{
    PGENERIC_LIST List;

    List = (PGENERIC_LIST)RtlAllocateHeap(ProcessHeap,
                                          0,
                                          sizeof(GENERIC_LIST));
    if (List == NULL)
        return NULL;

    InitializeListHead(&List->ListHead);
    List->NumOfEntries = 0;

    List->CurrentEntry = NULL;
    List->BackupEntry = NULL;

    return List;
}

VOID
DestroyGenericList(
    IN OUT PGENERIC_LIST List,
    IN BOOLEAN FreeUserData)
{
    PGENERIC_LIST_ENTRY ListEntry;
    PLIST_ENTRY Entry;

    /* Release list entries */
    while (!IsListEmpty(&List->ListHead))
    {
        Entry = RemoveHeadList(&List->ListHead);
        ListEntry = CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);

        /* Release user data */
        if (FreeUserData && ListEntry->UserData != NULL)
            RtlFreeHeap(ProcessHeap, 0, ListEntry->UserData);

        /* Release list entry */
        RtlFreeHeap(ProcessHeap, 0, ListEntry);
    }

    /* Release list head */
    RtlFreeHeap(ProcessHeap, 0, List);
}

BOOLEAN
AppendGenericListEntry(
    IN OUT PGENERIC_LIST List,
    IN PCHAR Text,
    IN PVOID UserData,
    IN BOOLEAN Current)
{
    PGENERIC_LIST_ENTRY Entry;

    Entry = (PGENERIC_LIST_ENTRY)RtlAllocateHeap(ProcessHeap,
                                                 0,
                                                 sizeof(GENERIC_LIST_ENTRY) + strlen(Text));
    if (Entry == NULL)
        return FALSE;

    strcpy (Entry->Text, Text);
    Entry->List = List;
    Entry->UserData = UserData;

    InsertTailList(&List->ListHead, &Entry->Entry);
    List->NumOfEntries++;

    if (Current || List->CurrentEntry == NULL)
    {
        List->CurrentEntry = Entry;
    }

    return TRUE;
}

VOID
SetCurrentListEntry(
    IN PGENERIC_LIST List,
    IN PGENERIC_LIST_ENTRY Entry)
{
    if (Entry->List != List)
        return;
    List->CurrentEntry = Entry;
}

PGENERIC_LIST_ENTRY
GetCurrentListEntry(
    IN PGENERIC_LIST List)
{
    return List->CurrentEntry;
}

PGENERIC_LIST_ENTRY
GetFirstListEntry(
    IN PGENERIC_LIST List)
{
    PLIST_ENTRY Entry = List->ListHead.Flink;

    if (Entry == &List->ListHead)
        return NULL;
    return CONTAINING_RECORD(Entry, GENERIC_LIST_ENTRY, Entry);
}

PGENERIC_LIST_ENTRY
GetNextListEntry(
    IN PGENERIC_LIST_ENTRY Entry)
{
    PLIST_ENTRY Next = Entry->Entry.Flink;

    if (Next == &Entry->List->ListHead)
        return NULL;
    return CONTAINING_RECORD(Next, GENERIC_LIST_ENTRY, Entry);
}

PVOID
GetListEntryUserData(
    IN PGENERIC_LIST_ENTRY Entry)
{
    return Entry->UserData;
}

LPCSTR
GetListEntryText(
    IN PGENERIC_LIST_ENTRY Entry)
{
    return Entry->Text;
}

ULONG
GetNumberOfListEntries(
    IN PGENERIC_LIST List)
{
    return List->NumOfEntries;
}

VOID
SaveGenericListState(
    IN PGENERIC_LIST List)
{
    List->BackupEntry = List->CurrentEntry;
}

VOID
RestoreGenericListState(
    IN PGENERIC_LIST List)
{
    List->CurrentEntry = List->BackupEntry;
}

BOOLEAN
GenericListHasSingleEntry(
    IN PGENERIC_LIST List)
{
    if (!IsListEmpty(&List->ListHead) && List->ListHead.Flink == List->ListHead.Blink)
        return TRUE;

    /* if both list head pointers (which normally point to the first and last list member, respectively)
       point to the same entry then it means that there's just a single thing in there, otherwise... false! */
    return FALSE;
}

/* EOF */
