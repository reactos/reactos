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

/* taken from linux fdisk src */
typedef enum PartitionTypes {
  PTEmpty = 0,
  PTDOS3xPrimary,  /*  1 */
  PTXENIXRoot,     /*  2 */
  PTXENIXUsr,      /*  3 */
  PTOLDDOS16Bit,   /*  4 */
  PTDosExtended,   /*  5 */
  PTDos5xPrimary,  /*  6 */
  PTOS2HPFS,       /*  7 */
  PTAIX,           /*  8 */
  PTAIXBootable,   /*  9 */
  PTOS2BootMgr,    /* 10 */
  PTWin95FAT32,
  PTWin95FAT32LBA,
  PTWin95FAT16LBA,
  PTWin95ExtendedLBA,
  PTVenix286 =      0x40,
  PTNovell =        0x51,
  PTMicroport =     0x52,
  PTGnuHurd =       0x63,
  PTNetware286 =    0x64,
  PTNetware386 =    0x65,
  PTPCIX =          0x75,
  PTOldMinix =      0x80,
  PTMinix =         0x81,
  PTLinuxSwap =     0x82,
  PTLinuxExt2 =     0x83,
  PTAmoeba =        0x93,
  PTAmoebaBBT =     0x94,
  PTBSD =           0xa5,
  PTBSDIFS =        0xb7,
  PTBSDISwap =      0xb8,
  PTSyrinx =        0xc7,
  PTCPM =           0xdb,
  PTDOSAccess =     0xe1,
  PTDOSRO =         0xe3,
  PTDOSSecondary =  0xf2,
  PTBBT =           0xff
} PARTITIONTYPES;

#define PartitionIsSupported(P)  \
    ((P)->PartitionType == PTDOS3xPrimary ||  \
    (P)->PartitionType == PTOLDDOS16Bit ||    \
    (P)->PartitionType == PTDos5xPrimary ||   \
    (P)->PartitionType == PTWin95FAT32 ||     \
    (P)->PartitionType == PTWin95FAT32LBA ||  \
    (P)->PartitionType == PTWin95FAT16LBA ||  \
    (P)->PartitionType == PTLinuxExt2)

#define PartitionIsExtended(P)  \
    ((P)->PartitionType == PTDosExtended)

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


