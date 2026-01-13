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


EXIT_CODE
DetailDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    BOOL bPrintHeader = TRUE;
    WCHAR szBuffer[40];

    DPRINT("DetailDisk()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    /* TODO: Print more disk details */
    ConPuts(StdOut, L"\n");
    ConResPrintf(StdOut, IDS_DETAIL_DISK_DESCRIPTION, CurrentDisk->Description);
    if (CurrentDisk->LayoutBuffer->PartitionStyle == PARTITION_STYLE_GPT)
        PrintGUID(szBuffer, &CurrentDisk->LayoutBuffer->Gpt.DiskId);
    else if (CurrentDisk->LayoutBuffer->PartitionStyle == PARTITION_STYLE_MBR)
        swprintf(szBuffer, L"%08lx", CurrentDisk->LayoutBuffer->Mbr.Signature);
    else
        wcscpy(szBuffer, L"00000000");
    ConResPrintf(StdOut, IDS_DETAIL_DISK_ID, szBuffer);
    PrintBusType(szBuffer, ARRAYSIZE(szBuffer), CurrentDisk->BusType);
    ConResPrintf(StdOut, IDS_DETAIL_DISK_TYPE, szBuffer);
    LoadStringW(GetModuleHandle(NULL),
                IDS_STATUS_ONLINE,
                szBuffer, ARRAYSIZE(szBuffer));
    ConResPrintf(StdOut, IDS_DETAIL_DISK_STATUS, szBuffer);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_PATH, CurrentDisk->PathId);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_TARGET, CurrentDisk->TargetId);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_LUN_ID, CurrentDisk->Lun);

    LoadStringW(GetModuleHandle(NULL),
                CurrentDisk->IsBoot ? IDS_STATUS_YES : IDS_STATUS_YES,
                szBuffer, ARRAYSIZE(szBuffer));
    ConResPrintf(StdOut, IDS_DETAIL_INFO_BOOT_DSK, szBuffer);

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

    return EXIT_SUCCESS;
}


EXIT_CODE
DetailPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PPARTENTRY PartEntry;
    ULONGLONG PartOffset;
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    BOOL bVolumeFound = FALSE, bPrintHeader = TRUE;
    WCHAR szBuffer[40];

    DPRINT("DetailPartition()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_PARTITION_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return EXIT_SUCCESS;
    }

    PartEntry = CurrentPartition;
    PartOffset = PartEntry->StartSector.QuadPart * CurrentDisk->BytesPerSector;

    /* TODO: Print more partition details */
    ConPuts(StdOut, L"\n");
    ConResPrintf(StdOut, IDS_DETAIL_PARTITION_NUMBER, PartEntry->PartitionNumber);
    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        PrintGUID(szBuffer, &PartEntry->Gpt.PartitionType);
        ConResPrintf(StdOut, IDS_DETAIL_PARTITION_TYPE, szBuffer);
        LoadStringW(GetModuleHandle(NULL),
                    (PartEntry->Gpt.Attributes & GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) ? IDS_STATUS_YES : IDS_STATUS_NO,
                    szBuffer, ARRAYSIZE(szBuffer));
        ConResPrintf(StdOut, IDS_DETAIL_PARTITION_HIDDEN, szBuffer);
        LoadStringW(GetModuleHandle(NULL),
                    (PartEntry->Gpt.Attributes & GPT_ATTRIBUTE_PLATFORM_REQUIRED) ? IDS_STATUS_YES : IDS_STATUS_NO,
                    szBuffer, ARRAYSIZE(szBuffer));
        ConResPrintf(StdOut, IDS_DETAIL_PARTITION_REQUIRED, szBuffer);
        ConResPrintf(StdOut, IDS_DETAIL_PARTITION_ATTRIBUTE, PartEntry->Gpt.Attributes);
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        swprintf(szBuffer, L"%02x", PartEntry->Mbr.PartitionType);
        ConResPrintf(StdOut, IDS_DETAIL_PARTITION_TYPE, szBuffer);
        ConResPrintf(StdOut, IDS_DETAIL_PARTITION_HIDDEN, "");
        LoadStringW(GetModuleHandle(NULL),
                    PartEntry->Mbr.BootIndicator ? IDS_STATUS_YES : IDS_STATUS_NO,
                    szBuffer, ARRAYSIZE(szBuffer));
        ConResPrintf(StdOut, IDS_DETAIL_PARTITION_ACTIVE, szBuffer);
    }
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
        ConResPuts(StdOut, IDS_DETAIL_NO_VOLUME);

    ConPuts(StdOut, L"\n");

    return EXIT_SUCCESS;
}


EXIT_CODE
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
        return EXIT_SUCCESS;
    }

    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return EXIT_SUCCESS;
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
        ConResPuts(StdOut, IDS_DETAIL_NO_DISKS);

    /* TODO: Print more volume details */

    ConPuts(StdOut, L"\n");

    return EXIT_SUCCESS;
}
