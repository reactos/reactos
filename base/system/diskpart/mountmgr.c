/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/mountmgr.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Eric Kohl
 */

#include "diskpart.h"

#include <mountmgr.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

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


BOOL
GetAutomountState(
    _Out_ PBOOL State)
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
        return FALSE;
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
        return FALSE;
    }

    if (State)
        *State = (AutoMount.CurrentState == Enabled);

    return TRUE;
}


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
        return FALSE;
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
        return FALSE;
    }

    return TRUE;
}



BOOL
AssignDriveLetter(
    _In_ PWSTR DeviceName,
    _In_ WCHAR DriveLetter)
{
    WCHAR DosDeviceName[30];
    ULONG DosDeviceNameLength, DeviceNameLength; 
    ULONG InputBufferLength;
    PMOUNTMGR_CREATE_POINT_INPUT InputBuffer;
    HANDLE MountMgrHandle = NULL;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    BOOL Ret = TRUE;

    DPRINT1("AssignDriveLetter(%S %c)\n", DeviceName, DriveLetter);

    DeviceNameLength = wcslen(DeviceName) * sizeof(WCHAR);

    swprintf(DosDeviceName, L"\\DosDevices\\%c:", DriveLetter);
    DosDeviceNameLength = wcslen(DosDeviceName) * sizeof(WCHAR);

    /* Allocate the input buffer for the MountMgr */
    InputBufferLength = DosDeviceNameLength + DeviceNameLength + sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    InputBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, InputBufferLength);
    if (InputBuffer == NULL)
    {
        DPRINT1("InputBuffer allocation failed!\n");
        return FALSE;
    }

    /* Fill the input buffer */
    InputBuffer->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    InputBuffer->SymbolicLinkNameLength = DosDeviceNameLength;
    InputBuffer->DeviceNameOffset = DosDeviceNameLength + sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    InputBuffer->DeviceNameLength = DeviceNameLength;
    RtlCopyMemory((PVOID)((ULONG_PTR)InputBuffer + sizeof(MOUNTMGR_CREATE_POINT_INPUT)),
                  DosDeviceName,
                  DosDeviceNameLength);
    RtlCopyMemory((PVOID)((ULONG_PTR)InputBuffer + InputBuffer->DeviceNameOffset),
                  DeviceName,
                  DeviceNameLength);

    Status = OpenMountManager(&MountMgrHandle, GENERIC_READ | GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenMountManager() failed (Status 0x%08lx)\n", Status);
        Ret = FALSE;
        goto done;
    }

    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_MOUNTMGR_CREATE_POINT,
                                   InputBuffer,
                                   InputBufferLength,
                                   NULL,
                                   0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() failed (Status 0x%08lx)\n", Status);
        Ret = FALSE;
        goto done;
    }

done:
    if (MountMgrHandle)
        NtClose(MountMgrHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, InputBuffer);

    return Ret;
}


BOOL
AssignNextDriveLetter(
    _In_ PWSTR DeviceName,
    _Out_ PWCHAR DriveLetter)
{
    MOUNTMGR_DRIVE_LETTER_INFORMATION LetterInfo;
    PMOUNTMGR_DRIVE_LETTER_TARGET InputBuffer;
    ULONG DeviceNameLength, InputBufferLength;
    HANDLE MountMgrHandle = NULL;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    BOOL Ret = TRUE;

    DPRINT("AssignNextDriveLetter(%S %p)\n", DeviceName, DriveLetter);

    DeviceNameLength = wcslen(DeviceName) * sizeof(WCHAR);

    InputBufferLength = DeviceNameLength + FIELD_OFFSET(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName);
    InputBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, InputBufferLength);
    if (InputBuffer == NULL)
    {
        DPRINT1("InputBuffer allocation failed!\n");
        return FALSE;
    }

    /* And fill it with the device hat needs a drive letter */
    InputBuffer->DeviceNameLength = DeviceNameLength;
    RtlCopyMemory(&InputBuffer->DeviceName[0],
                  DeviceName,
                  DeviceNameLength);

    Status = OpenMountManager(&MountMgrHandle, GENERIC_READ | GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenMountManager() failed (Status 0x%08lx)\n", Status);
        Ret = FALSE;
        goto done;
    }

    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_MOUNTMGR_CREATE_POINT,
                                   InputBuffer,
                                   InputBufferLength,
                                   &LetterInfo,
                                   sizeof(LetterInfo));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() failed (Status 0x%08lx)\n", Status);
        Ret = FALSE;
        goto done;
    }

done:
    if (MountMgrHandle)
        NtClose(MountMgrHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, InputBuffer);

    if (Ret)
    {
        if (LetterInfo.DriveLetterWasAssigned)
            *DriveLetter = LetterInfo.CurrentDriveLetter;
        else
            *DriveLetter = UNICODE_NULL;
    }

    return Ret;
}


BOOL
DeleteDriveLetter(
    _In_ WCHAR DriveLetter)
{
    PMOUNTMGR_MOUNT_POINT InputBuffer = NULL;
    PMOUNTMGR_MOUNT_POINTS OutputBuffer = NULL;
    WCHAR DosDeviceName[30];
    ULONG InputBufferLength, DosDeviceNameLength;
    ULONG OutputBufferLength = 0x1000;
    HANDLE MountMgrHandle = NULL;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    BOOL Ret = TRUE;

    DPRINT("DeleteDriveLetter(%c)\n", DriveLetter);

    /* Setup the device name of the letter to delete */
    swprintf(DosDeviceName, L"\\DosDevices\\%c:", DriveLetter);
    DosDeviceNameLength = wcslen(DosDeviceName) * sizeof(WCHAR);

    /* Allocate the input buffer for MountMgr */
    InputBufferLength = DosDeviceNameLength + sizeof(MOUNTMGR_MOUNT_POINT);
    InputBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, InputBufferLength);
    if (InputBuffer == NULL)
    {
        DPRINT1("InputBuffer allocation failed!\n");
        return FALSE;
    }

    /* Fill it in */
    InputBuffer->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    InputBuffer->SymbolicLinkNameLength = DosDeviceNameLength;
    RtlCopyMemory(&InputBuffer[1], DosDeviceName, DosDeviceNameLength);

    /* Allocate big enough output buffer (we don't care about the output) */
    OutputBuffer = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, OutputBufferLength);
    if (OutputBuffer == NULL)
    {
        DPRINT1("OutputBuffer allocation failed!\n");
        Ret = FALSE;
        goto done;
    }

    OutputBuffer->Size = OutputBufferLength;

    Status = OpenMountManager(&MountMgrHandle, GENERIC_READ | GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("OpenMountManager() failed (Status 0x%08lx)\n", Status);
        Ret = FALSE;
        goto done;
    }

    Status = NtDeviceIoControlFile(MountMgrHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_MOUNTMGR_DELETE_POINTS,
                                   InputBuffer,
                                   InputBufferLength,
                                   OutputBuffer,
                                   OutputBufferLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() failed (Status 0x%08lx)\n", Status);
        Ret = FALSE;
        goto done;
    }

done:
    if (MountMgrHandle)
        NtClose(MountMgrHandle);

    if (InputBuffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, InputBuffer);

    if (OutputBuffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, OutputBuffer);

    return Ret;
}
