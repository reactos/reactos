/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/convert.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL convert_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


/*
 * help_convert():
 * Shows the description and explains each argument type of the convert command
 */
VOID help_convert(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_CONVERT);
}
