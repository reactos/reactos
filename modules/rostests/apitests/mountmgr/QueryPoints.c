/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for QueryPoints IOCTL
 * PROGRAMMER:      Pierre Schweitzer
 */

#include "precomp.h"

START_TEST(QueryPoints)
{
    BOOL Ret;
    DWORD BytesReturned;
    HANDLE MountMgrHandle;
    MOUNTMGR_MOUNT_POINT SinglePoint;
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

    ZeroMemory(&SinglePoint, sizeof(MOUNTMGR_MOUNT_POINT));

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
        goto Done;
    }

    Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                          &SinglePoint, sizeof(MOUNTMGR_MOUNT_POINT),
                          AllocatedPoints, MountPoints.Size,
                          &BytesReturned, NULL);
    ok(Ret == TRUE, "IOCTL unexpectedly failed %lx\n", GetLastError());

    RtlFreeHeap(RtlGetProcessHeap(), 0, AllocatedPoints);

Done:
    CloseHandle(MountMgrHandle);
}
