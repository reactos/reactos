/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/setid.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


BOOL
setid_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    UCHAR PartitionType = 0;
    INT i, length;
    PWSTR pszSuffix = NULL;
    NTSTATUS Status;

    DPRINT("SetId()\n");

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

    for (i = 1; i < argc; i++)
    {
        if (HasPrefix(argv[i], L"id=", &pszSuffix))
        {
            /* id=<Byte>|<GUID> */
            DPRINT("Id : %s\n", pszSuffix);

            length = wcslen(pszSuffix);
            if (length == 0)
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }

            if (length > 2)
            {
                ConResPuts(StdErr, IDS_SETID_INVALID_FORMAT);
                return TRUE;
            }

            /* Byte */
            PartitionType = (UCHAR)wcstoul(pszSuffix, NULL, 16);
            if (PartitionType == 0)
            {
                ConResPuts(StdErr, IDS_SETID_INVALID_FORMAT);
                return TRUE;
            }
        }
    }

    if (PartitionType == 0x42)
    {
        ConResPuts(StdErr, IDS_SETID_INVALID_TYPE);
        return TRUE;
    }

    CurrentPartition->PartitionType = PartitionType;
    CurrentDisk->Dirty = TRUE;
    UpdateDiskLayout(CurrentDisk);
    Status = WritePartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_SETID_FAIL);
        return TRUE;
    }

    ConResPuts(StdOut, IDS_SETID_SUCCESS);

    return TRUE;
}
