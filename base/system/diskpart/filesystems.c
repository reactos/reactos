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

static
VOID
ShowFileSystemInfo(
    _In_ PVOLENTRY VolumeEntry)
{
    WCHAR szBuffer[32];
    PWSTR pszSizeUnit = L"";
    ULONG ulClusterSize;

    ConResPuts(StdOut, IDS_FILESYSTEMS_CURRENT);
    ConPuts(StdOut, L"\n");

    ConResPrintf(StdOut, IDS_FILESYSTEMS_TYPE, VolumeEntry->pszFilesystem);

    ulClusterSize = VolumeEntry->SectorsPerAllocationUnit * VolumeEntry->BytesPerSector;
    if (ulClusterSize >= SIZE_10MB) /* 10 MB */
    {
        ulClusterSize = RoundingDivide(ulClusterSize, SIZE_1MB);
        pszSizeUnit = L"MB";
    }
    else if (ulClusterSize >= SIZE_10KB) /* 10 KB */
    {
        ulClusterSize = RoundingDivide(ulClusterSize, SIZE_1KB);
        pszSizeUnit = L"KB";
    }

    wsprintf(szBuffer, L"%lu %s", ulClusterSize, pszSizeUnit);
    ConResPrintf(StdOut, IDS_FILESYSTEMS_CLUSTERSIZE, szBuffer);
    ConResPrintf(StdOut, IDS_FILESYSTEMS_SERIAL_NUMBER, VolumeEntry->SerialNumber);
    ConPuts(StdOut, L"\n");
}


static
VOID
ShowInstalledFileSystems(
    _In_ PVOLENTRY VolumeEntry)
{
    WCHAR szBuffer[256];
    WCHAR szDefault[32];
    BOOLEAN ret;
    DWORD dwIndex;
    UCHAR uMajor, uMinor;
    BOOLEAN bLatest;

    LoadStringW(GetModuleHandle(NULL),
                IDS_FILESYSTEMS_DEFAULT,
                szDefault, ARRAYSIZE(szDefault));

    ConResPuts(StdOut, IDS_FILESYSTEMS_FORMATTING);
    ConPuts(StdOut, L"\n");

    for (dwIndex = 0; ; dwIndex++)
    {
        ret = QueryAvailableFileSystemFormat(dwIndex,
                                             szBuffer,
                                             &uMajor,
                                             &uMinor,
                                             &bLatest);
        if (ret == FALSE)
            break;

        if (wcscmp(szBuffer, L"FAT") == 0)
            wcscat(szBuffer, szDefault);

        ConResPrintf(StdOut, IDS_FILESYSTEMS_TYPE, szBuffer);
        wcscpy(szBuffer, L"-");
        ConResPrintf(StdOut, IDS_FILESYSTEMS_CLUSTERSIZE, szBuffer);
        ConPuts(StdOut, L"\n");
    }
}


EXIT_CODE
filesystems_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return EXIT_SUCCESS;
    }

    ConPuts(StdOut, L"\n");

    ShowFileSystemInfo(CurrentVolume);
    ShowInstalledFileSystems(CurrentVolume);

    return EXIT_SUCCESS;
}
