/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             generic.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

BOOLEAN
Ext2IsBlockEmpty(PULONG BlockArray, ULONG SizeArray);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2LoadSuper)
#pragma alloc_text(PAGE, Ext2SaveSuper)
#pragma alloc_text(PAGE, Ext2RefreshSuper)

#pragma alloc_text(PAGE, Ext2LoadGroup)
#pragma alloc_text(PAGE, Ext2SaveGroup)
#pragma alloc_text(PAGE, Ext2RefreshGroup)

#pragma alloc_text(PAGE, Ext2GetInodeLba)
#pragma alloc_text(PAGE, Ext2LoadInode)
#pragma alloc_text(PAGE, Ext2SaveInode)

#pragma alloc_text(PAGE, Ext2LoadBlock)
#pragma alloc_text(PAGE, Ext2SaveBlock)

#pragma alloc_text(PAGE, Ext2SaveBuffer)

#pragma alloc_text(PAGE, Ext2GetBlock)
#pragma alloc_text(PAGE, Ext2BlockMap)

#pragma alloc_text(PAGE, Ext2UpdateVcbStat)
#pragma alloc_text(PAGE, Ext2NewBlock)
#pragma alloc_text(PAGE, Ext2FreeBlock)
#pragma alloc_text(PAGE, Ext2ExpandLast)
#pragma alloc_text(PAGE, Ext2ExpandBlock)

#pragma alloc_text(PAGE, Ext2IsBlockEmpty)
#pragma alloc_text(PAGE, Ext2TruncateBlock)

#pragma alloc_text(PAGE, Ext2NewInode)
#pragma alloc_text(PAGE, Ext2FreeInode)

#pragma alloc_text(PAGE, Ext2AddEntry)
#pragma alloc_text(PAGE, Ext2RemoveEntry)
#pragma alloc_text(PAGE, Ext2SetParentEntry)

#endif

/* FUNCTIONS ***************************************************************/

NTSTATUS
Ext2LoadSuper(IN PEXT2_VCB      Vcb,
              IN BOOLEAN        bVerify,
              OUT PEXT2_SUPER_BLOCK * Sb)
{
    NTSTATUS          Status;
    PEXT2_SUPER_BLOCK Ext2Sb = NULL;
    
    Ext2Sb = (PEXT2_SUPER_BLOCK)
                ExAllocatePoolWithTag(
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

        DEBUG(DL_ERR, ( "Ext2ReadDisk: disk device error.\n"));

        ExFreePoolWithTag(Ext2Sb, EXT2_SB_MAGIC);
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

    return TRUE;
}

BOOLEAN
Ext2LoadGroup(IN PEXT2_VCB Vcb)
{
    ULONG       Size;
    PVOID       Buffer;
    LONGLONG    Lba = 0;
    NTSTATUS    Status;

    Size = sizeof(EXT2_GROUP_DESC) * Vcb->NumOfGroups;

    if (Vcb->BlockSize == EXT2_MIN_BLOCK) {
        Lba = (LONGLONG)2 * Vcb->BlockSize;
    }

    if (Vcb->BlockSize > EXT2_MIN_BLOCK) {
        Lba = (LONGLONG) (Vcb->BlockSize);
    }

    if (Lba == 0) {
        return FALSE;
    }

    Buffer = ExAllocatePoolWithTag(PagedPool, Size, EXT2_GD_MAGIC);
    if (!Buffer) {
        DEBUG(DL_ERR, ( "Ext2LoadSuper: no enough memory.\n"));
        return FALSE;
    }

    DEBUG(DL_INF, ( "Ext2LoadGroup: Lba=%I64xh Size=%xh\n", Lba, Size));

    Status = Ext2ReadDisk(  Vcb,
                            Lba,
                            Size,
                            Buffer,
                            FALSE );

    if (!NT_SUCCESS(Status)) {
        ExFreePoolWithTag(Buffer, EXT2_GD_MAGIC);
        Buffer = NULL;

        return FALSE;
    }

    Vcb->GroupDesc = (PEXT2_GROUP_DESC) Buffer;

    return TRUE;
}

BOOLEAN
Ext2SaveGroup(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Group
    )
{
    LONGLONG    Offset;
    BOOLEAN     bRet;

    if (Vcb->BlockSize == EXT2_MIN_BLOCK) {

        Offset = (LONGLONG) (2 * Vcb->BlockSize);

    } else {

        Offset = (LONGLONG) (Vcb->BlockSize);
    }

    Offset += ((LONGLONG) sizeof(struct ext2_group_desc) * Group);

    bRet = Ext2SaveBuffer(
                    IrpContext,
                    Vcb,
                    Offset,
                    sizeof(struct ext2_group_desc),
                    &(Vcb->GroupDesc[Group]) );

    if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        Ext2StartFloppyFlushDpc(Vcb, NULL, NULL);
    }

    return bRet;
}   


BOOLEAN
Ext2RefreshGroup(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
    )
{
    IO_STATUS_BLOCK iosb;
    LONGLONG        offset;

    if (Vcb->BlockSize == EXT2_MIN_BLOCK) {
        offset = (LONGLONG) (2 * Vcb->BlockSize);
    } else {
        offset = (LONGLONG) (Vcb->BlockSize);
    }

    if (!CcCopyRead(
            Vcb->Volume,
            (PLARGE_INTEGER)&offset,
            sizeof(struct ext2_group_desc) *
            Vcb->NumOfGroups,
            TRUE,
            (PVOID)&Vcb->GroupDesc[0],
            &iosb)) {
        return FALSE;
    }

    if (!NT_SUCCESS(iosb.Status)) {
        return FALSE;
    }

    return TRUE;
}       

BOOLEAN
Ext2GetInodeLba (
    IN PEXT2_VCB    Vcb,
    IN  ULONG       inode,
    OUT PLONGLONG   offset
    )
{
    LONGLONG loc;

    if (inode < 1 || inode > INODES_COUNT)  {
        DEBUG(DL_ERR, ( "Ext2GetInodeLba: Inode value %xh is invalid.\n",inode));
        *offset = 0;
        return FALSE;
    }

    loc = (Vcb->GroupDesc[(inode - 1) / INODES_PER_GROUP ].bg_inode_table);
    loc = loc * Vcb->BlockSize;
    loc = loc + ((inode - 1) % INODES_PER_GROUP) * INODE_SIZE;

    *offset = loc;

    return TRUE;
}

BOOLEAN
Ext2LoadInode (IN PEXT2_VCB Vcb,
           IN ULONG       inode,
           IN PEXT2_INODE Inode)
{
    IO_STATUS_BLOCK     IoStatus;
    LONGLONG            Offset; 

    if (!Ext2GetInodeLba(Vcb, inode, &Offset))  {
        DEBUG(DL_ERR, ( "Ext2LoadInode: error get inode(%xh)'s addr.\n", inode));
        return FALSE;
    }

    if (!CcCopyRead(
            Vcb->Volume,
            (PLARGE_INTEGER)&Offset,
            INODE_SIZE,
            PIN_WAIT,
            (PVOID)Inode,
            &IoStatus )) {
        return FALSE;
    }

    if (!NT_SUCCESS(IoStatus.Status)) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
Ext2SaveInode ( IN PEXT2_IRP_CONTEXT IrpContext,
                IN PEXT2_VCB Vcb,
                IN ULONG Inode,
                IN PEXT2_INODE Ext2Inode)
{
    LONGLONG        Offset = 0;
    BOOLEAN         rc;

    DEBUG(DL_INF, ( "Ext2SaveInode: Saving Inode %xh: Mode=%xh Size=%xh\n",
                         Inode, Ext2Inode->i_mode, Ext2Inode->i_size));
    rc = Ext2GetInodeLba(Vcb, Inode, &Offset);
    if (!rc)  {
        DEBUG(DL_ERR, ( "Ext2SaveInode: error get inode(%xh)'s addr.\n", Inode));
        goto errorout;
    }

    rc = Ext2SaveBuffer(IrpContext, Vcb, Offset, INODE_SIZE, Ext2Inode);

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
    IO_STATUS_BLOCK     IoStatus;
    LONGLONG            Offset; 

    Offset = (LONGLONG) Index;
    Offset = Offset * Vcb->BlockSize;

    if (!CcCopyRead(
            Vcb->Volume,
            (PLARGE_INTEGER)&Offset,
            Vcb->BlockSize,
            PIN_WAIT,
            Buffer,
            &IoStatus ));

    if (!NT_SUCCESS(IoStatus.Status)) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
Ext2SaveBlock ( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN ULONG                Index,
                IN PVOID                Buf )
{
    LONGLONG Offset;
    BOOLEAN  rc;

    Offset = (LONGLONG) Index;
    Offset = Offset * Vcb->BlockSize;

    rc = Ext2SaveBuffer(IrpContext, Vcb, Offset, Vcb->BlockSize, Buf);

    if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        Ext2StartFloppyFlushDpc(Vcb, NULL, NULL);
    }

    return rc;
}

BOOLEAN
Ext2SaveBuffer( IN PEXT2_IRP_CONTEXT    IrpContext,
                IN PEXT2_VCB            Vcb,
                IN LONGLONG             Offset,
                IN ULONG                Size,
                IN PVOID                Buf )
{
    PBCB        Bcb;
    PVOID       Buffer;
    BOOLEAN     rc;

    if( !CcPreparePinWrite(
                    Vcb->Volume,
                    (PLARGE_INTEGER) (&Offset),
                    Size,
                    FALSE,
                    Ext2CanIWait(),
                    &Bcb,
                    &Buffer )) {

        DEBUG(DL_ERR, ( "Ext2SaveBuffer: failed to PinLock offset %I64xh ...\n", Offset));
        return FALSE;
    }

    __try {

        RtlCopyMemory(Buffer, Buf, Size);
        CcSetDirtyPinnedData(Bcb, NULL );
        SetFlag(Vcb->Volume->Flags, FO_FILE_MODIFIED);

        rc = Ext2AddVcbExtent(Vcb, Offset, (LONGLONG)Size);
        if (!rc) {
            DbgBreak();
            Ext2Sleep(100);
            rc = Ext2AddVcbExtent(Vcb, Offset, (LONGLONG)Size);
        }

    } __finally {
        CcUnpinData(Bcb);
    }


    return rc;
}

NTSTATUS
Ext2GetBlock(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Base,
    IN ULONG                Layer,
    IN ULONG                Start,
    IN ULONG                SizeArray,
    IN PULONG               BlockArray,
    IN BOOLEAN              bAlloc,
    IN OUT PULONG           Hint,
    OUT PULONG              Block,
    OUT PULONG              Number
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    PBCB        Bcb = NULL;    
    PULONG      pData = NULL;
    ULONG       Slot = 0, i = 0;
    ULONG       Unit = 1;

    LARGE_INTEGER Offset;

    if (Layer == 0) {

        *Number = 1;
        if (BlockArray[0] == 0 && bAlloc) {

            /* now allocate new block */
            Status = Ext2ExpandLast(
                        IrpContext,
                        Vcb,
                        Mcb,
                        Base,
                        Layer,
                        NULL,
                        Hint,
                        &BlockArray[0],
                        Number
                        );

            if (!NT_SUCCESS(Status)) {
                 goto errorout;
             }
        }

        *Block = BlockArray[0];
        for (i=1; i < SizeArray; i++) {
            if (BlockArray[i] == BlockArray[i-1] + 1) {
                *Number = *Number + 1;
            } else {
                break;
            }
        }
        *Hint = BlockArray[*Number - 1];

    } else if (Layer <= 3) {

        /* check the block is valid or not */
        if (BlockArray[0] >= TOTAL_BLOCKS) {
            DbgBreak();
            return STATUS_DISK_CORRUPT_ERROR;
        }

        /* map memory in cache for the index block */
        Offset.QuadPart = ((LONGLONG)BlockArray[0]) << BLOCK_BITS;
        if( !CcPinRead( Vcb->Volume,
                    (PLARGE_INTEGER) (&Offset),
                    BLOCK_SIZE,
                    Ext2CanIWait(),
                    &Bcb,
                    &pData )) {

            DEBUG(DL_ERR, ( "Ext2GetBlock: Failed to PinLock block: %xh ...\n",
                                   BlockArray[0] ));
            Status = STATUS_CANT_WAIT;
            goto errorout;
        }
 

        if (Layer > 1) {
            Unit = Vcb->NumBlocks[Layer - 1];
        } else {
            Unit = 1; 
        }

        Slot  = Start / Unit;
        Start = Start % Unit;
       
        if (pData[Slot] == 0) {

            if (bAlloc) {

                /* we need allocate new block and zero all data in case
                   it's an in-direct block. Index stores the new block no. */
                ULONG   Count = 1;
                Status = Ext2ExpandLast(
                            IrpContext,
                            Vcb,
                            Mcb,
                            Base,
                            Layer,
                            NULL,
                            Hint,
                            &pData[Slot],
                            &Count
                            );

                if (!NT_SUCCESS(Status)) {
                    goto errorout;
                }

                /* refresh hint block */
                *Hint = pData[Slot];

                /* set dirty bit to notify system to flush */
                CcSetDirtyPinnedData(Bcb, NULL );
                SetFlag(Vcb->Volume->Flags, FO_FILE_MODIFIED);
                if (!Ext2AddVcbExtent(Vcb, Offset.QuadPart,
                                     (LONGLONG)BLOCK_SIZE)) {
                    DbgBreak();
                    Ext2Sleep(100);
                    if (!Ext2AddVcbExtent(Vcb, Offset.QuadPart,
                                     (LONGLONG)BLOCK_SIZE)) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto errorout;
                    }
                }

                /* save inode information here */
                Ext2SaveInode(IrpContext, Vcb, Mcb->iNo,  Mcb->Inode);
  
            } else {

                *Number = 1;

                if (Layer == 1) {
                    for (i = Slot + 1; i < BLOCK_SIZE/4; i++) {
                        if (pData[i] == 0) {
                            *Number = *Number + 1;
                        } else {
                            break;
                        }
                    }
                } else if (Layer == 2) {
                    *Number = BLOCK_SIZE/4 - Start;
                } else {
                    *Number = BLOCK_SIZE/4;
                }

                goto errorout;
            }
        }

        /* transfer to next recursion call */
        Status = Ext2GetBlock(
                    IrpContext,
                    Vcb,
                    Mcb,
                    Base,
                    Layer - 1,
                    Start,
                    BLOCK_SIZE/4 - Slot,
                    &pData[Slot],
                    bAlloc,
                    Hint,
                    Block,
                    Number
                    );

        if (!NT_SUCCESS(Status)) {
            goto errorout;
        }
    }

errorout:

    /* free the memory of pData */
    if (Bcb) {
        CcUnpinData(Bcb);
    }

    if (!NT_SUCCESS(Status)) {
        *Block = 0;
    }

    return Status;
}

NTSTATUS
Ext2BlockMap(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Index,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock,
    OUT PULONG              Number
    )
{
    ULONG   Layer;
    ULONG   Slot;

    ULONG   Base = Index;

    NTSTATUS Status = STATUS_SUCCESS;

    *pBlock = 0;
    *Number = 0;

    for (Layer = 0; Layer < EXT2_BLOCK_TYPES; Layer++) {

        if (Index < Vcb->NumBlocks[Layer]) {

            ULONG   dwRet = 0;
            ULONG   dwBlk = 0;
            ULONG   dwHint = 0;

            Slot = (Layer==0) ? (Index):(Layer + EXT2_NDIR_BLOCKS - 1);
            dwBlk = Mcb->Inode->i_block[Slot];

            if (dwBlk == 0) {

                if (!bAlloc) {

                    *Number = 1;
                    goto errorout;

                } else {

                    if (Slot) {
                        dwHint = Mcb->Inode->i_block[Slot - 1];
                    }

                    /* allocate and zero block if necessary */
                    *Number = 1;
                    Status = Ext2ExpandLast(
                                IrpContext,
                                Vcb,
                                Mcb,
                                Base,
                                Layer,
                                NULL,
                                &dwHint,
                                &dwBlk,
                                Number
                                );

                    if (!NT_SUCCESS(Status)) {
                        goto errorout;
                    }

                    /* save the it into inode*/
                    Mcb->Inode->i_block[Slot] = dwBlk;

                    /* save the inode */
                    if (!Ext2SaveInode( IrpContext,
                                        Vcb,
                                        Mcb->iNo,
                                        Mcb->Inode)) {

                        Status = STATUS_UNSUCCESSFUL;
                        Ext2FreeBlock(IrpContext, Vcb, dwBlk, 1);

                        goto errorout;
                    }
                }
            }

            /* querying block number of the index-th file block */
           Status = Ext2GetBlock(
                        IrpContext,
                        Vcb,
                        Mcb,
                        Base,
                        Layer,
                        Index,
                        (Layer == 0) ? (Vcb->NumBlocks[Layer] - Index) : 1,
                        &Mcb->Inode->i_block[Slot],
                        bAlloc,
                        &dwHint,
                        &dwRet,
                        Number
                        );

            if (NT_SUCCESS(Status)) {
                *pBlock = dwRet;
            }

            break;
        }

        Index -= Vcb->NumBlocks[Layer];
    }

errorout:

    return Status;
}

VOID
Ext2UpdateVcbStat(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
    )
{
    ULONG i;

    Vcb->SuperBlock->s_free_blocks_count = 0;
    Vcb->SuperBlock->s_free_inodes_count = 0;
    
    for (i = 0; i < Vcb->NumOfGroups; i++) {
        Vcb->SuperBlock->s_free_blocks_count +=
            Vcb->GroupDesc[i].bg_free_blocks_count;
        Vcb->SuperBlock->s_free_inodes_count +=
            Vcb->GroupDesc[i].bg_free_inodes_count;
    }

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
    RTL_BITMAP              BlockBitmap;
    LARGE_INTEGER           Offset;

    PBCB                    BitmapBcb;
    PVOID                   BitmapCache;

    ULONG                   Group = 0;
    ULONG                   Index = 0xFFFFFFFF;
    ULONG                   dwHint = 0;
    ULONG                   Count = 0;
    ULONG                   Length = 0;

    NTSTATUS                Status = STATUS_DISK_FULL;

    *Block = 0;

    ExAcquireResourceExclusiveLite(&Vcb->MetaLock, TRUE);

    /* validate the hint group and hint block */ 
    if (GroupHint >= Vcb->NumOfGroups) {
        DbgBreak();
        GroupHint = Vcb->NumOfGroups - 1;
    }

    if (BlockHint != 0) {
        GroupHint = (BlockHint - EXT2_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;
        dwHint = (BlockHint - EXT2_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;
    }

    Group = GroupHint;

Again:

    if (Vcb->GroupDesc[Group].bg_free_blocks_count) {

        /* check the block is valid or not */
        if (Vcb->GroupDesc[Group].bg_block_bitmap >= TOTAL_BLOCKS) {
            DbgBreak();
            Status = STATUS_DISK_CORRUPT_ERROR;
            goto errorout;
        }

        Offset.QuadPart = Vcb->GroupDesc[Group].bg_block_bitmap;
        Offset.QuadPart = Offset.QuadPart << BLOCK_BITS;

        if (Group == Vcb->NumOfGroups - 1) {

            Length = TOTAL_BLOCKS % BLOCKS_PER_GROUP;

            /* s_blocks_count is integer multiple of s_blocks_per_group */
            if (Length == 0) {
                Length = BLOCKS_PER_GROUP;
            }
        } else {
            Length = BLOCKS_PER_GROUP;
        }

        if (!CcPinRead( Vcb->Volume,
                        &Offset,
                        Vcb->BlockSize,
                        Ext2CanIWait(),
                        &BitmapBcb,
                        &BitmapCache ) ) {

            DEBUG(DL_ERR, ("Ext2NewBlock: failed to PinLock bitmap block %xh ...\n",
                            Vcb->GroupDesc[Group].bg_block_bitmap ));
            Status = STATUS_CANT_WAIT;
            DbgBreak();
            goto errorout;
        }

        /* initialize bitmap buffer */
        RtlInitializeBitMap(&BlockBitmap, BitmapCache, Length);

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

                /* release bitmap */
                CcUnpinData(BitmapBcb);
                BitmapBcb = NULL;
                BitmapCache = NULL;
                RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));

                /* no blocks found: set bg_free_blocks_count to 0 */
                Vcb->GroupDesc[Group].bg_free_blocks_count = 0;
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
        Group = (Group + 1) % Vcb->NumOfGroups;
        if (Group != GroupHint) {
            goto Again;
        }

        Index = 0xFFFFFFFF;
    }

    if (Index < Length) {

        /* mark block bits as allocated */
        RtlSetBits(&BlockBitmap, Index, *Number);

        /* update group description */
        Vcb->GroupDesc[Group].bg_free_blocks_count =
                (USHORT)RtlNumberOfClearBits(&BlockBitmap);

        /* set block bitmap dirty in cache */
        CcSetDirtyPinnedData(BitmapBcb, NULL);
        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        Ext2SaveGroup(IrpContext, Vcb, Group);

        /* add group bitmap block to dirty range */
        Ext2AddVcbExtent(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);

        /* update Vcb free blocks */
        Ext2UpdateVcbStat(IrpContext, Vcb);

        /* validate the new allocated block number */
        *Block = Index + EXT2_FIRST_DATA_BLOCK + Group * BLOCKS_PER_GROUP;
        if (*Block >= TOTAL_BLOCKS || *Block + *Number >= TOTAL_BLOCKS) {
            DbgBreak();
            dwHint = 0;
            goto Again;
        }

        {
            ULONG i=0;
            for (i=0; i < Vcb->NumOfGroups; i++)  {
                if ((Vcb->GroupDesc[i].bg_block_bitmap == *Block) ||
                    (Vcb->GroupDesc[i].bg_inode_bitmap == *Block) ||
                    (Vcb->GroupDesc[i].bg_inode_table == *Block) ) {
                    DbgBreak();
                    dwHint = 0;
                    goto Again;
                }
            }
        }

       Status = STATUS_SUCCESS;
    }

errorout:

    ExReleaseResourceLite(&Vcb->MetaLock);

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
    RTL_BITMAP      BlockBitmap;
    LARGE_INTEGER   Offset;

    PBCB            BitmapBcb;
    PVOID           BitmapCache;

    ULONG           Group;
    ULONG           Index;
    ULONG           Length;
    ULONG           Count;

    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    ExAcquireResourceExclusiveLite(&Vcb->MetaLock, TRUE);

    DEBUG(DL_INF, ("Ext2FreeBlock: Block %xh to be freed.\n", Block));

    Group = (Block - EXT2_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;
    Index = (Block - EXT2_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;

Again:

    if ( Block < EXT2_FIRST_DATA_BLOCK || 
         Block >= TOTAL_BLOCKS ||
         Group >= Vcb->NumOfGroups) {

        DbgBreak();
        Status = STATUS_SUCCESS;

    } else  {

        /* check the block is valid or not */
        if (Vcb->GroupDesc[Group].bg_block_bitmap >= TOTAL_BLOCKS) {
            DbgBreak();
            Status = STATUS_DISK_CORRUPT_ERROR;
            goto errorout;
        }

        /* get bitmap block offset and length */
        Offset.QuadPart = Vcb->GroupDesc[Group].bg_block_bitmap;
        Offset.QuadPart = Offset.QuadPart << BLOCK_BITS;

        if (Group == Vcb->NumOfGroups - 1) {

            Length = TOTAL_BLOCKS % BLOCKS_PER_GROUP;

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
                        Ext2CanIWait(),
                        &BitmapBcb,
                        &BitmapCache ) ) {

            DEBUG(DL_ERR, ("Ext2FreeBlock: failed to PinLock bitmap block %xh.\n",
                            Vcb->GroupDesc[Group].bg_block_bitmap));
            Status = STATUS_CANT_WAIT;
            DbgBreak();
            goto errorout;
        }

        /* clear usused bits */
        RtlInitializeBitMap(&BlockBitmap, BitmapCache, Length);
        Count = min(Length - Index, Number);
        RtlClearBits(&BlockBitmap, Index, Count);

        /* update group description table */
        Vcb->GroupDesc[Group].bg_free_blocks_count = 
            (USHORT)RtlNumberOfClearBits(&BlockBitmap);

        /* indict the cache range is dirty */
        CcSetDirtyPinnedData(BitmapBcb, NULL );
        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        BitmapCache = NULL;
        Ext2AddVcbExtent(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);
        Ext2SaveGroup(IrpContext, Vcb, Group);

        /* save super block (used/unused blocks statics) */
        Ext2UpdateVcbStat(IrpContext, Vcb);

        /* try next group to clear all remaining */
        Number -= Count;
        if (Number) {
            Group += 1;
            if (Group < Vcb->NumOfGroups) {
                Index = 0; Block += Count;
                goto Again;
            } else {
                DEBUG(DL_ERR, ("Ext2FreeBlock: block number beyonds max group.\n"));
                goto errorout;
            }
        }
    }

    Status = STATUS_SUCCESS;

errorout:

    ExReleaseResourceLite(&Vcb->MetaLock);

    return Status;
}

NTSTATUS
Ext2ExpandLast(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Base,
    IN ULONG                Layer,
    IN PULONG *             Data,
    IN PULONG               Hint,
    IN PULONG               Block,
    IN OUT PULONG           Number
    )
{
    PULONG      pData = NULL;
    ULONG       i;
    NTSTATUS    Status = STATUS_SUCCESS;

    if (Layer > 0 || IsMcbDirectory(Mcb)) {

        /* allocate buffer for new block */
        pData = (ULONG *) ExAllocatePoolWithTag(
                    PagedPool,
                    BLOCK_SIZE,
                    EXT2_DATA_MAGIC
                    );
        if (!pData) {
            DEBUG(DL_ERR, ( "Ex2ExpandBlock: failed to allocate memory for Data.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }
        RtlZeroMemory(pData, BLOCK_SIZE);
        INC_MEM_COUNT(PS_BLOCK_DATA, pData, BLOCK_SIZE);
    }

    /* allocate block from disk */
    Status = Ext2NewBlock(
                IrpContext,
                Vcb,
                (Mcb->iNo - 1) / BLOCKS_PER_GROUP,
                *Hint,
                Block,
                Number
                );

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    /* increase inode i_blocks */
    Mcb->Inode->i_blocks += (*Number * (BLOCK_SIZE >> 9));

    if (Layer == 0) {

        if (IsMcbDirectory(Mcb)) {

            /* for directory we need initialize it's entry structure */
            PEXT2_DIR_ENTRY2 pEntry;
            pEntry = (PEXT2_DIR_ENTRY2) pData;
            pEntry->rec_len = (USHORT)(BLOCK_SIZE);
            ASSERT(*Number == 1);
            Ext2SaveBlock(IrpContext, Vcb, *Block, (PVOID)pData);

        } else {

            /* for file we need remove dirty MCB to prevent Volume's writing */
            if (!Ext2RemoveBlockExtent(Vcb, NULL, (*Block), *Number)) {
                DbgBreak();
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }
        }

        /* add new Extent into Mcb */
        if (!Ext2AddBlockExtent(Vcb, Mcb, Base, (*Block), *Number)) {
            DbgBreak();
            ClearFlag(Mcb->Flags, MCB_ZONE_INIT);
            Ext2ClearAllExtents(&Mcb->Extents);
        }

    } else {

        /* zero the content of all meta blocks */
        for (i = 0; i < *Number; i++) {
            Ext2SaveBlock(IrpContext, Vcb, *Block + i, (PVOID)pData);
        }
    }

errorout:

    if (NT_SUCCESS(Status)) {
        *Hint = *Block + *Number;
        if (Data) {
            *Data = pData;
            ASSERT(*Number == 1);
        } else {
            if (pData) {
                ExFreePoolWithTag(pData, EXT2_DATA_MAGIC);
                DEC_MEM_COUNT(PS_BLOCK_DATA, pData, BLOCK_SIZE);
            }
        }
    } else {
        if (pData) {
            ExFreePoolWithTag(pData, EXT2_DATA_MAGIC);
            DEC_MEM_COUNT(PS_BLOCK_DATA, pData, BLOCK_SIZE);
        }
        if (*Block) {
            Ext2FreeBlock(IrpContext, Vcb, *Block, *Number);
            *Block = 0;
        }
    }

    return Status;
}

NTSTATUS
Ext2ExpandBlock(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN PEXT2_MCB         Mcb,
    IN ULONG             Base,
    IN ULONG             Layer,
    IN ULONG             Start,
    IN ULONG             SizeArray,
    IN PULONG            BlockArray,
    IN PULONG            Hint,
    IN PULONG            Extra
    )
{
    ULONG       i = 0;
    ULONG       j;
    ULONG       Slot;
    ULONG       Block = 0;
    LARGE_INTEGER Offset;

    PBCB        Bcb = NULL;
    PULONG      pData = NULL;
    ULONG       Skip = 0;

    ULONG       Number;
    ULONG       Wanted;

    NTSTATUS    Status = STATUS_SUCCESS;

    if (Layer == 1) {

        /* try to make all leaf block continuous to avoid fragments */
        Number = min(SizeArray, ((*Extra + (Start & (BLOCK_SIZE/4 - 1))) * 4 / BLOCK_SIZE));
        Wanted = 0;
        DEBUG(DL_BLK, ("Ext2ExpandBlock: SizeArray=%xh Extra=%xh Start=%xh %xh\n",
                        SizeArray, *Extra, Start, Number ));

        for (i=0; i < Number; i++) {
            if (BlockArray[i] == 0) {
                Wanted += 1;
            }
        }

        i = 0;
        while (Wanted > 0) {

            Number = Wanted;
            Status = Ext2ExpandLast(
                        IrpContext,
                        Vcb,
                        Mcb,
                        Base,
                        Layer,
                        NULL,
                        Hint,
                        &Block,
                        &Number
                        );
            if (!NT_SUCCESS(Status)) {
                goto errorout;
            }

            ASSERT(Number > 0);
            Wanted -= Number;
            while (Number) {
                if (BlockArray[i] == 0) {
                    BlockArray[i] = Block++;
                    Number--;
                }
                i++;
            }
        }

    } else if (Layer == 0) {

        /* bulk allocation for inode data blocks */

        i = 0;

        while (*Extra && i < SizeArray) {

            Wanted = 0;
            Number = 1;

            for (j = i; j < SizeArray && j < i + *Extra; j++) {

                if (BlockArray[j] >= TOTAL_BLOCKS) {
                    DbgBreak();
                    BlockArray[j] = 0;
                }

                if (BlockArray[j] == 0) {
                    Wanted += 1;
                } else {
                    break;
                }
            }

            if (Wanted == 0) {

                /* add block extent into Mcb */
                ASSERT(BlockArray[i] != 0);
                if (!Ext2AddBlockExtent(Vcb, Mcb, Base + i, BlockArray[i], 1)) {
                    DbgBreak();
                    ClearFlag(Mcb->Flags, MCB_ZONE_INIT);
                    Ext2ClearAllExtents(&Mcb->Extents);
                }

            } else {

                Number = Wanted;
                Status = Ext2ExpandLast(
                                IrpContext,
                                Vcb,
                                Mcb,
                                Base + i,
                                0,
                                NULL,
                                Hint,
                                &Block,
                                &Number
                                );
                if (!NT_SUCCESS(Status)) {
                    goto errorout;
                }

                ASSERT(Number > 0);
                for (j = 0; j < Number; j++) {
                    BlockArray[i + j] = Block++;
                }
            }

            *Extra -= Number;
            i += Number;
        }

        goto errorout;
    }

    for (i = 0; *Extra && i < SizeArray; i++) {

        if (Layer <= 3) {

            if (BlockArray[i] >= TOTAL_BLOCKS) {
                DbgBreak();
                BlockArray[i] = 0;
            }

            if (BlockArray[i] == 0) {
                Number = 1;
                Status = Ext2ExpandLast(
                            IrpContext,
                            Vcb,
                            Mcb,
                            Base,
                            Layer,
                            &pData,
                            Hint,
                            &BlockArray[i],
                            &Number
                            );
                if (!NT_SUCCESS(Status)) {
                    goto errorout;
                }

            } else {

                Offset.QuadPart = (((LONGLONG)BlockArray[i]) << BLOCK_BITS);
                if (!CcPinRead(
                            Vcb->Volume,
                            &Offset,
                            BLOCK_SIZE,
                            Ext2CanIWait(),
                            &Bcb,
                            &pData )) {

                    DEBUG(DL_ERR, ( "Ext2ExpandInode: failed to PinLock offset :%I64xh...\n",
                                           Offset.QuadPart));
                    Status = STATUS_CANT_WAIT;
                    DbgBreak();
                    goto errorout;
                }
            }

            Skip = Vcb->NumBlocks[Layer] * i;

            if (i == 0) {
                if (Layer > 1) {
                    Slot  = Start / Vcb->NumBlocks[Layer - 1];
                    Start = Start % Vcb->NumBlocks[Layer - 1];
                    Skip += Slot * Vcb->NumBlocks[Layer - 1];
                } else {
                    Slot  = Start;
                    Start = 0;
                    Skip += Slot;
                }
            } else {
                Start = 0;
                Slot  = 0;
            }

            Status = Ext2ExpandBlock(
                            IrpContext,
                            Vcb,
                            Mcb,
                            Base + Skip,
                            Layer - 1,
                            Start,
                            BLOCK_SIZE/4 - Slot,
                            &pData[Slot],
                            Hint,
                            Extra
                            );

            if (Bcb) {
                CcSetDirtyPinnedData(Bcb, NULL);
                if (!Ext2AddBlockExtent(Vcb, NULL, 
                                        BlockArray[i],
                                        BlockArray[i], 1)) {
                    DbgBreak();
                    Ext2Sleep(500);
                    if (!Ext2AddBlockExtent(Vcb, NULL,
                                        BlockArray[i],
                                        BlockArray[i], 1)) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto errorout;
                    }
                }
            } else {
                Ext2SaveBlock(IrpContext, Vcb, BlockArray[i], (PVOID)pData);
            }

            if (pData) {
                if (Bcb) {
                    CcUnpinData(Bcb);
                    Bcb = NULL;
                } else {
                    ExFreePoolWithTag(pData, EXT2_DATA_MAGIC);
                    DEC_MEM_COUNT(PS_BLOCK_DATA, pData, BLOCK_SIZE);
                }
                pData = NULL;
            }

            if (!NT_SUCCESS(Status)) {
                DbgBreak();
                break;
            }
        }
    }

errorout:

    return Status;
}

BOOLEAN
Ext2IsBlockEmpty(PULONG BlockArray, ULONG SizeArray)
{
    ULONG i = 0;
    for (i=0; i < SizeArray; i++) {
        if (BlockArray[i]) {
            break;
        }
    }
    return (i == SizeArray);
}


NTSTATUS
Ext2TruncateBlock(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN PEXT2_MCB         Mcb,
    IN ULONG             Base,
    IN ULONG             Start,
    IN ULONG             Layer,
    IN ULONG             SizeArray,
    IN PULONG            BlockArray,
    IN PULONG            Extra
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       i = 0;
    ULONG       Slot = 0;
    ULONG       Skip = 0;

    LONGLONG    Offset;
    PBCB        Bcb = NULL;
    PULONG      pData = NULL;

    for (i = 0; i < SizeArray; i++) {

        if (Layer == 0) {

            ULONG   Number = 1;

            while (Extra &&  SizeArray > i + 1 && Number < *Extra) {

                if (BlockArray[SizeArray - i - 1] ==
                    BlockArray[SizeArray - i - 2] + 1) {

                    BlockArray[SizeArray - i - 1] = 0;
                    Number++;
                    SizeArray--;

                } else {
                    break;
                }
            }

            if (BlockArray[SizeArray - i - 1]) {

                Status = Ext2FreeBlock(IrpContext, Vcb, BlockArray[SizeArray - i - 1], Number);
                if (NT_SUCCESS(Status)) {
#if EXT2_DEBUG
                    if (i == 0 || i == SizeArray - 1 || *Extra == Number) {
                        DEBUG(DL_BLK, ("Ext2TruncateBlock: Vbn: %xh Lbn: %xh Num: %xh\n",
                            Base + SizeArray - 1 - i, BlockArray[SizeArray - i - 1], Number));
                    }
#endif
                    ASSERT(Mcb->Inode->i_blocks >= Number * (BLOCK_SIZE >> 9));
                    if (Mcb->Inode->i_blocks < Number * (BLOCK_SIZE >> 9)) {
                        Mcb->Inode->i_blocks = 0;
                        DbgBreak();
                    } else {
                        Mcb->Inode->i_blocks -= Number * (BLOCK_SIZE >> 9);
                    }
                    BlockArray[SizeArray - i - 1] = 0;
                }
            }

            if (Extra) {

                /* dec blocks count */
                ASSERT(*Extra >= Number);
                *Extra = *Extra - Number;

                /* remove block mapping frm Mcb Extents */
                if (!Ext2RemoveBlockExtent(Vcb, Mcb, Base + SizeArray - 1 - i, Number)) {
                    DbgBreak();
                    ClearFlag(Mcb->Flags, MCB_ZONE_INIT);
                    Ext2ClearAllExtents(&Mcb->Extents);
                }
            }

        } else {

            ASSERT(Layer <= 3);

            if (BlockArray[SizeArray - i - 1] >= TOTAL_BLOCKS) {
                DbgBreak();
                BlockArray[SizeArray - i - 1] = 0;
            }

            if (i == 0) {
                if (Layer > 1) {
                    Slot  = Start / Vcb->NumBlocks[Layer - 1];
                    Start = Start % Vcb->NumBlocks[Layer - 1];
                } else {
                    Slot  = Start;
                    Start = (BLOCK_SIZE / 4) - 1;
                }
            } else {
                Slot = Start = (BLOCK_SIZE / 4) - 1;
            }

            Skip = (SizeArray - i - 1) * Vcb->NumBlocks[Layer];

            if (BlockArray[SizeArray - i - 1]) {

                Offset = (LONGLONG) (BlockArray[SizeArray - i - 1]);
                Offset = Offset << BLOCK_BITS;

                if (!CcPinRead( Vcb->Volume,
                                (PLARGE_INTEGER) (&Offset),
                                BLOCK_SIZE,
                                Ext2CanIWait(),
                                &Bcb,
                                &pData )) {

                    DEBUG(DL_ERR, ( "Ext2TruncateBlock: PinLock failed on block %xh ...\n",
                                     BlockArray[SizeArray - i - 1]));
                    Status = STATUS_CANT_WAIT;
                    DbgBreak();
                    goto errorout;
                }

                Status = Ext2TruncateBlock(
                            IrpContext,
                            Vcb,
                            Mcb,
                            Base + Skip,
                            Start,
                            Layer - 1,
                            Slot + 1,
                            &pData[0],
                            Extra
                            );

                if (!NT_SUCCESS(Status)) {
                    break;
                }

                CcSetDirtyPinnedData(Bcb, NULL);
                Ext2AddVcbExtent(Vcb, Offset, (LONGLONG)BLOCK_SIZE);

                if (*Extra || Ext2IsBlockEmpty(pData, BLOCK_SIZE/4)) {

                    Status = Ext2TruncateBlock(
                                IrpContext,
                                Vcb,
                                Mcb,
                                Base + Skip,    /* base */
                                0,              /* start */
                                0,              /* layer */
                                1,
                                &BlockArray[SizeArray - i - 1],
                                NULL
                                );
                }

                if (pData) {
                    CcUnpinData(Bcb);
                    Bcb = NULL;
                    pData = NULL;
                }

            } else {

                if (Layer > 1) {
                    if (*Extra > Slot * Vcb->NumBlocks[Layer - 1] + Start + 1) {
                        *Extra -= (Slot * Vcb->NumBlocks[Layer - 1] + Start + 1);
                    } else {
                        *Extra  = 0;
                    }
                } else {
                    if (*Extra > Slot + 1) {
                        *Extra -= (Slot + 1);
                    } else {
                        *Extra  = 0;
                    }
                }

                if (!Ext2RemoveBlockExtent(Vcb, Mcb, Base + Skip, (Start + 1))) {
                    DbgBreak();
                    ClearFlag(Mcb->Flags, MCB_ZONE_INIT);
                    Ext2ClearAllExtents(&Mcb->Extents);
                }
            }
        }

        if (Extra && *Extra == 0) {
            break;
        }
    }

errorout:

    if (pData) {
        CcUnpinData(Bcb);
    }

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
    RTL_BITMAP      InodeBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    ULONG           Group, i, j;
    ULONG           Average, Length;
    LARGE_INTEGER   Offset;
    
    ULONG           dwInode;

    NTSTATUS        Status = STATUS_DISK_FULL;

    *Inode = dwInode = 0XFFFFFFFF;

    ExAcquireResourceExclusiveLite(&Vcb->MetaLock, TRUE);

repeat:

    Group = i = 0;
    
    if (Type == EXT2_FT_DIR) {

        Average = Vcb->SuperBlock->s_free_inodes_count / Vcb->NumOfGroups;

        for (j = 0; j < Vcb->NumOfGroups; j++) {

            i = (j + GroupHint) % (Vcb->NumOfGroups);

            if ((Vcb->GroupDesc[i].bg_used_dirs_count << 8) < 
                 Vcb->GroupDesc[i].bg_free_inodes_count ) {
                Group = i + 1;
                break;
            }
        }

        if (!Group) {

            /* get the group with the biggest vacancy */
            for (j = 0; j < Vcb->NumOfGroups; j++) {
                if (!Group) {
                    if (Vcb->GroupDesc[j].bg_free_inodes_count > 0) {
                        Group = j + 1;
                    }
                } else {
                    if ( Vcb->GroupDesc[j].bg_free_blocks_count >
                         Vcb->GroupDesc[Group - 1].bg_free_blocks_count ) {
                        Group = j + 1;
                    }
                }
            }
        }

    } else {

        /*
         * Try to place the inode in its parent directory (GroupHint)
         */

        if (Vcb->GroupDesc[GroupHint].bg_free_inodes_count) {

            Group = GroupHint + 1;

        } else {

            i = GroupHint;

            /*
             * Use a quadratic hash to find a group with a free inode
             */

            for (j = 1; j < Vcb->NumOfGroups; j <<= 1) {

                i = (i + j) % Vcb->NumOfGroups;

                if (Vcb->GroupDesc[i].bg_free_inodes_count) {
                    Group = i + 1;
                    break;
                }
            }
        }

        if (!Group) {
            /*
             * That failed: try linear search for a free inode
             */
            i = GroupHint;
            for (j = 2; j < Vcb->NumOfGroups; j++) {
                i = (i + 1) % Vcb->NumOfGroups;
                if (Vcb->GroupDesc[i].bg_free_inodes_count) {
                    Group = i + 1;
                    break;
                }
            }
        }
    }

    if (Group == 0) {
        goto errorout;
    }

    Group -= 1;

    /* finally we got the group */
    if (Group >= Vcb->NumOfGroups) {
        DbgBreak();
        goto errorout;
    }

    /* check the block is valid or not */
    if (Vcb->GroupDesc[Group].bg_inode_bitmap >= TOTAL_BLOCKS) {
        DbgBreak();
        Status = STATUS_DISK_CORRUPT_ERROR;
        goto errorout;
    }

    /*  this group is used up, we should go here */
    if (Vcb->GroupDesc[Group].bg_inode_bitmap == 0) {
        DbgBreak();
        goto errorout;
    }

    Offset.QuadPart = (LONGLONG) Vcb->GroupDesc[Group].bg_inode_bitmap;
    Offset.QuadPart = Offset.QuadPart << BLOCK_BITS ;

    if (Vcb->NumOfGroups == 1) {
        Length = INODES_COUNT;
    } else {
        if (Group + 1 == Vcb->NumOfGroups) {
            Length = INODES_COUNT % INODES_PER_GROUP;
            if (!Length) {
                /* INODES_COUNT is integer multiple of INODES_PER_GROUP */
                Length = INODES_PER_GROUP;
            }
        } else  {
            Length = INODES_PER_GROUP;
        }
    }
        
    if (!CcPinRead( Vcb->Volume,
                    &Offset,
                    Vcb->BlockSize,
                    Ext2CanIWait(),
                    &BitmapBcb,
                    &BitmapCache ) ) {

        DEBUG(DL_ERR, ( "Ext2NewInode: Failed to PinLock inode bitmap block %xh ...\n",
                         Vcb->GroupDesc[Group].bg_inode_bitmap ));
        Status = STATUS_CANT_WAIT;
        DbgBreak();
        goto errorout;
    }

    RtlInitializeBitMap(&InodeBitmap, BitmapCache, Length);
    dwInode = RtlFindClearBits(&InodeBitmap, 1, 0);

    if (dwInode == 0xFFFFFFFF || dwInode >= Length) {

        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        BitmapCache = NULL;
        RtlZeroMemory(&InodeBitmap, sizeof(RTL_BITMAP));
        if (Vcb->GroupDesc[Group].bg_free_inodes_count > 0) {
            Vcb->GroupDesc[Group].bg_free_inodes_count = 0;
            Ext2SaveGroup(IrpContext, Vcb, Group);
        }
        goto repeat;

    } else {

        RtlSetBits(&InodeBitmap, dwInode, 1);
        Vcb->GroupDesc[Group].bg_free_inodes_count = 
                (USHORT)RtlNumberOfClearBits(&InodeBitmap);
        CcSetDirtyPinnedData(BitmapBcb, NULL);
        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        Ext2AddVcbExtent(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);

        *Inode = dwInode + 1 + Group * INODES_PER_GROUP;

        /* update group_desc / super_block */
        if (Type == EXT2_FT_DIR) {
            Vcb->GroupDesc[Group].bg_used_dirs_count++;
        }
        Ext2SaveGroup(IrpContext, Vcb, Group);
        Ext2UpdateVcbStat(IrpContext, Vcb);
        Status = STATUS_SUCCESS;        
    }

errorout:

    ExReleaseResourceLite(&Vcb->MetaLock);
    return Status;
}

NTSTATUS
Ext2FreeInode(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Inode,
    IN ULONG                Type
    )
{
    RTL_BITMAP      InodeBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    ULONG           Group;
    ULONG           Length;
    LARGE_INTEGER   Offset;

    ULONG           dwIno;
    BOOLEAN         bModified = FALSE;

    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
 
    ExAcquireResourceExclusiveLite(&Vcb->MetaLock, TRUE);

    Group = (Inode - 1) / INODES_PER_GROUP;
    dwIno = (Inode - 1) % INODES_PER_GROUP;

    DEBUG(DL_INF, ( "Ext2FreeInode: Inode: %xh (Group/Off = %xh/%xh)\n",
                         Inode, Group, dwIno));

    if (Group < Vcb->NumOfGroups)  {

        /* check the block is valid or not */
        if (Vcb->GroupDesc[Group].bg_inode_bitmap >= TOTAL_BLOCKS) {
            DbgBreak();
            Status = STATUS_DISK_CORRUPT_ERROR;
            goto errorout;
        }

        Offset.QuadPart = (LONGLONG) Vcb->GroupDesc[Group].bg_inode_bitmap;
        Offset.QuadPart = Offset.QuadPart << BLOCK_BITS ;

        if (Group == Vcb->NumOfGroups - 1) {

            Length = INODES_COUNT % INODES_PER_GROUP;
            if (!Length) {
                /* s_inodes_count is integer multiple of s_inodes_per_group */
                Length = INODES_PER_GROUP;
            }
        } else {
            Length = INODES_PER_GROUP;
        }

        if (!CcPinRead( Vcb->Volume,
                        &Offset,
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &BitmapBcb,
                        &BitmapCache ) ) {
            DEBUG(DL_ERR, ( "Ext2FreeInode: Failed to PinLock inode bitmap %xh ...\n",
                             Vcb->GroupDesc[Group].bg_inode_bitmap));
            Status = STATUS_CANT_WAIT;
            goto errorout;
        }

        RtlInitializeBitMap( &InodeBitmap,
                             BitmapCache,
                             Length );

        if (RtlCheckBit(&InodeBitmap, dwIno) == 0) {
            DbgBreak();
            Status = STATUS_SUCCESS;
        } else {
            RtlClearBits(&InodeBitmap, dwIno, 1);
            bModified = TRUE;
        }

        if (!bModified) {
            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;
            RtlZeroMemory(&InodeBitmap, sizeof(RTL_BITMAP));
        }
    } else {

        DbgBreak();
        goto errorout;
    }
        
    if (bModified) {

        /* update group free inodes */
        Vcb->GroupDesc[Group].bg_free_inodes_count =
            (USHORT)RtlNumberOfClearBits(&InodeBitmap);

        /* set inode block dirty and add to vcb dirty range */
        CcSetDirtyPinnedData(BitmapBcb, NULL );
        CcUnpinData(BitmapBcb);
        Ext2AddVcbExtent(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);

        /* update group_desc and super_block */
        if (Type == EXT2_FT_DIR) {
            Vcb->GroupDesc[Group].bg_used_dirs_count--;
        }
        Ext2SaveGroup(IrpContext, Vcb, Group);
        Ext2UpdateVcbStat(IrpContext, Vcb);
        Status = STATUS_SUCCESS;
    }

errorout:

    ExReleaseResourceLite(&Vcb->MetaLock);
    return Status;
}


NTSTATUS
Ext2AddEntry (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Dcb,
    IN ULONG               FileType,
    IN ULONG               Inode,
    IN PUNICODE_STRING     FileName,
    OUT PULONG             EntryOffset
    )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    OEM_STRING              OemName;

    PEXT2_DIR_ENTRY2        pDir = NULL;
    PEXT2_DIR_ENTRY2        pNewDir = NULL;
    PEXT2_DIR_ENTRY2        pTarget = NULL;

    ULONG                   Length = 0;
    ULONG                   ByteOffset = 0;

    ULONG                   dwRet = 0;
    ULONG                   RecLen = 0;

    BOOLEAN                 bFound = FALSE;
    BOOLEAN                 bAdding = FALSE;

    BOOLEAN                 MainResourceAcquired = FALSE;

    if (!IsDirectory(Dcb)) {
        DbgBreak();
        return STATUS_NOT_A_DIRECTORY;
    }

    ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);
    MainResourceAcquired = TRUE;

    __try {

        Ext2ReferXcb(&Dcb->ReferenceCount);

        pDir = (PEXT2_DIR_ENTRY2) 
                    ExAllocatePoolWithTag(
                        PagedPool,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        EXT2_DENTRY_MAGIC
                    );
        if (!pDir) {
            DEBUG(DL_ERR, ( "Ex2AddEntry: failed to allocate pDir.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        pTarget = (PEXT2_DIR_ENTRY2)
                    ExAllocatePoolWithTag(
                        PagedPool,
                        2 * EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        EXT2_DENTRY_MAGIC
                    );
        if (!pTarget) {
            DEBUG(DL_ERR, ( "Ex2AddEntry: failed to allocate pTarget.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        if (IsFlagOn( SUPER_BLOCK->s_feature_incompat, 
                      EXT2_FEATURE_INCOMPAT_FILETYPE)) {
            pDir->file_type = (UCHAR) FileType;
        } else {
            pDir->file_type = 0;
        }

        OemName.Buffer = pDir->name;
        OemName.MaximumLength = EXT2_NAME_LEN;
        OemName.Length  = 0;

        Status = Ext2UnicodeToOEM(Vcb, &OemName, FileName);

        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        pDir->name_len = (CCHAR) OemName.Length;
        pDir->inode  = Inode;
        pDir->rec_len = (USHORT) (EXT2_DIR_REC_LEN(pDir->name_len));

        ByteOffset = 0;

Repeat:

        while ((LONGLONG)ByteOffset < Dcb->Header.AllocationSize.QuadPart) {

            RtlZeroMemory(pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

            // Reading the DCB contents
            Status = Ext2ReadInode(
                        IrpContext,
                        Vcb,
                        Dcb->Mcb,
                        (ULONGLONG)ByteOffset,
                        (PVOID)pTarget,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        FALSE,
                        &dwRet);

            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_ERR, ( "Ext2AddDirectory: failed to read directory.\n"));
                __leave;
            }

            if (pTarget->rec_len == 0) {
                RecLen = BLOCK_SIZE - (ByteOffset & (BLOCK_SIZE - 1));
            } else {
                RecLen = pTarget->rec_len;
            }

            if (((pTarget->inode == 0) && RecLen >= pDir->rec_len) || 
                (RecLen >= (ULONG)EXT2_DIR_REC_LEN(pTarget->name_len) + pDir->rec_len)) {

                /* we get emply slot for this entry */

                if (pTarget->inode) {

                    RtlZeroMemory(pTarget, 2 * EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

                    /* read enough data from directory content */
                    Status = Ext2ReadInode(
                                IrpContext,
                                Vcb,
                                Dcb->Mcb,
                                (ULONGLONG)ByteOffset,
                                (PVOID)pTarget,
                                2 * EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                                FALSE,
                                &dwRet);

                    if (!NT_SUCCESS(Status)) {
                        DEBUG(DL_ERR, ( "Ext2AddDirectory: Reading Directory Content error.\n"));
                        __leave;
                    }

                    Length = EXT2_DIR_REC_LEN(pTarget->name_len);
                    pNewDir = (PEXT2_DIR_ENTRY2) ((PUCHAR)pTarget + EXT2_DIR_REC_LEN(pTarget->name_len));
                    pNewDir->rec_len = (USHORT)(RecLen - EXT2_DIR_REC_LEN(pTarget->name_len));
                    pTarget->rec_len = EXT2_DIR_REC_LEN(pTarget->name_len);

                } else {

                    Length  = 0;
                    pNewDir = pTarget;
                }

                /* update it's entry offset in parent directory */
                if (EntryOffset) {
                    *EntryOffset = ByteOffset + Length;
                }

                /* set dir entry */
                pNewDir->file_type = pDir->file_type;
                pNewDir->inode = pDir->inode;
                pNewDir->name_len = pDir->name_len;
                memcpy(pNewDir->name, pDir->name, pDir->name_len);

                /* update Length to be written to dir content */
                Length += EXT2_DIR_REC_LEN(pDir->name_len);

                bFound = TRUE;
                break;
            }
            
            ByteOffset += RecLen;
        }

        if (bFound) {

            /* update the reference link count */
            if (FileType==EXT2_FT_DIR ) {

                if(((pDir->name_len == 1) && (pDir->name[0] == '.')) ||
                   ((pDir->name_len == 2) && (pDir->name[0] == '.') && (pDir->name[1] == '.')) ) {
                } else {
                    Dcb->Inode->i_links_count++;
                }
            }

            /* save the new entry */
            Status = Ext2WriteInode(
                        IrpContext,
                        Vcb,
                        Dcb->Mcb,
                        (ULONGLONG)ByteOffset,
                        pTarget,
                        Length,
                        FALSE,
                        &dwRet );
        } else {

            // We should expand the size of the dir inode 
            if (!bAdding) {

                /* allocate new block since there's no space for us */
                ByteOffset = Dcb->Header.AllocationSize.LowPart;
                Dcb->Header.AllocationSize.LowPart += BLOCK_SIZE;
                Status = Ext2ExpandFile(
                                    IrpContext,
                                    Vcb,
                                    Dcb->Mcb,
                                    &(Dcb->Header.AllocationSize)
                                    );

                if (NT_SUCCESS(Status)) {

                    /* update Dcb */
                    Dcb->Inode->i_size = Dcb->Header.AllocationSize.LowPart;
                    Ext2SaveInode(IrpContext, Vcb, Dcb->Mcb->iNo, Dcb->Inode);

                    Dcb->Header.ValidDataLength = Dcb->Header.FileSize = 
                    Dcb->Mcb->FileSize = Dcb->Header.AllocationSize;

                    /* save parent directory's inode */
                    Ext2SaveInode(
                        IrpContext,
                        Vcb,
                        Dcb->Mcb->iNo,
                        Dcb->Inode
                       );

                    bAdding = TRUE;
                    goto Repeat;
                }

                /* failed to allocate new block for this directory */
                __leave;

            } else { 
                /* Something must be error, since we already allocated new block ! */
                __leave;
            }
        }

    } __finally {

        Ext2DerefXcb(&Dcb->ReferenceCount);

        if(MainResourceAcquired)    {
            ExReleaseResourceLite(&Dcb->MainResource);
        }

        if (pTarget != NULL) {
            ExFreePoolWithTag(pTarget, EXT2_DENTRY_MAGIC);
        }

        if (pDir) {
            ExFreePoolWithTag(pDir, EXT2_DENTRY_MAGIC);
        }
    }
    
    return Status;
}


NTSTATUS
Ext2RemoveEntry (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Dcb,
    IN ULONG               EntryOffset,
    IN ULONG               FileType,
    IN ULONG               Inode
    )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PEXT2_DIR_ENTRY2        pTarget = NULL;
    PEXT2_DIR_ENTRY2        pPrevDir = NULL;

    ULONG                   Length = 0;
    ULONG                   ByteOffset = 0;
    ULONG                   dwRet;
    ULONG                   RecLen;

    USHORT                  PrevRecLen = 0;

    BOOLEAN                 MainResourceAcquired = FALSE;


    if (!IsDirectory(Dcb)) {
        return STATUS_NOT_A_DIRECTORY;
    }

    ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);
    MainResourceAcquired = TRUE;

    __try {

        Ext2ReferXcb(&Dcb->ReferenceCount);

        pTarget = (PEXT2_DIR_ENTRY2)
                        ExAllocatePoolWithTag( PagedPool,
                            EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                            EXT2_DENTRY_MAGIC
                        );
        if (!pTarget) {
            DEBUG(DL_ERR, ( "Ex2RemoveEntry: failed to allocate pTarget.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }
        
        pPrevDir = (PEXT2_DIR_ENTRY2)
                        ExAllocatePoolWithTag(
                            PagedPool,
                            EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                            EXT2_DENTRY_MAGIC
                        );
        if (!pPrevDir) {
            DEBUG(DL_ERR, ( "Ex2RemoveEntry: failed to allocate pPrevDir.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        /* we only need search the block where the entry is stored */
        ByteOffset = EntryOffset & (~(BLOCK_SIZE -1));

        while ((LONGLONG)ByteOffset < Dcb->Header.AllocationSize.QuadPart) {

            RtlZeroMemory(pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

            Status = Ext2ReadInode(
                        IrpContext,
                        Vcb,
                        Dcb->Mcb,
                        (ULONGLONG)ByteOffset,
                        (PVOID)pTarget,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        FALSE,
                        &dwRet);

            if (!NT_SUCCESS(Status)) {
                DEBUG(DL_ERR, ( "Ext2RemoveEntry: failed to read directory.\n"));
                __leave;
            }

            if (pTarget->rec_len == 0) {
                DbgBreak();
                RecLen = BLOCK_SIZE - (ByteOffset & (BLOCK_SIZE - 1));
            } else {
                RecLen = pTarget->rec_len;
            }

            /* Find it ! Then remove the entry from Dcb ... */

            if (pTarget->inode == Inode && ByteOffset == EntryOffset) {

                if (ByteOffset & (BLOCK_SIZE - 1)) {

                    /* it doesn't start from a block */

                    ASSERT(pTarget->rec_len != 0);

                    pPrevDir->rec_len += pTarget->rec_len;
                    RecLen = EXT2_DIR_REC_LEN(pTarget->name_len);

                    RtlZeroMemory(pTarget, RecLen);
                    Status = Ext2WriteInode(
                                IrpContext,
                                Vcb,
                                Dcb->Mcb,
                                (ULONGLONG)(ByteOffset - PrevRecLen),
                                pPrevDir,
                                8,
                                FALSE,
                                &dwRet
                                );

                    ASSERT(NT_SUCCESS(Status));

                    Status = Ext2WriteInode(
                                IrpContext,
                                Vcb,
                                Dcb->Mcb,
                                (ULONGLONG)ByteOffset,
                                pTarget,
                                RecLen,
                                FALSE,
                                &dwRet
                                );

                    ASSERT(NT_SUCCESS(Status));

                } else {

                    /* this entry just starts at the block beginning */

                    RecLen = EXT2_DIR_REC_LEN(pTarget->name_len);
                    pTarget->file_type = 0;
                    pTarget->inode = 0;
                    pTarget->name_len = 0;
                    RtlZeroMemory(&pTarget->name[0], EXT2_NAME_LEN);

                    Status = Ext2WriteInode(
                                    IrpContext,
                                    Vcb,
                                    Dcb->Mcb,
                                    (ULONGLONG)ByteOffset,
                                    pTarget,
                                    RecLen,
                                    FALSE,
                                    &dwRet
                                    );

                    ASSERT(NT_SUCCESS(Status));
                }

                //
                // Error if it's the entry of dot or dot-dot or drop the parent's refer link
                //

                if (FileType == EXT2_FT_DIR) {

                    if(((pTarget->name_len == 1) && (pTarget->name[0] == '.')) ||
                       ((pTarget->name_len == 2) && (pTarget->name[0] == '.') && (pTarget->name[1] == '.')) ) {

                        DbgBreak();
                    } else {
                        Dcb->Inode->i_links_count--;
                    }
                }

                //
                // Update at least mtime/atime if !EXT2_FT_DIR.
                //

                if ( !Ext2SaveInode(
                        IrpContext,
                        Vcb,
                        Dcb->Mcb->iNo,
                        Dcb->Inode
                       ) ) {
                    Status = STATUS_UNSUCCESSFUL;
                }
              
                break;

            } else {

                RtlCopyMemory(pPrevDir, pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));
                PrevRecLen = pTarget->rec_len;
            }

            ByteOffset += RecLen;
        }

    } __finally {

        Ext2DerefXcb(&Dcb->ReferenceCount);

        if(MainResourceAcquired)
            ExReleaseResourceLite(&Dcb->MainResource);

        if (pTarget != NULL) {
            ExFreePoolWithTag(pTarget, EXT2_DENTRY_MAGIC);
        }

        if (pPrevDir != NULL) {
            ExFreePoolWithTag(pPrevDir, EXT2_DENTRY_MAGIC);
        }
    }
    
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

    MainResourceAcquired = 
        ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

    __try {

        Ext2ReferXcb(&Dcb->ReferenceCount);

        pSelf = (PEXT2_DIR_ENTRY2)
                    ExAllocatePoolWithTag(
                        PagedPool,
                        EXT2_DIR_REC_LEN(1) + EXT2_DIR_REC_LEN(2),
                        EXT2_DENTRY_MAGIC
                    );
        if (!pSelf) {
            DEBUG(DL_ERR, ( "Ex2SetParentEntry: failed to allocate pSelf.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
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
            __leave;
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

    } __finally {


        if (Ext2DerefXcb(&Dcb->ReferenceCount) == 0) {
            DEBUG(DL_ERR, ( "Ext2SetParentEntry: Dcb reference goes to ZERO.\n"));
        }

        if(MainResourceAcquired)    {
            ExReleaseResourceLite(&Dcb->MainResource);
        }

        if (pSelf) {
            ExFreePoolWithTag(pSelf, EXT2_DENTRY_MAGIC);
        }
    }
    
    return Status;
}
