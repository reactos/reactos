/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/uniqueid.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

BOOL
UniqueIdDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PWSTR pszSuffix = NULL;
    ULONG ulValue;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    if (argc == 2)
    {
        ConPuts(StdOut, L"\n");
        ConPrintf(StdOut, L"Disk ID: %08lx\n", CurrentDisk->LayoutBuffer->Signature);
        ConPuts(StdOut, L"\n");
        return TRUE;
    }

    if (argc != 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if (!HasPrefix(argv[2], L"ID=", &pszSuffix))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if ((pszSuffix == NULL) ||
        (wcslen(pszSuffix) > 8) ||
        (IsHexString(pszSuffix) == FALSE))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    ulValue = wcstoul(pszSuffix, NULL, 16);
    if ((ulValue == 0) && (errno == ERANGE))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    ConPrintf(StdOut, L"Setting the disk signature is not implemented yet!\n");
#if 0
    DPRINT1("New Signature: %lx\n", ulValue);
    CurrentDisk->LayoutBuffer->Signature = ulValue;
//    SetDiskLayout(CurrentDisk);
#endif

    return TRUE;
}
