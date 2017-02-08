/*
 * PROJECT:          Mke2fs
 * FILE:             Super.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"
#include <debug.h>

/* DEFINITIONS ***********************************************************/

extern int inode_ratio;


/* FUNCTIONS *************************************************************/

void ext2_print_super(PEXT2_SUPER_BLOCK pExt2Sb)
{
    int i;

    DPRINT("\nExt2 Super Block Details ...\n\n");
    DPRINT("     Inode Count: %lu\n", pExt2Sb->s_inodes_count);
    DPRINT("     Block Count: %lu\n", pExt2Sb->s_blocks_count);
    DPRINT("     Reserved Block Count: %lu\n", pExt2Sb->s_r_blocks_count);
    DPRINT("     Free Blocks: %lu\n", pExt2Sb->s_free_blocks_count);
    DPRINT("     Free Inodes: %lu\n", pExt2Sb->s_free_inodes_count);
    DPRINT("     First Data Block: %lu\n", pExt2Sb->s_first_data_block);
    DPRINT("     Log Block Size: %lu\n", pExt2Sb->s_log_block_size);
    DPRINT("     Log Frag Size: %ld\n", pExt2Sb->s_log_frag_size);
    DPRINT("     Blocks per Group: %lu\n", pExt2Sb->s_blocks_per_group);
    DPRINT("     Fragments per Group: %lu\n", pExt2Sb->s_frags_per_group);
    DPRINT("     Inodes per Group: %lu\n", pExt2Sb->s_inodes_per_group);
//    DPRINT("     Mount Time: %s", ctime((time_t *) & (pExt2Sb->s_mtime)));
//    DPRINT("     Write Time: %s", ctime((time_t *) & (pExt2Sb->s_wtime)));
    DPRINT("     Mount Count: %u\n", pExt2Sb->s_mnt_count);
    DPRINT("     Max Mount Count: %d\n", pExt2Sb->s_max_mnt_count);
    DPRINT("     Magic Number: %X  (%s)\n", pExt2Sb->s_magic,
        pExt2Sb->s_magic == EXT2_SUPER_MAGIC ? "OK" : "BAD");
    DPRINT("     File System State: %X\n", pExt2Sb->s_state);
    DPRINT("     Error Behaviour: %X\n", pExt2Sb->s_errors);
    DPRINT("     Minor rev: %u\n", pExt2Sb->s_minor_rev_level);
//    DPRINT("     Last Check: %s", ctime((time_t *) & (pExt2Sb->s_lastcheck)));
    DPRINT("     Check Interval: %lu\n", pExt2Sb->s_checkinterval);
    DPRINT("     Creator OS: %lu\n", pExt2Sb->s_creator_os);
    DPRINT("     Revision Level: %lu\n", pExt2Sb->s_rev_level);
    DPRINT("     Reserved Block Default UID: %u\n", pExt2Sb->s_def_resuid);
    DPRINT("     Reserved Block Default GID: %u\n", pExt2Sb->s_def_resgid);
    DPRINT("     uuid = ");
    for (i=0; i < 16; i++)
        DbgPrint("%x ", pExt2Sb->s_uuid[i]);
    DbgPrint("\n");

    DPRINT("     volume label name: ");
    for (i=0; i < 16; i++)
    {
        if (pExt2Sb->s_volume_name[i] == 0)
            break;
        DbgPrint("%c", pExt2Sb->s_volume_name[i]);
    }
    DbgPrint("\n");

    DPRINT("\n\n");
}

#define set_field(field, default) if (!pExt2Sb->field) pExt2Sb->field = (default);

/*
 *  Initialize super block ...
 */

bool ext2_initialize_sb(PEXT2_FILESYS Ext2Sys)
{
    int frags_per_block = 0;
    ULONG overhead      = 0;
    ULONG rem           = 0;
    ULONG   i = 0;
    ULONG   group_block = 0;
    ULONG   numblocks = 0;
    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;
    LARGE_INTEGER   SysTime;

    NtQuerySystemTime(&SysTime);

    Ext2Sys->blocksize = EXT2_BLOCK_SIZE(pExt2Sb);
    Ext2Sys->fragsize = EXT2_FRAG_SIZE(pExt2Sb);
    frags_per_block = Ext2Sys->blocksize / Ext2Sys->fragsize;

    pExt2Sb->s_magic = EXT2_SUPER_MAGIC;
    pExt2Sb->s_state = EXT2_VALID_FS;

    pExt2Sb->s_first_data_block =  (pExt2Sb->s_log_block_size) ? 0 : 1;
    pExt2Sb->s_max_mnt_count = EXT2_DFL_MAX_MNT_COUNT;

    pExt2Sb->s_errors = EXT2_ERRORS_DEFAULT;

    pExt2Sb->s_checkinterval = EXT2_DFL_CHECKINTERVAL;

    if (!pExt2Sb->s_rev_level)
        pExt2Sb->s_rev_level = EXT2_GOOD_OLD_REV;

    if (pExt2Sb->s_rev_level >= EXT2_DYNAMIC_REV)
    {
        set_field(s_first_ino, EXT2_GOOD_OLD_FIRST_INO);
        set_field(s_inode_size, EXT2_GOOD_OLD_INODE_SIZE);
    }

    RtlTimeToSecondsSince1970(&SysTime, &pExt2Sb->s_wtime);
    pExt2Sb->s_lastcheck = pExt2Sb->s_mtime = pExt2Sb->s_wtime;

    if (!pExt2Sb->s_blocks_per_group)
        pExt2Sb->s_blocks_per_group = Ext2Sys->blocksize * 8;

    pExt2Sb->s_frags_per_group = pExt2Sb->s_blocks_per_group *  frags_per_block;
    pExt2Sb->s_creator_os = EXT2_OS_WINNT;

    if (pExt2Sb->s_r_blocks_count >= pExt2Sb->s_blocks_count)
    {
        goto cleanup;
    }

    /*
     * If we're creating an external journal device, we don't need
     * to bother with the rest.
     */
    if (pExt2Sb->s_feature_incompat &
        EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)
    {
        Ext2Sys->group_desc_count = 0;
        // ext2fs_mark_super_dirty(fs);
        return true;
    }

retry:

    Ext2Sys->group_desc_count = (pExt2Sb->s_blocks_count - pExt2Sb->s_first_data_block
        + EXT2_BLOCKS_PER_GROUP(pExt2Sb) - 1) / EXT2_BLOCKS_PER_GROUP(pExt2Sb);

    if (Ext2Sys->group_desc_count == 0)
        return false;

    Ext2Sys->desc_blocks = (Ext2Sys->group_desc_count +  EXT2_DESC_PER_BLOCK(pExt2Sb)
        - 1) / EXT2_DESC_PER_BLOCK(pExt2Sb);

    if (!pExt2Sb->s_inodes_count)
        pExt2Sb->s_inodes_count = pExt2Sb->s_blocks_count / ( inode_ratio /Ext2Sys->blocksize);

    /*
     * Make sure we have at least EXT2_FIRST_INO + 1 inodes, so
     * that we have enough inodes for the filesystem(!)
     */
    if (pExt2Sb->s_inodes_count < EXT2_FIRST_INODE(pExt2Sb)+1)
        pExt2Sb->s_inodes_count = EXT2_FIRST_INODE(pExt2Sb)+1;

    /*
     * There should be at least as many inodes as the user
     * requested.  Figure out how many inodes per group that
     * should be.  But make sure that we don't allocate more than
     * one bitmap's worth of inodes
     */
    pExt2Sb->s_inodes_per_group = (pExt2Sb->s_inodes_count + Ext2Sys->group_desc_count - 1)
        /Ext2Sys->group_desc_count;

    if (pExt2Sb->s_inodes_per_group > (ULONG)(Ext2Sys->blocksize*8))
        pExt2Sb->s_inodes_per_group = Ext2Sys->blocksize*8;

    /*
     * Make sure the number of inodes per group completely fills
     * the inode table blocks in the descriptor.  If not, add some
     * additional inodes/group.  Waste not, want not...
     */
    Ext2Sys->inode_blocks_per_group = (((pExt2Sb->s_inodes_per_group * EXT2_INODE_SIZE(pExt2Sb))
        + EXT2_BLOCK_SIZE(pExt2Sb) - 1) / EXT2_BLOCK_SIZE(pExt2Sb));

    pExt2Sb->s_inodes_per_group = ((Ext2Sys->inode_blocks_per_group * EXT2_BLOCK_SIZE(pExt2Sb))
        / EXT2_INODE_SIZE(pExt2Sb));

    /*
     * Finally, make sure the number of inodes per group is a
     * multiple of 8.  This is needed to simplify the bitmap
     * splicing code.
     */
    pExt2Sb->s_inodes_per_group &= ~7;
    Ext2Sys->inode_blocks_per_group = (((pExt2Sb->s_inodes_per_group * EXT2_INODE_SIZE(pExt2Sb))
        + EXT2_BLOCK_SIZE(pExt2Sb) - 1) / EXT2_BLOCK_SIZE(pExt2Sb));

    /*
     * adjust inode count to reflect the adjusted inodes_per_group
     */
    pExt2Sb->s_inodes_count = pExt2Sb->s_inodes_per_group * Ext2Sys->group_desc_count;
    pExt2Sb->s_free_inodes_count = pExt2Sb->s_inodes_count;

    /*
     * Overhead is the number of bookkeeping blocks per group.  It
     * includes the superblock backup, the group descriptor
     * backups, the inode bitmap, the block bitmap, and the inode
     * table.
     *
     * XXX Not all block groups need the descriptor blocks, but
     * being clever is tricky...
     */
    overhead = (3 + Ext2Sys->desc_blocks + Ext2Sys->inode_blocks_per_group);

    /*
     * See if the last group is big enough to support the
     * necessary data structures.  If not, we need to get rid of
     * it.
     */
    rem = ((pExt2Sb->s_blocks_count - pExt2Sb->s_first_data_block) %
             pExt2Sb->s_blocks_per_group);

    if ((Ext2Sys->group_desc_count == 1) && rem && (rem < overhead))
        return false;

    if (rem && (rem < overhead+50))
    {
        pExt2Sb->s_blocks_count -= rem;
        goto retry;
    }

    /*
     * At this point we know how big the filesystem will be.  So we can do
     * any and all allocations that depend on the block count.
     */

    // Allocate block bitmap
    if(!ext2_allocate_block_bitmap(Ext2Sys))
    {
        goto cleanup;
    }

    // Allocate inode bitmap
    if(!ext2_allocate_inode_bitmap(Ext2Sys))
    {
        goto cleanup;
    }

    // Allocate gourp desc
    if(!ext2_allocate_group_desc(Ext2Sys))
    {
        goto cleanup;
    }

    /*
     * Reserve the superblock and group descriptors for each
     * group, and fill in the correct group statistics for group.
     * Note that although the block bitmap, inode bitmap, and
     * inode table have not been allocated (and in fact won't be
     * by this routine), they are accounted for nevertheless.
     */
    group_block = pExt2Sb->s_first_data_block;
    numblocks = 0;

    pExt2Sb->s_free_blocks_count = 0;

    for (i = 0; i < Ext2Sys->group_desc_count; i++)
    {
        if (i == Ext2Sys->group_desc_count-1)
        {
            numblocks = (pExt2Sb->s_blocks_count - pExt2Sb->s_first_data_block)
                % pExt2Sb->s_blocks_per_group;

            if (!numblocks)
                numblocks = pExt2Sb->s_blocks_per_group;
        }
        else
        {
            numblocks = pExt2Sb->s_blocks_per_group;
        }

        if (ext2_bg_has_super(pExt2Sb, i))
        {
            ULONG j;

            for (j=0; j < Ext2Sys->desc_blocks+1; j++)
                ext2_mark_bitmap(Ext2Sys->block_map, group_block + j);

            numblocks -= 1 + Ext2Sys->desc_blocks;
        }

        numblocks -= 2 + Ext2Sys->inode_blocks_per_group;

        pExt2Sb->s_free_blocks_count += numblocks;
        Ext2Sys->group_desc[i].bg_free_blocks_count = (__u16)numblocks;
        Ext2Sys->group_desc[i].bg_free_inodes_count = (__u16)pExt2Sb->s_inodes_per_group;
        Ext2Sys->group_desc[i].bg_used_dirs_count = 0;

        group_block += pExt2Sb->s_blocks_per_group;
    }

    return true;

cleanup:

    ext2_free_group_desc(Ext2Sys);
    ext2_free_block_bitmap(Ext2Sys);
    ext2_free_inode_bitmap(Ext2Sys);

    return false;
}
