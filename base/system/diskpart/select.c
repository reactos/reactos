/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/select.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
SelectDisk(
    INT argc,
    LPWSTR *argv)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;
    LONG lValue;
    LPWSTR endptr = NULL;

    DPRINT("Select Disk()\n");

    if (argc > 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (argc == 2)
    {
        if (CurrentDisk == NULL)
            ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        else
            ConResPrintf(StdOut, IDS_SELECT_DISK, CurrentDisk->DiskNumber);
        return;
    }

    lValue = wcstol(argv[2], &endptr, 10);
    if (((lValue == 0) && (endptr == argv[2])) ||
        (lValue < 0))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    CurrentDisk = NULL;

    Entry = DiskListHead.Flink;
    while (Entry != &DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->DiskNumber == (ULONG)lValue)
        {
            CurrentDisk = DiskEntry;
            CurrentPartition = NULL;
            ConResPrintf(StdOut, IDS_SELECT_DISK, CurrentDisk->DiskNumber);
            return;
        }

        Entry = Entry->Flink;
    }

    ConResPuts(StdErr, IDS_SELECT_DISK_INVALID);
}


static
VOID
SelectPartition(
    INT argc,
    LPWSTR *argv)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    LONG lValue;
    LPWSTR endptr = NULL;
    ULONG PartNumber = 1;

    DPRINT("Select Partition()\n");

    if (argc > 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_PARTITION_NO_DISK);
        return;
    }

    if (argc == 2)
    {
        if (CurrentPartition == NULL)
            ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        else
            ConResPrintf(StdOut, IDS_SELECT_PARTITION, CurrentPartition);
        return;
    }

    lValue = wcstol(argv[2], &endptr, 10);
    if (((lValue == 0) && (endptr == argv[2])) ||
        (lValue < 0))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    Entry = CurrentDisk->PrimaryPartListHead.Flink;
    while (Entry != &CurrentDisk->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionType != 0)
        {
            if (PartNumber == (ULONG)lValue)
            {
                CurrentPartition = PartEntry;
                ConResPrintf(StdOut, IDS_SELECT_PARTITION, PartNumber);
                return;
            }

            PartNumber++;
        }

        Entry = Entry->Flink;
    }

    Entry = CurrentDisk->LogicalPartListHead.Flink;
    while (Entry != &CurrentDisk->LogicalPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionType != 0)
        {
            if (PartNumber == (ULONG)lValue)
            {
                CurrentPartition = PartEntry;
                ConResPrintf(StdOut, IDS_SELECT_PARTITION, PartNumber);
                return;
            }

            PartNumber++;
        }
        Entry = Entry->Flink;
    }

    ConResPuts(StdErr, IDS_SELECT_PARTITION_INVALID);
}


static
VOID
SelectVolume(
    INT argc,
    LPWSTR *argv)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    LONG lValue;
    LPWSTR endptr = NULL;

    DPRINT("SelectVolume()\n");

    if (argc > 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (argc == 2)
    {
        if (CurrentDisk == NULL)
            ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        else
            ConResPrintf(StdOut, IDS_SELECT_VOLUME, CurrentVolume->VolumeNumber);
        return;
    }

    lValue = wcstol(argv[2], &endptr, 10);
    if (((lValue == 0) && (endptr == argv[2])) ||
        (lValue < 0))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    CurrentVolume = NULL;

    Entry = VolumeListHead.Flink;
    while (Entry != &VolumeListHead)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (VolumeEntry->VolumeNumber == (ULONG)lValue)
        {
            CurrentVolume = VolumeEntry;
            ConResPrintf(StdOut, IDS_SELECT_VOLUME, CurrentVolume->VolumeNumber);
            return;
        }

        Entry = Entry->Flink;
    }

    ConResPuts(StdErr, IDS_SELECT_VOLUME_INVALID);
}


BOOL
select_main(
    INT argc,
    LPWSTR *argv)
{
    /* gets the first word from the string */
    if (argc == 1)
    {
        ConResPuts(StdOut, IDS_HELP_CMD_SELECT);
        return TRUE;
    }

    /* determines which to list (disk, partition, etc.) */
    if (!wcsicmp(argv[1], L"disk"))
        SelectDisk(argc, argv);
    else if (!wcsicmp(argv[1], L"partition"))
        SelectPartition(argc, argv);
    else if (!wcsicmp(argv[1], L"volume"))
        SelectVolume(argc, argv);
    else
        ConResPuts(StdOut, IDS_HELP_CMD_SELECT);

    return TRUE;
}
