/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/list.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

BOOL
ListDisk(
    INT argc,
    PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;
    ULONGLONG DiskSize;
    ULONGLONG FreeSize;
    LPWSTR lpSizeUnit;
    LPWSTR lpFreeUnit;

    /* Header labels */
    ConResPuts(StdOut, IDS_LIST_DISK_HEAD);
    ConResPuts(StdOut, IDS_LIST_DISK_LINE);

    Entry = DiskListHead.Flink;
    while (Entry != &DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        DiskSize = DiskEntry->SectorCount.QuadPart *
                   (ULONGLONG)DiskEntry->BytesPerSector;

        if (DiskSize >= 10737418240) /* 10 GB */
        {
             DiskSize = RoundingDivide(DiskSize, 1073741824);
             lpSizeUnit = L"GB";
        }
        else
        {
             DiskSize = RoundingDivide(DiskSize, 1048576);
             if (DiskSize == 0)
                 DiskSize = 1;
             lpSizeUnit = L"MB";
        }

        /* FIXME */
        FreeSize = 0;
        lpFreeUnit = L"B";

        ConResPrintf(StdOut, IDS_LIST_DISK_FORMAT,
                     (CurrentDisk == DiskEntry) ? L'*': ' ',
                     DiskEntry->DiskNumber,
                     L"Online",
                     DiskSize,
                     lpSizeUnit,
                     FreeSize,
                     lpFreeUnit,
                     L" ",
                     L" ");

        Entry = Entry->Flink;
    }

    ConPuts(StdOut, L"\n\n");

    return TRUE;
}


BOOL
ListPartition(
    INT argc,
    PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    ULONGLONG PartSize;
    ULONGLONG PartOffset;
    LPWSTR lpSizeUnit;
    LPWSTR lpOffsetUnit;
    ULONG PartNumber = 1;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_LIST_PARTITION_NO_DISK);
        return TRUE;
    }

    /* Header labels */
    ConResPuts(StdOut, IDS_LIST_PARTITION_HEAD);
    ConResPuts(StdOut, IDS_LIST_PARTITION_LINE);

    Entry = CurrentDisk->PrimaryPartListHead.Flink;
    while (Entry != &CurrentDisk->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionType != 0)
        {
            PartSize = PartEntry->SectorCount.QuadPart * CurrentDisk->BytesPerSector;

            if (PartSize >= 10737418240) /* 10 GB */
            {
                PartSize = RoundingDivide(PartSize, 1073741824);
                lpSizeUnit = L"GB";
            }
            else if (PartSize >= 10485760) /* 10 MB */
            {
                PartSize = RoundingDivide(PartSize, 1048576);
                lpSizeUnit = L"MB";
            }
            else
            {
                PartSize = RoundingDivide(PartSize, 1024);
                lpSizeUnit = L"KB";
            }

            PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;

            if (PartOffset >= 10737418240) /* 10 GB */
            {
                PartOffset = RoundingDivide(PartOffset, 1073741824);
                lpOffsetUnit = L"GB";
            }
            else if (PartOffset >= 10485760) /* 10 MB */
            {
                PartOffset = RoundingDivide(PartOffset, 1048576);
                lpOffsetUnit = L"MB";
            }
            else
            {
                PartOffset = RoundingDivide(PartOffset, 1024);
                lpOffsetUnit = L"KB";
            }

            ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                         (CurrentPartition == PartEntry) ? L'*': ' ',
                         PartNumber++,
                         IsContainerPartition(PartEntry->PartitionType) ? L"Extended" : L"Primary",
                         PartSize,
                         lpSizeUnit,
                         PartOffset,
                         lpOffsetUnit);
        }

        Entry = Entry->Flink;
    }

    Entry = CurrentDisk->LogicalPartListHead.Flink;
    while (Entry != &CurrentDisk->LogicalPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionType != 0)
        {
            PartSize = PartEntry->SectorCount.QuadPart * CurrentDisk->BytesPerSector;

            if (PartSize >= 10737418240) /* 10 GB */
            {
                PartSize = RoundingDivide(PartSize, 1073741824);
                lpSizeUnit = L"GB";
            }
            else if (PartSize >= 10485760) /* 10 MB */
            {
                PartSize = RoundingDivide(PartSize, 1048576);
                lpSizeUnit = L"MB";
            }
            else
            {
                PartSize = RoundingDivide(PartSize, 1024);
                lpSizeUnit = L"KB";
            }

            PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;

            if (PartOffset >= 10737418240) /* 10 GB */
            {
                PartOffset = RoundingDivide(PartOffset, 1073741824);
                lpOffsetUnit = L"GB";
            }
            else if (PartOffset >= 10485760) /* 10 MB */
            {
                PartOffset = RoundingDivide(PartOffset, 1048576);
                lpOffsetUnit = L"MB";
            }
            else
            {
                PartOffset = RoundingDivide(PartOffset, 1024);
                lpOffsetUnit = L"KB";
            }

            ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                         (CurrentPartition == PartEntry) ? L'*': ' ',
                         PartNumber++,
                         L"Logical",
                         PartSize,
                         lpSizeUnit,
                         PartOffset,
                         lpOffsetUnit);
        }

        Entry = Entry->Flink;
    }

    ConPuts(StdOut, L"\n");

    return TRUE;
}


BOOL
ListVolume(
    INT argc,
    PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    ULONGLONG VolumeSize;
    LPWSTR lpSizeUnit;

    ConResPuts(StdOut, IDS_LIST_VOLUME_HEAD);
    ConResPuts(StdOut, IDS_LIST_VOLUME_LINE);

    Entry = VolumeListHead.Flink;
    while (Entry != &VolumeListHead)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        VolumeSize = VolumeEntry->Size.QuadPart;
        if (VolumeSize >= 10737418240) /* 10 GB */
        {
            VolumeSize = RoundingDivide(VolumeSize, 1073741824);
            lpSizeUnit = L"GB";
        }
        else if (VolumeSize >= 10485760) /* 10 MB */
        {
            VolumeSize = RoundingDivide(VolumeSize, 1048576);
            lpSizeUnit = L"MB";
        }
        else
        {
            VolumeSize = RoundingDivide(VolumeSize, 1024);
            lpSizeUnit = L"KB";
        }

        ConResPrintf(StdOut, IDS_LIST_VOLUME_FORMAT,
                     VolumeEntry->VolumeNumber,
                     VolumeEntry->DriveLetter,
                     (VolumeEntry->pszLabel) ? VolumeEntry->pszLabel : L"",
                     (VolumeEntry->pszFilesystem) ? VolumeEntry->pszFilesystem : L"",
                     VolumeEntry->DriveType,
                     VolumeSize, lpSizeUnit);

        Entry = Entry->Flink;
    }

    ConPuts(StdOut, L"\n\n");

    return TRUE;
}


BOOL
ListVirtualDisk(
    INT argc,
    PWSTR *argv)
{
    ConPuts(StdOut, L"ListVirtualDisk()!\n");
    return TRUE;
}
