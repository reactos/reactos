/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <freeldr.h>
#include "cm.h"
#include <mm.h>
#include <disk.h>
#include <rtl.h>
#include <debug.h>
#include <arch.h>

// Returns a pointer to a CACHE_BLOCK structure
// Adds the block to the cache manager block list
// in cache memory if it isn't already there
PCACHE_BLOCK CacheInternalGetBlockPointer(PCACHE_DRIVE CacheDrive, U32 BlockNumber)
{
	PCACHE_BLOCK	CacheBlock = NULL;

	DbgPrint((DPRINT_CACHE, "CacheInternalGetBlockPointer() BlockNumber = %d\n", BlockNumber));

	CacheBlock = CacheInternalFindBlock(CacheDrive, BlockNumber);

	if (CacheBlock != NULL)
	{
		DbgPrint((DPRINT_CACHE, "Cache hit! BlockNumber: %d CacheBlock->BlockNumber: %d\n", BlockNumber, CacheBlock->BlockNumber));

		return CacheBlock;
	}

	DbgPrint((DPRINT_CACHE, "Cache miss! BlockNumber: %d\n", BlockNumber));

	CacheBlock = CacheInternalAddBlockToCache(CacheDrive, BlockNumber);

	// Optimize the block list so it has a LRU structure
	CacheInternalOptimizeBlockList(CacheDrive, CacheBlock);

	return CacheBlock;
}

PCACHE_BLOCK CacheInternalFindBlock(PCACHE_DRIVE CacheDrive, U32 BlockNumber)
{
	PCACHE_BLOCK	CacheBlock = NULL;

	DbgPrint((DPRINT_CACHE, "CacheInternalFindBlock() BlockNumber = %d\n", BlockNumber));

	//
	// Make sure the block list has entries before I start searching it.
	//
	if (!RtlListIsEmpty((PLIST_ITEM)CacheDrive->CacheBlockHead))
	{
		//
		// Search the list and find the BIOS drive number
		//
		CacheBlock = CacheDrive->CacheBlockHead;

		while (CacheBlock != NULL)
		{
			//
			// We found the block, so return it
			//
			if (CacheBlock->BlockNumber == BlockNumber)
			{
				//
				// Increment the blocks access count
				//
				CacheBlock->AccessCount++;

				return CacheBlock;
			}

			CacheBlock = (PCACHE_BLOCK)RtlListGetNext((PLIST_ITEM)CacheBlock);
		}
	}
	
	return NULL;
}

PCACHE_BLOCK CacheInternalAddBlockToCache(PCACHE_DRIVE CacheDrive, U32 BlockNumber)
{
	PCACHE_BLOCK	CacheBlock = NULL;

	DbgPrint((DPRINT_CACHE, "CacheInternalAddBlockToCache() BlockNumber = %d\n", BlockNumber));

	// Check the size of the cache so we don't exceed our limits
	CacheInternalCheckCacheSizeLimits(CacheDrive);

	// We will need to add the block to the
	// drive's list of cached blocks. So allocate
	// the block memory.
	CacheBlock = MmAllocateMemory(sizeof(CACHE_BLOCK));
	if (CacheBlock == NULL)
	{
		return NULL;
	}

	// Now initialize the structure and
	// allocate room for the block data
	RtlZeroMemory(CacheBlock, sizeof(CACHE_BLOCK));
	CacheBlock->BlockNumber = BlockNumber;
	CacheBlock->BlockData = MmAllocateMemory(CacheDrive->BlockSize * CacheDrive->DriveGeometry.BytesPerSector);
	if (CacheBlock->BlockData ==NULL)
	{
		MmFreeMemory(CacheBlock);
		return NULL;
	}

	// Now try to read in the block
	if (!DiskReadLogicalSectors(CacheDrive->DriveNumber, (BlockNumber * CacheDrive->BlockSize), CacheDrive->BlockSize, (PVOID)DISKREADBUFFER))
	{
		MmFreeMemory(CacheBlock->BlockData);
		MmFreeMemory(CacheBlock);
		return NULL;
	}
	RtlCopyMemory(CacheBlock->BlockData, (PVOID)DISKREADBUFFER, CacheDrive->BlockSize * CacheDrive->DriveGeometry.BytesPerSector);

	// Add it to our list of blocks managed by the cache
	if (CacheDrive->CacheBlockHead == NULL)
	{
		CacheDrive->CacheBlockHead = CacheBlock;
	}
	else
	{
		RtlListInsertTail((PLIST_ITEM)CacheDrive->CacheBlockHead, (PLIST_ITEM)CacheBlock);
	}

	// Update the cache data
	CacheBlockCount++;
	CacheSizeCurrent = CacheBlockCount * (CacheDrive->BlockSize * CacheDrive->DriveGeometry.BytesPerSector);

	CacheInternalDumpBlockList(CacheDrive);

	return CacheBlock;
}

BOOL CacheInternalFreeBlock(PCACHE_DRIVE CacheDrive)
{
	PCACHE_BLOCK	CacheBlockToFree;

	DbgPrint((DPRINT_CACHE, "CacheInternalFreeBlock()\n"));

	// Get a pointer to the last item in the block list
	// that isn't forced to be in the cache and remove
	// it from the list
	CacheBlockToFree = (PCACHE_BLOCK)RtlListGetTail((PLIST_ITEM)CacheDrive->CacheBlockHead);
	while (CacheBlockToFree != NULL && CacheBlockToFree->LockedInCache == TRUE)
	{
		CacheBlockToFree = (PCACHE_BLOCK)RtlListGetPrevious((PLIST_ITEM)CacheBlockToFree);
	}

	// No blocks left in cache that can be freed
	// so just return
	if (CacheBlockToFree == NULL)
	{
		return FALSE;
	}

	//
	// If we are freeing the head of the list then update it's pointer
	//
	if (CacheBlockToFree == CacheDrive->CacheBlockHead)
	{
		CacheDrive->CacheBlockHead = (PCACHE_BLOCK)RtlListGetNext((PLIST_ITEM)CacheBlockToFree);
	}

	RtlListRemoveEntry((PLIST_ITEM)CacheBlockToFree);

	// Free the block memory and the block structure
	MmFreeMemory(CacheBlockToFree->BlockData);
	MmFreeMemory(CacheBlockToFree);

	// Update the cache data
	CacheBlockCount--;
	CacheSizeCurrent = CacheBlockCount * (CacheDrive->BlockSize * CacheDrive->DriveGeometry.BytesPerSector);

	return TRUE;
}

VOID CacheInternalCheckCacheSizeLimits(PCACHE_DRIVE CacheDrive)
{
	U32		NewCacheSize;

	DbgPrint((DPRINT_CACHE, "CacheInternalCheckCacheSizeLimits()\n"));

	// Calculate the size of the cache if we added a block
	NewCacheSize = (CacheBlockCount + 1) * (CacheDrive->BlockSize * CacheDrive->DriveGeometry.BytesPerSector);

	// Check the new size against the cache size limit
	if (NewCacheSize > CacheSizeLimit)
	{
		CacheInternalFreeBlock(CacheDrive);
		CacheInternalDumpBlockList(CacheDrive);
	}
}

VOID CacheInternalDumpBlockList(PCACHE_DRIVE CacheDrive)
{
	PCACHE_BLOCK	CacheBlock;

	DbgPrint((DPRINT_CACHE, "Dumping block list for BIOS drive 0x%x.\n", CacheDrive->DriveNumber));
	DbgPrint((DPRINT_CACHE, "Cylinders: %d.\n", CacheDrive->DriveGeometry.Cylinders));
	DbgPrint((DPRINT_CACHE, "Heads: %d.\n", CacheDrive->DriveGeometry.Heads));
	DbgPrint((DPRINT_CACHE, "Sectors: %d.\n", CacheDrive->DriveGeometry.Sectors));
	DbgPrint((DPRINT_CACHE, "BytesPerSector: %d.\n", CacheDrive->DriveGeometry.BytesPerSector));
	DbgPrint((DPRINT_CACHE, "BlockSize: %d.\n", CacheDrive->BlockSize));
	DbgPrint((DPRINT_CACHE, "CacheSizeLimit: %d.\n", CacheSizeLimit));
	DbgPrint((DPRINT_CACHE, "CacheSizeCurrent: %d.\n", CacheSizeCurrent));
	DbgPrint((DPRINT_CACHE, "CacheBlockCount: %d.\n", CacheBlockCount));
	DbgPrint((DPRINT_CACHE, "Dumping %d cache blocks.\n", RtlListCountEntries((PLIST_ITEM)CacheDrive->CacheBlockHead)));

	CacheBlock = CacheDrive->CacheBlockHead;
	while (CacheBlock != NULL)
	{
		DbgPrint((DPRINT_CACHE, "Cache Block: CacheBlock: 0x%x\n", CacheBlock));
		DbgPrint((DPRINT_CACHE, "Cache Block: Block Number: %d\n", CacheBlock->BlockNumber));
		DbgPrint((DPRINT_CACHE, "Cache Block: Access Count: %d\n", CacheBlock->AccessCount));
		DbgPrint((DPRINT_CACHE, "Cache Block: Block Data: 0x%x\n", CacheBlock->BlockData));
		DbgPrint((DPRINT_CACHE, "Cache Block: Locked In Cache: %d\n", CacheBlock->LockedInCache));

		if (CacheBlock->BlockData == NULL)
		{
			BugCheck((DPRINT_CACHE, "What the heck?!?\n"));
		}

		CacheBlock = (PCACHE_BLOCK)RtlListGetNext((PLIST_ITEM)CacheBlock);
	}
}

VOID CacheInternalOptimizeBlockList(PCACHE_DRIVE CacheDrive, PCACHE_BLOCK CacheBlock)
{

	DbgPrint((DPRINT_CACHE, "CacheInternalOptimizeBlockList()\n"));

	// Don't do this if this block is already at the head of the list
	if (CacheBlock != CacheDrive->CacheBlockHead)
	{
		// Remove this item from the block list
		RtlListRemoveEntry((PLIST_ITEM)CacheBlock);

		// Re-insert it at the head of the list
		RtlListInsertHead((PLIST_ITEM)CacheDrive->CacheBlockHead, (PLIST_ITEM)CacheBlock);

		// Update the head pointer
		CacheDrive->CacheBlockHead = CacheBlock;
	}
}
