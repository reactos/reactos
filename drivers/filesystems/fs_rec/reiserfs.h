/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/btrfs.h
 * PURPOSE:          BTRFS Header File
 * PROGRAMMER:       Peter Hater
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

#include <pshpack1.h>
struct journal_params {
    // Block number of the block containing the first journal node.
    UINT32 jp_journal_1st_block;	      /* where does journal start from on its device */

    // Journal device number (?? for if the journal is on a separate drive ??)
    UINT32 jp_journal_dev;	      /* journal device st_rdev */

    // Original journal size.  (Needed when using partition on systems w/ different default journal sizes).
    UINT32 jp_journal_size;	      /* size of the journal */

    UINT32 jp_journal_trans_max;	      /* max number of blocks in a transaction. */
    UINT32 jp_journal_magic; 	      /* random value made on fs creation (this was sb_journal_block_count) */
    UINT32 jp_journal_max_batch;	      /* max number of blocks to batch into a trans */
    UINT32 jp_journal_max_commit_age;  /* in seconds, how old can an async commit be */
    UINT32 jp_journal_max_trans_age;   /* in seconds, how old can a transaction be */
};

typedef struct _RFSD_SUPER_BLOCK
{
    // The number of blocks in the partition
    UINT32 s_blocks_count;	   /* blocks count         */			//[mark] was _s_blocks_count

    // The number of free blocks in the partition
    UINT32 s_free_blocks_count;           /* free blocks count    */  //[mark] was _s_free_blocks

    // Block number of the block containing the root node
    UINT32 s_root_block;            /* root block number    */

    struct journal_params s_journal;

    // The size (in bytes) of a block
    UINT16 s_blocksize;             /* block size */

    UINT16 s_oid_maxsize;	   /* max size of object id array, see get_objectid() commentary  */
    UINT16 s_oid_cursize;	   /* current size of object id array */
    UINT16 s_umount_state;          /* this is set to 1 when filesystem was umounted, to 2 - when not */
    char s_magic[10];              /* reiserfs magic string indicates that
                                    * file system is reiserfs:
                                    * "ReIsErFs" or "ReIsEr2Fs" or "ReIsEr3Fs" */

    // State of the partition: valid(1), error (2)
    UINT16 s_fs_state;	           /* it is set to used by fsck to mark which phase of rebuilding is done */

    UINT32 s_hash_function_code;    /* indicate, what hash function is being use
                                     * to sort names in a directory*/
    UINT16 s_tree_height;           /* height of disk tree */
    UINT16 s_bmap_nr;               /* amount of bitmap blocks needed to address
                                     * each block of file system */

    // The reiserfs version number
    UINT16 s_version;               /* this field is only reliable on filesystem
                                     * with non-standard journal */
    UINT16 s_reserved_for_journal;  /* size in blocks of journal area on main
                                     * device, we need to keep after
                                     * making fs with non-standard journal */
} RFSD_SUPER_BLOCK, *PRFSD_SUPER_BLOCK;
#include <poppack.h>

C_ASSERT(FIELD_OFFSET(RFSD_SUPER_BLOCK, s_blocksize) == 44);
C_ASSERT(FIELD_OFFSET(RFSD_SUPER_BLOCK, s_magic) == 52);

#define REISERFS_DISK_OFFSET_IN_BYTES (64 * 1024)
#define REISERFS_SUPER_MAGIC_STRING "ReIsErFs"
#define REISER2FS_SUPER_MAGIC_STRING "ReIsEr2Fs"
#define REISER2FS_JR_SUPER_MAGIC_STRING "ReIsEr3Fs"
#define MAGIC_KEY_LENGTH 9
