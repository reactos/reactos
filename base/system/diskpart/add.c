/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/add.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL add_main(INT argc, WCHAR **argv)
{
    return TRUE;
}

/*
 * help_add():
 * Shows the description and explains each argument type of the add command
 */
VOID help_add(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_ADD);
}
