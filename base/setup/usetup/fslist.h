/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/fslist.h
 * PURPOSE:         Filesystem list functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#pragma once

#include <fmifs/fmifs.h>

typedef struct _FILE_SYSTEM_ITEM
{
    LIST_ENTRY ListEntry;
    PCWSTR FileSystemName; /* Not owned by the item */
    FORMATEX FormatFunc;
    CHKDSKEX ChkdskFunc;
    BOOLEAN QuickFormat;
} FILE_SYSTEM_ITEM, *PFILE_SYSTEM_ITEM;

typedef struct _FILE_SYSTEM_LIST
{
    SHORT Left;
    SHORT Top;
    PFILE_SYSTEM_ITEM Selected;
    LIST_ENTRY ListHead; /* List of FILE_SYSTEM_ITEM */
} FILE_SYSTEM_LIST, *PFILE_SYSTEM_LIST;

VOID
FS_AddProvider(
    IN OUT PFILE_SYSTEM_LIST List,
    IN PCWSTR FileSystemName,
    IN FORMATEX FormatFunc,
    IN CHKDSKEX ChkdskFunc);

PFILE_SYSTEM_ITEM
GetFileSystemByName(
    IN PFILE_SYSTEM_LIST List,
    IN PWSTR FileSystemName);

struct _PARTENTRY; // Defined in partlist.h
PFILE_SYSTEM_ITEM
GetFileSystem(
    IN PFILE_SYSTEM_LIST FileSystemList,
    IN struct _PARTENTRY* PartEntry);

PFILE_SYSTEM_LIST
CreateFileSystemList(
    IN SHORT Left,
    IN SHORT Top,
    IN BOOLEAN ForceFormat,
    IN PCWSTR SelectFileSystem);

VOID
DestroyFileSystemList(
    IN PFILE_SYSTEM_LIST List);

VOID
DrawFileSystemList(
    IN PFILE_SYSTEM_LIST List);

VOID
ScrollDownFileSystemList(
    IN PFILE_SYSTEM_LIST List);

VOID
ScrollUpFileSystemList(
    IN PFILE_SYSTEM_LIST List);

/* EOF */
