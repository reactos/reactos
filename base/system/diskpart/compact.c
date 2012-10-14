/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/compact.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL compact_main(INT argc, WCHAR **argv)
{
    return 0;
}


/*
 * help_compact():
 * Shows the description and explains each argument type of the compact command
 */
VOID help_compact(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_COMPACT);
}
