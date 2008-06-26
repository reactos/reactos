/*
 * PROJECT:          Mke2fs
 * FILE:             Bitmap.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"
#include <debug.h>

/* DEFINITIONS ***********************************************************/

/* FUNCTIONS *************************************************************/


bool ext2_set_bit(int nr,void * addr)
{
    int     mask;
    unsigned char *ADDR = (unsigned char *) addr;

    ADDR += nr >> 3;
    mask = 1 << (nr & 0x07);
    *ADDR |= mask;

    return true;
}

bool ext2_clear_bit(int nr, void * addr)
{
    int     mask;
    unsigned char   *ADDR = (unsigned char *) addr;

    ADDR += nr >> 3;
    mask = 1 << (nr & 0x07);
    *ADDR &= ~mask;
    return true;
}

bool ext2_test_bit(int nr, void * addr)
{
    int         mask;
    const unsigned char *ADDR = (const unsigned char *) addr;

    ADDR += nr >> 3;
    mask = 1 << (nr & 0x07);

    return ((mask & *ADDR) != 0);
}

bool ext2_mark_bitmap(PEXT2_BITMAP bitmap, ULONG bitno)
{
    if ((bitno < bitmap->start) || (bitno > bitmap->end))
    {
        return false;
    }

    return ext2_set_bit(bitno - bitmap->start, bitmap->bitmap);
}

bool ext2_unmark_bitmap(PEXT2_BITMAP bitmap, ULONG bitno)
{
    if ((bitno < bitmap->start) || (bitno > bitmap->end))
    {
        return false;
    }

    return ext2_clear_bit(bitno - bitmap->start, bitmap->bitmap);
}


bool ext2_test_block_bitmap(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG block)
{
    return ext2_test_bit(block - bitmap->start, bitmap->bitmap);
}


bool ext2_test_block_bitmap_range(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG block, int num)
{
    int i;

    for (i=0; i < num; i++)
    {
        if (ext2_test_block_bitmap(bitmap, block+i))
            return false;
    }
    return true;
}

bool ext2_test_inode_bitmap(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG inode)
{
    return ext2_test_bit(inode - bitmap->start, bitmap->bitmap);
}


bool ext2_allocate_block_bitmap(PEXT2_FILESYS Ext2Sys)
{
    ULONG   size = 0;

    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;
    Ext2Sys->block_map = (PEXT2_BLOCK_BITMAP)
        RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(EXT2_BLOCK_BITMAP));

    if (!Ext2Sys->block_map)
    {
        DPRINT1("Mke2fs: error allocating block bitmap...\n");
        return false;
    }

    memset(Ext2Sys->block_map, 0, sizeof(EXT2_BLOCK_BITMAP));

    Ext2Sys->block_map->start = pExt2Sb->s_first_data_block;
    Ext2Sys->block_map->end = pExt2Sb->s_blocks_count-1;
    Ext2Sys->block_map->real_end = (EXT2_BLOCKS_PER_GROUP(pExt2Sb)
        * Ext2Sys->group_desc_count) -1 + Ext2Sys->block_map->start;

    size = (((Ext2Sys->block_map->real_end - Ext2Sys->block_map->start) / 8) + 1);

    Ext2Sys->block_map->bitmap =
        (char *)RtlAllocateHeap(RtlGetProcessHeap(), 0, size);

    if (!Ext2Sys->block_map->bitmap)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Ext2Sys->block_map);
        Ext2Sys->block_map = NULL;
        DPRINT1("Mke2fs: error allocating block bitmap...\n");
        return false;
    }

    memset(Ext2Sys->block_map->bitmap, 0, size);

    return true;
}


bool ext2_allocate_inode_bitmap(PEXT2_FILESYS Ext2Sys)
{
    ULONG   size = 0;

    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;

    Ext2Sys->inode_map = (PEXT2_INODE_BITMAP)
        RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(EXT2_INODE_BITMAP));

    if (!Ext2Sys->inode_map)
    {
        DPRINT1("Mke2fs: error allocating inode bitmap...\n");
        return false;
    }

    memset(Ext2Sys->inode_map, 0, sizeof(EXT2_INODE_BITMAP));

    Ext2Sys->inode_map->start = 1;
    Ext2Sys->inode_map->end = pExt2Sb->s_inodes_count;
    Ext2Sys->inode_map->real_end = (EXT2_INODES_PER_GROUP(pExt2Sb)
        * Ext2Sys->group_desc_count);

    size = (((Ext2Sys->inode_map->real_end - Ext2Sys->inode_map->start) / 8) + 1);

    Ext2Sys->inode_map->bitmap =
        (char *)RtlAllocateHeap(RtlGetProcessHeap(), 0, size);

    if (!Ext2Sys->inode_map->bitmap)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Ext2Sys->inode_map);
        Ext2Sys->inode_map = NULL;
        DPRINT1("Mke2fs: error allocating block bitmap...\n");
        return false;
    }

    memset(Ext2Sys->inode_map->bitmap, 0, size);

    return true;
}

void ext2_free_generic_bitmap(PEXT2_GENERIC_BITMAP bitmap)
{
    if (!bitmap)
        return;

    if (bitmap->bitmap)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, bitmap->bitmap);
        bitmap->bitmap = 0;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, bitmap);
}

void ext2_free_inode_bitmap(PEXT2_FILESYS Ext2Sys)
{
    PEXT2_INODE_BITMAP bitmap = Ext2Sys->inode_map;
    if (!bitmap)
        return;

    ext2_free_generic_bitmap(bitmap);

    Ext2Sys->inode_map = NULL;
}

void ext2_free_block_bitmap(PEXT2_FILESYS Ext2Sys)
{
    PEXT2_BLOCK_BITMAP bitmap = Ext2Sys->block_map;
    if (!bitmap)
        return;

    ext2_free_generic_bitmap(bitmap);

    Ext2Sys->block_map = NULL;
}

bool ext2_write_inode_bitmap(PEXT2_FILESYS fs)
{
    ULONG   i;
    ULONG   nbytes;
    bool    retval;
    char    *inode_bitmap = fs->inode_map->bitmap;
    char    *bitmap_block = NULL;
    ULONG   blk;

    if (!inode_bitmap)
        return false;

    nbytes = (size_t) ((EXT2_INODES_PER_GROUP(fs->ext2_sb)+7) / 8);
    
    bitmap_block = (char *)RtlAllocateHeap(RtlGetProcessHeap(), 0, fs->blocksize);
    if (!bitmap_block) return false;

    memset(bitmap_block, 0xff, fs->blocksize);

    for (i = 0; i < fs->group_desc_count; i++)
    {
        memcpy(bitmap_block, inode_bitmap, nbytes);
        blk = fs->group_desc[i].bg_inode_bitmap;

        if (blk)
        {
/*
#ifdef EXT2_BIG_ENDIAN_BITMAPS
            if (!((fs->flags & EXT2_FLAG_SWAP_BYTES) ||
                  (fs->flags & EXT2_FLAG_SWAP_BYTES_WRITE)))
                ext2_swap_bitmap(fs, bitmap_block, nbytes);
#endif
*/
            retval = NT_SUCCESS(Ext2WriteDisk(
                    fs,
                    ((ULONGLONG)blk * fs->blocksize),
                    fs->blocksize, 
                    (unsigned char *)bitmap_block));

            if (!retval)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, bitmap_block);
                return false;
            }
        }

        inode_bitmap += nbytes;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, bitmap_block);

    return true;
}

bool ext2_write_block_bitmap (PEXT2_FILESYS fs)
{
    ULONG   i;
    int     j;
    int     nbytes;
    int     nbits;
    bool    retval;
    char    *block_bitmap = fs->block_map->bitmap;
    char    *bitmap_block = NULL;
    ULONG   blk;

    if (!block_bitmap)
        return false;

    nbytes = EXT2_BLOCKS_PER_GROUP(fs->ext2_sb) / 8;

    bitmap_block = (char *)RtlAllocateHeap(RtlGetProcessHeap(), 0, fs->blocksize);
    if (!bitmap_block)
        return false;

    memset(bitmap_block, 0xff, fs->blocksize);

    for (i = 0; i < fs->group_desc_count; i++)
    {
        memcpy(bitmap_block, block_bitmap, nbytes);

        if (i == fs->group_desc_count - 1)
        {
            /* Force bitmap padding for the last group */
            nbits = (int) ((fs->ext2_sb->s_blocks_count
                    - fs->ext2_sb->s_first_data_block)
                       % EXT2_BLOCKS_PER_GROUP(fs->ext2_sb));

            if (nbits)
            {
                for (j = nbits; j < fs->blocksize * 8; j++)
                {
                    ext2_set_bit(j, bitmap_block);
                }
            }
        }

        blk = fs->group_desc[i].bg_block_bitmap;

        if (blk)
        {
#ifdef EXT2_BIG_ENDIAN_BITMAPS
            if (!((fs->flags & EXT2_FLAG_SWAP_BYTES) ||
                  (fs->flags & EXT2_FLAG_SWAP_BYTES_WRITE)))
                ext2_swap_bitmap(fs, bitmap_block, nbytes);
#endif
            retval = NT_SUCCESS(Ext2WriteDisk(
                        fs,
                        ((ULONGLONG)blk * fs->blocksize),
                        fs->blocksize,
                        (unsigned char *)bitmap_block));

            if (!retval)
            {
                RtlFreeHeap(RtlGetProcessHeap(), 0, bitmap_block);
                return false;
            }
        }

        block_bitmap += nbytes;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, bitmap_block);

    return true;
}

bool ext2_write_bitmaps(PEXT2_FILESYS fs)
{
    bool    retval;

    if (fs->block_map) // && ext2fs_test_bb_dirty(fs))
    {
        retval = ext2_write_block_bitmap(fs);
        if (!retval)
            return retval;
    }

    if (fs->inode_map) // && ext2fs_test_ib_dirty(fs))
    {
        retval = ext2_write_inode_bitmap(fs);
        if (!retval)
            return retval;
    }

    return true;
}


bool read_bitmaps(PEXT2_FILESYS fs, int do_inode, int do_block)
{
    ULONG i;
    char *block_bitmap = 0, *inode_bitmap = 0;
    bool retval = false;
    ULONG block_nbytes = EXT2_BLOCKS_PER_GROUP(fs->ext2_sb) / 8;
    ULONG inode_nbytes = EXT2_INODES_PER_GROUP(fs->ext2_sb) / 8;
    ULONG blk;

//  fs->write_bitmaps = ext2_write_bitmaps;

    if (do_block)
    {
        if (fs->block_map)
            ext2_free_block_bitmap(fs);

        retval = ext2_allocate_block_bitmap(fs);

        if (!retval)
            goto cleanup;

        block_bitmap = fs->block_map->bitmap;
    }

    if (do_inode)
    {
        if (fs->inode_map)
            ext2_free_inode_bitmap(fs);

        retval = ext2_allocate_inode_bitmap(fs);
        if (!retval)
            goto cleanup;

        inode_bitmap = fs->inode_map->bitmap;
    }

    for (i = 0; i < fs->group_desc_count; i++)
    {
        if (block_bitmap)
        {
            blk = fs->group_desc[i].bg_block_bitmap;

            if (blk)
            {
                retval = NT_SUCCESS(Ext2ReadDisk(
                            fs,
                            ((ULONGLONG)blk * fs->blocksize), 
                            block_nbytes,
                            (unsigned char *) block_bitmap));

                if (!retval)
                {
                    goto cleanup;
                }

#ifdef EXT2_BIG_ENDIAN_BITMAPS
                if (!((fs->flags & EXT2_FLAG_SWAP_BYTES) ||
                      (fs->flags & EXT2_FLAG_SWAP_BYTES_READ)))
                    ext2_swap_bitmap(fs, block_bitmap, block_nbytes);
#endif
            }
            else
            {
                memset(block_bitmap, 0, block_nbytes);
            }

            block_bitmap += block_nbytes;
        }

        if (inode_bitmap)
        {
            blk = fs->group_desc[i].bg_inode_bitmap;
            if (blk)
            {
                retval = NT_SUCCESS(Ext2ReadDisk(
                    fs, ((LONGLONG)blk * fs->blocksize), 
                    inode_nbytes, 
                    (unsigned char *)inode_bitmap));

                if (!retval)
                {
                    goto cleanup;
                }

#ifdef EXT2_BIG_ENDIAN_BITMAPS
                if (!((fs->flags & EXT2_FLAG_SWAP_BYTES) ||
                      (fs->flags & EXT2_FLAG_SWAP_BYTES_READ)))
                    ext2_swap_bitmap(fs, inode_bitmap, inode_nbytes);
#endif
            }
            else
            {
                memset(inode_bitmap, 0, inode_nbytes);
            }

            inode_bitmap += inode_nbytes;
        }
    }

    return true;
    
cleanup:

    if (do_block)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, fs->block_map);
        fs->block_map = NULL;
    }

    if (do_inode)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, fs->inode_map);
        fs->inode_map = 0;
    }

    return false;
}

bool ext2_read_inode_bitmap (PEXT2_FILESYS fs)
{
    return read_bitmaps(fs, 1, 0);
}

bool ext2_read_block_bitmap(PEXT2_FILESYS fs)
{
    return read_bitmaps(fs, 0, 1);
}

bool ext2_read_bitmaps(PEXT2_FILESYS fs)
{

    if (fs->inode_map && fs->block_map)
        return 0;

    return read_bitmaps(fs, !fs->inode_map, !fs->block_map);
}

