/*
 * PROJECT:         ReactOS API Tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         NtQueryVolumeInformationFile tests
 * PROGRAMMER:      Víctor Martínez Calvo <vicmarcal@gmail.com>
 */

#define WIN32_NO_STATUS
#include <stdio.h>
#include <wine/test.h>
#include <ndk/ntndk.h>

static
VOID
TestFileFsDeviceInformation(HANDLE handle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION FileFsDevice;
    NTSTATUS status;

    /*Testing VALID handle, with NULL IN parameters*/
    SetLastError(0xdeadb33f);
    status = NtQueryVolumeInformationFile(handle, NULL, &FileFsDevice, sizeof(FILE_FS_DEVICE_INFORMATION), FileFsDeviceInformation);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got 0x%lx\n", status);
    ok(GetLastError() == 0xdeadb33f, "Expected 0xdeadb33f, got %lx\n", GetLastError());

    SetLastError(0xcacacaca);
    status = NtQueryVolumeInformationFile(handle,  &IoStatusBlock, NULL, sizeof(FILE_FS_DEVICE_INFORMATION), FileFsDeviceInformation);
    ok(status == STATUS_ACCESS_VIOLATION, "Expected STATUS_ACCESS_VIOLATION, got 0x%lx\n", status);
    ok(GetLastError() == 0xcacacaca, "Expected 0xcacacaca, got %lx\n", GetLastError());

    SetLastError(0xdadadada);
    status = NtQueryVolumeInformationFile(handle,  &IoStatusBlock, &FileFsDevice, 0, FileFsDeviceInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got 0x%lx\n", status);
    ok(GetLastError() == 0xdadadada, "Expected 0xdadadada, got %lx\n", GetLastError());

    /*All valid, invalid FsInformationClass value.*/
    SetLastError(0xdeadbeef);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, &FileFsDevice, sizeof(FILE_FS_DEVICE_INFORMATION), 0);
    ok(status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got 0x%lx\n", status);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %lx\n", GetLastError());

    /*Testing NULL handle*/
    SetLastError(0xdeadbeef);
    status = NtQueryVolumeInformationFile(NULL, &IoStatusBlock, &FileFsDevice, sizeof(FILE_FS_DEVICE_INFORMATION), FileFsDeviceInformation);
    ok(status == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got 0x%lx\n", status);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %lx\n", GetLastError());

    /*Testing INVALID_HANDLE_VALUE*/
    SetLastError(0xdeaddead);
    status = NtQueryVolumeInformationFile((HANDLE)(-1), &IoStatusBlock, &FileFsDevice, sizeof(FILE_FS_DEVICE_INFORMATION), FileFsDeviceInformation);
    ok(status == STATUS_OBJECT_TYPE_MISMATCH, "Expected STATUS_OBJECT_TYPE_MISMATCH, got 0x%lx\n", status);
    ok(GetLastError() == 0xdeaddead, "Expected 0xdeaddead, got %lx\n", GetLastError());

    /*Almost all NULL. Then it checks against the Length!*/
    SetLastError(0xdeadbeef);
    status = NtQueryVolumeInformationFile(NULL, NULL, NULL, 0, FileFsDeviceInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got 0x%lx\n", status);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %lx\n", GetLastError());
}

static
VOID
TestFileFsVolumeInformation(HANDLE handle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG Buffer[(sizeof(FILE_FS_VOLUME_INFORMATION) + MAX_PATH * sizeof(WCHAR)) / sizeof(ULONG)];
    PFILE_FS_VOLUME_INFORMATION VolumeInfo = (PFILE_FS_VOLUME_INFORMATION)Buffer;
    NTSTATUS status;

    status = NtQueryVolumeInformationFile(handle, NULL, NULL, 0, FileFsVolumeInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Got status 0x%lx\n", status);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, NULL, 0, FileFsVolumeInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Got status 0x%lx\n", status);
    ok(IoStatusBlock.Status == 0x55555555, "IoStatusBlock.Status = 0x%lx\n", IoStatusBlock.Status);
    ok(IoStatusBlock.Information == (ULONG_PTR)0x5555555555555555, "IoStatusBlock.Information = %Iu\n", IoStatusBlock.Information);

    status = NtQueryVolumeInformationFile(handle, NULL, VolumeInfo, 0, FileFsVolumeInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Got status 0x%lx\n", status);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, (PUCHAR)Buffer + 2, sizeof(FILE_FS_VOLUME_INFORMATION), FileFsVolumeInformation);
    ok(status == STATUS_DATATYPE_MISALIGNMENT, "Got status 0x%lx\n", status);
    ok(IoStatusBlock.Status == 0x55555555, "IoStatusBlock.Status = 0x%lx\n", IoStatusBlock.Status);
    ok(IoStatusBlock.Information == (ULONG_PTR)0x5555555555555555, "IoStatusBlock.Information = %Iu\n", IoStatusBlock.Information);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, VolumeInfo, 0, FileFsVolumeInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Got status 0x%lx\n", status);
    ok(IoStatusBlock.Status == 0x55555555, "IoStatusBlock.Status = 0x%lx\n", IoStatusBlock.Status);
    ok(IoStatusBlock.Information == (ULONG_PTR)0x5555555555555555, "IoStatusBlock.Information = %Iu\n", IoStatusBlock.Information);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, VolumeInfo, FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel), FileFsVolumeInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Got status 0x%lx\n", status);
    ok(IoStatusBlock.Status == 0x55555555, "IoStatusBlock.Status = 0x%lx\n", IoStatusBlock.Status);
    ok(IoStatusBlock.Information == (ULONG_PTR)0x5555555555555555, "IoStatusBlock.Information = %Iu\n", IoStatusBlock.Information);

    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, VolumeInfo, sizeof(FILE_FS_VOLUME_INFORMATION) - 1, FileFsVolumeInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Got status 0x%lx\n", status);
    ok(IoStatusBlock.Status == 0x55555555, "IoStatusBlock.Status = 0x%lx\n", IoStatusBlock.Status);
    ok(IoStatusBlock.Information == (ULONG_PTR)0x5555555555555555, "IoStatusBlock.Information = %Iu\n", IoStatusBlock.Information);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, VolumeInfo, sizeof(FILE_FS_VOLUME_INFORMATION), FileFsVolumeInformation);
    ok(status == STATUS_SUCCESS || status == STATUS_BUFFER_OVERFLOW, "Got status 0x%lx\n", status);
    ok(IoStatusBlock.Status == status, "IoStatusBlock.Status = 0x%lx, expected 0x%lx\n", IoStatusBlock.Status, status);
    if (status == STATUS_SUCCESS)
    {
        ok(VolumeInfo->VolumeLabelLength <= sizeof(FILE_FS_VOLUME_INFORMATION) - FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel),
           "VolumeInfo->VolumeLabelLength = %Iu\n", VolumeInfo->VolumeLabelLength);
        ok(IoStatusBlock.Information >= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel),
           "IoStatusBlock.Information = %Iu, expected >=%lu\n", IoStatusBlock.Information, (ULONG)FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel));
        ok(IoStatusBlock.Information <= sizeof(FILE_FS_VOLUME_INFORMATION),
           "IoStatusBlock.Information = %Iu, expected <=%lu\n", IoStatusBlock.Information, (ULONG)sizeof(FILE_FS_VOLUME_INFORMATION));
    }
    else
    {
        ok(VolumeInfo->VolumeLabelLength > sizeof(FILE_FS_VOLUME_INFORMATION) - FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel),
           "VolumeInfo->VolumeLabelLength = %Iu\n", VolumeInfo->VolumeLabelLength);
        ok(IoStatusBlock.Information == sizeof(FILE_FS_VOLUME_INFORMATION),
           "IoStatusBlock.Information = %Iu, expected %lu\n", IoStatusBlock.Information, (ULONG)sizeof(FILE_FS_VOLUME_INFORMATION));
    }
    ok(VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength / sizeof(WCHAR)] == 0x5555,
       "Got %x\n", VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength / sizeof(WCHAR)]);

    RtlFillMemory(Buffer, sizeof(Buffer), 0x55);
    RtlFillMemory(&IoStatusBlock, sizeof(IoStatusBlock), 0x55);
    status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, VolumeInfo, sizeof(Buffer), FileFsVolumeInformation);
    ok(status == STATUS_SUCCESS, "Got status 0x%lx\n", status);
    ok(IoStatusBlock.Status == STATUS_SUCCESS, "IoStatusBlock.Status = 0x%lx\n", IoStatusBlock.Status);
    ok(IoStatusBlock.Information >= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel),
           "IoStatusBlock.Information = %Iu, expected >=%lu\n", IoStatusBlock.Information, (ULONG)sizeof(FILE_FS_VOLUME_INFORMATION));
    ok(VolumeInfo->VolumeCreationTime.QuadPart != 0x5555555555555555, "VolumeInfo->VolumeCreationTime = %I64d\n", VolumeInfo->VolumeCreationTime.QuadPart);
    ok(VolumeInfo->VolumeSerialNumber != 0x55555555, "VolumeInfo->VolumeSerialNumber = %u\n", VolumeInfo->VolumeSerialNumber);
    ok(VolumeInfo->SupportsObjects == FALSE || VolumeInfo->SupportsObjects == TRUE, "VolumeInfo->SupportsObjects = %u\n", VolumeInfo->SupportsObjects);
    ok(VolumeInfo->VolumeLabelLength % sizeof(WCHAR) == 0, "VolumeInfo->VolumeLabelLength = %Iu\n", VolumeInfo->VolumeLabelLength);
    if (VolumeInfo->VolumeLabelLength >= sizeof(WCHAR))
        ok(VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength / sizeof(WCHAR) - 1] != 0x5555, "Incorrect VolumeLabel or Length\n");
    ok(VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength / sizeof(WCHAR)] == 0x5555,
       "Got %x\n", VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength / sizeof(WCHAR)]);
}

START_TEST(NtQueryVolumeInformationFile)
{
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES attr;
    HANDLE handle;
    WCHAR path[MAX_PATH];
    UNICODE_STRING pathW;
    NTSTATUS status;

    /*Store a valid Handle*/
    GetWindowsDirectoryW(path, MAX_PATH);
    RtlDosPathNameToNtPathName_U(path, &pathW, NULL, NULL);

    InitializeObjectAttributes(&attr, &pathW, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtOpenFile(&handle, SYNCHRONIZE|FILE_LIST_DIRECTORY, &attr, &IoStatusBlock, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

    RtlFreeUnicodeString(&pathW);

    if (!NT_SUCCESS(status))
    {
        skip("NtOpenFile failed: 0x%lx\n", status);
        return;
    }

    /*Now all NULL. Priority check: FsInformationClass value!*/
    SetLastError(0xcacacaca);
    status = NtQueryVolumeInformationFile(NULL, NULL, NULL, 0, 0);
    ok(status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got 0x%lx\n", status);
    ok(GetLastError() == 0xcacacaca, "Expected 0xcacacaca, got %lx\n", GetLastError());

    TestFileFsDeviceInformation(handle);
    TestFileFsVolumeInformation(handle);

    NtClose(handle);
}
