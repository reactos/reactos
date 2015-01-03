/*
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(HWDETECT);

static CHAR Hex[] = "0123456789ABCDEF";
//static unsigned int delay_count = 1;

extern ULONG reactos_disk_count;
extern ARC_DISK_SIGNATURE reactos_arc_disk_info[];
extern CHAR reactos_arc_strings[32][256];

static
PCM_PARTIAL_RESOURCE_LIST
GetHarddiskConfigurationData(UCHAR DriveNumber, ULONG* pSize)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    //EXTENDED_GEOMETRY ExtGeometry;
    GEOMETRY Geometry;
    ULONG Size;

    //
    // Initialize returned size
    //
    *pSize = 0;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate a full resource descriptor\n");
        return NULL;
    }

    memset(PartialResourceList, 0, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;
    PartialResourceList->PartialDescriptors[0].Type =
        CmResourceTypeDeviceSpecific;
//  PartialResourceList->PartialDescriptors[0].ShareDisposition =
//  PartialResourceList->PartialDescriptors[0].Flags =
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        sizeof(CM_DISK_GEOMETRY_DEVICE_DATA);

    /* Get pointer to geometry data */
    DiskGeometry = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));

    /* Get the disk geometry */
    //ExtGeometry.Size = sizeof(EXTENDED_GEOMETRY);

    if (MachDiskGetDriveGeometry(DriveNumber, &Geometry))
    {
        DiskGeometry->BytesPerSector = Geometry.BytesPerSector;
        DiskGeometry->NumberOfCylinders = Geometry.Cylinders;
        DiskGeometry->SectorsPerTrack = Geometry.Sectors;
        DiskGeometry->NumberOfHeads = Geometry.Heads;
    }
    else
    {
        ERR("Reading disk geometry failed\n");
        FrLdrHeapFree(PartialResourceList, TAG_HW_RESOURCE_LIST);
        return NULL;
    }
    TRACE("Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
          DriveNumber,
          DiskGeometry->NumberOfCylinders,
          DiskGeometry->NumberOfHeads,
          DiskGeometry->SectorsPerTrack,
          DiskGeometry->BytesPerSector);

    //
    // Return configuration data
    //
    *pSize = Size;
    return PartialResourceList;
}

typedef struct tagDISKCONTEXT
{
    UCHAR DriveNumber;
    ULONG SectorSize;
    ULONGLONG SectorOffset;
    ULONGLONG SectorCount;
    ULONGLONG SectorNumber;
} DISKCONTEXT;

static
ARC_STATUS
DiskClose(ULONG FileId)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);

    FrLdrTempFree(Context, TAG_HW_DISK_CONTEXT);
    return ESUCCESS;
}

static
ARC_STATUS
DiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(FILEINFORMATION));
    Information->EndingAddress.QuadPart = (Context->SectorOffset + Context->SectorCount) * Context->SectorSize;
    Information->CurrentAddress.QuadPart = (Context->SectorOffset + Context->SectorNumber) * Context->SectorSize;

    return ESUCCESS;
}

static
ARC_STATUS
DiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    DISKCONTEXT* Context;
    ULONG DrivePartition, SectorSize;
    UCHAR DriveNumber;
    ULONGLONG SectorOffset = 0;
    ULONGLONG SectorCount = 0;
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    CHAR FileName[1];

    if (!DissectArcPath(Path, FileName, &DriveNumber, &DrivePartition))
        return EINVAL;

    if (DrivePartition == 0xff)
    {
        /* This is a CD-ROM device */
        SectorSize = 2048;
    }
    else
    {
        /* This is either a floppy disk device (DrivePartition == 0) or
         * a hard disk device (DrivePartition != 0 && DrivePartition != 0xFF) but
         * it doesn't matter which one because they both have 512 bytes per sector */
        SectorSize = 512;
    }

    if (DrivePartition != 0xff && DrivePartition != 0)
    {
        if (!XboxDiskGetPartitionEntry(DriveNumber, DrivePartition, &PartitionTableEntry))
            return EINVAL;
        SectorOffset = PartitionTableEntry.SectorCountBeforePartition;
        SectorCount = PartitionTableEntry.PartitionSectorCount;
    }
    else
    {
        SectorCount = 0; /* FIXME */
    }

    Context = FrLdrTempAlloc(sizeof(DISKCONTEXT), TAG_HW_DISK_CONTEXT);
    if (!Context)
        return ENOMEM;
    Context->DriveNumber = DriveNumber;
    Context->SectorSize = SectorSize;
    Context->SectorOffset = SectorOffset;
    Context->SectorCount = SectorCount;
    Context->SectorNumber = 0;
    FsSetDeviceSpecific(*FileId, Context);

    return ESUCCESS;
}

static
ARC_STATUS
DiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    UCHAR* Ptr = (UCHAR*)Buffer;
    ULONG i, Length, Sectors;
    BOOLEAN ret;

    *Count = 0;
    i = 0;
    while (N > 0)
    {
        Length = N;
        if (Length > PcDiskReadBufferSize)
            Length = PcDiskReadBufferSize;
        Sectors = (Length + Context->SectorSize - 1) / Context->SectorSize;
        ret = MachDiskReadLogicalSectors(
                  Context->DriveNumber,
                  Context->SectorNumber + Context->SectorOffset + i,
                  Sectors,
                  (PVOID)DISKREADBUFFER);
        if (!ret)
            return EIO;
        RtlCopyMemory(Ptr, (PVOID)DISKREADBUFFER, Length);
        Ptr += Length;
        *Count += Length;
        N -= Length;
        i += Sectors;
    }

    return ESUCCESS;
}

static
ARC_STATUS
DiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);

    if (SeekMode != SeekAbsolute)
        return EINVAL;
    if (Position->LowPart & (Context->SectorSize - 1))
        return EINVAL;

    /* FIXME: take HighPart into account */
    Context->SectorNumber = Position->LowPart / Context->SectorSize;
    return ESUCCESS;
}

static const DEVVTBL DiskVtbl =
{
    DiskClose,
    DiskGetFileInformation,
    DiskOpen,
    DiskRead,
    DiskSeek,
};

static
VOID
GetHarddiskIdentifier(PCHAR Identifier,
                      UCHAR DriveNumber)
{
    PMASTER_BOOT_RECORD Mbr;
    ULONG *Buffer;
    ULONG i;
    ULONG Checksum;
    ULONG Signature;
    CHAR ArcName[256];
    PARTITION_TABLE_ENTRY PartitionTableEntry;

    /* Read the MBR */
    if (!MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
        ERR("Reading MBR failed\n");
        return;
    }

    Buffer = (ULONG*)DISKREADBUFFER;
    Mbr = (PMASTER_BOOT_RECORD)DISKREADBUFFER;

    Signature =  Mbr->Signature;
    TRACE("Signature: %x\n", Signature);

    /* Calculate the MBR checksum */
    Checksum = 0;
    for (i = 0; i < 128; i++)
    {
        Checksum += Buffer[i];
    }
    Checksum = ~Checksum + 1;
    TRACE("Checksum: %x\n", Checksum);

    /* Fill out the ARC disk block */
    reactos_arc_disk_info[reactos_disk_count].Signature = Signature;
    reactos_arc_disk_info[reactos_disk_count].CheckSum = Checksum;
    sprintf(ArcName, "multi(0)disk(0)rdisk(%lu)", reactos_disk_count);
    strcpy(reactos_arc_strings[reactos_disk_count], ArcName);
    reactos_arc_disk_info[reactos_disk_count].ArcName =
        reactos_arc_strings[reactos_disk_count];
    reactos_disk_count++;

    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(0)", DriveNumber - 0x80);
    FsRegisterDevice(ArcName, &DiskVtbl);

    /* Add partitions */
    i = 1;
    DiskReportError(FALSE);
    while (XboxDiskGetPartitionEntry(DriveNumber, i, &PartitionTableEntry))
    {
        if (PartitionTableEntry.SystemIndicator != PARTITION_ENTRY_UNUSED)
        {
            sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(%lu)", DriveNumber - 0x80, i);
            FsRegisterDevice(ArcName, &DiskVtbl);
        }
        i++;
    }
    DiskReportError(TRUE);

    /* Convert checksum and signature to identifier string */
    Identifier[0] = Hex[(Checksum >> 28) & 0x0F];
    Identifier[1] = Hex[(Checksum >> 24) & 0x0F];
    Identifier[2] = Hex[(Checksum >> 20) & 0x0F];
    Identifier[3] = Hex[(Checksum >> 16) & 0x0F];
    Identifier[4] = Hex[(Checksum >> 12) & 0x0F];
    Identifier[5] = Hex[(Checksum >> 8) & 0x0F];
    Identifier[6] = Hex[(Checksum >> 4) & 0x0F];
    Identifier[7] = Hex[Checksum & 0x0F];
    Identifier[8] = '-';
    Identifier[9] = Hex[(Signature >> 28) & 0x0F];
    Identifier[10] = Hex[(Signature >> 24) & 0x0F];
    Identifier[11] = Hex[(Signature >> 20) & 0x0F];
    Identifier[12] = Hex[(Signature >> 16) & 0x0F];
    Identifier[13] = Hex[(Signature >> 12) & 0x0F];
    Identifier[14] = Hex[(Signature >> 8) & 0x0F];
    Identifier[15] = Hex[(Signature >> 4) & 0x0F];
    Identifier[16] = Hex[Signature & 0x0F];
    Identifier[17] = '-';
    Identifier[18] = 'A';
    Identifier[19] = 0;
    TRACE("Identifier: %s\n", Identifier);
}

static
VOID
DetectBiosDisks(PCONFIGURATION_COMPONENT_DATA SystemKey,
                PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_INT13_DRIVE_PARAMETER Int13Drives;
    GEOMETRY Geometry;
    PCONFIGURATION_COMPONENT_DATA DiskKey, ControllerKey;
    UCHAR DiskCount;
    ULONG i;
    ULONG Size;
    BOOLEAN Changed;

    /* Count the number of visible drives */
    DiskReportError(FALSE);
    DiskCount = 0;

    /* There are some really broken BIOSes out there. There are even BIOSes
        * that happily report success when you ask them to read from non-existent
        * harddisks. So, we set the buffer to known contents first, then try to
        * read. If the BIOS reports success but the buffer contents haven't
        * changed then we fail anyway */
    memset((PVOID) DISKREADBUFFER, 0xcd, PcDiskReadBufferSize);
    while (MachDiskReadLogicalSectors(0x80 + DiskCount, 0ULL, 1, (PVOID)DISKREADBUFFER))
    {
        Changed = FALSE;
        for (i = 0; ! Changed && i < PcDiskReadBufferSize; i++)
        {
            Changed = ((PUCHAR)DISKREADBUFFER)[i] != 0xcd;
        }
        if (! Changed)
        {
            TRACE("BIOS reports success for disk %d but data didn't change\n",
                  (int)DiskCount);
            break;
        }
        DiskCount++;
        memset((PVOID) DISKREADBUFFER, 0xcd, PcDiskReadBufferSize);
    }
    DiskReportError(TRUE);
    TRACE("BIOS reports %d harddisk%s\n",
          (int)DiskCount, (DiskCount == 1) ? "" : "s");

    //DetectBiosFloppyController(BusKey);

    /* Allocate resource descriptor */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) +
           sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    memset(PartialResourceList, 0, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 1;
    PartialResourceList->PartialDescriptors[0].Type = CmResourceTypeDeviceSpecific;
    PartialResourceList->PartialDescriptors[0].ShareDisposition = 0;
    PartialResourceList->PartialDescriptors[0].Flags = 0;
    PartialResourceList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
        sizeof(CM_INT13_DRIVE_PARAMETER) * DiskCount;

    /* Get harddisk Int13 geometry data */
    Int13Drives = (PVOID)(((ULONG_PTR)PartialResourceList) + sizeof(CM_PARTIAL_RESOURCE_LIST));
    for (i = 0; i < DiskCount; i++)
    {
        if (MachDiskGetDriveGeometry(0x80 + i, &Geometry))
        {
            Int13Drives[i].DriveSelect = 0x80 + i;
            Int13Drives[i].MaxCylinders = Geometry.Cylinders - 1;
            Int13Drives[i].SectorsPerTrack = (USHORT)Geometry.Sectors;
            Int13Drives[i].MaxHeads = (USHORT)Geometry.Heads - 1;
            Int13Drives[i].NumberDrives = DiskCount;

            TRACE(
                "Disk %x: %u Cylinders  %u Heads  %u Sectors  %u Bytes\n",
                0x80 + i,
                Geometry.Cylinders - 1,
                Geometry.Heads - 1,
                Geometry.Sectors,
                Geometry.BytesPerSector);
        }
    }

    FldrCreateComponentKey(BusKey,
                           ControllerClass,
                           DiskController,
                           Output | Input,
                           0,
                           0xFFFFFFFF,
                           NULL,
                           PartialResourceList,
                           Size,
                           &ControllerKey);
    TRACE("Created key: DiskController\\0\n");

    /* Create and fill subkey for each harddisk */
    for (i = 0; i < DiskCount; i++)
    {
        CHAR Identifier[20];

        /* Get disk values */
        PartialResourceList = GetHarddiskConfigurationData(0x80 + i, &Size);
        GetHarddiskIdentifier(Identifier, 0x80 + i);

        /* Create disk key */
        FldrCreateComponentKey(ControllerKey,
                               PeripheralClass,
                               DiskPeripheral,
                               Output | Input,
                               0,
                               0xFFFFFFFF,
                               Identifier,
                               PartialResourceList,
                               Size,
                               &DiskKey);
    }
}

static
VOID
DetectIsaBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG Size;

    /* Set 'Configuration Data' value */
    Size = sizeof(CM_PARTIAL_RESOURCE_LIST) -
           sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        TRACE(
            "Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    memset(PartialResourceList, 0, Size);
    PartialResourceList->Version = 1;
    PartialResourceList->Revision = 1;
    PartialResourceList->Count = 0;

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0x0,
                           0x0,
                           0xFFFFFFFF,
                           "ISA",
                           PartialResourceList,
                           Size,
                           &BusKey);

    /* Increment bus number */
    (*BusNumber)++;

    /* Detect ISA/BIOS devices */
    DetectBiosDisks(SystemKey, BusKey);


    /* FIXME: Detect more ISA devices */
}

PCONFIGURATION_COMPONENT_DATA
XboxHwDetect(VOID)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("DetectHardware()\n");

    /* Create the 'System' key */
    FldrCreateSystemKey(&SystemKey);

    /* TODO: Build actual xbox's hardware configuration tree */
    DetectIsaBios(SystemKey, &BusNumber);

    TRACE("DetectHardware() Done\n");
    return SystemKey;
}

BOOLEAN
XboxInitializeBootDevices(VOID)
{
    // Emulate old behavior
    return XboxHwDetect() != NULL;
}

VOID XboxHwIdle(VOID)
{
    /* UNIMPLEMENTED */
}

/* EOF */
