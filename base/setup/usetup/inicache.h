/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/inicache.h
 * PURPOSE:         INI file parser that caches contents of INI file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
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
IniCacheSave(
    PINICACHE Cache,
    PWCHAR FileName);

PINICACHESECTION
IniCacheAppendSection(
    PINICACHE Cache,
    PWCHAR Name);

/* EOF */
