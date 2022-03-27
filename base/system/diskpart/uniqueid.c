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

static
VOID
UniqueIdDisk(
    _In_ INT argc,
    _In_ LPWSTR *argv)
{
    ULONG ulLength, ulValue;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return;
    }

    if (argc == 2)
    {
        ConPuts(StdOut, L"\n");
        ConPrintf(StdOut, L"Disk ID: %08lx\n", CurrentDisk->LayoutBuffer->Signature);
        ConPuts(StdOut, L"\n");
        return;
    }

    if (argc != 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    ulLength = wcslen(argv[2]);
    if ((ulLength <= 3) || (ulLength > 11))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (!HasPrefix(argv[2], L"ID="))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (!IsHexString(&argv[2][3]))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    ulValue = wcstoul(&argv[2][3], NULL, 16);
    if ((ulValue == 0) && (errno == ERANGE))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return;
    }

    ConPrintf(StdOut, L"Setting the disk signature is not implemented yet!\n");
#if 0
    DPRINT1("New Signature: %lx\n", ulValue);
    CurrentDisk->LayoutBuffer->Signature = ulValue;
//    SetDiskLayout(CurrentDisk);
#endif

}


BOOL uniqueid_main(INT argc, LPWSTR *argv)
{
    /* gets the first word from the string */
    if (argc == 1)
    {
        ConResPuts(StdOut, IDS_HELP_CMD_UNIQUEID);
        return TRUE;
    }

    /* determines which details to print (disk, partition, etc.) */
    if (!wcsicmp(argv[1], L"disk"))
        UniqueIdDisk(argc, argv);
    else
        ConResPuts(StdOut, IDS_HELP_CMD_UNIQUEID);

    return TRUE;
}
