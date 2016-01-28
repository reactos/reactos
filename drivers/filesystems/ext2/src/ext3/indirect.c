/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             indirect.c
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
        pData = (ULONG *) Ext2AllocatePool(
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
                 (Mcb->Inode.i_ino - 1) / BLOCKS_PER_GROUP,
                 *Hint,
                 Block,
                 Number
             );

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    /* increase inode i_blocks */
    Mcb->Inode.i_blocks += (*Number << (BLOCK_BITS - 9));

    if (Layer == 0) {

        if (IsMcbDirectory(Mcb)) {
            /* for directory we need initialize it's entry structure */
            PEXT2_DIR_ENTRY2 pEntry;
            pEntry = (PEXT2_DIR_ENTRY2) pData;
            pEntry->rec_len = (USHORT)(BLOCK_SIZE);
            ASSERT(*Number == 1);
            Ext2SaveBlock(IrpContext, Vcb, *Block, (PVOID)pData);
        }

        /* add new Extent into Mcb */
        if (!Ext2AddBlockExtent(Vcb, Mcb, Base, (*Block), *Number)) {
            DbgBreak();
            ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
            Ext2ClearAllExtents(&Mcb->Extents);
        }

    } else {

        /* zero the content of all meta blocks */
        for (i = 0; i < *Number; i++) {
            Ext2SaveBlock(IrpContext, Vcb, *Block + i, (PVOID)pData);
            /* add block to meta extents */
            if (!Ext2AddMcbMetaExts(Vcb, Mcb, *Block + i, 1)) {
                DbgBreak();
                Ext2Sleep(500);
                Ext2AddMcbMetaExts(Vcb, Mcb, *Block + i, 1);
            }
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
                Ext2FreePool(pData, EXT2_DATA_MAGIC);
                DEC_MEM_COUNT(PS_BLOCK_DATA, pData, BLOCK_SIZE);
            }
        }
    } else {
        if (pData) {
            Ext2FreePool(pData, EXT2_DATA_MAGIC);
            DEC_MEM_COUNT(PS_BLOCK_DATA, pData, BLOCK_SIZE);
        }
        if (*Block) {
            Ext2FreeBlock(IrpContext, Vcb, *Block, *Number);
            Mcb->Inode.i_blocks -= (*Number << (BLOCK_BITS - 9));
            *Block = 0;
        }
    }

    return Status;
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
        } else {
            /* check the block is valid or not */
            if (BlockArray[0] >= TOTAL_BLOCKS) {
                DbgBreak();
                Status = STATUS_DISK_CORRUPT_ERROR;
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
        if (BlockArray[0] == 0 || BlockArray[0] >= TOTAL_BLOCKS) {
            DbgBreak();
            Status = STATUS_DISK_CORRUPT_ERROR;
            goto errorout;
        }

        /* add block to meta extents */
        if (!Ext2AddMcbMetaExts(Vcb, Mcb, BlockArray[0], 1)) {
            DbgBreak();
            Ext2Sleep(500);
            Ext2AddMcbMetaExts(Vcb, Mcb, BlockArray[0], 1);
        }

        /* map memory in cache for the index block */
        Offset.QuadPart = ((LONGLONG)BlockArray[0]) << BLOCK_BITS;
        if ( !CcPinRead( Vcb->Volume,
                         (PLARGE_INTEGER) (&Offset),
                         BLOCK_SIZE,
                         PIN_WAIT,
                         &Bcb,
                         (void **)&pData )) {

            DEBUG(DL_ERR, ( "Ext2GetBlock: Failed to PinLock block: %xh ...\n",
                            BlockArray[0] ));
            Status = STATUS_CANT_WAIT;
            goto errorout;
        }

        if (Layer > 1) {
            Unit = Vcb->max_blocks_per_layer[Layer - 1];
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
                Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

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

        /*
         * try to make all leaf block continuous to avoid fragments
         */

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

        /*
         * bulk allocation for inode data blocks
         */

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
                    ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
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


    /*
     * only for meta blocks allocation
     */

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
                            PIN_WAIT,
                            &Bcb,
                            (void **)&pData )) {

                    DEBUG(DL_ERR, ( "Ext2ExpandInode: failed to PinLock offset :%I64xh...\n",
                                    Offset.QuadPart));
                    Status = STATUS_CANT_WAIT;
                    DbgBreak();
                    goto errorout;
                }

                /* add block to meta extents */
                if (!Ext2AddMcbMetaExts(Vcb, Mcb,  BlockArray[i], 1)) {
                    DbgBreak();
                    Ext2Sleep(500);
                    Ext2AddMcbMetaExts(Vcb, Mcb,  BlockArray[i], 1);
                }
            }

            Skip = Vcb->max_blocks_per_layer[Layer] * i;

            if (i == 0) {
                if (Layer > 1) {
                    Slot  = Start / Vcb->max_blocks_per_layer[Layer - 1];
                    Start = Start % Vcb->max_blocks_per_layer[Layer - 1];
                    Skip += Slot * Vcb->max_blocks_per_layer[Layer - 1];
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
                    Ext2FreePool(pData, EXT2_DATA_MAGIC);
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

    ASSERT(Mcb != NULL);

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
                    ASSERT(Mcb->Inode.i_blocks >= (Number << (BLOCK_BITS - 9)));
                    if (Mcb->Inode.i_blocks < (Number << (BLOCK_BITS - 9))) {
                        Mcb->Inode.i_blocks = 0;
                        DbgBreak();
                    } else {
                        Mcb->Inode.i_blocks -= (Number << (BLOCK_BITS - 9));
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
                    ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
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
                    Slot  = Start / Vcb->max_blocks_per_layer[Layer - 1];
                    Start = Start % Vcb->max_blocks_per_layer[Layer - 1];
                } else {
                    Slot  = Start;
                    Start = (BLOCK_SIZE / 4) - 1;
                }
            } else {
                Slot = Start = (BLOCK_SIZE / 4) - 1;
            }

            Skip = (SizeArray - i - 1) * Vcb->max_blocks_per_layer[Layer];

            if (BlockArray[SizeArray - i - 1]) {

                Offset = (LONGLONG) (BlockArray[SizeArray - i - 1]);
                Offset = Offset << BLOCK_BITS;

                if (!CcPinRead( Vcb->Volume,
                                (PLARGE_INTEGER) (&Offset),
                                BLOCK_SIZE,
                                PIN_WAIT,
                                &Bcb,
                                (void **)&pData )) {

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

                    Ext2TruncateBlock(
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

                    if (!Ext2RemoveMcbMetaExts(Vcb, Mcb, BlockArray[SizeArray - i - 1], 1)) {
                        DbgBreak();
                        Ext2Sleep(500);
                        Ext2RemoveMcbMetaExts(Vcb, Mcb, BlockArray[SizeArray - i - 1], 1);
                    }
                }

                if (pData) {
                    CcUnpinData(Bcb);
                    Bcb = NULL;
                    pData = NULL;
                }

            } else {

                if (Layer > 1) {
                    if (*Extra > Slot * Vcb->max_blocks_per_layer[Layer - 1] + Start + 1) {
                        *Extra -= (Slot * Vcb->max_blocks_per_layer[Layer - 1] + Start + 1);
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
                    ClearFlag(Mcb->Flags, MCB_ZONE_INITED);
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
Ext2MapIndirect(
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

        if (Index < Vcb->max_blocks_per_layer[Layer]) {

            ULONG   dwRet = 0, dwBlk = 0, dwHint = 0, dwArray = 0;

            Slot = (Layer==0) ? (Index):(Layer + EXT2_NDIR_BLOCKS - 1);
            dwBlk = Mcb->Inode.i_block[Slot];

            if (dwBlk == 0) {

                if (!bAlloc) {

                    *Number = 1;
                    goto errorout;

                } else {

                    if (Slot) {
                        dwHint = Mcb->Inode.i_block[Slot - 1];
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
                    Mcb->Inode.i_block[Slot] = dwBlk;

                    /* save the inode */
                    if (!Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode)) {
                        DbgBreak();
                        Status = STATUS_UNSUCCESSFUL;
                        goto errorout;
                    }
                }
            }

            if (Layer == 0) 
                dwArray = Vcb->max_blocks_per_layer[Layer] - Index;
            else
                dwArray = 1;

            /* querying block number of the index-th file block */
            Status = Ext2GetBlock(
                         IrpContext,
                         Vcb,
                         Mcb,
                         Base,
                         Layer,
                         Index,
                         dwArray,
                        (PULONG)&Mcb->Inode.i_block[Slot],
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

        Index -= Vcb->max_blocks_per_layer[Layer];
    }

errorout:

    return Status;
}

NTSTATUS
Ext2ExpandIndirect(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    ULONG             Start,
    ULONG             End,
    PLARGE_INTEGER    Size
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG    Layer = 0;
    ULONG    Extra = 0;
    ULONG    Hint = 0;
    ULONG    Slot = 0;
    ULONG    Base = 0;

    Extra = End - Start;

	/* exceeds the biggest file size (indirect) */
    if (End > Vcb->max_data_blocks) {
        return STATUS_INVALID_PARAMETER;
    }

    for (Layer = 0; Layer < EXT2_BLOCK_TYPES && Extra; Layer++) {

        if (Start >= Vcb->max_blocks_per_layer[Layer]) {

            Base  += Vcb->max_blocks_per_layer[Layer];
            Start -= Vcb->max_blocks_per_layer[Layer];

        } else {

            /* get the slot in i_block array */
            if (Layer == 0) {
                Base = Slot = Start;
            } else {
                Slot = Layer + EXT2_NDIR_BLOCKS - 1;
            }

            /* set block hint to avoid fragments */
            if (Hint == 0) {
                if (Mcb->Inode.i_block[Slot] != 0) {
                    Hint = Mcb->Inode.i_block[Slot];
                } else if (Slot > 1) {
                    Hint = Mcb->Inode.i_block[Slot-1];
                }
            }

            /* now expand this slot */
            Status = Ext2ExpandBlock(
                         IrpContext,
                         Vcb,
                         Mcb,
                         Base,
                         Layer,
                         Start,
                         (Layer == 0) ? (Vcb->max_blocks_per_layer[Layer] - Start) : 1,
                         (PULONG)&Mcb->Inode.i_block[Slot],
                         &Hint,
                         &Extra
                     );
            if (!NT_SUCCESS(Status)) {
                break;
            }

            Start = 0;
            if (Layer == 0) {
                Base = 0;
            }
            Base += Vcb->max_blocks_per_layer[Layer];
        }
    }

    Size->QuadPart = ((LONGLONG)(End - Extra)) << BLOCK_BITS;

    /* save inode whatever it succeeds to expand or not */
    Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

    return Status;
}

NTSTATUS
Ext2TruncateIndirectFast(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb
    )
{
    LONGLONG            Vba;
    LONGLONG            Lba;
    LONGLONG            Length;
    NTSTATUS            Status = STATUS_SUCCESS;
    int                 i;

    /* try to load all indirect blocks if mcb zone is not initialized */
    if (!IsZoneInited(Mcb)) {
        Status = Ext2InitializeZone(IrpContext, Vcb, Mcb);
        if (!NT_SUCCESS(Status)) {
            DbgBreak();
            ClearLongFlag(Mcb->Flags, MCB_ZONE_INITED);
            goto errorout;
        }
    }

    ASSERT (IsZoneInited(Mcb));

    /* delete all data blocks here */
    if (FsRtlNumberOfRunsInLargeMcb(&Mcb->Extents) != 0) {
        for (i = 0; FsRtlGetNextLargeMcbEntry(&Mcb->Extents, i, &Vba, &Lba, &Length); i++) {
            /* ignore the non-existing runs */
            if (-1 == Lba || Vba == 0 || Length <= 0)
                continue;
            /* now do data block free */
            Ext2FreeBlock(IrpContext, Vcb, (ULONG)(Lba - 1), (ULONG)Length);
        }
    }

    /* delete all meta blocks here */
    if (FsRtlNumberOfRunsInLargeMcb(&Mcb->MetaExts) != 0) {
        for (i = 0; FsRtlGetNextLargeMcbEntry(&Mcb->MetaExts, i, &Vba, &Lba, &Length); i++) {
            /* ignore the non-existing runs */
            if (-1 == Lba || Vba == 0 || Length <= 0)
                continue;
            /* now do meta block free */
            Ext2FreeBlock(IrpContext, Vcb, (ULONG)(Lba - 1), (ULONG)Length);
        }
    }

    /* clear data and meta extents */
    Ext2ClearAllExtents(&Mcb->Extents);
    Ext2ClearAllExtents(&Mcb->MetaExts);
    ClearFlag(Mcb->Flags, MCB_ZONE_INITED);

    /* clear inode blocks & sizes */
    Mcb->Inode.i_blocks = 0;
    Mcb->Inode.i_size = 0;
    memset(&Mcb->Inode.i_block[0], 0, sizeof(__u32) * 15);

    /* the caller will do inode save */

errorout:

    return Status;
}

NTSTATUS
Ext2TruncateIndirect(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG    Layer = 0;

    ULONG    Extra = 0;
    ULONG    Wanted = 0;
    ULONG    End;
    ULONG    Base;

    ULONG    SizeArray = 0;
    PULONG   BlockArray = NULL;

    /* translate file size to block */
    End = Base = Vcb->max_data_blocks;
    Wanted = (ULONG)((Size->QuadPart + BLOCK_SIZE - 1) >> BLOCK_BITS);

    /* do fast deletion here */
    if (Wanted == 0) {
        Status = Ext2TruncateIndirectFast(IrpContext, Vcb, Mcb);
        if (NT_SUCCESS(Status))
            goto errorout;
    }

    /* calculate blocks to be freed */
    Extra = End - Wanted;

    for (Layer = EXT2_BLOCK_TYPES; Layer > 0 && Extra; Layer--) {

        if (Vcb->max_blocks_per_layer[Layer - 1] == 0) {
            continue;
        }

        Base -= Vcb->max_blocks_per_layer[Layer - 1];

        if (Layer - 1 == 0) {
            BlockArray = (PULONG)&Mcb->Inode.i_block[0];
            SizeArray = End;
            ASSERT(End == EXT2_NDIR_BLOCKS && Base == 0);
        } else {
            BlockArray = (PULONG)&Mcb->Inode.i_block[EXT2_NDIR_BLOCKS - 1 + Layer - 1];
            SizeArray = 1;
        }

        Status = Ext2TruncateBlock(
                     IrpContext,
                     Vcb,
                     Mcb,
                     Base,
                     End - Base - 1,
                     Layer - 1,
                     SizeArray,
                     BlockArray,
                     &Extra
                 );
        if (!NT_SUCCESS(Status)) {
            break;
        }

        End = Base;
    }

errorout:

    if (!NT_SUCCESS(Status)) {
        Size->QuadPart += ((ULONGLONG)Extra << BLOCK_BITS);
    }

    /* save inode */
    if (Mcb->Inode.i_size > (loff_t)(Size->QuadPart))
        Mcb->Inode.i_size = (loff_t)(Size->QuadPart);
    Ext2SaveInode(IrpContext, Vcb, &Mcb->Inode);

    return Status;
}
