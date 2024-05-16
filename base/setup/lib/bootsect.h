/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Installation helpers for BIOS-based PC boot sectors
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca@sfr.fr>
 */

#pragma once

/* TYPEDEFS ******************************************************************/

#define FAT_BOOTSECTOR_SIZE     (1 * SECTORSIZE)
#define FAT32_BOOTSECTOR_SIZE   (1 * SECTORSIZE) // Counts only the primary sector.
#define BTRFS_BOOTSECTOR_SIZE   (3 * SECTORSIZE)
#define NTFS_BOOTSECTOR_SIZE   (16 * SECTORSIZE)

#include <pshpack1.h>
typedef struct _FAT_BOOTSECTOR
{
    UCHAR       JumpBoot[3];                // Jump instruction to boot code
    CHAR        OemName[8];                 // "MSWIN4.1" for MS formatted volumes
    USHORT      BytesPerSector;             // Bytes per sector
    UCHAR       SectorsPerCluster;          // Number of sectors in a cluster
    USHORT      ReservedSectors;            // Reserved sectors, usually 1 (the bootsector)
    UCHAR       NumberOfFats;               // Number of FAT tables
    USHORT      RootDirEntries;             // Number of root directory entries (fat12/16)
    USHORT      TotalSectors;               // Number of total sectors on the drive, 16-bit
    UCHAR       MediaDescriptor;            // Media descriptor byte
    USHORT      SectorsPerFat;              // Sectors per FAT table (fat12/16)
    USHORT      SectorsPerTrack;            // Number of sectors in a track
    USHORT      NumberOfHeads;              // Number of heads on the disk
    ULONG       HiddenSectors;              // Hidden sectors (sectors before the partition start like the partition table)
    ULONG       TotalSectorsBig;            // This field is the new 32-bit total count of sectors on the volume
    UCHAR       DriveNumber;                // Int 0x13 drive number (e.g. 0x80)
    UCHAR       Reserved1;                  // Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
    UCHAR       BootSignature;              // Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
    ULONG       VolumeSerialNumber;         // Volume serial number
    CHAR        VolumeLabel[11];            // Volume label. This field matches the 11-byte volume label recorded in the root directory
    CHAR        FileSystemType[8];          // One of the strings "FAT12   ", "FAT16   ", or "FAT     "

    UCHAR       BootCodeAndData[448];       // The remainder of the boot sector

    USHORT      BootSectorMagic;            // 0xAA55

} FAT_BOOTSECTOR, *PFAT_BOOTSECTOR;
C_ASSERT(sizeof(FAT_BOOTSECTOR) == FAT_BOOTSECTOR_SIZE);

typedef struct _FAT32_BOOTSECTOR
{
    UCHAR       JumpBoot[3];                // Jump instruction to boot code
    CHAR        OemName[8];                 // "MSWIN4.1" for MS formatted volumes
    USHORT      BytesPerSector;             // Bytes per sector
    UCHAR       SectorsPerCluster;          // Number of sectors in a cluster
    USHORT      ReservedSectors;            // Reserved sectors, usually 1 (the bootsector)
    UCHAR       NumberOfFats;               // Number of FAT tables
    USHORT      RootDirEntries;             // Number of root directory entries (fat12/16)
    USHORT      TotalSectors;               // Number of total sectors on the drive, 16-bit
    UCHAR       MediaDescriptor;            // Media descriptor byte
    USHORT      SectorsPerFat;              // Sectors per FAT table (fat12/16)
    USHORT      SectorsPerTrack;            // Number of sectors in a track
    USHORT      NumberOfHeads;              // Number of heads on the disk
    ULONG       HiddenSectors;              // Hidden sectors (sectors before the partition start like the partition table)
    ULONG       TotalSectorsBig;            // This field is the new 32-bit total count of sectors on the volume
    ULONG       SectorsPerFatBig;           // This field is the FAT32 32-bit count of sectors occupied by ONE FAT. BPB_FATSz16 must be 0
    USHORT      ExtendedFlags;              // Extended flags (fat32)
    USHORT      FileSystemVersion;          // File system version (fat32)
    ULONG       RootDirStartCluster;        // Starting cluster of the root directory (fat32)
    USHORT      FsInfo;                     // Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
    USHORT      BackupBootSector;           // If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record. Usually 6.
    UCHAR       Reserved[12];               // Reserved for future expansion
    UCHAR       DriveNumber;                // Int 0x13 drive number (e.g. 0x80)
    UCHAR       Reserved1;                  // Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
    UCHAR       BootSignature;              // Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
    ULONG       VolumeSerialNumber;         // Volume serial number
    CHAR        VolumeLabel[11];            // Volume label. This field matches the 11-byte volume label recorded in the root directory
    CHAR        FileSystemType[8];          // Always set to the string "FAT32   "

    UCHAR       BootCodeAndData[420];       // The remainder of the boot sector

    USHORT      BootSectorMagic;            // 0xAA55

} FAT32_BOOTSECTOR, *PFAT32_BOOTSECTOR;
C_ASSERT(sizeof(FAT32_BOOTSECTOR) == FAT32_BOOTSECTOR_SIZE);

typedef struct _BTRFS_BOOTSECTOR
{
    UCHAR JumpBoot[3];
    UCHAR ChunkMapSize;
    UCHAR BootDrive;
    ULONGLONG PartitionStartLBA;
    UCHAR Fill[1521]; // 1536 - 15
    USHORT BootSectorMagic;
} BTRFS_BOOTSECTOR, *PBTRFS_BOOTSECTOR;
C_ASSERT(sizeof(BTRFS_BOOTSECTOR) == BTRFS_BOOTSECTOR_SIZE);

typedef struct _NTFS_BOOTSECTOR
{
    UCHAR Jump[3];
    UCHAR OEMID[8];
    USHORT BytesPerSector;
    UCHAR SectorsPerCluster;
    UCHAR Unused0[7];
    UCHAR MediaId;
    UCHAR Unused1[2];
    USHORT SectorsPerTrack;
    USHORT Heads;
    UCHAR Unused2[4];
    UCHAR Unused3[4];
    USHORT Unknown[2];
    ULONGLONG SectorCount;
    ULONGLONG MftLocation;
    ULONGLONG MftMirrLocation;
    CHAR ClustersPerMftRecord;
    UCHAR Unused4[3];
    CHAR ClustersPerIndexRecord;
    UCHAR Unused5[3];
    ULONGLONG SerialNumber;
    UCHAR Checksum[4];
    UCHAR BootStrap[426];
    USHORT EndSector;
    UCHAR BootCodeAndData[7680]; // The remainder of the boot sector (8192 - 512)
} NTFS_BOOTSECTOR, *PNTFS_BOOTSECTOR;
C_ASSERT(sizeof(NTFS_BOOTSECTOR) == NTFS_BOOTSECTOR_SIZE);

// TODO: Add more bootsector structures!

#include <poppack.h>


/* INSTALLATION HELPERS ******************************************************/

typedef NTSTATUS
(*PFS_INSTALL_BOOTCODE)(
    _Inout_ PBOOTCODE BootCode, // Source bootsector
    _In_ HANDLE DstPath,        // Where to save the bootsector built from the source + partition information
    _In_ HANDLE RootPartition); // Partition holding the (old) bootsector information

NTSTATUS
InstallMbrBootCode(
    _Inout_ PBOOTCODE BootCode,
    _In_ HANDLE DstPath,
    _In_ HANDLE DiskHandle);

NTSTATUS
InstallFatBootCode(
    _Inout_ PBOOTCODE BootCode,
    _In_ HANDLE DstPath,
    _In_ HANDLE RootPartition);

NTSTATUS
InstallFat32BootCode(
    _Inout_ PBOOTCODE BootCode,
    _In_ HANDLE DstPath,
    _In_ HANDLE RootPartition);

NTSTATUS
InstallBtrfsBootCode(
    _Inout_ PBOOTCODE BootCode,
    _In_ HANDLE DstPath,
    _In_ HANDLE RootPartition);

NTSTATUS
InstallNtfsBootCode(
    _Inout_ PBOOTCODE BootCode,
    _In_ HANDLE DstPath,
    _In_ HANDLE RootPartition);


/* FUNCTIONS *****************************************************************/

NTSTATUS
InstallBootCodeToDisk(
    _Inout_ PBOOTCODE BootCode,
    _In_ /*PCWSTR*/PCUNICODE_STRING RootPath,
    _In_ PFS_INSTALL_BOOTCODE InstallBootCode);

NTSTATUS
InstallBootCodeToFile(
    _Inout_ PBOOTCODE BootCode,
    _In_ /*PCWSTR*/PCUNICODE_STRING DstPath,
    _In_ /*PCWSTR*/PCUNICODE_STRING RootPath,
    _In_ PFS_INSTALL_BOOTCODE InstallBootCode);

/* EOF */
