/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2024-2025  Daniel Victor <ilauncherdeveloper@gmail.com>
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

/*
 *  grub/fs/ext2.c
 *
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007  Free Software Foundation, Inc.
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
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pshpack1.h>

#define EXT_SUPERBLOCK_MAGIC 0xEF53
#define EXT_DYNAMIC_REVISION 1
#define EXT_DEFAULT_INODE_SIZE 128
#define EXT_DEFAULT_GROUP_DESC_SIZE 32

#define EXT_DIR_ENTRY_MAX_NAME_LENGTH 255

typedef struct _ExtSuperBlock
{
    /* SuperBlock Information Ext2 */
    ULONG InodesCount;
    ULONG BlocksCountLo;
    ULONG RBlocksCountLo;
    ULONG FreeBlocksCountLo;
    ULONG FreeInodesCount;
    ULONG FirstDataBlock;
    ULONG LogBlockSize;
    LONG LogFragSize;
    ULONG BlocksPerGroup;
    ULONG FragsPerGroup;
    ULONG InodesPerGroup;
    ULONG MTime;
    ULONG WTime;
    USHORT MntCount;
    USHORT MaxMntCount;
    USHORT Magic;
    USHORT State;
    USHORT Errors;
    USHORT MinorRevisionLevel;
    ULONG LastCheck;
    ULONG CheckInterval;
    ULONG CreatorOS;
    ULONG RevisionLevel;
    USHORT DefResUID;
    USHORT DefResGID;

    /* SuperBlock Information Ext3 */
    ULONG FirstInode;
    USHORT InodeSize;
    USHORT BlockGroupNr;
    ULONG FeatureCompat;
    ULONG FeatureIncompat;
    ULONG FeatureROCompat;
    UCHAR UUID[16];
    CHAR VolumeName[16];
    CHAR LastMounted[64];
    ULONG AlgorithmUsageBitmap;
    UCHAR PreallocBlocks;
    UCHAR PreallocDirBlocks;
    USHORT ReservedGdtBlocks;
    UCHAR JournalUUID[16];
    ULONG JournalInum;
    ULONG JournalDev;
    ULONG LastOrphan;
    ULONG HashSeed[4];
    UCHAR DefHashVersion;
    UCHAR JournalBackupType;
    USHORT GroupDescSize;

    ULONG DefaultMountOpts;
    ULONG FirstMetaBg;
    ULONG MkfsTime;
    ULONG JnlBlocks[17];

    /* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
    ULONG BlocksCountHi;
    ULONG RBlocksCountHi;
    ULONG FreeBlocksCountHi;

    UCHAR Reserved[676];
} EXT_SUPER_BLOCK, *PEXT_SUPER_BLOCK;
C_ASSERT(sizeof(EXT_SUPER_BLOCK) == 0x400);

typedef struct _ExtGroupDescriptor
{
    ULONG BlockBitmap;
    ULONG InodeBitmap;
    ULONG InodeTable;
    USHORT FreeBlocksCount;
    USHORT FreeInodesCount;
    USHORT UsedDirsCount;
} EXT_GROUP_DESC, *PEXT_GROUP_DESC;

typedef struct _Ext4ExtentHeader
{
    USHORT Magic;
    USHORT Entries;
    USHORT Max;
    USHORT Depth;
    ULONG Generation;
} EXT4_EXTENT_HEADER, *PEXT4_EXTENT_HEADER;

typedef struct _Ext4ExtentIdx
{
    ULONG Block;
    ULONG Leaf;
    USHORT LeafHigh;
    USHORT Unused;
} EXT4_EXTENT_IDX, *PEXT4_EXTENT_IDX;

typedef struct _Ext4Extent
{
    ULONG Block;
    USHORT Length;
    USHORT StartHigh;
    ULONG Start;
} EXT4_EXTENT, *PEXT4_EXTENT;

typedef struct _ExtInode
{
    USHORT Mode; // Combination of _S_IF* mode flags (FILETYPE_INO_* in grub/fs/ext2.c)
    USHORT UID;
    ULONG Size;
    ULONG Atime;
    ULONG Ctime;
    ULONG Mtime;
    ULONG Dtime;
    USHORT GID;
    USHORT LinksCount;
    ULONG BlocksCount;
    ULONG Flags;
    ULONG OSD1;
    union
    {
        CHAR SymLink[60];
        struct
        {
            ULONG DirectBlocks[12];
            ULONG IndirectBlock;
            ULONG DoubleIndirectBlock;
            ULONG TripleIndirectBlock;
        } Blocks;
        EXT4_EXTENT_HEADER ExtentHeader;
    };
    ULONG Generation;
    ULONG FileACL;
    ULONG DirACL;
    ULONG FragAddress;
    ULONG OSD2[3];
} EXT_INODE, *PEXT_INODE;

typedef struct _ExtDirEntry
{
    ULONG Inode;
    USHORT EntryLen;
    UCHAR NameLen;
    UCHAR FileType; // One of the FILETYPE_* values
    CHAR Name[EXT_DIR_ENTRY_MAX_NAME_LENGTH];
} EXT_DIR_ENTRY, *PEXT_DIR_ENTRY;

#include <poppack.h>

/* Special inode numbers.  */
#define EXT_ROOT_INODE 2

/* The revision level.  */
#define EXT_REVISION(sb) (sb->RevisionLevel)

/* The inode size.  */
#define EXT_INODE_SIZE(sb) \
    (EXT_REVISION(sb) < EXT_DYNAMIC_REVISION ? EXT_DEFAULT_INODE_SIZE : sb->InodeSize)

/* The group descriptor size.  */
#define EXT_GROUP_DESC_SIZE(sb) \
    ((EXT_REVISION(sb) >= EXT_DYNAMIC_REVISION && sb->GroupDescSize) ? sb->GroupDescSize : EXT_DEFAULT_GROUP_DESC_SIZE)

/* The inode extents flag.  */
#define EXT4_INODE_FLAG_EXTENTS 0x80000

/* The extent header magic value.  */
#define EXT4_EXTENT_HEADER_MAGIC 0xF30A

/* The maximum extent level.  */
#define EXT4_EXTENT_MAX_LEVEL 5

/* The maximum extent length used to check for sparse extents.  */
#define EXT4_EXTENT_MAX_LENGTH 32768

/* Filetype used in directory entry.  */
#define FILETYPE_UNKNOWN    0
#define FILETYPE_REG        1
#define FILETYPE_DIRECTORY  2
#define FILETYPE_SYMLINK    7

#define FAST_SYMLINK_MAX_NAME_SIZE 60

typedef struct _EXT_VOLUME_INFO *PEXT_VOLUME_INFO;

typedef struct _EXT_FILE_INFO
{
    PEXT_VOLUME_INFO Volume;
    PULONG FileBlockList;   // File block list
    EXT_INODE Inode;        // File inode
    ULONGLONG FileSize;     // File size
    ULONGLONG FilePointer;  // File pointer
    ULONG FileNameLength;
    UCHAR Attributes;
    CHAR FileName[RTL_FIELD_SIZE(FILEINFORMATION, FileName)];
} EXT_FILE_INFO, *PEXT_FILE_INFO;

ULONGLONG
ExtGetVolumeSize(
    _In_ ULONG DeviceId);

const DEVVTBL* ExtMount(ULONG DeviceId);
