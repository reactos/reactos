/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH(S)
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"


/**
 * @brief
 * Invokes either IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH or
 * IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS for testing, given
 * the volume device name, and returns an allocated volume
 * paths buffer. This buffer must be freed by the caller via
 * RtlFreeHeap(RtlGetProcessHeap(), ...) .
 *
 * These IOCTLs return both the drive letter (if any) and the
 * volume GUID symlink path, as well as any other file-system
 * mount reparse points linking to the volume.
 **/
static VOID
Call_QueryDosVolume_Path_Paths(
    _In_ HANDLE MountMgrHandle,
    _In_ PCWSTR NtVolumeName,
    _In_ ULONG IoctlPathOrPaths,
    _Out_ PMOUNTMGR_VOLUME_PATHS* pVolumePathPtr)
{
    NTSTATUS Status;
    ULONG Length;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING VolumeName;
    MOUNTMGR_VOLUME_PATHS VolumePath;
    PMOUNTMGR_VOLUME_PATHS VolumePathPtr;
    ULONG DeviceNameLength;
    /*
     * This variable is used to query the device name.
     * It's based on MOUNTMGR_TARGET_NAME (mountmgr.h).
     * Doing it this way prevents memory allocation.
     * The device name won't be longer.
     */
    struct
    {
        USHORT NameLength;
        WCHAR DeviceName[256];
    } DeviceName;


    *pVolumePathPtr = NULL;

    /* First, build the corresponding device name */
    RtlInitUnicodeString(&VolumeName, NtVolumeName);
    DeviceName.NameLength = VolumeName.Length;
    RtlCopyMemory(&DeviceName.DeviceName, VolumeName.Buffer, VolumeName.Length);
    DeviceNameLength = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) + DeviceName.NameLength;

    /* Now, query the MountMgr for the DOS path(s) */
    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IoctlPathOrPaths,
                                   &DeviceName, DeviceNameLength,
                                   &VolumePath, sizeof(VolumePath));

    /* Check for unsupported device */
    if (Status == STATUS_NO_MEDIA_IN_DEVICE || Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        skip("Device '%S': Doesn't support MountMgr queries, Status 0x%08lx\n",
             NtVolumeName, Status);
        return;
    }

    /* The only tolerated failure here is buffer too small, which is expected */
    ok(NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW),
       "Device '%S': IOCTL 0x%lx failed unexpectedly, Status 0x%08lx\n",
       NtVolumeName, IoctlPathOrPaths, Status);
    if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_OVERFLOW))
    {
        skip("Device '%S': Wrong Status\n", NtVolumeName);
        return;
    }

    /* Compute the needed size to store the DOS path(s).
     * Even if MOUNTMGR_VOLUME_PATHS allows bigger name lengths
     * than MAXUSHORT, we can't use them, because we have to return
     * this in an UNICODE_STRING that stores length in a USHORT. */
    Length = sizeof(VolumePath) + VolumePath.MultiSzLength;
    ok(Length <= MAXUSHORT,
       "Device '%S': DOS volume path too large: %lu\n",
       NtVolumeName, Length);
    if (Length > MAXUSHORT)
    {
        skip("Device '%S': Wrong Length\n", NtVolumeName);
        return;
    }

    /* Allocate the buffer and fill it with test pattern */
    VolumePathPtr = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (!VolumePathPtr)
    {
        skip("Device '%S': Failed to allocate buffer with size %lu)\n",
             NtVolumeName, Length);
        return;
    }
    RtlFillMemory(VolumePathPtr, Length, 0xCC);

    /* Re-query the DOS path(s) with the proper size */
    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IoctlPathOrPaths,
                                   &DeviceName, DeviceNameLength,
                                   VolumePathPtr, Length);
    ok(NT_SUCCESS(Status),
       "Device '%S': IOCTL 0x%lx failed unexpectedly, Status 0x%08lx\n",
       NtVolumeName, IoctlPathOrPaths, Status);

    if (winetest_debug > 1)
    {
        trace("Buffer:\n");
        DumpBuffer(VolumePathPtr, Length);
        printf("\n");
    }

    /* Return the buffer */
    *pVolumePathPtr = VolumePathPtr;
}

/**
 * @brief
 * Invokes IOCTL_MOUNTMGR_QUERY_POINTS for testing, given
 * the volume device name, and returns an allocated mount
 * points buffer. This buffer must be freed by the caller
 * via RtlFreeHeap(RtlGetProcessHeap(), ...) .
 *
 * This IOCTL only returns both the drive letter (if any)
 * and the volume GUID symlink path, but does NOT return
 * file-system mount reparse points.
 **/
static VOID
Call_QueryPoints(
    _In_ HANDLE MountMgrHandle,
    _In_ PCWSTR NtVolumeName,
    _Out_ PMOUNTMGR_MOUNT_POINTS* pMountPointsPtr)
{
    NTSTATUS Status;
    ULONG Length;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING VolumeName;
    MOUNTMGR_MOUNT_POINTS MountPoints;
    PMOUNTMGR_MOUNT_POINTS MountPointsPtr;
    /*
     * This variable is used to query the device name.
     * It's based on MOUNTMGR_MOUNT_POINT (mountmgr.h).
     * Doing it this way prevents memory allocation.
     * The device name won't be longer.
     */
    struct
    {
        MOUNTMGR_MOUNT_POINT;
        WCHAR DeviceName[256];
    } DeviceName;


    *pMountPointsPtr = NULL;

    /* First, build the corresponding device name */
    RtlInitUnicodeString(&VolumeName, NtVolumeName);
    DeviceName.SymbolicLinkNameOffset = DeviceName.UniqueIdOffset = 0;
    DeviceName.SymbolicLinkNameLength = DeviceName.UniqueIdLength = 0;
    DeviceName.DeviceNameOffset = ((ULONG_PTR)&DeviceName.DeviceName - (ULONG_PTR)&DeviceName);
    DeviceName.DeviceNameLength = VolumeName.Length;
    RtlCopyMemory(&DeviceName.DeviceName, VolumeName.Buffer, VolumeName.Length);

    /* Now, query the MountMgr for the mount points */
    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_QUERY_POINTS,
                                   &DeviceName, sizeof(DeviceName),
                                   &MountPoints, sizeof(MountPoints));

    /* Check for unsupported device */
    if (Status == STATUS_NO_MEDIA_IN_DEVICE || Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        skip("Device '%S': Doesn't support MountMgr queries, Status 0x%08lx\n",
             NtVolumeName, Status);
        return;
    }

    /* The only tolerated failure here is buffer too small, which is expected */
    ok(NT_SUCCESS(Status) || (Status == STATUS_BUFFER_OVERFLOW),
       "Device '%S': IOCTL 0x%lx failed unexpectedly, Status 0x%08lx\n",
       NtVolumeName, IOCTL_MOUNTMGR_QUERY_POINTS, Status);
    if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_OVERFLOW))
    {
        skip("Device '%S': Wrong Status\n", NtVolumeName);
        return;
    }

    /* Compute the needed size to retrieve the mount points */
    Length = MountPoints.Size;

    /* Allocate the buffer and fill it with test pattern */
    MountPointsPtr = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (!MountPointsPtr)
    {
        skip("Device '%S': Failed to allocate buffer with size %lu)\n",
             NtVolumeName, Length);
        return;
    }
    RtlFillMemory(MountPointsPtr, Length, 0xCC);

    /* Re-query the mount points with the proper size */
    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL, NULL, NULL,
                                   &IoStatusBlock,
                                   IOCTL_MOUNTMGR_QUERY_POINTS,
                                   &DeviceName, sizeof(DeviceName),
                                   MountPointsPtr, Length);
    ok(NT_SUCCESS(Status),
       "Device '%S': IOCTL 0x%lx failed unexpectedly, Status 0x%08lx\n",
       NtVolumeName, IOCTL_MOUNTMGR_QUERY_POINTS, Status);

    if (winetest_debug > 1)
    {
        trace("IOCTL_MOUNTMGR_QUERY_POINTS returned:\n"
              "  Size: %lu\n"
              "  NumberOfMountPoints: %lu\n",
              MountPointsPtr->Size,
              MountPointsPtr->NumberOfMountPoints);

        trace("Buffer:\n");
        DumpBuffer(MountPointsPtr, Length);
        printf("\n");
    }

    /* Return the buffer */
    *pMountPointsPtr = MountPointsPtr;
}


#define IS_DRIVE_LETTER_PFX(s) \
  ((s)->Length >= 2*sizeof(WCHAR) && (s)->Buffer[0] >= 'A' && \
   (s)->Buffer[0] <= 'Z' && (s)->Buffer[1] == ':')

/* Differs from MOUNTMGR_IS_DRIVE_LETTER(): no '\DosDevices\' accounted for */
#define IS_DRIVE_LETTER(s) \
  (IS_DRIVE_LETTER_PFX(s) && (s)->Length == 2*sizeof(WCHAR))


/**
 * @brief   Tests the output of IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH.
 **/
static VOID
Test_QueryDosVolumePath(
    _In_ PCWSTR NtVolumeName,
    _In_ PMOUNTMGR_VOLUME_PATHS VolumePath)
{
    UNICODE_STRING DosPath;

    UNREFERENCED_PARAMETER(NtVolumeName);

    /* The VolumePath should contain one NUL-terminated string (always there?),
     * plus one final NUL-terminator */
    ok(VolumePath->MultiSzLength >= 2 * sizeof(UNICODE_NULL),
       "DOS volume path string too short (length: %lu)\n",
       VolumePath->MultiSzLength / sizeof(WCHAR));
    ok(VolumePath->MultiSz[VolumePath->MultiSzLength / sizeof(WCHAR) - 2] == UNICODE_NULL,
       "Missing NUL-terminator (2)\n");
    ok(VolumePath->MultiSz[VolumePath->MultiSzLength / sizeof(WCHAR) - 1] == UNICODE_NULL,
       "Missing NUL-terminator (1)\n");

    /* Build the result string */
    // RtlInitUnicodeString(&DosPath, VolumePath->MultiSz);
    DosPath.Length = (USHORT)VolumePath->MultiSzLength - 2 * sizeof(UNICODE_NULL);
    DosPath.MaximumLength = DosPath.Length + sizeof(UNICODE_NULL);
    DosPath.Buffer = VolumePath->MultiSz;

    /* The returned DOS path is either a drive letter (*WITHOUT* any
     * '\DosDevices\' prefix present) or a Win32 file-system reparse point
     * path, or a volume GUID name in Win32 format, i.e. prefixed by '\\?\' */
    ok(IS_DRIVE_LETTER_PFX(&DosPath) || MOUNTMGR_IS_DOS_VOLUME_NAME(&DosPath),
       "Invalid DOS volume path returned '%s'\n", wine_dbgstr_us(&DosPath));
}

/**
 * @brief   Tests the output of IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS.
 **/
static VOID
Test_QueryDosVolumePaths(
    _In_ PCWSTR NtVolumeName,
    _In_ PMOUNTMGR_VOLUME_PATHS VolumePaths,
    _In_opt_ PMOUNTMGR_VOLUME_PATHS VolumePath)
{
    UNICODE_STRING DosPath;
    PCWSTR pMountPoint;

    /* The VolumePaths should contain zero or more NUL-terminated strings,
     * plus one final NUL-terminator */

    ok(VolumePaths->MultiSzLength >= sizeof(UNICODE_NULL),
       "DOS volume path string too short (length: %lu)\n",
       VolumePaths->MultiSzLength / sizeof(WCHAR));

    /* Check for correct double-NUL-termination, if there is at least one string */
    if (VolumePaths->MultiSzLength >= 2 * sizeof(UNICODE_NULL),
        VolumePaths->MultiSz[0] != UNICODE_NULL)
    {
        ok(VolumePaths->MultiSz[VolumePaths->MultiSzLength / sizeof(WCHAR) - 2] == UNICODE_NULL,
           "Missing NUL-terminator (2)\n");
    }
    /* Check for the final NUL-terminator */
    ok(VolumePaths->MultiSz[VolumePaths->MultiSzLength / sizeof(WCHAR) - 1] == UNICODE_NULL,
       "Missing NUL-terminator (1)\n");

    if (winetest_debug > 1)
    {
        trace("\n%S =>\n", NtVolumeName);
        for (pMountPoint = VolumePaths->MultiSz; *pMountPoint;
             pMountPoint += wcslen(pMountPoint) + 1)
        {
            printf("  '%S'\n", pMountPoint);
        }
        printf("\n");
    }

    for (pMountPoint = VolumePaths->MultiSz; *pMountPoint;
         pMountPoint += wcslen(pMountPoint) + 1)
    {
        /* The returned DOS path is either a drive letter (*WITHOUT* any
         * '\DosDevices\' prefix present) or a Win32 file-system reparse point
         * path, or a volume GUID name in Win32 format, i.e. prefixed by '\\?\' */
        RtlInitUnicodeString(&DosPath, pMountPoint);
        ok(IS_DRIVE_LETTER_PFX(&DosPath) || MOUNTMGR_IS_DOS_VOLUME_NAME(&DosPath),
           "Invalid DOS volume path returned '%s'\n", wine_dbgstr_us(&DosPath));
    }

    /*
     * If provided, verify that the single VolumePath is found at the
     * first position in the volume paths list, *IF* this is a DOS path;
     * otherwise if it's a Volume{GUID} path, this means there is no
     * DOS path associated, and none is listed in the volume paths list.
     */
    if (VolumePath)
    {
        RtlInitUnicodeString(&DosPath, VolumePath->MultiSz);
        if (IS_DRIVE_LETTER_PFX(&DosPath))
        {
            /*
             * The single path is a DOS path (single drive letter or Win32
             * file-system reparse point path). It has to be listed first
             * in the volume paths list.
             */
            UNICODE_STRING FirstPath;
            BOOLEAN AreEqual;

            ok(VolumePaths->MultiSzLength >= 2 * sizeof(UNICODE_NULL),
               "DOS VolumePaths list isn't long enough\n");
            ok(*VolumePaths->MultiSz != UNICODE_NULL,
               "Empty DOS VolumePaths list\n");

            RtlInitUnicodeString(&FirstPath, VolumePaths->MultiSz);
            AreEqual = RtlEqualUnicodeString(&DosPath, &FirstPath, FALSE);
            ok(AreEqual, "DOS paths '%s' and '%s' are not the same!\n",
               wine_dbgstr_us(&DosPath), wine_dbgstr_us(&FirstPath));
        }
        else if (MOUNTMGR_IS_DOS_VOLUME_NAME(&DosPath))
        {
            /*
             * The single "DOS" path is actually a volume name. This means
             * that it wasn't really mounted, and the volume paths list must
             * be empty. It contains only the last NUL-terminator.
             */
            ok(VolumePaths->MultiSzLength == sizeof(UNICODE_NULL),
               "DOS VolumePaths list isn't 1 WCHAR long\n");
            ok(*VolumePaths->MultiSz == UNICODE_NULL,
               "Non-empty DOS VolumePaths list\n");
        }
        else
        {
            /* The volume path is invalid (shouldn't happen) */
            ok(FALSE, "Invalid DOS volume path returned '%s'\n", wine_dbgstr_us(&DosPath));
        }
    }
}

static BOOLEAN
doesPathExistInMountPoints(
    _In_ PMOUNTMGR_MOUNT_POINTS MountPoints,
    _In_ PUNICODE_STRING DosPath)
{
    UNICODE_STRING DosDevicesPrefix = RTL_CONSTANT_STRING(L"\\DosDevices\\");
    ULONG i;
    BOOLEAN IsDosVolName;
    BOOLEAN Found = FALSE;

    IsDosVolName = MOUNTMGR_IS_DOS_VOLUME_NAME(DosPath);
    /* Temporarily patch \\?\ to \??\ in DosPath for comparison */
    if (IsDosVolName)
        DosPath->Buffer[1] = L'?';

    for (i = 0; i < MountPoints->NumberOfMountPoints; ++i)
    {
        UNICODE_STRING SymLink;

        SymLink.Length = SymLink.MaximumLength = MountPoints->MountPoints[i].SymbolicLinkNameLength;
        SymLink.Buffer = (PWCHAR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[i].SymbolicLinkNameOffset);

        if (IS_DRIVE_LETTER(DosPath))
        {
            if (RtlPrefixUnicodeString(&DosDevicesPrefix, &SymLink, FALSE))
            {
                /* Advance past the prefix */
                SymLink.Length -= DosDevicesPrefix.Length;
                SymLink.MaximumLength -= DosDevicesPrefix.Length;
                SymLink.Buffer += DosDevicesPrefix.Length / sizeof(WCHAR);

                Found = RtlEqualUnicodeString(DosPath, &SymLink, FALSE);
            }
        }
        else if (/*MOUNTMGR_IS_DOS_VOLUME_NAME(DosPath) ||*/ // See above
                 MOUNTMGR_IS_NT_VOLUME_NAME(DosPath))
        {
            Found = RtlEqualUnicodeString(DosPath, &SymLink, FALSE);
        }
        else
        {
            /* Just test for simple string comparison, the path should not be found */
            Found = RtlEqualUnicodeString(DosPath, &SymLink, FALSE);
        }

        /* Stop searching if we've found something */
        if (Found)
            break;
    }

    /* Revert \??\ back to \\?\ */
    if (IsDosVolName)
        DosPath->Buffer[1] = L'\\';

    return Found;
}

/**
 * @brief   Tests the output of IOCTL_MOUNTMGR_QUERY_POINTS.
 **/
static VOID
Test_QueryPoints(
    _In_ PCWSTR NtVolumeName,
    _In_ PMOUNTMGR_MOUNT_POINTS MountPoints,
    _In_opt_ PMOUNTMGR_VOLUME_PATHS VolumePath,
    _In_opt_ PMOUNTMGR_VOLUME_PATHS VolumePaths)
{
    UNICODE_STRING DosPath;
    PCWSTR pMountPoint;
    BOOLEAN ExpectedFound, Found;

    if (winetest_debug > 1)
    {
        ULONG i;
        trace("\n%S =>\n", NtVolumeName);
        for (i = 0; i < MountPoints->NumberOfMountPoints; ++i)
        {
            UNICODE_STRING DevName, SymLink;

            DevName.Length = DevName.MaximumLength = MountPoints->MountPoints[i].DeviceNameLength;
            DevName.Buffer = (PWCHAR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[i].DeviceNameOffset);

            SymLink.Length = SymLink.MaximumLength = MountPoints->MountPoints[i].SymbolicLinkNameLength;
            SymLink.Buffer = (PWCHAR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[i].SymbolicLinkNameOffset);

            printf("  '%s' -- '%s'\n", wine_dbgstr_us(&DevName), wine_dbgstr_us(&SymLink));
        }
        printf("\n");
    }

    /*
     * The Win32 file-system reparse point paths are NOT listed amongst
     * the mount points. Only the drive letter and the volume GUID name
     * are, but in an NT format (using '\DosDevices\' or '\??\' prefixes).
     */

    if (VolumePath)
    {
        /* VolumePath can either be a drive letter (usual case), a Win32
         * reparse point path (if the volume is mounted only with these),
         * or a volume GUID name (if the volume is NOT mounted). */
        RtlInitUnicodeString(&DosPath, VolumePath->MultiSz);
        ExpectedFound = (IS_DRIVE_LETTER(&DosPath) || MOUNTMGR_IS_DOS_VOLUME_NAME(&DosPath));
        Found = doesPathExistInMountPoints(MountPoints, &DosPath);
        ok(Found == ExpectedFound,
           "DOS path '%s' %sfound in the mount points list, expected %sto be found\n",
           wine_dbgstr_us(&DosPath), Found ? "" : "NOT ", ExpectedFound ? "" : "NOT ");
    }

    if (VolumePaths)
    {
        /* VolumePaths only contains a drive letter (usual case) or a Win32
         * reparse point path (if the volume is mounted only with these).
         * If the volume is NOT mounted, VolumePaths does not list the
         * volume GUID name, contrary to VolumePath. */
        for (pMountPoint = VolumePaths->MultiSz; *pMountPoint;
             pMountPoint += wcslen(pMountPoint) + 1)
        {
            /* Only the drive letter (but NOT the volume GUID name!) can be found in the list */
            RtlInitUnicodeString(&DosPath, pMountPoint);
            ExpectedFound = IS_DRIVE_LETTER(&DosPath);
            Found = doesPathExistInMountPoints(MountPoints, &DosPath);
            ok(Found == ExpectedFound,
               "DOS path '%s' %sfound in the mount points list, expected %sto be found\n",
               wine_dbgstr_us(&DosPath), Found ? "" : "NOT ", ExpectedFound ? "" : "NOT ");
        }
    }
}

/**
 * @brief
 * Tests the consistency of IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
 * IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS and IOCTL_MOUNTMGR_QUERY_POINTS.
 **/
static VOID
Test_QueryDosVolumePathAndPaths(
    _In_ HANDLE MountMgrHandle,
    _In_ PCWSTR NtVolumeName)
{
    PMOUNTMGR_VOLUME_PATHS VolumePath = NULL;
    PMOUNTMGR_VOLUME_PATHS VolumePaths = NULL;
    PMOUNTMGR_MOUNT_POINTS MountPoints = NULL;

    if (winetest_debug > 1)
        trace("%S\n", NtVolumeName);

    Call_QueryDosVolume_Path_Paths(MountMgrHandle,
                                   NtVolumeName,
                                   IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH,
                                   &VolumePath);
    if (VolumePath)
        Test_QueryDosVolumePath(NtVolumeName, VolumePath);
    else
        skip("Device '%S': IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH failed\n", NtVolumeName);

    Call_QueryDosVolume_Path_Paths(MountMgrHandle,
                                   NtVolumeName,
                                   IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
                                   &VolumePaths);
    if (VolumePaths)
        Test_QueryDosVolumePaths(NtVolumeName, VolumePaths, VolumePath);
    else
        skip("Device '%S': IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS failed\n", NtVolumeName);

    Call_QueryPoints(MountMgrHandle, NtVolumeName, &MountPoints);
    if (MountPoints)
        Test_QueryPoints(NtVolumeName, MountPoints, VolumePath, VolumePaths);
    else
        skip("Device '%S': IOCTL_MOUNTMGR_QUERY_POINTS failed\n", NtVolumeName);

    if (MountPoints)
        RtlFreeHeap(RtlGetProcessHeap(), 0, MountPoints);
    if (VolumePaths)
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePaths);
    if (VolumePath)
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePath);
}


START_TEST(QueryDosVolumePaths)
{
    HANDLE MountMgrHandle;
    HANDLE hFindVolume;
    WCHAR szVolumeName[MAX_PATH];

    MountMgrHandle = GetMountMgrHandle(FILE_READ_ATTRIBUTES);
    if (!MountMgrHandle)
    {
        win_skip("MountMgr unavailable: %lu\n", GetLastError());
        return;
    }

    hFindVolume = FindFirstVolumeW(szVolumeName, _countof(szVolumeName));
    if (hFindVolume == INVALID_HANDLE_VALUE)
        goto otherTests;
    do
    {
        UNICODE_STRING VolumeName;
        USHORT Length;

        /*
         * The Win32 FindFirst/NextVolumeW() functions convert the '\??\'
         * prefix in '\??\Volume{...}' to '\\?\' and append a trailing
         * backslash. Test this behaviour.
         *
         * NOTE: these functions actively filter out anything that is NOT
         * '\??\Volume{...}' returned from IOCTL_MOUNTMGR_QUERY_POINTS.
         * Thus, it also excludes mount-points specified as drive letters,
         * like '\DosDevices\C:' .
         */

        RtlInitUnicodeString(&VolumeName, szVolumeName);
        Length = VolumeName.Length / sizeof(WCHAR);
        ok(Length >= 1 && VolumeName.Buffer[Length - 1] == L'\\',
           "No trailing backslash found\n");

        /* Remove the trailing backslash */
        if (Length >= 1)
        {
            VolumeName.Length -= sizeof(WCHAR);
            if (szVolumeName[Length - 1] == L'\\')
                szVolumeName[Length - 1] = UNICODE_NULL;
        }

        ok(MOUNTMGR_IS_DOS_VOLUME_NAME(&VolumeName),
           "Invalid DOS volume path returned '%s'\n", wine_dbgstr_us(&VolumeName));

        /* Patch '\\?\' back to '\??\' to convert to an NT path */
        if (szVolumeName[0] == L'\\' && szVolumeName[1] == L'\\' &&
            szVolumeName[2] == L'?'  && szVolumeName[3] == L'\\')
        {
            szVolumeName[1] = L'?';
        }

        Test_QueryDosVolumePathAndPaths(MountMgrHandle, szVolumeName);
    } while (FindNextVolumeW(hFindVolume, szVolumeName, _countof(szVolumeName)));
    FindVolumeClose(hFindVolume);

otherTests:
    /* Test the drive containing SystemRoot */
    wcscpy(szVolumeName, L"\\DosDevices\\?:");
    szVolumeName[sizeof("\\DosDevices\\")-1] = SharedUserData->NtSystemRoot[0];
    Test_QueryDosVolumePathAndPaths(MountMgrHandle, szVolumeName);

    /* We are done */
    CloseHandle(MountMgrHandle);
}
