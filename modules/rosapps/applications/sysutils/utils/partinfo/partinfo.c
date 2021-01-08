/*
 * PROJECT:     ReactOS Partition Information Tool
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Displays disk and partition information for MBR and GPT disks.
 * COPYRIGHT:   Copyright 2001-2002 Eric Kohl
 *              Copyright 2020-2021 Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define WIN32_NO_STATUS
#include <windows.h>
#include <ntndk.h>

/* The maximum information a DISK_GEOMETRY_EX dynamic structure can contain */
typedef struct _DISK_GEOMETRY_EX_INTERNAL
{
    DISK_GEOMETRY Geometry;
    LARGE_INTEGER DiskSize;
    DISK_PARTITION_INFO Partition;
    DISK_DETECTION_INFO Detection;
} DISK_GEOMETRY_EX_INTERNAL, *PDISK_GEOMETRY_EX_INTERNAL;

#define DRIVE_LAYOUT_INFO_ENTRY_SIZE \
    RTL_FIELD_SIZE(DRIVE_LAYOUT_INFORMATION, PartitionEntry[0])

#define DRIVE_LAYOUT_INFO_SIZE(n) \
    (FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry) + \
        ((n) * DRIVE_LAYOUT_INFO_ENTRY_SIZE))

#define DRIVE_LAYOUT_INFOEX_ENTRY_SIZE \
    RTL_FIELD_SIZE(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry[0])

#define DRIVE_LAYOUT_INFOEX_SIZE(n) \
    (FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) + \
        ((n) * DRIVE_LAYOUT_INFOEX_ENTRY_SIZE))


/* Rounding up for number; not the same as ROUND_UP() with power-of-two sizes */
#define ROUND_UP_NUM(num, up)   ((((num) + (up) - 1) / (up)) * (up))


// #define DUMP_DATA


/* FORMATTING HELPERS *******************************************************/

static PCSTR PartitionStyleNames[] = {"MBR", "GPT", "RAW", "Unknown"};
#define PARTITION_STYLE_NAME(PartStyle) \
    ( ((PartStyle) <= PARTITION_STYLE_RAW)   \
          ? PartitionStyleNames[(PartStyle)] \
          : PartitionStyleNames[_countof(PartitionStyleNames)-1] )

static PCSTR DetectTypeNames[] = { "None", "DetectInt13", "DetectExInt13", "Unknown" };
#define DETECT_TYPE_NAME(DetectType) \
    ( ((DetectType) <= DetectExInt13)   \
          ? DetectTypeNames[(DetectType)] \
          : DetectTypeNames[_countof(DetectTypeNames)-1] )

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

    if (offset < size)
    {
        ptr = (PUCHAR)((ULONG_PTR)buffer + offset);
        printf("%08lx ", offset);
        while (offset < size)
        {
            printf(" %02hx", *ptr);
            offset++;
            ptr++;
        }
        printf("\n");
    }

    printf("\n");
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
    ULONG i;
    SYSTEM_DEVICE_INFORMATION DeviceInfo;
    IO_STATUS_BLOCK Iosb;
    ULONG BufferSize;
    union
    {
        DISK_GEOMETRY Info;
        DISK_GEOMETRY_EX_INTERNAL InfoEx;
    } DiskGeometry;
    PDISK_GEOMETRY pDiskGeometry;
    union
    {
        PARTITION_INFORMATION Info;
        PARTITION_INFORMATION_EX InfoEx;
    } PartInfo;
    PPARTITION_INFORMATION pPartInfo;
    PPARTITION_INFORMATION_EX pPartInfoEx;
    union
    {
        PDRIVE_LAYOUT_INFORMATION Info;
        PDRIVE_LAYOUT_INFORMATION_EX InfoEx;
    } LayoutBuffer;
    PVOID ptr;
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

    printf("Drive number: %lu\n\n", ulDrive);


    /*
     * Get the drive geometry.
     */
    Status = NtDeviceIoControlFile(hDisk,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL,
                                   0,
                                   &DiskGeometry.Info,
                                   sizeof(DiskGeometry.Info));
    if (!NT_SUCCESS(Status))
    {
        printf("NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_GEOMETRY) failed! (Status 0x%lx)\n", Status);
    }
    else
    {
        pDiskGeometry = &DiskGeometry.Info;

#ifdef DUMP_DATA
        HexDump(&DiskGeometry.Info, (ULONG)Iosb.Information);
#endif
        printf("IOCTL_DISK_GET_DRIVE_GEOMETRY\n"
               "Cylinders: %I64u\nMediaType: 0x%x\nTracksPerCylinder: %lu\n"
               "SectorsPerTrack: %lu\nBytesPerSector: %lu\n",
               pDiskGeometry->Cylinders.QuadPart,
               pDiskGeometry->MediaType,
               pDiskGeometry->TracksPerCylinder,
               pDiskGeometry->SectorsPerTrack,
               pDiskGeometry->BytesPerSector);
    }


    /*
     * Get the extended drive geometry.
     */
    printf("\n");
    Status = NtDeviceIoControlFile(hDisk,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                   NULL,
                                   0,
                                   &DiskGeometry.InfoEx,
                                   sizeof(DiskGeometry.InfoEx));
    if (!NT_SUCCESS(Status))
    {
        printf("NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX) failed! (Status 0x%lx)\n", Status);
    }
    else
    {
        // PDISK_DETECTION_INFO DiskDetectInfo = &DiskGeometry.InfoEx.Detection; // DiskGeometryGetDetect(&DiskGeometry.InfoEx);
        // PDISK_PARTITION_INFO DiskPartInfo = &DiskGeometry.InfoEx.Partition; // DiskGeometryGetPartition(&DiskGeometry.InfoEx);
        pDiskGeometry = &DiskGeometry.InfoEx.Geometry;

#ifdef DUMP_DATA
        HexDump(&DiskGeometry.InfoEx, (ULONG)Iosb.Information);
#endif
        printf("IOCTL_DISK_GET_DRIVE_GEOMETRY_EX\n"
               "Cylinders: %I64u\nMediaType: 0x%x\nTracksPerCylinder: %lu\n"
               "SectorsPerTrack: %lu\nBytesPerSector: %lu\n"
               "DiskSize: %I64u\n",
               pDiskGeometry->Cylinders.QuadPart,
               pDiskGeometry->MediaType,
               pDiskGeometry->TracksPerCylinder,
               pDiskGeometry->SectorsPerTrack,
               pDiskGeometry->BytesPerSector,
               DiskGeometry.InfoEx.DiskSize.QuadPart);

        printf("SizeOfDetectInfo: %lu  DetectionType: %u (%s)\n",
               DiskGeometry.InfoEx.Detection.SizeOfDetectInfo,
               DiskGeometry.InfoEx.Detection.DetectionType,
               DETECT_TYPE_NAME(DiskGeometry.InfoEx.Detection.DetectionType));
        switch (DiskGeometry.InfoEx.Detection.DetectionType)
        {
        case DetectInt13:
        {
            PDISK_INT13_INFO pInt13 = &DiskGeometry.InfoEx.Detection.Int13;
            printf("  DriveSelect: %u  MaxCylinders: %lu  SectorsPerTrack: %u  MaxHeads: %u  NumberDrives: %u\n",
                   pInt13->DriveSelect, pInt13->MaxCylinders,
                   pInt13->SectorsPerTrack, pInt13->MaxHeads,
                   pInt13->NumberDrives);
            break;
        }

        case DetectExInt13:
        {
            PDISK_EX_INT13_INFO pExInt13 = &DiskGeometry.InfoEx.Detection.ExInt13;
            printf("  ExBufferSize: %u  ExFlags: %u\n"
                   "  ExCylinders: %lu  ExHeads: %lu  ExSectorsPerTrack: %lu\n"
                   "  ExSectorsPerDrive: %I64u  ExSectorSize: %u  ExReserved: %u\n",
                   pExInt13->ExBufferSize, pExInt13->ExFlags,
                   pExInt13->ExCylinders,  pExInt13->ExHeads,
                   pExInt13->ExSectorsPerTrack, pExInt13->ExSectorsPerDrive,
                   pExInt13->ExSectorSize, pExInt13->ExReserved);
            break;
        }

        case DetectNone:
        default:
            break;
        }

        printf("SizeOfPartitionInfo: %lu  PartitionStyle: %u [%s]\n",
               DiskGeometry.InfoEx.Partition.SizeOfPartitionInfo,
               DiskGeometry.InfoEx.Partition.PartitionStyle,
               PARTITION_STYLE_NAME(DiskGeometry.InfoEx.Partition.PartitionStyle));

        if (DiskGeometry.InfoEx.Partition.PartitionStyle == PARTITION_STYLE_MBR)
        {
            printf("  Signature: 0x%08lx  Checksum 0x%08lx\n",
                   DiskGeometry.InfoEx.Partition.Mbr.Signature,
                   DiskGeometry.InfoEx.Partition.Mbr.CheckSum);
        }
        else if (DiskGeometry.InfoEx.Partition.PartitionStyle == PARTITION_STYLE_GPT)
        {
            printf("  DiskId: {" GUID_FORMAT_STR "}\n",
                   GUID_ELEMENTS(&DiskGeometry.InfoEx.Partition.Gpt.DiskId));
        }
        else
        {
            /* Unknown */
            printf("\n");
        }
    }


    /*
     * Display partition 0 (i.e. the whole disk) information.
     */
    printf("\n");
    Status = NtDeviceIoControlFile(hDisk,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_GET_PARTITION_INFO,
                                   NULL,
                                   0,
                                   &PartInfo.Info,
                                   sizeof(PartInfo.Info));
    if (!NT_SUCCESS(Status))
    {
        printf("NtDeviceIoControlFile(IOCTL_DISK_GET_PARTITION_INFO) failed! (Status 0x%lx)\n", Status);
    }
    else
    {
        pPartInfo = &PartInfo.Info;

#ifdef DUMP_DATA
        HexDump(&PartInfo.Info, (ULONG)Iosb.Information);
#endif
        printf("IOCTL_DISK_GET_PARTITION_INFO\n"
               "nr: %ld boot: %1x type: 0x%x (%s) start: 0x%016I64x count: 0x%016I64x hidden: 0x%lx\n",
               pPartInfo->PartitionNumber,
               pPartInfo->BootIndicator,
               pPartInfo->PartitionType,
               pPartInfo->RecognizedPartition ? "Recognized" : "Not recognized",
               pPartInfo->StartingOffset.QuadPart,
               pPartInfo->PartitionLength.QuadPart,
               pPartInfo->HiddenSectors);
    }

    printf("\n");
    Status = NtDeviceIoControlFile(hDisk,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_GET_PARTITION_INFO_EX,
                                   NULL,
                                   0,
                                   &PartInfo.InfoEx,
                                   sizeof(PartInfo.InfoEx));
    if (!NT_SUCCESS(Status))
    {
        printf("NtDeviceIoControlFile(IOCTL_DISK_GET_PARTITION_INFO_EX) failed! (Status 0x%lx)\n", Status);
    }
    else
    {
        pPartInfoEx = &PartInfo.InfoEx;

#ifdef DUMP_DATA
        HexDump(&PartInfo.InfoEx, (ULONG)Iosb.Information);
#endif
        printf("IOCTL_DISK_GET_PARTITION_INFO_EX\n");

        if (pPartInfoEx->PartitionStyle == PARTITION_STYLE_MBR)
        {
            printf("nr: %ld [%s] boot: %1x type: 0x%x (%s) start: 0x%016I64x count: 0x%016I64x hidden: 0x%lx\n",
                   pPartInfoEx->PartitionNumber,
                   PARTITION_STYLE_NAME(pPartInfoEx->PartitionStyle),
                   pPartInfoEx->Mbr.BootIndicator,
                   pPartInfoEx->Mbr.PartitionType,
                   pPartInfoEx->Mbr.RecognizedPartition ? "Recognized" : "Not recognized",
                   pPartInfoEx->StartingOffset.QuadPart,
                   pPartInfoEx->PartitionLength.QuadPart,
                   pPartInfoEx->Mbr.HiddenSectors);
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
            printf("    PartitionId: {" GUID_FORMAT_STR "}\n",
                   GUID_ELEMENTS(&pPartInfoEx->Mbr.PartitionId));
#endif
        }
        else if (pPartInfoEx->PartitionStyle == PARTITION_STYLE_GPT)
        {
            printf("nr: %ld [%s]\n"
                   "    type : {" GUID_FORMAT_STR "}\n"
                   "    id   : {" GUID_FORMAT_STR "}\n"
                   "    attrs: 0x%016I64x\n"
                   "    name : '%.*S'\n"
                   "    start: 0x%016I64x count: 0x%016I64x\n",
                   pPartInfoEx->PartitionNumber,
                   PARTITION_STYLE_NAME(pPartInfoEx->PartitionStyle),
                   GUID_ELEMENTS(&pPartInfoEx->Gpt.PartitionType),
                   GUID_ELEMENTS(&pPartInfoEx->Gpt.PartitionId),
                   pPartInfoEx->Gpt.Attributes,
                   (int)_countof(pPartInfoEx->Gpt.Name),
                   pPartInfoEx->Gpt.Name,
                   pPartInfoEx->StartingOffset.QuadPart,
                   pPartInfoEx->PartitionLength.QuadPart);
        }
    }


    /*
     * Retrieve the legacy partition layout
     */
    printf("\n");

    /* Allocate a layout buffer with initially 4 partition entries (or 16 for NEC PC-98) */
    BufferSize = DRIVE_LAYOUT_INFO_SIZE(IsNEC_98 ? 16 : 4);
    LayoutBuffer.Info = RtlAllocateHeap(RtlGetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        BufferSize);
    if (!LayoutBuffer.Info)
    {
        printf("Out of memory!");
        goto Quit;
    }

    /*
     * Keep looping while the drive layout buffer is too small.
     * Iosb.Information or PartitionCount only contain actual info only
     * once NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_LAYOUT) succeeds.
     */
    for (;;)
    {
        Status = NtDeviceIoControlFile(hDisk,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_DISK_GET_DRIVE_LAYOUT,
                                       NULL,
                                       0,
                                       LayoutBuffer.Info,
                                       BufferSize);
        if (NT_SUCCESS(Status))
        {
            /* We succeeded; compactify the layout structure but keep the
             * number of partition entries rounded up to a multiple of 4. */
#if 1
            BufferSize = DRIVE_LAYOUT_INFO_SIZE(ROUND_UP_NUM(LayoutBuffer.Info->PartitionCount, 4));
#else
            BufferSize = (ULONG)Iosb.Information;
#endif
            ptr = RtlReAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_REALLOC_IN_PLACE_ONLY,
                                    LayoutBuffer.Info, BufferSize);
            if (!ptr)
            {
                printf("Compactification failed; keeping original structure.\n");
            }
            else
            {
                LayoutBuffer.Info = ptr;
            }
            Status = STATUS_SUCCESS;
            break;
        }

        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            printf("NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_LAYOUT) failed! (Status 0x%lx)\n", Status);

            /* Bail out if any other error than "invalid function" has been emitted.
             * This happens for example when calling it on GPT disks. */
            if (Status != STATUS_INVALID_DEVICE_REQUEST)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, LayoutBuffer.Info);
                goto Quit;
            }
            else
            {
                /* Just stop getting this information */
                break;
            }
        }

        /* Reallocate the buffer by chunks of 4 entries */
        BufferSize += 4 * DRIVE_LAYOUT_INFO_ENTRY_SIZE;
        ptr = RtlReAllocateHeap(RtlGetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                LayoutBuffer.Info, BufferSize);
        if (!ptr)
        {
            printf("Out of memory!");
            RtlFreeHeap(RtlGetProcessHeap(), 0, LayoutBuffer.Info);
            goto Quit;
        }
        LayoutBuffer.Info = ptr;
    }

    if (Status == STATUS_SUCCESS)
    {
#ifdef DUMP_DATA
        HexDump(LayoutBuffer.Info, (ULONG)Iosb.Information);
#endif
        printf("IOCTL_DISK_GET_DRIVE_LAYOUT\n"
               "Partitions: %lu  Signature: 0x%08lx\n",
               LayoutBuffer.Info->PartitionCount,
               LayoutBuffer.Info->Signature);

        for (i = 0; i < LayoutBuffer.Info->PartitionCount; i++)
        {
            pPartInfo = &LayoutBuffer.Info->PartitionEntry[i];

            printf(" %ld: nr: %ld boot: %1x type: 0x%x (%s) start: 0x%016I64x count: 0x%016I64x hidden: 0x%lx\n",
                   i,
                   pPartInfo->PartitionNumber,
                   pPartInfo->BootIndicator,
                   pPartInfo->PartitionType,
                   pPartInfo->RecognizedPartition ? "Recognized" : "Not recognized",
                   pPartInfo->StartingOffset.QuadPart,
                   pPartInfo->PartitionLength.QuadPart,
                   pPartInfo->HiddenSectors);
        }
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, LayoutBuffer.Info);


    /*
     * Retrieve the extended partition layout
     */
    printf("\n");

    /* Allocate a layout buffer with initially 4 partition entries (or 16 for NEC PC-98) */
    BufferSize = DRIVE_LAYOUT_INFOEX_SIZE(IsNEC_98 ? 16 : 4);
    LayoutBuffer.InfoEx = RtlAllocateHeap(RtlGetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          BufferSize);
    if (!LayoutBuffer.InfoEx)
    {
        printf("Out of memory!");
        goto Quit;
    }

    /*
     * Keep looping while the drive layout buffer is too small.
     * Iosb.Information or PartitionCount only contain actual info only
     * once NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_LAYOUT_EX) succeeds.
     */
    for (;;)
    {
        Status = NtDeviceIoControlFile(hDisk,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                       NULL,
                                       0,
                                       LayoutBuffer.InfoEx,
                                       BufferSize);
        if (NT_SUCCESS(Status))
        {
            /* We succeeded; compactify the layout structure but keep the
             * number of partition entries rounded up to a multiple of 4. */
#if 1
            BufferSize = DRIVE_LAYOUT_INFOEX_SIZE(ROUND_UP_NUM(LayoutBuffer.InfoEx->PartitionCount, 4));
#else
            BufferSize = (ULONG)Iosb.Information;
#endif
            ptr = RtlReAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_REALLOC_IN_PLACE_ONLY,
                                    LayoutBuffer.InfoEx, BufferSize);
            if (!ptr)
            {
                printf("Compactification failed; keeping original structure.\n");
            }
            else
            {
                LayoutBuffer.InfoEx = ptr;
            }
            Status = STATUS_SUCCESS;
            break;
        }

        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            printf("NtDeviceIoControlFile(IOCTL_DISK_GET_DRIVE_LAYOUT_EX) failed! (Status 0x%lx)\n", Status);

            /* Bail out if any other error than "invalid function" has been emitted */
            if (Status != STATUS_INVALID_DEVICE_REQUEST)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, LayoutBuffer.InfoEx);
                goto Quit;
            }
            else
            {
                /* Just stop getting this information */
                break;
            }
        }

        /* Reallocate the buffer by chunks of 4 entries */
        BufferSize += 4 * DRIVE_LAYOUT_INFOEX_ENTRY_SIZE;
        ptr = RtlReAllocateHeap(RtlGetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                LayoutBuffer.InfoEx, BufferSize);
        if (!ptr)
        {
            printf("Out of memory!");
            RtlFreeHeap(RtlGetProcessHeap(), 0, LayoutBuffer.InfoEx);
            goto Quit;
        }
        LayoutBuffer.InfoEx = ptr;
    }

    if (Status == STATUS_SUCCESS)
    {
#ifdef DUMP_DATA
        HexDump(LayoutBuffer.InfoEx, (ULONG)Iosb.Information);
#endif
        printf("IOCTL_DISK_GET_DRIVE_LAYOUT_EX\n"
               "PartitionStyle: %lu [%s]\n",
               LayoutBuffer.InfoEx->PartitionStyle,
               PARTITION_STYLE_NAME(LayoutBuffer.InfoEx->PartitionStyle));

        if (LayoutBuffer.InfoEx->PartitionStyle == PARTITION_STYLE_MBR)
        {
            printf("Partitions: %lu  Signature: 0x%08lx",
                   LayoutBuffer.InfoEx->PartitionCount,
                   LayoutBuffer.InfoEx->Mbr.Signature);
#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)
            printf("  Checksum 0x%08lx\n",
                   LayoutBuffer.InfoEx->Mbr.CheckSum);
#else
            printf("\n");
#endif

            for (i = 0; i < LayoutBuffer.InfoEx->PartitionCount; i++)
            {
                pPartInfoEx = &LayoutBuffer.InfoEx->PartitionEntry[i];

                printf(" %ld: nr: %ld [%s] boot: %1x type: 0x%x (%s) start: 0x%016I64x count: 0x%016I64x hidden: 0x%lx\n",
                       i,
                       pPartInfoEx->PartitionNumber,
                       PARTITION_STYLE_NAME(pPartInfoEx->PartitionStyle),
                       pPartInfoEx->Mbr.BootIndicator,
                       pPartInfoEx->Mbr.PartitionType,
                       pPartInfoEx->Mbr.RecognizedPartition ? "Recognized" : "Not recognized",
                       pPartInfoEx->StartingOffset.QuadPart,
                       pPartInfoEx->PartitionLength.QuadPart,
                       pPartInfoEx->Mbr.HiddenSectors);
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
                printf("    PartitionId: {" GUID_FORMAT_STR "}\n",
                       GUID_ELEMENTS(&pPartInfoEx->Mbr.PartitionId));
#endif
            }
        }
        else if (LayoutBuffer.InfoEx->PartitionStyle == PARTITION_STYLE_GPT)
        {
            printf("Partitions: %lu  MaxPartitionCount: %lu\n"
                   "DiskId: {" GUID_FORMAT_STR "}\n"
                   "StartingUsableOffset: 0x%016I64x  UsableLength: 0x%016I64x\n",
                   LayoutBuffer.InfoEx->PartitionCount,
                   LayoutBuffer.InfoEx->Gpt.MaxPartitionCount,
                   GUID_ELEMENTS(&LayoutBuffer.InfoEx->Gpt.DiskId),
                   LayoutBuffer.InfoEx->Gpt.StartingUsableOffset.QuadPart,
                   LayoutBuffer.InfoEx->Gpt.UsableLength.QuadPart);

            for (i = 0; i < LayoutBuffer.InfoEx->PartitionCount; i++)
            {
                pPartInfoEx = &LayoutBuffer.InfoEx->PartitionEntry[i];

                printf(" %ld: nr: %ld [%s]\n"
                       "    type : {" GUID_FORMAT_STR "}\n"
                       "    id   : {" GUID_FORMAT_STR "}\n"
                       "    attrs: 0x%016I64x\n"
                       "    name : '%.*S'\n"
                       "    start: 0x%016I64x count: 0x%016I64x\n",
                       i,
                       pPartInfoEx->PartitionNumber,
                       PARTITION_STYLE_NAME(pPartInfoEx->PartitionStyle),
                       GUID_ELEMENTS(&pPartInfoEx->Gpt.PartitionType),
                       GUID_ELEMENTS(&pPartInfoEx->Gpt.PartitionId),
                       pPartInfoEx->Gpt.Attributes,
                       (int)_countof(pPartInfoEx->Gpt.Name),
                       pPartInfoEx->Gpt.Name,
                       pPartInfoEx->StartingOffset.QuadPart,
                       pPartInfoEx->PartitionLength.QuadPart);
            }
        }
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, LayoutBuffer.InfoEx);

Quit:
    CloseHandle(hDisk);
    return 0;
}
