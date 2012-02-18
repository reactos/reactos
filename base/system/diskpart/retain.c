/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/retain.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL retain_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


VOID help_retain(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_RETAIN);
}
