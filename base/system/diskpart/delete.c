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
    if (IsRecognizedPartition(PartEntry->Mbr.PartitionType) ||
        IsContainerPartition(PartEntry->Mbr.PartitionType))
        return TRUE;

    return FALSE;
}


EXIT_CODE
DeleteDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    return EXIT_SUCCESS;
}


static
VOID
DeleteMbrPartition(
    _In_ BOOL bOverride)
{
    PPARTENTRY PrevPartEntry;
    PPARTENTRY NextPartEntry;
    PPARTENTRY LogicalPartEntry;
    PLIST_ENTRY Entry;
    NTSTATUS Status;

    ASSERT(CurrentPartition->Mbr.PartitionType != PARTITION_ENTRY_UNUSED);

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
        return;
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
        CurrentPartition->Mbr.BootIndicator = FALSE;
        CurrentPartition->Mbr.PartitionType = PARTITION_ENTRY_UNUSED;
        CurrentPartition->FormatState = Unformatted;
        CurrentPartition->FileSystemName[0] = L'\0';
        CurrentPartition->DriveLetter = 0;
        RtlZeroMemory(CurrentPartition->VolumeLabel, sizeof(CurrentPartition->VolumeLabel));
    }

    CurrentPartition = NULL;

    UpdateMbrDiskLayout(CurrentDisk);
    Status = WriteMbrPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_DELETE_PARTITION_FAIL);
        return;
    }

    ConResPuts(StdOut, IDS_DELETE_PARTITION_SUCCESS);
}


static
VOID
DeleteGptPartition(
    _In_ BOOL bOverride)
{
    PPARTENTRY PrevPartEntry;
    PPARTENTRY NextPartEntry;
    NTSTATUS Status;

    /* Clear the system partition if it is being deleted */
#if 0
    if (List->SystemPartition == PartEntry)
    {
        ASSERT(List->SystemPartition);
        List->SystemPartition = NULL;
    }
#endif

    if ((bOverride == FALSE) &&
        ((memcmp(&CurrentPartition->Gpt.PartitionType, &PARTITION_SYSTEM_GUID, sizeof(GUID)) == 0) ||
         (memcmp(&CurrentPartition->Gpt.PartitionType, &PARTITION_MSFT_RESERVED_GUID, sizeof(GUID)) == 0)))
    {
        ConResPuts(StdOut, IDS_DELETE_PARTITION_FAIL);
        return;
    }

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    /* Dismount the partition */
    DismountVolume(CurrentPartition);

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

        ZeroMemory(&CurrentPartition->Gpt.PartitionType, sizeof(GUID));
        ZeroMemory(&CurrentPartition->Gpt.PartitionId, sizeof(GUID));
        CurrentPartition->Gpt.Attributes = 0ULL;

        CurrentPartition->FormatState = Unformatted;
        CurrentPartition->FileSystemName[0] = L'\0';
        CurrentPartition->DriveLetter = 0;
        RtlZeroMemory(CurrentPartition->VolumeLabel, sizeof(CurrentPartition->VolumeLabel));
    }

    CurrentPartition = NULL;

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    UpdateGptDiskLayout(CurrentDisk, TRUE);
#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(CurrentDisk);
#endif
    Status = WriteGptPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_DELETE_PARTITION_FAIL);
        return;
    }

    ConResPuts(StdOut, IDS_DELETE_PARTITION_SUCCESS);
}


EXIT_CODE
DeletePartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    INT i;
    BOOL bOverride = FALSE;

    DPRINT("DeletePartition()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return EXIT_SUCCESS;
    }

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
    }

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr - Already handled above */
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
            return EXIT_SUCCESS;
        }
    }

    if (CurrentPartition->IsBoot || CurrentPartition->IsSystem)
    {
        ConResPuts(StdOut, IDS_DELETE_PARTITION_SYSTEM);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        DeleteMbrPartition(bOverride);
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        DeleteGptPartition(bOverride);
    }

    return EXIT_SUCCESS;
}


EXIT_CODE
DeleteVolume(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    return EXIT_SUCCESS;
}
