/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/genlist.h
 * PURPOSE:         Generic list functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

typedef struct _GENERIC_LIST_ENTRY
{
    LIST_ENTRY Entry;
    struct _GENERIC_LIST* List;
    PVOID UserData;
    CHAR Text[1];       // FIXME: UI stuff

} GENERIC_LIST_ENTRY, *PGENERIC_LIST_ENTRY;

typedef struct _GENERIC_LIST
{
    LIST_ENTRY ListHead;
    ULONG NumOfEntries;

    PGENERIC_LIST_ENTRY CurrentEntry;
    PGENERIC_LIST_ENTRY BackupEntry;

} GENERIC_LIST, *PGENERIC_LIST;


PGENERIC_LIST
CreateGenericList(VOID);

VOID
DestroyGenericList(
    IN OUT PGENERIC_LIST List,
    IN BOOLEAN FreeUserData);

BOOLEAN
AppendGenericListEntry(
    IN OUT PGENERIC_LIST List,
    IN PCHAR Text,
    IN PVOID UserData,
    IN BOOLEAN Current);

VOID
SetCurrentListEntry(
    IN PGENERIC_LIST List,
    IN PGENERIC_LIST_ENTRY Entry);

PGENERIC_LIST_ENTRY
GetCurrentListEntry(
    IN PGENERIC_LIST List);

PGENERIC_LIST_ENTRY
GetFirstListEntry(
    IN PGENERIC_LIST List);

PGENERIC_LIST_ENTRY
GetNextListEntry(
    IN PGENERIC_LIST_ENTRY Entry);

PVOID
GetListEntryUserData(
    IN PGENERIC_LIST_ENTRY Entry);

LPCSTR
GetListEntryText(
    IN PGENERIC_LIST_ENTRY Entry);

ULONG
GetNumberOfListEntries(
    IN PGENERIC_LIST List);

VOID
SaveGenericListState(
    IN PGENERIC_LIST List);

VOID
RestoreGenericListState(
    IN PGENERIC_LIST List);

BOOLEAN
GenericListHasSingleEntry(
    IN PGENERIC_LIST List);

/* EOF */
