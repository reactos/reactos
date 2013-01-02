/*
 * PROJECT:          Mke2fs
 * FILE:             Inode.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"
#include <debug.h>

/* DEFINITIONS ***********************************************************/

extern char *device_name;

/* FUNCTIONS *************************************************************/


bool ext2_get_inode_lba(PEXT2_FILESYS Ext2Sys, ULONG no, LONGLONG *offset)
{
    LONGLONG loc = 0;
    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;

    if (no < 1 || no > pExt2Sb->s_inodes_count)
    {
        DPRINT1("Mke2fs: Inode value %lu was out of range in load_inode.(1-%ld)\n",
            no, pExt2Sb->s_inodes_count);
        *offset = 0;
        return false;
    }

    loc = (LONGLONG)(Ext2Sys->blocksize) * Ext2Sys->group_desc[(no - 1) / pExt2Sb->s_inodes_per_group].bg_inode_table +
        ((no - 1) % pExt2Sb->s_inodes_per_group) * sizeof(EXT2_INODE);

    *offset = loc;

    return true;    
}

bool ext2_load_inode(PEXT2_FILESYS Ext2Sys, ULONG no, PEXT2_INODE pInode)
{
    LONGLONG Offset;
    bool  bRet = false;

    if (ext2_get_inode_lba(Ext2Sys, no, &Offset))
    {
        bRet = NT_SUCCESS(Ext2ReadDisk(
                    Ext2Sys,
                    Offset,
                    sizeof(EXT2_INODE),
                    (unsigned char *)pInode));
    }

    return bRet;
}


bool ext2_save_inode(PEXT2_FILESYS Ext2Sys, ULONG no, PEXT2_INODE pInode)
{
    LONGLONG offset;
    bool  bRet = false;

    if (ext2_get_inode_lba(Ext2Sys, no, &offset))
    {
        bRet = NT_SUCCESS(Ext2WriteDisk(
                        Ext2Sys,
                        offset, 
                        sizeof(EXT2_INODE),
                        (unsigned char *)pInode));
    }

    return bRet;
}


/*
 * Right now, just search forward from the parent directory's block
 * group to find the next free inode.
 *
 * Should have a special policy for directories.
 */

bool ext2_new_inode(PEXT2_FILESYS fs, ULONG dir, int mode,
                      PEXT2_INODE_BITMAP map, ULONG *ret)
{
    ULONG   dir_group = 0;
    ULONG   i;
    ULONG   start_inode;

    if (!map)
        map = fs->inode_map;

    if (!map)
        return false;
    
    if (dir > 0) 
        dir_group = (dir - 1) / EXT2_INODES_PER_GROUP(fs->ext2_sb);

    start_inode = (dir_group * EXT2_INODES_PER_GROUP(fs->ext2_sb)) + 1;
    
    if (start_inode < EXT2_FIRST_INODE(fs->ext2_sb))
        start_inode = EXT2_FIRST_INODE(fs->ext2_sb);

    i = start_inode;

    do
    {
        if (!ext2_test_inode_bitmap(map, i))
            break;

        i++;

        if (i > fs->ext2_sb->s_inodes_count)
            i = EXT2_FIRST_INODE(fs->ext2_sb);

    } while (i != start_inode);
    
    if (ext2_test_inode_bitmap(map, i))
        return false;

    *ret = i;

    return true;
}


bool ext2_expand_block( PEXT2_FILESYS Ext2Sys, PEXT2_INODE Inode,
                        ULONG dwContent, ULONG Index, int layer,
                        ULONG newBlk, ULONG *dwRet, ULONG *off  )
{
    ULONG       *pData = NULL;
    ULONG       i = 0, j = 0, temp = 1;
    ULONG       dwBlk;
    ULONG       dwNewBlk = newBlk;
    bool        bDirty = false;
    bool        bRet = true;
    ULONG       Offset = 0;

    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;
 
    pData = (ULONG *)RtlAllocateHeap(RtlGetProcessHeap(), 0, Ext2Sys->blocksize);
    
    if (!pData)
    {
        bRet = false;
        goto errorout;
    }
    
    if (!ext2_read_block(Ext2Sys, dwContent, (void *)pData))
    {
        bRet = false;
        goto errorout;
    }
    
    if (layer == 1)
    {
        *dwRet = dwContent;
        *off   = Index;
        pData[Index] = newBlk;

        bDirty = TRUE;
    }
    else if (layer <= 3)
    {
        temp = 1 << ((10 + pExt2Sb->s_log_block_size - 2) * (layer - 1));

        i = Index / temp;
        j = Index % temp;

        dwBlk = pData[i];
        
        if (dwBlk == 0)
        {
            if (ext2_alloc_block(Ext2Sys, 0, &dwBlk) )
            {
                pData[i] = dwBlk;
                bDirty = true;

                Inode->i_blocks += (Ext2Sys->blocksize / SECTOR_SIZE);
            }
            
            if (!bDirty)
                goto errorout;
        }
        
        if (!ext2_expand_block(Ext2Sys, Inode, dwBlk, j, layer - 1, bDirty, &dwNewBlk, &Offset))
        {
            bRet = false;
            DPRINT1("Mke2fs: ext2_expand_block: ... error recuise...\n");
            goto errorout;
        }
    }

    if (bDirty)
    {
        bRet = ext2_write_block(Ext2Sys, dwContent, (void *)pData);
    }


errorout:

    if (pData)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pData);

    if (bRet && dwRet)
        *dwRet = dwNewBlk;

    return bRet;
}

bool ext2_expand_inode( PEXT2_FILESYS Ext2Sys,
                        PEXT2_INODE Inode,
                        ULONG newBlk         )
{
    ULONG dwSizes[4] = {12, 1, 1, 1};
    ULONG Index = 0;
    ULONG dwTotal = 0;
    ULONG dwBlk = 0, dwNewBlk = 0, Offset = 0;
    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;
    int   i = 0;
    bool  bRet = true;
    bool  bDirty = false;
    ULONG TotalBlocks;

    TotalBlocks = Inode->i_blocks / (Ext2Sys->blocksize / SECTOR_SIZE);
    Index = Ext2DataBlocks(Ext2Sys, TotalBlocks);

    for (i = 0; i < 4; i++)
    {
        dwSizes[i] = dwSizes[i] << ((10 + pExt2Sb->s_log_block_size - 2) * i);
        dwTotal += dwSizes[i];
    }

    if (Index >= dwTotal)
    {
        DPRINT1("Mke2fs: ext2_expand_inode: beyond the maxinum size of an inode.\n");
        return false;
    }

    for (i = 0; i < 4; i++)
    {
        if (Index < dwSizes[i])
        {
            if (i == 0)
            {
                Inode->i_block[Index] = newBlk;
                bDirty = true;
            }
            else
            {
                dwBlk = Inode->i_block[(i + 12 - 1)];

                if (dwBlk == 0)
                {
                    if (ext2_alloc_block(Ext2Sys, 0, &dwBlk))
                    {
                        Inode->i_block[(i + 12 - 1)] = dwBlk;
                        bDirty = true;

                        Inode->i_blocks += (Ext2Sys->blocksize / SECTOR_SIZE);
                    }
                    else
                    {
                        break;
                    }
                }

                dwNewBlk = 0;
                bRet = ext2_expand_block(
                             Ext2Sys,
                             Inode,
                             dwBlk,
                             Index,
                             i,
                             newBlk,
                             &dwNewBlk,
                             &Offset   );
            }
            
            break;
        }

        Index -= dwSizes[i];
    }

    return bRet;
}


bool ext2_get_block(PEXT2_FILESYS Ext2Sys, ULONG dwContent, ULONG Index, int layer, ULONG *dwRet)
{
    ULONG       *pData = NULL;
    LONGLONG    Offset = 0;
    ULONG       i = 0, j = 0, temp = 1;
    ULONG       dwBlk = 0;
    bool        bRet = true;

    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;

    Offset = (LONGLONG) dwContent;
    Offset = Offset * Ext2Sys->blocksize;

    pData = (ULONG *)RtlAllocateHeap(RtlGetProcessHeap(), 0, Ext2Sys->blocksize);

    if (!pData)
    {
        return false;
    }
    memset(pData, 0, Ext2Sys->blocksize);

    if (layer == 0)
    {
        dwBlk = dwContent;
    }
    else if (layer <= 3)
    {

        if (!ext2_read_block(Ext2Sys, dwContent, (void *)pData))
        {
            bRet = false;
            goto errorout;
        }

        temp = 1 << ((10 + pExt2Sb->s_log_block_size - 2) * (layer - 1));

        i = Index / temp;
        j = Index % temp;

        if (!ext2_get_block(Ext2Sys, pData[i], j, layer - 1, &dwBlk))
        {
            bRet = false;
            DPRINT1("Mke2fs: ext2_get_block: ... error recuise...\n");
            goto errorout;
        }
    }

errorout:

    if (pData)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pData);

    if (bRet && dwRet)
        *dwRet = dwBlk;

    return bRet;
}

bool ext2_block_map(PEXT2_FILESYS Ext2Sys, PEXT2_INODE inode, ULONG block, ULONG *dwRet)
{
    ULONG dwSizes[4] = { 12, 1, 1, 1 };
    ULONG Index = 0;
    ULONG dwBlk = 0;
    PEXT2_SUPER_BLOCK pExt2Sb = Ext2Sys->ext2_sb;
    UINT i;
    bool  bRet = false;

    Index = block;

    for (i = 0; i < 4; i++)
    {
        dwSizes[i] = dwSizes[i] << ((10 + pExt2Sb->s_log_block_size - 2) * i);
    }

    if (Index >= inode->i_blocks / (Ext2Sys->blocksize / SECTOR_SIZE))
    {
        DPRINT1("Mke2fs: ext2_block_map: beyond the size of the inode.\n");
        return false;
    }

    for (i = 0; i < 4; i++)
    {
        if (Index < dwSizes[i])
        {
            dwBlk = inode->i_block[i==0 ? (Index):(i + 12 - 1)];

            bRet = ext2_get_block(Ext2Sys, dwBlk, Index , i, &dwBlk); 

            break;
        }
        Index -= dwSizes[i];
    }

    if (bRet && dwBlk)
    {
        if (dwRet)
            *dwRet = dwBlk;

        return true;
    }
    else
        return false;
}

ULONG ext2_build_bdl(PEXT2_FILESYS Ext2Sys,
                    PEXT2_INODE ext2_inode,
                    IN ULONG offset, 
                    IN ULONG size, 
                    OUT PEXT2_BDL *ext2_bdl )
{
    ULONG   nBeg, nEnd, nBlocks;
    ULONG   dwBlk;
    ULONG   i;
    ULONG   dwBytes = 0;
    LONGLONG lba;

    PEXT2_BDL   ext2bdl = NULL;

    *ext2_bdl = NULL;

    if (offset >= ext2_inode->i_size)
    {
        DPRINT1("Mke2fs: ext2_build_bdl: beyond the file range.\n");
        return 0;
    }

/*
    if (offset + size > ext2_inode->i_size)
    {
        size = ext2_inode->i_size - offset;
    }
*/

    nBeg = offset / Ext2Sys->blocksize;
    nEnd = (size + offset + Ext2Sys->blocksize - 1) / Ext2Sys->blocksize;

    nBlocks = nEnd - nBeg;

    if (nBlocks > 0)
    {
        ext2bdl = (PEXT2_BDL)
            RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(EXT2_BDL) * nBlocks);

        if (ext2bdl)
        {

            memset(ext2bdl, 0, sizeof(EXT2_BDL) * nBlocks);
            
            for (i = nBeg; i < nEnd; i++)
            {
                if (!ext2_block_map(Ext2Sys, ext2_inode, i, &dwBlk))
                {
                    goto fail;
                }
                
                lba = (LONGLONG) dwBlk;
                lba = lba * Ext2Sys->blocksize;
                
                if (nBlocks == 1) // ie. (nBeg == nEnd - 1)
                {
                    dwBytes = size;
                    ext2bdl[i - nBeg].Lba = lba + (LONGLONG)(offset % (Ext2Sys->blocksize));
                    ext2bdl[i - nBeg].Length = dwBytes;
                    ext2bdl[i - nBeg].Offset = 0;
                }
                else
                {
                    if (i == nBeg)
                    {
                        dwBytes = Ext2Sys->blocksize - (offset % (Ext2Sys->blocksize));
                        ext2bdl[i - nBeg].Lba = lba + (LONGLONG)(offset % (Ext2Sys->blocksize));
                        ext2bdl[i - nBeg].Length = dwBytes;
                        ext2bdl[i - nBeg].Offset = 0;
                    }
                    else if (i == nEnd - 1)
                    {
                        ext2bdl[i - nBeg].Lba = lba;
                        ext2bdl[i - nBeg].Length = size - dwBytes;
                        ext2bdl[i - nBeg].Offset = dwBytes;
                        dwBytes = size;
                    }
                    else
                    {
                        ext2bdl[i - nBeg].Lba = lba;
                        ext2bdl[i - nBeg].Length = Ext2Sys->blocksize;
                        ext2bdl[i - nBeg].Offset = dwBytes;
                        dwBytes +=  Ext2Sys->blocksize;
                    }
                }
            }

            *ext2_bdl = ext2bdl;
            return nBlocks;
        }
    }

fail:

    if (ext2bdl)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ext2bdl);

    // Error
    return 0;
}


bool ext2_read_inode(PEXT2_FILESYS Ext2Sys,
            ULONG               ino,
            ULONG               offset,
            PVOID               Buffer,
            ULONG               size,
            PULONG              dwReturn)
{
    PEXT2_BDL   ext2_bdl = NULL;
    ULONG       blocks, i;
    bool        bRet = true;
    EXT2_INODE  ext2_inode;
    ULONG       dwTotal = 0;

    if (!ext2_load_inode(Ext2Sys, ino, &ext2_inode))
    {
        return false;
    }

    blocks = ext2_build_bdl(Ext2Sys, &ext2_inode, offset, size, &ext2_bdl);

    if (blocks <= 0)
        return  false;
    
   
    for(i = 0; i < blocks; i++)
    {
        bRet = NT_SUCCESS(Ext2ReadDisk(
                Ext2Sys,
                ext2_bdl[i].Lba, 
                ext2_bdl[i].Length,
                (PUCHAR)Buffer + ext2_bdl[i].Offset
               ));

        if (!bRet)
            break;
        dwTotal += ext2_bdl[i].Length;
    }

    *dwReturn = dwTotal;

    if (ext2_bdl)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ext2_bdl);

    return bRet;
}


bool ext2_write_inode (PEXT2_FILESYS Ext2Sys,
            ULONG               ino,
            ULONG               offset,
            PVOID               Buffer,
            ULONG               size,
            PULONG              dwReturn )
{
    PEXT2_BDL   ext2_bdl = NULL;
    ULONG       blocks, i;
    bool        bRet = true;
    EXT2_INODE  inode;
    ULONG       dwTotal = 0;
    ULONG       dwBlk = 0;
    ULONG       TotalBlks;

    blocks =  (size + offset + Ext2Sys->blocksize - 1) / Ext2Sys->blocksize;

    if (!ext2_load_inode(Ext2Sys, ino, &inode))
    {
        return false;
    }

    TotalBlks = inode.i_blocks / (Ext2Sys->blocksize / SECTOR_SIZE);
    TotalBlks = Ext2DataBlocks(Ext2Sys, TotalBlks);

    if (blocks > TotalBlks)
    {
        for (i=0; i < (blocks - TotalBlks);  i++)
        {
            if (ext2_alloc_block(Ext2Sys, 0, &dwBlk) )
            {
                ext2_expand_inode(Ext2Sys, &inode, dwBlk);
                inode.i_blocks += (Ext2Sys->blocksize/SECTOR_SIZE);
            }
        }
    }

    blocks = ext2_build_bdl(Ext2Sys, &inode, offset, size, &ext2_bdl);

    if (blocks <= 0)
        return  false;
    
    for(i = 0; i < blocks; i++)
    {
        bRet = NT_SUCCESS(Ext2WriteDisk(
                Ext2Sys,
                ext2_bdl[i].Lba, 
                ext2_bdl[i].Length,
                (PUCHAR)Buffer + ext2_bdl[i].Offset
               ));

        if (!bRet)
        {
            goto errorout;
        }

        dwTotal += ext2_bdl[i].Length;
    }

    *dwReturn = dwTotal;

    if (size + offset > inode.i_size)
    {
        inode.i_size = size + offset;
    }

    ext2_save_inode(Ext2Sys, ino, &inode);


errorout:
   
    if (ext2_bdl)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ext2_bdl);

    return bRet;
}

bool
ext2_add_entry( PEXT2_FILESYS Ext2Sys,
                ULONG parent, ULONG inode,
                int filetype, char *name )
{
    PEXT2_DIR_ENTRY2  dir = NULL, newdir = NULL;
    EXT2_INODE      parent_inode;
    ULONG       dwRet;
    char        *buf;
    int         rec_len;
    bool        bRet = false;

    rec_len = EXT2_DIR_REC_LEN(strlen(name));

    if (!ext2_load_inode(Ext2Sys, parent, &parent_inode))
    {
        return false;
    }

    buf = (char *)RtlAllocateHeap(RtlGetProcessHeap(), 0, parent_inode.i_size);

    if (!ext2_read_inode(Ext2Sys, parent, 0, buf, parent_inode.i_size, &dwRet))
    {
        return false;
    }

    dir = (PEXT2_DIR_ENTRY2) buf;

    while ((char *)dir < buf + parent_inode.i_size)
    {
        if ((dir->inode == 0 && dir->rec_len >= rec_len) || 
               (dir->rec_len >= dir->name_len + rec_len) )
        {
            if (dir->inode)
            {
                newdir = (PEXT2_DIR_ENTRY2) ((PUCHAR)dir + EXT2_DIR_REC_LEN(dir->name_len));
                newdir->rec_len = dir->rec_len - EXT2_DIR_REC_LEN(dir->name_len);

                dir->rec_len = EXT2_DIR_REC_LEN(dir->name_len);

                dir = newdir;
            }

            dir->file_type = filetype;
            dir->inode = inode;
            dir->name_len = strlen(name);
            memcpy(dir->name, name, strlen(name));
    
            bRet = true;
            break;
        }

        dir = (PEXT2_DIR_ENTRY2) (dir->rec_len + (PUCHAR) dir);
    }


    if (bRet)
        return ext2_write_inode(Ext2Sys, parent, 0, buf, parent_inode.i_size, &dwRet);

    return bRet;
}


bool ext2_reserve_inodes(PEXT2_FILESYS fs)
{
    ULONG   i;
    int     group;

    for (i = EXT2_ROOT_INO + 1; i < EXT2_FIRST_INODE(fs->ext2_sb); i++)
    {
        ext2_mark_inode_bitmap(fs->inode_map, i);
        group = ext2_group_of_ino(fs, i);
        fs->group_desc[group].bg_free_inodes_count--;
        fs->ext2_sb->s_free_inodes_count--;
    }

    return true;
}
