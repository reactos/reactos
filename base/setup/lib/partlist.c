/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/partlist.c
 * PURPOSE:         Partition list functions
 * PROGRAMMERS:     Eric Kohl
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include "precomp.h"
#include <ntddscsi.h>

#include "partlist.h"
#include "fsutil.h"

#define NDEBUG
#include <debug.h>

//#define DUMP_PARTITION_TABLE

#include <pshpack1.h>

typedef struct _REG_DISK_MOUNT_INFO
{
    ULONG Signature;
    LARGE_INTEGER StartingOffset;
} REG_DISK_MOUNT_INFO, *PREG_DISK_MOUNT_INFO;

#include <poppack.h>


/* FUNCTIONS ****************************************************************/

#ifdef DUMP_PARTITION_TABLE
static
VOID
DumpPartitionTable(
    PDISKENTRY DiskEntry)
{
    PPARTITION_INFORMATION PartitionInfo;
    ULONG i;

    DbgPrint("\n");
    DbgPrint("Index  Start         Length        Hidden      Nr  Type  Boot  RW\n");
    DbgPrint("-----  ------------  ------------  ----------  --  ----  ----  --\n");

    for (i = 0; i < DiskEntry->LayoutBuffer->PartitionCount; i++)
    {
        PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[i];
        DbgPrint("  %3lu  %12I64u  %12I64u  %10lu  %2lu    %2x     %c   %c\n",
                 i,
                 PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector,
                 PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector,
                 PartitionInfo->HiddenSectors,
                 PartitionInfo->PartitionNumber,
                 PartitionInfo->PartitionType,
                 PartitionInfo->BootIndicator ? '*': ' ',
                 PartitionInfo->RewritePartition ? 'Y': 'N');
    }

    DbgPrint("\n");
}
#endif


ULONGLONG
AlignDown(
    IN ULONGLONG Value,
    IN ULONG Alignment)
{
    ULONGLONG Temp;

    Temp = Value / Alignment;

    return Temp * Alignment;
}

ULONGLONG
AlignUp(
    IN ULONGLONG Value,
    IN ULONG Alignment)
{
    ULONGLONG Temp, Result;

    Temp = Value / Alignment;

    Result = Temp * Alignment;
    if (Value % Alignment)
        Result += Alignment;

    return Result;
}

ULONGLONG
RoundingDivide(
   IN ULONGLONG Dividend,
   IN ULONGLONG Divisor)
{
    return (Dividend + Divisor / 2) / Divisor;
}


static
VOID
GetDriverName(
    IN PDISKENTRY DiskEntry)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    WCHAR KeyName[32];
    NTSTATUS Status;

    RtlInitUnicodeString(&DiskEntry->DriverName, NULL);

    StringCchPrintfW(KeyName, ARRAYSIZE(KeyName),
                     L"\\Scsi\\Scsi Port %hu",
                     DiskEntry->Port);

    RtlZeroMemory(&QueryTable, sizeof(QueryTable));

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
VOID
AssignDriveLetters(
    IN PPARTLIST List)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PLIST_ENTRY Entry1;
    PLIST_ENTRY Entry2;
    CHAR Letter;

    Letter = 'C';

    /* Assign drive letters to primary partitions */
    Entry1 = List->DiskListHead.Flink;
    while (Entry1 != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry1, DISKENTRY, ListEntry);

        Entry2 = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry2 != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);

            PartEntry->DriveLetter = 0;

            if (PartEntry->IsPartitioned &&
                !IsContainerPartition(PartEntry->PartitionType))
            {
                if (IsRecognizedPartition(PartEntry->PartitionType) ||
                    (PartEntry->PartitionType == PARTITION_ENTRY_UNUSED &&
                     PartEntry->SectorCount.QuadPart != 0LL))
                {
                    if (Letter <= 'Z')
                    {
                        PartEntry->DriveLetter = Letter;
                        Letter++;
                    }
                }
            }

            Entry2 = Entry2->Flink;
        }

        Entry1 = Entry1->Flink;
    }

    /* Assign drive letters to logical drives */
    Entry1 = List->DiskListHead.Flink;
    while (Entry1 != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry1, DISKENTRY, ListEntry);

        Entry2 = DiskEntry->LogicalPartListHead.Flink;
        while (Entry2 != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);

            PartEntry->DriveLetter = 0;

            if (PartEntry->IsPartitioned)
            {
                if (IsRecognizedPartition(PartEntry->PartitionType) ||
                    (PartEntry->PartitionType == PARTITION_ENTRY_UNUSED &&
                     PartEntry->SectorCount.QuadPart != 0LL))
                {
                    if (Letter <= 'Z')
                    {
                        PartEntry->DriveLetter = Letter;
                        Letter++;
                    }
                }
            }

            Entry2 = Entry2->Flink;
        }

        Entry1 = Entry1->Flink;
    }
}

static NTSTATUS
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

static NTSTATUS
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

static NTSTATUS
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
            FullResourceDescriptor->PartialResourceList.PartialDescriptors[i].u.DeviceSpecificData.DataSize % sizeof(CM_INT13_DRIVE_PARAMETER) != 0)
            continue;

        *Int13Drives = (CM_INT13_DRIVE_PARAMETER*)RtlAllocateHeap(ProcessHeap, 0,
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

static VOID
EnumerateBiosDiskEntries(
    IN PPARTLIST PartList)
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
    while (TRUE)
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
            while (TRUE)
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
                    RtlFreeHeap(ProcessHeap, 0, Int13Drives);
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
                    while (TRUE)
                    {
                        BiosDiskEntry = (BIOSDISKENTRY*)RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, sizeof(BIOSDISKENTRY));
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
                            RtlFreeHeap(ProcessHeap, 0, BiosDiskEntry);
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

                        InsertTailList(&PartList->BiosDiskListHead, &BiosDiskEntry->ListEntry);

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

                RtlFreeHeap(ProcessHeap, 0, Int13Drives);
                return;
            }
        }

        AdapterCount++;
    }

    RtlFreeHeap(ProcessHeap, 0, Int13Drives);
}

static
VOID
AddPartitionToDisk(
    IN ULONG DiskNumber,
    IN PDISKENTRY DiskEntry,
    IN ULONG PartitionIndex,
    IN BOOLEAN LogicalPartition)
{
    PPARTITION_INFORMATION PartitionInfo;
    PPARTENTRY PartEntry;

    PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[PartitionIndex];

    if (PartitionInfo->PartitionType == PARTITION_ENTRY_UNUSED ||
        (LogicalPartition == TRUE && IsContainerPartition(PartitionInfo->PartitionType)))
    {
        return;
    }

    PartEntry = RtlAllocateHeap(ProcessHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(PARTENTRY));
    if (PartEntry == NULL)
        return;

    PartEntry->DiskEntry = DiskEntry;

    PartEntry->StartSector.QuadPart = (ULONGLONG)PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector;
    PartEntry->SectorCount.QuadPart = (ULONGLONG)PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector;

    PartEntry->BootIndicator = PartitionInfo->BootIndicator;
    PartEntry->PartitionType = PartitionInfo->PartitionType;
    PartEntry->HiddenSectors = PartitionInfo->HiddenSectors;

    PartEntry->LogicalPartition = LogicalPartition;
    PartEntry->IsPartitioned = TRUE;
    PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
    PartEntry->PartitionIndex = PartitionIndex;

    if (IsContainerPartition(PartEntry->PartitionType))
    {
        PartEntry->FormatState = Unformatted;
        PartEntry->FileSystem  = NULL;

        if (LogicalPartition == FALSE && DiskEntry->ExtendedPartition == NULL)
            DiskEntry->ExtendedPartition = PartEntry;
    }
    else if (IsRecognizedPartition(PartEntry->PartitionType))
    {
        PartEntry->FileSystem = GetFileSystem(PartEntry);
        if (PartEntry->FileSystem)
            PartEntry->FormatState = Preformatted;
        else
            PartEntry->FormatState = Unformatted;
        // PartEntry->FormatState = UnknownFormat;
    }
    else
    {
        /* Unknown partition, hence unknown partition format (may or may not be actually formatted) */
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
ScanForUnpartitionedDiskSpace(
    IN PDISKENTRY DiskEntry)
{
    ULONGLONG LastStartSector;
    ULONGLONG LastSectorCount;
    ULONGLONG LastUnusedSectorCount;
    PPARTENTRY PartEntry;
    PPARTENTRY NewPartEntry;
    PLIST_ENTRY Entry;

    DPRINT("ScanForUnpartitionedDiskSpace()\n");

    if (IsListEmpty(&DiskEntry->PrimaryPartListHead))
    {
        DPRINT1("No primary partition!\n");

        /* Create a partition entry that represents the empty disk */
        NewPartEntry = RtlAllocateHeap(ProcessHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(PARTENTRY));
        if (NewPartEntry == NULL)
            return;

        NewPartEntry->DiskEntry = DiskEntry;

        NewPartEntry->IsPartitioned = FALSE;
        NewPartEntry->StartSector.QuadPart = (ULONGLONG)DiskEntry->SectorAlignment;
        NewPartEntry->SectorCount.QuadPart = AlignDown(DiskEntry->SectorCount.QuadPart, DiskEntry->SectorAlignment) -
                                             NewPartEntry->StartSector.QuadPart;

        DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

        NewPartEntry->FormatState = Unformatted;
        NewPartEntry->FileSystem  = NULL;

        InsertTailList(&DiskEntry->PrimaryPartListHead,
                       &NewPartEntry->ListEntry);

        return;
    }

    /* Start partition at head 1, cylinder 0 */
    LastStartSector = DiskEntry->SectorAlignment;
    LastSectorCount = 0ULL;
    LastUnusedSectorCount = 0ULL;

    Entry = DiskEntry->PrimaryPartListHead.Flink;
    while (Entry != &DiskEntry->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionType != PARTITION_ENTRY_UNUSED ||
            PartEntry->SectorCount.QuadPart != 0ULL)
        {
            LastUnusedSectorCount =
                PartEntry->StartSector.QuadPart - (LastStartSector + LastSectorCount);

            if (PartEntry->StartSector.QuadPart > (LastStartSector + LastSectorCount) &&
                LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
            {
                DPRINT("Unpartitioned disk space %I64u sectors\n", LastUnusedSectorCount);

                NewPartEntry = RtlAllocateHeap(ProcessHeap,
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
                NewPartEntry->FileSystem  = NULL;

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
    if ((LastStartSector + LastSectorCount) < DiskEntry->SectorCount.QuadPart)
    {
        LastUnusedSectorCount = AlignDown(DiskEntry->SectorCount.QuadPart - (LastStartSector + LastSectorCount), DiskEntry->SectorAlignment);

        if (LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
        {
            DPRINT("Unpartitioned disk space: %I64u sectors\n", LastUnusedSectorCount);

            NewPartEntry = RtlAllocateHeap(ProcessHeap,
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
            NewPartEntry->FileSystem  = NULL;

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

            /* Create a partition entry that represents the empty extended partition */
            NewPartEntry = RtlAllocateHeap(ProcessHeap,
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
            NewPartEntry->FileSystem  = NULL;

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

            if (PartEntry->PartitionType != PARTITION_ENTRY_UNUSED ||
                PartEntry->SectorCount.QuadPart != 0ULL)
            {
                LastUnusedSectorCount =
                    PartEntry->StartSector.QuadPart - (ULONGLONG)DiskEntry->SectorAlignment - (LastStartSector + LastSectorCount);

                if ((PartEntry->StartSector.QuadPart - (ULONGLONG)DiskEntry->SectorAlignment) > (LastStartSector + LastSectorCount) &&
                    LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
                {
                    DPRINT("Unpartitioned disk space %I64u sectors\n", LastUnusedSectorCount);

                    NewPartEntry = RtlAllocateHeap(ProcessHeap,
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
                    NewPartEntry->FileSystem  = NULL;

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
            LastUnusedSectorCount = AlignDown(DiskEntry->ExtendedPartition->StartSector.QuadPart + DiskEntry->ExtendedPartition->SectorCount.QuadPart - (LastStartSector + LastSectorCount), DiskEntry->SectorAlignment);

            if (LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
            {
                DPRINT("Unpartitioned disk space: %I64u sectors\n", LastUnusedSectorCount);

                NewPartEntry = RtlAllocateHeap(ProcessHeap,
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
                NewPartEntry->FileSystem  = NULL;

                /* Append the table to the list */
                InsertTailList(&DiskEntry->LogicalPartListHead,
                               &NewPartEntry->ListEntry);
            }
        }
    }

    DPRINT("ScanForUnpartitionedDiskSpace() done\n");
}

static
VOID
SetDiskSignature(
    IN PPARTLIST List,
    IN PDISKENTRY DiskEntry)
{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;
    PLIST_ENTRY Entry2;
    PDISKENTRY DiskEntry2;
    PUCHAR Buffer;

    Buffer = (PUCHAR)&DiskEntry->LayoutBuffer->Signature;

    while (TRUE)
    {
        NtQuerySystemTime(&SystemTime);
        RtlTimeToTimeFields(&SystemTime, &TimeFields);

        Buffer[0] = (UCHAR)(TimeFields.Year & 0xFF) + (UCHAR)(TimeFields.Hour & 0xFF);
        Buffer[1] = (UCHAR)(TimeFields.Year >> 8) + (UCHAR)(TimeFields.Minute & 0xFF);
        Buffer[2] = (UCHAR)(TimeFields.Month & 0xFF) + (UCHAR)(TimeFields.Second & 0xFF);
        Buffer[3] = (UCHAR)(TimeFields.Day & 0xFF) + (UCHAR)(TimeFields.Milliseconds & 0xFF);

        if (DiskEntry->LayoutBuffer->Signature == 0)
        {
            continue;
        }

        /* check if the signature already exist */
        /* FIXME:
         *   Check also signatures from disks, which are
         *   not visible (bootable) by the bios.
         */
        Entry2 = List->DiskListHead.Flink;
        while (Entry2 != &List->DiskListHead)
        {
            DiskEntry2 = CONTAINING_RECORD(Entry2, DISKENTRY, ListEntry);

            if (DiskEntry != DiskEntry2 &&
                DiskEntry->LayoutBuffer->Signature == DiskEntry2->LayoutBuffer->Signature)
                break;

            Entry2 = Entry2->Flink;
        }

        if (Entry2 == &List->DiskListHead)
            break;
    }
}

static
VOID
UpdateDiskSignatures(
    IN PPARTLIST List)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;

    /* Print partition lines */
    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->LayoutBuffer &&
            DiskEntry->LayoutBuffer->Signature == 0)
        {
            SetDiskSignature(List, DiskEntry);
            DiskEntry->LayoutBuffer->PartitionEntry[0].RewritePartition = TRUE;
        }

        Entry = Entry->Flink;
    }
}

static
VOID
AddDiskToList(
    IN HANDLE FileHandle,
    IN ULONG DiskNumber,
    IN PPARTLIST List)
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
    ULONG LayoutBufferSize;
    PDRIVE_LAYOUT_INFORMATION NewLayoutBuffer;

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
        return;

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
        return;

    /*
     * Check whether the disk is initialized, by looking at its MBR.
     * NOTE that this must be generalized to GPT disks as well!
     */

    Mbr = (PARTITION_SECTOR*)RtlAllocateHeap(ProcessHeap,
                                             0,
                                             DiskGeometry.BytesPerSector);
    if (Mbr == NULL)
        return;

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
        RtlFreeHeap(ProcessHeap, 0, Mbr);
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

    DiskEntry = RtlAllocateHeap(ProcessHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(DISKENTRY));
    if (DiskEntry == NULL)
    {
        return;
    }

//    DiskEntry->Checksum = Checksum;
//    DiskEntry->Signature = Signature;
    DiskEntry->BiosFound = FALSE;

    /* Check if this disk has a valid MBR */
    // FIXME: Check for the MBR signature as well, etc...
    if (Mbr->BootCode[0] == 0 && Mbr->BootCode[1] == 0)
        DiskEntry->NoMbr = TRUE;
    else
        DiskEntry->NoMbr = FALSE;

    /* Free the MBR sector buffer */
    RtlFreeHeap(ProcessHeap, 0, Mbr);


    ListEntry = List->BiosDiskListHead.Flink;
    while (ListEntry != &List->BiosDiskListHead)
    {
        BiosDiskEntry = CONTAINING_RECORD(ListEntry, BIOSDISKENTRY, ListEntry);
        /* FIXME:
         *   Compare the size from bios and the reported size from driver.
         *   If we have more than one disk with a zero or with the same signature
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
    DPRINT("TracksPerCylinder %lu\n", DiskEntry->TracksPerCylinder);
    DPRINT("SectorsPerTrack %lu\n", DiskEntry->SectorsPerTrack);
    DPRINT("BytesPerSector %lu\n", DiskEntry->BytesPerSector);

    DiskEntry->SectorCount.QuadPart = DiskGeometry.Cylinders.QuadPart *
                                      (ULONGLONG)DiskGeometry.TracksPerCylinder *
                                      (ULONGLONG)DiskGeometry.SectorsPerTrack;

    DiskEntry->SectorAlignment = DiskGeometry.SectorsPerTrack;
    DiskEntry->CylinderAlignment = DiskGeometry.TracksPerCylinder *
                                   DiskGeometry.SectorsPerTrack;

    DPRINT("SectorCount %I64u\n", DiskEntry->SectorCount.QuadPart);
    DPRINT("SectorAlignment %lu\n", DiskEntry->SectorAlignment);

    DiskEntry->DiskNumber = DiskNumber;
    DiskEntry->Port = ScsiAddress.PortNumber;
    DiskEntry->Bus = ScsiAddress.PathId;
    DiskEntry->Id = ScsiAddress.TargetId;

    GetDriverName(DiskEntry);
    /*
     * Actually it would be more correct somehow to use:
     *
     * OBJECT_NAME_INFORMATION NameInfo; // ObjectNameInfo;
     * ULONG ReturnedLength;
     *
     * Status = NtQueryObject(SomeHandleToTheDisk,
     *                        ObjectNameInformation,
     *                        &NameInfo,
     *                        sizeof(NameInfo),
     *                        &ReturnedLength);
     * etc...
     *
     * See examples in https://git.reactos.org/?p=reactos.git;a=blob;f=reactos/ntoskrnl/io/iomgr/error.c;hb=2f3a93ee9cec8322a86bf74b356f1ad83fc912dc#l267
     */

    InsertAscendingList(&List->DiskListHead, DiskEntry, DISKENTRY, ListEntry, DiskNumber);

    /* Allocate a layout buffer with 4 partition entries first */
    LayoutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                       ((4 - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION));
    DiskEntry->LayoutBuffer = RtlAllocateHeap(ProcessHeap,
                                              HEAP_ZERO_MEMORY,
                                              LayoutBufferSize);
    if (DiskEntry->LayoutBuffer == NULL)
    {
        DPRINT1("Failed to allocate the disk layout buffer!\n");
        return;
    }

    for (;;)
    {
        DPRINT1("Buffer size: %lu\n", LayoutBufferSize);
        Status = NtDeviceIoControlFile(FileHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &Iosb,
                                       IOCTL_DISK_GET_DRIVE_LAYOUT,
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

        LayoutBufferSize += 4 * sizeof(PARTITION_INFORMATION);
        NewLayoutBuffer = RtlReAllocateHeap(ProcessHeap,
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

    DPRINT1("PartitionCount: %lu\n", DiskEntry->LayoutBuffer->PartitionCount);

#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(DiskEntry);
#endif

    if (DiskEntry->LayoutBuffer->PartitionEntry[0].StartingOffset.QuadPart != 0 &&
        DiskEntry->LayoutBuffer->PartitionEntry[0].PartitionLength.QuadPart != 0 &&
        DiskEntry->LayoutBuffer->PartitionEntry[0].PartitionType != PARTITION_ENTRY_UNUSED)
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
            AddPartitionToDisk(DiskNumber, DiskEntry, i, FALSE);
        }

        for (i = 4; i < DiskEntry->LayoutBuffer->PartitionCount; i += 4)
        {
            AddPartitionToDisk(DiskNumber, DiskEntry, i, TRUE);
        }
    }

    ScanForUnpartitionedDiskSpace(DiskEntry);
}

PPARTLIST
CreatePartitionList(VOID)
{
    PPARTLIST List;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SYSTEM_DEVICE_INFORMATION Sdi;
    IO_STATUS_BLOCK Iosb;
    ULONG ReturnSize;
    NTSTATUS Status;
    ULONG DiskNumber;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING Name;
    HANDLE FileHandle;

    List = (PPARTLIST)RtlAllocateHeap(ProcessHeap,
                                      0,
                                      sizeof(PARTLIST));
    if (List == NULL)
        return NULL;

    List->CurrentDisk = NULL;
    List->CurrentPartition = NULL;

    List->SystemPartition = NULL;
    List->OriginalSystemPartition = NULL;

    InitializeListHead(&List->DiskListHead);
    InitializeListHead(&List->BiosDiskListHead);

    EnumerateBiosDiskEntries(List);

    Status = NtQuerySystemInformation(SystemDeviceInformation,
                                      &Sdi,
                                      sizeof(Sdi),
                                      &ReturnSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQuerySystemInformation() failed, Status 0x%08lx", Status);
        RtlFreeHeap(ProcessHeap, 0, List);
        return NULL;
    }

    for (DiskNumber = 0; DiskNumber < Sdi.NumberOfDisks; DiskNumber++)
    {
        StringCchPrintfW(Buffer, ARRAYSIZE(Buffer),
                         L"\\Device\\Harddisk%lu\\Partition0",
                         DiskNumber);
        RtlInitUnicodeString(&Name, Buffer);

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
            AddDiskToList(FileHandle, DiskNumber, List);
            NtClose(FileHandle);
        }
    }

    UpdateDiskSignatures(List);

    AssignDriveLetters(List);

    /* Search for first usable disk and partition */
    if (IsListEmpty(&List->DiskListHead))
    {
        List->CurrentDisk = NULL;
        List->CurrentPartition = NULL;
    }
    else
    {
        List->CurrentDisk = CONTAINING_RECORD(List->DiskListHead.Flink,
                                              DISKENTRY,
                                              ListEntry);

        if (IsListEmpty(&List->CurrentDisk->PrimaryPartListHead))
        {
            List->CurrentPartition = NULL;
        }
        else
        {
            List->CurrentPartition = CONTAINING_RECORD(List->CurrentDisk->PrimaryPartListHead.Flink,
                                                       PARTENTRY,
                                                       ListEntry);
        }
    }

    return List;
}

VOID
DestroyPartitionList(
    IN PPARTLIST List)
{
    PDISKENTRY DiskEntry;
    PBIOSDISKENTRY BiosDiskEntry;
    PPARTENTRY PartEntry;
    PLIST_ENTRY Entry;

    /* Release disk and partition info */
    while (!IsListEmpty(&List->DiskListHead))
    {
        Entry = RemoveHeadList(&List->DiskListHead);
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        /* Release driver name */
        RtlFreeUnicodeString(&DiskEntry->DriverName);

        /* Release primary partition list */
        while (!IsListEmpty(&DiskEntry->PrimaryPartListHead))
        {
            Entry = RemoveHeadList(&DiskEntry->PrimaryPartListHead);
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            RtlFreeHeap(ProcessHeap, 0, PartEntry);
        }

        /* Release logical partition list */
        while (!IsListEmpty(&DiskEntry->LogicalPartListHead))
        {
            Entry = RemoveHeadList(&DiskEntry->LogicalPartListHead);
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            RtlFreeHeap(ProcessHeap, 0, PartEntry);
        }

        /* Release layout buffer */
        if (DiskEntry->LayoutBuffer != NULL)
            RtlFreeHeap(ProcessHeap, 0, DiskEntry->LayoutBuffer);

        /* Release disk entry */
        RtlFreeHeap(ProcessHeap, 0, DiskEntry);
    }

    /* Release the bios disk info */
    while (!IsListEmpty(&List->BiosDiskListHead))
    {
        Entry = RemoveHeadList(&List->BiosDiskListHead);
        BiosDiskEntry = CONTAINING_RECORD(Entry, BIOSDISKENTRY, ListEntry);

        RtlFreeHeap(ProcessHeap, 0, BiosDiskEntry);
    }

    /* Release list head */
    RtlFreeHeap(ProcessHeap, 0, List);
}

PDISKENTRY
GetDiskByBiosNumber(
    IN PPARTLIST List,
    IN ULONG BiosDiskNumber)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
        Entry = Entry->Flink;

        if (DiskEntry->BiosDiskNumber == BiosDiskNumber)
        {
            /* Disk found */
            return DiskEntry;
        }
    }

    /* Disk not found, stop there */
    return NULL;
}

PDISKENTRY
GetDiskByNumber(
    IN PPARTLIST List,
    IN ULONG DiskNumber)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
        Entry = Entry->Flink;

        if (DiskEntry->DiskNumber == DiskNumber)
        {
            /* Disk found */
            return DiskEntry;
        }
    }

    /* Disk not found, stop there */
    return NULL;
}

PDISKENTRY
GetDiskBySCSI(
    IN PPARTLIST List,
    IN USHORT Port,
    IN USHORT Bus,
    IN USHORT Id)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
        Entry = Entry->Flink;

        if (DiskEntry->Port == Port &&
            DiskEntry->Bus  == Bus  &&
            DiskEntry->Id   == Id)
        {
            /* Disk found */
            return DiskEntry;
        }
    }

    /* Disk not found, stop there */
    return NULL;
}

PDISKENTRY
GetDiskBySignature(
    IN PPARTLIST List,
    IN ULONG Signature)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);
        Entry = Entry->Flink;

        if (DiskEntry->LayoutBuffer->Signature == Signature)
        {
            /* Disk found */
            return DiskEntry;
        }
    }

    /* Disk not found, stop there */
    return NULL;
}

PPARTENTRY
GetPartition(
    // IN PPARTLIST List,
    IN PDISKENTRY DiskEntry,
    IN ULONG PartitionNumber)
{
    PPARTENTRY PartEntry;
    PLIST_ENTRY Entry;

    /* Disk found, loop over the primary partitions first... */
    Entry = DiskEntry->PrimaryPartListHead.Flink;
    while (Entry != &DiskEntry->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
        Entry = Entry->Flink;

        if (PartEntry->PartitionNumber == PartitionNumber)
        {
            /* Partition found */
            return PartEntry;
        }
    }

    /* ... then over the logical partitions if needed */
    Entry = DiskEntry->LogicalPartListHead.Flink;
    while (Entry != &DiskEntry->LogicalPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
        Entry = Entry->Flink;

        if (PartEntry->PartitionNumber == PartitionNumber)
        {
            /* Partition found */
            return PartEntry;
        }
    }

    /* The partition was not found on the disk, stop there */
    return NULL;
}

BOOLEAN
GetDiskOrPartition(
    IN PPARTLIST List,
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber OPTIONAL,
    OUT PDISKENTRY* pDiskEntry,
    OUT PPARTENTRY* pPartEntry OPTIONAL)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry = NULL;

    /* Find the disk */
    DiskEntry = GetDiskByNumber(List, DiskNumber);
    if (!DiskEntry)
        return FALSE;

    /* If we have a partition (PartitionNumber != 0), find it */
    if (PartitionNumber != 0)
    {
        PartEntry = GetPartition(/*List,*/ DiskEntry, PartitionNumber);
        if (!PartEntry)
            return FALSE;
        ASSERT(PartEntry->DiskEntry == DiskEntry);
    }

    /* Return the disk (and optionally the partition) */
    *pDiskEntry = DiskEntry;
    if (pPartEntry) *pPartEntry = PartEntry;
    return TRUE;
}

//
// NOTE: Was introduced broken in r6258 by Casper
//
BOOLEAN
SelectPartition(
    IN PPARTLIST List,
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    DiskEntry = GetDiskByNumber(List, DiskNumber);
    if (!DiskEntry)
        return FALSE;

    PartEntry = GetPartition(/*List,*/ DiskEntry, PartitionNumber);
    if (!PartEntry)
        return FALSE;

    ASSERT(PartEntry->DiskEntry == DiskEntry);
    ASSERT(DiskEntry->DiskNumber == DiskNumber);
    ASSERT(PartEntry->PartitionNumber == PartitionNumber);

    List->CurrentDisk = DiskEntry;
    List->CurrentPartition = PartEntry;
    return TRUE;
}

PPARTENTRY
GetNextPartition(
    IN PPARTLIST List)
{
    PLIST_ENTRY DiskListEntry;
    PLIST_ENTRY PartListEntry;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    /* Fail, if no disks are available */
    if (IsListEmpty(&List->DiskListHead))
        return NULL;

    /* Check for next usable entry on current disk */
    if (List->CurrentPartition != NULL)
    {
        if (List->CurrentPartition->LogicalPartition)
        {
            /* Logical partition */

            PartListEntry = List->CurrentPartition->ListEntry.Flink;
            if (PartListEntry != &List->CurrentDisk->LogicalPartListHead)
            {
                /* Next logical partition */
                PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

                List->CurrentPartition = PartEntry;
                return List->CurrentPartition;
            }
            else
            {
                PartListEntry = List->CurrentDisk->ExtendedPartition->ListEntry.Flink;
                if (PartListEntry != &List->CurrentDisk->PrimaryPartListHead)
                {
                    PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

                    List->CurrentPartition = PartEntry;
                    return List->CurrentPartition;
                }
            }
        }
        else
        {
            /* Primary or extended partition */

            if (List->CurrentPartition->IsPartitioned == TRUE &&
                IsContainerPartition(List->CurrentPartition->PartitionType))
            {
                /* First logical partition */
                PartListEntry = List->CurrentDisk->LogicalPartListHead.Flink;
                if (PartListEntry != &List->CurrentDisk->LogicalPartListHead)
                {
                    PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

                    List->CurrentPartition = PartEntry;
                    return List->CurrentPartition;
                }
            }
            else
            {
                /* Next primary partition */
                PartListEntry = List->CurrentPartition->ListEntry.Flink;
                if (PartListEntry != &List->CurrentDisk->PrimaryPartListHead)
                {
                    PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

                    List->CurrentPartition = PartEntry;
                    return List->CurrentPartition;
                }
            }
        }
    }

    /* Search for the first partition entry on the next disk */
    DiskListEntry = List->CurrentDisk->ListEntry.Flink;
    while (DiskListEntry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(DiskListEntry, DISKENTRY, ListEntry);

        PartListEntry = DiskEntry->PrimaryPartListHead.Flink;
        if (PartListEntry != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

            List->CurrentDisk = DiskEntry;
            List->CurrentPartition = PartEntry;
            return List->CurrentPartition;
        }

        DiskListEntry = DiskListEntry->Flink;
    }

    return NULL;
}

PPARTENTRY
GetPrevPartition(
    IN PPARTLIST List)
{
    PLIST_ENTRY DiskListEntry;
    PLIST_ENTRY PartListEntry;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    /* Fail, if no disks are available */
    if (IsListEmpty(&List->DiskListHead))
        return NULL;

    /* Check for previous usable entry on current disk */
    if (List->CurrentPartition != NULL)
    {
        if (List->CurrentPartition->LogicalPartition)
        {
            /* Logical partition */
            PartListEntry = List->CurrentPartition->ListEntry.Blink;
            if (PartListEntry != &List->CurrentDisk->LogicalPartListHead)
            {
                /* Previous logical partition */
                PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
            }
            else
            {
                /* Extended partition */
                PartEntry = List->CurrentDisk->ExtendedPartition;
            }

            List->CurrentPartition = PartEntry;
            return List->CurrentPartition;
        }
        else
        {
            /* Primary or extended partition */

            PartListEntry = List->CurrentPartition->ListEntry.Blink;
            if (PartListEntry != &List->CurrentDisk->PrimaryPartListHead)
            {
                PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

                if (PartEntry->IsPartitioned == TRUE &&
                    IsContainerPartition(PartEntry->PartitionType))
                {
                    PartListEntry = List->CurrentDisk->LogicalPartListHead.Blink;
                    PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
                }

                List->CurrentPartition = PartEntry;
                return List->CurrentPartition;
            }
        }
    }

    /* Search for the last partition entry on the previous disk */
    DiskListEntry = List->CurrentDisk->ListEntry.Blink;
    while (DiskListEntry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(DiskListEntry, DISKENTRY, ListEntry);

        PartListEntry = DiskEntry->PrimaryPartListHead.Blink;
        if (PartListEntry != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

            if (PartEntry->IsPartitioned == TRUE &&
                IsContainerPartition(PartEntry->PartitionType))
            {
                PartListEntry = DiskEntry->LogicalPartListHead.Blink;
                if (PartListEntry != &DiskEntry->LogicalPartListHead)
                {
                    PartEntry = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

                    List->CurrentDisk = DiskEntry;
                    List->CurrentPartition = PartEntry;
                    return List->CurrentPartition;
                }
            }
            else
            {
                List->CurrentDisk = DiskEntry;
                List->CurrentPartition = PartEntry;
                return List->CurrentPartition;
            }
        }

        DiskListEntry = DiskListEntry->Blink;
    }

    return NULL;
}

// static
FORCEINLINE
BOOLEAN
IsEmptyLayoutEntry(
    IN PPARTITION_INFORMATION PartitionInfo)
{
    if (PartitionInfo->StartingOffset.QuadPart == 0 &&
        PartitionInfo->PartitionLength.QuadPart == 0)
        return TRUE;

    return FALSE;
}

// static
FORCEINLINE
BOOLEAN
IsSamePrimaryLayoutEntry(
    IN PPARTITION_INFORMATION PartitionInfo,
    IN PDISKENTRY DiskEntry,
    IN PPARTENTRY PartEntry)
{
    if (PartitionInfo->StartingOffset.QuadPart == PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector &&
        PartitionInfo->PartitionLength.QuadPart == PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector)
//        PartitionInfo->PartitionNumber = PartEntry->PartitionNumber &&
//        PartitionInfo->PartitionType == PartEntry->PartitionType
        return TRUE;

    return FALSE;
}

static
ULONG
GetPrimaryPartitionCount(
    IN PDISKENTRY DiskEntry)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    ULONG Count = 0;

    Entry = DiskEntry->PrimaryPartListHead.Flink;
    while (Entry != &DiskEntry->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned == TRUE)
            Count++;

        Entry = Entry->Flink;
    }

    return Count;
}

static
ULONG
GetLogicalPartitionCount(
    IN PDISKENTRY DiskEntry)
{
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    ULONG Count = 0;

    ListEntry = DiskEntry->LogicalPartListHead.Flink;
    while (ListEntry != &DiskEntry->LogicalPartListHead)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned)
            Count++;

        ListEntry = ListEntry->Flink;
    }

    return Count;
}

static
BOOLEAN
ReAllocateLayoutBuffer(
    IN PDISKENTRY DiskEntry)
{
    PDRIVE_LAYOUT_INFORMATION NewLayoutBuffer;
    ULONG NewPartitionCount;
    ULONG CurrentPartitionCount = 0;
    ULONG LayoutBufferSize;
    ULONG i;

    DPRINT1("ReAllocateLayoutBuffer()\n");

    NewPartitionCount = 4 + GetLogicalPartitionCount(DiskEntry) * 4;

    if (DiskEntry->LayoutBuffer)
        CurrentPartitionCount = DiskEntry->LayoutBuffer->PartitionCount;

    DPRINT1("CurrentPartitionCount: %lu    NewPartitionCount: %lu\n",
            CurrentPartitionCount, NewPartitionCount);

    if (CurrentPartitionCount == NewPartitionCount)
        return TRUE;

    LayoutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                       ((NewPartitionCount - ANYSIZE_ARRAY) * sizeof(PARTITION_INFORMATION));
    NewLayoutBuffer = RtlReAllocateHeap(ProcessHeap,
                                        HEAP_ZERO_MEMORY,
                                        DiskEntry->LayoutBuffer,
                                        LayoutBufferSize);
    if (NewLayoutBuffer == NULL)
    {
        DPRINT1("Failed to allocate the new layout buffer (size: %lu)\n", LayoutBufferSize);
        return FALSE;
    }

    /* If the layout buffer grows, make sure the new (empty) entries are written to the disk */
    if (NewPartitionCount > CurrentPartitionCount)
    {
        for (i = CurrentPartitionCount; i < NewPartitionCount; i++)
            NewLayoutBuffer->PartitionEntry[i].RewritePartition = TRUE;
    }

    DiskEntry->LayoutBuffer = NewLayoutBuffer;
    DiskEntry->LayoutBuffer->PartitionCount = NewPartitionCount;

    return TRUE;
}

static
VOID
UpdateDiskLayout(
    IN PDISKENTRY DiskEntry)
{
    PPARTITION_INFORMATION PartitionInfo;
    PPARTITION_INFORMATION LinkInfo = NULL;
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    LARGE_INTEGER HiddenSectors64;
    ULONG Index;
    ULONG PartitionNumber = 1;

    DPRINT1("UpdateDiskLayout()\n");

    /* Resize the layout buffer if necessary */
    if (ReAllocateLayoutBuffer(DiskEntry) == FALSE)
    {
        DPRINT("ReAllocateLayoutBuffer() failed.\n");
        return;
    }

    /* Update the primary partition table */
    Index = 0;
    ListEntry = DiskEntry->PrimaryPartListHead.Flink;
    while (ListEntry != &DiskEntry->PrimaryPartListHead)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned == TRUE)
        {
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];

            if (!IsSamePrimaryLayoutEntry(PartitionInfo, DiskEntry, PartEntry))
            {
                DPRINT1("Updating primary partition entry %lu\n", Index);

                PartitionInfo->StartingOffset.QuadPart = PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector;
                PartitionInfo->PartitionLength.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
                PartitionInfo->HiddenSectors = PartEntry->StartSector.LowPart;
                PartitionInfo->PartitionNumber = (!IsContainerPartition(PartEntry->PartitionType)) ? PartitionNumber : 0;
                PartitionInfo->PartitionType = PartEntry->PartitionType;
                PartitionInfo->BootIndicator = PartEntry->BootIndicator;
                PartitionInfo->RecognizedPartition = FALSE;
                PartitionInfo->RewritePartition = TRUE;
            }

            PartEntry->PartitionNumber = (!IsContainerPartition(PartEntry->PartitionType)) ? PartitionNumber : 0;
            PartEntry->PartitionIndex = Index;

            if (!IsContainerPartition(PartEntry->PartitionType))
                PartitionNumber++;

            Index++;
        }

        ListEntry = ListEntry->Flink;
    }

    /* Update the logical partition table */
    Index = 4;
    ListEntry = DiskEntry->LogicalPartListHead.Flink;
    while (ListEntry != &DiskEntry->LogicalPartListHead)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned)
        {
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];

            DPRINT1("Updating logical partition entry %lu\n", Index);

            PartitionInfo->StartingOffset.QuadPart = PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector;
            PartitionInfo->PartitionLength.QuadPart = PartEntry->SectorCount.QuadPart * DiskEntry->BytesPerSector;
            PartitionInfo->HiddenSectors = DiskEntry->SectorAlignment;
            PartitionInfo->PartitionNumber = PartitionNumber;
            PartitionInfo->PartitionType = PartEntry->PartitionType;
            PartitionInfo->BootIndicator = FALSE;
            PartitionInfo->RecognizedPartition = FALSE;
            PartitionInfo->RewritePartition = TRUE;

            PartEntry->PartitionNumber = PartitionNumber;
            PartEntry->PartitionIndex = Index;

            /* Fill the link entry of the previous partition entry */
            if (LinkInfo != NULL)
            {
                LinkInfo->StartingOffset.QuadPart = (PartEntry->StartSector.QuadPart - DiskEntry->SectorAlignment) * DiskEntry->BytesPerSector;
                LinkInfo->PartitionLength.QuadPart = (PartEntry->StartSector.QuadPart + DiskEntry->SectorAlignment) * DiskEntry->BytesPerSector;
                HiddenSectors64.QuadPart = PartEntry->StartSector.QuadPart - DiskEntry->SectorAlignment - DiskEntry->ExtendedPartition->StartSector.QuadPart;
                LinkInfo->HiddenSectors = HiddenSectors64.LowPart;
                LinkInfo->PartitionNumber = 0;
                LinkInfo->PartitionType = PARTITION_EXTENDED;
                LinkInfo->BootIndicator = FALSE;
                LinkInfo->RecognizedPartition = FALSE;
                LinkInfo->RewritePartition = TRUE;
            }

            /* Save a pointer to the link entry of the current partition entry */
            LinkInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index + 1];

            PartitionNumber++;
            Index += 4;
        }

        ListEntry = ListEntry->Flink;
    }

    /* Wipe unused primary partition entries */
    for (Index = GetPrimaryPartitionCount(DiskEntry); Index < 4; Index++)
    {
        DPRINT1("Primary partition entry %lu\n", Index);

        PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];

        if (!IsEmptyLayoutEntry(PartitionInfo))
        {
            DPRINT1("Wiping primary partition entry %lu\n", Index);

            PartitionInfo->StartingOffset.QuadPart = 0;
            PartitionInfo->PartitionLength.QuadPart = 0;
            PartitionInfo->HiddenSectors = 0;
            PartitionInfo->PartitionNumber = 0;
            PartitionInfo->PartitionType = PARTITION_ENTRY_UNUSED;
            PartitionInfo->BootIndicator = FALSE;
            PartitionInfo->RecognizedPartition = FALSE;
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

                PartitionInfo->StartingOffset.QuadPart = 0;
                PartitionInfo->PartitionLength.QuadPart = 0;
                PartitionInfo->HiddenSectors = 0;
                PartitionInfo->PartitionNumber = 0;
                PartitionInfo->PartitionType = PARTITION_ENTRY_UNUSED;
                PartitionInfo->BootIndicator = FALSE;
                PartitionInfo->RecognizedPartition = FALSE;
                PartitionInfo->RewritePartition = TRUE;
            }
        }
    }

#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(DiskEntry);
#endif
}

static
PPARTENTRY
GetPrevUnpartitionedEntry(
    IN PDISKENTRY DiskEntry,
    IN PPARTENTRY PartEntry)
{
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
        if (PrevPartEntry->IsPartitioned == FALSE)
            return PrevPartEntry;
    }

    return NULL;
}

static
PPARTENTRY
GetNextUnpartitionedEntry(
    IN PDISKENTRY DiskEntry,
    IN PPARTENTRY PartEntry)
{
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
        if (NextPartEntry->IsPartitioned == FALSE)
            return NextPartEntry;
    }

    return NULL;
}

VOID
CreatePrimaryPartition(
    IN PPARTLIST List,
    IN ULONGLONG SectorCount,
    IN BOOLEAN AutoCreate)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PPARTENTRY NewPartEntry;

    DPRINT1("CreatePrimaryPartition(%I64u)\n", SectorCount);

    if (List == NULL ||
        List->CurrentDisk == NULL ||
        List->CurrentPartition == NULL ||
        List->CurrentPartition->IsPartitioned == TRUE)
    {
        return;
    }

    DiskEntry = List->CurrentDisk;
    PartEntry = List->CurrentPartition;

    DPRINT1("Current partition sector count: %I64u\n", PartEntry->SectorCount.QuadPart);

    if (AutoCreate == TRUE ||
        AlignDown(PartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment) - PartEntry->StartSector.QuadPart == PartEntry->SectorCount.QuadPart)
    {
        DPRINT1("Convert existing partition entry\n");

        /* Convert current entry to 'new (unformatted)' */
        PartEntry->IsPartitioned = TRUE;
        PartEntry->PartitionType = PARTITION_ENTRY_UNUSED;
        PartEntry->FormatState = Unformatted;
        PartEntry->FileSystem  = NULL;
        PartEntry->AutoCreate = AutoCreate;
        PartEntry->New = TRUE;
        PartEntry->BootIndicator = FALSE;

        DPRINT1("First Sector: %I64u\n", PartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", PartEntry->StartSector.QuadPart + PartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", PartEntry->SectorCount.QuadPart);
    }
    else
    {
        DPRINT1("Add new partition entry\n");

        /* Insert and initialize a new partition entry */
        NewPartEntry = RtlAllocateHeap(ProcessHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(PARTENTRY));
        if (NewPartEntry == NULL)
            return;

        /* Insert the new entry into the list */
        InsertTailList(&PartEntry->ListEntry,
                       &NewPartEntry->ListEntry);

        NewPartEntry->DiskEntry = DiskEntry;

        NewPartEntry->IsPartitioned = TRUE;
        NewPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
        NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment) -
                                             NewPartEntry->StartSector.QuadPart;
        NewPartEntry->PartitionType = PARTITION_ENTRY_UNUSED;

        DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

        NewPartEntry->New = TRUE;
        NewPartEntry->FormatState = Unformatted;
        NewPartEntry->FileSystem  = NULL;
        NewPartEntry->BootIndicator = FALSE;

        PartEntry->StartSector.QuadPart = NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart;
        PartEntry->SectorCount.QuadPart -= (PartEntry->StartSector.QuadPart - NewPartEntry->StartSector.QuadPart);
    }

    UpdateDiskLayout(DiskEntry);

    DiskEntry->Dirty = TRUE;

    AssignDriveLetters(List);
}

static
VOID
AddLogicalDiskSpace(
    IN PDISKENTRY DiskEntry)
{
    PPARTENTRY NewPartEntry;

    DPRINT1("AddLogicalDiskSpace()\n");

    /* Create a partition entry that represents the empty space in the container partition */
    NewPartEntry = RtlAllocateHeap(ProcessHeap,
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
    NewPartEntry->FileSystem  = NULL;

    InsertTailList(&DiskEntry->LogicalPartListHead,
                   &NewPartEntry->ListEntry);
}

VOID
CreateExtendedPartition(
    IN PPARTLIST List,
    IN ULONGLONG SectorCount)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PPARTENTRY NewPartEntry;

    DPRINT1("CreateExtendedPartition(%I64u)\n", SectorCount);

    if (List == NULL ||
        List->CurrentDisk == NULL ||
        List->CurrentPartition == NULL ||
        List->CurrentPartition->IsPartitioned == TRUE)
    {
        return;
    }

    DiskEntry = List->CurrentDisk;
    PartEntry = List->CurrentPartition;

    DPRINT1("Current partition sector count: %I64u\n", PartEntry->SectorCount.QuadPart);

    if (AlignDown(PartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment) - PartEntry->StartSector.QuadPart == PartEntry->SectorCount.QuadPart)
    {
        DPRINT1("Convert existing partition entry\n");

        /* Convert current entry to 'new (unformatted)' */
        PartEntry->IsPartitioned = TRUE;
        PartEntry->FormatState = Formatted;     // FIXME???????? Possibly to make GetNextUnformattedPartition work (i.e. skip the extended partition container)
        PartEntry->FileSystem  = NULL;
        PartEntry->AutoCreate = FALSE;
        PartEntry->New = FALSE;
        PartEntry->BootIndicator = FALSE;

        if (PartEntry->StartSector.QuadPart < 1450560)
        {
            /* Partition starts below the 8.4GB boundary ==> CHS partition */
            PartEntry->PartitionType = PARTITION_EXTENDED;
        }
        else
        {
            /* Partition starts above the 8.4GB boundary ==> LBA partition */
            PartEntry->PartitionType = PARTITION_XINT13_EXTENDED;
        }

        DiskEntry->ExtendedPartition = PartEntry;

        DPRINT1("First Sector: %I64u\n", PartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", PartEntry->StartSector.QuadPart + PartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", PartEntry->SectorCount.QuadPart);
    }
    else
    {
        DPRINT1("Add new partition entry\n");

        /* Insert and initialize a new partition entry */
        NewPartEntry = RtlAllocateHeap(ProcessHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(PARTENTRY));
        if (NewPartEntry == NULL)
            return;

        /* Insert the new entry into the list */
        InsertTailList(&PartEntry->ListEntry,
                       &NewPartEntry->ListEntry);

        NewPartEntry->DiskEntry = DiskEntry;

        NewPartEntry->IsPartitioned = TRUE;
        NewPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
        NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment) -
                                             NewPartEntry->StartSector.QuadPart;

        NewPartEntry->New = FALSE;
        NewPartEntry->FormatState = Formatted;     // FIXME???????? Possibly to make GetNextUnformattedPartition work (i.e. skip the extended partition container)
        NewPartEntry->FileSystem  = NULL;
        NewPartEntry->BootIndicator = FALSE;

        if (NewPartEntry->StartSector.QuadPart < 1450560)
        {
            /* Partition starts below the 8.4GB boundary ==> CHS partition */
            NewPartEntry->PartitionType = PARTITION_EXTENDED;
        }
        else
        {
            /* Partition starts above the 8.4GB boundary ==> LBA partition */
            NewPartEntry->PartitionType = PARTITION_XINT13_EXTENDED;
        }

        DiskEntry->ExtendedPartition = NewPartEntry;

        PartEntry->StartSector.QuadPart = NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart;
        PartEntry->SectorCount.QuadPart -= (PartEntry->StartSector.QuadPart - NewPartEntry->StartSector.QuadPart);

        DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);
    }

    AddLogicalDiskSpace(DiskEntry);

    UpdateDiskLayout(DiskEntry);

    DiskEntry->Dirty = TRUE;

    AssignDriveLetters(List);
}

VOID
CreateLogicalPartition(
    IN PPARTLIST List,
    IN ULONGLONG SectorCount,
    IN BOOLEAN AutoCreate)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PPARTENTRY NewPartEntry;

    DPRINT1("CreateLogicalPartition(%I64u)\n", SectorCount);

    if (List == NULL ||
        List->CurrentDisk == NULL ||
        List->CurrentPartition == NULL ||
        List->CurrentPartition->IsPartitioned == TRUE)
    {
        return;
    }

    DiskEntry = List->CurrentDisk;
    PartEntry = List->CurrentPartition;

    DPRINT1("Current partition sector count: %I64u\n", PartEntry->SectorCount.QuadPart);

    if (AutoCreate == TRUE ||
        AlignDown(PartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment) - PartEntry->StartSector.QuadPart == PartEntry->SectorCount.QuadPart)
    {
        DPRINT1("Convert existing partition entry\n");

        /* Convert current entry to 'new (unformatted)' */
        PartEntry->IsPartitioned = TRUE;
        PartEntry->PartitionType = PARTITION_ENTRY_UNUSED;
        PartEntry->FormatState = Unformatted;
        PartEntry->FileSystem  = NULL;
        PartEntry->AutoCreate = FALSE;
        PartEntry->New = TRUE;
        PartEntry->BootIndicator = FALSE;
        PartEntry->LogicalPartition = TRUE;

        DPRINT1("First Sector: %I64u\n", PartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", PartEntry->StartSector.QuadPart + PartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", PartEntry->SectorCount.QuadPart);
    }
    else
    {
        DPRINT1("Add new partition entry\n");

        /* Insert and initialize a new partition entry */
        NewPartEntry = RtlAllocateHeap(ProcessHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(PARTENTRY));
        if (NewPartEntry == NULL)
            return;

        /* Insert the new entry into the list */
        InsertTailList(&PartEntry->ListEntry,
                       &NewPartEntry->ListEntry);

        NewPartEntry->DiskEntry = DiskEntry;

        NewPartEntry->IsPartitioned = TRUE;
        NewPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
        NewPartEntry->SectorCount.QuadPart = AlignDown(NewPartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment) -
                                             NewPartEntry->StartSector.QuadPart;
        NewPartEntry->PartitionType = PARTITION_ENTRY_UNUSED;

        DPRINT1("First Sector: %I64u\n", NewPartEntry->StartSector.QuadPart);
        DPRINT1("Last Sector: %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
        DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

        NewPartEntry->New = TRUE;
        NewPartEntry->FormatState = Unformatted;
        NewPartEntry->FileSystem  = NULL;
        NewPartEntry->BootIndicator = FALSE;
        NewPartEntry->LogicalPartition = TRUE;

        PartEntry->StartSector.QuadPart = NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart;
        PartEntry->SectorCount.QuadPart -= (PartEntry->StartSector.QuadPart - NewPartEntry->StartSector.QuadPart);
    }

    UpdateDiskLayout(DiskEntry);

    DiskEntry->Dirty = TRUE;

    AssignDriveLetters(List);
}

VOID
DeleteCurrentPartition(
    IN PPARTLIST List)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PPARTENTRY PrevPartEntry;
    PPARTENTRY NextPartEntry;
    PPARTENTRY LogicalPartEntry;
    PLIST_ENTRY Entry;

    if (List == NULL ||
        List->CurrentDisk == NULL ||
        List->CurrentPartition == NULL ||
        List->CurrentPartition->IsPartitioned == FALSE)
    {
        return;
    }

    /* Clear the system disk and partition pointers if the system partition is being deleted */
    if (List->SystemPartition == List->CurrentPartition)
    {
        List->SystemPartition = NULL;
    }

    DiskEntry = List->CurrentDisk;
    PartEntry = List->CurrentPartition;

    /* Delete all logical partition entries if an extended partition will be deleted */
    if (DiskEntry->ExtendedPartition == PartEntry)
    {
        while (!IsListEmpty(&DiskEntry->LogicalPartListHead))
        {
            Entry = RemoveHeadList(&DiskEntry->LogicalPartListHead);
            LogicalPartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            RtlFreeHeap(ProcessHeap, 0, LogicalPartEntry);
        }

        DiskEntry->ExtendedPartition = NULL;
    }

    /* Adjust unpartitioned disk space entries */

    /* Get pointer to previous and next unpartitioned entries */
    PrevPartEntry = GetPrevUnpartitionedEntry(DiskEntry, PartEntry);
    NextPartEntry = GetNextUnpartitionedEntry(DiskEntry, PartEntry);

    if (PrevPartEntry != NULL && NextPartEntry != NULL)
    {
        /* Merge previous, current and next unpartitioned entry */

        /* Adjust the previous entries length */
        PrevPartEntry->SectorCount.QuadPart += (PartEntry->SectorCount.QuadPart + NextPartEntry->SectorCount.QuadPart);

        /* Remove the current entry */
        RemoveEntryList(&PartEntry->ListEntry);
        RtlFreeHeap(ProcessHeap, 0, PartEntry);

        /* Remove the next entry */
        RemoveEntryList(&NextPartEntry->ListEntry);
        RtlFreeHeap(ProcessHeap, 0, NextPartEntry);

        /* Update current partition */
        List->CurrentPartition = PrevPartEntry;
    }
    else if (PrevPartEntry != NULL && NextPartEntry == NULL)
    {
        /* Merge current and previous unpartitioned entry */

        /* Adjust the previous entries length */
        PrevPartEntry->SectorCount.QuadPart += PartEntry->SectorCount.QuadPart;

        /* Remove the current entry */
        RemoveEntryList(&PartEntry->ListEntry);
        RtlFreeHeap(ProcessHeap, 0, PartEntry);

        /* Update current partition */
        List->CurrentPartition = PrevPartEntry;
    }
    else if (PrevPartEntry == NULL && NextPartEntry != NULL)
    {
        /* Merge current and next unpartitioned entry */

        /* Adjust the next entries offset and length */
        NextPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
        NextPartEntry->SectorCount.QuadPart += PartEntry->SectorCount.QuadPart;

        /* Remove the current entry */
        RemoveEntryList(&PartEntry->ListEntry);
        RtlFreeHeap(ProcessHeap, 0, PartEntry);

        /* Update current partition */
        List->CurrentPartition = NextPartEntry;
    }
    else
    {
        /* Nothing to merge but change current entry */
        PartEntry->IsPartitioned = FALSE;
        PartEntry->PartitionType = PARTITION_ENTRY_UNUSED;
        PartEntry->FormatState = Unformatted;
        PartEntry->FileSystem  = NULL;
        PartEntry->DriveLetter = 0;
    }

    UpdateDiskLayout(DiskEntry);

    DiskEntry->Dirty = TRUE;

    AssignDriveLetters(List);
}

VOID
CheckActiveSystemPartition(
    IN PPARTLIST List)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PLIST_ENTRY ListEntry;

    PFILE_SYSTEM FileSystem;

    /* Check for empty disk list */
    if (IsListEmpty(&List->DiskListHead))
    {
        List->SystemPartition = NULL;
        List->OriginalSystemPartition = NULL;
        return;
    }

    /* Choose the currently selected disk */
    DiskEntry = List->CurrentDisk;

    /* Check for empty partition list */
    if (IsListEmpty(&DiskEntry->PrimaryPartListHead))
    {
        List->SystemPartition = NULL;
        List->OriginalSystemPartition = NULL;
        return;
    }

    if (List->SystemPartition != NULL)
    {
        /* We already have an active system partition */
        DPRINT1("Use the current system partition %lu in disk %lu, drive letter %c\n",
                List->SystemPartition->PartitionNumber,
                List->SystemPartition->DiskEntry->DiskNumber,
                (List->SystemPartition->DriveLetter == 0) ? '-' : List->SystemPartition->DriveLetter);
        return;
    }

    DPRINT1("We are here (1)!\n");

    List->SystemPartition = NULL;
    List->OriginalSystemPartition = NULL;

    /* Retrieve the first partition of the disk */
    PartEntry = CONTAINING_RECORD(DiskEntry->PrimaryPartListHead.Flink,
                                  PARTENTRY,
                                  ListEntry);
    ASSERT(DiskEntry == PartEntry->DiskEntry);
    List->SystemPartition = PartEntry;

    //
    // See: https://svn.reactos.org/svn/reactos/trunk/reactos/base/setup/usetup/partlist.c?r1=63355&r2=63354&pathrev=63355#l2318
    //

    /* Check if the disk is new and if so, use its first partition as the active system partition */
    if (DiskEntry->NewDisk)
    {
        if (PartEntry->PartitionType == PARTITION_ENTRY_UNUSED || PartEntry->BootIndicator == FALSE)
        {
            ASSERT(DiskEntry == PartEntry->DiskEntry);
            List->SystemPartition = PartEntry;

            List->OriginalSystemPartition = List->SystemPartition;

            DPRINT1("Use new first active system partition %lu in disk %lu, drive letter %c\n",
                    List->SystemPartition->PartitionNumber,
                    List->SystemPartition->DiskEntry->DiskNumber,
                    (List->SystemPartition->DriveLetter == 0) ? '-' : List->SystemPartition->DriveLetter);

            goto SetSystemPartition;
        }

        // FIXME: What to do??
        DPRINT1("NewDisk TRUE but first partition is used?\n");
    }

    DPRINT1("We are here (2)!\n");

    /*
     * The disk is not new, check if any partition is initialized;
     * if not, the first one becomes the system partition.
     */
    ListEntry = DiskEntry->PrimaryPartListHead.Flink;
    while (ListEntry != &DiskEntry->PrimaryPartListHead)
    {
        /* Retrieve the partition and go to the next one */
        PartEntry = CONTAINING_RECORD(ListEntry,
                                      PARTENTRY,
                                      ListEntry);

        /* Check if the partition is partitioned and is used */
        if (PartEntry->PartitionType != PARTITION_ENTRY_UNUSED || PartEntry->BootIndicator != FALSE)
        {
            break;
        }

        /* Go to the next one */
        ListEntry = ListEntry->Flink;
    }
    if (ListEntry == &DiskEntry->PrimaryPartListHead)
    {
        /*
         * OK we haven't encountered any used and active partition,
         * so use the first one as the system partition.
         */
        ASSERT(DiskEntry == List->SystemPartition->DiskEntry);
        List->OriginalSystemPartition = List->SystemPartition; // First PartEntry

        DPRINT1("Use first active system partition %lu in disk %lu, drive letter %c\n",
                List->SystemPartition->PartitionNumber,
                List->SystemPartition->DiskEntry->DiskNumber,
                (List->SystemPartition->DriveLetter == 0) ? '-' : List->SystemPartition->DriveLetter);

        goto SetSystemPartition;
    }

    List->SystemPartition = NULL;
    List->OriginalSystemPartition = NULL;

    DPRINT1("We are here (3)!\n");

    /* The disk is not new, scan all partitions to find the (active) system partition */
    ListEntry = DiskEntry->PrimaryPartListHead.Flink;
    while (ListEntry != &DiskEntry->PrimaryPartListHead)
    {
        /* Retrieve the partition and go to the next one */
        PartEntry = CONTAINING_RECORD(ListEntry,
                                      PARTENTRY,
                                      ListEntry);
        ListEntry = ListEntry->Flink;

        /* Check if the partition is partitioned and used */
        if (PartEntry->IsPartitioned &&
            PartEntry->PartitionType != PARTITION_ENTRY_UNUSED)
        {
            /* Check if the partition is active */
            if (PartEntry->BootIndicator)
            {
                /* Yes, we found it */
                ASSERT(DiskEntry == PartEntry->DiskEntry);
                List->SystemPartition = PartEntry;

                DPRINT1("Found active system partition %lu in disk %lu, drive letter %c\n",
                        PartEntry->PartitionNumber,
                        DiskEntry->DiskNumber,
                        (PartEntry->DriveLetter == 0) ? '-' : PartEntry->DriveLetter);
                break;
            }
        }
    }

    /* Check if we have found the system partition */
    if (List->SystemPartition == NULL)
    {
        /* Nothing, use the alternative system partition */
        DPRINT1("No system partition found, use the alternative partition!\n");
        goto UseAlternativeSystemPartition;
    }

    /* Save it */
    List->OriginalSystemPartition = List->SystemPartition;

    /*
     * ADDITIONAL CHECKS / BIG HACK:
     *
     * Retrieve its file system and check whether we have
     * write support for it. If that is the case we are fine
     * and we can use it directly. However if we don't have
     * write support we will need to change the active system
     * partition.
     *
     * NOTE that this is completely useless on architectures
     * where a real system partition is required, as on these
     * architectures the partition uses the FAT FS, for which
     * we do have write support.
     * NOTE also that for those architectures looking for a
     * partition boot indicator is insufficient.
     */
    FileSystem = GetFileSystem(List->OriginalSystemPartition);
    if (FileSystem == NULL)
    {
        DPRINT1("System partition %lu in disk %lu with no FS?!\n",
                List->OriginalSystemPartition->PartitionNumber,
                List->OriginalSystemPartition->DiskEntry->DiskNumber);
        goto FindAndUseAlternativeSystemPartition;
    }
    // HACK: WARNING: We cannot write on this FS yet!
    // See fsutil.c:GetFileSystem()
    if (List->OriginalSystemPartition->PartitionType == PARTITION_EXT2 ||
        List->OriginalSystemPartition->PartitionType == PARTITION_IFS)
    {
        DPRINT1("Recognized file system %S that doesn't support write support yet!\n",
                FileSystem->FileSystemName);
        goto FindAndUseAlternativeSystemPartition;
    }

    DPRINT1("Use existing active system partition %lu in disk %lu, drive letter %c\n",
            List->SystemPartition->PartitionNumber,
            List->SystemPartition->DiskEntry->DiskNumber,
            (List->SystemPartition->DriveLetter == 0) ? '-' : List->SystemPartition->DriveLetter);

    return;

FindAndUseAlternativeSystemPartition:
    /*
     * We are here because we have not found any (active) candidate
     * system partition that we know how to support. What we are going
     * to do is to change the existing system partition and use the
     * partition on which we install ReactOS as the new system partition,
     * and then we will need to add in FreeLdr's entry a boot entry to boot
     * from the original system partition.
     */

    /* Unset the old system partition */
    List->SystemPartition->BootIndicator = FALSE;
    List->SystemPartition->DiskEntry->LayoutBuffer->PartitionEntry[List->SystemPartition->PartitionIndex].BootIndicator = FALSE;
    List->SystemPartition->DiskEntry->LayoutBuffer->PartitionEntry[List->SystemPartition->PartitionIndex].RewritePartition = TRUE;
    List->SystemPartition->DiskEntry->Dirty = TRUE;

UseAlternativeSystemPartition:
    List->SystemPartition = List->CurrentPartition;

    DPRINT1("Use alternative active system partition %lu in disk %lu, drive letter %c\n",
            List->SystemPartition->PartitionNumber,
            List->SystemPartition->DiskEntry->DiskNumber,
            (List->SystemPartition->DriveLetter == 0) ? '-' : List->SystemPartition->DriveLetter);

SetSystemPartition:
    /* Set the new active system partition */
    List->SystemPartition->BootIndicator = TRUE;
    List->SystemPartition->DiskEntry->LayoutBuffer->PartitionEntry[List->SystemPartition->PartitionIndex].BootIndicator = TRUE;
    List->SystemPartition->DiskEntry->LayoutBuffer->PartitionEntry[List->SystemPartition->PartitionIndex].RewritePartition = TRUE;
    List->SystemPartition->DiskEntry->Dirty = TRUE;
}

static
NTSTATUS
WritePartitions(
    IN PPARTLIST List,
    IN PDISKENTRY DiskEntry)
{
    WCHAR DstPath[MAX_PATH];
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    UNICODE_STRING Name;
    ULONG BufferSize;
    HANDLE FileHandle = NULL;
    NTSTATUS Status;

    DPRINT("WritePartitions() Disk: %lu\n", DiskEntry->DiskNumber);

    StringCchPrintfW(DstPath, ARRAYSIZE(DstPath),
                     L"\\Device\\Harddisk%lu\\Partition0",
                     DiskEntry->DiskNumber);
    RtlInitUnicodeString(&Name, DstPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
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

#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(DiskEntry);
#endif

    BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                 ((DiskEntry->LayoutBuffer->PartitionCount - 1) * sizeof(PARTITION_INFORMATION));
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_SET_DRIVE_LAYOUT,
                                   DiskEntry->LayoutBuffer,
                                   BufferSize,
                                   NULL,
                                   0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IOCTL_DISK_SET_DRIVE_LAYOUT failed (Status 0x%08lx)\n", Status);
    }

    if (FileHandle != NULL)
        NtClose(FileHandle);

    //
    // NOTE: Originally (see r40437), we used to install here also a new MBR
    // for this disk (by calling InstallMbrBootCodeToDisk), only if:
    // DiskEntry->NewDisk == TRUE and DiskEntry->BiosDiskNumber == 0.
    // Then after that, both DiskEntry->NewDisk and DiskEntry->NoMbr were set
    // to FALSE. In the other place (in usetup.c) where InstallMbrBootCodeToDisk
    // was called too, the installation test was modified by checking whether
    // DiskEntry->NoMbr was TRUE (instead of NewDisk).
    //

    return Status;
}

BOOLEAN
WritePartitionsToDisk(
    IN PPARTLIST List)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;

    if (List == NULL)
        return TRUE;

    Entry = List->DiskListHead.Flink;
    while (Entry != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->Dirty == TRUE)
        {
            WritePartitions(List, DiskEntry);
            DiskEntry->Dirty = FALSE;
        }

        Entry = Entry->Flink;
    }

    return TRUE;
}

BOOLEAN
SetMountedDeviceValue(
    IN CHAR Letter,
    IN ULONG Signature,
    IN LARGE_INTEGER StartingOffset)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR ValueNameBuffer[16];
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\MountedDevices");
    UNICODE_STRING ValueName;
    REG_DISK_MOUNT_INFO MountInfo;
    NTSTATUS Status;
    HANDLE KeyHandle;

    StringCchPrintfW(ValueNameBuffer, ARRAYSIZE(ValueNameBuffer),
                     L"\\DosDevices\\%C:", Letter);
    RtlInitUnicodeString(&ValueName, ValueNameBuffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status =  NtOpenKey(&KeyHandle,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        Status = NtCreateKey(&KeyHandle,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    MountInfo.Signature = Signature;
    MountInfo.StartingOffset = StartingOffset;
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_BINARY,
                           (PVOID)&MountInfo,
                           sizeof(MountInfo));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SetMountedDeviceValues(
    IN PPARTLIST List)
{
    PLIST_ENTRY Entry1, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    LARGE_INTEGER StartingOffset;

    if (List == NULL)
        return FALSE;

    Entry1 = List->DiskListHead.Flink;
    while (Entry1 != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry1,
                                      DISKENTRY,
                                      ListEntry);

        Entry2 = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry2 != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry->IsPartitioned)
            {
                /* Assign a "\DosDevices\#:" mount point to this partition */
                if (PartEntry->DriveLetter)
                {
                    StartingOffset.QuadPart = PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector;
                    if (!SetMountedDeviceValue(PartEntry->DriveLetter,
                                               DiskEntry->LayoutBuffer->Signature,
                                               StartingOffset))
                    {
                        return FALSE;
                    }
                }
            }

            Entry2 = Entry2->Flink;
        }

        Entry2 = DiskEntry->LogicalPartListHead.Flink;
        while (Entry2 != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry->IsPartitioned)
            {
                /* Assign a "\DosDevices\#:" mount point to this partition */
                if (PartEntry->DriveLetter)
                {
                    StartingOffset.QuadPart = PartEntry->StartSector.QuadPart * DiskEntry->BytesPerSector;
                    if (!SetMountedDeviceValue(PartEntry->DriveLetter,
                                               DiskEntry->LayoutBuffer->Signature,
                                               StartingOffset))
                    {
                        return FALSE;
                    }
                }
            }

            Entry2 = Entry2->Flink;
        }

        Entry1 = Entry1->Flink;
    }

    return TRUE;
}

VOID
SetPartitionType(
    IN PPARTENTRY PartEntry,
    IN UCHAR PartitionType)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;

    PartEntry->PartitionType = PartitionType;

    DiskEntry->Dirty = TRUE;
    DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].PartitionType = PartitionType;
    DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].RewritePartition = TRUE;
}

ERROR_NUMBER
PrimaryPartitionCreationChecks(
    IN PPARTLIST List)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    DiskEntry = List->CurrentDisk;
    PartEntry = List->CurrentPartition;

    /* Fail if partition is already in use */
    if (PartEntry->IsPartitioned == TRUE)
        return ERROR_NEW_PARTITION;

    /* Fail if there are already 4 primary partitions in the list */
    if (GetPrimaryPartitionCount(DiskEntry) >= 4)
        return ERROR_PARTITION_TABLE_FULL;

    return ERROR_SUCCESS;
}

ERROR_NUMBER
ExtendedPartitionCreationChecks(
    IN PPARTLIST List)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    DiskEntry = List->CurrentDisk;
    PartEntry = List->CurrentPartition;

    /* Fail if partition is already in use */
    if (PartEntry->IsPartitioned == TRUE)
        return ERROR_NEW_PARTITION;

    /* Fail if there are already 4 primary partitions in the list */
    if (GetPrimaryPartitionCount(DiskEntry) >= 4)
        return ERROR_PARTITION_TABLE_FULL;

    /* Fail if there is another extended partition in the list */
    if (DiskEntry->ExtendedPartition != NULL)
        return ERROR_ONLY_ONE_EXTENDED;

    return ERROR_SUCCESS;
}

ERROR_NUMBER
LogicalPartitionCreationChecks(
    IN PPARTLIST List)
{
//    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

//    DiskEntry = List->CurrentDisk;
    PartEntry = List->CurrentPartition;

    /* Fail if partition is already in use */
    if (PartEntry->IsPartitioned == TRUE)
        return ERROR_NEW_PARTITION;

    return ERROR_SUCCESS;
}

BOOLEAN
GetNextUnformattedPartition(
    IN PPARTLIST List,
    OUT PDISKENTRY *pDiskEntry OPTIONAL,
    OUT PPARTENTRY *pPartEntry)
{
    PLIST_ENTRY Entry1, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    Entry1 = List->DiskListHead.Flink;
    while (Entry1 != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry1,
                                      DISKENTRY,
                                      ListEntry);

        Entry2 = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry2 != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry->IsPartitioned && PartEntry->New)
            {
                ASSERT(DiskEntry == PartEntry->DiskEntry);
                if (pDiskEntry) *pDiskEntry = DiskEntry;
                *pPartEntry = PartEntry;
                return TRUE;
            }

            Entry2 = Entry2->Flink;
        }

        Entry2 = DiskEntry->LogicalPartListHead.Flink;
        while (Entry2 != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry->IsPartitioned && PartEntry->New)
            {
                ASSERT(DiskEntry == PartEntry->DiskEntry);
                if (pDiskEntry) *pDiskEntry = DiskEntry;
                *pPartEntry = PartEntry;
                return TRUE;
            }

            Entry2 = Entry2->Flink;
        }

        Entry1 = Entry1->Flink;
    }

    if (pDiskEntry) *pDiskEntry = NULL;
    *pPartEntry = NULL;

    return FALSE;
}

BOOLEAN
GetNextUncheckedPartition(
    IN PPARTLIST List,
    OUT PDISKENTRY *pDiskEntry OPTIONAL,
    OUT PPARTENTRY *pPartEntry)
{
    PLIST_ENTRY Entry1, Entry2;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    Entry1 = List->DiskListHead.Flink;
    while (Entry1 != &List->DiskListHead)
    {
        DiskEntry = CONTAINING_RECORD(Entry1,
                                      DISKENTRY,
                                      ListEntry);

        Entry2 = DiskEntry->PrimaryPartListHead.Flink;
        while (Entry2 != &DiskEntry->PrimaryPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry->NeedsCheck == TRUE)
            {
                ASSERT(DiskEntry == PartEntry->DiskEntry);
                if (pDiskEntry) *pDiskEntry = DiskEntry;
                *pPartEntry = PartEntry;
                return TRUE;
            }

            Entry2 = Entry2->Flink;
        }

        Entry2 = DiskEntry->LogicalPartListHead.Flink;
        while (Entry2 != &DiskEntry->LogicalPartListHead)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);
            if (PartEntry->NeedsCheck == TRUE)
            {
                ASSERT(DiskEntry == PartEntry->DiskEntry);
                if (pDiskEntry) *pDiskEntry = DiskEntry;
                *pPartEntry = PartEntry;
                return TRUE;
            }

            Entry2 = Entry2->Flink;
        }

        Entry1 = Entry1->Flink;
    }

    if (pDiskEntry) *pDiskEntry = NULL;
    *pPartEntry = NULL;

    return FALSE;
}

/* EOF */
