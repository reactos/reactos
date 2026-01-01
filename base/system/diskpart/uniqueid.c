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

EXIT_CODE
UniqueIdDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    WCHAR szBuffer[40];
    PWSTR pszSuffix, pszId = NULL;
    INT i;
    ULONG ulValue;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (argc == 2)
    {
        ConPuts(StdOut, L"\n");
        if (CurrentDisk->LayoutBuffer->PartitionStyle == PARTITION_STYLE_GPT)
            PrintGUID(szBuffer, &CurrentDisk->LayoutBuffer->Gpt.DiskId);
        else if (CurrentDisk->LayoutBuffer->PartitionStyle == PARTITION_STYLE_MBR)
            swprintf(szBuffer, L"%08lx", CurrentDisk->LayoutBuffer->Mbr.Signature);
        else
            wcscpy(szBuffer, L"00000000");
        ConPrintf(StdOut, L"Disk ID: %s\n", szBuffer);
        ConPuts(StdOut, L"\n");
        return EXIT_SUCCESS;
    }

    if (argc != 3)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return EXIT_SUCCESS;
    }

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NOERR\n");
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
        }
    }

    for (i = 1; i < argc; i++)
    {
        if (HasPrefix(argv[2], L"id=", &pszSuffix))
        {
            /* id=<Byte>|<GUID> */
            DPRINT("ID : %s\n", pszSuffix);
            pszId = pszSuffix;
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        if (!StringToGUID(&CurrentDisk->LayoutBuffer->Gpt.DiskId, pszId))
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }

        CurrentDisk->Dirty = TRUE;
        UpdateGptDiskLayout(CurrentDisk, FALSE);
        WriteGptPartitions(CurrentDisk);
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        if ((pszId == NULL) ||
            (wcslen(pszId) != 8) ||
            (IsHexString(pszId) == FALSE))
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }

        ulValue = wcstoul(pszId, NULL, 16);
        if ((ulValue == 0) && (errno == ERANGE))
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }

        DPRINT("New Signature: 0x%08lx\n", ulValue);
        CurrentDisk->LayoutBuffer->Mbr.Signature = ulValue;
        CurrentDisk->Dirty = TRUE;
        UpdateMbrDiskLayout(CurrentDisk);
        WriteMbrPartitions(CurrentDisk);
    }
    else
    {
        ConResPuts(StdOut, IDS_UNIQUID_DISK_INVALID_STYLE);
    }

    return EXIT_SUCCESS;
}
