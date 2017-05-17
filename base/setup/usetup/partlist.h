/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
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
 * FILE:            base/setup/usetup/partlist.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

#include "../lib/partlist.h"

#if 0
typedef enum _FORMATMACHINESTATE
{
    Start,
    FormatSystemPartition,
    FormatInstallPartition,
    FormatOtherPartition,
    FormatDone,
    CheckSystemPartition,
    CheckInstallPartition,
    CheckOtherPartition,
    CheckDone
} FORMATMACHINESTATE, *PFORMATMACHINESTATE;
#endif

typedef struct _PARTLIST_UI
{
    PPARTLIST List;

    // PLIST_ENTRY FirstShown;
    // PLIST_ENTRY LastShown;

    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;

    SHORT Line;
    SHORT Offset;

    // BOOL Redraw;
} PARTLIST_UI, *PPARTLIST_UI;


VOID
GetPartTypeStringFromPartitionType(
    IN UCHAR partitionType,
    OUT PCHAR strPartType,
    IN ULONG cchPartType);

VOID
InitPartitionListUi(
    IN OUT PPARTLIST_UI ListUi,
    IN PPARTLIST List,
    IN SHORT Left,
    IN SHORT Top,
    IN SHORT Right,
    IN SHORT Bottom);

VOID
ScrollDownPartitionList(
    IN PPARTLIST_UI ListUi);

VOID
ScrollUpPartitionList(
    IN PPARTLIST_UI ListUi);

VOID
DrawPartitionList(
    IN PPARTLIST_UI ListUi);

/* EOF */
