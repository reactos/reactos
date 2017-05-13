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

VOID
FS_AddProvider(
    IN OUT PFILE_SYSTEM_LIST List,
    IN PCWSTR FileSystemName,
    IN FORMATEX FormatFunc,
    IN CHKDSKEX ChkdskFunc)
{
    PFILE_SYSTEM_ITEM Item;

    Item = (PFILE_SYSTEM_ITEM)RtlAllocateHeap(ProcessHeap, 0, sizeof(FILE_SYSTEM_ITEM));
    if (!Item)
        return;

    Item->FileSystemName = FileSystemName;
    Item->FormatFunc = FormatFunc;
    Item->ChkdskFunc = ChkdskFunc;
    Item->QuickFormat = TRUE;
    InsertTailList(&List->ListHead, &Item->ListEntry);

    if (!FormatFunc)
        return;

    Item = (PFILE_SYSTEM_ITEM)RtlAllocateHeap(ProcessHeap, 0, sizeof(FILE_SYSTEM_ITEM));
    if (!Item)
        return;

    Item->FileSystemName = FileSystemName;
    Item->FormatFunc = FormatFunc;
    Item->ChkdskFunc = ChkdskFunc;
    Item->QuickFormat = FALSE;
    InsertTailList(&List->ListHead, &Item->ListEntry);
}


PFILE_SYSTEM_ITEM
GetFileSystemByName(
    IN PFILE_SYSTEM_LIST List,
    IN PWSTR FileSystemName)
{
    PLIST_ENTRY ListEntry;
    PFILE_SYSTEM_ITEM Item;

    ListEntry = List->ListHead.Flink;
    while (ListEntry != &List->ListHead)
    {
        Item = CONTAINING_RECORD(ListEntry, FILE_SYSTEM_ITEM, ListEntry);
        if (Item->FileSystemName && wcsicmp(FileSystemName, Item->FileSystemName) == 0)
            return Item;

        ListEntry = ListEntry->Flink;
    }

    return NULL;
}

#if 0 // FIXME: To be fully enabled when our storage stack & al. will work better!

/* NOTE: Ripped & adapted from base/system/autochk/autochk.c */
static NTSTATUS
_MyGetFileSystem(
    IN struct _PARTENTRY* PartEntry,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
    UCHAR Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PartitionRootPath;
    WCHAR PathBuffer[MAX_PATH];

    FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    /* Set PartitionRootPath */
    swprintf(PathBuffer,
             // L"\\Device\\Harddisk%lu\\Partition%lu", // Should work! But because ReactOS sucks atm. it actually doesn't work!!
             L"\\Device\\Harddisk%lu\\Partition%lu\\",  // HACK: Use this as a temporary hack!
             PartEntry->DiskEntry->DiskNumber,
             PartEntry->PartitionNumber);
    RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
    DPRINT("PartitionRootPath: %wZ\n", &PartitionRootPath);

    /* Open the partition */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PartitionRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle, // PartitionHandle,
                        FILE_GENERIC_READ /* | SYNCHRONIZE */,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        0 /* FILE_SYNCHRONOUS_IO_NONALERT */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition %wZ, Status 0x%08lx\n", &PartitionRootPath, Status);
        return Status;
    }

    /* Retrieve the FS attributes */
    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          FileFsAttribute,
                                          sizeof(Buffer),
                                          FileFsAttributeInformation);
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryVolumeInformationFile failed for partition %wZ, Status 0x%08lx\n", &PartitionRootPath, Status);
        return Status;
    }

    if (FileSystemNameSize * sizeof(WCHAR) < FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    RtlCopyMemory(FileSystemName,
                  FileFsAttribute->FileSystemName,
                  FileFsAttribute->FileSystemNameLength);
    FileSystemName[FileFsAttribute->FileSystemNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;
}

#endif

PFILE_SYSTEM_ITEM
GetFileSystem(
    IN PFILE_SYSTEM_LIST FileSystemList,
    IN struct _PARTENTRY* PartEntry)
{
    PFILE_SYSTEM_ITEM CurrentFileSystem;
    PWSTR FileSystemName = NULL;
#if 0 // For code temporarily disabled below
    NTSTATUS Status;
    WCHAR FsRecFileSystemName[MAX_PATH];
#endif

    CurrentFileSystem = PartEntry->FileSystem;

    /* We have a file system, return it */
    if (CurrentFileSystem != NULL && CurrentFileSystem->FileSystemName != NULL)
        return CurrentFileSystem;

    DPRINT1("File system not found, try to guess one...\n");

    CurrentFileSystem = NULL;

#if 0 // FIXME: To be fully enabled when our storage stack & al. will work better!

    /*
     * We don't have one...
     *
     * Try to infer one using NT file system recognition.
     */
    Status = _MyGetFileSystem(PartEntry, FsRecFileSystemName, ARRAYSIZE(FsRecFileSystemName));
    if (NT_SUCCESS(Status) && *FsRecFileSystemName)
    {
        /* Temporary HACK: map FAT32 back to FAT */
        if (wcscmp(FsRecFileSystemName, L"FAT32") == 0)
            wcscpy(FsRecFileSystemName, L"FAT");

        FileSystemName = FsRecFileSystemName;
        goto Quit;
    }

#endif

    /*
     * We don't have one...
     *
     * Try to infer a preferred file system for this partition, given its ID.
     *
     * WARNING: This is partly a hack, since partitions with the same ID can
     * be formatted with different file systems: for example, usual Linux
     * partitions that are formatted in EXT2/3/4, ReiserFS, etc... have the
     * same partition ID 0x83.
     *
     * The proper fix is to make a function that detects the existing FS
     * from a given partition (not based on the partition ID).
     * On the contrary, for unformatted partitions with a given ID, the
     * following code is OK.
     */
    if ((PartEntry->PartitionType == PARTITION_FAT_12) ||
        (PartEntry->PartitionType == PARTITION_FAT_16) ||
        (PartEntry->PartitionType == PARTITION_HUGE  ) ||
        (PartEntry->PartitionType == PARTITION_XINT13) ||
        (PartEntry->PartitionType == PARTITION_FAT32 ) ||
        (PartEntry->PartitionType == PARTITION_FAT32_XINT13))
    {
        FileSystemName = L"FAT";
    }
    else if (PartEntry->PartitionType == PARTITION_EXT2)
    {
        // WARNING: See the warning above.
        FileSystemName = L"EXT2";
    }
    else if (PartEntry->PartitionType == PARTITION_IFS)
    {
        // WARNING: See the warning above.
        FileSystemName = L"NTFS"; /* FIXME: Not quite correct! */
    }

#if 0
Quit: // For code temporarily disabled above
#endif

    // HACK: WARNING: We cannot write on this FS yet!
    if (FileSystemName)
    {
        if (PartEntry->PartitionType == PARTITION_EXT2 || PartEntry->PartitionType == PARTITION_IFS)
            DPRINT1("Recognized file system %S that doesn't support write support yet!\n", FileSystemName);
    }

    DPRINT1("GetFileSystem -- PartitionType: 0x%02X ; FileSystemName (guessed): %S\n",
            PartEntry->PartitionType, FileSystemName ? FileSystemName : L"None");

    if (FileSystemName != NULL)
        CurrentFileSystem = GetFileSystemByName(FileSystemList, FileSystemName);

    return CurrentFileSystem;
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

    List = (PFILE_SYSTEM_LIST)RtlAllocateHeap(ProcessHeap, 0, sizeof(FILE_SYSTEM_LIST));
    if (List == NULL)
        return NULL;

    List->Left = Left;
    List->Top = Top;
    List->Selected = NULL;
    InitializeListHead(&List->ListHead);

    HOST_CreateFileSystemList(List);

    if (!ForceFormat)
    {
        /* Add the 'Keep existing filesystem' dummy provider */
        FS_AddProvider(List, NULL, NULL, NULL);
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
    PLIST_ENTRY ListEntry = List->ListHead.Flink;
    PFILE_SYSTEM_ITEM Item;
    PLIST_ENTRY Next;

    while (ListEntry != &List->ListHead)
    {
        Item = CONTAINING_RECORD(ListEntry, FILE_SYSTEM_ITEM, ListEntry);
        Next = ListEntry->Flink;

        RtlFreeHeap(ProcessHeap, 0, Item);

        ListEntry = Next;
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
