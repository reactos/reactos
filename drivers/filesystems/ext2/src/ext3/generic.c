/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             generic.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"
#ifdef __REACTOS__
#include "linux/ext4.h"
#else
#include "linux\ext4.h"
#endif

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/


/* FUNCTIONS ***************************************************************/

NTSTATUS
Ext2LoadSuper(IN PEXT2_VCB      Vcb,
              IN BOOLEAN        bVerify,
              OUT PEXT2_SUPER_BLOCK * Sb)
{
    NTSTATUS          Status;
    PEXT2_SUPER_BLOCK Ext2Sb = NULL;

    Ext2Sb = (PEXT2_SUPER_BLOCK)
             Ext2AllocatePool(
                 PagedPool,
                 SUPER_BLOCK_SIZE,
                 EXT2_SB_MAGIC
             );
    if (!Ext2Sb) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    Status = Ext2ReadDisk(
                 Vcb,
                 (ULONGLONG) SUPER_BLOCK_OFFSET,
                 SUPER_BLOCK_SIZE,
                 (PVOID) Ext2Sb,
                 bVerify );

    if (!NT_SUCCESS(Status)) {
        Ext2FreePool(Ext2Sb, EXT2_SB_MAGIC);
        Ext2Sb = NULL;
    }

errorout:

    *Sb = Ext2Sb;
    return Status;
}


BOOLEAN
Ext2SaveSuper(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
)
{
    LONGLONG    offset;
    BOOLEAN     rc;

    offset = (LONGLONG) SUPER_BLOCK_OFFSET;
    rc = Ext2SaveBuffer( IrpContext,
                         Vcb,
                         offset,
                         SUPER_BLOCK_SIZE,
                         Vcb->SuperBlock
                       );

    if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        Ext2StartFloppyFlushDpc(Vcb, NULL, NULL);
    }

    return rc;
}


BOOLEAN
Ext2RefreshSuper (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
)
{
    LONGLONG        offset;
    IO_STATUS_BLOCK iosb;

    offset = (LONGLONG) SUPER_BLOCK_OFFSET;
    if (!CcCopyRead(
                Vcb->Volume,
                (PLARGE_INTEGER)&offset,
                SUPER_BLOCK_SIZE,
                TRUE,
                (PVOID)Vcb->SuperBlock,
                &iosb )) {
        return FALSE;
    }

    if (!NT_SUCCESS(iosb.Status)) {
        return FALSE;
    }

    /* reload root inode */
    if (Vcb->McbTree) {

        if (!Ext2LoadInode(Vcb, &Vcb->McbTree->Inode))
            return FALSE;

        /* initializeroot node */
        Vcb->McbTree->CreationTime = Ext2NtTime(Vcb->McbTree->Inode.i_ctime);
        Vcb->McbTree->LastAccessTime = Ext2NtTime(Vcb->McbTree->Inode.i_atime);
        Vcb->McbTree->LastWriteTime = Ext2NtTime(Vcb->McbTree->Inode.i_mtime);
        Vcb->McbTree->ChangeTime = Ext2NtTime(Vcb->McbTree->Inode.i_mtime);
    }

    return TRUE;
}

VOID
Ext2DropGroupBH(IN PEXT2_VCB Vcb)
{
    struct ext3_sb_info *sbi = &Vcb->sbi;
    unsigned long i;

    if (NULL == Vcb->sbi.s_gd) {
        return;
    }

    for (i = 0; i < Vcb->sbi.s_gdb_count; i++) {
        if (Vcb->sbi.s_gd[i].bh) {
            fini_bh(&sbi->s_gd[i].bh);
            Vcb->sbi.s_gd[i].bh = NULL;
        }
    }
}

VOID
Ext2PutGroup(IN PEXT2_VCB Vcb)
{
    struct ext3_sb_info *sbi = &Vcb->sbi;
    unsigned long i;


    if (NULL == Vcb->sbi.s_gd) {
        return;
    }

    Ext2DropGroupBH(Vcb);

    kfree(Vcb->sbi.s_gd);
    Vcb->sbi.s_gd = NULL;

    ClearFlag(Vcb->Flags, VCB_GD_LOADED);
}


BOOLEAN
Ext2LoadGroupBH(IN PEXT2_VCB Vcb)
{
    struct super_block  *sb = &Vcb->sb;
    struct ext3_sb_info *sbi = &Vcb->sbi;
    unsigned long i;
    BOOLEAN rc = FALSE;

    _SEH2_TRY {

        ExAcquireResourceExclusiveLite(&Vcb->sbi.s_gd_lock, TRUE);
        ASSERT (NULL != sbi->s_gd);

        for (i = 0; i < sbi->s_gdb_count; i++) {
            ASSERT (sbi->s_gd[i].block);
            if (sbi->s_gd[i].bh)
                continue;
            sbi->s_gd[i].bh = sb_getblk(sb, sbi->s_gd[i].block);
            if (!sbi->s_gd[i].bh) {
                DEBUG(DL_ERR, ("Ext2LoadGroupBH: can't read group descriptor %d\n", i));
                DbgBreak();
                _SEH2_LEAVE;
            }
            sbi->s_gd[i].gd = (struct ext4_group_desc *)sbi->s_gd[i].bh->b_data;
        }

        rc = TRUE;

    } _SEH2_FINALLY {

        ExReleaseResourceLite(&Vcb->sbi.s_gd_lock);
    } _SEH2_END;

    return rc;
}


BOOLEAN
Ext2LoadGroup(IN PEXT2_VCB Vcb)
{
    struct super_block  *sb = &Vcb->sb;
    struct ext3_sb_info *sbi = &Vcb->sbi;
    ext3_fsblk_t sb_block = 1;
    unsigned long i;
    BOOLEAN rc = FALSE;

    _SEH2_TRY {

        ExAcquireResourceExclusiveLite(&Vcb->sbi.s_gd_lock, TRUE);

        if (NULL == sbi->s_gd) {
            sbi->s_gd = kzalloc(sbi->s_gdb_count * sizeof(struct ext3_gd),
                                        GFP_KERNEL);
        }
        if (sbi->s_gd == NULL) {
            DEBUG(DL_ERR, ("Ext2LoadGroup: not enough memory.\n"));
            _SEH2_LEAVE;
        }

        if (BLOCK_SIZE != EXT3_MIN_BLOCK_SIZE) {
            sb_block = EXT4_MIN_BLOCK_SIZE / BLOCK_SIZE;
        }

        for (i = 0; i < sbi->s_gdb_count; i++) {
            sbi->s_gd[i].block =  descriptor_loc(sb, sb_block, i);
            if (!sbi->s_gd[i].block) {
                DEBUG(DL_ERR, ("Ext2LoadGroup: can't locate group descriptor %d\n", i));
                _SEH2_LEAVE;
            }
        }

        if (!Ext2LoadGroupBH(Vcb)) {
            DEBUG(DL_ERR, ("Ext2LoadGroup: Failed to load group descriptions !\n"));
            _SEH2_LEAVE;
        }

        if (!ext4_check_descriptors(sb)) {
            DbgBreak();
            DEBUG(DL_ERR, ("Ext2LoadGroup: group descriptors corrupted !\n"));
            _SEH2_LEAVE;
        }

        SetFlag(Vcb->Flags, VCB_GD_LOADED);
        rc = TRUE;

    } _SEH2_FINALLY {

        if (!rc)
            Ext2PutGroup(Vcb);

        ExReleaseResourceLite(&Vcb->sbi.s_gd_lock);
    } _SEH2_END;

    return rc;
}

VOID
Ext2DropBH(IN PEXT2_VCB Vcb)
{
    struct ext3_sb_info *sbi = &Vcb->sbi;

    /* do nothing if Vcb is not initialized yet */
    if (!IsFlagOn(Vcb->Flags, VCB_INITIALIZED))
        return;

    _SEH2_TRY {

        /* acquire bd lock to avoid bh creation */
        ExAcquireResourceExclusiveLite(&Vcb->bd.bd_bh_lock, TRUE);

        SetFlag(Vcb->Flags, VCB_BEING_DROPPED);
        Ext2DropGroupBH(Vcb);

        while (!IsListEmpty(&Vcb->bd.bd_bh_free)) {
            struct buffer_head *bh;
            PLIST_ENTRY         l;
            l = RemoveHeadList(&Vcb->bd.bd_bh_free);
            bh = CONTAINING_RECORD(l, struct buffer_head, b_link);
            InitializeListHead(&bh->b_link);
            if (0 == atomic_read(&bh->b_count)) {
                buffer_head_remove(&Vcb->bd, bh);
                free_buffer_head(bh);
            }
        }

    } _SEH2_FINALLY {
        ExReleaseResourceLite(&Vcb->bd.bd_bh_lock);
    } _SEH2_END;

    ClearFlag(Vcb->Flags, VCB_BEING_DROPPED);
}


VOID
Ext2FlushRange(IN PEXT2_VCB Vcb, LARGE_INTEGER s, LARGE_INTEGER e)
{
    ULONG len;

    if (e.QuadPart <= s.QuadPart)
        return;

    /* loop per 2G */
    while (s.QuadPart < e.QuadPart) {
        if (e.QuadPart > s.QuadPart + 1024 * 1024 * 1024) {
            len = 1024 * 1024 * 1024;
        } else {
            len = (ULONG) (e.QuadPart - s.QuadPart);
        }
        CcFlushCache(&Vcb->SectionObject, &s, len, NULL);
        s.QuadPart += len;
    }
}

NTSTATUS
Ext2FlushVcb(IN PEXT2_VCB Vcb)
{
    LARGE_INTEGER        s = {0}, o;
    struct ext3_sb_info *sbi = &Vcb->sbi;
    struct rb_node      *node;
    struct buffer_head  *bh;

    if (!IsFlagOn(Vcb->Flags, VCB_GD_LOADED)) {
        CcFlushCache(&Vcb->SectionObject, NULL, 0, NULL);
        goto errorout;
    }

    ASSERT(ExIsResourceAcquiredExclusiveLite(&Vcb->MainResource));

    _SEH2_TRY {

        /* acqurie gd block */
        ExAcquireResourceExclusiveLite(&Vcb->sbi.s_gd_lock, TRUE);

        /* acquire bd lock to avoid bh creation */
        ExAcquireResourceExclusiveLite(&Vcb->bd.bd_bh_lock, TRUE);

        /* drop unused bh */
        Ext2DropBH(Vcb);

        /* flush volume with all outstanding bh skipped */

        node = rb_first(&Vcb->bd.bd_bh_root);
        while (node) {

            bh = container_of(node, struct buffer_head, b_rb_node);
            node = rb_next(node);

            o.QuadPart = bh->b_blocknr << BLOCK_BITS;
            ASSERT(o.QuadPart >= s.QuadPart);

            if (o.QuadPart == s.QuadPart) {
                s.QuadPart = s.QuadPart + bh->b_size;
                continue;
            }

            if (o.QuadPart > s.QuadPart) {
                Ext2FlushRange(Vcb, s, o);
                s.QuadPart = (bh->b_blocknr << BLOCK_BITS) + bh->b_size;
                continue;
            }
        }

        o = Vcb->PartitionInformation.PartitionLength;
        Ext2FlushRange(Vcb, s, o);

    } _SEH2_FINALLY {

        ExReleaseResourceLite(&Vcb->bd.bd_bh_lock);
        ExReleaseResourceLite(&Vcb->sbi.s_gd_lock);
    } _SEH2_END;

errorout:
    return STATUS_SUCCESS;
}


BOOLEAN
Ext2SaveGroup(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Group
)
{
    struct ext4_group_desc *gd;
    struct buffer_head     *gb = NULL;
    unsigned long i;

    gd = ext4_get_group_desc(&Vcb->sb, Group, &gb);
    if (!gd)
        return 0;

    gd->bg_checksum = ext4_group_desc_csum(&Vcb->sbi, Group, gd);
    mark_buffer_dirty(gb);
    fini_bh(&gb);

    return TRUE;
}


BOOLEAN
Ext2RefreshGroup(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
)
{
    return TRUE;
}

BOOLEAN
Ext2GetInodeLba (
    IN PEXT2_VCB    Vcb,
    IN  ULONG       inode,
    OUT PLONGLONG   offset
)
{
    PEXT2_GROUP_DESC gd;
    struct buffer_head *bh = NULL;
    ext4_fsblk_t loc;
    int group;

    if (inode < 1 || inode > INODES_COUNT)  {
        DEBUG(DL_ERR, ( "Ext2GetInodeLba: Inode value %xh is invalid.\n",inode));
        *offset = 0;
        return FALSE;
    }

    group = (inode - 1) / INODES_PER_GROUP ;
    gd = ext4_get_group_desc(&Vcb->sb, group, &bh);
    if (!bh) {
        *offset = 0;
        DbgBreak();
        return FALSE;
    }
    loc = (LONGLONG)ext4_inode_table(&Vcb->sb, gd);
    loc = loc << BLOCK_BITS;
    loc = loc + ((inode - 1) % INODES_PER_GROUP) * Vcb->InodeSize;

    *offset = loc;
    __brelse(bh);

    return TRUE;
}

void Ext2DecodeInode(struct inode *dst, struct ext3_inode *src)
{
    dst->i_mode = src->i_mode;
    dst->i_flags = src->i_flags;
    dst->i_uid = src->i_uid;
    dst->i_gid = src->i_gid;
    dst->i_nlink = src->i_links_count;
    dst->i_generation = src->i_generation;
    dst->i_size = src->i_size;
    if (S_ISREG(src->i_mode)) {
        dst->i_size |= (loff_t)src->i_size_high << 32;
    }
    dst->i_file_acl = src->i_file_acl_lo;
    dst->i_file_acl |= (ext4_fsblk_t)src->osd2.linux2.l_i_file_acl_high << 32;
    dst->i_atime = src->i_atime;
    dst->i_ctime = src->i_ctime;
    dst->i_mtime = src->i_mtime;
    dst->i_dtime = src->i_dtime;
    dst->i_blocks = ext3_inode_blocks(src, dst);
    memcpy(&dst->i_block[0], &src->i_block[0], sizeof(__u32) * 15);
    if (EXT3_HAS_RO_COMPAT_FEATURE(dst->i_sb,
                                   EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE))
        dst->i_extra_isize = src->i_extra_isize;
    else
        dst->i_extra_isize = 0;
}

void Ext2EncodeInode(struct ext3_inode *dst,  struct inode *src)
{
    dst->i_mode = src->i_mode;
    dst->i_flags = src->i_flags;
    dst->i_uid = src->i_uid;
    dst->i_gid = src->i_gid;
    dst->i_links_count = src->i_nlink;
    dst->i_generation = src->i_generation;
    dst->i_size = (__u32)src->i_size;
    if (S_ISREG(src->i_mode)) {
        dst->i_size_high = (__u32)(src->i_size >> 32);
    }
    dst->i_file_acl_lo = (__u32)src->i_file_acl;
    dst->osd2.linux2.l_i_file_acl_high |= (__u16)(src->i_file_acl >> 32);
    dst->i_atime = src->i_atime;
    dst->i_ctime = src->i_ctime;
    dst->i_mtime = src->i_mtime;
    dst->i_dtime = src->i_dtime;
    dst->i_extra_isize = src->i_extra_isize;
    ASSERT(src->i_sb);
    ext3_inode_blocks_set(dst, src);
    memcpy(&dst->i_block[0], &src->i_block[0], sizeof(__u32) * 15);
    if (EXT3_HAS_RO_COMPAT_FEATURE(src->i_sb,
                                   EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE))
        dst->i_extra_isize = src->i_extra_isize;
}


BOOLEAN
Ext2LoadInode (IN PEXT2_VCB Vcb,
               IN struct inode *Inode)
{
    struct ext3_inode   ext3i = {0};
    LONGLONG            offset;

    if (!Ext2GetInodeLba(Vcb, Inode->i_ino, &offset))  {
        DEBUG(DL_ERR, ("Ext2LoadInode: failed inode %u.\n", Inode->i_ino));
        return FALSE;
    }

    if (!Ext2LoadBuffer(NULL, Vcb, offset, sizeof(ext3i), &ext3i)) {
        return FALSE;
    }

    Ext2DecodeInode(Inode, &ext3i);

    return TRUE;
}


BOOLEAN
Ext2ClearInode (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN ULONG Inode)
{
    LONGLONG            Offset = 0;
    BOOLEAN             rc;

    rc = Ext2GetInodeLba(Vcb, Inode, &Offset);
    if (!rc)  {
        DEBUG(DL_ERR, ( "Ext2SaveInode: failed inode %u.\n", Inode));
        goto errorout;
    }

    rc = Ext2ZeroBuffer(IrpContext, Vcb, Offset, Vcb->InodeSize);

errorout:

    return rc;
}

BOOLEAN
Ext2SaveInode ( IN PEXT2_IRP_CONTEXT IrpContext,
                IN PEXT2_VCB Vcb,
                IN struct inode *Inode)
{
    struct ext3_inode   ext3i = {0};

    LONGLONG            Offset = 0;
    ULONG               InodeSize = sizeof(ext3i);
    BOOLEAN             rc = 0;

    DEBUG(DL_INF, ( "Ext2SaveInode: Saving Inode %xh: Mode=%xh Size=%xh\n",
                    Inode->i_ino, Inode->i_mode, Inode->i_size));
    rc = Ext2GetInodeLba(Vcb,  Inode->i_ino, &Offset);
    if (!rc)  {
        DEBUG(DL_ERR, ( "Ext2SaveInode: failed inode %u.\n", Inode->i_ino));
        goto errorout;
    }

    rc = Ext2LoadBuffer(NULL, Vcb, Offset, InodeSize, &ext3i);
    if (!rc) {
        DEBUG(DL_ERR, ( "Ext2SaveInode: failed reading inode %u.\n", Inode->i_ino));
        goto errorout;;
    }

    Ext2EncodeInode(&ext3i, Inode);
    if (InodeSize > Vcb->InodeSize)
        InodeSize = Vcb->InodeSize;
    rc = Ext2SaveBuffer(IrpContext, Vcb, Offset, InodeSize, &ext3i);

    if (rc && IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        Ext2StartFloppyFlushDpc(Vcb, NULL, NULL);
    }

errorout:
    return rc;
}

BOOLEAN
Ext2LoadInodeXattr(IN PEXT2_VCB Vcb,
	IN struct inode *Inode,
	IN PEXT2_INODE InodeXattr)
{
	IO_STATUS_BLOCK     IoStatus;
	LONGLONG            Offset;

	if (!Ext2GetInodeLba(Vcb, Inode->i_ino, &Offset)) {
		DEBUG(DL_ERR, ("Ext2LoadRawInode: error get inode(%xh)'s addr.\n", Inode->i_ino));
		return FALSE;
	}

	if (!CcCopyRead(
		Vcb->Volume,
		(PLARGE_INTEGER)&Offset,
		Vcb->InodeSize,
		PIN_WAIT,
		(PVOID)InodeXattr,
		&IoStatus)) {
		return FALSE;
	}

	if (!NT_SUCCESS(IoStatus.Status)) {
		return FALSE;
	}

	Ext2EncodeInode(InodeXattr, Inode);
	return TRUE;
}

BOOLEAN
Ext2SaveInodeXattr(IN PEXT2_IRP_CONTEXT IrpContext,
	IN PEXT2_VCB Vcb,
	IN struct inode *Inode,
	IN PEXT2_INODE InodeXattr)
{
	IO_STATUS_BLOCK     IoStatus;
	LONGLONG            Offset = 0;
	ULONG               InodeSize = Vcb->InodeSize;
	BOOLEAN             rc = 0;

	/* There is no way to put EA information in such a small inode */
	if (InodeSize == EXT2_GOOD_OLD_INODE_SIZE)
		return FALSE;

	DEBUG(DL_INF, ("Ext2SaveInodeXattr: Saving Inode %xh: Mode=%xh Size=%xh\n",
		Inode->i_ino, Inode->i_mode, Inode->i_size));
	rc = Ext2GetInodeLba(Vcb, Inode->i_ino, &Offset);
	if (!rc) {
		DEBUG(DL_ERR, ("Ext2SaveInodeXattr: error get inode(%xh)'s addr.\n", Inode->i_ino));
		goto errorout;
	}

	rc = Ext2SaveBuffer(IrpContext,
									Vcb,
									Offset + EXT2_GOOD_OLD_INODE_SIZE + Inode->i_extra_isize,
									InodeSize - EXT2_GOOD_OLD_INODE_SIZE - Inode->i_extra_isize,
									(char *)InodeXattr + EXT2_GOOD_OLD_INODE_SIZE + Inode->i_extra_isize);

	if (rc && IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
		Ext2StartFloppyFlushDpc(Vcb, NULL, NULL);
	}

errorout:
	return rc;
}


BOOLEAN
Ext2LoadBlock (IN PEXT2_VCB Vcb,
               IN ULONG     Index,
               IN PVOID     Buffer )
{
    struct buffer_head *bh = NULL;
    BOOLEAN             rc = 0;

    _SEH2_TRY {

        bh = sb_getblk(&Vcb->sb, (sector_t)Index);

        if (!bh) {
            DEBUG(DL_ERR, ("Ext2Loadblock: can't load block %u\n", Index));
            DbgBreak();
            _SEH2_LEAVE;
        }

        if (!buffer_uptodate(bh)) {
            int err = bh_submit_read(bh);
	        if (err < 0) {
	            DEBUG(DL_ERR, ("Ext2LoadBlock: reading failed %d\n", err));
		        _SEH2_LEAVE;
	        }
        }

        RtlCopyMemory(Buffer, bh->b_data, BLOCK_SIZE);
        rc = TRUE;

    } _SEH2_FINALLY {

        if (bh)
            fini_bh(&bh);
    } _SEH2_END;

    return rc;
}


BOOLEAN
Ext2SaveBlock ( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN ULONG                Index,
                IN PVOID                Buf )
{
    struct buffer_head *bh = NULL;
    BOOLEAN             rc = 0;

    _SEH2_TRY {

        bh = sb_getblk_zero(&Vcb->sb, (sector_t)Index);

        if (!bh) {
            DEBUG(DL_ERR, ("Ext2Saveblock: can't load block %u\n", Index));
            DbgBreak();
            _SEH2_LEAVE;
        }

        if (!buffer_uptodate(bh)) {
        }

        RtlCopyMemory(bh->b_data, Buf, BLOCK_SIZE);
        mark_buffer_dirty(bh);
        rc = TRUE;

    } _SEH2_FINALLY {

        if (bh)
            fini_bh(&bh);
    } _SEH2_END;

    return rc;
}

BOOLEAN
Ext2LoadBuffer( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN LONGLONG             offset,
                IN ULONG                size,
                IN PVOID                buf )
{
    struct buffer_head *bh = NULL;
    BOOLEAN             rc;

    _SEH2_TRY {

        while (size) {

            sector_t    block;
            ULONG       len = 0, delta = 0;

            block = (sector_t) (offset >> BLOCK_BITS);
            delta = (ULONG)offset & (BLOCK_SIZE - 1);
            len = BLOCK_SIZE - delta;
            if (size < len)
                len = size;

            bh = sb_getblk(&Vcb->sb, block);
            if (!bh) {
                DEBUG(DL_ERR, ("Ext2SaveBuffer: can't load block %I64u\n", block));
                DbgBreak();
                _SEH2_LEAVE;
            }

            if (!buffer_uptodate(bh)) {
	            int err = bh_submit_read(bh);
	            if (err < 0) {
		            DEBUG(DL_ERR, ("Ext2SaveBuffer: bh_submit_read failed: %d\n", err));
		            _SEH2_LEAVE;
	            }
            }

            _SEH2_TRY {
                RtlCopyMemory(buf, bh->b_data + delta, len);
            } _SEH2_FINALLY {
                fini_bh(&bh);
            } _SEH2_END;

            buf = (PUCHAR)buf + len;
            offset = offset + len;
            size = size - len;
        }

        rc = TRUE;

    } _SEH2_FINALLY {

        if (bh)
            fini_bh(&bh);

    } _SEH2_END;

    return rc;
}


BOOLEAN
Ext2ZeroBuffer( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN LONGLONG             offset,
                IN ULONG                size
    )
{
    struct buffer_head *bh = NULL;
    BOOLEAN             rc = 0;

    _SEH2_TRY {

        while (size) {

            sector_t    block;
            ULONG       len = 0, delta = 0;

            block = (sector_t) (offset >> BLOCK_BITS);
            delta = (ULONG)offset & (BLOCK_SIZE - 1);
            len = BLOCK_SIZE - delta;
            if (size < len)
                len = size;

            if (delta == 0 && len >= BLOCK_SIZE) {
                bh = sb_getblk_zero(&Vcb->sb, block);
            } else {
                bh = sb_getblk(&Vcb->sb, block);
            }

            if (!bh) {
                DEBUG(DL_ERR, ("Ext2SaveBuffer: can't load block %I64u\n", block));
                DbgBreak();
                _SEH2_LEAVE;
            }

            if (!buffer_uptodate(bh)) {
	            int err = bh_submit_read(bh);
	            if (err < 0) {
		            DEBUG(DL_ERR, ("Ext2SaveBuffer: bh_submit_read failed: %d\n", err));
		            _SEH2_LEAVE;
	            }
            }

            _SEH2_TRY {
                if (delta == 0 && len >= BLOCK_SIZE) {
                    /* bh (cache) was already cleaned as zero */
                } else {
                    RtlZeroMemory(bh->b_data + delta, len);
                }
                mark_buffer_dirty(bh);
            } _SEH2_FINALLY {
                fini_bh(&bh);
            } _SEH2_END;

            offset = offset + len;
            size = size - len;
        }

        rc = TRUE;

    } _SEH2_FINALLY {

        if (bh)
            fini_bh(&bh);

    } _SEH2_END;

    return rc;
}


#ifdef __REACTOS__
#define SIZE_256K 0x40000

BOOLEAN
Ext2SaveBuffer( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN LONGLONG             Offset,
                IN ULONG                Size,
                IN PVOID                Buf )
{
    BOOLEAN     rc;

    while (Size) {

        PBCB        Bcb;
        PVOID       Buffer;
        ULONG       Length;

        Length = (ULONG)Offset & (SIZE_256K - 1);
        Length = SIZE_256K - Length;
        if (Size < Length)
            Length = Size;

        if ( !CcPreparePinWrite(
                    Vcb->Volume,
                    (PLARGE_INTEGER) (&Offset),
                    Length,
                    FALSE,
                    PIN_WAIT | PIN_EXCLUSIVE,
                    &Bcb,
                    &Buffer )) {

            DEBUG(DL_ERR, ( "Ext2SaveBuffer: failed to PinLock offset %I64xh ...\n", Offset));
            return FALSE;
        }

        _SEH2_TRY {

            RtlCopyMemory(Buffer, Buf, Length);
            CcSetDirtyPinnedData(Bcb, NULL );
            SetFlag(Vcb->Volume->Flags, FO_FILE_MODIFIED);

            rc = Ext2AddVcbExtent(Vcb, Offset, (LONGLONG)Length);
            if (!rc) {
                DbgBreak();
                Ext2Sleep(100);
                rc = Ext2AddVcbExtent(Vcb, Offset, (LONGLONG)Length);
            }

        } _SEH2_FINALLY {
            CcUnpinData(Bcb);
        } _SEH2_END;

        Buf = (PUCHAR)Buf + Length;
        Offset = Offset + Length;
        Size = Size - Length;
    }

    return rc;
}
#else

BOOLEAN
Ext2SaveBuffer( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN LONGLONG             offset,
                IN ULONG                size,
                IN PVOID                buf )
{
    struct buffer_head *bh = NULL;
    BOOLEAN             rc = 0;

    __try {

        while (size) {

            sector_t    block;
            ULONG       len = 0, delta = 0;

            block = (sector_t) (offset >> BLOCK_BITS);
            delta = (ULONG)offset & (BLOCK_SIZE - 1);
            len = BLOCK_SIZE - delta;
            if (size < len)
                len = size;

            if (delta == 0 && len >= BLOCK_SIZE) {
                bh = sb_getblk_zero(&Vcb->sb, block);
            } else {
                bh = sb_getblk(&Vcb->sb, block);
            }

            if (!bh) {
                DEBUG(DL_ERR, ("Ext2SaveBuffer: can't load block %I64u\n", block));
                DbgBreak();
                __leave;
            }

            if (!buffer_uptodate(bh)) {
	            int err = bh_submit_read(bh);
	            if (err < 0) {
		            DEBUG(DL_ERR, ("Ext2SaveBuffer: bh_submit_read failed: %d\n", err));
		            __leave;
	            }
            }

            __try {
                RtlCopyMemory(bh->b_data + delta, buf, len);
                mark_buffer_dirty(bh);
            } __finally {
                fini_bh(&bh);
            }

            buf = (PUCHAR)buf + len;
            offset = offset + len;
            size = size - len;
        }

        rc = TRUE;

    } __finally {

        if (bh)
            fini_bh(&bh);

    }

    return rc;
}
#endif


VOID
Ext2UpdateVcbStat(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
)
{
    Vcb->SuperBlock->s_free_inodes_count = ext4_count_free_inodes(&Vcb->sb);
    ext3_free_blocks_count_set(SUPER_BLOCK, ext4_count_free_blocks(&Vcb->sb));
    Ext2SaveSuper(IrpContext, Vcb);
}

NTSTATUS
Ext2NewBlock(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                GroupHint,
    IN ULONG                BlockHint,
    OUT PULONG              Block,
    IN OUT PULONG           Number
)
{
    struct super_block      *sb = &Vcb->sb;
    PEXT2_GROUP_DESC        gd;
    struct buffer_head     *gb = NULL;
    struct buffer_head     *bh = NULL;
    ext4_fsblk_t            bitmap_blk;

    RTL_BITMAP              BlockBitmap;

    ULONG                   Group = 0;
    ULONG                   Index = 0xFFFFFFFF;
    ULONG                   dwHint = 0;
    ULONG                   Count = 0;
    ULONG                   Length = 0;

    NTSTATUS                Status = STATUS_DISK_FULL;

    *Block = 0;

    ExAcquireResourceExclusiveLite(&Vcb->MetaBlock, TRUE);

    /* validate the hint group and hint block */
    if (GroupHint >= Vcb->sbi.s_groups_count) {
        DbgBreak();
        GroupHint = Vcb->sbi.s_groups_count - 1;
    }

    if (BlockHint != 0) {
        GroupHint = (BlockHint - EXT2_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;
        dwHint = (BlockHint - EXT2_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;
    }

    Group = GroupHint;

Again:

    if (bh)
        fini_bh(&bh);

    if (gb)
        fini_bh(&gb);

    gd = ext4_get_group_desc(sb, Group, &gb);
    if (!gd) {
        DbgBreak();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    bitmap_blk = ext4_block_bitmap(sb, gd);

    if (gd->bg_flags & cpu_to_le16(EXT4_BG_BLOCK_UNINIT)) {
        bh = sb_getblk_zero(sb, bitmap_blk);
        if (!bh) {
            DbgBreak();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }
        gd->bg_checksum = ext4_group_desc_csum(EXT3_SB(sb), Group, gd);
        ext4_init_block_bitmap(sb, bh, Group, gd);
        set_buffer_uptodate(bh);
        gd->bg_flags &= cpu_to_le16(~EXT4_BG_BLOCK_UNINIT);
        Ext2SaveGroup(IrpContext, Vcb, Group);
    } else {
        bh = sb_getblk(sb, bitmap_blk);
        if (!bh) {
            DbgBreak();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }
    }

    if (!buffer_uptodate(bh)) {
	    int err = bh_submit_read(bh);
	    if (err < 0) {
		    DbgPrint("bh_submit_read error! err: %d\n", err);
		    Status = Ext2WinntError(err);
		    goto errorout;
	    }
    }

    if (ext4_free_blks_count(sb, gd)) {

        if (Group == Vcb->sbi.s_groups_count - 1) {

            Length = (ULONG)(TOTAL_BLOCKS % BLOCKS_PER_GROUP);

            /* s_blocks_count is integer multiple of s_blocks_per_group */
            if (Length == 0) {
                Length = BLOCKS_PER_GROUP;
            }
        } else {
            Length = BLOCKS_PER_GROUP;
        }

        /* initialize bitmap buffer */
        RtlInitializeBitMap(&BlockBitmap, (PULONG)bh->b_data, Length);

        /* try to find a clear bit range */
        Index = RtlFindClearBits(&BlockBitmap, *Number, dwHint);

        /* We could not get new block in the prefered group */
        if (Index == 0xFFFFFFFF) {

            /* search clear bits from the hint block */
            Count = RtlFindNextForwardRunClear(&BlockBitmap, dwHint, &Index);
            if (dwHint != 0 && Count == 0) {
                /* search clear bits from the very beginning */
                Count = RtlFindNextForwardRunClear(&BlockBitmap, 0, &Index);
            }

            if (Count == 0) {

                RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));

                /* no blocks found: set bg_free_blocks_count to 0 */
                ext4_free_blks_set(sb, gd, 0);
                Ext2SaveGroup(IrpContext, Vcb, Group);

                /* will try next group */
                goto Again;

            } else {

                /* we got free blocks */
                if (Count <= *Number) {
                    *Number = Count;
                }
            }
        }

    } else {

        /* try next group */
        dwHint = 0;
        Group = (Group + 1) % Vcb->sbi.s_groups_count;
        if (Group != GroupHint) {
            goto Again;
        }

        Index = 0xFFFFFFFF;
    }

    if (Index < Length) {

        /* mark block bits as allocated */
        RtlSetBits(&BlockBitmap, Index, *Number);

        /* set block bitmap dirty in cache */
        mark_buffer_dirty(bh);

        /* update group description */
        ext4_free_blks_set(sb, gd, RtlNumberOfClearBits(&BlockBitmap));
        Ext2SaveGroup(IrpContext, Vcb, Group);

        /* update Vcb free blocks */
        Ext2UpdateVcbStat(IrpContext, Vcb);

        /* validate the new allocated block number */
        *Block = Index + EXT2_FIRST_DATA_BLOCK + Group * BLOCKS_PER_GROUP;
        if (*Block >= TOTAL_BLOCKS || *Block + *Number > TOTAL_BLOCKS) {
            DbgBreak();
            dwHint = 0;
            goto Again;
        }

        if (ext4_block_bitmap(sb, gd) == *Block ||
            ext4_inode_bitmap(sb, gd) == *Block ||
            ext4_inode_table(sb,  gd)  == *Block ) {
            DbgBreak();
            dwHint = 0;
            goto Again;
        }

        /* Always remove dirty MCB to prevent Volume's lazy writing.
           Metadata blocks will be re-added during modifications.*/
        if (Ext2RemoveBlockExtent(Vcb, NULL, *Block, *Number)) {
        } else {
            DbgBreak();
            Ext2RemoveBlockExtent(Vcb, NULL, *Block, *Number);
        }

        DEBUG(DL_INF, ("Ext2NewBlock:  Block %xh - %x allocated.\n",
                       *Block, *Block + *Number));
        Status = STATUS_SUCCESS;
    }

errorout:

    ExReleaseResourceLite(&Vcb->MetaBlock);

    if (bh)
        fini_bh(&bh);

    if (gb)
        fini_bh(&gb);

    return Status;
}

NTSTATUS
Ext2FreeBlock(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Block,
    IN ULONG                Number
)
{
    struct super_block     *sb = &Vcb->sb;
    PEXT2_GROUP_DESC        gd;
    struct buffer_head     *gb = NULL;
    ext4_fsblk_t            bitmap_blk;

    RTL_BITMAP      BlockBitmap;
    LARGE_INTEGER   Offset;

    PBCB            BitmapBcb;
    PVOID           BitmapCache;

    ULONG           Group;
    ULONG           Index;
    ULONG           Length;
    ULONG           Count;

    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    ExAcquireResourceExclusiveLite(&Vcb->MetaBlock, TRUE);

    DEBUG(DL_INF, ("Ext2FreeBlock: Block %xh - %x to be freed.\n",
                   Block, Block + Number));

    Group = (Block - EXT2_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;
    Index = (Block - EXT2_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;

Again:

    if (gb)
        fini_bh(&gb);

    if ( Block < EXT2_FIRST_DATA_BLOCK ||
         Block >= TOTAL_BLOCKS ||
         Group >= Vcb->sbi.s_groups_count) {

        DbgBreak();
        Status = STATUS_SUCCESS;

    } else  {

        gd = ext4_get_group_desc(sb, Group, &gb);
        if (!gd) {
            DbgBreak();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }
        bitmap_blk = ext4_block_bitmap(sb, gd);

        /* check the block is valid or not */
        if (bitmap_blk >= TOTAL_BLOCKS) {
            DbgBreak();
            Status = STATUS_DISK_CORRUPT_ERROR;
            goto errorout;
        }

        /* get bitmap block offset and length */
        Offset.QuadPart = bitmap_blk;
        Offset.QuadPart = Offset.QuadPart << BLOCK_BITS;

        if (Group == Vcb->sbi.s_groups_count - 1) {

            Length = (ULONG)(TOTAL_BLOCKS % BLOCKS_PER_GROUP);

            /* s_blocks_count is integer multiple of s_blocks_per_group */
            if (Length == 0) {
                Length = BLOCKS_PER_GROUP;
            }

        } else {
            Length = BLOCKS_PER_GROUP;
        }

        /* read and initialize bitmap */
        if (!CcPinRead( Vcb->Volume,
                        &Offset,
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &BitmapBcb,
                        &BitmapCache ) ) {

            DEBUG(DL_ERR, ("Ext2FreeBlock: failed to PinLock bitmap block %xh.\n",
                           bitmap_blk));
            Status = STATUS_CANT_WAIT;
            DbgBreak();
            goto errorout;
        }

        /* clear usused bits */
        RtlInitializeBitMap(&BlockBitmap, BitmapCache, Length);
        Count = min(Length - Index, Number);
        RtlClearBits(&BlockBitmap, Index, Count);

        /* update group description table */
        ext4_free_blks_set(sb, gd, RtlNumberOfClearBits(&BlockBitmap));

        /* indict the cache range is dirty */
        CcSetDirtyPinnedData(BitmapBcb, NULL );
        Ext2AddVcbExtent(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);
        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        BitmapCache = NULL;
        Ext2SaveGroup(IrpContext, Vcb, Group);

        /* remove dirty MCB to prevent Volume's lazy writing. */
        if (Ext2RemoveBlockExtent(Vcb, NULL, Block, Count)) {
        } else {
            DbgBreak();
            Ext2RemoveBlockExtent(Vcb, NULL, Block, Count);
        }

        /* save super block (used/unused blocks statics) */
        Ext2UpdateVcbStat(IrpContext, Vcb);

        /* try next group to clear all remaining */
        Number -= Count;
        if (Number) {
            Group += 1;
            if (Group < Vcb->sbi.s_groups_count) {
                Index = 0;
                Block += Count;
                goto Again;
            } else {
                DEBUG(DL_ERR, ("Ext2FreeBlock: block number beyonds max group.\n"));
                goto errorout;
            }
        }
    }

    Status = STATUS_SUCCESS;

errorout:

    if (gb)
        fini_bh(&gb);

    ExReleaseResourceLite(&Vcb->MetaBlock);

    return Status;
}


NTSTATUS
Ext2NewInode(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                GroupHint,
    IN ULONG                Type,
    OUT PULONG              Inode
)
{
    struct super_block     *sb = &Vcb->sb;
    PEXT2_GROUP_DESC        gd;
    struct buffer_head     *gb = NULL;
    struct buffer_head     *bh = NULL;
    ext4_fsblk_t            bitmap_blk;

    RTL_BITMAP      InodeBitmap;

    ULONG           Group, i, j;
    ULONG           Average, Length;

    ULONG           dwInode;

    NTSTATUS        Status = STATUS_DISK_FULL;

    *Inode = dwInode = 0XFFFFFFFF;

    ExAcquireResourceExclusiveLite(&Vcb->MetaInode, TRUE);

    if (GroupHint >= Vcb->sbi.s_groups_count)
        GroupHint = GroupHint % Vcb->sbi.s_groups_count;

repeat:

    if (bh)
        fini_bh(&bh);

    if (gb)
        fini_bh(&gb);

    Group = i = 0;
    gd = NULL;

    if (Type == EXT2_FT_DIR) {

        Average = Vcb->SuperBlock->s_free_inodes_count / Vcb->sbi.s_groups_count;

        for (j = 0; j < Vcb->sbi.s_groups_count; j++) {

            i = (j + GroupHint) % (Vcb->sbi.s_groups_count);
            gd = ext4_get_group_desc(sb, i, &gb);
            if (!gd) {
                    DbgBreak();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }

            if ((gd->bg_flags & cpu_to_le16(EXT4_BG_INODE_UNINIT)) ||
                (ext4_used_dirs_count(sb, gd) << 8 <
                 ext4_free_inodes_count(sb, gd)) ) {
                Group = i + 1;
                break;
            }
            fini_bh(&gb);
        }

        if (!Group) {

            PEXT2_GROUP_DESC  desc = NULL;

            gd = NULL;

            /* get the group with the biggest vacancy */
            for (j = 0; j < Vcb->sbi.s_groups_count; j++) {

                struct buffer_head *gt = NULL;
                desc = ext4_get_group_desc(sb, j, &gt);
                if (!desc) {
                    DbgBreak();
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto errorout;
                }

                /* return the group if it's not initialized yet */
                if (desc->bg_flags & cpu_to_le16(EXT4_BG_INODE_UNINIT)) {
                    Group = j + 1;
                    gd = desc;

                    if (gb)
                        fini_bh(&gb);
                    gb = gt;
                    gt = NULL;
                    break;
                }

                if (!gd) {
                    if (ext4_free_inodes_count(sb, desc) > 0) {
                        Group = j + 1;
                        gd = desc;
                        if (gb)
                            fini_bh(&gb);
                        gb = gt;
                        gt = NULL;
                    }
                } else {
                    if (ext4_free_inodes_count(sb, desc) >
                        ext4_free_inodes_count(sb, gd)) {
                        Group = j + 1;
                        gd = desc;
                        if (gb)
                            fini_bh(&gb);
                        gb = gt;
                        gt = NULL;
                        break;
                    }
                }
                if (gt)
                    fini_bh(&gt);
            }
        }

    } else {

        /*
         * Try to place the inode in its parent directory (GroupHint)
         */

        gd = ext4_get_group_desc(sb, GroupHint, &gb);
        if (!gb) {
            DbgBreak();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        if (gd->bg_flags & cpu_to_le16(EXT4_BG_INODE_UNINIT) ||
            ext4_free_inodes_count(sb, gd)) {

            Group = GroupHint + 1;

        } else {

            /* this group is 100% cocucpied */
            fini_bh(&gb);

            i = GroupHint;

            /*
             * Use a quadratic hash to find a group with a free inode
             */

            for (j = 1; j < Vcb->sbi.s_groups_count; j <<= 1) {


                i = (i + j) % Vcb->sbi.s_groups_count;
                gd = ext4_get_group_desc(sb, i, &gb);
                if (!gd) {
                    DbgBreak();
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto errorout;
                }

                if (gd->bg_flags & cpu_to_le16(EXT4_BG_INODE_UNINIT) ||
                    ext4_free_inodes_count(sb, gd)) {
                    Group = i + 1;
                    break;
                }

                fini_bh(&gb);
            }
        }

        if (!Group) {
            /*
             * That failed: try linear search for a free inode
             */
            i = GroupHint;
            for (j = 2; j < Vcb->sbi.s_groups_count; j++) {

                i = (i + 1) % Vcb->sbi.s_groups_count;
                gd = ext4_get_group_desc(sb, i, &gb);
                if (!gd) {
                    DbgBreak();
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto errorout;
                }

                if (gd->bg_flags & cpu_to_le16(EXT4_BG_INODE_UNINIT) ||
                    ext4_free_inodes_count(sb, gd)) {
                    Group = i + 1;
                    break;
                }

                fini_bh(&gb);
            }
        }
    }

    if (gd == NULL || Group == 0) {
        goto errorout;
    }

    /* finally we got the group, but is it valid ? */
    if (Group > Vcb->sbi.s_groups_count) {
        DbgBreak();
        goto errorout;
    }

    /* valid group number starts from 1, not 0 */
    Group -= 1;

    ASSERT(gd);
    bitmap_blk = ext4_inode_bitmap(sb, gd);
    /* check the block is valid or not */
    if (bitmap_blk == 0 || bitmap_blk >= TOTAL_BLOCKS) {
        DbgBreak();
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto errorout;
    }

    if (gd->bg_flags & cpu_to_le16(EXT4_BG_INODE_UNINIT)) {
        bh = sb_getblk_zero(sb, bitmap_blk);
        if (!bh) {
            DbgBreak();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }
        gd->bg_checksum = ext4_group_desc_csum(EXT3_SB(sb), Group, gd);
        ext4_init_inode_bitmap(sb, bh, Group, gd);
        set_buffer_uptodate(bh);
        gd->bg_flags &= cpu_to_le16(~EXT4_BG_INODE_UNINIT);
        Ext2SaveGroup(IrpContext, Vcb, Group);
    } else {
        bh = sb_getblk(sb, bitmap_blk);
        if (!bh) {
            DbgBreak();
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }
    }

    if (!buffer_uptodate(bh)) {
	    int err = bh_submit_read(bh);
	    if (err < 0) {
		    DbgPrint("bh_submit_read error! err: %d\n", err);
		    Status = Ext2WinntError(err);
		    goto errorout;
	    }
    }

    if (Vcb->sbi.s_groups_count == 1) {
        Length = INODES_COUNT;
    } else {
        if (Group + 1 == Vcb->sbi.s_groups_count) {
            Length = INODES_COUNT % INODES_PER_GROUP;
            if (!Length) {
                /* INODES_COUNT is integer multiple of INODES_PER_GROUP */
                Length = INODES_PER_GROUP;
            }
        } else  {
            Length = INODES_PER_GROUP;
        }
    }

    RtlInitializeBitMap(&InodeBitmap, (PULONG)bh->b_data, Length);
    dwInode = RtlFindClearBits(&InodeBitmap, 1, 0);

    if (dwInode == 0xFFFFFFFF || dwInode >= Length) {

        RtlZeroMemory(&InodeBitmap, sizeof(RTL_BITMAP));
        if (ext4_free_inodes_count(sb, gd) > 0) {
            ext4_free_inodes_set(sb, gd, 0);
            Ext2SaveGroup(IrpContext, Vcb, Group);
        }
        goto repeat;

    } else {

        __u32 count = 0;

        /* update unused inodes count */
        count = ext4_free_inodes_count(sb, gd) - 1;
        ext4_free_inodes_set(sb, gd, count);

        RtlSetBits(&InodeBitmap, dwInode, 1);

        /* set block bitmap dirty in cache */
        mark_buffer_dirty(bh);

        /* If we didn't allocate from within the initialized part of the inode
         * table then we need to initialize up to this inode. */
        if (EXT3_HAS_RO_COMPAT_FEATURE(sb, EXT4_FEATURE_RO_COMPAT_GDT_CSUM)) {

            __u32 free;

            if (gd->bg_flags & cpu_to_le16(EXT4_BG_INODE_UNINIT)) {
                gd->bg_flags &= cpu_to_le16(~EXT4_BG_INODE_UNINIT);
                /* When marking the block group with
                 * ~EXT4_BG_INODE_UNINIT we don't want to depend
                 * on the value of bg_itable_unused even though
                 * mke2fs could have initialized the same for us.
                 * Instead we calculated the value below
                 */

                free = 0;
            } else {
                free = EXT3_INODES_PER_GROUP(sb) - ext4_itable_unused_count(sb, gd);
            }

            /*
             * Check the relative inode number against the last used
             * relative inode number in this group. if it is greater
             * we need to  update the bg_itable_unused count
             *
             */
            if (dwInode + 1 > free) {
                ext4_itable_unused_set(sb, gd,
                                       (EXT3_INODES_PER_GROUP(sb) - 1 - dwInode));
            }

            /* We may have to initialize the block bitmap if it isn't already */
            if (gd->bg_flags & cpu_to_le16(EXT4_BG_BLOCK_UNINIT)) {

                struct buffer_head *block_bitmap_bh = NULL;

                /* recheck and clear flag under lock if we still need to */
                block_bitmap_bh = sb_getblk_zero(sb, ext4_block_bitmap(sb, gd));
                if (block_bitmap_bh) {
                    gd->bg_checksum = ext4_group_desc_csum(EXT3_SB(sb), Group, gd);
                    free = ext4_init_block_bitmap(sb, block_bitmap_bh, Group, gd);
                    set_buffer_uptodate(block_bitmap_bh);
                    brelse(block_bitmap_bh);
                    gd->bg_flags &= cpu_to_le16(~EXT4_BG_BLOCK_UNINIT);
                    ext4_free_blks_set(sb, gd, free);
                    Ext2SaveGroup(IrpContext, Vcb, Group);
                }
            }
        }

        *Inode = dwInode + 1 + Group * INODES_PER_GROUP;

        /* update group_desc / super_block */
        if (Type == EXT2_FT_DIR) {
            ext4_used_dirs_set(sb, gd, ext4_used_dirs_count(sb, gd) + 1);
        }
        Ext2SaveGroup(IrpContext, Vcb, Group);
        Ext2UpdateVcbStat(IrpContext, Vcb);
        Status = STATUS_SUCCESS;
    }

errorout:

    ExReleaseResourceLite(&Vcb->MetaInode);

    if (bh)
        fini_bh(&bh);

    if (gb)
        fini_bh(&gb);


    return Status;
}

NTSTATUS
Ext2UpdateGroupDirStat(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                group
    )
{
    struct super_block     *sb = &Vcb->sb;
    PEXT2_GROUP_DESC        gd;
    struct buffer_head     *gb = NULL;
    NTSTATUS                status;

    ExAcquireResourceExclusiveLite(&Vcb->MetaInode, TRUE);

    /* get group desc */
    gd = ext4_get_group_desc(sb, group, &gb);
    if (!gd) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    /* update group_desc and super_block */
    ext4_used_dirs_set(sb, gd, ext4_used_dirs_count(sb, gd) - 1);
    Ext2SaveGroup(IrpContext, Vcb, group);
    Ext2UpdateVcbStat(IrpContext, Vcb);
    status = STATUS_SUCCESS;

errorout:

    ExReleaseResourceLite(&Vcb->MetaInode);

    if (gb)
        fini_bh(&gb);

    return status;
}


NTSTATUS
Ext2FreeInode(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Inode,
    IN ULONG                Type
)
{
    struct super_block     *sb = &Vcb->sb;
    PEXT2_GROUP_DESC        gd;
    struct buffer_head     *gb = NULL;
    struct buffer_head     *bh = NULL;
    ext4_fsblk_t            bitmap_blk;

    RTL_BITMAP      InodeBitmap;
    ULONG           Group;
    ULONG           Length;
    LARGE_INTEGER   Offset;

    ULONG           dwIno;
    BOOLEAN         bModified = FALSE;

    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    ExAcquireResourceExclusiveLite(&Vcb->MetaInode, TRUE);

    Group = (Inode - 1) / INODES_PER_GROUP;
    dwIno = (Inode - 1) % INODES_PER_GROUP;

    DEBUG(DL_INF, ( "Ext2FreeInode: Inode: %xh (Group/Off = %xh/%xh)\n",
                    Inode, Group, dwIno));

    if (Group >= Vcb->sbi.s_groups_count)  {
        DbgBreak();
        goto errorout;
    }

    gd = ext4_get_group_desc(sb, Group, &gb);
    if (!gd) {
        DbgBreak();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }

    bitmap_blk = ext4_inode_bitmap(sb, gd);
    bh = sb_getblk(sb, bitmap_blk);
    if (!bh) {
        DbgBreak();
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto errorout;
    }
    if (!buffer_uptodate(bh)) {
        int err = bh_submit_read(bh);
        if (err < 0) {
            DbgPrint("bh_submit_read error! err: %d\n", err);
            Status = Ext2WinntError(err);
            goto errorout;
        }
    }

    if (Group == Vcb->sbi.s_groups_count - 1) {

        Length = INODES_COUNT % INODES_PER_GROUP;
        if (!Length) {
            /* s_inodes_count is integer multiple of s_inodes_per_group */
            Length = INODES_PER_GROUP;
        }
    } else {
        Length = INODES_PER_GROUP;
    }

    RtlInitializeBitMap(&InodeBitmap, (PULONG)bh->b_data, Length);

    if (RtlCheckBit(&InodeBitmap, dwIno) == 0) {
        DbgBreak();
        Status = STATUS_SUCCESS;
    } else {
        RtlClearBits(&InodeBitmap, dwIno, 1);
        bModified = TRUE;
    }

    if (bModified) {
        /* update group free inodes */
        ext4_free_inodes_set(sb, gd,
                             RtlNumberOfClearBits(&InodeBitmap));

        /* set inode block dirty and add to vcb dirty range */
        mark_buffer_dirty(bh);

        /* update group_desc and super_block */
        if (Type == EXT2_FT_DIR) {
            ext4_used_dirs_set(sb, gd,
                               ext4_used_dirs_count(sb, gd) - 1);
        }
        Ext2SaveGroup(IrpContext, Vcb, Group);
        Ext2UpdateVcbStat(IrpContext, Vcb);
        Status = STATUS_SUCCESS;
    }

errorout:

    ExReleaseResourceLite(&Vcb->MetaInode);

    if (bh)
        fini_bh(&bh);

    if (gb)
        fini_bh(&gb);

    return Status;
}


NTSTATUS
Ext2AddEntry (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Dcb,
    IN struct inode       *Inode,
    IN PUNICODE_STRING     FileName,
    struct dentry        **Dentry
)
{
    struct dentry          *de = NULL;

    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    OEM_STRING              oem;
    int                     rc;

    BOOLEAN                 MainResourceAcquired = FALSE;

    if (!IsDirectory(Dcb)) {
        DbgBreak();
        return STATUS_NOT_A_DIRECTORY;
    }

    ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);
    MainResourceAcquired = TRUE;

    _SEH2_TRY {

        Ext2ReferXcb(&Dcb->ReferenceCount);
        de = Ext2BuildEntry(Vcb, Dcb->Mcb, FileName);
        if (!de) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }
        de->d_inode = Inode;

        rc = ext3_add_entry(IrpContext, de, Inode);
        status = Ext2WinntError(rc);
        if (NT_SUCCESS(status)) {

            /* increase dir inode's nlink for .. */
            if (S_ISDIR(Inode->i_mode)) {
                ext3_inc_count(Dcb->Inode);
                ext3_mark_inode_dirty(IrpContext, Dcb->Inode);
            }

            /* increase inode nlink reference */
            ext3_inc_count(Inode);
            ext3_mark_inode_dirty(IrpContext, Inode);

            if (Dentry) {
                *Dentry = de;
                de = NULL;
            }
        }

    } _SEH2_FINALLY {

        Ext2DerefXcb(&Dcb->ReferenceCount);

        if (MainResourceAcquired)    {
            ExReleaseResourceLite(&Dcb->MainResource);
        }

        if (de)
            Ext2FreeEntry(de);
    } _SEH2_END;

    return status;
}


NTSTATUS
Ext2SetFileType (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Dcb,
    IN PEXT2_MCB            Mcb,
    IN umode_t              mode
    )
{
    struct inode *dir = Dcb->Inode;
    struct buffer_head *bh = NULL;
    struct ext3_dir_entry_2 *de;
    struct inode *inode;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN  MainResourceAcquired = FALSE;

    if (!EXT3_HAS_INCOMPAT_FEATURE(dir->i_sb, EXT3_FEATURE_INCOMPAT_FILETYPE)) {
        return STATUS_SUCCESS;
    }

    if (!IsDirectory(Dcb)) {
        return STATUS_NOT_A_DIRECTORY;
    }

    ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);
    MainResourceAcquired = TRUE;

    _SEH2_TRY {

        Ext2ReferXcb(&Dcb->ReferenceCount);

        bh = ext3_find_entry(IrpContext, Mcb->de, &de);
        if (!bh)
            _SEH2_LEAVE;

        inode = &Mcb->Inode;
        if (le32_to_cpu(de->inode) != inode->i_ino)
            _SEH2_LEAVE;

        ext3_set_de_type(inode->i_sb, de, mode);
        mark_buffer_dirty(bh);

        if (S_ISDIR(inode->i_mode) == S_ISDIR(mode)) {
        } else if (S_ISDIR(inode->i_mode)) {
            ext3_dec_count(dir);
        } else if (S_ISDIR(mode)) {
            ext3_inc_count(dir);
        }
        dir->i_ctime = dir->i_mtime = ext3_current_time(dir);
        ext3_mark_inode_dirty(IrpContext, dir);

        inode->i_mode = mode;
        ext3_mark_inode_dirty(IrpContext, inode);

        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        Ext2DerefXcb(&Dcb->ReferenceCount);

        if (MainResourceAcquired)
            ExReleaseResourceLite(&Dcb->MainResource);

        if (bh)
            brelse(bh);
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2RemoveEntry (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Dcb,
    IN PEXT2_MCB            Mcb
)
{
    struct inode *dir = Dcb->Inode;
    struct buffer_head *bh = NULL;
    struct ext3_dir_entry_2 *de;
    struct inode *inode;
    int rc = -ENOENT;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN  MainResourceAcquired = FALSE;

    if (!IsDirectory(Dcb)) {
        return STATUS_NOT_A_DIRECTORY;
    }

    ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);
    MainResourceAcquired = TRUE;

    _SEH2_TRY {

        Ext2ReferXcb(&Dcb->ReferenceCount);

        bh = ext3_find_entry(IrpContext, Mcb->de, &de);
        if (!bh)
            _SEH2_LEAVE;

        inode = &Mcb->Inode;
        if (le32_to_cpu(de->inode) != inode->i_ino)
            _SEH2_LEAVE;

        if (!inode->i_nlink) {
            ext3_warning (inode->i_sb, "ext3_unlink",
                          "Deleting nonexistent file (%lu), %d",
                          inode->i_ino, inode->i_nlink);
            inode->i_nlink = 1;
        }
        rc = ext3_delete_entry(IrpContext, dir, de, bh);
        if (rc) {
            Status = Ext2WinntError(rc);
            _SEH2_LEAVE;
        }
        /*
        	    if (!inode->i_nlink)
        		    ext3_orphan_add(handle, inode);
        */
        inode->i_ctime = dir->i_ctime = dir->i_mtime = ext3_current_time(dir);
        ext3_dec_count(inode);
        ext3_mark_inode_dirty(IrpContext, inode);

        /* decrease dir inode's nlink for .. */
        if (S_ISDIR(inode->i_mode)) {
            ext3_update_dx_flag(dir);
            ext3_dec_count(dir);
            ext3_mark_inode_dirty(IrpContext, dir);
        }

        Status = STATUS_SUCCESS;

    } _SEH2_FINALLY {

        Ext2DerefXcb(&Dcb->ReferenceCount);

        if (MainResourceAcquired)
            ExReleaseResourceLite(&Dcb->MainResource);

        if (bh)
            brelse(bh);
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2SetParentEntry (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Dcb,
    IN ULONG               OldParent,
    IN ULONG               NewParent )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PEXT2_DIR_ENTRY2        pSelf   = NULL;
    PEXT2_DIR_ENTRY2        pParent = NULL;

    ULONG                   dwBytes = 0;

    BOOLEAN                 MainResourceAcquired = FALSE;

    ULONG                   Offset = 0;

    if (!IsDirectory(Dcb)) {
        return STATUS_NOT_A_DIRECTORY;
    }

    if (OldParent == NewParent) {
        return STATUS_SUCCESS;
    }

    MainResourceAcquired =
        ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

    _SEH2_TRY {

        Ext2ReferXcb(&Dcb->ReferenceCount);

        pSelf = (PEXT2_DIR_ENTRY2)
                Ext2AllocatePool(
                    PagedPool,
                    EXT2_DIR_REC_LEN(1) + EXT2_DIR_REC_LEN(2),
                    EXT2_DENTRY_MAGIC
                );
        if (!pSelf) {
            DEBUG(DL_ERR, ( "Ex2SetParentEntry: failed to allocate pSelf.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        dwBytes = 0;

        //
        // Reading the DCB contents
        //

        Status = Ext2ReadInode(
                     IrpContext,
                     Vcb,
                     Dcb->Mcb,
                     (ULONGLONG)Offset,
                     (PVOID)pSelf,
                     EXT2_DIR_REC_LEN(1) + EXT2_DIR_REC_LEN(2),
                     FALSE,
                     &dwBytes );

        if (!NT_SUCCESS(Status)) {
            DEBUG(DL_ERR, ( "Ext2SetParentEntry: failed to read directory.\n"));
            _SEH2_LEAVE;
        }

        ASSERT(dwBytes == EXT2_DIR_REC_LEN(1) + EXT2_DIR_REC_LEN(2));

        pParent = (PEXT2_DIR_ENTRY2)((PUCHAR)pSelf + pSelf->rec_len);

        if (pSelf->name_len == 1 && pSelf->name[0] == '.' &&
                pParent->name_len == 2 && pParent->name[0] == '.' &&
                pParent->name[1] == '.') {

            if (pParent->inode != OldParent) {
                DbgBreak();
            }
            pParent->inode = NewParent;

            Status = Ext2WriteInode(
                         IrpContext,
                         Vcb,
                         Dcb->Mcb,
                         (ULONGLONG)Offset,
                         pSelf,
                         dwBytes,
                         FALSE,
                         &dwBytes );
        } else {
            DbgBreak();
        }

    } _SEH2_FINALLY {


        if (Ext2DerefXcb(&Dcb->ReferenceCount) == 0) {
            DEBUG(DL_ERR, ( "Ext2SetParentEntry: Dcb reference goes to ZERO.\n"));
        }

        if (MainResourceAcquired)    {
            ExReleaseResourceLite(&Dcb->MainResource);
        }

        if (pSelf) {
            Ext2FreePool(pSelf, EXT2_DENTRY_MAGIC);
        }
    } _SEH2_END;

    return Status;
}

int ext3_check_dir_entry (const char * function, struct inode * dir,
                          struct ext3_dir_entry_2 * de,
                          struct buffer_head * bh,
                          unsigned long offset)
{
    const char * error_msg = NULL;
    const int rlen = ext3_rec_len_from_disk(de->rec_len);

    if (rlen < EXT3_DIR_REC_LEN(1))
        error_msg = "rec_len is smaller than minimal";
    else if (rlen % 4 != 0)
        error_msg = "rec_len % 4 != 0";
    else if (rlen < EXT3_DIR_REC_LEN(de->name_len))
        error_msg = "rec_len is too small for name_len";
    else if ((char *) de + rlen > bh->b_data + dir->i_sb->s_blocksize)
        error_msg = "directory entry across blocks";
    else if (le32_to_cpu(de->inode) >
             le32_to_cpu(EXT3_SB(dir->i_sb)->s_es->s_inodes_count))
        error_msg = "inode out of bounds";

    if (error_msg != NULL) {
        DEBUG(DL_ERR, ("%s: bad entry in directory %u: %s - "
                       "offset=%u, inode=%u, rec_len=%d, name_len=%d\n",
                       function, dir->i_ino, error_msg, offset,
                       (unsigned long) le32_to_cpu(de->inode),
                       rlen, de->name_len));
    }
    return error_msg == NULL ? 1 : 0;
}


/*
 * p is at least 6 bytes before the end of page
 */
struct ext3_dir_entry_2 *
            ext3_next_entry(struct ext3_dir_entry_2 *p)
{
    return (struct ext3_dir_entry_2 *)((char *)p +
                                       ext3_rec_len_from_disk(p->rec_len));
}

#define MAX_LFS_FILESIZE 	0x7fffffffffffffff

/*
 * Maximal extent format file size.
 * Resulting logical blkno at s_maxbytes must fit in our on-disk
 * extent format containers, within a sector_t, and within i_blocks
 * in the vfs.  ext4 inode has 48 bits of i_block in fsblock units,
 * so that won't be a limiting factor.
 *
 * Note, this does *not* consider any metadata overhead for vfs i_blocks.
 */
static loff_t ext4_max_size(int blkbits, int has_huge_files)
{
    loff_t res;
    loff_t upper_limit = MAX_LFS_FILESIZE;

    /* small i_blocks in vfs inode? */
    if (!has_huge_files || sizeof(blkcnt_t) < sizeof(u64)) {
        /*
         * CONFIG_LBD is not enabled implies the inode
         * i_block represent total blocks in 512 bytes
         * 32 == size of vfs inode i_blocks * 8
         */
        upper_limit = (1LL << 32) - 1;

        /* total blocks in file system block size */
        upper_limit >>= (blkbits - 9);
        upper_limit <<= blkbits;
    }

    /* 32-bit extent-start container, ee_block */
    res = 1LL << 32;
    res <<= blkbits;
    res -= 1;

    /* Sanity check against vm- & vfs- imposed limits */
    if (res > upper_limit)
        res = upper_limit;

    return res;
}

/*
 * Maximal extent format file size.
 * Resulting logical blkno at s_maxbytes must fit in our on-disk
 * extent format containers, within a sector_t, and within i_blocks
 * in the vfs.  ext4 inode has 48 bits of i_block in fsblock units,
 * so that won't be a limiting factor.
 *
 * Note, this does *not* consider any metadata overhead for vfs i_blocks.
 */
loff_t ext3_max_size(int blkbits, int has_huge_files)
{
    loff_t res;
    loff_t upper_limit = MAX_LFS_FILESIZE;

    /* small i_blocks in vfs inode? */
    if (!has_huge_files) {
        /*
         * CONFIG_LBD is not enabled implies the inode
         * i_block represent total blocks in 512 bytes
         * 32 == size of vfs inode i_blocks * 8
         */
        upper_limit = ((loff_t)1 << 32) - 1;

        /* total blocks in file system block size */
        upper_limit >>= (blkbits - 9);
        upper_limit <<= blkbits;
    }

    /* 32-bit extent-start container, ee_block */
    res = (loff_t)1 << 32;
    res <<= blkbits;
    res -= 1;

    /* Sanity check against vm- & vfs- imposed limits */
    if (res > upper_limit)
        res = upper_limit;

    return res;
}

/*
 * Maximal bitmap file size.  There is a direct, and {,double-,triple-}indirect
 * block limit, and also a limit of (2^48 - 1) 512-byte sectors in i_blocks.
 * We need to be 1 filesystem block less than the 2^48 sector limit.
 */
loff_t ext3_max_bitmap_size(int bits, int has_huge_files)
{
    loff_t res = EXT3_NDIR_BLOCKS;
    int meta_blocks;
    loff_t upper_limit;
    /* This is calculated to be the largest file size for a
     * dense, bitmapped file such that the total number of
     * sectors in the file, including data and all indirect blocks,
     * does not exceed 2^48 -1
     * __u32 i_blocks_lo and _u16 i_blocks_high representing the
     * total number of  512 bytes blocks of the file
     */

    if (!has_huge_files) {
        /*
         * !has_huge_files or CONFIG_LBD is not enabled
         * implies the inode i_block represent total blocks in
         * 512 bytes 32 == size of vfs inode i_blocks * 8
         */
        upper_limit = ((loff_t)1 << 32) - 1;

        /* total blocks in file system block size */
        upper_limit >>= (bits - 9);

    } else {
        /*
         * We use 48 bit ext4_inode i_blocks
         * With EXT4_HUGE_FILE_FL set the i_blocks
         * represent total number of blocks in
         * file system block size
         */
        upper_limit = ((loff_t)1 << 48) - 1;

    }

    /* indirect blocks */
    meta_blocks = 1;
    /* double indirect blocks */
    meta_blocks += 1 + ((loff_t)1 << (bits-2));
    /* tripple indirect blocks */
    meta_blocks += 1 + ((loff_t)1 << (bits-2)) + ((loff_t)1 << (2*(bits-2)));

    upper_limit -= meta_blocks;
    upper_limit <<= bits;

    res += (loff_t)1 << (bits-2);
    res += (loff_t)1 << (2*(bits-2));
    res += (loff_t)1 << (3*(bits-2));
    res <<= bits;
    if (res > upper_limit)
        res = upper_limit;

    if (res > MAX_LFS_FILESIZE)
        res = MAX_LFS_FILESIZE;

    return res;
}

blkcnt_t ext3_inode_blocks(struct ext3_inode *raw_inode,
                           struct inode *inode)
{
    blkcnt_t i_blocks ;
    struct super_block *sb = inode->i_sb;
    PEXT2_VCB Vcb = (PEXT2_VCB)sb->s_priv;

    if (EXT3_HAS_RO_COMPAT_FEATURE(sb,
                                   EXT4_FEATURE_RO_COMPAT_HUGE_FILE)) {
        /* we are using combined 48 bit field */
        i_blocks = ((u64)le16_to_cpu(raw_inode->i_blocks_high)) << 32 |
                   le32_to_cpu(raw_inode->i_blocks);
        if (inode->i_flags & EXT4_HUGE_FILE_FL) {
            /* i_blocks represent file system block size */
            return i_blocks  << (BLOCK_BITS - 9);
        } else {
            return i_blocks;
        }
    } else {
        return le32_to_cpu(raw_inode->i_blocks);
    }
}

int ext3_inode_blocks_set(struct ext3_inode *raw_inode,
                          struct inode * inode)
{
    u64 i_blocks = inode->i_blocks;
    struct super_block *sb = inode->i_sb;
    PEXT2_VCB Vcb = (PEXT2_VCB)sb->s_priv;

    if (i_blocks < 0x100000000) {
        /*
         * i_blocks can be represnted in a 32 bit variable
         * as multiple of 512 bytes
         */
        raw_inode->i_blocks = cpu_to_le32(i_blocks);
        raw_inode->i_blocks_high = 0;
        inode->i_flags &= ~EXT4_HUGE_FILE_FL;
        return 0;
    }

    if (!EXT3_HAS_RO_COMPAT_FEATURE(sb, EXT4_FEATURE_RO_COMPAT_HUGE_FILE)) {
        EXT3_SET_RO_COMPAT_FEATURE(sb, EXT4_FEATURE_RO_COMPAT_HUGE_FILE);
        Ext2SaveSuper(NULL, Vcb);
    }

    if (i_blocks <= 0xffffffffffff) {
        /*
         * i_blocks can be represented in a 48 bit variable
         * as multiple of 512 bytes
         */
        raw_inode->i_blocks = (__u32)cpu_to_le32(i_blocks);
        raw_inode->i_blocks_high = (__u16)cpu_to_le16(i_blocks >> 32);
        inode->i_flags &= ~EXT4_HUGE_FILE_FL;
    } else {
        inode->i_flags |= EXT4_HUGE_FILE_FL;
        /* i_block is stored in file system block size */
        i_blocks = i_blocks >> (BLOCK_BITS - 9);
        raw_inode->i_blocks  = (__u32)cpu_to_le32(i_blocks);
        raw_inode->i_blocks_high = (__u16)cpu_to_le16(i_blocks >> 32);
    }
    return 0;
}

ext4_fsblk_t ext4_block_bitmap(struct super_block *sb,
                               struct ext4_group_desc *bg)
{
    return le32_to_cpu(bg->bg_block_bitmap) |
           (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
            (ext4_fsblk_t)le32_to_cpu(bg->bg_block_bitmap_hi) << 32 : 0);
}

ext4_fsblk_t ext4_inode_bitmap(struct super_block *sb,
                               struct ext4_group_desc *bg)
{
    return le32_to_cpu(bg->bg_inode_bitmap) |
           (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
            (ext4_fsblk_t)le32_to_cpu(bg->bg_inode_bitmap_hi) << 32 : 0);
}

ext4_fsblk_t ext4_inode_table(struct super_block *sb,
                              struct ext4_group_desc *bg)
{
    return le32_to_cpu(bg->bg_inode_table) |
           (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
            (ext4_fsblk_t)le32_to_cpu(bg->bg_inode_table_hi) << 32 : 0);
}

__u32 ext4_free_blks_count(struct super_block *sb,
                           struct ext4_group_desc *bg)
{
    return le16_to_cpu(bg->bg_free_blocks_count) |
           (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
            (__u32)le16_to_cpu(bg->bg_free_blocks_count_hi) << 16 : 0);
}

__u32 ext4_free_inodes_count(struct super_block *sb,
                             struct ext4_group_desc *bg)
{
    return le16_to_cpu(bg->bg_free_inodes_count) |
           (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
            (__u32)le16_to_cpu(bg->bg_free_inodes_count_hi) << 16 : 0);
}

__u32 ext4_used_dirs_count(struct super_block *sb,
                           struct ext4_group_desc *bg)
{
    return le16_to_cpu(bg->bg_used_dirs_count) |
           (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
            (__u32)le16_to_cpu(bg->bg_used_dirs_count_hi) << 16 : 0);
}

__u32 ext4_itable_unused_count(struct super_block *sb,
                               struct ext4_group_desc *bg)
{
    return le16_to_cpu(bg->bg_itable_unused) |
           (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
            (__u32)le16_to_cpu(bg->bg_itable_unused_hi) << 16 : 0);
}

void ext4_block_bitmap_set(struct super_block *sb,
                           struct ext4_group_desc *bg, ext4_fsblk_t blk)
{
    bg->bg_block_bitmap = cpu_to_le32((u32)blk);
    if (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT)
        bg->bg_block_bitmap_hi = cpu_to_le32(blk >> 32);
}

void ext4_inode_bitmap_set(struct super_block *sb,
                           struct ext4_group_desc *bg, ext4_fsblk_t blk)
{
    bg->bg_inode_bitmap  = cpu_to_le32((u32)blk);
    if (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT)
        bg->bg_inode_bitmap_hi = cpu_to_le32(blk >> 32);
}

void ext4_inode_table_set(struct super_block *sb,
                          struct ext4_group_desc *bg, ext4_fsblk_t blk)
{
    bg->bg_inode_table = cpu_to_le32((u32)blk);
    if (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT)
        bg->bg_inode_table_hi = cpu_to_le32(blk >> 32);
}

void ext4_free_blks_set(struct super_block *sb,
                        struct ext4_group_desc *bg, __u32 count)
{
    bg->bg_free_blocks_count = cpu_to_le16((__u16)count);
    if (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT)
        bg->bg_free_blocks_count_hi = cpu_to_le16(count >> 16);
}

void ext4_free_inodes_set(struct super_block *sb,
                          struct ext4_group_desc *bg, __u32 count)
{
    bg->bg_free_inodes_count = cpu_to_le16((__u16)count);
    if (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT)
        bg->bg_free_inodes_count_hi = cpu_to_le16(count >> 16);
}

void ext4_used_dirs_set(struct super_block *sb,
                        struct ext4_group_desc *bg, __u32 count)
{
    bg->bg_used_dirs_count = cpu_to_le16((__u16)count);
    if (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT)
        bg->bg_used_dirs_count_hi = cpu_to_le16(count >> 16);
}

void ext4_itable_unused_set(struct super_block *sb,
                            struct ext4_group_desc *bg, __u32 count)
{
    bg->bg_itable_unused = cpu_to_le16((__u16)count);
    if (EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT)
        bg->bg_itable_unused_hi = cpu_to_le16(count >> 16);
}

/** CRC table for the CRC-16. The poly is 0x8005 (x16 + x15 + x2 + 1) */
__u16 const crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static inline __u16 crc16_byte(__u16 crc, const __u8 data)
{
    return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

__u16 crc16(__u16 crc, __u8 const *buffer, size_t len)
{
    while (len--)
        crc = crc16_byte(crc, *buffer++);
    return crc;
}

__le16 ext4_group_desc_csum(struct ext3_sb_info *sbi, __u32 block_group,
                            struct ext4_group_desc *gdp)
{
	int offset;
	__u16 crc = 0;
	__le32 le_group = cpu_to_le32(block_group);

	/* old crc16 code */
	if (!(sbi->s_es->s_feature_ro_compat &
	      cpu_to_le32(EXT4_FEATURE_RO_COMPAT_GDT_CSUM)))
		return 0;

	offset = offsetof(struct ext4_group_desc, bg_checksum);

	crc = crc16(~0, sbi->s_es->s_uuid, sizeof(sbi->s_es->s_uuid));
	crc = crc16(crc, (__u8 *)&le_group, sizeof(le_group));
	crc = crc16(crc, (__u8 *)gdp, offset);
	offset += sizeof(gdp->bg_checksum); /* skip checksum */
	/* for checksum of struct ext4_group_desc do the rest...*/
	if ((sbi->s_es->s_feature_incompat &
	     cpu_to_le32(EXT4_FEATURE_INCOMPAT_64BIT)) &&
	    offset < le16_to_cpu(sbi->s_es->s_desc_size))
		crc = crc16(crc, (__u8 *)gdp + offset,
			    le16_to_cpu(sbi->s_es->s_desc_size) -
				offset);

	return cpu_to_le16(crc);
}

int ext4_group_desc_csum_verify(struct ext3_sb_info *sbi, __u32 block_group,
                                struct ext4_group_desc *gdp)
{
    if ((sbi->s_es->s_feature_ro_compat & cpu_to_le32(EXT4_FEATURE_RO_COMPAT_GDT_CSUM)) &&
        (gdp->bg_checksum != ext4_group_desc_csum(sbi, block_group, gdp)))
        return 0;

    return 1;
}


static inline int test_root(ext3_group_t a, ext3_group_t b)
{
    ext3_group_t num = b;

    while (a > num)
        num *= b;
    return num == a;
}

static int ext3_group_sparse(ext3_group_t group)
{
    if (group <= 1)
        return 1;
    if (!(group & 1))
        return 0;
    return (test_root(group, 7) || test_root(group, 5) ||
            test_root(group, 3));
}

/**
 *	ext4_bg_has_super - number of blocks used by the superblock in group
 *	@sb: superblock for filesystem
 *	@group: group number to check
 *
 *	Return the number of blocks used by the superblock (primary or backup)
 *	in this group.  Currently this will be only 0 or 1.
 */
int ext3_bg_has_super(struct super_block *sb, ext3_group_t group)
{
    if (EXT3_HAS_RO_COMPAT_FEATURE(sb,
                                   EXT3_FEATURE_RO_COMPAT_SPARSE_SUPER) &&
            !ext3_group_sparse(group))
        return 0;
    return 1;
}

static unsigned long ext4_bg_num_gdb_meta(struct super_block *sb,
        ext4_group_t group)
{
    unsigned long metagroup = group / EXT4_DESC_PER_BLOCK(sb);
    ext4_group_t first = metagroup * EXT4_DESC_PER_BLOCK(sb);
    ext4_group_t last = first + EXT4_DESC_PER_BLOCK(sb) - 1;

    if (group == first || group == first + 1 || group == last)
        return 1;
    return 0;
}

static unsigned long ext4_bg_num_gdb_nometa(struct super_block *sb,
        ext4_group_t group)
{
    return ext3_bg_has_super(sb, group) ? EXT3_SB(sb)->s_gdb_count : 0;
}

/**
 *	ext4_bg_num_gdb - number of blocks used by the group table in group
 *	@sb: superblock for filesystem
 *	@group: group number to check
 *
 *	Return the number of blocks used by the group descriptor table
 *	(primary or backup) in this group.  In the future there may be a
 *	different number of descriptor blocks in each group.
 */
unsigned long ext4_bg_num_gdb(struct super_block *sb, ext4_group_t group)
{
    unsigned long first_meta_bg =
        le32_to_cpu(EXT3_SB(sb)->s_es->s_first_meta_bg);
    unsigned long metagroup = group / EXT4_DESC_PER_BLOCK(sb);

    if (!EXT3_HAS_INCOMPAT_FEATURE(sb,EXT4_FEATURE_INCOMPAT_META_BG) ||
            metagroup < first_meta_bg)
        return ext4_bg_num_gdb_nometa(sb, group);

    return ext4_bg_num_gdb_meta(sb,group);

}

ext3_fsblk_t descriptor_loc(struct super_block *sb,
                            ext3_fsblk_t logical_sb_block, unsigned int nr)
{
    struct ext3_sb_info *sbi = EXT3_SB(sb);
    ext3_group_t bg, first_meta_bg;
    int has_super = 0;

    first_meta_bg = le32_to_cpu(sbi->s_es->s_first_meta_bg);

    if (!EXT3_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_META_BG) ||
            nr < first_meta_bg)
        return logical_sb_block + nr + 1;
    bg = sbi->s_desc_per_block * nr;
    if (ext3_bg_has_super(sb, bg))
        has_super = 1;
    return (has_super + ext3_group_first_block_no(sb, bg));
}

#define ext4_set_bit(n, p) set_bit((int)(n), (unsigned long *)(p))

/*
 * The free inodes are managed by bitmaps.  A file system contains several
 * blocks groups.  Each group contains 1 bitmap block for blocks, 1 bitmap
 * block for inodes, N blocks for the inode table and data blocks.
 *
 * The file system contains group descriptors which are located after the
 * super block.  Each descriptor contains the number of the bitmap block and
 * the free blocks count in the block.
 */

/*
 * To avoid calling the atomic setbit hundreds or thousands of times, we only
 * need to use it within a single byte (to ensure we get endianness right).
 * We can use memset for the rest of the bitmap as there are no other users.
 */
void mark_bitmap_end(int start_bit, int end_bit, char *bitmap)
{
    int i;

    if (start_bit >= end_bit)
        return;

    DEBUG(DL_INF, ("mark end bits +%d through +%d used\n", start_bit, end_bit));
    for (i = start_bit; (unsigned)i < ((start_bit + 7) & ~7UL); i++)
        ext4_set_bit(i, bitmap);
    if (i < end_bit)
        memset(bitmap + (i >> 3), 0xff, (end_bit - i) >> 3);
}

/* Initializes an uninitialized inode bitmap */
unsigned ext4_init_inode_bitmap(struct super_block *sb, struct buffer_head *bh,
                                ext4_group_t block_group,
                                struct ext4_group_desc *gdp)
{
    struct ext3_sb_info *sbi = EXT3_SB(sb);

    mark_buffer_dirty(bh);

    /* If checksum is bad mark all blocks and inodes use to prevent
     * allocation, essentially implementing a per-group read-only flag. */
    if (!ext4_group_desc_csum_verify(sbi, block_group, gdp)) {
        ext4_error(sb, __FUNCTION__, "Checksum bad for group %u",
                   block_group);
        ext4_free_blks_set(sb, gdp, 0);
        ext4_free_inodes_set(sb, gdp, 0);
        ext4_itable_unused_set(sb, gdp, 0);
        memset(bh->b_data, 0xff, sb->s_blocksize);
        return 0;
    }

    memset(bh->b_data, 0, (EXT4_INODES_PER_GROUP(sb) + 7) / 8);
    mark_bitmap_end(EXT4_INODES_PER_GROUP(sb), sb->s_blocksize * 8,
                    bh->b_data);
    ext4_itable_unused_set(sb, gdp, EXT4_INODES_PER_GROUP(sb));

    return EXT4_INODES_PER_GROUP(sb);
}

/*
 * Calculate the block group number and offset, given a block number
 */
void ext4_get_group_no_and_offset(struct super_block *sb, ext4_fsblk_t blocknr,
                                  ext4_group_t *blockgrpp, ext4_grpblk_t *offsetp)
{
    struct ext3_super_block *es = EXT3_SB(sb)->s_es;
    ext4_grpblk_t offset;

    blocknr = blocknr - le32_to_cpu(es->s_first_data_block);
    offset = do_div(blocknr, EXT4_BLOCKS_PER_GROUP(sb));
    if (offsetp)
        *offsetp = offset;
    if (blockgrpp)
        *blockgrpp = (ext4_grpblk_t)blocknr;

}

static int ext4_block_in_group(struct super_block *sb, ext4_fsblk_t block,
                               ext4_group_t block_group)
{
    ext4_group_t actual_group;
    ext4_get_group_no_and_offset(sb, block, &actual_group, NULL);
    if (actual_group == block_group)
        return 1;
    return 0;
}

static int ext4_group_used_meta_blocks(struct super_block *sb,
                                       ext4_group_t block_group)
{
    ext4_fsblk_t tmp;
    struct ext3_sb_info *sbi = EXT3_SB(sb);
    /* block bitmap, inode bitmap, and inode table blocks */
    int used_blocks = sbi->s_itb_per_group + 2;

    if (EXT3_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_FLEX_BG)) {
        struct ext4_group_desc *gdp;
        struct buffer_head *bh = NULL;

        gdp = ext4_get_group_desc(sb, block_group, &bh);
        if (!ext4_block_in_group(sb, ext4_block_bitmap(sb, gdp),
                                 block_group))
            used_blocks--;

        if (!ext4_block_in_group(sb, ext4_inode_bitmap(sb, gdp),
                                 block_group))
            used_blocks--;

        tmp = ext4_inode_table(sb, gdp);
        for (; tmp < ext4_inode_table(sb, gdp) +
                sbi->s_itb_per_group; tmp++) {
            if (!ext4_block_in_group(sb, tmp, block_group))
                used_blocks -= 1;
        }
        if (bh)
            fini_bh(&bh);
    }
    return used_blocks;
}

/* Initializes an uninitialized block bitmap if given, and returns the
 * number of blocks free in the group. */
unsigned ext4_init_block_bitmap(struct super_block *sb, struct buffer_head *bh,
                                ext4_group_t block_group, struct ext4_group_desc *gdp)
{
    int bit, bit_max;
    unsigned free_blocks, group_blocks;
    struct ext3_sb_info *sbi = EXT3_SB(sb);

    if (bh) {
        mark_buffer_dirty(bh);
        /* If checksum is bad mark all blocks used to prevent allocation
         * essentially implementing a per-group read-only flag. */
        if (!ext4_group_desc_csum_verify(sbi, block_group, gdp)) {
            ext4_error(sb, __FUNCTION__,
                       "Checksum bad for group %u", block_group);
            ext4_free_blks_set(sb, gdp, 0);
            ext4_free_inodes_set(sb, gdp, 0);
            ext4_itable_unused_set(sb, gdp, 0);
            memset(bh->b_data, 0xff, sb->s_blocksize);
            return 0;
        }
        memset(bh->b_data, 0, sb->s_blocksize);
    }

    /* Check for superblock and gdt backups in this group */
    bit_max = ext3_bg_has_super(sb, block_group);

    if (!EXT3_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_META_BG) ||
            block_group < le32_to_cpu(sbi->s_es->s_first_meta_bg) *
            sbi->s_desc_per_block) {
        if (bit_max) {
            bit_max += ext4_bg_num_gdb(sb, block_group);
            bit_max +=
                le16_to_cpu(sbi->s_es->s_reserved_gdt_blocks);
        }
    } else { /* For META_BG_BLOCK_GROUPS */
        bit_max += ext4_bg_num_gdb(sb, block_group);
    }

    if (block_group == sbi->s_groups_count - 1) {
        /*
         * Even though mke2fs always initialize first and last group
         * if some other tool enabled the EXT4_BG_BLOCK_UNINIT we need
         * to make sure we calculate the right free blocks
         */
        group_blocks = (unsigned int)(ext3_blocks_count(sbi->s_es) -
                                      le32_to_cpu(sbi->s_es->s_first_data_block) -
                                      (EXT4_BLOCKS_PER_GROUP(sb) * (sbi->s_groups_count - 1)));
    } else {
        group_blocks = EXT4_BLOCKS_PER_GROUP(sb);
    }

    free_blocks = group_blocks - bit_max;

    if (bh) {
        ext4_fsblk_t start, tmp;
        int flex_bg = 0;

        for (bit = 0; bit < bit_max; bit++)
            ext4_set_bit(bit, bh->b_data);

        start = ext3_group_first_block_no(sb, block_group);

        if (EXT3_HAS_INCOMPAT_FEATURE(sb,
                                      EXT4_FEATURE_INCOMPAT_FLEX_BG))
            flex_bg = 1;

        /* Set bits for block and inode bitmaps, and inode table */
        tmp = ext4_block_bitmap(sb, gdp);
        if (!flex_bg || ext4_block_in_group(sb, tmp, block_group))
            ext4_set_bit(tmp - start, bh->b_data);

        tmp = ext4_inode_bitmap(sb, gdp);
        if (!flex_bg || ext4_block_in_group(sb, tmp, block_group))
            ext4_set_bit(tmp - start, bh->b_data);

        tmp = ext4_inode_table(sb, gdp);
        for (; tmp < ext4_inode_table(sb, gdp) +
                sbi->s_itb_per_group; tmp++) {
            if (!flex_bg ||
                    ext4_block_in_group(sb, tmp, block_group))
                ext4_set_bit(tmp - start, bh->b_data);
        }
        /*
         * Also if the number of blocks within the group is
         * less than the blocksize * 8 ( which is the size
         * of bitmap ), set rest of the block bitmap to 1
         */
        mark_bitmap_end(group_blocks, sb->s_blocksize * 8, bh->b_data);
    }
    return free_blocks - ext4_group_used_meta_blocks(sb, block_group);
}

/**
 * ext4_get_group_desc() -- load group descriptor from disk
 * @sb:			super block
 * @block_group:	given block group
 * @bh:			pointer to the buffer head to store the block
 *			group descriptor
 */
struct ext4_group_desc * ext4_get_group_desc(struct super_block *sb,
                    ext4_group_t block_group, struct buffer_head **bh)
{
    struct ext4_group_desc *desc = NULL;
    struct ext3_sb_info *sbi = EXT3_SB(sb);
    PEXT2_VCB vcb = sb->s_priv;
    ext4_group_t group;
    ext4_group_t offset;

    if (bh)
        *bh = NULL;

    if (block_group >= sbi->s_groups_count) {
        ext4_error(sb, "ext4_get_group_desc",
                   "block_group >= groups_count - "
                   "block_group = %u, groups_count = %u",
                   block_group, sbi->s_groups_count);

        return NULL;
    }

    _SEH2_TRY {

        group = block_group >> EXT4_DESC_PER_BLOCK_BITS(sb);
        offset = block_group & (EXT4_DESC_PER_BLOCK(sb) - 1);

        if (!sbi->s_gd) {
            if (!Ext2LoadGroup(vcb)) {
                _SEH2_LEAVE;
            }
        } else if ( !sbi->s_gd[group].block ||
                    !sbi->s_gd[group].bh) {
            if (!Ext2LoadGroupBH(vcb)) {
                _SEH2_LEAVE;
            }
        }

        desc = (struct ext4_group_desc *)((PCHAR)sbi->s_gd[group].gd +
                                          offset * EXT4_DESC_SIZE(sb));
        if (bh) {
            atomic_inc(&sbi->s_gd[group].bh->b_count);
            *bh = sbi->s_gd[group].bh;
        }
    } _SEH2_FINALLY {
        /* do cleanup */
    } _SEH2_END;

    return desc;
}


/**
 * ext4_count_free_blocks() -- count filesystem free blocks
 * @sb:		superblock
 *
 * Adds up the number of free blocks from each block group.
 */
ext4_fsblk_t ext4_count_free_blocks(struct super_block *sb)
{
    ext4_fsblk_t desc_count;
    struct ext4_group_desc *gdp;
    struct buffer_head *bh = NULL;
    ext4_group_t i;
    ext4_group_t ngroups = EXT3_SB(sb)->s_groups_count;

    desc_count = 0;
    smp_rmb();
    for (i = 0; i < ngroups; i++) {
        gdp = ext4_get_group_desc(sb, i, &bh);
        if (!bh)
            continue;
        desc_count += ext4_free_blks_count(sb, gdp);
        fini_bh(&bh);
    }

    return desc_count;
}

unsigned long ext4_count_free_inodes(struct super_block *sb)
{
    unsigned long desc_count;
    struct ext4_group_desc *gdp;
    struct buffer_head *bh = NULL;
    ext4_group_t i;

    desc_count = 0;
    for (i = 0; i < EXT3_SB(sb)->s_groups_count; i++) {
        gdp = ext4_get_group_desc(sb, i, &bh);
        if (!bh)
            continue;
        desc_count += ext4_free_inodes_count(sb, gdp);
        fini_bh(&bh);
    }
    return desc_count;
}

/* Called at mount-time, super-block is locked */
unsigned long ext4_count_dirs(struct super_block * sb)
{
    struct ext4_group_desc *gdp;
    struct buffer_head *bh = NULL;
    unsigned long count = 0;
    ext4_group_t i;

    for (i = 0; i < EXT3_SB(sb)->s_groups_count; i++) {
        gdp = ext4_get_group_desc(sb, i, &bh);
        if (!bh)
            continue;
        count += ext4_used_dirs_count(sb, gdp);
        fini_bh(&bh);
    }
    return count;
}

/* Called at mount-time, super-block is locked */
int ext4_check_descriptors(struct super_block *sb)
{
    PEXT2_VCB            Vcb = sb->s_priv;
    struct ext3_sb_info *sbi = EXT3_SB(sb);
    ext4_fsblk_t first_block = le32_to_cpu(sbi->s_es->s_first_data_block);
    ext4_fsblk_t last_block;
    ext4_fsblk_t block_bitmap;
    ext4_fsblk_t inode_bitmap;
    ext4_fsblk_t inode_table;
    int flexbg_flag = 0;
    ext4_group_t i;

    if (EXT3_HAS_INCOMPAT_FEATURE(sb, EXT4_FEATURE_INCOMPAT_FLEX_BG))
        flexbg_flag = 1;

    DEBUG(DL_INF, ("Checking group descriptors"));

    for (i = 0; i < sbi->s_groups_count; i++) {

        struct buffer_head *bh = NULL;
        struct ext4_group_desc *gdp = ext4_get_group_desc(sb, i, &bh);

        if (!bh)
            continue;

        if (i == sbi->s_groups_count - 1 || flexbg_flag)
            last_block = ext3_blocks_count(sbi->s_es) - 1;
        else
            last_block = first_block +
                         (EXT3_BLOCKS_PER_GROUP(sb) - 1);

        block_bitmap = ext4_block_bitmap(sb, gdp);
        if (block_bitmap < first_block || block_bitmap > last_block) {
            printk(KERN_ERR "EXT4-fs: ext4_check_descriptors: "
                   "Block bitmap for group %u not in group "
                   "(block %llu)!\n", i, block_bitmap);
            __brelse(bh);
            return 0;
        }
        inode_bitmap = ext4_inode_bitmap(sb, gdp);
        if (inode_bitmap < first_block || inode_bitmap > last_block) {
            printk(KERN_ERR "EXT4-fs: ext4_check_descriptors: "
                   "Inode bitmap for group %u not in group "
                   "(block %llu)!\n", i, inode_bitmap);
            __brelse(bh);
            return 0;
        }
        inode_table = ext4_inode_table(sb, gdp);
        if (inode_table < first_block ||
                inode_table + sbi->s_itb_per_group - 1 > last_block) {
            printk(KERN_ERR "EXT4-fs: ext4_check_descriptors: "
                   "Inode table for group %u not in group "
                   "(block %llu)!\n", i, inode_table);
            __brelse(bh);
            return 0;
        }

        if (!ext4_group_desc_csum_verify(sbi, i, gdp)) {
            printk(KERN_ERR "EXT4-fs: ext4_check_descriptors: "
                   "Checksum for group %u failed (%u!=%u)\n",
                   i, le16_to_cpu(ext4_group_desc_csum(sbi, i,
                                                       gdp)),
                   le16_to_cpu(gdp->bg_checksum));
            if (!IsVcbReadOnly(Vcb)) {
                __brelse(bh);
                return 0;
            }
        }

        if (!flexbg_flag)
            first_block += EXT4_BLOCKS_PER_GROUP(sb);

        __brelse(bh);
    }

    ext3_free_blocks_count_set(sbi->s_es, ext4_count_free_blocks(sb));
    sbi->s_es->s_free_inodes_count = cpu_to_le32(ext4_count_free_inodes(sb));
    return 1;
}
