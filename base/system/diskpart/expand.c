/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/expand.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL expand_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


VOID help_expand(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_EXPAND);
}
