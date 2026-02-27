/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/dump.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Eric Kohl
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
HexDump(
    _In_ UCHAR *addr,
    _In_ int len)
{
    WCHAR Buffer[17];
    UCHAR *pc;
    int i;

    Buffer[16] = L'\0';

    pc = addr;
    for (i = 0; i < len; i++)
    {
        if ((i % 16) == 0)
            ConPrintf(StdOut, L" %04x ", i);

        ConPrintf(StdOut, L" %02x", pc[i]);

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            Buffer[i % 16] = L'.';
        else
            Buffer[i % 16] = (WCHAR)(USHORT)pc[i];

        if ((i % 16) == (16 - 1))
            ConPrintf(StdOut, L"  %s\n", Buffer);
    }
}


EXIT_CODE
DumpDisk(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING Name;
    HANDLE FileHandle = NULL;
    PUCHAR pSectorBuffer = NULL;
    LARGE_INTEGER FileOffset;
    LONGLONG Sector;
    LPWSTR endptr = NULL;

#if 0
    if (argc == 2)
    {
        ConResPuts(StdOut, IDS_HELP_CMD_DUMP_DISK);
        return EXIT_SUCCESS;
    }
#endif

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    Sector = _wcstoi64(argv[2], &endptr, 0);
    if (((Sector == 0) && (endptr == argv[2])) ||
        (Sector < 0))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return EXIT_SUCCESS;
    }

    pSectorBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, CurrentDisk->BytesPerSector);
    if (pSectorBuffer == NULL)
    {
        DPRINT1("\n");
        /* Error message */
        goto done;
    }

    swprintf(Buffer,
             L"\\Device\\Harddisk%d\\Partition0",
             CurrentDisk->DiskNumber);
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
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("\n");
        goto done;
    }

    FileOffset.QuadPart = Sector * CurrentDisk->BytesPerSector;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &Iosb,
                        (PVOID)pSectorBuffer,
                        CurrentDisk->BytesPerSector,
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtReadFile failed, status=%x\n", Status);
        goto done;
    }

    HexDump(pSectorBuffer, CurrentDisk->BytesPerSector);

done:
    if (FileHandle)
        NtClose(FileHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, pSectorBuffer);

    return EXIT_SUCCESS;
}


EXIT_CODE
DumpPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING Name;
    HANDLE FileHandle = NULL;
    PUCHAR pSectorBuffer = NULL;
    LARGE_INTEGER FileOffset;
    LONGLONG Sector;
    LPWSTR endptr = NULL;


#if 0
    if (argc == 2)
    {
        ConResPuts(StdOut, IDS_HELP_CMD_DUMP_DISK);
        return EXIT_SUCCESS;
    }
#endif

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentPartition == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_PARTITION);
        return EXIT_SUCCESS;
    }

    Sector = _wcstoi64(argv[2], &endptr, 0);
    if (((Sector == 0) && (endptr == argv[2])) ||
        (Sector < 0))
    {
        ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
        return EXIT_SUCCESS;
    }

    pSectorBuffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, CurrentDisk->BytesPerSector);
    if (pSectorBuffer == NULL)
    {
        DPRINT1("\n");
        /* Error message */
        goto done;
    }

    swprintf(Buffer,
             L"\\Device\\Harddisk%d\\Partition%d",
             CurrentDisk->DiskNumber,
             CurrentPartition->PartitionNumber);
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
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("\n");
        goto done;
    }

    FileOffset.QuadPart = Sector * CurrentDisk->BytesPerSector;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &Iosb,
                        (PVOID)pSectorBuffer,
                        CurrentDisk->BytesPerSector,
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtReadFile failed, status=%x\n", Status);
        goto done;
    }

    HexDump(pSectorBuffer, CurrentDisk->BytesPerSector);

done:
    if (FileHandle)
        NtClose(FileHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, pSectorBuffer);

    return EXIT_SUCCESS;
}
