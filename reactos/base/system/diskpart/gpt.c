/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/gpt.c
 * PURPOSE:         Manages all the partitions of the OS in
 *					an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#include "diskpart.h"

BOOL gpt_main(INT argc, WCHAR **argv)
{
    return TRUE;
}

VOID help_gpt(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_GPT);
}
