/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/filesystems.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


BOOL
filesystems_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return TRUE;
    }

    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, IDS_FILESYSTEMS_CURRENT);
    ConPuts(StdOut, L"\n");
    ConResPrintf(StdOut, IDS_FILESYSTEMS_TYPE, (CurrentVolume->pszFilesystem == NULL) ? L"RAW" : CurrentVolume->pszFilesystem);
    ConResPrintf(StdOut, IDS_FILESYSTEMS_CLUSTERSIZE);

    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, IDS_FILESYSTEMS_FORMATTING);
    ConPuts(StdOut, L"\n");

    return TRUE;
}
