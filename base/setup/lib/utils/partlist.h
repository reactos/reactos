/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Partition list functions
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2018-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

/* EXTRA HANDFUL MACROS *****************************************************/

// NOTE: They should be moved into some global header.

/* OEM MBR partition types recognized by NT (see [MS-DMRP] Appendix B) */
#define PARTITION_EISA          0x12    // EISA partition
#define PARTITION_HIBERNATION   0x84    // Hibernation partition for laptops
#define PARTITION_DIAGNOSTIC    0xA0    // Diagnostic partition on some Hewlett-Packard (HP) notebooks
#define PARTITION_DELL          0xDE    // Dell partition
#define PARTITION_IBM           0xFE    // IBM Initial Microprogram Load (IML) partition

#define IsOEMPartition(PartitionType) \
    ( ((PartitionType) == PARTITION_EISA)        || \
      ((PartitionType) == PARTITION_HIBERNATION) || \
      ((PartitionType) == PARTITION_DIAGNOSTIC)  || \
      ((PartitionType) == PARTITION_DELL)        || \
      ((PartitionType) == PARTITION_IBM) )


/* PARTITION UTILITY FUNCTIONS **********************************************/

typedef enum _FORMATSTATE
{
    Unformatted,
    UnformattedOrDamaged,
    UnknownFormat,
    Formatted
} FORMATSTATE, *PFORMATSTATE;

#include "volutil.h"

typedef struct _PARTENTRY PARTENTRY, *PPARTENTRY;
typedef struct _VOLENTRY
{
    LIST_ENTRY ListEntry; ///< Entry in VolumesList

    VOLINFO Info;
    FORMATSTATE FormatState;

    /* Volume must be checked */
    BOOLEAN NeedsCheck;
    /* Volume is new and has not yet been actually formatted and mounted */
    BOOLEAN New;

    // union {
    //     PVOLUME_DISK_EXTENTS pExtents;
        PPARTENTRY PartEntry;
    // };
} VOLENTRY, *PVOLENTRY;

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
    WCHAR DeviceName[MAX_PATH];  ///< NT device name: "\Device\HarddiskM\PartitionN"

    BOOLEAN LogicalPartition;

    /* Partition is partitioned disk space */
    BOOLEAN IsPartitioned;

    /* Partition is new, table does not exist on disk yet */
    BOOLEAN New;

    /*
     * Volume-related properties:
     * NULL: No volume is associated to this partition (either because it is
     *       an empty disk region, or the partition type is unrecognized).
     * 0x1 : TBD.
     * Valid pointer: A basic volume associated to this partition is (or will)
     *       be mounted by the PARTMGR and enumerated by the MOUNTMGR.
     */
    PVOLENTRY Volume;

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

    /* (Basic) Volumes management */
    LIST_ENTRY VolumesList;

} PARTLIST, *PPARTLIST;

#define PARTITION_TBL_SIZE  4

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


#define GetPartEntryOffsetInBytes(PartEntry) \
    ((PartEntry)->StartSector.QuadPart * (PartEntry)->DiskEntry->BytesPerSector)

#define GetPartEntrySizeInBytes(PartEntry) \
    ((PartEntry)->SectorCount.QuadPart * (PartEntry)->DiskEntry->BytesPerSector)

#define GetDiskSizeInBytes(DiskEntry) \
    ((DiskEntry)->SectorCount.QuadPart * (DiskEntry)->BytesPerSector)


BOOLEAN
IsDiskSuperFloppy2(
    _In_ const DISK_PARTITION_INFO* DiskInfo,
    _In_opt_ const ULONGLONG* DiskSize,
    _In_ const PARTITION_INFORMATION* PartitionInfo);

BOOLEAN
IsDiskSuperFloppy(
    _In_ const DRIVE_LAYOUT_INFORMATION* Layout,
    _In_opt_ const ULONGLONG* DiskSize);

BOOLEAN
IsDiskSuperFloppyEx(
    _In_ const DRIVE_LAYOUT_INFORMATION_EX* LayoutEx,
    _In_opt_ const ULONGLONG* DiskSize);

BOOLEAN
IsSuperFloppy(
    _In_ PDISKENTRY DiskEntry);

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
    _In_ PPARTLIST List,
    _In_ ULONG HwDiskNumber);

PDISKENTRY
GetDiskByNumber(
    _In_ PPARTLIST List,
    _In_ ULONG DiskNumber);

PDISKENTRY
GetDiskBySCSI(
    _In_ PPARTLIST List,
    _In_ USHORT Port,
    _In_ USHORT Bus,
    _In_ USHORT Id);

PDISKENTRY
GetDiskBySignature(
    _In_ PPARTLIST List,
    _In_ ULONG Signature);

PPARTENTRY
GetPartition(
    _In_ PDISKENTRY DiskEntry,
    _In_ ULONG PartitionNumber);

PPARTENTRY
SelectPartition(
    _In_ PPARTLIST List,
    _In_ ULONG DiskNumber,
    _In_ ULONG PartitionNumber);

PPARTENTRY
GetNextPartition(
    IN PPARTLIST List,
    IN PPARTENTRY CurrentPart OPTIONAL);

PPARTENTRY
GetPrevPartition(
    IN PPARTLIST List,
    IN PPARTENTRY CurrentPart OPTIONAL);

PPARTENTRY
GetAdjUnpartitionedEntry(
    _In_ PPARTENTRY PartEntry,
    _In_ BOOLEAN Direction);

ERROR_NUMBER
PartitionCreationChecks(
    _In_ PPARTENTRY PartEntry);

ERROR_NUMBER
ExtendedPartitionCreationChecks(
    _In_ PPARTENTRY PartEntry);

BOOLEAN
CreatePartition(
    _In_ PPARTLIST List,
    _Inout_ PPARTENTRY PartEntry,
    _In_opt_ ULONGLONG SizeBytes,
    _In_opt_ ULONG_PTR PartitionInfo);

BOOLEAN
DeletePartition(
    _In_ PPARTLIST List,
    _In_ PPARTENTRY PartEntry,
    _Out_opt_ PPARTENTRY* FreeRegion);

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
SetMountedDeviceValues(
    _In_ PPARTLIST List);

VOID
SetMBRPartitionType(
    IN PPARTENTRY PartEntry,
    IN UCHAR PartitionType);

/* EOF */
