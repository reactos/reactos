/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/active.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL active_main(INT argc, WCHAR **argv)
{
    printf("\nActive\n");

    return TRUE;
}

/*
 * help_active():
 * Shows the description and explains each argument type of the active command
 */
VOID help_active(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_ACTIVE);
}
