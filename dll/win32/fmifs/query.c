/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/query.c
 * PURPOSE:         Query volume information
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"
#include <ntddstor.h>
#include <ntstrsafe.h>

#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>

BOOLEAN
NTAPI
QueryAvailableFileSystemFormat(
    IN DWORD Index,
    IN OUT PWCHAR FileSystem, /* FIXME: Probably one minimal size is mandatory, but which one? */
    OUT UCHAR *Major,
    OUT UCHAR *Minor,
    OUT BOOLEAN *LatestVersion)
{
    PLIST_ENTRY ListEntry;
    PIFS_PROVIDER Provider;

     if (!FileSystem || !Major ||!Minor ||!LatestVersion)
        return FALSE;

    ListEntry = ProviderListHead.Flink;
    while (TRUE)
    {
        if (ListEntry == &ProviderListHead)
            return FALSE;
        if (Index == 0)
            break;
        ListEntry = ListEntry->Flink;
        Index--;
    }

    Provider = CONTAINING_RECORD(ListEntry, IFS_PROVIDER, ListEntry);
    wcscpy(FileSystem, Provider->Name);
    *Major = 0; /* FIXME */
    *Minor = 0; /* FIXME */
    *LatestVersion = TRUE; /* FIXME */

    return TRUE;
}

/**
 * @brief
 * Retrieves disk device information.
 *
 * @param[in] DriveRoot
 * String which contains a DOS device name,
 *
 * @param[in,out] DeviceInformation
 * Pointer to buffer with DEVICE_INFORMATION structure which will receive data.
 *
 * @param[in] BufferSize
 * Size of buffer in bytes.
 *
 * @return
 * TRUE if the buffer was large enough and was filled with
 * the requested information, FALSE otherwise.
 *
 * @remarks
 * The returned information is mostly related to Sony Memory Stick devices.
 * On Vista+ the returned information is disk sector size and volume length in sectors,
 * regardless of the type of disk.
 * ReactOS implementation returns DEVICE_HOTPLUG flag if inspected device is a hotplug device
 * as well as sector size and volume length of disk device.
 */
BOOL
NTAPI
QueryDeviceInformation(
    _In_ PWCHAR DriveRoot,
    _Out_ PVOID DeviceInformation,
    _In_ ULONG BufferSize)
{
    PDEVICE_INFORMATION DeviceInfo = DeviceInformation;
    IO_STATUS_BLOCK Iosb;
    DISK_GEOMETRY DiskGeometry;
    STORAGE_HOTPLUG_INFO HotplugInfo;
    GET_LENGTH_INFORMATION LengthInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceName;
    HANDLE FileHandle;
    NTSTATUS Status;
    WCHAR DiskDevice[MAX_PATH];
    WCHAR DriveName[MAX_PATH];

    /* Buffer should be able to at least hold DeviceFlags */
    if (BufferSize < sizeof(ULONG) ||
        !NT_SUCCESS(RtlStringCchCopyW(DriveName, ARRAYSIZE(DriveName), DriveRoot)))
    {
        return FALSE;
    }

    if (DriveName[wcslen(DriveName) - 1] != L'\\')
    {
        /* Append the trailing backslash for GetVolumeNameForVolumeMountPointW */
        if (!NT_SUCCESS(RtlStringCchCatW(DriveName, ARRAYSIZE(DriveName), L"\\")))
            return FALSE;
    }

    if (!GetVolumeNameForVolumeMountPointW(DriveName, DiskDevice, ARRAYSIZE(DiskDevice)) ||
        !RtlDosPathNameToNtPathName_U(DiskDevice, &DeviceName, NULL, NULL))
    {
        /* Disk has no volume GUID, fallback to QueryDosDevice */
        DriveName[wcslen(DriveName) - 1] = UNICODE_NULL;
        if (!QueryDosDeviceW(DriveName, DiskDevice, ARRAYSIZE(DiskDevice)))
            return FALSE;
        RtlInitUnicodeString(&DeviceName, DiskDevice);
    }
    else
    {
        /* Trim the trailing backslash since we will work with a device object */
        DeviceName.Length -= sizeof(WCHAR);
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &Iosb,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
        return FALSE;

    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_STORAGE_GET_HOTPLUG_INFO,
                                   NULL,
                                   0,
                                   &HotplugInfo,
                                   sizeof(HotplugInfo));
    if (!NT_SUCCESS(Status))
        goto Quit;

    DeviceInfo->DeviceFlags = 0;
    if (HotplugInfo.MediaHotplug || HotplugInfo.DeviceHotplug)
    {
        /* This is a hotplug device */
        DeviceInfo->DeviceFlags |= DEVICE_HOTPLUG;
    }

    /* Other flags that would be set here are related to Sony "Memory Stick"
     * type of devices which we do not have any special support for */

    if (BufferSize >= sizeof(DEVICE_INFORMATION))
    {
        /* This is the Vista+ version of the structure.
         * We need to also provide disk sector size and volume length in sectors. */
        Status = NtDeviceIoControlFile(FileHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                       NULL,
                                       0,
                                       &DiskGeometry,
                                       sizeof(DiskGeometry));
        if (!NT_SUCCESS(Status))
            goto Quit;

        Status = NtDeviceIoControlFile(FileHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_DISK_GET_LENGTH_INFO,
                                       NULL,
                                       0,
                                       &LengthInformation,
                                       sizeof(LengthInformation));
        if (!NT_SUCCESS(Status))
            goto Quit;

        LengthInformation.Length.QuadPart /= DiskGeometry.BytesPerSector;
        DeviceInfo->SectorSize = DiskGeometry.BytesPerSector;
        DeviceInfo->SectorCount = LengthInformation.Length;
    }

    Status = STATUS_SUCCESS;

Quit:
    NtClose(FileHandle);
    return NT_SUCCESS(Status);
}
