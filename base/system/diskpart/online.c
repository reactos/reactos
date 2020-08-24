/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL online_main(INT argc, LPWSTR *argv)
{
    ConResPuts(StdOut, IDS_HELP_CMD_ONLINE);
    return TRUE;
}
