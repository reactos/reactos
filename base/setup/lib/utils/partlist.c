/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Partition list functions
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2018-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include "precomp.h"
#include <ntddscsi.h>

#include "partlist.h"
#include "volutil.h"
#include "fsrec.h" // For FileSystemToMBRPartitionType()

#include "registry.h"

#define NDEBUG
#include <debug.h>

// #define DUMP_PARTITION_TABLE

#include <pshpack1.h>
typedef struct _REG_DISK_MOUNT_INFO
{
    ULONG Signature;
    ULONGLONG StartingOffset;
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

    RtlStringCchPrintfW(KeyName, ARRAYSIZE(KeyName),
                        L"\\Scsi\\Scsi Port %hu",
                        DiskEntry->Port);

    RtlZeroMemory(&QueryTable, sizeof(QueryTable));

    QueryTable[0].Name = L"Driver";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].EntryContext = &DiskEntry->DriverName;

    /* This will allocate DiskEntry->DriverName if needed */
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
    WCHAR Letter;

    Letter = L'C';

    /* Assign drive letters to primary partitions */
    for (Entry1 = List->DiskListHead.Flink;
         Entry1 != &List->DiskListHead;
         Entry1 = Entry1->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry1, DISKENTRY, ListEntry);

        for (Entry2 = DiskEntry->PrimaryPartListHead.Flink;
             Entry2 != &DiskEntry->PrimaryPartListHead;
             Entry2 = Entry2->Flink)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);

            if (!PartEntry->Volume)
                continue;
            PartEntry->Volume->Info.DriveLetter = UNICODE_NULL;

            if (PartEntry->IsPartitioned &&
                !IsContainerPartition(PartEntry->PartitionType) &&
                (IsRecognizedPartition(PartEntry->PartitionType) ||
                 PartEntry->SectorCount.QuadPart != 0LL))
            {
                if (Letter <= L'Z')
                    PartEntry->Volume->Info.DriveLetter = Letter++;
            }
        }
    }

    /* Assign drive letters to logical drives */
    for (Entry1 = List->DiskListHead.Flink;
         Entry1 != &List->DiskListHead;
         Entry1 = Entry1->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry1, DISKENTRY, ListEntry);

        for (Entry2 = DiskEntry->LogicalPartListHead.Flink;
             Entry2 != &DiskEntry->LogicalPartListHead;
             Entry2 = Entry2->Flink)
        {
            PartEntry = CONTAINING_RECORD(Entry2, PARTENTRY, ListEntry);

            if (!PartEntry->Volume)
                continue;
            PartEntry->Volume->Info.DriveLetter = UNICODE_NULL;

            if (PartEntry->IsPartitioned &&
                (IsRecognizedPartition(PartEntry->PartitionType) ||
                 PartEntry->SectorCount.QuadPart != 0LL))
            {
                if (Letter <= L'Z')
                    PartEntry->Volume->Info.DriveLetter = Letter++;
            }
        }
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
        ValueLength == 20 * sizeof(WCHAR) &&
        ((PWCHAR)ValueData)[8] == L'-')
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


static VOID
EnumerateBiosDiskEntries(
    IN PPARTLIST PartList)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    WCHAR Name[120];
    ULONG AdapterCount;
    ULONG ControllerCount;
    ULONG DiskCount;
    NTSTATUS Status;
    PCM_INT13_DRIVE_PARAMETER Int13Drives;
    PBIOSDISKENTRY BiosDiskEntry;

#define ROOT_NAME   L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter"

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

    for (AdapterCount = 0; ; ++AdapterCount)
    {
        RtlStringCchPrintfW(Name, ARRAYSIZE(Name),
                            L"%s\\%lu",
                            ROOT_NAME, AdapterCount);
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        Name,
                                        &QueryTable[2],
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        RtlStringCchPrintfW(Name, ARRAYSIZE(Name),
                            L"%s\\%lu\\DiskController",
                            ROOT_NAME, AdapterCount);
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        Name,
                                        &QueryTable[2],
                                        NULL,
                                        NULL);
        if (NT_SUCCESS(Status))
        {
            for (ControllerCount = 0; ; ++ControllerCount)
            {
                RtlStringCchPrintfW(Name, ARRAYSIZE(Name),
                                    L"%s\\%lu\\DiskController\\%lu",
                                    ROOT_NAME, AdapterCount, ControllerCount);
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

                RtlStringCchPrintfW(Name, ARRAYSIZE(Name),
                                    L"%s\\%lu\\DiskController\\%lu\\DiskPeripheral",
                                    ROOT_NAME, AdapterCount, ControllerCount);
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

                    for (DiskCount = 0; ; ++DiskCount)
                    {
                        BiosDiskEntry = (BIOSDISKENTRY*)RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, sizeof(BIOSDISKENTRY));
                        if (BiosDiskEntry == NULL)
                        {
                            RtlFreeHeap(ProcessHeap, 0, Int13Drives);
                            return;
                        }

                        RtlStringCchPrintfW(Name, ARRAYSIZE(Name),
                                            L"%s\\%lu\\DiskController\\%lu\\DiskPeripheral\\%lu",
                                            ROOT_NAME, AdapterCount, ControllerCount, DiskCount);
                        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                                        Name,
                                                        QueryTable,
                                                        (PVOID)BiosDiskEntry,
                                                        NULL);
                        if (!NT_SUCCESS(Status))
                        {
                            RtlFreeHeap(ProcessHeap, 0, BiosDiskEntry);
                            RtlFreeHeap(ProcessHeap, 0, Int13Drives);
                            return;
                        }

                        BiosDiskEntry->AdapterNumber = 0; // And NOT "AdapterCount" as it needs to be hardcoded for BIOS!
                        BiosDiskEntry->ControllerNumber = ControllerCount;
                        BiosDiskEntry->DiskNumber = DiskCount;
                        BiosDiskEntry->DiskEntry = NULL;

                        if (DiskCount < Int13Drives[0].NumberDrives)
                        {
                            BiosDiskEntry->Int13DiskData = Int13Drives[DiskCount];
                        }
                        else
                        {
                            DPRINT1("Didn't find Int13 drive data for disk %u\n", DiskCount);
                        }

                        InsertTailList(&PartList->BiosDiskListHead, &BiosDiskEntry->ListEntry);

                        DPRINT("--->\n");
                        DPRINT("AdapterNumber:     %lu\n", BiosDiskEntry->AdapterNumber);
                        DPRINT("ControllerNumber:  %lu\n", BiosDiskEntry->ControllerNumber);
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
                        DPRINT("<---\n");
                    }
                }
            }
        }
    }

    RtlFreeHeap(ProcessHeap, 0, Int13Drives);

#undef ROOT_NAME
}


/*
 * Detects whether a disk is a "super-floppy", i.e. an unpartitioned
 * disk with only a valid VBR, as reported by IoReadPartitionTable()
 * and IoWritePartitionTable():
 * only one single partition starting at offset zero and spanning the
 * whole disk, without hidden sectors, whose type is FAT16 non-bootable.
 *
 * Accessing \Device\HarddiskN\Partition0 or Partition1 on such disks
 * returns the same data.
 */
BOOLEAN
IsDiskSuperFloppy2(
    _In_ const DISK_PARTITION_INFO* DiskInfo,
    _In_opt_ const ULONGLONG* DiskSize,
    _In_ const PARTITION_INFORMATION* PartitionInfo)
{
    /* Structure size must be valid */
    if (DiskInfo->SizeOfPartitionInfo < RTL_SIZEOF_THROUGH_FIELD(DISK_PARTITION_INFO, Mbr))
        return FALSE;

    /* The layout must be MBR */
    if (DiskInfo->PartitionStyle != PARTITION_STYLE_MBR)
        return FALSE;

    /* The single partition must start at the beginning of the disk */
    if (!(PartitionInfo->StartingOffset.QuadPart == 0 &&
          PartitionInfo->HiddenSectors == 0))
    {
        return FALSE;
    }

    /* The disk signature is usually set to 1; warn in case it's not */
    if (DiskInfo->Mbr.Signature != 1)
    {
        DPRINT1("Super-Floppy signature %08x != 1\n", DiskInfo->Mbr.Signature);
    }

    /* The partition must be recognized and report as FAT16 non-bootable */
    if ((PartitionInfo->RecognizedPartition != TRUE) ||
        (PartitionInfo->PartitionType != PARTITION_FAT_16) ||
        (PartitionInfo->BootIndicator != FALSE))
    {
        DPRINT1("Super-Floppy does not return default settings:\n"
                "    RecognizedPartition = %s, expected TRUE\n"
                "    PartitionType = 0x%02x, expected 0x04 (PARTITION_FAT_16)\n"
                "    BootIndicator = %s, expected FALSE\n",
                PartitionInfo->RecognizedPartition ? "TRUE" : "FALSE",
                PartitionInfo->PartitionType,
                PartitionInfo->BootIndicator ? "TRUE" : "FALSE");
    }

    /* The partition and disk sizes should agree */
    if (DiskSize && (PartitionInfo->PartitionLength.QuadPart != *DiskSize))
    {
        DPRINT1("PartitionLength = %I64u is different from DiskSize = %I64u\n",
                PartitionInfo->PartitionLength.QuadPart, *DiskSize);
    }

    return TRUE;
}

BOOLEAN
IsDiskSuperFloppy(
    _In_ const DRIVE_LAYOUT_INFORMATION* Layout,
    _In_opt_ const ULONGLONG* DiskSize)
{
    DISK_PARTITION_INFO DiskInfo;

    /* The layout must contain only one partition */
    if (Layout->PartitionCount != 1)
        return FALSE;

    /* Build the disk partition info */
    DiskInfo.SizeOfPartitionInfo = RTL_SIZEOF_THROUGH_FIELD(DISK_PARTITION_INFO, Mbr);
    DiskInfo.PartitionStyle = PARTITION_STYLE_MBR;
    DiskInfo.Mbr.Signature = Layout->Signature;
    DiskInfo.Mbr.CheckSum = 0; // Dummy value

    /* Call the helper on the single partition entry */
    return IsDiskSuperFloppy2(&DiskInfo, DiskSize, Layout->PartitionEntry);
}

BOOLEAN
IsDiskSuperFloppyEx(
    _In_ const DRIVE_LAYOUT_INFORMATION_EX* LayoutEx,
    _In_opt_ const ULONGLONG* DiskSize)
{
    DISK_PARTITION_INFO DiskInfo;
    const PARTITION_INFORMATION_EX* PartitionInfoEx;
    PARTITION_INFORMATION PartitionInfo;

    /* The layout must be MBR and contain only one partition */
    if (LayoutEx->PartitionStyle != PARTITION_STYLE_MBR)
        return FALSE;
    if (LayoutEx->PartitionCount != 1)
        return FALSE;

    /* Build the disk partition info */
    DiskInfo.SizeOfPartitionInfo = RTL_SIZEOF_THROUGH_FIELD(DISK_PARTITION_INFO, Mbr);
    DiskInfo.PartitionStyle = PARTITION_STYLE_MBR; // LayoutEx->PartitionStyle;
    DiskInfo.Mbr.Signature = LayoutEx->Mbr.Signature;
    DiskInfo.Mbr.CheckSum = 0; // Dummy value

    /* Convert the single partition entry */
    PartitionInfoEx = LayoutEx->PartitionEntry;

    PartitionInfo.StartingOffset = PartitionInfoEx->StartingOffset;
    PartitionInfo.PartitionLength = PartitionInfoEx->PartitionLength;
    PartitionInfo.HiddenSectors = PartitionInfoEx->Mbr.HiddenSectors;
    PartitionInfo.PartitionNumber = PartitionInfoEx->PartitionNumber;
    PartitionInfo.PartitionType = PartitionInfoEx->Mbr.PartitionType;
    PartitionInfo.BootIndicator = PartitionInfoEx->Mbr.BootIndicator;
    PartitionInfo.RecognizedPartition = PartitionInfoEx->Mbr.RecognizedPartition;
    PartitionInfo.RewritePartition = PartitionInfoEx->RewritePartition;

    /* Call the helper on the single partition entry */
    return IsDiskSuperFloppy2(&DiskInfo, DiskSize, &PartitionInfo);
}

BOOLEAN
IsSuperFloppy(
    _In_ PDISKENTRY DiskEntry)
{
    ULONGLONG DiskSize;

    /* No layout buffer: we cannot say anything yet */
    if (!DiskEntry->LayoutBuffer)
        return FALSE;

    /* The disk must be MBR */
    if (DiskEntry->DiskStyle != PARTITION_STYLE_MBR)
        return FALSE;

    DiskSize = GetDiskSizeInBytes(DiskEntry);
    return IsDiskSuperFloppy(DiskEntry->LayoutBuffer, &DiskSize);
}


/*
 * Inserts the disk region represented by PartEntry into either
 * the primary or the logical partition list of the given disk.
 * The lists are kept sorted by increasing order of start sectors.
 * Of course no disk region should overlap at all with one another.
 */
static
BOOLEAN
InsertDiskRegion(
    IN PDISKENTRY DiskEntry,
    IN PPARTENTRY PartEntry,
    IN BOOLEAN LogicalPartition)
{
    PLIST_ENTRY List;
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry2;

    /* Use the correct partition list */
    if (LogicalPartition)
        List = &DiskEntry->LogicalPartListHead;
    else
        List = &DiskEntry->PrimaryPartListHead;

    /* Find the first disk region before which we need to insert the new one */
    for (Entry = List->Flink; Entry != List; Entry = Entry->Flink)
    {
        PartEntry2 = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        /* Ignore any unused empty region */
        if ((PartEntry2->PartitionType == PARTITION_ENTRY_UNUSED &&
             PartEntry2->StartSector.QuadPart == 0) || PartEntry2->SectorCount.QuadPart == 0)
        {
            continue;
        }

        /* If the current region ends before the one to be inserted, try again */
        if (PartEntry2->StartSector.QuadPart + PartEntry2->SectorCount.QuadPart - 1 < PartEntry->StartSector.QuadPart)
            continue;

        /*
         * One of the disk region boundaries crosses the desired region
         * (it starts after the desired region, or ends before the end
         * of the desired region): this is an impossible situation because
         * disk regions (partitions) cannot overlap!
         * Throw an error and bail out.
         */
        if (max(PartEntry->StartSector.QuadPart, PartEntry2->StartSector.QuadPart)
            <=
            min( PartEntry->StartSector.QuadPart +  PartEntry->SectorCount.QuadPart - 1,
                PartEntry2->StartSector.QuadPart + PartEntry2->SectorCount.QuadPart - 1))
        {
            DPRINT1("Disk region overlap problem, stopping there!\n"
                    "Partition to be inserted:\n"
                    "    StartSector = %I64u ; EndSector = %I64u\n"
                    "Existing disk region:\n"
                    "    StartSector = %I64u ; EndSector = %I64u\n",
                     PartEntry->StartSector.QuadPart,
                     PartEntry->StartSector.QuadPart +  PartEntry->SectorCount.QuadPart - 1,
                    PartEntry2->StartSector.QuadPart,
                    PartEntry2->StartSector.QuadPart + PartEntry2->SectorCount.QuadPart - 1);
            return FALSE;
        }

        /* We have found the first region before which the new one has to be inserted */
        break;
    }

    /* Insert the disk region */
    InsertTailList(Entry, &PartEntry->ListEntry);
    return TRUE;
}

static
PPARTENTRY
CreateInsertBlankRegion(
    IN PDISKENTRY DiskEntry,
    IN OUT PLIST_ENTRY ListHead,
    IN ULONGLONG StartSector,
    IN ULONGLONG SectorCount,
    IN BOOLEAN LogicalSpace)
{
    PPARTENTRY NewPartEntry;

    NewPartEntry = RtlAllocateHeap(ProcessHeap,
                                   HEAP_ZERO_MEMORY,
                                   sizeof(PARTENTRY));
    if (!NewPartEntry)
        return NULL;

    NewPartEntry->DiskEntry = DiskEntry;

    NewPartEntry->StartSector.QuadPart = StartSector;
    NewPartEntry->SectorCount.QuadPart = SectorCount;

    NewPartEntry->LogicalPartition = LogicalSpace;
    NewPartEntry->IsPartitioned = FALSE;
    NewPartEntry->PartitionType = PARTITION_ENTRY_UNUSED;
    NewPartEntry->Volume = NULL;

    DPRINT1("First Sector : %I64u\n", NewPartEntry->StartSector.QuadPart);
    DPRINT1("Last Sector  : %I64u\n", NewPartEntry->StartSector.QuadPart + NewPartEntry->SectorCount.QuadPart - 1);
    DPRINT1("Total Sectors: %I64u\n", NewPartEntry->SectorCount.QuadPart);

    /* Insert the new entry into the list */
    InsertTailList(ListHead, &NewPartEntry->ListEntry);

    return NewPartEntry;
}

static
VOID
DestroyRegion(
    _Inout_ PPARTENTRY PartEntry)
{
    // RemoveEntryList(&PartEntry->Volume->ListEntry);
    if (PartEntry->Volume)
        RtlFreeHeap(ProcessHeap, 0, PartEntry->Volume);
    RtlFreeHeap(ProcessHeap, 0, PartEntry);
}

static
VOID
AddLogicalDiskSpace(
    _In_ PDISKENTRY DiskEntry)
{
    ULONGLONG StartSector;
    ULONGLONG SectorCount;
    PPARTENTRY NewPartEntry;

    DPRINT("AddLogicalDiskSpace()\n");

    /* Create a partition entry that represents the empty space in the container partition */

    StartSector = DiskEntry->ExtendedPartition->StartSector.QuadPart + (ULONGLONG)DiskEntry->SectorAlignment;
    SectorCount = DiskEntry->ExtendedPartition->SectorCount.QuadPart - (ULONGLONG)DiskEntry->SectorAlignment;

    NewPartEntry = CreateInsertBlankRegion(DiskEntry,
                                           &DiskEntry->LogicalPartListHead,
                                           StartSector,
                                           SectorCount,
                                           TRUE);
    if (!NewPartEntry)
        DPRINT1("Failed to create a new empty region for full extended partition space!\n");
}

// TODO: Improve upon the PartitionInfo parameter later
// (see VDS::CREATE_PARTITION_PARAMETERS and PPARTITION_INFORMATION_MBR/GPT for example)
// So far we only use it as the optional type of the partition to create.
//
// See also CreatePartition().
static
BOOLEAN
InitializePartitionEntry(
    _Inout_ PPARTENTRY PartEntry,
    _In_opt_ ULONGLONG SizeBytes,
    _In_opt_ ULONG_PTR PartitionInfo)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    ULONGLONG SectorCount;
    BOOLEAN isContainer = IsContainerPartition((UCHAR)PartitionInfo);

    DPRINT1("Current entry sector count: %I64u\n", PartEntry->SectorCount.QuadPart);

    /* The entry must not be already partitioned and not be void */
    ASSERT(!PartEntry->IsPartitioned);
    ASSERT(PartEntry->SectorCount.QuadPart);
    ASSERT(!PartEntry->Volume);

    /* Either we create a primary/logical partition, or we create an
     * extended partition but the entry must not be logical space */
    ASSERT(!isContainer || !PartEntry->LogicalPartition);

    /* Convert the size in bytes to sector count. SizeBytes being
     * zero means the caller wants to use all the empty space. */
    if ((SizeBytes == 0) || (SizeBytes == GetPartEntrySizeInBytes(PartEntry)))
    {
        /* Use all of the unpartitioned disk space */
        SectorCount = PartEntry->SectorCount.QuadPart;
    }
    else
    {
        SectorCount = SizeBytes / DiskEntry->BytesPerSector;
        if (SectorCount == 0)
        {
            /* SizeBytes was certainly less than the minimal size, so fail */
            DPRINT1("Partition size %I64u too small\n", SizeBytes);
            return FALSE;
        }
    }
    DPRINT1("    New sector count: %I64u\n", SectorCount);

    /* Fail if we request more sectors than what the entry actually contains */
    if (SectorCount > PartEntry->SectorCount.QuadPart)
        return FALSE;

    if ((SectorCount == 0) ||
        (AlignDown(PartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment) -
                   PartEntry->StartSector.QuadPart == PartEntry->SectorCount.QuadPart))
    {
        /* Reuse the whole current entry */
    }
    else
    {
        ULONGLONG StartSector;
        ULONGLONG SectorCount2;
        PPARTENTRY NewPartEntry;

        /* Create a partition entry that represents the remaining space
         * after the partition to be initialized */

        StartSector = AlignDown(PartEntry->StartSector.QuadPart + SectorCount, DiskEntry->SectorAlignment);
        SectorCount2 = PartEntry->StartSector.QuadPart + PartEntry->SectorCount.QuadPart - StartSector;

        NewPartEntry = CreateInsertBlankRegion(DiskEntry,
                                               PartEntry->ListEntry.Flink,
                                               StartSector,
                                               SectorCount2,
                                               PartEntry->LogicalPartition);
        if (!NewPartEntry)
        {
            DPRINT1("Failed to create a new empty region for disk space!\n");
            return FALSE;
        }

        /* Resize down the partition entry; its StartSector remains the same */
        PartEntry->SectorCount.QuadPart = StartSector - PartEntry->StartSector.QuadPart;
    }

    /* Convert to a new partition entry */
    PartEntry->New = TRUE;
    PartEntry->IsPartitioned = TRUE;

    PartEntry->BootIndicator = FALSE;
    if (PartitionInfo)
    {
        if (!isContainer)
        {
            PartEntry->PartitionType = (UCHAR)PartitionInfo;
        }
        else
        {
            /* Set the correct extended container partition type,
             * depending on whether it is contained below or above
             * the 1024-cylinder (usually 8.4GB/7.8GiB) boundary:
             * - below: INT13h CHS partition;
             * - above: Extended INT13h LBA partition. */
            if ((PartEntry->StartSector.QuadPart + PartEntry->SectorCount.QuadPart - 1)
                  / (DiskEntry->TracksPerCylinder * DiskEntry->SectorsPerTrack) < 1024)
            {
                PartEntry->PartitionType = PARTITION_EXTENDED;
            }
            else
            {
                PartEntry->PartitionType = PARTITION_XINT13_EXTENDED;
            }
        }
    }
    else
    {
// FIXME: Use FileSystemToMBRPartitionType() only for MBR, otherwise use PARTITION_BASIC_DATA_GUID.
        ASSERT(!isContainer);
        PartEntry->PartitionType = FileSystemToMBRPartitionType(L"RAW",
                                                                PartEntry->StartSector.QuadPart,
                                                                PartEntry->SectorCount.QuadPart);
    }
    ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);

    if (isContainer)
    {
        DiskEntry->ExtendedPartition = PartEntry;
        AddLogicalDiskSpace(DiskEntry);
    }

    DPRINT1("First Sector : %I64u\n", PartEntry->StartSector.QuadPart);
    DPRINT1("Last Sector  : %I64u\n", PartEntry->StartSector.QuadPart + PartEntry->SectorCount.QuadPart - 1);
    DPRINT1("Total Sectors: %I64u\n", PartEntry->SectorCount.QuadPart);

    return TRUE;
}

static
VOID
InitPartitionDeviceName(
    _Inout_ PPARTENTRY PartEntry)
{
    NTSTATUS Status;

    /* Ignore if this is a container partition */
    if (IsContainerPartition(PartEntry->PartitionType))
        return;
    ASSERT(PartEntry->IsPartitioned && PartEntry->PartitionNumber != 0);

    /* Make a device name for the partition */
    Status = RtlStringCchPrintfW(PartEntry->DeviceName,
                                 _countof(PartEntry->DeviceName),
                                 L"\\Device\\Harddisk%lu\\Partition%lu",
                                 PartEntry->DiskEntry->DiskNumber,
                                 PartEntry->PartitionNumber);
    ASSERT(NT_SUCCESS(Status));
}

static
VOID
InitVolumeDeviceName(
    _Inout_ PVOLENTRY Volume)
{
    NTSTATUS Status;
    PPARTENTRY PartEntry;

    /* If we already have a volume device name, do nothing more */
    if (*Volume->Info.DeviceName)
        return;

    /* Use the partition device name as a temporary volume device name */
    // TODO: Ask instead the MOUNTMGR for the name.
    PartEntry = Volume->PartEntry;
    ASSERT(PartEntry);
    ASSERT(PartEntry->IsPartitioned && PartEntry->PartitionNumber != 0);

    /* Copy the volume device name */
    Status = RtlStringCchCopyW(Volume->Info.DeviceName,
                               _countof(Volume->Info.DeviceName),
                               PartEntry->DeviceName);
    ASSERT(NT_SUCCESS(Status));
}

static
PVOLENTRY
InitVolume(
    _In_ PPARTLIST List,
    _In_opt_ PPARTENTRY PartEntry)
{
    PVOLENTRY Volume;

    Volume = RtlAllocateHeap(ProcessHeap,
                             HEAP_ZERO_MEMORY,
                             sizeof(VOLENTRY));
    if (!Volume)
        return NULL;

    /* Reset some volume information */

    /* No device name for now */
    Volume->Info.DeviceName[0] = UNICODE_NULL;
    // Volume->Info.VolumeName[0] = UNICODE_NULL;

    /* Initialize the volume letter and label */
    Volume->Info.DriveLetter = UNICODE_NULL;
    Volume->Info.VolumeLabel[0] = UNICODE_NULL;

    /* Specify the volume as initially unformatted */
    Volume->Info.FileSystem[0] = UNICODE_NULL;
    Volume->FormatState = Unformatted;
    Volume->NeedsCheck = FALSE;
    Volume->New = TRUE;

    if (PartEntry)
    {
        ASSERT(PartEntry->DiskEntry->PartList == List);
        Volume->PartEntry = PartEntry;
    }
    InsertTailList(&List->VolumesList, &Volume->ListEntry);
    return Volume;
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

    /* Ignore empty partitions */
    if (PartitionInfo->PartitionType == PARTITION_ENTRY_UNUSED)
        return;
    /* Request must be consistent, though! */
    ASSERT(!(LogicalPartition && IsContainerPartition(PartitionInfo->PartitionType)));

    PartEntry = RtlAllocateHeap(ProcessHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(PARTENTRY));
    if (!PartEntry)
        return;

    PartEntry->DiskEntry = DiskEntry;

    PartEntry->StartSector.QuadPart = (ULONGLONG)PartitionInfo->StartingOffset.QuadPart / DiskEntry->BytesPerSector;
    PartEntry->SectorCount.QuadPart = (ULONGLONG)PartitionInfo->PartitionLength.QuadPart / DiskEntry->BytesPerSector;

    PartEntry->BootIndicator = PartitionInfo->BootIndicator;
    PartEntry->PartitionType = PartitionInfo->PartitionType;

    PartEntry->LogicalPartition = LogicalPartition;
    PartEntry->IsPartitioned = TRUE;
    PartEntry->OnDiskPartitionNumber = PartitionInfo->PartitionNumber;
    PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
    PartEntry->PartitionIndex = PartitionIndex;
    InitPartitionDeviceName(PartEntry);

    /* No volume initially */
    PartEntry->Volume = NULL;

    if (IsContainerPartition(PartEntry->PartitionType))
    {
        if (!LogicalPartition && DiskEntry->ExtendedPartition == NULL)
            DiskEntry->ExtendedPartition = PartEntry;
    }
    else if (IsRecognizedPartition(PartEntry->PartitionType) || // PartitionInfo->RecognizedPartition
             IsOEMPartition(PartEntry->PartitionType))
    {
        PVOLENTRY Volume;
        NTSTATUS Status;

        ASSERT(PartEntry->PartitionNumber != 0);

        /* The PARTMGR should have notified the MOUNTMGR that a volume
         * associated with this partition had to be created */
        Volume = InitVolume(DiskEntry->PartList, PartEntry);
        if (!Volume)
        {
            DPRINT1("Couldn't allocate a volume for device '%S'\n",
                    PartEntry->DeviceName);
            goto SkipVolume;
        }
        PartEntry->Volume = Volume;
        InitVolumeDeviceName(Volume);
        Volume->New = FALSE;

        /* Attach and mount the volume */
        Status = MountVolume(&Volume->Info, PartEntry->PartitionType);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to mount volume '%S', Status 0x%08lx\n",
                    Volume->Info.DeviceName, Status);
        }

        //
        // FIXME: TEMP Backward-compatibility: Set the FormatState
        // flag in accordance with the FileSystem volume value.
        //
        /*
         * MountVolume() determines whether the given volume is actually
         * unformatted, if it was mounted with RawFS and the partition
         * type has specific values for FAT volumes. If so, the volume
         * stays mounted with RawFS (the FileSystem is "RAW"). However,
         * if the partition type has different values, the volume is
         * considered as having an unknown format (it may or may not be
         * formatted) and the FileSystem value has been emptied.
         */
        if (IsUnknown(&Volume->Info))
            Volume->FormatState = UnknownFormat;
        else if (IsUnformatted(&Volume->Info)) // FileSystem is "RAW"
            Volume->FormatState = Unformatted;
        else // !IsUnknown && !IsUnformatted == IsFormatted
            Volume->FormatState = Formatted;
SkipVolume:;
    }
    else
    {
        /* Unknown partition (may or may not be actually formatted):
         * the partition is hidden, hence no volume */
        DPRINT1("Disk %lu Partition %lu is not recognized (Type 0x%02x)\n",
                DiskEntry->DiskNumber, PartEntry->PartitionNumber,
                PartEntry->PartitionType);
    }

    InsertDiskRegion(DiskEntry, PartEntry, LogicalPartition);
}

static
VOID
ScanForUnpartitionedDiskSpace(
    IN PDISKENTRY DiskEntry)
{
    ULONGLONG StartSector;
    ULONGLONG SectorCount;
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

        if (DiskEntry->SectorAlignment < 2048)
            StartSector = 2048ULL;
        else
            StartSector = (ULONGLONG)DiskEntry->SectorAlignment;
        SectorCount = AlignDown(DiskEntry->SectorCount.QuadPart, DiskEntry->SectorAlignment) - StartSector;

        NewPartEntry = CreateInsertBlankRegion(DiskEntry,
                                               &DiskEntry->PrimaryPartListHead,
                                               StartSector,
                                               SectorCount,
                                               FALSE);
        if (!NewPartEntry)
            DPRINT1("Failed to create a new empty region for full disk space!\n");

        return;
    }

    /* Start partition at head 1, cylinder 0 */
    if (DiskEntry->SectorAlignment < 2048)
        LastStartSector = 2048ULL;
    else
        LastStartSector = (ULONGLONG)DiskEntry->SectorAlignment;
    LastSectorCount = 0ULL;
    LastUnusedSectorCount = 0ULL;

    for (Entry = DiskEntry->PrimaryPartListHead.Flink;
         Entry != &DiskEntry->PrimaryPartListHead;
         Entry = Entry->Flink)
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

                StartSector = LastStartSector + LastSectorCount;
                SectorCount = AlignDown(StartSector + LastUnusedSectorCount, DiskEntry->SectorAlignment) - StartSector;

                /* Insert the table into the list */
                NewPartEntry = CreateInsertBlankRegion(DiskEntry,
                                                       &PartEntry->ListEntry,
                                                       StartSector,
                                                       SectorCount,
                                                       FALSE);
                if (!NewPartEntry)
                {
                    DPRINT1("Failed to create a new empty region for disk space!\n");
                    return;
                }
            }

            LastStartSector = PartEntry->StartSector.QuadPart;
            LastSectorCount = PartEntry->SectorCount.QuadPart;
        }
    }

    /* Check for trailing unpartitioned disk space */
    if ((LastStartSector + LastSectorCount) < DiskEntry->SectorCount.QuadPart)
    {
        LastUnusedSectorCount = AlignDown(DiskEntry->SectorCount.QuadPart - (LastStartSector + LastSectorCount), DiskEntry->SectorAlignment);

        if (LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
        {
            DPRINT("Unpartitioned disk space: %I64u sectors\n", LastUnusedSectorCount);

            StartSector = LastStartSector + LastSectorCount;
            SectorCount = AlignDown(StartSector + LastUnusedSectorCount, DiskEntry->SectorAlignment) - StartSector;

            /* Append the table to the list */
            NewPartEntry = CreateInsertBlankRegion(DiskEntry,
                                                   &DiskEntry->PrimaryPartListHead,
                                                   StartSector,
                                                   SectorCount,
                                                   FALSE);
            if (!NewPartEntry)
            {
                DPRINT1("Failed to create a new empty region for trailing disk space!\n");
                return;
            }
        }
    }

    if (DiskEntry->ExtendedPartition != NULL)
    {
        if (IsListEmpty(&DiskEntry->LogicalPartListHead))
        {
            DPRINT1("No logical partition!\n");

            /* Create a partition entry that represents the empty extended partition */
            AddLogicalDiskSpace(DiskEntry);
            return;
        }

        /* Start partition at head 1, cylinder 0 */
        LastStartSector = DiskEntry->ExtendedPartition->StartSector.QuadPart + (ULONGLONG)DiskEntry->SectorAlignment;
        LastSectorCount = 0ULL;
        LastUnusedSectorCount = 0ULL;

        for (Entry = DiskEntry->LogicalPartListHead.Flink;
             Entry != &DiskEntry->LogicalPartListHead;
             Entry = Entry->Flink)
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

                    StartSector = LastStartSector + LastSectorCount;
                    SectorCount = AlignDown(StartSector + LastUnusedSectorCount, DiskEntry->SectorAlignment) - StartSector;

                    /* Insert the table into the list */
                    NewPartEntry = CreateInsertBlankRegion(DiskEntry,
                                                           &PartEntry->ListEntry,
                                                           StartSector,
                                                           SectorCount,
                                                           TRUE);
                    if (!NewPartEntry)
                    {
                        DPRINT1("Failed to create a new empty region for extended partition space!\n");
                        return;
                    }
                }

                LastStartSector = PartEntry->StartSector.QuadPart;
                LastSectorCount = PartEntry->SectorCount.QuadPart;
            }
        }

        /* Check for trailing unpartitioned disk space */
        if ((LastStartSector + LastSectorCount) < DiskEntry->ExtendedPartition->StartSector.QuadPart + DiskEntry->ExtendedPartition->SectorCount.QuadPart)
        {
            LastUnusedSectorCount = AlignDown(DiskEntry->ExtendedPartition->StartSector.QuadPart +
                                              DiskEntry->ExtendedPartition->SectorCount.QuadPart - (LastStartSector + LastSectorCount),
                                              DiskEntry->SectorAlignment);

            if (LastUnusedSectorCount >= (ULONGLONG)DiskEntry->SectorAlignment)
            {
                DPRINT("Unpartitioned disk space: %I64u sectors\n", LastUnusedSectorCount);

                StartSector = LastStartSector + LastSectorCount;
                SectorCount = AlignDown(StartSector + LastUnusedSectorCount, DiskEntry->SectorAlignment) - StartSector;

                /* Append the table to the list */
                NewPartEntry = CreateInsertBlankRegion(DiskEntry,
                                                       &DiskEntry->LogicalPartListHead,
                                                       StartSector,
                                                       SectorCount,
                                                       TRUE);
                if (!NewPartEntry)
                {
                    DPRINT1("Failed to create a new empty region for extended partition space!\n");
                    return;
                }
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

    if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT("GPT-partitioned disk detected, not currently supported by SETUP!\n");
        return;
    }

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

        /* Check if the signature already exist */
        /* FIXME:
         *   Check also signatures from disks, which are
         *   not visible (bootable) by the BIOS.
         */
        for (Entry2 = List->DiskListHead.Flink;
             Entry2 != &List->DiskListHead;
             Entry2 = Entry2->Flink)
        {
            DiskEntry2 = CONTAINING_RECORD(Entry2, DISKENTRY, ListEntry);

            if (DiskEntry2->DiskStyle == PARTITION_STYLE_GPT)
            {
                DPRINT("GPT-partitioned disk detected, not currently supported by SETUP!\n");
                continue;
            }

            if (DiskEntry != DiskEntry2 &&
                DiskEntry->LayoutBuffer->Signature == DiskEntry2->LayoutBuffer->Signature)
                break;
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

    /* Update each disk */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
        {
            DPRINT("GPT-partitioned disk detected, not currently supported by SETUP!\n");
            continue;
        }

        if (DiskEntry->LayoutBuffer &&
            DiskEntry->LayoutBuffer->Signature == 0)
        {
            SetDiskSignature(List, DiskEntry);
            DiskEntry->LayoutBuffer->PartitionEntry[0].RewritePartition = TRUE;
        }
    }
}

static
VOID
UpdateHwDiskNumbers(
    IN PPARTLIST List)
{
    PLIST_ENTRY ListEntry;
    PBIOSDISKENTRY BiosDiskEntry;
    PDISKENTRY DiskEntry;
    ULONG HwAdapterNumber = 0;
    ULONG HwControllerNumber = 0;
    ULONG RemovableDiskCount = 0;

    /*
     * Enumerate the disks recognized by the BIOS and recompute the disk
     * numbers on the system when *ALL* removable disks are not connected.
     * The entries are inserted in increasing order of AdapterNumber,
     * ControllerNumber and DiskNumber.
     */
    for (ListEntry = List->BiosDiskListHead.Flink;
         ListEntry != &List->BiosDiskListHead;
         ListEntry = ListEntry->Flink)
    {
        BiosDiskEntry = CONTAINING_RECORD(ListEntry, BIOSDISKENTRY, ListEntry);
        DiskEntry = BiosDiskEntry->DiskEntry;

        /*
         * If the adapter or controller numbers change, update them and reset
         * the number of removable disks on this adapter/controller.
         */
        if (HwAdapterNumber != BiosDiskEntry->AdapterNumber ||
            HwControllerNumber != BiosDiskEntry->ControllerNumber)
        {
            HwAdapterNumber = BiosDiskEntry->AdapterNumber;
            HwControllerNumber = BiosDiskEntry->ControllerNumber;
            RemovableDiskCount = 0;
        }

        /* Adjust the actual hardware disk number */
        if (DiskEntry)
        {
            ASSERT(DiskEntry->HwDiskNumber == BiosDiskEntry->DiskNumber);

            if (DiskEntry->MediaType == RemovableMedia)
            {
                /* Increase the number of removable disks and set the disk number to zero */
                ++RemovableDiskCount;
                DiskEntry->HwFixedDiskNumber = 0;
            }
            else // if (DiskEntry->MediaType == FixedMedia)
            {
                /* Adjust the fixed disk number, offset by the number of removable disks found before this one */
                DiskEntry->HwFixedDiskNumber = BiosDiskEntry->DiskNumber - RemovableDiskCount;
            }
        }
        else
        {
            DPRINT1("BIOS disk %lu is not recognized by NTOS!\n", BiosDiskEntry->DiskNumber);
        }
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

    /* Retrieve the drive geometry */
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL,
                                   0,
                                   &DiskGeometry,
                                   sizeof(DiskGeometry));
    if (!NT_SUCCESS(Status))
        return;

    if (DiskGeometry.MediaType != FixedMedia &&
        DiskGeometry.MediaType != RemovableMedia)
    {
        return;
    }

    /*
     * FIXME: Here we suppose the disk is always SCSI. What if it is
     * of another type? To check this we need to retrieve the name of
     * the driver the disk device belongs to.
     */
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_SCSI_GET_ADDRESS,
                                   NULL,
                                   0,
                                   &ScsiAddress,
                                   sizeof(ScsiAddress));
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

    RtlStringCchPrintfW(Identifier, ARRAYSIZE(Identifier),
                        L"%08x-%08x-%c",
                        Checksum, Signature,
                        (Mbr->Magic == PARTITION_MAGIC) ? L'A' : L'X');
    DPRINT("Identifier: %S\n", Identifier);

    DiskEntry = RtlAllocateHeap(ProcessHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(DISKENTRY));
    if (DiskEntry == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, Mbr);
        DPRINT1("Failed to allocate a new disk entry.\n");
        return;
    }

    DiskEntry->PartList = List;

#if 0
    {
        FILE_FS_DEVICE_INFORMATION FileFsDevice;

        /* Query the device for its type */
        Status = NtQueryVolumeInformationFile(FileHandle,
                                              &Iosb,
                                              &FileFsDevice,
                                              sizeof(FileFsDevice),
                                              FileFsDeviceInformation);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Couldn't detect device type for disk %lu of identifier '%S'...\n", DiskNumber, Identifier);
        }
        else
        {
            DPRINT1("Disk %lu : DeviceType: 0x%08x ; Characteristics: 0x%08x\n", DiskNumber, FileFsDevice.DeviceType, FileFsDevice.Characteristics);
        }
    }
    // NOTE: We may also use NtQueryVolumeInformationFile(FileFsDeviceInformation).
#endif
    DiskEntry->MediaType = DiskGeometry.MediaType;
    if (DiskEntry->MediaType == RemovableMedia)
    {
        DPRINT1("Disk %lu of identifier '%S' is removable\n", DiskNumber, Identifier);
    }
    else // if (DiskEntry->MediaType == FixedMedia)
    {
        DPRINT1("Disk %lu of identifier '%S' is fixed\n", DiskNumber, Identifier);
    }

//    DiskEntry->Checksum = Checksum;
//    DiskEntry->Signature = Signature;
    DiskEntry->BiosFound = FALSE;

    /*
     * Check if this disk has a valid MBR: verify its signature,
     * and whether its two first bytes are a valid instruction
     * (related to this, see IsThereAValidBootSector() in partlist.c).
     *
     * See also ntoskrnl/fstub/fstubex.c!FstubDetectPartitionStyle().
     */

    // DiskEntry->NoMbr = (Mbr->Magic != PARTITION_MAGIC || (*(PUSHORT)Mbr->BootCode) == 0x0000);

    /* If we have not the 0xAA55 then it's raw partition */
    if (Mbr->Magic != PARTITION_MAGIC)
    {
        DiskEntry->DiskStyle = PARTITION_STYLE_RAW;
    }
    /* Check partitions types: if first is 0xEE and all the others 0, we have GPT */
    else if (Mbr->Partition[0].PartitionType == EFI_PMBR_OSTYPE_EFI &&
             Mbr->Partition[1].PartitionType == 0 &&
             Mbr->Partition[2].PartitionType == 0 &&
             Mbr->Partition[3].PartitionType == 0)
    {
        DiskEntry->DiskStyle = PARTITION_STYLE_GPT;
    }
    /* Otherwise, partition table is in MBR */
    else
    {
        DiskEntry->DiskStyle = PARTITION_STYLE_MBR;
    }

    /* Free the MBR sector buffer */
    RtlFreeHeap(ProcessHeap, 0, Mbr);


    for (ListEntry = List->BiosDiskListHead.Flink;
         ListEntry != &List->BiosDiskListHead;
         ListEntry = ListEntry->Flink)
    {
        BiosDiskEntry = CONTAINING_RECORD(ListEntry, BIOSDISKENTRY, ListEntry);
        /* FIXME:
         *   Compare the size from BIOS and the reported size from driver.
         *   If we have more than one disk with a zero or with the same signature
         *   we must create new signatures and reboot. After the reboot,
         *   it is possible to identify the disks.
         */
        if (BiosDiskEntry->Signature == Signature &&
            BiosDiskEntry->Checksum == Checksum &&
            BiosDiskEntry->DiskEntry == NULL)
        {
            if (!DiskEntry->BiosFound)
            {
                DiskEntry->HwAdapterNumber = BiosDiskEntry->AdapterNumber;
                DiskEntry->HwControllerNumber = BiosDiskEntry->ControllerNumber;
                DiskEntry->HwDiskNumber = BiosDiskEntry->DiskNumber;

                if (DiskEntry->MediaType == RemovableMedia)
                {
                    /* Set the removable disk number to zero */
                    DiskEntry->HwFixedDiskNumber = 0;
                }
                else // if (DiskEntry->MediaType == FixedMedia)
                {
                    /* The fixed disk number will later be adjusted using the number of removable disks */
                    DiskEntry->HwFixedDiskNumber = BiosDiskEntry->DiskNumber;
                }

                DiskEntry->BiosFound = TRUE;
                BiosDiskEntry->DiskEntry = DiskEntry;
                break;
            }
            else
            {
                // FIXME: What to do?
                DPRINT1("Disk %lu of identifier '%S' has already been found?!\n", DiskNumber, Identifier);
            }
        }
    }

    if (!DiskEntry->BiosFound)
    {
        DPRINT1("WARNING: Setup could not find a matching BIOS disk entry. Disk %lu may not be bootable by the BIOS!\n", DiskNumber);
    }

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

    InitializeListHead(&DiskEntry->PrimaryPartListHead);
    InitializeListHead(&DiskEntry->LogicalPartListHead);

    InsertAscendingList(&List->DiskListHead, DiskEntry, DISKENTRY, ListEntry, DiskNumber);


    /*
     * We now retrieve the disk partition layout
     */

    /*
     * Stop there now if the disk is GPT-partitioned,
     * since we currently do not support such disks.
     */
    if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT1("GPT-partitioned disk detected, not currently supported by SETUP!\n");
        return;
    }

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

    /* Keep looping while the drive layout buffer is too small */
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

    if (IsSuperFloppy(DiskEntry))
        DPRINT1("Disk %lu is a super-floppy\n", DiskNumber);

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
        {
            DiskEntry->LayoutBuffer->PartitionEntry[i].RewritePartition = TRUE;
        }
    }
    else
    {
        /* Enumerate and add the first four primary partitions */
        for (i = 0; i < 4; i++)
        {
            AddPartitionToDisk(DiskNumber, DiskEntry, i, FALSE);
        }

        /* Enumerate and add the remaining partitions as logical ones */
        for (i = 4; i < DiskEntry->LayoutBuffer->PartitionCount; i += 4)
        {
            AddPartitionToDisk(DiskNumber, DiskEntry, i, TRUE);
        }
    }

    ScanForUnpartitionedDiskSpace(DiskEntry);
}

/*
 * Retrieve the system disk, i.e. the fixed disk that is accessible by the
 * firmware during boot time and where the system partition resides.
 * If no system partition has been determined, we retrieve the first disk
 * that verifies the mentioned criteria above.
 */
static
PDISKENTRY
GetSystemDisk(
    IN PPARTLIST List)
{
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;

    /* Check for empty disk list */
    if (IsListEmpty(&List->DiskListHead))
        return NULL;

    /*
     * If we already have a system partition, the system disk
     * is the one on which the system partition resides.
     */
    if (List->SystemPartition)
        return List->SystemPartition->DiskEntry;

    /* Loop over the disks and find the correct one */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        /* The disk must be a fixed disk and be found by the firmware */
        if (DiskEntry->MediaType == FixedMedia && DiskEntry->BiosFound)
        {
            break;
        }
    }
    if (Entry == &List->DiskListHead)
    {
        /* We haven't encountered any suitable disk */
        return NULL;
    }

    if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT1("System disk -- GPT-partitioned disk detected, not currently supported by SETUP!\n");
    }

    return DiskEntry;
}

/*
 * Retrieve the actual "active" partition of the given disk.
 * On MBR disks, partition with the Active/Boot flag set;
 * on GPT disks, partition with the correct GUID.
 */
BOOLEAN
IsPartitionActive(
    IN PPARTENTRY PartEntry)
{
    // TODO: Support for GPT disks!

    if (IsContainerPartition(PartEntry->PartitionType))
        return FALSE;

    /* Check if the partition is partitioned, used and active */
    if (PartEntry->IsPartitioned &&
        // !IsContainerPartition(PartEntry->PartitionType) &&
        PartEntry->BootIndicator)
    {
        /* Yes it is */
        ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);
        return TRUE;
    }

    return FALSE;
}

static
PPARTENTRY
GetActiveDiskPartition(
    IN PDISKENTRY DiskEntry)
{
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    PPARTENTRY ActivePartition = NULL;

    /* Check for empty disk list */
    // ASSERT(DiskEntry);
    if (!DiskEntry)
        return NULL;

    /* Check for empty partition list */
    if (IsListEmpty(&DiskEntry->PrimaryPartListHead))
        return NULL;

    if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT1("GPT-partitioned disk detected, not currently supported by SETUP!\n");
        return NULL;
    }

    /* Scan all (primary) partitions to find the active disk partition */
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Retrieve the partition */
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (IsPartitionActive(PartEntry))
        {
            /* Yes, we've found it */
            ASSERT(DiskEntry == PartEntry->DiskEntry);
            ASSERT(PartEntry->IsPartitioned);
            ASSERT(PartEntry->Volume);

            ActivePartition = PartEntry;

            DPRINT1("Found active system partition %lu in disk %lu, drive letter %C\n",
                    PartEntry->PartitionNumber, DiskEntry->DiskNumber,
                    !PartEntry->Volume->Info.DriveLetter ? L'-' : PartEntry->Volume->Info.DriveLetter);
            break;
        }
    }

    /* Check if the disk is new and if so, use its first partition as the active system partition */
    if (DiskEntry->NewDisk && ActivePartition != NULL)
    {
        // FIXME: What to do??
        DPRINT1("NewDisk TRUE but already existing active partition?\n");
    }

    /* Return the active partition found (or none) */
    return ActivePartition;
}

PPARTLIST
NTAPI
CreatePartitionList(VOID)
{
    PPARTLIST List;
    PDISKENTRY SystemDisk;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SYSTEM_DEVICE_INFORMATION Sdi;
    IO_STATUS_BLOCK Iosb;
    ULONG ReturnSize;
    NTSTATUS Status;
    ULONG DiskNumber;
    HANDLE FileHandle;
    UNICODE_STRING Name;
    WCHAR Buffer[MAX_PATH];

    List = (PPARTLIST)RtlAllocateHeap(ProcessHeap,
                                      0,
                                      sizeof(PARTLIST));
    if (!List)
        return NULL;

    List->SystemPartition = NULL;

    InitializeListHead(&List->DiskListHead);
    InitializeListHead(&List->BiosDiskListHead);
    InitializeListHead(&List->VolumesList);

    /*
     * Enumerate the disks seen by the BIOS; this will be used later
     * to map drives seen by NTOS with their corresponding BIOS names.
     */
    EnumerateBiosDiskEntries(List);

    /* Enumerate disks seen by NTOS */
    Status = NtQuerySystemInformation(SystemDeviceInformation,
                                      &Sdi,
                                      sizeof(Sdi),
                                      &ReturnSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQuerySystemInformation() failed, Status 0x%08lx\n", Status);
        RtlFreeHeap(ProcessHeap, 0, List);
        return NULL;
    }

    for (DiskNumber = 0; DiskNumber < Sdi.NumberOfDisks; DiskNumber++)
    {
        RtlStringCchPrintfW(Buffer, ARRAYSIZE(Buffer),
                            L"\\Device\\Harddisk%lu\\Partition0",
                            DiskNumber);
        RtlInitUnicodeString(&Name, Buffer);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &Name,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenFile(&FileHandle,
                            FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                            &ObjectAttributes,
                            &Iosb,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if (NT_SUCCESS(Status))
        {
            AddDiskToList(FileHandle, DiskNumber, List);
            NtClose(FileHandle);
        }
    }

    UpdateDiskSignatures(List);
    UpdateHwDiskNumbers(List);
    AssignDriveLetters(List);

    /*
     * Retrieve the system partition: the active partition on the system
     * disk (the one that will be booted by default by the hardware).
     */
    SystemDisk = GetSystemDisk(List);
    List->SystemPartition = (SystemDisk ? GetActiveDiskPartition(SystemDisk) : NULL);

    return List;
}

VOID
NTAPI
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
            DestroyRegion(PartEntry);
        }

        /* Release logical partition list */
        while (!IsListEmpty(&DiskEntry->LogicalPartListHead))
        {
            Entry = RemoveHeadList(&DiskEntry->LogicalPartListHead);
            PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
            DestroyRegion(PartEntry);
        }

        /* Release layout buffer */
        if (DiskEntry->LayoutBuffer != NULL)
            RtlFreeHeap(ProcessHeap, 0, DiskEntry->LayoutBuffer);

        /* Release disk entry */
        RtlFreeHeap(ProcessHeap, 0, DiskEntry);
    }

    /* Release the BIOS disk info */
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
    _In_ PPARTLIST List,
    _In_ ULONG HwDiskNumber)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->HwDiskNumber == HwDiskNumber)
            return DiskEntry; /* Disk found, return it */
    }

    /* Disk not found, stop there */
    return NULL;
}

PDISKENTRY
GetDiskByNumber(
    _In_ PPARTLIST List,
    _In_ ULONG DiskNumber)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->DiskNumber == DiskNumber)
            return DiskEntry; /* Disk found, return it */
    }

    /* Disk not found, stop there */
    return NULL;
}

PDISKENTRY
GetDiskBySCSI(
    _In_ PPARTLIST List,
    _In_ USHORT Port,
    _In_ USHORT Bus,
    _In_ USHORT Id)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->Port == Port &&
            DiskEntry->Bus  == Bus  &&
            DiskEntry->Id   == Id)
        {
            /* Disk found, return it */
            return DiskEntry;
        }
    }

    /* Disk not found, stop there */
    return NULL;
}

PDISKENTRY
GetDiskBySignature(
    _In_ PPARTLIST List,
    _In_ ULONG Signature)
{
    PDISKENTRY DiskEntry;
    PLIST_ENTRY Entry;

    /* Loop over the disks and find the correct one */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->LayoutBuffer->Signature == Signature)
            return DiskEntry; /* Disk found, return it */
    }

    /* Disk not found, stop there */
    return NULL;
}

PPARTENTRY
GetPartition(
    _In_ PDISKENTRY DiskEntry,
    _In_ ULONG PartitionNumber)
{
    PPARTENTRY PartEntry;
    PLIST_ENTRY Entry;

    /* Forbid whole-disk or extended container partition access */
    if (PartitionNumber == 0)
        return NULL;

    /* Loop over the primary partitions first... */
    for (Entry = DiskEntry->PrimaryPartListHead.Flink;
         Entry != &DiskEntry->PrimaryPartListHead;
         Entry = Entry->Flink)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionNumber == PartitionNumber)
            return PartEntry; /* Partition found, return it */
    }

    if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
        return NULL;

    /* ... then over the logical partitions if needed */
    for (Entry = DiskEntry->LogicalPartListHead.Flink;
         Entry != &DiskEntry->LogicalPartListHead;
         Entry = Entry->Flink)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

        if (PartEntry->PartitionNumber == PartitionNumber)
            return PartEntry; /* Partition found, return it */
    }

    /* The partition was not found on the disk, stop there */
    return NULL;
}

PPARTENTRY
SelectPartition(
    _In_ PPARTLIST List,
    _In_ ULONG DiskNumber,
    _In_ ULONG PartitionNumber)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;

    /* Find the disk */
    DiskEntry = GetDiskByNumber(List, DiskNumber);
    if (!DiskEntry)
        return NULL;
    ASSERT(DiskEntry->DiskNumber == DiskNumber);

    /* Find the partition */
    PartEntry = GetPartition(DiskEntry, PartitionNumber);
    if (!PartEntry)
        return NULL;
    ASSERT(PartEntry->DiskEntry == DiskEntry);
    ASSERT(PartEntry->PartitionNumber == PartitionNumber);

    return PartEntry;
}

PPARTENTRY
NTAPI
GetNextPartition(
    IN PPARTLIST List,
    IN PPARTENTRY CurrentPart OPTIONAL)
{
    PLIST_ENTRY DiskListEntry;
    PLIST_ENTRY PartListEntry;
    PDISKENTRY CurrentDisk;

    /* Fail if no disks are available */
    if (IsListEmpty(&List->DiskListHead))
        return NULL;

    /* Check for the next usable entry on the current partition's disk */
    if (CurrentPart != NULL)
    {
        CurrentDisk = CurrentPart->DiskEntry;

        if (CurrentPart->LogicalPartition)
        {
            /* Logical partition */

            PartListEntry = CurrentPart->ListEntry.Flink;
            if (PartListEntry != &CurrentDisk->LogicalPartListHead)
            {
                /* Next logical partition */
                CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
                return CurrentPart;
            }
            else
            {
                PartListEntry = CurrentDisk->ExtendedPartition->ListEntry.Flink;
                if (PartListEntry != &CurrentDisk->PrimaryPartListHead)
                {
                    CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
                    return CurrentPart;
                }
            }
        }
        else
        {
            /* Primary or extended partition */

            if (CurrentPart->IsPartitioned &&
                IsContainerPartition(CurrentPart->PartitionType))
            {
                /* First logical partition */
                PartListEntry = CurrentDisk->LogicalPartListHead.Flink;
                if (PartListEntry != &CurrentDisk->LogicalPartListHead)
                {
                    CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
                    return CurrentPart;
                }
            }
            else
            {
                /* Next primary partition */
                PartListEntry = CurrentPart->ListEntry.Flink;
                if (PartListEntry != &CurrentDisk->PrimaryPartListHead)
                {
                    CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
                    return CurrentPart;
                }
            }
        }
    }

    /* Search for the first partition entry on the next disk */
    for (DiskListEntry = (CurrentPart ? CurrentDisk->ListEntry.Flink
                                      : List->DiskListHead.Flink);
         DiskListEntry != &List->DiskListHead;
         DiskListEntry = DiskListEntry->Flink)
    {
        CurrentDisk = CONTAINING_RECORD(DiskListEntry, DISKENTRY, ListEntry);

        if (CurrentDisk->DiskStyle == PARTITION_STYLE_GPT)
        {
            DPRINT("GPT-partitioned disk detected, not currently supported by SETUP!\n");
            continue;
        }

        PartListEntry = CurrentDisk->PrimaryPartListHead.Flink;
        if (PartListEntry != &CurrentDisk->PrimaryPartListHead)
        {
            CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
            return CurrentPart;
        }
    }

    return NULL;
}

PPARTENTRY
NTAPI
GetPrevPartition(
    IN PPARTLIST List,
    IN PPARTENTRY CurrentPart OPTIONAL)
{
    PLIST_ENTRY DiskListEntry;
    PLIST_ENTRY PartListEntry;
    PDISKENTRY CurrentDisk;

    /* Fail if no disks are available */
    if (IsListEmpty(&List->DiskListHead))
        return NULL;

    /* Check for the previous usable entry on the current partition's disk */
    if (CurrentPart != NULL)
    {
        CurrentDisk = CurrentPart->DiskEntry;

        if (CurrentPart->LogicalPartition)
        {
            /* Logical partition */

            PartListEntry = CurrentPart->ListEntry.Blink;
            if (PartListEntry != &CurrentDisk->LogicalPartListHead)
            {
                /* Previous logical partition */
                CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
            }
            else
            {
                /* Extended partition */
                CurrentPart = CurrentDisk->ExtendedPartition;
            }
            return CurrentPart;
        }
        else
        {
            /* Primary or extended partition */

            PartListEntry = CurrentPart->ListEntry.Blink;
            if (PartListEntry != &CurrentDisk->PrimaryPartListHead)
            {
                CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

                if (CurrentPart->IsPartitioned &&
                    IsContainerPartition(CurrentPart->PartitionType))
                {
                    PartListEntry = CurrentDisk->LogicalPartListHead.Blink;
                    CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
                }

                return CurrentPart;
            }
        }
    }

    /* Search for the last partition entry on the previous disk */
    for (DiskListEntry = (CurrentPart ? CurrentDisk->ListEntry.Blink
                                      : List->DiskListHead.Blink);
         DiskListEntry != &List->DiskListHead;
         DiskListEntry = DiskListEntry->Blink)
    {
        CurrentDisk = CONTAINING_RECORD(DiskListEntry, DISKENTRY, ListEntry);

        if (CurrentDisk->DiskStyle == PARTITION_STYLE_GPT)
        {
            DPRINT("GPT-partitioned disk detected, not currently supported by SETUP!\n");
            continue;
        }

        PartListEntry = CurrentDisk->PrimaryPartListHead.Blink;
        if (PartListEntry != &CurrentDisk->PrimaryPartListHead)
        {
            CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);

            if (CurrentPart->IsPartitioned &&
                IsContainerPartition(CurrentPart->PartitionType))
            {
                PartListEntry = CurrentDisk->LogicalPartListHead.Blink;
                if (PartListEntry != &CurrentDisk->LogicalPartListHead)
                {
                    CurrentPart = CONTAINING_RECORD(PartListEntry, PARTENTRY, ListEntry);
                    return CurrentPart;
                }
            }
            else
            {
                return CurrentPart;
            }
        }
    }

    return NULL;
}

static inline
BOOLEAN
IsEmptyLayoutEntry(
    _In_ PPARTITION_INFORMATION PartitionInfo)
{
    return (PartitionInfo->StartingOffset.QuadPart == 0 &&
            PartitionInfo->PartitionLength.QuadPart == 0);
}

static inline
BOOLEAN
IsSamePrimaryLayoutEntry(
    _In_ PPARTITION_INFORMATION PartitionInfo,
    _In_ PPARTENTRY PartEntry)
{
    return ((PartitionInfo->StartingOffset.QuadPart == GetPartEntryOffsetInBytes(PartEntry)) &&
            (PartitionInfo->PartitionLength.QuadPart == GetPartEntrySizeInBytes(PartEntry)));
//        PartitionInfo->PartitionType == PartEntry->PartitionType
}


/**
 * @brief
 * Counts the number of partitioned disk regions in a given partition list.
 **/
static
ULONG
GetPartitionCount(
    _In_ PLIST_ENTRY PartListHead)
{
    PLIST_ENTRY Entry;
    PPARTENTRY PartEntry;
    ULONG Count = 0;

    for (Entry = PartListHead->Flink;
         Entry != PartListHead;
         Entry = Entry->Flink)
    {
        PartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned)
            ++Count;
    }

    return Count;
}

#define GetPrimaryPartitionCount(DiskEntry) \
    GetPartitionCount(&(DiskEntry)->PrimaryPartListHead)

#define GetLogicalPartitionCount(DiskEntry) \
    (((DiskEntry)->DiskStyle == PARTITION_STYLE_MBR) \
        ? GetPartitionCount(&(DiskEntry)->LogicalPartListHead) : 0)


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

    DPRINT1("CurrentPartitionCount: %lu ; NewPartitionCount: %lu\n",
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

static
VOID
UpdateDiskLayout(
    IN PDISKENTRY DiskEntry)
{
    PPARTITION_INFORMATION PartitionInfo;
    PPARTITION_INFORMATION LinkInfo;
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    LARGE_INTEGER HiddenSectors64;
    ULONG Index;
    ULONG PartitionNumber = 1;

    DPRINT1("UpdateDiskLayout()\n");

    if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT1("GPT-partitioned disk detected, not currently supported by SETUP!\n");
        return;
    }

    /* Resize the layout buffer if necessary */
    if (!ReAllocateLayoutBuffer(DiskEntry))
    {
        DPRINT("ReAllocateLayoutBuffer() failed.\n");
        return;
    }

    /* Update the primary partition table */
    Index = 0;
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned)
        {
            ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);

            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];
            PartEntry->PartitionIndex = Index;

            /* Reset the current partition number only for not-yet written partitions */
            if (PartEntry->New)
                PartEntry->PartitionNumber = 0;

            PartEntry->OnDiskPartitionNumber = (!IsContainerPartition(PartEntry->PartitionType) ? PartitionNumber : 0);

            if (!IsSamePrimaryLayoutEntry(PartitionInfo, PartEntry))
            {
                DPRINT1("Updating primary partition entry %lu\n", Index);

                PartitionInfo->StartingOffset.QuadPart = GetPartEntryOffsetInBytes(PartEntry);
                PartitionInfo->PartitionLength.QuadPart = GetPartEntrySizeInBytes(PartEntry);
                PartitionInfo->HiddenSectors = PartEntry->StartSector.LowPart;
                PartitionInfo->PartitionNumber = PartEntry->PartitionNumber;
                PartitionInfo->PartitionType = PartEntry->PartitionType;
                PartitionInfo->BootIndicator = PartEntry->BootIndicator;
                PartitionInfo->RecognizedPartition = IsRecognizedPartition(PartEntry->PartitionType);
                PartitionInfo->RewritePartition = TRUE;
            }

            if (!IsContainerPartition(PartEntry->PartitionType))
                PartitionNumber++;

            Index++;
        }
    }

    ASSERT(Index <= 4);

    /* Update the logical partition table */
    LinkInfo = NULL;
    Index = 4;
    for (ListEntry = DiskEntry->LogicalPartListHead.Flink;
         ListEntry != &DiskEntry->LogicalPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        if (PartEntry->IsPartitioned)
        {
            ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);

            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[Index];
            PartEntry->PartitionIndex = Index;

            /* Reset the current partition number only for not-yet written partitions */
            if (PartEntry->New)
                PartEntry->PartitionNumber = 0;

            PartEntry->OnDiskPartitionNumber = PartitionNumber;

            DPRINT1("Updating logical partition entry %lu\n", Index);

            PartitionInfo->StartingOffset.QuadPart = GetPartEntryOffsetInBytes(PartEntry);
            PartitionInfo->PartitionLength.QuadPart = GetPartEntrySizeInBytes(PartEntry);
            PartitionInfo->HiddenSectors = DiskEntry->SectorAlignment;
            PartitionInfo->PartitionNumber = PartEntry->PartitionNumber;
            PartitionInfo->PartitionType = PartEntry->PartitionType;
            PartitionInfo->BootIndicator = FALSE;
            PartitionInfo->RecognizedPartition = IsRecognizedPartition(PartEntry->PartitionType);
            PartitionInfo->RewritePartition = TRUE;

            /* Fill the link entry of the previous partition entry */
            if (LinkInfo)
            {
                LinkInfo->StartingOffset.QuadPart = (PartEntry->StartSector.QuadPart - DiskEntry->SectorAlignment) * DiskEntry->BytesPerSector;
                LinkInfo->PartitionLength.QuadPart = (PartEntry->StartSector.QuadPart + DiskEntry->SectorAlignment) * DiskEntry->BytesPerSector;
                HiddenSectors64.QuadPart = PartEntry->StartSector.QuadPart - DiskEntry->SectorAlignment - DiskEntry->ExtendedPartition->StartSector.QuadPart;
                LinkInfo->HiddenSectors = HiddenSectors64.LowPart;
                LinkInfo->PartitionNumber = 0;

                /* Extended partition links only use type 0x05, as observed
                 * on Windows NT. Alternatively they could inherit the type
                 * of the main extended container. */
                LinkInfo->PartitionType = PARTITION_EXTENDED; // DiskEntry->ExtendedPartition->PartitionType;

                LinkInfo->BootIndicator = FALSE;
                LinkInfo->RecognizedPartition = FALSE;
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

    // HACK: See the FIXMEs in WritePartitions(): (Re)set the PartitionStyle to MBR.
    DiskEntry->DiskStyle = PARTITION_STYLE_MBR;

    DiskEntry->Dirty = TRUE;

#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(DiskEntry);
#endif
}

/**
 * @brief
 * Retrieves, if any, the unpartitioned disk region that is adjacent
 * (next or previous) to the specified partition.
 *
 * @param[in]   PartEntry
 * Partition from where to find the adjacent unpartitioned region.
 *
 * @param[in]   Direction
 * TRUE or FALSE to search the next or previous region, respectively.
 *
 * @return  The adjacent unpartitioned region, if it exists, or NULL.
 **/
PPARTENTRY
NTAPI
GetAdjUnpartitionedEntry(
    _In_ PPARTENTRY PartEntry,
    _In_ BOOLEAN Direction)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    PLIST_ENTRY ListHead, AdjEntry;

    /* In case of MBR disks only, check the logical partitions if necessary */
    if ((DiskEntry->DiskStyle == PARTITION_STYLE_MBR) &&
        PartEntry->LogicalPartition)
    {
        ListHead = &DiskEntry->LogicalPartListHead;
    }
    else
    {
        ListHead = &DiskEntry->PrimaryPartListHead;
    }

    if (Direction)
        AdjEntry = PartEntry->ListEntry.Flink; // Next region.
    else
        AdjEntry = PartEntry->ListEntry.Blink; // Previous region.

    if (AdjEntry != ListHead)
    {
        PartEntry = CONTAINING_RECORD(AdjEntry, PARTENTRY, ListEntry);
        if (!PartEntry->IsPartitioned)
        {
            ASSERT(PartEntry->PartitionType == PARTITION_ENTRY_UNUSED);
            return PartEntry;
        }
    }
    return NULL;
}

static ERROR_NUMBER
MBRPartitionCreateChecks(
    _In_ PPARTENTRY PartEntry,
    _In_opt_ ULONGLONG SizeBytes,
    _In_opt_ ULONG_PTR PartitionInfo)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    BOOLEAN isContainer = IsContainerPartition((UCHAR)PartitionInfo);

    // TODO: Re-enable once we initialize unpartitioned disks before using them.
    // ASSERT(DiskEntry->DiskStyle == PARTITION_STYLE_MBR);
    ASSERT(!PartEntry->IsPartitioned);

    if (isContainer)
    {
        /* Cannot create an extended partition within logical partition space */
        if (PartEntry->LogicalPartition)
            return ERROR_ONLY_ONE_EXTENDED;

        /* Fail if there is another extended partition in the list */
        if (DiskEntry->ExtendedPartition)
            return ERROR_ONLY_ONE_EXTENDED;
    }

    /*
     * Primary or Extended partitions
     */
    if (!PartEntry->LogicalPartition || isContainer)
    {
        /* Only one primary partition is allowed on super-floppy */
        if (IsSuperFloppy(DiskEntry))
            return ERROR_PARTITION_TABLE_FULL;

        /* Fail if there are too many primary partitions */
        if (GetPrimaryPartitionCount(DiskEntry) >= 4)
            return ERROR_PARTITION_TABLE_FULL;
    }
    /*
     * Logical partitions
     */
    else
    {
        // TODO: Check that we are inside an extended partition!
        // Then the following check will be useless.

        /* Only one (primary) partition is allowed on super-floppy */
        if (IsSuperFloppy(DiskEntry))
            return ERROR_PARTITION_TABLE_FULL;
    }

    return ERROR_SUCCESS;
}

ERROR_NUMBER
NTAPI
PartitionCreateChecks(
    _In_ PPARTENTRY PartEntry,
    _In_opt_ ULONGLONG SizeBytes,
    _In_opt_ ULONG_PTR PartitionInfo)
{
    // PDISKENTRY DiskEntry = PartEntry->DiskEntry;

    /* Fail if the partition is already in use */
    if (PartEntry->IsPartitioned)
        return ERROR_NEW_PARTITION;

    // TODO: Re-enable once we initialize unpartitioned disks before
    // using them; because such disks would be mistook as GPT otherwise.
    // if (DiskEntry->DiskStyle == PARTITION_STYLE_MBR)
    return MBRPartitionCreateChecks(PartEntry, SizeBytes, PartitionInfo);
#if 0
    else // if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT1("GPT-partitioned disk detected, not currently supported by SETUP!\n");
        return ERROR_WARN_PARTITION;
    }
#endif
}

// TODO: Improve upon the PartitionInfo parameter later
// (see VDS::CREATE_PARTITION_PARAMETERS and PPARTITION_INFORMATION_MBR/GPT for example)
// So far we only use it as the optional type of the partition to create.
BOOLEAN
NTAPI
CreatePartition(
    _In_ PPARTLIST List,
    _Inout_ PPARTENTRY PartEntry,
    _In_opt_ ULONGLONG SizeBytes,
    _In_opt_ ULONG_PTR PartitionInfo)
{
    ERROR_NUMBER Error;
    BOOLEAN isContainer = IsContainerPartition((UCHAR)PartitionInfo);
    PDISKENTRY DiskEntry;
    PCSTR mainType = "Primary";

    if (isContainer)
        mainType = "Extended";
    else if (PartEntry && PartEntry->LogicalPartition)
        mainType = "Logical";

    DPRINT1("CreatePartition(%s, %I64u bytes)\n", mainType, SizeBytes);

    if (!List || !PartEntry ||
        !PartEntry->DiskEntry || PartEntry->IsPartitioned)
    {
        return FALSE;
    }

    Error = PartitionCreateChecks(PartEntry, SizeBytes, PartitionInfo);
    if (Error != NOT_AN_ERROR)
    {
        DPRINT1("PartitionCreateChecks(%s) failed with error %lu\n", mainType, Error);
        return FALSE;
    }

    /* Initialize the partition entry, inserting a new blank region if needed */
    if (!InitializePartitionEntry(PartEntry, SizeBytes, PartitionInfo))
        return FALSE;

    DiskEntry = PartEntry->DiskEntry;
    UpdateDiskLayout(DiskEntry);

    ASSERT(!PartEntry->Volume);
    if (!isContainer)
    {
        /* We create a primary/logical partition: initialize a new basic
         * volume entry. When the partition will actually be written onto
         * the disk, the PARTMGR will notify the MOUNTMGR that a volume
         * associated with this partition has to be created. */
        PartEntry->Volume = InitVolume(DiskEntry->PartList, PartEntry);
        ASSERT(PartEntry->Volume);
    }

    AssignDriveLetters(List);

    return TRUE;
}

static NTSTATUS
DismountPartition(
    _In_ PPARTLIST List,
    _In_ PPARTENTRY PartEntry)
{
    NTSTATUS Status;
    PVOLENTRY Volume = PartEntry->Volume;

    ASSERT(PartEntry->DiskEntry->PartList == List);

    /* Check whether the partition is valid and was mounted by the system */
    if (!PartEntry->IsPartitioned ||
        IsContainerPartition(PartEntry->PartitionType)   ||
        !IsRecognizedPartition(PartEntry->PartitionType) ||
        !Volume || Volume->FormatState == UnknownFormat  ||
        // NOTE: If FormatState == Unformatted but *FileSystem != 0 this means
        // it has been usually mounted with RawFS and thus needs to be dismounted.
        PartEntry->PartitionNumber == 0)
    {
        /* The partition is not mounted, so just return success */
        return STATUS_SUCCESS;
    }

    ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);
    ASSERT(Volume->PartEntry == PartEntry);

    /* Unlink the basic volume from the volumes list and dismount it */
    PartEntry->Volume = NULL;
    Volume->PartEntry = NULL;
    RemoveEntryList(&Volume->ListEntry);
    Status = DismountVolume(&Volume->Info, TRUE);
    RtlFreeHeap(ProcessHeap, 0, Volume);
    return Status;
}

BOOLEAN
NTAPI
DeletePartition(
    _In_ PPARTLIST List,
    _In_ PPARTENTRY PartEntry,
    _Out_opt_ PPARTENTRY* FreeRegion)
{
    PDISKENTRY DiskEntry;
    PPARTENTRY PrevPartEntry;
    PPARTENTRY NextPartEntry;
    PPARTENTRY LogicalPartEntry;
    PLIST_ENTRY Entry;

    if (!List || !PartEntry ||
        !PartEntry->DiskEntry || !PartEntry->IsPartitioned)
    {
        return FALSE;
    }

    ASSERT(PartEntry->DiskEntry->PartList == List);
    ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);

    /* Clear the system partition if it is being deleted */
    if (List->SystemPartition == PartEntry)
    {
        ASSERT(List->SystemPartition);
        List->SystemPartition = NULL;
    }

    DiskEntry = PartEntry->DiskEntry;

    /* Check which type of partition (primary/logical or extended) is being deleted */
    if (DiskEntry->ExtendedPartition == PartEntry)
    {
        /* An extended partition is being deleted: delete all logical partition entries */
        while (!IsListEmpty(&DiskEntry->LogicalPartListHead))
        {
            Entry = RemoveHeadList(&DiskEntry->LogicalPartListHead);
            LogicalPartEntry = CONTAINING_RECORD(Entry, PARTENTRY, ListEntry);

            /* Dismount the logical partition and delete it */
            DismountPartition(List, LogicalPartEntry);
            DestroyRegion(LogicalPartEntry);
        }

        DiskEntry->ExtendedPartition = NULL;
    }
    else
    {
        /* A primary/logical partition is being deleted: dismount it */
        DismountPartition(List, PartEntry);
    }

    /* Adjust the unpartitioned disk space entries */

    /* Get pointer to previous and next unpartitioned entries */
    PrevPartEntry = GetAdjUnpartitionedEntry(PartEntry, FALSE);
    NextPartEntry = GetAdjUnpartitionedEntry(PartEntry, TRUE);

    if (PrevPartEntry != NULL && NextPartEntry != NULL)
    {
        /* Merge the previous, current and next unpartitioned entries */

        /* Adjust the previous entry length */
        PrevPartEntry->SectorCount.QuadPart += (PartEntry->SectorCount.QuadPart + NextPartEntry->SectorCount.QuadPart);

        /* Remove the current and next entries */
        RemoveEntryList(&PartEntry->ListEntry);
        DestroyRegion(PartEntry);
        RemoveEntryList(&NextPartEntry->ListEntry);
        DestroyRegion(NextPartEntry);

        /* Optionally return the freed region */
        if (FreeRegion)
            *FreeRegion = PrevPartEntry;
    }
    else if (PrevPartEntry != NULL && NextPartEntry == NULL)
    {
        /* Merge the current and the previous unpartitioned entries */

        /* Adjust the previous entry length */
        PrevPartEntry->SectorCount.QuadPart += PartEntry->SectorCount.QuadPart;

        /* Remove the current entry */
        RemoveEntryList(&PartEntry->ListEntry);
        DestroyRegion(PartEntry);

        /* Optionally return the freed region */
        if (FreeRegion)
            *FreeRegion = PrevPartEntry;
    }
    else if (PrevPartEntry == NULL && NextPartEntry != NULL)
    {
        /* Merge the current and the next unpartitioned entries */

        /* Adjust the next entry offset and length */
        NextPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
        NextPartEntry->SectorCount.QuadPart += PartEntry->SectorCount.QuadPart;

        /* Remove the current entry */
        RemoveEntryList(&PartEntry->ListEntry);
        DestroyRegion(PartEntry);

        /* Optionally return the freed region */
        if (FreeRegion)
            *FreeRegion = NextPartEntry;
    }
    else
    {
        /* Nothing to merge but change the current entry */
        PartEntry->New = FALSE;
        PartEntry->IsPartitioned = FALSE;
        PartEntry->PartitionType = PARTITION_ENTRY_UNUSED;
        PartEntry->OnDiskPartitionNumber = 0;
        PartEntry->PartitionNumber = 0;
        // PartEntry->PartitionIndex = 0;
        PartEntry->BootIndicator = FALSE;
        PartEntry->DeviceName[0] = UNICODE_NULL;

        if (PartEntry->Volume)
        {
            RemoveEntryList(&PartEntry->Volume->ListEntry);
            RtlFreeHeap(ProcessHeap, 0, PartEntry->Volume);
        }
        PartEntry->Volume = NULL;

        /* Optionally return the freed region */
        if (FreeRegion)
            *FreeRegion = PartEntry;
    }

    UpdateDiskLayout(DiskEntry);
    AssignDriveLetters(List);

    return TRUE;
}

static
BOOLEAN
IsSupportedActivePartition(
    IN PPARTENTRY PartEntry)
{
    PVOLENTRY Volume;

    /* Check the type and the file system of this partition */

    /*
     * We do not support extended partition containers (on MBR disks) marked
     * as active, and containing code inside their extended boot records.
     */
    if (IsContainerPartition(PartEntry->PartitionType))
    {
        DPRINT1("System partition %lu in disk %lu is an extended partition container?!\n",
                PartEntry->PartitionNumber, PartEntry->DiskEntry->DiskNumber);
        return FALSE;
    }

    Volume = PartEntry->Volume;
    if (!Volume)
    {
        /* Still no recognizable volume mounted: partition not supported */
        return FALSE;
    }

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
    if (Volume->FormatState == Unformatted)
    {
        /* If this partition is mounted, it would use RawFS ("RAW") */
        return TRUE;
    }
    else if (Volume->FormatState == Formatted)
    {
        ASSERT(*Volume->Info.FileSystem);

        /* NOTE: Please keep in sync with the RegisteredFileSystems list! */
        if (_wcsicmp(Volume->Info.FileSystem, L"FAT")   == 0 ||
            _wcsicmp(Volume->Info.FileSystem, L"FAT32") == 0 ||
         // _wcsicmp(Volume->Info.FileSystem, L"NTFS")  == 0 ||
            _wcsicmp(Volume->Info.FileSystem, L"BTRFS") == 0)
        {
            return TRUE;
        }
        else
        {
            // WARNING: We cannot write on this FS yet!
            DPRINT1("Recognized file system '%S' that doesn't have write support yet!\n",
                    Volume->Info.FileSystem);
            return FALSE;
        }
    }
    else // if (Volume->FormatState == UnknownFormat)
    {
        ASSERT(!*Volume->Info.FileSystem);

        DPRINT1("System partition %lu in disk %lu with no or unknown FS?!\n",
                PartEntry->PartitionNumber, PartEntry->DiskEntry->DiskNumber);
        return FALSE;
    }

    // HACK: WARNING: We cannot write on this FS yet!
    // See fsutil.c:InferFileSystem()
    if (PartEntry->PartitionType == PARTITION_IFS)
    {
        DPRINT1("Recognized file system '%S' that doesn't have write support yet!\n",
                Volume->Info.FileSystem);
        return FALSE;
    }

    return TRUE;
}

PPARTENTRY
FindSupportedSystemPartition(
    IN PPARTLIST List,
    IN BOOLEAN ForceSelect,
    IN PDISKENTRY AlternativeDisk OPTIONAL,
    IN PPARTENTRY AlternativePart OPTIONAL)
{
    PLIST_ENTRY ListEntry;
    PDISKENTRY DiskEntry;
    PPARTENTRY PartEntry;
    PPARTENTRY ActivePartition;
    PPARTENTRY CandidatePartition = NULL;

    /* Check for empty disk list */
    if (IsListEmpty(&List->DiskListHead))
    {
        /* No system partition! */
        ASSERT(List->SystemPartition == NULL);
        goto NoSystemPartition;
    }

    /* Adjust the optional alternative disk if needed */
    if (!AlternativeDisk && AlternativePart)
        AlternativeDisk = AlternativePart->DiskEntry;

    /* Ensure that the alternative partition is on the alternative disk */
    if (AlternativePart)
        ASSERT(AlternativeDisk && (AlternativePart->DiskEntry == AlternativeDisk));

    /* Ensure that the alternative disk is in the list */
    if (AlternativeDisk)
        ASSERT(AlternativeDisk->PartList == List);

    /* Start fresh */
    CandidatePartition = NULL;

//
// Step 1 : Check the system disk.
//

    /*
     * First, check whether the system disk, i.e. the one that will be booted
     * by default by the hardware, contains an active partition. If so this
     * should be our system partition.
     */
    DiskEntry = GetSystemDisk(List);
    if (!DiskEntry)
    {
        /* No system disk found, directly go check the alternative disk */
        goto UseAlternativeDisk;
    }

    if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT1("System disk -- GPT-partitioned disk detected, not currently supported by SETUP!\n");
        goto UseAlternativeDisk;
    }

    /* If we have a system partition (in the system disk), validate it */
    ActivePartition = List->SystemPartition;
    if (ActivePartition && IsSupportedActivePartition(ActivePartition))
    {
        CandidatePartition = ActivePartition;

        DPRINT1("Use the current system partition %lu in disk %lu, drive letter %C\n",
                CandidatePartition->PartitionNumber,
                CandidatePartition->DiskEntry->DiskNumber,
                !CandidatePartition->Volume->Info.DriveLetter ? L'-' : CandidatePartition->Volume->Info.DriveLetter);

        /* Return the candidate system partition */
        return CandidatePartition;
    }

    /* If the system disk is not the optional alternative disk, perform the minimal checks */
    if (DiskEntry != AlternativeDisk)
    {
        /*
         * No active partition has been recognized. Enumerate all the (primary)
         * partitions in the system disk, excluding the possible current active
         * partition, to find a new candidate.
         */
        for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
             ListEntry != &DiskEntry->PrimaryPartListHead;
             ListEntry = ListEntry->Flink)
        {
            /* Retrieve the partition */
            PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

            /* Skip the current active partition */
            if (PartEntry == ActivePartition)
                continue;

            /* Check if the partition is partitioned and used */
            if (PartEntry->IsPartitioned &&
                !IsContainerPartition(PartEntry->PartitionType))
            {
                ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);

                /* If we get a candidate active partition in the disk, validate it */
                if (IsSupportedActivePartition(PartEntry))
                {
                    CandidatePartition = PartEntry;
                    goto UseAlternativePartition;
                }
            }

#if 0
            /* Check if the partition is partitioned and used */
            if (!PartEntry->IsPartitioned)
            {
                ASSERT(PartEntry->PartitionType == PARTITION_ENTRY_UNUSED);

                // TODO: Check for minimal size!!
                CandidatePartition = PartEntry;
                goto UseAlternativePartition;
            }
#endif
        }

        /*
         * Still nothing, look whether there is some free space that we can use
         * for the new system partition. We must be sure that the total number
         * of partition is less than the maximum allowed, and that the minimal
         * size is fine.
         */
//
// TODO: Fix the handling of system partition being created in unpartitioned space!!
// --> When to partition it? etc...
//
        if (GetPrimaryPartitionCount(DiskEntry) < 4)
        {
            for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
                 ListEntry != &DiskEntry->PrimaryPartListHead;
                 ListEntry = ListEntry->Flink)
            {
                /* Retrieve the partition */
                PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

                /* Skip the current active partition */
                if (PartEntry == ActivePartition)
                    continue;

                /* Check for unpartitioned space */
                if (!PartEntry->IsPartitioned)
                {
                    ASSERT(PartEntry->PartitionType == PARTITION_ENTRY_UNUSED);

                    // TODO: Check for minimal size!!
                    CandidatePartition = PartEntry;
                    goto UseAlternativePartition;
                }
            }
        }
    }


//
// Step 2 : No active partition found: Check the alternative disk if specified.
//

UseAlternativeDisk:
    if (!AlternativeDisk || (!ForceSelect && (DiskEntry != AlternativeDisk)))
        goto NoSystemPartition;

    if (AlternativeDisk->DiskStyle == PARTITION_STYLE_GPT)
    {
        DPRINT1("Alternative disk -- GPT-partitioned disk detected, not currently supported by SETUP!\n");
        goto NoSystemPartition;
    }

    if (DiskEntry != AlternativeDisk)
    {
        /* Choose the alternative disk */
        DiskEntry = AlternativeDisk;

        /* If we get a candidate active partition, validate it */
        ActivePartition = GetActiveDiskPartition(DiskEntry);
        if (ActivePartition && IsSupportedActivePartition(ActivePartition))
        {
            CandidatePartition = ActivePartition;
            goto UseAlternativePartition;
        }
    }

    /* We now may have an unsupported active partition, or none */

/***
 *** TODO: Improve the selection:
 *** - If we want a really separate system partition from the partition where
 ***   we install, do something similar to what's done below in the code.
 *** - Otherwise if we allow for the system partition to be also the partition
 ***   where we install, just directly fall down to using AlternativePart.
 ***/

    /* Retrieve the first partition of the disk */
    PartEntry = CONTAINING_RECORD(DiskEntry->PrimaryPartListHead.Flink,
                                  PARTENTRY, ListEntry);
    ASSERT(DiskEntry == PartEntry->DiskEntry);

    CandidatePartition = PartEntry;

    //
    // See: https://svn.reactos.org/svn/reactos/trunk/reactos/base/setup/usetup/partlist.c?r1=63355&r2=63354&pathrev=63355#l2318
    //

    /* Check if the disk is new and if so, use its first partition as the active system partition */
    if (DiskEntry->NewDisk)
    {
        // !IsContainerPartition(PartEntry->PartitionType);
        if (!CandidatePartition->IsPartitioned || !CandidatePartition->BootIndicator) /* CandidatePartition != ActivePartition */
        {
            ASSERT(DiskEntry == CandidatePartition->DiskEntry);

            DPRINT1("Use new first active system partition %lu in disk %lu, drive letter %C\n",
                    CandidatePartition->PartitionNumber,
                    CandidatePartition->DiskEntry->DiskNumber,
                    !CandidatePartition->Volume->Info.DriveLetter ? L'-' : CandidatePartition->Volume->Info.DriveLetter);

            /* Return the candidate system partition */
            return CandidatePartition;
        }

        // FIXME: What to do??
        DPRINT1("NewDisk TRUE but first partition is used?\n");
    }

    /*
     * The disk is not new, check if any partition is initialized;
     * if not, the first one becomes the system partition.
     */
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Retrieve the partition */
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);

        /* Check if the partition is partitioned and is used */
        // !IsContainerPartition(PartEntry->PartitionType);
        if (/* PartEntry->IsPartitioned && */
            PartEntry->PartitionType != PARTITION_ENTRY_UNUSED || PartEntry->BootIndicator)
        {
            break;
        }
    }
    if (ListEntry == &DiskEntry->PrimaryPartListHead)
    {
        /*
         * OK we haven't encountered any used and active partition,
         * so use the first one as the system partition.
         */
        ASSERT(DiskEntry == CandidatePartition->DiskEntry);

        DPRINT1("Use first active system partition %lu in disk %lu, drive letter %C\n",
                CandidatePartition->PartitionNumber,
                CandidatePartition->DiskEntry->DiskNumber,
                !CandidatePartition->Volume->Info.DriveLetter ? L'-' : CandidatePartition->Volume->Info.DriveLetter);

        /* Return the candidate system partition */
        return CandidatePartition;
    }

    /*
     * The disk is not new, we did not find any actual active partition,
     * or the one we found was not supported, or any possible other candidate
     * is not supported. We then use the alternative partition if specified.
     */
    if (AlternativePart)
    {
        DPRINT1("No valid or supported system partition has been found, use the alternative partition!\n");
        CandidatePartition = AlternativePart;
        goto UseAlternativePartition;
    }
    else
    {
NoSystemPartition:
        DPRINT1("No valid or supported system partition has been found on this system!\n");
        return NULL;
    }

UseAlternativePartition:
    /*
     * We are here because we did not find any (active) candidate system
     * partition that we know how to support. What we are going to do is
     * to change the existing system partition and use the alternative partition
     * (e.g. on which we install ReactOS) as the new system partition.
     * Then we will need to add in FreeLdr's boot menu an entry for booting
     * from the original system partition.
     */
    ASSERT(CandidatePartition);

    DPRINT1("Use alternative active system partition %lu in disk %lu, drive letter %C\n",
            CandidatePartition->PartitionNumber,
            CandidatePartition->DiskEntry->DiskNumber,
            !CandidatePartition->Volume->Info.DriveLetter ? L'-' : CandidatePartition->Volume->Info.DriveLetter);

    /* Return the candidate system partition */
    return CandidatePartition;
}

BOOLEAN
SetActivePartition(
    IN PPARTLIST List,
    IN PPARTENTRY PartEntry,
    IN PPARTENTRY OldActivePart OPTIONAL)
{
    /* Check for empty disk list */
    if (IsListEmpty(&List->DiskListHead))
        return FALSE;

    /* Validate the partition entry */
    if (!PartEntry)
        return FALSE;

    /*
     * If the partition entry is already the system partition, or if it is
     * the same as the old active partition hint the user provided (and if
     * it is already active), just return success.
     */
    if ((PartEntry == List->SystemPartition) ||
        ((PartEntry == OldActivePart) && IsPartitionActive(OldActivePart)))
    {
        return TRUE;
    }

    ASSERT(PartEntry->DiskEntry);

    /* Ensure that the partition's disk is in the list */
    ASSERT(PartEntry->DiskEntry->PartList == List);

    /*
     * If the user provided an old active partition hint, verify that it is
     * indeed active and belongs to the same disk where the new partition
     * belongs. Otherwise determine the current active partition on the disk
     * where the new partition belongs.
     */
    if (!(OldActivePart && IsPartitionActive(OldActivePart) && (OldActivePart->DiskEntry == PartEntry->DiskEntry)))
    {
        /* It's not, determine the current active partition for the disk */
        OldActivePart = GetActiveDiskPartition(PartEntry->DiskEntry);
    }

    /* Unset the old active partition if it exists */
    if (OldActivePart)
    {
        OldActivePart->BootIndicator = FALSE;
        OldActivePart->DiskEntry->LayoutBuffer->PartitionEntry[OldActivePart->PartitionIndex].BootIndicator = FALSE;
        OldActivePart->DiskEntry->LayoutBuffer->PartitionEntry[OldActivePart->PartitionIndex].RewritePartition = TRUE;
        OldActivePart->DiskEntry->Dirty = TRUE;
    }

    /* Modify the system partition if the new partition is on the system disk */
    if (PartEntry->DiskEntry == GetSystemDisk(List))
        List->SystemPartition = PartEntry;

    /* Set the new active partition */
    PartEntry->BootIndicator = TRUE;
    PartEntry->DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].BootIndicator = TRUE;
    PartEntry->DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].RewritePartition = TRUE;
    PartEntry->DiskEntry->Dirty = TRUE;

    return TRUE;
}

NTSTATUS
WritePartitions(
    IN PDISKENTRY DiskEntry)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE FileHandle;
    IO_STATUS_BLOCK Iosb;
    ULONG BufferSize;
    PPARTITION_INFORMATION PartitionInfo;
    ULONG PartitionCount;
    PLIST_ENTRY ListEntry;
    PPARTENTRY PartEntry;
    WCHAR DstPath[MAX_PATH];

    DPRINT("WritePartitions() Disk: %lu\n", DiskEntry->DiskNumber);

    /* If the disk is not dirty, there is nothing to do */
    if (!DiskEntry->Dirty)
        return STATUS_SUCCESS;

    RtlStringCchPrintfW(DstPath, ARRAYSIZE(DstPath),
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

#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(DiskEntry);
#endif

    //
    // FIXME: We first *MUST* use IOCTL_DISK_CREATE_DISK to initialize
    // the disk in MBR or GPT format in case the disk was not initialized!!
    // For this we must ask the user which format to use.
    //

    /* Save the original partition count to be restored later (see comment below) */
    PartitionCount = DiskEntry->LayoutBuffer->PartitionCount;

    /* Set the new disk layout and retrieve its updated version with
     * new partition numbers for the new partitions. The PARTMGR will
     * automatically notify the MOUNTMGR of new or deleted volumes. */
    BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
                 ((PartitionCount - 1) * sizeof(PARTITION_INFORMATION));
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Iosb,
                                   IOCTL_DISK_SET_DRIVE_LAYOUT,
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

#ifdef DUMP_PARTITION_TABLE
    DumpPartitionTable(DiskEntry);
#endif

    /* Update the partition numbers and device names */

    /* Update the primary partition table */
    for (ListEntry = DiskEntry->PrimaryPartListHead.Flink;
         ListEntry != &DiskEntry->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (!PartEntry->IsPartitioned)
            continue;
        ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);

        /*
         * Initialize the partition's number and its device name only
         * if the partition was new. Note that the partition number
         * should not change if this partition has not been deleted
         * during repartitioning.
         */
        // FIXME: Our PartMgr currently returns modified numbers
        // in the layout, this needs to be investigated and fixed.
        if (PartEntry->New)
        {
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex];
            PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
            InitPartitionDeviceName(PartEntry);
        }
        PartEntry->New = FALSE;
    }

    /* Update the logical partition table */
    for (ListEntry = DiskEntry->LogicalPartListHead.Flink;
         ListEntry != &DiskEntry->LogicalPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (!PartEntry->IsPartitioned)
            continue;
        ASSERT(PartEntry->PartitionType != PARTITION_ENTRY_UNUSED);

        /* See comment above */
        if (PartEntry->New)
        {
            PartitionInfo = &DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex];
            PartEntry->PartitionNumber = PartitionInfo->PartitionNumber;
            InitPartitionDeviceName(PartEntry);
        }
        PartEntry->New = FALSE;
    }

    //
    // NOTE: Originally (see r40437), we used to install here also a new MBR
    // for this disk (by calling InstallMbrBootCodeToDisk), only if:
    // DiskEntry->NewDisk == TRUE and DiskEntry->HwDiskNumber == 0.
    // Then after that, both DiskEntry->NewDisk and DiskEntry->NoMbr were set
    // to FALSE. In the other place (in usetup.c) where InstallMbrBootCodeToDisk
    // was called too, the installation test was modified by checking whether
    // DiskEntry->NoMbr was TRUE (instead of NewDisk).
    //

    // HACK: Parts of FIXMEs described above: (Re)set the PartitionStyle to MBR.
    DiskEntry->DiskStyle = PARTITION_STYLE_MBR;

    /* The layout has been successfully updated, the disk is not dirty anymore */
    DiskEntry->Dirty = FALSE;

    return Status;
}

BOOLEAN
WritePartitionsToDisk(
    IN PPARTLIST List)
{
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PDISKENTRY DiskEntry;
    PVOLENTRY Volume;

    if (!List)
        return TRUE;

    /* Write all the partitions to all the disks */
    for (Entry = List->DiskListHead.Flink;
         Entry != &List->DiskListHead;
         Entry = Entry->Flink)
    {
        DiskEntry = CONTAINING_RECORD(Entry, DISKENTRY, ListEntry);

        if (DiskEntry->DiskStyle == PARTITION_STYLE_GPT)
        {
            DPRINT("GPT-partitioned disk detected, not currently supported by SETUP!\n");
            continue;
        }

        if (DiskEntry->Dirty != FALSE)
        {
            Status = WritePartitions(DiskEntry);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("WritePartitionsToDisk() failed to update disk %lu, Status 0x%08lx\n",
                        DiskEntry->DiskNumber, Status);
            }
        }
    }

    /* The PARTMGR should have notified the MOUNTMGR that new volumes
     * associated with the new partitions had to be created */

    /* Assign valid device names to new volumes */
    for (Entry = List->VolumesList.Flink;
         Entry != &List->VolumesList;
         Entry = Entry->Flink)
    {
        Volume = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);
        InitVolumeDeviceName(Volume);
    }

    return TRUE;
}


/**
 * @brief
 * Assign a "\DosDevices\#:" mount point drive letter to a disk partition or
 * volume, specified by a given disk signature and starting partition offset.
 **/
static BOOLEAN
SetMountedDeviceValue(
    _In_ PVOLENTRY Volume)
{
    PPARTENTRY PartEntry = Volume->PartEntry;
    WCHAR Letter = Volume->Info.DriveLetter;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"SYSTEM\\MountedDevices");
    UNICODE_STRING ValueName;
    WCHAR Buffer[16];
    HANDLE KeyHandle;
    REG_DISK_MOUNT_INFO MountInfo;

    /* Ignore no letter */
    if (!Letter)
        return TRUE;

    RtlStringCchPrintfW(Buffer, _countof(Buffer),
                        L"\\DosDevices\\%c:", Letter);
    RtlInitUnicodeString(&ValueName, Buffer);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_LOCAL_MACHINE, NULL),
                               NULL);

    Status = NtOpenKey(&KeyHandle,
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

    MountInfo.Signature = PartEntry->DiskEntry->LayoutBuffer->Signature;
    MountInfo.StartingOffset = GetPartEntryOffsetInBytes(PartEntry);
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
    _In_ PPARTLIST List)
{
    PLIST_ENTRY Entry;
    PVOLENTRY Volume;

    if (!List)
        return FALSE;

    for (Entry = List->VolumesList.Flink;
         Entry != &List->VolumesList;
         Entry = Entry->Flink)
    {
        Volume = CONTAINING_RECORD(Entry, VOLENTRY, ListEntry);

        /* Assign a "\DosDevices\#:" mount point to this volume */
        if (!SetMountedDeviceValue(Volume))
            return FALSE;
    }

    return TRUE;
}

VOID
SetMBRPartitionType(
    IN PPARTENTRY PartEntry,
    IN UCHAR PartitionType)
{
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;

    ASSERT(DiskEntry->DiskStyle == PARTITION_STYLE_MBR);

    /* Nothing to do if we assign the same type */
    if (PartitionType == PartEntry->PartitionType)
        return;

    // TODO: We might need to remount the associated basic volume...

    PartEntry->PartitionType = PartitionType;

    DiskEntry->Dirty = TRUE;
    DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].PartitionType = PartitionType;
    DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].RecognizedPartition = IsRecognizedPartition(PartitionType);
    DiskEntry->LayoutBuffer->PartitionEntry[PartEntry->PartitionIndex].RewritePartition = TRUE;
}

/* EOF */
