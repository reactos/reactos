/*
 * PROJECT:          Mke2fs
 * FILE:             Badblock.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"

/* DEFINITIONS ***********************************************************/

/* FUNCTIONS *************************************************************/

bool create_bad_block_inode( PEXT2_FILESYS Ext2Sys,
                             PEXT2_BADBLK_LIST bb_list)
{
    bool            retval;
    EXT2_INODE      inode;
    LARGE_INTEGER   SysTime;
    
    NtQuerySystemTime(&SysTime);

    ext2_mark_inode_bitmap(Ext2Sys->inode_map, EXT2_BAD_INO);

    Ext2Sys->group_desc[0].bg_free_inodes_count--;
    Ext2Sys->ext2_sb->s_free_inodes_count--;

    memset(&inode, 0, sizeof(EXT2_INODE));
    inode.i_mode = (USHORT)((0777 & ~Ext2Sys->umask));
    inode.i_uid = inode.i_gid = 0;
    inode.i_blocks = 0;
    inode.i_block[0] = 0;
    inode.i_links_count = 2;
    RtlTimeToSecondsSince1970(&SysTime, &inode.i_mtime);
    inode.i_ctime = inode.i_atime = inode.i_mtime;
    inode.i_size = 0;

    retval = ext2_save_inode(Ext2Sys, EXT2_BAD_INO, &inode);

    return retval;
}
