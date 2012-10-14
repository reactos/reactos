/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/detach.h
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL detach_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


VOID help_detach(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_DETACH);
}
