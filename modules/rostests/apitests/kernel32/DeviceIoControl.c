/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for DeviceIoControl
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include "precomp.h"

#include <winioctl.h>
#include <mountdev.h>

WCHAR Letter;
HANDLE Device;
UINT DriveType;

#define ok_type(condition, format, ...) ok(condition, "(%d): " format, DriveType,  ##__VA_ARGS__)
#define skip_type(format, ...) skip("(%d): " format, DriveType, ##__VA_ARGS__)

static
BOOL
GetDiskGeometry(VOID)
{
    UINT Ret;
    DISK_GEOMETRY DG;
    DWORD Size, Error;
    DISK_GEOMETRY_EX DGE;

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DG, sizeof(DG) - 1, &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok_type(Error == ERROR_INSUFFICIENT_BUFFER, "Expecting ERROR_INSUFFICIENT_BUFFER, got %ld\n", Error);
    ok_type(Size == 0, "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &DG, sizeof(DG), &Size, NULL);
    /* Specific for CDROM, no disk present */
    if (Ret == 0 && GetLastError() == ERROR_NOT_READY)
    {
        skip_type("No CDROM present\n");
        return FALSE;
    }
    ok_type(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok_type(Size == sizeof(DG), "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &DGE, FIELD_OFFSET(DISK_GEOMETRY_EX, Data) - 1, &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok_type(Error == ERROR_INSUFFICIENT_BUFFER, "Expecting ERROR_INSUFFICIENT_BUFFER, got %ld\n", Error);
    ok_type(Size == 0, "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &DGE, FIELD_OFFSET(DISK_GEOMETRY_EX, Data), &Size, NULL);
    ok_type(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok_type(Size == FIELD_OFFSET(DISK_GEOMETRY_EX, Data), "Invalid output size: %ld\n", Size);

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &DGE, sizeof(DGE), &Size, NULL);
    ok_type(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    if (DriveType == DRIVE_FIXED)
    {
        ok_type(Size == sizeof(DGE), "Invalid output size: %ld\n", Size);
    }
    else
    {
        ok_type(Size == FIELD_OFFSET(DISK_GEOMETRY_EX, Data), "Invalid output size: %ld\n", Size);
    }

    return TRUE;
}

static
VOID
QueryDeviceName(VOID)
{
    UINT Ret;
    BOOL IsValid;
    DWORD Size, Error;
    MOUNTDEV_NAME MDN, *AllocatedMDN;

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &MDN, sizeof(MDN) - 1, &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    if (DriveType == DRIVE_FIXED)
    {
        ok_type(Error == ERROR_INVALID_PARAMETER, "Expecting ERROR_INVALID_PARAMETER, got %ld\n", Error);
    }
    else
    {
        ok_type(Error == ERROR_INSUFFICIENT_BUFFER, "Expecting ERROR_INSUFFICIENT_BUFFER, got %ld\n", Error);
    }
    ok_type(Size == 0, "Invalid output size: %ld\n", Size);

    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, &MDN, sizeof(MDN), &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok_type(Error == ERROR_MORE_DATA, "Expecting ERROR_MORE_DATA, got %ld\n", Error);
    ok_type(Size == sizeof(MOUNTDEV_NAME), "Invalid output size: %ld\n", Size);

    AllocatedMDN = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MOUNTDEV_NAME, Name) + MDN.NameLength + sizeof(UNICODE_NULL));
    if (AllocatedMDN == NULL)
    {
        skip_type("Memory allocation failure\n");
        return;
    }

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, NULL, 0, AllocatedMDN, FIELD_OFFSET(MOUNTDEV_NAME, Name) + MDN.NameLength, &Size, NULL);
    ok_type(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok_type(Size == FIELD_OFFSET(MOUNTDEV_NAME, Name) + MDN.NameLength, "Invalid output size: %ld\n", Size);
    ok_type(AllocatedMDN->NameLength == MDN.NameLength, "Mismatching sizes: %d %d\n", AllocatedMDN->NameLength, MDN.NameLength);

    if (Ret != 0)
    {
        IsValid = FALSE;
        AllocatedMDN->Name[AllocatedMDN->NameLength / sizeof(WCHAR) - 1] = UNICODE_NULL;

        if ((DriveType == DRIVE_FIXED && wcsstr(AllocatedMDN->Name, L"\\Device\\HarddiskVolume") != NULL) ||
            (DriveType == DRIVE_CDROM && wcsstr(AllocatedMDN->Name, L"\\Device\\CdRom") != NULL))
        {
            IsValid = TRUE;
        }
        else if (wcsstr(AllocatedMDN->Name, L"\\DosDevices\\") != NULL)
        {
            IsValid = (AllocatedMDN->Name[12] == Letter && AllocatedMDN->Name[13] == L':');
        }

        ok_type(IsValid, "Invalid name: %.*S\n", AllocatedMDN->NameLength, AllocatedMDN->Name);
    }
    else
    {
        skip_type("Failed to query device name\n");
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

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, NULL, 0, &MUI, sizeof(MUI) - 1, &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    if (DriveType == DRIVE_FIXED)
    {
        ok_type(Error == ERROR_INVALID_PARAMETER, "Expecting ERROR_INVALID_PARAMETER, got %ld\n", Error);
    }
    else
    {
        ok_type(Error == ERROR_INSUFFICIENT_BUFFER, "Expecting ERROR_INSUFFICIENT_BUFFER, got %ld\n", Error);
    }
    ok_type(Size == 0, "Invalid output size: %ld\n", Size);

    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, NULL, 0, &MUI, sizeof(MUI), &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    ok_type(Error == ERROR_MORE_DATA, "Expecting ERROR_MORE_DATA, got %ld\n", Error);
    ok_type(Size == sizeof(MOUNTDEV_UNIQUE_ID), "Invalid output size: %ld\n", Size);

    AllocatedMUI = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + MUI.UniqueIdLength + sizeof(UNICODE_NULL));
    if (AllocatedMUI == NULL)
    {
        skip_type("Memory allocation failure\n");
        return;
    }

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, NULL, 0, AllocatedMUI, FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + MUI.UniqueIdLength, &Size, NULL);
    ok_type(Ret != 0, "DeviceIoControl failed: %ld\n", GetLastError());
    ok_type(Size == FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + MUI.UniqueIdLength, "Invalid output size: %ld\n", Size);
    ok_type(AllocatedMUI->UniqueIdLength == MUI.UniqueIdLength, "Mismatching sizes: %d %d\n", AllocatedMUI->UniqueIdLength, MUI.UniqueIdLength);

    HeapFree(GetProcessHeap(), 0, AllocatedMUI);
}

static
VOID
QuerySuggestedLinkName(VOID)
{
    UINT Ret;
    DWORD Size, Error;
    MOUNTDEV_SUGGESTED_LINK_NAME MSLN;

    Size = 0;
    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME, NULL, 0, &MSLN, sizeof(MSLN) - 1, &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    if (DriveType == DRIVE_FIXED)
    {
        ok_type(Error == ERROR_INVALID_PARAMETER, "Expecting ERROR_INVALID_PARAMETER, got %ld\n", Error);
    }
    else
    {
        ok_type(Error == ERROR_INSUFFICIENT_BUFFER, "Expecting ERROR_INSUFFICIENT_BUFFER, got %ld\n", Error);
    }
    ok_type(Size == 0, "Invalid output size: %ld\n", Size);

    Ret = DeviceIoControl(Device, IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME, NULL, 0, &MSLN, sizeof(MSLN), &Size, NULL);
    ok_type(Ret == 0, "DeviceIoControl succeed\n");
    Error = GetLastError();
    if (DriveType == DRIVE_FIXED)
    {
        ok_type(Error == ERROR_NOT_FOUND, "Expecting ERROR_NOT_FOUND, got %ld\n", Error);
    }
    else
    {
        ok_type(Error == ERROR_FILE_NOT_FOUND, "Expecting ERROR_FILE_NOT_FOUND, got %ld\n", Error);
    }
    ok_type(Size == 0, "Invalid output size: %ld\n", Size);
}

START_TEST(DeviceIoControl)
{
    UINT Ret;
    WCHAR Path[MAX_PATH];
    DWORD DriveMap, Current;
    BOOL DiskDone, CdRomDone;

    DiskDone = FALSE;
    CdRomDone = FALSE;
    DriveMap = GetLogicalDrives();
    for (Current = 0; Current < 26; ++Current)
    {
        if (DriveMap & (1 << Current))
        {
            Ret = StringCchPrintfW(Path, MAX_PATH, L"%c:\\", Current + L'A');
            ok(Ret == S_OK, "StringCchPrintfW failed: %d\n", Ret);

            DriveType = GetDriveTypeW(Path);
            if ((DriveType == DRIVE_FIXED && !DiskDone) ||
                (DriveType == DRIVE_CDROM && !CdRomDone))
            {
                Ret = StringCchPrintfW(Path, MAX_PATH, L"\\\\?\\%c:", Current + L'A');
                ok(Ret == S_OK, "StringCchPrintfW failed: %d\n", Ret);

                Device = CreateFileW(Path, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (Device == INVALID_HANDLE_VALUE)
                {
                    skip_type("CreateFileW for %S failed: %ld\n", Path, GetLastError());
                    continue;
                }

                DiskDone = (DiskDone || (DriveType == DRIVE_FIXED));
                CdRomDone = (CdRomDone || (DriveType == DRIVE_CDROM));

                if (GetDiskGeometry())
                {
                    QueryDeviceName();
                    QueryUniqueId();
                    QuerySuggestedLinkName();
                }

                CloseHandle(Device);
            }

            if (CdRomDone && DiskDone)
            {
                break;
            }
        }
    }

    if (!DiskDone)
    {
        skip("No disk drive found\n");
    }

    if (!CdRomDone)
    {
        skip("No CDROM drive found\n");
    }
}
