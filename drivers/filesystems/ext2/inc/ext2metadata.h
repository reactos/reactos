/*************************************************************************
*
* File: ext2metadata.h
*
* Module: Ext2 File System Driver (Kernel mode execution only)
*
* Description:
*	Contains the definitions for the Ext2 Metadata structures.
*
* Author: Manoj Paul Joseph
*
*
*************************************************************************/

#ifndef EXT2_METADATA_STRUCTURES
#define EXT2_METADATA_STRUCTURES

//
//	Some type definitions...
//	These are used in the ext2_fs.h header file
//
typedef unsigned int		__u32 ;
typedef signed int			__s32 ;
typedef unsigned short int	__u16 ;
typedef signed short int	__s16 ;
typedef unsigned char		__u8 ;

//
//******************************************************
//
//	Using Remy Card's (slightly modified) Ext2 header...
//	
#include "ext2_fs.h"
//
//******************************************************
//

typedef struct ext2_super_block	EXT2_SUPER_BLOCK;
typedef EXT2_SUPER_BLOCK *		PEXT2_SUPER_BLOCK;

typedef struct ext2_inode		EXT2_INODE;
typedef EXT2_INODE *			PEXT2_INODE;

typedef struct ext2_group_desc	EXT2_GROUP_DESCRIPTOR;
typedef EXT2_GROUP_DESCRIPTOR *	PEXT2_GROUP_DESCRIPTOR;

typedef struct ext2_dir_entry_2	EXT2_DIR_ENTRY;
typedef EXT2_DIR_ENTRY *		PEXT2_DIR_ENTRY;

//
// Ext2 Supported File Types...
//
#define IMODE_FIFO            0x01
#define IMODE_CHARDEV         0x02
#define IMODE_DIR             0x04
#define IMODE_BLOCKDEV        0x06
#define IMODE_FILE            0x08
#define IMODE_SLINK           0x0A
#define IMODE_SOCKET          0x0C

#define _MKMODE(m)					 ( ( (m) >> 12 ) & 0x000F)
#define Ext2IsModeRegularFile(m)	 ( _MKMODE(m) == IMODE_FILE )
#define Ext2IsModeDirectory(m)		 ( _MKMODE(m) == IMODE_DIR   )
#define Ext2IsModeSymbolicLink(m)	 ( _MKMODE(m) == IMODE_SLINK )
#define Ext2IsModePipe(m)			 ( _MKMODE(m) == IMODE_FIFO )
#define Ext2IsModeCharacterDevice(m) ( _MKMODE(m) == IMODE_CHARDEV )
#define Ext2IsModeBlockDevice(m)	 ( _MKMODE(m) == IMODE_BLOCKDEV )
#define Ext2IsModeSocket(m)			 ( _MKMODE(m) == IMODE_SOCKET )

#define Ext2IsModeHidden(m)			( (m & 0x124) == 0)	//	No Read Permission
#define Ext2IsModeReadOnly(m)		( (m & 0x92) == 0)	//	No write Permission

#define Ext2SetModeHidden(m)		m = (m & (~0x124));	//	Turn off Read Permission
#define Ext2SetModeReadOnly(m)		m = (m & (~0x92));	//	Turn off write Permission
#define Ext2SetModeReadWrite(m)		m = (m & 0x1ff);	//	Set read/write Permission


//
//  Define the Packed and Unpacked BIOS Parameter Block
//
typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       // offset = 0x000  0
    UCHAR  SectorsPerCluster[1];                    // offset = 0x002  2
    UCHAR  ReservedSectors[2];                      // offset = 0x003  3
    UCHAR  Fats[1];                                 // offset = 0x005  5
    UCHAR  RootEntries[2];                          // offset = 0x006  6
    UCHAR  Sectors[2];                              // offset = 0x008  8
    UCHAR  Media[1];                                // offset = 0x00A 10
    UCHAR  SectorsPerFat[2];                        // offset = 0x00B 11
    UCHAR  SectorsPerTrack[2];                      // offset = 0x00D 13
    UCHAR  Heads[2];                                // offset = 0x00F 15
    UCHAR  HiddenSectors[4];                        // offset = 0x011 17
    UCHAR  LargeSectors[4];                         // offset = 0x015 21
} PACKED_BIOS_PARAMETER_BLOCK;                      // sizeof = 0x019 25
typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

//
//  Define the boot sector
//
typedef struct _PACKED_BOOT_SECTOR {
    UCHAR Jump[3];                                  // offset = 0x000   0
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;          // offset = 0x00B  11
    UCHAR PhysicalDriveNumber;                      // offset = 0x024  36
    UCHAR CurrentHead;                              // offset = 0x025  37
    UCHAR Signature;                                // offset = 0x026  38
    UCHAR Id[4];                                    // offset = 0x027  39
    UCHAR VolumeLabel[11];                          // offset = 0x02B  43
    UCHAR SystemId[8];                              // offset = 0x036  54
} PACKED_BOOT_SECTOR;                               // sizeof = 0x03E  62

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;


#endif
