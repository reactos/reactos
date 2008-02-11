/*
 * PROJECT:          Mke2fs
 * FILE:             Mke2fs.h
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

#ifndef __MKE2FS__INCLUDE__
#define __MKE2FS__INCLUDE__


/* INCLUDES **************************************************************/


#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <fmifs/fmifs.h>

#include "string.h"
#include "ctype.h"

#include "types.h"
#include "ext2_fs.h"

#include "getopt.h"

/* DEFINITIONS ***********************************************************/

#define SECTOR_SIZE (Ext2Sys->DiskGeometry.BytesPerSector)

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;
#endif /* GUID_DEFINED */

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#ifndef uuid_t
#define uuid_t UUID
#endif
#endif

#ifndef bool
#define bool    BOOLEAN
#endif

#ifndef true
#define true    TRUE
#endif

#ifndef false
#define false   FALSE
#endif


#define EXT2_CHECK_MAGIC(struct, code) \
      if ((struct)->magic != (code)) return (code)

/*
 * ext2fs_scan flags
 */
#define EXT2_SF_CHK_BADBLOCKS   0x0001
#define EXT2_SF_BAD_INODE_BLK   0x0002
#define EXT2_SF_BAD_EXTRA_BYTES 0x0004
#define EXT2_SF_SKIP_MISSING_ITABLE 0x0008

/*
 * ext2fs_check_if_mounted flags
 */
#define EXT2_MF_MOUNTED     1
#define EXT2_MF_ISROOT      2
#define EXT2_MF_READONLY    4
#define EXT2_MF_SWAP        8

/*
 * Ext2/linux mode flags.  We define them here so that we don't need
 * to depend on the OS's sys/stat.h, since we may be compiling on a
 * non-Linux system.
 */

#define LINUX_S_IFMT  00170000
#define LINUX_S_IFSOCK 0140000
#define LINUX_S_IFLNK    0120000
#define LINUX_S_IFREG  0100000
#define LINUX_S_IFBLK  0060000
#define LINUX_S_IFDIR  0040000
#define LINUX_S_IFCHR  0020000
#define LINUX_S_IFIFO  0010000
#define LINUX_S_ISUID  0004000
#define LINUX_S_ISGID  0002000
#define LINUX_S_ISVTX  0001000

#define LINUX_S_IRWXU 00700
#define LINUX_S_IRUSR 00400
#define LINUX_S_IWUSR 00200
#define LINUX_S_IXUSR 00100

#define LINUX_S_IRWXG 00070
#define LINUX_S_IRGRP 00040
#define LINUX_S_IWGRP 00020
#define LINUX_S_IXGRP 00010

#define LINUX_S_IRWXO 00007
#define LINUX_S_IROTH 00004
#define LINUX_S_IWOTH 00002
#define LINUX_S_IXOTH 00001

#define LINUX_S_ISLNK(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFLNK)
#define LINUX_S_ISREG(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFREG)
#define LINUX_S_ISDIR(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFDIR)
#define LINUX_S_ISCHR(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFCHR)
#define LINUX_S_ISBLK(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFBLK)
#define LINUX_S_ISFIFO(m)   (((m) & LINUX_S_IFMT) == LINUX_S_IFIFO)
#define LINUX_S_ISSOCK(m)   (((m) & LINUX_S_IFMT) == LINUX_S_IFSOCK)


#define EXT2_FIRST_INODE(s) EXT2_FIRST_INO(s)

typedef struct _ext2fs_bitmap {
    __u32       start, end;
    __u32       real_end;
    char*       bitmap;
} EXT2_BITMAP, *PEXT2_BITMAP;

typedef EXT2_BITMAP EXT2_GENERIC_BITMAP, *PEXT2_GENERIC_BITMAP;
typedef EXT2_BITMAP EXT2_INODE_BITMAP, *PEXT2_INODE_BITMAP;
typedef EXT2_BITMAP EXT2_BLOCK_BITMAP, *PEXT2_BLOCK_BITMAP;

typedef struct ext2_acl_entry   EXT2_ACL_ENTRY, *PEXT2_ACL_ENTRY;
typedef struct ext2_acl_header  EXT2_ACL_HEADER, *PEXT2_ACL_HEADER;
typedef struct ext2_dir_entry   EXT2_DIR_ENTRY, *PEXT2_DIR_ENTRY;
typedef struct ext2_dir_entry_2 EXT2_DIR_ENTRY2, *PEXT2_DIR_ENTRY2;
typedef struct ext2_dx_countlimit   EXT2_DX_CL, *PEXT2_DX_CL;
typedef struct ext2_dx_entry    EXT2_DX_ENTRY, *PEXT2_DX_ENTRY;
typedef struct ext2_dx_root_info    EXT2_DX_RI, *PEXT2_DX_RI;
typedef struct ext2_inode   EXT2_INODE, *PEXT2_INODE;
typedef struct ext2_group_desc  EXT2_GROUP_DESC, *PEXT2_GROUP_DESC;
typedef struct ext2_super_block EXT2_SUPER_BLOCK, *PEXT2_SUPER_BLOCK;

/*
 * Badblocks list
 */
struct ext2_struct_badblocks_list {
    int     num;
    int     size;
    ULONG   *list;
    int     badblocks_flags;
};

typedef struct ext2_struct_badblocks_list EXT2_BADBLK_LIST, *PEXT2_BADBLK_LIST;

typedef struct _ext2_filesys
{
    int                 flags;
    int                 blocksize;
    int                 fragsize;
    ULONG               group_desc_count;
    unsigned long       desc_blocks;
    PEXT2_GROUP_DESC    group_desc;
    PEXT2_SUPER_BLOCK   ext2_sb;
    unsigned long       inode_blocks_per_group;
    PEXT2_INODE_BITMAP  inode_map;
    PEXT2_BLOCK_BITMAP  block_map;

    EXT2_BADBLK_LIST    badblocks;
/*
    ext2_dblist         dblist;
*/
    __u32               stride; /* for mke2fs */

    __u32               umask;

    /*
     * Reserved for future expansion
     */
    __u32               reserved[8];

    /*
     * Reserved for the use of the calling application.
     */
    void *              priv_data;

    HANDLE              MediaHandle;

    DISK_GEOMETRY       DiskGeometry;

    PARTITION_INFORMATION   PartInfo;

} EXT2_FILESYS, *PEXT2_FILESYS;

// Block Description List
typedef struct _EXT2_BDL {
    LONGLONG    Lba;
    ULONG       Offset;
    ULONG       Length;
} EXT2_BDL, *PEXT2_BDL;

/*
 * Where the master copy of the superblock is located, and how big
 * superblocks are supposed to be.  We define SUPERBLOCK_SIZE because
 * the size of the superblock structure is not necessarily trustworthy
 * (some versions have the padding set up so that the superblock is
 * 1032 bytes long).
 */
#define SUPERBLOCK_OFFSET   1024
#define SUPERBLOCK_SIZE     1024


bool create_bad_block_inode(PEXT2_FILESYS fs, PEXT2_BADBLK_LIST bb_list);


/*
 *  Bitmap.c
 */

#define ext2_mark_block_bitmap ext2_mark_bitmap
#define ext2_mark_inode_bitmap ext2_mark_bitmap
#define ext2_unmark_block_bitmap ext2_unmark_bitmap
#define ext2_unmark_inode_bitmap ext2_unmark_bitmap

bool ext2_set_bit(int nr, void * addr);
bool ext2_clear_bit(int nr, void * addr);
bool ext2_test_bit(int nr, void * addr);

bool ext2_mark_bitmap(PEXT2_BITMAP bitmap, ULONG bitno);
bool ext2_unmark_bitmap(PEXT2_BITMAP bitmap, ULONG bitno);

bool ext2_test_block_bitmap(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG block);

bool ext2_test_block_bitmap_range(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG block, int num);

bool ext2_test_inode_bitmap(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG inode);


bool ext2_allocate_block_bitmap(PEXT2_FILESYS pExt2Sys);
bool ext2_allocate_inode_bitmap(PEXT2_FILESYS pExt2Sys);
void ext2_free_inode_bitmap(PEXT2_FILESYS pExt2Sys);
void ext2_free_block_bitmap(PEXT2_FILESYS pExt2Sys);

bool ext2_write_block_bitmap (PEXT2_FILESYS fs);
bool ext2_write_inode_bitmap (PEXT2_FILESYS fs);

bool ext2_write_bitmaps(PEXT2_FILESYS fs);

//bool read_bitmaps(PEXT2_FILESYS fs, int do_inode, int do_block);
bool ext2_read_inode_bitmap (PEXT2_FILESYS fs);
bool ext2_read_block_bitmap(PEXT2_FILESYS fs);
bool ext2_read_bitmaps(PEXT2_FILESYS fs);


/*
 *  Disk.c
 */

NTSTATUS
Ext2OpenDevice( PEXT2_FILESYS    Ext2Sys,
                PUNICODE_STRING  DeviceName );

NTSTATUS
Ext2CloseDevice( PEXT2_FILESYS  Ext2Sys);

NTSTATUS 
Ext2ReadDisk( PEXT2_FILESYS  Ext2Sys,
              ULONGLONG      Offset,
              ULONG          Length,
              PVOID          Buffer     );

NTSTATUS
Ext2WriteDisk( PEXT2_FILESYS  Ext2Sys,
               ULONGLONG      Offset,
               ULONG          Length,
               PVOID          Buffer );

NTSTATUS
Ext2GetMediaInfo( PEXT2_FILESYS Ext2Sys );


NTSTATUS
Ext2LockVolume( PEXT2_FILESYS Ext2Sys );

NTSTATUS
Ext2UnLockVolume( PEXT2_FILESYS Ext2Sys );

NTSTATUS
Ext2DisMountVolume( PEXT2_FILESYS Ext2Sys );


/*
 *  Group.c
 */

bool ext2_allocate_group_desc(PEXT2_FILESYS pExt2Sys);
void ext2_free_group_desc(PEXT2_FILESYS pExt2Sys);
bool ext2_bg_has_super(PEXT2_SUPER_BLOCK pExt2Sb, int group_block);

/*
 *  Inode.c
 */

bool ext2_get_inode_lba(PEXT2_FILESYS pExt2Sys, ULONG no, LONGLONG *offset);
bool ext2_load_inode(PEXT2_FILESYS pExt2Sys, ULONG no, PEXT2_INODE pInode);
bool ext2_save_inode(PEXT2_FILESYS pExt2Sys, ULONG no, PEXT2_INODE pInode);
bool ext2_new_inode(PEXT2_FILESYS fs, ULONG dir, int mode,
                      PEXT2_INODE_BITMAP map, ULONG *ret);
bool ext2_expand_inode(PEXT2_FILESYS pExt2Sys, PEXT2_INODE, ULONG newBlk);

bool ext2_read_inode (PEXT2_FILESYS pExt2Sys,
            ULONG               ino,
            ULONG               offset,
            PVOID               Buffer,
            ULONG               size,
            PULONG              dwReturn );
bool ext2_write_inode (PEXT2_FILESYS pExt2Sys,
            ULONG               ino,
            ULONG               offset,
            PVOID               Buffer,
            ULONG               size,
            PULONG              dwReturn );

bool ext2_add_entry(PEXT2_FILESYS pExt2Sys, ULONG parent, ULONG inode, int filetype, char *name);
bool ext2_reserve_inodes(PEXT2_FILESYS fs);
/*
 *  Memory.c
 */

//
// Return the group # of an inode number
//
int ext2_group_of_ino(PEXT2_FILESYS fs, ULONG ino);

//
// Return the group # of a block
//
int ext2_group_of_blk(PEXT2_FILESYS fs, ULONG blk);

/*
 *  Badblock.c
 */


void ext2_inode_alloc_stats2(PEXT2_FILESYS fs, ULONG ino, int inuse, int isdir);
void ext2_inode_alloc_stats(PEXT2_FILESYS fs, ULONG ino, int inuse);
void ext2_block_alloc_stats(PEXT2_FILESYS fs, ULONG blk, int inuse);

bool ext2_allocate_tables(PEXT2_FILESYS pExt2Sys);
bool ext2_allocate_group_table(PEXT2_FILESYS fs, ULONG group,
                      PEXT2_BLOCK_BITMAP bmap);
bool ext2_get_free_blocks(PEXT2_FILESYS fs, ULONG start, ULONG finish,
                 int num, PEXT2_BLOCK_BITMAP map, ULONG *ret);
bool write_inode_tables(PEXT2_FILESYS fs);

bool ext2_new_block(PEXT2_FILESYS fs, ULONG goal,
               PEXT2_BLOCK_BITMAP map, ULONG *ret);
bool ext2_alloc_block(PEXT2_FILESYS fs, ULONG goal, ULONG *ret);
bool ext2_new_dir_block(PEXT2_FILESYS fs, ULONG dir_ino,
                   ULONG parent_ino, char **block);
bool ext2_write_block(PEXT2_FILESYS fs, ULONG block, void *inbuf);
bool ext2_read_block(PEXT2_FILESYS fs, ULONG block, void *inbuf);

/*
 *  Mke2fs.c
 */

bool parase_cmd(int argc, char *argv[], PEXT2_FILESYS pExt2Sys);

bool zero_blocks(PEXT2_FILESYS fs, ULONG blk, ULONG num,
                 ULONG *ret_blk, ULONG *ret_count);

ULONG
Ext2DataBlocks(PEXT2_FILESYS Ext2Sys, ULONG TotalBlocks);

ULONG
Ext2TotalBlocks(PEXT2_FILESYS Ext2Sys, ULONG DataBlocks);



/*
 *  Super.c
 */

void ext2_print_super(PEXT2_SUPER_BLOCK pExt2Sb);
bool ext2_initialize_sb(PEXT2_FILESYS pExt2Sys);


/*
 *  Super.c
 */

LONGLONG ext2_nt_time (ULONG i_time);
ULONG ext2_unix_time (LONGLONG n_time);

/*
 *  Uuid.c
 */

void uuid_generate(__u8 * uuid);

#endif //__MKE2FS__INCLUDE__
