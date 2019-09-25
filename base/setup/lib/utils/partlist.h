/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Partition list functions
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2018-2019 Hermes Belusca-Maito
 */

#pragma once

/* HELPERS FOR PARTITION TYPES **********************************************/

typedef struct _PARTITION_TYPE
{
    UCHAR Type;
    PCHAR Description;
} PARTITION_TYPE, *PPARTITION_TYPE;

#define NUM_PARTITION_TYPE_ENTRIES  143
extern PARTITION_TYPE PartitionTypes[NUM_PARTITION_TYPE_ENTRIES];


/* PARTITION UTILITY FUNCTIONS **********************************************/

typedef enum _FORMATSTATE
{
    Unformatted,
    UnformattedOrDamaged,
    UnknownFormat,
    Preformatted,
    Formatted
} FORMATSTATE, *PFORMATSTATE;

typedef struct _PARTENTRY
{
    LIST_ENTRY ListEntry;

    /* The disk this partition belongs to */
    struct _DISKENTRY *DiskEntry;

    /* Partition geometry */
    ULARGE_INTEGER StartSector;
    ULARGE_INTEGER SectorCount;

    BOOLEAN BootIndicator;  // NOTE: See comment for the PARTLIST::SystemPartition member.
    UCHAR PartitionType;
    ULONG OnDiskPartitionNumber; /* Enumerated partition number (primary partitions first, excluding the extended partition container, then the logical partitions) */
    ULONG PartitionNumber;       /* Current partition number, only valid for the currently running NTOS instance */
    ULONG PartitionIndex;        /* Index in the LayoutBuffer->PartitionEntry[] cached array of the corresponding DiskEntry */

    WCHAR DriveLetter;
    WCHAR VolumeLabel[20];
    WCHAR FileSystem[MAX_PATH+1];
    FORMATSTATE FormatState;

    BOOLEAN LogicalPartition;

    /* Partition is partitioned disk space */
    BOOLEAN IsPartitioned;

/** The following three properties may be replaced by flags **/

    /* Partition is new, table does not exist on disk yet */
    BOOLEAN New;

    /* Partition was created automatically */
    BOOLEAN AutoCreate;

    /* Partition must be checked */
    BOOLEAN NeedsCheck;

} PARTENTRY, *PPARTENTRY;

typedef struct _DISKENTRY
{
    LIST_ENTRY ListEntry;

    /* The list of disks/partitions this disk belongs to */
    struct _PARTLIST *PartList;

    MEDIA_TYPE MediaType;   /* FixedMedia or RemovableMedia */

    /* Disk geometry */

    ULONGLONG Cylinders;
    ULONG TracksPerCylinder;
    ULONG SectorsPerTrack;
    ULONG BytesPerSector;

    ULARGE_INTEGER SectorCount;
    ULONG SectorAlignment;
    ULONG CylinderAlignment;

    /* BIOS Firmware parameters */
    BOOLEAN BiosFound;
    ULONG HwAdapterNumber;
    ULONG HwControllerNumber;
    ULONG HwDiskNumber;         /* Disk number currently assigned on the system */
    ULONG HwFixedDiskNumber;    /* Disk number on the system when *ALL* removable disks are not connected */
//    ULONG Signature;  // Obtained from LayoutBuffer->Signature
//    ULONG Checksum;

    /* SCSI parameters */
    ULONG DiskNumber;
//  SCSI_ADDRESS;
    USHORT Port;
    USHORT Bus;
    USHORT Id;

    /* Has the partition list been modified? */
    BOOLEAN Dirty;

    BOOLEAN NewDisk; /* If TRUE, the disk is uninitialized */
    PARTITION_STYLE DiskStyle;  /* MBR/GPT-partitioned disk, or uninitialized disk (RAW) */

    UNICODE_STRING DriverName;

    PDRIVE_LAYOUT_INFORMATION LayoutBuffer;
    // TODO: When adding support for GPT disks:
    // Use PDRIVE_LAYOUT_INFORMATION_EX which indicates whether
    // the disk is MBR, GPT, or unknown (uninitialized).
    // Depending on the style, either use the MBR or GPT partition info.

    LIST_ENTRY PrimaryPartListHead; /* List of primary partitions */
    LIST_ENTRY LogicalPartListHead; /* List of logical partitions (Valid only for MBR-partitioned disks) */

    /* Pointer to the unique extended partition on this disk (Valid only for MBR-partitioned disks) */
    PPARTENTRY ExtendedPartition;

} DISKENTRY, *PDISKENTRY;

typedef struct _BIOSDISKENTRY
{
    LIST_ENTRY ListEntry;
    ULONG AdapterNumber;
    ULONG ControllerNumber;
    ULONG DiskNumber;
    ULONG Signature;
    ULONG Checksum;
    PDISKENTRY DiskEntry;   /* Corresponding recognized disk; is NULL if the disk is not recognized */ // RecognizedDiskEntry;
    CM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    CM_INT13_DRIVE_PARAMETER Int13DiskData;
} BIOSDISKENTRY, *PBIOSDISKENTRY;

typedef struct _PARTLIST
{
    /*
     * The system partition where the boot manager resides.
     * The corresponding system disk is obtained via:
     *    SystemPartition->DiskEntry.
     */
    // NOTE: It seems to appear that the specifications of ARC and (u)EFI
    // actually allow for multiple system partitions to exist on the system.
    // If so we should instead rely on the BootIndicator bit of the PARTENTRY
    // structure in order to find these.
    PPARTENTRY SystemPartition;

    LIST_ENTRY DiskListHead;
    LIST_ENTRY BiosDiskListHead;

} PARTLIST, *PPARTLIST;

#define  PARTITION_TBL_SIZE 4

#define PARTITION_MAGIC     0xAA55

/* Defines system type for MBR showing that a GPT is following */
#define EFI_PMBR_OSTYPE_EFI 0xEE

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

typedef struct
{
    LIST_ENTRY ListEntry;
    ULONG DiskNumber;
    ULONG Identifier;
    ULONG Signature;
} BIOS_DISK, *PBIOS_DISK;



ULONGLONG
AlignDown(
    IN ULONGLONG Value,
    IN ULONG Alignment);

ULONGLONG
AlignUp(
    IN ULONGLONG Value,
    IN ULONG Alignment);

ULONGLONG
RoundingDivide(
   IN ULONGLONG Dividend,
   IN ULONGLONG Divisor);


BOOLEAN
IsSuperFloppy(
    IN PDISKENTRY DiskEntry);

BOOLEAN
IsPartitionActive(
    IN PPARTENTRY PartEntry);

PPARTLIST
CreatePartitionList(VOID);

VOID
DestroyPartitionList(
    IN PPARTLIST List);

PDISKENTRY
GetDiskByBiosNumber(
    IN PPARTLIST List,
    IN ULONG HwDiskNumber);

PDISKENTRY
GetDiskByNumber(
    IN PPARTLIST List,
    IN ULONG DiskNumber);

PDISKENTRY
GetDiskBySCSI(
    IN PPARTLIST List,
    IN USHORT Port,
    IN USHORT Bus,
    IN USHORT Id);

PDISKENTRY
GetDiskBySignature(
    IN PPARTLIST List,
    IN ULONG Signature);

PPARTENTRY
GetPartition(
    // IN PPARTLIST List,
    IN PDISKENTRY DiskEntry,
    IN ULONG PartitionNumber);

BOOLEAN
GetDiskOrPartition(
    IN PPARTLIST List,
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber OPTIONAL,
    OUT PDISKENTRY* pDiskEntry,
    OUT PPARTENTRY* pPartEntry OPTIONAL);

PPARTENTRY
SelectPartition(
    IN PPARTLIST List,
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber);

PPARTENTRY
GetNextPartition(
    IN PPARTLIST List,
    IN PPARTENTRY CurrentPart OPTIONAL);

PPARTENTRY
GetPrevPartition(
    IN PPARTLIST List,
    IN PPARTENTRY CurrentPart OPTIONAL);

BOOLEAN
CreatePrimaryPartition(
    IN PPARTLIST List,
    IN OUT PPARTENTRY PartEntry,
    IN ULONGLONG SectorCount,
    IN BOOLEAN AutoCreate);

BOOLEAN
CreateExtendedPartition(
    IN PPARTLIST List,
    IN OUT PPARTENTRY PartEntry,
    IN ULONGLONG SectorCount);

BOOLEAN
CreateLogicalPartition(
    IN PPARTLIST List,
    IN OUT PPARTENTRY PartEntry,
    IN ULONGLONG SectorCount,
    IN BOOLEAN AutoCreate);

NTSTATUS
DismountVolume(
    IN PPARTENTRY PartEntry);

BOOLEAN
DeletePartition(
    IN PPARTLIST List,
    IN PPARTENTRY PartEntry,
    OUT PPARTENTRY* FreeRegion OPTIONAL);

PPARTENTRY
FindSupportedSystemPartition(
    IN PPARTLIST List,
    IN BOOLEAN ForceSelect,
    IN PDISKENTRY AlternativeDisk OPTIONAL,
    IN PPARTENTRY AlternativePart OPTIONAL);

BOOLEAN
SetActivePartition(
    IN PPARTLIST List,
    IN PPARTENTRY PartEntry,
    IN PPARTENTRY OldActivePart OPTIONAL);

NTSTATUS
WritePartitions(
    IN PDISKENTRY DiskEntry);

BOOLEAN
WritePartitionsToDisk(
    IN PPARTLIST List);

BOOLEAN
SetMountedDeviceValue(
    IN WCHAR Letter,
    IN ULONG Signature,
    IN LARGE_INTEGER StartingOffset);

BOOLEAN
SetMountedDeviceValues(
    IN PPARTLIST List);

VOID
SetPartitionType(
    IN PPARTENTRY PartEntry,
    IN UCHAR PartitionType);

ERROR_NUMBER
PrimaryPartitionCreationChecks(
    IN PPARTENTRY PartEntry);

ERROR_NUMBER
ExtendedPartitionCreationChecks(
    IN PPARTENTRY PartEntry);

ERROR_NUMBER
LogicalPartitionCreationChecks(
    IN PPARTENTRY PartEntry);

BOOLEAN
GetNextUnformattedPartition(
    IN PPARTLIST List,
    OUT PDISKENTRY *pDiskEntry OPTIONAL,
    OUT PPARTENTRY *pPartEntry);

BOOLEAN
GetNextUncheckedPartition(
    IN PPARTLIST List,
    OUT PDISKENTRY *pDiskEntry OPTIONAL,
    OUT PPARTENTRY *pPartEntry);

/* EOF */
