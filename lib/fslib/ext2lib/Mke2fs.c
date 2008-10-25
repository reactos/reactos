/*
 * PROJECT:          Mke2fs
 * FILE:             Disk.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"
#include <debug.h>

/* GLOBALS ***************************************************************/

int     inode_ratio = 4096;

BOOLEAN bLocked = FALSE;

/* FUNCTIONS *************************************************************/

int int_log2(int arg)
{
    int l = 0;

    arg >>= 1;

    while (arg)
    {
        l++;
        arg >>= 1;
    }

    return l;
}

int int_log10(unsigned int arg)
{
    int l;

    for (l=0; arg ; l++)
        arg = arg / 10;

    return l;
}


static char default_str[] = "default";

struct mke2fs_defaults {
    const char  *type;
    int     size;
    int     blocksize;
    int     inode_ratio;
} settings[] = {
    { default_str, 0, 4096, 8192 },
    { default_str, 512, 1024, 4096 },
    { default_str, 3, 1024, 8192 },
    { "journal", 0, 4096, 8192 },
    { "news", 0, 4096, 4096 },
    { "largefile", 0, 4096, 1024 * 1024 },
    { "largefile4", 0, 4096, 4096 * 1024 },
    { 0, 0, 0, 0},
};

void set_fs_defaults(const char *fs_type,
                PEXT2_SUPER_BLOCK super,
                int blocksize, int *inode_ratio)
{
    int megs;
    int ratio = 0;
    struct mke2fs_defaults *p;

    megs = (super->s_blocks_count * (EXT2_BLOCK_SIZE(super) / 1024) / 1024);

    if (inode_ratio)
        ratio = *inode_ratio;

    if (!fs_type)
        fs_type = default_str;

    for (p = settings; p->type; p++)
    {
        if ((strcmp(p->type, fs_type) != 0) &&
            (strcmp(p->type, default_str) != 0))
            continue;

        if ((p->size != 0) &&
            (megs > p->size))
            continue;

        if (ratio == 0)
            *inode_ratio = p->inode_ratio;

        if (blocksize == 0)
        {
            super->s_log_frag_size = super->s_log_block_size =
                int_log2(p->blocksize >> EXT2_MIN_BLOCK_LOG_SIZE);
        }
    }

    if (blocksize == 0)
    {
        super->s_blocks_count /= EXT2_BLOCK_SIZE(super) / 1024;
    }
}

/*
 * Helper function which zeros out _num_ blocks starting at _blk_.  In
 * case of an error, the details of the error is returned via _ret_blk_
 * and _ret_count_ if they are non-NULL pointers.  Returns 0 on
 * success, and an error code on an error.
 *
 * As a special case, if the first argument is NULL, then it will
 * attempt to free the static zeroizing buffer.  (This is to keep
 * programs that check for memory leaks happy.)
 */
bool zero_blocks(PEXT2_FILESYS fs, ULONG blk, ULONG num,
                 ULONG *ret_blk, ULONG *ret_count)
{
    ULONG       j, count, next_update, next_update_incr;
    static unsigned char        *buf;
    bool        retval;

    /* If fs is null, clean up the static buffer and return */
    if (!fs)
    {
        if (buf)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, buf);
            buf = 0;
        }
        return true;
    }

#define STRIDE_LENGTH 8

    /* Allocate the zeroizing buffer if necessary */
    if (!buf)
    {
        buf = (unsigned char *)
            RtlAllocateHeap(RtlGetProcessHeap(), 0, fs->blocksize * STRIDE_LENGTH);
        if (!buf)
        {
            DPRINT1("Mke2fs: while allocating zeroizing buffer");
            return false;
        }
        memset(buf, 0, fs->blocksize * STRIDE_LENGTH);
    }

    /* OK, do the write loop */
    next_update = 0;
    next_update_incr = num / 100;
    if (next_update_incr < 1)
        next_update_incr = 1;

    for (j=0; j < num; j += STRIDE_LENGTH, blk += STRIDE_LENGTH)
    {
        if (num-j > STRIDE_LENGTH)
            count = STRIDE_LENGTH;
        else
            count = num - j;

        retval = NT_SUCCESS(Ext2WriteDisk(
                        fs,
                        ((ULONGLONG)blk * fs->blocksize),
                        count * fs->blocksize,
                        buf));

        if (!retval)
        {
            if (ret_count)
                *ret_count = count;

            if (ret_blk)
                *ret_blk = blk;

            return retval;
        }
    }

    return true;
}   


bool zap_sector(PEXT2_FILESYS Ext2Sys, int sect, int nsect)
{
    unsigned char *buf;
    ULONG         *magic;  

    buf = (unsigned char *)
        RtlAllocateHeap(RtlGetProcessHeap(), 0, SECTOR_SIZE*nsect);
    if (!buf)
    {
        DPRINT1("Mke2fs: Out of memory erasing sectors %d-%d\n",
                sect, sect + nsect - 1);
        return false;
    }

    memset(buf, 0, (ULONG)nsect * SECTOR_SIZE);
    
#define BSD_DISKMAGIC   (0x82564557UL)  /* The disk magic number */
#define BSD_MAGICDISK   (0x57455682UL)  /* The disk magic number reversed */
#define BSD_LABEL_OFFSET        64

    if (sect == 0)
    {
        Ext2ReadDisk(
                  Ext2Sys, 
                  (LONGLONG)(sect * SECTOR_SIZE),
                  SECTOR_SIZE, buf);

        // Check for a BSD disklabel, and don't erase it if so
        magic = (ULONG *) (buf + BSD_LABEL_OFFSET);
        if ((*magic == BSD_DISKMAGIC) ||   (*magic == BSD_MAGICDISK))
                goto clean_up;
    }

    // Write buf to disk
    Ext2WriteDisk( Ext2Sys,
                   (LONGLONG)(sect * SECTOR_SIZE),
                   (ULONG)nsect * SECTOR_SIZE,
                   buf );

clean_up:

    RtlFreeHeap(RtlGetProcessHeap(), 0, buf);
    
    return true;
}

bool ext2_mkdir( PEXT2_FILESYS fs, 
                 ULONG parent,
                 ULONG inum, 
                 char *name,
                 ULONG *no,
                 PEXT2_INODE pid )
{
    bool            retval;
    EXT2_INODE      parent_inode, inode;
    ULONG           ino = inum;
    //ULONG         scratch_ino;
    ULONG           blk;
    char            *block = 0;
    int             filetype = 0;

    LARGE_INTEGER   SysTime;
    
    NtQuerySystemTime(&SysTime);

    /*
     * Allocate an inode, if necessary
     */
    if (!ino)
    {
        retval = ext2_new_inode(fs, parent, LINUX_S_IFDIR | 0755, 0, &ino);
        if (!retval)
            goto cleanup;
    }

    if (no)
        *no = ino;

    /*
     * Allocate a data block for the directory
     */
    retval = ext2_new_block(fs, 0, 0, &blk);
    if (!retval)
        goto cleanup;

    /*
     * Create a scratch template for the directory
     */
    retval = ext2_new_dir_block(fs, ino, parent, &block);
    if (!retval)
        goto cleanup;

    /*
     * Get the parent's inode, if necessary
     */
    if (parent != ino)
    {
        retval = ext2_load_inode(fs, parent, &parent_inode);
        if (!retval)
            goto cleanup;
    }
    else
    {
        memset(&parent_inode, 0, sizeof(parent_inode));
    }

    /*
     * Create the inode structure....
     */
    memset(&inode, 0, sizeof(EXT2_INODE));
    inode.i_mode = (USHORT)(LINUX_S_IFDIR | (0777 & ~fs->umask));
    inode.i_uid = inode.i_gid = 0;
    inode.i_blocks = fs->blocksize / 512;
    inode.i_block[0] = blk;
    inode.i_links_count = 2;
    RtlTimeToSecondsSince1970(&SysTime, &inode.i_mtime);
    inode.i_ctime = inode.i_atime = inode.i_mtime;
    inode.i_size = fs->blocksize;

    /*
     * Write out the inode and inode data block
     */
    retval = ext2_write_block(fs, blk, block);
    if (!retval)
        goto cleanup;

    retval = ext2_save_inode(fs, ino, &inode); 
    if (!retval)
        goto cleanup;

    if (pid)
    {
        *pid = inode;
    }

    if (parent != ino)
    {
        /*
         * Add entry for this inode to parent dir 's block
         */

        if (fs->ext2_sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE)
                filetype = EXT2_FT_DIR;

        retval = ext2_add_entry(fs, parent, ino, filetype, name);

        if (!retval)
            goto cleanup;

        /*
         * Update parent inode's counts
         */

        parent_inode.i_links_count++;
        retval = ext2_save_inode(fs, parent, &parent_inode);
        if (!retval)
            goto cleanup;

    }
    
    /*
     * Update accounting....
     */
    ext2_block_alloc_stats(fs, blk, +1);
    ext2_inode_alloc_stats2(fs, ino, +1, 1);

cleanup:

    if (block)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, block);
        block = NULL;
    }

    return retval;
}

bool create_root_dir(PEXT2_FILESYS fs)
{
    bool        retval;
    EXT2_INODE  inode;

    retval = ext2_mkdir(fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 0, NULL, &inode);
    
    if (!retval)
    {
        DPRINT1("Mke2fs: while creating root dir");
        return false;
    }

    {
        inode.i_uid = 0;    
        inode.i_gid = 0;

        retval = ext2_save_inode(fs, EXT2_ROOT_INO, &inode);
        if (!retval)
        {
            DPRINT1("Mke2fs: while setting root inode ownership");
            return false;
        }
    }

    return true;
}

bool create_lost_and_found(PEXT2_FILESYS Ext2Sys)
{
    bool        retval;
    ULONG       ino;
    char        *name = "lost+found";
    int         lpf_size = 0;
    EXT2_INODE  inode;
    ULONG       dwBlk = 0;
    BOOLEAN     bExt= TRUE;

    PEXT2_DIR_ENTRY dir;

    char *      buf;

    buf = (char *)RtlAllocateHeap(RtlGetProcessHeap(), 0, Ext2Sys->blocksize);
    if (!buf)
    {
        bExt = FALSE;
    }
    else
    {
        memset(buf, 0, Ext2Sys->blocksize);

        dir = (PEXT2_DIR_ENTRY) buf;
        dir->rec_len = Ext2Sys->blocksize;
    }

    Ext2Sys->umask = 077;
    retval = ext2_mkdir(Ext2Sys, EXT2_ROOT_INO, 0, name, &ino, &inode);
    
    if (!retval)
    {
        DPRINT1("Mke2fs: while creating /lost+found.\n");
        return false;
    }

    if (!bExt)
        goto errorout;

    lpf_size = inode.i_size;

    while(TRUE)
    {
        if (lpf_size >= 16*1024)
            break;
        
        retval = ext2_alloc_block(Ext2Sys, 0, &dwBlk);

        if (! retval)
        {
            DPRINT1("Mke2fs: create_lost_and_found: error alloc block.\n");
            break;
        }

        retval = ext2_expand_inode(Ext2Sys, &inode, dwBlk);
        if (!retval)
        {
            DPRINT1("Mke2fs: errors when expanding /lost+found.\n");
            break;
        }

        ext2_write_block(Ext2Sys, dwBlk, buf);

        inode.i_blocks += (Ext2Sys->blocksize/SECTOR_SIZE);
        lpf_size += Ext2Sys->blocksize;
    }

    {
        inode.i_size = lpf_size;

        ASSERT( (inode.i_size/Ext2Sys->blocksize) == 
                Ext2DataBlocks(Ext2Sys, inode.i_blocks/(Ext2Sys->blocksize/SECTOR_SIZE)));

        ASSERT( (inode.i_blocks/(Ext2Sys->blocksize/SECTOR_SIZE)) == 
                Ext2TotalBlocks(Ext2Sys, inode.i_size/Ext2Sys->blocksize));

    }

    ext2_save_inode(Ext2Sys, ino, &inode);

errorout:

    if (buf)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, buf);
    }

    return true;
}

/*
 * This function forces out the primary superblock.  We need to only
 * write out those fields which we have changed, since if the
 * filesystem is mounted, it may have changed some of the other
 * fields.
 *
 * It takes as input a superblock which has already been byte swapped
 * (if necessary).
 */

bool write_primary_superblock(PEXT2_FILESYS Ext2Sys, PEXT2_SUPER_BLOCK super)
{
    bool bRet;    

    bRet = NT_SUCCESS(Ext2WriteDisk(
                           Ext2Sys,
                           ((LONGLONG)SUPERBLOCK_OFFSET),
                           SUPERBLOCK_SIZE, (PUCHAR)super));
            


    return bRet;
}


/*
 * Updates the revision to EXT2_DYNAMIC_REV
 */
void ext2_update_dynamic_rev(PEXT2_FILESYS fs)
{
    PEXT2_SUPER_BLOCK sb = fs->ext2_sb;

    if (sb->s_rev_level > EXT2_GOOD_OLD_REV)
        return;

    sb->s_rev_level = EXT2_DYNAMIC_REV;
    sb->s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
    sb->s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
    /* s_uuid is handled by e2fsck already */
    /* other fields should be left alone */
}


bool ext2_flush(PEXT2_FILESYS fs)
{
    ULONG       i,j,maxgroup,sgrp;
    ULONG       group_block;
    bool        retval;
    char        *group_ptr;
    unsigned long fs_state;
    PEXT2_SUPER_BLOCK super_shadow = 0;
    PEXT2_GROUP_DESC group_shadow = 0;

    LARGE_INTEGER   SysTime;
    
    NtQuerySystemTime(&SysTime);
    
    fs_state = fs->ext2_sb->s_state;

    RtlTimeToSecondsSince1970(&SysTime, &fs->ext2_sb->s_wtime);
    fs->ext2_sb->s_block_group_nr = 0;

    super_shadow = fs->ext2_sb;
    group_shadow = fs->group_desc;
    
    /*
     * Write out master superblock.  This has to be done
     * separately, since it is located at a fixed location
     * (SUPERBLOCK_OFFSET).
     */
    retval = write_primary_superblock(fs, super_shadow);
    if (!retval)
        goto errout;

    /*
     * If this is an external journal device, don't write out the
     * block group descriptors or any of the backup superblocks
     */
    if (fs->ext2_sb->s_feature_incompat &
        EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)
    {
        retval = false;
        goto errout;
    }

    /*
     * Set the state of the FS to be non-valid.  (The state has
     * already been backed up earlier, and will be restored when
     * we exit.)
     */
    fs->ext2_sb->s_state &= ~EXT2_VALID_FS;

    /*
     * Write out the master group descriptors, and the backup
     * superblocks and group descriptors.
     */
    group_block = fs->ext2_sb->s_first_data_block;
    maxgroup = fs->group_desc_count;

    for (i = 0; i < maxgroup; i++)
    {
        if (!ext2_bg_has_super(fs->ext2_sb, i))
            goto next_group;

        sgrp = i;
        if (sgrp > ((1 << 16) - 1))
            sgrp = (1 << 16) - 1;

        fs->ext2_sb->s_block_group_nr = (USHORT) sgrp;

        if (i !=0 )
        {
            retval = NT_SUCCESS(Ext2WriteDisk(
                                fs,
                                ((ULONGLONG)group_block * fs->blocksize),
                                SUPERBLOCK_SIZE, (PUCHAR)super_shadow));

            if (!retval)
            {
                goto errout;
            }
        }

        group_ptr = (char *) group_shadow;

        for (j=0; j < fs->desc_blocks; j++)
        {

            retval = NT_SUCCESS(Ext2WriteDisk(
                                fs,
                                ((ULONGLONG)(group_block+1+j) * fs->blocksize),
                                fs->blocksize, (PUCHAR) group_ptr));

            if (!retval)
            {
                goto errout;
            }

            group_ptr += fs->blocksize;
        }

    next_group:
        group_block += EXT2_BLOCKS_PER_GROUP(fs->ext2_sb);

    }

    fs->ext2_sb->s_block_group_nr = 0;

    /*
     * If the write_bitmaps() function is present, call it to
     * flush the bitmaps.  This is done this way so that a simple
     * program that doesn't mess with the bitmaps doesn't need to
     * drag in the bitmaps.c code.
     */
    retval = ext2_write_bitmaps(fs);
    if (!retval)
        goto errout;

    /*
     * Flush the blocks out to disk
     */

    // retval = io_channel_flush(fs->io);

errout:

    fs->ext2_sb->s_state = (USHORT) fs_state;

    return retval;
}


bool create_journal_dev(PEXT2_FILESYS fs)
{
    bool        retval = false;
    char        *buf = NULL;
    ULONG       blk;
    ULONG       count;

    if (!retval)
    {
        DPRINT1("Mke2fs: ext2_create_journal_dev: while initializing journal superblock.\n");
        return false;
    }

    DPRINT("Mke2fs: Zeroing journal device: \n");

    retval = zero_blocks(fs, 0, fs->ext2_sb->s_blocks_count,
                 &blk, &count);

    zero_blocks(0, 0, 0, 0, 0);

    if (!retval)
    {
        DPRINT1("Mke2fs: create_journal_dev: while zeroing journal device (block %lu, count %lu).\n",
            blk, count);
        return false;
    }

    retval = NT_SUCCESS(Ext2WriteDisk(
                    fs,
                    ((ULONGLONG)blk * (fs->ext2_sb->s_first_data_block+1)),
                    fs->blocksize, (unsigned char *)buf));

    if (!retval)
    {
        DPRINT1("Mke2fs: create_journal_dev: while writing journal superblock.\n");
        return false;
    }

    return true;
}

#define BLOCK_BITS (Ext2Sys->ext2_sb->s_log_block_size + 10)

ULONG
Ext2DataBlocks(PEXT2_FILESYS Ext2Sys, ULONG TotalBlocks)
{
    ULONG   dwData[4] = {1, 1, 1, 1};
    ULONG   dwMeta[4] = {0, 0, 0, 0};
    ULONG   DataBlocks = 0;
    ULONG   i, j;

    if (TotalBlocks <= EXT2_NDIR_BLOCKS)
    {
        return TotalBlocks;
    }

    TotalBlocks -= EXT2_NDIR_BLOCKS;

    for (i = 0; i < 4; i++)
    {
        dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);

        if (i > 0)
        {
            dwMeta[i] = 1 + (dwMeta[i - 1] << (BLOCK_BITS - 2));
        }
    }

    for( i=1; (i < 4) && (TotalBlocks > 0); i++)
    {
        if (TotalBlocks >= (dwData[i] + dwMeta[i]))
        {
            TotalBlocks -= (dwData[i] + dwMeta[i]);
            DataBlocks  += dwData[i];
        }
        else
        {
            ULONG   dwDivide = 0;
            ULONG   dwRemain = 0;

            for (j=i; (j > 0) && (TotalBlocks > 0); j--)
            {
                dwDivide = (TotalBlocks - 1) / (dwData[j-1] + dwMeta[j-1]);
                dwRemain = (TotalBlocks - 1) % (dwData[j-1] + dwMeta[j-1]);

                DataBlocks += (dwDivide * dwData[j-1]);
                TotalBlocks = dwRemain;
            }
        }
    }

    return (DataBlocks + EXT2_NDIR_BLOCKS);
}


ULONG
Ext2TotalBlocks(PEXT2_FILESYS Ext2Sys, ULONG DataBlocks)
{
    ULONG   dwData[4] = {1, 1, 1, 1};
    ULONG   dwMeta[4] = {0, 0, 0, 0};
    ULONG   TotalBlocks = 0;
    ULONG   i, j;

    if (DataBlocks <= EXT2_NDIR_BLOCKS)
    {
        return DataBlocks;
    }

    DataBlocks -= EXT2_NDIR_BLOCKS;

    for (i = 0; i < 4; i++)
    {
        dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);

        if (i > 0)
        {
            dwMeta[i] = 1 + (dwMeta[i - 1] << (BLOCK_BITS - 2));
        }
    }

    for( i=1; (i < 4) && (DataBlocks > 0); i++)
    {
        if (DataBlocks >= dwData[i])
        {
            DataBlocks  -= dwData[i];
            TotalBlocks += (dwData[i] + dwMeta[i]);
        }
        else
        {
            ULONG   dwDivide = 0;
            ULONG   dwRemain = 0;

            for (j=i; (j > 0) && (DataBlocks > 0); j--)
            {
                dwDivide = (DataBlocks) / (dwData[j-1]);
                dwRemain = (DataBlocks) % (dwData[j-1]);

                TotalBlocks += (dwDivide * (dwData[j-1] + dwMeta[j-1]) + 1);
                DataBlocks = dwRemain;
            }
        }
    }

    return (TotalBlocks + EXT2_NDIR_BLOCKS);
}


NTSTATUS NTAPI
Ext2Format(
	IN PUNICODE_STRING DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PUNICODE_STRING Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback)
{
    BOOLEAN    bRet = FALSE;
    NTSTATUS   Status = STATUS_UNSUCCESSFUL;
    /* Super Block: 1024 bytes long */
    EXT2_SUPER_BLOCK Ext2Sb;
    /* File Sys Structure */
    EXT2_FILESYS     FileSys;
    ULONG Percent;


    Callback(PROGRESS, 0, (PVOID)&Percent);


    RtlZeroMemory(&Ext2Sb, sizeof(EXT2_SUPER_BLOCK));
    RtlZeroMemory(&FileSys, sizeof(EXT2_FILESYS));
    FileSys.ext2_sb = &Ext2Sb;


    if (!NT_SUCCESS(Ext2OpenDevice(&FileSys, DriveRoot)))
    {
        DPRINT1("Mke2fs: Volume %wZ does not exist, ...\n", DriveRoot);
        goto clean_up;
    }


    if (!NT_SUCCESS(Ext2GetMediaInfo(&FileSys)))
    {
        DPRINT1("Mke2fs: Can't get media information\n");
        goto clean_up;
    }

    set_fs_defaults(NULL, &Ext2Sb, ClusterSize, &inode_ratio);

    Ext2Sb.s_blocks_count = FileSys.PartInfo.PartitionLength.QuadPart /
                            EXT2_BLOCK_SIZE(&Ext2Sb);


    /*
     * Calculate number of inodes based on the inode ratio
     */
    Ext2Sb.s_inodes_count =
        (ULONG)(((LONGLONG) Ext2Sb.s_blocks_count * EXT2_BLOCK_SIZE(&Ext2Sb)) / inode_ratio);

    /*
     * Calculate number of blocks to reserve
     */
    Ext2Sb.s_r_blocks_count = (Ext2Sb.s_blocks_count * 5) / 100;


    Status = Ext2LockVolume(&FileSys);
    if (NT_SUCCESS(Status))
    {
        bLocked = TRUE;
    }


    // Initialize 
    if (!ext2_initialize_sb(&FileSys))
    {
        DPRINT1("Mke2fs: error...\n");
        goto clean_up;
    }


    zap_sector(&FileSys, 2, 6);

    /*
     * Generate a UUID for it...
     */
    {
        __u8  uuid[16];
        uuid_generate(&uuid[0]);
        memcpy(&Ext2Sb.s_uuid[0], &uuid[0], 16);
    }

    /*
     * Add "jitter" to the superblock's check interval so that we
     * don't check all the filesystems at the same time.  We use a
     * kludgy hack of using the UUID to derive a random jitter value.
     */
    {
        int i, val;

        for (i = 0, val = 0 ; i < sizeof(Ext2Sb.s_uuid); i++)
            val += Ext2Sb.s_uuid[i];

        Ext2Sb.s_max_mnt_count += val % EXT2_DFL_MAX_MNT_COUNT;
    }

    /*
     * Set the volume label...
     */
    if (Label)
    {
        ANSI_STRING ansi_label;
        ansi_label.MaximumLength = sizeof(Ext2Sb.s_volume_name);
        ansi_label.Length = 0;
        ansi_label.Buffer = Ext2Sb.s_volume_name;
        RtlUnicodeStringToAnsiString(&ansi_label, Label, FALSE);
    }

    ext2_print_super(&Ext2Sb);

    bRet = ext2_allocate_tables(&FileSys);

    if (!bRet)
    {
        goto clean_up;
    }

    /* rsv must be a power of two (64kB is MD RAID sb alignment) */
    ULONG rsv = 65536 / FileSys.blocksize;
    ULONG blocks = Ext2Sb.s_blocks_count;
    ULONG start;
    ULONG ret_blk;

#ifdef ZAP_BOOTBLOCK
    zap_sector(&FileSys, 0, 2);
#endif

    /*
     * Wipe out any old MD RAID (or other) metadata at the end
     * of the device.  This will also verify that the device is
     * as large as we think.  Be careful with very small devices.
     */

    start = (blocks & ~(rsv - 1));
    if (start > rsv)
        start -= rsv;

    if (start > 0)
        bRet = zero_blocks(&FileSys, start, blocks - start, &ret_blk, NULL);

    if (!bRet)
    {
        DPRINT1("Mke2fs: zeroing block %lu at end of filesystem", ret_blk);
        goto clean_up;
    }

    write_inode_tables(&FileSys);

    create_root_dir(&FileSys);
    create_lost_and_found(&FileSys);

    ext2_reserve_inodes(&FileSys);

    create_bad_block_inode(&FileSys, NULL);

    DPRINT("Mke2fs: Writing superblocks and filesystem accounting information ... \n");

    if (!QuickFormat)
    {
        DPRINT1("Mke2fs: Slow format not supported yet\n");
    }

    if (!ext2_flush(&FileSys))
    {
        bRet = false;
        DPRINT1("Mke2fs: Warning, had trouble writing out superblocks.\n");
        goto clean_up;
    }

    DPRINT("Mke2fs: Writing superblocks and filesystem accounting information done!\n");

    bRet = true;
    Status = STATUS_SUCCESS;

clean_up:

    // Clean up ...
    ext2_free_group_desc(&FileSys);

    ext2_free_block_bitmap(&FileSys);
    ext2_free_inode_bitmap(&FileSys);

    if (!bRet)
    {
        Ext2DisMountVolume(&FileSys);
    }
    else
    {
        if(bLocked)
        {
            Ext2UnLockVolume(&FileSys);
        }
    }

    Ext2CloseDevice(&FileSys);

    Callback(DONE, 0, (PVOID)&bRet);

    return Status;
}

NTSTATUS WINAPI
Ext2Chkdsk(
	IN PUNICODE_STRING DriveRoot,
	IN BOOLEAN FixErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PFMIFSCALLBACK Callback)
{
	UNIMPLEMENTED;
	return STATUS_SUCCESS;
}
