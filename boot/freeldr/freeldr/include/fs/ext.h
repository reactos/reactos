/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

/* Magic value used to identify an ext filesystem.  */
#define    EXT_MAGIC        0xEF53
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS        12
/* Maximum length of a pathname.  */
#define EXT_PATH_MAX        4096
/* Maximum nesting of symlinks, used to prevent a loop.  */
#define    EXT_MAX_SYMLINKCNT    8

/* The good old revision and the default inode size.  */
#define EXT_GOOD_OLD_REVISION        0
#define EXT_DYNAMIC_REVISION        1
#define EXT_GOOD_OLD_INODE_SIZE    128

/* Filetype used in directory entry.  */
#define    FILETYPE_UNKNOWN    0
#define    FILETYPE_REG        1
#define    FILETYPE_DIRECTORY    2
#define    FILETYPE_SYMLINK    7

/* Filetype information as used in inodes.  */
#define FILETYPE_INO_MASK    0170000
#define FILETYPE_INO_REG    0100000
#define FILETYPE_INO_DIRECTORY    0040000
#define FILETYPE_INO_SYMLINK    0120000

/* The ext superblock.  */
struct ext_sblock
{
  ULONG total_inodes;
  ULONG total_blocks;
  ULONG reserved_blocks;
  ULONG free_blocks;
  ULONG free_inodes;
  ULONG first_data_block;
  ULONG log2_block_size;
  LONG log2_fragment_size;
  ULONG blocks_per_group;
  ULONG fragments_per_group;
  ULONG inodes_per_group;
  ULONG mtime;
  ULONG utime;
  USHORT mnt_count;
  USHORT max_mnt_count;
  USHORT magic;
  USHORT fs_state;
  USHORT error_handling;
  USHORT minor_revision_level;
  ULONG lastcheck;
  ULONG checkinterval;
  ULONG creator_os;
  ULONG revision_level;
  USHORT uid_reserved;
  USHORT gid_reserved;
  ULONG first_inode;
  USHORT inode_size;
  USHORT block_group_number;
  ULONG feature_compatibility;
  ULONG feature_incompat;
  ULONG feature_ro_compat;
  ULONG unique_id[4];
  char volume_name[16];
  char last_mounted_on[64];
  ULONG compression_info;
  ULONG padding[77];
};

/* The ext blockgroup.  */
struct ext_block_group
{
  ULONG block_id;
  ULONG inode_id;
  ULONG inode_table_id;
  USHORT free_blocks;
  USHORT free_inodes;
  USHORT used_dirs;
  USHORT pad;
  ULONG reserved[3];
};

/* The ext inode.  */
struct ext_inode
{
  USHORT mode;
  USHORT uid;
  ULONG size;
  ULONG atime;
  ULONG ctime;
  ULONG mtime;
  ULONG dtime;
  USHORT gid;
  USHORT nlinks;
  ULONG blockcnt;  /* Blocks of 512 bytes!! */
  ULONG flags;
  ULONG osd1;
  union
  {
    struct datablocks
    {
      ULONG dir_blocks[INDIRECT_BLOCKS];
      ULONG indir_block;
      ULONG double_indir_block;
      ULONG tripple_indir_block;
    } blocks;
    char symlink[60];
  };
  ULONG version;
  ULONG acl;
  ULONG dir_acl;
  ULONG fragment_addr;
  ULONG osd2[3];
};

/* The header of an ext directory entry.  */
#define EXT_NAME_LEN 255

struct ext_dirent
{
  ULONG inode;
  USHORT direntlen;
  UCHAR namelen;
  UCHAR filetype;
  CHAR name[EXT_NAME_LEN];
};

/*
 * End of code from grub/fs/ext2.c
 */

typedef struct ext_sblock        EXT_SUPER_BLOCK, *PEXT_SUPER_BLOCK;
typedef struct ext_inode        EXT_INODE, *PEXT_INODE;
typedef struct ext_block_group        EXT_GROUP_DESC, *PEXT_GROUP_DESC;
typedef struct ext_dirent        EXT_DIR_ENTRY, *PEXT_DIR_ENTRY;

/* Special inode numbers.  */
#define EXT_ROOT_INO        2

/* Feature set definitions.  */
#define EXT3_FEATURE_INCOMPAT_SUPP    0x0002

/* Log2 size of ext block in bytes.  */
#define LOG2_BLOCK_SIZE(sb)    (sb->log2_block_size + 10)

/* The size of an ext block in bytes.  */
#define EXT_BLOCK_SIZE(sb)    (((SIZE_T)1) << LOG2_BLOCK_SIZE(sb))

/* The revision level.  */
#define EXT_REVISION(sb)    (sb->revision_level)

/* The inode size.  */
#define EXT_INODE_SIZE(sb)    (EXT_REVISION(sb) == EXT_GOOD_OLD_REVISION \
                ? EXT_GOOD_OLD_INODE_SIZE \
                : sb->inode_size)

#define EXT_DESC_PER_BLOCK(s)    (EXT_BLOCK_SIZE(s) / sizeof(struct ext_block_group))

// EXT_INODE::mode values
#define EXT_S_IRWXO    0x0007    // Other mask
#define EXT_S_IXOTH    0x0001    // ---------x execute
#define EXT_S_IWOTH    0x0002    // --------w- write
#define EXT_S_IROTH    0x0004    // -------r-- read

#define EXT_S_IRWXG    0x0038    // Group mask
#define EXT_S_IXGRP    0x0008    // ------x--- execute
#define EXT_S_IWGRP    0x0010    // -----w---- write
#define EXT_S_IRGRP    0x0020    // ----r----- read

#define EXT_S_IRWXU    0x01C0    // User mask
#define EXT_S_IXUSR    0x0040    // ---x------ execute
#define EXT_S_IWUSR    0x0080    // --w------- write
#define EXT_S_IRUSR    0x0100    // -r-------- read

#define EXT_S_ISVTX    0x0200    // Sticky bit
#define EXT_S_ISGID    0x0400    // SGID
#define EXT_S_ISUID    0x0800    // SUID

#define EXT_S_IFMT        0xF000    // Format mask
#define EXT_S_IFIFO    0x1000    // FIFO buffer
#define EXT_S_IFCHR    0x2000    // Character device
#define EXT_S_IFDIR    0x4000    // Directory
#define EXT_S_IFBLK    0x6000    // Block device
#define EXT_S_IFREG    0x8000    // Regular file
#define EXT_S_IFLNK    0xA000    // Symbolic link
#define EXT_S_IFSOCK    0xC000    // Socket

#define FAST_SYMLINK_MAX_NAME_SIZE    60

typedef struct _EXT_VOLUME_INFO *PEXT_VOLUME_INFO;

typedef struct
{
    ULONGLONG   FileSize;       // File size
    ULONGLONG   FilePointer;    // File pointer
    ULONG*      FileBlockList;  // File block list
    EXT_INODE  Inode;          // File's inode
    PEXT_VOLUME_INFO   Volume;
} EXT_FILE_INFO, * PEXT_FILE_INFO;

const DEVVTBL* ExtMount(ULONG DeviceId);
