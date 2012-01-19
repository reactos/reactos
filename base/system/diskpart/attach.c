/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/attach.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL attach_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


/*
 * help_attach():
 * Shows the description and explains each argument type of the attach command
 */
VOID help_attach(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_ATTACH);
}
