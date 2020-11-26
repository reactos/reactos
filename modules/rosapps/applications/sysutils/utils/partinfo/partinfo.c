/*
 * partinfo - partition info program
 */

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntndk.h>

// #define DUMP_DATA
#define DUMP_SIZE_INFO

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
    DWORD i;
    SYSTEM_DEVICE_INFORMATION DeviceInfo;
    DISK_GEOMETRY DiskGeometry;
    PDRIVE_LAYOUT_INFORMATION LayoutBuffer;
    CHAR DriveName[40];

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
        printf("NtQuerySystemInformation() failed (Status %lx)\n", Status);
        return 0;
    }
    if (DeviceInfo.NumberOfDisks == 0)
    {
        printf("No disk drive installed!\n");
        return 0;
    }

    if (ulDrive >= DeviceInfo.NumberOfDisks)
    {
        printf("Invalid disk drive number! Valid drive numbers [0-%lu]\n",
               DeviceInfo.NumberOfDisks-1);
        return 0;
    }

    /* Build the full drive name */
    sprintf(DriveName, "\\\\.\\PHYSICALDRIVE%lu", ulDrive);

    /* Open the drive */
    hDisk = CreateFileA(DriveName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    if (hDisk == INVALID_HANDLE_VALUE)
    {
        printf("Invalid disk handle!");
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
    printf("Drive number: %lu\n", ulDrive);
    printf("Cylinders: %I64u\nMediaType: %x\nTracksPerCylinder: %lu\n"
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
    LayoutBuffer = (PDRIVE_LAYOUT_INFORMATION)malloc(8192);
    if (LayoutBuffer == NULL)
    {
        printf("Out of memory!");
        CloseHandle(hDisk);
        return 0;
    }
    memset(LayoutBuffer, 0, 8192);

    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_DRIVE_LAYOUT,
                         NULL,
                         0,
                         LayoutBuffer,
                         8192,
                         &dwRead,
                         NULL))
    {
        printf("DeviceIoControl(IOCTL_DISK_GET_DRIVE_LAYOUT) failed! Error: %lu\n",
               GetLastError());
        CloseHandle(hDisk);
        free(LayoutBuffer);
        return 0;
    }

    CloseHandle(hDisk);

#ifdef DUMP_DATA
    HexDump(LayoutBuffer, dwRead);
#endif

    printf("Partitions %lu  Signature %lx\n",
           LayoutBuffer->PartitionCount,
           LayoutBuffer->Signature);

    for (i = 0; i < LayoutBuffer->PartitionCount; i++)
    {
        printf(" %ld: nr: %ld boot: %1x type: %x start: 0x%I64x count: 0x%I64x\n",
               i,
               LayoutBuffer->PartitionEntry[i].PartitionNumber,
               LayoutBuffer->PartitionEntry[i].BootIndicator,
               LayoutBuffer->PartitionEntry[i].PartitionType,
               LayoutBuffer->PartitionEntry[i].StartingOffset.QuadPart,
               LayoutBuffer->PartitionEntry[i].PartitionLength.QuadPart);
    }

    free(LayoutBuffer);

    // TODO: Retrieve the extended partition layout

    return 0;
}
