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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/partlist.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:
 */

#pragma once

// #include "../lib/utils/partlist.h"

typedef enum _FORMATMACHINESTATE
{
    Start,
    FormatSystemPartition,
    FormatInstallPartition,
    FormatOtherPartition,
    FormatDone,
    // CheckSystemPartition,
    // CheckInstallPartition,
    // CheckOtherPartition,
    // CheckDone
} FORMATMACHINESTATE, *PFORMATMACHINESTATE;

typedef struct _PARTLIST_UI
{
    PPARTLIST List;

    /*
     * Selected partition.
     *
     * NOTE that when CurrentPartition != NULL, then CurrentPartition->DiskEntry
     * must be the same as CurrentDisk. We should however keep the two members
     * separated as we can have a selected disk without any partition.
     */
    PDISKENTRY CurrentDisk;
    PPARTENTRY CurrentPartition;

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
    IN PPARTENTRY CurrentEntry OPTIONAL,
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
