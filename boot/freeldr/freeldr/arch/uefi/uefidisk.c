/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Disk Access Functions
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define TAG_HW_RESOURCE_LIST    'lRwH'
#define TAG_HW_DISK_CONTEXT     'cDwH'
#define FIRST_BIOS_DISK 0x80
#define FIRST_PARTITION 1

/* Maximum block size we support (8KB) - filters out flash devices */
#define MAX_SUPPORTED_BLOCK_SIZE 8192

/* GPT (GUID Partition Table) definitions */
#define EFI_PARTITION_HEADER_SIGNATURE    "EFI PART"
#define EFI_HEADER_LOCATION               1ULL
#define EFI_TABLE_REVISION                0x00010000
#define EFI_PARTITION_ENTRIES_BLOCK       2ULL
#define EFI_PARTITION_ENTRY_COUNT          128
#define EFI_PARTITION_ENTRY_SIZE          128
#define EFI_PARTITION_NAME_LENGTH         36

/* GPT Partition Type GUIDs */
#define EFI_PART_TYPE_UNUSED_GUID \
    {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}

#define EFI_PART_TYPE_EFI_SYSTEM_PART_GUID \
    {0xc12a7328, 0xf81f, 0x11d2, {0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b}}

#include <pshpack1.h>

/* GPT Table Header */
typedef struct _GPT_TABLE_HEADER
{
    CHAR8   Signature[8];              /* "EFI PART" */
    UINT32  Revision;                   /* 0x00010000 */
    UINT32  HeaderSize;                 /* Size of header (usually 92) */
    UINT32  HeaderCrc32;                /* CRC32 of header */
    UINT32  Reserved;                   /* Must be 0 */
    UINT64  MyLba;                      /* LBA of this header */
    UINT64  AlternateLba;               /* LBA of alternate header */
    UINT64  FirstUsableLba;             /* First usable LBA for partitions */
    UINT64  LastUsableLba;              /* Last usable LBA for partitions */
    EFI_GUID DiskGuid;                  /* Disk GUID */
    UINT64  PartitionEntryLba;          /* LBA of partition entries array */
    UINT32  NumberOfPartitionEntries;   /* Number of partition entries */
    UINT32  SizeOfPartitionEntry;       /* Size of each entry (usually 128) */
    UINT32  PartitionEntryArrayCrc32;   /* CRC32 of partition entries array */
} GPT_TABLE_HEADER, *PGPT_TABLE_HEADER;

/* GPT Partition Entry */
typedef struct _GPT_PARTITION_ENTRY
{
    EFI_GUID PartitionTypeGuid;         /* Partition type GUID */
    EFI_GUID UniquePartitionGuid;       /* Unique partition GUID */
    UINT64  StartingLba;                /* Starting LBA */
    UINT64  EndingLba;                  /* Ending LBA */
    UINT64  Attributes;                 /* Partition attributes */
    CHAR16  PartitionName[EFI_PARTITION_NAME_LENGTH]; /* Partition name (UTF-16) */
} GPT_PARTITION_ENTRY, *PGPT_PARTITION_ENTRY;

#include <poppack.h>

typedef struct tagDISKCONTEXT
{
    UCHAR DriveNumber;
    ULONG SectorSize;
    ULONGLONG SectorOffset;
    ULONGLONG SectorCount;
    ULONGLONG SectorNumber;
} DISKCONTEXT;

typedef struct _INTERNAL_UEFI_DISK
{
    UCHAR ArcDriveNumber;
    UCHAR NumOfPartitions;
    ULONG UefiHandleIndex;
    BOOLEAN IsThisTheBootDrive;
    EFI_HANDLE Handle;
} INTERNAL_UEFI_DISK, *PINTERNAL_UEFI_DISK;

/* GLOBALS *******************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern EFI_HANDLE PublicBootHandle; /* Freeldr itself */

/* Made to match BIOS */
PVOID DiskReadBuffer;
static PVOID DiskReadBufferRaw;
static ULONG DiskReadBufferAlignment;
static BOOLEAN DiskReadBufferFromPool;
UCHAR PcBiosDiskCount;

UCHAR FrldrBootDrive;
ULONG FrldrBootPartition;
SIZE_T DiskReadBufferSize;
PVOID Buffer;

static const CHAR Hex[] = "0123456789abcdef";
static CHAR PcDiskIdentifier[32][20];

/* UEFI-specific */
static ULONG UefiBootRootIndex = 0;
static ULONG PublicBootArcDisk = 0;
static INTERNAL_UEFI_DISK* InternalUefiDisk = NULL;
static EFI_GUID BlockIoGuid = BLOCK_IO_PROTOCOL;
static EFI_HANDLE* handles = NULL;
static ULONG HandleCount = 0;

static
BOOLEAN
UefiIsAlignedPointer(
    IN PVOID Pointer,
    IN ULONG Alignment)
{
    ULONG_PTR Value;

    if (Alignment <= 1)
        return TRUE;

    Value = (ULONG_PTR)Pointer;
    return ((Value & (Alignment - 1)) == 0);
}

static
BOOLEAN
UefiEnsureDiskReadBufferAligned(
    IN ULONG Alignment)
{
    ULONG RequiredAlignment = (Alignment == 0) ? 1 : Alignment;
    EFI_STATUS Status;

    if (DiskReadBuffer != NULL && DiskReadBufferAlignment >= RequiredAlignment &&
        UefiIsAlignedPointer(DiskReadBuffer, RequiredAlignment))
    {
        return TRUE;
    }

    DiskReadBufferAlignment = RequiredAlignment;

    if (DiskReadBufferRaw != NULL && DiskReadBufferFromPool)
    {
        GlobalSystemTable->BootServices->FreePool(DiskReadBufferRaw);
        DiskReadBufferRaw = NULL;
        DiskReadBuffer = NULL;
        DiskReadBufferFromPool = FALSE;
    }

    Status = GlobalSystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                           DiskReadBufferSize + RequiredAlignment,
                                                           (void**)&DiskReadBufferRaw);
    if (EFI_ERROR(Status) || DiskReadBufferRaw == NULL)
    {
        ERR("Failed to allocate aligned disk read buffer (align %lu)\n", RequiredAlignment);
        DiskReadBuffer = NULL;
        return FALSE;
    }
    DiskReadBufferFromPool = TRUE;

    DiskReadBuffer = (PVOID)ALIGN_UP_POINTER_BY(DiskReadBufferRaw, RequiredAlignment);
    if (!UefiIsAlignedPointer(DiskReadBuffer, RequiredAlignment))
    {
        ERR("Aligned disk read buffer is not properly aligned (align %lu)\n", RequiredAlignment);
        return FALSE;
    }

    return TRUE;
}

/* FUNCTIONS *****************************************************************/

PCHAR
GetHarddiskIdentifier(UCHAR DriveNumber)
{
    TRACE("GetHarddiskIdentifier: DriveNumber: %d\n", DriveNumber);
    if (DriveNumber < FIRST_BIOS_DISK)
        return NULL;
    return PcDiskIdentifier[DriveNumber - FIRST_BIOS_DISK];
}

static LONG lReportError = 0; // >= 0: display errors; < 0: hide errors.

LONG
DiskReportError(BOOLEAN bShowError)
{
    /* Set the reference count */
    if (bShowError) ++lReportError;
    else            --lReportError;
    return lReportError;
}


/* GPT Support Functions ******************************************************/

static
BOOLEAN
UefiReadGptHeader(
    IN UCHAR DriveNumber,
    OUT PGPT_TABLE_HEADER GptHeader)
{
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG BlockSize;
    ULONG IoAlign;
    ULONGLONG HeaderLba = EFI_HEADER_LOCATION;

    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
        return FALSE;

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
        return FALSE;

    if (!BlockIo->Media->MediaPresent)
        return FALSE;

    BlockSize = BlockIo->Media->BlockSize;
    IoAlign = BlockIo->Media->IoAlign;

    if (!UefiEnsureDiskReadBufferAligned(IoAlign))
    {
        ERR("Failed to align disk read buffer for drive %d\n", DriveNumber);
        return FALSE;
    }

    /* Read GPT header from LBA 1 */
    Status = BlockIo->ReadBlocks(
        BlockIo,
        BlockIo->Media->MediaId,
        HeaderLba,
        BlockSize,
        DiskReadBuffer);

    if (EFI_ERROR(Status))
        return FALSE;

    RtlCopyMemory(GptHeader, DiskReadBuffer, sizeof(GPT_TABLE_HEADER));

    /* Verify GPT signature */
    if (memcmp(GptHeader->Signature, EFI_PARTITION_HEADER_SIGNATURE, 8) != 0)
        return FALSE;

    /* Verify revision */
    if (GptHeader->Revision != EFI_TABLE_REVISION)
    {
        TRACE("GPT header has unsupported revision: 0x%x\n", GptHeader->Revision);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
UefiGetGptPartitionEntry(
    IN UCHAR DriveNumber,
    IN ULONG PartitionNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    GPT_TABLE_HEADER GptHeader;
    GPT_PARTITION_ENTRY GptEntry;
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG BlockSize;
    ULONGLONG EntryLba;
    ULONG EntryOffset;
    ULONG EntriesPerBlock;
    EFI_GUID UnusedGuid = EFI_PART_TYPE_UNUSED_GUID;

    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
        return FALSE;

    /* Read GPT header */
    if (!UefiReadGptHeader(DriveNumber, &GptHeader))
        return FALSE;

    /* Validate partition number */
    if (PartitionNumber == 0 || PartitionNumber > GptHeader.NumberOfPartitionEntries)
        return FALSE;

    /* Convert to 0-based index */
    ULONG EntryIndex = PartitionNumber - 1;

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
        return FALSE;

    BlockSize = BlockIo->Media->BlockSize;
    EntriesPerBlock = BlockSize / GptHeader.SizeOfPartitionEntry;
    EntryLba = GptHeader.PartitionEntryLba + (EntryIndex / EntriesPerBlock);
    EntryOffset = (EntryIndex % EntriesPerBlock) * GptHeader.SizeOfPartitionEntry;

    /* Read the block containing the partition entry */
    Status = BlockIo->ReadBlocks(
        BlockIo,
        BlockIo->Media->MediaId,
        EntryLba,
        BlockSize,
        DiskReadBuffer);

    if (EFI_ERROR(Status))
        return FALSE;

    /* Extract partition entry */
    RtlCopyMemory(&GptEntry, (PUCHAR)DiskReadBuffer + EntryOffset, sizeof(GPT_PARTITION_ENTRY));

    /* Check if partition is unused */
    if (memcmp(&GptEntry.PartitionTypeGuid, &UnusedGuid, sizeof(EFI_GUID)) == 0)
        return FALSE;

    /* Convert GPT entry to MBR-style PARTITION_TABLE_ENTRY */
    RtlZeroMemory(PartitionTableEntry, sizeof(*PartitionTableEntry));

    /* Calculate sector offset and count */
    /* GPT uses LBA, convert to 512-byte sectors */
    ULONGLONG StartLba = GptEntry.StartingLba;
    ULONGLONG EndLba = GptEntry.EndingLba;
    ULONGLONG SectorCount = (EndLba - StartLba + 1);

    /* For GPT, we need to convert from device block size to 512-byte sectors */
    ULONGLONG StartSector = (StartLba * BlockSize) / 512;
    ULONGLONG SectorCount512 = (SectorCount * BlockSize) / 512;

    PartitionTableEntry->SectorCountBeforePartition = (ULONG)StartSector;
    PartitionTableEntry->PartitionSectorCount = (ULONG)SectorCount512;
    PartitionTableEntry->SystemIndicator = PARTITION_GPT; /* Mark as GPT partition */

    return TRUE;
}

static
BOOLEAN
UefiGetBootPartitionEntry(
    IN UCHAR DriveNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry,
    OUT PULONG BootPartition)
{
    ULONG PartitionNum;
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BootBlockIo;
    EFI_STATUS Status;
    ULONGLONG BootPartitionSize;
    PARTITION_TABLE_ENTRY TempPartitionEntry;

    TRACE("UefiGetBootPartitionEntry: DriveNumber: %d\n", DriveNumber - FIRST_BIOS_DISK);

    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount || handles == NULL || UefiBootRootIndex >= HandleCount)
        return FALSE;

    /* Get the boot handle's Block I/O protocol to determine partition offset/size */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        handles[UefiBootRootIndex],
        &BlockIoGuid,
        (VOID**)&BootBlockIo);

    if (EFI_ERROR(Status) || BootBlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for boot handle\n");
        return FALSE;
    }

    /* For logical partitions, UEFI Block I/O protocol starts at block 0 */
    /* We need to find which partition it corresponds to by comparing sizes */
    BootPartitionSize = BootBlockIo->Media->LastBlock + 1;

    TRACE("Boot partition: Size=%llu blocks, BlockSize=%lu\n",
        BootPartitionSize, BootBlockIo->Media->BlockSize);

    /* If boot handle is the root device itself (not a logical partition) */
    if (!BootBlockIo->Media->LogicalPartition)
    {
        TRACE("Boot handle is root device, using partition 0\n");
        *BootPartition = 0;
        if (PartitionTableEntry != NULL)
        {
            RtlZeroMemory(PartitionTableEntry, sizeof(*PartitionTableEntry));
        }
        return TRUE;
    }

    /* Boot handle is a logical partition - find matching partition entry */
    /* Try to detect GPT first by reading GPT header */
    GPT_TABLE_HEADER GptHeader;
    BOOLEAN IsGpt = UefiReadGptHeader(DriveNumber, &GptHeader);

    if (IsGpt)
    {
        /* For GPT, iterate through GPT partition entries */
        GPT_TABLE_HEADER GptHeader;
        GPT_PARTITION_ENTRY GptEntry;
        ULONG ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
        EFI_BLOCK_IO* RootBlockIo;
        ULONG BlockSize;
        ULONGLONG EntryLba;
        ULONG EntryOffset;
        ULONG EntriesPerBlock;
        EFI_STATUS Status;
        EFI_GUID UnusedGuid = EFI_PART_TYPE_UNUSED_GUID;

        if (!UefiReadGptHeader(DriveNumber, &GptHeader))
        {
            ERR("Failed to read GPT header\n");
            return FALSE;
        }

        Status = GlobalSystemTable->BootServices->HandleProtocol(
            InternalUefiDisk[ArcDriveIndex].Handle,
            &BlockIoGuid,
            (VOID**)&RootBlockIo);

        if (EFI_ERROR(Status) || RootBlockIo == NULL)
            return FALSE;

        BlockSize = RootBlockIo->Media->BlockSize;
        EntriesPerBlock = BlockSize / GptHeader.SizeOfPartitionEntry;

        /* Iterate through GPT partition entries */
        for (ULONG i = 0; i < GptHeader.NumberOfPartitionEntries; i++)
        {
            EntryLba = GptHeader.PartitionEntryLba + (i / EntriesPerBlock);
            EntryOffset = (i % EntriesPerBlock) * GptHeader.SizeOfPartitionEntry;

            /* Read the block containing the partition entry */
            Status = RootBlockIo->ReadBlocks(
                RootBlockIo,
                RootBlockIo->Media->MediaId,
                EntryLba,
                BlockSize,
                DiskReadBuffer);

            if (EFI_ERROR(Status))
                continue;

            /* Extract partition entry */
            RtlCopyMemory(&GptEntry, (PUCHAR)DiskReadBuffer + EntryOffset, sizeof(GPT_PARTITION_ENTRY));

            /* Skip unused partitions */
            if (memcmp(&GptEntry.PartitionTypeGuid, &UnusedGuid, sizeof(EFI_GUID)) == 0)
                continue;

            /* Calculate partition size in blocks */
            ULONGLONG PartitionSizeBlocks = GptEntry.EndingLba - GptEntry.StartingLba + 1;

            TRACE("GPT Partition %lu: StartLba=%llu, EndLba=%llu, SizeBlocks=%llu\n",
                i + 1, GptEntry.StartingLba, GptEntry.EndingLba, PartitionSizeBlocks);

            /* Match partition by size (within 1 block tolerance for rounding) */
            if (PartitionSizeBlocks == BootPartitionSize ||
                (PartitionSizeBlocks > 0 && 
                 (PartitionSizeBlocks - 1 <= BootPartitionSize && 
                  BootPartitionSize <= PartitionSizeBlocks + 1)))
            {
                TRACE("Found matching GPT partition %lu: Size matches (%llu blocks)\n",
                    i + 1, BootPartitionSize);

                *BootPartition = i + 1; /* GPT partitions are 1-indexed */

                /* Convert GPT entry to MBR-style entry for compatibility */
                if (PartitionTableEntry != NULL)
                {
                    RtlZeroMemory(PartitionTableEntry, sizeof(*PartitionTableEntry));
                    ULONGLONG StartSector = (GptEntry.StartingLba * BlockSize) / 512;
                    ULONGLONG SectorCount = (PartitionSizeBlocks * BlockSize) / 512;
                    PartitionTableEntry->SectorCountBeforePartition = (ULONG)StartSector;
                    PartitionTableEntry->PartitionSectorCount = (ULONG)SectorCount;
                    PartitionTableEntry->SystemIndicator = PARTITION_GPT;
                }
                return TRUE;
            }
        }
    }
    else
    {
        /* MBR partition matching */
        PartitionNum = FIRST_PARTITION;
        while (DiskGetPartitionEntry(DriveNumber, PartitionNum, &TempPartitionEntry))
        {
            ULONGLONG PartitionSizeSectors = TempPartitionEntry.PartitionSectorCount;
            ULONGLONG PartitionSizeBlocks;

            /* Convert partition size from MBR sectors (always 512 bytes) to UEFI blocks */
            /* MBR partition table always uses 512-byte sectors per specification */
            /* UEFI Block I/O protocol reports sizes in device's BlockSize bytes */
            /* Compare in bytes to avoid rounding issues, then convert to boot partition's block size */
            ULONGLONG PartitionSizeBytes = PartitionSizeSectors * 512ULL;
            PartitionSizeBlocks = PartitionSizeBytes / BootBlockIo->Media->BlockSize;

            TRACE("Partition %lu: SizeSectors=%llu, SizeBlocks=%llu\n",
                PartitionNum, PartitionSizeSectors, PartitionSizeBlocks);

            /* Match partition by size (within 1 block tolerance for rounding) */
            if (PartitionSizeBlocks == BootPartitionSize ||
                (PartitionSizeBlocks > 0 && 
                 (PartitionSizeBlocks - 1 <= BootPartitionSize && 
                  BootPartitionSize <= PartitionSizeBlocks + 1)))
            {
                TRACE("Found matching partition %lu: Size matches (%llu blocks)\n",
                    PartitionNum, BootPartitionSize);

                *BootPartition = PartitionNum;
                if (PartitionTableEntry != NULL)
                {
                    RtlCopyMemory(PartitionTableEntry, &TempPartitionEntry, sizeof(*PartitionTableEntry));
                }
                return TRUE;
            }

            PartitionNum++;
        }
    }

    /* If we couldn't find a match, check if it's a CD-ROM (special case) */
    if (BootBlockIo->Media->RemovableMedia && BootBlockIo->Media->BlockSize == 2048)
    {
        TRACE("Boot device is CD-ROM, using partition 0xFF\n");
        *BootPartition = 0xFF;
        if (PartitionTableEntry != NULL)
        {
            RtlZeroMemory(PartitionTableEntry, sizeof(*PartitionTableEntry));
        }
        return TRUE;
    }

    /* Fallback: if we can't determine, use partition 1 */
    ERR("Could not determine boot partition, using partition 1 as fallback\n");
    PartitionNum = FIRST_PARTITION;
    if (DiskGetPartitionEntry(DriveNumber, PartitionNum, &TempPartitionEntry))
    {
        *BootPartition = PartitionNum;
        if (PartitionTableEntry != NULL)
        {
            RtlCopyMemory(PartitionTableEntry, &TempPartitionEntry, sizeof(*PartitionTableEntry));
        }
        return TRUE;
    }

    return FALSE;
}

static
ARC_STATUS
UefiDiskClose(ULONG FileId)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    FrLdrTempFree(Context, TAG_HW_DISK_CONTEXT);
    return ESUCCESS;
}

static
ARC_STATUS
UefiDiskGetFileInformation(ULONG FileId, FILEINFORMATION *Information)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    RtlZeroMemory(Information, sizeof(*Information));

    /*
     * The ARC specification mentions that for partitions, StartingAddress and
     * EndingAddress are the start and end positions of the partition in terms
     * of byte offsets from the start of the disk.
     * CurrentAddress is the current offset into (i.e. relative to) the partition.
     */
    Information->StartingAddress.QuadPart = Context->SectorOffset * Context->SectorSize;
    Information->EndingAddress.QuadPart   = (Context->SectorOffset + Context->SectorCount) * Context->SectorSize;
    Information->CurrentAddress.QuadPart  = Context->SectorNumber * Context->SectorSize;

    Information->Type = DiskPeripheral; /* No floppy for you for now... */

    return ESUCCESS;
}

static
ARC_STATUS
UefiDiskOpen(CHAR *Path, OPENMODE OpenMode, ULONG *FileId)
{
    DISKCONTEXT* Context;
    UCHAR DriveNumber;
    ULONG DrivePartition, SectorSize;
    ULONGLONG SectorOffset = 0;
    ULONGLONG SectorCount = 0;
    ULONG ArcDriveIndex;
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;

    TRACE("UefiDiskOpen: File ID: %p, Path: %s\n", FileId, Path);

    if (DiskReadBufferSize == 0)
    {
        ERR("DiskOpen(): DiskReadBufferSize is 0, something is wrong.\n");
        ASSERT(FALSE);
        return ENOMEM;
    }

    if (!DissectArcPath(Path, NULL, &DriveNumber, &DrivePartition))
        return EINVAL;

    TRACE("Opening disk: DriveNumber: %d, DrivePartition: %d\n", DriveNumber, DrivePartition);

    if (DriveNumber < FIRST_BIOS_DISK)
        return EINVAL;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
        return EINVAL;

    /* Get Block I/O protocol for this drive */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return EINVAL;
    }

    /* Check media is present */
    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return EINVAL;
    }

    SectorSize = BlockIo->Media->BlockSize;

    if (DrivePartition != 0xff && DrivePartition != 0)
    {
        if (!DiskGetPartitionEntry(DriveNumber, DrivePartition, &PartitionTableEntry))
            return EINVAL;

        SectorOffset = PartitionTableEntry.SectorCountBeforePartition;
        SectorCount = PartitionTableEntry.PartitionSectorCount;
    }
    else
    {
        GEOMETRY Geometry;
        if (!MachDiskGetDriveGeometry(DriveNumber, &Geometry))
            return EINVAL;

        if (SectorSize != Geometry.BytesPerSector)
        {
            ERR("SectorSize (%lu) != Geometry.BytesPerSector (%lu), expect problems!\n",
                SectorSize, Geometry.BytesPerSector);
        }

        SectorOffset = 0;
        SectorCount = Geometry.Sectors;
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
UefiDiskRead(ULONG FileId, VOID *Buffer, ULONG N, ULONG *Count)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    UCHAR* Ptr = (UCHAR*)Buffer;
    ULONG Length, TotalSectors, MaxSectors, ReadSectors;
    ULONGLONG SectorOffset;
    BOOLEAN ret;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG ArcDriveIndex;

    ASSERT(DiskReadBufferSize > 0);


    TotalSectors = (N + Context->SectorSize - 1) / Context->SectorSize;
    MaxSectors   = DiskReadBufferSize / Context->SectorSize;
    SectorOffset = Context->SectorOffset + Context->SectorNumber;

    // If MaxSectors is 0, this will lead to infinite loop.
    // In release builds assertions are disabled, however we also have sanity checks in DiskOpen()
    ASSERT(MaxSectors > 0);

    if (MaxSectors == 0)
    {
        ERR("MaxSectors is 0, cannot read\n");
        *Count = 0;
        return EIO;
    }

    ArcDriveIndex = Context->DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
    {
        ERR("Invalid drive number %d\n", Context->DriveNumber);
        *Count = 0;
        return EINVAL;
    }

    /* Get Block I/O protocol */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol\n");
        *Count = 0;
        return EIO;
    }

    if (!UefiEnsureDiskReadBufferAligned(BlockIo->Media->IoAlign))
    {
        ERR("Failed to align disk read buffer\n");
        *Count = 0;
        return EIO;
    }

    ret = TRUE;

    while (TotalSectors)
    {
        ReadSectors = min(TotalSectors, MaxSectors);

        Status = BlockIo->ReadBlocks(
            BlockIo,
            BlockIo->Media->MediaId,
            SectorOffset,
            ReadSectors * Context->SectorSize,
            DiskReadBuffer);

        if (EFI_ERROR(Status))
        {
            ERR("ReadBlocks failed: Status = 0x%lx\n", (ULONG)Status);
            ret = FALSE;
            break;
        }

        Length = ReadSectors * Context->SectorSize;
        Length = min(Length, N);

        RtlCopyMemory(Ptr, DiskReadBuffer, Length);

        Ptr += Length;
        N -= Length;
        SectorOffset += ReadSectors;
        TotalSectors -= ReadSectors;
    }

    *Count = (ULONG)((ULONG_PTR)Ptr - (ULONG_PTR)Buffer);
    Context->SectorNumber = SectorOffset - Context->SectorOffset;

    return (ret ? ESUCCESS : EIO);
}

static
ARC_STATUS
UefiDiskSeek(ULONG FileId, LARGE_INTEGER *Position, SEEKMODE SeekMode)
{
    DISKCONTEXT* Context = FsGetDeviceSpecific(FileId);
    LARGE_INTEGER NewPosition = *Position;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += (Context->SectorNumber * Context->SectorSize);
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart & (Context->SectorSize - 1))
        return EINVAL;

    /* Convert in number of sectors */
    NewPosition.QuadPart /= Context->SectorSize;

    /* HACK: CDROMs may have a SectorCount of 0 */
    if (Context->SectorCount != 0 && NewPosition.QuadPart >= Context->SectorCount)
        return EINVAL;

    Context->SectorNumber = NewPosition.QuadPart;
    return ESUCCESS;
}

static const DEVVTBL UefiDiskVtbl =
{
    UefiDiskClose,
    UefiDiskGetFileInformation,
    UefiDiskOpen,
    UefiDiskRead,
    UefiDiskSeek,
};

static
VOID
GetHarddiskInformation(UCHAR DriveNumber)
{
    PMASTER_BOOT_RECORD Mbr;
    PULONG Buffer;
    ULONG i;
    ULONG Checksum;
    ULONG Signature;
    BOOLEAN ValidPartitionTable;
    CHAR ArcName[MAX_PATH];
    PARTITION_TABLE_ENTRY PartitionTableEntry;
    ULONG ArcDriveIndex;
    PCHAR Identifier;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;
    if (ArcDriveIndex >= 32)
        return;

    Identifier = PcDiskIdentifier[ArcDriveIndex];

    /* Detect disk partition type */
    DiskDetectPartitionType(DriveNumber);

    /* Read the MBR */
    if (!MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, DiskReadBuffer))
    {
        ERR("Reading MBR failed\n");
        /* We failed, use a default identifier */
        sprintf(Identifier, "BIOSDISK%d", ArcDriveIndex);
        return;
    }

    Buffer = (ULONG*)DiskReadBuffer;
    Mbr = (PMASTER_BOOT_RECORD)DiskReadBuffer;

    Signature = Mbr->Signature;
    TRACE("Signature: %x\n", Signature);

    /* Calculate the MBR checksum */
    Checksum = 0;
    for (i = 0; i < 512 / sizeof(ULONG); i++)
    {
        Checksum += Buffer[i];
    }
    Checksum = ~Checksum + 1;
    TRACE("Checksum: %x\n", Checksum);

    ValidPartitionTable = (Mbr->MasterBootRecordMagic == 0xAA55);

    /* Fill out the ARC disk block */
    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)", ArcDriveIndex);
    AddReactOSArcDiskInfo(ArcName, Signature, Checksum, ValidPartitionTable);

    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(0)", ArcDriveIndex);
    FsRegisterDevice(ArcName, &UefiDiskVtbl);

    /* Add partitions */
    i = FIRST_PARTITION;
    DiskReportError(FALSE);
    while (DiskGetPartitionEntry(DriveNumber, i, &PartitionTableEntry))
    {
        if (PartitionTableEntry.SystemIndicator != PARTITION_ENTRY_UNUSED)
        {
            sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(%lu)", ArcDriveIndex, i);
            FsRegisterDevice(ArcName, &UefiDiskVtbl);
        }
        i++;
    }
    DiskReportError(TRUE);

    if (ArcDriveIndex < PcBiosDiskCount && InternalUefiDisk != NULL)
    {
        InternalUefiDisk[ArcDriveIndex].NumOfPartitions = i;
    }

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
    Identifier[18] = (ValidPartitionTable ? 'A' : 'X');
    Identifier[19] = 0;
    TRACE("Identifier: %s\n", Identifier);
}

static
VOID
UefiSetupBlockDevices(VOID)
{
    ULONG BlockDeviceIndex;
    ULONG SystemHandleCount;
    EFI_STATUS Status;
    ULONG i;
    UINTN HandleSize = 0;
    EFI_BLOCK_IO* BlockIo;

    PcBiosDiskCount = 0;
    UefiBootRootIndex = 0;

    /* Step 1: Get the size needed for handles buffer - no matter how it fails we're good */
    Status = GlobalSystemTable->BootServices->LocateHandle(
        ByProtocol,
        &BlockIoGuid,
        NULL,
        &HandleSize,
        NULL);

    if (HandleSize == 0)
    {
        ERR("Failed to get handle buffer size: Status = 0x%lx\n", (ULONG)Status);
        return;
    }

    SystemHandleCount = HandleSize / sizeof(EFI_HANDLE);
    if (SystemHandleCount == 0)
    {
        ERR("No block devices found\n");
        return;
    }

    /* Step 2: Allocate buffer for handles */
    handles = MmAllocateMemoryWithType(HandleSize, LoaderFirmwareTemporary);
    if (handles == NULL)
    {
        ERR("Failed to allocate memory for handles\n");
        return;
    }

    /* Step 3: Get actual handles */
    Status = GlobalSystemTable->BootServices->LocateHandle(
        ByProtocol,
        &BlockIoGuid,
        NULL,
        &HandleSize,
        handles);

    if (EFI_ERROR(Status))
    {
        ERR("Failed to locate block device handles: Status = 0x%lx\n", (ULONG)Status);
        return;
    }

    HandleCount = SystemHandleCount;

    /* Step 4: Allocate internal disk structure */
    InternalUefiDisk = MmAllocateMemoryWithType(
        sizeof(INTERNAL_UEFI_DISK) * SystemHandleCount,
        LoaderFirmwareTemporary);

    if (InternalUefiDisk == NULL)
    {
        ERR("Failed to allocate memory for internal disk structure\n");
        return;
    }

    RtlZeroMemory(InternalUefiDisk, sizeof(INTERNAL_UEFI_DISK) * SystemHandleCount);

    /* Step 5: Find boot handle and determine if it's a root device or partition */
    UefiBootRootIndex = 0;
    for (i = 0; i < SystemHandleCount; i++)
    {
        if (handles[i] == PublicBootHandle)
        {
            UefiBootRootIndex = i;
            TRACE("Found boot handle at index %lu\n", i);

            /* Check if boot handle is a root device or partition */
            Status = GlobalSystemTable->BootServices->HandleProtocol(
                handles[i],
                &BlockIoGuid,
                (VOID**)&BlockIo);

            if (!EFI_ERROR(Status) && BlockIo != NULL)
            {
                TRACE("Boot handle: LogicalPartition=%s, RemovableMedia=%s, BlockSize=%lu\n",
                    BlockIo->Media->LogicalPartition ? "TRUE" : "FALSE",
                    BlockIo->Media->RemovableMedia ? "TRUE" : "FALSE",
                    BlockIo->Media->BlockSize);
            }
            break;
        }
    }

    /* Step 6: Enumerate root block devices (skip logical partitions) */
    BlockDeviceIndex = 0;
    for (i = 0; i < SystemHandleCount; i++)
    {
        Status = GlobalSystemTable->BootServices->HandleProtocol(
            handles[i],
            &BlockIoGuid,
            (VOID**)&BlockIo);

        if (EFI_ERROR(Status))
        {
            TRACE("HandleProtocol failed for handle %lu: Status = 0x%lx\n", i, (ULONG)Status);
            continue;
        }

        if (BlockIo == NULL)
        {
            TRACE("BlockIo is NULL for handle %lu\n", i);
            continue;
        }

        if (!BlockIo->Media->MediaPresent)
        {
            TRACE("Media not present for handle %lu\n", i);
            continue;
        }

        if (BlockIo->Media->BlockSize == 0)
        {
            TRACE("Invalid block size (0) for handle %lu\n", i);
            continue;
        }

        /* Filter out devices with unusually large block sizes (flash devices) */
        if (BlockIo->Media->BlockSize > MAX_SUPPORTED_BLOCK_SIZE)
        {
            TRACE("Block size too large (%lu) for handle %lu, skipping\n",
                BlockIo->Media->BlockSize, i);
            continue;
        }

        /* Logical partitions are handled separately by partition scanning */
        if (BlockIo->Media->LogicalPartition)
        {
            /* If boot handle is a logical partition, we need to find its parent root device */
            /* For now, we'll handle this after enumeration by matching handles */
            TRACE("Skipping logical partition handle %lu\n", i);
            continue;
        }

        /* This is a root block device */
        TRACE("Found root block device at index %lu: BlockSize=%lu, LastBlock=%llu\n",
            i, BlockIo->Media->BlockSize, BlockIo->Media->LastBlock);

        InternalUefiDisk[BlockDeviceIndex].ArcDriveNumber = BlockDeviceIndex;
        InternalUefiDisk[BlockDeviceIndex].UefiHandleIndex = i;
        InternalUefiDisk[BlockDeviceIndex].Handle = handles[i];
        InternalUefiDisk[BlockDeviceIndex].IsThisTheBootDrive = FALSE;

        /* Check if this root device contains the boot partition */
        /* If boot handle is a logical partition, we need to find which root device it belongs to */
        /* For now, if boot handle index matches, mark it as boot device */
        if (i == UefiBootRootIndex)
        {
            InternalUefiDisk[BlockDeviceIndex].IsThisTheBootDrive = TRUE;
            PublicBootArcDisk = BlockDeviceIndex;
            TRACE("Boot device is at ARC drive index %lu (root device)\n", BlockDeviceIndex);
        }

        /* Increment PcBiosDiskCount BEFORE calling GetHarddiskInformation
         * so that UefiDiskReadLogicalSectors can validate the drive number */
        PcBiosDiskCount = BlockDeviceIndex + 1;

        TRACE("Calling GetHarddiskInformation for drive %d (BlockDeviceIndex=%lu)\n",
            BlockDeviceIndex + FIRST_BIOS_DISK, BlockDeviceIndex);
        if (!UefiEnsureDiskReadBufferAligned(BlockIo->Media->IoAlign))
        {
            ERR("Failed to align disk read buffer for drive %d\n", BlockDeviceIndex + FIRST_BIOS_DISK);
            return;
        }

        GetHarddiskInformation(BlockDeviceIndex + FIRST_BIOS_DISK);
        BlockDeviceIndex++;
    }

    /* Step 7: If boot handle was a logical partition, find its parent root device */
    if (UefiBootRootIndex < HandleCount)
    {
        Status = GlobalSystemTable->BootServices->HandleProtocol(
            handles[UefiBootRootIndex],
            &BlockIoGuid,
            (VOID**)&BlockIo);

        if (!EFI_ERROR(Status) && BlockIo != NULL && BlockIo->Media->LogicalPartition)
        {
            TRACE("Boot handle is a logical partition, searching for parent root device\n");
            TRACE("Boot partition: BlockSize=%lu, RemovableMedia=%s\n",
                BlockIo->Media->BlockSize,
                BlockIo->Media->RemovableMedia ? "TRUE" : "FALSE");

            /* Find the root device that matches the boot partition's characteristics */
            /* For CD-ROMs: match BlockSize=2048 and RemovableMedia=TRUE */
            /* For hard disks: match BlockSize and find the root device before this partition */
            BOOLEAN FoundBootDevice = FALSE;
            for (i = 0; i < BlockDeviceIndex; i++)
            {
                EFI_BLOCK_IO* RootBlockIo;
                Status = GlobalSystemTable->BootServices->HandleProtocol(
                    InternalUefiDisk[i].Handle,
                    &BlockIoGuid,
                    (VOID**)&RootBlockIo);

                if (EFI_ERROR(Status) || RootBlockIo == NULL)
                    continue;

                /* For CD-ROM: match BlockSize=2048 and RemovableMedia */
                if (BlockIo->Media->BlockSize == 2048 && BlockIo->Media->RemovableMedia)
                {
                    if (RootBlockIo->Media->BlockSize == 2048 && 
                        RootBlockIo->Media->RemovableMedia &&
                        !RootBlockIo->Media->LogicalPartition)
                    {
                        PublicBootArcDisk = i;
                        InternalUefiDisk[i].IsThisTheBootDrive = TRUE;
                        FoundBootDevice = TRUE;
                        TRACE("Found CD-ROM boot device at ARC drive index %lu\n", i);
                        break;
                    }
                }
                /* For hard disk partitions: the root device should be before the partition handle */
                else if (InternalUefiDisk[i].UefiHandleIndex < UefiBootRootIndex)
                {
                    /* Check if this root device is likely the parent */
                    if (RootBlockIo->Media->BlockSize == BlockIo->Media->BlockSize &&
                        !RootBlockIo->Media->LogicalPartition)
                    {
                        /* This might be the parent, but we need to be more certain */
                        /* For now, use the last root device before the boot handle */
                        PublicBootArcDisk = i;
                        InternalUefiDisk[i].IsThisTheBootDrive = TRUE;
                        FoundBootDevice = TRUE;
                        TRACE("Found potential hard disk boot device at ARC drive index %lu\n", i);
                    }
                }
            }

            if (!FoundBootDevice && PcBiosDiskCount > 0)
            {
                PublicBootArcDisk = 0;
                InternalUefiDisk[0].IsThisTheBootDrive = TRUE;
                TRACE("Could not determine boot device, assuming first drive\n");
            }
        }
    }

    TRACE("Found %lu root block devices\n", PcBiosDiskCount);
}

static
BOOLEAN
UefiSetBootpath(VOID)
{
    EFI_BLOCK_IO* BootBlockIo = NULL;
    EFI_BLOCK_IO* RootBlockIo = NULL;
    EFI_STATUS Status;
    ULONG ArcDriveIndex;

    TRACE("UefiSetBootpath: Setting up boot path\n");

    if (UefiBootRootIndex >= HandleCount || handles == NULL)
    {
        ERR("Invalid boot root index\n");
        return FALSE;
    }

    ArcDriveIndex = PublicBootArcDisk;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
    {
        ERR("Invalid boot arc disk index\n");
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        handles[UefiBootRootIndex],
        &BlockIoGuid,
        (VOID**)&BootBlockIo);

    if (EFI_ERROR(Status) || BootBlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for boot handle\n");
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&RootBlockIo);

    if (EFI_ERROR(Status) || RootBlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for boot root device\n");
        return FALSE;
    }

    FrldrBootDrive = (FIRST_BIOS_DISK + ArcDriveIndex);

    /* Check if booting from CD-ROM by checking the boot handle properties */
    /* CD-ROMs have BlockSize=2048 and RemovableMedia=TRUE */
    if (BootBlockIo->Media->RemovableMedia == TRUE && BootBlockIo->Media->BlockSize == 2048)
    {
        /* Boot Partition 0xFF is the magic value that indicates booting from CD-ROM */
        FrldrBootPartition = 0xFF;
        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)cdrom(%u)", ArcDriveIndex);
        TRACE("Boot path set to CD-ROM: %s\n", FrLdrBootPath);
    }
    else
    {
        ULONG BootPartition;
        PARTITION_TABLE_ENTRY PartitionEntry;

        /* This is a hard disk */
        /* If boot handle is a logical partition, we need to determine which partition number */
        if (BootBlockIo->Media->LogicalPartition)
        {
            /* For logical partitions, we need to find the partition number */
            /* This is tricky - we'll use partition 1 as default for now */
            /* TODO: Properly determine partition number from boot handle */
            BootPartition = FIRST_PARTITION;
            TRACE("Boot handle is logical partition, using partition %lu\n", BootPartition);
        }
        else
        {
            /* Boot handle is the root device itself */
            if (!UefiGetBootPartitionEntry(FrldrBootDrive, &PartitionEntry, &BootPartition))
            {
                ERR("Failed to get boot partition entry\n");
                return FALSE;
            }
        }

        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)rdisk(%u)partition(%lu)",
                           ArcDriveIndex, BootPartition);
        TRACE("Boot path set to hard disk: %s\n", FrLdrBootPath);
    }

    return TRUE;
}

BOOLEAN
UefiInitializeBootDevices(VOID)
{
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG ArcDriveIndex;
    PMASTER_BOOT_RECORD Mbr;
    PULONG Buffer;
    ULONG Checksum = 0;
    ULONG Signature;
    ULONG i;

    DiskReadBufferSize = EFI_PAGE_SIZE;
    DiskReadBuffer = NULL;
    DiskReadBufferRaw = NULL;
    DiskReadBufferAlignment = 1;
    DiskReadBufferFromPool = FALSE;
    if (!UefiEnsureDiskReadBufferAligned(1))
    {
        ERR("Failed to allocate disk read buffer\n");
        return FALSE;
    }

    UefiSetupBlockDevices();

    if (PcBiosDiskCount == 0)
    {
        ERR("No block devices found\n");
        return FALSE;
    }

    if (!UefiSetBootpath())
    {
        ERR("Failed to set boot path\n");
        return FALSE;
    }

    /* Handle CD-ROM boot device registration */
    ArcDriveIndex = PublicBootArcDisk;
    if (ArcDriveIndex >= PcBiosDiskCount || InternalUefiDisk == NULL)
    {
        ERR("Invalid boot arc disk index\n");
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol\n");
        return FALSE;
    }

    if (BlockIo->Media->RemovableMedia == TRUE && BlockIo->Media->BlockSize == 2048)
    {
        /* Read the MBR from CD-ROM (sector 16) */
        if (!MachDiskReadLogicalSectors(FrldrBootDrive, 16ULL, 1, DiskReadBuffer))
        {
            ERR("Reading MBR from CD-ROM failed\n");
            return FALSE;
        }

        Buffer = (ULONG*)DiskReadBuffer;
        Mbr = (PMASTER_BOOT_RECORD)DiskReadBuffer;

        Signature = Mbr->Signature;
        TRACE("CD-ROM Signature: %x\n", Signature);

        /* Calculate the MBR checksum */
        for (i = 0; i < 2048 / sizeof(ULONG); i++)
        {
            Checksum += Buffer[i];
        }
        Checksum = ~Checksum + 1;
        TRACE("CD-ROM Checksum: %x\n", Checksum);

        /* Fill out the ARC disk block */
        AddReactOSArcDiskInfo(FrLdrBootPath, Signature, Checksum, TRUE);

        FsRegisterDevice(FrLdrBootPath, &UefiDiskVtbl);
        TRACE("Registered CD-ROM boot device: 0x%02X\n", (int)FrldrBootDrive);
    }

    return TRUE;
}

UCHAR
UefiGetFloppyCount(VOID)
{
    /* No floppy support in UEFI */
    return 0;
}

BOOLEAN
UefiDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;
    ULONG BlockSize;
    ULONG IoAlign;

    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;

    if (InternalUefiDisk == NULL)
    {
        ERR("InternalUefiDisk not initialized\n");
        return FALSE;
    }

    if (ArcDriveIndex >= 32)
    {
        ERR("Drive index out of bounds: %d (ArcDriveIndex=%lu)\n", DriveNumber, ArcDriveIndex);
        return FALSE;
    }

    /* Allow access during initialization: check if handle is set up */
    /* During initialization, Handle is set before GetHarddiskInformation is called */
    if (InternalUefiDisk[ArcDriveIndex].Handle == NULL)
    {
        ERR("Invalid drive number: %d (ArcDriveIndex=%lu, PcBiosDiskCount=%lu, Handle=NULL)\n", 
            DriveNumber, ArcDriveIndex, PcBiosDiskCount);
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return FALSE;
    }

    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return FALSE;
    }

    BlockSize = BlockIo->Media->BlockSize;
    IoAlign = BlockIo->Media->IoAlign;

    if (!UefiEnsureDiskReadBufferAligned(IoAlign))
    {
        ERR("Failed to align disk read buffer for drive %d\n", DriveNumber);
        return FALSE;
    }

    if (!UefiIsAlignedPointer(Buffer, (IoAlign == 0) ? 1 : IoAlign))
    {
        ULONG TotalSectors = SectorCount;
        ULONG MaxSectors = DiskReadBufferSize / BlockSize;
        ULONGLONG CurrentSector = SectorNumber;
        PUCHAR OutPtr = (PUCHAR)Buffer;

        if (MaxSectors == 0)
        {
            ERR("DiskReadBufferSize too small for block size %lu\n", BlockSize);
            return FALSE;
        }

        while (TotalSectors)
        {
            ULONG ReadSectors = min(TotalSectors, MaxSectors);
            UINTN ReadSize = ReadSectors * BlockSize;

            Status = BlockIo->ReadBlocks(
                BlockIo,
                BlockIo->Media->MediaId,
                CurrentSector,
                ReadSize,
                DiskReadBuffer);

            if (EFI_ERROR(Status))
            {
                ERR("ReadBlocks failed: DriveNumber=%d, SectorNumber=%llu, SectorCount=%lu, Status=0x%lx\n",
                    DriveNumber, CurrentSector, ReadSectors, (ULONG)Status);
                ERR("ReadBlocks details: BlockSize=%lu, IoAlign=%lu, Buffer=%p, DiskReadBuffer=%p, MediaId=0x%lx\n",
                    BlockSize, IoAlign, Buffer, DiskReadBuffer, (ULONG)BlockIo->Media->MediaId);
                ERR("ReadBlocks media: LastBlock=%llu, LogicalPartition=%s, RemovableMedia=%s\n",
                    BlockIo->Media->LastBlock,
                    BlockIo->Media->LogicalPartition ? "TRUE" : "FALSE",
                    BlockIo->Media->RemovableMedia ? "TRUE" : "FALSE");
                return FALSE;
            }

            RtlCopyMemory(OutPtr, DiskReadBuffer, ReadSize);
            OutPtr += ReadSize;
            CurrentSector += ReadSectors;
            TotalSectors -= ReadSectors;
        }

        return TRUE;
    }

    Status = BlockIo->ReadBlocks(
        BlockIo,
        BlockIo->Media->MediaId,
        SectorNumber,
        SectorCount * BlockSize,
        Buffer);

    if (EFI_ERROR(Status))
    {
        ERR("ReadBlocks failed: DriveNumber=%d, SectorNumber=%llu, SectorCount=%lu, Status=0x%lx\n",
            DriveNumber, SectorNumber, SectorCount, (ULONG)Status);
        ERR("ReadBlocks details: BlockSize=%lu, IoAlign=%lu, Buffer=%p, DiskReadBuffer=%p, MediaId=0x%lx\n",
            BlockSize, IoAlign, Buffer, DiskReadBuffer, (ULONG)BlockIo->Media->MediaId);
        ERR("ReadBlocks media: LastBlock=%llu, LogicalPartition=%s, RemovableMedia=%s\n",
            BlockIo->Media->LastBlock,
            BlockIo->Media->LogicalPartition ? "TRUE" : "FALSE",
            BlockIo->Media->RemovableMedia ? "TRUE" : "FALSE");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
UefiDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;

    if (DriveNumber < FIRST_BIOS_DISK)
        return FALSE;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;

    if (InternalUefiDisk == NULL)
    {
        ERR("InternalUefiDisk not initialized\n");
        return FALSE;
    }

    if (ArcDriveIndex >= 32 || InternalUefiDisk[ArcDriveIndex].Handle == NULL)
    {
        ERR("Invalid drive number: %d\n", DriveNumber);
        return FALSE;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return FALSE;
    }

    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return FALSE;
    }

    Geometry->Cylinders = 1; /* Not relevant for UEFI Block I/O protocol */
    Geometry->Heads = 1;     /* Not relevant for UEFI Block I/O protocol */
    Geometry->SectorsPerTrack = (ULONG)(BlockIo->Media->LastBlock + 1);
    Geometry->BytesPerSector = BlockIo->Media->BlockSize;
    Geometry->Sectors = BlockIo->Media->LastBlock + 1;

    return TRUE;
}

ULONG
UefiDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    ULONG ArcDriveIndex;
    EFI_BLOCK_IO* BlockIo;
    EFI_STATUS Status;

    if (DriveNumber < FIRST_BIOS_DISK)
        return 0;

    ArcDriveIndex = DriveNumber - FIRST_BIOS_DISK;

    if (InternalUefiDisk == NULL)
    {
        ERR("InternalUefiDisk not initialized\n");
        return 0;
    }

    if (ArcDriveIndex >= 32 || InternalUefiDisk[ArcDriveIndex].Handle == NULL)
    {
        ERR("Invalid drive number: %d\n", DriveNumber);
        return 0;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        InternalUefiDisk[ArcDriveIndex].Handle,
        &BlockIoGuid,
        (VOID**)&BlockIo);

    if (EFI_ERROR(Status) || BlockIo == NULL)
    {
        ERR("Failed to get Block I/O protocol for drive %d\n", DriveNumber);
        return 0;
    }

    if (!BlockIo->Media->MediaPresent)
    {
        ERR("Media not present for drive %d\n", DriveNumber);
        return 0;
    }

    return (ULONG)(BlockIo->Media->LastBlock + 1);
}
