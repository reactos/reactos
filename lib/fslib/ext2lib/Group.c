/*
 * PROJECT:          Mke2fs
 * FILE:             Group.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"

/* DEFINITIONS ***********************************************************/

/* FUNCTIONS *************************************************************/

int test_root(int a, int b)
{
    if (a == 0)
        return 1;
    while (1)
    {
        if (a == 1)
            return 1;
        if (a % b)
            return 0;
        a = a / b;
    }
}

bool ext2_bg_has_super(PEXT2_SUPER_BLOCK pExt2Sb, int group_block)
{
    if (!(pExt2Sb->s_feature_ro_compat & EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER))
        return true;

    if (test_root(group_block, 3) || (test_root(group_block, 5)) ||
        test_root(group_block, 7))
        return true;
    
    return false;
}


bool ext2_allocate_group_desc(PEXT2_FILESYS Ext2Sys)
{
    ULONG size;

    size = Ext2Sys->desc_blocks * Ext2Sys->blocksize;

    Ext2Sys->group_desc =
        (PEXT2_GROUP_DESC)RtlAllocateHeap(RtlGetProcessHeap(), 0, size);

    if (Ext2Sys->group_desc)
    {
        memset(Ext2Sys->group_desc, 0, size);
        return true;
    }

    return false;
}

void ext2_free_group_desc(PEXT2_FILESYS Ext2Sys)
{
    if (Ext2Sys->group_desc)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Ext2Sys->group_desc);
        Ext2Sys->group_desc = NULL;
    }
}
