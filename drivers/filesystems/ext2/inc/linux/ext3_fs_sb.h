/*
 *  linux/include/linux/ext3_fs_sb.h
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/include/linux/minix_fs_sb.h
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#ifndef _LINUX_EXT3_FS_SB
#define _LINUX_EXT3_FS_SB

#include <linux/types.h>
#include <linux/rbtree.h>

/*
 * third extended-fs super-block data in memory
 */
struct ext3_sb_info {

    unsigned long s_desc_size;      /* size of group desc */
    unsigned long s_gdb_count;	/* Number of group descriptor blocks */
    unsigned long s_desc_per_block;	/* Number of group descriptors per block */
    unsigned long s_inodes_per_group;/* Number of inodes in a group */
    unsigned long s_inodes_per_block;/* Number of inodes per block */
    unsigned long s_blocks_per_group;/* Number of blocks in a group */
    unsigned long s_groups_count;	/* Number of groups in the fs */
    unsigned long s_itb_per_group;	/* Number of inode table blocks per group */

    int s_addr_per_block_bits;
    int s_desc_per_block_bits;

    struct buffer_head **s_group_desc;

#if 0
    unsigned long s_frag_size;	/* Size of a fragment in bytes */
    unsigned long s_frags_per_block;/* Number of fragments per block */
    unsigned long s_frags_per_group;/* Number of fragments in a group */
    unsigned long s_inodes_per_group;/* Number of inodes in a group */
    unsigned long s_itb_per_group;	/* Number of inode table blocks per group */
    unsigned long s_desc_per_block;	/* Number of group descriptors per block */
    unsigned long s_overhead_last;  /* Last calculated overhead */
    unsigned long s_blocks_last;    /* Last seen block count */
#endif

    struct ext3_super_block * s_es;	/* Pointer to the super block in the buffer */

    __le32 s_first_ino;

    u32 s_hash_seed[4];
    int s_def_hash_version;
};

int ext3_release_dir (struct inode * inode, struct file * filp);

#endif	/* _LINUX_EXT3_FS_SB */
