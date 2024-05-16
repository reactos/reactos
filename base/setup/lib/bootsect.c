/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Installation helpers for BIOS-based PC boot sectors
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca@sfr.fr>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "partlist.h" // FIXME: Included for PARTITION_SECTOR
#include "bootcode.h"
#include "bootsect.h"

#define NDEBUG
#include <debug.h>


/* INSTALLATION HELPERS ******************************************************/

NTSTATUS
InstallMbrBootCode(
    _Inout_ PBOOTCODE BootCode, // MBR source bootsector
    _In_ HANDLE DstPath,        // Where to save the bootsector built from the source + disk information
    _In_ HANDLE DiskHandle)     // Disk holding the (old) MBR information
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    BOOTCODE OrgBootCode = {0};

C_ASSERT(sizeof(PARTITION_SECTOR) == SECTORSIZE);

    /* Check the source bootsector size (1 sector) */
    if (BootCode->Length != sizeof(PARTITION_SECTOR))
        return STATUS_INVALID_BUFFER_SIZE;

    /* Allocate and read the current original MBR bootsector */
    Status = ReadBootCodeByHandle(&OrgBootCode, DiskHandle, sizeof(PARTITION_SECTOR));
    if (!NT_SUCCESS(Status))
        return Status;

    /*
     * Copy the disk signature, the reserved fields and
     * the partition table from the old MBR to the new one.
     */
    RtlCopyMemory(&((PPARTITION_SECTOR)BootCode->BootCode)->Signature,
                  &((PPARTITION_SECTOR)OrgBootCode.BootCode)->Signature,
                  sizeof(PARTITION_SECTOR) -
                  FIELD_OFFSET(PARTITION_SECTOR, Signature)
                  /* Length of partition table */);

    /* Free the original bootsector */
    FreeBootCode(&OrgBootCode);

    /* Write the new bootsector to DstPath */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootCode->BootCode,
                         BootCode->Length,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("NtWriteFile() failed: Status %lx\n", Status);

    return Status;
}

NTSTATUS
InstallFatBootCode(
    _Inout_ PBOOTCODE BootCode, // FAT12/16 source bootsector
    _In_ HANDLE DstPath,        // Where to save the bootsector built from the source + partition information
    _In_ HANDLE RootPartition)  // Partition holding the (old) FAT12/16 information
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    BOOTCODE OrgBootCode = {0};

    /* Check the source bootsector size (1 sector) */
    if (BootCode->Length != FAT_BOOTSECTOR_SIZE)
        return STATUS_INVALID_BUFFER_SIZE;

    /* Allocate and read the current original partition bootsector */
    Status = ReadBootCodeByHandle(&OrgBootCode, RootPartition, FAT_BOOTSECTOR_SIZE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Adjust the bootsector (copy a part of the FAT12/16 BPB) */
    RtlCopyMemory(&((PFAT_BOOTSECTOR)BootCode->BootCode)->OemName,
                  &((PFAT_BOOTSECTOR)OrgBootCode.BootCode)->OemName,
                  FIELD_OFFSET(FAT_BOOTSECTOR, BootCodeAndData) -
                  FIELD_OFFSET(FAT_BOOTSECTOR, OemName));

    /* Free the original bootsector */
    FreeBootCode(&OrgBootCode);

    /* Write the new bootsector to DstPath */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootCode->BootCode,
                         BootCode->Length,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);

    return Status;
}

NTSTATUS
InstallFat32BootCode(
    _Inout_ PBOOTCODE BootCode, // FAT32 source bootsector
    _In_ HANDLE DstPath,        // Where to save the bootsector built from the source + partition information
    _In_ HANDLE RootPartition)  // Partition holding the (old) FAT32 information
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    USHORT BackupBootSector = 0;
    BOOTCODE OrgBootCode = {0};

    /* Check the source bootsector size (2 sectors) */
    if (BootCode->Length != 2 * FAT32_BOOTSECTOR_SIZE)
        return STATUS_INVALID_BUFFER_SIZE;

    /* Allocate and read the current original partition bootsector (only the first sector) */
    Status = ReadBootCodeByHandle(&OrgBootCode, RootPartition, FAT32_BOOTSECTOR_SIZE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Adjust the bootsector (copy a part of the FAT32 BPB) */
    RtlCopyMemory(&((PFAT32_BOOTSECTOR)BootCode->BootCode)->OemName,
                  &((PFAT32_BOOTSECTOR)OrgBootCode.BootCode)->OemName,
                  FIELD_OFFSET(FAT32_BOOTSECTOR, BootCodeAndData) -
                  FIELD_OFFSET(FAT32_BOOTSECTOR, OemName));

    /*
     * We know we copy the boot code to a file only when DstPath != RootPartition,
     * otherwise the boot code is copied to the specified root partition.
     */
    if (DstPath != RootPartition)
    {
        /* Copy to a file: Disable the backup bootsector */
        ((PFAT32_BOOTSECTOR)BootCode->BootCode)->BackupBootSector = 0;
    }
    else
    {
        /* Copy to a disk: Get the location of the backup bootsector */
        BackupBootSector = ((PFAT32_BOOTSECTOR)OrgBootCode.BootCode)->BackupBootSector;
    }

    /* Free the original bootsector */
    FreeBootCode(&OrgBootCode);

    /* Write the first sector of the new bootcode to DstPath sector 0 */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootCode->BootCode,
                         FAT32_BOOTSECTOR_SIZE,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
        return Status;
    }

    if (DstPath == RootPartition)
    {
        /* Copy to a disk: Write the backup bootsector */
        if ((BackupBootSector != 0x0000) && (BackupBootSector != 0xFFFF))
        {
            FileOffset.QuadPart = (ULONGLONG)((ULONG)BackupBootSector * FAT32_BOOTSECTOR_SIZE);
            Status = NtWriteFile(DstPath,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 BootCode->BootCode,
                                 FAT32_BOOTSECTOR_SIZE,
                                 &FileOffset,
                                 NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
                return Status;
            }
        }
    }

    /* Write the second sector of the new bootcode to boot disk sector 14 */
    // FileOffset.QuadPart = (ULONGLONG)(14 * FAT32_BOOTSECTOR_SIZE);
    FileOffset.QuadPart = 14 * FAT32_BOOTSECTOR_SIZE;
    Status = NtWriteFile(DstPath,   // or really RootPartition ???
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         ((PUCHAR)BootCode->BootCode + FAT32_BOOTSECTOR_SIZE),
                         FAT32_BOOTSECTOR_SIZE,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
    }

    return Status;
}

NTSTATUS
InstallBtrfsBootCode(
    _Inout_ PBOOTCODE BootCode, // BTRFS source bootsector
    _In_ HANDLE DstPath,        // Where to save the bootsector built from the source + partition information
    _In_ HANDLE RootPartition)  // Partition holding the (old) BTRFS information
{
    NTSTATUS Status;
    NTSTATUS LockStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    PARTITION_INFORMATION_EX PartInfo;

    /* Check the source bootsector size (3 sectors) */
    if (BootCode->Length != BTRFS_BOOTSECTOR_SIZE)
        return STATUS_INVALID_BUFFER_SIZE;

    /*
     * The BTRFS driver requires the volume to be locked in order to modify
     * the first sectors of the partition, even though they are outside the
     * file-system space / in the reserved area (they are situated before
     * the super-block at 0x1000) and is in principle allowed by the NT
     * storage stack.
     * So we lock here in order to write the bootsector at sector 0.
     * If locking fails, we ignore and continue nonetheless.
     */
    LockStatus = NtFsControlFile(DstPath,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_LOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("WARNING: Failed to lock BTRFS volume for writing bootsector! Operations may fail! (Status 0x%lx)\n", LockStatus);
    }

    /* Obtain partition info and write it to the bootsector */
    Status = NtDeviceIoControlFile(RootPartition,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_DISK_GET_PARTITION_INFO_EX,
                                   NULL,
                                   0,
                                   &PartInfo,
                                   sizeof(PartInfo));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IOCTL_DISK_GET_PARTITION_INFO_EX failed (Status %lx)\n", Status);
        goto Quit;
    }

    /* Write new bootsector to RootPath */
    ((PBTRFS_BOOTSECTOR)BootCode->BootCode)->PartitionStartLBA =
        PartInfo.StartingOffset.QuadPart / SECTORSIZE;

    /* Write sector 0 */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootCode->BootCode,
                         BootCode->Length,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
        goto Quit;
    }

Quit:
    /* Unlock the volume */
    LockStatus = NtFsControlFile(DstPath,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_UNLOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Failed to unlock BTRFS volume (Status 0x%lx)\n", LockStatus);
    }

    return Status;
}

NTSTATUS
InstallNtfsBootCode(
    _Inout_ PBOOTCODE BootCode, // NTFS source bootsector
    _In_ HANDLE DstPath,        // Where to save the bootsector built from the source + partition information
    _In_ HANDLE RootPartition)  // Partition holding the (old) NTFS information
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    BOOTCODE OrgBootCode = {0};

    /* Check the source bootsector size (16 sectors) */
    if (BootCode->Length != NTFS_BOOTSECTOR_SIZE)
        return STATUS_INVALID_BUFFER_SIZE;

    /* Allocate and read the current original partition bootsector */
    Status = ReadBootCodeByHandle(&OrgBootCode, RootPartition, NTFS_BOOTSECTOR_SIZE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Adjust the bootsector (copy a part of the NTFS BPB) */
    RtlCopyMemory(&((PNTFS_BOOTSECTOR)BootCode->BootCode)->OEMID,
                  &((PNTFS_BOOTSECTOR)OrgBootCode.BootCode)->OEMID,
                  FIELD_OFFSET(NTFS_BOOTSECTOR, BootStrap) - FIELD_OFFSET(NTFS_BOOTSECTOR, OEMID));

    /* Free the original bootsector */
    FreeBootCode(&OrgBootCode);

    /* Write sector 0 */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootCode->BootCode,
                         BootCode->Length,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);

    return Status;
}


/* FUNCTIONS *****************************************************************/

NTSTATUS
InstallBootCodeToDisk(
    _Inout_ PBOOTCODE BootCode,
    _In_ /*PCWSTR*/PCUNICODE_STRING RootPath,
    _In_ PFS_INSTALL_BOOTCODE InstallBootCode)
{
    NTSTATUS Status, LockStatus;
    // UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle;

    /*
     * Open the root partition from which the bootcode (MBR, VBR) parameters
     * will be obtained; this is also where we will write the updated bootcode.
     */
    // RtlInitUnicodeString(&Name, RootPath);
    // TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)RootPath, // &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&PartitionHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT /* | FILE_SEQUENTIAL_ONLY */);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Lock the volume */
    LockStatus = NtFsControlFile(PartitionHandle,
                                 NULL, NULL, NULL,
                                 &IoStatusBlock,
                                 FSCTL_LOCK_VOLUME,
                                 NULL, 0, NULL, 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Unable to lock the volume before installing boot code (Status 0x%08lx) Expect problems.\n", LockStatus);
    }

    /* Install the bootcode (MBR, VBR) */
    Status = InstallBootCode(BootCode, PartitionHandle, PartitionHandle);

    /* Dismount and unlock the volume */
    if (NT_SUCCESS(LockStatus))
    {
        LockStatus = NtFsControlFile(PartitionHandle,
                                     NULL, NULL, NULL,
                                     &IoStatusBlock,
                                     FSCTL_DISMOUNT_VOLUME,
                                     NULL, 0, NULL, 0);
        if (!NT_SUCCESS(LockStatus))
        {
            DPRINT1("Unable to dismount the volume after installing boot code (Status 0x%08lx) Expect problems.\n", LockStatus);
        }

        LockStatus = NtFsControlFile(PartitionHandle,
                                     NULL, NULL, NULL,
                                     &IoStatusBlock,
                                     FSCTL_UNLOCK_VOLUME,
                                     NULL, 0, NULL, 0);
        if (!NT_SUCCESS(LockStatus))
        {
            DPRINT1("Unable to unlock the volume after installing boot code (Status 0x%08lx) Expect problems.\n", LockStatus);
        }
    }

    /* Close the partition */
    NtClose(PartitionHandle);
    return Status;
}

NTSTATUS
InstallBootCodeToFile(
    _Inout_ PBOOTCODE BootCode,
    _In_ /*PCWSTR*/PCUNICODE_STRING DstPath,
    _In_ /*PCWSTR*/PCUNICODE_STRING RootPath,
    _In_ PFS_INSTALL_BOOTCODE InstallBootCode)
{
    NTSTATUS Status;
    // UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle, FileHandle;

    /*
     * Open the root partition from which the bootcode (MBR, VBR)
     * parameters will be obtained.
     *
     * FIXME? It might be possible that we need to also open it for writing
     * access in case we really need to still write the second portion of
     * the boot sector ????
     */
    // RtlInitUnicodeString(&Name, RootPath);
    // TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)RootPath, // &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&PartitionHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT /* | FILE_SEQUENTIAL_ONLY */);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open or create the file where the new bootsector will be saved */
    // RtlInitUnicodeString(&Name, DstPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING)DstPath, // &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_SUPERSEDE, // FILE_OVERWRITE_IF
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile() failed: Status %lx\n", Status);
        NtClose(PartitionHandle);
        return Status;
    }

    /* Install the bootcode (MBR, VBR) */
    Status = InstallBootCode(BootCode, FileHandle, PartitionHandle);

    /* Close the file and the partition */
    NtClose(FileHandle);
    NtClose(PartitionHandle);

    return Status;
}

/* EOF */
