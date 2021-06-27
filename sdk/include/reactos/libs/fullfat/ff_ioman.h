/*****************************************************************************
 *  FullFAT - High Performance, Thread-Safe Embedded FAT File-System         *
 *  Copyright (C) 2009  James Walmsley (james@worm.me.uk)                    *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  IMPORTANT NOTICE:                                                        *
 *  =================                                                        *
 *  Alternative Licensing is available directly from the Copyright holder,   *
 *  (James Walmsley). For more information consult LICENSING.TXT to obtain   *
 *  a Commercial license.                                                    *
 *                                                                           *
 *  See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.       *
 *                                                                           *
 *  Removing the above notice is illegal and will invalidate this license.   *
 *****************************************************************************
 *  See http://worm.me.uk/fullfat for more information.                      *
 *  Or  http://fullfat.googlecode.com/ for latest releases and the wiki.     *
 *****************************************************************************/

/**
 *	@file		ff_ioman.h
 *	@author		James Walmsley
 *	@ingroup	IOMAN
 **/

#ifndef _FF_IOMAN_H_
#define _FF_IOMAN_H_

#include <stdlib.h>							// Use of malloc()
#include "ff_error.h"
#include "ff_config.h"
#include "ff_types.h"
#include "ff_safety.h"						// Provide thread-safety via semaphores.
#include "ff_memory.h"						// Memory access routines for ENDIAN independence.
#include "ff_hash.h"

//#define	FF_MAX_PARTITION_NAME	5		///< Partition name length.

#define FF_T_FAT12				0x0A
#define FF_T_FAT16				0x0B
#define FF_T_FAT32				0x0C

#define FF_MODE_READ			0x01		///< Buffer / FILE Mode for Read Access.
#define	FF_MODE_WRITE			0x02		///< Buffer / FILE Mode for Write Access.
#define FF_MODE_APPEND			0x04		///< FILE Mode Append Access.
#define	FF_MODE_CREATE			0x08		///< FILE Mode Create file if not existing.
#define FF_MODE_TRUNCATE		0x10		///< FILE Mode Truncate an Existing file.
#define FF_MODE_DIR				0x80		///< Special Mode to open a Dir. (Internal use ONLY!)

#define FF_BUF_MAX_HANDLES		0xFFFF		///< Maximum number handles sharing a buffer. (16 bit integer, we don't want to overflow it!)

/**
 *	I/O Driver Definitions
 *	Provide access to any Block Device via the following interfaces.
 *	Returns the number of blocks actually read or written.
 **/

/**
 *	A special information structure for the FullFAT mass storage device
 *	driver model.
 **/
typedef struct {
	FF_T_UINT16 BlkSize;
	FF_T_UINT32	TotalBlocks;
} FF_DEVICE_INFO;

typedef FF_T_SINT32 (*FF_WRITE_BLOCKS)	(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);
typedef FF_T_SINT32 (*FF_READ_BLOCKS)	(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);

#define FF_ERR_DRIVER_BUSY			-10
#define FF_ERR_DRIVER_FATAL_ERROR	-11

/**
 *	@public
 *	@brief	Describes the block device driver interface to FullFAT.
 **/
typedef struct {
	FF_WRITE_BLOCKS	fnpWriteBlocks;	///< Function Pointer, to write a block(s) from a block device.
	FF_READ_BLOCKS	fnpReadBlocks;	///< Function Pointer, to read a block(s) from a block device.
	FF_T_UINT16		devBlkSize;		///< Block size that the driver deals with.
	void			*pParam;		///< Pointer to some parameters e.g. for a Low-Level Driver Handle
} FF_BLK_DEVICE;

/**
 *	@private
 *	@brief	FullFAT handles memory with buffers, described as below.
 *	@note	This may change throughout development.
 **/
typedef struct {
	FF_T_UINT32		Sector;			///< The LBA of the Cached sector.
	FF_T_UINT32		LRU;			///< For the Least Recently Used algorithm.
	FF_T_UINT16		NumHandles;		///< Number of objects using this buffer.
	FF_T_UINT16		Persistance;	///< For the persistance algorithm.
	FF_T_UINT8		Mode;			///< Read or Write mode.
	FF_T_BOOL		Modified;		///< If the sector was modified since read.
	FF_T_BOOL		Valid;			///< Initially FALSE.
	FF_T_UINT8		*pBuffer;		///< Pointer to the cache block.
} FF_BUFFER;

typedef struct {
#ifdef FF_UNICODE_SUPPORT
	FF_T_WCHAR	Path[FF_MAX_PATH];
#else
	FF_T_INT8	Path[FF_MAX_PATH];
#endif
	FF_T_UINT32	DirCluster;
} FF_PATHCACHE;

#ifdef FF_HASH_CACHE
typedef struct {
	FF_T_UINT32		ulDirCluster;	///< The Starting Cluster of the dir that the hash represents.
	FF_HASH_TABLE	pHashTable;		///< Pointer to the Hash Table object.
	FF_T_UINT32		ulNumHandles;	///< Number of active Handles using this hash table.
	FF_T_UINT32		ulMisses;		///< Number of times this Hash Table was missed, (i.e. how redundant it is).
} FF_HASHCACHE;
#endif

/**
 *	@private
 *	@brief	FullFAT identifies a partition with the following data.
 *	@note	This may shrink as development and optimisation goes on.
 **/
typedef struct {
	//FF_T_UINT8		ID;					///< Partition Incremental ID number.
	FF_T_UINT8		Type;				///< Partition Type Identifier.
	FF_T_UINT16		BlkSize;			///< Size of a Sector Block in bytes.
	FF_T_UINT8      BlkFactor;			///< Scale Factor for blocksizes above 512!
	//FF_T_INT8		Name[FF_MAX_PARTITION_NAME];	///< Partition Identifier e.g. c: sd0: etc.
	FF_T_INT8		VolLabel[12];		///< Volume Label of the partition.
	FF_T_UINT32		BeginLBA;			///< LBA start address of the partition.
	FF_T_UINT32		PartSize;			///< Size of Partition in number of sectors.
	FF_T_UINT32		FatBeginLBA;		///< LBA of the FAT tables.
	FF_T_UINT8		NumFATS;			///< Number of FAT tables.
	FF_T_UINT32		SectorsPerFAT;		///< Number of sectors per Fat.
	FF_T_UINT8		SectorsPerCluster;	///< Number of sectors per Cluster.
	FF_T_UINT32		TotalSectors;
	FF_T_UINT32		DataSectors;
	FF_T_UINT32		RootDirSectors;
	FF_T_UINT32		FirstDataSector;
	FF_T_UINT16		ReservedSectors;
	FF_T_UINT32		ClusterBeginLBA;	///< LBA of first cluster.
	FF_T_UINT32		NumClusters;		///< Number of clusters.
	FF_T_UINT32		RootDirCluster;		///< Cluster number of the root directory entry.
	FF_T_UINT32		LastFreeCluster;
	FF_T_UINT32		FreeClusterCount;	///< Records free space on mount.
	FF_T_BOOL		PartitionMounted;	///< FF_TRUE if the partition is mounted, otherwise FF_FALSE.
#ifdef FF_PATH_CACHE
	FF_PATHCACHE	PathCache[FF_PATH_CACHE_DEPTH];
	FF_T_UINT32		PCIndex;
#endif
} FF_PARTITION;



/**
 *	@public
 *	@brief	FF_IOMAN Object description.
 *
 *	FullFAT functions around an object like this.
 **/
#define FF_FAT_LOCK			0x01	///< Lock bit mask for FAT table locking.
#define FF_DIR_LOCK			0x02	///< Lock bit mask for DIR modification locking.
//#define FF_PATHCACHE_LOCK	0x04

/**
 *	@public
 *	@brief	FF_IOMAN Object. A developer should not touch these values.
 *
 *	In the commercial version these values are encapsulated. In the open-source
 *	version they are left completely open, in case someone really "needs" :P to
 *	do something stupid and access their members themselves. Also to help the
 *	open-source community help me improve FullFAT, and aid understanding.
 *
 *	THIS WOULD BE VERY STUPID, SO DON'T DO IT. Unless your're writing a patch or
 *	something!
 *
 **/
typedef struct {
	FF_BLK_DEVICE	*pBlkDevice;		///< Pointer to a Block device description.
	FF_PARTITION	*pPartition;		///< Pointer to a partition description.
	FF_BUFFER		*pBuffers;			///< Pointer to the first buffer description.
	void			*pSemaphore;		///< Pointer to a Semaphore object. (For buffer description modifications only!).
#ifdef FF_BLKDEV_USES_SEM
	void			*pBlkDevSemaphore;	///< Semaphore to guarantee Atomic access to the underlying block device, if required.
#endif
	void			*FirstFile;			///< Pointer to the first File object.
	FF_T_UINT8		*pCacheMem;			///< Pointer to a block of memory for the cache.
	FF_T_UINT32		LastReplaced;		///< Marks which sector was last replaced in the cache.
	FF_T_UINT16		BlkSize;			///< The Block size that IOMAN is configured to.
	FF_T_UINT16		CacheSize;			///< Size of the cache in number of Sectors.
	FF_T_UINT8		PreventFlush;		///< Flushing to disk only allowed when 0
	FF_T_UINT8		MemAllocation;		///< Bit-Mask identifying allocated pointers.
	FF_T_UINT8		Locks;				///< Lock Flag for FAT & DIR Locking etc (This must be accessed via a semaphore).
#ifdef FF_HASH_CACHE
	FF_HASHCACHE	HashCache[FF_HASH_CACHE_DEPTH];
#endif
} FF_IOMAN;

// Bit-Masks for Memory Allocation testing.
#define FF_IOMAN_ALLOC_BLKDEV	0x01	///< Flags the pBlkDevice pointer is allocated.
#define FF_IOMAN_ALLOC_PART		0x02	///< Flags the pPartition pointer is allocated.
#define	FF_IOMAN_ALLOC_BUFDESCR	0x04	///< Flags the pBuffers pointer is allocated.
#define	FF_IOMAN_ALLOC_BUFFERS	0x08	///< Flags the pCacheMem pointer is allocated.
#define FF_IOMAN_ALLOC_RESERVED	0xF0	///< Reserved Section.


//---------- PROTOTYPES (in order of appearance)

// PUBLIC (Interfaces):
FF_IOMAN	*FF_CreateIOMAN			(FF_T_UINT8 *pCacheMem, FF_T_UINT32 Size, FF_T_UINT16 BlkSize, FF_ERROR *pError);
FF_ERROR	FF_DestroyIOMAN			(FF_IOMAN *pIoman);
FF_ERROR	FF_RegisterBlkDevice	(FF_IOMAN *pIoman, FF_T_UINT16 BlkSize, FF_WRITE_BLOCKS fnWriteBlocks, FF_READ_BLOCKS fnReadBlocks, void *pParam);
FF_ERROR	FF_UnregisterBlkDevice	(FF_IOMAN *pIoman);
FF_ERROR	FF_MountPartition		(FF_IOMAN *pIoman, FF_T_UINT8 PartitionNumber);
FF_ERROR	FF_UnmountPartition		(FF_IOMAN *pIoman);
FF_ERROR	FF_FlushCache			(FF_IOMAN *pIoman);
FF_T_SINT32 FF_GetPartitionBlockSize(FF_IOMAN *pIoman);

#ifdef FF_64_NUM_SUPPORT
FF_T_UINT64 FF_GetVolumeSize(FF_IOMAN *pIoman);
#else
FF_T_UINT32 FF_GetVolumeSize(FF_IOMAN *pIoman);
#endif

// PUBLIC  (To FullFAT Only):
FF_T_SINT32 FF_BlockRead			(FF_IOMAN *pIoman, FF_T_UINT32 ulSectorLBA, FF_T_UINT32 ulNumSectors, void *pBuffer);
FF_T_SINT32 FF_BlockWrite			(FF_IOMAN *pIoman, FF_T_UINT32 ulSectorLBA, FF_T_UINT32 ulNumSectors, void *pBuffer);
FF_ERROR	FF_IncreaseFreeClusters	(FF_IOMAN *pIoman, FF_T_UINT32 Count);
FF_ERROR	FF_DecreaseFreeClusters	(FF_IOMAN *pIoman, FF_T_UINT32 Count);
FF_BUFFER	*FF_GetBuffer			(FF_IOMAN *pIoman, FF_T_UINT32 Sector, FF_T_UINT8 Mode);
void		FF_ReleaseBuffer		(FF_IOMAN *pIoman, FF_BUFFER *pBuffer);

// PRIVATE (For this module only!):


#endif


