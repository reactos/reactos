/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for IOCTL_MOUNTMGR_QUERY_POINTS
 * COPYRIGHT:   Copyright 2019 Pierre Schweitzer <pierre@reactos.org>
 */

#include "precomp.h"

VOID
TraceMountPoint(
    _In_ const MOUNTMGR_MOUNT_POINTS* MountPoints,
    _In_ const MOUNTMGR_MOUNT_POINT* MountPoint)
{
    trace("MountPoint: %p\n", MountPoint);
    trace("\tSymbolicOffset: %ld\n", MountPoint->SymbolicLinkNameOffset);
    trace("\tSymbolicLinkName: %.*S\n",
          MountPoint->SymbolicLinkNameLength / sizeof(WCHAR),
          (PWCHAR)((ULONG_PTR)MountPoints + MountPoint->SymbolicLinkNameOffset));
    trace("\tDeviceOffset: %ld\n", MountPoint->DeviceNameOffset);
    trace("\tDeviceName: %.*S\n",
          MountPoint->DeviceNameLength / sizeof(WCHAR),
          (PWCHAR)((ULONG_PTR)MountPoints + MountPoint->DeviceNameOffset));
}

START_TEST(QueryPoints)
{
    BOOL Ret;
    HANDLE MountMgrHandle;
    DWORD LastError, BytesReturned, Drives, i;
    struct
    {
        MOUNTMGR_MOUNT_POINT;
        WCHAR Buffer[sizeof("\\DosDevice\\A:")];
    } SinglePoint;
    MOUNTMGR_MOUNT_POINTS MountPoints;
    PMOUNTMGR_MOUNT_POINTS AllocatedPoints;

    MountMgrHandle = GetMountMgrHandle(FILE_READ_ATTRIBUTES);
    if (!MountMgrHandle)
    {
        win_skip("MountMgr unavailable: %lu\n", GetLastError());
        return;
    }

    ZeroMemory(&SinglePoint, sizeof(SinglePoint));

    /* Retrieve the size needed to enumerate all the existing mount points */
    Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                          &SinglePoint, sizeof(MOUNTMGR_MOUNT_POINT), // Size without the string.
                          &MountPoints, sizeof(MountPoints),
                          &BytesReturned, NULL);
    LastError = GetLastError();
    ok(Ret == FALSE, "IOCTL unexpectedly succeeded\n");
    ok(LastError == ERROR_MORE_DATA, "Unexpected failure: %lu\n", LastError);

    /* Allocate a suitably-sized buffer for the mount points */
    AllocatedPoints = RtlAllocateHeap(RtlGetProcessHeap(), 0, MountPoints.Size);
    if (AllocatedPoints == NULL)
    {
        skip("Insufficient memory\n");
    }
    else
    {
        AllocatedPoints->NumberOfMountPoints = 0;

        /* Retrieve the list of all the existing mount points */
        Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                              &SinglePoint, sizeof(MOUNTMGR_MOUNT_POINT), // Size without the string.
                              AllocatedPoints, MountPoints.Size,
                              &BytesReturned, NULL);
        ok(Ret == TRUE, "IOCTL unexpectedly failed: %lu\n", GetLastError());

        for (i = 0; i < AllocatedPoints->NumberOfMountPoints; ++i)
        {
            TraceMountPoint(AllocatedPoints, &AllocatedPoints->MountPoints[i]);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, AllocatedPoints);
    }

    /* Now, find the first unused drive letter */
    Drives = GetLogicalDrives();
    if (Drives == 0)
    {
        skip("Drives map unavailable: %lu\n", GetLastError());
        goto Done;
    }
    for (i = 0; i <= 'Z'-'A'; ++i)
    {
        if (!(Drives & (1 << i)))
            break;
    }
    if (i > 'Z'-'A')
    {
        skip("All the drive letters are in use, skipping\n");
        goto Done;
    }

    /* Check that this drive letter is not an existing mount point */
    SinglePoint.SymbolicLinkNameOffset = ((ULONG_PTR)&SinglePoint.Buffer - (ULONG_PTR)&SinglePoint);
    SinglePoint.SymbolicLinkNameLength = sizeof(SinglePoint.Buffer) - sizeof(UNICODE_NULL);
    StringCbPrintfW(SinglePoint.Buffer, sizeof(SinglePoint.Buffer),
                    L"\\DosDevice\\%C:", L'A' + i);

    Ret = DeviceIoControl(MountMgrHandle, IOCTL_MOUNTMGR_QUERY_POINTS,
                          &SinglePoint, sizeof(SinglePoint),
                          &MountPoints, sizeof(MountPoints),
                          &BytesReturned, NULL);
    LastError = GetLastError();
    ok(Ret == FALSE, "IOCTL unexpectedly succeeded\n");
    ok(LastError == ERROR_FILE_NOT_FOUND, "Unexpected failure: %lu\n", LastError);

Done:
    CloseHandle(MountMgrHandle);
}
