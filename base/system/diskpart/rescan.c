/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
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
