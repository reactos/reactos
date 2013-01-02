/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/clean.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL clean_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


/*
 * help_clean():
 * Shows the description and explains each argument type of the clean command
 */
VOID help_clean(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_CLEAN);
}
