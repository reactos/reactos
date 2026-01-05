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

static
ULONGLONG
GetFreeDiskSize(
    _In_ PDISKENTRY DiskEntry)
{
    ULONGLONG SectorCount;
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;

    if (DiskEntry->PartitionStyle == PARTITION_STYLE_MBR)
    {
        SectorCount = DiskEntry->EndSector.QuadPart - DiskEntry->StartSector.QuadPart + 1;

        Entry = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            if ((PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED) && 
                !IsContainerPartition(PartEntry->Mbr.PartitionType))
            {
                SectorCount -= PartEntry->SectorCount.QuadPart;
            }

            Entry = Entry->Flink;
        }

        Entry = DiskEntry->LogicalPartListHead.Flink;
        while (Entry != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            if (PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED)
            {
                SectorCount -= PartEntry->SectorCount.QuadPart;
            }

            Entry = Entry->Flink;
        }
    }
    else if (DiskEntry->PartitionStyle == PARTITION_STYLE_GPT)
    {
        SectorCount = DiskEntry->EndSector.QuadPart - DiskEntry->StartSector.QuadPart + 1;

        Entry = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            if (!IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
            {
                SectorCount -= PartEntry->SectorCount.QuadPart;
            }

            Entry = Entry->Flink;
        }
    }
    else
    {
        SectorCount = DiskEntry->SectorCount.QuadPart;
    }

    return SectorCount * DiskEntry->BytesPerSector;
}


VOID
PrintDisk(
    _In_ PDISKENTRY DiskEntry)
{
    ULONGLONG DiskSize;
    ULONGLONG FreeSize;
    LPWSTR lpSizeUnit;
    LPWSTR lpFreeUnit;

    DiskSize = DiskEntry->SectorCount.QuadPart *
               (ULONGLONG)DiskEntry->BytesPerSector;

    if (DiskSize >= SIZE_10TB) /* 10 TB */
    {
        DiskSize = RoundingDivide(DiskSize, SIZE_1TB);
        lpSizeUnit = L"TB";
    }
    else if (DiskSize >= SIZE_10GB) /* 10 GB */
    {
        DiskSize = RoundingDivide(DiskSize, SIZE_1GB);
        lpSizeUnit = L"GB";
    }
    else
    {
        DiskSize = RoundingDivide(DiskSize, SIZE_1MB);
        if (DiskSize == 0)
            DiskSize = 1;
        lpSizeUnit = L"MB";
    }

    FreeSize = GetFreeDiskSize(DiskEntry);
    if (FreeSize >= SIZE_10TB) /* 10 TB */
    {
        FreeSize = RoundingDivide(FreeSize, SIZE_1TB);
        lpFreeUnit = L"TB";
    }
    else if (FreeSize >= SIZE_10GB) /* 10 GB */
    {
        FreeSize = RoundingDivide(FreeSize, SIZE_1GB);
        lpFreeUnit = L"GB";
    }
    else if (FreeSize >= SIZE_10MB) /* 10 MB */
    {
        FreeSize = RoundingDivide(FreeSize, SIZE_1MB);
        lpFreeUnit = L"MB";
    }
    else if (FreeSize >= SIZE_10KB) /* 10 KB */
    {
        FreeSize = RoundingDivide(FreeSize, SIZE_1KB);
        lpFreeUnit = L"KB";
    }
    else
    {
        lpFreeUnit = L"B";
    }

    ConResPrintf(StdOut, IDS_LIST_DISK_FORMAT,
                 (CurrentDisk == DiskEntry) ? L'*' : L' ',
                 DiskEntry->DiskNumber,
                 L"Online",
                 DiskSize,
                 lpSizeUnit,
                 FreeSize,
                 lpFreeUnit,
                 L" ",
                 (DiskEntry->PartitionStyle == PARTITION_STYLE_GPT) ? L"*" : L" ");
}


EXIT_CODE
ListDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;

    /* Header labels */
    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, IDS_LIST_DISK_HEAD);
    ConResPuts(StdOut, IDS_LIST_DISK_LINE);

    Entry = DiskListHead.Flink;
    while (Entry != &DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        PrintDisk(DiskEntry);

        Entry = Entry->Flink;
    }

    ConPuts(StdOut, L"\n\n");

    return EXIT_SUCCESS;
}


EXIT_CODE
ListPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    ULONGLONG PartSize;
    ULONGLONG PartOffset;
    LPWSTR lpSizeUnit;
    LPWSTR lpOffsetUnit;
    ULONG PartNumber = 1;
    BOOL bPartitionFound = FALSE;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_LIST_PARTITION_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        Entry = CurrentDisk->PrimaryPartListHead.Flink;
        while (Entry != &CurrentDisk->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
            if (PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED)
                bPartitionFound = TRUE;

            Entry = Entry->Flink;
        }
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        Entry = CurrentDisk->PrimaryPartListHead.Flink;
        while (Entry != &CurrentDisk->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
            if (!IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
                bPartitionFound = TRUE;

            Entry = Entry->Flink;
        }
    }

    if (bPartitionFound == FALSE)
    {
        ConPuts(StdOut, L"\n");
        ConResPuts(StdOut, IDS_LIST_PARTITION_NONE);
        ConPuts(StdOut, L"\n");
        return EXIT_SUCCESS;
    }

    /* Header labels */
    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, IDS_LIST_PARTITION_HEAD);
    ConResPuts(StdOut, IDS_LIST_PARTITION_LINE);

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        Entry = CurrentDisk->PrimaryPartListHead.Flink;
        while (Entry != &CurrentDisk->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            if (PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED)
            {
                PartSize = PartEntry->SectorCount.QuadPart * CurrentDisk->BytesPerSector;

                if (PartSize >= SIZE_10TB) /* 10 TB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1TB);
                    lpSizeUnit = L"TB";
                }
                else if (PartSize >= SIZE_10GB) /* 10 GB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1GB);
                    lpSizeUnit = L"GB";
                }
                else if (PartSize >= SIZE_10MB) /* 10 MB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1MB);
                    lpSizeUnit = L"MB";
                }
                else
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1KB);
                    lpSizeUnit = L"KB";
                }

                PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;

                if (PartOffset >= SIZE_10TB) /* 10 TB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1TB);
                    lpOffsetUnit = L"TB";
                }
                else if (PartOffset >= SIZE_10GB) /* 10 GB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1GB);
                    lpOffsetUnit = L"GB";
                }
                else if (PartOffset >= SIZE_10MB) /* 10 MB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1MB);
                    lpOffsetUnit = L"MB";
                }
                else
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1KB);
                    lpOffsetUnit = L"KB";
                }

                ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                             (CurrentPartition == PartEntry) ? L'*' : L' ',
                             PartNumber++,
                             IsContainerPartition(PartEntry->Mbr.PartitionType) ? L"Extended" : L"Primary",
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

            if (PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED)
            {
                PartSize = PartEntry->SectorCount.QuadPart * CurrentDisk->BytesPerSector;

                if (PartSize >= SIZE_10TB) /* 10 TB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1TB);
                    lpSizeUnit = L"TB";
                }
                else if (PartSize >= SIZE_10GB) /* 10 GB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1GB);
                    lpSizeUnit = L"GB";
                }
                else if (PartSize >= SIZE_10MB) /* 10 MB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1MB);
                    lpSizeUnit = L"MB";
                }
                else
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1KB);
                    lpSizeUnit = L"KB";
                }

                PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;

                if (PartOffset >= SIZE_10TB) /* 10 TB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1TB);
                    lpOffsetUnit = L"TB";
                }
                else if (PartOffset >= SIZE_10GB) /* 10 GB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1GB);
                    lpOffsetUnit = L"GB";
                }
                else if (PartOffset >= SIZE_10MB) /* 10 MB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1MB);
                    lpOffsetUnit = L"MB";
                }
                else
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1KB);
                    lpOffsetUnit = L"KB";
                }

                ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                             (CurrentPartition == PartEntry) ? L'*' : L' ',
                             PartNumber++,
                             L"Logical",
                             PartSize,
                             lpSizeUnit,
                             PartOffset,
                             lpOffsetUnit);
            }

            Entry = Entry->Flink;
        }
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        LPWSTR lpPartitionType;

        Entry = CurrentDisk->PrimaryPartListHead.Flink;
        while (Entry != &CurrentDisk->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            if (!IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
            {
                PartSize = PartEntry->SectorCount.QuadPart * CurrentDisk->BytesPerSector;

                if (PartSize >= SIZE_10TB) /* 10 TB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1TB);
                    lpSizeUnit = L"TB";
                }
                else if (PartSize >= SIZE_10GB) /* 10 GB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1GB);
                    lpSizeUnit = L"GB";
                }
                else if (PartSize >= SIZE_10MB) /* 10 MB */
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1MB);
                    lpSizeUnit = L"MB";
                }
                else
                {
                    PartSize = RoundingDivide(PartSize, SIZE_1KB);
                    lpSizeUnit = L"KB";
                }

                PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;

                if (PartOffset >= SIZE_10TB) /* 10 TB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1TB);
                    lpOffsetUnit = L"TB";
                }
                else if (PartOffset >= SIZE_10GB) /* 10 GB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1GB);
                    lpOffsetUnit = L"GB";
                }
                else if (PartOffset >= SIZE_10MB) /* 10 MB */
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1MB);
                    lpOffsetUnit = L"MB";
                }
                else
                {
                    PartOffset = RoundingDivide(PartOffset, SIZE_1KB);
                    lpOffsetUnit = L"KB";
                }

                if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
                {
                    lpPartitionType = L"Unused";
                }
                else if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_BASIC_DATA_GUID))
                {
                    lpPartitionType = L"Primary";
                }
                else if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_SYSTEM_GUID))
                {
                    lpPartitionType = L"System";
                }
                else if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_MSFT_RESERVED_GUID))
                {
                    lpPartitionType = L"Reserved";
                }
                else
                {
                    lpPartitionType = L"Other"; /* ??? */
                }

                ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                             (CurrentPartition == PartEntry) ? L'*' : L' ',
                             PartNumber++,
                             lpPartitionType,
                             PartSize,
                             lpSizeUnit,
                             PartOffset,
                             lpOffsetUnit);
            }

            Entry = Entry->Flink;
        }
    }

    ConPuts(StdOut, L"\n");

    return EXIT_SUCCESS;
}


VOID
PrintVolume(
    _In_ PVOLENTRY VolumeEntry)
{
    ULONGLONG VolumeSize;
    PWSTR pszSizeUnit;
    PWSTR pszVolumeType;

    VolumeSize = VolumeEntry->Size.QuadPart;
    if (VolumeSize >= SIZE_10TB) /* 10 TB */
    {
        VolumeSize = RoundingDivide(VolumeSize, SIZE_1TB);
        pszSizeUnit = L"TB";
    }
    else if (VolumeSize >= SIZE_10GB) /* 10 GB */
    {
        VolumeSize = RoundingDivide(VolumeSize, SIZE_1GB);
        pszSizeUnit = L"GB";
    }
    else if (VolumeSize >= SIZE_10MB) /* 10 MB */
    {
        VolumeSize = RoundingDivide(VolumeSize, SIZE_1MB);
        pszSizeUnit = L"MB";
    }
    else
    {
        VolumeSize = RoundingDivide(VolumeSize, SIZE_1KB);
        pszSizeUnit = L"KB";
    }

    switch (VolumeEntry->VolumeType)
    {
        case VOLUME_TYPE_CDROM:
            pszVolumeType = L"DVD";
            break;
        case VOLUME_TYPE_PARTITION:
            pszVolumeType = L"Partition";
            break;
        case VOLUME_TYPE_REMOVABLE:
            pszVolumeType = L"Removable";
            break;
        case VOLUME_TYPE_UNKNOWN:
        default:
            pszVolumeType = L"Unknown";
            break;
    }

    ConResPrintf(StdOut, IDS_LIST_VOLUME_FORMAT,
                 (CurrentVolume == VolumeEntry) ? L'*' : L' ',
                 VolumeEntry->VolumeNumber,
                 VolumeEntry->DriveLetter,
                 (VolumeEntry->pszLabel) ? VolumeEntry->pszLabel : L"",
                 (VolumeEntry->pszFilesystem) ? VolumeEntry->pszFilesystem : L"",
                 pszVolumeType,
                 VolumeSize, pszSizeUnit,
                 L"", L"");
}


EXIT_CODE
ListVolume(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;

    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, IDS_LIST_VOLUME_HEAD);
    ConResPuts(StdOut, IDS_LIST_VOLUME_LINE);

    Entry = VolumeListHead.Flink;
    while (Entry != &VolumeListHead)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        PrintVolume(VolumeEntry);

        Entry = Entry->Flink;
    }

    ConPuts(StdOut, L"\n");

    return EXIT_SUCCESS;
}


EXIT_CODE
ListVirtualDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    ConPuts(StdOut, L"ListVirtualDisk()!\n");
    return EXIT_SUCCESS;
}
