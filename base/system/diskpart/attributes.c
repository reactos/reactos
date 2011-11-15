/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/attributes.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL attributes_main(INT argc, WCHAR **argv)
{
    return TRUE;
}

/*
 * help_attributes():
 * Shows the description and explains each argument type of the attributes command
 */
VOID help_attributes(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_ATTRIBUTES);
}
