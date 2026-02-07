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
VOID
PrintSize(
    _In_ ULONGLONG ullSize,
    _Out_ PWSTR pszOutBuffer,
    _In_ ULONG ulOutBufferSize)
{
    WCHAR szUnitBuffer[8];
    INT nUnitId;

    if (ullSize >= SIZE_10TB) /* 10 TB */
    {
        ullSize = RoundingDivide(ullSize, SIZE_1TB);
        nUnitId = IDS_UNIT_TB;
    }
    else if (ullSize >= SIZE_10GB) /* 10 GB */
    {
        ullSize = RoundingDivide(ullSize, SIZE_1GB);
        nUnitId = IDS_UNIT_GB;
    }
    else if (ullSize >= SIZE_10MB) /* 10 MB */
    {
        ullSize = RoundingDivide(ullSize, SIZE_1MB);
        nUnitId = IDS_UNIT_MB;
    }
    else if (ullSize >= SIZE_10KB) /* 10 KB */
    {
        ullSize = RoundingDivide(ullSize, SIZE_1KB);
        nUnitId = IDS_UNIT_KB;
    }
    else
    {
        nUnitId = IDS_UNIT_B;
    }

    LoadStringW(GetModuleHandle(NULL),
                nUnitId,
                szUnitBuffer, ARRAYSIZE(szUnitBuffer));

    swprintf(pszOutBuffer, L"%4I64u %-2s", ullSize, szUnitBuffer);
    StringCchPrintfW(pszOutBuffer,
                     ulOutBufferSize,
                     L"%4I64u %-2s", ullSize, szUnitBuffer);
}


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
    WCHAR szDiskSizeBuffer[8];
    WCHAR szFreeSizeBuffer[8];
    WCHAR szBuffer[40];
    ULONGLONG DiskSize;
    ULONGLONG FreeSize;

    DiskSize = DiskEntry->SectorCount.QuadPart *
               (ULONGLONG)DiskEntry->BytesPerSector;
    PrintSize(DiskSize, szDiskSizeBuffer, ARRAYSIZE(szDiskSizeBuffer));

    FreeSize = GetFreeDiskSize(DiskEntry);
    PrintSize(FreeSize, szFreeSizeBuffer, ARRAYSIZE(szFreeSizeBuffer));

    LoadStringW(GetModuleHandle(NULL),
                IDS_STATUS_ONLINE,
                szBuffer, ARRAYSIZE(szBuffer));

    ConResPrintf(StdOut, IDS_LIST_DISK_FORMAT,
                 (CurrentDisk == DiskEntry) ? L'*' : L' ',
                 DiskEntry->DiskNumber,
                 szBuffer,
                 szDiskSizeBuffer,
                 szFreeSizeBuffer,
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
    ULONG PartNumber = 1;
    BOOL bPartitionFound = FALSE;
    WCHAR szPartitionTypeBuffer[40];
    WCHAR szSizeBuffer[8];
    WCHAR szOffsetBuffer[8];
    INT nPartitionType;

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
                PrintSize(PartSize, szSizeBuffer, ARRAYSIZE(szSizeBuffer));

                PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;
                PrintSize(PartOffset, szOffsetBuffer, ARRAYSIZE(szOffsetBuffer));

                LoadStringW(GetModuleHandle(NULL),
                            IsContainerPartition(PartEntry->Mbr.PartitionType) ? IDS_PARTITION_TYPE_EXTENDED : IDS_PARTITION_TYPE_PRIMARY,
                            szPartitionTypeBuffer, ARRAYSIZE(szPartitionTypeBuffer));

                ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                             (CurrentPartition == PartEntry) ? L'*' : L' ',
                             PartNumber++,
                             szPartitionTypeBuffer,
                             szSizeBuffer,
                             szOffsetBuffer);
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
                PrintSize(PartSize, szSizeBuffer, ARRAYSIZE(szSizeBuffer));

                PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;
                PrintSize(PartOffset, szOffsetBuffer, ARRAYSIZE(szOffsetBuffer));

                LoadStringW(GetModuleHandle(NULL),
                            IDS_PARTITION_TYPE_LOGICAL,
                            szPartitionTypeBuffer, ARRAYSIZE(szPartitionTypeBuffer));
                ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                             (CurrentPartition == PartEntry) ? L'*' : L' ',
                             PartNumber++,
                             szPartitionTypeBuffer,
                             szSizeBuffer,
                             szOffsetBuffer);
            }

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
            {
                PartSize = PartEntry->SectorCount.QuadPart * CurrentDisk->BytesPerSector;
                PrintSize(PartSize, szSizeBuffer, ARRAYSIZE(szSizeBuffer));

                PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;
                PrintSize(PartOffset, szOffsetBuffer, ARRAYSIZE(szOffsetBuffer));

                if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
                {
                    nPartitionType = IDS_PARTITION_TYPE_UNUSED;
                }
                else if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_BASIC_DATA_GUID))
                {
                    nPartitionType = IDS_PARTITION_TYPE_PRIMARY;
                }
                else if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_SYSTEM_GUID))
                {
                    nPartitionType = IDS_PARTITION_TYPE_SYSTEM;
                }
                else if (IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_MSFT_RESERVED_GUID))
                {
                    nPartitionType = IDS_PARTITION_TYPE_RESERVED;
                }
                else
                {
                    nPartitionType = IDS_PARTITION_TYPE_UNKNOWN;
                }

                LoadStringW(GetModuleHandle(NULL),
                            nPartitionType,
                            szPartitionTypeBuffer, ARRAYSIZE(szPartitionTypeBuffer));

                ConResPrintf(StdOut, IDS_LIST_PARTITION_FORMAT,
                             (CurrentPartition == PartEntry) ? L'*' : L' ',
                             PartNumber++,
                             szPartitionTypeBuffer,
                             szSizeBuffer,
                             szOffsetBuffer);
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
    WCHAR szVolumeTypeBuffer[30];
    WCHAR szInfoBuffer[16];
    WCHAR szSizeBuffer[8];
    INT nVolumeType;

    switch (VolumeEntry->VolumeType)
    {
        case VOLUME_TYPE_CDROM:
            nVolumeType = IDS_VOLUME_TYPE_DVD;
            break;

        case VOLUME_TYPE_PARTITION:
            nVolumeType = IDS_VOLUME_TYPE_PARTITION;
            break;

        case VOLUME_TYPE_REMOVABLE:
            nVolumeType = IDS_VOLUME_TYPE_REMOVABLE;
            break;

        case VOLUME_TYPE_UNKNOWN:
        default:
            nVolumeType = IDS_VOLUME_TYPE_UNKNOWN;
            break;
    }

    LoadStringW(GetModuleHandle(NULL), nVolumeType, szVolumeTypeBuffer, ARRAYSIZE(szVolumeTypeBuffer));

    PrintSize(VolumeEntry->Size.QuadPart, szSizeBuffer, ARRAYSIZE(szSizeBuffer));

    szInfoBuffer[0] = UNICODE_NULL;
    if (VolumeEntry->IsSystem)
        LoadStringW(GetModuleHandle(NULL), IDS_INFO_SYSTEM, szInfoBuffer, ARRAYSIZE(szInfoBuffer));
    else if (VolumeEntry->IsBoot)
        LoadStringW(GetModuleHandle(NULL), IDS_INFO_BOOT, szInfoBuffer, ARRAYSIZE(szInfoBuffer));

    ConResPrintf(StdOut, IDS_LIST_VOLUME_FORMAT,
                 (CurrentVolume == VolumeEntry) ? L'*' : L' ',
                 VolumeEntry->VolumeNumber,
                 VolumeEntry->DriveLetter,
                 (VolumeEntry->pszLabel) ? VolumeEntry->pszLabel : L"",
                 (VolumeEntry->pszFilesystem) ? VolumeEntry->pszFilesystem : L"",
                 szVolumeTypeBuffer,
                 szSizeBuffer,
                 L"",
                 szInfoBuffer);
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
    ConPuts(StdOut, L"The LIST VDISK command is not implemented yet!\n");
    return EXIT_SUCCESS;
}
