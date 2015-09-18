/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/online.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL online_main(INT argc, LPWSTR *argv)
{
    PrintResourceString(IDS_HELP_CMD_ONLINE);

    return TRUE;
}
