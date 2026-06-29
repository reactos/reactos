/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     MBR partitioning scheme support
 * COPYRIGHT:   Copyright 2002-2003 Brian Palmer <brianp@sginet.com>
 */

#pragma once

#include <pshpack1.h>

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
C_ASSERT(sizeof(PARTITION_TABLE_ENTRY) == 16);

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
C_ASSERT(sizeof(MASTER_BOOT_RECORD) == 512);

#include <poppack.h>

/*
 * Partition type defines (of PSDK)
 */
#include <reactos/rosioctl.h>
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
