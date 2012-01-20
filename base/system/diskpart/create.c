/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/create.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL create_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


/*
 * help_create():
 * Shows the description and explains each argument type of the create command
 */
VOID help_create(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_CREATE);
}
