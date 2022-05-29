/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/detail.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
BOOL
IsDiskInVolume(
    _In_ PVOLENTRY VolumeEntry,
    _In_ PDISKENTRY DiskEntry)
{
    ULONG i;

    if ((VolumeEntry == NULL) ||
        (VolumeEntry->pExtents == NULL) ||
        (DiskEntry == NULL))
        return FALSE;

    for (i = 0; i < VolumeEntry->pExtents->NumberOfDiskExtents; i++)
    {
        if (VolumeEntry->pExtents->Extents[i].DiskNumber == DiskEntry->DiskNumber)
            return TRUE;
    }

    return FALSE;
}


static
BOOL
IsPartitionInVolume(
    _In_ PVOLENTRY VolumeEntry,
    _In_ PPARTENTRY PartEntry)
{
    ULONG i;

    if ((VolumeEntry == NULL) ||
        (VolumeEntry->pExtents == NULL) ||
        (PartEntry == NULL) ||
        (PartEntry->DiskEntry == NULL))
        return FALSE;

    for (i = 0; i < VolumeEntry->pExtents->NumberOfDiskExtents; i++)
    {
        if (VolumeEntry->pExtents->Extents[i].DiskNumber == PartEntry->DiskEntry->DiskNumber)
        {
            if ((VolumeEntry->pExtents->Extents[i].StartingOffset.QuadPart == PartEntry->StartSector.QuadPart * PartEntry->DiskEntry->BytesPerSector) &&
                (VolumeEntry->pExtents->Extents[i].ExtentLength.QuadPart == PartEntry->SectorCount.QuadPart * PartEntry->DiskEntry->BytesPerSector))
                return TRUE;
        }
    }

    return FALSE;
}


BOOL
DetailDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    BOOL bPrintHeader = TRUE;

    DPRINT("DetailDisk()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    /* TODO: Print more disk details */
    ConPuts(StdOut, L"\n");
    ConResPrintf(StdOut, IDS_DETAIL_INFO_DISK_ID, CurrentDisk->LayoutBuffer->Signature);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_PATH, CurrentDisk->PathId);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_TARGET, CurrentDisk->TargetId);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_LUN_ID, CurrentDisk->Lun);

    Entry = VolumeListHead.Flink;
    while (Entry != &VolumeListHead)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (IsDiskInVolume(VolumeEntry, CurrentDisk))
        {
            if (bPrintHeader)
            {
                ConPuts(StdOut, L"\n");
                ConResPuts(StdOut, IDS_LIST_VOLUME_HEAD);
                ConResPuts(StdOut, IDS_LIST_VOLUME_LINE);
                bPrintHeader = FALSE;
            }

            PrintVolume(VolumeEntry);
        }

        Entry = Entry->Flink;
    }

    ConPuts(StdOut, L"\n");

    return TRUE;
}


BOOL
DetailPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PPARTENTRY PartEntry;
    ULONGLONG PartOffset;
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    BOOL bVolumeFound = FALSE, bPrintHeader = TRUE;

    DPRINT("DetailPartition()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_PARTITION_NO_DISK);
        return TRUE;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return TRUE;
    }

    PartEntry = CurrentPartition;
    PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;

    /* TODO: Print more partition details */
    ConPuts(StdOut, L"\n");
    ConResPrintf(StdOut, IDS_DETAIL_PARTITION_NUMBER, PartEntry->PartitionNumber);
    ConResPrintf(StdOut, IDS_DETAIL_PARTITION_TYPE, PartEntry->PartitionType);
    ConResPrintf(StdOut, IDS_DETAIL_PARTITION_HIDDEN, "");
    ConResPrintf(StdOut, IDS_DETAIL_PARTITION_ACTIVE, PartEntry->BootIndicator ? L"Yes" : L"No");
    ConResPrintf(StdOut, IDS_DETAIL_PARTITION_OFFSET, PartOffset);

    Entry = VolumeListHead.Flink;
    while (Entry != &VolumeListHead)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (IsPartitionInVolume(VolumeEntry, CurrentPartition))
        {
            if (bPrintHeader)
            {
                ConPuts(StdOut, L"\n");
                ConResPuts(StdOut, IDS_LIST_VOLUME_HEAD);
                ConResPuts(StdOut, IDS_LIST_VOLUME_LINE);
                bPrintHeader = FALSE;
            }

            PrintVolume(VolumeEntry);
            bVolumeFound = TRUE;
        }

        Entry = Entry->Flink;
    }

    if (bVolumeFound == FALSE)
        ConPuts(StdOut, L"\nThere is no volume associated with this partition.\n");

    ConPuts(StdOut, L"\n");

    return TRUE;
}


BOOL
DetailVolume(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;
    BOOL bDiskFound = FALSE, bPrintHeader = TRUE;

    DPRINT("DetailVolume()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return TRUE;
    }


    Entry = DiskListHead.Flink;
    while (Entry != &DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (IsDiskInVolume(CurrentVolume, DiskEntry))
        {
            if (bPrintHeader)
            {
                ConPuts(StdOut, L"\n");
                ConResPuts(StdOut, IDS_LIST_DISK_HEAD);
                ConResPuts(StdOut, IDS_LIST_DISK_LINE);
                bPrintHeader = FALSE;
            }

            PrintDisk(DiskEntry);
            bDiskFound = TRUE;
        }

        Entry = Entry->Flink;
    }

    if (bDiskFound == FALSE)
        ConPuts(StdOut, L"\nThere are no disks attached to this volume.\n");

    /* TODO: Print more volume details */

    ConPuts(StdOut, L"\n");

    return TRUE;
}
