/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             rfsdblock.c
 * PURPOSE:          Disk block operations that are specific to ReiserFS.
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdAllocateAndLoadBlock)
#pragma alloc_text(PAGE, RfsdFindItemHeaderInBlock)
#pragma alloc_text(PAGE, RfsdLoadItem)
#endif

/* FUNCTIONS *************************************************************/

/** 
 Returns the address of an allocated buffer (WHICH THE CALLER WILL BE RESPONSIBLE FOR FREEING!), filled with the contents of the given block number on disk.
 (This is really just an alternative interface to LoadBlock)
*/
PUCHAR
RfsdAllocateAndLoadBlock(
	IN	PRFSD_VCB			Vcb,
	IN	ULONG				BlockIndex)			// The ordinal block number to read
{
	// Allocate the return buffer (the caller will be responsible for freeing this!)
	PUCHAR pReturnBuffer = ExAllocatePoolWithTag(NonPagedPool, Vcb->BlockSize, RFSD_POOL_TAG);		// NOTE: for now, I'm switching this to non-paged, because i was getting crashes

    PAGED_CODE();

    if (!pReturnBuffer) 
		{ DbgBreak(); return NULL; }

	// Read the block in from disk, or from cache.
	if (!RfsdLoadBlock(Vcb, BlockIndex, pReturnBuffer))
		{ DbgBreak(); ExFreePool(pReturnBuffer);  return NULL;  }	

	// Return the result to the caller.
	return pReturnBuffer;
}

/**
  Finds the item header inside a given leaf node block that matches a given key.

  STATUS_INVALID_HANDLE if the block given to search is invalid
  STATUS_NO_SUCH_MEMBER if the key is not found in the block given
*/
NTSTATUS
RfsdFindItemHeaderInBlock(
	 IN PRFSD_VCB			Vcb,
	 IN PRFSD_KEY_IN_MEMORY	pTargetKey,					// The key to match against
	 IN PUCHAR				pBlockBuffer,				// A filled disk block, provided by the caller
	 OUT PRFSD_ITEM_HEAD*	ppMatchingItemHeader,		// A pointer to a PRFSD_ITEM_HEAD.  The PRFSD_ITEM_HEAD will point to the item head matching Key, or NULL if there was no such item head in the given block.
	 IN	RFSD_KEY_COMPARISON (*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY)
	 )
{
	// Read the block header
	PRFSD_BLOCK_HEAD	pBlockHeader			= (PRFSD_BLOCK_HEAD) pBlockBuffer;
	PUCHAR				pItemHeaderListBuffer	= (PUCHAR) pBlockBuffer + sizeof(RFSD_BLOCK_HEAD);

    PAGED_CODE();

	*ppMatchingItemHeader = NULL;

	// Sanity check that the block (and therefore its header) is there
	if (!pBlockHeader) { return STATUS_INVALID_HANDLE; }
	ASSERT(pBlockHeader->blk_level == 1);
	
	// Search through the item headers to find one with a key matching pTargetKey
	{
		ULONG					idxCurrentItemHeader	= 0;
		PRFSD_ITEM_HEAD			pCurrentItemHeader		= NULL;
		RFSD_KEY_IN_MEMORY		CurrentItemKey;

		for(idxCurrentItemHeader = 0; idxCurrentItemHeader < pBlockHeader->blk_nr_item; idxCurrentItemHeader++)
		{
			// Grab the item header, and its key
			pCurrentItemHeader = (PRFSD_ITEM_HEAD) (pItemHeaderListBuffer + idxCurrentItemHeader * sizeof(RFSD_ITEM_HEAD));
					
			FillInMemoryKey(
				&( pCurrentItemHeader->ih_key ), pCurrentItemHeader->ih_version, 
				&( CurrentItemKey )	);											// <
			
			// Check if this item is the one being searched for
			if ( RFSD_KEYS_MATCH == (*fpComparisonFunction)( pTargetKey , &CurrentItemKey ) )
			{
				*ppMatchingItemHeader = pCurrentItemHeader;
				return STATUS_SUCCESS;
			}
		}

		// If a matching key was never found, simply return
		return STATUS_NO_SUCH_MEMBER;
	}	
}

/** 
Given an item's key, load the block, the item header, and the item buffer associated with the key. 

STATUS_INTERNAL_ERROR				if leaf node could not be reached
STATUS_NO_SUCH_MEMBER				if the item header could not be located
STATUS_INSUFFICIENT_RESOURCES		if the leaf node buffer could not be allocated

*/
NTSTATUS
RfsdLoadItem(
	IN	 PRFSD_VCB				Vcb,
	IN   PRFSD_KEY_IN_MEMORY	pItemKey,					// The key of the item to find
	OUT  PRFSD_ITEM_HEAD*		ppMatchingItemHeader,
	OUT  PUCHAR*				ppItemBuffer,				
	OUT  PUCHAR*				ppBlockBuffer,				// Block buffer, which backs the other output data structures.  The caller must free this (even in the case of an error)!
	OPTIONAL OUT PULONG			pBlockNumber,				// The ordinal disk block number at which the item was found
	IN	RFSD_KEY_COMPARISON (*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY)
	)
{
	NTSTATUS	Status;
	ULONG		LeafNodeBlockNumber;

    PAGED_CODE();

	// Clear the output pointers
	*ppItemBuffer = *ppBlockBuffer = NULL;
	*ppMatchingItemHeader = NULL;
	if (pBlockNumber) *pBlockNumber = 0;


	// Find the block number of the leaf node bock (on disk), that should contain the item w/ pItemKey	
	Status = NavigateToLeafNode(
		Vcb, pItemKey, Vcb->SuperBlock->s_root_block, 
		&(LeafNodeBlockNumber) 
	);	
	if (!NT_SUCCESS(Status)){ DbgBreak(); return STATUS_INTERNAL_ERROR; }
	if (pBlockNumber) *pBlockNumber = LeafNodeBlockNumber;


	// Load the block (which the caller must later free)
	*ppBlockBuffer = RfsdAllocateAndLoadBlock(Vcb, LeafNodeBlockNumber);
    if (!*ppBlockBuffer) { return STATUS_INSUFFICIENT_RESOURCES; }	


	// Get the item header and its information
	Status = RfsdFindItemHeaderInBlock(
		Vcb, pItemKey, *ppBlockBuffer,
		( ppMatchingItemHeader ),			//< 
		fpComparisonFunction
	); 
	if (Status == STATUS_NO_SUCH_MEMBER)		{ return STATUS_NO_SUCH_MEMBER; }
	if (!*ppMatchingItemHeader)					{ return STATUS_INTERNAL_ERROR; }
	
	// Setup the item buffer
	*ppItemBuffer = (PUCHAR) *ppBlockBuffer + (*ppMatchingItemHeader)->ih_item_location;

	return STATUS_SUCCESS;
}
