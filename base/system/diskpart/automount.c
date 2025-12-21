/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/automount.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


static
NTSTATUS
OpenMountManager(
    _Out_ PHANDLE MountMgrHandle,
    _In_ ACCESS_MASK Access)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceName;
    IO_STATUS_BLOCK Iosb;

    RtlInitUnicodeString(&DeviceName, MOUNTMGR_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               0,
                               NULL,
                               NULL);

    return NtOpenFile(MountMgrHandle,
                      Access | SYNCHRONIZE,
                      &ObjectAttributes,
                      &Iosb,
                      0,
                      FILE_SYNCHRONOUS_IO_NONALERT);
}


static
BOOL
ShowAutomountState(VOID)
{
    HANDLE MountMgrHandle;
    MOUNTMGR_QUERY_AUTO_MOUNT AutoMount;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    DPRINT("ShowAutomountState()\n");

    Status = OpenMountManager(&MountMgrHandle, GENERIC_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenMountManager() Status 0x%08lx\n", Status);
        return TRUE;
    }

    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT,
                                   NULL,
                                   0,
                                   &AutoMount,
                                   sizeof(AutoMount));

    NtClose(MountMgrHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() Status 0x%08lx\n", Status);
        return TRUE;
    }

    if (AutoMount.CurrentState == Enabled)
        ConResPuts(StdOut, IDS_AUTOMOUNT_ENABLED);
    else
        ConResPuts(StdOut, IDS_AUTOMOUNT_DISABLED);
    ConPuts(StdOut, L"\n");

    return TRUE;
}


static
BOOL
SetAutomountState(
    _In_ BOOL bEnable)
{
    HANDLE MountMgrHandle;
    MOUNTMGR_SET_AUTO_MOUNT AutoMount;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    DPRINT("SetAutomountState()\n");

    Status = OpenMountManager(&MountMgrHandle, GENERIC_READ | GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenMountManager() Status 0x%08lx\n", Status);
        return TRUE;
    }

    if (bEnable)
        AutoMount.NewState = Enabled;
    else
        AutoMount.NewState = Disabled;

    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_MOUNTMGR_SET_AUTO_MOUNT,
                                   &AutoMount,
                                   sizeof(AutoMount),
                                   NULL,
                                   0);

    NtClose(MountMgrHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() Status 0x%08lx\n", Status);
        return TRUE;
    }

    if (AutoMount.NewState == Enabled)
        ConResPuts(StdOut, IDS_AUTOMOUNT_ENABLED);
    else
        ConResPuts(StdOut, IDS_AUTOMOUNT_DISABLED);
    ConPuts(StdOut, L"\n");

    return TRUE;
}


static
BOOL
ScrubAutomount(VOID)
{
    HANDLE MountMgrHandle;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    DPRINT("ScrubAutomount()\n");

    Status = OpenMountManager(&MountMgrHandle, GENERIC_READ | GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenMountManager() Status 0x%08lx\n", Status);
        return TRUE;
    }

    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_MOUNTMGR_SCRUB_REGISTRY,
                                   NULL,
                                   0,
                                   NULL,
                                   0);

    NtClose(MountMgrHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() Status 0x%08lx\n", Status);
        return TRUE;
    }

    ConResPuts(StdOut, IDS_AUTOMOUNT_SCRUBBED);
    ConPuts(StdOut, L"\n");

    return TRUE;
}


BOOL
automount_main(
    INT argc,
    LPWSTR *argv)
{
    BOOL bDisable = FALSE, bEnable = FALSE, bScrub = FALSE;
#if 0
    BOOL bNoErr = FALSE;
#endif
    INT i;

    DPRINT("Automount()\n");

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
#if 0
            bNoErr = TRUE;
#endif
        }
    }

    for (i = 1; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"disable") == 0)
        {
            /* set automount state */
            bDisable = TRUE;
        }
        else if (_wcsicmp(argv[i], L"enable") == 0)
        {
            /* set automount state */
            bEnable = TRUE;
        }
        else if (_wcsicmp(argv[i], L"scrub") == 0)
        {
            /* scrub automount */
            bScrub = TRUE;
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }

    DPRINT("bDisable %u\n", bDisable);
    DPRINT("bEnable  %u\n", bEnable);
    DPRINT("bScrub   %u\n", bScrub);

    if (bDisable && bEnable)
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return TRUE;
    }

    if ((bDisable == FALSE) && (bEnable == FALSE) && (bScrub == FALSE))
    {
        DPRINT("Show automount\n");
        return ShowAutomountState();
    }

    if (bDisable)
    {
        DPRINT("Disable automount\n");
        return SetAutomountState(FALSE);
    }

    if (bEnable)
    {
        DPRINT("Enable automount\n");
        return SetAutomountState(TRUE);
    }

    if (bScrub)
    {
        DPRINT("Scrub automount\n");
        return ScrubAutomount();
    }

    return TRUE;
}
