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
 * PROGRAMMER:
 */

#pragma once

// #include "../lib/utils/genlist.h"

typedef NTSTATUS
(NTAPI *PGET_ENTRY_DESCRIPTION)(
    IN PGENERIC_LIST_ENTRY Entry,
    OUT PSTR Buffer,
    IN SIZE_T cchBufferSize);

typedef struct _GENERIC_LIST_UI
{
    PGENERIC_LIST List;

    PLIST_ENTRY FirstShown;
    PLIST_ENTRY LastShown;
    PGENERIC_LIST_ENTRY BackupEntry;

    PGET_ENTRY_DESCRIPTION GetEntryDescriptionProc;

    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
    BOOL Redraw;

    CHAR CurrentItemText[256];

} GENERIC_LIST_UI, *PGENERIC_LIST_UI;

VOID
InitGenericListUi(
    IN OUT PGENERIC_LIST_UI ListUi,
    IN PGENERIC_LIST List,
    IN PGET_ENTRY_DESCRIPTION GetEntryDescriptionProc);

VOID
RestoreGenericListUiState(
    IN PGENERIC_LIST_UI ListUi);

VOID
DrawGenericList(
    IN PGENERIC_LIST_UI ListUi,
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom);

VOID
DrawGenericListCurrentItem(
    IN PGENERIC_LIST List,
    IN PGET_ENTRY_DESCRIPTION GetEntryDescriptionProc,
    IN SHORT Left,
    IN SHORT Top);

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
