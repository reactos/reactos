/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/inactive.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


EXIT_CODE
inactive_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    NTSTATUS Status;

    DPRINT("Inactive()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        if (!CurrentPartition->Mbr.BootIndicator)
        {
            ConResPuts(StdOut, IDS_INACTIVE_ALREADY);
            return EXIT_SUCCESS;
        }

        CurrentPartition->Mbr.BootIndicator = FALSE;
        CurrentDisk->Dirty = TRUE;
        UpdateMbrDiskLayout(CurrentDisk);
        Status = WriteMbrPartitions(CurrentDisk);
        if (NT_SUCCESS(Status))
        {
            ConResPuts(StdOut, IDS_INACTIVE_SUCCESS);
        }
        else
        {
            ConResPuts(StdOut, IDS_INACTIVE_FAIL);
        }
    }
    else
    {
        ConResPuts(StdOut, IDS_INACTIVE_NO_MBR);
    }

    return EXIT_SUCCESS;
}
