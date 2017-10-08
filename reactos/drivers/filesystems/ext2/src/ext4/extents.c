/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             extents.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS *****************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#endif


NTSTATUS
Ext2MapExtent(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Index,
    IN BOOLEAN              Alloc,
    OUT PULONG              Block,
    OUT PULONG              Number
)
{
    EXT4_EXTENT_HEADER *eh;
    struct buffer_head bh_got = {0};
    int    flags, rc;
	ULONG max_blocks = 0;

    memset(&bh_got, 0, sizeof(struct buffer_head));
    eh = get_ext4_header(&Mcb->Inode);

    if (eh->eh_magic != EXT4_EXT_MAGIC) {
        if (Alloc) {
            /* now initialize inode extent root node */
            ext4_ext_tree_init(IrpContext, NULL, &Mcb->Inode);
        } else {
            /* return empty-mapping when inode extent isn't initialized */
            if (Block)
                *Block = 0;
            if (Number) {
                LONGLONG  _len = _len = Mcb->Inode.i_size;
                if (Mcb->Fcb)
                    _len = Mcb->Fcb->Header.AllocationSize.QuadPart;
                *Number = (ULONG)((_len + BLOCK_SIZE - 1) >> BLOCK_BITS);
            }
            return STATUS_SUCCESS;
        }
    }

    /* IrpContext is NULL when called during journal initialization */
    if (IsMcbDirectory(Mcb) || IrpContext == NULL ||
        IrpContext->MajorFunction == IRP_MJ_WRITE || !Alloc){
        flags = EXT4_GET_BLOCKS_IO_CONVERT_EXT;
		max_blocks = EXT_INIT_MAX_LEN;
    } else {
        flags = EXT4_GET_BLOCKS_IO_CREATE_EXT;
		max_blocks = EXT_UNWRITTEN_MAX_LEN;
    }
    
    if (Alloc) {
        if (Number && !*Number) {
            if (max_blocks > *Number) {
                max_blocks = *Number;
            }
        } else {
            max_blocks = 1;
        }
    }

    if ((rc = ext4_ext_get_blocks(
                            IrpContext,
                            NULL,
                            &Mcb->Inode,
                            Index,
                            max_blocks,
                            &bh_got,
                            Alloc,
                            flags)) < 0) {
        DEBUG(DL_ERR, ("Block insufficient resources, err: %d\n", rc));
        return Ext2WinntError(rc);
    }
    if (Alloc)
        Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);
    if (Number)
        *Number = rc ? rc : 1;
    if (Block)
        *Block = (ULONG)bh_got.b_blocknr;

    return STATUS_SUCCESS;
}


NTSTATUS
Ext2DoExtentExpand(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Index,
    IN OUT PULONG           Block,
    IN OUT PULONG           Number
)
{
    EXT4_EXTENT_HEADER *eh;
    struct buffer_head bh_got;
    int    rc, flags;

    if (IsMcbDirectory(Mcb) || IrpContext->MajorFunction == IRP_MJ_WRITE) {
        flags = EXT4_GET_BLOCKS_IO_CONVERT_EXT;
    } else {
        flags = EXT4_GET_BLOCKS_IO_CREATE_EXT;
    }

    memset(&bh_got, 0, sizeof(struct buffer_head));
    eh = get_ext4_header(&Mcb->Inode);

    if (eh->eh_magic != EXT4_EXT_MAGIC) {
        ext4_ext_tree_init(IrpContext, NULL, &Mcb->Inode);
    }

    if ((rc = ext4_ext_get_blocks( IrpContext, NULL, &Mcb->Inode, Index,
                                  *Number, &bh_got, 1, flags)) < 0) {
        DEBUG(DL_ERR, ("Expand Block insufficient resources, Number: %u,"
                       " err: %d\n", *Number, rc));
        DbgBreak();
        return Ext2WinntError(rc);
    }

    if (Number)
        *Number = rc ? rc : 1;
    if (Block)
        *Block = (ULONG)bh_got.b_blocknr;

    Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

    return STATUS_SUCCESS;
}


NTSTATUS
Ext2ExpandExtent(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    ULONG             Start,
    ULONG             End,
    PLARGE_INTEGER    Size
    )
{
    ULONG Count = 0, Number = 0, Block = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    if (End <= Start)
        return Status;

    while (End > Start + Count) {

        Number = End - Start - Count;
        Status = Ext2DoExtentExpand(IrpContext, Vcb, Mcb, Start + Count,
                                    &Block, &Number);
        if (!NT_SUCCESS(Status)) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        if (Number == 0) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if (Block && IsZoneInited(Mcb)) {
            if (!Ext2AddBlockExtent(Vcb, Mcb, Start + Count, Block, Number)) {
                DbgBreak();
                ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
                Ext2ClearAllExtents(&Mcb->Extents);
            }
        }
        Count += Number;
    }

    Size->QuadPart = ((LONGLONG)(Start + Count)) << BLOCK_BITS;

    /* save inode whatever it succeeds to expand or not */
    Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

    return Status;
}


NTSTATUS
Ext2TruncateExtent(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG    Extra = 0;
    ULONG    Wanted = 0;
    ULONG    End;
    ULONG    Removed;
    int      err;

    /* translate file size to block */
    End = Vcb->max_data_blocks;
    Wanted = (ULONG)((Size->QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);

    /* calculate blocks to be freed */
    Extra = End - Wanted;

    err = ext4_ext_truncate(IrpContext, &Mcb->Inode, Wanted);
    if (err == 0) {
        if (!Ext2RemoveBlockExtent(Vcb, Mcb, Wanted, Extra)) {
            ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
            Ext2ClearAllExtents(&Mcb->Extents);
        }
        Extra = 0;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(Status)) {
        Size->QuadPart += ((ULONGLONG)Extra << BLOCK_BITS);
    }

    if (Mcb->Inode.i_size > (loff_t)(Size->QuadPart))
        Mcb->Inode.i_size = (loff_t)(Size->QuadPart);

    /* Save modifications on i_blocks field and i_size field of the inode. */
    Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

    return Status;
}
