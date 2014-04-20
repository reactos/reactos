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

START_TEST(NtQueryVolumeInformationFile)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION FileFsDevice;
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

    /*Now all NULL. Priority check: FsInformationClass value!*/
    SetLastError(0xcacacaca);
    status = NtQueryVolumeInformationFile(NULL, NULL, NULL, 0, 0);
    ok(status == STATUS_INVALID_INFO_CLASS, "Expected STATUS_INVALID_INFO_CLASS, got 0x%lx\n", status);
    ok(GetLastError() == 0xcacacaca, "Expected 0xcacacaca, got %lx\n", GetLastError());

    /*Almost all NULL. Then it checks against the Length!*/
    SetLastError(0xdeadbeef);
    status = NtQueryVolumeInformationFile(NULL, NULL, NULL, 0, FileFsDeviceInformation);
    ok(status == STATUS_INFO_LENGTH_MISMATCH, "Expected STATUS_INFO_LENGTH_MISMATCH, got 0x%lx\n", status);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %lx\n", GetLastError());

    NtClose(handle);
}
