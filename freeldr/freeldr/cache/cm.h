/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
#include <rtl.h>
#include <disk.h>

#ifndef __CM_H
#define __CM_H

///////////////////////////////////////////////////////////////////////////////////////
//
// This structure describes a cached block element. The disk is divided up into
// cache blocks. For disks which LBA is not supported each block is the size of
// one track. This will force the cache manager to make track sized reads, and
// therefore maximizes throughput. For disks which support LBA the block size
// is 64k because they have no cylinder, head, or sector boundaries.
//
///////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	LIST_ITEM	ListEntry;					// Doubly linked list synchronization member

	U32			BlockNumber;				// Track index for CHS, 64k block index for LBA
	BOOL		LockedInCache;				// Indicates that this block is locked in cache memory
	U32			AccessCount;				// Access count for this block

	PVOID		BlockData;					// Pointer to block data

} CACHE_BLOCK, *PCACHE_BLOCK;

///////////////////////////////////////////////////////////////////////////////////////
//
// This structure describes a cached drive. It contains the BIOS drive number
// and indicates whether or not LBA is supported. If LBA is not supported then
// the drive's geometry is described here.
//
///////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	U32					DriveNumber;
	GEOMETRY			DriveGeometry;

	U32					BlockSize;			// Block size (in sectors)
	PCACHE_BLOCK		CacheBlockHead;

} CACHE_DRIVE, *PCACHE_DRIVE;


///////////////////////////////////////////////////////////////////////////////////////
//
// Internal data
//
///////////////////////////////////////////////////////////////////////////////////////
extern	CACHE_DRIVE		CacheManagerDrive;
extern	BOOL			CacheManagerInitialized;
extern	U32				CacheBlockCount;
extern	U32				CacheSizeLimit;
extern	U32				CacheSizeCurrent;

///////////////////////////////////////////////////////////////////////////////////////
//
// Internal functions
//
///////////////////////////////////////////////////////////////////////////////////////
PCACHE_BLOCK	CacheInternalGetBlockPointer(PCACHE_DRIVE CacheDrive, U32 BlockNumber);				// Returns a pointer to a CACHE_BLOCK structure given a block number
PCACHE_BLOCK	CacheInternalFindBlock(PCACHE_DRIVE CacheDrive, U32 BlockNumber);					// Searches the block list for a particular block
PCACHE_BLOCK	CacheInternalAddBlockToCache(PCACHE_DRIVE CacheDrive, U32 BlockNumber);				// Adds a block to the cache's block list
BOOL			CacheInternalFreeBlock(PCACHE_DRIVE CacheDrive);									// Removes a block from the cache's block list & frees the memory
VOID			CacheInternalCheckCacheSizeLimits(PCACHE_DRIVE CacheDrive);							// Checks the cache size limits to see if we can add a new block, if not calls CacheInternalFreeBlock()
VOID			CacheInternalDumpBlockList(PCACHE_DRIVE CacheDrive);								// Dumps the list of cached blocks to the debug output port
VOID			CacheInternalOptimizeBlockList(PCACHE_DRIVE CacheDrive, PCACHE_BLOCK CacheBlock);	// Moves the specified block to the head of the list


#endif // defined __CM_H
