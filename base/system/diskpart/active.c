/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/active.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


BOOL
active_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    NTSTATUS Status;

    DPRINT("Active()\n");

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

    if (CurrentPartition->BootIndicator)
    {
        ConResPuts(StdOut, IDS_ACTIVE_ALREADY);
        return TRUE;
    }

    CurrentPartition->BootIndicator = TRUE;
    CurrentDisk->Dirty = TRUE;
    UpdateDiskLayout(CurrentDisk);
    Status = WritePartitions(CurrentDisk);
    if (NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_ACTIVE_SUCCESS);
    }
    else
    {
        ConResPuts(StdOut, IDS_ACTIVE_FAIL);
    }

    return TRUE;
}
