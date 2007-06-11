/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/hal.h
 * PURPOSE:         Internal header for the I/O HAL Functions (Fstub)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */
#ifndef _HAL_
#define _HAL_

//
// Various offsets in the boot record
//
#define PARTITION_TABLE_OFFSET                      (0x1BE / 2)
#define BOOT_SIGNATURE_OFFSET                       ((0x200 / 2) - 1)
#define BOOT_RECORD_RESERVED                        0x1BC
#define BOOT_RECORD_SIGNATURE                       0xAA55
#define NUM_PARTITION_TABLE_ENTRIES                 4

//
// Helper Macros
//
#define GET_STARTING_SECTOR(p)                      \
    ((ULONG)(p->StartingSectorLsb0) +               \
     (ULONG)(p->StartingSectorLsb1 << 8 ) +         \
     (ULONG)(p->StartingSectorMsb0 << 16) +         \
     (ULONG)(p->StartingSectorMsb1 << 24))

#define GET_ENDING_S_OF_CHS(p)                      \
    ((UCHAR)(p->EndingCylinderLsb & 0x3F))

#define GET_PARTITION_LENGTH(p)                     \
    ((ULONG)(p->PartitionLengthLsb0) +              \
     (ULONG)(p->PartitionLengthLsb1 << 8) +         \
     (ULONG)(p->PartitionLengthMsb0 << 16) +        \
     (ULONG)(p->PartitionLengthMsb1 << 24))

//
// Structure describing a partition
//
typedef struct _PARTITION_DESCRIPTOR
{
    UCHAR ActiveFlag;
    UCHAR StartingTrack;
    UCHAR StartingCylinderLsb;
    UCHAR StartingCylinderMsb;
    UCHAR PartitionType;
    UCHAR EndingTrack;
    UCHAR EndingCylinderLsb;
    UCHAR EndingCylinderMsb;
    UCHAR StartingSectorLsb0;
    UCHAR StartingSectorLsb1;
    UCHAR StartingSectorMsb0;
    UCHAR StartingSectorMsb1;
    UCHAR PartitionLengthLsb0;
    UCHAR PartitionLengthLsb1;
    UCHAR PartitionLengthMsb0;
    UCHAR PartitionLengthMsb1;
} PARTITION_DESCRIPTOR, *PPARTITION_DESCRIPTOR;

//
// Structure describing a boot sector
//
typedef struct _BOOT_SECTOR_INFO
{
    UCHAR JumpByte[1];
    UCHAR Ignore1[2];
    UCHAR OemData[8];
    UCHAR BytesPerSector[2];
    UCHAR Ignore2[6];
    UCHAR NumberOfSectors[2];
    UCHAR MediaByte[1];
    UCHAR Ignore3[2];
    UCHAR SectorsPerTrack[2];
    UCHAR NumberOfHeads[2];
} BOOT_SECTOR_INFO, *PBOOT_SECTOR_INFO;

//
// Partition Table and Disk Layout
//
typedef struct _PARTITION_TABLE
{
    PARTITION_INFORMATION PartitionEntry[4];
} PARTITION_TABLE, *PPARTITION_TABLE;

typedef struct _DISK_LAYOUT
{
    ULONG TableCount;
    ULONG Signature;
    PARTITION_TABLE PartitionTable[1];
} DISK_LAYOUT, *PDISK_LAYOUT;

//
// Partition Table Entry
//
typedef struct _PTE
{
    UCHAR ActiveFlag;
    UCHAR StartingTrack;
    USHORT StartingCylinder;
    UCHAR PartitionType;
    UCHAR EndingTrack;
    USHORT EndingCylinder;
    ULONG StartingSector;
    ULONG PartitionLength;
} PTE, *PPTE;

#endif
