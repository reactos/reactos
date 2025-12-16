/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/gpt.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


BOOL
gpt_main(
    INT argc,
    LPWSTR *argv)
{
    ULONGLONG ullAttributes = 0ULL;
    PWSTR pszSuffix = NULL;
    INT i;
    NTSTATUS Status;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return TRUE;
    }

    if (CurrentDisk->PartitionStyle != PARTITION_STYLE_GPT)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_INVALID_STYLE);
        return TRUE;
    }

    for (i = 1; i < argc; i++)
    {
        if (HasPrefix(argv[i], L"attributes=", &pszSuffix))
        {
            /* attributes=<N> */
            DPRINT("Attributes: %s\n", pszSuffix);

            ullAttributes = _wcstoui64(pszSuffix, NULL, 0);
            if ((ullAttributes == 0) && (errno == ERANGE))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }
        }
       else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }

    DPRINT("Attributes: 0x%I64x\n", ullAttributes);
    CurrentPartition->Gpt.Attributes = ullAttributes;
    CurrentDisk->Dirty = TRUE;

    UpdateGptDiskLayout(CurrentDisk, FALSE);
    Status = WriteGptPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_GPT_FAIL);
        return TRUE;
    }

    ConResPuts(StdOut, IDS_GPT_SUCCESS);

    return TRUE;
}
