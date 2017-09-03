/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Generic list functions
 * COPYRIGHT:   Copyright 2008-2018 Christoph von Wittich <christoph at reactos.org>
 */

#pragma once

typedef struct _GENERIC_LIST_ENTRY
{
    LIST_ENTRY Entry;
    struct _GENERIC_LIST* List;
    PVOID UserData;
    WCHAR Text[1];      // FIXME: UI stuff

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
    IN PCWSTR Text,
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

PCWSTR
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
