/**
*** Partition.h - defines and structs for harddrive partition info
***
***  05/30/98  RJJ  Created
**/

#ifndef __PARTITION_H
#define __PARTITION_H

#define  PARTITION_MAGIC    0xaa55
#define  PART_MAGIC_OFFSET  0x01fe
#define  PARTITION_OFFSET   0x01be
#define  PARTITION_TBL_SIZE 4
#define  PTCHSToLBA(c, h, s, scnt, hcnt) ((s) & 0x3f) + \
    (scnt) * ( (h) + (hcnt) * ((c) | (((s) & 0xc0) << 2)))
#define  PTLBAToCHS(lba, c, h, s, scnt, hcnt) ( \
    (s) = (lba) % (scnt) + 1,  \
    (lba) /= (scnt), \
    (h) = (lba) % (hcnt), \
    (lba) /= (heads), \
    (c) = (lba) & 0xff, \
    (s) |= ((lba) >> 2) & 0xc0)


typedef enum PartitionTypes {
  PTEmpty = 0,
  PTDOS3xPrimary = 1,
  PT_OLDDOS16Bit = 4,
  PTDosExtended = 5,
  PTDos5xPrimary = 6
} PARTITIONTYPES;

#define PartitionIsSupported(P) ((P)->PartitionType == PTDOS3xPrimary || \
    (P)->PartitionType == PT_OLDDOS16Bit || (P)->PartitionType == PTDos5xPrimary)

typedef struct Partition {
  __u8   BootFlags;
  __u8   StartingHead;
  __u8   StartingSector;
  __u8   StartingCylinder;
  __u8   PartitionType;
  __u8   EndingHead;
  __u8   EndingSector;
  __u8   EndingCylinder;
  unsigned int  StartingBlock;
  unsigned int  SectorCount;

} PARTITION;

#endif  // PARTITION_H


