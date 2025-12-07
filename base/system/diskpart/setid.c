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
    PWSTR pszSuffix, pszId = NULL;
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
            DPRINT("ID : %s\n", pszSuffix);
            pszId = pszSuffix;
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NOERR\n");
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
        }
        else if (_wcsicmp(argv[i], L"override") == 0)
        {
            /* override */
            DPRINT("OVERRIDE\n");
            ConPuts(StdOut, L"The OVERRIDE option is not supported yet!\n");
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        ConPuts(StdErr, L"The setid command does not support GPT disks yet!\n");
        return TRUE;
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        length = wcslen(pszId);
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

        PartitionType = (UCHAR)wcstoul(pszSuffix, NULL, 16);
        if ((PartitionType == 0) && (errno == ERANGE))
        {
            ConResPuts(StdErr, IDS_SETID_INVALID_FORMAT);
            return TRUE;
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
    }

    ConResPuts(StdOut, IDS_SETID_SUCCESS);

    return TRUE;
}
