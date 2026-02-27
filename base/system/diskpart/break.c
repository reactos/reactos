/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/break.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

EXIT_CODE
break_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    ConPuts(StdOut, L"\nTODO: Add code later since Win 7 Home Premium doesn't have this feature.\n");
    return EXIT_SUCCESS;
}
