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


typedef struct Partition {
  unsigned char   BootFlags;
  unsigned char   StartingHead;
  unsigned char   StartingSector;
  unsigned char   StartingCylinder;
  unsigned char   PartitionType;
  unsigned char   EndingHead;
  unsigned char   EndingSector;
  unsigned char   EndingCylinder;
  unsigned int  StartingBlock;
  unsigned int  SectorCount;

} PARTITION;

#endif  // PARTITION_H


