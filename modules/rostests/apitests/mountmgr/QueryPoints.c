/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for QueryPoints IOCTL
 * PROGRAMMER:      Pierre Schweitzer
 */

#include "precomp.h"

VOID
TraceMountPoint(PMOUNTMGR_MOUNT_POINTS MountPoints,
                PMOUNTMGR_MOUNT_POINT MountPoint)
{
    trace("MountPoint: %p\n", MountPoint);
    trace("\tSymbolicOffset: %ld\n", MountPoint->SymbolicLinkNameOffset);
    trace("\tSymbolicLinkName: %.*S\n", MountPoint->SymbolicLinkNameLength / sizeof(WCHAR), (PWSTR)((ULONG_PTR)MountPoints + MountPoint->SymbolicLinkNameOffset));
    trace("\tDeviceOffset: %ld\n", MountPoint->DeviceNameOffset);
    trace("\tDeviceName: %.*S\n", MountPoint->DeviceNameLength / sizeof(WCHAR), (PWSTR)((ULONG_PTR)MountPoints + MountPoint->DeviceNameOffset));
}

START_TEST(QueryPoints)
{
    BOOL Ret;
    HANDLE MountMgrHandle;
    DWORD BytesReturned, Drives, i;
    struct {
        MOUNTMGR_MOUNT_POINT;
        WCHAR Buffer[sizeof(L"\\DosDevice\\A:")];
    } SinglePoint;
    MOUNTMGR_MOUNT_POINTS MountPoints;
    PMOUNTMGR_MOUNT_POINTS AllocatedPoints;

    MountMgrHandle = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME, 0,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 INVALID_HANDLE_VALUE);
    if (MountMgrHandle == INVALID_HANDLE_VALUE)
    {
        win_skip("MountMgr unavailable: %lx\n", GetLastError());
        return;
    }

    ZeroMemory(&SinglePoint, sizeof(SinglePoint));

    Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                          &SinglePoint, sizeof(MOUNTMGR_MOUNT_POINT),
                          &MountPoints, sizeof(MOUNTMGR_MOUNT_POINTS),
                          &BytesReturned, NULL);
    ok(Ret == FALSE, "IOCTL unexpectedly succeed\n");
    ok(GetLastError() == ERROR_MORE_DATA, "Unexcepted failure: %lx\n", GetLastError());

    AllocatedPoints = RtlAllocateHeap(RtlGetProcessHeap(), 0, MountPoints.Size);
    if (AllocatedPoints == NULL)
    {
        win_skip("Insufficiant memory\n");
    }
    else
    {
        AllocatedPoints->NumberOfMountPoints = 0;

        Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                              &SinglePoint, sizeof(MOUNTMGR_MOUNT_POINT),
                              AllocatedPoints, MountPoints.Size,
                              &BytesReturned, NULL);
        ok(Ret == TRUE, "IOCTL unexpectedly failed %lx\n", GetLastError());

        for (i = 0; i < AllocatedPoints->NumberOfMountPoints; ++i)
        {
            TraceMountPoint(AllocatedPoints, &AllocatedPoints->MountPoints[i]);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, AllocatedPoints);
    }

    Drives = GetLogicalDrives();
    if (Drives == 0)
    {
        win_skip("Drives map unavailable: %lx\n", GetLastError());
        goto Done;
    }

    for (i = 0; i < 26; i++)
    {
        if (!(Drives & (1 << i)))
        {
            break;
        }
    }

    if (i == 26)
    {
        win_skip("All the drive letters are in use, skipping\n");
        goto Done;
    }

    SinglePoint.SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    SinglePoint.SymbolicLinkNameLength = sizeof(L"\\DosDevice\\A:") - sizeof(UNICODE_NULL);
    StringCbPrintfW((PWSTR)((ULONG_PTR)&SinglePoint + sizeof(MOUNTMGR_MOUNT_POINT)),
                    sizeof(L"\\DosDevice\\A:"),
                    L"\\DosDevice\\%C:",
                    i + L'A');

    Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                          &SinglePoint, sizeof(SinglePoint),
                          &MountPoints, sizeof(MOUNTMGR_MOUNT_POINTS),
                          &BytesReturned, NULL);
    ok(Ret == FALSE, "IOCTL unexpectedly succeed\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Unexcepted failure: %lx\n", GetLastError());

Done:
    CloseHandle(MountMgrHandle);
}
