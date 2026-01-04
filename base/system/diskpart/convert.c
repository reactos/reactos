/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/convert.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
CreateDisk(
    _In_ ULONG DiskNumber,
    _In_ PCREATE_DISK DiskInfo)
{
    WCHAR DstPath[MAX_PATH];
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE FileHandle = NULL;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    DPRINT("CreateDisk(%lu %p)\n", DiskNumber, DiskInfo);

    StringCchPrintfW(DstPath, ARRAYSIZE(DstPath),
                     L"\\Device\\Harddisk%lu\\Partition0",
                     CurrentDisk->DiskNumber);
    RtlInitUnicodeString(&Name, DstPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &Iosb,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        goto done;
    }

    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_CREATE_DISK,
                                   DiskInfo,
                                   sizeof(*DiskInfo),
                                   NULL,
                                   0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() failed (Status %lx)\n", Status);
        goto done;
    }

    /* Free the layout buffer */
    if (CurrentDisk->LayoutBuffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentDisk->LayoutBuffer);

    CurrentDisk->LayoutBuffer = NULL;
    CurrentDisk->ExtendedPartition = NULL;
    CurrentDisk->Dirty = FALSE;
    CurrentDisk->NewDisk = TRUE;
    CurrentDisk->PartitionStyle = DiskInfo->PartitionStyle;

    ReadLayoutBuffer(FileHandle, CurrentDisk);

done:
    if (FileHandle)
        NtClose(FileHandle);

    return Status;
}


EXIT_CODE
ConvertGPT(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    CREATE_DISK DiskInfo;
    NTSTATUS Status;

    DPRINT("ConvertGPT()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        ConResPuts(StdOut, IDS_CONVERT_GPT_ALREADY);
        return EXIT_SUCCESS;
    }

    if (GetPrimaryPartitionCount(CurrentDisk) != 0)
    {
        ConResPuts(StdOut, IDS_CONVERT_GPT_NOT_EMPTY);
        return EXIT_SUCCESS;
    }

#if 0
    /* Fail if disk size is less than 128MB */
    if (CurrentDisk->SectorCount.QuadPart * CurrentDisk->BytesPerSector < 128ULL * 1024ULL * 1024ULL)
    {
        ConResPuts(StdOut, IDS_CONVERT_GPT_TOO_SMALL);
        return EXIT_SUCCESS;
    }
#endif

    DiskInfo.PartitionStyle = PARTITION_STYLE_GPT;
    CreateGUID(&DiskInfo.Gpt.DiskId);
    DiskInfo.Gpt.MaxPartitionCount = 128;

    Status = CreateDisk(CurrentDisk->DiskNumber, &DiskInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateDisk() failed!\n");
        return EXIT_SUCCESS;
    }

    CurrentDisk->StartSector.QuadPart = AlignDown(CurrentDisk->LayoutBuffer->Gpt.StartingUsableOffset.QuadPart / CurrentDisk->BytesPerSector,
                                                  CurrentDisk->SectorAlignment) + (ULONGLONG)CurrentDisk->SectorAlignment;
    CurrentDisk->EndSector.QuadPart = AlignDown(CurrentDisk->StartSector.QuadPart + (CurrentDisk->LayoutBuffer->Gpt.UsableLength.QuadPart / CurrentDisk->BytesPerSector) - 1,
                                                CurrentDisk->SectorAlignment);

    ScanForUnpartitionedGptDiskSpace(CurrentDisk);
    ConResPuts(StdOut, IDS_CONVERT_GPT_SUCCESS);

    return EXIT_SUCCESS;
}


EXIT_CODE
ConvertMBR(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    CREATE_DISK DiskInfo;
    NTSTATUS Status;

    DPRINT("ConvertMBR()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if ((CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR) ||
        (CurrentDisk->PartitionStyle == PARTITION_STYLE_RAW))
    {
        ConResPuts(StdOut, IDS_CONVERT_MBR_ALREADY);
        return EXIT_SUCCESS;
    }

    if (GetPrimaryPartitionCount(CurrentDisk) != 0)
    {
        ConResPuts(StdOut, IDS_CONVERT_MBR_NOT_EMPTY);
        return EXIT_SUCCESS;
    }

    DiskInfo.PartitionStyle = PARTITION_STYLE_MBR;
    CreateSignature(&DiskInfo.Mbr.Signature);

    Status = CreateDisk(CurrentDisk->DiskNumber, &DiskInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateDisk() failed!\n");
        return EXIT_SUCCESS;
    }

    CurrentDisk->StartSector.QuadPart = (ULONGLONG)CurrentDisk->SectorAlignment;
    CurrentDisk->EndSector.QuadPart = min(CurrentDisk->SectorCount.QuadPart, 0x100000000) - 1;

    ScanForUnpartitionedMbrDiskSpace(CurrentDisk);
    ConResPuts(StdOut, IDS_CONVERT_MBR_SUCCESS);

    return EXIT_SUCCESS;
}
