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

BOOL
DetailDisk(
    INT argc,
    PWSTR *argv)
{
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
    ConPuts(StdOut, L"\n");

    return TRUE;
}


BOOL
DetailPartition(
    INT argc,
    PWSTR *argv)
{
    PPARTENTRY PartEntry;
    ULONGLONG PartOffset;

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
    ConPuts(StdOut, L"\n");

    return TRUE;
}


BOOL
DetailVolume(
    INT argc,
    PWSTR *argv)
{
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

    /* TODO: Print volume details */

    return TRUE;
}
