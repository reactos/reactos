/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     INI file parser that caches contents of INI file in memory.
 * COPYRIGHT:   Copyright 2002-2018 Royce Mitchell III
 */

#pragma once

typedef struct _INI_KEYWORD
{
    PWSTR Name;
    PWSTR Data;
    LIST_ENTRY ListEntry;
} INI_KEYWORD, *PINI_KEYWORD;

typedef struct _INI_SECTION
{
    PWSTR Name;
    LIST_ENTRY KeyList;
    LIST_ENTRY ListEntry;
} INI_SECTION, *PINI_SECTION;

typedef struct _INICACHE
{
    LIST_ENTRY SectionList;
} INICACHE, *PINICACHE;

typedef struct _PINICACHEITERATOR
{
    PINI_SECTION Section;
    PINI_KEYWORD Key;
} INICACHEITERATOR, *PINICACHEITERATOR;

typedef enum
{
    INSERT_FIRST,
    INSERT_BEFORE,
    INSERT_AFTER,
    INSERT_LAST
} INSERTION_TYPE;

/* FUNCTIONS ****************************************************************/

NTSTATUS
IniCacheLoadFromMemory(
    PINICACHE *Cache,
    PCHAR FileBuffer,
    ULONG FileLength,
    BOOLEAN String);

NTSTATUS
IniCacheLoadByHandle(
    PINICACHE *Cache,
    HANDLE FileHandle,
    BOOLEAN String);

NTSTATUS
IniCacheLoad(
    PINICACHE *Cache,
    PWCHAR FileName,
    BOOLEAN String);

VOID
IniCacheDestroy(
    _In_ PINICACHE Cache);

PINI_SECTION
IniGetSection(
    _In_ PINICACHE Cache,
    _In_ PCWSTR Name);

PINI_KEYWORD
IniGetKey(
    _In_ PINI_SECTION Section,
    _In_ PCWSTR KeyName,
    _Out_ PCWSTR* KeyData);

PINICACHEITERATOR
IniFindFirstValue(
    _In_ PINI_SECTION Section,
    _Out_ PCWSTR* KeyName,
    _Out_ PCWSTR* KeyData);

BOOLEAN
IniFindNextValue(
    _In_ PINICACHEITERATOR Iterator,
    _Out_ PCWSTR* KeyName,
    _Out_ PCWSTR* KeyData);

VOID
IniFindClose(
    _In_ PINICACHEITERATOR Iterator);

PINI_SECTION
IniAddSection(
    _In_ PINICACHE Cache,
    _In_ PCWSTR Name);

VOID
IniRemoveSection(
    _In_ PINI_SECTION Section);

PINI_KEYWORD
IniInsertKey(
    _In_ PINI_SECTION Section,
    _In_ PINI_KEYWORD AnchorKey,
    _In_ INSERTION_TYPE InsertionType,
    _In_ PCWSTR Name,
    _In_ PCWSTR Data);

PINI_KEYWORD
IniAddKey(
    _In_ PINI_SECTION Section,
    _In_ PCWSTR Name,
    _In_ PCWSTR Data);

VOID
IniRemoveKeyByName(
    _In_ PINI_SECTION Section,
    _In_ PCWSTR KeyName);

VOID
IniRemoveKey(
    _In_ PINI_SECTION Section,
    _In_ PINI_KEYWORD Key);

PINICACHE
IniCacheCreate(VOID);

NTSTATUS
IniCacheSaveByHandle(
    PINICACHE Cache,
    HANDLE FileHandle);

NTSTATUS
IniCacheSave(
    PINICACHE Cache,
    PWCHAR FileName);

/* EOF */
