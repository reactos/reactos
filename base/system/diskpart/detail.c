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
VOID
DetailDisk(
    INT argc,
    LPWSTR *argv)
{
    DPRINT("DetailDisk()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return;
    }

    /* TODO: Print more disk details */
    ConPuts(StdOut, L"\n");
    ConResPrintf(StdOut, IDS_DETAIL_INFO_DISK_ID, CurrentDisk->LayoutBuffer->Signature);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_PATH, CurrentDisk->PathId);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_TARGET, CurrentDisk->TargetId);
    ConResPrintf(StdOut, IDS_DETAIL_INFO_LUN_ID, CurrentDisk->Lun);
    ConPuts(StdOut, L"\n");
}


static
VOID
DetailPartition(
    INT argc,
    LPWSTR *argv)
{
    PPARTENTRY PartEntry;
    ULONGLONG PartOffset;

    DPRINT("DetailPartition()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_PARTITION_NO_DISK);
        return;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return;
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
}


static
VOID
DetailVolume(
    INT argc,
    LPWSTR *argv)
{
    DPRINT("DetailVolume()\n");

    if (argc > 2)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return;
    }

    /* TODO: Print volume details */

}


BOOL
detail_main(
    INT argc,
    LPWSTR *argv)
{
    /* gets the first word from the string */
    if (argc == 1)
    {
        ConResPuts(StdOut, IDS_HELP_CMD_DETAIL);
        return TRUE;
    }

    /* determines which details to print (disk, partition, etc.) */
    if (!wcsicmp(argv[1], L"disk"))
        DetailDisk(argc, argv);
    else if (!wcsicmp(argv[1], L"partition"))
        DetailPartition(argc, argv);
    else if (!wcsicmp(argv[1], L"volume"))
        DetailVolume(argc, argv);
    else
        ConResPuts(StdOut, IDS_HELP_CMD_DETAIL);

    return TRUE;
}
