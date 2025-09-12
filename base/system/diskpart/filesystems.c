/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/filesystems.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

static
BOOL
ShowFileSystemInfo(
    PVOLENTRY VolumeEntry)
{
    WCHAR VolumeNameBuffer[MAX_PATH];
    UNICODE_STRING VolumeName;
    HANDLE VolumeHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG ulSize, ulClusterSize = 0;
    FILE_FS_FULL_SIZE_INFORMATION SizeInfo;
    FILE_FS_FULL_SIZE_INFORMATION FullSizeInfo;
    PFILE_FS_ATTRIBUTE_INFORMATION pAttributeInfo = NULL;
    NTSTATUS Status;
    BOOL Result = TRUE;

    wcscpy(VolumeNameBuffer, VolumeEntry->DeviceName);
    wcscat(VolumeNameBuffer, L"\\");

    RtlInitUnicodeString(&VolumeName, VolumeNameBuffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &VolumeName,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&VolumeHandle,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_NO_MEDIA_IN_DEVICE)
        {
            ConResPuts(StdOut, IDS_ERROR_NO_MEDIUM);
            return FALSE;
        }
        else if (Status == STATUS_UNRECOGNIZED_VOLUME)
        {
            ConResPuts(StdOut, IDS_FILESYSTEMS_CURRENT);
            ConPuts(StdOut, L"\n");
            ConResPrintf(StdOut, IDS_FILESYSTEMS_TYPE, L"RAW");
            ConResPrintf(StdOut, IDS_FILESYSTEMS_CLUSTERSIZE, 512);
        }

        return TRUE;
    }

    ulSize = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 255 * sizeof(WCHAR);
    pAttributeInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     ulSize);
    if (pAttributeInfo == NULL)
    {
        Result = FALSE;
        goto done;
    }

    Status = NtQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          pAttributeInfo,
                                          ulSize,
                                          FileFsAttributeInformation);
    if (!NT_SUCCESS(Status))
    {
        Result = FALSE;
        goto done;
    }

    Status = NtQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          &FullSizeInfo,
                                          sizeof(FILE_FS_FULL_SIZE_INFORMATION),
                                          FileFsFullSizeInformation);
    if (NT_SUCCESS(Status))
    {
        ulClusterSize  = FullSizeInfo.BytesPerSector * FullSizeInfo.SectorsPerAllocationUnit;
    }
    else
    {
        Status = NtQueryVolumeInformationFile(VolumeHandle,
                                              &IoStatusBlock,
                                              &SizeInfo,
                                              sizeof(FILE_FS_SIZE_INFORMATION),
                                              FileFsSizeInformation);
        if (NT_SUCCESS(Status))
        {
            ulClusterSize  = SizeInfo.BytesPerSector * SizeInfo.SectorsPerAllocationUnit;
        }
    }


    ConResPuts(StdOut, IDS_FILESYSTEMS_CURRENT);
    ConPuts(StdOut, L"\n");

    ConResPrintf(StdOut, IDS_FILESYSTEMS_TYPE, pAttributeInfo->FileSystemName);
    ConResPrintf(StdOut, IDS_FILESYSTEMS_CLUSTERSIZE, ulClusterSize);
    ConPuts(StdOut, L"\n");

done:
    if (pAttributeInfo)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pAttributeInfo);

    NtClose(VolumeHandle);

    return Result;
}

static
VOID
ShowInstalledFileSystems(VOID)
{
    WCHAR szBuffer[256];
    BOOLEAN ret;
    DWORD dwIndex;
    UCHAR uMajor, uMinor;
    BOOLEAN bLatest;

    ConResPuts(StdOut, IDS_FILESYSTEMS_FORMATTING);

    for (dwIndex = 0; ; dwIndex++)
    {
        ret = QueryAvailableFileSystemFormat(dwIndex,
                                             szBuffer,
                                             &uMajor,
                                             &uMinor,
                                            &bLatest);
        if (ret == FALSE)
            break;

        ConPrintf(StdOut, L"  %s\n", szBuffer);
    }

    ConPuts(StdOut, L"\n");
}

BOOL
filesystems_main(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    if (CurrentVolume == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_VOLUME);
        return TRUE;
    }

    ConPuts(StdOut, L"\n");

    if (ShowFileSystemInfo(CurrentVolume))
    {
        ShowInstalledFileSystems();
    }

    return TRUE;
}
