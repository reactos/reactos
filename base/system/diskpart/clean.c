/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/clean.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


EXIT_CODE
clean_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    PVOLENTRY VolumeEntry;
    BOOL bAll = FALSE;
    PUCHAR SectorsBuffer = NULL;
    ULONG LayoutBufferSize, Size;
    INT i;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle = NULL;
    LARGE_INTEGER Offset, Count, MaxCount;
    NTSTATUS Status;

    DPRINT("Clean()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    /* Do not allow to clean the boot disk */
    if ((CurrentDisk->BiosFound == TRUE) &&
        (CurrentDisk->BiosDiskNumber == 0))
    {
        ConResPuts(StdOut, IDS_CLEAN_SYSTEM);
        return EXIT_SUCCESS;
    }

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[1], L"all") == 0)
        {
            bAll = TRUE;
        }
    }

    /* Dismount and remove all logical partitions */
    while (!IsListEmpty(&CurrentDisk->LogicalPartListHead))
    {
        Entry = RemoveHeadList(&CurrentDisk->LogicalPartListHead);
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        /* Dismount the logical partition */
        if (PartEntry->Mbr.PartitionType != 0)
        {
            DismountVolume(PartEntry);
            VolumeEntry = GetVolumeFromPartition(PartEntry);
            if (VolumeEntry)
                RemoveVolume(VolumeEntry);
        }

        /* Delete it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, PartEntry);
    }

    /* Dismount and remove all primary partitions */
    while (!IsListEmpty(&CurrentDisk->PrimaryPartListHead))
    {
        Entry = RemoveHeadList(&CurrentDisk->PrimaryPartListHead);
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        /* Dismount the primary partition */
        if ((PartEntry->Mbr.PartitionType != 0) &&
            (IsContainerPartition(PartEntry->Mbr.PartitionType) == FALSE))
        {
            DismountVolume(PartEntry);
            VolumeEntry = GetVolumeFromPartition(PartEntry);
            if (VolumeEntry)
                RemoveVolume(VolumeEntry);
        }

        /* Delete it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, PartEntry);
    }

    /* Initialize the disk entry */
    CurrentDisk->ExtendedPartition = NULL;
    CurrentDisk->Dirty = FALSE;
    CurrentDisk->NewDisk = TRUE;
    CurrentDisk->PartitionStyle = PARTITION_STYLE_RAW;

    /* Wipe the layout buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentDisk->LayoutBuffer);

    LayoutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                       ((4 - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION_EX));
    CurrentDisk->LayoutBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                LayoutBufferSize);
    if (CurrentDisk->LayoutBuffer == NULL)
    {
        DPRINT1("Failed to allocate the disk layout buffer!\n");
        return EXIT_SUCCESS;
    }

    CurrentDisk->LayoutBuffer->PartitionStyle = PARTITION_STYLE_RAW;

    /* Allocate a 1MB sectors buffer */
    SectorsBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    1024 * 1024);
    if (SectorsBuffer == NULL)
    {
        DPRINT1("Failed to allocate the sectors buffer!\n");
        goto done;
    }

    /* Open the disk for writing */
    StringCchPrintfW(Buffer, ARRAYSIZE(Buffer),
                     L"\\Device\\Harddisk%d\\Partition0",
                     CurrentDisk->DiskNumber);

    RtlInitUnicodeString(&Name, Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the disk! (Status 0x%08lx)\n", Status);
        ConResPuts(StdOut, IDS_CLEAN_FAIL);
        goto done;
    }

    /* Clean sectors */
    if (bAll)
    {
        MaxCount.QuadPart = (CurrentDisk->SectorCount.QuadPart * CurrentDisk->BytesPerSector) / (1024 * 1024);
        for (Count.QuadPart = 0; Count.QuadPart < MaxCount.QuadPart; Count.QuadPart++)
        {
            Offset.QuadPart = Count.QuadPart * (1024 * 1024);
            Status = NtWriteFile(FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 SectorsBuffer,
                                 1024 * 1024,
                                 &Offset,
                                 NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to write MB! (Status 0x%08lx)\n", Status);
                ConResPuts(StdOut, IDS_CLEAN_FAIL);
                goto done;
            }
        }

        Size = (ULONG)(CurrentDisk->SectorCount.QuadPart * CurrentDisk->BytesPerSector) % (1024 * 1024);
        if (Size != 0)
        {
            Offset.QuadPart += (1024 * 1024);
            Status = NtWriteFile(FileHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 SectorsBuffer,
                                 Size,
                                 &Offset,
                                 NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to write the last part! (Status 0x%08lx)\n", Status);
                ConResPuts(StdOut, IDS_CLEAN_FAIL);
                goto done;
            }
        }
    }
    else
    {
        /* Clean the first MB */
        Offset.QuadPart = 0;
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             SectorsBuffer,
                             1024 * 1024,
                             &Offset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to write the first MB! (Status 0x%08lx)\n", Status);
            ConResPuts(StdOut, IDS_CLEAN_FAIL);
            goto done;
        }

        /* Clean the last MB */
        Offset.QuadPart = (CurrentDisk->SectorCount.QuadPart * CurrentDisk->BytesPerSector) - (1024 * 1024);
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             SectorsBuffer,
                             1024 * 1024,
                             &Offset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to write the last MB! (Status 0x%08lx)\n", Status);
            ConResPuts(StdOut, IDS_CLEAN_FAIL);
            goto done;
        }
    }

    ConResPuts(StdOut, IDS_CLEAN_SUCCESS);

done:
    if (FileHandle != NULL)
        NtClose(FileHandle);

    if (SectorsBuffer != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, SectorsBuffer);

    return EXIT_SUCCESS;
}
