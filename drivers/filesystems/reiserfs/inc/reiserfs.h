#ifndef __RFSD_REISER_FS_H__
#define __RFSD_REISER_FS_H__

#include <linux/types.h>

#ifdef __GCC__
#ifndef __REACTOS__
 #define __PACKED		__PACKED
#else
 #define __PACKED		__attribute__((packed))
#endif
#else
 #define __PACKED		
#endif

/***************************************************************************/
/*                             SUPER BLOCK                                 */
/***************************************************************************/

/*
 * Structure of super block on disk, a version of which in RAM is often accessed as REISERFS_SB(s)->s_rs
 * the version in RAM is part of a larger structure containing fields never written to disk.
 */
#define UNSET_HASH 0 // read_super will guess about, what hash names
                     // in directories were sorted with
#define TEA_HASH  1
#define YURA_HASH 2
#define R5_HASH   3
#define DEFAULT_HASH R5_HASH


struct journal_params {
    // Block number of the block containing the first journal node.
	__u32 jp_journal_1st_block;	      /* where does journal start from on its device */

	// Journal device number (?? for if the journal is on a seperate drive ??)
    __u32 jp_journal_dev;	      /* journal device st_rdev */

	// Original journal size.  (Needed when using partition on systems w/ different default journal sizes).
    __u32 jp_journal_size;	      /* size of the journal */
	
    __u32 jp_journal_trans_max;	      /* max number of blocks in a transaction. */
    __u32 jp_journal_magic; 	      /* random value made on fs creation (this was sb_journal_block_count) */
    __u32 jp_journal_max_batch;	      /* max number of blocks to batch into a trans */
    __u32 jp_journal_max_commit_age;  /* in seconds, how old can an async commit be */
    __u32 jp_journal_max_trans_age;   /* in seconds, how old can a transaction be */
};

/* this is the super from 3.5.X, where X >= 10 */
#ifndef __GCC__
 #pragma pack(push, 1)
#endif

struct reiserfs_super_block_v1
{
	// The number of blocks in the partition
    __u32 s_blocks_count;	   /* blocks count         */			//[mark] was _s_blocks_count

	// The number of free blocks in the partition
    __u32 s_free_blocks_count;           /* free blocks count    */  //[mark] was _s_free_blocks

	// Block number of the block containing the root node
    __u32 s_root_block;            /* root block number    */


    struct journal_params s_journal;

	// The size (in bytes) of a block
    __u16 s_blocksize;             /* block size */

    __u16 s_oid_maxsize;	   /* max size of object id array, see get_objectid() commentary  */
    __u16 s_oid_cursize;	   /* current size of object id array */
    __u16 s_umount_state;          /* this is set to 1 when filesystem was umounted, to 2 - when not */    
    char s_magic[10];              /* reiserfs magic string indicates that
				    * file system is reiserfs:
				    * "ReIsErFs" or "ReIsEr2Fs" or "ReIsEr3Fs" */
    
	// State of the partition: valid(1), error (2)
	__u16 s_fs_state;	           /* it is set to used by fsck to mark which phase of rebuilding is done */

    __u32 s_hash_function_code;    /* indicate, what hash function is being use
				    * to sort names in a directory*/
    __u16 s_tree_height;           /* height of disk tree */
    __u16 s_bmap_nr;               /* amount of bitmap blocks needed to address
				    * each block of file system */
    
	// The reiserfs version number
	__u16 s_version;               /* this field is only reliable on filesystem
				    * with non-standard journal */
    __u16 s_reserved_for_journal;  /* size in blocks of journal area on main
				    * device, we need to keep after
				    * making fs with non-standard journal */	
} __PACKED;
#ifndef __GCC__
 #pragma pack(pop)
#endif


#define SB_SIZE_V1 (sizeof(struct reiserfs_super_block_v1))

/* this is the on disk super block */
#ifndef __GCC__
 #pragma pack(push, 1)
#endif

struct reiserfs_super_block
{
    struct reiserfs_super_block_v1 s_v1;
    
	// Number of the current inode generation (a counter that is increased every time the tree gets re-balanced).
	__u32 s_inode_generation;

    __u32 s_flags;                  /* Right now used only by inode-attributes, if enabled */
    unsigned char s_uuid[16];       /* filesystem unique identifier */
    unsigned char s_label[16];      /* filesystem volume label */
    char s_unused[88] ;             /* zero filled by mkreiserfs and
				     * reiserfs_convert_objectid_map_v1()
				     * so any additions must be updated
				     * there as well. */
}  __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif

#define SB_SIZE (sizeof(struct reiserfs_super_block))

#define REISERFS_VERSION_1 0
#define REISERFS_VERSION_2 2

// ... [ommissions]


				/* used by gcc */
#define REISERFS_SUPER_MAGIC 0x52654973
				/* used by file system utilities that
                                   look at the superblock, etc. */
#define REISERFS_SUPER_MAGIC_STRING "ReIsErFs"
#define REISER2FS_SUPER_MAGIC_STRING "ReIsEr2Fs"
#define REISER2FS_JR_SUPER_MAGIC_STRING "ReIsEr3Fs"

/* ReiserFS leaves the first 64k unused, so that partition labels have
   enough space.  If someone wants to write a fancy bootloader that
   needs more than 64k, let us know, and this will be increased in size.
   This number must be larger than than the largest block size on any
   platform, or code will break.  -Hans */
#define REISERFS_DISK_OFFSET_IN_BYTES (64 * 1024)
#define REISERFS_FIRST_BLOCK unused_define
#define REISERFS_JOURNAL_OFFSET_IN_BYTES REISERFS_DISK_OFFSET_IN_BYTES

/* the spot for the super in versions 3.5 - 3.5.10 (inclusive) */
#define REISERFS_OLD_DISK_OFFSET_IN_BYTES (8 * 1024)



/***************************************************************************/
/*                             STAT DATA                                   */
/***************************************************************************/


#ifndef __GCC__
 #pragma pack(push, 1)
#endif

//
// old stat data is 32 bytes long. We are going to distinguish new one by
// different size
//
struct stat_data_v1
{
    __u16 sd_mode;	/* file type, permissions */
    __u16 sd_nlink;	/* number of hard links */
    __u16 sd_uid;		/* owner */
    __u16 sd_gid;		/* group */
    __u32 sd_size;	/* file size (in bytes) */
    __u32 sd_atime;	/* time of last access */
    __u32 sd_mtime;	/* time file was last modified  */
    __u32 sd_ctime;	/* time inode (stat data) was last changed (except changes to sd_atime and sd_mtime) */
    union {
	__u32 sd_rdev;
	__u32 sd_blocks;	/* number of blocks file uses */  //[mark]this is the one filled..
    } __PACKED u;
    __u32 sd_first_direct_byte; /* first byte of file which is stored
				   in a direct item: except that if it
				   equals 1 it is a symlink and if it
				   equals ~(__u32)0 there is no
				   direct item.  The existence of this
				   field really grates on me. Let's
				   replace it with a macro based on
				   sd_size and our tail suppression
				   policy.  Someday.  -Hans */
} __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif

/* inode flags stored in sd_attrs (nee sd_reserved) */

/* we want common flags to have the same values as in ext2,
   so chattr(1) will work without problems */
#define REISERFS_IMMUTABLE_FL EXT2_IMMUTABLE_FL
#define REISERFS_APPEND_FL    EXT2_APPEND_FL
#define REISERFS_SYNC_FL      EXT2_SYNC_FL
#define REISERFS_NOATIME_FL   EXT2_NOATIME_FL
#define REISERFS_NODUMP_FL    EXT2_NODUMP_FL
#define REISERFS_SECRM_FL     EXT2_SECRM_FL
#define REISERFS_UNRM_FL      EXT2_UNRM_FL
#define REISERFS_COMPR_FL     EXT2_COMPR_FL
#define REISERFS_NOTAIL_FL    EXT2_NOTAIL_FL

/* persistent flags that file inherits from the parent directory */
#define REISERFS_INHERIT_MASK ( REISERFS_IMMUTABLE_FL |	\
				REISERFS_SYNC_FL |	\
				REISERFS_NOATIME_FL |	\
				REISERFS_NODUMP_FL |	\
				REISERFS_SECRM_FL |	\
				REISERFS_COMPR_FL |	\
				REISERFS_NOTAIL_FL )


#ifndef __GCC__
 #pragma pack(push, 1)
#endif

/* Stat Data on disk (reiserfs version of UFS disk inode minus the
   address blocks) */
struct stat_data {
    __u16  i_mode;	/* file type, permissions */  // The low 9 bits (3 octals) contain world/group/user permissions.  The next 3 bits (from lower to higher) are the sticky bit, the set GID bit, and the set UID bit.  The high 4 bits are the file type (as defined in stat.h: socket, symlink, regular, block dev, directory, char device, fifo)
    __u16 sd_attrs;     /* persistent inode flags */
    __u32  i_links_count;	/* number of hard links */			//[mark] was sd_nlink
    __u64  i_size;	/* file size */
    __u32  i_uid;		/* owner */
    __u32  i_gid;		/* group */
    __u32  i_atime;	/* time of last access */
    __u32  i_mtime;	/* time file was last modified  */
    __u32  i_ctime;	/* time inode (stat data) was last changed (except changes to sd_atime and sd_mtime) */
    __u32 sd_blocks;
    union {
	__u32 sd_rdev;
	__u32  i_generation;
      //__u32 sd_first_direct_byte; 
      /* first byte of file which is stored in a
				       direct item: except that if it equals 1
				       it is a symlink and if it equals
				       ~(__u32)0 there is no direct item.  The
				       existence of this field really grates
				       on me. Let's replace it with a macro
				       based on sd_size and our tail
				       suppression policy? */
  } __PACKED u;
} __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif

//
// this is 44 bytes long
//
#define SD_SIZE (sizeof(struct stat_data))
#define SD_V2_SIZE              SD_SIZE




/*
 * values for s_umount_state field
 */
#define REISERFS_VALID_FS    1
#define REISERFS_ERROR_FS    2

//
// there are 5 item types currently
//

#define RFSD_KEY_TYPE_v1_STAT_DATA		0
#define RFSD_KEY_TYPE_v1_INDIRECT		0xFFFFFFFe
#define RFSD_KEY_TYPE_v1_DIRECT			0xFFFFFFFF
#define RFSD_KEY_TYPE_v1_DIRENTRY		500

#define RFSD_KEY_TYPE_v2_STAT_DATA		0
#define RFSD_KEY_TYPE_v2_INDIRECT		1
#define RFSD_KEY_TYPE_v2_DIRECT			2
#define RFSD_KEY_TYPE_v2_DIRENTRY		3 





/***************************************************************************/
/*                       KEY & ITEM HEAD                                   */
/***************************************************************************/

typedef struct reiserfs_cpu_key
{
    __u32 k_dir_id;
    __u32 k_objectid;
    __u64 k_offset;
    __u32 k_type;
} no_c4091;

//
// directories use this key as well as old files
//

#ifndef __GCC__
 #pragma pack(push, 1)
#endif

struct offset_v1 {
    __u32 k_offset;
    __u32 k_uniqueness;
} __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif


#ifndef __GCC__
 #pragma pack(push, 1)
#endif

struct offset_v2 {
#ifdef __LITTLE_ENDIAN
	    /* little endian version */
	    __u64 k_offset:60;
	    __u64 k_type: 4;
#else
	    /* big endian version */
	    __u64 k_type: 4;
	    __u64 k_offset:60;
#endif
} __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif


// ...

#ifndef __GCC__
 #pragma pack(push, 1)
#endif

/* Key of an item determines its location in the S+tree, and
   is composed of 4 components */
struct reiserfs_key {
    __u32 k_dir_id;    /* packing locality: by default parent directory object id */
    __u32 k_objectid;  /* object identifier */
    union {
	struct offset_v1 k_offset_v1;
	struct offset_v2 k_offset_v2;
    } u;
} __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif


/// ...

#ifndef __GCC__
 #pragma pack(push, 1)
#endif

/*  Everything in the filesystem is stored as a set of items.  The
    item head contains the key of the item, its free space (for
    indirect items) and specifies the location of the item itself
    within the block.  */
struct item_head
{
	/* Everything in the tree is found by searching for it based on
	 * its key.*/
	struct reiserfs_key ih_key;
	union {
		/* The free space in the last unformatted node of an
		   indirect item if this is an indirect item.  This
		   equals 0xFFFF iff this is a direct item or stat data
		   item. Note that the key, not this field, is used to
		   determine the item type, and thus which field this
		   union contains. */
		__u16 ih_free_space_reserved; 
		/* Iff this is a directory item, this field equals the
		   number of directory entries in the directory item. */
		__u16 ih_entry_count; 
	} u;
	__u16 ih_item_len;           /* total size of the item body */
	__u16 ih_item_location;      /* an offset to the item body within the block */
	__u16 ih_version;	     /* 0 for all old items, 2 for new
					ones. Highest bit is set by fsck
					temporary, cleaned after all
					done */
} __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif

/// ...

/* object identifier for root dir */
#define REISERFS_ROOT_OBJECTID 2
#define REISERFS_ROOT_PARENT_OBJECTID 1

/// ...


/* 
 * Picture represents a leaf of the S+tree
 *  ______________________________________________________
 * |      |  Array of     |                   |           |
 * |Block |  Object-Item  |      F r e e      |  Objects- |
 * | head |  Headers      |     S p a c e     |   Items   |
 * |______|_______________|___________________|___________|
 */

/* Header of a disk block.  More precisely, header of a formatted leaf
   or internal node, and not the header of an unformatted node. */

#ifndef __GCC__
 #pragma pack(push, 1)
#endif

struct block_head {       
  __u16 blk_level;        /* Level of a block in the tree. */
  __u16 blk_nr_item;      /* Number of keys/items in a block. */
  __u16 blk_free_space;   /* Block free space in bytes. */
  __u16 blk_reserved;
				/* dump this in v4/planA */
  struct reiserfs_key  blk_right_delim_key; /* kept only for compatibility */
};

#ifndef __GCC__
 #pragma pack(pop)
#endif







/***************************************************************************/
/*                      DIRECTORY STRUCTURE                                */
/***************************************************************************/
/* 
   Picture represents the structure of directory items
   ________________________________________________
   |  Array of     |   |     |        |       |   |
   | directory     |N-1| N-2 | ....   |   1st |0th|
   | entry headers |   |     |        |       |   |
   |_______________|___|_____|________|_______|___|
                    <----   directory entries         ------>

 First directory item has k_offset component 1. We store "." and ".."
 in one item, always, we never split "." and ".." into differing
 items.  This makes, among other things, the code for removing
 directories simpler. */

// ...

/*
   Q: How to get key of object pointed to by entry from entry?  

   A: Each directory entry has its header. This header has deh_dir_id and deh_objectid fields, those are key
      of object, entry points to */

/* NOT IMPLEMENTED:   
   Directory will someday contain stat data of object */


#ifndef __GCC__
 #pragma pack(push, 1)
#endif

struct reiserfs_de_head
{
  __u32 deh_offset;		/* third component of the directory entry key */
  __u32 deh_dir_id;		/* objectid of the parent directory of the object, that is referenced by directory entry */
  __u32 deh_objectid;		/* objectid of the object, that is referenced by directory entry */
  __u16 deh_location;		/* offset of name in the whole item */
  __u16 deh_state;		/* whether 1) entry contains stat data (for future), and 2) whether
					   entry is hidden (unlinked) */
} __PACKED;

#ifndef __GCC__
 #pragma pack(pop)
#endif






/*
 * Picture represents an internal node of the reiserfs tree
 *  ______________________________________________________
 * |      |  Array of     |  Array of         |  Free     |
 * |block |    keys       |  pointers         | space     |
 * | head |      N        |      N+1          |           |
 * |______|_______________|___________________|___________|
 */

/***************************************************************************/
/*                      DISK CHILD                                         */
/***************************************************************************/
/* Disk child pointer: The pointer from an internal node of the tree
   to a node that is on disk. */

#ifndef __GCC__
 #pragma pack(push, 1)
#endif

struct disk_child {
  __u32       dc_block_number;              /* Disk child's block number. */
  __u16       dc_size;						/* Disk child's used space.   */
  __u16       dc_reserved;
};

#ifndef __GCC__
 #pragma pack(pop)
#endif




#endif  // header
