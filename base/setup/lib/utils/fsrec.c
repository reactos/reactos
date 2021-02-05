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
static inline
NTSTATUS
GetFileSystemNameWorker(
    IN HANDLE PartitionHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    UCHAR Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    /* Retrieve the FS attributes */
    Status = NtQueryVolumeInformationFile(PartitionHandle,
                                          IoStatusBlock,
                                          FileFsAttribute,
                                          sizeof(Buffer),
                                          FileFsAttributeInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryVolumeInformationFile() failed, Status 0x%08lx\n", Status);
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
    IN PUNICODE_STRING PartitionPath OPTIONAL,
    IN HANDLE PartitionHandle OPTIONAL,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    if (PartitionPath && PartitionHandle)
        return STATUS_INVALID_PARAMETER;

    /* Open the partition if a path has been given;
     * otherwise just use the provided handle. */
    if (PartitionPath)
    {
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
            DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n",
                    PartitionPath, Status);
            return Status;
        }
    }

    /* Retrieve the FS attributes */
    Status = GetFileSystemNameWorker(PartitionHandle,
                                     &IoStatusBlock,
                                     FileSystemName,
                                     FileSystemNameSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFileSystemName() failed for partition '%wZ' (0x%p), Status 0x%08lx\n",
                PartitionPath, PartitionHandle, Status);
    }

    if (PartitionPath)
    {
        /* Close the partition */
        NtClose(PartitionHandle);
    }

    return Status;
}

NTSTATUS
GetFileSystemName(
    IN PCWSTR PartitionPath OPTIONAL,
    IN HANDLE PartitionHandle OPTIONAL,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    UNICODE_STRING PartitionPathU;

    if (PartitionPath && PartitionHandle)
        return STATUS_INVALID_PARAMETER;

    if (PartitionPath)
        RtlInitUnicodeString(&PartitionPathU, PartitionPath);

    return GetFileSystemName_UStr(PartitionPath ? &PartitionPathU : NULL,
                                  PartitionPath ? NULL : PartitionHandle,
                                  FileSystemName,
                                  FileSystemNameSize);
}

static inline
NTSTATUS
InferFileSystemWorker(
    IN HANDLE PartitionHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status, Status2;
    union
    {
        PARTITION_INFORMATION_EX InfoEx;
        PARTITION_INFORMATION Info;
    } PartInfo;
    UCHAR PartitionType;

    if (FileSystemNameSize < sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    *FileSystemName = L'\0';

    /* Try to infer a file system using NT file system recognition */
    Status = GetFileSystemName_UStr(NULL, PartitionHandle,
                                    FileSystemName,
                                    FileSystemNameSize);
    if (NT_SUCCESS(Status) && *FileSystemName)
        goto Quit;

    /*
     * Check whether the partition is MBR, and if so, retrieve its MBR
     * partition type and try to infer a preferred file system for it.
     */

    // NOTE: Use Status2 in order not to clobber the original Status.
    Status2 = NtDeviceIoControlFile(PartitionHandle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    IoStatusBlock,
                                    IOCTL_DISK_GET_PARTITION_INFO_EX,
                                    NULL,
                                    0,
                                    &PartInfo.InfoEx,
                                    sizeof(PartInfo.InfoEx));
    if (!NT_SUCCESS(Status2))
    {
        DPRINT1("IOCTL_DISK_GET_PARTITION_INFO_EX failed (Status %lx)\n", Status2);

        if (Status2 != STATUS_INVALID_DEVICE_REQUEST)
            goto Quit;

        /*
         * We could have failed because the partition is on a dynamic
         * MBR or GPT data disk, so retry with the non-EX IOCTL.
         */
        Status2 = NtDeviceIoControlFile(PartitionHandle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        IoStatusBlock,
                                        IOCTL_DISK_GET_PARTITION_INFO,
                                        NULL,
                                        0,
                                        &PartInfo.Info,
                                        sizeof(PartInfo.Info));
        if (!NT_SUCCESS(Status2))
        {
            /* We failed again, bail out */
            DPRINT1("IOCTL_DISK_GET_PARTITION_INFO failed (Status %lx)\n", Status2);
            goto Quit;
        }

        /* The partition is supposed to be on an MBR disk; retrieve its type */
        PartitionType = PartInfo.Info.PartitionType;
    }
    else
    {
        /* We succeeded; retrieve the partition type only if it is on an MBR disk */
        if (PartInfo.InfoEx.PartitionStyle != PARTITION_STYLE_MBR)
        {
            /* Disk is not MBR, bail out */
            goto Quit;
        }
        PartitionType = PartInfo.InfoEx.Mbr.PartitionType;
    }

    /*
     * Given an MBR partition type, try to infer a preferred file system.
     *
     * WARNING: This is partly a hack, since partitions with the same type
     * can be formatted with different file systems: for example, usual Linux
     * partitions that are formatted in EXT2/3/4, ReiserFS, etc... have the
     * same partition type 0x83.
     *
     * The proper fix is to make a function that detects the existing FS
     * from a given partition (not based on the partition type).
     * On the contrary, for unformatted partitions with a given type, the
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
    if (*FileSystemName && wcsicmp(FileSystemName, L"NTFS") == 0)
    {
        // WARNING: We cannot write on this FS yet!
        DPRINT1("Recognized file system '%S' that doesn't have write support yet!\n",
                FileSystemName);
    }

    return Status;
}

NTSTATUS
InferFileSystem(
    IN PCWSTR PartitionPath OPTIONAL,
    IN HANDLE PartitionHandle OPTIONAL,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    UNICODE_STRING PartitionPathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    if (PartitionPath && PartitionHandle)
        return STATUS_INVALID_PARAMETER;

    /* Open the partition if a path has been given;
     * otherwise just use the provided handle. */
    if (PartitionPath)
    {
        RtlInitUnicodeString(&PartitionPathU, PartitionPath);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &PartitionPathU,
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
            DPRINT1("Failed to open partition '%S', Status 0x%08lx\n",
                    PartitionPath, Status);
            return Status;
        }
    }

    /* Retrieve the FS */
    Status = InferFileSystemWorker(PartitionHandle,
                                   &IoStatusBlock,
                                   FileSystemName,
                                   FileSystemNameSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InferFileSystem() failed for partition '%S' (0x%p), Status 0x%08lx\n",
                PartitionPath, PartitionHandle, Status);
    }
    else
    {
        DPRINT1("InferFileSystem(): FileSystem (guessed): %S\n",
                *FileSystemName ? FileSystemName : L"None");
    }

    if (PartitionPath)
    {
        /* Close the partition */
        NtClose(PartitionHandle);
    }

    return Status;
}

UCHAR
FileSystemToMBRPartitionType(
    IN PCWSTR FileSystem,
    IN ULONGLONG StartSector,
    IN ULONGLONG SectorCount)
{
    ASSERT(FileSystem);

    if (SectorCount == 0)
        return PARTITION_ENTRY_UNUSED;

    if (wcsicmp(FileSystem, L"FAT")   == 0 ||
        wcsicmp(FileSystem, L"FAT32") == 0 ||
        wcsicmp(FileSystem, L"RAW")   == 0)
    {
        if (SectorCount < 8192ULL)
        {
            /* FAT12 CHS partition (disk is smaller than 4.1MB) */
            return PARTITION_FAT_12;
        }
        else if (StartSector < 1450560ULL)
        {
            /* Partition starts below the 8.4GB boundary ==> CHS partition */

            if (SectorCount < 65536ULL)
            {
                /* FAT16 CHS partition (partition size < 32MB) */
                return PARTITION_FAT_16;
            }
            else if (SectorCount < 1048576ULL)
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

            if (SectorCount < 1048576ULL)
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
