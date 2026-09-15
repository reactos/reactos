/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test disk geometry reported for logical MBR partition volumes
 * COPYRIGHT:   Copyright 2026 Alejandro Sánchez <alesangreat@gmail.com>
 */

#include "precomp.h"

#include <ntddstor.h>

#define FILE_SHARE_ALL \
    (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

#define INITIAL_DRIVE_LAYOUT_BUFFER_SIZE \
    (sizeof(DRIVE_LAYOUT_INFORMATION_EX) + \
     15 * sizeof(PARTITION_INFORMATION_EX))

static
NTSTATUS
OpenDevice(
    _In_ PCWSTR DeviceName,
    _Out_ PHANDLE DeviceHandle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name, DeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    return NtOpenFile(DeviceHandle,
                      FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      FILE_SHARE_ALL,
                      FILE_SYNCHRONOUS_IO_NONALERT);
}

static
NTSTATUS
QueryDiskGeometry(
    _In_ HANDLE DeviceHandle,
    _Out_ PDISK_GEOMETRY Geometry,
    _Out_ PDISK_GEOMETRY_EX GeometryEx)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    RtlZeroMemory(Geometry, sizeof(*Geometry));
    Status = NtDeviceIoControlFile(DeviceHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL, 0,
                                   Geometry, sizeof(*Geometry));
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return Status;

    ok(IoStatusBlock.Information == sizeof(*Geometry),
       "IOCTL_DISK_GET_DRIVE_GEOMETRY returned %lu bytes, expected %lu\n",
       IoStatusBlock.Information, (ULONG)sizeof(*Geometry));
    ok(Geometry->MediaType != Unknown,
       "IOCTL_DISK_GET_DRIVE_GEOMETRY returned unknown media type\n");
    ok(Geometry->BytesPerSector != 0,
       "IOCTL_DISK_GET_DRIVE_GEOMETRY returned zero BytesPerSector\n");

    RtlZeroMemory(GeometryEx, sizeof(*GeometryEx));
    Status = NtDeviceIoControlFile(DeviceHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                   NULL, 0,
                                   GeometryEx,
                                   FIELD_OFFSET(DISK_GEOMETRY_EX, Data));
    ok_ntstatus(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return Status;

    ok(IoStatusBlock.Information == FIELD_OFFSET(DISK_GEOMETRY_EX, Data),
       "IOCTL_DISK_GET_DRIVE_GEOMETRY_EX returned %lu bytes, expected %lu\n",
       IoStatusBlock.Information,
       (ULONG)FIELD_OFFSET(DISK_GEOMETRY_EX, Data));
    ok(GeometryEx->Geometry.MediaType != Unknown,
       "IOCTL_DISK_GET_DRIVE_GEOMETRY_EX returned unknown media type\n");
    ok(GeometryEx->Geometry.BytesPerSector != 0,
       "IOCTL_DISK_GET_DRIVE_GEOMETRY_EX returned zero BytesPerSector\n");
    ok(GeometryEx->Geometry.MediaType == Geometry->MediaType,
       "Media type differs: basic %u, extended %u\n",
       Geometry->MediaType, GeometryEx->Geometry.MediaType);
    ok(GeometryEx->Geometry.BytesPerSector == Geometry->BytesPerSector,
       "BytesPerSector differs: basic %lu, extended %lu\n",
       Geometry->BytesPerSector, GeometryEx->Geometry.BytesPerSector);

    return Status;
}

static
PDRIVE_LAYOUT_INFORMATION_EX
QueryDriveLayout(
    _In_ HANDLE PhysicalDrive)
{
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG BufferSize = INITIAL_DRIVE_LAYOUT_BUFFER_SIZE;
    ULONG Attempt;
    NTSTATUS Status;

    for (Attempt = 0; Attempt < 8; ++Attempt)
    {
        DriveLayout = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
        if (!DriveLayout)
            return NULL;

        Status = NtDeviceIoControlFile(PhysicalDrive,
                                       NULL, NULL, NULL,
                                       &IoStatusBlock,
                                       IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                       NULL, 0,
                                       DriveLayout, BufferSize);
        if (NT_SUCCESS(Status))
            return DriveLayout;

        HeapFree(GetProcessHeap(), 0, DriveLayout);
        if (Status != STATUS_BUFFER_TOO_SMALL &&
            Status != STATUS_BUFFER_OVERFLOW &&
            Status != STATUS_INFO_LENGTH_MISMATCH)
        {
            return NULL;
        }

        if (BufferSize > MAXULONG / 2)
            return NULL;
        BufferSize *= 2;
    }

    return NULL;
}

static
BOOLEAN
IsLogicalMbrPartition(
    _In_ HANDLE PhysicalDrive,
    _In_ const PARTITION_INFORMATION_EX *PartitionInfo)
{
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayout;
    ULONGLONG PartitionStart, PartitionEnd;
    ULONG i;
    BOOLEAN IsLogical = FALSE;

    if (PartitionInfo->PartitionStyle != PARTITION_STYLE_MBR ||
        IsContainerPartition(PartitionInfo->Mbr.PartitionType) ||
        PartitionInfo->StartingOffset.QuadPart < 0 ||
        PartitionInfo->PartitionLength.QuadPart <= 0)
    {
        return FALSE;
    }

    DriveLayout = QueryDriveLayout(PhysicalDrive);
    if (!DriveLayout)
        return FALSE;

    if (DriveLayout->PartitionStyle != PARTITION_STYLE_MBR)
        goto Quit;

    PartitionStart = PartitionInfo->StartingOffset.QuadPart;
    PartitionEnd = PartitionStart + PartitionInfo->PartitionLength.QuadPart;
    if (PartitionEnd < PartitionStart)
        goto Quit;

    for (i = 0; i < DriveLayout->PartitionCount; ++i)
    {
        const PARTITION_INFORMATION_EX *Entry = &DriveLayout->PartitionEntry[i];
        ULONGLONG ContainerStart, ContainerEnd;

        if (Entry->PartitionStyle != PARTITION_STYLE_MBR ||
            !IsContainerPartition(Entry->Mbr.PartitionType) ||
            Entry->StartingOffset.QuadPart < 0 ||
            Entry->PartitionLength.QuadPart <= 0)
        {
            continue;
        }

        ContainerStart = Entry->StartingOffset.QuadPart;
        ContainerEnd = ContainerStart + Entry->PartitionLength.QuadPart;
        if (ContainerEnd < ContainerStart)
            continue;

        if (PartitionStart >= ContainerStart && PartitionEnd <= ContainerEnd)
        {
            IsLogical = TRUE;
            break;
        }
    }

Quit:
    HeapFree(GetProcessHeap(), 0, DriveLayout);
    return IsLogical;
}

static
BOOLEAN
TestVolumeGeometry(
    _In_ WCHAR Drive)
{
    WCHAR VolumeName[] = L"\\DosDevices\\?:";
    WCHAR PhysicalName[MAX_PATH];
    STORAGE_DEVICE_NUMBER DeviceNumber;
    PARTITION_INFORMATION_EX PartitionInfo;
    DISK_GEOMETRY Geometry, PhysicalGeometry;
    DISK_GEOMETRY_EX GeometryEx;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE VolumeHandle = NULL, PhysicalHandle = NULL;
    NTSTATUS Status;
    BOOLEAN IsLogical = FALSE;

    VolumeName[sizeof("\\DosDevices\\") - 1] = Drive;
    Status = OpenDevice(VolumeName, &VolumeHandle);
    if (!NT_SUCCESS(Status))
        goto Quit;

    RtlZeroMemory(&DeviceNumber, sizeof(DeviceNumber));
    Status = NtDeviceIoControlFile(VolumeHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                   NULL, 0,
                                   &DeviceNumber, sizeof(DeviceNumber));
    if (!NT_SUCCESS(Status) ||
        (DeviceNumber.DeviceType != FILE_DEVICE_DISK &&
         DeviceNumber.DeviceType != FILE_DEVICE_VIRTUAL_DISK) ||
        DeviceNumber.PartitionNumber == 0 ||
        DeviceNumber.PartitionNumber == ULONG_MAX)
    {
        goto Quit;
    }

    RtlZeroMemory(&PartitionInfo, sizeof(PartitionInfo));
    Status = NtDeviceIoControlFile(VolumeHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_DISK_GET_PARTITION_INFO_EX,
                                   NULL, 0,
                                   &PartitionInfo, sizeof(PartitionInfo));
    if (!NT_SUCCESS(Status))
        goto Quit;

    RtlStringCchPrintfW(PhysicalName, _countof(PhysicalName),
                        L"\\??\\PhysicalDrive%lu", DeviceNumber.DeviceNumber);
    Status = OpenDevice(PhysicalName, &PhysicalHandle);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /*
     * CORE-10402 is specifically about logical MBR partitions inside an
     * extended partition. Do not make geometry assertions for unrelated
     * fixed volumes while searching for that target.
     */
    IsLogical = IsLogicalMbrPartition(PhysicalHandle, &PartitionInfo);
    if (!IsLogical)
        goto Quit;

    Status = QueryDiskGeometry(VolumeHandle, &Geometry, &GeometryEx);
    if (!NT_SUCCESS(Status))
        goto Report;

    /* Physical-disk geometry is useful context, but it is not the bug oracle. */
    RtlZeroMemory(&PhysicalGeometry, sizeof(PhysicalGeometry));
    Status = NtDeviceIoControlFile(PhysicalHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL, 0,
                                   &PhysicalGeometry, sizeof(PhysicalGeometry));
    if (NT_SUCCESS(Status) &&
        IoStatusBlock.Information == sizeof(PhysicalGeometry) &&
        PhysicalGeometry.BytesPerSector != 0 &&
        PhysicalGeometry.MediaType != Unknown)
    {
        ok(Geometry.BytesPerSector == PhysicalGeometry.BytesPerSector,
           "Drive %c: partition BPS %lu differs from physical BPS %lu\n",
           Drive, Geometry.BytesPerSector, PhysicalGeometry.BytesPerSector);
        ok(Geometry.MediaType == PhysicalGeometry.MediaType,
           "Drive %c: partition media %u differs from physical media %u\n",
           Drive, Geometry.MediaType, PhysicalGeometry.MediaType);
    }

Report:
    trace("[CORE-10402] drive %c: disk %lu partition %lu, "
          "offset %I64u length %I64u, media %u, BPS %lu\n",
          Drive, DeviceNumber.DeviceNumber, DeviceNumber.PartitionNumber,
          PartitionInfo.StartingOffset.QuadPart,
          PartitionInfo.PartitionLength.QuadPart,
          Geometry.MediaType, Geometry.BytesPerSector);

Quit:
    if (PhysicalHandle)
        NtClose(PhysicalHandle);
    if (VolumeHandle)
        NtClose(VolumeHandle);

    return IsLogical;
}

START_TEST(DiskGeometry)
{
    DWORD Drives;
    ULONG i;
    BOOLEAN LogicalPartitionFound = FALSE;

    Drives = GetLogicalDrives();
    if (!Drives)
    {
        skip("Drives map unavailable, error 0x%lx\n", GetLastError());
        return;
    }

    for (i = 0; i < 26; ++i)
    {
        WCHAR DriveName[] = L"?:\\";

        if (!(Drives & (1u << i)))
            continue;

        DriveName[0] = L'A' + i;
        if (GetDriveTypeW(DriveName) != DRIVE_FIXED)
            continue;

        if (TestVolumeGeometry(DriveName[0]))
            LogicalPartitionFound = TRUE;
    }

    if (!LogicalPartitionFound)
    {
        skip("No mounted logical partition inside an extended MBR partition found\n");
    }
}
