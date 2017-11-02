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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/fslist.c
 * PURPOSE:         Filesystem list functions
 * PROGRAMMER:      Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static VOID
FS_AddProvider(
    IN OUT PFILE_SYSTEM_LIST List,
    IN PCWSTR FileSystemName, // Redundant, I need to check whether this is reaaaaally needed....
    IN PFILE_SYSTEM FileSystem)
{
    PFILE_SYSTEM_ITEM Item;

    Item = (PFILE_SYSTEM_ITEM)RtlAllocateHeap(ProcessHeap, 0, sizeof(*Item));
    if (!Item)
        return;

    Item->FileSystemName = FileSystemName;
    Item->FileSystem = FileSystem;
    Item->QuickFormat = TRUE;
    InsertTailList(&List->ListHead, &Item->ListEntry);

    if (!FileSystem)
        return;

    Item = (PFILE_SYSTEM_ITEM)RtlAllocateHeap(ProcessHeap, 0, sizeof(*Item));
    if (!Item)
        return;

    Item->FileSystemName = FileSystemName;
    Item->FileSystem = FileSystem;
    Item->QuickFormat = FALSE;
    InsertTailList(&List->ListHead, &Item->ListEntry);
}

static VOID
InitializeFileSystemList(
    IN PFILE_SYSTEM_LIST List)
{
    ULONG Count;
    PFILE_SYSTEM FileSystems;

    FileSystems = GetRegisteredFileSystems(&Count);
    if (!FileSystems || Count == 0)
        return;

    while (Count--)
    {
        FS_AddProvider(List, FileSystems->FileSystemName, FileSystems);
        ++FileSystems;
    }
}

PFILE_SYSTEM_LIST
CreateFileSystemList(
    IN SHORT Left,
    IN SHORT Top,
    IN BOOLEAN ForceFormat,
    IN PCWSTR SelectFileSystem)
{
    PFILE_SYSTEM_LIST List;
    PFILE_SYSTEM_ITEM Item;
    PLIST_ENTRY ListEntry;

    List = (PFILE_SYSTEM_LIST)RtlAllocateHeap(ProcessHeap, 0, sizeof(*List));
    if (List == NULL)
        return NULL;

    List->Left = Left;
    List->Top = Top;
    List->Selected = NULL;
    InitializeListHead(&List->ListHead);

    InitializeFileSystemList(List);
    if (!ForceFormat)
    {
        /* Add the 'Keep existing filesystem' dummy provider */
        FS_AddProvider(List, NULL, NULL);
    }

    /* Search for SelectFileSystem in list */
    ListEntry = List->ListHead.Flink;
    while (ListEntry != &List->ListHead)
    {
        Item = CONTAINING_RECORD(ListEntry, FILE_SYSTEM_ITEM, ListEntry);
        if (Item->FileSystemName && wcscmp(SelectFileSystem, Item->FileSystemName) == 0)
        {
            List->Selected = Item;
            break;
        }
        ListEntry = ListEntry->Flink;
    }
    if (!List->Selected)
        List->Selected = CONTAINING_RECORD(List->ListHead.Flink, FILE_SYSTEM_ITEM, ListEntry);

    return List;
}

VOID
DestroyFileSystemList(
    IN PFILE_SYSTEM_LIST List)
{
    PLIST_ENTRY ListEntry;
    PFILE_SYSTEM_ITEM Item;

    ListEntry = List->ListHead.Flink;
    while (!IsListEmpty(&List->ListHead))
    {
        ListEntry = RemoveHeadList(&List->ListHead);
        Item = CONTAINING_RECORD(ListEntry, FILE_SYSTEM_ITEM, ListEntry);
        RtlFreeHeap(ProcessHeap, 0, Item);
    }

    RtlFreeHeap(ProcessHeap, 0, List);
}

VOID
DrawFileSystemList(
    IN PFILE_SYSTEM_LIST List)
{
    PLIST_ENTRY ListEntry;
    PFILE_SYSTEM_ITEM Item;
    COORD coPos;
    DWORD Written;
    ULONG Index = 0;
    CHAR Buffer[128];

    ListEntry = List->ListHead.Flink;
    while (ListEntry != &List->ListHead)
    {
        Item = CONTAINING_RECORD(ListEntry, FILE_SYSTEM_ITEM, ListEntry);

        coPos.X = List->Left;
        coPos.Y = List->Top + (SHORT)Index;
        FillConsoleOutputAttribute(StdOutput,
                                   FOREGROUND_WHITE | BACKGROUND_BLUE,
                                   sizeof(Buffer),
                                   coPos,
                                   &Written);
        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    sizeof(Buffer),
                                    coPos,
                                    &Written);

        if (Item->FileSystemName)
        {
            if (Item->QuickFormat)
                snprintf(Buffer, sizeof(Buffer), MUIGetString(STRING_FORMATDISK1), Item->FileSystemName);
            else
                snprintf(Buffer, sizeof(Buffer), MUIGetString(STRING_FORMATDISK2), Item->FileSystemName);
        }
        else
        {
            snprintf(Buffer, sizeof(Buffer), MUIGetString(STRING_KEEPFORMAT));
        }

        if (ListEntry == &List->Selected->ListEntry)
            CONSOLE_SetInvertedTextXY(List->Left,
                                      List->Top + (SHORT)Index,
                                      Buffer);
        else
            CONSOLE_SetTextXY(List->Left,
                              List->Top + (SHORT)Index,
                              Buffer);
        Index++;
        ListEntry = ListEntry->Flink;
    }
}

VOID
ScrollDownFileSystemList(
    IN PFILE_SYSTEM_LIST List)
{
    if (List->Selected->ListEntry.Flink != &List->ListHead)
    {
        List->Selected = CONTAINING_RECORD(List->Selected->ListEntry.Flink, FILE_SYSTEM_ITEM, ListEntry);
        DrawFileSystemList(List);
    }
}

VOID
ScrollUpFileSystemList(
    IN PFILE_SYSTEM_LIST List)
{
    if (List->Selected->ListEntry.Blink != &List->ListHead)
    {
        List->Selected = CONTAINING_RECORD(List->Selected->ListEntry.Blink, FILE_SYSTEM_ITEM, ListEntry);
        DrawFileSystemList(List);
    }
}

/* EOF */
