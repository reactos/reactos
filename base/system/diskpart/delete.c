/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/delete.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

static
BOOL
IsKnownPartition(
    _In_ PPARTENTRY PartEntry)
{
    if (IsRecognizedPartition(PartEntry->PartitionType) ||
        IsContainerPartition(PartEntry->PartitionType))
        return TRUE;

    return FALSE;
}


BOOL
DeleteDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    return TRUE;
}


BOOL
DeletePartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PPARTENTRY PrevPartEntry;
    PPARTENTRY NextPartEntry;
    PPARTENTRY LogicalPartEntry;
    PLIST_ENTRY Entry;
    INT i;
    BOOL bOverride = FALSE;
    NTSTATUS Status;

    DPRINT("DeletePartition()\n");

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

    ASSERT(CurrentPartition->PartitionType != PARTITION_ENTRY_UNUSED);

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NOERR\n");
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
        else if (_wcsicmp(argv[i], L"override") == 0)
        {
            /* override */
            DPRINT("OVERRIDE\n");
            bOverride = TRUE;
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }


    /* Clear the system partition if it is being deleted */
#if 0
    if (List->SystemPartition == PartEntry)
    {
        ASSERT(List->SystemPartition);
        List->SystemPartition = NULL;
    }
#endif

    if ((bOverride == FALSE) && (IsKnownPartition(CurrentPartition) == FALSE))
    {
        ConResPuts(StdOut, IDS_DELETE_PARTITION_FAIL);
        return FALSE;
    }

    /* Check which type of partition (primary/logical or extended) is being deleted */
    if (CurrentDisk->ExtendedPartition == CurrentPartition)
    {
        /* An extended partition is being deleted: delete all logical partition entries */
        while (!IsListEmpty(&CurrentDisk->LogicalPartListHead))
        {
            Entry = RemoveHeadList(&CurrentDisk->LogicalPartListHead);
            LogicalPartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            /* Dismount the logical partition */
            DismountVolume(LogicalPartEntry);

            /* Delete it */
            RtlFreeHeap(RtlGetProcessHeap(), 0, LogicalPartEntry);
        }

        CurrentDisk->ExtendedPartition = NULL;
    }
    else
    {
        /* A primary partition is being deleted: dismount it */
        DismountVolume(CurrentPartition);
    }

    /* Adjust the unpartitioned disk space entries */

    /* Get pointer to previous and next unpartitioned entries */
    PrevPartEntry = GetPrevUnpartitionedEntry(CurrentPartition);
    NextPartEntry = GetNextUnpartitionedEntry(CurrentPartition);

    if (PrevPartEntry != NULL && NextPartEntry != NULL)
    {
        /* Merge the previous, current and next unpartitioned entries */

        /* Adjust the previous entry length */
        PrevPartEntry->SectorCount.QuadPart += (CurrentPartition->SectorCount.QuadPart + NextPartEntry->SectorCount.QuadPart);

        /* Remove the current and next entries */
        RemoveEntryList(&CurrentPartition->ListEntry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentPartition);
        RemoveEntryList(&NextPartEntry->ListEntry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NextPartEntry);
    }
    else if (PrevPartEntry != NULL && NextPartEntry == NULL)
    {
        /* Merge the current and the previous unpartitioned entries */

        /* Adjust the previous entry length */
        PrevPartEntry->SectorCount.QuadPart += CurrentPartition->SectorCount.QuadPart;

        /* Remove the current entry */
        RemoveEntryList(&CurrentPartition->ListEntry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentPartition);
    }
    else if (PrevPartEntry == NULL && NextPartEntry != NULL)
    {
        /* Merge the current and the next unpartitioned entries */

        /* Adjust the next entry offset and length */
        NextPartEntry->StartSector.QuadPart = CurrentPartition->StartSector.QuadPart;
        NextPartEntry->SectorCount.QuadPart += CurrentPartition->SectorCount.QuadPart;

        /* Remove the current entry */
        RemoveEntryList(&CurrentPartition->ListEntry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentPartition);
    }
    else
    {
        /* Nothing to merge but change the current entry */
        CurrentPartition->IsPartitioned = FALSE;
        CurrentPartition->OnDiskPartitionNumber = 0;
        CurrentPartition->PartitionNumber = 0;
        // CurrentPartition->PartitionIndex = 0;
        CurrentPartition->BootIndicator = FALSE;
        CurrentPartition->PartitionType = PARTITION_ENTRY_UNUSED;
        CurrentPartition->FormatState = Unformatted;
        CurrentPartition->FileSystemName[0] = L'\0';
        CurrentPartition->DriveLetter = 0;
        RtlZeroMemory(CurrentPartition->VolumeLabel, sizeof(CurrentPartition->VolumeLabel));
    }

    CurrentPartition = NULL;

    UpdateDiskLayout(CurrentDisk);
    Status = WritePartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_DELETE_PARTITION_FAIL);
        return TRUE;
    }

    ConResPuts(StdOut, IDS_DELETE_PARTITION_SUCCESS);

    return TRUE;
}


BOOL
DeleteVolume(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    return TRUE;
}
