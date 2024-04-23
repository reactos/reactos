/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     INI file parser that caches contents of INI file in memory.
 * COPYRIGHT:   Copyright 2002-2018 Royce Mitchell III
 */

#pragma once

typedef struct _INI_KEYWORD
{
    PWCHAR Name;
    PWCHAR Data;

    struct _INI_KEYWORD *Next;
    struct _INI_KEYWORD *Prev;
} INI_KEYWORD, *PINI_KEYWORD;

typedef struct _INI_SECTION
{
    PWCHAR Name;

    PINI_KEYWORD FirstKey;
    PINI_KEYWORD LastKey;

    struct _INI_SECTION *Next;
    struct _INI_SECTION *Prev;
} INI_SECTION, *PINI_SECTION;

typedef struct _INICACHE
{
    PINI_SECTION FirstSection;
    PINI_SECTION LastSection;
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
    PINICACHE Cache);

PINI_SECTION
IniGetSection(
    PINICACHE Cache,
    PWCHAR Name);

NTSTATUS
IniGetKey(
    PINI_SECTION Section,
    PWCHAR KeyName,
    PWCHAR *KeyData);

PINICACHEITERATOR
IniFindFirstValue(
    PINI_SECTION Section,
    PWCHAR *KeyName,
    PWCHAR *KeyData);

BOOLEAN
IniFindNextValue(
    PINICACHEITERATOR Iterator,
    PWCHAR *KeyName,
    PWCHAR *KeyData);

VOID
IniFindClose(
    PINICACHEITERATOR Iterator);

PINI_SECTION
IniAddSection(
    _In_ PINICACHE Cache,
    _In_ PCWSTR Name);

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
