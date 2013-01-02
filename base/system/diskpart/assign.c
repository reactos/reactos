/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/assign.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL assign_main(INT argc, WCHAR **argv)
{
    return TRUE;
}

/*
 * help_assign():
 * Shows the description and explains each argument type of the assign command
 */
VOID help_assign(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_ASSIGN);
}
