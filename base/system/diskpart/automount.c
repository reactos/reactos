/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/automount.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL automount_main(INT argc, WCHAR **argv)
{
    printf("Automount\n");
    return TRUE;
}

/*
 * help_automount():
 * Shows the description and explains each argument type of the automount command
 */
VOID help_automount(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_AUTOMOUNT);
}
