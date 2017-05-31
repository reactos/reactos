/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     INI file parser that caches contents of INI file in memory.
 * COPYRIGHT:   Copyright 2002-2018 Royce Mitchell III
 */

#pragma once

typedef struct _INICACHEKEY
{
    PWCHAR Name;
    PWCHAR Data;

    struct _INICACHEKEY *Next;
    struct _INICACHEKEY *Prev;
} INICACHEKEY, *PINICACHEKEY;


typedef struct _INICACHESECTION
{
    PWCHAR Name;

    PINICACHEKEY FirstKey;
    PINICACHEKEY LastKey;

    struct _INICACHESECTION *Next;
    struct _INICACHESECTION *Prev;
} INICACHESECTION, *PINICACHESECTION;


typedef struct _INICACHE
{
    PINICACHESECTION FirstSection;
    PINICACHESECTION LastSection;
} INICACHE, *PINICACHE;


typedef struct _PINICACHEITERATOR
{
    PINICACHESECTION Section;
    PINICACHEKEY Key;
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

PINICACHESECTION
IniCacheGetSection(
    PINICACHE Cache,
    PWCHAR Name);

NTSTATUS
IniCacheGetKey(
    PINICACHESECTION Section,
    PWCHAR KeyName,
    PWCHAR *KeyData);

PINICACHEITERATOR
IniCacheFindFirstValue(
    PINICACHESECTION Section,
    PWCHAR *KeyName,
    PWCHAR *KeyData);

BOOLEAN
IniCacheFindNextValue(
    PINICACHEITERATOR Iterator,
    PWCHAR *KeyName,
    PWCHAR *KeyData);

VOID
IniCacheFindClose(
    PINICACHEITERATOR Iterator);


PINICACHEKEY
IniCacheInsertKey(
    PINICACHESECTION Section,
    PINICACHEKEY AnchorKey,
    INSERTION_TYPE InsertionType,
    PWCHAR Name,
    PWCHAR Data);

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

PINICACHESECTION
IniCacheAppendSection(
    PINICACHE Cache,
    PWCHAR Name);

/* EOF */
