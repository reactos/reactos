/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Disk Access Functions
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>
// AGENT-MODIFIED: Include header for UefiEnumerateArcDisks
#include <uefi/uefiarcname.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define TAG_HW_RESOURCE_LIST    'lRwH'
#define TAG_HW_DISK_CONTEXT     'cDwH'
#define FIRST_BIOS_DISK 0x80
#define FIRST_PARTITION 1

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
    UCHAR UefiRootNumber;
    BOOLEAN IsThisTheBootDrive;
} INTERNAL_UEFI_DISK, *PINTERNAL_UEFI_DISK;

/* GLOBALS *******************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern EFI_HANDLE PublicBootHandle; /* Freeldr itself */

/* Made to match BIOS */
PVOID DiskReadBuffer;
UCHAR PcBiosDiskCount;

UCHAR FrldrBootDrive;
ULONG FrldrBootPartition;
SIZE_T DiskReadBufferSize;
PVOID Buffer;

static const CHAR Hex[] = "0123456789abcdef";
static CHAR PcDiskIdentifier[32][20];

/* UEFI-specific */
static ULONG UefiBootRootIdentifier;
static ULONG OffsetToBoot;
static ULONG PublicBootArcDisk;
static INTERNAL_UEFI_DISK* InternalUefiDisk = NULL;
static EFI_GUID bioGuid = BLOCK_IO_PROTOCOL;
static EFI_BLOCK_IO* bio;
static EFI_HANDLE* handles = NULL;

/* FUNCTIONS *****************************************************************/

PCHAR
GetHarddiskIdentifier(UCHAR DriveNumber)
{
    TRACE("GetHarddiskIdentifier: DriveNumber: %d\n", DriveNumber);
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

static
BOOLEAN
UefiGetBootPartitionEntry(
    IN UCHAR DriveNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry,
    OUT PULONG BootPartition)
{
    ULONG PartitionNum;

    TRACE("UefiGetBootPartitionEntry: DriveNumber: %d\n", DriveNumber - FIRST_BIOS_DISK);
    /* UefiBootRoot is the offset into the array of handles where the raw disk of the boot drive is.
     * Partitions start with 1 in ARC, but UEFI root drive identitfier is also first partition. */
    PartitionNum = (OffsetToBoot - UefiBootRootIdentifier);
    if (PartitionNum == 0)
    {
        TRACE("Boot PartitionNumber is 0\n");
        /* The OffsetToBoot is equal to the RootIdentifier */
        PartitionNum = FIRST_PARTITION;
    }

    *BootPartition = PartitionNum;
    TRACE("UefiGetBootPartitionEntry: Boot Partition is: %d\n", PartitionNum);
    return TRUE;
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
    ULONG UefiDriveNumber = 0;
    PARTITION_TABLE_ENTRY PartitionTableEntry;

    TRACE("UefiDiskOpen: File ID: %d, Path: %s\n", FileId, Path);

    if (DiskReadBufferSize == 0)
    {
        ERR("DiskOpen(): DiskReadBufferSize is 0, something is wrong.\n");
        ASSERT(FALSE);
        return ENOMEM;
    }

    if (!DissectArcPath(Path, NULL, &DriveNumber, &DrivePartition))
        return EINVAL;

    TRACE("Opening disk: DriveNumber: %d, DrivePartition: %d\n", DriveNumber, DrivePartition);
    UefiDriveNumber = DriveNumber - FIRST_BIOS_DISK;
    GlobalSystemTable->BootServices->HandleProtocol(handles[UefiDriveNumber], &bioGuid, (void**)&bio);
    SectorSize = bio->Media->BlockSize;

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

    ASSERT(DiskReadBufferSize > 0);

    TotalSectors = (N + Context->SectorSize - 1) / Context->SectorSize;
    MaxSectors   = DiskReadBufferSize / Context->SectorSize;
    SectorOffset = Context->SectorOffset + Context->SectorNumber;

    // If MaxSectors is 0, this will lead to infinite loop.
    // In release builds assertions are disabled, however we also have sanity checks in DiskOpen()
    ASSERT(MaxSectors > 0);

    ret = TRUE;

    while (TotalSectors)
    {
        ReadSectors = min(TotalSectors, MaxSectors);

        ret = MachDiskReadLogicalSectors(Context->DriveNumber,
                                         SectorOffset,
                                         ReadSectors,
                                         DiskReadBuffer);
        if (!ret)
            break;

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
    PCHAR Identifier = PcDiskIdentifier[DriveNumber - FIRST_BIOS_DISK];

    /* Detect disk partition type */
    DiskDetectPartitionType(DriveNumber);

    /* Read the MBR */
    if (!MachDiskReadLogicalSectors(DriveNumber, 0ULL, 1, DiskReadBuffer))
    {
        ERR("Reading MBR failed\n");
        /* We failed, use a default identifier */
        sprintf(Identifier, "BIOSDISK%d", DriveNumber - FIRST_BIOS_DISK);
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
    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)", DriveNumber - FIRST_BIOS_DISK);
    AddReactOSArcDiskInfo(ArcName, Signature, Checksum, ValidPartitionTable);

    sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(0)", DriveNumber - FIRST_BIOS_DISK);
    FsRegisterDevice(ArcName, &UefiDiskVtbl);

    /* Add partitions */
    i = FIRST_PARTITION;
    DiskReportError(FALSE);
    while (DiskGetPartitionEntry(DriveNumber, i, &PartitionTableEntry))
    {
        if (PartitionTableEntry.SystemIndicator != PARTITION_ENTRY_UNUSED)
        {
            sprintf(ArcName, "multi(0)disk(0)rdisk(%u)partition(%lu)", DriveNumber - FIRST_BIOS_DISK, i);
            FsRegisterDevice(ArcName, &UefiDiskVtbl);
        }
        i++;
    }
    DiskReportError(TRUE);

    InternalUefiDisk[DriveNumber].NumOfPartitions = i;
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

    UINTN handle_size = 0;
    PcBiosDiskCount = 0;
    UefiBootRootIdentifier = 0;

    /* 1) Setup a list of boot handles by using the LocateHandle protocol */
    Status = GlobalSystemTable->BootServices->LocateHandle(ByProtocol, &bioGuid, NULL, &handle_size, NULL);
    
    TRACE("Initial LocateHandle call: Status=0x%lx, handle_size=%lu, EFI_BUFFER_TOO_SMALL=0x%lx\n", 
          (ULONG)Status, (ULONG)handle_size, (ULONG)EFI_BUFFER_TOO_SMALL);
    
    /* Check if the call succeeded to get the buffer size */
    if (Status == EFI_NOT_FOUND || handle_size == 0)
    {
        /* No block I/O devices found - this is possible in some UEFI setups */
        TRACE("No block I/O devices found (Status=0x%lx)\n", (ULONG)Status);
        handles = NULL;
        InternalUefiDisk = NULL;
        PcBiosDiskCount = 0;
        return;
    }
    
    /* When we pass NULL buffer, UEFI returns the required buffer size */
    /* The status should be EFI_BUFFER_TOO_SMALL (0x8000000000000005 on 64-bit) */
    /* But some implementations may just return 5. Check both cases. */
    ULONG StatusValue = (ULONG)Status;
    if (StatusValue == (ULONG)EFI_BUFFER_TOO_SMALL || StatusValue == 5 || 
        StatusValue == 0x80000005 || Status == EFI_BUFFER_TOO_SMALL)
    {
        /* This is the expected case - we have devices and need to allocate buffer */
        TRACE("LocateHandle needs buffer of size=%lu bytes for block devices\n", (ULONG)handle_size);
    }
    else if ((StatusValue == 0 || Status == EFI_SUCCESS) && handle_size > 0)
    {
        /* Some UEFI implementations might return success with a size */
        TRACE("LocateHandle returned success with handle_size=%lu\n", (ULONG)handle_size);
    }
    else
    {
        /* Only treat as error if we have neither success nor buffer_too_small with valid size */
        ERR("Unexpected LocateHandle status: Status=0x%lx, handle_size=%lu\n", StatusValue, (ULONG)handle_size);
        handles = NULL;
        InternalUefiDisk = NULL;
        PcBiosDiskCount = 0;
        return;
    }
    handles = MmAllocateMemoryWithType(handle_size, LoaderFirmwareTemporary);
    if (handles == NULL)
    {
        ERR("Failed to allocate memory for handles\n");
        return;
    }
    Status = GlobalSystemTable->BootServices->LocateHandle(ByProtocol, &bioGuid, NULL, &handle_size, handles);
    TRACE("Second LocateHandle call: Status=0x%lx, handle_size=%lu\n", (ULONG)Status, (ULONG)handle_size);
    
    /* Check if the second call succeeded */
    if (Status != EFI_SUCCESS && Status != 0)
    {
        ERR("Failed to locate handles: Status=0x%lx\n", (ULONG)Status);
        return;
    }
    
    SystemHandleCount = handle_size / sizeof(EFI_HANDLE);
    TRACE("Found %lu block I/O devices\n", SystemHandleCount);
    
    if (SystemHandleCount == 0)
    {
        TRACE("No block devices to enumerate\n");
        handles = NULL;
        InternalUefiDisk = NULL;
        PcBiosDiskCount = 0;
        return;
    }
    
    InternalUefiDisk = MmAllocateMemoryWithType(sizeof(INTERNAL_UEFI_DISK) * SystemHandleCount, LoaderFirmwareTemporary);
    if (InternalUefiDisk == NULL)
    {
        ERR("Failed to allocate memory for disk info\n");
        return;
    }

    BlockDeviceIndex = 0;
    /* 2) Parse the handle list */
    for (i = 0; i < SystemHandleCount; ++i)
    {
        Status = GlobalSystemTable->BootServices->HandleProtocol(handles[i], &bioGuid, (void**)&bio);
        if (handles[i] == PublicBootHandle)
        {
            OffsetToBoot = i; /* Drive offset in the handles list */
        }

        if (EFI_ERROR(Status) || 
            bio == NULL ||
            bio->Media->BlockSize == 0 ||
            bio->Media->BlockSize > 4096)
        {
            TRACE("UefiSetupBlockDevices: UEFI has found a block device that failed, skipping\n");
            continue;
        }
        if (bio->Media->LogicalPartition == FALSE)
        {
            TRACE("Found root of a HDD\n");
            PcBiosDiskCount++;
            InternalUefiDisk[BlockDeviceIndex].ArcDriveNumber = BlockDeviceIndex;
            InternalUefiDisk[BlockDeviceIndex].UefiRootNumber = i;
            GetHarddiskInformation(BlockDeviceIndex + FIRST_BIOS_DISK);
            BlockDeviceIndex++;
        }
        else if (handles[i] == PublicBootHandle)
        {
            GlobalSystemTable->BootServices->HandleProtocol(handles[i], &bioGuid, (void**)&bio);
            if (bio->Media->LogicalPartition == FALSE)
            {
                ULONG j;

                TRACE("Found root at index %u\n", i);
                UefiBootRootIdentifier = i;

                for (j = 0; j <= PcBiosDiskCount; ++j)
                {
                    /* Now only of the root drive number is equal to this drive we found above */
                    if (InternalUefiDisk[j].UefiRootNumber == UefiBootRootIdentifier)
                    {
                        InternalUefiDisk[j].IsThisTheBootDrive = TRUE;
                        PublicBootArcDisk = j;
                        TRACE("Found Boot drive\n");
                    }
                }
            }
        }
    }
}

static
BOOLEAN
UefiSetBootpath(VOID)
{
   TRACE("UefiSetBootpath: Setting up boot path\n");
   GlobalSystemTable->BootServices->HandleProtocol(handles[UefiBootRootIdentifier], &bioGuid, (void**)&bio);
   FrldrBootDrive = (FIRST_BIOS_DISK + PublicBootArcDisk);
   if (bio->Media->RemovableMedia == TRUE && bio->Media->BlockSize == 2048)
   {
        /* Boot Partition 0xFF is the magic value that indicates booting from CD-ROM (see isoboot.S) */
        FrldrBootPartition = 0xFF;
        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)cdrom(%u)", PublicBootArcDisk);
   }
   else
   {
        ULONG BootPartition;
        PARTITION_TABLE_ENTRY PartitionEntry;

        /* This is a hard disk */
        if (!UefiGetBootPartitionEntry(FrldrBootDrive, &PartitionEntry, &BootPartition))
        {
            ERR("Failed to get boot partition entry\n");
            return FALSE;
        }

        RtlStringCbPrintfA(FrLdrBootPath, sizeof(FrLdrBootPath),
                           "multi(0)disk(0)rdisk(%u)partition(%lu)",
                           PublicBootArcDisk, BootPartition);
    }

    return TRUE;
}

BOOLEAN
UefiInitializeBootDevices(VOID)
{
    ULONG i = 0;

    DiskReadBufferSize = EFI_PAGE_SIZE;
    DiskReadBuffer = MmAllocateMemoryWithType(DiskReadBufferSize, LoaderFirmwareTemporary);
    UefiSetupBlockDevices();
    UefiSetBootpath();
    
    // AGENT-MODIFIED: Enumerate all ARC disks for proper Windows boot support
    UefiEnumerateArcDisks();

    /* Check if handles were properly initialized */
    if (handles == NULL || InternalUefiDisk == NULL)
    {
        TRACE("No block devices available, skipping CD-ROM detection\n");
        return TRUE;
    }

    /* Add it, if it's a cdrom */
    GlobalSystemTable->BootServices->HandleProtocol(handles[UefiBootRootIdentifier], &bioGuid, (void**)&bio);
    if (bio->Media->RemovableMedia == TRUE && bio->Media->BlockSize == 2048)
    {
        PMASTER_BOOT_RECORD Mbr;
        PULONG Buffer;
        ULONG Checksum = 0;
        ULONG Signature;

        /* Read the MBR */
        if (!MachDiskReadLogicalSectors(FrldrBootDrive, 16ULL, 1, DiskReadBuffer))
        {
            ERR("Reading MBR failed\n");
            return FALSE;
        }

        Buffer = (ULONG*)DiskReadBuffer;
        Mbr = (PMASTER_BOOT_RECORD)DiskReadBuffer;

        Signature = Mbr->Signature;
        TRACE("Signature: %x\n", Signature);

        /* Calculate the MBR checksum */
        for (i = 0; i < 2048 / sizeof(ULONG); i++)
        {
            Checksum += Buffer[i];
        }
        Checksum = ~Checksum + 1;
        TRACE("Checksum: %x\n", Checksum);

        /* Fill out the ARC disk block */
        AddReactOSArcDiskInfo(FrLdrBootPath, Signature, Checksum, TRUE);

        FsRegisterDevice(FrLdrBootPath, &UefiDiskVtbl);
        PcBiosDiskCount++; // This is not accounted for in the number of pre-enumerated BIOS drives!
        TRACE("Additional boot drive detected: 0x%02X\n", (int)FrldrBootDrive);
    }
    return TRUE;
}

UCHAR
UefiGetFloppyCount(VOID)
{
    /* No floppy for you for now... */
    return 0;
}

BOOLEAN
UefiDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    ULONG UefiDriveNumber;
    EFI_STATUS Status;

    /* Validate drive number */
    if (DriveNumber < FIRST_BIOS_DISK || InternalUefiDisk == NULL)
    {
        ERR("Invalid drive number 0x%x (FIRST_BIOS_DISK=0x%x)\n", DriveNumber, FIRST_BIOS_DISK);
        return FALSE;
    }

    UefiDriveNumber = InternalUefiDisk[DriveNumber - FIRST_BIOS_DISK].UefiRootNumber;
    //TODO LOGS TEMP remove for not flodding logs TRACE("UefiDiskReadLogicalSectors: DriveNumber: 0x%x -> UefiDriveNumber: %d\n", DriveNumber, UefiDriveNumber);
    
    /* Validate UEFI drive number */
    if (handles == NULL || GlobalSystemTable == NULL || GlobalSystemTable->BootServices == NULL)
    {
        ERR("UEFI not properly initialized\n");
        return FALSE;
    }
    
    Status = GlobalSystemTable->BootServices->HandleProtocol(handles[UefiDriveNumber], &bioGuid, (void**)&bio);
    if (EFI_ERROR(Status) || bio == NULL)
    {
        ERR("Failed to get Block I/O protocol\n");
        return FALSE;
    }

    /* Devices setup */
    Status = bio->ReadBlocks(bio, bio->Media->MediaId, SectorNumber, SectorCount * bio->Media->BlockSize, Buffer);
    return !EFI_ERROR(Status);
}

BOOLEAN
UefiDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY Geometry)
{
    ULONG UefiDriveNumber;
    EFI_STATUS Status;

    /* Validate drive number */
    if (DriveNumber < FIRST_BIOS_DISK || InternalUefiDisk == NULL)
    {
        ERR("Invalid drive number 0x%x for geometry\n", DriveNumber);
        return FALSE;
    }

    UefiDriveNumber = InternalUefiDisk[DriveNumber - FIRST_BIOS_DISK].UefiRootNumber;
    
    /* Validate UEFI handles */
    if (handles == NULL || GlobalSystemTable == NULL || GlobalSystemTable->BootServices == NULL)
    {
        ERR("UEFI not properly initialized for geometry\n");
        return FALSE;
    }
    
    Status = GlobalSystemTable->BootServices->HandleProtocol(handles[UefiDriveNumber], &bioGuid, (void**)&bio);
    if (EFI_ERROR(Status) || bio == NULL || bio->Media == NULL)
    {
        ERR("Failed to get Block I/O protocol for geometry\n");
        return FALSE;
    }
    
    Geometry->Cylinders = 1; // Not relevant for the UEFI BIO protocol
    Geometry->Heads = 1;     // Not relevant for the UEFI BIO protocol
    Geometry->SectorsPerTrack = (bio->Media->LastBlock + 1);
    Geometry->BytesPerSector = bio->Media->BlockSize;
    Geometry->Sectors = (bio->Media->LastBlock + 1);

    return TRUE;
}

ULONG
UefiDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    ULONG UefiDriveNumber;
    EFI_STATUS Status;
    
    /* Validate drive number */
    if (DriveNumber < FIRST_BIOS_DISK || InternalUefiDisk == NULL)
    {
        ERR("Invalid drive number 0x%x for cache count\n", DriveNumber);
        return 0;
    }
    
    UefiDriveNumber = InternalUefiDisk[DriveNumber - FIRST_BIOS_DISK].UefiRootNumber;
    TRACE("UefiDiskGetCacheableBlockCount: DriveNumber: 0x%x -> UefiDriveNumber: %d\n", DriveNumber, UefiDriveNumber);

    /* Validate UEFI handles */
    if (handles == NULL || GlobalSystemTable == NULL || GlobalSystemTable->BootServices == NULL)
    {
        ERR("UEFI not properly initialized for cache count\n");
        return 0;
    }

    Status = GlobalSystemTable->BootServices->HandleProtocol(handles[UefiDriveNumber], &bioGuid, (void**)&bio);
    if (EFI_ERROR(Status) || bio == NULL || bio->Media == NULL)
    {
        ERR("Failed to get Block I/O protocol for cache count\n");
        return 0;
    }
    
    return (bio->Media->LastBlock + 1);
}
