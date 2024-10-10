/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/ntfslib/ntfslib.c
 * PURPOSE:     NTFS Lib
 * PROGRAMMERS: Pierre Schweitzer
 *              Daniel Victor
 */

#include "ntfslib.h"

static NTSTATUS NTAPI
FormatEx2(
    PUNICODE_STRING DriveRoot,
    FMIFS_MEDIA_FLAG MediaFlag,
    PUNICODE_STRING Label,
    BOOLEAN QuickFormat,
    ULONG ClusterSize,
    PFMIFSCALLBACK Callback)
{
    NTSTATUS Status;
    HANDLE h;
    LARGE_INTEGER offset;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    GET_LENGTH_INFORMATION gli;
    DISK_GEOMETRY dg;

    InitializeObjectAttributes(&attr, DriveRoot, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenFile(
        &h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli));
    if (!NT_SUCCESS(Status))
    {
        NtClose(h);
        return Status;
    }

    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(dg));
    if (!NT_SUCCESS(Status))
    {
        NtClose(h);
        return Status;
    }

    NTFS_BootSector bootsector;
    RtlZeroMemory(&bootsector, sizeof(bootsector));

    bootsector.jump[0] = 0xeb;
    bootsector.jump[1] = 0x52;
    bootsector.jump[2] = 0x90;
    RtlCopyMemory(&bootsector.oem_id, "NTFS    ", 8);
    bootsector.bytes_per_sector = dg.BytesPerSector;
    bootsector.sectors_per_cluster = 8;
    bootsector.reserved_sectors = 0;
    bootsector.media_descriptor = 0xf8;
    bootsector.sectors_per_track = 63;
    bootsector.number_of_heads = 255;
    bootsector.hidden_sectors = 0;
    bootsector.total_sectors = (gli.Length.QuadPart - sizeof(NTFS_BootSector)) / bootsector.bytes_per_sector;
    bootsector.mft_cluster = 4;
    bootsector.mft_mirror_cluster = 8;
    bootsector.clusters_per_file_record_segment = 1;
    bootsector.clusters_per_index_block = 1;
    bootsector.volume_serial_number = 0x123456789abcdef0;
    bootsector.signature = 0xaa55;

    offset.QuadPart = 0;
    Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, &bootsector, sizeof(bootsector), &offset, NULL);
    if (!NT_SUCCESS(Status))
    {
        NtClose(h);
        return Status;
    }

    NTFS_MFTEntry mft_entry;
    RtlZeroMemory(&mft_entry, sizeof(mft_entry));

    mft_entry.type = 0x454c4946;
    mft_entry.attrs_offset = sizeof(mft_entry);
    mft_entry.flags = 0x01;

    offset.QuadPart = bootsector.bytes_per_sector * bootsector.sectors_per_cluster * bootsector.mft_cluster;
    Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, &mft_entry, sizeof(mft_entry), &offset, NULL);
    if (!NT_SUCCESS(Status))
    {
        NtClose(h);
        return Status;
    }

    offset.QuadPart = bootsector.bytes_per_sector * bootsector.sectors_per_cluster * bootsector.mft_mirror_cluster;
    Status = NtWriteFile(h, NULL, NULL, NULL, &iosb, &mft_entry, sizeof(mft_entry), &offset, NULL);
    if (!NT_SUCCESS(Status))
    {
        NtClose(h);
        return Status;
    }

    NtClose(h);
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
NtfsFormat(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN QuickFormat,
    IN BOOLEAN BackwardCompatible,
    IN MEDIA_TYPE MediaType,
    IN PUNICODE_STRING Label,
    IN ULONG ClusterSize)
{
    NTSTATUS Status;

    Status = FormatEx2(DriveRoot, (FMIFS_MEDIA_FLAG)MediaType, Label, QuickFormat, ClusterSize, Callback);

    return NT_SUCCESS(Status);
}

BOOLEAN
NTAPI
NtfsChkdsk(
    IN PUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PVOID pUnknown1,
    IN PVOID pUnknown2,
    IN PVOID pUnknown3,
    IN PVOID pUnknown4,
    IN PULONG ExitStatus)
{
    UNIMPLEMENTED;
    *ExitStatus = (ULONG)STATUS_SUCCESS;
    return TRUE;
}
