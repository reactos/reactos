/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/rescan.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

EXIT_CODE
rescan_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    ConResPuts(StdOut, IDS_RESCAN_START);
    DestroyVolumeList();
    DestroyPartitionList();
    CreatePartitionList();
    CreateVolumeList();
    ConResPuts(StdOut, IDS_RESCAN_END);

    return EXIT_SUCCESS;
}
