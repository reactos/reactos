/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for DeviceIoControl
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <apitest.h>
#include <strsafe.h>
#include <winioctl.h>
#include <mountmgr.h>
#include <mountdev.h>

WCHAR Letter;
HANDLE Device;

static
VOID
GetDiskGeometry(VOID)
{
    UINT Ret;
    DISK_GEOMETRY DG;
    DWORD Size, Error;
    DISK_GEOMETRY_EX DGE;

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
}

static
VOID
QueryDeviceName(VOID)
{
    UINT Ret;
    BOOL IsValid;
    DWORD Size, Error;
    MOUNTDEV_NAME MDN, *AllocatedMDN;

    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &MDN, sizeof(MDN) - 1, &Size, NULL);
    ok(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok(Error == ERROR_INVALID_PARAMETER, "Expecting ERROR_INVALID_PARAMETER, got %ld\n", Error);
    ok(Size == 40 /* ?! */, "Invalid output size: %ld\n", Size);

    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &MDN, sizeof(MDN), &Size, NULL);
    ok(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok(Error == ERROR_MORE_DATA, "Expecting ERROR_MORE_DATA, got %ld\n", Error);
    ok(Size == sizeof(MOUNTDEV_NAME), "Invalid output size: %ld\n", Size);

    AllocatedMDN = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MOUNTDEV_NAME, Name) + MDN.NameLength + sizeof(UNICODE_NULL));
    if (AllocatedMDN == NULL)
    {
        skip("Memory allocation failure\n");
        return;
    }

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, AllocatedMDN, FIELD_OFFSET(MOUNTDEV_NAME, Name) + MDN.NameLength, &Size, NULL);
    ok(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok(Size == FIELD_OFFSET(MOUNTDEV_NAME, Name) + MDN.NameLength, "Invalid output size: %ld\n", Size);
    ok(AllocatedMDN->NameLength == MDN.NameLength, "Mismatching sizes: %d %d\n", AllocatedMDN->NameLength, MDN.NameLength);

    if (Ret != 0)
    {
        IsValid = FALSE;
        AllocatedMDN->Name[AllocatedMDN->NameLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

        if (wcsstr(AllocatedMDN->Name, L"\\Device\\HarddiskVolume") != NULL)
        {
            IsValid = TRUE;
        }
        else if (wcsstr(AllocatedMDN->Name, L"\\DosDevices\\") != NULL)
        {
            IsValid = (AllocatedMDN->Name[12] == Letter && AllocatedMDN->Name[13] == L':');
        }

        ok(IsValid, "Invalid name: %.*S", AllocatedMDN->NameLength, AllocatedMDN->Name);
    }
    else
    {
        skip("Failed to query device name\n");
    }

    HeapFree(GetProcessHeap(), 0, AllocatedMDN);
}

static
VOID
QueryUniqueId(VOID)
{
    UINT Ret;
    DWORD Size, Error;
    MOUNTDEV_UNIQUE_ID MUI, *AllocatedMUI;

    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, NULL, 0, &MUI, sizeof(MUI) - 1, &Size, NULL);
    ok(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok(Error == ERROR_INVALID_PARAMETER, "Expecting ERROR_INVALID_PARAMETER, got %ld\n", Error);
    ok(Size == 48 /* ?! */, "Invalid output size: %ld\n", Size);

    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, NULL, 0, &MUI, sizeof(MUI), &Size, NULL);
    ok(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok(Error == ERROR_MORE_DATA, "Expecting ERROR_MORE_DATA, got %ld\n", Error);
    ok(Size == sizeof(MOUNTDEV_UNIQUE_ID), "Invalid output size: %ld\n", Size);

    AllocatedMUI = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + MUI.UniqueIdLength + sizeof(UNICODE_NULL));
    if (AllocatedMUI == NULL)
    {
        skip("Memory allocation failure\n");
        return;
    }

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, NULL, 0, AllocatedMUI, FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + MUI.UniqueIdLength, &Size, NULL);
    ok(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok(Size == FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + MUI.UniqueIdLength, "Invalid output size: %ld\n", Size);
    ok(AllocatedMUI->UniqueIdLength == MUI.UniqueIdLength, "Mismatching sizes: %d %d\n", AllocatedMUI->UniqueIdLength, MUI.UniqueIdLength);

    HeapFree(GetProcessHeap(), 0, AllocatedMUI);
}

START_TEST(DeviceIoControl)
{
    UINT Ret;
    WCHAR Path[MAX_PATH];

    Path[0] = 'C';
    Path[1] = ':';
    Path[2] = '\\';
    Ret = GetSystemDirectoryW(Path, MAX_PATH);
    ok(Ret > 0, "GetSystemDirectory failed\n");

    Letter = towupper(Path[0]);
    ok(Path[1] == ':', "Not a drive letter: %c\n", Path[1]);
    ok(Path[2] == '\\', "Not a drive letter: %c\n", Path[2]);

    Ret = StringCchPrintfW(Path, MAX_PATH, L"\\\\?\\%c:", Letter);
    ok(Ret == S_OK, "StringCchPrintfW failed: %d\n", Ret);

    Device = CreateFileW(Path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (Device == INVALID_HANDLE_VALUE)
    {
        skip("CreateFileW for %S failed: %ld\n", Path, GetLastError());
        return;
    }

    GetDiskGeometry();
    QueryDeviceName();
    QueryUniqueId();

    CloseHandle(Device);
}
