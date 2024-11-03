/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#pragma once

#include <reactos/rosioctl.h>

/* FreeLoader-specific disk geometry structure */
typedef struct _GEOMETRY
{
    ULONG Cylinders;       ///< Number of cylinders on the disk
    ULONG Heads;           ///< Number of heads on the disk
    ULONG SectorsPerTrack; ///< Number of sectors per track
    ULONG BytesPerSector;  ///< Number of bytes per sector
    ULONGLONG Sectors;     ///< Total number of disk sectors/LBA blocks
} GEOMETRY, *PGEOMETRY;

#include <pshpack1.h>

/*
 * Extended disk geometry (Int13 / AH=48h)
 * See also ntdddisk.h DISK_EX_INT13_INFO
 */
typedef struct _EXTENDED_GEOMETRY
{
    USHORT      Size;
    USHORT      Flags;
    ULONG       Cylinders;
    ULONG       Heads;
    ULONG       SectorsPerTrack;
    ULONGLONG   Sectors;
    USHORT      BytesPerSector;
    ULONG       PDPTE;
} EXTENDED_GEOMETRY, *PEXTENDED_GEOMETRY;

/*
 * Define the structure of a partition table entry
 */
typedef struct _PARTITION_TABLE_ENTRY
{
    UCHAR   BootIndicator;              // 0x00 - non-bootable partition,
                                        // 0x80 - bootable partition (one partition only)
    UCHAR   StartHead;                  // Beginning head number
    UCHAR   StartSector;                // Beginning sector (2 high bits of cylinder #)
    UCHAR   StartCylinder;              // Beginning cylinder# (low order bits of cylinder #)
    UCHAR   SystemIndicator;            // System indicator
    UCHAR   EndHead;                    // Ending head number
    UCHAR   EndSector;                  // Ending sector (2 high bits of cylinder #)
    UCHAR   EndCylinder;                // Ending cylinder# (low order bits of cylinder #)
    ULONG   SectorCountBeforePartition; // Number of sectors preceding the partition
    ULONG   PartitionSectorCount;       // Number of sectors in the partition
} PARTITION_TABLE_ENTRY, *PPARTITION_TABLE_ENTRY;

/*
 * Define the structure of the master boot record
 */
typedef struct _MASTER_BOOT_RECORD
{
    UCHAR   MasterBootRecordCodeAndData[0x1b8]; /* 0x000 */
    ULONG   Signature;                          /* 0x1B8 */
    USHORT  Reserved;                           /* 0x1BC */
    PARTITION_TABLE_ENTRY   PartitionTable[4];  /* 0x1BE */
    USHORT  MasterBootRecordMagic;              /* 0x1FE */
} MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;

#include <poppack.h>

/*
 * Partition type defines (of PSDK)
 */
#define PARTITION_ENTRY_UNUSED          0x00      // Entry unused
#define PARTITION_FAT_12                0x01      // 12-bit FAT entries
#define PARTITION_XENIX_1               0x02      // Xenix
#define PARTITION_XENIX_2               0x03      // Xenix
#define PARTITION_FAT_16                0x04      // 16-bit FAT entries
#define PARTITION_EXTENDED              0x05      // Extended partition entry
#define PARTITION_HUGE                  0x06      // Huge partition MS-DOS V4
#define PARTITION_IFS                   0x07      // IFS Partition
#define PARTITION_OS2BOOTMGR            0x0A      // OS/2 Boot Manager/OPUS/Coherent swap
#define PARTITION_FAT32                 0x0B      // FAT32
#define PARTITION_FAT32_XINT13          0x0C      // FAT32 using extended int13 services
#define PARTITION_XINT13                0x0E      // Win95 partition using extended int13 services
#define PARTITION_XINT13_EXTENDED       0x0F      // Same as type 5 but uses extended int13 services
#define PARTITION_NTFS                  0x17      // NTFS
#define PARTITION_PREP                  0x41      // PowerPC Reference Platform (PReP) Boot Partition
#define PARTITION_LDM                   0x42      // Logical Disk Manager partition
#define PARTITION_UNIX                  0x63      // Unix
#define VALID_NTFT                      0xC0      // NTFT uses high order bits
#define PARTITION_NTFT                  0x80      // NTFT partition
#define PARTITION_GPT                   0xEE      // GPT protective partition
#ifdef __REACTOS__
#define PARTITION_OLD_LINUX             0x43
#define PARTITION_LINUX                 0x83
#endif

///////////////////////////////////////////////////////////////////////////////////////
//
// PC x86/64 BIOS Disk Functions (pcdisk.c)
//
///////////////////////////////////////////////////////////////////////////////////////
#if defined(__i386__) || defined(_M_AMD64)
VOID __cdecl DiskStopFloppyMotor(VOID);
#endif // defined __i386__ || defined(_M_AMD64)

/* Buffer for disk reads (hwdisk.c) */
extern PVOID DiskReadBuffer;
extern SIZE_T DiskReadBufferSize;


/* ARC path of the boot drive and partition */
extern CCHAR FrLdrBootPath[MAX_PATH];


///////////////////////////////////////////////////////////////////////////////////////
//
// Fixed Disk Partition Management Functions
//
///////////////////////////////////////////////////////////////////////////////////////

VOID
DiskDetectPartitionType(
    IN UCHAR DriveNumber);

BOOLEAN
DiskGetBootPartitionEntry(
    IN UCHAR DriveNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry,
    OUT PULONG BootPartition);

BOOLEAN
DiskGetPartitionEntry(
    IN UCHAR DriveNumber,
    IN ULONG PartitionNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry);

/*
 * SCSI support (disk/scsiport.c)
 */
ULONG LoadBootDeviceDriver(VOID);
