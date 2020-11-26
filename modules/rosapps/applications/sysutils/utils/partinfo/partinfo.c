/*
 * PROJECT:     ReactOS Partition Information Tool
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Displays disk and partition information for MBR and GPT disks.
 * COPYRIGHT:   Copyright 2001-2002 Eric Kohl
 *              Copyright 2020 Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntndk.h>

// #define DUMP_DATA
#define DUMP_SIZE_INFO


/* FORMATTING HELPERS *******************************************************/

static PCSTR PartitionStyleNames[] = {"MBR", "GPT", "RAW", "Unknown"};
#define PARTITION_STYLE_NAME(PartStyle) \
    ( ((PartStyle) <= PARTITION_STYLE_RAW)   \
          ? PartitionStyleNames[(PartStyle)] \
          : PartitionStyleNames[_countof(PartitionStyleNames)-1] )

#define GUID_FORMAT_STR "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUID_ELEMENTS(Guid) \
    (Guid)->Data1, (Guid)->Data2, (Guid)->Data3, \
    (Guid)->Data4[0], (Guid)->Data4[1], (Guid)->Data4[2], (Guid)->Data4[3], \
    (Guid)->Data4[4], (Guid)->Data4[5], (Guid)->Data4[6], (Guid)->Data4[7]


/* FUNCTIONS ****************************************************************/

#ifdef DUMP_DATA
void HexDump(
    IN PVOID buffer,
    IN ULONG size)
{
    ULONG_PTR offset = 0;
    PUCHAR ptr;

    while (offset < (size & ~15))
    {
        ptr = (PUCHAR)((ULONG_PTR)buffer + offset);
        printf("%08lx  %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx-%02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx\n",
               offset,
               ptr[0], ptr[1], ptr[2] , ptr[3] , ptr[4] , ptr[5] , ptr[6] , ptr[7],
               ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]);
      offset += 16;
    }

    ptr = (PUCHAR)((ULONG_PTR)buffer + offset);
    printf("%08lx ", offset);
    while (offset < size)
    {
        printf(" %02hx", *ptr);
        offset++;
        ptr++;
    }

    printf("\n\n\n");
}
#endif

void Usage(void)
{
    puts("Usage: partinfo <drive number>");
}

int main(int argc, char *argv[])
{
    NTSTATUS Status;
    ULONG ulDrive;
    HANDLE hDisk;
    DWORD dwRead;
    DWORD dwLastError;
    DWORD i;
    SYSTEM_DEVICE_INFORMATION DeviceInfo;
    DISK_GEOMETRY DiskGeometry;
    ULONG BufferSize;
    PDRIVE_LAYOUT_INFORMATION LayoutBuffer;
    PDRIVE_LAYOUT_INFORMATION_EX LayoutBufferEx;
    PVOID ptr;
    GUID Guid;
    WCHAR DriveName[40];

    if (argc != 2)
    {
        Usage();
        return 0;
    }

    ulDrive = strtoul(argv[1], NULL, 10);
    if (errno != 0)
    {
        printf("Error: Malformed drive number\n");
        return 0;
    }

    /*
     * Retrieve the number of disks on the system.
     */
    Status = NtQuerySystemInformation(SystemDeviceInformation,
                                      &DeviceInfo,
                                      sizeof(DeviceInfo),
                                      &i);
    if (!NT_SUCCESS(Status))
    {
        printf("NtQuerySystemInformation() failed (Status 0x%lx)\n", Status);
        return 0;
    }
    if (DeviceInfo.NumberOfDisks == 0)
    {
        printf("No disk drive installed!\n");
        return 0;
    }

    if (ulDrive >= DeviceInfo.NumberOfDisks)
    {
        printf("Invalid disk drive number! Valid drive numbers: [0-%lu]\n",
               DeviceInfo.NumberOfDisks-1);
        return 0;
    }

    /* Build the full drive name */
    swprintf(DriveName, L"\\\\.\\PHYSICALDRIVE%lu", ulDrive);

    /* Open the drive */
    hDisk = CreateFileW(DriveName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    if (hDisk == INVALID_HANDLE_VALUE)
    {
        printf("Could not open drive!");
        return 0;
    }


    /*
     * Get the drive geometry.
     */
    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_DRIVE_GEOMETRY,
                         NULL,
                         0,
                         &DiskGeometry,
                         sizeof(DiskGeometry),
                         &dwRead,
                         NULL))
    {
        printf("DeviceIoControl(IOCTL_DISK_GET_DRIVE_GEOMETRY) failed! Error: %lu\n",
               GetLastError());
        CloseHandle(hDisk);
        return 0;
    }

#ifdef DUMP_DATA
    HexDump(&DiskGeometry, dwRead);
#endif
    printf("Drive number: %lu\n\n", ulDrive);

    printf("IOCTL_DISK_GET_DRIVE_GEOMETRY\n"
           "Cylinders: %I64u\nMediaType: %x\nTracksPerCylinder: %lu\n"
           "SectorsPerTrack: %lu\nBytesPerSector: %lu\n\n",
           DiskGeometry.Cylinders.QuadPart,
           DiskGeometry.MediaType,
           DiskGeometry.TracksPerCylinder,
           DiskGeometry.SectorsPerTrack,
           DiskGeometry.BytesPerSector);

#if 0 // TODO!
    /* Get extended drive geometry */
    // IOCTL_DISK_GET_DRIVE_GEOMETRY_EX
#endif


    /*
     * Retrieve the legacy partition layout
     */

    /* Allocate a layout buffer with 4 partition entries first */
    BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                       ((4 - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION));
    LayoutBuffer = malloc(BufferSize);
    if (!LayoutBuffer)
    {
        printf("Out of memory!");
        CloseHandle(hDisk);
        return 0;
    }
    memset(LayoutBuffer, 0, BufferSize);

    /* Keep looping while the drive layout buffer is too small */
    for (;;)
    {
        if (DeviceIoControl(hDisk,
                            IOCTL_DISK_GET_DRIVE_LAYOUT,
                            NULL,
                            0,
                            LayoutBuffer,
                            BufferSize,
                            &dwRead,
                            NULL))
        {
            dwLastError = ERROR_SUCCESS;
            break;
        }

        dwLastError = GetLastError();
        if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
        {
            printf("DeviceIoControl(IOCTL_DISK_GET_DRIVE_LAYOUT) failed! Error: %lu\n",
                   dwLastError);

            /* Bail out if any other error than "invalid function" has been emitted.
             * This happens for example when calling it on GPT disks. */
            if (dwLastError != ERROR_INVALID_FUNCTION)
            {
                free(LayoutBuffer);
                CloseHandle(hDisk);
                return 0;
            }
            else
            {
                /* Just stop getting this information */
                break;
            }
        }

        /* Reallocate the buffer */
        BufferSize += 4 * sizeof(PARTITION_INFORMATION);
        ptr = realloc(LayoutBuffer, BufferSize);
        if (!ptr)
        {
            printf("Out of memory!");
            free(LayoutBuffer);
            CloseHandle(hDisk);
            return 0;
        }
        LayoutBuffer = ptr;
        memset(LayoutBuffer, 0, BufferSize);
    }

    if (dwLastError == ERROR_SUCCESS)
    {
#ifdef DUMP_DATA
        HexDump(LayoutBuffer, dwRead);
#endif

        printf("IOCTL_DISK_GET_DRIVE_LAYOUT\n"
               "Partitions: %lu  Signature: 0x%08lx\n",
               LayoutBuffer->PartitionCount,
               LayoutBuffer->Signature);

        for (i = 0; i < LayoutBuffer->PartitionCount; i++)
        {
            printf(" %ld: nr: %ld boot: %1x type: %x start: 0x%016I64x count: 0x%016I64x hidden: 0x%lx\n",
                   i,
                   LayoutBuffer->PartitionEntry[i].PartitionNumber,
                   LayoutBuffer->PartitionEntry[i].BootIndicator,
                   LayoutBuffer->PartitionEntry[i].PartitionType,
                   LayoutBuffer->PartitionEntry[i].StartingOffset.QuadPart,
                   LayoutBuffer->PartitionEntry[i].PartitionLength.QuadPart,
                   LayoutBuffer->PartitionEntry[i].HiddenSectors);
        }

        free(LayoutBuffer);
    }


    /*
     * Retrieve the extended partition layout
     */
    printf("\n");

    /* Allocate a layout buffer with 4 partition entries first */
    BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                       ((4 - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION_EX));
    LayoutBufferEx = malloc(BufferSize);
    if (!LayoutBufferEx)
    {
        printf("Out of memory!");
        CloseHandle(hDisk);
        return 0;
    }
    memset(LayoutBufferEx, 0, BufferSize);

    /* Keep looping while the drive layout buffer is too small */
    for (;;)
    {
        if (DeviceIoControl(hDisk,
                            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                            NULL,
                            0,
                            LayoutBufferEx,
                            BufferSize,
                            &dwRead,
                            NULL))
        {
            dwLastError = ERROR_SUCCESS;
            break;
        }

        dwLastError = GetLastError();
        if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
        {
            printf("DeviceIoControl(IOCTL_DISK_GET_DRIVE_LAYOUT_EX) failed! Error: %lu\n",
                   dwLastError);

            /* Bail out if any other error than "invalid function" has been emitted */
            if (dwLastError != ERROR_INVALID_FUNCTION)
            {
                free(LayoutBufferEx);
                CloseHandle(hDisk);
                return 0;
            }
            else
            {
                /* Just stop getting this information */
                break;
            }
        }

        /* Reallocate the buffer */
        BufferSize += 4 * sizeof(PARTITION_INFORMATION_EX);
        ptr = realloc(LayoutBufferEx, BufferSize);
        if (!ptr)
        {
            printf("Out of memory!");
            free(LayoutBufferEx);
            CloseHandle(hDisk);
            return 0;
        }
        LayoutBufferEx = ptr;
        memset(LayoutBufferEx, 0, BufferSize);
    }

    if (dwLastError == ERROR_SUCCESS)
    {
#ifdef DUMP_DATA
        HexDump(LayoutBufferEx, dwRead);
#endif

        printf("IOCTL_DISK_GET_DRIVE_LAYOUT_EX\n"
               "PartitionStyle: [%s]\n",
               PARTITION_STYLE_NAME(LayoutBufferEx->PartitionStyle));

        if (LayoutBufferEx->PartitionStyle == PARTITION_STYLE_MBR)
        {
            printf("Partitions: %lu  Signature: 0x%08lx",
                   LayoutBufferEx->PartitionCount,
                   LayoutBufferEx->Mbr.Signature);
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
            printf("  Checksum 0x%08lx\n",
                   LayoutBufferEx->Mbr.CheckSum);
#else
            printf("\n");
#endif

            for (i = 0; i < LayoutBufferEx->PartitionCount; i++)
            {
                printf(" %ld: nr: %ld [%s] boot: %1x type: %x start: 0x%016I64x count: 0x%016I64x hidden: 0x%lx\n",
                       i,
                       LayoutBufferEx->PartitionEntry[i].PartitionNumber,
                       PARTITION_STYLE_NAME(LayoutBufferEx->PartitionEntry[i].PartitionStyle),
                       LayoutBufferEx->PartitionEntry[i].Mbr.BootIndicator,
                       LayoutBufferEx->PartitionEntry[i].Mbr.PartitionType,
                       LayoutBufferEx->PartitionEntry[i].StartingOffset.QuadPart,
                       LayoutBufferEx->PartitionEntry[i].PartitionLength.QuadPart,
                       LayoutBufferEx->PartitionEntry[i].Mbr.HiddenSectors);
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
                Guid = LayoutBufferEx->PartitionEntry[i].Mbr.PartitionId;
                printf("    PartitionId: {" GUID_FORMAT_STR "}\n",
                       GUID_ELEMENTS(&Guid));
#endif
            }
        }
        else if (LayoutBufferEx->PartitionStyle == PARTITION_STYLE_GPT)
        {
            Guid = LayoutBufferEx->Gpt.DiskId;
            printf("Partitions: %lu  MaxPartitionCount: %lu\n"
                   "DiskId: {" GUID_FORMAT_STR "}\n"
                   "StartingUsableOffset: 0x%016I64x  UsableLength: 0x%016I64x\n",
                   LayoutBufferEx->PartitionCount,
                   LayoutBufferEx->Gpt.MaxPartitionCount,
                   GUID_ELEMENTS(&Guid),
                   LayoutBufferEx->Gpt.StartingUsableOffset.QuadPart,
                   LayoutBufferEx->Gpt.UsableLength.QuadPart);

            for (i = 0; i < LayoutBufferEx->PartitionCount; i++)
            {
                printf(" %ld: nr: %ld [%s]\n"
                       "    type : {" GUID_FORMAT_STR "}\n"
                       "    id   : {" GUID_FORMAT_STR "}\n"
                       "    attrs: 0x%016I64x\n"
                       "    name : '%.*S'\n"
                       "    start: 0x%016I64x count: 0x%016I64x\n",
                       i,
                       LayoutBufferEx->PartitionEntry[i].PartitionNumber,
                       PARTITION_STYLE_NAME(LayoutBufferEx->PartitionEntry[i].PartitionStyle),
                       GUID_ELEMENTS(&LayoutBufferEx->PartitionEntry[i].Gpt.PartitionType),
                       GUID_ELEMENTS(&LayoutBufferEx->PartitionEntry[i].Gpt.PartitionId),
                       LayoutBufferEx->PartitionEntry[i].Gpt.Attributes,
                       (int)_countof(LayoutBufferEx->PartitionEntry[i].Gpt.Name),
                       LayoutBufferEx->PartitionEntry[i].Gpt.Name,
                       LayoutBufferEx->PartitionEntry[i].StartingOffset.QuadPart,
                       LayoutBufferEx->PartitionEntry[i].PartitionLength.QuadPart);
            }
        }

        free(LayoutBufferEx);
    }

    CloseHandle(hDisk);

    return 0;
}
