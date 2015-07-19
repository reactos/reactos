/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/select.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


ULONG CurrentDisk = (ULONG)-1;
ULONG CurrentPartition = (ULONG)-1;


static
VOID
SelectDisk(
    INT argc,
    LPWSTR *argv)
{
    SYSTEM_DEVICE_INFORMATION Sdi;
    ULONG ReturnSize;
    NTSTATUS Status;
    LONG value;
    LPWSTR endptr = NULL;

    DPRINT("Select Disk()\n");

    if (argc > 3)
    {
        PrintResourceString(IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (argc == 2)
    {
        if (CurrentDisk == (ULONG)-1)
            PrintResourceString(IDS_SELECT_NO_DISK);
        else
            PrintResourceString(IDS_SELECT_DISK, CurrentDisk);
        return;
    }

    value = wcstol(argv[2], &endptr, 10);
    if (((value == 0) && (endptr == argv[2])) ||
        (value < 0))
    {
        PrintResourceString(IDS_ERROR_INVALID_ARGS);
        return;
    }

    Status = NtQuerySystemInformation(SystemDeviceInformation,
                                      &Sdi,
                                      sizeof(SYSTEM_DEVICE_INFORMATION),
                                      &ReturnSize);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    if ((ULONG)value >= Sdi.NumberOfDisks)
    {
        PrintResourceString(IDS_ERROR_INVALID_DISK);
        return;
    }

    CurrentDisk = (ULONG)value;
    CurrentPartition = (ULONG)-1;

    PrintResourceString(IDS_SELECT_DISK, CurrentDisk);
}


static
VOID
SelectPartition(
    INT argc,
    LPWSTR *argv)
{
    LONG value;
    LPWSTR endptr = NULL;

    DPRINT("Select Partition()\n");

    if (argc > 3)
    {
        PrintResourceString(IDS_ERROR_INVALID_ARGS);
        return;
    }

    if (argc == 2)
    {
        if (CurrentPartition == (ULONG)-1)
            PrintResourceString(IDS_SELECT_NO_PARTITION);
        else
            PrintResourceString(IDS_SELECT_PARTITION, CurrentPartition);
        return;
    }

    value = wcstol(argv[2], &endptr, 10);
    if (((value == 0) && (endptr == argv[2])) ||
        (value < 0))
    {
        PrintResourceString(IDS_ERROR_INVALID_ARGS);
        return;
    }

    /* FIXME: Check the new partition number */

    CurrentPartition = (ULONG)value;

    PrintResourceString(IDS_SELECT_PARTITION, CurrentPartition);
}


BOOL
select_main(
    INT argc,
    LPWSTR *argv)
{
    /* gets the first word from the string */
    if (argc == 1)
    {
        PrintResourceString(IDS_HELP_CMD_SELECT);
        return TRUE;
    }

    /* determines which to list (disk, partition, etc.) */
    if (!wcsicmp(argv[1], L"disk"))
        SelectDisk(argc, argv);
    else if (!wcsicmp(argv[1], L"partition"))
        SelectPartition(argc, argv);
    else
        PrintResourceString(IDS_HELP_CMD_SELECT);

    return TRUE;
}
