/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/partlist.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include "diskpart.h"

#define NDEBUG
#include <debug.h>

#define InsertAscendingList(ListHead, NewEntry, Type, ListEntryField, SortField)\
{\
  PLIST_ENTRY current;\
\
  current = (ListHead)->Flink;\
  while (current != (ListHead))\
  {\
    if (CONTAINING_RECORD(current, Type, ListEntryField)->SortField >=\
        (NewEntry)->SortField)\
    {\
      break;\
    }\
    current = current->Flink;\
  }\
\
  InsertTailList(current, &((NewEntry)->ListEntryField));\
}

/* We have to define it there, because it is not in the MS DDK */
#define PARTITION_LINUX 0x83

#define PARTITION_TBL_SIZE 4

#define MBR_MAGIC 0xAA55

#include <pshpack1.h>

typedef struct _PARTITION
{
    unsigned char   BootFlags;        /* bootable?  0=no, 128=yes  */
    unsigned char   StartingHead;     /* beginning head number */
    unsigned char   StartingSector;   /* beginning sector number */
    unsigned char   StartingCylinder; /* 10 bit nmbr, with high 2 bits put in begsect */
    unsigned char   PartitionType;    /* Operating System type indicator code */
    unsigned char   EndingHead;       /* ending head number */
    unsigned char   EndingSector;     /* ending sector number */
    unsigned char   EndingCylinder;   /* also a 10 bit nmbr, with same high 2 bit trick */
    unsigned int  StartingBlock;      /* first sector relative to start of disk */
    unsigned int  SectorCount;        /* number of sectors in partition */
} PARTITION, *PPARTITION;

typedef struct _PARTITION_SECTOR
{
    UCHAR BootCode[440];                     /* 0x000 */
    ULONG Signature;                         /* 0x1B8 */
    UCHAR Reserved[2];                       /* 0x1BC */
    PARTITION Partition[PARTITION_TBL_SIZE]; /* 0x1BE */
    USHORT Magic;                            /* 0x1FE */
} PARTITION_SECTOR, *PPARTITION_SECTOR;

#include <poppack.h>


/* GLOBALS ********************************************************************/

LIST_ENTRY DiskListHead;
LIST_ENTRY BiosDiskListHead;
LIST_ENTRY VolumeListHead;

PDISKENTRY CurrentDisk = NULL;
PPARTENTRY CurrentPartition = NULL;
PVOLENTRY  CurrentVolume = NULL;


/* FUNCTIONS ******************************************************************/

#ifdef DUMP_PARTITION_TABLE
VOID
DumpPartitionTable(
    _In_ PDISKENTRY DiskEntry)
{
    PPARTITION_INFORMATION_EX PartitionInfo;
    ULONG i;

    DbgPrint("\n");

    if (DiskEntry->LayoutBuffer->PartitionStyle == PARTITION_STYLE_MBR)
    {
        DbgPrint("Index  Start         Length        Hidden      Nr  Type  Boot  RW\n");
        DbgPrint("-----  ------------  ------------  ----------  --  ----  ----  --\n");

        for (i = 0; i < DiskEntry->LayoutBuffer->PartitionCount; i++)
        {
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[i];
            DbgPrint("  %3lu  %12I64u  %12I64u  %10lu  %2lu    %2x     %c   %c\n",
                     i,
                     PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector,
                     PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector,
                     PartitionInfo->Mbr.HiddenSectors,
                     PartitionInfo->PartitionNumber,
                     PartitionInfo->Mbr.PartitionType,
                     PartitionInfo->Mbr.BootIndicator ? '*': ' ',
                     PartitionInfo->RewritePartition ? 'Y': 'N');
        }
    }
    else if (DiskEntry->LayoutBuffer->PartitionStyle == PARTITION_STYLE_GPT)
    {
        DbgPrint("Index  Start         Length        Nr  RW  Type    \n");
        DbgPrint("-----  ------------  ------------  --  --  --------\n");
        for (i = 0; i < DiskEntry->LayoutBuffer->PartitionCount; i++)
        {
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[i];

            DbgPrint("  %3lu  %12I64u  %12I64u  %2lu    %c  %08lx\n",
                     i,
                     PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector,
                     PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector,
                     PartitionInfo->PartitionNumber,
                     PartitionInfo->RewritePartition ? 'Y': 'N',
                     PartitionInfo->Gpt.PartitionType.Data1);
        }
    }

    DbgPrint("\n");
}
#endif

#ifdef DUMP_PARTITION_LIST
VOID
DumpPartitionList(
    _In_ PDISKENTRY DiskEntry)
{
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;

    DbgPrint("\n");

    if (DiskEntry->PartitionStyle == PARTITION_STYLE_GPT)
    {
        DbgPrint("Disk Start    Disk End      Disk Length \n");
        DbgPrint("------------  ------------  ------------\n");
        DbgPrint("%12I64u  %12I64u  %12I64u\n",
                 DiskEntry->StartSector.QuadPart,
                 DiskEntry->EndSector.QuadPart,
                 DiskEntry->EndSector.QuadPart - DiskEntry->StartSector.QuadPart + 1);

        DbgPrint("\n");

        DbgPrint("Start         End           Length        Nr   Type      P\n");
        DbgPrint("------------  ------------  ------------  ---  --------  --\n");

        for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
             ListEntry != &DiskEntry->PrimaryPartListHead;
             ListEntry = ListEntry->Flink)
        {
            PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

            DbgPrint("%12I64u  %12I64u  %12I64u  %3lu  %08lx  %c\n",
                     PartEntry->StartSector.QuadPart,
                     PartEntry->StartSector.QuadPart + PartEntry->SectorCount.QuadPart - 1,
                     PartEntry->SectorCount.QuadPart,
                     PartEntry->PartitionNumber,
                     PartEntry->Gpt.PartitionType.Data1,
                     PartEntry->IsPartitioned ? 'Y': 'N');
        }
    }
    else if (DiskEntry->PartitionStyle == PARTITION_STYLE_MBR)
    {
        DbgPrint("Disk Start    Disk End      Disk Length \n");
        DbgPrint("------------  ------------  ------------\n");
        DbgPrint("%12I64u  %12I64u  %12I64u\n",
                 DiskEntry->StartSector.QuadPart,
                 DiskEntry->EndSector.QuadPart,
                 DiskEntry->EndSector.QuadPart - DiskEntry->StartSector.QuadPart + 1);

    }

    DbgPrint("\n");
}
#endif


ULONGLONG
AlignDown(
    _In_ ULONGLONG Value,
    _In_ ULONG Alignment)
{
    ULONGLONG Temp;

    Temp = Value / Alignment;

    return Temp * Alignment;
}

static
VOID
GetDriverName(
    PDISKENTRY DiskEntry)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    WCHAR KeyName[32];
    NTSTATUS Status;

    RtlInitUnicodeString(&DiskEntry->DriverName,
                         NULL);

    StringCchPrintfW(KeyName, ARRAYSIZE(KeyName),
                     L"\\Scsi\\Scsi Port %lu",
                     DiskEntry->Port);

    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].Name = L"Driver";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].EntryContext = &DiskEntry->DriverName;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                                    KeyName,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
    }
}

static
NTSTATUS
NTAPI
DiskIdentifierQueryRoutine(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext)
{
    PBIOSDISKENTRY BiosDiskEntry = (PBIOSDISKENTRY)Context;
    UNICODE_STRING NameU;

    if (ValueType == REG_SZ &&
        ValueLength == 20 * sizeof(WCHAR))
    {
        NameU.Buffer = (PWCHAR)ValueData;
        NameU.Length = NameU.MaximumLength = 8 * sizeof(WCHAR);
        RtlUnicodeStringToInteger(&NameU, 16, &BiosDiskEntry->Checksum);

        NameU.Buffer = (PWCHAR)ValueData + 9;
        RtlUnicodeStringToInteger(&NameU, 16, &BiosDiskEntry->Signature);

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
DiskConfigurationDataQueryRoutine(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext)
{
    PBIOSDISKENTRY BiosDiskEntry = (PBIOSDISKENTRY)Context;
    PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
    PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    ULONG i;

    if (ValueType != REG_FULL_RESOURCE_DESCRIPTOR ||
        ValueLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
        return STATUS_UNSUCCESSFUL;

    FullResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)ValueData;

    /* Hm. Version and Revision are not set on Microsoft Windows XP... */
#if 0
    if (FullResourceDescriptor->PartialResourceList.Version != 1 ||
        FullResourceDescriptor->PartialResourceList.Revision != 1)
        return STATUS_UNSUCCESSFUL;
#endif

    for (i = 0; i < FullResourceDescriptor->PartialResourceList.Count; i++)
    {
        if (FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].Type != CmResourceTypeDeviceSpecific ||
            FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize != sizeof(CM_DISK_GEOMETRY_DEVICE_DATA))
            continue;

        DiskGeometry = (PCM_DISK_GEOMETRY_DEVICE_DATA)&FullResourceDescriptor->PartialResourceList.PartialDescriptors[i + 1];
        BiosDiskEntry->DiskGeometry = *DiskGeometry;

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
SystemConfigurationDataQueryRoutine(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext)
{
    PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor;
    PCM_INT13_DRIVE_PARAMETER* Int13Drives = (PCM_INT13_DRIVE_PARAMETER*)Context;
    ULONG i;

    if (ValueType != REG_FULL_RESOURCE_DESCRIPTOR ||
        ValueLength < sizeof (CM_FULL_RESOURCE_DESCRIPTOR))
        return STATUS_UNSUCCESSFUL;

    FullResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)ValueData;

    /* Hm. Version and Revision are not set on Microsoft Windows XP... */
#if 0
    if (FullResourceDescriptor->PartialResourceList.Version != 1 ||
        FullResourceDescriptor->PartialResourceList.Revision != 1)
        return STATUS_UNSUCCESSFUL;
#endif

    for (i = 0; i < FullResourceDescriptor->PartialResourceList.Count; i++)
    {
        if (FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].Type != CmResourceTypeDeviceSpecific ||
            FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize % sizeof(CM_INT13_DRIVE_PARAMETER) != 0)
            continue;

        *Int13Drives = (CM_INT13_DRIVE_PARAMETER*)RtlAllocateHeap(RtlGetProcessHeap(), 0,
                       FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize);
        if (*Int13Drives == NULL)
            return STATUS_NO_MEMORY;

        memcpy(*Int13Drives,
               &FullResourceDescriptor->PartialResourceList.PartialDescriptors[i + 1],
               FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}


#define ROOT_NAME   L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter"

static
VOID
EnumerateBiosDiskEntries(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    WCHAR Name[120];
    ULONG AdapterCount;
    ULONG DiskCount;
    NTSTATUS Status;
    PCM_INT13_DRIVE_PARAMETER Int13Drives;
    PBIOSDISKENTRY BiosDiskEntry;

    memset(QueryTable, 0, sizeof(QueryTable));

    QueryTable[1].Name = L"Configuration Data";
    QueryTable[1].QueryRoutine = SystemConfigurationDataQueryRoutine;
    Int13Drives = NULL;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
                                    &QueryTable[1],
                                    (PVOID)&Int13Drives,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to query the 'Configuration Data' key in '\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System', status=%lx\n", Status);
        return;
    }

    AdapterCount = 0;
    while (1)
    {
        StringCchPrintfW(Name, ARRAYSIZE(Name),
                         L"%s\\%lu", ROOT_NAME, AdapterCount);
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        Name,
                                        &QueryTable[2],
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        StringCchPrintfW(Name, ARRAYSIZE(Name),
                         L"%s\\%lu\\DiskController", ROOT_NAME, AdapterCount);
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        Name,
                                        &QueryTable[2],
                                        NULL,
                                        NULL);
        if (NT_SUCCESS(Status))
        {
            while (1)
            {
                StringCchPrintfW(Name, ARRAYSIZE(Name),
                                 L"%s\\%lu\\DiskController\\0", ROOT_NAME, AdapterCount);
                Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                Name,
                                                &QueryTable[2],
                                                NULL,
                                                NULL);
                if (!NT_SUCCESS(Status))
                {
                    RtlFreeHeap(RtlGetProcessHeap(), 0, Int13Drives);
                    return;
                }

                StringCchPrintfW(Name, ARRAYSIZE(Name),
                                 L"%s\\%lu\\DiskController\\0\\DiskPeripheral", ROOT_NAME, AdapterCount);
                Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                Name,
                                                &QueryTable[2],
                                                NULL,
                                                NULL);
                if (NT_SUCCESS(Status))
                {
                    QueryTable[0].Name = L"Identifier";
                    QueryTable[0].QueryRoutine = DiskIdentifierQueryRoutine;
                    QueryTable[1].Name = L"Configuration Data";
                    QueryTable[1].QueryRoutine = DiskConfigurationDataQueryRoutine;

                    DiskCount = 0;
                    while (1)
                    {
                        BiosDiskEntry = (BIOSDISKENTRY*)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BIOSDISKENTRY));
                        if (BiosDiskEntry == NULL)
                        {
                            break;
                        }

                        StringCchPrintfW(Name, ARRAYSIZE(Name),
                                         L"%s\\%lu\\DiskController\\0\\DiskPeripheral\\%lu", ROOT_NAME, AdapterCount, DiskCount);
                        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                        Name,
                                                        QueryTable,
                                                        (PVOID)BiosDiskEntry,
                                                        NULL);
                        if (!NT_SUCCESS(Status))
                        {
                            RtlFreeHeap(RtlGetProcessHeap(), 0, BiosDiskEntry);
                            break;
                        }

                        BiosDiskEntry->DiskNumber = DiskCount;
                        BiosDiskEntry->Recognized = FALSE;

                        if (DiskCount < Int13Drives[0].NumberDrives)
                        {
                            BiosDiskEntry->Int13DiskData = Int13Drives[DiskCount];
                        }
                        else
                        {
                            DPRINT1("Didn't find int13 drive datas for disk %u\n", DiskCount);
                        }

                        InsertTailList(&BiosDiskListHead, &BiosDiskEntry->ListEntry);

                        DPRINT("DiskNumber:        %lu\n", BiosDiskEntry->DiskNumber);
                        DPRINT("Signature:         %08lx\n", BiosDiskEntry->Signature);
                        DPRINT("Checksum:          %08lx\n", BiosDiskEntry->Checksum);
                        DPRINT("BytesPerSector:    %lu\n", BiosDiskEntry->DiskGeometry.BytesPerSector);
                        DPRINT("NumberOfCylinders: %lu\n", BiosDiskEntry->DiskGeometry.NumberOfCylinders);
                        DPRINT("NumberOfHeads:     %lu\n", BiosDiskEntry->DiskGeometry.NumberOfHeads);
                        DPRINT("DriveSelect:       %02x\n", BiosDiskEntry->Int13DiskData.DriveSelect);
                        DPRINT("MaxCylinders:      %lu\n", BiosDiskEntry->Int13DiskData.MaxCylinders);
                        DPRINT("SectorsPerTrack:   %d\n", BiosDiskEntry->Int13DiskData.SectorsPerTrack);
                        DPRINT("MaxHeads:          %d\n", BiosDiskEntry->Int13DiskData.MaxHeads);
                        DPRINT("NumberDrives:      %d\n", BiosDiskEntry->Int13DiskData.NumberDrives);

                        DiskCount++;
                    }
                }

                RtlFreeHeap(RtlGetProcessHeap(), 0, Int13Drives);
                return;
            }
        }

        AdapterCount++;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, Int13Drives);
}


static
VOID
AddMbrPartitionToDisk(
    ULONG DiskNumber,
    PDISKENTRY DiskEntry,
    ULONG PartitionIndex,
    BOOLEAN LogicalPartition)
{
    PPARTITION_INFORMATION_EX PartitionInfo;
    PPARTENTRY PartEntry;

    PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[PartitionIndex];
    if (PartitionInfo->Mbr.PartitionType == 0 ||
        (LogicalPartition == TRUE && IsContainerPartition(PartitionInfo->Mbr.PartitionType)))
        return;

    PartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                sizeof(PARTENTRY));
    if (PartEntry == NULL)
    {
        /* TODO: Error message */
        return;
    }

    PartEntry->DiskEntry = DiskEntry;

    PartEntry->StartSector.QuadPart = (ULONGLONG)PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector;
    PartEntry->SectorCount.QuadPart = (ULONGLONG)PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector;

    PartEntry->Mbr.BootIndicator = PartitionInfo->Mbr.BootIndicator;
    PartEntry->Mbr.PartitionType = PartitionInfo->Mbr.PartitionType;

    PartEntry->LogicalPartition = LogicalPartition;
    PartEntry->IsPartitioned = TRUE;
    PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
    PartEntry->PartitionIndex = PartitionIndex;

    if (IsContainerPartition(PartEntry->Mbr.PartitionType))
    {
        PartEntry->FormatState = Unformatted;

        if (LogicalPartition == FALSE && DiskEntry->ExtendedPartition == NULL)
            DiskEntry->ExtendedPartition = PartEntry;
    }
    else if ((PartEntry->Mbr.PartitionType == PARTITION_FAT_12) ||
             (PartEntry->Mbr.PartitionType == PARTITION_FAT_16) ||
             (PartEntry->Mbr.PartitionType == PARTITION_HUGE) ||
             (PartEntry->Mbr.PartitionType == PARTITION_XINT13) ||
             (PartEntry->Mbr.PartitionType == PARTITION_FAT32) ||
             (PartEntry->Mbr.PartitionType == PARTITION_FAT32_XINT13))
    {
#if 0
        if (CheckFatFormat())
        {
            PartEntry->FormatState = Preformatted;
        }
        else
        {
            PartEntry->FormatState = Unformatted;
        }
#endif
        PartEntry->FormatState = Preformatted;
    }
    else if (PartEntry->Mbr.PartitionType == PARTITION_LINUX)
    {
#if 0
        if (CheckExt2Format())
        {
            PartEntry->FormatState = Preformatted;
        }
        else
        {
            PartEntry->FormatState = Unformatted;
        }
#endif
        PartEntry->FormatState = Preformatted;
    }
    else if (PartEntry->Mbr.PartitionType == PARTITION_IFS)
    {
#if 0
        if (CheckNtfsFormat())
        {
            PartEntry->FormatState = Preformatted;
        }
        else if (CheckHpfsFormat())
        {
            PartEntry->FormatState = Preformatted;
        }
        else
        {
            PartEntry->FormatState = Unformatted;
        }
#endif
        PartEntry->FormatState = Preformatted;
    }
    else
    {
        PartEntry->FormatState = UnknownFormat;
    }

    if (LogicalPartition)
        InsertTailList(&DiskEntry->LogicalPartListHead,
                       &PartEntry->ListEntry);
    else
        InsertTailList(&DiskEntry->PrimaryPartListHead,
                       &PartEntry->ListEntry);
}


static
VOID
AddGptPartitionToDisk(
    ULONG DiskNumber,
    PDISKENTRY DiskEntry,
    ULONG PartitionIndex)
{
    PPARTITION_INFORMATION_EX PartitionInfo;
    PPARTENTRY PartEntry;

    PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[PartitionIndex];
    if (IsEqualGUID(&PartitionInfo->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
        return;

    PartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                sizeof(PARTENTRY));
    if (PartEntry == NULL)
    {
        /* TODO: Error message */
        return;
    }

    PartEntry->DiskEntry = DiskEntry;

    PartEntry->StartSector.QuadPart = (ULONGLONG)PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector;
    PartEntry->SectorCount.QuadPart = (ULONGLONG)PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector;

    CopyMemory(&PartEntry->Gpt.PartitionType, &PartitionInfo->Gpt.PartitionType, sizeof(GUID));
    CopyMemory(&PartEntry->Gpt.PartitionId, &PartitionInfo->Gpt.PartitionId, sizeof(GUID));
    PartEntry->Gpt.Attributes = PartitionInfo->Gpt.Attributes;

    PartEntry->LogicalPartition = FALSE;
    PartEntry->IsPartitioned = TRUE;
    PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
    PartEntry->PartitionIndex = PartitionIndex;

    /* TODO: Determine the format state */
    PartEntry->FormatState = Unformatted;

    InsertTailList(&DiskEntry->PrimaryPartListHead,
                   &PartEntry->ListEntry);
}


VOID
ScanForUnpartitionedMbrDiskSpace(
    PDISKENTRY DiskEntry)
{
    ULONGLONG StartSector, EndSector;
    ULONGLONG LastStartSector;
    ULONGLONG LastSectorCount;
    ULONGLONG LastUnusedSectorCount;
    PPARTENTRY PartEntry;
    PPARTENTRY NewPartEntry;
    PLIST_ENTRY Entry;

    DPRINT("ScanForUnpartitionedMbrDiskSpace()\n");

    /* Calculate the disk sector limits */
    /* Limit the SectorCount to 2^32 sectors for MBR disks */
    StartSector = (ULONGLONG)DiskEntry->SectorAlignment;
    EndSector = min(DiskEntry->SectorCount.QuadPart, 0x100000000) - 1;

    if (IsListEmpty(&DiskEntry->PrimaryPartListHead))
    {
        DPRINT1("No primary partition!\n");

        /* Create a partition table that represents the empty disk */
        NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(PARTENTRY));
        if (NewPartEntry == NULL)
            return;

        NewPartEntry->DiskEntry = DiskEntry;

        NewPartEntry->IsPartitioned = FALSE;
        NewPartEntry->StartSector.QuadPart = StartSector;
        NewPartEntry->SectorCount.QuadPart = AlignDown(EndSector + 1, DiskEntry->SectorAlignment) -
                                             NewPartEntry->StartSector.QuadPart;

        DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

        NewPartEntry->FormatState = Unformatted;

        InsertTailList(&DiskEntry->PrimaryPartListHead,
                       &NewPartEntry->ListEntry);

        return;
    }

    /* Start at the first usable sector */
    LastStartSector = StartSector;
    LastSectorCount = 0ULL;
    LastUnusedSectorCount = 0ULL;

    Entry = DiskEntry->PrimaryPartListHead.Flink;
    while (Entry != &DiskEntry->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED ||
            PartEntry->SectorCount.QuadPart != 0ULL)
        {
            LastUnusedSectorCount =
                PartEntry->StartSector.QuadPart - (LastStartSector + LastSectorCount);

            if (PartEntry->StartSector.QuadPart > (LastStartSector + LastSectorCount) &&
                LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
            {
                DPRINT("Unpartitioned disk space %I64u sectors\n", LastUnusedSectorCount);

                NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               sizeof(PARTENTRY));
                if (NewPartEntry == NULL)
                    return;

                NewPartEntry->DiskEntry = DiskEntry;

                NewPartEntry->IsPartitioned = FALSE;
                NewPartEntry->StartSector.QuadPart = LastStartSector + LastSectorCount;
                NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + LastUnusedSectorCount, DiskEntry->SectorAlignment) -
                                                     NewPartEntry->StartSector.QuadPart;

                DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
                DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
                DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

                NewPartEntry->FormatState = Unformatted;

                /* Insert the table into the list */
                InsertTailList(&PartEntry->ListEntry,
                               &NewPartEntry->ListEntry);
            }

            LastStartSector = PartEntry->StartSector.QuadPart;
            LastSectorCount = PartEntry->SectorCount.QuadPart;
        }

        Entry = Entry->Flink;
    }

    /* Check for trailing unpartitioned disk space */
    if ((LastStartSector + LastSectorCount) < (EndSector + 1))
    {
        LastUnusedSectorCount = AlignDown((EndSector + 1) - (LastStartSector + LastSectorCount), DiskEntry->SectorAlignment);

        if (LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
        {
            DPRINT1("Unpartitioned disk space: %I64u sectors\n", LastUnusedSectorCount);

            NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(PARTENTRY));
            if (NewPartEntry == NULL)
                return;

            NewPartEntry->DiskEntry = DiskEntry;

            NewPartEntry->IsPartitioned = FALSE;
            NewPartEntry->StartSector.QuadPart = LastStartSector + LastSectorCount;
            NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + LastUnusedSectorCount, DiskEntry->SectorAlignment) -
                                                 NewPartEntry->StartSector.QuadPart;

            DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
            DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
            DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

            NewPartEntry->FormatState = Unformatted;

            /* Append the table to the list */
            InsertTailList(&DiskEntry->PrimaryPartListHead,
                           &NewPartEntry->ListEntry);
        }
    }

    if (DiskEntry->ExtendedPartition != NULL)
    {
        if (IsListEmpty(&DiskEntry->LogicalPartListHead))
        {
            DPRINT1("No logical partition!\n");

            /* Create a partition table entry that represents the empty extended partition */
            NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(PARTENTRY));
            if (NewPartEntry == NULL)
                return;

            NewPartEntry->DiskEntry = DiskEntry;
            NewPartEntry->LogicalPartition = TRUE;

            NewPartEntry->IsPartitioned = FALSE;
            NewPartEntry->StartSector.QuadPart = DiskEntry->ExtendedPartition->StartSector.QuadPart + (ULONGLONG)DiskEntry->SectorAlignment;
            NewPartEntry->SectorCount.QuadPart = DiskEntry->ExtendedPartition->SectorCount.QuadPart - (ULONGLONG)DiskEntry->SectorAlignment;

            DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
            DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
            DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

            NewPartEntry->FormatState = Unformatted;

            InsertTailList(&DiskEntry->LogicalPartListHead,
                           &NewPartEntry->ListEntry);

            return;
        }

        /* Start partition at head 1, cylinder 0 */
        LastStartSector = DiskEntry->ExtendedPartition->StartSector.QuadPart + (ULONGLONG)DiskEntry->SectorAlignment;
        LastSectorCount = 0ULL;
        LastUnusedSectorCount = 0ULL;

        Entry = DiskEntry->LogicalPartListHead.Flink;
        while (Entry != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            if (PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED ||
                PartEntry->SectorCount.QuadPart != 0ULL)
            {
                LastUnusedSectorCount =
                    PartEntry->StartSector.QuadPart - (ULONGLONG)DiskEntry->SectorAlignment - (LastStartSector + LastSectorCount);

                if ((PartEntry->StartSector.QuadPart - (ULONGLONG)DiskEntry->SectorAlignment) > (LastStartSector + LastSectorCount) &&
                    LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
                {
                    DPRINT("Unpartitioned disk space %I64u sectors\n", LastUnusedSectorCount);

                    NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   sizeof(PARTENTRY));
                    if (NewPartEntry == NULL)
                        return;

                    NewPartEntry->DiskEntry = DiskEntry;
                    NewPartEntry->LogicalPartition = TRUE;

                    NewPartEntry->IsPartitioned = FALSE;
                    NewPartEntry->StartSector.QuadPart = LastStartSector + LastSectorCount;
                    NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + LastUnusedSectorCount, DiskEntry->SectorAlignment) -
                                                         NewPartEntry->StartSector.QuadPart;

                    DPRINT("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
                    DPRINT("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
                    DPRINT("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

                    NewPartEntry->FormatState = Unformatted;

                    /* Insert the table into the list */
                    InsertTailList(&PartEntry->ListEntry,
                                   &NewPartEntry->ListEntry);
                }

                LastStartSector = PartEntry->StartSector.QuadPart;
                LastSectorCount = PartEntry->SectorCount.QuadPart;
            }

            Entry = Entry->Flink;
        }

        /* Check for trailing unpartitioned disk space */
        if ((LastStartSector + LastSectorCount) < DiskEntry->ExtendedPartition->StartSector.QuadPart + DiskEntry->ExtendedPartition->SectorCount.QuadPart)
        {
            LastUnusedSectorCount = AlignDown(DiskEntry->ExtendedPartition->StartSector.QuadPart + DiskEntry->ExtendedPartition->SectorCount.QuadPart - (LastStartSector + LastSectorCount),
                                              DiskEntry->SectorAlignment);

            if (LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
            {
                DPRINT("Unpartitioned disk space: %I64u sectors\n", LastUnusedSectorCount);

                NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               sizeof(PARTENTRY));
                if (NewPartEntry == NULL)
                    return;

                NewPartEntry->DiskEntry = DiskEntry;
                NewPartEntry->LogicalPartition = TRUE;

                NewPartEntry->IsPartitioned = FALSE;
                NewPartEntry->StartSector.QuadPart = LastStartSector + LastSectorCount;
                NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + LastUnusedSectorCount, DiskEntry->SectorAlignment) -
                                                     NewPartEntry->StartSector.QuadPart;

                DPRINT("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
                DPRINT("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
                DPRINT("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

                NewPartEntry->FormatState = Unformatted;

                /* Append the table to the list */
                InsertTailList(&DiskEntry->LogicalPartListHead,
                               &NewPartEntry->ListEntry);
            }
        }
    }

    DPRINT("ScanForUnpartitionedMbrDiskSpace() done\n");
}


VOID
ScanForUnpartitionedGptDiskSpace(
    PDISKENTRY DiskEntry)
{
    ULONGLONG LastStartSector;
    ULONGLONG LastSectorCount;
    ULONGLONG LastUnusedSectorCount;
    PPARTENTRY PartEntry;
    PPARTENTRY NewPartEntry;
    PLIST_ENTRY Entry;

    DPRINT("ScanForUnpartitionedGptDiskSpace()\n");

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(DiskEntry);
#endif

    if (IsListEmpty(&DiskEntry->PrimaryPartListHead))
    {
        DPRINT("No partitions!\n");

        /* Create a partition table that represents the empty disk */
        NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(PARTENTRY));
        if (NewPartEntry == NULL)
            return;

        NewPartEntry->DiskEntry = DiskEntry;

        NewPartEntry->IsPartitioned = FALSE;
        NewPartEntry->StartSector.QuadPart = DiskEntry->StartSector.QuadPart;
        NewPartEntry->SectorCount.QuadPart = DiskEntry->EndSector.QuadPart - DiskEntry->StartSector.QuadPart + 1;

        DPRINT("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
        DPRINT("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
        DPRINT("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

        NewPartEntry->FormatState = Unformatted;

        InsertTailList(&DiskEntry->PrimaryPartListHead,
                       &NewPartEntry->ListEntry);

#ifdef DUMP_PARTITION_LIST
        DumpPartitionList(DiskEntry);
#endif

        return;
    }

    /* Start at the first usable sector */
    LastStartSector = DiskEntry->StartSector.QuadPart;
    LastSectorCount = 0ULL;
    LastUnusedSectorCount = 0ULL;

    Entry = DiskEntry->PrimaryPartListHead.Flink;
    while (Entry != &DiskEntry->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (!IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID) ||
            PartEntry->SectorCount.QuadPart != 0ULL)
        {
            LastUnusedSectorCount =
                PartEntry->StartSector.QuadPart - (LastStartSector + LastSectorCount);

            if (PartEntry->StartSector.QuadPart > (LastStartSector + LastSectorCount) &&
                LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
            {
                DPRINT("Unpartitioned disk space %I64u sectors\n", LastUnusedSectorCount);

                NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               sizeof(PARTENTRY));
                if (NewPartEntry == NULL)
                    return;

                NewPartEntry->DiskEntry = DiskEntry;

                NewPartEntry->IsPartitioned = FALSE;
                NewPartEntry->StartSector.QuadPart = LastStartSector + LastSectorCount;
                NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + LastUnusedSectorCount, DiskEntry->SectorAlignment) -
                                                     NewPartEntry->StartSector.QuadPart;

                DPRINT("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
                DPRINT("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
                DPRINT("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

                NewPartEntry->FormatState = Unformatted;

                /* Insert the table into the list */
                InsertTailList(&PartEntry->ListEntry,
                               &NewPartEntry->ListEntry);
            }

            LastStartSector = PartEntry->StartSector.QuadPart;
            LastSectorCount = PartEntry->SectorCount.QuadPart;
        }

        Entry = Entry->Flink;
    }

    /* Check for trailing unpartitioned disk space */
    if ((LastStartSector + LastSectorCount) < DiskEntry->EndSector.QuadPart + 1)
    {
        LastUnusedSectorCount = AlignDown(DiskEntry->EndSector.QuadPart + 1 - (LastStartSector + LastSectorCount), DiskEntry->SectorAlignment);

        if (LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
        {
            DPRINT("Unpartitioned disk space: %I64u sectors\n", LastUnusedSectorCount);

            NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(PARTENTRY));
            if (NewPartEntry == NULL)
                return;

            NewPartEntry->DiskEntry = DiskEntry;

            NewPartEntry->IsPartitioned = FALSE;
            NewPartEntry->StartSector.QuadPart = LastStartSector + LastSectorCount;
            NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + LastUnusedSectorCount, DiskEntry->SectorAlignment) -
                                                 NewPartEntry->StartSector.QuadPart;

            DPRINT("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
            DPRINT("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
            DPRINT("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

            NewPartEntry->FormatState = Unformatted;

            /* Append the table to the list */
            InsertTailList(&DiskEntry->PrimaryPartListHead,
                           &NewPartEntry->ListEntry);
        }
    }

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(DiskEntry);
#endif

    DPRINT("ScanForUnpartitionedGptDiskSpace() done\n");
}


VOID
ReadLayoutBuffer(
    _In_ HANDLE FileHandle,
    _In_ PDISKENTRY DiskEntry)
{
    ULONG LayoutBufferSize;
    PDRIVE_LAYOUT_INFORMATION_EX NewLayoutBuffer;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    /* Allocate a layout buffer with 4 partition entries first */
    LayoutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                       ((4 - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION_EX));
    DiskEntry->LayoutBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              LayoutBufferSize);
    if (DiskEntry->LayoutBuffer == NULL)
    {
        DPRINT1("Failed to allocate the disk layout buffer!\n");
        return;
    }

    for (;;)
    {
        DPRINT("Buffer size: %lu\n", LayoutBufferSize);
        Status = NtDeviceIoControlFile(FileHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                       NULL,
                                       0,
                                       DiskEntry->LayoutBuffer,
                                       LayoutBufferSize);
        if (NT_SUCCESS(Status))
            break;

        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            DPRINT1("NtDeviceIoControlFile() failed (Status: 0x%08lx)\n", Status);
            return;
        }

        LayoutBufferSize += 4 * sizeof(PARTITION_INFORMATION_EX);
        NewLayoutBuffer = RtlReAllocateHeap(RtlGetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            DiskEntry->LayoutBuffer,
                                            LayoutBufferSize);
        if (NewLayoutBuffer == NULL)
        {
            DPRINT1("Failed to reallocate the disk layout buffer!\n");
            return;
        }

        DiskEntry->LayoutBuffer = NewLayoutBuffer;
    }
}


static
VOID
AddDiskToList(
    HANDLE FileHandle,
    ULONG DiskNumber)
{
    DISK_GEOMETRY DiskGeometry;
    SCSI_ADDRESS ScsiAddress;
    PDISKENTRY DiskEntry;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    PPARTITION_SECTOR Mbr;
    PULONG Buffer;
    LARGE_INTEGER FileOffset;
    WCHAR Identifier[20];
    ULONG Checksum;
    ULONG Signature;
    ULONG i;
    PLIST_ENTRY ListEntry;
    PBIOSDISKENTRY BiosDiskEntry;

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
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    if (DiskGeometry.MediaType != FixedMedia &&
        DiskGeometry.MediaType != RemovableMedia)
    {
        return;
    }

    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_SCSI_GET_ADDRESS,
                                   NULL,
                                   0,
                                   &ScsiAddress,
                                   sizeof(SCSI_ADDRESS));
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    PBYTE pBuffer = NULL;

    pBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             1024);

    STORAGE_PROPERTY_QUERY StoragePropertyQuery;
    StoragePropertyQuery.PropertyId = StorageDeviceProperty;
    StoragePropertyQuery.QueryType = PropertyStandardQuery;
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_STORAGE_QUERY_PROPERTY,
                                   &StoragePropertyQuery,
                                   sizeof(STORAGE_PROPERTY_QUERY),
                                   pBuffer,
                                   1024);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, pBuffer);
        pBuffer = NULL;
    }


    Mbr = (PARTITION_SECTOR*)RtlAllocateHeap(RtlGetProcessHeap(),
                                             0,
                                             DiskGeometry.BytesPerSector);
    if (Mbr == NULL)
    {
        return;
    }

    FileOffset.QuadPart = 0;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &Iosb,
                        (PVOID)Mbr,
                        DiskGeometry.BytesPerSector,
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Mbr);
        DPRINT1("NtReadFile failed, status=%x\n", Status);
        return;
    }
    Signature = Mbr->Signature;

    /* Calculate the MBR checksum */
    Checksum = 0;
    Buffer = (PULONG)Mbr;
    for (i = 0; i < 128; i++)
    {
        Checksum += Buffer[i];
    }
    Checksum = ~Checksum + 1;

    StringCchPrintfW(Identifier, ARRAYSIZE(Identifier),
                     L"%08x-%08x-A", Checksum, Signature);
    DPRINT("Identifier: %S\n", Identifier);

    DiskEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                sizeof(DISKENTRY));
    if (DiskEntry == NULL)
    {
        if (pBuffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pBuffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, Mbr);
        return;
    }

    if (pBuffer)
    {
        PSTORAGE_DESCRIPTOR_HEADER pDescriptorHeader;

        pDescriptorHeader = (PSTORAGE_DESCRIPTOR_HEADER)pBuffer;
        DPRINT("DescriptorHeader.Size %lu\n", pDescriptorHeader->Size);

        if (pDescriptorHeader->Size <= 1024)
        {
            PSTORAGE_DEVICE_DESCRIPTOR pDeviceDescriptor;
            pDeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)pBuffer;
            DPRINT("VendorIdOffset %lu\n", pDeviceDescriptor->VendorIdOffset);
            DPRINT("ProductIdOffset %lu\n", pDeviceDescriptor->ProductIdOffset);
            DPRINT("ProductRevisionOffset %lu\n", pDeviceDescriptor->ProductRevisionOffset);
            DPRINT("SerialNumberOffset %lu\n", pDeviceDescriptor->SerialNumberOffset);
            DPRINT("BusType: %u\n", pDeviceDescriptor->BusType);
            if (pDeviceDescriptor->VendorIdOffset)
            {
                DPRINT("Vendor: %s\n", (PSTR)((ULONG_PTR)pBuffer + pDeviceDescriptor->VendorIdOffset));
            }

            if (pDeviceDescriptor->ProductIdOffset)
            {
                DPRINT("Product: %s\n", (PSTR)((ULONG_PTR)pBuffer + pDeviceDescriptor->ProductIdOffset));
            }

            INT VendorLength = 0, ProductLength = 0;
            PWSTR VendorBuffer = NULL, ProductBuffer = NULL;

            if (pDeviceDescriptor->VendorIdOffset)
            {
                VendorLength = MultiByteToWideChar(437,
                                                   0,
                                                   (PSTR)((ULONG_PTR)pBuffer + pDeviceDescriptor->VendorIdOffset),
                                                   -1,
                                                   NULL,
                                                   0);
                VendorBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               VendorLength * sizeof(WCHAR));
                if (VendorBuffer)
                    MultiByteToWideChar(437,
                                        0,
                                        (PSTR)((ULONG_PTR)pBuffer + pDeviceDescriptor->VendorIdOffset),
                                        -1,
                                        VendorBuffer,
                                        VendorLength);
            }

            if (pDeviceDescriptor->ProductIdOffset)
            {
                ProductLength = MultiByteToWideChar(437,
                                                   0,
                                                   (PSTR)((ULONG_PTR)pBuffer + pDeviceDescriptor->ProductIdOffset),
                                                   -1,
                                                   NULL,
                                                   0);

                ProductBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                HEAP_ZERO_MEMORY,
                                                ProductLength * sizeof(WCHAR));
                if (ProductBuffer)
                    MultiByteToWideChar(437,
                                        0,
                                        (PSTR)((ULONG_PTR)pBuffer + pDeviceDescriptor->ProductIdOffset),
                                        -1,
                                        ProductBuffer,
                                        ProductLength);
            }

            DiskEntry->Description = RtlAllocateHeap(RtlGetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     (VendorLength + ProductLength + 2) * sizeof(WCHAR));
            if (VendorBuffer)
                wcscat(DiskEntry->Description, VendorBuffer);

            if ((VendorLength > 0) && (ProductLength > 0))
                wcscat(DiskEntry->Description, L" ");

            if (ProductBuffer)
                wcscat(DiskEntry->Description, ProductBuffer);

            DiskEntry->BusType = pDeviceDescriptor->BusType;

            if (VendorBuffer)
                RtlFreeHeap(RtlGetProcessHeap(), 0, VendorBuffer);

            if (ProductBuffer)
                RtlFreeHeap(RtlGetProcessHeap(), 0, ProductBuffer);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, pBuffer);
    }

//    DiskEntry->Checksum = Checksum;
//    DiskEntry->Signature = Signature;
    DiskEntry->BiosFound = FALSE;

    /* Check the disk partition style */
    if (Mbr->Magic != MBR_MAGIC)
    {
        DPRINT("Partition style: RAW\n");
        DiskEntry->PartitionStyle = PARTITION_STYLE_RAW;
    }
    else
    {
        if (Mbr->Partition[0].PartitionType == PARTITION_GPT)
        {
            DPRINT("Partition style: GPT\n");
            DiskEntry->PartitionStyle = PARTITION_STYLE_GPT;
        }
        else
        {
            DPRINT("Partition style: MBR\n");
            DiskEntry->PartitionStyle = PARTITION_STYLE_MBR;
        }
    }

    /* Free Mbr sector buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Mbr);

    ListEntry = BiosDiskListHead.Flink;
    while (ListEntry != &BiosDiskListHead)
    {
        BiosDiskEntry = CONTAINING_RECORD(ListEntry, BIOSDISKENTRY, ListEntry);
        /* FIXME:
         *   Compare the size from bios and the reported size from driver.
         *   If we have more than one disk with a zero or with the same signatur
         *   we must create new signatures and reboot. After the reboot,
         *   it is possible to identify the disks.
         */
        if (BiosDiskEntry->Signature == Signature &&
            BiosDiskEntry->Checksum == Checksum &&
            !BiosDiskEntry->Recognized)
        {
            if (!DiskEntry->BiosFound)
            {
                DiskEntry->BiosDiskNumber = BiosDiskEntry->DiskNumber;
                DiskEntry->BiosFound = TRUE;
                BiosDiskEntry->Recognized = TRUE;
            }
            else
            {
            }
        }
        ListEntry = ListEntry->Flink;
    }

    if (!DiskEntry->BiosFound)
    {
#if 0
        RtlFreeHeap(ProcessHeap, 0, DiskEntry);
        return;
#else
        DPRINT1("WARNING: Setup could not find a matching BIOS disk entry. Disk %d is not be bootable by the BIOS!\n", DiskNumber);
#endif
    }

    InitializeListHead(&DiskEntry->PrimaryPartListHead);
    InitializeListHead(&DiskEntry->LogicalPartListHead);

    DiskEntry->Cylinders = DiskGeometry.Cylinders.QuadPart;
    DiskEntry->TracksPerCylinder = DiskGeometry.TracksPerCylinder;
    DiskEntry->SectorsPerTrack = DiskGeometry.SectorsPerTrack;
    DiskEntry->BytesPerSector = DiskGeometry.BytesPerSector;

    DPRINT("Cylinders %I64u\n", DiskEntry->Cylinders);
    DPRINT("TracksPerCylinder %I64u\n", DiskEntry->TracksPerCylinder);
    DPRINT("SectorsPerTrack %I64u\n", DiskEntry->SectorsPerTrack);
    DPRINT("BytesPerSector %I64u\n", DiskEntry->BytesPerSector);

    DiskEntry->SectorCount.QuadPart = DiskGeometry.Cylinders.QuadPart *
                                      (ULONGLONG)DiskGeometry.TracksPerCylinder *
                                      (ULONGLONG)DiskGeometry.SectorsPerTrack;

//    DiskEntry->SectorAlignment = DiskGeometry.SectorsPerTrack;
//    DiskEntry->CylinderAlignment = DiskGeometry.SectorsPerTrack * DiskGeometry.TracksPerCylinder;
    DiskEntry->SectorAlignment = (1024 * 1024) / DiskGeometry.BytesPerSector;
    DiskEntry->CylinderAlignment = (1024 * 1024) / DiskGeometry.BytesPerSector;

    DPRINT("SectorCount: %I64u\n", DiskEntry->SectorCount);
    DPRINT("SectorAlignment: %lu\n", DiskEntry->SectorAlignment);
    DPRINT("CylinderAlignment: %lu\n", DiskEntry->CylinderAlignment);

    DiskEntry->DiskNumber = DiskNumber;
    DiskEntry->Port = ScsiAddress.PortNumber;
    DiskEntry->PathId = ScsiAddress.PathId;
    DiskEntry->TargetId = ScsiAddress.TargetId;
    DiskEntry->Lun = ScsiAddress.Lun;

    GetDriverName(DiskEntry);

    InsertAscendingList(&DiskListHead, DiskEntry, DISKENTRY, ListEntry, DiskNumber);

    ReadLayoutBuffer(FileHandle, DiskEntry);

    DPRINT("PartitionCount: %lu\n", DiskEntry->LayoutBuffer->PartitionCount);

#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(DiskEntry);
#endif

    if (DiskEntry->PartitionStyle == PARTITION_STYLE_MBR)
    {
        if (DiskEntry->LayoutBuffer->PartitionEntry[0].StartingOffset.QuadPart != 0 &&
            DiskEntry->LayoutBuffer->PartitionEntry[0].PartitionLength.QuadPart != 0 &&
            DiskEntry->LayoutBuffer->PartitionEntry[0].Mbr.PartitionType != 0)
        {
            if ((DiskEntry->LayoutBuffer->PartitionEntry[0].StartingOffset.QuadPart / DiskEntry->BytesPerSector) % DiskEntry->SectorsPerTrack == 0)
            {
                DPRINT("Use %lu Sector alignment!\n", DiskEntry->SectorsPerTrack);
            }
            else if (DiskEntry->LayoutBuffer->PartitionEntry[0].StartingOffset.QuadPart % (1024 * 1024) == 0)
            {
                DPRINT1("Use megabyte (%lu Sectors) alignment!\n", (1024 * 1024) / DiskEntry->BytesPerSector);
            }
            else
            {
                DPRINT1("No matching alignment found! Partition 1 starts at %I64u\n", DiskEntry->LayoutBuffer->PartitionEntry[0].StartingOffset.QuadPart);
            }
        }
        else
        {
            DPRINT1("No valid partition table found! Use megabyte (%lu Sectors) alignment!\n", (1024 * 1024) / DiskEntry->BytesPerSector);
        }

        /* Calculate the number of usable sectors */
        /* Limit the number of usable sectors to 2^32 */
        DiskEntry->StartSector.QuadPart = (ULONGLONG)DiskEntry->SectorAlignment;
        DiskEntry->EndSector.QuadPart = min(DiskEntry->SectorCount.QuadPart, 0x100000000) - 1;

        if (DiskEntry->LayoutBuffer->PartitionCount == 0)
        {
            DiskEntry->NewDisk = TRUE;
            DiskEntry->LayoutBuffer->PartitionCount = 4;

            for (i = 0; i < 4; i++)
                DiskEntry->LayoutBuffer->PartitionEntry[i].RewritePartition = TRUE;
        }
        else
        {
            for (i = 0; i < 4; i++)
            {
                AddMbrPartitionToDisk(DiskNumber, DiskEntry, i, FALSE);
            }

            for (i = 4; i < DiskEntry->LayoutBuffer->PartitionCount; i += 4)
            {
                AddMbrPartitionToDisk(DiskNumber, DiskEntry, i, TRUE);
            }
        }

        ScanForUnpartitionedMbrDiskSpace(DiskEntry);
    }
    else if (DiskEntry->PartitionStyle == PARTITION_STYLE_GPT)
    {
        /* Calculate the number of usable sectors */
        DiskEntry->StartSector.QuadPart = AlignDown(DiskEntry->LayoutBuffer->Gpt.StartingUsableOffset.QuadPart / DiskEntry->BytesPerSector,
                                                    DiskEntry->SectorAlignment) + (ULONGLONG)DiskEntry->SectorAlignment;
        DiskEntry->EndSector.QuadPart = AlignDown(DiskEntry->StartSector.QuadPart + (DiskEntry->LayoutBuffer->Gpt.UsableLength.QuadPart / DiskEntry->BytesPerSector) - 1,
                                                  DiskEntry->SectorAlignment);

        if (DiskEntry->LayoutBuffer->PartitionCount == 0)
        {
            DiskEntry->NewDisk = TRUE;
        }
        else
        {
            for (i = 0; i < DiskEntry->LayoutBuffer->PartitionCount; i++)
            {
                AddGptPartitionToDisk(DiskNumber, DiskEntry, i);
            }
        }

        ScanForUnpartitionedGptDiskSpace(DiskEntry);
    }
}


NTSTATUS
CreatePartitionList(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    SYSTEM_DEVICE_INFORMATION Sdi;
    IO_STATUS_BLOCK Iosb;
    ULONG ReturnSize;
    NTSTATUS Status;
    ULONG DiskNumber;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING Name;
    HANDLE FileHandle;

    CurrentDisk = NULL;
    CurrentPartition = NULL;

//    BootDisk = NULL;
//    BootPartition = NULL;

//    TempDisk = NULL;
//    TempPartition = NULL;
//    FormatState = Start;

    InitializeListHead(&DiskListHead);
    InitializeListHead(&BiosDiskListHead);

    EnumerateBiosDiskEntries();

    Status = NtQuerySystemInformation(SystemDeviceInformation,
                                      &Sdi,
                                      sizeof(SYSTEM_DEVICE_INFORMATION),
                                      &ReturnSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    for (DiskNumber = 0; DiskNumber < Sdi.NumberOfDisks; DiskNumber++)
    {
        StringCchPrintfW(Buffer, ARRAYSIZE(Buffer),
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
            AddDiskToList(FileHandle, DiskNumber);

            NtClose(FileHandle);
        }
    }

//    UpdateDiskSignatures(List);

//    AssignDriveLetters(List);

    return STATUS_SUCCESS;
}


VOID
DestroyPartitionList(VOID)
{
    PDISKENTRY DiskEntry;
    PBIOSDISKENTRY BiosDiskEntry;
    PPARTENTRY PartEntry;
    PLIST_ENTRY Entry;

    CurrentDisk = NULL;
    CurrentPartition = NULL;

    /* Release disk and partition info */
    while (!IsListEmpty(&DiskListHead))
    {
        Entry = RemoveHeadList(&DiskListHead);
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        /* Release driver name */
        RtlFreeUnicodeString(&DiskEntry->DriverName);

        /* Release primary partition list */
        while (!IsListEmpty(&DiskEntry->PrimaryPartListHead))
        {
            Entry = RemoveHeadList(&DiskEntry->PrimaryPartListHead);
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            RtlFreeHeap(RtlGetProcessHeap(), 0, PartEntry);
        }

        /* Release logical partition list */
        while (!IsListEmpty(&DiskEntry->LogicalPartListHead))
        {
            Entry = RemoveHeadList(&DiskEntry->LogicalPartListHead);
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            RtlFreeHeap(RtlGetProcessHeap(), 0, PartEntry);
        }

        /* Release layout buffer */
        if (DiskEntry->LayoutBuffer != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, DiskEntry->LayoutBuffer);

        if (DiskEntry->Description != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, DiskEntry->Description);

        if (DiskEntry->Location != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, DiskEntry->Location);

        /* Release disk entry */
        RtlFreeHeap(RtlGetProcessHeap(), 0, DiskEntry);
    }

    /* Release the bios disk info */
    while (!IsListEmpty(&BiosDiskListHead))
    {
        Entry = RemoveHeadList(&BiosDiskListHead);
        BiosDiskEntry = CONTAINING_RECORD(Entry, BIOSDISKENTRY, ListEntry);

        RtlFreeHeap(RtlGetProcessHeap(), 0, BiosDiskEntry);
    }
}


static
VOID
GetVolumeExtents(
    _In_ HANDLE VolumeHandle,
    _In_ PVOLENTRY VolumeEntry)
{
    DWORD dwBytesReturned = 0, dwLength, i;
    PVOLUME_DISK_EXTENTS pExtents;
    BOOL bResult;
    DWORD dwError;

    dwLength = sizeof(VOLUME_DISK_EXTENTS);
    pExtents = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
    if (pExtents == NULL)
        return;

    bResult = DeviceIoControl(VolumeHandle,
                              IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                              NULL,
                              0,
                              pExtents,
                              dwLength,
                              &dwBytesReturned,
                              NULL);
    if (!bResult)
    {
        dwError = GetLastError();

        if (dwError != ERROR_MORE_DATA)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, pExtents);
            return;
        }
        else
        {
            dwLength = sizeof(VOLUME_DISK_EXTENTS) + ((pExtents->NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT));
            RtlFreeHeap(RtlGetProcessHeap(), 0, pExtents);
            pExtents = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
            if (pExtents == NULL)
            {
                return;
            }

            bResult = DeviceIoControl(VolumeHandle,
                                      IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                      NULL,
                                      0,
                                      pExtents,
                                      dwLength,
                                      &dwBytesReturned,
                                      NULL);
            if (!bResult)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, pExtents);
                return;
            }
        }
    }

    for (i = 0; i < pExtents->NumberOfDiskExtents; i++)
        VolumeEntry->Size.QuadPart += pExtents->Extents[i].ExtentLength.QuadPart;

    VolumeEntry->pExtents = pExtents;
}


static
VOID
GetVolumeType(
    _In_ HANDLE VolumeHandle,
    _In_ PVOLENTRY VolumeEntry)
{
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          &DeviceInfo,
                                          sizeof(FILE_FS_DEVICE_INFORMATION),
                                          FileFsDeviceInformation);
    if (!NT_SUCCESS(Status))
        return;

    switch (DeviceInfo.DeviceType)
    {
        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            VolumeEntry->VolumeType = VOLUME_TYPE_CDROM;
            break;

        case FILE_DEVICE_DISK:
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            if (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA)
                VolumeEntry->VolumeType = VOLUME_TYPE_REMOVABLE;
            else
                VolumeEntry->VolumeType = VOLUME_TYPE_PARTITION;
            break;

        default:
            VolumeEntry->VolumeType = VOLUME_TYPE_UNKNOWN;
            break;
    }
}


static
VOID
GetVolumeSize(
    _In_ HANDLE VolumeHandle,
    _In_ PVOLENTRY VolumeEntry)
{
    FILE_FS_FULL_SIZE_INFORMATION SizeInfo;
    FILE_FS_FULL_SIZE_INFORMATION FullSizeInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    ZeroMemory(&FullSizeInfo, sizeof(FullSizeInfo));
    Status = NtQueryVolumeInformationFile(VolumeHandle,
                                          &IoStatusBlock,
                                          &FullSizeInfo,
                                          sizeof(FILE_FS_FULL_SIZE_INFORMATION),
                                          FileFsFullSizeInformation);
    if (NT_SUCCESS(Status))
    {
        DPRINT("FullSizeInfo.TotalAllocationUnits %I64u\n", FullSizeInfo.TotalAllocationUnits.QuadPart);
        DPRINT("FullSizeInfo.SectorsPerAllocationUnit %lu\n", FullSizeInfo.SectorsPerAllocationUnit);
        DPRINT("FullSizeInfo.BytesPerSector %lu\n", FullSizeInfo.BytesPerSector);

        VolumeEntry->TotalAllocationUnits.QuadPart = FullSizeInfo.TotalAllocationUnits.QuadPart;
        VolumeEntry->SectorsPerAllocationUnit = FullSizeInfo.SectorsPerAllocationUnit;
        VolumeEntry->BytesPerSector = FullSizeInfo.BytesPerSector;
    }
    else
    {
        ZeroMemory(&SizeInfo, sizeof(SizeInfo));
        Status = NtQueryVolumeInformationFile(VolumeHandle,
                                              &IoStatusBlock,
                                              &SizeInfo,
                                              sizeof(FILE_FS_SIZE_INFORMATION),
                                              FileFsSizeInformation);
        if (NT_SUCCESS(Status))
        {
            DPRINT("SizeInfo.TotalAllocationUnits %I64u\n", SizeInfo.TotalAllocationUnits.QuadPart);
            DPRINT("SizeInfo.SectorsPerAllocationUnit %lu\n", SizeInfo.SectorsPerAllocationUnit);
            DPRINT("SizeInfo.BytesPerSector %lu\n", SizeInfo.BytesPerSector);

            VolumeEntry->TotalAllocationUnits.QuadPart = SizeInfo.TotalAllocationUnits.QuadPart;
            VolumeEntry->SectorsPerAllocationUnit = SizeInfo.SectorsPerAllocationUnit;
            VolumeEntry->BytesPerSector = SizeInfo.BytesPerSector;
        }
        else
        {
            DPRINT("GetVolumeSize() failed!\n");
        }
    }
}


static
PDISKENTRY
GetDiskForVolume(
    _In_ PVOLENTRY VolumeEntry)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;
    INT i;

    DPRINT("GetDiskFromVolume(%p)\n", VolumeEntry);

    DPRINT("Extents: %p\n", VolumeEntry->pExtents);
    if (VolumeEntry->pExtents == NULL)
        return NULL;

    DPRINT("Extents: %lu\n", VolumeEntry->pExtents->NumberOfDiskExtents);

    Entry = DiskListHead.Flink;
    while (Entry != &DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        for (i = 0; i < VolumeEntry->pExtents->NumberOfDiskExtents; i++)
        {
            DPRINT("DiskNumber: %lu -- %lu\n", VolumeEntry->pExtents->Extents[i].DiskNumber, DiskEntry->DiskNumber);
            if (VolumeEntry->pExtents->Extents[i].DiskNumber == DiskEntry->DiskNumber)
                return DiskEntry;
        }

        Entry = Entry->Flink;
    }

    return NULL;
}


static
PPARTENTRY
GetPartitionForVolume(
    _In_ PVOLENTRY VolumeEntry)
{
    PLIST_ENTRY Entry1, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    INT i;

    DPRINT("GetPartitionFromVolume(%p)\n", VolumeEntry);

    DPRINT("Extents: %p\n", VolumeEntry->pExtents);
    if (VolumeEntry->pExtents == NULL)
        return NULL;

    DPRINT("Extents: %lu\n", VolumeEntry->pExtents->NumberOfDiskExtents);

    Entry1 = DiskListHead.Flink;
    while (Entry1 != &DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry1, DISKENTRY, ListEntry);

        for (i = 0; i < VolumeEntry->pExtents->NumberOfDiskExtents; i++)
        {
            DPRINT("DiskNumber: %lu -- %lu\n", VolumeEntry->pExtents->Extents[i].DiskNumber, DiskEntry->DiskNumber);
            if (VolumeEntry->pExtents->Extents[i].DiskNumber == DiskEntry->DiskNumber)
            {

                Entry2 = DiskEntry->PrimaryPartListHead.Flink;
                while (Entry2 != &DiskEntry->PrimaryPartListHead)
                {
                    PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);

                    if ((VolumeEntry->pExtents->Extents[i].StartingOffset.QuadPart == PartEntry->StartSector.QuadPart * PartEntry->DiskEntry->BytesPerSector) &&
                        (VolumeEntry->pExtents->Extents[i].ExtentLength.QuadPart == PartEntry->SectorCount.QuadPart * PartEntry->DiskEntry->BytesPerSector))
                        return PartEntry;

                    Entry2 = Entry2->Flink;
                }

                Entry2 = DiskEntry->LogicalPartListHead.Flink;
                while (Entry2 != &DiskEntry->LogicalPartListHead)
                {
                    PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);

                    if ((VolumeEntry->pExtents->Extents[i].StartingOffset.QuadPart == PartEntry->StartSector.QuadPart * PartEntry->DiskEntry->BytesPerSector) &&
                        (VolumeEntry->pExtents->Extents[i].ExtentLength.QuadPart == PartEntry->SectorCount.QuadPart * PartEntry->DiskEntry->BytesPerSector))
                        return PartEntry;

                    Entry2 = Entry2->Flink;
                }
            }
        }

        Entry1 = Entry1->Flink;
    }

    return NULL;
}


static
VOID
IsVolumeSystem(
    _In_ PVOLENTRY VolumeEntry)
{
    WCHAR szSystemPartition[MAX_PATH];
    PPARTENTRY PartEntry;
    HKEY hKey;
    DWORD dwError, dwLength;

    DPRINT("IsVolumeSystem()\n");

    VolumeEntry->IsSystem = FALSE;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("\n");
        return;
    }

    dwLength = sizeof(szSystemPartition);
    dwError = RegQueryValueExW(hKey,
                               L"SystemPartition",
                               NULL,
                               NULL,
                               (PBYTE)szSystemPartition,
                               &dwLength);
    RegCloseKey(hKey);

    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("\n");
        return;
    }

    DPRINT("SystemPartition: %S\n", szSystemPartition);
    DPRINT("DeviceName: %S\n", VolumeEntry->DeviceName);

    if (_wcsnicmp(szSystemPartition, VolumeEntry->DeviceName, wcslen(szSystemPartition)) == 0)
    {
        VolumeEntry->IsSystem = TRUE;

        PartEntry = GetPartitionForVolume(VolumeEntry);
        if (PartEntry)
            PartEntry->IsSystem = TRUE;
    }
}


static
VOID
IsVolumeBoot(
    _In_ PVOLENTRY VolumeEntry)
{
    WCHAR szSystemDir[MAX_PATH];
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    DPRINT("IsVolumeBoot()\n");

    VolumeEntry->IsBoot = FALSE;

    if (VolumeEntry->DriveLetter == UNICODE_NULL)
        return;

    GetSystemDirectoryW(szSystemDir,
                        ARRAYSIZE(szSystemDir));

    DPRINT("SystemDirectory: %S\n", szSystemDir);
    DPRINT("DriveLetter: %C\n", VolumeEntry->DriveLetter);

    if (szSystemDir[0] == VolumeEntry->DriveLetter)
    {
        VolumeEntry->IsBoot = TRUE;

        PartEntry = GetPartitionForVolume(VolumeEntry);
        if (PartEntry)
            PartEntry->IsBoot = TRUE;

        DiskEntry = GetDiskForVolume(VolumeEntry);
        if (DiskEntry)
            DiskEntry->IsBoot = TRUE;
    }
}


static
VOID
AddVolumeToList(
    ULONG ulVolumeNumber,
    PWSTR pszVolumeName)
{
    PVOLENTRY VolumeEntry;
    HANDLE VolumeHandle;

    DWORD dwError, dwLength;
    WCHAR szPathNames[MAX_PATH + 1];
    WCHAR szVolumeName[MAX_PATH + 1];
    WCHAR szFilesystem[MAX_PATH + 1];

    DWORD  CharCount            = 0;
    size_t Index                = 0;

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    DPRINT("AddVolumeToList(%S)\n", pszVolumeName);

    VolumeEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  sizeof(VOLENTRY));
    if (VolumeEntry == NULL)
        return;

    VolumeEntry->VolumeNumber = ulVolumeNumber;
    wcscpy(VolumeEntry->VolumeName, pszVolumeName);

    Index = wcslen(pszVolumeName) - 1;

    pszVolumeName[Index] = L'\0';

    CharCount = QueryDosDeviceW(&pszVolumeName[4], VolumeEntry->DeviceName, ARRAYSIZE(VolumeEntry->DeviceName)); 

    pszVolumeName[Index] = L'\\';

    if (CharCount == 0)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry);
        return;
    }

    DPRINT("DeviceName: %S\n", VolumeEntry->DeviceName);

    RtlInitUnicodeString(&Name, VolumeEntry->DeviceName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&VolumeHandle,
                        SYNCHRONIZE,
                        &ObjectAttributes,
                        &Iosb,
                        0,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    if (NT_SUCCESS(Status))
    {
        GetVolumeType(VolumeHandle, VolumeEntry);
        GetVolumeExtents(VolumeHandle, VolumeEntry);
        GetVolumeSize(VolumeHandle, VolumeEntry);
        NtClose(VolumeHandle);
    }

    if (GetVolumeInformationW(pszVolumeName,
                              szVolumeName,
                              MAX_PATH + 1,
                              &VolumeEntry->SerialNumber,
                              NULL,
                              NULL,
                              szFilesystem,
                              MAX_PATH + 1))
    {
        VolumeEntry->pszLabel = RtlAllocateHeap(RtlGetProcessHeap(),
                                                0,
                                                (wcslen(szVolumeName) + 1) * sizeof(WCHAR));
        if (VolumeEntry->pszLabel)
            wcscpy(VolumeEntry->pszLabel, szVolumeName);

        VolumeEntry->pszFilesystem = RtlAllocateHeap(RtlGetProcessHeap(),
                                                     0,
                                                     (wcslen(szFilesystem) + 1) * sizeof(WCHAR));
        if (VolumeEntry->pszFilesystem)
            wcscpy(VolumeEntry->pszFilesystem, szFilesystem);
    }
    else
    {
        dwError = GetLastError();
        if (dwError == ERROR_UNRECOGNIZED_VOLUME)
        {
            VolumeEntry->pszFilesystem = RtlAllocateHeap(RtlGetProcessHeap(),
                                                         0,
                                                         (3 + 1) * sizeof(WCHAR));
            if (VolumeEntry->pszFilesystem)
                wcscpy(VolumeEntry->pszFilesystem, L"RAW");
            VolumeEntry->SerialNumber = 0;
            VolumeEntry->SectorsPerAllocationUnit = 1;
            VolumeEntry->BytesPerSector = 512;
            VolumeEntry->VolumeType = VOLUME_TYPE_PARTITION;
        }
    }

    if (GetVolumePathNamesForVolumeNameW(pszVolumeName,
                                         szPathNames,
                                         ARRAYSIZE(szPathNames),
                                         &dwLength))
    {
        INT nPathLength;
        PWSTR pszPath = szPathNames;
        while (*pszPath != UNICODE_NULL)
        {
            DPRINT("PathName: %S\n", pszPath);
            nPathLength = wcslen(pszPath);
            if ((nPathLength == 3) && (iswalpha(pszPath[0])) &&
                (pszPath[1] == L':') && (pszPath[2] == L'\\'))
                VolumeEntry->DriveLetter = pszPath[0];

            pszPath += (nPathLength + 1);
        }
    }

    IsVolumeSystem(VolumeEntry);
    IsVolumeBoot(VolumeEntry);

    InsertTailList(&VolumeListHead,
                   &VolumeEntry->ListEntry);
}


NTSTATUS
CreateVolumeList(VOID)
{
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    WCHAR szVolumeName[MAX_PATH];
    ULONG ulVolumeNumber = 0;
    BOOL Success;

    CurrentVolume = NULL;

    InitializeListHead(&VolumeListHead);

    hVolume = FindFirstVolumeW(szVolumeName, ARRAYSIZE(szVolumeName));
    if (hVolume == INVALID_HANDLE_VALUE)
    {

        return STATUS_UNSUCCESSFUL;
    }

    AddVolumeToList(ulVolumeNumber++, szVolumeName);

    for (;;)
    {
        Success = FindNextVolumeW(hVolume, szVolumeName, ARRAYSIZE(szVolumeName));
        if (!Success)
        {
            break;
        }

        AddVolumeToList(ulVolumeNumber++, szVolumeName);
    }

    FindVolumeClose(hVolume);

    return STATUS_SUCCESS;
}


VOID
DestroyVolumeList(VOID)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;

    CurrentVolume = NULL;

    /* Release disk and partition info */
    while (!IsListEmpty(&VolumeListHead))
    {
        Entry = RemoveHeadList(&VolumeListHead);
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (VolumeEntry->pszLabel)
            RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry->pszLabel);

        if (VolumeEntry->pszFilesystem)
            RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry->pszFilesystem);

        if (VolumeEntry->pExtents)
            RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry->pExtents);

        /* Release disk entry */
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry);
    }
}


NTSTATUS
WriteMbrPartitions(
    _In_ PDISKENTRY DiskEntry)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE FileHandle;
    IO_STATUS_BLOCK Iosb;
    ULONG BufferSize;
    PPARTITION_INFORMATION_EX PartitionInfo;
    ULONG PartitionCount;
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    WCHAR DstPath[MAX_PATH];

    DPRINT("WriteMbrPartitions() Disk: %lu\n", DiskEntry->DiskNumber);

    /* If the disk is not dirty, there is nothing to do */
    if (!DiskEntry->Dirty)
        return STATUS_SUCCESS;

    StringCchPrintfW(DstPath, ARRAYSIZE(DstPath),
                     L"\\Device\\Harddisk%lu\\Partition0",
                     DiskEntry->DiskNumber);
    RtlInitUnicodeString(&Name, DstPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &Iosb,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        return Status;
    }

    //
    // FIXME: We first *MUST* use IOCTL_DISK_CREATE_DISK to initialize
    // the disk in MBR or GPT format in case the disk was not initialized!!
    // For this we must ask the user which format to use.
    //

    /* Save the original partition count to be restored later (see comment below) */
    PartitionCount = DiskEntry->LayoutBuffer->PartitionCount;

    /* Set the new disk layout and retrieve its updated version with possibly modified partition numbers */
    BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                 ((PartitionCount - 1) * sizeof(PARTITION_INFORMATION_EX));
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                                   DiskEntry->LayoutBuffer,
                                   BufferSize,
                                   DiskEntry->LayoutBuffer,
                                   BufferSize);
    NtClose(FileHandle);

    /*
     * IOCTL_DISK_SET_DRIVE_LAYOUT calls IoWritePartitionTable(), which converts
     * DiskEntry->LayoutBuffer->PartitionCount into a partition *table* count,
     * where such a table is expected to enumerate up to 4 partitions:
     * partition *table* count == ROUND_UP(PartitionCount, 4) / 4 .
     * Due to this we need to restore the original PartitionCount number.
     */
    DiskEntry->LayoutBuffer->PartitionCount = PartitionCount;

    /* Check whether the IOCTL_DISK_SET_DRIVE_LAYOUT call succeeded */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IOCTL_DISK_SET_DRIVE_LAYOUT failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Update the partition numbers */

    /* Update the primary partition table */
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned)
        {
            ASSERT(PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED);
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex];
            PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
        }
    }

    /* Update the logical partition table */
    for (ListEntry = DiskEntry->LogicalPartListHead.Flink;
         ListEntry != &DiskEntry->LogicalPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned)
        {
            ASSERT(PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED);
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex];
            PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
        }
    }

    /* The layout has been successfully updated, the disk is not dirty anymore */
    DiskEntry->Dirty = FALSE;

    return Status;
}


NTSTATUS
WriteGptPartitions(
    _In_ PDISKENTRY DiskEntry)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE FileHandle;
    IO_STATUS_BLOCK Iosb;
    ULONG BufferSize;
    WCHAR DstPath[MAX_PATH];

    DPRINT("WriteGptPartitions() Disk: %lu\n", DiskEntry->DiskNumber);

    /* If the disk is not dirty, there is nothing to do */
    if (!DiskEntry->Dirty)
        return STATUS_SUCCESS;

    StringCchPrintfW(DstPath, ARRAYSIZE(DstPath),
                     L"\\Device\\Harddisk%lu\\Partition0",
                     DiskEntry->DiskNumber);
    RtlInitUnicodeString(&Name, DstPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &Iosb,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        return Status;
    }

    //
    // FIXME: We first *MUST* use IOCTL_DISK_CREATE_DISK to initialize
    // the disk in MBR or GPT format in case the disk was not initialized!!
    // For this we must ask the user which format to use.
    //

    /* Set the new disk layout and retrieve its updated version with possibly modified partition numbers */
    BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                 ((DiskEntry->LayoutBuffer->PartitionCount - 1) * sizeof(PARTITION_INFORMATION_EX));
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                                   DiskEntry->LayoutBuffer,
                                   BufferSize,
                                   DiskEntry->LayoutBuffer,
                                   BufferSize);
    NtClose(FileHandle);

    /* Check whether the IOCTL_DISK_SET_DRIVE_LAYOUT_EX call succeeded */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IOCTL_DISK_SET_DRIVE_LAYOUT_EX failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* The layout has been successfully updated, the disk is not dirty anymore */
    DiskEntry->Dirty = FALSE;

    return Status;
}


static
BOOLEAN
IsEmptyLayoutEntry(
    IN PPARTITION_INFORMATION_EX PartitionInfo)
{
    if (PartitionInfo->StartingOffset.QuadPart == 0 &&
        PartitionInfo->PartitionLength.QuadPart == 0)
    {
        return TRUE;
    }

    return FALSE;
}


static
BOOLEAN
IsSamePrimaryLayoutEntry(
    IN PPARTITION_INFORMATION_EX PartitionInfo,
    IN PDISKENTRY DiskEntry,
    IN PPARTENTRY PartEntry)
{
    if ((PartitionInfo->StartingOffset.QuadPart == PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector) &&
        (PartitionInfo->PartitionLength.QuadPart == PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector))
    {
        return TRUE;
    }

    return FALSE;
}


ULONG
GetPrimaryPartitionCount(
    _In_ PDISKENTRY DiskEntry)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    ULONG Count = 0;

    for (Entry = DiskEntry->PrimaryPartListHead.Flink;
         Entry != &DiskEntry->PrimaryPartListHead;
         Entry = Entry->Flink)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned)
            Count++;
    }

    return Count;
}


static
ULONG
GetLogicalPartitionCount(
    _In_ PDISKENTRY DiskEntry)
{
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    ULONG Count = 0;

    for (ListEntry = DiskEntry->LogicalPartListHead.Flink;
         ListEntry != &DiskEntry->LogicalPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned)
            Count++;
    }

    return Count;
}


static
BOOLEAN
ReAllocateLayoutBuffer(
    _In_ PDISKENTRY DiskEntry)
{
    PDRIVE_LAYOUT_INFORMATION_EX NewLayoutBuffer;
    ULONG NewPartitionCount;
    ULONG CurrentPartitionCount = 0;
    ULONG LayoutBufferSize;
    ULONG i;

    DPRINT1("ReAllocateLayoutBuffer()\n");

    NewPartitionCount = 4 + GetLogicalPartitionCount(DiskEntry) * 4;

    if (DiskEntry->LayoutBuffer)
        CurrentPartitionCount = DiskEntry->LayoutBuffer->PartitionCount;

    DPRINT1("CurrentPartitionCount: %lu ; NewPartitionCount: %lu\n",
            CurrentPartitionCount, NewPartitionCount);

    if (CurrentPartitionCount == NewPartitionCount)
        return TRUE;

    LayoutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                       ((NewPartitionCount - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION_EX));
    NewLayoutBuffer = RtlReAllocateHeap(RtlGetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        DiskEntry->LayoutBuffer,
                                        LayoutBufferSize);
    if (NewLayoutBuffer == NULL)
    {
        DPRINT1("Failed to allocate the new layout buffer (size: %lu)\n", LayoutBufferSize);
        return FALSE;
    }

    NewLayoutBuffer->PartitionCount = NewPartitionCount;

    /* If the layout buffer grows, make sure the new (empty) entries are written to the disk */
    if (NewPartitionCount > CurrentPartitionCount)
    {
        for (i = CurrentPartitionCount; i < NewPartitionCount; i++)
        {
            NewLayoutBuffer->PartitionEntry[i].RewritePartition = TRUE;
        }
    }

    DiskEntry->LayoutBuffer = NewLayoutBuffer;

    return TRUE;
}


VOID
UpdateMbrDiskLayout(
    _In_ PDISKENTRY DiskEntry)
{
    PPARTITION_INFORMATION_EX PartitionInfo;
    PPARTITION_INFORMATION_EX LinkInfo = NULL;
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    LARGE_INTEGER HiddenSectors64;
    ULONG Index;
    ULONG PartitionNumber = 1;

    DPRINT("UpdateMbrDiskLayout()\n");

    /* Resize the layout buffer if necessary */
    if (ReAllocateLayoutBuffer(DiskEntry) == FALSE)
    {
        DPRINT("ReAllocateLayoutBuffer() failed.\n");
        return;
    }

    DiskEntry->LayoutBuffer->PartitionStyle = PARTITION_STYLE_MBR;

    /* Update the primary partition table */
    Index = 0;
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned)
        {
            ASSERT(PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED);

            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];
            PartEntry->PartitionIndex = Index;

            /* Reset the current partition number only for newly-created (unmounted) partitions */
            if (PartEntry->New)
                PartEntry->PartitionNumber = 0;

            PartEntry->OnDiskPartitionNumber = (!IsContainerPartition(PartEntry->Mbr.PartitionType) ? PartitionNumber : 0);

            if (!IsSamePrimaryLayoutEntry(PartitionInfo, DiskEntry, PartEntry))
            {
                DPRINT1("Updating primary partition entry %lu\n", Index);

                PartitionInfo->PartitionStyle = PARTITION_STYLE_MBR;
                PartitionInfo->StartingOffset.QuadPart = PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector;
                PartitionInfo->PartitionLength.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
                PartitionInfo->Mbr.HiddenSectors = PartEntry->StartSector.LowPart;
                PartitionInfo->PartitionNumber = PartEntry->PartitionNumber;
                PartitionInfo->Mbr.PartitionType = PartEntry->Mbr.PartitionType;
                PartitionInfo->Mbr.BootIndicator = PartEntry->Mbr.BootIndicator;
                PartitionInfo->Mbr.RecognizedPartition = IsRecognizedPartition(PartEntry->Mbr.PartitionType);
                PartitionInfo->RewritePartition = TRUE;
            }

            if (!IsContainerPartition(PartEntry->Mbr.PartitionType))
                PartitionNumber++;

            Index++;
        }
    }

    ASSERT(Index <= 4);

    /* Update the logical partition table */
    Index = 4;
    for (ListEntry = DiskEntry->LogicalPartListHead.Flink;
         ListEntry != &DiskEntry->LogicalPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned)
        {
            ASSERT(PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED);

            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];
            PartEntry->PartitionIndex = Index;

            /* Reset the current partition number only for newly-created (unmounted) partitions */
            if (PartEntry->New)
                PartEntry->PartitionNumber = 0;

            PartEntry->OnDiskPartitionNumber = PartitionNumber;

            DPRINT1("Updating logical partition entry %lu\n", Index);

            PartitionInfo->PartitionStyle = PARTITION_STYLE_MBR;
            PartitionInfo->StartingOffset.QuadPart = PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector;
            PartitionInfo->PartitionLength.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
            PartitionInfo->Mbr.HiddenSectors = DiskEntry->SectorAlignment;
            PartitionInfo->PartitionNumber = PartEntry->PartitionNumber;
            PartitionInfo->Mbr.PartitionType = PartEntry->Mbr.PartitionType;
            PartitionInfo->Mbr.BootIndicator = FALSE;
            PartitionInfo->Mbr.RecognizedPartition = IsRecognizedPartition(PartEntry->Mbr.PartitionType);
            PartitionInfo->RewritePartition = TRUE;

            /* Fill the link entry of the previous partition entry */
            if (LinkInfo != NULL)
            {
                LinkInfo->PartitionStyle = PARTITION_STYLE_MBR;
                LinkInfo->StartingOffset.QuadPart = (PartEntry->StartSector.QuadPart - DiskEntry->SectorAlignment) * DiskEntry->BytesPerSector;
                LinkInfo->PartitionLength.QuadPart = (PartEntry->StartSector.QuadPart + DiskEntry->SectorAlignment) * DiskEntry->BytesPerSector;
                HiddenSectors64.QuadPart = PartEntry->StartSector.QuadPart - DiskEntry->SectorAlignment - DiskEntry->ExtendedPartition->StartSector.QuadPart;
                LinkInfo->Mbr.HiddenSectors = HiddenSectors64.LowPart;
                LinkInfo->PartitionNumber = 0;
                LinkInfo->Mbr.PartitionType = PARTITION_EXTENDED;
                LinkInfo->Mbr.BootIndicator = FALSE;
                LinkInfo->Mbr.RecognizedPartition = FALSE;
                LinkInfo->RewritePartition = TRUE;
            }

            /* Save a pointer to the link entry of the current partition entry */
            LinkInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index + 1];

            PartitionNumber++;
            Index += 4;
        }
    }

    /* Wipe unused primary partition entries */
    for (Index = GetPrimaryPartitionCount(DiskEntry); Index < 4; Index++)
    {
        DPRINT1("Primary partition entry %lu\n", Index);

        PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];

        if (!IsEmptyLayoutEntry(PartitionInfo))
        {
            DPRINT1("Wiping primary partition entry %lu\n", Index);

            PartitionInfo->PartitionStyle = PARTITION_STYLE_MBR;
            PartitionInfo->StartingOffset.QuadPart = 0;
            PartitionInfo->PartitionLength.QuadPart = 0;
            PartitionInfo->Mbr.HiddenSectors = 0;
            PartitionInfo->PartitionNumber = 0;
            PartitionInfo->Mbr.PartitionType = PARTITION_ENTRY_UNUSED;
            PartitionInfo->Mbr.BootIndicator = FALSE;
            PartitionInfo->Mbr.RecognizedPartition = FALSE;
            PartitionInfo->RewritePartition = TRUE;
        }
    }

    /* Wipe unused logical partition entries */
    for (Index = 4; Index < DiskEntry->LayoutBuffer->PartitionCount; Index++)
    {
        if (Index % 4 >= 2)
        {
            DPRINT1("Logical partition entry %lu\n", Index);

            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];

            if (!IsEmptyLayoutEntry(PartitionInfo))
            {
                DPRINT1("Wiping partition entry %lu\n", Index);

                PartitionInfo->PartitionStyle = PARTITION_STYLE_MBR;
                PartitionInfo->StartingOffset.QuadPart = 0;
                PartitionInfo->PartitionLength.QuadPart = 0;
                PartitionInfo->Mbr.HiddenSectors = 0;
                PartitionInfo->PartitionNumber = 0;
                PartitionInfo->Mbr.PartitionType = PARTITION_ENTRY_UNUSED;
                PartitionInfo->Mbr.BootIndicator = FALSE;
                PartitionInfo->Mbr.RecognizedPartition = FALSE;
                PartitionInfo->RewritePartition = TRUE;
            }
        }
    }

    DiskEntry->Dirty = TRUE;
}


VOID
UpdateGptDiskLayout(
    _In_ PDISKENTRY DiskEntry,
    _In_ BOOL DeleteEntry)
{
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    PPARTITION_INFORMATION_EX PartitionInfo;
    ULONG Count, Index;

    DPRINT("UpdateGptDiskLayout()\n");

    /* Count used partition entries */
    Count = 0;
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (!IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
            Count++;
    }

    DPRINT("Used partition entries: %lu\n", Count);

    if (DeleteEntry)
        Count++;

    /* Reallocate the layout buffer */
    if (Count != DiskEntry->LayoutBuffer->PartitionCount)
    {
        PDRIVE_LAYOUT_INFORMATION_EX NewLayoutBuffer;
        ULONG NewLayoutBufferSize;

        NewLayoutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                              ((Count - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION_EX));
        NewLayoutBuffer = RtlReAllocateHeap(RtlGetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            DiskEntry->LayoutBuffer,
                                            NewLayoutBufferSize);
        if (NewLayoutBuffer == NULL)
        {
            DPRINT1("Failed to allocate the new layout buffer (size: %lu)\n", NewLayoutBufferSize);
            return;
        }

        NewLayoutBuffer->PartitionCount = Count;
        DiskEntry->LayoutBuffer = NewLayoutBuffer;
    }

    /* Fill the new layout buffer */
    Index = 0;
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (!IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID))
        {
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];
            PartEntry->PartitionIndex = Index;

            DPRINT("Updating primary partition entry %lu\n", Index);
            PartitionInfo->PartitionStyle = PARTITION_STYLE_GPT;
            PartitionInfo->StartingOffset.QuadPart = PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector;
            PartitionInfo->PartitionLength.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
            PartitionInfo->PartitionNumber = Index + 1;
            PartitionInfo->RewritePartition = TRUE;
            CopyMemory(&PartitionInfo->Gpt.PartitionType, &PartEntry->Gpt.PartitionType, sizeof(GUID));
            CopyMemory(&PartitionInfo->Gpt.PartitionId, &PartEntry->Gpt.PartitionId, sizeof(GUID));
            PartitionInfo->Gpt.Attributes = PartEntry->Gpt.Attributes;
            ZeroMemory(&PartitionInfo->Gpt.Name, 36 * sizeof(WCHAR)); /* ??? */

            PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;

            Index++;
        }
    }

    if (DeleteEntry)
    {
        PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];
        ZeroMemory(PartitionInfo, sizeof(PARTITION_INFORMATION_EX));
        PartitionInfo->RewritePartition = TRUE;
    }
}


PPARTENTRY
GetPrevUnpartitionedEntry(
    _In_ PPARTENTRY PartEntry)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    PPARTENTRY PrevPartEntry;
    PLIST_ENTRY ListHead;

    if (PartEntry->LogicalPartition)
        ListHead = &DiskEntry->LogicalPartListHead;
    else
        ListHead = &DiskEntry->PrimaryPartListHead;

    if (PartEntry->ListEntry.Blink != ListHead)
    {
        PrevPartEntry = CONTAINING_RECORD(PartEntry->ListEntry.Blink,
                                          PARTENTRY,
                                          ListEntry);
        if (!PrevPartEntry->IsPartitioned)
        {
            ASSERT(PrevPartEntry->Mbr.PartitionType == PARTITION_ENTRY_UNUSED);
            return PrevPartEntry;
        }
    }

    return NULL;
}


PPARTENTRY
GetNextUnpartitionedEntry(
    _In_ PPARTENTRY PartEntry)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    PPARTENTRY NextPartEntry;
    PLIST_ENTRY ListHead;

    if (PartEntry->LogicalPartition)
        ListHead = &DiskEntry->LogicalPartListHead;
    else
        ListHead = &DiskEntry->PrimaryPartListHead;

    if (PartEntry->ListEntry.Flink != ListHead)
    {
        NextPartEntry = CONTAINING_RECORD(PartEntry->ListEntry.Flink,
                                          PARTENTRY,
                                          ListEntry);
        if (!NextPartEntry->IsPartitioned)
        {
            ASSERT(NextPartEntry->Mbr.PartitionType == PARTITION_ENTRY_UNUSED);
            return NextPartEntry;
        }
    }

    return NULL;
}


NTSTATUS
DismountVolume(
    _In_ PPARTENTRY PartEntry)
{
    NTSTATUS Status;
    NTSTATUS LockStatus;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle;
    WCHAR Buffer[MAX_PATH];

    /* Check whether the partition is valid and was mounted by the system */
    if (PartEntry->DiskEntry->PartitionStyle == PARTITION_STYLE_MBR)
    {
        if (!PartEntry->IsPartitioned ||
            IsContainerPartition(PartEntry->Mbr.PartitionType)   ||
            !IsRecognizedPartition(PartEntry->Mbr.PartitionType) ||
            PartEntry->FormatState == UnknownFormat ||
            // NOTE: If FormatState == Unformatted but *FileSystem != 0 this means
            // it has been usually mounted with RawFS and thus needs to be dismounted.
/*            !*PartEntry->FileSystem || */
            PartEntry->PartitionNumber == 0)
        {
            /* The partition is not mounted, so just return success */
            return STATUS_SUCCESS;
        }

        ASSERT(PartEntry->Mbr.PartitionType != PARTITION_ENTRY_UNUSED);
    }
    else if (PartEntry->DiskEntry->PartitionStyle == PARTITION_STYLE_GPT)
    {
        if (!PartEntry->IsPartitioned ||
            IsEqualGUID(&PartEntry->Gpt.PartitionType, &PARTITION_ENTRY_UNUSED_GUID) ||
            (PartEntry->FormatState == UnknownFormat))
        {
            /* The partition is not mounted, so just return success */
            return STATUS_SUCCESS;
        }
    }

    /* Open the volume */
    StringCchPrintfW(Buffer, ARRAYSIZE(Buffer),
                     L"\\Device\\Harddisk%lu\\Partition%lu",
                     PartEntry->DiskEntry->DiskNumber,
                     PartEntry->PartitionNumber);
    RtlInitUnicodeString(&Name, Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&PartitionHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ERROR: Cannot open volume %wZ for dismounting! (Status 0x%lx)\n", &Name, Status);
        return Status;
    }

    /* Lock the volume */
    LockStatus = NtFsControlFile(PartitionHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_LOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("WARNING: Failed to lock volume! Operations may fail! (Status 0x%lx)\n", LockStatus);
    }

    /* Dismount the volume */
    Status = NtFsControlFile(PartitionHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_DISMOUNT_VOLUME,
                             NULL,
                             0,
                             NULL,
                             0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to unmount volume (Status 0x%lx)\n", Status);
    }

    /* Unlock the volume */
    LockStatus = NtFsControlFile(PartitionHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_UNLOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Failed to unlock volume (Status 0x%lx)\n", LockStatus);
    }

    /* Close the volume */
    NtClose(PartitionHandle);

    return Status;
}


PVOLENTRY
GetVolumeFromPartition(
    _In_ PPARTENTRY PartEntry)
{
    PLIST_ENTRY Entry;
    PVOLENTRY VolumeEntry;
    ULONG i;

    if ((PartEntry == NULL) ||
        (PartEntry->DiskEntry == NULL))
        return NULL;

    Entry = VolumeListHead.Flink;
    while (Entry != &VolumeListHead)
    {
        VolumeEntry = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        if (VolumeEntry->pExtents == NULL)
            return NULL;

        for (i = 0; i < VolumeEntry->pExtents->NumberOfDiskExtents; i++)
        {
            if (VolumeEntry->pExtents->Extents[i].DiskNumber == PartEntry->DiskEntry->DiskNumber)
            {
                if ((VolumeEntry->pExtents->Extents[i].StartingOffset.QuadPart == PartEntry->StartSector.QuadPart * PartEntry->DiskEntry->BytesPerSector) &&
                    (VolumeEntry->pExtents->Extents[i].ExtentLength.QuadPart == PartEntry->SectorCount.QuadPart * PartEntry->DiskEntry->BytesPerSector))
                    return VolumeEntry;
            }
        }

        Entry = Entry->Flink;
    }

    return NULL;
}


VOID
RemoveVolume(
    _In_ PVOLENTRY VolumeEntry)
{
    if (VolumeEntry == NULL)
        return;

    if (VolumeEntry == CurrentVolume)
        CurrentVolume = NULL;

    RemoveEntryList(&VolumeEntry->ListEntry);

    if (VolumeEntry->pszLabel)
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry->pszLabel);

    if (VolumeEntry->pszFilesystem)
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry->pszFilesystem);

    if (VolumeEntry->pExtents)
        RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry->pExtents);

    /* Release disk entry */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumeEntry);
}

/* EOF */
