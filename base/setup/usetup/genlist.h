/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
 * FILE:            base/setup/usetup/genlist.h
 * PURPOSE:         Generic list functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

struct _GENERIC_LIST_ENTRY;
typedef struct _GENERIC_LIST_ENTRY *PGENERIC_LIST_ENTRY;
struct _GENERIC_LIST;
typedef struct _GENERIC_LIST *PGENERIC_LIST;

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




typedef struct _GENERIC_LIST_UI
{
    PGENERIC_LIST List;

    PLIST_ENTRY FirstShown;
    PLIST_ENTRY LastShown;

    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
    BOOL Redraw;
} GENERIC_LIST_UI, *PGENERIC_LIST_UI;

VOID
InitGenericListUi(
    IN OUT PGENERIC_LIST_UI ListUi,
    IN PGENERIC_LIST List);

VOID
DrawGenericList(
    IN PGENERIC_LIST_UI ListUi,
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom);

VOID
ScrollDownGenericList(
    IN PGENERIC_LIST_UI ListUi);

VOID
ScrollUpGenericList(
    IN PGENERIC_LIST_UI ListUi);

VOID
ScrollPageDownGenericList(
    IN PGENERIC_LIST_UI ListUi);

VOID
ScrollPageUpGenericList(
    IN PGENERIC_LIST_UI ListUi);

VOID
ScrollToPositionGenericList(
    IN PGENERIC_LIST_UI ListUi,
    IN ULONG uIndex);

VOID
RedrawGenericList(
    IN PGENERIC_LIST_UI ListUi);

VOID
GenericListKeyPress(
    IN PGENERIC_LIST_UI ListUi,
    IN CHAR AsciiChar);

/* EOF */
