/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL active_main(INT argc, LPWSTR *argv)
{
    ConPuts(StdOut, L"\nActive\n");
    return TRUE;
}
