/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/break.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL break_main(INT argc, WCHAR **argv)
{
    printf("\nTODO: Add code later since Win 7 Home Premium doesn't have this feature.\n");

    return TRUE;
}

/*
 * help_break():
 * Shows the description and explains each argument type of the break command
 */
VOID help_break(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_BREAK);
}
