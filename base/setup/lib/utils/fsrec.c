/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem Recognition support functions,
 *              using NT OS functionality.
 * COPYRIGHT:   Copyright 2017-2020 Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "fsrec.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

/* NOTE: Ripped & adapted from base/system/autochk/autochk.c */
NTSTATUS
GetFileSystemNameByHandle(
    IN HANDLE PartitionHandle,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    UCHAR Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    /* Retrieve the FS attributes */
    Status = NtQueryVolumeInformationFile(PartitionHandle,
                                          &IoStatusBlock,
                                          FileFsAttribute,
                                          sizeof(Buffer),
                                          FileFsAttributeInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryVolumeInformationFile failed, Status 0x%08lx\n", Status);
        return Status;
    }

    if (FileSystemNameSize < FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    return RtlStringCbCopyNW(FileSystemName, FileSystemNameSize,
                             FileFsAttribute->FileSystemName,
                             FileFsAttribute->FileSystemNameLength);
}

NTSTATUS
GetFileSystemName_UStr(
    IN PUNICODE_STRING PartitionPath,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE PartitionHandle;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Open the partition */
    InitializeObjectAttributes(&ObjectAttributes,
                               PartitionPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionHandle,
                        FILE_GENERIC_READ /* | SYNCHRONIZE */,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0 /* FILE_SYNCHRONOUS_IO_NONALERT */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n", PartitionPath, Status);
        return Status;
    }

    /* Retrieve the FS attributes */
    Status = GetFileSystemNameByHandle(PartitionHandle, FileSystemName, FileSystemNameSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFileSystemNameByHandle() failed for partition '%wZ', Status 0x%08lx\n",
                PartitionPath, Status);
    }

    /* Close the partition */
    NtClose(PartitionHandle);

    return Status;
}

NTSTATUS
GetFileSystemName(
    IN PCWSTR Partition,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    UNICODE_STRING PartitionPath;

    RtlInitUnicodeString(&PartitionPath, Partition);
    return GetFileSystemName_UStr(&PartitionPath,
                                  FileSystemName,
                                  FileSystemNameSize);
}

NTSTATUS
InferFileSystemByHandle(
    IN HANDLE PartitionHandle,
    IN UCHAR PartitionType,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;

    if (FileSystemNameSize < sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    *FileSystemName = L'\0';

    /* Try to infer a file system using NT file system recognition */
    Status = GetFileSystemNameByHandle(PartitionHandle,
                                       FileSystemName,
                                       FileSystemNameSize);
    if (NT_SUCCESS(Status) && *FileSystemName)
    {
        goto Quit;
    }

    /*
     * Try to infer a preferred file system for this partition, given its ID.
     *
     * WARNING: This is partly a hack, since partitions with the same ID can
     * be formatted with different file systems: for example, usual Linux
     * partitions that are formatted in EXT2/3/4, ReiserFS, etc... have the
     * same partition ID 0x83.
     *
     * The proper fix is to make a function that detects the existing FS
     * from a given partition (not based on the partition ID).
     * On the contrary, for unformatted partitions with a given ID, the
     * following code is OK.
     */
    if ((PartitionType == PARTITION_FAT_12) ||
        (PartitionType == PARTITION_FAT_16) ||
        (PartitionType == PARTITION_HUGE  ) ||
        (PartitionType == PARTITION_XINT13))
    {
        /* FAT12 or FAT16 */
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"FAT");
    }
    else if ((PartitionType == PARTITION_FAT32) ||
             (PartitionType == PARTITION_FAT32_XINT13))
    {
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"FAT32");
    }
    else if (PartitionType == PARTITION_LINUX)
    {
        // WARNING: See the warning above.
        /* Could also be EXT2/3/4, ReiserFS, ... */
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"BTRFS");
    }
    else if (PartitionType == PARTITION_IFS)
    {
        // WARNING: See the warning above.
        /* Could also be HPFS */
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"NTFS");
    }

Quit:
    if (*FileSystemName)
    {
        // WARNING: We cannot write on this FS yet!
        if (PartitionType == PARTITION_IFS)
        {
            DPRINT1("Recognized file system '%S' that doesn't have write support yet!\n",
                    FileSystemName);
        }
    }

    DPRINT1("InferFileSystem -- PartitionType: 0x%02X ; FileSystem (guessed): %S\n",
            PartitionType, *FileSystemName ? FileSystemName : L"None");

    return Status;
}

NTSTATUS
InferFileSystem(
    IN PCWSTR Partition,
    IN UCHAR PartitionType,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    UNICODE_STRING PartitionPath;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE PartitionHandle;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Open the partition */
    RtlInitUnicodeString(&PartitionPath, Partition);
    InitializeObjectAttributes(&ObjectAttributes,
                               &PartitionPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionHandle,
                        FILE_GENERIC_READ /* | SYNCHRONIZE */,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0 /* FILE_SYNCHRONOUS_IO_NONALERT */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n", &PartitionPath, Status);
        return Status;
    }

    /* Retrieve the FS */
    Status = InferFileSystemByHandle(PartitionHandle,
                                     PartitionType,
                                     FileSystemName,
                                     FileSystemNameSize);

    /* Close the partition */
    NtClose(PartitionHandle);

    return Status;
}

UCHAR
FileSystemToPartitionType(
    IN PCWSTR FileSystem,
    IN PULARGE_INTEGER StartSector,
    IN PULARGE_INTEGER SectorCount)
{
    ASSERT(FileSystem && StartSector && SectorCount);

    if (wcsicmp(FileSystem, L"FAT")   == 0 ||
        wcsicmp(FileSystem, L"FAT32") == 0 ||
        wcsicmp(FileSystem, L"RAW")   == 0)
    {
        if (SectorCount->QuadPart < 8192)
        {
            /* FAT12 CHS partition (disk is smaller than 4.1MB) */
            return PARTITION_FAT_12;
        }
        else if (StartSector->QuadPart < 1450560)
        {
            /* Partition starts below the 8.4GB boundary ==> CHS partition */

            if (SectorCount->QuadPart < 65536)
            {
                /* FAT16 CHS partition (partition size < 32MB) */
                return PARTITION_FAT_16;
            }
            else if (SectorCount->QuadPart < 1048576)
            {
                /* FAT16 CHS partition (partition size < 512MB) */
                return PARTITION_HUGE;
            }
            else
            {
                /* FAT32 CHS partition (partition size >= 512MB) */
                return PARTITION_FAT32;
            }
        }
        else
        {
            /* Partition starts above the 8.4GB boundary ==> LBA partition */

            if (SectorCount->QuadPart < 1048576)
            {
                /* FAT16 LBA partition (partition size < 512MB) */
                return PARTITION_XINT13;
            }
            else
            {
                /* FAT32 LBA partition (partition size >= 512MB) */
                return PARTITION_FAT32_XINT13;
            }
        }
    }
    else if (wcsicmp(FileSystem, L"NTFS") == 0)
    {
        return PARTITION_IFS;
    }
    else if (wcsicmp(FileSystem, L"BTRFS") == 0 ||
             wcsicmp(FileSystem, L"EXT2")  == 0 ||
             wcsicmp(FileSystem, L"EXT3")  == 0 ||
             wcsicmp(FileSystem, L"EXT4")  == 0 ||
             wcsicmp(FileSystem, L"FFS")   == 0 ||
             wcsicmp(FileSystem, L"REISERFS") == 0)
    {
        return PARTITION_LINUX;
    }
    else
    {
        /* Unknown file system */
        DPRINT1("Unknown file system '%S'\n", FileSystem);
        return PARTITION_ENTRY_UNUSED;
    }
}

/* EOF */
