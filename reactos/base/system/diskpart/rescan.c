/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/rescan.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL rescan_main(INT argc, LPWSTR *argv)
{
    ConResPuts(StdOut, IDS_RESCAN_START);
    DestroyPartitionList();
    CreatePartitionList();
    ConResPuts(StdOut, IDS_RESCAN_END);

    return TRUE;
}
