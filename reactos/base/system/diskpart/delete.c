/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/delete.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL delete_main(INT argc, WCHAR **argv)
{
    return TRUE;
}

/*
 * help_delete():
 * Shows the description and explains each argument type of the delete command
 */
VOID help_delete(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_DELETE);
}
