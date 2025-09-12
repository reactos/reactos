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


BOOL
inactive_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    NTSTATUS Status;

    DPRINT("Inactive()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return TRUE;
    }

    if (!CurrentPartition->BootIndicator)
    {
        ConResPuts(StdOut, IDS_INACTIVE_ALREADY);
        return TRUE;
    }

    CurrentPartition->BootIndicator = FALSE;
    CurrentDisk->Dirty = TRUE;
    UpdateDiskLayout(CurrentDisk);
    Status = WritePartitions(CurrentDisk);
    if (NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_INACTIVE_SUCCESS);
    }
    else
    {
        ConResPuts(StdOut, IDS_INACTIVE_FAIL);
    }

    return TRUE;
}
