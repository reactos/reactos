/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        vfatlib.h
 */
#define NDEBUG
#include <debug.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <fslib/vfatlib.h>

#define SECTORSIZE 512

typedef struct _FAT32_BOOT_SECTOR
{
  unsigned char  magic0;                      // 0
  unsigned char  res0;                        // 1
  unsigned char  magic1;                      // 2
  unsigned char  OEMName[8];                  // 3
  unsigned short BytesPerSector;              // 11
  unsigned char  SectorsPerCluster;           // 13
  unsigned short ReservedSectors;             // 14
  unsigned char  FATCount;                    // 16
  unsigned short RootEntries;                 // 17
  unsigned short Sectors;                     // 19
  unsigned char  Media;                       // 21
  unsigned short FATSectors;                  // 22
  unsigned short SectorsPerTrack;             // 24
  unsigned short Heads;	                      // 22
  unsigned long  HiddenSectors;               // 24
  unsigned long  SectorsHuge;                 // 28
  unsigned long  FATSectors32;                // 36
  unsigned short ExtFlag;                     // 40
  unsigned short FSVersion;                   // 42
  unsigned long  RootCluster;                 // 44
  unsigned short FSInfoSector;                // 48
  unsigned short BootBackup;                  // 50
  unsigned char  Res3[12];                    // 52
  unsigned char  Drive;                       // 64
  unsigned char  Res4;                        // 65
  unsigned char  ExtBootSignature;            // 66
  unsigned long  VolumeID;                    // 67
  unsigned char  VolumeLabel[11];             // 71
  unsigned char  SysType[8];                  // 82
  unsigned char  Res2[418];                   // 90
  unsigned long  Signature1;                  // 508
} __attribute__((packed)) FAT32_BOOT_SECTOR, *PFAT32_BOOT_SECTOR;

typedef struct _FAT32_FSINFO
{
  unsigned int  LeadSig;          // 0
  unsigned char Res1[480];        // 4
  unsigned int  StrucSig;         // 484
  unsigned int  FreeCount;        // 488
  unsigned int  NextFree;         // 492
  unsigned int  Res2;             // 496
} __attribute__((packed)) FAT32_FSINFO, *PFAT32_FSINFO;
