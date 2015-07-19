/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/list.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

static
ULONGLONG
RoundingDivide(
   IN ULONGLONG Dividend,
   IN ULONGLONG Divisor)
{
    return (Dividend + Divisor / 2) / Divisor;
}


static
VOID
ListDisk(VOID)
{
    WCHAR Buffer[MAX_PATH];
    OBJECT_ATTRIBUTES ObjectAttributes;
    SYSTEM_DEVICE_INFORMATION Sdi;
    IO_STATUS_BLOCK Iosb;
    DISK_GEOMETRY DiskGeometry;
    ULONG ReturnSize;
    ULONG DiskNumber;
    UNICODE_STRING Name;
    HANDLE FileHandle;
    NTSTATUS Status;

    ULARGE_INTEGER DiskSize;
    ULARGE_INTEGER FreeSize;
    LPWSTR lpSizeUnit;
    LPWSTR lpFreeUnit;


    Status = NtQuerySystemInformation(SystemDeviceInformation,
                                      &Sdi,
                                      sizeof(SYSTEM_DEVICE_INFORMATION),
                                      &ReturnSize);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Header labels */
    PrintResourceString(IDS_LIST_DISK_HEAD);
    PrintResourceString(IDS_LIST_DISK_LINE);

    for (DiskNumber = 0; DiskNumber < Sdi.NumberOfDisks; DiskNumber++)
    {
        swprintf(Buffer,
                 L"\\Device\\Harddisk%d\\Partition0",
                 DiskNumber);
        RtlInitUnicodeString(&Name,
                             Buffer);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &Name,
                                   0,
                                   NULL,
                                   NULL);

        Status = NtOpenFile(&FileHandle,
                            FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                            &ObjectAttributes,
                            &Iosb,
                            FILE_SHARE_READ,
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if (NT_SUCCESS(Status))
        {
            Status = NtDeviceIoControlFile(FileHandle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &Iosb,
                                           IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                           NULL,
                                           0,
                                           &DiskGeometry,
                                           sizeof(DISK_GEOMETRY));
            if (NT_SUCCESS(Status))
            {
                DiskSize.QuadPart = DiskGeometry.Cylinders.QuadPart *
                                    (ULONGLONG)DiskGeometry.TracksPerCylinder *
                                    (ULONGLONG)DiskGeometry.SectorsPerTrack *
                                    (ULONGLONG)DiskGeometry.BytesPerSector;
                if (DiskSize.QuadPart >= 10737418240) /* 10 GB */
                {
                     DiskSize.QuadPart = RoundingDivide(DiskSize.QuadPart, 1073741824);
                     lpSizeUnit = L"GB";
                }
                else
                {
                    DiskSize.QuadPart = RoundingDivide(DiskSize.QuadPart, 1048576);
                    if (DiskSize.QuadPart == 0)
                        DiskSize.QuadPart = 1;
                    lpSizeUnit = L"MB";
                }

                /* FIXME */
                FreeSize.QuadPart = 0;
                lpFreeUnit = L"B";

                PrintResourceString(IDS_LIST_DISK_FORMAT,
                                    (CurrentDisk == DiskNumber) ? L'*': ' ',
                                    DiskNumber,
                                    L"Online",
                                    DiskSize.QuadPart,
                                    lpSizeUnit,
                                    FreeSize.QuadPart,
                                    lpFreeUnit,
                                    L" ",
                                    L" ");
            }
            else
            {
                printf("Status 0x%lx\n", Status);

            }

            NtClose(FileHandle);
        }
        else
        {
            printf("Status 0x%lx\n", Status);

        }
    }

    wprintf(L"\n\n");
}

static
VOID
ListPartition(VOID)
{
    printf("List Partition!!\n");
}

static
VOID
ListVolume(VOID)
{
    PrintResourceString(IDS_LIST_VOLUME_HEAD);
}

static
VOID
ListVdisk(VOID)
{
    printf("List VDisk!!\n");
}

BOOL
list_main(
    INT argc,
    LPWSTR *argv)
{
    /* gets the first word from the string */
    if (argc == 1)
    {
        PrintResourceString(IDS_HELP_CMD_LIST);
        return TRUE;
    }

    /* determines which to list (disk, partition, etc.) */
    if (!wcsicmp(argv[1], L"disk"))
        ListDisk();
    else if (!wcsicmp(argv[1], L"partition"))
        ListPartition();
    else if (!wcsicmp(argv[1], L"volume"))
        ListVolume();
    else if (!wcsicmp(argv[1], L"vdisk"))
        ListVdisk();
    else
        PrintResourceString(IDS_HELP_CMD_LIST);

    return TRUE;
}
