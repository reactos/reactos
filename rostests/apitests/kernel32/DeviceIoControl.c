/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for DeviceIoControl
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <apitest.h>
#include <strsafe.h>
#include <winioctl.h>

START_TEST(DeviceIoControl)
{
    UINT Ret;
    WCHAR Letter;
    HANDLE Device;
    DISK_GEOMETRY DG;
    DWORD Size, Error;
    WCHAR Path[MAX_PATH];
    DISK_GEOMETRY_EX DGE;

    Path[0] = 'C';
    Path[1] = ':';
    Path[2] = '\\';
    Ret = GetSystemDirectoryW(Path, MAX_PATH);
    ok(Ret > 0, "GetSystemDirectory failed\n");

    Letter = Path[0];
    ok(Path[1] == ':', "Not a drive letter: %c\n", Path[1]);
    ok(Path[2] == '\\', "Not a drive letter: %c\n", Path[2]);

    Ret = StringCchPrintfW(Path, MAX_PATH, L"%\\\\?\\c:", Letter);
    ok(Ret == S_OK, "StringCchPrintfW failed: %d\n", Ret);

    Device = CreateFileW(Path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(Device != INVALID_HANDLE_VALUE, "CreateFileW for %S failed: %ld\n", Path, GetLastError());

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DG, sizeof(DG) - 1, &Size, NULL);
    ok(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok(Error == ERROR_INSUFFICIENT_BUFFER, "Expecting ERROR_INSUFFICIENT_BUFFER, got %ld\n", Error);
    ok(Size == 0, "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DG, sizeof(DG), &Size, NULL);
    ok(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok(Size == sizeof(DG), "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &DGE, FIELD_OFFSET(DISK_GEOMETRY_EX, Data) - 1, &Size, NULL);
    ok(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok(Error == ERROR_INSUFFICIENT_BUFFER, "Expecting ERROR_INSUFFICIENT_BUFFER, got %ld\n", Error);
    ok(Size == 0, "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &DGE, FIELD_OFFSET(DISK_GEOMETRY_EX, Data), &Size, NULL);
    ok(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok(Size == FIELD_OFFSET(DISK_GEOMETRY_EX, Data), "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &DGE, sizeof(DGE), &Size, NULL);
    ok(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok(Size == sizeof(DGE), "Invalid output size: %ld\n", Size);

    CloseHandle(Device);
}
