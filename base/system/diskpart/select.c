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

BOOL
SelectDisk(
    INT argc,
    PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;
    ULONG ulValue;

    DPRINT("Select Disk()\n");

    if (argc > 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if (argc == 2)
    {
        if (CurrentDisk == NULL)
            ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        else
            ConResPrintf(StdOut, IDS_SELECT_DISK, CurrentDisk->DiskNumber);
        return TRUE;
    }

    if (!_wcsicmp(argv[2], L"system"))
    {
        CurrentDisk = NULL;

        Entry = DiskListHead.Flink;
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        CurrentDisk = DiskEntry;
        CurrentPartition = NULL;
        ConResPrintf(StdOut, IDS_SELECT_DISK, CurrentDisk->DiskNumber);
        return TRUE;
    }
    else if (!_wcsicmp(argv[2], L"next"))
    {
        if (CurrentDisk == NULL)
        {
            CurrentPartition = NULL;
            ConResPuts(StdErr, IDS_SELECT_DISK_ENUM_NO_START);
            return TRUE;
        }

        if (CurrentDisk->ListEntry.Flink == &DiskListHead)
        {
            CurrentDisk = NULL;
            CurrentPartition = NULL;
            ConResPuts(StdErr, IDS_SELECT_DISK_ENUM_FINISHED);
            return TRUE;
        }

        Entry = CurrentDisk->ListEntry.Flink;

        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        CurrentDisk = DiskEntry;
        CurrentPartition = NULL;
        ConResPrintf(StdOut, IDS_SELECT_DISK, CurrentDisk->DiskNumber);
        return TRUE;
    }
    else if (IsDecString(argv[2]))
    {
        ulValue = wcstoul(argv[2], NULL, 10);
        if ((ulValue == 0) && (errno == ERANGE))
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }

        CurrentDisk = NULL;

        Entry = DiskListHead.Flink;
        while (Entry != &DiskListHead)
        {
            DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

            if (DiskEntry->DiskNumber == ulValue)
            {
                CurrentDisk = DiskEntry;
                CurrentPartition = NULL;
                ConResPrintf(StdOut, IDS_SELECT_DISK, CurrentDisk->DiskNumber);
                return TRUE;
            }

            Entry = Entry->Flink;
        }
    }
    else
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    ConResPuts(StdErr, IDS_SELECT_DISK_INVALID);
    return TRUE;
}


BOOL
SelectPartition(
    INT argc,
    PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    ULONG ulValue;
    ULONG PartNumber = 1;

    DPRINT("Select Partition()\n");

    if (argc > 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_PARTITION_NO_DISK);
        return TRUE;
    }

    if (argc == 2)
    {
        if (CurrentPartition == NULL)
            ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        else
            ConResPrintf(StdOut, IDS_SELECT_PARTITION, CurrentPartition);
        return TRUE;
    }

    if (!IsDecString(argv[2]))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    ulValue = wcstoul(argv[2], NULL, 10);
    if ((ulValue == 0) && (errno == ERANGE))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    Entry = CurrentDisk->PrimaryPartListHead.Flink;
    while (Entry != &CurrentDisk->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionType != 0)
        {
            if (PartNumber == ulValue)
            {
                CurrentPartition = PartEntry;
                ConResPrintf(StdOut, IDS_SELECT_PARTITION, PartNumber);
                return TRUE;
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
            if (PartNumber == ulValue)
            {
                CurrentPartition = PartEntry;
                ConResPrintf(StdOut, IDS_SELECT_PARTITION, PartNumber);
                return TRUE;
            }

            PartNumber++;
        }
        Entry = Entry->Flink;
    }

    ConResPuts(StdErr, IDS_SELECT_PARTITION_INVALID);
    return TRUE;
}


BOOL
SelectVolume(
    INT argc,
    PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    ULONG ulValue;

    DPRINT("SelectVolume()\n");

    if (argc > 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if (argc == 2)
    {
        if (CurrentDisk == NULL)
            ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        else
            ConResPrintf(StdOut, IDS_SELECT_VOLUME, CurrentVolume->VolumeNumber);
        return TRUE;
    }

    if (!IsDecString(argv[2]))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    ulValue = wcstoul(argv[2], NULL, 10);
    if ((ulValue == 0) && (errno == ERANGE))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    CurrentVolume = NULL;

    Entry = VolumeListHead.Flink;
    while (Entry != &VolumeListHead)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (VolumeEntry->VolumeNumber == ulValue)
        {
            CurrentVolume = VolumeEntry;
            ConResPrintf(StdOut, IDS_SELECT_VOLUME, CurrentVolume->VolumeNumber);
            return TRUE;
        }

        Entry = Entry->Flink;
    }

    ConResPuts(StdErr, IDS_SELECT_VOLUME_INVALID);
    return TRUE;
}
