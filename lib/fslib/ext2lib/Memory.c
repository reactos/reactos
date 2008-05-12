/*
 * PROJECT:          Mke2fs
 * FILE:             Memory.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"
#include <debug.h>

/* DEFINITIONS ***********************************************************/

extern char *device_name;

/* FUNCTIONS *************************************************************/


/*
 * Return the group # of an inode number
 */
int ext2_group_of_ino(PEXT2_FILESYS fs, ULONG ino)
{
    return (ino - 1) / fs->ext2_sb->s_inodes_per_group;
}

/*
 * Return the group # of a block
 */
int ext2_group_of_blk(PEXT2_FILESYS fs, ULONG blk)
{
    return (blk - fs->ext2_sb->s_first_data_block) /
        fs->ext2_sb->s_blocks_per_group;
}

void ext2_inode_alloc_stats2(PEXT2_FILESYS fs, ULONG ino,
                   int inuse, int isdir)
{
    int group = ext2_group_of_ino(fs, ino);

    if (inuse > 0)
        ext2_mark_inode_bitmap(fs->inode_map, ino);
    else
        ext2_unmark_inode_bitmap(fs->inode_map, ino);

    fs->group_desc[group].bg_free_inodes_count -= inuse;

    if (isdir)
        fs->group_desc[group].bg_used_dirs_count += inuse;

    fs->ext2_sb->s_free_inodes_count -= inuse;
}


void ext2_inode_alloc_stats(PEXT2_FILESYS fs, ULONG ino, int inuse)
{
    ext2_inode_alloc_stats2(fs, ino, inuse, 0);
}

void ext2_block_alloc_stats(PEXT2_FILESYS fs, ULONG blk, int inuse)
{
    int group = ext2_group_of_blk(fs, blk);

    if (inuse > 0)
        ext2_mark_block_bitmap(fs->block_map, blk);
    else
        ext2_unmark_block_bitmap(fs->block_map, blk);

    fs->group_desc[group].bg_free_blocks_count -= inuse;
    fs->ext2_sb->s_free_blocks_count -= inuse;
}


bool ext2_allocate_tables(PEXT2_FILESYS Ext2Sys)
{
    bool    retval;
    ULONG   i;

    for (i = 0; i < Ext2Sys->group_desc_count; i++)
    {
        retval = ext2_allocate_group_table(Ext2Sys, i, Ext2Sys->block_map);

        if (!retval)
            return retval;
    }

    return true;
}


bool ext2_allocate_group_table(PEXT2_FILESYS fs, ULONG group,
                      PEXT2_BLOCK_BITMAP bmap)
{
    bool    retval;
    ULONG   group_blk, start_blk, last_blk, new_blk, blk;
    int     j;

    group_blk = fs->ext2_sb->s_first_data_block +
        (group * fs->ext2_sb->s_blocks_per_group);
    
    last_blk = group_blk + fs->ext2_sb->s_blocks_per_group;
    if (last_blk >= fs->ext2_sb->s_blocks_count)
        last_blk = fs->ext2_sb->s_blocks_count - 1;

    start_blk = group_blk + 3 + fs->desc_blocks;
    if (start_blk > last_blk)
        start_blk = group_blk;

    if (!bmap)
        bmap = fs->block_map;
    
    /*
     * Allocate the inode table
     */
    if (!fs->group_desc[group].bg_inode_table)
    {
        retval = ext2_get_free_blocks(fs, start_blk, last_blk,
                        fs->inode_blocks_per_group,
                        bmap, &new_blk);
        if (!retval)
            return retval;

        for (j=0, blk = new_blk;
             j < fs->inode_blocks_per_group;
             j++, blk++)
            ext2_mark_block_bitmap(bmap, blk);

        fs->group_desc[group].bg_inode_table = new_blk;
    }

    /*
     * Allocate the block and inode bitmaps, if necessary
     */
    if (fs->stride)
    {
        start_blk += fs->inode_blocks_per_group;
        start_blk += ((fs->stride * group) %
                  (last_blk - start_blk));
        if (start_blk > last_blk)
            /* should never happen */
            start_blk = group_blk;
    }
    else
    {
        start_blk = group_blk;
    }

    if (!fs->group_desc[group].bg_block_bitmap)
    {
        retval = ext2_get_free_blocks(fs, start_blk, last_blk,
                        1, bmap, &new_blk);

        if (!retval) 
            retval = ext2_get_free_blocks(fs, group_blk,
                    last_blk, 1, bmap, &new_blk);

        if (!retval)
            return retval;

        ext2_mark_block_bitmap(bmap, new_blk);
        fs->group_desc[group].bg_block_bitmap = new_blk;
    }

    if (!fs->group_desc[group].bg_inode_bitmap)
    {
        retval = ext2_get_free_blocks(fs, start_blk, last_blk,
                        1, bmap, &new_blk);
        if (!retval) 
            retval = ext2_get_free_blocks(fs, group_blk,
                    last_blk, 1, bmap, &new_blk);
        if (!retval)
            return retval;

        ext2_mark_block_bitmap(bmap, new_blk);
        fs->group_desc[group].bg_inode_bitmap = new_blk;
    }

    return true;
}


bool ext2_get_free_blocks(PEXT2_FILESYS fs, ULONG start, ULONG finish,
                 int num, PEXT2_BLOCK_BITMAP map, ULONG *ret)
{
    ULONG   b = start;

    if (!map)
        map = fs->block_map;

    if (!map)
        return false;

    if (!b)
        b = fs->ext2_sb->s_first_data_block;

    if (!finish)
        finish = start;

    if (!num)
        num = 1;

    do
    {
        if (b+num-1 > fs->ext2_sb->s_blocks_count)
            b = fs->ext2_sb->s_first_data_block;

        if (ext2_test_block_bitmap_range(map, b, num))
        {
            *ret = b;
            return true;
        }

        b++;

    } while (b != finish);

    return false;
}


bool write_inode_tables(PEXT2_FILESYS fs)
{
    bool    retval;
    ULONG   blk, num;
    int     i;

    for (i = 0; (ULONG)i < fs->group_desc_count; i++)
    {
        blk = fs->group_desc[i].bg_inode_table;
        num = fs->inode_blocks_per_group;

        retval = zero_blocks(fs, blk, num, &blk, &num);
        if (!retval)
        {
            DPRINT1("\nMke2fs: Could not write %lu blocks "
                "in inode table starting at %lu.\n",
                num, blk);

            zero_blocks(0, 0, 0, 0, 0);
            return false;
        }
    }

    zero_blocks(0, 0, 0, 0, 0);

    return true;
}


/*
 * Stupid algorithm --- we now just search forward starting from the
 * goal.  Should put in a smarter one someday....
 */
bool ext2_new_block(PEXT2_FILESYS fs, ULONG goal,
               PEXT2_BLOCK_BITMAP map, ULONG *ret)
{
    ULONG   i;

    if (!map)
        map = fs->block_map;

    if (!map)
        return false;

    if (!goal || (goal >= fs->ext2_sb->s_blocks_count))
        goal = fs->ext2_sb->s_first_data_block;

    i = goal;

    do
    {
        if (!ext2_test_block_bitmap(map, i))
        {
            *ret = i;
            return true;
        }

        i++;

        if (i >= fs->ext2_sb->s_blocks_count)
            i = fs->ext2_sb->s_first_data_block;

    } while (i != goal);

    return false;
}


/*
 * This function zeros out the allocated block, and updates all of the
 * appropriate filesystem records.
 */
bool ext2_alloc_block(PEXT2_FILESYS fs, ULONG goal, ULONG *ret)
{
    bool        retval;
    ULONG       block;
    char        *buf = NULL;

    buf = (char *)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, fs->blocksize);
    if (!buf)
        return false;

    if (!fs->block_map)
    {
        retval = ext2_read_block_bitmap(fs);
        if (!retval)
            goto fail;
    }

    retval = ext2_new_block(fs, goal, 0, &block);

    if (!retval)
        goto fail;

    retval = NT_SUCCESS(Ext2WriteDisk(
                fs,
                ((LONGLONG)block * fs->blocksize),
                fs->blocksize, (unsigned char *)buf));

    if (!retval)
    {
        goto fail;
    }
    
    ext2_block_alloc_stats(fs, block, +1);
    *ret = block;

    if (buf)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, buf);
    }

    return true;

fail:

    if (buf)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, buf);
    }

    return false;
}


/*
 * Create new directory block
 */
bool ext2_new_dir_block(PEXT2_FILESYS fs, ULONG dir_ino,
                   ULONG parent_ino, char **block)
{
    PEXT2_DIR_ENTRY dir = NULL;
    char        *buf;
    int         rec_len;
    int         filetype = 0;

    buf = (char *)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, fs->blocksize);
    if (!buf)
        return false;

    dir = (PEXT2_DIR_ENTRY) buf;
    dir->rec_len = fs->blocksize;

    if (dir_ino)
    {
        if (fs->ext2_sb->s_feature_incompat &
            EXT2_FEATURE_INCOMPAT_FILETYPE)
            filetype = EXT2_FT_DIR << 8;
        /*
         * Set up entry for '.'
         */
        dir->inode = dir_ino;
        dir->name_len = 1 | filetype;
        dir->name[0] = '.';
        rec_len = dir->rec_len - EXT2_DIR_REC_LEN(1);
        dir->rec_len = EXT2_DIR_REC_LEN(1);

        /*
         * Set up entry for '..'
         */
        dir = (struct ext2_dir_entry *) (buf + dir->rec_len);
        dir->rec_len = rec_len;
        dir->inode = parent_ino;
        dir->name_len = 2 | filetype;
        dir->name[0] = '.';
        dir->name[1] = '.';
    }

    *block = buf;

    return true;
}

bool ext2_write_block(PEXT2_FILESYS fs, ULONG block, void *inbuf)
{
    bool    retval = false;

    retval = NT_SUCCESS(Ext2WriteDisk(
                fs,
                ((ULONGLONG)block * fs->blocksize),
                fs->blocksize, (unsigned char *)inbuf));

    return retval;
}

bool ext2_read_block(PEXT2_FILESYS fs, ULONG block, void *inbuf)
{
    bool    retval = false;

    retval = NT_SUCCESS(Ext2ReadDisk(
                fs,
                ((ULONGLONG)block * fs->blocksize),
                fs->blocksize, (unsigned char *)inbuf));

    return retval;
}
