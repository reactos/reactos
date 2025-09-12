/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of GetFinalPathNameByHandleW
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "k32_vista.h"
#include <winnls.h>
#include <mountmgr.h>

#define NDEBUG
#include <debug.h>

// Note: The wine implementation is broken and cannot be used.

static
NTSTATUS
QueryDosVolumeNameForNtDeviceName(
    _In_ PCUNICODE_STRING DeviceName,
    _Inout_ PUNICODE_STRING DosVolumeName)
{
    static const UNICODE_STRING MupDevice = RTL_CONSTANT_STRING(L"\\Device\\Mup");
    static const UNICODE_STRING LanManDevice = RTL_CONSTANT_STRING(L"\\Device\\LanmanRedirector");
    static const UNICODE_STRING MountMgrDevice = RTL_CONSTANT_STRING(L"\\Device\\MountPointManager");
    static OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&MountMgrDevice, OBJ_CASE_INSENSITIVE);
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE MountMgrHandle;
    WCHAR Buffer[MAX_PATH];
    PMOUNTMGR_TARGET_NAME TargetName = (PMOUNTMGR_TARGET_NAME)Buffer;
    PMOUNTMGR_VOLUME_PATHS VolumePaths = (PMOUNTMGR_VOLUME_PATHS)Buffer;
    NTSTATUS Status;

    /* Validate parameters */
    if ((DeviceName == NULL) || (DeviceName->Buffer == NULL) || (DeviceName->Length == 0) ||
        (DosVolumeName == NULL) || (DosVolumeName->Buffer == NULL) || (DosVolumeName->MaximumLength == 0))
    {
        ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    /* Reset the output name */
    DosVolumeName->Length = 0;

    /* Check for network shares */
    if (RtlEqualUnicodeString(DeviceName, &MupDevice, TRUE) ||
        RtlEqualUnicodeString(DeviceName, &LanManDevice, TRUE))
    {
        /* For Mup or LanmanRedirector we use the UNC path format */
        return RtlAppendUnicodeToString(DosVolumeName, L"\\\\?\\UNC\\");
    }

    /* Open a handle to the mount manager */
    Status = NtCreateFile(&MountMgrHandle,
                          STANDARD_RIGHTS_READ,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile failed: %lx\n", Status);
        return Status;
    }

    /* Use the MountMgr to query the volume name */
    TargetName->DeviceNameLength = DeviceName->Length;
    RtlCopyMemory(TargetName->DeviceName, DeviceName->Buffer, DeviceName->Length);
    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                   Buffer,
                                   sizeof(Buffer),
                                   Buffer,
                                   sizeof(Buffer));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile failed: %lx\n", Status);
        goto Exit;
    }

    if ((IoStatusBlock.Information < sizeof(*VolumePaths)) ||
        (IoStatusBlock.Information < (VolumePaths->MultiSzLength + sizeof(ULONG))))
    {
        DPRINT1("Invalid information returned: %lu\n", IoStatusBlock.Information);
        ASSERT(FALSE);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    /* Construct the full volume path name from the first returned DOS volume */
    if (!NT_SUCCESS(RtlAppendUnicodeToString(DosVolumeName, L"\\\\?\\")) ||
        !NT_SUCCESS(RtlAppendUnicodeToString(DosVolumeName, VolumePaths->MultiSz) ||
        !NT_SUCCESS(RtlAppendUnicodeToString(DosVolumeName, L"\\"))))
    {
        DPRINT1("RtlAppendUnicodeToString failed: %lx\n", Status);
        goto Exit;
    }

    /* This is based on Windows behavior */
    SetLastError(ERROR_SUCCESS);

Exit:
    NtClose(MountMgrHandle);

    return Status;
}

DWORD
WINAPI
GetFinalPathNameByHandleW(
    _In_ HANDLE hFile,
    _Out_writes_(cchFilePath) LPWSTR lpszFilePath,
    _In_ DWORD cchFilePath,
    _In_ DWORD dwFlags)
{
    struct
    {
        union
        {
            OBJECT_NAME_INFORMATION Object;
            FILE_NAME_INFORMATION File;
        };
        WCHAR String[MAX_PATH];
    } NameInfoBuffer;
    PFILE_NAME_INFORMATION FileNameInfo = &NameInfoBuffer.File;
    PUNICODE_STRING ObjectName = &NameInfoBuffer.Object.Name;
    WCHAR VolumeBuffer[sizeof("\\Device\\Volume{01234567-89ab-cdef-0123-456789abcdef}")];
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR PathSplit;
    UNICODE_STRING DeviceName, RelativePath, VolumeName;
    SIZE_T VolumeLength;
    ULONG VolumeType;
    NTSTATUS Status;
    ULONG FinalLength;
    BOOL Success;

    /* Extract the requested volume type */
    VolumeType = dwFlags & (VOLUME_NAME_GUID | VOLUME_NAME_NONE | VOLUME_NAME_NT);

    /* Check if more than one volume type flag is set */
    if (VolumeType & (VolumeType- 1))
    {
        DPRINT1("Invalid flags passed: %lx\n", dwFlags);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Query the file name. This returns the relative path without volume.
       This also validates that the handle refers to a file. */
    Status = NtQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &NameInfoBuffer,
                                    sizeof(NameInfoBuffer),
                                    FileNameInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile failed: %lx\n", Status);
        BaseSetLastNTError(Status);
        return 0;
    }

    /* If no volume was requested, return the relative path */
    if (VolumeType == VOLUME_NAME_NONE)
    {
        FinalLength = FileNameInfo->FileNameLength / sizeof(WCHAR);
        if (cchFilePath < FinalLength + 1)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FinalLength + 1;
        }

        /* Copy the name to the caller's buffer */
        RtlCopyMemory(lpszFilePath, FileNameInfo->FileName, FileNameInfo->FileNameLength);
        lpszFilePath[FinalLength] = UNICODE_NULL;

        /* Return the length of the name */
        return FinalLength;
    }

    /* Query the NT object name */
    Status = NtQueryObject(hFile,
                           ObjectNameInformation,
                           &NameInfoBuffer,
                           sizeof(NameInfoBuffer) - sizeof(WCHAR),
                           NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryObject failed: %lx\n", Status);
        BaseSetLastNTError(Status);
        return 0;
    }

    /* If the NT path was requested, we return the name as it is */
    if (VolumeType == VOLUME_NAME_NT)
    {
        /* Calculate the final length and validate the buffer size */
        FinalLength = ObjectName->Length / sizeof(WCHAR);
        if (cchFilePath < FinalLength + 1)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FinalLength + 1;
        }

        /* Copy the name to the caller's buffer */
        RtlCopyMemory(lpszFilePath, ObjectName->Buffer, ObjectName->Length);
        lpszFilePath[ObjectName->Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Return the length of the name */
        return FinalLength;
    }

    /* For everything else we need to split the path */
    PathSplit = wcschr(&ObjectName->Buffer[sizeof("\\Device\\") - 1], L'\\');
    if (PathSplit == NULL)
    {
        DPRINT1("Invalid object name: %wZ\n", ObjectName);
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    DeviceName.Buffer = ObjectName->Buffer;
    DeviceName.Length = (USHORT)(PathSplit - ObjectName->Buffer) * sizeof(WCHAR);
    DeviceName.MaximumLength = DeviceName.Length;

    RelativePath.Buffer = PathSplit + 1;
    RelativePath.Length = ObjectName->Length - DeviceName.Length - sizeof(WCHAR);
    RelativePath.MaximumLength = RelativePath.Length;

    /* Query the DOS volume name */
    RtlInitEmptyUnicodeString(&VolumeName, VolumeBuffer, sizeof(VolumeBuffer));
    Status = QueryDosVolumeNameForNtDeviceName(&DeviceName, &VolumeName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("QueryDosVolumeNameForNtDeviceName failed: %lx\n", Status);
        BaseSetLastNTError(Status);
        return 0;
    }

    if (VolumeType == VOLUME_NAME_DOS)
    {
        /* Calculate the final length and validate the buffer size */
        FinalLength = ((VolumeName.Length + RelativePath.Length) / sizeof(WCHAR));
        if (cchFilePath < FinalLength + 1)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FinalLength + 1;
        }

        /* Construct the final name */
        RtlCopyMemory(lpszFilePath, VolumeName.Buffer, VolumeName.Length);
        RtlCopyMemory(lpszFilePath + (VolumeName.Length / sizeof(WCHAR)),
                      RelativePath.Buffer,
                      RelativePath.Length);
        lpszFilePath[FinalLength] = UNICODE_NULL;

        /* Return the length of the name */
        return FinalLength;
    }
    else if (VolumeType == VOLUME_NAME_GUID)
    {
        /* Query the GUID volume name */
        Success = GetVolumeNameForVolumeMountPointW(VolumeBuffer,
                                                    VolumeBuffer,
                                                    ARRAYSIZE(VolumeBuffer));
        if (!Success)
        {
            DPRINT1("GetVolumeNameForVolumeMountPointW failed: %d\n", GetLastError());
            SetLastError(ERROR_PATH_NOT_FOUND);
            return 0;
        }

        /* Calculate the final length and validate the buffer size */
        VolumeLength = wcslen(VolumeBuffer);
        FinalLength = VolumeLength + (RelativePath.Length / sizeof(WCHAR));
        if (cchFilePath < FinalLength + 1)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FinalLength + 1;
        }

        /* Construct the final name */
        RtlCopyMemory(lpszFilePath, VolumeBuffer, VolumeLength * sizeof(WCHAR));
        RtlCopyMemory(lpszFilePath + VolumeLength,
                      RelativePath.Buffer,
                      RelativePath.Length);
        lpszFilePath[FinalLength] = UNICODE_NULL;

        /* This is based on Windows behavior */
        SetLastError(ERROR_SUCCESS);

        return FinalLength;
    }

    DPRINT1("Invalid flags passed: %lx\n", dwFlags);
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
}
