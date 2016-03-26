/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             rfsd.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

// TODO: turn off (turns off warning about returning without return value, so I could easily disable code sections)
#ifdef _MSC_VER
#pragma warning(disable : 4716)
#endif

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdLoadSuper)
#pragma alloc_text(PAGE, RfsdSaveSuper)

#pragma alloc_text(PAGE, RfsdLoadGroup)
#pragma alloc_text(PAGE, RfsdSaveGroup)

#pragma alloc_text(PAGE, RfsdLoadInode)
#pragma alloc_text(PAGE, RfsdSaveInode)

#pragma alloc_text(PAGE, RfsdLoadBlock)
#pragma alloc_text(PAGE, RfsdSaveBlock)

#pragma alloc_text(PAGE, RfsdSaveBuffer)

#pragma alloc_text(PAGE, RfsdGetBlock)
#pragma alloc_text(PAGE, RfsdBlockMap)

#pragma alloc_text(PAGE, RfsdBuildBDL)
#pragma alloc_text(PAGE, RfsdBuildBDL2)

#pragma alloc_text(PAGE, RfsdNewBlock)
#pragma alloc_text(PAGE, RfsdFreeBlock)

#pragma alloc_text(PAGE, RfsdExpandBlock)
#pragma alloc_text(PAGE, RfsdExpandInode)

#pragma alloc_text(PAGE, RfsdNewInode)
#pragma alloc_text(PAGE, RfsdFreeInode)

#pragma alloc_text(PAGE, RfsdAddEntry)
#pragma alloc_text(PAGE, RfsdRemoveEntry)

#pragma alloc_text(PAGE, RfsdTruncateBlock)
#pragma alloc_text(PAGE, RfsdTruncateInode)

#pragma alloc_text(PAGE, RfsdAddMcbEntry)
#pragma alloc_text(PAGE, RfsdRemoveMcbEntry)
#pragma alloc_text(PAGE, RfsdLookupMcbEntry)

#pragma alloc_text(PAGE, SuperblockContainsMagicKey)
#pragma alloc_text(PAGE, DetermineOnDiskKeyFormat)
#pragma alloc_text(PAGE, FillInMemoryKey)
#pragma alloc_text(PAGE, CompareShortKeys)
#pragma alloc_text(PAGE, CompareKeysWithoutOffset)
#pragma alloc_text(PAGE, CompareKeys)
#pragma alloc_text(PAGE, NavigateToLeafNode)
#pragma alloc_text(PAGE, _NavigateToLeafNode)
#pragma alloc_text(PAGE, RfsdParseFilesystemTree)
#endif

/* FUNCTIONS ***************************************************************/

PRFSD_SUPER_BLOCK
RfsdLoadSuper(IN PRFSD_VCB      Vcb,
              IN BOOLEAN        bVerify )
{
    NTSTATUS          Status;
    PRFSD_SUPER_BLOCK RfsdSb = NULL;

    PAGED_CODE();

    RfsdSb = (PRFSD_SUPER_BLOCK) ExAllocatePoolWithTag(PagedPool, 
                                                 SUPER_BLOCK_SIZE, RFSD_POOL_TAG);
    if (!RfsdSb) {
        return NULL;
    }

    Status = RfsdReadDisk(
                Vcb,
                (ULONGLONG) SUPER_BLOCK_OFFSET,
                SUPER_BLOCK_SIZE,
                (PVOID) RfsdSb,
                bVerify );

    if (!NT_SUCCESS(Status)) {

        RfsdPrint((DBG_ERROR, "RfsdReadDisk: Read Block Device error.\n"));

        ExFreePool(RfsdSb);
        return NULL;
    }

    return RfsdSb;
}

#if 0

BOOLEAN
RfsdSaveSuper(  IN PRFSD_IRP_CONTEXT    IrpContext,
                IN PRFSD_VCB            Vcb )
{
DbgBreak();
#if DISABLED
    LONGLONG    Offset;
    BOOLEAN     bRet;

    Offset = (LONGLONG) SUPER_BLOCK_OFFSET;

    bRet = RfsdSaveBuffer( IrpContext,
                           Vcb,
                           Offset,
                           SUPER_BLOCK_SIZE,
                           Vcb->SuperBlock );

    if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        RfsdStartFloppyFlushDpc(Vcb, NULL, NULL);
    }

    return bRet;
#endif
}

#if DISABLED
BOOLEAN
RfsdLoadGroup(IN PRFSD_VCB Vcb)
{
    ULONG       Size;
    PVOID       Buffer;
    LONGLONG    Lba;
    NTSTATUS    Status;

    PRFSD_SUPER_BLOCK sb;

    sb = Vcb->SuperBlock;

    Vcb->BlockSize  = RFSD_MIN_BLOCK << sb->s_log_block_size;
    Vcb->SectorBits = RfsdLog2(SECTOR_SIZE);
    ASSERT(BLOCK_BITS == RfsdLog2(BLOCK_SIZE));

    Vcb->NumOfGroups = (sb->s_blocks_count - sb->s_first_data_block +
        sb->s_blocks_per_group - 1) / sb->s_blocks_per_group;

    Size = sizeof(RFSD_GROUP_DESC) * Vcb->NumOfGroups;

    if (Vcb->BlockSize == RFSD_MIN_BLOCK) {
        Lba = (LONGLONG)2 * Vcb->BlockSize;
    }

    if (Vcb->BlockSize > RFSD_MIN_BLOCK) {
        Lba = (LONGLONG) (Vcb->BlockSize);
    }

    if (Lba == 0) {
        return FALSE;
    }

    Buffer = ExAllocatePoolWithTag(PagedPool, Size, RFSD_POOL_TAG);
    if (!Buffer) {
        RfsdPrint((DBG_ERROR, "RfsdLoadSuper: no enough memory.\n"));
        return FALSE;
    }

    RfsdPrint((DBG_USER, "RfsdLoadGroup: Lba=%I64xh Size=%xh\n", Lba, Size));

    Status = RfsdReadDisk(  Vcb,
                            Lba,
                            Size,
                            Buffer,
                            FALSE );

    if (!NT_SUCCESS(Status)) {
        ExFreePool(Buffer);
        Buffer = NULL;

        return FALSE;
    }

/*
    bPinned = CcPinRead(
                Vcb->StreamObj,
                Lba, 
                Size,
                PIN_WAIT,
                &(Vcb->GroupDescBcb),
                &(Buffer));

    if (!bPinned)
    {
        Vcb->GroupDesc = NULL;
        return FALSE;
    }
*/

    Vcb->GroupDesc = (PRFSD_GROUP_DESC) Buffer;

    return TRUE;
}

BOOLEAN
RfsdSaveGroup(  IN PRFSD_IRP_CONTEXT    IrpContext,
                IN PRFSD_VCB            Vcb,
                IN ULONG                Group )
{
    LONGLONG    Offset;
    BOOLEAN     bRet;

    if (Vcb->BlockSize == RFSD_MIN_BLOCK) {

        Offset = (LONGLONG) (2 * Vcb->BlockSize);

    } else {

        Offset = (LONGLONG) (Vcb->BlockSize);
    }

    Offset += ((LONGLONG) sizeof(struct rfsd_group_desc) * Group);

    bRet = RfsdSaveBuffer(
                    IrpContext,
                    Vcb,
                    Offset,
                    sizeof(struct rfsd_group_desc),
                    &(Vcb->GroupDesc[Group]) );

    if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        RfsdStartFloppyFlushDpc(Vcb, NULL, NULL);
    }

    return bRet;
}     
#endif

#endif // 0

/** Reads an inode structure off disk into the Inode object (which has been allocated, but not filled with any data) */
BOOLEAN
RfsdLoadInode (IN PRFSD_VCB Vcb,
			   IN PRFSD_KEY_IN_MEMORY pKey,
			   IN OUT PRFSD_INODE Inode)
{
	NTSTATUS Status;
	
	PRFSD_ITEM_HEAD pItemHeader = NULL;
	PUCHAR	pItemBuffer  = NULL;
	PUCHAR	pBlockBuffer = NULL;
	
	// Crate the target key for the stat item (stat items always have an offset of 0)
	RFSD_KEY_IN_MEMORY		TargetKey;

    PAGED_CODE();

	TargetKey = *pKey;
	TargetKey.k_offset		= 0x0;
	TargetKey.k_type		= RFSD_KEY_TYPE_v2_STAT_DATA;	

	RfsdPrint((DBG_FUNC, /*__FUNCTION__*/ "on %i, %i\n", TargetKey.k_dir_id, TargetKey.k_objectid));

	//Load the stat data
	Status = RfsdLoadItem(Vcb, &TargetKey,
		&(pItemHeader), &(pItemBuffer), &(pBlockBuffer), NULL,  //<
		&CompareKeys
	);
	if (!NT_SUCCESS(Status))
		{ if (pBlockBuffer) {ExFreePool(pBlockBuffer);} return FALSE; }
	
	// Copy the item into the inode / stat data structure
	RtlCopyMemory(Inode, pItemBuffer, sizeof(RFSD_INODE));	

	// Cleanup
	if (pBlockBuffer)
		ExFreePool(pBlockBuffer);

	return TRUE;
}

#if 0

BOOLEAN
RfsdSaveInode ( IN PRFSD_IRP_CONTEXT IrpContext,
                IN PRFSD_VCB Vcb,
                IN ULONG Inode,
                IN PRFSD_INODE RfsdInode)
{
DbgBreak();
#if DISABLED
    LONGLONG        Offset = 0;
    LARGE_INTEGER   CurrentTime;
    BOOLEAN         bRet;

    KeQuerySystemTime(&CurrentTime);
    RfsdInode->i_mtime = RfsdInode->i_atime = 
                      (ULONG)(RfsdInodeTime(CurrentTime));

    RfsdPrint((DBG_INFO, "RfsdSaveInode: Saving Inode %xh: Mode=%xh Size=%xh\n",
                         Inode, RfsdInode->i_mode, RfsdInode->i_size));

    if (!RfsdGetInodeLba(Vcb, Inode, &Offset))  {
        RfsdPrint((DBG_ERROR, "RfsdSaveInode: error get inode(%xh)'s addr.\n", Inode));
        return FALSE;
    }

    bRet = RfsdSaveBuffer(IrpContext, Vcb, Offset, sizeof(RFSD_INODE), RfsdInode);

    if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        RfsdStartFloppyFlushDpc(Vcb, NULL, NULL);
    }

    return bRet;
#endif
}

#endif // 0

/** Just reads a block into a buffer */
BOOLEAN
RfsdLoadBlock (
			IN PRFSD_VCB		Vcb,
            IN ULONG			dwBlk,		// A disk block ptr (a disk block number)
            IN OUT PVOID		Buffer )	// A buffer, which must be allocated to contain at least Vcb->BlockSize
{
    IO_STATUS_BLOCK     IoStatus;
	LONGLONG            Offset;

    PAGED_CODE();

    Offset = (LONGLONG) dwBlk;
    Offset = Offset * Vcb->BlockSize;

    if (!RfsdCopyRead(
            Vcb->StreamObj,
            (PLARGE_INTEGER)&Offset,
            Vcb->BlockSize,
            PIN_WAIT,
            Buffer,
			&IoStatus )) {
		return FALSE;
	}

    if (!NT_SUCCESS(IoStatus.Status)) {
        return FALSE;
    }

    return TRUE;
}

#if 0

BOOLEAN
RfsdSaveBlock ( IN PRFSD_IRP_CONTEXT    IrpContext,
                IN PRFSD_VCB            Vcb,
                IN ULONG                dwBlk,
                IN PVOID                Buf )
{
DbgBreak();
#if DISABLED
    LONGLONG Offset;
    BOOLEAN  bRet;

    Offset = (LONGLONG) dwBlk;
    Offset = Offset * Vcb->BlockSize;

    bRet = RfsdSaveBuffer(IrpContext, Vcb, Offset, Vcb->BlockSize, Buf);

    if (IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK)) {
        RfsdStartFloppyFlushDpc(Vcb, NULL, NULL);
    }

    return bRet;
#endif
}

BOOLEAN
RfsdSaveBuffer( IN PRFSD_IRP_CONTEXT    IrpContext,
                IN PRFSD_VCB            Vcb,
                IN LONGLONG             Offset,
                IN ULONG                Size,
                IN PVOID                Buf )
{
    PBCB        Bcb;
    PVOID       Buffer;
    BOOLEAN     bRet;

    PAGED_CODE();

    if( !CcPinRead( Vcb->StreamObj,
                    (PLARGE_INTEGER) (&Offset),
                    Size,
                    PIN_WAIT,
                    &Bcb,
                    &Buffer )) {

        RfsdPrint((DBG_ERROR, "RfsdSaveBuffer: PinReading error ...\n"));
        return FALSE;
    }

    _SEH2_TRY {

        RtlCopyMemory(Buffer, Buf, Size);
        CcSetDirtyPinnedData(Bcb, NULL );
        RfsdRepinBcb(IrpContext, Bcb);

        SetFlag(Vcb->StreamObj->Flags, FO_FILE_MODIFIED);

        RfsdAddMcbEntry(Vcb, Offset, (LONGLONG)Size);

        bRet = TRUE;

    } _SEH2_FINALLY {

        if (AbnormalTermination()) {

            CcUnpinData(Bcb);
            bRet = FALSE;
        }
    } _SEH2_END;

    CcUnpinData(Bcb);

    return bRet;
}

NTSTATUS
RfsdGetBlock(
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                dwContent,			// A ptr to a disk block (disk block number)
    IN ULONG                Index,
    IN ULONG                Layer,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock
    )
{
DbgBreak();
#if DISABLED
    NTSTATUS    Status = STATUS_SUCCESS;
    ULONG       *pData = NULL;
    ULONG       i = 0, j = 0, temp = 1;
    ULONG       dwRet = 0;
    ULONG       dwBlk = 0;

    if (Layer == 0) {

        dwRet = dwContent;

    } else if (Layer <= 3) {

        /* allocate memory for pData to contain the block */
        pData = (ULONG *) ExAllocatePoolWithTag(NonPagedPool, Vcb->BlockSize, RFSD_POOL_TAG);
        if (!pData) {
            RfsdPrint((DBG_ERROR, "RfsdGetBlock: no enough memory.\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        /* load the block dwContext into pData buffer */
        if (!RfsdLoadBlock(Vcb, dwContent, pData)) {
            ExFreePool(pData);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        temp = 1 << ((BLOCK_BITS - 2) * (Layer - 1));
        
        i = Index / temp;
        j = Index % temp;

        dwBlk = pData[i];

#if DISABLED  // WRITE MODE ONLY
        if (dwBlk ==0 ) {

            if (bAlloc) {

                DbgBreak();

                /* we need allocate new block: dwBlk */

                Status = RfsdNewBlock(
                            IrpContext,
                            Vcb,
                            0,
                            dwContent,
                            &dwBlk
                            );

                if (!NT_SUCCESS(Status)) {
                    ExFreePool(pData);
                    goto errorout;
                }

                /* we need save pData now */
                pData[i] = dwBlk;

                /* save the block first, before next call */
                if (!RfsdSaveBlock( IrpContext, Vcb,
                                    dwContent, pData)) {

                    /* error occurs when saving the block */
                    Status = STATUS_INSUFFICIENT_RESOURCES;

                    /* we need free newly allocated block */
                    RfsdFreeBlock(IrpContext, Vcb, dwBlk);

                    /* free the memory of pData */
                    ExFreePool(pData);
                
                    goto errorout;
                }

            } else {

                /* free the memory of pData */
                ExFreePool(pData);
                
                goto errorout;
            }
        }
#endif

		/* free the memory of pData */
        ExFreePool(pData);

        /* transfer to next recursion call */
        Status = RfsdGetBlock(
                    IrpContext,
                    Vcb,
                    dwBlk,
                    j,
                    Layer - 1,
                    bAlloc,
                    &dwRet
                    );

        if (!NT_SUCCESS(Status)) {
            dwRet = 0;
        }
    }

errorout:

    *pBlock = dwRet;

    return Status;
#endif
}

/** I think this means it maps the blocks into the cache.
Basically, it goes through and calls GetBlock for each block.
*/
NTSTATUS
RfsdBlockMap(
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONG                Index,				// Ordinal index of this block in the BDL
    IN BOOLEAN              bAlloc,				// FALSE
    OUT PULONG              pBlock				// <
    )
{
	DbgBreak();
#if DISABLED
    ULONG       i;

    ULONG       dwSizes[RFSD_BLOCK_TYPES];
    NTSTATUS    Status = STATUS_SUCCESS;;

    *pBlock = 0;

    for (i = 0; i < RFSD_BLOCK_TYPES; i++) {
        dwSizes[i] = Vcb->dwData[i];
    }

    for (i = 0; i < RFSD_BLOCK_TYPES; i++) {

        if (Index < dwSizes[i]) {

            ULONG   dwRet = 0;
            ULONG   dwBlk = 0;

			// dwBlk will get the ptr to a block.
            dwBlk = Inode->i_block[i==0 ? (Index):(i + RFSD_NDIR_BLOCKS - 1)];

#if DISABLED // WRITE MODE ONLY
            if (dwBlk == 0) {

                if (!bAlloc) {

                    goto errorout;

                } else {

                    DbgBreak();

                    /* we need allocate new block: dwBlk */
                    Status = RfsdNewBlock(
                                IrpContext,
                                Vcb,
                                (InodeNo - 1) / BLOCKS_PER_GROUP,
                                0,
                                &dwBlk
                                );

                    if (!NT_SUCCESS(Status)) {
                        goto errorout;
                    }

                    /* save the it into inode*/
                    Inode->i_block[i==0 ? (Index):(i + RFSD_NDIR_BLOCKS - 1)] = dwBlk;

                    /* save the inode */
                    if (!RfsdSaveInode( IrpContext,
                                        Vcb,
                                        InodeNo,
                                        Inode)) {

                        Status = STATUS_UNSUCCESSFUL;

                        RfsdFreeBlock(IrpContext, Vcb, dwBlk);

                        goto errorout;
                    }
                }
            }
#endif
            Status = RfsdGetBlock(
                        IrpContext,
                        Vcb,
                        dwBlk,
                        Index,
                        i,
                        bAlloc,
                        &dwRet			//< 
                        );

            RfsdPrint((DBG_INFO, "RfsdBlockMap: i=%xh index=%xh dwBlk=%xh (%xh)\n",
                                 i, Index, dwRet, dwBlk));

            if (NT_SUCCESS(Status)) {
                *pBlock = dwRet;
            }

            break;
        }

        Index -= dwSizes[i];
    }

errorout:

    return Status;
#endif
}

#endif // 0

// NOTE:  ReiserFS starts it byte offsets at 1, as opposed to 0 (which is used for the buffer -- and therefore, the BDL is also 0-based).
NTSTATUS
RfsdBuildBDL2(	
	IN  PRFSD_VCB				Vcb,
	IN  PRFSD_KEY_IN_MEMORY		pKey,
	IN	PRFSD_INODE				pInode,
	OUT	PULONG					out_Count,
	OUT PRFSD_BDL*				out_ppBdl  )
{
	NTSTATUS			Status				= STATUS_SUCCESS;
	BOOLEAN				done				= FALSE;

	RFSD_KEY_IN_MEMORY	CurrentTargetKey	= *pKey;
	ULONGLONG			CurrentOffset		= 0;
	
	PRFSD_ITEM_HEAD		pItemHeader			= NULL;			// The temporary storage for retrieving items from disk
	PUCHAR				pItemBuffer			= NULL;
	PUCHAR				pBlockBuffer		= NULL;

	ULONG				idxCurrentBD		= 0;
	PRFSD_BDL			pBdl				= NULL;			// The block descriptor list, which will be allocated, filled, and assigned to out_Bdl

    PAGED_CODE();

	// Allocate the BDL for the maximum number of block descriptors that will be needed (including the tail)
	// FUTURE: sd_blocks DEFINITELY is not the number of blocks consumed by a file.  (at least not the number of 4096-byte blocks)
	// However, I'm unsure of how to calculate the number of blocks.  Perhaps I should consider using a linked list instead?
	KdPrint(("## Allocating %i BD's\n", pInode->i_size / Vcb->BlockSize + 3));
	pBdl = ExAllocatePoolWithTag(NonPagedPool, sizeof(RFSD_BDL) * (SIZE_T) (pInode->i_size / Vcb->BlockSize + 3), RFSD_POOL_TAG);
	if (!pBdl) { Status = STATUS_INSUFFICIENT_RESOURCES;	goto errorout; }
	//RtlZeroMemory(pBdl, sizeof(RFSD_BDL) * (pInode->sd_blocks + 1));
	RtlZeroMemory(pBdl, sizeof(RFSD_BDL) * (SIZE_T) (pInode->i_size / Vcb->BlockSize + 3));
		  

	// Build descriptors for all of the indirect items associated with the file
	while (!done)
	{
		// Search for an indirect item, corresponding to CurrentOffset...

		// Create the key to search for (note that the key always start with offset 1, even though it is for byte 0)
		CurrentTargetKey.k_offset	= CurrentOffset + 1;
		CurrentTargetKey.k_type		= RFSD_KEY_TYPE_v2_INDIRECT;

		// Perform the search
		Status = RfsdLoadItem(
			Vcb, &CurrentTargetKey, 
			&(pItemHeader), &(pItemBuffer), &(pBlockBuffer), NULL,
			&CompareKeys
			);
		
		// If there was no such indirect item...
		if (Status == STATUS_NO_SUCH_MEMBER)	{ Status = STATUS_SUCCESS; break; }
		if (!NT_SUCCESS(Status))				{ goto errorout; }

		// Otherwise, create a block descriptor for each pointer in the indirect item
		{
		  ULONG countBlockRefs = pItemHeader->ih_item_len / sizeof(ULONG);
		  ULONG idxBlockRef;

		  for (idxBlockRef = 0; idxBlockRef < countBlockRefs; idxBlockRef++)
		  {
			  PULONG BlockRef = (PULONG) ((PUCHAR) pItemBuffer + sizeof(ULONG) * idxBlockRef);
              
			  // Build a block descriptor for this block reference
			  pBdl[idxCurrentBD].Lba		= (LONGLONG) *BlockRef * (LONGLONG) Vcb->BlockSize;
			  pBdl[idxCurrentBD].Length		= Vcb->BlockSize;
			  pBdl[idxCurrentBD].Offset		= CurrentOffset;

			  // If this is the last reference in the indirect item, subtract the free space from the end
			  // TODO: this may not work, because the ih_free_space_reserved seems to be wrong / not there!
			  if (idxBlockRef == (countBlockRefs - 1))
				  pBdl[idxCurrentBD].Length -= pItemHeader->u.ih_free_space_reserved;

			  // Advance to the next block reference
			  CurrentOffset += Vcb->BlockSize;
			  idxCurrentBD++;
		  }
		  
		  if (countBlockRefs <= 0)		{ done = TRUE; }
		}
		
		if (pBlockBuffer)				{ ExFreePool(pBlockBuffer);  pBlockBuffer = NULL; }
	}
	
	// Cleanup the last remaining block buffer, from the indirect items
	if (pBlockBuffer) { ExFreePool(pBlockBuffer);  pBlockBuffer = NULL; }

	// Search for the tail of the file (its optional direct item), corresponding to CurrentOffset...
	{
	  ULONG BlockNumber = 0;

	  // Create the key to search for
	  CurrentTargetKey.k_offset	= CurrentOffset + 1;
	  CurrentTargetKey.k_type	= RFSD_KEY_TYPE_v2_DIRECT;

	  // Perform the search
	  Status = RfsdLoadItem(
		  Vcb, &CurrentTargetKey,
		  &(pItemHeader), &(pItemBuffer), &(pBlockBuffer), &(BlockNumber),
		  &CompareKeys
		  );
	  
	  if (Status == STATUS_SUCCESS) 
	  {
		  // If there was a tail, then build a block descriptor for it
		  pBdl[idxCurrentBD].Lba		= (LONGLONG) BlockNumber * (LONGLONG) Vcb->BlockSize + pItemHeader->ih_item_location;
		  pBdl[idxCurrentBD].Length		= pItemHeader->ih_item_len;
		  pBdl[idxCurrentBD].Offset		= CurrentOffset;

		  // Advance to the next block reference
		  CurrentOffset += pItemHeader->ih_item_len;
		  idxCurrentBD++;
	  }  
	  else
	  {
		  if (Status == STATUS_NO_SUCH_MEMBER) { Status = STATUS_SUCCESS; goto errorout; }		// If there wasn't a tail, it's fine
		  else								   { goto errorout; }				// But if there was some other problem, let's report it.
	  }		
	}

	if (pBlockBuffer) { ExFreePool(pBlockBuffer);  pBlockBuffer = NULL; }

	// Search for the second part of the tail of the file (its optional second direct item), corresponding to CurrentOffset...
	{
	  ULONG BlockNumber = 0;

	  // Create the key to search for
	  CurrentTargetKey.k_offset	= CurrentOffset + 1;
	  CurrentTargetKey.k_type	= RFSD_KEY_TYPE_v2_DIRECT;

	  // Perform the search
	  Status = RfsdLoadItem(
		  Vcb, &CurrentTargetKey,
		  &(pItemHeader), &(pItemBuffer), &(pBlockBuffer), &(BlockNumber),
		  &CompareKeys
		  );
	  
	  if (Status == STATUS_SUCCESS) 
	  {
		  // If there was a second part of the tail, then build a block descriptor for it
		  pBdl[idxCurrentBD].Lba		= (LONGLONG) BlockNumber * (LONGLONG) Vcb->BlockSize + pItemHeader->ih_item_location;
		  pBdl[idxCurrentBD].Length		= pItemHeader->ih_item_len;
		  pBdl[idxCurrentBD].Offset		= CurrentOffset;

		  idxCurrentBD++;
	  }  
	  else
	  {
		  if (Status == STATUS_NO_SUCH_MEMBER) { Status = STATUS_SUCCESS; }		// If there wasn't a second part of the tail, it's fine
		  else								   { goto errorout; }				// But if there was some other problem, let's report it.
	  }		
	}

errorout:	
	if (pBlockBuffer) { ExFreePool(pBlockBuffer);  pBlockBuffer = NULL; }

	*out_ppBdl	= pBdl;
	*out_Count	= idxCurrentBD;
	return Status;
}

#if 0

NTSTATUS
RfsdNewBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    ULONG     GroupHint,
    ULONG     BlockHint,  
    PULONG    dwRet )
{
DbgBreak();
#if DISABLED
    RTL_BITMAP      BlockBitmap;
    LARGE_INTEGER   Offset;
    ULONG           Length;

    PBCB            BitmapBcb;
    PVOID           BitmapCache;

    ULONG           Group = 0, dwBlk, dwHint = 0;

    *dwRet = 0;
    dwBlk = 0XFFFFFFFF;

    if (GroupHint > Vcb->NumOfGroups)
        GroupHint = Vcb->NumOfGroups - 1;

    if (BlockHint != 0) {
        GroupHint = (BlockHint - RFSD_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;
        dwHint = (BlockHint - RFSD_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;
    }
  
ScanBitmap:
  
    // Perform Prefered Group
    if (Vcb->GroupDesc[GroupHint].bg_free_blocks_count) {

        Offset.QuadPart = (LONGLONG) Vcb->BlockSize;
        Offset.QuadPart = Offset.QuadPart * 
                          Vcb->GroupDesc[GroupHint].bg_block_bitmap;

        if (GroupHint == Vcb->NumOfGroups - 1) {

            Length = TOTAL_BLOCKS % BLOCKS_PER_GROUP;

            /* s_blocks_count is integer multiple of s_blocks_per_group */
            if (Length == 0) {
                Length = BLOCKS_PER_GROUP;
            }
        } else {
            Length = BLOCKS_PER_GROUP;
        }

        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &BitmapBcb,
                        &BitmapCache ) ) {

            RfsdPrint((DBG_ERROR, "RfsdNewBlock: PinReading error ...\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlInitializeBitMap( &BlockBitmap,
                             BitmapCache,
                             Length );

        Group = GroupHint;

        if (RtlCheckBit(&BlockBitmap, dwHint) == 0) {
            dwBlk = dwHint;
        } else  {
            dwBlk = RtlFindClearBits(&BlockBitmap, 1, dwHint);
        }

        // We could not get new block in the prefered group.
        if (dwBlk == 0xFFFFFFFF) {

            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;

            RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
        }
    }

    if (dwBlk == 0xFFFFFFFF) {

        for(Group = 0; Group < Vcb->NumOfGroups; Group++)
        if (Vcb->GroupDesc[Group].bg_free_blocks_count) {
            if (Group == GroupHint)
                continue;

            Offset.QuadPart = (LONGLONG) Vcb->BlockSize;
            Offset.QuadPart = Offset.QuadPart * Vcb->GroupDesc[Group].bg_block_bitmap;

            if (Vcb->NumOfGroups == 1) {
                Length = TOTAL_BLOCKS;
            } else {
                if (Group == Vcb->NumOfGroups - 1) {

                    Length = TOTAL_BLOCKS % BLOCKS_PER_GROUP;

                    /* s_blocks_count is integer multiple of s_blocks_per_group */
                    if (Length == 0) {
                        Length = BLOCKS_PER_GROUP;
                    }
                } else {
                    Length = BLOCKS_PER_GROUP;
                }
            }

            if (!CcPinRead( Vcb->StreamObj,
                            &Offset,
                            Vcb->BlockSize,
                            PIN_WAIT,
                            &BitmapBcb,
                            &BitmapCache ) ) {
                RfsdPrint((DBG_ERROR, "RfsdNewBlock: PinReading error ...\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlInitializeBitMap( &BlockBitmap,
                                 BitmapCache,
                                 Length );

            dwBlk = RtlFindClearBits(&BlockBitmap, 1, 0);

            if (dwBlk != 0xFFFFFFFF) {
                break;

            } else {

                CcUnpinData(BitmapBcb);
                BitmapBcb = NULL;
                BitmapCache = NULL;

                RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
            }
        }
    }
        
    if (dwBlk < Length) {

        RtlSetBits(&BlockBitmap, dwBlk, 1);

        CcSetDirtyPinnedData(BitmapBcb, NULL );

        RfsdRepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        RfsdAddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);

        *dwRet = dwBlk + RFSD_FIRST_DATA_BLOCK + Group * BLOCKS_PER_GROUP;

        //Updating Group Desc / Superblock
        Vcb->GroupDesc[Group].bg_free_blocks_count--;
        RfsdSaveGroup(IrpContext, Vcb, Group);

        Vcb->SuperBlock->s_free_blocks_count--;
        RfsdSaveSuper(IrpContext, Vcb);

        {
            ULONG i=0;
            for (i=0; i < Vcb->NumOfGroups; i++)
            {
                if ((Vcb->GroupDesc[i].bg_block_bitmap == *dwRet) ||
                    (Vcb->GroupDesc[i].bg_inode_bitmap == *dwRet) ||
                    (Vcb->GroupDesc[i].bg_inode_table == *dwRet) ) {
                    DbgBreak();
                    GroupHint = Group;
                    goto ScanBitmap;
                }
            }
        }

        return STATUS_SUCCESS;
    }

    return STATUS_DISK_FULL;
#endif
}

NTSTATUS
RfsdFreeBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    ULONG     Block )
{
DbgBreak();
#if DISABLED

	RTL_BITMAP      BlockBitmap;
    LARGE_INTEGER   Offset;
    ULONG           Length;

    PBCB            BitmapBcb;
    PVOID           BitmapCache;

    ULONG           Group, dwBlk;
    BOOLEAN         bModified = FALSE;

    if ( Block < RFSD_FIRST_DATA_BLOCK || 
         (Block / BLOCKS_PER_GROUP) >= Vcb->NumOfGroups) {

        DbgBreak();
        return STATUS_INVALID_PARAMETER;
    }

    RfsdPrint((DBG_INFO, "RfsdFreeBlock: Block %xh to be freed.\n", Block));

    Group = (Block - RFSD_FIRST_DATA_BLOCK) / BLOCKS_PER_GROUP;

    dwBlk = (Block - RFSD_FIRST_DATA_BLOCK) % BLOCKS_PER_GROUP;
    
    {
        Offset.QuadPart = (LONGLONG) Vcb->BlockSize;
        Offset.QuadPart = Offset.QuadPart * Vcb->GroupDesc[Group].bg_block_bitmap;

        if (Group == Vcb->NumOfGroups - 1) {

            Length = TOTAL_BLOCKS % BLOCKS_PER_GROUP;

            /* s_blocks_count is integer multiple of s_blocks_per_group */
            if (Length == 0) {
                Length = BLOCKS_PER_GROUP;
            }

        } else {
            Length = BLOCKS_PER_GROUP;
        }

        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &BitmapBcb,
                        &BitmapCache ) ) {

            RfsdPrint((DBG_ERROR, "RfsdDeleteBlock: PinReading error ...\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlInitializeBitMap( &BlockBitmap,
                             BitmapCache,
                             Length );

        if (RtlCheckBit(&BlockBitmap, dwBlk) == 0) {
            
        } else {
            RtlClearBits(&BlockBitmap, dwBlk, 1);
            bModified = TRUE;
        }

        if (!bModified) {

            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;

            RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
        }
    }
        
    if (bModified) {

        CcSetDirtyPinnedData(BitmapBcb, NULL );

        RfsdRepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        RfsdAddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);

        //Updating Group Desc / Superblock
        Vcb->GroupDesc[Group].bg_free_blocks_count++;
        RfsdSaveGroup(IrpContext, Vcb, Group);

        Vcb->SuperBlock->s_free_blocks_count++;
        RfsdSaveSuper(IrpContext, Vcb);

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
#endif
}

NTSTATUS
RfsdExpandBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB   Vcb,
    PRFSD_FCB   Fcb,
    ULONG   dwContent,
    ULONG   Index,
    ULONG   layer,
    BOOLEAN bNew,
    ULONG   *dwRet )
{
DbgBreak();
#if DISABLED

    ULONG       *pData = NULL;
    ULONG       i = 0, j = 0, temp = 1;
    ULONG       dwNewBlk = 0, dwBlk = 0;
    BOOLEAN     bDirty = FALSE;
    NTSTATUS    Status = STATUS_SUCCESS;

    PRFSD_INODE       Inode  = Fcb->Inode;
    PRFSD_SUPER_BLOCK RfsdSb = Vcb->SuperBlock;

    pData = (ULONG *) ExAllocatePoolWithTag(PagedPool, Vcb->BlockSize, RFSD_POOL_TAG);

    if (!pData) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pData, Vcb->BlockSize);

    if (bNew) {

        if (layer == 0) {

            if (IsDirectory(Fcb)) {

                PRFSD_DIR_ENTRY2 pEntry;

                pEntry = (PRFSD_DIR_ENTRY2) pData;
                pEntry->rec_len = (USHORT)(Vcb->BlockSize);

                if (!RfsdSaveBlock(IrpContext, Vcb, dwContent, (PVOID)pData)) {

                    Status = STATUS_UNSUCCESSFUL;
                    goto errorout;
                }

            } else {

                LARGE_INTEGER   Offset;
            
                Offset.QuadPart  = (LONGLONG) dwContent;
                Offset.QuadPart = Offset.QuadPart * Vcb->BlockSize;

                RfsdRemoveMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);
            }
        } else {

            if (!RfsdSaveBlock(IrpContext, Vcb, dwContent, (PVOID)pData)) {

                Status = STATUS_UNSUCCESSFUL;
                goto errorout;
            }
        }
    }

    if (layer == 0) {

        dwNewBlk = dwContent;

    } else if (layer <= 3) {

        if (!bNew) {

            if (!RfsdLoadBlock(Vcb, dwContent, (void *)pData)) {

                Status = STATUS_UNSUCCESSFUL;
                goto errorout;
            }
        }

        temp = 1 << ((BLOCK_BITS - 2) * (layer - 1));

        i = Index / temp;
        j = Index % temp;

        dwBlk = pData[i];

        if (dwBlk == 0) {

            Status = RfsdNewBlock(
                        IrpContext,
                        Vcb, 0,
                        dwContent,
                       &dwBlk);

            if (!NT_SUCCESS(Status)) {

                RfsdPrint((DBG_ERROR, "RfsdExpandBlock: get new block error.\n"));
                goto errorout;
            }

            Inode->i_blocks += (Vcb->BlockSize / SECTOR_SIZE);

            pData[i] = dwBlk;
            bDirty = TRUE;
        }

        Status = RfsdExpandBlock(
                    IrpContext,
                    Vcb, Fcb,
                    dwBlk, j,
                    layer - 1,
                    bDirty,
                    &dwNewBlk );

        if (!NT_SUCCESS(Status))
        {
            RfsdPrint((DBG_ERROR, "RfsdExpandBlockk: ... error recuise...\n"));
            goto errorout;
        }
        
        if (bDirty)
        {
            RfsdSaveBlock( IrpContext,
                            Vcb, dwContent,
                            (void *)pData );
        }
    }

errorout:

    if (pData)
        ExFreePool(pData);

    if (NT_SUCCESS(Status) && dwRet)
        *dwRet = dwNewBlk;

    return Status;
#endif
}

NTSTATUS
RfsdExpandInode(
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN PRFSD_FCB            Fcb,
    OUT PULONG              dwRet
    )
{
DbgBreak();
#if DISABLED

    ULONG dwSizes[RFSD_BLOCK_TYPES];
    ULONG Index = 0;
    ULONG dwTotal = 0;
    ULONG dwBlk = 0, dwNewBlk = 0;
    ULONG    i;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  bNewBlock = FALSE;

    PRFSD_INODE Inode = Fcb->Inode;

    Index = (ULONG)(Fcb->Header.AllocationSize.QuadPart >> BLOCK_BITS);

    for (i = 0; i < RFSD_BLOCK_TYPES; i++) {
        dwSizes[i] = Vcb->dwData[i];
        dwTotal += dwSizes[i];
    }

    if (Index >= dwTotal) {

        RfsdPrint((DBG_ERROR, "RfsdExpandInode: beyond the maxinum size of an inode.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    for (i = 0; i < RFSD_BLOCK_TYPES; i++) {

        if (Index < dwSizes[i]) {

            dwBlk = Inode->i_block[i==0 ? (Index):(i + RFSD_NDIR_BLOCKS - 1)];

            if (dwBlk == 0) {

                Status = RfsdNewBlock(
                            IrpContext,
                            Vcb,
                            Fcb->BlkHint ? 0 : ((Fcb->RfsdMcb->Inode - 1) / INODES_PER_GROUP),
                            Fcb->BlkHint,
                            &dwBlk );

                if (!NT_SUCCESS(Status) ) {
                    RfsdPrint((DBG_ERROR, "RfsdExpandInode: get new block error.\n"));
                    break;
                }

                Inode->i_block[i==0 ? (Index):(i + RFSD_NDIR_BLOCKS - 1)] = dwBlk;

                Inode->i_blocks += (Vcb->BlockSize / SECTOR_SIZE);

                bNewBlock = TRUE;
            }

            Status = RfsdExpandBlock (
                            IrpContext,
                            Vcb, Fcb,
                            dwBlk, Index,
                            i, bNewBlock,
                            &dwNewBlk  ); 

            if (NT_SUCCESS(Status)) {
                Fcb->Header.AllocationSize.QuadPart += Vcb->BlockSize;
            }

            break;
        }

        Index -= dwSizes[i];
    }

    RfsdSaveInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, Inode);

    if (NT_SUCCESS(Status)) {
        if (dwNewBlk) {
            if (dwRet) {
                Fcb->BlkHint = dwNewBlk+1;
                *dwRet = dwNewBlk;

                RfsdPrint((DBG_INFO, "RfsdExpandInode: %S (%xh) i=%2.2xh Index=%8.8xh New Block=%8.8xh\n",
                           Fcb->RfsdMcb->ShortName.Buffer, Fcb->RfsdMcb->Inode, i, Index, dwNewBlk));
            }
        } else {
            DbgBreak();
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    return Status;
#endif
}

NTSTATUS
RfsdNewInode(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            ULONG   GroupHint,
            ULONG   Type,
            PULONG  Inode )
{
DbgBreak();
#if DISABLED

    RTL_BITMAP      InodeBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    ULONG           Group, i, j;
    ULONG           Average, Length;
    LARGE_INTEGER   Offset;
    
    ULONG           dwInode;

    *Inode = dwInode = 0XFFFFFFFF;

repeat:

    Group = i = 0;
    
    if (Type == RFSD_FT_DIR) {

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

            for (j = 0; j < Vcb->NumOfGroups; j++) {

                if (Vcb->GroupDesc[j].bg_free_inodes_count >= Average) {
                    if (!Group || (Vcb->GroupDesc[j].bg_free_blocks_count >
                                   Vcb->GroupDesc[Group].bg_free_blocks_count ))
                        Group = j + 1;
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
             * Use a quadratic hash to find a group with a
             * free inode
             */

            for (j = 1; j < Vcb->NumOfGroups; j <<= 1) {

                i += j;
                if (i > Vcb->NumOfGroups) 
                    i -= Vcb->NumOfGroups;

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
            i = GroupHint + 1;
            for (j = 2; j < Vcb->NumOfGroups; j++) {
                if (++i >= Vcb->NumOfGroups) i = 0;

                if (Vcb->GroupDesc[i].bg_free_inodes_count) {
                    Group = i + 1;
                    break;
                }
            }
        }
    }

    // Could not find a proper group.
    if (!Group) {

        return STATUS_DISK_FULL;

    } else {

        Group--;

        Offset.QuadPart = (LONGLONG) Vcb->BlockSize;
        Offset.QuadPart = Offset.QuadPart * Vcb->GroupDesc[Group].bg_inode_bitmap;

        if (Vcb->NumOfGroups == 1) {
            Length = INODES_COUNT;
        } else {
            if (Group == Vcb->NumOfGroups - 1) {
                Length = INODES_COUNT % INODES_PER_GROUP;
                if (!Length) {
                    /* INODES_COUNT is integer multiple of INODES_PER_GROUP */
                    Length = INODES_PER_GROUP;
                }
            } else  {
                Length = INODES_PER_GROUP;
            }
        }
        
        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &BitmapBcb,
                        &BitmapCache ) ) {

            RfsdPrint((DBG_ERROR, "RfsdNewInode: PinReading error ...\n"));

            return STATUS_UNSUCCESSFUL;
        }

        RtlInitializeBitMap( &InodeBitmap,
                             BitmapCache,
                             Length );

        dwInode = RtlFindClearBits(&InodeBitmap, 1, 0);

        if (dwInode == 0xFFFFFFFF) {
            CcUnpinData(BitmapBcb);
            BitmapBcb = NULL;
            BitmapCache = NULL;

            RtlZeroMemory(&InodeBitmap, sizeof(RTL_BITMAP));
        }
    }

    if (dwInode == 0xFFFFFFFF || dwInode >= Length) {

        if (Vcb->GroupDesc[Group].bg_free_inodes_count != 0) {

            Vcb->GroupDesc[Group].bg_free_inodes_count = 0;

            RfsdSaveGroup(IrpContext, Vcb, Group);
        }

        goto repeat;

    } else {

        RtlSetBits(&InodeBitmap, dwInode, 1);

        CcSetDirtyPinnedData(BitmapBcb, NULL );

        RfsdRepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        RfsdAddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);

        *Inode = dwInode + 1 + Group * INODES_PER_GROUP;

        //Updating Group Desc / Superblock
        Vcb->GroupDesc[Group].bg_free_inodes_count--;
        if (Type == RFSD_FT_DIR) {
            Vcb->GroupDesc[Group].bg_used_dirs_count++;
        }

        RfsdSaveGroup(IrpContext, Vcb, Group);

        Vcb->SuperBlock->s_free_inodes_count--;
        RfsdSaveSuper(IrpContext, Vcb);
        
        return STATUS_SUCCESS;        
    }

    return STATUS_DISK_FULL;

#endif
}

BOOLEAN
RfsdFreeInode(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            ULONG Inode,
            ULONG Type )
{
DbgBreak();
#if DISABLED

    RTL_BITMAP      InodeBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    ULONG           Group;
    ULONG           Length;
    LARGE_INTEGER   Offset;

    ULONG           dwIno;
    BOOLEAN         bModified = FALSE;
    

    Group = (Inode - 1) / INODES_PER_GROUP;
    dwIno = (Inode - 1) % INODES_PER_GROUP;

    RfsdPrint((DBG_INFO, "RfsdFreeInode: Inode: %xh (Group/Off = %xh/%xh)\n",
                         Inode, Group, dwIno));
    
    {
        Offset.QuadPart = (LONGLONG) Vcb->BlockSize;
        Offset.QuadPart = Offset.QuadPart * Vcb->GroupDesc[Group].bg_inode_bitmap;
        if (Group == Vcb->NumOfGroups - 1) {

            Length = INODES_COUNT % INODES_PER_GROUP;
            if (!Length) {
                /* s_inodes_count is integer multiple of s_inodes_per_group */
                Length = INODES_PER_GROUP;
            }
        } else {
            Length = INODES_PER_GROUP;
        }

        if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &BitmapBcb,
                        &BitmapCache ) ) {
            RfsdPrint((DBG_ERROR, "RfsdFreeInode: PinReading error ...\n"));
            return FALSE;
        }

        RtlInitializeBitMap( &InodeBitmap,
                             BitmapCache,
                             Length );

        if (RtlCheckBit(&InodeBitmap, dwIno) == 0) {
            DbgBreak();
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
    }
        
    if (bModified) {

        CcSetDirtyPinnedData(BitmapBcb, NULL );

        RfsdRepinBcb(IrpContext, BitmapBcb);

        CcUnpinData(BitmapBcb);

        RfsdAddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->BlockSize);

        //Updating Group Desc / Superblock
        if (Type == RFSD_FT_DIR) {
            Vcb->GroupDesc[Group].bg_used_dirs_count--;
        }

        Vcb->GroupDesc[Group].bg_free_inodes_count++;
        RfsdSaveGroup(IrpContext, Vcb, Group);

        Vcb->SuperBlock->s_free_inodes_count++;
        RfsdSaveSuper(IrpContext, Vcb);

        return TRUE;
    }

    return FALSE;

#endif
}

NTSTATUS
RfsdAddEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               FileType,
         IN ULONG               Inode,
         IN PUNICODE_STRING     FileName )
{
DbgBreak();
#if DISABLED
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PRFSD_DIR_ENTRY2        pDir = NULL;
    PRFSD_DIR_ENTRY2        pNewDir = NULL;
    PRFSD_DIR_ENTRY2        pTarget = NULL;

    ULONG                   Length = 0;
    ULONG                   dwBytes = 0;

    BOOLEAN                 bFound = FALSE;
    BOOLEAN                 bAdding = FALSE;

    BOOLEAN                 MainResourceAcquired = FALSE;

    ULONG                   dwRet;

    if (!IsDirectory(Dcb)) {
        DbgBreak();
        return STATUS_NOT_A_DIRECTORY;
    }

    MainResourceAcquired = ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

    _SEH2_TRY {

        Dcb->ReferenceCount++;

        pDir = (PRFSD_DIR_ENTRY2) ExAllocatePoolWithTag(PagedPool,
                                    RFSD_DIR_REC_LEN(RFSD_NAME_LEN), RFSD_POOL_TAG);
        if (!pDir) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        pTarget = (PRFSD_DIR_ENTRY2) ExAllocatePoolWithTag(PagedPool,
                                     2 * RFSD_DIR_REC_LEN(RFSD_NAME_LEN), RFSD_POOL_TAG);
        if (!pTarget) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

#if DISABLED			// disabled in FFS too
        if (IsFlagOn( SUPER_BLOCK->s_feature_incompat, 
                      RFSD_FEATURE_INCOMPAT_FILETYPE)) {
            pDir->file_type = (UCHAR) FileType;
        } else 
#endif
		  {
            pDir->file_type = 0;
        }

        {
            OEM_STRING  OemName;
            OemName.Buffer = pDir->name;
            OemName.MaximumLength = RFSD_NAME_LEN;
            OemName.Length  = 0;

            Status = RfsdUnicodeToOEM(&OemName, FileName);

            if (!NT_SUCCESS(Status)) {
                _SEH2_LEAVE;
            }

            pDir->name_len = (CCHAR) OemName.Length;
        }

        pDir->inode  = Inode;
        pDir->rec_len = (USHORT) (RFSD_DIR_REC_LEN(pDir->name_len));

        dwBytes = 0;

Repeat:

        while ((LONGLONG)dwBytes < Dcb->Header.AllocationSize.QuadPart) {

            RtlZeroMemory(pTarget, RFSD_DIR_REC_LEN(RFSD_NAME_LEN));

            // Reading the DCB contents
            Status = RfsdReadInode(
                        NULL,
                        Vcb,
                        Dcb->RfsdMcb->Inode,
                        Dcb->Inode,
                        dwBytes,
                        (PVOID)pTarget,
                        RFSD_DIR_REC_LEN(RFSD_NAME_LEN),
                        &dwRet);

            if (!NT_SUCCESS(Status)) {
                RfsdPrint((DBG_ERROR, "RfsdAddDirectory: Reading Directory Content error.\n"));
                _SEH2_LEAVE;
            }

            if (((pTarget->inode == 0) && pTarget->rec_len >= pDir->rec_len) || 
                (pTarget->rec_len >= RFSD_DIR_REC_LEN(pTarget->name_len) + pDir->rec_len)) {

                if (pTarget->inode) {

                    RtlZeroMemory(pTarget, 2 * RFSD_DIR_REC_LEN(RFSD_NAME_LEN));

                    // Reading the DCB contents
                    Status = RfsdReadInode(
                                NULL,
                                Vcb,
                                Dcb->RfsdMcb->Inode,
                                Dcb->Inode,
                                dwBytes,
                                (PVOID)pTarget,
                                2 * RFSD_DIR_REC_LEN(RFSD_NAME_LEN),
                                &dwRet);

                    if (!NT_SUCCESS(Status)) {
                        RfsdPrint((DBG_ERROR, "RfsdAddDirectory: Reading Directory Content error.\n"));
                        _SEH2_LEAVE;
                    }

                    Length = RFSD_DIR_REC_LEN(pTarget->name_len);
                    
                    pNewDir = (PRFSD_DIR_ENTRY2) ((PUCHAR)pTarget + RFSD_DIR_REC_LEN(pTarget->name_len));

                    pNewDir->rec_len = pTarget->rec_len - RFSD_DIR_REC_LEN(pTarget->name_len);

                    pTarget->rec_len = RFSD_DIR_REC_LEN(pTarget->name_len);

                } else {

                    Length  = 0;
                    pNewDir = pTarget;
                }

                pNewDir->file_type = pDir->file_type;
                pNewDir->inode = pDir->inode;
                pNewDir->name_len = pDir->name_len;
                memcpy(pNewDir->name, pDir->name, pDir->name_len);
                Length += RFSD_DIR_REC_LEN(pDir->name_len);

                bFound = TRUE;
                break;
            }
            
            dwBytes += pTarget->rec_len;
        }

        if (bFound) {

            ULONG   dwRet;

            if ( FileType==RFSD_FT_DIR ) {

                if(((pDir->name_len == 1) && (pDir->name[0] == '.')) ||
                   ((pDir->name_len == 2) && (pDir->name[0] == '.') && (pDir->name[1] == '.')) ) {
                } else {
                    Dcb->Inode->i_links_count++;
                }
            }

            Status = RfsdWriteInode(
                        IrpContext,
                        Vcb,
                        Dcb->RfsdMcb->Inode,
                        Dcb->Inode,
                        (ULONGLONG)dwBytes,
                        pTarget,
                        Length,
                        FALSE,
                        &dwRet );
        } else {

            // We should expand the size of the dir inode 
            if (!bAdding) {

                ULONG dwRet;

                Status = RfsdExpandInode(IrpContext, Vcb, Dcb, &dwRet);

                if (NT_SUCCESS(Status)) {

                    Dcb->Inode->i_size = Dcb->Header.AllocationSize.LowPart;

                    RfsdSaveInode(IrpContext, Vcb, Dcb->RfsdMcb->Inode, Dcb->Inode);

                    Dcb->Header.FileSize = Dcb->Header.AllocationSize;

                    bAdding = TRUE;

                    goto Repeat;
                }

                _SEH2_LEAVE;

            } else { // Something must be error!
                _SEH2_LEAVE;
            }
        }

    } _SEH2_FINALLY {

        Dcb->ReferenceCount--;

        if(MainResourceAcquired)    {
            ExReleaseResourceForThreadLite(
                    &Dcb->MainResource,
                    ExGetCurrentResourceThread());
        }

        if (pTarget != NULL) {
            ExFreePool(pTarget);
        }

        if (pDir)   {
            ExFreePool(pDir);
        }
    } _SEH2_END;
    
    return Status;
#endif
}

NTSTATUS
RfsdRemoveEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               FileType,
         IN ULONG               Inode )
{
DbgBreak();
#if DISABLED
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PRFSD_DIR_ENTRY2        pTarget = NULL;
    PRFSD_DIR_ENTRY2        pPrevDir = NULL;

    USHORT                  PrevRecLen = 0;

    ULONG                   Length = 0;
    ULONG                   dwBytes = 0;

    BOOLEAN                 MainResourceAcquired = FALSE;

    ULONG                   dwRet;

    if (!IsDirectory(Dcb)) {
        return STATUS_NOT_A_DIRECTORY;
    }

    MainResourceAcquired = 
            ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

    _SEH2_TRY {

        Dcb->ReferenceCount++;

        pTarget = (PRFSD_DIR_ENTRY2) ExAllocatePoolWithTag(PagedPool,
                                     RFSD_DIR_REC_LEN(RFSD_NAME_LEN), RFSD_POOL_TAG);
        if (!pTarget) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }
        
        pPrevDir = (PRFSD_DIR_ENTRY2) ExAllocatePoolWithTag(PagedPool,
                                     RFSD_DIR_REC_LEN(RFSD_NAME_LEN), RFSD_POOL_TAG);
        if (!pPrevDir) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        dwBytes = 0;

        while ((LONGLONG)dwBytes < Dcb->Header.AllocationSize.QuadPart) {

            RtlZeroMemory(pTarget, RFSD_DIR_REC_LEN(RFSD_NAME_LEN));

            Status = RfsdReadInode(
                        NULL,
                        Vcb,
                        Dcb->RfsdMcb->Inode,
                        Dcb->Inode,
                        dwBytes,
                        (PVOID)pTarget,
                        RFSD_DIR_REC_LEN(RFSD_NAME_LEN),
                        &dwRet);

            if (!NT_SUCCESS(Status)) {

                RfsdPrint((DBG_ERROR, "RfsdRemoveEntry: Reading Directory Content error.\n"));
                _SEH2_LEAVE;
            }

            //
            // Find it ! Then remove the entry from Dcb ...
            //

            if (pTarget->inode == Inode) {

                ULONG   RecLen;

                BOOLEAN bAcross = FALSE;

                if (((ULONG)PrevRecLen + pTarget->rec_len) > BLOCK_SIZE) {
                    bAcross = TRUE;
                } else {

                    dwRet = (dwBytes - PrevRecLen) & (~(BLOCK_SIZE - 1));
                    if (dwRet != (dwBytes & (~(BLOCK_SIZE - 1))))
                    {
                        bAcross = TRUE;
                    }
                }

                if (!bAcross) {

                    pPrevDir->rec_len += pTarget->rec_len;

                    RecLen = RFSD_DIR_REC_LEN(pTarget->name_len);

                    RtlZeroMemory(pTarget, RecLen);

                    Status = RfsdWriteInode(
                                IrpContext,
                                Vcb,
                                Dcb->RfsdMcb->Inode,
                                Dcb->Inode,
                                dwBytes - PrevRecLen,
                                pPrevDir,
                                8,
                                FALSE,
                                &dwRet
                                );

                    ASSERT(NT_SUCCESS(Status));

                    Status = RfsdWriteInode(
                                IrpContext,
                                Vcb,
                                Dcb->RfsdMcb->Inode,
                                Dcb->Inode,
                                dwBytes,
                                pTarget,
                                RecLen,
                                FALSE,
                                &dwRet
                                );

                    ASSERT(NT_SUCCESS(Status));

                } else {

                    RecLen = (ULONG)pTarget->rec_len;
                    if (RecLen > RFSD_DIR_REC_LEN(RFSD_NAME_LEN)) {

                        RtlZeroMemory(pTarget, RFSD_DIR_REC_LEN(RFSD_NAME_LEN));

                        pTarget->rec_len = (USHORT)RecLen;

                        Status = RfsdWriteInode(
                                    IrpContext,
                                    Vcb,
                                    Dcb->RfsdMcb->Inode,
                                    Dcb->Inode,
                                    dwBytes,
                                    pTarget,
                                    RFSD_DIR_REC_LEN(RFSD_NAME_LEN),
                                    FALSE,
                                    &dwRet
                                    );

                        ASSERT(NT_SUCCESS(Status));

                    } else {

                        RtlZeroMemory(pTarget, RecLen);
                        pTarget->rec_len = (USHORT)RecLen;

                        Status = RfsdWriteInode(
                                    IrpContext,
                                    Vcb,
                                    Dcb->RfsdMcb->Inode,
                                    Dcb->Inode,
                                    dwBytes,
                                    pTarget,
                                    RecLen,
                                    FALSE,
                                    &dwRet
                                    );

                        ASSERT(NT_SUCCESS(Status));
                    }
                }

                //
                // Error if it's the entry of dot or dot-dot or drop the parent's refer link
                //

                if (FileType == RFSD_FT_DIR) {

                    if(((pTarget->name_len == 1) && (pTarget->name[0] == '.')) ||
                       ((pTarget->name_len == 2) && (pTarget->name[0] == '.') && (pTarget->name[1] == '.')) ) {

                        DbgBreak();
                    } else {
                        Dcb->Inode->i_links_count--;
                    }
                }

                //
                // Update at least mtime/atime if !RFSD_FT_DIR.
                //

                if ( !RfsdSaveInode(
                        IrpContext,
                        Vcb,
                        Dcb->RfsdMcb->Inode,
                        Dcb->Inode
                       ) ) {
                    Status = STATUS_UNSUCCESSFUL;
                }
              
                break;

            } else {

                RtlCopyMemory(pPrevDir, pTarget, RFSD_DIR_REC_LEN(RFSD_NAME_LEN));
                PrevRecLen = pTarget->rec_len;
            }

            dwBytes += pTarget->rec_len;
        }

    } _SEH2_FINALLY {

        Dcb->ReferenceCount--;

        if(MainResourceAcquired)
            ExReleaseResourceForThreadLite(
                    &Dcb->MainResource,
                    ExGetCurrentResourceThread());

        if (pTarget != NULL) {
            ExFreePool(pTarget);
        }

        if (pPrevDir != NULL) {
            ExFreePool(pPrevDir);
        }
    } _SEH2_END;
    
    return Status;
#endif
}

NTSTATUS
RfsdSetParentEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               OldParent,
         IN ULONG               NewParent )
{
DbgBreak();
#if DISABLED
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PRFSD_DIR_ENTRY2        pSelf   = NULL;
    PRFSD_DIR_ENTRY2        pParent = NULL;

    ULONG                   dwBytes = 0;

    BOOLEAN                 MainResourceAcquired = FALSE;

    ULONG                   Offset = 0;

    if (!IsDirectory(Dcb)) {
        return STATUS_NOT_A_DIRECTORY;
    }

    MainResourceAcquired = 
        ExAcquireResourceExclusiveLite(&Dcb->MainResource, TRUE);

    _SEH2_TRY {

        Dcb->ReferenceCount++;

        pSelf = (PRFSD_DIR_ENTRY2) ExAllocatePoolWithTag(PagedPool,
                                   RFSD_DIR_REC_LEN(1) + RFSD_DIR_REC_LEN(2), RFSD_POOL_TAG);
        if (!pSelf) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }

        dwBytes = 0;

        //
        // Reading the DCB contents
        //

        Status = RfsdReadInode(
                        NULL,
                        Vcb,
                        Dcb->RfsdMcb->Inode,
                        Dcb->Inode,
                        Offset,
                        (PVOID)pSelf,
                        RFSD_DIR_REC_LEN(1) + RFSD_DIR_REC_LEN(2),
                        &dwBytes );

        if (!NT_SUCCESS(Status)) {
            RfsdPrint((DBG_ERROR, "RfsdSetParentEntry: Reading Directory Content error.\n"));
            _SEH2_LEAVE;
        }

        ASSERT(dwBytes == RFSD_DIR_REC_LEN(1) + RFSD_DIR_REC_LEN(2));

        pParent = (PRFSD_DIR_ENTRY2)((PUCHAR)pSelf + pSelf->rec_len);

        if (pParent->inode != OldParent) {
            DbgBreak();
        }

        pParent->inode = NewParent;

        Status = RfsdWriteInode(
                        IrpContext,
                        Vcb, 
                        Dcb->RfsdMcb->Inode,
                        Dcb->Inode,
                        Offset,
                        pSelf,
                        dwBytes,
                        FALSE,
                        &dwBytes );

    } _SEH2_FINALLY {

        Dcb->ReferenceCount--;

        if(MainResourceAcquired)    {
            ExReleaseResourceForThreadLite(
                    &Dcb->MainResource,
                    ExGetCurrentResourceThread());
        }

        if (pSelf) {
            ExFreePool(pSelf);
        }
    } _SEH2_END;
    
    return Status;
#endif
	 { return 1; }
}

NTSTATUS
RfsdTruncateBlock(
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB Vcb,
    IN PRFSD_FCB Fcb,
    IN ULONG   dwContent,
    IN ULONG   Index,
    IN ULONG   layer,
    OUT BOOLEAN *bFreed )
{
DbgBreak();
#if DISABLED

    ULONG       *pData = NULL;
    ULONG       i = 0, j = 0, temp = 1;
    BOOLEAN     bDirty = FALSE;
    ULONG       dwBlk;

    LONGLONG    Offset;

    PBCB        Bcb;

    NTSTATUS    Status = STATUS_SUCCESS;

    PRFSD_INODE Inode = Fcb->Inode;

    *bFreed = FALSE;

    if (layer == 0) {

        if ( dwContent > 0 && (dwContent / BLOCKS_PER_GROUP) < Vcb->NumOfGroups) {

            Status = RfsdFreeBlock(IrpContext, Vcb, dwContent);

            if (NT_SUCCESS(Status)) {

                ASSERT(Inode->i_blocks >= (Vcb->BlockSize / SECTOR_SIZE));
                Inode->i_blocks -= (Vcb->BlockSize / SECTOR_SIZE);            
                *bFreed = TRUE;
            }

        } else if (dwContent == 0) {
            Status = STATUS_SUCCESS;
        } else {
            DbgBreak();
            Status = STATUS_INVALID_PARAMETER;
        }

    } else if (layer <= 3) {

        Offset = (LONGLONG) dwContent;
        Offset = Offset * Vcb->BlockSize;

        if (!CcPinRead( Vcb->StreamObj,
                        (PLARGE_INTEGER) (&Offset),
                        Vcb->BlockSize,
                        PIN_WAIT,
                        &Bcb,
                        &pData )) {

            RfsdPrint((DBG_ERROR, "RfsdSaveBuffer: PinReading error ...\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto errorout;
        }

        temp = 1 << ((BLOCK_BITS - 2) * (layer - 1));

        i = Index / temp;
        j = Index % temp;

        dwBlk = pData[i];

        if (dwBlk) {

            Status = RfsdTruncateBlock(
                        IrpContext,
                        Vcb,
                        Fcb,
                        dwBlk,
                        j,
                        layer - 1,
                        &bDirty
                        );

            if(!NT_SUCCESS(Status)) {
                goto errorout;
            }

            if (bDirty) {
                pData[i] = 0;
            }
        }

        if (i == 0 && j == 0) {

            CcUnpinData(Bcb);
            pData = NULL;

            Status = RfsdFreeBlock(IrpContext, Vcb, dwContent);

            if (NT_SUCCESS(Status)) {
                ASSERT(Inode->i_blocks >= (Vcb->BlockSize / SECTOR_SIZE));
                Inode->i_blocks -= (Vcb->BlockSize / SECTOR_SIZE);

                *bFreed = TRUE;
            }

        } else {

            if (bDirty) {
                CcSetDirtyPinnedData(Bcb, NULL );
                RfsdRepinBcb(IrpContext, Bcb);

                RfsdAddMcbEntry(Vcb, Offset, (LONGLONG)Vcb->BlockSize);
            }
        }
    }

errorout:

    if (pData) {
        CcUnpinData(Bcb);
    }

    return Status;

#endif
}

NTSTATUS
RfsdTruncateInode(
         IN PRFSD_IRP_CONTEXT IrpContext,
         IN PRFSD_VCB   Vcb,
         IN PRFSD_FCB   Fcb )
{
DbgBreak();
#if DISABLED

    ULONG   dwSizes[RFSD_BLOCK_TYPES];
    ULONG   Index = 0;
    ULONG   dwTotal = 0;
    ULONG   dwBlk = 0;

    ULONG    i;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  bFreed = FALSE;

    PRFSD_INODE Inode = Fcb->Inode;

    Index = (ULONG)(Fcb->Header.AllocationSize.QuadPart >> BLOCK_BITS);

    if (Index > 0) {
        Index--;
    } else {
        return Status;
    }

    for (i = 0; i < RFSD_BLOCK_TYPES; i++) {
        dwSizes[i] = Vcb->dwData[i];
        dwTotal += dwSizes[i];
    }

    if (Index >= dwTotal) {
        RfsdPrint((DBG_ERROR, "RfsdTruncateInode: beyond the maxinum size of an inode.\n"));
        return Status;
    }

    for (i = 0; i < RFSD_BLOCK_TYPES; i++) {

        if (Index < dwSizes[i]) {

            dwBlk = Inode->i_block[i==0 ? (Index):(i + RFSD_NDIR_BLOCKS - 1)];

            if (dwBlk) {

                Status = RfsdTruncateBlock(IrpContext, Vcb, Fcb, dwBlk, Index , i, &bFreed); 
            }

            if (NT_SUCCESS(Status)) {

                Fcb->Header.AllocationSize.QuadPart -= Vcb->BlockSize;
            
                if (bFreed) {
                    Inode->i_block[i==0 ? (Index):(i + RFSD_NDIR_BLOCKS - 1)] = 0;
                }
            }

            break;
        }

        Index -= dwSizes[i];
    }

    //
    // Inode struct saving is done externally.
    //

    RfsdSaveInode(IrpContext, Vcb, Fcb->RfsdMcb->Inode, Inode);


    return Status;
#endif
}

BOOLEAN
RfsdAddMcbEntry (		// Mcb = map control block (maps file-relative block offsets to disk-relative block offsets)
    IN PRFSD_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length)
{
DbgBreak();
#if DISABLED

    BOOLEAN     bRet = FALSE;

    LONGLONG    Offset;

#if DBG
    LONGLONG    DirtyLba;
    LONGLONG    DirtyLen;
#endif


    Offset = Lba & (~((LONGLONG)BLOCK_SIZE - 1));

    Length = (Length + Lba - Offset + BLOCK_SIZE - 1) &
             (~((LONGLONG)BLOCK_SIZE - 1));

    ASSERT ((Offset & (BLOCK_SIZE - 1)) == 0);
    ASSERT ((Length & (BLOCK_SIZE - 1)) == 0);

    Offset = (Offset >> BLOCK_BITS) + 1;
    Length = (Length >> BLOCK_BITS);

    ExAcquireResourceExclusiveLite(
        &(Vcb->McbResource),
        TRUE );

    RfsdPrint((DBG_INFO, "RfsdAddMcbEntry: Lba=%I64xh Length=%I64xh\n",
                          Offset, Length ));

#if DBG
    bRet = FsRtlLookupLargeMcbEntry(
                    &(Vcb->DirtyMcbs),
                    Offset,
                    &DirtyLba,
                    &DirtyLen,
                    NULL,
                    NULL,
                    NULL );

    if (bRet && DirtyLba == Offset && DirtyLen >= Length) {

        RfsdPrint((DBG_INFO, "RfsdAddMcbEntry: this run already exists.\n"));
    }
#endif

    _SEH2_TRY {

        bRet = FsRtlAddLargeMcbEntry(
                         &(Vcb->DirtyMcbs),
                         Offset, Offset,
                         Length );

    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {

        DbgBreak();
        bRet = FALSE;
    } _SEH2_END;

#if DBG
    if (bRet) {

        BOOLEAN     bFound = FALSE;
        LONGLONG    RunStart;
        LONGLONG    RunLength;
        ULONG       Index;

        bFound = FsRtlLookupLargeMcbEntry(
                        &(Vcb->DirtyMcbs),
                        Offset,
                        &DirtyLba,
                        &DirtyLen,
                        &RunStart,
                        &RunLength,
                        &Index );

        if ((!bFound) || (DirtyLba == -1) ||
            (DirtyLba != Offset) || (DirtyLen < Length)) {
            LONGLONG            DirtyVba;
            LONGLONG            DirtyLba;
            LONGLONG            DirtyLength;

            DbgBreak();

            for ( Index = 0; 
                  FsRtlGetNextLargeMcbEntry( &(Vcb->DirtyMcbs),
                                             Index,
                                             &DirtyVba,
                                             &DirtyLba,
                                             &DirtyLength); 
                  Index++ ) {

                RfsdPrint((DBG_INFO, "Index = %xh\n", Index));
                RfsdPrint((DBG_INFO, "DirtyVba = %I64xh\n", DirtyVba));
                RfsdPrint((DBG_INFO, "DirtyLba = %I64xh\n", DirtyLba));
                RfsdPrint((DBG_INFO, "DirtyLen = %I64xh\n\n", DirtyLength));
            }
        }
    }
#endif

    ExReleaseResourceForThreadLite(
        &(Vcb->McbResource),
        ExGetCurrentResourceThread() );

    return bRet;
#endif

}

VOID
RfsdRemoveMcbEntry (
    IN PRFSD_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length)
{
	DbgBreak();
#if DISABLED

    LONGLONG Offset;

    Offset = Lba & (~((LONGLONG)BLOCK_SIZE - 1));

    Length = (Length + Lba - Offset + BLOCK_SIZE - 1) &
             (~((LONGLONG)BLOCK_SIZE - 1));

    ASSERT(Offset == Lba);

    ASSERT ((Offset & (BLOCK_SIZE - 1)) == 0);
    ASSERT ((Length & (BLOCK_SIZE - 1)) == 0);

    Offset = (Offset >> BLOCK_BITS) + 1;
    Length = (Length >> BLOCK_BITS);

    RfsdPrint((DBG_INFO, "RfsdRemoveMcbEntry: Lba=%I64xh Length=%I64xh\n",
                          Offset, Length ));

    ExAcquireResourceExclusiveLite(
        &(Vcb->McbResource),
        TRUE );

    _SEH2_TRY {
        FsRtlRemoveLargeMcbEntry(
            &(Vcb->DirtyMcbs),
            Offset, Length      );

    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        DbgBreak();
    } _SEH2_END;

#if DBG
    {
        BOOLEAN  bFound = FALSE;
        LONGLONG DirtyLba, DirtyLen;

        bFound = FsRtlLookupLargeMcbEntry(
                        &(Vcb->DirtyMcbs),
                        Offset,
                        &DirtyLba,
                        &DirtyLen,
                        NULL,
                        NULL,
                        NULL );

        if (bFound &&( DirtyLba != -1)) {
            DbgBreak();
        }
    }
#endif

    ExReleaseResourceForThreadLite(
        &(Vcb->McbResource),
        ExGetCurrentResourceThread() );
#endif
}

BOOLEAN
RfsdLookupMcbEntry (
    IN PRFSD_VCB    Vcb,
    IN LONGLONG     Lba,
    OUT PLONGLONG   pLba,
    OUT PLONGLONG   pLength,
    OUT PLONGLONG   RunStart,
    OUT PLONGLONG   RunLength,
    OUT PULONG      Index)
{
DbgBreak();
#if DISABLED

    BOOLEAN     bRet;
    LONGLONG    Offset;


    Offset = Lba & (~((LONGLONG)BLOCK_SIZE - 1));
    ASSERT ((Offset & (BLOCK_SIZE - 1)) == 0);

    ASSERT(Lba == Offset);

    Offset = (Offset >> BLOCK_BITS) + 1;

    ExAcquireResourceExclusiveLite(
        &(Vcb->McbResource),
        TRUE );

    bRet = FsRtlLookupLargeMcbEntry(
                &(Vcb->DirtyMcbs),
                Offset,
                pLba,
                pLength,
                RunStart,
                RunLength,
                Index );

    ExReleaseResourceForThreadLite(
        &(Vcb->McbResource),
        ExGetCurrentResourceThread() );

    if (bRet) {

        if (pLba && ((*pLba) != -1)) {

            ASSERT((*pLba) > 0);

            (*pLba) = (((*pLba) - 1) << BLOCK_BITS);
            (*pLba) += ((Lba) & ((LONGLONG)BLOCK_SIZE - 1));
        }

        if (pLength) {

            (*pLength) <<= BLOCK_BITS;
            (*pLength)  -= ((Lba) & ((LONGLONG)BLOCK_SIZE - 1));
        }

        if (RunStart && (*RunStart != -1)) {
            (*RunStart) = (((*RunStart) - 1) << BLOCK_BITS);
        }

        if (RunLength) {
            (*RunLength) <<= BLOCK_BITS;
        }
    }

    return bRet;

#endif
}

#endif // 0

///////////////////////////////////////////////////
///////////////////////////////////////////////////

/** 
 Invoked by FSCTL.c on mount volume
*/
BOOLEAN SuperblockContainsMagicKey(PRFSD_SUPER_BLOCK sb)
{	
#define  MAGIC_KEY_LENGTH							9
	
	UCHAR				sz_MagicKey[]	= REISER2FS_SUPER_MAGIC_STRING;
#ifndef __REACTOS__
	BOOLEAN			b_KeyMatches	= TRUE;
#endif
	UCHAR				currentChar;
	int				i;

    PAGED_CODE();

	// If any characters read from disk don't match the expected magic key, we don't have a ReiserFS volume.	
	for (i = 0; i < MAGIC_KEY_LENGTH; i++) 
	{ 
		currentChar = sb->s_magic[i];
		if (currentChar != sz_MagicKey[i])
			{ return FALSE; }
	}

	return TRUE;
}



/*
______________________________________________________________________________
 _  __             _   _ _   _ _     
| |/ /___ _   _   | | | | |_(_) |___ 
| ' // _ \ | | |  | | | | __| | / __|
| . \  __/ |_| |  | |_| | |_| | \__ \
|_|\_\___|\__, |   \___/ \__|_|_|___/
          |___/                      
______________________________________________________________________________
*/


/** 
 Guess whether a key is v1, or v2, by investigating its type field.
 NOTE: I based this off Florian Buchholz's code snippet, which is from reisefs lib.
 
 Old keys (on i386) have k_offset_v2.k_type == 15 (direct and indirect) or == 0 (dir items and stat data).
*/

RFSD_KEY_VERSION DetermineOnDiskKeyFormat(const PRFSD_KEY_ON_DISK key)
{
    int type = (int) key->u.k_offset_v2.k_type;

    PAGED_CODE();

    if ( type == 0x0 || type == 0xF )
        return RFSD_KEY_VERSION_1;

    return RFSD_KEY_VERSION_2;
}


/** Given the uniqueness value from a version 1 KEY_ON_DISK, convert that to the v2 equivalent type (which is used for the KEY_IN_MEMORY structures) */
__u32
ConvertKeyTypeUniqueness(__u32 k_uniqueness)
{
	switch (k_uniqueness)
	{
	case RFSD_KEY_TYPE_v1_STAT_DATA:		return RFSD_KEY_TYPE_v2_STAT_DATA;
	case RFSD_KEY_TYPE_v1_INDIRECT:			return RFSD_KEY_TYPE_v2_INDIRECT;
	case RFSD_KEY_TYPE_v1_DIRECT:			return RFSD_KEY_TYPE_v2_DIRECT;
	case RFSD_KEY_TYPE_v1_DIRENTRY:			return RFSD_KEY_TYPE_v2_DIRENTRY;
	
	default:
		RfsdPrint((DBG_ERROR, "Unexpected uniqueness value %i", k_uniqueness));
		// NOTE: If above value is 555, it's the 'any' value, which I'd be surprised to see on disk.
		DbgBreak();
		return 0xF;	// We'll return v2 'any', just to see what happens...
	}
}

/** Fills an in-memory key structure with equivalent data as that given by an on-disk key, converting any older v1 information ito the new v2 formats. */
void
FillInMemoryKey(
	IN		PRFSD_KEY_ON_DISK		pKeyOnDisk, 
	IN		RFSD_KEY_VERSION		KeyVersion, 
	IN OUT	PRFSD_KEY_IN_MEMORY		pKeyInMemory		)
{
    PAGED_CODE();

	// Sanity check that the input and output locations exist
	if (!pKeyOnDisk || !pKeyInMemory) { DbgBreak(); return; }

	// Copy over the fields that are compatible between keys
	pKeyInMemory->k_dir_id		= pKeyOnDisk->k_dir_id;
	pKeyInMemory->k_objectid	= pKeyOnDisk->k_objectid;	

	if (KeyVersion == RFSD_KEY_VERSION_UNKNOWN)	
		{ KeyVersion = DetermineOnDiskKeyFormat(pKeyOnDisk); }
	
	// Copy over the fields that are incompatible between keys, converting older type fields to the v2 format
	switch (KeyVersion)
	{
	case RFSD_KEY_VERSION_1: 
		pKeyInMemory->k_offset	= pKeyOnDisk->u.k_offset_v1.k_offset;
		pKeyInMemory->k_type	= ConvertKeyTypeUniqueness( pKeyOnDisk->u.k_offset_v1.k_uniqueness );
		break;

	case RFSD_KEY_VERSION_2: 
		pKeyInMemory->k_offset	= pKeyOnDisk->u.k_offset_v2.k_offset;
		pKeyInMemory->k_type	= (__u32) pKeyOnDisk->u.k_offset_v2.k_type;
		break;
	}	
}

/** Compares two in memory keys, returning KEY_SMALLER, KEY_LARGER, or KEYS_MATCH relative to the first key given. */
RFSD_KEY_COMPARISON
CompareShortKeys(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b		)
{
    PAGED_CODE();

    // compare 1. integer	
    if( a->k_dir_id < b->k_dir_id )        return RFSD_KEY_SMALLER;
    if( a->k_dir_id > b->k_dir_id )        return RFSD_KEY_LARGER;
    
    // compare 2. integer
    if( a->k_objectid < b->k_objectid )    return RFSD_KEY_SMALLER;
    if( a->k_objectid > b->k_objectid )    return RFSD_KEY_LARGER;

	return RFSD_KEYS_MATCH;
}


/** Compares two in memory keys, returning KEY_SMALLER, KEY_LARGER, or KEYS_MATCH relative to the first key given. */
RFSD_KEY_COMPARISON
CompareKeysWithoutOffset(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b		)
{
    PAGED_CODE();

    // compare 1. integer	
    if( a->k_dir_id < b->k_dir_id )        return RFSD_KEY_SMALLER;
    if( a->k_dir_id > b->k_dir_id )        return RFSD_KEY_LARGER;
    
    // compare 2. integer
    if( a->k_objectid < b->k_objectid )    return RFSD_KEY_SMALLER;
    if( a->k_objectid > b->k_objectid )    return RFSD_KEY_LARGER;
    
	// compare 4. integer
	// NOTE: Buchholz says that if we get to here in navigating the file tree, something has gone wrong...
    if( a->k_type < b->k_type )		       return RFSD_KEY_SMALLER;
    if( a->k_type > b->k_type )            return RFSD_KEY_LARGER;

	return RFSD_KEYS_MATCH;
}


/** Compares two in memory keys, returning KEY_SMALLER, KEY_LARGER, or KEYS_MATCH relative to the first key given. */
RFSD_KEY_COMPARISON
CompareKeys(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b		)
{
    PAGED_CODE();

    // compare 1. integer	
    if( a->k_dir_id < b->k_dir_id )        return RFSD_KEY_SMALLER;
    if( a->k_dir_id > b->k_dir_id )        return RFSD_KEY_LARGER;
    
    // compare 2. integer
    if( a->k_objectid < b->k_objectid )    return RFSD_KEY_SMALLER;
    if( a->k_objectid > b->k_objectid )    return RFSD_KEY_LARGER;
    
    // compare 3. integer
    if( a->k_offset < b->k_offset )        return RFSD_KEY_SMALLER;
    if( a->k_offset > b->k_offset )        return RFSD_KEY_LARGER;   

    // compare 4. integer
	// NOTE: Buchholz says that if we get to here in navigating the file tree, something has gone wrong...
    if( a->k_type < b->k_type )		       return RFSD_KEY_SMALLER;
    if( a->k_type > b->k_type )            return RFSD_KEY_LARGER;

	return RFSD_KEYS_MATCH;
}



/*
______________________________________________________________________________
 _____                 _   _             _             _   _             
|_   _| __ ___  ___   | \ | | __ ___   _(_) __ _  __ _| |_(_) ___  _ __  
  | || '__/ _ \/ _ \  |  \| |/ _` \ \ / / |/ _` |/ _` | __| |/ _ \| '_ \ 
  | || | |  __/  __/  | |\  | (_| |\ V /| | (_| | (_| | |_| | (_) | | | |
  |_||_|  \___|\___|  |_| \_|\__,_| \_/ |_|\__, |\__,_|\__|_|\___/|_| |_|
                                           |___/                         
______________________________________________________________________________
*/

NTSTATUS
NavigateToLeafNode(
		    IN	PRFSD_VCB					Vcb,
			IN	PRFSD_KEY_IN_MEMORY			Key,						// Key to search for.
			IN	ULONG						StartingBlockNumber,		// Block number of an internal or leaf node, to start the search from
			OUT	PULONG						out_NextBlockNumber			// Block number of the leaf node that contains Key
			)
{
    PAGED_CODE();

	return _NavigateToLeafNode(Vcb, Key, StartingBlockNumber, out_NextBlockNumber, TRUE, &CompareKeys, NULL, NULL);
}

NTSTATUS
RfsdParseFilesystemTree(
			IN	PRFSD_VCB					Vcb,
			IN	PRFSD_KEY_IN_MEMORY			Key,						// Key to search for.
			IN	ULONG						StartingBlockNumber,		// Block number of an internal or leaf node, to start the search from			
			IN	RFSD_CALLBACK(fpDirectoryCallback),						// A function ptr to trigger on hitting a matching leaf block
			IN  PVOID						pContext					// This context item will simply be passed through to the callback when invoked
			)
{
	ULONG out = 0;

    PAGED_CODE();

	return _NavigateToLeafNode(Vcb, Key, StartingBlockNumber, &out, FALSE, &CompareShortKeys, fpDirectoryCallback, pContext);
}


/**
 Returns the block number of the leaf node that should contain key (if that key exists at all within the disk tree).

 STATUS_INVALID_HANDLE				if a block or block header could not be read
 STATUS_INSUFFICIENT_RESOURCES		if processing was terminated due to a failure of memory allocation
 STATUS_SUCCESS						on success
 NOTE: the return value can also be anything defined by the invoked callback
**/
NTSTATUS
_NavigateToLeafNode(
		    IN	PRFSD_VCB					Vcb,
			IN	PRFSD_KEY_IN_MEMORY			Key,						// Key to search for.
			IN	ULONG						StartingBlockNumber,		// Block number of an internal or leaf node, to start the search from
			OUT	PULONG						out_NextBlockNumber,			// Block number of the leaf node that contains Key (when a callback is in use, this is typically ignored -- it returns the block number at which the callback was last triggered)
			IN	BOOLEAN						ReturnOnFirstMatch,			// Whether or not the function should return upon finding the first leaf node containing the Key
			IN	RFSD_KEY_COMPARISON (*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY),
			RFSD_CALLBACK(fpDirectoryCallback),							// A function ptr to trigger on hitting a matching leaf block
			IN	PVOID						pContext					// This context item will simply be passed through to the callback when invoked
									

			)
{	
	NTSTATUS				Status = STATUS_SUCCESS;
	ULONG					leafNodeBlockNumber;					// The result to be calculated
	PRFSD_DISK_NODE_REF		pTargetDiskNodeReference = NULL;		// 

	// Read in this disk node's data
	PUCHAR pBlockBuffer = RfsdAllocateAndLoadBlock(Vcb, StartingBlockNumber);	

	// Read the block header
	PRFSD_BLOCK_HEAD pBlockHeader = (PRFSD_BLOCK_HEAD) pBlockBuffer;

    PAGED_CODE();

	// Sanity check that we could read the block and the header is there
	if (!pBlockBuffer) { return STATUS_INVALID_HANDLE; }

	// If this block is a leaf, just return it (or invoke the given callback on the leaf block)	
	if (pBlockHeader->blk_level == RFSD_LEAF_BLOCK_LEVEL)
	{
		NTSTATUS	CallbackStatus;

		ExFreePool(pBlockBuffer);
		
		*out_NextBlockNumber = StartingBlockNumber;

		// If a callback should be invoked on finding a matching leaf node, do so...
		if (fpDirectoryCallback) 	return (*fpDirectoryCallback)(StartingBlockNumber, pContext);	
		else						return STATUS_SUCCESS;
	}

	// Otherwise, find the next node down in the tree, by obtaining pTargetDiskNodeReference
	{
		ULONG				idxRightKey			= 0;
		PRFSD_KEY_ON_DISK	pLeftKeyOnDisk		= NULL;
		PRFSD_KEY_ON_DISK	pRightKeyOnDisk		= NULL;

		RFSD_KEY_IN_MEMORY	LeftKeyInMemory, RightKeyInMemory;
		RFSD_KEY_COMPARISON	leftComparison,	 rightComparison;

		RightKeyInMemory.k_dir_id = 0;  // (Dummy statement to prevent needless warning aboujt using RightKeyInMemory before being initialized)

		// Search (within the increasing list of target Keys), for the target key that Key is <= to.
		for (idxRightKey = 0; idxRightKey <= pBlockHeader->blk_nr_item; idxRightKey++)
		{
			// Advance the left key to become what was the right key, and the right key to become the next key 
			pLeftKeyOnDisk		= pRightKeyOnDisk;
			pRightKeyOnDisk		= (idxRightKey == pBlockHeader->blk_nr_item) ? 
				(PRFSD_KEY_ON_DISK) NULL :
				(PRFSD_KEY_ON_DISK) (pBlockBuffer + sizeof(RFSD_BLOCK_HEAD) + (idxRightKey * sizeof(RFSD_KEY_ON_DISK)));
			
			LeftKeyInMemory = RightKeyInMemory;
			if (pRightKeyOnDisk)
				FillInMemoryKey(pRightKeyOnDisk, RFSD_KEY_VERSION_UNKNOWN, &(RightKeyInMemory));


			// Find if the target key falls in the range in between the left and right keys...
			{
			  // We must be smaller than the right key (if it exists).  However, we will allow the key to match if short key comparisons are in use.
			  rightComparison = pRightKeyOnDisk ? ((*fpComparisonFunction)(Key, &RightKeyInMemory)) : RFSD_KEY_SMALLER;
			  if (fpComparisonFunction == &CompareShortKeys)
				{ if (rightComparison == RFSD_KEY_LARGER)	 continue; }
			  else 
			    { if (rightComparison != RFSD_KEY_SMALLER)   continue; }

			  // And larger than or equal to the left key.
			  leftComparison  = pLeftKeyOnDisk ?  ((*fpComparisonFunction)(Key, &LeftKeyInMemory))  : RFSD_KEY_LARGER;
			  if ( (leftComparison == RFSD_KEY_LARGER) || (leftComparison == RFSD_KEYS_MATCH) )
			  {
				  // The target range has been found.  Read the reference to the disk node child, lower in the tree.
				  // This returns the pointer preceding the righthand key.
				  pTargetDiskNodeReference = (PRFSD_DISK_NODE_REF) (pBlockBuffer
					  + sizeof(RFSD_BLOCK_HEAD) + (pBlockHeader->blk_nr_item * sizeof(RFSD_KEY_ON_DISK)) + (idxRightKey *	sizeof(RFSD_DISK_NODE_REF)));
				  
				  // Continue recursion downwards; eventually a leaf node will be returned.
				  Status = _NavigateToLeafNode(
					  Vcb, Key, pTargetDiskNodeReference->dc_block_number, 
					  &(leafNodeBlockNumber),
					  ReturnOnFirstMatch, fpComparisonFunction, fpDirectoryCallback, pContext);	// <

				  if (ReturnOnFirstMatch || Status == STATUS_EVENT_DONE ||								// Success cases
					  Status == STATUS_INSUFFICIENT_RESOURCES || Status == STATUS_INVALID_HANDLE)		// Error cases
					  { goto return_results; }
			  }
			}
		}
	}

return_results:

	ExFreePool(pBlockBuffer);
	*out_NextBlockNumber = leafNodeBlockNumber;
	return Status;
}
