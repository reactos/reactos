/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __FS_H
#define __FS_H

//
// Define the structure of a partition table entry
//
typedef struct _PARTITION_TABLE_ENTRY
{
	BYTE	BootIndicator;					// 0x00 - non-bootable partition, 0x80 - bootable partition (one partition only)
	BYTE	StartHead;						// Beginning head number
	BYTE	StartSector;					// Beginning sector (2 high bits of cylinder #)
	BYTE	StartCylinder;					// Beginning cylinder# (low order bits of cylinder #)
	BYTE	SystemIndicator;				// System indicator
	BYTE	EndHead;						// Ending head number
	BYTE	EndSector;						// Ending sector (2 high bits of cylinder #)
	BYTE	EndCylinder;					// Ending cylinder# (low order bits of cylinder #)
	DWORD	SectorCountBeforePartition;		// Number of sectors preceding the partition
	DWORD	PartitionSectorCount;			// Number of sectors in the partition

} PACKED PARTITION_TABLE_ENTRY, *PPARTITION_TABLE_ENTRY;

//
// This macro will return the cylinder when you pass it a cylinder/sector
// pair where the high 2 bits of the cylinder are stored in the sector byte
//
#define MAKE_CYLINDER(cylinder, sector) ( cylinder + ((((WORD)sector) & 0xC0) << 2) )

//
// Define the structure of the master boot record
//
typedef struct _MASTER_BOOT_RECORD
{
	BYTE					MasterBootRecordCodeAndData[0x1be];
	PARTITION_TABLE_ENTRY	PartitionTable[4];
	WORD					MasterBootRecordMagic;

} PACKED MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;

//
// Partition type defines
//
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
#define PARTITION_PREP                  0x41      // PowerPC Reference Platform (PReP) Boot Partition
#define PARTITION_LDM                   0x42      // Logical Disk Manager partition
#define PARTITION_UNIX                  0x63      // Unix

typedef struct _GEOMETRY
{
	ULONG	Cylinders;
	ULONG	Heads;
	ULONG	Sectors;
	ULONG	BytesPerSector;

} GEOMETRY, *PGEOMETRY;

#define FILE VOID
#define PFILE FILE *

VOID	FileSystemError(PUCHAR ErrorString);
BOOL	OpenDiskDrive(ULONG DriveNumber, ULONG PartitionNumber);
VOID	SetDriveGeometry(ULONG Cylinders, ULONG Heads, ULONG Sectors, ULONG BytesPerSector);
VOID	SetVolumeProperties(ULONG HiddenSectors);
BOOL	ReadMultipleLogicalSectors(ULONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOL	ReadLogicalSector(ULONG SectorNumber, PVOID Buffer);
PFILE	OpenFile(PUCHAR FileName);
VOID	CloseFile(PFILE FileHandle);
BOOL	ReadFile(PFILE FileHandle, ULONG BytesToRead, PULONG BytesRead, PVOID Buffer);
ULONG	GetFileSize(PFILE FileHandle);
VOID	SetFilePointer(PFILE FileHandle, ULONG NewFilePointer);
ULONG	GetFilePointer(PFILE FileHandle);
BOOL	IsEndOfFile(PFILE FileHandle);


#define	EOF	-1

#define	ATTR_NORMAL		0x00
#define	ATTR_READONLY	0x01
#define	ATTR_HIDDEN		0x02
#define	ATTR_SYSTEM		0x04
#define	ATTR_VOLUMENAME	0x08
#define	ATTR_DIRECTORY	0x10
#define	ATTR_ARCHIVE	0x20

#define	FS_FAT			1
#define	FS_NTFS			2
#define	FS_EXT2			3

#define	FAT12			1
#define	FAT16			2
#define	FAT32			3

#endif // #defined __FS_H